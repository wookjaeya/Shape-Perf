// dwarf_line.cpp -- see dwarf_line.hpp.
#include "dwarf_line.hpp"

#include <algorithm>
#include <cstring>
#include <map>

namespace kz::anchor {

namespace {

// Forms and content types that the entry formats of DWARF 5 may use.
enum : uint32_t {
  DW_FORM_block = 0x09, DW_FORM_data1 = 0x0b, DW_FORM_data2 = 0x05, DW_FORM_data4 = 0x06, DW_FORM_data8 = 0x07,
  DW_FORM_data16 = 0x1e, DW_FORM_string = 0x08, DW_FORM_strp = 0x0e, DW_FORM_line_strp = 0x1f, DW_FORM_udata = 0x0f,
  DW_FORM_sdata = 0x0d, DW_FORM_block1 = 0x0a, DW_FORM_block2 = 0x03, DW_FORM_block4 = 0x04
};
enum : uint32_t { DW_LNCT_path = 1, DW_LNCT_directory_index = 2 };

enum : uint8_t {
  DW_LNS_copy = 1, DW_LNS_advance_pc = 2, DW_LNS_advance_line = 3, DW_LNS_set_file = 4, DW_LNS_set_column = 5,
  DW_LNS_negate_stmt = 6, DW_LNS_set_basic_block = 7, DW_LNS_const_add_pc = 8, DW_LNS_fixed_advance_pc = 9,
  DW_LNS_set_prologue_end = 10, DW_LNS_set_epilogue_begin = 11, DW_LNS_set_isa = 12
};
enum : uint8_t { DW_LNE_end_sequence = 1, DW_LNE_set_address = 2, DW_LNE_define_file = 3, DW_LNE_set_discriminator = 4 };

struct Reader {
  const uint8_t* p;
  const uint8_t* end;
  bool ok = true;
  size_t left() const { return static_cast<size_t>(end - p); }
  bool need(size_t n) {
    if (!ok || left() < n) {
      ok = false;
      return false;
    }
    return true;
  }
  uint8_t u8() { return need(1) ? *p++ : 0; }
  uint16_t u16() {
    if (!need(2)) return 0;
    uint16_t v;
    std::memcpy(&v, p, 2);
    p += 2;
    return v;
  }
  uint32_t u32() {
    if (!need(4)) return 0;
    uint32_t v;
    std::memcpy(&v, p, 4);
    p += 4;
    return v;
  }
  uint64_t u64() {
    if (!need(8)) return 0;
    uint64_t v;
    std::memcpy(&v, p, 8);
    p += 8;
    return v;
  }
  uint64_t uleb() {
    uint64_t v = 0;
    unsigned shift = 0;
    for (;;) {
      if (!need(1)) return 0;
      const uint8_t b = *p++;
      if (shift < 64) v |= static_cast<uint64_t>(b & 0x7f) << shift;
      if ((b & 0x80) == 0) break;
      shift += 7;
      if (shift > 70) {
        ok = false;
        return 0;
      }
    }
    return v;
  }
  int64_t sleb() {
    int64_t v = 0;
    unsigned shift = 0;
    uint8_t b = 0;
    for (;;) {
      if (!need(1)) return 0;
      b = *p++;
      if (shift < 64) v |= static_cast<int64_t>(b & 0x7f) << shift;
      shift += 7;
      if ((b & 0x80) == 0) break;
      if (shift > 70) {
        ok = false;
        return 0;
      }
    }
    if (shift < 64 && (b & 0x40) != 0) v |= -(static_cast<int64_t>(1) << shift);
    return v;
  }
  std::string cstr() {
    if (!ok) return std::string();
    const uint8_t* nul = static_cast<const uint8_t*>(std::memchr(p, 0, left()));
    if (nul == nullptr) {
      ok = false;
      return std::string();
    }
    std::string s(reinterpret_cast<const char*>(p), static_cast<size_t>(nul - p));
    p = nul + 1;
    return s;
  }
  void skip(uint64_t n) {
    if (!ok || n > left()) {
      ok = false;
      return;
    }
    p += n;
  }
};

bool string_at(const Slice& table, uint64_t off, std::string* out) {
  if (off >= table.size) return false;
  const uint8_t* start = table.data + off;
  const uint8_t* nul = static_cast<const uint8_t*>(std::memchr(start, 0, table.size - off));
  if (nul == nullptr) return false;
  out->assign(reinterpret_cast<const char*>(start), static_cast<size_t>(nul - start));
  return true;
}

struct EntryFormat {
  uint32_t content = 0, form = 0;
};

// One entry of a DWARF 5 directory or file table: the strings and numbers the line program header
// carries for it.
struct Entry {
  std::string path;
  uint64_t dir = 0;
};

Status read_entries_v5(Reader& r, bool dwarf64, const Slice& line_str, const Slice& str, std::vector<Entry>* out) {
  const uint8_t nformats = r.u8();
  std::vector<EntryFormat> formats(nformats);
  for (EntryFormat& f : formats) {
    f.content = static_cast<uint32_t>(r.uleb());
    f.form = static_cast<uint32_t>(r.uleb());
  }
  const uint64_t count = r.uleb();
  if (!r.ok) return Status::error(Err::Truncated, "line program header: entry format is cut off");
  if (count > (1u << 20)) return Status::error(Err::Invalid, "line program header: implausible entry count");
  for (uint64_t i = 0; i < count; ++i) {
    Entry e;
    for (const EntryFormat& f : formats) {
      std::string text;
      uint64_t num = 0;
      bool is_text = false;
      switch (f.form) {
        case DW_FORM_string: text = r.cstr(); is_text = true; break;
        case DW_FORM_line_strp:
        case DW_FORM_strp: {
          const uint64_t off = dwarf64 ? r.u64() : r.u32();
          is_text = true;
          if (r.ok && !string_at(f.form == DW_FORM_line_strp ? line_str : str, off, &text))
            return Status::error(Err::Invalid, "line program header: string offset outside .debug_line_str/.debug_str");
          break;
        }
        case DW_FORM_udata: num = r.uleb(); break;
        case DW_FORM_sdata: num = static_cast<uint64_t>(r.sleb()); break;
        case DW_FORM_data1: num = r.u8(); break;
        case DW_FORM_data2: num = r.u16(); break;
        case DW_FORM_data4: num = r.u32(); break;
        case DW_FORM_data8: num = r.u64(); break;
        case DW_FORM_data16: r.skip(16); break;
        case DW_FORM_block: r.skip(r.uleb()); break;
        case DW_FORM_block1: r.skip(r.u8()); break;
        case DW_FORM_block2: r.skip(r.u16()); break;
        case DW_FORM_block4: r.skip(r.u32()); break;
        default:
          return Status::error(Err::Invalid, "line program header: unsupported entry form 0x" + std::to_string(f.form));
      }
      if (f.content == DW_LNCT_path && is_text) e.path = text;
      if (f.content == DW_LNCT_directory_index && !is_text) e.dir = num;
    }
    if (!r.ok) return Status::error(Err::Truncated, "line program header: entry table is cut off");
    out->push_back(std::move(e));
  }
  return Status::success();
}

std::string join_path(const std::string& dir, const std::string& name) {
  if (name.empty()) return dir;
  if (name[0] == '/' || dir.empty()) return name;
  return dir + "/" + name;
}

struct Sequence {
  std::vector<LineRow> rows;
  uint64_t start = 0;
};

}  // namespace

Status LineTable::parse(const ElfImage& elf, LineTable* out) {
  *out = LineTable();
  const Slice line = elf.section(".debug_line");
  const Slice line_str = elf.section(".debug_line_str");
  const Slice str = elf.section(".debug_str");
  if (line.empty()) return Status::success();

  std::map<std::string, uint32_t> file_ids;  // path -> id, for a deterministic numbering across units
  auto file_id = [&](const std::string& path) {
    auto it = file_ids.find(path);
    if (it != file_ids.end()) return it->second;
    const uint32_t id = static_cast<uint32_t>(out->files_.size());
    out->files_.push_back(path);
    file_ids.emplace(path, id);
    return id;
  };

  std::vector<Sequence> sequences;
  Reader top{line.data, line.data + line.size};
  while (top.left() > 0) {
    // ---- unit header
    uint64_t unit_length = top.u32();
    bool dwarf64 = false;
    if (unit_length == 0xffffffffu) {
      dwarf64 = true;
      unit_length = top.u64();
    } else if (unit_length >= 0xfffffff0u) {
      return Status::error(Err::Invalid, ".debug_line: reserved unit length");
    }
    if (!top.ok || unit_length > top.left()) return Status::error(Err::Truncated, ".debug_line: a unit runs past the section");
    Reader u{top.p, top.p + unit_length};
    top.skip(unit_length);

    const uint16_t version = u.u16();
    if (version < 2 || version > 5) return Status::error(Err::Invalid, ".debug_line: unsupported version " + std::to_string(version));
    if (version >= 5) {
      const uint8_t address_size = u.u8();
      const uint8_t seg_selector_size = u.u8();
      if (address_size != 8 || seg_selector_size != 0) return Status::error(Err::Invalid, ".debug_line: only 8-byte addresses and no segment selectors are supported");
    }
    const uint64_t header_length = dwarf64 ? u.u64() : u.u32();
    if (!u.ok || header_length > u.left()) return Status::error(Err::Truncated, ".debug_line: header length runs past the unit");
    const uint8_t* program = u.p + header_length;
    const uint8_t min_inst_length = u.u8();
    uint8_t max_ops = 1;
    if (version >= 4) max_ops = u.u8();
    const bool default_is_stmt = u.u8() != 0;
    const int8_t line_base = static_cast<int8_t>(u.u8());
    const uint8_t line_range = u.u8();
    const uint8_t opcode_base = u.u8();
    if (!u.ok) return Status::error(Err::Truncated, ".debug_line: header is cut off");
    if (max_ops != 1) return Status::error(Err::Invalid, ".debug_line: VLIW (max_ops_per_inst != 1) is not supported");
    if (line_range == 0) return Status::error(Err::Invalid, ".debug_line: line_range is 0");
    std::vector<uint8_t> opcode_len(opcode_base == 0 ? 0 : opcode_base - 1);
    for (uint8_t& l : opcode_len) l = u.u8();

    // ---- directory and file tables -> this unit's file ids (index = the program's file register)
    std::vector<std::string> unit_dirs;
    std::vector<uint32_t> unit_files;  // file register -> id (v5: 0-based; earlier: slot 0 unused)
    if (version >= 5) {
      std::vector<Entry> dirs, files;
      Status s = read_entries_v5(u, dwarf64, line_str, str, &dirs);
      if (!s.ok()) return s;
      s = read_entries_v5(u, dwarf64, line_str, str, &files);
      if (!s.ok()) return s;
      for (const Entry& d : dirs) unit_dirs.push_back(d.path);
      for (const Entry& f : files) {
        const std::string dir = f.dir < unit_dirs.size() ? unit_dirs[f.dir] : std::string();
        unit_files.push_back(file_id(join_path(dir, f.path)));
      }
    } else {
      unit_dirs.push_back(std::string());  // directory 0 = the compilation directory (unknown here)
      for (;;) {
        std::string d = u.cstr();
        if (!u.ok) return Status::error(Err::Truncated, ".debug_line: include directories are cut off");
        if (d.empty()) break;
        unit_dirs.push_back(std::move(d));
      }
      unit_files.push_back(KZ_NONE32);
      for (;;) {
        std::string name = u.cstr();
        if (!u.ok) return Status::error(Err::Truncated, ".debug_line: file names are cut off");
        if (name.empty()) break;
        const uint64_t dir = u.uleb();
        (void)u.uleb();
        (void)u.uleb();
        if (!u.ok) return Status::error(Err::Truncated, ".debug_line: file entry is cut off");
        unit_files.push_back(file_id(join_path(dir < unit_dirs.size() ? unit_dirs[dir] : std::string(), name)));
      }
    }

    // ---- the program
    // The program runs from the end of the header to the end of the unit.
    if (program > u.end) return Status::error(Err::Truncated, ".debug_line: the header runs past the unit");
    Reader p{program, u.end};

    struct State {
      uint64_t addr = 0;
      uint32_t file = 1, line = 1, col = 0;
      bool is_stmt = false;
    };
    State st;
    st.is_stmt = default_is_stmt;
    Sequence cur;
    bool have_cur = false;
    auto reset = [&] {
      st = State();
      st.file = 1;
      st.line = 1;
      st.is_stmt = default_is_stmt;
    };
    auto emit = [&](bool end_seq) -> Status {
      LineRow row;
      row.addr = st.addr;
      row.line = st.line;
      row.col = st.col;
      row.is_stmt = st.is_stmt;
      row.end_sequence = end_seq;
      // The file register names a slot of the unit's table: 0-based in DWARF 5, 1-based before.
      if (st.file < unit_files.size() && unit_files[st.file] != KZ_NONE32) {
        row.file = unit_files[st.file];
      } else {
        return Status::error(Err::Invalid, ".debug_line: a row names a file that the header does not list");
      }
      if (!have_cur) {
        cur = Sequence();
        cur.start = row.addr;
        have_cur = true;
      }
      cur.rows.push_back(row);
      if (end_seq) {
        sequences.push_back(std::move(cur));
        cur = Sequence();
        have_cur = false;
      }
      return Status::success();
    };
    while (p.left() > 0) {
      const uint8_t op = p.u8();
      if (op >= opcode_base) {
        const uint8_t adj = static_cast<uint8_t>(op - opcode_base);
        st.addr += static_cast<uint64_t>(adj / line_range) * min_inst_length;
        st.line = static_cast<uint32_t>(static_cast<int64_t>(st.line) + line_base + (adj % line_range));
        Status s = emit(false);
        if (!s.ok()) return s;
      } else if (op == 0) {
        const uint64_t len = p.uleb();
        if (!p.ok || len == 0 || len > p.left()) return Status::error(Err::Truncated, ".debug_line: extended opcode runs past the unit");
        const uint8_t* next = p.p + len;
        const uint8_t sub = p.u8();
        switch (sub) {
          case DW_LNE_end_sequence: {
            Status s = emit(true);
            if (!s.ok()) return s;
            reset();
            break;
          }
          case DW_LNE_set_address:
            if (len - 1 != 8) return Status::error(Err::Invalid, ".debug_line: set_address with a non-8-byte operand");
            st.addr = p.u64();
            break;
          case DW_LNE_define_file: {
            std::string name = p.cstr();
            const uint64_t dir = p.uleb();
            (void)p.uleb();
            (void)p.uleb();
            if (p.ok) unit_files.push_back(file_id(join_path(dir < unit_dirs.size() ? unit_dirs[dir] : std::string(), name)));
            break;
          }
          default:
            break;  // set_discriminator and vendor extensions carry nothing the anchors need
        }
        if (!p.ok) return Status::error(Err::Truncated, ".debug_line: extended opcode is cut off");
        p.p = next;
      } else {
        switch (op) {
          case DW_LNS_copy: { Status s = emit(false); if (!s.ok()) return s; break; }
          case DW_LNS_advance_pc: st.addr += p.uleb() * min_inst_length; break;
          case DW_LNS_advance_line: st.line = static_cast<uint32_t>(static_cast<int64_t>(st.line) + p.sleb()); break;
          case DW_LNS_set_file: st.file = static_cast<uint32_t>(p.uleb()); break;
          case DW_LNS_set_column: st.col = static_cast<uint32_t>(p.uleb()); break;
          case DW_LNS_negate_stmt: st.is_stmt = !st.is_stmt; break;
          case DW_LNS_set_basic_block: break;
          case DW_LNS_const_add_pc: st.addr += static_cast<uint64_t>((255 - opcode_base) / line_range) * min_inst_length; break;
          case DW_LNS_fixed_advance_pc: st.addr += p.u16(); break;
          case DW_LNS_set_prologue_end: break;
          case DW_LNS_set_epilogue_begin: break;
          case DW_LNS_set_isa: (void)p.uleb(); break;
          default:
            // An opcode this reader does not know: its operand count is in the header.
            for (uint8_t k = 0; k < opcode_len[op - 1]; ++k) (void)p.uleb();
            break;
        }
      }
      if (!p.ok) return Status::error(Err::Truncated, ".debug_line: the program is cut off");
    }
    if (have_cur) return Status::error(Err::Invalid, ".debug_line: a sequence has no end_sequence");
  }

  std::stable_sort(sequences.begin(), sequences.end(), [](const Sequence& a, const Sequence& b) { return a.start < b.start; });
  for (const Sequence& s : sequences) out->rows_.insert(out->rows_.end(), s.rows.begin(), s.rows.end());
  return Status::success();
}

std::vector<uint32_t> LineTable::files_matching(const std::string& suffix) const {
  std::vector<uint32_t> ids;
  for (uint32_t i = 0; i < files_.size(); ++i) {
    const std::string& f = files_[i];
    if (f.size() < suffix.size()) continue;
    if (f.compare(f.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
    // a match must start at a path component: the whole path, or after a '/'
    if (f.size() == suffix.size() || f[f.size() - suffix.size() - 1] == '/' || (!suffix.empty() && suffix[0] == '/')) ids.push_back(i);
  }
  return ids;
}

std::vector<LineRange> LineTable::ranges_of(const std::vector<uint32_t>& file_ids, uint32_t line) const {
  std::vector<LineRange> out;
  auto wanted = [&](uint32_t f) { return std::find(file_ids.begin(), file_ids.end(), f) != file_ids.end(); };
  const size_t n = rows_.size();
  size_t i = 0;
  while (i < n) {
    const LineRow& r = rows_[i];
    if (r.end_sequence) {
      ++i;
      continue;
    }
    // Extend over the consecutive rows of the same (file, line) in this sequence.
    size_t j = i;
    bool stmt = false;
    while (j < n && !rows_[j].end_sequence && rows_[j].file == r.file && rows_[j].line == r.line) {
      stmt = stmt || rows_[j].is_stmt;
      ++j;
    }
    // rows_[j] is the first row of something else, or the end_sequence row: its address ends the range.
    if (j < n && wanted(r.file) && r.line == line && stmt && rows_[j].addr > r.addr) {
      LineRange lr;
      lr.begin = r.addr;
      lr.end = rows_[j].addr;
      lr.file = r.file;
      lr.line = r.line;
      lr.col = r.col;
      out.push_back(lr);
    }
    i = j;
  }
  return out;
}

const LineRow* LineTable::row_at(uint64_t addr) const {
  // Rows are ordered by sequence start; within a sequence by address.  Scan for the covering row.
  const LineRow* best = nullptr;
  for (size_t i = 0; i + 1 < rows_.size(); ++i) {
    const LineRow& r = rows_[i];
    if (r.end_sequence) continue;
    const LineRow& next = rows_[i + 1];
    if (r.addr <= addr && addr < next.addr) best = &r;
  }
  return best;
}

}  // namespace kz::anchor

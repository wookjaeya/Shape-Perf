// elf_synth.hpp -- builds small ELF64 x86-64 files and DWARF line programs byte by byte, for the
// tests of the ELF and line-table readers.  Independent of the readers; values are unit-test values.
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <elf.h>

namespace kzsyn {

using Buf = std::vector<uint8_t>;

inline void put(Buf& b, const void* p, size_t n) {
  const uint8_t* q = static_cast<const uint8_t*>(p);
  b.insert(b.end(), q, q + n);
}
inline void put8(Buf& b, uint8_t v) { b.push_back(v); }
inline void put16(Buf& b, uint16_t v) { put(b, &v, 2); }
inline void put32(Buf& b, uint32_t v) { put(b, &v, 4); }
inline void put64(Buf& b, uint64_t v) { put(b, &v, 8); }
inline void puts0(Buf& b, const std::string& s) { put(b, s.c_str(), s.size() + 1); }
inline void uleb(Buf& b, uint64_t v) {
  do {
    uint8_t c = v & 0x7f;
    v >>= 7;
    if (v != 0) c |= 0x80;
    b.push_back(c);
  } while (v != 0);
}
inline void sleb(Buf& b, int64_t v) {
  bool more = true;
  while (more) {
    uint8_t c = v & 0x7f;
    v >>= 7;
    if ((v == 0 && (c & 0x40) == 0) || (v == -1 && (c & 0x40) != 0)) more = false;
    else c |= 0x80;
    b.push_back(c);
  }
}

// A line-number program in the form of a gcc/clang compilation unit.
struct LineProgram {
  int version = 5;
  bool dwarf64 = false;
  uint8_t min_inst = 1, max_ops = 1;
  bool default_is_stmt = true;
  int8_t line_base = -5;
  uint8_t line_range = 14, opcode_base = 13;
  std::vector<std::string> dirs;               // v5: entry 0 is the compilation directory; v<5: directories 1..
  std::vector<std::pair<std::string, int>> files;  // (name, directory index)
  bool use_line_strp = false;                  // v5: store the strings in .debug_line_str instead of inline
  Buf program;                                 // opcodes
  Buf* line_str = nullptr;                     // receives the strings when use_line_strp

  void extended(uint8_t op, const Buf& operand) {
    put8(program, 0);
    uleb(program, 1 + operand.size());
    put8(program, op);
    put(program, operand.data(), operand.size());
  }
  void set_address(uint64_t a) { Buf o; put64(o, a); extended(2, o); }
  void end_sequence() { extended(1, Buf()); }
  void copy() { put8(program, 1); }
  void advance_pc(uint64_t n) { put8(program, 2); uleb(program, n); }
  void advance_line(int64_t n) { put8(program, 3); sleb(program, n); }
  void set_file(uint64_t n) { put8(program, 4); uleb(program, n); }
  void set_column(uint64_t n) { put8(program, 5); uleb(program, n); }
  void negate_stmt() { put8(program, 6); }
  void const_add_pc() { put8(program, 8); }
  void fixed_advance_pc(uint16_t n) { put8(program, 9); put16(program, n); }
  void special(uint8_t op) { put8(program, op); }

  // The whole unit: length, header, tables, program.
  Buf build() const {
    Buf header_rest;  // after header_length
    put8(header_rest, min_inst);
    if (version >= 4) put8(header_rest, max_ops);
    put8(header_rest, default_is_stmt ? 1 : 0);
    put8(header_rest, static_cast<uint8_t>(line_base));
    put8(header_rest, line_range);
    put8(header_rest, opcode_base);
    static const uint8_t lens[12] = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
    for (int i = 1; i < opcode_base; ++i) put8(header_rest, i <= 12 ? lens[i - 1] : 1);
    if (version >= 5) {
      auto str_entry = [&](Buf& b, const std::string& s) {
        if (use_line_strp && line_str != nullptr) {
          const uint32_t off = static_cast<uint32_t>(line_str->size());
          puts0(*line_str, s);
          if (dwarf64) put64(b, off); else put32(b, off);
        } else {
          puts0(b, s);
        }
      };
      const uint32_t form = use_line_strp && line_str != nullptr ? 0x1f : 0x08;
      put8(header_rest, 1);  // directory_entry_format_count
      uleb(header_rest, 1);  // DW_LNCT_path
      uleb(header_rest, form);
      uleb(header_rest, dirs.size());
      for (const std::string& d : dirs) str_entry(header_rest, d);
      put8(header_rest, 2);  // file_name_entry_format_count
      uleb(header_rest, 1);
      uleb(header_rest, form);
      uleb(header_rest, 2);  // DW_LNCT_directory_index
      uleb(header_rest, 0x0f);  // DW_FORM_udata
      uleb(header_rest, files.size());
      for (const auto& f : files) {
        str_entry(header_rest, f.first);
        uleb(header_rest, static_cast<uint64_t>(f.second));
      }
    } else {
      for (size_t i = 1; i < dirs.size(); ++i) puts0(header_rest, dirs[i]);
      put8(header_rest, 0);
      for (const auto& f : files) {
        puts0(header_rest, f.first);
        uleb(header_rest, static_cast<uint64_t>(f.second));
        uleb(header_rest, 0);
        uleb(header_rest, 0);
      }
      put8(header_rest, 0);
    }
    Buf body;
    put16(body, static_cast<uint16_t>(version));
    if (version >= 5) { put8(body, 8); put8(body, 0); }
    if (dwarf64) put64(body, header_rest.size()); else put32(body, static_cast<uint32_t>(header_rest.size()));
    put(body, header_rest.data(), header_rest.size());
    put(body, program.data(), program.size());
    Buf unit;
    if (dwarf64) { put32(unit, 0xffffffffu); put64(unit, body.size()); }
    else put32(unit, static_cast<uint32_t>(body.size()));
    put(unit, body.data(), body.size());
    return unit;
  }
};

struct SynthFunc {
  std::string name;
  uint64_t vaddr;
  uint64_t size;
  uint8_t bind = STB_GLOBAL;
};

// An ET_DYN file with one PT_LOAD (file offset == virtual address), a .text of given bytes at
// `text_vaddr`, a symbol table, a line table and an optional GNU build id.
struct SynthElf {
  uint64_t text_vaddr = 0x1000;
  Buf text;
  std::vector<SynthFunc> funcs;
  Buf debug_line, debug_line_str, debug_str;
  std::string build_id;  // raw bytes (not hex); empty = no note
  bool with_symtab = true;

  Buf build() const {
    struct Sec { std::string name; uint32_t type; uint64_t flags; Buf data; uint32_t link = 0; uint32_t info = 0; uint64_t entsize = 0; uint64_t addr = 0; };
    std::vector<Sec> secs;
    secs.push_back({"", SHT_NULL, 0, {}});
    Sec t{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, text};
    t.addr = text_vaddr;
    secs.push_back(t);
    int symtab_index = -1;
    if (with_symtab) {
      Buf strtab;
      strtab.push_back(0);
      Buf symtab(sizeof(Elf64_Sym), 0);  // the null symbol
      for (const SynthFunc& f : funcs) {
        Elf64_Sym s;
        std::memset(&s, 0, sizeof s);
        s.st_name = static_cast<uint32_t>(strtab.size());
        puts0(strtab, f.name);
        s.st_info = static_cast<uint8_t>(ELF64_ST_INFO(f.bind, STT_FUNC));
        s.st_shndx = 1;
        s.st_value = f.vaddr;
        s.st_size = f.size;
        put(symtab, &s, sizeof s);
      }
      symtab_index = static_cast<int>(secs.size());
      Sec st{".symtab", SHT_SYMTAB, 0, symtab};
      st.link = symtab_index + 1;
      st.info = 1;
      st.entsize = sizeof(Elf64_Sym);
      secs.push_back(st);
      secs.push_back({".strtab", SHT_STRTAB, 0, strtab});
    }
    if (!debug_line.empty()) secs.push_back({".debug_line", SHT_PROGBITS, 0, debug_line});
    if (!debug_line_str.empty()) secs.push_back({".debug_line_str", SHT_PROGBITS, 0, debug_line_str});
    if (!debug_str.empty()) secs.push_back({".debug_str", SHT_PROGBITS, 0, debug_str});
    if (!build_id.empty()) {
      Buf note;
      put32(note, 4);
      put32(note, static_cast<uint32_t>(build_id.size()));
      put32(note, NT_GNU_BUILD_ID);
      put(note, "GNU\0", 4);
      put(note, build_id.data(), build_id.size());
      while (note.size() % 4 != 0) note.push_back(0);
      secs.push_back({".note.gnu.build-id", SHT_NOTE, SHF_ALLOC, note});
    }
    Buf shstr;
    shstr.push_back(0);
    std::vector<uint32_t> name_off;
    for (const Sec& s : secs) {
      name_off.push_back(s.name.empty() ? 0 : static_cast<uint32_t>(shstr.size()));
      if (!s.name.empty()) puts0(shstr, s.name);
    }
    const uint32_t shstr_name = static_cast<uint32_t>(shstr.size());
    puts0(shstr, ".shstrtab");
    secs.push_back({".shstrtab", SHT_STRTAB, 0, shstr});
    name_off.push_back(shstr_name);

    // Layout: ehdr, phdr, then the sections; .text must sit at file offset == vaddr.
    Buf file(sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr), 0);
    std::vector<uint64_t> offsets(secs.size(), 0);
    for (size_t i = 1; i < secs.size(); ++i) {
      if (secs[i].name == ".text") {
        if (file.size() > text_vaddr) file.resize(file.size());  // (the caller picks text_vaddr beyond the headers)
        file.resize(text_vaddr, 0);
      }
      while (file.size() % 8 != 0) file.push_back(0);
      offsets[i] = file.size();
      put(file, secs[i].data.data(), secs[i].data.size());
    }
    while (file.size() % 8 != 0) file.push_back(0);
    const uint64_t shoff = file.size();
    for (size_t i = 0; i < secs.size(); ++i) {
      Elf64_Shdr h;
      std::memset(&h, 0, sizeof h);
      h.sh_name = name_off[i];
      h.sh_type = secs[i].type;
      h.sh_flags = secs[i].flags;
      h.sh_addr = secs[i].addr;
      h.sh_offset = offsets[i];
      h.sh_size = secs[i].data.size();
      h.sh_link = secs[i].link;
      h.sh_info = secs[i].info;
      h.sh_entsize = secs[i].entsize;
      put(file, &h, sizeof h);
    }
    Elf64_Ehdr eh;
    std::memset(&eh, 0, sizeof eh);
    std::memcpy(eh.e_ident, ELFMAG, SELFMAG);
    eh.e_ident[EI_CLASS] = ELFCLASS64;
    eh.e_ident[EI_DATA] = ELFDATA2LSB;
    eh.e_ident[EI_VERSION] = EV_CURRENT;
    eh.e_type = ET_DYN;
    eh.e_machine = EM_X86_64;
    eh.e_version = EV_CURRENT;
    eh.e_phoff = sizeof(Elf64_Ehdr);
    eh.e_shoff = shoff;
    eh.e_ehsize = sizeof(Elf64_Ehdr);
    eh.e_phentsize = sizeof(Elf64_Phdr);
    eh.e_phnum = 1;
    eh.e_shentsize = sizeof(Elf64_Shdr);
    eh.e_shnum = static_cast<uint16_t>(secs.size());
    eh.e_shstrndx = static_cast<uint16_t>(secs.size() - 1);
    Elf64_Phdr ph;
    std::memset(&ph, 0, sizeof ph);
    ph.p_type = PT_LOAD;
    ph.p_flags = PF_R | PF_X;
    ph.p_offset = 0;
    ph.p_vaddr = 0;
    ph.p_filesz = shoff;
    ph.p_memsz = shoff;
    ph.p_align = 0x1000;
    std::memcpy(file.data(), &eh, sizeof eh);
    std::memcpy(file.data() + sizeof eh, &ph, sizeof ph);
    return file;
  }
};

}  // namespace kzsyn

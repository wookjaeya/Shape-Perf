// anchor_resolve.cpp -- see anchor_resolve.hpp.
#include "anchor_resolve.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <vector>

namespace kz::anchor {

namespace {

constexpr uint8_t kEndbr64[4] = {0xf3, 0x0f, 0x1e, 0xfa};

void fill_insn(const ElfImage& elf, uint64_t file_offset, ResolvedTarget* t) {
  std::memset(t->insn, 0, sizeof t->insn);
  for (size_t n = sizeof t->insn; n > 0; --n)
    if (elf.read_offset(file_offset, n, t->insn)) {
      if (n < sizeof t->insn) std::memset(t->insn + n, 0, sizeof t->insn - n);
      return;
    }
}

// The addresses a call of `callee` can target: its function symbols and, for a function of another
// file (a shared library, or an exported function of a shared object itself), its PLT stub; `name` and
// `name@plt` both name the stub.
std::set<uint64_t> callee_addresses(const ElfImage& elf, const std::string& callee) {
  std::set<uint64_t> addrs;
  for (const ElfFunction* c : elf.find_functions(callee)) addrs.insert(c->vaddr);
  std::string base = callee;
  if (base.size() > 4 && base.compare(base.size() - 4, 4, "@plt") == 0) base.resize(base.size() - 4);
  const uint64_t stub = elf.plt_stub(base);
  if (stub != 0) addrs.insert(stub);
  return addrs;
}

// A return site that a direct branch of the function also reaches is a join of several paths: the
// probe would fire on paths that never made the call.  The scan reads every byte position of the
// function as a possible jmp/jcc (rel8 or rel32), so it cannot miss a direct branch; a byte pattern
// inside another instruction can only make it refuse a good site (conservative), never accept a bad
// one.  Indirect jumps (tables) are not seen; the anchor cross-check prints the code around the site.
// Returns the function offset of a branch to `ret`, or -1.
int64_t branch_to(const ElfImage& elf, const ElfFunction& in, uint64_t ret) {
  const uint64_t fsize = in.size;
  std::vector<uint8_t> code(fsize);
  if (fsize == 0 || !elf.read_vaddr(in.vaddr, fsize, code.data())) return -1;
  for (uint64_t p = 0; p < fsize; ++p) {
    const uint64_t at = in.vaddr + p;
    uint64_t target = 0;
    bool branch = false;
    const uint8_t b0 = code[p];
    if ((b0 == 0xeb || (b0 >= 0x70 && b0 <= 0x7f)) && p + 2 <= fsize) {
      target = at + 2 + static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(code[p + 1])));
      branch = true;
    } else if (b0 == 0xe9 && p + 5 <= fsize) {
      int32_t rel;
      std::memcpy(&rel, &code[p + 1], 4);
      target = at + 5 + static_cast<uint64_t>(static_cast<int64_t>(rel));
      branch = true;
    } else if (b0 == 0x0f && p + 6 <= fsize && code[p + 1] >= 0x80 && code[p + 1] <= 0x8f) {
      int32_t rel;
      std::memcpy(&rel, &code[p + 2], 4);
      target = at + 6 + static_cast<uint64_t>(static_cast<int64_t>(rel));
      branch = true;
    }
    if (branch && target == ret) return static_cast<int64_t>(p);
  }
  return -1;
}

}  // namespace

uint16_t classify_entry_probe(const uint8_t bytes[8], uint32_t delta) {
  // [R07 4.7], measured on this VM: a probe on the `push %rbp` after endbr64 is emulated by the
  // kernel (0.57 us per hit).  Nothing else is classified here: S1 measures the other classes.
  if (delta == 4 && std::memcmp(bytes, kEndbr64, 4) == 0 && bytes[4] == 0x55) return KZ_INSN_CLASS_EMULATED;
  return KZ_INSN_CLASS_UNKNOWN;
}

Status resolve_entry(const ElfImage& elf, const LineTable& lines, const std::string& func, const std::string& in_suffix, ResolvedTarget* out) {
  *out = ResolvedTarget();
  std::vector<const ElfFunction*> found = elf.find_functions(func);
  if (found.empty()) return Status::error(Err::Invalid, "no function symbol named " + func);
  if (!in_suffix.empty()) {
    const std::vector<uint32_t> ids = lines.files_matching(in_suffix);
    std::vector<const ElfFunction*> keep;
    for (const ElfFunction* f : found) {
      const LineRow* row = lines.row_at(f->vaddr);
      if (row != nullptr && std::find(ids.begin(), ids.end(), row->file) != ids.end()) keep.push_back(f);
    }
    if (keep.empty()) return Status::error(Err::Invalid, "no function named " + func + " is defined in a file ending in " + in_suffix);
    found = keep;
  }
  std::set<uint64_t> addrs;
  for (const ElfFunction* f : found) addrs.insert(f->vaddr);
  if (addrs.size() > 1) return Status::error(Err::Invalid, std::to_string(addrs.size()) + " different functions are named " + func + "; qualify the target with in=<file suffix>");
  const ElfFunction& f = *found.front();
  uint8_t head[8] = {0};
  uint64_t off0 = 0;
  if (!elf.vaddr_to_offset(f.vaddr, &off0)) return Status::error(Err::Invalid, "function " + func + " is not in file-backed code");
  ResolvedTarget t;
  fill_insn(elf, off0, &t);
  std::memcpy(head, t.insn, 8);
  const uint32_t delta = std::memcmp(head, kEndbr64, 4) == 0 ? 4u : 0u;
  uint64_t off = 0;
  if (!elf.vaddr_to_offset(f.vaddr + delta, &off)) return Status::error(Err::Invalid, "the probe address of " + func + " is not in file-backed code");
  t.kind = KZ_TARGET_KIND_FUNC_ENTRY;
  t.vaddr = f.vaddr + delta;
  t.file_offset = off;
  t.func_vaddr = f.vaddr;
  t.delta = delta;
  t.func = f.name;
  fill_insn(elf, off, &t);
  t.insn_class = classify_entry_probe(head, delta);
  const LineRow* row = lines.row_at(f.vaddr);
  if (row != nullptr) {
    t.line = row->line;
    t.col = row->col;
    t.file = lines.files()[row->file];
  }
  *out = t;
  return Status::success();
}

Status resolve_after_call(const ElfImage& elf, const LineTable& lines, const std::string& file_suffix, uint32_t line,
                          const std::string& callee, ResolvedTarget* out) {
  *out = ResolvedTarget();
  const std::string where = file_suffix + ":" + std::to_string(line);
  const std::vector<uint32_t> ids = lines.files_matching(file_suffix);
  if (ids.empty()) return Status::error(Err::Invalid, "no source file of the line table ends in " + file_suffix);
  {
    std::set<std::string> distinct;
    for (uint32_t id : ids) distinct.insert(lines.files()[id]);
    if (distinct.size() > 1) return Status::error(Err::Invalid, std::to_string(distinct.size()) + " source files end in " + file_suffix + "; give a longer suffix");
  }
  const std::vector<LineRange> ranges = lines.ranges_of(ids, line);
  if (ranges.empty()) return Status::error(Err::Invalid, "no machine code is attributed to " + where);
  // A function of a shared library (memset) is called through its PLT stub.
  const std::set<uint64_t> callee_addrs = callee_addresses(elf, callee);
  if (callee_addrs.empty()) return Status::error(Err::Invalid, "no function symbol or PLT stub named " + callee);

  struct Cand { uint64_t ret; uint32_t col; };
  std::vector<Cand> cands;
  // Instruction boundaries known from the line table: the end of each piece of the line's code, and
  // every row inside it (a row starts an instruction; e.g. gcc starts a row with a discriminator after
  // a call in a condition).  A call is taken only where it ends at such a boundary.
  for (const LineRange& r : ranges) {
    std::vector<uint64_t> ends;
    ends.push_back(r.end);
    for (const LineRow& row : lines.rows())
      if (!row.end_sequence && row.addr > r.begin && row.addr < r.end) ends.push_back(row.addr);
    for (uint64_t end : ends) {
      if (end - r.begin < 5) continue;
      uint8_t ins[5];
      if (!elf.read_vaddr(end - 5, 5, ins)) continue;
      if (ins[0] != 0xe8) continue;  // call rel32
      int32_t rel;
      std::memcpy(&rel, ins + 1, 4);
      const uint64_t target = end + static_cast<uint64_t>(static_cast<int64_t>(rel));
      if (callee_addrs.count(target) != 0) cands.push_back({end, r.col});
    }
  }
  std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.ret < b.ret; });
  cands.erase(std::unique(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.ret == b.ret; }), cands.end());
  if (cands.empty())
    return Status::error(Err::Invalid, "the code of " + where + " has no call to " + callee +
                                           " that ends at an instruction boundary of the line table (the end of the line's code or a row inside it)");
  if (cands.size() > 1) return Status::error(Err::Invalid, std::to_string(cands.size()) + " calls to " + callee + " end the code of " + where);
  const uint64_t ret = cands[0].ret;
  const ElfFunction* in = elf.function_at(ret - 1);
  if (in == nullptr) return Status::error(Err::Invalid, "the call at " + where + " is not inside a function symbol");
  if (elf.function_at(ret) != in) return Status::error(Err::Invalid, "the call at " + where + " is the last instruction of " + in->name + ": no instruction follows it");
  // A return site that a direct branch also reaches is a join of paths (branch_to above).
  {
    const int64_t p = branch_to(elf, *in, ret);
    if (p >= 0)
      return Status::error(Err::Invalid, "the return site of the call at " + where + " is also reached by a branch (at function offset " + std::to_string(p) +
                                             "): the point would fire on paths without the call; probe the call's line with line-start instead");
  }
  uint64_t off = 0;
  if (!elf.vaddr_to_offset(ret, &off)) return Status::error(Err::Invalid, "the return site of " + where + " is not in file-backed code");
  ResolvedTarget t;
  t.kind = KZ_TARGET_KIND_AFTER_CALL;
  t.insn_class = KZ_INSN_CLASS_UNKNOWN;
  t.vaddr = ret;
  t.file_offset = off;
  t.func_vaddr = in->vaddr;
  t.delta = static_cast<uint32_t>(ret - in->vaddr);
  t.line = line;
  t.col = cands[0].col;
  t.func = in->name;
  t.file = lines.files()[ranges.front().file];
  fill_insn(elf, off, &t);
  *out = t;
  return Status::success();
}

Status resolve_call_in_function(const ElfImage& elf, const std::string& func, const std::string& callee, ResolvedTarget* out) {
  *out = ResolvedTarget();
  const std::string where = "the function " + func;
  std::vector<const ElfFunction*> found = elf.find_functions(func);
  std::set<uint64_t> starts;
  for (const ElfFunction* f : found) starts.insert(f->vaddr);
  if (starts.empty()) return Status::error(Err::Invalid, "no function symbol named " + func);
  if (starts.size() > 1) return Status::error(Err::Invalid, std::to_string(starts.size()) + " different functions are named " + func);
  const ElfFunction& in = *found.front();
  const std::set<uint64_t> callee_addrs = callee_addresses(elf, callee);
  if (callee_addrs.empty()) return Status::error(Err::Invalid, "no function symbol or PLT stub named " + callee);
  // Every byte position of the function read as a `call rel32` to the callee.  Without a line table
  // the instruction boundaries are not known: a byte pattern inside another instruction could look
  // like such a call, so more than one candidate is refused (never a guess between them), and the one
  // candidate's return site must be inside the function and reached by no branch.
  std::vector<uint64_t> rets;
  std::vector<uint8_t> code(in.size);
  if (in.size == 0 || !elf.read_vaddr(in.vaddr, in.size, code.data())) return Status::error(Err::Invalid, where + " has no file-backed code");
  for (uint64_t p = 0; p + 5 <= in.size; ++p) {
    if (code[p] != 0xe8) continue;
    int32_t rel;
    std::memcpy(&rel, &code[p + 1], 4);
    const uint64_t end = in.vaddr + p + 5;
    if (callee_addrs.count(end + static_cast<uint64_t>(static_cast<int64_t>(rel))) != 0) rets.push_back(end);
  }
  if (rets.empty()) return Status::error(Err::Invalid, where + " makes no call to " + callee);
  if (rets.size() > 1) return Status::error(Err::Invalid, std::to_string(rets.size()) + " calls to " + callee + " are in " + where + ": a function-scoped point needs exactly one");
  const uint64_t ret = rets[0];
  if (elf.function_at(ret) != &in) return Status::error(Err::Invalid, "the call to " + callee + " is the last instruction of " + func + ": no instruction follows it");
  const int64_t p = branch_to(elf, in, ret);
  if (p >= 0)
    return Status::error(Err::Invalid, "the return site of the call to " + callee + " in " + func + " is also reached by a branch (at function offset " + std::to_string(p) +
                                           "): the point would fire on paths without the call");
  uint64_t off = 0;
  if (!elf.vaddr_to_offset(ret, &off)) return Status::error(Err::Invalid, "the return site of the call in " + func + " is not in file-backed code");
  ResolvedTarget t;
  t.kind = KZ_TARGET_KIND_AFTER_CALL;
  t.insn_class = KZ_INSN_CLASS_UNKNOWN;
  t.vaddr = ret;
  t.file_offset = off;
  t.func_vaddr = in.vaddr;
  t.delta = static_cast<uint32_t>(ret - in.vaddr);
  t.func = in.name;
  fill_insn(elf, off, &t);
  *out = t;
  return Status::success();
}

Status resolve_entry_reached(const ElfImage& elf, const LineTable& lines, const std::string& func, const std::vector<std::string>& chain, ResolvedTarget* out,
                             ReachedPath* path) {
  *out = ResolvedTarget();
  *path = ReachedPath();
  if (chain.empty()) return Status::error(Err::Invalid, "an entry-reached target needs at least one callee");
  auto unique_function = [&](const std::string& name, const ElfFunction** f) -> Status {
    std::vector<const ElfFunction*> found = elf.find_functions(name);
    std::set<uint64_t> starts;
    for (const ElfFunction* x : found) starts.insert(x->vaddr);
    if (starts.empty()) {
      if (elf.plt_stub(name) != 0) return Status::error(Err::Invalid, name + " is reached through its PLT stub: it is in another file, where a probe of this file cannot be");
      return Status::error(Err::Invalid, "no function symbol named " + name);
    }
    if (starts.size() > 1) return Status::error(Err::Invalid, std::to_string(starts.size()) + " different functions are named " + name);
    *f = found.front();
    return Status::success();
  };
  const ElfFunction* from = nullptr;
  Status s = unique_function(func, &from);
  if (!s.ok()) return s;
  path->via_func = from->name;
  path->chain = from->name;
  uint64_t last_call_end = 0;
  bool any_call = false;
  for (const std::string& callee : chain) {
    const ElfFunction* to = nullptr;
    s = unique_function(callee, &to);
    if (!s.ok()) return s;
    const uint64_t stub = elf.plt_stub(callee);
    std::vector<uint8_t> code(from->size);
    if (from->size == 0 || !elf.read_vaddr(from->vaddr, from->size, code.data())) return Status::error(Err::Invalid, "the function " + from->name + " has no file-backed code");
    struct Transfer {
      uint64_t end;
      bool call;
    };
    std::vector<Transfer> found;
    bool via_stub = false;
    for (uint64_t p = 0; p < from->size; ++p) {
      const uint64_t at = from->vaddr + p;
      const uint8_t b0 = code[p];
      uint64_t target = 0, end = 0;
      bool call = false, transfer = false;
      if ((b0 == 0xe8 || b0 == 0xe9) && p + 5 <= from->size) {
        int32_t rel;
        std::memcpy(&rel, &code[p + 1], 4);
        end = at + 5;
        target = end + static_cast<uint64_t>(static_cast<int64_t>(rel));
        call = b0 == 0xe8;
        transfer = true;
      } else if (b0 == 0x0f && p + 6 <= from->size && code[p + 1] >= 0x80 && code[p + 1] <= 0x8f) {
        int32_t rel;
        std::memcpy(&rel, &code[p + 2], 4);
        end = at + 6;
        target = end + static_cast<uint64_t>(static_cast<int64_t>(rel));
        transfer = true;
      } else if ((b0 == 0xeb || (b0 >= 0x70 && b0 <= 0x7f)) && p + 2 <= from->size) {
        end = at + 2;
        target = end + static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(code[p + 1])));
        transfer = true;
      }
      if (!transfer) continue;
      if (target == to->vaddr) found.push_back(Transfer{end, call});
      if (stub != 0 && target == stub) via_stub = true;
    }
    const std::string hop = from->name + " -> " + to->name;
    if (via_stub) return Status::error(Err::Invalid, "the hop " + hop + " goes through the PLT stub of " + callee + ": the callee is in another file");
    if (found.empty()) return Status::error(Err::Invalid, from->name + " makes no direct call or jump to " + callee);
    if (found.size() > 1)
      return Status::error(Err::Invalid, std::to_string(found.size()) + " calls or jumps to " + callee + " are in " + from->name + ": an entry-reached hop needs exactly one");
    path->hop_is_call.push_back(found[0].call);
    if (found[0].call) {
      any_call = true;
      last_call_end = found[0].end;
    }
    ++path->hops;
    path->chain += ">" + to->name;
    from = to;
  }
  // The return address at the top of the stack at the probe: a jump leaves the one of the frame it jumps
  // from, so it is the end of the last call hop, or F's own return address if no hop is a call.
  path->ret_dynamic = !any_call;
  path->expect_ret = any_call ? last_call_end : 0;
  ResolvedTarget t;
  s = resolve_entry(elf, lines, from->name, "", &t);
  if (!s.ok()) return s;
  t.kind = KZ_TARGET_KIND_ENTRY_REACHED;
  *out = t;
  return Status::success();
}

Status resolve_line_start(const ElfImage& elf, const LineTable& lines, const std::string& file_suffix, uint32_t line, ResolvedTarget* out) {
  *out = ResolvedTarget();
  const std::string where = file_suffix + ":" + std::to_string(line);
  const std::vector<uint32_t> ids = lines.files_matching(file_suffix);
  if (ids.empty()) return Status::error(Err::Invalid, "no source file of the line table ends in " + file_suffix);
  {
    std::set<std::string> distinct;
    for (uint32_t id : ids) distinct.insert(lines.files()[id]);
    if (distinct.size() > 1) return Status::error(Err::Invalid, std::to_string(distinct.size()) + " source files end in " + file_suffix + "; give a longer suffix");
  }
  std::vector<LineRange> ranges = lines.ranges_of(ids, line);
  if (ranges.empty()) return Status::error(Err::Invalid, "no machine code is attributed to " + where);
  std::sort(ranges.begin(), ranges.end(), [](const LineRange& a, const LineRange& b) { return a.begin < b.begin; });
  // One piece of code: a line whose code is split (a loop header, an inlined copy, a statement
  // interleaved with another line) has no single "before the statement" address.
  for (size_t i = 1; i < ranges.size(); ++i)
    if (ranges[i].begin != ranges[i - 1].end)
      return Status::error(Err::Invalid, "the code of " + where + " is in " + std::to_string(ranges.size()) + " separate pieces: a line-start probe needs one");
  const uint64_t at = ranges.front().begin;
  const ElfFunction* in = elf.function_at(at);
  if (in == nullptr) return Status::error(Err::Invalid, "the code of " + where + " is not inside a function symbol");
  if (elf.function_at(ranges.back().end - 1) != in) return Status::error(Err::Invalid, "the code of " + where + " spans two functions");
  uint64_t off = 0;
  if (!elf.vaddr_to_offset(at, &off)) return Status::error(Err::Invalid, "the code of " + where + " is not in file-backed code");
  ResolvedTarget t;
  t.kind = KZ_TARGET_KIND_LINE_START;
  t.insn_class = KZ_INSN_CLASS_UNKNOWN;
  t.vaddr = at;
  t.file_offset = off;
  t.func_vaddr = in->vaddr;
  t.delta = static_cast<uint32_t>(at - in->vaddr);
  t.line = line;
  t.col = ranges.front().col;
  t.func = in->name;
  t.file = lines.files()[ranges.front().file];
  fill_insn(elf, off, &t);
  *out = t;
  return Status::success();
}

}  // namespace kz::anchor

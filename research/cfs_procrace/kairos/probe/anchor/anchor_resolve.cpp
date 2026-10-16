// anchor_resolve.cpp -- see anchor_resolve.hpp.
#include "anchor_resolve.hpp"

#include <algorithm>
#include <cstring>
#include <set>

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
  const std::vector<const ElfFunction*> callees = elf.find_functions(callee);
  if (callees.empty()) return Status::error(Err::Invalid, "no function symbol named " + callee);
  std::set<uint64_t> callee_addrs;
  for (const ElfFunction* c : callees) callee_addrs.insert(c->vaddr);

  struct Cand { uint64_t ret; uint32_t col; };
  std::vector<Cand> cands;
  for (const LineRange& r : ranges) {
    if (r.end - r.begin < 5) continue;
    uint8_t ins[5];
    if (!elf.read_vaddr(r.end - 5, 5, ins)) continue;
    if (ins[0] != 0xe8) continue;  // call rel32
    int32_t rel;
    std::memcpy(&rel, ins + 1, 4);
    const uint64_t target = r.end + static_cast<uint64_t>(static_cast<int64_t>(rel));
    if (callee_addrs.count(target) != 0) cands.push_back({r.end, r.col});
  }
  std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.ret < b.ret; });
  cands.erase(std::unique(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.ret == b.ret; }), cands.end());
  if (cands.empty())
    return Status::error(Err::Invalid, "the code of " + where + " does not end with a call to " + callee +
                                           " (the return site is only taken where the call is the last instruction of the line)");
  if (cands.size() > 1) return Status::error(Err::Invalid, std::to_string(cands.size()) + " calls to " + callee + " end the code of " + where);
  const uint64_t ret = cands[0].ret;
  const ElfFunction* in = elf.function_at(ret - 1);
  if (in == nullptr) return Status::error(Err::Invalid, "the call at " + where + " is not inside a function symbol");
  if (elf.function_at(ret) != in) return Status::error(Err::Invalid, "the call at " + where + " is the last instruction of " + in->name + ": no instruction follows it");
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

}  // namespace kz::anchor

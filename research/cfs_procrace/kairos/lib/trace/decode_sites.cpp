// decode_sites.cpp -- see include/kz/trace/decode_sites.hpp.
#include "kz/trace/decode_sites.hpp"

#include <algorithm>
#include <cstring>

namespace kz::trace {

uint32_t perf_reg_of(uint32_t kz_reg) {
  // x86-64 perf register numbers: AX BX CX DX SI DI BP SP = 0..7, IP 8, FLAGS 9, segments 10..15,
  // R8..R15 = 16..23.  KZ_REG numbers the general registers in the same order, R8 onwards at 8.
  if (kz_reg <= KZ_REG_RSP) return kz_reg;
  if (kz_reg >= KZ_REG_R8 && kz_reg <= KZ_REG_R15) return 16 + (kz_reg - KZ_REG_R8);
  return KZ_NONE32;
}

uint16_t phase_of(const SiteSpec& s) {
  switch (s.kind) {
    case KZ_SITE_KIND_ENTRY_PROBE: return KZ_PHASE_ENTRY;
    case KZ_SITE_KIND_POINT_PROBE: return KZ_PHASE_POINT;
    case KZ_SITE_KIND_RET_PROBE: return KZ_PHASE_RET;
    default: return KZ_PHASE_NONE;
  }
}

Status check_site(const SiteSpec& s) {
  const std::string who = "site " + std::to_string(s.site) + " (" + s.label + ")";
  if (s.kind != KZ_SITE_KIND_ENTRY_PROBE && s.kind != KZ_SITE_KIND_POINT_PROBE && s.kind != KZ_SITE_KIND_RET_PROBE)
    return Status::error(Err::Invalid, who + ": the perf-sample backend probes entries, points and returns only");
  const uint16_t expect_phase = s.kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_SITE_PHASE_ENTRY
                                : s.kind == KZ_SITE_KIND_POINT_PROBE ? KZ_SITE_PHASE_POINT
                                                                     : KZ_SITE_PHASE_RET;
  if (s.phases != expect_phase) return Status::error(Err::Invalid, who + ": phases do not match the kind");
  if (s.decoder > KZ_DECODER_SPAWN_ID) return Status::error(Err::Invalid, who + ": unknown decoder");
  if (s.slots.size() > 4) return Status::error(Err::Invalid, who + ": more than four slots");
  bool used[4] = {false, false, false, false};
  for (const SlotSpec& sl : s.slots) {
    if (sl.slot >= 4) return Status::error(Err::Invalid, who + ": slot number beyond 3");
    if (used[sl.slot]) return Status::error(Err::Invalid, who + ": slot " + std::to_string(sl.slot) + " is declared twice");
    used[sl.slot] = true;
    if (sl.width != 1 && sl.width != 2 && sl.width != 4 && sl.width != 8) return Status::error(Err::Invalid, who + ": slot width must be 1, 2, 4 or 8 bytes");
    switch (sl.src) {
      case KZ_SLOT_SRC_REG:
        if (perf_reg_of(sl.index) == KZ_NONE32) return Status::error(Err::Invalid, who + ": slot " + std::to_string(sl.slot) + " names an unknown register");
        break;
      case KZ_SLOT_SRC_STACK:
        if (sl.index > 4096) return Status::error(Err::Invalid, who + ": stack offset beyond 4096 bytes");
        break;
      case KZ_SLOT_SRC_IP:
        break;
      case KZ_SLOT_SRC_MEM:
      case KZ_SLOT_SRC_STR:
        if (perf_reg_of(sl.index) == KZ_NONE32) return Status::error(Err::Invalid, who + ": slot " + std::to_string(sl.slot) + " reads memory at an unknown register");
        if (sl.src == KZ_SLOT_SRC_STR && sl.width != 4)
          return Status::error(Err::Invalid, who + ": slot " + std::to_string(sl.slot) + " is a string: it keeps the string's length in 4 bytes");
        break;
      default:
        return Status::error(Err::Invalid, who + ": slot " + std::to_string(sl.slot) + " has no source");
    }
    if (sl.role.empty()) return Status::error(Err::Invalid, who + ": slot " + std::to_string(sl.slot) + " has no role");
  }
  // The decoders of the identity join read fixed positions (kz_raw.h, KZ_DECODER).
  auto need = [&](uint8_t slot, const char* role, uint8_t max_width) -> Status {
    const SlotSpec* sl = slot_at(s, slot);
    if (sl == nullptr)
      return Status::error(Err::Invalid, who + ": the decoder reads slot " + std::to_string(slot) + (role != nullptr ? std::string(" (role ") + role + ")" : std::string()) + ", which the site does not declare");
    if (role != nullptr && sl->role != role)
      return Status::error(Err::Invalid, who + ": the decoder reads slot " + std::to_string(slot) + " as " + role + ", but the site declares it as " + sl->role);
    if (sl->width > max_width) return Status::error(Err::Invalid, who + ": slot " + std::to_string(slot) + " is wider than the decoder keeps (" + std::to_string(max_width) + " bytes)");
    if (sl->src == KZ_SLOT_SRC_STR) return Status::error(Err::Invalid, who + ": the decoder reads slot " + std::to_string(slot) + " as a number, but the site reads a string there");
    return Status::success();
  };
  Status st = Status::success();
  switch (s.decoder) {
    case KZ_DECODER_SPAWN_CREATE:
      if (s.kind != KZ_SITE_KIND_ENTRY_PROBE) return Status::error(Err::Invalid, who + ": a spawn-create site is an entry site");
      if (!(st = need(1, kRoleEntryFn, 8)).ok()) return st;
      if (!(st = need(2, kRolePriority, 4)).ok()) return st;
      if (!(st = need(kSlotReturnAddress, kRoleReturnAddress, 8)).ok()) return st;
      break;
    case KZ_DECODER_SPAWN_RETURN:
      if (s.kind != KZ_SITE_KIND_RET_PROBE) return Status::error(Err::Invalid, who + ": a spawn-return site is a return site");
      if (!(st = need(kSlotReturnAddress, kRoleReturnAddress, 8)).ok()) return st;
      break;
    case KZ_DECODER_THREAD_ENTRY:
      if (s.kind != KZ_SITE_KIND_ENTRY_PROBE) return Status::error(Err::Invalid, who + ": a thread-entry site is an entry site");
      if (!(st = need(0, nullptr, 4)).ok()) return st;
      break;
    case KZ_DECODER_OWNER_REQUEST:
      if (s.kind != KZ_SITE_KIND_ENTRY_PROBE) return Status::error(Err::Invalid, who + ": an owner-request site is an entry site");
      if (!(st = need(2, kRoleOwnerId, 4)).ok()) return st;
      if (!(st = need(kSlotReturnAddress, kRoleReturnAddress, 8)).ok()) return st;
      break;
    case KZ_DECODER_CHILD_REQUEST:
      if (s.kind != KZ_SITE_KIND_ENTRY_PROBE) return Status::error(Err::Invalid, who + ": a child-request site is an entry site");
      if (!(st = need(kSlotReturnAddress, kRoleReturnAddress, 8)).ok()) return st;
      break;
    case KZ_DECODER_REQUEST_RETURN:
      if (s.kind != KZ_SITE_KIND_RET_PROBE) return Status::error(Err::Invalid, who + ": a request-return site is a return site");
      if (!(st = need(kSlotReturnAddress, kRoleReturnAddress, 8)).ok()) return st;
      break;
    case KZ_DECODER_SPAWN_ID:
      if (s.kind != KZ_SITE_KIND_ENTRY_PROBE) return Status::error(Err::Invalid, who + ": a spawn-id site is an entry site");
      if (!(st = need(0, nullptr, 4)).ok()) return st;
      break;
    default:
      break;
  }
  return Status::success();
}

const SlotSpec* slot_at(const SiteSpec& s, uint8_t slot) {
  for (const SlotSpec& sl : s.slots)
    if (sl.slot == slot) return &sl;
  return nullptr;
}

bool has_return_address_key(const SiteSpec& s) {
  const SlotSpec* sl = slot_at(s, kSlotReturnAddress);
  return sl != nullptr && sl->role == kRoleReturnAddress && sl->width == 8;
}

bool has_fetch_slots(const SiteSpec& s) {
  for (const SlotSpec& sl : s.slots)
    if (is_fetch_slot(sl)) return true;
  return false;
}

Status check_plan(const std::vector<SiteSpec>& sites) {
  for (const SiteSpec& s : sites) {
    Status st = check_site(s);
    if (!st.ok()) return st;
  }
  // A point reached from a named function: its events are attributed by the return address (slot 3) to an
  // open call of that function, so the plan must probe that function's entry and return with their keys.
  for (const SiteSpec& v : sites) {
    if (v.via_symbol.empty()) continue;
    const std::string who = "site " + std::to_string(v.site) + " (" + v.label + ")";
    if (v.kind != KZ_SITE_KIND_POINT_PROBE) return Status::error(Err::Invalid, who + ": a site reached from a function is a point site");
    if (!has_return_address_key(v)) return Status::error(Err::Invalid, who + ": a site reached from " + v.via_symbol + " is attributed by slot 3, which must be an 8-byte return_address");
    bool entry = false, ret = false;
    for (const SiteSpec& f : sites) {
      if (f.symbol != v.via_symbol || f.binary != v.binary || !has_return_address_key(f)) continue;
      entry = entry || f.kind == KZ_SITE_KIND_ENTRY_PROBE;
      ret = ret || f.kind == KZ_SITE_KIND_RET_PROBE;
    }
    if (!entry || !ret)
      return Status::error(Err::Invalid, who + ": it is reached from " + v.via_symbol + ", whose entry and return sites (with return_address at slot 3, in the same file) the plan must have");
  }
  for (const SiteSpec& e : sites) {
    if (e.kind != KZ_SITE_KIND_ENTRY_PROBE || e.symbol.empty()) continue;
    for (const SiteSpec& r : sites) {
      if (r.kind != KZ_SITE_KIND_RET_PROBE || r.symbol != e.symbol || r.binary != e.binary) continue;  // a pair is one function of one file
      for (const SiteSpec* x : {&e, &r})
        if (!has_return_address_key(*x))
          return Status::error(Err::Invalid, "site " + std::to_string(x->site) + " (" + x->label + "): the entry and return sites of " + e.symbol +
                                                 " are matched on slot 3, which must be an 8-byte return_address");
    }
  }
  return Status::success();
}

PerfNeeds needs_of(const SiteSpec& s) {
  PerfNeeds n;
  for (const SlotSpec& sl : s.slots) {
    if (is_fetch_slot(sl)) continue;  // the probe event's fetch arguments read it, not the sample
    if (sl.src == KZ_SLOT_SRC_REG) {
      const uint32_t r = perf_reg_of(sl.index);
      if (r != KZ_NONE32) n.regs_user |= 1ull << r;
    } else if (sl.src == KZ_SLOT_SRC_STACK) {
      const uint32_t end = sl.index + sl.width;
      const uint32_t rounded = (end + 7u) & ~7u;
      n.stack_bytes = std::max(n.stack_bytes, rounded);
    }
  }
  // The kernel starts the stack dump at the user sp and refuses a dump without a register set;
  // asking for sp as well keeps the record self-describing.
  if (n.stack_bytes != 0) n.regs_user |= 1ull << perf_reg_of(KZ_REG_RSP);
  return n;
}

Status apply_slots(const SiteSpec& s, const PerfRecord& sample, uint64_t out[4], uint16_t* raw_flags) {
  static const std::vector<RawField> none;
  std::string strings[4];
  bool read[4];
  return apply_slots(s, sample, none, out, raw_flags, strings, read);
}

Status apply_slots(const SiteSpec& s, const PerfRecord& sample, const std::vector<RawField>& fields, uint64_t out[4], uint16_t* raw_flags,
                   std::string strings[4], bool string_read[4]) {
  for (int i = 0; i < 4; ++i) {
    strings[i].clear();
    string_read[i] = false;
  }
  out[0] = out[1] = out[2] = out[3] = 0;
  *raw_flags = 0;
  if (sample.kind != RecKind::Sample) return Status::error(Err::Invalid, "apply_slots: not a sample");
  for (const SlotSpec& sl : s.slots) {
    uint64_t v = 0;
    bool have = false;
    switch (sl.src) {
      case KZ_SLOT_SRC_REG: {
        const uint32_t r = perf_reg_of(sl.index);
        if (r != KZ_NONE32 && (sample.regs_valid & (1ull << r)) != 0) {
          v = sample.regs[r];
          have = true;
        }
        break;
      }
      case KZ_SLOT_SRC_STACK: {
        // A word beyond the bytes the kernel copied is not a value.
        const uint64_t end = static_cast<uint64_t>(sl.index) + sl.width;
        if (end <= sample.stack.size()) {
          for (uint32_t b = 0; b < sl.width; ++b) v |= static_cast<uint64_t>(sample.stack[sl.index + b]) << (8 * b);
          have = true;
        }
        break;
      }
      case KZ_SLOT_SRC_IP:
        v = sample.ip;
        have = true;
        break;
      case KZ_SLOT_SRC_MEM:
      case KZ_SLOT_SRC_STR: {
        const RawField* f = nullptr;
        for (const RawField& rf : fields)
          if (rf.slot == sl.slot) f = &rf;
        if (f == nullptr || f->is_string != (sl.src == KZ_SLOT_SRC_STR)) break;
        const uint64_t end = static_cast<uint64_t>(f->offset) + f->size;
        if (end > sample.raw.size() || f->size == 0 || f->size > 8) break;
        uint64_t w = 0;
        for (uint32_t b = 0; b < f->size; ++b) w |= static_cast<uint64_t>(sample.raw[f->offset + b]) << (8 * b);
        if (!f->is_string) {
          v = w;
          have = true;
          break;
        }
        // __data_loc: offset of the text in the record (low 16 bits), its length with the NUL (high 16 bits)
        const uint32_t loc = static_cast<uint32_t>(w);
        const uint32_t at = loc & 0xffffu, len = loc >> 16;
        if (len == 0 || static_cast<uint64_t>(at) + len > sample.raw.size()) break;
        size_t n = 0;
        while (n < len && sample.raw[at + n] != 0) ++n;
        if (n == len) break;  // no NUL inside the field: not a string the kernel completed
        strings[sl.slot].assign(reinterpret_cast<const char*>(&sample.raw[at]), n);
        string_read[sl.slot] = true;
        v = n;
        have = true;
        break;
      }
      default:
        break;
    }
    if (!have) {
      *raw_flags |= KZ_RAW_FLAG_TRUNC;
      continue;
    }
    if (sl.width < 8) v &= (1ull << (8 * sl.width)) - 1;
    out[sl.slot] = v;
  }
  return Status::success();
}

}  // namespace kz::trace

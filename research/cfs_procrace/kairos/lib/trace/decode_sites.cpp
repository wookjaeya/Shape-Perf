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

Status check_plan(const std::vector<SiteSpec>& sites) {
  for (const SiteSpec& s : sites) {
    Status st = check_site(s);
    if (!st.ok()) return st;
  }
  for (const SiteSpec& e : sites) {
    if (e.kind != KZ_SITE_KIND_ENTRY_PROBE || e.symbol.empty()) continue;
    for (const SiteSpec& r : sites) {
      if (r.kind != KZ_SITE_KIND_RET_PROBE || r.symbol != e.symbol) continue;
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

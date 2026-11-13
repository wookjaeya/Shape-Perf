// decode_sites.hpp -- a probe site as the trace layer sees it, and how a perf sample becomes
// a raw event (SPEC 5.3: "descriptors are data; there is no per-site code generation").
//
// A site names where the observer probes (a file offset in the traced binary and a phase) and
// up to four slots.  A slot says where a raw value comes from -- a register, a word of the
// captured stack, or the instruction pointer of the sample -- how many bytes of it to keep, and
// what it means (its role).  The loader derives from the slots what it asks the kernel to copy
// (needs_of); the decoder applies the same slots to what the kernel copied (apply_slots).  One
// description serves both, so a slot can never be requested in one way and read in another.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"
#include "kz/trace/decode_perf.hpp"

namespace kz::trace {

struct SlotSpec {
  uint8_t slot = 0;   // 0..3: the index into kz_raw_ev.a
  uint8_t src = 0;    // KZ_SLOT_SRC_*
  uint8_t width = 8;  // bytes kept: 1, 2, 4 or 8 (a STR slot keeps the string's byte length in 4 bytes)
  uint32_t index = 0; // KZ_REG_* for a register (also the base register of MEM and STR), the byte offset in the stack dump for a stack word
  std::string role;
  int32_t offset = 0; // MEM, STR: the byte offset added to the base register (the address read)
};

// A slot the kernel fills by a fetch argument of a probe event (memory at register + offset), not from
// the sampled registers or stack.
inline bool is_fetch_slot(const SlotSpec& s) { return s.src == 4 /* KZ_SLOT_SRC_MEM */ || s.src == 5 /* KZ_SLOT_SRC_STR */; }

struct SiteSpec {
  uint32_t site = 0;
  uint16_t kind = 0;      // KZ_SITE_KIND_*
  uint16_t phases = 0;    // KZ_SITE_PHASE_* (one bit for the plans of this implementation)
  uint16_t decoder = 0;   // KZ_DECODER_*
  uint16_t insn_class = 0;
  uint64_t file_offset = 0;
  uint64_t vaddr = 0;     // link-time address of the probed instruction (the basis target)
  uint32_t line = 0;
  std::string label, symbol, source;
  std::vector<SlotSpec> slots;
  // Which traced file the site is in: 0 = the primary binary (the executable), n = the n-th further
  // file of the plan (a shared object the process loads).  The load bias and the probed inode are
  // per file.
  uint16_t binary = 0;
  // A point at the entry of a function reached from `via_symbol` (KZ_TARGET_KIND_ENTRY_REACHED; kz_raw.h,
  // VIA): its events are kept only where attributed to an open call of `via_symbol` on the same thread
  // (decode_merge.hpp): the return address at the top of the stack (slot 3, role return_address) equals
  // `via_expect_ret` (a link-time address of this file) or, with `via_ret_dynamic` (every hop a jump), the
  // stack pointer equals the one at the entry of that open call.  Empty `via_symbol`: an ordinary site.
  std::string via_symbol;
  std::string via_chain;      // "F>G[>H]" as the plan wrote it (report only)
  uint64_t via_expect_ret = 0;
  bool via_ret_dynamic = false;
  uint16_t via_hops = 0;
};

// True if any slot of the site is a fetch slot (MEM or STR).
bool has_fetch_slots(const SiteSpec& s);


// What a site's slots require from the kernel: a mask of perf register numbers (bit n =
// PERF_REG_X86 register n) and the number of stack bytes to dump.
struct PerfNeeds {
  uint64_t regs_user = 0;
  uint32_t stack_bytes = 0;
};

// The roles that the engine reads at fixed positions (kz_raw.h, KZ_DECODER and the entry/return
// matching).  They name what a value is, not where it comes from in any framework.
inline constexpr const char* kRoleReturnAddress = "return_address";
inline constexpr const char* kRoleEntryFn = "entry_fn";
inline constexpr const char* kRolePriority = "priority";
inline constexpr const char* kRoleOwnerId = "owner_id";  // OWNER_REQUEST a[2]: the application the started task belongs to
inline constexpr uint8_t kSlotReturnAddress = 3;  // a[3] at a paired ENTRY and RET site, and at the spawn sites

// The slot declared at position `slot`, or nullptr.
const SlotSpec* slot_at(const SiteSpec& s, uint8_t slot);
// True if the site declares the role "return_address" at slot 3 (the key of entry/return matching).
bool has_return_address_key(const SiteSpec& s);

// One site: kind, phases, slot sources and widths, and, for a site with a KZ_DECODER other than
// GENERIC, the slots the decoder reads at fixed positions (kz_raw.h).  A site that would make the
// identity join read a value from a slot the plan does not fill is refused here, not decoded as 0.
Status check_site(const SiteSpec& s);
// The whole plan: every site (check_site), and for every ENTRY site that has a RET site of the same
// symbol (the pair that entry/return matching uses), both declare "return_address" at slot 3.
Status check_plan(const std::vector<SiteSpec>& sites);
PerfNeeds needs_of(const SiteSpec& s);
uint32_t perf_reg_of(uint32_t kz_reg);  // KZ_REG_* -> perf register number; KZ_NONE32 if invalid

// The phase of the raw events a site produces (KZ_PHASE_*).
uint16_t phase_of(const SiteSpec& s);

// Fills out[0..3] and *raw_flags from a sample of the site.  A slot that cannot be read (a
// register the kernel did not copy, a stack word beyond the bytes it did copy) is zero and sets
// KZ_RAW_FLAG_TRUNC, so a value is never taken from a record that did not carry it.
Status apply_slots(const SiteSpec& s, const PerfRecord& sample, uint64_t out[4], uint16_t* raw_flags);
// The same for a site with fetch slots: `fields` say where each fetched value is in sample.raw; the text of
// a STR slot goes to strings[slot] (its byte length to out[slot]) and string_read[slot] is set.  A fetch slot
// without a field, a field beyond the raw record, or a string the kernel could not read (length 0) is
// unreadable (zero, TRUNC, string_read false).
Status apply_slots(const SiteSpec& s, const PerfRecord& sample, const std::vector<RawField>& fields, uint64_t out[4], uint16_t* raw_flags,
                   std::string strings[4], bool string_read[4]);

}  // namespace kz::trace

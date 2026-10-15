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
  uint8_t width = 8;  // bytes kept: 1, 2, 4 or 8
  uint32_t index = 0; // KZ_REG_* for a register, the byte offset in the stack dump for a stack word
  std::string role;
};

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
};

// What a site's slots require from the kernel: a mask of perf register numbers (bit n =
// PERF_REG_X86 register n) and the number of stack bytes to dump.
struct PerfNeeds {
  uint64_t regs_user = 0;
  uint32_t stack_bytes = 0;
};

Status check_site(const SiteSpec& s);
PerfNeeds needs_of(const SiteSpec& s);
uint32_t perf_reg_of(uint32_t kz_reg);  // KZ_REG_* -> perf register number; KZ_NONE32 if invalid

// The phase of the raw events a site produces (KZ_PHASE_*).
uint16_t phase_of(const SiteSpec& s);

// Fills out[0..3] and *raw_flags from a sample of the site.  A slot that cannot be read (a
// register the kernel did not copy, a stack word beyond the bytes it did copy) is zero and sets
// KZ_RAW_FLAG_TRUNC, so a value is never taken from a record that did not carry it.
Status apply_slots(const SiteSpec& s, const PerfRecord& sample, uint64_t out[4], uint16_t* raw_flags);

}  // namespace kz::trace

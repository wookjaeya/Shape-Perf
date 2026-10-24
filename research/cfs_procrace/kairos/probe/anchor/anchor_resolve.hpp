// anchor_resolve.hpp -- from a symbolic probe target to a file offset in the traced binary.
//
// Three kinds of target exist (KZ_TARGET_KIND):
//   func-entry   the first instruction after the function's entry marker.  A gcc -O0 function
//                starts `endbr64; push %rbp; mov %rsp,%rbp`; the probe goes on the push (entry
//                + 4), where the argument registers still hold the arguments and the top of the
//                stack still holds the return address.  A function that does not start with
//                endbr64 is probed at its first byte.  The return probe of the same function
//                uses the same address.
//   after-call   the instruction that follows the call of a named function at a source line.
//                The line's machine code is taken from the DWARF line table; the call must end
//                at an instruction boundary the table states (the end of the line's code, or a
//                row inside it, e.g. the discriminator row gcc starts after a call in a
//                condition): a `call rel32` whose target is the callee's address, so that the
//                return site is the first byte after it and not a guess.  If no call ends at a
//                boundary, or more than one call matches, the resolver refuses.  It also refuses
//                a return site that a direct branch of the function reaches (a join of paths:
//                the probe would fire where the call was not made); line-start fits there.  A call through the PLT stub of a shared-library function
//                (memset) counts as a call of that function.
//                Without a line table, `resolve_call_in_function` finds the one call of the callee
//                inside a named function instead (same refusals; line 0).
//   line-start   the first instruction of a source line's code: the probe fires before the
//                statement runs (a store written as an assignment, which has no call to follow).
//                The line's code must be one contiguous piece inside one function.
//   entry-reached the entry (as func-entry) of the last function of a chain F>G[>H...], reached from F
//                through one direct transfer per hop: a `call rel32`, or a tail jump (`jmp rel32/rel8`,
//                a conditional `jcc rel32/rel8`) to the callee's function symbol, the only transfer to
//                that callee in its caller (read at every byte position, as without a line table; more
//                than one candidate is refused, never chosen between).  A transfer to the callee's PLT
//                stub is refused: the callee is in another file, where this file's probe cannot be.  This
//                is the target for a callee that is reached by a tail jump, after which an after-call
//                point would never fire.  The resolver also states the return address the reaching call
//                leaves at the top of the stack at the probe: the end of the last `call` hop, or, if
//                every hop is a jump, F's own return address (dynamic).  The observer keeps an event of
//                such a site only where that return address matches an open call of F on the same
//                thread (decode_merge.hpp), which is the W2 requirement that a point event mean the
//                call was made, carried to a probe that other callers of the callee also reach.
//
// The binary is only read.  Nothing here executes it or depends on its being loaded.
#pragma once

#include <cstdint>
#include <string>

#include "dwarf_line.hpp"
#include "elf_image.hpp"
#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"

namespace kz::anchor {

struct ResolvedTarget {
  uint16_t kind = KZ_TARGET_KIND_NONE;
  uint16_t insn_class = KZ_INSN_CLASS_UNKNOWN;
  uint64_t vaddr = 0;        // link-time address of the probed instruction
  uint64_t file_offset = 0;
  uint64_t func_vaddr = 0;
  uint32_t delta = 0;        // vaddr - func_vaddr
  uint32_t line = 0, col = 0;
  uint8_t insn[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // the bytes at the probed offset (as many as the file holds)
  std::string func;          // the function that contains the probed address
  std::string file;          // source file of the line (after-call) or of the function's first line (entry; may be empty)
};

// `in_suffix` (optional) selects among several functions of the same name by the source file
// that the line table assigns to the function's first instruction.
Status resolve_entry(const ElfImage& elf, const LineTable& lines, const std::string& func, const std::string& in_suffix, ResolvedTarget* out);
Status resolve_after_call(const ElfImage& elf, const LineTable& lines, const std::string& file_suffix, uint32_t line,
                          const std::string& callee, ResolvedTarget* out);
Status resolve_line_start(const ElfImage& elf, const LineTable& lines, const std::string& file_suffix, uint32_t line, ResolvedTarget* out);
// after-call without a line table (a binary built without -g): the instruction after the one call of
// `callee` (its symbol or PLT stub) inside the function `func`.  Kind after-call, line 0.  Refused if
// the function has no such call or more than one (instruction boundaries are not known, so two
// candidates are never decided between), or if a branch of the function reaches the return site.
Status resolve_call_in_function(const ElfImage& elf, const std::string& func, const std::string& callee, ResolvedTarget* out);

// The call path of an entry-reached target.
struct ReachedPath {
  std::string via_func;         // F
  std::string chain;            // "F>G[>H...]"
  uint16_t hops = 0;
  std::vector<bool> hop_is_call;  // per hop: a call (true) or a jump (false)
  bool ret_dynamic = false;     // every hop is a jump: the return address is F's own
  uint64_t expect_ret = 0;      // otherwise the link-time address after the last call hop
};
// entry-reached: the entry of chain.back() reached from `func` (see above).  Kind entry-reached.
Status resolve_entry_reached(const ElfImage& elf, const LineTable& lines, const std::string& func, const std::vector<std::string>& chain, ResolvedTarget* out,
                             ReachedPath* path);

// The cost class of a probe on `bytes` at `delta` bytes after the entry of a function, from the
// instruction patterns that were measured (see KZ_ENUM_INSN_CLASS); everything else is unknown.
uint16_t classify_entry_probe(const uint8_t bytes[8], uint32_t delta);

}  // namespace kz::anchor

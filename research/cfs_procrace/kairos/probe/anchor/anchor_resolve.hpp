// anchor_resolve.hpp -- from a symbolic probe target to a file offset in the traced binary.
//
// Two kinds of target exist (KZ_TARGET_KIND):
//   func-entry   the first instruction after the function's entry marker.  A gcc -O0 function
//                starts `endbr64; push %rbp; mov %rsp,%rbp`; the probe goes on the push (entry
//                + 4), where the argument registers still hold the arguments and the top of the
//                stack still holds the return address.  A function that does not start with
//                endbr64 is probed at its first byte.  The return probe of the same function
//                uses the same address.
//   after-call   the instruction that follows the call of a named function at a source line.
//                The line's machine code is taken from the DWARF line table; the call must be
//                the last instruction of that code (a `call rel32` whose target is the callee's
//                address), so that the return site is the first byte after it and not a guess.
//                If the call is not the last instruction, or more than one call matches, the
//                resolver refuses.
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

// The cost class of a probe on `bytes` at `delta` bytes after the entry of a function, from the
// instruction patterns that were measured (see KZ_ENUM_INSN_CLASS); everything else is unknown.
uint16_t classify_entry_probe(const uint8_t bytes[8], uint32_t delta);

}  // namespace kz::anchor

// elf_info.hpp -- the little of an ELF64 file the analysis basis needs, read with <elf.h>.
//
//   * the global symbols of a relocatable object (defined and undefined), to emulate the
//     archive-member selection of the link command (SPEC 4.1 step 1 / R07: "archive semantics
//     come from the link command");
//   * the compiler identification strings (.comment) and the build-id of a linked binary.
//
// Every read is bounds checked: a damaged file gives a Status, not a crash.
#pragma once

#include <set>
#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kzs {

struct ObjectSymbols {
  std::set<std::string> defined;    // global or weak symbols defined here (common symbols included)
  std::set<std::string> undefined;  // global symbols referenced and not defined (weak references excluded)
};

kz::Status read_object_symbols(const std::string& path, ObjectSymbols* out);

struct ElfNotes {
  std::string build_id_hex;           // empty if the file has no GNU build-id note
  std::vector<std::string> comments;  // the strings of .comment, in order, without duplicates
  std::string type;                   // "relocatable", "executable", "shared", "other"
};

kz::Status read_elf_notes(const std::string& path, ElfNotes* out);

}  // namespace kzs

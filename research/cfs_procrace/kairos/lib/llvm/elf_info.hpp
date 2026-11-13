// elf_info.hpp -- the little of an ELF file the analysis basis needs, read with <elf.h>.
//
//   * the global symbols of a relocatable object (defined and undefined), to emulate the
//     archive-member selection of the link command (SPEC 4.1 step 1 / R07: "archive semantics
//     come from the link command");
//   * the dynamic symbols of a linked executable or shared object (.dynsym): what it exports to
//     the dynamic loader and what it asks the loader for (SPEC 4.1 step 6, the app-unit probe);
//   * the compiler identification strings (.comment) and the build-id of a linked binary.
//
// Classes: ELF64 and (2026-10-29, a 32-bit target: E-ST of the instrumented-family plan) ELF32, both
// little-endian; the same three reads with the class's own headers, the same bounds checks.  Every read
// also gives the file's identity (class, byte order, machine); the files of one basis must all have the same one
// (ElfTarget), a mix is an error.
//
// Every read is bounds checked: a damaged file gives a Status, not a crash.
#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kzs {

// What an ELF header says the file is built for (e_ident class and data, e_machine).
struct ElfId {
  uint8_t elf_class = 0;  // ELFCLASS32 | ELFCLASS64 (0: not read)
  uint8_t data = 0;       // ELFDATA2LSB (the only byte order read)
  uint16_t machine = 0;   // EM_*
  bool operator==(const ElfId& o) const { return elf_class == o.elf_class && data == o.data && machine == o.machine; }
  bool operator!=(const ElfId& o) const { return !(*this == o); }
  uint32_t pointer_bytes() const;  // 4 for ELF32, 8 for ELF64, 0 if not read
  std::string text() const;        // e.g. "ELF32 little-endian EM_386 (3)"
};

struct ObjectSymbols {
  std::set<std::string> defined;    // global or weak symbols defined here (common symbols included)
  std::set<std::string> undefined;  // global symbols referenced and not defined (weak references excluded)
  std::set<std::string> hidden;     // the defined ones with hidden or internal visibility (not exported by a link)
  ElfId elf;                        // the file's ELF identity
};

kz::Status read_object_symbols(const std::string& path, ObjectSymbols* out);

// The dynamic symbol table of an executable or a shared object: `defined` = the symbols it exports
// (global or weak, default or protected visibility), `undefined` = the non-weak symbols it needs.
kz::Status read_dynamic_symbols(const std::string& path, ObjectSymbols* out);

struct ElfNotes {
  std::string build_id_hex;           // empty if the file has no GNU build-id note
  std::vector<std::string> comments;  // the strings of .comment, in order, without duplicates
  std::string type;                   // "relocatable", "executable", "shared", "other"
  ElfId elf;                          // the file's ELF identity
};

kz::Status read_elf_notes(const std::string& path, ElfNotes* out);

// The ELF identity shared by every file of one basis (objects, archive members, the executable, the loaded modules): the
// first file added sets it; a file with another class, byte order or machine is an error naming both files.
class ElfTarget {
 public:
  kz::Status add(const std::string& path, const ElfId& id);
  bool known() const { return id_.elf_class != 0; }
  const ElfId& id() const { return id_; }
  const std::string& first_path() const { return first_; }

 private:
  ElfId id_;
  std::string first_;
};

}  // namespace kzs

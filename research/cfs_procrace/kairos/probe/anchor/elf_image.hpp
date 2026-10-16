// elf_image.hpp -- the parts of an ELF64 x86-64 file that the observer needs, read with <elf.h>.
//
// The observer never modifies the traced binary and never needs more than: its identity (the
// build-id note), where its code and data are (program headers), its function symbols, and
// its DWARF line table.  Every access is bounds checked against the file image: a damaged or
// hostile file produces a Status, not a crash.
//
// Why not LLVM's object and DWARF readers (the choice SPEC 9.3 leaves open): the observer must
// run where the analysis tools are not built, LLVM must not reach tools/kz (lint no-llvm), and
// what is needed is small and fixed -- symbols, program headers and one line table.  The reader
// is cross-checked against the system's addr2line in the tests, and the LLVM-based `basis`
// tool of the static analyzer (a separate stage) can audit the anchors independently
// (SPEC 3.17).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/status.hpp"

namespace kz::anchor {

struct ElfSegment {
  uint64_t vaddr = 0, offset = 0, filesz = 0, memsz = 0;
  uint32_t flags = 0;
};

struct ElfFunction {
  std::string name;
  uint64_t vaddr = 0;
  uint64_t size = 0;
  uint8_t bind = 0;     // KZ_SYMBOL_BIND_*
  uint16_t shndx = 0;
};

struct Slice {
  const uint8_t* data = nullptr;
  size_t size = 0;
  bool empty() const { return size == 0; }
};

class ElfImage {
 public:
  static Status parse(Bytes bytes, ElfImage* out);

  const Bytes& bytes() const { return bytes_; }
  uint16_t type() const { return type_; }        // ET_EXEC or ET_DYN
  bool position_independent() const { return type_ == 3; }
  const std::string& build_id_hex() const { return build_id_; }  // empty if the file has none
  const std::vector<ElfSegment>& segments() const { return segments_; }
  // Function symbols defined in the file (STT_FUNC, section index != 0), sorted by (vaddr, name).
  const std::vector<ElfFunction>& functions() const { return functions_; }

  // Link-time address -> file offset, through the PT_LOAD segments.
  bool vaddr_to_offset(uint64_t vaddr, uint64_t* offset) const;
  // Copies n file bytes mapped at vaddr (all of them must lie in file-backed segment data).
  bool read_vaddr(uint64_t vaddr, size_t n, uint8_t* out) const;
  bool read_offset(uint64_t offset, size_t n, uint8_t* out) const;
  // A section by name (empty slice if absent).
  Slice section(std::string_view name) const;
  // Every function symbol with this exact name.
  std::vector<const ElfFunction*> find_functions(std::string_view name) const;
  // The function symbol containing vaddr, or null.
  const ElfFunction* function_at(uint64_t vaddr) const;

 private:
  Bytes bytes_;
  uint16_t type_ = 0;
  std::string build_id_;
  std::vector<ElfSegment> segments_;
  std::vector<ElfFunction> functions_;
  struct Sec {
    std::string name;
    uint64_t offset = 0, size = 0;
    uint32_t type = 0;
  };
  std::vector<Sec> sections_;
};

std::string to_hex_string(const uint8_t* p, size_t n);

}  // namespace kz::anchor

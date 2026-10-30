// dwarf_line.hpp -- a DWARF line-number program reader (versions 2 to 5).
//
// Reads .debug_line (with .debug_line_str and .debug_str for the DWARF 5 string forms) into
// rows (address, file, line, column, is_stmt).  That is all the anchor resolver needs: from a
// source position to the machine-code range that implements it.  It is small by design -- no
// .debug_info, no types, no inlining -- and it refuses what it does not understand (a VLIW
// target, an unknown standard-opcode length, an unsupported entry form) instead of guessing.
//
// Cross-checked against the system's addr2line on binaries built by gcc and clang in the tests.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "elf_image.hpp"
#include "kz/core/status.hpp"

namespace kz::anchor {

struct LineRow {
  uint64_t addr = 0;
  uint32_t file = 0;  // index into LineTable::files
  uint32_t line = 0;
  uint32_t col = 0;
  bool is_stmt = false;
  bool end_sequence = false;
};

// A maximal run of consecutive rows of one (file, line), as the address range [begin, end).
struct LineRange {
  uint64_t begin = 0, end = 0;
  uint32_t file = 0, line = 0, col = 0;  // col of the first row
};

class LineTable {
 public:
  // Reads every line program of the image.  An image without .debug_line gives an empty table.
  static Status parse(const ElfImage& elf, LineTable* out);

  const std::vector<std::string>& files() const { return files_; }
  // All rows: ordered by sequence start address; within a sequence in program order.
  const std::vector<LineRow>& rows() const { return rows_; }

  // The distinct file ids whose path ends with `suffix` at a path-component boundary.
  std::vector<uint32_t> files_matching(const std::string& suffix) const;
  // The ranges of (file in `file_ids`, line) that contain at least one is_stmt row, in address order.
  std::vector<LineRange> ranges_of(const std::vector<uint32_t>& file_ids, uint32_t line) const;
  // The row that covers `addr` (the last row at or before it in its sequence, not an end row), or null.
  const LineRow* row_at(uint64_t addr) const;

 private:
  std::vector<std::string> files_;
  std::vector<LineRow> rows_;
};

}  // namespace kz::anchor

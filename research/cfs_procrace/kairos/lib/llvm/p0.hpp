// p0.hpp -- the built-in table of thread-library and C-library primitives (P0).
//
// The profile-free mode of kz-static (`extract --no-profile`, wrappers.hpp) starts from these entries
// alone: every delay, terminal and spawn operation it uses is either an entry of this table or a
// function derived from the entries through wrappers.  The data functions (memset family) are the
// summaries every extraction applies (spec_add_builtin_data_fns); they are listed here once, with the
// other entries, so that the whole table carries one version tag per entry.
//
// An entry covers one function of an API family, and a family is listed whole (every mutex
// primitive, every termination primitive, ...): the table names no resource type and no API of a
// particular program.  `version` is the reference version the semantics are read from;
// `source` is the line of the upstream source the entry was checked against, or "unverified".  The
// entries are entered by hand (tier researcher) and counted as such in the report.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/schema/kz_schema.h"

namespace kzs {

struct P0Entry {
  uint8_t kind = KZ_P0_KIND_NONE;  // KZ_P0_KIND
  const char* family = "";
  const char* name = "";
  int out = -1;    // the parameter written through (the new thread id), -1 if none
  int entry = -1;  // thread creation: the function the new thread starts at
  int arg = -1;    // thread creation: the argument it receives
  bool conditional = false;  // the call may return without acquiring / waiting (try, timed)
  int mutex = -1;            // mutexes and condition waits: the mutex argument the semantics name (not written)
  // data functions: the summary the extraction applies (spec.hpp DataFn)
  int dest = -1, src = -1, fill = -1;
  bool zero_fill = false;
  uint8_t writer_kind = KZ_WRITER_KIND_NONE;
  const char* semantics = "";
  const char* version = "";
  const char* source = "";
};

// The table, in its fixed order (the data functions in the order spec_add_builtin_data_fns has always
// used).  The revision names the table as a whole; it changes whenever an entry does.
const std::vector<P0Entry>& p0_table();
const char* p0_table_revision();

// The comparison families: the byte-string comparison family (string.h, strings.h), whose functions
// return zero exactly when the bytes they compare are equal, and the thread-id comparison family
// (pthread_equal, non-zero exactly when the two ids are equal).  `kz-static relations` reads a key
// comparison from a call of one of them; the extraction does not use them, so they are a table of
// their own (with its own revision) and the P0 table and the facts' P0_TABLE section are as they were.
// The two compared operands are args 0 and 1 of every entry.
struct P0CompareEntry {
  const char* name = "";
  const char* family = "";
  const char* semantics = "";
  const char* version = "";
  const char* source = "";
  bool equal_is_zero = true;  // the result is zero when the operands are equal (else non-zero)
};
const std::vector<P0CompareEntry>& p0_compare_table();
const char* p0_compare_table_revision();

bool p0_is_delay(uint8_t kind);
bool p0_is_terminal(uint8_t kind);
bool p0_is_process_terminal(uint8_t kind);

}  // namespace kzs

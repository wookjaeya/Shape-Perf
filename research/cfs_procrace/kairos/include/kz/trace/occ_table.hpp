// occ_table.hpp -- the per-occurrence table of a replay output container and the comparison of two of them
// (`kz occ show`, `kz occ diff`; docs/replay_output.md).
//
// A container written with schema revision 4 carries OCC_BASIS, OCC_EDGES and REPLAY_READ (kz_replay.h): the table
// is read from them.  A container written before has SUMMARIES only: the table then gives what those records hold
// (the verdict, the summary view, the basis kinds and assumptions of summary.flags, the executing task of
// IDENTITIES), marks every row "not recorded" for what is missing (store-derived-order, control-order, the edges,
// the key), and never fills the missing part with an empty basis.
//
// The occurrence key (path independent: it names no actor, thread id or event position):
//   rspec | op | owner | resource | ordinal
// owner = "app[<record key>]" where the record establishes the owning application of the executing task (inferred or
// confirmed), followed by "#<task>" where the container records the task's place in that application (schema revision
// 5, REPLAY_READ format 2: OCC_BASIS task, real_trace.hpp ThreadIdentity::task_ordinal), else "app:undetermined";
// resource = the occurrence's key values (hex), or "name:<text arguments>" where the key was not fixed; ordinal = the
// k-th occurrence the executing task began with that key.  The comparison uses the task only when both containers
// record it.  It matches by the key, then the occurrences left by the key without the owner (the owners then differ),
// then the occurrences left whose key value one side fixed and the other did not, by their text arguments in place of
// the key value, where exactly one occurrence of each side has them (the key differs; said apart).  A key that more
// than one occurrence of a side shares, while the other side has it too, is never paired by order: the group is
// listed as ambiguous.  Every difference, an ambiguous group included, makes `kz occ diff` exit 3.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/status.hpp"

namespace kz::trace {

struct OccEdge {
  uint32_t from = 0, to = 0;
  uint8_t kind = 0;   // KZ_OCC_EDGE_KIND
  uint8_t flags = 0;  // KZ_OCC_EDGE_FLAG
  std::string rule;
};

struct OccRow {
  uint32_t index = 0;            // SUMMARIES index
  bool recorded = false;         // from OCC_BASIS (else from SUMMARIES only)
  std::string rspec, op, context, resource, name, owner_app, via, exec;
  uint32_t ordinal = 0, owner_key = KZ_NONE32, obs_reasons = 0, basis = 0, assumptions = 0;
  uint32_t call = KZ_NONE32, decision = KZ_NONE32;
  uint8_t status = 0, status_flags = 0, observation = 0, outcome = 0, owner_level = 0, relation = 0;
  uint16_t summary_flags = 0;
  uint16_t task = 0;           // OCC_BASIS task (0 = not established)
  bool task_recorded = false;  // the container records it (REPLAY_READ format 2)
  std::vector<OccEdge> edges;
};

struct OccTable {
  bool recorded = false;   // the container has OCC_BASIS (schema revision 4)
  bool task_recorded = false;  // its OCC_BASIS has the task's place in its application (REPLAY_READ format 2, revision 5)
  bool has_reading = false;
  kz_replay_read reading{};
  std::vector<std::pair<std::string, std::string>> basis;  // the BASIS items (replay inputs)
  std::vector<OccRow> rows;
};

Status read_occ_table(const Container& c, OccTable* out);

// The row's key; with_task: the task's place where the row's container records it.
std::string occ_key(const OccRow& r, bool with_owner = true, bool with_task = true);
// One line per occurrence after a head that names the reading.
std::string occ_table_text(const OccTable& t);

struct OccDiffCounts {
  // ambiguous: the groups of a shared key (their occurrences are not paired); key_differs: pairs matched by the text
  // arguments, the key value fixed on one side only
  uint64_t matched = 0, only_a = 0, only_b = 0, verdict = 0, basis = 0, owner_differs = 0, ambiguous = 0, ambiguous_rows = 0, key_differs = 0;
  bool any() const { return verdict != 0 || basis != 0 || only_a != 0 || only_b != 0 || owner_differs != 0 || ambiguous != 0 || key_differs != 0; }
};
// The comparison of two tables by the occurrence key: the verdict differences (status, its flags, the summary view
// and its reasons) and the basis differences (basis kinds, declared assumptions, outcome check, the rules of the
// edges) are listed apart; then the ambiguous groups, the pairs whose key differs, the occurrences of one side only.
// Without OCC_BASIS on either side the rows are paired by their position and only the verdict and the basis of
// summary.flags are compared (said in the head).
std::string occ_diff_text(const OccTable& a, const OccTable& b, OccDiffCounts* counts);

}  // namespace kz::trace

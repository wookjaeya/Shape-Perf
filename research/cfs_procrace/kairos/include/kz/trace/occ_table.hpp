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
// record it.  It matches in three passes, each on the occurrences the earlier ones left:
//   1. the full key;
//   2. the key without the owner, only where the owner is not established on one side (undetermined, or none
//      recorded), or the same application is established on both sides and the task's place in it on one side only.
//      Two established owners that differ (the application, or the task's place where both record it) are never the
//      same occurrence: such a candidate is not paired and is listed as "owners differ" (its rows stay one side only).
//      A candidate is paired only where nothing else the record says of the call differs (the role, the via names, the
//      text arguments); otherwise equal ordinals alone tie the two, and the candidate is listed as uncertain;
//   3. the occurrences whose key value one side fixed and the other did not, by their text arguments in place of the
//      key value, where exactly one occurrence of each side has them (the key differs; said apart).
// A key that more than one occurrence of a side shares (in pass 2: more than one candidate of a row) is never paired by
// order: the group is listed as ambiguous.  The verdict and basis differences count the pairs of the full key; the pairs
// of passes 2 and 3 are listed, and compared, in their own sections.  The basis comparison of a pair compares the
// basis kinds, the assumptions, the outcome check, the via names, the role and the rules of the edges per occurrence
// (rule name, kind, by-store, count), not the edge endpoints.  `kz occ diff` exits 4 where the pairing of some
// occurrences is not established (an ambiguous key, an uncertain candidate), else 3 with any difference, else 0.
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
  // matched: every pair = by_key (the full key, or the position) + owner_unestablished (the key without the owner, one
  // side's owner or task's place not established) + key_differs (the text arguments, the key value fixed on one side
  // only).  verdict, basis: the differences of the pairs of the full key; verdict_other, basis_other: those of the other
  // pairs.  ambiguous: the groups of a shared key or of a row with more than one candidate (their occurrences are not
  // paired); uncertain: candidates without the owner tied by the ordinal alone, the role, the via names or the text
  // arguments differing (not paired); owner_conflict: candidates without the owner whose established owners differ,
  // neither row paired otherwise (not paired; the rows count in only_a, only_b)
  uint64_t matched = 0, by_key = 0, owner_unestablished = 0, key_differs = 0, only_a = 0, only_b = 0, verdict = 0, basis = 0, verdict_other = 0, basis_other = 0;
  uint64_t ambiguous = 0, ambiguous_rows = 0, uncertain = 0, uncertain_rows = 0, owner_conflict = 0;
  bool any() const {
    return verdict != 0 || basis != 0 || verdict_other != 0 || basis_other != 0 || only_a != 0 || only_b != 0 || owner_unestablished != 0 || key_differs != 0 ||
           ambiguous != 0 || uncertain != 0 || owner_conflict != 0;
  }
};
// The comparison of two tables by the occurrence key: the verdict differences (status, its flags, the summary view
// and its reasons) and the basis differences (basis kinds, declared assumptions, outcome check, the rules of the
// edges) of the pairs of the full key are listed apart; then the pairs made with one side's owner not established, the
// pairs whose key value differs, the uncertain candidates, the ambiguous groups, the candidates whose owners differ,
// the occurrences of one side only.  A head line gives the exit status (occ_diff_status) and its reason.  Without
// OCC_BASIS on either side the rows are paired by their position and only the verdict and the basis of summary.flags
// are compared (said in the head).
std::string occ_diff_text(const OccTable& a, const OccTable& b, OccDiffCounts* counts);
// The exit status of `kz occ diff`: 4 where the pairing of some occurrences is not established (an ambiguous group, an
// uncertain candidate), else 3 with any difference (owners that differ included), else 0.
int occ_diff_status(const OccDiffCounts& n);

}  // namespace kz::trace

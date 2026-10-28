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
// confirmed), else "app:undetermined"; resource = the occurrence's key values (hex), or "name:<text arguments>" where
// the key was not fixed; ordinal = the k-th occurrence the executing task began with that key.  Two tasks of one
// application with the same resource and rspec would share a key: the comparison then pairs them in the order of the
// containers and says so.  The comparison first matches by the key, then the occurrences left by the key without the
// owner (the owners then differ, and the comparison says so).
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
  std::vector<OccEdge> edges;
};

struct OccTable {
  bool recorded = false;   // the container has OCC_BASIS (schema revision 4)
  bool has_reading = false;
  kz_replay_read reading{};
  std::vector<std::pair<std::string, std::string>> basis;  // the BASIS items (replay inputs)
  std::vector<OccRow> rows;
};

Status read_occ_table(const Container& c, OccTable* out);

std::string occ_key(const OccRow& r, bool with_owner = true);
// One line per occurrence after a head that names the reading.
std::string occ_table_text(const OccTable& t);

struct OccDiffCounts {
  uint64_t matched = 0, only_a = 0, only_b = 0, verdict = 0, basis = 0, owner_differs = 0, ambiguous = 0;
};
// The comparison of two tables by the occurrence key: the verdict differences (status, its flags, the summary view
// and its reasons) and the basis differences (basis kinds, declared assumptions, outcome check, the rules of the
// edges) are listed apart; then the occurrences of one side only.  Without OCC_BASIS on either side the rows are
// paired by their position and only the verdict and the basis of summary.flags are compared (said in the head).
std::string occ_diff_text(const OccTable& a, const OccTable& b, OccDiffCounts* counts);

}  // namespace kz::trace

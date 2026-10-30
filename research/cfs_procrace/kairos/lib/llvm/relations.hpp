// relations.hpp -- `kz-static relations`: the relations of a facts container, without a case input.
//
// The query reads one facts container (the extended facts F1-F8 and the target-independent rules:
// `extract --no-profile`, or a profile-driven extraction with `--extended-facts`) and writes, for the
// whole program, the rows of three kinds of relation (plan/extraction_design_20261030.md §3.1-§3.4):
//
//   A  name / id / registration tables: a key comparison (a call of the byte-string comparison family of
//      P0, or `==` / `!=`) between a record field and a parameter, in a search loop or in a callback a
//      search loop calls; the functions whose parameter reaches it (PARAM_FLOW); the calls of them from
//      outside (uses), split into lookups and creators (a creator reaches a write of the key or of an
//      active field); per lookup, the creators of the same partition and key in other actors;
//   B  a shared field passed as a call argument (ARGFLOW, shared base) to a function with a body, when
//      the field has writers in another actor; with the callee's check of that parameter (RETURNS);
//   C  creation and registration: a spawn call, the writes of its creator after it, and the reads of the
//      same fields in the code the spawned entry reaches, with the lock sets (LOCKSET, LOCK_IDS).
//
// Every row has separate columns: dependency basis, run order (from a run record only; this revision
// reads none, so the column says so), feasibility of the other order (static, graded: proven impossible
// / possible / undecidable, with the reason), violation basis (RV2 §5.4: an observed failure with a
// feasible other order, or a realized forced order; without a record: unconfirmed), the counterfactual
// outcome-flip signal (static-possible / static-undetermined / none), and the input layers the row rests
// on (B build, P0 primitives, L0 loader semantics of the startup script, R rules; a case profile or a
// hand-entered vocabulary of the facts is named as such).  The query takes no field, callee or case
// name.  Deterministic: the same container gives the same bytes.  No Clang types.
#pragma once

#include <string>

#include "kz/core/container.hpp"

namespace kzs {

const char* relations_rules_revision();

struct RelationsOptions {
  // nothing yet: the query has no parameter that selects what it reports
};

kz::Status relations_report(const kz::Container& facts, const RelationsOptions& options, std::string* out);

}  // namespace kzs

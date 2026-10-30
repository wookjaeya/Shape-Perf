// static_closure.hpp -- what the static closure of the program says about the writers and aliases of the
// record fields a profile binds, matched with the probe plan of a real record.
//
// The facts container of `kz-static extract` (artifact kind FACTS; docs/static_facts.md section 4) states, for
// every target field (Type.field) of the profile, its writer set (WRITERS), its A-TYPE hits (FACT_ATYPE) and a
// status (FACT_LOCS): `writer-set-complete` when the checks of SPEC 4.2 found no cast, address escape, union,
// untyped write or unknown effect, `DISABLED` otherwise.  That status rests on the assumptions G1-G4 of
// docs/static_facts.md section 8; it is not a proved property.
//
// For each kind and location of the profile, the fields bound to it (`bind field T F as state(K(...))`,
// `tag(K(...), Tg)`, `id(K(...))` or `value(L)`) are its record components.  This function sets, in the real
// trace (SPEC 4.8, ADR-055), only these two items of eval::Adequacy, and only where the closure says so:
//   aliases_covered  every bound field is a target of the closure with status writer-set-complete (no A-TYPE
//                    hit: under G1-G4 nothing but its typed writers can write it);
//   writers_covered  aliases_covered, and every static writer of every bound field is at a point the record
//                    observes: a point site of the plan probed at the writer's file and line whose point the
//                    profile maps to a write of that component (a lifecycle or raise effect of the kind for a
//                    state field, a settag of the tag, an effect with `id` for an id field, a store into the
//                    location or a publication site of the location for a value field), or, for a writer inside a
//                    function the plan does not probe, every direct call of that function is (recursively, a few
//                    levels) at such a point.  A function that is the target of an indirect call, or that has no
//                    direct call, leaves its writers unobserved.  An out-pointer writer is matched at its call site.
// Nothing else changes: start state, lifetime, the observation of every profile writer effect's point and
// the absence of loss are still required by the replay before recorded writes fix anything (ADR-055).  A
// kind or location none of whose fields is bound is left as it was.
//
// The matching is by file and line of the probe; it does not read the binary.  What it cannot establish stays
// uncovered, and the writer and alias locations it found unobserved are listed per kind and location
// (eval::StaticClosure::unobserved, OBS_ADEQUACY `unobserved`).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/status.hpp"

namespace kz::trace {

// A point site of the record whose label names a point or site of the profile, with where it is probed.
struct ObservedSite {
  std::string point;  // the point (or site) name the replay maps it to
  std::string file;   // the source file of the probe (the run container's SITES `source`)
  uint32_t line = 0;
};

struct ClosureReport {
  uint64_t targets = 0;        // FACT_LOCS rows
  uint64_t bound_fields = 0;   // fields the profile binds to a kind or location
  uint64_t writers = 0, writers_observed = 0;
  std::vector<std::string> lines;  // one per kind / location (for the replay's report text)
};

// `facts` is a FACTS container; nothing is read from the binary.
Status apply_static_closure(const Container& facts, const phi::Profile& p, const std::vector<ObservedSite>& observed, eval::Trace* trace, ClosureReport* report);

}  // namespace kz::trace

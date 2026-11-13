// facts_query.hpp -- questions asked of a facts container, answered from the container alone.
#pragma once

#include <string>
#include <vector>

#include "kz/core/container.hpp"

namespace kzs {

struct ReachQuery {
  std::vector<std::string> roots;  // function names; every function of that name is a root
  std::string loc;                 // "Type.field": the derives rows whose argument derives from it
  std::string callee;              // only calls to this callee (empty = any)
  bool follow_spawn = false;       // also follow the entry functions of spawn calls
};

// For every call site whose argument (of a parameter with a role) derives from `loc`, whether the
// function that contains the site is reachable in the static call graph from each root.  The
// graph has the direct calls and the resolved targets of indirect calls; a site that could not
// be resolved contributes no edge, and their number is reported (so "not reachable" is a
// statement about the resolved graph only).
kz::Status reach_report(const kz::Container& facts, const ReachQuery& query, std::string* out);

}  // namespace kzs

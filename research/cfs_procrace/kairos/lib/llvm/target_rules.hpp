// target_rules.hpp -- extraction targets chosen by rule in the profile-free mode (`kz-static extract
// --no-profile --targets-rule R-T1,R-T2`), in place of a profile's locations or a `--target` list.
//
//   R-T1  every record field a call argument reads or passes the address of (ARGFLOW: the source of
//         the argument after its copies), when the base of the access path is shared: a global
//         object, an element of a global array, the pointee of a global pointer, the pointee of a
//         pointer field whose access path starts at a global, or the pointee of the result of a call
//         to a function that returns only addresses of global objects (or null);
//   R-T2  every record field a function reads with a shared base (READS: the access path starts at a
//         global, or at the result of such a call) as the value it returns or stores through a
//         pointer parameter, or in a branch condition when the field is also in the condition under
//         which the function returns a constant (RETURNS);
//
// Both rules read the first pass of the mode only (the extended facts, which do not depend on the
// targets).  A field that only a parameter, a local or the result of another call reaches is left out
// by both: that is what "shared" means here.  No Clang types.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "model.hpp"
#include "spec.hpp"

namespace kzs {

struct TargetRules {
  bool t1 = false, t2 = false;
  bool any() const { return t1 || t2; }
  std::string text() const;  // "R-T1,R-T2"
};

// "R-T1", "R-T2" or "R-T1,R-T2" (either order); false and `error` otherwise.
bool parse_target_rules(const std::string& text, TargetRules* out, std::string* error);
const char* target_rules_revision();

struct RuleTargetStats {
  uint32_t t1 = 0, t2 = 0, both = 0;          // fields per rule, and in both
  uint32_t t1_unshared_only = 0;              // fields the arguments reach only through an unshared base
  uint32_t t2_unshared_only = 0;              // fields the reads of R-T2 reach only through an unshared base
  uint32_t argflow_rows_shared = 0, read_rows_shared = 0;
};

// The targets of the rules over a finalized first pass, sorted by (type, field); each with the origin
// "rule R-T1", "rule R-T2" or "rule R-T1,R-T2".
std::vector<TargetField> rule_targets(const Model& first, const TargetRules& rules, RuleTargetStats* stats);

}  // namespace kzs

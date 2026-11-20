// phi_allowed.hpp -- the caller-level Allowed of a readiness specification (SPEC 1.6), under the
// strict reading adopted by the PI (QT-4, Q15).
//
// The profile states Allowed as entries, each backed by provenance records with a side:
//   supplier  what the supplier returns or documents (framework level);
//   caller    a text saying what the using code must achieve or how it handles the outcome;
//   rule      a fixed rule of the tool, applied to the code, with no text that says the outcome is fine.
// The strict reading:
//   - an entry with a `caller` record is read as a horizon monitor and decides statuses;
//   - an entry with only `rule` records is NOT accepted.  A rule alone never accepts;
//   - an entry with only `supplier` records decides nothing; it is framework-level information;
//   - rule-only and supplier-only entries are not part of the caller level (SPEC 1.6 step 2).  With
//     no caller entry the caller level is `none` if appreq is specified and unspecified_by_reference
//     otherwise (step 1) -- the same for no entry, a rule-only entry and a supplier-only entry
//     (litmus LP-ALLOW (b1r)); `none` needs a specified appreq (a caller-side record);
//   - a caller entry that the author marked as in conflict with appreq makes the caller-level
//     Allowed unspecified_by_reference, with the conflict named; it is never TOLERATED silently.
// The second kind of conflict of SPEC 1.6 step 3, two caller entries that accept incompatible
// continuations, is read only as the author's mark `conflicts_caller` on an entry (the profile carries no
// notion of compatible continuations to compute it): it makes the caller level unspecified_by_reference
// with the conflict named, like `conflicts_appreq`.
// The function below derives the result and says why; the reason text is what a report prints.
#pragma once

#include <string>
#include <vector>

#include "kz/core/phi_ast.hpp"

namespace kz {
namespace phi {

enum class CallerAllowedKind : uint8_t { None, Unspecified, Entries };

struct CallerAllowed {
  CallerAllowedKind kind = CallerAllowedKind::Unspecified;
  std::vector<const AllowedEntry*> deciding;       // entries read as horizon monitors (kind Entries)
  std::vector<const AllowedEntry*> rule_only;      // not accepted: a rule alone never accepts
  std::vector<const AllowedEntry*> framework_only; // supplier records only: decide nothing
  bool conflict = false;
  std::string reason;
};

// `p` must have been checked (provenance indices resolved).
CallerAllowed derive_caller_allowed(const Profile& p, const RspecDecl& r);

const char* caller_allowed_text(CallerAllowedKind k);

}  // namespace phi
}  // namespace kz

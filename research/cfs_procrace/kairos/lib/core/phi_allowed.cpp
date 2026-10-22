#include "kz/core/phi_allowed.hpp"

#include "kz/core/phi.hpp"
#include "kz/core/phi_provenance.hpp"

namespace kz {
namespace phi {

const char* caller_allowed_text(CallerAllowedKind k) {
  switch (k) {
    case CallerAllowedKind::None: return "none";
    case CallerAllowedKind::Unspecified: return "unspecified_by_reference";
    case CallerAllowedKind::Entries: return "entries";
  }
  return "?";
}

CallerAllowed derive_caller_allowed(const Profile& p, const RspecDecl& r) {
  CallerAllowed out;
  // A caller-side record of a tier that cannot be caller-side (hand-entered or code-derived) is read as
  // a rule: it never decides a status and never specifies appreq (QT-4, QT-9).  The checker rejects
  // such records; this keeps an unchecked or hand-built profile from getting around it.
  auto caller_record = [&](int32_t pi) {
    const ProvDecl& d = p.provs[static_cast<size_t>(pi)];
    return d.side == "caller" && caller_side_tier(d.tier);
  };
  const bool appreq = !r.appreq_unspecified && r.appreq_idx >= 0 && caller_record(r.appreq_idx);
  for (const AllowedEntry& a : r.allowed) {
    bool caller = false, rule = false, supplier = false;
    for (int32_t pi : a.prov_idx) {
      const std::string& side = p.provs[static_cast<size_t>(pi)].side;
      if (caller_record(pi)) caller = true;
      else if (side == "caller") rule = true;
      else if (side == "rule") rule = true;
      else if (side == "supplier") supplier = true;
    }
    if (caller) out.deciding.push_back(&a);
    else if (rule) out.rule_only.push_back(&a);
    else if (supplier) out.framework_only.push_back(&a);
  }
  for (const AllowedEntry* a : out.deciding)
    if (a->conflicts_appreq) out.conflict = true;

  if (r.allowed.empty()) {
    if (r.allowed_head.kind == AllowedKind::None) {
      if (appreq) {
        out.kind = CallerAllowedKind::None;
        out.reason = "Allowed none, and appreq is specified by a caller-side record: no not-ready outcome is acceptable";
      } else {
        out.kind = CallerAllowedKind::Unspecified;
        out.reason = "Allowed none needs a specified appreq (a caller-side record); there is none, so the caller-level Allowed is unspecified_by_reference";
      }
    } else {
      out.kind = CallerAllowedKind::Unspecified;
      out.reason = "Allowed is unspecified_by_reference";
    }
    return out;
  }

  if (out.conflict) {
    out.kind = CallerAllowedKind::Unspecified;
    out.deciding.clear();
    out.reason = "a caller entry is recorded as in conflict with appreq: the caller-level Allowed is unspecified_by_reference until the PI sets a precedence";
    return out;
  }
  if (!out.deciding.empty()) {
    out.kind = CallerAllowedKind::Entries;
    out.reason = "caller-backed entries decide the statuses";
    if (!out.rule_only.empty()) out.reason += "; rule-only entries are not accepted (strict reading)";
    return out;
  }
  // No caller entry decides.  Rule-only and supplier-only entries are not part of the caller level
  // (SPEC 1.6, QT-4): the caller level is then what it is with no entry at all -- none if a
  // specified appreq requires the state (litmus LP-ALLOW (b1r): VIOLATING), unspecified otherwise.
  // A rule never accepts, in either case.
  std::string what;
  if (!out.rule_only.empty() && !out.framework_only.empty()) what = "only rule-backed and supplier-side entries (a rule alone never accepts; supplier-side entries decide no status)";
  else if (!out.rule_only.empty()) what = "only rule-backed entries (a rule alone never accepts)";
  else if (!out.framework_only.empty()) what = "only supplier-side entries (framework level, they decide no status)";
  else what = "only entries that cite no record with a side";
  if (appreq) {
    out.kind = CallerAllowedKind::None;
    out.reason = what + " and a specified appreq: the entries are not part of the caller level, which is none";
  } else {
    out.kind = CallerAllowedKind::Unspecified;
    out.reason = what + " and no appreq: the caller-level Allowed is unspecified_by_reference";
  }
  return out;
}

}  // namespace phi
}  // namespace kz

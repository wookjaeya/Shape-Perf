// The caller-level Allowed of a readiness specification under the strict reading (QT-4, Q15):
// every branch of derive_caller_allowed.  The mini profile has one record of each side:
// doc_sup (supplier), doc_call (caller) and code_rule (rule).

#include <string>
#include <vector>

#include "kz/core/phi.hpp"
#include "kz/core/phi_allowed.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::phi::CallerAllowed;
using kz::phi::CallerAllowedKind;
using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::SourceText;

// Loads the mini profile with the rspec's `appreq` and `allowed` lines replaced.
bool load_with(const std::string& appreq, const std::string& allowed, Profile* p) {
  std::vector<SourceText> f = kzmini::files();
  std::string& t = f[2].text;
  size_t at = t.find("appreq doc_call;");
  t.replace(at, 16, appreq);
  at = t.find("allowed none;");
  t.replace(at, 13, allowed);
  std::vector<Diag> diags;
  const bool ok = kz::phi::load_profile(f, p, &diags).ok();
  if (!ok)
    for (const Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p->files, d).c_str());
  return ok;
}

CallerAllowed derive(const std::string& appreq, const std::string& allowed, bool* ok = nullptr) {
  Profile p;
  const bool loaded = load_with(appreq, allowed, &p);
  if (ok != nullptr) *ok = loaded;
  KZ_CHECK(loaded);
  if (!loaded) return CallerAllowed();
  return kz::phi::derive_caller_allowed(p, p.rspecs[0]);
}

}  // namespace

int main() {
  const std::string specified = "appreq doc_call;";
  const std::string unspecified = "appreq unspecified_by_reference;";

  // (1) no caller entry: none needs a specified appreq
  {
    const CallerAllowed a = derive(specified, "allowed none;");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::None));
    KZ_CHECK(a.deciding.empty());
    KZ_CHECK(!a.conflict);
  }
  {
    const CallerAllowed a = derive(unspecified, "allowed none;");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Unspecified));
    KZ_CHECK(a.reason.find("needs a specified appreq") != std::string::npos);
  }
  {
    const CallerAllowed a = derive(specified, "allowed unspecified_by_reference;");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Unspecified));
  }
  {
    const CallerAllowed a = derive(unspecified, "allowed unspecified_by_reference;");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Unspecified));
  }

  // (2) an entry with a caller record is read as a horizon monitor and decides
  {
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"a caller-side text\" prov doc_call; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Entries));
    KZ_CHECK_EQ(a.deciding.size(), static_cast<size_t>(1));
    KZ_CHECK(a.rule_only.empty());
    KZ_CHECK(a.framework_only.empty());
  }
  {  // with or without a specified appreq: the entries decide
    const CallerAllowed a = derive(unspecified, "allowed { documented_drop \"a caller-side text\" prov doc_call; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Entries));
  }

  // (3) a rule alone never accepts, and a rule-only entry is not part of the caller level (SPEC 1.6
  //     step 2): the caller level is what it is with no entry -- none with a specified appreq (litmus
  //     LP-ALLOW (b1r): VIOLATING), unspecified_by_reference without
  {
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"what a rule found\" prov code_rule; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::None));
    KZ_CHECK_EQ(a.rule_only.size(), static_cast<size_t>(1));
    KZ_CHECK(a.deciding.empty());
    KZ_CHECK(a.reason.find("a rule alone never accepts") != std::string::npos);
    KZ_CHECK(a.reason.find("specified appreq") != std::string::npos);
  }
  {
    const CallerAllowed a = derive(unspecified, "allowed { documented_drop \"what a rule found\" prov code_rule; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Unspecified));
    KZ_CHECK_EQ(a.rule_only.size(), static_cast<size_t>(1));
    KZ_CHECK(a.reason.find("no appreq") != std::string::npos);
  }
  {  // a rule record beside a supplier record is still a rule-only entry; same outcome as the two alone
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"x\" prov doc_sup, code_rule; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::None));
    KZ_CHECK_EQ(a.rule_only.size(), static_cast<size_t>(1));
    const CallerAllowed b = derive(unspecified, "allowed { documented_drop \"x\" prov doc_sup, code_rule; }");
    KZ_CHECK_EQ(static_cast<int>(b.kind), static_cast<int>(CallerAllowedKind::Unspecified));
  }
  {  // a rule-only entry and a supplier-only entry, side by side: neither is part of the caller level
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"x\" prov code_rule; documented_drop \"y\" prov doc_sup; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::None));
    KZ_CHECK_EQ(a.rule_only.size(), static_cast<size_t>(1));
    KZ_CHECK_EQ(a.framework_only.size(), static_cast<size_t>(1));
    KZ_CHECK(a.reason.find("rule-backed and supplier-side") != std::string::npos);
  }
  {  // a caller record beside a rule record decides
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"x\" prov doc_call, code_rule; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Entries));
    KZ_CHECK_EQ(a.deciding.size(), static_cast<size_t>(1));
  }
  {  // one deciding entry and one rule-only entry: the rule-only one is reported, not accepted
    const CallerAllowed a = derive(specified,
                                   "allowed { documented_drop \"x\" prov doc_call; documented_drop \"y\" prov code_rule; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Entries));
    KZ_CHECK_EQ(a.deciding.size(), static_cast<size_t>(1));
    KZ_CHECK_EQ(a.rule_only.size(), static_cast<size_t>(1));
    KZ_CHECK(a.reason.find("rule-only entries are not accepted") != std::string::npos);
  }

  // (4) supplier-only entries are framework-level information and decide nothing
  {
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"supplier text\" prov doc_sup; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::None));
    KZ_CHECK_EQ(a.framework_only.size(), static_cast<size_t>(1));
    KZ_CHECK(a.deciding.empty());
  }
  {
    const CallerAllowed a = derive(unspecified, "allowed { documented_drop \"supplier text\" prov doc_sup; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Unspecified));
    KZ_CHECK_EQ(a.framework_only.size(), static_cast<size_t>(1));
  }

  // (5) a caller entry that the author marked as in conflict with appreq withholds the judgement
  {
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"x\" conflicts_appreq prov doc_call; }");
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::Unspecified));
    KZ_CHECK(a.conflict);
    KZ_CHECK(a.deciding.empty());
    KZ_CHECK(a.reason.find("in conflict with appreq") != std::string::npos);
  }
  {  // the flag on a rule-only entry is not a conflict of a caller entry
    const CallerAllowed a = derive(specified, "allowed { documented_drop \"x\" conflicts_appreq prov code_rule; }");
    KZ_CHECK(!a.conflict);
    KZ_CHECK_EQ(static_cast<int>(a.kind), static_cast<int>(CallerAllowedKind::None));
  }

  // the words
  KZ_CHECK_EQ(std::string(kz::phi::caller_allowed_text(CallerAllowedKind::None)), std::string("none"));
  KZ_CHECK_EQ(std::string(kz::phi::caller_allowed_text(CallerAllowedKind::Unspecified)), std::string("unspecified_by_reference"));
  KZ_CHECK_EQ(std::string(kz::phi::caller_allowed_text(CallerAllowedKind::Entries)), std::string("entries"));

  return kztest::finish("test_core_phi_allowed");
}

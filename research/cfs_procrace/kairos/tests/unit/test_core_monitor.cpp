// The horizon monitor of retry_until_ready in the replay (SPEC 1.6 "Horizon monitors", docs/phi_reference.md
// sections 7.5 and 7.6) on a small profile written here: a semaphore by name, a lookup whose not-found result
// a caller record accepts while a later attempt of the same loop is READY within the bound, and a creation.
// Unit-test values only (the bound 3, the codes, the names): no value here is a research condition.
//
//   - success after a retry: the not-ready attempt is TOLERATED (ACCEPT), the READY one READY;
//     summary NO_VIOLATION_IN_CHECKED_PREFIX for both (a caller record and an ACCEPT, D2);
//   - exhaustion: the bound-th not-ready attempt is the exhaustion path: every attempt VIOLATING;
//   - the record ends in the middle of the retries: PENDING, INCONCLUSIVE(horizon-open) (OPEN is no ACCEPT);
//   - an attempt whose outcome is outside the codes REJECTs; a NOT_EVALUATED attempt is no ACCEPT;
//   - what cannot be counted is withheld: an attempt whose key is not fixed, a loss over the attempts, a
//     possible generation boundary (an effect back to the initial state) -> UNDETERMINED, explanation_ambiguous;
//   - attempts on another key or by another actor belong to other runs;
//   - the bound comes from the profile's constant (an integer, or a ref eval value the trace gives); without a
//     value the monitor is not run;
//   - a REJECT decides VIOLATING only for the only deciding entry;
//   - the P1 alternative (no outcome narrowing) leaves the attempts undetermined, and the monitor decides nothing;
//   - a model trace is judged by the same monitor.

#include <string>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/verdict.hpp"
#include "support/kz_test.hpp"

namespace {

using kz::eval::OccStatus;
using kz::eval::Occurrence;
using kz::eval::Result;
using kz::eval::Trace;
using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::SourceText;

const char* const kHead = R"KZ(phi 1;
profile "retry-mini" revision "unit test";
source main root "" rev "abcdef0";
prov doc_sup { tier contract-backed; kind api_doc; side supplier; file "inc/sem.h"; lines 1 2;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
prov adm_call { tier admitted; kind code_admission; side caller; file "src/user.c"; lines 3 4;
  sha256 "2222222222222222222222222222222222222222222222222222222222222222"; }
prov code_a { tier code-derived; kind code_pattern; side supplier; file "src/sem.c"; lines 5;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
const OK = 0 prov code_a;
const NOT_FOUND = -17 prov doc_sup;
const BAD_PTR = -2 prov doc_sup;
const TRIES = 3 prov code_a;
const TRIES_REF = ref eval "TRIES" in "src/user.c" prov code_a;

kind sem(name: Str) { states ABSENT < LIVE; initial ABSENT; scope process; prov code_a; }

op lookup(out id, key name: Str) {
  bind symbol "lookup";
  conjunct exists: state(sem(arg.name)) = LIVE lp at fn("lookup").call("find") prov code_a;
  outcome OK: ready;
  outcome NOT_FOUND: not_ready(exists) documented "not found" prov doc_sup;
  outcome BAD_PTR: error documented "bad pointer" prov doc_sup;
  prov code_a;
}

op create(out id, name: Str) {
  bind symbol "create";
  effect publish: lifecycle sem(arg.name) -> LIVE filter lp.status = OK lp at fn("create").call("finalize") values status: Int prov code_a;
  outcome OK: ready;
  prov code_a;
}
)KZ";

const char* const kDestroy = R"KZ(
op destroy(name: Str) {
  bind symbol "destroy";
  effect gone: lifecycle sem(arg.name) -> ABSENT lp at fn("destroy").call("release") prov code_a;
  prov code_a;
}
)KZ";

std::string rspec(const std::string& allowed) {
  return "rspec K-LOOKUP {\n  use op lookup;\n  key sem(arg.name);\n  requires state(sem(arg.name)) = LIVE;\n  oracle fns \"find\";\n"
         "  appreq unspecified_by_reference;\n  allowed { " + allowed + " }\n  harm app_error_exit;\n  authored \"unit test\";\n  prov code_a;\n}\n";
}

const char* const kRetry = "retry_until_ready codes { NOT_FOUND } bound TRIES prov adm_call;";

Profile load(const std::string& text) {
  Profile p;
  std::vector<Diag> diags;
  std::vector<SourceText> files;
  files.push_back(SourceText{"retry_mini.kzp", text});
  if (!kz::phi::load_profile(files, &p, &diags).ok()) {
    for (const Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
    ++kztest::state().failures;
  }
  return p;
}

struct Run {
  Trace trace;
  Result result;
  std::string dump;
  const Occurrence& occ(size_t i) const {
    static const Occurrence none;
    return i < result.occurrences.size() ? result.occurrences[i] : none;
  }
  std::string summary(size_t i) const { return kz::render_observation(occ(i).summary); }
  bool finding(const std::string& fragment) const {
    for (const kz::eval::ReplayFinding& f : result.findings)
      if (f.what.find(fragment) != std::string::npos) return true;
    return false;
  }
};

Run replay(const Profile& p, const std::string& text, bool narrowing = true) {
  Run r;
  const kz::Status ps = kz::eval::parse_trace_text(text, &r.trace);
  if (!ps.ok()) {
    std::fprintf(stderr, "trace does not parse: %s\n", ps.describe().c_str());
    ++kztest::state().failures;
  }
  kz::eval::Options o;
  o.outcome_narrowing = narrowing;
  KZ_CHECK(kz::eval::replay(p, r.trace, o, &r.result).ok());
  r.dump = kz::eval::dump_result(p, r.trace, r.result);
  return r;
}

void expect(const Run& r, size_t i, OccStatus st, const std::string& summary, const char* what) {
  const bool ok = r.occ(i).status == st && r.summary(i) == summary && kz::validate_summary(r.occ(i).summary).empty();
  KZ_CHECK(ok);
  if (!ok)
    std::fprintf(stderr, "%s: occurrence %zu is %s / %s, wanted %s / %s\n%s\n", what, i, kz::eval::occ_status_text(r.occ(i).status), r.summary(i).c_str(),
                 kz::eval::occ_status_text(st), summary.c_str(), r.dump.c_str());
}

bool has(const std::string& text, const std::string& fragment) { return text.find(fragment) != std::string::npos; }

// The real record: the point inside the lookup and the creation's point are observed; nothing else is
// established (the kind stays open, as in a run container), so READY comes from the returned code (P1 current).
const char* const kReal = "record real\nstart_evidence init\nobserved lookup.exists\nobserved create.publish\n";

std::string attempt(const char* actor, const char* name, const char* result, bool point = true) {
  std::string t = std::string(actor) + " lookup call" + (name != nullptr ? std::string(" arg.name=@") + name : std::string()) + "\n";
  if (point) t += std::string(actor) + " lookup.exists pt\n";
  return t + actor + " lookup ret result=" + result + "\n";
}

std::string create(const char* name) {
  return std::string("P create call arg.name=@") + name + "\nP create.publish pt lp.status=0\nP create ret result=0\n";
}

void test_success_after_retry() {
  const Profile p = load(std::string(kHead) + rspec(kRetry));
  const std::string body = attempt("C", "S0", "-17") + create("S0") + attempt("C", "S0", "0");
  const Run r = replay(p, kReal + body);
  expect(r, 0, OccStatus::Tolerated, "NO_VIOLATION_IN_CHECKED_PREFIX", "retried");
  expect(r, 1, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", "retried");
  KZ_CHECK(r.occ(0).accepted);
  KZ_CHECK(has(r.occ(0).monitor, "ACCEPT at #"));
  KZ_CHECK(has(r.dump, "monitor retry_until_ready ACCEPT"));
  KZ_CHECK(has(r.occ(0).monitor, "bound TRIES = 3"));
  // the same as a model trace: the monitor is not specific to real records
  const Run m = replay(p, body);
  KZ_CHECK(m.occ(0).status == OccStatus::Tolerated);
  KZ_CHECK(m.occ(1).status == OccStatus::Ready);
  // the READY attempt is the last possible one (attempt 3 of bound 3): still within the bound
  const Run last = replay(p, kReal + attempt("C", "S0", "-17") + attempt("C", "S0", "-17") + create("S0") + attempt("C", "S0", "0"));
  expect(last, 0, OccStatus::Tolerated, "NO_VIOLATION_IN_CHECKED_PREFIX", "ready at the bound");
  expect(last, 1, OccStatus::Tolerated, "NO_VIOLATION_IN_CHECKED_PREFIX", "ready at the bound");
  expect(last, 2, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", "ready at the bound");
  // another key between the attempts is another run; another actor too
  const Run keys = replay(p, kReal + attempt("C", "S0", "-17") + create("S0") + create("S1") + attempt("C", "S1", "0") + attempt("D", "S0", "-17") +
                                 attempt("C", "S0", "0"));
  expect(keys, 0, OccStatus::Tolerated, "NO_VIOLATION_IN_CHECKED_PREFIX", "two keys");
  expect(keys, 1, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", "two keys");
  expect(keys, 2, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", "another actor");
  expect(keys, 3, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", "two keys");
}

void test_exhaustion() {
  const Profile p = load(std::string(kHead) + rspec(kRetry));
  const Run r = replay(p, kReal + attempt("C", "S0", "-17") + attempt("C", "S0", "-17") + attempt("C", "S0", "-17") + create("S0") + attempt("C", "S0", "0"));
  for (size_t i = 0; i < 3; ++i) expect(r, i, OccStatus::Violating, "OBSERVED_VIOLATION", "exhausted");
  KZ_CHECK(has(r.occ(2).monitor, "the bound is exhausted, attempt 3 not ready"));
  // the attempt after the exhaustion begins a new run: it is READY, and accepts nothing before it
  expect(r, 3, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", "after the exhaustion");
  // an outcome outside the codes rejects; the attempt that returned it before the point is NOT_EVALUATED
  const Run out = replay(p, kReal + attempt("C", "S0", "-17") + attempt("C", "S0", "-2", false));
  expect(out, 0, OccStatus::Violating, "OBSERVED_VIOLATION", "outside the codes");
  expect(out, 1, OccStatus::NotEvaluated, "INCONCLUSIVE(not_evaluated)", "outside the codes");
  KZ_CHECK(has(out.occ(0).monitor, "is outside the codes"));
  // a REJECT decides only for the only deciding entry
  const Profile two = load(std::string(kHead) + rspec(std::string(kRetry) + " exit BAD_PTR \"user text\" prov adm_call;"));
  const Run r2 = replay(two, kReal + attempt("C", "S0", "-17") + attempt("C", "S0", "-17") + attempt("C", "S0", "-17"));
  for (size_t i = 0; i < 3; ++i) expect(r2, i, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", "two entries");
  KZ_CHECK(has(r2.occ(0).monitor, "another deciding entry has not rejected"));
}

void test_record_ends_mid_retry() {
  const Profile p = load(std::string(kHead) + rspec(kRetry));
  const Run r = replay(p, kReal + attempt("C", "S0", "-17") + attempt("C", "S0", "-17"));
  expect(r, 0, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", "mid-retry");
  expect(r, 1, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", "mid-retry");
  KZ_CHECK(has(r.occ(1).monitor, "OPEN: the trace ends after attempt 2"));
  // a call that has not returned at the end of the record: OPEN itself, and the run stays open
  const Run open = replay(p, kReal + attempt("C", "S0", "-17") + "C lookup call arg.name=@S0\nC lookup.exists pt\n");
  expect(open, 0, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", "last call open");
  KZ_CHECK(open.occ(1).status == OccStatus::Open);
}

void test_not_countable() {
  const Profile p = load(std::string(kHead) + rspec(kRetry));
  // the name is not recorded: the key is not fixed, the attempts cannot be counted
  const Run nokey = replay(p, kReal + attempt("C", nullptr, "-17") + attempt("C", nullptr, "0"));
  KZ_CHECK(nokey.occ(0).status == OccStatus::Undetermined);
  KZ_CHECK(has(nokey.summary(0), "explanation_ambiguous"));
  KZ_CHECK(has(nokey.occ(0).monitor, "is not fixed"));
  // a reported loss over the attempts: some may be missing
  const Run lossy = replay(p, kReal + attempt("C", "S0", "-17") + "loss begin\n" + create("S0") + "loss end\n" + attempt("C", "S0", "0"));
  KZ_CHECK(lossy.occ(0).status == OccStatus::Undetermined);
  KZ_CHECK(has(lossy.summary(0), "loss"));
  KZ_CHECK(has(lossy.summary(0), "explanation_ambiguous"));
  // a possible generation boundary: an effect that returns the semaphore to ABSENT ...
  const Profile gen = load(std::string(kHead) + kDestroy + rspec(kRetry));
  const std::string between = "Q destroy call arg.name=@S0\nQ destroy.gone pt\nQ destroy ret\n";
  const Run g1 = replay(gen, std::string(kReal) + "observed destroy.gone\n" + attempt("C", "S0", "-17") + between + create("S0") + attempt("C", "S0", "0"));
  KZ_CHECK(g1.occ(0).status == OccStatus::Undetermined);
  KZ_CHECK(has(g1.occ(0).monitor, "generation boundary"));
  // ... observed elsewhere: the attempts are one generation
  const Run g2 = replay(gen, std::string(kReal) + "observed destroy.gone\n" + between + attempt("C", "S0", "-17") + create("S0") + attempt("C", "S0", "0"));
  expect(g2, 0, OccStatus::Tolerated, "NO_VIOLATION_IN_CHECKED_PREFIX", "no boundary between");
  // ... not observed by the record at all: a boundary may lie anywhere
  const Run g3 = replay(gen, kReal + attempt("C", "S0", "-17") + create("S0") + attempt("C", "S0", "0"));
  KZ_CHECK(g3.occ(0).status == OccStatus::Undetermined);
}

void test_bound_from_profile() {
  // a ref eval bound: the value comes from the trace; without it the monitor is not run
  const Profile p = load(std::string(kHead) + rspec("retry_until_ready codes { NOT_FOUND } bound TRIES_REF prov adm_call;"));
  const std::string body = attempt("C", "S0", "-17") + attempt("C", "S0", "-17");
  const Run none = replay(p, kReal + body);
  expect(none, 0, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", "no bound");
  KZ_CHECK(none.finding("the retry_until_ready monitor is not run"));
  KZ_CHECK(none.occ(0).monitor.empty());
  const Run two = replay(p, std::string(kReal) + "const TRIES_REF 2\n" + body);
  expect(two, 0, OccStatus::Violating, "OBSERVED_VIOLATION", "bound 2 from the trace");
  KZ_CHECK(has(two.occ(1).monitor, "bound TRIES_REF = 2"));
}

void test_p1_alternative() {
  // without outcome narrowing the codes fix nothing the record leaves open: every attempt is undetermined
  // (READY among its possibilities), and the monitor decides nothing
  const Profile p = load(std::string(kHead) + rspec(kRetry));
  const Run r = replay(p, kReal + attempt("C", "S0", "-17") + create("S0") + attempt("C", "S0", "0"), false);
  KZ_CHECK(r.occ(0).status == OccStatus::Undetermined);
  KZ_CHECK(r.occ(1).status == OccStatus::Undetermined);
  KZ_CHECK(!r.occ(0).accepted);
}

}  // namespace

int main() {
  test_success_after_retry();
  test_exhaustion();
  test_record_ends_mid_retry();
  test_not_countable();
  test_bound_from_profile();
  test_p1_alternative();
  return kztest::finish("test_core_monitor");
}

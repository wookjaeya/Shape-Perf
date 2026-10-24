// State-based judgement of a lookup whose provider is another task (2026-10-24, C1 state-based): the language items
// `bind ... via` (key separation of a shared field) and the hb template `reads_from` (SPEC 3.16 row RF), the order of a
// read against the writes of the teardown (SCOPE, SPEC 4.5; SPEC 1.6 "Teardown decides no default-scope occurrence")
// and the monitor of the Allowed entry `exit(code, src)` (SPEC 1.6 "Horizon monitors").  A small profile written
// here; every value is a unit-test value, not a research condition.
//
//   - `via` is parsed, printed in a sorted normal form and checked (each function binds an operation); a reads_from
//     rule names a conjunct, a ready outcome, a lifecycle publication on the conjunct's kind and out parameters of
//     both operations, and is an observed (O) rule;
//   - with the start state from an observed clearing of the whole kind, a successful lookup of another task's
//     creation is READY on the computed state once the reads-from edge (matched by name and id) orders the creation
//     before the lookup's point: not outcome-narrowed, in both P1 readings; without the edge, or with other ids, the
//     read stays open (outcome-narrowed under P1, undetermined without);
//   - a delete at teardown whose target the record does not give (after the TEARDOWN point on the main thread) is left
//     out of the order of a default-scope read; without the profile's scope it opens the read;
//   - exit(code): ACCEPT at a terminal call with that code (TOLERATED), REJECT with another code (VIOLATING for the
//     only deciding entry), OPEN when the record ends, no decision at a process-terminal step or at the teardown, a
//     terminal call without its code withheld; with no creation anywhere in a fully covered record the failed lookup's
//     not-ready state is computed (not outcome-narrowed).

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

const char* const kProv = R"KZ(phi 1;
profile "state-order-mini" revision "unit test";
source main root "" rev "abcdef0";
prov doc_sup { tier contract-backed; kind api_doc; side supplier; file "inc/sem.h"; lines 1 2;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
prov doc_call { tier contract-backed; kind api_doc; side caller; file "doc/user.md"; lines 3 4;
  sha256 "2222222222222222222222222222222222222222222222222222222222222222"; }
prov code_a { tier code-derived; kind code_pattern; side supplier; file "src/sem.c"; lines 5;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
const OK = 0 prov code_a;
const NOT_FOUND = -17 prov doc_sup;
const QUIT_ERR = 3 prov code_a;
)KZ";

const char* const kKinds = R"KZ(
kind sem(name: Str) { states ABSENT < LIVE; initial ABSENT; scope process; prov code_a; }
kind shut() { states RUN < REQ < DOWN; initial RUN; scope process; prov code_a; }
bind field "Rec_t" "id" as state(sem(name)) via "wipe", "destroy", "create" prov code_a;

op lookup(out id, key name: Str) {
  bind symbol "lookup";
  conjunct exists: state(sem(arg.name)) = LIVE lp at fn("lookup").call("find") prov code_a;
  outcome OK: ready;
  outcome NOT_FOUND: not_ready(exists) documented "not found" prov doc_sup;
  prov code_a;
}
op create(out id, name: Str) {
  bind symbol "create";
  effect publish: lifecycle sem(arg.name) -> LIVE filter lp.status = OK lp at fn("create").call("finalize") values status: Int prov code_a;
  outcome OK: ready;
  prov code_a;
}
op wipe() { bind symbol "wipe"; effect clear: lifecycle all sem -> ABSENT lp at fn("wipe").memset(Rec_t) prov code_a; prov code_a; }
op destroy(id) { bind symbol "destroy"; effect gone: lifecycle sem(lp.name) -> ABSENT lp at fn("destroy").call("fin") values name: Str prov code_a; prov code_a; }
op restart() { bind symbol "restart"; effect req: lifecycle shut() -> REQ lp at fn("restart").call("kill") prov code_a; prov code_a; }
op run() { bind symbol "run"; effect down: lifecycle shut() -> DOWN lp at fn("run").call("delete_all") prov code_a; prov code_a; }
op spawn(out tid) { bind symbol "spawn"; prov code_a; }
op entry(tid) { bind symbol "entry"; prov code_a; }
op quit(status) { bind symbol "quit"; terminal; prov code_a; }
op halt() { bind symbol "halt"; terminal process_terminal; prov code_a; }

hb PO { title "program order"; stratum S; robust unconditional; template program_order { looper no; } prov code_a; }
hb SP { title "spawn"; stratum S; robust unconditional; template spawn { create spawn out tid; entry entry; } prov code_a; }
hb RF {
  title "reads-from";
  stratum O;
  robust obs_soft;
  template reads_from { use lookup.exists; outcome OK; publish create.publish; match id; }
  prov code_a;
}
)KZ";

const char* const kScope = "scope { kind shut; request REQ; teardown DOWN; prov code_a; }\n";

std::string rspec(const std::string& allowed) {
  return "rspec K-LOOKUP {\n  use op lookup;\n  key sem(arg.name);\n  requires state(sem(arg.name)) = LIVE;\n  oracle fns \"find\";\n"
         "  appreq unspecified_by_reference;\n  allowed " + allowed + "\n  harm app_error_exit;\n  authored \"unit test\";\n  prov code_a;\n}\n";
}

Profile load(const std::string& text, bool expect_ok = true, std::string* diag_text = nullptr) {
  Profile p;
  std::vector<Diag> diags;
  std::vector<SourceText> files;
  files.push_back(SourceText{"state_order_mini.kzp", text});
  const bool ok = kz::phi::load_profile(files, &p, &diags).ok();
  std::string all;
  for (const Diag& d : diags) all += kz::phi::format_diag(p.files, d) + "\n";
  if (diag_text != nullptr) *diag_text = all;
  if (ok != expect_ok) {
    std::fprintf(stderr, "profile load %s unexpectedly:\n%s", ok ? "succeeded" : "failed", all.c_str());
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
  bool edge(const std::string& rule) const {
    for (const kz::eval::HbEdge& e : result.edges)
      if (e.rule == rule) return true;
    return false;
  }
};

Run replay(const Profile& p, const std::string& text, bool narrowing = true, bool reads_from = true) {
  Run r;
  const kz::Status ps = kz::eval::parse_trace_text(text, &r.trace);
  if (!ps.ok()) {
    std::fprintf(stderr, "trace does not parse: %s\n", ps.describe().c_str());
    ++kztest::state().failures;
  }
  kz::eval::Options o;
  o.outcome_narrowing = narrowing;
  o.reads_from_edges = reads_from;
  KZ_CHECK(kz::eval::replay(p, r.trace, o, &r.result).ok());
  r.dump = kz::eval::dump_result(p, r.trace, r.result);
  return r;
}

bool has(const std::string& text, const std::string& fragment) { return text.find(fragment) != std::string::npos; }

void expect(const Run& r, size_t i, OccStatus st, const std::string& summary, bool narrowed, const char* what) {
  const bool ok = r.occ(i).status == st && r.summary(i) == summary && r.occ(i).outcome_narrowed == narrowed && kz::validate_summary(r.occ(i).summary).empty();
  KZ_CHECK(ok);
  if (!ok)
    std::fprintf(stderr, "%s: occurrence %zu is %s / %s narrowed %d, wanted %s / %s narrowed %d\n%s\n", what, i, kz::eval::occ_status_text(r.occ(i).status), r.summary(i).c_str(),
                 r.occ(i).outcome_narrowed ? 1 : 0, kz::eval::occ_status_text(st), summary.c_str(), narrowed ? 1 : 0, r.dump.c_str());
}

// The record: the kind's writers, aliases and lifetime declared established (what the adapter sets from a closure),
// every writer point observed; the start state comes from the observed clearing (no start=known).
const char* const kHeader =
    "record real\nstart_evidence init\nadequacy kind sem writers=covered aliases=covered lifetime=known\n"
    "observed lookup.exists\nobserved create.publish\nobserved wipe.clear\nobserved destroy.gone\nobserved restart.req\nobserved run.down\n";

// M clears the table and creates A (the user) and B (the provider).
const char* const kStart =
    "M wipe.clear pt\n"
    "M spawn call\nM spawn ret out.tid=1 result=0\nA entry call arg.tid=1\n"
    "M spawn call\nM spawn ret out.tid=2 result=0\nB entry call arg.tid=2\n";

std::string create(const char* id) { return std::string("B create call arg.name=@S0\nB create.publish pt lp.status=0\nB create ret out.id=") + id + " result=0\n"; }
std::string lookup(const char* result, const char* id) {
  return std::string("A lookup call arg.name=@S0\nA lookup.exists pt\nA lookup ret ") + (id != nullptr ? std::string("out.id=") + id + " " : std::string()) + "result=" + result + "\n";
}
const char* const kTeardown = "C restart call\nC restart.req pt\nM run call\nM run.down pt\nM destroy call arg.id=0x30002\nM destroy.gone pt\nM destroy ret result=0\n";

void test_language() {
  const Profile p = load(std::string(kProv) + kKinds + kScope + rspec("unspecified_by_reference;"));
  const std::string nf = kz::phi::canonical_text(p);
  KZ_CHECK(has(nf, "bind field \"Rec_t\" \"id\" as state(sem(name)) via \"create\", \"destroy\", \"wipe\" prov code_a;"));
  KZ_CHECK(has(nf, "template reads_from {"));
  KZ_CHECK(has(nf, "use lookup.exists;"));
  KZ_CHECK(has(nf, "publish create.publish;"));
  KZ_CHECK(has(nf, "match id;"));
  // the normal form reads back to the same profile
  const Profile again = load(nf);
  KZ_CHECK(kz::phi::profile_hash(again) == kz::phi::profile_hash(p));
  // the checks
  std::string d;
  load(std::string(kProv) + kKinds + "bind field \"Rec_t\" \"x\" as state(sem(name)) via \"nowhere\" prov code_a;\n", false, &d);
  KZ_CHECK(has(d, "the 'via' function 'nowhere' is not the bound symbol of an operation"));
  const std::string bad_rf = R"KZ(
hb RF2 { title "bad"; stratum S; robust unconditional; template reads_from { use lookup.missing; outcome NOT_FOUND; publish wipe.clear; match tid; } prov code_a; }
)KZ";
  load(std::string(kProv) + kKinds + bad_rf, false, &d);
  KZ_CHECK(has(d, "a reads-from rule is an observed (O) rule"));
  KZ_CHECK(has(d, "'missing' is not a conjunct of 'lookup'"));
  KZ_CHECK(has(d, "'NOT_FOUND' is not a ready outcome of 'lookup'"));
  KZ_CHECK(has(d, "the publication is a lifecycle effect on one resource"));
  KZ_CHECK(has(d, "'match tid' is not an out parameter of 'lookup'"));
}

void test_reads_from_state() {
  const Profile p = load(std::string(kProv) + kKinds + kScope + rspec("unspecified_by_reference;"));
  const std::string body = kStart + create("0x30002") + lookup("0", "0x30002");
  // the creation is ordered before the lookup's point by RF: the state is computed (LIVE), in both P1 readings
  const Run r = replay(p, std::string(kHeader) + body);
  KZ_CHECK(r.edge("RF"));
  expect(r, 0, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", false, "reads-from");
  KZ_CHECK(r.finding("read from the happens-before edges"));
  const Run off = replay(p, std::string(kHeader) + body, false);
  expect(off, 0, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", false, "reads-from, P1 alternative");
  // without the edge (left out, or other ids): another task's write is not ordered with the read
  const Run norf = replay(p, std::string(kHeader) + body, true, false);
  KZ_CHECK(!norf.edge("RF"));
  expect(norf, 0, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", true, "no reads-from edge");
  const Run norf_off = replay(p, std::string(kHeader) + body, false, false);
  KZ_CHECK(norf_off.occ(0).status == OccStatus::Undetermined);
  KZ_CHECK(has(norf_off.summary(0), "explanation_ambiguous"));
  const Run other = replay(p, std::string(kHeader) + kStart + create("0x30002") + lookup("0", "0x30003"));
  KZ_CHECK(!other.edge("RF"));
  KZ_CHECK(other.occ(0).outcome_narrowed);
  const Run noid = replay(p, std::string(kHeader) + kStart + create("0x30002") + lookup("0", nullptr));
  KZ_CHECK(!noid.edge("RF"));
  KZ_CHECK(noid.finding("carries no out.id"));
  // a failed lookup has no reads-from edge: its not-ready state rests on the code (P1)
  const Run failed = replay(p, std::string(kHeader) + kStart + lookup("-17", nullptr) + create("0x30002"));
  KZ_CHECK(!failed.edge("RF"));
  expect(failed, 0, OccStatus::NotReadyUnjudged, "INCONCLUSIVE(caller_requirement_absent)", true, "failed lookup");
  // no observed clearing: the start state is unknown and the read stays open
  const Run nostart = replay(p, std::string(kHeader) + std::string(kStart).substr(std::string("M wipe.clear pt\n").size()) + create("0x30002") + lookup("0", "0x30002"));
  KZ_CHECK(nostart.occ(0).outcome_narrowed);
}

void test_scope_teardown() {
  const std::string body = kStart + create("0x30002") + lookup("0", "0x30002") + kTeardown;
  // the delete at teardown (its name not recorded) is after the TEARDOWN point on the main thread: left out
  const Profile p = load(std::string(kProv) + kKinds + kScope + rspec("unspecified_by_reference;"));
  const Run r = replay(p, std::string(kHeader) + body);
  expect(r, 0, OccStatus::Ready, "NO_VIOLATION_IN_CHECKED_PREFIX", false, "scope");
  KZ_CHECK(r.finding("left out of the order of the reads of the default scope"));
  KZ_CHECK(r.result.teardown_started && r.result.shutdown_requested);
  // without the profile's scope the same delete may precede the read: the read stays open
  const Profile noscope = load(std::string(kProv) + kKinds + rspec("unspecified_by_reference;"));
  const Run n = replay(noscope, std::string(kHeader) + body);
  KZ_CHECK(n.occ(0).outcome_narrowed);
  const Run noff = replay(noscope, std::string(kHeader) + body, false);
  KZ_CHECK(noff.occ(0).status == OccStatus::Undetermined);
}

void test_exit_monitor() {
  const Profile p = load(std::string(kProv) + kKinds + kScope + rspec("{ exit QUIT_ERR \"user doc\" prov doc_call; }"));
  const std::string fail = std::string(kStart) + lookup("-17", nullptr);
  const Run acc = replay(p, std::string(kHeader) + fail + "A quit call arg.status=3\n" + create("0x30002"));
  expect(acc, 0, OccStatus::Tolerated, "NO_VIOLATION_IN_CHECKED_PREFIX", true, "exit ACCEPT");
  KZ_CHECK(acc.occ(0).accepted && has(acc.occ(0).monitor, "exit ACCEPT at #"));
  KZ_CHECK(has(acc.dump, "monitor exit ACCEPT"));
  // no creation anywhere in the record: the only write is the clearing, so the not-ready state is computed (not narrowed)
  const Run rej = replay(p, std::string(kHeader) + fail + "A quit call arg.status=4\n");
  expect(rej, 0, OccStatus::Violating, "OBSERVED_VIOLATION", false, "exit REJECT");
  KZ_CHECK(has(rej.occ(0).monitor, "is another exit"));
  const Run open = replay(p, std::string(kHeader) + fail + create("0x30002"));
  expect(open, 0, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", true, "exit OPEN");
  KZ_CHECK(has(open.occ(0).monitor, "the trace ends before the actor reaches an exit"));
  const Run nocode = replay(p, std::string(kHeader) + fail + "A quit call\n");
  KZ_CHECK(nocode.occ(0).status == OccStatus::Undetermined);
  KZ_CHECK(has(nocode.occ(0).monitor, "carries no status"));
  const Run term = replay(p, std::string(kHeader) + fail + "A halt call\n");
  expect(term, 0, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", false, "process-terminal");
  KZ_CHECK(has(term.occ(0).monitor, "process-terminal step"));
  const Run down = replay(p, std::string(kHeader) + fail + "M run call\nM run.down pt\nA quit call arg.status=3\n");
  expect(down, 0, OccStatus::Pending, "INCONCLUSIVE(horizon-open)", false, "teardown first");
  KZ_CHECK(has(down.occ(0).monitor, "teardown decides no default-scope occurrence"));
  // the P1 alternative: with the creation after the lookup (not ordered with it) the not-ready attempt is not decided
  // and the monitor decides nothing; with no creation in the record the computed state decides it in both readings
  const Run p1 = replay(p, std::string(kHeader) + fail + "A quit call arg.status=3\n" + create("0x30002"), false);
  KZ_CHECK(p1.occ(0).status == OccStatus::Undetermined && !p1.occ(0).accepted);
  const Run p1s = replay(p, std::string(kHeader) + fail + "A quit call arg.status=3\n", false);
  expect(p1s, 0, OccStatus::Tolerated, "NO_VIOLATION_IN_CHECKED_PREFIX", false, "exit ACCEPT on the computed state");
  // a model trace is judged by the same monitor
  const Run model = replay(p, std::string(kStart) + lookup("-17", nullptr) + "A quit call arg.status=3\n");
  KZ_CHECK(model.occ(0).status == OccStatus::Tolerated);
}

}  // namespace

int main() {
  test_language();
  test_reads_from_state();
  test_scope_teardown();
  test_exit_monitor();
  return kztest::finish("test_core_state_order");
}

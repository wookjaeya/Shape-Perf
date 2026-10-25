// kz predict on a small model (lib/predict; SPEC 4.9 rev 1.3, RV 5.3 / 11 stage D).  A small profile and model
// written here: M clears the table and creates A (the user: one lookup; on failure the exit) and B (the provider: one
// creation).  The record is the order "B creates, then A's lookup succeeds" with its reads-from edge.  Every value is
// a unit-test value, not a research condition.
//
//   - the model is anchored in the record (it reproduces the record's cone steps under the record's edges), and a
//     record whose success has no creation, or whose key another actor uses, is not;
//   - the prediction keeps the spawn edges as constraints and the reads-from edge as a variable: the failing order
//     (A's lookup before B's creation: NOT_READY_UNJUDGED) is found, its result computed from the state;
//   - fixing the reads-from edge as a constraint removes that failing order (the error RV 5.3 warns about);
//   - dropping the edge while keeping the record's result (success) yields an order that the replay rejects
//     (a success before the creation disagrees with the state): no infeasible candidate is accepted;
//   - the hold plan of the failing order holds B at its creation and releases it after A's failed lookup, and the plan check
//     shows that every completed execution under it is that order (a stall when the release comes first);
//   - resolution refuses an unknown citation, a key not shorter than the mapping's length bound, a use whose call
//     context has no role; the state cap is reported.

#include <string>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/predict/predict.hpp"
#include "support/kz_test.hpp"

namespace {

using kz::predict::Model;
using kz::predict::PredictInput;
using kz::predict::PredictReport;
using kz::predict::Witness;

const char* const kProfile = R"KZ(phi 1;
profile "predict-mini" revision "unit test";
source main root "" rev "abcdef0";
prov doc_sup { tier contract-backed; kind api_doc; side supplier; file "inc/sem.h"; lines 1 2;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
prov code_a { tier code-derived; kind code_pattern; side supplier; file "src/sem.c"; lines 5;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
const OK = 0 prov code_a;
const TAKEN = -15 prov doc_sup;
const NOT_FOUND = -17 prov doc_sup;
const QUIT_ERR = 3 prov code_a;
const MAXN = 20 prov code_a;
const KEY0 = "S0" prov code_a;
const LONGKEY = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" prov code_a;

kind sem(name: Str) { states ABSENT < LIVE; initial ABSENT; scope process; prov code_a; }

op lookup(out id, key name: Str) {
  bind symbol "lookup";
  conjunct exists: state(sem(arg.name)) = LIVE lp at fn("lookup").call("find") prov code_a;
  outcome OK: ready narrow { api "lookup"; source main rev "abcdef0"; branch "found"; context any; captured name text length < MAXN; holds_at return; prov code_a; };
  outcome NOT_FOUND: not_ready(exists) documented "not found" prov doc_sup
    narrow { api "lookup"; source main rev "abcdef0"; branch "none found"; context any; captured name text length < MAXN; holds_at return; prov code_a; };
  prov code_a;
}
op create(out id, name: Str) {
  bind symbol "create";
  effect publish: lifecycle sem(arg.name) -> LIVE filter lp.status = OK lp at fn("create").call("finalize") values status: Int prov code_a;
  outcome OK: ready;
  outcome TAKEN: error documented "the name is taken" prov doc_sup;
  prov code_a;
}
op wipe() { bind symbol "wipe"; effect clear: lifecycle all sem -> ABSENT lp at fn("wipe").memset(Rec_t) prov code_a; prov code_a; }
op spawn(out tid) { bind symbol "spawn"; prov code_a; }
op entry(tid) { bind symbol "entry"; prov code_a; }
op quit(status) { bind symbol "quit"; terminal; prov code_a; }

site user_call = fn("user_main").call("lookup") prov code_a;
site other_call = fn("other_main").call("lookup") prov code_a;
site prov_call = fn("prov_main").call("create") prov code_a;
role user-dependent { op lookup; site user_call; purpose dependent_use; prov code_a; }

hb PO { title "program order"; stratum S; robust unconditional; template program_order { looper no; } prov code_a; }
hb SP { title "spawn"; stratum S; robust unconditional; template spawn { create spawn out tid; entry entry; } prov code_a; }
hb RF {
  title "reads-from";
  stratum O;
  robust obs_soft;
  template reads_from { use lookup.exists; outcome OK; publish create.publish; match id; }
  prov code_a;
}

rspec K-LOOKUP {
  use op lookup;
  key sem(arg.name);
  requires state(sem(arg.name)) = LIVE;
  oracle fns "find";
  appreq unspecified_by_reference;
  allowed unspecified_by_reference;
  harm app_error_exit;
  authored "unit test";
  prov code_a;
}
)KZ";

const char* const kModel = R"KZ(kzp-predict 1
resource s0 kind sem key KEY0
actor M start prov code_a
actor A prov code_a
actor B prov code_a
program M
  clear wipe.clear sem ABSENT prov code_a
  spawn spawn A prov code_a
  spawn spawn B prov code_a
  finish
end
program A
  use lookup s0 role-site user_call prov code_a
    hit lookup.entry before caller a.so name name
    hit user_call after
  unless-result OK fail prov code_a
  mark "ready"
  finish
  label fail
  exit quit QUIT_ERR prov code_a
    hit quit.entry before caller a.so
end
program B
  create create s0 effect publish role-site prov_call taken TAKEN prov code_a
    hit create.entry before caller b.so name name
    hit prov_call after
  finish
end
)KZ";

// The record: B creates S0, then A's lookup finds it (the reads-from edge B -> A).
const char* const kHead =
    "record real\nstart_evidence init\nadequacy kind sem writers=covered aliases=covered lifetime=known\n"
    "observed lookup.exists\nobserved create.publish\nobserved wipe.clear\nconst MAXN 20\n"
    "M wipe.clear pt\n"
    "M spawn call\nM spawn ret out.tid=1 result=0\nA entry call arg.tid=1\n"
    "M spawn call\nM spawn ret out.tid=2 result=0\nB entry call arg.tid=2\n";
const char* const kCreate = "B create call arg.name=@S0\nB create.publish pt lp.status=0\nB create ret out.id=0x30002 result=0\nB prov_call pt\n";
const char* const kLookupOk = "A lookup call arg.name=@S0\nA lookup.exists pt arg.name=@S0\nA lookup ret out.id=0x30002 result=0\nA user_call pt\n";

kz::phi::Profile load_profile() {
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  std::vector<kz::phi::SourceText> files{{"predict_mini.kzp", kProfile}};
  const bool ok = kz::phi::load_profile(files, &p, &diags).ok();
  KZ_CHECK(ok);
  if (!ok)
    for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
  return p;
}

bool load_model(const kz::phi::Profile& p, const std::string& text, Model* m, std::vector<std::string>* errors) {
  if (!kz::predict::parse_model("mini.kzm", text, m, errors).ok()) return false;
  return kz::predict::resolve_model(p, {{"MAXN", 20}}, m, errors).ok();
}

struct Rec {
  kz::eval::Trace trace;
  kz::eval::Result result;
};

Rec record(const kz::phi::Profile& p, const std::string& text) {
  Rec r;
  const kz::Status s = kz::eval::parse_trace_text(text, &r.trace);
  KZ_CHECK(s.ok());
  if (!s.ok()) std::fprintf(stderr, "record: %s\n", s.describe().c_str());
  KZ_CHECK(kz::eval::replay(p, r.trace, kz::eval::Options(), &r.result).ok());
  return r;
}

PredictReport run(const kz::phi::Profile& p, const Model& m, const Rec& r, kz::predict::EdgeMode e, kz::predict::OutcomeMode o, uint64_t cap = 100000) {
  PredictInput in;
  in.profile = &p;
  in.model = &m;
  in.record = &r.trace;
  in.record_result = &r.result;
  in.ref_consts = {{"MAXN", 20}};
  in.max_states = cap;
  in.edges = e;
  in.outcomes = o;
  PredictReport rep;
  KZ_CHECK(kz::predict::predict(in, &rep).ok());
  return rep;
}

// The candidates whose occurrence of A has this status (accepted ones only unless all).
size_t with_status(const PredictReport& r, kz::eval::OccStatus st, bool accepted_only = true) {
  size_t n = 0;
  for (const Witness& w : r.witnesses) {
    if (accepted_only && !w.accepted) continue;
    for (const kz::predict::Judged& j : w.judged)
      if (j.actor == "A" && j.status == st) ++n;
  }
  return n;
}

bool has(const std::string& s, const std::string& f) { return s.find(f) != std::string::npos; }

void test_anchor_and_prediction() {
  const kz::phi::Profile p = load_profile();
  Model m;
  std::vector<std::string> errors;
  KZ_CHECK(load_model(p, kModel, &m, &errors));
  for (const std::string& e : errors) std::fprintf(stderr, "%s\n", e.c_str());
  const Rec r = record(p, std::string(kHead) + kCreate + kLookupOk);
  // the record's own judgement: READY, and the reads-from edge exists
  KZ_CHECK(r.result.occurrences.size() == 1 && r.result.occurrences[0].status == kz::eval::OccStatus::Ready);
  bool rf = false;
  for (const kz::eval::HbEdge& e : r.result.edges) rf = rf || e.rule == "RF";
  KZ_CHECK(rf);

  const PredictReport v = run(p, m, r, kz::predict::EdgeMode::Variable, kz::predict::OutcomeMode::Rederive);
  KZ_CHECK(v.anchored);
  // the spawn edges are constraints, the reads-from edge and the outcome order are variables
  size_t constraints = 0, variables = 0;
  for (const kz::predict::EdgeInfo& e : v.edges) (e.constraint ? constraints : variables) += 1;
  KZ_CHECK(constraints == 2);
  KZ_CHECK(variables == 1);
  for (const kz::predict::EdgeInfo& e : v.edges)
    if (!e.constraint) KZ_CHECK(e.rule == "RF");
  KZ_CHECK(!v.search.cap_reached);
  KZ_CHECK(v.rejected == 0);
  // both orders: the observed READY and the predicted NOT_READY_UNJUDGED (the failing order is not removed)
  KZ_CHECK(with_status(v, kz::eval::OccStatus::Ready) == 1);
  KZ_CHECK(with_status(v, kz::eval::OccStatus::NotReadyUnjudged) == 1);
  const Witness* bad = nullptr;
  for (const Witness& w : v.witnesses)
    if (!w.observed) bad = &w;
  KZ_CHECK(bad != nullptr);
  if (bad != nullptr) {
    KZ_CHECK(bad->accepted && bad->in_scope);
    KZ_CHECK(has(bad->cls.key, "A/s0: -17x1 | uses before creation 1"));
    KZ_CHECK(has(bad->cls.key, "A=x"));
    // the reads-from of the record is not kept in this order
    bool rf_not_kept = false;
    for (const std::string& o : bad->orders) rf_not_kept = rf_not_kept || (has(o, "not kept") && has(o, "RF"));
    KZ_CHECK(rf_not_kept);
    // its plan: hold B at its creation, release at A's exit; every completed execution under it is this order
    KZ_CHECK(!bad->plans.empty());
    if (!bad->plans.empty()) {
      const kz::predict::PlanResult& pr = bad->plans[0];
      KZ_CHECK(pr.plan.points.size() == 2);
      bool gate_b = false, release_a = false;
      for (const kz::predict::ControlPoint& cp : pr.plan.points) {
        gate_b = gate_b || (!cp.release && cp.site == "create.entry" && cp.caller == "b.so" && cp.name_text == "S0");
        // the first site reached only after A's failed lookup: the after-call point of the lookup
        release_a = release_a || (cp.release && cp.site == "user_call" && cp.actor == 1);
      }
      KZ_CHECK(gate_b && release_a);
      KZ_CHECK(pr.checked && pr.forces && !pr.never);
      KZ_CHECK(pr.stall);  // A may exit before B reaches its gate: B is then held for good
      PredictInput in;
      in.profile = &p;
      in.model = &m;
      const std::string text = kz::predict::plan_text(in, *bad, 0, "unit record");
      KZ_CHECK(has(text, "kzp-hold 1\ngate G1_B site=create.entry when name name \"S0\" when caller-in b.so occurrence first\n"));
      KZ_CHECK(has(text, "release R1 site=user_call releases G1_B occurrence first\n"));
    }
  }
  const std::string rt = kz::predict::report_text(PredictInput{&p, &m, &r.trace, &r.result, {{"MAXN", 20}}, kz::eval::Options(), 100000}, v);
  KZ_CHECK(has(rt, "VARIABLE (re-derived per order) RF"));
  KZ_CHECK(has(rt, "CONSTRAINT (kept in every order) SP"));

  // fixing the observed-run-dependent orders as constraints loses the failing order
  const PredictReport f = run(p, m, r, kz::predict::EdgeMode::Fixed, kz::predict::OutcomeMode::Rederive);
  KZ_CHECK(f.anchored);
  KZ_CHECK(with_status(f, kz::eval::OccStatus::NotReadyUnjudged) == 0);
  KZ_CHECK(with_status(f, kz::eval::OccStatus::Ready) == 1);

  // dropping them while keeping the record's result: the order "lookup before creation" with the success kept is
  // generated and rejected by the replay; no accepted candidate has a success before the creation
  const PredictReport k = run(p, m, r, kz::predict::EdgeMode::Variable, kz::predict::OutcomeMode::Keep);
  KZ_CHECK(k.anchored);
  KZ_CHECK(k.rejected >= 1);
  bool rejected_has_reason = false;
  for (const Witness& w : k.witnesses) {
    if (!w.accepted) rejected_has_reason = rejected_has_reason || has(w.reject, "disagrees with the state");
    if (w.accepted) KZ_CHECK(!has(w.cls.key, "uses before creation 1"));
  }
  KZ_CHECK(rejected_has_reason);
  KZ_CHECK(with_status(k, kz::eval::OccStatus::NotReadyUnjudged) == 0);

  // a state cap is reported and leaves the search incomplete
  const PredictReport c = run(p, m, r, kz::predict::EdgeMode::Variable, kz::predict::OutcomeMode::Rederive, 3);
  KZ_CHECK(!c.anchored || c.search.cap_reached);
}

void test_anchor_refusals() {
  const kz::phi::Profile p = load_profile();
  Model m;
  std::vector<std::string> errors;
  KZ_CHECK(load_model(p, kModel, &m, &errors));
  // a success without any creation in the record: the model cannot reproduce it
  const Rec r1 = record(p, std::string(kHead) + kLookupOk);
  const PredictReport a = run(p, m, r1, kz::predict::EdgeMode::Variable, kz::predict::OutcomeMode::Rederive);
  KZ_CHECK(!a.anchored);
  KZ_CHECK(has(a.anchor_note, "does not reproduce"));
  KZ_CHECK(a.witnesses.empty());
  // an actor outside the model uses the key of the cone: the cone is not closed for this record
  const Rec r2 = record(p, std::string(kHead) + kCreate + kLookupOk + "C lookup call arg.name=@S0\nC lookup.exists pt arg.name=@S0\nC lookup ret out.id=0x30002 result=0\n");
  const PredictReport b = run(p, m, r2, kz::predict::EdgeMode::Variable, kz::predict::OutcomeMode::Rederive);
  KZ_CHECK(!b.anchored);
  KZ_CHECK(has(b.anchor_note, "the cone is not closed"));
}

void test_resolution() {
  const kz::phi::Profile p = load_profile();
  auto refused = [&](const std::string& text, const std::string& fragment) {
    Model m;
    std::vector<std::string> errors;
    const bool ok = load_model(p, text, &m, &errors);
    bool found = false;
    for (const std::string& e : errors) found = found || has(e, fragment);
    KZ_CHECK(!ok && found);
    if (ok || !found)
      for (const std::string& e : errors) std::fprintf(stderr, "  %s\n", e.c_str());
  };
  std::string t = kModel;
  // an unknown citation
  refused(std::string(t).replace(t.find("exit quit QUIT_ERR prov code_a"), 30, "exit quit QUIT_ERR prov nowhere"), "prov nowhere");
  // a step of the code without its citation
  refused(std::string(t).replace(t.find("unless-result OK fail prov code_a"), 33, "unless-result OK fail"), "needs its citation");
  // a key not shorter than the mapping's length bound: the code cannot be computed from the state
  refused(std::string(t).replace(t.find("key KEY0"), 8, "key LONGKEY"), "is not shorter than MAXN");
  // a use at a site that is no role's site: the call context is not stated
  refused(std::string(t).replace(t.find("role-site user_call"), 19, "role-site other_call"), "not the site of a role");
  // a create with a code that is not an outcome of the operation
  refused(std::string(t).replace(t.find("taken TAKEN"), 11, "taken NOT_FOUND"), "is not an outcome of create");
  // the parser: an instruction outside the grammar, a hit before any step
  refused("kzp-predict 1\nactor M start prov code_a\nprogram M\n  jump x\nend\n", "unknown instruction");
  refused("kzp-predict 1\nactor M start prov code_a\nprogram M\n  hit x before\n  finish\nend\n", "hit: must follow");
}

}  // namespace

int main() {
  test_anchor_and_prediction();
  test_anchor_refusals();
  test_resolution();
  return kztest::finish("test_predict_model");
}

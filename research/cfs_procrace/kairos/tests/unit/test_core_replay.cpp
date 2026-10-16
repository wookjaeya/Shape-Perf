// The replay of a trace with a profile (include/kz/core/eval_trace.hpp), on the mini profile of
// tests/unit/support/phi_mini.hpp: the effects of every kind, the happens-before rules of the four
// templates, the statuses of a readiness specification, the outcome check, the scope and the
// process-terminal steps, the guards, and the text form of a trace.  The golden test of the real
// profile (tests/phi) covers the same engine with the rules of the V1 subset.

#include <string>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::eval::HbEdge;
using kz::eval::OccStatus;
using kz::eval::Options;
using kz::eval::Phase;
using kz::eval::Result;
using kz::eval::Trace;
using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::SourceText;

const char* const kPredicate = "forall r: slot where state(r) != FREE { state(stage(r)) >= TWO }";

Profile load(const std::vector<SourceText>& files) {
  Profile p;
  std::vector<Diag> diags;
  if (!kz::phi::load_profile(files, &p, &diags).ok()) {
    for (const Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
  }
  return p;
}

Profile load_edited(const char* from, const char* to, int file = 2) {
  std::vector<SourceText> f = kzmini::files();
  std::string& t = f[static_cast<size_t>(file)].text;
  const size_t at = t.find(from);
  if (at == std::string::npos) {
    std::fprintf(stderr, "bad edit: %s\n", from);
    ++kztest::state().failures;
  } else {
    t.replace(at, std::string(from).size(), to);
  }
  return load(f);
}

struct Run {
  Trace trace;
  Result result;
  kz::Status status;
  std::string dump;

  // The id of the n-th event (from 1) of an actor at a site and phase; -1 if there is none.
  int64_t id(const std::string& actor, const std::string& site, Phase ph, int nth = 1) const {
    int seen = 0;
    for (const kz::eval::TraceEvent& e : trace.events)
      if (e.actor == actor && e.site == site && e.phase == ph && ++seen == nth) return static_cast<int64_t>(e.id);
    return -1;
  }
  bool edge(int64_t from, int64_t to, const std::string& rule) const {
    for (const HbEdge& e : result.edges)
      if (static_cast<int64_t>(e.from) == from && static_cast<int64_t>(e.to) == to && e.rule == rule) return true;
    return false;
  }
  size_t edges_of(const std::string& rule) const {
    size_t n = 0;
    for (const HbEdge& e : result.edges)
      if (e.rule == rule) ++n;
    return n;
  }
  bool finding(const std::string& fragment) const {
    for (const kz::eval::ReplayFinding& f : result.findings)
      if (f.what.find(fragment) != std::string::npos) return true;
    return false;
  }
};

Run replay(const Profile& p, const std::string& text, const Options& options = Options()) {
  Run r;
  const kz::Status ps = kz::eval::parse_trace_text(text, &r.trace);
  if (!ps.ok()) {
    std::fprintf(stderr, "trace does not parse: %s\n", ps.describe().c_str());
    ++kztest::state().failures;
  }
  r.status = kz::eval::replay(p, r.trace, options, &r.result);
  r.dump = kz::eval::dump_result(p, r.trace, r.result);
  return r;
}

std::string show_findings(const Run& r) {
  std::string out;
  for (const kz::eval::ReplayFinding& f : r.result.findings) out += "  " + f.what + "\n";
  return out;
}

// The state of a resource in the final state, as a name; "-" if the replay never set it.
std::string state_name(const Profile& p, const Run& r, const char* kind, int64_t key) {
  const int32_t k = p.find_kind(kind);
  const auto it = r.result.final_state.state.find(kz::eval::ResId{k, {key}});
  if (it == r.result.final_state.state.end()) return "-";
  return p.kinds[static_cast<size_t>(k)].states[static_cast<size_t>(it->second)];
}
int64_t id_of(const Profile& p, const Run& r, int64_t key) {
  const auto it = r.result.final_state.ids.find(kz::eval::ResId{p.find_kind("slot"), {key}});
  return it == r.result.final_state.ids.end() ? -1 : it->second;
}
std::string tag_of(const Profile& p, const Run& r, int64_t key) {
  const int32_t k = p.find_kind("slot");
  const auto it = r.result.final_state.tags.find({kz::eval::ResId{k, {key}}, 0});
  if (it == r.result.final_state.tags.end()) return "-";
  return p.kinds[static_cast<size_t>(k)].tags[0].values[static_cast<size_t>(it->second)];
}
int64_t loc_value(const Profile& p, const Run& r, const char* loc) {
  const auto it = r.result.final_state.loc_values.find(p.find_loc(loc));
  return it == r.result.final_state.loc_values.end() ? -1 : it->second;
}

}  // namespace

int main() {
  const Profile p = load(kzmini::files());
  KZ_CHECK(p.checked);
  Options with_version;
  with_version.versions["main"] = "2.0.1";

  // ---- the effects of every kind -------------------------------------------------------------------
  {
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
owner X 2
owner Z 3
M reset_all.zero_slots pt
M reset_all.zero_stages pt
M create_all.reserve pt lp.n=1
M create_all.typeit pt lp.n=1
M create_all.publish pt lp.n=1 lp.pid=0x1001
M create_all.reserve pt lp.n=2
M create_all.typeext pt lp.n=2
M create_all.publish pt lp.n=2 lp.pid=0x1002
M create_all.reserve pt lp.n=3
M create_all.publish pt lp.n=3 lp.pid=0x1003
M clear_site pt lp.n=3
W advance call arg.Wanted=0
W advance.raise pt lp.level=2
W advance.raise pt lp.level=1
W advance.raise pt lp.level=3
W advance ret result=0
X finish call arg.Code=0
X finish.to_two pt
X finish ret
W finish call
W finish.to_two pt
W finish ret
Z finish call
Z finish.to_two pt
Z finish ret
M scan.wait_one pt lp.n=2
M scan.wait_one pt lp.n=1
M free_up.free pt lp.n=1
W sign_in call arg.Filters=0
W sign_in.clear_reg pt
W sign_in.set_reg pt
W sign_in ret result=0
W get_id call
W get_id.alive pt
W get_id.store_ok pt out.IdPtr=@Box.id value=0x1001
W get_id ret result=0
W send.who_valid pt arg.Who=0x1001 cell=@Box.id value=0x1001
W send call arg.Who=0x1001
W send.who_reg pt
W send ret result=0
M free_up.free pt lp.n=2
Z get_id call
Z get_id.alive pt
Z get_id.store_bad pt out.IdPtr=@Box.id value=0
Z get_id ret result=0xc0000001
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    // Four points execute while their asserted guard (`when`) is false in the replay: W's finish (tag
    // CORE), Z's finish (its slot is not LIVE), the scan of slot 1 (stage not TWO) and the free of slot 1
    // (not EXTERNAL).  Each is a finding and a guard failure; the last one is a refutation of the
    // declared-trusted guard of free_up.free and names its obligations.  None of them is applied.
    if (r.result.findings.size() != 4) std::fprintf(stderr, "findings:\n%s", show_findings(r).c_str());
    KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(4));
    KZ_CHECK_EQ(r.result.guard_failures.size(), static_cast<size_t>(4));
    if (r.result.guard_failures.size() == 4) {
      const std::vector<kz::eval::GuardFailure>& g = r.result.guard_failures;
      KZ_CHECK_EQ(g[0].op + "." + g[0].effect, std::string("finish.to_two"));
      KZ_CHECK_EQ(g[1].op + "." + g[1].effect, std::string("finish.to_two"));
      KZ_CHECK_EQ(g[2].op + "." + g[2].effect, std::string("scan.wait_one"));
      KZ_CHECK_EQ(g[3].op + "." + g[3].effect, std::string("free_up.free"));
      KZ_CHECK(!g[0].trusted && !g[1].trusted && !g[2].trusted);
      KZ_CHECK(g[0].refutable_by.empty());
      KZ_CHECK(g[3].trusted);
      KZ_CHECK_EQ(g[3].refutable_by.size(), static_cast<size_t>(2));
      if (g[3].refutable_by.size() == 2) {
        KZ_CHECK_EQ(g[3].refutable_by[0], std::string("O3"));
        KZ_CHECK_EQ(g[3].refutable_by[1], std::string("O4"));
      }
      KZ_CHECK(g[0].event < g[1].event && g[1].event < g[2].event && g[2].event < g[3].event);
    }
    KZ_CHECK(r.dump.find("refuted refutable_by O3 O4") != std::string::npos);
    KZ_CHECK(r.dump.find("guard failures 4") != std::string::npos);
    // `lifecycle all` set every instance; later effects changed single ones
    KZ_CHECK_EQ(state_name(p, r, "slot", 0), std::string("FREE"));
    KZ_CHECK_EQ(state_name(p, r, "slot", 1), std::string("LIVE"));
    KZ_CHECK_EQ(id_of(p, r, 1), static_cast<int64_t>(0x1001));
    KZ_CHECK_EQ(tag_of(p, r, 1), std::string("CORE"));
    // slot 2: published, then freed by free_up (external, stage above TWO): FREE with the undefined id; its tag stays
    KZ_CHECK_EQ(state_name(p, r, "slot", 2), std::string("FREE"));
    KZ_CHECK_EQ(id_of(p, r, 2), static_cast<int64_t>(0));
    KZ_CHECK_EQ(tag_of(p, r, 2), std::string("EXTERNAL"));
    // slot 3: reserved, published, then cleared at the shared site
    KZ_CHECK_EQ(state_name(p, r, "slot", 3), std::string("FREE"));
    KZ_CHECK_EQ(id_of(p, r, 3), static_cast<int64_t>(0));
    KZ_CHECK_EQ(tag_of(p, r, 3), std::string("-"));
    // the stage: a raise never lowers (2, 1, 3 gives THREE); the conditional effects follow their guards
    KZ_CHECK_EQ(state_name(p, r, "stage", 1), std::string("THREE"));  // W: raised; its finish is guarded off by tag CORE
    KZ_CHECK_EQ(state_name(p, r, "stage", 2), std::string("THREE"));  // X: finish -> TWO, then scan -> THREE
    KZ_CHECK_EQ(state_name(p, r, "stage", 3), std::string("ZERO"));   // Z: its slot is not LIVE: finish did nothing
    KZ_CHECK_EQ(state_name(p, r, "stage", 0), std::string("ZERO"));
    // the registry entry that sign_in made, under the id of the owner
    const auto reg = r.result.final_state.state.find(kz::eval::ResId{p.find_kind("reg"), {0x1001}});
    KZ_CHECK(reg != r.result.final_state.state.end());
    if (reg != r.result.final_state.state.end()) KZ_CHECK_EQ(p.kinds[static_cast<size_t>(p.find_kind("reg"))].states[static_cast<size_t>(reg->second)], std::string("REG"));
    // the out-pointer stores: ok for the live task, the undefined id for the other
    KZ_CHECK_EQ(loc_value(p, r, "Box.id"), static_cast<int64_t>(0));
    // the occurrence of the request is READY, and the capture agreed with the cell the store wrote
    KZ_CHECK_EQ(r.result.occurrences.size(), static_cast<size_t>(1));
    if (!r.result.occurrences.empty()) {
      KZ_CHECK_EQ(static_cast<int>(r.result.occurrences[0].status), static_cast<int>(OccStatus::Ready));
      KZ_CHECK_EQ(static_cast<int>(r.result.occurrences[0].outcome), static_cast<int>(kz::eval::OutcomeCheck::Agrees));
    }
    // the second task's get_id: the conjunct fails and the outcome says not ready because of it
    KZ_CHECK(r.dump.find("finding") != std::string::npos);
  }
  {  // a filter guard (`filter`) at a point both branches reach: a false guard selects the other branch
    // and is not a finding; an asserted guard (`when`) that holds is applied as usual
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
M create_all.reserve pt lp.n=1
M create_all.publish pt lp.n=1 lp.pid=0x1001
W get_id call
W get_id.alive pt
W get_id.store_ok pt out.IdPtr=@Box.id value=0x1001
W get_id.store_bad pt out.IdPtr=@Box.id
W get_id ret result=0
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    if (!r.result.findings.empty()) std::fprintf(stderr, "findings:\n%s", show_findings(r).c_str());
    KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(0));
    KZ_CHECK_EQ(r.result.guard_failures.size(), static_cast<size_t>(0));
    KZ_CHECK_EQ(loc_value(p, r, "Box.id"), static_cast<int64_t>(0x1001));  // the filtered store did not run
  }
  {  // an asserted guard that is false at a point the trace shows: W is live, so the failure-branch store
     // of a profile that asserted (instead of filtering) its guard is a disagreement, and not applied
    std::vector<kz::phi::SourceText> files = kzmini::files();
    for (kz::phi::SourceText& f : files) {
      const std::string from = "effect store_bad: store *IdPtr := UNDEF filter";
      const size_t at = f.text.find(from);
      if (at != std::string::npos) f.text.replace(at, from.size(), "effect store_bad: store *IdPtr := UNDEF when");
    }
    const Profile asserted = load(files);
    KZ_CHECK(asserted.checked);
    const Run r = replay(asserted, R"(
const TABLE 4
owner W 1
M create_all.reserve pt lp.n=1
M create_all.publish pt lp.n=1 lp.pid=0x1001
W get_id call
W get_id.alive pt
W get_id.store_ok pt out.IdPtr=@Box.id value=0x1001
W get_id.store_bad pt out.IdPtr=@Box.id
W get_id ret result=0
)");
    KZ_CHECK_OK(r.status);
    KZ_CHECK_EQ(r.result.guard_failures.size(), static_cast<size_t>(1));
    if (r.result.guard_failures.size() == 1) {
      KZ_CHECK_EQ(r.result.guard_failures[0].effect, std::string("store_bad"));
      KZ_CHECK(!r.result.guard_failures[0].trusted);
      KZ_CHECK_EQ(r.result.guard_failures[0].event, static_cast<uint64_t>(5));
    }
    KZ_CHECK(r.dump.find("guard_failure #5 effect get_id.store_bad disagreement") != std::string::npos);
    KZ_CHECK_EQ(loc_value(asserted, r, "Box.id"), static_cast<int64_t>(0x1001));
  }
  {  // an owner whose key does not have the width of the owner kind: a finding, and the actor has no owner
    const Run r = replay(p, R"(
const TABLE 4
owner W 1 7
M create_all.reserve pt lp.n=1
M create_all.publish pt lp.n=1 lp.pid=0x1001
W get_id call
W get_id.alive pt
W get_id.store_bad pt out.IdPtr=@Box.id value=0
W get_id ret result=0xc0000001
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    KZ_CHECK(r.dump.find("the owner of actor W has 2 key element(s), kind slot has 1: the actor has no owner in the replay") != std::string::npos);
    // without an owner the task is not live: the failure branch is taken and the outcome agrees
    KZ_CHECK(r.dump.find("returned NOT_READY_A") == std::string::npos);
    KZ_CHECK_EQ(r.result.guard_failures.size(), static_cast<size_t>(0));
  }
  {  // the outcome of get_id for a task that is not live: the conjunct is false and the code agrees
    const Run r = replay(p, R"(
const TABLE 4
owner Z 3
Z get_id call
Z get_id.alive pt
Z get_id.store_bad pt out.IdPtr=@Box.id value=0
Z get_id ret result=0xc0000001
Z get_id call
Z get_id.alive pt
Z get_id ret result=0
)", with_version);
    KZ_CHECK_OK(r.status);
    // the first call agrees; the second returns success although the conjunct is false
    KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(1));
    KZ_CHECK(r.finding("returned OK (ready), and a conjunct of the call is false in the replay"));
  }
  {  // problems of single effects
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
A orphan.lost pt
W advance call arg.Wanted=0
W advance.raise pt lp.level=9
W advance.raise pt
M create_all.reserve pt
M create_all.reserve pt lp.n=1
W advance.raise pt lp.level=2
)");
    KZ_CHECK_OK(r.status);
    KZ_CHECK(r.finding("applies to an undefined resource"));
    KZ_CHECK(r.finding("lacks the value lp.n"));
    // W's owner (slot 1) is RESERVED, not live: the raise is guarded off and no level is looked at
    KZ_CHECK(!r.finding("is not a state of kind stage"));
  }
  {  // a raise to a level that is not a state, for a live task
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
M create_all.publish pt lp.n=1 lp.pid=0x1001
W advance call arg.Wanted=0
W advance.raise pt lp.level=9
W advance.raise pt lp.level=-1
)");
    KZ_CHECK_OK(r.status);
    KZ_CHECK(r.finding("the level 9 is not a state of kind stage"));
    KZ_CHECK(r.finding("the level -1 is not a state of kind stage"));
    KZ_CHECK_EQ(state_name(p, r, "stage", 1), std::string("-"));
  }
  {  // INIT facts are in the state before the first event
    const Run r = replay(p, "const TABLE 4\nA init_box.clear_box pt\n");
    KZ_CHECK_OK(r.status);
    KZ_CHECK_EQ(loc_value(p, r, "Box.id"), static_cast<int64_t>(0));
  }

  // ---- happens-before: program order and the looper ---------------------------------------------------
  {
    const char* trace = R"(
const TABLE 4
A mark_it call
A mark_it.use_it pt lp.n=1
A mark_it.begin pt
A mark_it.sync_it pt
A mark_it.env_it pt
A mark_it ret
B mark_it call
B mark_it ret
)";
    const Run r = replay(p, trace, with_version);
    KZ_CHECK_OK(r.status);
    const int64_t a0 = r.id("A", "mark_it", Phase::Call), a1 = r.id("A", "mark_it.use_it", Phase::Point), a2 = r.id("A", "mark_it.begin", Phase::Point),
                  a3 = r.id("A", "mark_it.sync_it", Phase::Point), a5 = r.id("A", "mark_it", Phase::Return);
    KZ_CHECK(r.edge(a0, a1, "H1"));
    KZ_CHECK(!r.edge(a1, a2, "H1"));  // a new handler begins: between handlers there is no edge on a looper
    KZ_CHECK(r.edge(a2, a3, "H1"));
    KZ_CHECK(r.edge(a3, r.id("A", "mark_it.env_it", Phase::Point), "H1"));
    KZ_CHECK(r.edge(r.id("A", "mark_it.env_it", Phase::Point), a5, "H1"));
    KZ_CHECK(r.edge(r.id("B", "mark_it", Phase::Call), r.id("B", "mark_it", Phase::Return), "H1"));
    KZ_CHECK_EQ(r.edges_of("H1"), static_cast<size_t>(5));  // A: 5 pairs less the one into the new handler; B: 1
    KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(0));
    // the same trace, on a profile whose rule is not for loopers: the edge is there
    const Profile q = load_edited("template program_order { looper yes; }", "template program_order { looper no; }");
    const Run r2 = replay(q, trace, with_version);
    KZ_CHECK(r2.edge(a1, a2, "H1"));
    KZ_CHECK_EQ(r2.edges_of("H1"), static_cast<size_t>(6));
    // edges are sorted by target, then source, then rule; each pair once
    for (size_t i = 1; i < r.result.edges.size(); ++i) KZ_CHECK(r.result.edges[i - 1] < r.result.edges[i]);
  }

  // ---- happens-before: spawn -------------------------------------------------------------------------------
  {
    const Run r = replay(p, R"(
const TABLE 4
P spawn_it call arg.Name=0
C1 child_main call arg.Handle=7
P spawn_it ret out.Handle=7 result=0
P spawn_it call arg.Name=0
P spawn_it ret out.Handle=7 result=0
C2 child_main call arg.Handle=7
C3 child_main call arg.Handle=9
P2 spawn_it call arg.Name=0
P2 spawn_it ret result=0
D mark_it call
D child_main call arg.Handle=1
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    const int64_t c0 = r.id("P", "spawn_it", Phase::Call, 1), c1 = r.id("P", "spawn_it", Phase::Call, 2);
    KZ_CHECK(r.edge(c0, r.id("C1", "child_main", Phase::Call), "H2"));  // the child ran before the creating call returned
    KZ_CHECK(r.edge(c1, r.id("C2", "child_main", Phase::Call), "H2"));  // an id that is used again pairs with the next entry
    KZ_CHECK(!r.edge(c0, r.id("C2", "child_main", Phase::Call), "H2"));
    KZ_CHECK_EQ(r.edges_of("H2"), static_cast<size_t>(2));  // C3 has no creator, D's entry has no creator
    KZ_CHECK(r.finding("lacks out.Handle"));
    KZ_CHECK(r.finding("is not the first event of actor D"));
  }

  // ---- happens-before: phase ------------------------------------------------------------------------------------
  {
    const Run r = replay(p, R"(
const TABLE 4
P phase_begin pt
P spawn_it call arg.Name=0
C child_main call arg.Handle=1
P spawn_it ret out.Handle=1 result=0
P phase_begin pt
Q phase_begin pt
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    const int64_t first = r.id("P", "phase_begin", Phase::Point, 1);
    KZ_CHECK(r.edge(first, r.id("C", "child_main", Phase::Call), "H3"));
    KZ_CHECK_EQ(r.edges_of("H3"), static_cast<size_t>(1));  // not from the point after the spawn, not from another actor
    KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(0));
  }

  // ---- happens-before: observation of a poll -----------------------------------------------------------------------------
  {
    const char* base = R"(
const TABLE 4
owner W 1
owner X 2
M create_all.reserve pt lp.n=1
M create_all.publish pt lp.n=1 lp.pid=0x1001
M create_all.reserve pt lp.n=2
M create_all.publish pt lp.n=2 lp.pid=0x1002
W sign_in.set_reg pt
W advance.raise pt lp.level=2
X advance.raise pt lp.level=@LEVEL@
M wait_poll call arg.Level=2 arg.Timeout=100
M wait_poll ret result=@RESULT@
@TAIL@
)";
    auto make = [&](const char* level, const char* result, const char* tail) {
      std::string t = base;
      t.replace(t.find("@LEVEL@"), 7, level);
      t.replace(t.find("@RESULT@"), 8, result);
      t.replace(t.find("@TAIL@"), 6, tail);
      return t;
    };
    kz::phi::ExprPtr pred;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::eval::resolve_rule_predicate(p, "H4", kPredicate, &pred, &diags));
    Options opt = with_version;
    opt.predicates["H4"] = pred.get();
    {  // the exit with the predicate true: the last writes of the other actors are ordered before it
      const Run r = replay(p, make("3", "0", ""), opt);
      KZ_CHECK_OK(r.status);
      const int64_t ret = r.id("M", "wait_poll", Phase::Return);
      KZ_CHECK(r.edge(r.id("W", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK(r.edge(r.id("X", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK_EQ(r.edges_of("H4"), static_cast<size_t>(2));  // the main actor's own writes and the write to a kind that is not watched give none
      KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(0));
      for (const HbEdge& e : r.result.edges)
        if (e.rule == "H4") KZ_CHECK_EQ(static_cast<int>(e.robust), static_cast<int>(kz::phi::Robust::ObsFatal));
    }
    {  // the poll exits while its predicate is false in the replay: a finding, no edge
      const Run r = replay(p, make("1", "0", ""), opt);
      KZ_CHECK_EQ(r.edges_of("H4"), static_cast<size_t>(0));
      KZ_CHECK(r.finding("awaited predicate is false in the replay"));
    }
    {  // no predicate supplied: the profile says it comes from the code
      const Run r = replay(p, make("3", "0", ""), with_version);
      KZ_CHECK_EQ(r.edges_of("H4"), static_cast<size_t>(0));
      KZ_CHECK(r.finding("comes from the code and none was supplied"));
    }
    {  // the other branch ends in the terminal operation
      const Run r = replay(p, make("3", "0xc0000004", "M panic call arg.Code=1\nM panic.terminal pt"), opt);
      KZ_CHECK_OK(r.status);
      KZ_CHECK_EQ(r.edges_of("H4"), static_cast<size_t>(0));
      KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(0));
      KZ_CHECK(r.result.terminated);
    }
    {  // the other branch goes on: the finding names the call
      const Run r = replay(p, make("3", "0xc0000004", "M mark_it call"), opt);
      KZ_CHECK(r.finding("did not reach its exit outcome, and the next call of the actor is mark_it, not the terminal operation panic"));
      KZ_CHECK(!r.result.terminated);
    }
    {  // the guard of the rule: the version decides
      const Run old = replay(p, make("3", "0", ""), [&] {
        Options o = opt;
        o.versions["main"] = "2.0.0";
        return o;
      }());
      KZ_CHECK_EQ(old.edges_of("H4"), static_cast<size_t>(0));
      KZ_CHECK(old.finding("rule H4 is not applied: the guard main >= 2.0.1 does not hold for version 2.0.0"));
      Options none;
      none.predicates["H4"] = pred.get();
      const Run unknown = replay(p, make("3", "0", ""), none);
      KZ_CHECK_EQ(unknown.edges_of("H4"), static_cast<size_t>(0));
      KZ_CHECK(unknown.finding("rule H4 is not applied: its guard on 'main' cannot be evaluated (no version given)"));
      Options newer = opt;
      newer.versions["main"] = "10.0";
      KZ_CHECK_EQ(replay(p, make("3", "0", ""), newer).edges_of("H4"), static_cast<size_t>(2));
    }
  }

  // ---- happens-before: which writes a poll can have seen -----------------------------------------------------------------
  //
  // The edge of an observation rule goes from a write that the scan which let the poll exit can have
  // seen: the last writes at the moment the awaited predicate became true (at the call, or at the
  // write that flipped it).  A later write may have landed after the scan.
  {
    kz::phi::ExprPtr pred;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::eval::resolve_rule_predicate(p, "H4", kPredicate, &pred, &diags));
    Options opt = with_version;
    opt.predicates["H4"] = pred.get();
    const char* slots = R"(
const TABLE 4
owner W 1
owner X 2
owner Y 1
owner Z 1
M create_all.reserve pt lp.n=1
M create_all.publish pt lp.n=1 lp.pid=0x1001
M create_all.reserve pt lp.n=2
M create_all.publish pt lp.n=2 lp.pid=0x1002
)";
    {  // the predicate becomes true during the call, at X's write; Z's later write is not an edge
      const Run r = replay(p, std::string(slots) + R"(
M wait_poll call arg.Level=2 arg.Timeout=100
W advance.raise pt lp.level=2
X advance.raise pt lp.level=2
Z advance.raise pt lp.level=3
M wait_poll ret result=0
)",
                           opt);
      KZ_CHECK_OK(r.status);
      const int64_t ret = r.id("M", "wait_poll", Phase::Return);
      KZ_CHECK(r.edge(r.id("W", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK(r.edge(r.id("X", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK(!r.edge(r.id("Z", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK_EQ(r.edges_of("H4"), static_cast<size_t>(2));
      KZ_CHECK_EQ(r.result.findings.size(), static_cast<size_t>(0));
      // every edge of an observation rule is read minimally; the structural ones are not
      for (const HbEdge& e : r.result.edges) KZ_CHECK_EQ(e.minimal, e.rule == "H4");
      KZ_CHECK(r.dump.find("edge H4 S obs_fatal minimal #") != std::string::npos);
      KZ_CHECK(r.dump.find("edge H1 S unconditional #") != std::string::npos);
    }
    {  // a raise to the level a resource already has is not a write: it gives no edge and replaces no writer
      const Run r = replay(p, std::string(slots) + R"(
W advance.raise pt lp.level=2
Y advance.raise pt lp.level=2
X advance.raise pt lp.level=2
M wait_poll call arg.Level=2 arg.Timeout=100
M wait_poll ret result=0
)",
                           opt);
      KZ_CHECK_OK(r.status);
      const int64_t ret = r.id("M", "wait_poll", Phase::Return);
      KZ_CHECK(r.edge(r.id("W", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK(!r.edge(r.id("Y", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK_EQ(r.edges_of("H4"), static_cast<size_t>(2));
    }
    {  // the predicate holds at the call: the writes before it are the ones seen, not those during it
      const Run r = replay(p, std::string(slots) + R"(
W advance.raise pt lp.level=2
X advance.raise pt lp.level=2
M wait_poll call arg.Level=2 arg.Timeout=100
Z advance.raise pt lp.level=3
M wait_poll ret result=0
)",
                           opt);
      KZ_CHECK_OK(r.status);
      const int64_t ret = r.id("M", "wait_poll", Phase::Return);
      KZ_CHECK(r.edge(r.id("W", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK(!r.edge(r.id("Z", "advance.raise", Phase::Point), ret, "H4"));
    }
    {  // the predicate turns false again and true again: the moment is the last one
      const Run r = replay(p, std::string(slots) + R"(
W advance.raise pt lp.level=2
X advance.raise pt lp.level=2
M wait_poll call arg.Level=2 arg.Timeout=100
M create_all.reserve pt lp.n=3
M create_all.publish pt lp.n=3 lp.pid=0x1003
owner V 3
V advance.raise pt lp.level=2
M wait_poll ret result=0
)",
                           opt);
      KZ_CHECK_OK(r.status);
      const int64_t ret = r.id("M", "wait_poll", Phase::Return);
      KZ_CHECK(r.edge(r.id("V", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK(r.edge(r.id("W", "advance.raise", Phase::Point), ret, "H4"));
      KZ_CHECK(r.edge(r.id("X", "advance.raise", Phase::Point), ret, "H4"));
    }
    {  // the exit outcome is compared modulo 2^32: an int32 result and its unsigned spelling are one code
      const Profile q = load_edited("exit_outcome OK;", "exit_outcome NOT_READY_A;");
      kz::phi::ExprPtr qpred;
      KZ_CHECK_OK(kz::eval::resolve_rule_predicate(q, "H4", kPredicate, &qpred, &diags));
      Options qopt = with_version;
      qopt.predicates["H4"] = qpred.get();
      const std::string tail = std::string(slots) + "W advance.raise pt lp.level=2\nX advance.raise pt lp.level=2\nM wait_poll call arg.Level=2 arg.Timeout=100\nM wait_poll ret result=";
      for (const char* result : {"-1073741823", "0xc0000001", "3221225473"}) {
        const Run r = replay(q, tail + result + "\n", qopt);
        KZ_CHECK_OK(r.status);
        KZ_CHECK_EQ(r.edges_of("H4"), static_cast<size_t>(2));
      }
      const Run other = replay(q, tail + "0\n", qopt);  // 0 is not the exit outcome any more
      KZ_CHECK_EQ(other.edges_of("H4"), static_cast<size_t>(0));
    }
    {  // a create-call and an entry with the same handle: an entry that comes before the call is not its child
      const Run r = replay(p, R"(
const TABLE 4
C0 child_main call arg.Handle=7
P spawn_it call arg.Name=0
P spawn_it ret out.Handle=7 result=0
)",
                           with_version);
      KZ_CHECK_OK(r.status);
      KZ_CHECK_EQ(r.edges_of("H2"), static_cast<size_t>(0));
    }
  }

  // ---- statuses of an occurrence ---------------------------------------------------------------------------------------------
  {
    const char* bad_id = R"(
const TABLE 4
owner W 1
W send.who_valid pt arg.Who=0xfff
W send call arg.Who=0xfff
W send ret result=0xc0000001
)";
    const struct {
      const char* allowed;
      OccStatus expect;
    } variants[] = {
        {"allowed none;", OccStatus::Violating},
        {"allowed unspecified_by_reference;", OccStatus::NotReadyUnjudged},
        {"allowed { documented_drop \"caller text\" prov doc_call; }", OccStatus::Pending},
        {"allowed { documented_drop \"what a rule found\" prov code_rule; }", OccStatus::Violating},  // a rule never accepts; appreq is specified: none (LP-ALLOW b1r)
        {"allowed { documented_drop \"supplier text\" prov doc_sup; }", OccStatus::Violating},  // supplier-only entries decide nothing: none, appreq specified
    };
    for (const auto& v : variants) {
      const Profile q = load_edited("allowed none;", v.allowed);
      const Run r = replay(q, bad_id);
      KZ_CHECK_OK(r.status);
      KZ_CHECK_EQ(r.result.occurrences.size(), static_cast<size_t>(1));
      if (!r.result.occurrences.empty()) {
        KZ_CHECK_EQ(std::string(kz::eval::occ_status_text(r.result.occurrences[0].status)), std::string(kz::eval::occ_status_text(v.expect)));
        KZ_CHECK(r.result.occurrences[0].status != OccStatus::Tolerated);  // a monitor that does not exist never accepts
      }
    }
    // without a specified appreq, "allowed none" is unspecified_by_reference
    const Profile q = load_edited("appreq doc_call;", "appreq unspecified_by_reference;");
    const Run r = replay(q, bad_id);
    KZ_CHECK_EQ(static_cast<int>(r.result.occurrences[0].status), static_cast<int>(OccStatus::NotReadyUnjudged));
    // a rule-only entry without a specified appreq: unjudged (LP-ALLOW b1r), and never accepted
    std::vector<SourceText> f = kzmini::files();
    std::string& t = f[2].text;
    t.replace(t.find("appreq doc_call;"), 16, "appreq unspecified_by_reference;");
    t.replace(t.find("allowed none;"), 13, "allowed { documented_drop \"what a rule found\" prov code_rule; }");
    const Profile q2 = load(f);
    const Run r2 = replay(q2, bad_id);
    KZ_CHECK_EQ(static_cast<int>(r2.result.occurrences[0].status), static_cast<int>(OccStatus::NotReadyUnjudged));
  }
  {  // two conjuncts false at different events: the earliest decides, here the capture, so at the call
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
W send.who_valid pt arg.Who=0xfff
W send call arg.Who=0xfff
W send.who_reg pt
W send ret result=0xc0000001
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    KZ_CHECK_EQ(r.result.occurrences.size(), static_cast<size_t>(1));
    if (!r.result.occurrences.empty()) {
      const kz::eval::Occurrence& o = r.result.occurrences[0];
      KZ_CHECK_EQ(static_cast<int>(o.status), static_cast<int>(OccStatus::Violating));
      KZ_CHECK_EQ(static_cast<int>(o.decided_at), static_cast<int>(kz::eval::DecidedAt::Call));
      KZ_CHECK_EQ(static_cast<int64_t>(o.decision), r.id("W", "send", Phase::Call));
      KZ_CHECK_EQ(o.conjuncts.size(), static_cast<size_t>(2));
      for (const kz::eval::ConjunctResult& c : o.conjuncts) KZ_CHECK(c.evaluated && !c.holds);
      KZ_CHECK_EQ(static_cast<int>(o.outcome), static_cast<int>(kz::eval::OutcomeCheck::Agrees));  // NOT_READY_A names who_valid
    }
  }
  {  // the value conjunct holds at the call, the registry conjunct fails at its point: decided at the point
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
W send.who_valid pt arg.Who=0x1001
W send call arg.Who=0x1001
W send.who_reg pt
W send ret result=0xc0000002
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    if (!r.result.occurrences.empty()) {
      const kz::eval::Occurrence& o = r.result.occurrences[0];
      KZ_CHECK_EQ(static_cast<int>(o.status), static_cast<int>(OccStatus::Violating));
      KZ_CHECK_EQ(static_cast<int>(o.decided_at), static_cast<int>(kz::eval::DecidedAt::Point));
      KZ_CHECK_EQ(static_cast<int64_t>(o.decision), r.id("W", "send.who_reg", Phase::Point));
      KZ_CHECK_EQ(static_cast<int>(o.outcome), static_cast<int>(kz::eval::OutcomeCheck::Agrees));  // NOT_READY_B names who_reg
    }
  }
  {  // O4(a): success returned over a false conjunct disagrees, on the occurrence and as a finding
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
W send.who_valid pt arg.Who=0xfff
W send call arg.Who=0xfff
W send.who_reg pt
W send ret result=0
)",
                         with_version);
    KZ_CHECK_OK(r.status);
    if (!r.result.occurrences.empty()) KZ_CHECK_EQ(static_cast<int>(r.result.occurrences[0].outcome), static_cast<int>(kz::eval::OutcomeCheck::Disagrees));
    KZ_CHECK(r.finding("returned OK (ready), and a conjunct of the call is false in the replay"));
    // the same code with both conjuncts true agrees
    const Run ok = replay(p, R"(
const TABLE 4
owner W 1
M create_all.publish pt lp.n=1 lp.pid=0x1001
W sign_in call arg.Filters=0
W sign_in.set_reg pt
W sign_in ret result=0
W send.who_valid pt arg.Who=0x1001
W send call arg.Who=0x1001
W send.who_reg pt
W send ret result=0
)",
                          with_version);
    if (!ok.result.occurrences.empty()) KZ_CHECK_EQ(static_cast<int>(ok.result.occurrences[0].outcome), static_cast<int>(kz::eval::OutcomeCheck::Agrees));
  }
  {  // the decision event and the source of each conjunct
    const Run r = replay(p, R"(
const TABLE 4
owner W 1
W send call arg.Who=0x1001
W send.who_reg pt
W send ret result=0xc0000002
)");
    KZ_CHECK_OK(r.status);
    const kz::eval::Occurrence& o = r.result.occurrences[0];
    KZ_CHECK_EQ(std::string(kz::eval::occ_status_text(o.status)), std::string("VIOLATING"));
    KZ_CHECK_EQ(static_cast<int>(o.decided_at), static_cast<int>(kz::eval::DecidedAt::Point));
    KZ_CHECK_EQ(static_cast<int64_t>(o.decision), r.id("W", "send.who_reg", Phase::Point));
    KZ_CHECK_EQ(o.conjuncts.size(), static_cast<size_t>(2));
    // the conjuncts are listed in the order of the (sorted) labels: who_reg, who_valid
    if (o.conjuncts.size() == 2) {
      KZ_CHECK_EQ(o.conjuncts[0].label, std::string("who_reg"));
      KZ_CHECK_EQ(static_cast<int>(o.conjuncts[0].source), static_cast<int>(kz::eval::LpSource::Point));
      KZ_CHECK(!o.conjuncts[0].holds);
      KZ_CHECK_EQ(o.conjuncts[1].label, std::string("who_valid"));
      KZ_CHECK_EQ(static_cast<int>(o.conjuncts[1].source), static_cast<int>(kz::eval::LpSource::Call));  // no capture: read at the call
      KZ_CHECK(o.conjuncts[1].holds);
    }
    KZ_CHECK_EQ(static_cast<int>(o.outcome), static_cast<int>(kz::eval::OutcomeCheck::Agrees));
    KZ_CHECK(o.in_scope);
    // statuses are final: a later pass of the other conjunct does not change a decided occurrence
    const Run r2 = replay(p, R"(
const TABLE 4
W send.who_valid pt arg.Who=0xfff
W send call arg.Who=0xfff
W send.who_reg pt
W send ret result=0xc0000001
)");
    KZ_CHECK_EQ(static_cast<int>(r2.result.occurrences[0].status), static_cast<int>(OccStatus::Violating));
    KZ_CHECK_EQ(static_cast<int>(r2.result.occurrences[0].decided_at), static_cast<int>(kz::eval::DecidedAt::Call));
  }
  {  // an occurrence still open when the trace ends
    const Run r = replay(p, "const TABLE 4\nW send.who_valid pt arg.Who=0x1001\nW send call arg.Who=0x1001\n");
    KZ_CHECK_EQ(r.result.occurrences.size(), static_cast<size_t>(1));
    KZ_CHECK_EQ(std::string(kz::eval::occ_status_text(r.result.occurrences[0].status)), std::string("OPEN"));
    KZ_CHECK(!r.result.occurrences[0].returned);
  }

  // ---- scope and process-terminal steps ------------------------------------------------------------------------------------------
  {
    const Run r = replay(p, R"(
const TABLE 4
W shutdown_req call arg.Kind=1
W shutdown_req.request pt
W send.who_valid pt arg.Who=0
W send call arg.Who=0
W send ret result=0xc0000001
W shutdown_req.terminal pt
W mark_it call
W mark_it ret
)");
    KZ_CHECK_OK(r.status);
    KZ_CHECK(r.result.shutdown_requested);
    KZ_CHECK_EQ(static_cast<int64_t>(r.result.shutdown_event), r.id("W", "shutdown_req.request", Phase::Point));
    KZ_CHECK(r.result.terminated);
    KZ_CHECK_EQ(static_cast<int64_t>(r.result.terminal_event), r.id("W", "shutdown_req.terminal", Phase::Point));
    KZ_CHECK(!r.result.occurrences.empty() && !r.result.occurrences[0].in_scope);
    KZ_CHECK(r.finding("2 event(s) follow the process-terminal step and are not replayed"));
    KZ_CHECK(r.dump.find("scope teardown") != std::string::npos);
  }
  {  // jumping to the teardown state is a request as well; a terminal operation without a pattern ends at its call
    const Run r = replay(p, "const TABLE 4\nW closer.close pt\nW die call\nW mark_it call\n");
    KZ_CHECK(r.result.shutdown_requested);
    KZ_CHECK(r.result.terminated);
    KZ_CHECK_EQ(static_cast<int64_t>(r.result.terminal_event), r.id("W", "die", Phase::Call));
    KZ_CHECK(r.finding("1 event(s) follow the process-terminal step"));
  }
  {  // a terminal step by another actor ends the process for everyone
    const Run r = replay(p, "const TABLE 4\nA mark_it call\nB panic call arg.Code=1\nB panic.terminal pt\nA mark_it ret\n");
    KZ_CHECK(r.result.terminated);
    KZ_CHECK(r.finding("1 event(s) follow the process-terminal step"));
    KZ_CHECK_EQ(r.edges_of("H1"), static_cast<size_t>(1));  // B's call to its terminal point only
  }

  // ---- unusable input -----------------------------------------------------------------------------------------------------------------------
  {
    Profile unchecked;
    Trace t;
    Result res;
    KZ_CHECK_ERR(kz::eval::replay(unchecked, t, Options(), &res), kz::Err::Invalid);
    const Run no_table = replay(p, "A mark_it call\n");  // the table size of the finite kinds is not given
    KZ_CHECK_ERR(no_table.status, kz::Err::Invalid);
    KZ_CHECK(no_table.status.message.find("does not give (const TABLE N)") != std::string::npos);
    const Run too_big = replay(p, "const TABLE 5000\nA mark_it call\n");
    KZ_CHECK_ERR(too_big.status, kz::Err::Invalid);
    KZ_CHECK(too_big.status.message.find("out of range") != std::string::npos);
    const Run negative = replay(p, "const TABLE -1\nA mark_it call\n");
    KZ_CHECK_ERR(negative.status, kz::Err::Invalid);
  }

  // ---- determinism ------------------------------------------------------------------------------------------------------------------------------
  {
    const char* text = "const TABLE 4\nowner W 1\nM create_all.publish pt lp.n=1 lp.pid=0x1001\nW send.who_valid pt arg.Who=0x1001\nW send call arg.Who=0x1001\nW send.who_reg pt\nW send ret result=0xc0000002\n";
    const Run a = replay(p, text), b = replay(p, text);
    KZ_CHECK_EQ(a.dump, b.dump);
    KZ_CHECK(a.dump.find("events 5\n") != std::string::npos);
    KZ_CHECK(a.dump.find("occurrence K-SEND actor W ordinal 1 key [0x1001]") != std::string::npos);
  }

  // ---- the replay is total: random traces never crash, and replaying twice gives the same text -------------------------------------------------------
  {
    struct Rng {
      uint64_t s;
      explicit Rng(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ull + 99) {
        for (int i = 0; i < 4; ++i) next();
      }
      uint64_t next() {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return s * 0x2545F4914F6CDD1Dull;
      }
      uint64_t below(uint64_t n) { return (next() >> 11) % n; }
    } rng(2026);
    // every name a trace can use: operations, the points of the profile, a few junk names
    std::vector<std::string> sites;
    for (const kz::phi::OpDecl& op : p.ops) {
      sites.push_back(op.name);
      for (const kz::phi::ConjunctDecl& c : op.conjuncts) sites.push_back(kz::eval::lp_name(op, c.label, c.lp));
      for (const kz::phi::EffectDecl& e : op.effects) sites.push_back(kz::eval::lp_name(op, e.label, e.lp));
      if (op.terminal.present && op.terminal.after.present) sites.push_back(kz::eval::terminal_name(op));
    }
    for (const kz::phi::SiteDecl& s : p.sites) sites.push_back(s.name);
    sites.push_back("junk");
    sites.push_back("");
    const std::vector<std::string> value_names = {"arg.Who", "arg.Info", "arg.Spec", "arg.Level", "arg.Timeout", "arg.Handle", "arg.Name", "arg.Wanted", "lp.n", "lp.level", "lp.pid", "value", "out.Handle", "out.IdPtr"};
    const std::vector<int64_t> numbers = {0, 1, 2, 3, 4, 5, 7, 0xfff, 0x1000, 0x10ff, 0x1100, 0xffffffffll, 0xc0000001ll, 0xc0000004ll, -1, INT64_MAX, INT64_MIN, 1ll << 40};
    const std::vector<std::string> actors = {"A", "B", "C"};
    long events_total = 0, findings_total = 0, edges_total = 0;
    for (int round = 0; round < 400; ++round) {
      Trace t;
      t.ref_consts["TABLE"] = static_cast<int64_t>(rng.below(7));
      for (const std::string& a : actors)
        if (rng.below(3) != 0) t.owners[a] = {static_cast<int64_t>(rng.below(8))};
      const size_t n = 1 + static_cast<size_t>(rng.below(40));
      for (size_t i = 0; i < n; ++i) {
        kz::eval::TraceEvent e;
        e.actor = actors[rng.below(actors.size())];
        e.site = sites[rng.below(sites.size())];
        e.phase = static_cast<Phase>(rng.below(3));
        const size_t nv = static_cast<size_t>(rng.below(4));
        for (size_t k = 0; k < nv; ++k) e.values[value_names[rng.below(value_names.size())]] = numbers[rng.below(numbers.size())];
        if (rng.below(3) == 0) {
          e.has_result = true;
          e.result = numbers[rng.below(numbers.size())];
        }
        if (rng.below(4) == 0) e.texts["out.IdPtr"] = rng.below(2) == 0 ? "Box.id" : "nowhere";
        if (rng.below(6) == 0) e.texts["cell"] = "Box.id";
        t.add(e);
      }
      Options opt = with_version;
      if (round % 2 == 0) opt.versions.clear();
      Result a, b;
      const kz::Status sa = kz::eval::replay(p, t, opt, &a);
      const kz::Status sb = kz::eval::replay(p, t, opt, &b);
      KZ_CHECK(sa.ok() && sb.ok());
      KZ_CHECK_EQ(kz::eval::dump_result(p, t, a), kz::eval::dump_result(p, t, b));
      events_total += static_cast<long>(t.events.size());
      findings_total += static_cast<long>(a.findings.size());
      edges_total += static_cast<long>(a.edges.size());
      for (size_t i = 1; i < a.edges.size(); ++i) KZ_CHECK(a.edges[i - 1] < a.edges[i]);
      for (const kz::eval::Occurrence& o : a.occurrences) {
        KZ_CHECK(o.decided_at != kz::eval::DecidedAt::None || o.status == OccStatus::Open || o.status == OccStatus::NotEvaluated || !o.returned);
      }
    }
    std::printf("random traces: %ld events, %ld edges, %ld findings\n", events_total, edges_total, findings_total);
    KZ_CHECK(events_total > 5000 && findings_total > 100 && edges_total > 100);  // the traces exercise something
  }

  // ---- the text form of a trace ------------------------------------------------------------------------------------------------------------------
  {
    Trace t;
    KZ_CHECK_OK(kz::eval::parse_trace_text("# a comment\n\nconst TABLE 0x10  # trailing comment\nowner A 3 -4 0x5\nA op call arg.x=-7 arg.y=0xff name=@Some.Where\nA op ret result=0xc2000003 out.z=5\nB site pt\n", &t));
    KZ_CHECK_EQ(t.events.size(), static_cast<size_t>(3));
    KZ_CHECK_EQ(t.ref_consts["TABLE"], static_cast<int64_t>(16));
    KZ_CHECK_EQ(t.owners["A"].size(), static_cast<size_t>(3));
    KZ_CHECK_EQ(t.owners["A"][1], static_cast<int64_t>(-4));
    KZ_CHECK_EQ(t.events[0].values["arg.x"], static_cast<int64_t>(-7));
    KZ_CHECK_EQ(t.events[0].values["arg.y"], static_cast<int64_t>(255));
    KZ_CHECK_EQ(t.events[0].texts["name"], std::string("Some.Where"));
    KZ_CHECK(t.events[1].has_result);
    KZ_CHECK_EQ(t.events[1].result, static_cast<int64_t>(0xc2000003ll));
    KZ_CHECK_EQ(t.events[1].values["out.z"], static_cast<int64_t>(5));
    KZ_CHECK(t.events[2].phase == Phase::Point);
    KZ_CHECK(!t.events[2].has_result);
    KZ_CHECK_EQ(t.events[2].id, static_cast<uint64_t>(2));
    // values above INT64_MAX are read as unsigned
    KZ_CHECK_OK(kz::eval::parse_trace_text("A op call arg.big=0xffffffffffffffff\n", &t));
    KZ_CHECK_EQ(t.events[0].values["arg.big"], static_cast<int64_t>(-1));
  }
  {
    const struct {
      const char* text;
      const char* message;
    } bad[] = {
        {"A\n", "trace line 1: expected: ACTOR SITE call|ret|pt"},
        {"A site\n", "trace line 1: expected: ACTOR SITE call|ret|pt"},
        {"\n\nA site jump\n", "trace line 3: 'jump' is not call, ret or pt"},
        {"A site call x\n", "trace line 1: expected name=value, found 'x'"},
        {"A site call =5\n", "trace line 1: expected name=value, found '=5'"},
        {"A site call x=\n", "trace line 1: expected name=value, found 'x='"},
        {"A site call x=zz\n", "trace line 1: value 'zz' of x is not an integer"},
        {"A site call x=12abc\n", "value '12abc' of x is not an integer"},
        {"const X\n", "trace line 1: expected: const NAME VALUE"},
        {"const X zz\n", "trace line 1: expected: const NAME VALUE"},
        {"owner A\n", "trace line 1: expected: owner ACTOR KEY [KEY ...]"},
        {"owner A zz\n", "trace line 1: owner key 'zz' is not an integer"},
        {"# ok\nA site call\nA site ret result=q\n", "trace line 3: value 'q' of result is not an integer"},
    };
    for (const auto& b : bad) {
      Trace t;
      const kz::Status s = kz::eval::parse_trace_text(b.text, &t);
      KZ_CHECK_ERR(s, kz::Err::Invalid);
      if (s.message.find(b.message) == std::string::npos) std::fprintf(stderr, "wanted \"%s\", got \"%s\"\n", b.message, s.message.c_str());
      KZ_CHECK(s.message.find(b.message) != std::string::npos);
    }
  }

  // ---- version comparison ---------------------------------------------------------------------------------------------------------------------------
  {
    using kz::phi::CmpOp;
    const struct {
      const char* have;
      CmpOp op;
      const char* want;
      bool expect;
    } v[] = {
        {"6.4.2", CmpOp::Ge, "6.4.2", true},   {"6.4.1", CmpOp::Ge, "6.4.2", false}, {"6.4.10", CmpOp::Gt, "6.4.9", true},   {"6.10", CmpOp::Gt, "6.9", true},
        {"7", CmpOp::Ge, "6.4.2", true},       {"2.0", CmpOp::Eq, "2.0.0", true},    {"2.0.0", CmpOp::Eq, "2", true},         {"2.0.1", CmpOp::Ne, "2.0.1", false},
        {"2.0.1", CmpOp::Lt, "2.0.2", true},   {"2.0.2", CmpOp::Le, "2.0.2", true},  {"02.0.1", CmpOp::Eq, "2.0.1", true},    {"1.2.3", CmpOp::Lt, "1.2.3", false},
        {"1.2.3a", CmpOp::Gt, "1.2.3", true},  {"1.2.3a", CmpOp::Lt, "1.2.3b", true}, {"10.0.0", CmpOp::Gt, "9.99.99", true}, {"0", CmpOp::Ge, "0.0.0", true},
    };
    for (const auto& c : v) {
      const bool got = kz::eval::version_satisfies(c.have, c.op, c.want);
      if (got != c.expect) std::fprintf(stderr, "version_satisfies(%s %s %s) = %d\n", c.have, kz::phi::cmp_text(c.op), c.want, got);
      KZ_CHECK_EQ(got, c.expect);
    }
  }

  return kztest::finish("test_core_replay");
}

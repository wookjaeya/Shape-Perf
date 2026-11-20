// The merged order of a real record is a representation, not an order (SPEC 5.8): tool tests that keep every actor's
// program order and the run's control facts the same and change only the merged representation, as a backend may give
// it (a clock tie broken by decode order, events without a clock merged in buffer order, another decode order), and check
// that the edges the replay makes and its results do not change (eval_trace.hpp linearize_real_trace; docs/replay_output.md
// section 7).  Small synthetic records on the state-order mini profile (support/state_mini.hpp); unit-test values, not a
// research condition and not a case (CONDITIONS_POLICY).
//
//   reads_from and spawn  a successful lookup reads from another task's creation; the representations put the lookup
//                before the creation, or every task's events before the creating task's: the same edges (rule, actor and
//                position in the actor) and the same result (READY on state-order + return-derived-order);
//   control      a failed lookup ordered before the provider's creation by a control edge (P1 off): the representations
//                put the creation first; the same control edge and NOT_READY on state-order + control-order;
//   conflict     a successful lookup that reads from the provider's creation, and a control edge that orders the lookup
//                before that creation (an inconsistent record): whichever direction the merged order gives either edge,
//                both are refused (they lie on one cycle), with the same edges and the same withheld result;
//   loss         a loss interval before a read that the linearization moves (a backward control edge, a backward
//                reads_from match): the read stays after the loss's begin and keeps the loss reason;
//   reading      a reading without control order of a record with a control edge is that of the record without it (no
//                event moved by the control edge); a reading without reads-from edges is not shaped by an RF match;
//   adapter      a kzrt-like neutral layer with a gate-hold and its release (NCTL): with the actors' events merged in
//                clock order, actor by actor, or the held actor first, the adapter makes the same control edge; two control
//                edges that contradict program order together: both are refused, each with the reason; a reading without
//                control order gives the trace of the layer without NCTL.

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/trace/neutral.hpp"
#include "kz/trace/real_trace.hpp"
#include "support/kz_test.hpp"
#include "support/state_mini.hpp"

namespace {

using kz::eval::HbEdge;
using kz::eval::Options;
using kz::eval::Result;
using kz::eval::Trace;

bool has(const std::string& text, const std::string& fragment) { return text.find(fragment) != std::string::npos; }

// Per-actor scripts (program order) merged by a pick string: one character per event, the actor's next line.
std::string merge(const std::map<char, std::vector<std::string>>& scripts, const std::string& picks) {
  std::map<char, size_t> next;
  std::string out;
  for (char c : picks) {
    if (c == ' ') continue;
    const auto it = scripts.find(c);
    KZ_CHECK(it != scripts.end() && next[c] < it->second.size());
    if (it == scripts.end() || next[c] >= it->second.size()) return out;
    out += it->second[next[c]++] + "\n";
  }
  return out;
}

// An event named by its actor and its position in the actor (path independent).
std::string at(const Trace& t, uint64_t e) {
  int k = 0;
  for (uint64_t i = 0; i < e; ++i) k += t.events[static_cast<size_t>(i)].actor == t.events[static_cast<size_t>(e)].actor;
  return t.events[static_cast<size_t>(e)].actor + "#" + std::to_string(k);
}
uint64_t find(const Trace& t, const std::string& actor, int k) {
  for (const kz::eval::TraceEvent& e : t.events)
    if (e.actor == actor && k-- == 0) return e.id;
  KZ_CHECK(false);
  return 0;
}

struct Seen {
  std::set<std::string> edges;  // rule from -> to, by actor positions
  std::string results;          // per occurrence: status, summary view, basis
  uint64_t moved = 0;
  std::vector<std::string> lines;
};

Seen replay(const kz::phi::Profile& p, const std::string& text, const Options& o, const std::vector<std::tuple<std::string, int, std::string, int>>& control = {}) {
  Seen out;
  Trace t;
  KZ_CHECK_OK(kz::eval::parse_trace_text(text, &t));
  for (const auto& c : control) {
    HbEdge e;
    e.from = find(t, std::get<0>(c), std::get<1>(c));
    e.to = find(t, std::get<2>(c), std::get<3>(c));
    e.rule = "control:gate0/release1";
    e.source = 2;
    t.control_edges.push_back(e);
  }
  kz::eval::LinearizeReport lr;
  KZ_CHECK_OK(kz::eval::linearize_real_trace(p, o, &t, &lr));
  out.moved = lr.moved;
  out.lines = lr.lines;
  Result r;
  KZ_CHECK_OK(kz::eval::replay(p, t, o, &r));
  for (const HbEdge& e : r.edges)
    if (t.events[static_cast<size_t>(e.from)].actor != t.events[static_cast<size_t>(e.to)].actor) out.edges.insert(e.rule + " " + at(t, e.from) + " -> " + at(t, e.to));
  for (const HbEdge& e : t.control_edges) out.edges.insert(e.rule + " " + at(t, e.from) + " -> " + at(t, e.to));
  for (const kz::eval::Occurrence& oc : r.occurrences)
    out.results += oc.rspec + " " + oc.actor + " " + std::to_string(oc.ordinal) + " " + kz::eval::occ_status_text(oc.status) + " " + std::to_string(oc.summary.observation) +
                   " basis " + std::to_string(oc.basis) + (oc.basis_store_order ? "+store" : "") + (oc.control_order ? "+control" : "") + "\n";
  return out;
}

void same(const Seen& a, const Seen& b, const char* what) {
  const bool ok = a.edges == b.edges && a.results == b.results;
  KZ_CHECK(ok);
  if (!ok) {
    std::fprintf(stderr, "%s: the representations give other edges or results\n  A:", what);
    for (const std::string& e : a.edges) std::fprintf(stderr, " [%s]", e.c_str());
    std::fprintf(stderr, "\n  B:");
    for (const std::string& e : b.edges) std::fprintf(stderr, " [%s]", e.c_str());
    std::fprintf(stderr, "\n  A results:\n%s  B results:\n%s", a.results.c_str(), b.results.c_str());
    for (const std::string& l : b.lines) std::fprintf(stderr, "  B: %s\n", l.c_str());
  }
}

std::map<char, std::vector<std::string>> scripts(const char* result) {
  return {{'M', {"M wipe.clear pt", "M spawn call", "M spawn ret out.tid=1 result=0", "M spawn call", "M spawn ret out.tid=2 result=0"}},
          {'A', {"A entry call arg.tid=1", "A lookup call arg.name=@S0", "A lookup.exists pt", std::string("A lookup ret ") + (result[0] == '0' ? "out.id=0x30002 " : "") + "result=" + result}},
          {'B', {"B entry call arg.tid=2", "B create call arg.name=@S0", "B create.publish pt lp.status=0", "B create ret out.id=0x30002 result=0"}}};
}

void test_reads_from_and_spawn() {
  const kz::phi::Profile p = kzstate::load();
  const auto s = scripts("0");
  const std::string head = kzstate::kHeader;
  // in clock order: the creating task first, A and B entered, B creates before A looks up
  const Seen clock = replay(p, head + merge(s, "MMMMM A BBBB AAA"), Options());
  KZ_CHECK(clock.moved == 0);
  KZ_CHECK(has(clock.results, "READY"));
  bool rf = false;
  for (const std::string& e : clock.edges) rf = rf || e.rfind("RF ", 0) == 0;
  KZ_CHECK(rf);
  // a tie broken the other way: A's lookup merged before B's creation
  same(clock, replay(p, head + merge(s, "MMMMM A B AAA BBB"), Options()), "tie");
  // no clock, one buffer after another (A, B, then M): every task's events before the spawns that created them
  same(clock, replay(p, head + merge(s, "AAAA BBBB MMMMM"), Options()), "actor by actor A B M");
  same(clock, replay(p, head + merge(s, "BBBB AAAA MMMMM"), Options()), "actor by actor B A M");
  same(clock, replay(p, head + merge(s, "MMMMM BBBB AAAA"), Options()), "actor by actor M B A");
  // P1 off: the same
  Options off;
  off.outcome_narrowing = false;
  same(replay(p, head + merge(s, "MMMMM A BBBB AAA"), off), replay(p, head + merge(s, "AAAA BBBB MMMMM"), off), "P1 off");
}

void test_control() {
  const kz::phi::Profile p = kzstate::load();
  const auto s = scripts("-17");
  const std::string head = kzstate::kHeader;
  Options o;
  o.outcome_narrowing = false;
  const std::vector<std::tuple<std::string, int, std::string, int>> ctl{{"A", 3, "B", 1}};  // A's lookup return -> B's create call
  const Seen clock = replay(p, head + merge(s, "MMMMM A B AAA BBB"), o, ctl);
  KZ_CHECK(has(clock.results, "NOT_READY_UNJUDGED") && has(clock.results, "+control"));
  same(clock, replay(p, head + merge(s, "MMMMM BBBB AAAA"), o, ctl), "creation merged first");
  same(clock, replay(p, head + merge(s, "BBBB AAAA MMMMM"), o, ctl), "actor by actor B A M");
  // control order off: the control edge is not an order, the creation is not ordered with the lookup either way
  Options ctloff = o;
  ctloff.control_order = false;
  same(replay(p, head + merge(s, "MMMMM A B AAA BBB"), ctloff, ctl), replay(p, head + merge(s, "MMMMM BBBB AAAA"), ctloff, ctl), "control order off");
  // two control edges that contradict program order together (B's create return -> A's entry, while A's lookup return ->
  // B's create call): both are refused, each with the reason naming the other (the record's order facts contradict each
  // other; which one came first decides nothing)
  Trace t;
  KZ_CHECK_OK(kz::eval::parse_trace_text(head + merge(s, "MMMMM A B AAA BBB"), &t));
  HbEdge e1, e2;
  e1.from = find(t, "A", 3);
  e1.to = find(t, "B", 1);
  e1.rule = "control:gate0/release1";
  e2.from = find(t, "B", 3);
  e2.to = find(t, "A", 1);
  e2.rule = "control:gate2/release3";
  t.control_edges = {e1, e2};
  kz::eval::LinearizeReport lr;
  KZ_CHECK_OK(kz::eval::linearize_real_trace(p, o, &t, &lr));
  KZ_CHECK(lr.refused_control.size() == 2 && t.control_edges.empty());
  int said = 0;
  for (const std::string& l : lr.lines)
    said += (has(l, "control edge control:gate2/release3") && has(l, "cycle with control edge control:gate0/release1")) ||
            (has(l, "control edge control:gate0/release1") && has(l, "cycle with control edge control:gate2/release3"));
  KZ_CHECK_EQ(said, 2);
  // the same two edges given in the other order: the same refusal
  Trace u;
  KZ_CHECK_OK(kz::eval::parse_trace_text(head + merge(s, "MMMMM A B AAA BBB"), &u));
  u.control_edges = {e2, e1};
  KZ_CHECK_OK(kz::eval::linearize_real_trace(p, o, &u, &lr));
  KZ_CHECK(lr.refused_control.size() == 2 && u.control_edges.empty());
}

// A successful lookup reading from B's creation (RF) and a control edge from the lookup's return to B's create call: the
// record contradicts itself.  B's creation merged first (the RF match forward, the control edge backward) or the lookup
// merged first (the RF match backward, the control edge forward): both edges refused, the same withheld result.
void test_conflict() {
  const kz::phi::Profile p = kzstate::load();
  const auto s = scripts("0");
  const std::string head = kzstate::kHeader;
  const std::vector<std::tuple<std::string, int, std::string, int>> ctl{{"A", 3, "B", 1}};
  for (bool p1 : {false, true}) {
    Options o;
    o.outcome_narrowing = p1;
    const Seen creation_first = replay(p, head + merge(s, "MMMMM A B BBB AAA"), o, ctl);
    const Seen lookup_first = replay(p, head + merge(s, "MMMMM A B AAA BBB"), o, ctl);
    same(creation_first, lookup_first, p1 ? "conflict, P1 on" : "conflict, P1 off");
    for (const Seen* v : {&creation_first, &lookup_first}) {
      for (const std::string& e : v->edges) KZ_CHECK(e.rfind("RF ", 0) != 0 && e.rfind("control:", 0) != 0);
      int refused = 0;
      for (const std::string& l : v->lines) refused += has(l, "refused: it lies on a cycle with");
      KZ_CHECK_EQ(refused, 2);
      if (!p1) KZ_CHECK(!has(v->results, " READY ") && !has(v->results, " NOT_READY"));
    }
  }
}

uint32_t reasons_of(const Result& r, const std::string& actor) {
  for (const kz::eval::Occurrence& oc : r.occurrences)
    if (oc.actor == actor) return oc.summary.obs_reasons;
  KZ_CHECK(false);
  return 0;
}

// Every event at or after a loss's begin stays at or after it, and every event before its end stays before it.
void check_cut(const Trace& before, const Trace& after, const kz::eval::LinearizeReport& lr) {
  KZ_CHECK(lr.moved > 0 && before.losses.size() == after.losses.size());
  for (size_t j = 0; j < before.losses.size() && j < after.losses.size(); ++j)
    for (size_t i = 0; i < before.events.size(); ++i) {
      if (i >= before.losses[j].begin) KZ_CHECK(lr.position[i] >= after.losses[j].begin);
      if (before.losses[j].end != kz::eval::kNoEvent && i < before.losses[j].end) KZ_CHECK(lr.position[i] < after.losses[j].end);
    }
}

void test_loss() {
  const kz::phi::Profile p = kzstate::load();
  const std::string head = kzstate::kHeader;
  // a control edge backward: B's create call lost, then A's failed lookup, and the control edge from the lookup's return
  // to the create call; A's lookup followed the loss in the record
  {
    const std::string text = head + kzstate::m_start() + kzstate::b_entry() + "loss begin\nB create call arg.name=@S0\nloss end\n" + kzstate::a_entry() +
                             kzstate::a_lookup("-17", nullptr) + "B create.publish pt lp.status=0\nB create ret out.id=0x30002 result=0\n";
    Options o;
    o.outcome_narrowing = false;
    Trace t;
    KZ_CHECK_OK(kz::eval::parse_trace_text(text, &t));
    const Trace given = t;
    HbEdge e;
    e.from = find(t, "A", 3);
    e.to = find(t, "B", 1);
    e.rule = "control:gate0/release1";
    e.source = 2;
    t.control_edges.push_back(e);
    kz::eval::LinearizeReport lr;
    KZ_CHECK_OK(kz::eval::linearize_real_trace(p, o, &t, &lr));
    check_cut(given, t, lr);
    Result r;
    KZ_CHECK_OK(kz::eval::replay(p, t, o, &r));
    KZ_CHECK((reasons_of(r, "A") & KZ_WITHHOLD_LOSS) != 0);
  }
  // a reads_from match backward: C (a third task) looks up another name after the loss of A's lookup point, then B
  // creates the semaphore A's lookup read from; the linearization delays A's lookup point after B's creation
  {
    const std::string text = head + kzstate::m_start() + "M spawn call\nM spawn ret out.tid=3 result=0\n" + kzstate::a_entry() + kzstate::b_entry() +
                             "C entry call arg.tid=3\nA lookup call arg.name=@S0\nloss begin\nA lookup.exists pt\nloss end\n"
                             "C lookup call arg.name=@S1\nC lookup.exists pt\nC lookup ret result=-17\n" +
                             kzstate::b_create("0x30002") + "A lookup ret out.id=0x30002 result=0\n";
    Options o;
    o.outcome_narrowing = false;  // C's failed lookup is read from the state (P1 on would narrow it by its returned code)
    Trace t;
    KZ_CHECK_OK(kz::eval::parse_trace_text(text, &t));
    const Trace given = t;
    kz::eval::LinearizeReport lr;
    KZ_CHECK_OK(kz::eval::linearize_real_trace(p, o, &t, &lr));
    KZ_CHECK_EQ(lr.matches.size(), static_cast<size_t>(1));
    check_cut(given, t, lr);
    Result r;
    KZ_CHECK_OK(kz::eval::replay(p, t, o, &r));
    KZ_CHECK((reasons_of(r, "C") & KZ_WITHHOLD_LOSS) != 0);
  }
}

// A reading leaves out a kind of order: the linearization is not shaped by it.
void test_reading() {
  const kz::phi::Profile p = kzstate::load();
  const std::string head = kzstate::kHeader;
  auto without_control = [](Seen v) {
    std::set<std::string> kept;
    for (const std::string& e : v.edges)
      if (e.rfind("control:", 0) != 0) kept.insert(e);
    v.edges = kept;
    return v;
  };
  // control order off: a record with a backward control edge reads as the record without it (P1 off and on)
  for (const char* result : {"-17", "0"}) {
    const auto s = scripts(result);
    for (bool p1 : {false, true}) {
      Options off;
      off.outcome_narrowing = p1;
      off.control_order = false;
      const std::string text = head + merge(s, "MMMMM A B BBB AAA");
      const Seen with = replay(p, text, off, {{"A", 3, "B", 1}});
      const Seen without = replay(p, text, off);
      KZ_CHECK_EQ(with.moved, static_cast<uint64_t>(0));
      same(without_control(with), without, "control order off, with and without the control edge");
    }
  }
  // reads-from edges off: the lookup merged before the creation it read from moves nothing
  {
    const auto s = scripts("0");
    Options off;
    off.reads_from_edges = false;
    const Seen v = replay(p, head + merge(s, "MMMMM A B AAA BBB"), off);
    KZ_CHECK_EQ(v.moved, static_cast<uint64_t>(0));
    for (const std::string& e : v.edges) KZ_CHECK(e.rfind("RF ", 0) != 0);
  }
}

// ---- the adapter: a kzrt-like neutral layer with a gate-hold and its release ----------------------------------------
// actors 0 A (consumer), 1 B (provider); sites 0 lookup call, 1 lookup return, 2 point a_release (after the lookup, the
// release site), 3 point b_gate (B's gate observation, written before it waits), 4 create call, 5 create return.
// Events per actor (aseq): A: lookup call, lookup return (-17), a_release; B: b_gate, create call, create return.
struct Ev {
  uint32_t actor, site;
  int64_t result;  // a return's result, or 0
};
kz::trace::NeutralRun layer(const std::vector<std::pair<uint32_t, uint32_t>>& merged, bool clocked, bool second_gate = false) {
  using namespace kz::trace;
  NeutralRun n;
  n.run.r.backend = KZ_NBACKEND_KZRT;
  n.run.r.clock = clocked ? KZ_NCLOCK_MONOTONIC : KZ_NCLOCK_NONE;
  n.run.r.flags = KZ_NRUN_FLAG_COMPLETE | KZ_NRUN_FLAG_LOSS_EVIDENCE | KZ_NRUN_FLAG_CONTROL_RECORDED;
  n.run.backend_name = "kzrt";
  auto site = [&](uint8_t kind, const char* label, const char* symbol) {
    NSite s;
    s.r.id = static_cast<uint32_t>(n.sites.size() + 1);
    s.r.kind = kind;
    s.r.flags = KZ_NSITE_FLAG_ATTACHED;
    s.r.bind = KZ_NONE32;
    s.label = label;
    s.symbol = symbol;
    n.sites.push_back(s);
  };
  site(KZ_NSITE_KIND_CALL, "lookup.call", "lookup");
  site(KZ_NSITE_KIND_RETURN, "lookup.ret", "lookup");
  site(KZ_NSITE_KIND_POINT, "a_release", "consumer");
  site(KZ_NSITE_KIND_POINT, "b_gate", "provider");
  site(KZ_NSITE_KIND_CALL, "create.call", "create");
  site(KZ_NSITE_KIND_RETURN, "create.ret", "create");
  NSlot result;
  result.r.site = 1;
  result.r.slot = 0;
  result.r.kind = KZ_NVAL_INT;
  result.role = "result";
  n.slots.push_back(result);
  for (const char* name : {"A", "B"}) {
    NActor a;
    a.r = nactor_init();
    a.r.binding = KZ_NBIND_KZRT_ACTOR;
    a.r.backend_id = static_cast<uint32_t>(n.actors.size() + 1);
    a.name = name;
    n.actors.push_back(a);
  }
  // the per-actor scripts
  const std::vector<std::vector<Ev>> per{{{0, 0, 0}, {0, 1, -17}, {0, 2, 0}}, {{1, 3, 0}, {1, 4, 0}, {1, 5, 0}}};
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> index;  // (actor, aseq) -> NEVENTS index
  for (size_t i = 0; i < merged.size(); ++i) {
    const Ev& e = per[merged[i].first][merged[i].second];
    kz_nevent ne = nevent_init();
    ne.actor = e.actor;
    ne.aseq = merged[i].second + 1;
    ne.site = e.site;
    ne.flags = KZ_NEV_FLAG_ASEQ_TARGET | (clocked ? KZ_NEV_FLAG_CLOCK : 0);
    ne.t = clocked ? 1000 + 10 * i : 0;
    ne.first_value = static_cast<uint32_t>(n.values.size());
    if (e.site == 1) {
      NValue v;
      v.r.event = static_cast<uint32_t>(i);
      v.r.slot = 0;
      v.r.kind = KZ_NVAL_INT;
      v.r.v = static_cast<uint64_t>(e.result);
      n.values.push_back(v);
      ne.nvals = 1;
    }
    n.events.push_back(ne);
    index[{e.actor, merged[i].second}] = static_cast<uint32_t>(i);
  }
  // the control facts: B held at its gate (b_gate), released by A's hit at a_release
  kz_nctl g = nctl_init();
  g.what = KZ_NCTL_WHAT_GATE_HOLD;
  g.actor = 1;
  g.subject = 1;
  g.event = index[{1, 0}];
  g.site = 3;
  g.ctl_seq = 1;
  g.point = 0;
  g.gate = 0;
  g.hit = 1;
  kz_nctl f = nctl_init();
  f.what = KZ_NCTL_WHAT_FIRE;
  f.reason = KZ_NCTL_REASON_RESUMED;
  f.actor = 0;
  f.subject = 0;
  f.other = 1;
  f.event = index[{0, 2}];
  f.site = 2;
  f.ctl_seq = 2;
  f.point = 1;
  f.gate = 0;
  f.hit = 1;
  n.ctl = {g, f};
  if (second_gate) {  // A held at its lookup call (its first event) until B's create return: with the first, a cycle
    kz_nctl g2 = g, f2 = f;
    g2.actor = g2.subject = 0;
    g2.event = index[{0, 0}];
    g2.ctl_seq = 3;
    g2.point = g2.gate = 2;
    f2.actor = f2.subject = 1;
    f2.other = 0;
    f2.event = index[{1, 2}];
    f2.ctl_seq = 4;
    f2.point = 3;
    f2.gate = 2;
    // the gate observation is written before the wait: the edge ends at A's first event after it (its lookup return)
    n.ctl.push_back(g2);
    n.ctl.push_back(f2);
  }
  return n;
}

struct AdapterSeen {
  std::vector<std::string> edges;
  std::vector<std::string> lines;
};
AdapterSeen adapt(const kz::trace::NeutralRun& n, const Options* reading = nullptr, std::string* events = nullptr) {
  AdapterSeen out;
  const kz::phi::Profile p = kzstate::load();
  Trace t;
  kz::trace::RealTraceReport rep;
  kz::trace::RealTraceOptions opt;
  opt.reading = reading;
  KZ_CHECK_OK(kz::trace::real_trace_from_neutral(n, p, opt, &t, &rep));
  for (const HbEdge& e : t.control_edges) out.edges.push_back(e.rule + " " + at(t, e.from) + " -> " + at(t, e.to));
  out.lines = rep.control_lines;
  if (events != nullptr)
    for (const kz::eval::TraceEvent& e : t.events) *events += e.actor + " " + e.site + " " + kz::eval::phase_text(e.phase) + "\n";
  return out;
}

void test_adapter() {
  // B's gate observation first (it waits), A looks up and releases, B creates: clock order
  const AdapterSeen clock = adapt(layer({{1, 0}, {0, 0}, {0, 1}, {0, 2}, {1, 1}, {1, 2}}, true));
  KZ_CHECK_EQ(clock.edges.size(), static_cast<size_t>(1));
  KZ_CHECK(!clock.edges.empty() && clock.edges[0] == "control:gate0/release1 A#1 -> B#0");
  // no clock, buffer after buffer: A then B, and B then A (the creation merged before the lookup that released it)
  const AdapterSeen ab = adapt(layer({{0, 0}, {0, 1}, {0, 2}, {1, 0}, {1, 1}, {1, 2}}, false));
  const AdapterSeen ba = adapt(layer({{1, 0}, {1, 1}, {1, 2}, {0, 0}, {0, 1}, {0, 2}}, false));
  KZ_CHECK(ab.edges == clock.edges);
  KZ_CHECK(ba.edges == clock.edges);
  if (ba.edges != clock.edges)
    for (const std::string& l : ba.lines) std::fprintf(stderr, "B then A: %s\n", l.c_str());
  // two control edges that contradict program order together: both are refused, each with the reason
  const AdapterSeen cyc = adapt(layer({{1, 0}, {0, 0}, {0, 1}, {0, 2}, {1, 1}, {1, 2}}, true, true));
  KZ_CHECK(cyc.edges.empty());
  int refused = 0;
  for (const std::string& l : cyc.lines) refused += (has(l, "gate2/release3") || has(l, "gate0/release1")) && has(l, "no edge (it contradicts");
  KZ_CHECK_EQ(refused, 2);
  // control order off: the layer whose control edge goes backward (B then A) gives the trace of the layer without NCTL
  Options off;
  off.control_order = false;
  std::string with, without;
  adapt(layer({{1, 0}, {1, 1}, {1, 2}, {0, 0}, {0, 1}, {0, 2}}, false), &off, &with);
  kz::trace::NeutralRun bare = layer({{1, 0}, {1, 1}, {1, 2}, {0, 0}, {0, 1}, {0, 2}}, false);
  bare.ctl.clear();
  bare.run.r.flags &= ~static_cast<uint32_t>(KZ_NRUN_FLAG_CONTROL_RECORDED);
  adapt(bare, &off, &without);
  KZ_CHECK(!with.empty() && with == without);
  std::string on;
  adapt(layer({{1, 0}, {1, 1}, {1, 2}, {0, 0}, {0, 1}, {0, 2}}, false), nullptr, &on);
  KZ_CHECK(on != with);  // control order on: the control edge moved the events
}

}  // namespace

int main() {
  test_reads_from_and_spawn();
  test_control();
  test_conflict();
  test_loss();
  test_reading();
  test_adapter();
  return kztest::finish("test_trace_merge_order");
}

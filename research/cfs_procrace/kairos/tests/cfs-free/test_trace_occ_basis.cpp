// The per-occurrence basis of a replay output container (schema revision 4; kz_replay.h, replay_container.hpp,
// occ_table.hpp, docs/replay_output.md): small synthetic records on the state-order mini profile (support/state_mini.hpp).
// Tool tests: every value is a unit-test value, not a research condition and not a case (CONDITIONS_POLICY).
//
//   - OCC_BASIS keeps, per occurrence, what summary.flags has no room for: store-derived-order apart from
//     return-derived-order, control-order, the edges the result rests on (a reads_from edge, a spawn edge, a control
//     edge, by kind and rule), with the verdict, the summary view, the outcome check and the occurrence key; REPLAY_READ
//     keeps every switch of the reading;
//   - a container without the new sections (written before revision 4) still reads: the rows come from SUMMARIES, the
//     basis from summary.flags, and the rest is not recorded (never an empty basis);
//   - kz occ diff: the same record read twice compares equal, also with other actor names (the key is path independent);
//     two readings (control order on and off) give one verdict difference and one basis difference, listed apart;
//   - keys (revision 5): two tasks of one application with the same key are told apart by the task's place where both
//     containers record it; where they are not, the shared key is listed as ambiguous and not paired (a difference); an
//     occurrence whose key value one side fixed and the other did not is matched by its text arguments and said apart;
//     different text arguments stay one side only;
//   - kz occ diff as a command (with the kz binary and a directory as arguments, test test_trace_occ_basis_cli): the
//     process exits 0 for a record against itself, 3 where the owners differ, 4 where a key two occurrences share is
//     not paired (cmd_occ.cpp returns occ_diff_status).

#include <sys/wait.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/sha256.hpp"
#include "kz/trace/occ_table.hpp"
#include "kz/trace/replay_container.hpp"
#include "support/kz_test.hpp"
#include "support/state_mini.hpp"

namespace {

using kz::Container;
using kz::eval::Options;
using kz::eval::Result;
using kz::eval::Trace;

bool has(const std::string& text, const std::string& fragment) { return text.find(fragment) != std::string::npos; }

// The trace text with the owner records of A (an application whose record key is 9).
std::string owned(const std::string& body) { return std::string(kzstate::kHeader) + "owner A 9\nowner_level A confirmed\n" + body; }

struct Out {
  Trace trace;
  Result result;
  kz::Bytes bytes;
  Container c;
  kz::trace::OccTable table;
};

Out run(const kz::phi::Profile& p, const std::string& text, const Options& o, const std::vector<std::pair<uint64_t, uint64_t>>& control = {}) {
  Out r;
  KZ_CHECK_OK(kz::eval::parse_trace_text(text, &r.trace));
  for (const auto& c : control) {
    kz::eval::HbEdge e;
    e.from = c.first;
    e.to = c.second;
    e.rule = "control:gate0/release1";
    e.source = 2;
    r.trace.control_edges.push_back(e);
  }
  KZ_CHECK_OK(kz::eval::replay(p, r.trace, o, &r.result));
  kz::trace::ReplayContainerInput in;
  in.profile_sha256 = kz::phi::profile_hash(p);
  in.tool_sha256 = kz::sha256(std::string("unit-test tool"));
  in.options = &o;
  KZ_CHECK_OK(kz::trace::write_replay_container(in, r.trace, r.result, kz::trace::RealTraceReport(), &r.bytes));
  KZ_CHECK_OK(Container::parse(r.bytes, &r.c));
  KZ_CHECK_OK(r.c.verify_content());
  KZ_CHECK_OK(kz::trace::read_occ_table(r.c, &r.table));
  return r;
}

// the trace position of the n-th event (from 0) of `actor` at `site`
uint64_t at(const Trace& t, const std::string& actor, const std::string& site, int n = 0) {
  for (const kz::eval::TraceEvent& e : t.events)
    if (e.actor == actor && e.site == site && n-- == 0) return e.id;
  KZ_CHECK(false);
  return 0;
}

bool edge_of(const kz::trace::OccRow& r, uint8_t kind, const std::string& rule) {
  for (const kz::trace::OccEdge& e : r.edges)
    if (e.kind == kind && e.rule == rule) return true;
  return false;
}

void test_return_derived() {
  using namespace kzstate;
  const kz::phi::Profile p = load();
  const Out r = run(p, owned(m_start() + a_entry() + b_entry() + b_create("0x30002") + a_lookup("0", "0x30002")), Options());
  KZ_CHECK(r.table.recorded && r.table.has_reading && r.table.rows.size() == 1);
  const kz::trace::OccRow& o = r.table.rows[0];
  KZ_CHECK_EQ(o.rspec, std::string("K-LOOKUP"));
  KZ_CHECK_EQ(o.op, std::string("lookup"));
  KZ_CHECK_EQ(o.name, std::string("name=S0"));
  KZ_CHECK(!o.resource.empty() && o.ordinal == 1);
  KZ_CHECK(o.owner_key == 9 && o.owner_level == KZ_OWNER_LEVEL_CONFIRMED);
  KZ_CHECK(o.status == KZ_OCC_READY && o.observation == KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX);
  KZ_CHECK_EQ(o.basis, static_cast<uint32_t>(KZ_OCC_BASIS_STATE_ORDER | KZ_OCC_BASIS_RETURN_DERIVED_ORDER));
  KZ_CHECK((o.assumptions & KZ_OCC_ASSUME_READS_FROM) != 0);
  KZ_CHECK_EQ(o.outcome, static_cast<uint8_t>(KZ_OUTCOME_CHECK_AGREES_RETURN_DERIVED));
  // the edges: the reads_from edge (by the code) and the creation of A (the clearing by M is ordered before the read)
  KZ_CHECK(edge_of(o, KZ_OCC_EDGE_KIND_READS_FROM, "RF"));
  KZ_CHECK(edge_of(o, KZ_OCC_EDGE_KIND_RULE, "SP"));
  for (const kz::trace::OccEdge& e : o.edges) KZ_CHECK(e.from < e.to && e.to <= r.trace.events.size());
  // the summary keeps what it had (the flags are those of revision 2-3)
  std::vector<kz_summary> sums;
  KZ_CHECK_OK(r.c.read_section(KZ_SEC_SUMMARIES, &sums));
  KZ_CHECK(sums.size() == 1 && (sums[0].flags & KZ_SUMMARY_FLAG_BASIS_RETURN_DERIVED_ORDER) != 0);
  const kz_replay_read& rd = r.table.reading;
  KZ_CHECK(rd.format == 2 && r.table.task_recorded && rd.outcome_narrowing == KZ_READ_SWITCH_ON && rd.control_order == KZ_READ_SWITCH_ON && rd.reads_from_outcome == KZ_READ_SWITCH_ON &&
           rd.relation_rule == KZ_RELATION_RULE_P10_OCCURRENCE && rd.control_edges == 0);
  const std::string text = kz::trace::occ_table_text(r.table);
  KZ_CHECK(has(text, "basis {state-order,return-derived-order}") && has(text, "RF[reads-from]#") && has(text, "outcome agrees-return-derived"));
}

void test_store_derived() {
  using namespace kzstate;
  std::string prof = kProfile;
  const std::string rf = "match id; }";
  prof.replace(prof.find(rf), rf.size(), "match id; stored_out id prov code_a; }");
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile({kz::phi::SourceText{"state_mini_store.kzp", prof}}, &p, &diags));
  const std::string stored = "A lookup call arg.name=@S0\nA lookup.exists pt\nA lookup ret out.id=0x30002 store.out.id=@found result=0\n";
  const Out r = run(p, owned(m_start() + a_entry() + b_entry() + b_create("0x30002") + stored), Options());
  KZ_CHECK(r.table.rows.size() == 1);
  const kz::trace::OccRow& o = r.table.rows[0];
  // store-derived-order (summary.flags has no bit for it): kept here, return-derived-order is not set
  KZ_CHECK_EQ(o.basis, static_cast<uint32_t>(KZ_OCC_BASIS_STATE_ORDER | KZ_OCC_BASIS_STORE_DERIVED_ORDER));
  KZ_CHECK((o.assumptions & KZ_OCC_ASSUME_READS_FROM) != 0);
  KZ_CHECK_EQ(o.outcome, static_cast<uint8_t>(KZ_OUTCOME_CHECK_AGREES));
  bool by_store = false;
  for (const kz::trace::OccEdge& e : o.edges) by_store = by_store || (e.kind == KZ_OCC_EDGE_KIND_READS_FROM && (e.flags & KZ_OCC_EDGE_FLAG_BY_STORE) != 0);
  KZ_CHECK(by_store);
}

// A failed lookup before the provider creates the name, ordered with the creation only by a control edge (A's return ->
// B's create call), outcome narrowing off: the not-ready state is computed only with the control order.
std::string control_body() {
  using namespace kzstate;
  return owned(m_start() + a_entry() + b_entry() + a_lookup("-17", nullptr) + b_create("0x30002"));
}

void test_control_order() {
  const kz::phi::Profile p = kzstate::load();
  Trace t;
  KZ_CHECK_OK(kz::eval::parse_trace_text(control_body(), &t));
  const std::vector<std::pair<uint64_t, uint64_t>> ctl{{at(t, "A", "lookup", 1), at(t, "B", "create")}};
  Options on;
  on.outcome_narrowing = false;
  Options off = on;
  off.control_order = false;
  const Out a = run(p, control_body(), on, ctl);
  const Out b = run(p, control_body(), off, ctl);
  KZ_CHECK(a.table.rows.size() == 1 && b.table.rows.size() == 1);
  const kz::trace::OccRow& x = a.table.rows[0];
  KZ_CHECK(x.status == KZ_OCC_NOT_READY_UNJUDGED);
  KZ_CHECK((x.basis & KZ_OCC_BASIS_CONTROL_ORDER) != 0 && (x.basis & KZ_OCC_BASIS_STATE_ORDER) != 0 && (x.basis & KZ_OCC_BASIS_OUTCOME_NARROWED) == 0);
  KZ_CHECK(edge_of(x, KZ_OCC_EDGE_KIND_CONTROL, "control:gate0/release1"));
  KZ_CHECK(a.table.reading.control_order == KZ_READ_SWITCH_ON && a.table.reading.control_edges == 1 && a.table.reading.outcome_narrowing == KZ_READ_SWITCH_OFF);
  KZ_CHECK(b.table.reading.control_order == KZ_READ_SWITCH_OFF);
  KZ_CHECK((b.table.rows[0].basis & KZ_OCC_BASIS_CONTROL_ORDER) == 0 && b.table.rows[0].status != KZ_OCC_NOT_READY_UNJUDGED);
  // the comparison: one verdict difference and one basis difference, listed apart; the same container: none
  kz::trace::OccDiffCounts n;
  const std::string d = kz::trace::occ_diff_text(a.table, b.table, &n);
  KZ_CHECK(n.matched == 1 && n.verdict == 1 && n.basis == 1 && n.only_a == 0 && n.only_b == 0 && kz::trace::occ_diff_status(n) == 3);
  KZ_CHECK(has(d, "reading differs: control-order A on, B off") && has(d, "result: exit 3 (differences)"));
  KZ_CHECK(has(d, "verdict differences (status, summary view): 1") && has(d, "basis differences (basis kinds, assumptions, outcome check, edge rules): 1"));
  kz::trace::occ_diff_text(a.table, a.table, &n);
  KZ_CHECK(n.matched == 1 && n.verdict == 0 && n.basis == 0);
  // the key names no actor: the same record with other actor names compares equal
  std::string renamed = control_body();
  for (const auto& rn : std::vector<std::pair<std::string, std::string>>{{"\nA ", "\na13 "}, {"\nB ", "\na14 "}, {"\nM ", "\na1 "}, {"owner A ", "owner a13 "}, {"owner_level A ", "owner_level a13 "}})
    for (size_t k = renamed.find(rn.first); k != std::string::npos; k = renamed.find(rn.first, k + rn.second.size())) renamed.replace(k, rn.first.size(), rn.second);
  Trace t2;
  KZ_CHECK_OK(kz::eval::parse_trace_text(renamed, &t2));
  const Out c = run(p, renamed, on, {{at(t2, "a13", "lookup", 1), at(t2, "a14", "create")}});
  kz::trace::occ_diff_text(a.table, c.table, &n);
  KZ_CHECK(n.matched == 1 && n.by_key == 1 && n.verdict == 0 && n.basis == 0 && n.owner_unestablished == 0 && kz::trace::occ_diff_status(n) == 0);
}

// A container written before revision 4 (SUMMARIES only): read, the rest said not recorded.
void test_old_container() {
  const kz::phi::Profile p = kzstate::load();
  const Out r = run(p, owned(kzstate::m_start() + kzstate::a_entry() + kzstate::b_entry() + kzstate::b_create("0x30002") + kzstate::a_lookup("0", "0x30002")), Options());
  kz::ContainerWriter w(KZ_ARTIFACT_VERDICTS);
  std::vector<kz_summary> sums;
  KZ_CHECK_OK(r.c.read_section(KZ_SEC_SUMMARIES, &sums));
  for (kz_summary& s : sums) s.identity_first = KZ_NONE32;
  KZ_CHECK_OK(w.add_section(KZ_SEC_SUMMARIES, sums));
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  Container old;
  KZ_CHECK_OK(Container::parse(b, &old));
  kz::trace::OccTable t;
  KZ_CHECK_OK(kz::trace::read_occ_table(old, &t));
  KZ_CHECK(!t.recorded && !t.has_reading && t.rows.size() == 1 && !t.rows[0].recorded);
  KZ_CHECK_EQ(t.rows[0].basis, static_cast<uint32_t>(KZ_OCC_BASIS_STATE_ORDER | KZ_OCC_BASIS_RETURN_DERIVED_ORDER));
  const std::string text = kz::trace::occ_table_text(t);
  KZ_CHECK(has(text, "per-occurrence basis not recorded") && has(text, "edges not recorded"));
  kz::trace::OccDiffCounts n;
  const std::string d = kz::trace::occ_diff_text(t, r.table, &n);
  KZ_CHECK(has(d, "matching: by position") && n.matched == 1 && n.verdict == 0 && n.basis == 0);
}


// Rows of a table written with revision 5 (built directly: the comparison is what is tested).
kz::trace::OccRow row(uint32_t index, uint16_t task, const std::string& resource, const std::string& name, uint32_t ordinal, uint8_t status) {
  kz::trace::OccRow r;
  r.index = index;
  r.recorded = true;
  r.task_recorded = true;
  r.rspec = "K-LOOKUP";
  r.op = "lookup";
  r.owner_key = 9;
  r.owner_level = KZ_OWNER_LEVEL_CONFIRMED;
  r.task = task;
  r.resource = resource;
  r.name = name;
  r.ordinal = ordinal;
  r.status = status;
  r.observation = KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX;
  return r;
}
kz::trace::OccTable table(const std::vector<kz::trace::OccRow>& rows, bool task_recorded = true) {
  kz::trace::OccTable t;
  t.recorded = true;
  t.task_recorded = task_recorded;
  t.rows = rows;
  for (kz::trace::OccRow& r : t.rows) r.task_recorded = task_recorded;
  return t;
}

void test_keys() {
  // two tasks of application 9 look up the same name: the task's place tells them apart, in either container order
  const kz::trace::OccTable a = table({row(0, 1, "0x30002", "name=S0", 1, KZ_OCC_READY), row(1, 2, "0x30002", "name=S0", 1, KZ_OCC_NOT_READY_UNJUDGED)});
  const kz::trace::OccTable b = table({row(0, 2, "0x30002", "name=S0", 1, KZ_OCC_NOT_READY_UNJUDGED), row(1, 1, "0x30002", "name=S0", 1, KZ_OCC_READY)});
  kz::trace::OccDiffCounts n;
  std::string d = kz::trace::occ_diff_text(a, b, &n);
  KZ_CHECK(n.matched == 2 && n.verdict == 0 && n.ambiguous == 0 && !n.any());
  KZ_CHECK(has(kz::trace::occ_key(a.rows[1]), "| app[9]#2 |"));
  // the same rows where a container does not record the task: one shared key, listed and not paired by order
  const kz::trace::OccTable b4 = table(b.rows, false);
  d = kz::trace::occ_diff_text(a, b4, &n);
  KZ_CHECK(n.matched == 0 && n.ambiguous == 1 && n.ambiguous_rows == 4 && n.verdict == 0 && n.only_a == 0 && n.only_b == 0 && n.any());
  KZ_CHECK(kz::trace::occ_diff_status(n) == 4 && has(d, "result: exit 4 (the pairing of some occurrences is not established: ambiguous keys 1, uncertain 0)"));
  KZ_CHECK(has(d, "task in the key: not used (B does not record it)"));
  KZ_CHECK(has(d, "ambiguous keys (shared by more than one occurrence of a side, or more than one candidate of a row; which is which is not established, none "
                  "paired): 1"));
  KZ_CHECK(has(d, "K-LOOKUP | lookup | app[9] | 0x30002 | 1: A#0 READY") && has(d, "B#1 READY"));
  // the key value fixed on one side only: matched by the text arguments, said apart
  const kz::trace::OccTable c = table({row(0, 1, "0x30002", "name=S0", 1, KZ_OCC_READY)});
  const kz::trace::OccTable e = table({row(0, 1, "", "name=S0", 3, KZ_OCC_READY)});
  d = kz::trace::occ_diff_text(c, e, &n);
  KZ_CHECK(n.matched == 1 && n.key_differs == 1 && n.only_a == 0 && n.only_b == 0 && n.any());
  KZ_CHECK(has(d, "matched by the text arguments (the key value is fixed on one side only): 1") && has(d, "(key value fixed in A only)"));
  // other text arguments: not the same resource, one side only each
  const kz::trace::OccTable f = table({row(0, 1, "", "name=S1", 1, KZ_OCC_READY)});
  kz::trace::occ_diff_text(c, f, &n);
  KZ_CHECK(n.matched == 0 && n.key_differs == 0 && n.only_a == 1 && n.only_b == 1);
}

std::string write_kz(const std::string& dir, const char* name, const kz::Bytes& bytes) {
  const std::string path = dir + "/" + name;
  std::FILE* f = std::fopen(path.c_str(), "wb");
  KZ_CHECK(f != nullptr);
  if (f != nullptr) {
    KZ_CHECK(std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size());
    std::fclose(f);
  }
  return path;
}

int occ_diff_exit(const std::string& kz, const std::string& x, const std::string& y) {
  const int rc = std::system((kz + " occ diff " + x + " " + y + " >/dev/null").c_str());
  return rc != -1 && WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
}

void test_cli(const std::string& kz, const std::string& dir) {
  using namespace kzstate;
  const kz::phi::Profile p = load();
  const std::string body = m_start() + a_entry() + b_entry() + b_create("0x30002") + a_lookup("0", "0x30002");
  const std::string b_lookup = "B lookup call arg.name=@S0\nB lookup.exists pt\nB lookup ret out.id=0x30002 result=0\n";
  const Out x = run(p, owned(body), Options());
  const Out y = run(p, std::string(kHeader) + "owner A 6\nowner_level A confirmed\n" + body, Options());
  // A and B both of application 9, the task's place not established (no report): one key, two occurrences
  const Out z = run(p, owned(std::string("owner B 9\nowner_level B confirmed\n") + body + b_lookup), Options());
  KZ_CHECK(x.table.rows.size() == 1 && y.table.rows.size() == 1 && z.table.rows.size() == 2);
  const std::string fx = write_kz(dir, "occ_cli_x.kz", x.bytes), fy = write_kz(dir, "occ_cli_y.kz", y.bytes), fz = write_kz(dir, "occ_cli_z.kz", z.bytes);
  KZ_CHECK_EQ(occ_diff_exit(kz, fx, fx), 0);
  KZ_CHECK_EQ(occ_diff_exit(kz, fx, fy), 3);
  KZ_CHECK_EQ(occ_diff_exit(kz, fz, fz), 4);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 3) {  // test_trace_occ_basis_cli
    test_cli(argv[1], argv[2]);
    return kztest::finish("test_trace_occ_basis_cli");
  }
  test_return_derived();
  test_store_derived();
  test_control_order();
  test_old_container();
  test_keys();
  return kztest::finish("test_trace_occ_basis");
}

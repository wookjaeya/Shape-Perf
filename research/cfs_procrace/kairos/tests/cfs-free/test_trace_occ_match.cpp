// The pairing rules of `kz occ diff` after the full key (occ_table.hpp, docs/replay_output.md §3-§4): small synthetic
// tables built directly, the comparison is what is tested.  Tool tests: every value is a unit-test value, not a research
// condition and not a case (CONDITIONS_POLICY).  The first two shapes follow the comparisons that showed the defect
// (plan/results_w4w5_20261028.md §2.3: an application's 4th occurrence left on one side, another application's 4th on
// the other; two such keys, the 7th and 8th); the rows here are invented.
//
//   - two established owners that differ are never paired: such candidates are listed as "owners differ" and their rows
//     stay one side only; one row against two rows of two other established applications is not ambiguous; the same
//     application with two different task places is not paired either;
//   - one side's owner not established (undetermined, none recorded), or the task's place established on one side
//     only: paired, listed and counted apart from the full-key pairs, a verdict difference counted apart too;
//   - the ordinal alone: one side's owner not established, but the role, the via names or the text arguments differ:
//     uncertain, not paired (exit 4); one row with two candidates: ambiguous, not paired (exit 4);
//   - the full key as before: the same pairs, counts and difference lines; a candidate refused for its owner can still
//     be paired by the text arguments (the third pass).

#include <string>
#include <vector>

#include "kz/trace/occ_table.hpp"
#include "support/kz_test.hpp"

namespace {

using kz::trace::OccDiffCounts;
using kz::trace::OccRow;
using kz::trace::OccTable;

bool has(const std::string& text, const std::string& fragment) { return text.find(fragment) != std::string::npos; }

// A row of a container written with revision 5; owner KZ_NONE32 = none recorded (the writer's undetermined owner).
OccRow row(uint32_t index, uint32_t owner, uint8_t level, uint16_t task, uint32_t ordinal, uint8_t status = KZ_OCC_READY) {
  OccRow r;
  r.index = index;
  r.recorded = true;
  r.task_recorded = true;
  r.rspec = "K-ID";
  r.op = "get_id";
  r.owner_key = owner;
  r.owner_level = owner == KZ_NONE32 ? static_cast<uint8_t>(KZ_OWNER_LEVEL_UNDETERMINED) : level;
  r.task = owner == KZ_NONE32 ? 0 : task;
  r.resource = "0x10";
  r.name = "id=0";
  r.ordinal = ordinal;
  r.status = status;
  r.observation = KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX;
  return r;
}
OccRow confirmed(uint32_t index, uint32_t owner, uint32_t ordinal, uint8_t status = KZ_OCC_READY) { return row(index, owner, KZ_OWNER_LEVEL_CONFIRMED, 1, ordinal, status); }
OccRow undetermined(uint32_t index, uint32_t ordinal, uint8_t status = KZ_OCC_READY) { return row(index, KZ_NONE32, 0, 0, ordinal, status); }

OccTable table(std::vector<OccRow> rows, bool task_recorded = true) {
  OccTable t;
  t.recorded = true;
  t.task_recorded = task_recorded;
  for (OccRow& r : rows) r.task_recorded = task_recorded;
  t.rows = std::move(rows);
  return t;
}

// rows of `owner`, ordinals 1..k, indices from `first`
void add(std::vector<OccRow>* rows, uint32_t owner, uint32_t k) {
  for (uint32_t i = 1; i <= k; ++i) rows->push_back(confirmed(static_cast<uint32_t>(rows->size()), owner, i));
}

void test_owners_differ() {
  OccDiffCounts n;
  // application 13 has 4 occurrences in A and 3 in B, application 6 has 3 in A and 4 in B: the 4th of 13 (A) and the
  // 4th of 6 (B) share the key without the owner.  Before: paired "without the owner".  Now: not paired.
  std::vector<OccRow> ra, rb;
  add(&ra, 13, 4);
  add(&ra, 6, 3);
  add(&rb, 13, 3);
  add(&rb, 6, 4);
  std::string d = kz::trace::occ_diff_text(table(ra), table(rb), &n);
  KZ_CHECK(n.matched == 6 && n.by_key == 6 && n.owner_unestablished == 0 && n.owner_conflict == 1 && n.only_a == 1 && n.only_b == 1);
  KZ_CHECK(n.ambiguous == 0 && n.uncertain == 0 && n.verdict == 0 && n.basis == 0 && kz::trace::occ_diff_status(n) == 3);
  KZ_CHECK(has(d, "owners differ 1 (not paired)") && has(d, "result: exit 3 (differences)"));
  KZ_CHECK(has(d, "K-ID | get_id | 0x10 | 4 (without the owner): A#3 owner 13 confirmed task 1 READY") && has(d, "B#6 owner 6 confirmed task 1 READY"));
  KZ_CHECK(has(d, "only in A: K-ID | get_id | app[13]#1 | 0x10 | 4 READY") && has(d, "(not paired with B#6: the owners differ)"));
  KZ_CHECK(!has(d, "matched with one side's owner not established"));
  // two such keys (the 7th and 8th of application 15 in A, of application 20 in B): two candidates refused
  ra.clear();
  rb.clear();
  add(&ra, 15, 8);
  add(&ra, 20, 6);
  add(&rb, 15, 6);
  add(&rb, 20, 8);
  kz::trace::occ_diff_text(table(ra), table(rb), &n);
  KZ_CHECK(n.matched == 12 && n.by_key == 12 && n.owner_conflict == 2 && n.only_a == 2 && n.only_b == 2 && n.owner_unestablished == 0 && n.ambiguous == 0);
  // one row of application 6 against rows of applications 13 and 14 (all established): no candidate, so not ambiguous
  d = kz::trace::occ_diff_text(table({confirmed(0, 6, 4)}), table({confirmed(0, 13, 4), confirmed(1, 14, 4)}), &n);
  KZ_CHECK(n.matched == 0 && n.ambiguous == 0 && n.owner_conflict == 2 && n.only_a == 1 && n.only_b == 2 && kz::trace::occ_diff_status(n) == 3);
  KZ_CHECK(has(d, "(not paired with B#0 B#1: the owners differ)"));
  // inferred against confirmed is still established on both sides
  kz::trace::occ_diff_text(table({row(0, 13, KZ_OWNER_LEVEL_INFERRED, 1, 4)}), table({confirmed(0, 6, 4)}), &n);
  KZ_CHECK(n.matched == 0 && n.owner_conflict == 1);
  // the same application, the task's place 1 in A and 2 in B (both containers record it): not paired
  d = kz::trace::occ_diff_text(table({row(0, 9, KZ_OWNER_LEVEL_CONFIRMED, 1, 1)}), table({row(0, 9, KZ_OWNER_LEVEL_CONFIRMED, 2, 1)}), &n);
  KZ_CHECK(n.matched == 0 && n.owner_conflict == 1 && n.only_a == 1 && n.only_b == 1 && has(d, "owner 9 confirmed task 1") && has(d, "owner 9 confirmed task 2"));
  // where B does not record the task, the key has no task: one full-key pair, as before
  kz::trace::occ_diff_text(table({row(0, 9, KZ_OWNER_LEVEL_CONFIRMED, 1, 1)}), table({row(0, 9, KZ_OWNER_LEVEL_CONFIRMED, 2, 1)}, false), &n);
  KZ_CHECK(n.matched == 1 && n.by_key == 1 && n.owner_conflict == 0);
}

void test_owner_not_established() {
  OccDiffCounts n;
  // the 4th of application 13 in A, an occurrence with no owner recorded in B, everything else equal: paired apart
  std::string d = kz::trace::occ_diff_text(table({confirmed(0, 13, 4, KZ_OCC_READY)}), table({undetermined(0, 4, KZ_OCC_NOT_READY_UNJUDGED)}), &n);
  KZ_CHECK(n.matched == 1 && n.by_key == 0 && n.owner_unestablished == 1 && n.owner_conflict == 0 && n.only_a == 0 && n.only_b == 0);
  KZ_CHECK(n.verdict == 0 && n.verdict_other == 1 && n.basis_other == 0 && kz::trace::occ_diff_status(n) == 3);
  KZ_CHECK(has(d, ", matched with one side's owner not established 1") && has(d, "pairs: by the full key 0, with one side's owner not established 1, by the text arguments 0"));
  KZ_CHECK(has(d, "verdict differences (status, summary view): 0\n"));
  KZ_CHECK(has(d, "matched with one side's owner not established (") && has(d, "): 1; verdict differences 1, basis differences 0"));
  KZ_CHECK(has(d, "K-ID | get_id | app[13]#1 | 0x10 | 4 ~ B K-ID | get_id | app:undetermined | 0x10 | 4 (the owner established in A only; A owner 13 confirmed task 1, "
                  "B owner not established (undetermined, none recorded)): verdict A READY"));
  // the other way round, verdicts equal
  d = kz::trace::occ_diff_text(table({undetermined(0, 4)}), table({confirmed(0, 13, 4)}), &n);
  KZ_CHECK(n.matched == 1 && n.owner_unestablished == 1 && n.verdict_other == 0 && has(d, "(the owner established in B only;") && has(d, "verdict same; basis same"));
  // the same application, the task's place established in A only: paired, said so
  d = kz::trace::occ_diff_text(table({row(0, 9, KZ_OWNER_LEVEL_CONFIRMED, 1, 2)}), table({row(0, 9, KZ_OWNER_LEVEL_CONFIRMED, 0, 2)}), &n);
  KZ_CHECK(n.matched == 1 && n.owner_unestablished == 1 && has(d, "(the task's place in application 9 established in A only;"));
  // A: rows of applications 13 and of no owner; B: a row of application 6.  Only the row without an owner is a
  // candidate: paired; the row of 13 stays one side only, its refused candidate named (B's row is paired, so no
  // "owners differ" entry)
  d = kz::trace::occ_diff_text(table({confirmed(0, 13, 4), undetermined(1, 4)}), table({confirmed(0, 6, 4)}), &n);
  KZ_CHECK(n.matched == 1 && n.owner_unestablished == 1 && n.only_a == 1 && n.only_b == 0 && n.owner_conflict == 0 && n.ambiguous == 0);
  KZ_CHECK(has(d, "only in A: K-ID | get_id | app[13]#1 | 0x10 | 4 READY ") && has(d, " (not paired with B#0: the owners differ)\n"));
}

void test_ordinal_only() {
  OccDiffCounts n;
  // one side's owner not established and the key without the owner equal, but the role differs: the ordinal alone ties
  // them, so they are uncertain and not paired
  OccRow x = confirmed(0, 13, 4);
  OccRow y = undetermined(0, 4);
  x.context = "caller";
  y.context = "provider";
  std::string d = kz::trace::occ_diff_text(table({x}), table({y}), &n);
  KZ_CHECK(n.matched == 0 && n.uncertain == 1 && n.uncertain_rows == 2 && n.only_a == 0 && n.only_b == 0 && kz::trace::occ_diff_status(n) == 4);
  KZ_CHECK(has(d, "uncertain 1 (2 occurrences, not paired)") && has(d, "result: exit 4 (the pairing of some occurrences is not established: ambiguous keys 0, uncertain 1)"));
  KZ_CHECK(has(d, "K-ID | get_id | 0x10 | 4 (without the owner): A#0 owner 13 confirmed task 1, B#0 owner not established (undetermined, none recorded); role A {caller} B {provider};"));
  // the text arguments differ (the key value fixed and equal): uncertain
  x = confirmed(0, 13, 4);
  y = undetermined(0, 4);
  y.name = "id=1";
  d = kz::trace::occ_diff_text(table({x}), table({y}), &n);
  KZ_CHECK(n.matched == 0 && n.uncertain == 1 && has(d, "args A {id=0} B {id=1};"));
  // the via names differ: uncertain
  y = undetermined(0, 4);
  y.via = "v1";
  d = kz::trace::occ_diff_text(table({x}), table({y}), &n);
  KZ_CHECK(n.matched == 0 && n.uncertain == 1 && has(d, "via A {} B {v1};"));
  // a row without an owner and two candidates (applications 13 and 6): which is which is not established
  d = kz::trace::occ_diff_text(table({undetermined(0, 4)}), table({confirmed(0, 13, 4), confirmed(1, 6, 4)}), &n);
  KZ_CHECK(n.matched == 0 && n.ambiguous == 1 && n.ambiguous_rows == 3 && n.only_a == 0 && n.only_b == 0 && kz::trace::occ_diff_status(n) == 4);
  KZ_CHECK(has(d, "K-ID | get_id | 0x10 | 4 (without the owner): A#0 READY ") && has(d, " owner not established (undetermined, none recorded); B#0 READY "));
  KZ_CHECK(has(d, " owner 13 confirmed task 1; B#1 READY ") && has(d, " owner 6 confirmed task 1;\n"));
}

void test_full_key_unchanged() {
  OccDiffCounts n;
  // full-key pairs: one verdict difference, one basis difference, and a key two occurrences of A share
  std::vector<OccRow> ra{confirmed(0, 9, 1, KZ_OCC_READY), confirmed(1, 9, 2, KZ_OCC_READY), confirmed(2, 9, 3, KZ_OCC_READY), confirmed(3, 9, 5), confirmed(4, 9, 5)};
  std::vector<OccRow> rb{confirmed(0, 9, 1, KZ_OCC_NOT_READY_UNJUDGED), confirmed(1, 9, 2, KZ_OCC_READY), confirmed(2, 9, 3, KZ_OCC_READY), confirmed(3, 9, 5)};
  ra[1].basis = KZ_OCC_BASIS_STATE_ORDER;
  rb[1].basis = KZ_OCC_BASIS_STATE_ORDER | KZ_OCC_BASIS_CONTROL_ORDER;
  const std::string d = kz::trace::occ_diff_text(table(ra), table(rb), &n);
  KZ_CHECK(n.matched == 3 && n.by_key == 3 && n.verdict == 1 && n.basis == 1 && n.ambiguous == 1 && n.ambiguous_rows == 3 && n.only_a == 0 && n.only_b == 0);
  KZ_CHECK(n.owner_unestablished == 0 && n.key_differs == 0 && n.uncertain == 0 && n.owner_conflict == 0 && kz::trace::occ_diff_status(n) == 4);
  KZ_CHECK(has(d, "occurrences A 5, B 4; matched 3, only in A 0, only in B 0, ambiguous keys 1 (3 occurrences, not paired)\n"));
  KZ_CHECK(has(d, "verdict differences (status, summary view): 1\n  K-ID | get_id | app[9]#1 | 0x10 | 1: A READY"));
  KZ_CHECK(has(d, "basis differences (basis kinds, assumptions, outcome check, edge rules): 1\n  K-ID | get_id | app[9]#1 | 0x10 | 2: basis A {state-order} B {state-order,control-order};"));
  KZ_CHECK(has(d, "ambiguous keys (shared by more than one occurrence of a side; which is which is not established, none paired): 1\n  K-ID | get_id | app[9]#1 | 0x10 | 5: A#3 "));
  // the same table: no difference, exit 0
  kz::trace::occ_diff_text(table(rb), table(rb), &n);
  KZ_CHECK(n.matched == 4 && n.by_key == 4 && !n.any() && kz::trace::occ_diff_status(n) == 0);
  // a candidate refused for its owner can still be paired by the text arguments: A's row of application 13 fixed the key
  // value, B's row of 13 did not; B's row of application 6 with the same key value and ordinal stays one side only
  OccRow a13 = confirmed(0, 13, 4);
  a13.name = "name=S";
  OccRow b6 = confirmed(0, 6, 4);
  OccRow b13 = confirmed(1, 13, 2);
  b13.resource.clear();
  b13.name = "name=S";
  const std::string e = kz::trace::occ_diff_text(table({a13}), table({b6, b13}), &n);
  KZ_CHECK(n.matched == 1 && n.key_differs == 1 && n.owner_conflict == 0 && n.only_a == 0 && n.only_b == 1 && kz::trace::occ_diff_status(n) == 3);
  KZ_CHECK(has(e, "(key value fixed in A only): verdict same; basis same") && has(e, "only in B: K-ID | get_id | app[6]#1 | 0x10 | 4 READY "));
  KZ_CHECK(has(e, " (not paired with A#0: the owners differ)\n"));
}

}  // namespace

int main() {
  test_owners_differ();
  test_owner_not_established();
  test_ordinal_only();
  test_full_key_unchanged();
  return kztest::finish("test_trace_occ_match");
}

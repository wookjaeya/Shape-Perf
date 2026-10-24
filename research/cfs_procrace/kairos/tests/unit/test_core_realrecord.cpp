// The real-record replay (include/kz/core/eval_trace.hpp, RecordKind::Real; SPEC 4.8 "Explanation set",
// "Replay convention for real records" OB-05, ADR-055; docs/phi_reference.md section 7.6) on the mini
// profile of tests/unit/support/phi_mini.hpp.  Unit-test values only: no value here is a research
// condition.
//
//   - with everything the record must establish declared, a real record gives what the model trace gives;
//   - an unobserved resource is unknown, not in its initial state (no initial-state default);
//   - recorded writes fix a kind only under the conditions of ADR-055 (writers, aliases, start state,
//     lifetime, observed writer points, no loss);
//   - PENDING and OPEN are INCONCLUSIVE(horizon-open) (D2), NOT_READY_UNJUDGED is
//     INCONCLUSIVE(caller_requirement_absent), NOT_EVALUATED INCONCLUSIVE(not_evaluated);
//   - the returned code fixes what the record leaves open (O4), and a code that contradicts what every
//     explanation fixes is an analysis input error (an empty explanation set), never a violation;
//   - the merged order of different actors' events is not the execution order (SPEC 5.8);
//   - derived records (an event lost, a point dropped, a late start) are never more decisive;
//   - an undetermined owner, an unobserved point, the text form;
//   - the execution relation (SPEC 1.1) between the use and the registrations of its key: same task, same
//     application, different applications (core status established or not), the creating executor, an
//     inferred owner, an undetermined owner on either side, no registration, the identity value of an owner;
//   - the alternative of the pending PI decision P1 (no outcome narrowing);
//   - a value a capture read from a named location (`cell`): the actors that wrote that location (a
//     publication site, a store effect) are actors of the relation; a write to an unnamed location leaves it open;
//   - the observation an occurrence rests on (probes, unobserved) and the static closure in the text form.

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/verdict.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::eval::OccStatus;
using kz::eval::Occurrence;
using kz::eval::Result;
using kz::eval::Trace;
using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::SourceText;

Profile load(const std::vector<SourceText>& files) {
  Profile p;
  std::vector<Diag> diags;
  if (!kz::phi::load_profile(files, &p, &diags).ok()) {
    for (const Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
    ++kztest::state().failures;
  }
  return p;
}

Profile load_edited(const char* from, const char* to) {
  std::vector<SourceText> f = kzmini::files();
  std::string& t = f[2].text;
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
  std::string dump;
  const Occurrence& occ(size_t i = 0) const {
    static const Occurrence none;
    return i < result.occurrences.size() ? result.occurrences[i] : none;
  }
  std::string summary(size_t i = 0) const { return kz::render_observation(occ(i).summary); }
  bool finding(const std::string& fragment) const {
    for (const kz::eval::ReplayFinding& f : result.findings)
      if (f.what.find(fragment) != std::string::npos) return true;
    return false;
  }
};

Run replay(const Profile& p, const std::string& text, const kz::eval::Options& options = kz::eval::Options()) {
  Run r;
  const kz::Status ps = kz::eval::parse_trace_text(text, &r.trace);
  if (!ps.ok()) {
    std::fprintf(stderr, "trace does not parse: %s\n", ps.describe().c_str());
    ++kztest::state().failures;
  }
  const kz::Status s = kz::eval::replay(p, r.trace, options, &r.result);
  KZ_CHECK(s.ok());
  r.dump = kz::eval::dump_result(p, r.trace, r.result);
  return r;
}

void check_status(const Run& r, OccStatus want, const char* what) {
  const bool ok = !r.result.occurrences.empty() && r.occ().status == want;
  KZ_CHECK(ok);
  if (!ok) std::fprintf(stderr, "%s: status %s, wanted %s\n%s\n", what, r.result.occurrences.empty() ? "(none)" : kz::eval::occ_status_text(r.occ().status),
                        kz::eval::occ_status_text(want), r.dump.c_str());
}

void check_summary(const Run& r, const std::string& want, const char* what) {
  const std::string got = r.summary();
  KZ_CHECK_EQ(got, want);
  if (got != want) std::fprintf(stderr, "%s:\n%s\n", what, r.dump.c_str());
  KZ_CHECK(kz::validate_summary(r.occ().summary).empty());
}

bool possible(const Run& r, OccStatus s) {
  for (OccStatus x : r.occ().possible)
    if (x == s) return true;
  return false;
}

// Everything the record must establish for the kinds slot and reg (ADR-055), and the points it observes.
std::string preamble(const std::string& reg = "start=known writers=covered aliases=covered lifetime=known", bool observe_set_reg = true,
                     bool observe_who_reg = true, bool owner = true, bool order = true) {
  std::string t = "record real\nstart_evidence init\n";
  if (order) t += "order established\n";
  t += "adequacy kind slot start=known writers=covered aliases=covered lifetime=known\n";
  t += "adequacy kind reg " + reg + "\n";
  for (const char* pt : {"reset_all.zero_slots", "create_all.reserve", "create_all.typeit", "create_all.typeext", "create_all.publish", "clear_site", "free_up.free",
                         "sign_in.clear_reg"})
    t += std::string("observed ") + pt + "\n";
  if (observe_set_reg) t += "observed sign_in.set_reg\n";
  if (observe_who_reg) t += "observed send.who_reg\n";
  if (owner) t += "owner_level W confirmed\n";
  return t;
}

const char* const kSetup =
    "const TABLE 4\n"
    "owner W 2\n"
    "T0 create_all.reserve pt lp.n=2\n"
    "T0 create_all.publish pt lp.n=2 lp.pid=0x1002\n";

const char* const kRegister =
    "W sign_in call\n"
    "W sign_in.set_reg pt\n"
    "W sign_in ret result=0\n";

std::string send(const char* result) {
  std::string t =
      "W send.who_valid pt arg.Who=0x1002\n"
      "W send call arg.Who=0x1002\n"
      "W send.who_reg pt\n";
  if (result != nullptr) t += std::string("W send ret result=") + result + "\n";
  return t;
}

const char* const kNotReadyB = "0xc0000002";  // not_ready(who_reg)
const char* const kBadArg = "0xc0000003";     // error: says nothing about readiness
const char* const kOk = "0";                  // ready

void test_full_adequacy_agrees_with_model() {
  const Profile p = load(kzmini::files());
  // never registered: the registry conjunct is false at its observed point in every explanation
  const Run real = replay(p, preamble() + kSetup + send(kNotReadyB));
  const Run model = replay(p, std::string(kSetup) + send(kNotReadyB));
  check_status(real, OccStatus::Violating, "full adequacy, never registered");
  check_status(model, OccStatus::Violating, "model trace, never registered");
  KZ_CHECK(real.occ().decided_at == model.occ().decided_at && real.occ().decision == model.occ().decision);
  check_summary(real, "OBSERVED_VIOLATION", "full adequacy, never registered");
  KZ_CHECK(real.occ().possible.empty());
  KZ_CHECK(!real.result.partial);
  // registered before the call: READY, and no violation in the checked prefix
  const Run reg = replay(p, preamble() + kSetup + kRegister + send(kOk));
  check_status(reg, OccStatus::Ready, "full adequacy, registered");
  check_summary(reg, "NO_VIOLATION_IN_CHECKED_PREFIX", "full adequacy, registered");
  KZ_CHECK(reg.dump.find("record real") != std::string::npos);
  KZ_CHECK(model.dump.find("record real") == std::string::npos);
  KZ_CHECK(model.dump.find("summary") == std::string::npos);  // the text of a model trace is unchanged
}

void test_no_initial_state_default() {
  const Profile p = load(kzmini::files());
  // The start state of the registry is not evidenced: the slot is not "unregistered" by default.  The
  // returned code (an error) says nothing about readiness, so nothing else fixes it.
  const std::string reg_unknown = "start=unknown writers=covered aliases=covered lifetime=known";
  const Run real = replay(p, preamble(reg_unknown) + kSetup + send(kBadArg));
  check_status(real, OccStatus::Undetermined, "unknown start state");
  KZ_CHECK(possible(real, OccStatus::Violating) && possible(real, OccStatus::Ready));
  KZ_CHECK((real.occ().reasons & KZ_WITHHOLD_START_STATE_UNKNOWN) != 0);
  KZ_CHECK(real.occ().summary.observation == KZ_OBS_INCONCLUSIVE);
  check_summary(real, "INCONCLUSIVE(start_state_unknown)", "unknown start state");
  // the same events as a complete model trace: the initial-state convention gives VIOLATING
  const Run model = replay(p, std::string(kSetup) + send(kBadArg));
  check_status(model, OccStatus::Violating, "model trace, error code");
  // nothing declared at all (an earlier record, or a plan without the writers): unknown as well
  const Run bare = replay(p, std::string("record real\n") + kSetup + send(kBadArg));
  check_status(bare, OccStatus::Undetermined, "nothing declared");
  KZ_CHECK(bare.occ().summary.observation == KZ_OBS_INCONCLUSIVE);
  KZ_CHECK((bare.occ().reasons & KZ_WITHHOLD_START_STATE_UNKNOWN) != 0);
  KZ_CHECK((bare.occ().reasons & KZ_WITHHOLD_EFFECT_UNSUPPORTED) != 0);
}

void test_recorded_writes_need_adequacy() {
  const Profile p = load(kzmini::files());
  // writers not covered: the recorded registration does not fix the slot (another writer may run)
  const Run w = replay(p, preamble("start=known writers=uncovered aliases=covered lifetime=known") + kSetup + kRegister + send(kBadArg));
  check_status(w, OccStatus::Undetermined, "writers uncovered");
  KZ_CHECK((w.occ().reasons & KZ_WITHHOLD_EFFECT_UNSUPPORTED) != 0);
  KZ_CHECK((w.occ().adequacy & KZ_ADEQ_WRITERS_COVERED) == 0);
  // the model trace of the same events: READY
  check_status(replay(p, std::string(kSetup) + kRegister + send(kBadArg)), OccStatus::Ready, "model trace, registered");
  // writers declared covered, but a writer's point is not observed: said, and not fixed
  const Run unobs = replay(p, preamble("start=known writers=covered aliases=covered lifetime=known", false) + kSetup + kRegister + send(kBadArg));
  check_status(unobs, OccStatus::Undetermined, "writer point not observed");
  KZ_CHECK(unobs.finding("is declared writers=covered, but the point of its writer(s) sign_in.set_reg is not observed"));
  // lifetime unknown
  const Run life = replay(p, preamble("start=known writers=covered aliases=covered lifetime=unknown") + kSetup + kRegister + send(kBadArg));
  check_status(life, OccStatus::Undetermined, "lifetime unknown");
  KZ_CHECK((life.occ().reasons & KZ_WITHHOLD_RESOURCE_OR_LIFETIME_UNMATCHED) != 0);
  // full adequacy: the recorded registration fixes the slot
  const Run full = replay(p, preamble() + kSetup + kRegister + send(kBadArg));
  check_status(full, OccStatus::Ready, "full adequacy, error code");
  // the same, but the order of the two actors' events is the collector's merged order (SPEC 5.8): the
  // identity T0 published may not have been written yet when W registered, so the slot is not fixed
  const Run merged = replay(p, preamble("start=known writers=covered aliases=covered lifetime=known", true, true, true, false) + kSetup + kRegister + send(kBadArg));
  check_status(merged, OccStatus::Undetermined, "merged order");
  KZ_CHECK((merged.occ().reasons & KZ_WITHHOLD_EXPLANATION_AMBIGUOUS) != 0);
  // a loss reported before the call: recorded writes fix nothing from there on
  const Run lost = replay(p, preamble() + kSetup + kRegister + "loss begin\nloss end\n" + send(kBadArg));
  check_status(lost, OccStatus::Undetermined, "loss before the call");
  KZ_CHECK((lost.occ().reasons & KZ_WITHHOLD_LOSS) != 0);
  KZ_CHECK((lost.occ().adequacy & KZ_ADEQ_LOSS_FREE) == 0);
  // a loss after the decision changes nothing that was decided before it
  const Run late = replay(p, preamble() + kSetup + send(kNotReadyB) + "loss begin\n");
  check_status(late, OccStatus::Violating, "loss after the decision");
  check_summary(late, "OBSERVED_VIOLATION", "loss after the decision");
  // a value conjunct over the arguments needs nothing from the state: decided even after a loss
  const Run arg = replay(p, std::string("record real\nloss begin\n") + kSetup +
                                "W send.who_valid pt arg.Who=0xfff\nW send call arg.Who=0xfff\nW send ret result=0xc0000001\n");
  check_status(arg, OccStatus::Violating, "invalid identity, after a loss");
  check_summary(arg, "OBSERVED_VIOLATION", "invalid identity, after a loss");
}

void test_outcome_and_input_error() {
  const Profile p = load(kzmini::files());
  const std::string reg_unknown = "start=unknown writers=covered aliases=covered lifetime=known";
  // The returned code says the registry conjunct was reached and false (O4): fixed by the code.
  const Run by_code = replay(p, preamble(reg_unknown) + kSetup + send(kNotReadyB));
  check_status(by_code, OccStatus::Violating, "fixed by the returned code");
  KZ_CHECK(by_code.occ().decided_at == kz::eval::DecidedAt::Return);
  KZ_CHECK(by_code.occ().outcome_narrowed);
  KZ_CHECK((by_code.occ().summary.flags & KZ_SUMMARY_FLAG_OUTCOME_NARROWED) != 0);
  check_summary(by_code, "OBSERVED_VIOLATION", "fixed by the returned code");
  // Empty explanation set (Qv and Qn both UNSAT): the slot is registered in every explanation, the code
  // says it was not.  An analysis input error: never a violation, never no violation.
  const Run contra = replay(p, preamble() + kSetup + kRegister + send(kNotReadyB));
  KZ_CHECK(contra.occ().input_error);
  KZ_CHECK(contra.result.partial);
  KZ_CHECK(contra.occ().summary.observation == KZ_OBS_NONE);
  KZ_CHECK((contra.occ().summary.flags & KZ_SUMMARY_FLAG_INPUT_ERROR) != 0);
  check_summary(contra, "analysis input error (reported apart)", "registered, code says not registered");
  KZ_CHECK(contra.finding("analysis input error"));
  // the other direction: unregistered in every explanation, the code says ready
  const Run contra2 = replay(p, preamble() + kSetup + send(kOk));
  KZ_CHECK(contra2.occ().input_error);
  KZ_CHECK(contra2.occ().summary.observation == KZ_OBS_NONE);
  KZ_CHECK(contra2.occ().summary.observation != KZ_OBS_OBSERVED_VIOLATION);
  // the model trace of the second one keeps its old reading (a finding; the status from the state)
  const Run model = replay(p, std::string(kSetup) + send(kOk));
  check_status(model, OccStatus::Violating, "model trace, code disagrees");
  KZ_CHECK(!model.occ().input_error);
}

void test_horizon_open_and_caller_requirement() {
  // PENDING: a caller-level Allowed entry judges it, and no monitor has accepted it (D2)
  const Profile pend = load_edited("allowed none;", "allowed { documented_drop \"caller text\" prov doc_call; }");
  const Run pending = replay(pend, preamble() + kSetup + send(kNotReadyB));
  check_status(pending, OccStatus::Pending, "pending");
  check_summary(pending, "INCONCLUSIVE(horizon-open)", "pending");
  const Run pending_model = replay(pend, std::string(kSetup) + send(kNotReadyB));
  check_status(pending_model, OccStatus::Pending, "pending, model trace");
  KZ_CHECK_EQ(pending_model.summary(), std::string("INCONCLUSIVE(horizon-open)"));  // computed for a model trace too (not printed)
  // OPEN: the record ends before the decision
  const Profile p = load(kzmini::files());
  const Run open = replay(p, preamble() + kSetup + "W send.who_valid pt arg.Who=0x1002\nW send call arg.Who=0x1002\n");
  check_status(open, OccStatus::Open, "open");
  check_summary(open, "INCONCLUSIVE(horizon-open)", "open");
  // NOT_READY_UNJUDGED: the caller-level requirement is absent
  const Profile unj = load_edited("allowed none;", "allowed unspecified_by_reference;");
  const Run unjudged = replay(unj, preamble() + kSetup + send(kNotReadyB));
  check_status(unjudged, OccStatus::NotReadyUnjudged, "unjudged");
  check_summary(unjudged, "INCONCLUSIVE(caller_requirement_absent)", "unjudged");
  // NOT_EVALUATED: the call returned before the observed point of the registry conjunct
  const Run early = replay(p, preamble() + kSetup + "W send.who_valid pt arg.Who=0x1002\nW send call arg.Who=0x1002\nW send ret result=0xc0000003\n");
  check_status(early, OccStatus::NotEvaluated, "early return");
  check_summary(early, "INCONCLUSIVE(not_evaluated)", "early return");
}

void test_unobserved_point_and_owner() {
  const Profile p = load(kzmini::files());
  // The registry conjunct's point is not observed: read over the whole call; whether it was passed is
  // not known, so READY and NOT_EVALUATED both remain.
  const Run interval = replay(p, preamble("start=known writers=covered aliases=covered lifetime=known", true, false) + kSetup + kRegister +
                                     "W send.who_valid pt arg.Who=0x1002\nW send call arg.Who=0x1002\nW send ret result=0\n");
  check_status(interval, OccStatus::Undetermined, "unobserved point");
  KZ_CHECK(possible(interval, OccStatus::Ready) && possible(interval, OccStatus::NotEvaluated));
  KZ_CHECK(!possible(interval, OccStatus::Violating));
  KZ_CHECK((interval.occ().reasons & KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS) != 0);
  KZ_CHECK(interval.dump.find("conjunct who_reg interval") != std::string::npos);
  // The owner of the registering task is undetermined: the registration may have hit any slot
  const Run owner = replay(p, preamble("start=known writers=covered aliases=covered lifetime=known", true, true, false) + kSetup + kRegister + send(kBadArg));
  check_status(owner, OccStatus::Undetermined, "owner undetermined");
  KZ_CHECK((owner.occ().reasons & KZ_WITHHOLD_OWNERSHIP_UNDETERMINED) != 0);
  KZ_CHECK_EQ(static_cast<int>(owner.occ().owner_level), static_cast<int>(KZ_OWNER_LEVEL_UNDETERMINED));
  // identities: the passed identifier is the stamp parameter, the owner as the record states it
  const Run full = replay(p, preamble() + "creator W T0\n" + kSetup + send(kNotReadyB));
  KZ_CHECK(full.occ().has_passed && full.occ().passed == 0x1002 && full.occ().passed_role == "Who");
  KZ_CHECK_EQ(full.occ().creator, std::string("T0"));
  KZ_CHECK(full.occ().has_owner_key && full.occ().owner_key == std::vector<int64_t>{2});
  KZ_CHECK(full.dump.find("identity exec W creator T0 owner confirmed [2] passed Who=0x1002") != std::string::npos);
}

void test_text_form() {
  const Profile p = load(kzmini::files());
  // a real-record line without `record real` is refused
  Trace t;
  KZ_CHECK(!kz::eval::parse_trace_text("adequacy kind reg start=known\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nloss end\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nadequacy loc Box.id lifetime=known\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nowner_level W sure\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nowner_id W x\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nowner_id W 0x100000000\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nowner_core W maybe\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("owner_core W yes\n", &t).ok());
  // the text form reads back to the same trace
  const std::string text = preamble() + "creator W T0\nowner_id W 0x1002\nowner_core W no\n" + kSetup + kRegister + "loss begin\n" + send(kOk) +
                           "loss end\nW send call arg.Who=0x1002\n";
  const Run a = replay(p, text);
  const Run b = replay(p, kz::eval::trace_text(a.trace));
  KZ_CHECK_EQ(a.dump, b.dump);
  KZ_CHECK_EQ(kz::eval::trace_text(a.trace), kz::eval::trace_text(b.trace));
  KZ_CHECK_EQ(a.trace.losses.size(), static_cast<size_t>(1));
  KZ_CHECK(b.trace.owner_ids.count("W") == 1 && b.trace.owner_ids.at("W") == 0x1002u);
  KZ_CHECK(b.trace.owner_core.count("W") == 1 && !b.trace.owner_core.at("W"));
}

// A send of the registered identity 0x1002 by `actor`.
std::string send_by(const std::string& actor, const char* result) {
  return actor + " send.who_valid pt arg.Who=0x1002\n" + actor + " send call arg.Who=0x1002\n" + actor + " send.who_reg pt\n" + actor + " send ret result=" + result + "\n";
}

void check_relation(const Run& r, uint8_t want, const std::string& note, const char* what) {
  const bool ok = r.occ().relation == want && r.occ().summary.relation == want && r.occ().relation_note.find(note) != std::string::npos;
  KZ_CHECK(ok);
  if (!ok) std::fprintf(stderr, "%s: relation %u (%s), wanted %u (%s)\n%s\n", what, r.occ().relation, r.occ().relation_note.c_str(), want, note.c_str(), r.dump.c_str());
}

// The execution relation (SPEC 1.1 "Execution relations", 1.10 item 10, ADR-056): between the executing task
// of the use and the actors whose effects registered the occurrence's key, with their app_owner.
void test_execution_relation() {
  const Profile p = load(kzmini::files());
  const uint16_t inferred = KZ_SUMMARY_FLAG_RELATION_INFERRED, core_unknown = KZ_SUMMARY_FLAG_CORE_STATUS_UNKNOWN;
  // same task: W registers its own slot and sends with its own identity
  const Run same = replay(p, preamble() + kSetup + kRegister + send(kOk));
  check_relation(same, KZ_EXEC_RELATION_INTRA_APPLICATION, "same task", "same task");
  KZ_CHECK(same.occ().counterparts == std::vector<std::string>{"W"});
  KZ_CHECK_EQ(static_cast<int>(same.occ().relation_level), static_cast<int>(KZ_OWNER_LEVEL_CONFIRMED));
  KZ_CHECK((same.occ().summary.flags & (inferred | core_unknown)) == 0);
  KZ_CHECK(same.dump.find("relation intra-application owners confirmed: same task (W)") != std::string::npos);
  // same application, another task: V belongs to the application W registered
  const std::string v = "owner V 2\nowner_level V confirmed\n";
  const Run app = replay(p, preamble() + v + kSetup + kRegister + send_by("V", kOk));
  check_relation(app, KZ_EXEC_RELATION_INTRA_APPLICATION, "same application", "same application");
  check_summary(app, "NO_VIOLATION_IN_CHECKED_PREFIX", "same application");
  // different applications: X belongs to slot 3 and sends with W's identity
  const std::string x = "owner X 3\nowner_level X confirmed\n";
  const Run inter = replay(p, preamble() + x + kSetup + kRegister + send_by("X", kOk));
  check_relation(inter, KZ_EXEC_RELATION_INTER_APPLICATION, "core status not established", "different applications");
  KZ_CHECK((inter.occ().summary.flags & core_unknown) != 0);
  const Run core = replay(p, preamble() + x + "owner_core W yes\n" + kSetup + kRegister + send_by("X", kOk));
  check_relation(core, KZ_EXEC_RELATION_CORE_APPLICATION, "core service", "a core service");
  KZ_CHECK((core.occ().summary.flags & core_unknown) == 0);
  const Run neither = replay(p, preamble() + x + "owner_core W no\nowner_core X no\n" + kSetup + kRegister + send_by("X", kOk));
  check_relation(neither, KZ_EXEC_RELATION_INTER_APPLICATION, "neither a core service", "neither core");
  KZ_CHECK((neither.occ().summary.flags & core_unknown) == 0);
  // the creating executor: W created X
  const Run created = replay(p, preamble() + x + "creator X W\n" + kSetup + kRegister + send_by("X", kOk));
  check_relation(created, KZ_EXEC_RELATION_CREATION_FRAMEWORK, "one created the other", "creator");
  // an inferred owner is evidence, and the summary carries the level
  const Run inf = replay(p, preamble() + "owner X 3\nowner_level X inferred\n" + kSetup + kRegister + send_by("X", kOk));
  check_relation(inf, KZ_EXEC_RELATION_INTER_APPLICATION, "different applications", "inferred owner");
  KZ_CHECK_EQ(static_cast<int>(inf.occ().relation_level), static_cast<int>(KZ_OWNER_LEVEL_INFERRED));
  KZ_CHECK((inf.occ().summary.flags & inferred) != 0);
  // undetermined stays undetermined: the user's owner, a registrar's owner, no registration at all
  const Run user = replay(p, preamble() + "owner X 3\n" + kSetup + kRegister + send_by("X", kOk));
  check_relation(user, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "the owner of the executing task X is undetermined", "user undetermined");
  KZ_CHECK((user.occ().summary.flags & inferred) == 0);
  const Run writer = replay(p, preamble() + kSetup + kRegister + "Y sign_in call\nY sign_in.set_reg pt\nY sign_in ret result=0\n" + send(kOk));
  check_relation(writer, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "a write of reg by Y whose target the record does not establish", "registrar undetermined");
  const Run none = replay(p, preamble() + kSetup + send(kNotReadyB));
  check_relation(none, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "no registration or publication of reg[0x1002]", "never registered");
  check_summary(none, "OBSERVED_VIOLATION", "never registered");  // the violation and the undetermined relation are reported apart
  // The collector's merged order leaves the slot's identity open (T0 wrote it, and no edge orders T0 with W), so
  // the registration's target is not fixed by the state; the identity value the record gives for W's confirmed
  // owner names it.  With Options::owner_identity_targets (the default since 2026-10-24) it also fixes the
  // registration's target: reg[0x1002] is registered by W and W's own send reads it in program order, so the
  // occurrence is READY from the state, and its condition says that it rests on the writer's identity.  Without
  // the option the state stays open (the reading before): the relation is computed, the observation is not made
  // more decisive.
  const std::string merged = preamble("start=known writers=covered aliases=covered lifetime=known", true, true, true, false);
  const Run open = replay(p, merged + kSetup + kRegister + send(kBadArg));
  check_relation(open, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "whose target the record does not establish", "merged order, no identity value");
  const Run named = replay(p, merged + "owner_id W 0x1002\n" + kSetup + kRegister + send(kBadArg));
  check_relation(named, KZ_EXEC_RELATION_INTRA_APPLICATION, "same task", "merged order, identity value");
  check_status(named, OccStatus::Ready, "merged order, identity value");
  KZ_CHECK(named.dump.find("owner-identity") != std::string::npos);
  KZ_CHECK((named.occ().summary.flags & KZ_SUMMARY_FLAG_OUTCOME_NARROWED) == 0);
  kz::eval::Options no_identity;
  no_identity.owner_identity_targets = false;
  const Run named_off = replay(p, merged + "owner_id W 0x1002\n" + kSetup + kRegister + send(kBadArg), no_identity);
  check_relation(named_off, KZ_EXEC_RELATION_INTRA_APPLICATION, "same task", "merged order, identity value, option off");
  check_status(named_off, OccStatus::Undetermined, "merged order, identity value, option off");
  KZ_CHECK_EQ(named_off.summary(), open.summary());
  KZ_CHECK(named_off.dump.find("owner-identity") == std::string::npos);
  // an inferred owner's identity value fixes nothing
  const Run inferred_id = replay(p, preamble("start=known writers=covered aliases=covered lifetime=known", true, true, false, false) + "owner_level W inferred\nowner_id W 0x1002\n" +
                                        kSetup + kRegister + send(kBadArg));
  check_status(inferred_id, OccStatus::Undetermined, "merged order, inferred owner's identity value");
  // a model trace computes no relation and prints none
  const Run model = replay(p, std::string(kSetup) + kRegister + send(kOk));
  KZ_CHECK_EQ(static_cast<int>(model.occ().relation), static_cast<int>(KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED));
  KZ_CHECK(model.dump.find("relation") == std::string::npos);
}

// P1 of STATUS_W0W3_20261024 (pending PI decision): the returned code fixes what the record leaves open
// (current behaviour).  The alternative fixes nothing by the code; a code that contradicts what every
// explanation fixes is still an analysis input error.
void test_outcome_narrowing_alternative() {
  const Profile p = load(kzmini::files());
  kz::eval::Options off;
  off.outcome_narrowing = false;
  const std::string reg_unknown = "start=unknown writers=covered aliases=covered lifetime=known";
  const Run on = replay(p, preamble(reg_unknown) + kSetup + send(kNotReadyB));
  const Run alt = replay(p, preamble(reg_unknown) + kSetup + send(kNotReadyB), off);
  check_summary(on, "OBSERVED_VIOLATION", "narrowing on");
  check_status(alt, OccStatus::Undetermined, "narrowing off");
  KZ_CHECK(!alt.occ().outcome_narrowed);
  KZ_CHECK((alt.occ().summary.flags & KZ_SUMMARY_FLAG_OUTCOME_NARROWED) == 0);
  KZ_CHECK(alt.occ().summary.observation == KZ_OBS_INCONCLUSIVE);
  KZ_CHECK((alt.occ().reasons & KZ_WITHHOLD_START_STATE_UNKNOWN) != 0);
  // ready by the code: narrowed to READY, or left open
  const Run on_ok = replay(p, preamble(reg_unknown) + kSetup + send(kOk));
  const Run alt_ok = replay(p, preamble(reg_unknown) + kSetup + send(kOk), off);
  check_summary(on_ok, "NO_VIOLATION_IN_CHECKED_PREFIX", "ready by the code, narrowing on");
  KZ_CHECK(alt_ok.occ().summary.observation == KZ_OBS_INCONCLUSIVE);
  KZ_CHECK(kz::validate_summary(alt_ok.occ().summary).empty());
  // what the record fixes is unchanged, and so is an input error
  const Run fixed = replay(p, preamble() + kSetup + kRegister + send(kOk), off);
  check_summary(fixed, "NO_VIOLATION_IN_CHECKED_PREFIX", "fixed by recorded writes, narrowing off");
  const Run contra = replay(p, preamble() + kSetup + kRegister + send(kNotReadyB), off);
  KZ_CHECK(contra.occ().input_error);
  KZ_CHECK(contra.occ().summary.observation == KZ_OBS_NONE);
}

// Derived records (masterplan 2026-10-24 section 9.1, priority 4) of records whose registry is fixed by
// recorded writes: every single event lost (declared), every observed point dropped from the plan, every
// late start.  No derived record may be more decisive than its original; some must be weaker.
struct Derived {
  Trace trace;
  std::vector<uint64_t> orig;
};

Derived derive(const Trace& t, const std::vector<bool>& keep) {
  Derived d;
  d.trace = t;
  d.trace.events.clear();
  d.trace.losses.clear();
  for (size_t i = 0; i < t.events.size(); ++i)
    if (keep[i]) {
      d.trace.add(t.events[i]);
      d.orig.push_back(i);
    }
  return d;
}

bool definite(uint8_t obs) { return obs == KZ_OBS_OBSERVED_VIOLATION || obs == KZ_OBS_NO_VIOLATION_IN_CHECKED_PREFIX; }

void compare(const Profile& p, const Result& base, const Derived& d, long* weaker, long* stronger) {
  Result r;
  KZ_CHECK(kz::eval::replay(p, d.trace, kz::eval::Options(), &r).ok());
  for (const Occurrence& o : r.occurrences) {
    const Occurrence* b = nullptr;
    for (const Occurrence& x : base.occurrences)
      if (x.call == d.orig[o.call]) b = &x;
    KZ_CHECK(b != nullptr);
    if (b == nullptr) continue;
    if (definite(o.summary.observation) && o.summary.observation != b->summary.observation) ++*stronger;
    if (definite(b->summary.observation) && !definite(o.summary.observation)) ++*weaker;
  }
}

void test_derived_records() {
  const Profile p = load(kzmini::files());
  long weaker = 0, stronger = 0, records = 0;
  for (const std::string& text : {preamble() + kSetup + kRegister + send(kBadArg), preamble() + kSetup + send(kBadArg)}) {
    Run base = replay(p, text);
    KZ_CHECK(definite(base.occ().summary.observation));  // READY / VIOLATING from the fixed registry
    const Trace& t = base.trace;
    const size_t n = t.events.size();
    for (size_t i = 0; i < n; ++i) {  // one event lost, the loss declared
      std::vector<bool> keep(n, true);
      keep[i] = false;
      Derived d = derive(t, keep);
      kz::eval::LossInterval l;
      l.begin = i;
      l.end = i;  // events may be missing just before position i of the derived record
      d.trace.losses.push_back(l);
      compare(p, base.result, d, &weaker, &stronger);
      ++records;
    }
    for (const std::string& pt : t.observed_points) {  // an observed point dropped from the plan
      std::vector<bool> keep(n, true);
      for (size_t i = 0; i < n; ++i)
        if (t.events[i].site == pt) keep[i] = false;
      Derived d = derive(t, keep);
      d.trace.observed_points.erase(pt);
      compare(p, base.result, d, &weaker, &stronger);
      ++records;
    }
    for (size_t cut = 1; cut < n; ++cut) {  // a late start: the start state is not evidenced
      std::vector<bool> keep(n, true);
      for (size_t i = 0; i < cut; ++i) keep[i] = false;
      Derived d = derive(t, keep);
      d.trace.start_evidence = KZ_START_EVIDENCE_NONE;
      for (auto& kv : d.trace.kind_adequacy) kv.second.start_known = false;
      compare(p, base.result, d, &weaker, &stronger);
      ++records;
    }
  }
  std::printf("derived records of the mini records: %ld, weaker %ld, more decisive %ld\n", records, weaker, stronger);
  KZ_CHECK_EQ(stronger, 0L);
  KZ_CHECK(weaker > 0);
}


// A value the use passes that a capture read from a location (`cell`), R2 remainder 2026-10-24: the actors that
// wrote that location in the record are actors of the use.  The mini profile's site box_pub passes
// addr(Box_t.id) to get_id's out parameter, through which get_id stores, and Box_t.id is bound to value(Box.id):
// a publication site of Box.id.
void test_captured_location_relation() {
  const Profile p = load(kzmini::files());
  const std::map<std::string, int32_t> pubs = kz::eval::publication_sites(p);
  KZ_CHECK(pubs.size() == 1 && pubs.count("box_pub") == 1 && pubs.at("box_pub") == p.find_loc("Box.id"));
  const std::string x = "owner X 3\nowner_level X confirmed\nowner_core W yes\n";
  // X passes the value 0 it read from Box.id; W publishes Box.id (after X's use in the merged order, which is
  // not the execution order and is not read by the relation); nobody registers reg[0]
  const std::string cap = "X send.who_valid pt arg.Who=0 value=0 cell=@Box.id\nX send call arg.Who=0\nX send ret result=0xc0000001\n";
  const Run r = replay(p, preamble() + x + kSetup + cap + "W box_pub pt\n");
  check_summary(r, "OBSERVED_VIOLATION", "captured, published");
  check_relation(r, KZ_EXEC_RELATION_CORE_APPLICATION, "no registration or publication of reg[0] in the record; the passed value was read from Box.id, written by W",
                 "captured, published");
  KZ_CHECK(r.occ().counterparts == std::vector<std::string>{"W"});
  bool cell_kept = false;
  for (const kz::eval::ConjunctResult& c : r.occ().conjuncts) cell_kept = cell_kept || (c.label == "who_valid" && c.cell == "Box.id");
  KZ_CHECK(cell_kept);
  // the same use without the location: no actor is named
  const Run nocell = replay(p, preamble() + x + kSetup + "X send.who_valid pt arg.Who=0\nX send call arg.Who=0\nX send ret result=0xc0000001\n" + "W box_pub pt\n");
  check_relation(nocell, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "no registration or publication of reg[0] in the record", "no cell");
  KZ_CHECK(nocell.occ().relation_note.find("Box.id") == std::string::npos);
  // a location nobody wrote in the record
  const Run unwritten = replay(p, preamble() + x + kSetup + cap);
  check_relation(unwritten, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "Box.id, which no recorded write targets", "no write");
  // the writer's owner is undetermined
  const Run y = replay(p, preamble() + x + kSetup + cap + "Y box_pub pt\n");
  check_relation(y, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "the owner of Y, which wrote the captured location, is undetermined", "writer undetermined");
  // a store through a pointer whose location the record does not name may have written any location (an actor
  // already named needs nothing more: its actor set is the same either way)
  const Run anyloc = replay(p, preamble() + x + kSetup + cap + "W box_pub pt\nZ get_id call\nZ get_id.store_ok pt\nZ get_id ret result=0\n");
  check_relation(anyloc, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "a write of reg or of a captured location by Z whose target the record does not establish",
                 "unnamed location");
  const Run named = replay(p, preamble() + x + kSetup + cap + "W box_pub pt\nW get_id call\nW get_id.store_ok pt\nW get_id ret result=0\n");
  check_relation(named, KZ_EXEC_RELATION_CORE_APPLICATION, "written by W", "unnamed location by a named writer");
  // a store effect into the location counts like the publication site; the same application is intra
  const Run store = replay(p, preamble() + x + kSetup + cap + "W init_box call\nW init_box.clear_box pt\nW init_box ret\n");
  check_relation(store, KZ_EXEC_RELATION_CORE_APPLICATION, "written by W", "store effect");
  const Run self = replay(p, preamble() + x + kSetup + cap + "X box_pub pt\n");
  check_relation(self, KZ_EXEC_RELATION_INTRA_APPLICATION, "same task (X)", "own publication");
  // the publication site changes the replayed location: with a value it fixes it (a fixed Box.id), without one
  // the location is open from there on
  const std::string box = "adequacy loc Box.id start=known writers=covered aliases=covered\nobserved init_box.clear_box\nobserved get_id.store_ok\nobserved get_id.store_bad\n";
  const Run fixed = replay(p, preamble() + box + x + kSetup + "W box_pub pt value=0x1002\n" + cap);
  KZ_CHECK(fixed.finding("the value loaded from Box.id is 0, the replay of the recorded effects has 0x1002"));
  const Run open = replay(p, preamble() + box + x + kSetup + "W box_pub pt\n" + cap);
  KZ_CHECK(!open.finding("the value loaded from Box.id"));
  KZ_CHECK(open.dump.find("loc Box.id open {effect_unsupported}") != std::string::npos);
}

// The observation an occurrence rests on (Occurrence::probes / ::unobserved) and the static closure lines of
// the text form (StaticClosure; the adapter sets them from kz-static facts, static_closure.hpp).
void test_observation_lists() {
  const Profile p = load(kzmini::files());
  const std::string x = "owner X 3\nowner_level X confirmed\n";
  const std::string cap = "X send.who_valid pt arg.Who=0 value=0 cell=@Box.id\nX send call arg.Who=0\nX send ret result=0xc0000001\n";
  const Run r = replay(p, preamble() + x + kSetup + cap + "W box_pub pt\n");
  const std::vector<std::string>& pr = r.occ().probes;
  auto has = [](const std::vector<std::string>& v, const std::string& x2) { return std::find(v.begin(), v.end(), x2) != v.end(); };
  KZ_CHECK(has(pr, "call send") && has(pr, "return send") && has(pr, "send.who_valid") && has(pr, "send.who_reg"));
  KZ_CHECK(has(pr, "sign_in.clear_reg") && has(pr, "sign_in.set_reg"));  // the observed writers of reg
  KZ_CHECK(!has(pr, "box_pub"));  // a publication site of Box.id is not in the plan of this record (no `observed box_pub`)
  const std::vector<std::string>& un = r.occ().unobserved;
  KZ_CHECK(has(un, "reg: static closure not given") && has(un, "Box.id: static closure not given"));
  KZ_CHECK(has(un, "init_box.clear_box") && has(un, "get_id.store_ok") && has(un, "get_id.store_bad"));  // the writers of Box.id, not observed
  KZ_CHECK(r.dump.find("  probes call send, return send,") != std::string::npos);
  KZ_CHECK(r.dump.find("writer effects not observed: init_box.clear_box, get_id.store_bad, get_id.store_ok") != std::string::npos);
  // a use without a capture reads no location; the static closure's unobserved locations are listed by name
  const std::string closure = "closure kind reg writers=3 covered=2 Reg_t.app writer-set-complete\nunobserved kind reg Reg_t.app writer memset at src/r.c:9 in early\n";
  const Run c = replay(p, preamble() + closure + x + "observed box_pub\n" + kSetup + kRegister + send_by("X", kOk));
  KZ_CHECK(has(c.occ().unobserved, "reg: Reg_t.app writer memset at src/r.c:9 in early"));
  KZ_CHECK(!has(c.occ().unobserved, "reg: static closure not given") && !has(c.occ().unobserved, "Box.id: static closure not given"));
  KZ_CHECK(c.dump.find("static closure: Reg_t.app writer-set-complete; writers 3, at observed points 2; not observed: Reg_t.app writer memset at src/r.c:9 in early") !=
           std::string::npos);
  KZ_CHECK(c.trace.kind_closure.at("reg").covered == 2u);
  const Run back = replay(p, kz::eval::trace_text(c.trace));
  KZ_CHECK_EQ(back.dump, c.dump);
  Trace t;
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nclosure kind reg writers=2 covered=3 x\n", &t).ok());  // more covered than writers
  KZ_CHECK(!kz::eval::parse_trace_text("record real\nclosure kind reg 2 3 x\n", &t).ok());
  KZ_CHECK(!kz::eval::parse_trace_text("closure kind reg writers=1 covered=1 x\n", &t).ok());  // real records only
}

}  // namespace


// The state from observed effects (2026-10-24): an effect on every resource of a kind without a table returns
// them to the initial state and is the evidence of the kind's start state from that event on (IN-03); the
// order of different actors is read from the happens-before edges over every write of the record; a confirmed
// owner's identity fixes the target of its own effects; the clearing writes of the execution relation (P10).
void test_state_from_effects() {
  const Profile p = load_edited("op init_box() {", "op zero_regs() {\n  bind symbol \"zero_regs\";\n  effect wipe: lifecycle all reg -> UNREG lp at fn(\"zero_regs\").memset(Reg_t) prov code_a;\n  prov code_a;\n}\n\nop init_box() {");
  // the reg kind's start state is not declared: it is fixed by the observed wipe
  const std::string head =
      "record real\nstart_evidence init\n"
      "adequacy kind reg writers=covered aliases=covered lifetime=known\n"
      "observed zero_regs.wipe\nobserved sign_in.clear_reg\nobserved sign_in.set_reg\nobserved send.who_reg\n"
      "owner_level W confirmed\nowner_id W 0x1002\nconst TABLE 4\nowner W 2\n";
  const std::string reg = "W sign_in call\nW sign_in.set_reg pt\nW sign_in ret result=0\n";
  // same actor: the wipe, the registration and the use are program order
  const Run own = replay(p, head + "W zero_regs.wipe pt\n" + reg + send(kBadArg));
  check_status(own, OccStatus::Ready, "own wipe, registration and use");
  KZ_CHECK(own.dump.find("start state: from zero_regs.wipe #0") != std::string::npos);
  KZ_CHECK(own.dump.find("owner-identity") != std::string::npos);
  // no wipe: the start state stays unknown
  const Run nowipe = replay(p, head + reg + send(kBadArg));
  check_status(nowipe, OccStatus::Undetermined, "no wipe");
  KZ_CHECK((nowipe.occ().reasons & KZ_WITHHOLD_START_STATE_UNKNOWN) != 0);
  // another actor wiped: ordered with W's events only through the task creation edge
  const std::string spawn = "T0 spawn_it call\nT0 spawn_it ret result=0 out.Handle=7\nW child_main call arg.Handle=7\n";
  const Run created = replay(p, head + "T0 zero_regs.wipe pt\n" + spawn + reg + send(kBadArg));
  check_status(created, OccStatus::Ready, "wipe before the creation of the user");
  KZ_CHECK(created.finding("read from the happens-before edges of the record (1 between actors)"));
  const Run unordered = replay(p, head + "T0 zero_regs.wipe pt\n" + reg + send(kBadArg));
  check_status(unordered, OccStatus::Undetermined, "wipe by an actor not ordered with the user");
  KZ_CHECK((unordered.occ().reasons & KZ_WITHHOLD_EXPLANATION_AMBIGUOUS) != 0);
  // a write of another actor that is not ordered with the read leaves it open even when it is merged after it
  const std::string y = "owner_level Y confirmed\nowner_id Y 0x1002\nowner Y 2\n";
  const Run later = replay(p, head + y + "W zero_regs.wipe pt\n" + reg + send(kBadArg) + "Y sign_in call\nY sign_in.clear_reg pt\nY sign_in ret result=0\n");
  check_status(later, OccStatus::Undetermined, "a concurrent write merged after the read");
  KZ_CHECK((later.occ().reasons & KZ_WITHHOLD_EXPLANATION_AMBIGUOUS) != 0);
  // ... and fixed when the write happens after the read (Y is created by W after the use)
  const std::string after = "W spawn_it call\nW spawn_it ret result=0 out.Handle=9\nY child_main call arg.Handle=9\nY sign_in call\nY sign_in.clear_reg pt\nY sign_in ret result=0\n";
  const Run ordered = replay(p, head + y + "W zero_regs.wipe pt\n" + reg + send(kBadArg) + after);
  check_status(ordered, OccStatus::Ready, "a write ordered after the read");
  // the kind without a table: the model replay returns every resource to the initial state
  const Run model = replay(p, std::string("const TABLE 4\nowner W 2\nT0 create_all.publish pt lp.n=2 lp.pid=0x1002\n") + reg + "W zero_regs.wipe pt\n" + send(kBadArg));
  check_status(model, OccStatus::Violating, "model: the wipe after the registration");
  // P10: a clearing store into the captured location (Box.id := UNDEF, not a valid identity) by an actor whose
  // owner is undetermined: counted (current) the relation is undetermined; not counted, W's publication decides
  const std::string x = "owner X 3\nowner_level X confirmed\nowner_core W yes\n";
  const std::string cap = "X send.who_valid pt arg.Who=0 value=0 cell=@Box.id\nX send call arg.Who=0\nX send ret result=0xc0000001\n";
  const std::string writes = "T9 init_box call\nT9 init_box.clear_box pt\nT9 init_box ret\nW box_pub pt\n";
  const Run counted = replay(p, preamble() + x + kSetup + writes + cap);
  check_relation(counted, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "the owner of T9, which wrote the captured location, is undetermined", "P10 current");
  kz::eval::Options alt;
  alt.relation_clearing_writes = false;
  const Run excluded = replay(p, preamble() + x + kSetup + writes + cap, alt);
  check_relation(excluded, KZ_EXEC_RELATION_CORE_APPLICATION, "1 clearing write(s) not counted (P10 alternative)", "P10 alternative");
  KZ_CHECK(excluded.occ().counterparts == std::vector<std::string>{"W"});
  KZ_CHECK_EQ(excluded.summary(), counted.summary());  // the observation does not depend on P10
  // a writer of the captured location only is named as such, also where the key is registered
  const std::string cap2 = "X send.who_valid pt arg.Who=0x1002 value=0x1002 cell=@Box.id\nX send call arg.Who=0x1002\nX send.who_reg pt\nX send ret result=0\n";
  const Run both = replay(p, preamble() + x + kSetup + kRegister + writes + cap2);
  check_relation(both, KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED, "the owner of T9, which wrote the captured location, is undetermined", "key registered, location cleared by T9");
}

int main() {
  test_full_adequacy_agrees_with_model();
  test_no_initial_state_default();
  test_recorded_writes_need_adequacy();
  test_outcome_and_input_error();
  test_horizon_open_and_caller_requirement();
  test_unobserved_point_and_owner();
  test_text_form();
  test_execution_relation();
  test_outcome_narrowing_alternative();
  test_derived_records();
  test_captured_location_relation();
  test_observation_lists();
  test_state_from_effects();
  return kztest::finish("test_core_realrecord");
}

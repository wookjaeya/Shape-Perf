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
//   - an undetermined owner, an unobserved point, the text form.

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

Run replay(const Profile& p, const std::string& text) {
  Run r;
  const kz::Status ps = kz::eval::parse_trace_text(text, &r.trace);
  if (!ps.ok()) {
    std::fprintf(stderr, "trace does not parse: %s\n", ps.describe().c_str());
    ++kztest::state().failures;
  }
  const kz::Status s = kz::eval::replay(p, r.trace, kz::eval::Options(), &r.result);
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
                     bool observe_who_reg = true, bool owner = true) {
  std::string t = "record real\nstart_evidence init\n";
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
  // the text form reads back to the same trace
  const std::string text = preamble() + "creator W T0\n" + kSetup + kRegister + "loss begin\n" + send(kOk) + "loss end\nW send call arg.Who=0x1002\n";
  const Run a = replay(p, text);
  const Run b = replay(p, kz::eval::trace_text(a.trace));
  KZ_CHECK_EQ(a.dump, b.dump);
  KZ_CHECK_EQ(kz::eval::trace_text(a.trace), kz::eval::trace_text(b.trace));
  KZ_CHECK_EQ(a.trace.losses.size(), static_cast<size_t>(1));
}

}  // namespace

int main() {
  test_full_adequacy_agrees_with_model();
  test_no_initial_state_default();
  test_recorded_writes_need_adequacy();
  test_outcome_and_input_error();
  test_horizon_open_and_caller_requirement();
  test_unobserved_point_and_owner();
  test_text_form();
  return kztest::finish("test_core_realrecord");
}

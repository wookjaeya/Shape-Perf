// eval_trace.hpp -- apply a profile to a trace: replay the effects, derive happens-before edges,
// classify the occurrences of readiness specifications.
//
// This is the concrete interpretation of the profile (SPEC 0.2 item 2, "concretely, over traces"):
// the one evaluator (eval.hpp) over the state that the effects of the trace build up.  The engine
// is generic: it knows no operation, no kind and no rule by name.  Everything it does comes from
// the profile it is given, which is how a golden test of "every rule on a synthetic trace" is
// possible without running anything.
//
// What a trace carries (the agreement between a recorder and a profile):
//   - a Call and a Return event name an operation of the profile; a Return carries `result`;
//     the values of the parameters are `arg.NAME` on the Call (an `out` parameter carries its
//     value on the Return, as `out.NAME`);
//   - a Point event is named after a linearization point (lp_name()) or a site of the profile,
//     or after the terminal step of an operation (terminal_name()); it carries the values the
//     profile declares for the point (`lp.NAME`), `arg.NAME` for the parameters it feeds, and
//     `result` where the point is inside a call that already knows its result;
//   - a capture point carries `arg.NAME` (the value the call will receive), and, to be
//     cross-checked against the replay, `cell` (the name of the location) and `value`;
//   - a point that stores through an out parameter carries `out.NAME`, the name of the location the
//     pointer designates, and optionally `value`, the value stored;
//   - the preamble gives the owner record of every actor and the value of every `ref eval`
//     constant the profile needs (the sizes of the finite tables).
//
// What the replay computes:
//   edges        happens-before edges of the rules (stratum, robustness), sorted;
//   occurrences  one per call of the operation of a readiness specification, with the status of
//                SPEC 1.6 under the strict reading (QT-4, Q15).  A monitor for an Allowed entry is
//                not implemented here: an occurrence that such an entry would judge is PENDING and
//                never TOLERATED;
//   findings     disagreements between the trace and the profile (an outcome that disagrees with the
//                computed state, a capture that disagrees with the replayed cell, an event after a
//                terminal step, a point outside of its call, a poll whose other branch does not end
//                in the terminal operation, a point whose asserted effect guard is false ...);
//   guard_failures  the points whose asserted effect guard is false: a refutation of a
//                declared_trusted guard (with its obligations), a disagreement otherwise.
// A status is final once decided (L-STATUS).  The replay is one pass over the trace; the edges that
// pair two events by a value (task creation) are matched when the pass is over.
//
// Two kinds of trace, chosen explicitly by the trace (`record real`) or by the caller, never guessed:
//   Model  a complete model trace (synthetic traces, the golden files, INIT): a resource the trace never
//          set is in the initial state of its kind and a location holds its INIT value or 0
//          (docs/phi_reference.md section 6, concretization rule (2)).  Everything below is unchanged
//          for it.
//   Real   a real record (SPEC 4.8 "Explanation set", "Replay convention for real records", OB-05,
//          ADR-055; docs/phi_reference.md section 7.6).  The conditions are evaluated in the abstract
//          domain over every explanation the record allows: a resource or location the record does not
//          fix is unknown (no initial-state default) unless its start state is evidenced, and recorded
//          writes fix a kind or location only while its writers and aliases are covered, its start state
//          and lifetime are known, every writer's point was observed and no loss was reported.  A result
//          the explanations disagree on is withheld with its reasons; a decision the trace does not
//          reach stays OPEN; a record that contradicts the model is an analysis input error.
#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "kz/core/eval.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"

namespace kz {
namespace eval {

enum class Phase : uint8_t { Call, Return, Point };
const char* phase_text(Phase p);

struct TraceEvent {
  uint64_t id = 0;        // the position in the trace, assigned by Trace::add
  std::string actor;      // canonical name of the actor instance
  std::string site;       // an operation (Call, Return) or a point (Point)
  Phase phase = Phase::Point;
  bool has_result = false;
  int64_t result = 0;     // Return
  std::map<std::string, int64_t> values;       // "arg.X", "out.X", "lp.X", "value", ...
  std::map<std::string, std::string> texts;    // "out.X", "cell": names of locations
};

constexpr uint64_t kNoEvent = ~static_cast<uint64_t>(0);

enum class RecordKind : uint8_t { Model, Real };
const char* record_kind_text(RecordKind k);

// What a real record establishes for one kind or one location (SPEC 4.8 "Recorded writes alone",
// ADR-055).  Nothing is established unless the record says so.
struct Adequacy {
  bool start_known = false;      // the start state is fixed (INIT at the process start, or a consistent capture)
  bool writers_covered = false;  // every writer is in the plan (SPEC 4.7, 4.1 "Closure per case")
  bool aliases_covered = false;  // every alias and external effect is in the plan
  bool lifetime_known = false;   // the lifetime and identity of every instance is known (kinds; n/a for a location)
};

// A loss reported in a real record: events may be missing from position `begin` (the first event that
// may follow a lost one) up to `end` (exclusive; kNoEvent: up to the end of the record).
struct LossInterval {
  uint64_t begin = 0;
  uint64_t end = kNoEvent;
};

struct Trace {
  std::vector<TraceEvent> events;
  std::map<std::string, int64_t> ref_consts;              // value of a `ref eval` constant, by name
  std::map<std::string, std::vector<int64_t>> owners;     // actor -> key of its owner record
  RecordKind record = RecordKind::Model;
  // Real records only (all empty for a model trace).
  uint8_t start_evidence = KZ_START_EVIDENCE_NONE;        // what the record's start is (informational)
  std::map<std::string, Adequacy> kind_adequacy;          // by kind name; absent: nothing established
  std::map<std::string, Adequacy> loc_adequacy;           // by location name
  std::set<std::string> observed_points;                  // points attached and active over the whole record
  std::map<std::string, uint8_t> owner_levels;            // actor -> KZ_OWNER_LEVEL; absent: undetermined
  std::map<std::string, std::string> creators;            // actor -> the actor that created it (creating executor)
  // The order of the events of different actors is established (HB edges or a reliable observed order,
  // SPEC 4.8 "order").  Default: it is the collector's merged order, which is never the execution order
  // (SPEC 5.8), so a value another actor wrote may be any value of the interleavings.
  bool order_established = false;
  std::vector<LossInterval> losses;
  uint64_t add(TraceEvent e) {
    e.id = events.size();
    events.push_back(std::move(e));
    return events.back().id;
  }
};

// The name a linearization point has in a trace: "OP.LABEL" for a point inside the operation or a
// capture point, the name of the site for `lp site NAME`.
std::string lp_name(const phi::OpDecl& op, const std::string& label, const phi::LpSpec& lp);
// The name of the point at which the terminal step of an operation with an `after` pattern happens.
std::string terminal_name(const phi::OpDecl& op);

struct HbEdge {
  uint64_t from = 0, to = 0;
  std::string rule;
  phi::Stratum stratum = phi::Stratum::S;
  phi::Robust robust = phi::Robust::Unconditional;
  // The edge of an observation rule is read minimally: the write is one the scan that let the poll
  // exit can have seen (the last writes at the moment the awaited predicate became true), but the
  // writer-set completeness and the falsifier of SPEC 4.2/4.8 are not implemented, so the edge is
  // not justified by them.  Consumers that need a justified edge (O3, MustBefore) must not take it
  // as one.  Structural edges (program order, task creation, phase) are not marked.
  bool minimal = false;
  bool operator<(const HbEdge& o) const {
    if (to != o.to) return to < o.to;
    if (from != o.from) return from < o.from;
    return rule < o.rule;
  }
  bool operator==(const HbEdge& o) const { return from == o.from && to == o.to && rule == o.rule; }
};

// Tolerated is part of the vocabulary of SPEC 1.6; the replay never produces it (no monitor).
// Undetermined is not a status: a real record whose explanations allow more than one status (the
// list is Occurrence::possible).  A model trace never produces it.
enum class OccStatus : uint8_t { Ready, NotEvaluated, Violating, NotReadyUnjudged, Tolerated, Pending, Open, Undetermined };
const char* occ_status_text(OccStatus s);

// Interval and Outcome occur in real records only: a conjunct whose linearization point the record
// does not observe is read over the whole call (every state between the call and the return), and a
// conjunct can be fixed by the returned code under O4 (the outcome list of the operation).
enum class LpSource : uint8_t { Capture, Call, Point, NotPassed, Interval, Outcome };
const char* lp_source_text(LpSource s);

struct ConjunctResult {
  std::string label;
  LpSource source = LpSource::NotPassed;
  uint64_t event = 0;     // the event whose state the conjunct was read at
  bool evaluated = false; // a value is known (real records: in every explanation)
  bool holds = false;
  // Real records only.
  bool unknown = false;          // reached, but the explanations of the record disagree on the value
  bool passage_unknown = false;  // whether the linearization point was passed is not observed
  bool contradicted = false;     // the record contradicts the model here (an empty explanation set)
  uint32_t reasons = 0;          // set KZ_WITHHOLD: why the value or the passage is unknown
  uint32_t adequacy = 0;         // set KZ_ADEQ: what the record establishes for what the conjunct read
};

enum class DecidedAt : uint8_t { None, Call, Point, Return };
const char* decided_at_text(DecidedAt d);

// O4(a): the returned code agrees with the readiness the replay computed.
enum class OutcomeCheck : uint8_t { NotChecked, Agrees, Disagrees, Unmapped };
const char* outcome_check_text(OutcomeCheck c);

struct Occurrence {
  std::string rspec;
  std::string actor;
  uint64_t call = 0;
  bool returned = false;
  uint64_t ret = 0;
  std::vector<int64_t> key;
  int64_t ordinal = 0;            // the k-th occurrence begun by this actor with this key (from 1)
  OccStatus status = OccStatus::Open;
  DecidedAt decided_at = DecidedAt::None;
  uint64_t decision = 0;          // the decision event
  bool in_scope = true;           // false: decided after the shutdown request (the teardown family)
  std::vector<ConjunctResult> conjuncts;  // the preconditions of the rspec, in the order of the operation
  bool has_result = false;
  int64_t result = 0;
  OutcomeCheck outcome = OutcomeCheck::NotChecked;
  // The summary view (SPEC 1.10, ADR-054, D2), computed last for every occurrence.  Printed only for a
  // real record, so that the text of a model trace is unchanged.
  kz_summary summary{};
  // Real records only.
  std::vector<OccStatus> possible;  // Undetermined: the statuses the explanations allow
  uint32_t reasons = 0;             // set KZ_WITHHOLD
  uint32_t adequacy = 0;            // set KZ_ADEQ: what the record establishes for everything the conditions read
  bool input_error = false;         // the explanation set is empty: never a violation, never no violation
  bool outcome_narrowed = false;    // a conjunct was fixed by the returned code (O4)
  // The identities of the executing actor (SPEC 1.2; schema kz_identity), as far as the record states them.
  std::string creator;              // the creating executor, empty if the record does not state it
  uint8_t owner_level = KZ_OWNER_LEVEL_UNDETERMINED;
  bool has_owner_key = false;
  std::vector<int64_t> owner_key;
  std::string passed_role;          // the operation's `stamp` parameter, if any
  bool has_passed = false;
  int64_t passed = 0;
};

struct ReplayFinding {
  uint64_t event = kNoEvent;  // the event the finding is about, or kNoEvent
  std::string what;
};

// A point event whose effect has an asserted guard (`when`, not `filter`) that is false in the replay.
// The trace says that the linearization point executed, and the code reaches it only where the guard
// holds, so either the replayed state is not the state of the run or, for a declared_trusted effect,
// the trusted guard is refuted by this observation.  The effect is not applied.
struct GuardFailure {
  uint64_t event = 0;
  std::string op, effect;
  bool trusted = false;                   // the effect is declared_trusted: a refutation
  std::vector<std::string> refutable_by;  // its obligations (trusted only), sorted
};

struct Options {
  // The awaited predicate of an observation rule that takes `predicate from_code` (the extractor's
  // job, here supplied by the caller), by rule name.  It is resolved with the parameters of the
  // poll operation, in order, and not owned.
  std::map<std::string, const phi::Expr*> predicates;
  // The version of each component, for the guards of rules and effects.  A guard on a component
  // that is not listed does not hold: the rule or effect is not applied and the replay says so.
  std::map<std::string, std::string> versions;
};

// What the real-record replay established for one kind or location (printed with the result).
struct Knowledge {
  std::string name;
  bool is_loc = false;
  uint32_t reasons = 0;    // set KZ_WITHHOLD: why recorded writes do not fix it (0: they do)
  uint32_t adequacy = 0;   // set KZ_ADEQ
  bool contradicted = false;
};

struct Result {
  RecordKind record = RecordKind::Model;
  bool partial = false;                // real records: an input error somewhere (SPEC 1.10)
  std::vector<Knowledge> knowledge;    // real records: per kind and location, in profile order
  std::vector<HbEdge> edges;
  std::vector<Occurrence> occurrences;
  std::vector<ReplayFinding> findings;
  std::vector<GuardFailure> guard_failures;  // in the order of the events (also listed as findings)
  ConcreteState final_state;
  bool terminated = false;     // a process-terminal step was replayed
  uint64_t terminal_event = 0;
  bool shutdown_requested = false;
  uint64_t shutdown_event = 0;  // the SHUTDOWN_REQ point
};

// Replays `trace` with a checked profile.  Status Invalid only for an unusable profile or trace
// (not checked, a table without a size); everything the trace does wrong is a finding.
Status replay(const phi::Profile& p, const Trace& trace, const Options& options, Result* out);

// Parses and resolves the awaited predicate of the observation rule `rule` from text.  It is resolved
// with the non-out parameters of the poll operation, in order (the layout Options::predicates wants).
Status resolve_rule_predicate(const phi::Profile& p, const std::string& rule, const std::string& text, phi::ExprPtr* out, std::vector<phi::Diag>* diags);

// Whether `have` satisfies `have OP want` for dotted numeric versions.
bool version_satisfies(const std::string& have, phi::CmpOp op, const std::string& want);

// Canonical text of a result (one record per line; the form of the golden files).
std::string dump_result(const phi::Profile& p, const Trace& trace, const Result& r);

// ---- text form of a trace (tests, tools) ----------------------------------------------------
//
//   # comment
//   const <name> <value>                     the value of a ref eval constant
//   owner <actor> <key> [<key> ...]          the owner record of an actor
//   <actor> <site> call|ret|pt [name=value ...]
//       value: an integer (decimal or 0x...) or @NAME, the name of a location (a text);
//       `result=N` on a ret
// A real record (docs/phi_reference.md section 7.6) starts with `record real` and may then say:
//   start_evidence none|init|state-capture|declared
//   adequacy kind|loc <name> [start=known|unknown] [writers=covered|uncovered] [aliases=covered|uncovered]
//            [lifetime=known|unknown]
//   observed <point>                         a point attached and active over the whole record
//   owner_level <actor> confirmed|inferred|undetermined
//   creator <actor> <creating actor>
//   order established|merged                 default merged: the order of different actors' events is not established
//   loss begin | loss end                    events may be missing from the next event on, up to the next event
Status parse_trace_text(const std::string& text, Trace* out);

// The text form of a trace (parse_trace_text reads it back); a real record with its preamble.
std::string trace_text(const Trace& t);

}  // namespace eval
}  // namespace kz

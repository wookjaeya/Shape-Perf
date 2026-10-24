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
//                SPEC 1.6 under the strict reading (QT-4, Q15).  Of the horizon monitors of the Allowed
//                entries only `retry_until_ready` is run (SPEC 1.6 "Horizon monitors"; see
//                Occurrence::monitor): it turns a PENDING attempt into TOLERATED (a later attempt of the
//                same loop instance is READY within the bound) or VIOLATING (the bound is exhausted, or an
//                attempt's outcome is outside its codes); an attempt it cannot count stays withheld.  The
//                monitor of `exit(code, src)` is run too (2026-10-24): ACCEPT when the using actor calls a
//                terminal operation with that code, REJECT when it calls one with another code.  An
//                occurrence that another entry would judge stays PENDING and is never TOLERATED;
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

// What the static closure says about one kind or location (SPEC 4.1 "Closure per case", 4.2; kz-static's
// FACT_LOCS and WRITERS for the record fields the profile binds to it), as the adapter matched it with the
// probe plan of the record.  Informational for the replay: the closure enters the replay only through
// Adequacy::writers_covered / aliases_covered, which the adapter sets from it (and every other condition of
// ADR-055 still applies).  Absent: no closure was given.
struct StaticClosure {
  std::string status;                   // per bound field: "<Type.field> writer-set-complete|DISABLED|not in the closure", ';'-joined
  uint32_t writers = 0;                 // static writers of the bound fields
  uint32_t covered = 0;                 // of those, at a point the plan observes and the profile maps to a write of it
  std::vector<std::string> unobserved;  // the static writer and alias locations the plan does not observe
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
  std::map<std::string, StaticClosure> kind_closure;      // by kind name; absent: no static closure given
  std::map<std::string, StaticClosure> loc_closure;       // by location name
  std::set<std::string> observed_points;                  // points attached and active over the whole record
  std::map<std::string, uint8_t> owner_levels;            // actor -> KZ_OWNER_LEVEL; absent: undetermined
  // actor -> the identity value of its owning application in the id space of the owner kind (the value the
  // creation request passed, ACTOR_OWNERS).  Used only to name the actor of a registration or publication
  // (`id(owner(exec))`, the execution relation); it never fixes the replayed state.  Absent: not recorded.
  std::map<std::string, uint32_t> owner_ids;
  // actor -> whether its owning application is a core service (true) or not (false), as the record
  // establishes it (the creation request's calling function, SPEC 3.3 "Creation events used for
  // app_owner").  Absent: not established.
  std::map<std::string, bool> owner_core;
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
// The publication sites of locations: a site of the profile whose call pattern passes the address of a field
// (`arg[i] is addr(T.F)`) to an `out` parameter of the called operation (the operation that binds the callee's
// symbol) through which that operation stores (`store *P := ...`), where `bind field T F as value(L)`.  An
// event at such a site (after the call, in the caller) records that its actor wrote L through that call.
// Site name -> location index.
std::map<std::string, int32_t> publication_sites(const phi::Profile& p);

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

// Tolerated is produced only by an ACCEPT of the retry_until_ready or the exit monitor; no other monitor is run.
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
  // A capture (real records): the location the value was loaded from, as the capture event names it
  // (`cell`); empty when the record does not name it.  The execution relation names the actors that wrote
  // that location (Occurrence::relation).
  std::string cell;
  // Real records: the value read a resource whose replayed state was set by an effect whose operand only the
  // writer's own identity fixed (Options::owner_identity_targets).
  bool owner_identity = false;
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
  // The execution relation (SPEC 1.1 "Execution relations", 1.10 item 10; real records only): between the
  // executing task of the use and the actors of the registrations and publications of the occurrence's key
  // resource in the record (the effects on the key's kind whose target, read with the writer's own
  // identity, is the key).  Only `confirmed` and `inferred` owners are evidence; anything else leaves it
  // ownership-undetermined.  Also stored in `summary.relation` (with relation-inferred, core-status-unknown).
  uint8_t relation = KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED;
  uint8_t relation_level = KZ_OWNER_LEVEL_UNDETERMINED;  // the weakest owner level the relation used
  std::vector<std::string> counterparts;  // the actors of the registrations / publications, sorted
  std::string relation_note;              // which pair decided it, or why it is undetermined
  // The role of the call context (SPEC 1.6 "Call contexts"): the profile role whose site event followed
  // the return of this call on the same actor; empty when no role site was seen for it.
  std::string context;
  bool role_excluded = false;             // internal: the role takes the call out (moved to Result::role_facts)
  bool key_known = false;                 // the key was evaluated (real records: fixed in every explanation)
  // The horizon monitor of a deciding Allowed entry (SPEC 1.6 "Horizon monitors"), where one ran on this
  // occurrence: its verdict and why (ACCEPT / REJECT / OPEN / not countable), with the events it used.
  // Only retry_until_ready is implemented.  `accepted` is the ACCEPT that makes the status TOLERATED; the
  // entry that accepted rests on a caller record (the caller-level Allowed reads only such entries).
  std::string monitor;
  bool accepted = false;
  // Real records: why the caller-level Allowed withholds a not-ready occurrence, where it records a conflict
  // (SPEC 1.6 step 3, QT-5: "the conflict named in the verdict"); empty otherwise.
  std::string allowed_note;
  // Real records: the observation the conditions rest on (OBS_ADEQUACY `probes` and `unobserved`).  `probes`
  // names what the record observes, as "call OP", "return OP" or a point / site name: the operation's call and
  // return, the points of its conditions (a capture where one was used), the points of the profile's writer
  // effects of every kind and location the conditions read (a captured location included) and the
  // publication sites of those locations.  `unobserved` names what is missing: a condition's point the record
  // does not observe, a writer effect whose point it does not observe ("OP.label"), and the static writer and
  // alias locations the static closure gives that the plan does not observe ("<kind or loc>: ..."), or
  // "<kind or loc>: static closure not given".  Both sorted, without duplicates.
  std::vector<std::string> probes;
  std::vector<std::string> unobserved;
};

// A call bound to a declared role of its call context (the role's site event right after the call's
// return, same actor).  A role whose purpose is not dependent_use carries no readiness precondition: the
// occurrences its call began are not instances (they are listed here as `excluded`), the call stays a fact.
struct RoleFact {
  std::string role, purpose, actor, op;
  uint64_t point = 0;                     // the role's site event
  uint64_t ret = 0;                       // the return it follows
  uint64_t call = kNoEvent;               // the call, where an occurrence names it
  bool has_result = false;
  int64_t result = 0;
  std::vector<std::string> excluded;      // the readiness specifications whose occurrence the role took out
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
  // Real records, P1 of STATUS_W0W3_20261024 (O4-real, a pending PI decision): whether the returned code
  // fixes a conjunct the record leaves open (`outcome-narrowed`, the current behaviour).  false is the
  // alternative: the code fixes nothing, and is still checked against what every explanation fixes (a
  // disagreement stays an analysis input error).
  bool outcome_narrowing = true;
  // Real records: whether the identity value the record gives for a `confirmed` app_owner (Trace::owner_ids, the
  // value its creation request passed) fixes `id(owner(exec))` in the target, id or value of an effect of that
  // actor where the replayed state does not (true, 2026-10-24).  The conditions that read a resource such an
  // effect set say so (ConjunctResult::owner_identity).  false: the effect's operand stays open (the reading
  // before 2026-10-24, in which the identity named only the actors of the execution relation).
  bool owner_identity_targets = true;
  // Real records, P10 of STATUS_W0W3_20261024 section 2a (a pending PI decision): whether a clearing write (an
  // effect that returns the key resource to its kind's initial state, or a store of a known value that is not a
  // valid identity into a captured location) makes its actor one of the actors of the execution relation.
  // true is the current behaviour (every observed write counts); false is the alternative.
  bool relation_clearing_writes = true;
  // Whether the edges of `reads_from` rules are made (SPEC 3.16 row RF, stratum O: a successful use reads from the
  // publication of the same resource and generation; with `match`, the out values of both are equal).  true: the
  // rules of the profile apply (2026-10-24); false: their edges are left out (a reading to compare with, not a PI
  // decision).
  bool reads_from_edges = true;
};

// What the real-record replay established for one kind or location (printed with the result).
struct Knowledge {
  std::string name;
  bool is_loc = false;
  uint32_t reasons = 0;    // set KZ_WITHHOLD: why recorded writes do not fix it (0: they do)
  uint32_t adequacy = 0;   // set KZ_ADEQ
  bool contradicted = false;
  std::vector<std::string> unobserved_writers;  // the profile's writer effects whose point the record does not observe
  std::string start;  // the observed effect from which the start state is known, where the record did not declare it
};

struct Result {
  RecordKind record = RecordKind::Model;
  bool partial = false;                // real records: an input error somewhere (SPEC 1.10)
  std::vector<Knowledge> knowledge;    // real records: per kind and location, in profile order
  std::vector<HbEdge> edges;
  std::vector<Occurrence> occurrences;
  std::vector<ReplayFinding> findings;
  std::vector<GuardFailure> guard_failures;  // in the order of the events (also listed as findings)
  std::vector<RoleFact> role_facts;          // calls bound to a role of their call context, in event order
  ConcreteState final_state;
  bool terminated = false;     // a process-terminal step was replayed
  uint64_t terminal_event = 0;
  bool shutdown_requested = false;
  uint64_t shutdown_event = 0;  // the SHUTDOWN_REQ point
  bool teardown_started = false;
  uint64_t teardown_event = 0;  // the TEARDOWN point (the scope kind reaches its teardown state)
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
//   owner_id <actor> <value>                 the identity value of the actor's owning application
//   owner_core <actor> yes|no                whether that application is a core service
//   creator <actor> <creating actor>
//   order established|merged                 default merged: the order of different actors' events is not established
//   loss begin | loss end                    events may be missing from the next event on, up to the next event
//   closure kind|loc <name> writers=<n> covered=<m> <status ...>   what the static closure says (StaticClosure)
//   unobserved kind|loc <name> <text ...>    one static writer or alias location the plan does not observe
Status parse_trace_text(const std::string& text, Trace* out);

// The text form of a trace (parse_trace_text reads it back); a real record with its preamble.
std::string trace_text(const Trace& t);

}  // namespace eval
}  // namespace kz

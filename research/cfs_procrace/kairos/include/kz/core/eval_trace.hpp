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
//                in the terminal operation ...).
// A status is final once decided (L-STATUS).  The replay is one pass over the trace; the edges that
// pair two events by a value (task creation) are matched when the pass is over.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "kz/core/eval.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/status.hpp"

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

struct Trace {
  std::vector<TraceEvent> events;
  std::map<std::string, int64_t> ref_consts;              // value of a `ref eval` constant, by name
  std::map<std::string, std::vector<int64_t>> owners;     // actor -> key of its owner record
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
enum class OccStatus : uint8_t { Ready, NotEvaluated, Violating, NotReadyUnjudged, Tolerated, Pending, Open };
const char* occ_status_text(OccStatus s);

enum class LpSource : uint8_t { Capture, Call, Point, NotPassed };
const char* lp_source_text(LpSource s);

struct ConjunctResult {
  std::string label;
  LpSource source = LpSource::NotPassed;
  uint64_t event = 0;     // the event whose state the conjunct was read at
  bool evaluated = false;
  bool holds = false;
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
};

constexpr uint64_t kNoEvent = ~static_cast<uint64_t>(0);

struct ReplayFinding {
  uint64_t event = kNoEvent;  // the event the finding is about, or kNoEvent
  std::string what;
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

struct Result {
  std::vector<HbEdge> edges;
  std::vector<Occurrence> occurrences;
  std::vector<ReplayFinding> findings;
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
Status parse_trace_text(const std::string& text, Trace* out);

}  // namespace eval
}  // namespace kz

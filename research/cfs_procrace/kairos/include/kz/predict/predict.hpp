// predict.hpp -- `kz predict`: bounded explicit-state prediction on a predict model, anchored in a record
// (SPEC 4.9 rev 1.3 "Engine", "Backward closure", "Values and later paths", PR-01-PR-04; P-Z3: explicit search
// first, no solver).
//
// The search runs the actor programs of a model (model.hpp) over every interleaving of their visible steps,
// with a visited-state set (no partial-order reduction: each visible step is one atomic transition at its
// linearization point, so two steps of different actors commute exactly when the model says they touch
// nothing in common, and the search does not rely on that).  Two kinds of order are kept apart (RV 5.3,
// 11 stage D):
//   constraints  program order (the programs), creation (a spawned actor runs only after its spawn step),
//                the guards of the programs (await: a poll exit by condition needs the enabling write), and
//                the record's stratum-S edges between cone steps (kept in every explored order);
//   variables    the record's stratum-O edges (outcome-implied reads-from) and the orders the record's
//                outcomes alone decide (a failed use before the creation of its resource): they are never
//                constraints of the prediction; every explored order computes its own results from the
//                state at each step, so a reads-from is re-derived per order.
// Two ablations make the alternatives explicit: `fixed` turns the observed-run-dependent orders into
// constraints (the failing order an RF edge forbids is then lost), `keep` keeps the record's results and
// branches while dropping those orders (the candidates are then re-judged and the infeasible ones rejected).
//
// Every class of executions found is re-judged by the profile's replay (eval::replay, model record): the
// same readiness specification, outcome mapping, horizon monitors and scope as the observation judgement.
// A candidate whose replay finds a result that disagrees with the state at its conjunct point, a guard
// failure or an input error is rejected (never a witness).  Nothing here is a verdict about the program:
// a witness is a MODEL_CANDIDATE until a controlled run confirms it (SPEC 4.13 CF-01-CF-04).
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi_ast.hpp"
#include "kz/core/status.hpp"
#include "kz/predict/model.hpp"

namespace kz::predict {

// What one visible step does, as the record would show it: the operation (or the point), the key text
// (the spawned actor for a spawn), the result.
struct Tuple {
  std::string op, key;
  bool has_result = false;
  int64_t result = 0;
  bool operator==(const Tuple& o) const { return op == o.op && key == o.key && has_result == o.has_result && (!has_result || result == o.result); }
};
std::string tuple_text(const Tuple& t);

struct HitInst {
  std::string site;
  bool before = true;
  std::string caller, name_role, name_text;
};

struct Step {
  int32_t actor = -1;
  int32_t index = -1;    // the actor's visible-step index (-1: a step without a visible instruction)
  int32_t pc = -1;       // the instruction of the step (its visible instruction, or the finish)
  Ins ins = Ins::Finish; // its kind
  bool created = false;  // a create that published (the resource's new generation)
  Tuple tuple;
  int32_t res = -1;      // the resource the step touched (or -1)
  uint32_t gen = 0;      // its generation after the step
  bool instance = false; // a use that is an instance of the readiness specification
  bool held = false;     // the actor was held at a gate of a plan (no effect)
  std::vector<eval::TraceEvent> events;
  std::vector<HitInst> hits;  // in the order they are reached
  uint32_t n_pre = 0;         // hits [n_pre, n_before) are the visible instruction's own (before its linearization point);
  uint32_t n_before = 0;      // hits before n_pre come from local instructions before it, hits from n_before on after it
  std::vector<std::string> marks;
};

// `to` (the to_index-th visible step of to_actor) may run only after `from` ran.
struct OrderConstraint {
  int32_t from_actor = -1, from_index = 0, to_actor = -1, to_index = 0;
  std::string why;
};

// A kzp-hold 1 control point.
struct ControlPoint {
  std::string label;
  bool release = false;
  std::string releases;  // release: the gate label
  std::string site, caller, name_role, name_text;
  uint32_t occurrence = 1;  // the qualifying hit of its marker it acts on (kzp-hold `occurrence <n>`; 1 = first)
  bool unless_released = false;  // (gate) kzp-hold `unless-released`: a release that acts before its hit satisfies it
  int32_t actor = -1;    // the model actor whose step it is
  std::string why;       // the step it stands for and its citations
};
struct Plan {
  bool any = false;
  std::vector<ControlPoint> points;
  std::vector<std::string> notes;
};

// A plan and what it does in the model under the supervisor's semantics (kz hold, probe/hold/supervisor.cpp): each point
// counts the qualifying hits of its marker (by any actor) and acts on the one its occurrence names; a gate holds the actor
// there (the first gate that acts on a hit; another one acting on the same hit is consumed); a release acts on its hit
// also when its gate does not hold an actor then (it is then consumed, and an unless-released gate is satisfied: it
// holds nothing afterwards); the releases of a hit at which an actor is held
// act when it is resumed; a resumed actor stays armed, so its later hits act like any other actor's.  The model has
// gates only at the before-hits of visible steps (where kz predict places them).  Before confirm2 (2026-10-25) a released
// actor's later hits acted on nothing (its debug registers were cleared) and only the first hit could act.
struct PlanResult {
  std::string name;   // the file stem: W<n> (every order of the candidate) or W<n>-<k> (the order of one key)
  std::string scope;  // what it enforces
  std::string same_as;  // an earlier plan with the same control points (empty: none)
  Plan plan;
  bool checked = false;  // the check search completed within the cap
  bool forces = false;   // every execution under the plan that does not stall is in the candidate's class
  bool stall = false;    // some execution stalls (an actor held for good)
  bool never = false;    // no execution under the plan completes without a stall in which every gate it reached was released
  std::vector<std::string> realized, forced, free, gates;
};

struct ExploreOptions {
  uint64_t max_states = 0;  // the visited-state cap (a researcher choice; reported)
  std::vector<OrderConstraint> constraints;
  const std::vector<std::vector<Tuple>>* follow = nullptr;  // per actor: the visible steps must make these tuples
  bool keep_outcomes = false;                               // with follow: the use results are the followed ones
  const Plan* plan = nullptr;                               // the supervisor semantics of a hold plan
};

struct ClassInfo {
  std::string key;                    // the abstract history (results per actor and resource, end states)
  std::vector<std::string> segments;  // the parts of the key, per actor and resource ("A/R ...") and the ends
  std::vector<Step> first;            // the representative: the execution with the smallest counts (in order)
  std::vector<Step> extreme;          // the execution with the largest counts
  std::vector<uint32_t> min_counts, max_counts, first_counts, extreme_counts;
  uint64_t terminals = 0;
  bool cut = false;     // the shutdown request came while an instance actor was unfinished: no default-scope witness
  bool stall = false;   // (plan) an actor is held at the end and was never released
  bool invalid = false; // a step the model cannot continue (an owned resource at exit)
};

struct ExploreResult {
  uint64_t states = 0, transitions = 0, terminals = 0;
  bool cap_reached = false;
  std::vector<ClassInfo> classes;
  std::string error;
};

Status explore(const phi::Profile& p, const Model& m, const ExploreOptions& o, ExploreResult* out);

// The model trace of an execution: record kind model, the constants, the owners of the actors (by record name).
eval::Trace execution_trace(const Model& m, const std::vector<Step>& steps, const std::map<std::string, int64_t>& ref_consts,
                            const std::map<std::string, std::vector<int64_t>>& owners);

// ---- predict --------------------------------------------------------------------------------

enum class EdgeMode : uint8_t { Variable, Fixed };
enum class OutcomeMode : uint8_t { Rederive, Keep };

struct PredictInput {
  const phi::Profile* profile = nullptr;
  const Model* model = nullptr;
  const eval::Trace* record = nullptr;          // the record's trace (real record)
  const eval::Result* record_result = nullptr;  // its replay
  std::map<std::string, int64_t> ref_consts;
  eval::Options replay_options;                 // the options of the record's replay (used for the candidates too)
  uint64_t max_states = 0;
  EdgeMode edges = EdgeMode::Variable;
  OutcomeMode outcomes = OutcomeMode::Rederive;
};

struct EdgeInfo {
  std::string rule;         // the HB rule, or "outcome" for an order the record's outcome decides
  std::string stratum;      // S, O, C, or "outcome"
  uint64_t from_event = eval::kNoEvent, to_event = eval::kNoEvent;
  OrderConstraint order;
  bool constraint = false;  // kept as a constraint of the prediction
  std::string text;
};

struct Judged {
  std::string actor, key;
  uint32_t ordinal = 0;
  eval::OccStatus status = eval::OccStatus::Open;
  std::string observation;  // the summary view
  std::string monitor;
  std::string scope;
  bool final_default = false;  // a final status in the default scope
  bool disagrees = false;
};

struct Witness {
  uint32_t id = 0;
  ClassInfo cls;
  std::vector<Judged> judged;
  bool accepted = false, observed = false, in_scope = false, time_based = false;
  std::string reject;
  std::vector<std::string> extreme_note;
  std::vector<std::string> orders;  // the observed-run-dependent orders: kept / not kept
  std::vector<PlanResult> plans;
  std::vector<std::string> plan_notes;
  eval::Trace trace;
};

struct PredictReport {
  bool anchored = false;
  std::string anchor_note;
  std::vector<Step> anchor;
  std::string anchor_key;
  std::vector<Judged> observed;        // the record's own judgement
  std::vector<Judged> anchor_judged;   // the replay of the anchor's model trace
  std::vector<std::string> outside;    // record steps outside the cone, with the reason
  std::vector<std::vector<Tuple>> record_tuples;
  std::vector<EdgeInfo> edges;
  ExploreResult search;
  std::vector<Witness> witnesses;
  uint64_t cut_classes = 0, cut_terminals = 0, rejected = 0;
  std::vector<std::string> bounds;
};

Status predict(const PredictInput& in, PredictReport* out);
std::string report_text(const PredictInput& in, const PredictReport& r);
// The kzp-hold 1 text of the k-th plan of a witness, with its comments.
std::string plan_text(const PredictInput& in, const Witness& w, size_t k, const std::string& record_name);

}  // namespace kz::predict

// eval_trace.cpp -- the replay of a trace with a profile (see eval_trace.hpp).
//
// One class, Replayer, walks the events once.  It keeps
//   - the concrete state the effects build up (the one that eval.hpp's ConcreteDomain reads),
//   - the open calls of every actor,
//   - the captures that wait for the call they feed,
//   - the occurrences of readiness specifications and their conjunct results.
// The expression evaluator is the template of eval_evaluator.hpp; nothing here interprets an
// expression itself.

#include "kz/core/eval_trace.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <set>
#include <tuple>

#include "kz/core/phi_allowed.hpp"
#include "kz/core/registry.hpp"
#include "kz/core/verdict.hpp"

namespace kz {
namespace eval {

using phi::CmpOp;
using phi::EffectDecl;
using phi::EffectKind;
using phi::Expr;
using phi::ExprKind;
using phi::HbRuleDecl;
using phi::KindDecl;
using phi::LpKind;
using phi::LpSpec;
using phi::NameRef;
using phi::OpDecl;
using phi::OutcomeClass;
using phi::Profile;
using phi::RuleTemplate;

// ---- vocabulary -----------------------------------------------------------------------------

const char* phase_text(Phase p) {
  switch (p) {
    case Phase::Call: return "call";
    case Phase::Return: return "ret";
    case Phase::Point: return "pt";
  }
  return "?";
}

const char* occ_status_text(OccStatus s) {
  switch (s) {
    case OccStatus::Ready: return "READY";
    case OccStatus::NotEvaluated: return "NOT_EVALUATED";
    case OccStatus::Violating: return "VIOLATING";
    case OccStatus::NotReadyUnjudged: return "NOT_READY_UNJUDGED";
    case OccStatus::Tolerated: return "TOLERATED";
    case OccStatus::Pending: return "PENDING";
    case OccStatus::Open: return "OPEN";
    case OccStatus::Undetermined: return "UNDETERMINED";
  }
  return "?";
}

const char* record_kind_text(RecordKind k) { return k == RecordKind::Real ? "real" : "model"; }

const char* lp_source_text(LpSource s) {
  switch (s) {
    case LpSource::Capture: return "capture";
    case LpSource::Call: return "call";
    case LpSource::Point: return "point";
    case LpSource::NotPassed: return "not_passed";
    case LpSource::Interval: return "interval";
    case LpSource::Outcome: return "outcome";
  }
  return "?";
}

const char* decided_at_text(DecidedAt d) {
  switch (d) {
    case DecidedAt::None: return "none";
    case DecidedAt::Call: return "call";
    case DecidedAt::Point: return "point";
    case DecidedAt::Return: return "return";
  }
  return "?";
}

const char* outcome_check_text(OutcomeCheck c) {
  switch (c) {
    case OutcomeCheck::NotChecked: return "not_checked";
    case OutcomeCheck::Agrees: return "agrees";
    case OutcomeCheck::Disagrees: return "disagrees";
    case OutcomeCheck::Unmapped: return "unmapped";
  }
  return "?";
}

std::string lp_name(const OpDecl& op, const std::string& label, const LpSpec& lp) {
  if (lp.kind == LpKind::Site) return lp.site;
  return op.name + "." + label;
}

std::string terminal_name(const OpDecl& op) { return op.name + ".terminal"; }

// ---- versions -------------------------------------------------------------------------------

namespace {

std::vector<std::string> split_dots(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == '.') {
      out.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  out.push_back(cur);
  return out;
}

bool all_digits(const std::string& s) {
  if (s.empty()) return false;
  for (char c : s)
    if (c < '0' || c > '9') return false;
  return true;
}

int compare_versions(const std::string& a, const std::string& b) {
  const std::vector<std::string> x = split_dots(a), y = split_dots(b);
  const size_t n = std::max(x.size(), y.size());
  for (size_t i = 0; i < n; ++i) {
    const std::string xa = i < x.size() ? x[i] : "0";
    const std::string ya = i < y.size() ? y[i] : "0";
    int c;
    if (all_digits(xa) && all_digits(ya)) {
      const std::string xs = xa.substr(std::min(xa.find_first_not_of('0'), xa.size() - 1));
      const std::string ys = ya.substr(std::min(ya.find_first_not_of('0'), ya.size() - 1));
      c = xs.size() != ys.size() ? (xs.size() < ys.size() ? -1 : 1) : xs.compare(ys);
    } else {
      c = xa.compare(ya);
    }
    if (c != 0) return c < 0 ? -1 : 1;
  }
  return 0;
}

}  // namespace

bool version_satisfies(const std::string& have, CmpOp op, const std::string& want) {
  const int c = compare_versions(have, want);
  switch (op) {
    case CmpOp::Eq: return c == 0;
    case CmpOp::Ne: return c != 0;
    case CmpOp::Lt: return c < 0;
    case CmpOp::Le: return c <= 0;
    case CmpOp::Gt: return c > 0;
    case CmpOp::Ge: return c >= 0;
  }
  return false;
}

// ---- the replayer ---------------------------------------------------------------------------

namespace {

using ArgMap = std::map<std::string, int64_t>;

struct Used {
  std::set<int32_t> args;
  bool result = false;
};

void collect_used(const Expr& e, Used* u) {
  if (e.kind == ExprKind::Name) {
    if (e.ref == NameRef::Arg) u->args.insert(e.ref_index);
    if (e.ref == NameRef::Result) u->result = true;
  }
  for (const phi::ExprPtr& a : e.args)
    if (a) collect_used(*a, u);
}

uint32_t u32(int64_t v) { return static_cast<uint32_t>(v); }

std::string hex_or_dec(int64_t v) {
  char buf[40];
  if (v > -1024 && v < 1024) std::snprintf(buf, sizeof buf, "%lld", static_cast<long long>(v));
  else std::snprintf(buf, sizeof buf, "0x%llx", static_cast<unsigned long long>(static_cast<uint64_t>(v)));
  return buf;
}

std::string key_text(const std::vector<int64_t>& key) {
  std::string s = "[";
  for (size_t i = 0; i < key.size(); ++i) {
    if (i != 0) s += ",";
    s += hex_or_dec(key[i]);
  }
  return s + "]";
}

struct WriteInfo {
  uint64_t event = 0;
  std::string actor;
};

// What an open call of a poll operation knows about the observation rule that waits in it: whether
// the awaited predicate holds in the replayed state now, and the last writes of every resource at
// the moment it last became true (at the call, or at the write that flipped it).  Only those writes
// can have been seen by the scan that let the poll exit; a later write may have landed after it.
struct PollWatch {
  size_t rule = 0;
  bool truth = false;
  std::map<ResId, WriteInfo> seen;
};

// ---- real records: the abstract evaluation over the explanations of the record ----------------

// What an evaluation read: kinds through state/tag/id, locations through value, the owner of the
// executing actor.  The reasons and the adequacy of a result are those of what it read.
struct Reads {
  std::set<int32_t> kinds;
  std::set<ResId> res;       // resources read by an exact key
  std::set<int32_t> boxes;   // kinds read over a box of keys
  std::set<int32_t> locs;
  bool owner = false;
};

// The abstract domain of eval.hpp, recording what it is asked.  The evaluator template calls the
// methods of the class it is instantiated with, so these hide the base ones.
class RecordingDomain : public AbstractDomain {
 public:
  RecordingDomain(const Profile& p, const AbstractState& s, Reads* reads) : AbstractDomain(p, s), p_(p), reads_(reads) {}
  AVal state_of(const AVal& res) const {
    note(res);
    return AbstractDomain::state_of(res);
  }
  AVal tag_of(const AVal& res, int32_t tag) const {
    note(res);
    return AbstractDomain::tag_of(res, tag);
  }
  AVal id_of(const AVal& res) const {
    note(res);
    return AbstractDomain::id_of(res);
  }
  AVal value_of(const AVal& loc) const {
    const int64_t n = static_cast<int64_t>(p_.locs.size());
    for (int64_t li = std::max<int64_t>(loc.lo, 0); li <= std::min<int64_t>(loc.hi, n - 1); ++li) reads_->locs.insert(static_cast<int32_t>(li));
    return AbstractDomain::value_of(loc);
  }
  AVal owner_of(const AVal& actor) const {
    reads_->owner = true;
    return AbstractDomain::owner_of(actor);
  }

 private:
  const Profile& p_;
  Reads* reads_;
  void note(const AVal& res) const {
    reads_->kinds.insert(res.kind);
    if (res.must_be_undefined) return;
    std::vector<int64_t> key;
    bool exact = !res.may_be_undefined;
    for (const auto& iv : res.key) {
      if (iv.first != iv.second) exact = false;
      key.push_back(iv.first);
    }
    if (exact) reads_->res.insert(ResId{res.kind, key});
    else reads_->boxes.insert(res.kind);
  }
};

constexpr uint32_t kAllAdequacy = KZ_ADEQ_WRITERS_COVERED | KZ_ADEQ_ALIASES_COVERED | KZ_ADEQ_START_STATE_KNOWN | KZ_ADEQ_LIFETIME_KNOWN |
                                  KZ_ADEQ_PROBES_ATTACHED | KZ_ADEQ_PROBES_ACTIVE | KZ_ADEQ_LOSS_FREE;

struct RealEval {
  AVal v;
  uint32_t reasons = 0;               // KZ_WITHHOLD of the unknown inputs it read
  uint32_t adequacy = kAllAdequacy;   // KZ_ADEQ: what the record establishes for what it read
  bool contradicted = false;
};

bool exact_key(const AVal& r, std::vector<int64_t>* key) {
  key->clear();
  if (r.may_be_undefined || r.must_be_undefined) return false;
  for (const auto& iv : r.key) {
    if (iv.first != iv.second) return false;
    key->push_back(iv.first);
  }
  return true;
}

bool exact_value(const AVal& v, int64_t* out) {
  if (v.lo != v.hi) return false;
  *out = v.lo;
  return true;
}

// A conjunct of a real record, read over the whole call because its point is not observed: the join of
// the values in every state between the call and the return.
struct IntervalRead {
  bool active = false;
  bool any = false;
  Tri tri = Tri::Maybe;
  uint32_t reasons = 0;
  uint32_t adequacy = kAllAdequacy;
  bool contradicted = false;
};

struct Frame {
  size_t op = 0;
  uint64_t call = 0;
  ArgMap args;
  std::vector<ConjunctResult> conj;  // one per conjunct of the operation
  std::vector<size_t> occs;          // indices into Result::occurrences, one per rspec of the operation
  std::vector<PollWatch> watches;    // one per observation rule that polls in this operation
  std::vector<IntervalRead> intervals;  // real records: one per conjunct, active where its point is not observed
};

struct PointRef {
  size_t op = 0;
  bool conjunct = false;
  size_t index = 0;
};

struct RspecInfo {
  size_t op = 0;
  phi::CallerAllowed allowed;
  std::vector<size_t> conj;  // indices of the operation's conjuncts that are the rspec's preconditions
};

struct CreateRec {
  size_t rule = 0;
  std::string actor;
  uint64_t call = 0;
  bool has_value = false;
  int64_t value = 0;
};

struct EntryRec {
  size_t rule = 0;
  uint64_t event = 0;
  std::string actor;
  bool has_value = false;
  int64_t value = 0;
};

struct SpawnRec {
  std::string parent;
  std::string child;
  uint64_t call = 0;
  uint64_t entry = 0;
};

class Replayer {
 public:
  Replayer(const Profile& p, const Trace& t, const Options& o, Result* r) : p_(p), t_(t), o_(o), r_(*r) {}
  Status run();

 private:
  const Profile& p_;
  const Trace& t_;
  const Options& o_;
  Result& r_;
  ConcreteState st_;

  std::map<std::string, std::vector<PointRef>> points_;
  std::map<std::string, size_t> terminal_points_;
  std::set<std::string> site_names_;
  std::vector<RspecInfo> rspec_info_;
  std::vector<std::vector<size_t>> rspecs_of_op_;
  std::vector<bool> rule_active_;

  std::map<std::string, int64_t> actor_no_;
  std::map<std::string, std::vector<Frame>> frames_;
  std::map<std::tuple<std::string, size_t, size_t>, ConjunctResult> captures_;
  std::map<std::string, uint64_t> last_event_;
  std::map<std::string, uint64_t> first_event_;
  std::map<std::string, int64_t> ordinals_;
  std::map<std::string, size_t> role_of_site_;  // site name -> role index (SPEC 1.6 "Call contexts")
  std::map<ResId, WriteInfo> last_write_;
  std::map<std::string, std::pair<size_t, uint64_t>> expect_terminal_;  // actor -> (rule, poll return event)
  std::set<size_t> predicate_missing_reported_;
  std::set<std::string> guard_reported_;
  std::set<std::pair<uint64_t, std::string>> seen_findings_;
  std::vector<CreateRec> creates_;
  std::vector<EntryRec> entries_;
  bool began_handler_ = false;
  size_t after_terminal_ = 0;
  int32_t request_ordinal_ = -1;  // the state of the shutdown kind that is the request

  // Real records (SPEC 4.8, ADR-055).
  bool real_ = false;
  std::vector<uint32_t> kind_reasons_, loc_reasons_;  // KZ_WITHHOLD: why recorded writes do not fix it (0: they do)
  std::vector<uint32_t> kind_adeq_, loc_adeq_;        // KZ_ADEQ: what the record establishes
  std::vector<bool> kind_contra_, loc_contra_;        // the record contradicted the replayed state
  std::map<ResId, std::set<std::string>> res_writers_;  // the actors whose recorded writes set a resource
  std::map<int32_t, std::set<std::string>> loc_writers_;
  // The registrations and publications in the record (the execution relation, SPEC 1.1): every effect on a
  // resource that may have been applied, with its target read with the writer's own identity.
  struct Publication {
    int32_t kind = -1;
    std::vector<int64_t> key;
    bool exact = false;   // the target is established
    bool maybe = false;   // the effect may not have been applied (a `filter` guard the record leaves open)
    std::string actor;
  };
  std::vector<Publication> pubs_;
  void note_publication(const OpDecl& op, const EffectDecl& eff, const TraceEvent& ev, const ArgMap* frame_args, bool maybe);
  void relate(Occurrence* o);
  size_t next_loss_ = 0;
  void prepare_real();
  void apply_losses(uint64_t position);
  void untrack_kind(int32_t kind, uint32_t why);
  void untrack_loc(int32_t loc, uint32_t why);
  void contradict(int32_t kind, int32_t loc, uint64_t event, const std::string& what);
  RealEval evaluate_real(const Expr& e, const OpDecl& op, const LpSpec* lp, const TraceEvent& ev, const ArgMap* frame_args, bool identity = false);
  void apply_effect_real(const OpDecl& op, const EffectDecl& eff, const TraceEvent& ev, const ArgMap* frame_args);
  void update_intervals(uint64_t at);
  void join_interval(IntervalRead* acc, const RealEval& r);
  bool overlaps_loss(uint64_t from, uint64_t to) const;
  void return_real(const OpDecl& op, Frame* f, const TraceEvent& ev);
  void check_outcome_real(const OpDecl& op, Frame* f, const TraceEvent& ev);
  void decide_real(Frame* f, size_t k, const TraceEvent& ev);
  void run_monitors();
  void finish();

  void finding(uint64_t event, const std::string& what) {
    if (seen_findings_.insert(std::make_pair(event, what)).second) r_.findings.push_back(ReplayFinding{event, what});
  }

  Status prepare();
  int64_t actor_number(const std::string& actor);
  void set_context(const std::string& actor);
  bool guards_hold(const std::vector<phi::Guard>& guards, uint64_t event, const std::string& what);
  bool evaluate(const Expr& e, const OpDecl& op, const LpSpec* lp, const TraceEvent& ev, const ArgMap* frame_args, CVal* out, bool quiet = false);
  bool rule_applies(size_t ri);

  void begin_event(const TraceEvent& ev);
  void end_event(const TraceEvent& ev);
  void on_call(const TraceEvent& ev);
  void on_return(const TraceEvent& ev);
  void on_point(const TraceEvent& ev);
  void on_role_point(const TraceEvent& ev, size_t role);
  void terminate(const TraceEvent& ev);

  void apply_effect(const OpDecl& op, const EffectDecl& eff, const TraceEvent& ev, const ArgMap* frame_args);
  void eval_conjunct(Frame* f, size_t ci, LpSource source, const TraceEvent& ev, const ArgMap* frame_args);
  void note_write(const ResId& res, const TraceEvent& ev);
  Frame* top_frame_of(const std::string& actor, size_t op);

  void progress(Frame* f);
  void decide(Occurrence* o, OccStatus s, DecidedAt at, uint64_t event);
  OccStatus not_ready_status(const RspecInfo& info) const;
  void sync_occurrence(const Frame& f, size_t k);
  void check_outcome(const OpDecl& op, Frame* f, const TraceEvent& ev);
  void observe_return(const OpDecl& op, const Frame& f, const TraceEvent& ev);
  void spawn_return(const OpDecl& op, const Frame& f, const TraceEvent& ev);
  const Expr* rule_predicate(size_t ri) const;
  bool poll_predicate_holds(size_t ri, const OpDecl& op, const std::string& actor, const ArgMap& args, uint64_t at, bool* holds);
  void open_watches(Frame* f, const OpDecl& op, const TraceEvent& call);
  void update_watches(const TraceEvent& at);

  void final_edges();
  void add_edge(uint64_t from, uint64_t to, const HbRuleDecl& rule) {
    // An edge of an observation rule is read minimally (see HbEdge::minimal).
    r_.edges.push_back(HbEdge{from, to, rule.name, rule.stratum, rule.robust, rule.tmpl == RuleTemplate::Observe});
  }
};

// ---- preparation ----------------------------------------------------------------------------

Status Replayer::prepare() {
  if (!p_.checked) return Status::error(Err::Invalid, "the profile has not been checked");
  st_ = initial_state(p_);

  // ref eval constants
  for (size_t i = 0; i < p_.consts.size(); ++i) {
    const phi::ConstDecl& c = p_.consts[i];
    if (c.value != phi::ConstDecl::Value::RefEval) continue;
    const auto it = t_.ref_consts.find(c.name);
    if (it != t_.ref_consts.end()) st_.consts[static_cast<int32_t>(i)] = it->second;
  }
  // finite tables
  for (size_t ki = 0; ki < p_.kinds.size(); ++ki) {
    const KindDecl& k = p_.kinds[ki];
    if (k.table.empty()) continue;
    const int32_t ci = p_.find_const(k.table);
    if (ci < 0) return Status::error(Err::Invalid, "kind '" + k.name + "': table constant '" + k.table + "' is not declared");
    const phi::ConstDecl& c = p_.consts[static_cast<size_t>(ci)];
    int64_t n = 0;
    if (c.value == phi::ConstDecl::Value::Int) {
      n = c.ival;
    } else if (c.value == phi::ConstDecl::Value::RefEval) {
      const auto it = t_.ref_consts.find(c.name);
      if (it == t_.ref_consts.end())
        return Status::error(Err::Invalid, "kind '" + k.name + "' is a table of size '" + c.name + "', which the trace does not give (const " + c.name + " N)");
      n = it->second;
    } else {
      return Status::error(Err::Invalid, "kind '" + k.name + "': table constant '" + c.name + "' is not an integer");
    }
    if (n < 0 || n > 4096) return Status::error(Err::Invalid, "kind '" + k.name + "': table size " + std::to_string(n) + " is out of range (0 .. 4096)");
    if (key_width(p_, static_cast<int32_t>(ki)) != 1)
      return Status::error(Err::Invalid, "kind '" + k.name + "': the replay supports finite kinds with a one-element key");
    std::vector<std::vector<int64_t>>& inst = st_.instances[static_cast<int32_t>(ki)];
    for (int64_t i = 0; i < n; ++i) inst.push_back({i});
  }

  if (p_.scope.present && p_.scope.kind_index >= 0) {
    const KindDecl& sk = p_.kinds[static_cast<size_t>(p_.scope.kind_index)];
    for (size_t i = 0; i < sk.states.size(); ++i)
      if (sk.states[i] == p_.scope.request) request_ordinal_ = static_cast<int32_t>(i);
  }

  // points: conjuncts and effects by the name of their linearization point
  rspecs_of_op_.assign(p_.ops.size(), {});
  for (size_t oi = 0; oi < p_.ops.size(); ++oi) {
    const OpDecl& op = p_.ops[oi];
    for (size_t ci = 0; ci < op.conjuncts.size(); ++ci) {
      const phi::ConjunctDecl& c = op.conjuncts[ci];
      if (c.lp.kind == LpKind::None) continue;
      points_[lp_name(op, c.label, c.lp)].push_back(PointRef{oi, true, ci});
    }
    for (size_t ei = 0; ei < op.effects.size(); ++ei) {
      const EffectDecl& e = op.effects[ei];
      if (e.lp.kind == LpKind::None) continue;
      points_[lp_name(op, e.label, e.lp)].push_back(PointRef{oi, false, ei});
    }
    if (op.terminal.present && op.terminal.process_terminal && op.terminal.after.present) terminal_points_[terminal_name(op)] = oi;
  }
  for (const phi::SiteDecl& s : p_.sites) site_names_.insert(s.name);
  for (size_t ri = 0; ri < p_.roles.size(); ++ri) role_of_site_[p_.roles[ri].site] = ri;

  // readiness specifications
  for (size_t ri = 0; ri < p_.rspecs.size(); ++ri) {
    const phi::RspecDecl& rs = p_.rspecs[ri];
    if (rs.use_op_index < 0) return Status::error(Err::Invalid, "rspec '" + rs.name + "' has no resolved operation");
    RspecInfo info;
    info.op = static_cast<size_t>(rs.use_op_index);
    info.allowed = phi::derive_caller_allowed(p_, rs);
    const OpDecl& op = p_.ops[info.op];
    std::vector<const Expr*> parts;
    if (rs.requires_) {
      if (rs.requires_->kind == ExprKind::And)
        for (const phi::ExprPtr& a : rs.requires_->args) parts.push_back(a.get());
      else
        parts.push_back(rs.requires_.get());
    }
    for (const Expr* part : parts) {
      const std::string t = phi::expr_text(*part);
      for (size_t ci = 0; ci < op.conjuncts.size(); ++ci)
        if (phi::expr_text(*op.conjuncts[ci].expr) == t) info.conj.push_back(ci);
    }
    std::sort(info.conj.begin(), info.conj.end());
    info.conj.erase(std::unique(info.conj.begin(), info.conj.end()), info.conj.end());
    rspecs_of_op_[info.op].push_back(rspec_info_.size());
    rspec_info_.push_back(std::move(info));
  }

  // rules: the guard decides whether a rule applies
  rule_active_.assign(p_.rules.size(), true);
  for (size_t ri = 0; ri < p_.rules.size(); ++ri) {
    const HbRuleDecl& rule = p_.rules[ri];
    for (const phi::Guard& g : rule.guards) {
      const auto it = o_.versions.find(g.component);
      if (it == o_.versions.end()) {
        rule_active_[ri] = false;
        finding(kNoEvent, "rule " + rule.name + " is not applied: its guard on '" + g.component + "' cannot be evaluated (no version given)");
      } else if (!version_satisfies(it->second, g.op, g.version)) {
        rule_active_[ri] = false;
        finding(kNoEvent, "rule " + rule.name + " is not applied: the guard " + g.component + " " + phi::cmp_text(g.op) + " " + g.version + " does not hold for version " + it->second);
      }
    }
  }
  real_ = t_.record == RecordKind::Real;
  r_.record = t_.record;
  if (real_) prepare_real();
  return Status::success();
}

// ---- real records: what the record fixes ------------------------------------------------------

// Per kind and location: recorded writes fix it only if its start state and lifetime are known, its
// writers and aliases are covered and every writer's point of the profile is observed (ADR-055).  A
// derived kind is never stored (its state is computed from others) and needs nothing.
void Replayer::prepare_real() {
  const size_t nk = p_.kinds.size(), nl = p_.locs.size();
  kind_reasons_.assign(nk, 0);
  loc_reasons_.assign(nl, 0);
  kind_adeq_.assign(nk, 0);
  loc_adeq_.assign(nl, 0);
  kind_contra_.assign(nk, false);
  loc_contra_.assign(nl, false);
  std::vector<std::vector<std::string>> kind_unobserved(nk), loc_unobserved(nl);
  std::vector<std::string> any_loc_unobserved;  // stores through a pointer: any location
  for (const OpDecl& op : p_.ops) {
    for (const EffectDecl& e : op.effects) {
      int32_t kind = -1, loc = -1;
      bool any_loc = false;
      switch (e.kind) {
        case EffectKind::Set:
          kind = !e.all_kind.empty() ? p_.find_kind(e.all_kind) : (e.res ? e.res->sort.kind_index : -1);
          break;
        case EffectKind::Raise:
        case EffectKind::SetTag: kind = e.res ? e.res->sort.kind_index : -1; break;
        case EffectKind::StoreLoc: loc = p_.find_loc(e.target); break;
        case EffectKind::StoreOut: any_loc = true; break;
        default: continue;  // no state change
      }
      const std::string name = e.lp.kind == LpKind::None ? std::string() : lp_name(op, e.label, e.lp);
      if (!name.empty() && t_.observed_points.count(name) != 0) continue;
      const std::string w = op.name + "." + e.label;
      if (kind >= 0 && static_cast<size_t>(kind) < nk) kind_unobserved[static_cast<size_t>(kind)].push_back(w);
      if (loc >= 0 && static_cast<size_t>(loc) < nl) loc_unobserved[static_cast<size_t>(loc)].push_back(w);
      if (any_loc) any_loc_unobserved.push_back(w);
    }
  }
  auto list = [](const std::vector<std::string>& v) {
    std::string out;
    for (const std::string& x : v) out += (out.empty() ? "" : ", ") + x;
    return out;
  };
  for (const auto& kv : t_.kind_adequacy)
    if (p_.find_kind(kv.first) < 0) finding(kNoEvent, "real record: adequacy of '" + kv.first + "', which is not a kind of the profile");
  for (const auto& kv : t_.loc_adequacy)
    if (p_.find_loc(kv.first) < 0) finding(kNoEvent, "real record: adequacy of '" + kv.first + "', which is not a location of the profile");
  for (size_t ki = 0; ki < nk; ++ki) {
    const KindDecl& k = p_.kinds[ki];
    if (!k.derived.empty()) {
      kind_adeq_[ki] = kAllAdequacy;
      continue;
    }
    const auto it = t_.kind_adequacy.find(k.name);
    const Adequacy a = it != t_.kind_adequacy.end() ? it->second : Adequacy();
    uint32_t adeq = KZ_ADEQ_LOSS_FREE, why = 0;
    if (a.start_known) adeq |= KZ_ADEQ_START_STATE_KNOWN; else why |= KZ_WITHHOLD_START_STATE_UNKNOWN;
    if (a.writers_covered) adeq |= KZ_ADEQ_WRITERS_COVERED; else why |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
    if (a.aliases_covered) adeq |= KZ_ADEQ_ALIASES_COVERED; else why |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
    if (a.lifetime_known) adeq |= KZ_ADEQ_LIFETIME_KNOWN; else why |= KZ_WITHHOLD_RESOURCE_OR_LIFETIME_UNMATCHED;
    if (kind_unobserved[ki].empty()) {
      adeq |= KZ_ADEQ_PROBES_ATTACHED | KZ_ADEQ_PROBES_ACTIVE;
    } else {
      why |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
      if (a.writers_covered)
        finding(kNoEvent, "real record: kind " + k.name + " is declared writers=covered, but the point of its writer(s) " + list(kind_unobserved[ki]) +
                              " is not observed: recorded writes do not fix it");
    }
    kind_adeq_[ki] = adeq;
    kind_reasons_[ki] = why;
  }
  for (size_t li = 0; li < nl; ++li) {
    const auto it = t_.loc_adequacy.find(p_.locs[li].name);
    const Adequacy a = it != t_.loc_adequacy.end() ? it->second : Adequacy();
    uint32_t adeq = KZ_ADEQ_LOSS_FREE | KZ_ADEQ_LIFETIME_KNOWN, why = 0;  // a location has no lifetime of its own
    if (a.start_known) adeq |= KZ_ADEQ_START_STATE_KNOWN; else why |= KZ_WITHHOLD_START_STATE_UNKNOWN;
    if (a.writers_covered) adeq |= KZ_ADEQ_WRITERS_COVERED; else why |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
    if (a.aliases_covered) adeq |= KZ_ADEQ_ALIASES_COVERED; else why |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
    std::vector<std::string> unobserved = loc_unobserved[li];
    unobserved.insert(unobserved.end(), any_loc_unobserved.begin(), any_loc_unobserved.end());
    if (unobserved.empty()) {
      adeq |= KZ_ADEQ_PROBES_ATTACHED | KZ_ADEQ_PROBES_ACTIVE;
    } else {
      why |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
      if (a.writers_covered)
        finding(kNoEvent, "real record: location " + p_.locs[li].name + " is declared writers=covered, but the point of its possible writer(s) " + list(unobserved) +
                              " is not observed: recorded writes do not fix it");
    }
    loc_adeq_[li] = adeq;
    loc_reasons_[li] = why;
  }
}

// A reported loss: from its first possible position on, recorded writes fix nothing (ADR-055: no loss is
// recorded for the interval from the start to the read).  The kernel does not say which records were
// lost, so every kind and location is affected.
void Replayer::apply_losses(uint64_t position) {
  bool any = false;
  while (next_loss_ < t_.losses.size() && t_.losses[next_loss_].begin <= position) {
    ++next_loss_;
    any = true;
  }
  if (!any) return;
  for (size_t ki = 0; ki < kind_reasons_.size(); ++ki) {
    if (!p_.kinds[ki].derived.empty()) continue;
    kind_reasons_[ki] |= KZ_WITHHOLD_LOSS;
    kind_adeq_[ki] &= ~static_cast<uint32_t>(KZ_ADEQ_LOSS_FREE);
  }
  for (size_t li = 0; li < loc_reasons_.size(); ++li) {
    loc_reasons_[li] |= KZ_WITHHOLD_LOSS;
    loc_adeq_[li] &= ~static_cast<uint32_t>(KZ_ADEQ_LOSS_FREE);
  }
  update_intervals(position);
}

void Replayer::untrack_kind(int32_t kind, uint32_t why) {
  if (kind < 0 || static_cast<size_t>(kind) >= kind_reasons_.size() || !p_.kinds[static_cast<size_t>(kind)].derived.empty()) return;
  kind_reasons_[static_cast<size_t>(kind)] |= why != 0 ? why : static_cast<uint32_t>(KZ_WITHHOLD_RESOURCE_OR_LIFETIME_UNMATCHED);
}

void Replayer::untrack_loc(int32_t loc, uint32_t why) {
  if (loc < 0 || static_cast<size_t>(loc) >= loc_reasons_.size()) return;
  loc_reasons_[static_cast<size_t>(loc)] |= why != 0 ? why : static_cast<uint32_t>(KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS);
}

// The record contradicts the state the replay fixed: no explanation agrees with both (an analysis input
// error for every result that reads it from now on, SPEC 1.10).
void Replayer::contradict(int32_t kind, int32_t loc, uint64_t event, const std::string& what) {
  if (kind >= 0 && static_cast<size_t>(kind) < kind_contra_.size()) kind_contra_[static_cast<size_t>(kind)] = true;
  if (loc >= 0 && static_cast<size_t>(loc) < loc_contra_.size()) loc_contra_[static_cast<size_t>(loc)] = true;
  r_.partial = true;
  finding(event, "real record: " + what + ": the record contradicts the replayed state (analysis input error for what reads it)");
}

bool Replayer::overlaps_loss(uint64_t from, uint64_t to) const {
  for (const LossInterval& l : t_.losses)
    if (l.begin <= to && l.end > from) return true;
  return false;
}

// Evaluates `e` at `ev` over the explanations of the real record: the abstract domain over the replayed
// state of what the record fixes; everything else is open (unknown), and so is a value the event lacks.
// With `identity`, the id of the executing actor's owner record is the identity value the record gives for
// its app_owner (Trace::owner_ids), whatever the replayed state says: this names the resource an effect
// targets by the writer's identity (the execution relation) and is never used to fix the state.
RealEval Replayer::evaluate_real(const Expr& e, const OpDecl& op, const LpSpec* lp, const TraceEvent& ev, const ArgMap* frame_args, bool identity) {
  constexpr int64_t kLo = std::numeric_limits<int64_t>::min(), kHi = std::numeric_limits<int64_t>::max();
  RealEval out;
  std::vector<std::string> names;
  for (const phi::OpParam& prm : op.params)
    if (prm.role != "out") names.push_back("arg." + prm.name);
  if (lp != nullptr)
    for (const phi::OpParam& v : lp->values) names.push_back("lp." + v.name);
  Used used;
  collect_used(e, &used);
  AbstractState a;
  for (size_t i = 0; i < names.size(); ++i) {
    auto it = ev.values.find(names[i]);
    bool found = it != ev.values.end();
    if (!found && frame_args != nullptr) {
      it = frame_args->find(names[i]);
      found = it != frame_args->end();
    }
    if (found) {
      a.args.push_back({it->second, it->second});
    } else {
      a.args.push_back({kLo, kHi});  // not recorded: any value
      if (used.args.count(static_cast<int32_t>(i)) != 0) out.reasons |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
    }
  }
  if (ev.has_result) {
    a.result = {ev.result, ev.result};
  } else {
    a.result = {kLo, kHi};
    if (used.result) out.reasons |= KZ_WITHHOLD_EFFECT_UNSUPPORTED;
  }
  const int64_t me = actor_number(ev.actor);
  a.exec = {me, me};
  // the owner: as the record states it, with an evidence level; otherwise undetermined
  bool owner_open = false;
  const auto lv = t_.owner_levels.find(ev.actor);
  const uint8_t level = lv != t_.owner_levels.end() ? lv->second : static_cast<uint8_t>(KZ_OWNER_LEVEL_UNDETERMINED);
  if (level == KZ_OWNER_LEVEL_UNDETERMINED) {
    a.owner_any = true;
    owner_open = true;
  } else {
    const auto ow = t_.owners.find(ev.actor);
    a.owner_none = true;
    if (ow != t_.owners.end() && p_.context.present && p_.context.owner_kind_index >= 0 && ow->second.size() == key_width(p_, p_.context.owner_kind_index)) {
      a.owners.push_back(ResId{p_.context.owner_kind_index, ow->second});
      a.owner_none = false;
    }
  }
  for (const auto& c : st_.consts) a.consts[c.first] = {c.second, c.second};
  a.instances = st_.instances;
  for (size_t ki = 0; ki < kind_reasons_.size(); ++ki)
    if (kind_reasons_[ki] != 0) a.open_kinds.insert(static_cast<int32_t>(ki));
  auto fixed = [&](const ResId& id) { return id.kind >= 0 && static_cast<size_t>(id.kind) < kind_reasons_.size() && kind_reasons_[static_cast<size_t>(id.kind)] == 0; };
  auto entry = [&](const ResId& id) -> AbstractRes& {
    auto it = a.res.find(id);
    if (it == a.res.end()) {
      AbstractRes r;
      const auto st = st_.state.find(id);
      r.states = 1ull << (st != st_.state.end() ? st->second : p_.kinds[static_cast<size_t>(id.kind)].initial_ordinal);
      const auto idv = st_.ids.find(id);
      if (idv != st_.ids.end()) {
        r.has_id = true;
        r.id = {idv->second, idv->second};
      }
      it = a.res.emplace(id, r).first;
    }
    return it->second;
  };
  for (const auto& kv : st_.state)
    if (fixed(kv.first)) entry(kv.first);
  for (const auto& kv : st_.ids)
    if (fixed(kv.first)) entry(kv.first);
  for (const auto& kv : st_.tags)
    if (fixed(kv.first.first)) entry(kv.first.first).tags[kv.first.second] = 1ull << kv.second;
  // Order (SPEC 4.8, 5.8): the merged order of a record is not the execution order of different actors.
  // A resource or location that an actor other than the reader wrote in the record may hold any of the
  // values of the interleavings, unless the record declares the order of its events established.
  auto foreign = [&](const std::set<std::string>& writers) {
    for (const std::string& w : writers)
      if (w != ev.actor) return true;
    return false;
  };
  std::set<ResId> order_open;
  std::set<int32_t> order_open_kinds, order_open_locs;
  if (!t_.order_established) {
    for (const auto& kv : res_writers_) {
      if (!fixed(kv.first) || !foreign(kv.second)) continue;
      const KindDecl& k = p_.kinds[static_cast<size_t>(kv.first.kind)];
      AbstractRes any;
      any.states = k.states.size() >= 64 ? ~0ull : ((1ull << k.states.size()) - 1);
      for (size_t ti = 0; ti < k.tags.size(); ++ti)
        any.tags[static_cast<int32_t>(ti)] = k.tags[ti].values.size() >= 64 ? ~0ull : ((1ull << k.tags[ti].values.size()) - 1);
      if (k.ids.present) {
        any.has_id = true;
        any.id = {0, 0xFFFFFFFFll};
      }
      a.res[kv.first] = any;
      order_open.insert(kv.first);
      order_open_kinds.insert(kv.first.kind);
    }
    for (const auto& kv : loc_writers_)
      if (foreign(kv.second)) order_open_locs.insert(kv.first);
  }
  if (identity && !a.owners.empty()) {
    const auto oid = t_.owner_ids.find(ev.actor);
    if (oid != t_.owner_ids.end()) {
      AbstractRes& r = a.res[a.owners.front()];
      if (r.states == 0) {
        const KindDecl& k = p_.kinds[static_cast<size_t>(a.owners.front().kind)];
        r.states = k.states.size() >= 64 ? ~0ull : ((1ull << k.states.size()) - 1);
        for (size_t ti = 0; ti < k.tags.size(); ++ti)
          r.tags[static_cast<int32_t>(ti)] = k.tags[ti].values.size() >= 64 ? ~0ull : ((1ull << k.tags[ti].values.size()) - 1);
      }
      r.has_id = true;
      r.id = {static_cast<int64_t>(oid->second), static_cast<int64_t>(oid->second)};
    }
  }
  a.open_locs = true;
  for (size_t li = 0; li < loc_reasons_.size(); ++li) {
    if (loc_reasons_[li] != 0 || order_open_locs.count(static_cast<int32_t>(li)) != 0) continue;
    const auto lv2 = st_.loc_values.find(static_cast<int32_t>(li));
    const int64_t v = lv2 != st_.loc_values.end() ? lv2->second : 0;
    a.locs[static_cast<int32_t>(li)] = {v, v};
  }
  Reads reads;
  RecordingDomain dom(p_, a, &reads);
  Evaluator<RecordingDomain> evaluator(p_, dom);
  out.v = evaluator.eval(e);
  for (int32_t k : reads.kinds) {
    if (k < 0 || static_cast<size_t>(k) >= kind_reasons_.size()) continue;
    out.reasons |= kind_reasons_[static_cast<size_t>(k)];
    out.adequacy &= kind_adeq_[static_cast<size_t>(k)];
    out.contradicted = out.contradicted || kind_contra_[static_cast<size_t>(k)];
  }
  for (const ResId& id : reads.res)
    if (order_open.count(id) != 0) out.reasons |= KZ_WITHHOLD_EXPLANATION_AMBIGUOUS;
  for (int32_t k : reads.boxes)
    if (order_open_kinds.count(k) != 0) out.reasons |= KZ_WITHHOLD_EXPLANATION_AMBIGUOUS;
  for (int32_t l : reads.locs) {
    if (order_open_locs.count(l) != 0) out.reasons |= KZ_WITHHOLD_EXPLANATION_AMBIGUOUS;
    out.reasons |= loc_reasons_[static_cast<size_t>(l)];
    out.adequacy &= loc_adeq_[static_cast<size_t>(l)];
    out.contradicted = out.contradicted || loc_contra_[static_cast<size_t>(l)];
  }
  if (reads.owner && owner_open) out.reasons |= KZ_WITHHOLD_OWNERSHIP_UNDETERMINED;
  return out;
}

void Replayer::join_interval(IntervalRead* acc, const RealEval& r) {
  if (!acc->any) {
    acc->tri = r.v.tri;
    acc->any = true;
  } else if (acc->tri != r.v.tri) {
    if (acc->tri != Tri::Maybe && r.v.tri != Tri::Maybe) acc->reasons |= KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS;  // two values over the call
    acc->tri = Tri::Maybe;
  }
  acc->reasons |= r.reasons;
  acc->adequacy &= r.adequacy;
  acc->contradicted = acc->contradicted || r.contradicted;
}

// The conjuncts read over a call (their point is not observed) see every state the call can see.
void Replayer::update_intervals(uint64_t at) {
  for (auto& stack : frames_) {
    for (Frame& fr : stack.second) {
      const OpDecl& op = p_.ops[fr.op];
      for (size_t ci = 0; ci < fr.intervals.size(); ++ci) {
        if (!fr.intervals[ci].active) continue;
        TraceEvent probe;  // no values of its own: the arguments are those of the call
        probe.id = at;
        probe.actor = stack.first;
        probe.site = op.name;
        join_interval(&fr.intervals[ci], evaluate_real(*op.conjuncts[ci].expr, op, &op.conjuncts[ci].lp, probe, &fr.args));
      }
    }
  }
}

int64_t Replayer::actor_number(const std::string& actor) {
  const auto it = actor_no_.find(actor);
  if (it != actor_no_.end()) return it->second;
  const int64_t n = static_cast<int64_t>(actor_no_.size()) + 1;
  actor_no_[actor] = n;
  return n;
}

void Replayer::set_context(const std::string& actor) {
  st_.exec = actor_number(actor);
  const auto it = t_.owners.find(actor);
  if (it != t_.owners.end() && p_.context.present && p_.context.owner_kind_index >= 0) {
    const size_t width = key_width(p_, p_.context.owner_kind_index);
    if (it->second.size() != width) {
      // Not a record of the owner kind: the actor is treated as having no owner (both domains read
      // such an owner as undefined, eval.hpp).
      finding(kNoEvent, "the owner of actor " + actor + " has " + std::to_string(it->second.size()) + " key element(s), kind " +
                            p_.kinds[static_cast<size_t>(p_.context.owner_kind_index)].name + " has " + std::to_string(width) + ": the actor has no owner in the replay");
      st_.has_owner = false;
      st_.owner = ResId();
      return;
    }
    st_.has_owner = true;
    st_.owner = ResId{p_.context.owner_kind_index, it->second};
  } else {
    st_.has_owner = false;
    st_.owner = ResId();
  }
}

bool Replayer::guards_hold(const std::vector<phi::Guard>& guards, uint64_t event, const std::string& what) {
  for (const phi::Guard& g : guards) {
    const auto it = o_.versions.find(g.component);
    const bool ok = it != o_.versions.end() && version_satisfies(it->second, g.op, g.version);
    if (!ok) {
      if (guard_reported_.insert(what).second) finding(event, what + " is not applied: its guard on '" + g.component + "' does not hold or cannot be evaluated");
      return false;
    }
  }
  return true;
}

bool Replayer::rule_applies(size_t ri) { return rule_active_[ri]; }

// Evaluates `e` at `ev`: the arguments are those of the operation's non-out parameters, then the
// values of the point, named lp.NAME.  A value the expression needs and the event lacks is a
// finding and the expression is not evaluated.
bool Replayer::evaluate(const Expr& e, const OpDecl& op, const LpSpec* lp, const TraceEvent& ev, const ArgMap* frame_args, CVal* out, bool quiet) {
  std::vector<std::string> names;
  for (const phi::OpParam& prm : op.params)
    if (prm.role != "out") names.push_back("arg." + prm.name);
  if (lp != nullptr)
    for (const phi::OpParam& v : lp->values) names.push_back("lp." + v.name);
  Used used;
  collect_used(e, &used);
  std::vector<int64_t> args(names.size(), 0);
  for (size_t i = 0; i < names.size(); ++i) {
    const auto it = ev.values.find(names[i]);
    if (it != ev.values.end()) {
      args[i] = it->second;
      continue;
    }
    if (frame_args != nullptr) {
      const auto jt = frame_args->find(names[i]);
      if (jt != frame_args->end()) {
        args[i] = jt->second;
        continue;
      }
    }
    if (used.args.count(static_cast<int32_t>(i)) != 0) {
      if (!quiet) finding(ev.id, "event " + ev.site + " lacks the value " + names[i] + " that an expression of operation " + op.name + " reads");
      return false;
    }
  }
  if (used.result) {
    if (!ev.has_result) {
      if (!quiet) finding(ev.id, "event " + ev.site + " lacks the result that an expression of operation " + op.name + " reads");
      return false;
    }
    st_.result = ev.result;
  }
  set_context(ev.actor);
  st_.args = args;
  ConcreteDomain dom(p_, st_);
  Evaluator<ConcreteDomain> evaluator(p_, dom);
  *out = evaluator.eval(e);
  return true;
}

Frame* Replayer::top_frame_of(const std::string& actor, size_t op) {
  const auto it = frames_.find(actor);
  if (it == frames_.end()) return nullptr;
  for (size_t i = it->second.size(); i > 0; --i)
    if (it->second[i - 1].op == op) return &it->second[i - 1];
  return nullptr;
}

// ---- the main loop ----------------------------------------------------------------------------

Status Replayer::run() {
  Status s = prepare();
  if (!s.ok()) return s;
  for (const TraceEvent& ev : t_.events) {
    if (r_.terminated) {
      ++after_terminal_;
      continue;
    }
    if (real_) apply_losses(ev.id);
    begin_event(ev);
    switch (ev.phase) {
      case Phase::Call: on_call(ev); break;
      case Phase::Return: on_return(ev); break;
      case Phase::Point: on_point(ev); break;
    }
    end_event(ev);
  }
  if (after_terminal_ != 0)
    finding(r_.terminal_event, std::to_string(after_terminal_) + " event(s) follow the process-terminal step and are not replayed (a real run has none)");
  final_edges();
  std::sort(r_.edges.begin(), r_.edges.end());
  r_.edges.erase(std::unique(r_.edges.begin(), r_.edges.end()), r_.edges.end());
  r_.final_state = st_;
  finish();
  return Status::success();
}

void Replayer::begin_event(const TraceEvent& ev) {
  began_handler_ = false;
  actor_number(ev.actor);
  if (first_event_.find(ev.actor) == first_event_.end()) first_event_[ev.actor] = ev.id;
  // the other branch of a poll: the next call of the actor must be the terminal operation
  const auto ex = expect_terminal_.find(ev.actor);
  if (ex != expect_terminal_.end() && ev.phase == Phase::Call) {
    const HbRuleDecl& rule = p_.rules[ex->second.first];
    if (ev.site != rule.else_terminal)
      finding(ev.id, "rule " + rule.name + ": the poll " + rule.poll_fn + " (#" + std::to_string(ex->second.second) + ") did not reach its exit outcome, and the next call of the actor is " + ev.site +
                         ", not the terminal operation " + rule.else_terminal);
    expect_terminal_.erase(ex);
  }
}

void Replayer::end_event(const TraceEvent& ev) {
  // program order
  const auto it = last_event_.find(ev.actor);
  if (it != last_event_.end()) {
    for (size_t ri = 0; ri < p_.rules.size(); ++ri) {
      const HbRuleDecl& rule = p_.rules[ri];
      if (rule.tmpl != RuleTemplate::ProgramOrder || !rule_applies(ri)) continue;
      if (rule.looper && began_handler_) continue;
      add_edge(it->second, ev.id, rule);
    }
  }
  last_event_[ev.actor] = ev.id;
}

void Replayer::terminate(const TraceEvent& ev) {
  r_.terminated = true;
  r_.terminal_event = ev.id;
}

// ---- calls ----------------------------------------------------------------------------------

void Replayer::on_call(const TraceEvent& ev) {
  const int32_t oi32 = p_.find_op(ev.site);
  if (oi32 < 0) {
    finding(ev.id, "call of '" + ev.site + "', which is not an operation of the profile");
    return;
  }
  const size_t oi = static_cast<size_t>(oi32);
  const OpDecl& op = p_.ops[oi];
  Frame f;
  f.op = oi;
  f.call = ev.id;
  f.args = ev.values;
  // A string argument (a parameter of sort Str) is given as text (`arg.NAME=@text`); expressions read
  // its handle (eval.hpp str_handle), the value a string constant of the profile has.
  for (const phi::OpParam& prm : op.params) {
    if (prm.role == "out" || !prm.has_sort || prm.sort.resolved.kind != phi::SortKind::Str) continue;
    const auto t = ev.texts.find("arg." + prm.name);
    if (t != ev.texts.end() && f.args.count("arg." + prm.name) == 0) f.args["arg." + prm.name] = str_handle(t->second);
  }
  f.conj.resize(op.conjuncts.size());
  for (size_t ci = 0; ci < op.conjuncts.size(); ++ci) f.conj[ci].label = op.conjuncts[ci].label;

  // value conjuncts: the capture attached to this call, else the call itself
  for (size_t ci = 0; ci < op.conjuncts.size(); ++ci) {
    if (op.conjuncts[ci].lp.kind != LpKind::Capture) continue;
    const auto key = std::make_tuple(ev.actor, oi, ci);
    const auto it = captures_.find(key);
    if (it != captures_.end()) {
      f.conj[ci] = it->second;
      captures_.erase(it);
    } else {
      eval_conjunct(&f, ci, LpSource::Call, ev, &f.args);
    }
  }

  // real records: a conjunct whose point the record does not observe is read over the whole call
  if (real_) {
    f.intervals.resize(op.conjuncts.size());
    for (size_t ci = 0; ci < op.conjuncts.size(); ++ci) {
      const phi::ConjunctDecl& c = op.conjuncts[ci];
      if (c.lp.kind == LpKind::Capture) continue;
      if (c.lp.kind != LpKind::None && t_.observed_points.count(lp_name(op, c.label, c.lp)) != 0) continue;
      f.intervals[ci].active = true;
      join_interval(&f.intervals[ci], evaluate_real(*c.expr, op, &c.lp, ev, &f.args));
    }
  }

  // occurrences
  for (size_t si : rspecs_of_op_[oi]) {
    const phi::RspecDecl& rs = p_.rspecs[si];
    Occurrence o;
    o.rspec = rs.name;
    o.actor = ev.actor;
    o.call = ev.id;
    o.key_known = !rs.key;
    if (real_) {
      if (rs.key) {
        const RealEval kr = evaluate_real(*rs.key, op, nullptr, ev, &f.args);
        std::vector<int64_t> k;
        if (exact_key(kr.v, &k)) {
          o.key = k;
          o.key_known = true;
        } else {
          o.reasons |= kr.reasons != 0 ? kr.reasons : static_cast<uint32_t>(KZ_WITHHOLD_RESOURCE_OR_LIFETIME_UNMATCHED);
        }
      }
      // the identities the record states (SPEC 1.2): creator, owner with its level, the passed identifier
      const auto cr = t_.creators.find(ev.actor);
      if (cr != t_.creators.end()) o.creator = cr->second;
      const auto lv = t_.owner_levels.find(ev.actor);
      if (lv != t_.owner_levels.end()) o.owner_level = lv->second;
      const auto ow = t_.owners.find(ev.actor);
      if (ow != t_.owners.end() && o.owner_level != KZ_OWNER_LEVEL_UNDETERMINED) {
        o.has_owner_key = true;
        o.owner_key = ow->second;
      }
      for (const phi::OpParam& prm : op.params) {
        if (prm.role != "stamp") continue;
        o.passed_role = prm.name;
        const auto pv = ev.values.find("arg." + prm.name);
        if (pv != ev.values.end()) {
          o.has_passed = true;
          o.passed = pv->second;
        }
      }
    } else {
      CVal key;
      if (rs.key && evaluate(*rs.key, op, nullptr, ev, &f.args, &key) && key.defined) {
        o.key = key.key;
        o.key_known = true;
      }
    }
    o.ordinal = ++ordinals_[ev.actor + "\n" + rs.name + "\n" + key_text(o.key)];
    for (size_t ci : rspec_info_[si].conj) o.conjuncts.push_back(f.conj[ci]);
    f.occs.push_back(r_.occurrences.size());
    r_.occurrences.push_back(std::move(o));
  }
  progress(&f);
  open_watches(&f, op, ev);
  frames_[ev.actor].push_back(std::move(f));

  // spawn rules: the create operation and the entry of the child
  for (size_t ri = 0; ri < p_.rules.size(); ++ri) {
    const HbRuleDecl& rule = p_.rules[ri];
    if (rule.tmpl != RuleTemplate::Spawn || !rule_applies(ri)) continue;
    if (rule.create_op == op.name) creates_.push_back(CreateRec{ri, ev.actor, ev.id, false, 0});
    if (rule.entry_op == op.name) {
      EntryRec e;
      e.rule = ri;
      e.event = ev.id;
      e.actor = ev.actor;
      const auto it = ev.values.find("arg." + rule.create_out);
      if (it != ev.values.end()) {
        e.has_value = true;
        e.value = it->second;
      }
      entries_.push_back(e);
      if (first_event_[ev.actor] != ev.id) finding(ev.id, "rule " + rule.name + ": the entry " + op.name + " is not the first event of actor " + ev.actor);
    }
  }

  if (op.terminal.present && op.terminal.process_terminal && !op.terminal.after.present) terminate(ev);
}

void Replayer::eval_conjunct(Frame* f, size_t ci, LpSource source, const TraceEvent& ev, const ArgMap* frame_args) {
  const OpDecl& op = p_.ops[f->op];
  const phi::ConjunctDecl& c = op.conjuncts[ci];
  ConjunctResult& res = f->conj[ci];
  if (res.evaluated || res.unknown) return;  // the first pass of the linearization point decides
  if (real_) {
    const RealEval r = evaluate_real(*c.expr, op, &c.lp, ev, frame_args);
    res.source = source;
    res.event = ev.id;
    res.reasons = r.reasons;
    res.adequacy = r.adequacy;
    res.contradicted = r.contradicted;
    if (r.v.tri == Tri::Maybe) {
      res.unknown = true;
    } else {
      res.evaluated = true;
      res.holds = r.v.tri == Tri::True;
    }
    if (ci < f->intervals.size()) f->intervals[ci].active = false;  // the point was observed after all: it was passed
    return;
  }
  CVal v;
  if (!evaluate(*c.expr, op, &c.lp, ev, frame_args, &v)) return;
  res.source = source;
  res.event = ev.id;
  res.evaluated = true;
  res.holds = v.is_true();
}

void Replayer::sync_occurrence(const Frame& f, size_t k) {
  Occurrence& o = r_.occurrences[f.occs[k]];
  const size_t si = rspecs_of_op_[f.op][k];
  o.conjuncts.clear();
  for (size_t ci : rspec_info_[si].conj) o.conjuncts.push_back(f.conj[ci]);
}

OccStatus Replayer::not_ready_status(const RspecInfo& info) const {
  switch (info.allowed.kind) {
    case phi::CallerAllowedKind::None: return OccStatus::Violating;
    case phi::CallerAllowedKind::Unspecified: return OccStatus::NotReadyUnjudged;
    case phi::CallerAllowedKind::Entries: return OccStatus::Pending;
  }
  return OccStatus::NotReadyUnjudged;
}

void Replayer::decide(Occurrence* o, OccStatus s, DecidedAt at, uint64_t event) {
  o->status = s;
  o->decided_at = at;
  o->decision = event;
  o->in_scope = !(r_.shutdown_requested && r_.shutdown_event < event);
}

// Decides the occurrences of a frame as far as the conjunct results allow (SPEC 1.6, Instances):
// the earliest false conjunct decides "not ready", all conjuncts true decide READY.
void Replayer::progress(Frame* f) {
  for (size_t k = 0; k < f->occs.size(); ++k) {
    sync_occurrence(*f, k);
    Occurrence& o = r_.occurrences[f->occs[k]];
    if (o.decided_at != DecidedAt::None) continue;
    const RspecInfo& info = rspec_info_[rspecs_of_op_[f->op][k]];
    // the earliest false conjunct, in the order of the events that evaluated them
    const ConjunctResult* bad = nullptr;
    bool all_true = !info.conj.empty();
    for (size_t ci : info.conj) {
      const ConjunctResult& c = f->conj[ci];
      if (!c.evaluated || c.passage_unknown) {  // a real record that does not show whether the point was passed decides nothing here
        all_true = false;
        continue;
      }
      if (!c.holds) {
        all_true = false;
        if (bad == nullptr || c.event < bad->event) bad = &c;
      }
    }
    if (bad != nullptr) {
      // (a conjunct fixed by the returned code of a real record is decided at the return)
      const DecidedAt at = bad->source == LpSource::Point ? DecidedAt::Point : bad->source == LpSource::Outcome ? DecidedAt::Return : DecidedAt::Call;
      decide(&o, not_ready_status(info), at, at == DecidedAt::Call ? f->call : bad->event);
    } else if (all_true) {
      uint64_t last = f->call;
      bool in_call_only = true, by_outcome = false;
      for (size_t ci : info.conj) {
        if (f->conj[ci].source == LpSource::Point || f->conj[ci].source == LpSource::Outcome) {
          in_call_only = false;
          by_outcome = by_outcome || f->conj[ci].source == LpSource::Outcome;
          last = std::max(last, f->conj[ci].event);
        }
      }
      decide(&o, OccStatus::Ready, in_call_only ? DecidedAt::Call : by_outcome ? DecidedAt::Return : DecidedAt::Point, in_call_only ? f->call : last);
    }
  }
}

// ---- returns --------------------------------------------------------------------------------

void Replayer::on_return(const TraceEvent& ev) {
  const int32_t oi32 = p_.find_op(ev.site);
  if (oi32 < 0) {
    finding(ev.id, "return of '" + ev.site + "', which is not an operation of the profile");
    return;
  }
  const size_t oi = static_cast<size_t>(oi32);
  const OpDecl& op = p_.ops[oi];
  std::vector<Frame>& stack = frames_[ev.actor];
  size_t pos = stack.size();
  while (pos > 0 && stack[pos - 1].op != oi) --pos;
  if (pos == 0) {
    finding(ev.id, "return of " + op.name + " without an open call of the actor " + ev.actor);
    return;
  }
  for (size_t i = stack.size(); i > pos; --i)
    finding(ev.id, "the call of " + p_.ops[stack[i - 1].op].name + " (#" + std::to_string(stack[i - 1].call) + ") did not return before the return of " + op.name);
  stack.resize(pos);
  Frame f = std::move(stack.back());
  stack.pop_back();

  if (real_) {
    return_real(op, &f, ev);
    observe_return(op, f, ev);
    spawn_return(op, f, ev);
    return;
  }

  // occurrences: the return decides what is still open
  for (size_t k = 0; k < f.occs.size(); ++k) {
    sync_occurrence(f, k);
    Occurrence& o = r_.occurrences[f.occs[k]];
    o.returned = true;
    o.ret = ev.id;
    o.has_result = ev.has_result;
    o.result = ev.result;
    if (o.decided_at == DecidedAt::None) {
      // Not decided by a call, a capture or a conjunct point: the call returned before one of the
      // conjunct points, with no conjunct found false.  The decision event is the last point passed.
      uint64_t last = f.call;
      for (size_t ci : rspec_info_[rspecs_of_op_[f.op][k]].conj)
        if (f.conj[ci].evaluated) last = std::max(last, f.conj[ci].event);
      decide(&o, OccStatus::NotEvaluated, DecidedAt::Return, last);
    }
  }
  check_outcome(op, &f, ev);
  observe_return(op, f, ev);
  spawn_return(op, f, ev);
}

// spawn: the value the create operation returned through its out parameter
void Replayer::spawn_return(const OpDecl& op, const Frame& f, const TraceEvent& ev) {
  for (CreateRec& c : creates_) {
    if (c.call != f.call) continue;
    const phi::HbRuleDecl& rule = p_.rules[c.rule];
    const auto it = ev.values.find("out." + rule.create_out);
    if (it != ev.values.end()) {
      c.has_value = true;
      c.value = it->second;
    } else {
      finding(ev.id, "rule " + rule.name + ": the return of " + op.name + " lacks out." + rule.create_out + ", so the child it created cannot be named");
    }
  }
  // outcome values of the out parameter reach the state through effects, not here
}

// O4(a): the code the call returned is mapped by the outcome list of the operation; the class of
// the outcome must agree with the readiness the replay computed.  An `error` outcome makes no
// statement about readiness and is not checked.
void Replayer::check_outcome(const OpDecl& op, Frame* f, const TraceEvent& ev) {
  OutcomeCheck chk = OutcomeCheck::NotChecked;
  if (!op.outcomes.empty() && ev.has_result) {
    const phi::OutcomeDecl* hit = nullptr;
    for (const phi::OutcomeDecl& oc : op.outcomes) {
      if (oc.code_const < 0) continue;
      const phi::ConstDecl& c = p_.consts[static_cast<size_t>(oc.code_const)];
      if (c.value == phi::ConstDecl::Value::Int && u32(c.ival) == u32(ev.result)) {
        hit = &oc;
        break;
      }
    }
    if (hit == nullptr) {
      chk = OutcomeCheck::Unmapped;
      finding(ev.id, op.name + " returned " + hex_or_dec(static_cast<int64_t>(u32(ev.result))) + ", which its outcome list does not map");
    } else {
      switch (hit->cls) {
        case OutcomeClass::Ready: {
          bool false_conjunct = false;
          for (const ConjunctResult& c : f->conj)
            if (c.evaluated && !c.holds) false_conjunct = true;
          chk = false_conjunct ? OutcomeCheck::Disagrees : OutcomeCheck::Agrees;
          if (false_conjunct) finding(ev.id, op.name + " returned " + hit->code + " (ready), and a conjunct of the call is false in the replay");
          break;
        }
        case OutcomeClass::NotReady: {
          const bool reported = hit->conjunct_index >= 0 && f->conj[static_cast<size_t>(hit->conjunct_index)].evaluated &&
                                !f->conj[static_cast<size_t>(hit->conjunct_index)].holds;
          chk = reported ? OutcomeCheck::Agrees : OutcomeCheck::Disagrees;
          if (!reported) finding(ev.id, op.name + " returned " + hit->code + " (not ready: " + hit->conjunct + "), and the replay does not find that conjunct false");
          break;
        }
        case OutcomeClass::Error: break;
      }
    }
  }
  for (size_t idx : f->occs) r_.occurrences[idx].outcome = chk;
}

// An observation rule: a poll that returns its exit outcome was ordered after the write that made
// its awaited predicate true.  The writers are the last writes, before the return, to a resource
// of a watched kind; writes by the polling actor itself are program order and give no edge.
void Replayer::observe_return(const OpDecl& op, const Frame& f, const TraceEvent& ev) {
  for (size_t ri = 0; ri < p_.rules.size(); ++ri) {
    const HbRuleDecl& rule = p_.rules[ri];
    if (rule.tmpl != RuleTemplate::Observe || !rule_applies(ri) || rule.poll_fn != op.name) continue;
    if (!ev.has_result) {
      finding(ev.id, "rule " + rule.name + ": the return of the poll " + op.name + " carries no result");
      continue;
    }
    const int32_t ci = p_.find_const(rule.exit_outcome);
    if (ci < 0) continue;
    if (u32(ev.result) != u32(p_.consts[static_cast<size_t>(ci)].ival)) {
      if (!rule.else_terminal.empty()) expect_terminal_[ev.actor] = std::make_pair(ri, ev.id);
      continue;
    }
    const Expr* pred = rule_predicate(ri);
    if (pred == nullptr) {
      if (predicate_missing_reported_.insert(ri).second)
        finding(ev.id, "rule " + rule.name + ": the awaited predicate comes from the code and none was supplied, so no observation edge is derived");
      continue;
    }
    if (real_) {
      const RealEval rv = evaluate_real(*pred, op, nullptr, ev, &f.args);
      if (rv.v.tri != Tri::True) {
        finding(ev.id, "rule " + rule.name + ": the poll " + op.name + " returned its exit outcome while its awaited predicate is " +
                           (rv.v.tri == Tri::False ? "false in every explanation of the real record" : "not fixed by the real record") + ": no observation edge");
        continue;
      }
    } else {
      CVal v;
      if (!evaluate(*pred, op, nullptr, ev, &f.args, &v)) continue;
      if (!v.is_true()) {
        finding(ev.id, "rule " + rule.name + ": the poll " + op.name + " returned its exit outcome while its awaited predicate is false in the replay");
        continue;
      }
    }
    // The writes the scan that let the poll exit can have seen: the last writes at the moment the
    // predicate became true.  Without that moment (the watch could not evaluate the predicate) the
    // last writes before the return are used, which can include a write that landed after the scan.
    const PollWatch* watch = nullptr;
    for (const PollWatch& w : f.watches)
      if (w.rule == ri) watch = &w;
    const std::map<ResId, WriteInfo>& writes = watch != nullptr && watch->truth ? watch->seen : last_write_;
    for (const auto& w : writes) {
      if (w.second.actor == ev.actor) continue;
      const std::string& kn = p_.kinds[static_cast<size_t>(w.first.kind)].name;
      if (std::find(rule.watch_kinds.begin(), rule.watch_kinds.end(), kn) == rule.watch_kinds.end()) continue;
      add_edge(w.second.event, ev.id, rule);
    }
  }
}

// The awaited predicate of an observation rule: the one in the profile, else the one the caller supplied.
const Expr* Replayer::rule_predicate(size_t ri) const {
  const HbRuleDecl& rule = p_.rules[ri];
  if (rule.predicate) return rule.predicate.get();
  const auto it = o_.predicates.find(rule.name);
  return it != o_.predicates.end() ? it->second : nullptr;
}

// Evaluates the awaited predicate of rule `ri` in the state as it is, with the arguments of the open
// call of `actor`.  False if there is no predicate or it cannot be evaluated (no finding is made:
// the return of the poll makes it).
bool Replayer::poll_predicate_holds(size_t ri, const OpDecl& op, const std::string& actor, const ArgMap& args, uint64_t at, bool* holds) {
  const Expr* pred = rule_predicate(ri);
  if (pred == nullptr) return false;
  TraceEvent probe;  // no values of its own: the arguments are those of the call
  probe.id = at;
  probe.actor = actor;
  probe.site = op.name;
  if (real_) {
    const RealEval r = evaluate_real(*pred, op, nullptr, probe, &args);
    if (r.v.tri == Tri::Maybe) return false;  // not fixed by the record: no moment
    *holds = r.v.tri == Tri::True;
    return true;
  }
  CVal v;
  if (!evaluate(*pred, op, nullptr, probe, &args, &v, true)) return false;
  *holds = v.is_true();
  return true;
}

void Replayer::open_watches(Frame* f, const OpDecl& op, const TraceEvent& call) {
  for (size_t ri = 0; ri < p_.rules.size(); ++ri) {
    const HbRuleDecl& rule = p_.rules[ri];
    if (rule.tmpl != RuleTemplate::Observe || !rule_applies(ri) || rule.poll_fn != op.name) continue;
    PollWatch w;
    w.rule = ri;
    bool holds = false;
    if (poll_predicate_holds(ri, op, call.actor, f->args, call.id, &holds) && holds) {
      w.truth = true;
      w.seen = last_write_;
    }
    f->watches.push_back(std::move(w));
  }
}

// After an effect: a predicate that became true has its moment; one that became false loses it.
void Replayer::update_watches(const TraceEvent& at) {
  for (auto& stack : frames_) {
    for (Frame& fr : stack.second) {
      for (PollWatch& w : fr.watches) {
        bool holds = false;
        if (!poll_predicate_holds(w.rule, p_.ops[fr.op], stack.first, fr.args, at.id, &holds)) continue;
        if (holds && !w.truth) {
          w.truth = true;
          w.seen = last_write_;
        } else if (!holds) {
          w.truth = false;
          w.seen.clear();
        }
      }
    }
  }
}

// ---- points ---------------------------------------------------------------------------------

void Replayer::note_write(const ResId& res, const TraceEvent& ev) { last_write_[res] = WriteInfo{ev.id, ev.actor}; }

namespace {

int32_t state_ordinal(const KindDecl& k, const std::string& name) {
  for (size_t i = 0; i < k.states.size(); ++i)
    if (k.states[i] == name) return static_cast<int32_t>(i);
  return -1;
}

}  // namespace

// The site of a role follows the call it names: in the recorded program order of the actor, the event
// right before it is the return of a call of the role's operation (a point after the call returns, e.g.
// a function-scoped after-call probe in the caller).  The role then binds that call's context.  A role
// whose purpose is not dependent_use takes the call's occurrences out of the readiness specifications
// (they are not instances: SPEC 1.6 "Call contexts"); the call itself is kept as a role fact.  Anything
// else is a finding and binds nothing (the occurrences stay instances).
void Replayer::on_role_point(const TraceEvent& ev, size_t ri) {
  const phi::RoleDecl& role = p_.roles[ri];
  const auto le = last_event_.find(ev.actor);
  const TraceEvent* prev = le == last_event_.end() || le->second >= t_.events.size() ? nullptr : &t_.events[le->second];
  if (prev == nullptr || prev->phase != Phase::Return || prev->site != role.op) {
    finding(ev.id, "role " + role.name + ": its site " + ev.site + " does not follow a return of " + role.op + " by the actor " + ev.actor + ": no call context is bound");
    return;
  }
  RoleFact rf;
  rf.role = role.name;
  rf.purpose = role.purpose;
  rf.actor = ev.actor;
  rf.op = role.op;
  rf.point = ev.id;
  rf.ret = prev->id;
  rf.has_result = prev->has_result;
  rf.result = prev->result;
  for (Occurrence& o : r_.occurrences) {
    if (o.actor != ev.actor || !o.returned || o.ret != prev->id) continue;
    o.context = role.name;
    rf.call = o.call;
    if (role.purpose == "dependent_use") continue;
    o.role_excluded = true;
    rf.excluded.push_back(o.rspec);
    // not an instance: its ordinal is not counted (it is the actor's latest occurrence of that key)
    auto ord = ordinals_.find(o.actor + "\n" + o.rspec + "\n" + key_text(o.key));
    if (ord != ordinals_.end() && ord->second == o.ordinal) --ord->second;
  }
  r_.role_facts.push_back(std::move(rf));
}

void Replayer::on_point(const TraceEvent& ev) {
  bool known = false;
  const auto role = role_of_site_.find(ev.site);
  if (role != role_of_site_.end()) on_role_point(ev, role->second);
  const auto tp = terminal_points_.find(ev.site);
  if (tp != terminal_points_.end()) {
    known = true;
    terminate(ev);
  }
  if (site_names_.count(ev.site) != 0) known = true;
  const auto it = points_.find(ev.site);
  if (it == points_.end()) {
    if (!known) finding(ev.id, "point '" + ev.site + "' is neither a linearization point, nor a site, nor a terminal step of the profile");
    return;
  }
  // The conjuncts read the state before the effects of the same point.
  for (const PointRef& ref : it->second) {
    if (!ref.conjunct) continue;
    const OpDecl& op = p_.ops[ref.op];
    const phi::ConjunctDecl& c = op.conjuncts[ref.index];
    if (c.lp.kind == LpKind::Capture && real_) {
      ConjunctResult res;
      res.label = c.label;
      res.source = LpSource::Capture;
      res.event = ev.id;
      // cross-check against the replayed cell, where the record fixes it
      const auto cell = ev.texts.find("cell");
      const auto value = ev.values.find("value");
      if (cell != ev.texts.end() && value != ev.values.end()) {
        const int32_t li = p_.find_loc(cell->second);
        if (li < 0) {
          finding(ev.id, "capture " + ev.site + ": '" + cell->second + "' is not a location of the profile");
        } else if (loc_reasons_[static_cast<size_t>(li)] == 0) {
          const auto lv = st_.loc_values.find(li);
          const int64_t replayed = lv == st_.loc_values.end() ? 0 : lv->second;
          if (replayed != value->second) {
            contradict(-1, li, ev.id, "capture " + ev.site + ": the value loaded from " + cell->second + " is " + hex_or_dec(value->second) + ", the replay of the recorded effects has " + hex_or_dec(replayed));
            res.contradicted = true;
          }
        }
      }
      const RealEval r = evaluate_real(*c.expr, op, &c.lp, ev, nullptr);
      res.reasons = r.reasons;
      res.adequacy = r.adequacy;
      res.contradicted = res.contradicted || r.contradicted;
      if (r.v.tri == Tri::Maybe) {
        res.unknown = true;
      } else {
        res.evaluated = true;
        res.holds = r.v.tri == Tri::True;
      }
      captures_[std::make_tuple(ev.actor, ref.op, ref.index)] = res;
    } else if (c.lp.kind == LpKind::Capture) {
      // cross-check against the replayed cell
      const auto cell = ev.texts.find("cell");
      const auto value = ev.values.find("value");
      if (cell != ev.texts.end() && value != ev.values.end()) {
        const int32_t li = p_.find_loc(cell->second);
        if (li < 0) {
          finding(ev.id, "capture " + ev.site + ": '" + cell->second + "' is not a location of the profile");
        } else {
          const auto lv = st_.loc_values.find(li);
          const int64_t replayed = lv == st_.loc_values.end() ? 0 : lv->second;
          if (replayed != value->second)
            finding(ev.id, "capture " + ev.site + ": the value loaded from " + cell->second + " is " + hex_or_dec(value->second) + ", the replay of the effects has " + hex_or_dec(replayed));
        }
      }
      CVal v;
      if (!evaluate(*c.expr, op, &c.lp, ev, nullptr, &v)) continue;
      ConjunctResult res;
      res.label = c.label;
      res.source = LpSource::Capture;
      res.event = ev.id;
      res.evaluated = true;
      res.holds = v.is_true();
      captures_[std::make_tuple(ev.actor, ref.op, ref.index)] = res;
    } else {
      Frame* f = top_frame_of(ev.actor, ref.op);
      if (f == nullptr) {
        finding(ev.id, "point " + ev.site + " is outside of a call of " + op.name + " by actor " + ev.actor);
        continue;
      }
      eval_conjunct(f, ref.index, LpSource::Point, ev, &f->args);
      progress(f);
    }
  }
  bool applied_any = false;
  for (const PointRef& ref : it->second) {
    if (ref.conjunct) continue;
    const OpDecl& op = p_.ops[ref.op];
    Frame* f = top_frame_of(ev.actor, ref.op);
    if (real_) apply_effect_real(op, op.effects[ref.index], ev, f != nullptr ? &f->args : nullptr);
    else apply_effect(op, op.effects[ref.index], ev, f != nullptr ? &f->args : nullptr);
    applied_any = true;
  }
  if (applied_any) {
    update_watches(ev);
    if (real_) update_intervals(ev.id);
  }
}

void Replayer::apply_effect(const OpDecl& op, const EffectDecl& eff, const TraceEvent& ev, const ArgMap* frame_args) {
  const std::string what = "effect " + op.name + "." + eff.label;
  if (!guards_hold(eff.guards, ev.id, what)) return;
  CVal v;
  if (eff.when) {
    if (!evaluate(*eff.when, op, &eff.lp, ev, frame_args, &v)) return;
    if (!v.is_true()) {
      // A filter selects the branch at a point that several branches share: nothing to report.
      if (eff.filter) return;
      // An asserted guard: the code reaches this point only where the guard holds, and the trace
      // says the point executed.  The replayed state is not the state of the run, or the trusted
      // guard is refuted.  The effect is not applied (its target state would be invented).
      GuardFailure g;
      g.event = ev.id;
      g.op = op.name;
      g.effect = eff.label;
      g.trusted = eff.status == "declared_trusted";
      if (g.trusted) g.refutable_by = eff.refutable_by;
      std::sort(g.refutable_by.begin(), g.refutable_by.end());
      std::string obligations;
      for (const std::string& o : g.refutable_by) obligations += (obligations.empty() ? "" : ", ") + o;
      if (g.trusted)
        finding(ev.id, what + ": its declared-trusted guard is false at its linearization point, which the trace shows executed: the guard is refuted by this observation (refutable_by " +
                           obligations + "); the effect is not applied");
      else
        finding(ev.id, what + ": its guard is false at its linearization point, which the trace shows executed and the code reaches only where the guard holds: the trace and the profile disagree; the effect is not applied");
      r_.guard_failures.push_back(std::move(g));
      return;
    }
  }
  auto check_value = [&](int64_t stored) {
    const auto it = ev.values.find("value");
    if (it != ev.values.end() && it->second != stored)
      finding(ev.id, what + ": the trace says the stored value is " + hex_or_dec(it->second) + ", the profile computes " + hex_or_dec(stored));
  };
  auto note_shutdown = [&](int32_t kind, int32_t to) {
    if (p_.scope.present && kind == p_.scope.kind_index && request_ordinal_ >= 0 && to >= request_ordinal_ && !r_.shutdown_requested) {
      r_.shutdown_requested = true;
      r_.shutdown_event = ev.id;
    }
  };
  switch (eff.kind) {
    case EffectKind::Set: {
      if (!eff.all_kind.empty()) {
        const int32_t ki = p_.find_kind(eff.all_kind);
        if (ki < 0) return;
        const int32_t to = state_ordinal(p_.kinds[static_cast<size_t>(ki)], eff.to_state);
        const auto inst = st_.instances.find(ki);
        if (to < 0 || inst == st_.instances.end()) return;
        for (const std::vector<int64_t>& key : inst->second) {
          const ResId id{ki, key};
          st_.state[id] = to;
          note_write(id, ev);
        }
        note_shutdown(ki, to);
        return;
      }
      CVal r;
      if (!evaluate(*eff.res, op, &eff.lp, ev, frame_args, &r)) return;
      if (!r.defined) {
        finding(ev.id, what + " applies to an undefined resource and is not applied");
        return;
      }
      const int32_t to = state_ordinal(p_.kinds[static_cast<size_t>(r.kind)], eff.to_state);
      if (to < 0) return;
      const ResId id{r.kind, r.key};
      st_.state[id] = to;
      note_write(id, ev);
      if (eff.with_id) {
        CVal idv;
        if (evaluate(*eff.with_id, op, &eff.lp, ev, frame_args, &idv)) st_.ids[id] = idv.v;
      }
      note_shutdown(r.kind, to);
      return;
    }
    case EffectKind::Raise: {
      CVal r, level;
      if (!evaluate(*eff.res, op, &eff.lp, ev, frame_args, &r) || !evaluate(*eff.value, op, &eff.lp, ev, frame_args, &level)) return;
      if (!r.defined) {
        finding(ev.id, what + " applies to an undefined resource and is not applied");
        return;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(r.kind)];
      if (level.v < 0 || level.v >= static_cast<int64_t>(k.states.size())) {
        finding(ev.id, what + ": the level " + hex_or_dec(level.v) + " is not a state of kind " + k.name);
        return;
      }
      const ResId id{r.kind, r.key};
      const auto cur = st_.state.find(id);
      const int32_t now = cur == st_.state.end() ? k.initial_ordinal : cur->second;
      if (level.v > now) {
        st_.state[id] = static_cast<int32_t>(level.v);
        note_write(id, ev);
      }
      return;
    }
    case EffectKind::SetTag: {
      CVal r;
      if (!evaluate(*eff.res, op, &eff.lp, ev, frame_args, &r)) return;
      if (!r.defined) {
        finding(ev.id, what + " applies to an undefined resource and is not applied");
        return;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(r.kind)];
      for (size_t ti = 0; ti < k.tags.size(); ++ti) {
        if (k.tags[ti].name != eff.target) continue;
        for (size_t vi = 0; vi < k.tags[ti].values.size(); ++vi)
          if (k.tags[ti].values[vi] == eff.tag_value) st_.tags[std::make_pair(ResId{r.kind, r.key}, static_cast<int32_t>(ti))] = static_cast<int32_t>(vi);
      }
      return;
    }
    case EffectKind::StoreOut: {
      if (!evaluate(*eff.value, op, &eff.lp, ev, frame_args, &v)) return;
      check_value(v.v);
      const auto where = ev.texts.find("out." + eff.target);
      if (where == ev.texts.end()) return;  // the pointer designates no location of the profile
      const int32_t li = p_.find_loc(where->second);
      if (li < 0) {
        finding(ev.id, what + ": '" + where->second + "' is not a location of the profile");
        return;
      }
      st_.loc_values[li] = v.v;
      return;
    }
    case EffectKind::StoreLoc: {
      if (!evaluate(*eff.value, op, &eff.lp, ev, frame_args, &v)) return;
      check_value(v.v);
      const int32_t li = p_.find_loc(eff.target);
      if (li >= 0) st_.loc_values[li] = v.v;
      return;
    }
    case EffectKind::Mark:
      if (eff.target == "handler_begin") began_handler_ = true;
      return;
    case EffectKind::Use:
    case EffectKind::Sync:
    case EffectKind::Env: return;
  }
}

// ---- real records: effects, returns, decisions ------------------------------------------------

// An effect of a real record (ADR-055).  The resource, the value and a `filter` are evaluated over the
// explanations of the record: where they are not fixed, the effect may have hit another resource or
// stored another value, so the target kind or location is no longer fixed by recorded writes.  An
// asserted guard (`when`) that is unknown holds in every explanation the record allows (the trace shows
// the point executed); one that is false contradicts the replayed state.
void Replayer::apply_effect_real(const OpDecl& op, const EffectDecl& eff, const TraceEvent& ev, const ArgMap* frame_args) {
  const std::string what = "effect " + op.name + "." + eff.label;
  if (!guards_hold(eff.guards, ev.id, what)) return;
  int32_t tkind = -1, tloc = -1;
  switch (eff.kind) {
    case EffectKind::Set: tkind = !eff.all_kind.empty() ? p_.find_kind(eff.all_kind) : (eff.res ? eff.res->sort.kind_index : -1); break;
    case EffectKind::Raise:
    case EffectKind::SetTag: tkind = eff.res ? eff.res->sort.kind_index : -1; break;
    case EffectKind::StoreLoc: tloc = p_.find_loc(eff.target); break;
    case EffectKind::StoreOut: {
      const auto where = ev.texts.find("out." + eff.target);
      if (where != ev.texts.end()) tloc = p_.find_loc(where->second);
      break;
    }
    default: break;
  }
  if (eff.when) {
    const RealEval g = evaluate_real(*eff.when, op, &eff.lp, ev, frame_args);
    if (g.v.tri == Tri::False) {
      if (eff.filter) return;
      GuardFailure gf;
      gf.event = ev.id;
      gf.op = op.name;
      gf.effect = eff.label;
      gf.trusted = eff.status == "declared_trusted";
      if (gf.trusted) gf.refutable_by = eff.refutable_by;
      std::sort(gf.refutable_by.begin(), gf.refutable_by.end());
      std::string obligations;
      for (const std::string& o : gf.refutable_by) obligations += (obligations.empty() ? "" : ", ") + o;
      if (gf.trusted)
        finding(ev.id, what + ": its declared-trusted guard is false at its linearization point in every explanation of the real record, which shows the point executed: the guard is refuted by this observation (refutable_by " +
                           obligations + "); the effect is not applied");
      else
        finding(ev.id, what + ": its guard is false at its linearization point in every explanation of the real record, which shows the point executed: the trace and the profile disagree; the effect is not applied");
      r_.guard_failures.push_back(std::move(gf));
      contradict(tkind, tloc, ev.id, what + " executed while its asserted guard is false");
      return;
    }
    if (g.v.tri == Tri::Maybe && eff.filter) {
      // a branch that may or may not have been taken
      note_publication(op, eff, ev, frame_args, true);
      untrack_kind(tkind, g.reasons | KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS);
      untrack_loc(tloc, g.reasons | KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS);
      if (eff.kind == EffectKind::StoreOut && tloc < 0)
        for (size_t li = 0; li < loc_reasons_.size(); ++li) untrack_loc(static_cast<int32_t>(li), g.reasons | KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS);
      return;
    }
  }
  note_publication(op, eff, ev, frame_args, false);
  auto note_shutdown = [&](int32_t kind, int32_t to) {
    if (p_.scope.present && kind == p_.scope.kind_index && request_ordinal_ >= 0 && to >= request_ordinal_ && !r_.shutdown_requested) {
      r_.shutdown_requested = true;
      r_.shutdown_event = ev.id;
    }
  };
  // The stored value: the one the record carries (`value`), else the profile's where it is fixed.
  auto stored = [&](int32_t li) {
    const auto rec = ev.values.find("value");
    const RealEval v = evaluate_real(*eff.value, op, &eff.lp, ev, frame_args);
    int64_t computed = 0;
    const bool fixed = exact_value(v.v, &computed);
    if (rec != ev.values.end()) {
      if (fixed && computed != rec->second) contradict(-1, li, ev.id, what + ": the record says the stored value is " + hex_or_dec(rec->second) + ", the profile computes " + hex_or_dec(computed));
      st_.loc_values[li] = rec->second;
      loc_writers_[li].insert(ev.actor);
      return;
    }
    if (fixed) {
      st_.loc_values[li] = computed;
      loc_writers_[li].insert(ev.actor);
    } else {
      untrack_loc(li, v.reasons);
    }
  };
  switch (eff.kind) {
    case EffectKind::Set: {
      if (!eff.all_kind.empty()) {
        const int32_t ki = p_.find_kind(eff.all_kind);
        if (ki < 0) return;
        const int32_t to = state_ordinal(p_.kinds[static_cast<size_t>(ki)], eff.to_state);
        const auto inst = st_.instances.find(ki);
        if (to < 0 || inst == st_.instances.end()) return;
        for (const std::vector<int64_t>& key : inst->second) {
          const ResId id{ki, key};
          st_.state[id] = to;
          note_write(id, ev);
          res_writers_[id].insert(ev.actor);
        }
        note_shutdown(ki, to);
        return;
      }
      const RealEval r = evaluate_real(*eff.res, op, &eff.lp, ev, frame_args);
      if (r.v.must_be_undefined) {
        finding(ev.id, what + " applies to an undefined resource and is not applied");
        return;
      }
      std::vector<int64_t> key;
      if (!exact_key(r.v, &key)) {
        untrack_kind(tkind, r.reasons);
        return;
      }
      const int32_t to = state_ordinal(p_.kinds[static_cast<size_t>(r.v.kind)], eff.to_state);
      if (to < 0) return;
      const ResId id{r.v.kind, key};
      st_.state[id] = to;
      note_write(id, ev);
          res_writers_[id].insert(ev.actor);
      if (eff.with_id) {
        const RealEval idv = evaluate_real(*eff.with_id, op, &eff.lp, ev, frame_args);
        int64_t v = 0;
        if (exact_value(idv.v, &v)) st_.ids[id] = id_value(v);  // (the writer is noted with the state above)
        else untrack_kind(tkind, idv.reasons);
      }
      note_shutdown(r.v.kind, to);
      return;
    }
    case EffectKind::Raise: {
      const RealEval r = evaluate_real(*eff.res, op, &eff.lp, ev, frame_args);
      const RealEval level = evaluate_real(*eff.value, op, &eff.lp, ev, frame_args);
      if (r.v.must_be_undefined) {
        finding(ev.id, what + " applies to an undefined resource and is not applied");
        return;
      }
      std::vector<int64_t> key;
      int64_t lv = 0;
      if (!exact_key(r.v, &key) || !exact_value(level.v, &lv)) {
        untrack_kind(tkind, r.reasons | level.reasons);
        return;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(r.v.kind)];
      if (lv < 0 || lv >= static_cast<int64_t>(k.states.size())) {
        finding(ev.id, what + ": the level " + hex_or_dec(lv) + " is not a state of kind " + k.name);
        return;
      }
      const ResId id{r.v.kind, key};
      const auto cur = st_.state.find(id);
      const int32_t now = cur == st_.state.end() ? k.initial_ordinal : cur->second;
      if (lv > now) {
        st_.state[id] = static_cast<int32_t>(lv);
        note_write(id, ev);
          res_writers_[id].insert(ev.actor);
      }
      return;
    }
    case EffectKind::SetTag: {
      const RealEval r = evaluate_real(*eff.res, op, &eff.lp, ev, frame_args);
      if (r.v.must_be_undefined) {
        finding(ev.id, what + " applies to an undefined resource and is not applied");
        return;
      }
      std::vector<int64_t> key;
      if (!exact_key(r.v, &key)) {
        untrack_kind(tkind, r.reasons);
        return;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(r.v.kind)];
      for (size_t ti = 0; ti < k.tags.size(); ++ti) {
        if (k.tags[ti].name != eff.target) continue;
        for (size_t vi = 0; vi < k.tags[ti].values.size(); ++vi)
          if (k.tags[ti].values[vi] == eff.tag_value) st_.tags[std::make_pair(ResId{r.v.kind, key}, static_cast<int32_t>(ti))] = static_cast<int32_t>(vi);
      }
      res_writers_[ResId{r.v.kind, key}].insert(ev.actor);
      return;
    }
    case EffectKind::StoreOut: {
      const auto where = ev.texts.find("out." + eff.target);
      if (where == ev.texts.end()) {
        // the record does not say where the pointer pointed: any location may have been written
        for (size_t li = 0; li < loc_reasons_.size(); ++li) untrack_loc(static_cast<int32_t>(li), KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS);
        return;
      }
      if (tloc < 0) {
        finding(ev.id, what + ": '" + where->second + "' is not a location of the profile");
        return;
      }
      stored(tloc);
      return;
    }
    case EffectKind::StoreLoc:
      if (tloc >= 0) stored(tloc);
      return;
    case EffectKind::Mark:
      if (eff.target == "handler_begin") began_handler_ = true;
      return;
    case EffectKind::Use:
    case EffectKind::Sync:
    case EffectKind::Env: return;
  }
}

// The return of a call in a real record.
void Replayer::return_real(const OpDecl& op, Frame* f, const TraceEvent& ev) {
  // 1. a conjunct read over the call: the join of the values in every state between the call and the
  //    return; whether its point was passed is not observed
  for (size_t ci = 0; ci < f->intervals.size(); ++ci) {
    const IntervalRead& in = f->intervals[ci];
    ConjunctResult& c = f->conj[ci];
    if (!in.active || c.evaluated || c.unknown) continue;
    c.source = LpSource::Interval;
    c.event = ev.id;
    c.passage_unknown = true;
    c.reasons = in.reasons | KZ_WITHHOLD_EFFECT_POINT_AMBIGUOUS;
    c.adequacy = in.adequacy;
    c.contradicted = in.contradicted;
    if (!in.any || in.tri == Tri::Maybe) {
      c.unknown = true;
    } else {
      c.evaluated = true;
      c.holds = in.tri == Tri::True;
    }
  }
  // 2. an observed point the call did not pass, while events may be missing: its passage is unknown
  if (overlaps_loss(f->call, ev.id)) {
    for (size_t ci = 0; ci < f->conj.size(); ++ci) {
      ConjunctResult& c = f->conj[ci];
      if (c.evaluated || c.unknown || c.passage_unknown || op.conjuncts[ci].lp.kind == LpKind::Capture) continue;
      c.passage_unknown = true;
      c.unknown = true;
      c.reasons |= KZ_WITHHOLD_LOSS;
    }
  }
  for (size_t k = 0; k < f->occs.size(); ++k) {
    Occurrence& o = r_.occurrences[f->occs[k]];
    o.returned = true;
    o.ret = ev.id;
    o.has_result = ev.has_result;
    o.result = ev.result;
  }
  // 3. the returned code, under O4 (the outcome list of the operation)
  check_outcome_real(op, f, ev);
  // 4. decide what is still open
  progress(f);
  for (size_t k = 0; k < f->occs.size(); ++k) decide_real(f, k, ev);
}

// O4(a) for a real record.  The returned code is observed, and the outcome list says what it means for
// the conjuncts: `not_ready(L)` means L was reached and false, `ready` that no reached conjunct is false.
// So the code fixes a conjunct the record left open (the explanations that disagree with the code are
// not explanations of the record), and a code that disagrees with a conjunct fixed in every explanation
// leaves no explanation at all: an analysis input error, never a violation and never no violation.
void Replayer::check_outcome_real(const OpDecl& op, Frame* f, const TraceEvent& ev) {
  OutcomeCheck chk = OutcomeCheck::NotChecked;
  bool narrowed = false, contra = false;
  if (!op.outcomes.empty() && ev.has_result) {
    const phi::OutcomeDecl* hit = nullptr;
    for (const phi::OutcomeDecl& oc : op.outcomes) {
      if (oc.code_const < 0) continue;
      const phi::ConstDecl& c = p_.consts[static_cast<size_t>(oc.code_const)];
      if (c.value == phi::ConstDecl::Value::Int && u32(c.ival) == u32(ev.result)) {
        hit = &oc;
        break;
      }
    }
    if (hit == nullptr) {
      chk = OutcomeCheck::Unmapped;
      finding(ev.id, op.name + " returned " + hex_or_dec(static_cast<int64_t>(u32(ev.result))) + ", which its outcome list does not map");
    } else if (hit->cls == OutcomeClass::Ready) {
      // P1 (a pending PI decision): with o_.outcome_narrowing false the code fixes nothing it leaves open.
      for (ConjunctResult& c : f->conj) {
        if (c.evaluated && !c.holds) {
          if (c.passage_unknown) {  // false wherever it is passed: it was not passed
            if (!o_.outcome_narrowing) continue;
            c.evaluated = false;
            c.passage_unknown = false;
            c.source = LpSource::Outcome;
            c.event = ev.id;
            narrowed = true;
          } else {
            c.contradicted = true;
            contra = true;
          }
        } else if (c.unknown && o_.outcome_narrowing) {
          c.unknown = false;
          c.evaluated = true;
          c.holds = true;
          c.source = LpSource::Outcome;
          c.event = ev.id;
          narrowed = true;
        }
      }
      chk = contra ? OutcomeCheck::Disagrees : OutcomeCheck::Agrees;
      if (contra) finding(ev.id, op.name + " returned " + hit->code + " (ready), and a conjunct of the call is false in every explanation of the real record: analysis input error");
    } else if (hit->cls == OutcomeClass::NotReady) {
      if (hit->conjunct_index < 0 || static_cast<size_t>(hit->conjunct_index) >= f->conj.size()) {
        chk = OutcomeCheck::Disagrees;
      } else {
        ConjunctResult& c = f->conj[static_cast<size_t>(hit->conjunct_index)];
        const bool not_passed = !c.evaluated && !c.unknown && !c.passage_unknown;
        if ((c.evaluated && c.holds) || not_passed) {
          c.contradicted = true;
          contra = true;
          chk = OutcomeCheck::Disagrees;
          finding(ev.id, op.name + " returned " + hit->code + " (not ready: " + hit->conjunct + "), and in every explanation of the real record that conjunct is " +
                             (not_passed ? "not reached" : "true") + ": analysis input error");
        } else {
          const bool fixed_false = c.evaluated && !c.holds && !c.passage_unknown;
          if (!fixed_false && o_.outcome_narrowing) {
            c.source = LpSource::Outcome;
            c.event = ev.id;
            narrowed = true;
            c.evaluated = true;
            c.holds = false;
            c.unknown = false;
            c.passage_unknown = false;
          }
          chk = OutcomeCheck::Agrees;
        }
      }
    }
  }
  for (size_t idx : f->occs) {
    Occurrence& o = r_.occurrences[idx];
    o.outcome = chk;
    if (narrowed) o.outcome_narrowed = true;
    if (contra) o.input_error = true;
  }
  if (contra) r_.partial = true;
}

// The status of an occurrence of a real record that the conjuncts fixed in every explanation did not
// decide: the statuses the explanations allow (SPEC 4.8 "Explanation set").  One status: that status;
// more: UNDETERMINED with the list and the reasons.
void Replayer::decide_real(Frame* f, size_t k, const TraceEvent& ev) {
  sync_occurrence(*f, k);
  Occurrence& o = r_.occurrences[f->occs[k]];
  if (o.decided_at != DecidedAt::None) return;
  const RspecInfo& info = rspec_info_[rspecs_of_op_[f->op][k]];
  bool must_false = false, can_false = false, can_all_true = !info.conj.empty(), may_skip = false;
  uint32_t why = 0;
  uint64_t last = f->call;
  for (size_t ci : info.conj) {
    const ConjunctResult& c = f->conj[ci];
    const int passed = c.passage_unknown ? 2 : (c.evaluated || c.unknown) ? 1 : 0;  // no, yes, maybe
    const int value = c.evaluated ? (c.holds ? 1 : 0) : 2;                           // false, true, maybe
    if (passed == 1 && value == 0) must_false = true;
    if (passed != 0 && value != 1) can_false = true;
    if (!(passed != 0 && value != 0)) can_all_true = false;
    if (passed != 1) may_skip = true;
    if (c.unknown || c.passage_unknown) why |= c.reasons;
    if (c.evaluated && !c.passage_unknown) last = std::max(last, c.event);
  }
  std::vector<OccStatus> poss;
  if (info.conj.empty()) {
    poss.push_back(OccStatus::NotEvaluated);
  } else {
    if (can_false) poss.push_back(not_ready_status(info));
    if (can_all_true && !must_false) poss.push_back(OccStatus::Ready);
    if (!must_false && may_skip) poss.push_back(OccStatus::NotEvaluated);
  }
  o.reasons |= why;
  if (poss.size() == 1) {
    decide(&o, poss[0], DecidedAt::Return, poss[0] == OccStatus::NotEvaluated ? last : ev.id);
    return;
  }
  for (OccStatus st : poss) {
    if (st == OccStatus::Pending) o.reasons |= KZ_WITHHOLD_HORIZON_OPEN;
    if (st == OccStatus::NotReadyUnjudged) o.reasons |= KZ_WITHHOLD_CALLER_REQUIREMENT_ABSENT;
    if (st == OccStatus::NotEvaluated) o.reasons |= KZ_WITHHOLD_NOT_EVALUATED;
  }
  if (o.reasons == 0) o.reasons = KZ_WITHHOLD_EXPLANATION_AMBIGUOUS;
  o.possible = poss;
  decide(&o, OccStatus::Undetermined, DecidedAt::Return, ev.id);
}

// A registration or publication (SPEC 1.1 "Execution relations"): an effect on one resource (not a reset of
// every resource of a kind, which names none) that may have been applied, with its target read with the
// writer's own identity.  The target is established only where the expression and the writer's app_owner
// fix it (an owner that is not confirmed or inferred leaves a target that reads `owner(exec)` open).
void Replayer::note_publication(const OpDecl& op, const EffectDecl& eff, const TraceEvent& ev, const ArgMap* frame_args, bool maybe) {
  if (!eff.res || !eff.all_kind.empty()) return;
  if (eff.kind != EffectKind::Set && eff.kind != EffectKind::Raise && eff.kind != EffectKind::SetTag) return;
  Publication pb;
  pb.kind = eff.res->sort.kind_index;
  pb.actor = ev.actor;
  pb.maybe = maybe;
  const RealEval r = evaluate_real(*eff.res, op, &eff.lp, ev, frame_args, true);
  pb.exact = !r.v.must_be_undefined && exact_key(r.v, &pb.key);
  if (r.v.kind >= 0) pb.kind = r.v.kind;
  pubs_.push_back(std::move(pb));
}

// The execution relation of one occurrence (SPEC 1.1, 1.10 item 10, ADR-056).  The actors involved are the
// executing task of the use and the actors of the registrations and publications of the occurrence's key in
// the record.  Each pair: either owner not confirmed or inferred -> ownership undetermined; the same task or
// the same owning application -> intra-application; one created the other (the creating executor) ->
// creation framework - new application; otherwise core-application where the record shows that one owner is
// a core service, else inter-application (core-status-unknown when the record does not show it for both).
// Several pairs: undetermined if any is, intra-application only if every pair is; otherwise the first of
// inter-application, core-application, creation framework that occurs.  Nothing here reads the order of the
// events (the relation is between actors, not an order between their events).
void Replayer::relate(Occurrence* o) {
  o->relation = KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED;
  o->relation_level = KZ_OWNER_LEVEL_UNDETERMINED;
  o->counterparts.clear();
  auto level_of = [&](const std::string& a) -> uint8_t {
    const auto it = t_.owner_levels.find(a);
    if (it == t_.owner_levels.end() || it->second == KZ_OWNER_LEVEL_UNDETERMINED) return KZ_OWNER_LEVEL_UNDETERMINED;
    return t_.owners.count(a) != 0 ? it->second : static_cast<uint8_t>(KZ_OWNER_LEVEL_UNDETERMINED);
  };
  const phi::RspecDecl* rs = nullptr;
  for (const phi::RspecDecl& r : p_.rspecs)
    if (r.name == o->rspec) rs = &r;
  if (rs == nullptr || !rs->key) {
    o->relation_note = "the readiness specification has no key: no registration or publication to relate the use to";
    return;
  }
  if (o->key.empty()) {
    o->relation_note = "the record does not fix the key of the occurrence";
    return;
  }
  const int32_t kk = rs->key->sort.kind_index;
  const std::string kname = kk >= 0 ? p_.kinds[static_cast<size_t>(kk)].name : std::string("?");
  std::set<std::string> cps, open_writers;
  for (const Publication& pb : pubs_) {
    if (pb.kind != kk) continue;
    if (!pb.exact) open_writers.insert(pb.actor);
    else if (pb.key == o->key) (pb.maybe ? open_writers : cps).insert(pb.actor);
  }
  for (const std::string& a : cps) open_writers.erase(a);
  o->counterparts.assign(cps.begin(), cps.end());
  const uint8_t lu = level_of(o->actor);
  if (lu == KZ_OWNER_LEVEL_UNDETERMINED) {
    o->relation_note = "the owner of the executing task " + o->actor + " is undetermined";
    return;
  }
  if (!open_writers.empty()) {
    o->relation_note = "a write of " + kname + " by " + *open_writers.begin() + " whose target the record does not establish (its owner is undetermined or a value is not recorded)";
    return;
  }
  if (cps.empty()) {
    o->relation_note = "no registration or publication of " + kname + key_text(o->key) + " in the record";
    return;
  }
  uint8_t level = lu;
  bool any_inter = false, any_inter_known = false, any_core = false, any_creation = false, all_intra = true;
  std::string intra_note;
  for (const std::string& c : cps) {
    const uint8_t lc = level_of(c);
    if (lc == KZ_OWNER_LEVEL_UNDETERMINED) {
      o->relation_note = "the owner of " + c + ", which wrote " + kname + key_text(o->key) + ", is undetermined";
      return;
    }
    level = std::min(level, lc);
    if (c == o->actor) {
      intra_note = intra_note.empty() || intra_note == "same task" ? "same task" : "same application";
      continue;
    }
    if (t_.owners.at(c) == t_.owners.at(o->actor)) {
      intra_note = "same application";
      continue;
    }
    all_intra = false;
    const auto cu = t_.creators.find(o->actor), cc = t_.creators.find(c);
    if ((cu != t_.creators.end() && cu->second == c) || (cc != t_.creators.end() && cc->second == o->actor)) {
      any_creation = true;
      continue;
    }
    const auto ku = t_.owner_core.find(o->actor), kc = t_.owner_core.find(c);
    const bool u_core = ku != t_.owner_core.end() && ku->second, c_core = kc != t_.owner_core.end() && kc->second;
    if (u_core || c_core) {
      any_core = true;
    } else {
      any_inter = true;
      if (ku != t_.owner_core.end() && kc != t_.owner_core.end()) any_inter_known = true;
    }
  }
  std::string who;
  for (const std::string& c : cps) who += (who.empty() ? "" : ",") + c;
  o->relation_level = level;
  if (all_intra) {
    o->relation = KZ_EXEC_RELATION_INTRA_APPLICATION;
    o->relation_note = intra_note + " (" + who + ")";
  } else if (any_inter) {
    o->relation = KZ_EXEC_RELATION_INTER_APPLICATION;
    o->relation_note = std::string(any_inter_known ? "different applications, neither a core service" : "different applications; core status not established") + " (" + who + ")";
    if (!any_inter_known && !any_core) o->summary.flags |= KZ_SUMMARY_FLAG_CORE_STATUS_UNKNOWN;
  } else if (any_core) {
    o->relation = KZ_EXEC_RELATION_CORE_APPLICATION;
    o->relation_note = "different applications, a core service among them (" + who + ")";
  } else if (any_creation) {
    o->relation = KZ_EXEC_RELATION_CREATION_FRAMEWORK;
    o->relation_note = "one created the other (" + who + ")";
  }
}

// ---- horizon monitors (SPEC 1.6 "Horizon monitors") ---------------------------------------------
//
// retry_until_ready(codes, bound): the monitored actor is the using actor; ACCEPT when a later attempt of
// the same loop instance is READY (a NOT_EVALUATED attempt is not); REJECT when the loop takes its
// exhaustion path, or an attempt's outcome is outside `codes`.  On a trace a loop instance is read as a
// maximal run of consecutive attempts (occurrences of the readiness specification) of one actor, in one
// call context, on one key and one generation of the key resource.  The run ends at a READY attempt, at an
// attempt whose outcome is not in `codes`, or at the bound-th attempt whose outcome is in `codes` (the
// exhaustion path: the loop makes at most `bound` attempts).  The loop's own entry is not an event of the
// trace, so two loops of one actor on one key and generation with nothing between them read as one run.
// The bound is the value of the entry's constant: an integer of the profile (with its provenance) or the
// value the trace gives for a `ref eval` constant; the tool has no number of its own.
// A generation boundary is an effect that returns a resource of the key's kind to the kind's initial
// state.  Where the profile has none, all attempts are of one generation (gen(R) = 0 in this subset);
// where one may lie between two attempts (its point is between them, or a real record does not observe
// its point), the two are not counted as one run.
// What cannot be counted is withheld, never decided: an attempt whose key is not fixed, a run that a
// reported loss overlaps (attempts may be missing), a possible generation boundary, a later attempt whose
// own status is undetermined with READY among its possibilities.  The not-ready attempts of such a run are
// UNDETERMINED (possible PENDING, TOLERATED and, for the only deciding entry, VIOLATING), reason
// explanation_ambiguous.  A run the trace ends before ACCEPT or REJECT stays PENDING: OPEN never counts as
// ACCEPT, and its summary is INCONCLUSIVE(horizon-open).  ACCEPT makes TOLERATED.  REJECT makes VIOLATING
// only where the retry entry is the only deciding entry (no other entry's monitor is run, and another
// entry might accept); otherwise the occurrence stays PENDING with the REJECT stated.
void Replayer::run_monitors() {
  for (size_t si = 0; si < rspec_info_.size(); ++si) {
    const RspecInfo& info = rspec_info_[si];
    if (info.allowed.kind != phi::CallerAllowedKind::Entries) continue;
    const phi::AllowedEntry* retry = nullptr;
    for (const phi::AllowedEntry* e : info.allowed.deciding)
      if (e->kind == phi::AllowedKind::RetryUntilReady) {
        retry = e;
        break;
      }
    if (retry == nullptr) continue;
    const phi::RspecDecl& rs = p_.rspecs[si];
    const bool sole = info.allowed.deciding.size() == 1;

    // the codes and the bound, from the profile's constants
    std::set<uint32_t> codes;
    std::string problem;
    for (const std::string& c : retry->codes) {
      const int32_t ci = p_.find_const(c);
      if (ci < 0 || p_.consts[static_cast<size_t>(ci)].value != phi::ConstDecl::Value::Int) problem = "the code " + c + " is not an integer constant of the profile";
      else codes.insert(u32(p_.consts[static_cast<size_t>(ci)].ival));
    }
    int64_t bound = -1;
    const int32_t bi = p_.find_const(retry->bound);
    if (bi < 0) {
      problem = "the bound " + retry->bound + " is not a constant of the profile";
    } else {
      const phi::ConstDecl& c = p_.consts[static_cast<size_t>(bi)];
      if (c.value == phi::ConstDecl::Value::Int) {
        bound = c.ival;
      } else if (c.value == phi::ConstDecl::Value::RefEval) {
        const auto it = t_.ref_consts.find(c.name);
        if (it != t_.ref_consts.end()) bound = it->second;
        else problem = "the bound " + c.name + " is a ref eval constant the trace does not give (const " + c.name + " N)";
      } else {
        problem = "the bound " + c.name + " is not an integer constant";
      }
      if (problem.empty() && bound <= 0) problem = "the bound " + c.name + " is " + std::to_string(bound) + ", not a positive number of attempts";
    }
    if (!problem.empty()) {
      finding(kNoEvent, "rspec " + rs.name + ": the retry_until_ready monitor is not run (" + problem + "); its not-ready attempts stay PENDING");
      continue;
    }
    const std::string bound_text = "bound " + retry->bound + " = " + std::to_string(bound);

    // generation boundaries of the key's kind: effects that return a resource of it to the initial state
    const int32_t key_kind = rs.key ? rs.key->sort.kind_index : -1;
    std::set<std::string> reset_points;
    bool reset_unobserved = false;
    if (key_kind >= 0) {
      const KindDecl& kd = p_.kinds[static_cast<size_t>(key_kind)];
      for (const OpDecl& op : p_.ops)
        for (const EffectDecl& e : op.effects) {
          if (e.kind != EffectKind::Set || e.to_state != kd.initial) continue;
          const bool hits = !e.all_kind.empty() ? p_.find_kind(e.all_kind) == key_kind : (e.res && e.res->sort.kind_index == key_kind);
          if (!hits) continue;
          if (e.lp.kind == LpKind::None) {
            reset_unobserved = true;
            continue;
          }
          const std::string name = lp_name(op, e.label, e.lp);
          reset_points.insert(name);
          if (real_ && t_.observed_points.count(name) == 0) reset_unobserved = true;
        }
    }
    auto boundary_between = [&](uint64_t a, uint64_t b) {
      if (reset_unobserved) return true;
      for (uint64_t e = a + 1; e < b && e < t_.events.size(); ++e)
        if (t_.events[e].phase == Phase::Point && reset_points.count(t_.events[e].site) != 0) return true;
      return false;
    };

    auto withhold = [&](Occurrence& o, const std::string& why, bool loss) {
      o.possible = {OccStatus::Pending, OccStatus::Tolerated};
      if (sole) o.possible.push_back(OccStatus::Violating);
      o.status = OccStatus::Undetermined;
      o.reasons |= KZ_WITHHOLD_EXPLANATION_AMBIGUOUS;
      if (loss) o.reasons |= KZ_WITHHOLD_LOSS;
      o.monitor = "retry_until_ready not countable: " + why + " (" + bound_text + ")";
    };

    // the attempts per actor and call context, in the order of their calls
    std::map<std::pair<std::string, std::string>, std::vector<size_t>> runs;
    for (size_t i = 0; i < r_.occurrences.size(); ++i)
      if (r_.occurrences[i].rspec == rs.name) runs[std::make_pair(r_.occurrences[i].actor, r_.occurrences[i].context)].push_back(i);
    for (const auto& run : runs) {
      const std::vector<size_t>& seq = run.second;
      bool unknown_key = false;
      for (size_t i : seq) unknown_key = unknown_key || !r_.occurrences[i].key_known;
      if (unknown_key) {
        for (size_t i : seq)
          if (r_.occurrences[i].status == OccStatus::Pending)
            withhold(r_.occurrences[i], "the key of an attempt of actor " + run.first.first + " is not fixed, so the attempts of a loop instance cannot be counted", false);
        continue;
      }
      std::map<std::vector<int64_t>, std::vector<size_t>> by_key;
      for (size_t i : seq) by_key[r_.occurrences[i].key].push_back(i);
      for (const auto& kk : by_key) {
        const std::vector<size_t>& att = kk.second;
        size_t k = 0;
        while (k < att.size()) {
          enum class End { Open, Accept, Exhausted, Outside, Ambiguous } end = End::Open;
          std::vector<size_t> inst;
          std::string why;
          bool loss = false;
          uint64_t at = 0;  // the event of the ACCEPT or REJECT
          while (k < att.size()) {
            Occurrence& a = r_.occurrences[att[k]];
            if (!inst.empty() && boundary_between(r_.occurrences[inst.back()].call, a.call)) {
              end = End::Ambiguous;
              why = "a generation boundary of the key resource may lie between the attempts #" + std::to_string(r_.occurrences[inst.back()].call) + " and #" + std::to_string(a.call);
              break;  // `a` begins the next run
            }
            inst.push_back(att[k]);
            ++k;
            const bool in_codes = a.has_result && codes.count(u32(a.result)) != 0;
            if (a.status == OccStatus::Ready) {
              end = End::Accept;
              at = a.decision;
              break;
            }
            if (a.status == OccStatus::Open || !a.returned) break;  // the run is open at the end of the trace
            if (a.status == OccStatus::Undetermined &&
                std::find(a.possible.begin(), a.possible.end(), OccStatus::Ready) != a.possible.end()) {
              end = End::Ambiguous;
              why = "the status of the attempt at #" + std::to_string(a.call) + " is undetermined, READY among its possibilities";
              break;
            }
            if (!in_codes) {
              end = End::Outside;
              at = a.ret;
              break;
            }
            if (inst.size() == static_cast<size_t>(bound)) {
              end = End::Exhausted;
              at = a.ret;
              break;
            }
          }
          if (inst.empty()) {  // a boundary before the first attempt cannot happen; keep the loop finite
            ++k;
            continue;
          }
          if (real_ && end != End::Ambiguous) {
            const uint64_t from = r_.occurrences[inst.front()].call;
            const uint64_t to = end == End::Open ? (t_.events.empty() ? from : t_.events.size() - 1) : r_.occurrences[inst.back()].ret;
            if (overlaps_loss(from, to)) {
              end = End::Ambiguous;
              loss = true;
              why = "a reported loss overlaps the attempts from #" + std::to_string(from) + " to #" + std::to_string(to) + ", so attempts may be missing";
            }
          }
          const std::string n_text = std::to_string(inst.size());
          for (size_t j = 0; j < inst.size(); ++j) {
            Occurrence& o = r_.occurrences[inst[j]];
            if (o.status != OccStatus::Pending) continue;
            const std::string self = "attempt " + std::to_string(j + 1) + " of " + n_text + " of the run from #" + std::to_string(r_.occurrences[inst.front()].call);
            switch (end) {
              case End::Accept:
                o.status = OccStatus::Tolerated;
                o.accepted = true;
                o.monitor = "retry_until_ready ACCEPT at #" + std::to_string(at) + ": attempt " + n_text + " is READY (" + self + "; " + bound_text + ")";
                break;
              case End::Exhausted:
              case End::Outside: {
                const std::string what = end == End::Exhausted
                                             ? "REJECT at #" + std::to_string(at) + ": the bound is exhausted, attempt " + n_text + " not ready"
                                             : "REJECT at #" + std::to_string(at) + ": the outcome " + hex_or_dec(static_cast<int64_t>(u32(r_.occurrences[inst.back()].result))) +
                                                   " of attempt " + n_text + " is outside the codes";
                if (sole) o.status = OccStatus::Violating;
                o.monitor = "retry_until_ready " + what + " (" + self + "; " + bound_text + ")" + (sole ? "" : "; another deciding entry is not monitored: stays PENDING");
                break;
              }
              case End::Open:
                o.monitor = "retry_until_ready OPEN: the trace ends after attempt " + n_text + " (" + self + "; " + bound_text + ")";
                break;
              case End::Ambiguous:
                withhold(o, why + " (" + self + ")", loss);
                break;
            }
          }
        }
      }
    }
  }
}

// The summary view of every occurrence (SPEC 1.10, D2), computed last; for a real record also the
// adequacy of each occurrence and what the replay established per kind and location.
void Replayer::finish() {
  // Occurrences a role took out are not instances (on_role_point): drop them before anything is counted.
  r_.occurrences.erase(std::remove_if(r_.occurrences.begin(), r_.occurrences.end(), [](const Occurrence& o) { return o.role_excluded; }), r_.occurrences.end());
  run_monitors();
  if (real_) {
    for (Occurrence& o : r_.occurrences) {
      uint32_t adeq = kAllAdequacy;
      bool any = false;
      for (const ConjunctResult& c : o.conjuncts) {
        if (c.contradicted) o.input_error = true;
        if (c.evaluated || c.unknown) {
          adeq &= c.adequacy;
          any = true;
        }
        if (o.status == OccStatus::Open && (c.unknown || c.passage_unknown)) o.reasons |= c.reasons;
      }
      o.adequacy = any ? adeq : 0;
      if (o.input_error) r_.partial = true;
    }
  }
  for (Occurrence& o : r_.occurrences) {
    SummaryInput in;
    switch (o.status) {
      case OccStatus::Ready: in.status = KZ_OCC_READY; break;
      case OccStatus::NotEvaluated: in.status = KZ_OCC_NOT_EVALUATED; break;
      case OccStatus::Violating: in.status = KZ_OCC_VIOLATING; break;
      case OccStatus::NotReadyUnjudged: in.status = KZ_OCC_NOT_READY_UNJUDGED; break;
      case OccStatus::Tolerated: in.status = KZ_OCC_TOLERATED; break;
      case OccStatus::Pending: in.status = KZ_OCC_PENDING; break;
      case OccStatus::Open: in.status_flags = KZ_STATUS_FLAG_OPEN; break;
      case OccStatus::Undetermined: in.status_flags = KZ_STATUS_FLAG_UNDETERMINED; break;
    }
    // A model trace is one explanation; in a real record a status is decided only where every
    // explanation agrees (the abstract evaluation, under the obligation L-OBS).
    in.all_explanations = true;
    in.caller_record = o.status == OccStatus::Tolerated;  // only an entry with a caller record decides (derive_caller_allowed)
    in.accepted = o.accepted;
    in.input_error = o.input_error;
    in.reasons = o.reasons;
    o.summary = summarize_observation(in);
    if (real_) {
      o.summary.flags |= KZ_SUMMARY_FLAG_REAL_RECORD;
      if (o.outcome_narrowed) o.summary.flags |= KZ_SUMMARY_FLAG_OUTCOME_NARROWED;
      if (r_.partial) o.summary.flags |= KZ_SUMMARY_FLAG_PARTIAL;
      relate(&o);
      o.summary.relation = o.relation;
      if (o.relation != KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED && o.relation_level == KZ_OWNER_LEVEL_INFERRED) o.summary.flags |= KZ_SUMMARY_FLAG_RELATION_INFERRED;
    }
  }
  if (!real_) return;
  for (size_t ki = 0; ki < p_.kinds.size(); ++ki) {
    if (!p_.kinds[ki].derived.empty()) continue;
    r_.knowledge.push_back(Knowledge{p_.kinds[ki].name, false, kind_reasons_[ki], kind_adeq_[ki], kind_contra_[ki]});
  }
  for (size_t li = 0; li < p_.locs.size(); ++li) r_.knowledge.push_back(Knowledge{p_.locs[li].name, true, loc_reasons_[li], loc_adeq_[li], loc_contra_[li]});
}

// ---- edges that pair events by a value --------------------------------------------------------

void Replayer::final_edges() {
  std::vector<SpawnRec> spawns;
  for (size_t ri = 0; ri < p_.rules.size(); ++ri) {
    const HbRuleDecl& rule = p_.rules[ri];
    if (rule.tmpl != RuleTemplate::Spawn || !rule_applies(ri)) continue;
    std::vector<bool> used(entries_.size(), false);
    for (const CreateRec& c : creates_) {
      if (c.rule != ri || !c.has_value) continue;
      for (size_t j = 0; j < entries_.size(); ++j) {
        const EntryRec& e = entries_[j];
        if (used[j] || e.rule != ri || !e.has_value || e.value != c.value || e.event <= c.call) continue;
        used[j] = true;
        add_edge(c.call, e.event, rule);
        spawns.push_back(SpawnRec{c.actor, e.actor, c.call, e.event});
        break;
      }
    }
  }
  for (size_t ri = 0; ri < p_.rules.size(); ++ri) {
    const HbRuleDecl& rule = p_.rules[ri];
    if (rule.tmpl != RuleTemplate::Phase || !rule_applies(ri)) continue;
    for (const TraceEvent& ev : t_.events) {
      if (r_.terminated && ev.id > r_.terminal_event) break;
      if (ev.site != rule.from_op) continue;
      const char* ph = ev.phase == Phase::Call ? "call" : ev.phase == Phase::Return ? "return" : "point";
      if (rule.from_phase != ph) continue;
      for (const SpawnRec& s : spawns) {
        if (s.parent != ev.actor || s.call <= ev.id) continue;
        const auto first = first_event_.find(s.child);
        if (first != first_event_.end()) add_edge(ev.id, first->second, rule);
      }
    }
  }
}

}  // namespace

Status replay(const Profile& p, const Trace& trace, const Options& options, Result* out) {
  *out = Result();
  Replayer r(p, trace, options, out);
  return r.run();
}

Status resolve_rule_predicate(const Profile& p, const std::string& rule, const std::string& text, phi::ExprPtr* out, std::vector<phi::Diag>* diags) {
  const int32_t ri = p.find_rule(rule);
  if (ri < 0 || p.rules[static_cast<size_t>(ri)].tmpl != RuleTemplate::Observe) return Status::error(Err::Invalid, "rule '" + rule + "' is not an observation rule of the profile");
  const int32_t oi = p.find_op(p.rules[static_cast<size_t>(ri)].poll_fn);
  if (oi < 0) return Status::error(Err::Invalid, "the poll operation '" + p.rules[static_cast<size_t>(ri)].poll_fn + "' of rule '" + rule + "' is not declared");
  phi::ExprContext ctx;
  ctx.exec = true;
  ctx.result = false;
  for (const phi::OpParam& prm : p.ops[static_cast<size_t>(oi)].params) {
    if (prm.role == "out") continue;
    phi::Sort s;
    s.kind = phi::SortKind::Int;
    if (prm.has_sort) s = prm.sort.resolved;
    ctx.args.push_back(phi::ArgSpec{"arg." + prm.name, s});
  }
  const Status st = phi::parse_expression(text, out, diags);
  if (!st.ok()) return st;
  phi::Sort want;
  want.kind = phi::SortKind::Bool;
  if (!phi::resolve_expression(p, out->get(), ctx, want, diags)) return Status::error(Err::Invalid, "the predicate does not resolve");
  return Status::success();
}

// ---- canonical text of a result -----------------------------------------------------------------

namespace {

// "{a,b}" with the display names of the set enumeration `name`.
std::string set_text(const char* name, uint64_t v) {
  const EnumDesc* en = find_enum(name);
  std::string out;
  if (en != nullptr)
    for (size_t i = 0; i < en->nitems; ++i)
      if ((v & en->items[i].value) != 0) out += (out.empty() ? "" : ",") + std::string(en->items[i].display);
  return "{" + out + "}";
}

std::string enum_text_of(const char* name, uint64_t v) {
  const EnumDesc* en = find_enum(name);
  const char* d = en != nullptr ? enum_display(*en, v) : nullptr;
  return d != nullptr ? d : "?";
}

std::string describe(const Trace& t, uint64_t id) {
  if (id >= t.events.size()) return "#" + std::to_string(id);
  const TraceEvent& e = t.events[id];
  return "#" + std::to_string(id) + " " + e.actor + " " + e.site + " " + phase_text(e.phase);
}

}  // namespace

std::string dump_result(const Profile& p, const Trace& trace, const Result& r) {
  std::string out;
  const bool real = r.record == RecordKind::Real;
  out += "replay profile " + phi::quote_string(p.name) + "\n";
  if (real) out += "record real\n";
  out += "events " + std::to_string(trace.events.size()) + "\n";
  out += std::string("terminated ") + (r.terminated ? "yes at " + describe(trace, r.terminal_event) : std::string("no")) + "\n";
  out += std::string("shutdown_request ") + (r.shutdown_requested ? "yes at " + describe(trace, r.shutdown_event) : std::string("no")) + "\n";
  out += "\nevent table\n";
  for (const TraceEvent& e : trace.events) out += "  " + describe(trace, e.id) + "\n";
  out += "\nedges " + std::to_string(r.edges.size()) + "\n";
  for (const HbEdge& e : r.edges)
    out += "edge " + e.rule + " " + phi::stratum_text(e.stratum) + " " + phi::robust_text(e.robust) + (e.minimal ? " minimal" : "") + " #" + std::to_string(e.from) + " -> #" +
           std::to_string(e.to) + "\n";
  out += "\noccurrences " + std::to_string(r.occurrences.size()) + "\n";
  for (const Occurrence& o : r.occurrences) {
    out += "occurrence " + o.rspec + " actor " + o.actor + " ordinal " + std::to_string(o.ordinal) + " key " + key_text(o.key) + " call #" + std::to_string(o.call);
    out += o.returned ? " return #" + std::to_string(o.ret) : std::string(" not returned");
    if (!o.context.empty()) out += " context " + o.context;
    out += "\n";
    out += std::string("  status ") + occ_status_text(o.status);
    if (o.decided_at != DecidedAt::None)
      out += std::string(" decided at ") + decided_at_text(o.decided_at) + " #" + std::to_string(o.decision) + (o.in_scope ? " scope default" : " scope teardown");
    if (real && !o.possible.empty()) {
      out += " possible {";
      for (size_t i = 0; i < o.possible.size(); ++i) out += std::string(i != 0 ? "," : "") + occ_status_text(o.possible[i]);
      out += "}";
    }
    out += "\n";
    if (!o.monitor.empty()) out += "  monitor " + o.monitor + "\n";
    for (const ConjunctResult& c : o.conjuncts) {
      out += "  conjunct " + c.label + " " + lp_source_text(c.source);
      if (c.source != LpSource::NotPassed || (real && (c.unknown || c.passage_unknown))) {
        if (c.source != LpSource::NotPassed) out += " #" + std::to_string(c.event);
        out += c.evaluated ? (c.holds ? " holds" : " fails") : (real && c.unknown) ? " unknown" : " not evaluated";
      }
      if (real) {
        if (c.passage_unknown) out += " passage-unknown";
        if (c.contradicted) out += " contradicted";
        if ((c.unknown || c.passage_unknown) && c.reasons != 0) out += " reasons " + set_text("KZ_WITHHOLD", c.reasons);
      }
      out += "\n";
    }
    if (o.has_result) out += "  result " + hex_or_dec(static_cast<int64_t>(u32(o.result))) + " outcome " + outcome_check_text(o.outcome) + "\n";
    if (real && o.returned && o.ret < trace.events.size()) {
      // the out values the record gives on the return (what the call stored through its out parameters)
      std::string outs;
      for (const auto& v : trace.events[o.ret].values)
        if (v.first.rfind("out.", 0) == 0) outs += " " + v.first.substr(4) + "=" + hex_or_dec(v.second);
      if (!outs.empty()) out += "  out" + outs + "\n";
    }
    if (real) {
      out += "  summary " + render_observation(o.summary) + " flags " + set_text("KZ_SUMMARY_FLAG", o.summary.flags) + "\n";
      out += "  adequacy " + set_text("KZ_ADEQ", o.adequacy) + "\n";
      out += "  identity exec " + o.actor + " creator " + (o.creator.empty() ? std::string("(not recorded)") : o.creator) + " owner " + enum_text_of("KZ_OWNER_LEVEL", o.owner_level);
      if (o.has_owner_key) out += " " + key_text(o.owner_key);
      if (!o.passed_role.empty()) out += " passed " + o.passed_role + "=" + (o.has_passed ? hex_or_dec(o.passed) : std::string("(not recorded)"));
      out += "\n";
      out += "  relation " + enum_text_of("KZ_EXEC_RELATION", o.relation);
      if (o.relation != KZ_EXEC_RELATION_OWNERSHIP_UNDETERMINED) out += " owners " + enum_text_of("KZ_OWNER_LEVEL", o.relation_level);
      out += ": " + o.relation_note + "\n";
    }
  }
  out += "\nfindings " + std::to_string(r.findings.size()) + "\n";
  for (const ReplayFinding& f : r.findings)
    out += std::string("finding ") + (f.event == kNoEvent ? std::string("-") : "#" + std::to_string(f.event)) + ": " + f.what + "\n";
  if (!r.role_facts.empty() || !p.roles.empty()) {
    out += "\nrole facts " + std::to_string(r.role_facts.size()) + "\n";
    for (const RoleFact& f : r.role_facts) {
      out += "role_fact " + f.role + " purpose " + f.purpose + " actor " + f.actor + " " + f.op + (f.call == kNoEvent ? std::string() : " call #" + std::to_string(f.call)) +
             " return #" + std::to_string(f.ret) + (f.has_result ? " result " + hex_or_dec(f.result) : std::string(" result -")) + " site #" + std::to_string(f.point);
      if (!f.excluded.empty()) {
        out += " not an instance of";
        for (const std::string& e : f.excluded) out += " " + e;
      }
      out += "\n";
    }
  }
  out += "\nguard failures " + std::to_string(r.guard_failures.size()) + "\n";
  for (const GuardFailure& g : r.guard_failures) {
    out += "guard_failure #" + std::to_string(g.event) + " effect " + g.op + "." + g.effect + (g.trusted ? " refuted refutable_by" : " disagreement");
    for (const std::string& o : g.refutable_by) out += " " + o;
    out += "\n";
  }

  out += "\nfinal state\n";
  for (const auto& s : r.final_state.state) {
    const KindDecl& k = p.kinds[static_cast<size_t>(s.first.kind)];
    out += "  " + k.name + key_text(s.first.key) + " = " + k.states[static_cast<size_t>(s.second)];
    const auto id = r.final_state.ids.find(s.first);
    if (id != r.final_state.ids.end()) out += " id " + hex_or_dec(id->second);
    out += "\n";
  }
  for (const auto& t : r.final_state.tags) {
    const KindDecl& k = p.kinds[static_cast<size_t>(t.first.first.kind)];
    const phi::TagDecl& td = k.tags[static_cast<size_t>(t.first.second)];
    out += "  " + k.name + key_text(t.first.first.key) + " tag " + td.name + " = " + td.values[static_cast<size_t>(t.second)] + "\n";
  }
  for (const auto& l : r.final_state.loc_values) out += "  loc " + p.locs[static_cast<size_t>(l.first)].name + " = " + hex_or_dec(l.second) + "\n";
  if (real) {
    // What the real record lets recorded writes fix (ADR-055); the final state above is the replay of the
    // recorded effects and is meaningful only for what is fixed here.
    out += "\nreal record knowledge\n";
    out += std::string("  partial ") + (r.partial ? "yes" : "no") + "\n";
    out += "  losses " + std::to_string(trace.losses.size()) + "\n";
    for (const Knowledge& k : r.knowledge) {
      out += std::string("  ") + (k.is_loc ? "loc " : "kind ") + k.name + (k.reasons == 0 ? " fixed" : " open " + set_text("KZ_WITHHOLD", k.reasons));
      out += " established " + set_text("KZ_ADEQ", k.adequacy) + (k.contradicted ? " contradicted" : "") + "\n";
    }
  }
  return out;
}

// ---- the text form of a trace --------------------------------------------------------------------

namespace {

std::vector<std::string> split_ws(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : line) {
    if (c == ' ' || c == '\t' || c == '\r') {
      if (!cur.empty()) out.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}

bool parse_int(const std::string& s, int64_t* out) {
  if (s.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const long long v = std::strtoll(s.c_str(), &end, 0);
  if (end == nullptr || *end != '\0' || errno != 0) {
    // 0xc2000003 and similar values above INT64_MAX do not occur; unsigned 64-bit values are accepted too
    errno = 0;
    const unsigned long long u = std::strtoull(s.c_str(), &end, 0);
    if (end == nullptr || *end != '\0' || errno != 0) return false;
    *out = static_cast<int64_t>(u);
    return true;
  }
  *out = v;
  return true;
}

Status line_error(size_t line, const std::string& what) { return Status::error(Err::Invalid, "trace line " + std::to_string(line) + ": " + what); }

}  // namespace

Status parse_trace_text(const std::string& text, Trace* out) {
  *out = Trace();
  size_t line_no = 0;
  size_t pos = 0;
  while (pos <= text.size()) {
    size_t nl = text.find('\n', pos);
    if (nl == std::string::npos) nl = text.size();
    std::string line = text.substr(pos, nl - pos);
    pos = nl + 1;
    ++line_no;
    const size_t hash = line.find('#');
    if (hash != std::string::npos) line.resize(hash);
    const std::vector<std::string> tok = split_ws(line);
    if (tok.empty()) continue;
    if (tok[0] == "const") {
      int64_t v = 0;
      if (tok.size() != 3 || !parse_int(tok[2], &v)) return line_error(line_no, "expected: const NAME VALUE");
      out->ref_consts[tok[1]] = v;
      continue;
    }
    if (tok[0] == "record") {
      if (tok.size() != 2 || (tok[1] != "real" && tok[1] != "model")) return line_error(line_no, "expected: record real|model");
      if (!out->events.empty()) return line_error(line_no, "'record' must precede the events");
      out->record = tok[1] == "real" ? RecordKind::Real : RecordKind::Model;
      continue;
    }
    if (tok[0] == "start_evidence" || tok[0] == "adequacy" || tok[0] == "observed" || tok[0] == "owner_level" || tok[0] == "owner_id" || tok[0] == "owner_core" ||
        tok[0] == "creator" || tok[0] == "loss" || tok[0] == "order") {
      if (out->record != RecordKind::Real) return line_error(line_no, "'" + tok[0] + "' belongs to a real record: 'record real' must come first");
      if (tok[0] == "start_evidence") {
        static const char* names[] = {"none", "init", "state-capture", "declared"};
        bool ok = false;
        for (uint8_t i = 0; i < 4; ++i)
          if (tok.size() == 2 && tok[1] == names[i]) {
            out->start_evidence = i;
            ok = true;
          }
        if (!ok) return line_error(line_no, "expected: start_evidence none|init|state-capture|declared");
      } else if (tok[0] == "adequacy") {
        if (tok.size() < 3 || (tok[1] != "kind" && tok[1] != "loc")) return line_error(line_no, "expected: adequacy kind|loc NAME [start=..] [writers=..] [aliases=..] [lifetime=..]");
        Adequacy a;
        for (size_t i = 3; i < tok.size(); ++i) {
          const std::string& t = tok[i];
          if (t == "start=known") a.start_known = true;
          else if (t == "start=unknown") a.start_known = false;
          else if (t == "writers=covered") a.writers_covered = true;
          else if (t == "writers=uncovered") a.writers_covered = false;
          else if (t == "aliases=covered") a.aliases_covered = true;
          else if (t == "aliases=uncovered") a.aliases_covered = false;
          else if (t == "lifetime=known" && tok[1] == "kind") a.lifetime_known = true;
          else if (t == "lifetime=unknown" && tok[1] == "kind") a.lifetime_known = false;
          else return line_error(line_no, "adequacy: '" + t + "' is not start=known|unknown, writers=covered|uncovered, aliases=covered|uncovered or (a kind) lifetime=known|unknown");
        }
        (tok[1] == "kind" ? out->kind_adequacy : out->loc_adequacy)[tok[2]] = a;
      } else if (tok[0] == "order") {
        if (tok.size() != 2 || (tok[1] != "established" && tok[1] != "merged")) return line_error(line_no, "expected: order established|merged");
        out->order_established = tok[1] == "established";
      } else if (tok[0] == "observed") {
        if (tok.size() != 2) return line_error(line_no, "expected: observed POINT");
        out->observed_points.insert(tok[1]);
      } else if (tok[0] == "owner_level") {
        uint8_t lv = 0;
        if (tok.size() != 3) return line_error(line_no, "expected: owner_level ACTOR confirmed|inferred|undetermined");
        if (tok[2] == "undetermined") lv = KZ_OWNER_LEVEL_UNDETERMINED;
        else if (tok[2] == "inferred") lv = KZ_OWNER_LEVEL_INFERRED;
        else if (tok[2] == "confirmed") lv = KZ_OWNER_LEVEL_CONFIRMED;
        else return line_error(line_no, "expected: owner_level ACTOR confirmed|inferred|undetermined");
        out->owner_levels[tok[1]] = lv;
      } else if (tok[0] == "owner_id") {
        int64_t v = 0;
        if (tok.size() != 3 || !parse_int(tok[2], &v) || v < 0 || v > 0xffffffffll) return line_error(line_no, "expected: owner_id ACTOR VALUE (a 32-bit identity value)");
        out->owner_ids[tok[1]] = static_cast<uint32_t>(v);
      } else if (tok[0] == "owner_core") {
        if (tok.size() != 3 || (tok[2] != "yes" && tok[2] != "no")) return line_error(line_no, "expected: owner_core ACTOR yes|no");
        out->owner_core[tok[1]] = tok[2] == "yes";
      } else if (tok[0] == "creator") {
        if (tok.size() != 3) return line_error(line_no, "expected: creator ACTOR CREATING_ACTOR");
        out->creators[tok[1]] = tok[2];
      } else {  // loss
        if (tok.size() != 2 || (tok[1] != "begin" && tok[1] != "end")) return line_error(line_no, "expected: loss begin|end");
        const bool open = !out->losses.empty() && out->losses.back().end == kNoEvent;
        if (tok[1] == "begin") {
          if (open) return line_error(line_no, "loss begin: the previous loss has not ended");
          LossInterval l;
          l.begin = out->events.size();
          out->losses.push_back(l);
        } else {
          if (!open) return line_error(line_no, "loss end without a loss begin");
          out->losses.back().end = out->events.size();
        }
      }
      continue;
    }
    if (tok[0] == "owner") {
      if (tok.size() < 3) return line_error(line_no, "expected: owner ACTOR KEY [KEY ...]");
      std::vector<int64_t> key;
      for (size_t i = 2; i < tok.size(); ++i) {
        int64_t v = 0;
        if (!parse_int(tok[i], &v)) return line_error(line_no, "owner key '" + tok[i] + "' is not an integer");
        key.push_back(v);
      }
      out->owners[tok[1]] = key;
      continue;
    }
    if (tok.size() < 3) return line_error(line_no, "expected: ACTOR SITE call|ret|pt [name=value ...]");
    TraceEvent ev;
    ev.actor = tok[0];
    ev.site = tok[1];
    if (tok[2] == "call") ev.phase = Phase::Call;
    else if (tok[2] == "ret") ev.phase = Phase::Return;
    else if (tok[2] == "pt") ev.phase = Phase::Point;
    else return line_error(line_no, "'" + tok[2] + "' is not call, ret or pt");
    for (size_t i = 3; i < tok.size(); ++i) {
      const size_t eq = tok[i].find('=');
      if (eq == std::string::npos || eq == 0 || eq + 1 == tok[i].size()) return line_error(line_no, "expected name=value, found '" + tok[i] + "'");
      const std::string name = tok[i].substr(0, eq), value = tok[i].substr(eq + 1);
      if (value[0] == '@') {
        ev.texts[name] = value.substr(1);
        continue;
      }
      int64_t v = 0;
      if (!parse_int(value, &v)) return line_error(line_no, "value '" + value + "' of " + name + " is not an integer");
      if (name == "result") {
        ev.has_result = true;
        ev.result = v;
      } else {
        ev.values[name] = v;
      }
    }
    out->add(std::move(ev));
  }
  return Status::success();
}

std::string trace_text(const Trace& t) {
  std::string out;
  auto num = [](int64_t v) { return hex_or_dec(v); };
  if (t.record == RecordKind::Real) {
    static const char* evidence[] = {"none", "init", "state-capture", "declared"};
    out += "record real\n";
    out += std::string("start_evidence ") + (t.start_evidence < 4 ? evidence[t.start_evidence] : "none") + "\n";
    out += std::string("order ") + (t.order_established ? "established" : "merged") + "\n";
    auto adequacy = [&](const char* what, const std::string& name, const Adequacy& a, bool kind) {
      out += std::string("adequacy ") + what + " " + name + (a.start_known ? " start=known" : " start=unknown") + (a.writers_covered ? " writers=covered" : " writers=uncovered") +
             (a.aliases_covered ? " aliases=covered" : " aliases=uncovered");
      if (kind) out += a.lifetime_known ? " lifetime=known" : " lifetime=unknown";
      out += "\n";
    };
    for (const auto& kv : t.kind_adequacy) adequacy("kind", kv.first, kv.second, true);
    for (const auto& kv : t.loc_adequacy) adequacy("loc", kv.first, kv.second, false);
    for (const std::string& pt : t.observed_points) out += "observed " + pt + "\n";
    for (const auto& kv : t.owner_levels) {
      static const char* lv[] = {"undetermined", "inferred", "confirmed"};
      out += "owner_level " + kv.first + " " + (kv.second < 3 ? lv[kv.second] : "undetermined") + "\n";
    }
    for (const auto& kv : t.owner_ids) out += "owner_id " + kv.first + " " + num(static_cast<int64_t>(kv.second)) + "\n";
    for (const auto& kv : t.owner_core) out += "owner_core " + kv.first + (kv.second ? " yes" : " no") + "\n";
    for (const auto& kv : t.creators) out += "creator " + kv.first + " " + kv.second + "\n";
  }
  for (const auto& kv : t.ref_consts) out += "const " + kv.first + " " + num(kv.second) + "\n";
  for (const auto& kv : t.owners) {
    out += "owner " + kv.first;
    for (int64_t k : kv.second) out += " " + num(k);
    out += "\n";
  }
  for (size_t i = 0; i <= t.events.size(); ++i) {
    // loss markers sit before the event at their position
    for (size_t j = 0; j < t.losses.size(); ++j) {
      if (t.losses[j].begin == i) out += "loss begin\n";
      if (t.losses[j].end == i) out += "loss end\n";
    }
    if (i == t.events.size()) break;
    const TraceEvent& e = t.events[i];
    out += e.actor + " " + e.site + " " + phase_text(e.phase);
    for (const auto& kv : e.values) out += " " + kv.first + "=" + num(kv.second);
    for (const auto& kv : e.texts) out += " " + kv.first + "=@" + kv.second;
    if (e.has_result) out += " result=" + num(e.result);
    out += "\n";
  }
  return out;
}

}  // namespace eval
}  // namespace kz

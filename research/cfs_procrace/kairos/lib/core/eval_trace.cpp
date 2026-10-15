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
#include <set>
#include <tuple>

#include "kz/core/phi_allowed.hpp"

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
  }
  return "?";
}

const char* lp_source_text(LpSource s) {
  switch (s) {
    case LpSource::Capture: return "capture";
    case LpSource::Call: return "call";
    case LpSource::Point: return "point";
    case LpSource::NotPassed: return "not_passed";
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

struct Frame {
  size_t op = 0;
  uint64_t call = 0;
  ArgMap args;
  std::vector<ConjunctResult> conj;  // one per conjunct of the operation
  std::vector<size_t> occs;          // indices into Result::occurrences, one per rspec of the operation
  std::vector<PollWatch> watches;    // one per observation rule that polls in this operation
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
  return Status::success();
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

  // occurrences
  for (size_t si : rspecs_of_op_[oi]) {
    const phi::RspecDecl& rs = p_.rspecs[si];
    Occurrence o;
    o.rspec = rs.name;
    o.actor = ev.actor;
    o.call = ev.id;
    CVal key;
    if (rs.key && evaluate(*rs.key, op, nullptr, ev, &f.args, &key) && key.defined) o.key = key.key;
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
  if (res.evaluated) return;  // the first pass of the linearization point decides
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
      if (!c.evaluated) {
        all_true = false;
        continue;
      }
      if (!c.holds) {
        all_true = false;
        if (bad == nullptr || c.event < bad->event) bad = &c;
      }
    }
    if (bad != nullptr) {
      const bool at_call = bad->source != LpSource::Point;
      decide(&o, not_ready_status(info), at_call ? DecidedAt::Call : DecidedAt::Point, at_call ? f->call : bad->event);
    } else if (all_true) {
      uint64_t last = f->call;
      bool in_call_only = true;
      for (size_t ci : info.conj) {
        if (f->conj[ci].source == LpSource::Point) {
          in_call_only = false;
          last = std::max(last, f->conj[ci].event);
        }
      }
      decide(&o, OccStatus::Ready, in_call_only ? DecidedAt::Call : DecidedAt::Point, in_call_only ? f->call : last);
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

  // spawn: the value the create operation returned through its out parameter
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
    CVal v;
    if (!evaluate(*pred, op, nullptr, ev, &f.args, &v)) continue;
    if (!v.is_true()) {
      finding(ev.id, "rule " + rule.name + ": the poll " + op.name + " returned its exit outcome while its awaited predicate is false in the replay");
      continue;
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

void Replayer::on_point(const TraceEvent& ev) {
  bool known = false;
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
    if (c.lp.kind == LpKind::Capture) {
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
    apply_effect(op, op.effects[ref.index], ev, f != nullptr ? &f->args : nullptr);
    applied_any = true;
  }
  if (applied_any) update_watches(ev);
}

void Replayer::apply_effect(const OpDecl& op, const EffectDecl& eff, const TraceEvent& ev, const ArgMap* frame_args) {
  const std::string what = "effect " + op.name + "." + eff.label;
  if (!guards_hold(eff.guards, ev.id, what)) return;
  CVal v;
  if (eff.when) {
    if (!evaluate(*eff.when, op, &eff.lp, ev, frame_args, &v)) return;
    if (!v.is_true()) return;
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

std::string describe(const Trace& t, uint64_t id) {
  if (id >= t.events.size()) return "#" + std::to_string(id);
  const TraceEvent& e = t.events[id];
  return "#" + std::to_string(id) + " " + e.actor + " " + e.site + " " + phase_text(e.phase);
}

}  // namespace

std::string dump_result(const Profile& p, const Trace& trace, const Result& r) {
  std::string out;
  out += "replay profile " + phi::quote_string(p.name) + "\n";
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
    out += "\n";
    out += std::string("  status ") + occ_status_text(o.status);
    if (o.decided_at != DecidedAt::None)
      out += std::string(" decided at ") + decided_at_text(o.decided_at) + " #" + std::to_string(o.decision) + (o.in_scope ? " scope default" : " scope teardown");
    out += "\n";
    for (const ConjunctResult& c : o.conjuncts) {
      out += "  conjunct " + c.label + " " + lp_source_text(c.source);
      if (c.source != LpSource::NotPassed) {
        out += " #" + std::to_string(c.event);
        out += c.evaluated ? (c.holds ? " holds" : " fails") : " not evaluated";
      }
      out += "\n";
    }
    if (o.has_result) out += "  result " + hex_or_dec(static_cast<int64_t>(u32(o.result))) + " outcome " + outcome_check_text(o.outcome) + "\n";
  }
  out += "\nfindings " + std::to_string(r.findings.size()) + "\n";
  for (const ReplayFinding& f : r.findings)
    out += std::string("finding ") + (f.event == kNoEvent ? std::string("-") : "#" + std::to_string(f.event)) + ": " + f.what + "\n";

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

}  // namespace eval
}  // namespace kz

// predict.cpp -- the record anchor, the prediction search, the re-judgement of candidates, hold plans and the
// report of `kz predict` (kz/predict/predict.hpp).
#include <algorithm>
#include <set>

#include "kz/core/verdict.hpp"
#include "kz/predict/predict.hpp"

namespace kz::predict {

namespace {

const phi::HbRuleDecl* rule_named(const phi::Profile& p, const std::string& n) {
  const int32_t i = p.find_rule(n);
  return i >= 0 ? &p.rules[static_cast<size_t>(i)] : nullptr;
}

const char* stratum_name(phi::Stratum s) { return phi::stratum_text(s); }

// The cone steps of the record, per model actor, and where each record event falls among them.
struct RecMap {
  std::vector<std::vector<Tuple>> tuples;
  std::vector<std::vector<uint64_t>> tuple_call;  // the first event of each tuple
  std::map<uint64_t, std::pair<int32_t, int32_t>> at;  // event -> (actor, tuple) for events inside a kept tuple
  std::vector<std::string> outside;
  std::string error;
};

const Instr* instr_for_op(const Actor& a, const std::string& op) {
  for (const Instr& in : a.prog)
    if (ins_visible(in.op) && (in.opname == op || ((in.op == Ins::Clear || in.op == Ins::Set) && in.site == op))) return &in;
  return nullptr;
}

RecMap record_map(const PredictInput& in) {
  const Model& m = *in.model;
  const eval::Trace& t = *in.record;
  RecMap rm;
  rm.tuples.resize(m.actors.size());
  rm.tuple_call.resize(m.actors.size());
  std::map<std::string, int32_t> by_record;
  for (size_t i = 0; i < m.actors.size(); ++i) by_record[m.actors[i].record] = static_cast<int32_t>(i);
  std::set<std::string> keys;
  for (const Resource& r : m.resources)
    if (!r.key_text.empty()) keys.insert(r.key_text);
  // ids of the cone's objects: the out value of a successful creation call of a key of the cone
  std::map<int64_t, std::string> id_key;
  {
    std::map<std::string, std::string> open_key;  // actor -> key of its open creation call
    for (const eval::TraceEvent& e : t.events) {
      auto ai = by_record.find(e.actor);
      if (ai == by_record.end()) continue;
      const Instr* ci = instr_for_op(m.actors[static_cast<size_t>(ai->second)], e.site);
      if (ci == nullptr || ci->op != Ins::Create) continue;
      if (e.phase == eval::Phase::Call) {
        auto tx = e.texts.find("arg." + ci->key_param);
        open_key[e.actor] = tx != e.texts.end() ? tx->second : std::string();
      } else if (e.phase == eval::Phase::Return) {
        auto ov = e.values.find("out." + ci->out_param);
        if (ov != e.values.end() && e.has_result && e.result == ci->ready_code && keys.count(open_key[e.actor]) != 0) id_key[ov->second] = open_key[e.actor];
      }
    }
  }
  // the spawn edges of the record: creating call -> the child's actor
  std::map<uint64_t, std::string> spawned;
  for (const eval::HbEdge& ed : in.record_result->edges) {
    const phi::HbRuleDecl* r = rule_named(*in.profile, ed.rule);
    if (r != nullptr && r->tmpl == phi::RuleTemplate::Spawn && ed.to < t.events.size()) spawned[ed.from] = t.events[ed.to].actor;
  }
  // a record event of an actor outside the model on a key of the cone: the cone is not closed for this record
  for (const eval::TraceEvent& e : t.events) {
    if (by_record.count(e.actor) != 0) continue;
    for (const auto& kv : e.texts)
      if (keys.count(kv.second) != 0) {
        rm.error = "the record's actor " + e.actor + " (not in the model) uses the key " + kv.second + " at event #" + std::to_string(e.id) + " (" + e.site + "): the cone is not closed";
        return rm;
      }
    for (const auto& kv : e.values)
      if (kv.first.compare(0, 4, "arg.") == 0 && id_key.count(kv.second) != 0 && e.phase == eval::Phase::Call) {
        rm.error = "the record's actor " + e.actor + " (not in the model) passes the id of " + id_key[kv.second] + " at event #" + std::to_string(e.id) + " (" + e.site + "): the cone is not closed";
        return rm;
      }
  }
  std::map<std::string, uint64_t> outside_counts;
  for (size_t ai = 0; ai < m.actors.size(); ++ai) {
    const Actor& a = m.actors[ai];
    int32_t open = -1;         // the kept tuple whose call is open
    bool open_dropped = false; // a dropped call is open
    bool open_result = false;  // the tuple of the open call carries the result (uses, creations, deletions)
    std::string open_op;
    for (const eval::TraceEvent& e : t.events) {
      if (e.actor != a.record) continue;
      const Instr* ci = instr_for_op(a, e.site);
      if (open >= 0 || open_dropped) {
        if (e.phase == eval::Phase::Return && e.site == open_op) {
          if (open >= 0) {
            Tuple& tu = rm.tuples[ai][static_cast<size_t>(open)];
            if (e.has_result && open_result) {
              tu.has_result = true;
              // a status the record carries as an unsigned 32-bit register value is the signed code of the API
              tu.result = (e.result >= 0x80000000LL && e.result <= 0xffffffffLL) ? e.result - 0x100000000LL : e.result;
            }
            rm.at[e.id] = {static_cast<int32_t>(ai), open};
          }
          open = -1;
          open_dropped = false;
          continue;
        }
        if (open >= 0) rm.at[e.id] = {static_cast<int32_t>(ai), open};
        continue;
      }
      if (ci == nullptr) continue;
      if ((ci->op == Ins::Clear || ci->op == Ins::Set) && e.phase == eval::Phase::Point && e.site == ci->site) {
        Tuple tu;
        tu.op = e.site;
        rm.tuples[ai].push_back(tu);
        rm.tuple_call[ai].push_back(e.id);
        rm.at[e.id] = {static_cast<int32_t>(ai), static_cast<int32_t>(rm.tuples[ai].size() - 1)};
        continue;
      }
      if (e.phase != eval::Phase::Call || ci->op == Ins::Clear || ci->op == Ins::Set) continue;
      Tuple tu;
      tu.op = e.site;
      std::string why;
      if (ci->op == Ins::Use || ci->op == Ins::Create) {
        auto tx = e.texts.find("arg." + ci->key_param);
        tu.key = tx != e.texts.end() ? tx->second : std::string();
        if (keys.count(tu.key) == 0) why = "a key outside the cone (" + (tu.key.empty() ? std::string("not recorded") : tu.key) + ")";
      } else if (ci->op == Ins::Spawn) {
        auto sp = spawned.find(e.id);
        if (sp == spawned.end() || by_record.count(sp->second) == 0) why = "spawns an actor outside the cone (" + (sp == spawned.end() ? std::string("unknown") : sp->second) + ")";
        else tu.key = m.actors[static_cast<size_t>(by_record[sp->second])].name;
      } else if (ci->op == Ins::DeleteEach) {
        auto iv = e.values.find("arg." + ci->key_param);
        if (iv == e.values.end() || id_key.count(iv->second) == 0) why = "an object outside the cone";
        else tu.key = id_key[iv->second];
      }
      if (!why.empty()) {
        ++outside_counts[a.name + " " + e.site + ": " + why];
        open_dropped = ci->op != Ins::Exit;
        open_op = e.site;
        continue;
      }
      rm.tuples[ai].push_back(tu);
      rm.tuple_call[ai].push_back(e.id);
      rm.at[e.id] = {static_cast<int32_t>(ai), static_cast<int32_t>(rm.tuples[ai].size() - 1)};
      if (ci->op != Ins::Exit) {
        open = static_cast<int32_t>(rm.tuples[ai].size() - 1);
        open_op = e.site;
        open_result = ci->op == Ins::Use || ci->op == Ins::Create || ci->op == Ins::DeleteEach;
      }
    }
  }
  for (const auto& kv : outside_counts) rm.outside.push_back(kv.first + " x" + std::to_string(kv.second));
  return rm;
}

// The visible-step index an event of a model actor stands for: its tuple, or the step whose local instructions
// emit it (the step of the tuple before it; the first step for events before the first tuple).
int32_t step_of(const RecMap& rm, int32_t ai, uint64_t ev) {
  auto it = rm.at.find(ev);
  if (it != rm.at.end()) return it->second.second;
  const std::vector<uint64_t>& calls = rm.tuple_call[static_cast<size_t>(ai)];
  int32_t k = 0;
  for (uint64_t c : calls)
    if (c < ev) ++k;
  return k == 0 ? 0 : k - 1;
}

std::string first_text(const eval::TraceEvent& e) { return e.texts.empty() ? std::string() : e.texts.begin()->second; }

std::vector<Judged> judge(const eval::Trace& t, const eval::Result& r) {
  std::vector<Judged> out;
  for (const eval::Occurrence& o : r.occurrences) {
    Judged j;
    j.actor = o.actor;
    j.key = o.call < t.events.size() ? first_text(t.events[o.call]) : std::string();
    j.ordinal = static_cast<uint32_t>(o.ordinal);
    j.status = o.status;
    j.observation = render_observation(o.summary);
    j.monitor = o.monitor;
    j.scope = o.in_scope ? "default" : "teardown";
    j.final_default = o.in_scope && (o.status == eval::OccStatus::Ready || o.status == eval::OccStatus::Tolerated || o.status == eval::OccStatus::Violating ||
                                     o.status == eval::OccStatus::NotReadyUnjudged);
    j.disagrees = o.outcome == eval::OutcomeCheck::Disagrees || o.input_error;
    out.push_back(j);
  }
  return out;
}

std::string judged_text(const Judged& j) {
  std::string s = j.actor + " " + j.key + " #" + std::to_string(j.ordinal) + " " + eval::occ_status_text(j.status) + " -> " + j.observation + " (scope " + j.scope + ")";
  if (j.disagrees) s += " [result disagrees with the state]";
  return s;
}

std::string step_text(const Model& m, const Step& s) {
  std::string t = m.actors[static_cast<size_t>(s.actor)].name + ": ";
  if (s.held) return t + "held at a gate";
  if (s.index < 0) return t + "leaves the cone";
  t += tuple_text(s.tuple);
  if (s.created) t += " [generation " + std::to_string(s.gen) + "]";
  for (const std::string& mk : s.marks) t += " {" + mk + "}";
  return t;
}

// The steps of an execution, one line each, a run of equal consecutive steps as one line with its count.
std::string order_text(const Model& m, const std::vector<Step>& w) {
  std::string o;
  uint32_t n = 0;
  for (size_t i = 0; i < w.size(); ++i) {
    if (w[i].index < 0 && !w[i].held) continue;
    const std::string t = step_text(m, w[i]);
    size_t j = i + 1;
    while (j < w.size() && (w[j].index >= 0 || w[j].held) && step_text(m, w[j]) == t) ++j;
    const size_t k = j - i;
    o += "    " + std::to_string(n + 1) + (k > 1 ? "-" + std::to_string(n + k) : std::string()) + ". " + t + (k > 1 ? " (x" + std::to_string(k) + ")" : std::string()) + "\n";
    n += static_cast<uint32_t>(k);
    i = j - 1;
  }
  return o;
}

// The occurrences, consecutive ordinals of one actor and key with the same status and summary as one line.
std::string judged_list(const std::vector<Judged>& js, bool monitor) {
  std::string o;
  for (size_t i = 0; i < js.size(); ++i) {
    size_t j = i + 1;
    while (j < js.size() && js[j].actor == js[i].actor && js[j].key == js[i].key && js[j].status == js[i].status && js[j].observation == js[i].observation && js[j].scope == js[i].scope &&
           js[j].ordinal == js[j - 1].ordinal + 1 && js[j].disagrees == js[i].disagrees)
      ++j;
    Judged first = js[i];
    std::string line = judged_text(first);
    if (j - i > 1) {
      const std::string one = " #" + std::to_string(first.ordinal) + " ";
      const size_t at = line.find(one);
      if (at != std::string::npos) line.replace(at, one.size(), " #" + std::to_string(first.ordinal) + "-#" + std::to_string(js[j - 1].ordinal) + " ");
    }
    if (monitor && !js[j - 1].monitor.empty()) line += "; " + js[j - 1].monitor;
    o += "    " + line + "\n";
    i = j - 1;
  }
  return o;
}

std::string plan_verdict(const PlanResult& pr) {
  if (pr.never) return "NOT REALIZABLE as declared: no execution under the plan completes with every gate held and released (a run that reaches a gate whose release is never reached stalls)";
  std::string v = pr.forces ? "every execution under the plan that does not stall is in the candidate's class" : "the plan leaves part of the class to the scheduler (" + std::to_string(pr.realized.size()) + " classes realized)";
  v += pr.stall ? "; a STALL is possible (a release reached before its gate holds, or a release that only a released thread reaches): such a run ends by the harness safeguard and is INCONCLUSIVE" : "; no stall";
  return v;
}

// ---- hold plans ------------------------------------------------------------------------------

struct HitPos {
  size_t step;
  size_t hit;
};

ControlPoint cp_of(const HitInst& h) {
  ControlPoint cp;
  cp.site = h.site;
  cp.caller = h.caller;
  cp.name_role = h.name_role;
  cp.name_text = h.name_text;
  return cp;
}

bool same_marker(const ControlPoint& a, const HitInst& h) {
  return a.site == h.site && a.caller == h.caller && a.name_role == h.name_role && a.name_text == h.name_text;
}

// Whether the marker of hit (step, hit) of path W is reached there for the first time in W.
bool first_hit(const std::vector<Step>& w, size_t step, size_t hit) {
  const ControlPoint cp = cp_of(w[step].hits[hit]);
  for (size_t s = 0; s < w.size(); ++s)
    for (size_t h = 0; h < w[s].hits.size(); ++h) {
      if (s == step && h == hit) return true;
      if (same_marker(cp, w[s].hits[h])) return false;
    }
  return false;
}

bool gate_before(const std::vector<Step>& w, size_t step, ControlPoint* cp) {
  for (size_t h = w[step].n_pre; h < w[step].n_before; ++h)
    if (first_hit(w, step, h)) {
      *cp = cp_of(w[step].hits[h]);
      return true;
    }
  return false;
}

bool release_after(const std::vector<Step>& w, size_t step, ControlPoint* cp, size_t* at) {
  const int32_t actor = w[step].actor;
  for (size_t s = step; s < w.size(); ++s) {
    if (w[s].actor != actor) continue;
    for (size_t h = s == step ? w[s].n_before : 0; h < w[s].hits.size(); ++h)
      if (first_hit(w, s, h)) {
        *cp = cp_of(w[s].hits[h]);
        *at = s;
        return true;
      }
  }
  return false;
}

std::string provs_of(const PredictInput& in, const Instr& ins) {
  std::string s;
  for (const std::string& pv : ins.prov) s += (s.empty() ? "" : "; ") + cite_text(*in.profile, *in.model, pv);
  return s;
}

// The plan of the needs of execution w (only < 0: all of them; else only that one); n_needs/whats/prefixes: the needs.
Plan synthesize(const PredictInput& in, const std::vector<Step>& w, int32_t only, std::vector<std::string>* whats, std::vector<std::string>* prefixes) {
  const Model& m = *in.model;
  Plan plan;
  struct Need {
    int32_t gate_actor;
    size_t gate_step;  // index into w; npos: the gate is a model instruction not reached in w
    const Instr* gate_ins;
    std::string gate_key;
    size_t after_step;  // the release is after this step of w
    std::string what;
    std::string prefix; // the class segment of the key it enforces
  };
  std::vector<Need> needs;
  struct AMerge {
    bool any = false;
    size_t gate_step = 0, create_step = 0;
    int32_t provider = -1;
    std::string what;
    std::string prefix;
  };
  std::vector<AMerge> amerge(m.actors.size());
  for (size_t ri = 0; ri < m.resources.size(); ++ri) {
    const std::string& key = m.resources[ri].key_text;
    for (size_t ci = 0; ci < m.actors.size(); ++ci) {
      if (!m.actors[ci].has_instance_use) continue;
      std::vector<size_t> uses;
      for (size_t s = 0; s < w.size(); ++s)
        if (w[s].actor == static_cast<int32_t>(ci) && w[s].ins == Ins::Use && w[s].res == static_cast<int32_t>(ri)) uses.push_back(s);
      if (uses.empty()) continue;
      size_t cp = w.size();
      for (size_t s = 0; s < w.size(); ++s)
        if (w[s].created && w[s].res == static_cast<int32_t>(ri)) {
          cp = s;
          break;
        }
      int32_t provider = cp < w.size() ? w[cp].actor : -1;
      const Instr* create_ins = nullptr;
      for (size_t ai = 0; ai < m.actors.size() && create_ins == nullptr; ++ai)
        for (const Instr& x : m.actors[ai].prog)
          if (x.op == Ins::Create && x.res_index == static_cast<int32_t>(ri) && (provider < 0 || provider == static_cast<int32_t>(ai))) {
            create_ins = &x;
            provider = static_cast<int32_t>(ai);
            break;
          }
      size_t k = 0;
      for (size_t u : uses)
        if (u < cp) ++k;
      const std::string who = m.actors[ci].name + "/" + m.resources[ri].name;
      if (k == 0) {
        AMerge& am = amerge[ci];
        if (!am.any || uses.front() < am.gate_step) am.gate_step = uses.front();
        if (!am.any || cp > am.create_step) am.create_step = cp;
        if (am.any && am.provider != provider) plan.notes.push_back(who + ": the creations come from two actors; only the first is enforced");
        if (!am.any) am.provider = provider;
        am.any = true;
        am.what += (am.what.empty() ? "" : ", ") + who + ": the creation before the first use";
        if (am.prefix.empty()) am.prefix = who + ":";
      } else if (create_ins == nullptr) {
        plan.notes.push_back(who + ": no creation in the model; nothing to hold");
      } else {
        const bool all = k == uses.size();
        needs.push_back(Need{provider, cp < w.size() ? cp : std::string::npos, create_ins, key, uses[k - 1],
                             who + (all ? ": the creation after the last use (" + std::to_string(k) + " uses)" : ": the creation after use " + std::to_string(k) + " (the use after it is left to the scheduler)"),
                             who + ":"});
        if (!all) plan.notes.push_back(who + ": the order of use " + std::to_string(k + 1) + " after the creation is not enforced: the hold vocabulary acts on the first qualifying hit only");
      }
    }
  }
  for (size_t ci = 0; ci < m.actors.size(); ++ci)
    if (amerge[ci].any) {
      if (amerge[ci].create_step >= w.size()) {
        plan.notes.push_back(amerge[ci].what + ": no creation in the execution");
        continue;
      }
      needs.push_back(Need{static_cast<int32_t>(ci), amerge[ci].gate_step, nullptr, std::string(), amerge[ci].create_step, amerge[ci].what, amerge[ci].prefix});
    }
  if (whats != nullptr)
    for (const Need& nd : needs) whats->push_back(nd.what);
  if (prefixes != nullptr)
    for (const Need& nd : needs) prefixes->push_back(nd.prefix);
  uint32_t n = 0;
  for (const Need& nd : needs) {
    ++n;
    if (only >= 0 && static_cast<int32_t>(n) - 1 != only) continue;
    ControlPoint g, r;
    bool ok = false;
    std::string gate_desc;
    if (nd.gate_step != std::string::npos) {
      ok = gate_before(w, nd.gate_step, &g);
      const Step& gs = w[nd.gate_step];
      gate_desc = step_text(m, gs) + "; " + provs_of(in, m.actors[static_cast<size_t>(gs.actor)].prog[static_cast<size_t>(gs.pc)]);
    } else if (nd.gate_ins != nullptr) {
      for (const SiteHit& h : nd.gate_ins->hits) {
        if (!h.before) continue;
        HitInst hi{h.site, true, h.caller, h.name_role, h.name_role.empty() ? std::string() : nd.gate_key};
        bool seen = false;
        for (const Step& s : w)
          for (const HitInst& x : s.hits) seen = seen || same_marker(cp_of(hi), x);
        if (!seen) {
          g = cp_of(hi);
          ok = true;
          break;
        }
      }
      gate_desc = m.actors[static_cast<size_t>(nd.gate_actor)].name + ": " + nd.gate_ins->opname + "(" + nd.gate_key + ") (not reached in the execution); " + provs_of(in, *nd.gate_ins);
    }
    if (!ok) {
      plan.notes.push_back(nd.what + ": no hold point reaches only this step first (no gate)");
      continue;
    }
    size_t rel_at = 0;
    if (!release_after(w, nd.after_step, &r, &rel_at)) {
      plan.notes.push_back(nd.what + ": no event after the step is reached there first (no release)");
      continue;
    }
    const Step& rs = w[rel_at];
    g.label = "G" + std::to_string(n) + "_" + m.actors[static_cast<size_t>(nd.gate_actor)].name;
    g.actor = nd.gate_actor;
    g.why = nd.what + " -- holds " + gate_desc;
    r.label = "R" + std::to_string(n);
    r.release = true;
    r.releases = g.label;
    r.actor = rs.actor;
    r.why = "first reached after " + step_text(m, w[nd.after_step]) + " (at " + (rs.pc >= 0 ? step_text(m, rs) : std::string("?")) + "); " +
            (rs.pc >= 0 ? provs_of(in, m.actors[static_cast<size_t>(rs.actor)].prog[static_cast<size_t>(rs.pc)]) : std::string());
    plan.points.push_back(g);
    plan.points.push_back(r);
  }
  // kzp-hold 1 and the debug registers: one control point per address, at most four addresses
  std::set<std::string> sites;
  bool clash = false;
  for (const ControlPoint& cp : plan.points) clash = !sites.insert(cp.site).second || clash;
  if (clash) {
    plan.notes.push_back("two control points on one site: the supervisor acts on one point per address; no plan");
    plan.points.clear();
  } else if (sites.size() > 4) {
    plan.notes.push_back("more than four sites: more than the four debug registers of a thread; no plan");
    plan.points.clear();
  }
  plan.any = !plan.points.empty();
  return plan;
}

std::vector<OrderConstraint> constraints_of(const std::vector<EdgeInfo>& edges, bool all) {
  std::vector<OrderConstraint> out;
  for (const EdgeInfo& e : edges)
    if (e.constraint || all) out.push_back(e.order);
  return out;
}

int32_t pos_of(const std::vector<Step>& w, int32_t actor, int32_t index) {
  for (size_t s = 0; s < w.size(); ++s)
    if (w[s].actor == actor && w[s].index == index) return static_cast<int32_t>(s);
  return -1;
}

}  // namespace

Status predict(const PredictInput& in, PredictReport* out) {
  *out = PredictReport();
  if (in.profile == nullptr || in.model == nullptr || in.record == nullptr || in.record_result == nullptr) return Status::error(Err::Invalid, "predict: missing input");
  const Model& m = *in.model;
  const phi::Profile& p = *in.profile;
  // the bounds no reference fixes (researcher choices) and what they leave unexplored
  out->bounds.push_back("cone: the actors, resources and operations of the model files (researcher choice); the record's events outside it are listed below, and a record event of another actor on a key of the cone refuses the prediction");
  out->bounds.push_back("granularity: each call of the model is one atomic transition at its linearization point (the object-table search or update under its lock); the call/return events commute with every other step; windows inside a call are not explored");
  out->bounds.push_back("time: untimed interleaving (delays are steps without duration): every order of the visible steps is explored, also those the real timing makes unlikely; durations and frequencies are not modelled");
  out->bounds.push_back("scope: an order in which the shutdown request comes while an instance's actor is unfinished is explored up to the request and then counted as scope-cut (no default-scope witness); after the teardown point only the teardown's own steps run");
  out->bounds.push_back("states: at most " + std::to_string(in.max_states) + " visited states per search (researcher choice); reaching it is reported and leaves the search incomplete");
  for (const auto& kv : m.consts) out->bounds.push_back("constant " + kv.first + " = " + std::to_string(kv.second) + " (from the profile or --const; loop bounds come only from such constants)");

  out->observed = judge(*in.record, *in.record_result);
  RecMap rm = record_map(in);
  out->record_tuples = rm.tuples;
  out->outside = rm.outside;
  if (!rm.error.empty()) {
    out->anchor_note = rm.error;
    return Status::success();
  }
  // edges of the record between cone steps of different actors
  for (const eval::HbEdge& ed : in.record_result->edges) {
    if (ed.from >= in.record->events.size() || ed.to >= in.record->events.size()) continue;
    int32_t from_actor = -1, to_actor = -1;
    for (size_t i = 0; i < m.actors.size(); ++i) {
      if (m.actors[i].record == in.record->events[ed.from].actor) from_actor = static_cast<int32_t>(i);
      if (m.actors[i].record == in.record->events[ed.to].actor) to_actor = static_cast<int32_t>(i);
    }
    if (from_actor < 0 || to_actor < 0 || from_actor == to_actor) continue;
    EdgeInfo ei;
    ei.rule = ed.rule;
    ei.stratum = stratum_name(ed.stratum);
    ei.from_event = ed.from;
    ei.to_event = ed.to;
    ei.order = OrderConstraint{from_actor, step_of(rm, from_actor, ed.from), to_actor, step_of(rm, to_actor, ed.to), ed.rule};
    ei.constraint = ed.stratum == phi::Stratum::S;
    ei.text = ed.rule + " (stratum " + ei.stratum + ", " + phi::robust_text(ed.robust) + ") #" + std::to_string(ed.from) + " " + in.record->events[ed.from].actor + " " +
              in.record->events[ed.from].site + " -> #" + std::to_string(ed.to) + " " + in.record->events[ed.to].actor + " " + in.record->events[ed.to].site;
    out->edges.push_back(ei);
  }
  // the anchor: an execution of the model whose steps make the record's tuples, under every recorded edge
  {
    ExploreOptions ao;
    ao.max_states = in.max_states;
    ao.follow = &rm.tuples;
    ao.constraints = constraints_of(out->edges, true);
    ExploreResult ar;
    Status s = explore(p, m, ao, &ar);
    if (!s.ok()) return s;
    if (ar.classes.empty()) {
      out->anchor_note = std::string("the model does not reproduce the record") + (ar.cap_reached ? " within the state cap" : "") + " (" + std::to_string(ar.states) + " states)";
      return Status::success();
    }
    out->anchored = true;
    out->anchor = ar.classes.front().first;
    out->anchor_key = ar.classes.front().key;
    out->anchor_note = "the model reproduces the record's cone steps under every recorded edge between them (" + std::to_string(ar.states) + " states)";
    eval::Trace at = execution_trace(m, out->anchor, in.ref_consts, in.record->owners);
    eval::Result arr;
    s = eval::replay(p, at, in.replay_options, &arr);
    if (!s.ok()) return s;
    out->anchor_judged = judge(at, arr);
  }
  // the orders the record's outcomes alone decide: a failed instance use before the creation of its resource
  for (size_t i = 0; i < out->anchor.size(); ++i) {
    const Step& u = out->anchor[i];
    if (u.ins != Ins::Use || !u.instance) continue;
    const Instr& ui = m.actors[static_cast<size_t>(u.actor)].prog[static_cast<size_t>(u.pc)];
    if (u.tuple.result != ui.not_ready_code) continue;
    for (size_t j = i + 1; j < out->anchor.size(); ++j)
      if (out->anchor[j].created && out->anchor[j].res == u.res) {
        EdgeInfo ei;
        ei.rule = "outcome";
        ei.stratum = "outcome";
        ei.order = OrderConstraint{u.actor, u.index, out->anchor[j].actor, out->anchor[j].index, "outcome-decided"};
        ei.text = "outcome-decided: " + step_text(m, u) + " before " + step_text(m, out->anchor[j]) + " (the not-ready result decides it in the observed run)";
        out->edges.push_back(ei);
        break;
      }
  }
  // the prediction search
  ExploreOptions so;
  so.max_states = in.max_states;
  so.constraints = constraints_of(out->edges, in.edges == EdgeMode::Fixed);
  if (in.outcomes == OutcomeMode::Keep) {
    so.follow = &rm.tuples;
    so.keep_outcomes = true;
    so.constraints = constraints_of(out->edges, false);
  }
  Status s = explore(p, m, so, &out->search);
  if (!s.ok()) return s;
  uint32_t wid = 0;
  for (const ClassInfo& c : out->search.classes) {
    if (c.cut) {
      ++out->cut_classes;
      out->cut_terminals += c.terminals;
      continue;
    }
    Witness w;
    w.id = ++wid;
    w.cls = c;
    w.observed = c.key == out->anchor_key;
    w.trace = execution_trace(m, c.first, in.ref_consts, in.record->owners);
    eval::Result rr;
    s = eval::replay(p, w.trace, in.replay_options, &rr);
    if (!s.ok()) return s;
    w.judged = judge(w.trace, rr);
    w.accepted = !c.invalid;
    for (const Judged& j : w.judged)
      if (j.disagrees) {
        w.accepted = false;
        w.reject = "the replay finds a result that disagrees with the state at the conjunct point (" + judged_text(j) + "): the order is infeasible with that result";
      }
    if (!rr.guard_failures.empty()) {
      w.accepted = false;
      w.reject = "guard failures in the replay: " + std::to_string(rr.guard_failures.size());
    }
    if (c.invalid) w.reject = "a step the model cannot continue (an owned resource at exit)";
    w.in_scope = w.accepted && !c.stall;
    for (const Judged& j : w.judged) w.in_scope = w.in_scope && j.final_default;
    if (!w.accepted) ++out->rejected;
    // the extreme member of the class has the same judgement pattern
    if (c.extreme_counts != c.first_counts) {
      eval::Trace et = execution_trace(m, c.extreme, in.ref_consts, in.record->owners);
      eval::Result er;
      if (eval::replay(p, et, in.replay_options, &er).ok()) {
        std::vector<Judged> ej = judge(et, er);
        std::set<std::string> a, b;
        for (const Judged& j : w.judged) a.insert(std::string(j.actor) + " " + j.key + " " + eval::occ_status_text(j.status));
        for (const Judged& j : ej) b.insert(std::string(j.actor) + " " + j.key + " " + eval::occ_status_text(j.status));
        w.extreme_note.push_back(std::string("the member with the largest counts (") + std::to_string(c.extreme.size()) + " steps) is judged " + (a == b ? "with the same statuses" : "with other statuses: the class is not uniform") +
                                 "; its occurrences: " + std::to_string(ej.size()));
      }
    }
    // a retry exhausted to its bound is a time-based transition (SPEC 1.8): its confirmation is timeout-exercised
    for (size_t ai = 0; ai < m.actors.size(); ++ai) {
      std::set<int64_t> bounds;
      for (const Instr& x : m.actors[ai].prog)
        if (x.op == Ins::IfBelow) bounds.insert(x.value);
      for (size_t ri = 0; ri < m.resources.size(); ++ri) {
        int64_t run = 0;
        for (const Step& st : c.first)
          if (st.actor == static_cast<int32_t>(ai) && st.ins == Ins::Use && st.res == static_cast<int32_t>(ri)) {
            const Instr& ui = m.actors[ai].prog[static_cast<size_t>(st.pc)];
            run = st.tuple.result == ui.not_ready_code ? run + 1 : 0;
            if (bounds.count(run) != 0) w.time_based = true;
          }
      }
    }
    // the observed-run-dependent orders: kept or not in this execution
    for (const EdgeInfo& e : out->edges) {
      if (e.constraint) continue;
      const int32_t a = pos_of(c.first, e.order.from_actor, e.order.from_index), b = pos_of(c.first, e.order.to_actor, e.order.to_index);
      const int32_t aa = pos_of(out->anchor, e.order.from_actor, e.order.from_index), bb = pos_of(out->anchor, e.order.to_actor, e.order.to_index);
      bool kept = a >= 0 && b >= 0 && a < b && aa >= 0 && bb >= 0;
      if (kept) {
        const Tuple &x1 = c.first[static_cast<size_t>(a)].tuple, &x2 = c.first[static_cast<size_t>(b)].tuple, &y1 = out->anchor[static_cast<size_t>(aa)].tuple,
                    &y2 = out->anchor[static_cast<size_t>(bb)].tuple;
        kept = x1.op == y1.op && x1.key == y1.key && x2.op == y2.op && x2.key == y2.key && x1.has_result == y1.has_result && x2.has_result == y2.has_result &&
               (!x2.has_result || x2.result == y2.result) && (!x1.has_result || x1.result == y1.result);
      }
      w.orders.push_back(std::string(kept ? "kept" : "not kept") + ": " + e.text);
    }
    // hold plans for a new candidate, and what each forces in the model
    if (w.accepted && w.in_scope && !w.observed && in.outcomes == OutcomeMode::Rederive && in.edges == EdgeMode::Variable) {
      auto check = [&](const Plan& pl, const std::string& name, const std::string& scope, PlanResult* res) -> Status {
        res->name = name;
        res->scope = scope;
        res->plan = pl;
        ExploreOptions po;
        po.max_states = in.max_states;
        po.constraints = constraints_of(out->edges, false);
        po.plan = &res->plan;
        ExploreResult pr;
        Status st = explore(p, m, po, &pr);
        if (!st.ok()) return st;
        res->checked = !pr.cap_reached;
        bool all_same = true;
        std::map<std::string, bool> forced;
        for (const std::string& seg : c.segments) forced[seg] = true;
        std::map<std::string, std::set<std::string>> gate_out;  // gate -> its outcomes in completed executions
        for (const ClassInfo& pc : pr.classes) {
          if (pc.stall) res->stall = true;
          if (pc.cut || pc.stall) continue;
          std::vector<std::string> segs;
          bool used = true;  // every gate is released or never reached
          for (const std::string& seg : pc.segments) {
            if (seg.compare(0, 5, "plan:") != 0) {
              segs.push_back(seg);
              continue;
            }
            size_t pos = 5;
            while (pos < seg.size()) {
              const size_t sp = seg.find(' ', pos + 1);
              const std::string item = seg.substr(pos + 1, (sp == std::string::npos ? seg.size() : sp) - pos - 1);
              const size_t eq = item.find('=');
              if (eq != std::string::npos) gate_out[item.substr(0, eq)].insert(item.substr(eq + 1));
              pos = sp == std::string::npos ? seg.size() : sp;
            }
          }
          (void)used;
          res->realized.push_back(pc.key);
          all_same = all_same && segs == c.segments;
          for (auto& kv : forced) kv.second = kv.second && std::find(segs.begin(), segs.end(), kv.first) != segs.end();
        }
        bool some_gate_never_released = false;
        for (const ControlPoint& cp : res->plan.points) {
          if (cp.release) continue;
          const std::set<std::string>& outs = gate_out[cp.label];
          std::string t;
          for (const std::string& x : outs) t += (t.empty() ? "" : ", ") + x;
          res->gates.push_back(cp.label + " in the completed executions: " + (t.empty() ? std::string("none") : t));
          some_gate_never_released = some_gate_never_released || outs.count("released") == 0;
        }
        res->never = res->realized.empty() || some_gate_never_released;
        res->forces = all_same && !res->never;
        for (const auto& kv : forced) (kv.second && !res->never ? res->forced : res->free).push_back(kv.first);
        return Status::success();
      };
      std::vector<std::string> whats, prefixes;
      const Plan full = synthesize(in, c.first, -1, &whats, &prefixes);
      w.plan_notes = full.notes;
      const std::string stem = "W" + std::to_string(w.id);
      if (full.any) {
        PlanResult pr;
        s = check(full, stem, "every order of the candidate", &pr);
        if (!s.ok()) return s;
        w.plans.push_back(pr);
      }
      const bool full_ok = !w.plans.empty() && w.plans[0].forces;
      if (!full_ok && whats.size() > 1)
        for (size_t k = 0; k < whats.size(); ++k) {
          const Plan kp = synthesize(in, c.first, static_cast<int32_t>(k), nullptr, nullptr);
          if (!kp.any) continue;
          PlanResult pr;
          s = check(kp, stem + "-" + std::to_string(k + 1), "one key only: " + whats[k], &pr);
          if (!s.ok()) return s;
          // kept when the key's part of the candidate is reached under it (forced or not; the check says which)
          std::string key_seg;
          for (const std::string& seg : c.segments)
            if (seg.compare(0, prefixes[k].size(), prefixes[k]) == 0) key_seg = seg;
          bool reached = false;
          for (const std::string& rk : pr.realized) reached = reached || (!key_seg.empty() && rk.find(key_seg) != std::string::npos);
          if (!pr.never && reached) w.plans.push_back(pr);
        }
    }
    out->witnesses.push_back(std::move(w));
  }
  // the same control points in two plans: the later names the earlier
  std::vector<std::pair<std::string, std::string>> seen;
  for (Witness& w : out->witnesses)
    for (PlanResult& pr : w.plans) {
      std::string sig;
      for (const ControlPoint& cp : pr.plan.points) sig += (cp.release ? "R " : "G ") + cp.site + "|" + cp.caller + "|" + cp.name_text + "|" + std::to_string(cp.actor) + ";";
      for (const auto& kv : seen)
        if (kv.first == sig) {
          pr.same_as = kv.second;
          break;
        }
      if (pr.same_as.empty()) seen.push_back({sig, pr.name});
    }
  return Status::success();
}

std::string report_text(const PredictInput& in, const PredictReport& r) {
  const Model& m = *in.model;
  std::string o;
  o += "mode: observed-run-dependent orders " + std::string(in.edges == EdgeMode::Variable ? "variable (re-derived per explored order)" : "FIXED as constraints (ablation)") + "; results " +
       (in.outcomes == OutcomeMode::Rederive ? "computed from the state at each step" : "KEPT from the record with those orders dropped (ablation)") + "\n";
  o += "model files:";
  for (const std::string& f : m.files) o += " " + f;
  o += "\n\nbounds (no reference fixes them; what they leave unexplored)\n";
  for (const std::string& b : r.bounds) o += "  " + b + "\n";
  o += "\nobservation judgement of the record (kz replay)\n";
  o += judged_list(r.observed, false);
  o += "\nrecord cone steps per actor\n";
  for (size_t i = 0; i < m.actors.size() && i < r.record_tuples.size(); ++i) {
    o += "  " + m.actors[i].name + " (record " + m.actors[i].record + "):";
    for (const Tuple& t : r.record_tuples[i]) o += " " + tuple_text(t) + ";";
    o += "\n";
  }
  o += "outside the cone (not modelled, with the reason)\n";
  for (const std::string& s : r.outside) o += "  " + s + "\n";
  o += "\norders between cone steps of different actors\n";
  for (const EdgeInfo& e : r.edges)
    o += std::string("  ") + (e.constraint ? "CONSTRAINT (kept in every order) " : "VARIABLE (re-derived per order) ") + e.text + "  [" + m.actors[static_cast<size_t>(e.order.from_actor)].name + " step " +
         std::to_string(e.order.from_index) + " -> " + m.actors[static_cast<size_t>(e.order.to_actor)].name + " step " + std::to_string(e.order.to_index) + "]\n";
  o += "\nanchor: " + r.anchor_note + "\n";
  if (!r.anchored) {
    o += "no prediction: the model is not anchored in this record\n";
    return o;
  }
  o += "  class " + r.anchor_key + "\n";
  o += "  model replay of the anchor:\n" + judged_list(r.anchor_judged, false);
  o += "\nsearch: states " + std::to_string(r.search.states) + ", transitions " + std::to_string(r.search.transitions) + ", terminal states " + std::to_string(r.search.terminals) +
       (r.search.cap_reached ? ", STATE CAP REACHED: the search is incomplete" : ", complete within the bounds above") + "\n";
  o += "  classes " + std::to_string(r.search.classes.size()) + ": scope-cut " + std::to_string(r.cut_classes) + " (" + std::to_string(r.cut_terminals) + " terminal states; no default-scope witness), candidates " +
       std::to_string(r.witnesses.size()) + ", rejected " + std::to_string(r.rejected) + "\n";
  o += "  (candidate counts are not results: each candidate is a MODEL_CANDIDATE until a controlled run confirms it)\n";
  for (const Witness& w : r.witnesses) {
    o += "\nW" + std::to_string(w.id) + (w.observed ? " [the observed class]" : " [prediction candidate]") + (w.accepted ? "" : " [REJECTED]") + (w.in_scope ? "" : " [not a default-scope witness]") +
         (w.time_based ? " [retry exhaustion: a time-based transition; a confirmation is timeout-exercised, kept apart (CF-04, T-16)]" : "") + "\n";
    if (!w.accepted) o += "  rejected: " + w.reject + "\n";
    for (const std::string& seg : w.cls.segments) o += "  class " + seg + "\n";
    o += "  counts min";
    for (uint32_t v : w.cls.min_counts) o += " " + std::to_string(v);
    o += " / max";
    for (uint32_t v : w.cls.max_counts) o += " " + std::to_string(v);
    o += " (" + std::to_string(w.cls.terminals) + " terminal states)\n";
    o += "  order (the representative: the execution of the class with the smallest counts):\n";
    o += order_text(m, w.cls.first);
    o += "  judgement (the replay of this order with the record's readiness specification):\n";
    o += judged_list(w.judged, true);
    for (const std::string& x : w.extreme_note) o += "  " + x + "\n";
    for (const std::string& x : w.orders) o += "  " + x + "\n";
    for (const PlanResult& pr : w.plans) {
      o += "  hold plan " + pr.name + ".kzh (" + pr.scope + ")" + (pr.same_as.empty() ? std::string() : " -- the same control points as " + pr.same_as + ".kzh") + "\n";
      for (const ControlPoint& cp : pr.plan.points)
        o += "    " + std::string(cp.release ? "release " : "gate ") + cp.label + " " + cp.site + (cp.caller.empty() ? "" : " caller-in " + cp.caller) +
             (cp.name_text.empty() ? "" : " name " + cp.name_role + " \"" + cp.name_text + "\"") + ": " + cp.why + "\n";
      o += "    plan check in the model" + std::string(pr.checked ? "" : " (INCOMPLETE: state cap)") + ": " + plan_verdict(pr) + "\n";
      for (const std::string& g : pr.gates) o += "      " + g + "\n";
      for (const std::string& seg : pr.forced) o += "      forced: " + seg + "\n";
      for (const std::string& seg : pr.free) o += "      not forced: " + seg + "\n";
    }
    if (w.plans.empty() && !w.observed && w.accepted && w.in_scope) o += "  hold plan: none\n";
    for (const std::string& x : w.plan_notes) o += "    note: " + x + "\n";
  }
  return o;
}

std::string plan_text(const PredictInput& in, const Witness& w, size_t k, const std::string& record_name) {
  const Model& m = *in.model;
  const PlanResult& pr = w.plans[k];
  std::string o;
  auto comment = [&o](const std::string& block) {
    size_t pos = 0;
    while (pos < block.size()) {
      const size_t nl = block.find('\n', pos);
      o += "#" + block.substr(pos, (nl == std::string::npos ? block.size() : nl) - pos) + "\n";
      pos = nl == std::string::npos ? block.size() : nl + 1;
    }
  };
  o += "# " + pr.name + ".kzh -- hold plan of a prediction candidate of kz predict (format kzp-hold 1); it enforces " + pr.scope + ".\n";
  o += "# Record: " + record_name + ".  Model:";
  for (const std::string& f : m.files) o += " " + f;
  o += "\n# The candidate W" + std::to_string(w.id) + " (a MODEL_CANDIDATE, not a result; SPEC 4.9 PR-02, 4.13 CF-01-CF-04):\n";
  for (const std::string& seg : w.cls.segments) o += "#   " + seg + "\n";
  o += "# Expected judgement of the candidate's order (the replay of its model trace):\n";
  comment(judged_list(w.judged, false));
  o += "# The candidate's order (the representative of the class):\n";
  comment(order_text(m, w.cls.first));
  if (w.time_based) o += "# It contains a retry exhausted to its bound: a time-based transition; a run that realizes it is timeout-exercised (CF-04, T-16).\n";
  o += "# Control points (each: the model step it stands for and its citations):\n";
  for (const ControlPoint& cp : pr.plan.points) o += "#   " + cp.label + ": " + cp.why + "\n";
  o += "# Plan check in the model: " + plan_verdict(pr) + ".\n";
  for (const std::string& g : pr.gates) o += "#   " + g + "\n";
  for (const std::string& seg : pr.forced) o += "#   forced: " + seg + "\n";
  for (const std::string& seg : pr.free) o += "#   not forced: " + seg + "\n";
  for (const std::string& x : w.plan_notes) o += "#   note: " + x + "\n";
  o += "# The gates hold the actors named in their labels; which label a run of this plan carries (a decided hold or a researcher\n";
  o += "# intervention) follows from which actor is held and is decided outside this tool.\n";
  o += "kzp-hold 1\n";
  auto cond_of = [](const ControlPoint& cp) {
    std::string cond;
    if (!cp.name_text.empty()) cond += " when name " + cp.name_role + " \"" + cp.name_text + "\"";
    if (!cp.caller.empty()) cond += " when caller-in " + cp.caller;
    return cond;
  };
  for (const ControlPoint& cp : pr.plan.points)
    if (!cp.release) o += "gate " + cp.label + " site=" + cp.site + cond_of(cp) + " occurrence first\n";
  for (const ControlPoint& cp : pr.plan.points)
    if (cp.release) o += "release " + cp.label + " site=" + cp.site + " releases " + cp.releases + cond_of(cp) + " occurrence first\n";
  return o;
}

}  // namespace kz::predict

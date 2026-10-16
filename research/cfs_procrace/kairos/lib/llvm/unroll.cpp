// unroll.cpp -- see unroll.hpp.
#include "unroll.hpp"

#include <algorithm>
#include <queue>
#include <tuple>

namespace kzs {

namespace {

bool is_const(ExprRef e) { return e != nullptr && e->op == KZ_EXPR_OP_CONST && e->a == nullptr && e->b == nullptr; }
bool is_func(ExprRef e) { return e != nullptr && e->op == KZ_EXPR_OP_LOAD && e->ref == KZ_EXPR_REF_FUNC; }
bool is_string(ExprRef e) { return e != nullptr && e->op == KZ_EXPR_OP_LOAD && e->ref == KZ_EXPR_REF_STRING; }
bool is_object_addr(ExprRef e) {
  return e != nullptr && e->op == KZ_EXPR_OP_ADDR && e->a != nullptr && e->a->op == KZ_EXPR_OP_LOAD && e->a->ref == KZ_EXPR_REF_GLOBAL;
}
// A pointer that is known not to be null.
bool is_nonnull(ExprRef e) { return is_func(e) || is_string(e) || is_object_addr(e); }

}  // namespace

int truth(ExprRef e) {
  if (e == nullptr) return -1;
  if (is_const(e)) return e->value != 0 ? 1 : 0;
  if (is_nonnull(e)) return 1;
  return -1;
}

ExprRef peval(ExprPool* p, ExprRef e, const ElemLookup& lookup) {
  if (e == nullptr) return nullptr;
  switch (e->op) {
    case KZ_EXPR_OP_CONST: return e;
    case KZ_EXPR_OP_LOAD: {
      if (e->ref != KZ_EXPR_REF_ELEM) return e;
      const ElemValue v = lookup(e->name);
      if (!v.known) return e;
      switch (v.kind) {
        case KZ_IMAGE_KIND_INT: return p->constant(v.value);
        case KZ_IMAGE_KIND_NULLPTR: return p->constant(0);
        case KZ_IMAGE_KIND_FUNC: return p->load(KZ_EXPR_REF_FUNC, v.text);
        case KZ_IMAGE_KIND_STRING: return p->load(KZ_EXPR_REF_STRING, v.text);
        case KZ_IMAGE_KIND_ADDR: return p->unary(KZ_EXPR_OP_ADDR, p->load(KZ_EXPR_REF_GLOBAL, v.text));
        default: return e;
      }
    }
    case KZ_EXPR_OP_NOT: {
      ExprRef a = peval(p, e->a, lookup);
      const int t = truth(a);
      if (t >= 0) return p->constant(t == 1 ? 0 : 1);
      return a == e->a ? e : p->unary(KZ_EXPR_OP_NOT, a);
    }
    case KZ_EXPR_OP_AND:
    case KZ_EXPR_OP_OR: {
      ExprRef a = peval(p, e->a, lookup), b = peval(p, e->b, lookup);
      const int ta = truth(a), tb = truth(b);
      const bool is_and = e->op == KZ_EXPR_OP_AND;
      if (is_and) {
        if (ta == 0 || tb == 0) return p->constant(0);
        if (ta == 1 && tb == 1) return p->constant(1);
        if (ta == 1) return b;
        if (tb == 1) return a;
      } else {
        if (ta == 1 || tb == 1) return p->constant(1);
        if (ta == 0 && tb == 0) return p->constant(0);
        if (ta == 0) return b;
        if (tb == 0) return a;
      }
      return (a == e->a && b == e->b) ? e : p->binary(e->op, a, b);
    }
    case KZ_EXPR_OP_EQ:
    case KZ_EXPR_OP_NE: {
      ExprRef a = peval(p, e->a, lookup), b = peval(p, e->b, lookup);
      const bool eq = e->op == KZ_EXPR_OP_EQ;
      int same = -1;  // 1 equal, 0 different, -1 unknown
      if (is_const(a) && is_const(b)) same = a->value == b->value ? 1 : 0;
      else if ((is_nonnull(a) && is_const(b) && b->value == 0) || (is_nonnull(b) && is_const(a) && a->value == 0)) same = 0;
      else if (is_func(a) && is_func(b)) same = a->name == b->name ? 1 : 0;
      else if (is_object_addr(a) && is_object_addr(b)) same = a->a->name == b->a->name ? 1 : 0;
      if (same >= 0) return p->constant((same == 1) == eq ? 1 : 0);
      return (a == e->a && b == e->b) ? e : p->binary(e->op, a, b);
    }
    default: {
      ExprRef a = peval(p, e->a, lookup), b = peval(p, e->b, lookup);
      if (a == e->a && b == e->b) return e;
      switch (e->op) {
        case KZ_EXPR_OP_LT:
        case KZ_EXPR_OP_LE:
        case KZ_EXPR_OP_GT:
        case KZ_EXPR_OP_GE: return p->binary(e->op, a, b);  // folds two constants
        default: return p->make(e->op, e->ref, e->value, e->name, a, b);
      }
    }
  }
}

// ---- spawn parameters ------------------------------------------------------------------------------

void find_spawn_params(Model* mp, const ProgramGraph& g) {
  Model& m = *mp;
  std::map<std::string, std::vector<std::string>> succ;
  {
    std::set<std::pair<std::string, std::string>> uniq;
    for (const FlowEdge& e : m.flow)
      if (uniq.insert({e.src, e.dst}).second) succ[e.src].push_back(e.dst);
    for (auto& kv : succ) std::sort(kv.second.begin(), kv.second.end());
  }
  auto reach_from = [&](const std::string& start) {
    std::set<std::string> seen{start};
    std::queue<std::string> q;
    q.push(start);
    while (!q.empty()) {
      const std::string n = q.front();
      q.pop();
      auto it = succ.find(n);
      if (it == succ.end()) continue;
      for (const std::string& d : it->second)
        if (seen.insert(d).second) q.push(d);
    }
    return seen;
  };
  auto funcs_from = [&](const std::string& root) {
    std::set<std::string> seen{root};
    std::queue<std::string> q;
    q.push(root);
    while (!q.empty()) {
      const std::string f = q.front();
      q.pop();
      auto it = g.cg.find(f);
      if (it == g.cg.end()) continue;
      for (const std::string& h : it->second)
        if (seen.insert(h).second) q.push(h);
    }
    return seen;
  };
  // the resolved entries of every spawn call
  std::map<std::string, std::set<std::string>> entries;
  for (const TargetFact& t : m.targets)
    if (t.kind == KZ_CALL_TARGET_KIND_SPAWN_ENTRY && !t.fn_key.empty()) entries[t.call_key].insert(t.fn_key);
  std::vector<std::string> spawns = m.spawn_calls;
  std::sort(spawns.begin(), spawns.end());
  for (const std::string& sk : spawns) {
    auto cit = g.call_caller.find(sk);
    if (cit == g.call_caller.end()) continue;
    const std::string& fkey = cit->second;
    auto fit = m.funcs.find(fkey);
    if (fit == m.funcs.end()) continue;
    const std::vector<std::string>& entry_nodes = m.spawn_entry_nodes[sk];
    for (uint32_t k = 0; k < fit->second.nparams; ++k) {
      const std::string pnode = "P:" + fkey + "#" + std::to_string(k);
      if (succ.count(pnode) == 0 && std::find(entry_nodes.begin(), entry_nodes.end(), pnode) == entry_nodes.end()) continue;
      const std::set<std::string> reach = reach_from(pnode);
      for (const std::string& en : entry_nodes)
        if (reach.count(en) != 0) {
          SpawnParamFact sp;
          sp.func_key = fkey;
          sp.param = k;
          sp.spawn_call_key = sk;
          sp.kind = KZ_SPAWN_PARAM_KIND_DIRECT;
          sp.via = en;
          m.spawn_params.push_back(sp);
          break;
        }
      for (const std::string& entry : entries[sk]) {
        const std::set<std::string> fs = funcs_from(entry);
        for (const IndirectCall& ic : m.indirect_calls) {
          auto ccit = g.call_caller.find(ic.call_key);
          if (ccit == g.call_caller.end() || fs.count(ccit->second) == 0 || reach.count(ic.node) == 0) continue;
          SpawnParamFact sp;
          sp.func_key = fkey;
          sp.param = k;
          sp.spawn_call_key = sk;
          sp.entry_key = entry;
          sp.call_key = ic.call_key;
          sp.kind = KZ_SPAWN_PARAM_KIND_VIA_ENTRY;
          sp.via = ic.node;
          m.spawn_params.push_back(sp);
        }
      }
    }
  }
  std::sort(m.spawn_params.begin(), m.spawn_params.end(), [](const SpawnParamFact& a, const SpawnParamFact& b) {
    return std::tie(a.func_key, a.param, a.spawn_call_key, a.kind, a.entry_key, a.call_key) < std::tie(b.func_key, b.param, b.spawn_call_key, b.kind, b.entry_key, b.call_key);
  });
  m.spawn_params.erase(std::unique(m.spawn_params.begin(), m.spawn_params.end(),
                                   [](const SpawnParamFact& a, const SpawnParamFact& b) {
                                     return std::tie(a.func_key, a.param, a.spawn_call_key, a.kind, a.entry_key, a.call_key) ==
                                            std::tie(b.func_key, b.param, b.spawn_call_key, b.kind, b.entry_key, b.call_key);
                                   }),
                       m.spawn_params.end());
}

// ---- unrolled iterations of table walks --------------------------------------------------------------

namespace {

ElemValue value_of(const ImageFact& f) {
  ElemValue v;
  v.pos = f.pos;
  switch (f.kind) {
    case KZ_IMAGE_KIND_INT:
    case KZ_IMAGE_KIND_NULLPTR:
    case KZ_IMAGE_KIND_FUNC:
    case KZ_IMAGE_KIND_ADDR:
    case KZ_IMAGE_KIND_STRING:
      v.known = true;
      v.kind = f.kind;
      v.value = f.value;
      v.text = f.text;
      v.fn_key = f.fn_key;
      break;
    default: break;
  }
  return v;
}

}  // namespace

void unroll_table_walks(Model* mp, const std::map<std::string, std::vector<ImageFact>>& entries) {
  Model& m = *mp;
  std::map<std::string, std::vector<const WalkUse*>> uses;
  for (const WalkUse& u : m.walk_uses) uses[u.loop_key].push_back(&u);
  std::map<std::string, std::set<uint32_t>> task_main;  // function key -> its task-main parameters
  for (const SpawnParamFact& sp : m.spawn_params) task_main[sp.func_key].insert(sp.param);
  std::map<std::string, const CallFact*> call_by_key;
  for (const CallFact& c : m.calls) call_by_key[c.pos.str() + "#" + std::to_string(c.ordinal)] = &c;

  for (const auto& kv : entries) {
    const std::string& loop_key = kv.first;
    for (size_t i = 0; i < kv.second.size(); ++i) {
      const ImageFact& entry = kv.second[i];
      // the element object, and a lookup of its fields
      const bool is_object = entry.kind == KZ_IMAGE_KIND_ADDR && entry.text.find('+') == std::string::npos;
      const std::vector<ImageFact>* img = nullptr;
      if (is_object) {
        auto it = m.object_images.find(entry.text);
        if (it != m.object_images.end()) img = &it->second;
      }
      ElemLookup lookup = [&](const std::string& field) {
        ElemValue v;
        if (field.empty()) {
          if (entry.kind == KZ_IMAGE_KIND_ADDR) {
            v.known = true;
            v.kind = KZ_IMAGE_KIND_ADDR;
            v.text = entry.text;
            v.pos = entry.pos;
          }
          return v;
        }
        if (img == nullptr) return v;
        for (const ImageFact& f : *img)
          if (f.path == "." + field) return value_of(f);
        return v;
      };
      IterFact el;
      el.loop_key = loop_key;
      el.iter = static_cast<uint32_t>(i);
      el.kind = KZ_ITER_KIND_ELEMENT;
      el.status = KZ_ITER_STATUS_TAKEN;
      el.value_kind = entry.kind;
      el.value = entry.value;
      el.text = entry.text;
      el.pos = entry.pos;
      if (is_object) {
        auto pit = m.object_pos.find(entry.text);
        if (pit != m.object_pos.end()) el.pos = pit->second;
      }
      m.iters.push_back(el);
      for (const WalkUse* u : uses[loop_key]) {
        IterFact c;
        c.loop_key = loop_key;
        c.iter = static_cast<uint32_t>(i);
        c.kind = KZ_ITER_KIND_CALL;
        c.call_key = u->call_key;
        ExprRef g = peval(&m.exprs, u->guard, lookup);
        const int t = g == nullptr ? 1 : truth(g);
        c.status = t == 1 ? KZ_ITER_STATUS_TAKEN : t == 0 ? KZ_ITER_STATUS_SKIPPED : KZ_ITER_STATUS_RESIDUAL;
        if (c.status == KZ_ITER_STATUS_RESIDUAL) c.guard = g;
        m.iters.push_back(c);
        if (c.status == KZ_ITER_STATUS_SKIPPED) continue;
        const CallFact* call = call_by_key.count(u->call_key) != 0 ? call_by_key[u->call_key] : nullptr;
        if (!u->callee_field.empty()) {
          const ElemValue v = lookup(u->callee_field);
          IterFact ce = c;
          ce.kind = KZ_ITER_KIND_CALLEE;
          ce.guard = nullptr;
          ce.value_kind = v.known ? v.kind : static_cast<uint8_t>(KZ_IMAGE_KIND_NONCONST);
          ce.text = v.known ? v.text : "field " + u->callee_field + " not in the image";
          ce.fn_key = v.fn_key;
          ce.pos = v.pos;
          m.iters.push_back(ce);
        }
        for (const auto& arg : u->args) {
          IterFact a = c;
          a.kind = KZ_ITER_KIND_ARG;
          a.guard = nullptr;
          a.arg_index = arg.first;
          ExprRef x = arg.second;
          if (x != nullptr && x->op == KZ_EXPR_OP_LOAD && x->ref == KZ_EXPR_REF_ELEM && !x->name.empty()) {
            const ElemValue v = lookup(x->name);
            a.value_kind = v.known ? v.kind : static_cast<uint8_t>(KZ_IMAGE_KIND_NONCONST);
            a.value = v.value;
            a.text = v.known ? v.text : "field " + x->name + " not in the image";
            a.fn_key = v.fn_key;
            a.pos = v.pos;
          } else {
            ExprRef y = peval(&m.exprs, x, lookup);
            if (is_const(y)) {
              a.value_kind = KZ_IMAGE_KIND_INT;
              a.value = y->value;
            } else {
              a.value_kind = KZ_IMAGE_KIND_NONCONST;
            }
            a.text = expr_text(y);
          }
          if (call != nullptr) {
            auto sit = m.spawn_entry_arg.find(u->call_key);
            if (sit != m.spawn_entry_arg.end() && sit->second == arg.first) a.spawn = KZ_ITER_SPAWN_ENTRY;
            auto tit = task_main.find(call->callee_key);
            if (tit != task_main.end() && tit->second.count(arg.first) != 0) a.spawn = KZ_ITER_SPAWN_TASK_MAIN;
          }
          m.iters.push_back(a);
        }
      }
    }
  }
}

// ---- the rule of each loop (SPEC 4.3, certificate view) --------------------------------------------------

namespace {

struct Loc {
  std::string type, field;
  bool operator<(const Loc& o) const { return std::tie(type, field) < std::tie(o.type, o.field); }
};

// The fields and globals an expression reads.  The base of a cursor over an array of known size
// is not a read: an array in an expression is its (constant) address.
void reads_of(ExprRef e, std::set<Loc>* out) {
  if (e == nullptr) return;
  if (e->op == KZ_EXPR_OP_CURSOR) {
    if (e->value <= 0) reads_of(e->a, out);
    reads_of(e->b, out);
    return;
  }
  if (e->op == KZ_EXPR_OP_LOAD) {
    if (e->ref == KZ_EXPR_REF_FIELD) {
      const size_t dot = e->name.rfind('.');
      if (dot != std::string::npos) out->insert(Loc{e->name.substr(0, dot), e->name.substr(dot + 1)});
    } else if (e->ref == KZ_EXPR_REF_GLOBAL) {
      out->insert(Loc{"<global>", e->name});
    }
    return;
  }
  reads_of(e->a, out);
  reads_of(e->b, out);
}

bool hits(const WriteEvent& w, const Loc& l) {
  if (w.kind == KZ_FWRITE_KIND_UNTYPED) return false;  // handled by the A-TYPE status
  if (w.type != l.type) return false;
  return w.field == "*" || w.field == l.field;
}

}  // namespace

void decide_loop_rules(Model* mp, const ExtractSpec& spec, const ProgramGraph& g) {
  Model& m = *mp;
  Model::Resolved& R = m.resolved;
  std::map<std::string, std::string> parent;
  for (const LoopFact& l : m.loops) parent[l.key] = l.parent_key;
  auto within = [&](std::string key, const std::string& outer) {
    for (int guard = 0; !key.empty() && guard < 1000; ++guard) {
      if (key == outer) return true;
      auto it = parent.find(key);
      if (it == parent.end()) return false;
      key = it->second;
    }
    return false;
  };
  std::map<std::string, std::vector<const WriteEvent*>> writes_of;
  for (const WriteEvent& w : m.write_events) writes_of[w.func_key].push_back(&w);
  std::map<std::string, std::set<std::string>> unresolved_in;  // function -> unresolved indirect calls it makes
  for (const std::string& ck : g.unresolved_calls) {
    auto it = g.call_caller.find(ck);
    if (it != g.call_caller.end()) unresolved_in[it->second].insert(ck);
  }
  std::map<std::pair<std::string, std::string>, uint8_t> target_status;
  for (const LocFact& l : m.locs) target_status[{l.loc_type, l.loc_field}] = l.status;

  for (LoopFact& l : m.loops) {
    if (l.delay_direct) l.lflags |= KZ_LOOP_FLAG_DELAY_DIRECT;
    const std::string trip = std::to_string(l.trip);
    switch (l.kind) {
      case KZ_LOOP_KIND_TABLE_WALK:
        if (l.trip_status == KZ_TRIP_STATUS_EXACT) {
          l.cert = KZ_LOOP_CERT_UNROLL;
          l.cert_note = "unrolled: " + trip + " iterations, one per element of " + l.table + " in its static-initializer image";
        } else {
          l.cert = KZ_LOOP_CERT_STAR;
          l.cert_note = "star-closed: the table walk was not resolved" + (l.trip_note.empty() ? std::string() : " (" + l.trip_note + ")");
        }
        break;
      case KZ_LOOP_KIND_CONST_TRIP:
      case KZ_LOOP_KIND_COUNT_BOUNDED:
        if (l.trip_status == KZ_TRIP_STATUS_EXACT || l.trip_status == KZ_TRIP_STATUS_UPPER_BOUND) {
          l.cert = KZ_LOOP_CERT_UNROLL;
          l.cert_note = "unrolled: " + trip + " iterations from the constant bound" +
                        std::string(l.trip_status == KZ_TRIP_STATUS_UPPER_BOUND ? ", with early exits" : "") +
                        std::string(l.kind == KZ_LOOP_KIND_COUNT_BOUNDED ? "; delay-paced: the failing outcome of the last attempt is a time-based transition" : "");
        } else {
          l.cert = KZ_LOOP_CERT_STAR;
          l.cert_note = "star-closed: no exact trip count";
        }
        break;
      case KZ_LOOP_KIND_POLL: {
        // L1 (ii): the reads of the test in one critical section of one lock, or one atomic read
        if (l.lock_kind == KZ_LOCK_KIND_ONE_REGION || l.lock_kind == KZ_LOCK_KIND_ATOMIC_READ) l.lflags |= KZ_LOOP_FLAG_L1_READ;
        // L1 (i): the body, with its callees, writes no relevant location
        std::set<Loc> relevant;
        reads_of(l.pred != nullptr ? l.pred : l.cond, &relevant);
        for (const TargetField& t : spec.fields) relevant.insert(Loc{t.field.type, t.field.field});
        std::set<std::string> roots;  // the callees of the calls in the body
        std::vector<std::string> written, incomplete;
        bool untyped_reach = false;
        for (const WriteEvent& w : m.write_events) {
          if (w.loop_key.empty() || !within(w.loop_key, l.key)) continue;
          if (w.kind == KZ_FWRITE_KIND_UNTYPED) untyped_reach = true;
          for (const Loc& r : relevant)
            if (hits(w, r)) written.push_back(r.type + "." + r.field + " at " + w.pos.str());
        }
        for (const CallFact& c : m.calls) {
          if (c.loop_key.empty() || !within(c.loop_key, l.key)) continue;
          const std::string ck = c.pos.str() + "#" + std::to_string(c.ordinal);
          if (!c.callee_key.empty()) roots.insert(c.callee_key);
          auto it = g.indirect_targets.find(ck);
          if (it != g.indirect_targets.end()) roots.insert(it->second.begin(), it->second.end());
          if (g.unresolved_calls.count(ck) != 0) incomplete.push_back("an indirect call at " + c.pos.str() + " has an unresolved target");
        }
        std::set<std::string> fs(roots.begin(), roots.end());
        std::queue<std::string> q;
        for (const std::string& f : roots) q.push(f);
        while (!q.empty()) {
          const std::string f = q.front();
          q.pop();
          auto it = g.cg.find(f);
          if (it == g.cg.end()) continue;
          for (const std::string& h : it->second)
            if (fs.insert(h).second) q.push(h);
        }
        for (const std::string& f : fs) {
          auto uit = unresolved_in.find(f);
          if (uit != unresolved_in.end()) incomplete.push_back("the callee " + f + " makes an indirect call with an unresolved target");
          auto wit = writes_of.find(f);
          if (wit == writes_of.end()) continue;
          for (const WriteEvent* w : wit->second) {
            if (w->kind == KZ_FWRITE_KIND_UNTYPED) untyped_reach = true;
            for (const Loc& r : relevant)
              if (hits(*w, r)) written.push_back(r.type + "." + r.field + " by " + f + " at " + w->pos.str());
          }
        }
        // Writes through pointers, casts and untyped views are excluded only for a location whose
        // A-TYPE listing is clean: the extraction targets with a complete writer set.  A relevant
        // location that is not a target has no listing, so its writer set is not closed; a
        // DISABLED target is not closed either.
        for (const Loc& r : relevant) {
          auto st = target_status.find({r.type, r.field});
          if (st == target_status.end())
            incomplete.push_back(r.type + "." + r.field + " is not an extraction target: no A-TYPE listing, writes through pointers are not excluded");
          else if (st->second != KZ_LOC_STATUS_COMPLETE)
            incomplete.push_back(r.type + "." + r.field + " is DISABLED" + std::string(untyped_reach ? " and an untyped write is reachable" : ""));
        }
        std::sort(written.begin(), written.end());
        written.erase(std::unique(written.begin(), written.end()), written.end());
        std::sort(incomplete.begin(), incomplete.end());
        incomplete.erase(std::unique(incomplete.begin(), incomplete.end()), incomplete.end());
        // a definite write decides (i) whatever else is open
        if (written.empty() && !incomplete.empty()) l.lflags |= KZ_LOOP_FLAG_MOD_INCOMPLETE;
        else if (written.empty()) l.lflags |= KZ_LOOP_FLAG_L1_MOD;
        std::string why;
        if (l.pred_form == KZ_PRED_FORM_NONE) why = "no recognised predicate (" + (l.pred_note.empty() ? std::string("no condition exit") : l.pred_note) + ")";
        else if ((l.lflags & KZ_LOOP_FLAG_L1_READ) == 0) why = "L1 (ii) fails: the test reads are not in one critical section of one lock, nor one atomic read";
        else if (!written.empty()) why = "L1 (i) fails: the body writes " + written.front() + (written.size() > 1 ? " (and " + std::to_string(written.size() - 1) + " more)" : "");
        else if (!incomplete.empty()) why = "L1 (i) undecided: " + incomplete.front() + (incomplete.size() > 1 ? " (and " + std::to_string(incomplete.size() - 1) + " more)" : "");
        if (why.empty()) {
          l.cert = KZ_LOOP_CERT_AWAIT;
          l.cert_note = "await(predicate) {success} {timeout}: " + l.pred_note + "; L1 (i) and (ii) hold on the extracted facts (" +
                        "relevant locations: " + std::to_string(relevant.size()) + ", functions called from the body: " + std::to_string(fs.size()) + "); lemma L1 itself is not decided here";
        } else {
          l.cert = KZ_LOOP_CERT_STAR;
          l.cert_note = "star-closed, unmodelled-sync: " + why;
        }
        break;
      }
      default:
        l.cert = KZ_LOOP_CERT_STAR;
        l.cert_note = "star-closed: no exact trip count";
        break;
    }
    if (l.cert == KZ_LOOP_CERT_UNROLL) ++R.loops_unroll;
    else if (l.cert == KZ_LOOP_CERT_AWAIT) ++R.loops_await;
    else ++R.loops_star;
  }
}

}  // namespace kzs

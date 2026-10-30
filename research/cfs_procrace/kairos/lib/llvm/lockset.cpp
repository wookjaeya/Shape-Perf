// lockset.cpp -- see lockset.hpp.
#include "lockset.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <queue>
#include <tuple>

#include "p0.hpp"

namespace kzs {

const char* lock_rules_revision() { return "lock-set rules F5, completeness rules and L1 (i) rule revision 1 (2026-10-30)"; }

namespace {

using N = LockIdNode;

std::string key_of(const CallFact& c) { return c.pos.str() + "#" + std::to_string(c.ordinal); }

LockId mk(uint8_t k, int64_t v, const std::string& name, LockId a = nullptr, LockId b = nullptr) {
  auto n = std::make_shared<LockIdNode>();
  n->k = k;
  n->v = v;
  n->name = name;
  n->a = std::move(a);
  n->b = std::move(b);
  return n;
}

std::string field_part(const std::string& tf) {
  const size_t dot = tf.rfind('.');
  return dot == std::string::npos ? tf : tf.substr(dot + 1);
}

std::string lv_may(const LockId& lv) {
  if (!lv) return "";
  switch (lv->k) {
    case N::FIELD: return lv->name;
    case N::GLOBAL: return "<global>." + lv->name;
    case N::INDEX: return lv_may(lv->a);
    default: return "";
  }
}

bool has_kind(const LockId& id, uint8_t k) {
  if (!id) return false;
  if (id->k == k) return true;
  return has_kind(id->a, k) || has_kind(id->b, k);
}

// The access path of an identity `&lv` (ADDR): the root (a global object, or "" for a pointer or anything else) and the
// steps from it (a FIELD with its name, the record type that declares it and whether that is a union; an INDEX).
struct PathStep {
  bool index = false;
  std::string name, owner, rtype;  // FIELD: "Type.field", the declaring record type, the record type it designates
  bool union_member = false;
};
struct LockPath {
  bool global = false;
  std::string root, root_rtype;
  std::vector<PathStep> steps;
};
bool lock_path(const LockId& lv, LockPath* p) {
  if (!lv) return false;
  switch (lv->k) {
    case N::GLOBAL:
      p->global = true;
      p->root = lv->name;
      p->root_rtype = lv->rtype;
      return true;
    case N::FIELD: {
      if (!lock_path(lv->a, p)) return false;
      PathStep st;
      st.name = lv->name;
      st.owner = lv->owner;
      st.rtype = lv->rtype;
      st.union_member = lv->v == kLockFieldUnionMember;
      p->steps.push_back(st);
      return true;
    }
    case N::INDEX: {
      if (!lock_path(lv->a, p)) return false;
      PathStep st;
      st.index = true;
      p->steps.push_back(st);
      return true;
    }
    default:
      // a pointer (DEREF of a value) or anything else: the root is unknown, the steps start here
      p->global = false;
      p->root.clear();
      return true;
  }
}
// From step i of x and step j of y on (the same object), whether the two lvalues may share storage: a step past the end
// of one is a part of the other (the object itself, its first member: a prefix); two members of one union overlap;
// two different members of a structure do not; subscripts are not compared.
bool aligned_overlap(const std::vector<PathStep>& x, size_t i, const std::vector<PathStep>& y, size_t j) {
  for (; i < x.size() && j < y.size(); ++i, ++j) {
    if (x[i].index && y[j].index) continue;
    if (x[i].index != y[j].index) return true;
    if (x[i].name == y[j].name) continue;
    return x[i].union_member && y[j].union_member && x[i].owner == y[j].owner;
  }
  return true;
}
// A path through a pointer (root unknown) against another: its first member names the record type T the pointer
// points to; it may point to any object of type T on the other path (the root object, or the object a step designates
// or whose member a step names).  A pointer to a record type the other path does not name is taken as another object
// (the premise of these facts, static_facts §5h).
bool pointer_overlap(const LockPath& ptr, const LockPath& other) {
  size_t j = 0;
  while (j < ptr.steps.size() && ptr.steps[j].index) ++j;
  if (j == ptr.steps.size()) return true;  // the pointer itself, or its elements: any object
  const std::string& t = ptr.steps[j].owner;
  if (t.empty()) return true;
  std::vector<std::string> obj;  // the record type of the object after k steps of `other`
  obj.push_back(other.root_rtype);
  for (const PathStep& st : other.steps) obj.push_back(st.index ? obj.back() : st.rtype);
  for (size_t k = 0; k <= other.steps.size(); ++k) {
    const bool here = obj[k] == t || (k < other.steps.size() && !other.steps[k].index && other.steps[k].owner == t);
    if (here && aligned_overlap(ptr.steps, j, other.steps, k)) return true;
  }
  return false;
}
// Whether two identities may name one mutex, for the locks a release drops: the same text; an identity that is no
// address (a parameter, a loaded value: anything); the same field-based location (`may`); or overlapping access paths
// (the same global object with one path a prefix of the other or two members of one union; a pointer path aligned on the
// record type it names).  Two different global objects do not overlap.
bool may_equal(const LockId& x, const std::string& tx, const LockId& y, const std::string& ty) {
  if (tx == ty) return true;
  const std::string mx = lock_id_may(x), my = lock_id_may(y);
  if (mx.empty() || my.empty() || mx == my) return true;
  LockPath px, py;
  if (!lock_path(x->a, &px) || !lock_path(y->a, &py)) return true;
  if (px.global && py.global) return px.root == py.root && aligned_overlap(px.steps, 0, py.steps, 0);
  if (!px.global && pointer_overlap(px, py)) return true;
  if (!py.global && pointer_overlap(py, px)) return true;
  return false;
}

// Strongly connected components of a graph over its nodes, callees first (completion order of Tarjan).
std::vector<std::vector<std::string>> sccs_of(const std::vector<std::string>& nodes, const std::map<std::string, std::set<std::string>>& g) {
  std::vector<std::vector<std::string>> out;
  std::map<std::string, int> index, low;
  std::set<std::string> on;
  std::vector<std::string> stack;
  int counter = 0;
  struct Frame {
    std::string v;
    std::vector<std::string> succ;
    size_t i = 0;
  };
  for (const std::string& root : nodes) {
    if (index.count(root) != 0) continue;
    std::vector<Frame> frames;
    auto push = [&](const std::string& v) {
      index[v] = low[v] = counter++;
      stack.push_back(v);
      on.insert(v);
      Frame f;
      f.v = v;
      auto it = g.find(v);
      if (it != g.end()) f.succ.assign(it->second.begin(), it->second.end());
      frames.push_back(f);
    };
    push(root);
    while (!frames.empty()) {
      Frame& fr = frames.back();
      if (fr.i < fr.succ.size()) {
        const std::string w = fr.succ[fr.i++];
        if (index.count(w) == 0) push(w);
        else if (on.count(w) != 0) low[fr.v] = std::min(low[fr.v], index[w]);
        continue;
      }
      const std::string v = fr.v;
      frames.pop_back();
      if (!frames.empty()) low[frames.back().v] = std::min(low[frames.back().v], low[v]);
      if (low[v] == index[v]) {
        std::vector<std::string> scc;
        for (;;) {
          const std::string w = stack.back();
          stack.pop_back();
          on.erase(w);
          scc.push_back(w);
          if (w == v) break;
        }
        std::sort(scc.begin(), scc.end());
        out.push_back(scc);
      }
    }
  }
  return out;
}

// The program as the derivation reads it.
struct Prog {
  explicit Prog(const Model& model) : m(model) {}
  const Model& m;
  std::map<std::string, size_t> call_index;
  std::map<std::string, std::vector<std::string>> itargets;
  std::set<std::string> iunresolved;
  std::set<std::string> body;
  std::map<std::string, std::set<std::string>> cg;
  std::map<std::string, std::vector<size_t>> calls_of;
  std::set<std::string> spawn_entries;
  std::map<std::string, size_t> p0_by_name;

  void build() {
    for (size_t i = 0; i < p0_table().size(); ++i) p0_by_name[p0_table()[i].name] = i;
    body = m.processed_functions;
    for (size_t i = 0; i < m.calls.size(); ++i) {
      call_index[key_of(m.calls[i])] = i;
      calls_of[m.calls[i].caller_key].push_back(i);
    }
    for (const TargetFact& t : m.targets) {
      if (t.kind == KZ_CALL_TARGET_KIND_SPAWN_ENTRY) {
        if (!t.fn_key.empty()) spawn_entries.insert(t.fn_key);
        continue;
      }
      if (t.kind != KZ_CALL_TARGET_KIND_INDIRECT) continue;
      if (t.fn_key.empty()) iunresolved.insert(t.call_key);
      else itargets[t.call_key].push_back(t.fn_key);
    }
    for (auto& kv : itargets) {
      std::sort(kv.second.begin(), kv.second.end());
      kv.second.erase(std::unique(kv.second.begin(), kv.second.end()), kv.second.end());
    }
    for (size_t i = 0; i < m.calls.size(); ++i) {
      const CallFact& c = m.calls[i];
      for (const std::string& t : targets(i))
        if (body.count(t) != 0) cg[c.caller_key].insert(t);
    }
  }
  std::vector<std::string> targets(size_t ci) const {
    const CallFact& c = m.calls[ci];
    if (!c.callee_key.empty()) return {c.callee_key};
    auto it = itargets.find(key_of(c));
    return it == itargets.end() ? std::vector<std::string>{} : it->second;
  }
  bool unresolved(size_t ci) const {
    const CallFact& c = m.calls[ci];
    return c.callee_key.empty() && (iunresolved.count(key_of(c)) != 0 || itargets.count(key_of(c)) == 0);
  }
  const P0Entry* prim(const std::string& key) const {
    auto it = p0_by_name.find(key);
    if (it == p0_by_name.end() || body.count(key) != 0) return nullptr;
    return &p0_table()[it->second];
  }
  std::string name(const std::string& k) const {
    auto it = m.funcs.find(k);
    return it == m.funcs.end() ? k : it->second.name;
  }
};

// Whether a function runs at most once (FIELD_STATUS `once`): not address-taken, no spawn entry, not
// recursive, and no call site or one call site outside every loop in such a function.
class OnceCheck {
 public:
  explicit OnceCheck(const Model& m) : p_(m) {
    p_.build();
    std::vector<std::string> nodes(p_.body.begin(), p_.body.end());
    for (const auto& scc : sccs_of(nodes, p_.cg)) {
      const bool rec = scc.size() > 1 || (p_.cg.count(scc[0]) != 0 && p_.cg.at(scc[0]).count(scc[0]) != 0);
      if (rec) recursive_.insert(scc.begin(), scc.end());
    }
    for (size_t i = 0; i < m.calls.size(); ++i)
      for (const std::string& t : p_.targets(i)) callers_[t].push_back(i);
  }
  bool once(const std::string& f) {
    auto it = memo_.find(f);
    if (it != memo_.end()) return it->second;
    memo_[f] = false;  // a cycle through the callers is no proof
    bool r = true;
    auto fit = p_.m.funcs.find(f);
    if (fit == p_.m.funcs.end() || (fit->second.flags & KZ_FUNC_FLAG_ADDRESS_TAKEN) != 0 || p_.spawn_entries.count(f) != 0 || recursive_.count(f) != 0) {
      r = false;
    } else {
      auto cit = callers_.find(f);
      if (cit != callers_.end() && !cit->second.empty()) {
        if (cit->second.size() != 1) {
          r = false;
        } else {
          const CallFact& c = p_.m.calls[cit->second.front()];
          r = c.loop_key.empty() && !c.callee_key.empty() && once(c.caller_key);
        }
      }
    }
    return memo_[f] = r;
  }
  bool call_in_loop(const std::string& ck) const {
    auto it = p_.call_index.find(ck);
    return it != p_.call_index.end() && !p_.m.calls[it->second].loop_key.empty();
  }

 private:
  Prog p_;
  std::set<std::string> recursive_;
  std::map<std::string, std::vector<size_t>> callers_;
  std::map<std::string, bool> memo_;
};

// ---- the lock-set analysis -----------------------------------------------------------------------------

struct Acq {
  LockId id;
  std::string text;
  uint8_t mode = KZ_LOCKOP_MODE_NONE;
  bool assumed = false;
  bool has_success = false;
  int64_t success = 0;
};
// What a call does to the locks, over the caller's parameters.
struct Effect {
  std::vector<Acq> acq;
  std::vector<LockId> rel, rel_may, kill, wait;  // rel: on every path; rel_may: on some path
  bool unknown = false;
  bool has_result = false, result_assumed = false;
  int64_t result = 0;
  bool empty() const { return acq.empty() && rel.empty() && rel_may.empty() && kill.empty() && wait.empty() && !unknown && !has_result; }
};
// A function's summary, over its own parameters.
struct Summary {
  std::vector<Acq> acq;   // EVERY_RETURN, RESULT, TRY
  std::vector<Acq> cond;  // CONDITIONAL: no operation
  std::vector<Acq> rel;   // releases of the caller's lock: EVERY_RETURN, RESULT (on the paths that return `success`)
  std::vector<LockId> rel_may, kill, wait;  // rel_may: released on some path only
  bool unknown = false;
  bool has_result = false, result_assumed = false;
  int64_t result = 0;
  bool cfg = true;
  // per identity text: the call that leads there (acquire: an opener; release / wait: the call)
  std::map<std::string, std::string> via;
};

struct Val {
  uint8_t op = 0;  // 1: == c, 2: != c
  int64_t c = 0;
  std::string from;  // the acquisition whose assumed success gives it
  bool operator==(const Val& o) const { return op == o.op && c == o.c && from == o.from; }
};
struct Entry {
  LockId id;
  std::set<std::string> openers;
  bool assumed = false, try_ = false;
};
struct State {
  std::map<std::string, Entry> held;
  std::map<std::string, Val> facts;
  std::set<std::string> released;  // locks of the caller released on this path
  bool failure = false;
  bool unknown_release = false;
  // the first call (by source position) on a path from the entry that released a lock the function does not hold,
  // may have released a held lock of an identity that is not a static instance, or could not be followed
  // (LOCK_PATHS entry-release); merged as the earliest over the paths that reach a point
  std::string touched;
  // one state per set of locks held and of the caller's locks released, so that the paths of an operation that
  // returns a value telling them apart stay apart
  std::string key() const {
    std::string k = failure ? "F|" : "S|";
    for (const auto& kv : held) k += kv.first + "|";
    k += "R|";
    for (const std::string& r : released) k += r + "|";
    return k;
  }
};
using States = std::map<std::string, State>;

// the order of the witness calls of `touched`: by source position, then by call key (set by the analysis)
std::function<bool(const std::string&, const std::string&)> g_touched_before = [](const std::string& a, const std::string& b) { return a < b; };

bool merge_into(States* dst, const State& s) {
  const std::string k = s.key();
  auto it = dst->find(k);
  if (it == dst->end()) {
    dst->emplace(k, s);
    return true;
  }
  State& d = it->second;
  bool changed = false;
  for (const auto& kv : s.held) {
    Entry& e = d.held[kv.first];
    for (const std::string& o : kv.second.openers) changed |= e.openers.insert(o).second;
    if (kv.second.assumed && !e.assumed) e.assumed = changed = true;
    if (kv.second.try_ && !e.try_) e.try_ = changed = true;
  }
  for (auto fit = d.facts.begin(); fit != d.facts.end();) {
    auto sit = s.facts.find(fit->first);
    if (sit == s.facts.end() || !(sit->second == fit->second)) {
      fit = d.facts.erase(fit);
      changed = true;
    } else {
      ++fit;
    }
  }
  for (auto rit = d.released.begin(); rit != d.released.end();) {
    if (s.released.count(*rit) == 0) {
      rit = d.released.erase(rit);
      changed = true;
    } else {
      ++rit;
    }
  }
  if (s.unknown_release && !d.unknown_release) d.unknown_release = changed = true;
  if (!s.touched.empty() && (d.touched.empty() || g_touched_before(s.touched, d.touched))) {
    d.touched = s.touched;
    changed = true;
  }
  return changed;
}

void kill_may(State* s, const LockId& r) {
  const std::string tr = lock_id_text(r);
  for (auto it = s->held.begin(); it != s->held.end();) {
    if (may_equal(it->second.id, it->first, r, tr)) it = s->held.erase(it);
    else ++it;
  }
}

struct Ret {
  State st;
  bool known = false, none = false;
  Val v;
};

struct EventAcc {
  std::vector<State> st;  // the states that reach the event
};

}  // namespace

std::string lock_id_text(const LockId& id) {
  if (!id) return "?";
  switch (id->k) {
    case N::UNKNOWN: return "?(" + id->name + ")";
    case N::PARAM: return "$" + std::to_string(id->v);
    case N::CONST: return std::to_string(id->v);
    case N::GLOBAL: return id->name;
    case N::FIELD:
      if (id->a && id->a->k == N::DEREF) return lock_id_text(id->a->a) + "->" + field_part(id->name);
      return lock_id_text(id->a) + "." + field_part(id->name);
    case N::INDEX: return lock_id_text(id->a) + "[" + lock_id_text(id->b) + "]";
    case N::DEREF: return "*(" + lock_id_text(id->a) + ")";
    case N::ADDR: return "&" + lock_id_text(id->a);
    case N::LOAD: return lock_id_text(id->a);
    default: return "?";
  }
}

LockId lock_id_subst(const LockId& id, const std::vector<LockId>& args) {
  if (!id) return id;
  switch (id->k) {
    case N::PARAM:
      if (id->v >= 0 && static_cast<size_t>(id->v) < args.size() && args[static_cast<size_t>(id->v)]) return args[static_cast<size_t>(id->v)];
      return mk(N::UNKNOWN, 0, "argument " + std::to_string(id->v) + " is not passed");
    case N::UNKNOWN:
    case N::CONST:
    case N::GLOBAL: return id;
    default: break;
  }
  const LockId a = lock_id_subst(id->a, args);
  const LockId b = lock_id_subst(id->b, args);
  switch (id->k) {
    case N::ADDR:
      if (a->k == N::DEREF) return a->a;
      break;
    case N::DEREF:
      if (a->k == N::ADDR) return a->a;
      break;
    case N::INDEX:
      // p[i] with p = &x[0] is x[i]
      if (id->a && id->a->k == N::DEREF) {
        const LockId pv = lock_id_subst(id->a->a, args);
        if (pv->k == N::ADDR && pv->a && pv->a->k == N::INDEX && pv->a->b && pv->a->b->k == N::CONST && pv->a->b->v == 0) return mk(N::INDEX, 0, "", pv->a->a, b);
      }
      break;
    default: break;
  }
  if (a == id->a && b == id->b) return id;
  auto n = std::make_shared<LockIdNode>(*id);
  n->a = a;
  n->b = b;
  return n;
}

std::string lock_id_may(const LockId& id) {
  if (!id || id->k != N::ADDR) return "";
  return lv_may(id->a);
}

std::vector<FieldRef> completeness_fields(const Model& m) {
  std::set<std::string> types;
  for (const ReadFact& r : m.reads)
    if (r.kind == KZ_READ_KIND_FIELD) types.insert(r.type);
  std::set<FieldRef> out;
  for (const std::string& t : types) {
    auto it = m.record_fields.find(t);
    if (it == m.record_fields.end()) continue;
    for (const std::string& f : it->second) out.insert(FieldRef{t, f});
  }
  for (const ReadFact& r : m.reads)
    if (r.kind == KZ_READ_KIND_FIELD) out.insert(FieldRef{r.type, r.field});
  return std::vector<FieldRef>(out.begin(), out.end());
}

const FieldStatusFact* FieldStatusTable::find(const std::string& type, const std::string& field) const {
  auto it = rows.find({type, field});
  return it == rows.end() ? nullptr : &it->second;
}

void field_status_from(const Model& pass, const Model& m, FieldStatusTable* out) {
  *out = FieldStatusTable();
  OnceCheck once(m);
  std::map<std::pair<std::string, std::string>, bool> write_in_loop;  // (function, position) -> in a loop
  for (const WriteEvent& w : pass.write_events) write_in_loop[{w.func_key, w.pos.str()}] = write_in_loop[{w.func_key, w.pos.str()}] || !w.loop_key.empty();
  for (const CallFact& c : pass.calls) write_in_loop[{c.caller_key, c.pos.str()}] = write_in_loop[{c.caller_key, c.pos.str()}] || !c.loop_key.empty();
  const uint32_t untyped = static_cast<uint32_t>(pass.untyped_writes.size());
  // the raw hits per type, one per event (a type-level hit is repeated for every target of the type)
  struct Hit {
    uint8_t kind;
    std::string cause;
  };
  std::map<std::string, std::vector<Hit>> hits;
  {
    std::set<std::tuple<std::string, uint8_t, std::string, uint32_t, std::string, std::string>> seen;
    for (const ATypeFact& a : pass.atypes_raw) {
      if (a.kind != KZ_ATYPE_KIND_CAST && a.kind != KZ_ATYPE_KIND_UNION && a.kind != KZ_ATYPE_KIND_ADDRESS_ESCAPE && a.kind != KZ_ATYPE_KIND_UNKNOWN_EFFECT) continue;
      if (!seen.insert(std::make_tuple(a.type_name, a.kind, a.pos.str(), a.ordinal, a.detail, a.cause_field)).second) continue;
      hits[a.type_name].push_back(Hit{a.kind, a.cause_field});
    }
  }
  out->targets = static_cast<uint32_t>(pass.locs.size());
  for (const LocFact& l : pass.locs) {
    FieldStatusFact r;
    r.type = l.loc_type;
    r.field = l.loc_field;
    r.n_writers = l.n_writers;
    r.n_direct = l.n_direct;
    r.n_data_fn = l.n_data_fn;
    r.n_outptr = l.n_outptr;
    r.n_init = l.n_init;
    r.untyped = untyped;
    if (untyped > 0) r.flags |= KZ_FSTATUS_FLAG_UNTYPED_IN_PROGRAM;
    auto hit = hits.find(l.loc_type);
    if (hit != hits.end())
      for (const Hit& h : hit->second) {
        if (!h.cause.empty() && h.cause != l.loc_field) {
          ++r.sibling_hits;
          continue;
        }
        switch (h.kind) {
          case KZ_ATYPE_KIND_CAST: ++r.own_cast; break;
          case KZ_ATYPE_KIND_UNION: ++r.own_union; break;
          case KZ_ATYPE_KIND_ADDRESS_ESCAPE: ++r.own_escape; break;
          default: ++r.own_unknown; break;
        }
      }
    if (r.sibling_hits > 0) r.flags |= KZ_FSTATUS_FLAG_SIBLING_EXPOSED;
    const uint32_t own = r.own_cast + r.own_union + r.own_escape + r.own_unknown;
    r.status_field = own > 0 ? KZ_LOC_STATUS_DISABLED : KZ_LOC_STATUS_COMPLETE;
    r.status_type = (own > 0 || (r.sibling_hits > 0 && untyped > 0)) ? KZ_LOC_STATUS_DISABLED : KZ_LOC_STATUS_COMPLETE;
    // an array field whose elements are not const: a store to one element makes no writer row (the writer collection
    // reads typed stores of a field, not of its elements), so its writer set is not known to be complete
    if (m.array_fields.count(r.type + "." + r.field) != 0) {
      r.flags |= KZ_FSTATUS_FLAG_ARRAY_ELEMENTS;
      r.status_field = r.status_type = KZ_LOC_STATUS_DISABLED;
    }
    out->rows[{r.type, r.field}] = r;
  }
  // writer instances
  std::map<std::pair<std::string, std::string>, std::map<std::string, std::pair<std::string, bool>>> inst;  // field -> instance -> (function, in loop)
  std::map<std::pair<std::string, std::string>, std::set<std::string>> fns;
  for (const WriterFact& w : pass.writers) {
    const std::pair<std::string, std::string> f{w.loc_type, w.loc_field};
    if (out->rows.count(f) == 0) continue;
    FieldWriter fw;
    fw.func_key = w.func_key;
    fw.call_key = w.call_key;
    fw.pos = w.pos;
    fw.kind = w.kind;
    out->writers[f].push_back(fw);
    std::string key;
    bool in_loop = false;
    if (w.kind == KZ_WRITER_KIND_OUTPTR) {
      key = "call " + w.call_key;
      in_loop = once.call_in_loop(w.call_key);
    } else {
      key = w.pos.str() + "#" + std::to_string(w.ordinal);
      auto it = write_in_loop.find({w.func_key, w.pos.str()});
      in_loop = it != write_in_loop.end() && it->second;
    }
    inst[f][key] = {w.func_key, in_loop};
    auto fit = m.funcs.find(w.func_key);
    fns[f].insert(fit != m.funcs.end() ? fit->second.name : w.func_key);
  }
  for (auto& kv : out->rows) {
    FieldStatusFact& r = kv.second;
    const auto& is = inst[kv.first];
    r.n_instances = static_cast<uint32_t>(is.size());
    for (const std::string& f : fns[kv.first]) r.writer_fns += (r.writer_fns.empty() ? "" : "; ") + f;
    if (is.empty()) {
      r.once = KZ_FSTATUS_ONCE_NO_WRITER;
      continue;
    }
    bool ok = is.size() == 1;
    if (!ok) r.flags |= KZ_FSTATUS_FLAG_SEVERAL_INSTANCES;
    for (const auto& x : is) {
      if (x.second.second) {
        r.flags |= KZ_FSTATUS_FLAG_WRITER_IN_LOOP;
        ok = false;
      }
      if (!once.once(x.second.first)) {
        r.flags |= KZ_FSTATUS_FLAG_WRITER_NOT_ONCE;
        ok = false;
      }
    }
    r.once = ok ? KZ_FSTATUS_ONCE_ONCE : KZ_FSTATUS_ONCE_MORE;
  }
}

namespace {

// The static-instance check of an identity (LOCK_IDS).
struct IdCheck {
  uint8_t status = KZ_LOCKID_STATUS_STATIC;
  uint16_t flags = 0;
  std::string why, location, object;
  uint32_t writers = 0;
};

std::string root_global(const LockId& lv) {
  if (!lv) return "";
  if (lv->k == N::GLOBAL) return lv->name;
  if (lv->k == N::FIELD || lv->k == N::INDEX) return root_global(lv->a);
  return "";
}

void check_id(const LockId& id, const FieldStatusTable* fields, IdCheck* c, bool value) {
  if (!id) return;
  auto worse = [&](uint8_t st, const std::string& why) {
    // undecidable > parameter-relative > not-static > static
    auto rank = [](uint8_t s) { return s == KZ_LOCKID_STATUS_UNDECIDABLE ? 3 : s == KZ_LOCKID_STATUS_PARAM ? 2 : s == KZ_LOCKID_STATUS_NOT_STATIC ? 1 : 0; };
    if (rank(st) > rank(c->status)) {
      c->status = st;
      c->why = why;
    } else if (rank(st) == rank(c->status) && c->why.empty()) {
      c->why = why;
    }
  };
  switch (id->k) {
    case N::UNKNOWN: worse(KZ_LOCKID_STATUS_UNDECIDABLE, id->name); return;
    case N::PARAM: worse(KZ_LOCKID_STATUS_PARAM, "parameter " + std::to_string(id->v)); return;
    case N::CONST:
    case N::GLOBAL: return;
    case N::FIELD: check_id(id->a, fields, c, false); return;
    case N::INDEX:
      check_id(id->a, fields, c, false);
      check_id(id->b, fields, c, true);
      return;
    case N::DEREF: check_id(id->a, fields, c, true); return;
    case N::ADDR: check_id(id->a, fields, c, false); return;
    case N::LOAD: {
      c->flags |= KZ_LOCKID_FLAG_READS_VALUE;
      const LockId& lv = id->a;
      check_id(lv, fields, c, false);
      if (lv->k != N::FIELD || root_global(lv).empty()) {
        worse(KZ_LOCKID_STATUS_NOT_STATIC, lv->k == N::GLOBAL ? "the value of the global variable " + lv->name + " (no writer-set rule for a variable)"
                                                            : "a value that is not a field of a global object: " + lock_id_text(id));
        return;
      }
      const size_t dot = lv->name.rfind('.');
      const std::string type = lv->name.substr(0, dot), field = lv->name.substr(dot + 1);
      if (c->location.empty()) {
        c->location = lv->name;
        c->object = root_global(lv);
      }
      const FieldStatusFact* fs = fields != nullptr ? fields->find(type, field) : nullptr;
      if (fs == nullptr) {
        worse(KZ_LOCKID_STATUS_NOT_STATIC, lv->name + " has no FIELD_STATUS row");
        return;
      }
      c->writers = std::max(c->writers, fs->n_instances);
      if (fs->status_type != KZ_LOC_STATUS_COMPLETE) c->flags |= KZ_LOCKID_FLAG_TYPE_EXPOSED;
      if (fs->once == KZ_FSTATUS_ONCE_NO_WRITER) c->flags |= KZ_LOCKID_FLAG_INITIAL_VALUE;
      if (fs->status_field != KZ_LOC_STATUS_COMPLETE) worse(KZ_LOCKID_STATUS_NOT_STATIC, lv->name + ": the writer set is not complete (per-field rule)");
      else if (fs->once != KZ_FSTATUS_ONCE_NO_WRITER && fs->once != KZ_FSTATUS_ONCE_ONCE)
        worse(KZ_LOCKID_STATUS_NOT_STATIC, lv->name + ": " + std::to_string(fs->n_instances) + " writer instance(s), not one that runs once");
      return;
    }
    default: return;
  }
  (void)value;
}

IdCheck check(const LockId& id, const FieldStatusTable* fields) {
  IdCheck c;
  check_id(id, fields, &c, true);
  if (id && id->k == N::ADDR && c.status == KZ_LOCKID_STATUS_STATIC && (c.flags & KZ_LOCKID_FLAG_READS_VALUE) == 0) c.flags |= KZ_LOCKID_FLAG_ADDRESS_OF_GLOBAL;
  if (id && id->k == N::ADDR && c.status == KZ_LOCKID_STATUS_STATIC) c.flags |= KZ_LOCKID_FLAG_ADDRESS_OF_GLOBAL;
  return c;
}

bool stable(const LockId& id, const FieldStatusTable* fields) { return check(id, fields).status == KZ_LOCKID_STATUS_STATIC; }

class Analysis {
 public:
  Analysis(const Prog& p, const FieldStatusTable* fields) : p_(p), fields_(fields) {}

  void run(LockDerivation* d) {
    g_touched_before = [this](const std::string& a, const std::string& b) {
      auto ia = p_.call_index.find(a), ib = p_.call_index.find(b);
      if (ia != p_.call_index.end() && ib != p_.call_index.end()) {
        const Pos& pa = p_.m.calls[ia->second].pos;
        const Pos& pb = p_.m.calls[ib->second].pos;
        if (pa < pb) return true;
        if (pb < pa) return false;
      }
      return a < b;
    };
    std::vector<std::string> nodes(p_.body.begin(), p_.body.end());
    const auto sccs = sccs_of(nodes, p_.cg);
    d->stats.bodies = static_cast<uint32_t>(nodes.size());
    d->stats.cfg_built = static_cast<uint32_t>(p_.m.lock_cfgs.size());
    d->stats.sccs = static_cast<uint32_t>(sccs.size());
    for (const auto& scc : sccs) {
      const bool rec = scc.size() > 1 || (p_.cg.count(scc[0]) != 0 && p_.cg.at(scc[0]).count(scc[0]) != 0);
      if (rec) ++d->stats.sccs_recursive;
      cur_scc_ = rec ? std::set<std::string>(scc.begin(), scc.end()) : std::set<std::string>();
      for (const std::string& f : scc) analyse(f, &d->stats);
    }
    rows(d);
    ops(d);
    g_touched_before = [](const std::string& a, const std::string& b) { return a < b; };
  }

 private:
  // ---- the effect of a call ----
  // The effect of a call is read only while its caller is analysed, after every callee outside the
  // caller's own component has its summary: it is computed once.
  const Effect& effect(size_t ci) {
    auto it = effects_.find(ci);
    if (it != effects_.end()) return it->second;
    return effects_[ci] = compute_effect(ci);
  }
  Effect compute_effect(size_t ci) {
    const CallFact& c = p_.m.calls[ci];
    const std::string ck = key_of(c);
    Effect e;
    if (p_.unresolved(ci)) {
      if (c.callee_key.empty()) {
        e.unknown = true;
        return e;
      }
    }
    static const std::vector<LockId> none;
    auto ait = p_.m.call_arg_idents.find(ck);
    const std::vector<LockId>& args = ait != p_.m.call_arg_idents.end() ? ait->second : none;
    auto arg = [&](int k) { return k >= 0 && static_cast<size_t>(k) < args.size() && args[static_cast<size_t>(k)] ? args[static_cast<size_t>(k)] : mk(N::UNKNOWN, 0, "argument not recorded"); };
    std::vector<Effect> per;
    for (const std::string& t : p_.targets(ci)) {
      Effect x;
      if (const P0Entry* pe = p_.prim(t)) {
        switch (pe->kind) {
          case KZ_P0_KIND_MUTEX_ACQUIRE: {
            Acq a;
            a.id = arg(pe->mutex);
            a.text = lock_id_text(a.id);
            a.mode = pe->conditional ? KZ_LOCKOP_MODE_TRY : KZ_LOCKOP_MODE_EVERY_RETURN;
            a.assumed = !pe->conditional;
            a.has_success = true;
            a.success = 0;
            x.acq.push_back(a);
            if (!pe->conditional) {
              x.has_result = true;
              x.result_assumed = true;
            }
            break;
          }
          case KZ_P0_KIND_MUTEX_RELEASE:
            x.rel.push_back(arg(pe->mutex));
            x.has_result = x.result_assumed = true;
            break;
          case KZ_P0_KIND_COND_WAIT:
            x.wait.push_back(arg(pe->mutex));
            if (!pe->conditional) x.has_result = x.result_assumed = true;
            break;
          default: break;
        }
      } else if (p_.body.count(t) != 0) {
        if (cur_scc_.count(t) != 0) {
          x.unknown = true;
        } else {
          auto sit = sum_.find(t);
          if (sit != sum_.end()) {
            const Summary& s = sit->second;
            for (const Acq& a0 : s.acq) {
              Acq a = a0;
              a.id = lock_id_subst(a0.id, args);
              a.text = lock_id_text(a.id);
              x.acq.push_back(a);
            }
            for (const Acq& r : s.rel) x.rel.push_back(lock_id_subst(r.id, args));
            for (const LockId& r : s.rel_may) x.rel_may.push_back(lock_id_subst(r, args));
            for (const LockId& r : s.kill) {
              const LockId k = lock_id_subst(r, args);
              if (!stable(k, fields_)) x.kill.push_back(k);
            }
            for (const LockId& r : s.wait) x.wait.push_back(lock_id_subst(r, args));
            x.unknown = s.unknown;
            x.has_result = s.has_result;
            x.result = s.result;
            x.result_assumed = s.result_assumed;
          }
        }
      }
      per.push_back(x);
    }
    if (per.empty()) return e;
    if (per.size() == 1) return per.front();
    // several targets (an indirect call): an acquisition only when every target makes it; the rest of any
    e = per.front();
    for (size_t i = 1; i < per.size(); ++i) {
      std::vector<Acq> keep;
      for (const Acq& a : e.acq)
        for (const Acq& b : per[i].acq)
          if (a.text == b.text && a.mode == b.mode) {
            keep.push_back(a);
            break;
          }
      e.acq = keep;
      // a release only some targets make is a release on some path
      for (const LockId& r : per[i].rel) e.rel_may.push_back(r);
      e.rel_may.insert(e.rel_may.end(), per[i].rel_may.begin(), per[i].rel_may.end());
      e.kill.insert(e.kill.end(), per[i].kill.begin(), per[i].kill.end());
      e.wait.insert(e.wait.end(), per[i].wait.begin(), per[i].wait.end());
      e.unknown = e.unknown || per[i].unknown;
      if (!(per[i].has_result && e.has_result && per[i].result == e.result)) e.has_result = false;
    }
    return e;
  }

  // ---- one body ----
  struct Run {
    std::map<uint32_t, States> in;  // block -> states at its entry
    std::vector<Ret> rets;
    std::map<std::string, EventAcc> events;  // event key -> states
    std::set<std::string> rel_net, kill, wait;  // rel_net: releases of the caller's locks on some path
    std::map<std::string, LockId> ids;
    std::map<std::string, std::string> via;
    bool unknown = false;
  };

  static void apply_test(const LockBlock& blk, const State& s, std::vector<std::pair<uint32_t, State>>* out) {
    const LockTest& t = blk.test;
    const std::string subj = t.on_call ? "#" + t.subject : t.subject;
    auto it = s.facts.find(subj);
    int result = -1;  // -1 unknown, 0 false, 1 true
    if (it != s.facts.end()) {
      const Val& f = it->second;
      if (f.op == 1) {
        const bool eq = f.c == t.value;
        result = (t.op == 1) == eq ? 1 : 0;
      } else if (f.op == 2 && f.c == t.value) {
        result = t.op == 2 ? 1 : 0;
      }
    }
    const uint32_t yes = blk.succ.size() > 0 ? blk.succ[0] : kNone, no = blk.succ.size() > 1 ? blk.succ[1] : kNone;
    if (result < 0) {
      State a = s, b = s;
      a.facts[subj] = Val{t.op, t.value, ""};
      b.facts[subj] = Val{static_cast<uint8_t>(t.op == 1 ? 2 : 1), t.value, ""};
      if (yes != kNone) out->push_back({yes, a});
      if (no != kNone) out->push_back({no, b});
      return;
    }
    const uint32_t take = result == 1 ? yes : no, other = result == 1 ? no : yes;
    if (take != kNone) out->push_back({take, s});
    const Val& f = it->second;
    if (other == kNone || f.from.empty()) return;  // a branch the code itself excludes
    // the branch only a failure of the acquisition `f.from` takes
    State x = s;
    for (auto hit = x.held.begin(); hit != x.held.end();) {
      if (hit->second.openers.count(f.from) != 0) hit = x.held.erase(hit);
      else ++hit;
    }
    x.failure = true;
    x.facts.erase(subj);
    out->push_back({other, x});
  }

  // Runs the events of a block on one state; `acc` (optional) receives the state at each event.
  void step(const std::string& f, const LockBlock& blk, State s, Run* r, bool record, std::vector<std::pair<uint32_t, State>>* out) {
    for (const LockEvent& ev : blk.events) {
      switch (ev.kind) {
        case LockEvent::CALL: {
          if (record) r->events["C" + ev.key].st.push_back(s);
          auto cit = p_.call_index.find(ev.key);
          if (cit == p_.call_index.end()) break;
          apply(ev.key, effect(cit->second), &s, r);
          break;
        }
        case LockEvent::READ:
          if (record) r->events["R" + ev.key].st.push_back(s);
          break;
        case LockEvent::WRITE:
          if (record) r->events["W" + std::to_string(ev.index)].st.push_back(s);
          break;
        case LockEvent::DEF: {
          if (ev.vkind == 2) {
            auto it = s.facts.find("#" + ev.call);
            if (it != s.facts.end()) s.facts[ev.key] = it->second;
            else s.facts.erase(ev.key);
          } else if (ev.vkind == 1) {
            s.facts[ev.key] = Val{1, ev.value, ""};
          } else {
            s.facts.erase(ev.key);
          }
          break;
        }
        case LockEvent::RET: {
          if (record) {
            Ret rt;
            rt.st = s;
            if (ev.vkind == 4) {
              rt.none = true;
            } else if (ev.vkind == 1) {
              rt.known = true;
              rt.v = Val{1, ev.value, ""};
            } else if (ev.vkind == 2 || ev.vkind == 3) {
              auto it = s.facts.find(ev.vkind == 2 ? "#" + ev.call : ev.key);
              if (it != s.facts.end()) {
                rt.known = true;
                rt.v = it->second;
              }
            }
            r->rets.push_back(rt);
          }
          break;
        }
        default: break;
      }
      if (split_.has_value()) {
        // a try acquisition split the state: the rest of the block runs on both
        State other = *split_;
        split_.reset();
        // continue the other state through the rest of the block
        const size_t at = static_cast<size_t>(&ev - blk.events.data()) + 1;
        LockBlock rest;
        rest.events.assign(blk.events.begin() + static_cast<std::ptrdiff_t>(at), blk.events.end());
        rest.succ = blk.succ;
        rest.branch = blk.branch;
        rest.noreturn = blk.noreturn;
        rest.test = blk.test;
        step(f, rest, other, r, record, out);
      }
    }
    end_of_block(blk, s, r, record, out);
  }

  void end_of_block(const LockBlock& blk, const State& s, Run* r, bool record, std::vector<std::pair<uint32_t, State>>* out) {
    if (blk.noreturn) return;
    // a block that falls into the exit without a return statement returns no value
    if (record && cur_cfg_ != nullptr) {
      bool to_exit = false, has_ret = false;
      for (uint32_t sx : blk.succ)
        if (sx == cur_cfg_->exit) to_exit = true;
      for (const LockEvent& ev : blk.events)
        if (ev.kind == LockEvent::RET) has_ret = true;
      if (to_exit && !has_ret && &blk != &cur_cfg_->blocks[cur_cfg_->exit]) {
        Ret rt;
        rt.st = s;
        rt.none = true;
        r->rets.push_back(rt);
      }
    }
    if (blk.branch && blk.test.op != 0) {
      apply_test(blk, s, out);
      return;
    }
    for (uint32_t sx : blk.succ)
      if (sx != kNone) out->push_back({sx, s});
  }

  void apply(const std::string& ck, const Effect& e, State* s, Run* r) {
    // a lock the caller holds may not be held after this call: a release of a lock the function does not hold
    // itself, a release of a lock of an identity that is not a static instance, or an effect not followed
    bool touches = e.unknown || !e.kill.empty();
    for (const std::vector<LockId>* v : {&e.rel, &e.rel_may})
      for (const LockId& x : *v)
        if (s->held.count(lock_id_text(x)) == 0) touches = true;
    // a condition wait on a lock the function does not hold releases the caller's lock while it waits
    for (const LockId& x : e.wait)
      if (s->held.count(lock_id_text(x)) == 0) touches = true;
    if (touches && (s->touched.empty() || g_touched_before(ck, s->touched))) s->touched = ck;
    if (e.unknown) {
      if (!s->held.empty()) s->unknown_release = true;
      s->held.clear();
      if (!s->failure) r->unknown = true;
    }
    auto release = [&](const LockId& x, bool every) {
      const std::string tx = lock_id_text(x);
      if (s->held.count(tx) != 0) {
        if (!s->failure) r->kill.insert(tx);  // a pair of the function's own: the caller decides whether it is one mutex
      } else {
        if (every) s->released.insert(tx);
        if (!s->failure) {
          r->rel_net.insert(tx);
          if (r->via.count("R" + tx) == 0) r->via["R" + tx] = ck;
        }
      }
      if (!s->failure) r->ids[tx] = x;
      kill_may(s, x);
    };
    for (const LockId& x : e.rel) release(x, true);
    for (const LockId& x : e.rel_may) release(x, false);
    for (const LockId& x : e.kill) {
      const std::string tx = lock_id_text(x);
      if (!s->failure) {
        r->kill.insert(tx);
        r->ids[tx] = x;
      }
      kill_may(s, x);
    }
    // a condition wait holds the mutex again when it returns: a lock held before it is held after it, from
    // this call on (the mutex was released in between); one not known to be held stays not held
    for (const LockId& x : e.wait) {
      const std::string tx = lock_id_text(x);
      const bool own = s->held.count(tx) != 0;
      if (!s->failure && !own) {
        r->wait.insert(tx);
        r->ids[tx] = x;
        if (r->via.count("W" + tx) == 0) r->via["W" + tx] = ck;
      }
      Entry en;
      if (own) {
        en = s->held[tx];
        en.openers = {ck};
      }
      kill_may(s, x);
      if (own) s->held[tx] = en;
    }
    s->facts.erase("#" + ck);
    bool split = false;
    for (const Acq& a : e.acq) {
      if (a.mode == KZ_LOCKOP_MODE_TRY && !split) {
        split = true;
        State other = *s;
        other.facts["#" + ck] = Val{2, a.success, ""};
        split_ = other;
        Entry en;
        en.id = a.id;
        en.openers.insert(ck);
        en.try_ = true;
        if (s->held.count(a.text) == 0) s->held[a.text] = en;
        s->facts["#" + ck] = Val{1, a.success, ""};
        continue;
      }
      if (a.mode == KZ_LOCKOP_MODE_TRY) continue;
      if (s->held.count(a.text) == 0) {
        Entry en;
        en.id = a.id;
        en.openers.insert(ck);
        en.assumed = a.assumed;
        s->held[a.text] = en;
      }
    }
    if (!split && e.has_result) s->facts["#" + ck] = Val{1, e.result, e.result_assumed ? ck : ""};
  }

  void analyse(const std::string& f, Model::LockStats* stats) {
    Summary sm;
    auto git = p_.m.lock_cfgs.find(f);
    if (git == p_.m.lock_cfgs.end()) {
      // no control-flow graph: any lock effect of its calls makes it a call that cannot be followed
      sm.cfg = false;
      auto cit = p_.calls_of.find(f);
      if (cit != p_.calls_of.end())
        for (size_t ci : cit->second)
          if (!effect(ci).empty()) sm.unknown = true;
      sum_[f] = sm;
      return;
    }
    const LockCfg& g = git->second;
    cur_cfg_ = &g;
    Run r;
    std::vector<uint32_t> work;
    std::set<uint32_t> queued;
    if (g.entry < g.blocks.size()) {
      merge_into(&r.in[g.entry], State());
      work.push_back(g.entry);
      queued.insert(g.entry);
    }
    while (!work.empty()) {
      std::sort(work.begin(), work.end(), std::greater<uint32_t>());
      const uint32_t b = work.back();
      work.pop_back();
      queued.erase(b);
      const States ins = r.in[b];
      std::vector<std::pair<uint32_t, State>> out;
      for (const auto& kv : ins) step(f, g.blocks[b], kv.second, &r, false, &out);
      for (auto& o : out) {
        if (o.first >= g.blocks.size()) continue;
        if (merge_into(&r.in[o.first], o.second) && queued.insert(o.first).second) work.push_back(o.first);
      }
    }
    // the final sweep: events, returns, releases
    Run fin;
    fin.in = r.in;
    for (const auto& bk : r.in) {
      std::vector<std::pair<uint32_t, State>> out;
      for (const auto& kv : bk.second) step(f, g.blocks[bk.first], kv.second, &fin, true, &out);
    }
    cur_cfg_ = nullptr;
    runs_[f] = fin.events;
    // the summary from the returns of the paths the assumption keeps
    std::vector<const Ret*> rets;
    for (const Ret& rt : fin.rets)
      if (!rt.st.failure) rets.push_back(&rt);
    std::map<std::string, LockId> idents;
    for (const Ret* rt : rets)
      for (const auto& kv : rt->st.held) idents[kv.first] = kv.second.id;
    // the mode of an operation from the returns on which it holds (EVERY_RETURN: all of them; RESULT: exactly
    // those that return one constant, the others returning values known to differ; CONDITIONAL: neither)
    auto classify = [&](const std::function<bool(const Ret*)>& on, Acq* a) {
      std::vector<const Ret*> holding, others;
      for (const Ret* rt : rets) (on(rt) ? holding : others).push_back(rt);
      if (others.empty()) {
        a->mode = KZ_LOCKOP_MODE_EVERY_RETURN;
        return;
      }
      bool ok = !holding.empty();
      int64_t c = 0;
      for (size_t i = 0; i < holding.size() && ok; ++i) {
        if (!holding[i]->known || holding[i]->v.op != 1) ok = false;
        else if (i == 0) c = holding[i]->v.c;
        else if (holding[i]->v.c != c) ok = false;
      }
      for (const Ret* o : others) {
        if (!ok) break;
        if (!o->known) ok = false;
        else if (o->v.op == 1 && o->v.c == c) ok = false;
        else if (o->v.op == 2 && o->v.c != c) ok = false;
      }
      if (ok) {
        a->mode = KZ_LOCKOP_MODE_RESULT;
        a->has_success = true;
        a->success = c;
      } else {
        a->mode = KZ_LOCKOP_MODE_CONDITIONAL;
      }
    };
    for (const auto& kv : idents) {
      bool assumed = false, tried = false;
      std::string opener;
      for (const Ret* rt : rets) {
        auto it = rt->st.held.find(kv.first);
        if (it == rt->st.held.end()) continue;
        assumed = assumed || it->second.assumed;
        tried = tried || it->second.try_;
        if (!it->second.openers.empty() && (opener.empty() || *it->second.openers.begin() < opener)) opener = *it->second.openers.begin();
      }
      Acq a;
      a.id = kv.second;
      a.text = kv.first;
      a.assumed = assumed;
      const std::string text = kv.first;
      classify([&](const Ret* rt) { return rt->st.held.count(text) != 0; }, &a);
      if (a.mode == KZ_LOCKOP_MODE_RESULT && tried && !assumed) a.mode = KZ_LOCKOP_MODE_TRY;
      sm.via["A" + a.text] = opener;
      if (a.mode == KZ_LOCKOP_MODE_CONDITIONAL) sm.cond.push_back(a);
      else sm.acq.push_back(a);
    }
    // releases of the caller's locks, read like the acquisitions (a release that is not a try succeeds too)
    for (const std::string& t : fin.rel_net) {
      Acq a;
      a.id = fin.ids[t];
      a.text = t;
      a.assumed = true;
      classify([&](const Ret* rt) { return rt->st.released.count(t) != 0; }, &a);
      if (rets.empty()) a.mode = KZ_LOCKOP_MODE_CONDITIONAL;
      if (a.mode == KZ_LOCKOP_MODE_CONDITIONAL) sm.rel_may.push_back(a.id);
      else sm.rel.push_back(a);
    }
    // the value an operation that holds on every return gives back
    if (!rets.empty() && (!sm.acq.empty() || !sm.rel.empty())) {
      bool same = true, any_assumed = false;
      int64_t c = 0;
      for (size_t i = 0; i < rets.size() && same; ++i) {
        if (!rets[i]->known || rets[i]->v.op != 1) same = false;
        else if (i == 0) c = rets[i]->v.c;
        else if (rets[i]->v.c != c) same = false;
        if (same && !rets[i]->v.from.empty()) any_assumed = true;
      }
      for (const std::vector<Acq>* ops : {&sm.acq, &sm.rel})
        for (const Acq& a : *ops)
          if (a.mode == KZ_LOCKOP_MODE_RESULT && !sm.has_result) {
            sm.has_result = true;
            sm.result = a.success;
            sm.result_assumed = a.assumed;
          }
      if (!sm.has_result && same) {
        sm.has_result = true;
        sm.result = c;
        sm.result_assumed = any_assumed;
      }
    }
    for (const std::string& t : fin.kill)
      if (fin.rel_net.count(t) == 0) sm.kill.push_back(fin.ids[t]);
    for (const std::string& t : fin.wait) sm.wait.push_back(fin.ids[t]);
    for (const auto& kv : fin.via) sm.via[kv.first] = kv.second;
    sm.unknown = fin.unknown;
    if (sm.unknown) ++stats->kills_unknown;
    sum_[f] = sm;
  }

  // ---- LOCKSET rows ----
  void rows(LockDerivation* d) {
    std::map<std::string, size_t> id_index;
    std::map<std::string, LockId> id_of;
    std::vector<LockSetFact> out;
    auto row_from = [&](LockSetFact* row, const std::string& fkey, const std::string& ek) {
      auto fit = runs_.find(fkey);
      if (fit == runs_.end()) {
        row->flags |= p_.m.lock_cfgs.count(fkey) == 0 ? KZ_LOCKSET_FLAG_NO_CFG : KZ_LOCKSET_FLAG_NOT_REACHED;
        return;
      }
      auto eit = fit->second.find(ek);
      if (eit == fit->second.end() || eit->second.st.empty()) {
        row->flags |= KZ_LOCKSET_FLAG_NOT_REACHED;
        return;
      }
      std::vector<const State*> ok, failed;
      for (const State& s : eit->second.st) (s.failure ? failed : ok).push_back(&s);
      const std::vector<const State*>& use = ok.empty() ? failed : ok;
      if (ok.empty()) row->flags |= KZ_LOCKSET_FLAG_FAILURE_PATH;
      for (const State* s : use)
        if (!s->touched.empty()) {
          std::string& t = touched_[row];
          if (t.empty() || g_touched_before(s->touched, t)) t = s->touched;
        }
      std::map<std::string, std::set<std::string>> common;
      bool first = true;
      for (const State* s : use) {
        if (s->unknown_release) row->flags |= KZ_LOCKSET_FLAG_UNKNOWN_RELEASE;
        if (first) {
          for (const auto& kv : s->held) {
            common[kv.first] = kv.second.openers;
            id_of[kv.first] = kv.second.id;
          }
          first = false;
          continue;
        }
        for (auto it = common.begin(); it != common.end();) {
          auto h = s->held.find(it->first);
          if (h == s->held.end()) {
            it = common.erase(it);
          } else {
            it->second.insert(h->second.openers.begin(), h->second.openers.end());
            ++it;
          }
        }
      }
      // the identities by text for now; LOCK_IDS indices below
      std::vector<std::string>& texts = held_text_[row];
      for (const auto& kv : common) {
        row->openers.insert(row->openers.end(), kv.second.begin(), kv.second.end());
        id_index[kv.first] = 0;
        texts.push_back(kv.first);
      }
      std::sort(row->openers.begin(), row->openers.end());
      row->openers.erase(std::unique(row->openers.begin(), row->openers.end()), row->openers.end());
    };
    out.reserve(p_.m.calls.size() + p_.m.reads.size() + p_.m.write_sites.size());
    for (const CallFact& c : p_.m.calls) {
      if (p_.body.count(c.caller_key) == 0) continue;
      LockSetFact r;
      r.func_key = c.caller_key;
      r.kind = KZ_LOCKSET_KIND_CALL;
      r.ref = key_of(c);
      r.pos = c.pos;
      out.push_back(r);
    }
    for (const ReadFact& rd : p_.m.reads) {
      LockSetFact r;
      r.func_key = rd.func_key;
      r.kind = KZ_LOCKSET_KIND_READ;
      r.ref = rd.key();
      r.pos = rd.pos;
      out.push_back(r);
    }
    for (size_t i = 0; i < p_.m.write_sites.size(); ++i) {
      LockSetFact r;
      r.func_key = p_.m.write_sites[i].func_key;
      r.kind = KZ_LOCKSET_KIND_WRITE;
      r.ref = std::to_string(i);
      r.write = static_cast<uint32_t>(i);
      r.pos = p_.m.write_sites[i].pos;
      out.push_back(r);
    }
    for (LockSetFact& r : out) {
      const std::string ek = (r.kind == KZ_LOCKSET_KIND_CALL ? "C" : r.kind == KZ_LOCKSET_KIND_READ ? "R" : "W") + r.ref;
      row_from(&r, r.func_key, ek);
    }
    paths(out, d);
    // LOCK_IDS: the identities held anywhere, by text
    std::vector<std::string> texts;
    for (const auto& kv : id_index) texts.push_back(kv.first);
    std::sort(texts.begin(), texts.end());
    std::map<std::string, uint32_t> idx;
    for (const std::string& t : texts) {
      idx[t] = static_cast<uint32_t>(d->ids.size());
      const LockId& id = id_of[t];
      LockIdFact f;
      f.text = t;
      f.may = lock_id_may(id);
      f.kind = has_kind(id, N::UNKNOWN) ? KZ_LOCKID_KIND_UNKNOWN : has_kind(id, N::PARAM) ? KZ_LOCKID_KIND_PARAM : KZ_LOCKID_KIND_CLOSED;
      const IdCheck c = check(id, fields_);
      f.status = c.status;
      f.flags = c.flags;
      f.location = c.location;
      f.object = c.object;
      f.why = c.why;
      f.writers = c.writers;
      d->ids.push_back(f);
    }
    for (LockSetFact& r : out) {
      auto it = held_text_.find(&r);
      r.held.clear();
      if (it != held_text_.end())
        for (const std::string& t : it->second) r.held.push_back(idx[t]);
      std::sort(r.held.begin(), r.held.end());
      if (!r.held.empty()) ++d->stats.rows_held;
      if (r.flags & KZ_LOCKSET_FLAG_FAILURE_PATH) ++d->stats.rows_failure_path;
    }
    held_text_.clear();
    touched_.clear();
    d->rows = std::move(out);
  }

  // ---- LOCK_PATHS rows ----
  // ENTRY_RELEASE from the states of the final sweep (touched_); AFTER_SPAWN from the skeleton: the events of the
  // spawn call's block after it, and every event of a block reachable from that block's successors (the spawn's own
  // block included when a back edge leads to it).  No branch test prunes a successor.
  void paths(const std::vector<LockSetFact>& out, LockDerivation* d) {
    std::map<std::string, uint32_t> row_of;  // function key, event key -> row
    for (size_t i = 0; i < out.size(); ++i) {
      const LockSetFact& r = out[i];
      const std::string ek = (r.kind == KZ_LOCKSET_KIND_CALL ? "C" : r.kind == KZ_LOCKSET_KIND_READ ? "R" : "W") + r.ref;
      row_of[r.func_key + '\x1f' + ek] = static_cast<uint32_t>(i);
    }
    auto event_key = [](const LockEvent& ev) -> std::string {
      switch (ev.kind) {
        case LockEvent::CALL: return "C" + ev.key;
        case LockEvent::READ: return "R" + ev.key;
        case LockEvent::WRITE: return "W" + std::to_string(ev.index);
        default: return "";
      }
    };
    for (const auto& [f, g] : p_.m.lock_cfgs) {
      std::vector<std::pair<uint32_t, uint32_t>> spawns;  // (block, event)
      for (uint32_t b = 0; b < g.blocks.size(); ++b)
        for (uint32_t i = 0; i < g.blocks[b].events.size(); ++i) {
          const LockEvent& ev = g.blocks[b].events[i];
          if (ev.kind != LockEvent::CALL) continue;
          auto cit = p_.call_index.find(ev.key);
          if (cit != p_.call_index.end() && (p_.m.calls[cit->second].flags & KZ_CALL_FLAG_SPAWN_OP) != 0) spawns.push_back({b, i});
        }
      for (const auto& [sb, si] : spawns) {
        ++d->stats.spawn_calls;
        const std::string spawn_key = g.blocks[sb].events[si].key;
        std::set<uint32_t> rows;
        auto add = [&](const LockEvent& ev) {
          const std::string ek = event_key(ev);
          if (ek.empty()) return;
          auto it = row_of.find(f + '\x1f' + ek);
          if (it != row_of.end()) rows.insert(it->second);
        };
        for (uint32_t i = si + 1; i < g.blocks[sb].events.size(); ++i) add(g.blocks[sb].events[i]);
        std::set<uint32_t> seen;
        std::vector<uint32_t> work;
        for (uint32_t x : g.blocks[sb].succ)
          if (x < g.blocks.size() && seen.insert(x).second) work.push_back(x);
        while (!work.empty()) {
          const uint32_t b = work.back();
          work.pop_back();
          for (const LockEvent& ev : g.blocks[b].events) add(ev);
          for (uint32_t x : g.blocks[b].succ)
            if (x < g.blocks.size() && seen.insert(x).second) work.push_back(x);
        }
        for (uint32_t row : rows) {
          LockPathFact pf;
          pf.row = row;
          pf.func_key = f;
          pf.call_key = spawn_key;
          pf.kind = KZ_LOCKPATH_KIND_AFTER_SPAWN;
          d->paths.push_back(pf);
          ++d->stats.after_spawn;
        }
      }
    }
    for (size_t i = 0; i < out.size(); ++i) {
      auto it = touched_.find(&out[i]);
      if (it == touched_.end()) continue;
      LockPathFact pf;
      pf.row = static_cast<uint32_t>(i);
      pf.func_key = out[i].func_key;
      pf.call_key = it->second;
      pf.kind = KZ_LOCKPATH_KIND_ENTRY_RELEASE;
      d->paths.push_back(pf);
      ++d->stats.entry_release;
    }
    std::sort(d->paths.begin(), d->paths.end(), [](const LockPathFact& a, const LockPathFact& b) {
      return std::tie(a.row, a.kind, a.call_key) < std::tie(b.row, b.kind, b.call_key);
    });
  }

  // ---- LOCK_OPS rows ----
  void ops(LockDerivation* d) {
    // primitives the program calls
    std::set<std::string> prims;
    for (size_t ci = 0; ci < p_.m.calls.size(); ++ci)
      for (const std::string& t : p_.targets(ci))
        if (const P0Entry* pe = p_.prim(t))
          if (pe->kind == KZ_P0_KIND_MUTEX_ACQUIRE || pe->kind == KZ_P0_KIND_MUTEX_RELEASE || pe->kind == KZ_P0_KIND_COND_WAIT) prims.insert(t);
    for (const std::string& t : prims) {
      const P0Entry* pe = p_.prim(t);
      LockOpFact r;
      r.func_key = t;
      r.primitive = t;
      r.chain = t;
      r.ident = "$" + std::to_string(pe->mutex);
      r.flags = KZ_LOCKOP_FLAG_PRIMITIVE | KZ_LOCKOP_FLAG_PARAM_IDENT;
      if (pe->kind == KZ_P0_KIND_MUTEX_ACQUIRE) {
        r.kind = KZ_LOCKOP_KIND_ACQUIRE;
        r.mode = pe->conditional ? KZ_LOCKOP_MODE_TRY : KZ_LOCKOP_MODE_RESULT;
        r.success = 0;
        if (!pe->conditional) {
          r.flags |= KZ_LOCKOP_FLAG_ASSUMES_SUCCESS;
          d->lock_keys.insert(t);
          ++d->stats.acquire_ops;
        } else {
          ++d->stats.try_ops;
        }
      } else if (pe->kind == KZ_P0_KIND_MUTEX_RELEASE) {
        r.kind = KZ_LOCKOP_KIND_RELEASE;
        r.mode = KZ_LOCKOP_MODE_EVERY_RETURN;
        d->unlock_keys.insert(t);
        ++d->stats.release_ops;
      } else {
        r.kind = KZ_LOCKOP_KIND_WAIT;
        ++d->stats.wait_ops;
      }
      d->ops.push_back(r);
    }
    // derived
    std::map<std::string, std::pair<std::string, uint32_t>> chain_memo;
    std::function<std::pair<std::string, uint32_t>(const std::string&, const std::string&, std::set<std::string>&)> chain =
        [&](const std::string& f, const std::string& tag, std::set<std::string>& seen) -> std::pair<std::string, uint32_t> {
      if (p_.prim(f) != nullptr) return {f, 0};
      if (!seen.insert(f).second) return {p_.name(f) + " -> ...", 0};
      auto sit = sum_.find(f);
      std::string via;
      if (sit != sum_.end()) {
        auto vit = sit->second.via.find(tag);
        if (vit != sit->second.via.end()) via = vit->second;
        if (via.empty())
          for (const auto& kv : sit->second.via)
            if (kv.first[0] == tag[0] && !kv.second.empty()) {
              via = kv.second;
              break;
            }
      }
      auto cit = p_.call_index.find(via);
      if (via.empty() || cit == p_.call_index.end()) return {p_.name(f), 0};
      std::string next;
      for (const std::string& t : p_.targets(cit->second))
        if (next.empty() || t < next) next = t;
      if (next.empty()) return {p_.name(f), 0};
      // the callee's own operation of the same kind (any identity)
      const auto sub = chain(next, std::string(1, tag[0]), seen);
      return {p_.name(f) + " -> " + sub.first, sub.second + 1};
    };
    auto next_of = [&](const std::string& ck) {
      auto cit = p_.call_index.find(ck);
      if (ck.empty() || cit == p_.call_index.end()) return std::string();
      std::string next;
      for (const std::string& t : p_.targets(cit->second))
        if (next.empty() || t < next) next = t;
      return next;
    };
    auto prim_of = [&](const std::string& c) {
      const size_t sp = c.rfind(" -> ");
      return sp == std::string::npos ? c : c.substr(sp + 4);
    };
    for (const auto& kv : sum_) {
      const std::string& f = kv.first;
      const Summary& s = kv.second;
      auto base = [&](uint8_t kind, const std::string& tag) {
        LockOpFact r;
        r.func_key = f;
        r.kind = kind;
        auto vit = s.via.find(tag);
        r.call_key = vit != s.via.end() ? vit->second : std::string();
        r.next_key = next_of(r.call_key);
        std::set<std::string> seen;
        const auto ch = chain(f, tag, seen);
        r.chain = ch.first;
        r.depth = ch.second;
        r.primitive = prim_of(ch.first);
        if (!s.cfg) r.flags |= KZ_LOCKOP_FLAG_NO_CFG;
        auto cit = p_.call_index.find(r.call_key);
        if (cit != p_.call_index.end() && p_.m.calls[cit->second].callee_key.empty()) r.flags |= KZ_LOCKOP_FLAG_INDIRECT;
        return r;
      };
      auto ident = [&](LockOpFact* r, const LockId& id) {
        r->ident = lock_id_text(id);
        r->may = lock_id_may(id);
        if (has_kind(id, N::PARAM)) r->flags |= KZ_LOCKOP_FLAG_PARAM_IDENT;
        if (has_kind(id, N::UNKNOWN)) r->flags |= KZ_LOCKOP_FLAG_UNKNOWN_IDENT;
      };
      bool is_lock = false, is_unlock = false;
      for (const Acq& a : s.acq) {
        LockOpFact r = base(KZ_LOCKOP_KIND_ACQUIRE, "A" + a.text);
        ident(&r, a.id);
        r.mode = a.mode;
        r.success = a.has_success ? a.success : 0;
        if (a.assumed) r.flags |= KZ_LOCKOP_FLAG_ASSUMES_SUCCESS;
        if (a.mode == KZ_LOCKOP_MODE_TRY) ++d->stats.try_ops;
        else {
          is_lock = true;
          ++d->stats.acquire_ops;
        }
        d->ops.push_back(r);
      }
      for (const Acq& a : s.cond) {
        LockOpFact r = base(KZ_LOCKOP_KIND_ACQUIRE, "A" + a.text);
        ident(&r, a.id);
        r.mode = KZ_LOCKOP_MODE_CONDITIONAL;
        if (a.assumed) r.flags |= KZ_LOCKOP_FLAG_ASSUMES_SUCCESS;
        d->ops.push_back(r);
      }
      for (const Acq& a : s.rel) {
        LockOpFact r = base(KZ_LOCKOP_KIND_RELEASE, "R" + a.text);
        ident(&r, a.id);
        r.mode = a.mode;
        r.success = a.has_success ? a.success : 0;
        r.flags |= KZ_LOCKOP_FLAG_ASSUMES_SUCCESS;
        is_unlock = true;
        ++d->stats.release_ops;
        d->ops.push_back(r);
      }
      for (const LockId& x : s.rel_may) {
        LockOpFact r = base(KZ_LOCKOP_KIND_RELEASE, "R" + lock_id_text(x));
        ident(&r, x);
        r.mode = KZ_LOCKOP_MODE_CONDITIONAL;
        d->ops.push_back(r);
      }
      for (const LockId& x : s.wait) {
        LockOpFact r = base(KZ_LOCKOP_KIND_WAIT, "W" + lock_id_text(x));
        ident(&r, x);
        ++d->stats.wait_ops;
        d->ops.push_back(r);
      }
      if (is_lock) d->lock_keys.insert(f);
      (void)is_unlock;
    }
    // the unlock operations of the vocabulary: a release on every return (or result-conditional) of a mutex of a
    // location some acquire operation of the vocabulary acquires (field-based; the P0 primitive always)
    std::set<std::string> acquired_at;
    for (const LockOpFact& r : d->ops)
      if (r.kind == KZ_LOCKOP_KIND_ACQUIRE && (r.mode == KZ_LOCKOP_MODE_EVERY_RETURN || r.mode == KZ_LOCKOP_MODE_RESULT) && !r.may.empty()) acquired_at.insert(r.may);
    for (const LockOpFact& r : d->ops)
      if (r.kind == KZ_LOCKOP_KIND_RELEASE && (r.flags & KZ_LOCKOP_FLAG_PRIMITIVE) == 0 && (r.mode == KZ_LOCKOP_MODE_EVERY_RETURN || r.mode == KZ_LOCKOP_MODE_RESULT) &&
          acquired_at.count(r.may) != 0)
        d->unlock_keys.insert(r.func_key);
    std::stable_sort(d->ops.begin(), d->ops.end(), [](const LockOpFact& a, const LockOpFact& b) {
      return std::tie(a.func_key, a.kind, a.ident, a.mode) < std::tie(b.func_key, b.kind, b.ident, b.mode);
    });
  }

  const Prog& p_;
  const FieldStatusTable* fields_;
  std::set<std::string> cur_scc_;
  const LockCfg* cur_cfg_ = nullptr;
  std::map<std::string, Summary> sum_;
  std::map<std::string, std::map<std::string, EventAcc>> runs_;
  std::map<const LockSetFact*, std::vector<std::string>> held_text_;
  std::map<const LockSetFact*, std::string> touched_;
  std::optional<State> split_;
  std::map<size_t, Effect> effects_;
};

}  // namespace

void derive_locks(const Model& m, const FieldStatusTable* fields, LockDerivation* out) {
  *out = LockDerivation();
  Prog p(m);
  p.build();
  Analysis a(p, fields);
  a.run(out);
}

namespace {

struct Loc {
  std::string type, field;
  bool operator<(const Loc& o) const { return std::tie(type, field) < std::tie(o.type, o.field); }
};

// The fields and globals an expression reads (the base of a cursor over an array of known size is no read), and
// the functions it calls (CALL_RESULT names).
void pred_reads(ExprRef e, std::set<Loc>* out, std::set<std::string>* callees) {
  if (e == nullptr) return;
  if (e->op == KZ_EXPR_OP_CURSOR) {
    if (e->value <= 0) pred_reads(e->a, out, callees);
    pred_reads(e->b, out, callees);
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
  if (e->op == KZ_EXPR_OP_CALL_RESULT) callees->insert(e->name);
  pred_reads(e->a, out, callees);
  pred_reads(e->b, out, callees);
}

}  // namespace

void finalize_lock_facts(Model* mp, const LockDerivation& d, const FieldStatusTable& fields, const std::set<std::string>& vocabulary) {
  Model& m = *mp;
  m.lock_ops = d.ops;
  // `vocabulary` is a property of the row (the same in both modes); `vocabulary_in` (the profile-free second
  // pass's lock and unlock operations) is compared with it for the record
  (void)vocabulary;
  for (LockOpFact& r : m.lock_ops)
    if ((r.kind == KZ_LOCKOP_KIND_ACQUIRE && (r.mode == KZ_LOCKOP_MODE_EVERY_RETURN || r.mode == KZ_LOCKOP_MODE_RESULT)) ||
        (r.kind == KZ_LOCKOP_KIND_RELEASE && (d.lock_keys.count(r.func_key) != 0 || d.unlock_keys.count(r.func_key) != 0) &&
         (r.mode == KZ_LOCKOP_MODE_EVERY_RETURN || r.mode == KZ_LOCKOP_MODE_RESULT) &&
         ((r.flags & KZ_LOCKOP_FLAG_PRIMITIVE) != 0 || std::any_of(m.lock_ops.begin(), m.lock_ops.end(), [&](const LockOpFact& a) {
            return a.kind == KZ_LOCKOP_KIND_ACQUIRE && (a.mode == KZ_LOCKOP_MODE_EVERY_RETURN || a.mode == KZ_LOCKOP_MODE_RESULT) && !a.may.empty() && a.may == r.may;
          }))))
      r.flags |= KZ_LOCKOP_FLAG_VOCABULARY;
  m.lock_ids = d.ids;
  m.locksets = d.rows;
  m.lock_paths = d.paths;
  for (LockSetFact& r : m.locksets) {
    if (!r.held.empty()) {
      std::vector<ExprRef> h;
      for (uint32_t x : r.held) h.push_back(m.exprs.constant(x));
      r.held_expr = m.exprs.list(KZ_EXPR_OP_SEQ, h);
    }
    if (!r.openers.empty()) {
      std::vector<ExprRef> o;
      for (const std::string& ck : r.openers) {
        auto it = m.resolved.call_site.find(ck);
        o.push_back(m.exprs.constant(it != m.resolved.call_site.end() ? it->second : static_cast<int64_t>(kNone)));
      }
      r.opener_expr = m.exprs.list(KZ_EXPR_OP_SEQ, o);
    }
  }
  m.lock_stats = d.stats;
  m.lock_stats.fields = static_cast<uint32_t>(fields.rows.size());
  m.lock_stats.completeness_targets = fields.targets;
  m.field_status.clear();
  for (const auto& kv : fields.rows) m.field_status.push_back(kv.second);

  // ---- AWAIT_L1
  Prog p(m);
  p.build();
  std::map<std::string, std::string> parent;
  for (const LoopFact& l : m.loops) parent[l.key] = l.parent_key;
  auto within = [&](std::string key, const std::string& outer) {
    // the parent chain of a loop has at most as many links as there are loops
    for (size_t guard = 0; !key.empty() && guard <= m.loops.size(); ++guard) {
      if (key == outer) return true;
      auto it = parent.find(key);
      if (it == parent.end()) return false;
      key = it->second;
    }
    return false;
  };
  std::map<std::string, std::vector<const ReadFact*>> reads_of;
  for (const ReadFact& r : m.reads) reads_of[r.func_key].push_back(&r);
  std::map<std::pair<std::string, std::string>, std::set<std::string>> loops_at;  // (function, position) -> loops of a write or call there
  for (const WriteEvent& w : m.write_events)
    if (!w.loop_key.empty()) loops_at[{w.func_key, w.pos.str()}].insert(w.loop_key);
  for (const CallFact& c : m.calls)
    if (!c.loop_key.empty()) loops_at[{c.caller_key, c.pos.str()}].insert(c.loop_key);
  std::map<std::string, const LockSetFact*> ls_of;
  for (const LockSetFact& r : m.locksets) ls_of[(r.kind == KZ_LOCKSET_KIND_CALL ? "C" : r.kind == KZ_LOCKSET_KIND_READ ? "R" : "W") + r.ref] = &r;
  auto closure = [&](const std::set<std::string>& roots) {
    std::set<std::string> fs(roots.begin(), roots.end());
    std::queue<std::string> q;
    for (const std::string& f : roots) q.push(f);
    while (!q.empty()) {
      const std::string f = q.front();
      q.pop();
      auto it = p.cg.find(f);
      if (it == p.cg.end()) continue;
      for (const std::string& h : it->second)
        if (fs.insert(h).second) q.push(h);
    }
    return fs;
  };
  for (const LoopFact& l : m.loops) {
    if (l.kind != KZ_LOOP_KIND_POLL) continue;
    AwaitFact a;
    a.loop_key = l.key;
    a.func_key = l.func_key;
    std::set<Loc> direct, via;
    std::set<std::string> pcallees;
    pred_reads(l.pred != nullptr ? l.pred : l.cond, &direct, &pcallees);
    // the calls of the loop, and those of the predicate's functions
    std::set<std::string> roots, pred_fns;
    std::vector<std::string> pred_calls;
    bool unresolved = false;
    for (size_t ci = 0; ci < m.calls.size(); ++ci) {
      const CallFact& c = m.calls[ci];
      if (c.caller_key != l.func_key || c.loop_key.empty() || !within(c.loop_key, l.key)) continue;
      for (const std::string& t : p.targets(ci)) roots.insert(t);
      if (p.unresolved(ci)) unresolved = true;
      if (!c.callee_key.empty() && pcallees.count(c.callee_name) != 0) {
        pred_fns.insert(c.callee_key);
        pred_calls.push_back(key_of(c));
      }
    }
    for (const std::string& f : closure(pred_fns)) {
      auto it = reads_of.find(f);
      if (it == reads_of.end()) continue;
      for (const ReadFact* r : it->second) {
        const Loc x{r->type, r->field};
        if (direct.count(x) == 0) via.insert(x);
      }
    }
    std::set<Loc> relevant = direct;
    relevant.insert(via.begin(), via.end());
    for (const Loc& x : relevant) a.relevant.push_back(x.type + "." + x.field);
    if (!relevant.empty()) {
      std::vector<ExprRef> rl;
      for (const Loc& x : relevant)
        rl.push_back(x.type == "<global>" ? m.exprs.load(KZ_EXPR_REF_GLOBAL, x.field) : m.exprs.load(KZ_EXPR_REF_FIELD, x.type + "." + x.field));
      a.relevant_expr = m.exprs.list(KZ_EXPR_OP_SEQ, rl);
    }
    for (const Loc& x : via) a.via_callee.push_back(x.type + "." + x.field);
    if (!via.empty()) a.flags |= KZ_AWAIT_FLAG_VIA_CALLEE;
    // L1 (i)
    const std::set<std::string> fs = closure(roots);
    for (const std::string& f : fs) {
      auto cit = p.calls_of.find(f);
      if (cit == p.calls_of.end()) continue;
      for (size_t ci : cit->second)
        if (p.unresolved(ci)) unresolved = true;
    }
    if (unresolved) a.flags |= KZ_AWAIT_FLAG_UNRESOLVED_CALL;
    std::vector<std::string> written, open_field, open_type;
    for (const Loc& x : relevant) {
      const std::string name = x.type + "." + x.field;
      if (x.type == "<global>") {
        a.flags |= KZ_AWAIT_FLAG_GLOBAL_VARIABLE;
        open_field.push_back(name + " is a global variable (no writer-set rule)");
        open_type.push_back(open_field.back());
        continue;
      }
      const FieldStatusFact* fsr = fields.find(x.type, x.field);
      if (fsr == nullptr) {
        open_field.push_back(name + " has no FIELD_STATUS row");
        open_type.push_back(open_field.back());
        continue;
      }
      if (fsr->status_field != KZ_LOC_STATUS_COMPLETE) open_field.push_back(name + " is not complete (per-field rule)");
      if (fsr->status_type != KZ_LOC_STATUS_COMPLETE) open_type.push_back(name + " is not complete (type-level rule)");
      auto wit = fields.writers.find({x.type, x.field});
      if (wit == fields.writers.end()) continue;
      for (const FieldWriter& w : wit->second) {
        bool hit = fs.count(w.func_key) != 0;
        if (!hit && w.func_key == l.func_key) {
          std::set<std::string> ls;
          if (w.kind == KZ_WRITER_KIND_OUTPTR) {
            auto c = p.call_index.find(w.call_key);
            if (c != p.call_index.end() && !m.calls[c->second].loop_key.empty()) ls.insert(m.calls[c->second].loop_key);
          } else {
            auto it = loops_at.find({w.func_key, w.pos.str()});
            if (it != loops_at.end()) ls = it->second;
          }
          for (const std::string& k : ls)
            if (within(k, l.key)) hit = true;
        }
        if (hit) written.push_back(name + " by " + p.name(w.func_key) + " at " + w.pos.str());
      }
    }
    std::sort(written.begin(), written.end());
    written.erase(std::unique(written.begin(), written.end()), written.end());
    if (unresolved) {
      open_field.push_back("the body reaches an unresolved indirect call");
      open_type.push_back(open_field.back());
    }
    a.l1_mod_field = !written.empty() ? KZ_AWAIT_MOD_FAILS : !open_field.empty() ? KZ_AWAIT_MOD_UNDECIDED : KZ_AWAIT_MOD_HOLDS;
    a.l1_mod_type = !written.empty() ? KZ_AWAIT_MOD_FAILS : !open_type.empty() ? KZ_AWAIT_MOD_UNDECIDED : KZ_AWAIT_MOD_HOLDS;
    // L1 (ii): one lock acquired at one call over the predicate's reads and calls in the body
    std::vector<const LockSetFact*> test_rows;
    for (const ReadFact* r : reads_of[l.func_key]) {
      if (r->loop_key.empty() || !within(r->loop_key, l.key)) continue;
      if (direct.count(Loc{r->type, r->field}) == 0) continue;
      auto it = ls_of.find("R" + r->key());
      if (it != ls_of.end()) test_rows.push_back(it->second);
    }
    for (const std::string& ck : pred_calls) {
      auto it = ls_of.find("C" + ck);
      if (it != ls_of.end()) test_rows.push_back(it->second);
    }
    uint32_t lock = kNone;
    if (!test_rows.empty()) {
      std::set<uint32_t> common(test_rows.front()->held.begin(), test_rows.front()->held.end());
      std::set<std::string> openers(test_rows.front()->openers.begin(), test_rows.front()->openers.end());
      for (const LockSetFact* e : test_rows) {
        std::set<uint32_t> h(e->held.begin(), e->held.end()), keep;
        for (uint32_t x : common)
          if (h.count(x) != 0) keep.insert(x);
        common = keep;
        std::set<std::string> o(e->openers.begin(), e->openers.end()), ko;
        for (const std::string& x : openers)
          if (o.count(x) != 0) ko.insert(x);
        openers = ko;
        if (e->flags & KZ_LOCKSET_FLAG_FAILURE_PATH) common.clear();
      }
      if (!common.empty() && openers.size() == 1) lock = *common.begin();
    }
    if (lock != kNone) {
      a.l1_read = KZ_AWAIT_READ_ONE_LOCK;
      a.lock = lock;
      if (lock < m.lock_ids.size() && m.lock_ids[lock].status == KZ_LOCKID_STATUS_STATIC) a.flags |= KZ_AWAIT_FLAG_LOCK_STATIC;
    } else if (l.lock_kind == KZ_LOCK_KIND_ATOMIC_READ) {
      a.l1_read = KZ_AWAIT_READ_ATOMIC;
    } else {
      a.l1_read = KZ_AWAIT_READ_FAILS;
    }
    auto decide = [&](uint8_t mod, const std::vector<std::string>& open, uint8_t* cert, std::string* note) {
      std::string why;
      if (l.pred_form == KZ_PRED_FORM_NONE) {
        why = "no recognised predicate (" + (l.pred_note.empty() ? std::string("no condition exit") : l.pred_note) + ")";
        a.flags |= KZ_AWAIT_FLAG_NO_PREDICATE;
      } else if (a.l1_read == KZ_AWAIT_READ_FAILS) {
        why = "L1 (ii) fails: the test reads and the predicate's calls do not hold one lock acquired at one call, nor are they one atomic read";
      } else if (mod == KZ_AWAIT_MOD_FAILS) {
        why = "L1 (i) fails: the body writes " + written.front() + (written.size() > 1 ? " (and " + std::to_string(written.size() - 1) + " more)" : "");
      } else if (mod == KZ_AWAIT_MOD_UNDECIDED) {
        why = "L1 (i) undecided: " + open.front() + (open.size() > 1 ? " (and " + std::to_string(open.size() - 1) + " more)" : "");
      }
      if (why.empty()) {
        *cert = KZ_LOOP_CERT_AWAIT;
        *note = "L1 (i) and (ii) hold on the extracted facts (relevant locations: " + std::to_string(relevant.size()) + ", functions called from the body: " +
                std::to_string(fs.size()) + "); lemma L1 itself is not decided here";
      } else {
        *cert = KZ_LOOP_CERT_STAR;
        *note = why;
      }
    };
    decide(a.l1_mod_field, open_field, &a.cert_field, &a.note_field);
    decide(a.l1_mod_type, open_type, &a.cert_type, &a.note_type);
    m.awaits.push_back(a);
  }
}

}  // namespace kzs

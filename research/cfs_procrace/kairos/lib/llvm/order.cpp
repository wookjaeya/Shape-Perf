// order.cpp -- see order.hpp.
//
// Every rule below reads the facts container and the relations rows built from it (relations.hpp).  Nothing names a
// framework, a case or a resource type.  The rules are written down in docs/static_facts.md §5p; the revision string
// names them.
#include "order.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include "kz/core/dump.hpp"
#include "kz/core/registry.hpp"
#include "kz/core/sha256.hpp"
#include "p0.hpp"
#include "rel_facts.hpp"
#include "relations.hpp"

namespace kzs {

const char* order_rules_revision() { return "order rules revision 1 (2026-11-16)"; }

namespace {

constexpr uint32_t kN = KZ_NONE32;
// The pair lines a row lists per value (a print bound: every pair is computed and counted; the lines beyond the bound
// are counted on the row's `pairs` line and not written).
constexpr size_t kPairLines = 6;

// The failure class of a status constant: negative, or at least 0xC0000000 as a 32-bit value (the method's S1 rule, the
// same test as plan_gen.cpp's fail_class).
bool fail_class(int64_t v) { return v < 0 || (v >= 0xC0000000LL && v <= 0xFFFFFFFFLL); }

// An event of the program: a call (FACTS index), a read (READS index) or a typed write (LOCKSET index).
struct Ev {
  char k = 0;
  uint32_t i = kN;
  bool operator<(const Ev& o) const { return std::tie(k, i) < std::tie(o.k, o.i); }
  bool operator==(const Ev& o) const { return k == o.k && i == o.i; }
  bool ok() const { return k != 0 && i != kN; }
};

struct Edge {
  std::string id, rule, kind, from, from_actor, to, to_actor, condition, scope, instance;
  std::vector<std::string> premises, notes;
};

// A step of the closure from an event to another event of another thread (HB-lock, HB-await).
struct Jump {
  Ev to;
  uint32_t to_actor = kN;
  size_t edge = 0;
  bool hard = true;
};

// A node of the closure: an event on a thread ('e'), a spawn call on a thread ('s'), the start of a thread ('n').
struct Node {
  char t = 0;
  Ev e;
  uint32_t a = kN;
  bool operator<(const Node& o) const { return std::tie(t, e, a) < std::tie(o.t, o.e, o.a); }
};

struct Pred {
  Node prev;
  size_t edge = 0;
  bool start = false;
};

// The result of one pair: the value and the edges of the path.
struct PairOrder {
  std::string value, why;
  std::vector<size_t> path;
  std::set<std::string> conditions;
};

class Ord : public RelFacts {
 public:
  explicit Ord(const kz::Container& c) : RelFacts(c) {}

  kz::Status run(std::string* text, std::string* tsv) {
    kz::Status s = prepare();
    if (!s.ok()) return s;
    s = relations_rows(c_, RelationsOptions{}, &rel_);
    if (!s.ok()) return s;
    positions();
    spawn_edges();
    script_edges();
    lock_edges();
    waits();
    rows();
    render(text, tsv);
    return kz::Status::success();
  }

 private:
  // ---------------------------------------------------------------- expressions
  std::string ex(uint32_t root, int depth = 0) const {
    if (root == kN) return "-";
    if (root >= exprs_.size()) return "expr#" + std::to_string(root) + "?";
    if (depth > 40) return "...";
    const kz_fact_expr& e = exprs_[root];
    const std::string name = str(e.name);
    const bool leaf = e.a == kN && e.b == kN;
    if (leaf && e.op == KZ_EXPR_OP_CONST) return std::to_string(e.value);
    if (leaf && e.op == KZ_EXPR_OP_LOAD) {
      std::string t = enum_text("KZ_EXPR_REF", e.flags) + ":" + (name.find('\0') == std::string::npos ? name : kz::escape_string(name));
      if (e.flags == KZ_EXPR_REF_PARAM) t += "#" + std::to_string(e.value);
      return t;
    }
    std::string t = "(" + enum_text("KZ_EXPR_OP", e.op);
    if (!name.empty()) t += " " + name;
    if (e.op == KZ_EXPR_OP_CONST || e.op == KZ_EXPR_OP_FORALL || e.op == KZ_EXPR_OP_CURSOR) t += " " + std::to_string(e.value);
    if (e.a != kN) t += " " + ex(e.a, depth + 1);
    if (e.b != kN) t += " " + ex(e.b, depth + 1);
    return t + ")";
  }
  // pre-order visit; a child that does not have a smaller index than its parent is not followed (post-order pool)
  void visit(uint32_t root, const std::function<void(const kz_fact_expr&)>& f) const {
    if (root == kN || root >= exprs_.size()) return;
    const kz_fact_expr& e = exprs_[root];
    f(e);
    if (e.a < root) visit(e.a, f);
    if (e.b < root) visit(e.b, f);
  }
  std::set<std::string> refs(uint32_t root, uint16_t flags) const {
    std::set<std::string> s;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_LOAD && e.flags == flags) s.insert(str(e.name));
    });
    return s;
  }
  std::set<std::string> call_results(uint32_t root) const {
    std::set<std::string> s;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_CALL_RESULT) s.insert(str(e.name));
    });
    return s;
  }
  bool has_const(uint32_t root, int64_t v) const {
    bool hit = false;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_CONST && e.a == kN && e.b == kN && e.value == v) hit = true;
    });
    return hit;
  }
  std::vector<uint32_t> seq(uint32_t root) const {
    std::vector<uint32_t> out;
    for (uint32_t cur = root; cur != kN && cur < exprs_.size();) {
      const kz_fact_expr& e = exprs_[cur];
      if (e.op != KZ_EXPR_OP_SEQ) break;
      if (e.a != kN && e.a < cur) out.push_back(e.a);
      if (e.b != kN && e.b >= cur) break;
      cur = e.b;
    }
    return out;
  }
  // the disjuncts of a condition: an `or` chain, and a negated `and` as the negated parts (`not (and a b)` is
  // `not a or not b`); `neg` marks a disjunct that is the negation of its node
  struct Dj {
    uint32_t n = kN;
    bool neg = false;
  };
  void disjuncts(uint32_t root, std::vector<Dj>* out, bool neg = false) const {
    if (root == kN || root >= exprs_.size()) return;
    const kz_fact_expr& e = exprs_[root];
    if (e.op == KZ_EXPR_OP_NOT && e.a < root) {
      disjuncts(e.a, out, !neg);
      return;
    }
    if (e.op == (neg ? KZ_EXPR_OP_AND : KZ_EXPR_OP_OR) && e.a < root && e.b < root) {
      disjuncts(e.a, out, neg);
      disjuncts(e.b, out, neg);
      return;
    }
    out->push_back(Dj{root, neg});
  }
  std::string dj_text(const Dj& d) const { return d.neg ? "(not " + ex(d.n) + ")" : ex(d.n); }
  // a condition under values of fields and call results; -1 unknown, 0 false, 1 true
  int eval(uint32_t root, const std::map<std::string, int64_t>& fields, const std::map<std::string, int64_t>& calls, int64_t* value = nullptr) const {
    if (root == kN || root >= exprs_.size()) return -1;
    const kz_fact_expr& e = exprs_[root];
    if ((e.a != kN && e.a >= root) || (e.b != kN && e.b >= root)) return -1;
    int64_t a = 0, b = 0;
    auto num = [&](uint32_t r, int64_t* v) { return eval(r, fields, calls, v) >= 0; };
    auto ret = [&](int64_t v) {
      if (value != nullptr) *value = v;
      return v != 0 ? 1 : 0;
    };
    switch (e.op) {
      case KZ_EXPR_OP_CONST:
        return ret(e.value);
      case KZ_EXPR_OP_LOAD: {
        if (e.flags != KZ_EXPR_REF_FIELD) return -1;
        auto it = fields.find(str(e.name));
        return it == fields.end() ? -1 : ret(it->second);
      }
      case KZ_EXPR_OP_CALL_RESULT: {
        auto it = calls.find(str(e.name));
        return it == calls.end() ? -1 : ret(it->second);
      }
      case KZ_EXPR_OP_NOT: {
        const int x = eval(e.a, fields, calls);
        return x < 0 ? -1 : ret(x == 0);
      }
      case KZ_EXPR_OP_AND:
      case KZ_EXPR_OP_OR: {
        const int x = eval(e.a, fields, calls), y = eval(e.b, fields, calls);
        if (e.op == KZ_EXPR_OP_AND) {
          if (x == 0 || y == 0) return ret(0);
          if (x == 1 && y == 1) return ret(1);
          return -1;
        }
        if (x == 1 || y == 1) return ret(1);
        if (x == 0 && y == 0) return ret(0);
        return -1;
      }
      case KZ_EXPR_OP_EQ:
      case KZ_EXPR_OP_NE:
      case KZ_EXPR_OP_LT:
      case KZ_EXPR_OP_LE:
      case KZ_EXPR_OP_GT:
      case KZ_EXPR_OP_GE:
        if (!num(e.a, &a) || !num(e.b, &b)) return -1;
        switch (e.op) {
          case KZ_EXPR_OP_EQ: return ret(a == b);
          case KZ_EXPR_OP_NE: return ret(a != b);
          case KZ_EXPR_OP_LT: return ret(a < b);
          case KZ_EXPR_OP_LE: return ret(a <= b);
          case KZ_EXPR_OP_GT: return ret(a > b);
          default: return ret(a >= b);
        }
      default:
        return -1;
    }
  }

  // ---------------------------------------------------------------- events
  void positions() {
    for (size_t i = 0; i < calls_.size(); ++i) call_at_.emplace(call_pos(static_cast<uint32_t>(i)), static_cast<uint32_t>(i));
    for (size_t i = 0; i < lockset_.size(); ++i)
      if (lockset_[i].kind == KZ_LOCKSET_KIND_WRITE) write_at_.emplace(pos(str(lockset_[i].file), lockset_[i].line, lockset_[i].col), static_cast<uint32_t>(i));
    for (size_t i = 0; i < reads_.size(); ++i) read_at_.emplace(pos(str(reads_[i].file), reads_[i].line, reads_[i].col), static_cast<uint32_t>(i));
    for (const kz_fact_writer& w : writers_) writer_guard_[pos(str(w.file), w.line, w.col)] = w.guard_root;
    for (size_t i = 0; i < calls_.size(); ++i)
      if (calls_[i].caller_fn < funcs_.size()) {
        auto se = spawn_children(static_cast<uint32_t>(i));
        if (!se.empty()) spawns_of_fn_[calls_[i].caller_fn].push_back(static_cast<uint32_t>(i));
      }
  }
  uint32_t ev_fn(const Ev& e) const {
    if (e.k == 'c' && e.i < calls_.size()) return calls_[e.i].caller_fn;
    if (e.k == 'r' && e.i < reads_.size()) return reads_[e.i].fn;
    if (e.k == 'w' && e.i < lockset_.size()) return lockset_[e.i].fn;
    return kN;
  }
  // the LOCKSET row of an event (kN: none)
  uint32_t ev_row(const Ev& e) const {
    if (e.k == 'w') return e.i;
    if (e.k == 'c' && e.i < calls_.size()) {
      auto it = lockset_call_.find(calls_[e.i].site);
      return it == lockset_call_.end() ? kN : it->second;
    }
    if (e.k == 'r') {
      auto it = lockset_read_.find(e.i);
      return it == lockset_read_.end() ? kN : it->second;
    }
    return kN;
  }
  std::string ev_pos(const Ev& e) const {
    if (e.k == 'c' && e.i < calls_.size()) return call_pos(e.i);
    if (e.k == 'r' && e.i < reads_.size()) return pos(str(reads_[e.i].file), reads_[e.i].line, reads_[e.i].col);
    if (e.k == 'w' && e.i < lockset_.size()) return pos(str(lockset_[e.i].file), lockset_[e.i].line, lockset_[e.i].col);
    return "?";
  }
  std::string ev_text(const Ev& e) const {
    if (e.k == 'c' && e.i < calls_.size()) {
      const kz_fact_call& c = calls_[e.i];
      const std::string callee = defined(c.callee_fn) || c.callee_fn != kN ? fname(c.callee_fn) : (str(c.callee).empty() ? "<indirect>" : str(c.callee));
      return "call " + ev_pos(e) + " in " + fname(c.caller_fn) + " -> " + callee;
    }
    if (e.k == 'r' && e.i < reads_.size()) return "read " + ev_pos(e) + " in " + fname(reads_[e.i].fn) + " [" + str(reads_[e.i].text) + "]";
    if (e.k == 'w' && e.i < lockset_.size())
      return "write " + ev_pos(e) + " in " + fname(lockset_[e.i].fn) + " [" + str(lockset_[e.i].type) + "." + str(lockset_[e.i].field) + "]";
    return "?";
  }
  // a call, write or read at a position as the relations rows write it
  Ev ev_at(const std::string& p, char prefer) const {
    if (prefer == 'w') {
      auto w = write_at_.find(p);
      if (w != write_at_.end()) return Ev{'w', w->second};
    }
    auto c = call_at_.find(p);
    if (c != call_at_.end()) return Ev{'c', c->second};
    auto w = write_at_.find(p);
    if (w != write_at_.end()) return Ev{'w', w->second};
    auto r = read_at_.find(p);
    if (r != read_at_.end()) return Ev{'r', r->second};
    return Ev{};
  }
  std::string actor_text(uint32_t r) const {
    auto it = root_kind_.find(r);
    return fname(r) + " (" + (it == root_kind_.end() ? std::string("?") : it->second) + ")";
  }
  std::string actors_text(const std::set<uint32_t>& as) const {
    std::vector<std::string> v;
    for (uint32_t a : as) v.push_back(actor_text(a));
    return "{" + join(v, ", ") + "}";
  }
  bool in_code(uint32_t fn, uint32_t actor) const {
    auto it = actors_of_.find(fn);
    return it != actors_of_.end() && it->second.count(actor) != 0;
  }

  // ---------------------------------------------------------------- spawn calls
  // the threads a spawn call starts: the actors of its resolved entries (a trampoline stands for its roots)
  std::set<uint32_t> spawn_children(uint32_t ci) const {
    std::set<uint32_t> es;
    auto it = spawn_entries_.find(calls_[ci].site);
    if (it != spawn_entries_.end())
      for (uint32_t e : it->second)
        if (defined(e)) es.insert(e);
    auto im = image_spawn_.find(calls_[ci].site);
    if (im != image_spawn_.end()) es.insert(im->second.begin(), im->second.end());
    std::set<uint32_t> out;
    for (uint32_t r : entry_actors(es))
      if (root_kind_.count(r) != 0) out.insert(r);
    return out;
  }
  bool is_p0(const std::string& name, uint8_t kind) const {
    auto it = p0_kind_.find(name);
    return it != p0_kind_.end() && it->second == kind;
  }
  // the constants a function may return: its RET_CONSTS and those of the functions whose call result it returns,
  // followed; `open` gets the returns not followed
  std::map<int64_t, std::string> consts_of(uint32_t fn, std::set<uint32_t>* seen, std::vector<std::string>* open) const {
    std::map<int64_t, std::string> out;
    if (!defined(fn) || !seen->insert(fn).second) return out;
    auto it = retconsts_of_.find(fn);
    if (it != retconsts_of_.end())
      for (uint32_t i : it->second) out.emplace(retconsts_[i].value, str(retconsts_[i].text));
    auto rit = returns_of_.find(fn);
    if (rit != returns_of_.end())
      for (uint32_t ri : rit->second) {
        const kz_fact_return& r = returns_[ri];
        if (r.vkind == KZ_RET_VALUE_CONST || r.vkind == KZ_RET_VALUE_STRING) continue;
        if (r.vkind == KZ_RET_VALUE_CALL && r.value_root < exprs_.size() && exprs_[r.value_root].op == KZ_EXPR_OP_CALL_RESULT) {
          const std::string callee = str(exprs_[r.value_root].name);
          auto fit = fn_by_name_.find(callee);
          bool any = false;
          if (fit != fn_by_name_.end())
            for (uint32_t g : fit->second)
              if (defined(g)) {
                any = true;
                for (const auto& kv : consts_of(g, seen, open)) out.emplace(kv.first, kv.second);
              }
          if (!any && is_p0(callee, KZ_P0_KIND_THREAD_CREATE)) {
            out.emplace(0, callee + " created the thread (P0 " + callee + ")");  // the primitive's success value
            continue;
          }
          if (!any && open != nullptr) open->push_back(fname(fn) + " returns the result of " + callee + " (no body in these facts)");
          continue;
        }
        if (open != nullptr) open->push_back(fname(fn) + " returns " + enum_text("KZ_RET_VALUE", r.vkind) + " [" + str(r.text) + "] at " + pos(str(r.file), r.line, r.col));
      }
    return out;
  }
  // the success values of what a spawn call calls: a P0 thread-creation primitive returns 0 when it created the thread;
  // a spawn operation with a body, the constants it may return that are no failure
  std::map<int64_t, std::string> success_of(uint32_t ci, std::string* text) const {
    const kz_fact_call& c = calls_[ci];
    std::map<int64_t, std::string> out;
    if (!defined(c.callee_fn)) {
      const std::string callee = str(c.callee);
      if (is_p0(callee, KZ_P0_KIND_THREAD_CREATE)) {
        out[0] = "0";
        *text = callee + " returns 0 when it created the thread (P0 " + callee + ")";
      } else {
        *text = "not determined: " + (callee.empty() ? std::string("<indirect>") : callee) + " has no body in these facts and is no P0 thread creation";
      }
      return out;
    }
    std::set<uint32_t> seen;
    std::vector<std::string> open;
    std::vector<std::string> ok, bad;
    for (const auto& [v, t] : consts_of(c.callee_fn, &seen, &open)) {
      if (fail_class(v)) bad.push_back(std::to_string(v) + " [" + t + "]");
      else {
        ok.push_back(std::to_string(v) + " [" + t + "]");
        out[v] = t;
      }
    }
    if (ok.empty()) *text = "not determined: " + fname(c.callee_fn) + " has no constant return that is not of failure class in these facts";
    else *text = fname(c.callee_fn) + " returns " + join(ok, " or ");
    if (!bad.empty()) *text += "; failure class: " + join(bad, ", ");
    if (!open.empty()) *text += "; returns not followed " + std::to_string(open.size());
    return out;
  }
  bool after(const std::string& fa, uint32_t la, uint32_t ca, const kz_fact_call& c) const {
    const std::string fc = str(c.file);
    return fa == fc && (la > c.line || (la == c.line && ca > c.col));
  }
  // "child without success": whether the spawn operation a spawn call reaches may report a failure after a thread was
  // created -- a return of the operation (or of an operation below it) positioned after its inner spawn call whose value
  // is of failure class, or not followed, and whose condition is not false when the inner call returned its success
  // value.  The source position stands for "after"; a return in another branch is counted (an over-approximation: the
  // flag says "possible", never that it happens).  `chain` gets the creation primitive the operation reaches.
  std::string child_without_success(uint32_t ci, std::set<uint32_t>* seen, std::string* chain) const {
    const kz_fact_call& c = calls_[ci];
    if (!defined(c.callee_fn)) {
      const std::string callee = str(c.callee);
      *chain = callee.empty() ? "<indirect>" : callee;
      if (is_p0(callee, KZ_P0_KIND_THREAD_CREATE)) return "not found: " + callee + " creates no thread when it fails (P0 " + callee + ")";
      return "unknown: " + *chain + " has no body in these facts";
    }
    const uint32_t g = c.callee_fn;
    *chain = fname(g);
    if (!seen->insert(g).second) return "unknown: the spawn operations call each other";
    // the inner spawn calls: the calls of g flagged spawn-op, and the spawn call of every SPAWN_WRAP row of g (an operation
    // the profile-free run does not keep as vocabulary, such as one with a fixed entry, has rows but no flag)
    std::set<uint32_t> inner_set;
    auto cit = calls_of_.find(g);
    if (cit != calls_of_.end())
      for (uint32_t x : cit->second)
        if (calls_[x].flags & KZ_CALL_FLAG_SPAWN_OP) inner_set.insert(x);
    for (const kz_fact_spawnw& w : spawnw_) {
      if (w.fn != g || w.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY) continue;
      auto sit = call_by_site_.find(w.site);
      if (sit != call_by_site_.end() && calls_[sit->second].caller_fn == g) inner_set.insert(sit->second);
    }
    const std::vector<uint32_t> inner(inner_set.begin(), inner_set.end());
    if (inner.empty()) return "unknown: " + fname(g) + " makes no spawn call in its body";
    std::vector<std::string> possible, unknown, inner_text;
    for (uint32_t x : inner) {
      std::string sub_chain;
      const std::string sub = child_without_success(x, seen, &sub_chain);
      // a return under the inner call's failure is a child without success unless the inner call creates no thread when
      // it fails (its own flag is "not found")
      const bool inner_clean = sub.compare(0, 9, "not found") == 0;
      const std::string via = inner_clean ? std::string() : " (the inner call " + call_pos(x) + " may report a failure after its thread started: " + sub.substr(0, sub.find(':')) + ")";
      if (inner_text.empty()) *chain = fname(g) + " -> " + sub_chain;
      std::string stext;
      const std::map<int64_t, std::string> succ = success_of(x, &stext);
      const kz_fact_call& xc = calls_[x];
      const std::string inner_name = defined(xc.callee_fn) ? str(funcs_[xc.callee_fn].name) : str(xc.callee);
      auto rit = returns_of_.find(g);
      if (rit == returns_of_.end()) continue;
      for (uint32_t ri : rit->second) {
        const kz_fact_return& r = returns_[ri];
        const std::string at = pos(str(r.file), r.line, r.col);
        const bool of_inner = r.vkind == KZ_RET_VALUE_CALL && r.value_root < exprs_.size() && exprs_[r.value_root].op == KZ_EXPR_OP_CALL_RESULT &&
                              str(exprs_[r.value_root].name) == inner_name;
        if (of_inner) {
          if (sub.compare(0, 8, "possible") == 0) possible.push_back(fname(g) + " returns the result of " + inner_name + " at " + at + " (" + sub + ")");
          else if (sub.compare(0, 7, "unknown") == 0) unknown.push_back(fname(g) + " returns the result of " + inner_name + " at " + at + " (" + sub + ")");
          continue;
        }
        const bool later = after(str(r.file), r.line, r.col, xc) || (r.loop != kN && r.loop == xc.loop);
        if (!later) continue;
        if (inner_clean && succ.size() == 1 && r.cond_root != kN) {
          std::map<std::string, int64_t> calls{{inner_name, succ.begin()->first}};
          if (eval(r.cond_root, {}, calls) == 0) continue;  // runs only when the inner call failed
        }
        if (r.vkind == KZ_RET_VALUE_CONST) {
          if (fail_class(r.value)) possible.push_back(fname(g) + " returns " + std::to_string(r.value) + " [" + str(r.text) + "] at " + at + " after " + call_pos(x) + via);
          continue;
        }
        if (r.vkind == KZ_RET_VALUE_CALL && r.value_root < exprs_.size() && exprs_[r.value_root].op == KZ_EXPR_OP_CALL_RESULT) {
          std::set<uint32_t> s2;
          std::vector<std::string> o2;
          bool fails = false;
          auto fit = fn_by_name_.find(str(exprs_[r.value_root].name));
          if (fit != fn_by_name_.end())
            for (uint32_t h : fit->second)
              for (const auto& kv : consts_of(h, &s2, &o2))
                if (fail_class(kv.first)) fails = true;
          if (fails) possible.push_back(fname(g) + " returns the result of " + str(exprs_[r.value_root].name) + " at " + at + " after " + call_pos(x) + " (it may return a failure-class constant)" + via);
          else if (!o2.empty() || fit == fn_by_name_.end()) unknown.push_back(fname(g) + " returns the result of " + str(exprs_[r.value_root].name) + " at " + at + " (not followed)");
          continue;
        }
        unknown.push_back(fname(g) + " returns " + enum_text("KZ_RET_VALUE", r.vkind) + " [" + str(r.text) + "] at " + at + " after " + call_pos(x) + " (not followed)");
      }
      inner_text.push_back(sub);
    }
    auto first = [](const std::vector<std::string>& v) {
      std::string t;
      for (size_t i = 0; i < v.size() && i < 3; ++i) t += (i == 0 ? "" : "; ") + v[i];
      if (v.size() > 3) t += "; and " + std::to_string(v.size() - 3) + " more";
      return t;
    };
    if (!possible.empty()) return "possible: " + first(possible);
    if (!unknown.empty()) return "unknown: " + first(unknown);
    return "not found: no return of failure class after the creation call in " + fname(g) + (inner_text.empty() ? std::string() : " (below: " + inner_text.front() + ")");
  }

  // ---------------------------------------------------------------- edges
  size_t add_edge(Edge e) {
    std::string base = e.rule + "|" + e.from + "|" + e.from_actor + "|" + e.to + "|" + e.to_actor;
    e.id = "E" + kz::to_hex(kz::sha256(base)).substr(0, 16);
    auto it = edge_by_id_.find(e.id);
    if (it != edge_by_id_.end()) return it->second;
    edges_.push_back(std::move(e));
    edge_by_id_[edges_.back().id] = edges_.size() - 1;
    return edges_.size() - 1;
  }
  void spawn_edges() {
    for (size_t i = 0; i < calls_.size(); ++i) {
      const uint32_t ci = static_cast<uint32_t>(i);
      const std::set<uint32_t> kids = spawn_children(ci);
      if (kids.empty()) continue;
      std::set<uint32_t> seen;
      std::string chain, stext;
      const std::string flag = child_without_success(ci, &seen, &chain);
      (void)success_of(ci, &stext);
      ++n_spawn_calls_;
      if (flag.compare(0, 8, "possible") == 0) ++n_cws_possible_;
      else if (flag.compare(0, 9, "not found") == 0) ++n_cws_none_;
      else ++n_cws_unknown_;
      flag_of_[ci] = flag;
      status_of_[ci] = stext;
      const kz_fact_call& c = calls_[ci];
      for (uint32_t r : kids) {
        Edge e;
        e.rule = "HB-spawn";
        e.kind = "hard";
        e.from = "spawn call " + call_pos(ci) + " in " + fname(c.caller_fn) + " -> " + (defined(c.callee_fn) ? fname(c.callee_fn) : str(c.callee));
        e.from_actor = actors_text(actor_set(c.caller_fn));
        e.to = "the start of the thread of " + fname(r);
        e.to_actor = actor_text(r);
        e.condition = "a thread of " + fname(r) + " is created by this call (" + stext + ")";
        e.premises.push_back("P0 thread creation: the new thread starts after the creation call has begun, and what the creator did before the call is visible to it (POSIX.1-2017 pthread_create; XBD 4.12 memory synchronization)");
        for (const std::string& p : primitive_names(chain))
          if (p0_unverified_.count(p) != 0) e.premises.push_back("P0-unverified(" + p + ")");
        e.scope = "one call at " + call_pos(ci) + " and the thread it creates";
        e.instance = "the call instance at " + call_pos(ci) + " (its call link) and the thread it creates (the creation link)";
        e.notes.push_back("create primitive: " + chain);
        e.notes.push_back("status condition: " + stext);
        e.notes.push_back("child without success: " + flag);
        spawn_edge_[{ci, r}] = add_edge(std::move(e));
      }
    }
  }
  std::vector<std::string> primitive_names(const std::string& chain) const {
    std::vector<std::string> out;
    const size_t at = chain.rfind(" -> ");
    out.push_back(at == std::string::npos ? chain : chain.substr(at + 4));
    return out;
  }
  // L0-script and the iterations of a spawning loop: the spawn calls of a startup script in the order of its lines, of
  // a table walk in the order of its iterations; the children's starts are observed-only
  void script_edges() {
    std::map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> lines;  // spawn site -> (line, entry)
    for (const kz_fact_spawnw& w : spawnw_) {
      if (w.kind != KZ_SPAWNW_KIND_SCRIPT_ENTRY || !defined(w.entry_fn)) continue;
      const std::string via = str(w.via);
      const std::string pre = "startup.entry[";
      if (via.compare(0, pre.size(), pre) != 0) continue;
      lines[w.site].push_back({static_cast<uint32_t>(std::strtoul(via.c_str() + pre.size(), nullptr, 10)), w.entry_fn});
      line_text_[{w.site, w.entry_fn}] = via.substr(0, via.find(']') + 1);
    }
    for (auto& [site, v] : lines) {
      std::sort(v.begin(), v.end());
      auto cit = call_by_site_.find(site);
      if (cit == call_by_site_.end()) continue;
      const uint32_t ci = cit->second;
      for (size_t k = 0; k + 1 < v.size(); ++k) {
        Edge e;
        e.rule = "L0-script";
        e.kind = "hard";
        e.from = "spawn call " + call_pos(ci) + " for startup.entry[" + std::to_string(v[k].first) + "] (" + fname(v[k].second) + ")";
        e.to = "spawn call " + call_pos(ci) + " for startup.entry[" + std::to_string(v[k + 1].first) + "] (" + fname(v[k + 1].second) + ")";
        e.from_actor = e.to_actor = actors_text(actor_set(calls_[ci].caller_fn));
        e.premises.push_back("L0: the loader reads the startup script's lines in file order on one thread, and the spawn call of a line returns before the next line is read (the layer L0 of the relations rows; the loader's code is not read for it in this revision)");
        e.premises.push_back("the line order is the basis's startup.entry[k] order");
        e.scope = "one run of the startup script";
        e.instance = "the loader's processing of startup.entry[" + std::to_string(v[k].first) + "] and then of startup.entry[" + std::to_string(v[k + 1].first) + "]";
        const size_t id = add_edge(std::move(e));
        line_edges_[site].push_back(id);
        observed_start(ci, v[k].second, v[k + 1].second, "L0-script", "startup.entry[" + std::to_string(v[k].first) + "] before startup.entry[" + std::to_string(v[k + 1].first) + "]");
      }
    }
    for (const auto& [site, by_entry] : iter_spawn_) {
      auto cit = call_by_site_.find(site);
      if (cit == call_by_site_.end()) continue;
      const uint32_t ci = cit->second;
      std::vector<std::pair<uint32_t, uint32_t>> v;
      for (const auto& [entry, iter] : by_entry) v.push_back({iter, entry});
      std::sort(v.begin(), v.end());
      for (size_t k = 0; k + 1 < v.size(); ++k) {
        Edge e;
        e.rule = "HB-seq";
        e.kind = "hard";
        e.from = "spawn call " + call_pos(ci) + " in iteration " + std::to_string(v[k].first) + " (" + fname(v[k].second) + ")";
        e.to = "spawn call " + call_pos(ci) + " in iteration " + std::to_string(v[k + 1].first) + " (" + fname(v[k + 1].second) + ")";
        e.from_actor = e.to_actor = actors_text(actor_set(calls_[ci].caller_fn));
        e.premises.push_back("program order of one thread: the iterations of one run of the loop follow one another (LOOP_ITERS of the unrolled walk)");
        e.scope = "one run of the loop that walks the table";
        e.instance = "the loop instance and its iterations " + std::to_string(v[k].first) + ", " + std::to_string(v[k + 1].first);
        line_edges_[site].push_back(add_edge(std::move(e)));
        observed_start(ci, v[k].second, v[k + 1].second, "HB-seq", "iteration " + std::to_string(v[k].first) + " before iteration " + std::to_string(v[k + 1].first));
      }
    }
  }
  void observed_start(uint32_t ci, uint32_t a, uint32_t b, const char* rule, const std::string& order) {
    for (uint32_t ra : entry_actors({a}))
      for (uint32_t rb : entry_actors({b})) {
        if (ra == rb) continue;
        Edge e;
        e.rule = rule;
        e.kind = "observed-only";
        e.from = "the start of the thread of " + fname(ra);
        e.to = "the start of the thread of " + fname(rb);
        e.from_actor = actor_text(ra);
        e.to_actor = actor_text(rb);
        e.condition = "none: the two threads are created in this order (" + order + " at " + call_pos(ci) + "); which of them runs its code first is not fixed by any rule of this revision";
        e.scope = "the two threads created at " + call_pos(ci);
        e.instance = "the two creation links of " + call_pos(ci);
        line_edges_[calls_[ci].site].push_back(add_edge(std::move(e)));
      }
  }
  // HB-lock: from the relations path C rows graded proven-impossible
  void lock_edges() {
    static const std::regex wpos(R"(^write after the spawn(?: in \S+ \(through the call \S+\))?: (\S+:\d+:\d+))");
    for (const RelGroup& g : rel_.groups)
      for (const RelRow& row : g.rows) {
        if (row.path != 'C' || row.feasibility_grade != "proven-impossible" || row.call >= calls_.size() || row.read >= reads_.size()) continue;
        const uint32_t ci = row.call;
        const std::set<uint32_t> kids = spawn_children(ci);
        // the static-instance locks held at the spawn and at the read (or at the call through which the child reaches it)
        std::set<uint32_t> at_spawn, at_read;
        auto sl = lockset_call_.find(calls_[ci].site);
        if (sl != lockset_call_.end())
          for (uint32_t x : seq(lockset_[sl->second].held_root))
            if (exprs_[x].value >= 0 && static_cast<uint64_t>(exprs_[x].value) < lockids_.size() && lockids_[exprs_[x].value].status == KZ_LOCKID_STATUS_STATIC) at_spawn.insert(static_cast<uint32_t>(exprs_[x].value));
        std::vector<uint32_t> rrows;
        auto rl = lockset_read_.find(row.read);
        if (rl != lockset_read_.end()) rrows.push_back(rl->second);
        if (row.context_call != kN && row.context_call < calls_.size()) {
          auto cl = lockset_call_.find(calls_[row.context_call].site);
          if (cl != lockset_call_.end()) rrows.push_back(cl->second);
        }
        for (uint32_t rr : rrows)
          for (uint32_t x : seq(lockset_[rr].held_root))
            if (exprs_[x].value >= 0) at_read.insert(static_cast<uint32_t>(exprs_[x].value));
        std::vector<std::string> locks;
        for (uint32_t l : at_spawn)
          if (at_read.count(l) != 0) locks.push_back("#" + std::to_string(l) + " " + str(lockids_[l].text));
        std::string rests;
        const size_t ro = row.layers.find("rests-on(");
        if (ro != std::string::npos) {
          int depth = 0;
          size_t end = ro + 8;
          for (; end < row.layers.size(); ++end) {
            if (row.layers[end] == '(') ++depth;
            if (row.layers[end] == ')' && --depth == 0) break;
          }
          rests = row.layers.substr(ro + 9, end - ro - 9);
        }
        const Ev read{'r', row.read};
        for (const RelLine& l : row.lines) {
          std::smatch m;
          if (l.text.compare(0, 21, "write after the spawn") != 0 || !std::regex_search(l.text, m, wpos)) continue;
          const Ev w = ev_at(m[1].str(), 'w');
          if (!w.ok()) continue;
          for (uint32_t pa : actor_set(row.caller_fn))
            for (uint32_t q : kids) {
              if (pa == q) continue;
              Edge e;
              e.rule = "HB-lock";
              e.kind = "hard";
              e.from = ev_text(w);
              e.from_actor = actor_text(pa);
              e.to = ev_text(read);
              e.to_actor = actor_text(q);
              auto gw = writer_guard_.find(ev_pos(w));
              e.condition = "the write runs: it follows the spawn call " + call_pos(ci) + " in this call of " + fname(row.caller_fn) + (gw != writer_guard_.end() && gw->second != kN ? " under " + ex(gw->second) : std::string()) +
                            "; " + status_of_[ci] + "; child without success: " + flag_of_[ci];
              e.premises.push_back("lock " + (locks.empty() ? std::string("(the relations row's)") : join(locks, ", ")) + " held from one acquisition across the spawn and the write, and held by the read (relations path C, row " +
                                   std::string(1, row.path) + std::to_string(row.number) + ": proven-impossible)");
              if (!rests.empty()) e.premises.push_back("rests on: " + rests);
              e.scope = "one call of " + fname(row.caller_fn) + " and the thread its spawn call " + call_pos(ci) + " creates";
              e.instance = "the acquisition of the lock in that call (held across the spawn and the write) and the reader's acquisition of it";
              jumps_[{w, pa}].push_back(Jump{read, q, add_edge(std::move(e)), true});
            }
        }
      }
  }

  // ---------------------------------------------------------------- waits
  struct Use {
    Ev e;
    std::string what, consequence;
  };
  struct Wait {
    uint32_t loop = kN;
    std::string id, head;
    std::vector<std::string> lines;
    std::vector<Use> uses;
  };
  bool in_loop(uint32_t inner, uint32_t outer) const {
    for (uint32_t l = inner, n = 0; l != kN && l < loops_.size() && n < 1000; l = loops_[l].parent, ++n)
      if (l == outer) return true;
    return false;
  }
  void waits() {
    for (size_t li = 0; li < loops_.size(); ++li) {
      const kz_fact_loop& L = loops_[li];
      if (L.kind != KZ_LOOP_KIND_POLL && L.kind != KZ_LOOP_KIND_COUNT_BOUNDED) continue;
      const uint32_t lx = static_cast<uint32_t>(li);
      Wait w;
      w.loop = lx;
      const std::string at = pos(str(L.file), L.line, L.col);
      w.id = "W" + kz::to_hex(kz::sha256("wait|" + at + "|" + fname(L.fn))).substr(0, 16);
      const std::set<uint32_t> as = actor_set(L.fn);
      w.head = at + " in " + fname(L.fn) + " " + enum_text("KZ_LOOP_KIND", L.kind) + " actors " + actors_text(as);
      ++(L.kind == KZ_LOOP_KIND_POLL ? n_poll_ : n_bounded_);
      // the budget locals: those of FACT_LOOPS budget, or (count-bounded) the locals the exits compare with the trip
      std::set<std::string> budget = refs(L.budget_root, KZ_EXPR_REF_LOCAL);
      auto eit = loopexits_of_.find(lx);
      std::vector<uint32_t> exit_rows;
      if (eit != loopexits_of_.end())
        for (uint32_t x : eit->second)
          if (loopexits_[x].exit != kN && !(loopexits_[x].flags & KZ_LOOPEXIT_FLAG_NEVER)) exit_rows.push_back(x);
      if (budget.empty() && L.kind == KZ_LOOP_KIND_COUNT_BOUNDED)
        for (uint32_t x : exit_rows) {
          std::vector<Dj> ds;
          disjuncts(loopexits_[x].aux_root, &ds);
          for (const Dj& d : ds) {
            const std::set<std::string> loc = refs(d.n, KZ_EXPR_REF_LOCAL);
            if (loc.size() == 1 && refs(d.n, KZ_EXPR_REF_FIELD).empty() && call_results(d.n).empty() && (has_const(d.n, L.trip) || has_const(d.n, L.trip - 1))) budget.insert(*loc.begin());
          }
        }
      std::vector<std::string> bl(budget.begin(), budget.end());
      w.lines.push_back("budget: " + (bl.empty() ? std::string("none found") : "the locals " + join(bl, ", ")) +
                        (L.trip_status != KZ_TRIP_STATUS_UNKNOWN && L.trip > 0 ? "; trip " + std::to_string(L.trip) : std::string()) + delay_text(L));
      // the exits, split into condition and budget parts
      bool budget_falls = false, budget_goto = false, any_budget = false;
      for (uint32_t x : exit_rows) {
        const kz_fact_loopexit& X = loopexits_[x];
        std::vector<Dj> ds;
        disjuncts(X.aux_root, &ds);
        std::vector<std::string> cp, bp;
        for (const Dj& d : ds) {
          const std::set<std::string> loc = refs(d.n, KZ_EXPR_REF_LOCAL);
          bool only_budget = !loc.empty() && refs(d.n, KZ_EXPR_REF_FIELD).empty() && call_results(d.n).empty() && refs(d.n, KZ_EXPR_REF_GLOBAL).empty();
          for (const std::string& l : loc)
            if (budget.count(l) == 0) only_budget = false;
          (only_budget ? bp : cp).push_back(dj_text(d));
        }
        if (X.aux_root == kN) cp.push_back("unconditional");
        if (!bp.empty()) {
          any_budget = true;
          if (X.kind == KZ_LOOPEXIT_KIND_GOTO) budget_goto = true;
          else if (X.kind != KZ_LOOPEXIT_KIND_RETURN) budget_falls = true;
        }
        w.lines.push_back("exit " + std::to_string(X.exit) + " " + enum_text("KZ_LOOPEXIT_KIND", X.kind) + " at " + pos(str(X.file), X.line, X.col) + ": condition part " +
                          (cp.empty() ? std::string("-") : join(cp, " or ")) + "; budget part " + (bp.empty() ? std::string("-") : join(bp, " or ")));
      }
      // the predicate and the awaited locations
      const kz_fact_await* aw = nullptr;
      auto ait = awaits_of_.find(L.fn);
      if (ait != awaits_of_.end())
        for (uint32_t x : ait->second)
          if (awaits_[x].loop == L.site) aw = &awaits_[x];
      const bool recognised = L.pred_root != kN;
      if (recognised) ++n_recognised_;
      w.lines.push_back("predicate: " + (recognised ? enum_text("KZ_PRED_FORM", L.pred_form) + " " + ex(L.pred_root) : "not recognised (" + str(L.cert_note) + ")") +
                        (aw != nullptr ? "; AWAIT_L1 per-field " + enum_text("KZ_LOOP_CERT", aw->cert_field) : std::string("; no AWAIT_L1 row")));
      std::set<std::string> awaited;
      if (recognised) {
        for (const std::string& f : refs(L.pred_root, KZ_EXPR_REF_FIELD)) awaited.insert(f);
      }
      const std::set<std::string> cond_calls = call_results(L.cond_root);
      // the writers of the awaited fields in other threads (LOCKSET write rows)
      std::vector<std::pair<Ev, uint32_t>> writers;
      std::vector<std::string> wtext, wpremise;
      bool complete = !awaited.empty();
      int init_ok = 1;
      std::map<std::string, int64_t> init_vals;
      for (const std::string& f : awaited) {
        auto fs = fstatus_of_.find(f);
        if (fs == fstatus_of_.end() || fstatus_[fs->second].status_field != KZ_LOC_STATUS_COMPLETE) complete = false;
        int64_t iv = 0;
        if (init_value(f, &iv)) init_vals[f] = iv;
        for (size_t r = 0; r < lockset_.size(); ++r) {
          const kz_fact_lockset& s = lockset_[r];
          if (s.kind != KZ_LOCKSET_KIND_WRITE || str(s.type) + "." + str(s.field) != f) continue;
          for (uint32_t pa : actor_set(s.fn))
            if (as.count(pa) == 0) {
              writers.push_back({Ev{'w', static_cast<uint32_t>(r)}, pa});
              wtext.push_back(ev_text(Ev{'w', static_cast<uint32_t>(r)}) + " " + actor_text(pa));
            }
        }
      }
      if (recognised && !awaited.empty()) {
        // the exit condition at the initial values: false means only a write can make the wait leave by it
        const int at_init = init_vals.size() == awaited.size() ? eval(L.cond_root, init_vals, {}) : -1;
        init_ok = at_init == 0 ? 1 : at_init == 1 ? 0 : -1;
        std::vector<std::string> av(awaited.begin(), awaited.end());
        w.lines.push_back("awaited: " + join(av, ", ") + "; writers in other threads " + std::to_string(writers.size()) + (wtext.empty() ? std::string() : ": " + join(wtext, "; ")) +
                          "; writer sets complete under the per-field rule: " + (complete ? "yes" : "no") + "; the condition exit at the initial values: " +
                          (init_ok == 1 ? "false (only a write makes it true)" : init_ok == 0 ? "true (the wait may pass on the initial value)" : "not decided"));
      } else {
        std::vector<std::string> cc(cond_calls.begin(), cond_calls.end());
        w.lines.push_back(std::string("awaited: not named: ") +
                          (recognised ? "the predicate reads no field" : "the condition exits read " + (cc.empty() ? std::string("locals only") : "the results of " + join(cc, ", ") + " (not followed into the calls)") +
                                                                             " and the predicate is not recognised"));
      }
      // the dependent uses: the reads and calls after the loop in its function, the calls of the function
      const std::string budget_consequence = !any_budget ? "no-budget" : budget_goto ? "unknown(a budget exit is a goto)" : budget_falls ? "continue" : "skip";
      const std::string f0 = str(L.file);
      auto after_loop = [&](const std::string& f, uint32_t l, uint32_t c, uint32_t inner) {
        if (in_loop(inner, lx)) return 0;
        return f == f0 && (l > L.line || (l == L.line && c > L.col)) ? 1 : -1;
      };
      for (size_t r = 0; r < reads_.size(); ++r) {
        const kz_fact_read& R = reads_[r];
        if (R.fn != L.fn) continue;
        const int a = after_loop(str(R.file), R.line, R.col, R.loop);
        if (a < 0) continue;
        Use u{Ev{'r', static_cast<uint32_t>(r)}, ev_text(Ev{'r', static_cast<uint32_t>(r)}), a == 0 ? "inside" : budget_consequence};
        if (a == 1 && u.consequence == "continue" && R.guard_root != kN) u.what += " guard " + ex(R.guard_root);
        w.uses.push_back(u);
      }
      auto cit = calls_of_.find(L.fn);
      if (cit != calls_of_.end())
        for (uint32_t x : cit->second) {
          const kz_fact_call& C = calls_[x];
          const int a = after_loop(str(C.file), C.line, C.col, C.loop);
          if (a < 0) continue;
          w.uses.push_back(Use{Ev{'c', x}, ev_text(Ev{'c', x}), a == 0 ? "inside" : budget_consequence});
        }
      if (callers_[L.fn].size() + (indirect_callers_.count(L.fn) != 0 ? indirect_callers_.at(L.fn).size() : 0) > 0) {
        std::vector<uint32_t> in(callers_[L.fn].begin(), callers_[L.fn].end());
        auto ic = indirect_callers_.find(L.fn);
        if (ic != indirect_callers_.end()) in.insert(in.end(), ic->second.begin(), ic->second.end());
        std::sort(in.begin(), in.end());
        in.erase(std::unique(in.begin(), in.end()), in.end());
        for (uint32_t x : in) {
          const uint32_t fl = calls_[x].flags;
          const std::string use = (fl & KZ_CALL_FLAG_RESULT_TESTED) ? "tested" : (fl & KZ_CALL_FLAG_RESULT_USED) ? "used" : "discarded";
          std::string cons;
          if (budget_consequence == "no-budget") cons = "no-budget";
          else if (use == "discarded") cons = "continue";
          else cons = "unknown(the caller " + std::string(use == "tested" ? "tests" : "uses") + " the result; the conditions of its calls are not in these facts)";
          w.uses.push_back(Use{Ev{'c', x}, "the call of " + fname(L.fn) + " " + call_pos(x) + " in " + fname(calls_[x].caller_fn) + " (result " + use + ")", cons});
        }
      }
      for (const Use& u : w.uses) ++use_count_[u.consequence.substr(0, u.consequence.find('('))];
      // HB-await: a write of an awaited field in another thread before each use after the wait on the waiting thread
      if (recognised && !writers.empty() && init_ok != 0) {
        const bool l1 = aw != nullptr && aw->cert_field == KZ_LOOP_CERT_AWAIT;
        for (const auto& [we, wa] : writers)
          for (const Use& u : w.uses) {
            if (u.consequence == "inside" || u.what.compare(0, 12, "the call of ") == 0) continue;
            const bool hard = l1 && complete && writers.size() == 1 && init_ok == 1 && (u.consequence == "skip" || u.consequence == "no-budget");
            for (uint32_t qa : as) {
              Edge e;
              e.rule = "HB-await";
              e.kind = hard ? "hard" : "conditional";
              e.from = ev_text(we);
              e.from_actor = actor_text(wa);
              e.to = ev_text(u.e);
              e.to_actor = actor_text(qa);
              e.condition = "the wait at " + at + " was left by its condition exit, which read this write" + (u.consequence == "continue" ? "; its budget exit also reaches the use (continue)" : "");
              e.premises.push_back("reads-from: the condition exit reads the value this write stored" + std::string(writers.size() == 1 ? " (the only write of the awaited fields in another thread)" : " (one of " + std::to_string(writers.size()) + " writes; which one is not decided)"));
              e.premises.push_back(std::string("the awaited fields' writer sets are ") + (complete ? "complete under the per-field rule of FIELD_STATUS" : "not complete under the per-field rule") );
              e.premises.push_back(std::string("L1 under the per-field rule: ") + (aw != nullptr ? enum_text("KZ_LOOP_CERT", aw->cert_field) : std::string("no AWAIT_L1 row")));
              e.premises.push_back(std::string("the condition exit is false at the initial values: ") + (init_ok == 1 ? "checked" : "not decided"));
              e.scope = "one run of the wait at " + at + " on the waiting thread and the use after it";
              e.instance = "the loop instance at " + at + " and the exit it takes (condition or budget)";
              e.notes.push_back("wait " + w.id + "; budget exit: " + u.consequence);
              jumps_[{we, wa}].push_back(Jump{u.e, qa, add_edge(std::move(e)), hard});
            }
          }
      }
      waits_.push_back(std::move(w));
    }
  }
  std::string delay_text(const kz_fact_loop& L) const {
    if (L.delay_site == kN) return "";
    auto cit = call_by_site_.find(L.delay_site);
    if (cit == call_by_site_.end()) return "";
    const kz_fact_call& c = calls_[cit->second];
    std::string t = "; delay " + call_pos(cit->second) + " -> " + (defined(c.callee_fn) ? fname(c.callee_fn) : str(c.callee));
    auto af = argflow_at_.find({c.site, 0});
    if (af != argflow_at_.end()) t += " arg 0 [" + str(argflow_[af->second].text) + "]" + (argflow_[af->second].src == KZ_ARGSRC_CONST ? " = " + std::to_string(argflow_[af->second].value) : std::string());
    return t;
  }
  bool init_value(const std::string& tf, int64_t* v) const {
    auto it = inits_of_.find(tf);
    if (it == inits_of_.end()) return false;
    std::set<int64_t> vals;
    for (uint32_t i : it->second) {
      if (inits_[i].source == KZ_INIT_SOURCE_NONCONST) return false;
      vals.insert(inits_[i].value);
    }
    if (vals.size() != 1) return false;
    *v = *vals.begin();
    return true;
  }

  // ---------------------------------------------------------------- the closure
  // From an event on a thread: the nodes reached through hard edges (and conditional ones when `cond`), with the edge of
  // each step.  HB-seq steps are made when they are used: an event before a spawn call of its function that cannot reach
  // it afterwards (LOCK_PATHS after-spawn), an event inside a call of its function, the start of a thread before its
  // spawn calls and before the first event of an HB-lock or HB-await edge in its code.
  std::map<Node, Pred> closure(const Ev& start, uint32_t actor, bool cond) {
    std::map<Node, Pred> seen;
    std::vector<Node> todo;
    const Node s{'e', start, actor};
    seen[s] = Pred{s, 0, true};
    todo.push_back(s);
    auto go = [&](const Node& from, const Node& to, size_t edge) {
      if (seen.count(to) != 0) return;
      seen[to] = Pred{from, edge, false};
      todo.push_back(to);
    };
    auto jump = [&](const Node& from, const Ev& e, uint32_t a) {
      auto jt = jumps_.find({e, a});
      if (jt == jumps_.end()) return;
      for (const Jump& j : jt->second)
        if (j.hard || cond) go(from, Node{'e', j.to, j.to_actor}, j.edge);
    };
    while (!todo.empty()) {
      const Node n = todo.back();
      todo.pop_back();
      if (n.t == 'e') {
        const uint32_t fn = ev_fn(n.e), row = ev_row(n.e);
        jump(n, n.e, n.a);
        // before a spawn call of the same function
        auto sp = spawns_of_fn_.find(fn);
        if (sp != spawns_of_fn_.end() && row != kN && !(lockset_[row].flags & (KZ_LOCKSET_FLAG_NO_CFG | KZ_LOCKSET_FLAG_NOT_REACHED)))
          for (uint32_t S : sp->second) {
            if (n.e == Ev{'c', S} || !(calls_[S].flags & KZ_CALL_FLAG_SPAWN_OP)) continue;
            auto as = after_spawn_.find(calls_[S].site);
            if (as != after_spawn_.end() && as->second.count(row) != 0) continue;
            go(n, Node{'s', Ev{'c', S}, n.a}, seq_edge(n.e, S, n.a, "before"));
          }
        // inside a call of its function, on the same thread
        std::vector<uint32_t> in;
        if (fn < callers_.size()) in.insert(in.end(), callers_[fn].begin(), callers_[fn].end());
        auto ic = indirect_callers_.find(fn);
        if (ic != indirect_callers_.end()) in.insert(in.end(), ic->second.begin(), ic->second.end());
        auto cb = callback_callers_.find(fn);
        if (cb != callback_callers_.end()) in.insert(in.end(), cb->second.begin(), cb->second.end());
        for (uint32_t c : in)
          if (in_code(calls_[c].caller_fn, n.a)) go(n, Node{'e', Ev{'c', c}, n.a}, seq_edge(n.e, c, n.a, "inside"));
      } else if (n.t == 's') {
        for (uint32_t r : spawn_children(n.e.i)) {
          auto it = spawn_edge_.find({n.e.i, r});
          if (it != spawn_edge_.end()) go(n, Node{'n', Ev{'f', r}, r}, it->second);
        }
      } else if (n.t == 'n') {
        const uint32_t r = n.a;
        for (const auto& [fn, sites] : spawns_of_fn_)
          if (in_code(fn, r))
            for (uint32_t S : sites) go(n, Node{'s', Ev{'c', S}, r}, entry_edge(r, Ev{'c', S}));
        for (const auto& [key, js] : jumps_)
          if (key.second == r && in_code(ev_fn(key.first), r)) go(n, Node{'e', key.first, r}, entry_edge(r, key.first));
      }
    }
    return seen;
  }
  size_t seq_edge(const Ev& e, uint32_t c, uint32_t actor, const char* how) {
    Edge x;
    x.rule = "HB-seq";
    x.kind = "hard";
    x.from = ev_text(e);
    x.from_actor = x.to_actor = actor_text(actor);
    if (std::string(how) == "before") {
      x.to = "spawn call " + call_pos(c) + " in " + fname(calls_[c].caller_fn);
      x.premises.push_back("program order of one call: LOCK_PATHS after-spawn has no row of the event for this spawn call (no path of the control-flow graph reaches it after the call, back edges included)");
      x.scope = "one call of " + fname(calls_[c].caller_fn);
      x.instance = "the call instance of " + fname(calls_[c].caller_fn);
    } else {
      x.to = "the return of the call " + call_pos(c) + " in " + fname(calls_[c].caller_fn);
      x.premises.push_back("program order of one thread: the event is inside the call (the call graph of these facts)");
      x.scope = "one call at " + call_pos(c);
      x.instance = "the call instance at " + call_pos(c) + " (its call link)";
    }
    return add_edge(std::move(x));
  }
  size_t entry_edge(uint32_t r, const Ev& e) {
    Edge x;
    x.rule = "HB-seq";
    x.kind = "hard";
    x.from = "the start of the thread of " + fname(r);
    x.to = ev_text(e);
    x.from_actor = x.to_actor = actor_text(r);
    x.premises.push_back("program order of one thread: a thread's start precedes every event of its code (the code of the root: the call graph from it, direct calls and resolved indirect targets)");
    x.scope = "the thread of " + fname(r);
    x.instance = "the thread (its creation link) and the event's instance on it";
    return add_edge(std::move(x));
  }
  // the order of one pair (w on thread P, u on thread Q)
  PairOrder pair(const Ev& w, uint32_t P, const Ev& u, uint32_t Q) {
    PairOrder out;
    if (P == Q) {
      out.value = "unknown";
      out.why = "one thread: the order of two events of one thread is not in these facts apart from the spawn calls";
      return out;
    }
    auto found = [&](std::map<Node, Pred>& seen, const Ev& target, uint32_t ta, std::vector<size_t>* path) {
      Node end{};
      bool hit = false;
      const Node exact{'e', target, ta};
      if (seen.count(exact) != 0) {
        end = exact;
        hit = true;
      } else {
        const Node entry{'n', Ev{'f', ta}, ta};
        if (seen.count(entry) != 0 && in_code(ev_fn(target), ta)) {
          path->push_back(entry_edge(ta, target));
          end = entry;
          hit = true;
        }
      }
      if (!hit) return false;
      for (Node n = end; !seen[n].start; n = seen[n].prev) path->push_back(seen[n].edge);
      std::reverse(path->begin(), path->end());
      return true;
    };
    for (int dir = 0; dir < 2; ++dir) {
      const Ev& a = dir == 0 ? w : u;
      const Ev& b = dir == 0 ? u : w;
      const uint32_t pa = dir == 0 ? P : Q, pb = dir == 0 ? Q : P;
      std::map<Node, Pred>& hard = memo(a, pa, false);
      std::vector<size_t> path;
      if (found(hard, b, pb, &path)) {
        out.value = dir == 0 ? "hard_before" : "fixed_early_proved";
        out.path = path;
        return out;
      }
    }
    for (int dir = 0; dir < 2; ++dir) {
      const Ev& a = dir == 0 ? w : u;
      const Ev& b = dir == 0 ? u : w;
      const uint32_t pa = dir == 0 ? P : Q, pb = dir == 0 ? Q : P;
      std::map<Node, Pred>& all = memo(a, pa, true);
      std::vector<size_t> path;
      if (found(all, b, pb, &path)) {
        out.value = dir == 0 ? "conditional_before" : "fixed_early_candidate";
        out.path = path;
        for (size_t e : path)
          if (edges_[e].kind == "conditional") out.conditions.insert(edges_[e].condition);
        return out;
      }
    }
    out.value = "unordered";
    out.why = "no path of hard or conditional edges either way";
    return out;
  }
  std::map<Node, Pred>& memo(const Ev& e, uint32_t a, bool cond) {
    auto key = std::make_tuple(e, a, cond);
    auto it = memo_.find(key);
    if (it != memo_.end()) return it->second;
    return memo_[key] = closure(e, a, cond);
  }

  // ---------------------------------------------------------------- rows
  struct RowOut {
    std::string id, key, value, subject;
    std::vector<std::string> lines;
  };
  void rows() {
    static const std::regex creator(R"((\S+:\d+:\d+) in (\S+) -> )");
    static const std::regex writer(R"( at (\S+:\d+:\d+) in )");
    static const std::regex wpos(R"(^write after the spawn(?: in \S+ \(through the call \S+\))?: (\S+:\d+:\d+))");
    for (const RelGroup& g : rel_.groups)
      for (const RelRow& row : g.rows) {
        RowOut o;
        o.id = std::string(1, row.path) + std::to_string(row.number);
        o.key = row.key;
        // the writes (or creations) and the uses of the row
        std::vector<Ev> ws, us;
        std::vector<std::string> missing;
        std::set<uint32_t> u_actors_override;
        if (row.path == 'A') {
          if (row.call < calls_.size()) us.push_back(Ev{'c', row.call});
          for (const RelLine& l : row.lines) {
            std::smatch m;
            if (l.name != "creator" || !std::regex_search(l.text, m, creator)) continue;
            const Ev e = ev_at(m[1].str(), 'c');
            if (e.ok()) ws.push_back(e);
            else missing.push_back(m[1].str());
          }
        } else if (row.path == 'B') {
          for (uint32_t ci : row.site_calls) us.push_back(Ev{'c', ci});
          for (const RelLine& l : row.lines) {
            std::smatch m;
            if (l.name != "writer" || !std::regex_search(l.text, m, writer)) continue;
            const Ev e = ev_at(m[1].str(), 'w');
            if (e.ok()) ws.push_back(e);
            else missing.push_back(m[1].str());
          }
        } else {
          if (row.read < reads_.size()) us.push_back(Ev{'r', row.read});
          if (row.call < calls_.size() && row.read < reads_.size()) {
            // the child threads of the spawn whose code has the read; else every thread whose code has it
            for (uint32_t q : spawn_children(row.call))
              if (in_code(reads_[row.read].fn, q)) u_actors_override.insert(q);
            if (u_actors_override.empty()) u_actors_override = actor_set(reads_[row.read].fn);
          }
          for (const RelLine& l : row.lines) {
            std::smatch m;
            if (l.text.compare(0, 21, "write after the spawn") != 0 || !std::regex_search(l.text, m, wpos)) continue;
            const Ev e = ev_at(m[1].str(), 'w');
            if (e.ok()) ws.push_back(e);
            else missing.push_back(m[1].str());
          }
        }
        std::sort(ws.begin(), ws.end());
        ws.erase(std::unique(ws.begin(), ws.end()), ws.end());
        // the pairs
        std::map<std::string, int> values;
        std::map<std::string, std::vector<std::string>> by_value;
        std::set<uint32_t> threads;
        for (const Ev& w : ws)
          for (const Ev& u : us) {
            const std::set<uint32_t> pa = actor_set(ev_fn(w));
            const std::set<uint32_t> qa = row.path == 'C' ? u_actors_override : actor_set(ev_fn(u));
            if (pa.empty() || qa.empty()) {
              const std::string why = std::string("no root reaches ") + (pa.empty() ? fname(ev_fn(w)) : fname(ev_fn(u)));
              by_value["unknown"].push_back("pair: " + ev_text(w) + " -> " + ev_text(u) + ": unknown: " + why);
              ++values["unknown"];
              continue;
            }
            for (uint32_t P : pa)
              for (uint32_t Q : qa) {
                if (P == Q && (row.path == 'A' || pa.size() * qa.size() > 1)) continue;  // path A pairs creators of other threads only
                const PairOrder po = pair(w, P, u, Q);
                threads.insert(P);
                threads.insert(Q);
                std::string t = "pair: " + ev_text(w) + " " + actor_text(P) + " -> " + ev_text(u) + " " + actor_text(Q) + ": " + po.value;
                if (!po.why.empty()) t += ": " + po.why;
                if (!po.path.empty()) {
                  std::vector<std::string> ids;
                  for (size_t e : po.path) ids.push_back(edges_[e].id + " " + edges_[e].rule);
                  t += "; path " + join(ids, ", ");
                }
                if (!po.conditions.empty()) t += "; conditions: " + join(std::vector<std::string>(po.conditions.begin(), po.conditions.end()), "; ");
                for (size_t e : po.path)
                  if (edges_[e].rule == "HB-lock" && !edges_[e].condition.empty()) t += "; applies when " + edges_[e].condition;
                if (by_value[po.value].size() < kPairLines)
                  for (size_t e : po.path) used_.insert(e);
                by_value[po.value].push_back(t);
                ++values[po.value];
              }
          }
        for (const auto& [v, ls] : by_value)
          for (size_t i = 0; i < ls.size() && i < kPairLines; ++i) o.lines.push_back(ls[i]);
        if (!values.empty()) {
          std::vector<std::string> vv;
          for (const auto& [v, n] : values) vv.push_back(v + " " + std::to_string(n) + (static_cast<size_t>(n) > kPairLines ? " (listed " + std::to_string(kPairLines) + ")" : ""));
          o.lines.push_back("pairs: " + join(vv, ", "));
        }
        for (const std::string& m : missing) o.lines.push_back("not read: " + m + " is no call, read or write of these facts");
        if (ws.empty() || us.empty()) {
          o.value = "unknown";
          o.lines.insert(o.lines.begin(), std::string("pairs: none: ") + (ws.empty() ? "the row names no write, or no creator in another thread," : "the row names no use") + " that these facts locate");
        } else if (values.empty()) {
          o.value = "unknown";
          o.lines.insert(o.lines.begin(), "pairs: none of two threads");
        } else if (values.size() == 1) {
          o.value = values.begin()->first;
        } else if (values.count("unordered") != 0) {
          o.value = "unordered";
        } else if (values.count("unknown") != 0) {
          o.value = "unknown";
        } else if ((values.count("hard_before") + values.count("conditional_before") != 0) && (values.count("fixed_early_proved") + values.count("fixed_early_candidate") != 0)) {
          o.value = "unknown";
          o.lines.push_back("pairs ordered in different directions");
        } else {
          o.value = values.count("conditional_before") != 0 ? "conditional_before" : values.count("fixed_early_candidate") != 0 ? "fixed_early_candidate" : values.begin()->first;
        }
        // the edges that start the threads of the pairs
        std::vector<std::string> ctx;
        for (uint32_t t : threads)
          for (const auto& [k, id] : spawn_edge_)
            if (k.second == t && (row.path != 'C' || k.first == row.call)) {
              const kz_fact_call& c = calls_[k.first];
              std::string line;
              auto lt = line_text_.find({c.site, t});
              if (lt != line_text_.end()) line = " " + lt->second;
              ctx.push_back(fname(t) + " <- " + edges_[id].id + " HB-spawn " + call_pos(k.first) + line);
              used_.insert(id);
            }
        std::set<uint32_t> sites;
        for (uint32_t t : threads)
          for (const auto& [k, id] : spawn_edge_)
            if (k.second == t && (row.path != 'C' || k.first == row.call)) sites.insert(calls_[k.first].site);
        for (uint32_t s : sites) {
          auto le = line_edges_.find(s);
          if (le == line_edges_.end()) continue;
          for (size_t id : le->second) {
            const Edge& e = edges_[id];
            bool both = false;
            for (uint32_t a : threads)
              for (uint32_t b : threads)
                if (a != b && e.from.find(fname(a)) != std::string::npos && e.to.find(fname(b)) != std::string::npos) both = true;
            if (!both) continue;
            ctx.push_back(e.id + " " + e.rule + " " + e.kind + ": " + e.from + " -> " + e.to);
            used_.insert(id);
          }
        }
        if (!ctx.empty()) o.lines.push_back("threads started by: " + join(ctx, "; "));
        o.lines.push_back("opposite: " + (row.feasibility_grade.empty() ? std::string("-") : row.feasibility_grade) + " (the relations row's feasibility of the other order)");
        ++row_count_[std::string(1, row.path)][o.value];
        rows_.push_back(std::move(o));
      }
  }

  // ---------------------------------------------------------------- text
  void render(std::string* text, std::string* tsv) const {
    std::ostringstream os;
    os << "kz-static order (" << order_rules_revision() << ")\n";
    os << "facts " << kz::to_hex(c_.artifact_hash()) << "\n";
    os << "relations " << relations_rules_revision() << "\n";
    os << "kinds: hard: every execution in which both instances occur has them in this order, under the premises; conditional: only when the condition holds; "
          "observed-only: no rule of this revision orders them, a run record may show an order\n";
    os << "closure: hard_before and fixed_early_proved follow hard edges only; a conditional edge makes the path conditional (conditional_before, "
          "fixed_early_candidate) and is never in the hard closure; observed-only edges are not followed\n";
    os << "values: hard_before, conditional_before (the write or creation before the use), fixed_early_proved, fixed_early_candidate (the use before it), "
          "unordered (no path either way between two threads of resolved roots), unknown (why)\n";
    // the edges: the precomputed ones and the HB-seq steps the rows' listed paths use
    std::vector<size_t> order;
    size_t steps = 0;
    for (size_t i = 0; i < edges_.size(); ++i) {
      const bool step = edges_[i].rule == "HB-seq" && edges_[i].kind == "hard" && edges_[i].from.compare(0, 10, "spawn call") != 0;
      if (step) ++steps;
      if (!step || used_.count(i) != 0) order.push_back(i);
    }
    std::map<std::string, size_t> by_rule, by_kind;
    for (size_t i : order) {
      ++by_rule[edges_[i].rule];
      ++by_kind[edges_[i].kind];
    }
    auto counts = [](const std::map<std::string, size_t>& m) {
      std::vector<std::string> v;
      for (const auto& [k, n] : m) v.push_back(k + " " + std::to_string(n));
      return join(v, ", ");
    };
    os << "summary edges listed " << order.size() << " (" << counts(by_rule) << "; " << counts(by_kind) << "); HB-seq steps the closures followed " << steps
       << " (listed where a listed pair's path uses them)\n";
    os << "listing: a row lists at most " << kPairLines << " pairs per value (a print bound); its `pairs` line counts them all\n";
    os << "summary spawn calls with resolved threads " << n_spawn_calls_ << "; child without success: possible " << n_cws_possible_ << ", not found " << n_cws_none_ << ", unknown " << n_cws_unknown_ << "\n";
    std::map<std::string, size_t> uc(use_count_.begin(), use_count_.end());
    os << "summary waits " << waits_.size() << " (poll " << n_poll_ << ", count-bounded " << n_bounded_ << "; predicate recognised " << n_recognised_ << "); dependent uses " << counts(uc) << "\n";
    for (const auto& [p, m] : row_count_) {
      size_t n = 0;
      std::map<std::string, size_t> mm;
      for (const auto& [v, k] : m) {
        n += static_cast<size_t>(k);
        mm[v] = static_cast<size_t>(k);
      }
      os << "summary rows " << p << " " << n << " (" << counts(mm) << ")\n";
    }
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      return std::tie(edges_[a].rule, edges_[a].from, edges_[a].to, edges_[a].id) < std::tie(edges_[b].rule, edges_[b].from, edges_[b].to, edges_[b].id);
    });
    os << "[edges] " << order.size() << "\n";
    std::ostringstream ts;
    ts << "id\trule\tkind\tfrom\tfrom_actor\tto\tto_actor\tcondition\tpremises\tscope\tinstance_key\n";
    for (size_t i : order) {
      const Edge& e = edges_[i];
      os << "  " << e.id << " " << e.rule << " " << e.kind << "\n";
      os << "      from: " << e.from << " " << e.from_actor << "\n";
      os << "      to: " << e.to << " " << e.to_actor << "\n";
      if (!e.condition.empty()) os << "      condition: " << e.condition << "\n";
      for (const std::string& p : e.premises) os << "      premise: " << p << "\n";
      os << "      scope: " << e.scope << "\n";
      os << "      instance key: " << e.instance << "\n";
      for (const std::string& n : e.notes) os << "      " << n << "\n";
      ts << e.id << "\t" << e.rule << "\t" << e.kind << "\t" << e.from << "\t" << e.from_actor << "\t" << e.to << "\t" << e.to_actor << "\t" << e.condition << "\t" << join(e.premises, "; ") << "\t"
         << e.scope << "\t" << e.instance << "\n";
    }
    os << "[waits] " << waits_.size() << "\n";
    for (const Wait& w : waits_) {
      os << "  " << w.id << " " << w.head << "\n";
      for (const std::string& l : w.lines) os << "      " << l << "\n";
      for (const Use& u : w.uses) os << "      use " << u.what << ": " << u.consequence << "\n";
    }
    os << "[rows] " << rows_.size() << "\n";
    for (const RowOut& r : rows_) {
      os << "  " << r.id << " key " << r.key << " order.static " << r.value << "\n";
      for (const std::string& l : r.lines) os << "      " << l << "\n";
    }
    *text = os.str();
    *tsv = ts.str();
  }

  // ---------------------------------------------------------------- state
  RelReport rel_;
  std::multimap<std::string, uint32_t> call_at_, write_at_, read_at_;
  std::map<std::string, uint32_t> writer_guard_;
  std::map<uint32_t, std::vector<uint32_t>> spawns_of_fn_;
  std::vector<Edge> edges_;
  std::map<std::string, size_t> edge_by_id_;
  std::map<std::pair<uint32_t, uint32_t>, size_t> spawn_edge_;
  std::map<uint32_t, std::vector<size_t>> line_edges_;
  std::map<std::pair<uint32_t, uint32_t>, std::string> line_text_;
  std::map<uint32_t, std::string> flag_of_, status_of_;
  std::map<std::pair<Ev, uint32_t>, std::vector<Jump>> jumps_;
  std::map<std::tuple<Ev, uint32_t, bool>, std::map<Node, Pred>> memo_;
  std::vector<Wait> waits_;
  std::vector<RowOut> rows_;
  std::set<size_t> used_;
  std::map<std::string, int> use_count_;
  std::map<std::string, std::map<std::string, int>> row_count_;
  size_t n_spawn_calls_ = 0, n_cws_possible_ = 0, n_cws_none_ = 0, n_cws_unknown_ = 0, n_poll_ = 0, n_bounded_ = 0, n_recognised_ = 0;
};

}  // namespace

kz::Status order_report(const kz::Container& facts, const OrderOptions& options, std::string* text, std::string* tsv) {
  (void)options;
  if (facts.header().artifact_kind != KZ_ARTIFACT_FACTS) return kz::Status::error(kz::Err::Invalid, "not a facts container");
  Ord o(facts);
  return o.run(text, tsv);
}

}  // namespace kzs

// relations.cpp -- see relations.hpp.
//
// Every rule below reads the facts container only.  Nothing names a framework, a case or a resource
// type: the program's own facts and the P0 tables are the vocabulary.  The rules are written down in
// docs/static_facts.md §5e; the revision string names them.
#include "relations.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

#include "kz/core/dump.hpp"
#include "kz/core/registry.hpp"
#include "kz/core/sha256.hpp"
#include "p0.hpp"

namespace kzs {

const char* relations_rules_revision() { return "relations rules revision 3 (2026-10-30)"; }

namespace {

constexpr uint32_t kN = KZ_NONE32;

// A three-valued constant: known or not.
struct Val {
  bool known = false;
  int64_t v = 0;
};
Val known(int64_t v) { return Val{true, v}; }

// Substitution for evaluating a condition: field loads and parameters with a value, call results with a value (by the
// callee's name; "<indirect>" for an indirect call), calls whose result is known not to be zero (a comparison call that
// does not match: only `== 0` and `!= 0` against it are decided), and nodes with a forced value.
struct Subst {
  std::map<std::string, int64_t> fields;  // "Type.field"
  std::map<uint32_t, int64_t> params;
  std::map<std::string, int64_t> calls;
  std::set<std::string> nonzero_calls;
  std::map<uint32_t, int64_t> forced;  // FACT_EXPRS node -> value
};

class Rel {
 public:
  explicit Rel(const kz::Container& c) : c_(c) {}

  kz::Status run(std::string* out) {
    kz::Status s = load();
    if (!s.ok()) return s;
    index();
    actors();
    std::ostringstream body_a, body_b, body_c;
    path_a(body_a);
    path_b(body_b);
    path_c(body_c);
    std::ostringstream os;
    header(os);
    os << body_a.str() << body_b.str() << body_c.str();
    *out = os.str();
    return kz::Status::success();
  }

 private:
  // ---------------------------------------------------------------- loading
  template <class T>
  kz::Status read(uint32_t id, std::vector<T>* v) {
    return c_.read_section(id, v);
  }
  kz::Status load() {
    for (uint32_t need : {KZ_SEC_READS, KZ_SEC_RETURNS, KZ_SEC_ARGFLOW, KZ_SEC_PARAM_FLOW, KZ_SEC_LOCKSET, KZ_SEC_LOCK_IDS, KZ_SEC_FIELD_STATUS})
      if (c_.find_section(need) == nullptr)
        return kz::Status::error(kz::Err::Invalid,
                                 "the facts have no extended facts (section " + std::to_string(need) +
                                     "): extract them with --no-profile, or with --extended-facts in the profile-driven mode");
    if (c_.find_section(KZ_SEC_LOCK_PATHS) == nullptr)
      return kz::Status::error(kz::Err::Invalid,
                               "the facts have no LOCK_PATHS section (facts schema revision 11, written with the extended facts): extract them again with this kz-static");
    kz::Status s;
    if (!(s = read(KZ_SEC_BASIS, &basis_)).ok()) return s;
    if (!(s = read(KZ_SEC_FUNCS, &funcs_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACTS, &calls_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_EXPRS, &exprs_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_LOOPS, &loops_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_INIT, &inits_)).ok()) return s;
    if (!(s = read(KZ_SEC_WRITERS, &writers_)).ok()) return s;
    if (!(s = read(KZ_SEC_CALL_TARGETS, &targets_)).ok()) return s;
    if (!(s = read(KZ_SEC_TERMINALS, &terminals_)).ok()) return s;
    if (!(s = read(KZ_SEC_LOOP_ITERS, &iters_)).ok()) return s;
    if (!(s = read(KZ_SEC_FUNC_WRITES, &fwrites_)).ok()) return s;
    if (!(s = read(KZ_SEC_P0_TABLE, &p0_)).ok()) return s;
    if (!(s = read(KZ_SEC_OP_DERIV, &opderiv_)).ok()) return s;
    if (!(s = read(KZ_SEC_SPAWN_WRAP, &spawnw_)).ok()) return s;
    if (!(s = read(KZ_SEC_READS, &reads_)).ok()) return s;
    if (!(s = read(KZ_SEC_RETURNS, &returns_)).ok()) return s;
    if (!(s = read(KZ_SEC_RET_CONSTS, &retconsts_)).ok()) return s;
    if (!(s = read(KZ_SEC_LOOP_EXITS, &loopexits_)).ok()) return s;
    if (!(s = read(KZ_SEC_ARGFLOW, &argflow_)).ok()) return s;
    if (!(s = read(KZ_SEC_PARAM_FLOW, &pflow_)).ok()) return s;
    if (!(s = read(KZ_SEC_PTR_FIELDS, &ptrfields_)).ok()) return s;
    if (!(s = read(KZ_SEC_GLOBAL_IMAGES, &gimages_)).ok()) return s;
    if (!(s = read(KZ_SEC_LOCK_IDS, &lockids_)).ok()) return s;
    if (!(s = read(KZ_SEC_LOCKSET, &lockset_)).ok()) return s;
    if (!(s = read(KZ_SEC_FIELD_STATUS, &fstatus_)).ok()) return s;
    if (!(s = read(KZ_SEC_AWAIT_L1, &awaits_)).ok()) return s;
    if (!(s = read(KZ_SEC_LOCK_PATHS, &lockpaths_)).ok()) return s;
    return kz::Status::success();
  }

  std::string str(const kz_str& s) const {
    std::string out;
    c_.string(s, &out);
    return out;
  }
  static std::string base_name(const std::string& key) {
    const size_t at = key.find('@');
    return at == std::string::npos ? key : key.substr(0, at);
  }
  std::string fname(uint32_t fn) const {
    if (fn == kN || fn >= funcs_.size()) return "-";
    if (defined(fn)) note_file(str(funcs_[fn].file));
    std::string n = str(funcs_[fn].name);
    if (funcs_[fn].flags & KZ_FUNC_FLAG_STATIC) n += "@" + str(funcs_[fn].file) + ":" + std::to_string(funcs_[fn].line);
    return n;
  }
  static std::string join(const std::vector<std::string>& v, const char* sep) {
    std::string t;
    for (const std::string& x : v) t += (t.empty() ? "" : sep) + x;
    return t;
  }
  std::string short_fname(uint32_t fn) const { return fn == kN || fn >= funcs_.size() ? "-" : str(funcs_[fn].name); }
  bool defined(uint32_t fn) const { return fn != kN && fn < funcs_.size() && (funcs_[fn].flags & KZ_FUNC_FLAG_DEFINED) != 0; }
  // a position; the file is noted for the row being written (the basis deviations of its unit)
  void note_file(const std::string& f) const {
    if (!f.empty()) row_files_.insert(f);
  }
  std::string pos(const std::string& file, uint32_t line, uint32_t col) const {
    note_file(file);
    std::string p = file + ":" + std::to_string(line);
    if (col != 0) p += ":" + std::to_string(col);
    return p;
  }
  std::string call_pos(uint32_t ci) const { return pos(str(calls_[ci].file), calls_[ci].line, calls_[ci].col); }
  static std::string written_flags(uint32_t f) {
    std::string t;
    if (f & KZ_GIMAGE_FLAG_OBJECT_WRITTEN) t += "object-written";
    if (f & KZ_GIMAGE_FLAG_FIELD_WRITTEN) t += std::string(t.empty() ? "" : ", ") + "field-written";
    if (f & KZ_GIMAGE_FLAG_RECORD_WRITTEN) t += std::string(t.empty() ? "" : ", ") + "record-written";
    return t;
  }
  std::string enum_text(const char* name, uint64_t v) const {
    const kz::EnumDesc* d = kz::find_enum(name);
    if (d == nullptr) return std::to_string(v);
    const char* t = kz::enum_display(*d, v);
    return t != nullptr ? t : "#" + std::to_string(v);
  }
  static bool before(const std::string& fa, uint32_t la, uint32_t ca, const std::string& fb, uint32_t lb, uint32_t cb) {
    return fa == fb && (la < lb || (la == lb && ca < cb));
  }

  // ---------------------------------------------------------------- expressions
  // the text of a tree as `kz-static show` writes it (with its 40-level print limit)
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
  // pre-order visit of a tree.  The writer numbers the nodes post-order (serialize.cpp ExprTable), so a child
  // has a smaller index than its parent; a child that does not is not followed, and every walk ends.
  void visit(uint32_t root, const std::function<void(const kz_fact_expr&)>& f) const {
    if (root == kN || root >= exprs_.size()) return;
    const kz_fact_expr& e = exprs_[root];
    f(e);
    if (e.a < root) visit(e.a, f);
    if (e.b < root) visit(e.b, f);
  }
  bool has_param(uint32_t root, uint32_t k) const {
    bool hit = false;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_LOAD && e.flags == KZ_EXPR_REF_PARAM && static_cast<uint32_t>(e.value) == k) hit = true;
    });
    return hit;
  }
  bool has_field(uint32_t root, const std::string& tf) const {
    bool hit = false;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_LOAD && e.flags == KZ_EXPR_REF_FIELD && str(e.name) == tf) hit = true;
    });
    return hit;
  }
  bool has_call(uint32_t root, const std::string& callee) const {
    bool hit = false;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_CALL_RESULT && str(e.name) == callee) hit = true;
    });
    return hit;
  }
  std::vector<std::string> calls_in(uint32_t root) const {
    std::set<std::string> s;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_CALL_RESULT) s.insert(str(e.name));
    });
    return {s.begin(), s.end()};
  }
  std::set<std::string> fields_in(uint32_t root) const {
    std::set<std::string> s;
    visit(root, [&](const kz_fact_expr& e) {
      if (e.op == KZ_EXPR_OP_LOAD && e.flags == KZ_EXPR_REF_FIELD) s.insert(str(e.name));
    });
    return s;
  }
  std::vector<uint32_t> seq(uint32_t root) const {
    std::vector<uint32_t> out;
    for (uint32_t cur = root; cur != kN && cur < exprs_.size();) {
      const kz_fact_expr& e = exprs_[cur];
      if (e.op != KZ_EXPR_OP_SEQ) break;
      if (e.a != kN && e.a < cur) out.push_back(static_cast<uint32_t>(exprs_[e.a].value));
      if (e.b != kN && e.b >= cur) break;
      cur = e.b;
    }
    return out;
  }
  // a condition or a value under a substitution; unknown where the facts do not decide it
  Val eval(uint32_t root, const Subst& s, std::set<std::string>* unknown) const {
    if (root == kN || root >= exprs_.size()) return Val{};
    const kz_fact_expr& e = exprs_[root];
    if ((e.a != kN && e.a >= root) || (e.b != kN && e.b >= root)) return Val{};  // not post-order: not followed (see visit)
    auto fit = s.forced.find(root);
    if (fit != s.forced.end()) return known(fit->second);
    if ((e.op == KZ_EXPR_OP_EQ || e.op == KZ_EXPR_OP_NE) && !s.nonzero_calls.empty() && e.a != kN && e.b != kN) {
      // a call known not to return zero, compared with zero
      for (int side = 0; side < 2; ++side) {
        const kz_fact_expr& c = exprs_[side == 0 ? e.a : e.b];
        const kz_fact_expr& z = exprs_[side == 0 ? e.b : e.a];
        if (c.op == KZ_EXPR_OP_CALL_RESULT && s.nonzero_calls.count(str(c.name)) != 0 && z.op == KZ_EXPR_OP_CONST && z.a == kN && z.b == kN && z.value == 0)
          return known(e.op == KZ_EXPR_OP_NE ? 1 : 0);
      }
    }
    auto un = [&](const std::string& why) {
      if (unknown != nullptr) unknown->insert(why);
      return Val{};
    };
    switch (e.op) {
      case KZ_EXPR_OP_CONST:
        return known(e.value);
      case KZ_EXPR_OP_LOAD: {
        if (e.flags == KZ_EXPR_REF_FIELD) {
          auto it = s.fields.find(str(e.name));
          if (it != s.fields.end()) return known(it->second);
          return un("the field " + str(e.name));
        }
        if (e.flags == KZ_EXPR_REF_PARAM) {
          auto it = s.params.find(static_cast<uint32_t>(e.value));
          if (it != s.params.end()) return known(it->second);
          return un("the parameter " + str(e.name) + "#" + std::to_string(e.value));
        }
        return un(enum_text("KZ_EXPR_REF", e.flags) + " " + str(e.name));
      }
      case KZ_EXPR_OP_NOT: {
        const Val a = eval(e.a, s, unknown);
        return a.known ? known(a.v == 0) : Val{};
      }
      case KZ_EXPR_OP_AND:
      case KZ_EXPR_OP_OR: {
        const Val a = eval(e.a, s, unknown), b = eval(e.b, s, unknown);
        const bool is_and = e.op == KZ_EXPR_OP_AND;
        if (is_and && ((a.known && a.v == 0) || (b.known && b.v == 0))) return known(0);
        if (!is_and && ((a.known && a.v != 0) || (b.known && b.v != 0))) return known(1);
        if (a.known && b.known) return known(is_and ? 1 : 0);
        return Val{};
      }
      case KZ_EXPR_OP_EQ:
      case KZ_EXPR_OP_NE:
      case KZ_EXPR_OP_LT:
      case KZ_EXPR_OP_LE:
      case KZ_EXPR_OP_GT:
      case KZ_EXPR_OP_GE: {
        const Val a = eval(e.a, s, unknown), b = eval(e.b, s, unknown);
        if (!a.known || !b.known) return Val{};
        switch (e.op) {
          case KZ_EXPR_OP_EQ: return known(a.v == b.v);
          case KZ_EXPR_OP_NE: return known(a.v != b.v);
          case KZ_EXPR_OP_LT: return known(a.v < b.v);
          case KZ_EXPR_OP_LE: return known(a.v <= b.v);
          case KZ_EXPR_OP_GT: return known(a.v > b.v);
          default: return known(a.v >= b.v);
        }
      }
      case KZ_EXPR_OP_CALL_RESULT: {
        auto cit = s.calls.find(str(e.name));
        if (cit != s.calls.end()) return known(cit->second);
        return un("the result of " + str(e.name));
      }
      default:
        return un("a " + enum_text("KZ_EXPR_OP", e.op) + " expression");
    }
  }

  // ---------------------------------------------------------------- indices
  void index() {
    for (const kz_basis_item& b : basis_) basis_map_[str(b.name)] = str(b.value);
    for (const kz_basis_item& b : basis_)
      if (b.policy == KZ_POLICY_DECLARED && b.prov != kN) {
        const std::string n = str(b.name);
        if (n.compare(0, 3, "op.") == 0 || n.compare(0, 7, "target.") == 0) ++hand_entered_vocabulary_;
      }
    profile_free_ = basis_map_.count("extract.mode") != 0 && basis_map_["extract.mode"].compare(0, 12, "profile-free") == 0;
    for (size_t i = 0; i < funcs_.size(); ++i) fn_by_name_[str(funcs_[i].name)].push_back(static_cast<uint32_t>(i));
    succ_.assign(funcs_.size(), {});
    callers_.assign(funcs_.size(), {});
    for (size_t i = 0; i < calls_.size(); ++i) {
      const kz_fact_call& c = calls_[i];
      call_by_site_[c.site] = static_cast<uint32_t>(i);
      if (c.caller_fn < funcs_.size()) calls_of_[c.caller_fn].push_back(static_cast<uint32_t>(i));
      if (c.caller_fn < funcs_.size() && defined(c.callee_fn)) {
        succ_[c.caller_fn].push_back({c.callee_fn, false});
        callers_[c.callee_fn].push_back(static_cast<uint32_t>(i));
      }
    }
    for (const kz_fact_target& t : targets_) {
      auto it = call_by_site_.find(t.site);
      if (it == call_by_site_.end()) continue;
      const uint32_t caller = calls_[it->second].caller_fn;
      if (t.kind == KZ_CALL_TARGET_KIND_INDIRECT && defined(t.fn) && caller < funcs_.size()) {
        succ_[caller].push_back({t.fn, true});
        indirect_callers_[t.fn].push_back(it->second);
        itargets_[t.site].insert(t.fn);
      }
      if (t.kind == KZ_CALL_TARGET_KIND_INDIRECT && !defined(t.fn)) unresolved_sites_.insert(t.site);
      if (t.kind == KZ_CALL_TARGET_KIND_SPAWN_ENTRY) spawn_entries_[t.site].insert(t.fn);
    }
    // an indirect call with no resolved target, or with a target the flow graph does not resolve, may call any function
    for (const kz_fact_call& c : calls_)
      if (c.kind == KZ_CALL_KIND_INDIRECT && itargets_.count(c.site) == 0) unresolved_sites_.insert(c.site);
    for (auto& v : succ_) {
      std::sort(v.begin(), v.end());
      v.erase(std::unique(v.begin(), v.end()), v.end());
    }
    for (const kz_fact_iter& it : iters_)
      if (it.spawn != KZ_ITER_SPAWN_NONE && it.site != kN && defined(it.fn)) {
        spawn_entries_[it.site].insert(it.fn);
        iter_spawn_[it.site][it.fn] = it.iter;
        iter_loop_[it.site] = it.loop;
      }
    for (const kz_fact_spawnw& w : spawnw_)
      if (w.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY) {
        spawn_entries_[w.site].insert(w.entry_fn);
        if (defined(w.entry_fn)) script_entries_.insert(w.entry_fn);
      }
    for (size_t i = 0; i < argflow_.size(); ++i) argflow_at_[{argflow_[i].site, argflow_[i].arg_index}] = static_cast<uint32_t>(i);
    for (size_t i = 0; i < reads_.size(); ++i) reads_of_field_[str(reads_[i].type) + "." + str(reads_[i].field)].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < returns_.size(); ++i) returns_of_[returns_[i].fn].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < retconsts_.size(); ++i) retconsts_of_[retconsts_[i].fn].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < loopexits_.size(); ++i) loopexits_of_[loopexits_[i].loop].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < pflow_.size(); ++i) pflow_of_[{pflow_[i].fn, pflow_[i].param}].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < lockset_.size(); ++i) {
      const kz_fact_lockset& r = lockset_[i];
      if (r.kind == KZ_LOCKSET_KIND_CALL) lockset_call_[r.site] = static_cast<uint32_t>(i);
      else if (r.kind == KZ_LOCKSET_KIND_READ) lockset_read_[r.read] = static_cast<uint32_t>(i);
      else if (r.kind == KZ_LOCKSET_KIND_WRITE) lockset_writes_of_[r.fn].push_back(static_cast<uint32_t>(i));
    }
    for (size_t i = 0; i < fstatus_.size(); ++i) fstatus_of_[str(fstatus_[i].type) + "." + str(fstatus_[i].field)] = static_cast<uint32_t>(i);
    for (size_t i = 0; i < writers_.size(); ++i) writers_of_[str(writers_[i].loc_type) + "." + str(writers_[i].loc_field)].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < inits_.size(); ++i) inits_of_[str(inits_[i].loc_type) + "." + str(inits_[i].loc_field)].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < fwrites_.size(); ++i)
      if (fwrites_[i].kind != KZ_FWRITE_KIND_UNTYPED) fwrites_of_[str(fwrites_[i].type) + "." + str(fwrites_[i].field)].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < terminals_.size(); ++i) terminals_of_[terminals_[i].fn].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < opderiv_.size(); ++i) opderiv_of_[opderiv_[i].fn].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < ptrfields_.size(); ++i)
      if (ptrfields_[i].kind == KZ_PTRF_KIND_READ) ptrread_of_[ptrfields_[i].read] = static_cast<uint32_t>(i);
    for (size_t i = 0; i < gimages_.size(); ++i) gimages_of_[base_name(str(gimages_[i].object))].push_back(static_cast<uint32_t>(i));
    for (size_t i = 0; i < loops_.size(); ++i) loop_by_index_[loops_[i].loop] = static_cast<uint32_t>(i);
    for (size_t i = 0; i < awaits_.size(); ++i) awaits_of_[awaits_[i].fn].push_back(static_cast<uint32_t>(i));
    for (const kz_fact_p0& p : p0_) {
      p0_kind_[str(p.name)] = p.kind;
      if (str(p.source) == "unverified") p0_unverified_.insert(str(p.name));
    }
    for (const P0CompareEntry& e : p0_compare_table()) {
      compare_names_.insert(e.name);
      if (std::string(e.source) == "unverified") p0_unverified_.insert(e.name);
    }
    // the lock primitives of P0 the program calls: the lock sets rest on their semantics
    for (const kz_fact_call& c : calls_) {
      if (defined(c.callee_fn)) continue;
      auto pk = p0_kind_.find(str(c.callee));
      if (pk != p0_kind_.end() && (pk->second == KZ_P0_KIND_MUTEX_ACQUIRE || pk->second == KZ_P0_KIND_MUTEX_RELEASE || pk->second == KZ_P0_KIND_COND_WAIT))
        lock_prims_.insert(pk->first);
    }
    // the P0 primitive a startup-script entry is resolved through (the symbol lookup the spawn operation reaches)
    for (const kz_fact_spawnw& w : spawnw_)
      if (w.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY && defined(w.entry_fn)) {
        auto cit = call_by_site_.find(w.call_site);
        if (cit != call_by_site_.end()) script_prim_[w.entry_fn].insert(str(calls_[cit->second].callee));
      }
    for (const kz_fact_lockpath& p : lockpaths_) {
      if (p.kind == KZ_LOCKPATH_KIND_AFTER_SPAWN) after_spawn_[p.site].insert(p.row);
      else if (p.kind == KZ_LOCKPATH_KIND_ENTRY_RELEASE && entry_release_.count(p.row) == 0) entry_release_[p.row] = p.site;
    }
    // the deviations of the basis the extraction copied (extract.basis_deviation[<item>]); a unit's own deviations
    // (an item whose name has the unit's file in its first [...]) mark the rows that name a position of that file
    const std::string dev = "extract.basis_deviation[";
    for (const auto& [k, v] : basis_map_) {
      if (k.compare(0, dev.size(), dev) != 0 || k.back() != ']') continue;
      const std::string item = k.substr(dev.size(), k.size() - dev.size() - 1);
      deviations_.push_back(item);
      const size_t a = item.find('['), b = a == std::string::npos ? std::string::npos : item.find(']', a);
      if (b != std::string::npos) deviation_files_[item.substr(a + 1, b - a - 1)].insert(item);
    }
    for (const kz_basis_item& b : basis_)
      if (b.policy == KZ_POLICY_DECLARED && str(b.name).compare(0, 7, "assume.") == 0) assumptions_.insert(str(b.name));
    thread_end_code();
    global_address_functions();
    image_entries();
  }

  // The code of the program a thread may run after its entry function has returned: the destructors of the
  // thread-specific keys (P0 key creation with a destructor argument that is not a null constant) and the cleanup
  // handlers (P0 cleanup push; conservatively every push of the program).
  void thread_end_code() {
    for (size_t ci = 0; ci < calls_.size(); ++ci) {
      const kz_fact_call& c = calls_[ci];
      if (defined(c.callee_fn)) continue;
      const std::string callee = str(c.callee);
      for (const kz_fact_p0& p : p0_) {
        if (str(p.name) != callee) continue;
        if (p.kind == KZ_P0_KIND_TSD_KEY_CREATE && p.entry_param != kN) {
          auto af = argflow_at_.find({c.site, p.entry_param});
          const bool null = af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_CONST && argflow_[af->second].value == 0;
          (null ? keys_null_dtor_ : keys_dtor_)
              .push_back(callee + " at " + call_pos(static_cast<uint32_t>(ci)) + (af != argflow_at_.end() ? " (arg " + std::to_string(p.entry_param) + " [" + str(argflow_[af->second].text) + "])" : ""));
        }
        if (p.kind == KZ_P0_KIND_CLEANUP_PUSH) cleanup_pushes_.push_back(callee + " at " + call_pos(static_cast<uint32_t>(ci)));
      }
    }
  }

  // The entries of a spawn call whose entry argument is read from a global table with a static image: the function
  // leaves of that image at the field the argument reads (GLOBAL_IMAGES), for the spawn calls the facts leave without
  // a resolved entry.  The entry argument is the P0 entry parameter of a thread-creation primitive, or the parameter a
  // derived spawn operation passes on (SPAWN_WRAP).  No creation order is read from the image.  The image is the run-time
  // value only when nothing writes the member: the member's writer set must be complete with no writer instance under
  // both completeness rules of FIELD_STATUS (typed, data-function and out-pointer writes; an escape, a cast, a union or
  // an unknown effect makes it not complete), and no leaf used may be marked as written (object-, field- or
  // record-written: a store to the object, the field or a whole record of its type); otherwise the spawn stays
  // unresolved, with the reason.
  void image_entries() {
    std::map<uint32_t, uint32_t> entry_param_of;  // spawn operation (function) -> its entry parameter
    for (const kz_fact_spawnw& w : spawnw_)
      if (w.param != kN && (w.kind == KZ_SPAWNW_KIND_ENTRY_ARG || w.kind == KZ_SPAWNW_KIND_CALLED_BY_ENTRY || w.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY) &&
          entry_param_of.count(w.fn) == 0)
        entry_param_of[w.fn] = w.param;
    for (size_t ci = 0; ci < calls_.size(); ++ci) {
      const kz_fact_call& c = calls_[ci];
      if ((c.flags & KZ_CALL_FLAG_SPAWN_OP) == 0 || !defined(c.caller_fn)) continue;
      auto se = spawn_entries_.find(c.site);
      bool resolved = false;
      if (se != spawn_entries_.end())
        for (uint32_t e : se->second)
          if (defined(e)) resolved = true;
      if (resolved) continue;
      uint32_t k = kN;
      if (defined(c.callee_fn)) {
        auto it = entry_param_of.find(c.callee_fn);
        if (it != entry_param_of.end()) k = it->second;
      } else {
        for (const kz_fact_p0& p : p0_)
          if (str(p.name) == str(c.callee) && p.kind == KZ_P0_KIND_THREAD_CREATE) k = p.entry_param;
      }
      const std::string where = call_pos(static_cast<uint32_t>(ci)) + " in " + fname(c.caller_fn) + " -> " + str(c.callee);
      if (k == kN) {
        unresolved_spawns_.push_back(where + ": no entry parameter of the spawn operation is known");
        continue;
      }
      auto af = argflow_at_.find({c.site, k});
      if (af == argflow_at_.end()) {
        unresolved_spawns_.push_back(where + ": no ARGFLOW row of the entry argument " + std::to_string(k));
        continue;
      }
      const kz_fact_argflow& a = argflow_[af->second];
      const std::string object = base_name(str(a.object)), field = str(a.field), type = str(a.type);
      if (a.src != KZ_ARGSRC_FIELD || object.empty() || field.empty() || (a.base != KZ_ARGBASE_GLOBAL && a.base != KZ_ARGBASE_GLOBAL_ELEMENT)) {
        unresolved_spawns_.push_back(where + ": the entry argument " + std::to_string(k) + " [" + str(a.text) + "] is no field of a global table");
        continue;
      }
      auto git = gimages_of_.find(object);
      std::set<uint32_t> found;
      std::set<std::string> others;  // the members of the image leaves of functions whose name is not the field read
      std::map<uint32_t, std::set<std::string>> paths;  // entry -> its image paths
      uint16_t written = 0;
      if (git != gimages_of_.end())
        for (uint32_t gi : git->second) {
          const kz_fact_gimage& g = gimages_[gi];
          if (g.kind != KZ_IMAGE_KIND_FUNC || !defined(g.fn)) continue;
          const std::string path = str(g.path), owner = str(g.owner), gfield = str(g.field);
          const size_t dot = path.rfind('.');
          if (dot == std::string::npos) continue;
          if (path.substr(dot + 1) != field) {
            others.insert(path.substr(dot + 1));
            continue;
          }
          if (!gfield.empty() && (gfield != field || owner != type)) continue;
          found.insert(g.fn);
          paths[g.fn].insert(path);
          written |= g.flags & (KZ_GIMAGE_FLAG_OBJECT_WRITTEN | KZ_GIMAGE_FLAG_FIELD_WRITTEN | KZ_GIMAGE_FLAG_RECORD_WRITTEN);
        }
      if (found.empty()) {
        std::string o;
        for (const std::string& x : others) o += (o.empty() ? "" : ", ") + x;
        unresolved_spawns_.push_back(where + ": the entry argument " + std::to_string(k) + " [" + str(a.text) + "] reads " + type + "." + field + " of " + object +
                                     ", and the static image has no function leaf of that member" +
                                     (o.empty() ? "" : " (its function leaves are of " + o + ": a read through another member of a union is not resolved in this revision)"));
        continue;
      }
      // the image is the run-time value: no write of the member
      std::string why;
      auto fsit = fstatus_of_.find(type + "." + field);
      if (written != 0) {
        why = "a leaf of the image is marked " + written_flags(written) + " (GLOBAL_IMAGES): the run-time value may differ from the image";
      } else if (fsit == fstatus_of_.end()) {
        why = "FIELD_STATUS has no row of " + type + "." + field + ": that nothing writes it is not known";
      } else {
        const kz_fact_fstatus& f = fstatus_[fsit->second];
        if (f.status_field != KZ_LOC_STATUS_COMPLETE || f.status_type != KZ_LOC_STATUS_COMPLETE)
          why = "the writer set of " + type + "." + field + " is not complete (FIELD_STATUS per-field " + enum_text("KZ_LOC_STATUS", f.status_field) + ", type-level " +
                enum_text("KZ_LOC_STATUS", f.status_type) + ")";
        else if (f.n_direct + f.n_data_fn + f.n_outptr != 0)
          why = type + "." + field + " has writers at run time (FIELD_STATUS: direct " + std::to_string(f.n_direct) + ", data function " + std::to_string(f.n_data_fn) +
                ", out-pointer " + std::to_string(f.n_outptr) + ")";
      }
      if (!why.empty()) {
        unresolved_spawns_.push_back(where + ": the entry argument " + std::to_string(k) + " [" + str(a.text) + "] reads " + type + "." + field + " of " + object +
                                     ", whose static image is not taken as its value: " + why);
        continue;
      }
      image_spawn_[c.site] = found;
      image_member_[c.site] = type + "." + field;
      for (const auto& [e, ps] : paths) image_paths_[c.site][e] = join(std::vector<std::string>(ps.begin(), ps.end()), ", ");
      for (uint32_t e : found) spawn_entries_[c.site].insert(e);
    }
  }

  // the functions whose every return value is the address of (a part of) a global object or null
  // (target_rules.cpp, the same rule over the records)
  void global_address_functions() {
    std::set<std::string> globals;
    for (const kz_fact_read& r : reads_)
      if (r.root_kind == KZ_READ_ROOT_GLOBAL) globals.insert(str(r.root));
    for (const kz_fact_argflow& a : argflow_) {
      const std::string o = str(a.object);
      if (!o.empty()) globals.insert(base_name(o));
    }
    auto innermost = [&](uint32_t r) -> const kz_fact_expr* {
      while (r != kN && r < exprs_.size() && exprs_[r].op != KZ_EXPR_OP_LOAD) r = exprs_[r].a;
      return r != kN && r < exprs_.size() ? &exprs_[r] : nullptr;
    };
    auto written_global = [&](const std::string& text) {
      if (text.size() < 2 || text[0] != '&') return false;
      size_t i = 1;
      while (i < text.size() && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) ++i;
      if (i < text.size() && text[i] != '.' && text[i] != '[') return false;
      return globals.count(text.substr(1, i - 1)) != 0;
    };
    std::map<std::string, std::vector<uint32_t>> by_fn;
    for (size_t i = 0; i < returns_.size(); ++i) by_fn[base_name(fname(returns_[i].fn))].push_back(static_cast<uint32_t>(i));
    for (bool changed = true; changed;) {
      changed = false;
      for (const auto& [fn, rows] : by_fn) {
        if (gaf_.count(fn) != 0) continue;
        bool all = true, address = false;
        for (uint32_t ri : rows) {
          const kz_fact_return& r = returns_[ri];
          if (r.vkind == KZ_RET_VALUE_CONST && r.value == 0) continue;
          if (r.vkind == KZ_RET_VALUE_ADDR) {
            const kz_fact_expr* in = innermost(r.value_root);
            if ((in != nullptr && (in->flags == KZ_EXPR_REF_GLOBAL || in->flags == KZ_EXPR_REF_ELEM)) || written_global(str(r.text))) {
              address = true;
              continue;
            }
          }
          if (r.vkind == KZ_RET_VALUE_CALL && r.value_root < exprs_.size() && exprs_[r.value_root].op == KZ_EXPR_OP_CALL_RESULT &&
              gaf_.count(str(exprs_[r.value_root].name)) != 0) {
            address = true;
            continue;
          }
          all = false;
          break;
        }
        if (all && address) {
          gaf_.insert(fn);
          changed = true;
        }
      }
    }
  }
  bool shared_base(const kz_fact_argflow& a) const {
    switch (a.base) {
      case KZ_ARGBASE_GLOBAL:
      case KZ_ARGBASE_GLOBAL_ELEMENT:
      case KZ_ARGBASE_GLOBAL_POINTER:
        return true;
      case KZ_ARGBASE_FIELD_POINTER:
        return !str(a.object).empty();
      case KZ_ARGBASE_CALL_RESULT:
        return gaf_.count(str(a.root)) != 0;
      default:
        return false;
    }
  }

  // ---------------------------------------------------------------- actors (static, may)
  // Roots: the process root (`main`), the resolved entries of spawn calls (CALL_TARGETS spawn-entry, the
  // unrolled table-walk iterations, the startup-script entries of SPAWN_WRAP, the function leaves of the static
  // image a spawn's entry argument is read from).  A root's code is what it reaches over the call graph (direct
  // calls and resolved indirect targets) without entering another root.  A root other than the process root is a
  // trampoline -- no actor of its own, its code belongs to the roots it calls (transitively through other
  // trampolines) -- when it dispatches only: it enters other roots, only through indirect calls, and its own body
  // reads no field or global through a shared root (READS: a global, or the result of a function that returns
  // global addresses only) and writes no global object (LOCKSET write rows of `<global>`).  The record-field writes
  // of its body are not read: a typed write carries no base in these facts.  The roots a trampoline stands for are
  // those its own code enters through resolved indirect calls (field-based flow graph), and the entries resolved at
  // the calls of every derived spawn operation that passes the trampoline as the entry of its inner spawn (SPAWN_WRAP
  // called-by-entry): a thread that operation starts runs the trampoline and, through it, the entry its caller passed.
  // An entry not resolved at such a call is missing (an under-approximation, named in the header).
  bool own_shared_access(uint32_t r) const {
    for (const kz_fact_read& rd : reads_)
      if (rd.fn == r && (rd.root_kind == KZ_READ_ROOT_GLOBAL || (rd.root_kind == KZ_READ_ROOT_CALL && gaf_.count(str(rd.root)) != 0))) return true;
    auto it = lockset_writes_of_.find(r);
    if (it != lockset_writes_of_.end())
      for (uint32_t li : it->second)
        if (str(lockset_[li].type) == "<global>") return true;
    return false;
  }
  void actors() {
    for (uint32_t fn : fn_by_name_["main"])
      if (defined(fn)) root_kind_[fn] = "process";
    for (const auto& [site, entries] : spawn_entries_)
      for (uint32_t e : entries)
        if (defined(e) && root_kind_.count(e) == 0)
          root_kind_[e] = script_entries_.count(e) != 0 ? "script-entry"
                          : iter_spawn_.count(site) != 0 ? "table-walk-entry"
                          : image_spawn_.count(site) != 0 ? "table-image-entry"
                                                          : "spawn-entry";
    std::map<uint32_t, std::set<uint32_t>> calls_roots, calls_roots_direct;
    for (const auto& [r, kind] : root_kind_) {
      std::set<uint32_t>& reach = reach_[r];
      std::vector<uint32_t> stack{r};
      reach.insert(r);
      while (!stack.empty()) {
        const uint32_t x = stack.back();
        stack.pop_back();
        for (const auto& [y, indirect] : succ_[x]) {
          if (y != r && root_kind_.count(y) != 0) {
            calls_roots[r].insert(y);
            if (!indirect) calls_roots_direct[r].insert(y);
            continue;
          }
          if (reach.insert(y).second) stack.push_back(y);
        }
      }
    }
    for (const auto& [r, kind] : root_kind_) {
      if (kind == "process" || calls_roots[r].empty()) continue;
      if (!calls_roots_direct[r].empty()) kept_direct_.insert(r);
      else if (own_shared_access(r)) kept_shared_.insert(r);
      else trampolines_.insert(r);
    }
    // the entries resolved at the calls of a derived spawn operation that passes a trampoline to its inner spawn
    for (const kz_fact_spawnw& w : spawnw_) {
      if (w.kind != KZ_SPAWNW_KIND_CALLED_BY_ENTRY || trampolines_.count(w.entry_fn) == 0 || !defined(w.fn)) continue;
      std::vector<uint32_t> in;
      for (uint32_t ci : callers_[w.fn]) in.push_back(ci);
      auto ic = indirect_callers_.find(w.fn);
      if (ic != indirect_callers_.end()) in.insert(in.end(), ic->second.begin(), ic->second.end());
      for (uint32_t ci : in) {
        auto se = spawn_entries_.find(calls_[ci].site);
        if (se == spawn_entries_.end()) {
          via_op_missing_[w.entry_fn].insert(call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + " -> " + fname(w.fn));
          continue;
        }
        bool any = false;
        for (uint32_t e : se->second)
          if (defined(e) && root_kind_.count(e) != 0 && e != w.entry_fn) {
            via_op_[w.entry_fn][e].insert(fname(w.fn));
            any = true;
          }
        if (!any) via_op_missing_[w.entry_fn].insert(call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + " -> " + fname(w.fn));
      }
    }
    // what a trampoline stands for, through other trampolines
    std::function<void(uint32_t, std::set<uint32_t>*, std::set<uint32_t>*)> expand = [&](uint32_t t, std::set<uint32_t>* out, std::set<uint32_t>* seen) {
      if (!seen->insert(t).second) return;
      std::set<uint32_t> ys = calls_roots[t];
      auto vo = via_op_.find(t);
      if (vo != via_op_.end())
        for (const auto& [e, ops] : vo->second) ys.insert(e);
      for (uint32_t y : ys) {
        if (y == t) continue;
        if (trampolines_.count(y) != 0) expand(y, out, seen);
        else out->insert(y);
      }
    };
    for (uint32_t t : trampolines_) {
      std::set<uint32_t> seen;
      expand(t, &delegates_[t], &seen);
    }
    for (const auto& [r, reach] : reach_) {
      std::set<uint32_t> as;
      if (trampolines_.count(r) != 0) as = delegates_[r];
      else as.insert(r);
      for (uint32_t f : reach) actors_of_[f].insert(as.begin(), as.end());
    }
  }
  std::set<uint32_t> actor_set(uint32_t fn) const {
    auto it = actors_of_.find(fn);
    return it == actors_of_.end() ? std::set<uint32_t>{} : it->second;
  }
  // the actors a set of spawned entries stands for
  std::set<uint32_t> entry_actors(const std::set<uint32_t>& entries) const {
    std::set<uint32_t> out;
    for (uint32_t e : entries) {
      if (!defined(e)) continue;
      auto it = delegates_.find(e);
      if (trampolines_.count(e) != 0 && it != delegates_.end()) out.insert(it->second.begin(), it->second.end());
      else out.insert(e);
    }
    return out;
  }
  std::string actors_text(const std::set<uint32_t>& as) const {
    if (as.empty()) return "{} (no root reaches it)";
    std::string t = "{";
    bool first = true;
    for (uint32_t a : as) {
      auto sp = script_prim_.find(a);
      if (sp != script_prim_.end())
        for (const std::string& p : sp->second) note_p0(p);
      auto k = root_kind_.find(a);
      t += (first ? "" : ", ") + fname(a) + " (" + (k == root_kind_.end() ? "?" : k->second) + ")";
      first = false;
    }
    return t + "}";
  }
  bool uses_script_root(const std::set<uint32_t>& as) const {
    for (uint32_t a : as)
      if (script_entries_.count(a) != 0) return true;
    return false;
  }
  std::set<uint32_t> reach_direct(uint32_t from) const {
    std::set<uint32_t> seen{from};
    std::vector<uint32_t> stack{from};
    while (!stack.empty()) {
      const uint32_t x = stack.back();
      stack.pop_back();
      for (const auto& [y, indirect] : succ_[x])
        if (!indirect && seen.insert(y).second) stack.push_back(y);
    }
    return seen;
  }
  // the shortest call chain from `from` to a function that calls a P0 primitive of one of the kinds
  std::string chain_to_p0(uint32_t from, const std::set<uint8_t>& kinds) const {
    std::map<uint32_t, uint32_t> parent;
    std::vector<uint32_t> queue{from};
    parent[from] = kN;
    for (size_t qi = 0; qi < queue.size(); ++qi) {
      const uint32_t x = queue[qi];
      auto cit = calls_of_.find(x);
      if (cit != calls_of_.end())
        for (uint32_t ci : cit->second) {
          const std::string callee = str(calls_[ci].callee);
          auto pk = p0_kind_.find(callee);
          if (pk != p0_kind_.end() && kinds.count(pk->second) != 0 && !defined(calls_[ci].callee_fn)) {
            std::vector<std::string> names{callee + " (P0 " + enum_text("KZ_P0_KIND", pk->second) + ")"};
            for (uint32_t y = x; y != kN; y = parent[y]) names.push_back(fname(y));
            std::reverse(names.begin(), names.end());
            std::string t;
            for (const std::string& n : names) t += (t.empty() ? "" : " -> ") + n;
            return t;
          }
        }
      for (const auto& [y, indirect] : succ_[x])
        if (parent.count(y) == 0) {
          parent[y] = x;
          queue.push_back(y);
        }
    }
    return "";
  }

  // ---------------------------------------------------------------- layers
  // A row starts with begin_row(): the positions and functions it names note their files (row_files_), and its
  // feasibility verdict notes what it rests on (row_tags_).  The layers column then names, besides the input layers,
  // the basis deviations of the units of those files (the copies extract.basis_deviation[...] in the facts) and the
  // declared assumptions and the completeness rule the verdict uses.
  void begin_row() const {
    row_files_.clear();
    row_tags_.clear();
    row_p0_.clear();
  }
  void tag(const std::string& t) const { row_tags_.insert(t); }
  // a P0 entry the row's derivation uses; named in the layers column when its source is not checked
  void note_p0(const std::string& name) const {
    if (p0_unverified_.count(name) != 0) row_p0_.insert(name);
  }
  void note_p0_chain(const std::string& chain) const {
    // the last element of an OP_DERIV / SPAWN_WRAP chain "a -> b -> prim"
    const size_t at = chain.rfind(" -> ");
    note_p0(at == std::string::npos ? chain : chain.substr(at + 4));
  }
  void note_lock_prims() const {
    for (const std::string& p : lock_prims_) note_p0(p);
  }
  std::set<std::string> row_deviations() const {
    std::set<std::string> out;
    for (const std::string& f : row_files_)
      for (const auto& [unit, items] : deviation_files_) {
        const bool same = f == unit || (f.size() > unit.size() && f.compare(f.size() - unit.size(), unit.size(), unit) == 0 && f[f.size() - unit.size() - 1] == '/') ||
                          (unit.size() > f.size() && unit.compare(unit.size() - f.size(), f.size(), f) == 0 && unit[unit.size() - f.size() - 1] == '/');
        if (same) out.insert(items.begin(), items.end());
      }
    return out;
  }
  std::string layers_text(bool p0, bool l0, const std::string& extra = "") const {
    std::string t = "B";
    if (p0) t += " P0";
    if (l0) t += " L0";
    t += " R";
    if (!profile_free_) t += " case:profile(" + basis_map_.at_or("extract.profile") + ")";
    if (hand_entered_vocabulary_ != 0) t += " hand-entered:command-line(" + std::to_string(hand_entered_vocabulary_) + " items; G0 or other names, not automatic)";
    if (!extra.empty()) t += " " + extra;
    const std::set<std::string> devs = row_deviations();
    if (!devs.empty()) {
      t += " basis-deviation(";
      bool first = true;
      for (const std::string& d : devs) {
        t += (first ? "" : "; ") + d;
        first = false;
      }
      t += ")";
    }
    if (!row_tags_.empty()) {
      t += " rests-on(";
      bool first = true;
      for (const std::string& d : row_tags_) {
        t += (first ? "" : "; ") + d;
        first = false;
      }
      t += ")";
    }
    if (!row_p0_.empty()) t += " P0-unverified(" + join(std::vector<std::string>(row_p0_.begin(), row_p0_.end()), ", ") + ")";
    return t;
  }

  // ---------------------------------------------------------------- header
  void header(std::ostream& os) const {
    os << "kz-relations 1\n";
    os << "facts " << kz::to_hex(c_.artifact_hash()) << "\n";
    os << "rules " << relations_rules_revision() << "\n";
    os << "mode " << (profile_free_ ? "profile-free" : "profile-driven") << "\n";
    for (const char* k : {"extract.builtin_profile", "extract.profile", "extract.p0_table", "extract.p0_table_scope", "extract.derivation_rules", "extract.targets_rule"}) {
      auto it = basis_map_.find(k);
      if (it != basis_map_.end()) os << "  " << k << " = " << it->second << "\n";
    }
    os << "p0-compare " << p0_compare_table_revision() << ":";
    for (const P0CompareEntry& e : p0_compare_table()) os << " " << e.name << " (" << e.family << "; " << e.version << "; source " << e.source << ")";
    os << "\n";
    os << "p0-compare scope: " << p0_compare_table_scope() << "\n";
    os << "basis deviations copied into these facts: " << deviations_.size();
    for (const std::string& d : deviations_) os << " [" << d << "]";
    os << " (a row that names a position in a unit with its own deviation says so in its layers column)\n";
    os << "layers of these facts: B (build: compile database, link, closure, startup script lines), P0 (" << (profile_free_ ? "the P0 tables" : "profile vocabulary")
       << "), L0 (startup-script loader semantics, when a row rests on a script entry), R (extraction and relation rules)";
    os << "; a row whose derivation uses P0 entries not checked against their source names them: P0-unverified(...) (the source check of pthread_create is against "
          "the upstream glibc 2.39 copy; the builds link a distribution package of glibc 2.39 whose patches are not checked)";
    if (!profile_free_) os << "; case layer: the profile " << basis_map_.at_or("extract.profile");
    if (hand_entered_vocabulary_ != 0) os << "; hand-entered vocabulary on the command line: " << hand_entered_vocabulary_ << " items";
    os << "\n";
    os << "run order: no record given (this revision reads no run record; the column says so in every row)\n";
    os << "actors: roots " << root_kind_.size() << " (process " << count_kind("process") << ", spawn-entry " << count_kind("spawn-entry") << ", table-walk-entry "
       << count_kind("table-walk-entry") << ", table-image-entry " << count_kind("table-image-entry") << ", script-entry " << count_kind("script-entry") << "), trampolines "
       << trampolines_.size();
    for (uint32_t t : trampolines_) os << " [" << fname(t) << " -> " << actors_text(delegates_.at(t)) << "]";
    os << "; roots a trampoline stands for through the calls of a derived spawn operation that passes it to its inner spawn:";
    if (via_op_.empty()) os << " none";
    for (const auto& [t, es] : via_op_) {
      os << " [" << fname(t) << " <-";
      for (const auto& [e, ops] : es) os << " " << fname(e) << " (" << join(std::vector<std::string>(ops.begin(), ops.end()), ", ") << ")";
      os << "]";
    }
    os << "; calls of such an operation with no resolved entry (their threads are missing from the trampoline's roots: an under-approximation):";
    size_t nmiss = 0;
    for (const auto& [t, ms] : via_op_missing_) nmiss += ms.size();
    os << " " << nmiss;
    for (const auto& [t, ms] : via_op_missing_)
      for (const std::string& m : ms) os << " [" << fname(t) << ": " << m << "]";
    os << "; roots that enter other roots and stay actors: through a direct call " << kept_direct_.size();
    for (uint32_t t : kept_direct_) os << " [" << fname(t) << "]";
    os << ", with a shared access of their own body " << kept_shared_.size();
    for (uint32_t t : kept_shared_) os << " [" << fname(t) << "]";
    os << "\n";
    os << "spawn calls resolved through the static image of the table their entry argument is read from: " << image_spawn_.size();
    for (const auto& [site, es] : image_spawn_) {
      auto cit = call_by_site_.find(site);
      os << " [" << (cit != call_by_site_.end() ? call_pos(cit->second) : "site " + std::to_string(site)) << ":";
      for (uint32_t e : es) os << " " << fname(e);
      os << "; the member " << image_member_.at(site) << " has a complete writer set with no writer instance under both rules of FIELD_STATUS and no leaf marked written]";
    }
    os << "\n";
    os << "spawn calls with no resolved entry (their threads are no actors here; the code only they reach has the actors of its other callers): " << unresolved_spawns_.size();
    for (const std::string& u : unresolved_spawns_) os << " [" << u << "]";
    os << "\n";
    os << "summary A " << n_a_ << " (namespaces " << n_ns_ << ", lookup uses " << n_a_ << ", with a creator of the same partition in another actor " << n_a_paired_
       << ", key-matched " << n_a_keymatch_ << ") B " << n_b_ << " (with a callee check " << n_b_check_ << ") C " << n_c_ << " (self-identity reads " << n_c_self_
       << "); flip static-possible " << n_flip_possible_ << ", static-undetermined " << n_flip_undet_ << ", none " << n_flip_none_ << "; feasibility proven-impossible "
       << n_feas_impossible_ << ", possible " << n_feas_possible_ << ", undecidable " << n_feas_undecidable_ << ", not graded " << n_feas_not_graded_ << "\n";
  }
  size_t count_kind(const char* k) const {
    size_t n = 0;
    for (const auto& [r, kind] : root_kind_)
      if (kind == k) ++n;
    return n;
  }
  void tally_flip(const std::string& grade) {
    if (grade == "static-possible") ++n_flip_possible_;
    else if (grade == "static-undetermined") ++n_flip_undet_;
    else ++n_flip_none_;
  }
  void tally_feas(const std::string& grade) {
    if (grade == "proven-impossible") ++n_feas_impossible_;
    else if (grade == "possible") ++n_feas_possible_;
    else if (grade == "not-graded") ++n_feas_not_graded_;
    else ++n_feas_undecidable_;
  }
  static std::string violation_text() { return "unconfirmed: RV2 §5.4 needs an observed failure with a feasible other order, or a realized forced order; no record given"; }

  // ---------------------------------------------------------------- shared helpers
  struct WriterRow {
    std::string where, kind, value_text, guard;
    uint32_t fn = kN;
    bool known = false;
    int64_t value = 0;
    std::string file;
    uint32_t line = 0, col = 0;
    uint32_t flags = 0;  // KZ_WRITER_FLAG (WRITERS rows only)
  };
  // the writers of a field: WRITERS rows when the field is a target, else the write events of LOCKSET (all
  // write sites, without values), else FUNC_WRITES (first write per function)
  std::vector<WriterRow> writers_of(const std::string& tf, std::string* source) const {
    std::vector<WriterRow> out;
    auto wit = writers_of_.find(tf);
    if (wit != writers_of_.end()) {
      *source = "WRITERS (the field is an extraction target)";
      for (uint32_t i : wit->second) {
        const kz_fact_writer& w = writers_[i];
        WriterRow r;
        r.fn = w.fn;
        r.file = str(w.file);
        r.line = w.line;
        r.col = w.col;
        r.where = pos(r.file, w.line, w.col) + " in " + fname(w.fn) + (w.via_fn != kN ? " via " + fname(w.via_fn) : "");
        r.kind = enum_text("KZ_WRITER_KIND", w.kind);
        r.known = w.value_kind == KZ_ARG_KIND_CONST;
        r.value = w.value;
        r.value_text = (r.known ? std::to_string(w.value) : enum_text("KZ_ARG_KIND", w.value_kind)) + " [" + str(w.value_text) + "]";
        r.guard = ex(w.guard_root);
        r.flags = w.flags;
        out.push_back(r);
      }
      return out;
    }
    std::set<std::tuple<std::string, uint32_t, uint32_t, uint32_t>> seen;
    for (const auto& [fn, rows] : lockset_writes_of_)
      for (uint32_t i : rows) {
        const kz_fact_lockset& l = lockset_[i];
        if (str(l.type) + "." + str(l.field) != tf) continue;
        if (!seen.insert({str(l.file), l.line, l.col, l.fn}).second) continue;
        WriterRow r;
        r.fn = l.fn;
        r.file = str(l.file);
        r.line = l.line;
        r.col = l.col;
        r.where = pos(r.file, l.line, l.col) + " in " + fname(l.fn);
        r.kind = "write";
        r.value_text = "value not extracted (not a target)";
        out.push_back(r);
      }
    if (!out.empty()) {
      *source = "LOCKSET write events (positions only: the field is no extraction target)";
      std::sort(out.begin(), out.end(), [](const WriterRow& a, const WriterRow& b) { return std::tie(a.file, a.line, a.col) < std::tie(b.file, b.line, b.col); });
      return out;
    }
    auto fit = fwrites_of_.find(tf);
    if (fit != fwrites_of_.end()) {
      *source = "FUNC_WRITES (first write per function, positions only)";
      for (uint32_t i : fit->second) {
        const kz_fact_fwrite& w = fwrites_[i];
        WriterRow r;
        r.fn = w.fn;
        r.file = str(w.file);
        r.line = w.line;
        r.where = pos(r.file, w.line, 0) + " in " + fname(w.fn);
        r.kind = enum_text("KZ_FWRITE_KIND", w.kind);
        r.value_text = "value not extracted";
        out.push_back(r);
      }
    }
    return out;
  }
  // the initial value of a field from the INIT image
  bool init_value(const std::string& tf, int64_t* v, std::string* text) const {
    auto it = inits_of_.find(tf);
    if (it == inits_of_.end()) return false;
    std::set<int64_t> vals;
    for (uint32_t i : it->second) {
      if (inits_[i].source == KZ_INIT_SOURCE_NONCONST) return false;  // a value that is not a constant, or a walk that stopped
      vals.insert(inits_[i].value);
    }
    if (vals.size() != 1) return false;
    *v = *vals.begin();
    *text = "INIT " + std::to_string(*v) + " (" + str(inits_[it->second.front()].object) + ")";
    return true;
  }
  std::string result_use(uint32_t ci) const {
    const uint32_t f = calls_[ci].flags;
    if (f & KZ_CALL_FLAG_RESULT_TESTED) return "tested";
    if (f & KZ_CALL_FLAG_RESULT_USED) return "used";
    return "discarded";
  }
  std::vector<uint32_t> held(uint32_t lockset_row) const { return seq(lockset_[lockset_row].held_root); }
  std::set<uint32_t> openers(uint32_t lockset_row) const {
    const std::vector<uint32_t> v = seq(lockset_[lockset_row].opener_root);
    return {v.begin(), v.end()};
  }
  std::string lock_text(uint32_t id) const {
    if (id >= lockids_.size()) return "#" + std::to_string(id) + "?";
    return "#" + std::to_string(id) + " " + str(lockids_[id].text) + " (" + enum_text("KZ_LOCKID_STATUS", lockids_[id].status) + ")";
  }
  bool static_lock(uint32_t id) const { return id < lockids_.size() && lockids_[id].status == KZ_LOCKID_STATUS_STATIC; }
  std::string held_text(uint32_t row) const {
    std::string t = "{";
    bool first = true;
    for (uint32_t id : held(row)) {
      t += (first ? "" : ", ") + lock_text(id);
      first = false;
    }
    t += "}";
    const std::set<uint32_t> op = openers(row);
    if (!op.empty()) {
      t += " acquired-at [";
      bool f2 = true;
      for (uint32_t s : op) {
        auto it = call_by_site_.find(s);
        t += (f2 ? "" : ", ") + (it != call_by_site_.end() ? call_pos(it->second) : "site " + std::to_string(s));
        f2 = false;
      }
      t += "]";
    }
    if (lockset_[row].flags & KZ_LOCKSET_FLAG_FAILURE_PATH) t += " failure-path";
    if (lockset_[row].flags & KZ_LOCKSET_FLAG_UNKNOWN_RELEASE) t += " unknown-release";
    return t;
  }

  // the constant returns of a function whose condition mentions a parameter (directly or in the
  // arguments of a call in the condition)
  struct Check {
    int64_t value = 0;
    std::string text, cond;
    uint32_t cond_root = kN;
  };
  std::vector<Check> checks_on_param(uint32_t fn, uint32_t k) const {
    std::vector<Check> out;
    std::set<std::pair<int64_t, uint32_t>> seen;
    auto it = returns_of_.find(fn);
    if (it == returns_of_.end()) return out;
    for (uint32_t ri : it->second) {
      const kz_fact_return& r = returns_[ri];
      if (r.vkind != KZ_RET_VALUE_CONST || r.cond_root == kN || !has_param(r.cond_root, k)) continue;
      if (!seen.insert({r.value, r.cond_root}).second) continue;
      out.push_back(Check{r.value, str(r.text), ex(r.cond_root), r.cond_root});
    }
    return out;
  }
  // a function's constant returns whose condition mentions a field
  std::vector<Check> checks_on_field(uint32_t fn, const std::string& tf) const {
    std::vector<Check> out;
    std::set<std::pair<int64_t, uint32_t>> seen;
    auto it = returns_of_.find(fn);
    if (it == returns_of_.end()) return out;
    for (uint32_t ri : it->second) {
      const kz_fact_return& r = returns_[ri];
      if (r.vkind != KZ_RET_VALUE_CONST || r.cond_root == kN || !has_field(r.cond_root, tf)) continue;
      if (!seen.insert({r.value, r.cond_root}).second) continue;
      out.push_back(Check{r.value, str(r.text), ex(r.cond_root), r.cond_root});
    }
    return out;
  }
  // the constants a function (by name) returns, with their conditions (RET_CONSTS)
  std::string retconsts_text(const std::string& name) const {
    std::string t;
    auto fit = fn_by_name_.find(name);
    if (fit == fn_by_name_.end()) return t;
    for (uint32_t fn : fit->second) {
      auto it = retconsts_of_.find(fn);
      if (it == retconsts_of_.end()) continue;
      for (uint32_t i : it->second) {
        const kz_fact_retconst& r = retconsts_[i];
        t += (t.empty() ? "" : "; ") + std::to_string(r.value) + " [" + str(r.text) + "] when " + ex(r.cond_root);
      }
    }
    return t;
  }

  // A terminal operation the derivation reaches only on some paths (OP_DERIV mode may, or noreturn: no path returns,
  // and some path reaches the primitive): its terminal calls with their guards, the guards evaluated with each
  // constant value an argument takes at the call (the other arguments and the rest of the program unknown), and its
  // loops that have no exit (the call does not return on a path through one).
  std::string may_terminal(uint32_t fn, const std::map<uint32_t, std::set<int64_t>>& argvals) const {
    auto it = opderiv_of_.find(fn);
    if (it == opderiv_of_.end()) return "";
    std::vector<uint32_t> rows;
    uint8_t mode = KZ_OPDERIV_MODE_NONE;
    for (uint32_t i : it->second)
      if (opderiv_[i].kind == KZ_OPDERIV_KIND_TERMINAL) {
        rows.push_back(i);
        mode = opderiv_[i].mode;
      }
    if ((mode != KZ_OPDERIV_MODE_MAY && mode != KZ_OPDERIV_MODE_NORETURN) || rows.empty()) return "";
    std::string t = enum_text("KZ_OPDERIV_MODE", mode) + ": the terminal calls of " + fname(fn) + " are guarded:";
    for (uint32_t i : rows) {
      note_p0_chain(str(opderiv_[i].chain));
      auto cit = call_by_site_.find(opderiv_[i].site);
      t += " " + (cit != call_by_site_.end() ? call_pos(cit->second) : "site " + std::to_string(opderiv_[i].site)) + " -> " + fname(opderiv_[i].next_fn) + " under " + ex(opderiv_[i].guard_root) + ";";
    }
    if (argvals.empty()) {
      t += " not evaluated: no constant argument value is known at this call";
    } else {
      bool all_false = true, any_true = false;
      std::set<std::string> unknown;
      std::string vals;
      for (const auto& [a, vs] : argvals)
        for (int64_t v : vs) {
          vals += (vals.empty() ? "" : ", ") + std::string("argument ") + std::to_string(a) + " = " + std::to_string(v);
          Subst sb;
          sb.params[a] = v;
          for (uint32_t i : rows) {
            const Val g = opderiv_[i].guard_root == kN ? known(1) : eval(opderiv_[i].guard_root, sb, &unknown);
            if (!g.known || g.v != 0) all_false = false;
            if (g.known && g.v != 0) any_true = true;
          }
        }
      if (all_false) {
        t += " with " + vals + " every guard is false: no derived terminal call is reached on this path";
      } else if (any_true) {
        t += " with " + vals + " a guard holds";
      } else {
        std::string u;
        for (const std::string& x : unknown) u += (u.empty() ? "" : "; ") + x;
        t += " with " + vals + " not decided: " + u;
      }
    }
    for (const kz_fact_loopexit& l : loopexits_)
      if (l.fn == fn && l.kind == KZ_LOOPEXIT_KIND_LOOP && (l.flags & KZ_LOOPEXIT_FLAG_NO_EXIT) != 0)
        t += "; the loop " + pos(str(l.file), l.line, l.col) + " in " + fname(fn) + " has no exit: the call does not return on a path through it (no P0 terminal on that path)";
    return t;
  }
  // the chain from a terminal or delay operation down to its P0 primitive (OP_DERIV); the primitive is noted for the
  // row's layers
  std::string opderiv_chain(uint32_t fn, uint8_t kind) const {
    auto it = opderiv_of_.find(fn);
    if (it == opderiv_of_.end()) return "";
    std::string best;
    uint32_t depth = kN;
    for (uint32_t i : it->second) {
      const kz_fact_opderiv& r = opderiv_[i];
      if (r.kind != kind) continue;
      if (depth == kN || r.depth < depth) {
        depth = r.depth;
        best = enum_text("KZ_OPDERIV_MODE", r.mode) + " " + str(r.chain);
      }
    }
    note_p0_chain(opderiv_chain_raw(fn, kind));
    return best;
  }
  std::string opderiv_chain_raw(uint32_t fn, uint8_t kind) const {
    auto it = opderiv_of_.find(fn);
    if (it == opderiv_of_.end()) return "";
    std::string best;
    uint32_t depth = kN;
    for (uint32_t i : it->second) {
      const kz_fact_opderiv& r = opderiv_[i];
      if (r.kind != kind) continue;
      if (depth == kN || r.depth < depth) {
        depth = r.depth;
        best = str(r.chain);
      }
    }
    return best;
  }

  // ---------------------------------------------------------------- path B
  void path_b(std::ostream& os) {
    std::map<std::tuple<std::string, uint32_t, uint32_t>, std::vector<uint32_t>> groups;
    for (size_t i = 0; i < argflow_.size(); ++i) {
      const kz_fact_argflow& a = argflow_[i];
      if (a.src != KZ_ARGSRC_FIELD) continue;
      const std::string type = str(a.type), field = str(a.field);
      if (type.empty() || field.empty() || type == "<global>" || !shared_base(a)) continue;
      auto cit = call_by_site_.find(a.site);
      if (cit == call_by_site_.end()) continue;
      const kz_fact_call& c = calls_[cit->second];
      if (c.kind != KZ_CALL_KIND_DIRECT || !defined(c.callee_fn)) continue;
      groups[{type + "." + field, c.callee_fn, a.arg_index}].push_back(static_cast<uint32_t>(i));
    }
    std::ostringstream rows;
    uint32_t no_writer = 0, same_actor = 0;
    for (const auto& [key, afs] : groups) {
      const auto& [tf, callee, argi] = key;
      begin_row();
      std::string wsource;
      const std::vector<WriterRow> ws = writers_of(tf, &wsource);
      if (ws.empty()) {
        ++no_writer;
        continue;
      }
      std::set<uint32_t> ra, wa;
      std::set<uint32_t> reader_fns;
      for (uint32_t ai : afs) reader_fns.insert(calls_[call_by_site_.at(argflow_[ai].site)].caller_fn);
      for (uint32_t f : reader_fns) {
        const std::set<uint32_t> s = actor_set(f);
        ra.insert(s.begin(), s.end());
      }
      for (const WriterRow& w : ws) {
        const std::set<uint32_t> s = actor_set(w.fn);
        wa.insert(s.begin(), s.end());
      }
      bool inter = false;
      for (uint32_t r : ra)
        for (uint32_t w : wa)
          if (r != w) inter = true;
      const bool undetermined = ra.empty() || wa.empty();
      if (!inter && !undetermined) {
        ++same_actor;
        continue;
      }
      ++n_b_;
      const std::vector<Check> checks = checks_on_param(callee, argi);
      if (!checks.empty()) ++n_b_check_;
      rows << "  B" << n_b_ << " " << tf << " -> " << fname(callee) << "#" << argi << " sites=" << afs.size() << " callee-check=" << (checks.empty() ? "none" : "yes") << "\n";
      std::map<std::string, uint32_t> handling;
      for (uint32_t ai : afs) {
        const uint32_t ci = call_by_site_.at(argflow_[ai].site);
        const std::string use = result_use(ci);
        ++handling[use];
        rows << "      site " << call_pos(ci) << " in " << fname(calls_[ci].caller_fn) << " [" << str(argflow_[ai].text) << "] base=" << enum_text("KZ_ARGBASE", argflow_[ai].base)
             << " hops=" << argflow_[ai].hops << " result=" << use << "\n";
      }
      int64_t iv = 0;
      std::string itext;
      const bool has_init = init_value(tf, &iv, &itext);
      rows << "      field: " << (has_init ? itext : "no INIT row") << "; writers " << ws.size() << " from " << wsource << "\n";
      for (const WriterRow& w : ws)
        rows << "        " << w.kind << " at " << w.where << " value=" << w.value_text << (w.guard.empty() || w.guard == "-" ? "" : " guard=" + w.guard) << "\n";
      rows << "      actors: readers " << actors_text(ra) << "; writers " << actors_text(wa) << (undetermined ? " (undetermined)" : " (inter-actor)") << "\n";
      for (const Check& ck : checks) rows << "      callee-check: " << fname(callee) << " returns " << ck.value << " [" << ck.text << "] when " << ck.cond << "\n";
      // one level into the calls the check's conditions make with the parameter
      std::set<std::string> deeper;
      for (const Check& ck : checks)
        visit(ck.cond_root, [&](const kz_fact_expr& e) {
          if (e.op == KZ_EXPR_OP_CALL_RESULT && has_param(e.a, argi)) deeper.insert(str(e.name));
        });
      for (const std::string& g : deeper) {
        const std::string rc = retconsts_text(g);
        if (!rc.empty()) rows << "      callee-check (called with the parameter): " << g << " returns " << rc << "\n";
      }
      rows << "      handling: discarded " << handling["discarded"] << ", used " << handling["used"] << ", tested " << handling["tested"] << "\n";
      rows << "      dependency: path B: the shared field " << tf << " is read as argument " << argi << " of " << fname(callee) << " at " << afs.size()
           << " sites; its writers run in " << (undetermined ? "an undetermined actor" : "another actor") << (checks.empty() ? "" : "; the callee's constant returns depend on the argument") << "\n";
      rows << "      run-order: no record given\n";
      const std::string feas = b_feasibility(ra, wa);
      rows << "      feasibility: " << feas << "\n";
      rows << "      violation: " << violation_text() << "\n";
      // flip: the check's conditions with the argument at the initial value and at each written constant
      std::string grade = "none", why = "no callee check depends on the argument";
      if (!checks.empty()) {
        std::set<std::string> unknown;
        bool possible = false, all_known = true;
        std::vector<int64_t> values;
        for (const WriterRow& w : ws)
          if (w.known) values.push_back(w.value);
        for (const Check& ck : checks) {
          Subst si;
          if (has_init) si.params[argi] = iv;
          const Val vi = has_init ? eval(ck.cond_root, si, &unknown) : Val{};
          if (!vi.known) all_known = false;
          bool any_symbolic = false;
          for (const WriterRow& w : ws)
            if (!w.known) any_symbolic = true;
          if (any_symbolic) {
            all_known = false;
            unknown.insert("a written value that is not a constant");
          }
          for (int64_t v : values) {
            Subst sw;
            sw.params[argi] = v;
            const Val vw = eval(ck.cond_root, sw, &unknown);
            if (!vw.known) all_known = false;
            if (vi.known && vw.known && vi.v != vw.v) possible = true;
          }
        }
        if (possible) {
          grade = "static-possible";
          why = "a written constant and the initial value give different results of the callee check";
        } else if (all_known) {
          grade = "none";
          why = "the initial value and the written constants give the same result of the callee check";
        } else {
          grade = "static-undetermined";
          std::string u;
          for (const std::string& x : unknown) u += (u.empty() ? "" : "; ") + x;
          why = "the check is not evaluated: " + u;
        }
      }
      tally_flip(grade);
      rows << "      flip: " << grade << ": " << why << "\n";
      bool l0 = uses_script_root(ra) || uses_script_root(wa);
      rows << "      layers: " << layers_text(true, l0, wsource.compare(0, 7, "WRITERS") == 0 ? "(writers: rule targets)" : "") << "\n";
    }
    os << "[B] " << n_b_ << " (field-argument groups " << groups.size() << "; left out: no writer " << no_writer << ", one actor only " << same_actor << ")\n" << rows.str();
  }
  // creation order between actors (unrolled table-walk spawns) and an await after the creation; roots resolved from a
  // static image get no order in this revision (the image index is not read as an iteration order)
  std::string b_feasibility(const std::set<uint32_t>& ra, const std::set<uint32_t>& wa) {
    std::vector<std::string> parts;
    bool ordered = false, any_await = false;
    for (const auto& [site, iters] : iter_spawn_) {
      std::set<uint32_t> rin, win;
      for (uint32_t r : ra)
        if (iters.count(r) != 0) rin.insert(r);
      for (uint32_t w : wa)
        if (iters.count(w) != 0) win.insert(w);
      if (rin.empty() || win.empty()) continue;
      ordered = true;
      note_p0_chain(spawn_chain(site));
      const uint32_t ci = call_by_site_.at(site);
      const std::string await = await_after(ci);
      if (!await.empty()) any_await = true;
      std::string t = "creation order at the table walk " + call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + ":";
      for (uint32_t w : win) {
        std::string beforew, afterw;
        for (uint32_t r : rin) {
          if (r == w) continue;
          (iters.at(r) < iters.at(w) ? beforew : afterw) += (iters.at(r) < iters.at(w) ? (beforew.empty() ? "" : ", ") : (afterw.empty() ? "" : ", ")) + fname(r) + " (iteration " + std::to_string(iters.at(r)) + ")";
        }
        t += " writer actor " + fname(w) + " at iteration " + std::to_string(iters.at(w)) + "; reader actors created before it: " + (beforew.empty() ? "none" : beforew) +
             "; after it: " + (afterw.empty() ? "none" : afterw) + ".";
      }
      t += await.empty() ? " No await after the creation in the same iteration." : " " + await;
      parts.push_back(t);
    }
    std::vector<std::string> images;
    for (const auto& [site, es] : image_spawn_) {
      std::string rin, win;
      const std::map<uint32_t, std::string>& ps = image_paths_[site];
      auto named = [&](uint32_t a) {
        auto it = ps.find(a);
        return fname(a) + (it != ps.end() ? " [" + it->second + "]" : "");
      };
      for (uint32_t r : ra)
        if (es.count(r) != 0) rin += (rin.empty() ? "" : ", ") + named(r);
      for (uint32_t w : wa)
        if (es.count(w) != 0) win += (win.empty() ? "" : ", ") + named(w);
      if (rin.empty() || win.empty()) continue;
      const uint32_t ci = call_by_site_.at(site);
      images.push_back("the spawn " + call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + " starts writer actor(s) " + win + " and reader actor(s) " + rin +
                       " (image paths of " + image_member_.at(site) + ")");
    }
    std::string grade = "undecidable";
    std::string text;
    if (ordered) {
      text = std::string("fixed unconfirmed (") + (any_await ? "creation order + await; the await rests on the L1 premise only" : "creation order only") + "): ";
      for (const std::string& p : parts) text += p + " ";
      text += "That a use precedes or follows the awaited state of its actor is not decided statically, so neither order is proven.";
    } else if (!images.empty()) {
      text = "no static order derived: ";
      for (size_t i = 0; i < images.size(); ++i) text += (i == 0 ? "" : "; ") + images[i];
      text += "; for roots resolved from a static image this revision derives no creation order (the image index is not read as an iteration order) and no await";
    } else {
      text = "no static order between the reader and the writer actors: they are not created by one unrolled table walk (LOOP_ITERS) nor resolved from one static "
             "image, and the process root's order relative to the spawns is not decided";
    }
    tally_feas(grade);
    return grade + ": " + text;
  }
  // the chain of the spawn operation a spawn call makes, down to its primitive (SPAWN_WRAP), or the callee itself
  std::string spawn_chain(uint32_t site) const {
    for (const kz_fact_spawnw& w : spawnw_)
      if (w.site == site && !str(w.chain).empty()) return str(w.chain);
    auto cit = call_by_site_.find(site);
    if (cit == call_by_site_.end()) return "";
    const kz_fact_call& c = calls_[cit->second];
    if (defined(c.callee_fn)) {
      for (const kz_fact_spawnw& w : spawnw_)
        if (w.fn == c.callee_fn && !str(w.chain).empty()) return str(w.chain);
      return "";
    }
    return str(c.callee);
  }
  // a call after the creation call in the same loop iteration that reaches an await loop (AWAIT_L1, per-field rule).
  // The premise of lemma L1 is what AWAIT_L1 certifies; whether the loop's predicate concerns the created actor and
  // whether its timeout exit ends the creator are not read, so the creations are not claimed to be serialized.
  std::string await_after(uint32_t spawn_ci) {
    const kz_fact_call& s = calls_[spawn_ci];
    if (s.loop == kN) return "";
    auto cit = calls_of_.find(s.caller_fn);
    if (cit == calls_of_.end()) return "";
    for (uint32_t ci : cit->second) {
      const kz_fact_call& c = calls_[ci];
      if (c.loop != s.loop || !before(str(s.file), s.line, s.col, str(c.file), c.line, c.col) || !defined(c.callee_fn)) continue;
      for (uint32_t f : reach_direct(c.callee_fn)) {
        auto ait = awaits_of_.find(f);
        if (ait == awaits_of_.end()) continue;
        for (uint32_t ai : ait->second) {
          const kz_fact_await& a = awaits_[ai];
          if (a.cert_field != KZ_LOOP_CERT_AWAIT) continue;
          std::string where = "loop site " + std::to_string(a.loop);
          uint32_t delay_site = kN;
          for (const kz_fact_loop& l : loops_)
            if (l.site == a.loop) {
              where = pos(str(l.file), l.line, l.col);
              delay_site = l.delay_site;
            }
          if (delay_site != kN) {
            auto dit = call_by_site_.find(delay_site);
            if (dit != call_by_site_.end()) note_p0_chain(opderiv_chain_raw(calls_[dit->second].callee_fn, KZ_OPDERIV_KIND_DELAY));
          }
          if (a.lock != kN) note_lock_prims();
          tag("per-field completeness rule (PI decision pending)");
          if (assumptions_.count("assume.no_cross_member_pointer_arithmetic") != 0) tag("assume.no_cross_member_pointer_arithmetic");
          return "The call " + call_pos(ci) + " (" + fname(c.callee_fn) + ") after the creation reaches the await loop " + where + " in " + fname(f) +
                 ": an await loop follows the creation (the L1 premise holds under the AWAIT_L1 per-field rule, which rests on assume.no_cross_member_pointer_arithmetic and is a "
                 "PI decision pending; type-level " +
                 enum_text("KZ_LOOP_CERT", a.cert_type) + "); lemma L1 itself is not decided, and whether the loop's predicate concerns the created actor and whether its "
                 "timeout exit ends the creator are not read.";
        }
      }
    }
    return "";
  }

  // ---------------------------------------------------------------- path C
  // a write after a spawn: a LOCKSET write row of the creator (call_row kN), or of a function a call of the creator
  // after the spawn reaches (call_row that call's LOCKSET row; first_level: in the function the call calls), or a
  // store the spawn operation makes through an argument (row kN, spawn_arg)
  struct WriteCtx {
    uint32_t row = kN;
    uint32_t call_row = kN;
    bool first_level = false;
    uint32_t fn = kN;
    uint32_t spawn_arg = kN;  // a store the spawn operation makes through this argument (row kN)
    std::string how;          // "" a typed write of the field, "record" a write of the whole record, "covers" a data function on another field
  };
  // what a read of the field in the child runs under: its own row and, through a call, the call's row
  struct Ctx {
    std::string where;
    uint32_t row = kN;       // LOCKSET row of the read
    uint32_t call_row = kN;  // LOCKSET row of the call, or kN
    bool entry = false;      // the read's function is the entry of a spawned thread (no caller lock)
    bool not_reached = false;  // a call the spawned entries do not reach (no grade)
  };
  // the facts path C's feasibility reads besides the lock sets
  struct CExtra {
    uint32_t site = kN;                 // the spawn call
    uint32_t creator = kN;              // its function
    std::string tf;                     // the field
    std::vector<std::string> unknown;   // calls after the spawn that may write anything (unresolved indirect calls)
    std::vector<std::string> callers;   // writes of the field in the creator's callers' own bodies
    std::string callers_names;          // the creator's callers
  };
  void path_c(std::ostream& os) {
    std::ostringstream rows;
    uint32_t unresolved = 0, spawn_calls = 0, no_paths = 0, callee_only_fields = 0, record_only_fields = 0, not_reached_ctx = 0;
    for (const auto& [site, entries] : spawn_entries_) {
      auto cit = call_by_site_.find(site);
      if (cit == call_by_site_.end()) continue;
      const uint32_t si = cit->second;
      const kz_fact_call& s = calls_[si];
      if (!defined(s.caller_fn)) continue;
      ++spawn_calls;
      std::set<uint32_t> resolved;
      for (uint32_t e : entries)
        if (defined(e)) resolved.insert(e);
      if (resolved.empty()) {
        ++unresolved;
        continue;
      }
      const std::set<uint32_t> child = entry_actors(resolved);
      const uint32_t P = s.caller_fn;
      auto lsit = lockset_call_.find(site);
      const std::string s_held = lsit == lockset_call_.end() ? "{} (no LOCKSET row)" : held_text(lsit->second);
      // the identity the creator receives: the out-stores of the spawn operation's parameters
      std::string identity;
      if (defined(s.callee_fn)) {
        for (uint32_t k = 0; k < s.nargs; ++k) {
          auto pit = pflow_of_.find({s.callee_fn, k});
          if (pit == pflow_of_.end()) continue;
          bool out = false;
          for (uint32_t pi : pit->second)
            if (pflow_[pi].kind == KZ_PFLOW_KIND_OUT_STORE) out = true;
          if (!out) continue;
          auto ait = argflow_at_.find({site, k});
          identity += (identity.empty() ? "" : "; ") + std::string("arg ") + std::to_string(k) + (ait != argflow_at_.end() ? " [" + str(argflow_[ait->second].text) + "]" : "") +
                      " (" + fname(s.callee_fn) + " stores through it)";
        }
      } else {
        for (const kz_fact_p0& p : p0_)
          if (str(p.name) == str(s.callee) && p.out_param != kN) identity = "arg " + std::to_string(p.out_param) + " (P0 " + str(p.name) + " stores the new thread id through it)";
      }
      // the creator's writes after the spawn call, on its control-flow graph (LOCK_PATHS after-spawn), and those of the
      // functions its calls after the spawn may run (closure: direct calls, resolved indirect targets, and functions
      // passed to a callee without a body that is no P0 entry); the calls after the spawn whose targets are not resolved
      std::vector<WriteCtx> all_after;  // every write row after the spawn (own and in callees)
      std::set<std::string> own_typed;  // the fields with a typed write of the creator's own after the spawn
      std::set<std::string> callee_typed;
      std::set<std::string> record_types;  // the record types written whole after the spawn
      std::vector<std::string> unknown;
      auto note_unknown = [&](uint32_t ci, const std::string& via) {
        const std::string t = call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + via + " (" + (calls_[ci].kind == KZ_CALL_KIND_INDIRECT ? "indirect call, targets not resolved" : "?") + ")";
        if (std::find(unknown.begin(), unknown.end(), t) == unknown.end()) unknown.push_back(t);
      };
      auto ait = after_spawn_.find(site);
      if (ait == after_spawn_.end()) ++no_paths;
      if (ait != after_spawn_.end()) {
        for (uint32_t li : ait->second) {
          const kz_fact_lockset& w = lockset_[li];
          if (w.kind != KZ_LOCKSET_KIND_WRITE) continue;
          const std::string type = str(w.type), field = str(w.field);
          if (type.empty() || type == "<global>") continue;
          WriteCtx wc;
          wc.row = li;
          wc.fn = P;
          all_after.push_back(wc);
          if (field == "*") record_types.insert(type);
          else own_typed.insert(type + "." + field);
        }
        for (uint32_t li : ait->second) {
          const kz_fact_lockset& c = lockset_[li];
          if (c.kind != KZ_LOCKSET_KIND_CALL || c.site == site) continue;
          auto cc = call_by_site_.find(c.site);
          if (cc == call_by_site_.end()) continue;
          if (unresolved_sites_.count(c.site) != 0) note_unknown(cc->second, "");
          for (uint32_t g : call_runs(cc->second))
            for (uint32_t h : closure(g)) {
              auto hc = calls_of_.find(h);
              if (hc != calls_of_.end())
                for (uint32_t hci : hc->second)
                  if (unresolved_sites_.count(calls_[hci].site) != 0) note_unknown(hci, " (reached from the call " + call_pos(cc->second) + ")");
              auto hw = lockset_writes_of_.find(h);
              if (hw == lockset_writes_of_.end()) continue;
              for (uint32_t wi : hw->second) {
                const kz_fact_lockset& w = lockset_[wi];
                const std::string type = str(w.type), field = str(w.field);
                if (type.empty() || type == "<global>") continue;
                WriteCtx wc;
                wc.row = wi;
                wc.call_row = li;
                wc.first_level = h == g;
                wc.fn = h;
                all_after.push_back(wc);
                if (field == "*") record_types.insert(type);
                else callee_typed.insert(type + "." + field);
              }
            }
        }
      }
      // the stores a derived spawn operation makes through its arguments (PARAM_FLOW out-store): inside the call, in an
      // order to its own thread creation and to its lock operations that the facts do not give, so after the spawn and
      // under no known lock.  The out argument of a P0 thread creation is stored before the new thread starts (the
      // semantics of the P0 entries of that kind state it).
      std::map<std::string, std::vector<WriteCtx>> arg_stores;
      if (defined(s.callee_fn))
        for (uint32_t k = 0; k < s.nargs; ++k) {
          auto pit = pflow_of_.find({s.callee_fn, k});
          if (pit == pflow_of_.end()) continue;
          bool out = false;
          for (uint32_t pi : pit->second)
            if (pflow_[pi].kind == KZ_PFLOW_KIND_OUT_STORE) out = true;
          auto aa = argflow_at_.find({site, k});
          if (!out || aa == argflow_at_.end() || argflow_[aa->second].src != KZ_ARGSRC_ADDRESS) continue;
          const std::string type = str(argflow_[aa->second].type), field = str(argflow_[aa->second].field);
          if (type.empty() || field.empty() || type == "<global>") continue;
          WriteCtx wc;
          wc.fn = s.callee_fn;
          wc.spawn_arg = k;
          arg_stores[type + "." + field].push_back(wc);
          own_typed.insert(type + "." + field);
        }
      for (const std::string& tf : callee_typed)
        if (own_typed.count(tf) == 0) ++callee_only_fields;
      // the creator's callers: what they run after it returns is outside ED §3.3 item 3 (the creator's writes); their
      // own bodies are read without order (a write of the field there may follow the return)
      std::vector<uint32_t> pcalls;
      for (uint32_t ci : callers_[P]) pcalls.push_back(ci);
      auto pic = indirect_callers_.find(P);
      if (pic != indirect_callers_.end()) pcalls.insert(pcalls.end(), pic->second.begin(), pic->second.end());
      std::set<uint32_t> pcallers;
      for (uint32_t ci : pcalls) pcallers.insert(calls_[ci].caller_fn);
      std::string pcallers_names;
      for (uint32_t q : pcallers) pcallers_names += (pcallers_names.empty() ? "" : ", ") + fname(q);
      for (const std::string& tf : own_typed) {
        const std::string owner = owner_type(tf);
        const std::set<std::tuple<uint32_t, std::string, uint32_t, uint32_t>> covers = covers_of(tf);
        std::vector<WriteCtx> wrows;
        for (const WriteCtx& wc : all_after) {
          const kz_fact_lockset& w = lockset_[wc.row];
          const std::string type = str(w.type), field = str(w.field);
          WriteCtx x = wc;
          if (type + "." + field == tf) {
            x.how = "";
          } else if (field == "*" && type == owner) {
            x.how = "record";
          } else if (covers.count({w.fn, str(w.file), w.line, w.col}) != 0) {
            x.how = "covers";
          } else {
            continue;
          }
          wrows.push_back(x);
        }
        auto as = arg_stores.find(tf);
        if (as != arg_stores.end()) wrows.insert(wrows.end(), as->second.begin(), as->second.end());
        if (wrows.empty()) continue;
        auto rit = reads_of_field_.find(tf);
        if (rit == reads_of_field_.end()) continue;
        CExtra x;
        x.site = site;
        x.creator = P;
        x.tf = tf;
        x.unknown = unknown;
        x.callers_names = pcallers_names;
        for (uint32_t q : pcallers) {
          auto qw = lockset_writes_of_.find(q);
          if (qw != lockset_writes_of_.end())
            for (uint32_t wi : qw->second) {
              const kz_fact_lockset& w = lockset_[wi];
              const std::string type = str(w.type), field = str(w.field);
              if (type + "." + field == tf || (field == "*" && type == owner) || covers.count({w.fn, str(w.file), w.line, w.col}) != 0)
                x.callers.push_back(pos(str(w.file), w.line, w.col) + " in " + fname(q) + (field == "*" ? " (the whole record)" : ""));
            }
          auto qc = calls_of_.find(q);
          if (qc != calls_of_.end())
            for (uint32_t qci : qc->second)
              if (unresolved_sites_.count(calls_[qci].site) != 0) x.callers.push_back(call_pos(qci) + " in " + fname(q) + " (indirect call, targets not resolved)");
        }
        for (uint32_t ri : rit->second) {
          const kz_fact_read& u = reads_[ri];
          if ((u.use & (KZ_READ_USE_BRANCH | KZ_READ_USE_RETURN | KZ_READ_USE_OUT_STORE)) == 0) continue;
          const std::set<uint32_t> ua = actor_set(u.fn);
          bool in_child = false;
          for (uint32_t a : ua)
            if (child.count(a) != 0) in_child = true;
          if (!in_child || u.fn == P) continue;
          // the contexts of the read in the child: the read itself when it holds a static lock of its own; else the
          // read run as the entry of a spawned thread (no caller lock), and each direct or indirect call of its
          // function (a call the spawned entries do not reach gets no grade)
          std::vector<Ctx> ctxs;
          auto lr = lockset_read_.find(ri);
          const uint32_t urow = lr == lockset_read_.end() ? kN : lr->second;
          bool own_static = false;
          if (urow != kN)
            for (uint32_t id : held(urow))
              if (static_lock(id)) own_static = true;
          if (own_static) {
            ctxs.push_back(Ctx{"the read itself", urow, kN, false, false});
          } else {
            if (child.count(u.fn) != 0 || resolved.count(u.fn) != 0) ctxs.push_back(Ctx{"the read run as the entry of the spawned thread, under no lock of a caller", urow, kN, true, false});
            std::vector<std::pair<uint32_t, bool>> calls;  // (call, indirect)
            for (uint32_t ci : callers_[u.fn]) calls.push_back({ci, false});
            auto ic = indirect_callers_.find(u.fn);
            if (ic != indirect_callers_.end())
              for (uint32_t ci : ic->second) calls.push_back({ci, true});
            for (const auto& [ci, indirect] : calls) {
              const std::set<uint32_t> ca = actor_set(calls_[ci].caller_fn);
              bool cc = false;
              for (uint32_t a : ca)
                if (child.count(a) != 0) cc = true;
              auto cr = lockset_call_.find(calls_[ci].site);
              ctxs.push_back(Ctx{std::string(indirect ? "via the indirect call " : "via the call ") + call_pos(ci) + " in " + fname(calls_[ci].caller_fn) +
                                     (cc ? "" : "; the spawned entries do not reach this call"),
                                 urow, cr == lockset_call_.end() ? kN : cr->second, false, !cc});
            }
            if (ctxs.empty()) ctxs.push_back(Ctx{"the read itself", urow, kN, false, false});
          }
          for (const Ctx& cx : ctxs) {
            begin_row();
            note_p0_chain(spawn_chain(site));
            // self identity of the read's subscript
            std::string self;
            for (const std::string& g : calls_in(u.index_root)) {
              auto fit = fn_by_name_.find(g);
              if (fit == fn_by_name_.end()) continue;
              for (uint32_t gf : fit->second) {
                std::string ch;
                auto pk = p0_kind_.find(g);
                if (pk != p0_kind_.end() && (pk->second == KZ_P0_KIND_THREAD_SELF || pk->second == KZ_P0_KIND_TSD_GET)) ch = g + " (P0 " + enum_text("KZ_P0_KIND", pk->second) + ")";
                else if (defined(gf)) ch = chain_to_p0(gf, {KZ_P0_KIND_THREAD_SELF, KZ_P0_KIND_TSD_GET});
                if (!ch.empty()) self = ch;
              }
            }
            if (!self.empty()) {
              const size_t sp = self.find(" (P0 ");
              const std::string head = sp == std::string::npos ? self : self.substr(0, sp);
              note_p0_chain(head);
            }
            ++n_c_;
            if (!self.empty()) ++n_c_self_;
            if (cx.not_reached) ++not_reached_ctx;
            rows << "  C" << n_c_ << " spawn " << call_pos(si) << " in " << fname(P) << " -> " << fname(s.callee_fn == kN ? s.caller_fn : s.callee_fn) << "; field " << tf
                 << "; read " << pos(str(u.file), u.line, u.col) << " in " << fname(u.fn) << " (" << cx.where << ")" << (self.empty() ? "" : " self-identity") << "\n";
            rows << "      spawn: callee " << str(s.callee) << ", entries";
            for (uint32_t e : resolved) rows << " " << fname(e);
            rows << (image_spawn_.count(site) != 0 ? " (from the static image of the table the entry argument is read from)" : "");
            rows << "; child actors " << actors_text(child) << "; creator identity: " << (identity.empty() ? "none found" : identity) << "; locks held " << s_held << "\n";
            for (const WriteCtx& wc : wrows) {
              if (wc.spawn_arg != kN) {
                auto aa = argflow_at_.find({site, wc.spawn_arg});
                rows << "      write after the spawn by the spawn operation: " << fname(wc.fn) << " stores through arg " << wc.spawn_arg << " ["
                     << (aa != argflow_at_.end() ? str(argflow_[aa->second].text) : std::string("?")) << "] inside the call at " << call_pos(si)
                     << " (PARAM_FLOW out-store; its order to the thread creation inside the call is not read) locks not known\n";
                continue;
              }
              const kz_fact_lockset& w = lockset_[wc.row];
              std::string key;
              auto wr = writers_of_.find(tf);
              if (wr != writers_of_.end() && wc.how.empty())
                for (uint32_t i : wr->second)
                  if (writers_[i].fn == w.fn && writers_[i].line == w.line && str(writers_[i].file) == str(w.file))
                    key = " key=" + str(writers_[i].key) + " value=" + (writers_[i].value_kind == KZ_ARG_KIND_CONST ? std::to_string(writers_[i].value) : enum_text("KZ_ARG_KIND", writers_[i].value_kind)) +
                          " [" + str(writers_[i].value_text) + "]";
              const std::string how = wc.how == "record" ? " (a write of the whole record " + str(w.type) + ")"
                                      : wc.how == "covers" ? " (a write of " + str(w.type) + "." + str(w.field) + " whose size may reach " + tf + ": covers-more-than-the-field)"
                                                           : "";
              if (wc.call_row == kN) {
                rows << "      write after the spawn: " << pos(str(w.file), w.line, w.col) << key << how << " locks " << held_text(wc.row) << "\n";
              } else {
                const kz_fact_lockset& c = lockset_[wc.call_row];
                rows << "      write after the spawn in " << fname(w.fn) << " (through the call " << pos(str(c.file), c.line, c.col) << "): " << pos(str(w.file), w.line, w.col) << key << how
                     << " locks " << held_text(wc.row) << "; at the call " << held_text(wc.call_row);
                if (!wc.first_level) rows << " (not composed: the write is more than one call below the creator)";
                else if (entry_release_.count(wc.row) != 0) rows << " (not composed: " << release_text(wc.row) << ")";
                rows << "\n";
              }
            }
            if (!unknown.empty()) rows << "      calls after the spawn whose targets are not resolved (they may write " << tf << " under no known lock): " << join(unknown, "; ") << "\n";
            rows << "      read: [" << str(u.text) << "] use=" << enum_text("KZ_READ_USE", u.use) << " index=" << ex(u.index_root) << " guard=" << ex(u.guard_root)
                 << " locks " << (cx.row == kN ? "{} (no LOCKSET row)" : held_text(cx.row));
            if (cx.call_row != kN) {
              rows << "; at the call " << held_text(cx.call_row);
              if (cx.row != kN && entry_release_.count(cx.row) != 0) rows << " (not composed: " << release_text(cx.row) << ")";
            }
            rows << "\n";
            if (!self.empty()) rows << "      read subscript: the reader's own identity: " << self << "\n";
            rows << "      dependency: path C: the creator writes " << tf << " after the spawn; code the spawned entry reaches reads it" << (self.empty() ? "" : " at the subscript of its own identity")
                 << "; that the two identities are equal is not decided statically\n";
            rows << "      run-order: no record given\n";
            std::string feas;
            if (cx.not_reached) {
              feas = "not graded: the spawned entries do not reach this call in the static actor sets, so this context is no read of the child; no grade is given from the "
                     "locks at the call";
              tally_feas("not-graded");
            } else {
              feas = c_feasibility(lsit == lockset_call_.end() ? kN : lsit->second, wrows, cx, x);
            }
            rows << "      feasibility: " << feas << "\n";
            rows << "      violation: " << violation_text() << "\n";
            // flip: the reader's constant returns that depend on the field, before and after the write
            std::string grade = "none", why = "the reader's function returns no constant that depends on " + tf;
            const std::vector<Check> checks = checks_on_field(u.fn, tf);
            if (!checks.empty()) {
              int64_t iv = 0;
              std::string itext;
              const bool has_init = init_value(tf, &iv, &itext);
              std::vector<std::pair<int64_t, std::string>> wvals;
              auto wr = writers_of_.find(tf);
              if (wr != writers_of_.end())
                for (uint32_t i : wr->second)
                  if (writers_[i].fn == P && writers_[i].value_kind == KZ_ARG_KIND_CONST) wvals.push_back({writers_[i].value, str(writers_[i].value_text)});
              std::set<std::string> unknown_vals;
              bool possible = false, any_unknown = false;
              std::string detail;
              for (const Check& ck : checks) {
                Subst before_write;
                if (has_init) before_write.fields[tf] = iv;
                const Val vi = has_init ? eval(ck.cond_root, before_write, &unknown_vals) : Val{};
                if (!has_init) unknown_vals.insert("no initial value of " + tf);
                for (const auto& [v, vt] : wvals) {
                  Subst sw;
                  sw.fields[tf] = v;
                  const Val vw = eval(ck.cond_root, sw, &unknown_vals);
                  if (vi.known && vw.known && vi.v != vw.v) {
                    possible = true;
                    detail += std::string(detail.empty() ? "" : "; ") + fname(u.fn) + " returns " + std::to_string(ck.value) + " [" + ck.text + "] " +
                              (vi.v ? "with" : "not with") + " the earlier value (" + itext + "), " + (vw.v ? "with" : "not with") + " the written value " + std::to_string(v) + " [" + vt + "]";
                  }
                  if (!vw.known) any_unknown = true;
                }
                if (!vi.known || wvals.empty()) any_unknown = true;
              }
              if (wvals.empty()) unknown_vals.insert("no constant written value of " + tf + " in " + fname(P) + " (not a target, or not a constant)");
              if (possible) {
                grade = "static-possible";
                why = detail;
              } else if (any_unknown) {
                grade = "static-undetermined";
                std::string un;
                for (const std::string& v : unknown_vals) un += (un.empty() ? "" : "; ") + v;
                why = "not evaluated: " + un;
              } else {
                grade = "none";
                why = "the earlier and the written values give the same constant returns";
              }
            }
            tally_flip(grade);
            rows << "      flip: " << grade << ": " << why << "\n";
            rows << "      layers: " << layers_text(true, uses_script_root(child) || uses_script_root(ua)) << "\n";
          }
        }
      }
      for (const std::string& rt : record_types) {
        bool any = false;
        for (const std::string& tf : own_typed)
          if (owner_type(tf) == rt) any = true;
        if (!any) ++record_only_fields;
      }
    }
    os << "[C] " << n_c_ << " (spawn calls " << spawn_calls << ", with no resolved entry " << unresolved << ", with no event after them on their control-flow graph " << no_paths
       << "; fields written after a spawn only in the functions the creator calls, not paired in this revision: " << callee_only_fields
       << "; record types written whole after a spawn with no typed write of a field of theirs by the creator, not paired: " << record_only_fields
       << "; read contexts through a call the spawned entries do not reach (rows with no grade): " << not_reached_ctx << ")\n"
       << rows.str();
  }
  // the WRITERS rows of a field that are data functions on another field whose size may reach it (covers-more), by place
  std::set<std::tuple<uint32_t, std::string, uint32_t, uint32_t>> covers_of(const std::string& tf) const {
    std::set<std::tuple<uint32_t, std::string, uint32_t, uint32_t>> out;
    auto wit = writers_of_.find(tf);
    if (wit == writers_of_.end()) return out;
    for (uint32_t i : wit->second)
      if (writers_[i].flags & KZ_WRITER_FLAG_COVERS_MORE) out.insert({writers_[i].fn, str(writers_[i].file), writers_[i].line, writers_[i].col});
    return out;
  }
  // the functions a call may run: its defined callee, its resolved indirect targets, and the functions passed as
  // arguments to a callee without a body that is no P0 entry (it may call them back)
  std::set<uint32_t> call_runs(uint32_t ci) const {
    std::set<uint32_t> t;
    const kz_fact_call& c = calls_[ci];
    if (defined(c.callee_fn)) t.insert(c.callee_fn);
    auto itg = itargets_.find(c.site);
    if (itg != itargets_.end()) t.insert(itg->second.begin(), itg->second.end());
    const std::string callee = str(c.callee);
    if (!defined(c.callee_fn) && c.kind != KZ_CALL_KIND_INDIRECT && p0_kind_.count(callee) == 0 && compare_names_.count(callee) == 0)
      for (uint32_t k = 0; k < c.nargs; ++k) {
        auto af = argflow_at_.find({c.site, k});
        if (af == argflow_at_.end() || argflow_[af->second].src != KZ_ARGSRC_FUNCTION) continue;
        auto fit = fn_by_name_.find(base_name(str(argflow_[af->second].root)));
        if (fit != fn_by_name_.end())
          for (uint32_t f : fit->second)
            if (defined(f)) t.insert(f);
      }
    return t;
  }
  // the functions a function may run (call_runs, transitively), itself included
  const std::set<uint32_t>& closure(uint32_t g) {
    auto it = closure_.find(g);
    if (it != closure_.end()) return it->second;
    std::set<uint32_t> seen;
    std::vector<uint32_t> stack{g};
    seen.insert(g);
    while (!stack.empty()) {
      const uint32_t x = stack.back();
      stack.pop_back();
      auto cit = calls_of_.find(x);
      if (cit == calls_of_.end()) continue;
      for (uint32_t ci : cit->second)
        for (uint32_t y : call_runs(ci))
          if (seen.insert(y).second) stack.push_back(y);
    }
    return closure_[g] = seen;
  }
  std::string release_text(uint32_t row) const {
    auto it = entry_release_.find(row);
    if (it == entry_release_.end()) return "";
    auto cit = call_by_site_.find(it->second);
    return "on a path from the entry of " + fname(lockset_[row].fn) + " the call " + (cit != call_by_site_.end() ? call_pos(cit->second) + " " + str(calls_[cit->second].callee) : "site " + std::to_string(it->second)) +
           " releases a lock " + fname(lockset_[row].fn) + " does not hold, may release one, or has a lock effect not followed (LOCK_PATHS entry-release), so a lock held at the call is not known to be held here";
  }
  // the locks a write after the spawn holds, and the acquisitions they come from: its own (relative to its function's
  // entry), and those held at the creator's call it is under when it is in the function that call calls and no call on
  // a path from that function's entry releases one (LOCK_PATHS entry-release); a write deeper down holds its own only
  std::set<uint32_t> ctx_held(const WriteCtx& w) const {
    std::set<uint32_t> h;
    if (w.spawn_arg != kN) return h;
    for (uint32_t id : held(w.row)) h.insert(id);
    if (w.call_row != kN && w.first_level && entry_release_.count(w.row) == 0)
      for (uint32_t id : held(w.call_row)) h.insert(id);
    return h;
  }
  std::set<uint32_t> ctx_openers(const WriteCtx& w) const {
    if (w.spawn_arg != kN) return {};
    std::set<uint32_t> o = openers(w.row);
    if (w.call_row != kN && w.first_level && entry_release_.count(w.row) == 0) {
      const std::set<uint32_t> c = openers(w.call_row);
      o.insert(c.begin(), c.end());
    }
    return o;
  }
  // an acquisition call of the set that is itself an event after the spawn on the creator's control-flow graph (the lock
  // may be released and taken again between the spawn and a write)
  std::string reached_again(uint32_t site, const std::set<uint32_t>& op) const {
    auto ait = after_spawn_.find(site);
    if (ait == after_spawn_.end()) return "";
    for (uint32_t o : op) {
      auto r = lockset_call_.find(o);
      auto cit = call_by_site_.find(o);
      if (r != lockset_call_.end() && ait->second.count(r->second) != 0 && cit != call_by_site_.end()) return call_pos(cit->second);
    }
    return "";
  }
  // what the static-instance status of a lock rests on (named in the row's layers)
  void tag_static() const {
    for (const char* a : {"assume.lock_acquisition_succeeds", "assume.static_instance_read_after_write", "assume.no_cross_member_pointer_arithmetic"})
      if (assumptions_.count(a) != 0) tag(a);
    tag("per-field completeness rule (PI decision pending)");
  }
  std::string where_of(const WriteCtx& w) const {
    if (w.spawn_arg != kN) return "the store of the spawn operation through arg " + std::to_string(w.spawn_arg);
    return pos(str(lockset_[w.row].file), lockset_[w.row].line, lockset_[w.row].col);
  }
  // The other order (the read between the spawn and the writes after it) under the lock sets.  A lock rules the read out
  // only when it is held from one acquisition at the spawn and at every write after it, that acquisition is not reached
  // again after the spawn, the read holds it, and its identity is a static instance (must identity, ED §3.3); a lock
  // held so whose identity is not a static instance gives `undecidable`.  `proven-impossible` needs, besides: the field's
  // writer set complete under the per-field rule (so the typed, record-wide and covering writes after the spawn are all
  // its writes, and an untyped store does not reach it under that rule), no call after the spawn whose targets are not
  // resolved, and no write of the field in the creator's callers' own bodies.  Its scope is one call of the creator (ED
  // §3.3 item 3): what the callers run after it returns is not read beyond their own bodies.
  std::string c_feasibility(uint32_t s_row, const std::vector<WriteCtx>& w_rows, const Ctx& cx, const CExtra& x) {
    std::string grade, text;
    const uint32_t u_row = cx.row, call_row = cx.call_row;
    auto fail = [&](uint32_t row) { return row != kN && (lockset_[row].flags & KZ_LOCKSET_FLAG_FAILURE_PATH) != 0; };
    std::set<uint32_t> hu;
    if (u_row != kN)
      for (uint32_t id : held(u_row)) hu.insert(id);
    bool dropped = false;
    if (call_row != kN) {
      if (u_row != kN && entry_release_.count(u_row) != 0) dropped = true;
      else
        for (uint32_t id : held(call_row)) hu.insert(id);
    }
    // the locks every write after the spawn holds (any identity)
    std::set<uint32_t> common;
    bool first = true;
    const WriteCtx* unlocked = nullptr;
    for (const WriteCtx& wr : w_rows) {
      const std::set<uint32_t> h = ctx_held(wr);
      if (h.empty() && unlocked == nullptr) unlocked = &wr;
      if (first) common = h;
      else {
        std::set<uint32_t> keep;
        for (uint32_t id : common)
          if (h.count(id) != 0) keep.insert(id);
        common = keep;
      }
      first = false;
    }
    std::vector<uint32_t> hs;
    std::set<uint32_t> so;
    if (s_row != kN) {
      hs = held(s_row);
      so = openers(s_row);
    }
    // a lock held from the spawn's acquisition across every write after it, not taken again after the spawn
    auto continuous = [&](uint32_t id, std::string* why) {
      if (s_row == kN || std::find(hs.begin(), hs.end(), id) == hs.end()) {
        *why = "lock window: " + lock_text(id) + " is not held at the spawn; the writes after it hold it from a later acquisition, so the read may come before them";
        return false;
      }
      for (const WriteCtx& wr : w_rows)
        if (ctx_openers(wr) != so) {
          *why = "lock " + lock_text(id) + " is held at the spawn and at the writes from different acquisitions: it may be released between them";
          return false;
        }
      std::set<uint32_t> all = so;
      for (const WriteCtx& wr : w_rows) {
        const std::set<uint32_t> o = ctx_openers(wr);
        all.insert(o.begin(), o.end());
      }
      const std::string again = reached_again(x.site, all);
      if (!again.empty()) {
        *why = "lock " + lock_text(id) + " is held at the spawn and at the writes after it, but its acquisition " + again +
               " is reached again after the spawn on the creator's control-flow graph (LOCK_PATHS after-spawn), so it may be released and taken again between them";
        return false;
      }
      return true;
    };
    if (fail(u_row) || fail(call_row)) {
      grade = "undecidable";
      text = "the read lies on a failure path of a lock acquisition (assume.lock_acquisition_succeeds does not cover it): the locks it holds there are not known";
    } else if (hu.empty()) {
      grade = "possible";
      text = cx.entry ? "the read holds no lock (it runs as the entry of the spawned thread, under no lock of a caller)" : "the read holds no lock";
    } else {
      uint32_t proven = kN, undec = kN, other = kN;
      std::string other_why;
      for (uint32_t id : hu) {
        if (common.count(id) == 0) continue;
        std::string why;
        if (continuous(id, &why)) {
          if (static_lock(id)) {
            if (proven == kN) proven = id;
          } else if (undec == kN) {
            undec = id;
          }
        } else if (other == kN) {
          other = id;
          other_why = why;
        }
      }
      if (proven != kN) {
        tag_static();
        const std::string held_how = "lock " + lock_text(proven) + " is held from the same acquisition across the spawn and every write of " + x.tf + " after it in this call of " +
                                     fname(x.creator) + ", that acquisition is not reached again after the spawn, and the read holds it";
        auto fs = fstatus_of_.find(x.tf);
        if (fs == fstatus_of_.end() || fstatus_[fs->second].status_field != KZ_LOC_STATUS_COMPLETE) {
          grade = "undecidable";
          text = held_how + "; but the writer set of " + x.tf + " is not complete under the per-field rule (FIELD_STATUS " +
                 (fs == fstatus_of_.end() ? std::string("has no row") : "per-field " + enum_text("KZ_LOC_STATUS", fstatus_[fs->second].status_field)) +
                 "), so the writes after the spawn are not known to be all its writes";
        } else if (writers_of_.count(x.tf) == 0 && fstatus_[fs->second].n_data_fn != 0) {
          grade = "undecidable";
          text = held_how + "; but " + x.tf + " has " + std::to_string(fstatus_[fs->second].n_data_fn) +
                 " data-function writers (FIELD_STATUS) whose places these facts do not give (the field is no extraction target)";
        } else if (!x.unknown.empty()) {
          grade = "undecidable";
          text = held_how + "; but a call after the spawn whose targets are not resolved may write " + x.tf + " under no known lock: " + join(x.unknown, "; ");
        } else if (!x.callers.empty()) {
          grade = "undecidable";
          text = held_how + "; but the callers of " + fname(x.creator) + " write " + x.tf + " in their own bodies, and their order to its return is not in these facts: " + join(x.callers, "; ");
        } else {
          grade = "proven-impossible";
          text = held_how + ": the read cannot fall between them.  Scope: one call of " + fname(x.creator) +
                 " (ED §3.3 item 3), on its control-flow graph from the spawn to its return, loops included (LOCK_PATHS after-spawn); the writes counted are the typed writes of " +
                 x.tf + ", the writes of the whole record " + owner_type(x.tf) +
                 ", the data functions that may cover the field, and those of the functions the creator calls after the spawn; " + x.tf +
                 " is writer-set-complete under the per-field rule (an untyped store does not reach it under that rule); the callers of " + fname(x.creator) + " (" +
                 (x.callers_names.empty() ? std::string("none in these facts") : x.callers_names) +
                 ") write it nowhere in their own bodies; what their callees and their own callers run after it returns is not read";
          std::string on;
          for (const char* a : {"assume.lock_acquisition_succeeds", "assume.static_instance_read_after_write", "assume.no_cross_member_pointer_arithmetic"})
            if (assumptions_.count(a) != 0) on += std::string(on.empty() ? "" : ", ") + a;
          on += std::string(on.empty() ? "" : ", ") + "the per-field completeness rule of FIELD_STATUS that makes " + lock_text(proven) + " a static instance and " + x.tf +
                " complete (PI decision pending)";
          if (proven < lockids_.size() && (lockids_[proven].flags & KZ_LOCKID_FLAG_TYPE_EXPOSED) != 0) on += "; under the type-level rule the lock is no static instance";
          text += "; rests on " + on;
        }
        note_lock_prims();
      } else if (undec != kN) {
        grade = "undecidable";
        text = "lock " + lock_text(undec) + " is held from the same acquisition across the spawn and every write after it, and the read holds it, but its identity is no static "
               "instance: that the read's lock and the writes' lock are one mutex is not proven (must identity, ED §3.3)";
        note_lock_prims();
      } else if (other != kN) {
        grade = "possible";
        text = other_why;
        if (static_lock(other)) tag_static();
        note_lock_prims();
      } else {
        // the read and the writes hold no common identity
        uint32_t wl = kN;
        for (uint32_t id : common) {
          std::string why;
          if (continuous(id, &why)) {
            wl = id;
            break;
          }
        }
        if (wl != kN) {
          uint32_t alias = kN;
          for (uint32_t r : hu)
            if (!(static_lock(r) && static_lock(wl))) {
              alias = r;
              break;
            }
          note_lock_prims();
          if (alias != kN) {
            grade = "undecidable";
            text = "the writes after the spawn hold " + lock_text(wl) + " from the spawn's acquisition and the read holds " + lock_text(alias) +
                   ": their identities differ and one is no static instance, so whether they are one mutex is not proven (must identity, ED §3.3)";
          } else {
            grade = "possible";
            text = "the read holds " + lock_text(*hu.begin()) + " and the writes after the spawn hold " + lock_text(wl) + ": two different static instances, no common lock";
            tag_static();
          }
        } else {
          grade = "possible";
          text = unlocked != nullptr ? "a write after the spawn (" + where_of(*unlocked) + ") holds no lock, so the read may come before it"
                                     : "the read and the writes after the spawn hold no common lock held from the spawn's acquisition";
          if (unlocked == nullptr) note_lock_prims();
        }
      }
    }
    if (dropped) text += "; the locks held at the call are not counted at the read: " + release_text(u_row);
    tally_feas(grade);
    return grade + ": " + text;
  }

  // ---------------------------------------------------------------- path A
  struct KeyCmp {
    uint32_t fn = kN;          // M: the function with the comparison
    std::string key;           // Type.field
    uint32_t param = kN;       // the parameter compared with it, or kN
    uint32_t site = kN;        // the comparison call, or kN for `==`
    uint32_t param_arg = kN;   // the argument of the comparison call the parameter is
    std::string how;           // "strcmp at ..." or "== at ..."
    std::string operand;       // what the key is compared with
    uint32_t loop = kN;        // the loop around it in M (FACT_LOOPS index), or kN
    uint32_t read = kN;        // the READS row of the key
  };
  struct Namespace {
    std::string type, key;
    std::vector<KeyCmp> cmps;
    std::set<std::string> active;
    std::set<std::string> tables;
    std::vector<std::string> searches;
    std::set<std::pair<uint32_t, uint32_t>> key_params;  // (fn, param) reaching a comparison
  };

  std::string loop_pos(uint32_t loop) const {
    auto it = loop_by_index_.find(loop);
    if (it == loop_by_index_.end()) return "loop " + std::to_string(loop);
    const kz_fact_loop& l = loops_[it->second];
    return pos(str(l.file), l.line, l.col) + " in " + str(l.func);
  }
  std::string owner_type(const std::string& tf) const {
    const size_t dot = tf.rfind('.');
    return dot == std::string::npos ? tf : tf.substr(0, dot);
  }

  void key_comparisons(std::vector<KeyCmp>* out) const {
    // (a) a call of the comparison family of P0 with a field and a parameter as its compared arguments
    for (size_t ci = 0; ci < calls_.size(); ++ci) {
      const kz_fact_call& c = calls_[ci];
      // a body the program itself defines shadows the library function; an inline body (a system header's
      // extern inline definition) is the library's own
      if (compare_names_.count(str(c.callee)) == 0 || c.nargs < 2) continue;
      if (defined(c.callee_fn) && (funcs_[c.callee_fn].flags & KZ_FUNC_FLAG_INLINE) == 0) continue;
      auto a0 = argflow_at_.find({c.site, 0}), a1 = argflow_at_.find({c.site, 1});
      if (a0 == argflow_at_.end() || a1 == argflow_at_.end()) continue;
      for (int side = 0; side < 2; ++side) {
        const kz_fact_argflow& f = argflow_[side == 0 ? a0->second : a1->second];
        const kz_fact_argflow& o = argflow_[side == 0 ? a1->second : a0->second];
        const std::string type = str(f.type), field = str(f.field);
        if (f.src != KZ_ARGSRC_FIELD || type.empty() || field.empty() || type == "<global>") continue;
        KeyCmp k;
        k.fn = c.caller_fn;
        k.key = type + "." + field;
        k.site = c.site;
        k.read = f.read;
        k.loop = c.loop;
        k.how = str(c.callee) + " at " + call_pos(ci);
        if (o.src == KZ_ARGSRC_PARAM || ((o.src == KZ_ARGSRC_DEREF || o.base == KZ_ARGBASE_PARAM) && o.param != kN)) {
          k.param = o.param;
          k.param_arg = side == 0 ? 1 : 0;
          k.operand = std::string(o.src == KZ_ARGSRC_PARAM ? "parameter " : "the pointee of parameter ") + std::to_string(o.param) + " [" + str(o.text) + "]";
        } else {
          k.operand = enum_text("KZ_ARGSRC", o.src) + " [" + str(o.text) + "]";
        }
        out->push_back(k);
      }
    }
    // (b) `==` / `!=` between a field and a parameter, a local or a call result, in a branch condition
    for (size_t ri = 0; ri < reads_.size(); ++ri) {
      const kz_fact_read& r = reads_[ri];
      if (r.kind != KZ_READ_KIND_FIELD || (r.use & KZ_READ_USE_BRANCH) == 0 || r.ctx_root == kN || r.ctx_root >= exprs_.size()) continue;
      const kz_fact_expr& e = exprs_[r.ctx_root];
      if ((e.op != KZ_EXPR_OP_EQ && e.op != KZ_EXPR_OP_NE) || e.a == kN || e.b == kN) continue;
      const std::string tf = str(r.type) + "." + str(r.field);
      for (int side = 0; side < 2; ++side) {
        const uint32_t fa = side == 0 ? e.a : e.b, oa = side == 0 ? e.b : e.a;
        const kz_fact_expr& f = exprs_[fa];
        const kz_fact_expr& o = exprs_[oa];
        if (!(f.op == KZ_EXPR_OP_LOAD && f.flags == KZ_EXPR_REF_FIELD && str(f.name) == tf)) continue;
        const bool param = o.op == KZ_EXPR_OP_LOAD && o.flags == KZ_EXPR_REF_PARAM;
        const bool local = o.op == KZ_EXPR_OP_LOAD && o.flags == KZ_EXPR_REF_LOCAL;
        const bool call = o.op == KZ_EXPR_OP_CALL_RESULT;
        if (!param && !local && !call) continue;
        if (r.loop == kN && !param) continue;  // a local or a call result: only in a search loop
        KeyCmp k;
        k.fn = r.fn;
        k.key = tf;
        k.read = static_cast<uint32_t>(ri);
        k.loop = r.loop;
        k.how = std::string(e.op == KZ_EXPR_OP_EQ ? "==" : "!=") + " at " + pos(str(r.file), r.line, r.col);
        if (param) {
          k.param = static_cast<uint32_t>(o.value);
          k.operand = "parameter " + std::to_string(o.value) + " [" + str(o.name) + "]";
        } else {
          k.operand = ex(oa);
        }
        out->push_back(k);
      }
    }
  }

  // the searches a comparison serves: its own loop (A1), or a loop of a function whose indirect call
  // reaches the comparison's function (A2)
  struct Search {
    uint32_t fn = kN, loop = kN, param = kN;  // the search function, its loop, the parameter carried to the key
    std::string kind;
  };
  std::vector<Search> searches_of(const KeyCmp& k) const {
    std::vector<Search> out;
    if (k.loop != kN) out.push_back(Search{k.fn, k.loop, k.param, "A1 search loop"});
    auto it = indirect_callers_.find(k.fn);
    if (it != indirect_callers_.end())
      for (uint32_t ci : it->second) {
        const kz_fact_call& c = calls_[ci];
        if (c.loop == kN) continue;
        uint32_t p = kN;
        if (k.param != kN) {
          auto af = argflow_at_.find({c.site, k.param});
          if (af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_PARAM) p = argflow_[af->second].param;
        }
        out.push_back(Search{c.caller_fn, c.loop, p, "A2 callback search (indirect call " + call_pos(ci) + ")"});
      }
    return out;
  }
  // the record type's other fields in the exit conditions of a loop
  std::set<std::string> loop_fields(uint32_t loop, const std::string& type) const {
    std::set<std::string> out;
    auto it = loopexits_of_.find(loop);
    if (it == loopexits_of_.end()) return out;
    for (uint32_t i : it->second)
      for (uint32_t root : {loopexits_[i].root, loopexits_[i].aux_root})
        for (const std::string& f : fields_in(root))
          if (owner_type(f) == type) out.insert(f);
    return out;
  }
  // per (fn, param): the parameters whose value reaches a comparison (PARAM_FLOW)
  void key_params(const KeyCmp& k, std::set<std::pair<uint32_t, uint32_t>>* out) const {
    if (k.param == kN) return;
    out->insert({k.fn, k.param});
    for (const kz_fact_pflow& p : pflow_) {
      if (k.site != kN) {
        if (p.kind == KZ_PFLOW_KIND_CALL_ARG && p.sink_site == k.site && p.n == k.param_arg) out->insert({p.fn, p.param});
        if (p.kind == KZ_PFLOW_KIND_CALL_ARG && p.depth == 0 && p.site == k.site && p.fn == k.fn && p.param == k.param) out->insert({p.fn, p.param});
      } else if (p.kind == KZ_PFLOW_KIND_COMPARE && p.sink_fn == k.fn && k.read < reads_.size() && p.line == reads_[k.read].line) {
        out->insert({p.fn, p.param});
      }
    }
  }
  // the constants of the first call down the chain from (fn, param) to the comparison that passes one;
  // `desc` names them (value, text as written, callee and argument)
  std::string partition(uint32_t fn, uint32_t param, std::set<std::pair<uint32_t, uint32_t>>* seen, const std::set<std::pair<uint32_t, uint32_t>>& kp,
                        std::string* desc) const {
    if (!seen->insert({fn, param}).second) return "";
    auto it = pflow_of_.find({fn, param});
    if (it == pflow_of_.end()) return "";
    // the depth-0 call arguments of this parameter to another key parameter
    std::vector<const kz_fact_pflow*> steps;
    for (uint32_t i : it->second) {
      const kz_fact_pflow& p = pflow_[i];
      if (p.kind == KZ_PFLOW_KIND_CALL_ARG && p.depth == 0 && p.next_fn != kN && kp.count({p.next_fn, p.next_param}) != 0) steps.push_back(&p);
    }
    std::sort(steps.begin(), steps.end(), [](const kz_fact_pflow* a, const kz_fact_pflow* b) { return a->site < b->site; });
    for (const kz_fact_pflow* p : steps) {
      std::string consts;
      auto cit = call_by_site_.find(p->site);
      if (cit != call_by_site_.end())
        for (uint32_t a = 0; a < calls_[cit->second].nargs; ++a) {
          auto af = argflow_at_.find({p->site, a});
          if (af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_CONST) {
            consts += (consts.empty() ? "" : ",") + std::to_string(argflow_[af->second].value);
            *desc += std::string(desc->empty() ? "" : ", ") + std::to_string(argflow_[af->second].value) + " [" + str(argflow_[af->second].text) + "] as " +
                     str(calls_[cit->second].callee) + " arg " + std::to_string(a) + " at " + call_pos(cit->second);
          }
        }
      if (!consts.empty()) return consts;
      const std::string deeper = partition(p->next_fn, p->next_param, seen, kp, desc);
      if (!deeper.empty()) return deeper;
    }
    return "";
  }
  struct KeyValue {
    std::string status;  // "known", "unresolved"
    std::set<std::string> values;
    std::string text;
  };
  // the key a use passes: a literal, a constant, or a field whose static image the facts reach
  KeyValue key_value(uint32_t site, uint32_t arg) const {
    KeyValue kv;
    auto af = argflow_at_.find({site, arg});
    if (af == argflow_at_.end()) {
      kv.status = "unresolved";
      kv.text = "no ARGFLOW row";
      return kv;
    }
    const kz_fact_argflow& a = argflow_[af->second];
    if (a.src == KZ_ARGSRC_STRING) {
      kv.status = "known";
      kv.values.insert("\"" + kz::escape_string(str(a.root)) + "\"");
      kv.text = "string literal \"" + kz::escape_string(str(a.root)) + "\"";
      return kv;
    }
    if (a.src == KZ_ARGSRC_CONST) {
      kv.status = "known";
      kv.values.insert(std::to_string(a.value));
      kv.text = "constant " + std::to_string(a.value);
      return kv;
    }
    if (a.src == KZ_ARGSRC_FIELD && a.read != kN) {
      const std::string field = str(a.field);
      auto pit = ptrread_of_.find(a.read);
      std::string object;
      if (pit != ptrread_of_.end()) {
        const kz_fact_ptrfield& p = ptrfields_[pit->second];
        if (p.status == KZ_PTRF_STATUS_UNRESOLVED) {
          kv.status = "unresolved";
          kv.text = "read through the pointer field " + str(p.type) + "." + str(p.field) + ": needs field points-to (PTR_FIELDS)";
          return kv;
        }
        object = str(p.object);
        kv.text = "read through the pointer field " + str(p.type) + "." + str(p.field) + " -> " + object + " (PTR_FIELDS " + enum_text("KZ_PTRF_STATUS", p.status) + ")";
        if (p.idx_n != 0) kv.text += ", elements from " + std::to_string(p.elem) + " step " + std::to_string(p.idx_step) + " count " + std::to_string(p.idx_n);
        // the image of the object, at the elements the reader's index takes
        auto git = gimages_of_.find(base_name(object));
        bool written = false;
        if (git != gimages_of_.end()) {
          std::set<int64_t> elems;
          if (p.idx_n != 0)
            for (uint32_t k = 0; k < p.idx_n; ++k) elems.insert(p.elem + static_cast<int64_t>(k) * p.idx_step);
          for (uint32_t gi : git->second) {
            const kz_fact_gimage& g = gimages_[gi];
            if (str(g.field) != field) continue;
            const std::string path = str(g.path);
            if (!elems.empty()) {
              bool in = false;
              for (int64_t e : elems)
                if (path.compare(0, ("[" + std::to_string(e) + "]").size(), "[" + std::to_string(e) + "]") == 0) in = true;
              if (!in) continue;
            }
            const std::string val = g.kind == KZ_IMAGE_KIND_STRING ? "\"" + kz::escape_string(str(g.text)) + "\"" : g.kind == KZ_IMAGE_KIND_INT ? std::to_string(g.value) : str(g.text);
            kv.values.insert(val);
            kv.text += "; image " + base_name(object) + path + " = " + val;
            const std::string macro = str(g.macro);
            if (!macro.empty()) kv.text += " (macro " + macro + ", spelled at " + pos(str(g.spell_file), g.spell_line, g.spell_col) + ")";
            if (g.flags & (KZ_GIMAGE_FLAG_FIELD_WRITTEN | KZ_GIMAGE_FLAG_RECORD_WRITTEN)) {
              kv.text += " (" + written_flags(g.flags & (KZ_GIMAGE_FLAG_FIELD_WRITTEN | KZ_GIMAGE_FLAG_RECORD_WRITTEN)) + ")";
              written = true;
            }
          }
        }
        if (written) {
          // the image is not the run-time value: no exclusion and no flip rests on it
          kv.status = "unresolved";
          kv.text += "; unresolved (image, field written at run time)";
          return kv;
        }
        kv.status = kv.values.empty() ? "unresolved" : "known";
        if (kv.values.empty()) kv.text += "; no image leaf of the field";
        return kv;
      }
      kv.status = "unresolved";
      kv.text = "field " + str(a.type) + "." + field + " read with base " + enum_text("KZ_ARGBASE", a.base) + " (no static image reached)";
      return kv;
    }
    kv.status = "unresolved";
    kv.text = enum_text("KZ_ARGSRC", a.src) + " [" + str(a.text) + "]";
    return kv;
  }

  // the substitution under which a key comparison does not match: a comparison call of P0 returns non-zero; a `==` is
  // false and a `!=` true
  Subst no_match(const KeyCmp& k) const {
    Subst sb;
    if (k.site != kN) {
      auto cit = call_by_site_.find(k.site);
      if (cit != call_by_site_.end()) sb.nonzero_calls.insert(str(calls_[cit->second].callee));
    } else if (k.read != kN && k.read < reads_.size() && reads_[k.read].ctx_root != kN && reads_[k.read].ctx_root < exprs_.size()) {
      sb.forced[reads_[k.read].ctx_root] = exprs_[reads_[k.read].ctx_root].op == KZ_EXPR_OP_NE ? 1 : 0;
    }
    return sb;
  }
  // The values a lookup's callee returns when its search finds no record: the constant returns of the search function
  // (RET_CONSTS) whose condition holds when the key comparison does not match (for a callback search, the callback's
  // returns evaluated so give its result, which must be one known value), carried unchanged up to the lookup's callee
  // over RETURNS rows that return a call result.  `match` gets the other constant returns of those searches.  Empty
  // when that is not established.
  std::set<int64_t> nomatch_values(const Namespace& n, uint32_t lookup_fn, std::set<int64_t>* match, std::string* via) const {
    std::map<uint32_t, Subst> subs;
    for (const KeyCmp& k : n.cmps)
      for (const Search& sr : searches_of(k)) {
        if (subs.count(sr.fn) != 0) continue;
        const Subst cmp = no_match(k);
        if (sr.fn == k.fn) {
          subs[sr.fn] = cmp;
          continue;
        }
        // a callback search: the callback's result when its comparison does not match
        std::set<int64_t> vals;
        bool ok = true;
        auto rit = returns_of_.find(k.fn);
        if (rit == returns_of_.end()) continue;
        for (uint32_t ri : rit->second) {
          const kz_fact_return& r = returns_[ri];
          if (r.cond_root != kN) {
            const Val c = eval(r.cond_root, cmp, nullptr);
            if (c.known && c.v == 0) continue;
          }
          if (r.vkind == KZ_RET_VALUE_CONST) {
            vals.insert(r.value);
            continue;
          }
          const Val v = eval(r.value_root, cmp, nullptr);
          if (!v.known) {
            ok = false;
            break;
          }
          vals.insert(v.v);
        }
        if (!ok || vals.size() != 1) continue;
        Subst sb;
        sb.calls["<indirect>"] = *vals.begin();
        subs[sr.fn] = sb;
      }
    std::map<uint32_t, std::set<int64_t>> nm;
    for (const auto& [fn, sb] : subs) {
      auto it = retconsts_of_.find(fn);
      if (it == retconsts_of_.end()) continue;
      for (uint32_t i : it->second) {
        const kz_fact_retconst& r = retconsts_[i];
        const Val c = r.cond_root == kN ? known(1) : eval(r.cond_root, sb, nullptr);
        if (c.known && c.v != 0) nm[fn].insert(r.value);
        else if (match != nullptr) match->insert(r.value);
      }
    }
    std::set<int64_t> out;
    std::set<uint32_t> seen;
    std::vector<uint32_t> work{lookup_fn};
    std::map<uint32_t, uint32_t> parent;
    while (!work.empty()) {
      const uint32_t f = work.back();
      work.pop_back();
      if (!defined(f) || !seen.insert(f).second) continue;
      auto ni = nm.find(f);
      if (ni != nm.end() && !ni->second.empty()) {
        out.insert(ni->second.begin(), ni->second.end());
        std::vector<std::string> chain;
        for (uint32_t x = f; x != kN; x = parent.count(x) != 0 ? parent[x] : kN) chain.push_back(fname(x));
        std::reverse(chain.begin(), chain.end());
        if (via != nullptr && via->empty()) *via = join(chain, " -> ");
        continue;
      }
      auto rit = returns_of_.find(f);
      if (rit == returns_of_.end()) continue;
      for (uint32_t ri : rit->second) {
        const kz_fact_return& r = returns_[ri];
        if (r.vkind != KZ_RET_VALUE_CALL || r.value_root >= exprs_.size() || exprs_[r.value_root].op != KZ_EXPR_OP_CALL_RESULT) continue;
        auto fit = fn_by_name_.find(str(exprs_[r.value_root].name));
        if (fit == fn_by_name_.end()) continue;
        for (uint32_t g : fit->second)
          if (defined(g) && seen.count(g) == 0) {
            if (parent.count(g) == 0) parent[g] = f;
            work.push_back(g);
          }
      }
    }
    if (match != nullptr)
      for (int64_t v : out) match->erase(v);
    return out;
  }

  void path_a(std::ostream& os) {
    std::vector<KeyCmp> cmps;
    key_comparisons(&cmps);
    std::map<std::string, Namespace> spaces;
    for (const KeyCmp& k : cmps) {
      const std::vector<Search> ss = searches_of(k);
      if (ss.empty()) continue;
      Namespace& n = spaces[k.key];
      n.key = k.key;
      n.type = owner_type(k.key);
      n.cmps.push_back(k);
      for (const Search& s : ss) {
        n.searches.push_back(s.kind + " " + loop_pos(s.loop) + ": key " + k.key + " " + k.how + " compared with " + k.operand +
                             (s.param != kN ? " (search parameter " + std::to_string(s.param) + ")" : ""));
        for (const std::string& f : loop_fields(s.loop, n.type))
          if (f != k.key) n.active.insert(f);
        // the table: the global objects the search (or the comparison's function) reads the record type from
        for (const kz_fact_argflow& a : argflow_)
          if ((a.fn == s.fn || a.fn == k.fn) && str(a.type) == n.type && (a.base == KZ_ARGBASE_GLOBAL_ELEMENT || a.base == KZ_ARGBASE_GLOBAL) && !str(a.object).empty())
            n.tables.insert(base_name(str(a.object)));
        for (const kz_fact_read& r : reads_)
          if ((r.fn == s.fn || r.fn == k.fn) && str(r.type) == n.type && r.root_kind == KZ_READ_ROOT_GLOBAL) n.tables.insert(str(r.root));
        if (s.param != kN) n.key_params.insert({s.fn, s.param});
      }
      key_params(k, &n.key_params);
    }
    // the active fields of a record type: those any search over it tests; and the values the searches
    // read as "in use" and "empty": an active-field test (`== c`, `!= c`) in a search loop that has a key
    // comparison
    std::map<std::string, std::set<std::string>> active_of_type;
    for (const auto& [key, n] : spaces) active_of_type[n.type].insert(n.active.begin(), n.active.end());
    for (const auto& [key, n] : spaces)
      for (const KeyCmp& k : n.cmps)
        for (const Search& sr : searches_of(k))
          for (const kz_fact_read& r : reads_) {
            const std::string tf = str(r.type) + "." + str(r.field);
            if (r.fn != sr.fn || r.loop != sr.loop || tf == n.key || owner_type(tf) != n.type || r.ctx_root == kN || r.ctx_root >= exprs_.size()) continue;
            const kz_fact_expr& e = exprs_[r.ctx_root];
            if ((e.op != KZ_EXPR_OP_EQ && e.op != KZ_EXPR_OP_NE) || e.b == kN || e.b >= exprs_.size() || exprs_[e.b].op != KZ_EXPR_OP_CONST) continue;
            (e.op == KZ_EXPR_OP_EQ ? in_use_ : empty_)[tf].insert(exprs_[e.b].value);
          }
    std::ostringstream rows;
    uint32_t no_table = 0;
    for (auto& [key, n] : spaces) {
      // a registration table is shared: the searches read the record type from a global object
      if (n.tables.empty()) {
        ++no_table;
        continue;
      }
      ++n_ns_;
      begin_row();
      rows << "  namespace " << key << " (record type " << n.type << "; tables";
      if (n.tables.empty()) rows << " none found";
      for (const std::string& t : n.tables) rows << " " << t;
      rows << ")\n";
      std::set<std::string> uniq(n.searches.begin(), n.searches.end());
      for (const std::string& s : uniq) rows << "      search: " << s << "\n";
      rows << "      active fields: ";
      if (n.active.empty()) rows << "none in this namespace's searches";
      for (const std::string& f : n.active) rows << f << " ";
      const std::set<std::string>& others = active_of_type[n.type];
      std::set<std::string> missing;
      for (const std::string& f : others)
        if (n.active.count(f) == 0 && f != n.key) missing.insert(f);
      if (n.active.empty() && !missing.empty()) {
        rows << "; no activity check: other searches of " << n.type << " test";
        for (const std::string& f : missing) rows << " " << f;
      }
      rows << "\n";
      const std::map<std::string, std::set<int64_t>>& in_use = in_use_;
      const std::map<std::string, std::set<int64_t>>& empty = empty_;
      for (const auto& [f, v] : in_use)
        if (owner_type(f) == n.type)
          for (int64_t x : v) rows << "      active test: " << f << " == " << x << " is in use (a search with the key comparison requires it)\n";
      for (const auto& [f, v] : empty)
        if (owner_type(f) == n.type)
          for (int64_t x : v) rows << "      active test: " << f << " == " << x << " is empty (a search with the key comparison skips it)\n";
      // writes of the key and the active fields, and the P0 primitives that write them
      std::set<std::string> watch(n.active.begin(), n.active.end());
      watch.insert(n.key);
      for (const std::string& f : others) watch.insert(f);
      for (const std::string& f : watch) {
        std::string src;
        const std::vector<WriterRow> ws = writers_of(f, &src);
        uint32_t covers = 0;
        for (const WriterRow& w : ws)
          if (w.flags & KZ_WRITER_FLAG_COVERS_MORE) ++covers;
        rows << "      lifecycle " << f << ": writers " << ws.size() << " from " << (src.empty() ? "nothing" : src)
             << (covers != 0 ? " (" + std::to_string(covers) + " of them write another field with a size that may reach this one: covers-more-than-the-field)" : "") << "\n";
        for (const WriterRow& w : ws) {
          const std::set<uint32_t> as = actor_set(w.fn);
          std::string cls;
          if (as.size() == 1 && root_kind_.count(*as.begin()) != 0 && root_kind_.at(*as.begin()) == "process") cls = " (process actor only)";
          auto ev = empty.find(f);
          auto uv = in_use.find(f);
          const bool init = !cls.empty() && (w.flags & (KZ_WRITER_FLAG_IN_LOOP | KZ_WRITER_FLAG_EARLY_INIT)) != 0;
          if (w.known && ev != empty.end() && ev->second.count(w.value) != 0)
            cls += init ? " class=initialization (writes the empty value, in a loop or early init of the process actor)" : " class=release (writes the empty value)";
          else if (w.known && uv != in_use.end() && uv->second.count(w.value) != 0) cls += " class=reservation or activation (writes the in-use value)";
          rows << "        " << w.kind << " at " << w.where << " value=" << w.value_text << (w.guard.empty() || w.guard == "-" ? "" : " guard=" + w.guard) << cls
               << ((w.flags & KZ_WRITER_FLAG_COVERS_MORE) ? " covers-more-than-the-field" : "") << "\n";
        }
        for (const kz_fact_argflow& a : argflow_) {
          if (a.src != KZ_ARGSRC_ADDRESS || str(a.type) + "." + str(a.field) != f) continue;
          auto cit = call_by_site_.find(a.site);
          if (cit == call_by_site_.end()) continue;
          const std::string callee = str(calls_[cit->second].callee);
          for (const kz_fact_p0& p : p0_)
            if (str(p.name) == callee && p.out_param == a.arg_index && !defined(calls_[cit->second].callee_fn))
              rows << "        P0 out-pointer: " << callee << " (P0 " << enum_text("KZ_P0_KIND", p.kind) << ") stores through arg " << a.arg_index << " [" << str(a.text) << "] at "
                   << call_pos(cit->second) << " in " << fname(a.fn)
                   << " (joined here from ARGFLOW and P0_TABLE: the extraction's writer sets -- WRITERS, FACT_LOCS, FIELD_STATUS -- have no row for this store)\n";
        }
      }
      // the spawn operations that write the table: what runs when the entry they start returns
      lifecycle_entry_paths(n, rows);
      path_a_uses(n, rows);
    }
    os << "[A] " << n_a_ << " (namespaces " << n_ns_ << ", key comparisons " << cmps.size() << "; left out: searches over no global table " << no_table << ")\n" << rows.str();
  }

  // for a function that both writes a field of the namespace and passes the entry of a spawn: whether the
  // entry it passes to the thread-creation primitive is its caller's entry (then no function of the program
  // runs when that entry returns) or a function of the program
  void lifecycle_entry_paths(const Namespace& n, std::ostream& rows) const {
    std::set<std::string> fields(n.active.begin(), n.active.end());
    fields.insert(n.key);
    std::set<uint32_t> writers;
    for (const std::string& f : fields) {
      std::string src;
      for (const WriterRow& w : writers_of(f, &src)) writers.insert(w.fn);
      for (const kz_fact_argflow& a : argflow_)
        if (a.src == KZ_ARGSRC_ADDRESS && str(a.type) + "." + str(a.field) == f) writers.insert(a.fn);
    }
    for (const kz_fact_spawnw& w : spawnw_) {
      if (w.kind != KZ_SPAWNW_KIND_ENTRY_ARG || writers.count(w.fn) == 0) continue;
      const std::string prim = str(w.primitive);
      auto cit = call_by_site_.find(w.site);
      std::string entry_text;
      auto ent = spawn_entries_.find(w.site);
      if (ent != spawn_entries_.end())
        for (uint32_t e : ent->second)
          if (defined(e)) entry_text += " " + fname(e);
      rows << "      entry path: " << fname(w.fn) << " parameter " << w.param << " is the entry of the spawn " << (cit != call_by_site_.end() ? call_pos(cit->second) : "?") << " (" << prim
           << ", depth " << w.depth << ", chain " << str(w.chain) << ")";
      if (w.depth == 1 && !prim.empty() && cit != call_by_site_.end() && str(calls_[cit->second].callee) == prim) {
        rows << ": the parameter is passed unchanged to the thread-creation primitive, so the new thread starts at the caller's entry (P0 thread creation: the "
             << "thread runs that function and ends when it returns)";
        if (keys_dtor_.empty() && cleanup_pushes_.empty()) {
          for (const std::string& k : keys_null_dtor_) note_p0(k.substr(0, k.find(' ')));
          rows << "; no function of this program runs after that entry returns (thread-specific keys created with a null destructor only: "
               << (keys_null_dtor_.empty() ? std::string("none created") : join(keys_null_dtor_, "; ")) << "; no cleanup handler push in the program), so the table has no write on that path"
               << " (not read in this revision: handlers registered for process exit (atexit, at_quick_exit, on_exit), destructor functions, and units built with "
                  "exceptions, where pthread_cleanup_push calls no function; they run, if at all, when the process ends or through the unwinder)";
        } else {
          rows << "; after that entry returns the thread may still run functions of this program (" << (keys_dtor_.empty() ? "" : "thread-specific destructors: " + join(keys_dtor_, "; "))
               << (keys_dtor_.empty() || cleanup_pushes_.empty() ? "" : "; ") << (cleanup_pushes_.empty() ? "" : "cleanup handler pushes: " + join(cleanup_pushes_, "; "))
               << "), so no claim about the table on that path is made";
        }
        rows << (entry_text.empty() ? "" : " (entries the facts resolve:" + entry_text + ")") << "\n";
      } else
        rows << ": the thread starts at a function of the program" << (entry_text.empty() ? "" : " (" + entry_text.substr(1) + ")") << "\n";
    }
  }

  void path_a_uses(const Namespace& n, std::ostream& rows) {
    // the functions that reach a write of the key (direct calls): a use of one is a creator
    std::set<uint32_t> writer_fns;
    {
      auto fit = fwrites_of_.find(n.key);
      if (fit != fwrites_of_.end())
        for (uint32_t i : fit->second) writer_fns.insert(fwrites_[i].fn);
      auto wit = writers_of_.find(n.key);
      if (wit != writers_of_.end())
        for (uint32_t i : wit->second)
          if (writers_[i].kind != KZ_WRITER_KIND_MEMSET || !(writers_[i].flags & KZ_WRITER_FLAG_EARLY_INIT)) writer_fns.insert(writers_[i].fn);
    }
    struct Use {
      uint32_t ci = kN, callee = kN, param = kN;
      bool creator = false;
      std::string part, part_desc;
      KeyValue key;
      std::set<uint32_t> actors;
    };
    std::vector<Use> uses;
    for (const auto& kp : n.key_params) {
      const uint32_t F = kp.first, k = kp.second;
      for (uint32_t ci : callers_[F]) {
        const kz_fact_call& c = calls_[ci];
        auto af = argflow_at_.find({c.site, k});
        if (af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_PARAM && n.key_params.count({c.caller_fn, argflow_[af->second].param}) != 0) continue;
        Use u;
        u.ci = ci;
        u.callee = F;
        u.param = k;
        bool creates = false;
        for (uint32_t f : reach_direct(F))
          if (writer_fns.count(f) != 0) creates = true;
        u.creator = creates;
        std::set<std::pair<uint32_t, uint32_t>> seen;
        u.part = partition(F, k, &seen, n.key_params, &u.part_desc);
        u.key = key_value(c.site, k);
        u.actors = actor_set(c.caller_fn);
        uses.push_back(u);
      }
    }
    std::sort(uses.begin(), uses.end(), [&](const Use& a, const Use& b) { return calls_[a.ci].site < calls_[b.ci].site; });
    // the key-parameter functions, as lookups or creators; for a creator, the writes of the key and the
    // active fields in the functions it reaches through direct calls (its publication writes)
    {
      std::string lk, cr;
      std::set<std::string> watch(n.active.begin(), n.active.end());
      watch.insert(n.key);
      std::vector<WriterRow> all;
      for (const std::string& f : watch) {
        std::string src;
        for (WriterRow w : writers_of(f, &src)) {
          w.kind = f + " " + w.kind;
          all.push_back(w);
        }
      }
      for (const auto& [F, k] : n.key_params) {
        bool creates = false;
        const std::set<uint32_t> reach = reach_direct(F);
        for (uint32_t f : reach)
          if (writer_fns.count(f) != 0) creates = true;
        const std::string name = fname(F) + "#" + std::to_string(k);
        if (!creates) {
          lk += " " + name;
          continue;
        }
        cr += " " + name;
        std::string pub;
        for (const WriterRow& w : all)
          if (reach.count(w.fn) != 0)
            pub += std::string(pub.empty() ? "" : "; ") + w.kind + " " + pos(w.file, w.line, w.col) + " in " + fname(w.fn) + (w.known ? " := " + std::to_string(w.value) : "");
        if (!pub.empty()) rows << "      publication writes of the creator " << name << ": " << pub << "\n";
      }
      rows << "      key-parameter functions: lookups" << (lk.empty() ? " none" : lk) << "; creators" << (cr.empty() ? " none" : cr) << "\n";
    }
    uint32_t lookups = 0, creators = 0;
    bool ns_l0 = false;
    for (const Use& u : uses) {
      (u.creator ? creators : lookups)++;
      if (uses_script_root(u.actors)) ns_l0 = true;
    }
    rows << "      uses: lookups " << lookups << ", creators " << creators << " (calls from outside the key-parameter functions; a creator reaches a write of the key)\n";
    rows << "      namespace layers: " << layers_text(true, ns_l0) << "\n";
    for (const Use& u : uses) {
      if (u.creator) continue;
      begin_row();
      // the comparison functions of P0 the namespace's searches use
      for (const KeyCmp& k : n.cmps)
        if (k.site != kN) {
          auto kc = call_by_site_.find(k.site);
          if (kc != call_by_site_.end()) note_p0(str(calls_[kc->second].callee));
        }
      ++n_a_;
      const kz_fact_call& c = calls_[u.ci];
      rows << "  A" << n_a_ << " lookup " << call_pos(u.ci) << " in " << fname(c.caller_fn) << " -> " << fname(u.callee) << "#" << u.param << " namespace " << n.key << " partition {"
           << u.part << "} key " << u.key.status << "\n";
      if (!u.part_desc.empty()) rows << "      partition: " << u.part_desc << "\n";
      rows << "      key: " << u.key.text << "\n";
      rows << "      actors: " << actors_text(u.actors) << "; result=" << result_use(u.ci) << "\n";
      std::vector<std::string> paired, excluded, probes;
      bool any_inter = false, any_match = false, any_unres = false;
      uint32_t n_probe = 0, n_differ = 0, n_same_actor = 0;
      for (const Use& w : uses) {
        if (!w.creator || w.part != u.part) continue;
        const kz_fact_call& wc = calls_[w.ci];
        std::string why;
        // probe-create (ED §3.5): the same function, the same key argument, the creator's key read under the lookup's
        // result, and on the failure polarity: the read runs when the lookup returns what its search returns on no
        // match, and not for another value the guard compares the result with or the search returns
        bool probe = false;
        std::string polarity, not_probe;
        if (wc.caller_fn == c.caller_fn) {
          auto a1 = argflow_at_.find({wc.site, w.param}), a0 = argflow_at_.find({c.site, u.param});
          if (a1 != argflow_at_.end() && a0 != argflow_at_.end() && str(argflow_[a1->second].text) == str(argflow_[a0->second].text)) {
            const uint32_t rd = argflow_[a1->second].read;
            if (rd != kN && rd < reads_.size() && has_call(reads_[rd].guard_root, str(c.callee))) {
              const uint32_t G = reads_[rd].guard_root;
              std::set<int64_t> match;
              std::string via;
              const std::set<int64_t> F = nomatch_values(n, c.callee_fn, &match, &via);
              bool f_ok = !F.empty();
              for (int64_t f : F) {
                Subst sb;
                sb.calls[str(c.callee)] = f;
                const Val g = eval(G, sb, nullptr);
                if (g.known && g.v == 0) f_ok = false;
              }
              std::set<int64_t> V = match;
              visit(G, [&](const kz_fact_expr& e) {
                if ((e.op != KZ_EXPR_OP_EQ && e.op != KZ_EXPR_OP_NE) || e.a == kN || e.b == kN) return;
                const kz_fact_expr& x0 = exprs_[e.a];
                const kz_fact_expr& x1 = exprs_[e.b];
                if (x0.op == KZ_EXPR_OP_CALL_RESULT && str(x0.name) == str(c.callee) && x1.op == KZ_EXPR_OP_CONST) V.insert(x1.value);
                if (x1.op == KZ_EXPR_OP_CALL_RESULT && str(x1.name) == str(c.callee) && x0.op == KZ_EXPR_OP_CONST) V.insert(x0.value);
              });
              std::string off;
              for (int64_t v : V) {
                if (F.count(v) != 0) continue;
                Subst sb;
                sb.calls[str(c.callee)] = v;
                const Val g = eval(G, sb, nullptr);
                if (g.known && g.v == 0) off += (off.empty() ? "" : ", ") + std::to_string(v);
              }
              std::string fs;
              for (int64_t f : F) fs += (fs.empty() ? "" : ", ") + std::to_string(f);
              if (f_ok && !off.empty()) {
                probe = true;
                polarity = "the creator's key read runs when " + str(c.callee) + " returns " + fs + " (no match: " + via + ") and not when it returns " + off;
              } else {
                not_probe = F.empty() ? "the value the lookup returns on no match is not established from RET_CONSTS and RETURNS"
                                      : (!f_ok ? "the creator's key read does not run when the lookup returns " + fs + " (no match)"
                                               : "the creator's key read is not excluded for any other value of the lookup's result");
              }
            }
          }
        }
        bool same_actor_only = !u.actors.empty() && u.actors == w.actors && u.actors.size() == 1;
        std::string keyrel;
        if (u.key.status == "known" && w.key.status == "known") {
          bool meet = false;
          for (const std::string& v : u.key.values)
            if (w.key.values.count(v) != 0) meet = true;
          keyrel = meet ? "same key" : "different keys";
        } else {
          keyrel = "key unresolved (" + std::string(u.key.status != "known" ? "lookup" : "creator") + ")";
        }
        const std::string line = call_pos(w.ci) + " in " + fname(wc.caller_fn) + " -> " + fname(w.callee) + " key " + w.key.text + " actors " + actors_text(w.actors);
        if (probe) {
          ++n_probe;
          excluded.push_back("probe-create (same function, the same key argument, the creator's key read under the lookup's result; " + polarity + "): " + line);
          probes.push_back(call_pos(w.ci) + " -> " + fname(w.callee) + " (" + polarity + ")");
          continue;
        }
        if (!not_probe.empty()) excluded.push_back("not a probe-create although in the same function under the lookup's result: " + not_probe + ": " + line);
        if (keyrel == "different keys") {
          ++n_differ;
          excluded.push_back("key constants differ: " + line);
          continue;
        }
        if (same_actor_only) {
          ++n_same_actor;
          excluded.push_back("same actor only (program order): " + line);
          continue;
        }
        any_inter = true;
        if (keyrel == "same key") any_match = true;
        else any_unres = true;
        paired.push_back(keyrel + ": " + line);
      }
      if (any_inter) ++n_a_paired_;
      if (any_match) ++n_a_keymatch_;
      for (const std::string& p : paired) rows << "      creator: " << p << "\n";
      for (const std::string& x : excluded) rows << "      not a pair: " << x << "\n";
      // handling of the result
      rows << a_handling(u.ci);
      for (const std::string& pc : probes) rows << "      handling: probe-create: on the lookup's failure result the same function creates the same key: " << pc << "\n";
      std::string dep;
      if (!paired.empty()) {
        dep = std::to_string(paired.size()) + " creator(s) of this partition in other actors";
      } else if (n_probe + n_differ + n_same_actor == 0) {
        dep = "no creator of this partition";
      } else {
        dep = "no creator left after exclusions (probe-create " + std::to_string(n_probe) + ", key constants differ " + std::to_string(n_differ) + ", same actor only " +
              std::to_string(n_same_actor) + ")";
      }
      rows << "      dependency: path A: " << dep << "\n";
      rows << "      run-order: no record given\n";
      std::string feas;
      if (paired.empty()) {
        feas = "undecidable: no pair";
      } else {
        feas = "undecidable: the facts give no static order between the lookup's actor and the creators' actors (creation order does not fix the order of initialization)";
      }
      tally_feas("undecidable");
      rows << "      feasibility: " << feas << "\n";
      rows << "      violation: " << violation_text() << "\n";
      std::string grade, why;
      if (any_match) {
        grade = "static-possible";
        why = "a creator of the same key and partition in another actor: the lookup's failure return becomes success if its publication comes first";
      } else if (any_unres) {
        grade = "static-undetermined";
        why = "the key of the lookup or of a creator is not resolved statically";
      } else {
        grade = "none";
        why = "no creator of the same partition and key in another actor";
      }
      const std::string fails = search_failures(n);
      if (!fails.empty()) why += "; " + fails;
      tally_flip(grade);
      rows << "      flip: " << grade << ": " << why << "\n";
      bool l0 = uses_script_root(u.actors);
      rows << "      layers: " << layers_text(true, l0) << "\n";
    }
  }
  // the constant returns of the searches of a namespace: the failure constant (returned when no
  // record matches) and the success constant
  std::string search_failures(const Namespace& n) const {
    std::set<std::string> out;
    for (const KeyCmp& k : n.cmps)
      for (const Search& s : searches_of(k)) {
        auto it = retconsts_of_.find(s.fn);
        if (it == retconsts_of_.end()) continue;
        for (uint32_t i : it->second) out.insert(fname(s.fn) + " returns " + std::to_string(retconsts_[i].value) + " [" + str(retconsts_[i].text) + "]");
      }
    std::string t;
    for (const std::string& s : out) t += (t.empty() ? "" : ", ") + s;
    return t.empty() ? "" : "search returns: " + t;
  }
  // what the caller does with a lookup's result: a retry loop around it, and where the failure leads
  std::string a_handling(uint32_t ci) const {
    std::ostringstream os;
    const kz_fact_call& c = calls_[ci];
    if (c.loop != kN) {
      auto lit = loop_by_index_.find(c.loop);
      if (lit != loop_by_index_.end()) {
        const kz_fact_loop& l = loops_[lit->second];
        os << "      handling: in the loop " << pos(str(l.file), l.line, l.col) << " kind " << enum_text("KZ_LOOP_KIND", l.kind) << " trip " << enum_text("KZ_TRIP_STATUS", l.trip_status)
           << " " << l.trip;
        std::string exit;
        auto eit = loopexits_of_.find(l.loop);
        if (eit != loopexits_of_.end())
          for (uint32_t i : eit->second)
            if (loopexits_[i].kind == KZ_LOOPEXIT_KIND_LOOP) exit = ex(loopexits_[i].root);
        os << " exit " << (exit.empty() ? ex(l.exit_root) : exit);
        if (l.delay_site != kN) {
          auto dit = call_by_site_.find(l.delay_site);
          if (dit != call_by_site_.end()) {
            const std::string chain = opderiv_chain(calls_[dit->second].callee_fn, KZ_OPDERIV_KIND_DELAY);
            os << " delay " << call_pos(dit->second) << " " << str(calls_[dit->second].callee) << (chain.empty() ? "" : " (F8 " + chain + ")");
          }
        }
        os << "\n";
      }
    }
    // the failure upward: the functions that return the result, and the terminal operations and the field
    // writes on the way
    std::set<uint32_t> seen;
    std::vector<std::pair<uint32_t, uint32_t>> work{{c.caller_fn, ci}};  // (function, the call whose result it holds)
    std::vector<std::string> path;
    while (!work.empty()) {
      const auto [fn, call] = work.back();
      work.pop_back();
      if (!seen.insert(fn).second) continue;
      const std::string callee = str(calls_[call].callee);
      // terminal calls in fn guarded by the result
      auto tit = terminals_of_.find(fn);
      if (tit != terminals_of_.end())
        for (uint32_t ti : tit->second) {
          const kz_fact_terminal& t = terminals_[ti];
          auto tcall = call_by_site_.find(t.site);
          const std::string top = str(t.op);
          std::string chain;
          if (tcall != call_by_site_.end()) chain = opderiv_chain(calls_[tcall->second].callee_fn, KZ_OPDERIV_KIND_TERMINAL);
          const bool guarded = has_call(t.guard_root, callee);
          // a field written under the result (or a local of it) and passed to the terminal operation
          std::string via;
          std::map<uint32_t, std::set<int64_t>> argvals;  // the constant values the terminal call's arguments take on this path
          if (tcall != call_by_site_.end())
            for (uint32_t a = 0; a < calls_[tcall->second].nargs; ++a) {
              auto af = argflow_at_.find({t.site, a});
              if (af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_CONST) argvals[a].insert(argflow_[af->second].value);
              if (af == argflow_at_.end() || argflow_[af->second].src != KZ_ARGSRC_FIELD) continue;
              const std::string tf = str(argflow_[af->second].type) + "." + str(argflow_[af->second].field);
              auto wit = writers_of_.find(tf);
              if (wit == writers_of_.end()) continue;
              for (uint32_t wi : wit->second) {
                const kz_fact_writer& w = writers_[wi];
                if (w.fn != fn || w.guard_root == kN || (w.kind != KZ_WRITER_KIND_ASSIGN && w.kind != KZ_WRITER_KIND_INCDEC)) continue;
                const bool direct = has_call(w.guard_root, callee);
                bool local = false;
                visit(w.guard_root, [&](const kz_fact_expr& e) {
                  if (e.op == KZ_EXPR_OP_LOAD && e.flags == KZ_EXPR_REF_LOCAL) local = true;
                });
                if (!direct && !(local && (calls_[call].flags & KZ_CALL_FLAG_RESULT_USED))) continue;
                if (w.value_kind == KZ_ARG_KIND_CONST) argvals[a].insert(w.value);
                via += std::string(via.empty() ? "" : "; ") + "the field " + tf + " := " + str(w.value_text) + " at " + pos(str(w.file), w.line, w.col) + " under " + ex(w.guard_root) +
                       (direct ? " (the guard tests the result)" : " (the guard tests a local; the result of " + callee + " is used in " + fname(fn) + ": may)") + " is argument " +
                       std::to_string(a) + " of it";
              }
            }
          if (guarded || !via.empty()) {
            const std::string may = tcall != call_by_site_.end() ? may_terminal(calls_[tcall->second].callee_fn, argvals) : "";
            path.push_back("terminal " + top + " at " + pos(str(t.file), t.line, t.col) + " in " + fname(fn) + (guarded ? " under the result" : "") + (via.empty() ? "" : ": " + via) +
                           (chain.empty() ? "" : " (F8 " + chain + (may.empty() ? "" : "; " + may) + ")"));
          }
        }
      // returned to the callers
      auto rit = returns_of_.find(fn);
      bool returned = false;
      if (rit != returns_of_.end())
        for (uint32_t ri : rit->second)
          if (returns_[ri].site == calls_[call].site) returned = true;
      if (returned) {
        path.push_back(fname(fn) + " returns the result of " + callee + " (" + call_pos(call) + ")");
        for (uint32_t up : callers_[fn]) work.push_back({calls_[up].caller_fn, up});
      }
    }
    if (path.empty()) os << "      handling: result " << result_use(ci) << "; no return of it and no terminal operation under it found\n";
    for (const std::string& p : path) os << "      handling: " << p << "\n";
    return os.str();
  }

  // ---------------------------------------------------------------- state
  const kz::Container& c_;
  std::vector<kz_basis_item> basis_;
  std::vector<kz_fact_func> funcs_;
  std::vector<kz_fact_call> calls_;
  std::vector<kz_fact_expr> exprs_;
  std::vector<kz_fact_loop> loops_;
  std::vector<kz_fact_init> inits_;
  std::vector<kz_fact_writer> writers_;
  std::vector<kz_fact_target> targets_;
  std::vector<kz_fact_terminal> terminals_;
  std::vector<kz_fact_iter> iters_;
  std::vector<kz_fact_fwrite> fwrites_;
  std::vector<kz_fact_p0> p0_;
  std::vector<kz_fact_opderiv> opderiv_;
  std::vector<kz_fact_spawnw> spawnw_;
  std::vector<kz_fact_read> reads_;
  std::vector<kz_fact_return> returns_;
  std::vector<kz_fact_retconst> retconsts_;
  std::vector<kz_fact_loopexit> loopexits_;
  std::vector<kz_fact_argflow> argflow_;
  std::vector<kz_fact_pflow> pflow_;
  std::vector<kz_fact_ptrfield> ptrfields_;
  std::vector<kz_fact_gimage> gimages_;
  std::vector<kz_fact_lockid> lockids_;
  std::vector<kz_fact_lockset> lockset_;
  std::vector<kz_fact_fstatus> fstatus_;
  std::vector<kz_fact_await> awaits_;
  std::vector<kz_fact_lockpath> lockpaths_;

  struct BasisMap : std::map<std::string, std::string> {
    std::string at_or(const std::string& k) const {
      auto it = find(k);
      return it == end() ? std::string("?") : it->second;
    }
  } basis_map_;
  bool profile_free_ = false;
  uint32_t hand_entered_vocabulary_ = 0;
  std::map<std::string, std::vector<uint32_t>> fn_by_name_;
  std::vector<std::vector<std::pair<uint32_t, bool>>> succ_;
  std::vector<std::vector<uint32_t>> callers_;  // direct calls into a function
  std::map<uint32_t, std::vector<uint32_t>> indirect_callers_;
  std::map<uint32_t, uint32_t> call_by_site_;
  std::map<uint32_t, std::vector<uint32_t>> calls_of_;
  std::map<uint32_t, std::set<uint32_t>> spawn_entries_;  // spawn site -> entries (kN: unresolved)
  std::map<uint32_t, std::map<uint32_t, uint32_t>> iter_spawn_;  // spawn site -> entry -> iteration
  std::map<uint32_t, uint32_t> iter_loop_;
  std::set<uint32_t> script_entries_;
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> argflow_at_;
  std::map<std::string, std::vector<uint32_t>> reads_of_field_;
  std::map<uint32_t, std::vector<uint32_t>> returns_of_, retconsts_of_, loopexits_of_, lockset_writes_of_, terminals_of_, opderiv_of_, awaits_of_;
  std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> pflow_of_;
  std::map<uint32_t, uint32_t> lockset_call_, lockset_read_, ptrread_of_, loop_by_index_;
  std::map<std::string, uint32_t> fstatus_of_;
  std::map<std::string, std::vector<uint32_t>> writers_of_, inits_of_, fwrites_of_, gimages_of_;
  std::map<std::string, uint8_t> p0_kind_;
  std::set<std::string> compare_names_;
  std::set<std::string> gaf_;
  std::map<uint32_t, std::string> root_kind_;
  std::map<uint32_t, std::set<uint32_t>> reach_, delegates_, actors_of_;
  std::set<uint32_t> trampolines_, kept_direct_, kept_shared_;
  std::map<uint32_t, std::set<uint32_t>> after_spawn_;  // spawn site -> LOCKSET rows after it (LOCK_PATHS)
  std::map<uint32_t, uint32_t> entry_release_;           // LOCKSET row -> the call site (LOCK_PATHS entry-release)
  std::map<uint32_t, std::set<uint32_t>> itargets_;      // indirect call site -> resolved targets
  std::map<uint32_t, std::set<uint32_t>> image_spawn_;   // spawn site -> entries from a static image
  std::map<uint32_t, std::string> image_member_;          // spawn site -> the member the entry argument reads (Type.field)
  std::map<uint32_t, std::map<uint32_t, std::string>> image_paths_;  // spawn site -> entry -> its image paths
  std::map<uint32_t, std::map<uint32_t, std::set<std::string>>> via_op_;  // trampoline -> root -> the derived spawn operations
  std::map<uint32_t, std::set<std::string>> via_op_missing_;  // trampoline -> calls of such an operation with no resolved entry
  std::set<uint32_t> unresolved_sites_;                  // indirect calls with a target the flow graph does not resolve
  std::set<std::string> p0_unverified_, lock_prims_;
  std::map<uint32_t, std::set<std::string>> script_prim_;  // script entry -> the P0 symbol lookup it is resolved through
  std::map<uint32_t, std::set<uint32_t>> closure_;
  std::vector<std::string> deviations_;
  std::map<std::string, std::set<std::string>> deviation_files_;
  std::set<std::string> assumptions_;
  std::vector<std::string> keys_dtor_, keys_null_dtor_, cleanup_pushes_, unresolved_spawns_;
  mutable std::set<std::string> row_files_, row_tags_, row_p0_;
  std::map<std::string, std::set<int64_t>> in_use_, empty_;
  uint32_t n_a_ = 0, n_ns_ = 0, n_a_paired_ = 0, n_a_keymatch_ = 0, n_b_ = 0, n_b_check_ = 0, n_c_ = 0, n_c_self_ = 0;
  uint32_t n_flip_possible_ = 0, n_flip_undet_ = 0, n_flip_none_ = 0, n_feas_impossible_ = 0, n_feas_possible_ = 0, n_feas_undecidable_ = 0, n_feas_not_graded_ = 0;
};

}  // namespace

kz::Status relations_report(const kz::Container& facts, const RelationsOptions& options, std::string* out) {
  (void)options;
  if (facts.header().artifact_kind != KZ_ARTIFACT_FACTS) return kz::Status::error(kz::Err::Invalid, "not a facts container");
  Rel r(facts);
  return r.run(out);
}

}  // namespace kzs

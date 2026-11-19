// relations.cpp -- see relations.hpp.
//
// Every rule below reads the facts container only.  Nothing names a framework, a case or a resource
// type: the program's own facts and the P0 tables are the vocabulary.  The rules are written down in
// docs/static_facts.md §5e (revision 1), §5f (revision 2), §5g (revision 3), §5h (revision 4), §5i (revision 5) and §5m
// (revision 6: the image cut at the budget, the incompleteness line); the revision string names them.
#include "outcome.hpp"
#include "relations.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
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

namespace kzs {

const char* relations_rules_revision() { return "relations rules revision 12 (2026-11-18)"; }
// Revision 10 adds to revision 9 only what facts with RESULT_FLOW and RESULT_EFFECT carry (static_facts.md 5u): on path A
// rows the lines absence and handling_label, on the creators the lines callee-check (result flow), on path B rows the line
// handling_label.  Without those sections the rows are revision 9's to the byte, and the line `rules` names revision 9.
const char* relations_result_flow_revision() { return "relations rules revision 13 (2026-11-18)"; }
// Revision 11 adds to revision 10 only what facts with LOCK_HANDLES carry (static_facts.md 5v): on path C, a lock whose
// identity is no static instance is one mutex at the spawn and at the read when every acquisition that holds it there passes
// the same handle and that handle is a static instance with a verified semantics item (LOCK_HANDLES static-instance);
// a common handle of another status is named in the row's text.  Without the section the rows are revision 10's.
const char* relations_lock_handle_revision() { return "relations rules revision 11 (2026-11-16)"; }

namespace {

constexpr uint32_t kN = KZ_NONE32;

// The incompleteness reasons of a row (relations rules revision 6, docs/static_facts.md §5m): a reason of the vocabulary
// of the judgement rules (plan/ovm_rev1_20261116/JUDGEMENT_RULES_REV1.md 3.5) and a sub-reason.  The enumerators index
// the table relations_incomplete_reasons(), in its order, which is the order of the line and of the summary.
enum Inc : uint32_t {
  kNoRun,
  kActorUnresolvedSpawn,
  kActorCutSpawn,
  kActorNoRoot,
  kActorChildSet,
  kKeyNoArgflow,
  kKeyPtrField,
  kKeyImageCut,
  kKeyImageWritten,
  kKeyImageNotValue,
  kKeyNoImageLeaf,
  kKeyFieldNoImage,
  kKeyOther,
  kKeyCreator,
  kTruncImage,
  kTruncInit,
  kTruncFpInit,
  kTruncExpr,
  kTruncDerives,
  kTruncLoopText,
  kWriterNotComplete,
  kWriterNoStatus,
  kWriterPremise,
  kWriterZero,
  kWriterUnread,
  kAbsentSearch,
  kAbsentCallee,
  kAbsentReader,
  kFixedNoOrder,
  kFixedCreationOnly,
  kFixedImageOrder,
  kFixedLocks,
  kFixedUnresolved,
  kFixedChildSet,
  kFixedAssumption,
  kLinkIdentity,
  kLinkSelf,
  kLinkMembership,
  kReadingDeviation,
  kReadingOptions,
  kIncCount
};

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

// The lines of the row being written: line(name) starts a line (the stream takes its text, without the indentation
// and the newline) and closes the one before; done() closes the last one.
struct RowLines {
  explicit RowLines(RelRow* r) : row(r) {}
  RowLines(const RowLines&) = delete;
  RowLines& operator=(const RowLines&) = delete;
  ~RowLines() { done(); }
  std::ostream& line(const char* name, uint8_t indent = 6) {
    done();
    open = true;
    cur_name = name;
    cur_indent = indent;
    cur.str("");
    cur.clear();
    return cur;
  }
  void done() {
    if (!open) return;
    row->lines.push_back(RelLine{cur_name, cur.str(), cur_indent});
    open = false;
  }
  RelRow* row;
  std::ostringstream cur;
  std::string cur_name;
  uint8_t cur_indent = 6;
  bool open = false;
};

class Rel : public RelFacts {
 public:
  explicit Rel(const kz::Container& c) : RelFacts(c) { file_notes_ = &row_files_; }

  kz::Status run(RelReport* out) {
    if (relations_incomplete_reasons().size() != kIncCount) return kz::Status::error(kz::Err::Invalid, "the incompleteness reason table does not match its index");
    kz::Status s = prepare();
    if (!s.ok()) return s;
    // semantics with a verification state (static_facts.md 5t): with a SEMANTICS section, the rows note the functions they
    // name, and their layers column names the framework items of those functions that are not verified
    if (!sem_.empty()) {
      fn_notes_ = &row_fns_;
      for (const kz_fact_sem& r : sem_) {
        if (r.fn == kN || r.layer != KZ_SEM_LAYER_FRAMEWORK) continue;
        sem_items_[r.fn] += 1;
        if (r.verify == KZ_SEM_VERIFY_UNVERIFIED) sem_unverified_[r.fn].insert(str(r.api) + ":" + enum_text("KZ_SEM_COLUMN", r.column));
      }
    }
    global_reasons();
    RelReport rep;
    path_a(&rep);
    path_b(&rep);
    path_c(&rep);
    std::ostringstream os;
    header(os);
    rep.header = os.str();
    rep.facts = kz::to_hex(c_.artifact_hash());
    *out = std::move(rep);
    return kz::Status::success();
  }

 private:
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

  // The threads a spawn call starts: the actors of its resolved entries (a trampoline stands for its roots), and, in
  // turn, the threads the code of those starts at its own spawn calls (descendants), with the code they all run (the
  // call graph from their roots, direct calls and resolved indirect targets, not stopping at another root: a root the
  // code calls runs in the calling thread).  The inner spawn call of a derived spawn operation that passes its own
  // parameter (SPAWN_WRAP entry-argument, called-by-entry) is read at the operation's calls, which are spawn calls of
  // their own.  The set is not complete when a trampoline among them stands for threads started at calls with no
  // resolved entry, or the code makes a spawn call with no resolved entry.
  struct ChildSet {
    std::set<uint32_t> roots, descendants, code;
    std::vector<std::string> incomplete;  // why the set is not known to be complete
    bool cut = false;                     // entries of the set come from a static image cut at the image budget (revision 6)
    std::vector<uint32_t> unres;          // the indirect calls of that code whose targets are not (all) resolved (revision 5)
  };
  const ChildSet& child_set(uint32_t site, const std::set<uint32_t>& resolved) {
    auto memo = child_sets_.find(site);
    if (memo != child_sets_.end()) return memo->second;
    ChildSet cs;
    std::set<uint32_t> inner;  // the inner spawn calls of derived spawn operations
    for (const kz_fact_spawnw& w : spawnw_)
      if (w.kind == KZ_SPAWNW_KIND_ENTRY_ARG || w.kind == KZ_SPAWNW_KIND_CALLED_BY_ENTRY) inner.insert(w.site);
    auto missing = [&](const std::set<uint32_t>& es) {
      for (uint32_t e : es) {
        auto vm = via_op_missing_.find(e);
        if (trampolines_.count(e) != 0 && vm != via_op_missing_.end() && !vm->second.empty())
          for (const std::string& m : vm->second) {
            const std::string t = "the trampoline " + fname(e) + " also runs the threads started at " + m + ", whose entry is not resolved";
            if (std::find(cs.incomplete.begin(), cs.incomplete.end(), t) == cs.incomplete.end()) cs.incomplete.push_back(t);
          }
      }
    };
    cs.roots = entry_actors(resolved);
    missing(resolved);
    // entries read from a static image under a premise of the spawn's guard (image_premise_)
    auto image_premise = [&](uint32_t spawn_site, const std::string& where) {
      auto ip = image_premise_.find(spawn_site);
      if (ip != image_premise_.end()) {
        const std::string t = "the entries of the spawn " + where + " are read from a static image: " + ip->second;
        if (std::find(cs.incomplete.begin(), cs.incomplete.end(), t) == cs.incomplete.end()) cs.incomplete.push_back(t);
      }
      // relations rules revision 6: entries from a cut image are those of the part read only
      auto ic = image_cut_.find(spawn_site);
      if (ic != image_cut_.end()) {
        cs.cut = true;
        const std::string t = "the entries of the spawn " + where + " are read from a static image: " + ic->second;
        if (std::find(cs.incomplete.begin(), cs.incomplete.end(), t) == cs.incomplete.end()) cs.incomplete.push_back(t);
      }
    };
    {
      auto sc = call_by_site_.find(site);
      image_premise(site, sc != call_by_site_.end() ? call_pos(sc->second) : "site " + std::to_string(site));
    }
    std::set<uint32_t> seen_sites{site};
    std::vector<uint32_t> stack;
    auto enter = [&](uint32_t f) {
      if (defined(f) && cs.code.insert(f).second) stack.push_back(f);
    };
    for (uint32_t e : resolved) enter(e);  // a trampoline's own code runs in the thread too
    for (uint32_t a : cs.roots) enter(a);
    while (!stack.empty()) {
      const uint32_t x = stack.back();
      stack.pop_back();
      for (const auto& [y, indirect] : succ_[x]) enter(y);
      for (uint32_t ci : calls_of_.count(x) != 0 ? calls_of_.at(x) : std::vector<uint32_t>{})
        for (uint32_t y : callback_args(ci)) enter(y);  // a function passed to a callee without a body may be called back
      auto cit = calls_of_.find(x);
      if (cit == calls_of_.end()) continue;
      for (uint32_t ci : cit->second)
        if (unresolved_sites_.count(calls_[ci].site) != 0) cs.unres.push_back(ci);
      for (uint32_t ci : cit->second) {
        const kz_fact_call& c = calls_[ci];
        if ((c.flags & KZ_CALL_FLAG_SPAWN_OP) == 0 || inner.count(c.site) != 0 || !seen_sites.insert(c.site).second) continue;
        std::set<uint32_t> es;
        auto se = spawn_entries_.find(c.site);
        if (se != spawn_entries_.end())
          for (uint32_t e : se->second)
            if (defined(e)) es.insert(e);
        if (es.empty()) {
          cs.incomplete.push_back("the spawn call " + call_pos(ci) + " in " + fname(c.caller_fn) + " that this code reaches has no resolved entry");
          continue;
        }
        missing(es);
        image_premise(c.site, call_pos(ci));
        for (uint32_t e : es) enter(e);
        for (uint32_t a : entry_actors(es)) {
          if (cs.roots.count(a) == 0) cs.descendants.insert(a);
          enter(a);
        }
      }
    }
    std::sort(cs.unres.begin(), cs.unres.end());
    cs.unres.erase(std::unique(cs.unres.begin(), cs.unres.end()), cs.unres.end());
    return child_sets_[site] = cs;
  }
  // the functions an address-taken function may run (call_runs, transitively): an indirect call whose targets are not
  // resolved may call an address-taken function, and so run any of these
  bool at_reach(uint32_t fn) {
    if (!at_reach_done_) {
      at_reach_done_ = true;
      std::vector<uint32_t> stack;
      for (size_t f = 0; f < funcs_.size(); ++f)
        if (defined(static_cast<uint32_t>(f)) && (funcs_[f].flags & KZ_FUNC_FLAG_ADDRESS_TAKEN) != 0 && at_reach_.insert(static_cast<uint32_t>(f)).second)
          stack.push_back(static_cast<uint32_t>(f));
      while (!stack.empty()) {
        const uint32_t x = stack.back();
        stack.pop_back();
        auto cit = calls_of_.find(x);
        if (cit == calls_of_.end()) continue;
        for (uint32_t ci : cit->second)
          for (uint32_t y : call_runs(ci))
            if (at_reach_.insert(y).second) stack.push_back(y);
      }
    }
    return at_reach_.count(fn) != 0;
  }
  std::string unres_text(const std::vector<uint32_t>& cis) const {
    std::vector<std::string> v;
    for (uint32_t ci : cis) {
      if (v.size() == 6) break;
      auto w = unresolved_why_.find(calls_[ci].site);
      v.push_back(call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + (w != unresolved_why_.end() ? " (" + w->second + ")" : ""));
    }
    return join(v, "; ") + (cis.size() > v.size() ? "; ... (" + std::to_string(cis.size()) + " calls)" : "");
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
    row_inc_.clear();
    row_fns_.clear();
  }
  // the framework semantics items not verified of the functions the row names ("api:column"), for facts with SEMANTICS
  std::set<std::string> row_sem_unverified() const {
    std::set<std::string> out;
    for (uint32_t f : row_fns_) {
      auto it = sem_unverified_.find(f);
      if (it != sem_unverified_.end()) out.insert(it->second.begin(), it->second.end());
    }
    return out;
  }
  bool row_has_sem() const {
    for (uint32_t f : row_fns_)
      if (sem_items_.count(f) != 0) return true;
    return false;
  }
  // ---------------------------------------------------------------- incompleteness (relations rules revision 6, §5m)
  // A reason of the row being written (Inc, relations_incomplete_reasons()).
  void inc(Inc i) const { row_inc_.insert(i); }
  // The reasons of the whole facts, which every row carries: spawn calls with no resolved entry (and the calls of a
  // derived spawn operation a trampoline stands for with no resolved entry), spawn entries from a cut static image, and
  // the walk bounds BASIS count.walk_bounds_hit counts as hit (the two counts of the image budget excluded: a cut image is
  // read per object, image_cut).
  void global_reasons() {
    bool missing = !unresolved_spawns_.empty();
    for (const auto& [t, ms] : via_op_missing_)
      if (!ms.empty()) missing = true;
    if (missing) global_inc_.insert(kActorUnresolvedSpawn);
    if (!image_cut_.empty()) global_inc_.insert(kActorCutSpawn);
    if (reading_options_differ()) global_inc_.insert(kReadingOptions);
    auto it = basis_map_.find("count.walk_bounds_hit");
    if (it == basis_map_.end()) return;
    const std::string& v = it->second;
    const std::pair<const char*, Inc> bounds[] = {{"FACT_INIT walk ", kTruncInit},
                                                  {"function-value initializer nesting ", kTruncFpInit},
                                                  {"expression depth ", kTruncExpr},
                                                  {"DERIVES copy chains ", kTruncDerives},
                                                  {"LOOP_ITERS texts ", kTruncLoopText}};
    for (const auto& [label, reason] : bounds) {
      const size_t at = v.find(label);
      if (at == std::string::npos) continue;
      size_t i = at + std::strlen(label);
      uint64_t n = 0;
      bool digits = false;
      while (i < v.size() && std::isdigit(static_cast<unsigned char>(v[i]))) {
        n = n * 10 + static_cast<uint64_t>(v[i] - '0');
        digits = true;
        ++i;
      }
      if (digits && n != 0) global_inc_.insert(reason);
    }
  }
  // the writer-set reason of a field: FIELD_STATUS has no row, or its row is not complete under both rules
  void inc_writers(const std::string& tf) const {
    auto fsit = fstatus_of_.find(tf);
    if (fsit == fstatus_of_.end()) {
      inc(kWriterNoStatus);
      return;
    }
    const kz_fact_fstatus& f = fstatus_[fsit->second];
    if (f.status_field != KZ_LOC_STATUS_COMPLETE || f.status_type != KZ_LOC_STATUS_COMPLETE) inc(kWriterNotComplete);
  }
  // The row's reasons, after its layers are written (they read the row's files and tags): the reasons of the whole
  // facts, no run record, a basis deviation of a unit the row names, the per-field completeness rule and, for a
  // proven-impossible grade, the declared assumptions and the premises of path C the grade rests on.
  void finish(RelRow* r) {
    inc(kNoRun);
    for (uint32_t g : global_inc_) row_inc_.insert(g);
    if (!row_deviations().empty()) inc(kReadingDeviation);
    for (const std::string& t : row_tags_) {
      if (t.compare(0, 28, "per-field completeness rule ") == 0) inc(kWriterPremise);
      if (r->feasibility_grade == "proven-impossible" && (t.compare(0, 7, "assume.") == 0 || t.compare(0, 19, "premises of path C ") == 0)) inc(kFixedAssumption);
    }
    if (!sem_.empty() && row_has_sem()) {
      ++n_sem_rows_;
      if (!row_sem_unverified().empty()) ++n_sem_unverified_rows_;
    }
    r->incomplete.assign(row_inc_.begin(), row_inc_.end());
    std::set<std::string> reasons;
    for (uint32_t i : r->incomplete) {
      ++inc_rows_[i];
      reasons.insert(relations_incomplete_reasons()[i].reason);
    }
    for (const std::string& x : reasons) ++inc_reason_rows_[x];
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
  // The extraction read the units with other reading options or another front end than its basis records, and was
  // told to go on (extract --accept-option-difference): BASIS extract.reading_options_vs_basis starts with "differs"
  // (static_facts.md 5q).  Facts without the item (an extraction before it) are not taken as differing.
  bool reading_options_differ() const {
    auto it = basis_map_.find("extract.reading_options_vs_basis");
    return it != basis_map_.end() && it->second.compare(0, 7, "differs") == 0;
  }
  std::string layers_text(bool p0, bool l0, const std::string& extra = "") const {
    std::string t = "B";
    if (p0) t += " P0";
    if (l0) t += " L0";
    t += " R";
    if (!profile_free_) t += " case:profile(" + basis_map_.at_or("extract.profile") + ")";
    if (hand_entered_vocabulary_ != 0) t += " hand-entered:command-line(" + std::to_string(hand_entered_vocabulary_) + " items; G0 or other names, not automatic)";
    if (!extra.empty()) t += " " + extra;
    if (reading_options_differ()) t += " reading-options-differ(extract.reading_options_vs_basis)";
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
    if (!sem_.empty()) {
      const std::set<std::string> su = row_sem_unverified();
      if (!su.empty()) t += " sem-unverified(" + join(std::vector<std::string>(su.begin(), su.end()), ", ") + ")";
    }
    return t;
  }

  // ---------------------------------------------------------------- header
  void header(std::ostream& os) const {
    os << "kz-relations 1\n";
    os << "facts " << kz::to_hex(c_.artifact_hash()) << "\n";
    os << "rules " << relations_rules_revision() << "\n";
    os << "mode " << (profile_free_ ? "profile-free" : "profile-driven") << "\n";
    for (const char* k : {"extract.builtin_profile", "extract.profile", "extract.p0_table", "extract.p0_table_scope", "extract.derivation_rules", "extract.targets_rule",
                          "extract.reading_options_vs_basis"}) {
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
    if (!sem_.empty())
      os << "semantics of these facts (SEMANTICS, extract --semantics, docs/static_facts.md 5t): " << basis_map_.at_or("count.semantics")
         << "; rows that name a function with framework semantics items " << n_sem_rows_ << ", of them with items not verified " << n_sem_unverified_rows_
         << " (sem-unverified(api:column) in the layers column; the P0 entries stay in P0-unverified)\n";
    if (lock_handles_)
      os << "rules with lock handles: " << relations_lock_handle_revision() << ", over LOCK_HANDLES (extract --lock-handles, docs/static_facts.md 5v): "
         << basis_map_.at_or("count.lock_handles") << "; path C rows whose common lock is identified by its handle " << n_handle_rows_ << ", of them with a static-instance handle "
         << n_handle_static_ << "\n";
    if (result_flow_) {
      os << "rules with result flow: " << relations_result_flow_revision() << ", over RESULT_FLOW and RESULT_EFFECT (extract --result-flow, docs/static_facts.md 5u): "
         << basis_map_.at_or("count.result_flow") << "; " << basis_map_.at_or("count.result_effect") << "\n";
      os << "handling labels (judgement rules 3.4: processing facts, never a verdict) of the path A and B rows:";
      for (const auto& [k, n] : n_lab_) os << " " << k << " " << n;
      os << "\n";
    }
    os << "run order: no record given (this revision reads no run record; the column says so in every row)\n";
    os << "functions that return twice (C library; path C gives no proven-impossible over the lock sets of a function that calls one): ";
    for (size_t i = 0; i < sizeof kReturnsTwice / sizeof kReturnsTwice[0]; ++i) os << (i == 0 ? "" : ", ") << kReturnsTwice[i];
    size_t n_rt = 0;
    for (const auto& [f, cs] : returns_twice_) n_rt += cs.size();
    os << "; calls of them in these facts: " << n_rt << "\n";
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
      os << "; the member " << image_member_.at(site) << " has a complete writer set with no writer instance under both rules of FIELD_STATUS and no leaf marked written";
      auto ip = image_premise_.find(site);
      if (ip != image_premise_.end()) os << "; " << ip->second << " (so the threads of this spawn are taken as not complete)";
      auto ic = image_cut_.find(site);
      if (ic != image_cut_.end()) os << "; " << ic->second << " (so the threads of this spawn are taken as not complete)";
      os << "]";
    }
    os << "\n";
    os << "spawn calls with no resolved entry (their threads are no actors here; the code only they reach has the actors of its other callers): " << unresolved_spawns_.size();
    for (const std::string& u : unresolved_spawns_) os << " [" << u << "]";
    os << "\n";
    os << "summary A " << n_a_ << " (namespaces " << n_ns_ << ", lookup uses " << n_a_ << ", with a creator of the same partition in another actor " << n_a_paired_
       << ", key-matched " << n_a_keymatch_ << ") B " << n_b_ << " (with a callee check " << n_b_check_ << ") C " << n_c_ << " (self-identity reads " << n_c_self_
       << "); flip static-possible " << n_flip_possible_ << ", static-undetermined " << n_flip_undet_ << ", none " << n_flip_none_ << "; feasibility proven-impossible "
       << n_feas_impossible_ << ", possible " << n_feas_possible_ << ", undecidable " << n_feas_undecidable_ << ", not graded " << n_feas_not_graded_ << "\n";
    // relations rules revision 7: the uses of path A left out for another callback, the rows whose membership is not
    // resolved, and the lookup rows without a creator of their partition per sub-reason (a row may have several)
    os << "summary A membership and creators (relations rules revision 7): uses left out, another callback passed to the search: lookups " << n_a_other_lookups_ << ", creators "
       << n_a_other_creators_ << "; lookup rows with the membership unresolved " << n_a_member_unres_ << "; lookup rows without a creator of their partition " << n_a_nocreator_ << " [";
    {
      bool first = true;
      for (const auto& [w, k] : no_creator_why_) {
        os << (first ? "" : ", ") << w << " " << k;
        first = false;
      }
    }
    os << "]";
    if (n_a_clearing_ != 0) os << "; key-clearing calls, no creator sites (relations rules revision 12): " << n_a_clearing_;
    os << "\n";
    // relations rules revision 6: the rows per incompleteness reason (the reason, then its sub-reasons)
    os << "summary incomplete (rows that carry the reason, then per sub-reason; reasons of the judgement rules' vocabulary, docs/static_facts.md §5m):";
    const std::vector<RelReason>& table = relations_incomplete_reasons();
    std::string last;
    bool any = false;
    for (size_t i = 0; i < table.size(); ++i) {
      auto n = inc_rows_.find(static_cast<uint32_t>(i));
      if (n == inc_rows_.end()) continue;
      if (last != table[i].reason) {
        os << (any ? "]; " : " ") << table[i].reason << " " << inc_reason_rows_.at(table[i].reason) << " [";
        last = table[i].reason;
      } else {
        os << ", ";
      }
      os << table[i].sub << " " << n->second;
      any = true;
    }
    os << (any ? "]" : " no rows");
    os << "; reasons of the whole facts on every row:";
    if (global_inc_.empty()) os << " none";
    for (uint32_t g : global_inc_) os << " " << reason_text(table[g]);
    os << "\n";
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
    last_feas_grade_ = grade;
    if (grade == "proven-impossible") ++n_feas_impossible_;
    else if (grade == "possible") ++n_feas_possible_;
    else if (grade == "not-graded") ++n_feas_not_graded_;
    else ++n_feas_undecidable_;
  }
  static std::string violation_text() { return "unconfirmed: RV2 §5.4 needs an observed failure with a feasible other order, or a realized forced order; no record given"; }

  // the lines of a text (each with its indentation and a newline) as lines of the row, all named `name`
  static void add_lines(RelRow* r, const std::string& text, const char* name) {
    size_t at = 0;
    while (at < text.size()) {
      size_t end = text.find('\n', at);
      if (end == std::string::npos) end = text.size();
      const std::string l = text.substr(at, end - at);
      size_t ind = 0;
      while (ind < l.size() && l[ind] == ' ') ++ind;
      r->lines.push_back(RelLine{name, l.substr(ind), static_cast<uint8_t>(ind)});
      at = end + 1;
    }
  }

  // ---------------------------------------------------------------- use-site keys (docs/static_facts.md §5l)
  // A key names where the row's use is in the program, not what the rules found there: no line or column, no
  // creator, writer, partition, counter or verdict.  It reads names only and notes no file (it is no part of the text).
  // A function: its name, and for a function of internal linkage the file it is defined in.
  // The file of a function outside the root is written by its source tree (BASIS `source_tree[i]`, option
  // --object-access, static_facts.md 5x): "<tree>/<path in the tree>"; facts without those items keep the recorded path.
  std::string key_fn(uint32_t fn) const {
    if (fn == kN || fn >= funcs_.size()) return "-";
    std::string n = str(funcs_[fn].name);
    if (funcs_[fn].flags & KZ_FUNC_FLAG_STATIC) n += "@" + key_file(str(funcs_[fn].file));
    return n;
  }
  std::string key_file(const std::string& f) const {
    if (f.empty() || f[0] != '/') return f;
    if (!trees_read_) {
      trees_read_ = true;
      for (const auto& [k, v] : basis_map_) {
        if (k.rfind("source_tree[", 0) != 0) continue;
        const size_t sp = v.rfind(' ');
        if (sp != std::string::npos) trees_.emplace_back(v.substr(0, sp), v.substr(sp + 1));
      }
      // the longest directory first: a tree inside another tree is its own tree
      std::sort(trees_.begin(), trees_.end(), [](const auto& a, const auto& b) { return a.first.size() != b.first.size() ? a.first.size() > b.first.size() : a.first < b.first; });
    }
    for (const auto& [d, t] : trees_)
      if (f.size() > d.size() && f.compare(0, d.size(), d) == 0 && f[d.size()] == '/') return t + f.substr(d.size());
    return f;
  }
  mutable bool trees_read_ = false;
  mutable std::vector<std::pair<std::string, std::string>> trees_;
  // the callee of a call: the function it calls, or the name the call gives (a function without a body, an indirect call)
  std::string key_callee(uint32_t ci) const {
    const kz_fact_call& c = calls_[ci];
    return defined(c.callee_fn) ? key_fn(c.callee_fn) : str(c.callee);
  }
  // the n-th call (from 1) of the same callee among the calls of the caller, in source order (file, line, column; the
  // site number orders two calls at one position)
  uint32_t call_ordinal(uint32_t ci) const {
    const kz_fact_call& c = calls_[ci];
    auto it = calls_of_.find(c.caller_fn);
    if (it == calls_of_.end()) return 0;
    const std::string callee = key_callee(ci);
    std::vector<std::tuple<std::string, uint32_t, uint32_t, uint32_t>> same;
    for (uint32_t x : it->second)
      if (key_callee(x) == callee) same.emplace_back(str(calls_[x].file), calls_[x].line, calls_[x].col, calls_[x].site);
    std::sort(same.begin(), same.end());
    const auto me = std::make_tuple(str(c.file), c.line, c.col, c.site);
    return static_cast<uint32_t>(std::lower_bound(same.begin(), same.end(), me) - same.begin()) + 1;
  }
  // the n-th read (from 1) of the same field among the reads of the reading function, in source order (the READS row
  // orders two reads at one position)
  uint32_t read_ordinal(uint32_t ri) const {
    const kz_fact_read& u = reads_[ri];
    auto it = reads_of_field_.find(str(u.type) + "." + str(u.field));
    if (it == reads_of_field_.end()) return 0;
    std::vector<std::tuple<std::string, uint32_t, uint32_t, uint32_t>> same;
    for (uint32_t x : it->second)
      if (reads_[x].fn == u.fn) same.emplace_back(str(reads_[x].file), reads_[x].line, reads_[x].col, x);
    std::sort(same.begin(), same.end());
    const auto me = std::make_tuple(str(u.file), u.line, u.col, ri);
    return static_cast<uint32_t>(std::lower_bound(same.begin(), same.end(), me) - same.begin()) + 1;
  }
  static void set_key(RelRow* r, const std::string& text) {
    r->key_text = text;
    r->key = kz::to_hex(kz::sha256(text)).substr(0, 16);
  }
  // path A: the namespace, the function of the lookup call, the key-parameter function and parameter, the n-th call
  void key_a(RelRow* r) const {
    set_key(r, "A|" + r->subject + "|" + key_fn(r->caller_fn) + "|" + key_fn(r->callee_fn) + "#" + std::to_string(r->param) + "|" + std::to_string(call_ordinal(r->call)));
  }
  // path B: a row is one field passed as one argument of one callee (its sites are row content); each site has a key
  // of its own in the listing (site_key)
  void key_b(RelRow* r) const { set_key(r, "B|" + r->subject + "|-|" + key_fn(r->callee_fn) + "#" + std::to_string(r->param) + "|-"); }
  std::string site_key_text(const RelRow& r, uint32_t ci) const {
    return "B-site|" + r.subject + "|" + key_fn(calls_[ci].caller_fn) + "|" + key_fn(r.callee_fn) + "#" + std::to_string(r.param) + "|" + std::to_string(call_ordinal(ci));
  }
  // path C: the field, the creator, the spawn call's callee and its n-th call in the creator, the reading function and
  // the n-th read of the field in it, and the context (itself, entry, unresolved; or a call of the reading function:
  // its kind, the calling function, the callee as the call names it and the n-th such call)
  void key_c(RelRow* r) const {
    std::string ctx = r->context_kind;
    if (r->context_call != kN)
      ctx += " " + key_fn(calls_[r->context_call].caller_fn) + " " + key_callee(r->context_call) + " " + std::to_string(call_ordinal(r->context_call));
    set_key(r, "C|" + r->subject + "|" + key_fn(r->caller_fn) + "|" + key_callee(r->call) + "|" + std::to_string(call_ordinal(r->call)) + "|read " + key_fn(r->read_fn) + " " +
                   std::to_string(read_ordinal(r->read)) + "|" + ctx);
  }
  // the keys of the sites of a path B row, for the listing
  void site_keys(RelRow* r) const {
    for (uint32_t ci : r->site_calls) {
      const std::string t = site_key_text(*r, ci);
      r->site_keys.push_back(kz::to_hex(kz::sha256(t)).substr(0, 16) + " " + t);
    }
  }

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
  // the names of the bits of a set enumeration, ", "-separated (a bit the registry does not name as "#0x..")
  std::string flags_text(const char* name, uint64_t v) const {
    const kz::EnumDesc* d = kz::find_enum(name);
    std::string t;
    uint64_t left = v;
    if (d != nullptr)
      for (size_t i = 0; i < d->nitems; ++i)
        if (d->items[i].value != 0 && (v & d->items[i].value) == d->items[i].value) {
          t += (t.empty() ? "" : ", ") + std::string(d->items[i].display);
          left &= ~d->items[i].value;
        }
    if (left != 0) {
      char buf[32];
      std::snprintf(buf, sizeof buf, "#0x%llx", static_cast<unsigned long long>(left));
      t += (t.empty() ? "" : ", ") + std::string(buf);
    }
    return t;
  }
  // relations rules revision 7 (O4): the writer set of a field the row uses, with the statuses of FIELD_STATUS under both
  // completeness rules, the premises the set rests on and the channels it does not cover.  Zero writers is no verdict:
  // the text says "no writer found under these premises" and `zero` is set (the row carries
  // writer_set_incomplete(zero writers under the rules)).
  std::string writer_premises(const std::string& tf, bool* zero) const {
    std::string src;
    const std::vector<WriterRow> ws = writers_of(tf, &src);
    std::map<std::string, uint32_t> kinds;
    uint32_t covers = 0, early = 0, tree = 0, partial = 0;
    for (const WriterRow& w : ws) {
      ++kinds[w.kind];
      if (w.flags & KZ_WRITER_FLAG_COVERS_MORE) ++covers;
      if (w.flags & KZ_WRITER_FLAG_EARLY_INIT) ++early;
      if (w.flags & KZ_WRITER_FLAG_INIT_TREE) ++tree;
      if (w.flags & KZ_WRITER_FLAG_PARTIAL) ++partial;
    }
    std::ostringstream os;
    os << "writers: " << tf << ": ";
    *zero = ws.empty();
    if (ws.empty()) {
      os << "no writer found under these premises";
    } else {
      os << ws.size() << " (";
      bool first = true;
      for (const auto& [k, nk] : kinds) {
        os << (first ? "" : ", ") << k << " " << nk;
        first = false;
      }
      os << ") from " << src;
      if (covers + early + tree + partial != 0)
        os << "; of them covers-more-than-the-field " << covers << ", early-init " << early << ", init-tree " << tree << ", partial-write " << partial;
    }
    auto fsit = fstatus_of_.find(tf);
    if (fsit == fstatus_of_.end()) {
      os << "; premises: no FIELD_STATUS row (the completeness of the set is not graded)";
    } else {
      const kz_fact_fstatus& f = fstatus_[fsit->second];
      os << "; premises: FIELD_STATUS per-field " << enum_text("KZ_LOC_STATUS", f.status_field) << ", type-level " << enum_text("KZ_LOC_STATUS", f.status_type)
         << " (writers " << f.n_writers << ": direct " << f.n_direct << ", data-function " << f.n_data_fn << ", out-pointer " << f.n_outptr << ", initial values " << f.n_init
         << "; instances " << f.n_instances << "); hits on the field: escape " << f.own_escape << ", cast " << f.own_cast << ", union " << f.own_union << ", unknown effect "
         << f.own_unknown << "; hits on other fields of the type " << f.sibling_hits << "; untyped write sites in the program " << f.untyped;
      if (f.flags != 0) os << "; flags " << flags_text("KZ_FSTATUS_FLAG", f.flags);
    }
    os << "; not covered: calls whose targets are not resolved (" << unresolved_sites_.size()
       << " in these facts; they may run any address-taken function), writes in units outside the closure (not read)";
    return os.str();
  }
  // relations rules revision 7 (O5): the out-pointer arguments of a call of a key-parameter function -- the parameters of
  // the callee, other than the key, that it stores through (PARAM_FLOW out-store, the shallowest row) -- with the argument
  // as passed (ARGFLOW: its text, source, field, base and object).  The object's id, when the callee gives one, is among them.
  std::vector<std::string> id_holders(uint32_t ci, uint32_t F, uint32_t key) const {
    std::vector<std::string> out;
    if (!defined(F)) return out;
    const kz_fact_call& c = calls_[ci];
    for (uint32_t j = 0; j < c.nargs; ++j) {
      if (j == key) continue;
      auto it = pflow_of_.find({F, j});
      if (it == pflow_of_.end()) continue;
      const kz_fact_pflow* best = nullptr;
      for (uint32_t i : it->second) {
        const kz_fact_pflow& p = pflow_[i];
        if (p.kind == KZ_PFLOW_KIND_OUT_STORE && (best == nullptr || p.depth < best->depth)) best = &p;
      }
      if (best == nullptr) continue;
      std::string arg = "no ARGFLOW row";
      auto af = argflow_at_.find({c.site, j});
      if (af != argflow_at_.end()) {
        const kz_fact_argflow& a = argflow_[af->second];
        arg = "[" + str(a.text) + "] " + enum_text("KZ_ARGSRC", a.src);
        if (!str(a.type).empty() && !str(a.field).empty()) arg += " " + str(a.type) + "." + str(a.field);
        arg += " base " + enum_text("KZ_ARGBASE", a.base);
        if (!str(a.object).empty()) arg += " object " + str(a.object);
      }
      const uint32_t sink = best->depth == 0 ? best->fn : best->sink_fn;
      out.push_back("arg " + std::to_string(j) + " " + arg + " <- " + fname(F) + " stores through its parameter " + std::to_string(j) + " in " + fname(sink) + " at " +
                    pos(str(best->file), best->line, best->col) + " [" + str(best->text) + "]");
    }
    return out;
  }
  // relations rules revision 7: the constants a creator's callee may return as these facts give them -- its RET_CONSTS and
  // those of the functions whose call result it returns (RETURNS call-result rows), followed transitively -- each with
  // the chain it comes through and its condition in the innermost function; the returns not followed (a parameter, a
  // local, a field, an expression, a call of a function without a body) go to `open`.  The set is what may be returned,
  // not a proof of what is.
  // Revision 9: a constant is marked "not shown to reach the return" when it comes through a definition that the
  // RETURNS rows flag redefined later on a path to the return (the RET_CONSTS row of the constant, or a call-result
  // return on its chain): the value of such a definition may be overwritten before the return on the path where the
  // constant is returned (reaching definitions without the values of the conditions).  `marked` counts those lines.
  std::vector<std::string> callee_returns(uint32_t F, std::vector<std::string>* open, size_t* marked) const {
    std::vector<std::string> out;
    std::set<uint32_t> seen;
    std::set<std::pair<int64_t, std::string>> printed;
    std::function<void(uint32_t, const std::string&, const std::string&)> rec = [&](uint32_t fn, const std::string& chain, const std::string& killed) {
      if (!defined(fn) || !seen.insert(fn).second) return;
      auto it = retconsts_of_.find(fn);
      if (it != retconsts_of_.end())
        for (uint32_t i : it->second) {
          const kz_fact_retconst& r = retconsts_[i];
          if (!printed.insert({r.value, str(r.text) + "|" + chain + "|" + std::to_string(i)}).second) continue;
          std::string k = killed;
          if (k.empty() && (r.flags & KZ_RET_FLAG_REDEFINED)) k = "a definition of the value returned in " + fname(fn) + " is redefined later on a path to the return";
          out.push_back(fname(F) + " returns " + std::to_string(r.value) + " [" + str(r.text) + "]" + (chain.empty() ? "" : " through " + chain) +
                        (r.cond_root == kN ? std::string(" with no condition") : " when " + ex(r.cond_root)) +
                        (k.empty() ? std::string() : "; not shown to reach the return: " + k));
          if (!k.empty()) ++*marked;
        }
      auto rit = returns_of_.find(fn);
      if (rit == returns_of_.end()) return;
      for (uint32_t ri : rit->second) {
        const kz_fact_return& r = returns_[ri];
        if (r.vkind == KZ_RET_VALUE_CONST || r.vkind == KZ_RET_VALUE_STRING) continue;
        const std::string at = pos(str(r.file), r.line, r.col);
        if (r.vkind == KZ_RET_VALUE_CALL && r.value_root < exprs_.size() && exprs_[r.value_root].op == KZ_EXPR_OP_CALL_RESULT) {
          const std::string callee = str(exprs_[r.value_root].name);
          std::string k = killed;
          if (k.empty() && (r.flags & KZ_RET_FLAG_REDEFINED))
            k = fname(fn) + " returns the result of " + callee + " at " + at + " from the definition at " + pos(str(r.def_file), r.def_line, r.def_col) + ", redefined later on a path to the return";
          bool any = false;
          auto fit = fn_by_name_.find(callee);
          if (fit != fn_by_name_.end())
            for (uint32_t g : fit->second)
              if (defined(g)) {
                any = true;
                rec(g, (chain.empty() ? fname(fn) : chain) + " -> " + fname(g), k);
              }
          if (!any) open->push_back(fname(fn) + " returns the result of " + callee + " at " + at + " (no body in these facts)");
          continue;
        }
        open->push_back(fname(fn) + " returns " + enum_text("KZ_RET_VALUE", r.vkind) + " [" + str(r.text) + "] at " + at);
      }
    };
    rec(F, "", "");
    std::sort(open->begin(), open->end());
    open->erase(std::unique(open->begin(), open->end()), open->end());
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
  // revision 11 (LOCK_HANDLES): the handle of the acquisition at the call `site` of lock `id`: the LOCK_HANDLES rows of the
  // callee's acquisitions of that lock, a handle that is a parameter of the callee read at this call (its ARGFLOW source a
  // field of a global object, judged by FIELD_STATUS as the extraction judges a field handle).  False when there is none or
  // the rows disagree.
  bool handle_at(uint32_t site, uint32_t id, std::string* field, uint8_t* status, std::string* premise) const {
    auto cit = call_by_site_.find(site);
    if (cit == call_by_site_.end()) return false;
    const kz_fact_call& c = calls_[cit->second];
    auto hit = lockh_of_fn_.find(c.callee_fn);
    if (hit == lockh_of_fn_.end()) return false;
    bool any = false;
    for (uint32_t hi : hit->second) {
      const kz_fact_lockh& h = lockh_[hi];
      if (h.lock_id != id || h.lockop >= lockops_.size() || lockops_[h.lockop].kind != KZ_LOCKOP_KIND_ACQUIRE) continue;
      std::string f = str(h.field), pr = str(h.premise);
      uint8_t st = h.status;
      if (f.empty() && h.handle_root < exprs_.size() && exprs_[h.handle_root].op == KZ_EXPR_OP_LOAD && exprs_[h.handle_root].flags == KZ_EXPR_REF_PARAM) {
        const uint32_t q = static_cast<uint32_t>(exprs_[h.handle_root].value);
        auto af = argflow_at_.find({site, q});
        if (af == argflow_at_.end()) return false;
        const kz_fact_argflow& a = argflow_[af->second];
        if (a.src != KZ_ARGSRC_FIELD || a.base != KZ_ARGBASE_GLOBAL || str(a.root).empty()) return false;
        f = str(a.type) + "." + str(a.field);
        auto fs = fstatus_of_.find(f);
        if (fs == fstatus_of_.end() || fstatus_[fs->second].status_field != KZ_LOC_STATUS_COMPLETE) st = KZ_LOCKH_STATUS_WRITERS_INCOMPLETE;
        else if (fstatus_[fs->second].once != KZ_FSTATUS_ONCE_ONCE) st = KZ_LOCKH_STATUS_NOT_STATIC;
        else st = h.sem != kN ? KZ_LOCKH_STATUS_STATIC_INSTANCE : KZ_LOCKH_STATUS_SEM_UNVERIFIED;
        pr = "the handle " + f + " passed at " + call_pos(cit->second) + " as parameter " + std::to_string(q) + " of " + fname(c.callee_fn) + " (" + enum_text("KZ_LOCKH_STATUS", st) + ")";
      }
      if (f.empty()) return false;
      if (any && (f != *field || st != *status)) return false;
      *field = f;
      *status = st;
      *premise = pr;
      any = true;
    }
    return any;
  }
  // the common handle of every acquisition in `sites` of lock `id`
  bool common_handle(const std::set<uint32_t>& sites, uint32_t id, std::string* field, uint8_t* status, std::string* premise) const {
    if (sites.empty()) return false;
    bool first = true;
    for (uint32_t st : sites) {
      std::string f, p;
      uint8_t s = 0;
      if (!handle_at(st, id, &f, &s, &p)) return false;
      if (!first && (f != *field || s != *status)) return false;
      *field = f;
      *status = s;
      if (first) *premise = p;
      first = false;
    }
    return true;
  }
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
  void path_b(RelReport* rep) {
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
    RelGroup out;
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
      RelRow r;
      r.path = 'B';
      r.number = n_b_;
      r.subject = tf;
      r.callee_fn = callee;
      r.param = argi;
      RowLines L(&r);
      {
        std::ostringstream h;
        h << tf << " -> " << fname(callee) << "#" << argi << " sites=" << afs.size() << " callee-check=" << (checks.empty() ? "none" : "yes");
        r.head = h.str();
      }
      std::map<std::string, uint32_t> handling;
      for (uint32_t ai : afs) {
        const uint32_t ci = call_by_site_.at(argflow_[ai].site);
        r.site_calls.push_back(ci);
        const std::string use = result_use(ci);
        ++handling[use];
        L.line("site") << "site " << call_pos(ci) << " in " << fname(calls_[ci].caller_fn) << " [" << str(argflow_[ai].text) << "] base=" << enum_text("KZ_ARGBASE", argflow_[ai].base)
                       << " hops=" << argflow_[ai].hops << " result=" << use;
      }
      int64_t iv = 0;
      std::string itext;
      const bool has_init = init_value(tf, &iv, &itext);
      L.line("field") << "field: " << (has_init ? itext : "no INIT row") << "; writers " << ws.size() << " from " << wsource;
      for (const WriterRow& w : ws)
        L.line("writer", 8) << w.kind << " at " << w.where << " value=" << w.value_text << (w.guard.empty() || w.guard == "-" ? "" : " guard=" + w.guard);
      bool b_zero = false;  // relations rules revision 7 (O4)
      L.line("writers") << writer_premises(tf, &b_zero);
      L.line("actors") << "actors: readers " << actors_text(ra) << "; writers " << actors_text(wa) << (undetermined ? " (undetermined)" : " (inter-actor)");
      for (const Check& ck : checks) L.line("callee-check") << "callee-check: " << fname(callee) << " returns " << ck.value << " [" << ck.text << "] when " << ck.cond;
      // one level into the calls the check's conditions make with the parameter
      std::set<std::string> deeper;
      for (const Check& ck : checks)
        visit(ck.cond_root, [&](const kz_fact_expr& e) {
          if (e.op == KZ_EXPR_OP_CALL_RESULT && has_param(e.a, argi)) deeper.insert(str(e.name));
        });
      for (const std::string& g : deeper) {
        const std::string rc = retconsts_text(g);
        if (!rc.empty()) L.line("callee-check (called with the parameter)") << "callee-check (called with the parameter): " << g << " returns " << rc;
      }
      L.line("handling") << "handling: discarded " << handling["discarded"] << ", used " << handling["used"] << ", tested " << handling["tested"];
      if (result_flow_) b_result_flow(L, r.site_calls, checks);
      L.done();
      {
        std::ostringstream d;
        d << "path B: the shared field " << tf << " is read as argument " << argi << " of " << fname(callee) << " at " << afs.size() << " sites; its writers run in "
          << (undetermined ? "an undetermined actor" : "another actor") << (checks.empty() ? "" : "; the callee's constant returns depend on the argument");
        r.dependency = d.str();
      }
      r.run_order = "no record given";
      const std::string feas = b_feasibility(ra, wa);
      r.feasibility = feas;
      r.feasibility_grade = last_feas_grade_;
      r.violation = violation_text();
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
      r.flip_grade = grade;
      r.flip = grade + ": " + why;
      bool l0 = uses_script_root(ra) || uses_script_root(wa);
      r.layers = layers_text(true, l0, wsource.compare(0, 7, "WRITERS") == 0 ? "(writers: rule targets)" : "");
      // relations rules revision 6: the incompleteness reasons of the field-argument row (the feasibility's in b_feasibility)
      if (undetermined) inc(kActorNoRoot);
      inc_writers(tf);
      if (b_zero) inc(kWriterZero);
      if (grade == "static-undetermined") inc(kAbsentCallee);
      finish(&r);
      key_b(&r);
      site_keys(&r);
      out.rows.push_back(std::move(r));
    }
    std::ostringstream os;
    os << "[B] " << n_b_ << " (field-argument groups " << groups.size() << "; left out: no writer " << no_writer << ", one actor only " << same_actor << ")\n";
    out.text = os.str();
    rep->groups.push_back(std::move(out));
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
    inc(ordered ? kFixedCreationOnly : !images.empty() ? kFixedImageOrder : kFixedNoOrder);
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
    bool callback = false;     // a call of a function without a body that receives the reading function
    bool unresolved = false;   // indirect calls the facts do not resolve that may call the reading function
    bool open_reach = false;   // not reached, but an unresolved indirect call of the threads may run its function (revision 5)
    uint32_t call_fn = kN;     // the function that makes the call of the context
    std::string kind = "itself";  // the context in the use-site key: itself, entry, call, indirect-call, callback, unresolved
    uint32_t ci = kN;             // the call of the context (FACTS row), or kN
  };
  // the facts path C's feasibility reads besides the lock sets
  struct CExtra {
    uint32_t site = kN;                 // the spawn call
    uint32_t creator = kN;              // its function
    std::string tf;                     // the field
    std::vector<std::string> unknown;   // calls after the spawn that may write anything (unresolved indirect calls)
    std::vector<std::string> callers;   // writes of the field in the creator's callers' own bodies
    std::string callers_names;          // the creator's callers
    std::string callers_open;           // why the creator's callers are not all known ("" when they are)
    std::set<uint32_t> after_fns;       // the creator and the functions its calls after the spawn may run
    std::string reenter;                // a call after the spawn from which the creator runs again ("" when none)
    uint32_t spawn_ci = kN;             // the spawn call (index into FACTS)
    uint32_t read_fn = kN, ctx_fn = kN; // the reading function, the function of the call of the context
  };
  void path_c(RelReport* rep) {
    RelGroup grp;
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
      const ChildSet& cs = child_set(site, resolved);
      const std::set<uint32_t>& child = cs.roots;
      std::set<uint32_t> threads = cs.roots;  // the child and its descendants
      threads.insert(cs.descendants.begin(), cs.descendants.end());
      // code the threads of this spawn run: the actors' code (a trampoline's code is its roots'), or the call graph from them
      auto runs_in = [&](uint32_t fn) {
        if (cs.code.count(fn) != 0) return true;
        for (uint32_t a : actor_set(fn))
          if (threads.count(a) != 0) return true;
        return false;
      };
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
      std::set<uint32_t> after_fns{P};  // the creator and the functions its calls after the spawn may run
      std::string reenter;              // the call after the spawn from which the creator runs again (revision 5)
      auto note_unknown = [&](uint32_t ci, const std::string& via) {
        auto w = unresolved_why_.find(calls_[ci].site);
        const std::string why = w == unresolved_why_.end() ? std::string() : w->second;
        const std::string t = call_pos(ci) + " in " + fname(calls_[ci].caller_fn) + via + " (" +
                              (calls_[ci].kind != KZ_CALL_KIND_INDIRECT ? std::string("?")
                               : why == "incomplete-source"
                                   ? std::string("indirect call, targets resolved but not all: incomplete-source")
                                   : std::string("indirect call, targets not resolved")) +
                              ")";
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
              after_fns.insert(h);
              if (h == P && reenter.empty()) reenter = call_pos(cc->second);
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
      // a call that passes P to a callee without a body: that callee may call P back during the call, and the function
      // that makes the call runs on after it returns
      auto pcb = callback_callers_.find(P);
      if (pcb != callback_callers_.end()) pcalls.insert(pcalls.end(), pcb->second.begin(), pcb->second.end());
      std::set<uint32_t> pcallers;
      for (uint32_t ci : pcalls) pcallers.insert(calls_[ci].caller_fn);
      std::string pcallers_names;
      for (uint32_t q : pcallers) pcallers_names += (pcallers_names.empty() ? "" : ", ") + fname(q);
      // the callers are not all known when P's address is taken and an indirect call the facts do not resolve exists
      std::string pcallers_open;
      if (P < funcs_.size() && (funcs_[P].flags & KZ_FUNC_FLAG_ADDRESS_TAKEN) != 0 && !unresolved_sites_.empty()) {
        std::vector<std::string> sites;
        for (uint32_t u : unresolved_sites_) {
          auto uc = call_by_site_.find(u);
          if (uc != call_by_site_.end()) sites.push_back(call_pos(uc->second) + " in " + fname(calls_[uc->second].caller_fn));
          if (sites.size() == 8) break;
        }
        pcallers_open = fname(P) + " is address-taken and these facts have " + std::to_string(unresolved_sites_.size()) +
                        " indirect call(s) whose targets are not resolved, so its callers are not all known (" + join(sites, "; ") +
                        (unresolved_sites_.size() > sites.size() ? "; ..." : "") + ")";
      }
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
        x.callers_open = pcallers_open;
        x.after_fns = after_fns;
        x.reenter = reenter;
        x.spawn_ci = si;
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
          // revision 5: a function the threads do not reach in the static call graph may still run in them when their
          // code makes an indirect call whose targets are not (all) resolved and the function runs from an
          // address-taken one (such a call may call that); its contexts are graded `undecidable` below
          const bool via_unres = !runs_in(u.fn) && !cs.unres.empty() && at_reach(u.fn);
          if ((!runs_in(u.fn) && !via_unres) || u.fn == P) continue;
          if (via_unres) ++n_c_open_reach_reads_;
          // the contexts of the read in the threads of the spawn: the read itself when it holds a static lock of its
          // own; else the read run as the entry of one of those threads (no caller lock), each direct or resolved indirect
          // call of its function, each call that passes the function to a callee without a body (it may call it back
          // under the locks of that call), and, when its address is taken, the indirect calls the facts do not resolve in
          // code those threads run (no lock known: undecidable).  A call those threads do not reach gets no grade, or
          // `undecidable` when the set of those threads is not complete.
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
            if (threads.count(u.fn) != 0 || resolved.count(u.fn) != 0) {
              Ctx c{"the read run as the entry of the spawned thread, under no lock of a caller", urow, kN, true, false};
              c.kind = "entry";
              ctxs.push_back(c);
            }
            std::vector<std::pair<uint32_t, int>> calls;  // (call, 0 direct / 1 resolved indirect / 2 a callee without a body it is passed to)
            for (uint32_t ci : callers_[u.fn]) calls.push_back({ci, 0});
            auto ic = indirect_callers_.find(u.fn);
            if (ic != indirect_callers_.end())
              for (uint32_t ci : ic->second) calls.push_back({ci, 1});
            auto cb = callback_callers_.find(u.fn);
            if (cb != callback_callers_.end())
              for (uint32_t ci : cb->second) calls.push_back({ci, 2});
            for (const auto& [ci, how] : calls) {
              const bool cc = runs_in(calls_[ci].caller_fn);
              auto cr = lockset_call_.find(calls_[ci].site);
              Ctx c{std::string(how == 1 ? "via the indirect call " : how == 2 ? "called back from the call " : "via the call ") + call_pos(ci) + " in " +
                        fname(calls_[ci].caller_fn) + (how == 2 ? " of " + str(calls_[ci].callee) + ", a function without a body that receives " + fname(u.fn) : "") +
                        (cc ? "" : "; the threads this spawn starts do not reach this call"),
                    urow, cr == lockset_call_.end() ? kN : cr->second, false, !cc};
              c.callback = how == 2;
              c.call_fn = calls_[ci].caller_fn;
              c.kind = how == 1 ? "indirect-call" : how == 2 ? "callback" : "call";
              c.ci = ci;
              c.open_reach = !cc && !cs.unres.empty() && at_reach(calls_[ci].caller_fn);
              ctxs.push_back(c);
            }
            // an indirect call the facts do not resolve, in code these threads run, may call the reading function when its
            // address is taken: no lock of a caller is known
            if (u.fn < funcs_.size() && (funcs_[u.fn].flags & KZ_FUNC_FLAG_ADDRESS_TAKEN) != 0) {
              std::vector<std::string> open;
              size_t n_open = 0;
              for (uint32_t us : unresolved_sites_) {
                auto uc = call_by_site_.find(us);
                if (uc == call_by_site_.end() || !runs_in(calls_[uc->second].caller_fn)) continue;
                ++n_open;
                if (open.size() < 8) open.push_back(call_pos(uc->second) + " in " + fname(calls_[uc->second].caller_fn));
              }
              if (n_open != 0) {
                Ctx c{"through an indirect call whose targets are not resolved (" + fname(u.fn) + " is address-taken): " + join(open, "; ") +
                          (n_open > open.size() ? "; ... (" + std::to_string(n_open) + " calls)" : ""),
                      urow, kN, false, false};
                c.unresolved = true;
                c.kind = "unresolved";
                ctxs.push_back(c);
              }
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
            if (cx.not_reached && cs.incomplete.empty() && !cx.open_reach) ++not_reached_ctx;
            if (cx.not_reached && !cs.incomplete.empty()) ++n_c_incomplete_ctx_;
            if (cx.not_reached && cs.incomplete.empty() && cx.open_reach) ++n_c_open_reach_ctx_;
            if (cx.callback) ++n_c_callback_ctx_;
            if (cx.unresolved) ++n_c_unresolved_ctx_;
            RelRow r;
            r.path = 'C';
            r.number = n_c_;
            r.subject = tf;
            r.call = si;
            r.caller_fn = P;
            r.callee_fn = s.callee_fn;
            r.read = ri;
            r.read_fn = u.fn;
            r.context_kind = cx.kind;
            r.context_call = cx.ci;
            RowLines L(&r);
            {
              std::ostringstream h;
              h << "spawn " << call_pos(si) << " in " << fname(P) << " -> " << fname(s.callee_fn == kN ? s.caller_fn : s.callee_fn) << "; field " << tf << "; read "
                << pos(str(u.file), u.line, u.col) << " in " << fname(u.fn) << " (" << cx.where << ")" << (self.empty() ? "" : " self-identity");
              r.head = h.str();
            }
            {
              std::ostream& sl = L.line("spawn");
              sl << "spawn: callee " << str(s.callee) << ", entries";
              for (uint32_t e : resolved) sl << " " << fname(e);
              sl << (image_spawn_.count(site) != 0 ? " (from the static image of the table the entry argument is read from)" : "");
              sl << "; child actors " << actors_text(child);
              if (!cs.descendants.empty()) sl << "; threads the child's code starts (descendants) " << actors_text(cs.descendants);
              sl << "; creator identity: " << (identity.empty() ? "none found" : identity) << "; locks held " << s_held;
            }
            for (const WriteCtx& wc : wrows) {
              if (wc.spawn_arg != kN) {
                auto aa = argflow_at_.find({site, wc.spawn_arg});
                L.line("write after the spawn by the spawn operation")
                    << "write after the spawn by the spawn operation: " << fname(wc.fn) << " stores through arg " << wc.spawn_arg << " ["
                    << (aa != argflow_at_.end() ? str(argflow_[aa->second].text) : std::string("?")) << "] inside the call at " << call_pos(si)
                    << " (PARAM_FLOW out-store; its order to the thread creation inside the call is not read) locks not known";
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
                L.line("write after the spawn") << "write after the spawn: " << pos(str(w.file), w.line, w.col) << key << how << " locks " << held_text(wc.row);
              } else {
                const kz_fact_lockset& c = lockset_[wc.call_row];
                std::ostream& wl = L.line("write after the spawn in a callee");
                wl << "write after the spawn in " << fname(w.fn) << " (through the call " << pos(str(c.file), c.line, c.col) << "): " << pos(str(w.file), w.line, w.col) << key << how
                   << " locks " << held_text(wc.row) << "; at the call " << held_text(wc.call_row);
                if (!wc.first_level) wl << " (not composed: the write is more than one call below the creator)";
                else if (entry_release_.count(wc.row) != 0) wl << " (not composed: " << release_text(wc.row) << ")";
              }
            }
            if (!unknown.empty())
              L.line("calls after the spawn whose targets are not resolved")
                  << "calls after the spawn whose targets are not resolved (they may write " << tf << " under no known lock): " << join(unknown, "; ");
            {
              std::ostream& rl = L.line("read");
              rl << "read: [" << str(u.text) << "] use=" << enum_text("KZ_READ_USE", u.use) << " index=" << ex(u.index_root) << " guard=" << ex(u.guard_root) << " locks "
                 << (cx.row == kN ? "{} (no LOCKSET row)" : held_text(cx.row));
              if (cx.call_row != kN) {
                rl << "; at the call " << held_text(cx.call_row);
                if (cx.row != kN && entry_release_.count(cx.row) != 0) rl << " (not composed: " << release_text(cx.row) << ")";
              }
            }
            if (!self.empty()) L.line("read subscript") << "read subscript: the reader's own identity: " << self;
            bool c_zero = false;  // relations rules revision 7 (O4)
            L.line("writers") << writer_premises(tf, &c_zero);
            L.done();
            r.dependency = "path C: the creator writes " + tf + " after the spawn; code the spawned entry reaches reads it" + (self.empty() ? "" : " at the subscript of its own identity") +
                           "; that the two identities are equal is not decided statically";
            r.run_order = "no record given";
            std::string feas;
            // the lock primitives of P0: a row that names a lock, and every grade the lock sets give, rest on their semantics
            bool names_lock = s_held.compare(0, 2, "{}") != 0 || (cx.row != kN && !held(cx.row).empty()) || (cx.call_row != kN && !held(cx.call_row).empty());
            for (const WriteCtx& wc : wrows)
              if (wc.row != kN && (!held(wc.row).empty() || (wc.call_row != kN && !held(wc.call_row).empty()))) names_lock = true;
            if (names_lock) note_lock_prims();
            if (cx.not_reached && !cs.incomplete.empty()) {
              feas = "undecidable: the threads this spawn starts that these facts resolve (the child and its descendants) do not reach this call, but that set is not complete: " +
                     join(cs.incomplete, "; ");
              inc(kFixedChildSet);
              tally_feas("undecidable");
            } else if (cx.not_reached && cx.open_reach) {
              // revision 5: an unresolved indirect call of the threads may call an address-taken function from which
              // the call of this context runs
              feas = "undecidable: the threads this spawn starts (the child and the threads its code starts) do not reach this call in the static call graph, but their code "
                     "makes indirect calls whose targets are not (all) resolved (" + unres_text(cs.unres) + "), and " + fname(cx.call_fn) +
                     " runs from an address-taken function such a call may call: the locks held where it runs are not known";
              note_lock_prims();
              inc(kFixedUnresolved);
              tally_feas("undecidable");
            } else if (cx.not_reached) {
              feas = "not graded: the threads this spawn starts (the child and the threads its code starts) do not reach this call in the static call graph, so this context is "
                     "no read of theirs; no grade is given from the locks at the call" +
                     std::string(cs.unres.empty() ? "; their code makes no indirect call whose targets are not resolved"
                                                  : "; their indirect calls whose targets are not resolved may call only address-taken functions, and " + fname(cx.call_fn) +
                                                        " runs from none");
              tally_feas("not-graded");
            } else if (cx.unresolved) {
              feas = "undecidable: an indirect call whose targets these facts do not resolve may call " + fname(u.fn) + " in code these threads run; the locks held there are not known";
              note_lock_prims();
              inc(kFixedUnresolved);
              tally_feas("undecidable");
            } else {
              CExtra xc = x;
              xc.read_fn = u.fn;
              xc.ctx_fn = cx.call_row != kN ? lockset_[cx.call_row].fn : kN;
              feas = c_feasibility(lsit == lockset_call_.end() ? kN : lsit->second, wrows, cx, xc);
              if (last_feas_grade_ == "undecidable") inc(kFixedLocks);
            }
            r.feasibility = feas;
            r.feasibility_grade = last_feas_grade_;
            r.violation = violation_text();
            // flip: the reader's constant returns that depend on the field, before and after the write
            std::string grade = "none", why = "the reader's function returns no constant that depends on " + tf;
            const std::vector<Check> checks = checks_on_field(u.fn, tf);
            if (!checks.empty()) {
              int64_t iv = 0;
              std::string itext;
              const bool has_init = init_value(tf, &iv, &itext);
              // the values of the writes after the spawn the feasibility column counts (the creator's, its callees', the
              // record-wide and covering writes, the spawn operation's stores): a constant of a typed write (WRITERS), else
              // not evaluated
              std::vector<std::pair<int64_t, std::string>> wvals;
              std::set<std::string> unknown_vals;
              const size_t n_unknown_before = unknown_vals.size();
              auto wr = writers_of_.find(tf);
              for (const WriteCtx& wc : wrows) {
                if (wc.spawn_arg != kN) {
                  // the store of the spawn operation through its argument: the out-pointer writer of the spawn call
                  bool got = false;
                  if (wr != writers_of_.end())
                    for (uint32_t i : wr->second)
                      if (writers_[i].kind == KZ_WRITER_KIND_OUTPTR && writers_[i].fn == P && writers_[i].line == s.line && str(writers_[i].file) == str(s.file) &&
                          writers_[i].value_kind == KZ_ARG_KIND_CONST) {
                        got = true;
                        if (std::find(wvals.begin(), wvals.end(), std::make_pair(writers_[i].value, str(writers_[i].value_text))) == wvals.end())
                          wvals.push_back({writers_[i].value, str(writers_[i].value_text)});
                      }
                  if (!got) unknown_vals.insert("the store of the spawn operation through arg " + std::to_string(wc.spawn_arg));
                  continue;
                }
                const kz_fact_lockset& w = lockset_[wc.row];
                const std::string at = pos(str(w.file), w.line, w.col);
                if (!wc.how.empty()) {
                  unknown_vals.insert(std::string(wc.how == "record" ? "the write of the whole record" : "the covering write") + " at " + at);
                  continue;
                }
                bool found = false;
                if (wr != writers_of_.end())
                  for (uint32_t i : wr->second)
                    if (writers_[i].fn == w.fn && writers_[i].line == w.line && str(writers_[i].file) == str(w.file)) {
                      found = true;
                      if (writers_[i].value_kind == KZ_ARG_KIND_CONST) {
                        if (std::find(wvals.begin(), wvals.end(), std::make_pair(writers_[i].value, str(writers_[i].value_text))) == wvals.end())
                          wvals.push_back({writers_[i].value, str(writers_[i].value_text)});
                      } else {
                        unknown_vals.insert("a value that is no constant at " + at);
                      }
                    }
                if (!found) unknown_vals.insert("no WRITERS row of the write at " + at + " (not a target)");
              }
              const bool write_unknown = unknown_vals.size() != n_unknown_before;
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
              if (write_unknown) any_unknown = true;
              if (wvals.empty()) unknown_vals.insert("no constant value among the writes of " + tf + " after the spawn");
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
            r.flip_grade = grade;
            r.flip = grade + ": " + why;
            r.layers = layers_text(true, uses_script_root(child) || uses_script_root(cs.descendants) || uses_script_root(ua));
            // relations rules revision 6: the incompleteness reasons of the creation row (the feasibility's above)
            inc(kLinkIdentity);
            if (!self.empty()) inc(kLinkSelf);
            if (!cs.incomplete.empty()) inc(kActorChildSet);
            if (cs.cut) inc(kTruncImage);
            inc_writers(tf);
            if (c_zero) inc(kWriterZero);
            if (grade == "static-undetermined") inc(kAbsentReader);
            finish(&r);
            key_c(&r);
            grp.rows.push_back(std::move(r));
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
    std::ostringstream os;
    os << "[C] " << n_c_ << " (spawn calls " << spawn_calls << ", with no resolved entry " << unresolved << ", with no event after them on their control-flow graph " << no_paths
       << "; fields written after a spawn only in the functions the creator calls, not paired in this revision: " << callee_only_fields
       << "; record types written whole after a spawn with no typed write of a field of theirs by the creator, not paired: " << record_only_fields
       << "; read contexts through a call the threads of the spawn (the child and the threads its code starts) do not reach (rows with no grade): " << not_reached_ctx
       << ", such contexts of a spawn whose threads are not all resolved (undecidable): " << n_c_incomplete_ctx_
       << ", such contexts an indirect call of those threads whose targets are not resolved may reach (undecidable): " << n_c_open_reach_ctx_
       << " (reads taken in only so: " << n_c_open_reach_reads_ << ")"
       << "; contexts called back from a function without a body: " << n_c_callback_ctx_ << "; contexts through indirect calls the facts do not resolve: "
       << n_c_unresolved_ctx_ << ")\n";
    grp.text = os.str();
    rep->groups.push_back(std::move(grp));
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
  // §3.3 item 3): what the callers run after it returns is not read beyond their own bodies.  Revision 4 adds: the
  // callers include the calls that pass the creator to a function without a body, and they are not all known (so
  // `undecidable`) when the creator is address-taken and an indirect call is not resolved; no function whose lock sets the
  // verdict reads calls a function that returns twice; the claim names the keys of the writes it concerns and, for a
  // derived spawn operation, that a path on which the operation reports failure after its thread started is not covered;
  // and it names the premises of path C (static_facts §5h).  Revision 5 (§5i): the creator run again after the spawn
  // (a call after it reaches the creator), or a write in a frame below the creator holding the lock from its own
  // acquisition at the spawn's call site, is a new acquisition (`possible`); the premises named add the mutex owner and
  // P0-only changes of a mutex object, the completeness of indirect-call targets apart from the incomplete-source forms,
  // and that an unresolved indirect call calls only address-taken functions.
  std::string c_feasibility(uint32_t s_row, const std::vector<WriteCtx>& w_rows, const Ctx& cx, const CExtra& x) {
    std::string grade, text;
    note_lock_prims();  // every grade here is read from the lock sets
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
      auto sites_text = [&](const std::set<uint32_t>& op) {
        std::vector<std::string> v;
        for (uint32_t o : op) {
          auto oc = call_by_site_.find(o);
          v.push_back(oc != call_by_site_.end() ? call_pos(oc->second) : "site " + std::to_string(o));
        }
        return join(v, ", ");
      };
      // revision 5: the creator runs again after the spawn (a call after it reaches the creator): the nested call
      // takes the lock again at the same call site, a new acquisition
      if (!x.reenter.empty()) {
        *why = "lock " + lock_text(id) + " is held at the spawn and at the writes after it, but " + fname(x.creator) + " runs again after the spawn (the call " + x.reenter +
               " reaches it): its acquisition " + sites_text(so) + " runs again in that nested call, a new acquisition, so the lock may be released and taken again between them";
        return false;
      }
      // a write in a frame below the creator that holds the lock from its own acquisition holds it from a new one, even
      // where that acquisition is at the call site of the spawn's (the same function entered again)
      for (const WriteCtx& wr : w_rows) {
        if (wr.call_row == kN || wr.spawn_arg != kN) continue;
        std::set<uint32_t> again;
        for (uint32_t o : openers(wr.row))
          if (so.count(o) != 0) again.insert(o);
        if (!again.empty()) {
          *why = "lock " + lock_text(id) + " is held at the write " + where_of(wr) + " in " + fname(wr.fn) + ", a frame below the creator, from its own acquisition " +
                 sites_text(again) + ": a new acquisition at the call site of the spawn's, so the lock may be released and taken again between them";
          return false;
        }
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
      std::string other_why, proven_handle, undec_handle;
      bool row_handle = false, row_handle_static = false;
      // revision 11: the acquisitions that hold a lock at the read (the read's own and those at the call that reaches it)
      std::set<uint32_t> ro;
      if (lock_handles_) {
        if (u_row != kN) ro = openers(u_row);
        if (call_row != kN && !dropped)
          for (uint32_t o : openers(call_row)) ro.insert(o);
      }
      for (uint32_t id : hu) {
        if (common.count(id) == 0) continue;
        std::string why;
        if (continuous(id, &why)) {
          std::string hf, hp, hf2, hp2;
          uint8_t hst = 0, hst2 = 0;
          const bool by_handle = !static_lock(id) && lock_handles_ && common_handle(so, id, &hf, &hst, &hp) && common_handle(ro, id, &hf2, &hst2, &hp2) && hf == hf2 && hst == hst2;
          if (by_handle) row_handle = true;
          if (static_lock(id)) {
            if (proven == kN) proven = id;
          } else if (by_handle && hst == KZ_LOCKH_STATUS_STATIC_INSTANCE) {
            row_handle_static = true;
            if (proven == kN) {
              proven = id;
              proven_handle = hf + " (" + hp + ")";
            }
          } else if (undec == kN) {
            undec = id;
            if (by_handle) undec_handle = hf + " " + enum_text("KZ_LOCKH_STATUS", hst) + " (" + hp + ")";
          }
        } else if (other == kN) {
          other = id;
          other_why = why;
        }
      }
      if (row_handle) ++n_handle_rows_;
      if (row_handle_static) ++n_handle_static_;
      if (proven != kN) {
        tag_static();
        const std::string held_how = "lock " + lock_text(proven) + (proven_handle.empty() ? std::string() : ", one mutex by its handle " + proven_handle + ",") +
                                     " is held from the same acquisition across the spawn and every write of " + x.tf + " after it in this call of " +
                                     fname(x.creator) + ", that acquisition is not reached again after the spawn, and the read holds it";
        auto fs = fstatus_of_.find(x.tf);
        // the functions whose lock sets this verdict reads that call a function returning twice
        std::vector<std::string> twice;
        std::set<uint32_t> fns = x.after_fns;
        fns.insert(x.creator);
        if (x.read_fn != kN) fns.insert(x.read_fn);
        if (x.ctx_fn != kN) fns.insert(x.ctx_fn);
        for (uint32_t f : fns) {
          auto rt = returns_twice_.find(f);
          if (rt != returns_twice_.end())
            for (uint32_t ci : rt->second) twice.push_back(call_pos(ci) + " " + str(calls_[ci].callee) + " in " + fname(f));
        }
        if (fs == fstatus_of_.end() || fstatus_[fs->second].status_field != KZ_LOC_STATUS_COMPLETE) {
          grade = "undecidable";
          text = held_how + "; but the writer set of " + x.tf + " is not complete under the per-field rule (FIELD_STATUS " +
                 (fs == fstatus_of_.end() ? std::string("has no row") : "per-field " + enum_text("KZ_LOC_STATUS", fstatus_[fs->second].status_field)) +
                 (fs != fstatus_of_.end() && (fstatus_[fs->second].flags & KZ_FSTATUS_FLAG_ARRAY_ELEMENTS) != 0
                      ? "; an array field: a store to one of its elements makes no writer row in these facts"
                      : "") +
                 "), so the writes after the spawn are not known to be all its writes";
        } else if (!twice.empty()) {
          grade = "undecidable";
          text = held_how + "; but " + join(twice, "; ") +
                 " returns twice: a non-local jump back to it runs code in an order and under locks the control-flow graph does not have";
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
        } else if (!x.callers_open.empty()) {
          grade = "undecidable";
          text = held_how + "; but " + x.callers_open + ": a caller may write " + x.tf + " after it returns";
        } else {
          grade = "proven-impossible";
          text = held_how + ": the read cannot fall between them.  Scope: one call of " + fname(x.creator) +
                 " (ED §3.3 item 3), on its control-flow graph from the spawn to its return, loops included (LOCK_PATHS after-spawn); the writes counted are the typed writes of " +
                 x.tf + ", the writes of the whole record " + owner_type(x.tf) +
                 ", the data functions that may cover the field, and those of the functions the creator calls after the spawn; " + x.tf +
                 " is writer-set-complete under the per-field rule (an untyped store does not reach it under that rule); the callers of " + fname(x.creator) + " (" +
                 (x.callers_names.empty() ? std::string("none in these facts") : x.callers_names) +
                 ") write it nowhere in their own bodies; what their callees and their own callers run after it returns is not read";
          // what the claim is about: the writes of this call, on the paths that run them
          std::set<std::string> keys;
          auto wr = writers_of_.find(x.tf);
          for (const WriteCtx& wc : w_rows)
            if (wc.row != kN && wc.how.empty() && wr != writers_of_.end())
              for (uint32_t i : wr->second)
                if (writers_[i].fn == lockset_[wc.row].fn && writers_[i].line == lockset_[wc.row].line && str(writers_[i].file) == str(lockset_[wc.row].file))
                  keys.insert(str(writers_[i].key));
          text += "; it concerns the writes of this call: whether their keys (" + (keys.empty() ? std::string("not in WRITERS") : join(std::vector<std::string>(keys.begin(), keys.end()), "; ")) +
                  ") name the record of the child this call starts, and not one another call registers, is not decided";
          if (x.spawn_ci != kN && defined(calls_[x.spawn_ci].callee_fn))
            text += "; it covers the paths on which those writes run: where the spawn operation " + fname(calls_[x.spawn_ci].callee_fn) +
                    " reports a failure after its thread has started and the creator skips them, the child may read the earlier value (not read in this revision)";
          tag("premises of path C (static_facts §5h, §5i: lock identities overlap as their access paths show; functions without a body call back only the functions "
              "passed to them; returns-twice calls by name; a mutex is released only by the thread that holds it and its object changes only through the P0 "
              "primitives; indirect-call target sets are complete apart from the incomplete-source forms the extraction checks; an unresolved indirect call "
              "calls only address-taken functions)");
          std::string on;
          for (const char* a : {"assume.lock_acquisition_succeeds", "assume.static_instance_read_after_write", "assume.no_cross_member_pointer_arithmetic"})
            if (assumptions_.count(a) != 0) on += std::string(on.empty() ? "" : ", ") + a;
          on += std::string(on.empty() ? "" : ", ") + "the per-field completeness rule of FIELD_STATUS that makes " + lock_text(proven) + " a static instance and " + x.tf +
                " complete (PI decision pending)";
          if (proven < lockids_.size() && (lockids_[proven].flags & KZ_LOCKID_FLAG_TYPE_EXPOSED) != 0) on += "; under the type-level rule the lock is no static instance";
          on += "; and the premises of path C (static_facts §5h, §5i): two lock identities name one mutex only as their access paths may overlap (the same global "
                "object with one path a prefix of the other or two members of one union, a pointer path aligned on a record type the other path names), a function "
                "without a body has no lock effect of its own and calls back only the functions passed to it in that call (their lock effect is read at the "
                "call), a function returns twice only by the names of the C library list, a mutex is released only by the thread that holds it and its object "
                "changes only through the P0 primitives (no unlock by another thread, no plain store over a held mutex), the targets of an indirect call are all "
                "its resolved candidates unless the extraction marks it incomplete-source (it marks every store the flow graph does not follow that goes through "
                "an address of a function pointer taken, passed, decayed or copied in these units, a node of another type, a function without a body or a field "
                "not complete per field; a store by inline assembly or through pointer arithmetic from another object is not seen), and an indirect call whose "
                "targets are not resolved calls only address-taken "
                "functions (a function whose address a symbol lookup returns at run time is not counted, apart from the startup-script entries resolved)";
          text += "; rests on " + on;
          if (!proven_handle.empty())
            text += "; and the lock identity by handle (LOCK_HANDLES, relations rules revision 11): every acquisition at the spawn and at the read passes that handle, "
                    "its field's writer set is complete per field with one writer instance that runs once, a verified semantics item says the operation acts on the "
                    "object the handle references, and that object is not deleted and created again between the acquisitions";
        }
        note_lock_prims();
      } else if (undec != kN) {
        grade = "undecidable";
        text = "lock " + lock_text(undec) + " is held from the same acquisition across the spawn and every write after it, and the read holds it, but its identity is no static "
               "instance: that the read's lock and the writes' lock are one mutex is not proven (must identity, ED §3.3)";
        if (!undec_handle.empty()) text += "; every acquisition of it at the spawn and at the read passes the handle " + undec_handle + ", which is no static instance with a verified semantics item";
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
    // A2 (relations rules revision 7): the indirect call, and the parameter of the search function that carries the callee
    // (CALL_TARGETS `via` "P:<search function>#<j>" of the call's row for the comparison's function), or kN with the why
    uint32_t call = kN, cb = kN;
    std::string cb_why;
  };
  std::vector<Search> searches_of(const KeyCmp& k) const {
    std::vector<Search> out;
    if (k.loop != kN) {
      Search a1;
      a1.fn = k.fn;
      a1.loop = k.loop;
      a1.param = k.param;
      a1.kind = "A1 search loop";
      out.push_back(a1);
    }
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
        Search s;
        s.fn = c.caller_fn;
        s.loop = c.loop;
        s.param = p;
        s.kind = "A2 callback search (indirect call " + call_pos(ci) + ")";
        s.call = static_cast<uint32_t>(ci);
        callback_param(ci, k.fn, &s.cb, &s.cb_why);
        out.push_back(s);
      }
    return out;
  }
  // The parameter of the calling function that carries the callee of an indirect call to `target`: the CALL_TARGETS row
  // of the call for that target names the node the function value came through; a parameter of the caller is written
  // "P:<function key>#<index>" (ast_fn.cpp, the flow-graph node of a parameter).  Anything else (a field, a global, a local
  // the flow graph does not name) leaves the callback not carried by a parameter: kN, with the node in `why`.
  void callback_param(uint32_t ci, uint32_t target, uint32_t* cb, std::string* why) const {
    *cb = kN;
    const kz_fact_call& c = calls_[ci];
    for (const kz_fact_target& t : targets_) {
      if (t.site != c.site || t.fn != target || t.kind != KZ_CALL_TARGET_KIND_INDIRECT) continue;
      const std::string via = str(t.via);
      const size_t hash = via.rfind('#');
      if (via.compare(0, 2, "P:") == 0 && hash != std::string::npos && hash + 1 < via.size()) {
        std::string key = via.substr(2, hash - 2);
        const size_t at = key.find('@');
        if (at != std::string::npos) key = key.substr(0, at);
        const std::string idx = via.substr(hash + 1);
        if (key == short_fname(c.caller_fn) && idx.find_first_not_of("0123456789") == std::string::npos) {
          *cb = static_cast<uint32_t>(std::stoul(idx));
          return;
        }
      }
      *why = "the callee of the indirect call " + call_pos(ci) + " comes through " + (via.empty() ? std::string("no named node") : via) + ", no parameter of " + fname(c.caller_fn);
      return;
    }
    *why = "the indirect call " + call_pos(ci) + " has no CALL_TARGETS row for " + fname(target);
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
  // ---- relations rules revision 7: namespace membership by the callback passed, partition by the table selector only
  // A state of the walk from a search up the chains of calls that carry the key (PARAM_FLOW call-argument rows of depth 0
  // between key-parameter functions): how the comparison is reached from (fn, param).  kind 0: through direct calls to a
  // comparison in its own search loop (A1); 1: the callback of an A2 search is parameter `idx` of fn; 2: the callback is
  // the function `name`; 3: the callback is not resolved (`how` says why).  `target` is the comparison's function.
  struct CbState {
    int kind = 0;
    uint32_t idx = kN, target = kN;
    std::string name;
    bool operator<(const CbState& o) const { return std::tie(kind, idx, target, name) < std::tie(o.kind, o.idx, o.target, o.name); }
  };
  using CbStates = std::map<std::pair<uint32_t, uint32_t>, std::map<CbState, std::string>>;  // (fn, param) -> state -> how
  CbStates membership_states(const Namespace& n) const {
    std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> pred;  // (next_fn, next_param) -> PARAM_FLOW rows
    for (size_t i = 0; i < pflow_.size(); ++i) {
      const kz_fact_pflow& p = pflow_[i];
      if (p.kind != KZ_PFLOW_KIND_CALL_ARG || p.depth != 0 || p.next_fn == kN || n.key_params.count({p.fn, p.param}) == 0) continue;
      pred[{p.next_fn, p.next_param}].push_back(static_cast<uint32_t>(i));
    }
    CbStates st;
    std::vector<std::tuple<uint32_t, uint32_t, CbState>> work;
    auto add = [&](uint32_t fn, uint32_t param, const CbState& cs, const std::string& how) {
      if (st[{fn, param}].emplace(cs, how).second) work.emplace_back(fn, param, cs);
    };
    for (const KeyCmp& k : n.cmps)
      for (const Search& sr : searches_of(k)) {
        if (sr.fn == k.fn) {
          if (k.param != kN) add(k.fn, k.param, CbState{0, kN, k.fn, ""}, "the comparison " + k.how + " in its own search loop");
          continue;
        }
        if (sr.param == kN) continue;
        if (sr.cb == kN) add(sr.fn, sr.param, CbState{3, kN, k.fn, ""}, sr.cb_why);
        else add(sr.fn, sr.param, CbState{1, sr.cb, k.fn, ""}, "");
      }
    while (!work.empty()) {
      const auto [h, q, cs] = work.back();
      work.pop_back();
      const std::string how = st[{h, q}][cs];
      auto pit = pred.find({h, q});
      if (pit == pred.end()) continue;
      for (uint32_t i : pit->second) {
        const kz_fact_pflow& p = pflow_[i];
        CbState next = cs;
        std::string nhow = how;
        if (cs.kind == 1) {
          auto af = argflow_at_.find({p.site, cs.idx});
          auto cit = call_by_site_.find(p.site);
          const std::string at = cit != call_by_site_.end() ? call_pos(cit->second) : "site " + std::to_string(p.site);
          if (af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_FUNCTION) {
            next = CbState{2, kN, cs.target, str(argflow_[af->second].root)};
            nhow = str(argflow_[af->second].root) + " passed as argument " + std::to_string(cs.idx) + " at " + at + " in " + fname(p.fn);
          } else if (af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_PARAM && argflow_[af->second].param != kN) {
            next = CbState{1, argflow_[af->second].param, cs.target, ""};
          } else {
            next = CbState{3, kN, cs.target, ""};
            nhow = "argument " + std::to_string(cs.idx) + " at " + at + " is " +
                   (af == argflow_at_.end() ? std::string("no ARGFLOW row") : enum_text("KZ_ARGSRC", argflow_[af->second].src) + " [" + str(argflow_[af->second].text) + "]");
          }
        }
        add(p.fn, p.param, next, nhow);
      }
    }
    return st;
  }
  // The membership of a use (the call `ci` of the key-parameter function F, parameter k) in the namespace: 0 member (a
  // chain reaches a comparison of the namespace through its own search loop, or passes the comparison's function as the
  // callback of a search), 1 another callback only (every chain resolved passes another function), 2 unresolved.
  int use_membership(const CbStates& st, uint32_t ci, uint32_t F, uint32_t k, std::string* text) const {
    auto it = st.find({F, k});
    if (it == st.end()) {
      *text = "no chain of direct calls from the use to a search of the namespace in PARAM_FLOW";
      return 2;
    }
    std::string member, other, unres;
    const kz_fact_call& c = calls_[ci];
    for (const auto& [cs, how] : it->second) {
      std::string name = cs.name, h = how;
      int kind = cs.kind;
      if (kind == 1) {
        auto af = argflow_at_.find({c.site, cs.idx});
        if (af != argflow_at_.end() && argflow_[af->second].src == KZ_ARGSRC_FUNCTION) {
          kind = 2;
          name = str(argflow_[af->second].root);
          h = name + " passed as argument " + std::to_string(cs.idx) + " of the use";
        } else {
          kind = 3;
          h = "the callback is argument " + std::to_string(cs.idx) + " of the use" +
              (af == argflow_at_.end() ? std::string(" (no ARGFLOW row)") : " [" + str(argflow_[af->second].text) + "] (" + enum_text("KZ_ARGSRC", argflow_[af->second].src) + ")") +
              ", not followed beyond the use";
        }
      }
      if (kind == 0) member = member.empty() ? "reaches " + h : member;
      else if (kind == 2 && name == short_fname(cs.target)) member = member.empty() ? "the callback " + h + " is the comparison's function " + fname(cs.target) : member;
      else if (kind == 2) other = other.empty() ? "the callback " + h + " is not the comparison's function " + fname(cs.target) : other;
      else unres = unres.empty() ? h : unres;
    }
    if (!member.empty()) {
      *text = member;
      return 0;
    }
    if (!unres.empty()) {
      *text = unres;
      return 2;
    }
    *text = other;
    return 1;
  }
  // Whether a constant argument of a call reaches the table selection of a search of the namespace: a use of the callee's
  // parameter (PARAM_FLOW, any depth) that is a subscript in the search function, or a comparison in its search loop's
  // exit, or a store of it to a field the search function reads as a subscript, in the exit of its search loop, or into
  // a local that exit reads.  "" when none: the constant is no table selector (a lock mode, a flag, a count).
  std::string selects_table(uint32_t G, uint32_t a, const std::vector<Search>& ss) const {
    if (!defined(G)) return "";
    auto it = pflow_of_.find({G, a});
    if (it == pflow_of_.end()) return "";
    for (uint32_t i : it->second) {
      const kz_fact_pflow& p = pflow_[i];
      const uint32_t sink = p.depth == 0 ? p.fn : p.sink_fn;
      const std::string at = pos(str(p.file), p.line, p.col);
      for (const Search& sr : ss) {
        if (p.kind == KZ_PFLOW_KIND_SUBSCRIPT && sink == sr.fn) return "a subscript in " + fname(sr.fn) + " at " + at;
        if (p.kind == KZ_PFLOW_KIND_COMPARE && sink == sr.fn && in_exit(sr.loop, p.expr_root)) return "a comparison of the exit of the search loop " + loop_pos(sr.loop) + " at " + at;
        if (p.kind != KZ_PFLOW_KIND_STORE || p.expr_root == kN || p.expr_root >= exprs_.size()) continue;
        const kz_fact_expr& d = exprs_[p.expr_root];
        if (d.op != KZ_EXPR_OP_LOAD || d.flags != KZ_EXPR_REF_FIELD) continue;
        const std::string tf = str(d.name);
        auto rit = reads_of_field_.find(tf);
        if (rit == reads_of_field_.end()) continue;
        for (uint32_t ri : rit->second) {
          const kz_fact_read& r = reads_[ri];
          if (r.fn != sr.fn) continue;
          const std::string rat = pos(str(r.file), r.line, r.col);
          std::string local;
          if (r.ctx_root != kN && r.ctx_root < exprs_.size() && exprs_[r.ctx_root].op == KZ_EXPR_OP_LOAD && exprs_[r.ctx_root].flags == KZ_EXPR_REF_LOCAL) local = str(exprs_[r.ctx_root].name);
          if (r.use & KZ_READ_USE_SUBSCRIPT) return "stored to " + tf + " at " + at + ", which " + fname(sr.fn) + " reads as a subscript at " + rat;
          if (r.loop == sr.loop && (r.use & KZ_READ_USE_BRANCH) && in_exit(sr.loop, r.ctx_root)) return "stored to " + tf + " at " + at + ", read in the exit of the search loop at " + rat;
          if (!local.empty() && exit_reads_local(sr.loop, local))
            return "stored to " + tf + " at " + at + ", which " + fname(sr.fn) + " reads at " + rat + " into the local " + local + " of the search loop's exit";
        }
      }
    }
    return "";
  }
  // whether a node is (in) an exit condition of a loop (LOOP_EXITS)
  bool in_exit(uint32_t loop, uint32_t node) const {
    if (node == kN) return false;
    auto it = loopexits_of_.find(loop);
    if (it == loopexits_of_.end()) return false;
    bool found = false;
    for (uint32_t i : it->second)
      for (uint32_t root : {loopexits_[i].root, loopexits_[i].aux_root})
        if (root != kN && root < exprs_.size()) visit(root, [&](const kz_fact_expr& e) {
            if (&e == &exprs_[node]) found = true;
          });
    return found;
  }
  bool exit_reads_local(uint32_t loop, const std::string& local) const {
    auto it = loopexits_of_.find(loop);
    if (it == loopexits_of_.end()) return false;
    bool found = false;
    for (uint32_t i : it->second)
      for (uint32_t root : {loopexits_[i].root, loopexits_[i].aux_root})
        if (root != kN && root < exprs_.size()) visit(root, [&](const kz_fact_expr& e) {
            if (e.op == KZ_EXPR_OP_LOAD && e.flags == KZ_EXPR_REF_LOCAL && str(e.name) == local) found = true;
          });
    return found;
  }
  // The partition of a use: the constants of the first call down the chain from (fn, param) to the comparison that
  // passes a table selector (selects_table); the constants of that call and of the calls before it that are no table
  // selector go to `nonsel`.  `desc` names the selectors (value, text as written, callee and argument, and how it selects).
  std::string partition(uint32_t fn, uint32_t param, std::set<std::pair<uint32_t, uint32_t>>* seen, const std::set<std::pair<uint32_t, uint32_t>>& kp,
                        const std::vector<Search>& ss, std::string* desc, std::string* nonsel) const {
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
      std::string consts, d, ns;
      auto cit = call_by_site_.find(p->site);
      if (cit != call_by_site_.end())
        for (uint32_t a = 0; a < calls_[cit->second].nargs; ++a) {
          auto af = argflow_at_.find({p->site, a});
          if (af == argflow_at_.end() || argflow_[af->second].src != KZ_ARGSRC_CONST) continue;
          const std::string item = std::to_string(argflow_[af->second].value) + " [" + str(argflow_[af->second].text) + "] as " + str(calls_[cit->second].callee) + " arg " +
                                   std::to_string(a) + " at " + call_pos(cit->second);
          const std::string how = selects_table(calls_[cit->second].callee_fn, a, ss);
          if (how.empty()) {
            ns += std::string(ns.empty() ? "" : ", ") + item;
            continue;
          }
          consts += (consts.empty() ? "" : ",") + std::to_string(argflow_[af->second].value);
          d += std::string(d.empty() ? "" : ", ") + item + " (selects the table: " + how + ")";
        }
      if (!ns.empty()) *nonsel += std::string(nonsel->empty() ? "" : ", ") + ns;
      if (!consts.empty()) {
        *desc += std::string(desc->empty() ? "" : ", ") + d;
        return consts;
      }
      const std::string deeper = partition(p->next_fn, p->next_param, seen, kp, ss, desc, nonsel);
      if (!deeper.empty()) return deeper;
    }
    return "";
  }
  struct KeyValue {
    std::string status;  // "known", "unresolved"
    std::set<std::string> values;
    std::string text;
    Inc why = kIncCount;  // unresolved: the sub-reason of key_unresolved (relations rules revision 6)
    bool cut = false;     // the static image the key is read from was cut at the image budget
    // relations rules revision 7: the field whose static image gives the values (Type.field), and the resolved pointer
    // field the key is read through (O5 aliases)
    std::string image_field, alias;
  };
  // the key a use passes: a literal, a constant, or a field whose static image the facts reach
  KeyValue key_value(uint32_t site, uint32_t arg) const {
    KeyValue kv;
    auto af = argflow_at_.find({site, arg});
    if (af == argflow_at_.end()) {
      kv.status = "unresolved";
      kv.text = "no ARGFLOW row";
      kv.why = kKeyNoArgflow;
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
          kv.why = kKeyPtrField;
          return kv;
        }
        object = str(p.object);
        kv.text = "read through the pointer field " + str(p.type) + "." + str(p.field) + " -> " + object + " (PTR_FIELDS " + enum_text("KZ_PTRF_STATUS", p.status) + ")";
        kv.alias = str(p.type) + "." + str(p.field) + " -> " + object + " (PTR_FIELDS " + enum_text("KZ_PTRF_STATUS", p.status) + ")";
        if (p.idx_n != 0) kv.text += ", elements from " + std::to_string(p.elem) + " step " + std::to_string(p.idx_step) + " count " + std::to_string(p.idx_n);
        // the image of the object, at the elements the reader's index takes
        auto git = gimages_of_.find(base_name(object));
        uint16_t written = 0;
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
            const uint16_t wf = g.flags & (KZ_GIMAGE_FLAG_OBJECT_WRITTEN | KZ_GIMAGE_FLAG_FIELD_WRITTEN | KZ_GIMAGE_FLAG_RECORD_WRITTEN);
            if (wf != 0) {
              kv.text += " (" + written_flags(wf) + ")";
              written |= wf;
            }
          }
        }
        // relations rules revision 6: an image cut at the image budget gives the values of the part read, not all of
        // them, so the key is unresolved (with or without values found) and no exclusion and no flip rests on it
        const std::string cut = image_cut(base_name(object));
        if (!cut.empty()) {
          kv.status = "unresolved";
          kv.cut = true;
          kv.why = kKeyImageCut;
          kv.text += "; unresolved (image, not taken as the run-time value: " + image_value_why(str(a.type), field, written, cut) + ")";
          return kv;
        }
        // the image is the run-time value only under the rule of the static-image entries; otherwise no exclusion and no
        // flip rests on it
        if (!kv.values.empty()) {
          const std::string why = image_value_why(str(a.type), field, written);
          if (!why.empty()) {
            kv.status = "unresolved";
            kv.why = written != 0 ? kKeyImageWritten : kKeyImageNotValue;
            kv.text += written & (KZ_GIMAGE_FLAG_FIELD_WRITTEN | KZ_GIMAGE_FLAG_RECORD_WRITTEN) ? "; unresolved (image, field written at run time)"
                       : written != 0                                                         ? "; unresolved (image, object written at run time)"
                                                                                                : "; unresolved (image, not taken as the run-time value: " + why + ")";
            return kv;
          }
        }
        kv.status = kv.values.empty() ? "unresolved" : "known";
        if (!kv.values.empty()) kv.image_field = str(a.type) + "." + field;
        if (kv.values.empty()) {
          kv.text += "; no image leaf of the field";
          kv.why = kKeyNoImageLeaf;
        }
        return kv;
      }
      kv.status = "unresolved";
      kv.text = "field " + str(a.type) + "." + field + " read with base " + enum_text("KZ_ARGBASE", a.base) + " (no static image reached)";
      kv.why = kKeyFieldNoImage;
      return kv;
    }
    kv.status = "unresolved";
    kv.text = enum_text("KZ_ARGSRC", a.src) + " [" + str(a.text) + "]";
    kv.why = kKeyOther;
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

  void path_a(RelReport* rep) {
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
    const size_t heading = rep->groups.size();
    rep->groups.emplace_back();
    uint32_t no_table = 0;
    for (auto& [key, n] : spaces) {
      // a registration table is shared: the searches read the record type from a global object
      if (n.tables.empty()) {
        ++no_table;
        continue;
      }
      ++n_ns_;
      std::ostringstream rows;  // the lines of the namespace (no row)
      RelGroup g;
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
      path_a_uses(n, rows, &g.rows);
      g.text = rows.str();
      rep->groups.push_back(std::move(g));
    }
    std::ostringstream os;
    os << "[A] " << n_a_ << " (namespaces " << n_ns_ << ", key comparisons " << cmps.size() << "; left out: searches over no global table " << no_table << ")\n";
    rep->groups[heading].text = os.str();
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

  void path_a_uses(const Namespace& n, std::ostream& rows, std::vector<RelRow>* out) {
    // the functions that reach a write of the key (direct calls).  relations rules revision 12: a write whose stored
    // value WRITERS gives as a constant clears or initialises the key (a clearing store); a function is a live writer when
    // one of its writes of the key is not shown to be such a store (a WRITERS row with another value kind, or a FWRITES
    // row no WRITERS row of that line covers).  A use that reaches a live writer is a creator; a use that reaches clearing
    // stores only is a key-clearing call: no creator site and no lookup row
    std::set<uint32_t> writer_fns, live_fns;
    std::map<uint32_t, std::vector<std::string>> clear_stores;  // function -> its clearing stores of the key
    {
      std::map<std::tuple<uint32_t, std::string, uint32_t>, int> at;  // (function, file, line) -> 1 constant stores only, 2 another value kind
      auto wit = writers_of_.find(n.key);
      if (wit != writers_of_.end())
        for (uint32_t i : wit->second) {
          const kz_fact_writer& w = writers_[i];
          int& k = at[std::make_tuple(w.fn, str(w.file), w.line)];
          k = std::max(k, w.value_kind == KZ_ARG_KIND_CONST ? 1 : 2);
          if (w.kind == KZ_WRITER_KIND_MEMSET && (w.flags & KZ_WRITER_FLAG_EARLY_INIT)) continue;
          writer_fns.insert(w.fn);
          if (w.value_kind != KZ_ARG_KIND_CONST) live_fns.insert(w.fn);
          else clear_stores[w.fn].push_back(pos(str(w.file), w.line, w.col) + " in " + fname(w.fn) + " := " + std::to_string(w.value) + " [" + str(w.value_text) + "]");
        }
      auto fit = fwrites_of_.find(n.key);
      if (fit != fwrites_of_.end())
        for (uint32_t i : fit->second) {
          const kz_fact_fwrite& w = fwrites_[i];
          writer_fns.insert(w.fn);
          auto it = at.find(std::make_tuple(w.fn, str(w.file), w.line));
          if (it == at.end() || it->second != 1) live_fns.insert(w.fn);
        }
    }
    struct Use {
      uint32_t ci = kN, callee = kN, param = kN;
      bool creator = false;
      bool clears = false;      // relations rules revision 12: reaches clearing stores of the key only
      std::string clear_text;   // those stores
      std::string part, part_desc, nonsel;
      KeyValue key;
      std::set<uint32_t> actors;
      int member = 0;           // relations rules revision 7: 0 member, 2 membership unresolved (1, another callback: no use here)
      std::string member_text;  // how the use reaches the namespace's comparison
    };
    // relations rules revision 7 (O3): the searches of the namespace, and how each key-parameter function reaches them
    std::vector<Search> all_searches;
    for (const KeyCmp& k : n.cmps)
      for (const Search& sr : searches_of(k)) all_searches.push_back(sr);
    const CbStates cbst = membership_states(n);
    uint32_t other_lookups = 0, other_creators = 0;
    std::vector<std::pair<uint32_t, std::string>> left_out;  // (site, line) of the uses left out for another callback
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
        bool creates = false, writes = false;
        for (uint32_t f : reach_direct(F)) {
          if (writer_fns.count(f) != 0) writes = true;
          if (live_fns.count(f) != 0) creates = true;
        }
        u.creator = creates;
        u.clears = writes && !creates;
        if (u.clears) {
          std::vector<std::string> cs;
          for (uint32_t f : reach_direct(F)) {
            auto ct = clear_stores.find(f);
            if (ct != clear_stores.end()) cs.insert(cs.end(), ct->second.begin(), ct->second.end());
          }
          std::sort(cs.begin(), cs.end());
          cs.erase(std::unique(cs.begin(), cs.end()), cs.end());
          u.clear_text = join(cs, "; ");
        }
        u.member = use_membership(cbst, ci, F, k, &u.member_text);
        if (u.member == 1) {
          // another callback only: the use searches with another comparison function, so it is no use of this namespace
          ++(creates ? other_creators : other_lookups);
          (creates ? n_a_other_creators_ : n_a_other_lookups_)++;
          left_out.emplace_back(c.site, call_pos(ci) + " in " + fname(c.caller_fn) + " -> " + fname(F) + "#" + std::to_string(k) + (creates ? " (creator)" : " (lookup)") + ": " +
                                                    u.member_text);
          continue;
        }
        std::set<std::pair<uint32_t, uint32_t>> seen;
        u.part = partition(F, k, &seen, n.key_params, all_searches, &u.part_desc, &u.nonsel);
        u.key = key_value(c.site, k);
        u.actors = actor_set(c.caller_fn);
        uses.push_back(u);
      }
    }
    std::sort(uses.begin(), uses.end(), [&](const Use& a, const Use& b) { return calls_[a.ci].site < calls_[b.ci].site; });
    // the key-parameter functions, as lookups or creators; for a creator, the writes of the key and the
    // active fields in the functions it reaches through direct calls (its publication writes)
    {
      std::string lk, cr, kc;
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
        bool creates = false, writes = false;
        const std::set<uint32_t> reach = reach_direct(F);
        for (uint32_t f : reach) {
          if (writer_fns.count(f) != 0) writes = true;
          if (live_fns.count(f) != 0) creates = true;
        }
        const std::string name = fname(F) + "#" + std::to_string(k);
        if (!writes) {
          lk += " " + name;
          continue;
        }
        if (!creates) {  // relations rules revision 12: reaches clearing stores of the key only
          kc += " " + name;
          continue;
        }
        cr += " " + name;
        std::string pub;
        for (const WriterRow& w : all)
          if (reach.count(w.fn) != 0)
            pub += std::string(pub.empty() ? "" : "; ") + w.kind + " " + pos(w.file, w.line, w.col) + " in " + fname(w.fn) + (w.known ? " := " + std::to_string(w.value) : "");
        if (!pub.empty()) rows << "      publication writes of the creator " << name << ": " << pub << "\n";
      }
      rows << "      key-parameter functions: lookups" << (lk.empty() ? " none" : lk) << "; creators" << (cr.empty() ? " none" : cr)
           << (kc.empty() ? std::string() : "; key-clearing (they reach constant stores of the key only)" + kc) << "\n";
    }
    uint32_t lookups = 0, creators = 0, clearing = 0;
    bool ns_l0 = false;
    for (const Use& u : uses) {
      (u.creator ? creators : u.clears ? clearing : lookups)++;
      if (uses_script_root(u.actors)) ns_l0 = true;
    }
    n_a_clearing_ += clearing;
    cur_ns_clearing_ = clearing;
    rows << "      uses: lookups " << lookups << ", creators " << creators << " (calls from outside the key-parameter functions; a creator reaches a write of the key)";
    if (clearing != 0)
      rows << "; key-clearing calls " << clearing << " (every write of the key they reach stores a constant, WRITERS: no creator site and no lookup row)";
    // relations rules revision 7 (O3): a use whose every chain passes another comparison function to the search is no use here
    rows << "; left out, another callback passed to the search: lookups " << other_lookups << ", creators " << other_creators << "\n";
    std::sort(left_out.begin(), left_out.end());
    for (const auto& [site, l] : left_out) rows << "      left out: " << l << "\n";
    rows << "      namespace layers: " << layers_text(true, ns_l0) << "\n";
    for (const Use& u : uses) {
      if (u.creator || u.clears) continue;
      begin_row();
      // the comparison functions of P0 the namespace's searches use
      for (const KeyCmp& k : n.cmps)
        if (k.site != kN) {
          auto kc = call_by_site_.find(k.site);
          if (kc != call_by_site_.end()) note_p0(str(calls_[kc->second].callee));
        }
      ++n_a_;
      const kz_fact_call& c = calls_[u.ci];
      RelRow r;
      r.path = 'A';
      r.number = n_a_;
      r.subject = n.key;
      r.call = u.ci;
      r.caller_fn = c.caller_fn;
      r.callee_fn = u.callee;
      r.param = u.param;
      RowLines L(&r);
      {
        std::ostringstream h;
        h << "lookup " << call_pos(u.ci) << " in " << fname(c.caller_fn) << " -> " << fname(u.callee) << "#" << u.param << " namespace " << n.key << " partition {" << u.part
          << "} key " << u.key.status;
        r.head = h.str();
      }
      // relations rules revision 7 (O3): the partition takes the table selectors only; the other constants are named
      if (!u.part_desc.empty() || !u.nonsel.empty())
        L.line("partition") << "partition: " << (u.part_desc.empty() ? std::string("no table selector") : u.part_desc) << (u.nonsel.empty() ? "" : "; not a table selector: " + u.nonsel);
      L.line("membership") << "membership: " << (u.member == 0 ? "" : "unresolved: ") << u.member_text;
      L.line("key") << "key: " << u.key.text;
      if (!u.key.alias.empty()) L.line("aliases") << "aliases: the key is read through the pointer field " << u.key.alias;
      L.line("actors") << "actors: " << actors_text(u.actors) << "; result=" << result_use(u.ci);
      for (const std::string& h : id_holders(u.ci, u.callee, u.param)) L.line("id holder") << "id holder: the lookup's " << h;
      std::vector<std::string> paired, excluded, probes;
      std::vector<const Use*> named;  // the creators the row pairs, and its probe-creates (relations rules revision 7)
      bool creator_member_unres = false;
      bool any_inter = false, any_match = false, any_unres = false, creator_unres = false, creator_cut = false;
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
        // a probe-create is no pair only when program order fixes it: one actor runs both (ED §3.5, the W(u) rule of a
        // write of the same actor); run by several actors, the lookup of one and the creation of another stay a pair
        // (relations rules revision 5)
        if (probe && same_actor_only) {
          named.push_back(&w);
          ++n_probe;
          excluded.push_back("probe-create (same function, the same key argument, the creator's key read under the lookup's result; " + polarity + "): " + line);
          probes.push_back(call_pos(w.ci) + " -> " + fname(w.callee) + " (" + polarity + ")");
          continue;
        }
        if (probe)
          probes.push_back(call_pos(w.ci) + " -> " + fname(w.callee) + " (" + polarity + "; the function is run by " + std::to_string(u.actors.size()) +
                           " actor(s) " + actors_text(u.actors) + " and the creation by " + actors_text(w.actors) +
                           ": kept as a pair, a creation by another actor may come between this lookup and that actor's own creation)");
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
        if (w.key.status != "known") creator_unres = true;
        if (w.key.cut) creator_cut = true;
        if (w.member == 2) creator_member_unres = true;
        named.push_back(&w);
        // the creator's position stays the first of the line (the plan generator reads it so): the membership note goes last
        paired.push_back(keyrel + ": " + line + (w.member == 2 ? "; namespace membership unresolved: " + w.member_text : ""));
      }
      if (any_inter) ++n_a_paired_;
      if (any_match) ++n_a_keymatch_;
      for (const std::string& p : paired) L.line("creator") << "creator: " << p;
      for (const std::string& x : excluded) L.line("not a pair") << "not a pair: " << x;
      // relations rules revision 12: the key-clearing calls of the row's partition (a delete or a reset of the key: the
      // plan generator reads the position as the first of the line)
      for (const Use& w : uses)
        if (w.clears && w.part == u.part)
          L.line("key clearing") << "key clearing: " << call_pos(w.ci) << " in " << fname(calls_[w.ci].caller_fn) << " -> " << fname(w.callee) << " key " << w.key.text << " actors "
                                 << actors_text(w.actors) << "; no creator: every write of the key it reaches stores a constant (" << w.clear_text << ")";
      // relations rules revision 7 (O5): the out-pointer arguments and the resolved pointer fields of the creators named
      // above; the constants their callees may return (a creator's out is read under its callee's success constant)
      std::set<uint32_t> creator_callees;
      std::set<std::string> key_fields(n.active.begin(), n.active.end());
      key_fields.insert(n.key);
      if (!u.key.image_field.empty()) key_fields.insert(u.key.image_field);
      for (const Use* w : named) {
        const std::string at = call_pos(w->ci) + " -> " + fname(w->callee);
        for (const std::string& h : id_holders(w->ci, w->callee, w->param)) L.line("id holder") << "id holder: the creator " << at << ": " << h;
        if (!w->key.alias.empty()) L.line("aliases") << "aliases: the creator " << at << ": the key is read through the pointer field " << w->key.alias;
        if (!w->key.image_field.empty()) key_fields.insert(w->key.image_field);
        creator_callees.insert(w->callee);
      }
      for (uint32_t F : creator_callees) {
        std::vector<std::string> open;
        size_t marked = 0;
        for (const std::string& t : callee_returns(F, &open, &marked)) L.line("callee-check") << "callee-check: " << t;
        if (!open.empty()) L.line("callee-check (returns not followed)") << "callee-check (returns not followed): " << fname(F) << ": " << join(open, "; ");
        // relations rules revision 9: whether the constants listed are all the callee returns and each reaches the return
        if (open.empty() && marked == 0)
          L.line("callee-check (set)") << "callee-check (set): " << fname(F) << ": complete under these facts (every return is a constant or the result of a followed call, none through a definition redefined later)";
        else
          L.line("callee-check (set)") << "callee-check (set): " << fname(F) << ": not established (" << open.size() << " returns not followed, " << marked
                                       << " constants not shown to reach the return): the constants listed are not all it returns, or not shown; no success constant is taken from them";
        if (result_flow_) creator_result_flow(L, F);
      }
      // relations rules revision 7 (O4): the writer sets the row uses -- the key, the active fields, and the fields the keys
      // are read from in a static image -- with their rule statuses, premises and what they do not cover
      bool zero_writers = false;
      for (const std::string& f : key_fields) {
        bool z = false;
        L.line("writers") << writer_premises(f, &z);
        if (z) zero_writers = true;
      }
      // handling of the result
      L.done();
      add_lines(&r, a_handling(u.ci), "handling");
      for (const std::string& pc : probes) L.line("handling") << "handling: probe-create: on the lookup's failure result the same function creates the same key: " << pc;
      L.done();
      if (result_flow_) {
        std::string nm_via;
        const std::set<int64_t> nm = nomatch_values(n, calls_[u.ci].callee_fn, nullptr, &nm_via);
        std::set<int64_t> absence;
        for (int64_t v : nm)
          if (rf_fail(v, calls_[u.ci].callee_fn)) absence.insert(v);
        a_result_flow(L, u.ci, absence, nm_via, !probes.empty());
        L.done();
      }
      std::string dep;
      bool unread_writes = false;
      if (!paired.empty()) {
        dep = std::to_string(paired.size()) + " creator(s) of this partition in other actors";
      } else {
        // relations rules revision 7 (I3): why no creator of the partition is left
        std::set<std::string> other_parts;
        for (const Use& w : uses)
          if (w.creator && w.part != u.part) other_parts.insert("{" + w.part + "}");
        std::vector<std::string> why = no_creator_reasons(n, other_parts, live_fns, creators, u.member == 2, &unread_writes);  // revision 12: the live writers
        if (n_probe + n_differ + n_same_actor == 0) dep = "no creator of this partition";
        else
          dep = "no creator left after exclusions (probe-create " + std::to_string(n_probe) + ", key constants differ " + std::to_string(n_differ) + ", same actor only " +
                std::to_string(n_same_actor) + ")";
        dep += ": " + join(why, "; ");
        ++n_a_nocreator_;
        for (const std::string& w : why) ++no_creator_why_[w.substr(0, w.find(" ("))];
      }
      r.dependency = "path A: " + dep;
      r.run_order = "no record given";
      std::string feas;
      if (paired.empty()) {
        feas = "undecidable: no pair";
      } else {
        feas = "undecidable: the facts give no static order between the lookup's actor and the creators' actors (creation order does not fix the order of initialization)";
      }
      tally_feas("undecidable");
      r.feasibility = feas;
      r.feasibility_grade = last_feas_grade_;
      r.violation = violation_text();
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
      r.flip_grade = grade;
      r.flip = grade + ": " + why;
      bool l0 = uses_script_root(u.actors);
      r.layers = layers_text(true, l0);
      // relations rules revision 6: the incompleteness reasons of the lookup row
      if (u.key.status != "known" && u.key.why != kIncCount) inc(u.key.why);
      if (creator_unres) inc(kKeyCreator);
      if (u.key.cut || creator_cut) inc(kTruncImage);
      if (u.actors.empty()) inc(kActorNoRoot);
      inc_writers(n.key);
      for (const std::string& f : n.active) inc_writers(f);
      if (fails.empty()) inc(kAbsentSearch);
      if (!paired.empty()) inc(kFixedNoOrder);
      // relations rules revision 7
      if (zero_writers) inc(kWriterZero);
      if (unread_writes) inc(kWriterUnread);
      if (u.member == 2 || creator_member_unres) inc(kLinkMembership);
      if (u.member == 2) ++n_a_member_unres_;
      finish(&r);
      key_a(&r);
      out->push_back(std::move(r));
    }
  }
  // relations rules revision 7 (I3): the sub-reasons of "no creator" for a lookup: creators of other partitions only; the
  // key written, but by no function a key-parameter function reaches (the writer kinds and functions); the writer set of
  // the key or an active field not complete; a function that writes the key reachable through an indirect call whose
  // targets are not resolved; zero writers of the key under the rules; the lookup's namespace membership unresolved.
  std::vector<std::string> no_creator_reasons(const Namespace& n, const std::set<std::string>& other_parts, const std::set<uint32_t>& writer_fns, uint32_t creators,
                                              bool member_unres, bool* unread) {
    std::vector<std::string> why;
    *unread = false;
    if (!other_parts.empty()) why.push_back("creators of other partitions only (" + join(std::vector<std::string>(other_parts.begin(), other_parts.end()), " ") + ")");
    std::string src;
    const std::vector<WriterRow> ws = writers_of(n.key, &src);
    if (!ws.empty() && creators == 0) {
      std::set<std::string> kf;
      for (const WriterRow& w : ws) kf.insert(w.kind + " in " + fname(w.fn));
      // relations rules revision 12: a key-parameter function may reach a writer whose stores of the key are all constants
      const bool reaches_writer = cur_ns_clearing_ != 0;  // the namespace has key-clearing calls
      why.push_back(std::string(reaches_writer ? "key written under a key parameter by clearing stores only (every write of " : "key written without a key parameter (no key-parameter function reaches a writer of ") +
                    n.key + (reaches_writer ? " the namespace's key-clearing calls reach stores a constant; the writers: " : ": ") + join(std::vector<std::string>(kf.begin(), kf.end()), ", ") + ")");
    }
    std::set<std::string> fields(n.active.begin(), n.active.end());
    fields.insert(n.key);
    std::set<std::string> open;
    for (const std::string& f : fields) {
      auto fsit = fstatus_of_.find(f);
      if (fsit == fstatus_of_.end()) open.insert(f + " no FIELD_STATUS row");
      else if (fstatus_[fsit->second].status_field != KZ_LOC_STATUS_COMPLETE || fstatus_[fsit->second].status_type != KZ_LOC_STATUS_COMPLETE) open.insert(f + " not complete");
    }
    if (!open.empty()) why.push_back("writer set not complete (" + join(std::vector<std::string>(open.begin(), open.end()), ", ") + ")");
    if (!unresolved_sites_.empty()) {
      std::set<std::string> reach;
      for (uint32_t f : writer_fns)
        if (at_reach(f)) reach.insert(fname(f));
      if (!reach.empty()) {
        *unread = true;
        why.push_back("creators may be in code not read (" + std::to_string(unresolved_sites_.size()) + " indirect calls with unresolved targets may run " +
                      join(std::vector<std::string>(reach.begin(), reach.end()), ", ") + ", which write the key)");
      }
    }
    if (ws.empty()) why.push_back("only initial values (no writer of " + n.key + " found under the premises of its writers line)");
    if (member_unres) why.push_back("namespace membership unresolved (the lookup's callback)");
    return why;
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
  // ---------------------------------------------------------------- relations rules revision 10 (result flow)
  // Only for facts with RESULT_FLOW and RESULT_EFFECT (extract --result-flow, static_facts.md 5u); without them nothing
  // below runs and the rows are those of revision 9.  The handling label of a row (judgement rules 3.4: a processing
  // fact, never a verdict) is read from the RESULT_EFFECT rows of the row's call for the failure value it follows (the
  // absence value of a lookup, the callee-check failure values of a field argument), and through the returns of failure
  // class up to the callers' calls (a fixed point over the call graph with a visited set; no numeric bound).
  //   R   a terminal or an application exit reached on a decided path, or a failure value returned up to a root;
  //   H   the call runs again on that value (a retry), or a probe-create;
  //   T?  an effect that is open (a branch the value does not decide, a value not enumerated, an exit after a call that
  //       may change the stored status, a result neither tested nor returned), or an event send only: the semantics data
  //       give an event type's domain constants, not their rank, so an event send alone does not decide R;
  //   J   the failure value reaches a call whose result is discarded (J? when the absence value is not evaluated);
  //   T   nothing but returns of values not of failure class, stores and the join (checked and continued).
  static constexpr int kLabR = 1, kLabH = 2, kLabOpen = 4, kLabEvent = 8, kLabJ = 16, kLabT = 32;
  struct Lab {
    int f = 0;
    std::vector<std::string> why;
    void add(int flag, const std::string& w) {
      f |= flag;
      if (why.size() < 64) why.push_back(w);
    }
  };
  static bool rf_same(int64_t a, int64_t b) {
    if (a == b) return true;
    auto fits = [](int64_t x) { return x >= -2147483648LL && x <= 4294967295LL; };
    return fits(a) && fits(b) && (static_cast<uint64_t>(a) & 0xFFFFFFFFu) == (static_cast<uint64_t>(b) & 0xFFFFFFFFu);
  }
  // the outcome class rule (outcome.hpp, static_facts.md 5w) in the status space of the function `fn` (kN: a function
  // without a semantics entry), read from the SEMANTICS rows of the facts
  const OutcomeSpaces& spaces() const {
    if (!spaces_built_) {
      for (const kz_fact_sem& r : sem_) spaces_.add(r.layer, r.column, str(r.component), r.fn, str(r.item), r.value);
      spaces_built_ = true;
    }
    return spaces_;
  }
  bool rf_fail(int64_t v, uint32_t fn = kN) const { return spaces().cls(fn, v) == KZ_REFF_OUTCOME_FAILURE; }
  mutable OutcomeSpaces spaces_;
  mutable bool spaces_built_ = false;
  std::string reff_pos(const kz_fact_reffect& e) const {
    if (e.effect_site != kN) {
      auto it = call_by_site_.find(e.effect_site);
      if (it != call_by_site_.end()) return call_pos(it->second);
    }
    // an effect without a call is in the function of the call (a return or a store of that body)
    if (e.effect_line != 0 && e.fn != kN && e.fn < funcs_.size()) return pos(str(funcs_[e.fn].file), e.effect_line, 0);
    return "-";
  }
  // the rows of the value v at a call site (the complement rows when v is not enumerated there)
  std::vector<uint32_t> reff_rows(uint32_t site, int64_t v, bool* complement) const {
    std::vector<uint32_t> out, comp;
    *complement = false;
    auto it = reffect_of_site_.find(site);
    if (it == reffect_of_site_.end()) return out;
    for (uint32_t i : it->second) {
      const kz_fact_reffect& e = reffect_[i];
      if (e.reach == KZ_REFF_REACH_NONE) comp.push_back(i);
      else if (rf_same(e.outcome_value, v)) out.push_back(i);
    }
    if (out.empty() && !comp.empty()) {
      *complement = true;
      return comp;
    }
    return out;
  }
  void lab_of(uint32_t site, int64_t v, std::set<std::pair<uint32_t, int64_t>>* seen, Lab* out) const {
    if (!seen->insert({site, v}).second) return;
    auto cit = call_by_site_.find(site);
    const std::string at = cit != call_by_site_.end() ? call_pos(cit->second) : "site " + std::to_string(site);
    if (cit != call_by_site_.end() && result_use(cit->second) == "discarded") {
      out->add(kLabJ, "J: the result of the call at " + at + " is discarded");
      return;
    }
    bool comp = false;
    const std::vector<uint32_t> rows = reff_rows(site, v, &comp);
    if (rows.empty()) {
      out->add(kLabOpen, "T?: no RESULT_EFFECT row for the value " + std::to_string(v) + " at " + at + " (the result is neither tested nor returned in its function)");
      return;
    }
    if (comp) out->add(kLabOpen, "T?: the value " + std::to_string(v) + " is not among the values enumerated at " + at);
    for (uint32_t i : rows) {
      const kz_fact_reffect& e = reffect_[i];
      const bool complete = e.status == KZ_REFF_STATUS_COMPLETE;
      const std::string where = reff_pos(e), d = str(e.detail);
      switch (e.effect) {
        case KZ_REFF_EFFECT_TERMINAL:
        case KZ_REFF_EFFECT_EXIT_APP:
          if (complete) out->add(kLabR, "R: " + enum_text("KZ_REFF_EFFECT", e.effect) + " at " + where + " [" + d + "] (" + str(e.chain) + ")");
          else out->add(kLabOpen, "T?: " + enum_text("KZ_REFF_EFFECT", e.effect) + " at " + where + " is open: " + str(e.reason));
          break;
        case KZ_REFF_EFFECT_RETRY:
          // a recovery action in the code (judgement rules R05): also on a path through a branch the value does not decide
          // (a retry loop's own bound is such a branch)
          out->add(kLabH, "H: " + d + " (" + str(e.chain) + (complete ? "" : "; " + str(e.reason)) + ")");
          break;
        case KZ_REFF_EFFECT_EVENT_SEND:
          out->add(kLabEvent | (complete ? 0 : kLabOpen), "event: " + d + " at " + where + (complete ? "" : " (open: " + str(e.reason) + ")"));
          break;
        case KZ_REFF_EFFECT_RETURN_RESULT:
        case KZ_REFF_EFFECT_RETURN_CONST: {
          const int64_t c = e.effect == KZ_REFF_EFFECT_RETURN_CONST ? e.effect_value : v;
          if (!complete) out->add(kLabOpen, "T?: the return at " + where + " is on a path that is open: " + str(e.reason));
          if (!rf_fail(c, e.fn)) {
            out->add(kLabT, "T: " + fname(e.fn) + " returns " + std::to_string(c) + " at " + where + " (not of failure class)");
            break;
          }
          const uint32_t g = e.fn;
          const bool direct = g < callers_.size() && !callers_[g].empty();
          const bool indirect = indirect_callers_.count(g) != 0;
          if (!direct && !indirect) {
            // up to the root of an actor: the failure is what that thread's entry returns
            if (root_kind_.count(g) != 0) out->add(complete ? kLabR : kLabOpen, std::string(complete ? "R" : "T?") + ": the failure value " + std::to_string(c) + " is returned up to the root " + fname(g) + " (" + root_kind_.at(g) + ") at " + where);
            else out->add(kLabOpen, "T?: the failure value " + std::to_string(c) + " is returned by " + fname(g) + ", which no call of these facts calls and which is no root (at " + where + ")");
            break;
          }
          if (indirect) out->add(kLabOpen, "T?: " + fname(g) + " is also called through a function pointer; those callers are not followed");
          if (direct)
            for (uint32_t x : callers_[g]) lab_of(calls_[x].site, c, seen, out);
          break;
        }
        case KZ_REFF_EFFECT_STORE:
        case KZ_REFF_EFFECT_FALLTHROUGH:
          if (complete) out->add(kLabT, "T: " + enum_text("KZ_REFF_EFFECT", e.effect) + (d.empty() ? std::string() : " [" + d + "]") + " at " + where);
          else out->add(kLabOpen, "T?: " + enum_text("KZ_REFF_EFFECT", e.effect) + " at " + where + " is open: " + str(e.reason));
          break;
        default:
          out->add(kLabOpen, "T?: " + (str(e.reason).empty() ? std::string("an effect not followed") : str(e.reason)) + " at " + where);
      }
    }
  }
  static std::string lab_text(const Lab& l) {
    if (l.f & kLabR) return "R";
    if (l.f & kLabH) return "H";
    if (l.f & (kLabOpen | kLabEvent)) return "T?";
    if (l.f & kLabJ) return "J";
    return "T";
  }
  // the reasons a label names: those of the deciding flag first
  static std::string lab_why(const Lab& l, const std::string& label) {
    std::vector<std::string> first, rest;
    const std::string pre = label == "T?" ? "T?:" : label + ":";
    for (const std::string& w : l.why) (w.compare(0, pre.size(), pre) == 0 || (label == "T?" && w.compare(0, 6, "event:") == 0) ? first : rest).push_back(w);
    if (label == "T?" && (l.f & kLabEvent) && !(l.f & kLabOpen)) first.insert(first.begin(), "T?: an event send only (the semantics data give the event type's domain constants, not their rank)");
    std::string t;
    for (size_t i = 0; i < first.size() && i < 3; ++i) t += (i == 0 ? "" : "; ") + first[i];
    if (first.size() > 3) t += "; and " + std::to_string(first.size() - 3) + " more";
    return t;
  }
  // the effects of the value at the call itself (not composed), for the row
  std::string effects_text(uint32_t site, int64_t v) const {
    bool comp = false;
    const std::vector<uint32_t> rows = reff_rows(site, v, &comp);
    std::vector<std::string> out;
    for (uint32_t i : rows) {
      const kz_fact_reffect& e = reffect_[i];
      const std::string d = str(e.detail);
      std::string t = enum_text("KZ_REFF_EFFECT", e.effect);
      if (e.effect == KZ_REFF_EFFECT_EVENT_SEND || e.effect == KZ_REFF_EFFECT_EXIT_APP || e.effect == KZ_REFF_EFFECT_TERMINAL || e.effect == KZ_REFF_EFFECT_STORE) t += " [" + d + "]";
      if (e.effect == KZ_REFF_EFFECT_RETURN_CONST) t += " " + std::to_string(e.effect_value);
      t += " at " + reff_pos(e) + " " + enum_text("KZ_REFF_STATUS", e.status);
      out.push_back(t);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    if (out.empty()) return "none";
    std::string t = (comp ? "(the value is not enumerated; the rows of the other values) " : "");
    for (size_t i = 0; i < out.size() && i < 8; ++i) t += (i == 0 ? "" : "; ") + out[i];
    if (out.size() > 8) t += "; and " + std::to_string(out.size() - 8) + " more";
    return t;
  }
  // a path A row: the absence line and the handling label (the absence values of the lookup, else its failure values)
  void a_result_flow(RowLines& L, uint32_t ci, const std::set<int64_t>& absence, const std::string& via, bool probe) {
    const kz_fact_call& c = calls_[ci];
    std::set<int64_t> values = absence;
    std::string over = "the absence value";
    if (absence.empty()) {
      auto it = reffect_of_site_.find(c.site);
      if (it != reffect_of_site_.end())
        for (uint32_t i : it->second)
          if (reffect_[i].reach != KZ_REFF_REACH_NONE && reffect_[i].outcome == KZ_REFF_OUTCOME_FAILURE) values.insert(reffect_[i].outcome_value);
      over = "the failure values (the absence value is not evaluated)";
    }
    if (absence.empty()) L.line("absence") << "absence: unevaluated (the lookup's no-match value is not established in these facts)";
    else
      for (int64_t v : absence) {
        bool comp = false;
        const std::vector<uint32_t> rows = reff_rows(c.site, v, &comp);
        std::string reach = comp || rows.empty() ? "not enumerated at the call" : enum_text("KZ_REFF_REACH", reffect_[rows.front()].reach);
        L.line("absence") << "absence: " << v << " (no match: " << via << ") at the call " << call_pos(ci) << ": " << reach << "; effects: " << effects_text(c.site, v);
      }
    std::string label, why;
    if (probe) {
      label = "H";
      why = "H: a probe-create on the lookup's failure result";
    } else if (result_use(ci) == "discarded") {
      label = absence.empty() ? "J?" : "J";
      why = std::string(label) + ": the result is discarded" + (absence.empty() ? " and the absence value is not evaluated" : "");
    } else if (values.empty()) {
      L.line("handling_label") << "handling_label: none (no failure value to follow at " << call_pos(ci) << ")";
      ++n_lab_["none"];
      return;
    } else {
      Lab l;
      std::set<std::pair<uint32_t, int64_t>> seen;
      for (int64_t v : values) lab_of(c.site, v, &seen, &l);
      label = lab_text(l);
      why = lab_why(l, label);
    }
    std::string vs;
    for (int64_t v : values) vs += (vs.empty() ? "" : ", ") + std::to_string(v);
    L.line("handling_label") << "handling_label: " << label << " (over " << over << (vs.empty() ? "" : " " + vs) << ") " << why;
    ++n_lab_[label];
  }
  // a path B row: the label of each site over the callee-check failure values
  void b_result_flow(RowLines& L, const std::vector<uint32_t>& sites, const std::vector<Check>& checks) {
    std::set<int64_t> values;
    for (const Check& ck : checks)
      if (rf_fail(ck.value)) values.insert(ck.value);
    if (values.empty()) {
      L.line("handling_label") << "handling_label: none (" << (checks.empty() ? "no callee check" : "no callee-check value of failure class") << ")";
      ++n_lab_["none"];
      return;
    }
    std::map<std::string, uint32_t> per;
    std::string first_why;
    std::string best;
    for (uint32_t ci : sites) {
      Lab l;
      std::set<std::pair<uint32_t, int64_t>> seen;
      for (int64_t v : values) lab_of(calls_[ci].site, v, &seen, &l);
      const std::string lb = lab_text(l);
      ++per[lb];
      if (first_why.empty() || lb == "R") first_why = call_pos(ci) + ": " + lab_why(l, lb);
    }
    for (const char* k : {"R", "H", "T?", "J", "T"})
      if (best.empty() && per.count(k) != 0) best = k;
    std::string ps;
    for (const auto& [k, n] : per) ps += (ps.empty() ? "" : ", ") + k + " " + std::to_string(n);
    std::string vs;
    for (int64_t v : values) vs += (vs.empty() ? "" : ", ") + std::to_string(v);
    L.line("handling_label") << "handling_label: " << best << " (sites: " << ps << "; over the callee-check failure values " << vs << ") " << first_why;
    ++n_lab_[best];
  }
  // a creator's callee: the values the RESULT_EFFECT rows of its calls show to reach its returns, and its success constants
  void creator_result_flow(RowLines& L, uint32_t F) const {
    std::map<int64_t, std::pair<std::string, bool>> vals;  // value -> (first text, shown: complete and reached)
    for (const kz_fact_reffect& e : reffect_) {
      if (e.fn != F || (e.effect != KZ_REFF_EFFECT_RETURN_RESULT && e.effect != KZ_REFF_EFFECT_RETURN_CONST)) continue;
      if (e.effect == KZ_REFF_EFFECT_RETURN_RESULT && e.reach == KZ_REFF_REACH_NONE) continue;
      const int64_t v = e.effect == KZ_REFF_EFFECT_RETURN_CONST ? e.effect_value : e.outcome_value;
      const bool shown = e.status == KZ_REFF_STATUS_COMPLETE && (e.effect == KZ_REFF_EFFECT_RETURN_CONST || e.reach == KZ_REFF_REACH_REACHED);
      auto cit = call_by_site_.find(e.site);
      const std::string t = (e.effect == KZ_REFF_EFFECT_RETURN_CONST ? str(e.detail) + " (a constant returned at " + reff_pos(e) + ")"
                                                                     : "the value of " + (cit != call_by_site_.end() ? str(calls_[cit->second].callee) + " at " + call_pos(cit->second) : std::string("a call")) +
                                                                           " reaches the return at " + reff_pos(e) + " [" + str(e.detail) + "]") +
                            (shown ? "" : " (" + std::string(e.status == KZ_REFF_STATUS_COMPLETE ? "present-only" : "open") + ")");
      auto it = vals.find(v);
      if (it == vals.end()) vals[v] = {t, shown};
      else if (shown && !it->second.second) it->second = {t, true};
    }
    std::vector<std::string> succ;
    for (const auto& [v, p] : vals) {
      L.line("callee-check (result flow)") << "callee-check (result flow): " << fname(F) << " returns " << v << ": " << p.first;
      if (p.second && spaces().cls(F, v) == KZ_REFF_OUTCOME_SUCCESS) succ.push_back(std::to_string(v));
    }
    L.line("callee-check (result flow, success)") << "callee-check (result flow, success): " << fname(F) << ": "
                                                  << (succ.empty() ? std::string("no value of success class is shown to reach the return") : "shown to reach the return: " + join(succ, ", "))
                                                  << " (the values RESULT_EFFECT shows to reach a return of " << fname(F) << ", not all it returns: callee-check (set) says whether that set is complete)";
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
  std::set<uint32_t> at_reach_;                           // the functions an address-taken function may run (call_runs, transitively)
  bool at_reach_done_ = false;
  std::map<uint32_t, std::set<uint32_t>> closure_;
  std::map<uint32_t, ChildSet> child_sets_;  // spawn site -> its child set
  uint32_t n_c_incomplete_ctx_ = 0, n_c_descendant_ctx_ = 0, n_c_callback_ctx_ = 0, n_c_unresolved_ctx_ = 0, n_c_open_reach_ctx_ = 0, n_c_open_reach_reads_ = 0;
  mutable std::set<std::string> row_files_, row_tags_, row_p0_;
  mutable std::set<uint32_t> row_fns_;                       // the functions the row names (facts with SEMANTICS only)
  std::map<uint32_t, uint32_t> sem_items_;                   // function -> its framework SEMANTICS rows
  std::map<uint32_t, std::set<std::string>> sem_unverified_;  // function -> "api:column" of its rows not verified
  uint32_t n_sem_rows_ = 0, n_sem_unverified_rows_ = 0;
  uint32_t n_handle_rows_ = 0, n_handle_static_ = 0;  // revision 11: path C rows whose common lock has one handle
  std::map<std::string, uint32_t> n_lab_;  // relations rules revision 10: rows per handling label
  mutable std::set<uint32_t> row_inc_;          // the incompleteness reasons of the row being written (Inc)
  std::set<uint32_t> global_inc_;               // the reasons of the whole facts (global_reasons)
  std::map<uint32_t, uint32_t> inc_rows_;       // reason (Inc) -> rows that carry it
  std::map<std::string, uint32_t> inc_reason_rows_;  // reason of the vocabulary -> rows that carry it with any sub-reason
  std::string last_feas_grade_;  // the grade tally_feas counted last (the row's feasibility grade)
  std::map<std::string, std::set<int64_t>> in_use_, empty_;
  uint32_t n_a_clearing_ = 0;  // relations rules revision 12: key-clearing calls (no creator sites)
  uint32_t cur_ns_clearing_ = 0;  // those of the namespace whose rows are being written
  uint32_t n_a_other_lookups_ = 0, n_a_other_creators_ = 0, n_a_member_unres_ = 0, n_a_nocreator_ = 0;  // relations rules revision 7: uses left out, another callback
  std::map<std::string, uint32_t> no_creator_why_;            // relations rules revision 7: lookup rows per no-creator sub-reason
  uint32_t n_a_ = 0, n_ns_ = 0, n_a_paired_ = 0, n_a_keymatch_ = 0, n_b_ = 0, n_b_check_ = 0, n_c_ = 0, n_c_self_ = 0;
  uint32_t n_flip_possible_ = 0, n_flip_undet_ = 0, n_flip_none_ = 0, n_feas_impossible_ = 0, n_feas_possible_ = 0, n_feas_undecidable_ = 0, n_feas_not_graded_ = 0;
};

}  // namespace

const std::vector<RelReason>& relations_incomplete_reasons() {
  static const std::vector<RelReason> table = {
      {"no_run", "no run record read", "every row: this revision reads no run record"},
      {"actor_not_extracted", "spawn call with no resolved entry", "every row, when the facts have a spawn call with no resolved entry, or a call of a derived spawn operation a trampoline stands for with no resolved entry: the code their threads run is not known, so an actor set may miss them"},
      {"actor_not_extracted", "spawn entries from a cut static image", "every row, when a spawn's entries are read from a static image cut at the image budget: the entries of the part not read are no roots"},
      {"actor_not_extracted", "no root reaches the use", "A: no root reaches the lookup; B: no root reaches the readers or the writers"},
      {"actor_not_extracted", "child threads not all resolved", "C: the set of the threads the spawn starts is not complete (ChildSet)"},
      {"key_unresolved", "no argument flow row", "A: the key argument has no ARGFLOW row"},
      {"key_unresolved", "pointer field without points-to", "A: the key is read through a pointer field PTR_FIELDS does not resolve"},
      {"key_unresolved", "static image truncated", "A: the key's static image was cut at the image budget"},
      {"key_unresolved", "static image written at run time", "A: a leaf of the key's static image is marked object-, field- or record-written"},
      {"key_unresolved", "static image not the run-time value", "A: the key's image is not taken as its value: no FIELD_STATUS row, a writer set not complete, or writers at run time"},
      {"key_unresolved", "no image leaf of the field", "A: the static image reached has no leaf of the key field"},
      {"key_unresolved", "field read without a static image", "A: the key is a field read with a base whose static image the facts do not reach"},
      {"key_unresolved", "argument is no literal, constant or field", "A: the key argument is a local, a parameter, a call result, a global or another source"},
      {"key_unresolved", "creator key", "A: the key of a creator the row pairs is not resolved"},
      {"truncated", "static image", "A: the key's image, the lookup's or a paired creator's, was cut at the image budget; C: the spawn's threads include entries from a cut image"},
      {"truncated", "FACT_INIT walk bound", "every row, when BASIS count.walk_bounds_hit counts a FACT_INIT walk that stopped"},
      {"truncated", "function-value initializer nesting bound", "every row, when BASIS count.walk_bounds_hit counts a function-value initializer cut"},
      {"truncated", "expression depth bound", "every row, when BASIS count.walk_bounds_hit counts a subexpression cut"},
      {"truncated", "DERIVES copy chain bound", "every row, when BASIS count.walk_bounds_hit counts a DERIVES chain cut"},
      {"truncated", "LOOP_ITERS text bound", "every row, when BASIS count.walk_bounds_hit counts a LOOP_ITERS text cut"},
      {"writer_set_incomplete", "FIELD_STATUS not complete", "A: the key or an active field, B and C: the field, whose FIELD_STATUS row is not complete under both rules"},
      {"writer_set_incomplete", "no FIELD_STATUS row", "A: the key or an active field, B and C: the field, with no FIELD_STATUS row"},
      {"writer_set_incomplete", "per-field completeness rule is a premise", "a row whose grade rests on the per-field completeness rule (rests-on in its layers)"},
      {"writer_set_incomplete", "zero writers under the rules", "A: the key, an active field or the field a key is read from in a static image, B and C: the field, with no writer found: zero writers is no completeness verdict (relations rules revision 7)"},
      {"writer_set_incomplete", "key writes reachable through unresolved indirect calls", "A: no creator of the partition, and a function that writes the key may run through an indirect call whose targets are not resolved (relations rules revision 7)"},
      {"absent_result_unevaluated", "no failure return of the search", "A: the searches of the namespace have no constant return (RET_CONSTS)"},
      {"absent_result_unevaluated", "callee check not evaluated", "B: the flip column is static-undetermined"},
      {"absent_result_unevaluated", "reader's returns not evaluated", "C: the flip column is static-undetermined"},
      {"fixed_proof_open", "no static order between the actors", "A: a creator is paired and no static order is derived; B: neither a creation order nor a static image relates the actors"},
      {"fixed_proof_open", "creation order without an order of the use", "B: a creation order (and an await) is derived, but not the order of the use and the awaited state"},
      {"fixed_proof_open", "no creation order from a static image", "B: the actors come from one static image, whose index is not read as an order"},
      {"fixed_proof_open", "lock sets do not decide the order", "C: the feasibility column, graded from the lock sets and paths, is undecidable"},
      {"fixed_proof_open", "indirect call with unresolved targets", "C: an indirect call whose targets are not resolved may run the read or the call of its context"},
      {"fixed_proof_open", "child threads not all resolved", "C: the context is not reached by the threads resolved, and their set is not complete"},
      {"fixed_proof_open", "proof rests on an assumption", "C: proven-impossible under a declared assumption or the premises of path C (rests-on in its layers)"},
      {"link_unlinked", "identity of the reader and the created thread not decided", "C: every row (the dependency column says so)"},
      {"link_unlinked", "self-reported identity", "C: the read's subscript is the reader's own identity"},
      {"link_unlinked", "namespace membership unresolved", "A: the comparison function the use's chain of calls passes to the namespace's callback search is not resolved, so the use may search another table (relations rules revision 7)"},
      {"reading_options_differ", "basis deviation of a unit of the row", "a row that names a position in a unit with a basis deviation (layers basis-deviation)"},
      {"reading_options_differ", "extraction options differ from the basis",
       "every row, when BASIS extract.reading_options_vs_basis says the extraction read the units with other options or another front end than the basis "
       "records (extract --accept-option-difference; layers reading-options-differ; relations rules revision 8)"},
  };
  return table;
}

std::string reason_text(const RelReason& r) { return std::string(r.reason) + "(" + r.sub + ")"; }

kz::Status relations_rows(const kz::Container& facts, const RelationsOptions& options, RelReport* out) {
  if (facts.header().artifact_kind != KZ_ARTIFACT_FACTS) return kz::Status::error(kz::Err::Invalid, "not a facts container");
  Rel r(facts);
  const kz::Status s = r.run(out);
  if (!s.ok() || options.tool_line.empty()) return s;
  // the tool line right after the `rules` line (static_facts.md 5x)
  const size_t at = out->header.find("\nrules ");
  if (at == std::string::npos) return kz::Status::error(kz::Err::Invalid, "relations header without a rules line");
  const size_t end = out->header.find('\n', at + 1);
  out->header.insert(end == std::string::npos ? out->header.size() : end + 1, options.tool_line + "\n");
  return s;
}

std::string render_row(const RelRow& r) {
  std::string t = "  ";
  t += r.path;
  t += std::to_string(r.number) + " " + r.head + "\n";
  for (const RelLine& l : r.lines) t += std::string(l.indent, ' ') + l.text + "\n";
  t += "      dependency: " + r.dependency + "\n";
  t += "      run-order: " + r.run_order + "\n";
  t += "      feasibility: " + r.feasibility + "\n";
  t += "      violation: " + r.violation + "\n";
  t += "      flip: " + r.flip + "\n";
  t += "      layers: " + r.layers + "\n";
  t += "      incomplete: ";
  const std::vector<RelReason>& table = relations_incomplete_reasons();
  for (size_t i = 0; i < r.incomplete.size(); ++i) t += (i == 0 ? "" : "; ") + (r.incomplete[i] < table.size() ? reason_text(table[r.incomplete[i]]) : std::string("?"));
  if (r.incomplete.empty()) t += "none found";
  t += "\n";
  return t;
}

std::string render_relations(const RelReport& rep) {
  std::string t = rep.header;
  for (const RelGroup& g : rep.groups) {
    t += g.text;
    for (const RelRow& r : g.rows) t += render_row(r);
  }
  return t;
}

std::string relation_keys_text(const RelReport& rep) {
  std::map<std::string, std::vector<std::string>> by_key;
  std::map<std::string, std::set<std::string>> texts;
  size_t rows = 0;
  std::string body;
  for (const RelGroup& g : rep.groups)
    for (const RelRow& r : g.rows) {
      ++rows;
      const std::string id = std::string(1, r.path) + std::to_string(r.number);
      by_key[r.key].push_back(id);
      texts[r.key].insert(r.key_text);
      body += id + " " + r.key + " " + r.key_text + "\n";
      for (const std::string& sk : r.site_keys) body += id + ".site " + sk + "\n";
    }
  size_t shared = 0, distinct_texts_sharing = 0;
  std::string coll;
  for (const auto& [k, ids] : by_key) {
    if (ids.size() < 2) continue;
    shared += ids.size();
    if (texts[k].size() > 1) ++distinct_texts_sharing;
    coll += "collision " + k + (texts[k].size() > 1 ? " (different key texts)" : " (one key text)") + ":";
    for (const std::string& id : ids) coll += " " + id;
    coll += "\n";
  }
  std::string t = "kz-relation-keys 1\n";
  t += "facts " + rep.facts + "\n";
  t += std::string("rules ") + relations_rules_revision() + "\n";
  t += "key: the first 16 hex digits of the sha256 of the key text (docs/static_facts.md §5l): path|subject|caller|callee#parameter|n-th call; path C adds |read <function> <n-th read>|<context>\n";
  t += "rows " + std::to_string(rows) + ", distinct keys " + std::to_string(by_key.size()) + ", rows that share a key " + std::to_string(shared) +
       ", keys shared by different key texts " + std::to_string(distinct_texts_sharing) + "\n";
  t += coll;
  t += body;
  return t;
}

kz::Status relations_report(const kz::Container& facts, const RelationsOptions& options, std::string* out) {
  RelReport rep;
  const kz::Status s = relations_rows(facts, options, &rep);
  if (!s.ok()) return s;
  *out = render_relations(rep);
  return kz::Status::success();
}

}  // namespace kzs

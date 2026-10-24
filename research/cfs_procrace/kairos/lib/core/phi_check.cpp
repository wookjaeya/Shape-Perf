// phi_check.cpp -- semantic checks of a profile: name resolution, sorts, vocabularies and
// provenance.  Every error is collected (the parser stops at the first syntax error, this does
// not).  The rules are listed in docs/phi_reference.md section 8; the error texts are stable
// and the tests match on them.

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>

#include "kz/core/phi.hpp"
#include "kz/core/phi_provenance.hpp"
#include "kz/core/registry.hpp"

namespace kz {
namespace phi {

namespace {

bool in_vocabulary(const char* enum_name, const std::string& word) {
  const EnumDesc* en = find_enum(enum_name);
  if (en == nullptr) return false;
  for (size_t i = 0; i < en->nitems; ++i)
    if (word == en->items[i].display) return true;
  return false;
}

std::string vocabulary_list(const char* enum_name) {
  const EnumDesc* en = find_enum(enum_name);
  std::string out;
  if (en == nullptr) return out;
  for (size_t i = 0; i < en->nitems; ++i) {
    if (i != 0) out += ", ";
    out += en->items[i].display;
  }
  return out;
}

bool valid_version(const std::string& v) {
  if (v.empty()) return false;
  size_t i = 0;
  bool digit_seen = false;
  while (i < v.size()) {
    if (v[i] >= '0' && v[i] <= '9') {
      digit_seen = true;
      ++i;
    } else if (v[i] == '.' && digit_seen && i + 1 < v.size() && v[i + 1] >= '0' && v[i + 1] <= '9') {
      digit_seen = false;
      ++i;
    } else {
      break;
    }
  }
  if (!digit_seen) return false;
  if (i < v.size() && v[i] >= 'a' && v[i] <= 'z') ++i;  // a release letter (6.5.0a)
  return i == v.size();
}

bool is_hex_digest(const std::string& s) {
  if (s.size() != 64) return false;
  for (char c : s)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}

bool is_rev(const std::string& s) {
  if (s.size() < 7 || s.size() > 40) return false;
  for (char c : s)
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  return true;
}

bool is_obligation(const std::string& s) {
  static const char* const names[] = {"O1", "O2", "O3", "O4", "O5", "O6", "O7", "O8", "O9", "O10"};
  for (const char* n : names)
    if (s == n) return true;
  return false;
}

struct Binder {
  std::string name;
  Sort sort;
};

struct ExprEnv {
  std::vector<ArgSpec> args;
  bool exec = false;
  bool result = false;
  std::vector<Binder> vars;
  bool open_binders = false;  // an unresolved plain name that is the argument of a resource constructor declares a variable
};

class Checker {
 public:
  Checker(Profile* p, std::vector<Diag>* d, const CheckOptions& o) : p_(*p), diags_(d), opt_(o) {}

  bool run();
  bool resolve_expr_public(Expr* e, const ExprContext& ctx, Sort expect);

 private:
  Profile& p_;
  std::vector<Diag>* diags_;
  CheckOptions opt_;
  bool ok_ = true;

  // ---- diagnostics ---------------------------------------------------------------------------
  void err(const SrcLoc& loc, const std::string& msg) {
    Diag d;
    d.severity = Severity::Error;
    d.loc = loc;
    d.message = msg;
    diags_->push_back(std::move(d));
    ok_ = false;
  }
  void warn(const SrcLoc& loc, const std::string& msg) {
    Diag d;
    d.severity = Severity::Warning;
    d.loc = loc;
    d.message = msg;
    diags_->push_back(std::move(d));
  }
  void note(const SrcLoc& loc, const std::string& msg) {
    Diag d;
    d.severity = Severity::Note;
    d.loc = loc;
    d.message = msg;
    diags_->push_back(std::move(d));
  }

  // ---- sorts ---------------------------------------------------------------------------------
  Sort bool_sort() const { return Sort{SortKind::Bool, -1, -1}; }
  Sort int_sort() const { return Sort{SortKind::Int, -1, -1}; }
  static Sort simple(SortKind k) { return Sort{k, -1, -1}; }

  std::string sort_name(const Sort& s) const {
    switch (s.kind) {
      case SortKind::Unknown: return "unknown";
      case SortKind::Bool: return "Bool";
      case SortKind::Int: return "Int";
      case SortKind::Str: return "Str";
      case SortKind::Actor: return "Actor";
      case SortKind::Loc: return "Loc";
      case SortKind::Id: return s.kind_index >= 0 ? "Id<" + p_.kinds[static_cast<size_t>(s.kind_index)].name + ">" : "Id<*>";
      case SortKind::State: return "State<" + p_.kinds[static_cast<size_t>(s.kind_index)].name + ">";
      case SortKind::Tag: return "Tag<" + p_.kinds[static_cast<size_t>(s.kind_index)].name + ">";
      case SortKind::Res: return "Res<" + p_.kinds[static_cast<size_t>(s.kind_index)].name + ">";
    }
    return "?";
  }

  // Id<*> (kind_index -2 or -1) is compatible with every Id; Int and Id compare with each other.
  bool compatible(const Sort& a, const Sort& b) const {
    if (a.kind == SortKind::Unknown || b.kind == SortKind::Unknown) return true;
    const bool a_num = a.kind == SortKind::Int || a.kind == SortKind::Id;
    const bool b_num = b.kind == SortKind::Int || b.kind == SortKind::Id;
    if (a_num && b_num) {
      if (a.kind == SortKind::Id && b.kind == SortKind::Id && a.kind_index >= 0 && b.kind_index >= 0) return a.kind_index == b.kind_index;
      return true;
    }
    if (a.kind != b.kind) return false;
    if (a.kind == SortKind::State || a.kind == SortKind::Res) return a.kind_index == b.kind_index;
    if (a.kind == SortKind::Tag) return a.kind_index == b.kind_index && a.tag_index == b.tag_index;
    return true;
  }

  bool resolve_sort_spec(SortSpec* s) {
    s->resolved = simple(s->kind);
    if (s->kind == SortKind::Id || s->kind == SortKind::Res) {
      const int32_t k = p_.find_kind(s->ref);
      if (k < 0) {
        err(s->loc, "unknown kind '" + s->ref + "' in " + (s->kind == SortKind::Id ? "Id<...>" : "Res<...>"));
        return false;
      }
      if (s->kind == SortKind::Id && !p_.kinds[static_cast<size_t>(k)].ids.present) {
        err(s->loc, "kind '" + s->ref + "' has no 'ids' clause, so Id<" + s->ref + "> is not a sort");
        return false;
      }
      s->resolved.kind_index = k;
    }
    return true;
  }

  // ---- provenance --------------------------------------------------------------------------
  void resolve_provs(const SrcLoc& loc, const std::vector<std::string>& names, std::vector<int32_t>* idx, const std::string& what) {
    idx->clear();
    for (const std::string& n : names) {
      const int32_t i = p_.find_prov(n);
      if (i < 0) {
        err(loc, "unknown provenance record '" + n + "' in " + what);
        continue;
      }
      idx->push_back(i);
    }
  }
  void need_provenance(const SrcLoc& loc, const std::vector<std::string>& names, const std::string& what) {
    if (opt_.require_provenance && names.empty()) err(loc, what + " cites no provenance record (every claim carries one; hand-entered claims use a record of tier researcher)");
  }
  void check_guards(const std::vector<Guard>& gs) {
    for (const Guard& g : gs) {
      if (p_.find_source(g.component) < 0) err(g.loc, "version guard names '" + g.component + "', which is not a declared source");
      if (!valid_version(g.version)) err(g.loc, "malformed version \"" + g.version + "\" (digits separated by '.', optionally one final letter)");
    }
  }
  void entry_common(EntryBase* e, const std::string& what) {
    resolve_provs(e->loc, e->prov, &e->prov_idx, what);
    need_provenance(e->loc, e->prov, what);
    check_guards(e->guards);
  }

  // ---- expressions -------------------------------------------------------------------------
  bool resolve_expr(Expr* e, ExprEnv* env, Sort hint);
  bool resolve_name(Expr* e, ExprEnv* env, Sort hint, bool report);
  bool resolve_call(Expr* e, ExprEnv* env);
  bool resolve_cmp(Expr* e, ExprEnv* env);
  // Warns when an Id is compared with an integer literal or constant outside 0 .. 0xFFFFFFFF: an Id
  // is a 32-bit value and the Int operand is not reduced (eval.hpp), so the result is decided by the
  // sorts alone (`id = -1` is always false).
  void warn_id_range(const Expr& id_side, const Expr& int_side, const SrcLoc& loc) {
    if (id_side.sort.kind != SortKind::Id || int_side.sort.kind != SortKind::Int) return;
    int64_t v = 0;
    if (int_side.kind == ExprKind::IntLit) {
      v = int_side.ival;
    } else if (int_side.kind == ExprKind::Name && int_side.ref == NameRef::Const && int_side.ref_index >= 0 &&
               p_.consts[static_cast<size_t>(int_side.ref_index)].value == ConstDecl::Value::Int) {
      v = p_.consts[static_cast<size_t>(int_side.ref_index)].ival;
    } else {
      return;
    }
    if (v < 0 || v > 0xFFFFFFFFll)
      warn(loc, "'" + expr_text(int_side) + "' is outside 0 .. 0xFFFFFFFF and is compared with the Id '" + expr_text(id_side) +
                    "': an Id is a 32-bit value and the integer is not reduced, so the comparison is decided by the sorts alone");
  }
  bool expect_bool(Expr* e, ExprEnv* env) {
    if (!resolve_expr(e, env, simple(SortKind::Unknown))) return false;
    if (e->sort.kind != SortKind::Bool) {
      err(e->loc, "expected a Bool expression, found " + sort_name(e->sort) + " in '" + expr_text(*e) + "'");
      return false;
    }
    return true;
  }

  // ---- declarations ------------------------------------------------------------------------
  void check_provs();
  void check_sources();
  void check_consts();
  void check_kinds();
  void check_kind_exprs();
  void check_locs();
  void check_context_scope();
  void check_binds();
  void check_sites();
  void check_ops();
  void check_inits();
  void check_rules();
  void check_rspecs();
  void check_trusted();
  void check_roles();

  void check_pattern(const Pattern& pat, const std::string& what);
  void check_lp(LpSpec* lp, const std::string& what);
  ExprEnv op_env(const OpDecl& op, bool result, const LpSpec* lp);
  int32_t find_state(const KindDecl& k, const std::string& s) const {
    for (size_t i = 0; i < k.states.size(); ++i)
      if (k.states[i] == s) return static_cast<int32_t>(i);
    return -1;
  }
  bool res_cycle(int32_t kind, std::vector<int32_t>* path) const {
    for (int32_t k : *path)
      if (k == kind) return true;
    path->push_back(kind);
    for (const KindParam& kp : p_.kinds[static_cast<size_t>(kind)].params)
      if (kp.sort.resolved.kind == SortKind::Res && kp.sort.resolved.kind_index >= 0 && res_cycle(kp.sort.resolved.kind_index, path)) return true;
    path->pop_back();
    return false;
  }
  bool is_terminal_symbol(const std::string& fn) const {
    for (const OpDecl& o : p_.ops)
      if (o.terminal.present && o.symbol == fn) return true;
    return false;
  }
};

// ================================================================================================
// expressions
// ================================================================================================

bool Checker::resolve_expr_public(Expr* e, const ExprContext& ctx, Sort expect) {
  ExprEnv env;
  env.args = ctx.args;
  env.exec = ctx.exec;
  env.result = ctx.result;
  if (!resolve_expr(e, &env, expect)) return false;
  return true;
}

bool Checker::resolve_name(Expr* e, ExprEnv* env, Sort hint, bool report) {
  const std::string& n = e->text;
  for (size_t i = env->vars.size(); i-- > 0;) {
    if (env->vars[i].name == n) {
      e->ref = NameRef::BoundVar;
      e->ref_index = static_cast<int32_t>(i);
      e->sort = env->vars[i].sort;
      return true;
    }
  }
  if (n == "exec") {
    if (!env->exec) {
      if (report) err(e->loc, "'exec' (the executing actor) is not available here");
      return false;
    }
    e->ref = NameRef::Exec;
    e->sort = simple(SortKind::Actor);
    return true;
  }
  if (n == "result") {
    if (!env->result) {
      if (report) err(e->loc, "'result' (the value the call returned) is not available here");
      return false;
    }
    e->ref = NameRef::Result;
    e->sort = int_sort();
    return true;
  }
  for (size_t i = 0; i < env->args.size(); ++i) {
    if (env->args[i].name == n) {
      e->ref = NameRef::Arg;
      e->ref_index = static_cast<int32_t>(i);
      e->sort = env->args[i].sort.kind == SortKind::Unknown ? int_sort() : env->args[i].sort;
      return true;
    }
  }
  if (n.rfind("arg.", 0) == 0 || n.rfind("hdr.", 0) == 0 || n.rfind("lp.", 0) == 0) {
    if (report) err(e->loc, "unknown parameter '" + n + "'");
    return false;
  }
  if (hint.kind == SortKind::State && hint.kind_index >= 0) {
    const int32_t o = find_state(p_.kinds[static_cast<size_t>(hint.kind_index)], n);
    if (o >= 0) {
      e->ref = NameRef::StateConst;
      e->ref_index = o;
      e->sort = hint;
      return true;
    }
  }
  if (hint.kind == SortKind::Tag && hint.kind_index >= 0) {
    const KindDecl& k = p_.kinds[static_cast<size_t>(hint.kind_index)];
    const TagDecl& t = k.tags[static_cast<size_t>(hint.tag_index)];
    for (size_t i = 0; i < t.values.size(); ++i)
      if (t.values[i] == n) {
        e->ref = NameRef::TagConst;
        e->ref_index = static_cast<int32_t>(i);
        e->sort = hint;
        return true;
      }
  }
  const int32_t c = p_.find_const(n);
  if (c >= 0) {
    const ConstDecl& cd = p_.consts[static_cast<size_t>(c)];
    e->ref = NameRef::Const;
    e->ref_index = c;
    e->sort = cd.value == ConstDecl::Value::Str ? simple(SortKind::Str) : cd.value == ConstDecl::Value::Bool ? bool_sort() : int_sort();
    return true;
  }
  const int32_t l = p_.find_loc(n);
  if (l >= 0) {
    e->ref = NameRef::LocRef;
    e->ref_index = l;
    e->sort = simple(SortKind::Loc);
    return true;
  }
  if (report) err(e->loc, "unknown name '" + n + "'");
  return false;
}

bool Checker::resolve_cmp(Expr* e, ExprEnv* env) {
  Expr* l = e->args[0].get();
  Expr* r = e->args[1].get();
  bool ok = true;
  // A bare constant name on the left (REG = state(x)) needs the sort of the right side first.
  bool left_first = true;
  if (l->kind == ExprKind::Name) {
    ExprPtr copy = clone_expr(*l);
    left_first = resolve_name(copy.get(), env, simple(SortKind::Unknown), false);
  }
  if (left_first) {
    ok = resolve_expr(l, env, simple(SortKind::Unknown)) && ok;
    ok = resolve_expr(r, env, ok ? l->sort : simple(SortKind::Unknown)) && ok;
  } else {
    ok = resolve_expr(r, env, simple(SortKind::Unknown)) && ok;
    ok = resolve_expr(l, env, ok ? r->sort : simple(SortKind::Unknown)) && ok;
  }
  e->sort = bool_sort();
  if (!ok) return false;
  if (!compatible(l->sort, r->sort)) {
    err(e->loc, "cannot compare " + sort_name(l->sort) + " with " + sort_name(r->sort) + " in '" + expr_text(*e) + "'");
    return false;
  }
  if (e->cmp != CmpOp::Eq && e->cmp != CmpOp::Ne) {
    const SortKind k = l->sort.kind;
    if (!(k == SortKind::Int || k == SortKind::Id || k == SortKind::State)) {
      err(e->loc, std::string("the operator '") + cmp_text(e->cmp) + "' needs Int, Id or State operands, found " + sort_name(l->sort));
      return false;
    }
  }
  warn_id_range(*l, *r, e->loc);
  warn_id_range(*r, *l, e->loc);
  return true;
}

bool Checker::resolve_call(Expr* e, ExprEnv* env) {
  const size_t n = e->args.size();
  auto arity = [&](size_t want) {
    if (n == want) return true;
    err(e->loc, "'" + e->text + "' takes " + std::to_string(want) + " argument" + (want == 1 ? "" : "s") + ", found " + std::to_string(n));
    return false;
  };
  switch (e->fn) {
    case CallFn::State:
    case CallFn::Key:
    case CallFn::Gen:
    case CallFn::Id: {
      if (!arity(1)) return false;
      if (!resolve_expr(e->args[0].get(), env, simple(SortKind::Unknown))) return false;
      const Sort& a = e->args[0]->sort;
      if (a.kind != SortKind::Res) {
        err(e->loc, "'" + e->text + "' needs a resource (a kind applied to its key), found " + sort_name(a));
        return false;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(a.kind_index)];
      if (e->fn == CallFn::State) {
        e->sort = Sort{SortKind::State, a.kind_index, -1};
      } else if (e->fn == CallFn::Gen) {
        e->sort = int_sort();
      } else if (e->fn == CallFn::Id) {
        if (!k.ids.present) {
          err(e->loc, "kind '" + k.name + "' has no 'ids' clause, so id(...) is undefined for it");
          return false;
        }
        e->sort = Sort{SortKind::Id, a.kind_index, -1};
      } else {
        if (k.params.empty()) {
          err(e->loc, "kind '" + k.name + "' has no key");
          return false;
        }
        e->sort = k.params[0].sort.resolved;
      }
      return true;
    }
    case CallFn::Valid: {
      if (!arity(1)) return false;
      if (!resolve_expr(e->args[0].get(), env, simple(SortKind::Unknown))) return false;
      if (e->args[0]->sort.kind != SortKind::Id) {
        err(e->loc, "valid(...) needs an Id, found " + sort_name(e->args[0]->sort));
        return false;
      }
      e->sort = bool_sort();
      return true;
    }
    case CallFn::Value: {
      if (!arity(1)) return false;
      if (!resolve_expr(e->args[0].get(), env, simple(SortKind::Unknown))) return false;
      Expr* a = e->args[0].get();
      if (a->sort.kind != SortKind::Loc) {
        err(e->loc, "value(...) needs a location, found " + sort_name(a->sort));
        return false;
      }
      if (a->ref == NameRef::LocRef) {
        const LocDecl& l = p_.locs[static_cast<size_t>(a->ref_index)];
        e->sort = l.holds.resolved;
        e->ref_aux = a->ref_index;
      } else {
        e->sort = Sort{SortKind::Id, -2, -1};  // a location bound to a parameter: the kind of Id is known at run time
      }
      return true;
    }
    case CallFn::Tag: {
      if (!arity(2)) return false;
      if (!resolve_expr(e->args[0].get(), env, simple(SortKind::Unknown))) return false;
      const Sort& a = e->args[0]->sort;
      if (a.kind != SortKind::Res) {
        err(e->loc, "tag(...) needs a resource as its first argument, found " + sort_name(a));
        return false;
      }
      Expr* t = e->args[1].get();
      if (t->kind != ExprKind::Name) {
        err(t->loc, "tag(...) needs the name of a tag as its second argument");
        return false;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(a.kind_index)];
      int32_t ti = -1;
      for (size_t i = 0; i < k.tags.size(); ++i)
        if (k.tags[i].name == t->text) ti = static_cast<int32_t>(i);
      if (ti < 0) {
        err(t->loc, "kind '" + k.name + "' has no tag '" + t->text + "'");
        return false;
      }
      t->sort = simple(SortKind::Str);
      e->ref_aux = ti;
      e->sort = Sort{SortKind::Tag, a.kind_index, ti};
      return true;
    }
    case CallFn::Owner: {
      if (!arity(1)) return false;
      if (!resolve_expr(e->args[0].get(), env, simple(SortKind::Unknown))) return false;
      if (e->args[0]->sort.kind != SortKind::Actor) {
        err(e->loc, "owner(...) needs an actor, found " + sort_name(e->args[0]->sort));
        return false;
      }
      if (!p_.context.present || p_.context.owner_kind_index < 0) {
        err(e->loc, "owner(...) needs 'context { owner_kind K; }' in the profile");
        return false;
      }
      e->sort = Sort{SortKind::Res, p_.context.owner_kind_index, -1};
      return true;
    }
    case CallFn::Res: {
      const int32_t ki = p_.find_kind(e->text);
      if (ki < 0) {
        err(e->loc, "unknown function or kind '" + e->text + "'");
        return false;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(ki)];
      if (n != k.params.size()) {
        err(e->loc, "kind '" + k.name + "' takes " + std::to_string(k.params.size()) + " key argument(s), found " + std::to_string(n));
        return false;
      }
      bool ok = true;
      for (size_t i = 0; i < n; ++i) {
        Expr* a = e->args[i].get();
        const Sort want = k.params[i].sort.resolved;
        if (env->open_binders && a->kind == ExprKind::Name) {
          ExprPtr copy = clone_expr(*a);
          if (!resolve_name(copy.get(), env, simple(SortKind::Unknown), false)) {
            env->vars.push_back(Binder{a->text, want});
          }
        }
        if (!resolve_expr(a, env, simple(SortKind::Unknown))) {
          ok = false;
          continue;
        }
        if (!compatible(a->sort, want) || (want.kind == SortKind::Res && a->sort.kind != SortKind::Res) ||
            (want.kind == SortKind::Actor && a->sort.kind != SortKind::Actor) || (want.kind == SortKind::Loc && a->sort.kind != SortKind::Loc)) {
          err(a->loc, "key argument " + std::to_string(i + 1) + " of '" + k.name + "' is " + sort_name(a->sort) + ", the kind wants " + sort_name(want));
          ok = false;
        }
      }
      if (!ok) return false;
      e->ref_index = ki;
      e->sort = Sort{SortKind::Res, ki, -1};
      return true;
    }
  }
  return false;
}

bool Checker::resolve_expr(Expr* e, ExprEnv* env, Sort hint) {
  switch (e->kind) {
    case ExprKind::BoolLit: e->sort = bool_sort(); return true;
    case ExprKind::IntLit: e->sort = int_sort(); return true;
    case ExprKind::StrLit: e->sort = simple(SortKind::Str); return true;
    case ExprKind::Name: return resolve_name(e, env, hint, true);
    case ExprKind::Not:
    case ExprKind::And:
    case ExprKind::Or:
    case ExprKind::Imp: {
      bool ok = true;
      for (ExprPtr& a : e->args) ok = expect_bool(a.get(), env) && ok;
      e->sort = bool_sort();
      return ok;
    }
    case ExprKind::Cmp: return resolve_cmp(e, env);
    case ExprKind::In: {
      Expr* x = e->args[0].get();
      bool ok = resolve_expr(x, env, simple(SortKind::Unknown));
      for (size_t i = 1; i < e->args.size(); ++i) {
        ok = resolve_expr(e->args[i].get(), env, ok ? x->sort : simple(SortKind::Unknown)) && ok;
        if (ok && !compatible(x->sort, e->args[i]->sort)) {
          err(e->args[i]->loc, "cannot compare " + sort_name(x->sort) + " with " + sort_name(e->args[i]->sort) + " in a set");
          ok = false;
        }
        if (ok) {
          warn_id_range(*x, *e->args[i], e->args[i]->loc);
          warn_id_range(*e->args[i], *x, e->args[i]->loc);
        }
      }
      e->sort = bool_sort();
      return ok;
    }
    case ExprKind::Call: return resolve_call(e, env);
    case ExprKind::Quant: {
      const int32_t ki = p_.find_kind(e->kind_name);
      if (ki < 0) {
        err(e->loc, "unknown kind '" + e->kind_name + "' in a quantifier");
        return false;
      }
      if (p_.kinds[static_cast<size_t>(ki)].table.empty()) {
        err(e->loc, "kind '" + e->kind_name + "' has no 'table' clause: a quantifier ranges only over kinds with a finite set of instances");
        return false;
      }
      e->ref_index = ki;
      env->vars.push_back(Binder{e->text, Sort{SortKind::Res, ki, -1}});
      bool ok = true;
      if (e->args[0]) ok = expect_bool(e->args[0].get(), env) && ok;
      ok = expect_bool(e->args[1].get(), env) && ok;
      env->vars.pop_back();
      e->sort = bool_sort();
      return ok;
    }
  }
  return false;
}

// ================================================================================================
// declarations
// ================================================================================================

void Checker::check_provs() {
  for (ProvDecl& d : p_.provs) {
    if (!in_vocabulary("KZ_TIER", d.tier)) {
      err(d.loc, "provenance '" + d.name + "': unknown tier '" + d.tier + "' (expected one of: " + vocabulary_list("KZ_TIER") + ")");
      continue;
    }
    if (!in_vocabulary("KZ_PROV_KIND", d.kind)) {
      err(d.loc, "provenance '" + d.name + "': unknown kind '" + d.kind + "' (expected one of: " + vocabulary_list("KZ_PROV_KIND") + ")");
      continue;
    }
    if (!tier_kind_compatible(d.tier, d.kind)) {
      std::string msg = "provenance '" + d.name + "': tier '" + d.tier + "' does not go with kind '" + d.kind +
                        "' (contract-backed: api_doc; requirement-derived: requirement; admitted: code_admission; issue-derived: issue; code-derived: code_pattern; researcher: researcher; unreferenced: none)";
      if (d.kind == "requirement")
        msg += "; the tier of a requirement record is requirement-derived (SPEC 1.6), which the vocabulary KZ_TIER of this build does not contain, so a requirement record is not accepted yet";
      err(d.loc, msg);
    }
    if (!d.side.empty() && d.side != "supplier" && d.side != "caller" && d.side != "rule")
      err(d.loc, "provenance '" + d.name + "': unknown side '" + d.side + "' (expected supplier, caller or rule)");
    if (d.side == "caller" && !caller_side_tier(d.tier))
      err(d.loc, "provenance '" + d.name + "': a record of tier '" + d.tier +
                     "' cannot have side caller: a caller-side record is a cited text addressed to the caller (tier contract-backed, requirement-derived, admitted or issue-derived); a code pattern is a rule, and a hand-entered record never accepts an outcome (QT-4) nor states an appreq (QT-9)");
    const bool cites_text = d.kind == "api_doc" || d.kind == "requirement" || d.kind == "code_admission" || d.kind == "code_pattern";
    if (cites_text) {
      if (d.file.empty()) err(d.loc, "provenance '" + d.name + "' of kind " + d.kind + " names no file");
      if (d.line_from == 0) err(d.loc, "provenance '" + d.name + "' of kind " + d.kind + " names no lines");
      if (d.sha256.empty()) err(d.loc, "provenance '" + d.name + "' of kind " + d.kind + " has no sha256 of the cited text");
    }
    if (!d.sha256.empty() && !is_hex_digest(d.sha256))
      err(d.loc, "provenance '" + d.name + "': sha256 must be 64 lower-case hexadecimal digits");
    if (!d.file.empty() && d.line_from == 0) err(d.loc, "provenance '" + d.name + "' names a file but no lines");
    if (d.file.empty() && (d.line_from != 0 || !d.sha256.empty())) err(d.loc, "provenance '" + d.name + "' has lines or a hash but no file");
    if (!d.file.empty() && opt_.require_sources && resolve_source(p_, d.file) < 0)
      err(d.loc, "provenance '" + d.name + "': the file '" + d.file + "' belongs to no declared source");
    if (d.kind == "issue" && d.cite.empty()) err(d.loc, "provenance '" + d.name + "' of kind issue has no 'cite' (issue or commit locator)");
    if (d.kind == "researcher" && d.authored.empty())
      err(d.loc, "provenance '" + d.name + "' of kind researcher has no 'authored' record (who entered it, and which cases they had inspected)");
  }
}

void Checker::check_sources() {
  std::set<std::string> roots;
  for (SourceDecl& d : p_.sources) {
    if (!is_rev(d.rev)) err(d.loc, "source '" + d.name + "': the revision must be 7 to 40 lower-case hexadecimal digits");
    if (!roots.insert(d.root).second) err(d.loc, "source '" + d.name + "': another source has the same root '" + d.root + "'");
    if (!d.root.empty() && (d.root.front() == '/' || d.root.back() == '/')) err(d.loc, "source '" + d.name + "': the root is relative and has no leading or trailing '/'");
    resolve_provs(d.loc, d.prov, &d.prov_idx, "source '" + d.name + "'");
    check_guards(d.guards);
  }
}

void Checker::check_consts() {
  for (ConstDecl& d : p_.consts) {
    entry_common(&d, "constant '" + d.name + "'");
    if (d.value == ConstDecl::Value::RefEval) {
      if (d.ref_symbol.empty()) err(d.loc, "constant '" + d.name + "': ref eval names no symbol");
      if (opt_.require_sources && resolve_source(p_, d.ref_file) < 0)
        err(d.loc, "constant '" + d.name + "': the translation unit '" + d.ref_file + "' belongs to no declared source");
    }
  }
}

void Checker::check_kinds() {
  for (KindDecl& k : p_.kinds) {
    entry_common(&k, "kind '" + k.name + "'");
    if (k.states.empty()) err(k.loc, "kind '" + k.name + "' declares no states");
    if (k.states.size() > 64) err(k.loc, "kind '" + k.name + "' declares more than 64 states (the abstract domain keeps a set of states in one word)");
    std::set<std::string> seen;
    for (const std::string& s : k.states)
      if (!seen.insert(s).second) err(k.loc, "kind '" + k.name + "': state '" + s + "' is declared twice");
    if (k.initial.empty()) {
      err(k.loc, "kind '" + k.name + "' has no 'initial' state");
    } else {
      k.initial_ordinal = find_state(k, k.initial);
      if (k.initial_ordinal < 0) err(k.loc, "kind '" + k.name + "': the initial state '" + k.initial + "' is not one of its states");
    }
    if (!k.scope.empty() && k.scope != "process" && k.scope != "persistent")
      err(k.loc, "kind '" + k.name + "': scope must be process or persistent, found '" + k.scope + "'");
    if (!k.table.empty()) {
      const int32_t c = p_.find_const(k.table);
      if (c < 0) {
        err(k.loc, "kind '" + k.name + "': the table size '" + k.table + "' is not a declared constant");
      } else {
        const ConstDecl& cd = p_.consts[static_cast<size_t>(c)];
        if (cd.value != ConstDecl::Value::Int && cd.value != ConstDecl::Value::RefEval)
          err(k.loc, "kind '" + k.name + "': the table size '" + k.table + "' must be an integer constant");
      }
    }
    if (k.ids.present) {
      struct Item {
        const std::string* name;
        int64_t* dst;
        const char* what;
      };
      const Item items[] = {{&k.ids.undefined_c, &k.ids.undefined, "undefined"},
                            {&k.ids.reserved_c, &k.ids.reserved, "reserved"},
                            {&k.ids.base_c, &k.ids.base, "base"},
                            {&k.ids.serial_max_c, &k.ids.serial_max, "serial_max"}};
      bool ids_ok = true;
      for (const Item& it : items) {
        if (it.name->empty()) {
          err(k.ids.loc, "kind '" + k.name + "': ids lacks '" + it.what + "'");
          ids_ok = false;
          continue;
        }
        const int32_t c = p_.find_const(*it.name);
        if (c < 0 || p_.consts[static_cast<size_t>(c)].value != ConstDecl::Value::Int) {
          err(k.ids.loc, "kind '" + k.name + "': ids " + it.what + " '" + *it.name + "' is not a declared integer constant (a literal)");
          ids_ok = false;
          continue;
        }
        *it.dst = p_.consts[static_cast<size_t>(c)].ival;
      }
      if (ids_ok) {
        const int64_t top = 0xFFFFFFFFll;
        // No sum: base and serial_max are 64-bit literals of the profile, and base + serial_max
        // would overflow for the values this check exists to reject.
        if (k.ids.base < 0 || k.ids.serial_max < 0 || k.ids.base > top || k.ids.serial_max > top - k.ids.base)
          err(k.ids.loc, "kind '" + k.name + "': base and base + serial_max must lie in 0 .. 0xFFFFFFFF (an Id is a 32-bit value)");
        if (k.ids.undefined < 0 || k.ids.undefined > top || k.ids.reserved < 0 || k.ids.reserved > top)
          err(k.ids.loc, "kind '" + k.name + "': the undefined and reserved markers must lie in 0 .. 0xFFFFFFFF");
        if (k.ids.undefined == k.ids.reserved) err(k.ids.loc, "kind '" + k.name + "': the undefined and the reserved marker are equal");
      }
    }
    for (TagDecl& t : k.tags) {
      std::set<std::string> vs;
      if (t.values.empty()) err(t.loc, "tag '" + t.name + "' has no values");
      if (t.values.size() > 64) err(t.loc, "tag '" + t.name + "' has more than 64 values");
      for (const std::string& v : t.values)
        if (!vs.insert(v).second) err(t.loc, "tag '" + t.name + "': value '" + v + "' is declared twice");
    }
    for (TransitionDecl& t : k.transitions) {
      if (find_state(k, t.from) < 0) err(t.loc, "transition from unknown state '" + t.from + "' in kind '" + k.name + "'");
      if (find_state(k, t.to) < 0) err(t.loc, "transition to unknown state '" + t.to + "' in kind '" + k.name + "'");
    }
    for (DerivedDecl& d : k.derived)
      if (find_state(k, d.state) < 0) err(d.loc, "derived rule for unknown state '" + d.state + "' in kind '" + k.name + "'");
  }
  // Sorts of the parameters need every kind's ids to be known first.
  for (KindDecl& k : p_.kinds)
    for (KindParam& kp : k.params) resolve_sort_spec(&kp.sort);
  for (size_t i = 0; i < p_.kinds.size(); ++i) {
    std::vector<int32_t> path;
    if (res_cycle(static_cast<int32_t>(i), &path)) {
      err(p_.kinds[i].loc, "kind '" + p_.kinds[i].name + "': its Res<...> parameters form a cycle");
      break;
    }
  }
}

void Checker::check_kind_exprs() {
  for (KindDecl& k : p_.kinds) {
    for (DerivedDecl& d : k.derived) {
      ExprEnv env;
      for (const KindParam& kp : k.params) env.vars.push_back(Binder{kp.name, kp.sort.resolved});
      expect_bool(d.when.get(), &env);
    }
  }
}

void Checker::check_locs() {
  for (LocDecl& l : p_.locs) {
    entry_common(&l, "location '" + l.name + "'");
    if (l.struct_name.empty() || l.field.empty()) err(l.loc, "location '" + l.name + "' needs 'struct' and 'field'");
    resolve_sort_spec(&l.holds);
  }
}

void Checker::check_context_scope() {
  if (p_.context.present) {
    ContextDecl& c = p_.context;
    c.owner_kind_index = p_.find_kind(c.owner_kind);
    if (c.owner_kind_index < 0) err(c.loc, "context: owner_kind '" + c.owner_kind + "' is not a declared kind");
    resolve_provs(c.loc, c.prov, &c.prov_idx, "context");
    need_provenance(c.loc, c.prov, "the context");
  }
  if (p_.scope.present) {
    ScopeDecl& s = p_.scope;
    s.kind_index = p_.find_kind(s.kind);
    if (s.kind_index < 0) {
      err(s.loc, "scope: kind '" + s.kind + "' is not a declared kind");
    } else {
      const KindDecl& k = p_.kinds[static_cast<size_t>(s.kind_index)];
      const int32_t r = find_state(k, s.request);
      const int32_t t = find_state(k, s.teardown);
      if (r < 0) err(s.loc, "scope: '" + s.request + "' is not a state of kind '" + k.name + "'");
      if (t < 0) err(s.loc, "scope: '" + s.teardown + "' is not a state of kind '" + k.name + "'");
      if (r >= 0 && t >= 0 && r >= t) err(s.loc, "scope: the request state must come before the teardown state in kind '" + k.name + "'");
    }
    resolve_provs(s.loc, s.prov, &s.prov_idx, "scope");
    need_provenance(s.loc, s.prov, "the scope");
  }
}

void Checker::check_binds() {
  for (BindDecl& b : p_.binds) {
    entry_common(&b, "binding '" + b.name + "'");
    if (b.struct_name.empty() || b.field.empty()) err(b.loc, "binding needs a struct type and a field");
    Expr* t = b.target.get();
    if (t == nullptr) continue;
    ExprEnv env;
    env.open_binders = true;
    if (t->kind != ExprKind::Call || !(t->fn == CallFn::State || t->fn == CallFn::Tag || t->fn == CallFn::Id || t->fn == CallFn::Value)) {
      err(t->loc, "a binding target is state(K(v)), tag(K(v), T), id(K(v)) or value(L), found '" + expr_text(*t) + "'");
      continue;
    }
    if (resolve_expr(t, &env, simple(SortKind::Unknown)) && t->fn == CallFn::Value && t->ref_aux >= 0) {
      const LocDecl& l = p_.locs[static_cast<size_t>(t->ref_aux)];
      if (l.struct_name != b.struct_name || l.field != b.field)
        err(b.loc, "binding " + b.struct_name + "." + b.field + " names location '" + l.name + "', which is declared as " + l.struct_name + "." + l.field);
    }
  }
}

void Checker::check_pattern(const Pattern& pat, const std::string& what) {
  if (!pat.present) return;
  if (pat.fn.empty()) err(pat.loc, what + ": the pattern names no function");
  if (pat.op == PatOp::Call && pat.callee.empty()) err(pat.loc, what + ": the call pattern names no callee");
  for (const ArgPred& a : pat.args) {
    if ((a.cmp == ArgCmp::Eq || a.cmp == ArgCmp::Ne) && a.is_name && p_.find_const(a.name) < 0)
      err(pat.loc, what + ": the pattern compares an argument with '" + a.name + "', which is not a declared constant");
  }
}

void Checker::check_sites() {
  for (SiteDecl& s : p_.sites) {
    entry_common(&s, "site '" + s.name + "'");
    check_pattern(s.pattern, "site '" + s.name + "'");
  }
}

ExprEnv Checker::op_env(const OpDecl& op, bool result, const LpSpec* lp) {
  ExprEnv env;
  env.exec = true;
  env.result = result;
  for (const OpParam& prm : op.params) {
    if (prm.role == "out") continue;
    env.args.push_back(ArgSpec{"arg." + prm.name, prm.has_sort ? prm.sort.resolved : int_sort()});
  }
  if (lp != nullptr)
    for (const OpParam& v : lp->values) env.args.push_back(ArgSpec{"lp." + v.name, v.sort.resolved});
  return env;
}

void Checker::check_lp(LpSpec* lp, const std::string& what) {
  for (OpParam& v : lp->values) resolve_sort_spec(&v.sort);
  if (lp->kind == LpKind::At) check_pattern(lp->pattern, what);
  if (lp->kind == LpKind::Site && p_.find_site(lp->site) < 0) err(lp->loc, what + ": unknown site '" + lp->site + "'");
}

namespace {

// A value conjunct (lp capture) is about a value the caller passes: it may read only parameters
// of role stamp, key or id, constants and literals, and the function valid(...).
bool capture_ok(const Expr& e, const OpDecl& op, std::string* why) {
  switch (e.kind) {
    case ExprKind::Name:
      if (e.ref == NameRef::Arg) {
        const std::string pn = e.text.substr(e.text.find('.') + 1);
        for (const OpParam& prm : op.params)
          if (prm.name == pn) {
            if (prm.role == "stamp" || prm.role == "key" || prm.role == "id") return true;
            *why = "parameter '" + pn + "' has role '" + (prm.role.empty() ? "none" : prm.role) + "'";
            return false;
          }
      }
      if (e.ref == NameRef::Const || e.ref == NameRef::StateConst) return true;
      *why = "'" + e.text + "' is not a value the caller passes";
      return false;
    case ExprKind::Call:
      if (e.fn != CallFn::Valid) {
        *why = "the function '" + e.text + "' reads resource state";
        return false;
      }
      break;
    case ExprKind::Quant:
      *why = "a quantifier reads resource state";
      return false;
    default: break;
  }
  for (const ExprPtr& a : e.args)
    if (a && !capture_ok(*a, op, why)) return false;
  return true;
}

bool mentions_arg(const Expr& e) {
  if (e.kind == ExprKind::Name && e.ref == NameRef::Arg) return true;
  for (const ExprPtr& a : e.args)
    if (a && mentions_arg(*a)) return true;
  return false;
}

}  // namespace

void Checker::check_ops() {
  static const char* const roles[] = {"", "out", "stamp", "key", "id", "buffer", "msg"};
  static const char* const sync_roles[] = {"acq", "rel", "give", "take", "flush", "put", "get", "create", "start", "exit", "join", "observe", "timeout"};
  for (OpDecl& op : p_.ops) {
    const std::string what = "operation '" + op.name + "'";
    entry_common(&op, what);
    std::set<std::string> pn;
    for (OpParam& prm : op.params) {
      bool role_ok = false;
      for (const char* r : roles)
        if (prm.role == r) role_ok = true;
      if (!role_ok) err(op.loc, what + ": unknown parameter role '" + prm.role + "'");
      if (!pn.insert(prm.name).second) err(op.loc, what + ": parameter '" + prm.name + "' is declared twice");
      if (prm.has_sort) resolve_sort_spec(&prm.sort);
    }
    if (op.symbol.empty()) warn(op.loc, what + " has no 'bind symbol'");
    // conjuncts
    for (ConjunctDecl& c : op.conjuncts) {
      check_lp(&c.lp, what + ", conjunct '" + c.label + "'");
      resolve_provs(c.loc, c.prov, &c.prov_idx, what + ", conjunct '" + c.label + "'");
      ExprEnv env = op_env(op, false, &c.lp);
      if (!expect_bool(c.expr.get(), &env)) continue;
      if (c.lp.kind == LpKind::Capture) {
        std::string why;
        if (!capture_ok(*c.expr, op, &why) || !mentions_arg(*c.expr))
          err(c.loc, what + ", conjunct '" + c.label + "': a conjunct at 'lp capture' is about a value the caller passes (a parameter of role stamp, key or id)" +
                         (why.empty() ? "" : ": " + why));
      } else if (c.lp.kind == LpKind::At && !op.symbol.empty() && c.lp.pattern.fn != op.symbol) {
        warn(c.loc, what + ", conjunct '" + c.label + "': the pattern searches function '" + c.lp.pattern.fn + "', the operation is bound to '" + op.symbol + "'");
      }
    }
    // effects
    for (EffectDecl& e : op.effects) {
      const std::string ewhat = what + ", effect '" + e.label + "'";
      check_lp(&e.lp, ewhat);
      if (e.lp.kind == LpKind::Capture) err(e.loc, ewhat + ": 'lp capture' is for conjuncts; an effect names the point where it takes place");
      if (e.lp.kind == LpKind::None) err(e.loc, ewhat + ": no linearization point");
      if (e.lp.kind == LpKind::At && !op.symbol.empty() && e.lp.pattern.fn != op.symbol)
        warn(e.loc, ewhat + ": the pattern searches function '" + e.lp.pattern.fn + "', the operation is bound to '" + op.symbol + "'");
      ExprEnv env = op_env(op, true, &e.lp);
      resolve_provs(e.loc, e.prov, &e.prov_idx, ewhat);
      need_provenance(e.loc, e.prov, ewhat);
      check_guards(e.guards);
      if (!e.status.empty() && e.status != "declared_trusted" && e.status != "declared_verified")
        err(e.loc, ewhat + ": unknown status '" + e.status + "' (expected declared_trusted or declared_verified)");
      if (e.status == "declared_trusted" && e.refutable_by.empty()) err(e.loc, ewhat + ": a declared_trusted effect names the obligations that can refute it (refutable_by O3, O4 ...)");
      for (const std::string& o : e.refutable_by)
        if (!is_obligation(o)) err(e.loc, ewhat + ": '" + o + "' is not an obligation (O1 ... O10)");
      if (e.when) expect_bool(e.when.get(), &env);
      if (e.filter && e.status == "declared_trusted")
        err(e.loc, ewhat + ": the guard of a declared_trusted effect is asserted (`when`), never a filter: a filter would hide the observations that refute it");
      auto res_of = [&](Sort* s) {
        if (!e.res || !resolve_expr(e.res.get(), &env, simple(SortKind::Unknown))) return false;
        if (e.res->sort.kind != SortKind::Res) {
          err(e.res->loc, ewhat + ": expected a resource, found " + sort_name(e.res->sort));
          return false;
        }
        *s = e.res->sort;
        return true;
      };
      Sort rs;
      switch (e.kind) {
        case EffectKind::Set: {
          if (!e.all_kind.empty()) {
            const int32_t ki = p_.find_kind(e.all_kind);
            if (ki < 0) {
              err(e.loc, ewhat + ": unknown kind '" + e.all_kind + "'");
              break;
            }
            const KindDecl& k = p_.kinds[static_cast<size_t>(ki)];
            const int32_t to = find_state(k, e.to_state);
            if (to < 0) err(e.loc, ewhat + ": '" + e.to_state + "' is not a state of kind '" + k.name + "'");
            // Without a finite instance set, every resource of the kind can only be returned to the initial state:
            // the replay keeps no state of its own for any of them (a memset of the whole table, 2026-10-24).
            if (k.table.empty() && to >= 0 && k.initial != e.to_state)
              err(e.loc, ewhat + ": 'lifecycle all " + k.name + " -> " + e.to_state + "' needs a kind with a finite set of instances (a 'table' clause); without one, only the initial state '" + k.initial + "' can be set");
            if (e.with_id) err(e.loc, ewhat + ": an effect on every instance cannot carry an id");
            break;
          }
          if (!res_of(&rs)) break;
          const KindDecl& k = p_.kinds[static_cast<size_t>(rs.kind_index)];
          if (find_state(k, e.to_state) < 0) err(e.loc, ewhat + ": '" + e.to_state + "' is not a state of kind '" + k.name + "'");
          if (e.with_id) {
            if (!k.ids.present) err(e.loc, ewhat + ": kind '" + k.name + "' has no ids, so the effect cannot carry an id");
            else if (resolve_expr(e.with_id.get(), &env, Sort{SortKind::Id, rs.kind_index, -1}) && !compatible(e.with_id->sort, Sort{SortKind::Id, rs.kind_index, -1}))
              err(e.loc, ewhat + ": the id is " + sort_name(e.with_id->sort) + ", the kind wants " + sort_name(Sort{SortKind::Id, rs.kind_index, -1}));
          }
          break;
        }
        case EffectKind::Raise: {
          if (!res_of(&rs)) break;
          if (resolve_expr(e.value.get(), &env, Sort{SortKind::State, rs.kind_index, -1}) && e.value->sort.kind != SortKind::Int && e.value->sort.kind != SortKind::State)
            err(e.loc, ewhat + ": the level to raise to is " + sort_name(e.value->sort) + ", expected a state or an Int ordinal");
          break;
        }
        case EffectKind::Use: res_of(&rs); break;
        case EffectKind::StoreOut: {
          bool found = false;
          for (const OpParam& prm : op.params)
            if (prm.name == e.target && prm.role == "out") found = true;
          if (!found) err(e.loc, ewhat + ": '" + e.target + "' is not an out parameter of the operation");
          resolve_expr(e.value.get(), &env, simple(SortKind::Unknown));
          break;
        }
        case EffectKind::StoreLoc: {
          const int32_t li = p_.find_loc(e.target);
          if (li < 0) {
            err(e.loc, ewhat + ": unknown location '" + e.target + "'");
            break;
          }
          if (resolve_expr(e.value.get(), &env, simple(SortKind::Unknown)) &&
              !compatible(e.value->sort, p_.locs[static_cast<size_t>(li)].holds.resolved))
            err(e.loc, ewhat + ": the value is " + sort_name(e.value->sort) + ", the location holds " + sort_name(p_.locs[static_cast<size_t>(li)].holds.resolved));
          break;
        }
        case EffectKind::SetTag: {
          if (!res_of(&rs)) break;
          const KindDecl& k = p_.kinds[static_cast<size_t>(rs.kind_index)];
          const TagDecl* tag = nullptr;
          for (const TagDecl& t : k.tags)
            if (t.name == e.target) tag = &t;
          if (tag == nullptr) {
            err(e.loc, ewhat + ": kind '" + k.name + "' has no tag '" + e.target + "'");
            break;
          }
          if (std::find(tag->values.begin(), tag->values.end(), e.tag_value) == tag->values.end())
            err(e.loc, ewhat + ": '" + e.tag_value + "' is not a value of tag '" + e.target + "'");
          break;
        }
        case EffectKind::Mark:
          if (e.target != "handler_begin" && e.target != "handler_end") err(e.loc, ewhat + ": a mark is handler_begin or handler_end");
          break;
        case EffectKind::Sync: {
          bool ok = false;
          for (const char* r : sync_roles)
            if (e.target == r) ok = true;
          if (!ok) err(e.loc, ewhat + ": unknown synchronization role '" + e.target + "'");
          break;
        }
        case EffectKind::Env: break;
      }
    }
    // outcomes
    for (OutcomeDecl& oc : op.outcomes) {
      const std::string owhat = what + ", outcome '" + oc.code + "'";
      oc.code_const = p_.find_const(oc.code);
      if (oc.code_const < 0)
        err(oc.loc, owhat + ": the result code is not a declared constant");
      else if (p_.consts[static_cast<size_t>(oc.code_const)].value != ConstDecl::Value::Int)
        err(oc.loc, owhat + ": the result code must be an integer constant");
      if (oc.cls == OutcomeClass::NotReady) {
        oc.conjunct_index = -1;
        for (size_t i = 0; i < op.conjuncts.size(); ++i)
          if (op.conjuncts[i].label == oc.conjunct) oc.conjunct_index = static_cast<int32_t>(i);
        if (oc.conjunct_index < 0) err(oc.loc, owhat + ": not_ready(" + oc.conjunct + ") names no conjunct of the operation");
      }
      resolve_provs(oc.loc, oc.doc_prov, &oc.doc_prov_idx, owhat);
      if (!oc.documented.empty()) {
        if (oc.doc_prov.empty()) err(oc.loc, owhat + ": a documented result cites the text that documents it (prov ...)");
        for (int32_t pi : oc.doc_prov_idx)
          if (p_.provs[static_cast<size_t>(pi)].side != "supplier")
            err(oc.loc, owhat + ": the documentation of a result is a supplier-side record, '" + p_.provs[static_cast<size_t>(pi)].name + "' has side '" +
                            (p_.provs[static_cast<size_t>(pi)].side.empty() ? "n/a" : p_.provs[static_cast<size_t>(pi)].side) + "'");
      }
    }
    if (op.terminal.present && op.symbol.empty()) err(op.terminal.loc, what + ": a terminal operation needs 'bind symbol'");
    if (op.terminal.after.present) check_pattern(op.terminal.after, what + ", terminal");
  }
}

void Checker::check_inits() {
  for (InitDecl& d : p_.inits) {
    const std::string what = "INIT fact '" + d.name + "'";
    entry_common(&d, what);
    ExprEnv env;
    if (d.is_loc) {
      const int32_t li = p_.find_loc(d.loc_name);
      if (li < 0) {
        err(d.loc, what + ": unknown location '" + d.loc_name + "'");
        continue;
      }
      if (resolve_expr(d.value.get(), &env, simple(SortKind::Unknown)) && !compatible(d.value->sort, p_.locs[static_cast<size_t>(li)].holds.resolved))
        err(d.loc, what + ": the value is " + sort_name(d.value->sort) + ", the location holds " + sort_name(p_.locs[static_cast<size_t>(li)].holds.resolved));
    } else {
      if (!resolve_expr(d.res.get(), &env, simple(SortKind::Unknown))) continue;
      if (d.res->sort.kind != SortKind::Res) {
        err(d.loc, what + ": expected a resource, found " + sort_name(d.res->sort));
        continue;
      }
      const KindDecl& k = p_.kinds[static_cast<size_t>(d.res->sort.kind_index)];
      if (find_state(k, d.state) < 0) err(d.loc, what + ": '" + d.state + "' is not a state of kind '" + k.name + "'");
    }
  }
}

void Checker::check_rules() {
  for (HbRuleDecl& r : p_.rules) {
    const std::string what = "hb rule '" + r.name + "'";
    entry_common(&r, what);
    const bool s_ok = r.robust == Robust::Unconditional || r.robust == Robust::ObsFatal;
    const bool o_ok = r.robust == Robust::ObsSoft || r.robust == Robust::ScheduleChosen || r.robust == Robust::Assumed;
    if (r.stratum == Stratum::S && !s_ok) err(r.loc, what + ": a structural (S) rule is unconditional or obs_fatal, not " + robust_text(r.robust));
    if (r.stratum == Stratum::O && !o_ok) err(r.loc, what + ": an observed (O) rule is obs_soft, schedule_chosen or assumed, not " + robust_text(r.robust));
    if (r.stratum == Stratum::C && r.robust != Robust::ScheduleChosen)
      err(r.loc, what + ": a schedule constraint (C) is tagged schedule_chosen, and it is never a happens-before edge");
    auto known_op_or_site = [&](const std::string& n) { return p_.find_op(n) >= 0 || p_.find_site(n) >= 0; };
    switch (r.tmpl) {
      case RuleTemplate::ProgramOrder: break;
      case RuleTemplate::Spawn: {
        const int32_t oi = p_.find_op(r.create_op);
        if (oi < 0) {
          err(r.loc, what + ": the creating operation '" + r.create_op + "' is not declared");
        } else {
          bool out = false;
          for (const OpParam& prm : p_.ops[static_cast<size_t>(oi)].params)
            if (prm.name == r.create_out && prm.role == "out") out = true;
          if (!out) err(r.loc, what + ": '" + r.create_out + "' is not an out parameter of '" + r.create_op + "'");
        }
        if (!known_op_or_site(r.entry_op)) err(r.loc, what + ": the entry '" + r.entry_op + "' is neither a declared operation nor a site");
        break;
      }
      case RuleTemplate::Phase:
        if (!known_op_or_site(r.from_op)) err(r.loc, what + ": '" + r.from_op + "' is neither a declared operation nor a site");
        if (r.from_phase != "call" && r.from_phase != "return" && r.from_phase != "point") err(r.loc, what + ": the phase is call, return or point");
        break;
      case RuleTemplate::Observe: {
        if (r.poll_fn.empty()) err(r.loc, what + ": no polling function");
        check_pattern(r.called_at, what);
        if (!r.called_at.present) err(r.loc, what + ": no 'called_at' pattern");
        const int32_t c = p_.find_const(r.exit_outcome);
        if (c < 0 || p_.consts[static_cast<size_t>(c)].value != ConstDecl::Value::Int)
          err(r.loc, what + ": exit_outcome '" + r.exit_outcome + "' is not a declared integer constant");
        if (!r.else_terminal.empty() && !is_terminal_symbol(r.else_terminal))
          err(r.loc, what + ": else_terminal '" + r.else_terminal + "' is not a declared terminal operation");
        if (r.watch_kinds.empty()) err(r.loc, what + ": 'watch' names no kind (the kinds whose lifecycle effects write the awaited state)");
        for (const std::string& wk : r.watch_kinds)
          if (p_.find_kind(wk) < 0) err(r.loc, what + ": 'watch' names an undeclared kind '" + wk + "'");
        if (!r.predicate_from_code && r.predicate) {
          ExprEnv env;
          expect_bool(r.predicate.get(), &env);
        }
        if (r.robust == Robust::ObsFatal && r.else_terminal.empty()) err(r.loc, what + ": an obs_fatal rule names the terminal escape of the other branch (else_terminal)");
        break;
      }
    }
  }
}

void Checker::check_rspecs() {
  for (RspecDecl& r : p_.rspecs) {
    const std::string what = "rspec '" + r.name + "'";
    entry_common(&r, what);
    r.use_op_index = p_.find_op(r.use_op);
    if (r.use_op_index < 0) {
      err(r.loc, what + ": the operation '" + r.use_op + "' is not declared");
      continue;
    }
    OpDecl& op = p_.ops[static_cast<size_t>(r.use_op_index)];
    ExprEnv env = op_env(op, false, nullptr);
    if (!r.key) err(r.loc, what + ": no 'key' (the resource selector)");
    else if (resolve_expr(r.key.get(), &env, simple(SortKind::Unknown)) && r.key->sort.kind != SortKind::Res)
      err(r.key->loc, what + ": the selector must be a resource, found " + sort_name(r.key->sort));
    if (!r.requires_) {
      err(r.loc, what + ": no 'requires' (the readiness preconditions)");
    } else if (expect_bool(r.requires_.get(), &env)) {
      std::vector<const Expr*> conjuncts;
      if (r.requires_->kind == ExprKind::And)
        for (const ExprPtr& a : r.requires_->args) conjuncts.push_back(a.get());
      else
        conjuncts.push_back(r.requires_.get());
      std::set<std::string> seen;
      for (const Expr* c : conjuncts) {
        const std::string t = expr_text(*c);
        if (!seen.insert(t).second) err(c->loc, what + ": the readiness precondition '" + t + "' is listed twice");
        int matches = 0;
        for (const ConjunctDecl& oc : op.conjuncts)
          if (expr_text(*oc.expr) == t) ++matches;
        if (matches == 0)
          err(c->loc, what + ": the readiness precondition '" + t + "' has no linearization point: operation '" + op.name + "' declares no conjunct with this expression");
        else if (matches > 1)
          err(c->loc, what + ": the readiness precondition '" + t + "' matches several conjuncts of operation '" + op.name + "'");
      }
    }
    if (r.oracle_fns.empty()) warn(r.loc, what + " names no oracle function");
    if (r.appreq_unspecified) {
      r.appreq_idx = -1;
    } else {
      r.appreq_idx = p_.find_prov(r.appreq);
      if (r.appreq_idx < 0) err(r.loc, what + ": appreq names an unknown provenance record '" + r.appreq + "'");
      else if (p_.provs[static_cast<size_t>(r.appreq_idx)].side != "caller")
        err(r.loc, what + ": appreq must cite a caller-side record (side caller), '" + r.appreq + "' has side '" +
                       (p_.provs[static_cast<size_t>(r.appreq_idx)].side.empty() ? "n/a" : p_.provs[static_cast<size_t>(r.appreq_idx)].side) +
                       "'; a supplier-side text or a rule never states what the caller must achieve");
      else if (!caller_side_tier(p_.provs[static_cast<size_t>(r.appreq_idx)].tier))
        err(r.loc, what + ": appreq cites '" + r.appreq + "' of tier '" + p_.provs[static_cast<size_t>(r.appreq_idx)].tier +
                       "'; an appreq is a cited reference addressed to the caller, never a hand-entered or code-derived record (QT-9: no researcher-authored appreq)");
    }
    if (r.allowed_head.kind == AllowedKind::None && r.appreq_unspecified)
      note(r.loc, what + ": 'allowed none' without a specified appreq: the caller-level Allowed is unspecified_by_reference (SPEC 1.6)");
    for (AllowedEntry& a : r.allowed) {
      const std::string awhat = what + ", Allowed entry " + allowed_text(a.kind);
      resolve_provs(a.loc, a.prov, &a.prov_idx, awhat);
      if (a.prov.empty() && opt_.require_provenance) err(a.loc, awhat + " cites no provenance record (side supplier, caller or rule)");
      for (int32_t pi : a.prov_idx) {
        const std::string& side = p_.provs[static_cast<size_t>(pi)].side;
        if (side.empty()) err(a.loc, awhat + ": provenance '" + p_.provs[static_cast<size_t>(pi)].name + "' has no side (supplier, caller or rule)");
      }
      for (const std::string& c : a.codes) {
        const int32_t ci = p_.find_const(c);
        if (ci < 0 || p_.consts[static_cast<size_t>(ci)].value != ConstDecl::Value::Int) err(a.loc, awhat + ": '" + c + "' is not a declared integer constant");
      }
      if (a.kind == AllowedKind::RetryUntilReady) {
        const int32_t bi = p_.find_const(a.bound);
        if (bi < 0) err(a.loc, awhat + ": the bound '" + a.bound + "' is not a declared constant (the horizon comes from a reference)");
      }
      if (a.kind == AllowedKind::Fallback && p_.find_op(a.op) < 0) err(a.loc, awhat + ": the fallback operation '" + a.op + "' is not declared");
      if ((a.kind == AllowedKind::DocumentedDrop || a.kind == AllowedKind::DeferByDesign || a.kind == AllowedKind::Exit) && a.src.empty())
        err(a.loc, awhat + " names no citation text");
    }
    if (!r.harm.empty() && !in_vocabulary("KZ_HARM", r.harm))
      err(r.loc, what + ": unknown harm '" + r.harm + "' (expected one of: " + vocabulary_list("KZ_HARM") + ")");
    if (r.harm.empty()) warn(r.loc, what + " has no harm (the default is unspecified_by_reference)");
    if (r.authored.empty()) warn(r.loc, what + " has no 'authored' record");
  }
}

// Roles of call contexts (SPEC 1.6 "Call contexts"): the operation and the site exist, the site is a
// call of the operation's symbol, the purpose is one of the vocabulary, one role per site.  A role that
// takes calls out of a readiness specification (any purpose but dependent_use) must be justified by a
// source: at least one cited record whose tier is not researcher or unreferenced, so that a hand-entered
// claim alone never removes a call context (the rule against exclusions written to fit a result).
void Checker::check_roles() {
  std::map<std::string, std::string> by_site;
  for (RoleDecl& r : p_.roles) {
    const std::string what = "role '" + r.name + "'";
    entry_common(&r, what);
    r.op_index = p_.find_op(r.op);
    r.site_index = p_.find_site(r.site);
    if (r.op.empty()) err(r.loc, what + " names no operation (op NAME;)");
    else if (r.op_index < 0) err(r.loc, what + ": unknown operation '" + r.op + "'");
    if (r.site.empty()) err(r.loc, what + " names no site (site NAME;)");
    else if (r.site_index < 0) err(r.loc, what + ": unknown site '" + r.site + "'");
    if (r.purpose != "dependent_use" && r.purpose != "existence_probe")
      err(r.loc, what + ": purpose is dependent_use or existence_probe, found '" + r.purpose + "'");
    if (r.site_index >= 0) {
      const auto ins = by_site.emplace(r.site, r.name);
      if (!ins.second) err(r.loc, what + ": the site '" + r.site + "' already has the role '" + ins.first->second + "'");
      const Pattern& pat = p_.sites[static_cast<size_t>(r.site_index)].pattern;
      if (pat.op != PatOp::Call) err(r.loc, what + ": the site '" + r.site + "' is not a call pattern");
      else if (r.op_index >= 0) {
        const OpDecl& op = p_.ops[static_cast<size_t>(r.op_index)];
        const std::string sym = op.symbol.empty() ? op.name : op.symbol;
        if (pat.callee != sym) err(r.loc, what + ": the site '" + r.site + "' calls '" + pat.callee + "', the operation binds '" + sym + "'");
      }
    }
    if (r.purpose != "dependent_use" && !r.prov_idx.empty()) {
      bool sourced = false;
      for (int32_t pi : r.prov_idx) {
        if (pi < 0) continue;
        const std::string& t = p_.provs[static_cast<size_t>(pi)].tier;
        if (t != "researcher" && t != "unreferenced") sourced = true;
      }
      if (!sourced)
        err(r.loc, what + ": a role that takes calls out of a readiness specification cites a source (a record of tier other than researcher or unreferenced); hand-entered records alone do not justify it");
    }
  }
}

void Checker::check_trusted() {
  for (TrustedDecl& t : p_.trusted) {
    const std::string what = "trusted entry '" + t.name + "'";
    entry_common(&t, what);
    if (t.cls != "declared_trusted" && t.cls != "assumption") err(t.loc, what + ": class is declared_trusted or assumption, found '" + t.cls + "'");
    if (t.statement.empty()) err(t.loc, what + " has no statement");
    if (t.refutable_by.empty()) err(t.loc, what + ": a trusted entry is refutable, not proved: name the obligations that can refute it (refutable_by O9 ...)");
    for (const std::string& o : t.refutable_by)
      if (!is_obligation(o)) err(t.loc, what + ": '" + o + "' is not an obligation (O1 ... O10)");
  }
}

bool Checker::run() {
  if (opt_.require_header && !p_.has_header) err(SrcLoc{}, "the profile has no name (profile \"...\";)");
  check_provs();
  check_sources();
  check_consts();
  check_kinds();
  check_locs();
  check_context_scope();
  check_kind_exprs();
  check_binds();
  check_sites();
  check_ops();
  check_inits();
  check_rules();
  check_rspecs();
  check_trusted();
  check_roles();
  p_.checked = ok_;
  return ok_;
}

}  // namespace

bool check_profile(Profile* p, std::vector<Diag>* diags, const CheckOptions& options) {
  Checker c(p, diags, options);
  return c.run();
}

bool resolve_expression(const Profile& p, Expr* e, const ExprContext& ctx, Sort expect, std::vector<Diag>* diags) {
  // The checker only reads the profile for resolution; the const_cast keeps one code path.
  Checker c(const_cast<Profile*>(&p), diags, CheckOptions());
  return c.resolve_expr_public(e, ctx, expect);
}

Status load_profile(std::vector<SourceText> files, Profile* out, std::vector<Diag>* diags, const CheckOptions& options) {
  std::stable_sort(files.begin(), files.end(), [](const SourceText& a, const SourceText& b) { return a.name < b.name; });
  for (const SourceText& f : files) {
    const Status s = parse_text(f, out, diags);
    if (!s.ok()) return s;
  }
  if (!normalize_profile(out, diags)) return Status::error(Err::Invalid, format_diag(out->files, diags->back()));
  if (!check_profile(out, diags, options)) {
    for (const Diag& d : *diags)
      if (d.severity == Severity::Error) return Status::error(Err::Invalid, format_diag(out->files, d));
  }
  return Status::success();
}

}  // namespace phi
}  // namespace kz

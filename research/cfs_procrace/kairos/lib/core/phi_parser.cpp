// phi_parser.cpp -- recursive-descent parser of the profile language.
//
// One function per production.  Every function returns false after recording a diagnostic;
// the first error ends the parse (`P_TRY` propagates it).  The grammar is in
// docs/phi_reference.md section 3; the canonical printer (phi_print.cpp) writes exactly the
// forms accepted here.

#include <algorithm>
#include <cstdint>

#include "kz/core/phi.hpp"
#include "phi_lexer.hpp"

namespace kz {
namespace phi {

namespace {

#define P_TRY(expr)    \
  do {                 \
    if (!(expr)) return false; \
  } while (0)

struct Parser {
  const std::vector<Token>& toks;
  size_t pos = 0;
  Profile* prof;
  uint32_t file;
  std::vector<Diag>* diags;

  Parser(const std::vector<Token>& t, Profile* p, uint32_t f, std::vector<Diag>* d) : toks(t), prof(p), file(f), diags(d) {}

  // ---- token access -------------------------------------------------------------------
  const Token& cur() const { return toks[pos]; }
  const Token& at(size_t k) const { return toks[std::min(pos + k, toks.size() - 1)]; }
  bool is(Tok k) const { return cur().kind == k; }
  bool is_kw(const char* kw) const { return cur().kind == Tok::Ident && cur().text == kw; }
  bool is_kw_at(size_t k, const char* kw) const { return at(k).kind == Tok::Ident && at(k).text == kw; }
  void advance() {
    if (pos + 1 < toks.size()) ++pos;
  }
  bool accept(Tok k) {
    if (!is(k)) return false;
    advance();
    return true;
  }
  bool accept_kw(const char* kw) {
    if (!is_kw(kw)) return false;
    advance();
    return true;
  }

  std::string describe(const Token& t) const {
    switch (t.kind) {
      case Tok::Ident: return "identifier '" + t.text + "'";
      case Tok::Int: return "integer " + t.text;
      case Tok::Str: return "a string";
      default: return tok_spelling(t.kind);
    }
  }

  bool fail_at(const SrcLoc& loc, const std::string& msg) {
    Diag d;
    d.severity = Severity::Error;
    d.loc = loc;
    d.message = msg;
    diags->push_back(std::move(d));
    return false;
  }
  bool fail(const std::string& msg) { return fail_at(cur().loc, msg); }
  bool expected(const std::string& what) { return fail("expected " + what + ", found " + describe(cur())); }

  bool expect(Tok k, const char* context) {
    if (accept(k)) return true;
    return expected(std::string(tok_spelling(k)) + (context[0] != '\0' ? std::string(" ") + context : std::string()));
  }
  bool expect_kw(const char* kw, const char* context = "") {
    if (accept_kw(kw)) return true;
    return expected(std::string("'") + kw + "'" + (context[0] != '\0' ? std::string(" ") + context : std::string()));
  }
  bool expect_ident(std::string* out, const char* what, SrcLoc* loc = nullptr) {
    if (!is(Tok::Ident)) return expected(what);
    *out = cur().text;
    if (loc != nullptr) *loc = cur().loc;
    advance();
    return true;
  }
  bool expect_str(std::string* out, const char* what) {
    if (!is(Tok::Str)) return expected(what);
    *out = cur().text;
    advance();
    return true;
  }
  bool expect_int(int64_t* out, const char* what) {
    if (!is(Tok::Int)) return expected(what);
    *out = cur().ival;
    advance();
    return true;
  }
  bool expect_semi(const char* context) { return expect(Tok::Semi, context); }

  // ---- expressions ---------------------------------------------------------------------
  bool parse_expr(ExprPtr* out) { return parse_imp(out); }

  ExprPtr node(ExprKind k, const SrcLoc& loc) {
    ExprPtr e(new Expr());
    e->kind = k;
    e->loc = loc;
    return e;
  }

  bool parse_imp(ExprPtr* out) {
    ExprPtr lhs;
    P_TRY(parse_or(&lhs));
    if (is(Tok::FatArrow)) {
      const SrcLoc loc = cur().loc;
      advance();
      ExprPtr rhs;
      P_TRY(parse_imp(&rhs));
      ExprPtr e = node(ExprKind::Imp, loc);
      e->args.push_back(std::move(lhs));
      e->args.push_back(std::move(rhs));
      *out = std::move(e);
      return true;
    }
    *out = std::move(lhs);
    return true;
  }

  static void append_flat(ExprKind kind, std::vector<ExprPtr>* list, ExprPtr x) {
    if (x->kind == kind) {
      for (ExprPtr& c : x->args) list->push_back(std::move(c));
    } else {
      list->push_back(std::move(x));
    }
  }

  bool parse_or(ExprPtr* out) {
    ExprPtr first;
    P_TRY(parse_and(&first));
    if (!is(Tok::OrOr)) {
      *out = std::move(first);
      return true;
    }
    const SrcLoc loc = first->loc;
    std::vector<ExprPtr> list;
    append_flat(ExprKind::Or, &list, std::move(first));
    while (accept(Tok::OrOr)) {
      ExprPtr next;
      P_TRY(parse_and(&next));
      append_flat(ExprKind::Or, &list, std::move(next));
    }
    ExprPtr e = node(ExprKind::Or, loc);
    e->args = std::move(list);
    *out = std::move(e);
    return true;
  }

  bool parse_and(ExprPtr* out) {
    ExprPtr first;
    P_TRY(parse_not(&first));
    if (!is(Tok::AndAnd)) {
      *out = std::move(first);
      return true;
    }
    const SrcLoc loc = first->loc;
    std::vector<ExprPtr> list;
    append_flat(ExprKind::And, &list, std::move(first));
    while (accept(Tok::AndAnd)) {
      ExprPtr next;
      P_TRY(parse_not(&next));
      append_flat(ExprKind::And, &list, std::move(next));
    }
    ExprPtr e = node(ExprKind::And, loc);
    e->args = std::move(list);
    *out = std::move(e);
    return true;
  }

  bool parse_not(ExprPtr* out) {
    if (is(Tok::Bang)) {
      const SrcLoc loc = cur().loc;
      advance();
      ExprPtr x;
      P_TRY(parse_not(&x));
      ExprPtr e = node(ExprKind::Not, loc);
      e->args.push_back(std::move(x));
      *out = std::move(e);
      return true;
    }
    return parse_cmp(out);
  }

  bool parse_cmp(ExprPtr* out) {
    ExprPtr lhs;
    P_TRY(parse_term(&lhs));
    CmpOp op = CmpOp::Eq;
    bool is_cmp = true;
    switch (cur().kind) {
      case Tok::Eq:
      case Tok::EqEq: op = CmpOp::Eq; break;
      case Tok::Ne: op = CmpOp::Ne; break;
      case Tok::Lt: op = CmpOp::Lt; break;
      case Tok::Le: op = CmpOp::Le; break;
      case Tok::Gt: op = CmpOp::Gt; break;
      case Tok::Ge: op = CmpOp::Ge; break;
      default: is_cmp = false; break;
    }
    if (is_cmp) {
      const SrcLoc loc = cur().loc;
      advance();
      ExprPtr rhs;
      P_TRY(parse_term(&rhs));
      ExprPtr e = node(ExprKind::Cmp, loc);
      e->cmp = op;
      e->args.push_back(std::move(lhs));
      e->args.push_back(std::move(rhs));
      *out = std::move(e);
      return true;
    }
    if (is_kw("in")) {
      const SrcLoc loc = cur().loc;
      advance();
      P_TRY(expect(Tok::LBrace, "after 'in'"));
      ExprPtr e = node(ExprKind::In, loc);
      e->args.push_back(std::move(lhs));
      if (is(Tok::RBrace)) return fail("an 'in' set needs at least one element");
      while (true) {
        ExprPtr el;
        P_TRY(parse_term(&el));
        e->args.push_back(std::move(el));
        if (!accept(Tok::Comma)) break;
      }
      P_TRY(expect(Tok::RBrace, "to close the 'in' set"));
      *out = std::move(e);
      return true;
    }
    *out = std::move(lhs);
    return true;
  }

  bool parse_term(ExprPtr* out) {
    const Token& t = cur();
    if (t.kind == Tok::Int) {
      ExprPtr e = node(ExprKind::IntLit, t.loc);
      e->ival = t.ival;
      advance();
      *out = std::move(e);
      return true;
    }
    if (t.kind == Tok::Str) {
      ExprPtr e = node(ExprKind::StrLit, t.loc);
      e->text = t.text;
      advance();
      *out = std::move(e);
      return true;
    }
    if (t.kind == Tok::LParen) {
      advance();
      ExprPtr inner;
      P_TRY(parse_expr(&inner));
      P_TRY(expect(Tok::RParen, "to close the parenthesis"));
      *out = std::move(inner);
      return true;
    }
    if (t.kind != Tok::Ident) return expected("an expression");
    if (t.text == "true" || t.text == "false") {
      ExprPtr e = node(ExprKind::BoolLit, t.loc);
      e->bval = t.text == "true";
      advance();
      *out = std::move(e);
      return true;
    }
    if ((t.text == "forall" || t.text == "exists") && at(1).kind == Tok::Ident && at(2).kind == Tok::Colon) return parse_quant(out);
    const SrcLoc loc = t.loc;
    const std::string name = t.text;
    advance();
    if (!is(Tok::LParen)) {
      ExprPtr e = node(ExprKind::Name, loc);
      e->text = name;
      *out = std::move(e);
      return true;
    }
    advance();  // '('
    ExprPtr e = node(ExprKind::Call, loc);
    e->text = name;
    if (name == "state") e->fn = CallFn::State;
    else if (name == "key") e->fn = CallFn::Key;
    else if (name == "gen") e->fn = CallFn::Gen;
    else if (name == "valid") e->fn = CallFn::Valid;
    else if (name == "value") e->fn = CallFn::Value;
    else if (name == "tag") e->fn = CallFn::Tag;
    else if (name == "id") e->fn = CallFn::Id;
    else if (name == "owner") e->fn = CallFn::Owner;
    else e->fn = CallFn::Res;
    if (!is(Tok::RParen)) {
      while (true) {
        ExprPtr a;
        P_TRY(parse_expr(&a));
        e->args.push_back(std::move(a));
        if (!accept(Tok::Comma)) break;
      }
    }
    P_TRY(expect(Tok::RParen, "to close the argument list"));
    *out = std::move(e);
    return true;
  }

  bool parse_quant(ExprPtr* out) {
    const SrcLoc loc = cur().loc;
    ExprPtr e = node(ExprKind::Quant, loc);
    e->quant = cur().text == "forall" ? QuantKind::Forall : QuantKind::Exists;
    advance();
    P_TRY(expect_ident(&e->text, "a variable name"));
    P_TRY(expect(Tok::Colon, "after the quantified variable"));
    P_TRY(expect_ident(&e->kind_name, "a kind name"));
    ExprPtr guard;
    if (accept_kw("where")) P_TRY(parse_expr(&guard));
    P_TRY(expect(Tok::LBrace, "before the body of the quantifier"));
    ExprPtr body;
    P_TRY(parse_expr(&body));
    P_TRY(expect(Tok::RBrace, "after the body of the quantifier"));
    e->args.push_back(std::move(guard));  // may be null
    e->args.push_back(std::move(body));
    *out = std::move(e);
    return true;
  }

  // ---- sorts, patterns, provenance clauses ------------------------------------------------
  bool parse_sort(SortSpec* s) {
    s->loc = cur().loc;
    std::string name;
    P_TRY(expect_ident(&name, "a sort (Bool, Int, Str, Actor, Loc, Gen or Id<kind>)"));
    if (name == "Bool") s->kind = SortKind::Bool;
    else if (name == "Int" || name == "Gen") s->kind = SortKind::Int;
    else if (name == "Str") s->kind = SortKind::Str;
    else if (name == "Actor") s->kind = SortKind::Actor;
    else if (name == "Loc") s->kind = SortKind::Loc;
    else if (name == "Id" || name == "Res") {
      s->kind = name == "Id" ? SortKind::Id : SortKind::Res;
      P_TRY(expect(Tok::Lt, name == "Id" ? "after 'Id'" : "after 'Res'"));
      P_TRY(expect_ident(&s->ref, "a kind name"));
      P_TRY(expect(Tok::Gt, name == "Id" ? "to close Id<...>" : "to close Res<...>"));
    } else {
      return fail_at(s->loc, "unknown sort '" + name + "' (expected Bool, Int, Str, Actor, Loc, Gen, Id<kind> or Res<kind>)");
    }
    return true;
  }

  bool parse_pattern(Pattern* p) {
    p->present = true;
    p->loc = cur().loc;
    P_TRY(expect_kw("fn", "to start a pattern"));
    P_TRY(expect(Tok::LParen, "after 'fn'"));
    P_TRY(expect_str(&p->fn, "the name of the function as a string"));
    P_TRY(expect(Tok::RParen, "after the function name"));
    P_TRY(expect(Tok::Dot, "before the pattern operation"));
    std::string op;
    const SrcLoc oploc = cur().loc;
    P_TRY(expect_ident(&op, "a pattern operation (call, store, memset, load, loop or return)"));
    if (op == "call") {
      p->op = PatOp::Call;
      P_TRY(expect(Tok::LParen, "after 'call'"));
      P_TRY(expect_str(&p->callee, "the name of the callee as a string"));
      while (accept(Tok::Comma)) {
        ArgPred a;
        P_TRY(expect_kw("arg", "to start an argument predicate"));
        P_TRY(expect(Tok::LBracket, "after 'arg'"));
        int64_t idx = 0;
        P_TRY(expect_int(&idx, "an argument index"));
        if (idx < 0 || idx > 255) return fail("argument index out of range (0..255)");
        a.index = static_cast<uint32_t>(idx);
        P_TRY(expect(Tok::RBracket, "after the argument index"));
        if (is(Tok::EqEq) || is(Tok::Ne)) {
          a.cmp = is(Tok::EqEq) ? ArgCmp::Eq : ArgCmp::Ne;
          advance();
          if (is(Tok::Int)) {
            a.ival = cur().ival;
            advance();
          } else if (is(Tok::Ident)) {
            a.is_name = true;
            a.name = cur().text;
            advance();
          } else {
            return expected("an integer or a constant name");
          }
        } else if (accept_kw("is")) {
          P_TRY(expect_kw("addr", "after 'is'"));
          P_TRY(expect(Tok::LParen, "after 'addr'"));
          a.cmp = ArgCmp::AddrOf;
          P_TRY(expect_ident(&a.name, "Type.field"));
          P_TRY(expect(Tok::RParen, "after addr(...)"));
        } else if (accept_kw("any")) {
          a.cmp = ArgCmp::Any;
        } else {
          return expected("'==', '!=', 'is addr(...)' or 'any' in an argument predicate");
        }
        p->args.push_back(std::move(a));
      }
      P_TRY(expect(Tok::RParen, "to close call(...)"));
    } else if (op == "store" || op == "load") {
      p->op = op == "store" ? PatOp::Store : PatOp::Load;
      P_TRY(expect(Tok::LParen, "after the pattern operation"));
      std::string tf;
      P_TRY(expect_ident(&tf, "Type.field"));
      const size_t dot = tf.rfind('.');
      if (dot == std::string::npos || dot == 0 || dot + 1 == tf.size()) return fail("expected Type.field in " + op + "(...)");
      p->type_name = tf.substr(0, dot);
      p->field = tf.substr(dot + 1);
      P_TRY(expect(Tok::RParen, "to close the pattern operation"));
    } else if (op == "memset") {
      p->op = PatOp::Memset;
      P_TRY(expect(Tok::LParen, "after 'memset'"));
      P_TRY(expect_ident(&p->type_name, "a type name"));
      P_TRY(expect(Tok::RParen, "to close memset(...)"));
    } else if (op == "loop") {
      p->op = PatOp::LoopPoll;
      P_TRY(expect(Tok::LParen, "after 'loop'"));
      P_TRY(expect_kw("poll", "in loop(...)"));
      P_TRY(expect(Tok::RParen, "to close loop(...)"));
    } else if (op == "return") {
      p->op = PatOp::Return;
    } else {
      return fail_at(oploc, "unknown pattern operation '" + op + "' (expected call, store, memset, load, loop or return)");
    }
    if (accept_kw("nth")) {
      int64_t k = 0;
      P_TRY(expect_int(&k, "the ordinal of the match (1 or more)"));
      if (k < 1) return fail("the ordinal after 'nth' counts from 1");
      p->nth = static_cast<uint32_t>(k);
    }
    if (accept_kw("macro")) {
      std::string m;
      const SrcLoc mloc = cur().loc;
      P_TRY(expect_ident(&m, "yes, no or any"));
      if (m == "yes") p->macro = MacroFlag::Yes;
      else if (m == "no") p->macro = MacroFlag::No;
      else if (m == "any") p->macro = MacroFlag::Any;
      else return fail_at(mloc, "expected yes, no or any after 'macro', found '" + m + "'");
    }
    return true;
  }

  bool parse_name_list(std::vector<std::string>* out, const char* what) {
    while (true) {
      std::string n;
      P_TRY(expect_ident(&n, what));
      out->push_back(std::move(n));
      if (!accept(Tok::Comma)) break;
    }
    return true;
  }

  bool parse_guard(Guard* g) {
    g->loc = cur().loc;
    P_TRY(expect_ident(&g->component, "the name of a source"));
    switch (cur().kind) {
      case Tok::Ge: g->op = CmpOp::Ge; break;
      case Tok::Gt: g->op = CmpOp::Gt; break;
      case Tok::Le: g->op = CmpOp::Le; break;
      case Tok::Lt: g->op = CmpOp::Lt; break;
      case Tok::Eq:
      case Tok::EqEq: g->op = CmpOp::Eq; break;
      case Tok::Ne: g->op = CmpOp::Ne; break;
      default: return expected("a comparison operator in a version guard");
    }
    advance();
    P_TRY(expect_str(&g->version, "a version string"));
    return true;
  }

  // prov A, B | guard C >= "v" | note "text": the clauses every claim may carry.
  bool is_common_clause() const { return is_kw("prov") || is_kw("guard") || is_kw("note"); }
  bool parse_common_clause(EntryBase* e) {
    if (accept_kw("prov")) return parse_name_list(&e->prov, "the name of a provenance record");
    if (accept_kw("guard")) {
      Guard g;
      P_TRY(parse_guard(&g));
      e->guards.push_back(std::move(g));
      return true;
    }
    if (accept_kw("note")) return expect_str(&e->note, "the note as a string");
    return fail("internal: not a common clause");
  }

  // ---- declarations ---------------------------------------------------------------------
  bool parse_file_body() {
    P_TRY(expect_kw("phi", "at the start of the file"));
    int64_t ver = 0;
    const SrcLoc vloc = cur().loc;
    P_TRY(expect_int(&ver, "the language version"));
    if (ver != kLanguageVersion)
      return fail_at(vloc, "unsupported language version " + std::to_string(ver) + " (this build reads version " + std::to_string(kLanguageVersion) + ")");
    P_TRY(expect_semi("after the language version"));
    while (!is(Tok::End)) P_TRY(parse_decl());
    return true;
  }

  bool parse_decl() {
    if (!is(Tok::Ident)) return expected("a declaration");
    const std::string kw = cur().text;
    const SrcLoc loc = cur().loc;
    advance();
    if (kw == "profile") return parse_profile_decl(loc);
    if (kw == "source") return parse_source(loc);
    if (kw == "const") return parse_const(loc);
    if (kw == "context") return parse_context(loc);
    if (kw == "scope") return parse_scope(loc);
    if (kw == "kind") return parse_kind(loc);
    if (kw == "loc") return parse_loc(loc);
    if (kw == "bind") return parse_bind(loc);
    if (kw == "site") return parse_site(loc);
    if (kw == "op") return parse_op(loc);
    if (kw == "init") return parse_init(loc);
    if (kw == "hb") return parse_hb(loc);
    if (kw == "rspec") return parse_rspec(loc);
    if (kw == "trusted") return parse_trusted(loc);
    if (kw == "prov") return parse_prov(loc);
    return fail_at(loc, "unknown declaration '" + kw +
                            "' (expected profile, source, const, context, scope, kind, loc, bind, site, op, init, hb, rspec, trusted or prov)");
  }

  bool parse_profile_decl(const SrcLoc& loc) {
    if (prof->has_header) return fail_at(loc, "the profile name is declared twice");
    P_TRY(expect_str(&prof->name, "the profile name as a string"));
    if (accept_kw("revision")) P_TRY(expect_str(&prof->revision, "the revision as a string"));
    prof->has_header = true;
    return expect_semi("after the profile declaration");
  }

  bool parse_source(const SrcLoc& loc) {
    SourceDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the source"));
    P_TRY(expect_kw("root", "in a source declaration"));
    P_TRY(expect_str(&d.root, "the root path as a string"));
    P_TRY(expect_kw("rev", "in a source declaration"));
    P_TRY(expect_str(&d.rev, "the revision as a string"));
    while (is_common_clause()) P_TRY(parse_common_clause(&d));
    P_TRY(expect_semi("after the source declaration"));
    prof->sources.push_back(std::move(d));
    return true;
  }

  bool parse_const(const SrcLoc& loc) {
    ConstDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the constant"));
    P_TRY(expect(Tok::Eq, "after the constant name"));
    if (is(Tok::Int)) {
      d.value = ConstDecl::Value::Int;
      d.ival = cur().ival;
      advance();
    } else if (is(Tok::Str)) {
      d.value = ConstDecl::Value::Str;
      d.sval = cur().text;
      advance();
    } else if (is_kw("true") || is_kw("false")) {
      d.value = ConstDecl::Value::Bool;
      d.bval = cur().text == "true";
      advance();
    } else if (accept_kw("ref")) {
      d.value = ConstDecl::Value::RefEval;
      P_TRY(expect_kw("eval", "after 'ref'"));
      P_TRY(expect_str(&d.ref_symbol, "the symbol to evaluate as a string"));
      P_TRY(expect_kw("in", "in a ref eval constant"));
      P_TRY(expect_str(&d.ref_file, "the translation unit as a string"));
    } else {
      return expected("an integer, a string, true, false or 'ref eval ...' as the value of the constant");
    }
    while (is_common_clause()) P_TRY(parse_common_clause(&d));
    P_TRY(expect_semi("after the constant"));
    prof->consts.push_back(std::move(d));
    return true;
  }

  bool parse_context(const SrcLoc& loc) {
    if (prof->context.present) return fail_at(loc, "the context is declared twice");
    ContextDecl& c = prof->context;
    c.present = true;
    c.loc = loc;
    P_TRY(expect(Tok::LBrace, "after 'context'"));
    while (!accept(Tok::RBrace)) {
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the context"));
      if (kw == "owner_kind") {
        P_TRY(expect_ident(&c.owner_kind, "a kind name"));
      } else if (kw == "prov") {
        P_TRY(parse_name_list(&c.prov, "the name of a provenance record"));
      } else {
        return fail_at(kloc, "unknown item '" + kw + "' in context (expected owner_kind or prov)");
      }
      P_TRY(expect_semi("after the context item"));
    }
    return true;
  }

  bool parse_scope(const SrcLoc& loc) {
    if (prof->scope.present) return fail_at(loc, "the scope is declared twice");
    ScopeDecl& s = prof->scope;
    s.present = true;
    s.loc = loc;
    P_TRY(expect(Tok::LBrace, "after 'scope'"));
    while (!accept(Tok::RBrace)) {
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the scope"));
      if (kw == "kind") P_TRY(expect_ident(&s.kind, "a kind name"));
      else if (kw == "request") P_TRY(expect_ident(&s.request, "a state name"));
      else if (kw == "teardown") P_TRY(expect_ident(&s.teardown, "a state name"));
      else if (kw == "prov") P_TRY(parse_name_list(&s.prov, "the name of a provenance record"));
      else return fail_at(kloc, "unknown item '" + kw + "' in scope (expected kind, request, teardown or prov)");
      P_TRY(expect_semi("after the scope item"));
    }
    return true;
  }

  bool parse_kind(const SrcLoc& loc) {
    KindDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the kind"));
    P_TRY(expect(Tok::LParen, "after the kind name"));
    if (!is(Tok::RParen)) {
      while (true) {
        KindParam p;
        P_TRY(expect_ident(&p.name, "a parameter name"));
        P_TRY(expect(Tok::Colon, "after the parameter name"));
        P_TRY(parse_sort(&p.sort));
        d.params.push_back(std::move(p));
        if (!accept(Tok::Comma)) break;
      }
    }
    P_TRY(expect(Tok::RParen, "to close the parameter list"));
    P_TRY(expect(Tok::LBrace, "before the body of the kind"));
    while (!accept(Tok::RBrace)) {
      if (is_common_clause()) {
        P_TRY(parse_common_clause(&d));
        P_TRY(expect_semi("after the clause"));
        continue;
      }
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the kind"));
      if (kw == "states") {
        while (true) {
          std::string s;
          P_TRY(expect_ident(&s, "a state name"));
          d.states.push_back(std::move(s));
          if (!accept(Tok::Lt)) break;
        }
      } else if (kw == "initial") {
        P_TRY(expect_ident(&d.initial, "a state name"));
      } else if (kw == "scope") {
        P_TRY(expect_ident(&d.scope, "process or persistent"));
      } else if (kw == "table") {
        P_TRY(expect_ident(&d.table, "the name of a constant"));
      } else if (kw == "ids") {
        d.ids.present = true;
        d.ids.loc = kloc;
        P_TRY(expect(Tok::LBrace, "after 'ids'"));
        while (!accept(Tok::RBrace)) {
          std::string k;
          const SrcLoc iloc = cur().loc;
          P_TRY(expect_ident(&k, "undefined, reserved, base or serial_max"));
          std::string* dst = nullptr;
          if (k == "undefined") dst = &d.ids.undefined_c;
          else if (k == "reserved") dst = &d.ids.reserved_c;
          else if (k == "base") dst = &d.ids.base_c;
          else if (k == "serial_max") dst = &d.ids.serial_max_c;
          else return fail_at(iloc, "unknown item '" + k + "' in ids (expected undefined, reserved, base or serial_max)");
          P_TRY(expect_ident(dst, "the name of an integer constant"));
          P_TRY(expect_semi("after the ids item"));
        }
        continue;  // the block closes with '}' and has no ';'
      } else if (kw == "tag") {
        TagDecl t;
        t.loc = kloc;
        P_TRY(expect_ident(&t.name, "the name of the tag"));
        P_TRY(expect(Tok::LBrace, "after the tag name"));
        P_TRY(parse_name_list(&t.values, "a tag value"));
        P_TRY(expect(Tok::RBrace, "to close the tag values"));
        d.tags.push_back(std::move(t));
      } else if (kw == "transition") {
        TransitionDecl t;
        t.loc = kloc;
        P_TRY(expect_ident(&t.from, "a state name"));
        P_TRY(expect(Tok::Arrow, "between the states of a transition"));
        P_TRY(expect_ident(&t.to, "a state name"));
        P_TRY(expect_kw("on", "in a transition"));
        P_TRY(expect_ident(&t.action, "the name of the action"));
        d.transitions.push_back(std::move(t));
      } else if (kw == "derived") {
        DerivedDecl dd;
        dd.loc = kloc;
        P_TRY(expect_ident(&dd.state, "a state name"));
        P_TRY(expect_kw("when", "in a derived rule"));
        P_TRY(parse_expr(&dd.when));
        d.derived.push_back(std::move(dd));
      } else {
        return fail_at(kloc, "unknown item '" + kw + "' in kind (expected states, initial, scope, table, ids, tag, transition, derived, prov, guard or note)");
      }
      P_TRY(expect_semi("after the kind item"));
    }
    prof->kinds.push_back(std::move(d));
    return true;
  }

  bool parse_loc(const SrcLoc& loc) {
    LocDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the location"));
    P_TRY(expect(Tok::LBrace, "after the location name"));
    bool has_holds = false;
    while (!accept(Tok::RBrace)) {
      if (is_common_clause()) {
        P_TRY(parse_common_clause(&d));
        P_TRY(expect_semi("after the clause"));
        continue;
      }
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the location"));
      if (kw == "struct") P_TRY(expect_str(&d.struct_name, "the struct type as a string"));
      else if (kw == "field") P_TRY(expect_str(&d.field, "the field name as a string"));
      else if (kw == "holds") {
        P_TRY(parse_sort(&d.holds));
        has_holds = true;
      } else return fail_at(kloc, "unknown item '" + kw + "' in loc (expected struct, field, holds, prov, guard or note)");
      P_TRY(expect_semi("after the location item"));
    }
    if (!has_holds) return fail_at(loc, "location '" + d.name + "' has no 'holds' sort");
    prof->locs.push_back(std::move(d));
    return true;
  }

  bool parse_bind(const SrcLoc& loc) {
    BindDecl d;
    d.loc = loc;
    P_TRY(expect_kw("field", "after 'bind'"));
    P_TRY(expect_str(&d.struct_name, "the struct type as a string"));
    P_TRY(expect_str(&d.field, "the field name as a string"));
    P_TRY(expect_kw("as", "in a binding"));
    P_TRY(parse_expr(&d.target));
    while (is_common_clause()) P_TRY(parse_common_clause(&d));
    P_TRY(expect_semi("after the binding"));
    d.name = d.struct_name + "." + d.field;
    prof->binds.push_back(std::move(d));
    return true;
  }

  bool parse_site(const SrcLoc& loc) {
    SiteDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the site"));
    P_TRY(expect(Tok::Eq, "after the site name"));
    P_TRY(parse_pattern(&d.pattern));
    while (is_common_clause()) P_TRY(parse_common_clause(&d));
    P_TRY(expect_semi("after the site"));
    prof->sites.push_back(std::move(d));
    return true;
  }

  bool parse_lp(LpSpec* lp) {
    lp->loc = cur().loc;
    if (accept_kw("capture")) {
      lp->kind = LpKind::Capture;
    } else if (accept_kw("at")) {
      lp->kind = LpKind::At;
      P_TRY(parse_pattern(&lp->pattern));
    } else if (accept_kw("site")) {
      lp->kind = LpKind::Site;
      P_TRY(expect_ident(&lp->site, "the name of a site"));
    } else {
      return expected("'capture', 'at <pattern>' or 'site <name>' after 'lp'");
    }
    if (accept_kw("values")) {
      while (true) {
        OpParam v;
        P_TRY(expect_ident(&v.name, "the name of a decoded value"));
        P_TRY(expect(Tok::Colon, "after the name of a decoded value"));
        v.has_sort = true;
        P_TRY(parse_sort(&v.sort));
        lp->values.push_back(std::move(v));
        if (!accept(Tok::Comma)) break;
      }
    }
    return true;
  }

  bool parse_op(const SrcLoc& loc) {
    OpDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the operation"));
    P_TRY(expect(Tok::LParen, "after the operation name"));
    if (!is(Tok::RParen)) {
      while (true) {
        OpParam p;
        static const char* const roles[] = {"out", "stamp", "key", "id", "buffer", "msg"};
        bool role = false;
        if (is(Tok::Ident) && at(1).kind == Tok::Ident)
          for (const char* r : roles)
            if (cur().text == r) role = true;
        if (role) {
          p.role = cur().text;
          advance();
        }
        P_TRY(expect_ident(&p.name, "a parameter name"));
        if (accept(Tok::Colon)) {
          p.has_sort = true;
          P_TRY(parse_sort(&p.sort));
        }
        d.params.push_back(std::move(p));
        if (!accept(Tok::Comma)) break;
      }
    }
    P_TRY(expect(Tok::RParen, "to close the parameter list"));
    P_TRY(expect(Tok::LBrace, "before the body of the operation"));
    while (!accept(Tok::RBrace)) {
      if (is_common_clause()) {
        P_TRY(parse_common_clause(&d));
        P_TRY(expect_semi("after the clause"));
        continue;
      }
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the operation"));
      if (kw == "bind") {
        P_TRY(expect_kw("symbol", "after 'bind'"));
        P_TRY(expect_str(&d.symbol, "the symbol as a string"));
      } else if (kw == "conjunct") {
        ConjunctDecl c;
        c.loc = kloc;
        P_TRY(expect_ident(&c.label, "the label of the conjunct"));
        P_TRY(expect(Tok::Colon, "after the conjunct label"));
        P_TRY(parse_expr(&c.expr));
        P_TRY(expect_kw("lp", "to give the linearization point of the conjunct"));
        P_TRY(parse_lp(&c.lp));
        if (accept_kw("prov")) P_TRY(parse_name_list(&c.prov, "the name of a provenance record"));
        d.conjuncts.push_back(std::move(c));
      } else if (kw == "effect") {
        EffectDecl e;
        P_TRY(parse_effect(&e, kloc));
        d.effects.push_back(std::move(e));
      } else if (kw == "outcome") {
        OutcomeDecl o;
        o.loc = kloc;
        P_TRY(expect_ident(&o.code, "the name of a result code"));
        P_TRY(expect(Tok::Colon, "after the result code"));
        std::string cls;
        const SrcLoc cloc = cur().loc;
        P_TRY(expect_ident(&cls, "ready, error or not_ready(<conjunct>)"));
        if (cls == "ready") {
          o.cls = OutcomeClass::Ready;
        } else if (cls == "error") {
          o.cls = OutcomeClass::Error;
        } else if (cls == "not_ready") {
          o.cls = OutcomeClass::NotReady;
          P_TRY(expect(Tok::LParen, "after 'not_ready'"));
          P_TRY(expect_ident(&o.conjunct, "the label of a conjunct"));
          P_TRY(expect(Tok::RParen, "after the conjunct label"));
        } else {
          return fail_at(cloc, "unknown outcome class '" + cls + "' (expected ready, error or not_ready(<conjunct>))");
        }
        if (accept_kw("documented")) {
          P_TRY(expect_str(&o.documented, "the documented result as a string"));
          if (accept_kw("prov")) P_TRY(parse_name_list(&o.doc_prov, "the name of a provenance record"));
        }
        d.outcomes.push_back(std::move(o));
      } else if (kw == "terminal") {
        d.terminal.present = true;
        d.terminal.loc = kloc;
        if (accept_kw("process_terminal")) d.terminal.process_terminal = true;
        if (accept_kw("after")) P_TRY(parse_pattern(&d.terminal.after));
      } else {
        return fail_at(kloc, "unknown item '" + kw +
                                 "' in operation (expected bind, conjunct, effect, outcome, terminal, prov, guard or note)");
      }
      P_TRY(expect_semi("after the operation item"));
    }
    prof->ops.push_back(std::move(d));
    return true;
  }

  bool parse_effect(EffectDecl* e, const SrcLoc& loc) {
    e->loc = loc;
    P_TRY(expect_ident(&e->label, "the label of the effect"));
    P_TRY(expect(Tok::Colon, "after the effect label"));
    std::string kw;
    const SrcLoc kloc = cur().loc;
    P_TRY(expect_ident(&kw, "an effect (lifecycle, use, store, settag, mark, sync or env)"));
    if (kw == "lifecycle") {
      if (is_kw("all") && at(1).kind == Tok::Ident && at(2).kind == Tok::Arrow) {
        advance();
        P_TRY(expect_ident(&e->all_kind, "a kind name"));
      } else {
        P_TRY(parse_expr(&e->res));
      }
      if (accept(Tok::Arrow)) {
        e->kind = EffectKind::Set;
        P_TRY(expect_ident(&e->to_state, "the target state"));
        if (accept_kw("id")) P_TRY(parse_expr(&e->with_id));
      } else if (e->res && accept_kw("raise")) {
        e->kind = EffectKind::Raise;
        P_TRY(parse_expr(&e->value));
      } else {
        return expected("'->' or 'raise' in a lifecycle effect");
      }
    } else if (kw == "use") {
      e->kind = EffectKind::Use;
      P_TRY(parse_expr(&e->res));
    } else if (kw == "store") {
      if (accept(Tok::Star)) {
        e->kind = EffectKind::StoreOut;
      } else {
        e->kind = EffectKind::StoreLoc;
      }
      P_TRY(expect_ident(&e->target, e->kind == EffectKind::StoreOut ? "an out parameter" : "a location name"));
      P_TRY(expect(Tok::Assign, "in a store effect"));
      P_TRY(parse_expr(&e->value));
    } else if (kw == "settag") {
      e->kind = EffectKind::SetTag;
      P_TRY(expect_ident(&e->target, "the tag name"));
      P_TRY(expect_kw("of", "in a settag effect"));
      P_TRY(parse_expr(&e->res));
      P_TRY(expect(Tok::Assign, "in a settag effect"));
      P_TRY(expect_ident(&e->tag_value, "the tag value"));
    } else if (kw == "mark") {
      e->kind = EffectKind::Mark;
      P_TRY(expect_ident(&e->target, "handler_begin or handler_end"));
    } else if (kw == "sync") {
      e->kind = EffectKind::Sync;
      P_TRY(expect_ident(&e->target, "the synchronization role"));
      P_TRY(expect_str(&e->sync_object, "the synchronization object as a string"));
    } else if (kw == "env") {
      e->kind = EffectKind::Env;
      P_TRY(expect_str(&e->target, "the environment input as a string"));
    } else {
      return fail_at(kloc, "unknown effect '" + kw + "' (expected lifecycle, use, store, settag, mark, sync or env)");
    }
    if (accept_kw("when")) P_TRY(parse_expr(&e->when));
    P_TRY(expect_kw("lp", "to give the linearization point of the effect"));
    P_TRY(parse_lp(&e->lp));
    while (true) {
      if (accept_kw("status")) {
        P_TRY(expect_ident(&e->status, "declared_trusted or declared_verified"));
      } else if (accept_kw("refutable_by")) {
        P_TRY(parse_name_list(&e->refutable_by, "an obligation (O1 ... O10)"));
      } else if (accept_kw("prov")) {
        P_TRY(parse_name_list(&e->prov, "the name of a provenance record"));
      } else if (accept_kw("guard")) {
        Guard g;
        P_TRY(parse_guard(&g));
        e->guards.push_back(std::move(g));
      } else {
        break;
      }
    }
    return true;
  }

  bool parse_init(const SrcLoc& loc) {
    InitDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the label of the INIT fact"));
    P_TRY(expect(Tok::Colon, "after the label"));
    if (accept_kw("loc")) {
      d.is_loc = true;
      P_TRY(expect_ident(&d.loc_name, "the name of a location"));
      P_TRY(expect(Tok::Eq, "after the location"));
      P_TRY(parse_expr(&d.value));
    } else if (accept_kw("state")) {
      d.is_loc = false;
      P_TRY(parse_term(&d.res));  // a resource, not a comparison: the `=` that follows is the state of the fact
      P_TRY(expect(Tok::Eq, "after the resource"));
      P_TRY(expect_ident(&d.state, "a state name"));
    } else {
      return expected("'loc' or 'state' in an INIT fact");
    }
    while (is_common_clause()) P_TRY(parse_common_clause(&d));
    P_TRY(expect_semi("after the INIT fact"));
    prof->inits.push_back(std::move(d));
    return true;
  }

  bool parse_hb(const SrcLoc& loc) {
    HbRuleDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the rule"));
    P_TRY(expect(Tok::LBrace, "after the rule name"));
    bool has_template = false, has_stratum = false, has_robust = false;
    while (!accept(Tok::RBrace)) {
      if (is_common_clause()) {
        P_TRY(parse_common_clause(&d));
        P_TRY(expect_semi("after the clause"));
        continue;
      }
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the rule"));
      if (kw == "title") {
        P_TRY(expect_str(&d.title, "the title as a string"));
      } else if (kw == "stratum") {
        std::string s;
        const SrcLoc sloc = cur().loc;
        P_TRY(expect_ident(&s, "S, O or C"));
        if (s == "S") d.stratum = Stratum::S;
        else if (s == "O") d.stratum = Stratum::O;
        else if (s == "C") d.stratum = Stratum::C;
        else return fail_at(sloc, "unknown stratum '" + s + "' (expected S, O or C)");
        has_stratum = true;
      } else if (kw == "robust") {
        std::string s;
        const SrcLoc sloc = cur().loc;
        P_TRY(expect_ident(&s, "a robustness tag"));
        if (s == "unconditional") d.robust = Robust::Unconditional;
        else if (s == "obs_fatal") d.robust = Robust::ObsFatal;
        else if (s == "obs_soft") d.robust = Robust::ObsSoft;
        else if (s == "schedule_chosen") d.robust = Robust::ScheduleChosen;
        else if (s == "assumed") d.robust = Robust::Assumed;
        else return fail_at(sloc, "unknown robustness tag '" + s + "' (expected unconditional, obs_fatal, obs_soft, schedule_chosen or assumed)");
        has_robust = true;
      } else if (kw == "template") {
        if (has_template) return fail_at(kloc, "a rule has one template");
        has_template = true;
        P_TRY(parse_template(&d));
        continue;  // the template block closes with '}' and has no ';'
      } else {
        return fail_at(kloc, "unknown item '" + kw + "' in hb rule (expected title, stratum, robust, template, prov, guard or note)");
      }
      P_TRY(expect_semi("after the rule item"));
    }
    if (!has_template) return fail_at(loc, "rule '" + d.name + "' has no template");
    if (!has_stratum) return fail_at(loc, "rule '" + d.name + "' has no stratum");
    if (!has_robust) return fail_at(loc, "rule '" + d.name + "' has no robustness tag");
    prof->rules.push_back(std::move(d));
    return true;
  }

  bool parse_template(HbRuleDecl* d) {
    std::string t;
    const SrcLoc tloc = cur().loc;
    P_TRY(expect_ident(&t, "program_order, spawn, phase or observe"));
    if (t == "program_order") d->tmpl = RuleTemplate::ProgramOrder;
    else if (t == "spawn") d->tmpl = RuleTemplate::Spawn;
    else if (t == "phase") d->tmpl = RuleTemplate::Phase;
    else if (t == "observe") d->tmpl = RuleTemplate::Observe;
    else return fail_at(tloc, "unknown template '" + t + "' (expected program_order, spawn, phase or observe)");
    P_TRY(expect(Tok::LBrace, "after the template name"));
    while (!accept(Tok::RBrace)) {
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the template"));
      if (d->tmpl == RuleTemplate::ProgramOrder && kw == "looper") {
        std::string yn;
        P_TRY(expect_ident(&yn, "yes or no"));
        if (yn != "yes" && yn != "no") return fail_at(kloc, "expected yes or no after 'looper'");
        d->looper = yn == "yes";
      } else if (d->tmpl == RuleTemplate::Spawn && kw == "create") {
        P_TRY(expect_ident(&d->create_op, "the creating operation"));
        P_TRY(expect_kw("out", "in 'create <op> out <param>'"));
        P_TRY(expect_ident(&d->create_out, "the out parameter that carries the id of the child"));
      } else if (d->tmpl == RuleTemplate::Spawn && kw == "entry") {
        P_TRY(expect_ident(&d->entry_op, "the operation or site of the first event of the child"));
      } else if (d->tmpl == RuleTemplate::Phase && kw == "from") {
        P_TRY(expect_ident(&d->from_op, "the operation or site whose events come first"));
        P_TRY(expect_kw("phase", "in 'from <op> phase <phase>'"));
        P_TRY(expect_ident(&d->from_phase, "call, return or point"));
      } else if (d->tmpl == RuleTemplate::Observe && kw == "poll") {
        P_TRY(expect_str(&d->poll_fn, "the polling function as a string"));
        P_TRY(expect_kw("called_at", "in 'poll <fn> called_at <pattern>'"));
        P_TRY(parse_pattern(&d->called_at));
      } else if (d->tmpl == RuleTemplate::Observe && kw == "exit_outcome") {
        P_TRY(expect_ident(&d->exit_outcome, "a result constant"));
      } else if (d->tmpl == RuleTemplate::Observe && kw == "predicate") {
        if (accept_kw("from_code")) {
          d->predicate_from_code = true;
        } else {
          d->predicate_from_code = false;
          P_TRY(parse_expr(&d->predicate));
        }
      } else if (d->tmpl == RuleTemplate::Observe && kw == "else_terminal") {
        P_TRY(expect_str(&d->else_terminal, "the terminal function as a string"));
      } else if (d->tmpl == RuleTemplate::Observe && kw == "watch") {
        P_TRY(parse_name_list(&d->watch_kinds, "a kind name"));
      } else {
        return fail_at(kloc, "unknown item '" + kw + "' in template " + t);
      }
      P_TRY(expect_semi("after the template item"));
    }
    return true;
  }

  bool parse_allowed_entry(AllowedEntry* a) {
    a->loc = cur().loc;
    std::string kw;
    P_TRY(expect_ident(&kw, "an Allowed entry"));
    if (kw == "documented_drop") {
      a->kind = AllowedKind::DocumentedDrop;
      P_TRY(expect_str(&a->src, "the citation as a string"));
    } else if (kw == "returned_and_tested") {
      a->kind = AllowedKind::ReturnedAndTested;
      P_TRY(expect_kw("codes", "in returned_and_tested"));
      P_TRY(expect(Tok::LBrace, "after 'codes'"));
      P_TRY(parse_name_list(&a->codes, "a result constant"));
      P_TRY(expect(Tok::RBrace, "to close the codes"));
      if (accept_kw("continuation")) P_TRY(expect_str(&a->continuation, "the normal continuation as a string"));
    } else if (kw == "retry_until_ready") {
      a->kind = AllowedKind::RetryUntilReady;
      P_TRY(expect_kw("codes", "in retry_until_ready"));
      P_TRY(expect(Tok::LBrace, "after 'codes'"));
      P_TRY(parse_name_list(&a->codes, "a result constant"));
      P_TRY(expect(Tok::RBrace, "to close the codes"));
      P_TRY(expect_kw("bound", "in retry_until_ready"));
      P_TRY(expect_ident(&a->bound, "the constant that gives the bound"));
    } else if (kw == "fallback") {
      a->kind = AllowedKind::Fallback;
      P_TRY(expect_ident(&a->op, "the fallback operation"));
    } else if (kw == "defer_by_design") {
      a->kind = AllowedKind::DeferByDesign;
      P_TRY(expect_str(&a->src, "the citation as a string"));
    } else if (kw == "exit") {
      a->kind = AllowedKind::Exit;
      P_TRY(expect_ident(&a->exit_code, "the exit code"));
      P_TRY(expect_str(&a->src, "the citation as a string"));
    } else {
      return fail_at(a->loc, "unknown Allowed entry '" + kw +
                                 "' (expected documented_drop, returned_and_tested, retry_until_ready, fallback, defer_by_design or exit)");
    }
    if (accept_kw("conflicts_appreq")) a->conflicts_appreq = true;
    if (accept_kw("prov")) P_TRY(parse_name_list(&a->prov, "the name of a provenance record"));
    return true;
  }

  bool parse_rspec(const SrcLoc& loc) {
    RspecDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the specification"));
    P_TRY(expect(Tok::LBrace, "after the specification name"));
    bool has_allowed = false;
    while (!accept(Tok::RBrace)) {
      if (is_common_clause()) {
        P_TRY(parse_common_clause(&d));
        P_TRY(expect_semi("after the clause"));
        continue;
      }
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the specification"));
      if (kw == "use") {
        P_TRY(expect_kw("op", "after 'use'"));
        P_TRY(expect_ident(&d.use_op, "the name of the operation"));
      } else if (kw == "key") {
        P_TRY(parse_expr(&d.key));
      } else if (kw == "requires") {
        P_TRY(parse_expr(&d.requires_));
      } else if (kw == "oracle") {
        P_TRY(expect_kw("fns", "after 'oracle'"));
        while (true) {
          std::string f;
          P_TRY(expect_str(&f, "the name of an oracle function as a string"));
          d.oracle_fns.push_back(std::move(f));
          if (!accept(Tok::Comma)) break;
        }
      } else if (kw == "appreq") {
        if (accept_kw("unspecified_by_reference")) {
          d.appreq_unspecified = true;
        } else {
          d.appreq_unspecified = false;
          P_TRY(expect_ident(&d.appreq, "unspecified_by_reference or the name of a provenance record"));
        }
      } else if (kw == "allowed") {
        if (has_allowed) return fail_at(kloc, "Allowed is declared twice");
        has_allowed = true;
        if (accept_kw("none")) {
          d.allowed_head.kind = AllowedKind::None;
        } else if (accept_kw("unspecified_by_reference")) {
          d.allowed_head.kind = AllowedKind::Unspecified;
        } else if (is(Tok::LBrace)) {
          advance();
          d.allowed_head.kind = AllowedKind::Unspecified;
          while (!accept(Tok::RBrace)) {
            AllowedEntry a;
            P_TRY(parse_allowed_entry(&a));
            P_TRY(expect_semi("after the Allowed entry"));
            d.allowed.push_back(std::move(a));
          }
          if (d.allowed.empty()) return fail_at(kloc, "an Allowed block needs at least one entry (use 'none' or 'unspecified_by_reference')");
          continue;  // block form: no ';'
        } else {
          return expected("'none', 'unspecified_by_reference' or '{' after 'allowed'");
        }
      } else if (kw == "harm") {
        P_TRY(expect_ident(&d.harm, "a harm"));
        if (is(Tok::LParen)) {  // terminal_pending(panic | abort) has the parenthesized form
          advance();
          std::string arg;
          P_TRY(expect_ident(&arg, "panic or abort"));
          P_TRY(expect(Tok::RParen, "after the harm argument"));
          d.harm += "(" + arg + ")";
        }
      } else if (kw == "authored") {
        P_TRY(expect_str(&d.authored, "the authoring record as a string"));
      } else {
        return fail_at(kloc, "unknown item '" + kw +
                                 "' in rspec (expected use, key, requires, oracle, appreq, allowed, harm, authored, prov, guard or note)");
      }
      P_TRY(expect_semi("after the specification item"));
    }
    prof->rspecs.push_back(std::move(d));
    return true;
  }

  bool parse_trusted(const SrcLoc& loc) {
    TrustedDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the trusted entry"));
    P_TRY(expect(Tok::LBrace, "after the name"));
    while (!accept(Tok::RBrace)) {
      if (is_common_clause()) {
        P_TRY(parse_common_clause(&d));
        P_TRY(expect_semi("after the clause"));
        continue;
      }
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the trusted entry"));
      if (kw == "class") P_TRY(expect_ident(&d.cls, "declared_trusted or assumption"));
      else if (kw == "statement") P_TRY(expect_str(&d.statement, "the statement as a string"));
      else if (kw == "refutable_by") P_TRY(parse_name_list(&d.refutable_by, "an obligation (O1 ... O10)"));
      else return fail_at(kloc, "unknown item '" + kw + "' in trusted (expected class, statement, refutable_by, prov, guard or note)");
      P_TRY(expect_semi("after the trusted item"));
    }
    prof->trusted.push_back(std::move(d));
    return true;
  }

  bool parse_prov(const SrcLoc& loc) {
    ProvDecl d;
    d.loc = loc;
    P_TRY(expect_ident(&d.name, "the name of the provenance record"));
    P_TRY(expect(Tok::LBrace, "after the name"));
    while (!accept(Tok::RBrace)) {
      std::string kw;
      const SrcLoc kloc = cur().loc;
      P_TRY(expect_ident(&kw, "an item of the provenance record"));
      if (kw == "tier") P_TRY(expect_ident(&d.tier, "a tier"));
      else if (kw == "kind") P_TRY(expect_ident(&d.kind, "a provenance kind"));
      else if (kw == "side") P_TRY(expect_ident(&d.side, "supplier, caller or rule"));
      else if (kw == "file") P_TRY(expect_str(&d.file, "the path as a string"));
      else if (kw == "lines") {
        int64_t a = 0, b = 0;
        const SrcLoc lloc = cur().loc;
        P_TRY(expect_int(&a, "the first line"));
        b = a;
        if (is(Tok::Int)) P_TRY(expect_int(&b, "the last line"));
        if (a < 1 || b < a || b > 0x7fffffff) return fail_at(lloc, "invalid line range (lines count from 1 and the second number may not be smaller)");
        d.line_from = static_cast<uint32_t>(a);
        d.line_to = static_cast<uint32_t>(b);
      } else if (kw == "sha256") P_TRY(expect_str(&d.sha256, "the hash as a string"));
      else if (kw == "cite") P_TRY(expect_str(&d.cite, "the locator as a string"));
      else if (kw == "authored") P_TRY(expect_str(&d.authored, "the authoring record as a string"));
      else if (kw == "note") P_TRY(expect_str(&d.note, "the note as a string"));
      else return fail_at(kloc, "unknown item '" + kw + "' in prov (expected tier, kind, side, file, lines, sha256, cite, authored or note)");
      P_TRY(expect_semi("after the provenance item"));
    }
    prof->provs.push_back(std::move(d));
    return true;
  }
};

}  // namespace

Status parse_text(const SourceText& src, Profile* into, std::vector<Diag>* diags) {
  const uint32_t file = static_cast<uint32_t>(into->files.size());
  into->files.push_back(src.name);
  std::vector<Token> toks;
  LexError le;
  if (!tokenize(src.text, file, &toks, &le)) {
    Diag d;
    d.loc = le.loc;
    d.message = le.message;
    diags->push_back(std::move(d));
    return Status::error(Err::Invalid, format_diag(into->files, diags->back()));
  }
  Parser p(toks, into, file, diags);
  if (!p.parse_file_body()) return Status::error(Err::Invalid, format_diag(into->files, diags->back()));
  return Status::success();
}

Status parse_expression(const std::string& text, ExprPtr* out, std::vector<Diag>* diags) {
  std::vector<std::string> files{"<expression>"};
  std::vector<Token> toks;
  LexError le;
  if (!tokenize(text, 0, &toks, &le)) {
    Diag d;
    d.loc = le.loc;
    d.message = le.message;
    diags->push_back(std::move(d));
    return Status::error(Err::Invalid, format_diag(files, diags->back()));
  }
  Profile scratch;
  Parser p(toks, &scratch, 0, diags);
  ExprPtr e;
  if (!p.parse_expr(&e)) return Status::error(Err::Invalid, format_diag(files, diags->back()));
  if (!p.is(Tok::End)) {
    p.expected("the end of the expression");
    return Status::error(Err::Invalid, format_diag(files, diags->back()));
  }
  *out = std::move(e);
  return Status::success();
}

std::string format_diag(const std::vector<std::string>& files, const Diag& d) {
  std::string out = d.loc.file < files.size() ? files[d.loc.file] : std::string("<unknown>");
  out += ":" + std::to_string(d.loc.line) + ":" + std::to_string(d.loc.col) + ": ";
  out += d.severity == Severity::Error ? "error" : d.severity == Severity::Warning ? "warning" : "note";
  out += ": " + d.message;
  return out;
}

bool has_errors(const std::vector<Diag>& diags) {
  for (const Diag& d : diags)
    if (d.severity == Severity::Error) return true;
  return false;
}

}  // namespace phi
}  // namespace kz

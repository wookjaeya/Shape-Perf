// The lexer and the parser of the profile language: the error paths report the first error with
// its line and column (columns count bytes), the accepted forms produce the expected syntax tree,
// and expressions print back to a form that parses to the same tree.

#include <string>
#include <vector>

#include "kz/core/phi.hpp"
#include "support/kz_test.hpp"

namespace {

using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::SourceText;

struct ErrCase {
  const char* text;
  uint32_t line, col;
  const char* message;  // a fragment of the message
};

// Parses `text` and checks that it fails with one diagnostic at line:col whose message contains `message`.
void expect_error(const ErrCase& c) {
  Profile p;
  std::vector<Diag> diags;
  const kz::Status s = kz::phi::parse_text(SourceText{"t.kzp", c.text}, &p, &diags);
  if (s.ok() || diags.size() != 1 || diags[0].loc.line != c.line || diags[0].loc.col != c.col ||
      diags[0].message.find(c.message) == std::string::npos) {
    std::string got = s.ok() ? "no error" : (diags.empty() ? "no diagnostic" : kz::phi::format_diag(p.files, diags[0]));
    std::fprintf(stderr, "parse error case failed:\n--- text\n%s\n--- wanted %u:%u containing \"%s\"\n--- got %s\n", c.text, c.line, c.col, c.message, got.c_str());
  }
  ++kztest::state().checks;
  if (s.ok() || diags.size() != 1 || diags[0].loc.line != c.line || diags[0].loc.col != c.col || diags[0].message.find(c.message) == std::string::npos)
    ++kztest::state().failures;
  // The status text carries the same position.
  if (!s.ok()) {
    ++kztest::state().checks;
    const std::string pos = "t.kzp:" + std::to_string(c.line) + ":" + std::to_string(c.col) + ": error: ";
    if (s.message.find(pos) == std::string::npos) {
      std::fprintf(stderr, "status message lacks \"%s\": %s\n", pos.c_str(), s.message.c_str());
      ++kztest::state().failures;
    }
  }
}

kz::phi::ExprPtr parse_expr_ok(const std::string& text) {
  kz::phi::ExprPtr e;
  std::vector<Diag> diags;
  const kz::Status s = kz::phi::parse_expression(text, &e, &diags);
  if (!s.ok()) std::fprintf(stderr, "expression \"%s\": %s\n", text.c_str(), s.describe().c_str());
  KZ_CHECK(s.ok());
  return e;
}

}  // namespace

int main() {
  // ---- lexical errors -------------------------------------------------------------------------
  const ErrCase lexical[] = {
      {"phi 1;\n$", 2, 1, "unexpected character '$'"},
      {"phi 1;\nconst A = 1 & 2;", 2, 13, "unexpected character '&'"},
      {"phi 1;\nconst A = 1 | 2;", 2, 13, "unexpected character '|'"},
      {"phi 1;\nconst A = 99999999999999999999;", 2, 11, "does not fit in 64 bits"},
      {"phi 1;\nconst A = 0xFFFFFFFFFFFFFFFFF;", 2, 11, "does not fit in 64 bits"},
      {"phi 1;\nconst A = 0x;", 2, 11, "hexadecimal literal without digits"},
      {"phi 1;\nconst A = 12ab;", 2, 11, "a letter follows the digits"},
      {"phi 1;\nconst A = \"abc;", 2, 11, "unterminated string"},
      {"phi 1;\nconst A = \"abc", 2, 11, "unterminated string"},
      {"phi 1;\nconst A = \"a\\qb\";", 2, 13, "unknown escape '\\q'"},
      {"phi 1;\nconst A = \"a\x01" "b\";", 2, 13, "control character 0x01"},
      {"phi 1;\nconst A = 1; \xc3\xa9", 2, 14, "non-ASCII byte 0xC3"},
      {"phi 1;\n\n\n  @", 4, 3, "unexpected character '@'"},
  };
  for (const ErrCase& c : lexical) expect_error(c);

  // A comment and a string may hold any bytes; a tab is allowed in a string; the escapes work.
  {
    Profile p;
    std::vector<Diag> diags;
    const kz::Status s = kz::phi::parse_text(SourceText{"t.kzp", "phi 1; # caf\xc3\xa9 \xe2\x82\xac\nconst A = \"caf\xc3\xa9\\n\\t\\\\\\\"x\" prov q;\n"}, &p, &diags);
    KZ_CHECK_OK(s);
    KZ_CHECK_EQ(p.consts.size(), static_cast<size_t>(1));
    if (!p.consts.empty()) KZ_CHECK_EQ(p.consts[0].sval, std::string("caf\xc3\xa9\n\t\\\"x"));
  }
  {  // CRLF line ends count as one line each; the column of the next line starts at 1
    Profile p;
    std::vector<Diag> diags;
    const kz::Status s = kz::phi::parse_text(SourceText{"t.kzp", "phi 1;\r\nconst A = ;\r\n"}, &p, &diags);
    KZ_CHECK(!s.ok());
    KZ_CHECK(diags.size() == 1 && diags[0].loc.line == 2 && diags[0].loc.col == 11);
  }
  {  // a name may hold dots and hyphens inside; a trailing one is punctuation
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::parse_text(SourceText{"t.kzp", "phi 1;\nkind a.b-c() { states X < Y; initial X; prov q; }\n"}, &p, &diags));
    KZ_CHECK_EQ(p.kinds.size(), static_cast<size_t>(1));
    if (!p.kinds.empty()) {
      KZ_CHECK_EQ(p.kinds[0].name, std::string("a.b-c"));
      KZ_CHECK_EQ(p.kinds[0].states.size(), static_cast<size_t>(2));
    }
    KZ_CHECK_OK(kz::phi::parse_text(SourceText{"t.kzp", "phi 1;\nkind k() { states A < B; transition A->B on go; }\n"}, &p, &diags));
  }

  // ---- syntax errors ----------------------------------------------------------------------------
  const ErrCase syntax[] = {
      {"const A = 1;", 1, 1, "expected 'phi' at the start of the file, found identifier 'const'"},
      {"", 1, 1, "expected 'phi' at the start of the file, found end of file"},
      {"phi 2;", 1, 5, "unsupported language version 2"},
      {"phi x;", 1, 5, "expected the language version, found identifier 'x'"},
      {"phi 1", 1, 6, "expected ';' after the language version, found end of file"},
      {"phi 1;\nconst A = 1\nconst B = 2;", 3, 1, "expected ';' after the constant, found identifier 'const'"},
      {"phi 1;\nconst A = ;", 2, 11, "expected an integer, a string, true, false or 'ref eval ...'"},
      {"phi 1;\nconst = 1;", 2, 7, "expected the name of the constant, found '='"},
      {"phi 1;\nconst A = ref 5;", 2, 15, "expected 'eval' after 'ref', found integer 5"},
      {"phi 1;\nfoo bar;", 2, 1, "unknown declaration 'foo'"},
      {"phi 1;\n5;", 2, 1, "expected a declaration, found integer 5"},
      {"phi 1;\nprofile \"a\";\nprofile \"b\";", 3, 1, "the profile name is declared twice"},
      {"phi 1;\ncontext { owner_kind a; }\ncontext { }", 3, 1, "the context is declared twice"},
      {"phi 1;\ncontext { frob a; }", 2, 11, "unknown item 'frob' in context"},
      {"phi 1;\nscope { frob a; }", 2, 9, "unknown item 'frob' in scope"},
      {"phi 1;\nsource s root \"\" rev;", 2, 21, "expected the revision as a string, found ';'"},
      {"phi 1;\nkind k(x: Foo) { }", 2, 11, "unknown sort 'Foo'"},
      {"phi 1;\nkind k(x: Id k) { }", 2, 14, "expected '<' after 'Id'"},
      {"phi 1;\nkind k() { frob x; }", 2, 12, "unknown item 'frob' in kind"},
      {"phi 1;\nkind k() { ids { weird X; } }", 2, 18, "unknown item 'weird' in ids"},
      {"phi 1;\nkind k() { transition A B on go; }", 2, 25, "expected '->' between the states of a transition"},
      {"phi 1;\nkind k() { states A < B", 2, 24, "expected ';' after the kind item, found end of file"},
      {"phi 1;\nkind k() {", 2, 11, "expected an item of the kind, found end of file"},
      {"phi 1;\nloc L { struct \"S\"; field \"f\"; }", 2, 1, "location 'L' has no 'holds' sort"},
      {"phi 1;\nloc L { holds Int; frob 1; }", 2, 20, "unknown item 'frob' in loc"},
      {"phi 1;\nbind field \"S\" \"f\" state(k(x));", 2, 20, "expected 'as' in a binding"},
      {"phi 1;\nsite s = fn(abc).call(\"x\");", 2, 13, "expected the name of the function as a string"},
      {"phi 1;\nsite s = fn(\"f\").frob(\"x\");", 2, 18, "unknown pattern operation 'frob'"},
      {"phi 1;\nsite s = fn(\"f\").call(\"x\", arg[0]);", 2, 34, "expected '==', '!=', 'is addr(...)' or 'any'"},
      {"phi 1;\nsite s = fn(\"f\").call(\"x\", arg[300] == 1);", 2, 35, "argument index out of range"},
      {"phi 1;\nsite s = fn(\"f\").call(\"x\") nth 0;", 2, 33, "counts from 1"},
      {"phi 1;\nsite s = fn(\"f\").call(\"x\") macro maybe;", 2, 34, "expected yes, no or any after 'macro', found 'maybe'"},
      {"phi 1;\nsite s = fn(\"f\").store(Type);", 2, 28, "expected Type.field in store(...)"},
      {"phi 1;\nop f() { frob; }", 2, 10, "unknown item 'frob' in operation"},
      {"phi 1;\nop f() { conjunct c: true; }", 2, 26, "expected 'lp' to give the linearization point of the conjunct"},
      {"phi 1;\nop f() { conjunct c: true lp somewhere; }", 2, 30, "expected 'capture', 'at <pattern>' or 'site <name>' after 'lp'"},
      {"phi 1;\nop f() { effect e: frob x lp capture; }", 2, 20, "unknown effect 'frob'"},
      {"phi 1;\nop f() { effect e: lifecycle k(1) lp capture; }", 2, 35, "expected '->' or 'raise' in a lifecycle effect"},
      {"phi 1;\nop f() { outcome C: weird; }", 2, 21, "unknown outcome class 'weird'"},
      {"phi 1;\nop f() { outcome C: not_ready x; }", 2, 31, "expected '(' after 'not_ready'"},
      {"phi 1;\nhb R { template program_order { } }", 2, 1, "rule 'R' has no stratum"},
      {"phi 1;\nhb R { stratum S; template program_order { } }", 2, 1, "rule 'R' has no robustness tag"},
      {"phi 1;\nhb R { stratum Q; }", 2, 16, "unknown stratum 'Q'"},
      {"phi 1;\nhb R { robust sometimes; }", 2, 15, "unknown robustness tag 'sometimes'"},
      {"phi 1;\nhb R { template weird { } }", 2, 17, "unknown template 'weird'"},
      {"phi 1;\nhb R { template program_order { looper maybe; } }", 2, 33, "expected yes or no after 'looper'"},
      {"phi 1;\nhb R { template spawn { entry x; } template phase { } }", 2, 36, "a rule has one template"},
      {"phi 1;\nrspec S { allowed { }; }", 2, 11, "an Allowed block needs at least one entry"},
      {"phi 1;\nrspec S { allowed none; allowed none; }", 2, 25, "Allowed is declared twice"},
      {"phi 1;\nrspec S { allowed { frob 1; }; }", 2, 21, "unknown Allowed entry 'frob'"},
      {"phi 1;\nrspec S { frob 1; }", 2, 11, "unknown item 'frob' in rspec"},
      {"phi 1;\ntrusted T { frob 1; }", 2, 13, "unknown item 'frob' in trusted"},
      {"phi 1;\nprov p { lines 5 3; }", 2, 16, "invalid line range"},
      {"phi 1;\nprov p { lines 0; }", 2, 16, "invalid line range"},
      {"phi 1;\nprov p { frob 1; }", 2, 10, "unknown item 'frob' in prov"},
      {"phi 1;\ninit i: weird x = 1;", 2, 9, "expected 'loc' or 'state' in an INIT fact"},
  };
  for (const ErrCase& c : syntax) expect_error(c);

  // ---- expressions ------------------------------------------------------------------------------
  {
    std::vector<Diag> diags;
    kz::phi::ExprPtr e;
    KZ_CHECK(!kz::phi::parse_expression("a +", &e, &diags).ok());
    KZ_CHECK(diags.size() == 1 && diags[0].loc.line == 1 && diags[0].loc.col == 3 && diags[0].message.find("unexpected character '+'") != std::string::npos);
    diags.clear();
    KZ_CHECK(!kz::phi::parse_expression("(a", &e, &diags).ok());
    KZ_CHECK(diags.size() == 1 && diags[0].message.find("expected ')' to close the parenthesis, found end of file") != std::string::npos);
    diags.clear();
    KZ_CHECK(!kz::phi::parse_expression("a in {}", &e, &diags).ok());
    KZ_CHECK(diags.size() == 1 && diags[0].message.find("needs at least one element") != std::string::npos);
    diags.clear();
    KZ_CHECK(!kz::phi::parse_expression("a < b < c", &e, &diags).ok());
    KZ_CHECK(diags.size() == 1 && diags[0].loc.col == 7 && diags[0].message.find("expected the end of the expression, found '<'") != std::string::npos);
    diags.clear();
    KZ_CHECK(!kz::phi::parse_expression("forall x: k where { true }", &e, &diags).ok());
    KZ_CHECK(diags.size() == 1 && diags[0].message.find("expected an expression, found '{'") != std::string::npos);
    diags.clear();
    KZ_CHECK(!kz::phi::parse_expression("f(a,", &e, &diags).ok());
    KZ_CHECK(!kz::phi::parse_expression("", &e, &diags).ok());
    KZ_CHECK(!kz::phi::parse_expression("a b", &e, &diags).ok());
  }
  {  // precedence and flattening: => < || < && < ! < comparison
    using kz::phi::ExprKind;
    kz::phi::ExprPtr e = parse_expr_ok("a || b && !c => d");
    KZ_CHECK_EQ(static_cast<int>(e->kind), static_cast<int>(ExprKind::Imp));
    KZ_CHECK_EQ(static_cast<int>(e->args[0]->kind), static_cast<int>(ExprKind::Or));
    KZ_CHECK_EQ(e->args[0]->args.size(), static_cast<size_t>(2));
    KZ_CHECK_EQ(static_cast<int>(e->args[0]->args[1]->kind), static_cast<int>(ExprKind::And));
    KZ_CHECK_EQ(static_cast<int>(e->args[0]->args[1]->args[1]->kind), static_cast<int>(ExprKind::Not));
    e = parse_expr_ok("a && b && c && d");
    KZ_CHECK_EQ(static_cast<int>(e->kind), static_cast<int>(ExprKind::And));
    KZ_CHECK_EQ(e->args.size(), static_cast<size_t>(4));
    e = parse_expr_ok("a || (b || c) || d");
    KZ_CHECK_EQ(static_cast<int>(e->kind), static_cast<int>(ExprKind::Or));
    KZ_CHECK_EQ(e->args.size(), static_cast<size_t>(4));
    e = parse_expr_ok("a => b => c");  // right associative
    KZ_CHECK_EQ(static_cast<int>(e->args[1]->kind), static_cast<int>(ExprKind::Imp));
    e = parse_expr_ok("x == 1 && y != 2");
    KZ_CHECK_EQ(static_cast<int>(e->kind), static_cast<int>(ExprKind::And));
    e = parse_expr_ok("x = 1");  // '=' and '==' are the same comparison
    KZ_CHECK_EQ(static_cast<int>(e->cmp), static_cast<int>(kz::phi::CmpOp::Eq));
    e = parse_expr_ok("state(k(1)) in {A, B, C}");
    KZ_CHECK_EQ(static_cast<int>(e->kind), static_cast<int>(ExprKind::In));
    KZ_CHECK_EQ(e->args.size(), static_cast<size_t>(4));
    e = parse_expr_ok("forall r: k where state(r) != A { exists q: m { true } }");
    KZ_CHECK_EQ(static_cast<int>(e->kind), static_cast<int>(ExprKind::Quant));
    KZ_CHECK(e->args[0] != nullptr);
    KZ_CHECK_EQ(static_cast<int>(e->args[1]->kind), static_cast<int>(ExprKind::Quant));
    KZ_CHECK(e->args[1]->args[0] == nullptr);  // no guard
  }
  {  // print and parse again: the text of an expression is a fixed point
    const char* const exprs[] = {
        "a",
        "!a",
        "!(a && b)",
        "!a && !b",
        "a && (b || c)",
        "(a && b) || c",
        "a => b",
        "(a => b) => c",
        "a => (b => c)",
        "a || b => c && d",
        "x == 1",
        "x != -5",
        "valid(arg.p) && state(k(arg.p)) = REG",
        "state(k(1, 2)) in {A, B}",
        "tag(k(1), T) != CORE",
        "forall r: k where state(r) != FREE { state(m(r)) >= RUNNING }",
        "exists r: k { !(state(r) = FREE) }",
        "id(owner(exec)) = 0x1234",
        "value(L) = 0",
        "\"text\\n\" = name",
        "true && false",
        "a == b && c == d || e == f",
    };
    for (const char* text : exprs) {
      kz::phi::ExprPtr e = parse_expr_ok(text);
      if (!e) continue;
      const std::string once = kz::phi::expr_text(*e);
      kz::phi::ExprPtr e2 = parse_expr_ok(once);
      if (!e2) continue;
      const std::string twice = kz::phi::expr_text(*e2);
      if (once != twice) std::fprintf(stderr, "not a fixed point: %s -> %s -> %s\n", text, once.c_str(), twice.c_str());
      KZ_CHECK_EQ(once, twice);
    }
    // Spot-check the printed form: only the parentheses that the precedence needs.
    KZ_CHECK_EQ(kz::phi::expr_text(*parse_expr_ok("((a))")), std::string("a"));
    KZ_CHECK_EQ(kz::phi::expr_text(*parse_expr_ok("(a && b) && c")), std::string("a && b && c"));
    KZ_CHECK_EQ(kz::phi::expr_text(*parse_expr_ok("a && (b || c)")), std::string("a && (b || c)"));
    KZ_CHECK_EQ(kz::phi::expr_text(*parse_expr_ok("x = 0x10")), std::string("x = 16"));
  }

  // ---- accepted forms ----------------------------------------------------------------------------
  {  // the Allowed forms, harm with an argument, appreq, a predicate in a rule, a state INIT fact
    const char* text = R"KZ(phi 1;
rspec S {
  use op f;
  key k(arg.a);
  requires true;
  appreq unspecified_by_reference;
  allowed {
    documented_drop "src one" prov a;
    returned_and_tested codes { X, Y } continuation "after the test" prov b, c;
    retry_until_ready codes { X } bound LIMIT prov a;
    fallback g conflicts_appreq prov a;
    defer_by_design "source two" prov a;
    exit CODE "source three" prov a;
  }
  harm terminal_pending(panic);
  authored "t";
}
hb R {
  stratum O;
  robust obs_soft;
  template observe { poll "p" called_at fn("f").loop(poll); exit_outcome OK; predicate state(k(1)) = A; else_terminal "t"; watch k, m; }
}
init s: state k(1) = LIVE prov a;
)KZ";
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::parse_text(SourceText{"t.kzp", text}, &p, &diags));
    KZ_CHECK_EQ(p.rspecs.size(), static_cast<size_t>(1));
    if (!p.rspecs.empty()) {
      const kz::phi::RspecDecl& r = p.rspecs[0];
      KZ_CHECK_EQ(r.allowed.size(), static_cast<size_t>(6));
      KZ_CHECK_EQ(r.harm, std::string("terminal_pending(panic)"));
      KZ_CHECK(r.appreq_unspecified);
      if (r.allowed.size() == 6) {
        using kz::phi::AllowedKind;
        KZ_CHECK_EQ(static_cast<int>(r.allowed[0].kind), static_cast<int>(AllowedKind::DocumentedDrop));
        KZ_CHECK_EQ(static_cast<int>(r.allowed[1].kind), static_cast<int>(AllowedKind::ReturnedAndTested));
        KZ_CHECK_EQ(r.allowed[1].codes.size(), static_cast<size_t>(2));
        KZ_CHECK_EQ(r.allowed[1].continuation, std::string("after the test"));
        KZ_CHECK_EQ(r.allowed[1].prov.size(), static_cast<size_t>(2));
        KZ_CHECK_EQ(static_cast<int>(r.allowed[2].kind), static_cast<int>(AllowedKind::RetryUntilReady));
        KZ_CHECK_EQ(r.allowed[2].bound, std::string("LIMIT"));
        KZ_CHECK_EQ(static_cast<int>(r.allowed[3].kind), static_cast<int>(AllowedKind::Fallback));
        KZ_CHECK(r.allowed[3].conflicts_appreq);
        KZ_CHECK_EQ(static_cast<int>(r.allowed[4].kind), static_cast<int>(AllowedKind::DeferByDesign));
        KZ_CHECK_EQ(static_cast<int>(r.allowed[5].kind), static_cast<int>(AllowedKind::Exit));
        KZ_CHECK_EQ(r.allowed[5].exit_code, std::string("CODE"));
      }
    }
    KZ_CHECK_EQ(p.rules.size(), static_cast<size_t>(1));
    if (!p.rules.empty()) {
      KZ_CHECK(!p.rules[0].predicate_from_code);
      KZ_CHECK(p.rules[0].predicate != nullptr);
      KZ_CHECK_EQ(p.rules[0].watch_kinds.size(), static_cast<size_t>(2));
      KZ_CHECK_EQ(static_cast<int>(p.rules[0].called_at.op), static_cast<int>(kz::phi::PatOp::LoopPoll));
    }
    KZ_CHECK_EQ(p.inits.size(), static_cast<size_t>(1));
    if (!p.inits.empty()) {
      KZ_CHECK(!p.inits[0].is_loc);
      KZ_CHECK_EQ(p.inits[0].state, std::string("LIVE"));
    }
  }
  {  // patterns: every operation and every argument predicate
    const char* text = R"KZ(phi 1;
site a = fn("f").call("g", arg[0] == 5, arg[1] != NAME, arg[2] is addr(T.fld), arg[3] any) nth 2 macro yes;
site b = fn("f").store(T.fld);
site c = fn("f").memset(T);
site d = fn("f").load(T.fld) macro no;
site e = fn("f").loop(poll);
site f = fn("f").return;
)KZ";
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::parse_text(SourceText{"t.kzp", text}, &p, &diags));
    KZ_CHECK_EQ(p.sites.size(), static_cast<size_t>(6));
    if (p.sites.size() == 6) {
      const kz::phi::Pattern& a = p.sites[0].pattern;
      KZ_CHECK_EQ(a.args.size(), static_cast<size_t>(4));
      KZ_CHECK_EQ(a.nth, 2u);
      KZ_CHECK_EQ(static_cast<int>(a.macro), static_cast<int>(kz::phi::MacroFlag::Yes));
      if (a.args.size() == 4) {
        KZ_CHECK_EQ(static_cast<int>(a.args[0].cmp), static_cast<int>(kz::phi::ArgCmp::Eq));
        KZ_CHECK_EQ(a.args[0].ival, static_cast<int64_t>(5));
        KZ_CHECK(a.args[1].is_name);
        KZ_CHECK_EQ(a.args[1].name, std::string("NAME"));
        KZ_CHECK_EQ(static_cast<int>(a.args[2].cmp), static_cast<int>(kz::phi::ArgCmp::AddrOf));
        KZ_CHECK_EQ(a.args[2].name, std::string("T.fld"));
        KZ_CHECK_EQ(static_cast<int>(a.args[3].cmp), static_cast<int>(kz::phi::ArgCmp::Any));
      }
      KZ_CHECK_EQ(p.sites[1].pattern.type_name, std::string("T"));
      KZ_CHECK_EQ(p.sites[1].pattern.field, std::string("fld"));
      KZ_CHECK_EQ(static_cast<int>(p.sites[2].pattern.op), static_cast<int>(kz::phi::PatOp::Memset));
      KZ_CHECK_EQ(static_cast<int>(p.sites[3].pattern.op), static_cast<int>(kz::phi::PatOp::Load));
      KZ_CHECK_EQ(static_cast<int>(p.sites[5].pattern.op), static_cast<int>(kz::phi::PatOp::Return));
    }
  }

  return kztest::finish("test_core_phi_parser");
}

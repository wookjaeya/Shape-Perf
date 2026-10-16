// The one evaluator of the core expression language, in its two domains: the meaning of every
// construct over a concrete state (the mini profile of tests/unit/support/phi_mini.hpp), and the
// specific behaviour of the abstract domain (three-valued logic, intervals, sets of states).
// The agreement of the two domains on generated expressions is the property test
// tests/property/test_eval_agreement.cpp.

#include <string>
#include <vector>

#include "kz/core/eval.hpp"
#include "kz/core/phi.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::eval::AbstractDomain;
using kz::eval::AbstractRes;
using kz::eval::AbstractState;
using kz::eval::AVal;
using kz::eval::ConcreteDomain;
using kz::eval::ConcreteState;
using kz::eval::CVal;
using kz::eval::Evaluator;
using kz::eval::ResId;
using kz::eval::Tri;
using kz::phi::Diag;
using kz::phi::Profile;

// The mini profile plus a kind without an identifier space and two more locations (an Int and an Id).
const char* const kExtra = R"KZ(# extra declarations of the evaluator test
phi 1;
kind plain(i: Int) { states P < Q; initial P; scope process; prov code_a; }
loc Aux.n { struct "Aux_t"; field "n"; holds Int; prov code_a; }
loc Aux.m { struct "Aux_t"; field "m"; holds Id<slot>; prov code_a; }
)KZ";

struct Fix {
  Profile p;
  kz::phi::ExprContext ctx;
  int32_t slot = -1, stage = -1, reg = -1, gate = -1, self = -1, plain = -1;
  int32_t box_loc = -1, aux_n = -1, aux_m = -1;
  int32_t type_tag = 0;

  Fix() {
    std::vector<Diag> diags;
    std::vector<kz::phi::SourceText> files = kzmini::files();
    files.push_back(kz::phi::SourceText{"extra.kzp", kExtra});
    const bool ok = kz::phi::load_profile(files, &p, &diags).ok();
    if (!ok) std::fprintf(stderr, "the evaluator test profile does not load\n");
    plain = p.find_kind("plain");
    aux_n = p.find_loc("Aux.n");
    aux_m = p.find_loc("Aux.m");
    slot = p.find_kind("slot");
    stage = p.find_kind("stage");
    reg = p.find_kind("reg");
    gate = p.find_kind("gate");
    self = p.find_kind("self");
    box_loc = p.find_loc("Box.id");
    kz::phi::Sort id_slot;
    id_slot.kind = kz::phi::SortKind::Id;
    id_slot.kind_index = slot;
    kz::phi::Sort int_sort;
    int_sort.kind = kz::phi::SortKind::Int;
    kz::phi::Sort str_sort;
    str_sort.kind = kz::phi::SortKind::Str;
    kz::phi::Sort loc_sort;
    loc_sort.kind = kz::phi::SortKind::Loc;
    ctx.args = {kz::phi::ArgSpec{"arg.Who", id_slot}, kz::phi::ArgSpec{"arg.N", int_sort}, kz::phi::ArgSpec{"arg.S", str_sort}, kz::phi::ArgSpec{"arg.L", loc_sort}};
    ctx.exec = true;
    ctx.result = true;
  }

  kz::phi::ExprPtr parse(const std::string& text) {
    kz::phi::ExprPtr e;
    std::vector<Diag> diags;
    if (!kz::phi::parse_expression(text, &e, &diags).ok()) {
      std::fprintf(stderr, "parse \"%s\" failed\n", text.c_str());
      return nullptr;
    }
    kz::phi::Sort any;
    if (!kz::phi::resolve_expression(p, e.get(), ctx, any, &diags)) {
      for (const Diag& d : diags) std::fprintf(stderr, "resolve \"%s\": %s\n", text.c_str(), d.message.c_str());
      return nullptr;
    }
    return e;
  }

  CVal run(const std::string& text, const ConcreteState& st) {
    kz::phi::ExprPtr e = parse(text);
    KZ_CHECK(e != nullptr);
    if (!e) return CVal();
    ConcreteDomain dom(p, st);
    Evaluator<ConcreteDomain> ev(p, dom);
    return ev.eval(*e);
  }
  bool truth(const std::string& text, const ConcreteState& st) {
    const CVal v = run(text, st);
    KZ_CHECK(v.sort == kz::phi::SortKind::Bool);
    return v.is_true();
  }
  AVal arun(const std::string& text, const AbstractState& st) {
    kz::phi::ExprPtr e = parse(text);
    KZ_CHECK(e != nullptr);
    if (!e) return AVal();
    AbstractDomain dom(p, st);
    Evaluator<AbstractDomain> ev(p, dom);
    return ev.eval(*e);
  }
  ConcreteState state() {
    ConcreteState st = kz::eval::initial_state(p);
    st.args = {0, 0, 0};
    st.exec = 1;
    for (size_t i = 0; i < p.consts.size(); ++i)
      if (p.consts[i].value == kz::phi::ConstDecl::Value::RefEval) st.consts[static_cast<int32_t>(i)] = 4;
    std::vector<std::vector<int64_t>> keys;
    for (int64_t i = 0; i < 4; ++i) keys.push_back({i});
    st.instances[slot] = keys;
    st.instances[stage] = keys;
    return st;
  }
  ResId rid(int32_t kind, int64_t k) { return ResId{kind, {k}}; }
  int32_t state_ord(int32_t kind, const char* name) {
    const auto& st = p.kinds[static_cast<size_t>(kind)].states;
    for (size_t i = 0; i < st.size(); ++i)
      if (st[i] == name) return static_cast<int32_t>(i);
    return -1;
  }
};

}  // namespace

int main() {
  Fix f;
  KZ_CHECK(f.p.checked);

  {  // connectives (no short-circuit, but the values are those of the truth tables)
    ConcreteState st = f.state();
    const bool t = true, o = false;
    for (bool a : {t, o})
      for (bool b : {t, o}) {
        const std::string A = a ? "true" : "false", B = b ? "true" : "false";
        KZ_CHECK_EQ(f.truth(A + " && " + B, st), a && b);
        KZ_CHECK_EQ(f.truth(A + " || " + B, st), a || b);
        KZ_CHECK_EQ(f.truth("(" + A + ") => (" + B + ")", st), !a || b);
        KZ_CHECK_EQ(f.truth("!" + A, st), !a);
        KZ_CHECK_EQ(f.truth(A + " = " + B, st), a == b);
        KZ_CHECK_EQ(f.truth(A + " != " + B, st), a != b);
      }
    KZ_CHECK(f.truth("true && true && true && !false", st));
    KZ_CHECK(!f.truth("true && true && false && true", st));
    KZ_CHECK(f.truth("false || false || true", st));
  }

  {  // integers, strings, arguments
    ConcreteState st = f.state();
    st.args = {0, 7, kz::eval::str_handle("x")};
    KZ_CHECK(f.truth("arg.N = 7", st));
    KZ_CHECK(f.truth("arg.N != 8", st));
    KZ_CHECK(f.truth("arg.N < 8 && arg.N <= 7 && arg.N > 6 && arg.N >= 7", st));
    KZ_CHECK(!f.truth("arg.N < 7", st));
    KZ_CHECK(f.truth("arg.N in {1, 7, 9}", st));
    KZ_CHECK(!f.truth("arg.N in {1, 2}", st));
    KZ_CHECK(f.truth("arg.S = \"x\"", st));
    KZ_CHECK(!f.truth("arg.S = \"y\"", st));
    KZ_CHECK(f.truth("\"abc\" = \"abc\" && \"abc\" != \"abd\"", st));
    KZ_CHECK(f.truth("-5 < 0", st));
    KZ_CHECK(f.truth("OK = 0 && BAD_ARG = 0xc0000003", st));
    KZ_CHECK(f.truth("FLAG && NAME = \"mini\"", st));
    KZ_CHECK(f.truth("TABLE = 4", st));  // the ref eval constant comes from the state
    ConcreteState no_consts = f.state();
    no_consts.consts.clear();
    KZ_CHECK(f.truth("TABLE = 0", no_consts));  // not given: 0
  }

  {  // the state of a resource: the initial state unless one was set; ordering follows the declaration
    ConcreteState st = f.state();
    KZ_CHECK(f.truth("state(slot(1)) = FREE", st));
    KZ_CHECK(f.truth("state(stage(slot(1))) = ZERO", st));
    st.state[f.rid(f.slot, 1)] = f.state_ord(f.slot, "RESERVED");
    KZ_CHECK(f.truth("state(slot(1)) = RESERVED && state(slot(2)) = FREE", st));
    KZ_CHECK(f.truth("state(slot(1)) > FREE && state(slot(1)) < LIVE && state(slot(1)) >= RESERVED && state(slot(1)) <= RESERVED", st));
    KZ_CHECK(f.truth("state(slot(1)) in {FREE, RESERVED}", st));
    KZ_CHECK(!f.truth("state(slot(1)) in {FREE, LIVE}", st));
    st.state[f.rid(f.stage, 2)] = f.state_ord(f.stage, "TWO");
    KZ_CHECK(f.truth("state(stage(slot(2))) = TWO && state(stage(slot(3))) = ZERO", st));
    KZ_CHECK(f.truth("state(stage(slot(2))) > ONE", st));
    // a resource built from an argument
    st.args = {0, 2, 0};
    KZ_CHECK(f.truth("state(stage(slot(arg.N))) = TWO", st));
    st.args = {0, 3, 0};
    KZ_CHECK(f.truth("state(stage(slot(arg.N))) = ZERO", st));
    // the key and the generation
    KZ_CHECK(f.truth("key(slot(3)) = 3", st));
    KZ_CHECK(f.truth("gen(slot(3)) = 0", st));
  }

  {  // valid(): (x - base) mod 2^32 <= serial_max, so the markers are never valid
    ConcreteState st = f.state();
    const struct {
      int64_t id;
      bool valid;
    } ids[] = {{0x1000, true}, {0x1001, true}, {0x10ff, true}, {0x1100, false}, {0xfff, false}, {0, false}, {0xffffffff, false},
               {0xffffff00, false}, {0x100001000ll, true}, {-1, false}};
    for (const auto& c : ids) {
      st.args = {c.id, 0, 0};
      KZ_CHECK_EQ(f.truth("valid(arg.Who)", st), c.valid);
    }
  }

  {  // identifiers, tags, locations
    ConcreteState st = f.state();
    KZ_CHECK(f.truth("id(slot(1)) = UNDEF", st));
    st.ids[f.rid(f.slot, 1)] = 0x1005;
    KZ_CHECK(f.truth("id(slot(1)) = 0x1005 && id(slot(2)) = 0", st));
    KZ_CHECK(f.truth("tag(slot(1), Type) = UNSET", st));
    st.tags[{f.rid(f.slot, 1), 0}] = 1;
    KZ_CHECK(f.truth("tag(slot(1), Type) = CORE && tag(slot(1), Type) != EXTERNAL && tag(slot(2), Type) = UNSET", st));
    KZ_CHECK(f.truth("value(Box.id) = 0", st));
    st.loc_values[f.box_loc] = 0x1010;
    KZ_CHECK(f.truth("value(Box.id) = 0x1010 && valid(value(Box.id))", st));
    st.loc_values[f.box_loc] = 0;
    KZ_CHECK(!f.truth("valid(value(Box.id))", st));
  }

  {  // the executing actor and its owner; the state of an undefined resource is the initial state
    ConcreteState st = f.state();
    KZ_CHECK(f.truth("state(slot(1)) = FREE", st));
    KZ_CHECK(f.truth("state(owner(exec)) = FREE", st));  // no owner: undefined resource, initial state
    KZ_CHECK(f.truth("id(owner(exec)) = UNDEF", st));
    KZ_CHECK(f.truth("tag(owner(exec), Type) = UNSET", st));
    KZ_CHECK(f.truth("owner(exec) = owner(exec)", st));
    st.has_owner = true;
    st.owner = f.rid(f.slot, 2);
    st.state[f.rid(f.slot, 2)] = f.state_ord(f.slot, "LIVE");
    st.ids[f.rid(f.slot, 2)] = 0x1002;
    KZ_CHECK(f.truth("state(owner(exec)) = LIVE && id(owner(exec)) = 0x1002 && owner(exec) = slot(2) && owner(exec) != slot(1)", st));
    st.result = -5;
    KZ_CHECK(f.truth("result = -5", st));
    st.exec = 4;
    KZ_CHECK(f.truth("exec = exec", st));
  }

  {  // derived kinds: the state follows other state
    ConcreteState st = f.state();
    KZ_CHECK(f.truth("state(self(exec)) = ABSENT", st));
    st.has_owner = true;
    st.owner = f.rid(f.slot, 2);
    KZ_CHECK(f.truth("state(self(exec)) = ABSENT", st));
    st.state[f.rid(f.slot, 2)] = f.state_ord(f.slot, "LIVE");
    KZ_CHECK(f.truth("state(self(exec)) = PRESENT", st));
    KZ_CHECK(f.truth("state(cell(Box.id)) = INVALID", st));
    st.loc_values[f.box_loc] = 0x1003;
    KZ_CHECK(f.truth("state(cell(Box.id)) = VALID", st));
  }

  {  // quantifiers range over the instances of the kind; an empty set is vacuous
    ConcreteState st = f.state();
    KZ_CHECK(f.truth("forall r: slot { state(r) = FREE }", st));
    KZ_CHECK(!f.truth("exists r: slot { state(r) != FREE }", st));
    st.state[f.rid(f.slot, 1)] = f.state_ord(f.slot, "LIVE");
    st.state[f.rid(f.slot, 3)] = f.state_ord(f.slot, "RESERVED");
    st.state[f.rid(f.stage, 1)] = f.state_ord(f.stage, "THREE");
    KZ_CHECK(!f.truth("forall r: slot { state(r) = FREE }", st));
    KZ_CHECK(f.truth("exists r: slot { state(r) = LIVE }", st));
    KZ_CHECK(f.truth("forall r: slot where state(r) = LIVE { state(stage(r)) >= TWO }", st));
    KZ_CHECK(!f.truth("forall r: slot where state(r) != FREE { state(stage(r)) >= TWO }", st));  // slot 3 is RESERVED and at ZERO
    KZ_CHECK(f.truth("forall r: slot where state(r) = LIVE && state(r) = FREE { false }", st));  // a guard that never holds
    KZ_CHECK(f.truth("exists r: slot { exists q: slot { state(r) = LIVE && state(q) = RESERVED } }", st));
    st.instances[f.slot].clear();
    KZ_CHECK(f.truth("forall r: slot { false }", st));
    KZ_CHECK(!f.truth("exists r: slot { true }", st));
  }

  // ---- the abstract domain -----------------------------------------------------------------------
  {
    AbstractState as;
    as.owner_none = true;
    as.exec = {1, 1};
    as.args = {{0, 0}, {0, 0}, {0, 0}};
    as.result = {0, 0};
    // a Boolean of unknown value
    AVal v = f.arun("true && false", as);
    KZ_CHECK_EQ(static_cast<int>(v.tri), static_cast<int>(Tri::False));
    // arguments as intervals: three-valued comparisons
    as.args[1] = {3, 9};
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N < 10", as).tri), static_cast<int>(Tri::True));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N < 3", as).tri), static_cast<int>(Tri::False));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N < 5", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N = 12", as).tri), static_cast<int>(Tri::False));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N != 12", as).tri), static_cast<int>(Tri::True));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N = 5", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N in {1, 2}", as).tri), static_cast<int>(Tri::False));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N in {1, 5}", as).tri), static_cast<int>(Tri::Maybe));
    // Kleene logic: false absorbs, true absorbs, unknown stays unknown
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N = 5 && false", as).tri), static_cast<int>(Tri::False));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N = 5 || true", as).tri), static_cast<int>(Tri::True));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.N = 5 && true", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("!(arg.N = 5)", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("(arg.N = 5) => (arg.N < 100)", as).tri), static_cast<int>(Tri::True));
    // valid() over an interval of identifiers
    as.args[0] = {0x1010, 0x1020};
    KZ_CHECK_EQ(static_cast<int>(f.arun("valid(arg.Who)", as).tri), static_cast<int>(Tri::True));
    as.args[0] = {0x10f0, 0x1110};
    KZ_CHECK_EQ(static_cast<int>(f.arun("valid(arg.Who)", as).tri), static_cast<int>(Tri::Maybe));
    as.args[0] = {0x2000, 0x3000};
    KZ_CHECK_EQ(static_cast<int>(f.arun("valid(arg.Who)", as).tri), static_cast<int>(Tri::False));
    as.args[0] = {0, 0xffffffffll};
    KZ_CHECK_EQ(static_cast<int>(f.arun("valid(arg.Who)", as).tri), static_cast<int>(Tri::Maybe));
    as.args[0] = {-5, 5};  // outside 32 bits: the concrete value wraps, so nothing is claimed
    KZ_CHECK_EQ(static_cast<int>(f.arun("valid(arg.Who)", as).tri), static_cast<int>(Tri::Maybe));
    // the state of a resource listed with a set of states
    AbstractRes r;
    r.states = (1ull << f.state_ord(f.slot, "RESERVED")) | (1ull << f.state_ord(f.slot, "LIVE"));
    as.res[ResId{f.slot, {1}}] = r;
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(slot(1)) = LIVE", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(slot(1)) = FREE", as).tri), static_cast<int>(Tri::False));
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(slot(1)) >= RESERVED", as).tri), static_cast<int>(Tri::True));
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(slot(1)) > RESERVED", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(slot(2)) = FREE", as).tri), static_cast<int>(Tri::True));  // not listed: the initial state
    // a resource whose key is an interval: the states of the listed resources in the box, and the initial state
    as.args[1] = {1, 2};
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(slot(arg.N)) = FREE", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(slot(arg.N)) = RESERVED", as).tri), static_cast<int>(Tri::Maybe));
  }
  {  // the owner: possible owners, or none
    AbstractState as;
    as.exec = {1, 1};
    as.args = {{0, 0}, {0, 0}, {0, 0}};
    as.owner_none = true;
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(owner(exec)) = FREE", as).tri), static_cast<int>(Tri::True));
    as.owner_none = false;
    as.owners = {ResId{f.slot, {2}}};
    AbstractRes r;
    r.states = 1ull << f.state_ord(f.slot, "LIVE");
    as.res[ResId{f.slot, {2}}] = r;
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(owner(exec)) = LIVE", as).tri), static_cast<int>(Tri::True));
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(self(exec)) = PRESENT", as).tri), static_cast<int>(Tri::True));
    as.owner_none = true;  // it may have no owner
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(owner(exec)) = LIVE", as).tri), static_cast<int>(Tri::Maybe));
    as.exec = {1, 3};  // another actor may be the one asked about
    as.owner_none = false;
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(owner(exec)) = LIVE", as).tri), static_cast<int>(Tri::Maybe));
  }
  {  // contains() / exact(): a concrete value is described by the abstraction of itself
    ConcreteState st = f.state();
    st.args = {0x1005, 7, kz::eval::str_handle("q")};
    for (const char* text : {"arg.N", "valid(arg.Who)", "state(slot(1))", "tag(slot(1), Type)", "owner(exec)", "slot(2)", "arg.S", "id(slot(3))"}) {
      const CVal c = f.run(text, st);
      KZ_CHECK(kz::eval::contains(kz::eval::exact(c), c));
    }
    CVal one;
    one.sort = kz::phi::SortKind::Int;
    one.v = 5;
    AVal interval = kz::eval::exact(one);
    interval.hi = 9;
    KZ_CHECK(kz::eval::contains(interval, one));
    one.v = 10;
    KZ_CHECK(!kz::eval::contains(interval, one));
    CVal undefined;
    undefined.sort = kz::phi::SortKind::Res;
    undefined.kind = f.slot;
    undefined.defined = false;
    AVal a = kz::eval::exact(undefined);
    KZ_CHECK(kz::eval::contains(a, undefined));
    CVal defined = undefined;
    defined.defined = true;
    defined.key = {1};
    KZ_CHECK(!kz::eval::contains(a, defined));
  }


  // ---- an Id is a 32-bit value in both domains (eval.hpp) ---------------------------------------------------
  {
    // The identifier 0x100000005 reaches the evaluator as the Id 5: a comparison with the edge of
    // the space is decided on the 32-bit value in the concrete domain, and an abstract state that
    // says nothing about the argument gives the same definite answer.
    ConcreteState st = f.state();
    st.args = {0x100000005ll, 0, 0, 0};
    AbstractState none;  // no argument described: all unknown
    none.exec = {1, 1};
    none.owner_none = true;
    for (const char* text : {"arg.Who > 0xffffffff", "arg.Who >= 0x100000000", "arg.Who < 0", "arg.Who = 0x100000005"}) {
      const CVal c = f.run(text, st);
      const AVal a = f.arun(text, none);
      KZ_CHECK(!c.is_true());
      KZ_CHECK_EQ(static_cast<int>(a.tri), static_cast<int>(Tri::False));
      KZ_CHECK(kz::eval::contains(a, c));
    }
    KZ_CHECK(f.truth("arg.Who = 5", st));                    // the residue is the value
    KZ_CHECK(f.truth("arg.Who <= 0xffffffff", st));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who = 5", none).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK(kz::eval::contains(f.arun("arg.Who = 5", none), f.run("arg.Who = 5", st)));
    st.args[0] = -1;
    KZ_CHECK(f.truth("arg.Who = 0xffffffff", st));           // a negative number is its residue
    KZ_CHECK(!f.truth("valid(arg.Who)", st));
    st.args[0] = 0x100001000ll;
    KZ_CHECK(f.truth("valid(arg.Who)", st));                 // 0x1000 is the base of the mini kind
    KZ_CHECK(f.truth("arg.Who = 0x1000", st));
    // the id a resource carries, the value of a location and a key are read the same way
    st.ids[ResId{f.slot, {1}}] = 0x200001001ll;
    KZ_CHECK(f.truth("id(slot(1)) = 0x1001", st));
    st.loc_values[f.box_loc] = -0xfffffffll;                  // 0xF0000001 mod 2^32
    KZ_CHECK(f.truth("value(Box.id) = 0xf0000001", st));
    // an abstract argument outside 32 bits is read modulo 2^32 as well
    AbstractState as = none;
    as.args = {{0x100001000ll, 0x100001000ll}, {0, 0}, {0, 0}, {0, 0}};
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who = 0x1000", as).tri), static_cast<int>(Tri::True));
    as.args[0] = {-1, -1};
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who = 0xffffffff", as).tri), static_cast<int>(Tri::True));
    as.args[0] = {-5, 5};          // the residues wrap around: the whole space
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who = 0xfffffffb", as).tri), static_cast<int>(Tri::Maybe));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who = 7", as).tri), static_cast<int>(Tri::Maybe));
    as.args[0] = {INT64_MIN, INT64_MAX};  // the span does not fit in an int64: no overflow, the whole space
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who < 0", as).tri), static_cast<int>(Tri::False));
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who > 0xffffffff", as).tri), static_cast<int>(Tri::False));
    as.args[0] = {0x1000, 0x2000};
    KZ_CHECK_EQ(static_cast<int>(f.arun("arg.Who > 0x2000", as).tri), static_cast<int>(Tri::False));
    KZ_CHECK_EQ(kz::eval::id_value(-1), static_cast<int64_t>(0xffffffffll));
    KZ_CHECK_EQ(kz::eval::id_value(0x100000005ll), static_cast<int64_t>(5));
    KZ_CHECK_EQ(kz::eval::id_value(0), static_cast<int64_t>(0));
  }

  // ---- a Loc-sorted value that the state does not constrain ------------------------------------------------
  {
    // An absent argument of sort Loc is the interval of all int64: the span of its locations must
    // not be computed in int64 (signed overflow), and every declared location is a possible value.
    AbstractState none;
    none.exec = {1, 1};
    none.owner_none = true;
    const AVal v = f.arun("state(cell(arg.L)) = VALID", none);
    KZ_CHECK_EQ(v.sort, kz::phi::SortKind::Bool);
    KZ_CHECK_EQ(static_cast<int>(v.tri), static_cast<int>(Tri::Maybe));
    const AVal whole = f.arun("value(arg.L)", none);
    KZ_CHECK(whole.lo <= 0 && whole.hi >= 0);
    // with the locations listed, the hull covers all of them and 0 (an undeclared location holds 0)
    none.locs[f.box_loc] = {0x1000, 0x1010};
    none.locs[f.aux_n] = {10, 20};
    none.locs[f.aux_m] = {0x1001, 0x1002};
    const AVal hull = f.arun("value(arg.L)", none);
    KZ_CHECK_EQ(hull.lo, static_cast<int64_t>(0));
    KZ_CHECK_EQ(hull.hi, static_cast<int64_t>(0x1010));
    KZ_CHECK(hull.sort == kz::phi::SortKind::Id);
    // each concrete location is inside the hull, declared or not
    for (int64_t li : {0ll, 1ll, 2ll, 3ll, 7ll, -1ll, 0x100000002ll}) {
      ConcreteState st = f.state();
      st.args = {0, 0, 0, li};
      st.loc_values[f.box_loc] = 0x1005;
      st.loc_values[f.aux_n] = 15;
      st.loc_values[f.aux_m] = 0x1002;
      const CVal c = f.run("value(arg.L)", st);
      KZ_CHECK(kz::eval::contains(hull, c));
    }
    // a bounded interval of locations: only those, and 0 if it reaches beyond the declared ones
    AbstractState two = none;
    two.args = {{0, 0}, {0, 0}, {0, 0}, {static_cast<int64_t>(f.aux_n), static_cast<int64_t>(f.aux_n)}};
    const AVal exact_n = f.arun("value(arg.L)", two);
    KZ_CHECK_EQ(exact_n.lo, static_cast<int64_t>(10));
    KZ_CHECK_EQ(exact_n.hi, static_cast<int64_t>(20));
    KZ_CHECK(exact_n.sort == kz::phi::SortKind::Int);                      // an Int location: not an identifier
    KZ_CHECK_EQ(static_cast<int>(f.arun("valid(value(arg.L))", two).tri), static_cast<int>(Tri::False));
    two.args[3] = {static_cast<int64_t>(f.aux_n), static_cast<int64_t>(f.aux_n) + 40};   // beyond the declared ones
    const AVal beyond = f.arun("value(arg.L)", two);
    KZ_CHECK(beyond.lo <= 0 && beyond.hi >= 0x1010);
    two.args[3] = {5, 2};  // an empty interval is malformed: everything is possible
    const AVal malformed = f.arun("value(arg.L)", two);
    KZ_CHECK(malformed.lo <= 0 && malformed.hi >= 0x1010);
    // a unbounded Loc inside a kind parameter: the same primitive, no crash
    KZ_CHECK_EQ(static_cast<int>(f.arun("state(cell(arg.L)) != INVALID", none).tri), static_cast<int>(Tri::Maybe));
  }

  // ---- a kind without an identifier space; contains() of resources -----------------------------------------
  {
    ConcreteState st = f.state();
    CVal id;
    id.sort = kz::phi::SortKind::Id;
    id.kind = f.plain;
    id.v = 5;
    ConcreteDomain cd(f.p, st);
    KZ_CHECK(!cd.valid(id).is_true());                 // no identifier space: no identifier is valid
    AbstractState as;
    as.exec = {1, 1};
    AbstractDomain ad(f.p, as);
    AVal aid = kz::eval::exact(id);
    KZ_CHECK_EQ(static_cast<int>(ad.valid(aid).tri), static_cast<int>(Tri::False));
    aid.kind = -1;                                    // a kind that is not known: nothing is claimed
    KZ_CHECK_EQ(static_cast<int>(ad.valid(aid).tri), static_cast<int>(Tri::Maybe));
    AVal not_an_id = kz::eval::exact(CVal());
    not_an_id.sort = kz::phi::SortKind::Int;
    KZ_CHECK_EQ(static_cast<int>(ad.valid(not_an_id).tri), static_cast<int>(Tri::False));
  }
  {
    // contains(): resources
    CVal defined;
    defined.sort = kz::phi::SortKind::Res;
    defined.kind = f.slot;
    defined.key = {2};
    CVal undefined = defined;
    undefined.defined = false;
    undefined.key.clear();
    AVal a = kz::eval::exact(defined);
    KZ_CHECK(kz::eval::contains(a, defined));
    KZ_CHECK(!kz::eval::contains(a, undefined));          // a resource that must be defined
    AVal widened = a;
    widened.may_be_undefined = true;
    KZ_CHECK(kz::eval::contains(widened, defined));
    KZ_CHECK(kz::eval::contains(widened, undefined));     // may be undefined
    AVal never = kz::eval::exact(undefined);
    KZ_CHECK(kz::eval::contains(never, undefined));
    KZ_CHECK(!kz::eval::contains(never, defined));        // must be undefined
    AVal other_kind = a;
    other_kind.kind = f.stage;
    KZ_CHECK(!kz::eval::contains(other_kind, defined));
    AVal wider_key = a;
    wider_key.key = {{1, 3}};
    KZ_CHECK(kz::eval::contains(wider_key, defined));
    CVal outside = defined;
    outside.key = {5};
    KZ_CHECK(!kz::eval::contains(wider_key, outside));
    CVal two_keys = defined;
    two_keys.key = {2, 2};
    KZ_CHECK(!kz::eval::contains(a, two_keys));           // a different key width
    CVal as_int;
    as_int.sort = kz::phi::SortKind::Int;
    as_int.v = 2;
    KZ_CHECK(!kz::eval::contains(a, as_int));              // a different sort
  }

  return kztest::finish("test_core_eval");
}

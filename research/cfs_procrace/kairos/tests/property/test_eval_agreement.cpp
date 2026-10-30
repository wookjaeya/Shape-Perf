// Property test of the evaluator: the abstract result contains the concrete result.
//
// For generated, well-typed expressions of the core language (over the mini profile), generated
// concrete states, and abstractions that contain each concrete state (an exact one and randomly
// widened ones), the abstract value of the expression contains the concrete value:
//
//     c in gamma(a)   =>   eval_concrete(e, c) in gamma(eval_abstract(e, a))
//
// The concretization gamma is defined in the comments of AbstractState (include/kz/core/eval.hpp).
// Three more checks keep the test from being vacuous:
//   - with the exact abstraction of a concrete state the result of a Boolean expression is
//     definite (never "maybe") and equal to the concrete one;
//   - a large share of the results with widened abstractions is definite as well;
//   - the same loop with a deliberately wrong abstraction (one that leaves the concrete value
//     out) does report violations: the test can fail.
//
// The generated inputs cover what a static analysis feeds the abstract domain when it knows little:
// arguments that the abstract state does not describe at all (unknown), an argument of sort Loc
// that names any location (declared or not), several locations of different sorts, a kind without
// an identifier space, and identifiers outside 32 bits (the domains read an Id modulo 2^32).
//
// Everything is deterministic: a fixed-seed generator, fixed counts.  The values are unit-test
// values, not research conditions.

#include <algorithm>
#include <cstdint>
#include <map>
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

// xorshift64*: small, deterministic, good enough for generating test inputs.
struct Rng {
  uint64_t s;
  explicit Rng(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ull + 0x1234567ull) {
    for (int i = 0; i < 4; ++i) next();
  }
  uint64_t next() {
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    return s * 0x2545F4914F6CDD1Dull;
  }
  uint64_t below(uint64_t n) { return n == 0 ? 0 : (next() >> 11) % n; }
  bool coin(unsigned percent) { return below(100) < percent; }
  template <class T>
  const T& pick(const std::vector<T>& v) {
    return v[static_cast<size_t>(below(v.size()))];
  }
};

int64_t sat_add(int64_t a, int64_t b) {
  if (b > 0 && a > INT64_MAX - b) return INT64_MAX;
  if (b < 0 && a < INT64_MIN - b) return INT64_MIN;
  return a + b;
}

// ---- the world the expressions are about ------------------------------------------------------

// The mini profile plus a kind without an identifier space and two more locations (one holds an
// Int, one an Id), so that a Loc-sorted value can name locations of different sorts.
const char* const kExtra = R"KZ(# extra declarations of the property test
phi 1;
kind plain(i: Int) { states P < Q; initial P; scope process; prov code_a; }
loc Aux.n { struct "Aux_t"; field "n"; holds Int; prov code_a; }
loc Aux.m { struct "Aux_t"; field "m"; holds Id<slot>; prov code_a; }
)KZ";

struct World {
  Profile p;
  kz::phi::ExprContext ctx;
  int32_t slot = -1, stage = -1, self = -1, reg = -1, gate = -1, cell = -1, plain = -1;
  int32_t box_loc = -1, aux_n = -1, aux_m = -1;
  std::vector<int64_t> table_consts;  // indices of the ref eval constants

  World() {
    std::vector<Diag> diags;
    std::vector<kz::phi::SourceText> files = kzmini::files();
    files.push_back(kz::phi::SourceText{"extra.kzp", kExtra});
    if (!kz::phi::load_profile(files, &p, &diags).ok()) {
      std::fprintf(stderr, "the property test profile does not load\n");
      for (const Diag& d : diags) std::fprintf(stderr, "  %s\n", kz::phi::format_diag(p.files, d).c_str());
    }
    plain = p.find_kind("plain");
    aux_n = p.find_loc("Aux.n");
    aux_m = p.find_loc("Aux.m");
    slot = p.find_kind("slot");
    stage = p.find_kind("stage");
    self = p.find_kind("self");
    reg = p.find_kind("reg");
    gate = p.find_kind("gate");
    cell = p.find_kind("cell");
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
    for (size_t i = 0; i < p.consts.size(); ++i)
      if (p.consts[i].value == kz::phi::ConstDecl::Value::RefEval) table_consts.push_back(static_cast<int64_t>(i));
  }
  size_t n_states(int32_t kind) const { return p.kinds[static_cast<size_t>(kind)].states.size(); }
};

// ---- expression generator -----------------------------------------------------------------------

class ExprGen {
 public:
  explicit ExprGen(Rng* rng) : r_(*rng) {}

  std::string boolean(int d) {
    if (d <= 0) return leaf();
    switch (r_.below(14)) {
      case 0: return "!(" + boolean(d - 1) + ")";
      case 1: return "(" + boolean(d - 1) + ") && (" + boolean(d - 1) + ")";
      case 2: return "(" + boolean(d - 1) + ") || (" + boolean(d - 1) + ")";
      case 3: return "(" + boolean(d - 1) + ") => (" + boolean(d - 1) + ")";
      case 4: return "(" + boolean(d - 1) + ") && (" + boolean(d - 1) + ") && (" + boolean(d - 1) + ")";
      case 5: return quantifier(d);
      case 6: return in_set(d);
      case 7: return "(" + boolean(d - 1) + ") = (" + boolean(d - 1) + ")";
      default: return leaf();
    }
  }

 private:
  Rng& r_;
  std::vector<std::string> slot_vars_, stage_vars_;
  int next_var_ = 0;

  static const char* cmp_op(uint64_t k) {
    static const char* const ops[] = {"=", "!=", "<", "<=", ">", ">="};
    return ops[k % 6];
  }
  const char* eq_op() { return r_.coin(50) ? "=" : "!="; }

  std::string leaf() {
    switch (r_.below(14)) {
      case 12: return std::string("state(plain(") + integer(1) + ")) " + cmp_op(r_.below(6)) + (r_.coin(50) ? " P" : " Q");
      case 13: return std::string("state(cell(arg.L)) ") + eq_op() + (r_.coin(50) ? " VALID" : " INVALID");
      case 0: return r_.coin(50) ? "true" : "false";
      case 1:
      case 2: return "(" + integer(2) + ") " + cmp_op(r_.below(6)) + " (" + integer(2) + ")";
      case 3: return "valid(" + identifier(2) + ")";
      case 4: return "(" + identifier(2) + ") " + cmp_op(r_.below(6)) + " (" + integer(1) + ")";
      case 5: return "(" + identifier(2) + ") " + cmp_op(r_.below(6)) + " (" + identifier(2) + ")";
      case 6: return state_cmp();
      case 7: return state_cmp();
      case 8: return tag_cmp();
      case 9: return res_eq();
      case 10: return str_cmp();
      default: return "state(" + res_slot(2) + ") " + cmp_op(r_.below(6)) + " " + slot_state();
    }
  }

  std::string str_cmp() {
    static const char* const lits[] = {"\"x\"", "\"y\"", "\"\"", "NAME"};
    const std::string a = r_.coin(50) ? "arg.S" : lits[r_.below(4)];
    const std::string b = lits[r_.below(4)];
    return a + " " + eq_op() + " " + b;
  }

  std::string slot_state() {
    static const char* const s[] = {"FREE", "RESERVED", "LIVE"};
    return s[r_.below(3)];
  }

  std::string state_cmp() {
    switch (r_.below(7)) {
      case 0: return "state(" + res_slot(2) + ") " + cmp_op(r_.below(6)) + " " + slot_state();
      case 1: return "state(" + res_slot(2) + ") " + cmp_op(r_.below(6)) + " state(" + res_slot(2) + ")";
      case 2: {
        static const char* const s[] = {"ZERO", "ONE", "TWO", "THREE"};
        return "state(" + res_stage(2) + ") " + cmp_op(r_.below(6)) + " " + s[r_.below(4)];
      }
      case 3: return std::string("state(self(exec)) ") + eq_op() + (r_.coin(50) ? " PRESENT" : " ABSENT");
      case 4: return "state(reg(" + identifier(2) + ")) " + eq_op() + (r_.coin(50) ? " REG" : " UNREG");
      case 5: return std::string("state(gate()) ") + cmp_op(r_.below(6)) + (r_.coin(33) ? " OPEN" : r_.coin(50) ? " CLOSING" : " CLOSED");
      default: return std::string("state(cell(Box.id)) ") + eq_op() + (r_.coin(50) ? " VALID" : " INVALID");
    }
  }

  std::string tag_cmp() {
    static const char* const t[] = {"UNSET", "CORE", "EXTERNAL"};
    return "tag(" + res_slot(2) + ", Type) " + eq_op() + " " + t[r_.below(3)];
  }

  std::string res_eq() {
    switch (r_.below(3)) {
      case 0: return res_slot(2) + " " + eq_op() + " " + res_slot(2);
      case 1: return res_stage(2) + " " + eq_op() + " " + res_stage(2);
      default: return "owner(exec) " + std::string(eq_op()) + " " + res_slot(1);
    }
  }

  std::string in_set(int d) {
    switch (r_.below(3)) {
      case 0: return "(" + integer(d - 1) + ") in {" + integer(1) + ", " + integer(1) + ", " + integer(0) + "}";
      case 1: return "state(" + res_slot(2) + ") in {" + slot_state() + ", " + slot_state() + "}";
      default: return "(" + identifier(1) + ") in {" + integer(0) + ", " + identifier(1) + "}";
    }
  }

  std::string quantifier(int d) {
    const bool over_slot = r_.coin(70);
    const std::string v = "v" + std::to_string(next_var_++);
    std::string guard;
    (over_slot ? slot_vars_ : stage_vars_).push_back(v);
    if (r_.coin(60)) guard = " where " + boolean(d - 1);
    const std::string body = boolean(d - 1);
    (over_slot ? slot_vars_ : stage_vars_).pop_back();
    return std::string(r_.coin(50) ? "forall " : "exists ") + v + ": " + (over_slot ? "slot" : "stage") + guard + " { " + body + " }";
  }

  std::string res_slot(int d) {
    switch (r_.below(d <= 0 ? 2 : 4)) {
      case 0: return "owner(exec)";
      case 1:
        if (!slot_vars_.empty()) return slot_vars_[static_cast<size_t>(r_.below(slot_vars_.size()))];
        return "slot(" + std::to_string(r_.below(5)) + ")";
      default: return "slot(" + integer(d - 1) + ")";
    }
  }

  std::string res_stage(int d) {
    if (!stage_vars_.empty() && r_.coin(40)) return stage_vars_[static_cast<size_t>(r_.below(stage_vars_.size()))];
    return "stage(" + res_slot(d - 1) + ")";
  }

  std::string integer(int d) {
    if (d <= 0 || r_.coin(40)) {
      switch (r_.below(10)) {
        case 0: return std::to_string(r_.below(6));
        case 1: return "arg.N";
        case 2: return "result";
        case 3: return r_.coin(50) ? "BASE" : "SERIAL_MAX";
        case 4: return r_.coin(50) ? "LIMIT" : "TABLE";
        case 5: return r_.coin(50) ? "UNDEF" : "-1";
        case 6: return r_.coin(50) ? "0x10ff" : "0x1100";
        case 7: return r_.coin(50) ? "0xffffffff" : "0x100000000";  // the edge of the 32-bit identifier space
        case 8: return r_.coin(50) ? "value(Aux.n)" : "value(arg.L)";
        default: return r_.coin(50) ? "0xfff" : "4096";
      }
    }
    switch (r_.below(3)) {
      case 0: return "key(" + res_slot(d - 1) + ")";
      case 1: return "gen(" + res_slot(d - 1) + ")";
      default: return "(" + integer(d - 1) + ")";
    }
  }

  std::string identifier(int d) {
    if (d <= 0 || r_.coin(30)) {
      switch (r_.below(6)) {
        case 0: return "value(Box.id)";
        case 1: return "value(Aux.m)";
        case 2: return "value(arg.L)";
        default: return "arg.Who";
      }
    }
    switch (r_.below(3)) {
      case 0: return "id(" + res_slot(d - 1) + ")";
      case 1: return "id(owner(exec))";
      default: return "arg.Who";
    }
  }
};

// ---- concrete states ---------------------------------------------------------------------------------

struct StateGen {
  World& w;
  Rng& r;
  bool in_range_ids;

  std::vector<int64_t> id_pool() const {
    std::vector<int64_t> v = {0x1000, 0x1001, 0x1005, 0x10ff, 0x1100, 0xfff, 0, 0xffffffffll, 0x2000, 0xffffff00ll};
    if (!in_range_ids) {
      v.push_back(0x100001000ll);
      v.push_back(-1);
      v.push_back(-4096);
    }
    return v;
  }
  // An owner that is not a record of the context's owner kind (slot, one key element): another kind,
  // or the slot kind with a key of the wrong width.  Both domains read it as undefined (eval.hpp).
  ResId ill_formed_owner() {
    switch (r.below(3)) {
      case 0: return ResId{w.stage, {static_cast<int64_t>(r.below(5))}};
      case 1: return ResId{w.slot, {static_cast<int64_t>(r.below(5)), 1}};
      default: return ResId{w.gate, {}};
    }
  }
  std::vector<int64_t> int_pool() const { return {0, 1, 2, 3, 4, 5, -1, 7, 0xfff, 0x1000, 0x1100, 100}; }

  ConcreteState make() {
    ConcreteState st = kz::eval::initial_state(w.p);
    // arg.L names a declared location, one past the declared ones, or one that is far out
    static const int64_t locs[] = {0, 1, 2, 3, 4, -1, 100, 0x100000001ll};
    st.args = {r.pick(id_pool()), r.pick(int_pool()), r.coin(50) ? kz::eval::str_handle("x") : (r.coin(50) ? kz::eval::str_handle("y") : 0), locs[r.below(8)]};
    st.exec = static_cast<int64_t>(1 + r.below(3));
    st.result = r.pick(int_pool());
    const int n_slot = static_cast<int>(r.below(5)), n_stage = static_cast<int>(r.below(5));
    for (int i = 0; i < n_slot; ++i) st.instances[w.slot].push_back({i});
    for (int i = 0; i < n_stage; ++i) st.instances[w.stage].push_back({i});
    st.has_owner = r.coin(60);
    if (st.has_owner) st.owner = r.coin(85) ? ResId{w.slot, {static_cast<int64_t>(r.below(5))}} : ill_formed_owner();
    for (int64_t ci : w.table_consts) st.consts[static_cast<int32_t>(ci)] = r.coin(70) ? static_cast<int64_t>(r.below(8)) : 4;
    for (int64_t k = 0; k < 6; ++k) {
      if (r.coin(55)) st.state[ResId{w.slot, {k}}] = static_cast<int32_t>(r.below(w.n_states(w.slot)));
      if (r.coin(45)) st.state[ResId{w.stage, {k}}] = static_cast<int32_t>(r.below(w.n_states(w.stage)));
      if (r.coin(40)) st.tags[{ResId{w.slot, {k}}, 0}] = static_cast<int32_t>(r.below(3));
      if (r.coin(40)) st.ids[ResId{w.slot, {k}}] = r.pick(id_pool());
    }
    for (int64_t id : id_pool())
      if (r.coin(25)) st.state[ResId{w.reg, {id}}] = 1;
    if (r.coin(50)) st.state[ResId{w.gate, {}}] = static_cast<int32_t>(r.below(3));
    st.loc_values[w.box_loc] = r.coin(30) ? 0 : r.pick(id_pool());
    if (r.coin(70)) st.loc_values[w.aux_n] = r.pick(int_pool());
    if (r.coin(70)) st.loc_values[w.aux_m] = r.pick(id_pool());
    return st;
  }
};

// ---- abstractions -------------------------------------------------------------------------------------

std::pair<int64_t, int64_t> widen(Rng& r, int64_t v, bool exact) {
  if (exact) return {v, v};
  static const int64_t pool[] = {0, 0, 1, 3, 0x100, 0x1000, 1ll << 33};
  const int64_t lo = sat_add(v, -pool[r.below(7)]);
  const int64_t hi = sat_add(v, pool[r.below(7)]);
  return {lo, hi};
}

AbstractState abstract_of(World& w, const ConcreteState& c, Rng& r, bool exact) {
  AbstractState a;
  a.exec = widen(r, c.exec, exact);
  a.result = widen(r, c.result, exact);
  for (int64_t v : c.args) a.args.push_back(widen(r, v, exact));
  // An argument the abstraction does not describe is unknown (any value of its sort): drop a tail.
  if (!exact && r.coin(30)) a.args.resize(static_cast<size_t>(r.below(a.args.size() + 1)));
  a.instances = c.instances;
  a.owner_none = c.has_owner ? (!exact && r.coin(40)) : true;
  // Extra listed owners, some of them ill-formed (another kind or key width): a listed owner that is
  // not the concrete one only widens the abstraction.
  auto extra_owner = [&]() {
    switch (r.below(5)) {
      case 0: return ResId{w.stage, {static_cast<int64_t>(r.below(5))}};
      case 1: return ResId{w.slot, {static_cast<int64_t>(r.below(5)), 2}};
      default: return ResId{w.slot, {static_cast<int64_t>(r.below(5))}};
    }
  };
  if (c.has_owner) {
    a.owners.push_back(c.owner);
    if (!exact && r.coin(40)) a.owners.push_back(extra_owner());
    if (!exact && r.coin(20)) a.owners.insert(a.owners.begin(), extra_owner());
  } else if (!exact && r.coin(40)) {
    a.owners.push_back(extra_owner());
  }
  for (const auto& kv : c.consts) {
    if (exact || r.coin(70)) a.consts[kv.first] = widen(r, kv.second, exact);
  }
  for (const auto& kv : c.loc_values) a.locs[kv.first] = widen(r, kv.second, exact);
  if (!exact && r.coin(30)) a.locs[w.box_loc] = widen(r, c.loc_values.count(w.box_loc) ? c.loc_values.at(w.box_loc) : 0, false);
  if (!exact && r.coin(20)) a.locs[w.aux_m] = widen(r, c.loc_values.count(w.aux_m) ? c.loc_values.at(w.aux_m) : 0, false);

  // the resources that c lists
  std::map<ResId, bool> listed;
  for (const auto& kv : c.state) listed[kv.first] = true;
  for (const auto& kv : c.tags) listed[kv.first.first] = true;
  for (const auto& kv : c.ids) listed[kv.first] = true;
  if (!exact)
    for (int64_t k = 0; k < 6; ++k)
      if (r.coin(20)) listed[ResId{w.slot, {k}}] = true;  // listed but in the initial state: "unknown"
  for (const auto& kv : listed) {
    const kz::phi::KindDecl& kind = w.p.kinds[static_cast<size_t>(kv.first.kind)];
    AbstractRes ar;
    const auto st = c.state.find(kv.first);
    const int32_t actual = st != c.state.end() ? st->second : kind.initial_ordinal;
    ar.states = 1ull << actual;
    if (!exact)
      for (size_t i = 0; i < kind.states.size(); ++i)
        if (r.coin(25)) ar.states |= 1ull << i;
    for (size_t ti = 0; ti < kind.tags.size(); ++ti) {
      const auto tv = c.tags.find({kv.first, static_cast<int32_t>(ti)});
      if (tv == c.tags.end() && (exact || r.coin(50))) continue;  // absent: the first value only
      uint64_t m = 1ull << (tv != c.tags.end() ? tv->second : 0);
      if (!exact)
        for (size_t i = 0; i < kind.tags[ti].values.size(); ++i)
          if (r.coin(25)) m |= 1ull << i;
      ar.tags[static_cast<int32_t>(ti)] = m;
    }
    const auto idv = c.ids.find(kv.first);
    if (idv != c.ids.end()) {
      ar.has_id = true;
      ar.id = widen(r, idv->second, exact);
    } else if (!exact && kind.ids.present && r.coin(40)) {
      ar.has_id = true;
      ar.id = widen(r, kind.ids.undefined, false);
    }
    a.res[kv.first] = ar;
  }
  return a;
}

// Opens an abstraction the way a real record does (SPEC 4.8 OB-05): some kinds become open (their
// unlisted resources may be in any state, with any tag and id) and some of their listed resources are
// dropped, unlisted locations may become unknown, and the owner may become unknown.  Every concrete
// state the abstraction described is still described, so no evaluation may leave it out.
void open_up(AbstractState* a, const World& w, Rng& r) {
  for (size_t ki = 0; ki < w.p.kinds.size(); ++ki) {
    if (!r.coin(40)) continue;
    const int32_t kind = static_cast<int32_t>(ki);
    a->open_kinds.insert(kind);
    for (auto it = a->res.begin(); it != a->res.end();) {
      if (it->first.kind == kind && r.coin(50)) it = a->res.erase(it);
      else ++it;
    }
  }
  if (r.coin(50)) {
    a->open_locs = true;
    for (auto it = a->locs.begin(); it != a->locs.end();) {
      if (r.coin(50)) it = a->locs.erase(it);
      else ++it;
    }
  }
  if (r.coin(30)) a->owner_any = true;
}

// Moves an abstraction so that it leaves the concrete state out: the test must notice.
void break_abstraction(AbstractState* a, const ConcreteState& c) {
  for (size_t i = 0; i < a->args.size(); ++i) a->args[i] = {sat_add(c.args[i], 1), sat_add(c.args[i], 2)};
  a->exec = {c.exec + 1, c.exec + 2};
}

struct Totals {
  long evaluations = 0, violations = 0, definite = 0, bool_results = 0, exact_checked = 0, exact_failures = 0, exact_definite = 0;
};

// Runs `n_expr` expressions against `n_state` states each.  mode 0: exact abstraction, 1: widened, 2: broken,
// 3: widened and opened as a real record opens it.
void run(World& w, uint64_t seed, int n_expr, int n_state, int mode, bool in_range_ids, Totals* t) {
  Rng r(seed);
  ExprGen gen(&r);
  StateGen sg{w, r, in_range_ids};
  for (int i = 0; i < n_expr; ++i) {
    const std::string text = gen.boolean(1 + static_cast<int>(r.below(4)));
    kz::phi::ExprPtr e;
    std::vector<Diag> diags;
    if (!kz::phi::parse_expression(text, &e, &diags).ok()) {
      std::fprintf(stderr, "generated text does not parse: %s\n", text.c_str());
      ++kztest::state().failures;
      return;
    }
    kz::phi::Sort want;
    want.kind = kz::phi::SortKind::Bool;
    if (!kz::phi::resolve_expression(w.p, e.get(), w.ctx, want, &diags)) {
      std::fprintf(stderr, "generated text does not resolve: %s\n", text.c_str());
      for (const Diag& d : diags) std::fprintf(stderr, "  %s\n", d.message.c_str());
      ++kztest::state().failures;
      return;
    }
    for (int j = 0; j < n_state; ++j) {
      ConcreteState c = sg.make();
      AbstractState a = abstract_of(w, c, r, mode == 0);
      if (mode == 2) break_abstraction(&a, c);
      if (mode == 3) open_up(&a, w, r);
      ConcreteDomain cd(w.p, c);
      AbstractDomain ad(w.p, a);
      const CVal cv = Evaluator<ConcreteDomain>(w.p, cd).eval(*e);
      const AVal av = Evaluator<AbstractDomain>(w.p, ad).eval(*e);
      ++t->evaluations;
      ++t->bool_results;
      if (av.tri != Tri::Maybe) ++t->definite;
      if (!kz::eval::contains(av, cv)) {
        ++t->violations;
        if (mode != 2 && t->violations <= 5)
          std::fprintf(stderr, "VIOLATION (mode %d, seed %llu): %s\n  concrete %lld, abstract tri %d\n", mode, static_cast<unsigned long long>(seed), text.c_str(),
                       static_cast<long long>(cv.v), static_cast<int>(av.tri));
      }
      if (mode == 0) {
        ++t->exact_checked;
        if (av.tri == Tri::Maybe) {
          ++t->exact_failures;
          if (t->exact_failures <= 5) std::fprintf(stderr, "IMPRECISE under the exact abstraction: %s\n", text.c_str());
        } else {
          ++t->exact_definite;
        }
      }
    }
  }
}

}  // namespace

int main() {
  World w;
  KZ_CHECK(w.p.checked);

  // 1. exact abstractions (identifiers within 32 bits): sound and exact
  Totals exact;
  run(w, 1, 1500, 12, 0, true, &exact);
  std::printf("exact abstraction: %ld evaluations, %ld violations, %ld imprecise\n", exact.evaluations, exact.violations, exact.exact_failures);
  KZ_CHECK_EQ(exact.violations, 0L);
  KZ_CHECK_EQ(exact.exact_failures, 0L);
  KZ_CHECK(exact.evaluations >= 18000);

  // 2. widened abstractions, several seeds, identifiers also outside 32 bits
  Totals wide;
  for (uint64_t seed = 2; seed <= 7; ++seed) run(w, seed, 1200, 10, 1, seed % 2 == 0, &wide);
  std::printf("widened abstractions: %ld evaluations, %ld violations, %ld definite results\n", wide.evaluations, wide.violations, wide.definite);
  KZ_CHECK_EQ(wide.violations, 0L);
  KZ_CHECK(wide.evaluations >= 70000);
  // the abstraction says something: a good part of the results is true or false, not maybe
  KZ_CHECK(wide.definite * 100 >= wide.evaluations * 15);

  // 2b. abstractions opened as a real record opens them (open kinds, unknown locations, unknown owner)
  Totals opened;
  for (uint64_t seed = 11; seed <= 14; ++seed) run(w, seed, 1200, 10, 3, seed % 2 == 0, &opened);
  std::printf("opened abstractions: %ld evaluations, %ld violations, %ld definite results\n", opened.evaluations, opened.violations, opened.definite);
  KZ_CHECK_EQ(opened.violations, 0L);
  KZ_CHECK(opened.evaluations >= 45000);

  // 3. the test can fail: with abstractions that leave the concrete state out, violations appear
  Totals broken;
  run(w, 99, 600, 10, 2, true, &broken);
  std::printf("deliberately wrong abstractions: %ld evaluations, %ld violations\n", broken.evaluations, broken.violations);
  KZ_CHECK(broken.violations > 0);

  return kztest::finish("test_property_eval_agreement");
}

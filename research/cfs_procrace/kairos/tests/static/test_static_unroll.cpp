// The whole-program rules of the certificate view (lib/llvm/unroll.cpp), on synthetic models:
// partial evaluation against an element image, the unrolled iterations of a table walk, the
// task-main parameters of spawning functions, and the rule of each loop (unroll, await, star).
#include <string>
#include <vector>

#include "support/kz_test.hpp"
#include "unroll.hpp"

using namespace kzs;

namespace {

ImageFact img(const std::string& object, const std::string& path, uint8_t kind, const std::string& text = "", int64_t value = 0, const std::string& fn_key = "") {
  ImageFact f;
  f.object = object;
  f.path = path;
  f.kind = kind;
  f.text = text;
  f.value = value;
  f.fn_key = fn_key;
  f.pos.file = "t.c";
  f.pos.line = 7;
  return f;
}

Pos at(uint32_t line, uint32_t col = 1) {
  Pos p;
  p.file = "t.c";
  p.line = line;
  p.col = col;
  return p;
}

CallFact call(uint32_t line, const std::string& caller, const std::string& callee, const std::string& loop_key = "") {
  CallFact c;
  c.pos = at(line);
  c.caller_key = caller;
  c.caller_name = caller;
  c.callee_key = callee;
  c.callee_name = callee;
  c.kind = callee.empty() ? KZ_CALL_KIND_INDIRECT : KZ_CALL_KIND_DIRECT;
  c.loop_key = loop_key;
  return c;
}
std::string key_of(const CallFact& c) { return c.pos.str() + "#" + std::to_string(c.ordinal); }

ElemLookup lookup_of(const std::map<std::string, ElemValue>& fields) {
  return [fields](const std::string& f) {
    auto it = fields.find(f);
    return it == fields.end() ? ElemValue() : it->second;
  };
}
ElemValue known(uint8_t kind, int64_t value = 0, const std::string& text = "") {
  ElemValue v;
  v.known = true;
  v.kind = kind;
  v.value = value;
  v.text = text;
  return v;
}

}  // namespace

int main() {
  // ---- partial evaluation ---------------------------------------------------------------------------
  {
    ExprPool p;
    const ElemLookup L = lookup_of({{"task", known(KZ_IMAGE_KIND_FUNC, 0, "b_task")},
                                    {"init", known(KZ_IMAGE_KIND_NULLPTR)},
                                    {"prio", known(KZ_IMAGE_KIND_INT, 5)},
                                    {"name", known(KZ_IMAGE_KIND_STRING, 0, "b")},
                                    {"", known(KZ_IMAGE_KIND_ADDR, 0, "mod_b")}});
    ExprRef task = p.load(KZ_EXPR_REF_ELEM, "task"), init = p.load(KZ_EXPR_REF_ELEM, "init"), prio = p.load(KZ_EXPR_REF_ELEM, "prio");
    ExprRef self = p.load(KZ_EXPR_REF_ELEM, ""), other = p.load(KZ_EXPR_REF_ELEM, "missing");
    ExprRef zero = p.constant(0), x = p.binary(KZ_EXPR_OP_NE, p.load(KZ_EXPR_REF_LOCAL, "x"), zero);
    // a known function, string or address is a non-null pointer; NULL is 0
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_EQ, task, zero), L)), 0);
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_NE, task, zero), L)), 1);
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_NE, init, zero), L)), 0);
    KZ_CHECK_EQ(truth(peval(&p, p.unary(KZ_EXPR_OP_NOT, p.binary(KZ_EXPR_OP_EQ, self, zero)), L)), 1);
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_NE, p.load(KZ_EXPR_REF_ELEM, "name"), zero), L)), 1);
    // integer fields fold through comparisons
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_LT, prio, p.constant(6)), L)), 1);
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_GE, prio, p.constant(6)), L)), 0);
    // two functions compare by name
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_EQ, task, p.load(KZ_EXPR_REF_FUNC, "b_task")), L)), 1);
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_EQ, task, p.load(KZ_EXPR_REF_FUNC, "a_task")), L)), 0);
    // what the image does not know stays, and the connectives fold around it
    ExprRef unknown_field = p.binary(KZ_EXPR_OP_NE, other, zero);
    KZ_CHECK(peval(&p, unknown_field, L) == unknown_field);
    KZ_CHECK_EQ(truth(peval(&p, unknown_field, L)), -1);
    KZ_CHECK(peval(&p, p.binary(KZ_EXPR_OP_AND, p.binary(KZ_EXPR_OP_NE, task, zero), x), L) == x);
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_AND, p.binary(KZ_EXPR_OP_EQ, task, zero), x), L)), 0);
    KZ_CHECK_EQ(truth(peval(&p, p.binary(KZ_EXPR_OP_OR, x, p.binary(KZ_EXPR_OP_NE, task, zero)), L)), 1);
    KZ_CHECK(peval(&p, p.binary(KZ_EXPR_OP_OR, x, p.binary(KZ_EXPR_OP_EQ, task, zero)), L) == x);
    // a local is never folded; a null tree stays null
    KZ_CHECK_EQ(truth(peval(&p, x, L)), -1);
    KZ_CHECK(peval(&p, nullptr, L) == nullptr);
    KZ_CHECK_EQ(truth(nullptr), -1);
  }

  // ---- unrolled iterations of a table walk -----------------------------------------------------------
  {
    Model m;
    // the elements: mod_a (no task), mod_b (task b_task, prio 5); the entries of the walked table
    m.object_images["mod_a"] = {img("mod_a", ".name", KZ_IMAGE_KIND_STRING, "a"), img("mod_a", ".task", KZ_IMAGE_KIND_NULLPTR)};
    m.object_images["mod_b"] = {img("mod_b", ".name", KZ_IMAGE_KIND_STRING, "b"), img("mod_b", ".task", KZ_IMAGE_KIND_FUNC, "b_task", 0, "b_task"),
                                img("mod_b", ".init", KZ_IMAGE_KIND_FUNC, "b_init", 0, "b_init")};
    m.object_pos["mod_b"] = at(30);
    std::map<std::string, std::vector<ImageFact>> entries;
    entries["W"] = {img("list", "[0]", KZ_IMAGE_KIND_ADDR, "mod_a"), img("list", "[1]", KZ_IMAGE_KIND_ADDR, "mod_b")};
    // the calls of the loop body: a spawn guarded by `e->task != 0` and a local, and an indirect
    // call through e->init
    CallFact spawn = call(40, "spawn_all", "make_task", "W");
    CallFact ind = call(41, "spawn_all", "", "W");
    m.calls = {spawn, ind};
    m.spawn_calls = {};
    m.spawn_entry_arg[key_of(spawn)] = 1;
    ExprPool& p = m.exprs;
    WalkUse u1;
    u1.loop_key = "W";
    u1.call_key = key_of(spawn);
    u1.guard = p.binary(KZ_EXPR_OP_AND, p.unary(KZ_EXPR_OP_NOT, p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_ELEM, "task"), p.constant(0))),
                        p.binary(KZ_EXPR_OP_NE, p.load(KZ_EXPR_REF_LOCAL, "ok"), p.constant(0)));
    u1.args = {{0, p.load(KZ_EXPR_REF_ELEM, "name")}, {1, p.load(KZ_EXPR_REF_ELEM, "task")}};
    WalkUse u2;
    u2.loop_key = "W";
    u2.call_key = key_of(ind);
    u2.callee_field = "init";
    m.walk_uses = {u1, u2};
    unroll_table_walks(&m, entries);
    // per iteration: the element row, then per call its row and the value rows
    std::vector<std::string> got;
    for (const IterFact& f : m.iters) {
      std::string s = std::to_string(f.iter) + ":" + std::to_string(f.kind) + ":" + std::to_string(f.status) + ":" + f.text;
      if (f.kind == KZ_ITER_KIND_ARG) s += ":arg" + std::to_string(f.arg_index) + ":spawn" + std::to_string(f.spawn);
      if (f.guard != nullptr) s += ":" + expr_text(f.guard);
      got.push_back(s);
    }
    const std::vector<std::string> want = {
        "0:1:1:mod_a",                                  // element mod_a
        "0:2:2:",                                       // the spawn is skipped: task is NULL
        "0:2:1:",                                       // the indirect call is taken (no guard) ...
        "0:4:1:field init not in the image",            // ... through a field the image does not have
        "1:1:1:mod_b",                                  // element mod_b
        "1:2:3::(ne local:ok 0)",                       // residual: only the local is left
        "1:3:3:b:arg0:spawn0",                          // the name argument
        "1:3:3:b_task:arg1:spawn1",                     // the entry argument of the spawn
        "1:2:1:",                                       // the indirect call
        "1:4:1:b_init",                                 // calls b_init
    };
    KZ_CHECK_EQ(got.size(), want.size());
    for (size_t i = 0; i < got.size() && i < want.size(); ++i) KZ_CHECK_EQ(got[i], want[i]);
    // the element row of mod_b points at the object's definition
    KZ_CHECK_EQ(m.iters[4].pos.line, uint32_t(30));
  }

  // ---- task-main parameters ------------------------------------------------------------------------------
  {
    Model m;
    // start(f): stores f in a record field and spawns `tramp`; tramp calls the field.  Also
    // direct(g): passes g as the spawn's entry argument.
    FuncFact fs;
    fs.key = fs.name = "start";
    fs.nparams = 1;
    FuncFact fd;
    fd.key = fd.name = "direct";
    fd.nparams = 2;
    m.funcs["start"] = fs;
    m.funcs["direct"] = fd;
    CallFact sp1 = call(10, "start", "spawn");
    CallFact ic = call(20, "tramp", "");
    CallFact sp2 = call(30, "direct", "spawn");
    sp2.ordinal = 0;
    m.calls = {sp1, ic, sp2};
    m.spawn_calls = {key_of(sp1), key_of(sp2)};
    m.spawn_entry_nodes[key_of(sp1)] = {"F:tramp"};
    m.spawn_entry_nodes[key_of(sp2)] = {"P:direct#1"};
    m.indirect_calls = {IndirectCall{key_of(ic), "Rec.main"}};
    m.flow = {{"P:start#0", "Rec.main"}, {"F:tramp", "?:unused"}};
    TargetFact t;
    t.call_key = key_of(sp1);
    t.fn_key = "tramp";
    t.kind = KZ_CALL_TARGET_KIND_SPAWN_ENTRY;
    m.targets = {t};
    ProgramGraph g;
    for (const CallFact& c : m.calls) g.call_caller[key_of(c)] = c.caller_key;
    find_spawn_params(&m, g);
    KZ_CHECK_EQ(m.spawn_params.size(), size_t(2));
    if (m.spawn_params.size() == 2) {
      KZ_CHECK_EQ(m.spawn_params[0].func_key, std::string("direct"));
      KZ_CHECK_EQ(m.spawn_params[0].param, uint32_t(1));
      KZ_CHECK_EQ(m.spawn_params[0].kind, uint8_t(KZ_SPAWN_PARAM_KIND_DIRECT));
      KZ_CHECK_EQ(m.spawn_params[1].func_key, std::string("start"));
      KZ_CHECK_EQ(m.spawn_params[1].kind, uint8_t(KZ_SPAWN_PARAM_KIND_VIA_ENTRY));
      KZ_CHECK_EQ(m.spawn_params[1].entry_key, std::string("tramp"));
      KZ_CHECK_EQ(m.spawn_params[1].call_key, key_of(ic));
    }
  }

  // ---- the rule of each loop ---------------------------------------------------------------------------------
  {
    Model m;
    ExprPool& p = m.exprs;
    ExtractSpec spec;
    spec.fields.push_back(TargetField{FieldRef{"Rec", "state"}, "test"});
    spec.fields.push_back(TargetField{FieldRef{"Gate", "state"}, "test"});
    LocFact rec_state;
    rec_state.loc_type = "Rec";
    rec_state.loc_field = "state";
    rec_state.status = KZ_LOC_STATUS_COMPLETE;
    LocFact gate_state = rec_state;
    gate_state.loc_type = "Gate";
    m.locs = {gate_state, rec_state};
    auto loop = [&](const std::string& key, uint8_t kind, uint8_t trip_status, int64_t trip) {
      LoopFact l;
      l.key = key;
      l.pos = at(static_cast<uint32_t>(m.loops.size() + 1) * 100);
      l.kind = kind;
      l.trip_status = trip_status;
      l.trip = trip;
      return l;
    };
    ExprRef pred = p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_FIELD, "Gate.state"), p.constant(2));
    LoopFact walk = loop("walk", KZ_LOOP_KIND_TABLE_WALK, KZ_TRIP_STATUS_EXACT, 3);
    walk.table = "tab";
    LoopFact unresolved_walk = loop("walk2", KZ_LOOP_KIND_TABLE_WALK, KZ_TRIP_STATUS_UNKNOWN, 0);
    LoopFact counted = loop("cnt", KZ_LOOP_KIND_CONST_TRIP, KZ_TRIP_STATUS_EXACT, 4);
    LoopFact ok = loop("ok", KZ_LOOP_KIND_POLL, KZ_TRIP_STATUS_UNKNOWN, 0);  // predicate, lock, nothing written
    ok.pred = pred;
    ok.pred_form = KZ_PRED_FORM_DIRECT;
    ok.lock_kind = KZ_LOCK_KIND_ONE_REGION;
    ok.delay_direct = true;
    LoopFact nolock = ok;  // no lock region around the test
    nolock.key = "nolock";
    nolock.lock_kind = KZ_LOCK_KIND_NONE;
    LoopFact writes = ok;  // a callee writes the tested field
    writes.key = "writes";
    LoopFact unres = ok;  // an indirect call with an unknown target in the body
    unres.key = "unres";
    LoopFact untyped = ok;  // the tested field is not an extraction target: it has no A-TYPE listing
    untyped.key = "untyped";
    untyped.pred = p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_FIELD, "Other.f"), p.constant(2));
    LoopFact nopred = ok;
    nopred.key = "nopred";
    nopred.pred = nullptr;
    nopred.pred_form = KZ_PRED_FORM_NONE;
    nopred.pred_note = "the condition reads the local x";
    LoopFact other = loop("other", KZ_LOOP_KIND_OTHER, KZ_TRIP_STATUS_UNKNOWN, 0);
    m.loops = {walk, unresolved_walk, counted, ok, nolock, writes, unres, untyped, nopred, other};
    CallFact c_ok = call(301, "f", "delay", "ok");
    CallFact c_w = call(601, "f", "bump", "writes");
    CallFact c_u = call(701, "f", "", "unres");
    CallFact c_t = call(801, "f", "fmt", "untyped");
    m.calls = {c_ok, c_w, c_u, c_t};
    WriteEvent w1;
    w1.func_key = "bump";
    w1.kind = KZ_FWRITE_KIND_FIELD;
    w1.type = "Gate";
    w1.field = "state";
    w1.pos = at(5);
    WriteEvent w2;
    w2.func_key = "fmt";
    w2.kind = KZ_FWRITE_KIND_UNTYPED;
    w2.pos = at(6);
    m.write_events = {w1, w2};
    ProgramGraph g;
    for (const CallFact& c : m.calls) g.call_caller[key_of(c)] = c.caller_key;
    g.cg["f"] = {"delay", "bump", "fmt"};
    g.unresolved_calls.insert(key_of(c_u));
    decide_loop_rules(&m, spec, g);
    auto rule = [&](size_t i) { return m.loops[i].cert; };
    KZ_CHECK_EQ(rule(0), uint8_t(KZ_LOOP_CERT_UNROLL));
    KZ_CHECK_EQ(rule(1), uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK_EQ(rule(2), uint8_t(KZ_LOOP_CERT_UNROLL));
    KZ_CHECK_EQ(rule(3), uint8_t(KZ_LOOP_CERT_AWAIT));
    KZ_CHECK((m.loops[3].lflags & (KZ_LOOP_FLAG_L1_MOD | KZ_LOOP_FLAG_L1_READ)) == (KZ_LOOP_FLAG_L1_MOD | KZ_LOOP_FLAG_L1_READ));
    KZ_CHECK_EQ(rule(4), uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK(m.loops[4].cert_note.find("L1 (ii) fails") != std::string::npos);
    KZ_CHECK_EQ(rule(5), uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK(m.loops[5].cert_note.find("L1 (i) fails: the body writes Gate.state by bump") != std::string::npos);
    KZ_CHECK_EQ(rule(6), uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK((m.loops[6].lflags & KZ_LOOP_FLAG_MOD_INCOMPLETE) != 0);
    KZ_CHECK(m.loops[6].cert_note.find("unresolved target") != std::string::npos);
    KZ_CHECK_EQ(rule(7), uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK(m.loops[7].cert_note.find("L1 (i) undecided: Other.f is not an extraction target") != std::string::npos);
    KZ_CHECK((m.loops[7].lflags & KZ_LOOP_FLAG_MOD_INCOMPLETE) != 0);
    // a definite write takes precedence over what is open: "writes" fails, it is not undecided
    KZ_CHECK((m.loops[5].lflags & KZ_LOOP_FLAG_MOD_INCOMPLETE) == 0);
    KZ_CHECK_EQ(rule(8), uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK(m.loops[8].cert_note.find("no recognised predicate (the condition reads the local x)") != std::string::npos);
    KZ_CHECK_EQ(rule(9), uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK_EQ(m.resolved.loops_unroll, uint32_t(2));
    KZ_CHECK_EQ(m.resolved.loops_await, uint32_t(1));
    KZ_CHECK_EQ(m.resolved.loops_star, uint32_t(7));
    // an untyped write cannot reach a tested field that is an extraction target with a complete
    // writer set: the same loop over Rec.state is an await
    m.loops = {untyped};
    m.loops[0].pred = p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_FIELD, "Rec.state"), p.constant(2));
    decide_loop_rules(&m, spec, g);
    KZ_CHECK_EQ(m.loops[0].cert, uint8_t(KZ_LOOP_CERT_AWAIT));
    // ... unless its writer set is DISABLED
    m.locs[1].status = KZ_LOC_STATUS_DISABLED;
    m.loops = {untyped};
    m.loops[0].pred = p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_FIELD, "Rec.state"), p.constant(2));
    decide_loop_rules(&m, spec, g);
    KZ_CHECK_EQ(m.loops[0].cert, uint8_t(KZ_LOOP_CERT_STAR));
    KZ_CHECK(m.loops[0].cert_note.find("is DISABLED") != std::string::npos);
  }
  return kztest::finish("test_static_unroll");
}

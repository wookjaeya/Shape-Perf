// The profile-free mode's derivation (lib/llvm/wrappers.cpp, p0.cpp) on synthetic models: the P0
// table, F8 (delay and terminal operations through wrappers: must, noreturn, may, the guards' calls,
// the chains, recursion, indirect calls, a body without a control-flow graph, a P0 name the program
// defines), F7 (spawn operations: an entry argument, an entry called through a field, a fixed-entry
// spawner one level down, entries named by the startup script through the symbol lookup), the
// vocabulary handed to the second pass and the startup entries read from a basis.  The names are
// those of the synthetic programs; nothing here is a research condition.
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "p0.hpp"
#include "spec.hpp"
#include "support/kz_test.hpp"
#include "wrappers.hpp"

using namespace kzs;

namespace {

// A small program built by hand: functions, calls, control-flow graphs, function-pointer flow.
struct Prog {
  Model m;
  uint32_t line = 1;

  void fn(const std::string& key, uint32_t nparams = 0, bool body = true) {
    FuncFact f;
    f.key = key;
    f.name = key.substr(0, key.find('@'));
    f.nparams = nparams;
    f.flags = body ? KZ_FUNC_FLAG_DEFINED : 0;
    f.pos.file = "w.c";
    f.pos.line = line;
    m.funcs[key] = f;
    if (body) m.processed_functions.insert(key);
  }
  // A call; an empty callee is an indirect call through `node`.
  std::string call(const std::string& caller, const std::string& callee, const std::string& node = "") {
    CallFact c;
    c.pos.file = "w.c";
    c.pos.line = line++;
    c.pos.col = 1;
    c.caller_key = caller;
    c.caller_name = caller;
    c.callee_key = callee;
    c.callee_name = callee;
    c.kind = callee.empty() ? KZ_CALL_KIND_INDIRECT : KZ_CALL_KIND_DIRECT;
    m.calls.push_back(c);
    const std::string k = c.pos.str() + "#0";
    if (callee.empty()) m.indirect_calls.push_back(IndirectCall{k, node});
    return k;
  }
  void target(const std::string& call_key, const std::string& fn_key) {
    TargetFact t;
    t.call_key = call_key;
    t.fn_key = fn_key;
    t.kind = KZ_CALL_TARGET_KIND_INDIRECT;
    m.targets.push_back(t);
  }
  void fp_arg(const std::string& call_key, uint32_t index, const std::vector<std::string>& nodes, bool from_data = false) {
    FpArg a;
    a.index = index;
    a.nodes = nodes;
    a.from_data = from_data;
    m.call_fp_args[call_key].push_back(a);
  }
  // A control-flow graph: blocks[i] = (calls, successors, declared not to return); block 0 is the exit.
  struct B {
    std::vector<std::string> calls;
    std::vector<uint32_t> succ;
    bool noreturn = false;
  };
  void cfg(const std::string& f, uint32_t entry, const std::vector<B>& blocks) {
    FuncCfg g;
    g.entry = entry;
    g.exit = 0;
    for (const B& b : blocks) {
      CfgBlock x;
      x.calls = b.calls;
      x.succ = b.succ;
      x.noreturn = b.noreturn;
      g.blocks.push_back(x);
    }
    m.cfgs[f] = g;
  }
  // straight line: entry -> one block with the calls -> exit
  void line_cfg(const std::string& f, const std::vector<std::string>& calls, bool noreturn = false) { cfg(f, 2, {{}, {calls, {0}, noreturn}, {{}, {1}}}); }
};

const OpDerivFact* row(const Derivation& d, const std::string& fn, uint8_t kind, const std::string& next = "") {
  for (const OpDerivFact& r : d.ops)
    if (r.func_key == fn && r.kind == kind && (next.empty() || r.next_key == next)) return &r;
  return nullptr;
}
uint8_t mode(const Derivation& d, const std::string& fn, uint8_t kind) {
  const OpDerivFact* r = row(d, fn, kind);
  return r == nullptr ? static_cast<uint8_t>(KZ_OPDERIV_MODE_NONE) : r->mode;
}
std::vector<const SpawnWrapFact*> spawn_rows(const Derivation& d, const std::string& fn) {
  std::vector<const SpawnWrapFact*> out;
  for (const SpawnWrapFact& r : d.spawns)
    if (r.func_key == fn) out.push_back(&r);
  return out;
}
const P0Fact* p0_row(const Derivation& d, const std::string& name) {
  for (const P0Fact& r : d.p0)
    if (std::string(p0_table()[r.entry].name) == name) return &r;
  return nullptr;
}

void test_table() {
  std::set<std::string> names;
  size_t data = 0;
  for (const P0Entry& e : p0_table()) {
    KZ_CHECK(names.insert(e.name).second);  // each name once
    KZ_CHECK(std::string(e.version).size() > 0);
    KZ_CHECK(std::string(e.source).size() > 0);
    if (e.kind == KZ_P0_KIND_DATA_FN) ++data;
  }
  KZ_CHECK_EQ(data, size_t(12));
  for (const char* n : {"pthread_create", "pthread_mutex_lock", "pthread_mutex_trylock", "pthread_mutex_timedlock", "pthread_mutex_unlock", "sem_wait", "sem_trywait", "sem_timedwait",
                        "sem_post", "pthread_cond_wait", "pthread_cond_timedwait", "pthread_cond_signal", "pthread_cond_broadcast", "clock_nanosleep", "nanosleep", "pthread_exit",
                        "exit", "_exit", "abort", "pthread_self", "pthread_getspecific", "pthread_setspecific", "dlopen", "dlsym"})
    KZ_CHECK(names.count(n) == 1);
  const P0Entry& pc = p0_table()[0];
  KZ_CHECK_EQ(std::string(pc.name), std::string("pthread_create"));
  KZ_CHECK_EQ(pc.out, 0);
  KZ_CHECK_EQ(pc.entry, 2);
  KZ_CHECK_EQ(pc.arg, 3);
  KZ_CHECK(std::string(pc.source).find("nptl/pthread_create.c:624") != std::string::npos && std::string(pc.source).find(":755") != std::string::npos);
  // the data functions in their old order, with their old summaries
  ExtractSpec spec;
  spec_add_builtin_data_fns(&spec);
  KZ_CHECK_EQ(spec.declared.size(), size_t(12));
  KZ_CHECK_EQ(spec.declared[0].name, std::string("data_fn.memset"));
  KZ_CHECK_EQ(spec.declared[1].name, std::string("data_fn.bzero"));
  KZ_CHECK_EQ(spec.declared[11].name, std::string("data_fn.vsnprintf"));
  KZ_CHECK_EQ(spec.data_fns.at("memset").fill, 1);
  KZ_CHECK(spec.data_fns.at("bzero").zero_fill);
  KZ_CHECK_EQ(spec.data_fns.at("memcpy").src, 1);
  KZ_CHECK_EQ(spec.declared[0].origin, std::string("built-in table (hand-entered, tier researcher)"));
  ExtractSpec p0spec;
  spec_add_p0_entries(&p0spec);
  KZ_CHECK_EQ(p0spec.declared.size(), p0_table().size() - 12);
  // the built-in profile loads and has no vocabulary
  kz::phi::Profile prof;
  KZ_CHECK_OK(load_builtin_profile(&prof));
  KZ_CHECK_EQ(prof.name, std::string("kz-static-builtin-minimal"));
  KZ_CHECK(prof.ops.empty() && prof.locs.empty() && prof.rules.empty());
}

// F8 on a program of delay and terminal wrappers.
void test_f8() {
  Prog p;
  for (const char* n : {"nanosleep", "exit", "abort", "pthread_exit"}) p.fn(n, 1, false);
  // delay: sleep_impl -> nanosleep (every path); delay -> sleep_impl; backoff delays on one branch
  p.fn("sleep_impl", 1);
  const std::string c_ns = p.call("sleep_impl", "nanosleep");
  p.line_cfg("sleep_impl", {c_ns});
  p.fn("delay", 1);
  const std::string c_d = p.call("delay", "sleep_impl");
  p.line_cfg("delay", {c_d});
  p.fn("backoff", 1);
  const std::string c_b = p.call("backoff", "delay");
  p.cfg("backoff", 3, {{}, {{c_b}, {0}}, {{}, {1, 0}}, {{}, {2}}});
  // a function that never returns and never delays: not a delay operation
  p.fn("spin");
  p.cfg("spin", 2, {{}, {{}, {1}}, {{}, {1}}});
  // a body of the program with a P0 name: it is read as the program's own, not as the primitive
  p.fn("sleep", 1);
  p.line_cfg("sleep", {});
  p.fn("nap");
  const std::string c_nap = p.call("nap", "sleep");
  p.line_cfg("nap", {c_nap});
  // terminal: panic ends the process on both branches; task_exit ends the thread
  p.fn("panic", 1);
  const std::string c_ex = p.call("panic", "exit"), c_ab = p.call("panic", "abort");
  p.cfg("panic", 3, {{}, {{c_ab}, {0}, true}, {{c_ex}, {0}, true}, {{}, {2, 1}}});
  p.fn("task_exit");
  const std::string c_pe = p.call("task_exit", "pthread_exit");
  p.line_cfg("task_exit", {c_pe}, true);
  // exit_app: panics on one branch, waits for ever on the other (never returns)
  p.fn("exit_app", 1);
  const std::string c_ea1 = p.call("exit_app", "panic"), c_ea2 = p.call("exit_app", "delay");
  p.cfg("exit_app", 4, {{}, {}, {{c_ea2}, {2}}, {{c_ea1}, {0}}, {{}, {3, 2}}});
  // check: ends the thread when the check fails, returns otherwise
  p.fn("check", 1);
  const std::string c_ck = p.call("check", "task_exit");
  p.cfg("check", 3, {{}, {{}, {0}}, {{c_ck}, {1}}, {{}, {2, 1}}});
  // serve: an endless loop with a terminal call on one path is not `must`
  p.fn("serve");
  const std::string c_sv1 = p.call("serve", "try_once"), c_sv2 = p.call("serve", "panic");
  p.fn("try_once", 0, false);
  p.cfg("serve", 4, {{}, {{}, {3, 2}}, {{c_sv1}, {1}}, {{c_sv2}, {0}}, {{}, {2}}});
  // an indirect call whose resolved targets all end: covered; one with an unknown target: may at most
  p.fn("dispatch");
  const std::string c_ind = p.call("dispatch", "", "G:handler");
  p.target(c_ind, "panic");
  p.target(c_ind, "task_exit");
  p.line_cfg("dispatch", {c_ind});
  p.fn("dispatch2");
  const std::string c_ind2 = p.call("dispatch2", "", "G:handler2");
  p.target(c_ind2, "panic");
  p.target(c_ind2, "");
  p.line_cfg("dispatch2", {c_ind2});
  // a body without a control-flow graph: at most `may`
  p.fn("nocfg");
  p.call("nocfg", "panic");
  // recursion: odd and even are one component
  p.fn("odd", 1);
  p.fn("even", 1);
  const std::string c_o1 = p.call("odd", "panic"), c_o2 = p.call("odd", "even"), c_e1 = p.call("even", "odd");
  p.cfg("odd", 4, {{}, {{c_o2}, {0}}, {{c_o1}, {0}}, {}, {{}, {2, 1}}});
  p.cfg("even", 3, {{}, {{c_e1}, {0}}, {{}, {0}}, {{}, {2, 1}}});

  Derivation d;
  derive_wrappers(p.m, {}, &d);
  KZ_CHECK_EQ(d.functions_with_body, uint32_t(16));
  KZ_CHECK(d.sccs_recursive >= 1);
  // delay
  KZ_CHECK_EQ(mode(d, "sleep_impl", KZ_OPDERIV_KIND_DELAY), KZ_OPDERIV_MODE_MUST);
  KZ_CHECK_EQ(mode(d, "delay", KZ_OPDERIV_KIND_DELAY), KZ_OPDERIV_MODE_MUST);
  KZ_CHECK_EQ(mode(d, "backoff", KZ_OPDERIV_KIND_DELAY), KZ_OPDERIV_MODE_MAY);
  KZ_CHECK_EQ(mode(d, "spin", KZ_OPDERIV_KIND_DELAY), KZ_OPDERIV_MODE_NONE);
  KZ_CHECK_EQ(mode(d, "nap", KZ_OPDERIV_KIND_DELAY), KZ_OPDERIV_MODE_NONE);
  KZ_CHECK(d.delay_keys.count("nanosleep") == 1 && d.delay_keys.count("sleep_impl") == 1 && d.delay_keys.count("delay") == 1);
  KZ_CHECK(d.delay_keys.count("backoff") == 0 && d.delay_keys.count("sleep") == 0);
  const OpDerivFact* rd = row(d, "delay", KZ_OPDERIV_KIND_DELAY);
  KZ_CHECK(rd != nullptr);
  if (rd != nullptr) {
    KZ_CHECK_EQ(rd->chain, std::string("delay -> sleep_impl -> nanosleep"));
    KZ_CHECK_EQ(rd->depth, uint32_t(2));
    KZ_CHECK_EQ(rd->primitive, std::string("nanosleep"));
    KZ_CHECK_EQ(rd->call_key, c_d);
    KZ_CHECK((rd->flags & KZ_OPDERIV_FLAG_VOCABULARY) != 0);
  }
  const OpDerivFact* rb = row(d, "backoff", KZ_OPDERIV_KIND_DELAY);
  KZ_CHECK(rb != nullptr && (rb->flags & KZ_OPDERIV_FLAG_VOCABULARY) == 0);
  // terminal
  KZ_CHECK_EQ(mode(d, "panic", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MUST);
  KZ_CHECK_EQ(mode(d, "task_exit", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MUST);
  KZ_CHECK_EQ(mode(d, "exit_app", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_NORETURN);
  KZ_CHECK_EQ(mode(d, "check", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MAY);
  KZ_CHECK_EQ(mode(d, "serve", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_NORETURN);
  KZ_CHECK_EQ(mode(d, "dispatch", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MUST);
  KZ_CHECK_EQ(mode(d, "dispatch2", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MAY);
  KZ_CHECK_EQ(mode(d, "nocfg", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MAY);
  KZ_CHECK_EQ(mode(d, "odd", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MAY);
  KZ_CHECK_EQ(mode(d, "even", KZ_OPDERIV_KIND_TERMINAL), KZ_OPDERIV_MODE_MAY);
  KZ_CHECK_EQ(mode(d, "exit_app", KZ_OPDERIV_KIND_DELAY), KZ_OPDERIV_MODE_MAY);
  const OpDerivFact* rp = row(d, "panic", KZ_OPDERIV_KIND_TERMINAL, "exit");
  KZ_CHECK(rp != nullptr && (rp->flags & KZ_OPDERIV_FLAG_PROCESS_TERMINAL) != 0 && (rp->flags & KZ_OPDERIV_FLAG_THREAD_TERMINAL) == 0);
  const OpDerivFact* rt = row(d, "task_exit", KZ_OPDERIV_KIND_TERMINAL);
  KZ_CHECK(rt != nullptr && (rt->flags & KZ_OPDERIV_FLAG_THREAD_TERMINAL) != 0 && (rt->flags & KZ_OPDERIV_FLAG_PROCESS_TERMINAL) == 0);
  const OpDerivFact* ri = row(d, "dispatch", KZ_OPDERIV_KIND_TERMINAL);
  KZ_CHECK(ri != nullptr && (ri->flags & KZ_OPDERIV_FLAG_INDIRECT) != 0 && (ri->flags & KZ_OPDERIV_FLAG_THREAD_TERMINAL) != 0 &&
           (ri->flags & KZ_OPDERIV_FLAG_PROCESS_TERMINAL) != 0);
  const OpDerivFact* rn = row(d, "nocfg", KZ_OPDERIV_KIND_TERMINAL);
  KZ_CHECK(rn != nullptr && (rn->flags & KZ_OPDERIV_FLAG_NO_CFG) != 0);
  const OpDerivFact* rc = row(d, "check", KZ_OPDERIV_KIND_TERMINAL);
  KZ_CHECK(rc != nullptr && rc->call_key == c_ck && rc->chain == "check -> task_exit -> pthread_exit");
  // the vocabulary of the second pass
  for (const char* n : {"exit", "abort", "pthread_exit", "panic", "task_exit", "exit_app", "check", "serve", "dispatch", "dispatch2", "nocfg", "odd", "even"})
    KZ_CHECK(d.terminal_keys.count(n) == 1);
  KZ_CHECK(d.conditional_terminal_keys.count("exit_app") == 1 && d.conditional_terminal_keys.count("check") == 1);
  KZ_CHECK(d.conditional_terminal_keys.count("panic") == 0 && d.conditional_terminal_keys.count("dispatch") == 0);
  KZ_CHECK(d.process_terminal_keys.count("panic") == 1 && d.process_terminal_keys.count("exit") == 1);
  KZ_CHECK(d.process_terminal_keys.count("task_exit") == 0 && d.process_terminal_keys.count("dispatch") == 0);
  KZ_CHECK(d.lock_keys.count("pthread_mutex_lock") == 1 && d.lock_keys.count("pthread_mutex_trylock") == 0 && d.unlock_keys.count("pthread_mutex_unlock") == 1);
  // P0 rows
  const P0Fact* ps = p0_row(d, "sleep");
  KZ_CHECK(ps != nullptr && (ps->flags & KZ_P0_FLAG_SHADOWED) != 0 && (ps->flags & KZ_P0_FLAG_VOCABULARY) == 0);
  const P0Fact* pn = p0_row(d, "nanosleep");
  KZ_CHECK(pn != nullptr && pn->n_calls == 1 && pn->fn_key == "nanosleep" && (pn->flags & KZ_P0_FLAG_VOCABULARY) != 0);
  const P0Fact* pt = p0_row(d, "pthread_mutex_trylock");
  KZ_CHECK(pt != nullptr && (pt->flags & KZ_P0_FLAG_CONDITIONAL) != 0 && (pt->flags & KZ_P0_FLAG_VOCABULARY) == 0 && pt->fn_key.empty());

  ExtractSpec spec;
  apply_derivation(d, &spec);
  KZ_CHECK(spec.delay_keys.count("delay") == 1 && spec.terminal_keys.count("check") == 1 && spec.guard_calls.count(c_ck) == 1);
}

// F7 on a program of spawn wrappers.
void test_f7() {
  Prog p;
  p.fn("pthread_create", 4, false);
  p.fn("dlsym", 2, false);
  // thread_create(id, start, arg): its parameter 1 is the entry argument of pthread_create
  p.fn("thread_create", 3);
  const std::string c_pc = p.call("thread_create", "pthread_create");
  p.fp_arg(c_pc, 2, {"P:thread_create#1"});
  // task_create_impl(slot): a fixed entry, the trampoline
  p.fn("task_create_impl", 1);
  const std::string c_tci = p.call("task_create_impl", "thread_create");
  p.fp_arg(c_tci, 1, {"F:trampoline"});
  p.fn("trampoline", 1);
  const std::string c_ind = p.call("trampoline", "", "Task.entry");
  // task_create(slot, entry): stores the entry in a field the trampoline calls
  p.fn("task_create", 2);
  p.m.flow.push_back(FlowEdge{"P:task_create#1", "Task.entry"});
  const std::string c_tc = p.call("task_create", "task_create_impl");
  // app_create(slot, main): passes its parameter on
  p.fn("app_create", 2);
  const std::string c_ac = p.call("app_create", "task_create");
  p.fp_arg(c_ac, 1, {"P:app_create#1"});
  // start(): a fixed entry through a wrapper; start2() calls start(): not followed
  p.fn("worker");
  p.fn("start");
  const std::string c_st = p.call("start", "app_create");
  p.fp_arg(c_st, 1, {"F:worker"});
  p.fn("start2");
  p.call("start2", "start");
  // load_app(sym): the entry is an address looked up by name
  p.fn("lookup", 1);
  p.call("lookup", "dlsym");
  p.fn("load_app", 1);
  p.call("load_app", "lookup");
  const std::string c_la = p.call("load_app", "task_create");
  p.fp_arg(c_la, 1, {"L:load_app/addr"}, true);
  // load_cb(): reaches the lookup only through an indirect call: no script entry
  p.fn("load_cb");
  const std::string c_cb = p.call("load_cb", "", "G:loader");
  p.target(c_cb, "lookup");
  const std::string c_cb2 = p.call("load_cb", "task_create");
  p.fp_arg(c_cb2, 1, {"L:load_cb/addr"}, true);
  // the startup script: two apps, one of them not in the program, and a library
  p.fn("app_main");
  std::vector<StartupEntry> script;
  for (auto e : std::vector<std::vector<std::string>>{{"app", "app_main"}, {"app", "missing_main"}, {"library", "lib_init"}}) {
    StartupEntry s;
    s.index = static_cast<uint32_t>(script.size());
    s.kind = e[0];
    s.symbol = e[1];
    s.text = e[0] + " mod, " + e[1] + ", X, 0";
    script.push_back(s);
  }

  Derivation d;
  derive_wrappers(p.m, script, &d);
  for (const char* n : {"pthread_create", "thread_create", "task_create", "app_create"}) KZ_CHECK(d.spawn_keys.count(n) == 1);
  KZ_CHECK(d.spawn_keys.count("task_create_impl") == 0 && d.spawn_keys.count("start") == 0 && d.spawn_keys.count("start2") == 0);
  KZ_CHECK_EQ(d.spawn_entry_param.at("thread_create"), uint32_t(1));
  KZ_CHECK_EQ(d.spawn_entry_param.at("task_create"), uint32_t(1));
  KZ_CHECK_EQ(d.spawn_entry_param.at("app_create"), uint32_t(1));
  auto tc = spawn_rows(d, "thread_create");
  KZ_CHECK(tc.size() == 1 && tc[0]->kind == KZ_SPAWNW_KIND_ENTRY_ARG && tc[0]->call_key == c_pc && tc[0]->chain == "thread_create -> pthread_create");
  auto tk = spawn_rows(d, "task_create");
  KZ_CHECK(tk.size() == 1 && tk[0]->kind == KZ_SPAWNW_KIND_CALLED_BY_ENTRY && tk[0]->entry_key == "trampoline" && tk[0]->icall_key == c_ind && tk[0]->call_key == c_tc);
  if (!tk.empty()) {
    KZ_CHECK_EQ(tk[0]->chain, std::string("task_create -> task_create_impl -> thread_create -> pthread_create"));
    KZ_CHECK_EQ(tk[0]->depth, uint32_t(3));
    KZ_CHECK((tk[0]->flags & KZ_SPAWNW_FLAG_VOCABULARY) != 0);
  }
  auto ti = spawn_rows(d, "task_create_impl");
  KZ_CHECK(ti.size() == 1 && ti[0]->kind == KZ_SPAWNW_KIND_FIXED_ENTRY && ti[0]->entry_key == "trampoline" && (ti[0]->flags & KZ_SPAWNW_FLAG_VOCABULARY) == 0);
  auto ac = spawn_rows(d, "app_create");
  KZ_CHECK(ac.size() == 1 && ac[0]->kind == KZ_SPAWNW_KIND_ENTRY_ARG && ac[0]->call_key == c_ac);
  auto st = spawn_rows(d, "start");
  KZ_CHECK(st.size() == 1 && st[0]->kind == KZ_SPAWNW_KIND_FIXED_ENTRY && st[0]->entry_key == "worker");
  KZ_CHECK_EQ(d.fixed_callers_not_followed, uint32_t(1));
  // the startup script: the app entries, through the lookup reached by direct calls only
  auto la = spawn_rows(d, "load_app");
  KZ_CHECK_EQ(la.size(), size_t(2));
  if (la.size() == 2) {
    KZ_CHECK(la[0]->kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY && la[0]->entry_key == "app_main" && la[0]->call_key == c_la && la[0]->param == 1);
    KZ_CHECK(la[0]->chain == "load_app -> lookup -> dlsym");
    KZ_CHECK(la[1]->entry_key.empty() && (la[1]->flags & KZ_SPAWNW_FLAG_NOT_IN_CLOSURE) != 0);
  }
  KZ_CHECK(spawn_rows(d, "load_cb").empty());
  KZ_CHECK(d.spawn_script_entries.count(c_la) == 1 && d.spawn_script_entries.at(c_la) == std::set<std::string>{"app_main"});
  KZ_CHECK(d.spawn_script_entries.count(c_cb2) == 0);
  KZ_CHECK_EQ(d.script_calls, uint32_t(1));
  KZ_CHECK_EQ(d.script_in_closure, uint32_t(1));
  ExtractSpec spec;
  apply_derivation(d, &spec);
  KZ_CHECK(spec.spawn_keys.count("app_create") == 1 && spec.spawn_entry_param.at("app_create") == 1 && spec.spawn_script_entries.count(c_la) == 1);
  // the derivation is a function of the model
  Derivation d2;
  derive_wrappers(p.m, script, &d2);
  KZ_CHECK_EQ(d2.spawns.size(), d.spawns.size());
  for (size_t i = 0; i < d.spawns.size() && i < d2.spawns.size(); ++i) KZ_CHECK(d.spawns[i].chain == d2.spawns[i].chain && d.spawns[i].func_key == d2.spawns[i].func_key);
}

// The startup entries of a basis: the kind, the second field.
void test_startup_entries() {
  kz::ContainerWriter w(KZ_ARTIFACT_BASIS);
  std::vector<kz_basis_item> items;
  auto add = [&](const std::string& n, const std::string& v) {
    kz_basis_item b;
    std::memset(&b, 0, sizeof b);
    b.name = w.intern(n);
    b.value = w.intern(v);
    b.prov = KZ_NONE32;
    b.deviation = KZ_NONE32;
    items.push_back(b);
  };
  add("startup.entry[1]", "app /cf/b.so, b_main, B_APP, 50, 8192, 0x0, 0");
  add("startup.entry[0]", "library tinylib, NULL, T_LIB, 0, 0, 0x0, 0");
  add("startup.entries", "1 libraries, 1 applications");
  KZ_CHECK_OK(w.add_section(KZ_SEC_BASIS, items));
  kz::Bytes bytes;
  KZ_CHECK_OK(w.finish(&bytes));
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(bytes, &c));
  const std::vector<StartupEntry> e = startup_entries_from_basis(c);
  KZ_CHECK_EQ(e.size(), size_t(2));
  if (e.size() == 2) {
    KZ_CHECK(e[0].index == 0 && e[0].kind == "library" && e[0].symbol == "NULL");
    KZ_CHECK(e[1].index == 1 && e[1].kind == "app" && e[1].symbol == "b_main");
  }
}

}  // namespace

int main() {
  test_table();
  test_f8();
  test_f7();
  test_startup_entries();
  return kztest::finish("test_static_wrappers");
}

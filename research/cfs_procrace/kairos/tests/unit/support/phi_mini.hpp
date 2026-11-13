// phi_mini.hpp -- a small profile for the tests of the profile language, the evaluator and the
// replay.  It uses every construct of the language once and names nothing from a real framework:
// resources are "box", "slot", "gate", "stage" and so on.  All values are unit-test values.
//
// The text is split into three files on purpose (the profile hash does not depend on the split).
#pragma once

#include <string>
#include <vector>

#include "kz/core/phi.hpp"

namespace kzmini {

// The cited texts of the mini profile do not exist; the checker only needs well-formed records.
inline const char* const kHashA = "1111111111111111111111111111111111111111111111111111111111111111";
inline const char* const kHashB = "2222222222222222222222222222222222222222222222222222222222222222";

inline std::string file_head() {
  return R"KZ(# mini profile, file 1: header, sources, provenance, constants
phi 1;

profile "mini" revision "unit test";

source main root "" rev "abcdef0";
source libx root "libx" rev "1234567890abcdef";

prov doc_sup { tier contract-backed; kind api_doc; side supplier; file "inc/api.h"; lines 10 12;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
prov doc_call { tier contract-backed; kind api_doc; side caller; file "inc/api.h"; lines 20 22;
  sha256 "2222222222222222222222222222222222222222222222222222222222222222"; }
prov code_a { tier code-derived; kind code_pattern; side supplier; file "src/a.c"; lines 5;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
prov code_rule { tier code-derived; kind code_pattern; side rule; file "libx/b.c"; lines 7 8;
  sha256 "2222222222222222222222222222222222222222222222222222222222222222"; }
prov hand_a { tier researcher; kind researcher; authored "unit test, nobody"; note "made up for the tests"; }

const OK = 0 prov code_a;
const NOT_READY_A = 0xc0000001 prov doc_sup;
const NOT_READY_B = 0xc0000002 prov doc_sup;
const BAD_ARG = 0xc0000003 prov doc_sup;
const TIMED_OUT = 0xc0000004 prov doc_sup;
const UNDEF = 0 prov code_a;
const RESERVED = 0xffffffff prov code_a;
const BASE = 0x1000 prov code_a;
const SERIAL_MAX = 0xff prov code_a;
const TABLE = ref eval "TABLE_SIZE" in "src/a.c" prov code_a;
const LIMIT = 8 prov hand_a;
const NAME = "mini" prov hand_a;
const FLAG = true prov hand_a;
)KZ";
}

inline std::string file_kinds() {
  return R"KZ(# mini profile, file 2: kinds, locations, bindings, sites, INIT facts
phi 1;

context { owner_kind slot; prov code_a; }
scope { kind gate; request CLOSING; teardown CLOSED; prov code_a; }

kind slot(rec: Int) {
  states FREE < RESERVED < LIVE;
  initial FREE;
  scope process;
  table TABLE;
  ids { undefined UNDEF; reserved RESERVED; base BASE; serial_max SERIAL_MAX; }
  tag Type { UNSET, CORE, EXTERNAL };
  transition FREE -> RESERVED on reserve;
  transition RESERVED -> LIVE on publish;
  prov code_a;
}

kind stage(rec: Res<slot>) {
  states ZERO < ONE < TWO < THREE;
  initial ZERO;
  scope process;
  table TABLE;
  prov code_a;
}

kind self(a: Actor) {
  states ABSENT < PRESENT;
  initial ABSENT;
  scope process;
  derived PRESENT when state(owner(a)) = LIVE;
  prov hand_a;
}

kind reg(app: Id<slot>) {
  states UNREG < REG;
  initial UNREG;
  scope process;
  prov code_a;
}

kind gate() {
  states OPEN < CLOSING < CLOSED;
  initial OPEN;
  scope process;
  prov code_a;
}

kind cell(l: Loc) {
  states INVALID < VALID;
  initial INVALID;
  scope process;
  derived VALID when valid(value(l));
  prov hand_a;
}

loc Box.id { struct "Box_t"; field "id"; holds Id<slot>; prov code_a; }

bind field "Slot_t" "state" as state(slot(rec)) prov code_a;
bind field "Slot_t" "stage" as state(stage(rec)) prov code_a;
bind field "Box_t" "id" as value(Box.id) prov code_a;

init box_zero: loc Box.id = UNDEF prov code_a;

site phase_begin = fn("create_all").call("lock", arg[0] == LIMIT) nth 1 prov code_a;
site clear_site = fn("create_all").call("memset") macro no prov code_a;
site box_pub = fn("task_init").call("get_id", arg[0] is addr(Box_t.id)) prov code_a;
site some_store = fn("task_init").store(Slot_t.state) prov code_a;
)KZ";
}

inline std::string file_ops() {
  return R"KZ(# mini profile, file 3: operations, rules, readiness specification, trusted entries
phi 1;

op send(Info, stamp Who: Id<slot>, Spec) {
  bind symbol "send";
  conjunct who_valid: valid(arg.Who) lp capture;
  conjunct who_reg: state(reg(arg.Who)) = REG lp at fn("send").call("is_match");
  outcome OK: ready narrow { api "send"; source main rev "abcdef0"; branch "both checks passed"; context any; captured Who; holds_at return; prov code_a; };
  outcome NOT_READY_A: not_ready(who_valid) documented "retval NOT_READY_A of send" prov doc_sup
    narrow { api "send"; source main rev "abcdef0"; branch "the id check failed"; context any; captured Who; holds_at return; prov code_a; };
  outcome NOT_READY_B: not_ready(who_reg) documented "retval NOT_READY_B of send" prov doc_sup
    narrow { api "send"; source main rev "abcdef0"; branch "the registration check failed"; context any; captured Who; holds_at return; prov code_a; };
  outcome BAD_ARG: error;
  prov code_a, doc_sup;
}

op sign_in(Filters) {
  bind symbol "sign_in";
  effect clear_reg: lifecycle reg(id(owner(exec))) -> UNREG lp at fn("sign_in").memset(Reg_t) prov code_a;
  effect set_reg: lifecycle reg(id(owner(exec))) -> REG lp at fn("sign_in").call("set_used") prov code_a;
  outcome OK: ready;
  prov code_a;
}

op get_id(out IdPtr) {
  bind symbol "get_id";
  conjunct alive: state(self(exec)) = PRESENT lp at fn("get_id").call("lookup");
  effect store_ok: store *IdPtr := id(owner(exec)) when state(self(exec)) = PRESENT lp at fn("get_id").call("get_rec") prov code_a;
  effect store_bad: store *IdPtr := UNDEF filter state(self(exec)) != PRESENT lp at fn("get_id").call("unlock") prov code_a;
  outcome OK: ready;
  outcome NOT_READY_A: not_ready(alive) documented "retval NOT_READY_A of get_id" prov doc_sup;
  prov code_a;
}

op init_box() {
  bind symbol "init_box";
  effect clear_box: store Box.id := UNDEF lp at fn("init_box").memset(Box_t) prov code_a;
  prov code_a;
}

op reset_all() {
  bind symbol "reset_all";
  effect zero_slots: lifecycle all slot -> FREE lp at fn("reset_all").memset(All_t) prov code_a;
  effect zero_stages: lifecycle all stage -> ZERO lp at fn("reset_all").memset(All_t) prov code_a;
  prov code_a;
}

op create_all() {
  bind symbol "create_all";
  effect reserve: lifecycle slot(lp.n) -> RESERVED id RESERVED lp at fn("create_all").call("set_used", arg[1] == RESERVED) values n: Int prov code_a;
  effect typeit: settag Type of slot(lp.n) := CORE lp at fn("create_all").store(Slot_t.type) values n: Int prov code_a;
  effect typeext: settag Type of slot(lp.n) := EXTERNAL lp at fn("create_all").store(Slot_t.type_ext) values n: Int prov code_a;
  effect publish: lifecycle slot(lp.n) -> LIVE id lp.pid lp at fn("create_all").call("set_used", arg[1] != RESERVED) values n: Int, pid: Id<slot> prov code_a;
  effect clear: lifecycle slot(lp.n) -> FREE id UNDEF lp site clear_site values n: Int prov code_a;
  prov code_a;
}

op wait_poll(Level, Timeout) {
  bind symbol "wait_poll";
  outcome OK: ready;
  outcome TIMED_OUT: error;
  prov code_a;
}

op advance(Wanted) {
  bind symbol "advance";
  effect raise: lifecycle stage(owner(exec)) raise lp.level when state(self(exec)) = PRESENT lp at fn("advance").store(Slot_t.stage) values level: Int prov code_a;
  prov code_a;
}

op finish(Code) {
  bind symbol "finish";
  effect to_two: lifecycle stage(owner(exec)) -> TWO when state(self(exec)) = PRESENT && tag(owner(exec), Type) != CORE lp at fn("finish").store(Slot_t.stage) prov code_a;
  prov code_a;
}

op scan(Arg) {
  bind symbol "scan";
  effect wait_one: lifecycle stage(slot(lp.n)) -> THREE when state(stage(slot(lp.n))) = TWO && tag(slot(lp.n), Type) = EXTERNAL lp at fn("scan").store(Slot_t.stage) values n: Int prov code_a;
  prov code_a;
}

op free_up(Id) {
  bind symbol "free_up";
  effect free: lifecycle slot(lp.n) -> FREE id UNDEF when tag(slot(lp.n), Type) = EXTERNAL && state(stage(slot(lp.n))) > TWO lp at fn("free_up").call("set_free") values n: Int status declared_trusted refutable_by O3, O4 prov code_a, hand_a;
  prov code_a;
}

op mark_it() {
  bind symbol "mark_it";
  effect begin: mark handler_begin lp at fn("mark_it").call("begin_handler") prov code_a;
  effect use_it: use slot(lp.n) lp at fn("mark_it").call("touch") values n: Int prov code_a;
  effect sync_it: sync acq "lock_a" lp at fn("mark_it").call("lock_a") prov code_a;
  effect env_it: env "input_a" lp at fn("mark_it").call("read_input") prov code_a;
  prov code_a;
}

op spawn_it(out Handle, Name, Fn) {
  bind symbol "spawn_it";
  prov code_a;
}

op child_main(Handle) {
  bind symbol "child_main";
  prov code_a;
}

op orphan() {
  bind symbol "orphan";
  effect lost: lifecycle stage(owner(exec)) -> ONE lp at fn("orphan").call("touch") prov code_a;
  prov code_a;
}

op shutdown_req(Kind) {
  bind symbol "shutdown_req";
  effect request: lifecycle gate() -> CLOSING lp at fn("shutdown_req").store(Gate_t.state) prov code_a;
  terminal process_terminal after fn("shutdown_req").call("die");
  prov code_a;
}

op closer() {
  bind symbol "closer";
  effect close: lifecycle gate() -> CLOSED lp at fn("closer").call("delete_all") prov code_a;
  prov code_a;
}

op panic(Code) {
  bind symbol "panic";
  terminal process_terminal after fn("panic").call("die");
  prov code_a;
}

op die() {
  bind symbol "die";
  terminal process_terminal;
  prov hand_a;
}

hb H1 {
  title "program order";
  stratum S;
  robust unconditional;
  template program_order { looper yes; }
  prov hand_a;
}

hb H2 {
  title "task creation";
  stratum S;
  robust unconditional;
  template spawn { create spawn_it out Handle; entry child_main; }
  prov code_a;
}

hb H3 {
  title "phase";
  stratum S;
  robust unconditional;
  template phase { from phase_begin phase point; }
  prov hand_a;
}

hb H4 {
  title "poll";
  stratum S;
  robust obs_fatal;
  template observe {
    poll "wait_poll" called_at fn("create_all").call("wait_poll");
    exit_outcome OK;
    predicate from_code;
    else_terminal "panic";
    watch slot, stage;
  }
  guard main >= "2.0.1";
  prov code_a, hand_a;
}

rspec K-SEND {
  use op send;
  key reg(arg.Who);
  requires valid(arg.Who) && state(reg(arg.Who)) = REG;
  oracle fns "is_valid", "is_match";
  appreq doc_call;
  allowed none;
  harm event_request_rejected;
  authored "unit test";
  prov doc_call, doc_sup, code_a;
}

trusted T-FRESH {
  class declared_trusted;
  statement "the made-up assumption of the tests";
  refutable_by O9;
  prov hand_a;
}
)KZ";
}

inline std::vector<kz::phi::SourceText> files() {
  return {kz::phi::SourceText{"mini_1.kzp", file_head()}, kz::phi::SourceText{"mini_2.kzp", file_kinds()}, kz::phi::SourceText{"mini_3.kzp", file_ops()}};
}

}  // namespace kzmini

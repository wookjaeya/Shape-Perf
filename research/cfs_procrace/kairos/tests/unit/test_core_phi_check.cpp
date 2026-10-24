// The checker of the profile language: every rule of docs/phi_reference.md section 8 has a
// negative case, made by one edit of the mini profile (tests/unit/support/phi_mini.hpp), and the
// diagnostic must say what is wrong.  The unedited profile loads.
//
// Each case: the file (0 head, 1 kinds, 2 operations), the text to replace (it must occur exactly
// once, otherwise the case itself is wrong), its replacement, and a fragment of the diagnostic.

#include <string>
#include <vector>

#include "kz/core/phi.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::phi::Diag;
using kz::phi::Profile;
using kz::phi::SourceText;

struct Neg {
  int file;
  const char* from;
  const char* to;
  const char* expect;
  const char* from2 = nullptr;  // an optional second edit, in the same file
  const char* to2 = nullptr;
};

size_t count_of(const std::string& text, const std::string& what) {
  size_t n = 0;
  for (size_t at = text.find(what); at != std::string::npos; at = text.find(what, at + what.size())) ++n;
  return n;
}

// Applies the edit and loads.  Returns the diagnostics as text, or "NO ERROR".
std::string run(const Neg& n, bool* bad_case) {
  std::vector<SourceText> f = kzmini::files();
  std::string& text = f[static_cast<size_t>(n.file)].text;
  if (count_of(text, n.from) != 1) {
    std::fprintf(stderr, "bad case: \"%s\" occurs %zu times in file %d\n", n.from, count_of(text, n.from), n.file);
    *bad_case = true;
    return "";
  }
  text.replace(text.find(n.from), std::string(n.from).size(), n.to);
  if (n.from2 != nullptr) {
    if (count_of(text, n.from2) != 1) {
      std::fprintf(stderr, "bad case: \"%s\" occurs %zu times in file %d\n", n.from2, count_of(text, n.from2), n.file);
      *bad_case = true;
      return "";
    }
    text.replace(text.find(n.from2), std::string(n.from2).size(), n.to2);
  }
  Profile p;
  std::vector<Diag> diags;
  const kz::Status s = kz::phi::load_profile(f, &p, &diags);
  std::string out;
  for (const Diag& d : diags) out += kz::phi::format_diag(p.files, d) + "\n";
  if (s.ok() && !kz::phi::has_errors(diags)) return "NO ERROR\n" + out;
  return out;
}

std::string many_states(const char* prefix, int n, const char* sep = " < ") {
  std::string s;
  for (int i = 0; i < n; ++i) s += std::string(i == 0 ? "" : sep) + prefix + std::to_string(i);
  return s;
}

}  // namespace

int main() {
  {  // the unedited profile loads, with warnings and notes only
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));
    KZ_CHECK(!kz::phi::has_errors(diags));
  }

  const std::string big_states = "states " + many_states("S", 65) + ";";
  const std::string big_tag = "tag Type { " + many_states("V", 65, ", ") + " };";

  const Neg cases[] = {
      // ---- provenance and sources -----------------------------------------------------------
      {0, "prov doc_sup { tier contract-backed;", "prov doc_sup { tier gold-plated;", "unknown tier 'gold-plated'"},
      {0, "prov code_a { tier code-derived; kind code_pattern;", "prov code_a { tier code-derived; kind api_doc;", "tier 'code-derived' does not go with kind 'api_doc'"},
      {0, "kind api_doc; side supplier; file \"inc/api.h\"", "kind poem; side supplier; file \"inc/api.h\"", "unknown kind 'poem'"},
      {0, "source main root \"\" rev \"abcdef0\";", "", "belongs to no declared source"},
      {0, "lines 5;\n  sha256 \"1111111111111111111111111111111111111111111111111111111111111111\"; }\nprov code_rule",
       "lines 5;\n  sha256 \"abc\"; }\nprov code_rule", "sha256 must be 64 lower-case hexadecimal digits"},
      {0, "lines 5;\n  sha256", "sha256", "of kind code_pattern names no lines"},
      {0, "kind researcher; authored \"unit test, nobody\"; ", "kind researcher; ", "of kind researcher has no 'authored' record"},
      {0, "side supplier; file \"inc/api.h\"; lines 10 12;", "side nobody; file \"inc/api.h\"; lines 10 12;", "unknown side 'nobody'"},
      {0, "rev \"abcdef0\"", "rev \"xyz\"", "the revision must be 7 to 40 lower-case hexadecimal digits"},
      {0, "source libx root \"libx\"", "source libx root \"\"", "another source has the same root"},
      {0, "source libx root \"libx\"", "source libx root \"/libx\"", "no leading or trailing '/'"},
      {0, "const OK = 0 prov code_a;", "const OK = 0 prov nope;", "unknown provenance record 'nope'"},
      {2, "conjunct who_valid: valid(arg.Who) lp capture;", "conjunct who_valid: valid(arg.Who) lp capture prov nope;", "unknown provenance record 'nope' in operation 'send', conjunct 'who_valid'"},
      {0, "const FLAG = true prov hand_a;", "const FLAG = true;", "constant 'FLAG' cites no provenance record"},
      {0, "const TABLE = ref eval \"TABLE_SIZE\" in \"src/a.c\"", "const TABLE = ref eval \"TABLE_SIZE\" in \"nowhere/a.c\"", "constant 'TABLE': the translation unit 'nowhere/a.c' belongs to no declared source",
       "source main root \"\" rev", "source main root \"src\" rev"},
      {0, "const BASE = 0x1000 prov code_a;", "const BASE = 0x100000000 prov code_a;", "base and base + serial_max must lie in 0 .. 0xFFFFFFFF"},
      // the sum of base and serial_max would overflow an int64: the check does not form it
      {0, "const BASE = 0x1000 prov code_a;", "const BASE = 0x7fffffffffffffff prov code_a;", "base and base + serial_max must lie in 0 .. 0xFFFFFFFF"},
      {0, "const SERIAL_MAX = 0xff prov code_a;", "const SERIAL_MAX = 0x7fffffffffffffff prov code_a;", "base and base + serial_max must lie in 0 .. 0xFFFFFFFF"},
      {0, "const BASE = 0x1000 prov code_a;", "const BASE = 0xffffff01 prov code_a;", "base and base + serial_max must lie in 0 .. 0xFFFFFFFF"},   // 0xffffff01 + 0xff is 2^32
      // a requirement record is not contract-backed: its tier does not exist in the vocabulary yet
      {0, "prov doc_sup { tier contract-backed; kind api_doc;", "prov doc_sup { tier contract-backed; kind requirement;", "tier 'contract-backed' does not go with kind 'requirement'"},
      {0, "prov doc_sup { tier contract-backed; kind api_doc;", "prov doc_sup { tier requirement-derived; kind requirement;", "unknown tier 'requirement-derived'"},
      {2, "guard main >= \"2.0.1\";", "guard other >= \"2.0.1\";", "'other', which is not a declared source"},
      {2, "guard main >= \"2.0.1\";", "guard main >= \"2.x.1\";", "malformed version \"2.x.1\""},

      // ---- kinds ------------------------------------------------------------------------------
      {1, "states FREE < RESERVED < LIVE;", "states FREE < FREE < LIVE;", "state 'FREE' is declared twice"},
      {1, "states FREE < RESERVED < LIVE;", big_states.c_str(), "more than 64 states"},
      {1, "states ZERO < ONE < TWO < THREE;\n  initial ZERO;", "initial ZERO;", "declares no states"},
      {1, "initial FREE;", "initial NOWHERE;", "the initial state 'NOWHERE' is not one of its states"},
      {1, "initial FREE;\n  scope process;\n  table TABLE;\n  ids", "scope process;\n  table TABLE;\n  ids", "has no 'initial' state"},
      {1, "scope process;\n  table TABLE;\n  ids", "scope forever;\n  table TABLE;\n  ids", "scope must be process or persistent"},
      {1, "table TABLE;\n  ids", "table NAME;\n  ids", "the table size 'NAME' must be an integer constant"},
      {1, "table TABLE;\n  ids", "table NOPE;\n  ids", "the table size 'NOPE' is not a declared constant"},
      {1, " serial_max SERIAL_MAX;", "", "ids lacks 'serial_max'"},
      {1, "reserved RESERVED;", "reserved UNDEF;", "the undefined and the reserved marker are equal"},
      {1, "base BASE;", "base NAME;", "ids base 'NAME' is not a declared integer constant"},
      {1, "tag Type { UNSET, CORE, EXTERNAL };", "tag Type { UNSET, CORE, CORE };", "value 'CORE' is declared twice"},
      {1, "tag Type { UNSET, CORE, EXTERNAL };", big_tag.c_str(), "more than 64 values"},
      {1, "transition FREE -> RESERVED on reserve;", "transition FREE -> NOWHERE on reserve;", "transition to unknown state 'NOWHERE'"},
      {1, "transition FREE -> RESERVED on reserve;", "transition NOWHERE -> RESERVED on reserve;", "transition from unknown state 'NOWHERE'"},
      {1, "derived PRESENT when", "derived NOWHERE when", "derived rule for unknown state 'NOWHERE'"},
      {1, "kind reg(app: Id<slot>)", "kind reg(app: Id<stage>)", "kind 'stage' has no 'ids' clause, so Id<stage> is not a sort"},
      {1, "kind reg(app: Id<slot>)", "kind reg(app: Id<nothing>)", "unknown kind 'nothing' in Id<...>"},
      {1, "kind slot(rec: Int)", "kind slot(rec: Res<stage>)", "form a cycle"},
      {1, "owner_kind slot;", "owner_kind nothing;", "owner_kind 'nothing' is not a declared kind"},
      {1, "kind gate() {\n  states OPEN < CLOSING < CLOSED;", "kind gate() {\n  states CLOSED < CLOSING < OPEN;", "the request state must come before the teardown state"},
      {1, "request CLOSING;", "request NOWHERE;", "'NOWHERE' is not a state of kind 'gate'"},
      {1, "scope { kind gate;", "scope { kind nothing;", "scope: kind 'nothing' is not a declared kind"},
      {1, "struct \"Box_t\"; field \"id\"; ", "struct \"Box_t\"; ", "needs 'struct' and 'field'"},
      {1, "holds Id<slot>;", "holds Id<nothing>;", "unknown kind 'nothing'"},
      {1, "as state(slot(rec)) prov code_a;", "as 5 prov code_a;", "a binding target is state(K(v)), tag(K(v), T), id(K(v)) or value(L)"},
      {1, "bind field \"Box_t\" \"id\" as value(Box.id)", "bind field \"Other_t\" \"id\" as value(Box.id)", "names location 'Box.id', which is declared as Box_t.id"},
      {1, "init box_zero: loc Box.id = UNDEF", "init box_zero: loc Box.nope = UNDEF", "unknown location 'Box.nope'"},
      {1, "init box_zero: loc Box.id = UNDEF", "init box_zero: loc Box.id = \"text\"", "the value is Str, the location holds"},
      {1, "arg[0] == LIMIT", "arg[0] == NOPE", "compares an argument with 'NOPE', which is not a declared constant"},

      // ---- expressions -------------------------------------------------------------------------
      {2, "conjunct alive: state(self(exec)) = PRESENT", "conjunct alive: state(self(exec)) = PRESENTT", "unknown name 'PRESENTT'"},
      {2, "conjunct alive: state(self(exec)) = PRESENT", "conjunct alive: state(self(exec)) = PRESENT && result = 0", "'result'"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = 5 lp", "cannot compare"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) lp", "expected a Bool expression"},
      {2, "conjunct who_valid: valid(arg.Who) lp capture;", "conjunct who_valid: valid(arg.Who, 1) lp capture;", "'valid' takes 1 argument"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who, 1)) = REG lp", "kind 'reg' takes 1 key argument(s), found 2"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && tag(reg(arg.Who), Type) = CORE lp", "kind 'reg' has no tag 'Type'"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && state(arg.Who) = REG lp", "'state' needs a resource"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && valid(5) lp", "valid(...) needs an Id"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && frob(1) lp", "unknown function or kind 'frob'"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && arg.Nope = 1 lp", "unknown parameter 'arg.Nope'"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && forall x: reg { true } lp", "kind 'reg' has no 'table' clause"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && state(slot(1)) < LIVE && state(slot(1)) in {FREE, 5} lp", "in a set"},
      {2, "conjunct who_reg: state(reg(arg.Who)) = REG lp", "conjunct who_reg: state(reg(arg.Who)) = REG && true < false lp", "needs Int, Id or State operands"},

      // ---- operations --------------------------------------------------------------------------
      {2, "op send(Info, stamp Who: Id<slot>, Spec)", "op send(Info, stamp Who: Id<slot>, Info)", "parameter 'Info' is declared twice"},
      {2, "op send(Info, stamp Who: Id<slot>, Spec)", "op send(Info, Who: Id<slot>, Spec)", "a conjunct at 'lp capture' is about a value the caller passes"},
      {2, "conjunct who_valid: valid(arg.Who) lp capture;", "conjunct who_valid: state(reg(arg.Who)) = REG lp capture;", "the function 'state' reads resource state"},
      {2, "effect clear_box: store Box.id := UNDEF lp at fn(\"init_box\").memset(Box_t)", "effect clear_box: store Box.id := UNDEF lp capture", "'lp capture' is for conjuncts"},
      {2, "status declared_trusted refutable_by O3, O4", "status declared_trusted", "names the obligations that can refute it"},
      {2, "refutable_by O3, O4", "refutable_by O3, O99", "'O99' is not an obligation"},
      {2, "status declared_trusted refutable_by", "status trusted_ish refutable_by", "unknown status 'trusted_ish'"},
      {2, "-> TWO when", "-> TWO id UNDEF when", "kind 'stage' has no ids, so the effect cannot carry an id"},
      {2, "lifecycle all slot -> FREE lp", "lifecycle all slot -> FREE id UNDEF lp", "an effect on every instance cannot carry an id"},
      {2, "lifecycle all slot -> FREE lp", "lifecycle all reg -> UNREG lp", "needs a kind with a finite set of instances"},
      {2, "lifecycle all slot -> FREE lp", "lifecycle all nothing -> FREE lp", "unknown kind 'nothing'"},
      {2, "-> RESERVED id RESERVED lp", "-> NOWHERE id RESERVED lp", "'NOWHERE' is not a state of kind 'slot'"},
      {2, "-> RESERVED id RESERVED lp", "-> RESERVED id \"text\" lp", "the id is Str, the kind wants Id<slot>"},
      {2, "raise lp.level when", "raise \"text\" when", "the level to raise to is Str"},
      {2, "store *IdPtr := id(owner(exec)) when", "store *Nope := id(owner(exec)) when", "'Nope' is not an out parameter of the operation"},
      {2, "effect clear_box: store Box.id := UNDEF", "effect clear_box: store Box.nope := UNDEF", "unknown location 'Box.nope'"},
      {2, "effect clear_box: store Box.id := UNDEF", "effect clear_box: store Box.id := \"x\"", "the value is Str, the location holds Id<slot>"},
      {2, "settag Type of slot(lp.n) := CORE", "settag Nope of slot(lp.n) := CORE", "kind 'slot' has no tag 'Nope'"},
      {2, "settag Type of slot(lp.n) := CORE", "settag Type of slot(lp.n) := NOPE", "'NOPE' is not a value of tag 'Type'"},
      {2, "effect begin: mark handler_begin", "effect begin: mark handler_middle", "a mark is handler_begin or handler_end"},
      {2, "sync acq \"lock_a\"", "sync grab \"lock_a\"", "unknown synchronization role 'grab'"},
      {2, "effect free: lifecycle slot(lp.n) -> FREE id UNDEF when", "effect free: lifecycle 5 -> FREE id UNDEF when", "expected a resource, found Int"},
      {2, "outcome OK: ready;\n  outcome NOT_READY_A: not_ready(who_valid)", "outcome NOPE: ready;\n  outcome NOT_READY_A: not_ready(who_valid)", "the result code is not a declared constant"},
      {2, "outcome OK: ready;\n  outcome NOT_READY_A: not_ready(who_valid)", "outcome NAME: ready;\n  outcome NOT_READY_A: not_ready(who_valid)", "the result code must be an integer constant"},
      {2, "not_ready(who_valid)", "not_ready(nope)", "not_ready(nope) names no conjunct of the operation"},
      {2, "documented \"retval NOT_READY_A of send\" prov doc_sup;", "documented \"retval NOT_READY_A of send\";", "a documented result cites the text that documents it"},
      {2, "documented \"retval NOT_READY_A of send\" prov doc_sup;", "documented \"retval NOT_READY_A of send\" prov doc_call;", "the documentation of a result is a supplier-side record"},
      {2, "op die() {\n  bind symbol \"die\";\n", "op die() {\n", "a terminal operation needs 'bind symbol'"},
      {2, "lp site clear_site", "lp site nope_site", "unknown site 'nope_site'"},
      {2, "effect to_two: lifecycle stage(owner(exec))", "effect to_two: lifecycle stage(frob(exec))", "unknown function or kind 'frob'"},
      {2, "outcome BAD_ARG: error;", "outcome BAD_ARG: error;\n  outcome BAD_ARG: ready;", "duplicate outcome 'BAD_ARG'"},
      {2, "effect clear_box:", "effect clear_box:", "NO ERROR"},  // the control: an edit that changes nothing loads

      // ---- roles of call contexts (SPEC 1.6 "Call contexts") -----------------------------------------
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site box_pub; purpose existence_probe; prov code_a; }", "NO ERROR"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site box_pub; purpose dependent_use; prov hand_a; }", "NO ERROR"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op nope; site box_pub; purpose existence_probe; prov code_a; }", "unknown operation 'nope'"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site nope_site; purpose existence_probe; prov code_a; }", "unknown site 'nope_site'"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site box_pub; purpose banana; prov code_a; }", "purpose is dependent_use or existence_probe"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site some_store; purpose existence_probe; prov code_a; }", "is not a call pattern"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site phase_begin; purpose existence_probe; prov code_a; }", "calls 'lock', the operation binds 'get_id'"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site box_pub; purpose existence_probe; prov hand_a; }", "hand-entered records alone do not justify it"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site box_pub; purpose existence_probe; }", "cites no provenance record"},
      {1, "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;", "site box_pub = fn(\"task_init\").call(\"get_id\", arg[0] is addr(Box_t.id)) prov code_a;\nrole r1 { op get_id; site box_pub; purpose existence_probe; prov code_a; }\nrole r2 { op get_id; site box_pub; purpose dependent_use; prov code_a; }", "already has the role 'r1'"},

      // ---- happens-before rules ------------------------------------------------------------------
      {2, "stratum S;\n  robust unconditional;\n  template program_order", "stratum S;\n  robust obs_soft;\n  template program_order", "a structural (S) rule is unconditional or obs_fatal, not obs_soft"},
      {2, "stratum S;\n  robust unconditional;\n  template program_order", "stratum O;\n  robust unconditional;\n  template program_order", "an observed (O) rule is obs_soft, schedule_chosen or assumed"},
      {2, "stratum S;\n  robust unconditional;\n  template program_order", "stratum C;\n  robust unconditional;\n  template program_order", "a schedule constraint (C) is tagged schedule_chosen"},
      {2, "create spawn_it out Handle;", "create nope_op out Handle;", "the creating operation 'nope_op' is not declared"},
      {2, "create spawn_it out Handle;", "create spawn_it out Name;", "'Name' is not an out parameter of 'spawn_it'"},
      {2, "entry child_main;", "entry nope_op;", "the entry 'nope_op' is neither a declared operation nor a site"},
      {2, "from phase_begin phase point;", "from nope phase point;", "'nope' is neither a declared operation nor a site"},
      {2, "from phase_begin phase point;", "from phase_begin phase later;", "the phase is call, return or point"},
      {2, "exit_outcome OK;", "exit_outcome NOPE;", "exit_outcome 'NOPE' is not a declared integer constant"},
      {2, "else_terminal \"panic\";", "else_terminal \"send\";", "else_terminal 'send' is not a declared terminal operation"},
      {2, "watch slot, stage;", "watch slot, nope;", "'watch' names an undeclared kind 'nope'"},
      {2, "    else_terminal \"panic\";\n", "", "an obs_fatal rule names the terminal escape of the other branch"},
      {2, "called_at fn(\"create_all\").call(\"wait_poll\");", "called_at fn(\"\").call(\"wait_poll\");", "the pattern names no function"},

      // ---- readiness specifications -------------------------------------------------------------
      {2, "use op send;", "use op nope;", "the operation 'nope' is not declared"},
      {2, "requires valid(arg.Who) && state(reg(arg.Who)) = REG;", "requires valid(arg.Who) && state(reg(arg.Who)) = UNREG;", "has no linearization point"},
      {2, "requires valid(arg.Who) && state(reg(arg.Who)) = REG;", "requires valid(arg.Who) && valid(arg.Who);", "is listed twice"},
      {2, "key reg(arg.Who);", "key arg.Who;", "the selector must be a resource"},
      {2, "appreq doc_call;", "appreq doc_sup;", "appreq must cite a caller-side record"},
      {2, "appreq doc_call;", "appreq nope;", "appreq names an unknown provenance record 'nope'"},
      {2, "harm event_request_rejected;", "harm sad;", "unknown harm 'sad'"},
      {2, "class declared_trusted;", "class believed;", "class is declared_trusted or assumption"},
      {2, "refutable_by O9;", "", "a trusted entry is refutable, not proved"},
      {2, "refutable_by O9;", "refutable_by O11;", "'O11' is not an obligation"},
      {2, "hb H4 {", "hb H1 {", "duplicate hb rule 'H1'"},

      // ---- caller-side records (QT-4, QT-9) and effect guards -------------------------------------
      {0, "prov hand_a { tier researcher; kind researcher; authored", "prov hand_a { tier researcher; kind researcher; side caller; authored", "cannot have side caller"},
      {0, "side rule; file \"libx/b.c\"", "side caller; file \"libx/b.c\"", "a record of tier 'code-derived' cannot have side caller"},
      {2, "effect free: lifecycle slot(lp.n) -> FREE id UNDEF when tag", "effect free: lifecycle slot(lp.n) -> FREE id UNDEF filter tag", "the guard of a declared_trusted effect is asserted"},
  };

  for (const Neg& n : cases) {
    bool bad = false;
    const std::string got = run(n, &bad);
    ++kztest::state().checks;
    if (bad) {
      ++kztest::state().failures;
      continue;
    }
    if (got.find(n.expect) == std::string::npos) {
      std::fprintf(stderr, "case \"%s\" -> \"%s\"\n  wanted a diagnostic containing: %s\n  got:\n%s\n", n.from, n.to, n.expect, got.c_str());
      ++kztest::state().failures;
    }
  }

  {  // an appreq that cites a hand-entered record with side caller: the record and the appreq are errors
    std::vector<SourceText> f = kzmini::files();
    const std::string from = "prov hand_a { tier researcher; kind researcher; authored";
    f[0].text.replace(f[0].text.find(from), from.size(), "prov hand_a { tier researcher; kind researcher; side caller; authored");
    f[2].text.replace(f[2].text.find("appreq doc_call;"), 16, "appreq hand_a;");
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(!kz::phi::load_profile(f, &p, &diags).ok());
    bool record = false, appreq = false;
    for (const Diag& d : diags) {
      if (d.message.find("'hand_a': a record of tier 'researcher' cannot have side caller") != std::string::npos) record = true;
      if (d.message.find("no researcher-authored appreq") != std::string::npos) appreq = true;
    }
    KZ_CHECK(record);
    KZ_CHECK(appreq);
  }
  {  // an admitted or issue-derived record may be caller-side (a cited text addressed to the caller)
    std::vector<SourceText> f = kzmini::files();
    f[0].text += "prov adm_a { tier admitted; kind code_admission; side caller; file \"src/a.c\"; lines 9 9;\n  sha256 \"1111111111111111111111111111111111111111111111111111111111111111\"; }\n"
                 "prov iss_a { tier issue-derived; kind issue; side caller; cite \"issue 1\"; }\n";
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::load_profile(f, &p, &diags));
    KZ_CHECK(!kz::phi::has_errors(diags));
  }
  {  // an Id compared with an integer outside 0 .. 0xFFFFFFFF: a warning (the comparison is decided by the sorts)
    std::vector<SourceText> f = kzmini::files();
    const std::string from = "when state(self(exec)) = PRESENT && tag(owner(exec), Type) != CORE";
    f[2].text.replace(f[2].text.find(from), from.size(), from + " && id(owner(exec)) != -1 && !(id(owner(exec)) in {0x100000000, BASE}) && id(owner(exec)) != RESERVED");
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::load_profile(f, &p, &diags));
    int warned = 0;
    for (const Diag& d : diags)
      if (d.severity == kz::phi::Severity::Warning && d.message.find("is outside 0 .. 0xFFFFFFFF and is compared with the Id") != std::string::npos) ++warned;
    KZ_CHECK_EQ(warned, 2);  // -1 and 0x100000000; BASE and RESERVED (0xffffffff) are inside
  }

  // Positive cases: edits that must still load.
  {
    std::vector<SourceText> f = kzmini::files();
    // Allowed none without a specified appreq: a note, not an error (the caller-level Allowed is unspecified).
    const size_t at = f[2].text.find("appreq doc_call;");
    f[2].text.replace(at, 16, "appreq unspecified_by_reference;");
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::load_profile(f, &p, &diags));
    bool noted = false;
    for (const Diag& d : diags)
      if (d.severity == kz::phi::Severity::Note && d.message.find("without a specified appreq") != std::string::npos) noted = true;
    KZ_CHECK(noted);
  }
  {  // a profile without the header is an error unless the option says otherwise
    std::vector<SourceText> f = kzmini::files();
    const size_t at = f[0].text.find("profile \"mini\" revision \"unit test\";");
    f[0].text.erase(at, std::string("profile \"mini\" revision \"unit test\";").size());
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK(!kz::phi::load_profile(f, &p, &diags).ok());
    Profile q;
    std::vector<Diag> diags2;
    kz::phi::CheckOptions options;
    options.require_header = false;
    KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &q, &diags2, options));
    Profile r;
    std::vector<Diag> diags3;
    KZ_CHECK_OK(kz::phi::load_profile(f, &r, &diags3, options));
  }
  {  // the options relax provenance and sources
    std::vector<SourceText> f = kzmini::files();
    const size_t at = f[0].text.find("const FLAG = true prov hand_a;");
    f[0].text.replace(at, 30, "const FLAG = true;");
    kz::phi::CheckOptions options;
    options.require_provenance = false;
    Profile p;
    std::vector<Diag> diags;
    KZ_CHECK_OK(kz::phi::load_profile(f, &p, &diags, options));
  }

  return kztest::finish("test_core_phi_check");
}

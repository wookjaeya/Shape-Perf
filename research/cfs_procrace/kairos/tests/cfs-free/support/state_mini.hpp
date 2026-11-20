// state_mini.hpp -- a small profile for the tests of the per-occurrence basis and of the merged order (a lookup whose
// provider is another task: kinds, a lifecycle publication, spawn and reads_from rules).  The same profile text as the
// one of tests/unit/test_core_state_order.cpp; every value is a unit-test value, not a research condition.
#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "kz/core/phi.hpp"
#include "support/kz_test.hpp"

namespace kzstate {

inline const char* const kProfile = R"KZ(phi 1;
profile "state-order-mini" revision "unit test";
source main root "" rev "abcdef0";
prov doc_sup { tier contract-backed; kind api_doc; side supplier; file "inc/sem.h"; lines 1 2;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
prov code_a { tier code-derived; kind code_pattern; side supplier; file "src/sem.c"; lines 5;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
const OK = 0 prov code_a;
const NOT_FOUND = -17 prov doc_sup;

kind sem(name: Str) { states ABSENT < LIVE; initial ABSENT; scope process; prov code_a; }
op lookup(out id, key name: Str) {
  bind symbol "lookup";
  conjunct exists: state(sem(arg.name)) = LIVE lp at fn("lookup").call("find") prov code_a;
  outcome OK: ready narrow { api "lookup"; source main rev "abcdef0"; branch "found"; context any; captured name text; holds_at return; prov code_a; };
  outcome NOT_FOUND: not_ready(exists) documented "not found" prov doc_sup
    narrow { api "lookup"; source main rev "abcdef0"; branch "the search found no record"; context any; captured name text; holds_at return; prov code_a; };
  prov code_a;
}
op create(out id, name: Str) {
  bind symbol "create";
  effect publish: lifecycle sem(arg.name) -> LIVE filter lp.status = OK lp at fn("create").call("finalize") values status: Int prov code_a;
  outcome OK: ready;
  prov code_a;
}
op wipe() { bind symbol "wipe"; effect clear: lifecycle all sem -> ABSENT lp at fn("wipe").memset(Rec_t) prov code_a; prov code_a; }
op spawn(out tid) { bind symbol "spawn"; prov code_a; }
op entry(tid) { bind symbol "entry"; prov code_a; }

hb PO { title "program order"; stratum S; robust unconditional; template program_order { looper no; } prov code_a; }
hb SP { title "spawn"; stratum S; robust unconditional; template spawn { create spawn out tid; entry entry; } prov code_a; }
hb RF {
  title "reads-from";
  stratum O;
  robust obs_soft;
  template reads_from { use lookup.exists; outcome OK; publish create.publish; match id; }
  prov code_a;
}

rspec K-LOOKUP {
  use op lookup;
  key sem(arg.name);
  requires state(sem(arg.name)) = LIVE;
  oracle fns "find";
  appreq unspecified_by_reference;
  allowed unspecified_by_reference;
  harm app_error_exit;
  authored "unit test";
  prov code_a;
}
)KZ";

inline kz::phi::Profile load() {
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  std::vector<kz::phi::SourceText> files{kz::phi::SourceText{"state_mini.kzp", kProfile}};
  const bool ok = kz::phi::load_profile(files, &p, &diags).ok();
  if (!ok) {
    for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
    ++kztest::state().failures;
  }
  return p;
}

// A real record whose kind is covered and whose start state comes from the observed clearing.
inline const char* const kHeader =
    "record real\nstart_evidence init\nadequacy kind sem writers=covered aliases=covered lifetime=known\n"
    "observed lookup.exists\nobserved create.publish\nobserved wipe.clear\n";

// The actors' events (each actor's program order), merged in any order by the tests.
inline std::string m_start() { return "M wipe.clear pt\nM spawn call\nM spawn ret out.tid=1 result=0\nM spawn call\nM spawn ret out.tid=2 result=0\n"; }
inline std::string a_entry() { return "A entry call arg.tid=1\n"; }
inline std::string b_entry() { return "B entry call arg.tid=2\n"; }
inline std::string b_create(const char* id) { return std::string("B create call arg.name=@S0\nB create.publish pt lp.status=0\nB create ret out.id=") + id + " result=0\n"; }
inline std::string a_lookup(const char* result, const char* id) {
  return std::string("A lookup call arg.name=@S0\nA lookup.exists pt\nA lookup ret ") + (id != nullptr ? std::string("out.id=") + id + " " : std::string()) + "result=" + result + "\n";
}

}  // namespace kzstate

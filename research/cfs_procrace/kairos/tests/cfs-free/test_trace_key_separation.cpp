// Key separation of a field shared by the records of several kinds (`bind ... via`, 2026-10-24) in the static-closure
// adapter (kz/trace/static_closure.hpp).  A synthetic facts container and a small profile written here; every value is
// a unit-test value, not a research condition.
//
//   - a writer in a `via` function is covered where that function's operation has an observed effect point that writes
//     the bound component; a writer reached from it by direct calls likewise (a helper of the create);
//   - a writer that no `via` function reaches is key-separated (another kind's records), counted apart;
//   - a writer whose function is the target of an indirect call and is reached from no `via` function is not covered;
//   - an unobserved effect of a `via` operation leaves its writers uncovered, with the reason;
//   - a field bound without `via` keeps the earlier rule.

#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/trace/static_closure.hpp"
#include "support/kz_test.hpp"

namespace {

using kz::Container;

template <class T>
T zero() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

const char* const kProfile = R"KZ(phi 1;
profile "via-mini" revision "unit test";
source main root "" rev "abcdef0";
prov code_a { tier code-derived; kind code_pattern; side supplier; file "src/sem.c"; lines 5;
  sha256 "1111111111111111111111111111111111111111111111111111111111111111"; }
kind sem(name: Str) { states ABSENT < LIVE; initial ABSENT; scope process; prov code_a; }
bind field "Rec_t" "id" as state(sem(name)) via "wipe", "create", "destroy" prov code_a;
op create(out id, name: Str) {
  bind symbol "create";
  effect publish: lifecycle sem(arg.name) -> LIVE lp at fn("create").call("finalize") prov code_a;
  prov code_a;
}
op wipe() { bind symbol "wipe"; effect clear: lifecycle all sem -> ABSENT lp at fn("wipe").memset(Rec_t) prov code_a; prov code_a; }
op destroy(id) { bind symbol "destroy"; effect gone: lifecycle sem(lp.name) -> ABSENT lp at fn("destroy").call("fin") values name: Str prov code_a; prov code_a; }
)KZ";

kz::phi::Profile load(const std::string& text) {
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  std::vector<kz::phi::SourceText> files{kz::phi::SourceText{"via_mini.kzp", text}};
  KZ_CHECK_OK(kz::phi::load_profile(files, &p, &diags));
  return p;
}

// Rec_t.id writers: wipe's memset, the helper finish_tx (reached from create through alloc, and from other_create),
// other_create's own assign (another type), cb_target (an indirect-call target), destroy's assign.
Container make_facts() {
  kz::ContainerWriter w(KZ_ARTIFACT_FACTS);
  std::vector<kz_fact_loc> locs(1, zero<kz_fact_loc>());
  locs[0].loc_type = w.intern("Rec_t");
  locs[0].loc_field = w.intern("id");
  locs[0].status = KZ_LOC_STATUS_COMPLETE;
  std::vector<kz_fact_writer> writers;
  auto writer = [&](const char* file, uint32_t line, const char* func, uint8_t kind) {
    kz_fact_writer x = zero<kz_fact_writer>();
    x.loc_type = w.intern("Rec_t");
    x.loc_field = w.intern("id");
    x.file = w.intern(file);
    x.func = w.intern(func);
    x.line = line;
    x.kind = kind;
    x.key = w.intern("");
    x.value_text = w.intern("");
    x.via_file = w.intern("");
    writers.push_back(x);
  };
  writer("src/init.c", 5, "wipe", KZ_WRITER_KIND_MEMSET);
  writer("src/idmap.c", 20, "finish_tx", KZ_WRITER_KIND_ASSIGN);
  writer("src/other.c", 9, "other_create", KZ_WRITER_KIND_ASSIGN);
  writer("src/cb.c", 3, "cb_target", KZ_WRITER_KIND_ASSIGN);
  writer("src/del.c", 4, "destroy", KZ_WRITER_KIND_ASSIGN);
  std::vector<kz_fact_call> calls;
  auto call = [&](const char* file, uint32_t line, const char* caller, const char* callee) {
    kz_fact_call k = zero<kz_fact_call>();
    k.file = w.intern(file);
    k.line = line;
    k.caller = w.intern(caller);
    k.callee = w.intern(callee);
    calls.push_back(k);
  };
  call("src/sem.c", 10, "create", "alloc");
  call("src/idmap.c", 30, "alloc", "finish_tx");
  call("src/other.c", 12, "other_create", "finish_tx");
  call("src/main.c", 7, "main", "other_create");
  std::vector<kz_fact_func> funcs(1, zero<kz_fact_func>());
  funcs[0].fn = 0;
  funcs[0].tu = KZ_NONE32;
  funcs[0].name = w.intern("cb_target");
  funcs[0].file = w.intern("src/cb.c");
  std::vector<kz_fact_target> targets(1, zero<kz_fact_target>());
  targets[0].fn = 0;
  targets[0].kind = KZ_CALL_TARGET_KIND_INDIRECT;
  targets[0].via = w.intern("");
  KZ_CHECK_OK(w.add_section(KZ_SEC_FACT_LOCS, locs));
  KZ_CHECK_OK(w.add_section(KZ_SEC_WRITERS, writers));
  KZ_CHECK_OK(w.add_section(KZ_SEC_FACTS, calls));
  KZ_CHECK_OK(w.add_section(KZ_SEC_FUNCS, funcs));
  KZ_CHECK_OK(w.add_section(KZ_SEC_CALL_TARGETS, targets));
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  Container c;
  KZ_CHECK_OK(Container::parse(b, &c));
  return c;
}

bool has_prefix(const std::vector<std::string>& v, const std::string& x) {
  for (const std::string& s : v)
    if (s.compare(0, x.size(), x) == 0) return true;
  return false;
}

bool any_has(const std::vector<std::string>& v, const std::string& x) {
  for (const std::string& s : v)
    if (s.find(x) != std::string::npos) return true;
  return false;
}

void test_via() {
  const kz::phi::Profile p = load(kProfile);
  const Container facts = make_facts();
  const std::vector<kz::trace::ObservedSite> all = {{"wipe.clear", "/w/src/os.c", 50}, {"create.publish", "/w/src/sem.c", 12}, {"destroy.gone", "/w/src/del.c", 6}};
  {
    kz::eval::Trace t;
    kz::trace::ClosureReport cr;
    KZ_CHECK_OK(kz::trace::apply_static_closure(facts, p, all, &t, &cr));
    const kz::eval::StaticClosure& sc = t.kind_closure.at("sem");
    KZ_CHECK_EQ(sc.writers, 5u);
    KZ_CHECK_EQ(sc.covered, 4u);  // wipe, finish_tx (via create), other_create (separated), destroy
    KZ_CHECK(has_prefix(sc.unobserved, "Rec_t.id writer assign at src/cb.c:3 in cb_target (cb_target is the target of an indirect call"));
    KZ_CHECK(t.kind_adequacy.at("sem").aliases_covered && !t.kind_adequacy.at("sem").writers_covered);
    KZ_CHECK(any_has(cr.lines, "writers 5, at observed points 3, key-separated by 'via' 1; aliases covered, writers not covered"));
  }
  {
    const Container f2 = make_facts();
    kz::eval::Trace t;
    kz::trace::ClosureReport cr;
    // without the destroy point the delete's writer is not covered, with the reason
    const std::vector<kz::trace::ObservedSite> nodel = {{"wipe.clear", "/w/src/os.c", 50}, {"create.publish", "/w/src/sem.c", 12}};
    KZ_CHECK_OK(kz::trace::apply_static_closure(f2, p, nodel, &t, &cr));
    KZ_CHECK(has_prefix(t.kind_closure.at("sem").unobserved, "Rec_t.id writer assign at src/del.c:4 in destroy (reached from destroy, whose operation has no observed effect point that writes the state of sem)"));
    KZ_CHECK(!t.kind_adequacy.at("sem").writers_covered);
  }
  {
    // without the create point the helper's writer is not covered
    kz::eval::Trace t;
    kz::trace::ClosureReport cr;
    const std::vector<kz::trace::ObservedSite> nocreate = {{"wipe.clear", "/w/src/os.c", 50}, {"destroy.gone", "/w/src/del.c", 6}};
    KZ_CHECK_OK(kz::trace::apply_static_closure(facts, p, nocreate, &t, &cr));
    KZ_CHECK(has_prefix(t.kind_closure.at("sem").unobserved, "Rec_t.id writer assign at src/idmap.c:20 in finish_tx (reached from create"));
  }
  {
    // the same field without `via`: the earlier rule (every direct call of the writing function at a point that
    // writes it), so other_create's and finish_tx's writers are not covered
    std::string text = kProfile;
    const std::string from = " via \"wipe\", \"create\", \"destroy\"";
    text.replace(text.find(from), from.size(), "");
    const kz::phi::Profile q = load(text);
    kz::eval::Trace t;
    kz::trace::ClosureReport cr;
    KZ_CHECK_OK(kz::trace::apply_static_closure(facts, q, all, &t, &cr));
    KZ_CHECK(any_has(t.kind_closure.at("sem").unobserved, "in other_create"));
    KZ_CHECK(any_has(t.kind_closure.at("sem").unobserved, "in finish_tx"));
    KZ_CHECK(!any_has(cr.lines, "key-separated"));
  }
}

}  // namespace

int main() {
  test_via();
  return kztest::finish("test_trace_key_separation");
}

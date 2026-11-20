// Schema revision 12: the side tables of the neutral layer (kz/schema/kz_neutral.h sections 112-118,
// kz/trace/neutral.hpp) and the extraction sections 137-144 (kz/schema/kz_facts.h).  Everything here is a unit-test
// value, not a research condition (CONDITIONS_POLICY).
//
//   absent       a layer whose side tables are empty is written without any of sections 112-118, so a converter that
//                does not fill them writes the container it wrote before revision 12 (the same bytes for the same layer
//                and tool hash), and it reads back with empty side tables;
//   round trip   one record in each side table (strings included) is written, parses, verifies, dumps and reads back
//                equal, field by field and string by string;
//   checks       check_neutral refuses an index out of range or an enumeration value out of range in each side table;
//   facts        one record of each extraction section 137-144 in a facts container parses, verifies and dumps under
//                its section name; the public structures of kz/trace/run_analysis.hpp hold the side-table rows.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/sha256.hpp"
#include "kz/trace/neutral.hpp"
#include "kz/trace/run_analysis.hpp"
#include "support/kz_test.hpp"

namespace {

using kz::Container;
using kz::trace::NeutralRun;

template <class T>
T zero() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

bool has(const std::string& text, const std::string& what) { return text.find(what) != std::string::npos; }

// A small layer: one site, two actors (the second created by the first), three events.
NeutralRun base_layer() {
  NeutralRun n;
  n.run.r.backend = KZ_NBACKEND_KZRT;
  n.run.r.clock = KZ_NCLOCK_MONOTONIC;
  n.run.backend_name = "kzrt";
  kz::trace::NSite site;
  site.r.id = 1;
  site.r.kind = KZ_NSITE_KIND_CALL;
  site.r.bind = KZ_NONE32;
  site.label = "s1";
  site.symbol = "fx_create";
  n.sites.push_back(site);
  for (uint32_t a = 0; a < 2; ++a) {
    kz::trace::NActor actor;
    actor.r = kz::trace::nactor_init();
    actor.r.binding = KZ_NBIND_KZRT_ACTOR;
    actor.r.backend_id = a + 1;
    if (a == 1) actor.r.creator = 0;
    actor.name = a == 0 ? "main" : "child";
    n.actors.push_back(actor);
  }
  for (uint32_t i = 0; i < 3; ++i) {
    kz_nevent e = kz::trace::nevent_init();
    e.actor = i == 2 ? 1u : 0u;
    e.aseq = i;
    e.site = 0;
    e.bind = 1;
    n.events.push_back(e);
  }
  return n;
}

kz::Bytes write_layer(const NeutralRun& n) {
  kz::ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_tool_hash(kz::sha256("revision 12 side-table test tool"));
  kz::Bytes out;
  kz::Status s = kz::trace::add_neutral_sections(n, &w);
  KZ_CHECK_OK(s);
  if (s.ok()) KZ_CHECK_OK(w.finish(&out));
  return out;
}

bool parse(const kz::Bytes& b, Container* c) {
  kz::Status s = Container::parse(b, c);
  KZ_CHECK_OK(s);
  if (!s.ok()) return false;
  kz::Status v = c->verify_content();
  KZ_CHECK_OK(v);
  return v.ok();
}

const uint32_t kSideTables[] = {KZ_SEC_NACTOR_TOKEN, KZ_SEC_NLIFE, KZ_SEC_NBUILD, KZ_SEC_NSUFFICE,
                                KZ_SEC_NCALLEND,     KZ_SEC_NORDER, KZ_SEC_NORDER_OBS};

// ---- absent -------------------------------------------------------------------------------------------------------
void test_absent() {
  const NeutralRun n = base_layer();
  const kz::Bytes a = write_layer(n), b = write_layer(n);
  KZ_CHECK(a == b);
  Container c;
  if (!parse(a, &c)) return;
  for (uint32_t t : kSideTables) KZ_CHECK(c.find_section(t) == nullptr);
  KZ_CHECK_EQ(c.section_table().size(), static_cast<size_t>(9));  // STRINGS and NRUN..NCTL
  NeutralRun back;
  KZ_CHECK_OK(kz::trace::read_neutral(c, &back));
  KZ_CHECK(back.tokens.empty() && back.lifetimes.empty() && back.build_checks.empty() && back.needs.empty() &&
           back.call_ends.empty() && back.order_instances.empty() && back.order_observed.empty());
  KZ_CHECK_EQ(back.events.size(), n.events.size());
}

// ---- round trip ---------------------------------------------------------------------------------------------------
NeutralRun full_layer() {
  NeutralRun n = base_layer();
  kz::trace::NToken t;
  t.r = kz::trace::ntoken_init();
  t.r.actor = 1;
  t.r.token_actor = 1;
  t.r.token_aseq = 0;
  t.r.spawn_event = 0;
  t.r.spawn_return = 1;
  t.r.begin_event = 2;
  t.r.self_id = 7;
  t.r.self_event = 2;
  t.r.creator_id = 8;
  t.r.tie = KZ_NTIE_TOKEN_TIED;
  t.r.end_kind = KZ_NAEND_ENTRY_RETURN;
  t.r.self_check = KZ_NSELF_DIFFERS;
  t.r.root_kind = KZ_NROOT_START;
  t.r.start = 0x401000;
  t.r.arg = 0x2a;
  t.root_func = "fx_child_main";
  t.reason = "";
  n.tokens.push_back(t);

  kz::trace::NLife l0, l1;
  l0.r = kz::trace::nlife_init();
  l0.r.resource = KZ_NRES_NAMED_OBJECT;
  l0.r.key_value = 3;
  l0.r.begin_event = 0;
  l0.r.creator = 0;
  l0.r.end_kind = KZ_NLIFE_END_DELETED;
  l0.r.end_event = 1;
  l0.key_space = "fixture-table";
  l0.key_text = "fx_name";
  l1 = l0;
  l1.r.generation = 1;
  l1.r.previous = 0;
  l1.r.end_kind = KZ_NLIFE_END_OPEN_AT_END;
  l1.r.end_event = KZ_NONE32;
  l1.r.flags = KZ_NLIFE_FLAG_RE_CREATION;
  n.lifetimes.push_back(l0);
  n.lifetimes.push_back(l1);

  kz::trace::NBuild b;
  b.r = zero<kz_nbuild>();
  b.r.link = KZ_NBUILD_LINK_EXECUTABLE;
  b.r.verdict = KZ_NBUILD_VERDICT_REFUSED;
  b.r.flags = KZ_NBUILD_FLAG_EXPECTED_KNOWN | KZ_NBUILD_FLAG_OBSERVED_KNOWN;
  b.r.index = 0;
  b.r.expected_sha256[0] = 0x11;
  b.r.observed_sha256[0] = 0x22;
  b.path = "bin/fx";
  b.build_id = "abcd";
  b.reason = "the executable differs";
  n.build_checks.push_back(b);
  kz::trace::NBuild whole = b;
  whole.r = zero<kz_nbuild>();
  whole.r.link = KZ_NBUILD_LINK_RUN;
  whole.r.verdict = KZ_NBUILD_VERDICT_REFUSED;
  whole.path = "";
  whole.build_id = "";
  whole.reason = "one link refused";
  n.build_checks.push_back(whole);

  kz::trace::NSuffice x;
  x.r = kz::trace::nsuffice_init();
  x.r.site = 0;
  x.r.event = 0;
  x.r.state = KZ_NSUFF_VALUE_MISSING;
  x.r.slot = 1;
  x.need = "need s1 arg1";
  x.reason = "record_insufficient";
  x.note = "slot declared, value not valid";
  n.needs.push_back(x);

  kz::trace::NCallEnd ce;
  ce.r = kz::trace::ncallend_init();
  ce.r.event = 0;
  ce.r.actor = 0;
  ce.r.cause = KZ_NCALLEND_ACTOR_ENDED;
  ce.r.evidence = 2;
  ce.evidence_text = "the actor's end record follows the call";
  n.call_ends.push_back(ce);

  kz::trace::NOrder o;
  o.r = kz::trace::norder_init();
  o.r.from_event = 0;
  o.r.to_event = 2;
  o.r.from_actor = 0;
  o.r.to_actor = 1;
  o.r.token = 0;
  o.r.rule = KZ_NORDER_RULE_HB_SPAWN;
  o.r.kind = KZ_NORDER_KIND_HARD;
  o.r.result = KZ_NORDER_RESULT_INSTANTIATED;
  o.edge = "E0123456789abcdef";
  o.instance_key = "token 1:0";
  n.order_instances.push_back(o);

  kz::trace::NOrderObs ob;
  ob.r = kz::trace::norderobs_init();
  ob.r.actor = 1;
  ob.r.writer_actor = 0;
  ob.r.write_event = 1;
  ob.r.use_event = 2;
  ob.r.instance = 0;
  ob.r.observed = KZ_NOBS_WRITE_FIRST;
  ob.r.flags = KZ_NOBS_FLAG_ORDERED_BY_INSTANCE;
  ob.row_key = "0123456789abcdef";
  ob.order_static = "hard_before";
  n.order_observed.push_back(ob);
  return n;
}

void test_round_trip() {
  const NeutralRun n = full_layer();
  KZ_CHECK_OK(kz::trace::check_neutral(n));
  const kz::Bytes bytes = write_layer(n);
  Container c;
  if (!parse(bytes, &c)) return;
  for (uint32_t t : kSideTables) KZ_CHECK(c.find_section(t) != nullptr);
  const std::string dump = kz::dump_container(c);
  KZ_CHECK(has(dump, "section NACTOR_TOKEN type=112 version=1 record=kz_ntoken record_size=80 count=1"));
  KZ_CHECK(has(dump, "section NLIFE type=113 version=1 record=kz_nlife record_size=56 count=2"));
  KZ_CHECK(has(dump, "section NBUILD type=114 version=1 record=kz_nbuild record_size=104 count=2"));
  KZ_CHECK(has(dump, "section NSUFFICE type=115 version=1 record=kz_nsuffice record_size=44 count=1"));
  KZ_CHECK(has(dump, "section NCALLEND type=116 version=1 record=kz_ncallend record_size=28 count=1"));
  KZ_CHECK(has(dump, "section NORDER type=117 version=1 record=kz_norder record_size=60 count=1"));
  KZ_CHECK(has(dump, "section NORDER_OBS type=118 version=1 record=kz_norderobs record_size=48 count=1"));
  KZ_CHECK(has(dump, "token-tied"));
  KZ_CHECK(has(dump, "self-id-differs"));
  KZ_CHECK(has(dump, "re-creation"));
  KZ_CHECK(has(dump, "value-missing"));
  KZ_CHECK(has(dump, "HB-spawn"));
  KZ_CHECK(has(dump, "write-before-use"));
  NeutralRun back;
  KZ_CHECK_OK(kz::trace::read_neutral(c, &back));
  KZ_CHECK_EQ(back.tokens.size(), static_cast<size_t>(1));
  KZ_CHECK_EQ(back.lifetimes.size(), static_cast<size_t>(2));
  KZ_CHECK_EQ(back.build_checks.size(), static_cast<size_t>(2));
  KZ_CHECK_EQ(back.needs.size(), static_cast<size_t>(1));
  KZ_CHECK_EQ(back.call_ends.size(), static_cast<size_t>(1));
  KZ_CHECK_EQ(back.order_instances.size(), static_cast<size_t>(1));
  KZ_CHECK_EQ(back.order_observed.size(), static_cast<size_t>(1));
  if (back.tokens.size() != 1 || back.lifetimes.size() != 2 || back.build_checks.size() != 2 || back.needs.size() != 1 ||
      back.call_ends.size() != 1 || back.order_instances.size() != 1 || back.order_observed.size() != 1)
    return;
  // the records equal what was written, apart from the kz_str fields (they mean nothing in memory)
  auto same = [](auto a, auto b) {
    return std::memcmp(&a, &b, sizeof a) == 0;
  };
  kz_ntoken t0 = n.tokens[0].r, t1 = back.tokens[0].r;
  t0.root_func = t1.root_func = zero<kz_str>();
  t0.reason = t1.reason = zero<kz_str>();
  KZ_CHECK(same(t0, t1));
  KZ_CHECK_EQ(back.tokens[0].root_func, std::string("fx_child_main"));
  KZ_CHECK_EQ(back.lifetimes[1].r.previous, 0u);
  KZ_CHECK_EQ(back.lifetimes[1].key_text, std::string("fx_name"));
  KZ_CHECK_EQ(back.lifetimes[1].r.flags, static_cast<uint32_t>(KZ_NLIFE_FLAG_RE_CREATION));
  KZ_CHECK_EQ(back.build_checks[0].r.expected_sha256[0], 0x11);
  KZ_CHECK_EQ(back.build_checks[0].r.observed_sha256[0], 0x22);
  KZ_CHECK_EQ(back.build_checks[0].build_id, std::string("abcd"));
  KZ_CHECK_EQ(back.build_checks[1].reason, std::string("one link refused"));
  KZ_CHECK_EQ(back.needs[0].r.slot, 1);
  KZ_CHECK_EQ(back.needs[0].reason, std::string("record_insufficient"));
  KZ_CHECK_EQ(back.call_ends[0].evidence_text, std::string("the actor's end record follows the call"));
  KZ_CHECK_EQ(back.order_instances[0].edge, std::string("E0123456789abcdef"));
  KZ_CHECK_EQ(back.order_instances[0].r.token, 0u);
  KZ_CHECK_EQ(back.order_observed[0].order_static, std::string("hard_before"));
  KZ_CHECK_EQ(back.order_observed[0].r.instance, 0u);
  // written again from what was read: the same bytes
  KZ_CHECK(write_layer(back) == bytes);
}

// ---- checks -------------------------------------------------------------------------------------------------------
void test_checks() {
  auto refused = [](NeutralRun n, const char* what) {
    kz::Status s = kz::trace::check_neutral(n);
    KZ_CHECK(!s.ok());
    if (!s.ok()) KZ_CHECK(has(s.describe(), what));
  };
  NeutralRun n = full_layer();
  {
    NeutralRun m = n;
    m.tokens[0].r.actor = 9;
    refused(m, "NACTOR_TOKEN[0].actor");
  }
  {
    NeutralRun m = n;
    m.tokens[0].r.tie = 99;
    refused(m, "NACTOR_TOKEN[0] enum");
  }
  {
    NeutralRun m = n;
    m.lifetimes[0].r.previous = 1;  // a generation must name an earlier one
    refused(m, "NLIFE[0] index");
  }
  {
    NeutralRun m = n;
    m.build_checks[0].r.verdict = 3;
    refused(m, "NBUILD[0] enum");
  }
  {
    NeutralRun m = n;
    m.needs[0].r.loss = 0;  // the layer has no loss record
    refused(m, "NSUFFICE[0] index");
  }
  {
    NeutralRun m = n;
    m.call_ends[0].r.event = 3;
    refused(m, "NCALLEND[0] index");
  }
  {
    NeutralRun m = n;
    m.order_instances[0].r.life = 2;
    refused(m, "NORDER[0] index");
  }
  {
    NeutralRun m = n;
    m.order_observed[0].r.instance = 1;
    refused(m, "NORDER_OBS[0] index");
  }
}

// ---- facts sections 137-144 and the public structures ---------------------------------------------------------------
void test_facts_sections() {
  kz::ContainerWriter w(KZ_ARTIFACT_FACTS);
  w.set_tool_hash(kz::sha256("revision 12 side-table test tool"));
  auto one = [&](uint32_t type, auto rec) {
    std::vector<decltype(rec)> v{rec};
    KZ_CHECK_OK(w.add_section(type, v));
  };
  kz_fact_objacc oa = zero<kz_fact_objacc>();
  oa.read = KZ_NONE32;
  oa.writer = 0;
  oa.site = KZ_NONE32;
  oa.index_root = KZ_NONE32;
  oa.kind = KZ_OBJACC_KIND_WRITE;
  oa.base_kind = KZ_OBJBASE_GLOBAL_ELEMENT;
  oa.status = KZ_OBJACC_STATUS_RESOLVED;
  oa.type = w.intern("fx_rec");
  oa.field = w.intern("tab");
  oa.base_object = w.intern("fx_global");
  oa.path = w.intern("tab[i].id");
  one(KZ_SEC_OBJECT_ACCESS, oa);
  kz_fact_ftype ft = zero<kz_fact_ftype>();
  ft.extent = 20;
  ft.elem_size = 1;
  ft.elem_kind = KZ_FTYPE_ELEM_CHAR;
  ft.extent_kind = KZ_FTYPE_EXTENT_CONSTANT;
  ft.type = w.intern("fx_rec");
  ft.field = w.intern("name");
  ft.extent_text = w.intern("FX_NAME_MAX");
  one(KZ_SEC_FIELD_TYPES, ft);
  kz_fact_argtype at = zero<kz_fact_argtype>();
  at.read = KZ_NONE32;
  at.type_class = KZ_ARGT_CLASS_CHAR_ARRAY;
  at.read_dom = KZ_ARGT_READ_DOMINATING;
  one(KZ_SEC_ARG_TYPES, at);
  kz_fact_loopdef ld = zero<kz_fact_loopdef>();
  ld.value_root = ld.guard_root = KZ_NONE32;
  ld.where = KZ_LOOPDEF_WHERE_IN_BODY;
  ld.status = KZ_LOOPDEF_STATUS_RESET_IN_BODY;
  one(KZ_SEC_LOOP_DEFS, ld);
  kz_fact_sem se = zero<kz_fact_sem>();
  se.fn = se.pos = se.decl_fh = KZ_NONE32;
  se.value = -17;
  se.layer = KZ_SEM_LAYER_FRAMEWORK;
  se.column = KZ_SEM_COLUMN_ABSENCE;
  se.flags = KZ_SEM_FLAG_VALUE_SET;
  one(KZ_SEC_SEMANTICS, se);
  kz_fact_rflow rf = zero<kz_fact_rflow>();
  rf.use_site = rf.pos = rf.cond_root = KZ_NONE32;
  rf.def_kind = KZ_RFLOW_DEF_LOCAL_DEF;
  rf.use_kind = KZ_RFLOW_USE_BRANCH;
  rf.reach = KZ_RFLOW_REACH_UNIQUE;
  one(KZ_SEC_RESULT_FLOW, rf);
  kz_fact_reffect re = zero<kz_fact_reffect>();
  re.flow = 0;
  re.effect_site = KZ_NONE32;
  re.outcome = KZ_REFF_OUTCOME_ABSENCE;
  re.effect = KZ_REFF_EFFECT_FALLTHROUGH;
  re.status = KZ_REFF_STATUS_COMPLETE;
  one(KZ_SEC_RESULT_EFFECT, re);
  kz_fact_lockh lh = zero<kz_fact_lockh>();
  lh.handle_root = lh.writer = lh.sem = lh.lock_id = KZ_NONE32;
  lh.status = KZ_LOCKH_STATUS_STATIC_INSTANCE;
  lh.flags = KZ_LOCKH_FLAG_ONE_WRITER | KZ_LOCKH_FLAG_WRITER_ONCE;
  one(KZ_SEC_LOCK_HANDLES, lh);
  kz::Bytes out;
  KZ_CHECK_OK(w.finish(&out));
  Container c;
  if (!parse(out, &c)) return;
  const std::string dump = kz::dump_container(c);
  const char* heads[] = {"section OBJECT_ACCESS type=137 version=1 record=kz_fact_objacc record_size=72 count=1",
                         "section FIELD_TYPES type=138 version=1 record=kz_fact_ftype record_size=60 count=1",
                         "section ARG_TYPES type=139 version=1 record=kz_fact_argtype record_size=44 count=1",
                         "section LOOP_DEFS type=140 version=1 record=kz_fact_loopdef record_size=48 count=1",
                         "section SEMANTICS type=141 version=1 record=kz_fact_sem record_size=72 count=1",
                         "section RESULT_FLOW type=142 version=1 record=kz_fact_rflow record_size=76 count=1",
                         "section RESULT_EFFECT type=143 version=1 record=kz_fact_reffect record_size=72 count=1",
                         "section LOCK_HANDLES type=144 version=1 record=kz_fact_lockh record_size=56 count=1"};
  for (const char* h : heads) {
    KZ_CHECK(has(dump, h));
    if (!has(dump, h)) std::fprintf(stderr, "missing: %s\n", h);
  }
  KZ_CHECK(has(dump, "global-element"));
  KZ_CHECK(has(dump, "refused-reset-in-body"));
  KZ_CHECK(has(dump, "static-instance"));
  std::vector<kz_fact_sem> sem;
  KZ_CHECK_OK(c.read_section(KZ_SEC_SEMANTICS, &sem));
  KZ_CHECK(sem.size() == 1 && sem[0].value == -17);

  // the public structures hold the side-table rows
  kz::trace::SameBuildResult sb;
  KZ_CHECK_EQ(sb.verdict, static_cast<uint8_t>(KZ_NBUILD_VERDICT_UNCONFIRMED));
  kz::trace::TokenJoinResult tj;
  tj.tokens = full_layer().tokens;
  tj.by_tie[KZ_NTIE_TOKEN_TIED] = 1;
  KZ_CHECK_EQ(tj.tokens.size(), static_cast<size_t>(1));
  kz::trace::SufficeResult sr;
  kz::trace::LifetimeResult lr;
  kz::trace::OrderInstanceResult orr;
  KZ_CHECK(sr.needs.empty() && lr.lifetimes.empty() && orr.instances.empty() && orr.observed.empty());
}

}  // namespace

int main() {
  test_absent();
  test_round_trip();
  test_checks();
  test_facts_sections();
  return kztest::finish("test_trace_neutral_rev12");
}

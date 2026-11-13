// fixtures_data.cpp -- three small containers with meaningful content: facts, a run and
// verdicts.  The programs, functions and numbers are invented for the tests.

#include <cstring>

#include "fixtures.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/verdict.hpp"

namespace kzfix {

namespace {

kz_ref self_ref(uint32_t section, uint32_t index) {
  kz_ref r = zero<kz_ref>();
  r.section = section;
  r.index = index;
  return r;
}

kz_ref foreign_ref(const kz::Digest& artifact, uint32_t section, uint32_t index) {
  kz_ref r = self_ref(section, index);
  std::memcpy(r.artifact_sha256, artifact.data(), 32);
  return r;
}

void set_hash(uint8_t (&dst)[32], const kz::Digest& d) { std::memcpy(dst, d.data(), 32); }

kz::Digest unit_tool() { return fixture_tool_hash(); }

}  // namespace

kz::Bytes build_facts_fixture() {
  kz::ContainerWriter w(KZ_ARTIFACT_FACTS);
  w.set_basis_hash(kz::sha256("fixture basis"));
  w.set_profile_hash(kz::sha256("fixture profile"));
  w.set_tool_hash(unit_tool());

  std::vector<kz_prov> prov(3);
  for (auto& p : prov) p = zero<kz_prov>();
  prov[0].tier = KZ_TIER_DOC_BACKED;
  prov[0].kind = KZ_PROV_KIND_API_DOC;
  prov[0].side = KZ_PROV_SIDE_CALLER;
  prov[0].line_from = 10;
  prov[0].line_to = 12;
  prov[0].file = w.intern("example/inc/example_api.h");
  prov[0].authored = w.intern("unit test");
  set_hash(prov[0].text_sha256, kz::sha256("example api text"));
  prov[1].tier = KZ_TIER_RESEARCHER;  // a hand-entered entry: marked, and counted by reports
  prov[1].kind = KZ_PROV_KIND_RESEARCHER;
  prov[1].cite = w.intern("note 1");
  // prov[2] stays all zero: unreferenced.
  must(w.add_section(KZ_SEC_PROVENANCE, prov), "PROVENANCE");

  std::vector<kz_deviation> dev(1, zero<kz_deviation>());
  dev[0].subject = w.intern("number of runs N");
  dev[0].used_value = w.intern("3");
  dev[0].reason = w.intern("same count as the reference runs");
  dev[0].effect = w.intern("no frequency or overhead claim");
  dev[0].avoidable = KZ_AVOIDABLE_NO;
  dev[0].prov = KZ_NONE32;
  must(w.add_section(KZ_SEC_DEVIATIONS, dev), "DEVIATIONS");

  std::vector<kz_basis_item> basis(3, zero<kz_basis_item>());
  basis[0].name = w.intern("compiler");
  basis[0].value = w.intern("example-cc 1.0");
  basis[0].policy = KZ_POLICY_UNIT_TEST;
  basis[0].prov = KZ_NONE32;
  basis[0].deviation = KZ_NONE32;
  basis[1].name = w.intern("run count");
  basis[1].value = w.intern("3");
  basis[1].policy = KZ_POLICY_DEVIATION;
  basis[1].prov = KZ_NONE32;
  basis[1].deviation = 0;
  basis[2].name = w.intern("api constant");
  basis[2].value = w.intern("(silent)");
  basis[2].policy = KZ_POLICY_UNSPECIFIED;
  basis[2].prov = 0;
  basis[2].deviation = KZ_NONE32;
  must(w.add_section(KZ_SEC_BASIS, basis), "BASIS");

  std::vector<kz_file_hash> files(1, zero<kz_file_hash>());
  files[0].path = w.intern("example/src/producer.c");
  set_hash(files[0].sha256, kz::sha256("producer source"));
  files[0].size = 1234;
  files[0].prov = KZ_NONE32;
  must(w.add_section(KZ_SEC_FILE_HASHES, files), "FILE_HASHES");

  std::vector<kz_fact_call> calls(2, zero<kz_fact_call>());
  calls[0].site = 0;
  calls[0].tu = 0;
  calls[0].file = w.intern("example/src/producer.c");
  calls[0].caller = w.intern("example_producer_main");
  calls[0].callee = w.intern("example_publish");
  calls[0].line = 40;
  calls[0].col = 5;
  calls[0].first_arg = 0;
  calls[0].nargs = 2;
  calls[0].flags = KZ_CALL_FLAG_RESULT_USED;
  calls[1] = calls[0];
  calls[1].site = 1;
  calls[1].callee = w.intern("example_consume");
  calls[1].line = 52;
  calls[1].first_arg = 2;
  calls[1].nargs = 1;
  calls[1].flags = KZ_CALL_FLAG_RESULT_TESTED | KZ_CALL_FLAG_IN_LOOP;
  calls[1].loop_depth = 1;
  must(w.add_section(KZ_SEC_FACTS, calls), "FACTS");

  std::vector<kz_fact_arg> args(3, zero<kz_fact_arg>());
  args[0].site = 0;
  args[0].index = 0;
  args[0].kind = KZ_ARG_KIND_CONST;
  args[0].value = -1;
  args[1].site = 0;
  args[1].index = 1;
  args[1].kind = KZ_ARG_KIND_SYMBOLIC;
  args[1].role = KZ_ARG_ROLE_KEY;
  args[1].symbol = w.intern("example_state.ready");
  args[2].site = 1;
  args[2].index = 0;
  args[2].kind = KZ_ARG_KIND_TOP;
  must(w.add_section(KZ_SEC_FACT_ARGS, args), "FACT_ARGS");

  std::vector<kz_fact_writer> writers(2, zero<kz_fact_writer>());
  for (auto& x : writers) {
    x.loc_type = w.intern("example_state");
    x.loc_field = w.intern("ready");
    x.file = w.intern("example/src/producer.c");
    x.func = w.intern("example_producer_main");
    x.guard_root = KZ_NONE32;
  }
  writers[0].line = 38;
  writers[0].col = 3;
  writers[0].site = 10;
  writers[0].kind = KZ_WRITER_KIND_ASSIGN;
  writers[0].value_kind = KZ_ARG_KIND_CONST;
  writers[0].value = 1;
  writers[0].tstatus = KZ_TSTATUS_COMPUTED;
  writers[0].kstatus = KZ_KSTATUS_MUST_KEY;
  writers[0].wd = KZ_WD_W | KZ_WD_W_MUST;
  writers[1].line = 12;
  writers[1].col = 3;
  writers[1].site = 11;
  writers[1].kind = KZ_WRITER_KIND_MEMSET;
  writers[1].value_kind = KZ_ARG_KIND_CONST;
  writers[1].value = 0;
  writers[1].tstatus = KZ_TSTATUS_DECLARED_TRUSTED;
  writers[1].kstatus = KZ_KSTATUS_MAY_KEY;
  writers[1].wd = KZ_WD_D;
  must(w.add_section(KZ_SEC_WRITERS, writers), "WRITERS");

  std::vector<kz_skel_node> nodes(4, zero<kz_skel_node>());
  const uint8_t kinds[4] = {KZ_NODE_KIND_ENTRY, KZ_NODE_KIND_EFFECT, KZ_NODE_KIND_STORE, KZ_NODE_KIND_EXIT};
  for (uint32_t i = 0; i < 4; ++i) {
    nodes[i].actor = 0;
    nodes[i].kind = kinds[i];
    nodes[i].view = KZ_VIEW_CERT;
    nodes[i].site = (i == 0 || i == 3) ? KZ_NONE32 : 9 + i;
    nodes[i].profile_entry = KZ_NONE32;
    nodes[i].region = KZ_NONE32;
  }
  nodes[1].flags = KZ_NODE_FLAG_ONCE;
  must(w.add_section(KZ_SEC_SKEL_CERT, nodes), "SKEL_CERT");

  std::vector<kz_skel_edge> edges(3, zero<kz_skel_edge>());
  for (uint32_t i = 0; i < 3; ++i) {
    edges[i].from = i;
    edges[i].to = i + 1;
    edges[i].kind = KZ_EDGE_KIND_PO;
    edges[i].label = KZ_NONE32;
  }
  must(w.add_section(KZ_SEC_SKEL_CERT_EDGES, edges), "SKEL_CERT_EDGES");

  std::vector<kz_actor> actors(1, zero<kz_actor>());
  actors[0].actor = 0;
  actors[0].kind = KZ_ACTOR_KIND_STATIC_ENTRY;
  actors[0].once = 1;
  actors[0].entry_node = 0;
  actors[0].name = w.intern("example_producer_main");
  must(w.add_section(KZ_SEC_ACTORS, actors), "ACTORS");

  std::vector<kz_skel_once> once(1, zero<kz_skel_once>());
  once[0].subject_kind = KZ_ONCE_SUBJECT_ACTOR;
  once[0].rule = KZ_ONCE_RULE_ACTOR_ONCE;
  once[0].subject = 0;
  once[0].derivation = self_ref(KZ_SEC_ACTORS, 0);
  must(w.add_section(KZ_SEC_ONCE, once), "ONCE");

  kz::Bytes out;
  must(w.finish(&out), "finish");
  return out;
}

kz::Bytes build_run_fixture() {
  kz::ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_basis_hash(kz::sha256("fixture basis"));
  w.set_tool_hash(unit_tool());

  std::vector<kz_site> sites(2, zero<kz_site>());
  for (uint32_t i = 0; i < 2; ++i) {
    sites[i].site = i;
    sites[i].kind = KZ_SITE_KIND_ENTRY_PROBE;
    sites[i].insn_class = KZ_INSN_CLASS_EMULATED;
    sites[i].decoder = 1;
    sites[i].phases = KZ_SITE_PHASE_ENTRY;
    sites[i].file_offset = 0x1000u + 0x40u * i;
    sites[i].inode = 4242;
    sites[i].device = 1;
    for (uint8_t b = 0; b < 20; ++b) sites[i].build_id[b] = static_cast<uint8_t>(b + 1);
    sites[i].symbol = w.intern(i == 0 ? "example_publish" : "example_consume");
    sites[i].path = w.intern("example/bin/example_program");
  }
  must(w.add_section(KZ_SEC_SITES, sites), "SITES");

  std::vector<kz_raw_ev> events(4, zero<kz_raw_ev>());
  for (uint32_t i = 0; i < 4; ++i) {
    events[i].t_ns = 1000u * (i + 1);
    events[i].tid = 100 + (i % 2);
    events[i].tseq = i / 2;
    events[i].site = i % 2;
    events[i].phase = KZ_PHASE_ENTRY;
    events[i].a[0] = 0xdead0000u + i;
  }
  events[3].phase = KZ_PHASE_RET;
  events[3].flags = KZ_RAW_FLAG_OBJSEQ_VALID;
  events[3].objseq = 7;
  must(w.add_section(KZ_SEC_RAW_EVENTS, events), "RAW_EVENTS");

  std::vector<kz_loss> loss(1, zero<kz_loss>());
  loss[0].source = KZ_LOSS_SOURCE_PERF_RECORD_LOST;
  loss[0].cpu = 2;
  loss[0].site = KZ_NONE32;
  loss[0].count = 2;
  must(w.add_section(KZ_SEC_LOSS, loss), "LOSS");

  std::vector<kz_runmeta> meta(1, zero<kz_runmeta>());
  set_hash(meta[0].basis_sha256, kz::sha256("fixture basis"));
  set_hash(meta[0].plan_sha256, kz::sha256("fixture plan"));
  set_hash(meta[0].s0_sha256, kz::sha256("fixture initial state"));
  meta[0].run_ordinal = 1;
  meta[0].stratum = KZ_STRATUM_WARM;
  meta[0].backend = KZ_BACKEND_PERF_SAMPLE;
  meta[0].fidelity = KZ_FIDELITY_F0;
  meta[0].flags = KZ_RUN_FLAG_LOSS_PRESENT;
  meta[0].kernel = w.intern("example-kernel 1.2.3");
  meta[0].harness = w.intern("example-harness 0");
  meta[0].placement = w.intern("cpu 3");
  meta[0].observer_cpu_ns = 5000;
  meta[0].boot_ns = 1000000;
  must(w.add_section(KZ_SEC_RUNMETA, meta), "RUNMETA");

  std::vector<kz_perturbation> pert(2, zero<kz_perturbation>());
  pert[0].kind = KZ_PERTURBATION_KIND_PROBE_HITS;
  pert[0].cost_class = KZ_INSN_CLASS_EMULATED;
  pert[0].site = KZ_NONE32;
  pert[0].count = 4;
  pert[1].kind = KZ_PERTURBATION_KIND_OBSERVER_CPU;
  pert[1].cost_class = 0;
  pert[1].site = KZ_NONE32;
  pert[1].ns = 5000;
  must(w.add_section(KZ_SEC_PERTURBATION, pert), "PERTURBATION");

  std::vector<kz_deviation> dev(1, zero<kz_deviation>());
  dev[0].subject = w.intern("observer placement");
  dev[0].used_value = w.intern("cpu 3");
  dev[0].reason = w.intern("the reference does not fix a CPU");
  dev[0].effect = w.intern("timing of the observer only");
  dev[0].avoidable = KZ_AVOIDABLE_UNKNOWN;
  dev[0].prov = KZ_NONE32;
  must(w.add_section(KZ_SEC_DEVIATIONS, dev), "DEVIATIONS");

  kz::Bytes out;
  must(w.finish(&out), "finish");
  return out;
}

kz::Bytes build_verdicts_fixture(const kz::Digest& facts, const kz::Digest& run) {
  kz::ContainerWriter w(KZ_ARTIFACT_VERDICTS);
  w.set_basis_hash(kz::sha256("fixture basis"));
  w.set_profile_hash(kz::sha256("fixture profile"));
  w.set_tool_hash(unit_tool());
  must(w.add_parent(KZ_ARTIFACT_FACTS, 0, facts), "parent facts");
  must(w.add_parent(KZ_ARTIFACT_RUN, 0, run), "parent run");

  // Ledger of a certificate: a rechecked step, extracted facts, trusted entries (a profile
  // entry, a hand-declared one, an assumption and two lemmas that are not proved), and one
  // obligation that has not been run.
  std::vector<kz_trust> trust;
  auto entry = [&](uint8_t part, uint8_t kind) -> kz_trust& {
    trust.push_back(kz::make_trust_entry(part, kind));
    return trust.back();
  };
  {
    kz_trust& t = entry(KZ_TRUST_PART_RECHECKED, KZ_TRUST_KIND_CERT_STEP);
    t.status = KZ_TRUST_STATUS_ACCEPTED;
    t.ident = 0;
    t.bound = KZ_BOUND_INFINITE;
    t.source = self_ref(KZ_SEC_CERT_STEPS, 0);
  }
  {
    kz_trust& t = entry(KZ_TRUST_PART_RECHECKED, KZ_TRUST_KIND_CERT_STEP);
    t.status = KZ_TRUST_STATUS_ACCEPTED_VIA_T5;  // this step also called the shared evaluator
    t.ident = 1;
    t.bound = KZ_BOUND_INFINITE;
    t.source = self_ref(KZ_SEC_CERT_STEPS, 1);
  }
  {
    kz_trust& t = entry(KZ_TRUST_PART_EXTRACTED, KZ_TRUST_KIND_FACT);
    t.ident = 0;
    t.source = foreign_ref(facts, KZ_SEC_WRITERS, 0);
  }
  {
    kz_trust& t = entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_DECLARED_TRUSTED);
    t.ident = 7;
    t.source = foreign_ref(facts, KZ_SEC_WRITERS, 1);
  }
  {
    kz_trust& t = entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_ASSUMPTION);
    t.ident = 3;
  }
  {
    kz_trust& t = entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_LEMMA);  // only listed: unproved
    t.ident = 1;
  }
  {
    kz_trust& t = entry(KZ_TRUST_PART_TRUSTED, KZ_TRUST_KIND_LEMMA);
    t.ident = 2;
    t.proof = KZ_PROOF_DEPENDS;  // depends on a stated assumption (listed above as ASSUMPTION, id 3)
  }
  {
    kz_trust& t = entry(KZ_TRUST_PART_REFUTABLE, KZ_TRUST_KIND_OBLIGATION);
    t.status = KZ_TRUST_STATUS_NOT_RUN;
    t.ident = 4;
    t.refutes = 3;
    t.source = self_ref(KZ_SEC_OBLIGATIONS, 0);
  }
  must(w.add_section(KZ_SEC_TRUST, trust), "TRUST");
  std::vector<kz_trust_ledger> ledgers(1, kz::make_ledger(trust, 0));
  ledgers[0].facts = foreign_ref(facts, KZ_SEC_FACTS, 0);
  ledgers[0].checker = self_ref(KZ_SEC_PROVENANCE, 0);
  must(w.add_section(KZ_SEC_TRUST_LEDGERS, ledgers), "TRUST_LEDGERS");

  std::vector<kz_obligation> obligations(1, zero<kz_obligation>());
  obligations[0].obligation = KZ_OBL_O4;
  obligations[0].status = KZ_TRUST_STATUS_NOT_RUN;
  obligations[0].first_run = 0;
  obligations[0].n_runs = 0;
  obligations[0].exercised_limit = w.intern("not run");
  must(w.add_section(KZ_SEC_OBLIGATIONS, obligations), "OBLIGATIONS");

  std::vector<kz_prov> prov(1, zero<kz_prov>());
  prov[0].tier = KZ_TIER_CODE_DERIVED;
  prov[0].kind = KZ_PROV_KIND_CODE_PATTERN;
  prov[0].side = KZ_PROV_SIDE_RULE;
  prov[0].file = w.intern("example/src/producer.c");
  prov[0].line_from = 38;
  prov[0].line_to = 40;
  must(w.add_section(KZ_SEC_PROVENANCE, prov), "PROVENANCE");

  std::vector<kz_cert_step> steps(3, zero<kz_cert_step>());
  steps[0].rule = KZ_RULE_VIEW;
  steps[0].from_node = 0;
  steps[0].to_node = 3;
  steps[0].site_from = KZ_NONE32;
  steps[0].site_to = KZ_NONE32;
  steps[0].profile_entry = KZ_NONE32;
  steps[0].view = KZ_VIEW_CERT;
  steps[1].rule = KZ_RULE_WRITER;
  steps[1].from_node = 2;
  steps[1].to_node = 1;
  steps[1].site_from = 10;
  steps[1].site_to = 9;
  steps[1].profile_entry = 5;
  steps[1].tstatus = KZ_TSTATUS_COMPUTED;
  steps[1].kstatus = KZ_KSTATUS_MUST_KEY;
  steps[1].view = KZ_VIEW_CERT;
  steps[2].rule = KZ_RULE_BADNESS;
  steps[2].site_from = 11;
  steps[2].site_to = KZ_NONE32;
  steps[2].profile_entry = 6;
  steps[2].view = KZ_VIEW_CERT;
  steps[2].flags = KZ_STEP_FLAG_REJECT_EVENT | KZ_STEP_FLAG_AT_DECISION;
  must(w.add_section(KZ_SEC_CERT_STEPS, steps), "CERT_STEPS");

  std::vector<kz_cert> certs(1, zero<kz_cert>());
  certs[0].instance = 0x1234;
  certs[0].kind = KZ_CERT_KIND_FIXED_ORDER;
  certs[0].mlevel = KZ_MLEVEL_API;
  certs[0].flags = KZ_CERT_FLAG_DECIDES;
  certs[0].proof_status = KZ_PROOF_UNPROVED;  // the weakest of its ledger's lemmas: one is only listed
  certs[0].nsteps = 3;
  certs[0].first_step = 0;
  certs[0].trust = self_ref(KZ_SEC_TRUST_LEDGERS, 0);
  must(w.add_section(KZ_SEC_CERTIFICATES, certs), "CERTIFICATES");

  std::vector<kz_witness> witnesses(1, zero<kz_witness>());
  witnesses[0].kind = KZ_WITNESS_KIND_BAD;
  witnesses[0].status = KZ_OCC_VIOLATING;
  witnesses[0].naturalness = KZ_NATURALNESS_NATURAL;
  witnesses[0].realization = KZ_REALIZATION_REALIZED;
  witnesses[0].ordinal_k = 1;
  witnesses[0].actor = w.intern("example_producer_main#1");
  witnesses[0].run = foreign_ref(run, KZ_SEC_RUNMETA, 0);
  witnesses[0].events = foreign_ref(run, KZ_SEC_RAW_EVENTS, 0);
  witnesses[0].n_events = 2;
  must(w.add_section(KZ_SEC_WITNESSES, witnesses), "WITNESSES");

  std::vector<kz_bound> bounds(1, zero<kz_bound>());
  bounds[0].h = KZ_BOUND_INFINITE;
  bounds[0].policy = KZ_POLICY_UNIT_TEST;
  bounds[0].cls = KZ_CLS_FIXED;
  bounds[0].reach = KZ_REACH_PROVED;
  bounds[0].progress = KZ_PROGRESS_UNKNOWN;
  bounds[0].safety_routes = kz::route_set(0, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_CERT);
  bounds[0].flags = KZ_CELL_FLAG_DECIDES;
  bounds[0].n_bad = 1;
  bounds[0].witness_bad = self_ref(KZ_SEC_WITNESSES, 0);
  must(w.add_section(KZ_SEC_BOUNDS, bounds), "BOUNDS");

  std::vector<kz_verdict_cell> cells;
  {
    kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_API);
    c.cls = KZ_CLS_FIXED;
    c.unknown_reason = KZ_UNKNOWN_REASON_NONE;
    c.flags = KZ_CELL_FLAG_DECIDES;
    c.realization = KZ_REALIZATION_CERT;
    c.proof_status = KZ_PROOF_UNPROVED;  // PS3: shown next to the certificate-based statement
    c.safety_routes = kz::route_set(0, KZ_ROUTE_NOT_OK_SHIFT, KZ_ROUTE_CERT);
    c.reach = KZ_REACH_PROVED;
    c.runs = foreign_ref(run, KZ_SEC_RUNMETA, 0);
    c.observed_n = 1;
    c.observed_N = 1;
    c.bounds = self_ref(KZ_SEC_BOUNDS, 0);
    c.certificate = self_ref(KZ_SEC_CERTIFICATES, 0);
    c.trust = self_ref(KZ_SEC_TRUST_LEDGERS, 0);
    cells.push_back(c);
  }
  {
    kz_verdict_cell c = kz::make_cell(KZ_SIGMA_U, KZ_MLEVEL_IMPL);  // nothing established here
    c.fw_conf = KZ_FW_CONF_AS_DOCUMENTED;
    cells.push_back(c);
  }
  must(w.add_section(KZ_SEC_VERDICT_CELLS, cells), "VERDICT_CELLS");

  std::vector<kz_verdict> verdicts(1, kz::make_verdict(0x1234));
  verdicts[0].provider = KZ_PROVIDER_PRESENT;
  verdicts[0].outcome_label = KZ_OUTCOME_LABEL_VIOLATION;
  verdicts[0].shape = KZ_SHAPE_WRONG_WAY;
  verdicts[0].consequence = KZ_CONSEQUENCE_EVENT_REJECTED;
  verdicts[0].harm = KZ_HARM_EVENT_REQUEST_REJECTED;
  verdicts[0].fw_doc = KZ_FW_DOC_DOC_RESULT;
  verdicts[0].ncells = 2;
  verdicts[0].first_cell = 0;
  verdicts[0].provenance = self_ref(KZ_SEC_PROVENANCE, 0);
  must(w.add_section(KZ_SEC_VERDICTS, verdicts), "VERDICTS");

  std::vector<kz_ref> run_set(2, zero<kz_ref>());
  run_set[0] = foreign_ref(run, KZ_SEC_RUNMETA, 0);
  run_set[1] = foreign_ref(facts, KZ_SEC_FACTS, 0);
  must(w.add_section(KZ_SEC_REF_LISTS, run_set), "REF_LISTS");

  kz::Bytes out;
  must(w.finish(&out), "finish");
  return out;
}

}  // namespace kzfix

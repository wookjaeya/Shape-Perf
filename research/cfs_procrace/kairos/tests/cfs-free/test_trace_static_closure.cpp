// The run container adapter with a capture site, a publication site and the static closure, through the
// replay to its output container (R2 remainder, 2026-10-24): kz/trace/real_trace.hpp ("capture sites",
// "cells", "closure"), kz/trace/static_closure.hpp, kz/trace/replay_container.hpp (OBS_ADEQUACY `probes` in
// REF_LISTS, `unobserved`).  A synthetic run container and a synthetic facts container on the mini profile
// (tests/unit/support/phi_mini.hpp); every value is a unit-test value, not a research condition.
//
//   - a point site labelled `P@Q` of a capture point P maps to P; a slot role `value(L)` gives `value` and `cell`;
//   - the publication site box_pub names its actor as the writer of Box.id; the use that passed a value read
//     from Box.id is related to that actor;
//   - the static closure sets aliases-covered for a field whose status is writer-set-complete and
//     writers-covered only when every static writer is at an observed point that writes it (directly, or every
//     direct call of the writing function); a field outside the closure covers nothing; the unobserved
//     writer locations are listed;
//   - the output container lists the probes of each occurrence in REF_LISTS (null-terminated, the parent's
//     SITES) and the unobserved names.

#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/sha256.hpp"
#include "kz/trace/real_trace.hpp"
#include "kz/trace/replay_container.hpp"
#include "kz/trace/static_closure.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::Container;

template <class T>
T zero() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

kz::phi::Profile load() {
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));
  return p;
}

// Sites: 0 send entry, 1 send return, 2 the capture point send.who_valid at one call site, 3 the publication
// site box_pub (src/task.c:12), 4 create_all.reserve (src/create.c:30).  X reads Box.id (value 0) and sends it;
// W publishes Box.id; T0 reserves slot 2.
Container make_run() {
  kz::ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_basis_hash(kz::sha256(std::string("unit-test basis")));
  struct S {
    uint16_t kind;
    const char* symbol;
    const char* label;
    const char* source;
    uint32_t line;
  };
  const S defs[] = {{KZ_SITE_KIND_ENTRY_PROBE, "send", "send.entry", "", 0},
                    {KZ_SITE_KIND_RET_PROBE, "send", "send.ret", "", 0},
                    {KZ_SITE_KIND_POINT_PROBE, "caller", "send.who_valid@api_40", "/w/src/api.c", 40},
                    {KZ_SITE_KIND_POINT_PROBE, "task_init", "box_pub", "/w/src/task.c", 12},
                    {KZ_SITE_KIND_POINT_PROBE, "create_all", "create_all.reserve", "/w/src/create.c", 30}};
  std::vector<kz_site> sites;
  for (uint32_t i = 0; i < 5; ++i) {
    kz_site s = zero<kz_site>();
    s.site = i;
    s.kind = defs[i].kind;
    s.phases = defs[i].kind == KZ_SITE_KIND_ENTRY_PROBE ? KZ_SITE_PHASE_ENTRY : defs[i].kind == KZ_SITE_KIND_RET_PROBE ? KZ_SITE_PHASE_RET : KZ_SITE_PHASE_POINT;
    s.line = defs[i].line;
    s.symbol = w.intern(defs[i].symbol);
    s.label = w.intern(defs[i].label);
    s.source = w.intern(defs[i].source);
    sites.push_back(s);
  }
  std::vector<kz_site_slot> slots;
  auto slot = [&](uint32_t site, uint8_t k, const char* role) {
    kz_site_slot sl = zero<kz_site_slot>();
    sl.site = site;
    sl.slot = k;
    sl.src = KZ_SLOT_SRC_REG;
    sl.width = 4;
    sl.role = w.intern(role);
    slots.push_back(sl);
  };
  slot(0, 1, "who");
  slot(0, 3, "return_address");
  slot(1, 0, "status");
  slot(1, 3, "return_address");
  slot(2, 1, "value(Box.id)");
  slot(2, 2, "who");
  slot(4, 0, "n");
  std::vector<kz_thread> threads;
  const char* names[] = {"X", "W", "T0"};
  for (uint32_t i = 0; i < 3; ++i) {
    kz_thread t = zero<kz_thread>();
    t.pid = 10;
    t.tid = 10 + i;
    t.ptid = KZ_NONE32;
    t.announced_id = KZ_NONE32;
    t.creator_event = KZ_NONE32;
    t.first_event = KZ_NONE32;
    t.name = w.intern(names[i]);
    threads.push_back(t);
  }
  std::vector<kz_raw_ev> ev;
  auto add = [&](uint64_t t_ns, uint32_t tid, uint32_t site, uint16_t phase, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    kz_raw_ev e = zero<kz_raw_ev>();
    e.t_ns = t_ns;
    e.tid = tid;
    e.tseq = static_cast<uint32_t>(ev.size());
    e.site = site;
    e.phase = phase;
    e.a[0] = a0;
    e.a[1] = a1;
    e.a[2] = a2;
    e.a[3] = a3;
    ev.push_back(e);
  };
  add(100, 12, 4, KZ_PHASE_POINT, 2, 0, 0, 0);            // T0 reserves slot 2
  add(200, 10, 2, KZ_PHASE_POINT, 0, 0, 0, 0);            // X: the capture, value 0 read from Box.id
  add(201, 10, 0, KZ_PHASE_ENTRY, 0, 0, 0, 0x5000);       // X: send(Who = 0)
  add(202, 10, 1, KZ_PHASE_RET, 0xc0000001, 0, 0, 0x5000);  // X: NOT_READY_A
  add(300, 11, 3, KZ_PHASE_POINT, 0, 0, 0, 0);            // W: the publication site of Box.id
  std::vector<kz_map_ev> maps(1, zero<kz_map_ev>());
  maps[0].t_ns = 50;
  maps[0].pid = 10;
  maps[0].path = w.intern("/w/prog");
  std::vector<kz_runmeta> meta(1, zero<kz_runmeta>());
  meta[0].kernel = w.intern("");
  meta[0].harness = w.intern("");
  meta[0].placement = w.intern("");
  std::vector<kz_app_owner> owners(2, zero<kz_app_owner>());
  owners[0].thread = 0;
  owners[0].creator_thread = KZ_NONE32;
  owners[0].creation_event = KZ_NONE32;
  owners[0].owner_key = 0x1003;
  owners[0].level = KZ_OWNER_LEVEL_CONFIRMED;
  owners[0].app_name = w.intern("APP_X");
  owners[1] = owners[0];
  owners[1].thread = 1;
  owners[1].owner_key = 0x1002;
  owners[1].app_name = w.intern("APP_W");
  KZ_CHECK_OK(w.add_section(KZ_SEC_RAW_EVENTS, ev));
  KZ_CHECK_OK(w.add_section(KZ_SEC_SITES, sites));
  KZ_CHECK_OK(w.add_section(KZ_SEC_SITE_SLOTS, slots));
  KZ_CHECK_OK(w.add_section(KZ_SEC_THREADS, threads));
  KZ_CHECK_OK(w.add_section(KZ_SEC_MAPS, maps));
  KZ_CHECK_OK(w.add_section(KZ_SEC_RUNMETA, meta));
  KZ_CHECK_OK(w.add_section(KZ_SEC_ACTOR_OWNERS, owners));
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  Container c;
  KZ_CHECK_OK(Container::parse(b, &c));
  return c;
}

// The closure: Box_t.id complete with an out-pointer writer at the publication site's call and a memset in
// init_box; Slot_t.state complete, written by the helper set_used, called at the observed create.c:30 and at
// an unobserved other.c:7; Slot_t.stage is not a target.
Container make_facts() {
  kz::ContainerWriter w(KZ_ARTIFACT_FACTS);
  std::vector<kz_fact_loc> locs;
  auto loc = [&](const char* type, const char* field, uint8_t status) {
    kz_fact_loc l = zero<kz_fact_loc>();
    l.loc_type = w.intern(type);
    l.loc_field = w.intern(field);
    l.status = status;
    locs.push_back(l);
  };
  loc("Box_t", "id", KZ_LOC_STATUS_COMPLETE);
  loc("Slot_t", "state", KZ_LOC_STATUS_COMPLETE);
  std::vector<kz_fact_writer> writers;
  auto writer = [&](const char* type, const char* field, const char* file, uint32_t line, const char* func, uint8_t kind) {
    kz_fact_writer x = zero<kz_fact_writer>();
    x.loc_type = w.intern(type);
    x.loc_field = w.intern(field);
    x.file = w.intern(file);
    x.func = w.intern(func);
    x.line = line;
    x.kind = kind;
    x.key = w.intern("");
    x.value_text = w.intern("");
    x.via_file = w.intern("");
    writers.push_back(x);
  };
  writer("Box_t", "id", "src/task.c", 12, "task_init", KZ_WRITER_KIND_OUTPTR);
  writer("Box_t", "id", "src/init.c", 5, "init_box", KZ_WRITER_KIND_MEMSET);
  writer("Slot_t", "state", "src/util.h", 3, "set_used", KZ_WRITER_KIND_ASSIGN);
  writer("Slot_t", "state", "src/all.c", 9, "reset_all", KZ_WRITER_KIND_MEMSET);
  std::vector<kz_fact_call> calls;
  auto call = [&](const char* file, uint32_t line, const char* caller, const char* callee) {
    kz_fact_call k = zero<kz_fact_call>();
    k.file = w.intern(file);
    k.line = line;
    k.caller = w.intern(caller);
    k.callee = w.intern(callee);
    calls.push_back(k);
  };
  call("src/create.c", 30, "create_all", "set_used");
  call("src/other.c", 7, "free_up", "set_used");
  std::vector<kz_fact_func> funcs;
  KZ_CHECK_OK(w.add_section(KZ_SEC_FACT_LOCS, locs));
  KZ_CHECK_OK(w.add_section(KZ_SEC_WRITERS, writers));
  KZ_CHECK_OK(w.add_section(KZ_SEC_FACTS, calls));
  KZ_CHECK_OK(w.add_section(KZ_SEC_FUNCS, funcs));
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  Container c;
  KZ_CHECK_OK(Container::parse(b, &c));
  return c;
}

bool has(const std::vector<std::string>& v, const std::string& x) {
  for (const std::string& s : v)
    if (s == x) return true;
  return false;
}

bool has_prefix(const std::vector<std::string>& v, const std::string& x) {
  for (const std::string& s : v)
    if (s.compare(0, x.size(), x) == 0) return true;
  return false;
}

void test_adapter_closure_relation_container() {
  const kz::phi::Profile p = load();
  const Container run = make_run();
  const Container facts = make_facts();
  kz::trace::RealTraceOptions opt;
  opt.consts["TABLE"] = 4;
  opt.facts = &facts;
  kz::eval::Trace t;
  kz::trace::RealTraceReport rep;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(run, p, opt, &t, &rep));
  t.ref_consts["TABLE"] = 4;
  // the capture site maps to the capture point, with the location it names
  KZ_CHECK_EQ(rep.site_map.size(), static_cast<size_t>(5));
  KZ_CHECK(rep.site_map[2].kind == 3 && rep.site_map[2].name == "send.who_valid");
  KZ_CHECK(rep.site_map[3].kind == 3 && rep.site_map[3].name == "box_pub");
  KZ_CHECK(rep.site_map[0].kind == 1 && rep.site_map[0].name == "send");
  KZ_CHECK_EQ(rep.cell_values, 1u);
  bool cell = false;
  for (const kz::eval::TraceEvent& e : t.events) {
    const auto c = e.texts.find("cell");
    cell = cell || (e.site == "send.who_valid" && c != e.texts.end() && c->second == "Box.id" && e.values.count("value") == 1 && e.values.count("arg.Who") == 1);
  }
  KZ_CHECK(cell);
  // the closure
  KZ_CHECK(rep.closure_given);
  const kz::eval::StaticClosure& box = t.loc_closure.at("Box.id");
  KZ_CHECK(box.writers == 2 && box.covered == 1);
  KZ_CHECK(has_prefix(box.unobserved, "Box_t.id writer memset at src/init.c:5 in init_box"));
  KZ_CHECK(t.loc_adequacy.at("Box.id").aliases_covered && !t.loc_adequacy.at("Box.id").writers_covered);
  KZ_CHECK(t.loc_adequacy.at("Box.id").start_known);  // the INIT fact box_zero, from the start of the record, kept
  // a kind's start state is not declared by the adapter (the replay takes it from an observed effect on all of
  // it); its lifetime is known where every end of a lifetime in the profile is observed (none of slot's are:
  // reset_all.zero_slots, create_all.clear, free_up.free), and trivially for a kind whose profile ends none (reg)
  KZ_CHECK(!t.kind_adequacy.at("slot").start_known && !t.kind_adequacy.at("slot").lifetime_known);
  KZ_CHECK(!t.kind_adequacy.at("reg").start_known);
  KZ_CHECK_EQ(t.kind_adequacy.at("reg").lifetime_known, t.observed_points.count("sign_in.clear_reg") != 0);
  bool lifetime_line = false;
  for (const std::string& l : rep.lifetime_lines) lifetime_line = lifetime_line || l == "kind slot: lifetime not established: the end of a lifetime reset_all.zero_slots is not observed";
  KZ_CHECK(lifetime_line);
  const kz::eval::StaticClosure& slot = t.kind_closure.at("slot");
  KZ_CHECK(slot.writers == 2 && slot.covered == 0);  // set_used: the call at other.c:7 is not observed; reset_all's memset: no site
  KZ_CHECK(has_prefix(slot.unobserved, "Slot_t.state writer assign at src/util.h:3 in set_used (the call at src/other.c:7 (free_up) is not observed)"));
  KZ_CHECK(t.kind_adequacy.at("slot").aliases_covered && !t.kind_adequacy.at("slot").writers_covered);
  const kz::eval::StaticClosure& stage = t.kind_closure.at("stage");
  KZ_CHECK(has(stage.unobserved, "Slot_t.stage: not a target of the static closure"));
  KZ_CHECK(!t.kind_adequacy.at("stage").aliases_covered && !t.kind_adequacy.at("stage").writers_covered);
  KZ_CHECK(t.kind_closure.count("reg") == 0);  // no field of reg is bound: left as it was
  KZ_CHECK(kz::trace::report_text(rep).find("static closure loc Box.id: Box_t.id writer-set-complete; writers 2, at observed points 1; aliases covered, writers not covered") !=
           std::string::npos);

  // every static writer at an observed point: create.c:30 observes set_used's only call, a site at all.c:9 the memset
  {
    kz::trace::ClosureReport cr;
    kz::eval::Trace t2;
    std::vector<kz::trace::ObservedSite> obs = {{"create_all.reserve", "/w/src/create.c", 30}, {"create_all.reserve", "/w/src/other.c", 7}, {"reset_all.zero_slots", "/w/src/all.c", 9}};
    KZ_CHECK_OK(kz::trace::apply_static_closure(facts, p, obs, &t2, &cr));
    KZ_CHECK(t2.kind_adequacy.at("slot").writers_covered && t2.kind_adequacy.at("slot").aliases_covered);
    KZ_CHECK(t2.kind_closure.at("slot").unobserved.empty());
    // a point at the line that writes another component does not cover the writer
    std::vector<kz::trace::ObservedSite> wrong = {{"reset_all.zero_stages", "/w/src/all.c", 9}, {"create_all.reserve", "/w/src/create.c", 30}, {"create_all.reserve", "/w/src/other.c", 7}};
    kz::eval::Trace t3;
    KZ_CHECK_OK(kz::trace::apply_static_closure(facts, p, wrong, &t3, &cr));
    KZ_CHECK(!t3.kind_adequacy.at("slot").writers_covered);
    KZ_CHECK(has_prefix(t3.kind_closure.at("slot").unobserved, "Slot_t.state writer memset at src/all.c:9 in reset_all (the point reset_all.zero_stages observed at that line does not write the state of slot)"));
    // not a facts container
    KZ_CHECK(!kz::trace::apply_static_closure(run, p, obs, &t3, &cr).ok());
  }

  // the replay: the relation names W, the writer of the location the value was read from
  kz::eval::Result r;
  KZ_CHECK_OK(kz::eval::replay(p, t, kz::eval::Options(), &r));
  KZ_CHECK_EQ(r.occurrences.size(), static_cast<size_t>(1));
  if (r.occurrences.empty()) return;
  const kz::eval::Occurrence& o = r.occurrences[0];
  KZ_CHECK_EQ(o.summary.observation, static_cast<uint8_t>(KZ_OBS_OBSERVED_VIOLATION));
  KZ_CHECK_EQ(o.relation, static_cast<uint8_t>(KZ_EXEC_RELATION_INTER_APPLICATION));
  KZ_CHECK((o.summary.flags & KZ_SUMMARY_FLAG_CORE_STATUS_UNKNOWN) != 0);
  KZ_CHECK(o.counterparts == std::vector<std::string>{"W"});
  KZ_CHECK(o.relation_note.find("the passed value was read from Box.id, written by W") != std::string::npos);
  KZ_CHECK(has(o.probes, "box_pub") && has(o.probes, "send.who_valid") && has(o.probes, "call send") && has(o.probes, "return send"));
  KZ_CHECK(has(o.unobserved, "Box.id: Box_t.id writer memset at src/init.c:5 in init_box (init_box has no direct call in the closure)"));
  KZ_CHECK(has(o.unobserved, "reg: static closure not given"));

  // the output container
  kz::trace::ReplayContainerInput in;
  in.run = &run;
  in.profile_sha256 = kz::phi::profile_hash(p);
  in.tool_sha256 = kz::sha256(std::string("unit-test tool"));
  kz::Bytes bytes;
  KZ_CHECK_OK(kz::trace::write_replay_container(in, t, r, rep, &bytes));
  Container c;
  KZ_CHECK_OK(Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  std::vector<kz_obs_adequacy> adeq;
  std::vector<kz_ref> lists;
  KZ_CHECK_OK(c.read_section(KZ_SEC_OBS_ADEQUACY, &adeq));
  KZ_CHECK_OK(c.read_section(KZ_SEC_REF_LISTS, &lists));
  KZ_CHECK_EQ(adeq.size(), static_cast<size_t>(1));
  if (adeq.empty()) return;
  const kz_ref& pr = adeq[0].probes;
  KZ_CHECK(pr.section == KZ_SEC_REF_LISTS);
  std::vector<uint32_t> got;
  const kz::Digest run_hash = run.artifact_hash();
  for (size_t i = pr.index; i < lists.size(); ++i) {
    if (lists[i].section == 0) break;  // the null reference that ends the list
    KZ_CHECK(lists[i].section == KZ_SEC_SITES && std::memcmp(lists[i].artifact_sha256, run_hash.data(), 32) == 0);
    got.push_back(lists[i].index);
  }
  KZ_CHECK(got == (std::vector<uint32_t>{0, 1, 2, 3}));  // call, return, the capture site, the publication site
  std::string unobs;
  KZ_CHECK(c.string(adeq[0].unobserved, &unobs));
  KZ_CHECK(unobs.find("Box.id: Box_t.id writer memset at src/init.c:5 in init_box") != std::string::npos);
  KZ_CHECK(unobs.find(";reg: static closure not given") != std::string::npos || unobs.find("reg: static closure not given;") != std::string::npos);
}

}  // namespace

int main() {
  test_adapter_closure_relation_container();
  return kztest::finish("test_trace_static_closure");
}

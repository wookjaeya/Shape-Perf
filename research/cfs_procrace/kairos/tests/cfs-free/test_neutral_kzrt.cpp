// An instrumented run read by the replay (kz/trace/kzrt_convert.hpp, docs/schema.md section 12.4): the runtime of
// the instrumented family (kairos/runtime, POSIX adapter) writes a kzrt-dump in this process, the converter turns it
// into the neutral event layer with a unit-test site manifest, and the real-record replay reads the container on the
// mini profile (tests/unit/support/phi_mini.hpp).  Everything here is a unit-test value, not a research condition.
//
//   - a bootstrap event (before attach, provisional actor) is joined to the run actor that took its cell;
//   - each actor's events keep their aseq order and carry ASEQ_TARGET; an after event is linked to its before event;
//   - a call-site binding is a call/return of its callee, and a manifest point whose label names a profile point is
//     that point; a site whose only binding is removed is not an observed point; an undeclared site is a placeholder;
//   - the replay's trace from the container equals the one from the layer in memory, and the occurrences are found;
//   - a truncated stream is converted with its loss (the whole record) and without start evidence; a stream that is
//     not a kzrt-dump is refused.

#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/trace/kzrt_convert.hpp"
#include "kz/trace/neutral.hpp"
#include "kz/trace/real_trace.hpp"
#include "kz_rt.h"
#include "kz_rt_posix.h"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

const char* const kManifest =
    "kzs-sites 1\n"
    "source unit rev=0000000000000000000000000000000000000000 ref=\"tests/cfs-free/test_neutral_kzrt.cpp\"\n"
    "module 1 name=test_neutral_kzrt source=unit\n"
    "site 1 label=send.before op=send phase=before tu=t.c func=caller callee=send line=10\n"
    "value 1 0 role=Info from=arg0 type=u32\n"
    "value 1 1 role=Who from=arg1 type=u32\n"
    "site 2 label=send.after op=send phase=after tu=t.c func=caller callee=send line=10\n"
    "value 2 0 role=status from=ret type=u32\n"
    "site 3 label=send.who_valid op=send phase=point tu=t.c func=caller line=9\n"
    "value 3 0 role=Who from=operand0 type=u32\n"
    "site 4 label=gone op=other phase=point tu=t.c func=caller line=20\n"
    "value 4 0 role=x from=arg0 type=u32\n"
    "bind 101 site=1 backend=hand module=1 at=\"t.c:10\" status=bound\n"
    "bind 201 site=2 backend=hand module=1 at=\"t.c:10\" status=bound\n"
    "bind 301 site=3 backend=hand module=1 at=\"t.c:9\" status=bound\n"
    "bind 401 site=4 backend=hand module=1 at=\"t.c:20\" status=removed reason=\"unit test: inlined away\"\n";

int sink(void* ctx, const void* bytes, size_t n) {
  std::vector<uint8_t>* v = static_cast<std::vector<uint8_t>*>(ctx);
  v->insert(v->end(), static_cast<const uint8_t*>(bytes), static_cast<const uint8_t*>(bytes) + n);
  return 0;
}

void* worker(void*) {
  kz_rt_obs(3, 301, 0, 0x1u, 0x1003, 0, 0, 0);                    // the capture of Who
  const uint32_t c = kz_rt_obs(1, 101, 0, 0x3u, 7, 0x1003, 0, 0);  // send(Info = 7, Who = 0x1003)
  kz_rt_obs(2, 201, c, 0x1u, 0, 0, 0, 0);                          // OK
  kz_rt_actor_exit();
  return nullptr;
}

std::vector<uint8_t> record() {
  kz_rt_obs(3, 301, 0, 0x1u, 0x1002, 0, 0, 0);  // before init and attach: the bootstrap buffer, a provisional actor
  kz_rt_config cfg;
  std::memset(&cfg, 0, sizeof cfg);
  cfg.abi = KZ_RT_ABI_VERSION;
  cfg.mode = KZ_RT_MODE_RECORD;
  cfg.run_id = 42;
  cfg.instance_id = 1;
  cfg.backend = "hand";
  KZ_CHECK_EQ(kz_rt_init(&cfg), KZ_RT_OK);
  KZ_CHECK_EQ(kz_rt_attach(kz_rt_posix_adapter()), KZ_RT_OK);
  const uint32_t c = kz_rt_obs(1, 101, 0, 0x3u, 5, 0x1002, 0, 0);  // send(Info = 5, Who = 0x1002)
  kz_rt_obs(2, 201, c, 0x1u, 0xc0000001u, 0, 0, 0);                // NOT_READY_A
  kz_rt_obs(4, 401, 0, 0x1u, 1, 0, 0, 0);                          // at a binding the manifest says was removed
  kz_rt_obs(9, 901, 0, 0, 0, 0, 0, 0);                             // a site the manifest does not declare
  pthread_t t;
  KZ_CHECK_EQ(pthread_create(&t, nullptr, worker, nullptr), 0);
  KZ_CHECK_EQ(pthread_join(t, nullptr), 0);
  kz_rt_stop();
  std::vector<uint8_t> dump;
  KZ_CHECK_EQ(kz_rt_dump(sink, &dump), KZ_RT_OK);
  return dump;
}

bool has_note(const std::vector<std::string>& notes, const std::string& part) {
  for (const std::string& n : notes)
    if (n.find(part) != std::string::npos) return true;
  return false;
}

void test_convert_and_replay(const std::vector<uint8_t>& dump) {
  kz::trace::KzrtConvertInput in;
  in.dump = dump;
  in.dump_name = "unit.kzrt";
  in.manifests.push_back({"unit.kzs", kManifest});
  in.tool_sha256 = kz::sha256(std::string("unit-test tool"));
  kz::trace::KzrtConvertResult r;
  KZ_CHECK_OK(kz::trace::convert_kzrt_container(in, &r));
  const kz::trace::NeutralRun& n = r.layer;
  KZ_CHECK_EQ(n.run.r.backend, KZ_NBACKEND_KZRT);
  KZ_CHECK_EQ(n.run.r.start_evidence, KZ_NSTART_BOOTSTRAP);
  KZ_CHECK_EQ(n.run.r.clock, KZ_NCLOCK_MONOTONIC);
  KZ_CHECK_EQ(n.run.r.run_id, 42u);
  for (const kz_nloss& l : n.losses) std::printf("loss %s\n", kz::trace::loss_text(l).c_str());
  for (const std::string& note : r.notes) std::printf("note %s\n", note.c_str());
  KZ_CHECK((n.run.r.flags & KZ_NRUN_FLAG_COMPLETE) != 0 && (n.run.r.flags & KZ_NRUN_FLAG_LOSS_PRESENT) == 0);
  KZ_CHECK(has_note(r.notes, "binding 401 is declared removed but has events"));
  KZ_CHECK(has_note(r.notes, "site 9 is recorded but not declared"));
  // sites: four declared, one placeholder
  KZ_CHECK_EQ(n.sites.size(), static_cast<size_t>(5));
  KZ_CHECK_EQ(n.sites[0].r.kind, KZ_NSITE_KIND_CALL);
  KZ_CHECK_EQ(n.sites[0].symbol, std::string("send"));
  KZ_CHECK((n.sites[0].r.flags & (KZ_NSITE_FLAG_CALL_SITE | KZ_NSITE_FLAG_ATTACHED)) == (KZ_NSITE_FLAG_CALL_SITE | KZ_NSITE_FLAG_ATTACHED));
  KZ_CHECK_EQ(n.sites[1].r.kind, KZ_NSITE_KIND_RETURN);
  KZ_CHECK_EQ(n.sites[2].r.kind, KZ_NSITE_KIND_POINT);
  KZ_CHECK((n.sites[3].r.flags & KZ_NSITE_FLAG_ATTACHED) == 0);
  KZ_CHECK_EQ(n.sites[4].label, std::string("site9"));
  // actors: two run actors, the provisional one joined to the first
  size_t run_actors = 0;
  uint32_t main_actor = KZ_NONE32, prov = KZ_NONE32;
  for (size_t i = 0; i < n.actors.size(); ++i) {
    if (n.actors[i].r.binding == KZ_NBIND_KZRT_ACTOR && (n.actors[i].r.flags & KZ_NACTOR_FLAG_JOIN_RECORD) != 0) ++run_actors;
    if (n.actors[i].r.boot_link != KZ_NONE32) main_actor = static_cast<uint32_t>(i), prov = n.actors[i].r.boot_link;
  }
  KZ_CHECK_EQ(run_actors, static_cast<size_t>(2));
  KZ_CHECK(main_actor != KZ_NONE32 && prov != KZ_NONE32 && n.actors[prov].r.binding == KZ_NBIND_KZRT_PROVISIONAL);
  KZ_CHECK_EQ(n.actors[main_actor].r.native_kind, KZ_NKEY_LINUX_TID);
  KZ_CHECK_EQ(n.events.size(), static_cast<size_t>(8));
  KZ_CHECK_EQ(n.events[0].actor, main_actor);  // the bootstrap event first, under the run actor
  KZ_CHECK((n.events[0].flags & KZ_NEV_FLAG_BOOT) != 0 && (n.events[0].flags & KZ_NEV_FLAG_CLOCK) == 0);
  std::vector<uint32_t> last(n.actors.size(), 0);
  bool ordered = true, linked = false;
  for (size_t i = 0; i < n.events.size(); ++i) {
    const kz_nevent& e = n.events[i];
    ordered = ordered && (e.flags & KZ_NEV_FLAG_ASEQ_TARGET) != 0 && e.aseq > last[e.actor];
    last[e.actor] = e.aseq;
    if (e.site == 1 && e.link != KZ_NONE32) linked = linked || (n.events[e.link].site == 0 && n.events[e.link].actor == e.actor);
  }
  KZ_CHECK(ordered);
  KZ_CHECK(linked);
  // the container reads back to the same layer and is replayed
  kz::Container c;
  KZ_CHECK_OK(kz::Container::parse(r.container, &c));
  KZ_CHECK_OK(c.verify_content());
  KZ_CHECK(c.find_section(KZ_SEC_RAW_EVENTS) == nullptr && c.find_section(KZ_SEC_NEVENTS) != nullptr);
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));
  kz::trace::RealTraceOptions opt;
  opt.consts["TABLE"] = 4;
  kz::eval::Trace t, t2;
  kz::trace::RealTraceReport rep, rep2;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(c, p, opt, &t, &rep));
  KZ_CHECK_OK(kz::trace::real_trace_from_neutral(n, p, opt, &t2, &rep2));
  KZ_CHECK(kz::eval::trace_text(t) == kz::eval::trace_text(t2));
  KZ_CHECK(rep.neutral_sections);
  KZ_CHECK_EQ(rep.start, KZ_NSTART_BOOTSTRAP);
  KZ_CHECK_EQ(rep.threads, 2u);
  KZ_CHECK_EQ(rep.events, 6u);  // two captures, two calls, two returns; the removed binding's site and the placeholder are not mapped
  KZ_CHECK(t.observed_points.count("send.who_valid") != 0);
  KZ_CHECK_EQ(t.start_evidence, KZ_START_EVIDENCE_INIT);
  KZ_CHECK_EQ(t.events[1].site, std::string("send"));
  KZ_CHECK(t.events[1].phase == kz::eval::Phase::Call && t.events[1].values.count("arg.Who") != 0 && t.events[1].values.at("arg.Who") == 0x1002);
  KZ_CHECK(t.events[2].phase == kz::eval::Phase::Return && t.events[2].has_result && t.events[2].result == static_cast<int64_t>(0xc0000001u));
  KZ_CHECK_EQ(t.events[0].actor, t.events[1].actor);  // the bootstrap capture and the call: one actor
  t.ref_consts["TABLE"] = 4;  // as kz replay does with --const
  kz::eval::Result res;
  KZ_CHECK_OK(kz::eval::replay(p, t, kz::eval::Options(), &res));
  KZ_CHECK_EQ(res.occurrences.size(), static_cast<size_t>(2));
}

void test_truncated(const std::vector<uint8_t>& dump) {
  kz::trace::KzrtConvertInput in;
  in.dump.assign(dump.begin(), dump.end() - 24);  // without END
  in.dump_name = "cut.kzrt";
  in.manifests.push_back({"unit.kzs", kManifest});
  kz::trace::KzrtConvertResult r;
  KZ_CHECK_OK(kz::trace::neutral_from_kzrt(in, &r));
  KZ_CHECK_EQ(r.layer.run.r.start_evidence, KZ_NSTART_NONE);
  KZ_CHECK((r.layer.run.r.flags & KZ_NRUN_FLAG_LOSS_PRESENT) != 0 && (r.layer.run.r.flags & KZ_NRUN_FLAG_COMPLETE) == 0);
  bool truncated = false;
  for (const kz_nloss& l : r.layer.losses) truncated = truncated || (l.kind == KZ_NLOSS_TRUNCATED_STREAM && l.begin == 0 && l.end == KZ_NONE32);
  KZ_CHECK(truncated);
  in.dump[0] = 'X';
  KZ_CHECK(!kz::trace::neutral_from_kzrt(in, &r).ok());
  in.dump = dump;
  in.manifests[0].second = "kzs-sites 1\nsite x\n";
  KZ_CHECK(!kz::trace::neutral_from_kzrt(in, &r).ok());
}

}  // namespace

int main() {
  const std::vector<uint8_t> dump = record();
  test_convert_and_replay(dump);
  test_truncated(dump);
  return kztest::finish("test_neutral_kzrt");
}

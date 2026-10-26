// The identity join of an instrumented run (kz/trace/identity_rule.hpp, kzs-sites 2 `identity`): the runtime of the
// instrumented family (kairos/runtime, POSIX adapter) writes a kzrt-dump in this process with real threads, the
// converter reads it with a version 1 site file and a version 2 identity file, and the actors get their creator and
// owner by the id alone.  Every name and value here is a unit-test value, not a research condition.
//
//   - a task whose announced id equals the id its creator's spawn call passed to it (and returned with success) is
//     tied to that call: creator, creator_event, spawn_return, owner `confirmed` from the owner request open on the
//     creator (owner_from_request, the rule F0 uses);
//   - a child task (child request) takes its creator's owner, and the request's calling function is the call-site
//     binding's func;
//   - the id passed to the task and the id returned disagree: no tie (conflicting), the owner is undetermined;
//   - a call that reports failure: its out parameter is not used (it keeps its old content);
//   - two tasks announce the same id: ambiguous, no tie;
//   - without identity reads (the version 1 file alone) nothing is joined and the run has no owner records;
//   - the replay reads the owners from the layer (confirmed 2) and the identity notes are in the container.

#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/trace/identity_rule.hpp"
#include "kz/trace/kzrt_convert.hpp"
#include "kz/trace/neutral.hpp"
#include "kz/trace/real_trace.hpp"
#include "kz_rt.h"
#include "kz_rt_posix.h"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

const char* const kSites =
    "kzs-sites 1\n"
    "source unit rev=0000000000000000000000000000000000000000 ref=\"tests/cfs-free/test_kzrt_identity.cpp\"\n"
    "module 1 name=test_kzrt_identity source=unit\n"
    "site 1 label=start.before op=start phase=before tu=t.c func=start_app\n"
    "value 1 2 role=owner_id from=arg4 type=u32\n"
    "site 2 label=start.after op=start phase=after tu=t.c func=start_app\n"
    "value 2 0 role=status from=ret type=s32\n"
    "site 3 label=child.before op=child phase=before tu=t.c func=app_main callee=make_child line=30\n"
    "site 4 label=child.after op=child phase=after tu=t.c func=app_main callee=make_child line=30\n"
    "value 4 0 role=status from=ret type=s32\n"
    "site 5 label=create.before op=create phase=before tu=t.c func=task_create\n"
    "site 6 label=create.after op=create phase=after tu=t.c func=task_create\n"
    "value 6 0 role=status from=ret type=s32\n"
    "value 6 1 role=task_id from=out0 type=u32\n"
    "site 7 label=create.id op=create phase=before tu=t.c func=create_impl\n"
    "value 7 0 role=task_id from=arg4 type=u32\n"
    "site 8 label=task.entry op=entry phase=before tu=t.c func=task_entry\n"
    "value 8 0 role=task_id from=arg0 type=u32\n"
    "bind 101 site=1 backend=hand module=1 at=\"t.c start_app\" status=bound\n"
    "bind 201 site=2 backend=hand module=1 at=\"t.c start_app\" status=bound\n"
    "bind 301 site=3 backend=hand module=1 at=\"t.c:30\" status=bound\n"
    "bind 401 site=4 backend=hand module=1 at=\"t.c:30\" status=bound\n"
    "bind 501 site=5 backend=hand module=1 at=\"t.c task_create\" status=bound\n"
    "bind 601 site=6 backend=hand module=1 at=\"t.c task_create\" status=bound\n"
    "bind 701 site=7 backend=hand module=1 at=\"t.c create_impl\" status=bound\n"
    "bind 801 site=8 backend=hand module=1 at=\"t.c task_entry\" status=bound\n";

const char* const kIdentity =
    "kzs-sites 2\n"
    "identity 1 decoder=owner-request owner=2 ref=\"unit test\"\n"
    "identity 2 decoder=request-return\n"
    "identity 3 decoder=child-request\n"
    "identity 4 decoder=request-return\n"
    "identity 5 decoder=spawn-create\n"
    "identity 6 decoder=spawn-return id=1 status=0 ok=0\n"
    "identity 7 decoder=spawn-id id=0\n"
    "identity 8 decoder=thread-entry id=0\n";

int sink(void* ctx, const void* bytes, size_t n) {
  std::vector<uint8_t>* v = static_cast<std::vector<uint8_t>*>(ctx);
  v->insert(v->end(), static_cast<const uint8_t*>(bytes), static_cast<const uint8_t*>(bytes) + n);
  return 0;
}

void* announce(void* id) {
  kz_rt_obs(8, 801, 0, 0x1u, reinterpret_cast<uintptr_t>(id), 0, 0, 0);
  kz_rt_actor_exit();
  return nullptr;
}

// A task that creates a child task of its own application (a child request around a spawn call).
void* app_with_child(void*) {
  kz_rt_obs(8, 801, 0, 0x1u, 0x10001, 0, 0, 0);
  const uint32_t q = kz_rt_obs(3, 301, 0, 0, 0, 0, 0, 0);
  const uint32_t c = kz_rt_obs(5, 501, 0, 0, 0, 0, 0, 0);
  kz_rt_obs(7, 701, 0, 0x1u, 0x10003, 0, 0, 0);
  pthread_t t;
  KZ_CHECK_EQ(pthread_create(&t, nullptr, announce, reinterpret_cast<void*>(static_cast<uintptr_t>(0x10003))), 0);
  kz_rt_obs(6, 601, c, 0x3u, 0, 0x10003, 0, 0);
  kz_rt_obs(4, 401, q, 0x1u, 0, 0, 0, 0);
  KZ_CHECK_EQ(pthread_join(t, nullptr), 0);
  kz_rt_actor_exit();
  return nullptr;
}

std::vector<uint8_t> record() {
  kz_rt_config cfg;
  std::memset(&cfg, 0, sizeof cfg);
  cfg.abi = KZ_RT_ABI_VERSION;
  cfg.mode = KZ_RT_MODE_RECORD;
  cfg.run_id = 7;
  cfg.backend = "hand";
  KZ_CHECK_EQ(kz_rt_init(&cfg), KZ_RT_OK);
  KZ_CHECK_EQ(kz_rt_attach(kz_rt_posix_adapter()), KZ_RT_OK);
  pthread_t t1, t2, t4a, t4b;
  // application 0x110001: its task (id 0x10001) is tied by the id the call passed and returned
  uint32_t a = kz_rt_obs(1, 101, 0, 0x4u, 0, 0, 0x110001, 0);
  uint32_t c = kz_rt_obs(5, 501, 0, 0, 0, 0, 0, 0);
  kz_rt_obs(7, 701, 0, 0x1u, 0x10001, 0, 0, 0);
  KZ_CHECK_EQ(pthread_create(&t1, nullptr, app_with_child, nullptr), 0);
  kz_rt_obs(6, 601, c, 0x3u, 0, 0x10001, 0, 0);
  kz_rt_obs(2, 201, a, 0x1u, 0, 0, 0, 0);
  KZ_CHECK_EQ(pthread_join(t1, nullptr), 0);
  // application 0x110002: the id passed to the task (0x10002) and the id returned (0x10009) disagree
  a = kz_rt_obs(1, 101, 0, 0x4u, 0, 0, 0x110002, 0);
  c = kz_rt_obs(5, 501, 0, 0, 0, 0, 0, 0);
  kz_rt_obs(7, 701, 0, 0x1u, 0x10002, 0, 0, 0);
  KZ_CHECK_EQ(pthread_create(&t2, nullptr, announce, reinterpret_cast<void*>(static_cast<uintptr_t>(0x10002))), 0);
  kz_rt_obs(6, 601, c, 0x3u, 0, 0x10009, 0, 0);
  kz_rt_obs(2, 201, a, 0x1u, 0, 0, 0, 0);
  KZ_CHECK_EQ(pthread_join(t2, nullptr), 0);
  // application 0x110003: the call fails (status -1); its out parameter still holds an old value
  a = kz_rt_obs(1, 101, 0, 0x4u, 0, 0, 0x110003, 0);
  c = kz_rt_obs(5, 501, 0, 0, 0, 0, 0, 0);
  kz_rt_obs(6, 601, c, 0x3u, static_cast<uint64_t>(-1), 0x10005, 0, 0);
  kz_rt_obs(2, 201, a, 0x1u, static_cast<uint64_t>(-1), 0, 0, 0);
  // a spawn call outside any request whose id two tasks announce
  c = kz_rt_obs(5, 501, 0, 0, 0, 0, 0, 0);
  kz_rt_obs(7, 701, 0, 0x1u, 0x10007, 0, 0, 0);
  KZ_CHECK_EQ(pthread_create(&t4a, nullptr, announce, reinterpret_cast<void*>(static_cast<uintptr_t>(0x10007))), 0);
  KZ_CHECK_EQ(pthread_create(&t4b, nullptr, announce, reinterpret_cast<void*>(static_cast<uintptr_t>(0x10007))), 0);
  kz_rt_obs(6, 601, c, 0x3u, 0, 0x10007, 0, 0);
  KZ_CHECK_EQ(pthread_join(t4a, nullptr), 0);
  KZ_CHECK_EQ(pthread_join(t4b, nullptr), 0);
  kz_rt_stop();
  std::vector<uint8_t> dump;
  KZ_CHECK_EQ(kz_rt_dump(sink, &dump), KZ_RT_OK);
  return dump;
}

// The actor that announced `id` (KZ_NONE32 if none or more than one).
uint32_t announcer(const kz::trace::NeutralRun& n, uint32_t id) {
  uint32_t found = KZ_NONE32, count = 0;
  for (size_t i = 0; i < n.actors.size(); ++i)
    if ((n.actors[i].r.join_flags & KZ_THREAD_FLAG_ENTRY_SEEN) != 0 && n.actors[i].r.announced_id == id) found = static_cast<uint32_t>(i), ++count;
  return count == 1 ? found : KZ_NONE32;
}

bool has_note(const std::vector<std::string>& notes, const std::string& part) {
  for (const std::string& n : notes)
    if (n.find(part) != std::string::npos) return true;
  return false;
}

void test_join(const std::vector<uint8_t>& dump) {
  kz::trace::KzrtConvertInput in;
  in.dump = dump;
  in.dump_name = "unit.kzrt";
  in.manifests.push_back({"sites.kzs", kSites});
  in.manifests.push_back({"identity.kzs", kIdentity});
  in.tool_sha256 = kz::sha256(std::string("unit-test tool"));
  kz::trace::KzrtConvertResult r;
  KZ_CHECK_OK(kz::trace::convert_kzrt_container(in, &r));
  for (const std::string& note : r.notes) std::printf("note %s\n", note.c_str());
  const kz::trace::NeutralRun& n = r.layer;
  KZ_CHECK((n.run.r.flags & KZ_NRUN_FLAG_OWNERS_RECORDED) != 0);
  KZ_CHECK(has_note(r.notes, "tied by the id 2,"));
  KZ_CHECK(has_note(r.notes, "conflicting 1"));
  KZ_CHECK(has_note(r.notes, "not success 1"));
  KZ_CHECK(has_note(r.notes, "ambiguous 2"));
  KZ_CHECK_EQ(n.sites[0].r.decoder, KZ_DECODER_OWNER_REQUEST);
  KZ_CHECK_EQ(n.sites[7].r.decoder, KZ_DECODER_THREAD_ENTRY);
  // the main thread: the creator of the first task
  const uint32_t t1 = announcer(n, 0x10001), t3 = announcer(n, 0x10003), t2 = announcer(n, 0x10002);
  KZ_CHECK(t1 != KZ_NONE32 && t3 != KZ_NONE32 && t2 != KZ_NONE32);
  const kz::trace::NActor& a1 = n.actors[t1];
  KZ_CHECK((a1.r.join_flags & (KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_ID_MATCHED)) == (KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_ID_MATCHED));
  KZ_CHECK((a1.r.join_flags & (KZ_THREAD_FLAG_FORK_SEEN | KZ_THREAD_FLAG_MATCH_BY_TIME | KZ_THREAD_FLAG_ID_UNVERIFIED)) == 0);
  KZ_CHECK(a1.r.creator != KZ_NONE32 && a1.r.creator != t1);
  const uint32_t main_actor = a1.r.creator;
  KZ_CHECK_EQ(n.sites[n.events[a1.r.creator_event].site].label, std::string("create.before"));
  KZ_CHECK(a1.r.spawn_return != KZ_NONE32 && n.sites[n.events[a1.r.spawn_return].site].label == "create.after");
  KZ_CHECK_EQ(n.events[a1.r.spawn_return].actor, main_actor);
  KZ_CHECK_EQ(a1.r.owner_level, KZ_OWNER_LEVEL_CONFIRMED);
  KZ_CHECK_EQ(a1.r.owner_id, 0x110001u);
  KZ_CHECK_EQ(a1.r.owner_evidence, static_cast<uint32_t>(KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_ANNOUNCED_ID));
  KZ_CHECK_EQ(n.sites[n.events[a1.r.owner_request].site].label, std::string("start.before"));
  KZ_CHECK(a1.request_func.empty());  // bound at the function's entry: the calling function is not known
  // the child task of the first task: its creator's application, through the child request
  const kz::trace::NActor& a3 = n.actors[t3];
  KZ_CHECK_EQ(a3.r.creator, t1);
  KZ_CHECK_EQ(a3.r.owner_level, KZ_OWNER_LEVEL_CONFIRMED);
  KZ_CHECK_EQ(a3.r.owner_id, 0x110001u);
  KZ_CHECK((a3.r.owner_evidence & KZ_OWNER_EVIDENCE_CREATOR_OWNER) != 0 && (a3.r.owner_evidence & KZ_OWNER_EVIDENCE_ANALYSED_TABLE_EXCLUDED) == 0);
  KZ_CHECK_EQ(a3.request_func, std::string("app_main"));
  KZ_CHECK_EQ(a3.r.owner_creator, t1);
  // the disagreeing ids: no tie, undetermined, never "no owner"
  const kz::trace::NActor& a2 = n.actors[t2];
  KZ_CHECK(a2.r.creator == KZ_NONE32 && (a2.r.join_flags & KZ_THREAD_FLAG_ENTRY_UNMATCHED) != 0 && (a2.r.join_flags & KZ_THREAD_FLAG_ID_UNVERIFIED) != 0);
  KZ_CHECK(a2.r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED && a2.r.owner_id == KZ_NONE32 && (a2.r.flags & KZ_NACTOR_FLAG_OWNER_RECORDED) != 0);
  // the two tasks with one id
  size_t ambiguous = 0;
  for (const kz::trace::NActor& a : n.actors)
    if (a.r.announced_id == 0x10007u) {
      KZ_CHECK((a.r.join_flags & KZ_THREAD_FLAG_SPAWN_AMBIGUOUS) != 0 && a.r.creator == KZ_NONE32 && a.r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED);
      ++ambiguous;
    }
  KZ_CHECK_EQ(ambiguous, static_cast<size_t>(2));
  // the main thread announced nothing: undetermined
  KZ_CHECK(n.actors[main_actor].r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED && n.actors[main_actor].r.creator == KZ_NONE32);

  // the container reads back to the same layer; the replay takes the owners from it
  kz::Container cont;
  KZ_CHECK_OK(kz::Container::parse(r.container, &cont));
  KZ_CHECK_OK(cont.verify_content());
  kz::trace::NeutralRun back;
  KZ_CHECK_OK(kz::trace::read_neutral(cont, &back));
  KZ_CHECK_EQ(back.actors.size(), n.actors.size());
  KZ_CHECK_EQ(back.actors[t3].request_func, std::string("app_main"));
  KZ_CHECK_EQ(back.actors[t3].r.owner_id, 0x110001u);
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));
  kz::trace::RealTraceOptions opt;
  opt.consts["TABLE"] = 4;
  kz::eval::Trace t;
  kz::trace::RealTraceReport rep;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(cont, p, opt, &t, &rep));
  KZ_CHECK(rep.owner_section);
  KZ_CHECK_EQ(rep.owners_confirmed, 2u);
  KZ_CHECK_EQ(rep.owners_inferred, 0u);

  // the same dump with the version 1 file alone: no identity read, no join
  in.manifests.pop_back();
  kz::trace::KzrtConvertResult r1;
  KZ_CHECK_OK(kz::trace::neutral_from_kzrt(in, &r1));
  KZ_CHECK((r1.layer.run.r.flags & KZ_NRUN_FLAG_OWNERS_RECORDED) == 0);
  for (const kz::trace::NActor& a : r1.layer.actors) KZ_CHECK(a.r.creator == KZ_NONE32 && (a.r.flags & KZ_NACTOR_FLAG_OWNER_RECORDED) == 0 && a.r.join_flags == 0);
  KZ_CHECK(!has_note(r1.notes, "identity join"));
}

// The shared rule on its own: an owner request, an unreadable one, a child request with and without its creator's owner.
void test_rule() {
  using namespace kz::trace;
  CreationRequest rq;
  rq.owner_readable = true;
  rq.owner_id = 0x110004;
  OwnerDecision o;
  KZ_CHECK(owner_from_request(rq, KZ_OWNER_LEVEL_INFERRED, nullptr, &o) == OwnerSource::Request);
  KZ_CHECK(o.owner_key == 0x110004u && o.level == KZ_OWNER_LEVEL_INFERRED && o.evidence == KZ_OWNER_EVIDENCE_CREATION_REQUEST);
  rq.owner_readable = false;
  o = OwnerDecision();
  KZ_CHECK(owner_from_request(rq, KZ_OWNER_LEVEL_CONFIRMED, nullptr, &o) == OwnerSource::None && o.level == KZ_OWNER_LEVEL_UNDETERMINED && o.owner_key == KZ_NONE32);
  CreationRequest ch;
  ch.child = true;
  ch.nested_owner_request = true;
  OwnerDecision creator;
  creator.owner_key = 0x110005;
  creator.level = KZ_OWNER_LEVEL_INFERRED;
  o = OwnerDecision();
  KZ_CHECK(owner_from_request(ch, KZ_OWNER_LEVEL_CONFIRMED, &creator, &o) == OwnerSource::Creator);
  KZ_CHECK(o.owner_key == 0x110005u && o.level == KZ_OWNER_LEVEL_INFERRED);  // at most the creator's level
  KZ_CHECK((o.evidence & (KZ_OWNER_EVIDENCE_CREATOR_OWNER | KZ_OWNER_EVIDENCE_ANALYSED_TABLE_EXCLUDED)) == (KZ_OWNER_EVIDENCE_CREATOR_OWNER | KZ_OWNER_EVIDENCE_ANALYSED_TABLE_EXCLUDED));
  o = OwnerDecision();
  KZ_CHECK(owner_from_request(ch, KZ_OWNER_LEVEL_CONFIRMED, nullptr, &o) == OwnerSource::None && o.level == KZ_OWNER_LEVEL_UNDETERMINED);
}

}  // namespace

int main() {
  test_rule();
  const std::vector<uint8_t> dump = record();
  test_join(dump);
  return kztest::finish("test_kzrt_identity");
}

// The identity join of an instrumented run for a creation request that names its application only after the task
// exists (kz/trace/identity_rule.hpp, kzs-sites 3 `app-request`, `owner-publish`, `owner-check`, `request-return
// status=`; schema revision 6 NOWNER; research/cfs_procrace/plan/c5_execution_basis_20261028.md section 10.4 item 9).
// The runtime (kairos/runtime, POSIX adapter) writes a kzrt-dump in this process with real threads; the converter reads
// it with a version 2 site file and a version 3 identity file.  Tool tests: every name and value is a unit-test value,
// not a research condition and not a case (CONDITIONS_POLICY).
//
//   (a) one value published inside the request instance (linked to the request's record), the request returns with
//       success and the cross-check agrees: the owner is the value, `confirmed` (the tie's level), evidence
//       creation-request, announced-id, owner-at-publish; NOWNER names the request, publish, return and check events;
//   (b) the request reports failure: the owner is undetermined, the tie (creator, creator_event, spawn_return) stays;
//   (c) a publish without a link, and one recorded by another actor: not used, counted (unlinked);
//   (d) two different values in one instance: neither is used (conflicting);
//   (e) no publish inside the instance (one after the return, so linked to a closed request: unlinked): undetermined;
//   (f) the task's events are recorded before the publish (the creator waits for the task first): the owner is the same
//       and the join adds, removes and moves no event (the layer's events equal those of the conversion without
//       identity reads): an identity is not an order;
//   (g) a cross-check that differs withdraws the owner; a request that never returns names nothing;
//   - the same dump with a version 2 identity file (no app requests): no NOWNER section, no app-request counts in the
//     note; the container reads back with NOWNER; the replay takes the owners from the layer.

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
    "kzs-sites 2\n"
    "source unit rev=0000000000000000000000000000000000000000 ref=\"tests/cfs-free/test_kzrt_app_request.cpp\"\n"
    "module 1 name=test_kzrt_app_request source=unit\n"
    "site 1 label=app.before op=app phase=before tu=t.c func=create_app\n"
    "site 2 label=app.after op=app phase=after tu=t.c func=create_app\n"
    "value 2 0 role=status from=ret type=s32\n"
    "site 3 label=create.before op=create phase=before tu=t.c func=task_create\n"
    "site 4 label=create.after op=create phase=after tu=t.c func=task_create\n"
    "value 4 0 role=status from=ret type=s32\n"
    "value 4 1 role=task_id from=out0 type=u32\n"
    "site 5 label=create.id op=create phase=before tu=t.c func=create_impl\n"
    "value 5 0 role=task_id from=arg4 type=u32\n"
    "site 6 label=task.entry op=entry phase=before tu=t.c func=task_entry\n"
    "value 6 0 role=task_id from=arg0 type=u32\n"
    "site 7 label=app.out op=app phase=point tu=t.c func=create_app line=40 place=store\n"
    "value 7 0 role=app_id from=operand0 type=u32\n"
    "site 8 label=app.table op=pub phase=point tu=t.c func=create_app line=38 place=store\n"
    "value 8 0 role=app_id from=operand0 type=u32\n"
    "site 9 label=task.use op=use phase=before tu=t.c func=task_use\n"
    "bind 101 site=1 backend=hand module=1 at=\"t.c create_app\" status=bound\n"
    "bind 201 site=2 backend=hand module=1 at=\"t.c create_app\" status=bound\n"
    "bind 301 site=3 backend=hand module=1 at=\"t.c task_create\" status=bound\n"
    "bind 401 site=4 backend=hand module=1 at=\"t.c task_create\" status=bound\n"
    "bind 501 site=5 backend=hand module=1 at=\"t.c create_impl\" status=bound\n"
    "bind 601 site=6 backend=hand module=1 at=\"t.c task_entry\" status=bound\n"
    "bind 701 site=7 backend=hand module=1 at=\"t.c:40\" status=bound\n"
    "bind 801 site=8 backend=hand module=1 at=\"t.c:38\" status=bound\n"
    "bind 901 site=9 backend=hand module=1 at=\"t.c task_use\" status=bound\n";

const char* const kIdentity3 =
    "kzs-sites 3\n"
    "identity 1 decoder=app-request ref=\"unit test\"\n"
    "identity 2 decoder=request-return status=0 ok=0\n"
    "identity 3 decoder=spawn-create\n"
    "identity 4 decoder=spawn-return id=1 status=0 ok=0\n"
    "identity 5 decoder=spawn-id id=0\n"
    "identity 6 decoder=thread-entry id=0\n"
    "identity 7 decoder=owner-publish owner=0\n"
    "identity 8 decoder=owner-check owner=0\n";

// The same tie without the readers of version 3.
const char* const kIdentity2 =
    "kzs-sites 2\n"
    "identity 3 decoder=spawn-create\n"
    "identity 4 decoder=spawn-return id=1 status=0 ok=0\n"
    "identity 5 decoder=spawn-id id=0\n"
    "identity 6 decoder=thread-entry id=0\n";

int sink(void* ctx, const void* bytes, size_t n) {
  std::vector<uint8_t>* v = static_cast<std::vector<uint8_t>*>(ctx);
  v->insert(v->end(), static_cast<const uint8_t*>(bytes), static_cast<const uint8_t*>(bytes) + n);
  return 0;
}

struct Task {
  uint32_t id = 0;
  uint32_t foreign_link = 0;  // (c) a publish this task records with a link of the creator's record
};

void* task_main(void* arg) {
  const Task* t = static_cast<const Task*>(arg);
  kz_rt_obs(6, 601, 0, 0x1u, t->id, 0, 0, 0);
  kz_rt_obs(9, 901, 0, 0, 0, 0, 0, 0);  // a use of the task before its creator publishes
  if (t->foreign_link != 0) kz_rt_obs(7, 701, t->foreign_link, 0x1u, 0x8, 0, 0, 0);
  kz_rt_actor_exit();
  return nullptr;
}

// One app request on the main thread: request, spawn (id passed and returned), the task joined (its events come first),
// then what `body` records inside the instance, then the return with `status` (none if `returns` is false).
template <typename Body>
void app(uint32_t id, int32_t status, bool returns, Task* t, Body body) {
  const uint32_t q = kz_rt_obs(1, 101, 0, 0, 0, 0, 0, 0);
  const uint32_t c = kz_rt_obs(3, 301, 0, 0, 0, 0, 0, 0);
  kz_rt_obs(5, 501, 0, 0x1u, id, 0, 0, 0);
  t->id = id;
  pthread_t th;
  KZ_CHECK_EQ(pthread_create(&th, nullptr, task_main, t), 0);
  kz_rt_obs(4, 401, c, 0x3u, 0, id, 0, 0);
  KZ_CHECK_EQ(pthread_join(th, nullptr), 0);  // (f): the task's records precede the publish
  body(q);
  if (returns) kz_rt_obs(2, 201, q, 0x1u, static_cast<uint64_t>(static_cast<int64_t>(status)), 0, 0, 0);
}

std::vector<uint8_t> record() {
  kz_rt_config cfg;
  std::memset(&cfg, 0, sizeof cfg);
  cfg.abi = KZ_RT_ABI_VERSION;
  cfg.mode = KZ_RT_MODE_RECORD;
  cfg.run_id = 9;
  cfg.backend = "hand";
  KZ_CHECK_EQ(kz_rt_init(&cfg), KZ_RT_OK);
  KZ_CHECK_EQ(kz_rt_attach(kz_rt_posix_adapter()), KZ_RT_OK);
  static Task t[9];
  // (a) application 5: the check (a table store) and the publish (the out parameter) agree, success
  app(0x20001, 0, true, &t[0], [](uint32_t q) {
    kz_rt_obs(8, 801, q, 0x1u, 5, 0, 0, 0);
    kz_rt_obs(7, 701, q, 0x1u, 5, 0, 0, 0);
  });
  // (b) application 6: published, the request reports failure
  app(0x20002, -1, true, &t[1], [](uint32_t q) { kz_rt_obs(7, 701, q, 0x1u, 6, 0, 0, 0); });
  // (c) application 7: the publish has no link; application 8: the task records it, with the creator's aseq as link
  app(0x20003, 0, true, &t[2], [](uint32_t) { kz_rt_obs(7, 701, 0, 0x1u, 7, 0, 0, 0); });
  t[3].foreign_link = 0;
  {
    const uint32_t q = kz_rt_obs(1, 101, 0, 0, 0, 0, 0, 0);
    const uint32_t c = kz_rt_obs(3, 301, 0, 0, 0, 0, 0, 0);
    kz_rt_obs(5, 501, 0, 0x1u, 0x20004, 0, 0, 0);
    t[3].id = 0x20004;
    t[3].foreign_link = q;
    pthread_t th;
    KZ_CHECK_EQ(pthread_create(&th, nullptr, task_main, &t[3]), 0);
    kz_rt_obs(4, 401, c, 0x3u, 0, 0x20004, 0, 0);
    KZ_CHECK_EQ(pthread_join(th, nullptr), 0);
    kz_rt_obs(2, 201, q, 0x1u, 0, 0, 0, 0);
  }
  // (d) two values in one instance
  app(0x20005, 0, true, &t[4], [](uint32_t q) {
    kz_rt_obs(7, 701, q, 0x1u, 9, 0, 0, 0);
    kz_rt_obs(7, 701, q, 0x1u, 10, 0, 0, 0);
  });
  // (e) nothing published inside the instance; a publish after the return (the request is closed)
  uint32_t closed = 0;
  app(0x20006, 0, true, &t[5], [&closed](uint32_t q) { closed = q; });
  kz_rt_obs(7, 701, closed, 0x1u, 11, 0, 0, 0);
  // (g) the check differs; then a request that never returns
  app(0x20007, 0, true, &t[6], [](uint32_t q) {
    kz_rt_obs(8, 801, q, 0x1u, 12, 0, 0, 0);
    kz_rt_obs(7, 701, q, 0x1u, 13, 0, 0, 0);
  });
  app(0x20008, 0, false, &t[7], [](uint32_t q) { kz_rt_obs(7, 701, q, 0x1u, 14, 0, 0, 0); });
  kz_rt_stop();
  std::vector<uint8_t> dump;
  KZ_CHECK_EQ(kz_rt_dump(sink, &dump), KZ_RT_OK);
  return dump;
}

uint32_t announcer(const kz::trace::NeutralRun& n, uint32_t id) {
  uint32_t found = KZ_NONE32, count = 0;
  for (size_t i = 0; i < n.actors.size(); ++i)
    if ((n.actors[i].r.join_flags & KZ_THREAD_FLAG_ENTRY_SEEN) != 0 && n.actors[i].r.announced_id == id) found = static_cast<uint32_t>(i), ++count;
  return count == 1 ? found : KZ_NONE32;
}

const kz_nowner* nowner(const kz::trace::NeutralRun& n, uint32_t actor) {
  for (const kz_nowner& o : n.owners)
    if (o.actor == actor) return &o;
  return nullptr;
}

bool has_note(const std::vector<std::string>& notes, const std::string& part) {
  for (const std::string& s : notes)
    if (s.find(part) != std::string::npos) return true;
  return false;
}

std::string label(const kz::trace::NeutralRun& n, uint32_t event) { return event < n.events.size() ? n.sites[n.events[event].site].label : "-"; }

void test_join(const std::vector<uint8_t>& dump) {
  kz::trace::KzrtConvertInput in;
  in.dump = dump;
  in.dump_name = "unit.kzrt";
  in.manifests.push_back({"sites.kzs", kSites});
  in.manifests.push_back({"identity3.kzs", kIdentity3});
  in.tool_sha256 = kz::sha256(std::string("unit-test tool"));
  kz::trace::KzrtConvertResult r;
  KZ_CHECK_OK(kz::trace::convert_kzrt_container(in, &r));
  for (const std::string& note : r.notes) std::printf("note %s\n", note.c_str());
  const kz::trace::NeutralRun& n = r.layer;
  KZ_CHECK_EQ(n.sites[0].r.decoder, KZ_DECODER_APP_REQUEST);
  KZ_CHECK_EQ(n.sites[6].r.decoder, KZ_DECODER_OWNER_PUBLISH);
  KZ_CHECK_EQ(n.sites[7].r.decoder, KZ_DECODER_OWNER_CHECK);
  KZ_CHECK(has_note(r.notes, "tied by the id 8,"));
  KZ_CHECK(has_note(r.notes, "app requests 8, not success 1"));
  KZ_CHECK(has_note(r.notes, "owner values published 9, unlinked 3, unreadable 0, conflicting 1"));
  KZ_CHECK(has_note(r.notes, "owner checks 2, unlinked 0, unreadable 0, differing 1"));
  KZ_CHECK(has_note(r.notes, "owners named at publish 1, withheld 7"));
  KZ_CHECK_EQ(n.owners.size(), static_cast<size_t>(8));

  // (a) and (f)
  const uint32_t ta = announcer(n, 0x20001);
  KZ_CHECK(ta != KZ_NONE32);
  const kz::trace::NActor& a = n.actors[ta];
  KZ_CHECK((a.r.join_flags & (KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_ID_MATCHED)) == (KZ_THREAD_FLAG_SPAWN_MATCHED | KZ_THREAD_FLAG_ID_MATCHED));
  KZ_CHECK_EQ(a.r.owner_level, KZ_OWNER_LEVEL_CONFIRMED);
  KZ_CHECK_EQ(a.r.owner_id, 5u);
  KZ_CHECK_EQ(a.r.owner_evidence, static_cast<uint32_t>(KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_ANNOUNCED_ID | KZ_OWNER_EVIDENCE_OWNER_AT_PUBLISH));
  KZ_CHECK_EQ(label(n, a.r.owner_request), std::string("app.before"));
  KZ_CHECK_EQ(label(n, a.r.creator_event), std::string("create.before"));
  KZ_CHECK_EQ(label(n, a.r.spawn_return), std::string("create.after"));
  const kz_nowner* oa = nowner(n, ta);
  KZ_CHECK(oa != nullptr);
  KZ_CHECK_EQ(oa->request, a.r.owner_request);
  KZ_CHECK_EQ(label(n, oa->publish), std::string("app.out"));
  KZ_CHECK_EQ(label(n, oa->request_return), std::string("app.after"));
  KZ_CHECK_EQ(label(n, oa->check), std::string("app.table"));
  KZ_CHECK_EQ(oa->owner_id, 5u);
  KZ_CHECK_EQ(oa->level, KZ_OWNER_LEVEL_CONFIRMED);
  KZ_CHECK_EQ(oa->flags, static_cast<uint32_t>(KZ_NOWNER_FLAG_PUBLISHED | KZ_NOWNER_FLAG_RETURNED | KZ_NOWNER_FLAG_STATUS_OK | KZ_NOWNER_FLAG_CHECK_AGREED));
  // (f) the task's events come before the publish, and are the task's own
  KZ_CHECK(a.r.first_event < oa->publish);
  for (uint32_t i = 0; i < n.events.size(); ++i)
    if (n.events[i].actor == ta) KZ_CHECK(i < oa->publish);

  // (b) failure: the tie stays, the owner is undetermined
  const uint32_t tb = announcer(n, 0x20002);
  KZ_CHECK(tb != KZ_NONE32);
  const kz::trace::NActor& b = n.actors[tb];
  KZ_CHECK(b.r.creator != KZ_NONE32 && b.r.creator_event != KZ_NONE32 && b.r.spawn_return != KZ_NONE32);
  KZ_CHECK(b.r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED && b.r.owner_id == KZ_NONE32);
  KZ_CHECK((b.r.owner_evidence & KZ_OWNER_EVIDENCE_OWNER_AT_PUBLISH) != 0);
  const kz_nowner* ob = nowner(n, tb);
  KZ_CHECK(ob != nullptr && ob->flags == static_cast<uint32_t>(KZ_NOWNER_FLAG_PUBLISHED | KZ_NOWNER_FLAG_RETURNED | KZ_NOWNER_FLAG_STATUS_NOT_OK));
  KZ_CHECK(ob->owner_id == 6u && ob->level == KZ_OWNER_LEVEL_UNDETERMINED);

  // (c) unlinked publishes: not used
  for (uint32_t id : {0x20003u, 0x20004u}) {
    const uint32_t tc = announcer(n, id);
    KZ_CHECK(tc != KZ_NONE32);
    KZ_CHECK(n.actors[tc].r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED && n.actors[tc].r.creator != KZ_NONE32);
    const kz_nowner* oc = nowner(n, tc);
    KZ_CHECK(oc != nullptr && oc->publish == KZ_NONE32 && oc->owner_id == KZ_NONE32);
    KZ_CHECK_EQ(oc->flags, static_cast<uint32_t>(KZ_NOWNER_FLAG_RETURNED | KZ_NOWNER_FLAG_STATUS_OK));
  }
  // (d) two values
  const kz_nowner* od = nowner(n, announcer(n, 0x20005));
  KZ_CHECK(od != nullptr && od->owner_id == KZ_NONE32 && od->level == KZ_OWNER_LEVEL_UNDETERMINED);
  KZ_CHECK_EQ(od->flags, static_cast<uint32_t>(KZ_NOWNER_FLAG_PUBLISH_CONFLICT | KZ_NOWNER_FLAG_RETURNED | KZ_NOWNER_FLAG_STATUS_OK));
  // (e) nothing inside the instance
  const kz_nowner* oe = nowner(n, announcer(n, 0x20006));
  KZ_CHECK(oe != nullptr && oe->publish == KZ_NONE32 && oe->level == KZ_OWNER_LEVEL_UNDETERMINED);
  // (g) the check differs; no return
  const kz_nowner* og = nowner(n, announcer(n, 0x20007));
  KZ_CHECK(og != nullptr && og->level == KZ_OWNER_LEVEL_UNDETERMINED && og->owner_id == 13u);
  KZ_CHECK_EQ(og->flags, static_cast<uint32_t>(KZ_NOWNER_FLAG_PUBLISHED | KZ_NOWNER_FLAG_RETURNED | KZ_NOWNER_FLAG_STATUS_OK | KZ_NOWNER_FLAG_CHECK_DIFFERS));
  const kz_nowner* oh = nowner(n, announcer(n, 0x20008));
  KZ_CHECK(oh != nullptr && oh->request_return == KZ_NONE32 && oh->flags == static_cast<uint32_t>(KZ_NOWNER_FLAG_PUBLISHED));
  KZ_CHECK_EQ(oh->level, KZ_OWNER_LEVEL_UNDETERMINED);

  // (f) the join adds, removes and moves no event: the layer without identity reads has the same events
  kz::trace::KzrtConvertInput in0 = in;
  in0.manifests.pop_back();
  kz::trace::KzrtConvertResult r0;
  KZ_CHECK_OK(kz::trace::neutral_from_kzrt(in0, &r0));
  KZ_CHECK_EQ(r0.layer.events.size(), n.events.size());
  bool same = r0.layer.events.size() == n.events.size();
  for (size_t i = 0; same && i < n.events.size(); ++i)
    same = std::memcmp(&r0.layer.events[i], &n.events[i], sizeof(kz_nevent)) == 0;
  KZ_CHECK(same);
  KZ_CHECK(r0.layer.owners.empty());

  // the container: NOWNER reads back; the replay takes the owners from the layer
  kz::Container cont;
  KZ_CHECK_OK(kz::Container::parse(r.container, &cont));
  KZ_CHECK_OK(cont.verify_content());
  KZ_CHECK(cont.find_section(KZ_SEC_NOWNER) != nullptr);
  kz::trace::NeutralRun back;
  KZ_CHECK_OK(kz::trace::read_neutral(cont, &back));
  KZ_CHECK_EQ(back.owners.size(), n.owners.size());
  KZ_CHECK(std::memcmp(back.owners.data(), n.owners.data(), n.owners.size() * sizeof(kz_nowner)) == 0);
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));
  kz::trace::RealTraceOptions opt;
  opt.consts["TABLE"] = 4;
  kz::eval::Trace t;
  kz::trace::RealTraceReport rep;
  KZ_CHECK_OK(kz::trace::real_trace_from_run(cont, p, opt, &t, &rep));
  KZ_CHECK(rep.owner_section);
  KZ_CHECK_EQ(rep.owners_confirmed, 1u);

  // version 2 identity reads only (no app request): no NOWNER, the note as before
  kz::trace::KzrtConvertInput in2 = in0;
  in2.manifests.push_back({"identity2.kzs", kIdentity2});
  kz::trace::KzrtConvertResult r2;
  KZ_CHECK_OK(kz::trace::convert_kzrt_container(in2, &r2));
  KZ_CHECK(r2.layer.owners.empty());
  KZ_CHECK(!has_note(r2.notes, "app requests"));
  KZ_CHECK(has_note(r2.notes, "tied by the id 8,"));
  kz::Container c2;
  KZ_CHECK_OK(kz::Container::parse(r2.container, &c2));
  KZ_CHECK(c2.find_section(KZ_SEC_NOWNER) == nullptr);
  for (const kz::trace::NActor& x : r2.layer.actors) KZ_CHECK(x.r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED);
}

// The shared rule on its own: an app request names its value at the tie's level, or nothing.
void test_rule() {
  using namespace kz::trace;
  CreationRequest rq;
  rq.app = true;
  rq.owner_readable = true;
  rq.owner_id = 0x31;
  OwnerDecision o;
  KZ_CHECK(owner_from_request(rq, KZ_OWNER_LEVEL_CONFIRMED, nullptr, &o) == OwnerSource::Request);
  KZ_CHECK(o.owner_key == 0x31u && o.level == KZ_OWNER_LEVEL_CONFIRMED);
  KZ_CHECK_EQ(o.evidence, static_cast<uint32_t>(KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_OWNER_AT_PUBLISH));
  rq.owner_readable = false;
  o = OwnerDecision();
  KZ_CHECK(owner_from_request(rq, KZ_OWNER_LEVEL_CONFIRMED, nullptr, &o) == OwnerSource::None && o.level == KZ_OWNER_LEVEL_UNDETERMINED);
}

}  // namespace

int main() {
  test_rule();
  const std::vector<uint8_t> dump = record();
  test_join(dump);
  return kztest::finish("test_kzrt_app_request");
}

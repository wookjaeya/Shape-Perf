/* test_rt_bootstrap -- recording before kz_rt_init and kz_rt_attach: static bootstrap buffer,
 * provisional actor ids per thread, the link of a thread's provisional id to its run actor, a new
 * thread after attach without a link, and the preinit loss when the bootstrap buffer is full. */
#include "rt_test.h"

#include <sys/syscall.h>
#include <unistd.h>

#include "kz_rt_posix.h"

#define BOOT_FLOOD 3000u

static void *boot_worker(void *arg) {
  uint32_t i;
  (void)arg;
  for (i = 0; i < 5; ++i) CHECK(kz_rt_obs(101, 1011, 0, 1u, i, 0, 0, 0) == i + 1u);
  return NULL;
}

static uint64_t g_run_worker_tid;
static void *run_worker(void *arg) {
  uint32_t i;
  (void)arg;
  g_run_worker_tid = (uint64_t)syscall(SYS_gettid);
  for (i = 0; i < 3; ++i) CHECK(kz_rt_obs(105, 1051, 0, 1u, i, 0, 0, 0) == i + 1u);
  return NULL;
}

int main(void) {
  pthread_t t;
  uint32_t i, a;
  kz_rt_config cfg;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  size_t boot_events = 0, main_run_events = 0, j;
  uint32_t main_prov = 0, worker_prov = 0, main_actor = 0, worker_actor = 0, max_boot_aseq = 0;
  const kz_rt_dec_actor *ma, *wa;

  /* 1. before any setup call: the static default records into the bootstrap buffer */
  for (i = 0; i < 10; ++i) CHECK(kz_rt_obs(100, 1001, 0, 3u, i, 0xAA, 0, 0) == i + 1u);
  CHECK(pthread_create(&t, NULL, boot_worker, NULL) == 0);
  CHECK(pthread_join(t, NULL) == 0);
  /* 2. init (before attach): still the bootstrap phase */
  cfg = test_config(KZ_RT_MODE_RECORD);
  cfg.run_id = 7;
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_init(&cfg) == KZ_RT_ESTATE);
  CHECK(kz_rt_obs(102, 1021, 0, 1u, 11, 0, 0, 0) == 11u);
  /* 3. fill the bootstrap buffer: the events that do not fit are counted, not waited for */
  for (i = 0; i < BOOT_FLOOD; ++i) (void)kz_rt_obs(104, 1041, 0, 1u, i, 0, 0, 0);
  /* 4. attach: the same thread continues under a run actor, its sequence continues */
  CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
  a = kz_rt_obs(103, 1031, 0, 1u, 12, 0, 0, 0);
  CHECK(a == 11u + BOOT_FLOOD + 1u);
  CHECK(pthread_create(&t, NULL, run_worker, NULL) == 0);
  CHECK(pthread_join(t, NULL) == 0);

  dump_decode(&mb, &it, &st);
  for (j = 0; j < it.n; ++j) {
    const kz_rt_dec_item *x = &it.v[j];
    if (x->type != KZ_RT_REC_EVENT) continue;
    if (x->u.event.owner == 0) {
      ++boot_events;
      CHECK((x->u.event.flags & KZ_RT_EF_BOOT) != 0);
      CHECK((x->u.event.flags & KZ_RT_EF_CLOCK) == 0); /* no clock before attach */
      CHECK((x->u.event.actor & KZ_RT_ACTOR_PROVISIONAL) != 0);
      CHECK(x->u.event.actor != KZ_RT_ACTOR_UNRESOLVED);
      if (x->u.event.site == 100) main_prov = x->u.event.actor;
      if (x->u.event.site == 101) worker_prov = x->u.event.actor;
      if (x->u.event.actor == main_prov && x->u.event.aseq > max_boot_aseq) max_boot_aseq = x->u.event.aseq;
    } else {
      CHECK((x->u.event.flags & (KZ_RT_EF_BOOT | KZ_RT_EF_UNRESOLVED)) == 0);
      CHECK((x->u.event.flags & KZ_RT_EF_CLOCK) != 0);
      if (x->u.event.site == 103) {
        main_actor = x->u.event.actor;
        ++main_run_events;
        CHECK(x->u.event.aseq == a);
      }
      if (x->u.event.site == 105) worker_actor = x->u.event.actor;
    }
  }
  /* two threads before attach: two provisional ids */
  CHECK(main_prov != 0 && worker_prov != 0 && main_prov != worker_prov);
  CHECK(main_run_events == 1);
  CHECK(main_actor != 0 && (main_actor & KZ_RT_ACTOR_PROVISIONAL) == 0);
  CHECK(worker_actor != 0 && worker_actor != main_actor);
  /* the link: same cell, the run actor's own range starts after the provisional records */
  ma = find_actor(&it, main_actor);
  CHECK(ma != NULL && ma->boot_link == main_prov && ma->link_evidence == KZ_RT_LINK_SAME_CELL);
  CHECK(ma->first_aseq == a && ma->last_aseq == a);
  CHECK(ma->key_kind == KZ_RT_KEY_LINUX_TID && ma->native_key == (uint64_t)syscall(SYS_gettid));
  wa = find_actor(&it, worker_actor);
  CHECK(wa != NULL && wa->boot_link == 0 && wa->link_evidence == KZ_RT_LINK_NONE && wa->first_aseq == 1 && wa->last_aseq == 3);
  CHECK(wa->native_key == g_run_worker_tid);
  /* the provisional worker (exited before attach) has no run actor: it stays provisional */
  CHECK(find_actor(&it, worker_prov) == NULL);
  /* bootstrap loss: everything attempted before attach is either in the buffer or counted */
  CHECK(loss_count(&it, KZ_RT_LOSS_PREINIT, 0) > 0);
  CHECK(boot_events + loss_count(&it, KZ_RT_LOSS_PREINIT, 0) == 10u + 5u + 1u + BOOT_FLOOD);
  CHECK(loss_count(&it, KZ_RT_LOSS_UNRESOLVED, 0) == 0);
  CHECK(max_boot_aseq <= 11u + BOOT_FLOOD);
  printf("bootstrap: %zu boot events, preinit loss %llu, provisional P%u -> actor %u\n", boot_events,
         (unsigned long long)loss_count(&it, KZ_RT_LOSS_PREINIT, 0), main_prov & ~KZ_RT_ACTOR_PROVISIONAL, main_actor);
  free(mb.p);
  free(it.v);
  return 0;
}

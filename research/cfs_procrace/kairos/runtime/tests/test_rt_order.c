/* test_rt_order -- per-actor buffers and per-actor order: several threads record at the same time,
 * each actor's records are in its own buffer with consecutive actor-local sequence numbers in
 * program order; there is no global sequence (the order between actors is not recorded). */
#include "rt_test.h"

#include "kz_rt_posix.h"

#define THREADS 4u
#define EVENTS 500u

static pthread_barrier_t g_bar;

static void *worker(void *arg) {
  const uint64_t me = (uint64_t)(uintptr_t)arg;
  uint32_t i, prev = 0;
  pthread_barrier_wait(&g_bar);
  for (i = 0; i < EVENTS; ++i) {
    const uint32_t a = kz_rt_obs(200 + (uint32_t)me, 2000 + (uint32_t)me, prev, 3u, me, i, 0, 0);
    CHECK(a == i + 1u);
    prev = a;
  }
  return NULL;
}

int main(void) {
  pthread_t t[THREADS];
  uint32_t i;
  kz_rt_config cfg = test_config(KZ_RT_MODE_RECORD);
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  size_t j, per_owner[KZ_RT_MAX_ACTORS + 1];
  uint32_t last_aseq[KZ_RT_MAX_ACTORS + 1], who[KZ_RT_MAX_ACTORS + 1];
  memset(per_owner, 0, sizeof per_owner);
  memset(last_aseq, 0, sizeof last_aseq);
  memset(who, 0xFF, sizeof who);
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
  CHECK(pthread_barrier_init(&g_bar, NULL, THREADS) == 0);
  for (i = 0; i < THREADS; ++i) CHECK(pthread_create(&t[i], NULL, worker, (void *)(uintptr_t)i) == 0);
  for (i = 0; i < THREADS; ++i) CHECK(pthread_join(t[i], NULL) == 0);

  dump_decode(&mb, &it, &st);
  CHECK(count_type(&it, KZ_RT_REC_ACTOR) == THREADS);
  for (j = 0; j < it.n; ++j) {
    const kz_rt_dec_event *e;
    if (it.v[j].type != KZ_RT_REC_EVENT) continue;
    e = &it.v[j].u.event;
    CHECK(e->owner == e->actor); /* in its own buffer */
    CHECK(e->actor >= 1 && e->actor <= KZ_RT_MAX_ACTORS);
    if (who[e->actor] == 0xFFFFFFFFu) who[e->actor] = (uint32_t)e->v[0];
    CHECK(e->v[0] == who[e->actor]);          /* one thread per actor */
    CHECK(e->site == 200 + who[e->actor]);
    CHECK(e->aseq == last_aseq[e->actor] + 1u); /* consecutive, in program order */
    CHECK(e->v[1] == e->aseq - 1u);
    CHECK(e->link == last_aseq[e->actor]);
    last_aseq[e->actor] = e->aseq;
    per_owner[e->owner]++;
  }
  for (i = 1; i <= THREADS; ++i) {
    const kz_rt_dec_actor *a = find_actor(&it, i);
    CHECK(per_owner[i] == EVENTS && last_aseq[i] == EVENTS);
    CHECK(a != NULL && a->first_aseq == 1 && a->last_aseq == EVENTS && a->recorded == EVENTS && a->overflow == 0);
  }
  CHECK(per_owner[0] == 0); /* nothing in the shared buffer */
  printf("order: %u actors x %u events, consecutive per actor\n", THREADS, EVENTS);
  free(mb.p);
  free(it.v);
  return 0;
}

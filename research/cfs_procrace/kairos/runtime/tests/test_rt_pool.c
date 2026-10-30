/* test_rt_pool -- the chunk pool of the run actors (kz_rt.h "static sizes", event_semantics.md 8.3):
 *   big      one actor keeps far more than the old 64 KiB per-actor buffer without loss; its BUF is its
 *            chunks' records in order, and the ACTOR record counts the chunks
 *   quota    a per-actor quota (run parameter) drops records with their bytes (overflow_quota, aseq gaps)
 *   pool     (built with a 4-chunk pool) an empty pool drops records of whichever actor needs a chunk
 *            (overflow_pool, RUNX pool_dropped), the other actors keep theirs
 *   dumps    four writers and a dumper at the same time: every dump is a gap-free prefix per actor
 * With a second argument the final dump is written to that file (for kz-rt-dump).  Sizes and counts
 * are unit-test values, not research conditions. */
#include "rt_test.h"

#include <stdint.h>

static uint64_t g_clock;
static kz_rt_cell *t_cell(void *ctx) {
  (void)ctx;
  return kz_rt_tls_cell();
}
static uint64_t t_now(void *ctx, uint32_t *clock_id) {
  (void)ctx;
  *clock_id = KZ_RT_CLOCK_OTHER;
  return __atomic_add_fetch(&g_clock, 1u, __ATOMIC_RELAXED);
}
static kz_rt_adapter g_ad;

static void start(uint32_t quota) {
  kz_rt_config cfg = test_config(KZ_RT_MODE_RECORD);
  memset(&g_ad, 0, sizeof g_ad);
  g_ad.abi = KZ_RT_ABI_VERSION;
  g_ad.caps = KZ_RT_CAP_CLOCK;
  g_ad.name = "pool-test";
  g_ad.self_cell = t_cell;
  g_ad.now_ns = t_now;
  cfg.actor_buf_bytes = quota;
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(&g_ad) == KZ_RT_OK);
}

static const kz_rt_dec_runx *find_runx(const items *s) {
  size_t i;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_RUNX) return &s->v[i].u.runx;
  return NULL;
}

/* The events of actor a are aseq first..first+n-1 in order (no gap); returns n. */
static uint32_t events_in_order(const items *s, uint32_t a, uint32_t first) {
  size_t i;
  uint32_t n = 0;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_EVENT && s->v[i].u.event.actor == a) {
      CHECK(s->v[i].u.event.aseq == first + n);
      CHECK(s->v[i].u.event.v[0] == first + n); /* the value is the writer's own count */
      ++n;
    }
  return n;
}

static void write_file(const char *path, const membuf *mb) {
  FILE *f;
  if (path == NULL) return;
  f = fopen(path, "wb");
  CHECK(f != NULL && fwrite(mb->p, 1, mb->n, f) == mb->n && fclose(f) == 0);
}

typedef struct writer {
  uint32_t site, n, last;
} writer;

static void *write_events(void *p) {
  writer *w = (writer *)p;
  uint32_t i;
  for (i = 1; i <= w->n; ++i) __atomic_store_n(&w->last, kz_rt_obs(w->site, w->site * 10u + 1u, 0, 0xFu, i, w->site, 0, 0), __ATOMIC_RELAXED);
  return NULL;
}

int main(int argc, char **argv) {
  const char *what = argc > 1 ? argv[1] : "big";
  const char *out = argc > 2 ? argv[2] : NULL;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_actor *a;
  const kz_rt_dec_runx *rx;
  uint32_t i;
  /* every record here: an event with 4 values = 56 + 32 = 88 bytes; 46 fit in a 4096-byte chunk */
  if (strcmp(what, "big") == 0) {
    const uint32_t n = 1000; /* 88000 bytes > 65536 */
    start(0);
    for (i = 1; i <= n; ++i) CHECK(kz_rt_obs(1, 11, 0, 0xFu, i, 0, 0, 0) == i);
    kz_rt_stop();
    dump_decode(&mb, &it, &st);
    a = find_actor(&it, 1);
    rx = find_runx(&it);
    CHECK(a != NULL && a->has_demand && a->recorded == n && a->overflow == 0 && a->buf_bytes == 88u * n && a->dropped_bytes == 0);
    CHECK(a->chunks == (n + 45u) / 46u && (KZ_RT_CHUNK_BYTES != 4096u || a->chunks == 22u));
    CHECK(events_in_order(&it, 1, 1) == n);
    CHECK(rx != NULL && rx->pool_bytes == KZ_RT_POOL_BYTES && rx->chunk_bytes == KZ_RT_CHUNK_BYTES && rx->chunks_used == a->chunks);
    CHECK(rx->window_mode == KZ_RT_MODE_RECORD && rx->mode_source == KZ_RT_MODE_SRC_INIT && rx->actor_quota == 0 && rx->pool_dropped == 0);
    printf("big: one actor kept %u bytes in %u chunks, no loss\n", a->buf_bytes, a->chunks);
  } else if (strcmp(what, "quota") == 0) {
    start(1000); /* 11 records of 88 bytes fit (968), the 12th does not */
    for (i = 1; i <= 30; ++i) CHECK(kz_rt_obs(1, 11, 0, 0xFu, i, 0, 0, 0) == i);
    kz_rt_stop();
    dump_decode(&mb, &it, &st);
    a = find_actor(&it, 1);
    rx = find_runx(&it);
    CHECK(a != NULL && a->recorded == 11 && a->buf_bytes == 968 && a->overflow == 19 && a->overflow_quota == 19 && a->overflow_pool == 0);
    CHECK(a->dropped_bytes == 19u * 88u && a->first_aseq == 1 && a->last_aseq == 30);
    CHECK(loss_count(&it, KZ_RT_LOSS_OVERFLOW, 1) == 19);
    CHECK(rx != NULL && rx->actor_quota == 1000 && rx->pool_dropped == 0);
    CHECK(events_in_order(&it, 1, 1) == 11);
    printf("quota: kept 11 (968 bytes), dropped 19 (%u bytes), counted as overflow_quota\n", a->dropped_bytes);
  } else if (strcmp(what, "pool") == 0) {
    writer w1 = {1, 0, 0}, w2 = {2, 0, 0};
    pthread_t t;
    CHECK(KZ_RT_POOL_BYTES / KZ_RT_CHUNK_BYTES == 4u); /* built with a 4-chunk pool */
    start(0);
    /* actor 1 takes 3 chunks (138 records), actor 2 the last one (46 records) and then finds the pool empty */
    w1.n = 138;
    CHECK(pthread_create(&t, NULL, write_events, &w1) == 0 && pthread_join(t, NULL) == 0);
    w2.n = 60;
    CHECK(pthread_create(&t, NULL, write_events, &w2) == 0 && pthread_join(t, NULL) == 0);
    kz_rt_stop();
    dump_decode(&mb, &it, &st);
    rx = find_runx(&it);
    a = find_actor(&it, 1);
    CHECK(a != NULL && a->recorded == 138 && a->overflow == 0 && a->chunks == 3);
    CHECK(events_in_order(&it, 1, 1) == 138);
    a = find_actor(&it, 2);
    CHECK(a != NULL && a->recorded == 46 && a->overflow == 14 && a->overflow_pool == 14 && a->overflow_quota == 0 && a->chunks == 1);
    CHECK(a->dropped_bytes == 14u * 88u && a->last_aseq == 60);
    CHECK(events_in_order(&it, 2, 1) == 46);
    CHECK(rx != NULL && rx->chunks_used == 4 && rx->pool_dropped == 14);
    printf("pool: 4 chunks taken, actor 2 dropped 14 records (%u bytes) for lack of a chunk\n", a->dropped_bytes);
  } else if (strcmp(what, "dumps") == 0) {
    writer w[4];
    pthread_t t[4];
    uint32_t k, rounds = 0;
    int running = 1;
    start(0);
    for (k = 0; k < 4; ++k) {
      w[k].site = k + 1;
      w[k].n = 3000;
      w[k].last = 0;
      CHECK(pthread_create(&t[k], NULL, write_events, &w[k]) == 0);
    }
    while (running) {
      /* a dump while the writers append: each actor's records are whole and gap-free from its first */
      size_t j;
      mb.n = 0;
      it.n = 0;
      CHECK(kz_rt_dump(mem_write, &mb) == KZ_RT_OK);
      CHECK(kz_rt_decode(mb.p, mb.n, collect, &it, &st) == KZ_RT_DEC_OK && st.crc_ok && st.unknown == 0);
      for (j = 0; j < it.n; ++j)
        if (it.v[j].type == KZ_RT_REC_ACTOR) {
          const kz_rt_dec_actor *x = &it.v[j].u.actor;
          const uint32_t got = events_in_order(&it, x->actor, x->first_aseq);
          CHECK(x->overflow == 0 && got <= 3000u);
        }
      ++rounds;
      running = 0;
      for (k = 0; k < 4; ++k) running |= __atomic_load_n(&w[k].last, __ATOMIC_RELAXED) != 3000u;
    }
    for (k = 0; k < 4; ++k) CHECK(pthread_join(t[k], NULL) == 0);
    kz_rt_stop();
    dump_decode(&mb, &it, &st);
    for (k = 1; k <= 4; ++k) {
      a = find_actor(&it, k);
      CHECK(a != NULL && a->recorded == 3000 && a->buf_bytes == 3000u * 88u);
      CHECK(events_in_order(&it, k, 1) == 3000);
    }
    printf("dumps: %u dumps during 4 x 3000 records, each a gap-free prefix per actor\n", rounds);
  } else {
    fprintf(stderr, "usage: test_rt_pool big|quota|pool|dumps [dump-file]\n");
    return 2;
  }
  write_file(out, &mb);
  free(mb.p);
  free(it.v);
  return 0;
}

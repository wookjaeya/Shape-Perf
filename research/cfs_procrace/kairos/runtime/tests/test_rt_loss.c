/* test_rt_loss -- loss and coverage counters by kind: overflow (with the sequence gap it leaves),
 * partial (a record being written when the dump reads), re-entry, noted excluded/unloaded code,
 * after-stop, and a truncated stream at the reader. */
#include "rt_test.h"

#include "kz_rt_posix.h"

/* A test adapter: the runtime's TLS cell and a clock whose next call can re-enter a hook or dump
 * (deterministic stand-ins for a signal handler inside a hook and a concurrent dumper). */
static int g_reenter, g_dump_inside;
static membuf g_inner;
static uint64_t g_clock;

static kz_rt_cell *t_cell(void *ctx) {
  (void)ctx;
  return kz_rt_tls_cell();
}
static uint64_t t_now(void *ctx, uint32_t *clock_id) {
  (void)ctx;
  *clock_id = KZ_RT_CLOCK_OTHER;
  if (g_reenter) {
    g_reenter = 0;
    CHECK(kz_rt_obs(301, 3011, 0, 0, 0, 0, 0, 0) == 0); /* nested: not recorded */
  }
  if (g_dump_inside) {
    g_dump_inside = 0;
    CHECK(kz_rt_dump(mem_write, &g_inner) == KZ_RT_OK);
  }
  return ++g_clock;
}

int main(void) {
  kz_rt_adapter ad;
  kz_rt_config cfg = test_config(KZ_RT_MODE_RECORD);
  membuf mb = {0};
  items it = {0}, inner = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_actor *a;
  uint32_t i, last = 0;
  size_t j, events = 0;
  memset(&ad, 0, sizeof ad);
  ad.abi = KZ_RT_ABI_VERSION;
  ad.caps = KZ_RT_CAP_CLOCK;
  ad.name = "loss-test";
  ad.self_cell = t_cell;
  ad.now_ns = t_now;
  cfg.actor_buf_bytes = 1024; /* unit-test capacity: 11 events of 4 values */
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(&ad) == KZ_RT_OK);

  for (i = 0; i < 100; ++i) last = kz_rt_obs(300, 3001, 0, 0xFu, i, 1, 2, 3);
  CHECK(last == 100);
  kz_rt_note_loss(KZ_RT_LOSS_EXCLUDED, 7, 3);
  kz_rt_note_loss(KZ_RT_LOSS_UNLOADED, 9, 1);
  kz_rt_note_loss(KZ_RT_LOSS_EXCLUDED, 7, 2);
  g_reenter = 1;
  CHECK(kz_rt_obs(300, 3001, 0, 0xFu, 100, 1, 2, 3) == 101);
  g_dump_inside = 1;
  CHECK(kz_rt_obs(300, 3001, 0, 0xFu, 101, 1, 2, 3) == 102);
  kz_rt_stop();
  CHECK(kz_rt_obs(300, 3001, 0, 0xFu, 102, 1, 2, 3) == 0);

  /* the dump taken while the record of aseq 102 was being written */
  CHECK(kz_rt_decode(g_inner.p, g_inner.n, collect, &inner, &st) == KZ_RT_DEC_OK);
  a = find_actor(&inner, 1);
  CHECK(a != NULL && a->partial == 1);
  CHECK(loss_count(&inner, KZ_RT_LOSS_PARTIAL, 1) == 1);

  dump_decode(&mb, &it, &st);
  CHECK(st.dump_flags == KZ_RT_DUMP_STOPPED);
  a = find_actor(&it, 1);
  CHECK(a != NULL && a->partial == 0 && a->first_aseq == 1 && a->last_aseq == 102);
  CHECK(a->recorded == 11 && a->overflow == 102 - 11);
  CHECK(a->recorded + a->overflow == a->last_aseq - a->first_aseq + 1);
  CHECK(loss_count(&it, KZ_RT_LOSS_OVERFLOW, 1) == 91);
  CHECK(loss_count(&it, KZ_RT_LOSS_PARTIAL, 1) == 0);
  CHECK(loss_count(&it, KZ_RT_LOSS_REENTRY, 0) == 1);
  CHECK(loss_count(&it, KZ_RT_LOSS_EXCLUDED, 0) == 5);
  CHECK(loss_count(&it, KZ_RT_LOSS_UNLOADED, 0) == 1);
  CHECK(loss_count(&it, KZ_RT_LOSS_AFTER_STOP, 0) == 1);
  for (j = 0; j < it.n; ++j)
    if (it.v[j].type == KZ_RT_REC_EVENT) {
      ++events;
      CHECK(it.v[j].u.event.aseq == events); /* the kept records are the first ones */
    }
  CHECK(events == 11);

  /* the reader on a cut stream: what it read is delivered, the cut is reported */
  {
    items cut = {0};
    kz_rt_dec_stats cs;
    CHECK(kz_rt_decode(mb.p, mb.n / 2, collect, &cut, &cs) == KZ_RT_DEC_ETRUNC);
    CHECK(cs.partial >= 1 && !cs.end_seen && cut.n > 0);
    free(cut.v);
  }
  printf("loss: recorded 11, overflow 91, partial seen by an inner dump, reentry 1, excluded 5, unloaded 1, after-stop 1\n");
  free(mb.p);
  free(it.v);
  free(inner.v);
  free(g_inner.p);
  return 0;
}

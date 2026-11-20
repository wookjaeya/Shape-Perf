/* test_rt_holdsem <scenario> -- kzp-hold 1 semantics in the runtime (event_semantics.md section 4),
 * the cases of tests/cfs-free/target/hold_{rearm,nth,defer}.kzh with two threads, plus the busy case.
 *   rearm  a thread resumed by a release keeps acting at its later hits (its release resumes another)
 *   nth    ordinals count qualifying hits of any thread (value condition); "n" acts only on the n-th
 *   defer  gates before releases at one site; the held hit's release acts when its thread is resumed
 *          (deferred), and an unless-released gate it reaches holding nothing is satisfied
 *   busy   an "each" gate that already holds a thread records busy and does not hold another
 *          (kzp-hold 1 leaves this case open; the F0 supervisor overwrites its held thread) */
#include "rt_test.h"

#include "kz_rt_posix.h"

enum { S_LOOKUP = 1, S_CREATE = 2, S_CREATED = 3, S_DONE = 4, S_FAIL = 5, S_STEP = 6 };

static void start(const kz_rt_point *pts, uint32_t n) {
  kz_rt_config cfg = test_config(KZ_RT_MODE_CONTROL);
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
  CHECK(kz_rt_plan(pts, n) == KZ_RT_OK);
}

/* One hit: observation (with a validated name in slot 0 when name != NULL, value v in slot 1) then control. */
static int hit(uint32_t site, const char *name, uint64_t v) {
  uint32_t ev;
  if (name != NULL)
    ev = kz_rt_obs_str(site, site * 100u, 0, 3u, (uint64_t)(uintptr_t)name, v, 0, 0, 0, 16);
  else
    ev = kz_rt_obs(site, site * 100u, 0, 2u, 0, v, 0, 0);
  return kz_rt_ctl(site, site * 100u, ev, KZ_RT_CTX_TASK);
}

static volatile int g_step;
static int g_r1, g_r2;

/* ---- rearm (hold_rearm.kzh) ---------------------------------------------------------------- */
static void *rearm_consumer(void *a) {
  (void)a;
  g_r1 = hit(S_LOOKUP, "OBJ0", 0); /* held by G_c, resumed by the provider's first create */
  wait_held(2);                    /* the provider is now held at its second create */
  g_r2 = hit(S_DONE, NULL, 0);     /* the resumed consumer's later hit acts: it resumes the provider */
  return NULL;
}
static int scenario_rearm(void) {
  kz_rt_point p[4];
  pthread_t c;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_ctl *f1, *f2;
  int r;
  p[0] = gate_point(S_LOOKUP, 1, 0);
  with_name(&p[0], 0, "OBJ0");
  p[1] = release_point(S_CREATE, 0, 1);
  p[2] = gate_point(S_CREATE, 2, 0);
  p[3] = release_point(S_DONE, 2, 1);
  start(p, 4);
  CHECK(pthread_create(&c, NULL, rearm_consumer, NULL) == 0);
  wait_held(0);
  CHECK(hit(S_CREATE, "OBJ0", 0) == KZ_RT_CTL_PASS);     /* create 1: releases the consumer */
  r = hit(S_CREATE, "OBJ1", 0);                          /* create 2: held until the consumer's done */
  CHECK(pthread_join(c, NULL) == 0);
  CHECK(r == KZ_RT_CTL_RESUMED && g_r1 == KZ_RT_CTL_RESUMED && g_r2 == KZ_RT_CTL_PASS);
  dump_decode(&mb, &it, &st);
  f1 = find_ctl(&it, KZ_RT_CW_FIRE, 0);
  f2 = find_ctl(&it, KZ_RT_CW_FIRE, 1);
  if (f1 != NULL && f2 != NULL && f1->ctl_seq > f2->ctl_seq) {
    const kz_rt_dec_ctl *x = f1;
    f1 = f2;
    f2 = x;
  }
  CHECK(f1 != NULL && f2 != NULL && f1->point == 1 && f2->point == 3);
  CHECK(f2->actor == f1->other && f2->other == f1->actor && f2->reason == KZ_RT_CR_RESUMED);
  free(mb.p);
  free(it.v);
  printf("rearm: the resumed consumer's later hit resumed the provider\n");
  return 0;
}

/* ---- nth (hold_nth.kzh) ---------------------------------------------------------------------- */
static void *nth_releaser(void *a) {
  int i;
  (void)a;
  wait_held(0);
  for (i = 1; i <= 3; ++i) {
    __atomic_store_n(&g_step, i, __ATOMIC_SEQ_CST);
    CHECK(hit(S_FAIL, NULL, 0) == KZ_RT_CTL_PASS);
  }
  return NULL;
}
static int scenario_nth(void) {
  kz_rt_point p[2];
  pthread_t t;
  kz_rt_point_state s0, s1;
  p[0] = gate_point(S_STEP, 2, 0);
  p[0].val_slot = 1;
  p[0].val = 7;
  p[1] = release_point(S_FAIL, 0, 3);
  start(p, 2);
  CHECK(pthread_create(&t, NULL, nth_releaser, NULL) == 0);
  CHECK(hit(S_STEP, NULL, 1) == KZ_RT_CTL_PASS); /* does not qualify (value 1) */
  CHECK(hit(S_STEP, NULL, 7) == KZ_RT_CTL_PASS); /* qualifying hit 1 */
  CHECK(hit(S_STEP, NULL, 7) == KZ_RT_CTL_RESUMED); /* qualifying hit 2: held until the 3rd failure */
  CHECK(__atomic_load_n(&g_step, __ATOMIC_SEQ_CST) == 3);
  CHECK(pthread_join(t, NULL) == 0);
  CHECK(hit(S_STEP, NULL, 7) == KZ_RT_CTL_PASS); /* hit 3: the point acted once */
  CHECK(kz_rt_ctl_state(0, &s0) == KZ_RT_OK && kz_rt_ctl_state(1, &s1) == KZ_RT_OK);
  CHECK(s0.hits == 3 && s0.holds == 1 && s1.hits == 3 && s1.acted == 1);
  printf("nth: gate occurrence 2 with a value condition, release occurrence 3\n");
  return 0;
}

/* ---- defer (hold_defer.kzh) -------------------------------------------------------------------- */
static void *defer_consumer(void *a) {
  (void)a;
  g_r1 = hit(S_LOOKUP, "OBJ0", 0); /* G_c holds it; R_same at this hit is deferred */
  return NULL;
}
static int scenario_defer(void) {
  kz_rt_point p[4];
  pthread_t c;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_ctl *f_rp, *f_same;
  size_t k;
  p[0] = gate_point(S_LOOKUP, 1, 0); /* G_c */
  with_name(&p[0], 0, "OBJ0");
  p[1] = release_point(S_CREATED, 0, 1); /* R_p */
  p[2] = gate_point(S_CREATE, 1, KZ_RT_PT_UNLESS_RELEASED); /* G_x */
  with_name(&p[2], 0, "OBJ1");
  p[3] = release_point(S_LOOKUP, 2, 1); /* R_same */
  with_name(&p[3], 0, "OBJ0");
  start(p, 4);
  CHECK(pthread_create(&c, NULL, defer_consumer, NULL) == 0);
  wait_held(0);
  CHECK(hit(S_CREATED, NULL, 0) == KZ_RT_CTL_PASS);   /* R_p resumes the consumer; its R_same acts now */
  CHECK(hit(S_CREATE, "OBJ1", 0) == KZ_RT_CTL_PASS);  /* G_x is satisfied: no hold */
  CHECK(pthread_join(c, NULL) == 0);
  CHECK(g_r1 == KZ_RT_CTL_RESUMED);
  dump_decode(&mb, &it, &st);
  f_rp = f_same = NULL;
  for (k = 0; find_ctl(&it, KZ_RT_CW_FIRE, k) != NULL; ++k) {
    const kz_rt_dec_ctl *f = find_ctl(&it, KZ_RT_CW_FIRE, k);
    if (f->point == 1) f_rp = f;
    if (f->point == 3) f_same = f;
  }
  CHECK(f_rp != NULL && f_same != NULL && f_rp->reason == KZ_RT_CR_RESUMED);
  CHECK(f_same->reason == (KZ_RT_CR_SATISFIED | KZ_RT_CR_DEFERRED) && f_same->subject == f_rp->other && f_same->actor == f_rp->actor);
  CHECK(f_same->ctl_seq == f_rp->ctl_seq + 1u); /* applied at the resume, before anything else */
  CHECK(find_ctl(&it, KZ_RT_CW_GATE_HOLD, 1) == NULL); /* only G_c held */
  free(mb.p);
  free(it.v);
  printf("defer: the held hit's release acted at the resume and satisfied the unless-released gate\n");
  return 0;
}

/* ---- busy -------------------------------------------------------------------------------------- */
static void *busy_first(void *a) {
  (void)a;
  g_r1 = hit(S_STEP, NULL, 0);
  return NULL;
}
static int scenario_busy(void) {
  kz_rt_point p[2];
  pthread_t t;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_ctl *b;
  p[0] = gate_point(S_STEP, 0, 0); /* each */
  p[1] = release_point(S_DONE, 0, 0);
  start(p, 2);
  CHECK(pthread_create(&t, NULL, busy_first, NULL) == 0);
  wait_held(0);
  CHECK(hit(S_STEP, NULL, 0) == KZ_RT_CTL_PASS); /* the gate already holds the other thread */
  CHECK(hit(S_DONE, NULL, 0) == KZ_RT_CTL_PASS);
  CHECK(pthread_join(t, NULL) == 0);
  CHECK(g_r1 == KZ_RT_CTL_RESUMED);
  dump_decode(&mb, &it, &st);
  b = find_ctl(&it, KZ_RT_CW_GATE_BUSY, 0);
  CHECK(b != NULL && b->other != 0 && b->other != b->actor);
  free(mb.p);
  free(it.v);
  printf("busy: recorded, not held\n");
  return 0;
}

int main(int argc, char **argv) {
  const char *s = argc > 1 ? argv[1] : "";
  if (strcmp(s, "rearm") == 0) return scenario_rearm();
  if (strcmp(s, "nth") == 0) return scenario_nth();
  if (strcmp(s, "defer") == 0) return scenario_defer();
  if (strcmp(s, "busy") == 0) return scenario_busy();
  fprintf(stderr, "usage: test_rt_holdsem rearm|nth|defer|busy\n");
  return 2;
}

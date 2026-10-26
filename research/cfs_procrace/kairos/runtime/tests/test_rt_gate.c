/* test_rt_gate <scenario> -- observation and control are separate hooks.
 *   separation  an observation hook at a gate site never blocks; the control hook holds until the
 *               release's hit; the realized-control records (hold, fire, resume) in ctl_seq order
 *   record      mode RECORD (same plan): the control hook returns at once, nothing is counted
 *   context     refused in a context that cannot hold (and the act is consumed); lock-held holds
 *               only at a point that allows it
 *   isr         an ISR-context control call is not evaluated at all (counted as ctl-skipped)
 *   nowait      an adapter without a wait primitive: gates are recorded no-ops (refused no-wait)
 *   timeout     a gate with a timeout resumes unreleased, records it, and its hit's releases act
 *   preattach   a plan loaded before attach (the runtime's own spin lock): a gate refuses before
 *               attach (recorded in the shared buffer) and holds after it
 * The POSIX adapter's gate is a real wait (pthread condition variable). */
#include "rt_test.h"

#include "kz_rt_posix.h"

enum { S_GATE = 10, S_REL = 20, S_OTHER = 30, S_LOCKNO = 40 };

typedef struct hit_arg {
  uint32_t site, bind, ctx;
  int result;
  volatile int obs_done, passed;
} hit_arg;

static void *hit_thread(void *p) {
  hit_arg *h = (hit_arg *)p;
  const uint32_t ev = kz_rt_obs(h->site, h->bind, 0, 1u, 1, 0, 0, 0);
  CHECK(ev != 0);
  __atomic_store_n(&h->obs_done, 1, __ATOMIC_SEQ_CST); /* the observation hook returned */
  h->result = kz_rt_ctl(h->site, h->bind, ev, h->ctx);
  __atomic_store_n(&h->passed, 1, __ATOMIC_SEQ_CST);
  return NULL;
}

static int hit(uint32_t site, uint32_t bind, uint32_t ctx) {
  const uint32_t ev = kz_rt_obs(site, bind, 0, 0, 0, 0, 0, 0);
  return kz_rt_ctl(site, bind, ev, ctx);
}

static void start(uint32_t mode, const kz_rt_point *pts, uint32_t n, const kz_rt_adapter *ad) {
  kz_rt_config cfg = test_config(mode);
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(ad) == KZ_RT_OK);
  CHECK(kz_rt_plan(pts, n) == KZ_RT_OK);
  CHECK(kz_rt_plan(pts, n) == KZ_RT_ESTATE);
}

static void finish(membuf *mb, items *it) {
  free(mb->p);
  free(it->v);
}

static int scenario_separation(void) {
  kz_rt_point p[2];
  pthread_t t;
  hit_arg h = {S_GATE, 1010, KZ_RT_CTX_TASK, 0, 0, 0};
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_ctl *hold, *fire, *resume;
  kz_rt_point_state ps;
  p[0] = gate_point(S_GATE, 1, 0);
  p[1] = release_point(S_REL, 0, 1);
  start(KZ_RT_MODE_CONTROL, p, 2, kz_rt_posix_adapter());
  CHECK(pthread_create(&t, NULL, hit_thread, &h) == 0);
  wait_held(0);
  CHECK(__atomic_load_n(&h.obs_done, __ATOMIC_SEQ_CST) == 1);
  /* other hooks run while a thread is held, including an observation at the gate site */
  CHECK(kz_rt_obs(S_GATE, 1011, 0, 1u, 2, 0, 0, 0) != 0);
  CHECK(hit(S_OTHER, 1030, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
  sleep_ms(20);
  CHECK(__atomic_load_n(&h.passed, __ATOMIC_SEQ_CST) == 0);
  CHECK(hit(S_REL, 1020, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
  CHECK(pthread_join(t, NULL) == 0);
  CHECK(h.result == KZ_RT_CTL_RESUMED);
  CHECK(kz_rt_ctl_state(0, &ps) == KZ_RT_OK && ps.hits == 1 && ps.acted == 1 && ps.held_actor == 0 && ps.holds == 1);
  dump_decode(&mb, &it, &st);
  hold = find_ctl(&it, KZ_RT_CW_GATE_HOLD, 0);
  fire = find_ctl(&it, KZ_RT_CW_FIRE, 0);
  resume = find_ctl(&it, KZ_RT_CW_RESUME, 0);
  CHECK(hold != NULL && fire != NULL && resume != NULL);
  CHECK(hold->actor != fire->actor && fire->other == hold->actor && fire->reason == KZ_RT_CR_RESUMED && resume->other == fire->actor);
  CHECK(hold->ctl_seq < fire->ctl_seq && fire->ctl_seq < resume->ctl_seq);
  CHECK(hold->ctx == KZ_RT_CTX_TASK && hold->hit == 1 && hold->point == 0 && fire->point == 1 && fire->gate == 0);
  CHECK(hold->aseq == hold->ev + 1u && resume->aseq == hold->aseq + 1u); /* in the held actor's own order */
  finish(&mb, &it);
  printf("separation: obs at the gate site returned, ctl held until the release, hold < fire < resume\n");
  return 0;
}

static int scenario_record(void) {
  kz_rt_point p[2];
  kz_rt_point_state ps;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  p[0] = gate_point(S_GATE, 1, 0);
  p[1] = release_point(S_REL, 0, 1);
  start(KZ_RT_MODE_RECORD, p, 2, kz_rt_posix_adapter());
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
  CHECK(kz_rt_ctl_state(0, &ps) == KZ_RT_OK && ps.hits == 0);
  dump_decode(&mb, &it, &st);
  CHECK(count_type(&it, KZ_RT_REC_CTL) == 0 && count_type(&it, KZ_RT_REC_EVENT) == 1 && count_type(&it, KZ_RT_REC_POINT) == 2);
  finish(&mb, &it);
  printf("record: control hook inactive in record mode\n");
  return 0;
}

static int scenario_context(void) {
  kz_rt_point p[4];
  pthread_t t;
  hit_arg h = {S_OTHER, 1030, KZ_RT_CTX_LOCK_HELD, 0, 0, 0};
  kz_rt_point_state ps;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_ctl *r0, *r1;
  p[0] = gate_point(S_GATE, 1, 0);
  p[1] = gate_point(S_OTHER, 1, KZ_RT_PT_ALLOW_LOCK_HELD);
  p[2] = release_point(S_REL, 1, 1);
  p[3] = gate_point(S_LOCKNO, 1, 0);
  start(KZ_RT_MODE_CONTROL, p, 4, kz_rt_posix_adapter());
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_UNSUPPORTED) == KZ_RT_CTL_REFUSED);
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS); /* "first" was consumed by the refusal */
  CHECK(kz_rt_ctl_state(0, &ps) == KZ_RT_OK && ps.hits == 2 && ps.acted == 1 && ps.holds == 0);
  CHECK(hit(S_LOCKNO, 1040, KZ_RT_CTX_LOCK_HELD) == KZ_RT_CTL_REFUSED);
  CHECK(pthread_create(&t, NULL, hit_thread, &h) == 0);
  wait_held(1);
  CHECK(hit(S_REL, 1020, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
  CHECK(pthread_join(t, NULL) == 0);
  CHECK(h.result == KZ_RT_CTL_RESUMED);
  dump_decode(&mb, &it, &st);
  r0 = find_ctl(&it, KZ_RT_CW_GATE_REFUSED, 0);
  r1 = find_ctl(&it, KZ_RT_CW_GATE_REFUSED, 1);
  CHECK(r0 != NULL && r0->reason == KZ_RT_CR_CONTEXT && r0->ctx == KZ_RT_CTX_UNSUPPORTED && r0->point == 0);
  CHECK(r1 != NULL && r1->reason == KZ_RT_CR_CONTEXT && r1->ctx == KZ_RT_CTX_LOCK_HELD && r1->point == 3);
  CHECK(find_ctl(&it, KZ_RT_CW_GATE_HOLD, 0) != NULL && find_ctl(&it, KZ_RT_CW_GATE_HOLD, 0)->ctx == KZ_RT_CTX_LOCK_HELD);
  finish(&mb, &it);
  printf("context: unsupported and lock-held refused (consumed, recorded); lock-held held where allowed\n");
  return 0;
}

static int scenario_isr(void) {
  kz_rt_point p[1];
  kz_rt_point_state ps;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  p[0] = gate_point(S_GATE, 1, 0);
  start(KZ_RT_MODE_CONTROL, p, 1, kz_rt_posix_adapter());
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_ISR) == KZ_RT_CTL_PASS);
  CHECK(kz_rt_ctl_state(0, &ps) == KZ_RT_OK && ps.hits == 0);
  dump_decode(&mb, &it, &st);
  CHECK(count_type(&it, KZ_RT_REC_CTL) == 0 && loss_count(&it, KZ_RT_LOSS_CTL_SKIPPED, 0) == 1);
  finish(&mb, &it);
  printf("isr: not evaluated, counted ctl-skipped\n");
  return 0;
}

static int scenario_nowait(void) {
  kz_rt_point p[1];
  kz_rt_adapter ad = *kz_rt_posix_adapter();
  kz_rt_caps caps;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_ctl *r;
  ad.caps &= ~(uint32_t)(KZ_RT_CAP_WAIT | KZ_RT_CAP_WAIT_TIMEOUT);
  ad.hold_wait = NULL;
  ad.hold_wake = NULL;
  ad.name = "posix-nowait";
  p[0] = gate_point(S_GATE, 0, 0);
  start(KZ_RT_MODE_CONTROL, p, 1, &ad);
  kz_rt_caps_get(&caps);
  CHECK(caps.holdable_ctx_mask == 0);
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_TASK) == KZ_RT_CTL_REFUSED);
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_TASK) == KZ_RT_CTL_REFUSED); /* each: refused at every hit */
  dump_decode(&mb, &it, &st);
  r = find_ctl(&it, KZ_RT_CW_GATE_REFUSED, 1);
  CHECK(r != NULL && r->reason == KZ_RT_CR_NO_WAIT && r->hit == 2);
  finish(&mb, &it);
  printf("nowait: gate is a recorded no-op\n");
  return 0;
}

static int scenario_timeout(void) {
  kz_rt_point p[3];
  struct timespec a, b;
  long ms;
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_ctl *to, *fi;
  p[0] = gate_point(S_GATE, 1, 0);
  p[0].timeout_ns = 50000000u; /* unit-test value */
  p[1] = release_point(S_GATE, 2, 1);
  p[2] = gate_point(S_OTHER, 1, KZ_RT_PT_UNLESS_RELEASED);
  start(KZ_RT_MODE_CONTROL, p, 3, kz_rt_posix_adapter());
  clock_gettime(CLOCK_MONOTONIC, &a);
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_TASK) == KZ_RT_CTL_TIMEOUT);
  clock_gettime(CLOCK_MONOTONIC, &b);
  ms = (long)(b.tv_sec - a.tv_sec) * 1000L + (b.tv_nsec - a.tv_nsec) / 1000000L;
  CHECK(ms >= 49);
  CHECK(hit(S_OTHER, 1030, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS); /* satisfied by the deferred release */
  dump_decode(&mb, &it, &st);
  to = find_ctl(&it, KZ_RT_CW_TIMEOUT, 0);
  fi = find_ctl(&it, KZ_RT_CW_FIRE, 0);
  CHECK(to != NULL && fi != NULL && fi->ctl_seq > to->ctl_seq);
  CHECK(fi->reason == (KZ_RT_CR_SATISFIED | KZ_RT_CR_DEFERRED) && fi->gate == 2);
  finish(&mb, &it);
  printf("timeout: resumed unreleased after %ld ms, the hit's release acted then\n", ms);
  return 0;
}

static int scenario_preattach(void) {
  kz_rt_point p[3];
  kz_rt_config cfg = test_config(KZ_RT_MODE_CONTROL);
  pthread_t t;
  hit_arg h = {S_OTHER, 1030, KZ_RT_CTX_TASK, 0, 0, 0};
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  size_t j;
  const kz_rt_dec_ctl *r;
  int found = 0;
  p[0] = gate_point(S_GATE, 1, 0);
  p[1] = gate_point(S_OTHER, 1, 0);
  p[2] = release_point(S_REL, 1, 1);
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_plan(p, 3) == KZ_RT_OK);
  CHECK(hit(S_GATE, 1010, KZ_RT_CTX_TASK) == KZ_RT_CTL_REFUSED);
  CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
  CHECK(pthread_create(&t, NULL, hit_thread, &h) == 0);
  wait_held(1);
  CHECK(hit(S_REL, 1020, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
  CHECK(pthread_join(t, NULL) == 0);
  CHECK(h.result == KZ_RT_CTL_RESUMED);
  dump_decode(&mb, &it, &st);
  r = find_ctl(&it, KZ_RT_CW_GATE_REFUSED, 0);
  CHECK(r != NULL && r->reason == KZ_RT_CR_PRE_ATTACH && r->owner == 0 && (r->actor & KZ_RT_ACTOR_PROVISIONAL) != 0);
  for (j = 0; j < it.n; ++j)
    if (it.v[j].type == KZ_RT_REC_RUN) {
      CHECK((it.v[j].u.run.phase & 0x100u) == 0); /* the spin lock serves the control state */
      found = 1;
    }
  CHECK(found);
  finish(&mb, &it);
  printf("preattach: refused before attach (shared buffer), held after attach under the spin lock\n");
  return 0;
}

int main(int argc, char **argv) {
  const char *s = argc > 1 ? argv[1] : "";
  if (strcmp(s, "separation") == 0) return scenario_separation();
  if (strcmp(s, "record") == 0) return scenario_record();
  if (strcmp(s, "context") == 0) return scenario_context();
  if (strcmp(s, "isr") == 0) return scenario_isr();
  if (strcmp(s, "nowait") == 0) return scenario_nowait();
  if (strcmp(s, "timeout") == 0) return scenario_timeout();
  if (strcmp(s, "preattach") == 0) return scenario_preattach();
  fprintf(stderr, "usage: test_rt_gate separation|record|context|isr|nowait|timeout|preattach\n");
  return 2;
}

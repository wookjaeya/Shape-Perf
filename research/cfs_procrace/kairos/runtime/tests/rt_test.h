/* rt_test.h -- helpers of the runtime unit tests (each test is one process: the runtime is a
 * singleton with static state).  Values in the tests are unit-test values, not research conditions. */
#ifndef KZ_RT_TEST_H
#define KZ_RT_TEST_H

#define _GNU_SOURCE 1
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "kz_rt.h"
#include "kz_rt_decode.h"
#include "kz_rt_format.h"

#define CHECK(c)                                                                  \
  do {                                                                            \
    if (!(c)) {                                                                   \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c);       \
      exit(1);                                                                    \
    }                                                                             \
  } while (0)

typedef struct membuf {
  unsigned char *p;
  size_t n, cap;
} membuf;

static inline int mem_write(void *ctx, const void *b, size_t n) {
  membuf *m = (membuf *)ctx;
  if (m->n + n > m->cap) {
    size_t c = m->cap ? m->cap : 4096;
    unsigned char *q;
    while (c < m->n + n) c *= 2;
    q = (unsigned char *)realloc(m->p, c);
    if (q == NULL) return 1;
    m->p = q;
    m->cap = c;
  }
  memcpy(m->p + m->n, b, n);
  m->n += n;
  return 0;
}

typedef struct items {
  kz_rt_dec_item *v;
  size_t n, cap;
} items;

static inline int collect(void *ctx, const kz_rt_dec_item *it) {
  items *s = (items *)ctx;
  if (s->n == s->cap) {
    size_t c = s->cap ? 2 * s->cap : 256;
    kz_rt_dec_item *q = (kz_rt_dec_item *)realloc(s->v, c * sizeof *q);
    if (q == NULL) return 1;
    s->v = q;
    s->cap = c;
  }
  s->v[s->n++] = *it;
  return 0;
}

/* Dump the runtime into *mb (reset first) and decode it into *out (reset first). */
static inline void dump_decode(membuf *mb, items *out, kz_rt_dec_stats *st) {
  mb->n = 0;
  out->n = 0;
  CHECK(kz_rt_dump(mem_write, mb) == KZ_RT_OK);
  CHECK(kz_rt_decode(mb->p, mb->n, collect, out, st) == KZ_RT_DEC_OK);
  CHECK(st->end_seen && st->crc_ok);
  CHECK(st->partial == 0 && st->unknown == 0);
}

static inline const kz_rt_dec_actor *find_actor(const items *s, uint32_t actor) {
  size_t i;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_ACTOR && s->v[i].u.actor.actor == actor) return &s->v[i].u.actor;
  return NULL;
}

/* The count of a LOSS kind (actor 0 = global records; key ignored), summed. */
static inline uint64_t loss_count(const items *s, uint32_t kind, uint32_t actor) {
  size_t i;
  uint64_t c = 0;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_LOSS && s->v[i].u.loss.kind == kind && s->v[i].u.loss.actor == actor) c += s->v[i].u.loss.count;
  return c;
}

static inline size_t count_type(const items *s, uint32_t type) {
  size_t i, c = 0;
  for (i = 0; i < s->n; ++i) c += s->v[i].type == type;
  return c;
}

/* The k-th (0-based) CTL record with this what, or NULL. */
static inline const kz_rt_dec_ctl *find_ctl(const items *s, uint32_t what, size_t k) {
  size_t i;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_CTL && s->v[i].u.ctl.what == what && k-- == 0) return &s->v[i].u.ctl;
  return NULL;
}

static inline void sleep_ms(long ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;
  nanosleep(&ts, NULL);
}

/* Wait (up to 10 s) until gate point p holds a thread. */
static inline void wait_held(uint32_t p) {
  int i;
  for (i = 0; i < 10000; ++i) {
    kz_rt_point_state s;
    CHECK(kz_rt_ctl_state(p, &s) == KZ_RT_OK);
    if (s.held_actor != 0) return;
    sleep_ms(1);
  }
  fprintf(stderr, "gate %u never held a thread\n", p);
  exit(1);
}

static inline kz_rt_config test_config(uint32_t mode) {
  kz_rt_config c;
  memset(&c, 0, sizeof c);
  c.abi = KZ_RT_ABI_VERSION;
  c.mode = mode;
  c.backend = "unit-test";
  return c;
}

static inline kz_rt_point gate_point(uint32_t site, uint32_t occurrence, uint32_t flags) {
  kz_rt_point p;
  memset(&p, 0, sizeof p);
  p.kind = KZ_RT_POINT_GATE;
  p.site = site;
  p.occurrence = occurrence;
  p.flags = flags;
  p.str_slot = -1;
  p.val_slot = -1;
  return p;
}

static inline kz_rt_point release_point(uint32_t site, uint32_t releases, uint32_t occurrence) {
  kz_rt_point p = gate_point(site, occurrence, 0);
  p.kind = KZ_RT_POINT_RELEASE;
  p.releases = releases;
  return p;
}

static inline void with_name(kz_rt_point *p, uint32_t slot, const char *s) {
  p->str_slot = (int32_t)slot;
  p->str_len = (uint32_t)strlen(s);
  memcpy(p->str, s, p->str_len);
}

#endif /* KZ_RT_TEST_H */

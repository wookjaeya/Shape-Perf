/* test_rt_plan <scenario> [file] -- the serialized plan "kzrt-plan 1" and kz_rt_plan_load (kz_rt_plan.c).
 *   ok        a plan for the build identity's manifest loads (checked); its gate holds and its release resumes; the
 *             dump's PLAN record keeps the bytes and the status
 *   mismatch  a plan for another manifest is refused before any point is set (no POINT records, status manifest)
 *   crc       a changed byte: refused (crc)
 *   format    a point count that does not match the size: refused (format); a short buffer: refused (size)
 *   points    a release of a point that is not a gate: refused by kz_rt_plan (points)
 *   twice     a second load is refused (ESTATE) and the first outcome stays in the dump
 *   nobuild   (built without the build-identity object) loads unchecked (no-build-identity)
 *   write F   writes the "ok" plan to F (for the start-up object cases rt_auto_plan*)
 * Built with the build identity of data/sites_example.kzs (KZRT_TEST_MANIFEST_SHA) except test_rt_plan_nb.
 * Unit-test values, not research conditions. */
#include "rt_test.h"

#include "kz_rt_posix.h"

#ifndef KZRT_TEST_MANIFEST_SHA
#define KZRT_TEST_MANIFEST_SHA "0000000000000000000000000000000000000000000000000000000000000000"
#endif

enum { S_GATE = 10, S_REL = 20 };

static void put32(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)v;
  p[1] = (unsigned char)(v >> 8);
  p[2] = (unsigned char)(v >> 16);
  p[3] = (unsigned char)(v >> 24);
}
static void put64(unsigned char *p, uint64_t v) {
  put32(p, (uint32_t)v);
  put32(p + 4, (uint32_t)(v >> 32));
}
static uint32_t crc32_of(const unsigned char *p, size_t n) {
  uint32_t c = 0xFFFFFFFFu;
  size_t i;
  int b;
  for (i = 0; i < n; ++i) {
    c ^= p[i];
    for (b = 0; b < 8; ++b) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return c ^ 0xFFFFFFFFu;
}
static void sha_bytes(const char *hex, unsigned char out[32]) {
  int i;
  for (i = 0; i < 32; ++i) {
    unsigned v = 0;
    CHECK(sscanf(hex + 2 * i, "%2x", &v) == 1);
    out[i] = (unsigned char)v;
  }
}

/* Serializes pts (names "P<i>") for manifest sha `msha` into buf; returns the size. */
static size_t serialize(unsigned char *buf, const kz_rt_point *pts, uint32_t n, const unsigned char msha[32]) {
  const size_t total = KZ_RT_PLAN_HEADER_BYTES + (size_t)n * KZ_RT_PLAN_POINT_BYTES + KZ_RT_PLAN_TRAILER_BYTES;
  uint32_t i, j;
  memset(buf, 0, total);
  memcpy(buf, KZ_RT_PLAN_MAGIC, 8);
  buf[8] = (unsigned char)KZ_RT_PLAN_MAJOR;
  buf[10] = (unsigned char)KZ_RT_PLAN_MINOR;
  put32(buf + 12, n);
  put32(buf + 16, (uint32_t)total);
  memcpy(buf + 24, msha, 32);
  for (i = 0; i < n; ++i) {
    unsigned char *q = buf + KZ_RT_PLAN_HEADER_BYTES + (size_t)i * KZ_RT_PLAN_POINT_BYTES;
    const kz_rt_point *p = &pts[i];
    put32(q, p->kind);
    put32(q + 4, p->label);
    put32(q + 8, p->site);
    put32(q + 12, p->releases);
    put32(q + 16, p->occurrence);
    put32(q + 20, p->flags);
    put32(q + 24, p->bind_n);
    for (j = 0; j < KZ_RT_POINT_BINDS; ++j) put32(q + 28 + 4 * j, p->bind[j]);
    put32(q + 60, (uint32_t)p->str_slot);
    put32(q + 64, p->str_len);
    memcpy(q + 68, p->str, KZ_RT_STR_MAX);
    put32(q + 132, (uint32_t)p->val_slot);
    put64(q + 140, p->val);
    put64(q + 148, p->timeout_ns);
    snprintf((char *)q + 156, KZ_RT_PLAN_NAME_BYTES, "P%u", i);
  }
  put32(buf + total - 4, crc32_of(buf, total - 4));
  return total;
}

static size_t ok_plan(unsigned char *buf, const char *sha_hex) {
  kz_rt_point p[2];
  unsigned char sha[32];
  p[0] = gate_point(S_GATE, 1, KZ_RT_PT_UNLESS_RELEASED);
  p[0].label = 1;
  p[1] = release_point(S_REL, 0, 1);
  p[1].label = 2;
  sha_bytes(sha_hex, sha);
  return serialize(buf, p, 2, sha);
}

static const kz_rt_dec_plan *find_plan(const items *s) {
  size_t i;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_PLAN) return &s->v[i].u.plan;
  return NULL;
}

static void start(void) {
  kz_rt_config cfg = test_config(KZ_RT_MODE_CONTROL);
  cfg.backend = NULL; /* the build identity's, where linked */
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
}

/* The dump: the PLAN record's status and flags, the bytes it kept, and the POINT record count. */
static void expect(uint32_t status, uint32_t flags, const unsigned char *bytes, size_t n, size_t points) {
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_plan *q;
  dump_decode(&mb, &it, &st);
  q = find_plan(&it);
  CHECK(q != NULL);
  CHECK(q->status == status);
  CHECK(q->flags == flags);
  CHECK(q->nbytes == n && q->given == n);
  CHECK(n == 0 || memcmp(q->bytes, bytes, n) == 0);
  CHECK(count_type(&it, KZ_RT_REC_POINT) == points);
  free(mb.p);
  free(it.v);
}

typedef struct hit_arg {
  int result;
} hit_arg;

static void *gate_thread(void *a) {
  hit_arg *h = (hit_arg *)a;
  const uint32_t ev = kz_rt_obs(S_GATE, 1010, 0, 0, 0, 0, 0, 0);
  h->result = kz_rt_ctl(S_GATE, 1010, ev, KZ_RT_CTX_TASK);
  return NULL;
}

int main(int argc, char **argv) {
  static unsigned char buf[KZ_RT_PLAN_MAX_BYTES + 16];
  const char *s = argc > 1 ? argv[1] : "";
  size_t n;
  if (strcmp(s, "write") == 0) {
    FILE *f;
    CHECK(argc > 2);
    n = ok_plan(buf, KZRT_TEST_MANIFEST_SHA);
    f = fopen(argv[2], "wb");
    CHECK(f != NULL && fwrite(buf, 1, n, f) == n && fclose(f) == 0);
    printf("plan written: %zu bytes\n", n);
    return 0;
  }
  start();
  if (strcmp(s, "ok") == 0) {
    pthread_t t;
    hit_arg h = {0};
    kz_rt_point_state ps;
    n = ok_plan(buf, KZRT_TEST_MANIFEST_SHA);
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_OK);
    CHECK(pthread_create(&t, NULL, gate_thread, &h) == 0);
    wait_held(0);
    CHECK(kz_rt_ctl(S_REL, 1020, kz_rt_obs(S_REL, 1020, 0, 0, 0, 0, 0, 0), KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
    CHECK(pthread_join(t, NULL) == 0);
    CHECK(h.result == KZ_RT_CTL_RESUMED);
    CHECK(kz_rt_ctl_state(1, &ps) == KZ_RT_OK && ps.hits == 1 && ps.acted == 1);
    expect(KZ_RT_PLANST_LOADED, KZ_RT_PLANF_CHECKED, buf, n, 2);
  } else if (strcmp(s, "mismatch") == 0) {
    n = ok_plan(buf, "1111111111111111111111111111111111111111111111111111111111111111");
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_EINVAL);
    CHECK(kz_rt_ctl(S_GATE, 1010, kz_rt_obs(S_GATE, 1010, 0, 0, 0, 0, 0, 0), KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS); /* no plan: no hold */
    expect(KZ_RT_PLANST_MANIFEST, KZ_RT_PLANF_CHECKED, buf, n, 0);
  } else if (strcmp(s, "crc") == 0) {
    n = ok_plan(buf, KZRT_TEST_MANIFEST_SHA);
    buf[KZ_RT_PLAN_HEADER_BYTES + 16] ^= 1u; /* the first point's occurrence */
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_EINVAL);
    expect(KZ_RT_PLANST_CRC, 0, buf, n, 0);
  } else if (strcmp(s, "format") == 0) {
    n = ok_plan(buf, KZRT_TEST_MANIFEST_SHA);
    put32(buf + 12, 3); /* three points in the size of two */
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_EINVAL);
    expect(KZ_RT_PLANST_FORMAT, 0, buf, n, 0);
  } else if (strcmp(s, "size") == 0) {
    n = ok_plan(buf, KZRT_TEST_MANIFEST_SHA);
    CHECK(kz_rt_plan_load(buf, 60) == KZ_RT_EINVAL);
    expect(KZ_RT_PLANST_SIZE, 0, buf, 60, 0);
  } else if (strcmp(s, "points") == 0) {
    kz_rt_point p[2];
    unsigned char sha[32];
    p[0] = release_point(S_REL, 1, 1);
    p[1] = release_point(S_REL, 0, 1); /* releases a release */
    sha_bytes(KZRT_TEST_MANIFEST_SHA, sha);
    n = serialize(buf, p, 2, sha);
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_EINVAL);
    expect(KZ_RT_PLANST_POINTS, KZ_RT_PLANF_CHECKED, buf, n, 0);
  } else if (strcmp(s, "twice") == 0) {
    n = ok_plan(buf, KZRT_TEST_MANIFEST_SHA);
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_OK);
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_ESTATE);
    kz_rt_plan_note(KZ_RT_PLANST_NONE); /* ignored: the outcome is settled */
    expect(KZ_RT_PLANST_LOADED, KZ_RT_PLANF_CHECKED, buf, n, 2);
  } else if (strcmp(s, "nobuild") == 0) {
    n = ok_plan(buf, KZRT_TEST_MANIFEST_SHA);
    CHECK(kz_rt_plan_load(buf, n) == KZ_RT_OK);
    expect(KZ_RT_PLANST_LOADED, KZ_RT_PLANF_NO_BUILD, buf, n, 2);
  } else {
    fprintf(stderr, "usage: test_rt_plan ok|mismatch|crc|format|size|points|twice|nobuild|write F\n");
    return 2;
  }
  printf("plan %s: ok\n", s);
  return 0;
}

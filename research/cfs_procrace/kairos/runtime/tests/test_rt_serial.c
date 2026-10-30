/* test_rt_serial -- serialization round trip: every field written by the runtime is read back by
 * the host decoder; the bytes are little-endian whatever the host; strings are copied only through
 * kz_rt_obs_str and only up to the bound; the reader skips unknown record types and refuses another
 * major version.  With an argument, the dump is also written to that file (for kz-rt-dump). */
#include "rt_test.h"

#include <stdint.h>

#include "kz_rt_posix.h"

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

static void le32(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)v, p[1] = (unsigned char)(v >> 8), p[2] = (unsigned char)(v >> 16), p[3] = (unsigned char)(v >> 24);
}

static int find_bytes(const membuf *m, const unsigned char *pat, size_t n) {
  size_t i;
  for (i = 0; i + n <= m->n; ++i)
    if (memcmp(m->p + i, pat, n) == 0) return 1;
  return 0;
}

int main(int argc, char **argv) {
  static const char name0[] = "OBJ0";
  static const char longname[] = "OBJ_LONG_NAME";
  unsigned char sha[32];
  uint64_t v8[8];
  kz_rt_config cfg = test_config(KZ_RT_MODE_RECORD);
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  uint32_t a1, a2, a3, i;
  size_t j, ev = 0;
  const kz_rt_dec_run *run = NULL;
  for (i = 0; i < 32; ++i) sha[i] = (unsigned char)(0xA0u + i); /* not a pattern the byte-order check looks for */
#ifdef KZRT_TEST_MANIFEST_SHA
  /* the sha256 of the example manifest (set by the build), so that kz-rt-dump --manifest finds it */
  for (i = 0; i < 32; ++i) {
    unsigned v = 0;
    CHECK(sscanf(&KZRT_TEST_MANIFEST_SHA[2 * i], "%2x", &v) == 1);
    sha[i] = (unsigned char)v;
  }
#endif
  cfg.run_id = 0x1122334455667788ull;
  cfg.instance_id = 2;
  cfg.manifest_sha256 = sha;
  cfg.compiler = "cc-test";
  cfg.target = "host";
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);

  a1 = kz_rt_obs(10, 1001, 0, 0x7u, 0x0102030405060708ull, (uint64_t)(int64_t)-2, (uint64_t)(uintptr_t)name0, 0);
  kz_rt_module(5, sha); /* registered by actor 1 (its cell has an actor after the first event) */
  a2 = kz_rt_obs_str(11, 1101, a1, 0x3u, (uint64_t)(uintptr_t)name0, 4, 0, 0, 0, 16);
  a3 = kz_rt_obs_str(11, 1101, a1, 0x3u, (uint64_t)(uintptr_t)longname, 13, 0, 0, 0, 3);
  for (i = 0; i < 8; ++i) v8[i] = 0x100u + i;
  CHECK(kz_rt_obsv(12, 1201, a3, 0xFFu, 8, v8) == 4);
  CHECK(kz_rt_obs_str(11, 1101, 0, 0x2u, (uint64_t)(uintptr_t)name0, 4, 0, 0, 0, 16) == 5); /* slot 0 not valid: no copy */
  kz_rt_actor_exit();
  CHECK(a1 == 1 && a2 == 2 && a3 == 3);

  dump_decode(&mb, &it, &st);
  CHECK(st.major == KZ_RT_FORMAT_MAJOR && st.minor == KZ_RT_FORMAT_MINOR);
  for (j = 0; j < it.n; ++j) {
    const kz_rt_dec_item *x = &it.v[j];
    if (x->type == KZ_RT_REC_RUN) run = &x->u.run;
    if (x->type == KZ_RT_REC_MODULE) CHECK(x->u.module.module == 5 && memcmp(x->u.module.manifest_sha256, sha, 32) == 0 && x->u.module.actor == 1);
    if (x->type == KZ_RT_REC_AEXIT) CHECK(x->u.aexit.actor == 1 && x->u.aexit.aseq == 6);
    if (x->type == KZ_RT_REC_ACTOR) CHECK((x->u.actor.flags & KZ_RT_AF_EXITED) != 0 && x->u.actor.last_aseq == 6);
    if (x->type != KZ_RT_REC_EVENT) continue;
    {
      const kz_rt_dec_event *e = &x->u.event;
      ++ev;
      CHECK(e->actor == 1 && e->aseq == ev && (e->flags & KZ_RT_EF_PC) != 0 && e->pc != 0 && (e->flags & KZ_RT_EF_CLOCK) != 0);
      if (ev == 1) {
        CHECK(e->site == 10 && e->bind == 1001 && e->link == 0 && e->valid == 0x7u && e->nvals == 4);
        CHECK(e->v[0] == 0x0102030405060708ull && e->v[1] == 0xFFFFFFFFFFFFFFFEull && e->v[2] == (uint64_t)(uintptr_t)name0);
        CHECK((e->flags & KZ_RT_EF_STR) == 0 && e->str_len == 0); /* kz_rt_obs never reads the pointer */
      } else if (ev == 2) {
        CHECK(e->site == 11 && e->link == 1 && (e->flags & KZ_RT_EF_STR) != 0 && (e->flags & KZ_RT_EF_STR_TRUNC) == 0);
        CHECK(e->str_slot == 0 && e->str_len == 4 && memcmp(e->str, "OBJ0", 4) == 0);
      } else if (ev == 3) {
        CHECK((e->flags & KZ_RT_EF_STR_TRUNC) != 0 && e->str_len == 3 && memcmp(e->str, "OBJ", 3) == 0);
      } else if (ev == 4) {
        CHECK(e->nvals == 8 && e->valid == 0xFFu && e->v[7] == 0x107u && e->link == 3);
      } else if (ev == 5) {
        CHECK((e->flags & KZ_RT_EF_STR) == 0);
      }
    }
  }
  CHECK(ev == 5);
  CHECK(run != NULL && run->run_id == 0x1122334455667788ull && run->instance_id == 2 && run->mode == KZ_RT_MODE_RECORD);
  CHECK(memcmp(run->manifest_sha256, sha, 32) == 0 && strcmp(run->backend, "unit-test") == 0 && strcmp(run->compiler, "cc-test") == 0);
  CHECK(strcmp(run->target, "host") == 0 && strcmp(run->adapter, "posix") == 0 && strcmp(run->runtime, KZ_RT_RUNTIME_VERSION) == 0);
  CHECK(run->max_actors == KZ_RT_MAX_ACTORS && run->str_max == KZ_RT_STR_MAX && run->clock_id == KZ_RT_CLOCK_POSIX_MONOTONIC);
  CHECK((run->holdable_ctx_mask & (1u << KZ_RT_CTX_TASK)) != 0 && (run->holdable_ctx_mask & (1u << KZ_RT_CTX_ISR)) == 0);
  CHECK(count_type(&it, KZ_RT_REC_RESOURCE) == 2);
  /* little-endian bytes, whatever the host order */
  {
    static const unsigned char le[8] = {8, 7, 6, 5, 4, 3, 2, 1}, be[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    static const unsigned char run_le[8] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};
    CHECK(find_bytes(&mb, le, 8) && !find_bytes(&mb, be, 8) && find_bytes(&mb, run_le, 8));
    CHECK(memcmp(mb.p, "KZRTDUMP", 8) == 0 && mb.p[8] == KZ_RT_FORMAT_MAJOR && mb.p[9] == 0);
  }
  /* a record type the reader does not know is skipped by its size */
  {
    unsigned char s[16 + 16 + 24];
    items x = {0};
    kz_rt_dec_stats xs;
    memcpy(s, mb.p, 16);
    memset(s + 16, 0, 16);
    le32(s + 16, 16);
    s[20] = 0x77;
    memset(s + 32, 0, 24);
    le32(s + 32, 24);
    s[36] = 0xFF; /* END */
    le32(s + 40, 32);
    le32(s + 48, crc32_of(s, 32));
    CHECK(kz_rt_decode(s, sizeof s, collect, &x, &xs) == KZ_RT_DEC_OK && xs.unknown == 1 && xs.end_seen && xs.crc_ok);
    s[8] = 2; /* another major version */
    CHECK(kz_rt_decode(s, sizeof s, collect, &x, &xs) == KZ_RT_DEC_EVERSION);
    free(x.v);
  }
  /* a changed byte is caught by the END check */
  {
    items x = {0};
    kz_rt_dec_stats xs;
    unsigned char *copy = (unsigned char *)malloc(mb.n);
    CHECK(copy != NULL);
    memcpy(copy, mb.p, mb.n);
    copy[24] ^= 1; /* RUN's abi field (record at 16, field at +8) */
    CHECK(kz_rt_decode(copy, mb.n, collect, &x, &xs) == KZ_RT_DEC_OK && xs.end_seen && !xs.crc_ok);
    free(copy);
    free(x.v);
  }
  if (argc > 1) {
    FILE *f = fopen(argv[1], "wb");
    CHECK(f != NULL && fwrite(mb.p, 1, mb.n, f) == mb.n && fclose(f) == 0);
  }
  printf("serial: %zu records round trip, %zu bytes\n", it.n, mb.n);
  free(mb.p);
  free(it.v);
  return 0;
}

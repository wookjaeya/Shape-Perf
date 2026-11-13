/* test_rt_mode_api -- the setup calls around the mode word and the build identity (kz_rt.h), with the
 * build-identity object of the example manifest (kzrt_build_info, src/kz_rt_build_info.c) linked in:
 *   latch   kz_rt_early_mode(off) latches the mode: a second early call and kz_rt_init with another mode
 *           are refused (recorded), kz_rt_init with the same mode works, no hook records or counts,
 *           kz_rt_stop keeps OFF; the RUN tags are the build info's although the harness passes others
 *           (recorded as a conflict); kz_rt_module counts a repeated (module, sha256) once and keeps a
 *           second sha256 of the same module apart
 *   late    a hook before kz_rt_early_mode (static default record): the early mode records that hooks
 *           ran before it; the bootstrap record stays
 * The argument after the case is the dump file for kz-rt-dump.  Values are unit-test values. */
#include "rt_test.h"

#include "kz_rt_posix.h"

/* The build info's manifest sha256 as bytes. */
static void build_sha(const kz_rt_build *bi, unsigned char out[32]) {
  unsigned i, v;
  CHECK(bi->manifest_sha256_hex != NULL && strlen(bi->manifest_sha256_hex) == 64);
  for (i = 0; i < 32; ++i) {
    CHECK(sscanf(bi->manifest_sha256_hex + 2 * i, "%2x", &v) == 1);
    out[i] = (unsigned char)v;
  }
}

static const kz_rt_dec_runx *find_runx(const items *s) {
  size_t i;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_RUNX) return &s->v[i].u.runx;
  return NULL;
}
static const kz_rt_dec_run *find_run(const items *s) {
  size_t i;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_RUN) return &s->v[i].u.run;
  return NULL;
}

int main(int argc, char **argv) {
  extern const kz_rt_build kz_rt_build_info; /* the build-identity object */
  const char *what = argc > 1 ? argv[1] : "latch";
  membuf mb = {0};
  items it = {0};
  kz_rt_dec_stats st;
  const kz_rt_dec_run *run;
  const kz_rt_dec_runx *rx;
  kz_rt_config cfg;
  size_t j, modules = 0, events = 0;
  unsigned char bsha[32];
  CHECK(kz_rt_build_info.abi == KZ_RT_ABI_VERSION);
  build_sha(&kz_rt_build_info, bsha);
  if (strcmp(what, "latch") == 0) {
    unsigned char other[32];
    memset(other, 0x5A, sizeof other);
    CHECK(kz_rt_early_mode(7, 0) == KZ_RT_EINVAL);
    CHECK(kz_rt_early_mode(KZ_RT_MODE_OFF, KZ_RT_EARLY_GIVEN) == KZ_RT_OK);
    CHECK(kz_rt_early_mode(KZ_RT_MODE_RECORD, KZ_RT_EARLY_GIVEN) == KZ_RT_ESTATE);
    CHECK(kz_rt_obs(1, 11, 0, 1u, 1, 0, 0, 0) == 0); /* off from before init */
    cfg = test_config(KZ_RT_MODE_RECORD);
    cfg.compiler = "a compiler the binary was not built with";
    CHECK(kz_rt_init(&cfg) == KZ_RT_EINVAL); /* another mode than the latched one */
    cfg.mode = KZ_RT_MODE_OFF;
    CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
    CHECK(kz_rt_early_mode(KZ_RT_MODE_OFF, KZ_RT_EARLY_GIVEN) == KZ_RT_ESTATE);
    CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
    CHECK(kz_rt_obs(1, 11, 0, 1u, 2, 0, 0, 0) == 0);
    CHECK(kz_rt_ctl(1, 11, 0, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
    kz_rt_module(5, bsha);
    kz_rt_module(5, bsha);
    kz_rt_module(5, bsha);
    kz_rt_module(5, other); /* the same module from another manifest: kept apart (kz-rt-dump fails) */
    kz_rt_stop();
    CHECK(kz_rt_obs(1, 11, 0, 1u, 3, 0, 0, 0) == 0);
    dump_decode(&mb, &it, &st);
    run = find_run(&it);
    rx = find_runx(&it);
    CHECK(st.dump_flags == KZ_RT_DUMP_STOPPED && run != NULL && run->mode == KZ_RT_MODE_OFF);
    CHECK(strcmp(run->backend, kz_rt_build_info.backend) == 0 && strcmp(run->compiler, kz_rt_build_info.compiler) == 0);
    CHECK(strcmp(run->target, kz_rt_build_info.target) == 0 && memcmp(run->manifest_sha256, bsha, 32) == 0);
    CHECK(rx != NULL && rx->window_mode == KZ_RT_MODE_OFF && rx->mode_source == KZ_RT_MODE_SRC_EARLY);
    CHECK(rx->flags == (KZ_RT_RX_BUILD_INFO | KZ_RT_RX_TAG_CONFLICT | KZ_RT_RX_INIT_REFUSED));
    CHECK(loss_count(&it, KZ_RT_LOSS_AFTER_STOP, 0) == 0); /* off counts nothing, also after stop */
    for (j = 0; j < it.n; ++j) {
      const kz_rt_dec_item *x = &it.v[j];
      events += x->type == KZ_RT_REC_EVENT;
      if (x->type != KZ_RT_REC_MODULE) continue;
      ++modules;
      CHECK(x->u.module.module == 5);
      if (memcmp(x->u.module.manifest_sha256, other, 32) == 0)
        CHECK(x->u.module.registrations == 1);
      else
        CHECK(x->u.module.registrations == 3 && memcmp(x->u.module.manifest_sha256, bsha, 32) == 0);
    }
    CHECK(modules == 2 && events == 0);
    printf("latch: off from before init, init with another mode refused, build info kept, module 5: 3 registrations + 1 other manifest\n");
  } else if (strcmp(what, "late") == 0) {
    CHECK(kz_rt_obs(1, 11, 0, 1u, 1, 0, 0, 0) == 1); /* static default: recorded in the bootstrap buffer */
    CHECK(kz_rt_early_mode(KZ_RT_MODE_RECORD, KZ_RT_EARLY_DEFAULTED) == KZ_RT_OK);
    cfg = test_config(KZ_RT_MODE_RECORD);
    cfg.backend = NULL; /* nothing to conflict with */
    CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
    CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
    CHECK(kz_rt_obs(1, 11, 0, 1u, 2, 0, 0, 0) == 2);
    kz_rt_module(5, bsha);
    kz_rt_stop();
    dump_decode(&mb, &it, &st);
    run = find_run(&it);
    rx = find_runx(&it);
    CHECK(run != NULL && run->mode == KZ_RT_MODE_STOPPED && rx != NULL && rx->window_mode == KZ_RT_MODE_RECORD);
    CHECK(rx->mode_source == KZ_RT_MODE_SRC_EARLY && rx->flags == (KZ_RT_RX_BUILD_INFO | KZ_RT_RX_HOOKS_BEFORE_EARLY | KZ_RT_RX_MODE_DEFAULTED));
    CHECK(count_type(&it, KZ_RT_REC_EVENT) == 2);
    printf("late: the early mode after a hook is recorded as such; both events kept\n");
  } else {
    fprintf(stderr, "usage: test_rt_mode_api latch|late [dump-file]\n");
    return 2;
  }
  if (argc > 2) {
    FILE *f = fopen(argv[2], "wb");
    CHECK(f != NULL && fwrite(mb.p, 1, mb.n, f) == mb.n && fclose(f) == 0);
  }
  free(mb.p);
  free(it.v);
  return 0;
}

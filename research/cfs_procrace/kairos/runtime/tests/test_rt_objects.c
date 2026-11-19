/* test_rt_objects.c -- the build-id records of dump 1.5 (kzrt 0.5.0, event_semantics.md 13.5).
 *
 *   test_rt_objects <dump> <module-with-build-id.so> <module-without-build-id.so>
 *       a registration of this executable (module 3) and one at an address no object holds (module 9, a stack array) before the object
 *       reader is given; the POSIX reader; then the two modules loaded with dlopen, whose constructors register modules 7
 *       and 8; module 3 registered again (one record per (module, object)).  The dump's BUILDID records are checked
 *       here; check_buildid.sh compares their build-ids with readelf -n of the files.
 *   test_rt_objects <dump> noreader
 *       registrations without a reader: no BUILDID record, LOSS no-build-id per module.
 * Unit-test values, not research conditions. */
#include "rt_test.h"

#include <dlfcn.h>
#include <stdint.h>

#include "kz_rt_posix.h"

static const unsigned char exe_sha[32] = {0x33, 0x33};

typedef struct seen {
  int n, loss_n;
  kz_rt_dec_buildid b[16];
  kz_rt_dec_loss loss[16];
} seen;

static int collect_obj(void *ctx, const kz_rt_dec_item *it) {
  seen *s = (seen *)ctx;
  if (it->type == KZ_RT_REC_BUILDID && s->n < 16) s->b[s->n++] = it->u.buildid;
  if (it->type == KZ_RT_REC_LOSS && s->loss_n < 16 && (it->u.loss.kind == KZ_RT_LOSS_NO_BUILD_ID || it->u.loss.kind == KZ_RT_LOSS_OBJECT_TABLE))
    s->loss[s->loss_n++] = it->u.loss;
  return 0;
}

static const kz_rt_dec_buildid *find(const seen *s, uint32_t object) {
  int i;
  for (i = 0; i < s->n; ++i)
    if (s->b[i].object == object) return &s->b[i];
  return NULL;
}

static uint64_t loss_of(const seen *s, uint32_t kind, uint32_t key) {
  int i;
  for (i = 0; i < s->loss_n; ++i)
    if (s->loss[i].kind == kind && s->loss[i].key == key) return s->loss[i].count;
  return 0;
}

int main(int argc, char **argv) {
  kz_rt_config cfg;
  membuf mb = {0};
  seen s;
  kz_rt_dec_stats st;
  unsigned char on_stack[32] = {9};
  const int noreader = argc == 3 && strcmp(argv[2], "noreader") == 0;
  CHECK(argc == 4 || noreader);
  memset(&cfg, 0, sizeof cfg);
  cfg.abi = KZ_RT_ABI_VERSION;
  cfg.mode = KZ_RT_MODE_RECORD;
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_attach(kz_rt_posix_adapter()) == KZ_RT_OK);
  kz_rt_module(3, exe_sha);                                  /* before the reader: kept */
  kz_rt_module(9, on_stack);                                 /* an address no loaded object holds (the stack) */
  if (!noreader) {
    void *h1, *h2;
    kz_rt_posix_objects();
    kz_rt_posix_objects(); /* a second call is ignored */
    h1 = dlopen(argv[2], RTLD_NOW);
    CHECK(h1 != NULL);
    h2 = dlopen(argv[3], RTLD_NOW);
    CHECK(h2 != NULL);
    kz_rt_module(3, exe_sha); /* the same (module, object) again */
    (void)h1;
    (void)h2;
  }
  kz_rt_stop();
  CHECK(kz_rt_dump(mem_write, &mb) == KZ_RT_OK);
  {
    FILE *f = fopen(argv[1], "wb");
    CHECK(f != NULL && fwrite(mb.p, 1, mb.n, f) == mb.n && fclose(f) == 0);
  }
  memset(&s, 0, sizeof s);
  CHECK(kz_rt_decode(mb.p, mb.n, collect_obj, &s, &st) == KZ_RT_DEC_OK && st.minor == 5 && st.unknown == 0);
  if (noreader) {
    CHECK(s.n == 0);
    CHECK(loss_of(&s, KZ_RT_LOSS_NO_BUILD_ID, 3) == 1 && loss_of(&s, KZ_RT_LOSS_NO_BUILD_ID, 9) == 1);
    printf("noreader: no BUILDID record; loss no-build-id for modules 3 and 9\n");
    return 0;
  }
  CHECK(s.n == 5);
  {
    const kz_rt_dec_buildid *m = find(&s, 0), *e = find(&s, 3), *u = find(&s, 9), *a = find(&s, 7), *b = find(&s, 8);
    CHECK(m != NULL && e != NULL && u != NULL && a != NULL && b != NULL);
    CHECK(s.b[0].object == 0); /* the main executable first */
    CHECK(m->source == KZ_RT_OBJSRC_START && m->note == KZ_RT_NOTE_PRESENT && m->id_len >= 8u);
    /* module 3 is a registration of this executable: the same object, the same build-id */
    CHECK(e->source == KZ_RT_OBJSRC_REGISTRATION && e->note == KZ_RT_NOTE_PRESENT && e->id_len == m->id_len && memcmp(e->id, m->id, m->id_len) == 0);
    CHECK(u->note == KZ_RT_NOTE_UNREADABLE && u->id_len == 0 && u->path_len == 0);
    CHECK(a->source == KZ_RT_OBJSRC_REGISTRATION && a->note == KZ_RT_NOTE_PRESENT && strstr(a->path, argv[2]) != NULL);
    CHECK(b->note == KZ_RT_NOTE_ABSENT && b->id_len == 0 && strstr(b->path, argv[3]) != NULL);
    CHECK(a->id_len != m->id_len || memcmp(a->id, m->id, m->id_len) != 0);
    CHECK(loss_of(&s, KZ_RT_LOSS_NO_BUILD_ID, 8) == 1 && loss_of(&s, KZ_RT_LOSS_NO_BUILD_ID, 9) == 1 && loss_of(&s, KZ_RT_LOSS_NO_BUILD_ID, 7) == 0);
  }
  printf("objects: 5 BUILDID records checked (main, module 3 of the executable, 7 and 8 loaded, 9 unreadable)\n");
  return 0;
}

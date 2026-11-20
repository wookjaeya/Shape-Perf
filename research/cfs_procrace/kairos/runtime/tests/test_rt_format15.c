/* test_rt_format15.c -- the readers of dump 1.5 on synthetic dumps (event_semantics.md 13.5): BUILDID records (a main
 * executable, a module with a cut path, a module without a note) and the reserved creation-token records TOK_CREATE,
 * TOK_RETURN and TOK_START in an actor's buffer, which this runtime never writes.  Malformed sizes are refused.
 *   test_rt_format15 <out.kzrt>     writes the well-formed synthetic dump for kz-rt-dump (rt_format15_dump)
 * Unit-test values, not research conditions. */
#include "rt_test.h" /* first: it defines _GNU_SOURCE */

#include <stdint.h>

static void le16(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)v;
  p[1] = (unsigned char)(v >> 8);
}
static void le32(unsigned char *p, uint32_t v) {
  le16(p, v & 0xFFFFu);
  le16(p + 2, v >> 16);
}
static void le64(unsigned char *p, uint64_t v) {
  le32(p, (uint32_t)v);
  le32(p + 4, (uint32_t)(v >> 32));
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

static unsigned char g[8192];
static size_t gn;

static unsigned char *rec(uint32_t size, uint32_t type, uint32_t sub) {
  unsigned char *r = g + gn;
  memset(r, 0, size);
  le32(r, size);
  le16(r + 4, type);
  le16(r + 6, sub);
  gn += size;
  return r;
}

static void buildid(uint32_t object, uint32_t source, uint32_t note, uint32_t actor, uint64_t t, const unsigned char *id, uint32_t id_len,
                    const char *path, uint32_t flags) {
  const uint32_t pl = (uint32_t)strlen(path), size = (KZ_RT_BUILDID_FIXED + id_len + pl + 3u) & ~3u;
  unsigned char *r = rec(size, KZ_RT_REC_BUILDID, 0);
  le32(r + 8, object);
  le32(r + 12, source);
  le32(r + 16, note);
  le32(r + 20, actor);
  le64(r + 24, t);
  le16(r + 32, id_len);
  le16(r + 34, pl);
  le32(r + 36, flags);
  memcpy(r + KZ_RT_BUILDID_FIXED, id, id_len);
  memcpy(r + KZ_RT_BUILDID_FIXED + id_len, path, pl);
}

/* The synthetic stream; tok_return_size / bad_id_len make it malformed. */
static void make(uint32_t minor, uint32_t tok_return_size, uint32_t bad_id_len) {
  static const unsigned char id_main[20] = {0xde, 0xad, 0xbe, 0xef, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  static const unsigned char id_mod[8] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89};
  unsigned char *buf, *r;
  size_t bstart;
  gn = 0;
  memcpy(g, "KZRTDUMP", 8);
  le16(g + 8, KZ_RT_FORMAT_MAJOR);
  le16(g + 10, minor);
  le32(g + 12, 1u);
  gn = 16;
  buildid(0, KZ_RT_OBJSRC_START, KZ_RT_NOTE_PRESENT, 1, 100, id_main, 20, "./core-test", 0);
  buildid(4, KZ_RT_OBJSRC_REGISTRATION, KZ_RT_NOTE_PRESENT, 1, 200, id_mod, bad_id_len ? 8 : 8, "cf/a \"q\".so", KZ_RT_OBJF_PATH_CUT);
  if (bad_id_len) le16(g + gn - ((KZ_RT_BUILDID_FIXED + 8 + 11 + 3) & ~3u) + 32, bad_id_len);
  buildid(5, KZ_RT_OBJSRC_REGISTRATION, KZ_RT_NOTE_ABSENT, 0, 300, NULL, 0, "cf/b.so", 0);
  /* BUF of actor 1: the three token records, one after the other */
  buf = rec(KZ_RT_BUF_FIXED, KZ_RT_REC_BUF, 0);
  bstart = gn;
  le32(buf + 8, 1);
  r = rec(KZ_RT_TOK_CREATE_SIZE, KZ_RT_REC_TOK_CREATE, KZ_RT_EF_CLOCK);
  le32(r + 8, 1);
  le32(r + 12, 1);
  le32(r + 16, 12);
  le64(r + 24, 77);
  le64(r + 32, 1000);
  r = rec(tok_return_size, KZ_RT_REC_TOK_RETURN, KZ_RT_EF_CLOCK);
  le32(r + 8, 1);
  le32(r + 12, 2);
  le32(r + 16, 12);
  le64(r + 24, 77);
  if (tok_return_size >= KZ_RT_TOK_RETURN_SIZE) {
    le64(r + 32, (uint64_t)(int64_t)-5);
    le64(r + 40, 0x7f00deadbeefull);
    le64(r + 48, 1001);
  }
  r = rec(KZ_RT_TOK_START_SIZE, KZ_RT_REC_TOK_START, KZ_RT_EF_CLOCK);
  le32(r + 8, 1);
  le32(r + 12, 3);
  le64(r + 24, 77);
  le64(r + 32, 1002);
  le32(buf, (uint32_t)(KZ_RT_BUF_FIXED + gn - bstart));
  le32(buf + 12, (uint32_t)(gn - bstart));
  le32(buf + 16, 65536);
  r = rec(KZ_RT_END_SIZE, KZ_RT_REC_END, 0);
  le64(r + 8, (uint64_t)(r - g));
  le32(r + 16, crc32_of(g, (size_t)(r - g)));
}

int main(int argc, char **argv) {
  items x = {0};
  kz_rt_dec_stats st;
  size_t i;
  int nb = 0, nt = 0;
  CHECK(argc == 2);
  make(5, KZ_RT_TOK_RETURN_SIZE, 0);
  CHECK(kz_rt_decode(g, gn, collect, &x, &st) == KZ_RT_DEC_OK && st.minor == 5 && st.unknown == 0 && st.end_seen && st.crc_ok);
  for (i = 0; i < x.n; ++i) {
    const kz_rt_dec_item *it = &x.v[i];
    if (it->type == KZ_RT_REC_BUILDID) {
      const kz_rt_dec_buildid *b = &it->u.buildid;
      if (nb == 0) CHECK(b->object == 0 && b->source == KZ_RT_OBJSRC_START && b->note == KZ_RT_NOTE_PRESENT && b->id_len == 20 && b->id[0] == 0xde &&
                         strcmp(b->path, "./core-test") == 0 && b->actor == 1 && b->t == 100);
      if (nb == 1) CHECK(b->object == 4 && b->id_len == 8 && (b->flags & KZ_RT_OBJF_PATH_CUT) != 0 && strcmp(b->path, "cf/a \"q\".so") == 0);
      if (nb == 2) CHECK(b->object == 5 && b->note == KZ_RT_NOTE_ABSENT && b->id_len == 0 && b->actor == 0);
      ++nb;
    }
    if (it->type == KZ_RT_REC_TOK_CREATE) CHECK(it->u.token.owner == 1 && it->u.token.aseq == 1 && it->u.token.site == 12 && it->u.token.token == 77 && it->u.token.t == 1000 && nt++ == 0);
    if (it->type == KZ_RT_REC_TOK_RETURN)
      CHECK(it->u.token.aseq == 2 && it->u.token.status == -5 && it->u.token.handle == 0x7f00deadbeefull && it->u.token.t == 1001 && nt++ == 1);
    if (it->type == KZ_RT_REC_TOK_START) CHECK(it->u.token.aseq == 3 && it->u.token.token == 77 && it->u.token.t == 1002 && nt++ == 2);
  }
  CHECK(nb == 3 && nt == 3);
  {
    FILE *f = fopen(argv[1], "wb");
    CHECK(f != NULL && fwrite(g, 1, gn, f) == gn && fclose(f) == 0);
  }
  /* a 1.4 reader's view is not tested here; a 1.5 stream with a short TOK_RETURN or an over-long build-id is refused */
  make(5, KZ_RT_TOK_CREATE_SIZE, 0);
  CHECK(kz_rt_decode(g, gn, collect, &x, &st) == KZ_RT_DEC_EFORMAT);
  make(5, KZ_RT_TOK_RETURN_SIZE, KZ_RT_BUILDID_MAX + 1u);
  CHECK(kz_rt_decode(g, gn, collect, &x, &st) == KZ_RT_DEC_EFORMAT);
  free(x.v);
  printf("format 1.5: 3 BUILDID and 3 token records read; 2 malformed streams refused\n");
  return 0;
}

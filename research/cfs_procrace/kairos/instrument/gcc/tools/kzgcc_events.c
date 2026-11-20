/* kzgcc_events.c -- prints the events of a kzrt-dump as tab-separated lines for the comparison scripts,
 * with what kz-rt-dump does not print: the native key (Linux TID) of each actor and the pc of each
 * event (the return address of the hook call: which code copy of a binding ran).  Host tool of the
 * stage C prototype; reads the dump with the runtime's own decoder (kairos/runtime/tools/decode).
 *
 *   kzgcc-events <dump>
 *   actor <tab> <actor> <tab> <native key> <tab> <boot link> <tab> <first aseq> <tab> <last aseq> <tab> <overflow>
 *   ev <tab> <actor> <tab> <aseq> <tab> <site> <tab> <bind> <tab> <link> <tab> <pc hex> <tab> <valid hex>
 *      <tab> v0 <tab> v1 <tab> v2 <tab> v3 <tab> <string or ->           (a value not bound is "-")
 *   loss <tab> <kind> <tab> <actor> <tab> <count>
 * Exit 0 = decoded to the end with a valid CRC; 1 = otherwise. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "kz_rt_decode.h"

static int on_item(void *ctx, const kz_rt_dec_item *it) {
  (void)ctx;
  if (it->type == KZ_RT_REC_ACTOR) {
    const kz_rt_dec_actor *a = &it->u.actor;
    printf("actor\t%u\t%" PRIu64 "\t%u\t%u\t%u\t%u\n", a->actor, a->native_key, a->boot_link, a->first_aseq, a->last_aseq, a->overflow);
  } else if (it->type == KZ_RT_REC_EVENT) {
    const kz_rt_dec_event *e = &it->u.event;
    unsigned i;
    printf("ev\t%u\t%u\t%u\t%u\t%u\t0x%" PRIx64 "\t0x%x", e->actor, e->aseq, e->site, e->bind, e->link, e->pc, e->valid);
    for (i = 0; i < 4; ++i) {
      if (i < e->nvals && (e->valid & (1u << i)) != 0) printf("\t0x%" PRIx64, e->v[i]);
      else printf("\t-");
    }
    if ((e->flags & KZ_RT_EF_STR) != 0) printf("\t%.*s\n", (int)e->str_len, (const char *)e->str);
    else printf("\t-\n");
  } else if (it->type == KZ_RT_REC_LOSS) {
    printf("loss\t%u\t%u\t%" PRIu64 "\n", it->u.loss.kind, it->u.loss.actor, it->u.loss.count);
  }
  return 0;
}

int main(int argc, char **argv) {
  FILE *f;
  unsigned char *buf;
  long n;
  kz_rt_dec_stats st;
  int r;
  if (argc != 2) {
    fprintf(stderr, "usage: kzgcc-events <dump>\n");
    return 2;
  }
  f = fopen(argv[1], "rb");
  if (f == NULL || fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
    fprintf(stderr, "kzgcc-events: cannot read %s\n", argv[1]);
    return 2;
  }
  buf = (unsigned char *)malloc((size_t)n + 1u);
  if (buf == NULL || fread(buf, 1, (size_t)n, f) != (size_t)n) return 2;
  fclose(f);
  r = kz_rt_decode(buf, (size_t)n, on_item, NULL, &st);
  free(buf);
  if (r != KZ_RT_DEC_OK || !st.end_seen || !st.crc_ok) {
    fprintf(stderr, "kzgcc-events: %s (end %u crc %u)\n", kz_rt_dec_error(r), st.end_seen, st.crc_ok);
    return 1;
  }
  return 0;
}

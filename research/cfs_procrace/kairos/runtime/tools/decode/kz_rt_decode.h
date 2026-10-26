/* kz_rt_decode.h -- host-side reader of the "kzrt-dump" format (kz_rt_format.h), major 1.
 * Reads the explicit little-endian bytes field by field (independent of the host's layout) and
 * hands each record to a callback, in stream order and, inside a BUF, in buffer order. */
#ifndef KZ_RT_DECODE_H
#define KZ_RT_DECODE_H

#include <stddef.h>
#include <stdint.h>

#include "kz_rt.h"
#include "kz_rt_format.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kz_rt_dec_run {
  uint32_t abi, mode, instance_id, phase, max_actors, actor_buf_bytes, boot_buf_bytes, max_points, max_vals, str_max, adapter_caps,
      clock_id, holdable_ctx_mask;
  uint64_t run_id, t_init, t_attach, t_stop, t_dump;
  unsigned char manifest_sha256[32];
  char runtime[KZ_RT_TAG_MAX + 1], adapter[KZ_RT_TAG_MAX + 1], backend[KZ_RT_TAG_MAX + 1], compiler[KZ_RT_TAG_MAX + 1],
      target[KZ_RT_TAG_MAX + 1];
} kz_rt_dec_run;

typedef struct kz_rt_dec_module {
  uint32_t module, actor;
  uint64_t t;
  unsigned char manifest_sha256[32];
} kz_rt_dec_module;

typedef struct kz_rt_dec_resource {
  uint32_t kind, origin;
  uint64_t count;
  char what[KZ_RT_TAG_MAX + 1];
} kz_rt_dec_resource;

typedef struct kz_rt_dec_actor {
  uint32_t actor, flags, boot_link, link_evidence, key_kind, key_generation, first_aseq, last_aseq, recorded, overflow, partial, buf_bytes;
  uint64_t native_key;
} kz_rt_dec_actor;

typedef struct kz_rt_dec_loss {
  uint32_t kind, actor, key;
  uint64_t count;
} kz_rt_dec_loss;

typedef struct kz_rt_dec_point {
  uint32_t index, kind, label, site, releases, occurrence, flags, hits, acted, satisfied, held_actor, holds;
} kz_rt_dec_point;

typedef struct kz_rt_dec_buf {
  uint32_t owner, nbytes, cap;
} kz_rt_dec_buf;

typedef struct kz_rt_dec_event {
  uint32_t owner; /* the buffer it was in: 0 = shared, else the run actor */
  uint32_t flags, actor, aseq, site, bind, link, valid, nvals, str_slot, str_len;
  uint64_t t, pc;
  uint64_t v[KZ_RT_MAX_VALS];
  unsigned char str[KZ_RT_STR_MAX];
} kz_rt_dec_event;

typedef struct kz_rt_dec_ctl {
  uint32_t owner, what, actor, aseq, site, bind, ev, ctl_seq, point, hit, subject, other, gate, reason, ctx;
  uint64_t t;
} kz_rt_dec_ctl;

typedef struct kz_rt_dec_aexit {
  uint32_t owner, flags, actor, aseq;
  uint64_t t;
} kz_rt_dec_aexit;

typedef struct kz_rt_dec_end {
  uint64_t bytes;
  uint32_t crc32, crc_ok;
} kz_rt_dec_end;

typedef struct kz_rt_dec_item {
  uint32_t type;   /* KZ_RT_REC_* */
  uint64_t offset; /* byte offset of the record in the stream (a buffer record: inside its BUF) */
  union {
    kz_rt_dec_run run;
    kz_rt_dec_module module;
    kz_rt_dec_resource resource;
    kz_rt_dec_actor actor;
    kz_rt_dec_loss loss;
    kz_rt_dec_point point;
    kz_rt_dec_buf buf;
    kz_rt_dec_event event;
    kz_rt_dec_ctl ctl;
    kz_rt_dec_aexit aexit;
    kz_rt_dec_end end;
  } u;
} kz_rt_dec_item;

typedef struct kz_rt_dec_stats {
  uint32_t major, minor, dump_flags;
  uint64_t records;      /* records handed to the callback */
  uint64_t unknown;      /* records of a type this reader does not know (skipped by size) */
  uint64_t partial;      /* truncated or uncommitted records (reading stopped there) */
  uint64_t pads;
  uint32_t end_seen, crc_ok;
} kz_rt_dec_stats;

/* Non-zero return stops the decoding (kz_rt_decode then returns KZ_RT_DEC_STOPPED). */
typedef int (*kz_rt_dec_fn)(void *ctx, const kz_rt_dec_item *item);

enum { KZ_RT_DEC_OK = 0, KZ_RT_DEC_EMAGIC = -1, KZ_RT_DEC_EVERSION = -2, KZ_RT_DEC_ETRUNC = -3, KZ_RT_DEC_EFORMAT = -4, KZ_RT_DEC_STOPPED = -5 };

int kz_rt_decode(const unsigned char *p, size_t n, kz_rt_dec_fn fn, void *ctx, kz_rt_dec_stats *st);
const char *kz_rt_dec_error(int code);

#ifdef __cplusplus
}
#endif
#endif /* KZ_RT_DECODE_H */

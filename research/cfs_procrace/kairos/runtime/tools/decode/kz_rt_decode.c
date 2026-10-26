/* kz_rt_decode.c -- see kz_rt_decode.h. */
#include "kz_rt_decode.h"

#include <string.h>

static uint32_t g16(const unsigned char *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
static uint32_t g32(const unsigned char *p) { return g16(p) | (g16(p + 2) << 16); }
static uint64_t g64(const unsigned char *p) { return (uint64_t)g32(p) | ((uint64_t)g32(p + 4) << 32); }

static uint32_t crc32_update(uint32_t crc, const unsigned char *p, size_t n) {
  size_t i;
  int b;
  for (i = 0; i < n; ++i) {
    crc ^= p[i];
    for (b = 0; b < 8; ++b) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return crc;
}

/* A u16-length text at *off inside [b, b+size); copies at most KZ_RT_TAG_MAX bytes. */
static int text(const unsigned char *b, uint32_t size, uint32_t *off, char *out) {
  uint32_t len;
  if (*off + 2u > size) return 0;
  len = g16(b + *off);
  if (*off + 2u + len > size) return 0;
  {
    const uint32_t c = len < KZ_RT_TAG_MAX ? len : KZ_RT_TAG_MAX;
    memcpy(out, b + *off + 2u, c);
    out[c] = 0;
  }
  *off += 2u + len;
  return 1;
}

static int buffer_record(const unsigned char *b, uint32_t size, uint32_t type, uint32_t sub, uint32_t owner, kz_rt_dec_item *it) {
  uint32_t i;
  memset(it, 0, sizeof *it);
  it->type = type;
  if (type == KZ_RT_REC_EVENT) {
    kz_rt_dec_event *e = &it->u.event;
    if (size < KZ_RT_EVENT_FIXED) return 0;
    e->owner = owner;
    e->flags = sub;
    e->actor = g32(b + 8);
    e->aseq = g32(b + 12);
    e->site = g32(b + 16);
    e->bind = g32(b + 20);
    e->link = g32(b + 24);
    e->valid = g32(b + 28);
    e->t = g64(b + 32);
    e->pc = g64(b + 40);
    e->nvals = g16(b + 48);
    e->str_len = g16(b + 50);
    e->str_slot = b[52];
    if (e->nvals > KZ_RT_MAX_VALS || e->str_len > KZ_RT_STR_MAX) return 0;
    if (KZ_RT_EVENT_FIXED + 8u * e->nvals + e->str_len > size) return 0;
    for (i = 0; i < e->nvals; ++i) e->v[i] = g64(b + KZ_RT_EVENT_FIXED + 8u * i);
    memcpy(e->str, b + KZ_RT_EVENT_FIXED + 8u * e->nvals, e->str_len);
    return 1;
  }
  if (type == KZ_RT_REC_CTL) {
    kz_rt_dec_ctl *c = &it->u.ctl;
    if (size < KZ_RT_CTL_SIZE) return 0;
    c->owner = owner;
    c->what = sub;
    c->actor = g32(b + 8);
    c->aseq = g32(b + 12);
    c->site = g32(b + 16);
    c->bind = g32(b + 20);
    c->ev = g32(b + 24);
    c->ctl_seq = g32(b + 28);
    c->t = g64(b + 32);
    c->point = g32(b + 40);
    c->hit = g32(b + 44);
    c->subject = g32(b + 48);
    c->other = g32(b + 52);
    c->gate = g32(b + 56);
    c->reason = g16(b + 60);
    c->ctx = g16(b + 62);
    return 1;
  }
  if (type == KZ_RT_REC_AEXIT) {
    kz_rt_dec_aexit *x = &it->u.aexit;
    if (size < KZ_RT_AEXIT_SIZE) return 0;
    x->owner = owner;
    x->flags = sub;
    x->actor = g32(b + 8);
    x->aseq = g32(b + 12);
    x->t = g64(b + 16);
    return 1;
  }
  return -1; /* unknown */
}

static int stream_record(const unsigned char *b, uint32_t size, uint32_t type, kz_rt_dec_item *it) {
  uint32_t off;
  memset(it, 0, sizeof *it);
  it->type = type;
  switch (type) {
    case KZ_RT_REC_RUN: {
      kz_rt_dec_run *r = &it->u.run;
      if (size < KZ_RT_RUN_FIXED) return 0;
      r->abi = g32(b + 8);
      r->mode = g32(b + 12);
      r->run_id = g64(b + 16);
      r->instance_id = g32(b + 24);
      r->phase = g32(b + 28);
      r->max_actors = g32(b + 32);
      r->actor_buf_bytes = g32(b + 36);
      r->boot_buf_bytes = g32(b + 40);
      r->max_points = g32(b + 44);
      r->max_vals = g32(b + 48);
      r->str_max = g32(b + 52);
      r->adapter_caps = g32(b + 56);
      r->clock_id = g32(b + 60);
      r->t_init = g64(b + 64);
      r->t_attach = g64(b + 72);
      r->t_stop = g64(b + 80);
      r->t_dump = g64(b + 88);
      memcpy(r->manifest_sha256, b + 96, 32);
      r->holdable_ctx_mask = g32(b + 128);
      off = KZ_RT_RUN_FIXED;
      if (!text(b, size, &off, r->runtime) || !text(b, size, &off, r->adapter) || !text(b, size, &off, r->backend) ||
          !text(b, size, &off, r->compiler) || !text(b, size, &off, r->target))
        return 0;
      return 1;
    }
    case KZ_RT_REC_RUNX: {
      kz_rt_dec_runx *x = &it->u.runx;
      if (size < KZ_RT_RUNX_SIZE_1_1) return 0;
      x->window_mode = g32(b + 8);
      x->mode_source = g32(b + 12);
      x->flags = g32(b + 16);
      x->pool_bytes = g32(b + 20);
      x->chunk_bytes = g32(b + 24);
      x->chunks_used = g32(b + 28);
      x->pool_dropped = g32(b + 32);
      x->actor_quota = g32(b + 36);
      x->boot_bytes_used = g32(b + 40);
      x->boot_bytes_dropped = g32(b + 44);
      x->max_modules = g32(b + 48);
      x->max_notes = g32(b + 52);
      x->process = x->initial_key = 0;
      x->initial_kind = x->process_evidence = 0;
      if (size >= KZ_RT_RUNX_SIZE) {
        x->process = g64(b + 56);
        x->initial_key = g64(b + 64);
        x->initial_kind = g32(b + 72);
        x->process_evidence = g32(b + 76);
      }
      return 1;
    }
    case KZ_RT_REC_MODULE:
      if (size < KZ_RT_MODULE_SIZE_1_0) return 0;
      it->u.module.module = g32(b + 8);
      it->u.module.actor = g32(b + 12);
      it->u.module.t = g64(b + 16);
      memcpy(it->u.module.manifest_sha256, b + 24, 32);
      it->u.module.registrations = size >= KZ_RT_MODULE_SIZE ? g32(b + 56) : 1u;
      return 1;
    case KZ_RT_REC_RESOURCE:
      if (size < KZ_RT_RESOURCE_FIXED) return 0;
      it->u.resource.kind = g32(b + 8);
      it->u.resource.origin = g32(b + 12);
      it->u.resource.count = g64(b + 16);
      off = KZ_RT_RESOURCE_FIXED;
      return text(b, size, &off, it->u.resource.what);
    case KZ_RT_REC_ACTOR: {
      kz_rt_dec_actor *a = &it->u.actor;
      if (size < KZ_RT_ACTOR_SIZE_1_0) return 0;
      a->actor = g32(b + 8);
      a->flags = g32(b + 12);
      a->boot_link = g32(b + 16);
      a->link_evidence = g32(b + 20);
      a->native_key = g64(b + 24);
      a->key_kind = g32(b + 32);
      a->key_generation = g32(b + 36);
      a->first_aseq = g32(b + 40);
      a->last_aseq = g32(b + 44);
      a->recorded = g32(b + 48);
      a->overflow = g32(b + 52);
      a->partial = g32(b + 56);
      a->buf_bytes = g32(b + 60);
      if (size >= KZ_RT_ACTOR_SIZE) {
        a->has_demand = 1;
        a->dropped_bytes = g32(b + 64);
        a->chunks = g32(b + 68);
        a->overflow_quota = g32(b + 72);
        a->overflow_pool = g32(b + 76);
      }
      return 1;
    }
    case KZ_RT_REC_LOSS:
      if (size < KZ_RT_LOSS_SIZE) return 0;
      it->u.loss.kind = g32(b + 8);
      it->u.loss.actor = g32(b + 12);
      it->u.loss.key = g32(b + 16);
      it->u.loss.count = g64(b + 24);
      return 1;
    case KZ_RT_REC_POINT: {
      kz_rt_dec_point *q = &it->u.point;
      if (size < KZ_RT_POINT_SIZE) return 0;
      q->index = g32(b + 8);
      q->kind = g32(b + 12);
      q->label = g32(b + 16);
      q->site = g32(b + 20);
      q->releases = g32(b + 24);
      q->occurrence = g32(b + 28);
      q->flags = g32(b + 32);
      q->hits = g32(b + 36);
      q->acted = g32(b + 40);
      q->satisfied = g32(b + 44);
      q->held_actor = g32(b + 48);
      q->holds = g32(b + 52);
      return 1;
    }
    case KZ_RT_REC_PLAN: {
      kz_rt_dec_plan *q = &it->u.plan;
      if (size < KZ_RT_PLAN_REC_FIXED) return 0;
      q->status = g32(b + 8);
      q->flags = g32(b + 12);
      q->nbytes = g32(b + 16);
      q->given = g32(b + 20);
      if (q->nbytes > size - KZ_RT_PLAN_REC_FIXED) return 0;
      q->bytes = b + KZ_RT_PLAN_REC_FIXED;
      return 1;
    }
    default:
      return -1;
  }
}

int kz_rt_decode(const unsigned char *p, size_t n, kz_rt_dec_fn fn, void *ctx, kz_rt_dec_stats *st) {
  kz_rt_dec_stats local;
  kz_rt_dec_item it;
  size_t off = KZ_RT_FILE_HEADER_BYTES;
  uint32_t crc = 0xFFFFFFFFu;
  if (st == NULL) st = &local;
  memset(st, 0, sizeof *st);
  if (n < KZ_RT_FILE_HEADER_BYTES || memcmp(p, KZ_RT_MAGIC, 8) != 0) return KZ_RT_DEC_EMAGIC;
  st->major = g16(p + 8);
  st->minor = g16(p + 10);
  st->dump_flags = g32(p + 12);
  if (st->major != KZ_RT_FORMAT_MAJOR) return KZ_RT_DEC_EVERSION;
  crc = crc32_update(crc, p, KZ_RT_FILE_HEADER_BYTES);
  while (off < n) {
    uint32_t size, type;
    int r;
    if (n - off < KZ_RT_REC_HEADER_BYTES) {
      st->partial++;
      return KZ_RT_DEC_ETRUNC;
    }
    size = g32(p + off);
    type = g16(p + off + 4);
    if (size < KZ_RT_REC_HEADER_BYTES || size % 4u != 0) return KZ_RT_DEC_EFORMAT;
    if (size > n - off) {
      st->partial++;
      return KZ_RT_DEC_ETRUNC;
    }
    if (type == KZ_RT_REC_END) {
      if (size < KZ_RT_END_SIZE) return KZ_RT_DEC_EFORMAT;
      memset(&it, 0, sizeof it);
      it.type = type;
      it.u.end.bytes = g64(p + off + 8);
      it.u.end.crc32 = g32(p + off + 16);
      it.u.end.crc_ok = it.u.end.bytes == off && it.u.end.crc32 == (crc ^ 0xFFFFFFFFu);
      it.offset = off;
      st->end_seen = 1;
      st->crc_ok = it.u.end.crc_ok;
      st->records++;
      if (fn != NULL && fn(ctx, &it) != 0) return KZ_RT_DEC_STOPPED;
      return KZ_RT_DEC_OK;
    }
    crc = crc32_update(crc, p + off, size);
    if (type == KZ_RT_REC_BUF) {
      uint32_t nb, boff;
      const unsigned char *bb;
      if (size < KZ_RT_BUF_FIXED) return KZ_RT_DEC_EFORMAT;
      nb = g32(p + off + 12);
      if (nb > size - KZ_RT_BUF_FIXED) return KZ_RT_DEC_EFORMAT;
      memset(&it, 0, sizeof it);
      it.type = type;
      it.u.buf.owner = g32(p + off + 8);
      it.u.buf.nbytes = nb;
      it.u.buf.cap = g32(p + off + 16);
      it.offset = off;
      st->records++;
      if (fn != NULL && fn(ctx, &it) != 0) return KZ_RT_DEC_STOPPED;
      {
        const uint32_t owner = it.u.buf.owner;
        bb = p + off + KZ_RT_BUF_FIXED;
        boff = 0;
        while (boff < nb) {
          uint32_t rs, rt;
          if (nb - boff < KZ_RT_REC_HEADER_BYTES) {
            st->partial++;
            break;
          }
          rs = g32(bb + boff);
          rt = g16(bb + boff + 4);
          if (rs == 0 || rs % 4u != 0 || rs > nb - boff) {
            st->partial++;
            break;
          }
          if (rt == KZ_RT_REC_PAD) {
            st->pads++;
          } else {
            r = buffer_record(bb + boff, rs, rt, g16(bb + boff + 6), owner, &it);
            if (r == 0) return KZ_RT_DEC_EFORMAT;
            if (r < 0) {
              st->unknown++;
            } else {
              it.offset = off + KZ_RT_BUF_FIXED + boff;
              st->records++;
              if (fn != NULL && fn(ctx, &it) != 0) return KZ_RT_DEC_STOPPED;
            }
          }
          boff += rs;
        }
      }
      off += size;
      continue;
    }
    r = stream_record(p + off, size, type, &it);
    if (r == 0) return KZ_RT_DEC_EFORMAT;
    if (r < 0) {
      st->unknown++;
    } else {
      it.offset = off;
      st->records++;
      if (fn != NULL && fn(ctx, &it) != 0) return KZ_RT_DEC_STOPPED;
    }
    off += size;
  }
  st->partial++; /* no END: the stream was cut */
  return KZ_RT_DEC_ETRUNC;
}

const char *kz_rt_dec_error(int code) {
  switch (code) {
    case KZ_RT_DEC_OK: return "ok";
    case KZ_RT_DEC_EMAGIC: return "not a kzrt-dump stream (magic)";
    case KZ_RT_DEC_EVERSION: return "unsupported format major version";
    case KZ_RT_DEC_ETRUNC: return "truncated stream (no END record)";
    case KZ_RT_DEC_EFORMAT: return "malformed record";
    case KZ_RT_DEC_STOPPED: return "stopped by the reader";
    default: return "unknown error";
  }
}

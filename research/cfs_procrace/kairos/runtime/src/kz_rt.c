/* kz_rt.c -- the Kairos instrumentation runtime core (see kz_rt.h, kz_rt_format.h and
 * kairos/docs/event_semantics.md).  C99 with the GCC/Clang __atomic builtins and __thread; no
 * library calls (only <stddef.h>/<stdint.h>); all memory is static.
 *
 * Concurrency model:
 *   - every run actor writes its records into chunks of one static pool; a chunk has one writer (its
 *     actor), a record never spans chunks, and the actor publishes each chunk's committed length and
 *     the link to its next chunk with release stores, which the dumper reads with acquire loads;
 *     taking a chunk is one atomic add on the pool counter (no lock, no wait; an empty pool or a
 *     reached quota drops the record and counts it);
 *   - the bootstrap (shared) buffer has many writers: a writer reserves bytes with an atomic
 *     add, writes the record, and commits it by storing its size word last (release);
 *   - control state is changed only under the control lock (the adapter's, or the runtime's own
 *     spin lock, fixed when the plan is loaded); observation hooks never take it;
 *   - none of these runtime orders is an order of the target program (event_semantics.md 2.10).
 */
#include <stddef.h>
#include <stdint.h>

#include "kz_rt.h"
#include "kz_rt_format.h"

#ifndef KZ_RT_HAVE_TLS
#if defined(__GNUC__) || defined(__clang__)
#define KZ_RT_HAVE_TLS 1
#else
#define KZ_RT_HAVE_TLS 0
#endif
#endif

#if (KZ_RT_CHUNK_BYTES % 4u) != 0 || (KZ_RT_BOOT_BUF_BYTES % 4u) != 0 || (KZ_RT_POOL_BYTES % KZ_RT_CHUNK_BYTES) != 0
#error "buffer sizes must be multiples of 4 and the pool a multiple of the chunk"
#endif
#if KZ_RT_CHUNK_BYTES < 256u || KZ_RT_POOL_BYTES < KZ_RT_CHUNK_BYTES
#error "a chunk must hold the largest record (184 bytes) and the pool at least one chunk"
#endif
#define POOL_CHUNKS (KZ_RT_POOL_BYTES / KZ_RT_CHUNK_BYTES)

#define LD(p) __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define LDR(p) __atomic_load_n((p), __ATOMIC_RELAXED)
#define ST(p, v) __atomic_store_n((p), (v), __ATOMIC_RELEASE)
#define STR_(p, v) __atomic_store_n((p), (v), __ATOMIC_RELAXED)
#define ADD(p, v) __atomic_fetch_add((p), (v), __ATOMIC_RELAXED)

#define CF_NOSLOT 1u     /* cell flag: no run slot (actor table full) */
#define CF_STR_TRUNC 2u  /* cell flag: the cached string reached its bound without a NUL */
#define NO_STR 0xFFu

/* ---- little-endian encoding ----------------------------------------------------------- */
static void put16(unsigned char *p, uint32_t v) {
  p[0] = (unsigned char)(v & 0xFFu);
  p[1] = (unsigned char)((v >> 8) & 0xFFu);
}
static void put32(unsigned char *p, uint32_t v) {
  put16(p, v & 0xFFFFu);
  put16(p + 2, v >> 16);
}
static void put64(unsigned char *p, uint64_t v) {
  put32(p, (uint32_t)(v & 0xFFFFFFFFu));
  put32(p + 4, (uint32_t)(v >> 32));
}
static void cpy(unsigned char *d, const unsigned char *s, uint32_t n) {
  uint32_t i;
  for (i = 0; i < n; ++i) d[i] = s[i];
}
static void zero(unsigned char *d, uint32_t n) {
  uint32_t i;
  for (i = 0; i < n; ++i) d[i] = 0;
}
static uint32_t pad4(uint32_t n) { return (n + 3u) & ~3u; }
/* The 32-bit word whose bytes in memory are v in little-endian order (host independent). */
static uint32_t le_word(uint32_t v) {
  uint32_t w = 0;
  put32((unsigned char *)&w, v);
  return w;
}

/* ---- state ------------------------------------------------------------------------------ */
typedef struct actor_slot {
  uint32_t state; /* 1 = in use (published) */
  uint32_t id;
  uint32_t boot_link, link_evidence;
  uint64_t native_key;
  uint32_t key_kind, key_generation;
  uint32_t first_aseq, last_aseq, recorded, overflow;
  uint32_t busy;      /* the owner is writing a record */
  uint32_t committed; /* committed bytes (all its chunks) */
  uint32_t first_chunk, cur_chunk; /* chunk index + 1 (0 = none); cur_chunk is the owner's only */
  uint32_t chunks, dropped_bytes, ovf_quota, ovf_pool;
  uint32_t exited;
  /* control (under the control lock) */
  uint32_t released;   /* set (release) when a release resumes it; the adapter waits on it */
  uint32_t held_point; /* point index + 1 */
  uint32_t resumed_by; /* the actor whose release resumed it */
  uint32_t ndeferred;
  uint32_t deferred[KZ_RT_MAX_POINTS];
  uint32_t def_site, def_bind, def_ev;
} actor_slot;

static uint32_t g_pool[KZ_RT_POOL_BYTES / 4u];
static uint32_t g_chunk_next[POOL_CHUNKS]; /* the next chunk (index + 1) of the same actor; 0 = none yet */
static uint32_t g_chunk_used[POOL_CHUNKS]; /* committed bytes of the chunk (whole records) */
static actor_slot g_actor[KZ_RT_MAX_ACTORS];
static uint32_t g_boot_buf[KZ_RT_BOOT_BUF_BYTES / 4u];

typedef struct note_entry {
  uint32_t used, kind, key, rsv;
  uint64_t count;
} note_entry;
typedef struct module_entry {
  uint32_t used, module, actor, count;
  uint64_t t;
  unsigned char sha[32];
} module_entry;

/* The mode word: the static default records from the first hook, before any setup call. */
static uint32_t g_mode = KZ_RT_MODE_RECORD;

static struct {
  uint32_t phase; /* KZ_RT_PHASE_* */
  const kz_rt_adapter *ad;
  uint32_t init_once, attach_once, early_once;
  uint32_t early_set, early_mode;   /* the latched mode of kz_rt_early_mode */
  uint32_t mode_source, window_mode, rx_flags;
  uint32_t actor_quota;             /* 0 = none */
  uint32_t pool_next, pool_dropped; /* chunks taken (may pass POOL_CHUNKS under a race); records dropped: no chunk */
  uint32_t boot_dropped;            /* bytes of the records the bootstrap buffer could not take */
  uint64_t run_id;
  uint32_t instance_id, clock_id;
  unsigned char manifest[32];
  char tags[3][KZ_RT_TAG_MAX + 1];
  uint32_t tag_len[3];
  uint64_t t_init, t_attach, t_stop;
  uint32_t run_next, boot_next, boot_reserved, boot_aseq;
  uint64_t loss[KZ_RT_LOSS_NOTE_TABLE + 1];
  note_entry notes[KZ_RT_MAX_NOTES];
  uint32_t notes_next;
  module_entry modules[KZ_RT_MAX_MODULES];
  uint32_t modules_next;
} g;

/* An actor's bound: its quota, else the pool. */
static uint32_t actor_bound(void) {
  const uint32_t q = LDR(&g.actor_quota);
  return q != 0 && q < KZ_RT_POOL_BYTES ? q : KZ_RT_POOL_BYTES;
}

static unsigned char *chunk_base(uint32_t idx) { return (unsigned char *)g_pool + (size_t)idx * KZ_RT_CHUNK_BYTES; }

/* The build identity linked into the executable, or NULL (event_semantics.md 8.5).  A weak reference:
 * no library symbol, and a binary without the build-identity object resolves it to 0. */
#if defined(__GNUC__) || defined(__clang__)
extern const kz_rt_build kz_rt_build_info __attribute__((weak));
#endif
static const kz_rt_build *build_info(void) {
#if defined(__GNUC__) || defined(__clang__)
  const kz_rt_build *b = &kz_rt_build_info;
  if (b != NULL && b->abi == KZ_RT_ABI_VERSION) return b;
#endif
  return NULL;
}

static int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

/* The build info's manifest sha256 as 32 bytes: 1 = given and valid, 0 = not given, -1 = malformed. */
static int build_manifest(const kz_rt_build *bi, unsigned char out[32]) {
  const char *h = bi != NULL ? bi->manifest_sha256_hex : NULL;
  uint32_t i;
  if (h == NULL) return 0;
  for (i = 0; i < 32u; ++i) {
    const int a = hex_digit(h[2u * i]), b = a < 0 ? -1 : hex_digit(h[2u * i + 1u]);
    if (a < 0 || b < 0) return -1;
    out[i] = (unsigned char)((a << 4) | b);
  }
  return h[64] == 0 ? 1 : -1;
}

static struct {
  uint32_t loaded, plan_once;
  uint32_t n;
  uint32_t adapter_lock; /* fixed at kz_rt_plan: 1 = the adapter's lock, 0 = the spin lock */
  uint32_t spin;
  uint32_t ctl_seq;
  kz_rt_point pt[KZ_RT_MAX_POINTS];
  uint32_t hits[KZ_RT_MAX_POINTS], acted[KZ_RT_MAX_POINTS], satisfied[KZ_RT_MAX_POINTS];
  uint32_t held[KZ_RT_MAX_POINTS], holds[KZ_RT_MAX_POINTS];
  uint32_t queue[KZ_RT_MAX_POINTS + 1];
  uint32_t qn;
} k;

#if KZ_RT_HAVE_TLS
/* initial-exec: the runtime is linked into the executable (not loaded later), so the cell needs
 * no call into the dynamic loader (__tls_get_addr) */
static __thread kz_rt_cell g_tls_cell __attribute__((tls_model("initial-exec")));
#endif

KZ_RT_API kz_rt_cell *kz_rt_tls_cell(void) {
#if KZ_RT_HAVE_TLS
  return &g_tls_cell;
#else
  return NULL;
#endif
}

static void loss(uint32_t kind, uint64_t n) {
  if (kind <= KZ_RT_LOSS_NOTE_TABLE) (void)ADD(&g.loss[kind], n);
}

static kz_rt_cell *self_cell(const kz_rt_adapter **adp) {
  const kz_rt_adapter *ad = LD(&g.ad);
  *adp = ad;
  if (ad != NULL) return ad->self_cell(ad->ctx);
  return kz_rt_tls_cell();
}

static uint64_t now(const kz_rt_adapter *ad, uint32_t *ef) {
  uint32_t clock = 0;
  if (ad == NULL || ad->now_ns == NULL || (ad->caps & KZ_RT_CAP_CLOCK) == 0) return 0;
  *ef |= KZ_RT_EF_CLOCK;
  return ad->now_ns(ad->ctx, &clock);
}

/* The run actor slot of the cell, allocating one after attach; NULL = the event goes to the
 * shared buffer under the cell's provisional id (before attach, or the table is full). */
static actor_slot *ensure_actor(kz_rt_cell *c, const kz_rt_adapter *ad, uint32_t *ef) {
  uint32_t a = c->actor, idx;
  actor_slot *s;
  if (a != 0 && (a & KZ_RT_ACTOR_PROVISIONAL) == 0) return &g_actor[a - 1u];
  if (ad == NULL || (c->flags & CF_NOSLOT) != 0) {
    if (a == 0) c->actor = KZ_RT_ACTOR_PROVISIONAL | (ADD(&g.boot_next, 1u) + 1u);
    *ef |= ad == NULL ? KZ_RT_EF_BOOT : KZ_RT_EF_NOSLOT;
    return NULL;
  }
  idx = ADD(&g.run_next, 1u);
  if (idx >= KZ_RT_MAX_ACTORS) {
    if (a == 0) c->actor = KZ_RT_ACTOR_PROVISIONAL | (ADD(&g.boot_next, 1u) + 1u);
    c->flags |= CF_NOSLOT;
    loss(KZ_RT_LOSS_ACTOR_TABLE, 1u);
    *ef |= KZ_RT_EF_NOSLOT;
    return NULL;
  }
  s = &g_actor[idx];
  s->id = idx + 1u;
  s->boot_link = a;
  s->link_evidence = a != 0 ? KZ_RT_LINK_SAME_CELL : KZ_RT_LINK_NONE;
  if (ad->native_key != NULL && (ad->caps & KZ_RT_CAP_NATIVE_KEY) != 0)
    s->native_key = ad->native_key(ad->ctx, &s->key_kind, &s->key_generation);
  s->first_aseq = c->aseq + 1u;
  ST(&s->state, 1u);
  c->actor = idx + 1u;
  return s;
}

/* ---- buffers ------------------------------------------------------------------------------ */
/* A record the actor could not keep: counted with its bytes (the demand) and the reason. */
static void actor_drop(actor_slot *s, uint32_t size, int pool) {
  STR_(&s->overflow, s->overflow + 1u);
  STR_(&s->dropped_bytes, s->dropped_bytes + size);
  if (pool) {
    STR_(&s->ovf_pool, s->ovf_pool + 1u);
    (void)ADD(&g.pool_dropped, 1u);
  } else {
    STR_(&s->ovf_quota, s->ovf_quota + 1u);
  }
}

static void actor_append(actor_slot *s, const unsigned char *rec, uint32_t size) {
  const uint32_t quota = LDR(&g.actor_quota), committed = s->committed, cur = s->cur_chunk;
  uint32_t idx, used;
  if (quota != 0 && (committed >= quota || size > quota - committed)) {
    actor_drop(s, size, 0);
    return;
  }
  used = cur != 0 ? LDR(&g_chunk_used[cur - 1u]) : KZ_RT_CHUNK_BYTES;
  if (size <= KZ_RT_CHUNK_BYTES - used) {
    cpy(chunk_base(cur - 1u) + used, rec, size);
    ST(&g_chunk_used[cur - 1u], used + size); /* commit in the chunk */
  } else {
    /* a new chunk: the record goes there whole; the link to it is published after its content */
    if (LDR(&g.pool_next) >= POOL_CHUNKS || (idx = ADD(&g.pool_next, 1u)) >= POOL_CHUNKS) {
      actor_drop(s, size, 1);
      return;
    }
    cpy(chunk_base(idx), rec, size);
    ST(&g_chunk_used[idx], size);
    if (cur != 0)
      ST(&g_chunk_next[cur - 1u], idx + 1u);
    else
      ST(&s->first_chunk, idx + 1u);
    s->cur_chunk = idx + 1u;
    STR_(&s->chunks, s->chunks + 1u);
  }
  ST(&s->committed, committed + size);
  STR_(&s->recorded, s->recorded + 1u);
}

static void boot_append(const unsigned char *rec, uint32_t size) {
  uint32_t off = LDR(&g.boot_reserved);
  unsigned char *base = (unsigned char *)g_boot_buf;
  if (off >= KZ_RT_BOOT_BUF_BYTES || size > KZ_RT_BOOT_BUF_BYTES - off) {
    loss(KZ_RT_LOSS_PREINIT, 1u);
    (void)ADD(&g.boot_dropped, size);
    return;
  }
  off = ADD(&g.boot_reserved, size);
  if (off >= KZ_RT_BOOT_BUF_BYTES || size > KZ_RT_BOOT_BUF_BYTES - off) {
    /* lost a race for the last bytes: fill them so that the reader can walk past */
    if (off < KZ_RT_BOOT_BUF_BYTES && KZ_RT_BOOT_BUF_BYTES - off >= KZ_RT_REC_HEADER_BYTES) {
      put16(base + off + 4u, KZ_RT_REC_PAD);
      put16(base + off + 6u, 0u);
      ST(&g_boot_buf[off / 4u], le_word(KZ_RT_BOOT_BUF_BYTES - off));
    }
    loss(KZ_RT_LOSS_PREINIT, 1u);
    (void)ADD(&g.boot_dropped, size);
    return;
  }
  cpy(base + off + 4u, rec + 4u, size - 4u);
  ST(&g_boot_buf[off / 4u], le_word(size)); /* commit: the size word last */
}

static void append(actor_slot *s, const unsigned char *rec, uint32_t size) {
  if (s != NULL)
    actor_append(s, rec, size);
  else
    boot_append(rec, size);
}

/* ---- records ------------------------------------------------------------------------------ */
#define EVENT_MAX (KZ_RT_EVENT_FIXED + 8u * KZ_RT_MAX_VALS + KZ_RT_STR_MAX)

static uint32_t enc_event(unsigned char *b, uint32_t ef, uint32_t actor, uint32_t aseq, uint32_t site, uint32_t bind, uint32_t link,
                          uint32_t valid, uint64_t t, uint64_t pc, uint32_t n, const uint64_t *v, uint32_t str_slot, uint32_t str_len,
                          const unsigned char *str) {
  const uint32_t size = KZ_RT_EVENT_FIXED + 8u * n + pad4(str_len);
  uint32_t i;
  put32(b, size);
  put16(b + 4, KZ_RT_REC_EVENT);
  put16(b + 6, ef);
  put32(b + 8, actor);
  put32(b + 12, aseq);
  put32(b + 16, site);
  put32(b + 20, bind);
  put32(b + 24, link);
  put32(b + 28, valid);
  put64(b + 32, t);
  put64(b + 40, pc);
  put16(b + 48, n);
  put16(b + 50, str_len);
  b[52] = (unsigned char)str_slot;
  b[53] = b[54] = b[55] = 0;
  for (i = 0; i < n; ++i) put64(b + KZ_RT_EVENT_FIXED + 8u * i, v[i]);
  cpy(b + KZ_RT_EVENT_FIXED + 8u * n, str, str_len);
  zero(b + KZ_RT_EVENT_FIXED + 8u * n + str_len, pad4(str_len) - str_len);
  return size;
}

static uint32_t unresolved_event(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid, uint32_t n, const uint64_t *v, uint64_t pc,
                                 const kz_rt_adapter *ad) {
  unsigned char rec[EVENT_MAX];
  uint32_t ef = KZ_RT_EF_UNRESOLVED | KZ_RT_EF_PC | (ad == NULL ? KZ_RT_EF_BOOT : 0u);
  const uint32_t aseq = ADD(&g.boot_aseq, 1u) + 1u;
  const uint64_t t = now(ad, &ef);
  const uint32_t size = enc_event(rec, ef, KZ_RT_ACTOR_UNRESOLVED, aseq, site, bind, link, valid, t, pc, n, v, NO_STR, 0, rec);
  loss(KZ_RT_LOSS_UNRESOLVED, 1u);
  boot_append(rec, size);
  return aseq;
}

static uint32_t obs_common(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid, uint32_t n, const uint64_t *v, uint32_t str_slot,
                           uint32_t str_bound, uint64_t pc) {
  const uint32_t mode = LDR(&g_mode);
  const kz_rt_adapter *ad;
  kz_rt_cell *c;
  actor_slot *s;
  unsigned char rec[EVENT_MAX];
  uint32_t ef = KZ_RT_EF_PC, aseq, i, size;
  uint64_t t;
  if (mode == KZ_RT_MODE_OFF) return 0;
  if (mode == KZ_RT_MODE_STOPPED) {
    loss(KZ_RT_LOSS_AFTER_STOP, 1u);
    return 0;
  }
  if (n > KZ_RT_MAX_VALS) n = KZ_RT_MAX_VALS;
  c = self_cell(&ad);
  if (c == NULL) return unresolved_event(site, bind, link, valid, n, v, pc, ad);
  if (c->depth != 0) {
    loss(KZ_RT_LOSS_REENTRY, 1u);
    return 0;
  }
  c->depth = 1;
  s = ensure_actor(c, ad, &ef);
  aseq = ++c->aseq;
  if (s != NULL) ST(&s->busy, 1u);
  t = now(ad, &ef);
  /* the last event's values, for a control condition at this hit */
  c->last_aseq = aseq;
  c->last_site = site;
  c->last_bind = bind;
  c->last_valid = valid;
  c->last_nvals = n;
  for (i = 0; i < n; ++i) c->last_v[i] = v[i];
  c->last_str_slot = NO_STR;
  c->last_str_len = 0;
  c->flags &= ~CF_STR_TRUNC;
  if (str_slot < n && (valid & (1u << str_slot)) != 0 && v[str_slot] != 0) {
    const unsigned char *p = (const unsigned char *)(uintptr_t)v[str_slot];
    const uint32_t bound = str_bound < KZ_RT_STR_MAX ? str_bound : KZ_RT_STR_MAX;
    uint32_t len = 0;
    while (len < bound && p[len] != 0) {
      c->last_str[len] = p[len];
      ++len;
    }
    c->last_str_slot = str_slot;
    c->last_str_len = len;
    ef |= KZ_RT_EF_STR;
    if (len == bound) {
      ef |= KZ_RT_EF_STR_TRUNC;
      c->flags |= CF_STR_TRUNC;
    }
  }
  size = enc_event(rec, ef, c->actor, aseq, site, bind, link, valid, t, pc, n, v, c->last_str_slot, c->last_str_len, c->last_str);
  append(s, rec, size);
  if (s != NULL) {
    STR_(&s->last_aseq, aseq);
    ST(&s->busy, 0u);
  }
  c->depth = 0;
  return aseq;
}

KZ_RT_API uint32_t kz_rt_obs(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid, uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3) {
  uint64_t v[4];
  v[0] = v0;
  v[1] = v1;
  v[2] = v2;
  v[3] = v3;
  return obs_common(site, bind, link, valid, 4u, v, NO_STR, 0, (uint64_t)(uintptr_t)__builtin_return_address(0));
}

KZ_RT_API uint32_t kz_rt_obsv(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid, uint32_t n, const uint64_t *v) {
  static const uint64_t none[1] = {0};
  if (v == NULL) n = 0;
  return obs_common(site, bind, link, valid, n, v != NULL ? v : none, NO_STR, 0, (uint64_t)(uintptr_t)__builtin_return_address(0));
}

KZ_RT_API uint32_t kz_rt_obs_str(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid, uint64_t v0, uint64_t v1, uint64_t v2,
                                 uint64_t v3, uint32_t str_slot, uint32_t str_bound) {
  uint64_t v[4];
  v[0] = v0;
  v[1] = v1;
  v[2] = v2;
  v[3] = v3;
  return obs_common(site, bind, link, valid, 4u, v, str_slot, str_bound, (uint64_t)(uintptr_t)__builtin_return_address(0));
}

/* ---- lifecycle -------------------------------------------------------------------------------- */
static void copy_tag(int which, const char *s) {
  uint32_t n = 0;
  if (s != NULL)
    while (n < KZ_RT_TAG_MAX && s[n] != 0) {
      g.tags[which][n] = s[n];
      ++n;
    }
  g.tags[which][n] = 0;
  g.tag_len[which] = n;
}

/* Bounded text equality (the RUN tags keep at most KZ_RT_TAG_MAX bytes). */
static int tag_eq(const char *a, const char *b) {
  uint32_t i;
  for (i = 0; i < KZ_RT_TAG_MAX; ++i) {
    if (a[i] != b[i]) return 0;
    if (a[i] == 0) return 1;
  }
  return 1;
}
static int tag_given(const char *s) { return s != NULL && s[0] != 0; }
static int bytes_eq(const unsigned char *a, const unsigned char *b, uint32_t n) {
  uint32_t i;
  for (i = 0; i < n; ++i)
    if (a[i] != b[i]) return 0;
  return 1;
}

/* Did any hook run (under the static default) before this point?  Its traces: bootstrap bytes or
 * losses, an unresolved or re-entered hook, a run actor. */
static int hooks_ran(void) {
  return LDR(&g.boot_reserved) != 0 || LDR(&g.run_next) != 0 || LDR(&g.loss[KZ_RT_LOSS_PREINIT]) != 0 ||
         LDR(&g.loss[KZ_RT_LOSS_UNRESOLVED]) != 0 || LDR(&g.loss[KZ_RT_LOSS_REENTRY]) != 0;
}

KZ_RT_API int kz_rt_early_mode(uint32_t mode, uint32_t note) {
  uint32_t f = 0;
  if (mode != KZ_RT_MODE_OFF && mode != KZ_RT_MODE_RECORD && mode != KZ_RT_MODE_CONTROL) return KZ_RT_EINVAL;
  if (LD(&g.init_once) != 0) return KZ_RT_ESTATE;
  if (__atomic_exchange_n(&g.early_once, 1u, __ATOMIC_ACQ_REL) != 0) return KZ_RT_ESTATE;
  if (hooks_ran()) f |= KZ_RT_RX_HOOKS_BEFORE_EARLY;
  if ((note & KZ_RT_EARLY_DEFAULTED) != 0) f |= KZ_RT_RX_MODE_DEFAULTED;
  if ((note & KZ_RT_EARLY_INVALID) != 0) f |= KZ_RT_RX_MODE_INVALID;
  (void)__atomic_fetch_or(&g.rx_flags, f, __ATOMIC_RELAXED);
  g.early_mode = mode;
  g.mode_source = KZ_RT_MODE_SRC_EARLY;
  ST(&g_mode, mode);
  ST(&g.early_set, 1u);
  return KZ_RT_OK;
}

KZ_RT_API int kz_rt_init(const kz_rt_config *cfg) {
  const kz_rt_build *bi = build_info();
  uint32_t ef = 0;
  if (cfg == NULL) return KZ_RT_EINVAL;
  if (cfg->abi != KZ_RT_ABI_VERSION) return KZ_RT_EABI;
  if (cfg->mode != KZ_RT_MODE_OFF && cfg->mode != KZ_RT_MODE_RECORD && cfg->mode != KZ_RT_MODE_CONTROL) return KZ_RT_EINVAL;
  if (LD(&g.early_set) != 0 && cfg->mode != g.early_mode) {
    (void)__atomic_fetch_or(&g.rx_flags, KZ_RT_RX_INIT_REFUSED, __ATOMIC_RELAXED);
    return KZ_RT_EINVAL; /* one mode per process: the latched early mode */
  }
  if (__atomic_exchange_n(&g.init_once, 1u, __ATOMIC_ACQ_REL) != 0) return KZ_RT_ESTATE;
  g.run_id = cfg->run_id;
  g.instance_id = cfg->instance_id;
  STR_(&g.actor_quota, cfg->actor_buf_bytes & ~3u);
  if (cfg->manifest_sha256 != NULL) cpy(g.manifest, cfg->manifest_sha256, 32u);
  copy_tag(0, cfg->backend);
  copy_tag(1, cfg->compiler);
  copy_tag(2, cfg->target);
  if (bi != NULL) { /* the binary's identity is kept; a differing harness value is a conflict */
    unsigned char bm[32];
    const int have_bm = build_manifest(bi, bm);
    if ((tag_given(cfg->backend) && tag_given(bi->backend) && !tag_eq(cfg->backend, bi->backend)) ||
        (tag_given(cfg->compiler) && tag_given(bi->compiler) && !tag_eq(cfg->compiler, bi->compiler)) ||
        (tag_given(cfg->target) && tag_given(bi->target) && !tag_eq(cfg->target, bi->target)) ||
        (cfg->manifest_sha256 != NULL && have_bm == 1 && !bytes_eq(cfg->manifest_sha256, bm, 32u)))
      (void)__atomic_fetch_or(&g.rx_flags, KZ_RT_RX_TAG_CONFLICT, __ATOMIC_RELAXED);
  }
  g.t_init = now(LD(&g.ad), &ef);
  (void)__atomic_fetch_or(&g.phase, KZ_RT_PHASE_INIT, __ATOMIC_ACQ_REL);
  if (LD(&g.early_set) == 0) {
    g.mode_source = KZ_RT_MODE_SRC_INIT;
    ST(&g_mode, cfg->mode);
  }
  return KZ_RT_OK;
}

KZ_RT_API int kz_rt_attach(const kz_rt_adapter *ad) {
  if (ad == NULL || ad->self_cell == NULL) return KZ_RT_EINVAL;
  if (ad->abi != KZ_RT_ABI_VERSION) return KZ_RT_EABI;
  if (__atomic_exchange_n(&g.attach_once, 1u, __ATOMIC_ACQ_REL) != 0) return KZ_RT_ESTATE;
  if (ad->now_ns != NULL && (ad->caps & KZ_RT_CAP_CLOCK) != 0) {
    g.t_attach = ad->now_ns(ad->ctx, &g.clock_id);
  }
  ST(&g.ad, ad);
  (void)__atomic_fetch_or(&g.phase, KZ_RT_PHASE_ATTACH, __ATOMIC_ACQ_REL);
  return KZ_RT_OK;
}

/* Ends the observation window.  A RECORD/CONTROL window becomes STOPPED (hooks only count); OFF stays
 * OFF (its hooks never count).  The window's mode is kept for the dump (RUNX window_mode). */
KZ_RT_API void kz_rt_stop(void) {
  uint32_t ef = 0;
  const uint32_t m = LDR(&g_mode);
  g.t_stop = now(LD(&g.ad), &ef);
  (void)__atomic_fetch_or(&g.phase, KZ_RT_PHASE_STOP, __ATOMIC_ACQ_REL);
  if (m == KZ_RT_MODE_RECORD || m == KZ_RT_MODE_CONTROL) {
    g.window_mode = m;
    ST(&g_mode, (uint32_t)KZ_RT_MODE_STOPPED);
  }
}

KZ_RT_API void kz_rt_module(uint32_t module, const uint8_t *sha) {
  static const unsigned char none[32] = {0};
  const unsigned char *key = sha != NULL ? sha : none;
  uint32_t n = LD(&g.modules_next), idx, ef = 0;
  const kz_rt_adapter *ad;
  kz_rt_cell *c;
  module_entry *m;
  if (n > KZ_RT_MAX_MODULES) n = KZ_RT_MAX_MODULES;
  /* the same (module, sha256) again (one constructor per unit): count it.  Two first registrations
   * racing on different threads can both take an entry; the reader adds such entries up. */
  for (idx = 0; idx < n; ++idx) {
    m = &g.modules[idx];
    if (LD(&m->used) != 0 && m->module == module && bytes_eq(m->sha, key, 32u)) {
      (void)ADD(&m->count, 1u);
      return;
    }
  }
  idx = ADD(&g.modules_next, 1u);
  if (idx >= KZ_RT_MAX_MODULES) {
    loss(KZ_RT_LOSS_MODULE_TABLE, 1u);
    return;
  }
  m = &g.modules[idx];
  c = self_cell(&ad);
  m->module = module;
  m->actor = c != NULL ? c->actor : 0u;
  m->t = now(ad, &ef);
  cpy(m->sha, key, 32u);
  STR_(&m->count, 1u);
  ST(&m->used, 1u);
}

KZ_RT_API void kz_rt_note_loss(uint32_t kind, uint32_t key, uint32_t count) {
  uint32_t n = LD(&g.notes_next), i;
  if (n > KZ_RT_MAX_NOTES) n = KZ_RT_MAX_NOTES;
  for (i = 0; i < n; ++i) {
    note_entry *e = &g.notes[i];
    if (LD(&e->used) != 0 && e->kind == kind && e->key == key) {
      (void)ADD(&e->count, (uint64_t)count);
      return;
    }
  }
  i = ADD(&g.notes_next, 1u);
  if (i >= KZ_RT_MAX_NOTES) {
    loss(KZ_RT_LOSS_NOTE_TABLE, 1u);
    return;
  }
  g.notes[i].kind = kind;
  g.notes[i].key = key;
  STR_(&g.notes[i].count, (uint64_t)count);
  ST(&g.notes[i].used, 1u);
}

KZ_RT_API void kz_rt_actor_exit(void) {
  const kz_rt_adapter *ad;
  kz_rt_cell *c = self_cell(&ad);
  actor_slot *s;
  unsigned char rec[KZ_RT_AEXIT_SIZE];
  uint32_t ef = 0;
  uint64_t t;
  if (LDR(&g_mode) == KZ_RT_MODE_OFF || c == NULL || c->depth != 0 || c->actor == 0) return;
  s = (c->actor & KZ_RT_ACTOR_PROVISIONAL) == 0 ? &g_actor[c->actor - 1u] : NULL;
  t = now(ad, &ef);
  put32(rec, KZ_RT_AEXIT_SIZE);
  put16(rec + 4, KZ_RT_REC_AEXIT);
  put16(rec + 6, ef);
  put32(rec + 8, c->actor);
  put32(rec + 12, ++c->aseq);
  put64(rec + 16, t);
  if (s != NULL) {
    ST(&s->busy, 1u);
    append(s, rec, KZ_RT_AEXIT_SIZE);
    STR_(&s->last_aseq, c->aseq);
    ST(&s->exited, 1u);
    ST(&s->busy, 0u);
  } else {
    append(NULL, rec, KZ_RT_AEXIT_SIZE);
  }
}

/* ---- control (kzp-hold 1 semantics; event_semantics.md section 4) ------------------------------- */
static void ctl_lock(const kz_rt_adapter *ad) {
  if (k.adapter_lock != 0) {
    ad->ctl_lock(ad->ctx);
    return;
  }
  while (__atomic_exchange_n(&k.spin, 1u, __ATOMIC_ACQUIRE) != 0)
    while (LDR(&k.spin) != 0) {
    }
}
static void ctl_unlock(const kz_rt_adapter *ad) {
  if (k.adapter_lock != 0) {
    ad->ctl_unlock(ad->ctx);
    return;
  }
  __atomic_store_n(&k.spin, 0u, __ATOMIC_RELEASE);
}

KZ_RT_API int kz_rt_plan(const kz_rt_point *pts, uint32_t n) {
  const kz_rt_adapter *ad = LD(&g.ad);
  uint32_t i;
  if (n > KZ_RT_MAX_POINTS || (n > 0 && pts == NULL)) return KZ_RT_EINVAL;
  for (i = 0; i < n; ++i) {
    const kz_rt_point *p = &pts[i];
    if (p->kind != KZ_RT_POINT_GATE && p->kind != KZ_RT_POINT_RELEASE) return KZ_RT_EINVAL;
    if (p->occurrence > 1000000u) return KZ_RT_EINVAL; /* kzp-hold 1: n in 1..1000000 */
    if (p->kind == KZ_RT_POINT_RELEASE && (p->releases >= n || pts[p->releases].kind != KZ_RT_POINT_GATE)) return KZ_RT_EINVAL;
    if (p->kind == KZ_RT_POINT_RELEASE && (p->flags & (KZ_RT_PT_UNLESS_RELEASED | KZ_RT_PT_ALLOW_LOCK_HELD | KZ_RT_PT_ALLOW_OSAL_INTERNAL)) != 0)
      return KZ_RT_EINVAL; /* these are forms of a gate */
    if ((p->flags & KZ_RT_PT_UNLESS_RELEASED) != 0 && p->occurrence == 0) return KZ_RT_EINVAL; /* not with each */
    if (p->bind_n > KZ_RT_POINT_BINDS || p->str_len > KZ_RT_STR_MAX) return KZ_RT_EINVAL;
    if (p->str_slot >= (int32_t)KZ_RT_MAX_VALS || p->val_slot >= (int32_t)KZ_RT_MAX_VALS) return KZ_RT_EINVAL;
  }
  if (__atomic_exchange_n(&k.plan_once, 1u, __ATOMIC_ACQ_REL) != 0) return KZ_RT_ESTATE;
  for (i = 0; i < n; ++i) cpy((unsigned char *)&k.pt[i], (const unsigned char *)&pts[i], (uint32_t)sizeof(kz_rt_point));
  k.n = n;
  k.adapter_lock = ad != NULL && ad->ctl_lock != NULL && ad->ctl_unlock != NULL && (ad->caps & KZ_RT_CAP_LOCK) != 0 ? 1u : 0u;
  (void)__atomic_fetch_or(&g.phase, KZ_RT_PHASE_PLAN, __ATOMIC_ACQ_REL);
  ST(&k.loaded, 1u);
  return KZ_RT_OK;
}

/* The deciding thread's control record (its own buffer, or the shared one). */
static void ctl_record(kz_rt_cell *c, actor_slot *s, const kz_rt_adapter *ad, uint32_t what, uint32_t reason, uint32_t site, uint32_t bind,
                       uint32_t ev, uint32_t point, uint32_t hit, uint32_t subject, uint32_t other, uint32_t gate, uint32_t ctx) {
  unsigned char rec[KZ_RT_CTL_SIZE];
  uint32_t ef = 0;
  const uint64_t t = now(ad, &ef);
  const uint32_t aseq = ++c->aseq;
  put32(rec, KZ_RT_CTL_SIZE);
  put16(rec + 4, KZ_RT_REC_CTL);
  put16(rec + 6, what);
  put32(rec + 8, c->actor);
  put32(rec + 12, aseq);
  put32(rec + 16, site);
  put32(rec + 20, bind);
  put32(rec + 24, ev);
  put32(rec + 28, ++k.ctl_seq);
  put64(rec + 32, t);
  put32(rec + 40, point);
  put32(rec + 44, hit);
  put32(rec + 48, subject);
  put32(rec + 52, other);
  put32(rec + 56, gate);
  put16(rec + 60, reason);
  put16(rec + 62, ctx);
  if (s != NULL) ST(&s->busy, 1u);
  append(s, rec, KZ_RT_CTL_SIZE);
  if (s != NULL) {
    STR_(&s->last_aseq, aseq);
    ST(&s->busy, 0u);
  }
}

/* Count a qualifying hit of point i; 1 if the point acts on it (first / n-th once, each always). */
static int count_hit(uint32_t i) {
  const uint32_t h = ++k.hits[i];
  if (k.pt[i].occurrence == 0) return 1;
  if (k.acted[i] != 0 || h != k.pt[i].occurrence) return 0;
  k.acted[i] = 1;
  return 1;
}

static int qualifies(uint32_t i, uint32_t site, uint32_t bind, const kz_rt_cell *c, int cond_ok) {
  const kz_rt_point *p = &k.pt[i];
  uint32_t j;
  if (p->site != site) return 0;
  if (p->bind_n > 0) {
    for (j = 0; j < p->bind_n && p->bind[j] != bind; ++j) {
    }
    if (j == p->bind_n) return 0;
  }
  if (p->val_slot < 0 && p->str_slot < 0) return 1;
  if (!cond_ok) {
    loss(KZ_RT_LOSS_CTL_NOCOND, 1u);
    return 0;
  }
  if (p->val_slot >= 0) {
    const uint32_t vs = (uint32_t)p->val_slot;
    if (vs >= c->last_nvals || (c->last_valid & (1u << vs)) == 0) {
      loss(KZ_RT_LOSS_CTL_NOCOND, 1u);
      return 0;
    }
    if (c->last_v[vs] != p->val) return 0;
  }
  if (p->str_slot >= 0) {
    if (c->last_str_slot != (uint32_t)p->str_slot) {
      loss(KZ_RT_LOSS_CTL_NOCOND, 1u);
      return 0;
    }
    if ((c->flags & CF_STR_TRUNC) != 0 && p->str_len >= c->last_str_len) {
      loss(KZ_RT_LOSS_CTL_NOCOND, 1u); /* the copy stopped at its bound: equality is not known */
      return 0;
    }
    if (c->last_str_len != p->str_len) return 0;
    for (j = 0; j < p->str_len; ++j)
      if (c->last_str[j] != p->str[j]) return 0;
  }
  return 1;
}

/* A release point acts: resume the thread its gate holds, or satisfy / consume. */
static void fire(uint32_t i, kz_rt_cell *c, actor_slot *s, const kz_rt_adapter *ad, uint32_t subject, uint32_t site, uint32_t bind,
                 uint32_t ev, uint32_t deferred, uint32_t ctx) {
  const uint32_t gate = k.pt[i].releases, h = k.held[gate];
  const uint32_t dflag = deferred ? KZ_RT_CR_DEFERRED : 0u;
  if (h != 0) {
    actor_slot *hs = &g_actor[h - 1u];
    k.held[gate] = 0;
    STR_(&hs->held_point, 0u);
    hs->resumed_by = c->actor;
    ctl_record(c, s, ad, KZ_RT_CW_FIRE, KZ_RT_CR_RESUMED | dflag, site, bind, ev, i, k.hits[i], subject, h, gate, ctx);
    ST(&hs->released, 1u);
    ad->hold_wake(ad->ctx, h - 1u);
    k.queue[k.qn++] = h;
    return;
  }
  if ((k.pt[gate].flags & KZ_RT_PT_UNLESS_RELEASED) != 0 && k.acted[gate] == 0) {
    k.acted[gate] = 1;
    k.satisfied[gate] = 1;
    ctl_record(c, s, ad, KZ_RT_CW_FIRE, KZ_RT_CR_SATISFIED | dflag, site, bind, ev, i, k.hits[i], subject, 0, gate, ctx);
    return;
  }
  ctl_record(c, s, ad, KZ_RT_CW_FIRE, KZ_RT_CR_CONSUMED | dflag, site, bind, ev, i, k.hits[i], subject, 0, gate, ctx);
}

/* The releases of the hit at which each resumed thread was held act now, in resume order. */
static void drain(kz_rt_cell *c, actor_slot *s, const kz_rt_adapter *ad, uint32_t ctx) {
  uint32_t qi = 0, j;
  while (qi < k.qn) {
    actor_slot *hs = &g_actor[k.queue[qi++] - 1u];
    const uint32_t nd = hs->ndeferred;
    hs->ndeferred = 0;
    for (j = 0; j < nd; ++j)
      if (count_hit(hs->deferred[j])) fire(hs->deferred[j], c, s, ad, hs->id, hs->def_site, hs->def_bind, hs->def_ev, 1u, ctx);
  }
  k.qn = 0;
}

static uint32_t hold_refusal(uint32_t i, uint32_t ctx, const actor_slot *s, const kz_rt_adapter *ad) {
  const uint32_t f = k.pt[i].flags;
  if (ad == NULL) return KZ_RT_CR_PRE_ATTACH;
  if (s == NULL) return KZ_RT_CR_NO_SLOT;
  if ((ad->caps & KZ_RT_CAP_WAIT) == 0 || ad->hold_wait == NULL || ad->hold_wake == NULL) return KZ_RT_CR_NO_WAIT;
  if (ctx == KZ_RT_CTX_TASK) return KZ_RT_CR_NONE;
  if (ctx == KZ_RT_CTX_LOCK_HELD && (f & KZ_RT_PT_ALLOW_LOCK_HELD) != 0) return KZ_RT_CR_NONE;
  if (ctx == KZ_RT_CTX_OSAL_INTERNAL && (f & KZ_RT_PT_ALLOW_OSAL_INTERNAL) != 0) return KZ_RT_CR_NONE;
  return KZ_RT_CR_CONTEXT;
}

KZ_RT_API int kz_rt_ctl(uint32_t site, uint32_t bind, uint32_t ev, uint32_t ctx) {
  const uint32_t mode = LDR(&g_mode);
  const kz_rt_adapter *ad;
  kz_rt_cell *c;
  actor_slot *s;
  uint32_t i, ef = 0, any = 0, gate_held = 0, held_i = 0, refused = 0;
  int cond_ok, result = KZ_RT_CTL_PASS;
  if (mode != KZ_RT_MODE_CONTROL) {
    if (mode == KZ_RT_MODE_STOPPED) loss(KZ_RT_LOSS_AFTER_STOP, 1u);
    return KZ_RT_CTL_PASS;
  }
  if (LD(&k.loaded) == 0) return KZ_RT_CTL_PASS;
  for (i = 0; i < k.n && !any; ++i) any = k.pt[i].site == site;
  if (!any) return KZ_RT_CTL_PASS;
  if (ctx == KZ_RT_CTX_ISR) {
    loss(KZ_RT_LOSS_CTL_SKIPPED, 1u);
    return KZ_RT_CTL_PASS;
  }
  c = self_cell(&ad);
  if (c == NULL) {
    loss(KZ_RT_LOSS_CTL_SKIPPED, 1u);
    return KZ_RT_CTL_PASS;
  }
  if (c->depth != 0) {
    loss(KZ_RT_LOSS_REENTRY, 1u);
    return KZ_RT_CTL_PASS;
  }
  if (ctx == KZ_RT_CTX_OSAL_INTERNAL && k.adapter_lock != 0 && (ad->caps & KZ_RT_CAP_LOCK_PRIVATE) == 0) {
    loss(KZ_RT_LOSS_CTL_SKIPPED, 1u);
    return KZ_RT_CTL_PASS;
  }
  c->depth = 1;
  s = ensure_actor(c, ad, &ef);
  cond_ok = ev != 0 && c->last_aseq == ev && c->last_site == site && c->last_bind == bind;
  ctl_lock(ad);
  /* gates first, in plan order: the first that acts holds the thread */
  for (i = 0; i < k.n; ++i) {
    uint32_t why;
    if (k.pt[i].kind != KZ_RT_POINT_GATE || !qualifies(i, site, bind, c, cond_ok) || !count_hit(i)) continue;
    if (gate_held) {
      ctl_record(c, s, ad, KZ_RT_CW_GATE_CONSUMED, KZ_RT_CR_NONE, site, bind, ev, i, k.hits[i], c->actor, 0, held_i, ctx);
      continue;
    }
    if (k.held[i] != 0) {
      ctl_record(c, s, ad, KZ_RT_CW_GATE_BUSY, KZ_RT_CR_NONE, site, bind, ev, i, k.hits[i], c->actor, k.held[i], i, ctx);
      continue;
    }
    why = hold_refusal(i, ctx, s, ad);
    if (why != KZ_RT_CR_NONE) {
      ctl_record(c, s, ad, KZ_RT_CW_GATE_REFUSED, why, site, bind, ev, i, k.hits[i], c->actor, 0, i, ctx);
      refused = 1;
      continue;
    }
    gate_held = 1;
    held_i = i;
    k.held[i] = s->id;
    k.holds[i]++;
    STR_(&s->held_point, i + 1u);
    s->ndeferred = 0;
    s->def_site = site;
    s->def_bind = bind;
    s->def_ev = ev;
    ST(&s->released, 0u);
    ctl_record(c, s, ad, KZ_RT_CW_GATE_HOLD, KZ_RT_CR_NONE, site, bind, ev, i, k.hits[i], c->actor, 0, i, ctx);
  }
  /* then the releases; those of a held hit act when its thread is resumed */
  for (i = 0; i < k.n; ++i) {
    if (k.pt[i].kind != KZ_RT_POINT_RELEASE || !qualifies(i, site, bind, c, cond_ok)) continue;
    if (gate_held) {
      s->deferred[s->ndeferred++] = i;
      continue;
    }
    if (count_hit(i)) fire(i, c, s, ad, c->actor, site, bind, ev, 0u, ctx);
  }
  drain(c, s, ad, ctx);
  ctl_unlock(ad);
  if (!gate_held) {
    c->depth = 0;
    return refused ? KZ_RT_CTL_REFUSED : KZ_RT_CTL_PASS;
  }
  (void)ad->hold_wait(ad->ctx, s->id - 1u, &s->released, k.pt[held_i].timeout_ns);
  ctl_lock(ad);
  if (LD(&s->released) != 0) {
    ctl_record(c, s, ad, KZ_RT_CW_RESUME, KZ_RT_CR_NONE, site, bind, ev, held_i, k.hits[held_i], c->actor, s->resumed_by, held_i, ctx);
    result = KZ_RT_CTL_RESUMED;
  } else {
    /* the timeout passed: proceed unreleased; the releases of this hit act now */
    k.held[held_i] = 0;
    STR_(&s->held_point, 0u);
    ctl_record(c, s, ad, KZ_RT_CW_TIMEOUT, KZ_RT_CR_NONE, site, bind, ev, held_i, k.hits[held_i], c->actor, 0, held_i, ctx);
    k.queue[k.qn++] = s->id;
    drain(c, s, ad, ctx);
    result = KZ_RT_CTL_TIMEOUT;
  }
  ctl_unlock(ad);
  c->depth = 0;
  return result;
}

KZ_RT_API int kz_rt_ctl_state(uint32_t point, kz_rt_point_state *out) {
  const kz_rt_adapter *ad = LD(&g.ad);
  if (out == NULL || LD(&k.loaded) == 0 || point >= k.n) return KZ_RT_EINVAL;
  ctl_lock(ad);
  out->hits = k.hits[point];
  out->acted = k.acted[point];
  out->satisfied = k.satisfied[point];
  out->held_actor = k.held[point];
  out->holds = k.holds[point];
  ctl_unlock(ad);
  return KZ_RT_OK;
}

/* ---- capabilities and dump ------------------------------------------------------------------- */
static uint32_t holdable_mask(const kz_rt_adapter *ad) {
  uint32_t m = 0;
  if (ad == NULL || (ad->caps & KZ_RT_CAP_WAIT) == 0 || ad->hold_wait == NULL || ad->hold_wake == NULL) return 0;
  m |= 1u << KZ_RT_CTX_TASK;
  m |= 1u << KZ_RT_CTX_LOCK_HELD; /* only at points that allow it */
  if ((ad->caps & KZ_RT_CAP_LOCK) == 0 || ad->ctl_lock == NULL || (ad->caps & KZ_RT_CAP_LOCK_PRIVATE) != 0) m |= 1u << KZ_RT_CTX_OSAL_INTERNAL;
  return m;
}

KZ_RT_API void kz_rt_caps_get(kz_rt_caps *out) {
  const kz_rt_adapter *ad = LD(&g.ad);
  if (out == NULL) return;
  out->abi = KZ_RT_ABI_VERSION;
  out->format_major = KZ_RT_FORMAT_MAJOR;
  out->format_minor = KZ_RT_FORMAT_MINOR;
  out->max_actors = KZ_RT_MAX_ACTORS;
  out->actor_buf_bytes = actor_bound();
  out->boot_buf_bytes = KZ_RT_BOOT_BUF_BYTES;
  out->max_points = KZ_RT_MAX_POINTS;
  out->max_vals = KZ_RT_MAX_VALS;
  out->str_max = KZ_RT_STR_MAX;
  out->max_modules = KZ_RT_MAX_MODULES;
  out->adapter_caps = ad != NULL ? ad->caps : 0u;
  out->holdable_ctx_mask = holdable_mask(ad);
  out->gates_per_actor = 1u;
  out->distinct_sites = KZ_RT_MAX_POINTS;
}

typedef struct sink {
  kz_rt_write_fn w;
  void *ctx;
  uint64_t bytes;
  uint32_t crc;
  int err;
} sink;

static void emit(sink *o, const unsigned char *p, uint32_t n) {
  uint32_t i, b;
  if (o->err || n == 0) return;
  for (i = 0; i < n; ++i) {
    o->crc ^= p[i];
    for (b = 0; b < 8u; ++b) o->crc = (o->crc >> 1) ^ (0xEDB88320u & (0u - (o->crc & 1u)));
  }
  o->bytes += n;
  if (o->w(o->ctx, p, n) != 0) o->err = 1;
}

static void rec_header(unsigned char *b, uint32_t size, uint32_t type, uint32_t sub) {
  put32(b, size);
  put16(b + 4, type);
  put16(b + 6, sub);
}

static uint32_t put_text(unsigned char *b, const char *s, uint32_t n) {
  put16(b, n);
  cpy(b + 2, (const unsigned char *)s, n);
  return 2u + n;
}

static void emit_loss(sink *o, uint32_t kind, uint32_t actor, uint32_t key, uint64_t count) {
  unsigned char b[KZ_RT_LOSS_SIZE];
  if (count == 0) return;
  rec_header(b, KZ_RT_LOSS_SIZE, KZ_RT_REC_LOSS, 0);
  put32(b + 8, kind);
  put32(b + 12, actor);
  put32(b + 16, key);
  put32(b + 20, 0);
  put64(b + 24, count);
  emit(o, b, KZ_RT_LOSS_SIZE);
}

/* The committed prefix of the shared buffer; *partial = 1 if it ends at an uncommitted record. */
static uint32_t boot_extent(uint32_t *partial) {
  uint32_t reserved = LD(&g.boot_reserved), off = 0;
  *partial = 0;
  if (reserved > KZ_RT_BOOT_BUF_BYTES) reserved = KZ_RT_BOOT_BUF_BYTES;
  while (off + KZ_RT_REC_HEADER_BYTES <= reserved) {
    uint32_t w = LD(&g_boot_buf[off / 4u]), size;
    const unsigned char *q = (const unsigned char *)&w;
    size = (uint32_t)q[0] | ((uint32_t)q[1] << 8) | ((uint32_t)q[2] << 16) | ((uint32_t)q[3] << 24);
    if (size == 0 || size % 4u != 0 || size > reserved - off) {
      *partial = 1;
      break;
    }
    off += size;
  }
  return off;
}

static uint32_t text_len(const char *s) {
  uint32_t n = 0;
  if (s != NULL)
    while (n < KZ_RT_TAG_MAX && s[n] != 0) ++n;
  return n;
}

KZ_RT_API int kz_rt_dump(kz_rt_write_fn write, void *wctx) {
  const kz_rt_adapter *ad = LD(&g.ad);
  const kz_rt_build *bi = build_info();
  unsigned char b[KZ_RT_RUN_FIXED + 5u * (2u + KZ_RT_TAG_MAX + 1u) + 64u];
  sink o;
  uint32_t i, n, off, ef = 0, partial_boot, nact;
  const char *rt = KZ_RT_RUNTIME_VERSION;
  const char *adn = ad != NULL && ad->name != NULL ? ad->name : "";
  const uint32_t mode = LDR(&g_mode);
  uint32_t rtl = 0, adl = 0;
  if (write == NULL) return KZ_RT_EINVAL;
  o.w = write;
  o.ctx = wctx;
  o.bytes = 0;
  o.crc = 0xFFFFFFFFu;
  o.err = 0;
  /* file header */
  cpy(b, (const unsigned char *)KZ_RT_MAGIC, 8u);
  put16(b + 8, KZ_RT_FORMAT_MAJOR);
  put16(b + 10, KZ_RT_FORMAT_MINOR);
  put32(b + 12, (LD(&g.phase) & KZ_RT_PHASE_STOP) != 0 ? KZ_RT_DUMP_STOPPED : 0u);
  emit(&o, b, KZ_RT_FILE_HEADER_BYTES);
  /* RUN */
  while (rt[rtl] != 0 && rtl < KZ_RT_TAG_MAX) ++rtl;
  while (adn[adl] != 0 && adl < KZ_RT_TAG_MAX) ++adl;
  zero(b, sizeof b);
  put32(b + 8, KZ_RT_ABI_VERSION);
  put32(b + 12, mode);
  put64(b + 16, g.run_id);
  put32(b + 24, g.instance_id);
  put32(b + 28, LD(&g.phase) | (k.adapter_lock != 0 ? 0x100u : 0u));
  put32(b + 32, KZ_RT_MAX_ACTORS);
  put32(b + 36, actor_bound());
  put32(b + 40, KZ_RT_BOOT_BUF_BYTES);
  put32(b + 44, KZ_RT_MAX_POINTS);
  put32(b + 48, KZ_RT_MAX_VALS);
  put32(b + 52, KZ_RT_STR_MAX);
  put32(b + 56, ad != NULL ? ad->caps : 0u);
  put32(b + 60, g.clock_id);
  put64(b + 64, g.t_init);
  put64(b + 72, g.t_attach);
  put64(b + 80, g.t_stop);
  put64(b + 88, now(ad, &ef));
  /* build identity: the binary's values where it has them, else the harness's */
  {
    unsigned char bm[32];
    const int have_bm = build_manifest(bi, bm);
    if (have_bm < 0) (void)__atomic_fetch_or(&g.rx_flags, KZ_RT_RX_BUILD_INFO_BAD, __ATOMIC_RELAXED);
    cpy(b + 96, have_bm == 1 ? bm : g.manifest, 32u);
  }
  put32(b + 128, holdable_mask(ad));
  off = KZ_RT_RUN_FIXED;
  off += put_text(b + off, rt, rtl);
  off += put_text(b + off, adn, adl);
  {
    const char *bt[3];
    bt[0] = bi != NULL ? bi->backend : NULL;
    bt[1] = bi != NULL ? bi->compiler : NULL;
    bt[2] = bi != NULL ? bi->target : NULL;
    for (i = 0; i < 3u; ++i)
      off += tag_given(bt[i]) ? put_text(b + off, bt[i], text_len(bt[i])) : put_text(b + off, g.tags[i], g.tag_len[i]);
  }
  off = pad4(off);
  rec_header(b, off, KZ_RT_REC_RUN, 0);
  emit(&o, b, off);
  /* RUNX (1.1) */
  {
    const uint32_t taken = LDR(&g.pool_next);
    const uint32_t boot = LDR(&g.boot_reserved);
    zero(b, KZ_RT_RUNX_SIZE);
    rec_header(b, KZ_RT_RUNX_SIZE, KZ_RT_REC_RUNX, 0);
    put32(b + 8, mode == KZ_RT_MODE_STOPPED ? g.window_mode : mode);
    put32(b + 12, g.mode_source);
    put32(b + 16, LDR(&g.rx_flags) | (bi != NULL ? KZ_RT_RX_BUILD_INFO : 0u));
    put32(b + 20, KZ_RT_POOL_BYTES);
    put32(b + 24, KZ_RT_CHUNK_BYTES);
    put32(b + 28, taken < POOL_CHUNKS ? taken : POOL_CHUNKS);
    put32(b + 32, LDR(&g.pool_dropped));
    put32(b + 36, LDR(&g.actor_quota));
    put32(b + 40, boot < KZ_RT_BOOT_BUF_BYTES ? boot : KZ_RT_BOOT_BUF_BYTES);
    put32(b + 44, LDR(&g.boot_dropped));
    put32(b + 48, KZ_RT_MAX_MODULES);
    put32(b + 52, KZ_RT_MAX_NOTES);
    emit(&o, b, KZ_RT_RUNX_SIZE);
  }
  /* MODULE */
  n = LD(&g.modules_next);
  if (n > KZ_RT_MAX_MODULES) n = KZ_RT_MAX_MODULES;
  for (i = 0; i < n; ++i) {
    const module_entry *m = &g.modules[i];
    if (LD(&m->used) == 0) continue;
    rec_header(b, KZ_RT_MODULE_SIZE, KZ_RT_REC_MODULE, 0);
    put32(b + 8, m->module);
    put32(b + 12, m->actor);
    put64(b + 16, m->t);
    cpy(b + 24, m->sha, 32u);
    put32(b + 56, LDR(&m->count));
    put32(b + 60, 0);
    emit(&o, b, KZ_RT_MODULE_SIZE);
  }
  /* RESOURCE */
  if (ad != NULL && ad->resources != NULL) {
    kz_rt_resource r[8];
    const uint32_t nr = ad->resources(ad->ctx, r, 8u);
    for (i = 0; i < nr && i < 8u; ++i) {
      uint32_t tl = 0;
      if (r[i].what != NULL)
        while (r[i].what[tl] != 0 && tl < KZ_RT_TAG_MAX) ++tl;
      off = pad4(KZ_RT_RESOURCE_FIXED + 2u + tl);
      zero(b, off);
      rec_header(b, off, KZ_RT_REC_RESOURCE, 0);
      put32(b + 8, r[i].kind);
      put32(b + 12, r[i].origin);
      put64(b + 16, r[i].count);
      (void)put_text(b + KZ_RT_RESOURCE_FIXED, r[i].what != NULL ? r[i].what : "", tl);
      emit(&o, b, off);
    }
  }
  /* ACTOR */
  nact = LD(&g.run_next);
  if (nact > KZ_RT_MAX_ACTORS) nact = KZ_RT_MAX_ACTORS;
  for (i = 0; i < nact; ++i) {
    const actor_slot *s = &g_actor[i];
    if (LD(&s->state) == 0) continue;
    rec_header(b, KZ_RT_ACTOR_SIZE, KZ_RT_REC_ACTOR, 0);
    put32(b + 8, s->id);
    put32(b + 12, (LD(&s->exited) != 0 ? KZ_RT_AF_EXITED : 0u) | (LDR(&s->held_point) != 0 ? KZ_RT_AF_HELD : 0u));
    put32(b + 16, s->boot_link);
    put32(b + 20, s->link_evidence);
    put64(b + 24, s->native_key);
    put32(b + 32, s->key_kind);
    put32(b + 36, s->key_generation);
    put32(b + 40, s->first_aseq);
    put32(b + 44, LDR(&s->last_aseq));
    put32(b + 48, LDR(&s->recorded));
    put32(b + 52, LDR(&s->overflow));
    put32(b + 56, LD(&s->busy));
    put32(b + 60, LD(&s->committed));
    put32(b + 64, LDR(&s->dropped_bytes));
    put32(b + 68, LDR(&s->chunks));
    put32(b + 72, LDR(&s->ovf_quota));
    put32(b + 76, LDR(&s->ovf_pool));
    emit(&o, b, KZ_RT_ACTOR_SIZE);
  }
  /* LOSS: global kinds, per actor, notes */
  (void)boot_extent(&partial_boot);
  for (i = 1; i <= KZ_RT_LOSS_NOTE_TABLE; ++i) emit_loss(&o, i, 0, 0, LDR(&g.loss[i]));
  emit_loss(&o, KZ_RT_LOSS_PARTIAL, 0, 0, partial_boot);
  for (i = 0; i < nact; ++i) {
    const actor_slot *s = &g_actor[i];
    if (LD(&s->state) == 0) continue;
    emit_loss(&o, KZ_RT_LOSS_OVERFLOW, s->id, 0, LDR(&s->overflow));
    emit_loss(&o, KZ_RT_LOSS_PARTIAL, s->id, 0, LD(&s->busy));
  }
  n = LD(&g.notes_next);
  if (n > KZ_RT_MAX_NOTES) n = KZ_RT_MAX_NOTES;
  for (i = 0; i < n; ++i)
    if (LD(&g.notes[i].used) != 0) emit_loss(&o, g.notes[i].kind, 0, g.notes[i].key, LDR(&g.notes[i].count));
  /* POINT */
  if (LD(&k.loaded) != 0) {
    ctl_lock(ad);
    for (i = 0; i < k.n; ++i) {
      rec_header(b, KZ_RT_POINT_SIZE, KZ_RT_REC_POINT, 0);
      put32(b + 8, i);
      put32(b + 12, k.pt[i].kind);
      put32(b + 16, k.pt[i].label);
      put32(b + 20, k.pt[i].site);
      put32(b + 24, k.pt[i].releases);
      put32(b + 28, k.pt[i].occurrence);
      put32(b + 32, k.pt[i].flags);
      put32(b + 36, k.hits[i]);
      put32(b + 40, k.acted[i]);
      put32(b + 44, k.satisfied[i]);
      put32(b + 48, k.held[i]);
      put32(b + 52, k.holds[i]);
      emit(&o, b, KZ_RT_POINT_SIZE);
    }
    ctl_unlock(ad);
  }
  /* BUF: the shared buffer, then each actor's */
  {
    uint32_t pb;
    const uint32_t nb = boot_extent(&pb);
    rec_header(b, KZ_RT_BUF_FIXED + nb, KZ_RT_REC_BUF, 0);
    put32(b + 8, 0);
    put32(b + 12, nb);
    put32(b + 16, KZ_RT_BOOT_BUF_BYTES);
    put32(b + 20, 0);
    emit(&o, b, KZ_RT_BUF_FIXED);
    emit(&o, (const unsigned char *)g_boot_buf, nb);
  }
  for (i = 0; i < nact; ++i) {
    const actor_slot *s = &g_actor[i];
    uint32_t first, c, nx, u, nb = 0, last = 0, last_used = 0;
    if (LD(&s->state) == 0) continue;
    /* the committed prefix of the actor's chunks: the link is read before the length, so a chunk
     * with a next chunk is complete; the last one's length is read once and reused below */
    first = LD(&s->first_chunk);
    for (c = first; c != 0; c = nx) {
      nx = LD(&g_chunk_next[c - 1u]);
      u = LD(&g_chunk_used[c - 1u]);
      nb += u;
      if (nx == 0) {
        last = c;
        last_used = u;
      }
    }
    rec_header(b, KZ_RT_BUF_FIXED + nb, KZ_RT_REC_BUF, 0);
    put32(b + 8, s->id);
    put32(b + 12, nb);
    put32(b + 16, actor_bound());
    put32(b + 20, 0);
    emit(&o, b, KZ_RT_BUF_FIXED);
    for (c = first; c != 0; c = LD(&g_chunk_next[c - 1u])) {
      if (c == last) {
        emit(&o, chunk_base(c - 1u), last_used);
        break;
      }
      emit(&o, chunk_base(c - 1u), LD(&g_chunk_used[c - 1u]));
    }
  }
  /* END */
  {
    const uint64_t total = o.bytes;
    const uint32_t crc = o.crc ^ 0xFFFFFFFFu;
    rec_header(b, KZ_RT_END_SIZE, KZ_RT_REC_END, 0);
    put64(b + 8, total);
    put32(b + 16, crc);
    put32(b + 20, 0);
    o.crc = 0; /* END is not covered */
    emit(&o, b, KZ_RT_END_SIZE);
  }
  return o.err ? KZ_RT_EWRITE : KZ_RT_OK;
}

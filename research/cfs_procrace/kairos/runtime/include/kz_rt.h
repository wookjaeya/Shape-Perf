/* kz_rt.h -- the Kairos instrumentation runtime: hook ABI 1, control ABI 1, adapter ABI 1.
 *
 * Semantics: kairos/docs/event_semantics.md (common event information, site manifest
 * "kzs-sites 1", kzp-hold 1 control semantics, realized-control records, capabilities,
 * holdable contexts).  This header is the interface between three parties:
 *
 *   1. the compiler binding (GCC plugin, Clang source rewrite, LLVM pass): it inserts calls to
 *      the observation hooks kz_rt_obs* and the control hook kz_rt_ctl at the bindings the site
 *      manifest lists; site and binding ids are compile-time constants taken from the manifest;
 *   2. the target harness: kz_rt_init / kz_rt_attach / kz_rt_plan / kz_rt_stop / kz_rt_dump;
 *   3. the OS adapter (kz_rt_posix.h, kz_rt_osal.h): identity cell, clock, wait primitive.
 *
 * Rules the runtime keeps (decision record 2026-10-26 section 4):
 *   - observation hooks never block, never take a lock, never call instrumented code, and never
 *     read target memory except the bounded string copy of kz_rt_obs_str (allowed only at
 *     bindings the manifest marks string=validated);
 *   - values are the operands the instrumented code used, passed by the binding (64-bit,
 *     zero- or sign-extended per the manifest type); the runtime never re-reads shared memory;
 *   - the runtime's own order (buffer order, actor allocation, ctl_seq, its locks) is never a
 *     happens-before edge of the target; control decisions carry their own order (ctl_seq);
 *   - before kz_rt_attach every event goes to a static bootstrap buffer under a provisional
 *     actor id; an unknown identity stays unknown (never actor 0 = "no application");
 *   - every record is serialized little-endian byte by byte when it is written, so a buffer is
 *     readable on the host whatever the target's byte order and struct layout;
 *   - all memory is static (no allocation); sizes are the KZ_RT_* macros below.
 *
 * The runtime itself must not be instrumented, and it is linked once per process (into the
 * executable; loaded modules call it through the dynamic symbol table).
 */
#ifndef KZ_RT_H
#define KZ_RT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define KZ_RT_API __attribute__((visibility("default")))
#else
#define KZ_RT_API
#endif

/* ---- versions -------------------------------------------------------------------------- */
#define KZ_RT_ABI_VERSION 1u        /* hooks, control points, config, adapter */
#define KZ_RT_FORMAT_MAJOR 1u       /* dump format "kzrt-dump"; a reader accepts only its major */
#define KZ_RT_FORMAT_MINOR 0u       /* added record types or fields at the end of a record */
#define KZ_RT_RUNTIME_VERSION "kzrt 0.1.0"

/* ---- static sizes (override with -D when building the runtime; recorded in the dump) ---- */
#ifndef KZ_RT_MAX_ACTORS
#define KZ_RT_MAX_ACTORS 64u        /* run actors with their own buffer */
#endif
#ifndef KZ_RT_ACTOR_BUF_BYTES
#define KZ_RT_ACTOR_BUF_BYTES 65536u
#endif
#ifndef KZ_RT_BOOT_BUF_BYTES
#define KZ_RT_BOOT_BUF_BYTES 65536u /* shared buffer before attach, and for unresolved actors */
#endif
#ifndef KZ_RT_MAX_POINTS
#define KZ_RT_MAX_POINTS 32u        /* control points of one plan */
#endif
#ifndef KZ_RT_MAX_MODULES
#define KZ_RT_MAX_MODULES 32u
#endif
#ifndef KZ_RT_MAX_NOTES
#define KZ_RT_MAX_NOTES 32u         /* (kind, key) loss notes */
#endif
#define KZ_RT_MAX_VALS 8u           /* values of one event */
#define KZ_RT_STR_MAX 64u           /* bytes of a validated string copy (and of a string condition) */
#define KZ_RT_TAG_MAX 63u           /* bytes of a config tag string */
#define KZ_RT_POINT_BINDS 8u        /* binding filter of one control point */

/* ---- actor ids ------------------------------------------------------------------------- */
/* 0 = unresolved (no per-thread cell in this context); 1..KZ_RT_MAX_ACTORS = run actors;
 * provisional ids (before attach, or when the actor table is full) have this bit set. */
#define KZ_RT_ACTOR_UNRESOLVED 0u
#define KZ_RT_ACTOR_PROVISIONAL 0x80000000u

/* ---- modes (same binary; G-I0 / G-I / G-I-H of the review section 13.1) ----------------- */
enum {
  KZ_RT_MODE_OFF = 0,     /* hooks return at once (records before kz_rt_init stay in the boot buffer) */
  KZ_RT_MODE_RECORD = 1,  /* observation hooks record; kz_rt_ctl returns at once.  The static default */
  KZ_RT_MODE_CONTROL = 2, /* observation hooks record; kz_rt_ctl evaluates the plan */
  KZ_RT_MODE_STOPPED = 3  /* after kz_rt_stop: hooks only count (loss kind after-stop) */
};

/* ---- holdable-context classes (manifest "control ctx=", event_semantics.md section 7) --- */
enum {
  KZ_RT_CTX_UNDECLARED = 0,    /* treated as unsupported */
  KZ_RT_CTX_TASK = 1,          /* ordinary task, the release path can progress: may hold */
  KZ_RT_CTX_LOCK_HELD = 2,     /* holds a lock a release may need: holds only if the point allows it */
  KZ_RT_CTX_ISR = 3,           /* interrupt/exception/signal context: no control evaluation at all */
  KZ_RT_CTX_OSAL_INTERNAL = 4, /* inside scheduler/OSAL registration: holds only if the point allows it
                                  and the adapter's control lock is private (KZ_RT_CAP_LOCK_PRIVATE) */
  KZ_RT_CTX_TERMINATING = 5,   /* task being deleted or exiting: never holds */
  KZ_RT_CTX_UNSUPPORTED = 6    /* declared unsupported: never holds (releases still act) */
};

/* ---- loss and coverage kinds (dump LOSS records) ---------------------------------------- */
enum {
  KZ_RT_LOSS_OVERFLOW = 1,     /* actor buffer full: record dropped (its aseq was used: a gap) */
  KZ_RT_LOSS_PARTIAL = 2,      /* a record was being written when the dump read the buffer */
  KZ_RT_LOSS_PREINIT = 3,      /* bootstrap buffer full: an event before attach was not recorded */
  KZ_RT_LOSS_EXCLUDED = 4,     /* noted: code not instrumented (kz_rt_note_loss; key = module/site) */
  KZ_RT_LOSS_UNLOADED = 5,     /* noted: module not loaded or unloaded (key = module) */
  KZ_RT_LOSS_REENTRY = 6,      /* a hook entered while its thread was inside a hook: not recorded */
  KZ_RT_LOSS_ACTOR_TABLE = 7,  /* actors without a slot (their events use the shared buffer) */
  KZ_RT_LOSS_UNRESOLVED = 8,   /* events recorded with actor 0 (no per-thread cell) */
  KZ_RT_LOSS_CTL_SKIPPED = 9,  /* kz_rt_ctl calls not evaluated (ISR, or a context the lock cannot serve) */
  KZ_RT_LOSS_CTL_NOCOND = 10,  /* control conditions not evaluable (the hit's values were not cached) */
  KZ_RT_LOSS_MODULE_TABLE = 11,/* kz_rt_module calls beyond KZ_RT_MAX_MODULES */
  KZ_RT_LOSS_AFTER_STOP = 12,  /* hook calls after kz_rt_stop (outside the observation window) */
  KZ_RT_LOSS_NOTE_TABLE = 13   /* kz_rt_note_loss calls beyond KZ_RT_MAX_NOTES */
};

/* ---- the per-thread cell ---------------------------------------------------------------- */
/* Storage the adapter provides per thread, zero-initialized, never shared between threads.
 * The runtime keeps the actor id, the actor-local sequence and the last event's values in it
 * (the values a control condition reads).  Fields are the runtime's; adapters only provide it. */
typedef struct kz_rt_cell {
  uint32_t actor;   /* 0 = none yet */
  uint32_t aseq;    /* last actor-local sequence number used */
  uint32_t depth;   /* hook nesting guard */
  uint32_t flags;
  uint32_t last_aseq, last_site, last_bind, last_valid;
  uint32_t last_nvals, last_str_slot, last_str_len, rsv;
  uint64_t last_v[KZ_RT_MAX_VALS];
  unsigned char last_str[KZ_RT_STR_MAX];
} kz_rt_cell;

/* The runtime's own per-thread cell (compiler thread-local storage; NULL when the runtime was
 * built with KZ_RT_HAVE_TLS=0).  Before attach every event uses it; an adapter whose platform has
 * thread-local storage returns it from self_cell, so that a thread's provisional id and its run
 * actor are linked by the same cell (ACTOR link_evidence same-cell). */
KZ_RT_API kz_rt_cell *kz_rt_tls_cell(void);

/* ---- adapter interface (ABI 1) ---------------------------------------------------------- */
enum { /* adapter capability bits */
  KZ_RT_CAP_CLOCK = 1u << 0,        /* now_ns works */
  KZ_RT_CAP_NATIVE_KEY = 1u << 1,   /* native_key works */
  KZ_RT_CAP_LOCK = 1u << 2,         /* ctl_lock / ctl_unlock */
  KZ_RT_CAP_LOCK_PRIVATE = 1u << 3, /* the lock is not a target object (safe inside OSAL internals) */
  KZ_RT_CAP_WAIT = 1u << 4,         /* hold_wait / hold_wake: gates can hold */
  KZ_RT_CAP_WAIT_TIMEOUT = 1u << 5  /* hold_wait honours a timeout */
};
enum { /* native key kinds */
  KZ_RT_KEY_NONE = 0, KZ_RT_KEY_LINUX_TID = 1, KZ_RT_KEY_PTHREAD = 2, KZ_RT_KEY_OSAL_ID = 3, KZ_RT_KEY_OTHER = 15
};
enum { /* clock ids */
  KZ_RT_CLOCK_NONE = 0, KZ_RT_CLOCK_POSIX_MONOTONIC = 1, KZ_RT_CLOCK_OSAL_LOCAL = 2, KZ_RT_CLOCK_OTHER = 15
};
enum { /* resource kinds and origins (adapter accounting, review section 11.2) */
  KZ_RT_RES_THREAD = 1, KZ_RT_RES_MUTEX = 2, KZ_RT_RES_BINSEM = 3, KZ_RT_RES_COUNTSEM = 4,
  KZ_RT_RES_QUEUE = 5, KZ_RT_RES_CONDVAR = 6, KZ_RT_RES_MEMORY_BYTES = 7, KZ_RT_RES_FILE = 8
};
enum { KZ_RT_ORIGIN_PRIVATE = 1, KZ_RT_ORIGIN_TARGET_TABLE = 2 };

typedef struct kz_rt_resource {
  uint32_t kind;    /* KZ_RT_RES_* */
  uint32_t origin;  /* KZ_RT_ORIGIN_*: TARGET_TABLE = taken from the target's object table (e.g. OSAL) */
  uint64_t count;
  const char *what; /* short text, may be NULL */
} kz_rt_resource;

typedef struct kz_rt_adapter {
  uint32_t abi;     /* KZ_RT_ABI_VERSION */
  uint32_t caps;    /* KZ_RT_CAP_* */
  const char *name; /* "posix", "osal", ... */
  void *ctx;
  /* Required.  The calling thread's cell, or NULL when this context has none (the event is then
   * recorded as unresolved).  Must not call instrumented code or block. */
  kz_rt_cell *(*self_cell)(void *ctx);
  /* Optional (caps).  None of them may call instrumented code. */
  uint64_t (*native_key)(void *ctx, uint32_t *key_kind, uint32_t *key_generation);
  uint64_t (*now_ns)(void *ctx, uint32_t *clock_id);
  void (*ctl_lock)(void *ctx);
  void (*ctl_unlock)(void *ctx);
  /* Wait (no runtime lock held) until *released != 0 or the timeout passes (0 = no timeout).
   * Returns 0 when released, 1 on timeout.  hold_wake is called (control lock held) after the
   * runtime set *released; a wake before the wait must not be lost. */
  int (*hold_wait)(void *ctx, uint32_t actor_index, volatile const uint32_t *released, uint64_t timeout_ns);
  void (*hold_wake)(void *ctx, uint32_t actor_index);
  /* Resources the adapter consumed so far; returns how many entries it wrote (<= max). */
  uint32_t (*resources)(void *ctx, kz_rt_resource *out, uint32_t max);
} kz_rt_adapter;

/* ---- configuration (kz_rt_init) ---------------------------------------------------------- */
typedef struct kz_rt_config {
  uint32_t abi;               /* KZ_RT_ABI_VERSION */
  uint32_t mode;              /* KZ_RT_MODE_OFF / RECORD / CONTROL */
  uint64_t run_id;            /* given by the harness (0 = not given) */
  uint32_t instance_id;       /* the cFS instance of this process in the run (0 = not given) */
  uint32_t actor_buf_bytes;   /* per-actor capacity, <= KZ_RT_ACTOR_BUF_BYTES (0 = the maximum) */
  const uint8_t *manifest_sha256; /* 32 bytes: the site manifest the binding used (NULL = none) */
  const char *backend;        /* binding identity, e.g. "gcc-plugin 0.1" (bounded copy) */
  const char *compiler;       /* e.g. the compiler's version string */
  const char *target;         /* e.g. "x86_64-linux-gnu" */
} kz_rt_config;

/* ---- control plan (kzp-hold 1 semantics, event_semantics.md section 4) -------------------- */
enum { KZ_RT_POINT_GATE = 1, KZ_RT_POINT_RELEASE = 2 };
enum {
  KZ_RT_PT_UNLESS_RELEASED = 1u << 0,     /* gate: a release that acts while it holds nothing satisfies it */
  KZ_RT_PT_ALLOW_LOCK_HELD = 1u << 1,     /* gate: may hold in KZ_RT_CTX_LOCK_HELD (risk recorded) */
  KZ_RT_PT_ALLOW_OSAL_INTERNAL = 1u << 2  /* gate: may hold in KZ_RT_CTX_OSAL_INTERNAL */
};
typedef struct kz_rt_point {
  uint32_t kind;        /* KZ_RT_POINT_GATE / RELEASE */
  uint32_t label;       /* the plan's label id (the host maps it to the .kzh label) */
  uint32_t site;        /* logical site id */
  uint32_t releases;    /* release: index (in this plan) of the gate it releases */
  uint32_t occurrence;  /* 0 = each; n >= 1 = only the n-th qualifying hit acts ("first" = 1) */
  uint32_t flags;       /* KZ_RT_PT_* */
  uint32_t bind_n;      /* > 0: a hit qualifies only at these bindings (host form of caller-in) */
  uint32_t bind[KZ_RT_POINT_BINDS];
  int32_t str_slot;     /* >= 0: the hit's validated string captured from that value slot must equal str */
  uint32_t str_len;
  unsigned char str[KZ_RT_STR_MAX];
  int32_t val_slot;     /* >= 0: the hit's value in that slot must equal val */
  uint32_t rsv;
  uint64_t val;
  uint64_t timeout_ns;  /* gate: 0 = hold until released; else resume and record a timeout */
} kz_rt_point;

typedef struct kz_rt_point_state { /* kz_rt_ctl_state */
  uint32_t hits;        /* qualifying hits counted so far */
  uint32_t acted;       /* first / n-th: it acted (or, for a gate, was satisfied) */
  uint32_t satisfied;   /* gate: satisfied by an earlier release (unless-released) */
  uint32_t held_actor;  /* gate: the run actor it holds now (0 = none) */
  uint32_t holds;       /* gate: holds made */
} kz_rt_point_state;

/* kz_rt_ctl results (the instrumented code ignores them) */
enum { KZ_RT_CTL_PASS = 0, KZ_RT_CTL_RESUMED = 1, KZ_RT_CTL_TIMEOUT = 2, KZ_RT_CTL_REFUSED = 3 };

/* error codes of the setup calls */
enum { KZ_RT_OK = 0, KZ_RT_EINVAL = -1, KZ_RT_ESTATE = -2, KZ_RT_EABI = -3, KZ_RT_EWRITE = -4 };

/* Capability descriptor (event_semantics.md section 6), also written to the dump's RUN record. */
typedef struct kz_rt_caps {
  uint32_t abi, format_major, format_minor;
  uint32_t max_actors, actor_buf_bytes, boot_buf_bytes, max_points, max_vals, str_max, max_modules;
  uint32_t adapter_caps;        /* 0 before attach */
  uint32_t holdable_ctx_mask;   /* bit c = a gate can hold in context class c (with this adapter) */
  uint32_t gates_per_actor;     /* gates that can hold one actor at a time (1) */
  uint32_t distinct_sites;      /* control sites per plan (KZ_RT_MAX_POINTS; no address limit) */
} kz_rt_caps;

/* Byte sink of kz_rt_dump: returns 0 on success. */
typedef int (*kz_rt_write_fn)(void *ctx, const void *bytes, size_t n);

/* ---- setup and lifecycle ----------------------------------------------------------------- */
KZ_RT_API int kz_rt_init(const kz_rt_config *cfg);         /* once; before or after attach */
KZ_RT_API int kz_rt_attach(const kz_rt_adapter *adapter);  /* once; ends the bootstrap phase */
KZ_RT_API int kz_rt_plan(const kz_rt_point *points, uint32_t n); /* once, before the first control hit */
KZ_RT_API void kz_rt_stop(void);                           /* ends the observation window */
KZ_RT_API int kz_rt_dump(kz_rt_write_fn write, void *ctx); /* the whole state, format 1.0 */
KZ_RT_API void kz_rt_caps_get(kz_rt_caps *out);
KZ_RT_API int kz_rt_ctl_state(uint32_t point, kz_rt_point_state *out);

/* ---- notes -------------------------------------------------------------------------------- */
KZ_RT_API void kz_rt_module(uint32_t module, const uint8_t *manifest_sha256); /* an instrumented module runs */
KZ_RT_API void kz_rt_note_loss(uint32_t kind, uint32_t key, uint32_t count);  /* EXCLUDED, UNLOADED, ... */
KZ_RT_API void kz_rt_actor_exit(void);  /* the calling thread ends (its actor id is never reused) */

/* ---- observation hooks (hook ABI 1) -------------------------------------------------------- */
/* site: logical site id; bind: binding id (both manifest constants).  link: the aseq of a related
 * record of the same actor (the "before" record of this call for an "after" record), 0 = none.
 * valid: bit i set = value i was bound by the binding (an unset bit means "not observed", which
 * is never read as the value 0).  Returns the record's actor-local sequence number (aseq), which
 * the binding passes as link / ev; 0 when nothing was recorded (mode off, re-entry, stopped). */
KZ_RT_API uint32_t kz_rt_obs(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid,
                             uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3);
KZ_RT_API uint32_t kz_rt_obsv(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid,
                              uint32_t n, const uint64_t *v);  /* n <= KZ_RT_MAX_VALS */
/* As kz_rt_obs, and copies at most str_bound (<= KZ_RT_STR_MAX) bytes of the NUL-terminated
 * string at the pointer in value slot str_slot.  Only at bindings the manifest marks
 * string=validated with that bound: the original code has already validated the pointer. */
KZ_RT_API uint32_t kz_rt_obs_str(uint32_t site, uint32_t bind, uint32_t link, uint32_t valid,
                                 uint64_t v0, uint64_t v1, uint64_t v2, uint64_t v3,
                                 uint32_t str_slot, uint32_t str_bound);

/* ---- control hook (control ABI 1) ----------------------------------------------------------- */
/* At a binding the manifest marks "control": evaluates the plan's gates and releases at this site
 * for the calling thread's hit whose observation record is ev (the aseq kz_rt_obs* returned just
 * before, same thread), in context class ctx (KZ_RT_CTX_*, a manifest constant).  May block
 * (a gate holds) only in a holdable context.  Never called from inside an observation hook. */
KZ_RT_API int kz_rt_ctl(uint32_t site, uint32_t bind, uint32_t ev, uint32_t ctx);

#ifdef __cplusplus
}
#endif
#endif /* KZ_RT_H */

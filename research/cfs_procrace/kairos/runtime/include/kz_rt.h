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
#define KZ_RT_ABI_VERSION 1u        /* hooks, control points, config, adapter (0.2.0 only adds calls) */
#define KZ_RT_FORMAT_MAJOR 1u       /* dump format "kzrt-dump"; a reader accepts only its major */
#define KZ_RT_FORMAT_MINOR 2u       /* added record types or fields at the end of a record (1.1: RUNX,
                                       MODULE registrations, ACTOR demand and chunks; 1.2: RUNX process) */
#define KZ_RT_RUNTIME_VERSION "kzrt 0.2.1"

/* ---- static sizes (override with -D when building the runtime; recorded in the dump) ---- */
/* Tool capacities, not research conditions (event_semantics.md 8.3, 8.9: how the defaults were derived
 * from the stage C pilot dumps).  Run actors keep their records in chunks of one static pool: each chunk
 * has one writer (its actor), records never span chunks, and a full pool or a reached per-actor quota
 * drops the record without waiting (LOSS overflow, aseq gap, dropped bytes in the ACTOR record). */
#ifndef KZ_RT_MAX_ACTORS
#define KZ_RT_MAX_ACTORS 64u        /* run actors with their own chunks */
#endif
#ifndef KZ_RT_POOL_BYTES
#define KZ_RT_POOL_BYTES (KZ_RT_MAX_ACTORS * 65536u) /* the run actors' records (all actors together) */
#endif
#ifndef KZ_RT_CHUNK_BYTES
#define KZ_RT_CHUNK_BYTES 4096u     /* unit of the pool an actor takes (>= the largest record, 184 B) */
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
/* The mode word is read by every hook.  Its sources, earliest first: kz_rt_early_mode (latches the
 * mode for the whole process; the POSIX start-up object calls it from .preinit_array, before any
 * constructor), kz_rt_init (when no early mode was given), and the static default RECORD (hooks
 * before either).  The dump's RUNX record says which source set it and whether hooks ran before. */
enum {
  KZ_RT_MODE_OFF = 0,     /* hooks return at once, nothing is counted; kz_rt_stop keeps it OFF */
  KZ_RT_MODE_RECORD = 1,  /* observation hooks record; kz_rt_ctl returns at once.  The static default */
  KZ_RT_MODE_CONTROL = 2, /* observation hooks record; kz_rt_ctl evaluates the plan */
  KZ_RT_MODE_STOPPED = 3  /* after kz_rt_stop of a RECORD/CONTROL window: hooks only count (after-stop) */
};
enum { /* mode sources (RUNX mode_source) */
  KZ_RT_MODE_SRC_STATIC = 0, /* no setup call yet: the static default RECORD */
  KZ_RT_MODE_SRC_EARLY = 1,  /* kz_rt_early_mode */
  KZ_RT_MODE_SRC_INIT = 2    /* kz_rt_init */
};
enum { /* notes of kz_rt_early_mode (RUNX flags) */
  KZ_RT_EARLY_GIVEN = 0,          /* the mode was named explicitly */
  KZ_RT_EARLY_DEFAULTED = 1u << 0,/* no mode was named: the caller used its documented default */
  KZ_RT_EARLY_INVALID = 1u << 1   /* the named mode was not valid: the caller kept the static default */
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
  KZ_RT_LOSS_OVERFLOW = 1,     /* actor quota reached or pool empty: record dropped (its aseq was used: a gap) */
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
  uint32_t actor_buf_bytes;   /* per-actor quota of committed bytes (0 = none: only the pool limits) */
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
  uint32_t max_actors, actor_buf_bytes /* per-actor bound: the quota, else the pool */, boot_buf_bytes, max_points, max_vals, str_max,
      max_modules;
  uint32_t adapter_caps;        /* 0 before attach */
  uint32_t holdable_ctx_mask;   /* bit c = a gate can hold in context class c (with this adapter) */
  uint32_t gates_per_actor;     /* gates that can hold one actor at a time (1) */
  uint32_t distinct_sites;      /* control sites per plan (KZ_RT_MAX_POINTS; no address limit) */
} kz_rt_caps;

/* Byte sink of kz_rt_dump: returns 0 on success. */
typedef int (*kz_rt_write_fn)(void *ctx, const void *bytes, size_t n);

/* ---- build identity (event_semantics.md 8.5) ---------------------------------------------- */
/* What the binary was built with: the binding's identity, the compiler, the target and the sha256 of
 * the site manifest the binding read.  Defined once, in the object of the fixed source
 * src/kz_rt_build_info.c that the instrumented build compiles with -D values (CMake target
 * kzrt-build-info) and links into the executable as an object (not an archive member: the runtime's
 * reference is weak and would not pull it).  Without it the RUN tags are the harness's kz_rt_config
 * values ("" = not given).  With it, the RUN tags are its values, and a harness value that differs is
 * recorded as a conflict (RUNX flag), never silently preferred.  The symbol is
 * "const kz_rt_build kz_rt_build_info"; only the runtime core declares it (weakly, kz_rt.c). */
typedef struct kz_rt_build {
  uint32_t abi;                    /* KZ_RT_ABI_VERSION */
  const char *backend;             /* e.g. "kzgcc 0.1 (gcc-plugin)" (<= KZ_RT_TAG_MAX bytes are kept) */
  const char *compiler;            /* e.g. the first line of "gcc --version" */
  const char *target;              /* e.g. "gcc -dumpmachine" */
  const char *manifest_sha256_hex; /* 64 hex digits: sha256 of the manifest file's bytes; NULL = none */
  const char *origin;              /* how the values were given (the source file and its -D names) */
} kz_rt_build;

/* ---- setup and lifecycle ----------------------------------------------------------------- */
/* Sets the mode word before anything else and latches it: kz_rt_init must then name the same mode
 * (else KZ_RT_EINVAL and nothing is set).  Once, before kz_rt_init; note = KZ_RT_EARLY_* bits.
 * Touches only the runtime's own memory (callable from .preinit_array or a BSP's first code). */
KZ_RT_API int kz_rt_early_mode(uint32_t mode, uint32_t note);
KZ_RT_API int kz_rt_init(const kz_rt_config *cfg);         /* once; before or after attach */
KZ_RT_API int kz_rt_attach(const kz_rt_adapter *adapter);  /* once; ends the bootstrap phase */
KZ_RT_API int kz_rt_plan(const kz_rt_point *points, uint32_t n); /* once, before the first control hit */
KZ_RT_API void kz_rt_stop(void);                           /* ends the observation window */
KZ_RT_API int kz_rt_dump(kz_rt_write_fn write, void *ctx); /* the whole state, format 1.0 */
KZ_RT_API void kz_rt_caps_get(kz_rt_caps *out);
enum { /* kz_rt_process evidence (RUNX process_evidence) */
  KZ_RT_PROC_CONSTRUCTOR = 1u << 0, /* called from a constructor of the executable (glibc runs them in the initial thread, before main) */
  KZ_RT_PROC_TID_IS_PID = 1u << 1   /* the calling thread's Linux TID equals the process id (gettid(2) = getpid(2): the thread-group leader) */
};
/* The process this runtime records and its initial thread (the thread that runs main), for the host's reading of
 * "the initial thread is not an application's task" (profile context `initial_thread not_app`, P10).  Called once,
 * by start-up code that runs in that thread (the POSIX start-up object's constructor): process = the process id,
 * initial_key/key_kind = the calling thread's native key as the adapter states it (KZ_RT_KEY_*), evidence =
 * KZ_RT_PROC_* bits (above): how the caller knows the thread is the initial one.  Written to RUNX (1.2).
 * Touches only the runtime's own memory; a second call is refused (KZ_RT_ESTATE). */
KZ_RT_API int kz_rt_process(uint64_t process, uint64_t initial_key, uint32_t key_kind, uint32_t evidence);
KZ_RT_API int kz_rt_ctl_state(uint32_t point, kz_rt_point_state *out);

/* ---- notes -------------------------------------------------------------------------------- */
/* An instrumented module runs.  One MODULE entry per (module, manifest sha256): further calls with the
 * same pair only count (MODULE registrations); the same module with another sha256 is a new entry. */
KZ_RT_API void kz_rt_module(uint32_t module, const uint8_t *manifest_sha256);
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

/* ---- serialized plans (control ABI 1, added in kzrt 0.3.0) ----------------------------------- */
/* A plan the host converted from a .kzh plan and the build's site manifest and binding list ("kz rtplan",
 * format "kzrt-plan 1" in kz_rt_format.h), given to the target as bytes (a file the POSIX start-up object
 * reads from KZ_RT_PLAN, or memory a BSP provides).  kz_rt_plan_load checks the bytes (magic, version, size,
 * crc32, the points as kz_rt_plan does) and the plan's manifest sha256 against the build identity
 * (kz_rt_build_info) when the binary carries one: a plan converted for another build is refused, never
 * loaded half.  Once per process, before the first control hit.  Whatever the outcome, the bytes (up to
 * KZ_RT_PLAN_MAX_BYTES) and the status are kept and written to the dump (PLAN record), so the record names
 * the plan the run had.  kz_rt_plan_note records a harness outcome without bytes (no plan in control mode,
 * a plan given in another mode, a read error).  Neither blocks or reads target memory. */
/* the largest plan: header + KZ_RT_MAX_POINTS points + trailer (KZ_RT_PLAN_* of kz_rt_format.h) */
#define KZ_RT_PLAN_MAX_BYTES (120u + KZ_RT_MAX_POINTS * 172u + 4u)
enum { /* PLAN record status */
  KZ_RT_PLANST_LOADED = 1,      /* the plan was loaded (kz_rt_plan) */
  KZ_RT_PLANST_NONE = 2,        /* control mode without a plan: every gate inactive (harness note) */
  KZ_RT_PLANST_IGNORED = 3,     /* a plan was given but the mode is not control: not loaded (harness note) */
  KZ_RT_PLANST_READ_ERROR = 4,  /* the harness could not read the plan (harness note) */
  KZ_RT_PLANST_SIZE = 5,        /* refused: shorter than a header or longer than KZ_RT_PLAN_MAX_BYTES */
  KZ_RT_PLANST_FORMAT = 6,      /* refused: magic, major, total size or point count */
  KZ_RT_PLANST_CRC = 7,         /* refused: crc32 */
  KZ_RT_PLANST_MANIFEST = 8,    /* refused: converted for another manifest than the build identity's */
  KZ_RT_PLANST_POINTS = 9,      /* refused by kz_rt_plan (KZ_RT_EINVAL: a point is not valid) */
  KZ_RT_PLANST_STATE = 10       /* refused: a plan was already set (kz_rt_plan called before) */
};
enum { /* PLAN record flags */
  KZ_RT_PLANF_CHECKED = 1u << 0,   /* the plan's manifest sha256 was compared with the build identity's */
  KZ_RT_PLANF_NO_BUILD = 1u << 1,  /* the binary has no build identity: the manifest was not compared */
  KZ_RT_PLANF_TRUNCATED = 1u << 2  /* more bytes were given than the dump keeps */
};
KZ_RT_API int kz_rt_plan_load(const void *bytes, size_t n); /* KZ_RT_OK, or EINVAL / ESTATE (status in the dump) */
KZ_RT_API void kz_rt_plan_note(uint32_t status);            /* KZ_RT_PLANST_NONE / IGNORED / READ_ERROR */

#ifdef __cplusplus
}
#endif
#endif /* KZ_RT_H */

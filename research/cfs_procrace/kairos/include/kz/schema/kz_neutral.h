/*
 * kz_neutral.h -- the neutral event layer of a run container (schema revision 3; side tables of revision 12).
 *
 * SPEC rev 1.4 section 2.3 "Formats in revision 1.4", decision record 2026-10-26 section 2 item 5,
 * review of 2026-10-26 sections 9.1-9.2; the meaning of every item is kairos/docs/event_semantics.md
 * section 2 (common event information), section 5 (realized control) and section 8.4 (loss kinds).
 * Both execution families write this layer: F0 by converting its run container (`kz convert`), the
 * instrumented family by converting a kzrt-dump stream.  The replay (kz/trace/real_trace.hpp) reads a
 * run container through this layer only; a container without it (every container written before
 * revision 3) is converted in memory with the same converter, so it is read as it was written.
 *
 * Backend raw evidence stays as backend extension data: F0's sections of kz_raw.h (RAW_EVENTS, SITES,
 * SITE_SLOTS, THREADS, ACTOR_OWNERS, LOSS, RUNMETA, MAPS, ...) are kept unchanged beside this layer, and
 * a neutral record references its raw record with `raw` (a kz_ref into this container) or, for a raw
 * stream that is not a container section (a kzrt-dump), with `raw_off` (byte offset in the stream whose
 * sha256 NRUN.raw_sha256 names).  The common interpretation never reads the raw evidence.
 *
 * What is neutral here and what it replaces in the F0 reading:
 *   - actor: NACTORS index (an actor of the run, never reused in it) with its creation generation and
 *     its backend binding (KZ_NBIND: a Linux thread incarnation of F0's identity join, a runtime actor
 *     of kzrt, a provisional runtime actor before attach, or unresolved), instead of a kernel tid
 *     looked up by time.  The native key (tid) is kept as the binding's key, not as the identity;
 *   - per-actor order `aseq`, with KZ_NEV_FLAG_ASEQ_TARGET when the target assigned it at the event
 *     (a gap is then a loss) and without it when a decoder assigned it (F0 tseq: a gap proves nothing);
 *   - logical site (NSITES) with its operation, phase, function and source position, and the binding id
 *     of every event (`bind`);
 *   - values with their validity: a value that was not observed has no NVALUES record (never 0);
 *   - loss records (NLOSS) with their kind, where they were counted, and the event interval they may
 *     affect (KZ_NONE32 end = up to the end of the record);
 *   - realized control records (NCTL), apart from the plan;
 *   - (schema revision 6) owners a creation request named only after the spawn (NOWNER): at which events the owner
 *     was established; a run without such an owner has no NOWNER section, so its container is written as before.
 *   - (schema revision 12) side tables keyed by actor, event, site or record index (NACTOR_TOKEN, NLIFE, NBUILD,
 *     NSUFFICE, NCALLEND, NORDER, NORDER_OBS, sections 112-118): the creation token that ties an actor to the call that
 *     created it and how the actor ended, lifetime generations of keyed resources, the same-build verdict of a
 *     conversion, the sufficiency state of every planned need and the cause of every call without a return, and the
 *     instances of static order edges in the run with the order observed per relation row.  Their records are defined
 *     here so that the schema changes once; no tool writes them in this revision, each section is written only when it
 *     has a record, and every existing record and section keeps its layout and version.
 *
 * Byte order: the records of this layer are written and read by the explicit little-endian codec of
 * kz/core/record_codec.hpp (field by field from the field lists below), never by copying host structs;
 * sections 100-119 are the sections the container layer serializes that way (kz::section_is_serialized).
 *
 * Not usable alone: include kz_schema.h.
 */
#ifndef KZ_SCHEMA_KZ_NEUTRAL_H
#define KZ_SCHEMA_KZ_NEUTRAL_H

#ifndef KZ_SCHEMA_KZ_SCHEMA_H
#error "include kz/schema/kz_schema.h, not this file"
#endif

enum kz_neutral_section_id {
  KZ_SEC_NRUN = 100,
  KZ_SEC_NSITES = 101,
  KZ_SEC_NSLOTS = 102,
  KZ_SEC_NEVENTS = 103,
  KZ_SEC_NVALUES = 104,
  KZ_SEC_NACTORS = 105,
  KZ_SEC_NLOSS = 106,
  KZ_SEC_NCTL = 107,
  KZ_SEC_NOWNER = 111, /* schema revision 6 (108-110 are kz_replay.h) */
  KZ_SEC_NACTOR_TOKEN = 112, /* schema revision 12: 112-118 */
  KZ_SEC_NLIFE = 113,
  KZ_SEC_NBUILD = 114,
  KZ_SEC_NSUFFICE = 115,
  KZ_SEC_NCALLEND = 116,
  KZ_SEC_NORDER = 117,
  KZ_SEC_NORDER_OBS = 118
};

/* The range of section ids whose records the container layer serializes field by field. */
#define KZ_SEC_SERIALIZED_FIRST 100u
#define KZ_SEC_SERIALIZED_LAST 119u

#define KZ_NEUTRAL_SECTIONS(S)       \
  S(NRUN, 100, kz_nrun, 1)           \
  S(NSITES, 101, kz_nsite, 1)        \
  S(NSLOTS, 102, kz_nslot, 1)        \
  S(NEVENTS, 103, kz_nevent, 1)      \
  S(NVALUES, 104, kz_nvalue, 1)      \
  S(NACTORS, 105, kz_nactor, 1)      \
  S(NLOSS, 106, kz_nloss, 1)         \
  S(NCTL, 107, kz_nctl, 1)           \
  S(NOWNER, 111, kz_nowner, 1)         \
  S(NACTOR_TOKEN, 112, kz_ntoken, 1)   \
  S(NLIFE, 113, kz_nlife, 1)           \
  S(NBUILD, 114, kz_nbuild, 1)         \
  S(NSUFFICE, 115, kz_nsuffice, 1)     \
  S(NCALLEND, 116, kz_ncallend, 1)     \
  S(NORDER, 117, kz_norder, 1)         \
  S(NORDER_OBS, 118, kz_norderobs, 1)

/*
 * The run (one record).  `backend` is the execution family, `backend_name` the backend inside it
 * (F0: the RUNMETA backend, e.g. "perf-sample"; kzrt: "kzrt" and the adapter).  `start_evidence` says
 * why the record covers the process from its start (KZ_NSTART), `clock` which clock `t` is.  The tags
 * are the versions of event_semantics.md 2.13 (empty = not given).  `raw_sha256` is the sha256 of a raw
 * stream that is not in this container (kzrt-dump), zero if the raw evidence is in this container;
 * `manifest_sha256` the site manifest the runtime was given (zero = none); `raw_meta` the F0 RUNMETA.
 * `end_kind` / `end_value` (schema revision 5; were reserved, zero in every container written before): how the record
 * says the run ended (KZ_NEND; kzrt-dump 1.4 RUNX end_kind / end_flags / end_value, event_semantics.md section 12):
 * not recorded (F0, a dump before 1.4), the process's own dump, or a dump recovered from the runtime's store after the
 * process ended with the signal or exit status the harness gave (`end_value`).  Flags `recovered` and `dumped` say
 * the same of the dump itself (its header bit; the process had completed its own dump first).
 */
#define KZ_NRUN_FIELDS(X, T)                                                         \
  X(T, uint8_t, backend, )        /* KZ_NBACKEND */                                  \
  X(T, uint8_t, clock, )          /* KZ_NCLOCK */                                    \
  X(T, uint8_t, start_evidence, ) /* KZ_NSTART */                                    \
  X(T, uint8_t, end_kind, )       /* KZ_NEND (revision 5) */                        \
  X(T, uint32_t, flags, )         /* KZ_NRUN_FLAG */                                 \
  X(T, uint64_t, run_id, )        /* the harness's run id (kzrt RUN), 0 = not given */ \
  X(T, uint32_t, instance_id, )   /* instance of a multi-instance run, 0 = not given */ \
  X(T, uint32_t, end_value, )     /* the signal or exit status of KZ_NEND_SIGNAL / EXIT, else 0 */ \
  X(T, uint8_t, manifest_sha256, [32])                                               \
  X(T, uint8_t, raw_sha256, [32])                                                    \
  X(T, kz_str, backend_name, )                                                       \
  X(T, kz_str, runtime, )                                                            \
  X(T, kz_str, adapter, )                                                            \
  X(T, kz_str, compiler, )                                                           \
  X(T, kz_str, target, )                                                             \
  X(T, kz_ref, raw_meta, )

/*
 * A logical site (event_semantics.md 2.4-2.5).  `kind` is how the replay reads it (a call of the
 * function `symbol`, its return, or a point), `phase` the manifest phase where the backend has one.
 * `label` is the plan's / manifest's unique name, `op` the manifest operation (empty for F0), `source`
 * and `line` the source position the plan or manifest names, `at` where the backend bound it.  `bind`
 * is the binding id when the site has exactly one binding (F0: the plan site id), else KZ_NONE32: the
 * event's own `bind` is authoritative.  `decoder` is F0's KZ_DECODER of the site (how its identity join
 * read it), kept for reports.  Flag `attached`: the site was observed for the whole record (F0: probes
 * attached throughout; kzrt: every binding of the site is bound, inlined or duplicated); a site
 * without it is not an observed point of the record.  Flag `call-site`: a call or return observed at the
 * call site (a manifest `callee=`), not inside the callee; the replay also reads it as the profile point
 * its label names, if any (F0 observes the two with two probes).
 */
#define KZ_NSITE_FIELDS(X, T)                                                        \
  X(T, uint32_t, id, )       /* the backend's logical site id */                     \
  X(T, uint8_t, kind, )      /* KZ_NSITE_KIND */                                     \
  X(T, uint8_t, phase, )     /* KZ_NPHASE */                                         \
  X(T, uint16_t, decoder, )  /* KZ_DECODER (F0), none otherwise */                   \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, bind, )                                                             \
  X(T, uint32_t, flags, )    /* KZ_NSITE_FLAG */                                     \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, kz_str, label, )                                                              \
  X(T, kz_str, op, )                                                                 \
  X(T, kz_str, symbol, )                                                             \
  X(T, kz_str, source, )                                                             \
  X(T, kz_str, at, )                                                                 \
  X(T, kz_ref, raw, )        /* SITES record (F0), or null */

/*
 * A value slot of a site: its role (what the value means, as the profile names it), its kind (an
 * integer, a text, a pointer recorded as a pointer only) and where the backend took it (`from`: kzrt
 * "arg<k>|ret|out<k>|operand<k>", F0 the slot source "reg|stack|ip|mem|str").
 */
#define KZ_NSLOT_FIELDS(X, T)                                                        \
  X(T, uint32_t, site, )   /* NSITES index */                                        \
  X(T, uint8_t, slot, )                                                              \
  X(T, uint8_t, kind, )    /* KZ_NVAL */                                             \
  X(T, uint8_t, width, )   /* bytes kept, 0 = not stated */                          \
  X(T, uint8_t, flags, )   /* KZ_NSLOT_FLAG */                                       \
  X(T, kz_str, role, )                                                               \
  X(T, kz_str, from, )                                                               \
  X(T, kz_ref, raw, )      /* SITE_SLOTS record (F0), or null */

/*
 * A neutral event.  The section's order is the container's merged order: it is NOT an order between
 * actors (the replay never takes it as one); the per-actor order is `aseq`.  `link` is the event of the
 * same call instance that opened it (the call / before event), where the backend establishes it.
 * The values are NVALUES[first_value .. first_value + nvals), valid values only.
 */
#define KZ_NEVENT_FIELDS(X, T)                                                       \
  X(T, uint64_t, t, )          /* NRUN.clock; evidence only, never an HB edge */     \
  X(T, uint32_t, actor, )      /* NACTORS index */                                   \
  X(T, uint32_t, aseq, )       /* per-actor order */                                 \
  X(T, uint32_t, site, )       /* NSITES index */                                    \
  X(T, uint32_t, bind, )       /* binding id */                                      \
  X(T, uint32_t, link, )       /* NEVENTS index of the call instance's opening event, or KZ_NONE32 */ \
  X(T, uint32_t, first_value, )                                                      \
  X(T, uint16_t, nvals, )                                                            \
  X(T, uint16_t, flags, )      /* KZ_NEV_FLAG */                                     \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, uint64_t, raw_off, )    /* offset in the raw stream of NRUN.raw_sha256, or KZ_NONE64 */ \
  X(T, kz_ref, raw, )          /* RAW_EVENTS record (F0), or null */

/* One observed value of an event (an unobserved value has no record).  A text keeps its length in `v`. */
#define KZ_NVALUE_FIELDS(X, T)                                                       \
  X(T, uint64_t, v, )                                                                \
  X(T, uint32_t, event, )  /* NEVENTS index */                                       \
  X(T, uint8_t, slot, )                                                              \
  X(T, uint8_t, kind, )    /* KZ_NVAL */                                             \
  X(T, uint8_t, flags, )   /* KZ_NVALUE_FLAG */                                      \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, kz_str, text, )

/*
 * An actor of the run: one execution context with its creation generation and its backend binding.
 * The identities are kept apart (SPEC 1.2, event_semantics.md 2.3): the executor (this record), its
 * creating executor (`creator`), the owning application (owner_*: level, evidence, the identity value
 * the creation request passed, the creation request event and its calling code), the announced id.
 * An owner not recorded is undetermined, never "no owner" (flag owner-recorded absent).
 * `join_flags` is the evidence of the backend's identity join (KZ_THREAD_FLAG; F0 THREADS.flags).
 * `spawn_return` is the return event of the call that created this actor, bound to it by the id the
 * call assigned (the identity join's id match), KZ_NONE32 where not bound.
 */
#define KZ_NACTOR_FIELDS(X, T)                                                       \
  X(T, uint64_t, native_key, )   /* the binding's native key (Linux tid), 0 if none */ \
  X(T, uint64_t, request_addr, ) /* link-time return address of the owner request (flag request-addr-known) */ \
  X(T, uint32_t, backend_id, )   /* F0: tid; kzrt: the runtime's actor id (provisional bit kept) */ \
  X(T, uint32_t, generation, )   /* creation generation of the native key in this run */ \
  X(T, uint32_t, process, )      /* native process key (pid), or KZ_NONE32 */       \
  X(T, uint32_t, flags, )        /* KZ_NACTOR_FLAG */                               \
  X(T, uint32_t, join_flags, )   /* KZ_THREAD_FLAG */                               \
  X(T, uint32_t, creator, )      /* NACTORS index, or KZ_NONE32 */                  \
  X(T, uint32_t, announced_id, ) /* or KZ_NONE32 */                                 \
  X(T, uint32_t, creator_event, ) /* NEVENTS index, or KZ_NONE32 */                 \
  X(T, uint32_t, first_event, )  /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, spawn_return, ) /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, boot_link, )    /* NACTORS index of the provisional actor joined to this one, or KZ_NONE32 */ \
  X(T, uint8_t, binding, )       /* KZ_NBIND */                                     \
  X(T, uint8_t, native_kind, )   /* KZ_NKEY */                                      \
  X(T, uint8_t, owner_level, )   /* KZ_OWNER_LEVEL */                               \
  X(T, uint8_t, rsv0, )                                                             \
  X(T, uint32_t, owner_evidence, ) /* KZ_OWNER_EVIDENCE */                          \
  X(T, uint32_t, owner_id, )     /* identity value of the owning application, or KZ_NONE32 */ \
  X(T, uint32_t, owner_creator, ) /* NACTORS index the owner record names, or KZ_NONE32 */ \
  X(T, uint32_t, owner_request, ) /* NEVENTS index of the creation request, or KZ_NONE32 */ \
  X(T, kz_str, name, )                                                              \
  X(T, kz_str, owner_app, )                                                         \
  X(T, kz_str, request_func, )   /* the request's calling function where the backend's binding names it */ \
  X(T, kz_ref, raw, )            /* THREADS record (F0), or null */                 \
  X(T, kz_ref, raw_owner, )      /* ACTOR_OWNERS record (F0), or null */

/*
 * A loss (event_semantics.md 2.11, 8.4).  `begin`/`end` are the NEVENTS interval the loss may affect
 * (end KZ_NONE32: to the end of the record); `actor` the actor it is attributed to (KZ_NONE32: not
 * attributed); `unit_kind`/`unit` where it was counted (F0: the CPU ring of a kernel report; kzrt: the
 * runtime actor's buffer, the bootstrap buffer or a noted key).  Loss 0 means "no loss counted".
 */
#define KZ_NLOSS_FIELDS(X, T)                                                        \
  X(T, uint64_t, count, )                                                            \
  X(T, uint32_t, begin, )                                                            \
  X(T, uint32_t, end, )                                                              \
  X(T, uint32_t, actor, )                                                            \
  X(T, uint32_t, unit, )                                                             \
  X(T, uint8_t, kind, )      /* KZ_NLOSS */                                          \
  X(T, uint8_t, unit_kind, ) /* KZ_NLOSS_UNIT */                                     \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, rsv1, )                                                             \
  X(T, kz_ref, raw, )        /* LOSS record (F0), or null */

/*
 * A realized control record (event_semantics.md 5): what the controller did, apart from the plan.
 * Actors are NACTORS indices; `event` is the observation event of the hit; `ctl_seq` is the order of
 * control decisions (the controller's order, never a program order).  `t` is the controller's clock (kzrt: the
 * runtime's; F0 converted with a hold record: the hold supervisor's CLOCK_MONOTONIC), evidence only.  `raw_off` is
 * the record's byte offset in the raw stream: kzrt the dump, F0 the hold record (BASIS convert.hold.sha256).
 */
#define KZ_NCTL_FIELDS(X, T)                                                         \
  X(T, uint64_t, t, )                                                                \
  X(T, uint32_t, actor, )    /* the writer: the actor that decided */                \
  X(T, uint32_t, subject, )  /* the actor whose hit it is, or KZ_NONE32 */           \
  X(T, uint32_t, other, )    /* FIRE: the resumed actor; RESUME: the releasing actor; or KZ_NONE32 */ \
  X(T, uint32_t, event, )    /* NEVENTS index of the hit's observation, or KZ_NONE32 */ \
  X(T, uint32_t, site, )     /* NSITES index, or KZ_NONE32 */                        \
  X(T, uint32_t, bind, )                                                             \
  X(T, uint32_t, ctl_seq, )                                                          \
  X(T, uint32_t, point, )                                                            \
  X(T, uint32_t, hit, )                                                              \
  X(T, uint32_t, gate, )                                                             \
  X(T, uint8_t, what, )      /* KZ_NCTL_WHAT */                                      \
  X(T, uint8_t, reason, )    /* KZ_NCTL_REASON */                                    \
  X(T, uint8_t, ctx, )       /* KZ_NCTX */                                           \
  X(T, uint8_t, cflags, )    /* KZ_NCTL_FLAG */                                      \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, uint64_t, raw_off, )  /* offset in the raw stream, or KZ_NONE64 */

/*
 * An owner named after the spawn (schema revision 6; the instrumented family, kzs-sites 3 `app-request`,
 * kz/trace/identity_rule.hpp): one record per actor tied by the id to a spawn call whose creation request (the
 * outermost one open on the creating actor when the call began) is an app-request, i.e. a request that creates the
 * application and hands its value back only after the task exists.  It says at which events the owner was
 * established: `request` the app-request record (the actor's owner_request), `publish` the owner-publish record of
 * the same request instance whose value `owner_id` is (paired by its link), `request_return` the record that closed
 * the request, `check` the first owner-check record compared (KZ_NONE32 where there is none of each); `flags`
 * (KZ_NOWNER_FLAG) what was found and `level` the owner level the actor got (KZ_OWNER_LEVEL).  The tie itself
 * (creator, creator_event, spawn_return) is the actor's.  None of these events orders anything: the order of the
 * task's uses and the publication is the replay's business.  The section is written only when it has a record.
 */
#define KZ_NOWNER_FIELDS(X, T)                                                       \
  X(T, uint32_t, actor, )          /* NACTORS index */                               \
  X(T, uint32_t, request, )        /* NEVENTS index of the app-request record */     \
  X(T, uint32_t, publish, )        /* NEVENTS index of the owner-publish record, or KZ_NONE32 */ \
  X(T, uint32_t, request_return, ) /* NEVENTS index of the request-return record, or KZ_NONE32 */ \
  X(T, uint32_t, check, )          /* NEVENTS index of the first owner-check record, or KZ_NONE32 */ \
  X(T, uint32_t, owner_id, )       /* the value published, or KZ_NONE32 */           \
  X(T, uint32_t, flags, )          /* KZ_NOWNER_FLAG */                              \
  X(T, uint8_t, level, )           /* KZ_OWNER_LEVEL the actor got */                \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )

/*
 * Schema revision 12: sections 112-118, side tables beside the records above (none of which changes).  No tool writes
 * them in this revision.
 *
 * NACTOR_TOKEN (112).  One record per actor (`actor`) for which the run recorded a creation token or a result of the
 * token join.  The token is the pair (`token_actor`, `token_aseq`): the creating actor's runtime id and the per-actor
 * sequence number of its spawn record, both assigned by the runtime on the creating side, so the pair is unique per
 * creation call whatever thread handle, thread id or program-level id values are reused.  `spawn_event`,
 * `spawn_return` and `begin_event` are the spawn record, its return record (the creation status and the out value)
 * and the child's first record, which carries the token; `end_event` and `end_kind` how the actor ended.  `tie` is the
 * result of the join (tied by the token, the token lost, two spawn records with the token, no token in the record).
 * An id the actor reported itself (`self_id` at `self_event`) is only compared with the creator-side id of the same
 * token (`creator_id`; `self_check`); it never makes or confirms a tie.  `start` / `arg` are the start routine and
 * argument the child's first record names; `root_kind` / `root_func` the static root the start value maps to.
 */
#define KZ_NTOKEN_FIELDS(X, T)                                                       \
  X(T, uint64_t, start, )         /* link-time start routine value, 0 = not recorded */ \
  X(T, uint64_t, arg, )           /* the start argument value */                     \
  X(T, uint32_t, actor, )         /* NACTORS index */                                \
  X(T, uint32_t, token_actor, )   /* the creating runtime actor id, KZ_NONE32 = no token */ \
  X(T, uint32_t, token_aseq, )    /* aseq of the spawn record on the creator */      \
  X(T, uint32_t, spawn_event, )   /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, spawn_return, )  /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, begin_event, )   /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, end_event, )     /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, self_id, )       /* the id the actor reported, or KZ_NONE32 */      \
  X(T, uint32_t, self_event, )    /* NEVENTS index of that report, or KZ_NONE32 */   \
  X(T, uint32_t, creator_id, )    /* the creator-side id of the token, or KZ_NONE32 */ \
  X(T, uint8_t, tie, )            /* KZ_NTIE */                                      \
  X(T, uint8_t, end_kind, )       /* KZ_NAEND */                                     \
  X(T, uint8_t, self_check, )     /* KZ_NSELF */                                     \
  X(T, uint8_t, root_kind, )      /* KZ_NROOT */                                     \
  X(T, uint32_t, flags, )         /* KZ_NTOKEN_FLAG */                               \
  X(T, kz_str, root_func, )       /* the static root function, empty when not mapped */ \
  X(T, kz_str, reason, )          /* why the tie or the root is not established */

/*
 * NLIFE (113).  One record per generation of a keyed resource: the resource kind, its key space (`key_space`, e.g. a
 * table and its slot index, a named-object class) and key (`key_value`, `key_text`), the generation number per (kind,
 * space, key), the events that began and ended it, the creating and the ending actor, and how it ended.  `previous`
 * links the generations of one key.  Flags name what the generations show: a key reused while an older generation has
 * no end (`reuse-while-stale`), a new generation of the same name or slot, a deregistration.
 */
#define KZ_NLIFE_FIELDS(X, T)                                                        \
  X(T, uint64_t, key_value, )     /* the key (slot, id, handle, native id), per resource kind */ \
  X(T, uint32_t, generation, )                                                       \
  X(T, uint32_t, begin_event, )   /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, end_event, )     /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, creator, )       /* NACTORS index, or KZ_NONE32 */                  \
  X(T, uint32_t, ender, )         /* NACTORS index, or KZ_NONE32 */                  \
  X(T, uint32_t, previous, )      /* NLIFE index of the previous generation, or KZ_NONE32 */ \
  X(T, uint8_t, resource, )       /* KZ_NRES */                                      \
  X(T, uint8_t, end_kind, )       /* KZ_NLIFE_END */                                 \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, flags, )         /* KZ_NLIFE_FLAG */                                \
  X(T, kz_str, key_space, )                                                          \
  X(T, kz_str, key_text, )        /* the key as text (a name), empty when none */

/*
 * NBUILD (114).  The same-build verdict of a conversion: one record per link checked (the site manifest against the
 * plan, the plan's facts against the facts given, each executable and module of the run against the derived build's
 * basis, the derived build's facts against the plan's facts section by section, build-ids) with the expected and the
 * observed sha256, and one record of link `run` with the verdict of the whole: `confirmed` only when every link is;
 * `refused` when a link differs; else `unconfirmed`.  Anything but `confirmed` is a build mismatch for a strong
 * conclusion.
 */
#define KZ_NBUILD_FIELDS(X, T)                                                       \
  X(T, uint8_t, link, )           /* KZ_NBUILD_LINK */                               \
  X(T, uint8_t, verdict, )        /* KZ_NBUILD_VERDICT */                            \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, flags, )         /* KZ_NBUILD_FLAG */                               \
  X(T, uint32_t, index, )         /* the executable or module number within its link, else 0 */ \
  X(T, uint32_t, rsv1, )                                                             \
  X(T, uint8_t, expected_sha256, [32])                                               \
  X(T, uint8_t, observed_sha256, [32])                                               \
  X(T, kz_str, path, )                                                               \
  X(T, kz_str, build_id, )        /* the build-id observed, empty when not recorded */ \
  X(T, kz_str, reason, )

/*
 * NSUFFICE (115).  The sufficiency state of each planned need at each of its sites: the value was recorded, the event
 * was recorded without the value, the text was cut at its bound, the site was attached and not reached, a loss interval
 * covers it, the site has no binding, the need was not planned, or the build does not match.  `reason` is the reason of
 * the judgement rules' vocabulary the state maps to (empty for `present`).
 */
#define KZ_NSUFFICE_FIELDS(X, T)                                                     \
  X(T, uint32_t, site, )          /* NSITES index, or KZ_NONE32 */                   \
  X(T, uint32_t, event, )         /* NEVENTS index of the evidence, or KZ_NONE32 */  \
  X(T, uint32_t, loss, )          /* NLOSS index, or KZ_NONE32 */                    \
  X(T, uint32_t, actor, )         /* NACTORS index, or KZ_NONE32 */                  \
  X(T, uint8_t, state, )          /* KZ_NSUFF */                                     \
  X(T, uint8_t, slot, )           /* the value slot the need names, 0xff = none */   \
  X(T, uint16_t, flags, )         /* KZ_NSUFF_FLAG */                                \
  X(T, kz_str, need, )            /* the plan's need */                              \
  X(T, kz_str, reason, )                                                             \
  X(T, kz_str, note, )

/*
 * NCALLEND (116).  One record per call record of the run whose return was not recorded, with its cause and the
 * evidence of the cause: the actor ended, the process ended inside the call, the static facts say the call does not
 * return, the record stopped, a loss follows, the return has no binding, a non-local exit is possible, the actor was
 * held at the end; `unknown` otherwise.
 */
#define KZ_NCALLEND_FIELDS(X, T)                                                     \
  X(T, uint32_t, event, )         /* NEVENTS index of the call record */             \
  X(T, uint32_t, actor, )         /* NACTORS index */                                \
  X(T, uint32_t, evidence, )      /* NEVENTS index of the evidence, or KZ_NONE32 */  \
  X(T, uint32_t, loss, )          /* NLOSS index, or KZ_NONE32 */                    \
  X(T, uint8_t, cause, )          /* KZ_NCALLEND */                                  \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, kz_str, evidence_text, )

/*
 * NORDER (117).  One record per instance of a static order edge in the run (`edge` the edge id of the static order
 * output whose facts the run's build was extracted to; `rule` and `kind` copied from it): the events and actors it
 * joins, the creation token it rests on (spawn edges), the object generation (lock edges), the exit kind and the
 * iteration count (wait edges), and the result: instantiated, not instantiated, a wait that left by its budget (no
 * edge), an operation that failed (no edge, the edge's premise falsified for this instance), no token.
 */
#define KZ_NORDER_FIELDS(X, T)                                                       \
  X(T, uint32_t, from_event, )    /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, to_event, )      /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, from_actor, )    /* NACTORS index, or KZ_NONE32 */                  \
  X(T, uint32_t, to_actor, )      /* NACTORS index, or KZ_NONE32 */                  \
  X(T, uint32_t, life, )          /* NLIFE index of the object generation, or KZ_NONE32 */ \
  X(T, uint32_t, token, )         /* NACTOR_TOKEN index, or KZ_NONE32 */             \
  X(T, uint32_t, iterations, )    /* wait instances: iterations counted, else 0 */   \
  X(T, uint8_t, rule, )           /* KZ_NORDER_RULE */                               \
  X(T, uint8_t, kind, )           /* KZ_NORDER_KIND */                               \
  X(T, uint8_t, result, )         /* KZ_NORDER_RESULT */                             \
  X(T, uint8_t, exit_kind, )      /* KZ_NWAIT_EXIT */                                \
  X(T, uint32_t, flags, )         /* KZ_NORDER_FLAG */                               \
  X(T, kz_str, edge, )            /* the static edge id */                           \
  X(T, kz_str, instance_key, )                                                       \
  X(T, kz_str, reason, )

/*
 * NORDER_OBS (118).  The order observed in the run per relation row (`row_key`, the row's use-site key) and consumer
 * actor: the write and the use events and the edge instance that orders them, if any.  `order_static` is the row's
 * static order value, copied for the reader and never merged with the observed one.
 */
#define KZ_NORDEROBS_FIELDS(X, T)                                                    \
  X(T, uint32_t, actor, )         /* NACTORS index of the consumer */                \
  X(T, uint32_t, writer_actor, )  /* NACTORS index, or KZ_NONE32 */                  \
  X(T, uint32_t, write_event, )   /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, use_event, )     /* NEVENTS index, or KZ_NONE32 */                  \
  X(T, uint32_t, instance, )      /* NORDER index, or KZ_NONE32 */                   \
  X(T, uint8_t, observed, )       /* KZ_NOBS */                                      \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )         /* KZ_NOBS_FLAG */                                 \
  X(T, kz_str, row_key, )                                                            \
  X(T, kz_str, order_static, )                                                       \
  X(T, kz_str, reason, )

#define KZ_NEUTRAL_RECORDS(R)          \
  R(kz_nrun, KZ_NRUN_FIELDS)           \
  R(kz_nsite, KZ_NSITE_FIELDS)         \
  R(kz_nslot, KZ_NSLOT_FIELDS)         \
  R(kz_nevent, KZ_NEVENT_FIELDS)       \
  R(kz_nvalue, KZ_NVALUE_FIELDS)       \
  R(kz_nactor, KZ_NACTOR_FIELDS)       \
  R(kz_nloss, KZ_NLOSS_FIELDS)         \
  R(kz_nctl, KZ_NCTL_FIELDS)           \
  R(kz_nowner, KZ_NOWNER_FIELDS)       \
  R(kz_ntoken, KZ_NTOKEN_FIELDS)       \
  R(kz_nlife, KZ_NLIFE_FIELDS)         \
  R(kz_nbuild, KZ_NBUILD_FIELDS)       \
  R(kz_nsuffice, KZ_NSUFFICE_FIELDS)   \
  R(kz_ncallend, KZ_NCALLEND_FIELDS)   \
  R(kz_norder, KZ_NORDER_FIELDS)       \
  R(kz_norderobs, KZ_NORDEROBS_FIELDS)

KZ_NEUTRAL_RECORDS(KZ_RECORD)

/* ------------------------------------------------------------------------- */
/* Enumerations.  0 is always the conservative / "not established" value.   */
/* ------------------------------------------------------------------------- */

#define KZ_ENUM_NBACKEND(ITEM, P) ITEM(P, NONE, 0, "none") ITEM(P, F0, 1, "F0") ITEM(P, KZRT, 2, "kzrt")

/* ADAPTER: the adapter's own clock, not CLOCK_MONOTONIC (the RUN adapter tag names it); it may be settable, so even
 * the order inside an actor is taken from aseq only. */
#define KZ_ENUM_NCLOCK(ITEM, P)                                                        \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, MONOTONIC, 1, "CLOCK_MONOTONIC")             \
  ITEM(P, ADAPTER, 2, "adapter-clock") ITEM(P, NONE, 3, "none")

/* Why the record covers the process from its start (the replay's start evidence `init`). */
#define KZ_ENUM_NSTART(ITEM, P)                                                        \
  ITEM(P, NONE, 0, "none") ITEM(P, EXEC_MAPPING, 1, "exec-mapping")                   \
  ITEM(P, BOOTSTRAP, 2, "bootstrap-buffer")

/* RECOVERED (revision 5): a kzrt dump made from the runtime's store after the process ended (dump flag recovered), never
 * `complete`; DUMPED (revision 5): recovered after the process had completed its own dump, which holds the same records. */
#define KZ_ENUM_NRUN_FLAGS(ITEM, P) /* set */                                         \
  ITEM(P, LOSS_EVIDENCE, 0x1, "loss-evidence") ITEM(P, LOSS_PRESENT, 0x2, "loss-present") \
  ITEM(P, OWNERS_RECORDED, 0x4, "owners-recorded") ITEM(P, COMPLETE, 0x8, "complete")  \
  ITEM(P, CONTROL_RECORDED, 0x10, "control-recorded") ITEM(P, STOPPED, 0x20, "stopped") \
  ITEM(P, RECOVERED, 0x40, "recovered") ITEM(P, DUMPED, 0x80, "dumped")

/* How the record says the run ended (revision 5, NRUN end_kind).  IN_PROCESS: the process wrote the dump itself (it had
 * not ended then).  SIGNAL / EXIT: a dump recovered from the store after the process ended, by the signal or with the
 * exit status (NRUN end_value) the harness gave; UNSTATED: recovered, the harness gave neither. */
#define KZ_ENUM_NEND(ITEM, P)                                                          \
  ITEM(P, NOT_RECORDED, 0, "not-recorded") ITEM(P, IN_PROCESS, 1, "in-process")       \
  ITEM(P, SIGNAL, 2, "signal") ITEM(P, EXIT, 3, "exit") ITEM(P, UNSTATED, 4, "unstated")

#define KZ_ENUM_NSITE_KIND(ITEM, P)                                                    \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, CALL, 1, "call") ITEM(P, RETURN, 2, "return") \
  ITEM(P, POINT, 3, "point")

#define KZ_ENUM_NPHASE(ITEM, P)                                                        \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, BEFORE, 1, "before") ITEM(P, EFFECT, 2, "effect") \
  ITEM(P, AFTER, 3, "after") ITEM(P, EXIT, 4, "exit") ITEM(P, POINT, 5, "point")

#define KZ_ENUM_NSITE_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, ATTACHED, 0x1, "attached") ITEM(P, PARTIAL_BINDING, 0x2, "partial-binding")  \
  ITEM(P, CALL_SITE, 0x4, "call-site")

#define KZ_ENUM_NVAL(ITEM, P)                                                          \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, INT, 1, "int") ITEM(P, TEXT, 2, "text")      \
  ITEM(P, POINTER, 3, "pointer")

#define KZ_ENUM_NSLOT_FLAGS(ITEM, P) /* set */ ITEM(P, SIGNED, 0x1, "signed")

#define KZ_ENUM_NVALUE_FLAGS(ITEM, P) /* set */ ITEM(P, TEXT_TRUNCATED, 0x1, "text-truncated")

#define KZ_ENUM_NEV_FLAGS(ITEM, P) /* set */                                          \
  ITEM(P, CLOCK, 0x1, "clock") ITEM(P, NO_VALUES, 0x2, "no-values")                  \
  ITEM(P, ASEQ_TARGET, 0x4, "aseq-target") ITEM(P, BOOT, 0x8, "boot")                \
  ITEM(P, PC, 0x10, "pc")

#define KZ_ENUM_NBIND(ITEM, P)                                                         \
  ITEM(P, UNRESOLVED, 0, "unresolved") ITEM(P, LINUX_TID, 1, "linux-tid")             \
  ITEM(P, KZRT_ACTOR, 2, "kzrt-actor") ITEM(P, KZRT_PROVISIONAL, 3, "kzrt-provisional")

/* The native key kinds (values equal to the runtime's key kinds); TASK_ID: the task id of an RTOS abstraction layer. */
#define KZ_ENUM_NKEY(ITEM, P)                                                          \
  ITEM(P, NONE, 0, "none") ITEM(P, LINUX_TID, 1, "linux-tid") ITEM(P, PTHREAD, 2, "pthread") \
  ITEM(P, TASK_ID, 3, "task-id") ITEM(P, OTHER, 15, "other")

#define KZ_ENUM_NACTOR_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, JOIN_RECORD, 0x1, "join-record") ITEM(P, INITIAL, 0x2, "process-initial")  \
  ITEM(P, OWNER_RECORDED, 0x4, "owner-recorded")                                     \
  ITEM(P, REQUEST_ADDR, 0x8, "request-addr-known") ITEM(P, EXITED, 0x10, "exited")  \
  ITEM(P, HELD_AT_END, 0x20, "held-at-end")

/* ABNORMAL_END (revision 5): the kzrt dump was recovered from the store after the process ended without its own dump: the
 * records are the prefix the store held, each actor's gap free, and what the actors did after their last record is not
 * known.  No interval inside the events (begin = end = the event count); a record an actor was writing when the process
 * ended is the actor's own partial loss. */
#define KZ_ENUM_NLOSS(ITEM, P)                                                         \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, NO_EVIDENCE, 1, "no-loss-evidence")          \
  ITEM(P, FLAGGED, 2, "flagged-without-record") ITEM(P, RECORDS_LOST, 3, "records-lost") \
  ITEM(P, THROTTLE, 4, "throttle") ITEM(P, RING_RESERVE_FAIL, 5, "ring-reserve-fail") \
  ITEM(P, TSEQ_GAP, 6, "tseq-gap") ITEM(P, OVERFLOW, 7, "overflow")                   \
  ITEM(P, PARTIAL, 8, "partial") ITEM(P, PREINIT, 9, "preinit")                       \
  ITEM(P, EXCLUDED, 10, "excluded") ITEM(P, UNLOADED, 11, "unloaded")                 \
  ITEM(P, REENTRY, 12, "reentry") ITEM(P, ACTOR_TABLE, 13, "actor-table")             \
  ITEM(P, UNRESOLVED, 14, "unresolved") ITEM(P, CTL_SKIPPED, 15, "ctl-skipped")       \
  ITEM(P, CTL_NOCOND, 16, "ctl-nocond") ITEM(P, MODULE_TABLE, 17, "module-table")     \
  ITEM(P, AFTER_STOP, 18, "after-stop") ITEM(P, NOTE_TABLE, 19, "note-table")         \
  ITEM(P, TRUNCATED_STREAM, 20, "truncated-stream") ITEM(P, ABNORMAL_END, 21, "abnormal-end")

#define KZ_ENUM_NLOSS_UNIT(ITEM, P)                                                    \
  ITEM(P, NONE, 0, "none") ITEM(P, CPU_RING, 1, "cpu-ring") ITEM(P, ACTOR_BUFFER, 2, "actor-buffer") \
  ITEM(P, BOOT_BUFFER, 3, "boot-buffer") ITEM(P, KEY, 4, "key")

#define KZ_ENUM_NCTL_WHAT(ITEM, P)                                                     \
  ITEM(P, NONE, 0, "none") ITEM(P, GATE_HOLD, 1, "gate-hold") ITEM(P, GATE_REFUSED, 2, "gate-refused") \
  ITEM(P, GATE_CONSUMED, 3, "gate-consumed") ITEM(P, GATE_BUSY, 4, "gate-busy")      \
  ITEM(P, FIRE, 5, "fire") ITEM(P, RESUME, 6, "resume") ITEM(P, TIMEOUT, 7, "timeout")

#define KZ_ENUM_NCTL_REASON(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, RESUMED, 1, "resumed") ITEM(P, SATISFIED, 2, "satisfied") \
  ITEM(P, CONSUMED, 3, "consumed") ITEM(P, NO_WAIT, 4, "no-wait") ITEM(P, PRE_ATTACH, 5, "pre-attach") \
  ITEM(P, CONTEXT, 6, "context") ITEM(P, NO_SLOT, 7, "no-slot")

/* DEFERRED: a release of a held thread's hit, acted on when it was resumed (kzp-hold 1 H4).  OBS_AFTER (revision 4): the
 * hit's observation event was recorded after the control acted on it -- F0's hold traps before the instruction its
 * probe observes, so a held thread's gate observation follows its resume and a release hit's observation follows the
 * release; the runtime writes the gate observation before the wait and evaluates a release after its observation
 * record, so its records have no such flag.  NO_OBS (revision 4): the record has no observation event of this hit
 * (`event` is KZ_NONE32); why is a note of the conversion. */
#define KZ_ENUM_NCTL_FLAGS(ITEM, P) /* set */                                         \
  ITEM(P, DEFERRED, 0x1, "deferred") ITEM(P, OBS_AFTER, 0x2, "observed-after-control") \
  ITEM(P, NO_OBS, 0x4, "no-observation-event")

/* The holdable context classes (event_semantics.md section 7; values equal to the runtime's).  REGISTRATION_INTERNAL
 * is the class of a point inside the scheduler or the OS abstraction layer's object registration code. */
#define KZ_ENUM_NCTX(ITEM, P)                                                          \
  ITEM(P, UNDECLARED, 0, "undeclared") ITEM(P, TASK, 1, "task") ITEM(P, LOCK_HELD, 2, "lock-held") \
  ITEM(P, ISR, 3, "isr") ITEM(P, REGISTRATION_INTERNAL, 4, "registration-internal")  \
  ITEM(P, TERMINATING, 5, "terminating") ITEM(P, UNSUPPORTED, 6, "unsupported")

/* NOWNER flags (revision 6).  PUBLISHED: one value was published in the request instance; PUBLISH_CONFLICT: two
 * different values were (none is used); RETURNED: a request-return record closed the request; STATUS_OK / STATUS_NOT_OK:
 * its declared status slot held the value of success / did not (or was not observed); CHECK_AGREED: every owner-check
 * value equals the published one; CHECK_DIFFERS: one does not (the owner is withdrawn). */
#define KZ_ENUM_NOWNER_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, PUBLISHED, 0x1, "published") ITEM(P, PUBLISH_CONFLICT, 0x2, "publish-conflict") \
  ITEM(P, RETURNED, 0x4, "returned") ITEM(P, STATUS_OK, 0x8, "status-ok")            \
  ITEM(P, STATUS_NOT_OK, 0x10, "status-not-ok") ITEM(P, CHECK_AGREED, 0x20, "check-agreed") \
  ITEM(P, CHECK_DIFFERS, 0x40, "check-differs")

/* Schema revision 12.  NACTOR_TOKEN: the join result, how the actor ended, the self-reported id against the creator's,
 * the static root. */
#define KZ_ENUM_NTIE(ITEM, P)                                                          \
  ITEM(P, UNDETERMINED, 0, "undetermined") ITEM(P, TOKEN_TIED, 1, "token-tied")       \
  ITEM(P, TOKEN_LOST, 2, "token-lost") ITEM(P, SPAWN_AMBIGUOUS, 3, "spawn-ambiguous") \
  ITEM(P, NO_TOKEN, 4, "no-creation-token")

#define KZ_ENUM_NAEND(ITEM, P)                                                         \
  ITEM(P, NOT_OBSERVED, 0, "not-observed") ITEM(P, ENTRY_RETURN, 1, "entry-return")   \
  ITEM(P, THREAD_EXIT, 2, "thread-exit") ITEM(P, CANCELLED, 3, "cancelled") ITEM(P, PROCESS_END, 4, "process-end")

#define KZ_ENUM_NSELF(ITEM, P)                                                         \
  ITEM(P, ABSENT, 0, "self-id-absent") ITEM(P, EQUAL, 1, "self-id-equal") ITEM(P, DIFFERS, 2, "self-id-differs")

#define KZ_ENUM_NROOT(ITEM, P)                                                         \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, START, 1, "start") ITEM(P, TRAMPOLINE, 2, "trampoline") \
  ITEM(P, TRAMPOLINE_UNRESOLVED, 3, "trampoline-unresolved") ITEM(P, SYMBOL_UNMAPPED, 4, "symbol-unmapped")

#define KZ_ENUM_NTOKEN_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, SPAWN_FAILED, 0x1, "spawn-failed") ITEM(P, BEGIN_LOST, 0x2, "begin-lost")   \
  ITEM(P, SLOT_TABLE_FULL, 0x4, "slot-table-full")

/* NLIFE. */
#define KZ_ENUM_NRES(ITEM, P)                                                          \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, TASK_SLOT, 1, "task-slot") ITEM(P, APP_RECORD, 2, "app-record") \
  ITEM(P, TASK_RECORD, 3, "task-record") ITEM(P, NAMED_OBJECT, 4, "named-object")     \
  ITEM(P, THREAD_HANDLE, 5, "thread-handle") ITEM(P, NATIVE_THREAD, 6, "native-thread")

#define KZ_ENUM_NLIFE_END(ITEM, P)                                                     \
  ITEM(P, NOT_OBSERVED, 0, "not-observed") ITEM(P, DELETED, 1, "deleted")             \
  ITEM(P, RELEASED_BY_CLEANUP, 2, "released-by-cleanup")                               \
  ITEM(P, THREAD_ENDED, 3, "thread-ended-without-delete") ITEM(P, OPEN_AT_END, 4, "open-at-end")

#define KZ_ENUM_NLIFE_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, REUSE_WHILE_STALE, 0x1, "reuse-while-stale") ITEM(P, RE_CREATION, 0x2, "re-creation") \
  ITEM(P, DEREGISTRATION, 0x4, "deregistration") ITEM(P, KEY_TEXT_TRUNCATED, 0x8, "key-text-truncated")

/* NBUILD. */
#define KZ_ENUM_NBUILD_LINK(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, RUN, 1, "run") ITEM(P, MANIFEST, 2, "manifest")   \
  ITEM(P, PLAN_FACTS, 3, "plan-facts") ITEM(P, EXECUTABLE, 4, "executable") ITEM(P, MODULE, 5, "module") \
  ITEM(P, BASIS_BINARY, 6, "basis-binary") ITEM(P, BASIS_FACTS, 7, "basis-facts") ITEM(P, BUILD_ID, 8, "build-id")

#define KZ_ENUM_NBUILD_VERDICT(ITEM, P)                                                \
  ITEM(P, UNCONFIRMED, 0, "unconfirmed") ITEM(P, CONFIRMED, 1, "confirmed") ITEM(P, REFUSED, 2, "refused")

#define KZ_ENUM_NBUILD_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, EXPECTED_KNOWN, 0x1, "expected-known") ITEM(P, OBSERVED_KNOWN, 0x2, "observed-known") \
  ITEM(P, MAPPED_BY_CONTENT, 0x4, "mapped-by-content")

/* NSUFFICE. */
#define KZ_ENUM_NSUFF(ITEM, P)                                                         \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, PRESENT, 1, "present") ITEM(P, VALUE_MISSING, 2, "value-missing") \
  ITEM(P, TEXT_PARTIAL, 3, "text-partial") ITEM(P, NOT_REACHED, 4, "not-reached")    \
  ITEM(P, UNKNOWN_LOSS, 5, "unknown-loss") ITEM(P, UNBOUND, 6, "unbound") ITEM(P, NOT_PLANNED, 7, "not-planned") \
  ITEM(P, BUILD_MISMATCH, 8, "build-mismatch")

#define KZ_ENUM_NSUFF_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, BRACKET_INTACT, 0x1, "bracket-intact") ITEM(P, CALL_ATTRIBUTED, 0x2, "call-attributed")

/* NCALLEND. */
#define KZ_ENUM_NCALLEND(ITEM, P)                                                      \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, ACTOR_ENDED, 1, "actor-ended")               \
  ITEM(P, PROCESS_ENDED_INSIDE, 2, "process-ended-inside") ITEM(P, STATIC_NO_RETURN, 3, "static-no-return") \
  ITEM(P, RECORD_STOPPED, 4, "record-stopped") ITEM(P, LOSS_AFTER, 5, "loss-after")  \
  ITEM(P, RETURN_UNBOUND, 6, "return-unbound") ITEM(P, NONLOCAL_EXIT, 7, "nonlocal-exit-possible") \
  ITEM(P, HELD_AT_END, 8, "held-at-end")

/* NORDER and NORDER_OBS. */
#define KZ_ENUM_NORDER_RULE(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, HB_SPAWN, 1, "HB-spawn") ITEM(P, HB_SEQ, 2, "HB-seq") \
  ITEM(P, HB_LOCK, 3, "HB-lock") ITEM(P, HB_AWAIT, 4, "HB-await") ITEM(P, L0_SCRIPT, 5, "L0-script")

#define KZ_ENUM_NORDER_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, HARD, 1, "hard") ITEM(P, CONDITIONAL, 2, "conditional") \
  ITEM(P, OBSERVED_ONLY, 3, "observed-only")

#define KZ_ENUM_NORDER_RESULT(ITEM, P)                                                 \
  ITEM(P, UNDETERMINED, 0, "undetermined") ITEM(P, INSTANTIATED, 1, "instantiated")   \
  ITEM(P, NOT_INSTANTIATED, 2, "not-instantiated") ITEM(P, WAIT_TIMED_OUT, 3, "wait-timed-out") \
  ITEM(P, STATUS_FAILED, 4, "status-failed") ITEM(P, TOKEN_MISSING, 5, "token-missing")

#define KZ_ENUM_NWAIT_EXIT(ITEM, P)                                                    \
  ITEM(P, NONE, 0, "none") ITEM(P, CONDITION, 1, "condition") ITEM(P, BUDGET, 2, "budget")

#define KZ_ENUM_NORDER_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, PREMISE_FALSIFIED, 0x1, "premise-falsified") ITEM(P, SAME_GENERATION, 0x2, "same-generation")

#define KZ_ENUM_NOBS(ITEM, P)                                                          \
  ITEM(P, UNDETERMINED, 0, "undetermined") ITEM(P, WRITE_FIRST, 1, "write-before-use") \
  ITEM(P, USE_FIRST, 2, "use-before-write") ITEM(P, USE_WITHOUT_WRITE, 3, "use-without-write") \
  ITEM(P, NOT_REACHED, 4, "not-reached")

#define KZ_ENUM_NOBS_FLAGS(ITEM, P) /* set */                                         \
  ITEM(P, ORDERED_BY_INSTANCE, 0x1, "ordered-by-instance") ITEM(P, VALUES_LINKED, 0x2, "values-linked")

#define KZ_NEUTRAL_ENUMS(D)                     \
  D(KZ_NBACKEND, KZ_ENUM_NBACKEND, 0)           \
  D(KZ_NCLOCK, KZ_ENUM_NCLOCK, 0)               \
  D(KZ_NSTART, KZ_ENUM_NSTART, 0)               \
  D(KZ_NRUN_FLAG, KZ_ENUM_NRUN_FLAGS, 1)        \
  D(KZ_NEND, KZ_ENUM_NEND, 0)                   \
  D(KZ_NSITE_KIND, KZ_ENUM_NSITE_KIND, 0)       \
  D(KZ_NPHASE, KZ_ENUM_NPHASE, 0)               \
  D(KZ_NSITE_FLAG, KZ_ENUM_NSITE_FLAGS, 1)      \
  D(KZ_NVAL, KZ_ENUM_NVAL, 0)                   \
  D(KZ_NSLOT_FLAG, KZ_ENUM_NSLOT_FLAGS, 1)      \
  D(KZ_NVALUE_FLAG, KZ_ENUM_NVALUE_FLAGS, 1)    \
  D(KZ_NEV_FLAG, KZ_ENUM_NEV_FLAGS, 1)          \
  D(KZ_NBIND, KZ_ENUM_NBIND, 0)                 \
  D(KZ_NKEY, KZ_ENUM_NKEY, 0)                   \
  D(KZ_NACTOR_FLAG, KZ_ENUM_NACTOR_FLAGS, 1)    \
  D(KZ_NLOSS, KZ_ENUM_NLOSS, 0)                 \
  D(KZ_NLOSS_UNIT, KZ_ENUM_NLOSS_UNIT, 0)       \
  D(KZ_NCTL_WHAT, KZ_ENUM_NCTL_WHAT, 0)         \
  D(KZ_NCTL_REASON, KZ_ENUM_NCTL_REASON, 0)     \
  D(KZ_NCTL_FLAG, KZ_ENUM_NCTL_FLAGS, 1)        \
  D(KZ_NCTX, KZ_ENUM_NCTX, 0)                   \
  D(KZ_NOWNER_FLAG, KZ_ENUM_NOWNER_FLAGS, 1)     \
  D(KZ_NTIE, KZ_ENUM_NTIE, 0)                   \
  D(KZ_NAEND, KZ_ENUM_NAEND, 0)                 \
  D(KZ_NSELF, KZ_ENUM_NSELF, 0)                 \
  D(KZ_NROOT, KZ_ENUM_NROOT, 0)                 \
  D(KZ_NTOKEN_FLAG, KZ_ENUM_NTOKEN_FLAGS, 1)    \
  D(KZ_NRES, KZ_ENUM_NRES, 0)                   \
  D(KZ_NLIFE_END, KZ_ENUM_NLIFE_END, 0)         \
  D(KZ_NLIFE_FLAG, KZ_ENUM_NLIFE_FLAGS, 1)      \
  D(KZ_NBUILD_LINK, KZ_ENUM_NBUILD_LINK, 0)     \
  D(KZ_NBUILD_VERDICT, KZ_ENUM_NBUILD_VERDICT, 0) \
  D(KZ_NBUILD_FLAG, KZ_ENUM_NBUILD_FLAGS, 1)    \
  D(KZ_NSUFF, KZ_ENUM_NSUFF, 0)                 \
  D(KZ_NSUFF_FLAG, KZ_ENUM_NSUFF_FLAGS, 1)      \
  D(KZ_NCALLEND, KZ_ENUM_NCALLEND, 0)           \
  D(KZ_NORDER_RULE, KZ_ENUM_NORDER_RULE, 0)     \
  D(KZ_NORDER_KIND, KZ_ENUM_NORDER_KIND, 0)     \
  D(KZ_NORDER_RESULT, KZ_ENUM_NORDER_RESULT, 0) \
  D(KZ_NWAIT_EXIT, KZ_ENUM_NWAIT_EXIT, 0)       \
  D(KZ_NORDER_FLAG, KZ_ENUM_NORDER_FLAGS, 1)    \
  D(KZ_NOBS, KZ_ENUM_NOBS, 0)                   \
  D(KZ_NOBS_FLAG, KZ_ENUM_NOBS_FLAGS, 1)

KZ_NEUTRAL_ENUMS(KZ_DEFINE_ENUM_ENTRY)

#define KZ_NEUTRAL_FORMATS(F)                         \
  F(kz_nrun, backend, ENUM, KZ_NBACKEND)              \
  F(kz_nrun, clock, ENUM, KZ_NCLOCK)                  \
  F(kz_nrun, start_evidence, ENUM, KZ_NSTART)         \
  F(kz_nrun, end_kind, ENUM, KZ_NEND)                 \
  F(kz_nrun, flags, FLAGS, KZ_NRUN_FLAG)              \
  F(kz_nrun, run_id, HEX, 0)                          \
  F(kz_nsite, kind, ENUM, KZ_NSITE_KIND)              \
  F(kz_nsite, phase, ENUM, KZ_NPHASE)                 \
  F(kz_nsite, decoder, ENUM, KZ_DECODER)              \
  F(kz_nsite, bind, IDX, 0)                           \
  F(kz_nsite, flags, FLAGS, KZ_NSITE_FLAG)            \
  F(kz_nslot, kind, ENUM, KZ_NVAL)                    \
  F(kz_nslot, flags, FLAGS, KZ_NSLOT_FLAG)            \
  F(kz_nevent, link, IDX, 0)                          \
  F(kz_nevent, flags, FLAGS, KZ_NEV_FLAG)             \
  F(kz_nevent, raw_off, HEX, 0)                       \
  F(kz_nvalue, v, HEX, 0)                             \
  F(kz_nvalue, kind, ENUM, KZ_NVAL)                   \
  F(kz_nvalue, flags, FLAGS, KZ_NVALUE_FLAG)          \
  F(kz_nactor, native_key, HEX, 0)                    \
  F(kz_nactor, request_addr, HEX, 0)                  \
  F(kz_nactor, backend_id, HEX, 0)                    \
  F(kz_nactor, process, IDX, 0)                       \
  F(kz_nactor, flags, FLAGS, KZ_NACTOR_FLAG)          \
  F(kz_nactor, join_flags, FLAGS, KZ_THREAD_FLAG)     \
  F(kz_nactor, creator, IDX, 0)                       \
  F(kz_nactor, announced_id, IDX, 0)                  \
  F(kz_nactor, creator_event, IDX, 0)                 \
  F(kz_nactor, first_event, IDX, 0)                   \
  F(kz_nactor, spawn_return, IDX, 0)                  \
  F(kz_nactor, boot_link, IDX, 0)                     \
  F(kz_nactor, binding, ENUM, KZ_NBIND)               \
  F(kz_nactor, native_kind, ENUM, KZ_NKEY)            \
  F(kz_nactor, owner_level, ENUM, KZ_OWNER_LEVEL)     \
  F(kz_nactor, owner_evidence, FLAGS, KZ_OWNER_EVIDENCE) \
  F(kz_nactor, owner_id, IDX, 0)                      \
  F(kz_nactor, owner_creator, IDX, 0)                 \
  F(kz_nactor, owner_request, IDX, 0)                 \
  F(kz_nloss, end, IDX, 0)                            \
  F(kz_nloss, actor, IDX, 0)                          \
  F(kz_nloss, unit, IDX, 0)                           \
  F(kz_nloss, kind, ENUM, KZ_NLOSS)                   \
  F(kz_nloss, unit_kind, ENUM, KZ_NLOSS_UNIT)         \
  F(kz_nctl, subject, IDX, 0)                         \
  F(kz_nctl, other, IDX, 0)                           \
  F(kz_nctl, event, IDX, 0)                           \
  F(kz_nctl, site, IDX, 0)                            \
  F(kz_nctl, what, ENUM, KZ_NCTL_WHAT)                \
  F(kz_nctl, reason, ENUM, KZ_NCTL_REASON)            \
  F(kz_nctl, ctx, ENUM, KZ_NCTX)                      \
  F(kz_nctl, cflags, FLAGS, KZ_NCTL_FLAG)             \
  F(kz_nctl, raw_off, HEX, 0)                         \
  F(kz_nowner, actor, IDX, 0)                         \
  F(kz_nowner, request, IDX, 0)                       \
  F(kz_nowner, publish, IDX, 0)                       \
  F(kz_nowner, request_return, IDX, 0)                \
  F(kz_nowner, check, IDX, 0)                         \
  F(kz_nowner, owner_id, IDX, 0)                      \
  F(kz_nowner, flags, FLAGS, KZ_NOWNER_FLAG)          \
  F(kz_nowner, level, ENUM, KZ_OWNER_LEVEL)          \
  F(kz_ntoken, start, HEX, 0)                         \
  F(kz_ntoken, arg, HEX, 0)                           \
  F(kz_ntoken, actor, IDX, 0)                         \
  F(kz_ntoken, token_actor, IDX, 0)                   \
  F(kz_ntoken, spawn_event, IDX, 0)                   \
  F(kz_ntoken, spawn_return, IDX, 0)                  \
  F(kz_ntoken, begin_event, IDX, 0)                   \
  F(kz_ntoken, end_event, IDX, 0)                     \
  F(kz_ntoken, self_id, IDX, 0)                       \
  F(kz_ntoken, self_event, IDX, 0)                    \
  F(kz_ntoken, creator_id, IDX, 0)                    \
  F(kz_ntoken, tie, ENUM, KZ_NTIE)                    \
  F(kz_ntoken, end_kind, ENUM, KZ_NAEND)              \
  F(kz_ntoken, self_check, ENUM, KZ_NSELF)            \
  F(kz_ntoken, root_kind, ENUM, KZ_NROOT)             \
  F(kz_ntoken, flags, FLAGS, KZ_NTOKEN_FLAG)          \
  F(kz_nlife, key_value, HEX, 0)                      \
  F(kz_nlife, begin_event, IDX, 0)                    \
  F(kz_nlife, end_event, IDX, 0)                      \
  F(kz_nlife, creator, IDX, 0)                        \
  F(kz_nlife, ender, IDX, 0)                          \
  F(kz_nlife, previous, IDX, 0)                       \
  F(kz_nlife, resource, ENUM, KZ_NRES)                \
  F(kz_nlife, end_kind, ENUM, KZ_NLIFE_END)           \
  F(kz_nlife, flags, FLAGS, KZ_NLIFE_FLAG)            \
  F(kz_nbuild, link, ENUM, KZ_NBUILD_LINK)            \
  F(kz_nbuild, verdict, ENUM, KZ_NBUILD_VERDICT)      \
  F(kz_nbuild, flags, FLAGS, KZ_NBUILD_FLAG)          \
  F(kz_nsuffice, site, IDX, 0)                        \
  F(kz_nsuffice, event, IDX, 0)                       \
  F(kz_nsuffice, loss, IDX, 0)                        \
  F(kz_nsuffice, actor, IDX, 0)                       \
  F(kz_nsuffice, state, ENUM, KZ_NSUFF)               \
  F(kz_nsuffice, flags, FLAGS, KZ_NSUFF_FLAG)         \
  F(kz_ncallend, evidence, IDX, 0)                    \
  F(kz_ncallend, loss, IDX, 0)                        \
  F(kz_ncallend, cause, ENUM, KZ_NCALLEND)            \
  F(kz_norder, from_event, IDX, 0)                    \
  F(kz_norder, to_event, IDX, 0)                      \
  F(kz_norder, from_actor, IDX, 0)                    \
  F(kz_norder, to_actor, IDX, 0)                      \
  F(kz_norder, life, IDX, 0)                          \
  F(kz_norder, token, IDX, 0)                         \
  F(kz_norder, rule, ENUM, KZ_NORDER_RULE)            \
  F(kz_norder, kind, ENUM, KZ_NORDER_KIND)            \
  F(kz_norder, result, ENUM, KZ_NORDER_RESULT)        \
  F(kz_norder, exit_kind, ENUM, KZ_NWAIT_EXIT)        \
  F(kz_norder, flags, FLAGS, KZ_NORDER_FLAG)          \
  F(kz_norderobs, writer_actor, IDX, 0)               \
  F(kz_norderobs, write_event, IDX, 0)                \
  F(kz_norderobs, use_event, IDX, 0)                  \
  F(kz_norderobs, instance, IDX, 0)                   \
  F(kz_norderobs, observed, ENUM, KZ_NOBS)            \
  F(kz_norderobs, flags, FLAGS, KZ_NOBS_FLAG)

#endif /* KZ_SCHEMA_KZ_NEUTRAL_H */

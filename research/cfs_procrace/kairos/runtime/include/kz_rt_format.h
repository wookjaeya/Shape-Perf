/* kz_rt_format.h -- the dump format "kzrt-dump" 1.0 (byte layout; shared by the runtime and the
 * host decoder).  Every multi-byte field is little-endian and written byte by byte, so the bytes
 * do not depend on the target's byte order, alignment or struct layout.
 *
 * Stream:
 *   file header (16 bytes): "KZRTDUMP", u16 major, u16 minor, u32 dump_flags
 *   records, each: u32 size (bytes of the whole record, a multiple of 4), u16 type, u16 sub,
 *                  then the body.  A reader skips a record type it does not know (by size).
 *   order: RUN, MODULE*, RESOURCE*, ACTOR*, LOSS*, POINT*, BUF*, END
 *   END: u64 bytes (stream bytes before END), u32 crc32 (IEEE, of those bytes).
 *
 * A BUF record's body is u32 owner (0 = the bootstrap/shared buffer, else a run actor id),
 * u32 nbytes, u32 cap, u32 rsv, then nbytes of buffer records (EVENT, CTL, AEXIT) exactly as the
 * runtime wrote them in memory.  Inside a buffer, a record whose size word is 0 was reserved but
 * not committed (bootstrap buffer only): the reader stops there and counts a partial record.
 *
 * Offsets below are from the start of the record (the size word).
 */
#ifndef KZ_RT_FORMAT_H
#define KZ_RT_FORMAT_H

#define KZ_RT_MAGIC "KZRTDUMP"
#define KZ_RT_FILE_HEADER_BYTES 16u
#define KZ_RT_REC_HEADER_BYTES 8u

/* dump_flags */
#define KZ_RT_DUMP_STOPPED 1u   /* kz_rt_stop was called before the dump */

/* stream record types */
#define KZ_RT_REC_RUN 0x0001u
#define KZ_RT_REC_MODULE 0x0002u
#define KZ_RT_REC_RESOURCE 0x0003u
#define KZ_RT_REC_ACTOR 0x0004u
#define KZ_RT_REC_LOSS 0x0005u
#define KZ_RT_REC_POINT 0x0006u
#define KZ_RT_REC_BUF 0x0010u
#define KZ_RT_REC_END 0x00FFu
/* buffer record types */
#define KZ_RT_REC_EVENT 0x0101u
#define KZ_RT_REC_CTL 0x0102u
#define KZ_RT_REC_AEXIT 0x0103u
#define KZ_RT_REC_PAD 0x0100u /* filler to the end of the shared buffer after a lost reservation race */

/* RUN (sub = 0):
 *   8 u32 abi, 12 u32 mode, 16 u64 run_id, 24 u32 instance_id, 28 u32 phase_flags,
 *  32 u32 max_actors, 36 u32 actor_buf_bytes (configured), 40 u32 boot_buf_bytes, 44 u32 max_points,
 *  48 u32 max_vals, 52 u32 str_max, 56 u32 adapter_caps, 60 u32 clock_id,
 *  64 u64 t_init, 72 u64 t_attach, 80 u64 t_stop, 88 u64 t_dump (adapter clock; 0 = not known),
 *  96 u8[32] manifest_sha256 (zero = none), 128 u32 holdable_ctx_mask, 132 u32 rsv,
 * 136 strings: runtime, adapter, backend, compiler, target, each u16 len + bytes, padded to 4 at the end */
#define KZ_RT_RUN_FIXED 136u
#define KZ_RT_PHASE_INIT 1u     /* phase_flags: kz_rt_init was called */
#define KZ_RT_PHASE_ATTACH 2u   /* kz_rt_attach was called */
#define KZ_RT_PHASE_PLAN 4u     /* a plan was loaded */
#define KZ_RT_PHASE_STOP 8u

/* MODULE: 8 u32 module, 12 u32 actor (who registered it), 16 u64 t, 24 u8[32] manifest_sha256; size 56 */
#define KZ_RT_MODULE_SIZE 56u
/* RESOURCE: 8 u32 kind, 12 u32 origin, 16 u64 count, 24 u16 len + text, padded */
#define KZ_RT_RESOURCE_FIXED 24u
/* ACTOR: 8 u32 actor, 12 u32 flags, 16 u32 boot_link (provisional id, 0 = none), 20 u32 link_evidence,
 *  24 u64 native_key, 32 u32 key_kind, 36 u32 key_generation, 40 u32 first_aseq, 44 u32 last_aseq,
 *  48 u32 recorded, 52 u32 overflow, 56 u32 partial, 60 u32 buf_bytes; size 64 */
#define KZ_RT_ACTOR_SIZE 64u
#define KZ_RT_AF_EXITED 1u      /* ACTOR flags: kz_rt_actor_exit was called */
#define KZ_RT_AF_HELD 2u        /* held at a gate when dumped */
#define KZ_RT_LINK_NONE 0u      /* link_evidence */
#define KZ_RT_LINK_SAME_CELL 1u /* the provisional id was in the same per-thread cell */
/* LOSS: 8 u32 kind, 12 u32 actor (0 = global), 16 u32 key, 20 u32 rsv, 24 u64 count; size 32 */
#define KZ_RT_LOSS_SIZE 32u
/* POINT (realized-control summary per plan point):
 *  8 u32 index, 12 u32 kind, 16 u32 label, 20 u32 site, 24 u32 releases, 28 u32 occurrence, 32 u32 flags,
 *  36 u32 hits, 40 u32 acted, 44 u32 satisfied, 48 u32 held_actor (unreleased at the dump if non-zero),
 *  52 u32 holds; size 56 */
#define KZ_RT_POINT_SIZE 56u
/* BUF: 8 u32 owner, 12 u32 nbytes, 16 u32 cap, 20 u32 rsv; then nbytes; size 24 + nbytes */
#define KZ_RT_BUF_FIXED 24u
/* END: 8 u64 bytes, 16 u32 crc32, 20 u32 rsv; size 24 */
#define KZ_RT_END_SIZE 24u

/* EVENT (sub = event flags):
 *   8 u32 actor, 12 u32 aseq, 16 u32 site, 20 u32 bind, 24 u32 link, 28 u32 valid,
 *  32 u64 t, 40 u64 pc (the hook's return address in the instrumented code; 0 = not recorded),
 *  48 u16 nvals, 50 u16 str_len, 52 u8 str_slot (0xFF = none), 53 u8[3] rsv,
 *  56 u64 v[nvals], then str_len bytes (no NUL), padded to 4 */
#define KZ_RT_EVENT_FIXED 56u
#define KZ_RT_EF_CLOCK 0x0001u      /* t is valid */
#define KZ_RT_EF_PC 0x0002u         /* pc is valid */
#define KZ_RT_EF_STR 0x0004u        /* a validated string copy follows */
#define KZ_RT_EF_STR_TRUNC 0x0008u  /* the bound was reached before a NUL */
#define KZ_RT_EF_BOOT 0x0010u       /* recorded in the bootstrap buffer before attach */
#define KZ_RT_EF_UNRESOLVED 0x0020u /* no per-thread cell: actor 0, aseq from the shared buffer */
#define KZ_RT_EF_NOSLOT 0x0040u     /* after attach, but the actor table was full (provisional id) */

/* CTL (sub = what):
 *   8 u32 actor (the writer: the thread that decided), 12 u32 aseq (writer's), 16 u32 site, 20 u32 bind,
 *  24 u32 ev (the hit's observation aseq), 28 u32 ctl_seq (order of control decisions),
 *  32 u64 t, 40 u32 point, 44 u32 hit (the point's qualifying hit), 48 u32 subject (actor whose hit it is),
 *  52 u32 other (FIRE: resumed actor; RESUME: actor whose release resumed it), 56 u32 gate (gate index),
 *  60 u16 reason, 62 u16 ctx; size 64 */
#define KZ_RT_CTL_SIZE 64u
#define KZ_RT_CW_GATE_HOLD 1u      /* a gate acted and the thread is held (the wait starts) */
#define KZ_RT_CW_GATE_REFUSED 2u   /* a gate acted but could not hold (reason); the act is consumed */
#define KZ_RT_CW_GATE_CONSUMED 3u  /* a further gate acted on a hit another gate already holds */
#define KZ_RT_CW_GATE_BUSY 4u      /* an "each" gate acted while it already holds another thread */
#define KZ_RT_CW_FIRE 5u           /* a release acted (reason: RESUMED / SATISFIED / CONSUMED) */
#define KZ_RT_CW_RESUME 6u         /* the held thread proceeds past the gate site */
#define KZ_RT_CW_TIMEOUT 7u        /* the hold's timeout passed: the thread proceeds unreleased */
/* reasons */
#define KZ_RT_CR_NONE 0u
#define KZ_RT_CR_RESUMED 1u        /* FIRE: resumed the held thread */
#define KZ_RT_CR_SATISFIED 2u      /* FIRE: the gate held nothing and is unless-released: satisfied */
#define KZ_RT_CR_CONSUMED 3u       /* FIRE: the gate held nothing: the release is consumed */
#define KZ_RT_CR_DEFERRED 0x10u    /* FIRE: the hit was held; it acted when its thread was resumed (or bit) */
#define KZ_RT_CR_NO_WAIT 4u        /* REFUSED: the adapter has no wait primitive (gate is a no-op) */
#define KZ_RT_CR_PRE_ATTACH 5u     /* REFUSED: before attach */
#define KZ_RT_CR_CONTEXT 6u        /* REFUSED: the context class does not allow a hold */
#define KZ_RT_CR_NO_SLOT 7u        /* REFUSED: the thread has no run actor slot */

/* AEXIT: 8 u32 actor, 12 u32 aseq, 16 u64 t; size 24 */
#define KZ_RT_AEXIT_SIZE 24u

#endif /* KZ_RT_FORMAT_H */

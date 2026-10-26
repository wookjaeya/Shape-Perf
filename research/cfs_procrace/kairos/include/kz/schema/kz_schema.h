/*
 * kz_schema.h -- the ONLY record-layout definition of Kairos (SPEC 2.3).
 *
 * Every record is declared once as an X-macro field list.  One list expands into
 *   (i)   the C struct,
 *   (ii)  the field descriptor table that the canonical dump printer walks
 *         (the table itself is expanded in lib/core/schema_tables.cpp from the same lists),
 *   (iii) a static assertion on the offset of every field, and
 *   (iv)  an assertion that sizeof(record) equals the sum of the field sizes,
 * so that no record has implicit padding.  Explicit `rsv` fields fill alignment gaps
 * and must be zero; writers zero every record before filling it.  No file is
 * generated and no schema compiler is used.
 *
 * The header is plain C: it compiles as C99 (gcc -std=c99 -pedantic -Wall -Werror),
 * as C11 and as C++17, and it can be included by freestanding code (clang -target bpf
 * -ffreestanding).  Containers are little-endian; big-endian hosts are rejected here.
 *
 * Deviations from the excerpt in SPEC 2.3 (all reviewed at S0, recorded in docs/schema.md):
 *   - the list macros take the record type as a second parameter, FIELDS(X, T), and the
 *     entry macros take it as their first, X(T, type, name, array), so that the offset
 *     assertions can name the type;
 *   - sections are regular arrays of one record type each: the "followed by n records"
 *     convention of the excerpt is replaced by explicit `first_*` indices into a section
 *     of its own (CERT_STEPS, VERDICT_CELLS);
 *   - the proof status fields of D20 (KZ_TRUST.proof, KZ_CERT.proof_status,
 *     KZ_VERDICT_CELL.proof_status) are as in SPEC rev 1.2; see kz_cert.h.
 *
 * Schema revision 2 (SPEC rev 1.3, ADR-054 to ADR-056, change table X-01): the summary view, the
 * model identifiers, the observation adequacy record and the actor identities are NEW sections
 * (SUMMARIES, MODEL_IDS, OBS_ADEQUACY, IDENTITIES here; ACTOR_OWNERS in kz_raw.h).  No record of
 * revision 1 changed its layout and no section changed its version, so every container written
 * before (ABI 1, revision 1) is read as it was written.  A container without the new sections has
 * no summary, no adequacy record and no owner: a reader reports them absent and never fills them
 * with a normal value (docs/schema.md section 10).
 *
 * Component-owned parts live in separate headers that this file includes at its end:
 *   kz_raw.h    (observer records, initial, owned by K2)
 *   kz_facts.h  (extraction and skeleton records, initial, owned by K4a)
 *   kz_cert.h   (certificate and trust-ledger records, initial, owned by K4b)
 *   kz_neutral.h (the neutral event layer of a run container, schema revision 3)
 * They are not usable alone.
 *
 * Schema revision 3 (SPEC rev 1.4 2.3, decision record 2026-10-26 2 item 5): the neutral event layer is
 * NEW sections (100-107, kz_neutral.h); no earlier record or section version changed, so every container
 * written before is read as it was written.  The records of sections 100-119 are stored by the explicit
 * little-endian codec (kz/core/record_codec.hpp), not by copying host structs.
 */
#ifndef KZ_SCHEMA_KZ_SCHEMA_H
#define KZ_SCHEMA_KZ_SCHEMA_H

#include <stddef.h>
#include <stdint.h>

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__) || \
    (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "KZ containers are little-endian; this host byte order is not supported"
#endif

#define KZ_ABI_VERSION 1u
/* Revision of the record set (docs/schema.md sections 10, 12).  Revisions 2 and 3 add sections only; it
 * is part of the layout text, hence of the schema fingerprint and of every tool hash.  Revision 3 adds the
 * neutral event layer (kz_neutral.h). */
#define KZ_SCHEMA_REVISION 3u

/* Sentinels.  A null kz_ref is all zero bytes; index fields use KZ_NONE32. */
#define KZ_NONE32 0xFFFFFFFFu
#define KZ_NONE64 0xFFFFFFFFFFFFFFFFull
#define KZ_BOUND_INFINITE 0xFFFFFFFFu /* KZ_BOUND.h: the bound b = infinity */

/* ------------------------------------------------------------------------- */
/* Machinery                                                                 */
/* ------------------------------------------------------------------------- */

#define KZ_PASTE_(a, b) a##b
#define KZ_PASTE(a, b) KZ_PASTE_(a, b)

/* Static assertion that works in C99 (negative array size), C11 and C++. */
#if defined(__cplusplus)
#define KZ_STATIC_ASSERT(name, cond) static_assert(cond, #name)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define KZ_STATIC_ASSERT(name, cond) _Static_assert(cond, #name)
#else
#define KZ_STATIC_ASSERT(name, cond) typedef char KZ_PASTE(kz_sa_, name)[(cond) ? 1 : -1]
#endif

/* Field kinds understood by the dump printer and the container validator. */
enum kz_field_kind {
  KZ_FK_U8 = 1,
  KZ_FK_U16 = 2,
  KZ_FK_U32 = 3,
  KZ_FK_U64 = 4,
  KZ_FK_I32 = 5,
  KZ_FK_I64 = 6,
  KZ_FK_REF = 7, /* kz_ref: typed cross-artifact reference */
  KZ_FK_STR = 8  /* kz_str: (offset, length) into the STRINGS section */
};

#define KZ_KIND_uint8_t KZ_FK_U8
#define KZ_KIND_uint16_t KZ_FK_U16
#define KZ_KIND_uint32_t KZ_FK_U32
#define KZ_KIND_uint64_t KZ_FK_U64
#define KZ_KIND_int32_t KZ_FK_I32
#define KZ_KIND_int64_t KZ_FK_I64
#define KZ_KIND_kz_ref KZ_FK_REF
#define KZ_KIND_kz_str KZ_FK_STR

#define KZ_ALIGN_uint8_t 1u
#define KZ_ALIGN_uint16_t 2u
#define KZ_ALIGN_uint32_t 4u
#define KZ_ALIGN_uint64_t 8u
#define KZ_ALIGN_int32_t 4u
#define KZ_ALIGN_int64_t 8u
#define KZ_ALIGN_kz_ref 4u
#define KZ_ALIGN_kz_str 4u

/* Field descriptor and record descriptor (tables are built in lib/core). */
typedef struct kz_field_desc {
  const char *name;
  uint32_t offset;
  uint32_t elem_size; /* size of one element */
  uint32_t count;     /* number of elements: 1 for scalars */
  uint32_t kind;      /* enum kz_field_kind */
} kz_field_desc;

typedef struct kz_record_desc {
  const char *name;
  uint32_t size;
  uint32_t nfields;
  const kz_field_desc *fields;
} kz_record_desc;

/* Entry macros used by KZ_RECORD.  X(T, type, name, array_suffix). */
#define KZ_FIELD_DECL(T, type, name, arr) type name arr;
#define KZ_FIELD_BYTES(T, type, name, arr) unsigned char name[sizeof(((T *)0)->name)];
#define KZ_FIELD_SIZE(T, type, name, arr) +sizeof(((T *)0)->name)
#define KZ_FIELD_OFFSET_ASSERT(T, type, name, arr)                                       \
  KZ_STATIC_ASSERT(T##_off_##name, offsetof(T, name) == offsetof(struct T##_kz_bytes, name)); \
  KZ_STATIC_ASSERT(T##_align_##name, offsetof(T, name) % KZ_ALIGN_##type == 0u);

/*
 * KZ_RECORD(T, LIST) defines the record type T from the field list LIST.
 *   - T##_kz_bytes is a twin made only of byte arrays: it cannot have padding, so the
 *     offset of each field in T must equal its offset in the twin (the sum of the sizes
 *     of the preceding fields);
 *   - sizeof(T) must equal the sum of the field sizes (no trailing padding either);
 *   - every field is naturally aligned.
 */
#define KZ_RECORD(T, LIST)                                                              \
  typedef struct T {                                                                    \
    LIST(KZ_FIELD_DECL, T)                                                              \
  } T;                                                                                  \
  struct T##_kz_bytes {                                                                 \
    LIST(KZ_FIELD_BYTES, T)                                                             \
  };                                                                                    \
  KZ_STATIC_ASSERT(T##_size_is_twin_size, sizeof(T) == sizeof(struct T##_kz_bytes));    \
  KZ_STATIC_ASSERT(T##_size_is_field_sum, sizeof(T) == (0u LIST(KZ_FIELD_SIZE, T)));    \
  LIST(KZ_FIELD_OFFSET_ASSERT, T)

/* Enumerations: ITEM(P, id, value, "display name").  Defines P_id constants. */
#define KZ_ENUM_ITEM(P, id, value, display) P##_##id = value,
#define KZ_DEFINE_ENUM(P, LIST, kind) \
  enum { LIST(KZ_ENUM_ITEM, P) };

/* ------------------------------------------------------------------------- */
/* Section type ids                                                          */
/* ------------------------------------------------------------------------- */
/*
 * Ids are assigned centrally so that parallel components cannot collide.  The
 * canonical section order of a container is ascending id.  Ranges:
 *    1- 9  generic (this file)
 *   10-39  extraction and skeleton (kz_facts.h)
 *   40-59  observation (kz_raw.h)
 *   60-79  certificate and ledger (kz_cert.h)
 *   80-99  witnesses and verdicts (this file)
 *  100-119 neutral event layer (kz_neutral.h; serialized field by field)
 * Ids reserved for SPEC 2.3 section types whose records are not defined yet (no
 * record, no registry entry; define them in the owning header when they are needed):
 *   22 TEMPLATES   25 SKEL_EXPL   27 PROVENANCE_MSG   30 RESOURCES
 *   42 EVENTS      47 EDGES       48 INSTANCES        49 HOLDPLANS
 */
enum kz_section_id {
  KZ_SEC_NONE = 0,
  KZ_SEC_STRINGS = 1,
  KZ_SEC_BASIS = 2,
  KZ_SEC_FILE_HASHES = 3,
  KZ_SEC_PROVENANCE = 4,
  KZ_SEC_DEVIATIONS = 5,
  KZ_SEC_REF_LISTS = 6,
  KZ_SEC_WITNESSES = 80,
  KZ_SEC_VERDICTS = 81,
  KZ_SEC_VERDICT_CELLS = 82,
  KZ_SEC_BOUNDS = 83,
  KZ_SEC_SUMMARIES = 84,
  KZ_SEC_MODEL_IDS = 85,
  KZ_SEC_OBS_ADEQUACY = 86,
  KZ_SEC_IDENTITIES = 87
};

/* S(NAME, id, record, schema_version).  Sections defined by this file. */
#define KZ_CORE_SECTIONS(S)                \
  S(STRINGS, 1, kz_byte, 1)                \
  S(BASIS, 2, kz_basis_item, 1)            \
  S(FILE_HASHES, 3, kz_file_hash, 1)       \
  S(PROVENANCE, 4, kz_prov, 1)             \
  S(DEVIATIONS, 5, kz_deviation, 1)        \
  S(REF_LISTS, 6, kz_ref, 1)               \
  S(WITNESSES, 80, kz_witness, 1)          \
  S(VERDICTS, 81, kz_verdict, 1)           \
  S(VERDICT_CELLS, 82, kz_verdict_cell, 1) \
  S(BOUNDS, 83, kz_bound, 1)               \
  S(SUMMARIES, 84, kz_summary, 1)          \
  S(MODEL_IDS, 85, kz_model_id, 1)         \
  S(OBS_ADEQUACY, 86, kz_obs_adequacy, 1)  \
  S(IDENTITIES, 87, kz_identity, 1)

/* ------------------------------------------------------------------------- */
/* Generic records                                                           */
/* ------------------------------------------------------------------------- */

/* One byte of a byte-array section (STRINGS). */
#define KZ_BYTE_FIELDS(X, T) X(T, uint8_t, b, )

/* Typed reference: (artifact sha256, section type, index).  All zero = null.  A zero
 * artifact hash with a non-zero section means "this container". */
#define KZ_REF_FIELDS(X, T) \
  X(T, uint8_t, artifact_sha256, [32]) X(T, uint32_t, section, ) X(T, uint32_t, index, )

/* String reference into the STRINGS section.  (0, 0) is the empty string. */
#define KZ_STR_FIELDS(X, T) X(T, uint32_t, off, ) X(T, uint32_t, len, )

/* Container header (160 bytes).  header_crc covers header, parent table and section
 * table with header_crc itself taken as zero. */
#define KZ_HEADER_FIELDS(X, T)                                                       \
  X(T, uint8_t, magic, [8])                                                          \
  X(T, uint32_t, abi_version, )                                                      \
  X(T, uint32_t, artifact_kind, )                                                    \
  X(T, uint32_t, flags, ) /* reserved, zero */                                       \
  X(T, uint32_t, header_size, )                                                      \
  X(T, uint32_t, n_parents, )                                                        \
  X(T, uint32_t, n_sections, )                                                       \
  X(T, uint64_t, parents_off, )                                                      \
  X(T, uint64_t, sections_off, )                                                     \
  X(T, uint64_t, total_size, )                                                       \
  X(T, uint8_t, basis_sha256, [32])   /* B hash */                                   \
  X(T, uint8_t, profile_sha256, [32]) /* profile hash */                             \
  X(T, uint8_t, tool_sha256, [32])    /* tool hash */                                \
  X(T, uint32_t, header_crc, )                                                       \
  X(T, uint32_t, rsv0, )

/* Parent artifact (Merkle-style): the sha256 of the whole parent file. */
#define KZ_PARENT_FIELDS(X, T) \
  X(T, uint8_t, sha256, [32]) X(T, uint32_t, kind, ) X(T, uint32_t, role, )

/* Section table entry (48 bytes). */
#define KZ_SECTION_ENTRY_FIELDS(X, T) \
  X(T, uint32_t, type, )              \
  X(T, uint32_t, schema_version, )    \
  X(T, uint32_t, record_size, )       \
  X(T, uint32_t, rsv0, )              \
  X(T, uint64_t, count, )             \
  X(T, uint64_t, offset, )            \
  X(T, uint64_t, size, )              \
  X(T, uint32_t, crc32, )             \
  X(T, uint32_t, rsv1, )

/*
 * Provenance of an entry (a basis item, a profile entry, a fact, a cited text).  Tiers
 * (T1): contract-backed / admitted / code-derived / issue-derived / researcher /
 * unreferenced.  A zero record is `unreferenced`, the weakest tier.  Hand-entered
 * rules and facts carry tier `researcher`; the reports count them.
 */
#define KZ_PROV_FIELDS(X, T)                                                         \
  X(T, uint8_t, tier, )                                                              \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, side, )                                                              \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint32_t, line_from, )                                                        \
  X(T, uint32_t, line_to, )                                                          \
  X(T, kz_str, file, )     /* path of the cited text, or a requirement id */         \
  X(T, kz_str, cite, )     /* issue/commit/URL or other locator */                   \
  X(T, kz_str, authored, ) /* author and inspected cases, free text */               \
  X(T, uint8_t, text_sha256, [32])

/* A deviation from a reference condition (CONDITIONS_POLICY): value, reason, effect. */
#define KZ_DEVIATION_FIELDS(X, T)                                                    \
  X(T, kz_str, subject, )         /* which condition */                              \
  X(T, kz_str, reference_value, ) /* what the reference says (empty if it is silent) */ \
  X(T, kz_str, used_value, )                                                         \
  X(T, kz_str, reason, )                                                             \
  X(T, kz_str, effect, )                                                             \
  X(T, uint8_t, avoidable, )                                                         \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, prov, ) /* PROVENANCE index or KZ_NONE32 */

/* One item of the analysis basis B / the run conditions: value, source, policy status. */
#define KZ_BASIS_ITEM_FIELDS(X, T)                                                   \
  X(T, kz_str, name, )                                                               \
  X(T, kz_str, value, )                                                              \
  X(T, uint8_t, policy, )                                                            \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, prov, )      /* PROVENANCE index or KZ_NONE32 */                    \
  X(T, uint32_t, deviation, ) /* DEVIATIONS index or KZ_NONE32 */

/* A hashed input file (source file, binary, table, script). */
#define KZ_FILE_HASH_FIELDS(X, T)                                                    \
  X(T, kz_str, path, )                                                               \
  X(T, uint8_t, sha256, [32])                                                        \
  X(T, uint64_t, size, )                                                             \
  X(T, uint32_t, prov, )                                                             \
  X(T, uint32_t, rsv0, )

/*
 * A witness: a finite execution (or a model path) with an occurrence.  The ordinal is
 * (canonical actor-instance name, k): the k-th occurrence begun by that actor.
 */
#define KZ_WITNESS_FIELDS(X, T)                                                      \
  X(T, uint8_t, kind, )        /* BAD|GOOD|STALL */                                  \
  X(T, uint8_t, status, )      /* occurrence status */                               \
  X(T, uint8_t, naturalness, ) /* set */                                             \
  X(T, uint8_t, flags, )                                                             \
  X(T, uint16_t, realization, ) /* set */                                            \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, ordinal_k, )                                                        \
  X(T, uint32_t, rsv1, )                                                             \
  X(T, kz_str, actor, ) /* canonical actor-instance name */                          \
  X(T, kz_ref, run, )                                                                \
  X(T, kz_ref, events, ) /* first event of the occurrence's extent */                \
  X(T, uint32_t, n_events, )                                                         \
  X(T, uint32_t, rsv2, )

/* One bound of one cell (192 bytes, SPEC 2.3). */
#define KZ_BOUND_FIELDS(X, T)                                                        \
  X(T, uint32_t, h, ) /* the bound; KZ_BOUND_INFINITE = infinity */                  \
  X(T, uint8_t, policy, )                                                            \
  X(T, uint8_t, cls, )                                                               \
  X(T, uint8_t, reach, )                                                             \
  X(T, uint8_t, progress, )                                                          \
  X(T, uint8_t, progress_kind, )                                                     \
  X(T, uint8_t, safety_routes, )                                                     \
  X(T, uint8_t, flags, )                                                             \
  X(T, uint8_t, unknown_reason, )                                                    \
  X(T, uint8_t, progress_reason, )                                                   \
  X(T, uint8_t, progress_harm, )                                                     \
  X(T, uint16_t, refinements, )                                                      \
  X(T, uint16_t, progress_flags, )                                                   \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, n_bad, )                                                            \
  X(T, uint32_t, n_good, )                                                           \
  X(T, uint32_t, n_stall, )                                                          \
  X(T, kz_ref, witness_bad, )                                                        \
  X(T, kz_ref, witness_good, )                                                       \
  X(T, kz_ref, witness_stall, )                                                      \
  X(T, kz_ref, progress_chain, )

/*
 * Verdict of an instance (112 bytes).  There is never a "safe" field and no field means
 * "live" or "deadlock-free".  `rsv1` was `reach` in an earlier draft; reach is per cell
 * and the field must be zero.
 */
#define KZ_VERDICT_FIELDS(X, T)                                                      \
  X(T, uint64_t, instance, ) /* (rspec, context-qualified template, key) */          \
  X(T, uint8_t, provider, )                                                          \
  X(T, uint8_t, rsv1, )                                                              \
  X(T, uint8_t, scope, )                                                             \
  X(T, uint8_t, outcome_label, )                                                     \
  X(T, uint8_t, shape, )                                                             \
  X(T, uint8_t, consequence, )                                                       \
  X(T, uint8_t, harm, )                                                              \
  X(T, uint8_t, fw_doc, ) /* set */                                                  \
  X(T, uint32_t, sublabels, )                                                        \
  X(T, uint32_t, ncells, )                                                           \
  X(T, uint32_t, first_cell, ) /* VERDICT_CELLS index of the first of ncells cells */ \
  X(T, uint32_t, rsv2, )                                                             \
  X(T, kz_ref, chain, ) /* violation chains only */                                  \
  X(T, kz_ref, provenance, )

/* Verdict cell for one (sigma, M) pair (472 bytes, SPEC 2.3). */
#define KZ_VERDICT_CELL_FIELDS(X, T)                                                 \
  X(T, uint8_t, sigma, )                                                             \
  X(T, uint8_t, mlevel, )                                                            \
  X(T, uint8_t, cls, )                                                               \
  X(T, uint8_t, unknown_reason, )                                                    \
  X(T, uint16_t, refinements, )                                                      \
  X(T, uint16_t, realization, )                                                      \
  X(T, uint8_t, naturalness, )                                                       \
  X(T, uint8_t, flags, )                                                             \
  X(T, uint8_t, reach, )                                                             \
  X(T, uint8_t, progress, ) /* default 0 = unknown, with reason not_checked (D20) */ \
  X(T, uint8_t, progress_reason, )                                                   \
  X(T, uint8_t, progress_harm, )                                                     \
  X(T, uint8_t, progress_realization, )                                              \
  X(T, uint8_t, safety_routes, ) /* 2 bits each: bits 0-1 not-Bad, 2-3 not-OK, 4-5 not-NR */ \
  X(T, uint16_t, progress_flags, )                                                   \
  X(T, uint8_t, progress_kind, )                                                     \
  X(T, uint8_t, fw_conf, )                                                           \
  X(T, uint8_t, proof_status, ) /* weakest LEMMA status of the cell's ledger that its statements use; NA if none */ \
  X(T, uint8_t, rsv2, [3])                                                           \
  X(T, uint32_t, observed_n, )                                                       \
  X(T, uint32_t, observed_N, )                                                       \
  X(T, kz_ref, bounds, )                                                             \
  X(T, kz_ref, refinement_bounds, )                                                  \
  X(T, kz_ref, progress_bounds, )                                                    \
  X(T, kz_ref, pool_key, )                                                           \
  X(T, kz_ref, certificate, )                                                        \
  X(T, kz_ref, trust, )                                                              \
  X(T, kz_ref, witness_bad, )                                                        \
  X(T, kz_ref, witness_good, )                                                       \
  X(T, kz_ref, witness_stall, )                                                      \
  X(T, kz_ref, progress_chain, )                                                     \
  X(T, kz_ref, runs, )

/*
 * Summary view of one verdict cell (SPEC 1.10 "Summary view", ADR-054, D2; 44 bytes).  A VIEW: it is
 * computed last from the unchanged record (occurrence status, three judgements, tier, evidence) and
 * never replaces it.  Observation, prediction and confirmation are reported and counted apart
 * (OUT-01).  `status` is the occurrence status the observation summary was computed from; OPEN (no
 * decision in the trace) and an undetermined status (the explanations of a real record disagree)
 * are status_flags, because neither is one of the six statuses.  An analysis input error (an empty
 * explanation set: the record and the model contradict each other) is the flag `input-error` with
 * observation `none`: it is reported apart, never as a violation and never as no violation.
 */
#define KZ_SUMMARY_FIELDS(X, T)                                                      \
  X(T, uint32_t, cell, )          /* VERDICT_CELLS index of the cell, or KZ_NONE32 */ \
  X(T, uint8_t, observation, )    /* KZ_OBS */                                       \
  X(T, uint8_t, prediction, )     /* KZ_PRED */                                      \
  X(T, uint8_t, confirmation, )   /* KZ_CONF */                                      \
  X(T, uint8_t, relation, )       /* KZ_EXEC_RELATION (SPEC 1.1) */                  \
  X(T, uint32_t, obs_reasons, )   /* set KZ_WITHHOLD: the reasons of INCONCLUSIVE */ \
  X(T, uint32_t, pred_reasons, )  /* set KZ_WITHHOLD */                              \
  X(T, uint8_t, status, )         /* KZ_OCC */                                       \
  X(T, uint8_t, status_flags, )   /* set KZ_STATUS_FLAG: open, undetermined */       \
  X(T, uint16_t, flags, )         /* set KZ_SUMMARY_FLAG */                          \
  X(T, uint32_t, model, )         /* MODEL_IDS index, or KZ_NONE32 */                \
  X(T, uint32_t, adequacy, )      /* OBS_ADEQUACY index, or KZ_NONE32 */             \
  X(T, uint32_t, identity_first, ) /* IDENTITIES index of the first actor identity, or KZ_NONE32 */ \
  X(T, uint32_t, n_identities, )                                                     \
  X(T, kz_str, conf_reason, )     /* NOT_CONFIRMED(reason): the SPEC fixes no vocabulary */

/*
 * Model identifiers of a result (SPEC 1.8 "Model and schedule identifiers on every result", C-09;
 * 44 bytes).  `profile_id` is the profile used to interpret an observed result, recorded apart.
 */
#define KZ_MODEL_ID_FIELDS(X, T)                                                     \
  X(T, kz_str, model_id, )                                                           \
  X(T, uint8_t, model_level, ) /* KZ_MLEVEL: M_api | M_impl; unset = not recorded */ \
  X(T, uint8_t, sigma, )       /* KZ_SIGMA of the schedule profile */                \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, kz_str, schedule_profile_id, )                                                \
  X(T, kz_str, basis_id, )                                                           \
  X(T, kz_str, scope_id, )                                                           \
  X(T, kz_str, profile_id, )

/*
 * Observation adequacy of one condition instance (SPEC 2.3 rev 1.3, 4.7, 4.8 "Recorded writes alone",
 * ADR-055; 88 bytes).  `established` is what the record establishes for the resources and locations
 * the condition reads; `absent` what the source did not carry at all (an earlier container): an
 * absent item is never read as established.  `reasons` are the withholding reasons the missing items
 * cause.  The interval is in event positions of the replayed record.
 */
#define KZ_OBS_ADEQUACY_FIELDS(X, T)                                                 \
  X(T, uint32_t, established, ) /* set KZ_ADEQ */                                    \
  X(T, uint32_t, absent, )      /* set KZ_ADEQ */                                    \
  X(T, uint32_t, reasons, )     /* set KZ_WITHHOLD */                                \
  X(T, uint8_t, start_evidence, ) /* KZ_START_EVIDENCE */                            \
  X(T, uint8_t, end_reason, )   /* KZ_INTERVAL_END */                                \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint64_t, first_event, ) /* KZ_NONE64 if unknown */                           \
  X(T, uint64_t, last_event, )                                                       \
  X(T, uint64_t, loss_reports, ) /* losses reported in the interval; KZ_NONE64 = not recorded */ \
  X(T, kz_ref, probes, )        /* REF_LISTS list of the SITES attached and active, or null */ \
  X(T, kz_str, unobserved, )    /* unobserved effects and points, by name, ';'-separated */

/*
 * The identities of one actor at one event (SPEC 1.2 "Four identity fields" and "Creating
 * application", ADR-056; masterplan 2026-10-24 section 3.2; 68 bytes).  Four categories, never
 * merged and never assumed equal:
 *   1 executing task      the thread incarnation that executes the event (exec(e));
 *   2 creating executor   the thread incarnation that ran the creation of the executing task;
 *   3 owning application  app_owner of the executing task: the application that created / owns it,
 *                         with its evidence level and evidence (match-by-time, id-unverified ...);
 *   4 passed identifier   an identity value the code passes (stamp(e)): e.g. an application id argument.
 * The owning application is never taken from a table whose ordering is analysed (EV-03).
 */
#define KZ_IDENTITY_FIELDS(X, T)                                                     \
  X(T, uint32_t, exec_tid, )                                                         \
  X(T, uint32_t, exec_generation, )                                                  \
  X(T, uint32_t, creator_tid, )        /* valid with CREATOR_KNOWN */                \
  X(T, uint32_t, creator_generation, )                                               \
  X(T, uint32_t, owner_key, )          /* key in the profile's owner kind; KZ_NONE32: none established */ \
  X(T, uint8_t, owner_level, )         /* KZ_OWNER_LEVEL */                          \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, owner_evidence, )     /* set KZ_OWNER_EVIDENCE */                   \
  X(T, uint32_t, passed_id, )          /* valid with PASSED_ID_PRESENT */            \
  X(T, uint32_t, flags, )              /* set KZ_IDENTITY_FLAG */                    \
  X(T, kz_str, exec_name, )                                                          \
  X(T, kz_str, creator_name, )                                                       \
  X(T, kz_str, owner_name, )                                                         \
  X(T, kz_str, passed_role, )          /* the parameter that carried passed_id, e.g. the name of an id parameter */

/* Records of this file, in definition order (kz_ref and kz_str first). */
#define KZ_CORE_RECORDS(R)                      \
  R(kz_byte, KZ_BYTE_FIELDS)                    \
  R(kz_ref, KZ_REF_FIELDS)                      \
  R(kz_str, KZ_STR_FIELDS)                      \
  R(kz_header, KZ_HEADER_FIELDS)                \
  R(kz_parent, KZ_PARENT_FIELDS)                \
  R(kz_section_entry, KZ_SECTION_ENTRY_FIELDS)  \
  R(kz_prov, KZ_PROV_FIELDS)                    \
  R(kz_deviation, KZ_DEVIATION_FIELDS)          \
  R(kz_basis_item, KZ_BASIS_ITEM_FIELDS)        \
  R(kz_file_hash, KZ_FILE_HASH_FIELDS)          \
  R(kz_witness, KZ_WITNESS_FIELDS)              \
  R(kz_bound, KZ_BOUND_FIELDS)                  \
  R(kz_verdict, KZ_VERDICT_FIELDS)              \
  R(kz_verdict_cell, KZ_VERDICT_CELL_FIELDS)    \
  R(kz_summary, KZ_SUMMARY_FIELDS)              \
  R(kz_model_id, KZ_MODEL_ID_FIELDS)            \
  R(kz_obs_adequacy, KZ_OBS_ADEQUACY_FIELDS)    \
  R(kz_identity, KZ_IDENTITY_FIELDS)

KZ_CORE_RECORDS(KZ_RECORD)

/* ------------------------------------------------------------------------- */
/* Enumerations of this file.  0 is always the conservative / "unset" value  */
/* ------------------------------------------------------------------------- */

#define KZ_ENUM_ARTIFACT_KIND(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, BASIS, 1, "basis") ITEM(P, PROFILE, 2, "profile")          \
  ITEM(P, FACTS, 3, "facts") ITEM(P, ANCHORS, 4, "anchors") ITEM(P, SKEL, 5, "skel")          \
  ITEM(P, CENSUS, 6, "census") ITEM(P, PLAN, 7, "plan") ITEM(P, RUN, 8, "run")               \
  ITEM(P, HB, 9, "hb") ITEM(P, WITNESS, 10, "witness") ITEM(P, HOLDPLAN, 11, "holdplan")      \
  ITEM(P, VERDICTS, 12, "verdicts") ITEM(P, LOOP, 13, "loop") ITEM(P, TEST, 14, "unit-test")

/* Policy status of a condition (CONDITIONS_POLICY); unit-test values are not research conditions. */
#define KZ_ENUM_POLICY(ITEM, P)                                                         \
  ITEM(P, UNSET, 0, "unset") ITEM(P, FROM_REFERENCE, 1, "from-reference")                  \
  ITEM(P, DEVIATION, 2, "deviation") ITEM(P, UNSPECIFIED, 3, "unspecified_by_reference")   \
  ITEM(P, UNIT_TEST, 4, "unit-test-value") ITEM(P, DECLARED, 5, "declared-assumption")

#define KZ_ENUM_AVOIDABLE(ITEM, P) \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, YES, 1, "yes") ITEM(P, NO, 2, "no")

/* Provenance tiers (T1).  The identifier avoids the word the PI retired. */
#define KZ_ENUM_TIER(ITEM, P)                                                           \
  ITEM(P, UNREFERENCED, 0, "unreferenced") ITEM(P, DOC_BACKED, 1, "contract-backed")       \
  ITEM(P, ADMITTED, 2, "admitted") ITEM(P, CODE_DERIVED, 3, "code-derived")                \
  ITEM(P, ISSUE_DERIVED, 4, "issue-derived") ITEM(P, RESEARCHER, 5, "researcher")

#define KZ_ENUM_PROV_KIND(ITEM, P)                                                      \
  ITEM(P, NONE, 0, "none") ITEM(P, API_DOC, 1, "api_doc") ITEM(P, REQUIREMENT, 2, "requirement") \
  ITEM(P, CODE_ADMISSION, 3, "code_admission") ITEM(P, CODE_PATTERN, 4, "code_pattern")    \
  ITEM(P, ISSUE, 5, "issue") ITEM(P, RESEARCHER, 6, "researcher")

#define KZ_ENUM_PROV_SIDE(ITEM, P) \
  ITEM(P, NA, 0, "n/a") ITEM(P, SUPPLIER, 1, "supplier") ITEM(P, CALLER, 2, "caller") ITEM(P, RULE, 3, "rule")

#define KZ_ENUM_SIGMA(ITEM, P)                                                          \
  ITEM(P, NONE, 0, "unset") ITEM(P, U, 1, "sigma_U") ITEM(P, U_INF, 2, "sigma_Uinf")          \
  ITEM(P, R1, 3, "sigma_R1") ITEM(P, E, 4, "sigma_E")

#define KZ_ENUM_MLEVEL(ITEM, P) ITEM(P, NONE, 0, "unset") ITEM(P, API, 1, "M_api") ITEM(P, IMPL, 2, "M_impl")

#define KZ_ENUM_CLS(ITEM, P)                                                            \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, INACTIVE, 1, "inactive") ITEM(P, PROTECTED, 2, "protected") \
  ITEM(P, FIXED, 3, "fixed-order") ITEM(P, NO_FINAL, 4, "no-final-status")                 \
  ITEM(P, SCHED_DEP, 5, "schedule-dependent") ITEM(P, ORDINAL_SPLIT, 6, "ordinal-split")   \
  ITEM(P, CONTESTED, 7, "contested")

#define KZ_ENUM_UNKNOWN_REASON(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, NOT_REACHED, 1, "not_reached") ITEM(P, MODEL_GAP, 2, "model_gap") \
  ITEM(P, BOUND, 3, "bound") ITEM(P, BUDGET, 4, "budget") ITEM(P, KEY_UNRESOLVED, 5, "key_unresolved") \
  ITEM(P, LOSS, 6, "loss") ITEM(P, UNMODELLED, 7, "unmodelled")                            \
  ITEM(P, WRITER_INCOMPLETE, 8, "writer_incomplete") ITEM(P, HORIZON, 9, "horizon")        \
  ITEM(P, HORIZON_STATIC, 10, "horizon_static") ITEM(P, NO_DECISION_ROUTE, 11, "no_decision_route") \
  ITEM(P, ONE_SIDED, 12, "one_sided") ITEM(P, UNORDERED, 13, "unordered")                  \
  ITEM(P, O_ORDERED_ONLY, 14, "O-ordered-only")                                         \
  ITEM(P, CONDITIONAL_OBSERVATION, 15, "conditional-observation")                       \
  ITEM(P, CONFORMANCE, 16, "conformance") ITEM(P, MODEL_FAULT, 17, "model_fault")          \
  ITEM(P, MODEL_LEVEL, 18, "model_level")

#define KZ_ENUM_REACH(ITEM, P)                                                          \
  ITEM(P, ASSUMED, 0, "assumed") ITEM(P, PROVED, 1, "proved") ITEM(P, MODEL_B, 2, "model(b)") \
  ITEM(P, UNREACHED_B, 3, "unreached(b)") ITEM(P, CONTESTED, 4, "contested")

#define KZ_ENUM_PROGRESS(ITEM, P)                                                       \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, STALL_WITNESS, 1, "stall-witness")                \
  ITEM(P, NO_STALL_B, 2, "no-stall-within(b)") ITEM(P, CONTESTED, 3, "contested")

#define KZ_ENUM_PROGRESS_REASON(ITEM, P)                                                \
  ITEM(P, NOT_CHECKED, 0, "not_checked") ITEM(P, BOUND, 1, "bound") ITEM(P, BUDGET, 2, "budget") \
  ITEM(P, UNMODELLED, 3, "unmodelled") ITEM(P, TEARDOWN_SCOPE, 4, "teardown_scope")        \
  ITEM(P, MODEL_GAP, 5, "model_gap") ITEM(P, MODEL_FAULT, 6, "model_fault")

#define KZ_ENUM_PROGRESS_HARM(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, HANG_OR_DEADLOCK, 1, "hang_or_deadlock")                \
  ITEM(P, TASK_NEVER_RUNS, 2, "task_never_runs")                                        \
  ITEM(P, TERMINAL_PENDING_PANIC, 3, "terminal_pending(panic)")                         \
  ITEM(P, TERMINAL_PENDING_ABORT, 4, "terminal_pending(abort)")

#define KZ_ENUM_PROGRESS_REALIZATION(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, MODEL, 1, "model(b)") ITEM(P, REALIZED, 2, "realized")

#define KZ_ENUM_PROGRESS_KIND(ITEM, P) /* set */                                        \
  ITEM(P, PRE_USE, 0x1, "pre-use") ITEM(P, IN_USE, 0x2, "in-use") ITEM(P, HORIZON, 0x4, "horizon")

#define KZ_ENUM_PROGRESS_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, DEADLOCK_CYCLE, 0x1, "deadlock-cycle") ITEM(P, LEAKED_LOCK, 0x2, "leaked-lock")  \
  ITEM(P, IDLE, 0x4, "idle") ITEM(P, TERMINAL_PENDING, 0x8, "terminal-pending")            \
  ITEM(P, CHOICE_DEPENDENT, 0x10, "choice-dependent") ITEM(P, CONFIRMABLE, 0x20, "confirmable") \
  ITEM(P, CHAINED, 0x40, "chained")                                                     \
  ITEM(P, TIMEOUT_MASKED_LIVENESS, 0x80, "timeout-masked-liveness")                     \
  ITEM(P, TML_ROOT, 0x100, "tml-root")

#define KZ_ENUM_ROUTE(ITEM, P) /* 2-bit route of a safety statement */                  \
  ITEM(P, NONE, 0, "none") ITEM(P, CERT, 1, "cert") ITEM(P, MODEL, 2, "model") ITEM(P, TRANSFERRED, 3, "transferred")

#define KZ_ENUM_REFINEMENT(ITEM, P) /* set */                                           \
  ITEM(P, TOLERATED, 0x1, "tolerated") ITEM(P, TIMEOUT_DEP, 0x2, "timeout-dependent")      \
  ITEM(P, TIMEOUT_RESCUED, 0x4, "timeout-rescued")

#define KZ_ENUM_REALIZATION(ITEM, P) /* set */                                          \
  ITEM(P, CERT, 0x1, "certificate") ITEM(P, MODEL, 0x2, "model(b)")                        \
  ITEM(P, PREDICTED, 0x4, "predicted") ITEM(P, PRED_MODELLED, 0x8, "predicted-modelled")   \
  ITEM(P, PRED_EXTRAP, 0x10, "predicted-extrapolated") ITEM(P, REALIZED, 0x20, "realized")

#define KZ_ENUM_NATURALNESS(ITEM, P) /* set */                                          \
  ITEM(P, NATURAL, 0x1, "natural") ITEM(P, HELD_F0, 0x2, "held-F0")                        \
  ITEM(P, GATED_F1, 0x4, "gated-F1") ITEM(P, TIMEOUT_EXERCISED, 0x8, "timeout-exercised")

#define KZ_ENUM_CELL_FLAGS(ITEM, P) /* set */                                           \
  ITEM(P, DERIVED, 0x1, "derived") ITEM(P, SIGMA_E_HELD, 0x2, "sigma_E(held)")             \
  ITEM(P, SAFETY_TRANSFERRED, 0x4, "safety-transferred") ITEM(P, DECIDES, 0x8, "decides")

#define KZ_ENUM_FW_CONF(ITEM, P) \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, AS_DOCUMENTED, 1, "as-documented") ITEM(P, DEVIATES, 2, "deviates")

#define KZ_ENUM_FW_DOC(ITEM, P) /* set */                                               \
  ITEM(P, DOC_NOT_ERROR, 0x1, "documented-not-error") ITEM(P, DOC_RESULT, 0x2, "documented-result") \
  ITEM(P, UNDOCUMENTED, 0x4, "undocumented")

#define KZ_ENUM_PROVIDER(ITEM, P)                                                       \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, PRESENT, 1, "present") ITEM(P, NONE, 2, "none")      \
  ITEM(P, ENV_ABSENT, 3, "env-absent")

#define KZ_ENUM_SCOPE(ITEM, P) ITEM(P, IN_SCOPE, 0, "in-scope") ITEM(P, TEARDOWN, 1, "teardown")

#define KZ_ENUM_OUTCOME_LABEL(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, VIOLATION, 1, "violation")                              \
  ITEM(P, LOSS_UNDER_ORDER, 2, "LOSS-UNDER-ORDER")                                      \
  ITEM(P, NOT_READY_UNJUDGED, 3, "not-ready-unjudged")

#define KZ_ENUM_SHAPE(ITEM, P)                                                          \
  ITEM(P, NONE, 0, "none") ITEM(P, WRONG_WAY, 1, "WRONG-WAY") ITEM(P, UNORDERED, 2, "UNORDERED") \
  ITEM(P, ABSENT, 3, "ABSENT") ITEM(P, NOT_OBSERVED, 4, "NOT-OBSERVED") ITEM(P, STALE, 5, "STALE")

#define KZ_ENUM_CONSEQUENCE(ITEM, P)                                                    \
  ITEM(P, NOT_OBSERVED, 0, "not-observed") ITEM(P, CRASH_OR_UB, 1, "crash-or-ub")          \
  ITEM(P, DEADLOCK, 2, "deadlock") ITEM(P, PROCESS_ABORT, 3, "process-abort")              \
  ITEM(P, PANIC, 4, "panic") ITEM(P, TASK_LOST, 5, "task-lost")                            \
  ITEM(P, APP_ERROR_EXIT, 6, "app-error-exit") ITEM(P, APP_ZOMBIE, 7, "app-zombie")        \
  ITEM(P, CORE_SERVICE_TASK_ABSENT, 8, "core-service-task-absent")                      \
  ITEM(P, STATUS_IGNORED_EFFECT_LOST, 9, "status-ignored-effect-lost")                  \
  ITEM(P, EVENT_THEN_CONTINUE, 10, "event-then-continue")                               \
  ITEM(P, DELAYED_BUT_RECOVERED, 11, "delayed-but-recovered")                           \
  ITEM(P, MESSAGE_LOST, 12, "message-lost") ITEM(P, EVENT_REJECTED, 13, "event-rejected")

/* Caller-level harm (SPEC 1.6 `harm`).  0 is the default: unspecified by reference.
 * `event_request_rejected`: the event service refuses the request before it builds a packet
 * and the caller discards the status.  The request is "rejected", never "lost" (SPEC 0.9
 * item 4); tests/unit/test_core_vocab.cpp pins this list to the SPEC. */
#define KZ_ENUM_HARM(ITEM, P)                                                           \
  ITEM(P, UNSPECIFIED, 0, "unspecified_by_reference")                                   \
  ITEM(P, EVENT_REQUEST_REJECTED, 1, "event_request_rejected") ITEM(P, STATUS_IGNORED, 2, "status_ignored") \
  ITEM(P, WRONG_IDENTITY_STAMPED, 3, "wrong_identity_stamped")                          \
  ITEM(P, APP_ERROR_EXIT, 4, "app_error_exit") ITEM(P, APP_ZOMBIE, 5, "app_zombie")        \
  ITEM(P, SERVICE_TASK_ABSENT, 6, "service_task_absent")                                \
  ITEM(P, PROCESSOR_RESET_REQUESTED, 7, "processor_reset_requested")                    \
  ITEM(P, PROCESS_ABORT, 8, "process_abort") ITEM(P, PANIC, 9, "panic")                    \
  ITEM(P, TASK_NEVER_RUNS, 10, "task_never_runs") ITEM(P, HANG_OR_DEADLOCK, 11, "hang_or_deadlock") \
  ITEM(P, TERMINAL_PENDING_PANIC, 12, "terminal_pending(panic)")                        \
  ITEM(P, TERMINAL_PENDING_ABORT, 13, "terminal_pending(abort)")                        \
  ITEM(P, MESSAGE_LOST, 14, "message_lost")

#define KZ_ENUM_SUBLABEL(ITEM, P) /* set */                                             \
  ITEM(P, PORTABILITY_HAZARD, 0x1, "portability-hazard")                                \
  ITEM(P, IMPL_DEFECT_CANDIDATE, 0x2, "implementation-defect-candidate")                \
  ITEM(P, PORTABILITY_HAZARD_EVIDENCE, 0x4, "portability-hazard-evidence")              \
  ITEM(P, ASSUMED_VISIBILITY, 0x8, "assumed-visibility") ITEM(P, A_ZT, 0x10, "A-ZT")       \
  ITEM(P, TB_UNDECLARED, 0x20, "tb-undeclared")

#define KZ_ENUM_WITNESS_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, BAD, 1, "bad") ITEM(P, GOOD, 2, "good") ITEM(P, STALL, 3, "stall")

/* Status of an occurrence (SPEC 1.6).  0 is NOT_EVALUATED: neither ready nor not ready. */
#define KZ_ENUM_OCC_STATUS(ITEM, P)                                                     \
  ITEM(P, NOT_EVALUATED, 0, "NOT_EVALUATED") ITEM(P, READY, 1, "READY")                    \
  ITEM(P, TOLERATED, 2, "TOLERATED") ITEM(P, VIOLATING, 3, "VIOLATING")                    \
  ITEM(P, PENDING, 4, "PENDING") ITEM(P, NOT_READY_UNJUDGED, 5, "NOT_READY_UNJUDGED")

/* ---- Summary view and observation records (schema revision 2, SPEC rev 1.3) ---------------- */

/* Observation summary (SPEC 1.10, ADR-054).  0: no summary was computed (never read as a result). */
#define KZ_ENUM_OBS(ITEM, P)                                                            \
  ITEM(P, NONE, 0, "none") ITEM(P, OBSERVED_VIOLATION, 1, "OBSERVED_VIOLATION")            \
  ITEM(P, NO_VIOLATION_IN_CHECKED_PREFIX, 2, "NO_VIOLATION_IN_CHECKED_PREFIX")          \
  ITEM(P, INCONCLUSIVE, 3, "INCONCLUSIVE") ITEM(P, NOT_APPLICABLE, 4, "NOT_APPLICABLE")

/* Prediction summary (SPEC 1.10, 4.9 PR-01-PR-04). */
#define KZ_ENUM_PRED(ITEM, P)                                                           \
  ITEM(P, NOT_RUN, 0, "NOT_RUN") ITEM(P, MODEL_CANDIDATE, 1, "MODEL_CANDIDATE")            \
  ITEM(P, NONE_IN_SCOPE, 2, "NONE_IN_SCOPE") ITEM(P, INCONCLUSIVE, 3, "INCONCLUSIVE")

/* Confirmation summary (SPEC 1.10, 4.13). */
#define KZ_ENUM_CONF(ITEM, P) \
  ITEM(P, NOT_RUN, 0, "NOT_RUN") ITEM(P, CONFIRMED, 1, "CONFIRMED") ITEM(P, NOT_CONFIRMED, 2, "NOT_CONFIRMED")

/* Execution relation of an instance (SPEC 1.1 "Execution relations").  0: ownership undetermined. */
#define KZ_ENUM_EXEC_RELATION(ITEM, P)                                                  \
  ITEM(P, OWNERSHIP_UNDETERMINED, 0, "ownership-undetermined")                          \
  ITEM(P, INTER_APPLICATION, 1, "inter-application")                                    \
  ITEM(P, CORE_APPLICATION, 2, "core-application")                                      \
  ITEM(P, CREATION_FRAMEWORK, 3, "creation-framework-new-application")                  \
  ITEM(P, INTRA_APPLICATION, 4, "intra-application")

/* Reasons for withholding (SPEC 4.8 "Reasons for withholding", C-27). */
#define KZ_ENUM_WITHHOLD(ITEM, P) /* set */                                             \
  ITEM(P, START_STATE_UNKNOWN, 0x1, "start_state_unknown") ITEM(P, LOSS, 0x2, "loss")      \
  ITEM(P, RESOURCE_OR_LIFETIME_UNMATCHED, 0x4, "resource_or_lifetime_unmatched")        \
  ITEM(P, OWNERSHIP_UNDETERMINED, 0x8, "ownership_undetermined")                        \
  ITEM(P, EFFECT_POINT_AMBIGUOUS, 0x10, "effect_point_ambiguous")                       \
  ITEM(P, CALLER_REQUIREMENT_ABSENT, 0x20, "caller_requirement_absent")                 \
  ITEM(P, EFFECT_UNSUPPORTED, 0x40, "effect_unsupported")                               \
  ITEM(P, HORIZON_OPEN, 0x80, "horizon-open") ITEM(P, NOT_EVALUATED, 0x100, "not_evaluated") \
  ITEM(P, EXPLANATION_AMBIGUOUS, 0x200, "explanation_ambiguous") ITEM(P, LIMIT, 0x400, "limit")

/* A summary's occurrence status that is not one of the six statuses. */
#define KZ_ENUM_STATUS_FLAGS(ITEM, P) /* set */ \
  ITEM(P, OPEN, 0x1, "open") ITEM(P, UNDETERMINED, 0x2, "undetermined")

/* relation-inferred: the execution relation rests on at least one `inferred` app_owner (the weakest level
 * of the owners it used; absent on a determined relation: every owner it used is `confirmed`).
 * core-status-unknown: the relation is inter-application because the owners differ, but the record did
 * not establish for both owners whether they are core services, so core-application is not excluded
 * (2026-10-24, enum values only).
 * 2026-10-25 (enum values only; review section 5.3, 8.2, 6.3; PI decisions P1, P10): the basis of the result, more
 * than one possible -- basis-direct-capture (a fixed condition read only recorded argument values), basis-state-order
 * (one read replayed state: observed effects, synchronization and creation order, identity), basis-return-derived-order
 * (that state's order rests on a reads_from edge, an order derived from a returned code), and outcome-narrowed (the
 * returned code fixed a condition); the declared assumptions it used -- assumes-via-key-separation (`bind ... via`),
 * assumes-owner-identity-targets (A-OWNER), assumes-role (a role of the call context); and of the execution relation
 * (P10 rule) -- relation-unconfirmed-influence (a write the record does not link to the object, kept apart) and
 * relation-not-app-actor (an actor confirmed not to be an application's task: the initial thread). */
#define KZ_ENUM_SUMMARY_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, INPUT_ERROR, 0x1, "analysis-input-error") ITEM(P, PARTIAL, 0x2, "partial")       \
  ITEM(P, CALLER_RECORD, 0x4, "caller-record") ITEM(P, ACCEPT, 0x8, "accept")              \
  ITEM(P, REAL_RECORD, 0x10, "real-record") ITEM(P, OUTCOME_NARROWED, 0x20, "outcome-narrowed") \
  ITEM(P, RELATION_INFERRED, 0x40, "relation-inferred")                                  \
  ITEM(P, CORE_STATUS_UNKNOWN, 0x80, "core-status-unknown")                              \
  ITEM(P, BASIS_DIRECT_CAPTURE, 0x100, "basis-direct-capture")                           \
  ITEM(P, BASIS_STATE_ORDER, 0x200, "basis-state-order")                                 \
  ITEM(P, BASIS_RETURN_DERIVED_ORDER, 0x400, "basis-return-derived-order")               \
  ITEM(P, ASSUMES_VIA_SEPARATION, 0x800, "assumes-via-key-separation")                   \
  ITEM(P, ASSUMES_OWNER_IDENTITY, 0x1000, "assumes-owner-identity-targets")              \
  ITEM(P, ASSUMES_ROLE, 0x2000, "assumes-role")                                          \
  ITEM(P, UNCONFIRMED_INFLUENCE, 0x4000, "relation-unconfirmed-influence")               \
  ITEM(P, NOT_APP_ACTOR, 0x8000, "relation-not-app-actor")

/* What a record establishes for the resources and locations a condition reads (ADR-055). */
#define KZ_ENUM_ADEQ(ITEM, P) /* set */                                                 \
  ITEM(P, WRITERS_COVERED, 0x1, "writers-covered") ITEM(P, ALIASES_COVERED, 0x2, "aliases-covered") \
  ITEM(P, START_STATE_KNOWN, 0x4, "start-state-known") ITEM(P, LIFETIME_KNOWN, 0x8, "lifetime-known") \
  ITEM(P, PROBES_ATTACHED, 0x10, "probes-attached") ITEM(P, PROBES_ACTIVE, 0x20, "probes-active") \
  ITEM(P, LOSS_FREE, 0x40, "loss-free")

/* Evidence of the start state (SPEC 4.8 construction of Cobs, IN-03). */
#define KZ_ENUM_START_EVIDENCE(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, INIT, 1, "init") ITEM(P, CAPTURE, 2, "state-capture")   \
  ITEM(P, DECLARED, 3, "declared")

/* Why the checked interval ended. */
#define KZ_ENUM_INTERVAL_END(ITEM, P)                                                   \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, RECORD_END, 1, "record-end")                      \
  ITEM(P, PROCESS_TERMINAL, 2, "process-terminal") ITEM(P, LOSS, 3, "loss") ITEM(P, DECISION, 4, "decision")

/* Evidence level of app_owner (SPEC 1.2 "Creating application", ADR-056). */
#define KZ_ENUM_OWNER_LEVEL(ITEM, P) \
  ITEM(P, UNDETERMINED, 0, "undetermined") ITEM(P, INFERRED, 1, "inferred") ITEM(P, CONFIRMED, 2, "confirmed")

/* The evidence app_owner rests on (SPEC 5.7 items 1-4).  match-by-time: the creator link rests on the
 * containment of the fork record in the creating call alone (one thread's clock, A-CLK-PO);
 * id-unverified: the announced id was not compared with a creator-side value; analysed-table-excluded:
 * the task-application table was not used because its ordering is analysed (EV-03); creator-owner: a child
 * task's owner is its creating task's own app_owner (the request names no application), and the level is
 * at most the creating task's level. */
#define KZ_ENUM_OWNER_EVIDENCE(ITEM, P) /* set */                                       \
  ITEM(P, CREATION_REQUEST, 0x1, "creation-request") ITEM(P, FORK_IN_CALL, 0x2, "fork-in-creating-call") \
  ITEM(P, ANNOUNCED_ID, 0x4, "announced-id") ITEM(P, ES_RECORDS, 0x8, "es-records")        \
  ITEM(P, MATCH_BY_TIME, 0x10, "match-by-time") ITEM(P, ID_UNVERIFIED, 0x20, "id-unverified") \
  ITEM(P, RAW_THREAD, 0x40, "raw-thread") ITEM(P, ANALYSED_TABLE_EXCLUDED, 0x80, "analysed-table-excluded") \
  ITEM(P, CREATOR_OWNER, 0x100, "creator-owner")

#define KZ_ENUM_IDENTITY_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, EXEC_KNOWN, 0x1, "exec-known") ITEM(P, CREATOR_KNOWN, 0x2, "creator-known")      \
  ITEM(P, PASSED_ID_PRESENT, 0x4, "passed-id-present")

/* ITEM(P, id, value, display) lists registered for dumping.  D(prefix, list, is_set). */
#define KZ_CORE_ENUMS(D)                                            \
  D(KZ_ARTIFACT, KZ_ENUM_ARTIFACT_KIND, 0)                          \
  D(KZ_POLICY, KZ_ENUM_POLICY, 0)                                   \
  D(KZ_AVOIDABLE, KZ_ENUM_AVOIDABLE, 0)                             \
  D(KZ_TIER, KZ_ENUM_TIER, 0)                                       \
  D(KZ_PROV_KIND, KZ_ENUM_PROV_KIND, 0)                             \
  D(KZ_PROV_SIDE, KZ_ENUM_PROV_SIDE, 0)                             \
  D(KZ_SIGMA, KZ_ENUM_SIGMA, 0)                                     \
  D(KZ_MLEVEL, KZ_ENUM_MLEVEL, 0)                                   \
  D(KZ_CLS, KZ_ENUM_CLS, 0)                                         \
  D(KZ_UNKNOWN_REASON, KZ_ENUM_UNKNOWN_REASON, 0)                   \
  D(KZ_REACH, KZ_ENUM_REACH, 0)                                     \
  D(KZ_PROGRESS, KZ_ENUM_PROGRESS, 0)                               \
  D(KZ_PROGRESS_REASON, KZ_ENUM_PROGRESS_REASON, 0)                 \
  D(KZ_PROGRESS_HARM, KZ_ENUM_PROGRESS_HARM, 0)                     \
  D(KZ_PROGRESS_REALIZATION, KZ_ENUM_PROGRESS_REALIZATION, 0)       \
  D(KZ_PROGRESS_KIND, KZ_ENUM_PROGRESS_KIND, 1)                     \
  D(KZ_PROGRESS_FLAG, KZ_ENUM_PROGRESS_FLAGS, 1)                    \
  D(KZ_ROUTE, KZ_ENUM_ROUTE, 0)                                     \
  D(KZ_REFINEMENT, KZ_ENUM_REFINEMENT, 1)                           \
  D(KZ_REALIZATION, KZ_ENUM_REALIZATION, 1)                         \
  D(KZ_NATURALNESS, KZ_ENUM_NATURALNESS, 1)                         \
  D(KZ_CELL_FLAG, KZ_ENUM_CELL_FLAGS, 1)                            \
  D(KZ_FW_CONF, KZ_ENUM_FW_CONF, 0)                                 \
  D(KZ_FW_DOC, KZ_ENUM_FW_DOC, 1)                                   \
  D(KZ_PROVIDER, KZ_ENUM_PROVIDER, 0)                               \
  D(KZ_SCOPE, KZ_ENUM_SCOPE, 0)                                     \
  D(KZ_OUTCOME_LABEL, KZ_ENUM_OUTCOME_LABEL, 0)                     \
  D(KZ_SHAPE, KZ_ENUM_SHAPE, 0)                                     \
  D(KZ_CONSEQUENCE, KZ_ENUM_CONSEQUENCE, 0)                         \
  D(KZ_HARM, KZ_ENUM_HARM, 0)                                       \
  D(KZ_SUBLABEL, KZ_ENUM_SUBLABEL, 1)                               \
  D(KZ_WITNESS_KIND, KZ_ENUM_WITNESS_KIND, 0)                       \
  D(KZ_OCC, KZ_ENUM_OCC_STATUS, 0)                                  \
  D(KZ_OBS, KZ_ENUM_OBS, 0)                                         \
  D(KZ_PRED, KZ_ENUM_PRED, 0)                                       \
  D(KZ_CONF, KZ_ENUM_CONF, 0)                                       \
  D(KZ_EXEC_RELATION, KZ_ENUM_EXEC_RELATION, 0)                     \
  D(KZ_WITHHOLD, KZ_ENUM_WITHHOLD, 1)                               \
  D(KZ_STATUS_FLAG, KZ_ENUM_STATUS_FLAGS, 1)                        \
  D(KZ_SUMMARY_FLAG, KZ_ENUM_SUMMARY_FLAGS, 1)                      \
  D(KZ_ADEQ, KZ_ENUM_ADEQ, 1)                                       \
  D(KZ_START_EVIDENCE, KZ_ENUM_START_EVIDENCE, 0)                   \
  D(KZ_INTERVAL_END, KZ_ENUM_INTERVAL_END, 0)                       \
  D(KZ_OWNER_LEVEL, KZ_ENUM_OWNER_LEVEL, 0)                         \
  D(KZ_OWNER_EVIDENCE, KZ_ENUM_OWNER_EVIDENCE, 1)                   \
  D(KZ_IDENTITY_FLAG, KZ_ENUM_IDENTITY_FLAGS, 1)

#define KZ_DEFINE_ENUM_ENTRY(P, LIST, is_set) KZ_DEFINE_ENUM(P, LIST, is_set)
KZ_CORE_ENUMS(KZ_DEFINE_ENUM_ENTRY)

/* Bits of a verdict cell's `safety_routes` field. */
#define KZ_ROUTE_NOT_BAD_SHIFT 0
#define KZ_ROUTE_NOT_OK_SHIFT 2
#define KZ_ROUTE_NOT_NR_SHIFT 4
#define KZ_ROUTE_MASK 0x3u
#define KZ_ROUTES_VALID_MASK 0x3Fu /* bits 6-7 must be zero */

/* ------------------------------------------------------------------------- */
/* Field presentation (dump only; has no effect on the layout)               */
/* ------------------------------------------------------------------------- */
/* F(record, field, kind, arg): kind is ENUM|FLAGS (arg = enum prefix), HEX|IDX|ROUTES (arg = 0). */
#define KZ_CORE_FORMATS(F)                                           \
  F(kz_header, artifact_kind, ENUM, KZ_ARTIFACT)                     \
  F(kz_header, header_crc, HEX, 0)                                   \
  F(kz_parent, kind, ENUM, KZ_ARTIFACT)                              \
  F(kz_section_entry, crc32, HEX, 0)                                 \
  F(kz_prov, tier, ENUM, KZ_TIER)                                    \
  F(kz_prov, kind, ENUM, KZ_PROV_KIND)                               \
  F(kz_prov, side, ENUM, KZ_PROV_SIDE)                               \
  F(kz_deviation, avoidable, ENUM, KZ_AVOIDABLE)                     \
  F(kz_deviation, prov, IDX, 0)                                      \
  F(kz_basis_item, policy, ENUM, KZ_POLICY)                          \
  F(kz_basis_item, prov, IDX, 0)                                     \
  F(kz_basis_item, deviation, IDX, 0)                                \
  F(kz_file_hash, prov, IDX, 0)                                      \
  F(kz_witness, kind, ENUM, KZ_WITNESS_KIND)                         \
  F(kz_witness, status, ENUM, KZ_OCC)                                \
  F(kz_witness, naturalness, FLAGS, KZ_NATURALNESS)                  \
  F(kz_witness, realization, FLAGS, KZ_REALIZATION)                  \
  F(kz_bound, h, BOUND, 0)                                             \
  F(kz_bound, policy, ENUM, KZ_POLICY)                               \
  F(kz_bound, cls, ENUM, KZ_CLS)                                     \
  F(kz_bound, reach, ENUM, KZ_REACH)                                 \
  F(kz_bound, progress, ENUM, KZ_PROGRESS)                           \
  F(kz_bound, progress_kind, FLAGS, KZ_PROGRESS_KIND)                \
  F(kz_bound, safety_routes, ROUTES, 0)                              \
  F(kz_bound, flags, FLAGS, KZ_CELL_FLAG)                            \
  F(kz_bound, unknown_reason, ENUM, KZ_UNKNOWN_REASON)               \
  F(kz_bound, progress_reason, ENUM, KZ_PROGRESS_REASON)             \
  F(kz_bound, progress_harm, ENUM, KZ_PROGRESS_HARM)                 \
  F(kz_bound, refinements, FLAGS, KZ_REFINEMENT)                     \
  F(kz_bound, progress_flags, FLAGS, KZ_PROGRESS_FLAG)               \
  F(kz_verdict, instance, HEX, 0)                                    \
  F(kz_verdict, provider, ENUM, KZ_PROVIDER)                         \
  F(kz_verdict, scope, ENUM, KZ_SCOPE)                               \
  F(kz_verdict, outcome_label, ENUM, KZ_OUTCOME_LABEL)               \
  F(kz_verdict, shape, ENUM, KZ_SHAPE)                               \
  F(kz_verdict, consequence, ENUM, KZ_CONSEQUENCE)                   \
  F(kz_verdict, harm, ENUM, KZ_HARM)                                 \
  F(kz_verdict, fw_doc, FLAGS, KZ_FW_DOC)                            \
  F(kz_verdict, sublabels, FLAGS, KZ_SUBLABEL)                       \
  F(kz_verdict_cell, sigma, ENUM, KZ_SIGMA)                          \
  F(kz_verdict_cell, mlevel, ENUM, KZ_MLEVEL)                        \
  F(kz_verdict_cell, cls, ENUM, KZ_CLS)                              \
  F(kz_verdict_cell, unknown_reason, ENUM, KZ_UNKNOWN_REASON)        \
  F(kz_verdict_cell, refinements, FLAGS, KZ_REFINEMENT)              \
  F(kz_verdict_cell, realization, FLAGS, KZ_REALIZATION)             \
  F(kz_verdict_cell, naturalness, FLAGS, KZ_NATURALNESS)             \
  F(kz_verdict_cell, flags, FLAGS, KZ_CELL_FLAG)                     \
  F(kz_verdict_cell, reach, ENUM, KZ_REACH)                          \
  F(kz_verdict_cell, progress, ENUM, KZ_PROGRESS)                    \
  F(kz_verdict_cell, progress_reason, ENUM, KZ_PROGRESS_REASON)      \
  F(kz_verdict_cell, progress_harm, ENUM, KZ_PROGRESS_HARM)          \
  F(kz_verdict_cell, progress_realization, ENUM, KZ_PROGRESS_REALIZATION) \
  F(kz_verdict_cell, safety_routes, ROUTES, 0)                       \
  F(kz_verdict_cell, progress_flags, FLAGS, KZ_PROGRESS_FLAG)        \
  F(kz_verdict_cell, progress_kind, FLAGS, KZ_PROGRESS_KIND)         \
  F(kz_verdict_cell, fw_conf, ENUM, KZ_FW_CONF)                      \
  F(kz_verdict_cell, proof_status, ENUM, KZ_PROOF)                   \
  F(kz_summary, cell, IDX, 0)                                        \
  F(kz_summary, observation, ENUM, KZ_OBS)                           \
  F(kz_summary, prediction, ENUM, KZ_PRED)                           \
  F(kz_summary, confirmation, ENUM, KZ_CONF)                         \
  F(kz_summary, relation, ENUM, KZ_EXEC_RELATION)                    \
  F(kz_summary, obs_reasons, FLAGS, KZ_WITHHOLD)                     \
  F(kz_summary, pred_reasons, FLAGS, KZ_WITHHOLD)                    \
  F(kz_summary, status, ENUM, KZ_OCC)                                \
  F(kz_summary, status_flags, FLAGS, KZ_STATUS_FLAG)                 \
  F(kz_summary, flags, FLAGS, KZ_SUMMARY_FLAG)                       \
  F(kz_summary, model, IDX, 0)                                       \
  F(kz_summary, adequacy, IDX, 0)                                    \
  F(kz_summary, identity_first, IDX, 0)                              \
  F(kz_model_id, model_level, ENUM, KZ_MLEVEL)                       \
  F(kz_model_id, sigma, ENUM, KZ_SIGMA)                              \
  F(kz_obs_adequacy, established, FLAGS, KZ_ADEQ)                    \
  F(kz_obs_adequacy, absent, FLAGS, KZ_ADEQ)                         \
  F(kz_obs_adequacy, reasons, FLAGS, KZ_WITHHOLD)                    \
  F(kz_obs_adequacy, start_evidence, ENUM, KZ_START_EVIDENCE)        \
  F(kz_obs_adequacy, end_reason, ENUM, KZ_INTERVAL_END)              \
  F(kz_obs_adequacy, first_event, IDX, 0)                            \
  F(kz_obs_adequacy, last_event, IDX, 0)                             \
  F(kz_obs_adequacy, loss_reports, IDX, 0)                           \
  F(kz_identity, creator_tid, IDX, 0)                                \
  F(kz_identity, creator_generation, IDX, 0)                         \
  F(kz_identity, owner_key, IDX, 0)                                  \
  F(kz_identity, owner_level, ENUM, KZ_OWNER_LEVEL)                  \
  F(kz_identity, owner_evidence, FLAGS, KZ_OWNER_EVIDENCE)           \
  F(kz_identity, passed_id, HEX, 0)                                  \
  F(kz_identity, flags, FLAGS, KZ_IDENTITY_FLAG)

/* ------------------------------------------------------------------------- */
/* Component-owned parts                                                     */
/* ------------------------------------------------------------------------- */
#include "kz/schema/kz_raw.h"
#include "kz/schema/kz_facts.h"
#include "kz/schema/kz_cert.h"
#include "kz/schema/kz_neutral.h"

/* Everything the registry in lib/core is built from. */
#define KZ_ALL_RECORDS(R) \
  KZ_CORE_RECORDS(R) KZ_RAW_RECORDS(R) KZ_FACTS_RECORDS(R) KZ_CERT_RECORDS(R) KZ_NEUTRAL_RECORDS(R)
#define KZ_ALL_ENUMS(D) KZ_CORE_ENUMS(D) KZ_RAW_ENUMS(D) KZ_FACTS_ENUMS(D) KZ_CERT_ENUMS(D) KZ_NEUTRAL_ENUMS(D)
#define KZ_ALL_FORMATS(F) \
  KZ_CORE_FORMATS(F) KZ_RAW_FORMATS(F) KZ_FACTS_FORMATS(F) KZ_CERT_FORMATS(F) KZ_NEUTRAL_FORMATS(F)
#define KZ_ALL_SECTIONS(S) \
  KZ_CORE_SECTIONS(S) KZ_RAW_SECTIONS(S) KZ_FACTS_SECTIONS(S) KZ_CERT_SECTIONS(S) KZ_NEUTRAL_SECTIONS(S)

/* The ids in the section lists and in the enumerators must agree. */
#define KZ_SECTION_ID_CHECK(NAME, id, record, version) \
  KZ_STATIC_ASSERT(KZ_SEC_##NAME##_id_matches_list, (unsigned)KZ_SEC_##NAME == (unsigned)(id));
KZ_ALL_SECTIONS(KZ_SECTION_ID_CHECK)

#endif /* KZ_SCHEMA_KZ_SCHEMA_H */

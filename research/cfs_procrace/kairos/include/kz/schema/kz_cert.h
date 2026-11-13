/*
 * kz_cert.h -- certificate and trust-ledger records: initial, owned by K4b (static
 * analysis and the independent checker).
 *
 * Derived from SPEC 2.3 (KZ_CERT_STEP, KZ_CERT, KZ_TRUST, KZ_TRUST_LEDGER), 4.5 (what a
 * certificate is and what the checker re-validates) and 4.5.1 (the four-part trust
 * ledger).  Refinements made at S0 (docs/schema.md):
 *   - the steps and the ledger entries are sections of their own (CERT_STEPS, TRUST);
 *     the certificate and the ledger name their range with `first_step` / `first`;
 *   - the proof status (D20: `proved`, `depends-on-stated-assumption`, `unproved`) is carried
 *     by `KZ_TRUST.proof` (per lemma entry), `KZ_CERT.proof_status` and
 *     `KZ_VERDICT_CELL.proof_status` (the weakest status among the lemmas used), as SPEC
 *     rev 1.2 states in 2.3 and 6.2 (PS3); a lemma that is only listed as an obligation is
 *     `unproved` and is never shown as proved (PS2);
 *   - the profile (SPEC: Phi) is called `profile` in identifiers.
 *
 * Not usable alone: include kz_schema.h.
 */
#ifndef KZ_SCHEMA_KZ_CERT_H
#define KZ_SCHEMA_KZ_CERT_H

#ifndef KZ_SCHEMA_KZ_SCHEMA_H
#error "include kz/schema/kz_schema.h, not this file"
#endif

enum kz_cert_section_id {
  KZ_SEC_CERTIFICATES = 60,
  KZ_SEC_CERT_STEPS = 61,
  KZ_SEC_TRUST_LEDGERS = 62,
  KZ_SEC_TRUST = 63,
  KZ_SEC_OBLIGATIONS = 64
};

#define KZ_CERT_SECTIONS(S)                \
  S(CERTIFICATES, 60, kz_cert, 1)          \
  S(CERT_STEPS, 61, kz_cert_step, 1)       \
  S(TRUST_LEDGERS, 62, kz_trust_ledger, 1) \
  S(TRUST, 63, kz_trust, 1)                \
  S(OBLIGATIONS, 64, kz_obligation, 1)

/* One inference step of a certificate (SPEC 2.3; `flags` added at S0). */
#define KZ_CERT_STEP_FIELDS(X, T)                                                    \
  X(T, uint32_t, rule, )       /* KZ_RULE_* */                                       \
  X(T, uint32_t, from_node, )  /* certificate-view nodes */                          \
  X(T, uint32_t, to_node, )                                                          \
  X(T, uint32_t, site_from, )  /* -> ANCHORS */                                      \
  X(T, uint32_t, site_to, )                                                          \
  X(T, uint32_t, profile_entry, ) /* profile rule / effect / binding id */            \
  X(T, uint16_t, tstatus, )    /* COMPUTED|DECLARED_VERIFIED|DECLARED_TRUSTED|ASSUMED_VIS */ \
  X(T, uint16_t, kstatus, )    /* MUST_KEY|MAY_KEY|NO_KEY|NA */                      \
  X(T, uint32_t, view, )       /* VIEW_CERT; any other value is rejected by the checker */ \
  X(T, uint16_t, flags, )      /* AT_DECISION | AFTER_DECISION | REJECT_EVENT */     \
  X(T, uint16_t, rsv0, )

/* One certificate (SPEC 2.3; `first_step` and `rsv1` added at S0). */
#define KZ_CERT_FIELDS(X, T)                                                         \
  X(T, uint64_t, instance, ) /* (rspec, context-qualified template, key) */          \
  X(T, uint8_t, kind, )      /* FIXED_ORDER | PROTECTED | INACTIVE */                \
  X(T, uint8_t, mlevel, )    /* M_API | M_IMPL */                                    \
  X(T, uint8_t, flags, )     /* DECIDES | CONTESTED */                               \
  X(T, uint8_t, proof_status, ) /* weakest LEMMA status of its ledger part (iii); written by kz proofcheck */ \
  X(T, uint32_t, nsteps, )                                                           \
  X(T, uint32_t, first_step, ) /* CERT_STEPS index of the first of nsteps steps */   \
  X(T, uint32_t, rsv1, )                                                             \
  X(T, uint8_t, contested_profile, [32]) /* profile hash for which CONTESTED holds; zero if unset */ \
  X(T, kz_ref, trust, )        /* its ledger */                                      \
  X(T, kz_ref, contested_by, ) /* the exhaustive-explorer witness (O10) */

/* One trust-ledger entry (SPEC 4.5.1; `proof` is the status of a LEMMA entry, SPEC 2.3). */
#define KZ_TRUST_FIELDS(X, T)                                                        \
  X(T, uint8_t, part, )   /* RECHECKED | EXTRACTED | TRUSTED | REFUTABLE = parts (i)-(iv) */ \
  X(T, uint8_t, kind, )   /* see KZ_TRUST_KIND */                                    \
  X(T, uint8_t, status, ) /* (i) ACCEPTED|ACCEPTED_VIA_T5; (iv) NOT_RUN|NOT_REFUTED_ON_EXERCISED|REFUTED; else NA */ \
  X(T, uint8_t, proof, )  /* LEMMA entries only: UNPROVED|DEPENDS|PROVED, copied from the lemma table of SPEC 6.2; else NA */ \
  X(T, uint32_t, ident, ) /* step index | profile entry id | assumption, lemma or obligation id */ \
  X(T, uint32_t, site, )  /* -> ANCHORS, or KZ_NONE32 */                             \
  X(T, uint32_t, refutes, ) /* (iv): index of the (ii)/(iii) entry it can refute, else KZ_NONE32 */ \
  X(T, uint32_t, bound, ) /* the bound the entry supports: h, or KZ_BOUND_INFINITE */ \
  X(T, kz_ref, source, )  /* the record itself (for a PROVED lemma: the record of the text that holds the proof); for (iv) the OBLIGATIONS record */

/* One ledger per certificate and per verdict cell (SPEC 4.5.1). */
#define KZ_TRUST_LEDGER_FIELDS(X, T)                                                 \
  X(T, uint32_t, n_rechecked, )                                                      \
  X(T, uint32_t, n_extracted, )                                                      \
  X(T, uint32_t, n_trusted, ) /* all of part (iii) */                                \
  X(T, uint32_t, n_refutable, )                                                      \
  X(T, uint32_t, first, )  /* entry range in section TRUST */                        \
  X(T, uint32_t, count, )                                                            \
  X(T, kz_ref, facts, )                                                              \
  X(T, kz_ref, skel, )                                                               \
  X(T, kz_ref, anchors, )                                                            \
  X(T, kz_ref, profile, )                                                            \
  X(T, kz_ref, checker, ) /* tool-hash record of the checker build that accepted */

/*
 * One obligation result (SPEC 4.5.1 part iv): the run set R it was checked on and the
 * exercised-path limit.  Passing an obligation is never recorded as proof.
 */
#define KZ_OBLIGATION_FIELDS(X, T)                                                   \
  X(T, uint32_t, obligation, ) /* KZ_OBL_* */                                        \
  X(T, uint8_t, status, )      /* NOT_RUN | NOT_REFUTED_ON_EXERCISED | REFUTED */    \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, first_run, )  /* range in REF_LISTS: the runs R */                   \
  X(T, uint32_t, n_runs, )                                                           \
  X(T, kz_str, exercised_limit, )                                                    \
  X(T, kz_ref, witness, ) /* refuting evidence when REFUTED */

#define KZ_CERT_RECORDS(R)                 \
  R(kz_cert_step, KZ_CERT_STEP_FIELDS)     \
  R(kz_cert, KZ_CERT_FIELDS)               \
  R(kz_trust, KZ_TRUST_FIELDS)             \
  R(kz_trust_ledger, KZ_TRUST_LEDGER_FIELDS) \
  R(kz_obligation, KZ_OBLIGATION_FIELDS)

KZ_CERT_RECORDS(KZ_RECORD)

/* Enumerations ------------------------------------------------------------- */

#define KZ_ENUM_RULE(ITEM, P)                                                        \
  ITEM(P, NONE, 0, "none") ITEM(P, T_PO, 1, "T_PO") ITEM(P, T_SPAWN, 2, "T_SPAWN")   \
  ITEM(P, T_OBS, 3, "T_OBS") ITEM(P, T_TERM, 4, "T_TERM") ITEM(P, FRONTIER, 5, "FRONTIER") \
  ITEM(P, ONCE, 6, "ONCE") ITEM(P, ONCE_PWB, 7, "ONCE_PWB") ITEM(P, OWN_NOPATH, 8, "OWN_NOPATH") \
  ITEM(P, KEY_SEP, 9, "KEY_SEP") ITEM(P, SCOPE, 10, "SCOPE") ITEM(P, WRITER, 11, "WRITER") \
  ITEM(P, INIT_FALSE, 12, "INIT_FALSE") ITEM(P, INIT_TRUE, 13, "INIT_TRUE")           \
  ITEM(P, DERIVES, 14, "DERIVES") ITEM(P, PRED_EXTRACT, 15, "PRED_EXTRACT")           \
  ITEM(P, BADNESS, 16, "BADNESS") ITEM(P, STATUS_CONSIST, 17, "STATUS_CONSIST")       \
  ITEM(P, REACH, 18, "REACH") ITEM(P, VIEW, 19, "VIEW")

#define KZ_ENUM_TSTATUS(ITEM, P)                                                     \
  ITEM(P, NA, 0, "n/a") ITEM(P, COMPUTED, 1, "COMPUTED")                              \
  ITEM(P, DECLARED_VERIFIED, 2, "DECLARED_VERIFIED")                                 \
  ITEM(P, DECLARED_TRUSTED, 3, "DECLARED_TRUSTED") ITEM(P, ASSUMED_VIS, 4, "ASSUMED_VIS") \
  ITEM(P, DISABLED, 5, "DISABLED")

#define KZ_ENUM_KSTATUS(ITEM, P)                                                     \
  ITEM(P, NA, 0, "NA") ITEM(P, MUST_KEY, 1, "MUST_KEY") ITEM(P, MAY_KEY, 2, "MAY_KEY") \
  ITEM(P, NO_KEY, 3, "NO_KEY")

#define KZ_ENUM_VIEW(ITEM, P) ITEM(P, NONE, 0, "none") ITEM(P, CERT, 1, "VIEW_CERT") ITEM(P, EXPL, 2, "VIEW_EXPL")

#define KZ_ENUM_STEP_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, AT_DECISION, 0x1, "at-decision") ITEM(P, AFTER_DECISION, 0x2, "after-decision") \
  ITEM(P, REJECT_EVENT, 0x4, "reject-event")

#define KZ_ENUM_CERT_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, FIXED_ORDER, 1, "FIXED_ORDER")                    \
  ITEM(P, PROTECTED, 2, "PROTECTED") ITEM(P, INACTIVE, 3, "INACTIVE")

#define KZ_ENUM_CERT_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, DECIDES, 0x1, "decides") ITEM(P, CONTESTED, 0x2, "contested")

#define KZ_ENUM_TRUST_PART(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, RECHECKED, 1, "i-rechecked")                      \
  ITEM(P, EXTRACTED, 2, "ii-extracted") ITEM(P, TRUSTED, 3, "iii-trusted")           \
  ITEM(P, REFUTABLE, 4, "iv-refutable")

#define KZ_ENUM_TRUST_KIND(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, CERT_STEP, 1, "CERT_STEP") ITEM(P, FACT, 2, "FACT") \
  ITEM(P, SKEL, 3, "SKEL") ITEM(P, CLOSURE, 4, "CLOSURE") ITEM(P, ANCHOR, 5, "ANCHOR") \
  ITEM(P, RUN, 6, "RUN") ITEM(P, PROBE, 7, "PROBE")                                  \
  ITEM(P, PROFILE_ENTRY, 8, "PROFILE_ENTRY") ITEM(P, DECLARED_TRUSTED, 9, "DECLARED_TRUSTED") \
  ITEM(P, ASSUMPTION, 10, "ASSUMPTION") ITEM(P, LEMMA, 11, "LEMMA")                  \
  ITEM(P, TOOL_BASE, 12, "TOOL_BASE") ITEM(P, OBLIGATION, 13, "OBLIGATION")

#define KZ_ENUM_TRUST_STATUS(ITEM, P)                                                \
  ITEM(P, NA, 0, "n/a") ITEM(P, ACCEPTED, 1, "ACCEPTED")                              \
  ITEM(P, ACCEPTED_VIA_T5, 2, "ACCEPTED_VIA_T5") ITEM(P, NOT_RUN, 3, "NOT_RUN")      \
  ITEM(P, NOT_REFUTED_ON_EXERCISED, 4, "NOT_REFUTED_ON_EXERCISED")                   \
  ITEM(P, REFUTED, 5, "REFUTED")

/*
 * Proof status of a lemma (D20, SPEC 6.2).  `unproved` is the status of a lemma whose proof
 * obligation is only written down (PS2).  Only `proved` may be shown as proved; a ledger
 * entry marked PROVED must name in `source` the record of the text that holds the proof (PS1:
 * a status is a property of that text; a run, a test or a ledger entry never raises it).
 * Numeric order is the strength order, with 0 = not applicable (no lemma).
 */
#define KZ_ENUM_PROOF(ITEM, P)                                                       \
  ITEM(P, NA, 0, "n/a") ITEM(P, UNPROVED, 1, "unproved")                             \
  ITEM(P, DEPENDS, 2, "depends-on-stated-assumption") ITEM(P, PROVED, 3, "proved")

#define KZ_ENUM_OBL(ITEM, P)                                                         \
  ITEM(P, NONE, 0, "none") ITEM(P, O1, 1, "O1") ITEM(P, O2, 2, "O2") ITEM(P, O3, 3, "O3") \
  ITEM(P, O4, 4, "O4") ITEM(P, O5, 5, "O5") ITEM(P, O6, 6, "O6") ITEM(P, O7, 7, "O7") \
  ITEM(P, O8, 8, "O8") ITEM(P, O9, 9, "O9") ITEM(P, O10, 10, "O10") ITEM(P, CC, 11, "CC")

#define KZ_CERT_ENUMS(D)                       \
  D(KZ_RULE, KZ_ENUM_RULE, 0)                  \
  D(KZ_TSTATUS, KZ_ENUM_TSTATUS, 0)            \
  D(KZ_KSTATUS, KZ_ENUM_KSTATUS, 0)            \
  D(KZ_VIEW, KZ_ENUM_VIEW, 0)                  \
  D(KZ_STEP_FLAG, KZ_ENUM_STEP_FLAGS, 1)       \
  D(KZ_CERT_KIND, KZ_ENUM_CERT_KIND, 0)        \
  D(KZ_CERT_FLAG, KZ_ENUM_CERT_FLAGS, 1)       \
  D(KZ_TRUST_PART, KZ_ENUM_TRUST_PART, 0)      \
  D(KZ_TRUST_KIND, KZ_ENUM_TRUST_KIND, 0)      \
  D(KZ_TRUST_STATUS, KZ_ENUM_TRUST_STATUS, 0)  \
  D(KZ_PROOF, KZ_ENUM_PROOF, 0)                \
  D(KZ_OBL, KZ_ENUM_OBL, 0)

KZ_CERT_ENUMS(KZ_DEFINE_ENUM_ENTRY)

#define KZ_CERT_FORMATS(F)                          \
  F(kz_cert_step, rule, ENUM, KZ_RULE)              \
  F(kz_cert_step, profile_entry, IDX, 0)         \
  F(kz_cert_step, site_from, IDX, 0)                \
  F(kz_cert_step, site_to, IDX, 0)                  \
  F(kz_cert_step, tstatus, ENUM, KZ_TSTATUS)        \
  F(kz_cert_step, kstatus, ENUM, KZ_KSTATUS)        \
  F(kz_cert_step, view, ENUM, KZ_VIEW)              \
  F(kz_cert_step, flags, FLAGS, KZ_STEP_FLAG)       \
  F(kz_cert, instance, HEX, 0)                      \
  F(kz_cert, kind, ENUM, KZ_CERT_KIND)              \
  F(kz_cert, mlevel, ENUM, KZ_MLEVEL)               \
  F(kz_cert, flags, FLAGS, KZ_CERT_FLAG)            \
  F(kz_cert, proof_status, ENUM, KZ_PROOF)          \
  F(kz_trust, part, ENUM, KZ_TRUST_PART)            \
  F(kz_trust, kind, ENUM, KZ_TRUST_KIND)            \
  F(kz_trust, status, ENUM, KZ_TRUST_STATUS)        \
  F(kz_trust, proof, ENUM, KZ_PROOF)                \
  F(kz_trust, site, IDX, 0)                         \
  F(kz_trust, refutes, IDX, 0)                      \
  F(kz_trust, bound, BOUND, 0)                        \
  F(kz_obligation, obligation, ENUM, KZ_OBL)        \
  F(kz_obligation, status, ENUM, KZ_TRUST_STATUS)

#endif /* KZ_SCHEMA_KZ_CERT_H */

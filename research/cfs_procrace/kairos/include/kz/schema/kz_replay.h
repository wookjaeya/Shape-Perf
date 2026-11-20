/*
 * kz_replay.h -- the per-occurrence basis of a real-record replay output container (schema revision 4).
 *
 * A container of kind `verdicts` written by `kz replay --out` (kz/trace/replay_container.hpp, docs/schema.md
 * section 11) holds one SUMMARIES record per occurrence.  Its 16-bit `flags` hold the summary view, the
 * basis kinds direct-capture, state-order, return-derived-order and outcome-narrowed and three declared
 * assumptions, and have no room for the basis kinds added later (store-derived-order 2026-10-26,
 * control-order P11 2026-10-26) nor for the edges a result rests on (plan/results_instr_pilot_20261026.md
 * section 8.2 item 4).  Revision 4 adds three NEW sections, so every container written before is read as it
 * was written; a reader of such a container reports the per-occurrence basis as not recorded, never as an
 * empty basis (docs/replay_output.md):
 *
 *   OCC_BASIS    one record per SUMMARIES record, in the same order: the verdict and its summary view, the
 *                full set of basis kinds and declared assumptions, the outcome check, the path-independent
 *                key of the occurrence (rspec, operation, call context, owning application, resource key and
 *                the text arguments of the call, ordinal), and its run of OCC_EDGES;
 *   OCC_EDGES    the happens-before edges between actors that lie on an order the result read: for a fixed
 *                condition that read replayed state, every edge on a path between the read and a write of
 *                another actor it is ordered with, in the order of the replay's previous pass (the order the
 *                result rests on); the rule name, the kind (a rule of the profile, a reads_from rule, the
 *                realized control of a controlled run) and the endpoints in the replayed trace;
 *   REPLAY_READ  one record: the reading of the replay (every option that changes a result, recorded whether
 *                or not it has its default value), the number of control edges and of events the trace's
 *                linearization moved.
 *
 * Event positions (`call`, `decision`, `from`, `to`) are positions in the replayed trace: they depend on the
 * record and its backend, and are kept for audit only.  The key fields are what a comparison of two records
 * (`kz occ diff`) reads.
 *
 * Byte order: sections 100-119 are serialized field by field (kz/core/record_codec.hpp).
 *
 * Not usable alone: include kz_schema.h.
 */
#ifndef KZ_SCHEMA_KZ_REPLAY_H
#define KZ_SCHEMA_KZ_REPLAY_H

#ifndef KZ_SCHEMA_KZ_SCHEMA_H
#error "include kz/schema/kz_schema.h, not this file"
#endif

enum kz_replay_section_id { KZ_SEC_OCC_BASIS = 108, KZ_SEC_OCC_EDGES = 109, KZ_SEC_REPLAY_READ = 110 };

#define KZ_REPLAY_SECTIONS(S)         \
  S(OCC_BASIS, 108, kz_occ_basis, 1)  \
  S(OCC_EDGES, 109, kz_occ_edge, 1)   \
  S(REPLAY_READ, 110, kz_replay_read, 1)

/*
 * One occurrence.  `summary` is its SUMMARIES index; `status`, `status_flags`, `observation` and `obs_reasons`
 * repeat that record's verdict and summary view so that the section is read on its own.  `basis` and
 * `assumptions` are the full sets (KZ_OCC_BASIS, KZ_OCC_ASSUME); `outcome` how the returned code was checked
 * (KZ_OUTCOME_CHECK).  The key: `rspec`, `op` (the operation called at the use site), `context` (the role of the
 * call context, empty if none), the owning application of the executing task (`owner_key` = its record key in
 * the profile's owner kind and `owner_level`, KZ_NONE32 / undetermined where the record does not establish it;
 * `owner_app` its name where the record gives one), `resource` (the occurrence's key values, hex, `,`-separated;
 * empty where not fixed), `name` (the text arguments of the call, `;`-separated, e.g. the name looked up) and
 * `ordinal` (the k-th occurrence the executing task began with that key, from 1).  `via` names the kinds and
 * locations whose writer coverage rests on a `bind ... via` declaration, `;`-separated.
 * `task` (schema revision 5; was reserved): the executing task's place among the tasks of its owning application in
 * creation order, from 1, where the record establishes it (real_trace.hpp ThreadIdentity::task_ordinal); 0 = not
 * established, and in every container written before (REPLAY_READ format 1).
 */
#define KZ_OCC_BASIS_FIELDS(X, T)                                                    \
  X(T, uint32_t, summary, )      /* SUMMARIES index */                               \
  X(T, uint32_t, ordinal, )                                                          \
  X(T, uint32_t, call, )         /* position of the call in the replayed trace */    \
  X(T, uint32_t, decision, )     /* position of the decision, or KZ_NONE32 */        \
  X(T, uint32_t, basis, )        /* set KZ_OCC_BASIS */                              \
  X(T, uint32_t, assumptions, )  /* set KZ_OCC_ASSUME */                             \
  X(T, uint32_t, first_edge, )   /* OCC_EDGES index */                               \
  X(T, uint32_t, n_edges, )                                                          \
  X(T, uint32_t, owner_key, )    /* or KZ_NONE32 */                                  \
  X(T, uint32_t, obs_reasons, )  /* set KZ_WITHHOLD */                               \
  X(T, uint8_t, status, )        /* KZ_OCC */                                        \
  X(T, uint8_t, status_flags, )  /* set KZ_STATUS_FLAG */                            \
  X(T, uint8_t, observation, )   /* KZ_OBS */                                        \
  X(T, uint8_t, outcome, )       /* KZ_OUTCOME_CHECK */                              \
  X(T, uint8_t, owner_level, )   /* KZ_OWNER_LEVEL */                                \
  X(T, uint8_t, relation, )      /* KZ_EXEC_RELATION */                              \
  X(T, uint16_t, task, )         /* the task's place in its application, 0 = not established (revision 5) */ \
  X(T, kz_str, rspec, )                                                              \
  X(T, kz_str, op, )                                                                 \
  X(T, kz_str, context, )                                                            \
  X(T, kz_str, resource, )                                                           \
  X(T, kz_str, name, )                                                               \
  X(T, kz_str, owner_app, )                                                          \
  X(T, kz_str, via, )

/* One edge a result rests on (see the header).  `occurrence` is the OCC_BASIS index. */
#define KZ_OCC_EDGE_FIELDS(X, T)                                                     \
  X(T, uint32_t, occurrence, )                                                       \
  X(T, uint32_t, from, )         /* position in the replayed trace */                \
  X(T, uint32_t, to, )                                                               \
  X(T, uint8_t, kind, )          /* KZ_OCC_EDGE_KIND */                              \
  X(T, uint8_t, flags, )         /* set KZ_OCC_EDGE_FLAG */                          \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, kz_str, rule, )

/* The reading of the replay (one record).  Every switch is recorded, default or not (KZ_READ_SWITCH). */
#define KZ_REPLAY_READ_FIELDS(X, T)                                                  \
  X(T, uint32_t, format, )        /* 1: OCC_BASIS / OCC_EDGES of revision 4; 2: OCC_BASIS `task` filled (revision 5) */ \
  X(T, uint32_t, control_edges, ) /* control edges of the replayed trace */          \
  X(T, uint32_t, reordered, )     /* events the trace's linearization moved */       \
  X(T, uint8_t, outcome_narrowing, )      /* KZ_READ_SWITCH (P1) */                  \
  X(T, uint8_t, owner_identity_targets, ) /* KZ_READ_SWITCH */                       \
  X(T, uint8_t, reads_from_edges, )       /* KZ_READ_SWITCH */                       \
  X(T, uint8_t, reads_from_outcome, )     /* KZ_READ_SWITCH */                       \
  X(T, uint8_t, control_order, )          /* KZ_READ_SWITCH (P11) */                 \
  X(T, uint8_t, relation_rule, )          /* KZ_RELATION_RULE (P10) */               \
  X(T, uint16_t, rsv0, )

#define KZ_REPLAY_RECORDS(R)              \
  R(kz_occ_basis, KZ_OCC_BASIS_FIELDS)    \
  R(kz_occ_edge, KZ_OCC_EDGE_FIELDS)      \
  R(kz_replay_read, KZ_REPLAY_READ_FIELDS)

KZ_REPLAY_RECORDS(KZ_RECORD)

/* ------------------------------------------------------------------------- */
/* Enumerations.  0 is always the conservative / "not established" value.   */
/* ------------------------------------------------------------------------- */

/* The basis kinds of a result (docs/phi_reference.md section 7.6; P1, P11).  RETURN_DERIVED_ORDER: a reads_from edge
 * whose use only its returned code shows successful (RF); STORE_DERIVED_ORDER: reads_from edges shown by the
 * success-path store of the out value only; CONTROL_ORDER: the realized control of a controlled run. */
#define KZ_ENUM_OCC_BASIS(ITEM, P) /* set */                                          \
  ITEM(P, DIRECT_CAPTURE, 0x1, "direct-capture") ITEM(P, STATE_ORDER, 0x2, "state-order") \
  ITEM(P, RETURN_DERIVED_ORDER, 0x4, "return-derived-order")                         \
  ITEM(P, OUTCOME_NARROWED, 0x8, "outcome-narrowed")                                 \
  ITEM(P, STORE_DERIVED_ORDER, 0x10, "store-derived-order")                          \
  ITEM(P, CONTROL_ORDER, 0x20, "control-order")

/* The declared assumptions a result used.  READS_FROM: a reads_from edge (RF) of either kind. */
#define KZ_ENUM_OCC_ASSUME(ITEM, P) /* set */                                         \
  ITEM(P, VIA_KEY_SEPARATION, 0x1, "via-key-separation")                             \
  ITEM(P, OWNER_IDENTITY_TARGETS, 0x2, "owner-identity-targets")                     \
  ITEM(P, READS_FROM, 0x4, "reads-from-edges") ITEM(P, ROLE, 0x8, "role")

/* How the returned code was checked against the replayed state (kz/core/eval_trace.hpp OutcomeCheck, same order). */
#define KZ_ENUM_OUTCOME_CHECK(ITEM, P)                                                 \
  ITEM(P, NOT_CHECKED, 0, "not-checked") ITEM(P, AGREES, 1, "agrees")                \
  ITEM(P, DISAGREES, 2, "disagrees") ITEM(P, UNMAPPED, 3, "unmapped")                \
  ITEM(P, NARROWED, 4, "narrowed") ITEM(P, AGREES_RETURN_DERIVED, 5, "agrees-return-derived") \
  ITEM(P, INAPPLICABLE, 6, "inapplicable") ITEM(P, CONSISTENT, 7, "consistent")

#define KZ_ENUM_OCC_EDGE_KIND(ITEM, P)                                                 \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, RULE, 1, "rule") ITEM(P, READS_FROM, 2, "reads-from") \
  ITEM(P, CONTROL, 3, "control")

#define KZ_ENUM_OCC_EDGE_FLAGS(ITEM, P) /* set */ ITEM(P, BY_STORE, 0x1, "by-store")

#define KZ_ENUM_READ_SWITCH(ITEM, P) ITEM(P, UNSET, 0, "unset") ITEM(P, OFF, 1, "off") ITEM(P, ON, 2, "on")

/* The rule of the execution relation (P10 decided 2026-10-25; the two earlier readings for sensitivity). */
#define KZ_ENUM_RELATION_RULE(ITEM, P)                                                 \
  ITEM(P, UNSET, 0, "unset") ITEM(P, P10_OCCURRENCE, 1, "P10-occurrence")            \
  ITEM(P, CLEARING_COUNTED, 2, "sensitivity-clearing-writes-counted")                \
  ITEM(P, CLEARING_EXCLUDED, 3, "sensitivity-clearing-writes-excluded")

#define KZ_REPLAY_ENUMS(D)                      \
  D(KZ_OCC_BASIS, KZ_ENUM_OCC_BASIS, 1)         \
  D(KZ_OCC_ASSUME, KZ_ENUM_OCC_ASSUME, 1)       \
  D(KZ_OUTCOME_CHECK, KZ_ENUM_OUTCOME_CHECK, 0) \
  D(KZ_OCC_EDGE_KIND, KZ_ENUM_OCC_EDGE_KIND, 0) \
  D(KZ_OCC_EDGE_FLAG, KZ_ENUM_OCC_EDGE_FLAGS, 1) \
  D(KZ_READ_SWITCH, KZ_ENUM_READ_SWITCH, 0)     \
  D(KZ_RELATION_RULE, KZ_ENUM_RELATION_RULE, 0)

KZ_REPLAY_ENUMS(KZ_DEFINE_ENUM_ENTRY)

#define KZ_REPLAY_FORMATS(F)                          \
  F(kz_occ_basis, summary, IDX, 0)                    \
  F(kz_occ_basis, call, IDX, 0)                       \
  F(kz_occ_basis, decision, IDX, 0)                   \
  F(kz_occ_basis, basis, FLAGS, KZ_OCC_BASIS)         \
  F(kz_occ_basis, assumptions, FLAGS, KZ_OCC_ASSUME)  \
  F(kz_occ_basis, first_edge, IDX, 0)                 \
  F(kz_occ_basis, owner_key, IDX, 0)                  \
  F(kz_occ_basis, obs_reasons, FLAGS, KZ_WITHHOLD)    \
  F(kz_occ_basis, status, ENUM, KZ_OCC)               \
  F(kz_occ_basis, status_flags, FLAGS, KZ_STATUS_FLAG) \
  F(kz_occ_basis, observation, ENUM, KZ_OBS)          \
  F(kz_occ_basis, outcome, ENUM, KZ_OUTCOME_CHECK)    \
  F(kz_occ_basis, owner_level, ENUM, KZ_OWNER_LEVEL)  \
  F(kz_occ_basis, relation, ENUM, KZ_EXEC_RELATION)   \
  F(kz_occ_edge, occurrence, IDX, 0)                  \
  F(kz_occ_edge, from, IDX, 0)                        \
  F(kz_occ_edge, to, IDX, 0)                          \
  F(kz_occ_edge, kind, ENUM, KZ_OCC_EDGE_KIND)        \
  F(kz_occ_edge, flags, FLAGS, KZ_OCC_EDGE_FLAG)      \
  F(kz_replay_read, outcome_narrowing, ENUM, KZ_READ_SWITCH)      \
  F(kz_replay_read, owner_identity_targets, ENUM, KZ_READ_SWITCH) \
  F(kz_replay_read, reads_from_edges, ENUM, KZ_READ_SWITCH)       \
  F(kz_replay_read, reads_from_outcome, ENUM, KZ_READ_SWITCH)     \
  F(kz_replay_read, control_order, ENUM, KZ_READ_SWITCH)          \
  F(kz_replay_read, relation_rule, ENUM, KZ_RELATION_RULE)

#endif /* KZ_SCHEMA_KZ_REPLAY_H */

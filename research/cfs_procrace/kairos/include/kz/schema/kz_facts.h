/*
 * kz_facts.h -- extracted facts, anchors and skeleton records (owner: K4a).
 *
 * Derived from SPEC 4.1 (fact extraction), 4.2 (abstract locations, writers, INIT image,
 * A-TYPE listings) and 4.3 (skeleton: certificate view, once facts, loops), and from the
 * trusted base T1-T3 that the checker reads (SPEC 4.5).  The extraction records (sections
 * 10-21, 31-36) are written by `kz-static extract` and `kz-static basis`; the skeleton
 * records (23-29) are defined here for the skeleton builder and are not written yet.  Only the
 * certificate view of the skeleton exists: the exploration view belongs to the explorer.
 *
 * Revision 2 of the extraction records: sections 10-18 were widened and sections 20, 31-36 were
 * added.  Revision 3 (this file): FACT_LOOPS carries the certificate-view rule of SPEC 4.3 that
 * the extractor can decide (cert, pred_form, lflags, pred_root, cond_root, parent, cert_note;
 * schema version 3); FACT_EXPRS has the operators `forall` and `cursor` and the reference kind
 * `element` (version 3); sections 37 (LOOP_ITERS: the unrolled iterations of table walks),
 * 38 (SPAWN_PARAMS: parameters that carry the function a spawned task runs) and 39 (FUNC_WRITES:
 * the fields each function writes) are new.  Writers no longer claim the transition status
 * COMPUTED: the extractor did steps 2-3 of SPEC 4.2 only, so tstatus is n/a until the census
 * classifies the entry.
 *
 * Source positions.  Every fact carries (file, line, col) as strings and numbers, and `fh`,
 * the index of that file in the FILE_HASHES section of the same artifact.  So a fact is
 * anchored by "file:line, hash of the file" (SPEC 2.5, provenance on every artifact).  A `site`
 * is a number that is unique in the artifact and is what anchors, writers and skeleton nodes
 * refer to; sites are assigned in the order of (file, line, col, kind) after all translation
 * units have been read, so they do not depend on the order in which the units were parsed.
 * `tu` is the FILE_HASHES index of the main source file of the translation unit in which the
 * fact was read first (the same text in a header that several units include is one fact).
 *
 * Indices are positions in a section of this artifact: `fn` in FUNCS, `site` in the sites of
 * FACTS (calls), FACT_LOOPS and the other located records, expression roots in FACT_EXPRS.
 * KZ_NONE32 is "no index".
 *
 * Not usable alone: include kz_schema.h.
 */
#ifndef KZ_SCHEMA_KZ_FACTS_H
#define KZ_SCHEMA_KZ_FACTS_H

#ifndef KZ_SCHEMA_KZ_SCHEMA_H
#error "include kz/schema/kz_schema.h, not this file"
#endif

enum kz_facts_section_id {
  KZ_SEC_FACTS = 10,
  KZ_SEC_FACT_ARGS = 11,
  KZ_SEC_DERIVES = 12,
  KZ_SEC_FACT_EXPRS = 13,
  KZ_SEC_FACT_LOOPS = 14,
  KZ_SEC_FACT_INIT = 15,
  KZ_SEC_FACT_CONSTS = 16,
  KZ_SEC_FACT_ATYPE = 17,
  KZ_SEC_WRITERS = 18,
  KZ_SEC_ALLOCS = 19,
  KZ_SEC_FUNCS = 20,
  KZ_SEC_ANCHORS = 21,
  KZ_SEC_SKEL_CERT = 23,
  KZ_SEC_SKEL_CERT_EDGES = 24,
  KZ_SEC_ONCE = 26,
  KZ_SEC_ACTORS = 28,
  KZ_SEC_CLOSURE_SPAWNS = 29,
  KZ_SEC_CALL_TARGETS = 31,
  KZ_SEC_STATIC_IMAGE = 32,
  KZ_SEC_FACT_LOCS = 33,
  KZ_SEC_TERMINALS = 34,
  KZ_SEC_OUTCALLS = 35,
  KZ_SEC_TUS = 36,
  KZ_SEC_LOOP_ITERS = 37,
  KZ_SEC_SPAWN_PARAMS = 38,
  KZ_SEC_FUNC_WRITES = 39
};

#define KZ_FACTS_SECTIONS(S)                   \
  S(FACTS, 10, kz_fact_call, 2)                \
  S(FACT_ARGS, 11, kz_fact_arg, 2)             \
  S(DERIVES, 12, kz_fact_derives, 2)           \
  S(FACT_EXPRS, 13, kz_fact_expr, 3)           \
  S(FACT_LOOPS, 14, kz_fact_loop, 3)           \
  S(FACT_INIT, 15, kz_fact_init, 2)            \
  S(FACT_CONSTS, 16, kz_fact_const, 2)         \
  S(FACT_ATYPE, 17, kz_fact_atype, 2)          \
  S(WRITERS, 18, kz_fact_writer, 2)            \
  S(ALLOCS, 19, kz_fact_alloc, 1)              \
  S(FUNCS, 20, kz_fact_func, 1)                \
  S(ANCHORS, 21, kz_anchor, 1)                 \
  S(SKEL_CERT, 23, kz_skel_node, 1)            \
  S(SKEL_CERT_EDGES, 24, kz_skel_edge, 1)      \
  S(ONCE, 26, kz_skel_once, 1)                 \
  S(ACTORS, 28, kz_actor, 1)                   \
  S(CLOSURE_SPAWNS, 29, kz_closure_spawn, 1)   \
  S(CALL_TARGETS, 31, kz_fact_target, 1)       \
  S(STATIC_IMAGE, 32, kz_fact_image, 1)        \
  S(FACT_LOCS, 33, kz_fact_loc, 1)             \
  S(TERMINALS, 34, kz_fact_terminal, 1)        \
  S(OUTCALLS, 35, kz_fact_outcall, 1)          \
  S(TUS, 36, kz_tu, 1)                         \
  S(LOOP_ITERS, 37, kz_fact_iter, 1)           \
  S(SPAWN_PARAMS, 38, kz_fact_spawn_param, 1)  \
  S(FUNC_WRITES, 39, kz_fact_fwrite, 1)

/* A function: defined in the closed program, or only declared (library, external).  */
#define KZ_FACT_FUNC_FIELDS(X, T)                                                    \
  X(T, uint32_t, fn, ) /* its own index */                                           \
  X(T, uint32_t, tu, ) /* FILE_HASHES index of the unit that defined it, or KZ_NONE32 */ \
  X(T, kz_str, name, )                                                               \
  X(T, kz_str, file, ) /* the definition, or the first declaration */                \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, end_line, )                                                         \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, flags, ) /* KZ_FUNC_FLAG */                                         \
  X(T, uint32_t, nparams, )

/* A call site (SPEC 4.1 step 3).  Arguments are in FACT_ARGS [first_arg, first_arg+nargs). */
#define KZ_FACT_CALL_FIELDS(X, T)                                                    \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, tu, ) /* FILE_HASHES index of the translation unit, or KZ_NONE32 */  \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, caller, ) /* enclosing function */                                    \
  X(T, kz_str, callee, ) /* callee name; empty for an indirect call */               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, first_arg, )                                                        \
  X(T, uint32_t, nargs, )                                                            \
  X(T, uint32_t, flags, ) /* macro expansion, intra-TU, result use, loop context */  \
  X(T, uint16_t, loop_depth, )                                                       \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, caller_fn, ) /* FUNCS index */                                      \
  X(T, uint32_t, callee_fn, ) /* FUNCS index of the direct callee, or KZ_NONE32 */   \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, loop, ) /* FACT_LOOPS index of the innermost loop, or KZ_NONE32 */   \
  X(T, uint8_t, kind, ) /* KZ_CALL_KIND */                                           \
  X(T, uint8_t, rsv1, )                                                              \
  X(T, uint16_t, rsv2, )

/*
 * An argument of a call site: constant, symbolic (table field, global) or top.  A symbolic
 * argument that reads a struct field names it in (loc_type, loc_field); an argument that is
 * the address of a field has kind ADDR_FIELD with the same pair; the address of a function has
 * kind FUNC and `fn`.
 */
#define KZ_FACT_ARG_FIELDS(X, T)                                                     \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, index, )                                                            \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, role, ) /* stamp / key / id role of the parameter */                 \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, rsv1, )                                                             \
  X(T, int64_t, value, )                                                             \
  X(T, kz_str, symbol, )                                                             \
  X(T, kz_str, loc_type, )                                                           \
  X(T, kz_str, loc_field, )                                                          \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, rsv2, )

/*
 * derives(arg, loc, site) (SPEC 1.4, T-DERIVE): the argument is a copy chain, without
 * arithmetic, from one shared location read at the capture point (file, line, col).  A row is
 * written for every argument whose parameter has a stamp, key or id role; `status` says whether
 * the chain was found, and `reason` why not.
 */
#define KZ_FACT_DERIVES_FIELDS(X, T)                                                 \
  X(T, uint32_t, site, ) /* the call site */                                         \
  X(T, uint32_t, arg_index, )                                                        \
  X(T, kz_str, loc_type, )                                                           \
  X(T, kz_str, loc_field, )                                                          \
  X(T, uint32_t, chain_root, ) /* FACT_EXPRS root of the serialized copy chain */    \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, kz_str, file, ) /* capture point */                                           \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, hops, ) /* number of copies between the load and the argument */    \
  X(T, kz_str, callee, )                                                             \
  X(T, uint8_t, status, ) /* KZ_DERIVES_STATUS */                                    \
  X(T, uint8_t, reason, ) /* KZ_DERIVES_REASON */                                    \
  X(T, uint16_t, rsv1, )

/*
 * A node of a serialized expression tree (loop-exit conditions, mod sets, guards, copy
 * chains).  Binary nodes use a and b; AND/OR/SEQ/LOCSET are built as chains: for SEQ and
 * LOCSET `a` is the element and `b` the rest of the list (KZ_NONE32 ends it).  `flags` is
 * the kind of reference of a LOAD (KZ_EXPR_REF); `value` is a constant, a parameter index, or
 * the trip count of a stride.
 */
#define KZ_FACT_EXPR_FIELDS(X, T)                                                    \
  X(T, uint16_t, op, )                                                               \
  X(T, uint16_t, flags, )                                                            \
  X(T, uint32_t, a, ) /* child node index or KZ_NONE32 */                            \
  X(T, uint32_t, b, )                                                                \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, int64_t, value, )                                                             \
  X(T, kz_str, name, )

/*
 * A loop of the closed program (SPEC 4.1 step 8, 4.3).  Every loop gets a record with its
 * kind and, when the trip count can be computed from build constants, the count (`trip`,
 * `trip_status`); the expression trees (exit conditions, body mod set, budget) are written for
 * the loops that contain a call to a delay operation and for table walks.
 *
 * `cert` is the rule of SPEC 4.3 for the certificate view, as far as the extractor can decide
 * it: UNROLL (the trip count is exact: a constant-trip loop, a resolved table walk, a
 * count-bounded retry loop), AWAIT (a poll loop whose condition exit has a recognised predicate
 * form and for which both preconditions of lemma L1 hold at the level of the extracted facts:
 * `lflags` L1-i and L1-ii), STAR (anything else).  The lemma itself is not decided here; its
 * proof status belongs to the certificate.  `cond_root` is the condition exits (the exits that
 * do not test the budget), `pred_root` the predicate extracted from them (forms of SPEC 4.3:
 * a direct test, or a counter-zero test after a constant-trip loop, rewritten as `forall`).
 * `cert_note` says why, in words.
 */
#define KZ_FACT_LOOP_FIELDS(X, T)                                                    \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, line, )                                                             \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, func, )                                                               \
  X(T, uint32_t, delay_site, ) /* the delay-operation call inside the loop */        \
  X(T, uint32_t, exit_root, )  /* exit conditions: FACT_EXPRS root */                \
  X(T, uint32_t, modset_root, )                                                      \
  X(T, uint32_t, budget_root, )                                                      \
  X(T, uint32_t, lock_site, )  /* the lock acquisition that brackets the test, or KZ_NONE32 */ \
  X(T, uint8_t, lock_kind, )                                                         \
  X(T, uint8_t, cert, )      /* KZ_LOOP_CERT */                                      \
  X(T, uint8_t, pred_form, ) /* KZ_PRED_FORM */                                      \
  X(T, uint8_t, lflags, )    /* KZ_LOOP_FLAG */                                      \
  X(T, uint32_t, loop, ) /* its own index */                                         \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint16_t, depth, ) /* nesting depth, 1 = outermost */                         \
  X(T, uint8_t, kind, )   /* KZ_LOOP_KIND */                                          \
  X(T, uint8_t, trip_status, ) /* KZ_TRIP_STATUS */                                  \
  X(T, uint32_t, n_exits, )                                                          \
  X(T, uint32_t, n_calls, )                                                          \
  X(T, uint32_t, parent, ) /* FACT_LOOPS index of the enclosing loop, or KZ_NONE32 */ \
  X(T, int64_t, trip, )                                                              \
  X(T, kz_str, table, ) /* TABLE_WALK: the table symbol */                          \
  X(T, kz_str, trip_note, ) /* why the trip count is unknown, when a walk could not be resolved */ \
  X(T, uint32_t, pred_root, ) /* FACT_EXPRS: the extracted predicate, or KZ_NONE32 */ \
  X(T, uint32_t, cond_root, ) /* FACT_EXPRS: the condition exits, or KZ_NONE32 */     \
  X(T, kz_str, cert_note, )

/* One location of the INIT image (SPEC 4.2 step 9): the program-entry state of a field. */
#define KZ_FACT_INIT_FIELDS(X, T)                                                    \
  X(T, kz_str, loc_type, )                                                           \
  X(T, kz_str, loc_field, )                                                          \
  X(T, int64_t, value, )                                                             \
  X(T, uint8_t, source, )                                                            \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, prov, )                                                             \
  X(T, kz_str, object, ) /* the global object that holds the field */                \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, rsv2, )

/* A constant-evaluated configuration value, evaluated in the translation unit of its use. */
#define KZ_FACT_CONST_FIELDS(X, T)                                                   \
  X(T, kz_str, name, )                                                               \
  X(T, kz_str, file, )                                                               \
  X(T, int64_t, value, )                                                             \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, tu, )                                                               \
  X(T, uint32_t, prov, )                                                             \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, kz_str, text, ) /* the value of a string constant */                          \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint8_t, kind, ) /* KZ_CONST_KIND */                                          \
  X(T, uint8_t, rsv1, )                                                              \
  X(T, uint16_t, rsv2, )

/* An A-TYPE listing entry: an untyped write, an address escape, a cast, a union (SPEC 4.2 step 3). */
#define KZ_FACT_ATYPE_FIELDS(X, T)                                                   \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, site, )                                                             \
  X(T, kz_str, loc_type, )                                                           \
  X(T, kz_str, loc_field, )                                                          \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, fn, )                                                               \
  X(T, kz_str, detail, )

/*
 * A writer of an abstract location (SPEC 4.2): `W`, `W_must` and `D` classification with
 * the transition and key statuses.  The abstract location is (struct type, field path).  For an
 * out-pointer store instantiated at a call site, (file, line, col, func) is the call site and
 * (via_file, via_line, via_fn) is the store inside the callee.
 */
#define KZ_FACT_WRITER_FIELDS(X, T)                                                  \
  X(T, kz_str, loc_type, )                                                           \
  X(T, kz_str, loc_field, )                                                          \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, func, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, site, )                                                             \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, value_kind, )                                                        \
  X(T, uint8_t, tstatus, ) /* KZ_TSTATUS */                                          \
  X(T, uint8_t, kstatus, ) /* KZ_KSTATUS */                                          \
  X(T, uint8_t, wd, )      /* set: W, W_MUST, D */                                   \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, guard_root, ) /* dominating guard: FACT_EXPRS root, or KZ_NONE32 */  \
  X(T, kz_str, key, )          /* key expression */                                  \
  X(T, int64_t, value, )                                                             \
  X(T, kz_str, value_text, )   /* the stored value as written */                     \
  X(T, kz_str, via_file, )                                                           \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, via_line, )                                                         \
  X(T, uint32_t, via_fn, )                                                           \
  X(T, uint32_t, flags, ) /* KZ_WRITER_FLAG */                                       \
  X(T, uint32_t, rsv2, )

/* An allocation record: a reservation node and the record it allocates (SPEC 4.2 step 8). */
#define KZ_FACT_ALLOC_FIELDS(X, T)                                                   \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, node, ) /* certificate-view node of the reservation */              \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, func, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, kz_str, record_type, )                                                        \
  X(T, kz_str, tag_field, ) /* per-record field set in the reservation's critical section */ \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, int64_t, tag_value, )

/* Binding of a fact site to the gcc binary that runs (SPEC 3.17, anchor audit). */
#define KZ_ANCHOR_FIELDS(X, T)                                                       \
  X(T, uint32_t, site, )                                                             \
  X(T, uint8_t, audit, )                                                             \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, insn_class, )                                                       \
  X(T, uint64_t, file_offset, )                                                      \
  X(T, uint8_t, build_id, [20])                                                      \
  X(T, kz_str, function, )                                                           \
  X(T, kz_str, binary, )                                                             \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, rsv1, )

/* A static actor (SPEC 4.3): one per generation copy. */
#define KZ_ACTOR_FIELDS(X, T)                                                        \
  X(T, uint32_t, actor, )                                                            \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, once, ) /* once(actor) as decided on the certificate view */         \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, entry_node, )                                                       \
  X(T, uint32_t, generation, )                                                       \
  X(T, kz_str, name, )

/* The closure's spawn-site list (trusted input T3): who creates which actor, where. */
#define KZ_CLOSURE_SPAWN_FIELDS(X, T)                                                \
  X(T, uint32_t, actor, ) /* the created actor */                                    \
  X(T, uint32_t, spawn_site, )                                                       \
  X(T, uint32_t, creator, ) /* the creating actor, or KZ_NONE32 */                   \
  X(T, uint8_t, resolution, )                                                        \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )

/* A node of the certificate view (SPEC 4.3).  The node index is its position. */
#define KZ_SKEL_NODE_FIELDS(X, T)                                                    \
  X(T, uint32_t, actor, )                                                            \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, view, )                                                              \
  X(T, uint16_t, flags, )                                                            \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, context, ) /* call-string context id */                             \
  X(T, uint32_t, profile_entry, )                                                    \
  X(T, uint16_t, conjunct, )                                                         \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, region, ) /* star-closed region id or KZ_NONE32 */

#define KZ_SKEL_EDGE_FIELDS(X, T)                                                    \
  X(T, uint32_t, from, )                                                             \
  X(T, uint32_t, to, )                                                               \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, label, ) /* outcome or choice label id, or KZ_NONE32 */

/* once(actor) / once(node) with the reference to what it derives from (SPEC 4.3). */
#define KZ_SKEL_ONCE_FIELDS(X, T)                                                    \
  X(T, uint8_t, subject_kind, )                                                      \
  X(T, uint8_t, rule, )                                                              \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, subject, )                                                          \
  X(T, kz_ref, derivation, )

/*
 * The functions an indirect call or a spawn call may reach.  One row per (site, candidate);
 * a site that could not be resolved has one row with fn = KZ_NONE32.  `via` names the path
 * that carried the function value (a field, a variable, a parameter), `n` the number of
 * candidates of the site.
 */
#define KZ_FACT_TARGET_FIELDS(X, T)                                                  \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint8_t, kind, ) /* KZ_CALL_TARGET_KIND */                                         \
  X(T, uint8_t, reason, ) /* KZ_CALL_TARGET_REASON, for an unresolved site */             \
  X(T, uint16_t, rsv0, )                                                             \
  X(T, uint32_t, n, )                                                                \
  X(T, kz_str, via, )

/*
 * One element of the evaluated static-initializer image of a global object (a table, a
 * configuration record).  `path` is "[i]" for an array element and ".name" for a field,
 * nested ("[3].name").  An address or function value names its target in `text`.
 */
#define KZ_FACT_IMAGE_FIELDS(X, T)                                                   \
  X(T, kz_str, object, )                                                             \
  X(T, kz_str, path, )                                                               \
  X(T, int64_t, value, )                                                             \
  X(T, kz_str, text, )                                                               \
  X(T, uint32_t, fn, ) /* FUNCS index when kind = FUNC */                            \
  X(T, uint8_t, kind, ) /* KZ_IMAGE_KIND */                                          \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, fh, )

/* Summary of one extraction target (the O7 report row): counts and the writer-set status. */
#define KZ_FACT_LOC_FIELDS(X, T)                                                     \
  X(T, kz_str, loc_type, )                                                           \
  X(T, kz_str, loc_field, )                                                          \
  X(T, uint32_t, n_writers, )                                                        \
  X(T, uint32_t, n_direct, )   /* assignments, increments, aggregate assignments */   \
  X(T, uint32_t, n_data_fn, )  /* typed memset / memcpy family */                    \
  X(T, uint32_t, n_outptr, )   /* out-pointer stores instantiated at call sites */    \
  X(T, uint32_t, n_untyped, )  /* A-TYPE: untyped writes */                          \
  X(T, uint32_t, n_escape, )   /* A-TYPE: address escapes */                         \
  X(T, uint32_t, n_cast, )     /* A-TYPE: casts to or from an untyped view */        \
  X(T, uint32_t, n_union, )    /* A-TYPE: union members */                           \
  X(T, uint32_t, n_init, )     /* objects in the INIT image */                       \
  X(T, uint32_t, n_unresolved_indirect, ) /* whole program, reported per location */  \
  X(T, uint8_t, status, )      /* KZ_LOC_STATUS */                                   \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )

/* A call to a terminal operation with the conditions it executes under. */
#define KZ_FACT_TERMINAL_FIELDS(X, T)                                                \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, guard_root, ) /* conjunction of the enclosing conditions, or KZ_NONE32 */ \
  X(T, uint32_t, depth, )                                                            \
  X(T, kz_str, op, )                                                                 \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, flags, ) /* KZ_TERMINAL_FLAG */

/* A call that passes the address of an extraction target (out-pointer effect, SPEC 4.2 step 2). */
#define KZ_FACT_OUTCALL_FIELDS(X, T)                                                 \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, arg_index, )                                                        \
  X(T, uint32_t, callee_fn, )                                                        \
  X(T, uint32_t, n_stores, )                                                         \
  X(T, kz_str, loc_type, )                                                           \
  X(T, kz_str, loc_field, )                                                          \
  X(T, kz_str, callee, )                                                             \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint8_t, summary, ) /* KZ_OUTPTR_SUMMARY */                                   \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )

/*
 * A translation unit of the closed program (basis container, SPEC 4.1 step 1).  The command is
 * kept as the compiler, the working directory and the flags (the recorded command without
 * output and input; arguments with blanks or quotes are single-quoted), so the unit can be
 * parsed again exactly as recorded.  command_sha256 is the SHA-256 of the compiler, the flag
 * arguments and the source path, each followed by a zero byte.
 */
#define KZ_TU_FIELDS(X, T)                                                           \
  X(T, uint32_t, tu, )                                                               \
  X(T, uint32_t, fh, ) /* FILE_HASHES index of the main source file */               \
  X(T, kz_str, source, )                                                             \
  X(T, kz_str, object, )                                                             \
  X(T, kz_str, flags, )                                                              \
  X(T, kz_str, member, ) /* how the link command takes the object in */              \
  X(T, kz_str, directory, )                                                          \
  X(T, kz_str, compiler, )                                                           \
  X(T, uint8_t, command_sha256, [32])

/*
 * One unrolled iteration of a resolved table walk (SPEC 4.3: constant-trip loops over B
 * constants are unrolled), evaluated against the static-initializer image of the element.
 *   ELEMENT  one row per iteration: the element object (`text`) and where its initializer is.
 *   CALL     one row per (iteration, call in the loop body): whether the call is reached in this
 *            iteration.  The guards of the call inside the loop are partially evaluated with
 *            the element's field values: TAKEN (they hold), SKIPPED (one is false), RESIDUAL
 *            (what is left does not depend on the element: `guard_root`).
 *   ARG      an argument of such a call that reads a field of the element, with its value in
 *            this iteration (`value_kind`, `value`, `text`, `fn`).  `spawn` marks the argument
 *            that is the entry of a spawn operation, or the task-main parameter of a function
 *            that spawns (SPAWN_PARAMS).
 *   CALLEE   an indirect call through a field of the element: the function it calls.
 * (file, line, fh) locate the value: the initializer of the element object.
 */
#define KZ_FACT_ITER_FIELDS(X, T)                                                    \
  X(T, uint32_t, loop, ) /* FACT_LOOPS index */                                     \
  X(T, uint32_t, iter, ) /* 0-based */                                               \
  X(T, uint32_t, site, ) /* the call, or KZ_NONE32 for an ELEMENT row */             \
  X(T, uint32_t, arg_index, ) /* ARG rows, else KZ_NONE32 */                         \
  X(T, uint8_t, kind, )       /* KZ_ITER_KIND */                                     \
  X(T, uint8_t, status, )     /* KZ_ITER_STATUS */                                   \
  X(T, uint8_t, value_kind, ) /* KZ_IMAGE_KIND */                                    \
  X(T, uint8_t, spawn, )      /* KZ_ITER_SPAWN */                                    \
  X(T, uint32_t, guard_root, ) /* FACT_EXPRS: the residual guard, or KZ_NONE32 */    \
  X(T, uint32_t, fn, )         /* FUNCS index of a function value, or KZ_NONE32 */   \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, int64_t, value, )                                                             \
  X(T, kz_str, text, )                                                               \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, fh, )

/*
 * A parameter of a function that carries the function a task it spawns will run.  DIRECT: the
 * parameter flows to the entry argument of a spawn call inside the function.  VIA_ENTRY: it
 * flows (through the function-pointer flow graph, field-based) to the callee of an indirect call
 * in a function reachable from the entry of such a spawn call; the spawned task may call it.
 * This is a may-flow fact: which value one particular task calls is not decided here.
 */
#define KZ_FACT_SPAWN_PARAM_FIELDS(X, T)                                             \
  X(T, uint32_t, fn, )         /* FUNCS index */                                     \
  X(T, uint32_t, param, )                                                            \
  X(T, uint32_t, spawn_site, ) /* the spawn call inside fn */                        \
  X(T, uint32_t, entry_fn, )   /* VIA_ENTRY: the spawned entry, else KZ_NONE32 */    \
  X(T, uint32_t, call_site, )  /* VIA_ENTRY: the indirect call, else KZ_NONE32 */    \
  X(T, uint8_t, kind, )        /* KZ_SPAWN_PARAM_KIND */                             \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, kz_str, via, )

/*
 * A field a function writes in its own body (not through its callees): an assignment or an
 * increment of the field (FIELD), a whole-record write (RECORD, field "*": an aggregate
 * assignment or a typed data function; one row per record type it contains by value), the
 * address of the field passed to a call (OUTARG), or a write through an untyped pointer
 * (UNTYPED, no type).  (file, line) is the first such write in the function.  The transitive
 * write set of a loop body is the union over the calls it makes (FACTS, CALL_TARGETS).
 */
#define KZ_FACT_FWRITE_FIELDS(X, T)                                                  \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint8_t, kind, ) /* KZ_FWRITE_KIND */                                         \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, kz_str, type, )                                                               \
  X(T, kz_str, field, )                                                              \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, fh, )

#define KZ_FACTS_RECORDS(R)                       \
  R(kz_fact_func, KZ_FACT_FUNC_FIELDS)            \
  R(kz_fact_call, KZ_FACT_CALL_FIELDS)            \
  R(kz_fact_arg, KZ_FACT_ARG_FIELDS)              \
  R(kz_fact_derives, KZ_FACT_DERIVES_FIELDS)      \
  R(kz_fact_expr, KZ_FACT_EXPR_FIELDS)            \
  R(kz_fact_loop, KZ_FACT_LOOP_FIELDS)            \
  R(kz_fact_init, KZ_FACT_INIT_FIELDS)            \
  R(kz_fact_const, KZ_FACT_CONST_FIELDS)          \
  R(kz_fact_atype, KZ_FACT_ATYPE_FIELDS)          \
  R(kz_fact_writer, KZ_FACT_WRITER_FIELDS)        \
  R(kz_fact_alloc, KZ_FACT_ALLOC_FIELDS)          \
  R(kz_anchor, KZ_ANCHOR_FIELDS)                  \
  R(kz_actor, KZ_ACTOR_FIELDS)                    \
  R(kz_closure_spawn, KZ_CLOSURE_SPAWN_FIELDS)    \
  R(kz_skel_node, KZ_SKEL_NODE_FIELDS)            \
  R(kz_skel_edge, KZ_SKEL_EDGE_FIELDS)            \
  R(kz_skel_once, KZ_SKEL_ONCE_FIELDS)            \
  R(kz_fact_target, KZ_FACT_TARGET_FIELDS)        \
  R(kz_fact_image, KZ_FACT_IMAGE_FIELDS)          \
  R(kz_fact_loc, KZ_FACT_LOC_FIELDS)              \
  R(kz_fact_terminal, KZ_FACT_TERMINAL_FIELDS)    \
  R(kz_fact_outcall, KZ_FACT_OUTCALL_FIELDS)      \
  R(kz_tu, KZ_TU_FIELDS)                          \
  R(kz_fact_iter, KZ_FACT_ITER_FIELDS)            \
  R(kz_fact_spawn_param, KZ_FACT_SPAWN_PARAM_FIELDS) \
  R(kz_fact_fwrite, KZ_FACT_FWRITE_FIELDS)

KZ_FACTS_RECORDS(KZ_RECORD)

/* Enumerations ------------------------------------------------------------- */

#define KZ_ENUM_ARG_KIND(ITEM, P)                                                    \
  ITEM(P, NONE, 0, "none") ITEM(P, CONST, 1, "const") ITEM(P, SYMBOLIC, 2, "symbolic") \
  ITEM(P, TOP, 3, "top") ITEM(P, FUNC, 4, "function") ITEM(P, STRING, 5, "string")   \
  ITEM(P, ADDR_FIELD, 6, "addr-of-field") ITEM(P, ADDR_OBJECT, 7, "addr-of-object")

#define KZ_ENUM_ARG_ROLE(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, STAMP, 1, "stamp") ITEM(P, KEY, 2, "key") ITEM(P, ID, 3, "id")

#define KZ_ENUM_CALL_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, IN_MACRO, 0x1, "in-macro") ITEM(P, INTRA_TU, 0x2, "intra-tu")              \
  ITEM(P, RESULT_USED, 0x4, "result-used") ITEM(P, RESULT_TESTED, 0x8, "result-tested") \
  ITEM(P, IN_LOOP, 0x10, "in-loop") ITEM(P, TERMINAL_OP, 0x20, "terminal-op")        \
  ITEM(P, SPAWN_OP, 0x40, "spawn-op") ITEM(P, DELAY_OP, 0x80, "delay-op")            \
  ITEM(P, DATA_FN, 0x100, "data-fn") ITEM(P, LOCK_OP, 0x200, "lock-op")              \
  ITEM(P, UNLOCK_OP, 0x400, "unlock-op")

#define KZ_ENUM_CALL_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, DIRECT, 1, "direct") ITEM(P, INDIRECT, 2, "indirect") \
  ITEM(P, BUILTIN, 3, "builtin")

#define KZ_ENUM_FUNC_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, DEFINED, 0x1, "defined") ITEM(P, STATIC, 0x2, "static")                    \
  ITEM(P, ADDRESS_TAKEN, 0x4, "address-taken") ITEM(P, VARIADIC, 0x8, "variadic")    \
  ITEM(P, BUILTIN, 0x10, "builtin") ITEM(P, INLINE, 0x20, "inline")

#define KZ_ENUM_DERIVES_STATUS(ITEM, P)                                              \
  ITEM(P, NOT_DERIVED, 0, "not-derived") ITEM(P, DERIVED, 1, "derived") ITEM(P, CONSTANT, 2, "constant")

#define KZ_ENUM_DERIVES_REASON(ITEM, P)                                              \
  ITEM(P, NONE, 0, "none") ITEM(P, MULTI_DEF, 1, "multiple-definitions")             \
  ITEM(P, NOT_COPY, 2, "not-a-copy") ITEM(P, ADDRESS_TAKEN, 3, "address-taken")      \
  ITEM(P, FROM_PARAM, 4, "from-parameter") ITEM(P, FROM_CALL, 5, "from-call-result") \
  ITEM(P, MULTI_LOC, 6, "several-locations") ITEM(P, NO_DEF, 7, "no-definition")      \
  ITEM(P, NOT_SHARED, 8, "not-a-shared-location")

#define KZ_ENUM_EXPR_OP(ITEM, P)                                                     \
  ITEM(P, NONE, 0, "none") ITEM(P, CONST, 1, "const") ITEM(P, LOAD, 2, "load")       \
  ITEM(P, AND, 3, "and") ITEM(P, OR, 4, "or") ITEM(P, NOT, 5, "not") ITEM(P, EQ, 6, "eq") \
  ITEM(P, NE, 7, "ne") ITEM(P, LT, 8, "lt") ITEM(P, LE, 9, "le") ITEM(P, ADD, 10, "add") \
  ITEM(P, SUB, 11, "sub") ITEM(P, CALL_RESULT, 12, "call-result")                    \
  ITEM(P, COPY, 13, "copy") ITEM(P, SEQ, 14, "seq") ITEM(P, LOCSET, 15, "locset")    \
  ITEM(P, GT, 16, "gt") ITEM(P, GE, 17, "ge") ITEM(P, MUL, 18, "mul") ITEM(P, DIV, 19, "div") \
  ITEM(P, MOD, 20, "mod") ITEM(P, BITAND, 21, "bitand") ITEM(P, BITOR, 22, "bitor")  \
  ITEM(P, BITXOR, 23, "bitxor") ITEM(P, SHL, 24, "shl") ITEM(P, SHR, 25, "shr")      \
  ITEM(P, NEG, 26, "neg") ITEM(P, BITNOT, 27, "bitnot") ITEM(P, COND, 28, "cond")    \
  ITEM(P, TOP, 29, "top") ITEM(P, ADDR, 30, "addr") ITEM(P, INDEX, 31, "index")      \
  ITEM(P, DEREF, 32, "deref") ITEM(P, STORE, 33, "store")                            \
  ITEM(P, FORALL, 34, "forall") ITEM(P, CURSOR, 35, "cursor")

/*
 * `forall`: name = the induction variable, value = the trip count N, a = the body (a condition
 * that holds for every i in [0, N)), b = a list (SEQ) of `cursor` nodes.  `cursor`: name = a
 * local pointer that starts at `a` and advances by one element per iteration, value = the
 * number of elements of `a` when it is an array of known size (else 0).
 */

/* What a LOAD node reads (kz_fact_expr.flags).  ELEMENT: a field (name) of the current element
 * of a table walk; an empty name is the element pointer itself. */
#define KZ_ENUM_EXPR_REF(ITEM, P)                                                    \
  ITEM(P, NONE, 0, "none") ITEM(P, FIELD, 1, "field") ITEM(P, GLOBAL, 2, "global")   \
  ITEM(P, LOCAL, 3, "local") ITEM(P, PARAM, 4, "param") ITEM(P, STRING, 5, "string") \
  ITEM(P, FUNC, 6, "function") ITEM(P, ELEM, 7, "element")

#define KZ_ENUM_LOCK_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, ONE_REGION, 1, "one-lock-region") ITEM(P, ATOMIC_READ, 2, "atomic-read")

#define KZ_ENUM_LOOP_KIND(ITEM, P)                                                   \
  ITEM(P, OTHER, 0, "other") ITEM(P, CONST_TRIP, 1, "constant-trip")                 \
  ITEM(P, COUNT_BOUNDED, 2, "count-bounded") ITEM(P, POLL, 3, "poll")                \
  ITEM(P, TABLE_WALK, 4, "table-walk") ITEM(P, MESSAGE, 5, "message-loop")

#define KZ_ENUM_LOOP_CERT(ITEM, P)                                                   \
  ITEM(P, UNDECIDED, 0, "undecided") ITEM(P, UNROLL, 1, "unroll")                    \
  ITEM(P, AWAIT, 2, "await") ITEM(P, STAR, 3, "star-closed")

#define KZ_ENUM_PRED_FORM(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, DIRECT, 1, "direct-test")                         \
  ITEM(P, COUNTER_ZERO, 2, "counter-zero") ITEM(P, COMBINED, 3, "combination")

/* L1-i: the body writes none of the relevant locations (the fields the predicate reads and the
 * extraction targets), through its callees included; L1-ii: the test reads happen in one
 * critical section of one lock, or are one atomic read; MOD_INCOMPLETE: the transitive write set
 * could not be closed (an unresolved indirect call or an untyped write is reachable). */
#define KZ_ENUM_LOOP_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, DELAY_DIRECT, 0x1, "delay-in-body") ITEM(P, L1_MOD, 0x2, "L1-i")           \
  ITEM(P, L1_READ, 0x4, "L1-ii") ITEM(P, MOD_INCOMPLETE, 0x8, "mod-incomplete")

#define KZ_ENUM_ITER_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, ELEMENT, 1, "element") ITEM(P, CALL, 2, "call")   \
  ITEM(P, ARG, 3, "argument") ITEM(P, CALLEE, 4, "callee")

#define KZ_ENUM_ITER_STATUS(ITEM, P)                                                 \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, TAKEN, 1, "taken") ITEM(P, SKIPPED, 2, "skipped") \
  ITEM(P, RESIDUAL, 3, "residual")

#define KZ_ENUM_ITER_SPAWN(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, ENTRY, 1, "spawn-entry") ITEM(P, TASK_MAIN, 2, "task-main")

#define KZ_ENUM_SPAWN_PARAM_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, DIRECT, 1, "entry-argument") ITEM(P, VIA_ENTRY, 2, "called-by-entry")

#define KZ_ENUM_FWRITE_KIND(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, FIELD, 1, "field") ITEM(P, RECORD, 2, "record")   \
  ITEM(P, OUTARG, 3, "address-argument") ITEM(P, UNTYPED, 4, "untyped")

#define KZ_ENUM_TRIP_STATUS(ITEM, P)                                                 \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, EXACT, 1, "exact") ITEM(P, UPPER_BOUND, 2, "upper-bound") \
  ITEM(P, NONCONSTANT, 3, "non-constant-bound")

#define KZ_ENUM_INIT_SOURCE(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, STATIC_INIT, 1, "static-initializer") ITEM(P, ZERO, 2, "zero-initialized") \
  ITEM(P, DECODED_S0, 3, "decoded-S0") ITEM(P, NONCONST, 4, "static-initializer-non-constant")

#define KZ_ENUM_CONST_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, INT, 1, "int") ITEM(P, STRING, 2, "string")

#define KZ_ENUM_ATYPE_KIND(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, UNTYPED_WRITE, 1, "untyped-write")                \
  ITEM(P, ADDRESS_ESCAPE, 2, "address-escape") ITEM(P, CAST, 3, "cast")              \
  ITEM(P, UNION, 4, "union")

#define KZ_ENUM_WRITER_KIND(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, ASSIGN, 1, "assign") ITEM(P, MEMSET, 2, "memset") \
  ITEM(P, MEMCPY, 3, "memcpy") ITEM(P, OUTPTR, 4, "out-pointer") ITEM(P, EXTERNAL, 5, "external") \
  ITEM(P, UNKNOWN_EFFECT, 6, "unknown_effect") ITEM(P, INCDEC, 7, "inc-dec")         \
  ITEM(P, AGGREGATE, 8, "aggregate-assign")

#define KZ_ENUM_WD(ITEM, P) /* set */ \
  ITEM(P, W, 0x1, "W") ITEM(P, W_MUST, 0x2, "W_must") ITEM(P, D, 0x4, "D")

#define KZ_ENUM_WRITER_FLAGS(ITEM, P) /* set */                                      \
  ITEM(P, EARLY_INIT, 0x1, "early-init") ITEM(P, INIT_TREE, 0x2, "init-tree")        \
  ITEM(P, BASE_LOCAL, 0x4, "base-local") ITEM(P, IN_LOOP, 0x8, "in-loop")            \
  ITEM(P, COVERS_MORE, 0x10, "covers-more-than-the-field")                           \
  ITEM(P, IN_MACRO, 0x20, "in-macro")

#define KZ_ENUM_CALL_TARGET_KIND(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, INDIRECT, 1, "indirect-call")                     \
  ITEM(P, SPAWN_ENTRY, 2, "spawn-entry")

#define KZ_ENUM_CALL_TARGET_REASON(ITEM, P)                                               \
  ITEM(P, RESOLVED, 0, "resolved") ITEM(P, NO_FLOW, 1, "no-function-flows-to-it")   \
  ITEM(P, UNKNOWN_SOURCE, 2, "unknown-source") ITEM(P, CAST_FROM_DATA, 3, "cast-from-data")

#define KZ_ENUM_IMAGE_KIND(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, INT, 1, "int") ITEM(P, STRING, 2, "string")       \
  ITEM(P, ADDR, 3, "address") ITEM(P, FUNC, 4, "function") ITEM(P, NULLPTR, 5, "null") \
  ITEM(P, NONCONST, 6, "non-constant") ITEM(P, ZERO_FILL, 7, "zero-fill")

#define KZ_ENUM_LOC_STATUS(ITEM, P)                                                  \
  ITEM(P, UNCHECKED, 0, "unchecked") ITEM(P, COMPLETE, 1, "writer-set-complete")     \
  ITEM(P, DISABLED, 2, "DISABLED")

#define KZ_ENUM_TERMINAL_FLAGS(ITEM, P) /* set */                                    \
  ITEM(P, PROCESS_TERMINAL, 0x1, "process-terminal") ITEM(P, IN_LOOP, 0x2, "in-loop") \
  ITEM(P, UNGUARDED, 0x4, "unguarded")

#define KZ_ENUM_OUTPTR_SUMMARY(ITEM, P)                                              \
  ITEM(P, NONE, 0, "none") ITEM(P, STORE_ONLY, 1, "store-only")                      \
  ITEM(P, ESCAPES, 2, "escapes") ITEM(P, UNSUMMARIZED, 3, "unsummarized")            \
  ITEM(P, DATA_FN, 4, "data-function")

#define KZ_ENUM_ANCHOR_AUDIT(ITEM, P)                                                \
  ITEM(P, UNAUDITED, 0, "unaudited") ITEM(P, OK, 1, "ok") ITEM(P, MISMATCH, 2, "mismatch") \
  ITEM(P, AMBIGUOUS, 3, "ambiguous") ITEM(P, MISSING, 4, "missing")

#define KZ_ENUM_ACTOR_KIND(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, STATIC_ENTRY, 1, "static-entry") ITEM(P, SPAWNED, 2, "spawned") \
  ITEM(P, CALLBACK, 3, "callback") ITEM(P, ENV_ROOT, 4, "env-root") ITEM(P, ASYNC, 5, "async")

#define KZ_ENUM_SPAWN_RES(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, RESOLVED, 1, "resolved") ITEM(P, UNKNOWN_EFFECT, 2, "unknown_effect")

#define KZ_ENUM_NODE_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, ENTRY, 1, "entry") ITEM(P, EFFECT, 2, "effect")   \
  ITEM(P, LOAD, 3, "load") ITEM(P, STORE, 4, "store") ITEM(P, SPAWN, 5, "spawn")     \
  ITEM(P, TERMINAL, 6, "terminal") ITEM(P, HANDLER_MARK, 7, "handler-mark")          \
  ITEM(P, LOOP_HEAD, 8, "loop-head") ITEM(P, CHOICE, 9, "choice") ITEM(P, EXIT, 10, "exit") \
  ITEM(P, CAPTURE, 11, "capture") ITEM(P, AWAIT, 12, "await")

#define KZ_ENUM_NODE_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, STAR_CLOSED, 0x1, "star-closed") ITEM(P, TRUNC, 0x2, "TRUNC")              \
  ITEM(P, ONCE, 0x4, "once") ITEM(P, UNKNOWN_EFFECT, 0x8, "unknown_effect")

#define KZ_ENUM_EDGE_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, PO, 1, "po") ITEM(P, SPAWN, 2, "spawn")           \
  ITEM(P, BACK, 3, "back") ITEM(P, OUTCOME, 4, "outcome") ITEM(P, CHOICE, 5, "choice")

#define KZ_ENUM_ONCE_SUBJECT(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, ACTOR, 1, "actor") ITEM(P, NODE, 2, "node")

#define KZ_ENUM_ONCE_RULE(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, ACTOR_ONCE, 1, "actor-once") ITEM(P, NODE_OUTSIDE_STAR, 2, "node-outside-star-closed-region")

#define KZ_FACTS_ENUMS(D)                      \
  D(KZ_ARG_KIND, KZ_ENUM_ARG_KIND, 0)          \
  D(KZ_ARG_ROLE, KZ_ENUM_ARG_ROLE, 0)          \
  D(KZ_CALL_FLAG, KZ_ENUM_CALL_FLAGS, 1)       \
  D(KZ_CALL_KIND, KZ_ENUM_CALL_KIND, 0)        \
  D(KZ_FUNC_FLAG, KZ_ENUM_FUNC_FLAGS, 1)       \
  D(KZ_DERIVES_STATUS, KZ_ENUM_DERIVES_STATUS, 0) \
  D(KZ_DERIVES_REASON, KZ_ENUM_DERIVES_REASON, 0) \
  D(KZ_EXPR_OP, KZ_ENUM_EXPR_OP, 0)            \
  D(KZ_EXPR_REF, KZ_ENUM_EXPR_REF, 0)          \
  D(KZ_LOCK_KIND, KZ_ENUM_LOCK_KIND, 0)        \
  D(KZ_LOOP_KIND, KZ_ENUM_LOOP_KIND, 0)        \
  D(KZ_LOOP_CERT, KZ_ENUM_LOOP_CERT, 0)        \
  D(KZ_PRED_FORM, KZ_ENUM_PRED_FORM, 0)        \
  D(KZ_LOOP_FLAG, KZ_ENUM_LOOP_FLAGS, 1)       \
  D(KZ_ITER_KIND, KZ_ENUM_ITER_KIND, 0)        \
  D(KZ_ITER_STATUS, KZ_ENUM_ITER_STATUS, 0)    \
  D(KZ_ITER_SPAWN, KZ_ENUM_ITER_SPAWN, 0)      \
  D(KZ_SPAWN_PARAM_KIND, KZ_ENUM_SPAWN_PARAM_KIND, 0) \
  D(KZ_FWRITE_KIND, KZ_ENUM_FWRITE_KIND, 0)    \
  D(KZ_TRIP_STATUS, KZ_ENUM_TRIP_STATUS, 0)    \
  D(KZ_INIT_SOURCE, KZ_ENUM_INIT_SOURCE, 0)    \
  D(KZ_CONST_KIND, KZ_ENUM_CONST_KIND, 0)      \
  D(KZ_ATYPE_KIND, KZ_ENUM_ATYPE_KIND, 0)      \
  D(KZ_WRITER_KIND, KZ_ENUM_WRITER_KIND, 0)    \
  D(KZ_WD, KZ_ENUM_WD, 1)                      \
  D(KZ_WRITER_FLAG, KZ_ENUM_WRITER_FLAGS, 1)   \
  D(KZ_CALL_TARGET_KIND, KZ_ENUM_CALL_TARGET_KIND, 0)    \
  D(KZ_CALL_TARGET_REASON, KZ_ENUM_CALL_TARGET_REASON, 0) \
  D(KZ_IMAGE_KIND, KZ_ENUM_IMAGE_KIND, 0)      \
  D(KZ_LOC_STATUS, KZ_ENUM_LOC_STATUS, 0)      \
  D(KZ_TERMINAL_FLAG, KZ_ENUM_TERMINAL_FLAGS, 1) \
  D(KZ_OUTPTR_SUMMARY, KZ_ENUM_OUTPTR_SUMMARY, 0) \
  D(KZ_ANCHOR_AUDIT, KZ_ENUM_ANCHOR_AUDIT, 0)  \
  D(KZ_ACTOR_KIND, KZ_ENUM_ACTOR_KIND, 0)      \
  D(KZ_SPAWN_RES, KZ_ENUM_SPAWN_RES, 0)        \
  D(KZ_NODE_KIND, KZ_ENUM_NODE_KIND, 0)        \
  D(KZ_NODE_FLAG, KZ_ENUM_NODE_FLAGS, 1)       \
  D(KZ_EDGE_KIND, KZ_ENUM_EDGE_KIND, 0)        \
  D(KZ_ONCE_SUBJECT, KZ_ENUM_ONCE_SUBJECT, 0)  \
  D(KZ_ONCE_RULE, KZ_ENUM_ONCE_RULE, 0)

KZ_FACTS_ENUMS(KZ_DEFINE_ENUM_ENTRY)

#define KZ_FACTS_FORMATS(F)                           \
  F(kz_fact_func, tu, IDX, 0)                         \
  F(kz_fact_func, fh, IDX, 0)                         \
  F(kz_fact_func, flags, FLAGS, KZ_FUNC_FLAG)         \
  F(kz_fact_call, tu, IDX, 0)                         \
  F(kz_fact_call, flags, FLAGS, KZ_CALL_FLAG)         \
  F(kz_fact_call, caller_fn, IDX, 0)                  \
  F(kz_fact_call, callee_fn, IDX, 0)                  \
  F(kz_fact_call, fh, IDX, 0)                         \
  F(kz_fact_call, loop, IDX, 0)                       \
  F(kz_fact_call, kind, ENUM, KZ_CALL_KIND)           \
  F(kz_fact_arg, kind, ENUM, KZ_ARG_KIND)             \
  F(kz_fact_arg, role, ENUM, KZ_ARG_ROLE)             \
  F(kz_fact_arg, fn, IDX, 0)                          \
  F(kz_fact_derives, chain_root, IDX, 0)              \
  F(kz_fact_derives, fh, IDX, 0)                      \
  F(kz_fact_derives, status, ENUM, KZ_DERIVES_STATUS) \
  F(kz_fact_derives, reason, ENUM, KZ_DERIVES_REASON) \
  F(kz_fact_expr, op, ENUM, KZ_EXPR_OP)               \
  F(kz_fact_expr, flags, ENUM, KZ_EXPR_REF)           \
  F(kz_fact_expr, a, IDX, 0)                          \
  F(kz_fact_expr, b, IDX, 0)                          \
  F(kz_fact_loop, delay_site, IDX, 0)                 \
  F(kz_fact_loop, exit_root, IDX, 0)                  \
  F(kz_fact_loop, modset_root, IDX, 0)                \
  F(kz_fact_loop, budget_root, IDX, 0)                \
  F(kz_fact_loop, lock_site, IDX, 0)                  \
  F(kz_fact_loop, lock_kind, ENUM, KZ_LOCK_KIND)      \
  F(kz_fact_loop, fn, IDX, 0)                         \
  F(kz_fact_loop, fh, IDX, 0)                         \
  F(kz_fact_loop, kind, ENUM, KZ_LOOP_KIND)           \
  F(kz_fact_loop, trip_status, ENUM, KZ_TRIP_STATUS)  \
  F(kz_fact_loop, cert, ENUM, KZ_LOOP_CERT)           \
  F(kz_fact_loop, pred_form, ENUM, KZ_PRED_FORM)      \
  F(kz_fact_loop, lflags, FLAGS, KZ_LOOP_FLAG)        \
  F(kz_fact_loop, parent, IDX, 0)                     \
  F(kz_fact_loop, pred_root, IDX, 0)                  \
  F(kz_fact_loop, cond_root, IDX, 0)                  \
  F(kz_fact_iter, site, IDX, 0)                       \
  F(kz_fact_iter, arg_index, IDX, 0)                  \
  F(kz_fact_iter, kind, ENUM, KZ_ITER_KIND)           \
  F(kz_fact_iter, status, ENUM, KZ_ITER_STATUS)       \
  F(kz_fact_iter, value_kind, ENUM, KZ_IMAGE_KIND)    \
  F(kz_fact_iter, spawn, ENUM, KZ_ITER_SPAWN)         \
  F(kz_fact_iter, guard_root, IDX, 0)                 \
  F(kz_fact_iter, fn, IDX, 0)                         \
  F(kz_fact_iter, fh, IDX, 0)                         \
  F(kz_fact_spawn_param, fn, IDX, 0)                  \
  F(kz_fact_spawn_param, spawn_site, IDX, 0)          \
  F(kz_fact_spawn_param, entry_fn, IDX, 0)            \
  F(kz_fact_spawn_param, call_site, IDX, 0)           \
  F(kz_fact_spawn_param, kind, ENUM, KZ_SPAWN_PARAM_KIND) \
  F(kz_fact_fwrite, fn, IDX, 0)                       \
  F(kz_fact_fwrite, kind, ENUM, KZ_FWRITE_KIND)       \
  F(kz_fact_fwrite, fh, IDX, 0)                       \
  F(kz_fact_init, source, ENUM, KZ_INIT_SOURCE)       \
  F(kz_fact_init, prov, IDX, 0)                       \
  F(kz_fact_init, fh, IDX, 0)                         \
  F(kz_fact_const, tu, IDX, 0)                        \
  F(kz_fact_const, prov, IDX, 0)                      \
  F(kz_fact_const, fh, IDX, 0)                        \
  F(kz_fact_const, kind, ENUM, KZ_CONST_KIND)         \
  F(kz_fact_atype, kind, ENUM, KZ_ATYPE_KIND)         \
  F(kz_fact_atype, fh, IDX, 0)                        \
  F(kz_fact_atype, fn, IDX, 0)                        \
  F(kz_fact_writer, kind, ENUM, KZ_WRITER_KIND)       \
  F(kz_fact_writer, value_kind, ENUM, KZ_ARG_KIND)    \
  F(kz_fact_writer, tstatus, ENUM, KZ_TSTATUS)        \
  F(kz_fact_writer, kstatus, ENUM, KZ_KSTATUS)        \
  F(kz_fact_writer, wd, FLAGS, KZ_WD)                 \
  F(kz_fact_writer, guard_root, IDX, 0)               \
  F(kz_fact_writer, fh, IDX, 0)                       \
  F(kz_fact_writer, fn, IDX, 0)                       \
  F(kz_fact_writer, via_fn, IDX, 0)                   \
  F(kz_fact_writer, flags, FLAGS, KZ_WRITER_FLAG)     \
  F(kz_anchor, audit, ENUM, KZ_ANCHOR_AUDIT)          \
  F(kz_anchor, insn_class, ENUM, KZ_INSN_CLASS)       \
  F(kz_anchor, file_offset, HEX, 0)                   \
  F(kz_actor, kind, ENUM, KZ_ACTOR_KIND)              \
  F(kz_closure_spawn, creator, IDX, 0)                \
  F(kz_closure_spawn, resolution, ENUM, KZ_SPAWN_RES) \
  F(kz_skel_node, kind, ENUM, KZ_NODE_KIND)           \
  F(kz_skel_node, view, ENUM, KZ_VIEW)                \
  F(kz_skel_node, flags, FLAGS, KZ_NODE_FLAG)         \
  F(kz_skel_node, site, IDX, 0)                       \
  F(kz_skel_node, profile_entry, IDX, 0)              \
  F(kz_skel_node, region, IDX, 0)                     \
  F(kz_skel_edge, kind, ENUM, KZ_EDGE_KIND)           \
  F(kz_skel_edge, label, IDX, 0)                      \
  F(kz_skel_once, subject_kind, ENUM, KZ_ONCE_SUBJECT) \
  F(kz_skel_once, rule, ENUM, KZ_ONCE_RULE)           \
  F(kz_fact_target, fn, IDX, 0)                       \
  F(kz_fact_target, kind, ENUM, KZ_CALL_TARGET_KIND)       \
  F(kz_fact_target, reason, ENUM, KZ_CALL_TARGET_REASON)   \
  F(kz_fact_image, fn, IDX, 0)                        \
  F(kz_fact_image, kind, ENUM, KZ_IMAGE_KIND)         \
  F(kz_fact_image, fh, IDX, 0)                        \
  F(kz_fact_loc, status, ENUM, KZ_LOC_STATUS)         \
  F(kz_fact_terminal, fn, IDX, 0)                     \
  F(kz_fact_terminal, guard_root, IDX, 0)             \
  F(kz_fact_terminal, fh, IDX, 0)                     \
  F(kz_fact_terminal, flags, FLAGS, KZ_TERMINAL_FLAG) \
  F(kz_fact_outcall, callee_fn, IDX, 0)               \
  F(kz_fact_outcall, fh, IDX, 0)                      \
  F(kz_fact_outcall, summary, ENUM, KZ_OUTPTR_SUMMARY) \
  F(kz_tu, fh, IDX, 0)

#endif /* KZ_SCHEMA_KZ_FACTS_H */

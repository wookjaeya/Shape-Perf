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
 * classifies the entry.  Revision 4: FACT_ATYPE has the kind `unknown-effect` (SPEC 4.2 steps
 * 2-3: a pointer that reaches a target goes to code without a body, to an unresolved indirect
 * call, to a variadic position, or to inline assembly; version 3); FACT_LOCS counts those hits
 * in n_unknown_effect (version 2); writers have the flag `partial-write` (a store to a part of a
 * record- or array-typed target); loops have the flag `static-image-assumed` (a table walk read
 * through a link that is not const, unrolled under a declared assumption).  Revision 5
 * (2026-10-30, the profile-free mode of `kz-static extract --no-profile`): three NEW sections in a
 * second extraction range, 120 (P0_TABLE: the built-in table of thread-library and C-library
 * primitives the mode starts from, one row per entry with its version tag and source check),
 * 121 (OP_DERIV: the delay and terminal operations derived from those primitives through
 * wrappers, per function `must` / `noreturn` / `may` with the call that leads there, its guard
 * and the call chain) and 122 (SPAWN_WRAP: the spawn operations derived the same way, and the
 * startup-script entries a spawn with a data-valued entry resolves to through the symbol lookup);
 * terminals have the flag `conditional`.  No earlier record or section version changes, and the
 * profile-driven mode writes none of the new sections, so its facts are written as before.
 * Revision 6 (2026-10-30, the extended facts F1-F3): four NEW sections in the second extraction
 * range, 123 (READS: the field and global reads of every body, with their use, the subscripts on
 * the access path and the guard), 124 (RETURNS: every return value, a returned local resolved to
 * the definitions that reach the return, each with its value and the condition under which the
 * function returns it), 125 (RET_CONSTS: per function the constants it returns and when) and 126
 * (LOOP_EXITS: the exit conditions, every exit and the mod set of every loop, whatever its kind).
 * They are written by `kz-static extract --no-profile` and, in the profile-driven mode, only with
 * `--extended-facts`; a profile-driven extraction without that option writes its facts as before.
 * Their expression roots are in FACT_EXPRS after the roots of the earlier sections.
 * Revision 7 (2026-10-30, the extended facts F4 and F6): four NEW sections in the second extraction
 * range, written with the extended facts (the same condition as revision 6), 127 (ARGFLOW: the copy
 * chain of every call argument, not only of the arguments with a role, with its source and the
 * storage kind of the base of its access path), 128 (PARAM_FLOW: per function parameter, where its
 * value goes in the body -- a call argument, a comparison, a return, a store, a subscript, a store
 * through it -- and the same composed through the calls over the call graph, callees first), 129
 * (PTR_FIELDS: the writes of a pointer field whose value is the address of a global object, and
 * the reads through such a field resolved to that object, with the program order of the write and
 * the read) and 130 (GLOBAL_IMAGES: the static initial values of the global objects those facts
 * reach).  Their expression roots are in FACT_EXPRS after the roots of revision 6.
 * Revision 8 (2026-10-30, F5 and the target-independent rules): five NEW sections in the second
 * extraction range, written with the extended facts (the same condition as revision 6), 131
 * (LOCK_OPS: the lock operations derived from the mutex primitives of P0 through wrappers, with the
 * identity of the lock over the parameters), 132 (LOCK_IDS: the lock identities and whether each is
 * a static instance), 133 (LOCKSET: the locks held at every call, read and write, by a forward
 * must analysis on each body's control-flow graph), 134 (FIELD_STATUS: the writer-set completeness
 * of every field of the record types the reads name under two rules that do not depend on the
 * targets, per field and type-level, from an extraction whose targets are exactly those fields) and
 * 135 (AWAIT_L1: per poll loop, L1 (i) with the relevant locations the predicate reads, directly and
 * through the functions it calls, under each rule, and L1 (ii) from LOCKSET).  Their expression roots
 * are in FACT_EXPRS after the roots of revision 7.
 * Revision 9 (2026-10-30, the order and the entry paths of the F5 events): one NEW section in the second
 * extraction range, written with the extended facts (the same condition as revision 6), 136
 * (LOCK_PATHS: per LOCKSET event, the spawn calls of its function it follows on the control-flow
 * graph, and whether a call on some path from the function's entry to it releases a lock, may release
 * one or has a lock effect the analysis cannot follow); P0_TABLE has four more kinds (thread-specific
 * key creation and deletion, cleanup handler push and pop).  The section has no expression roots; no
 * earlier record or section version changes.
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
  KZ_SEC_FUNC_WRITES = 39,
  KZ_SEC_P0_TABLE = 120,
  KZ_SEC_OP_DERIV = 121,
  KZ_SEC_SPAWN_WRAP = 122,
  KZ_SEC_READS = 123,
  KZ_SEC_RETURNS = 124,
  KZ_SEC_RET_CONSTS = 125,
  KZ_SEC_LOOP_EXITS = 126,
  KZ_SEC_ARGFLOW = 127,
  KZ_SEC_PARAM_FLOW = 128,
  KZ_SEC_PTR_FIELDS = 129,
  KZ_SEC_GLOBAL_IMAGES = 130,
  KZ_SEC_LOCK_OPS = 131,
  KZ_SEC_LOCK_IDS = 132,
  KZ_SEC_LOCKSET = 133,
  KZ_SEC_FIELD_STATUS = 134,
  KZ_SEC_AWAIT_L1 = 135,
  KZ_SEC_LOCK_PATHS = 136
};

#define KZ_FACTS_SECTIONS(S)                   \
  S(FACTS, 10, kz_fact_call, 2)                \
  S(FACT_ARGS, 11, kz_fact_arg, 2)             \
  S(DERIVES, 12, kz_fact_derives, 2)           \
  S(FACT_EXPRS, 13, kz_fact_expr, 3)           \
  S(FACT_LOOPS, 14, kz_fact_loop, 3)           \
  S(FACT_INIT, 15, kz_fact_init, 2)            \
  S(FACT_CONSTS, 16, kz_fact_const, 2)         \
  S(FACT_ATYPE, 17, kz_fact_atype, 3)          \
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
  S(FACT_LOCS, 33, kz_fact_loc, 2)             \
  S(TERMINALS, 34, kz_fact_terminal, 1)        \
  S(OUTCALLS, 35, kz_fact_outcall, 1)          \
  S(TUS, 36, kz_tu, 1)                         \
  S(LOOP_ITERS, 37, kz_fact_iter, 1)           \
  S(SPAWN_PARAMS, 38, kz_fact_spawn_param, 1)  \
  S(FUNC_WRITES, 39, kz_fact_fwrite, 1)        \
  S(P0_TABLE, 120, kz_fact_p0, 1)              \
  S(OP_DERIV, 121, kz_fact_opderiv, 1)         \
  S(SPAWN_WRAP, 122, kz_fact_spawnw, 1)        \
  S(READS, 123, kz_fact_read, 1)               \
  S(RETURNS, 124, kz_fact_return, 1)           \
  S(RET_CONSTS, 125, kz_fact_retconst, 1)      \
  S(LOOP_EXITS, 126, kz_fact_loopexit, 1)      \
  S(ARGFLOW, 127, kz_fact_argflow, 1)          \
  S(PARAM_FLOW, 128, kz_fact_pflow, 1)         \
  S(PTR_FIELDS, 129, kz_fact_ptrfield, 1)      \
  S(GLOBAL_IMAGES, 130, kz_fact_gimage, 1)     \
  S(LOCK_OPS, 131, kz_fact_lockop, 1)          \
  S(LOCK_IDS, 132, kz_fact_lockid, 1)          \
  S(LOCKSET, 133, kz_fact_lockset, 1)          \
  S(FIELD_STATUS, 134, kz_fact_fstatus, 1)     \
  S(AWAIT_L1, 135, kz_fact_await, 1)          \
  S(LOCK_PATHS, 136, kz_fact_lockpath, 1)

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
 * candidates of the site.  With the extended facts, a site whose candidates are resolved but whose
 * function value may also come through a store the flow graph does not follow has, besides its
 * candidate rows, a row with fn = KZ_NONE32 and reason `incomplete-source` (`via` names the node
 * and why; static_facts.md §5i).
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
  X(T, uint32_t, n_unknown_effect, ) /* A-TYPE: code whose effect is unknown may write it */ \
  X(T, uint32_t, n_init, )     /* objects in the INIT image */                       \
  X(T, uint32_t, n_unresolved_indirect, ) /* whole program; informational (an unresolved call counts only through unknown-effect hits) */ \
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

/*
 * P0_TABLE (revision 5).  One entry of the built-in table of thread-library and C-library
 * primitives that `kz-static extract --no-profile` starts from (tools/kz-static, lib/llvm/p0.cpp).
 * An entry covers one function of an API family (every mutex primitive, every termination
 * primitive, ...); `version` names the reference version the semantics are read from and `source`
 * the line of the upstream source it was checked against, or "unverified".  The parameters are
 * positions in the call (KZ_NONE32 where the kind has none).  `fn` is the function of that name in
 * this program (KZ_NONE32 when the program does not mention it); `n_calls` counts its direct calls.
 * A program that defines a function of the same name has its own body read instead (`shadowed`).
 * Revision 9 of the extraction records: a thread-specific key creation has the destructor the thread
 * runs at its exit in `entry_param` (the key is written through `out_param`), and a cleanup push the
 * handler in `entry_param` when the call passes it.
 */
#define KZ_FACT_P0_FIELDS(X, T)                                                      \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, out_param, )   /* written through: the new thread id, a data destination */ \
  X(T, uint32_t, entry_param, ) /* thread creation: the function the new thread starts at; key creation: the destructor */ \
  X(T, uint32_t, arg_param, )   /* thread creation: the argument that function receives */ \
  X(T, uint32_t, n_calls, )                                                          \
  X(T, uint8_t, kind, )         /* KZ_P0_KIND */                                     \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )       /* KZ_P0_FLAG */                                     \
  X(T, kz_str, name, )                                                               \
  X(T, kz_str, family, )                                                             \
  X(T, kz_str, semantics, )                                                          \
  X(T, kz_str, version, )                                                            \
  X(T, kz_str, source, )

/*
 * OP_DERIV (revision 5, F8 DELAY-W / TERM-W).  A function that reaches a delay or a terminal
 * primitive of P0_TABLE through its calls, derived to a fixed point over the strongly connected
 * components of the call graph (direct calls and resolved indirect targets), callees first.  One
 * row per (function, kind, call in the function that leads there): `site` is that call, `next_fn`
 * its callee (for an indirect call, the target with the shortest chain), `guard_root` the
 * conditions it executes under in `fn`, `chain` the calls from `fn` to the primitive through
 * `next_fn` (each step the shortest, ties by name), `depth` their number.  `mode` is the
 * function's: MUST every path of `fn` reaches the primitive (delay: every path that returns, and
 * one does; terminal: every path, an endless one included, ends in a terminal call); NORETURN
 * (terminal) no path returns and some path reaches it; MAY some path does, under the guards of
 * the rows.  `vocabulary`: the function is an operation of that kind in the profile-free run
 * (delay: MUST; terminal: every mode), so the call, loop and terminal facts of that run follow
 * from it.
 */
#define KZ_FACT_OPDERIV_FIELDS(X, T)                                                 \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, next_fn, )                                                          \
  X(T, uint32_t, guard_root, ) /* FACT_EXPRS, or KZ_NONE32 */                        \
  X(T, uint32_t, depth, )                                                            \
  X(T, uint8_t, kind, )        /* KZ_OPDERIV_KIND */                                 \
  X(T, uint8_t, mode, )        /* KZ_OPDERIV_MODE */                                 \
  X(T, uint16_t, flags, )      /* KZ_OPDERIV_FLAG */                                 \
  X(T, kz_str, primitive, )                                                          \
  X(T, kz_str, chain, )

/*
 * SPAWN_WRAP (revision 5, F7 SPAWN-W).  A spawn operation derived from the thread-creation
 * primitives of P0_TABLE to a fixed point, or a resolution of a spawn entry:
 *   ENTRY_ARG     parameter `param` of `fn` flows (function-pointer flow graph) to the entry
 *                 argument of the spawn call `site` in `fn`;
 *   CALLED_BY_ENTRY  it flows (stored in a field, field-based) to the indirect call `call_site` in
 *                 code reachable from `entry_fn`, the entry of the spawn call `site` in `fn`;
 *   FIXED_ENTRY   `fn` makes the spawn call `site` with an entry that does not come from its
 *                 parameters and that the flow graph resolves (`entry_fn`, more rows for more).  It
 *                 serves the CALLED_BY_ENTRY test of the functions that call `fn` (one level: their
 *                 own callers do not get it), and `fn` is no spawn operation (no `vocabulary`);
 *   SCRIPT_ENTRY  the entry argument `param` of the spawn call `site` in `fn` is data converted to
 *                 a function pointer that no function constant reaches, and `fn` reaches the symbol
 *                 lookup primitive of P0_TABLE through direct calls (`chain`, `call_site` the call
 *                 of the primitive):
 *                 `entry_fn` is the entry a line of the startup script names (basis
 *                 startup.entry[k], second field, of the kind `app`; `via`), or KZ_NONE32 with
 *                 `not-in-closure` when no function of that name is in the closed program.
 * `next_fn` is the spawn operation `site` calls, `depth` the spawn operations from `fn` down to the
 * primitive, `chain` that path.  `vocabulary`: a spawn operation of the profile-free run (the
 * primitives and the rows with an entry parameter).  This is a may-flow fact like SPAWN_PARAMS.
 */
#define KZ_FACT_SPAWNW_FIELDS(X, T)                                                  \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, param, )                                                            \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, next_fn, )                                                          \
  X(T, uint32_t, entry_fn, )                                                         \
  X(T, uint32_t, call_site, )                                                        \
  X(T, uint32_t, depth, )                                                            \
  X(T, uint8_t, kind, )        /* KZ_SPAWNW_KIND */                                  \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )      /* KZ_SPAWNW_FLAG */                                  \
  X(T, kz_str, primitive, )                                                          \
  X(T, kz_str, chain, )                                                              \
  X(T, kz_str, via, )

/*
 * READS (revision 6, F1).  A read of a field or of a global object in a function body: an
 * lvalue-to-rvalue conversion of a member access, of a global variable or of an element of a
 * global array (KZ_READ_KIND), or an array of that kind that decays to a pointer (the contents are
 * read through it: `array-contents`).  (type, field) is the abstract location as the writers name
 * it (FIELD: the owner type and the field; GLOBAL: "<global>" and the object).  The access path is
 * followed to its root (`root_kind`, `root`, `root_param`): `x.a[i].f` has the root x, `p->f` the
 * pointer p (`through-pointer`), `g()->f` the call.  `index_root` lists the subscripts on the path,
 * root first (`index-unfolded`: a subscript local replaced by its single definition, as in RETURNS).  `use` is every context the value passes on its way up to the statement: a branch
 * condition (if / loop / switch / ?: condition, an operand of && or ||; `ctx_root` the innermost
 * such condition), the value of a return statement, the right side of a store through a pointer
 * parameter (`out-store`; `ctx_root` the destination) or of another store or initializer
 * (`store`), a call argument (`site`, `arg_index`: the innermost call), a subscript, a
 * dereferenced pointer, the callee of an indirect call; `other` when none of them.  The guard is
 * the writers' (the conditions the read executes under).  `loop` is the innermost loop.
 */
#define KZ_FACT_READ_FIELDS(X, T)                                                    \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, loop, )       /* FACT_LOOPS index, or KZ_NONE32 */                  \
  X(T, uint32_t, site, )       /* call-argument: the call, else KZ_NONE32 */         \
  X(T, uint32_t, arg_index, )  /* call-argument: the argument, else KZ_NONE32 */     \
  X(T, uint32_t, index_root, ) /* FACT_EXPRS: SEQ of the subscripts, or KZ_NONE32 */ \
  X(T, uint32_t, guard_root, ) /* FACT_EXPRS, or KZ_NONE32 */                        \
  X(T, uint32_t, ctx_root, )   /* FACT_EXPRS: the condition or the destination, or KZ_NONE32 */ \
  X(T, uint8_t, kind, )        /* KZ_READ_KIND */                                    \
  X(T, uint8_t, root_kind, )   /* KZ_READ_ROOT */                                    \
  X(T, uint16_t, use, )        /* KZ_READ_USE */                                     \
  X(T, uint32_t, flags, )      /* KZ_READ_FLAG */                                    \
  X(T, uint32_t, root_param, ) /* root_kind param: its index, else KZ_NONE32 */      \
  X(T, kz_str, type, )                                                               \
  X(T, kz_str, field, )                                                              \
  X(T, kz_str, root, )                                                               \
  X(T, kz_str, text, )         /* the access path as written */                      \
  X(T, kz_str, file, )                                                               \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, rsv0, )

/*
 * RETURNS (revision 6, F2).  One row per (return statement with a value, alternative).  The value
 * is the return expression with the locals that have a single copy definition before the return
 * replaced by their value (`unfolded`; the definition table of the derives rows).  A returned local
 * is resolved to the definitions that reach the return on the control-flow graph of the body
 * (reaching definitions): one alternative per definition (`from-definition`, `local`, (def_line,
 * def_col) in def_file), its value the definition's right side; `redefined-later` when another of
 * those definitions can follow it on a path to the return; `undefined-on-a-path` when a path
 * reaches the return with no definition.  `guard_root` is the conditions at the return statement,
 * `def_guard_root` those at the definition, and `cond_root` the condition under which the function
 * returns this value: the return's guard, the definition's guard and the negations of the guards of
 * the definitions that can follow it, with single-definition locals unfolded.  The guards are those
 * of the writers (structured conditions): inside a loop (`in-loop`) they hold per iteration.
 */
#define KZ_FACT_RETURN_FIELDS(X, T)                                                  \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, alt, )            /* 0-based alternative of this return statement */ \
  X(T, uint32_t, n_alts, )                                                           \
  X(T, uint32_t, value_root, )     /* FACT_EXPRS */                                  \
  X(T, uint32_t, guard_root, )     /* FACT_EXPRS, or KZ_NONE32 */                    \
  X(T, uint32_t, def_guard_root, ) /* FACT_EXPRS, or KZ_NONE32 */                    \
  X(T, uint32_t, cond_root, )      /* FACT_EXPRS, or KZ_NONE32 (always) */           \
  X(T, uint32_t, site, )           /* call-result: the call returned, else KZ_NONE32 */ \
  X(T, uint32_t, loop, )           /* FACT_LOOPS index of the innermost loop, or KZ_NONE32 */ \
  X(T, uint8_t, vkind, )           /* KZ_RET_VALUE */                                \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )          /* KZ_RET_FLAG */                                 \
  X(T, int64_t, value, )           /* const: the value */                            \
  X(T, kz_str, local, )            /* from-definition: the local */                  \
  X(T, kz_str, text, )             /* the value as written */                        \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, def_file, )                                                           \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, def_line, )                                                         \
  X(T, uint32_t, def_col, )                                                          \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, rsv1, )

/*
 * RET_CONSTS (revision 6, F2).  Per function and constant: the function returns `value` when
 * `cond_root` holds, the disjunction of the conditions of its RETURNS rows of that constant
 * (KZ_NONE32: always).  `flags` is the union of the rows' flags, and `only-constants` when every
 * RETURNS row of the function is a constant.  `text` is the constant as the first row writes it.
 */
#define KZ_FACT_RETCONST_FIELDS(X, T)                                                \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, n_rows, )                                                           \
  X(T, uint32_t, cond_root, ) /* FACT_EXPRS, or KZ_NONE32 */                         \
  X(T, uint32_t, flags, )     /* KZ_RET_FLAG */                                      \
  X(T, int64_t, value, )                                                             \
  X(T, kz_str, text, )

/*
 * LOOP_EXITS (revision 6, F3).  The exits of every loop, whatever its kind and whether it calls a
 * delay operation (FACT_LOOPS keeps these trees for the loops that wait and the table walks only:
 * `as-in-loops`).  A LOOP row: `root` the exit conditions (their disjunction), `aux_root` the mod set
 * of the body (LOCSET: fields, globals and locals written, callees), `n` the number of exits.  One
 * EXIT row per exit, in the order of the walk (the loop condition first): `root` the conditions under
 * which it is taken (the guards from the loop down to it), `aux_root` the condition that decides it
 * (the innermost guard inside the loop, or the negated loop condition; KZ_NONE32: `unconditional`).
 * The kind and the delay-in-body of the loop stay in FACT_LOOPS.
 */
#define KZ_FACT_LOOPEXIT_FIELDS(X, T)                                                \
  X(T, uint32_t, loop, )     /* FACT_LOOPS index */                                  \
  X(T, uint32_t, exit, )     /* EXIT: 0-based, LOOP: KZ_NONE32 */                    \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, root, )     /* FACT_EXPRS, or KZ_NONE32 */                          \
  X(T, uint32_t, aux_root, ) /* FACT_EXPRS, or KZ_NONE32 */                          \
  X(T, uint32_t, n, )                                                                \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint8_t, kind, )      /* KZ_LOOPEXIT_KIND */                                  \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )    /* KZ_LOOPEXIT_FLAG */                                  \
  X(T, kz_str, file, )

/*
 * ARGFLOW (revision 7, F4).  One row per argument of every call (`site`, `arg_index`, `fn` the
 * caller): where the value comes from in the caller, following copies through locals that have a
 * single copy definition (the rule of the derives rows, applied to every argument; `hops` copies,
 * `chain_root` the chain, its innermost node the source).  A copy is a conversion that keeps the
 * value: lvalue-to-rvalue, a no-op, a widening integer conversion, and a conversion between
 * pointer types (`pointer-conversion`: the address is kept).  `src` is what the chain ends at; for
 * a FIELD, GLOBAL, ELEMENT, DEREF or ADDRESS source, (type, field) is the location as the writers
 * name it and `base` the storage kind of the object at the base of the access path: a global
 * object, an element of a global array, a local, the pointee of a parameter (`param`), of a local
 * pointer, of a pointer field (`root` the field, `object` the global at the root of its path), of a
 * global pointer or of a call result.  A local pointer with a single copy definition is followed
 * to what it points at (`base-via-local`).  `object` is the global object at the root of the path
 * (its key in STATIC_IMAGE and GLOBAL_IMAGES).  `index_root` lists the subscripts on the path, root
 * first.  `read` is the READS row of the source read.  A PARAM source is parameter `param` of the
 * caller; a CALL source names the callee in `root`.  LOCAL and EXPR sources are not followed
 * further (`reason`, the derives reasons).
 */
#define KZ_FACT_ARGFLOW_FIELDS(X, T)                                                 \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, arg_index, )                                                        \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, param, )      /* PARAM source or parameter base: its index, else KZ_NONE32 */ \
  X(T, uint32_t, hops, )                                                             \
  X(T, uint32_t, chain_root, ) /* FACT_EXPRS */                                      \
  X(T, uint32_t, index_root, ) /* FACT_EXPRS: SEQ of the subscripts, or KZ_NONE32 */ \
  X(T, uint32_t, read, )       /* READS index of the source read, or KZ_NONE32 */    \
  X(T, uint8_t, src, )         /* KZ_ARGSRC */                                       \
  X(T, uint8_t, base, )        /* KZ_ARGBASE */                                      \
  X(T, uint8_t, reason, )      /* KZ_DERIVES_REASON: why a LOCAL or EXPR source stops */ \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint32_t, flags, )      /* KZ_ARGFLOW_FLAG */                                 \
  X(T, int64_t, value, )       /* CONST: the value */                                \
  X(T, kz_str, type, )                                                               \
  X(T, kz_str, field, )                                                              \
  X(T, kz_str, root, )                                                               \
  X(T, kz_str, object, )                                                             \
  X(T, kz_str, text, )         /* the argument as written */

/*
 * PARAM_FLOW (revision 7, F4).  Per function parameter (`fn`, `param`): where its value goes.  A
 * row of depth 0 is a use in the body of `fn` of a copy of the parameter (the copy rule of
 * ARGFLOW): CALL_ARG argument `next_param` of the call `site` to `next_fn` (an indirect call: one
 * row per resolved target, `indirect` and `may`; `unresolved` when its targets are not all known;
 * `callee-without-body` when the callee has no body in the closed program; `variadic` in a variadic
 * position); COMPARISON an operand of a comparison or a condition tested for truth (`expr_root` the
 * comparison); RETURN the value returned; STORE the value stored to a field, a global or through a
 * pointer parameter (`expr_root` the destination); SUBSCRIPT a subscript (`expr_root` the access);
 * OUT_STORE the parameter is a pointer the function stores through (`expr_root` the value stored).
 * A row of depth d > 0 is a use of depth 0 in `sink_fn` (its kind, position, expressions and guard)
 * that the parameter reaches through d calls, CALL_ARG rows of functions with a body, composed
 * bottom-up over the strongly connected components of the call graph (callees first) to a fixed
 * point with the shortest depth: `site`, `next_fn` and `next_param` are the first call, `chain` the
 * path, `may` when it passes an indirect call.  CALL_ARG rows of depth > 0 are the open ends of the
 * path (`callee-without-body`, `unresolved`, `variadic`; `sink_site` the call and `n` the argument).
 * OPEN rows: the composition of the parameter was cut by a bound (`depth-bound`, `size-bound`;
 * `n` the uses left out), counted as unresolved.  `guard_root` is the guard at the use in `sink_fn`.
 */
#define KZ_FACT_PFLOW_FIELDS(X, T)                                                   \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, param, )                                                            \
  X(T, uint32_t, depth, )                                                            \
  X(T, uint32_t, site, )       /* the call of the first step, or KZ_NONE32 */        \
  X(T, uint32_t, next_fn, )                                                          \
  X(T, uint32_t, next_param, )                                                       \
  X(T, uint32_t, sink_fn, )                                                          \
  X(T, uint32_t, sink_site, )  /* CALL_ARG at depth > 0: the call at the use, else KZ_NONE32 */ \
  X(T, uint32_t, expr_root, )  /* FACT_EXPRS, or KZ_NONE32 */                        \
  X(T, uint32_t, guard_root, ) /* FACT_EXPRS, or KZ_NONE32 */                        \
  X(T, uint32_t, n, )          /* CALL_ARG at depth > 0: the argument; OPEN: the uses left out */ \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint8_t, kind, )        /* KZ_PFLOW_KIND */                                   \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )      /* KZ_PFLOW_FLAG */                                   \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, text, )                                                               \
  X(T, kz_str, chain, )        /* depth > 0: "f#k -> g#j -> ..." */

/*
 * PTR_FIELDS (revision 7, F4 pointer-field flow).  ADDRESS_WRITE: a store to the pointer field
 * (type, field) whose value is the address of the global `object` (element `elem`; an array that
 * decays is element 0) in `fn` (`root` the global at the root of the written member, `guard_root`
 * its guard, `n` the other typed writes of the field -- stores, whole-record writes, address
 * escapes, in any function -- that are not overwritten by it: a write in `fn` on every path before
 * it, outside a loop around it, is overwritten).  STATIC_INIT: the static initializer of `root`
 * sets the field to that address.  READ: a READS row (`read`, `fn`) whose access path goes through
 * the pointer field (the step nearest to the root of the path; `multi-level` when more steps
 * follow), with `path` the rest of the path from the pointee to the location read and `index_root`
 * the subscript of the step (none for `->`, element 0).  `status`: RESOLVED when exactly one
 * address write of the same root (or a static initializer) holds the address, it precedes the read
 * in program order -- the read is in the writer after the write on every path, or every call path
 * to the reader passes a call of the writer that every path reaches after the write
 * (`write-before-read`) -- and no other typed write of the field remains (`n` 0 on the write);
 * MAY when an address write exists but not all of that holds (`n` the candidate writes); NEEDS
 * field points-to (UNRESOLVED) when no address write of the field is found.  `write` is the
 * ADDRESS_WRITE or STATIC_INIT row of the (first) candidate.  When the subscript is the induction
 * variable of an enclosing counted loop of known trip count, its values are `elem` + k * `idx_step`
 * for k < `idx_n` (`index-from-counted-loop`; `loop-may-exit-early` for an upper bound).  Typed
 * writes only: stores through untyped pointers and code without a body are not excluded.
 */
#define KZ_FACT_PTRFIELD_FIELDS(X, T)                                                \
  X(T, uint32_t, fn, )         /* the writer or the reader; KZ_NONE32 for a static initializer */ \
  X(T, uint32_t, read, )       /* READ: READS index, else KZ_NONE32 */               \
  X(T, uint32_t, write, )      /* READ: the PTR_FIELDS row of the address, else KZ_NONE32 */ \
  X(T, uint32_t, n, )                                                                \
  X(T, uint32_t, index_root, ) /* READ: FACT_EXPRS, or KZ_NONE32 */                  \
  X(T, uint32_t, guard_root, ) /* ADDRESS_WRITE: FACT_EXPRS, or KZ_NONE32 */         \
  X(T, uint32_t, idx_n, )      /* READ: the number of index values, 0 when unknown */ \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint8_t, kind, )        /* KZ_PTRF_KIND */                                    \
  X(T, uint8_t, status, )      /* READ: KZ_PTRF_STATUS */                            \
  X(T, uint16_t, flags, )      /* KZ_PTRF_FLAG */                                    \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, int64_t, elem, )        /* writes: the element; READ: the first index value */ \
  X(T, int64_t, idx_step, )    /* READ */                                            \
  X(T, kz_str, type, )         /* the owner type of the pointer field */             \
  X(T, kz_str, field, )        /* the pointer field */                               \
  X(T, kz_str, root, )                                                               \
  X(T, kz_str, object, )       /* writes: the object addressed; READ: resolved (first candidate) */ \
  X(T, kz_str, path, )         /* READ: from the pointee to the location; writes: the value as written */ \
  X(T, kz_str, file, )

/*
 * GLOBAL_IMAGES (revision 7, F6).  The static initial value of every global object that ARGFLOW or
 * PTR_FIELDS reaches (`reach`), whatever table walks there are: one row per leaf of its
 * initializer as the static-image reader of STATIC_IMAGE flattens it (the same leaves, kinds and
 * paths), a scalar object as one leaf, an object defined without an initializer as one
 * `zero-initialized` row.  (file, line, col) is where the value is written; when it comes from a
 * macro, `macro` names the macro written there and (spell_file, spell_line, spell_col) is where its
 * text is spelled (the definition).  (owner, field) is the record field the leaf is, when it is
 * one.  `object-written`: a function stores to the object or takes its address; `field-written`:
 * a function stores to (owner, field) or takes its address (field-based, any object of the type);
 * `record-written`: a whole record of type `owner` is written.  Without these the static value is
 * the value at run time for typed writes; stores through untyped pointers are not excluded.
 */
#define KZ_FACT_GIMAGE_FIELDS(X, T)                                                  \
  X(T, uint32_t, fn, )         /* FUNCTION leaf: the function, else KZ_NONE32 */     \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint32_t, spell_line, )                                                       \
  X(T, uint32_t, spell_col, )                                                        \
  X(T, uint8_t, kind, )        /* KZ_IMAGE_KIND */                                   \
  X(T, uint8_t, reach, )       /* KZ_GIMAGE_REACH */                                 \
  X(T, uint16_t, flags, )      /* KZ_GIMAGE_FLAG */                                  \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, int64_t, value, )                                                             \
  X(T, kz_str, object, )                                                             \
  X(T, kz_str, path, )                                                               \
  X(T, kz_str, text, )                                                               \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, spell_file, )                                                         \
  X(T, kz_str, macro, )                                                              \
  X(T, kz_str, owner, )                                                              \
  X(T, kz_str, field, )

/*
 * LOCK_OPS (revision 8, F5).  A lock operation: a mutex primitive of P0_TABLE (`primitive-op`), or a
 * function derived from those primitives through its calls, bottom-up over the strongly connected
 * components of the call graph (callees first; a call into the function's own recursive component
 * is read as one that may release every lock).  Each body is read on its control-flow graph by a
 * forward analysis whose state is the set of locks held, one state per distinct set (meet within a
 * set: the facts about the locals that hold call results).  `kind`: ACQUIRE the function returns
 * holding the lock `ident` that it did not hold on entry; RELEASE it returns without a lock it did
 * not acquire itself (a release of a lock the caller holds); WAIT it releases the lock and holds it
 * again (a condition wait).  `mode` of an ACQUIRE: EVERY_RETURN on every path that returns;
 * RESULT_CONDITIONAL on the paths that return `success`, and the others return a value known to
 * differ; TRY on the paths of a try or timed acquisition (no assumption of success); CONDITIONAL on
 * some paths without a result that tells (no operation: the caller does not count it as held).
 * `assumes-success`: the derivation reads the acquisitions under the declared assumption that an
 * acquisition which is not a try succeeds (P0: the primitive returns 0; a derived operation returns
 * its `success`); the branch a failure takes is read as not reached, and the LOCKSET rows in it are
 * marked `failure-path`.  `ident` is the identity of the lock over the parameters of `fn` ("$k" the
 * parameter k), `may` the field-based location of the mutex object (never used to prove that two
 * operations are the same lock; used only to keep a release from releasing a lock of another
 * location).  `site` is the call in `fn` that leads to the primitive, `next_fn` its callee, `chain`
 * the path, `depth` its length.  `vocabulary`: an ACQUIRE of mode EVERY_RETURN or RESULT_CONDITIONAL,
 * or a RELEASE of those modes of a mutex whose field-based location an ACQUIRE of the vocabulary
 * acquires (the primitive always): an operation of this kind is a lock or unlock operation of the
 * profile-free run (its
 * call and loop facts: KZ_CALL_FLAG lock-op / unlock-op, the L1 (ii) region), derived on its first pass
 * (BASIS extract.lock_vocabulary says whether that pass gave the same set).  The rows are the same in
 * both modes.
 */
#define KZ_FACT_LOCKOP_FIELDS(X, T)                                                  \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, site, )       /* the call that leads to the primitive, or KZ_NONE32 */ \
  X(T, uint32_t, next_fn, )                                                          \
  X(T, uint32_t, depth, )                                                            \
  X(T, uint8_t, kind, )        /* KZ_LOCKOP_KIND */                                  \
  X(T, uint8_t, mode, )        /* KZ_LOCKOP_MODE */                                  \
  X(T, uint16_t, flags, )      /* KZ_LOCKOP_FLAG */                                  \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, int64_t, success, )     /* RESULT_CONDITIONAL: the value returned when it holds the lock */ \
  X(T, kz_str, ident, )                                                              \
  X(T, kz_str, may, )                                                                \
  X(T, kz_str, primitive, )                                                          \
  X(T, kz_str, chain, )

/*
 * LOCK_IDS (revision 8, F5).  One lock identity that a LOCKSET row holds, as an expression over the
 * parameters of the function the row is in: `$k` parameter k, `&x` the address of x, `x.f`, `x[i]`,
 * `p->f`, a global object by name, `?(why)` what the analysis does not follow.  `status`: STATIC
 * INSTANCE when the expression has no parameter and no unknown part and every value it reads is a
 * static instance -- a field of a global object (`location`, `object`) whose writer set is complete
 * under the per-field rule of FIELD_STATUS and has at most one writer instance, an instance that
 * runs at most once (`writers`; none: the static initial value) -- so that two rows holding this
 * identity hold one mutex (under the declared assumption that the lock operations read the value
 * after that write); NOT_STATIC when a value it reads is not one (`why`); PARAMETER_RELATIVE when it
 * names a parameter (the callers decide); UNDECIDABLE when a part is unknown.  `may` as in LOCK_OPS.
 * Two rows whose identities differ, or whose identity is not a static instance, are not shown to
 * hold the same lock: a field-based `may` location shared by them proves nothing.
 */
#define KZ_FACT_LOCKID_FIELDS(X, T)                                                  \
  X(T, uint32_t, writers, )    /* STATIC check: the writer instances of `location` */ \
  X(T, uint8_t, kind, )        /* KZ_LOCKID_KIND */                                  \
  X(T, uint8_t, status, )      /* KZ_LOCKID_STATUS */                                \
  X(T, uint16_t, flags, )      /* KZ_LOCKID_FLAG */                                  \
  X(T, kz_str, text, )                                                               \
  X(T, kz_str, may, )                                                                \
  X(T, kz_str, location, )     /* the field whose value it reads ("Type.field"), or empty */ \
  X(T, kz_str, object, )                                                             \
  X(T, kz_str, why, )

/*
 * LOCKSET (revision 8, F5).  The locks held at one call (`site`), field or global read (`read`, a
 * READS row) or typed write (`type`, `field`: a FUNC_WRITES event, every site) of a body, relative to
 * the entry of its function (`fn`; what its callers hold is in the LOCKSET rows of the calls to it):
 * the intersection over the paths of the forward analysis of LOCK_OPS (must), `n_held` locks,
 * `held_root` their LOCK_IDS indices (a SEQ of constants in FACT_EXPRS), `opener_root` the calls
 * that acquired them on some path (a SEQ of call sites).  `failure-path`: the statement is reached
 * only on a branch an acquisition takes when it fails (the declared assumption of LOCK_OPS reads it
 * as not reached); the locks are those held on that branch.  `unknown-release`: on some path an
 * unresolved indirect call or a call into the recursive component was made after an acquisition (read
 * as releasing every lock).  `no-cfg`: the body has no control-flow graph (nothing is held).
 */
#define KZ_FACT_LOCKSET_FIELDS(X, T)                                                 \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, site, )       /* CALL: the call, else KZ_NONE32 */                  \
  X(T, uint32_t, read, )       /* READ: READS index, else KZ_NONE32 */               \
  X(T, uint32_t, n_held, )                                                           \
  X(T, uint32_t, held_root, )  /* FACT_EXPRS: SEQ of LOCK_IDS indices, or KZ_NONE32 */ \
  X(T, uint32_t, opener_root, ) /* FACT_EXPRS: SEQ of call sites, or KZ_NONE32 */    \
  X(T, uint32_t, first, )      /* LOCK_IDS index of the first lock held, or KZ_NONE32 */ \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, fh, )                                                               \
  X(T, uint8_t, kind, )        /* KZ_LOCKSET_KIND */                                 \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )      /* KZ_LOCKSET_FLAG */                                 \
  X(T, uint32_t, rsv1, )                                                             \
  X(T, kz_str, type, )         /* WRITE: the location written */                     \
  X(T, kz_str, field, )                                                              \
  X(T, kz_str, file, )

/*
 * FIELD_STATUS (revision 8, SPEC 4.2 writer-set completeness, rules that do not depend on the
 * targets).  One row per field (type, field) of every record type a READS row names (all fields of
 * those types), from an extraction of its own whose targets are exactly those fields, so that the
 * row of a field is the same whatever targets the run itself has.  The writers (direct, data
 * function, out-pointer, initial values) and the A-TYPE hits are those of WRITERS / FACT_ATYPE for
 * that target; a hit is about the field (an address escape of the field, a store through a union
 * member of its name) or about the type as a whole (a cast, a union that holds the type, an
 * unknown effect).  `status_field` (per-field rule): complete when no hit is about the field or the
 * type as a whole -- an untyped store reaches the field only through such an exposure (a pointer to
 * another member does not reach it: C11 6.5.6 p8, the member is an array of one element);
 * `status_type` (type-level conservative rule): complete when, besides, no hit is about another
 * field of the type, or no untyped write site is in the program.  The old rule of FACT_LOCS (an
 * untyped write counts for every target of a type exposed by any of its targets) depends on which
 * other fields are targets; neither of these does.  `n_instances`: the writer instances (a direct
 * or data-function write site, an out-pointer call), `once` whether the field has at most one and
 * it runs at most once (outside every loop, in a function that is not address-taken, not a spawn
 * entry, not recursive, and has at most one call site, outside every loop, in such a function).
 */
#define KZ_FACT_FSTATUS_FIELDS(X, T)                                                 \
  X(T, uint32_t, n_writers, )                                                        \
  X(T, uint32_t, n_direct, )                                                         \
  X(T, uint32_t, n_data_fn, )                                                        \
  X(T, uint32_t, n_outptr, )                                                         \
  X(T, uint32_t, n_init, )                                                           \
  X(T, uint32_t, n_instances, )                                                      \
  X(T, uint32_t, own_escape, )                                                       \
  X(T, uint32_t, own_cast, )                                                         \
  X(T, uint32_t, own_union, )                                                        \
  X(T, uint32_t, own_unknown, )                                                      \
  X(T, uint32_t, sibling_hits, )                                                     \
  X(T, uint32_t, untyped, )    /* untyped write sites in the program */              \
  X(T, uint8_t, status_field, ) /* KZ_LOC_STATUS */                                  \
  X(T, uint8_t, status_type, )  /* KZ_LOC_STATUS */                                  \
  X(T, uint8_t, once, )        /* KZ_FSTATUS_ONCE */                                 \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )      /* KZ_FSTATUS_FLAG */                                 \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, kz_str, type, )                                                               \
  X(T, kz_str, field, )                                                              \
  X(T, kz_str, writer_fns, )   /* the functions of the writers, "; " */

/*
 * AWAIT_L1 (revision 8, SPEC 4.3 await, the L1 (i) rule whose relevant locations do not depend on the
 * targets).  One row per poll loop (`loop`).  The relevant locations (`relevant_root`, a SEQ of loads
 * in FACT_EXPRS) are the fields and globals its predicate reads directly (the base of a cursor over
 * an array of known size excluded, as in FACT_LOOPS) and those the functions the predicate calls read,
 * with the functions they call (READS rows; `via` the ones only those give).  L1 (i) under each
 * completeness rule of FIELD_STATUS (`l1_mod_field`, `l1_mod_type`): FAILS when a writer of a relevant
 * field is in the loop body or in a function the body calls (directly or through resolved indirect
 * calls); else UNDECIDED when a relevant field's writer set is not complete under that rule, a
 * relevant location is a global variable (no writer-set rule in this revision), or the body reaches
 * an unresolved indirect call; else HOLDS.  L1 (ii) (`l1_read`): ONE_LOCK_REGION when every read of
 * the predicate in the body (and every call of a function the predicate calls) holds one lock
 * acquired at one call (LOCKSET; `lock` its LOCK_IDS row, `lock-static` when that identity is a
 * static instance); ATOMIC_READ as in FACT_LOOPS; else FAILS.  `cert_field` / `cert_type`: AWAIT when
 * the predicate is recognised (FACT_LOOPS) and L1 (ii) and L1 (i) hold under that rule, else
 * STAR_CLOSED (`note_field`, `note_type` why).  The certificate-view rule of FACT_LOOPS is unchanged.
 */
#define KZ_FACT_AWAIT_FIELDS(X, T)                                                   \
  X(T, uint32_t, loop, )       /* the loop site */                                   \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, lock, )       /* LOCK_IDS index, or KZ_NONE32 */                    \
  X(T, uint32_t, relevant_root, ) /* FACT_EXPRS: SEQ of loads, or KZ_NONE32 */       \
  X(T, uint32_t, n_relevant, )                                                       \
  X(T, uint32_t, n_via, )                                                            \
  X(T, uint8_t, l1_read, )     /* KZ_AWAIT_READ */                                   \
  X(T, uint8_t, l1_mod_field, ) /* KZ_AWAIT_MOD */                                   \
  X(T, uint8_t, l1_mod_type, ) /* KZ_AWAIT_MOD */                                    \
  X(T, uint8_t, cert_field, )  /* KZ_LOOP_CERT */                                    \
  X(T, uint8_t, cert_type, )   /* KZ_LOOP_CERT */                                    \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )      /* KZ_AWAIT_FLAG */                                   \
  X(T, kz_str, via, )          /* the relevant locations a called function gives, "; " */ \
  X(T, kz_str, note_field, )                                                         \
  X(T, kz_str, note_type, )

/*
 * LOCK_PATHS (revision 9, F5 event order and entry paths).  One row per (LOCKSET event `row`, fact),
 * read on the same control-flow graph and the same call effects as LOCKSET:
 *   AFTER_SPAWN    the event `row` of `fn` is reachable on the control-flow graph of `fn` from the
 *                  point after the spawn call `site` of `fn` (a call flagged spawn-op), through any
 *                  branch and any back edge; the spawn call itself is such an event when a loop leads
 *                  back to it.  No branch is pruned, so the set is an over-approximation of the
 *                  events that run after the spawn;
 *   ENTRY_RELEASE  on some path from the entry of `fn` to the event `row`, a call releases a lock that
 *                  `fn` did not acquire (a release of the caller's lock, on every path or on some), may
 *                  release a held lock of an identity that is not a static instance (a pair a callee
 *                  acquires and releases itself), or has a lock effect the analysis cannot follow (an
 *                  unresolved indirect call, a call into the recursive component); `site` is the first
 *                  such call by call key.  A lock the caller of `fn` holds at its call is held at the
 *                  event only when the event has no such row.
 * `flags` is reserved (zero).
 */
#define KZ_FACT_LOCKPATH_FIELDS(X, T)                                                \
  X(T, uint32_t, fn, )                                                               \
  X(T, uint32_t, row, )        /* LOCKSET index of the event */                      \
  X(T, uint32_t, site, )       /* AFTER_SPAWN: the spawn call; ENTRY_RELEASE: the call */ \
  X(T, uint8_t, kind, )        /* KZ_LOCKPATH_KIND */                                \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, flags, )

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
  R(kz_fact_fwrite, KZ_FACT_FWRITE_FIELDS)        \
  R(kz_fact_p0, KZ_FACT_P0_FIELDS)                \
  R(kz_fact_opderiv, KZ_FACT_OPDERIV_FIELDS)      \
  R(kz_fact_spawnw, KZ_FACT_SPAWNW_FIELDS)        \
  R(kz_fact_read, KZ_FACT_READ_FIELDS)            \
  R(kz_fact_return, KZ_FACT_RETURN_FIELDS)        \
  R(kz_fact_retconst, KZ_FACT_RETCONST_FIELDS)    \
  R(kz_fact_loopexit, KZ_FACT_LOOPEXIT_FIELDS)    \
  R(kz_fact_argflow, KZ_FACT_ARGFLOW_FIELDS)      \
  R(kz_fact_pflow, KZ_FACT_PFLOW_FIELDS)          \
  R(kz_fact_ptrfield, KZ_FACT_PTRFIELD_FIELDS)    \
  R(kz_fact_gimage, KZ_FACT_GIMAGE_FIELDS)        \
  R(kz_fact_lockop, KZ_FACT_LOCKOP_FIELDS)        \
  R(kz_fact_lockid, KZ_FACT_LOCKID_FIELDS)        \
  R(kz_fact_lockset, KZ_FACT_LOCKSET_FIELDS)      \
  R(kz_fact_fstatus, KZ_FACT_FSTATUS_FIELDS)      \
  R(kz_fact_await, KZ_FACT_AWAIT_FIELDS)          \
  R(kz_fact_lockpath, KZ_FACT_LOCKPATH_FIELDS)

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
  ITEM(P, NOT_SHARED, 8, "not-a-shared-location") ITEM(P, WALK_BOUND, 9, "walk-bound")

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
 * could not be closed (an unresolved indirect call or an untyped write is reachable);
 * IMAGE_ASSUMED: a table walk whose path, table or elements are not const was unrolled under the
 * declared assumption that their static-initializer image is their value when the walk runs (no
 * typed write of them was found; the assumption is a BASIS item with policy declared). */
#define KZ_ENUM_LOOP_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, DELAY_DIRECT, 0x1, "delay-in-body") ITEM(P, L1_MOD, 0x2, "L1-i")           \
  ITEM(P, L1_READ, 0x4, "L1-ii") ITEM(P, MOD_INCOMPLETE, 0x8, "mod-incomplete")  \
  ITEM(P, IMAGE_ASSUMED, 0x10, "static-image-assumed")

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
  ITEM(P, UNION, 4, "union") ITEM(P, UNKNOWN_EFFECT, 5, "unknown-effect")

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
  ITEM(P, IN_MACRO, 0x20, "in-macro") ITEM(P, PARTIAL, 0x40, "partial-write")

#define KZ_ENUM_CALL_TARGET_KIND(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, INDIRECT, 1, "indirect-call")                     \
  ITEM(P, SPAWN_ENTRY, 2, "spawn-entry")

#define KZ_ENUM_CALL_TARGET_REASON(ITEM, P)                                               \
  ITEM(P, RESOLVED, 0, "resolved") ITEM(P, NO_FLOW, 1, "no-function-flows-to-it")   \
  ITEM(P, UNKNOWN_SOURCE, 2, "unknown-source") ITEM(P, CAST_FROM_DATA, 3, "cast-from-data") \
  ITEM(P, INCOMPLETE_SOURCE, 4, "incomplete-source")

#define KZ_ENUM_IMAGE_KIND(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, INT, 1, "int") ITEM(P, STRING, 2, "string")       \
  ITEM(P, ADDR, 3, "address") ITEM(P, FUNC, 4, "function") ITEM(P, NULLPTR, 5, "null") \
  ITEM(P, NONCONST, 6, "non-constant") ITEM(P, ZERO_FILL, 7, "zero-fill")

#define KZ_ENUM_LOC_STATUS(ITEM, P)                                                  \
  ITEM(P, UNCHECKED, 0, "unchecked") ITEM(P, COMPLETE, 1, "writer-set-complete")     \
  ITEM(P, DISABLED, 2, "DISABLED")

/* CONDITIONAL (revision 5): the operation is a derived terminal operation that does not end the
 * thread or the process on every path (OP_DERIV mode noreturn or may; the rows give the guards). */
#define KZ_ENUM_TERMINAL_FLAGS(ITEM, P) /* set */                                    \
  ITEM(P, PROCESS_TERMINAL, 0x1, "process-terminal") ITEM(P, IN_LOOP, 0x2, "in-loop") \
  ITEM(P, UNGUARDED, 0x4, "unguarded") ITEM(P, CONDITIONAL, 0x8, "conditional")

#define KZ_ENUM_P0_KIND(ITEM, P)                                                     \
  ITEM(P, NONE, 0, "none") ITEM(P, THREAD_CREATE, 1, "thread-create")                \
  ITEM(P, MUTEX_ACQUIRE, 2, "mutex-acquire") ITEM(P, MUTEX_RELEASE, 3, "mutex-release") \
  ITEM(P, SEM_WAIT, 4, "semaphore-wait") ITEM(P, SEM_POST, 5, "semaphore-post")      \
  ITEM(P, COND_WAIT, 6, "condition-wait") ITEM(P, COND_SIGNAL, 7, "condition-signal") \
  ITEM(P, SLEEP, 8, "sleep") ITEM(P, THREAD_EXIT, 9, "thread-exit")                  \
  ITEM(P, PROCESS_EXIT, 10, "process-exit") ITEM(P, THREAD_SELF, 11, "thread-self")  \
  ITEM(P, TSD_GET, 12, "thread-specific-get") ITEM(P, TSD_SET, 13, "thread-specific-set") \
  ITEM(P, DL_OPEN, 14, "dynamic-load") ITEM(P, DL_SYM, 15, "symbol-lookup")          \
  ITEM(P, DATA_FN, 16, "data-function") ITEM(P, TSD_KEY_CREATE, 17, "thread-specific-key-create") \
  ITEM(P, TSD_KEY_DELETE, 18, "thread-specific-key-delete") ITEM(P, CLEANUP_PUSH, 19, "cleanup-push") \
  ITEM(P, CLEANUP_POP, 20, "cleanup-pop")

/* CONDITIONAL: the call may return without doing it (try, timed); SHADOWED: the program defines a
 * function of this name; VOCABULARY: the entry is an operation of the profile-free run. */
#define KZ_ENUM_P0_FLAGS(ITEM, P) /* set */                                          \
  ITEM(P, CONDITIONAL, 0x1, "conditional") ITEM(P, SHADOWED, 0x2, "shadowed")        \
  ITEM(P, VOCABULARY, 0x4, "vocabulary")

#define KZ_ENUM_OPDERIV_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, DELAY, 1, "delay") ITEM(P, TERMINAL, 2, "terminal")

#define KZ_ENUM_OPDERIV_MODE(ITEM, P)                                                \
  ITEM(P, NONE, 0, "none") ITEM(P, MUST, 1, "must") ITEM(P, NORETURN, 2, "noreturn") \
  ITEM(P, MAY, 3, "may")

/* PROCESS_TERMINAL / THREAD_TERMINAL: the terminal chains of the function end at process-exit /
 * thread-exit primitives (both set: some of each); INDIRECT: the call of the row is an indirect
 * call with resolved targets; NO_CFG: no control-flow graph of the function was built, so it is
 * at most `may`. */
#define KZ_ENUM_OPDERIV_FLAGS(ITEM, P) /* set */                                     \
  ITEM(P, VOCABULARY, 0x1, "vocabulary") ITEM(P, PROCESS_TERMINAL, 0x2, "process-terminal") \
  ITEM(P, THREAD_TERMINAL, 0x4, "thread-terminal") ITEM(P, INDIRECT, 0x8, "indirect") \
  ITEM(P, NO_CFG, 0x10, "no-cfg")

#define KZ_ENUM_SPAWNW_KIND(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, ENTRY_ARG, 1, "entry-argument")                   \
  ITEM(P, CALLED_BY_ENTRY, 2, "called-by-entry") ITEM(P, FIXED_ENTRY, 3, "fixed-entry") \
  ITEM(P, SCRIPT_ENTRY, 4, "script-entry")

#define KZ_ENUM_SPAWNW_FLAGS(ITEM, P) /* set */ \
  ITEM(P, VOCABULARY, 0x1, "vocabulary") ITEM(P, NOT_IN_CLOSURE, 0x2, "not-in-closure")

#define KZ_ENUM_READ_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, FIELD, 1, "field") ITEM(P, GLOBAL, 2, "global")

#define KZ_ENUM_READ_ROOT(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, GLOBAL, 1, "global") ITEM(P, PARAM, 2, "param")   \
  ITEM(P, LOCAL, 3, "local") ITEM(P, CALL, 4, "call-result") ITEM(P, OTHER, 5, "other")

#define KZ_ENUM_READ_USE(ITEM, P) /* set */                                          \
  ITEM(P, BRANCH, 0x1, "branch") ITEM(P, RETURN, 0x2, "return")                      \
  ITEM(P, OUT_STORE, 0x4, "out-store") ITEM(P, CALL_ARG, 0x8, "call-argument")       \
  ITEM(P, SUBSCRIPT, 0x10, "subscript") ITEM(P, STORE, 0x20, "store")                \
  ITEM(P, DEREF, 0x40, "dereference") ITEM(P, CALLEE, 0x80, "callee")                \
  ITEM(P, OTHER, 0x100, "other")

/* THROUGH_POINTER: the access path dereferences a pointer (`->`, `*`, a subscript of a pointer);
 * ARRAY_CONTENTS: an array that decays to a pointer; RECORD: a whole record is read (copied);
 * INDEX_UNFOLDED: a subscript local with a single copy definition before the read is written as the
 * value of that definition (the rule of RETURNS). */
#define KZ_ENUM_READ_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, THROUGH_POINTER, 0x1, "through-pointer") ITEM(P, ARRAY_CONTENTS, 0x2, "array-contents") \
  ITEM(P, IN_MACRO, 0x4, "in-macro") ITEM(P, RECORD, 0x8, "whole-record")            \
  ITEM(P, INDEX_UNFOLDED, 0x10, "index-unfolded")

#define KZ_ENUM_RET_VALUE(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, CONST, 1, "const") ITEM(P, FIELD, 2, "field")     \
  ITEM(P, GLOBAL, 3, "global") ITEM(P, CALL, 4, "call-result") ITEM(P, PARAM, 5, "param") \
  ITEM(P, LOCAL, 6, "local") ITEM(P, ADDR, 7, "address") ITEM(P, STRING, 8, "string") \
  ITEM(P, EXPR, 9, "expression")

/* NO_CFG: no control-flow graph of the body was built, so a returned local with several definitions
 * is left as it is; ONLY_CONSTANTS (RET_CONSTS): every return value of the function is a constant. */
#define KZ_ENUM_RET_FLAGS(ITEM, P) /* set */                                         \
  ITEM(P, UNFOLDED, 0x1, "unfolded") ITEM(P, FROM_DEF, 0x2, "from-definition")       \
  ITEM(P, REDEFINED, 0x4, "redefined-later") ITEM(P, UNDEF_PATH, 0x8, "undefined-on-a-path") \
  ITEM(P, IN_LOOP, 0x10, "in-loop") ITEM(P, IN_MACRO, 0x20, "in-macro")              \
  ITEM(P, NO_CFG, 0x40, "no-cfg") ITEM(P, ONLY_CONSTANTS, 0x80, "only-constants")

#define KZ_ENUM_LOOPEXIT_KIND(ITEM, P)                                               \
  ITEM(P, NONE, 0, "none") ITEM(P, LOOP, 1, "loop") ITEM(P, COND, 2, "condition")    \
  ITEM(P, BREAK, 3, "break") ITEM(P, RETURN, 4, "return") ITEM(P, GOTO, 5, "goto")

/* AS_IN_LOOPS (LOOP): FACT_LOOPS carries the same exit and mod-set trees; NEVER (EXIT): a constant
 * false condition; UNCONDITIONAL (EXIT): taken whenever it is reached; NO_EXIT (LOOP): no exit can
 * be taken. */
#define KZ_ENUM_LOOPEXIT_FLAGS(ITEM, P) /* set */                                    \
  ITEM(P, AS_IN_LOOPS, 0x1, "as-in-loops") ITEM(P, NEVER, 0x2, "never-taken")        \
  ITEM(P, UNCONDITIONAL, 0x4, "unconditional") ITEM(P, NO_EXIT, 0x8, "no-exit")

#define KZ_ENUM_ARGSRC(ITEM, P)                                                      \
  ITEM(P, NONE, 0, "none") ITEM(P, CONST, 1, "const") ITEM(P, STRING, 2, "string")   \
  ITEM(P, FUNCTION, 3, "function") ITEM(P, FIELD, 4, "field") ITEM(P, GLOBAL, 5, "global") \
  ITEM(P, ELEMENT, 6, "element") ITEM(P, DEREF, 7, "dereference") ITEM(P, ADDRESS, 8, "address") \
  ITEM(P, PARAM, 9, "param") ITEM(P, CALL, 10, "call-result") ITEM(P, LOCAL, 11, "local") \
  ITEM(P, EXPR, 12, "expression")

#define KZ_ENUM_ARGBASE(ITEM, P)                                                     \
  ITEM(P, NONE, 0, "none") ITEM(P, GLOBAL, 1, "global") ITEM(P, GLOBAL_ELEMENT, 2, "global-array-element") \
  ITEM(P, LOCAL, 3, "local") ITEM(P, PARAM, 4, "parameter") ITEM(P, LOCAL_POINTER, 5, "local-pointer") \
  ITEM(P, FIELD_POINTER, 6, "pointer-field") ITEM(P, GLOBAL_POINTER, 7, "global-pointer") \
  ITEM(P, CALL_RESULT, 8, "call-result") ITEM(P, OTHER, 9, "other")

/* ARRAY_CONTENTS: an array that decays to a pointer (its contents are passed by address);
 * POINTER_CONVERSION: a conversion between pointer types on the chain (the address is kept);
 * BASE_VIA_LOCAL: the base pointer is a local followed through its single copy definition;
 * MULTI_LEVEL: the access path dereferences more than one pointer (the base is the step nearest
 * to the root). */
#define KZ_ENUM_ARGFLOW_FLAGS(ITEM, P) /* set */                                     \
  ITEM(P, THROUGH_POINTER, 0x1, "through-pointer") ITEM(P, ARRAY_CONTENTS, 0x2, "array-contents") \
  ITEM(P, POINTER_CONVERSION, 0x4, "pointer-conversion") ITEM(P, BASE_VIA_LOCAL, 0x8, "base-via-local") \
  ITEM(P, IN_MACRO, 0x10, "in-macro") ITEM(P, VARIADIC, 0x20, "variadic")            \
  ITEM(P, NULLPTR, 0x40, "null") ITEM(P, INDEX_UNFOLDED, 0x80, "index-unfolded")     \
  ITEM(P, MULTI_LEVEL, 0x100, "multi-level")

#define KZ_ENUM_PFLOW_KIND(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, CALL_ARG, 1, "call-argument") ITEM(P, COMPARE, 2, "comparison") \
  ITEM(P, RETURN, 3, "return") ITEM(P, STORE, 4, "store") ITEM(P, SUBSCRIPT, 5, "subscript") \
  ITEM(P, OUT_STORE, 6, "out-store") ITEM(P, OPEN, 7, "open")

/* RESULT_TESTED (CALL_ARG): the result of the call is tested; THROUGH_POINTER_PARAM (STORE): the
 * destination is reached through a pointer parameter; TRUTH_TEST, SWITCH (COMPARISON): the value is
 * a condition tested for truth, the controlling expression of a switch. */
#define KZ_ENUM_PFLOW_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, MAY, 0x1, "may") ITEM(P, INDIRECT, 0x2, "indirect")                        \
  ITEM(P, NO_BODY, 0x4, "callee-without-body") ITEM(P, UNRESOLVED, 0x8, "unresolved") \
  ITEM(P, VARIADIC, 0x10, "variadic") ITEM(P, DEPTH_BOUND, 0x20, "depth-bound")      \
  ITEM(P, SIZE_BOUND, 0x40, "size-bound") ITEM(P, TRUTH_TEST, 0x80, "truth-test")    \
  ITEM(P, SWITCH, 0x100, "switch") ITEM(P, THROUGH_POINTER_PARAM, 0x200, "through-pointer-parameter") \
  ITEM(P, RESULT_TESTED, 0x400, "result-tested") ITEM(P, IN_LOOP, 0x800, "in-loop")

#define KZ_ENUM_PTRF_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, ADDRESS_WRITE, 1, "address-write")                \
  ITEM(P, STATIC_INIT, 2, "static-initializer") ITEM(P, READ, 3, "read")

#define KZ_ENUM_PTRF_STATUS(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, RESOLVED, 1, "resolved") ITEM(P, MAY, 2, "may")   \
  ITEM(P, UNRESOLVED, 3, "needs-field-points-to")

/* WRITE_BEFORE_READ (READ): the address write precedes the read in program order (see PTR_FIELDS);
 * SAME_ROOT: the write and the read name the field of the same global; FIELD_BASED: the write's
 * member is not rooted at a global, so it is matched by the field only; OTHER_WRITES: other typed
 * writes of the field remain; EARLIER_WRITES_OVERWRITTEN (ADDRESS_WRITE): writes of the field in
 * the writer before it on every path. */
#define KZ_ENUM_PTRF_FLAGS(ITEM, P) /* set */                                        \
  ITEM(P, WRITE_BEFORE_READ, 0x1, "write-before-read") ITEM(P, SAME_ROOT, 0x2, "same-root") \
  ITEM(P, FIELD_BASED, 0x4, "field-based") ITEM(P, OTHER_WRITES, 0x8, "other-writes") \
  ITEM(P, IN_LOOP, 0x10, "in-loop") ITEM(P, MULTI_LEVEL, 0x20, "multi-level")        \
  ITEM(P, INDEX_RANGE, 0x40, "index-from-counted-loop") ITEM(P, INDEX_UPPER_BOUND, 0x80, "loop-may-exit-early") \
  ITEM(P, EARLIER_OVERWRITTEN, 0x100, "earlier-writes-overwritten") ITEM(P, CONST_INDEX, 0x200, "constant-index")

#define KZ_ENUM_GIMAGE_REACH(ITEM, P) /* set */ \
  ITEM(P, ARGUMENT, 0x1, "argument") ITEM(P, PTR_FIELD, 0x2, "pointer-field")

#define KZ_ENUM_GIMAGE_FLAGS(ITEM, P) /* set */                                      \
  ITEM(P, CONST_OBJECT, 0x1, "const-object") ITEM(P, NO_INITIALIZER, 0x2, "zero-initialized") \
  ITEM(P, TRUNCATED, 0x4, "image-truncated") ITEM(P, OBJECT_WRITTEN, 0x8, "object-written") \
  ITEM(P, FIELD_WRITTEN, 0x10, "field-written") ITEM(P, RECORD_WRITTEN, 0x20, "record-written") \
  ITEM(P, FROM_MACRO, 0x40, "from-macro")

/* LOCK_OPS (revision 8). */
#define KZ_ENUM_LOCKOP_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, ACQUIRE, 1, "acquire") ITEM(P, RELEASE, 2, "release") ITEM(P, WAIT, 3, "wait")

#define KZ_ENUM_LOCKOP_MODE(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, EVERY_RETURN, 1, "every-return")                  \
  ITEM(P, RESULT, 2, "result-conditional") ITEM(P, TRY, 3, "try") ITEM(P, CONDITIONAL, 4, "conditional")

#define KZ_ENUM_LOCKOP_FLAGS(ITEM, P) /* set */                                      \
  ITEM(P, PRIMITIVE, 0x1, "primitive-op") ITEM(P, VOCABULARY, 0x2, "vocabulary")     \
  ITEM(P, ASSUMES_SUCCESS, 0x4, "assumes-success") ITEM(P, INDIRECT, 0x8, "indirect") \
  ITEM(P, NO_CFG, 0x10, "no-cfg") ITEM(P, PARAM_IDENT, 0x20, "parameter-identity")   \
  ITEM(P, UNKNOWN_IDENT, 0x40, "unknown-identity")

/* LOCK_IDS (revision 8). */
#define KZ_ENUM_LOCKID_KIND(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, CLOSED, 1, "closed") ITEM(P, PARAM, 2, "parameter-relative") \
  ITEM(P, UNKNOWN, 3, "unknown")

#define KZ_ENUM_LOCKID_STATUS(ITEM, P)                                               \
  ITEM(P, NONE, 0, "none") ITEM(P, STATIC, 1, "static-instance") ITEM(P, NOT_STATIC, 2, "not-static") \
  ITEM(P, PARAM, 3, "parameter-relative") ITEM(P, UNDECIDABLE, 4, "undecidable")

/* ADDRESS_OF_GLOBAL: the mutex is at a fixed address (a global object, or an element of a global
 * array at a static index); READS_VALUE: the identity reads a value (a handle); TYPE_EXPOSED: a value
 * location's writer set is not complete under the type-level rule of FIELD_STATUS; INITIAL_VALUE: a
 * value location has no writer instance (its static initial value). */
#define KZ_ENUM_LOCKID_FLAGS(ITEM, P) /* set */                                      \
  ITEM(P, ADDRESS_OF_GLOBAL, 0x1, "address-of-global") ITEM(P, READS_VALUE, 0x2, "reads-value") \
  ITEM(P, TYPE_EXPOSED, 0x4, "type-level-exposed") ITEM(P, INITIAL_VALUE, 0x8, "initial-value")

/* LOCKSET (revision 8). */
#define KZ_ENUM_LOCKSET_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, CALL, 1, "call") ITEM(P, READ, 2, "read") ITEM(P, WRITE, 3, "write")

#define KZ_ENUM_LOCKSET_FLAGS(ITEM, P) /* set */                                     \
  ITEM(P, FAILURE_PATH, 0x1, "failure-path") ITEM(P, UNKNOWN_RELEASE, 0x2, "unknown-release") \
  ITEM(P, NO_CFG, 0x4, "no-cfg") ITEM(P, NOT_REACHED, 0x8, "not-reached")

/* FIELD_STATUS (revision 8). */
#define KZ_ENUM_FSTATUS_ONCE(ITEM, P)                                                \
  ITEM(P, NONE, 0, "unchecked") ITEM(P, NO_WRITER, 1, "no-writer-instance")          \
  ITEM(P, ONCE, 2, "one-instance-once") ITEM(P, MORE, 3, "not-once")

#define KZ_ENUM_FSTATUS_FLAGS(ITEM, P) /* set */                                     \
  ITEM(P, SIBLING_EXPOSED, 0x1, "sibling-exposed") ITEM(P, UNTYPED_IN_PROGRAM, 0x2, "untyped-in-program") \
  ITEM(P, WRITER_IN_LOOP, 0x4, "writer-in-loop") ITEM(P, WRITER_NOT_ONCE, 0x8, "writer-function-not-once") \
  ITEM(P, SEVERAL_INSTANCES, 0x10, "several-instances") ITEM(P, ARRAY_ELEMENTS, 0x20, "array-element-stores-not-collected")

/* AWAIT_L1 (revision 8). */
#define KZ_ENUM_AWAIT_READ(ITEM, P)                                                  \
  ITEM(P, NONE, 0, "none") ITEM(P, ONE_LOCK, 1, "one-lock-region") ITEM(P, ATOMIC, 2, "atomic-read") \
  ITEM(P, FAILS, 3, "fails")

#define KZ_ENUM_AWAIT_MOD(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, HOLDS, 1, "holds") ITEM(P, FAILS, 2, "fails") ITEM(P, UNDECIDED, 3, "undecided")

#define KZ_ENUM_AWAIT_FLAGS(ITEM, P) /* set */                                       \
  ITEM(P, VIA_CALLEE, 0x1, "via-callee") ITEM(P, GLOBAL_VARIABLE, 0x2, "global-variable") \
  ITEM(P, UNRESOLVED_CALL, 0x4, "unresolved-call") ITEM(P, LOCK_STATIC, 0x8, "lock-static") \
  ITEM(P, NO_PREDICATE, 0x10, "no-predicate")

/* LOCK_PATHS (revision 9). */
#define KZ_ENUM_LOCKPATH_KIND(ITEM, P)                                               \
  ITEM(P, NONE, 0, "none") ITEM(P, AFTER_SPAWN, 1, "after-spawn") ITEM(P, ENTRY_RELEASE, 2, "entry-release")

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
  D(KZ_P0_KIND, KZ_ENUM_P0_KIND, 0)            \
  D(KZ_P0_FLAG, KZ_ENUM_P0_FLAGS, 1)           \
  D(KZ_OPDERIV_KIND, KZ_ENUM_OPDERIV_KIND, 0)  \
  D(KZ_OPDERIV_MODE, KZ_ENUM_OPDERIV_MODE, 0)  \
  D(KZ_OPDERIV_FLAG, KZ_ENUM_OPDERIV_FLAGS, 1) \
  D(KZ_SPAWNW_KIND, KZ_ENUM_SPAWNW_KIND, 0)    \
  D(KZ_SPAWNW_FLAG, KZ_ENUM_SPAWNW_FLAGS, 1)   \
  D(KZ_READ_KIND, KZ_ENUM_READ_KIND, 0)        \
  D(KZ_READ_ROOT, KZ_ENUM_READ_ROOT, 0)        \
  D(KZ_READ_USE, KZ_ENUM_READ_USE, 1)          \
  D(KZ_READ_FLAG, KZ_ENUM_READ_FLAGS, 1)       \
  D(KZ_RET_VALUE, KZ_ENUM_RET_VALUE, 0)        \
  D(KZ_RET_FLAG, KZ_ENUM_RET_FLAGS, 1)         \
  D(KZ_LOOPEXIT_KIND, KZ_ENUM_LOOPEXIT_KIND, 0) \
  D(KZ_LOOPEXIT_FLAG, KZ_ENUM_LOOPEXIT_FLAGS, 1) \
  D(KZ_ARGSRC, KZ_ENUM_ARGSRC, 0)              \
  D(KZ_ARGBASE, KZ_ENUM_ARGBASE, 0)            \
  D(KZ_ARGFLOW_FLAG, KZ_ENUM_ARGFLOW_FLAGS, 1) \
  D(KZ_PFLOW_KIND, KZ_ENUM_PFLOW_KIND, 0)      \
  D(KZ_PFLOW_FLAG, KZ_ENUM_PFLOW_FLAGS, 1)     \
  D(KZ_PTRF_KIND, KZ_ENUM_PTRF_KIND, 0)        \
  D(KZ_PTRF_STATUS, KZ_ENUM_PTRF_STATUS, 0)    \
  D(KZ_PTRF_FLAG, KZ_ENUM_PTRF_FLAGS, 1)       \
  D(KZ_GIMAGE_REACH, KZ_ENUM_GIMAGE_REACH, 1)  \
  D(KZ_GIMAGE_FLAG, KZ_ENUM_GIMAGE_FLAGS, 1)   \
  D(KZ_LOCKOP_KIND, KZ_ENUM_LOCKOP_KIND, 0)    \
  D(KZ_LOCKOP_MODE, KZ_ENUM_LOCKOP_MODE, 0)    \
  D(KZ_LOCKOP_FLAG, KZ_ENUM_LOCKOP_FLAGS, 1)   \
  D(KZ_LOCKID_KIND, KZ_ENUM_LOCKID_KIND, 0)    \
  D(KZ_LOCKID_STATUS, KZ_ENUM_LOCKID_STATUS, 0) \
  D(KZ_LOCKID_FLAG, KZ_ENUM_LOCKID_FLAGS, 1)   \
  D(KZ_LOCKSET_KIND, KZ_ENUM_LOCKSET_KIND, 0)  \
  D(KZ_LOCKSET_FLAG, KZ_ENUM_LOCKSET_FLAGS, 1) \
  D(KZ_FSTATUS_ONCE, KZ_ENUM_FSTATUS_ONCE, 0)  \
  D(KZ_FSTATUS_FLAG, KZ_ENUM_FSTATUS_FLAGS, 1) \
  D(KZ_AWAIT_READ, KZ_ENUM_AWAIT_READ, 0)      \
  D(KZ_AWAIT_MOD, KZ_ENUM_AWAIT_MOD, 0)        \
  D(KZ_AWAIT_FLAG, KZ_ENUM_AWAIT_FLAGS, 1)     \
  D(KZ_LOCKPATH_KIND, KZ_ENUM_LOCKPATH_KIND, 0) \
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
  F(kz_tu, fh, IDX, 0)                                \
  F(kz_fact_p0, fn, IDX, 0)                           \
  F(kz_fact_p0, out_param, IDX, 0)                    \
  F(kz_fact_p0, entry_param, IDX, 0)                  \
  F(kz_fact_p0, arg_param, IDX, 0)                    \
  F(kz_fact_p0, kind, ENUM, KZ_P0_KIND)               \
  F(kz_fact_p0, flags, FLAGS, KZ_P0_FLAG)             \
  F(kz_fact_opderiv, fn, IDX, 0)                      \
  F(kz_fact_opderiv, site, IDX, 0)                    \
  F(kz_fact_opderiv, next_fn, IDX, 0)                 \
  F(kz_fact_opderiv, guard_root, IDX, 0)              \
  F(kz_fact_opderiv, kind, ENUM, KZ_OPDERIV_KIND)     \
  F(kz_fact_opderiv, mode, ENUM, KZ_OPDERIV_MODE)     \
  F(kz_fact_opderiv, flags, FLAGS, KZ_OPDERIV_FLAG)   \
  F(kz_fact_spawnw, fn, IDX, 0)                       \
  F(kz_fact_spawnw, param, IDX, 0)                    \
  F(kz_fact_spawnw, site, IDX, 0)                     \
  F(kz_fact_spawnw, next_fn, IDX, 0)                  \
  F(kz_fact_spawnw, entry_fn, IDX, 0)                 \
  F(kz_fact_spawnw, call_site, IDX, 0)                \
  F(kz_fact_spawnw, kind, ENUM, KZ_SPAWNW_KIND)       \
  F(kz_fact_spawnw, flags, FLAGS, KZ_SPAWNW_FLAG)     \
  F(kz_fact_read, fn, IDX, 0)                         \
  F(kz_fact_read, loop, IDX, 0)                       \
  F(kz_fact_read, site, IDX, 0)                       \
  F(kz_fact_read, arg_index, IDX, 0)                  \
  F(kz_fact_read, index_root, IDX, 0)                 \
  F(kz_fact_read, guard_root, IDX, 0)                 \
  F(kz_fact_read, ctx_root, IDX, 0)                   \
  F(kz_fact_read, kind, ENUM, KZ_READ_KIND)           \
  F(kz_fact_read, root_kind, ENUM, KZ_READ_ROOT)      \
  F(kz_fact_read, use, FLAGS, KZ_READ_USE)            \
  F(kz_fact_read, flags, FLAGS, KZ_READ_FLAG)         \
  F(kz_fact_read, root_param, IDX, 0)                 \
  F(kz_fact_read, fh, IDX, 0)                         \
  F(kz_fact_return, fn, IDX, 0)                       \
  F(kz_fact_return, value_root, IDX, 0)               \
  F(kz_fact_return, guard_root, IDX, 0)               \
  F(kz_fact_return, def_guard_root, IDX, 0)           \
  F(kz_fact_return, cond_root, IDX, 0)                \
  F(kz_fact_return, site, IDX, 0)                     \
  F(kz_fact_return, loop, IDX, 0)                     \
  F(kz_fact_return, vkind, ENUM, KZ_RET_VALUE)        \
  F(kz_fact_return, flags, FLAGS, KZ_RET_FLAG)        \
  F(kz_fact_return, fh, IDX, 0)                       \
  F(kz_fact_retconst, fn, IDX, 0)                     \
  F(kz_fact_retconst, cond_root, IDX, 0)              \
  F(kz_fact_retconst, flags, FLAGS, KZ_RET_FLAG)      \
  F(kz_fact_loopexit, loop, IDX, 0)                   \
  F(kz_fact_loopexit, exit, IDX, 0)                   \
  F(kz_fact_loopexit, fn, IDX, 0)                     \
  F(kz_fact_loopexit, root, IDX, 0)                   \
  F(kz_fact_loopexit, aux_root, IDX, 0)               \
  F(kz_fact_loopexit, fh, IDX, 0)                     \
  F(kz_fact_loopexit, kind, ENUM, KZ_LOOPEXIT_KIND)   \
  F(kz_fact_loopexit, flags, FLAGS, KZ_LOOPEXIT_FLAG) \
  F(kz_fact_argflow, site, IDX, 0)                    \
  F(kz_fact_argflow, fn, IDX, 0)                      \
  F(kz_fact_argflow, param, IDX, 0)                   \
  F(kz_fact_argflow, chain_root, IDX, 0)              \
  F(kz_fact_argflow, index_root, IDX, 0)              \
  F(kz_fact_argflow, read, IDX, 0)                    \
  F(kz_fact_argflow, src, ENUM, KZ_ARGSRC)            \
  F(kz_fact_argflow, base, ENUM, KZ_ARGBASE)          \
  F(kz_fact_argflow, reason, ENUM, KZ_DERIVES_REASON) \
  F(kz_fact_argflow, flags, FLAGS, KZ_ARGFLOW_FLAG)   \
  F(kz_fact_pflow, fn, IDX, 0)                        \
  F(kz_fact_pflow, site, IDX, 0)                      \
  F(kz_fact_pflow, next_fn, IDX, 0)                   \
  F(kz_fact_pflow, next_param, IDX, 0)                \
  F(kz_fact_pflow, sink_fn, IDX, 0)                   \
  F(kz_fact_pflow, sink_site, IDX, 0)                 \
  F(kz_fact_pflow, expr_root, IDX, 0)                 \
  F(kz_fact_pflow, guard_root, IDX, 0)                \
  F(kz_fact_pflow, fh, IDX, 0)                        \
  F(kz_fact_pflow, kind, ENUM, KZ_PFLOW_KIND)         \
  F(kz_fact_pflow, flags, FLAGS, KZ_PFLOW_FLAG)       \
  F(kz_fact_ptrfield, fn, IDX, 0)                     \
  F(kz_fact_ptrfield, read, IDX, 0)                   \
  F(kz_fact_ptrfield, write, IDX, 0)                  \
  F(kz_fact_ptrfield, index_root, IDX, 0)             \
  F(kz_fact_ptrfield, guard_root, IDX, 0)             \
  F(kz_fact_ptrfield, fh, IDX, 0)                     \
  F(kz_fact_ptrfield, kind, ENUM, KZ_PTRF_KIND)       \
  F(kz_fact_ptrfield, status, ENUM, KZ_PTRF_STATUS)   \
  F(kz_fact_ptrfield, flags, FLAGS, KZ_PTRF_FLAG)     \
  F(kz_fact_gimage, fn, IDX, 0)                       \
  F(kz_fact_gimage, fh, IDX, 0)                       \
  F(kz_fact_gimage, kind, ENUM, KZ_IMAGE_KIND)        \
  F(kz_fact_gimage, reach, FLAGS, KZ_GIMAGE_REACH)    \
  F(kz_fact_gimage, flags, FLAGS, KZ_GIMAGE_FLAG)     \
  F(kz_fact_lockop, fn, IDX, 0)                       \
  F(kz_fact_lockop, site, IDX, 0)                     \
  F(kz_fact_lockop, next_fn, IDX, 0)                  \
  F(kz_fact_lockop, kind, ENUM, KZ_LOCKOP_KIND)       \
  F(kz_fact_lockop, mode, ENUM, KZ_LOCKOP_MODE)       \
  F(kz_fact_lockop, flags, FLAGS, KZ_LOCKOP_FLAG)     \
  F(kz_fact_lockid, kind, ENUM, KZ_LOCKID_KIND)       \
  F(kz_fact_lockid, status, ENUM, KZ_LOCKID_STATUS)   \
  F(kz_fact_lockid, flags, FLAGS, KZ_LOCKID_FLAG)     \
  F(kz_fact_lockset, fn, IDX, 0)                      \
  F(kz_fact_lockset, site, IDX, 0)                    \
  F(kz_fact_lockset, read, IDX, 0)                    \
  F(kz_fact_lockset, held_root, IDX, 0)               \
  F(kz_fact_lockset, opener_root, IDX, 0)             \
  F(kz_fact_lockset, first, IDX, 0)                   \
  F(kz_fact_lockset, fh, IDX, 0)                      \
  F(kz_fact_lockset, kind, ENUM, KZ_LOCKSET_KIND)     \
  F(kz_fact_lockset, flags, FLAGS, KZ_LOCKSET_FLAG)   \
  F(kz_fact_fstatus, status_field, ENUM, KZ_LOC_STATUS) \
  F(kz_fact_fstatus, status_type, ENUM, KZ_LOC_STATUS) \
  F(kz_fact_fstatus, once, ENUM, KZ_FSTATUS_ONCE)     \
  F(kz_fact_fstatus, flags, FLAGS, KZ_FSTATUS_FLAG)   \
  F(kz_fact_await, loop, IDX, 0)                      \
  F(kz_fact_await, fn, IDX, 0)                        \
  F(kz_fact_await, lock, IDX, 0)                      \
  F(kz_fact_await, relevant_root, IDX, 0)             \
  F(kz_fact_await, l1_read, ENUM, KZ_AWAIT_READ)      \
  F(kz_fact_await, l1_mod_field, ENUM, KZ_AWAIT_MOD)  \
  F(kz_fact_await, l1_mod_type, ENUM, KZ_AWAIT_MOD)   \
  F(kz_fact_await, cert_field, ENUM, KZ_LOOP_CERT)    \
  F(kz_fact_await, cert_type, ENUM, KZ_LOOP_CERT)     \
  F(kz_fact_await, flags, FLAGS, KZ_AWAIT_FLAG)       \
  F(kz_fact_lockpath, fn, IDX, 0)                     \
  F(kz_fact_lockpath, row, IDX, 0)                    \
  F(kz_fact_lockpath, site, IDX, 0)                   \
  F(kz_fact_lockpath, kind, ENUM, KZ_LOCKPATH_KIND)

#endif /* KZ_SCHEMA_KZ_FACTS_H */

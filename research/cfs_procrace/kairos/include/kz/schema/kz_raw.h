/*
 * kz_raw.h -- observation records: owned by K2 (the observer).
 *
 * Derived from SPEC 2.3 (kz_raw_ev) and 5.5 (the `.kzt` run container: SITES, RAW_EVENTS,
 * STRINGS, STACKS, LOSS, RUNMETA, PERTURBATION), 5.6 (loss accounting), 5.7 (identity
 * bootstrap) and 4.13 (perturbation accounting).  The layouts are the S1 layouts of the
 * perf-sample backend; the owner may revise them (bump the section's schema version when a
 * layout changes).
 *
 * Revision 2 of the initial K0 draft (K2, perf-sample backend):
 *   - SITES (kz_site) is version 2: `rsv1` became `line`, and `label`/`source` were added
 *     (the plan's name for the site and the source position it came from);
 *   - LOSS (kz_loss) is version 2: `t_ns` was added (where in time the loss was reported);
 *   - PERTURBATION (kz_perturbation): the reserved byte `rsv0` became `trap_account`
 *     (KZ_TRAP_ACCOUNT, 0 = not stated), and the cost class `ret-trampoline` and the kind
 *     `proc-capture` were added (review of 2026-10-16).  Offsets and sizes are unchanged and
 *     the value 0 means what every earlier container holds there, so the section stays at
 *     version 1 and run containers written before (env/logs/s1r2_p1.kzt) stay readable;
 *   - new sections: SITE_SLOTS, SIDE_EVENTS, THREADS, MAPS (run containers) and SYMBOLS,
 *     TARGETS (basis containers).
 *
 * Conventions of the perf-sample backend (SPEC 5.1, "zero-install bring-up backend"):
 *   - `site` is the id of the plan's site; the backend knows it from the perf event that
 *     produced the record (every record carries PERF_SAMPLE_IDENTIFIER), not from a cookie;
 *   - `tseq` is assigned by the decoder, per thread, in the order of the merged stream
 *     (timestamp, CPU, arrival).  The kernel assigns nothing here, so a gap in `tseq` is not
 *     a loss detector: loss is reported only by the kernel (LOSS: perf-record-lost,
 *     perf-throttle).  Per-thread program order is therefore evidence under the
 *     clock assumption A-CLK-PO (SPEC 5.4).  A timestamp order between DIFFERENT threads is
 *     never a happens-before edge (SPEC 5.8): counting tools may sort and count by it, as
 *     one-clock evidence that says what the clock showed, and must say so in their output;
 *     the checker never derives an HB edge from it;
 *   - `objseq` is 0 and OBJSEQ_VALID is clear: there is no kernel-side object token;
 *   - a[0..3] are filled by the site's slot descriptors (SITE_SLOTS).  A GENERIC site is read
 *     by role (the slot whose role is "event_id", ...).  Two engine functions read fixed
 *     positions, and the plan checker (kz::trace::check_site / check_plan) refuses a plan
 *     whose slots do not declare the required role at those positions:
 *       - the identity join reads the KZ_DECODER positions listed below;
 *       - entry/return matching reads a[3] with role "return_address" at both the ENTRY site
 *         (the word at the top of the stack) and the RET site (the sample's instruction
 *         pointer).
 *     An event with TRUNC set may hold 0 in any slot: it is never matched on a slot value.
 *
 * Not usable alone: include kz_schema.h.
 */
#ifndef KZ_SCHEMA_KZ_RAW_H
#define KZ_SCHEMA_KZ_RAW_H

#ifndef KZ_SCHEMA_KZ_SCHEMA_H
#error "include kz/schema/kz_schema.h, not this file"
#endif

enum kz_raw_section_id {
  KZ_SEC_SITES = 40,
  KZ_SEC_RAW_EVENTS = 41,
  KZ_SEC_STACKS = 43,
  KZ_SEC_LOSS = 44,
  KZ_SEC_RUNMETA = 45,
  KZ_SEC_PERTURBATION = 46,
  KZ_SEC_SITE_SLOTS = 50,
  KZ_SEC_SIDE_EVENTS = 51,
  KZ_SEC_THREADS = 52,
  KZ_SEC_MAPS = 53,
  KZ_SEC_SYMBOLS = 54,
  KZ_SEC_TARGETS = 55
};

#define KZ_RAW_SECTIONS(S)               \
  S(SITES, 40, kz_site, 2)               \
  S(RAW_EVENTS, 41, kz_raw_ev, 1)        \
  S(STACKS, 43, kz_stack_frame, 1)       \
  S(LOSS, 44, kz_loss, 2)                \
  S(RUNMETA, 45, kz_runmeta, 1)          \
  S(PERTURBATION, 46, kz_perturbation, 1) \
  S(SITE_SLOTS, 50, kz_site_slot, 1)     \
  S(SIDE_EVENTS, 51, kz_side_ev, 1)      \
  S(THREADS, 52, kz_thread, 1)           \
  S(MAPS, 53, kz_map_ev, 1)              \
  S(SYMBOLS, 54, kz_symbol, 1)           \
  S(TARGETS, 55, kz_target, 1)

/*
 * Raw event record (64 bytes, SPEC 2.3).  The timestamp is evidence and a search
 * heuristic only; it never becomes an HB edge.  `site` is the plan's site id (the attach
 * cookie in the eBPF backend), never the runtime address.  Strings and stacks go into side
 * sections keyed by (tid, tseq).
 */
#define KZ_RAW_EV_FIELDS(X, T)                                                       \
  X(T, uint64_t, t_ns, )   /* clock named in RUNMETA: evidence only, never HB */     \
  X(T, uint32_t, tid, )    /* kernel thread id */                                    \
  X(T, uint32_t, tseq, )   /* per-thread sequence: program order (see header note) */ \
  X(T, uint32_t, site, )   /* site id = the attach cookie */                         \
  X(T, uint16_t, phase, )  /* ENTRY | POINT | RET | WATCH | GATE | MARK */           \
  X(T, uint16_t, flags, )  /* STR_FOLLOWS, STACK_FOLLOWS, TRUNC, OBJSEQ_VALID */     \
  X(T, uint64_t, objseq, ) /* per-object token taken inside the critical section; 0 = none */ \
  X(T, uint64_t, a, [4])   /* raw values per site slots: registers, stack words, ip */

/*
 * A probe site, copied from the plan with the binary identity (SPEC 5.5, 4.7).
 * `label` is the plan's name for the site, `source` and `line` the source position the
 * plan named (empty / 0 for a function-entry site), `symbol` the function that contains
 * the probed address.  `decoder` says how the slots of the site are to be read by the
 * trace layer (KZ_DECODER).
 */
#define KZ_SITE_FIELDS(X, T)                                                         \
  X(T, uint32_t, site, ) /* = attach cookie */                                       \
  X(T, uint16_t, kind, )                                                             \
  X(T, uint16_t, insn_class, ) /* cost class of the probed instruction */            \
  X(T, uint16_t, decoder, )    /* KZ_DECODER: how the trace layer reads the slots */ \
  X(T, uint16_t, phases, )     /* set of phases attached at this site */             \
  X(T, uint32_t, line, )                                                             \
  X(T, uint64_t, file_offset, )                                                      \
  X(T, uint64_t, inode, )                                                            \
  X(T, uint64_t, device, )                                                           \
  X(T, uint8_t, build_id, [20])                                                      \
  X(T, kz_str, symbol, )                                                             \
  X(T, kz_str, path, )                                                               \
  X(T, kz_str, label, )                                                              \
  X(T, kz_str, source, )                                                             \
  X(T, uint32_t, rsv0, )

/*
 * Where one of the four raw values of a site comes from.  `src` is a KZ_SLOT_SRC;
 * `index` is a KZ_REG for REG, the byte offset into the captured stack words for STACK,
 * and 0 otherwise.  `width` is the number of bytes kept (1, 2, 4 or 8); the value is zero
 * extended into a[slot].  `role` names what the value means ("event_id", "status", ...):
 * the trace layer binds by role, never by position.
 */
#define KZ_SITE_SLOT_FIELDS(X, T)                                                    \
  X(T, uint32_t, site, )                                                             \
  X(T, uint8_t, slot, )                                                              \
  X(T, uint8_t, src, )                                                               \
  X(T, uint8_t, width, )                                                             \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint32_t, index, )                                                            \
  X(T, kz_str, role, )

/* One frame of a user stack captured for event (tid, tseq); frames sorted by index. */
#define KZ_STACK_FRAME_FIELDS(X, T)                                                  \
  X(T, uint32_t, tid, )                                                              \
  X(T, uint32_t, tseq, )                                                             \
  X(T, uint32_t, index, )                                                            \
  X(T, uint32_t, flags, ) /* TRUNC */                                                \
  X(T, uint64_t, ip, )

/*
 * Loss accounting (SPEC 5.6).  Loss is never treated as the absence of events.  The
 * perf-sample backend learns of loss only from the kernel: `count` is the number of
 * records the kernel says it dropped (perf-record-lost), or 0 with source perf-throttle
 * (an interval in which the kernel dropped records without counting them): then `t_ns` is
 * the start of the interval, `count` its length in ns, and `tseq_first` is 1 if the interval was
 * still open at the end of the trace (its end, and so its length, unknown).  Otherwise `t_ns` is
 * the time of the kernel's report; `tid`/`site` are those of the report, which the kernel attributes to
 * the record that made the loss visible, not to the records that were lost.
 */
#define KZ_LOSS_FIELDS(X, T)                                                         \
  X(T, uint8_t, source, )                                                            \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, cpu, )                                                              \
  X(T, uint32_t, site, ) /* KZ_NONE32 if the source does not know the site */        \
  X(T, uint32_t, tid, )                                                              \
  X(T, uint32_t, tseq_first, )                                                       \
  X(T, uint32_t, tseq_last, )                                                        \
  X(T, uint32_t, rsv1, )                                                             \
  X(T, uint64_t, count, )                                                            \
  X(T, uint64_t, t_ns, )

/*
 * Run metadata (SPEC 5.5).  No wall-clock date: a recorded run is identified by the
 * hashes of its inputs and by its own artifact hash.  The conditions of the run (user,
 * command, scheduling, environment, deviations) are BASIS items and DEVIATIONS of the run
 * container; `boot_ns` is the lifetime of the traced process as seen by the observer
 * (first execution mapping of the traced file to its last event), not the time to a
 * framework state.
 */
#define KZ_RUNMETA_FIELDS(X, T)                                                      \
  X(T, uint8_t, basis_sha256, [32])                                                  \
  X(T, uint8_t, plan_sha256, [32])                                                   \
  X(T, uint8_t, s0_sha256, [32]) /* content hash of the canonical initial state */  \
  X(T, uint32_t, run_ordinal, )                                                      \
  X(T, uint32_t, stratum, )                                                          \
  X(T, uint8_t, backend, )                                                           \
  X(T, uint8_t, fidelity, )                                                          \
  X(T, uint16_t, flags, )                                                            \
  X(T, uint32_t, n_holds, )                                                          \
  X(T, kz_str, kernel, )                                                             \
  X(T, kz_str, harness, )                                                            \
  X(T, kz_str, placement, ) /* observer CPU placement */                             \
  X(T, uint64_t, observer_cpu_ns, )                                                  \
  X(T, uint64_t, boot_ns, )

/*
 * One row of the per-run perturbation accounting (SPEC 4.13).
 *   probe-hits    `site`, `count` hits.  An ENTRY or POINT site costs one trap per hit at its
 *                 address (`cost_class` = the instruction class there).  A RET site costs the
 *                 return trampoline per hit (`cost_class` ret-trampoline); the trap it needs at
 *                 the function's entry is the breakpoint at its address, shared with any other
 *                 site at the same file offset.  `trap_account` says where each row's trap is
 *                 counted, so that a cost estimate counts every trap once (KZ_TRAP_ACCOUNT):
 *                 counted-here (the first ENTRY or POINT site at its file offset), in-other-row
 *                 (that site's row, found by the same SITES.file_offset), uncounted (a RET site
 *                 with no ENTRY or POINT site at its offset: its entry trap is in no row), or
 *                 0 = not stated (rows of other kinds, and containers written before the field).
 *   observer-cpu  `ns` CPU time of the observer's drain thread.
 *   boot-time     `ns` lifetime of the traced process as the observer saw it (kz_runmeta).
 *   proc-capture  the observer's reads of /proc/<pid>/... while the traced process ran:
 *                 `count` files read, `ns` the wall time the observer spent in them (cost_class
 *                 unknown).  Reading `environ` and `cmdline` copies the target's memory and takes
 *                 its address-space lock; the observation record names the capture mode.
 */
#define KZ_PERTURBATION_FIELDS(X, T)                                                 \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, trap_account, ) /* KZ_TRAP_ACCOUNT; 0 = not stated */                \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, cost_class, )                                                       \
  X(T, uint32_t, site, )                                                             \
  X(T, uint32_t, rsv2, )                                                             \
  X(T, uint64_t, count, )                                                            \
  X(T, uint64_t, ns, )

/*
 * A thread or process event from the kernel's side-band stream (PERF_RECORD_FORK, EXIT,
 * COMM): the creation relation and the kernel thread names of SPEC 5.7 items 1, 6.
 *   FORK: `tid` is the new thread, `ptid` the thread that created it (pid/ppid likewise);
 *   EXIT: `tid` left;
 *   COMM: `tid` set its kernel name to `comm` (flag COMM_EXEC: by an exec).
 * Only events of the traced processes are kept.
 */
#define KZ_SIDE_EV_FIELDS(X, T)                                                      \
  X(T, uint64_t, t_ns, )                                                             \
  X(T, uint32_t, pid, )                                                              \
  X(T, uint32_t, tid, )                                                              \
  X(T, uint32_t, ppid, )                                                             \
  X(T, uint32_t, ptid, )                                                             \
  X(T, uint8_t, kind, )                                                              \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, cpu, )                                                              \
  X(T, uint32_t, flags, )                                                            \
  X(T, kz_str, comm, )

/*
 * An executable mapping of the traced file in a process (PERF_RECORD_MMAP2).  `bias` is the
 * load bias of the mapping: runtime address - link-time address of the same byte of the file,
 * computed from the plan's sites that lie in the mapped file range (`bias_known` is 1 if at
 * least one does and all agree).  A return address minus the bias is the link-time address that
 * the basis' symbols are in.
 */
#define KZ_MAP_EV_FIELDS(X, T)                                                       \
  X(T, uint64_t, t_ns, )                                                             \
  X(T, uint64_t, addr, )                                                             \
  X(T, uint64_t, len, )                                                              \
  X(T, uint64_t, pgoff, )                                                            \
  X(T, uint64_t, ino, )                                                              \
  X(T, uint64_t, bias, )                                                             \
  X(T, uint32_t, pid, )                                                              \
  X(T, uint32_t, tid, )                                                              \
  X(T, uint32_t, dev_major, )                                                        \
  X(T, uint32_t, dev_minor, )                                                        \
  X(T, uint32_t, prot, )                                                             \
  X(T, uint32_t, flags, )                                                            \
  X(T, uint32_t, bias_known, )                                                       \
  X(T, uint32_t, rsv0, )                                                             \
  X(T, kz_str, path, )

/*
 * The identity join (SPEC 5.7): one record per thread incarnation of a traced process.
 * `creator_event` is the RAW_EVENTS index of the creator-side event (a spawn site) that
 * created the thread; `first_event` the first RAW_EVENTS record of the thread.  `announced_id`
 * is the id the thread announced at its entry site.  The name is the kernel thread name in
 * force at the end of the trace.  Indices are KZ_NONE32 when unknown; `flags` is the
 * evidence (KZ_THREAD_FLAG): a record without a flag was not established.
 */
#define KZ_THREAD_FIELDS(X, T)                                                       \
  X(T, uint32_t, pid, )                                                              \
  X(T, uint32_t, tid, )                                                              \
  X(T, uint32_t, generation, ) /* 0, then 1, 2 ... when the kernel reuses the tid */ \
  X(T, uint32_t, ptid, )       /* creating thread, or KZ_NONE32 */                   \
  X(T, uint32_t, announced_id, )    /* id announced at the entry site, or KZ_NONE32 */    \
  X(T, uint32_t, flags, )                                                            \
  X(T, uint32_t, creator_event, )                                                    \
  X(T, uint32_t, first_event, )                                                      \
  X(T, uint64_t, t_fork, )                                                           \
  X(T, uint64_t, t_exit, )                                                           \
  X(T, uint64_t, t_name, ) /* time the final name was set */                         \
  X(T, uint64_t, entry_fn, ) /* entry function pointer at the creator site */        \
  X(T, uint32_t, priority, )                                                         \
  X(T, uint32_t, n_events, )                                                         \
  X(T, kz_str, name, )                                                               \
  X(T, kz_str, entry_symbol, ) /* entry_fn resolved against the basis symbols */

/* A function symbol of the binary (basis containers): for return-address attribution. */
#define KZ_SYMBOL_FIELDS(X, T)                                                       \
  X(T, uint64_t, vaddr, )                                                            \
  X(T, uint64_t, size, )                                                             \
  X(T, uint64_t, file_offset, )                                                      \
  X(T, kz_str, name, )                                                               \
  X(T, uint8_t, bind, )                                                              \
  X(T, uint8_t, rsv0, )                                                              \
  X(T, uint16_t, rsv1, )                                                             \
  X(T, uint32_t, rsv2, )

/*
 * A probe target resolved against the binary that runs (the anchor table of a basis
 * container).  `id` is the index of the plan's site.  `insn` holds the first bytes at the
 * probed file offset (checked again by the observer before it attaches).
 */
#define KZ_TARGET_FIELDS(X, T)                                                       \
  X(T, uint32_t, id, )                                                               \
  X(T, uint16_t, kind, )                                                             \
  X(T, uint16_t, insn_class, )                                                       \
  X(T, uint64_t, vaddr, )                                                            \
  X(T, uint64_t, file_offset, )                                                      \
  X(T, uint64_t, func_vaddr, )                                                       \
  X(T, uint32_t, delta, ) /* probed address - function start */                      \
  X(T, uint32_t, line, )                                                             \
  X(T, uint32_t, col, )                                                              \
  X(T, uint32_t, prov, ) /* PROVENANCE index or KZ_NONE32 */                         \
  X(T, uint8_t, insn, [8])                                                           \
  X(T, kz_str, func, )                                                               \
  X(T, kz_str, file, )                                                               \
  X(T, kz_str, label, )

#define KZ_RAW_RECORDS(R)                  \
  R(kz_raw_ev, KZ_RAW_EV_FIELDS)           \
  R(kz_site, KZ_SITE_FIELDS)               \
  R(kz_site_slot, KZ_SITE_SLOT_FIELDS)     \
  R(kz_stack_frame, KZ_STACK_FRAME_FIELDS) \
  R(kz_loss, KZ_LOSS_FIELDS)               \
  R(kz_runmeta, KZ_RUNMETA_FIELDS)         \
  R(kz_perturbation, KZ_PERTURBATION_FIELDS) \
  R(kz_side_ev, KZ_SIDE_EV_FIELDS)         \
  R(kz_map_ev, KZ_MAP_EV_FIELDS)           \
  R(kz_thread, KZ_THREAD_FIELDS)           \
  R(kz_symbol, KZ_SYMBOL_FIELDS)           \
  R(kz_target, KZ_TARGET_FIELDS)

KZ_RAW_RECORDS(KZ_RECORD)

KZ_STATIC_ASSERT(kz_raw_ev_is_64_bytes, sizeof(kz_raw_ev) == 64u);

/* Enumerations ------------------------------------------------------------- */

#define KZ_ENUM_PHASE(ITEM, P)                                                       \
  ITEM(P, NONE, 0, "none") ITEM(P, ENTRY, 1, "ENTRY") ITEM(P, POINT, 2, "POINT")     \
  ITEM(P, RET, 3, "RET") ITEM(P, WATCH, 4, "WATCH") ITEM(P, GATE, 5, "GATE")         \
  ITEM(P, MARK, 6, "MARK")

#define KZ_ENUM_RAW_FLAGS(ITEM, P) /* set */                                         \
  ITEM(P, STR_FOLLOWS, 0x1, "STR_FOLLOWS") ITEM(P, STACK_FOLLOWS, 0x2, "STACK_FOLLOWS") \
  ITEM(P, TRUNC, 0x4, "TRUNC") ITEM(P, OBJSEQ_VALID, 0x8, "OBJSEQ_VALID")

#define KZ_ENUM_SITE_KIND(ITEM, P)                                                   \
  ITEM(P, NONE, 0, "none") ITEM(P, ENTRY_PROBE, 1, "entry-probe")                    \
  ITEM(P, POINT_PROBE, 2, "point-probe") ITEM(P, RET_PROBE, 3, "ret-probe")          \
  ITEM(P, WATCHPOINT, 4, "watchpoint") ITEM(P, GATE, 5, "gate")

/*
 * Cost class of the probed instruction (SPEC 5.1, 5.10).  `emulated` and `trap` are
 * assigned only to the instruction patterns that [R07 4.7] measured (a probe on the push
 * after endbr64 is emulated; a probe on endbr64 or on the mov after the push traps);
 * every other instruction is `unknown` until the S1 calibration measures it.
 * `ret-trampoline` is not an instruction class: it is the cost class of the RETURN side of a
 * return probe in PERTURBATION (the kernel's uretprobe trampoline, entered once per return).
 * No site's insn_class is ever ret-trampoline, and its cost is unknown until S1 measures it.
 */
#define KZ_ENUM_INSN_CLASS(ITEM, P)                                                   \
  ITEM(P, UNKNOWN, 0, "unknown") ITEM(P, EMULATED, 1, "emulated") ITEM(P, TRAP, 2, "trap") \
  ITEM(P, RET_TRAMPOLINE, 3, "ret-trampoline")

#define KZ_ENUM_SITE_PHASES(ITEM, P) /* set */                                       \
  ITEM(P, ENTRY, 0x1, "ENTRY") ITEM(P, POINT, 0x2, "POINT") ITEM(P, RET, 0x4, "RET") \
  ITEM(P, WATCH, 0x8, "WATCH") ITEM(P, GATE, 0x10, "GATE")

/*
 * How the trace layer reads a site.  GENERIC: the slot roles say everything.  The three
 * others are the roles the identity join (SPEC 5.7) needs, independent of any framework.  The
 * join reads these positions; check_site refuses a site whose slots do not declare them (the
 * role names in quotes are required, the others are free):
 *   SPAWN_CREATE  creator side, ENTRY: a[1] "entry_fn", a[2] "priority" (at most 4 bytes),
 *                 a[3] "return_address"; a[0] (a name pointer) is kept raw and not read
 *   SPAWN_RETURN  creator side, RET:   a[3] "return_address" (the sample ip); a[0] status
 *   THREAD_ENTRY  child side, ENTRY:   a[0] the id the thread announces (any role, at most
 *                 4 bytes)
 */
#define KZ_ENUM_DECODER(ITEM, P)                                                     \
  ITEM(P, NONE, 0, "none") ITEM(P, GENERIC, 1, "generic")                            \
  ITEM(P, SPAWN_CREATE, 2, "spawn-create") ITEM(P, SPAWN_RETURN, 3, "spawn-return")  \
  ITEM(P, THREAD_ENTRY, 4, "thread-entry")

#define KZ_ENUM_SLOT_SRC(ITEM, P)                                                    \
  ITEM(P, NONE, 0, "none") ITEM(P, REG, 1, "reg") ITEM(P, STACK, 2, "stack")         \
  ITEM(P, IP, 3, "ip")

/* Registers by name, independent of the backend (x86-64). */
#define KZ_ENUM_REG(ITEM, P)                                                         \
  ITEM(P, RAX, 0, "rax") ITEM(P, RBX, 1, "rbx") ITEM(P, RCX, 2, "rcx") ITEM(P, RDX, 3, "rdx") \
  ITEM(P, RSI, 4, "rsi") ITEM(P, RDI, 5, "rdi") ITEM(P, RBP, 6, "rbp") ITEM(P, RSP, 7, "rsp") \
  ITEM(P, R8, 8, "r8") ITEM(P, R9, 9, "r9") ITEM(P, R10, 10, "r10") ITEM(P, R11, 11, "r11")   \
  ITEM(P, R12, 12, "r12") ITEM(P, R13, 13, "r13") ITEM(P, R14, 14, "r14") ITEM(P, R15, 15, "r15")

#define KZ_ENUM_LOSS_SOURCE(ITEM, P)                                                 \
  ITEM(P, NONE, 0, "none") ITEM(P, RING_RESERVE_FAIL, 1, "ring-reserve-fail")        \
  ITEM(P, PERF_RECORD_LOST, 2, "perf-record-lost") ITEM(P, TSEQ_GAP, 3, "tseq-gap")  \
  ITEM(P, PERF_THROTTLE, 4, "perf-throttle")

#define KZ_ENUM_BACKEND(ITEM, P)                                                     \
  ITEM(P, NONE, 0, "none") ITEM(P, PERF_SAMPLE, 1, "perf-sample")                    \
  ITEM(P, BPF_RAW, 2, "bpf-raw") ITEM(P, TRACEFS, 3, "tracefs") ITEM(P, GDB, 4, "gdb")

/* Fidelity classes of SPEC 5.1. */
#define KZ_ENUM_FIDELITY(ITEM, P)                                                    \
  ITEM(P, NONE, 0, "none") ITEM(P, F0, 1, "F0") ITEM(P, F1, 2, "F1") ITEM(P, F2, 3, "F2") \
  ITEM(P, F3, 4, "F3")

#define KZ_ENUM_STRATUM(ITEM, P) ITEM(P, NONE, 0, "none") ITEM(P, WARM, 1, "warm") ITEM(P, COLD, 2, "cold")

#define KZ_ENUM_RUN_FLAGS(ITEM, P) /* set */                                         \
  ITEM(P, NORMALIZATION_BOOT, 0x1, "normalization-boot") ITEM(P, HOLDS_USED, 0x2, "holds-used") \
  ITEM(P, ABNORMAL_EXIT, 0x4, "abnormal-exit") ITEM(P, LOSS_PRESENT, 0x8, "loss-present")

#define KZ_ENUM_PERTURBATION_KIND(ITEM, P)                                           \
  ITEM(P, NONE, 0, "none") ITEM(P, PROBE_HITS, 1, "probe-hits") ITEM(P, HOLD, 2, "hold") \
  ITEM(P, OBSERVER_CPU, 3, "observer-cpu") ITEM(P, BOOT_TIME, 4, "boot-time")      \
  ITEM(P, PROC_CAPTURE, 5, "proc-capture")

#define KZ_ENUM_TRAP_ACCOUNT(ITEM, P)                                                 \
  ITEM(P, NOT_STATED, 0, "not-stated") ITEM(P, COUNTED_HERE, 1, "counted-here")       \
  ITEM(P, IN_OTHER_ROW, 2, "in-other-row") ITEM(P, UNCOUNTED, 3, "uncounted")

#define KZ_ENUM_SIDE_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, FORK, 1, "FORK") ITEM(P, EXIT, 2, "EXIT") ITEM(P, COMM, 3, "COMM")

#define KZ_ENUM_SIDE_FLAGS(ITEM, P) /* set */ ITEM(P, COMM_EXEC, 0x1, "comm-exec")

/*
 * Evidence of an identity record (a flag that is clear was not established).
 *   SPAWN_MATCHED    the thread's FORK lies inside exactly one closed creator-side interval of
 *                    its creating thread.  The perf-sample backend has no other key: the match
 *                    is by timestamps of ONE thread (the creator's events and the kernel's FORK
 *                    record of the same thread), so it holds under A-CLK-PO; MATCH_BY_TIME says so.
 *   MATCH_BY_TIME    the creator link (matched or ambiguous) rests on timestamp containment only.
 *   ID_UNVERIFIED    the announced id was recorded but not compared with a creator-side value:
 *                    the id the spawn call assigns is an out-parameter that the perf-sample
 *                    backend cannot read (it cannot dereference memory).  Deferred to a backend
 *                    that reads it at the spawn return.
 */
#define KZ_ENUM_THREAD_FLAGS(ITEM, P) /* set */                                      \
  ITEM(P, FORK_SEEN, 0x1, "fork-seen") ITEM(P, COMM_SEEN, 0x2, "comm-seen")          \
  ITEM(P, ENTRY_SEEN, 0x4, "entry-seen") ITEM(P, SPAWN_MATCHED, 0x8, "spawn-matched") \
  ITEM(P, SPAWN_AMBIGUOUS, 0x10, "spawn-ambiguous")                                  \
  ITEM(P, NAME_INHERITED, 0x20, "name-inherited")                                    \
  ITEM(P, NAME_AFTER_FIRST_EVENT, 0x40, "name-after-first-event")                    \
  ITEM(P, TID_REUSED, 0x80, "tid-reused") ITEM(P, EXIT_SEEN, 0x100, "exit-seen")     \
  ITEM(P, ENTRY_UNMATCHED, 0x200, "entry-unmatched")                                \
  ITEM(P, MATCH_BY_TIME, 0x400, "match-by-time") ITEM(P, ID_UNVERIFIED, 0x800, "id-unverified")

#define KZ_ENUM_SYMBOL_BIND(ITEM, P) \
  ITEM(P, LOCAL, 0, "local") ITEM(P, GLOBAL, 1, "global") ITEM(P, WEAK, 2, "weak")

#define KZ_ENUM_TARGET_KIND(ITEM, P) \
  ITEM(P, NONE, 0, "none") ITEM(P, FUNC_ENTRY, 1, "func-entry") ITEM(P, AFTER_CALL, 2, "after-call")

#define KZ_RAW_ENUMS(D)                           \
  D(KZ_PHASE, KZ_ENUM_PHASE, 0)                   \
  D(KZ_RAW_FLAG, KZ_ENUM_RAW_FLAGS, 1)            \
  D(KZ_SITE_KIND, KZ_ENUM_SITE_KIND, 0)           \
  D(KZ_INSN_CLASS, KZ_ENUM_INSN_CLASS, 0)         \
  D(KZ_SITE_PHASE, KZ_ENUM_SITE_PHASES, 1)        \
  D(KZ_DECODER, KZ_ENUM_DECODER, 0)               \
  D(KZ_SLOT_SRC, KZ_ENUM_SLOT_SRC, 0)             \
  D(KZ_LOSS_SOURCE, KZ_ENUM_LOSS_SOURCE, 0)       \
  D(KZ_BACKEND, KZ_ENUM_BACKEND, 0)               \
  D(KZ_FIDELITY, KZ_ENUM_FIDELITY, 0)             \
  D(KZ_STRATUM, KZ_ENUM_STRATUM, 0)               \
  D(KZ_RUN_FLAG, KZ_ENUM_RUN_FLAGS, 1)            \
  D(KZ_PERTURBATION_KIND, KZ_ENUM_PERTURBATION_KIND, 0) \
  D(KZ_TRAP_ACCOUNT, KZ_ENUM_TRAP_ACCOUNT, 0)     \
  D(KZ_SIDE_KIND, KZ_ENUM_SIDE_KIND, 0)           \
  D(KZ_SIDE_FLAG, KZ_ENUM_SIDE_FLAGS, 1)          \
  D(KZ_THREAD_FLAG, KZ_ENUM_THREAD_FLAGS, 1)      \
  D(KZ_SYMBOL_BIND, KZ_ENUM_SYMBOL_BIND, 0)       \
  D(KZ_TARGET_KIND, KZ_ENUM_TARGET_KIND, 0)

KZ_RAW_ENUMS(KZ_DEFINE_ENUM_ENTRY)

/* Not a field format (a slot's `index` means a register only when `src` is REG), so this
 * list defines the constants KZ_REG_* and is not registered for the dump. */
KZ_DEFINE_ENUM(KZ_REG, KZ_ENUM_REG, 0)

#define KZ_RAW_FORMATS(F)                          \
  F(kz_raw_ev, phase, ENUM, KZ_PHASE)              \
  F(kz_raw_ev, flags, FLAGS, KZ_RAW_FLAG)          \
  F(kz_raw_ev, objseq, HEX, 0)                     \
  F(kz_raw_ev, a, HEX, 0)                          \
  F(kz_site, kind, ENUM, KZ_SITE_KIND)             \
  F(kz_site, insn_class, ENUM, KZ_INSN_CLASS)      \
  F(kz_site, decoder, ENUM, KZ_DECODER)            \
  F(kz_site, phases, FLAGS, KZ_SITE_PHASE)         \
  F(kz_site, file_offset, HEX, 0)                  \
  F(kz_site_slot, src, ENUM, KZ_SLOT_SRC)          \
  F(kz_stack_frame, ip, HEX, 0)                    \
  F(kz_loss, source, ENUM, KZ_LOSS_SOURCE)         \
  F(kz_loss, site, IDX, 0)                         \
  F(kz_runmeta, stratum, ENUM, KZ_STRATUM)         \
  F(kz_runmeta, backend, ENUM, KZ_BACKEND)         \
  F(kz_runmeta, fidelity, ENUM, KZ_FIDELITY)       \
  F(kz_runmeta, flags, FLAGS, KZ_RUN_FLAG)         \
  F(kz_perturbation, kind, ENUM, KZ_PERTURBATION_KIND) \
  F(kz_perturbation, cost_class, ENUM, KZ_INSN_CLASS) \
  F(kz_perturbation, site, IDX, 0)                 \
  F(kz_perturbation, trap_account, ENUM, KZ_TRAP_ACCOUNT) \
  F(kz_side_ev, kind, ENUM, KZ_SIDE_KIND)          \
  F(kz_side_ev, flags, FLAGS, KZ_SIDE_FLAG)        \
  F(kz_map_ev, addr, HEX, 0)                       \
  F(kz_map_ev, pgoff, HEX, 0)                      \
  F(kz_map_ev, bias, HEX, 0)                       \
  F(kz_thread, ptid, IDX, 0)                       \
  F(kz_thread, announced_id, IDX, 0)                    \
  F(kz_thread, flags, FLAGS, KZ_THREAD_FLAG)       \
  F(kz_thread, creator_event, IDX, 0)              \
  F(kz_thread, first_event, IDX, 0)                \
  F(kz_thread, entry_fn, HEX, 0)                   \
  F(kz_symbol, vaddr, HEX, 0)                      \
  F(kz_symbol, file_offset, HEX, 0)                \
  F(kz_symbol, bind, ENUM, KZ_SYMBOL_BIND)         \
  F(kz_target, kind, ENUM, KZ_TARGET_KIND)         \
  F(kz_target, insn_class, ENUM, KZ_INSN_CLASS)    \
  F(kz_target, vaddr, HEX, 0)                      \
  F(kz_target, file_offset, HEX, 0)                \
  F(kz_target, func_vaddr, HEX, 0)                 \
  F(kz_target, prov, IDX, 0)

#endif /* KZ_SCHEMA_KZ_RAW_H */

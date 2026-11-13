# Judgment J2: cFS / flight-software domain lens

- Date: 2026-10-12. Judge lens: cFS / flight-software domain expert (NASA GSFC style). Scoring weights fidelity to the real cFS architecture (OSAL/PSP/cFE/apps, configuration, startup, SB/TBL/ES lifecycle), coverage of the 15 readiness-contract kinds of [R03] §4 and beyond the five cases, usefulness to cFS developers, and generality across versions, configurations and ports.
- Proposals judged:
  - **A**: `proposal_A-static-model.md`, called "CAIRN".
  - **B**: `proposal_B-dynamic-predictive.md`, called "Sextant".
  - **C**: `proposal_C-hybrid-layered.md`, called "Kairos".
- Evidence base: [R01]–[R07] under `scratchpad/arch/`, the research records (`CONDITIONS_POLICY.md`, `cases/ledger.md`, `results/R2_gdb_v701.md`), and read-only spot checks of the pinned tree `/home/user/work/procrace/cfs_ref/cFS` (cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`). Code paths below are relative to that tree.
- Labels:
  - **[V]**: verified by me in the tree for this judgment.
  - **[R]**: taken from a report, with its citation.
  - **[I]**: my inference.
- Literature is mentioned only as the proposals cite it. I checked that every cited key appears in [R04], [R05] or [R06] with the status tag used (§6). Nothing in this judgment was built or run. I wrote only this file.

---

## 0. Verdict

| Design | Validity | cFS fit | Openness | Prior work | Feasibility | Evaluation | **Total /60** |
|---|---|---|---|---|---|---|---|
| A (static model) | 7 | 9 | 9 | 8 | 5 | 9 | **47** |
| B (dynamic predictive) | 7 | 8 | 7 | 8 | 7 | 8 | **45** |
| C (hybrid layered) | 8 | 9 | 9 | 9 | 6 | 8 | **49** |

**Winner: C (Kairos), used as the spine.** The synthesis should graft in A's model-level rigor and B's observation and confirmation craft (§4).

Why C wins under this lens:

1. **One profile, three uses.** C's profile Φ_cFS holds resource kinds, operations, HB rules with strata, and contracts, as versioned data. The same profile is interpreted three ways: statically over skeletons, concretely over F0 traces, and operationally in a bounded explorer. This is the most maintainable form of cFS knowledge, and the only one that states, as refutable hypotheses, how generality across cFE versions and configurations would be tested (H-V, H-C).
2. **Ordering strata and a patient semantics.** C splits ordering into S (structural), O (observed sync) and C_σ (schedule constraints). It also adds σ_U∞, a "patient" semantics with no timeouts. That split matches how cFS really orders things:
   - core serialization ends in panic, so it is structural;
   - app barriers are soft, so they are observed sync;
   - priorities matter only on RT single-core targets, so they are schedule constraints.

   No other design separates "schedule-dependent only through a soft timeout" from "schedule-dependent in the ordering logic". That separation is exactly what a cFS developer needs to decide whether a finding is a margin problem or a protocol problem.
3. **C has no fatal flaw.** A has a validity bug in its headline tier-1 rule (§2.1) and the heaviest engineering load. B's verdicts are bounded by observed executions, and its lock-reversal predictor depends on a solver that is not installed (§2.2).

The margins are small (49 / 47 / 45). A is the better *model* and B is the better *instrument*. C is the better *architecture* for combining them.

---

## 1. Spot checks made for this judgment

| # | Claim used by one or more proposals | Result |
|---|---|---|
| 1 | Core phase 2: `MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)`, and on failure `CFE_PSP_Panic` | **[V]** `cfe/modules/es/fsw/src/cfe_es_start.c` L833-846 |
| 2 | `MainTaskSyncDelay` counts *used* records with `AppState < target` under the ES lock, polls with `OS_TaskDelay` | **[V]** `cfe_es_start.c` L870-917 |
| 3 | `CFE_ES_TaskInit` has **early returns before every SB use**, and `CFE_ES_TaskMain` still calls `WaitForSystemState(CORE_READY)` after an init failure | **[V]** returns at `cfe_es_task.c` L340, L371, L381, L393 (after `CFE_EVS_Register` L336, `CFE_SB_CreatePipe` L367, `Subscribe` L377, L387). TaskMain sets `CORE_APP_INIT_ERROR` and **continues** to `CFE_ES_WaitForSystemState` (L118-133). This decides §2.1 |
| 4 | An EVS init failure is terminal-ish: `CFE_ES_ExitApp(CORE_APP_INIT_ERROR)` → `CFE_ES_ResetCFE(PROCESSOR)` | **[V]** `cfe_evs_task.c` L214-221; `cfe_es_api.c` L333-362. Inference [I]: PSP restart is not instantaneous (10 s before abort, [R01] §5.2), so "terminal" needs care in the model |
| 5 | SB publishes its AppId through an out-pointer and ignores the status | **[V]** `cfe/modules/sb/fsw/src/cfe_sb_task.c` L137-138 |
| 6 | `CFE_EVS_SendEventWithAppID` precondition, with syslog named as the alternative | **[V]** `cfe/modules/core_api/fsw/inc/cfe_evs.h` L168-172 |
| 7 | K-PHASE provenance: "caller has also been initialized sufficiently to satisfy the global system state it is waiting for" | **[V]** `cfe_es.h` L400-403. A cites this; C cites only the state-enum comments |
| 8 | Doxygen counts in `core_api/fsw/inc` | **[V]** 138 `\par Assumptions`; 429 `\retval` (A's count); 452 `\retval`+`@retval` and 74 `\param[out]`+`@param[out]` (B's count). Both are correct under their definitions |
| 9 | `GetTaskFunction`: 50 ms delay, check under the lock, stop when ready or `Timeout <= 0` | **[V]** `cfe_es_apps.c` L559-596. The task record is filled under the lock after `OS_TaskCreate` (L664-722). The App record is set used at about L869 |
| 10 | OSAL `ConvertToken` returns `OS_ERR_INCORRECT_OBJ_STATE` before the unlock logic, while its header says the lock is always released on failure | **[V]** `osal/src/os/shared/src/osapi-idmap.c` L384-385 vs L401-404 |
| 11 | CF retry: 25 attempts on `NAME_NOT_FOUND` only, and the loop breaks on any other code; comment "start up race condition" | **[V]** `apps/cf/fsw/src/cf_cfdp.c` L1270-1301 |
| 12 | RunLoop raises the caller to RUNNING; WaitForSystemState raises the caller's own state under the lock, then polls `SystemState` | **[V]** `cfe_es_api.c` L460-472, L520-606 |
| 13 | **HS `linux_sysmon`** is a "NO_PROVIDER" lookup (A §1.6, §3.17 RC1) | **Refuted [V].** `linux_sysmon` is in the PSP fixed module list (`psp/fsw/pc-linux/psp_module_list.cmake` L9). A failed `CFE_PSP_IODriver_FindByName` is *handled* (syslog, `CFE_SUCCESS`; `apps/hs/fsw/src/hs_sysmon.c` L50-54). The measured HS failure is the `SET_RUNNING` device start returning `CFE_STATUS_EXTERNAL_RESOURCE_FAIL` (L65-72), because `/proc/schedstat` is missing ([R01] §2.6). That is a platform-capability failure, not a missing provider. [R03] §3 row 20 has the same conflation |
| 14 | R2 counts: ES 20 (17 `SEND_NO_SUBS`, 2 subscribe, 1 pipe) and EVS 4; 0 uses between publication and registration | **[V]** `results/R2_gdb_v701.md` §2. A's predicted breakdown (Appendix A.1 step 4) matches |
| 15 | Record layouts: A 64 B, B 48 B, C 72 B | **[V]** by arithmetic over the declared fields |

---

## 2. Per-design assessment

### 2.1 A: CAIRN (model-first)

**What it is.**
- Configuration closure from the real build: compile DB, `link.txt`, the generated object table, the startup script, and table initializers constant-evaluated by Clang and cross-checked byte for byte against the built `.tbl` through the GCC binary's DWARF.
- Whole-program LLVM IR is sliced into an MLIR `cfsk` skeleton dialect.
- Typed contracts (RCL) whose readiness predicates *call cFE's own validity functions*.
- Tier 1 is a must-HB fixpoint with independently checked certificates. Tier 2 is optimal DPOR over the skeleton, with an optional SMT backend.
- F0 uprobe/eBPF observation is used for **model conformance**: every observed trace must belong to L(M).
- Confirmation uses ptrace holds (F0) or an `ld --wrap` PCT variant (F1).

**Strengths (cFS lens).**
- **OSAL portability axis.** A models OSAL at two levels: M_api (the API contract, portable to the VxWorks and RTEMS backends) and M_impl (the POSIX implementation at the pin). It reports "PROTECTED under M_impl, violable under M_api" as a *portability hazard*. This is the only design that addresses cFS's multi-backend reality, and it is the right answer to [R01] §7 item 6.
- **Framework-defined readiness.** For example, `es.app.LIVE(a) := CFE_ES_AppRecordIsMatch(...)`. The definition of "ready" comes from cFS code, and the RESERVED windows fall out of the actual fields. Writer and disabler sets are *computed* as the stores that can change the predicate. This is the strongest anti-circularity device of the three, and it gives the completeness of writer sets that observation edges need.
- **Violation shapes.** WRONG-WAY, UNORDERED, ABSENT and STALE (generation mismatch) are first-class. STALE matters for RC15 and for restarts.
- **Closure from generated artifacts.** A cross-checks the closure against one observed boot (task creations, `dlsym` results). It is never hard-coded: [R02] §1.3 corrected the record that had hard-coded the core order.
- **Conformance checking.** Observed boots are checked against the model, and non-conforming regions turn the affected verdicts into UNKNOWN(conformance). That measured link is what makes model verdicts defensible to a flight-software reviewer.
- **Route and capacity losses.** The LOSS-UNDER-ORDER label is used where no reference defines a must-deliver rule. This is correct for SB: a no-subscriber send returns `CFE_SUCCESS`, and [R03] §1 confirms the semantics.
- **Measured phenomena as a research question.** RQ5 asks whether the tool classifies the losses already measured in the default bundle (`SBNSubPipe`, `TO_LAB_TLM_PIPE`, `DS_CMD_PIPE` overflows; `NoSubscribersCounter`). That is the evaluation question a cFS reviewer would ask first.
- **Further strengths:**
  - K-CAP tier-1 upper bound by maximum-weight closure. The citation is marked "not verified in AR1", which is honest.
  - Restart scenario family with the cleanup order EVS → SB → TBL → TIME.
  - TIME's 1 Hz pipeline used as a calibration check that should come out PROTECTED.
  - Build lints that reject case identifiers and bare numeric constants in the tool's semantic code.

**Flaws.**
1. **SERIOUS (validity of the headline claim; fixable). Tier-1 FIXED_ORDER is defined by dominance-based `MustBefore`, which cannot certify R2.**
   - A requires `s_u ∈ MustBefore(x)` for every writer x. In A's semantics this means "u occurs in every execution in which x occurs", because T-PO uses dominance.
   - At the pin, every SB use in `CFE_ES_TaskInit` sits behind an earlier early-return check (spot check 3).
   - On the error path, `CFE_ES_TaskMain` still raises ES to RUNNING through `WaitForSystemState(CORE_READY)`. The startup thread then creates EVS and SB, and SB publishes at `cfe_sb_task.c` L138.
   - So u does not dominate the raise, `u ∉ MustBefore(x)`, and A's rules **cannot** produce the "b = ∞" R2 certificate that §6.1 G2 and the M3 exit criterion promise. Tier 2 would still find FIXED_ORDER *within b*, so the rule is incomplete rather than unsound.
   - **Fix:** certify on "no execution in which a publisher precedes u" (u cannot happen after x). For example: there is an m in task(u) with no CFG path m → u, and m ∈ MustBefore(x) for every x ∈ W. Here m is ES's state raise. C's formulation ("on every path on which p occurs") already has the right semantics.
2. **MAJOR (feasibility).** A carries the heaviest engineering load:
   - whole-program slicing into a code skeleton;
   - loop classification;
   - a `cfsk` interpreter with M_impl POSIX semantics for pthread, sem and mq;
   - optimal DPOR over a 27-thread startup;
   - conformance as a linearization search;
   - an MLIR/ODS toolchain on an unreleased LLVM trunk.

   Lemmas L1 and L2 are unproved (listed honestly as A15 and A16). The ES and SB locks are touched by nearly every task, so state explosion is likely. A acknowledges this risk.
3. **MINOR (fidelity).**
   - The HS `linux_sysmon` example of NO_PROVIDER is factually wrong (spot check 13).
   - Optimal DPOR on a model with blocking operations (await, locks) needs the disabling-aware version. Its JACM extension has status **P** in [R05] §2.2, which A does not mention.
4. **MINOR (requirement friction).** MLIR ODS/TableGen generation at build time pulls against "no messy code generation" ([R06] K8). A puts it behind gate G-MLIR, which is correct.

**Fact check of A's R2 worked example.** The predicted counts (ES 20, EVS 4, and 11 with a NULL version table) agree with the records (spot check 14). The proof shape is right; only the tier-1 rule cannot derive it (flaw 1).

### 2.2 B: Sextant (trace-first predictive)

**What it is.**
- Static work is limited to probe placement and API/site summaries.
- Root-side F0 uprobes, with eBPF or a tracefs fallback, observe the unmodified GCC binary.
- A lifter builds a typed event IR with four identities and generation-aware resources.
- HB is cFS-aware and keeps two vector clocks: VC over all definite rules, and VC_R over "robust" (unconditional) rules.
- Contracts are checked on both ordered and unordered pairs.
- Prediction runs in three stages: P1 SHB, P2 WCP/SyncP/M2, P3 an SMT order-variable encoding.
- Confirmation holds a thread at a debug-register execution breakpoint until a named event occurs.

**Strengths (cFS lens).**
- **Highest instrument fidelity and lowest setup.**
  - Probes sit at entry+4 of the GCC binary, with cost classes decoded by MC (0.57 µs vs 31.7 µs, [R07] §4.7).
  - Identity comes from creation events only, never from `GetAppID`.
  - Per-object tokens are taken at a point *inside* the critical section (`OS_MutSemGive` entry). This recovers lock order from cheap entry probes without timestamps. It is clever and cheap.
- **Outcome mining.** Every not-ready return code at the API boundary, from a vocabulary built from `\retval`/`@retval`, becomes a candidate even when no template exists. This is the cheapest form of template-free openness, and it fits cFS well because cFE/OSAL return codes are rich and documented.
- **Summary-guided extrapolation.** A retry or poll site observed once can be extended in prediction up to its reference bound (CF 25, `GetTaskFunction` 21, OSAL 4 waits, `WaitForSystemState` ⌈T/50⌉). Without this, predictive analysis could never show that CF's retry budget can be exhausted. The idea is well suited to cFS.
- **Confirmation without touching the binary.** Holds use debug-register breakpoints and release on a named event (hold-until-event), with watchdogs taken from reference timeouts. This is a clean F0 confirmation mechanism.
- **A separate `timeout-dependent` class**, and a rule-coverage report (anything unmodelled yields `unknown`, never `protected`).
- **Best for developers.** Findings come as concrete traces from real boots, plus confirmed schedules.

**Flaws.**
1. **MAJOR (openness; structural).** Verdicts exist only for executed instances. Latent contracts become `unobserved` unless someone builds configuration variants by hand: CF `sem_name`, HS AppMon (all NOACT), CS tables, MD/MM symbols, TBL atomicity, restarts, file TOCTOU. By [R03] §4, most of RC2/3/4/8/12/15 are latent in the default configuration.
   - Static work is confined to summaries, so B cannot classify (fixed-order / protected / schedule-dependent) what it never saw, and choosing the variants re-introduces researcher choice.
   - Predictions are also limited to observed paths, which B admits (§6).
2. **MAJOR (feasibility of a core class).** P3 is needed for lock-reversal prediction, i.e. the C1/C5 shape: lock-ordered create vs lookup, which SyncP's class excludes by definition ([R04] §2.5). P3 needs an SMT solver, and **none is installed** ([R07] §2.4).
   - B's fallback is `unknown(no-solver)`. It does not propose an explicit-search engine. C does.
   - Without a PI install decision, the lock-protected order-violation class, central to the problem statement, is undecided.
3. **SERIOUS (validity).**
   - The tracefs fallback reconstructs per-object order from timestamps under A-CLK. That contradicts the rule that HB never comes from timestamps ([R04] §2.1 L1; v0.1 §9.3), even though it is labelled.
   - OBS-FATAL is tagged "unconditional". Robustness across executions needs the *writer set* of the observed predicate to be complete, but B does no whole-program writer analysis by design.
   - The "path stability" assumption of robust certificates is discharged only by a syntactic site summary.
4. **MINOR (generality).** Internal line probes keyed by line number (`cfe_es_apps.c:821/869/703`, `cfe_es_start.c:818`) are version-fragile. B says they are re-resolved "by AST pattern", but no mechanism is specified.
5. **MINOR.** P1 and P2 add engines that mostly cannot decide the dominant cFS shapes: R2 is ordered, and C1/C5 need lock reversal. They are cheap pre-filters at best.

### 2.3 C: Kairos (hybrid, layered, feedback)

**What it is.**
- One formal core: the KEM event type plus the profile DSL Φ_cFS (kinds with lifecycle automata, operations with probe bindings, HB rules with strata S/O and code citations, contracts with `Allowed` and harm).
- Φ is interpreted three ways:
  - **abstractly**, over static event skeletons: census and tier-1 `≺_S` proofs;
  - **concretely**, over F0 traces: HB_π, ordered and unordered contract checks, and predictive reordering by explicit search with sleep-set POR;
  - **operationally**, in a per-candidate bounded explorer: source/optimal DPOR with the two queries.
- Confirmation uses ptrace holds (F0), or `--wrap` gates (F1, PI-gated).
- A feedback loop with six consistency obligations (O1–O6) and a monotone, terminating fixed point ties the layers together.

**Strengths (cFS lens).**
- **The best fit to cFS's mixture of hard and soft orders.**
  - S stratum: EarlyInit, then core serialization guarded by a terminal panic (R18, R19), and script sequencing of library init and creation (R3).
  - O stratum: mutexes, semaphores, delivered SB messages, polls that exit by their condition, outcome-implied reads-from.
  - C_σ: priorities, only under σ_R1.
  - σ_U∞ separates the ES App-record window (schedule-dependent even without timeouts: only a 50 ms sleep separates the two) from barrier-timeout cases (schedule-dependent only through a soft timeout).
- **Profile as data, with version guards.** For example, R19 enters in 6.4.2 as C3's fix F3, and the two-phase barrier enters in 6.6.0 as C2's fix. H-V proposes the right test: the same engines with Φ_6.4.x must classify C3 at 6.4.1 as fixed under σ_R1 and schedule-dependent under σ_U. This is the clearest generality story across cFS versions and configurations.
- **On-behalf frames from frame-pointer stacks.** These cover TIME synch callbacks in TIME's task ([R02] §5.3), FS write callbacks in `ES_BG_TASK` ([R02] §6.1), and OSAL timer callbacks in the timebase thread. This is the most complete answer to gap G6 (layered identity).
- **Contract sources go beyond the 15 kinds without case knowledge.**
  - `K-LOCKBAL` is derived from OSAL's *own documented invariant* (`osapi-idmap.c` L384-385) and yields the F1 lock-leak candidate generically.
  - `K-LOCK.cancel` covers deferred cancellation while a program lock is held. It is cFS-real: `CFE_ES_DeleteChildTask` deletes under the ES lock ([R02] §1.10).
  - Lifecycle symmetry: every kind with create and delete gets K-REG, K-LIFE and K-GEN automatically.
  - Retry loops derive `Allowed = retry(bound read at the site)` with the loop itself as provenance.
- **Prediction needs no solver.** Explicit search with POR runs without Z3, and SMT-LIB stays optional.
- **Wait-for oracle for the lock-leak confirmation** (`/proc/<tid>/wchan` or futex tracepoints), instead of a timeout.
- **The layers check each other.** The obligations O1–O6 (template concretization, skeleton acceptance, no observed order contradicting an S-edge, and so on) make each layer an oracle for the others, and contradictions suspend rules instead of being averaged. This is a strong validity argument for a hybrid.
- **The feedback loop is itself ablated**, with per-iteration metrics.

**Flaws.**
1. **SERIOUS (specification gap).** C computes `≺_S` by "assigning per-actor clocks to the (loop-summarized) skeleton DAGs … path conditions carried on edges". Skeletons with branches and loops are not DAGs, and the procedure that decides "t ≺_S p on every path on which p occurs" is not given. The *semantics* is right, and it is what A's rule gets wrong. The *algorithm* has to be specified before the R2 proof claim can be audited. A's must-HB fixpoint, corrected as in §2.1, can fill the gap.
2. **MAJOR (breadth and integration risk).** C has about 14 components plus three interpreters that must agree. The risk is many shallow pieces. C mitigates it with milestones: M0–M4 already decide R2 and every structurally ordered instance.
3. **MINOR.**
   - The explorer's event-level skeletons turn data branches into nondeterministic choices, which can yield spurious model-level schedule-dependent verdicts. They are labelled `model(b)` and need confirmation, which is acceptable.
   - K-PHASE cites the state-enum comments rather than the documented API assumption `cfe_es.h` L400-403.
   - The RC coverage summary says "11 fully, 4 partially", but the table marks RC2/RC5 harm as partial too. The counting is loose.
   - Return probes for `objseq` at mutex-take return are unmeasured. B's give-entry trick is cheaper.

---

## 3. Coverage of the 15 contract kinds and beyond ([R03] §4)

| RC | A | B | C | Comment (cFS lens) |
|---|---|---|---|---|
| RC1 OSAL/PSP name lookup | full (M_api + M_impl outcomes) | full when executed | full (+ K-LOCKBAL) | A's HS example needs the correction in §1 #13 |
| RC2 ES name lookup | full; `IsMatch` predicate catches the `0xFFFFFFFF` sentinel | full when executed (HS XCT) | full ordering; harm `unspecified` | All three catch the RESERVED sentinel |
| RC3 TBL share | full (latent) | variant needed | full | latent in the default configuration (CS) |
| RC4 loader symbols | full (SBN lazy load resolved from its table) | full when executed | full | |
| RC5 route | full, LOSS-UNDER-ORDER | full | full, harm partial | The measured phenomenon; all three include it |
| RC6 identity | full | full | full | R2, C3, C5 |
| RC7 phase declaration | full (documented assumption) | full | full | A has the best provenance |
| RC8 persistence | partial (two-run composition) | phase 2 | partial (run sequences) | all honest |
| RC9 parent/child | full (order); memory part separate | order only | full + class M | all separate memory races |
| RC10 timer → task | full | full | full | |
| RC11 lib before client | certificate (calibration) | certificate | structural (calibration) | |
| RC12 file TOCTOU | partial | needs inputs | partial (Velodrome/AVIO style) | |
| RC13 SB global mode | full | full | full | |
| RC14 capacity | full + backlog bound | full (SMT capacity) | full | A's max-closure bound is a cheap PROTECTED test |
| RC15 stale reference | full in scenario family | only with restart inputs | partial | |
| Beyond | OSAL F1 (deadlock check), F7 (`entry_runs`), SB buffer lifetime, TBL load atomicity, TIME callback after unload, portability hazards | outcome mining, K-LIFE/K-MODE/K-FILE, watchpoint discovery of direct global reads | K-LOCKBAL, K-LOCK.cancel, K-GEN, K-ATOM.tbl-load, lifecycle-symmetry templates, retry-derived contracts | A and C are both open by construction; B is open only over observed executions |

---

## 4. Best ideas to graft

**From A:**
1. **The (B, σ, M, b) verdict with M_api vs M_impl.** It adds sub-labels for portability hazards and implementation-defect candidates (OSAL backends).
2. **Readiness predicates that call cFE/OSAL validity functions.** Writer and disabler sets are computed from the predicate's support locations. This feeds writer-set completeness for observation edges, which both B and C need.
3. **Violation shapes** WRONG-WAY, UNORDERED, ABSENT and STALE.
4. **Configuration closure from generated artifacts,** with table values cross-checked byte for byte against the built `.tbl` through GCC DWARF, and a closure-boot cross-check.
5. **Model conformance (trace ⊆ L(M)).** Fold it into C's O1, O2 and O6 as a precise, budgeted projection-inclusion check.
6. **An independent certificate checker** with mutation tests (a corrupted edge must be rejected).
7. **The K-CAP max-weight-closure backlog bound** as a tier-1 PROTECTED test for capacity.
8. **The LOSS-UNDER-ORDER label.**
9. **RQ5 on measured phenomena**, and perturbation group W (code-layout effect of the variant build).
10. **Build lints** (`CAIRN_REF(value, "path:line")`; no case identifiers in `lib/`).
11. **Lemma L1 (await summarization)** as an explicit, switchable proof obligation.

**From B:**
1. **Outcome mining.** Template-free candidates come from the `\retval`/`@retval` vocabulary.
2. **Summary-guided extrapolation** of retry and poll loops to their reference bounds in prediction.
3. **VC_R robust clock** for per-trace robust certificates, used as a cheap tier-1 on traces, with writer-set completeness supplied by A's predicates.
4. **Per-object tokens at in-critical-section probe points** (give entry), which recover lock order without timestamps or return probes.
5. **Debug-register execution breakpoints** for F0 holds (at most 4 per thread; no text modification), with hold-until-event and reference watchdogs.
6. **Entry+4 placement with MC-decoded cost classes**, and the minimal-plan perturbation group C′.
7. **The `timeout-dependent` class** (mapped onto C's σ_U vs σ_U∞).
8. **DataCollider-style watchpoint discovery** of direct global reads during publication windows.

**From C (the spine):**
1. **Profile Φ as data**, with version guards and the refutable hypotheses H-V and H-C.
2. **Strata S/O/C_σ**, plus σ_U∞ and the containment lemma.
3. **Consistency obligations O1–O6**, and suspension of rules on contradiction.
4. **On-behalf frames from stacks.**
5. **K-LOCKBAL, K-LOCK.cancel**, lifecycle-symmetry templates and retry-derived contracts.
6. **Explicit-search prediction with POR**, needing no solver.
7. **Wait-for oracle** for deadlock and lock-leak confirmations.
8. **A generic, data-driven BPF program** with site descriptors (no per-site code generation).
9. **The feedback-loop ablation.**

---

## 5. What the synthesis must fix

1. **Fixed-order certificate semantics.** Define and certify FIXED_ORDER as "no execution of M under σ in which any publisher of the required state precedes u". Do not certify it as dominance-based must-before.
   - Regression test: the R2 uses in `CFE_ES_TaskInit` sit behind early returns (`cfe_es_task.c` L336-393), and `CFE_ES_TaskMain` raises RUNNING even after an init failure (L118-133). The certificate must still go through.
   - Require writer-set completeness for every observation edge. Example: `MainTaskSyncDelay`'s predicate ranges over all used records.
   - State how terminal branches are modelled. Core `ExitApp(INIT_ERROR)` → `ResetCFE` (`cfe_es_api.c` L333-362) is not an instant stop (PSP restart aborts only after 10 s, [R01] §5.2).
2. **Specify the static ordering algorithm** on branching and looping skeletons. C's `≺_S` is underspecified. Use A's must-HB fixpoint corrected as in item 1, give its complexity, and run the independent checker.
3. **Lock-reversal prediction must not depend on an uninstalled solver.** Explicit search with POR is the default (C). SMT-LIB is an optional cross-check (A/B), behind PI decision OPEN-3/D5.
4. **Never derive HB from timestamps.** Drop B's A-CLK fallback ordering, or demote it to non-HB evidence. Recover per-object order from in-critical-section tokens (B) or matched protocol keys.
5. **Two OSAL model levels (M_api, M_impl)** with a portability-hazard sub-verdict. API-level edges only for shapes. OSAL-internal locks constrain interleavings only under M_impl.
6. **Readiness predicates bound to framework validity functions,** with writer sets computed, not declared. Keep header provenance: K-PHASE cites `cfe_es.h` L400-403, and K-ID cites `cfe_evs.h` L168-172.
7. **Latent paths get model verdicts, not just "unobserved".** Every census instance gets INACTIVE, NO_PROVIDER, a model class or UNKNOWN(reason). Restart, ground-command and table variants are declared B.Input / B′, never "new bugs".
8. **Correct NO_PROVIDER examples.** HS `linux_sysmon` is a platform-capability failure of `SET_RUNNING`, not a missing provider (`hs_sysmon.c` L50-72). Add a distinct consequence or class for "provider present, platform capability absent". Propagate the correction to [R03] §3 row 20.
9. **Version-robust site binding.** Do not use line-number probes. Resolve internal sites (record publications at `cfe_es_apps.c` ~L703/L869, `cfe_es_start.c` ~L803/L818, `cfe_sb_task.c` L138) by AST pattern plus the GCC DWARF of each basis. Rules carry version guards.
10. **Confirmation safety rules, made explicit:**
    - the held thread's program-lock set is empty, including the ES shared-data lock and the case `DeleteChildTask` under the ES lock;
    - no hold inside `handler_mutex` callbacks or inside binsem internal sections (the 2 s `timedlock`);
    - watchdog = the smallest reference timeout the hold could make expire;
    - label runs "timeout branch exercised (σ_U witness)" vs natural σ_E.
11. **Observation obligations before trusting traces:**
    - measure return-probe cost, BPF body cost and controller cost;
    - test uprobe pre-registration on a not-yet-`dlopen`ed app `.so` ([R07] §9 item 6);
    - write an own eBPF loader (no libbpf) or get an install decision;
    - get a decision on the root-side observer hook;
    - re-baseline group A on kernel `-fc-v114`.
12. **Route and capacity results** carry LOSS-UNDER-ORDER and harm `unspecified_by_reference`, with SB counters (`NoSubscribersCounter`, `PipeOverflowErrorCounter`) as the oracle. Include A's RQ5.
13. **Optimal DPOR with blocking operations** (awaits, locks, semaphores) needs the disabling-aware variant, whose JACM extension is status P in [R05] §2.2. Cite it with that status, or use source-DPOR with an explicit treatment of disabling. With a preemption bound, use BPOR (V-abs).
14. **Gate MLIR** against a plain-C++ arm on stated criteria (A's G-MLIR / C's T1). No checked-in generated code. The contribution must not depend on MLIR.
15. **Scope order:** census + tier-1 + trace HB + R2 re-observed by the new recorder first; then prediction, exploration and confirmation. The publishable core (R2 fixed-order certificate, route/capacity census) must not wait for the explorer.
16. **Anti-circularity bookkeeping:** template-only vs refined runs, freeze by hash before the held-out sources (a)–(e), and authoring timestamps per profile entry.
17. **Persistence and identity across runs:** B.Init snapshots, run sequences (PO → PR → PO), cross-run keys `(type, name, creator, creation ordinal)`, and four identities plus on-behalf frames.

---

## 6. Literature check

Every literature key used by the three proposals appears in [R04], [R05] or [R06], with the status tags they use:
- Vindicator, Velodrome, AVIO, FCatch, DESCRY, the Go memory model (V-doc), SHB, M2, SyncP, GPredict, MCM and DataCollider are in [R04].
- SIERRA, UBITect, Holzmann–Joshi, MLTA, SCTBench, Alglave, Wang et al. FM 2009, soundiness, Arcuri–Briand and Klees (trial count P) are in [R05].
- FlowDroid, Engler and typestate (V-me) are in [R06].

DCatch is cited as P, and PCT's mechanism as P, by all three, correctly. One status nuance no proposal states: optimal DPOR's extension to disabling (JACM) is P ([R05] §2.2). This matters because all three explore models with blocking operations (synthesis item 13).

## 7. Not verified here

- I did not re-run any measurement from [R07].
- I did not check the K-CAP maximum-weight-closure claim; A marks its citation as unverified.
- I did not check the SBN `ENABLE_SUB_REPORTING` vs TO_LAB burst attribution; it is an inference in [R03] §5.2.
- I did not check whether the F1 lock leak and F7 silent task loss are reachable at run time; they are inferences in [R01] §7.

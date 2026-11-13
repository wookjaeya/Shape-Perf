# J1 judgment: the three AR2 architecture proposals, read by a formal-methods PC reviewer

- Date: 2026-10-12. Judge J1. The lens is a PLDI/OOPSLA/ICSE/FSE program-committee reviewer. It asks four things: are the definitions and claims sound, how does the design relate to prior work (HB, predictive analysis, DPOR, event-driven and distributed detectors) and what is new, are the guarantees stated honestly, and is the core contribution real.
- Inputs: proposals A (`proposal_A-static-model.md`, "CAIRN"), B (`proposal_B-dynamic-predictive.md`, "Sextant") and C (`proposal_C-hybrid-layered.md`, "Kairos"), all read in full. Evidence base: [R01]–[R07] in `scratchpad/arch/`; `CONDITIONS_POLICY.md` [POL]; `results/R2_gdb_v701.md` [R2g]; `results/R2_cord_v701.md` [R2c]; `env/BASELINE_MEASURE.md` [BM]. Code was spot-checked read-only in the pinned tree `/home/user/work/procrace/cfs_ref/cFS`; §1 lists every check.
- Labels: **FACT** means verified in code, a record or a report, with a citation. **INFERENCE** means my reasoning; nothing was executed. **JUDGMENT** means a reviewer's assessment.
- Nothing was built, run, installed or modified. Literature is referred to only as [R04]/[R05]/[R06] verified it.

---

## 0. Verdict

| Design | validity | cfs_fit | openness | prior_work | feasibility | evaluation | **total /60** |
|---|---|---|---|---|---|---|---|
| A: model-first (CAIRN) | 8 | 9 | 8 | 9 | 5 | 8 | **47** |
| B: trace-first predictive (Sextant) | 6 | 7 | 6 | 7 | 7 | 7 | **40** |
| C: hybrid, layered, feedback (Kairos) | 8 | 9 | 9 | 8 | 6 | 8 | **48** |

**Winner: C, by a narrow margin over A.** The synthesis should keep C's architecture and replace C's tier-1 ordering engine with A's.

C's skeleton is the right one. It has one typed event model and one profile, interpreted three ways: static census, trace HB with prediction, and bounded exploration. Six consistency obligations tie the layers together. It also has the best semantic apparatus: σ_U, the timeout-free σ_U∞, σ_R1 and σ_E, with a containment lemma, and S/O/C_σ stratification of the ordering relations. C's weak point is the engine that has to make the S-stratum proofs sound. As written, C computes `≺_S` with "vector clocks over templates". That is a *may*-order construction, not a *must*-order analysis.

A specifies exactly that engine: a greatest-fixpoint MustBefore with meets over predecessors and spawn sites, T-OBS over complete writer sets, T-TERM, and an independent certificate checker. A also brings the M_api/M_impl distinction and model-conformance checking. Taken alone, A has the highest risk of never producing a result, and its σ_U-only semantics will flood the app level with SCHEDULE_DEPENDENT verdicts.

B is the most buildable, and it has several excellent observation and prediction ideas. Its universal (Exec-level) claims, however, rest on a trace-local "robust HB" plus a path-stability assumption that B cannot discharge for most uses. Its flagship worked example is misclassified, and its central lock-reversal class depends on a solver that is not installed.

**Is the core contribution real?** JUDGMENT: yes, if it is framed as all three proposals and [R06] §6.1 frame it. No single mechanism is new ([R04] §5). The defensible contribution has four parts:

1. **Formulation.** Typestate-style readiness contracts on shared framework resources, with allowed not-ready outcomes. Verdicts are quantified over (B, σ) and separate fixed-order, schedule-dependent and timeout-dependent.
2. **Stratified ordering.** A must/structural relation lifts an observed or static "ordered the wrong way" use to a fixed-order verdict with a checkable certificate, separately from may/observed synchronization.
3. **Key-level prediction.** The registry key is the conflict object and the lock gives atomicity, so the predictor may reverse lock order. This is explicitly outside SyncP's tractable class ([R04] §2.5).
4. **Cross-validation.** The static model and the traces of the unmodified binary check each other.

R2 is a real demonstration that matters. Every memory-level HB or predictive detector is silent on it by construction ([R04] L4), and A and C both derive its certificate correctly from code (§1, checks 1–2).

---

## 1. Verification performed (read-only)

| # | Claim used by the proposals | Checked at | Result | Consequence |
|---|---|---|---|---|
| 1 | Core serialization: `MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` per core entry; panic on failure | `cfe/modules/es/fsw/src/cfe_es_start.c` L776-846 (sync L835, `CFE_PSP_Panic` L845); `psp/fsw/pc-linux/src/cfe_psp_support.c` L116-121 (`abort()`) | **Confirmed.** On pc-linux the panic is terminal (`abort`), preceded by `OS_TaskDelay(CFE_ES_PANIC_DELAY)` (500 ms, `cfe_es_start.c` L55), during which other threads still run | T-TERM (A), OBS-FATAL (B) and guarded R19 (C) are justified. The terminal path creates no later core task, so it cannot contain SB's publication |
| 2 | A core task becomes RUNNING inside its own `WaitForSystemState` | `cfe_es_api.c` L528-576 | **Confirmed**, with one detail none of the proposals states: the raise happens only if `CFE_ES_GetAppRecordByContext()` is non-NULL (L533-576), i.e. only if the task's **App record is already final** | R2's certificate survives: if the raise is skipped, the 30 s sync times out and panics (check 1). It also exposes a new candidate (§1.1) |
| 3 | B's worked example 1: "No HB edge links `:869` to the child's first use → unordered → P1 (SHB)" | `CFE_ES_GetAppID` takes the ES lock (`cfe_es_api.c` L748-763); App-record finalization is under the ES lock (`cfe_es_apps.c` L861-869) | **Refuted as stated.** In any trace that probes `OS_MutSemTake/Give` (B does), the two critical sections are ordered by a LOCK edge, which B tags schedule-chosen. The pair is "ordered only via non-robust edges" (B §4.3, row 3), not unordered. Reversing it reverses the order of conflicting critical sections, which is outside WCP/SyncP ([R04] §2.5), so B routes it to P3 (SMT), and no solver is installed ([R07] §2.4) | B's flagship openness example yields `unknown(no-solver)` in B's own pipeline. A (tier 1: no must-order, so tier 2) and C (stratum O, lock-reversing search) classify it correctly |
| 4 | During the App-record window, `GetAppIDByName` returns `CFE_SUCCESS` with `0xFFFFFFFF` | `cfe_es_api.c` L637-656; `cfe_es_resource.c` L97 (`AppRecordIsUsed` test); `cfe_es_resource.h` L166-169 (`TEST_DEFINED`); `cfe_resourceid_api_typedefs.h` L74 (`RESERVED = 0xFFFFFFFF`) | **Confirmed** | A's and C's "RESERVED-sentinel success is a violation" outcome is grounded in code |
| 5 | OSAL lock-leak hypothesis F1 | `osal/src/os/shared/src/osapi-idmap.c` L384-385 (header says that on failure the lock is always released) vs L401-404 (early `return OS_ERR_INCORRECT_OBJ_STATE`) | **Text confirmed.** Whether this path is reached with the table lock held still depends on the caller chain ([R01] §1.8). All three proposals correctly keep F1 as a hypothesis | — |
| 6 | CF retry: stops on any code other than `NAME_NOT_FOUND`; 25 × 100 ms | `apps/cf/fsw/src/cf_cfdp.c` L1270-1301; `apps/cf/fsw/inc/cf_internal_cfg.h` L172-185 | **Confirmed**, with one caveat: 25 and 100 are the `DEFAULT_` values behind `CF_INTERNAL_CFGVAL(...)`. The effective bound must be constant-evaluated in the TU of the build (A §4.1 step 4 does this; B and C cite only the default line) | POL: the bound must cite the build's effective value |
| 7 | Doxygen contract material in headers | `grep` over `cfe/modules/core_api/fsw/inc/*.h`, `osal/src/os/inc/*.h` | 429 `\retval`, 138 `\par Assumptions`, 74 `\param[out]` (cFE); 593 retval lines (OSAL). **A is correct. B's "452 `\retval`" (§2.5, §A.2) is wrong** | Minor factual error in B |
| 8 | R2 records | [R2g] L36-56: 24 violating uses (ES 20, EVS 4), all on `CFE_ES_TaskMain`/`CFE_EVS_TaskMain`; [R2c] L164-168: total `sb.appid` uses 240–258, ordered 211–229 | **Consistent** with A and B. **C's "211–258 SB-AppId uses per boot" (§5.9) merges the ordered minimum with the total maximum** | Minor factual error in C |
| 9 | SB publishes its AppId through an out-pointer and ignores the status; the EVS precondition text | `cfe/modules/sb/fsw/src/cfe_sb_task.c` L137-138; `cfe/modules/core_api/fsw/inc/cfe_evs.h` L168-171 | **Confirmed** | The generic K-ID template is grounded in a header assumption |
| 10 | Effective queue capacities and losses | [BM] L11 (22 of 26 pipes truncated, 247 of 584 slots), L127-128, L147-156 (4–49 drops; A3 a different interleaving) | **Confirmed** | K-CAP/K-ROUTE scope is justified by measured phenomena |
| 11 | Tooling facts | [R07] §0, §2.2, §2.4, §3.4, §4.7; `/usr/include/linux/bpf.h` present | Z3, libbpf, bpftool and bpftrace absent; MLIR import 0.78 s; uprobe at entry+4 costs 0.57 µs, a single-stepped one 31.7 µs; raw `bpf()` load and attach probed | No design may put a solver or libbpf on its critical path without a PI install decision (B does, for P3) |
| 12 | Literature status | [R04] §2.5 L226-268 (SHB, WCP, M2, SyncP, Vindicator V-abs); [R04] L279, L316 (AVIO, Velodrome V-abs); [R05] L170 (Wang et al. FM 2009 V-abs), L139-140 (ODPOR, BPOR V-abs), L196 (SIERRA), L227 (MLTA); [R06] L790-798 (RaceMob, FlowDroid, Engler V-me) | **No invented citation found** in any proposal. One characterization error in B: WCP is described as *deciding* reversibility (B §4.4 P2), but [R04] records WCP only as *sound* and linear-time, and only SyncP as sound and complete for its class | B's wording must be corrected |

### 1.1 A candidate that the verification exposed (INFERENCE, code reading only, not run)

Core tasks start through the same trampoline as apps: `CFE_ES_StartAppTask` → `OS_TaskCreate(..., CFE_ES_TaskEntryPoint, ...)` (`cfe_es_apps.c` L664-671). A core task's App record is finalized only after `StartAppTask` returns, under the ES lock (`cfe_es_start.c` L810-818). The child's `CFE_ES_GetTaskFunction` checks only the task record, after a 50 ms delay (`cfe_es_apps.c` L570-590).

So under σ_U, a core task's `*_TaskInit` can call `CFE_ES_GetAppID` before L818. EVS does this at `cfe/modules/evs/fsw/src/cfe_evs_task.c` L273. The call returns `CFE_ES_ERR_RESOURCEID_NOT_VALID`, the task treats it as an init error, and ES requests a processor reset (`cfe_es_api.c` L333-352). Separately, if the task reaches its `WaitForSystemState` raise inside the window, the raise is skipped (check 2), and the core sync times out and panics.

This is the core-task twin of the script-app window that A §A.3 and C §A.3 discuss. None of the three lists it, but a generic K-SPAWN census (A, C) should produce it. B would see it only through lock-reversing prediction, which needs P3. It belongs in the RQ1 open-census test set as a candidate. Its σ_E frequency is unknown, because it is timing-protected by a 50 ms sleep.

---

## 2. Cross-cutting formal issues (they affect all three designs)

**X1. Linearization points (LP) of uses and publications are undefined.** A evaluates `Ready` "immediately before u" (A §1.3). B and C evaluate the state "after the transitions that precede u". None of them fixes *where* inside a non-atomic API the use reads, or the publication writes, the registry. The difference is material in exactly the windows these tools target:
- OSAL name visibility comes before finalize ([R01] §1.5);
- `CFE_SB_CreatePipe` makes the OSAL queue visible before the SB descriptor commits ([R02] §1.16);
- `GetAppID` reads the App record under the ES lock (check 3).

Fix: give each API in the profile a declared LP for each effect (e.g. "the registry read inside the critical section"). Evaluate `Ready` at the LP. Use the outcome map as a consistency check. Under M_api (A), make the whole API atomic at that LP.

**X2. "Allowed" outcomes quantify over continuations, but the horizon is unstated.** Examples are "tested_by_caller", "retry until success within the bound" and "error_propagated". The horizon must come from a reference: the retry bound, the next operation on the same resource, task exit, or the main-loop horizon h. It must also say how each layer evaluates the property (static path check, trace continuation, explorer suffix). A's "bounded continuation of θ(u)" is the closest to a definition.

**X3. σ_U alone floods the app level.** [R06] §0 item 2 and T5 warn that under untimed semantics "almost everything becomes schedule-dependent". A defines only σ_U, σ_R1 and σ_E. A reports margins as σ_E evidence, but it has no class separating "violable only through a soft-barrier timeout" from "violable in the ordering logic". C's σ_U∞ (with `stall`) and B's `timeout-dependent` class both solve this.

The synthesis needs σ_U∞ *and* a timeout-dependent label. σ_U∞ also needs a stated fairness notion: without timeouts, unbounded polls need weak fairness, or the run ends in a `stall` classification.

**X4. Must-order and writer-set completeness are soundness-critical.** All three proofs of R2 use an "observation" edge: the success exit of `MainTaskSyncDelay` → whatever precedes the write it observed. That edge is sound only if the writer set of the observed predicate is complete, *including the initial value* and writes through pointers.
- A states the rule correctly (T-OBS over a complete `W_o`, disabled if an `unknown_effect` may write). But it computes `W_i`/`D_i`/`W_o` as "stores that can make the predicate true" with no specified mod-ref or points-to analysis. That analysis must be field-based and memset-aware: for example, `memset(TaskRecPtr, 0, …)` at `cfe_es_apps.c` ~L690 and `memset(&CFE_ES_Global, 0, …)` at `cfe_es_start.c` L82.
- C writes the writer set into the rule (`writers es.appstate(a) := RUNNING only by actor main(a)`, checked statically). That is good, but the check itself is not specified.
- B's OBS-FATAL has no writer-set condition at all; it relies on "path stability".

**X5. Lifting trace order to Exec-level claims needs an all-paths argument.** A trace shows one path per thread. "Fixed-order in every execution" needs every path to every publisher (A's MustBefore; C's "every non-terminal path to p passes the guarded edge"). B's G4 replaces this with "the same per-thread paths". For init code, where every step is control-dependent on earlier API statuses (shared reads), that assumption is almost never discharged by B's site summaries. So B's R2 claim in §4.3 ("fixed-order under σ_U (and therefore σ_ENV and σ_RT1)") is stronger than its own G4 supports.

**X6. DPOR exactness with local nondeterminism.** A §6.1 G3 and C G5 claim "one execution per Mazurkiewicz trace, hence exact within b". Optimal DPOR's guarantee is for deterministic per-thread transition systems ([R05] §2.2). The skeletons contain thread-local nondeterminism: havoc values (A), star-closure and nondeterministic data branches (C), and await outcomes success/timeout under σ_U. The claim needs one of two treatments: (a) enumerate local choices as part of the explored state, with the standard extension and its proof cited or stated; or (b) a lemma. The dependence relation must also include await-predicate reads against the writes that enable them (A states this; C's "same resource or same sync object" does not, unless every polled location is a declared resource).

**X7. The soundness domain of predictive engines.** SHB, WCP, M2 and SyncP are data-race relations over memory accesses with lock semantics ([R04] §2.5). Applied to an abstract trace (lifecycle as write, use as read), their guarantees transfer only under the abstraction's faithfulness (B's A-API). Even then, none of them predicts a reversal of *conflicting* critical sections, and that is the central cFS class (C1, C5, the App-record windows). The synthesis must use them only as cheap prefilters where they apply, and never call a pair "protected" or "not reversible" because they found nothing. The decider for lock-protected readiness pairs is a lock-reversing search (C §4.6, A §4.7), relative to Pred(π) or to the model.

**X8. The model–binary link.** A's global conformance check (a model execution whose projection equals the observed per-thread sequences, A §4.8) is the strongest link, but it is a search problem. C's per-actor acceptance (O1/O2) is cheap but ignores cross-thread consistency. The synthesis should run per-actor acceptance on every trace and global conformance within a recorded budget. Non-conformance → `unknown(model_gap)` for every verdict whose slice touches the region. Note that an over-approximate model (havoc, star-closure) accepts more traces, so conformance is necessary evidence, not sufficient.

**X9. Who defines "ready" and the publishers (circularity).**
- A: "ready" is defined by calling the framework's own validity predicates (`CFE_ES_AppRecordIsMatch`, …), and the publishers are computed. This minimizes author bias but needs X4's mod-ref analysis.
- C (and B): the profile *declares* lifecycle effects per API (trusted, with O3/O4 runtime refutation).
- Best: declared effects as the working model, framework predicates as an independent oracle, and agreement between the two as a consistency obligation.

**X10. Transfer of verdicts across σ.** A says "no transfer across σ" (§6.3 item 3). That is too conservative. With the subset relations C states (σ_R1, σ_U∞ and σ_E ⊆ σ_U under assumption A7):
- universally quantified verdicts (protected, fixed-order) transfer *down* wherever the use stays reachable;
- witnesses transfer *up*;
- nothing transfers into σ_E as a frequency.

C's containment lemma states this correctly. B's §4.3 transfer of R2 to σ_ENV and σ_RT1 is valid *given* the Exec-level certificate it does not fully have (X5).

---

## 3. Proposal A (CAIRN, model-first)

**What it is.** It builds a configuration-closed model of the real build:
- compile DB and `link.txt` closure;
- generated core object table, startup script and tables, cross-checked against the `.tbl` bytes;
- whole-program LLVM IR sliced into an MLIR `cfsk` skeleton dialect with `await` summaries.

Generic RCL contracts are instantiated by a static census. A tier-1 must-HB fixpoint emits checkable certificates. A tier-2 optimal-DPOR explorer answers the two queries, with SMT optional. The dynamic layer — F0 uprobe/eBPF observation, conformance checking, ptrace holds and an optional `--wrap` PCT arm — validates the model and confirms uncertain verdicts.

**Soundness of definitions and claims.**
- The verdict lattice over (B, σ, M, b) is the most carefully stated of the three (A §1.6). INACTIVE and NO_PROVIDER are kept apart from PROTECTED; HANDLED, LOSS-UNDER-ORDER, portability hazard and implementation-defect candidate are sub-labels; evidence is an orthogonal axis.
- The tier-1 FIXED_ORDER argument (A §4.4) is a correct first-occurrence argument. If `s_u ∈ MustBefore(x)` for every `x ∈ W_i`, then at the first occurrence of `s_u` the conjunct `P_i` is false. The definition makes MustBefore *strong* (u must occur before x in every execution where x occurs), so conditional uses fall to tier 2: incomplete, but sound.
- T-OBS with complete writer sets and T-TERM are exactly the right rules (X4). The independent certificate checker, which shares no code with the solver, is a reviewer-pleasing design.
- Lemmas L1 (await summarization) and L2 (irrelevant critical sections) are stated as proof obligations and switchable per instance. The L1 sketch correctly refuses to summarize OSAL lookups under M_impl, because lookups can leak the lock and their unlock broadcasts consume waiters' attempts.
- The shape axis (WRONG-WAY / UNORDERED / ABSENT / STALE, A §1.7) precisely separates R2's class from race-shaped instances.
- **M_api vs M_impl** (A §1.5) is a genuinely good formal idea: "protected under M_impl but violable under M_api" = portability hazard; "violable only under M_impl" = implementation-defect candidate. It answers [R01] §7 item 6 instead of hiding it.
- Gaps: X1 (LP), X3 (no σ_U∞ / timeout class), X4 (mod-ref unspecified), X6 (DPOR with havoc and timeouts), X10 (over-conservative non-transfer).

**Relation to prior work.** Excellent. A's §1.9 table pairs each ingredient with its source and with what is added: SIERRA-style static action HB, the CAFA/DCatch framework rules, Tian/Xiong ad hoc synchronization, optimal DPOR, BPOR, Alglave's encoding, MCM/RVPredict/GPredict prediction, and the RacePro/CTrigger confirmation precedents. It claims no mechanism novelty. Max-weight closure for the K-CAP upper bound is flagged as an algorithmic fact whose citation AR1 did not verify, which is the correct handling.

**cFS fit.** The deepest of the three. It covers:
- idmap states with name visibility;
- the trampoline's two handshakes;
- core serialization taken from the *generated* table;
- soft barriers modelled as declaration plus `await`;
- the SB three-step pipe creation;
- EVS with two not-ready kinds;
- TBL's two load paths;
- TIME callbacks with `α_attr`;
- persistence as `Init`;
- restart as a scenario family.

Bounds cite code constants (A §4.10). `GetAppIDByName`'s sentinel is handled through an `IsMatch` predicate.

**Openness.** A static census of every template instance across core and 15 apps, all 15 RC kinds mapped, belief-deviance ranking, Snowboard-style per-app profiling, configuration variants as new bases, and a freeze plus template-only first run. It has no template-free discovery: B's outcome mining is missing.

**Feasibility (the weak point).**
- The `cfsk` interpreter must evaluate framework C predicates (`call @CFE_ES_AppRecordIsMatch(...)`) on model state. That makes it effectively an interpreter of sliced IR with tracked memory, pointers and GEPs.
- M_impl must model OSAL POSIX code over pthread, semaphore and mqueue semantics.
- Global conformance is a search.
- MLIR adds ODS/TableGen build-time generation (OPEN-2 against "no messy code generation").
- None of this has been prototyped. A's G-MLIR gate and its fallback to plain LLVM IR are sensible, but the core risk is the skeleton's fidelity and cost, not MLIR.
- In A's favour, no solver is needed.

**Evaluation.** Strong: seven RQs, including model fidelity (conformance rate, anchor audit, closure statistics, havoc counts, certificate-checker pass rate); eleven ablations; perturbation groups including the variant-layout group W; denominators; two raters.

**Fatal flaws (as written).**
1. **Writer-set completeness depends on an analysis A never specifies.** The publishers W, disablers D and observation writer sets `W_o` are "the stores that can make the predicate true". Without a specified, memset-aware, field-based mod-ref analysis over the closed IR, T-OBS is either unsound (sets too small) or disabled everywhere (every load becomes `unknown_effect`). Either way the certificates, A's headline contribution, cannot be produced.
2. **σ_U as the only model semantics.** Every instance behind a soft barrier, including CF's retry window and K-PHASE behind 1000 ms barriers, becomes SCHEDULE_DEPENDENT. That is the flood [R06] T5 warns about. It is fixable by adding σ_U∞ or a timeout-dependent class, but as written it makes app-level verdicts uninformative.
3. (Severe risk rather than flaw.) A whole-program sliced-IR interpreter, plus M_impl, plus global conformance search, is the largest engineering commitment of the three, and none of it has been prototyped.

**Best ideas to graft from A.**
- The MustBefore greatest fixpoint with rule ids (T-PO, T-SPAWN as a meet over spawn sites, T-OBS over complete writer sets including the initial value, T-TERM). The first-occurrence FIXED_ORDER argument. An independent certificate checker with mutation tests (a corrupted edge must be rejected).
- M_api/M_impl, with the portability-hazard and implementation-defect labels.
- Readiness defined by the framework's own validity predicates, used as an independent oracle.
- Shapes WRONG-WAY/UNORDERED/ABSENT/STALE; violation chains; HANDLED and LOSS-UNDER-ORDER sub-labels; an orthogonal evidence axis.
- Lemmas L1/L2 as explicit, switchable proof obligations.
- Model conformance (trace ∈ L(M)), with non-conformance → UNKNOWN(conformance).
- The K-CAP sound upper bound by maximum-weight closure over the MustBefore order.
- The gate-placement rule (empty lockset under M_impl; never in a signal handler, a timer callback or before `TaskPrepare`; reference-bounded watchdog) and the per-protocol T_ref table.
- `CAIRN_REF(value, "path:line")` with a lint against bare constants; a lint against case identifiers in `lib/`.
- Table constant-evaluation cross-checked byte for byte against the built `.tbl` using GCC DWARF layouts.

---

## 4. Proposal B (Sextant, trace-first predictive)

**What it is.** Static analysis is confined to probe placement and site summaries. F0 uprobes run on the unmodified gcc binary. A lifter builds a typed event IR with four identities. A cFS-aware HB is computed with two vector clocks (all definite edges; "robust" unconditional edges only). Tier 1 checks contracts on ordered and unordered pairs. Prediction uses P1 (SHB), P2 (WCP/SyncP/M2) and P3 (SMT). Confirmation holds threads at debug-register breakpoints until an event occurs.

**Soundness of definitions and claims.**
- The trace-relative classes are honest. `fixed-order` and `protected` are "relative to Pred(T)", and Exec-level claims are reserved for robust certificates (B §1.5, §4.5). G1–G3 (observation fidelity, no fabricated order, exact ordered/unordered status on T) are sound as stated.
- **The robust-HB certificate is the problem** (X4, X5).
  - VC_R is computed on one trace. OBS-FATAL takes "the write that established s" in that trace, with no writer-set completeness condition.
  - G4 then needs "the same per-thread paths", and B discharges that only where reaching a site is not control-dependent on a shared read. For init code that is almost never true.
  - So B's own §4.3 conclusion for R2 — "every edge is unconditional, so … fixed-order under σ_U (and therefore under σ_ENV and σ_RT1)" — is stronger than G4. As B's machinery stands, it is an assumption-laden claim.
- **Worked example 1 is misclassified** (check 3). The App-record window is ES-lock-ordered in any trace with mutex probes, so it is not unordered and not an SHB case. It needs a reversal of conflicting critical sections, i.e. P3, i.e. a solver.
- P2 overclaims: WCP does not "decide" reversibility; it is sound and incomplete ([R04] §2.5).
- Summary-guided extrapolation (B §4.4 (e)) adds iterations that are not in T. That is model-based reasoning, so witnesses produced through (e) are no longer trace-sound. B acknowledges this under A-API, but the classification table should mark such witnesses separately.
- σ_ENV "admits the same interleavings as σ_U" is labelled [I]. It is acceptable as a modelling stance, not as a fact.

**Relation to prior work.** B has the richest predictive-analysis framing (MCM/RVPredict control-flow abstraction, GPredict properties, SHB, WCP, M2, SyncP, CloudRaid, Snowboard), and it is correct that sync-preserving prediction excludes the C1/C5 class. The novelty claim is the combination and is honestly stated. Deductions are for applying data-race relations as deciders on an abstract trace (X7) and for "decide".

**cFS fit.** A good event-level mapping (B §3), including the RESERVED-window lookup outcomes, OBS-FATAL vs OBS-SOFT barriers, the looper rule for main loops, the global SB mode, and lossy queues. Weaknesses:
- the ES-lock misreading;
- the TIME seqlock left unmodelled;
- loader and configuration knowledge available mostly from observation;
- `inactive(B)` needs a "configuration summary" whose analysis is not specified.

**Openness.** Mixed.
- Strong: outcome mining needs no template, so every not-ready code at the API boundary becomes an instance; key-pair enumeration; CloudRaid "always one order" pairs; ad hoc synchronization discovery; census denominators.
- Weak: verdicts exist only for observed executions. Latent and configuration-gated instances are `unobserved` until a variant is run, and B states that prediction "is not complete … limited by the observed paths". Cases like §1.1 are found only through P3.

**Feasibility.** The best of the three for the first results.
- A small static footprint.
- A tracefs fallback needs zero installs, with A-CLK recorded.
- Entry-only lock reconstruction avoids unmeasured return probes.
- Its M1 milestone, re-observing R2 with the new recorder, is the right first deliverable.
- But the central lock-reversal class sits behind P3, which needs a solver install. Without it those instances are `unknown(no-solver)`.
- Debug-register gates allow at most 4 points per thread and cost about 34 µs per trap; that is acceptable for confirmation.

**Evaluation.** Good. RQ3 (predicted then confirmed vs merely observed), baselines reimplemented over the same traces (FastTrack/WCP/SHB/DCatch/RacePro observation models, all labelled), ablations including "conditional edges treated as unconditional", perturbation groups. Missing: model-fidelity metrics, because there is no static model to check.

**Fatal flaws (as written).**
1. **No sound route from trace to Exec-level fixed-order or protected verdicts.** Robust HB is computed on a trace without writer-set completeness, and path stability is undischargeable for most uses. B's strongest universal claim, on R2, exceeds its own G4.
2. **The decisive lock-reversal class depends on an uninstalled solver**, and the flagship openness example is misclassified (check 3). Without P3, C1/C5-shaped instances and both App-record windows end as `unknown(no-solver)`.

**Best ideas to graft from B.**
- **Robustness tags on HB edges** (unconditional / observe-then-act fatal / observe-then-act soft / schedule-chosen / assumed) and the second vector clock VC_R. In the synthesis these become the trace-side image of C's S/O strata, with VC_R as a cheap first test before the static proof.
- **The `timeout-dependent` class** (needed per X3).
- **Outcome mining** (template-free instances from not-ready codes) and **key-pair enumeration**.
- **Entry-only per-object lock tokens.** The give-entry probe runs inside the critical section, so give order equals critical-section order. This avoids return probes at hot sites.
- The tracefs zero-install fallback with A-CLK recorded in B; per-thread `tseq` and per-thread loss → `unknown(loss)`.
- Debug-register execution breakpoints for gates (no text modification), with release on an event and `inconclusive(await-timeout)`.
- Summary-guided extrapolation to reference retry/poll bounds, kept but labelled as model-based.
- Baselines reimplemented over the same traces and labelled; a perturbation group C′ (minimal R2 plan).
- M1 = "R2 re-observed by the new recorder", the first exit criterion.

---

## 5. Proposal C (Kairos, hybrid layered with feedback)

**What it is.** One typed event model (KEM) and one profile DSL Φ, which holds resource kinds with lifecycle automata, operations with probe bindings, HB rules tagged with stratum S or O, and contracts with allowed outcomes. Φ is interpreted three ways:
- abstractly, over static event skeletons (census and `≺_S` proofs);
- concretely, over F0 traces (HB_π, contract checking, lock-reversing prediction by explicit search with sleep sets, no solver);
- operationally, in a source/optimal-DPOR explorer over a closed event-level model.

Confirmation uses ptrace holds, with an optional `--wrap` arm. A feedback loop and six consistency obligations (O1–O6) close the layers.

**Soundness of definitions and claims.**
- The best semantic apparatus of the three:
  - σ_U, **σ_U∞** (timeout-free, `stall` instead of a verdict), σ_E and σ_R1;
  - a **containment lemma** with correct transfer rules (X10);
  - **stratification** S (structural, every execution on the path), O (observed sync, reorderable) and C_σ (schedule constraints, never HB);
  - a proof obligation per class (C §1.9);
  - a three-valued per-use state on traces (ordered-conforming, ordered-violating, unordered), with the program's own outcome code required to agree (O4).
- Assumptions A1–A9 are enumerated; G1–G9 are qualified; the "not claimed" list is complete.
- **Main formal gap: the `≺_S` algorithm** (C §4.3). "Assigning per-actor clocks to the (loop-summarized) skeleton DAGs and propagating S-edges, i.e. vector clocks over templates; path conditions are carried on edges." Vector-clock propagation over a template graph computes the existence of a path of edges, which is a *may*-order. "On every path to t" needs a meet over alternative predecessors and alternative spawn sites (A's T-SPAWN). As written, an S-proof could be accepted when the publisher lies on only one branch of a merge. The prose of the proof obligations is right; the algorithm does not implement it.
- Writer-set completeness is in the rule syntax (`writers … only by actor main(a)`), but its static check is unspecified (X4).
- The publisher closure ("P is closed") trusts Φ (X9). O3/O4 refute it only when a trace contradicts it.
- The dependence relation and local nondeterminism in DPOR are not addressed (X6). Skeleton over-approximation (nondeterministic data branches, star-closure) produces spurious model executions. The `model(b)` evidence label is honest, but the explorer's schedule-dependent verdicts will include infeasible ones, so O6 and confirmation carry real weight.
- The loop's "join is monotone" claim conflicts with rule suspension (O3/O4 withdraw proofs). C handles this by restarting on a new Φ hash, so the termination argument should be stated per Φ version.

**Relation to prior work.** Good and accurate: Vindicator's candidate-plus-vindication split, the CAFA/DCatch looper rule, MCM/RVPredict, InitRacer, EventRacer race coverage (reporting the root synchronization pair rather than its dependents), Velodrome/AVIO for K-ATOM, SDV and FlowDroid environment models, Holzmann–Joshi harness principles at event level. No mechanism novelty is claimed. Slightly less precise than A on what each ingredient contributes.

**cFS fit.** As deep as A's, plus:
- on-behalf frames (`svc` as a frame stack: TIME synch callbacks, FS callbacks in `ES_BG_TASK`, timer callbacks);
- K-LOCKBAL derived from OSAL's own documented invariant (`osapi-idmap.c` L384-385);
- `WaitForSystemState` split into a publication (the raise) and a conditional consumption;
- RESERVED counted as used and STOPPED counted as past in `MainTaskSyncDelay`;
- the looper rule;
- capacity from B.

Facts are well cited. One minor numeric slip (check 8), and the CF bound cited from the default line (check 6).

**Openness.** The strongest. A static census plus trace instances; templates derived automatically from retry/timeout loops; lifecycle symmetry (every kind with create/delete automatically gets K-REG, K-LIFE and K-GEN); belief deviance as a ranking only; O5 (an observed key outside the static universe means the closure is incomplete); configuration variants; stated generalization hypotheses (H-V, H-C, H-F) with refutation criteria; template-only and refined runs.

**Feasibility.** Moderate.
- C is the union of a static layer, a trace layer, an explorer, a predictor and a loop. Each part is lighter than A's equivalent: event-level skeletons rather than an IR interpreter, declared effects, no MLIR, and no solver (explicit search).
- The milestones decide R2 by M4.
- Risks: scope; eBPF features not yet probed (ring buffers, task storage, `bpf_get_stack`, IP-keyed descriptors; C lists perf-sample fallbacks); and `objseq` assigned at the mutex *take return*, which needs return probes whose cost is unmeasured. B's entry-only scheme avoids that.

**Evaluation.** Strong: RQ1–RQ6, a loop ablation (keys and targets resolved and unknowns removed per iteration), obligation failures per iteration as a fidelity metric, labelled reference detectors, perturbation groups A–E, the Arcuri–Briand statistics guidance.

**Fatal flaws (as written).**
1. **The `≺_S` computation is a may-analysis presented as a must-analysis** (vector clocks over templates, with no meet at merges or over spawn sites). If it were implemented as described, S-proofs of `protected`/`fixed-order` could be unsound. It is fixable by adopting A's MustBefore fixpoint. Until then, C's G2 is not supported.
2. (Severe risk rather than flaw.) Scope: three interpreters, a DSL, a loop and an observer. Without a staged minimum viable core, the paper risks shipping only the R2 part. C's own M0–M4 milestone line mitigates this, if it is enforced.

**Best ideas to graft from C.**
- **The one-core architecture.** KEM plus Φ interpreted statically, concretely and operationally; a single type for static templates and dynamic events, with a concretization relation (`t ⊒ e`).
- **σ_U∞ and the containment lemma**, with explicit transfer rules.
- **S/O/C_σ stratification.**
- **Consistency obligations O1–O6.** Each layer is a test oracle for the others; a contradiction suspends the rule and is reported as a finding about the tool.
- A lock-reversing explicit-search predictor with sleep sets and memoization, and no solver dependency. SMT-LIB is optional cross-checking only.
- Outcome-implied reads-from (the DefUse idea) as an O-edge. On-behalf frames from frame-pointer stacks.
- K-LOCKBAL from the code's own documented invariant, with a wait-for oracle rather than a timeout.
- Retry-loop-derived and lifecycle-symmetry templates.
- A generic BPF program per phase, driven by site descriptors (no per-site code generation).
- A probe plan by weighted set cover over obligations, with its cost reported.
- The "derived" labelling of every confirmation that needs a constructed schedule.

---

## 6. Winner and rationale

**C wins (48 vs 47 vs 40).** The deciding considerations, from this lens:

1. **The semantics of the verdict space.** C is the only proposal whose semantic family (σ_U, σ_U∞, σ_R1, σ_E with containment) gives informative verdicts at the app level. That is where the soft barriers live, and where [R06] T5 predicts that σ_U alone collapses into "schedule-dependent". Validity questions are decided here first.
2. **Validity by cross-checking.** The obligations O1–O6 make the static model, the traces and the explorer refute each other. A has conformance (one direction); B has refutation by confirmation only. For a tool whose rules are authored, layer-to-layer refutation is the strongest answer a PC can be given to "how do you know your model is right?".
3. **Openness without overreach.** C combines A's static census with trace instances and loop-derived templates, and it keeps an "unknown" reason for everything else.
4. **C's main flaw is local and fixable** by adopting A's MustBefore engine. A's main flaws (the unspecified mod-ref, σ_U-only semantics, the sliced-IR interpreter risk) are spread through the whole design. B's flaws reach its core claim: there is no sound trace-to-Exec lift, and lock reversal is solver-gated.

A is the essential donor: the synthesis should be "C's architecture, A's tier-1 engine and model-level rigor, B's observation and prediction details".

---

## 7. What the synthesis must fix (prioritized)

1. **Replace C's `≺_S` with A's MustBefore fixpoint.** Greatest fixpoint with meets over predecessors (T-TERM excluding terminal branches), T-SPAWN as a meet over all spawn sites, T-OBS only with a complete writer set *including the initial value*, and no rule for locks alone. Use the first-occurrence FIXED_ORDER argument, and add an independent certificate checker with mutation tests. State the soundness lemma (certificate ⇒ verdict over Exec(B, σ_U) under the listed assumptions) and prove it in the paper.
2. **Specify writer-set and mod-ref computation.** Field-based and memset-aware over the closed IR, including out-pointer stores across calls (e.g. `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, `cfe_sb_task.c` L138). Cross-validate against Φ-declared lifecycle effects (agreement is an obligation). T-OBS is disabled when an `unknown_effect` may write. Report counts of disabled rules.
3. **Semantics: σ_U, σ_U∞, σ_R1, σ_E,** plus a `timeout-dependent` label (B) and `stall` (C). State the fairness used for σ_U∞, and the containment lemma with assumption A7. Transfer rules: universal verdicts transfer down where the use is reachable, witnesses transfer up, nothing transfers into σ_E frequencies. Drop A's blanket "no transfer".
4. **Linearization points.** For each API in Φ, declare the LP of each effect (publish, use, retire) inside the API. Evaluate `Ready` at the LP and use outcome codes as a consistency check (O4). Keep A's M_api (atomic at the LP) vs M_impl (OSAL internals) with the portability-hazard and implementation-defect labels.
5. **Allowed-outcome semantics.** A finite vocabulary (C), each entry with a continuation horizon taken from a reference (the retry bound, the next operation on the same resource, task exit, or the main-loop horizon h). `unspecified_by_reference` → harm unspecified; LOSS-UNDER-ORDER for route and capacity contracts without a must-deliver reference.
6. **DPOR correctness statement.** Treat local nondeterminism (havoc, await outcomes, star-closure) explicitly in the explored state. The dependence relation must include await-predicate reads against enabling writes, and enabling/disabling pairs (spawn, give/take, put/get). Under σ_R1, use A's conservative priority dependences. Use BPOR whenever a preemption bound is introduced. Sweep and report every bound (POL).
7. **Prediction.** The decider for lock-protected readiness pairs is C's lock-reversing explicit search with the registry key as the conflict object (an RVPredict/GPredict-style Pred(π)). SHB, WCP and SyncP are prefilters only where their domain applies, and never deciders. Remove "decide" wording. Label witnesses that use extrapolation beyond T as model-based. No solver on the critical path.
8. **Model–binary link.** Per-actor skeleton acceptance (O1/O2) on every trace, plus A's global conformance within a recorded budget. Non-conformance → `unknown(model_gap)` for every affected verdict. A model class contradicted by an observation becomes UNKNOWN with a model-fault record and is never silently overridden (A §2.5).
9. **Verdict record.** Keep the axes orthogonal: class (inactive / no-provider / protected / fixed-order / schedule-dependent / timeout-dependent / unknown(reason)), σ, M level, bound b, B hash, Φ hash, evidence (static proof / model(b) / predicted / observed n-of-N / confirmed F0/F1, marked derived), shape (WRONG-WAY / UNORDERED / ABSENT / STALE), consequence, harm, chain links. Unknowns always carry a reason. No class means "safe".
10. **Correct the worked examples and facts.**
    - The script-app App-record window is ES-lock ordered (stratum O), not unordered; it needs a lock reversal (check 3).
    - Add the core-task App-record window (§1.1) as an open-census test, labelled INFERENCE until run.
    - 429 `\retval`, not 452.
    - SB-AppId uses are 240–258 total, of which 211–229 are ordered.
    - The CF bound is the build's effective `CF_INTERNAL_CFGVAL` value, constant-evaluated.
11. **Staged feasibility (a minimum viable core first).**
    - **(i)** Closure and a plain-C++ skeleton (no MLIR), tier-1 MustBefore with certificates, the K-ID/K-SPAWN/K-REG census, and the F0 observer (tracefs fallback first, then a raw-`bpf()` loader) with B's entry-only lock tokens. Trace HB plus contract check. Exit: R2 FIXED_ORDER certificate checked, and R2 re-observed by the new recorder.
    - **(ii)** Explicit-search predictor and explorer.
    - **(iii)** Confirmation.
    - **(iv)** The loop with obligations.
    - MLIR only behind A's G-MLIR gate. The F1 `--wrap` arm and native PCT only after a PI decision.
12. **Anti-circularity.** Φ effects (declared) vs framework validity predicates (A, computed) as two oracles, with agreement checked. Every template carries provenance, an authoring time and which cases had been inspected. Freeze by hash; run template-only first; label refinements.
13. **Openness.** The union of discovery sources: static census (A/C); outcome mining and key-pair enumeration (B); retry-loop and lifecycle-symmetry templates (C); belief-deviance ranking (all, a ranking only); configuration variants as new bases; the held-out protocol (a)–(e) of [R06] §5.6.
14. **Evaluation.**
    - RQs on openness, development cases (template-only vs refined), held-out results, fidelity (conformance rate, obligation failures, certificate-checker results, writer-set disablement counts), measured phenomena (K-CAP/K-ROUTE against SB counters), cost and perturbation.
    - Ablations: σ_U∞ off, robustness tags off, M_api vs M_impl, the loop, identities collapsed, conditional edges made unconditional, no tier 1, no prediction.
    - Baselines reimplemented over the same traces and labelled; external tools stay `NOT_RUN`.
    - Two raters with an agreement statistic; denominators everywhere; N runs `unspecified_by_reference` and set by the PI.
15. **Cost honesty.** Report analysis, exploration, recording, control and authoring costs separately. List unmeasured unit costs (return probes, BPF body, purpose-built ptrace holder) and measure them before any overhead claim. Re-measure group A on the current kernel (`-fc-v114`, [R07] §1).

---

## 8. Claims the synthesis must not make (in addition to [R06] §6.3)

- "Fixed-order" from a trace-local robust chain without an all-paths static certificate (X5). "Protected" from silence of SHB, WCP or SyncP (X7).
- "Exact within b" for DPOR without the local-nondeterminism treatment (X6).
- "No transfer across σ" (too weak), or σ_U-to-σ_E frequency transfer (too strong). Use the containment rules.
- Any count, bound or frequency not tied to a reference or a measurement (POL). That includes the CF bound taken from a `DEFAULT_` line instead of the effective configuration value.
- The OSAL F1/F7 hypotheses, or the core-task window of §1.1, as bugs before confirmation runs.

# Judgment J3: systems, experimental methodology and feasibility

- Date: 2026-10-12. Judge lens: OSDI/EuroSys/ISSTA artifact-evaluation reviewer. I asked whether each design can be built in this environment ([R07]), how much observation perturbs the run and how much it costs, how clean the architecture and interfaces are, how sound the evaluation design is (baselines, ablations, held-out data, statistics), whether results are reproducible under `CONDITIONS_POLICY.md` ([POL]), and how large the risk of state explosion or manual effort is.
- Inputs: proposals A (CAIRN, model-first), B (Sextant, trace-first) and C (Kairos, hybrid); reports [R01]–[R07]; [POL]; `env/BASELINE_MEASURE.md` ([BM]); the pinned tree `/home/user/work/procrace/cfs_ref/cFS` for spot checks. The other judge's file (`judgment_J2-cfs.md`) was opened only after my scores were fixed, to read its headline. Its two main findings agree with mine (§2.1, §2.2). Those findings were reached separately: I verified them against the code below.
- Labels: **FACT** (code at the pinned tree, a record, or a report, with citation), **MEASURED** (from [R07]/[BM]), **INFERENCE** (my reasoning; nothing was executed except read-only `grep`, `sed` and `objdump` on the pinned tree and the reference binary). Literature is used only as the proposals cite it from [R04]/[R05], with their status tags.
- Nothing was modified, installed, run or committed. The only file written is this one.

---

## 1. Scores

| Design | Validity | cFS fit | Openness | Prior work | Feasibility | Evaluation | **Total /60** |
|---|---|---|---|---|---|---|---|
| A — CAIRN (model-first) | 7 | 9 | 8 | 8 | **4** | 8 | **44** |
| B — Sextant (trace-first) | 7 | 8 | **6** | 8 | **7** | 8 | **44** |
| C — Kairos (hybrid) | 8 | 9 | 9 | 8 | 6 | 9 | **49** |

**Winner: C**, used as the spine. B supplies the observation engine and the trace certificates; A supplies the model levels, the extra verdict classes and the test discipline. The ties and the margins are explained in §5.

---

## 2. Spot checks that decide scores

### 2.1 Tier-1 "fixed-order" rule versus uses that may not execute (affects A; wording affects C)

- **FACT.** `CFE_ES_TaskInit` returns early on several failures before later R2 use sites. For example, a failed `CFE_EVS_Register` returns at the `return Status` after `CFE_EVS_Register` (`cfe/modules/es/fsw/src/cfe_es_task.c`, `CFE_ES_TaskInit` starting near L306). The same pattern follows `CFE_SB_CreatePipe` and each `CFE_SB_Subscribe` (relative lines 57–114 of an `awk` window from L280).
- **FACT.** `CFE_ES_TaskMain` calls `CFE_ES_WaitForSystemState(CORE_READY, …)` unconditionally, even when init failed (`cfe_es_task.c:114-134`). So SB is still created and still publishes its AppId (`cfe_es_start.c:810-846`; `cfe_sb_task.c:138`).
- **Consequence for A.** A's FIXED_ORDER condition is `s_u ∈ MustBefore(x)` for every writer `x` (§4.4). MustBefore(x) means "u occurred earlier in every execution in which x occurs". That condition is false for any use that a failure path can skip, because SB still publishes on that path. As specified, A's headline R2 certificate does not go through for most of the 24 uses.
- **The relation actually needed** is *precedes-whenever-both-occur*. The barrier store (WaitForSystemState's AppState raise) post-dominates every use in TaskInit and runs once. The publication requires that store (guarded edge plus terminal panic). So any use that executes precedes the publication.
- **C** states its condition as "`t ≺_S p` on every path on which p occurs" (§4.3). This is ambiguous in the same way and must be pinned down.
- **B's** robust-HB certificate is conditional by construction ("in every execution … in which the certified events execute on the same paths", G4). It is limited to observed paths (path-stability assumption).
- **Writer-set completeness also needs transition-aware reasoning.** **FACT:** the ES background scan writes `AppState = WAITING` (which is ≥ RUNNING), but only from RUNNING (`cfe_es_apps.c:1139-1146`). A writer set that ignores that guard would add a spurious writer and break the guarded edge. B has no static analysis to establish writer-set completeness for its OBS-FATAL edges.

### 2.2 B's worked example 1 is routed through the wrong engine

- **FACT.** The App-record publication is under the ES shared-data lock (`cfe_es_apps.c:861-869`). `CFE_ES_GetAppID` takes the same lock around `CFE_ES_GetAppRecordByContext` (`cfe_es_api.c:738-765`).
- **Consequence.** Under B's own rules (LOCK is a definite, schedule-chosen edge), the child's first `GetAppID` and the publication are **lock-ordered in the trace**, not unordered. Their critical sections conflict on the App record. So the reversal is outside SHB (unordered pairs) and outside WCP/SyncP (non-conflicting or sync-preserving). It needs B's P3 SMT engine, and **no solver is installed** (MEASURED, [R07] §2.4; re-checked: no `z3` in `dpkg -l`).
- B's §7.2 claim "tier 1: unordered → P1 schedulable" is therefore wrong. Without a PI install decision, B's flagship open-world candidate ends as `unknown(no-solver)`. The C1/C5 lock-reversal class depends on P3 in the same way.
- A (DPOR over the model) and C (`kz-predict`, explicit search with lock reversal allowed, no solver) do not have this dependency.

### 2.3 How return values are captured: an unmeasured perturbation that all three share

- **FACT (objdump of the reference `core-cpu1`).**
  - At `-O0`, the instruction after an API call is a `mov`, e.g. `call CFE_SB_CreatePipe` followed by `mov %eax,-0x138(%rbp)`.
  - Callee epilogues are `leave; ret` (checked for `CFE_EVS_SendEventWithAppID`, `OS_MutSemTake`, `CFE_ES_GetAppID`).
- **MEASURED ([R07] §4.7).** A uprobe on the `mov` at entry+5 costs 31.7 µs, the single-step class. A uprobe on the emulated `push` at entry+4 costs 0.57 µs. uretprobe cost and `ret` emulation were **not measured**.
- **INFERENCE.** A and C plan caller-side return probes "on the first instruction after the call". These are very likely on the 31.7 µs path, not the 0.57 µs path.
  - Arithmetic on measured values only: CORD logged 626–632 events per boot ([R2C] §4.1). At 31.7 µs each that is about 20 ms per boot. A 10× larger generic plan would be about 200 ms.
  - The time is concentrated in the startup burst. The order-sensitive outcomes sit there too: SBNSubPipe overflow about 47–49 ms after OPERATIONAL ([BM] §1.3; D8).
- B's entry-only lock reconstruction (`obj_seq` at `OS_MutSemGive` entry, inside the critical section) avoids return probes on the hottest sites. That is a real methodological advantage. All three still need return values at contract-use sites.

### 2.4 Other checks (all consistent with the proposals)

- **Core serialization.** `CFE_ES_MainTaskSyncDelay(RUNNING, CORE_MAX_STARTUP_MSEC)` is followed by `CFE_PSP_Panic` on failure (`cfe_es_start.c:833-846`). On pc-linux, `CFE_PSP_Panic` calls `abort()` (`psp/fsw/pc-linux/src/cfe_psp_support.c`). So the T-TERM / guarded-S / OBS-FATAL rule is justified.
- **Child-side identity anchors.**
  - `OS_TaskEntryPoint(osal_id_t)` is exported (`osapi-task.c:124`), the anchor B uses.
  - `OS_PthreadTaskEntry` is `static` (`os-impl-tasks.c:131`), the anchor C uses. It can be probed from `.symtab` because the binary is not stripped ([R07] §4.1).
  - `OS_TimeBase_CallbackThread(osal_id_t)` is at `osapi-timebase.c:365`.
- **eBPF userland.** The UAPI `/usr/include/linux/bpf.h` is present and defines ring-buffer and task-storage map types. `/usr/include/bpf` (libbpf) is absent. This matches C's "re-read" claim and the libbpf decision that all three request.
- **Cold first run ([BM] §0).** The first run after a VM reboot had OPERATIONAL at **2570 ms** against 1613–1633 ms, because app loading took 849 ms instead of 2–3 ms. None of the three perturbation protocols controls for this cold/warm state. See must-fix M6.

---

## 3. Per-design assessment

### 3.1 A — CAIRN (model-first): 44

**Strengths.**

1. The best formal frame. A verdict is quantified over (B, σ, **M**, b). The model levels **M_api** and **M_impl** produce two labels no other proposal has: "portability hazard" (protected only under M_impl) and "implementation defect candidate" (violable only under M_impl). This answers [R01] §7 item 6 directly.
2. Extra classes and shapes:
   - `NO_PROVIDER` (HS `linux_sysmon`);
   - `LOSS-UNDER-ORDER`, which keeps route and capacity losses apart from violations when no must-deliver reference exists;
   - the shapes WRONG-WAY, UNORDERED, ABSENT and STALE.
3. Readiness is defined by the framework's own validity predicates (`CFE_ES_AppRecordIsMatch`, `EVS_AppDataIsMatch`). This lowers author bias, and RESERVED windows appear automatically.
4. The K-CAP tier-1 upper bound by maximum-weight closure gives a sound PROTECTED for capacity. A honestly flags the citation as unverified.
5. The deadlock property is a generic oracle for OSAL F1.
6. Engineering discipline is the best of the three:
   - Merkle-style parent hashes in every artifact;
   - the lint `CAIRN_REF(value, "path:line")` rejects bare constants ([POL] made executable);
   - a lint against case identifiers in `lib/`;
   - an independent certificate checker with mutation tests;
   - a **litmus suite** of small gcc-built programs on the real OSAL, with expected classes per σ and per M;
   - perturbation group **W**: the variant layout with gates disabled.

**Weaknesses (methods/feasibility lens).**

1. **Critical path through a from-scratch model checker for C.** The design needs all of:
   - slicing imported LLVM IR in MLIR;
   - lowering to a new `cfsk` dialect with havoc;
   - an interpreter that executes framework predicates (record lookups and struct-field reads) over sliced memory;
   - M_impl, which analyses the OSAL code on top of pthread/semaphore/mqueue semantics;
   - optimal DPOR with wakeup trees;
   - trace-inclusion conformance checking.
   No existing stateless model checker fits this LLVM ([R05] §1 F11: GenMC needs LLVM 19–22, Nidhugg 8–19). Tier 2, conformance and confirmation planning all hang on this component. The proposal itself rates skeleton infidelity as medium likelihood, high impact.
2. **The tier-1 rule is wrong for conditionally executed uses** (§2.1). The R2 certificate at M3 fails as written.
3. **State explosion.** The ES and SB locks are touched by almost every task, which A acknowledges. Lemma L2 (drop critical sections irrelevant to the cone) is the right reduction, but it is still an unproved assumption (A16).
4. **Conformance as a search per boot** (§4.8) is NP-hard in general. If its budget is exhausted, every dependent verdict becomes UNKNOWN(conformance). Conformance can therefore become the bottleneck for all results.
5. **The full recorder arrives only at M5.** This follows v0.1 §15, so it is defensible. But it means no empirical cost or perturbation data exists until the model exists.
6. **MLIR ODS (TableGen) generation** stays on the table until gate G-MLIR, in tension with K8. A flags this itself (OPEN-2).

**Fatal flaws (as specified).** The feasibility of the C-level model on the critical path, and the headline certificate rule (§2.1). The rule is fixable. The feasibility problem is fixable only by moving the model off the critical path.

### 3.2 B — Sextant (trace-first): 44

**Strengths.**

1. **The most implementable design.** Every runtime mechanism it uses is MEASURED in [R07]: uprobes at entry+4, raw `bpf()`, tracefs mountable in a private namespace, debug registers and ptrace. The static work is limited to probe placement and summaries (LibTooling at 10 s for 448 TUs; DWARF and MC read from the **gcc** binary).
2. **Robust HB with two vector clocks.** VC runs over all definite edges and VC_R over unconditional edges only. Together with the tier-1 table over ordered and unordered pairs, this is the cleanest way to decide ordered-the-wrong-way violations like R2 from a trace. The certificate is correctly conditional (G4).
3. **Observation craft:**
   - the probe-site rule at entry+4, verified on nine prologues;
   - probe cost classes assigned by decoding instructions with LLVM MC;
   - **entry-only lock-order reconstruction**;
   - per-thread `tseq` assigned in the kernel, plus loss accounting;
   - a **zero-install tracefs fallback** with an explicit A-CLK assumption (tsc with constant and nonstop flags verified);
   - **debug-register execution breakpoints** as gates, so the text is never patched;
   - hold-until-event with watchdogs taken from reference constants;
   - a gate-placement rule that forbids holding inside the OSAL RESERVED window except as a declared experiment;
   - a `timeout-influenced` flag on confirmation runs.
4. **Template-free discovery.** Outcome mining (not-ready codes from `api.cat`) and key-pair enumeration need no authored template.
5. **Prediction extrapolates retries** up to their reference bounds (CF 25, `GetTaskFunction` 21, OSAL 4 waits), and windows are defined by HB, not by constants.
6. **Baselines as reimplementations over the same traces:** FastTrack, WCP, SHB, DCatch-style and a RacePro-style syscall model. This is the fairest comparison design of the three.
7. **Early ground truth.** Milestone M1 re-observes R2 with the new recorder. The minimal-plan perturbation group C′ isolates the cost of the R2 probes alone.

**Weaknesses.**

1. **Lock-reversal and capacity prediction need P3 (SMT), which is not installed** (§2.2). The worked example also routes the case wrongly.
2. **Openness is bounded by what executes.**
   - There is no static census, so latent sites are only `unobserved` against a denominator from `site.cen`.
   - `inactive(B)` needs a configuration summary that the thin static layer barely provides.
   - Certificates exist only on observed paths.
3. **OBS-FATAL robustness needs a complete writer set** (§2.1), and B has no analysis that establishes it.
4. **Return-probe cost and controller cost are unmeasured.** B acknowledges both; they are M1 and M6 exit items.

**Fatal flaws (as a standalone design).** Without a solver, the main open-world classes (lock reversal, capacity) cannot be predicted. Without static analysis, the tool cannot speak about code that has not executed. Both are fixable by grafting C's explicit-state predictor and C's static census.

### 3.3 C — Kairos (hybrid): 49

**Strengths.**

1. **Strata.** S (structural) and O (observed sync) are HB strata; C_σ (schedule constraint) is never HB. Together with **σ_U∞** (patient semantics, no timeouts) and a containment lemma, this separates "timing-dependent" from "logic-dependent" cleanly.
2. **Six consistency obligations (O1–O6).** Each layer is a test oracle for the others, and a contradiction suspends the rule involved instead of being averaged away. This is the strongest validity argument of the three.
3. **The profile Φ is data, interpreted, never code-generated.**
   - `libkz-core` has no LLVM dependency.
   - One C schema header, with `_Static_assert` checks, is shared by the BPF program and C++.
   - `kz-proofcheck` re-validates proof-carrying verdicts.
4. **`kz-predict` is an explicit search with sleep-set reduction.** It allows lock reversals and needs **no solver**.
5. **Evidence is an axis separate from class.** Confirmations that need a constructed schedule are labelled "derived".
6. **Contracts come from documented sources:**
   - lifecycle symmetry (create/delete operations automatically get K-REG, K-LIFE, K-GEN);
   - retry loops become `Allowed` automatically;
   - K-LOCKBAL comes from OSAL's own documented invariant (`osapi-idmap.c:384-385`) and uses a wait-for oracle (`wchan`/futex), not a timeout.
7. **The evaluation measures each layer:**
   - layer ablations and a **feedback-loop ablation**;
   - obligation failures counted per iteration;
   - property tests for HB transitivity, the containment lemma and loop monotonicity;
   - perturbation groups A–E;
   - two-rater triage with an agreement statistic.

**Weaknesses.**

1. **The largest scope:** eleven components, three interpreters, a feedback loop and a proof checker. The integration risk is real. The milestone order (M3 trace HB, M4 census) mitigates it but does not remove it.
2. **The explorer explodes as specified.** It keeps the sync skeletons of all actors unsliced, and dependence includes "same sync object". Every ES-lock and SB-lock critical section therefore becomes mutually dependent.
   - INFERENCE: about 20 tasks run during startup, each with several SB-lock sections (subscribe, create pipe). The number of Mazurkiewicz traces is far beyond any DPOR budget.
   - The fix: key-level dependence (the lock as atomicity) and A's L2.
3. **The fixed point depends on which runs were sampled.** Observation runs are nondeterministic, so the reached fixed point depends on the run set. The greedy set-cover probe plan also changes between iterations. σ_E statistics would then mix instrumentation plans unless they are stratified by plan hash.
4. **The tier-1 relation is ambiguous** for uses that may not execute (§2.1).
5. **Caller-side return probes** are likely in the expensive cost class (§2.3).
6. **Routing gap.** §4.5 sends only pairs with concurrent lifecycle events (`L∥ ≠ ∅`) explicitly to prediction. Pairs ordered only by O edges (C1/C5) need an explicit routing rule. §1.8 and §4.6 imply one, but §4.5 does not state it.

**Fatal flaws.** None strictly fatal. Weaknesses 2 and 3 would sink the evaluation if left unfixed.

---

## 4. Cross-cutting methodological findings

1. **Each of the simpler designs is the ablation that the hybrid's paper needs.** "A-only" (no trace layer) and "B-only" (no static layer) should be run as ablations on the same engine. They are what justifies C's complexity. Report the decided share, the unknown share by reason, false candidates removed by O1–O6, and cost.
2. **Perturbation protocol (combined).** Use these groups:
   - **A:** no observer, re-measured on `-fc-v114`;
   - **B:** traps with empty programs;
   - **C′:** minimal R2 plan;
   - **C:** full plan;
   - **D:** holds;
   - **W:** variant layout, only if the F1 build is approved.

   Compare distributions, not means, of:
   - OPERATIONAL time;
   - `PipeOverflowErrorCounter` (4–49 per boot);
   - `NoSubscribersCounter` (15–26);
   - which pipes overflowed;
   - app init-completion order;
   - the R2 use set (24 per boot: ES 20, EVS 4).

   Record the cold/warm state ([BM] A1 at 2570 ms). Run the groups interleaved, not in blocks. The order is an `unspecified_by_reference` assumption, recorded with its reason: the host is shared and drifts ([R07] §1). Pre-register N with the PI ([POL]; the ~30-trial norm is status P in [R05] §6.5).
3. **Reproducibility.** Every verdict must carry the hashes of B, the profile/model, the contract library, the plan and the tool. Persistent state S0 is snapshotted per run. σ_E frequencies are pooled only over identical (B, plan hash) runs. The analysis must be replayable deterministically from stored traces.
4. **Zero-install path for every core capability.** Use the tracefs backend or a raw `bpf()` loader, and explicit search instead of SMT. libbpf, Z3, the F1 `--wrap` variant and native `SCHED_FIFO` PCT remain recorded PI decisions ([R07] §9).

---

## 5. Winner and rationale

C wins because it is the only design that meets all four of the user's top-level requirements together:

- **open:** a static census over all sites, plus template-free trace oracles;
- **valid:** strata, σ_U/σ_U∞/σ_E/σ_R1, cross-layer obligations, proof checking;
- **consistent with established technique:** framework HB à la CAFA/DCatch, maximal-causal prediction, DPOR, confirmation à la CTrigger/RacePro;
- **built around cFS:** the profile Φ with R1–R19 and the P-* rules.

It also keeps no hard dependency on an uninstalled solver or on MLIR.

A and B tie at 44 with opposite profiles. A is the most rigorous model and the hardest to build. B is the most buildable and the least open. C's feasibility score (6) is below B's (7) because of its scope and its explorer. The synthesis must therefore adopt B's observation engine and B's early milestones, and demote the explorer to a gated late stage.

---

## 6. What the synthesis must fix (prioritized)

- **M1. Certificate semantics.**
  - Define fixed-order and protected over *precedes-whenever-both-occur*: a post-dominating barrier that runs once, plus path conditions. Never use membership of the use in MustBefore(writer).
  - Test the rule on the early-return paths of `CFE_ES_TaskInit` (§2.1).
  - Writer sets must be **transition-aware** (below→at-or-above the threshold). Example: the background scan writes WAITING only from RUNNING (`cfe_es_apps.c:1139-1146`). Writer-set completeness comes from static analysis; observation cannot establish it.
- **M2. Lock-reversal prediction without installs.**
  - The explicit-state predictor (C) is primary. SMT is an optional cross-check after the PI decides on an install.
  - Correct B's routing: `cfe_es_apps.c:861-869` and `cfe_es_api.c:738-765` both take the ES lock, so the pair is lock-ordered with conflicting critical sections.
  - Route pairs ordered only by O edges to prediction explicitly.
- **M3. State explosion.**
  - Explorer dependence is at resource-key level (the lock as atomicity), with A's L2 as a switchable assumption, cones, and trace-anchored windows (B).
  - Budgets and preemption bounds are swept (BPOR when bounded). The decided share is reported per budget.
  - The explorer is a gated late stage with a stated kill criterion. If it cannot decide development-set cones within the swept budgets, verdicts fall back to prediction and confirmation.
- **M4. Return-value capture.**
  - Measure the uretprobe cost and the cost class of `ret` and of the post-call `mov` **before** committing to a plan (§2.3).
  - Use B's entry-only reconstruction for locks and queues. Restrict return probes to contract-use sites. Plan with the per-thread ΔC_i model and record a cost class per probe.
- **M5. Staged build with exit gates.** Each stage is publishable on its own, and the recorder stays narrow and model-driven, respecting v0.1 §15.
  1. Formats and the profile DSL.
  2. F0 observer:
     - R2 re-observed: 24 per boot, ES 20, EVS 4;
     - return-probe cost measured;
     - `.so` pre-registration tested;
     - group A re-measured.
  3. Trace HB, VC/VC_R certificates and dynamic contract checking.
  4. Static closure, census and structural proofs, with the `kz-proofcheck` and Clang-to-gcc anchor audit.
  5. Trace prediction.
  6. F0 confirmation.
  7. Explorer.
  8. MLIR gate. No ODS/TableGen unless the gate passes on stated criteria (K8).
- **M6. Perturbation and statistics protocol** as in §4.2:
  - groups A, B, C′, C, D, W;
  - distributions of the order-sensitive outcomes;
  - cold/warm state recorded;
  - interleaved order (assumption recorded);
  - Mann–Whitney / Fisher tests with effect sizes and binomial confidence intervals;
  - N pre-registered by the PI.
- **M7. Feedback-loop reproducibility.**
  - Record run IDs, B hash and plan hash per iteration. State the fixed point together with its run set.
  - Never pool σ_E statistics across plans.
  - The stopping rule is a fixed point of the evidence. Any run budget per iteration is `unspecified_by_reference` and is swept.
- **M8. Ablations that justify the hybrid.**
  - No-trace layer (≈ A) and no-static layer (≈ B).
  - No feedback loop; no obligations.
  - Collapsed identities; atomic APIs; conditional edges made unconditional.
  - Implementation-level OSAL locks treated as synchronization.
- **M9. Cheap conformance.** O1/O2 check per-thread acceptance by the skeleton automata. Do not use A's full trace-inclusion search on every boot. Disagreements produce `unknown(model_gap)`.
- **M10. Confirmation discipline.**
  - Release on an event. Watchdogs come from reference timeouts.
  - Gates are placed where the held thread's lockset is empty, according to the observed lock events.
  - Use debug-register gates (B).
  - Measure the controller cost and the interaction between ptrace and uprobes.
  - Labels: `derived`, `timeout-influenced`, `confirmed-controlled(σ_U)`.
  - Holds inside an OSAL RESERVED window happen only as declared experiments.
- **M11. Contract governance.**
  - Freeze the library by hash twice: F1 before the case runs, F2 before the held-out runs. Run template-only before any refinement.
  - Measure authoring effort.
  - Enforce `CAIRN_REF`-style reference constants and the case-identifier lint.
  - Import A's NO_PROVIDER class, LOSS-UNDER-ORDER label, M_api/M_impl levels and litmus suite.
- **M12. Baselines.** Reimplementations over the **same traces** (B's list), always labelled as reimplementations. Thread data-race tools appear only as reference points (S13). `NOT_RUN` is never counted as a false negative.

---

## 7. Best ideas to graft, by design

- **From A:**
  - the M_api/M_impl levels (portability hazard versus implementation defect);
  - NO_PROVIDER; LOSS-UNDER-ORDER; the four shapes;
  - readiness defined by the framework's validity predicates;
  - the K-CAP max-closure bound;
  - the deadlock oracle;
  - the litmus suite;
  - perturbation group W;
  - Merkle parent hashes;
  - the `CAIRN_REF` and case-identifier lints;
  - an independent certificate checker with mutation tests;
  - Lemmas L1/L2 as switchable assumptions with their own ablation.
- **From B:**
  - VC/VC_R robust certificates with explicit path stability;
  - the tier-1 VC-relation table;
  - entry-only lock reconstruction (`obj_seq` at give entry);
  - probe cost classes decoded with LLVM MC from the gcc binary;
  - the tracefs zero-install fallback with A-CLK;
  - debug-register gates and the gate-placement rule;
  - outcome mining and key-pair enumeration;
  - retry extrapolation to reference bounds;
  - the `timeout-dependent` and `tolerated` classes;
  - baselines over the same traces;
  - perturbation group C′;
  - R2 re-observation as the first gate;
  - canonical text dumps for golden tests.
- **From C:**
  - the S/O/C_σ strata;
  - σ_U∞ and the containment lemma;
  - obligations O1–O6;
  - the interpreted profile DSL with an LLVM-free core;
  - one schema header shared with BPF;
  - proof-carrying verdicts with `kz-proofcheck`;
  - the solver-free explicit predictor;
  - evidence as an axis separate from class;
  - lifecycle-symmetry and retry-derived contracts;
  - K-LOCKBAL with a wait-for oracle;
  - the `.so` fallback (hold at the return of `OS_ModuleLoad`);
  - layer and loop ablations; property tests.

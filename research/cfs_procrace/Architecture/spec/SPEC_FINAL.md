# SPEC FINAL — Kairos: a detector for process-level readiness and order violations in cFS

- Date: 2026-10-12. Role: lead architect of panel AR2. Status: **final architecture specification**. Nothing was built, installed, run against cFS, modified or committed for this document. Files written: this one, `ISSUE_RESOLUTION.md` and `PI_DECISIONS.md` in the same directory.
- This document revises `SPEC_v1.md` after three adversarial reviews: [X1] formal soundness (`review_X1-soundness.md`), [X2] cFS fidelity (`review_X2-cfs-fidelity.md`), [X3] feasibility and evaluation (`review_X3-feasibility-eval.md`). Every critical and major issue is fixed in place; the minor ones are fixed as well. The disposition of each issue, and where its fix lives, is in `ISSUE_RESOLUTION.md`. Decisions that only the PI can take are in `PI_DECISIONS.md`.
- Name: **Kairos** (tool prefix `kz`). The name is a label only; no name-collision search was made, and naming is a PI decision (`PI_DECISIONS.md`, P-NAME).
- Inputs and how they are cited:
  - AR1 reports [R01]–[R07] in `scratchpad/arch/`, cited with their own section and `path:line` citations.
  - Proposals [A] (`proposal_A-static-model.md`, CAIRN), [B] (`proposal_B-dynamic-predictive.md`, Sextant), [C] (`proposal_C-hybrid-layered.md`, Kairos). Judgments [J1] (formal), [J2] (cFS), [J3] (methods). Reviews [X1], [X2], [X3].
  - Research records: `CONDITIONS_POLICY.md` [POL], `cases/ledger.md` [LED], `results/R2_gdb_v701.md` [R2g], `results/R2_cord_v701.md` [R2c], `env/BASELINE_MEASURE.md` [BM]; the user's v0.1 design [v0.1].
  - The pinned tree `/home/user/work/procrace/cfs_ref/cFS` (cFS `088b2fa8`; cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`). Code paths are relative to it. Lines re-read for this document are marked "(re-read)".
- Labels: **[F]** fact with citation (code, record or report). **[M]** measured, in [R07], a record, a review ("[X3, here]") or for this document ("measured here"). **[I]** inference or design reasoning; not executed. **[D]** design decision (an ADR in §11 records it). **[U]** unverified. Literature is cited only as [R04]/[R05]/[R06] verified it, with their status tags (V-doc, V-abs, V-me, P, UNVERIFIED). Where a concept is used without a verified citation, the text says "citation: open literature check".
- Policy [POL] (line numbers of `CONDITIONS_POLICY.md`): every bound, budget and parameter is one of
  - `from-reference` (POL L15): a code constant or pinned document, cited;
  - `deviation` (POL L16, L18–21): differs from or is absent from every reference, recorded with reason, effect and necessity. **Every value that a person chooses because resources are finite** (numbers of runs, work-unit caps, sample sizes) is a deviation, never `unspecified_by_reference`;
  - `unspecified_by_reference` (POL L17): no reference fixes it and **no value is chosen**; if the analysis needs a premise, it is recorded as an assumption.
  - Tool parameters follow POL L22: event-based control, or constants that a reference contains. Tool constants appear in code only as `KZ_REF(value, "path:line")` (§9.2). Values used in Kairos's own unit tests are not research conditions (POL L23).

---

## 0. Summary

### 0.1 What Kairos decides

For every **readiness-contract instance** in a cFS build (a use of a framework resource that requires that resource to be in some lifecycle state), Kairos produces a verdict record. Its axes are orthogonal (§1.10):

- **per semantics σ and OSAL model level M** (a cell for each (σ, M) that was analysed): `class`, refinements, and the bound `b` under which the class holds;
- `provider ∈ {present, none, env-absent, unknown}` and `reach ∈ {proved(run | model path), assumed}`;
- evidence on two axes: **realization** (`certificate`, `model(b)`, `predicted`, `predicted-modelled`, `predicted-extrapolated`, `realized`) and **naturalness** of realized runs (`natural`, `held-F0`, `gated-F1`, `timeout-exercised`; flag `derived`), with `n of N` and the pool key;
- shape, outcome label, consequence, harm, chain links, scope (`in-scope` or `teardown`), B hash, Φ hash, tool hash.

`class ∈ {inactive, protected (refinement: tolerated), fixed-order, schedule-dependent (refinements: timeout-dependent, timeout-masked-liveness), unknown(reason)}`; a good-side flag `timeout-rescued` marks instances whose good executions all need a timeout. `σ ∈ {σ_U, σ_U∞, σ_R1[P_sched], σ_E}`; `M ∈ {M_api, M_impl}`.

**No class means "safe".** The absence of an alarm is the absence of evidence. It maps to `unknown(reason)`. No σ_E class is ever derived from the absence of bad runs.

### 0.2 The formal core on one page

1. **One typed event model (KEM) and one framework profile Φ_cFS.** Φ is written once in a small declarative DSL with a **typed core expression language over resource state** (§2.3). It declares resource kinds with lifecycle automata, operations with effects, a **linearization point (LP) per conjunct of every requirement** (with **capture points** for values that a caller loads earlier), HB rules with stratum and robustness tag, and contracts with a required state, a framework-predicate oracle, allowed not-ready outcomes with a reference horizon, harm, provenance and an implicit **scope** ("before the shutdown request").
2. **Φ is interpreted three ways over the same event type**, through one evaluator in `libkz-core`:
   - **abstractly**, over configuration-closed per-actor skeletons in two views: a **certificate view** in which every unbounded loop is star-closed, and an **exploration view** cut at a horizon h. Census, MustBefore, PWB and certificates use the certificate view only;
   - **concretely**, over traces of the unmodified gcc binary: HB with vector clocks (handler-scoped program order on loopers), contract checking at LPs, lock-reversing prediction;
   - **operationally**, in a bounded per-candidate explorer (gated, late).
3. **Static ordering is a must-analysis.** MustBefore is a greatest fixpoint with meets over predecessors (T-PO) and over all spawn sites (T-SPAWN), with ∅ at every root. Observation edges (T-OBS) need a complete, transition-aware writer set, an **await predicate extracted from the code** and a falsifier. Writers that cannot lie between the falsifier and the observation are removed by checkable rules: own-actor no-path, PWB before the falsifier or after the observation (by the frontier or the once rule), and key separation. Branches that can only end in a terminal event are cut (T-TERM). Locks alone give no order.
   - Fixed-order and protected are certified over **precedes-whenever-both-occur** (PWB). PWB needs **once(τ)**: the actor runs at most once per execution, proved by an at-most-once analysis.
   - Every certificate is re-validated by an **independent checker** that shares no code with the producer, with certificate-mutation tests; the analysis itself is tested by **program mutants** (§8.5).
4. **Writer sets** come from a field-based, memset- and out-pointer-aware writer extraction at the Clang AST level under an explicit typed-access assumption A-TYPE (stage S3a), with Φ-declared effects supplying keys and guards. Each writer entry carries a transition status and a **key status** (`MUST_KEY`, `MAY_KEY`, `NO_KEY`). Agreement between the two oracles is obligation O7. Andersen-style points-to over IR is a later option (S3b), adopted only if it removes measured blockers.
5. **Semantics.** σ_U (untimed; retries exhaust only after their reference bound), σ_U∞ (no timeouts; delay-paced retries are patient; strongly fair; outcomes good / bad / stall), σ_R1[P_sched] (fixed priority on one CPU, parameterized by a cited scheduling profile), σ_E (the real binary). **Containment is stated per model level**: `Exec(B,σ_E) ⊆ Exec(B,σ_U,M_impl)`. M_api verdicts are claims relative to the OSAL API contract (portability claims) and transfer to σ_E only when the cone contains no M_impl-relevant operation, or when the M_impl verdict agrees. Universal verdicts transfer down (same M, same b), witnesses transfer up, nothing becomes a σ_E frequency.
6. **Strata.** S (structural, must-order), O (observed synchronization, reorderable in prediction), C_σ (schedule constraints of σ_R1, never HB). Between-handler program order on a looper is **not** an HB edge; it survives only as a FIFO-derived constraint in prediction (CAFA, DCatch).
7. **Layers check each other.** Obligations O1–O9 plus the certificate checker. A contradiction suspends the rule or model involved, sets `unknown(model_fault)` with a model-fault record, ends the feedback loop for that Φ version, and is never resolved silently. A contradiction of an M_api-only certificate by the real binary is portability-hazard evidence, not a model fault.
8. **Evaluation measures misses.** A pre-registered recall protocol on history-derived program mutants and pre-fix/post-fix pairs, frozen before F2, reports detected / unknown / missed per layer with exact binomial intervals; a certificate contradicted by a mutant's documented behaviour is reported as a soundness failure.

### 0.3 What was taken from where

| From | Grafted element | Where |
|---|---|---|
| C (spine) | KEM + Φ interpreted three ways; S/O/C_σ strata; σ_U∞ and containment; obligations; feedback loop; on-behalf frames; K-LOCKBAL, K-LOCK.cancel, retry- and symmetry-derived templates; generic data-driven BPF program; solver-free predictor; loop ablation; H-V/H-C/H-F | §1, §2, §3, §4.8–4.9, §4.14, §7 |
| A | MustBefore fixpoint (T-PO, T-SPAWN, T-OBS, T-TERM); first-occurrence argument; independent certificate checker with mutation tests; M_api/M_impl with portability-hazard and implementation-defect labels; framework validity predicates as oracle; no-provider (now a provider axis); LOSS-UNDER-ORDER; shapes; K-CAP max-weight-closure bound; conformance; litmus suite; `CAIRN_REF`-style lints; perturbation group W; Merkle parent hashes; table cross-check; G-MLIR gate | §1.4, §1.10, §4.2–4.5, §4.10–4.11, §8, §9 |
| B | VC/VC_R with robustness tags; `timeout-dependent` and `tolerated`; outcome mining and key-pair enumeration; entry-only per-object lock tokens; entry+4 placement with MC-decoded cost classes; per-thread tokens and loss; hold-until-event; summary-guided extrapolation (labelled model-based); baselines over the same traces; group C′; R2 re-observation as first gate; canonical text dumps | §1.9, §4.6–4.9, §4.13, §5, §8 |
| Judges | PWB certificates; transition-aware writer sets; LPs; Allowed horizons; DPOR correctness statement; key-level dependence; explorer kill criterion; staged build; perturbation protocol with cold/warm state; loop reproducibility; corrected facts | throughout; §0.4 |
| X1 | two skeleton views; per-M containment; per-conjunct LPs and capture points; once(τ) and once-PWB; MUST_KEY and allocation records; A-COMM and `predicted-modelled`; σ_U∞ with patient retries and stall outcomes; shutdown scope; message provenance; code-extracted await predicates; G5 conditional on a named theorem plus a verified exhaustive mode; L3 as a mover reduction; two evidence axes; corrected prior-work attributions and three same-family baselines | §1.4–1.10, §4.2–4.12, §6, §8.3 |
| X2 | own-actor no-path writer exclusion (R2 certificate repair); K-SELF.es at seven call sites with branch-complete consequence chains; OSAL timed-wait attempts and by-ID/by-name split; faithful restart/shutdown model; SB-actor K-MODE; two-phase SB transmit and MsgLim capacity; σ_R1 scheduling profiles; basis additions (reset type, script selection, euid, CWD, CDS images); header-carried MsgIds; loader symbol scope; level-indexed K-PHASE | §1.3, §1.7, §3, App. A |
| X3 | recall protocol; attach-cookie BPF keying with inode registration; perturbation ratio and split measures; normalization boots; S3a AST writer extraction; re-scoped stages with a thin MVC and estimates; comparison metric and frozen comparators; sampled blinded triage; labelled ablations; POL relabelling; work-unit budgets; Φ core expression and pattern languages; joint debug-register allocator; perf-sample bring-up backend | §2, §4, §5, §8, §9 |

### 0.4 Disposition of the judges' must-fix items (as revised by the reviews)

"J1-n" is item n of [J1 §7]; "J2-n" of [J2 §5]; "J3-Mn" of [J3 §6]. Where a review changed the v1 fix, the last column says so.

| Item | Problem | How it is fixed now | Where |
|---|---|---|---|
| J1-1, J2-2 | C's static order was a may-analysis | MustBefore greatest fixpoint on the **certificate view** (star-closed loops), ∅ at every root, T-SPAWN as a meet over all spawn sites, T-OBS with complete writer sets, code-extracted predicates and a recomputed W_o, no lock rule; independent checker (revised by X1-C1, X1-m2, X1-M8) | §4.3, §4.4, §6.2 L-MB |
| J2-1, J3-M1 | Dominance-based FIXED_ORDER fails on R2's early returns | Frontier certificate over PWB with **once(τ)**; R2 regression and the startup thread's own failure-path record clear (`cfe_es_start.c:823`) handled by the own-actor no-path rule; S3a exit test with mutations (revised by X1-M2, X2-01) | §4.4, §4.5, App. A.1 |
| J1-2, J2-6, J3-M1 | Writer sets and mod-ref | Field-based writer extraction at the AST level under A-TYPE (S3a), out-pointer summaries, transition-aware filter, key status, allocation records; Andersen (S3b) only if blockers are measured (revised by X3-M4, X1-M3) | §4.2 |
| J1-3 | Semantics, fairness, stall, containment, transfer | σ_U with exact retry bounds; σ_U∞ with patient retries, strong fairness and good/bad/stall outcomes; per-M containment; transfer qualified by M and b (revised by X1-M5, X1-C2, X1-m3, X1-m6) | §1.8, §6.2 |
| J1-4, J2-5 | LPs; M_api vs M_impl | An LP **per conjunct**, with capture points for caller-loaded values (`derives` facts); M_impl with timed-wait attempts and by-ID/by-name split (revised by X1-M1, X2-03) | §1.4, §1.6, §3.2 |
| J1-5, J2-12 | Allowed horizons; route/capacity harm | Finite Allowed vocabulary with reference horizons; `documented_drop` for the SB recursion guard; LOSS-UNDER-ORDER with SB counters (two-phase transmit, MsgLim) | §1.6, §3.9 |
| J1-6, J2-13 | DPOR correctness | Two explorer engines: `exh` (stateful exhaustive search; correctness by finite reachability) and `dpor` (source-DPOR + sleep sets; G5 conditional on the named JACM theorem, status P, hence labelled unverified); differential test is validation only; no "one execution per class" claim (revised by X1-M9) | §4.10, §6.2 L-DPOR |
| J1-7, J2-3, J3-M2 | Lock reversal without a solver | Explicit-state lock-reversing search; constraint "an acquire needs the lock free"; A-COMM check, else `predicted-modelled` (revised by X1-M4) | §4.9 |
| J3-M2 | Routing of the App-record windows | O-ordered (lock edge) → prediction | §4.8, App. A.3 |
| J1-8, J3-M9 | Model–binary link | O1/O2 on every trace; O8 deferred to S5's engine (revised by X3-M5) | §2.5, §4.11 |
| J1-9, J2-7 | Verdict record; latent instances | Per-(σ, M) cells, provider and reach axes, two evidence axes, typed references (revised by X1-m3, X1-M11, X3-m6) | §1.10, §2.3 |
| J1-10 | Facts and examples | Corrected; see §0.5 | §0.5, App. A |
| J2-8 | HS `linux_sysmon` | provider axis `env-absent` (was class `capability-absent`) | §1.10, App. A.6 |
| J2-9 | Version-robust site binding | AST-pattern language with a match semantics; DWARF compile-unit binding of same-named local copies (revised by X3-M11, X3-m3) | §2.3, §3.17 |
| J2-10, J3-M10 | Confirmation safety | Gate placement rules; joint debug-register allocator; attach strategy measured; normalization boot after abnormal exits (revised by X3-m1, X3-M3) | §4.13 |
| J2-11, J1-15, J3-M4 | Observation obligations | S1 measures return capture (incl. R-e), BPF body, attach cookies; perf-sample bring-up backend (revised by X3-M1, X3-m2, X3-m3) | §5 |
| J2-4 | No HB from timestamps | Unchanged | §5.8 |
| J2-14, J1-11 | MLIR gate; staged MVC | G-MLIR unchanged; **thin MVC = S0 + S1 + S2 + S3a** (revised by X3-M5) | §2.6, §9.3 |
| J2-15, J3-M5 | Scope order | S0 → S1 → S2 → S3a → S4 (O1–O7) → S5 (+O8) → S6 → S7 → S8; S3b conditional | §9.3 |
| J1-12, J2-16, J3-M11 | Anti-circularity | Two oracles; provenance; freezes; refinement accounting that names framework rules such as R19 (revised by X1-M8) | §7.2 |
| J1-13 | Openness | Union of discovery sources | §7.1 |
| J1-14, J3-M8, J3-M12 | Evaluation | Recall protocol on program mutants; labelled ablations; common comparison metric; same-family baselines (revised by X3-C1, X3-M6, X3-M8, X1-M12) | §8 |
| J3-M3 | Explorer state explosion | Key-level dependence; L3 as a mover reduction; work-unit budgets; kill criterion (revised by X1-M10, X3-M10) | §4.10 |
| J3-M6 | Perturbation and statistics | Uninstrumented-observable vs recorder-only measures; per-witness perturbation ratio; `σ_E(plan P)` labels; observer CPU recorded; N as a deviation (revised by X3-M2, X3-M9) | §8.6, §8.7 |
| J3-M7 | Loop reproducibility | Fixed-point test excludes counts and pool keys; work units, never wall clock (revised by X1-m8, X3-M10) | §4.14 |
| J2-17 | Identity and persistence | Generations (B_restart) as well as run sequences; canonical content-only Init hash (revised by X2-12, X3-M3) | §1.7, §3.12 |
| J3-M12, J1-14 | NOT_RUN never a false negative | Unchanged | §8.3, §6.4 |

### 0.5 Errata to the inputs (recorded here; the inputs are read-only)

| Input | Statement | Correction | Evidence |
|---|---|---|---|
| [B §7.2] | Script-app App-record window is "unordered → P1 (SHB)" | Publication (`cfe_es_apps.c:861–869`) and `CFE_ES_GetAppID` (`cfe_es_api.c:738–765`) both hold the ES lock: O-ordered; needs lock reversal | [J1 §1 check 3]; [J3 §2.2] |
| [B §2.5] | "452 `\retval`" | 429 `\retval` and 138 `\par Assumptions` in `cfe/modules/core_api/fsw/inc` | [J1 §1 check 7] |
| [C §5.9] | "211–258 SB-AppId uses per boot" | 240–258 total, 211–229 ordered (CORD r2_c4–c6); 199/205/205 ordered under gdb | [R2c §7]; [R2g §2] |
| [B], [C] | CF retry bound "25" | the effective `CF_STARTUP_SEM_MAX_RETRIES`, constant-evaluated in the build's TU | [J1 §1 check 6] |
| [A], [R03 §3 row 20] | HS `linux_sysmon` is NO_PROVIDER | provider present; platform capability absent → provider axis `env-absent` | [J2 §1 check 13] |
| [A §A.1 step 6], [R06 §5.5] | Post-publication SB/TBL identity uses expected PROTECTED | not provable: the core-task App-record window can leave the identity UNDEFINED; expected `schedule-dependent(σ_U)`, shape ABSENT, chained. INFERENCE until the census runs | [J1 §1.1]; [LED §3.4] |
| [C §3.6] | K-PHASE provenance from enum comments | `cfe_es.h:400–403` | [J2 §1 check 7] |
| [B §4.4 P2]; SPEC v1 §0.5 row 8 | "WCP decides"; v1's rewording "only SyncP is sound and complete for its class" | WCP is sound and linear-time. SyncP is sound and complete for sync-preserving races; SHB "identifies exactly the schedulable races"; M2 detects all races for two-thread traces. None reverses conflicting critical sections | [R04 §2.5] (L226–247); [X1-m6] |
| [R03 §2.15, §5.2] | SBN replays subscriptions and toggles reporting | SBN only transmits `ENABLE_SUB_REPORTING` and `SEND_PREV_SUBS` (`sbn_subs.c:43–60`); SB's own task flips `SubscriptionReporting` (`cfe_sb_task.c:296`, `:1226`, re-read) and runs the replay (`cfe_sb_task.c:1169`) | [X2-05] |
| [R03 §4 RC2] | `CFE_ES_GetTaskIDByName` is an ES record lookup | it defers to `OS_TaskGetIdByName` (`cfe_es_api.c:716–717`, re-read): an `osal.obj<task>` lookup | [X2-10] |
| [X3-m5] (reviewer statement) | "apps are loaded `RTLD_GLOBAL`" | ES loads **apps** with `OS_MODULE_FLAG_LOCAL_SYMBOLS` and **libraries** with GLOBAL symbols (`cfe_es_apps.c:470–491`, re-read); the POSIX loader maps LOCAL to `RTLD_LOCAL` (`osal/src/os/portable/os-impl-posix-dl-loader.c:79–96`, re-read) | this document |
| SPEC v1 §3.8 | constants at `cfe_es_internal_cfg.h:199, 229` | the CFGVAL wrappers are at `:198, :228` | [X2 §A] |

### 0.6 What the reviews changed (summary; the full table is `ISSUE_RESOLUTION.md`)

57 issues: X1 2 critical / 12 major / 9 minor; X2 1 / 9 / 4; X3 1 / 11 / 8. All are resolved in this document; none is rejected. The ten changes with the largest effect:

1. **Two skeleton views** (X1-C1). Universal verdicts and `inactive` come only from the certificate view, in which message loops are star-closed. Results from the h-cut view carry h in b. `kz proofcheck` rejects any certificate step in a truncated region.
2. **Containment per M** (X1-C2). M_api results are API-contract (portability) claims; σ_E transfer goes through M_impl.
3. **The R2 certificate is repaired and fully written out** (X2-01, X1-M2, X1-M3, X1-M8, X1-M1): own-actor no-path exclusion, once-PWB, MUST_KEY and allocation records, a code-extracted await predicate, a capture point for the stamped AppId, and every trusted entry listed (App. A.1).
4. **σ_U∞ and the refinements are well defined** (X1-M5): patient retries, stall outcomes, timeout-dependent defined over pre-timeout continuations, classes computed per σ.
5. **Shutdown is modelled as on pc-linux** (X1-M6, X2-04): the requester never returns and aborts after `CFE_PSP_RESTART_DELAY`; `OS_DeleteAllObjects` runs only on the PSP main thread after `CFE_ES_Main` returns and `ShutdownReq` is observed; contracts are scoped to "before the shutdown request".
6. **cFS mechanisms corrected** (X2-02 … X2-10): K-SELF.es at seven call sites; OSAL timed waits; SB-actor K-MODE; two-phase transmit and MsgLim; σ_R1 scheduling profiles; basis closure; header-carried MsgIds; loader scope.
7. **Evaluation measures what Kairos misses** (X3-C1): recall on program mutants and pre/post-fix pairs, with soundness failures counted.
8. **Feasibility repairs** (X3-M1, X3-M4, X3-M5): attach cookies instead of IP keys; S3a AST-level writer extraction; a thin MVC with estimates and a cut list.
9. **POL compliance** (X3-M9, X3-M10): N and caps are deviations; budgets are deterministic work units; replay is byte-exact.
10. **Honest prior-work positioning and stronger baselines** (X1-M12, X3-M6): InitRacer, ExceptioNULL, RVPredict/GPredict credited; a lock-reversing predictor without Φ outcomes, a typestate monitor, a DefUse-style learner, and helgrind/DRD as an external reference point.

---

## 1. Problem definition

### 1.1 Scope: "process-level"

Following v0.1 §2.1 and [R06 F1], *process-level* means ordering between the logical work of different cFS applications, service tasks and execution contexts, independent of OS process boundaries. In the native POSIX port they are all threads of one process ([F] [R05 §1 F1]; `osal/src/os/posix/src/os-impl-tasks.c:568`). The objects of interest are **framework resources**: named OSAL objects, ES app/task/lib records, service identities, SB routes and pipes, pipe capacity, TBL registry entries, phases and app states, CDS blocks, callbacks, loader symbols and files. Raw memory words are not objects of interest.

A memory-level data race found in passing (e.g. the unlocked EVS filter counters, [R02 §3.3]) goes into a separate class **M**. It is never counted as an order violation ([R02 §8.4 item 7]).

### 1.2 Actors, instances, once(τ) and the four identities

**Actors** `T = T_osal ∪ T_raw ∪ T_env` [F] ([R01 §2.6]; [R02 §1.1]):

- `T_osal`: tasks created by `OS_TaskCreate`: cFE core main tasks, app main tasks and child tasks (`ES_BG_TASK`, TIME's tone and local children, FM's child, …).
- `T_raw`: threads without an OSAL task record: the PSP main thread (the "ES startup thread" running `CFE_ES_Main`, later `OS_Application_Run`), the console writer, the OSAL timebase thread `cFS-Master`, signal-handler contexts, and the `linux_sysmon` worker (absent here, [R01 §2.6]).
- `T_env`: environment roots (ground commands, SBN peers, the stop command). They have no incoming HB edges ([R04 §4 row 17]).

A **static actor** τ is a closure entry: a spawn site plus an entry function (§3.1). An **actor instance** is `(τ, generation)`. Skeleton nodes belong to static actors.

**once(τ)** [D] (fixes [X1-M2]). τ is *once* iff in every execution at most one instance of τ exists. It is proved, never asserted:
- the PSP main thread is once by definition (it is the process entry, A3);
- an actor with spawn sites `s_1 … s_k` is once iff (i) every `s_i` lies in a once actor, (ii) no `s_i` lies in a star-closed loop region of the certificate view (§4.3), and (iii) the sites are pairwise exclusive: they lie in the same once actor and no skeleton path connects any two of them in either direction (a run of an actor is one path, so two exclusive sites cannot both execute);
- environment roots and asynchronous contexts (signal handlers, the exception path) are never once.

The facts used are recorded in every certificate that relies on them and are re-checked by `kz proofcheck` (§4.5). For actors that are not once, frontier and once-PWB arguments are forbidden. In the restart family `B_restart` (one documented request, §3.8), the skeleton is duplicated per generation `g ∈ {0, 1}`, each copy with its own spawn site, so that each copy is once.

A node `n` is **once** iff its actor is once and `n` is not inside a star-closed loop region: `n` then occurs at most once per execution.

**Identity record** per instance: `⟨kind, osal_id, serial, tid, name, creator_event, generation⟩`. OSAL IDs are `(type << 16) | serial` and change across runs; cross-run correlation uses `(type, name, creator, creation ordinal)` ([F] [R01 §1.2, §6.2 item 3]).

**Four identity fields per event**, never merged ([R06 F1]; v0.1 §2.1):

| Field | Definition | Why it is separate (cFS fact) |
|---|---|---|
| `exec(e)` | the actor instance that executes `e` | SB code runs in the caller's task ([R02 §2.4, §2.8]) |
| `owner(e)` | the ES app whose task set contains `exec(e)` at `e` (generation-aware); `⊥` for raw threads | an app has a main task and child tasks ([R02 §1.12]) |
| `svc(e)` | the stack of active frames: **API frames** (SB, EVS, TBL, …) and **on-behalf frames** `cb(registrant)` | TIME synch callbacks run in TIME's task for the registering app ([R02 §5.3]); FS callbacks run in `ES_BG_TASK` for SB/TBL ([R02 §6.1]); OSAL timer callbacks run in the timebase thread for the timer's creator ([R01 §3]) |
| `stamp(e)` | an identity value the code passes or stores explicitly (the `AppID` argument of `CFE_EVS_SendEventWithAppID`; the pipe owner stored by `CFE_SB_CreatePipe`) | R2: the ES task executes SB code that stamps SB's unpublished AppId ([R2g §2]) |

### 1.3 Resources, kinds, lifecycle automata, generations and keys

A resource is `r = ⟨kind, scope, key, gen⟩`.
- `scope ∈ {process, persistent}`. Persistent resources survive the process: CDS, boot record, reset area, files on `/dev/shm/osal:RAM` (mounted as `/ram`), `EEPROM.DAT` ([R01 §5.3]).
- `key` is a name, numeric ID, `(MsgId, pipe)` pair, slot index, path, symbol with scope, or an **allocation record** `ρ(n)` (the record allocated by reservation node `n`, §4.2). In static mode a key may be **symbolic** (e.g. "the string in CF table field `chan[i].sem_name`") or `⊤`.
- `gen` is the OSAL serial, the cFE resource-ID serial ([R02 §6.2]) or a restart generation.

Each kind `k` has a lifecycle automaton `A_k = (Q_k, q0_k, Σ_k, δ_k)` (typestate; Strom & Yemini, IEEE TSE 1986 [V-me, R06 §9]). Anchors are in [R01 §1.5–1.7], [R02 §1–7] and §3. Examples:

| Kind | States | Publication anchor (LP) |
|---|---|---|
| `osal.obj<T>` | FREE → ALLOC → RES_C (name invisible) → RES_CN (name visible, RESERVED, type lock **not** held) → LIVE → RES_D → FREE | final ID write under the type lock, `osapi-idmap.c:1092`, unlock `:1105` ([R01 §1.5]) |
| `es.app` / `es.task` / `es.lib` | FREE → RESERVED → LIVE → RESERVED (cleanup) → FREE | script apps: reserve `cfe_es_apps.c:821`, publish `:869`, failure free `:893`; core apps: reserve `cfe_es_start.c:803`, publish `:818`, failure clear `:823` (re-read); task record `cfe_es_apps.c:694–703` |
| `es.self(exec)` | ABSENT → LIVE (both the executing task's task record and its owner's App record resolvable) | the publications above; read by `CFE_ES_GetAppRecordByContext` / `GetTaskRecordByContext` (§3.3) |
| `cell(loc)` | INVALID → VALID (generic: a shared location whose value is a resource identity) | the stores to `loc` that make its value valid; e.g. `CFE_SB_Global.AppId` written through `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` (`cfe_sb_task.c:138`). Display names such as `svc.identity(SB)` are labels of a cell, not Φ entries |
| `evs.reg(slot)` | UNREG → REG; `CFE_EVS_Register` first memsets the slot (`cfe_evs.c:56`, re-read): a disabler on re-registration | store `cfe_evs.c:106`, no lock ([R02 §3.1]) |
| `sb.pipe` | FREE → RES (OSAL queue name visible) → LIVE → RES_D → FREE | `cfe_sb_api.c:153,163,190–194` |
| `sb.route(m,p)` | NONE ↔ ROUTED | `CFE_SB_SubscribeFull` under the SB lock ([R02 §2.3]) |
| `sb.queue(p)` | depth `0..cap_B(p)` | `cap_B(p) = min(requested, msg_max)` **only when euid ≠ 0** (`osal/src/bsp/generic-linux/src/bsp_start.c:56–74`; truncation `os-impl-queues.c:98–105`, re-read); requested depth when euid = 0 [I: up to the kernel's hard limit]. [M] `msg_max` 10, 22 of 26 pipes truncated ([BM §3.1]) |
| `sb.msglim(m,p)` | `BuffCount 0..MsgId2PipeLim(m,p)` | limit per subscription; default `CFE_PLATFORM_SB_DEFAULT_MSG_LIMIT` = 4 (`cfe/cmake/sample_defs/example_platform_cfg.h:1098`); checked at the route snapshot (`cfe_sb_priv.c:1058–1064`, re-read) |
| `sb.mode.subreport` | OFF ↔ ON | store in `CFE_SB_SetSubscriptionReporting` (`cfe_sb_task.c:1226`), reached from SB's command handler `cfe_sb_task.c:296` (re-read) |
| `es.phase` | EARLY_INIT < CORE_STARTUP < CORE_READY < APPS_INIT < OPERATIONAL (monotone) | `cfe_es_start.c:87,183,194,216,234` |
| `es.appstate(rec)` | UNDEFINED < EARLY_INIT < LATE_INIT < RUNNING < WAITING < STOPPED | raise `cfe_es_api.c:573` (level by caller type and requested state, `:536–563`), `:471`, `:399` (external apps only); scan `cfe_es_apps.c:1144` (external apps, from RUNNING) (re-read) |
| `es.shutdown` | RUN → SHUTDOWN_REQ → TEARDOWN (process scope; §3.5) | `ShutdownReq = true` (`psp/fsw/pc-linux/src/cfe_psp_support.c:87`); `OS_DeleteAllObjects` (`cfe_psp_start.c:507`) (re-read) |
| `tbl.table(name)` | FREE → REGISTERED → LOADED → ACTIVE, with pending substates and an access lock flag | [R02 §4.2–4.5] |
| `es.cds(name)` (persistent) | ABSENT → REGISTERED(new \| restored) | `es/cfe_es_cds.c:311–477`; restore also within one run after a restart or reload (`:342–348, :461–472`) |
| `time.synchcb(slot)` | EMPTY → SET → EMPTY; keyed by **app index, without generation**; unlocked check-then-set | `cfe_time_api.c:628–640` (re-read); cleared by index `cfe_time_utils.c:1026–1034` |
| `loader.sym(name, scope)` | ABSENT → DEFINED; `scope ∈ {GLOBAL, MODULE(m)}` | provider `OS_ModuleLoad` with constant-evaluated flags: apps LOCAL, libraries GLOBAL (`cfe_es_apps.c:470–491`) |

### 1.4 Operations, effects, linearization points, capture points and model levels

An **operation** `op ∈ Φ.ops` has a binding (symbol or AST pattern, §3.17), parameters with roles (name, id, out-id, AppId, stamp, buffer, msg), an outcome map from result codes to `{ready, not_ready(q), error}`, and a list of **effects**:

- `lifecycle(r, a ∈ Σ_k) when guard`;
- `use(r)`;
- `sync(obj, role)`, `role ∈ {acq, rel, give, take, flush, put, get, create, start, exit, join, observe, timeout}`;
- `env(input)`;
- `mark(handler_begin | handler_end)`.

**Every conjunct of a requirement has its own LP** [D] (fixes [X1-M1]). For a lifecycle effect, the LP is the instruction-level event inside the API at which the effect takes place. For a use, each conjunct `P_j` of `Req` declares the point at which the program reads the state that `P_j` is about:

- **registry conjuncts** (the state lives in a framework table): the LP is the read inside the API, e.g. EVS's slot check;
- **value conjuncts** (the requirement is about a value the caller passes, role `stamp`, `key` or `id`): the LP is the **capture point**, the load from which the argument's value derives. `kz-static extract` (AST def-use, S3a) emits `derives(arg, loc, site)` when the argument is a copy chain (loads, stores, parameter passing, casts without arithmetic) from a single shared location `loc` read at `site`. The conjunct is then evaluated at `site` over the support location `loc`, i.e. over `cell(loc)`. This is the **translation rule T-DERIVE**: it is generic and carries no knowledge of SB. If no unique `derives` fact exists, the capture point is the call itself, and every certificate that would need it is withheld (`unknown(writer_incomplete)`).

Examples [F]:

| API | Effect / conjunct | LP |
|---|---|---|
| `OS_*Create` | `lifecycle(osal.obj, name_visible)` | `OS_OBJECT_INIT` writes `name_entry` (`os-shared-idmap.h:106–111`), type lock released |
| | `lifecycle(osal.obj, publish)` | final ID write under the type lock, `osapi-idmap.c:1092` |
| `OS_*GetIdByName` | `use(osal.obj(name))` | the name search under the type lock |
| `CFE_ES_GetAppID` | `use(es.self(exec))`; out-pointer store | `CFE_ES_GetAppRecordByContext()` under the ES lock (`cfe_es_api.c:748–763`, re-read) |
| `CFE_SB_TransmitMsg` | `use(sb.route(hdr.MsgId, ·))` | **route_lookup** under the SB lock (`cfe_sb_priv.c:1007–1104`); then one **put** LP per destination outside the lock (`:1177–1216`) (§3.9) |
| `CFE_EVS_Register` | `lifecycle(evs.reg(slot(owner)), clear)` then `REG` | memset `cfe_evs.c:56`; store `:106` |
| `CFE_EVS_SendEventWithAppID` | `valid(arg.AppID)` (value conjunct) | capture point from `derives` (e.g. the load of `CFE_SB_Global.AppId` at argument evaluation in SB code, `cfe_sb_api.c:226–228`) |
| | `evs.reg(arg.AppID) ∈ {REG}` (registry conjunct) | the slot check, `cfe_evs.c:185–194` |
| `CFE_ES_WaitForSystemState` | raise (`lifecycle(es.appstate(owner))`) then `observe(SystemState)` | the store under the ES lock at `cfe_es_api.c:573`, only if `GetAppRecordByContext()` ≠ NULL; then each unlocked read of `SystemState` (`:585`) |

**Readiness is evaluated per conjunct at its LP.** The program's own outcome code at the use is a consistency check on the computed state (O4, §2.5). Value conjuncts map to their own outcome codes (for K-ID: `ILLEGAL_APP_ID` ⟺ the value conjunct failed; `NOT_REGISTERED` ⟺ the registry conjunct failed, `cfe_evs.c:185–194`).

**Model levels** [D] (A's M_api/M_impl, [A §1.5]; containment per level fixes [X1-C2]):

- **M_api**: each OSAL API is one atomic transition at its main LP; create is atomic w.r.t. lookup; lookups return found or not found. `Exec(B,σ,M_api)` is the set of executions of a **hypothetical OSAL backend that conforms atomically to the OSAL API contract**. M_api verdicts are therefore **claims relative to the API contract** (portability claims), not claims about the POSIX binary.
- **M_impl**: for the OSAL idmap, task, binsem and queue paths, the skeleton builder projects the OSAL shared and POSIX source onto a lower vocabulary Φ_L-1 (pthread mutex and condvar incl. `pthread_cond_timedwait`, `sem_*`, `mq_*`, `clock_nanosleep`, `pthread_create/cancel/join`). The API is split at its LPs, the RESERVED windows appear, the bounded RESERVED wait is modelled with **both** wake causes (§3.2), and the lock-leak path exists ([R01 §1.5–1.8, §2.2]).
- OSAL-internal locks constrain interleavings only under M_impl. They never create shape-HB edges at either level ([R01 §3, §6.2 item 2]).
- **M-relevance.** An operation is *M_impl-relevant* if M_impl models it differently from M_api: idmap create, lookup, delete and token conversion; task create, prepare and delete; binsem; queue create, put and get. A cone (§4.10) or certificate is M_impl-relevant if it contains such an operation.
- **When the levels disagree**, the report says so:
  - protected under M_impl, violable under M_api → **portability hazard**;
  - violable only under M_impl → **implementation-defect candidate** of OSAL POSIX at the pin;
  - an M_api-only certificate contradicted by an observation of the real binary (O4(b)) → **portability-hazard evidence** (the binary is outside the API-contract model there), never a model fault.

### 1.5 Events, templates, traces and concretization

An event is `e = ⟨id, actor, seq, op, phase, args, outcome, keys, site, ι(e), lp_intervals, t⟩`:
- `seq` is the per-actor index (the program-order token);
- `phase ∈ {call, return, point}`;
- `outcome` is the result the program itself received ([R06 F11]);
- `ι(e)` is the identity quadruple;
- `lp_intervals` brackets the LP of each conjunct between two observed points (§4.8);
- `t` is a timestamp kept **as evidence only**: physical time gives no HB (Lamport, CACM 1978 [V-abs]).

A **trace** π is a sequence of events in which each actor's events appear in program order.

A **static template** has the same type with symbolic fields and a path condition. Templates are **context-qualified**: `(static actor, inlining call string, site)` [D]. The SB event site inside `CFE_SB_CreatePipe` reached from `CFE_ES_TaskInit` is therefore a different template from the same site reached from ES's command loop. Recursion makes the affected summary `unknown`. Inlining full call strings is worst-case exponential in call depth; the fallback is stated in §4.3 (fixes [X1-m7]).

A template `t` *concretizes* to an event `e` (`t ⊒ e`) iff the actor and call string of `e` match `t`, `site(e) = site(t)`, and the fields of `e` satisfy `t`'s constraints. This single type lets the static, trace and model layers share one checker.

### 1.6 Readiness contracts

A contract is `κ = ⟨id, U, sel, Req, Oracle, Allowed, harm, prov, authored, guard, scope⟩`:

- `U`: a use pattern (operation plus site predicate, in the pattern language of §2.3).
- `sel`: a resource selector from event fields to a key (e.g. `osal.countsem(arg.name)`, `evs.reg(arg.AppID)`).
- `Req`: a conjunction `P_1 ∧ … ∧ P_n` of required lifecycle or value predicates, each written in the core expression language (§2.3) and each with its own LP (§1.4).
- `Oracle`: the **framework validity predicate** for each conjunct. Examples: `CFE_ES_AppRecordIsMatch(CFE_ES_LocateAppRecordByID(a), a)`; `EVS_GetAppDataByID(a) != NULL && EVS_AppDataIsMatch(...)` ([A §7.2]). It is never executed by Kairos. Statically its read set defines the **support locations** of the conjunct (§4.2). Dynamically the program evaluates it, and the outcome code is the observed image of the conjunct (O4) ([J1 X9]).
- `Allowed`: documented not-ready behaviours from a **finite vocabulary**, each with a **continuation horizon taken from a reference** [D] ([J1 X2]):

| Allowed entry | Meaning | Horizon (reference) | Static check | Trace check | Explorer check |
|---|---|---|---|---|---|
| `none` | no not-ready outcome is acceptable | the use itself | — | — | — |
| `documented_drop(src)` | the API drops the request silently by documented design: EVS's own-ID guard (`cfe_evs_utils.c:613–623`); **SB's per-task event recursion guard and missing-task-record drop** (`cfe_sb_priv.c:305–328, 773–790`, re-read) | the use itself | outcome class | outcome code | outcome |
| `returned_and_tested(codes)` | the use returns a documented code; the caller branches on it and reaches its normal continuation | until the next operation of the same actor on the same resource, or actor exit | path check within the horizon | continuation events; trace ends early → `unknown(horizon)` | suffix |
| `retry_until_ready(codes, bound)` | the caller retries on the not-ready codes | the loop's bound, constant-evaluated in the build (e.g. CF's effective `CF_STARTUP_SEM_MAX_RETRIES`) | loop unrolled exactly (§4.3) | later iteration observed ready within the bound | suffix within the bound |
| `fallback(op′)` | the caller uses a documented alternative | the next operation of the actor | path check | next event | suffix |
| `defer_by_design(src)` | the reference documents the deferral (TO_LAB subscribes after OPERATIONAL, `to_lab_app.c:64–66`) | the documented deferral point | path check | event | suffix |
| `exit(code, src)` | a documented exit | actor exit | path check | event | suffix |
| `unspecified_by_reference` | no reference says whether the not-ready outcome is acceptable | — | — | — | — |

  `unspecified_by_reference` sets harm `unspecified_by_reference`. For route and capacity contracts with no must-deliver reference it also sets the outcome label **LOSS-UNDER-ORDER**.

- `harm`: the effect class when the outcome is not allowed. Vocabulary: `event_lost_silently`, `status_ignored`, `wrong_identity_stamped`, `app_error_exit`, `app_zombie` (record used forever, never cleaned), `service_task_absent` (a core task exits while the system continues), `processor_reset_requested`, `process_abort` (on pc-linux a reset request aborts after `CFE_PSP_RESTART_DELAY`, §3.5), `panic` (core-sync failure), `task_never_runs`, `hang_or_deadlock`, `message_lost`, `unspecified_by_reference`.
- `prov`: provenance records `(kind ∈ {api_doc, code_admission, code_pattern, issue, researcher}, path:line, sha256 of the text)`.
- `authored`: authoring time, author, and which design cases (C1, C2, C3, C5, R2) the author had inspected for that API ([R06 §5.3 item 4]).
- `guard`: version guard (e.g. `cfe ≥ 6.6.0`) for profile reuse across revisions (§7.6).
- `scope` [D] (fixes [X1-M6], [X2-04]): every contract carries the implicit scope conjunct **"no SHUTDOWN_REQ LP of `es.shutdown` precedes the use's LP"**. Occurrences outside the scope are instances of the separate family **K-TEARDOWN** (§3.5), classified and reported apart.

**Instances.** An instance is `i = (κ, t, key)` for a context-qualified template `t` matching `U`. For a linearization λ of an execution, an occurrence `u` of `t` and conjunct `P_j`, let `state_λ(r, P_j, u)` be the state of `A_k` after the lifecycle effects on `r` (same generation) whose LPs precede the LP of `P_j` at `u`. The **per-occurrence outcome** of an in-scope occurrence is:

- `READY` iff every conjunct holds at its LP;
- `TOLERATED` iff not ready and the outcome and continuation satisfy `Allowed` within its horizon;
- `VIOLATING` iff not ready and not allowed;
- `NOT_READY_UNJUDGED` iff not ready and `Allowed = unspecified_by_reference`.

An execution is **bad** for `i` iff some in-scope occurrence is `VIOLATING`, or, where Allowed is `unspecified_by_reference`, `NOT_READY_UNJUDGED`. The outcome label says which meaning was used. Liveness outcomes (`stall`) are defined in §1.8.

**Violation chains** ([v0.1] §8; [R06 §3]): `i₁ ⇝ i₂` when an occurrence of `u₂` exists only because of `u₁`'s not-ready outcome (control or data dependence; for traces, frame containment plus Φ `emits` annotations). A chain records **every** branch of the consequence, not one (fixes [X2-02]). Example [F]: 17 of R2's 24 identity uses are `SEND_NO_SUBS` events from ES's EVS packets (MsgId 0x808) transmitted before any route ([R2g §2]).

### 1.7 The analysis basis B

`B = ⟨Build, Config, Init, Env, Input⟩`. Every item is recorded with its value, its source and its policy status (fixes [X2-08], [X3-M3]):

- **Build**: commits; gcc 13.3.0 and the recorded flags ([R06 K1]); `link.txt` archive semantics ([R07 §3.4]); generated files by hash, including `cfe_static_module_list.c` (core order, [R02 §1.3]) and the generated module version table, whose absence changes R2's count from 24 to 11 ([R2c §4.2, §7]).
- **Config**:
  - the **startup-script selection rule**, evaluated by `kz-static basis`: on a PROCESSOR reset ES first tries the volatile `/ram/cfe_es_startup.scr` (`CFE_PLATFORM_ES_VOLATILE_STARTUP_FILE`) and falls back to the non-volatile script (`cfe_es_apps.c:130–168`, re-read); on POWERON it uses the generated script (2 libraries + 15 apps, [R03 §6.1]). The path actually opened is recorded by the closure boot (O5);
  - built tables (`*.tbl`) cross-checked byte for byte against their source initializers through the gcc DWARF layout ([A §4.1 step 4]); platform constants constant-evaluated in their TU; `osconfig.h` limits; the PSP module list.
- **Init** (the persistent state before the run): boot record, CDS and reset-area SysV segments, user-reserved memory, `/dev/shm/osal:RAM`, `EEPROM.DAT` ([R01 §5.2–5.3]); **decoded CDS contents and critical-table images**, since critical tables are restored from CDS on PROCESSOR resets (`cfe_tbl_api.c:117–131`, re-read). A snapshot S0 is taken per run. The **Init hash is content-only** (no mtimes; `EEPROM.DAT`'s mtime changes every boot, [BM]).
- **Env**: kernel (`6.18.44-fc-v114`, drifted from the recorded `-fc-v80`, [R07 §1]); 4 vCPU; scheduling policy as realized (`SCHED_OTHER` for all 27 threads, [M] [BM §0]); affinity (the PSP pins `CFE_*` tasks to CPU 0, `cfe_psp_start.c:184–189`, not yet measured, [R06 K5]); **euid** (decides queue truncation, §1.3); **CWD** (fixes the `/cf` mapping `cfe_psp_start.c:378`, the SysV key files and `EEPROM.DAT`); effective queue depth (`msg_max` 10); platform capabilities relevant to providers (`/proc/schedstat` absent, [R01 §2.6]); **stratum** cold (first run after a VM reboot) or warm ([BM]: 2570 ms vs 1613–1633 ms to OPERATIONAL; 849 ms vs 2.2–3.3 ms load span).
- **Input**: external commands with **event-based** triggers; the **reset-type input** (`-R` on argv, else the boot record's `NextResetType`, `cfe_psp_start.c:411–434`, re-read); the stop method (PI decision D17).

**INIT** [D] (fixes [X1-m1], [X2-12]). INIT is the **program-entry state**: the static-initializer image of the closed program for process-scope locations, and the decoded S0 for persistent-scope locations. EarlyInit stores (e.g. `memset(&CFE_ES_Global, 0, …)` at `cfe_es_start.c:82`; `memset(&CFE_SB_Global, 0, …)` at `cfe_sb_init.c:50`, re-read) are ordinary W/D writer entries, ordered against falsifiers and uses by MB, once-PWB and the frontier rule like any other writer.

**Run sequences and generations.** For cross-run contracts (K-PERSIST), the end state of run *n* becomes `B_{n+1}.Init`; the reset type of run *n+1* depends on how run *n* ended ([M] PO → PR → PO after signal stops, [R01 §5.2]). Within one run, restarts and reloads create generations (`B_restart`), and K-PERSIST is also checked across generations (CDS `ALREADY_EXISTS` within one run, `cfe_es_cds.c:342–348`).

**Normalization** (§4.13): a run that did not end through `CFE_PSP_Restart(POWERON)` leaves the SysV segments and a boot record with `NextResetType = PROCESSOR` ([X3-M3]). Before any further pooled run, one unobserved normalization boot is made; Init is then checked against the canonical S0.

Every verdict carries the hash of B. **A configuration variant is a new basis B′, never a "new bug"** ([R06 §5.4]).

### 1.8 Scheduling and timing semantics σ

`Exec(B, σ, M)` is the set of maximal executions. Transitions are effects at LP granularity [D]. Loops are interpreted on the skeleton views of §4.3: every semantics is defined on the **certificate view** (no horizon); the explorer explores the **exploration view** and its results carry h in b (fixes [X1-C1]).

| σ | Definition | Role |
|---|---|---|
| **σ_U** (untimed) | Any interleaving of enabled transitions. `OS_TaskDelay` is a yield. Every **timed wait** (an await with a timeout: soft barriers, polls, the OSAL RESERVED wait's timed wake) may take its timeout transition at any point after it starts. A **count-bounded retry** takes its exhaustion branch **only after the reference number of attempts** (exact unrolling, §4.3). Polls read the current value. Priorities and affinity impose nothing. `CFE_PSP_Panic` (→ `abort()`, `cfe_psp_support.c:116–121`) is terminal. `CFE_PSP_Restart` is terminal **for its caller** (it never returns: `OS_TaskDelay(CFE_PSP_RESTART_DELAY)` then `abort()`, `cfe_psp_support.c:60–107`, re-read); the other actors continue (§3.5) | Reference semantics for certificates and model classification |
| **σ_U∞** (patient) | σ_U without timeout transitions: no timed wait times out. A **delay-paced retry** (a count-bounded loop whose iterations are separated by a Φ delay op, recognized by pattern, §4.3) is **patient**: the failure of its **last** attempt, which exhausts the loop, is enabled only at **quiescence** (no other transition is enabled), i.e. when its predicate is permanently false; until then the last attempt waits (is not scheduled) until its predicate holds and then succeeds. Because delays are yields in σ_U, this only restricts σ_U's scheduling. Scheduling is **strongly fair** (every transition enabled infinitely often is taken infinitely often; the strongly fair scheduler of fair stateless model checking, Musuvathi & Qadeer PLDI 2008 [V-abs, R05 §2.1 L111]). In the explorer's finite models every execution is finite, so maximality implies fairness [I] | Separates "violable only through a timeout" from "violable in the ordering logic", and exposes liveness failures that timeouts mask |
| **σ_R1[P_sched]** (RT, one core) | Fixed-priority preemptive scheduling on one CPU, **parameterized by a cited target scheduling profile** `P_sched` (fixes [X2-07]): policy and timeslice; the priority-mapping function; the startup/root-thread priority; priority inheritance on mutexes; per OSAL backend. Delays **block** with a discrete-time wake order computed from constant-evaluated delay arguments (fixes [X1-m5]); computation between blocking points takes zero time (assumption A-ZT, recorded on every σ_R1 verdict that depends on a delay). Timeouts as in σ_U. Model-only by default | Platform classes where priorities order (C3 at 6.4.1, [LED §1a]) |
| **σ_E** (ENV) | The executions of the real binary under the recorded Env. Not modelled; sampled by observation and confirmation runs | Empirical witnesses and frequencies |

**The POSIX scheduling profile** `P_sched^posix` [F] (for any RT realization or σ_R1 claim about OSAL POSIX): when RT scheduling is permitted, OSAL selects `SCHED_RR` unless FIFO's priority spread exceeds `OS_MAX_TASK_PRIORITY` (255), which never holds on Linux (1..99) (`os-impl-tasks.c:348–365`, re-read); the root thread, which runs `CFE_ES_Main`, gets the highest priority (`:391–409`); OSAL priorities are compressed many-to-one by `OS_PriorityRemap` (`:78–106`); mutexes use `PTHREAD_PRIO_INHERIT` (`os-impl-mutex.c:82`). Profiles for RTEMS and VxWorks (6.4.x, H-V) are `unspecified_by_reference` until their backend sources are part of a basis (D13); no σ_R1 verdict is issued for them before that. [I] Under `P_sched^posix` the core App-record window cannot open on one CPU while the startup thread is runnable, because the startup thread outranks every task.

**Outcomes of a maximal execution relative to instance i** (fixes [X1-M5]):
- `good`: `u` is reached (in scope) and every occurrence is READY or TOLERATED;
- `bad`: some occurrence is VIOLATING (or NOT_READY_UNJUDGED where Allowed is unspecified);
- `stall`: `u` is not reached, and the execution ends in a quiescent state in which `u`'s actor, or an actor it awaits transitively, is blocked on a predicate that is false (a permanent stall; harm `task_never_runs` or `hang_or_deadlock`);
- executions that do not reach `u` for other reasons are irrelevant to `i`.

**Containment lemma, per model level** (L-CONT, §6.2; fixes [X1-C2]). Under A7-impl (Φ_L-1 semantics over-approximate kernel, libc and OSAL POSIX on the M_impl paths), A7-api (the remaining OSAL primitives are linearizable at their LP on the real backend, A10), A3, A4, A9 and A-SKEL:

`Exec(B,σ_U∞,M) ⊆ Exec(B,σ_U,M)` and `Exec(B,σ_R1,M) ⊆ Exec(B,σ_U,M)` for each M, and `Exec(B,σ_E) ⊆ Exec(B,σ_U,M_impl)`.

`Exec(B,σ_E) ⊆ Exec(B,σ_U,M_api)` is **not** claimed: M_api excludes real POSIX behaviours (`INCORRECT_OBJ_STATE` in RESERVED windows, the type-lock leak, silent task loss). For a cone that contains no M_impl-relevant operation, the two levels have the same projected executions (Lemma L-MLEVEL, §6.2) [I].

**Transfer rules** [D]:

1. A universally quantified class (protected, tolerated, fixed-order) proved under σ_U at level M and bound b holds under every σ′ ⊆ σ_U **at the same M and the same b**, for every execution of σ′ that reaches the use. If no execution of σ′ reaches the use, the σ′ class is `inactive_σ′`.
2. **Transfer to σ_E** is allowed only from M_impl, or from M_api when (i) the instance's cone and certificate contain no M_impl-relevant operation, or (ii) the M_impl result exists and agrees. Otherwise the σ_E cell stays `unknown(model_level)`, and the M_api result is reported as a portability claim.
3. A witness found under any σ′ is a witness under σ_U at the same M.
4. Nothing transfers *into* σ_E as a frequency or class. A σ_E class needs natural witnesses (§1.10).
5. A blanket "no transfer across σ" ([A §6.3 item 3]) is dropped as too weak; B's transfer of a trace-local certificate ([B §4.3]) is dropped as too strong.

**Refinements, each computed per σ and stored with its own bound** [D] (fixes [X1-M5], [X1-m3]):

- **timeout-dependent**: class(σ_U) = schedule-dependent, **every** bad σ_U execution takes at least one timeout transition (a timed-wait timeout, or a delay-paced retry exhausted after its reference bound), and for each bad execution, the σ_U∞ continuations from the state just before its first timeout transition reach only `good` outcomes.
- **timeout-masked-liveness**: as above, but some such σ_U∞ continuation reaches `stall`. The class stays schedule-dependent, with liveness harm; the timeout hides an ordering or liveness defect (e.g. a raise skipped inside the core App-record window makes `MainTaskSyncDelay` unsatisfiable, §3.3; a provider task that never runs, K-SPAWN F7).
- If σ_U∞ has its own bad executions, the violation lives in the ordering logic; no refinement.
- **timeout-rescued** (good-side flag): every good σ_U execution takes a timeout transition, and σ_U∞ has no good execution. The instance is good only because some wait gave up.

Confirmation runs in which a hold makes a program timeout fire are labelled `timeout-exercised` (naturalness axis), never natural σ_E.

### 1.9 Ordering relations

Φ's HB rules are inference rules `e₁ → e₂` with a matching condition, a stratum, a robustness tag and a code citation.

**Strata** ([C §0]) [D]:

| Stratum | Meaning | Used for |
|---|---|---|
| **S** (structural) | Holds in every execution that contains both endpoints on the given path: program order within an actor (inside one handler for loopers), task create → child start, join, script sequencing, and **guarded observation edges** whose writer set is complete, whose predicate is extracted from the code, and whose only alternative continuation is terminal | MustBefore and certificates |
| **O** (observed synchronization) | Holds in the observed execution only: outermost mutex release → next acquire, semaphore give → take, delivered queue/SB message, poll exit by condition with a soft alternative, timer add/set → callback, TBL request and flag protocols, FS request → completion, outcome-implied reads-from | HB of a trace; reorderable in prediction, subject to the primitive's semantics |
| **C_σ** (schedule constraint) | Fixed-priority preemption on one CPU, affinity, under P_sched | Feasibility under σ_R1 only; never an HB edge ([R04 §4 row 20]; [R05 §2.7]) |

**Looper rule** [D] (fixes [X1-M7](b)). On an actor that loops over `ReceiveBuffer` and dispatches (a *looper*), program order holds **inside one handler** and from the loop's init prefix to every handler. **Between two handlers on the same looper there is no HB edge**, neither in VC nor in VC_R; shapes and the UNORDERED route are computed without it (CAFA PLDI 2014 [V-abs]; DCatch Rule-Pnreg ASPLOS 2017 [P]; [R04 §2.7, §4 row 8]). The sequential execution of handlers survives only as a **prediction constraint**: handlers of one looper do not overlap, and handlers of messages from the same pipe run in dequeue order, which is the FIFO order of their enqueues.

**Trace-side image: robustness tags** ([B §4.2]):
- `unconditional` (S-rules that need no observation);
- `obs-fatal` (S, guarded observation with terminal escape);
- `obs-soft` (O, log-and-continue escape);
- `schedule-chosen` (O: locks, semaphores, messages);
- `assumed` (holds only under assumption A4: volatile flags, seqlock, TBL result flags, the SB subscription-reporting flag).

**Relations computed:**

1. **MustBefore** (static; §4.4). `a ∈ MB(n)` iff in every execution of `Exec(B,σ_U,M)` in which `n` occurs, every occurrence of `n` is preceded by an occurrence of `a`. It is a greatest fixpoint over the certificate view, using S-rules only, computed per M.
2. **Precedes-whenever-both-occur** `PWB(a, b)`: in every execution containing both, every occurrence of `a` precedes every occurrence of `b`. Certified by the **frontier rule** (`once(actor(a))`, `∃ m` in `a`'s actor with no skeleton path `m →* a` and `m ∈ MB(b)`) or by the **once rule** (`once(a) ∧ a ∈ MB(b)`), §4.5.
3. **HB_π** on a trace π: the transitive closure of S- and O-edges (looper rule applied), computed with vector clocks (Fidge 1988; Mattern 1989 [V-abs]). Two clocks per event ([B §4.2]):
   - `VC` over all definite edges;
   - `VC_R` over the edges tagged `unconditional` or `obs-fatal`.

   `a ≺_R b` is a cheap trace-side prefilter for a static certificate, never an Exec-level claim ([J1 X5]).
4. **C_σ** constraints only restrict σ_R1 feasibility, and only in the explorer (never in trace prediction, §4.9).

Rules not in Φ are listed as **unmodelled**. A loop that matches the poll pattern (§4.3) but whose predicate cannot be extracted, or a protocol over app-private memory with no Φ rule (e.g. FM's parent–child queue, `fm_child.c:65–100, 162`), becomes an `unmodelled-sync` mark, and every instance whose decision depends on it is `unknown(unmodelled)`. This is the KCSAN lesson that an HB model missing one ordering mechanism floods false positives (`kcsan.rst` L366–373 [V-doc, R04 §2.4]).

### 1.10 Classes, refinements, axes, shapes, evidence and the verdict record

Fix instance `i`, basis B, semantics σ, level M and bound b. Let `E_i` be the maximal executions of `Exec_b(B,σ,M)` in which an in-scope occurrence of `u` exists, and `S_i` those with outcome `stall` for `i` (§1.8).

| Class | Definition | Proof obligation |
|---|---|---|
| `inactive(B)` | no execution reaches the use | static: the template is unreachable in the **certificate view** under B constants and message provenance (path condition as proof). An unobserved use is **not** inactive; it is `unknown(not_reached)` |
| `protected(B,σ,M,b)` | `E_i ≠ ∅`, no execution in `E_i` is bad, and `S_i = ∅` | certificate (§4.5, b = ∞ on the certificate view) or exhaustive exploration within b |
| ↳ refinement `tolerated` | protected, and some execution has a `TOLERATED` occurrence | as protected, plus a tolerated witness |
| `fixed-order(B,σ,M,b)` | `E_i ≠ ∅` and every execution in `E_i` is bad | certificate or exhaustive exploration within b. **Never** from k-of-k failing runs ([R06 S9]) |
| `schedule-dependent(B,σ,M,b)` | `E_i` contains a good execution, and `E_i` contains a bad execution or `S_i ≠ ∅` | two witnesses (bad or stall, and good). The record keeps the weaker evidence per axis |
| ↳ refinements | `timeout-dependent`, `timeout-masked-liveness` (§1.8) | per σ, with their own bound |
| `unknown(reason)` | none of the above established | reason ∈ {`not_reached`, `model_gap`, `bound`, `budget`, `key_unresolved`, `loss`, `unmodelled`, `writer_incomplete`, `horizon`, `conformance`, `model_fault`, `model_level`} |

The good-side flag `timeout-rescued` (§1.8) may accompany protected or schedule-dependent.

**Orthogonal axes** [D] (fixes [X1-m3]):
- **provider** ∈ {`present`, `none` (a conjunct is INIT_FALSE and its complete writer set is empty), `env-absent` (the only writers are Φ effects guarded by an `env` predicate that B.Env records false, with outcome agreement O4; e.g. HS `SET_RUNNING`, App. A.6), `unknown`}. A use with provider `none` is classified by its outcomes like any other: fixed-order if every reaching execution is bad, tolerated if its not-ready outcome is allowed;
- **reach** ∈ {`proved(run id | confirmed model path)`, `assumed`}. A class with `reach = assumed` reads "… if reached";
- **scope** ∈ {`in-scope`, `teardown`} (§1.6).

**Shape** ([A §1.7]), for a bad occurrence `u` with `p*` the first transition that would make the failing conjunct true; computed with the looper rule (no between-handler edges) and API-level edges only:
- **WRONG-WAY**: `u →hb p*`;
- **UNORDERED**: `u ∥hb p*`;
- **ABSENT**: no `p*` in the execution;
- **STALE**: the conjunct held earlier, then a disabling transition occurred before `u` (generation mismatch).

**Evidence**, two axes ([R06 S1]; fixes [X1-M11], [X1-M4]):

| Axis | Values |
|---|---|
| **realization** | `certificate(id)`; `model(b)`; `predicted` (trace-sound under A-COMM, §4.9); `predicted-modelled` (A-COMM failed: model-based); `predicted-extrapolated` (summary-guided extrapolation: model-based); `realized` (the order and outcome occurred on the gcc binary) |
| **naturalness** (realized only) | `natural` (no hold, no gate); `held-F0`; `gated-F1`; `timeout-exercised` (a program timeout fired during a hold); flag `derived` (the schedule was constructed) |

"Weaker" is decided per axis: realization `model(b)` ≈ `predicted-modelled` ≈ `predicted-extrapolated` < `predicted` < `realized`; naturalness `timeout-exercised` < `gated-F1` ≈ `held-F0` < `natural`.

**σ_E classes** come only from realized, natural witnesses: `schedule-dependent(σ_E)` needs a natural bad (or stall) witness and a natural good witness, each with its pool key. With realized but held or gated witnesses the label is `σ_E(held)`. No σ_E protected or fixed-order class is ever derived from runs; such σ_E cells come only from transfer rule 2. Observation counts are written `no σ_E bad witness in n of N runs (pool key); σ_E class unknown`, never "timing-protected".

**Outcome label**: `violation` (Allowed specified) / `LOSS-UNDER-ORDER` (route or capacity, Allowed unspecified) / `not-ready-unjudged` (other contracts, Allowed unspecified) / `liveness` (stall).

**Consequence** ([A §1.3], effect-oriented as in ConMem and Portend [V-abs, R04 §2.6, §2.10]): crash or undefined behaviour; deadlock; process abort; panic; app error exit; app zombie; core service task absent; status ignored and effect lost silently; event then continue; delayed but recovered.

**Sub-labels**: `portability-hazard`, `implementation-defect-candidate`, `portability-hazard-evidence` (§1.4); `assumed-visibility` (§3.14); `A-ZT` (σ_R1 delay semantics); `trusted:<n>` (number of `DECLARED_TRUSTED` entries the certificate relies on).

**The verdict record** (`kz_verdict` plus per-(σ, M) `kz_verdict_cell` records, §2.3) holds: instance id; provider, reach, scope; per cell: class, refinements, bound reference (a BOUNDS record listing every bound and its policy status, including h where the exploration view was used), realization and naturalness sets, `n of N` with pool key; shape, outcome label, consequence, harm; chain links; unknown reason; B, Φ and tool hashes; typed references to the certificate, witnesses and runs. **There is never a "safe" field.**

### 1.11 Why plain data-race HB detection is insufficient, and what is borrowed

| Phenomenon in cFS | What prior work does | What Kairos adds (no new mechanism claimed) |
|---|---|---|
| **R2**: all 24 violating uses per boot are HB-ordered **before** the publication by core serialization ([R2g §2]; [R2c §7]) | FastTrack (PLDI 2009 [V-abs]), TSan and Djit+ report unordered conflicting accesses; CP, WCP, SHB, M2 and SyncP (all [V-abs]) report reorderable pairs only ([R04 §3 L4]). DefUse (OOPSLA 2010 [V-abs]) learns def-use invariants from runs; [I] it would learn R2's wrong definition (the EarlyInit value) as normal, because every run reads it. GPredict (ICSE 2015 [V-abs]) checks generic properties on predicted traces but has "no notion of a legal error outcome" ([R04 L553]) | A reference-sourced contract with documented Allowed outcomes, checked on **ordered** pairs, and a σ- and M-qualified PWB certificate that lifts "ordered the wrong way" to `fixed-order` ([R04 §5 G2]) |
| **C1 / C5** and both App-record windows: create/lookup or publish/`GetAppID` under the same lock | HB orders them through the lock; lockset sees consistent protection ([R04 §2.3, §3 L5]). **RVPredict (PLDI 2014) and GPredict-style constraint encodings already treat the key as the conflict object and the lock as atomicity** ([R04 L259, L492]); ExceptioNULL (FSE 2012 [V-abs]) predicts "use before initialized" from one execution with static lockset pruning and SMT ([R04 L253–255]) — the closest predictive prior work for readiness | Explicit-state search instead of a solver (an engineering choice, not novelty); framework typestate with Allowed outcomes; certificates; held confirmation. Kairos's predictive soundness is **weaker** than RVPredict's on its sparse API-level traces (A-COMM, §4.9) |
| **No-subscriber loss**: a late subscribe makes a transmit a silent loss with a success return | **InitRacer's "late event handler registration"** (OOPSLA 2017 [V-abs]; [R04 L69, L367, L529]) | The cFS instance of that taxonomy, with registries as keys and LOSS-UNDER-ORDER when no must-deliver reference exists |
| **Capacity loss**: queue full drops silently ([M] 4–49 drops per boot, [BM §0]) | Event-driven and distributed HB models assume delivery ([R04 §5 G5]) | Capacity as typestate (queue depth and MsgLim); prediction and exploration ask whether a feasible order changes the outcome. G5 is genuinely open only for capacity |
| Soft, opt-in, timed barriers (`WaitForStartupSync` returns void, `cfe_es_api.c:616–619`) | EventRacer and Tian et al. recognize ad hoc synchronization but not timeout-conditional edges ([R04 §5 G3]) | Conditional O-edges, guarded S-edges, σ_U/σ_U∞ split, timeout-dependent refinements. **The novelty of the G3 formalization is an open literature check**: R04 records that a targeted search was not done ([R04 §8 item 5]) |
| Executing task ≠ owner ≠ service ≠ stamped AppId (G6) | thread identity; CAFA/DroidRacer separate thread from event; DCatch abstracts RPC pools | four identity fields, on-behalf frames, capture points |
| Configuration decides reachability and capacity (G7) | configuration taken as given | B with configuration closure; `inactive`; provider axis |
| Priority order only on RT targets (G8) | no edge, but no verdict either | σ_R1[P_sched] as a schedule constraint, verdict per σ |

**Established theory used.** No mechanism is claimed as new ([R04 §5], last paragraph):

| Element | Source (status) |
|---|---|
| HB as a partial order; vector clocks | Lamport 1978; Fidge 1988; Mattern 1989 [V-abs] |
| Feasible vs apparent races; order violations as a major class | Netzer & Miller LOPLAS 1992 [V-abs]; Lu et al. ASPLOS 2008 [V-abs] |
| Framework/event HB rules (looper, post/begin, RPC/event) | DroidRacer PLDI 2014 [V-abs]; CAFA PLDI 2014 [V-abs]; DCatch ASPLOS 2017 [P] |
| Initialization races (late registration, access before definition) | InitRacer OOPSLA 2017 [V-abs] |
| Typestate contracts | Strom & Yemini 1986 [V-me] |
| Def-use invariants; generic predictive properties; predicted use-before-init | DefUse OOPSLA 2010 [V-abs]; GPredict ICSE 2015 [V-abs]; ExceptioNULL FSE 2012 [V-abs] |
| Sound prediction with control-flow abstraction; candidate + vindication | Maximal causal models RV 2012 [V-abs]; RVPredict PLDI 2014 [V-abs]; Vindicator PLDI 2018 [V-abs] |
| Ad hoc synchronization; race coverage | Xiong et al. OSDI 2010 [V-abs]; Tian et al. ISSTA 2008 [V-abs]; EventRacer OOPSLA 2013 [V-abs] |
| Atomicity / serializability | Velodrome PLDI 2008 [V-abs]; AVIO ASPLOS 2006 [V-abs]. Left/right-mover reduction (L3): citation open literature check |
| Systematic exploration | DPOR POPL 2005 [V-abs; guarantee wording UNVERIFIED]; source/optimal DPOR POPL 2014 [V-abs; JACM extension to disabling P]; BPOR OOPSLA 2013 [V-abs]; fair SMC PLDI 2008 [V-abs; strongly fair scheduler]; PCT ASPLOS 2010 [V-abs; mechanism P]; SCTBench PPoPP 2014 [V-abs] |
| Static action HB; environment models; harnesses | SIERRA ASPLOS 2018 [V-abs]; SDV EuroSys 2006 [V-abs]; Holzmann & Joshi SPIN 2004 [V-abs]; FlowDroid PLDI 2014 [V-me] |
| Points-to and indirect calls | Andersen 1994 [V-abs]; SVF CC 2016 [V-abs]; MLTA CCS 2019 [V-abs] |
| Triggering and confirmation; outcome triage | CTrigger ASPLOS 2009, RaceFuzzer PLDI 2008, RacePro SOSP 2011, Portend ASPLOS 2012, DataCollider OSDI 2010 [all V-abs] |
| Log/order mining and potential communications | CloudRaid ESEC/FSE 2018 [V-abs]; Snowboard SOSP 2021 [V-abs] |
| Belief deviance | Engler et al. SOSP 2001 [V-me] |
| Soundiness | Livshits et al. CACM 2015 [V-abs] |

**Defensible contribution** [I] (after [X1 §4]): a σ- and M-qualified must-order certificate that lifts an observed HB-ordered contract violation to a universal fixed-order class; documented Allowed outcomes and per-conjunct LPs; combined with lock-reversing prediction, held confirmation and a recall-measured evaluation; organized around the cFS semantics G1–G8. Every individual mechanism has prior art.

---

## 2. Architecture

### 2.1 Layers and principles

```
L5  Orchestration & reporting      kz loop (feedback driver), kz report, kz dump, kz proofcheck (independent)
L4  Real-binary observation/control kz observe (F0: perf-uprobe + eBPF raw loader, attach cookies |
                                   perf-sample bring-up backend), kz hold (F0: ptrace + debug-register gates),
                                   kz gate (F1, PI-gated)
L3  Analyses (three interpreters)   abstract: kz-static static (census, MustBefore, PWB/once, certificates)
                                   concrete: kz trace (identity, HB, per-conjunct LPs, routing), kz predict, kz conform
                                   operational: kz explore (gated, late; exh and dpor engines)
L2  Formal core                     libkz-core: KEM types, Φ parser, typed core expression evaluator (one evaluator,
                                   three abstract domains), rule engine, typestate, vector clocks, verdict lattice,
                                   provenance, KZ container I/O  (no LLVM dependency)
L1  Static closure & skeletons      kz-static basis, extract (Clang AST facts, derives, writer extraction S3a),
                                   link (IR call graph, DWARF/MC binder, anchor audit), modref (S3b, conditional),
                                   skel (certificate view + exploration view)   (libkz-llvm)
L0  Inputs                          basis B (build tree, configuration, Env, Init, Input), profile Φ_cFS (text)
```

Principles [D]:

1. **One formal core.** Every component reads and writes KEM types through `libkz-core`. No component has a private notion of event, resource, edge, predicate or verdict. Φ predicates have one evaluator.
2. **The profile is data, not code.** Framework knowledge lives in versioned Φ files with provenance. Engines are framework-neutral; a lint forbids case identifiers in engine code (§9.2). App-private protocols, if ever modelled, go into a separate, provenance-carrying **app-profile layer** whose authoring cost is measured (§7.2).
3. **Proof-carrying verdicts.** Every certificate is a list of rule instances, writer entries (with transition and key statuses), once facts, derives facts and extracted predicates, each with code sites. `kz proofcheck` shares no code with `kz-static static` and re-validates every step. Certificate-mutation tests check the checker; program mutants (§8.5) check the analysis.
4. **Observation first, control second.** F0 observation of the unmodified gcc binary is the default ([R07 §7]). Control is used only for confirmation; every hold is recorded.
5. **Layers check each other.** Obligations O1–O9 (§2.5).
6. **Honest unknowns.** Every gap produces `unknown(reason)`.
7. **Zero-install core.** Every core capability works without new packages: perf-PMU uprobes, raw `bpf()` with hand-written helper prototypes and a glibc `<elf.h>` ELF reader, explicit search instead of SMT, plain C++ instead of MLIR. Every install is a PI decision (`PI_DECISIONS.md`).
8. **Deterministic analyses.** Every analysis budget is a deterministic **work-unit** count (states expanded, transitions executed, search nodes, fixpoint updates), with a deterministic search order. Wall time and peak memory are recorded as costs only (fixes [X3-M10]).

### 2.2 Components

| Component | Binary | Language, libraries | Inputs → outputs | Why this technology (measured or argued) |
|---|---|---|---|---|
| `libkz-core` | both | C++17, standard library only | — | shared formal core; LLVM-free |
| `basis` | `kz-static` | C++17 | build tree, `link.txt`, generated files, startup scripts, `.tbl`, Env/Init snapshots, argv → `basis.kz` | evaluates the script-selection rule and reset type; content-only Init hash |
| `extract` | `kz-static` | C++17, Clang LibTooling + ASTMatchers (LLVM 23 `1053047a`) | 448 FSW compile commands → `facts.kz` | [M] 448/448 TUs parse in 10 s ([R07 §3.3]); macro provenance (467 of 3,762 API call sites in macro expansions, [R07 §4.6]); constant evaluation; Doxygen extraction; **S3a writer extraction and `derives` facts at the AST level**, where field identity is explicit (fixes [X3-M4]); serialized loop-exit expressions for predicate extraction |
| `link` | `kz-static` | C++17, LLVM IR linker semantics, DebugInfo/DWARF, Object, MC | per-unit bitcode; the **gcc** `core-cpu1` and app `.so` → call graph, `anchors.kz`, anchor audit | [M] bitcode 4.4 s, closed `core-cpu1.bc` 0.35 s ([R07 §3.4]). IR is used **for the call graph only**; if IR is ever used for writes, the pass pipeline is pinned without InstCombine and SROA. The app-unit link probe of §4.1 step 6 is an early S3a item (fixes [X3-m5]) |
| `modref` (S3b) | `kz-static` | C++17 over LLVM IR | closed IR + Φ → points-to, writer sets | **conditional**: built only if the S3a O7 report shows `DISABLED` conjuncts blocking certificates; its gain is measured by ablation 17. Andersen-style ([V-abs, R05 §2.5]); SVF ([V-abs]) is a reference, not a dependency (SVF lists LLVM 21–22) |
| `skel` | `kz-static` | C++17 | IR call graph + AST facts + closure + Φ → `skel.kz` (certificate view, exploration view, once facts, message provenance) | §4.3 |
| `static` | `kz-static` | C++17 on `libkz-core` | skeletons + writer sets + Φ → census, MustBefore, certificates, verdicts, open candidates | §4.4–4.6 |
| `plan` | `kz-static` | C++17, LLVM MC | open candidates + sync skeleton + anchors + calibration → `plan.kz` | site validation by disassembly; joint debug-register allocation |
| `proofcheck` | `kz` | C++17, separate module, no code shared with `static` | certificates + `skel.kz` + `facts.kz` + Φ → accept/reject per step | trusts extraction (`facts.kz`) and re-derives everything else, including dominance, reachability, once facts, derives chains and await predicates |
| `observe` | `kz` | C (one generic BPF program per phase, system clang-18 `-target bpf`); C++17 loader on raw `bpf()`/`perf_event_open`; perf-sample backend | plan + binaries → `run-*.kzt` | [M] uprobe 0.57 µs at entry+4 ([R07 §4.7]); perf-PMU uprobes work without tracefs and count hits registered before the file is mapped ([R07] L340); attach-cookie kernel symbols present ([X3, here]) |
| `trace` | `kz` | C++17 | trace + Φ + anchors → events, identities, `HB_π`, per-conjunct LP states, observed violations, routed candidates, chains | §4.8 |
| `conform` | `kz` | C++17 | trace + skeletons → O1/O2; O8 (from S5, on S5's engine) | §4.11 |
| `predict` | `kz` | C++17; optional SMT-LIB2 emitter | trace cone + Φ + A-COMM facts → witnesses or `unknown(budget)` | explicit search; no solver (Z3 not installed, [R07 §2.4]) |
| `explore` | `kz` | C++17 | closed candidate model + σ + M + b → classes within b, witnesses | `exh` and `dpor` engines (§4.10); no external model checker fits this LLVM without an install (GenMC needs LLVM 19–22; Nidhugg's README range is LLVM 8–19 and LLVM 18 is installed **as runtime only**: an `llvm-18-dev` install would be needed, [R05 F11]; [R07 §2]) |
| `hold` | `kz` | C++17, ptrace, x86 debug registers | hold plan → controlled run + trace | [M] ptrace from root works on uid 1000; #DB trap ≈34 µs; gdb ≈203 µs per hit ([R07 §4.7]) |
| `gate` | `kz` (PI-gated, D3) | C99 `__wrap_*` objects under cFS flags; C++17 controller | variant link → gated runs | [M] `--wrap` misses 88 of 3,762 intra-TU sites ([R07 §4.2]); a link-option deviation |
| `loop` | `kz` | C++17 | all artifacts → iteration log, fixed point with its run set | §4.14 |
| `report`, `dump` | `kz` | C++17 | any artifact → Markdown/CSV views; canonical one-record-per-line text | the store stays binary and typed |

**Two binaries** [D] (fixes [X3-m7]): `kz-static` links `libkz-llvm` (one static LibTooling link, [R07 §2.2]: 56.7 MB) and holds the L1 subcommands, `static` and `plan`; `kz` links only `libkz-core` and holds everything else. `kz proofcheck` therefore links no Clang and shares no analysis code with the producer.

### 2.3 Typed formats

**One container format, KZ** [D]. Every artifact is a KZ container:
- a header carrying magic, ABI version, B hash, Φ hash, tool hash and the hash of every **parent artifact** (Merkle-style, [A §2.3]);
- a section table; **each section type carries its own schema version** (fixes [X3-m6]); each section is an array of fixed-layout little-endian records with a CRC;
- a string/blob pool.

**Schema discipline** (fixes [X3-m6]). Record layouts are declared once, in `include/kz/schema/kz_schema.h`, as **X-macro field lists**. Each list expands into (i) the C struct, (ii) the canonical dump printer, (iii) `_Static_assert`s on every offset, and (iv) an assert that `sizeof` equals the sum of the field sizes, so that no record has implicit padding. Explicit `rsv` fields fill alignment gaps and must be zero; writers zero every record before filling it. No file is generated and no external schema compiler is used. The header is included by the BPF program, the gcc-built gate objects (it compiles under `gcc -std=c99 -pedantic -Wall -Werror`, [X3, here]) and the C++ tools. Cross-artifact references are typed: `kz_ref = (artifact sha256, section type, index)`.

```c
/* kz_schema.h (excerpt; layouts are reviewed at S0 and versioned per section afterwards) */
#define KZ_REF_FIELDS(X) \
  X(uint8_t,  artifact_sha256, [32]) X(uint32_t, section, ) X(uint32_t, index, )
#define KZ_RAW_EV_FIELDS(X)                                                                  \
  X(uint64_t, t_ns,   )   /* CLOCK_MONOTONIC: evidence and search heuristic only, never HB */ \
  X(uint32_t, tid,    )   /* kernel thread id */                                              \
  X(uint32_t, tseq,   )   /* per-thread sequence assigned in kernel: program order, loss */   \
  X(uint32_t, site,   )   /* site id = the uprobe's attach cookie (never the runtime IP) */   \
  X(uint16_t, phase,  )   /* ENTRY | POINT | RET | WATCH | GATE | MARK */                     \
  X(uint16_t, flags,  )   /* STR_FOLLOWS, STACK_FOLLOWS, TRUNC, OBJSEQ_VALID, ... */          \
  X(uint64_t, objseq, )   /* per-object token taken inside the critical section; 0 = none */  \
  X(uint64_t, a, [4])     /* raw values per site decoder: registers, stack slots, derefs */
#define KZ_CERT_STEP_FIELDS(X)                                                               \
  X(uint32_t, rule, )       /* T_PO|T_SPAWN|T_OBS|T_TERM|FRONTIER|ONCE|ONCE_PWB|OWN_NOPATH|   \
                               KEY_SEP|SCOPE|WRITER|INIT_FALSE|DERIVES|PRED_EXTRACT|... */    \
  X(uint32_t, from_node, ) X(uint32_t, to_node, )  /* certificate-view nodes */              \
  X(uint32_t, site_from, ) X(uint32_t, site_to, )  /* -> ANCHORS */                          \
  X(uint32_t, phi_entry, )                         /* Φ rule / effect / binding id */        \
  X(uint16_t, tstatus,  )   /* COMPUTED|DECLARED_VERIFIED|DECLARED_TRUSTED|ASSUMED_VIS */     \
  X(uint16_t, kstatus,  )   /* MUST_KEY|MAY_KEY|NO_KEY|NA */                                  \
  X(uint32_t, view,     )   /* VIEW_CERT; any other value is rejected by kz proofcheck */
#define KZ_VERDICT_FIELDS(X)                                                                 \
  X(uint64_t, instance, )   /* (contract, context-qualified template, key) */                \
  X(uint8_t,  provider, )   /* PRESENT|NONE|ENV_ABSENT|UNKNOWN */                             \
  X(uint8_t,  reach,    )   /* PROVED|ASSUMED */                                              \
  X(uint8_t,  scope,    )   /* IN_SCOPE|TEARDOWN */                                           \
  X(uint8_t,  outcome_label, ) X(uint8_t, shape, ) X(uint8_t, consequence, )                  \
  X(uint8_t,  harm, )       X(uint8_t, rsv0, )                                                \
  X(uint32_t, sublabels, )  X(uint32_t, ncells, )   /* followed by ncells kz_verdict_cell */   \
  X(kz_ref,   chain, )      X(kz_ref, provenance, )
#define KZ_VERDICT_CELL_FIELDS(X)                                                            \
  X(uint8_t,  sigma, )      /* SIGMA_U|SIGMA_U_INF|SIGMA_R1|SIGMA_E */                         \
  X(uint8_t,  mlevel, )     /* M_API|M_IMPL */                                                \
  X(uint8_t,  cls, )        /* INACTIVE|PROTECTED|FIXED|SCHED_DEP|UNKNOWN */                  \
  X(uint8_t,  unknown_reason, )                                                               \
  X(uint16_t, refinements, )/* TOLERATED|TIMEOUT_DEP|TIMEOUT_MASKED_LIVENESS|TIMEOUT_RESCUED */\
  X(uint16_t, realization, )/* CERT|MODEL|PREDICTED|PRED_MODELLED|PRED_EXTRAP|REALIZED */      \
  X(uint8_t,  naturalness, )/* NATURAL|HELD_F0|GATED_F1|TIMEOUT_EXERCISED */                  \
  X(uint8_t,  flags, )      /* DERIVED|SIGMA_E_HELD */                                         \
  X(uint16_t, rsv0, )                                                                         \
  X(uint32_t, observed_n, ) X(uint32_t, observed_N, )  /* pooled over identical pool keys */   \
  X(kz_ref,   bounds, )     X(kz_ref, refinement_bounds, ) X(kz_ref, pool_key, )             \
  X(kz_ref,   certificate, ) X(kz_ref, witness_bad, ) X(kz_ref, witness_good, ) X(kz_ref, runs, )
/* each list expands into: struct, dump printer, offset asserts, and
   _Static_assert(sizeof(T) == KZ_SUM_SIZES(T##_FIELDS)) */
```

Section types: `STRINGS, BASIS, FACTS, DERIVES, SITES, ANCHORS, TEMPLATES, SKEL_CERT, SKEL_EXPL, ONCE, PROVENANCE_MSG, WRITERS, ALLOCS, ACTORS, RESOURCES, RAW_EVENTS, EVENTS, STACKS, EDGES, INSTANCES, CERTIFICATES, WITNESSES, HOLDPLANS, VERDICTS, VERDICT_CELLS, BOUNDS, LOSS, RUNMETA, OBLIGATIONS, PERTURBATION`.

| Artifact | Producer → consumer | Content |
|---|---|---|
| `basis.kz` | basis → all | B items with value, source, policy status; file hashes; selected script path and reset type; euid, CWD; canonical S0 and its content hash; stratum |
| `profile.kzp` (text) → `profile.kz` | human → libkz-core | Φ: kinds, ops, effects with per-conjunct LPs, bindings, rules, contracts, constants, version guards; provenance and authoring record per entry |
| `facts.kz`, `anchors.kz` | extract, link | call sites, arguments (constant, symbolic, ⊤), guards, table values, doc annotations, `derives` facts, writer sites per abstract location, escape and cast listings (A-TYPE), serialized loop-exit expressions; site → gcc address and cost class; audit |
| `skel.kz` | skel | certificate view and exploration view per actor; once facts; message-provenance sets; loop summaries with bounds and policy status; summary fallbacks |
| `census.kz` | static | instances, MustBefore (bitsets), certificates, verdicts, open candidates |
| `plan.kz` | plan | probe sites (file inode, offset, instruction class, decoder, cookie), watchpoints, gates, debug-register allocation per thread, expected cost |
| `run-<id>.kzt` | observe / hold / gate | raw events, strings, stacks, loss table, run metadata (B hash, plan hash, S0 hash, stratum, holds, observer CPU time and placement, harness version, kernel, backend) |
| `hb-<id>.kz` | trace | events, identities, edges with tags, per-conjunct LP states, routed candidates, chains |
| `witness-<cand>.kz`, `holdplan-<cand>.kz` | predict / explore | schedules with realization labels; hold points, release events, watchdog with reference |
| `verdicts.kz` | analyses → report | append-only verdict store, versioned by B, Φ and tool hashes |
| `loop-<iter>.kz` | loop | run IDs, B/plan/Φ/tool hashes, deltas, obligation results, work units spent |

**The profile DSL Φ** [D] (fixes [X3-M11], [X1-M8]). It is parsed by a hand-written recursive-descent parser in `libkz-core` and **interpreted, never compiled**. It has three sublanguages, all fixed at S0:

1. **Typed core expression language** (for Req conjuncts, guards, falsifier conditions and HB-rule conditions). Sorts: `Bool`, `Int`, `Key<K>`, `Id<K>`, `State<K>` (an enum with the order declared in the kind), `Gen`. Terms: literals; `state(r)`, `key(r)`, `gen(r)`; bound parameters (`arg.name`, `hdr.MsgId`, `exec`, `owner(exec)`); comparisons (`=`, `≠`, and `<`, `≤` on ordered sorts); `in {…}`; `∧ ∨ ¬ ⇒`; bounded quantifiers `forall r: K where φ` and `exists r: K where φ` over **declared resource kinds only** (finite in every model state). There are no C arrays, C function calls or pointer expressions.
2. **Bindings.** C memory enters only through `bind` declarations on effects: an S3a abstract location `(struct type, field)` or a global field is bound to a component of a resource (`bind field CFE_ES_AppRecord_t.AppState as state(es.appstate(record))`). O7 checks every binding: each store to a bound location must lie at an effect LP. Oracles are **sets of C functions** whose read sets define support locations; they are never evaluated.
3. **AST-pattern language** with a match semantics:
   ```
   pattern := 'fn' '(' NAME ')' '.' op [ '#' ORDINAL ] [ 'macro' ('yes'|'no'|'any') ]
   op      := 'call' '(' NAME { ',' argpred } ')' | 'store' '(' TYPE '.' FIELD ')' | 'memset' '(' TYPE ')'
            | 'load' '(' TYPE '.' FIELD ')' | 'loop' '(' 'poll' ')' | 'return'
   argpred := 'arg' '[' INT ']' ( '==' CONST | '!=' CONST | 'is' 'addr' '(' TYPE '.' FIELD ')' | 'any' )
   ```
   The match set of a pattern is the set of AST nodes in the bodies of all definitions of function `NAME` in B's link set (same-named local copies distinguished by DWARF compile unit, §3.17) whose kind, callee or field match, whose constant-evaluated arguments satisfy the predicates, and whose macro-expansion provenance matches the flag; the ordinal selects the k-th match in source order. A pattern with exactly one match is bound; zero or several matches go to the anchor audit.

**One evaluator, three domains.** `libkz-core` implements one evaluator of the core expression language behind an abstract-domain interface with three instances: concrete (trace states), abstract (the step-6 domain of §4.2: constants, intervals, defined/undefined/reserved IDs, record-type tags; results in {true, false, maybe}) and operational (explorer states). A property test on small random models checks that the concrete and operational instances agree exactly and that the abstract result always contains the concrete one.

Excerpt (citations from §3):

```
profile cfs "cFS 088b2fa8 / cFE c5fb2b4d / OSAL d2d877a6 / PSP c4b3b0b6";

const CORE_MAX_STARTUP_MSEC = ref eval "CFE_PLATFORM_CORE_MAX_STARTUP_MSEC"
                                  in "cfe/modules/es/fsw/src/cfe_es_start.c";   # constant-evaluated in its TU
const PSP_RESTART_DELAY     = ref eval "CFE_PSP_RESTART_DELAY"
                                  in "psp/fsw/pc-linux/src/cfe_psp_support.c";

kind evs.reg(slot: Int)        { states UNREG < REG; initial UNREG; }
kind es.shutdown()             { states RUN < SHUTDOWN_REQ < TEARDOWN; initial RUN; scope process; }

op CFE_ES_GetAppID(out AppIdPtr) {
  bind symbol "CFE_ES_GetAppID";
  effect use es.self(exec) lp at fn("CFE_ES_GetAppID").call("CFE_ES_GetAppRecordByContext");  # cfe_es_api.c:750
  effect store *AppIdPtr := id(owner(exec))        when result == CFE_SUCCESS;
  effect store *AppIdPtr := CFE_ES_APPID_UNDEFINED when result != CFE_SUCCESS;  # neutral for validity
  outcome CFE_ES_ERR_RESOURCEID_NOT_VALID : not_ready(es.self);
}

op CFE_EVS_SendEventWithAppID(EventID, EventType, stamp AppID, Spec) {
  bind symbol "CFE_EVS_SendEventWithAppID";
  conjunct valid(arg.AppID)              lp capture;                             # T-DERIVE (§1.4)
  conjunct state(evs.reg(arg.AppID)) = REG lp at fn("CFE_EVS_SendEventWithAppID").call("EVS_GetAppDataByID");
  outcome CFE_EVS_APP_ILLEGAL_APP_ID : not_ready(valid);                         # cfe_evs.c:185-189
  outcome CFE_EVS_APP_NOT_REGISTERED : not_ready(evs.reg);                       # cfe_evs.c:190-194
}

op CFE_PSP_Restart(resetType) {
  bind symbol "CFE_PSP_Restart";
  effect lifecycle es.shutdown() -> SHUTDOWN_REQ lp at fn("CFE_PSP_Restart").store(CFE_PSP_IdleTaskState_t.ShutdownReq);
  terminal after call("OS_TaskDelay") then call("abort");                         # cfe_psp_support.c:87-107
}

rule CORE-SERIAL stratum S robust obs_fatal guard cfe >= "6.4.2" {
  observe loop poll in fn("CFE_ES_MainTaskSyncDelay")
          called at fn("CFE_ES_CreateObjects").call("CFE_ES_MainTaskSyncDelay");  # cfe_es_start.c:835
  predicate from code;                        # extracted by kz-static skel, re-derived by kz proofcheck
  else terminal call("CFE_PSP_Panic");        # cfe_es_start.c:838-846
  source code "cfe/modules/es/fsw/src/cfe_es_start.c:776-918";
}

contract K-ID.send-with-appid {
  use op CFE_EVS_SendEventWithAppID;
  requires valid(arg.AppID) && state(evs.reg(arg.AppID)) = REG;
  oracle   fns { "EVS_GetAppDataByID", "EVS_AppDataIsMatch" };
  allowed  none;  harm event_lost_silently;
  source   api_doc "cfe/modules/core_api/fsw/inc/cfe_evs.h:168-172" sha256 <h>;
  authored 2026-10-12 by <author> inspected { R2, C3 };
}

contract K-SELF.es {                             # instantiated at every caller (§3.3)
  use op_group { CFE_ES_ExitApp, CFE_ES_RunLoop, CFE_ES_WaitForSystemState, CFE_ES_GetAppID,
                 CFE_ES_GetTaskID, CFE_ES_CreateChildTask, CFE_ES_ExitChildTask };
  requires state(es.self(exec)) = LIVE;
  oracle   fns { "CFE_ES_GetAppRecordByContext", "CFE_ES_GetTaskRecordByContext" };
  allowed  per_op;  harm per_op;                  # per-op outcomes and harms in §3.3
  source   code_pattern "cfe/modules/es/fsw/src/cfe_es_api.c:312,463,533,750,785,1262,1459" sha256 <h>;
}

contract K-REG.osal-countsem-lookup {
  use op OS_CountSemGetIdByName;
  requires state(osal.obj<countsem>(arg.name)) = LIVE;
  allowed  retry_until_ready({OS_ERR_NAME_NOT_FOUND}, bound = loop_bound(caller));
  harm     app_error_exit;
  source   api_doc "osal/src/os/inc/osapi-countsem.h:<retvals>" sha256 <h>,
           code_admission "apps/cf/fsw/src/cf_cfdp.c:1272-1278" sha256 <h>;
}
```

### 2.4 Data flow

Numbered flow [D] (diagram D2 in §12):

1. `kz-static basis` records B. `extract` and `link` produce facts (incl. `derives`, writer sites, loop-exit expressions), the configuration-resolved call graph and the gcc anchor table, and run the anchor audit.
2. `skel` builds the certificate view and the exploration view per static actor, computes once facts and message provenance, and extracts await predicates.
3. Writer sets for every Φ conjunct are assembled from the S3a writer sites (and S3b if built), filtered transition-aware, keyed, and checked against Φ (O7).
4. `static` computes MustBefore per M, instantiates every contract (census), issues certificates and verdicts, lists open candidates. `kz proofcheck` re-validates every certificate.
5. `plan` selects probes: the mandatory synchronization skeleton plus the obligations of open candidates, and allocates debug registers.
6. `kz observe` runs the unmodified binary under B and writes a trace; a normalization boot follows any abnormal exit (§4.13).
7. `kz trace` joins identities, builds `HB_π`, evaluates each conjunct at its LP, and routes each use: conforming; wrong-way (asks `static` for a certificate); O-ordered or unordered (to prediction).
8. `kz conform` checks O1/O2 on every trace (O8 from S5). O3–O5 and O9 run in `kz trace`. Model gaps and new keys go back to steps 1–3 (feedback F1/F2).
9. `kz predict` searches Pred(π) for reversals. `kz explore` (S7, gated) classifies candidates on closed models. O6 compares the two.
10. Witnesses become hold plans. `kz hold` (F0), or `kz gate` (F1, if approved), runs them. Confirmation traces re-enter step 7 (feedback F3).
11. Inconsistencies are reported against Φ, the extractor or the model, as findings about the tool (F4).
12. `kz loop` stops at a fixed point (§4.14) and records its run set. `kz report` renders the verdicts.

### 2.5 Consistency obligations between layers

| ID | Obligation | Checked | Failure means | Action |
|---|---|---|---|---|
| O1 | Every decoded event concretizes some template of its actor's skeleton (certificate view) | every trace | closure or extraction gap | `unknown(model_gap)` for instances touching the actor; feedback F2 |
| O2 | Each actor's observed event sequence is accepted by its certificate-view automaton | every trace | path abstraction or loop summary wrong | as O1; model evidence of instances touching the actor dropped |
| O3 | No observed order contradicts a MustBefore fact, a once fact or a certificate step on the observed path | every trace | an S-rule, writer set, binding or once proof is invalid | the rule is suspended; certificates using it withdrawn; `unknown(model_fault)`; **the loop ends for this Φ version** |
| O4 | (a) The program's outcome code at each use agrees with the state computed at each conjunct's LP. **Its checking power is reported only over uses whose LP order HB decides**; uses whose order was decided from the outcome code are not counted as checks (fixes [X1-m4]). (b) No bad occurrence at a certified-protected instance; no good occurrence at a certified-fixed-order instance | every trace | rule, effect, LP, contract or binding error — **or**, if the contradicted certificate holds at M_api only, a portability hazard of the real backend (fixes [X1-C2]) | (a) and (b) at M_impl: as O3. (b) at M_api only: `portability-hazard-evidence` record; no suspension |
| O5 | Every observed resource key is in the static key universe; the startup-script path opened equals the one `basis` selected; every observed actor is in the closure | every trace | closure incomplete | feedback F1 |
| O6 | A predicted witness is admitted by the explorer's model; an explorer witness that **keeps every observed outcome class in the cone** is admitted by Pred(π) (fixes [X1-m4]) | when both exist | model too strict or predictor too loose | `unknown(model_gap)` for the pair; report |
| O7 | The writer sites found by S3a (and S3b) for each support location equal the LP stores of Φ effects and bindings (no undeclared writer; no declared effect without a store); every `char*`, `void*` or cast write that may reach a support type is listed (A-TYPE) | every model build | Φ effect missing or wrong; untyped write | the conjunct's writer set is `DISABLED` (`unknown(writer_incomplete)`); disablement counts reported |
| O8 | Global conformance: some model execution's projection equals the observed per-actor sequences with their outcomes ([A §4.8]), within a work-unit cap (deviation), on a pre-registered sample | from S5 (trace-constrained search on S5's engine) | cross-actor inconsistency | `unknown(conformance)` for verdicts whose slice touches the region; cap exhaustion → "conformance undecided" |
| O9 | Allocation and ownership facts: distinct reservation nodes allocate distinct records (observed `PendingAppId` values and slots differ: A-FRESH); the observed task → app identity joins equal the static OWNER-MAP (fixes [X1-M3]) | every trace | a `DECLARED_TRUSTED` allocation or ownership entry is wrong | as O3 for every certificate that lists the entry |
| CC | Every certificate passes `kz proofcheck` | every census | certificate wrong | rejected; the instance is open |

Conformance is necessary evidence, not sufficient: an over-approximate model (star closure, havoc) accepts more traces ([J1 X8]).

### 2.6 Technology decisions

- **Clang/LLVM for analysis only** [D] (ADR-023). Clang provides macro provenance, constant evaluation, source locations, and the explicit field identity S3a needs. Whole-link-unit IR gives the call graph with archive semantics. DWARF and MC bind every fact to the gcc binary that runs. Clang never builds the executed binary.
- **gcc is the executed compiler.** gcc 13.3.0, `-O0 -g` ([R06 K1]); a recompiled object is byte-identical to the build tree's ([R07 §3.5]).
- **MLIR is optional, behind gate G-MLIR** [D] (ADR-018); the contribution does not depend on it.
- **SMT solver optional** [D] (ADR-014). An SMT-LIB2 emitter exists for cross-checks after PI decision P-Z3.
- **eBPF userland** [D] (ADR-013, ADR-038). Perf-PMU uprobes with attach cookies and a raw `bpf()` loader; hand-written helper prototypes; ELF parsed with glibc `<elf.h>`. A perf-sample backend is the zero-install bring-up path; tracefs dynamic events are optional and, if used, follow §5.4.
- **No Python, no JSON in the tool.** All components are C/C++. The existing harness (`env/run_cfs.sh`) remains the run driver; Kairos adds one root-side observer hook (PI decision D1, [R07 §9 item 1]).

---

## 3. cFS mapping: the profile Φ_cFS for v7.0.1

Each mechanism becomes Φ entries: resource kinds, operations (binding, effects, per-conjunct LPs, outcomes), HB rules (stratum, robustness tag) and contracts. Rule IDs R1–R19 are those of [R02 §8.1]; P-* are the OSAL primitive rules of [R01 §3, §6.1]. **Edges for shapes are API-level only**; OSAL-internal locks constrain interleavings only under M_impl (§1.4).

### 3.1 Actor inventory and closure

| Actor class | Source in B | Entry resolution | Observation anchor |
|---|---|---|---|
| ES startup thread (PSP main) | `bsp_start.c` → `CFE_PSP_MAIN_FUNCTION` ([R01 §5.1]); after `CFE_ES_Main` returns it runs `OS_Application_Run` (`cfe_psp_start.c:452–508`) | static; once by definition | `CFE_ES_Main` entry |
| Core main tasks ES, EVS, SB, TBL, TIME | generated `cfe_static_module_list.c` ([R02 §1.3]); the table loop in `CFE_ES_CreateObjects` is unrolled exactly (constant trip count) | `TaskMain` field of the constant object table; one spawn node per unrolled iteration | `OS_TaskCreate` in `CFE_ES_StartAppTask`; child `OS_TaskEntryPoint` (exported, `osapi-task.c:124`) |
| App main tasks (15) | the startup script **selected by B** (§1.7) | entry symbol → definition in the module's `.so` link unit (static analogue of `dlsym`, [R05 §2.5 item 1]); confirmed by the observed `OS_ModuleSymbolLookup` (O5); one spawn node per unrolled script line | as above |
| Child tasks | `CFE_ES_CreateChildTask` sites with constant function arguments (`cfe_es_api.c:1202–1294`); e.g. TIME's tone and local tasks (`cfe_time_task.c:160, 173`), `ES_BG_TASK` (`cfe_es_backgroundtask.c:214`), FM's child (`fm_child.c:86`) (re-read) | constant argument; owner = creating app | as above |
| Timebase `cFS-Master` | PSP `soft_timebase` module (`cfe_psp_soft_timebase.c:53`) | OSAL timebase | `OS_TimeBase_CallbackThread` (`osapi-timebase.c:365`) |
| OSAL-internal threads with a self-lookup handshake | generic rule: an OSAL-internal thread entry that calls `OS_ObjectIdGetById` on its own object; found by pattern in the closure (fixes [X2-03]) | static | entry |
| Callback contexts | `OS_TimerAdd`/`OS_TimerSet`, `CFE_TIME_RegisterSynchCallback`, FS background requests, TBL validation functions | **asynchronous actors whose spawn sites are their registration calls** (T-SPAWN gives registration → callback); never once; target resolution by registration sites ([R05 §2.5 item 3]), then type layers (MLTA [V-abs]), else `unknown_effect` | on-behalf frames from user stacks |
| Console writer, signal contexts | OSAL console create; PSP exception handlers ([R01 §2.6, §5.4]) | static; signal contexts have no in-model spawn site: `MB(entry) = ∅` | creation sites |
| Environment roots | B.Input | — (`MB(entry) = ∅`) | CI_LAB receive returns |

The 110 indirect call sites of the FSW set ([R07 §4.2]) are resolved in this order: configuration (task entries, trampoline `RealEntryFunc`, core `TaskMain`/`EarlyInit`, `CFE_Config_IterateAll` callbacks with the iteration count from the generated version table, SBN module ops via configuration table → module → symbol); then registration sites; then struct-layer types; then `unknown_effect`. The closure report gives the count resolved by each rule.

**Generations.** In the default B, every static actor is checked for once(τ) (§1.2). In `B_restart` the skeleton of the restarted app (and its children) is duplicated for generation 1, whose spawn site is the restart path of the background scan (§3.8).

### 3.2 OSAL ID map lifecycle and lookups

- **Kind** `osal.obj<T>` with the automaton of §1.3. The publication LP is the final ID write under the type lock (`osapi-idmap.c:1092`, unlock `:1105`), not the `OS_EVENT_RESOURCE_CREATED` callback (`:858–865`), which runs after the unlock ([R01 §6.1]).
- **M_api**: create is one transition `ABSENT → LIVE`; by-name lookup returns FOUND or `NAME_NOT_FOUND`.
- **M_impl** ([R01 §1.5–1.8]; fixes [X2-03]): ALLOC under the type lock → `ConvertToken(EXCLUSIVE)` sets RESERVED and **releases** the lock → `OS_OBJECT_INIT` makes the name visible → implementation call (`pthread_create`, `mq_open`, …) → `FinalizeNew` relocks and publishes. Token conversion is split by token kind:
  - **by-name and search tokens** copy `active_id = RESERVED` and fail at once with `OS_ERR_INCORRECT_OBJ_STATE`, returning before the unlock logic (`osapi-idmap.c:401–404`), while the header says "Upon failure, the global table lock is always released for all lock modes other than OS_LOCK_MODE_NONE" (`:384–385`) ([J1 §1 check 5]). Contract **K-LOCKBAL** (the API returns with the type lock released) is derived from that documented invariant;
  - **by-ID tokens** (GLOBAL/REFCOUNT/EXCLUSIVE through `OS_ObjectIdGetById`) on a RESERVED record enter the bounded wait: attempt `k = 1, 2, …` is `await(state ≠ RESERVED ∨ refcount admits; wake ∈ {broadcast(type unlock), timeout_k})`, with `timeout_k = k² × 10 ms` for `k ≤ 10` (`pthread_cond_timedwait`, `posix/src/os-impl-idmap.c:138–170`, re-read); every type unlock broadcasts (`:106–129`); the 5th attempt fails with `OS_ERR_OBJECT_IN_USE` (`osapi-idmap.c:475–480`, re-read). The timed wake is a **timeout transition** (enabled at any point in σ_U, disabled in σ_U∞ by the definition of σ_U∞; no deviation); broadcast wakes remain in every σ. [I] In σ_U, four timed wakes take 10 + 40 + 90 + 160 = 300 ms in σ_E terms, so F7 needs only a creator delay of that order.
- **Observation**: entry of each `OS_*Create`, `OS_*GetIdByName`, `OS_*Delete`; entry of `OS_ObjectIdFinalizeNew` (publication interval); return values at contract-use sites only (§5.2).
- **Outcome-implied reads-from (O)**: a successful lookup reads from the publication of the same generation. Timestamps are not used to order them ([A §5.6]).
- **Contracts**: `K-REG.osal-lookup` (Req LIVE; Allowed per caller: `retry_until_ready` with the caller's loop bound, else `returned_and_tested`); `K-LOCKBAL` (M_impl, by-name/search path); `K-REG.unique` (duplicate names under concurrent creation, [R01 §1.8 F2]); `K-LIFE.osal` (NONE-mode use concurrent with deletion, F4). rwlock and condvar are unmodelled: no non-test caller ([R01 §0 item 6]).

### 3.3 Task create, start, delete; ES handshakes; the App-record windows; K-SELF.es

- **P-TASK / R2 (S, unconditional)**: the creator's events before the `OS_TaskCreate` call → the child's first event ([R01 §2.2]).
- **K-SPAWN.osal (F7, M_impl)**: every thread that performs a by-ID self-lookup handshake on its own still-RESERVED record is an instance (fixes [X2-03]):
  - the child's `OS_TaskPrepare` (task table): after four consumed attempts the entry never runs, although the creator got `OS_SUCCESS` ([R01 §2.2]). Harm `task_never_runs`;
  - the timebase callback thread (`osapi-timebase.c:383–390`, re-read): on failure it silently `return`s, so `cFS-Master` delivers no callbacks and the 1 Hz chain never starts. Harm `task_never_runs` (consequence: no TIME tone, no SCH_LAB wakeups);
  - any further OSAL-internal thread found by the pattern of §3.1.
- **ES task-record handshake (O, conditional; S for the guarded part)**: `CFE_ES_StartAppTask` creates the task before taking the ES lock (`cfe_es_apps.c:664–672`) and fills the task record afterwards (`:694–703`). The child polls in `CFE_ES_GetTaskFunction`: a 50 ms delay, then a locked check of the task record, with a budget of `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` = 1000 ms, i.e. at most 21 checks (`cfe_es_apps.c:559–596`, re-read; [R05 §1 F4]). On timeout the entry is never called.
- **App-record windows.** `CFE_ES_GetAppRecordByContext` needs the caller's App record to be final; the trampoline checks only the task record. Two creation paths have the same structure ([LED §3.4]):
  - **Script apps**: publication at `cfe_es_apps.c:869` under the ES lock (`:861–869`), after the 50 ms-separated poll can already have succeeded ([R02 §1.6]). During the window `CFE_ES_GetAppIDByName` returns `CFE_SUCCESS` with `0xFFFFFFFF` (`cfe_es_api.c:637–656`; [J1 §1 check 4]); the contract's oracle (`CFE_ES_AppRecordIsMatch`) makes that success a not-ready outcome.
  - **Core apps**: reservation at `cfe_es_start.c:803`, `StartAppTask` at `:810`, publication at `:818` (re-read). A core task's `*_TaskInit` can call `GetAppID` before `:818`; EVS does so as its first action (`cfe_evs_task.c:273`, re-read).
  - **Child tasks are not immune** (corrects SPEC v1 §3.7; fixes [X2-02]): a child created inside its parent's window (TIME at `cfe_time_task.c:160, 173`; `ES_BG_TASK` at `cfe_es_backgroundtask.c:214`; FM at `fm_child.c:86`) resolves to the same not-yet-final App record.
  - **Both windows are O-ordered, not unordered**: the use's LP and the publication are both under the ES lock, so a trace orders them by a lock edge; deciding them needs lock reversal (§4.9) or the explorer (§4.10).
- **K-SELF.es** [D] (generic contract `es.self(exec) = LIVE`, fixes [X2-02]): instantiated at **every** caller of `GetAppRecordByContext` / `GetTaskRecordByContext`, each with its own documented not-ready behaviour (all re-read in `cfe_es_api.c`):

| Caller | Call site | Not-ready behaviour (outcome) | Allowed | Harm if bad |
|---|---|---|---|---|
| `CFE_ES_ExitApp` | `:312` | silent no-op: unlock and return (`:312–313, :417`) | `unspecified_by_reference` | chain (below) |
| `CFE_ES_RunLoop` | `:463` | returns `false` (`:494–500`) | `unspecified_by_reference` | chain (below) |
| `CFE_ES_WaitForSystemState` | `:533` | the raise is skipped; the wait proceeds (`:533–576`) | `unspecified_by_reference` | chain (below) |
| `CFE_ES_GetAppID` | `:750` | `CFE_ES_ERR_RESOURCEID_NOT_VALID`, out = `UNDEFINED` (`:757–761`) | per caller (status tested or ignored) | per caller |
| `CFE_ES_GetTaskID` | `:785` | error code | per caller | per caller (e.g. SB event drop, §3.9) |
| `CFE_ES_CreateChildTask` | `:1262` | error code; no child | per caller | `task_never_runs` |
| `CFE_ES_ExitChildTask` | `:1459` | error path (no record freed) | `unspecified_by_reference` | per chain |

- **Consequence chains, every branch** (all [I] from code until run; fixes [X2-02]):
  - *EVS inside the core window.* `GetAppID` fails (`cfe_evs_task.c:273`) → EVS init error → `CFE_ES_ExitApp(CORE_APP_INIT_ERROR)` (`:219–220`):
    - (a) window **still open** → `ExitApp` is a silent no-op, so EVS's "ExitApp will not return" (`cfe_evs_task.c:219–220`) is false here. EVS continues to `WaitForSystemState`: (a1) window closed by then → raise happens; loop condition false → `ExitApp(RUNTIME_ERROR)` → `OS_TaskExit` (`cfe_es_api.c:365–379`): the system runs without an EVS task (harm `service_task_absent`); (a2) window still open → raise skipped → the core sync waits `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` = 30000 ms → `CFE_PSP_Panic` (harm `panic`);
    - (b) window **closed** before the first `ExitApp` → `CFE_ES_ResetCFE(PROCESSOR)` → `CFE_PSP_Restart`: the EVS task (the requester) sleeps `CFE_PSP_RESTART_DELAY` = 10000 ms (`pc-linux/inc/cfe_psp_config.h:57`) and calls `abort()`. `SIGUSR1` is blocked on every thread and is consumed only by `OS_Application_Run`, which has not started (the startup thread is still in `CFE_ES_Main`), so no orderly shutdown happens. EVS never raises, so the core sync would panic at 30 s, but the abort at 10 s comes first (harm `process_abort`; `processor_reset_requested` is recorded in the boot record).
  - *Script app inside its window.* `RunLoop` returns false → the app leaves its loop → `ExitApp` is a no-op (window open) → the task returns → its record stays used with `AppState ≤ RUNNING`; the scan acts only on `> RUNNING` (`cfe_es_apps.c:1112–1146`) → harm `app_zombie`, and both soft barriers time out after `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` each. If the window closes before `ExitApp`, the app is STOPPED and cleaned up (harm `app_error_exit`).
  - *A core task's raise inside its window* (ES, SB, TBL, TIME): the raise is skipped → the core sync panics after 30 s, unless the task raises again later.
  - *SB or TBL `GetAppID` inside the window* (`cfe_sb_task.c:138`; `cfe_tbl_task.c:196`; status ignored): the identity cell stays `UNDEFINED` for the run; every later SB/TBL `SendEventWithAppID` use is bad with shape ABSENT, chained to this root (EventRacer-style race coverage reports the root, not the 211–229 dependents).
- **Delete/exit (P-TASK-JOIN / R17, S)**: `OS_TaskDelete` cancels and joins (`os-impl-tasks.c:693–723`); after it returns, the victim executes nothing more. Deferred cancellation leaves a program lock held if the victim holds it at a cancellation point ([R01 §2.4]) → **K-LOCK.cancel** on `CFE_ES_DeleteChildTask`, which deletes under the ES lock (`cfe_es_api.c:1348, 1383`).

### 3.4 OSAL primitives

| Primitive | Rule (stratum, tag) | Matching condition | Outcomes recorded | Probe anchors (entry+4 unless stated) |
|---|---|---|---|---|
| Mutex (`OS_MutSem*`, recursive, `os-impl-mutex.c:94`; `PTHREAD_PRIO_INHERIT`, `:82`) | P-MUT / R4 (O, schedule-chosen) | outermost release → next outermost acquire of the same id+gen; nested operations add nothing | result at contract-use sites only; recursion depth | `OS_MutSemTake` entry; `OS_MutSemGive` entry with **`objseq` assigned in kernel at give entry**, inside the critical section ([B §5.5]) |
| Binary semaphore | P-BIN / R9 (O) | give → the take that consumed it; conservatively the join of all gives since the last consuming take; flush → each flushed take; timeout → none | consumed vs flushed; timeout; `OS_SEM_FAILURE` from the internal 2 s `timedlock` (`os-impl-binsem.c:50, 60–77`) | give/flush entry; take return only where contract-relevant |
| Counting semaphore | P-CNT / R10 (O) | conservative join of prior gives ([R01 §3]); exact counts in prediction | result; EINTR failure (`os-impl-countsem.c:145–148`) | give entry; take as above |
| Queue (all 26 are SB pipes, [M] [BM §0]) | P-Q (O) | put(d) → get(d) for the same 8-byte descriptor value; FIFO per queue; `OS_QUEUE_FULL` → **drop outcome, no edge** | put outcome (delivered / full / error), get outcome | put entry and outcome; get outcome (§5.2) |
| Timebase and timer | P-TB (O); registration → callback (S, via T-SPAWN of the callback actor, §3.1); callback k → k+1 (S within the timebase thread) | `OS_TimerAdd`/`Set` (`handler_mutex`, `osapi-time.c:185, 345`) → later callbacks (`osapi-timebase.c:448`); callbacks attributed to the timer's creator; `OS_TaskGetId` returns the timebase ID inside callbacks (`osapi-timebase.c:383`) | callback begin/end | timer add/set; callback entry |
| Modules and symbols | P-MOD (O, reads-from) | `OS_ModuleLoad` success → successful symbol lookup **in the module's scope** (§3.7) | names, flags, results | load/lookup entry and outcome |
| Files, dirs, paths | none (shared state) | path as resource; `OS_FileOpenCheck` is check-then-act ([R01 §3]) | path, op, result | API entry and outcome |
| Sockets | P-NET (O) | datagram send → receive; external senders are `T_env` | payload identity | receive outcome |
| `OS_printf` | **none** | implementation-level serialization only ([R01 §3]) | — | not probed |
| `OS_TaskDelay` | **none** in HB; a yield in σ_U; a blocking delay with a discrete wake time in σ_R1 | — | delay calls (a timing-based protocol is a race signature) | entry |
| Condvar, rwlock | unmodelled | no non-test caller | any observed call → `unknown(unmodelled)` | — |

### 3.5 PSP startup, persistence, restart, shutdown and panic

- **Startup order (S, program order on the PSP main thread)** ([R01 §5.1]): `OS_API_Init` (console created) → event-handler registration → reserved memory → `OS_FileSysAddFixedMap` → `CFE_PSP_ModuleInit` (creates and starts `cFS-Master`) → `sleep(1)` → reset-type decision (`cfe_psp_start.c:411–434`) → `CFE_PSP_InitProcessorReservedMemory` → `CFE_ES_Main` → `OS_Application_Run`. Hence `cFS-Master` is LIVE before any cFE task exists.
- **PSP task-startup hook**: on `OS_EVENT_TASK_STARTUP` the PSP calls `OS_GetResourceName` (task-table GLOBAL lock; an attempt consumer for F7) and pins `CFE_*` tasks to CPU 0 (`cfe_psp_start.c:174, 184–189`; an Env fact to measure).
- **Panic is terminal**: `CFE_PSP_Panic` → `abort()` (`cfe_psp_support.c:116–121`); the ES callers first wait `OS_TaskDelay(CFE_ES_PANIC_DELAY)` = 500 ms (`cfe_es_start.c:55, 769, 844`). Other threads still run during the delay ([J1 §1 check 1]). T-TERM excludes only the panicking thread's terminal branch.
- **Restart and shutdown** [D] (fixes [X1-M6], [X2-04]); three effects, all re-read:
  1. **Requester** (any actor calling `CFE_PSP_Restart`, e.g. through `CFE_ES_ResetCFE`): if POWERON, delete the shm segments (`cfe_psp_support.c:62–69`); write the boot record's `NextResetType` (`:78–79`); store `ShutdownReq = true` (`:87`) — the **SHUTDOWN_REQ LP**; `pthread_kill(SIGUSR1)` to the PSP main thread (`:88`); `OS_TaskDelay(CFE_PSP_RESTART_DELAY)` = 10000 ms (`:99`); `abort()` (`:107`). The requester **never returns**: `CFE_PSP_Restart` is a terminal operation for its caller (T-TERM after the delay).
  2. **PSP main thread**, only after `CFE_ES_Main` returns: `OS_Application_Run` loops in `sigwait` until it observes `ShutdownReq` (`cfe_psp_start.c:484–499`), then `OS_TaskDelay(100)` and `OS_DeleteAllObjects` (`:505–507`) — the **TEARDOWN LP**. `SIGUSR1` is blocked process-wide (`os-impl-tasks.c:262–291`), so while the startup thread is inside `CFE_ES_Main`, a restart request causes no teardown: only the requester's abort after 10 s.
  3. **Next run**: `CFE_PSP_Restart(POWERON)` deletes the shm segments, so B.Init of the next run is clean; any other exit (abort, PROCESSOR restart, a harness kill) leaves them, and the next boot is a PROCESSOR reset (§1.7, §4.13).
- **Scope** (§1.6). The `ShutdownReq` writer set is the single store above (re-read: no other writer in `psp/fsw/pc-linux/src`). In the skeleton, `OS_DeleteAllObjects` is preceded by the observation of `ShutdownReq`; T-OBS gives `SHUTDOWN_REQ ∈ MB(OS_DeleteAllObjects)`. A disabler or writer `d` with `SHUTDOWN_REQ ∈ MB(d)` can only be followed by out-of-scope occurrences, so certificates exclude it (rule SCOPE, §4.5). Default-B certificates are therefore stated **outside teardown**; uses after the shutdown request form the family **K-TEARDOWN** (K-LIFE during deletion), reported separately. Because the experiments' stop method itself is an ES POWERON restart command (the CI step `build-run-app-reusable.yml` L209–214, E11(a) in [BM]), every observed run ends with a teardown; its uses are K-TEARDOWN instances.
- **Persistence (scope = persistent)**: boot record, CDS, reset area, user-reserved shm (keyed by `ftok` of CWD key files), `/dev/shm/osal:RAM`, `EEPROM.DAT` ([R01 §5.3]); snapshotted per run (S0) and canonicalized (§1.7).
- **Asynchronous PSP paths**: the exception ring (4-entry lock-free ring with an acknowledged race, `cfe_psp_exception.c:72–86`) → `SIGUSR1` → `SystemNotify` → ES. Fault scenarios outside the first scope; their kinds and edges are in Φ, so they are not silently unordered.

### 3.6 ES phases, soft barriers and core serialization

- **Core object order** comes from the generated `cfe_static_module_list.c` (CONFIG, ES, EVS, FS, SB, TBL, TIME, MSG, SBR, RESOURCEID), not from `cfe_es_objtab.c` ([R02 §1.3]). Changing `MISSION_CORE_MODULES` gives a new B.
- **EarlyInit** runs every `EarlyInit` in table order on the startup thread before the first core task is created (`cfe_es_start.c:752–773`); an EarlyInit failure panics. EarlyInit stores are ordinary writers (§1.7).
- **R19 CORE-SERIAL (S, obs-fatal)**: per core entry with a `TaskMain` (`cfe_es_start.c:776–847`, re-read):
  1. reserve under the lock (`:787–806`; `Type = CORE` at `:795`; `SetUsed(RESERVED)` at `:803`) — the falsifier `f` and the allocation node of record `ρ(f)` (§4.2);
  2. `StartAppTask` (`:810`);
  3. publish `SetUsed(AppRecPtr, PendingAppId)` (`:818`), or on failure `memset(AppRecPtr, 0, …)` (`:823`), which un-uses the record and is followed only by the panic branch;
  4. `MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` (`:835`) — the observation `o`;
  5. on any failure, `CFE_PSP_Panic` (`:838–846`).
- **The observation behind R19** (fixes [X1-M8]). `MainTaskSyncDelay` is recognized as a poll loop by pattern (delay plus exit test, §4.3). Its predicate is **extracted from the code**: the exit-by-condition branch tests `AppNotReadyCounter == 0` after a constant-trip loop over `AppTable[0..CFE_PLATFORM_ES_MAX_APPLICATIONS)` under the ES lock (`cfe_es_start.c:860–918`, re-read), giving `Pred_code = ∀ r ∈ AppTable: ¬(IsUsed(r) ∧ r.AppState < AppStateId)` with `AppStateId` constant-evaluated at the call site (RUNNING at `:835`). The observed conjunct for falsifier `f` is the instance at `ρ(f)`: `C = ¬(IsUsed(ρ(f)) ∧ ρ(f).AppState < RUNNING)`. Φ only binds the rule name, stratum and terminal escape to the loop.
- **Writer set of C** (all re-read; statuses in §4.2; worked out for the ES and EVS iterations in App. A.1):
  - raises: `CFE_ES_WaitForSystemState` (`cfe_es_api.c:573`, conditional on `GetAppRecordByContext() ≠ NULL`; level by caller type and requested state, `:536–563`); `CFE_ES_RunLoop` (`:471`); no core task calls `RunLoop` (comment at `cfe_es_task.c:143–145`; established over the call graph);
  - `ExitApp`'s STOPPED store (`:399`), external apps only (guard `Type != CORE` on the owner record);
  - the background scan's WAITING store (`cfe_es_apps.c:1144`), guarded by `AppState == RUNNING` on the same record: neutral by the transition filter;
  - record freeing in `CFE_ES_CleanUpApp`, reached from the scan only for external apps with `AppState > RUNNING` (`cfe_es_apps.c:1110–1113`): relational guard → `DECLARED_TRUSTED` (refutable by O3/O4);
  - **un-use stores on the startup thread**: `memset` at `cfe_es_start.c:823` (core failure path; no skeleton path to any `o`, only to the panic: removed by the own-actor no-path rule, §4.4; fixes [X2-01]) and `CFE_ES_AppRecordSetFree` at `cfe_es_apps.c:893` (script `AppCreate` failure path; it **has** a path to the script-phase observations, so it stays in their writer sets — one more reason script-phase barriers are O-edges);
  - the EarlyInit `memset(&CFE_ES_Global, 0, …)` (`cfe_es_start.c:82`): before every `f`, removed by once-PWB.
- **`CFE_ES_WaitForSystemState(s, T)`** is two effects ([R02 §1.7]): (1) a raise under the ES lock, level per the caller-type table (`cfe_es_api.c:536–563`): core apps → RUNNING for `s ≥ CORE_READY`; external apps → EARLY_INIT, LATE_INIT (APPS_INIT), RUNNING (OPERATIONAL), STOPPED (SHUTDOWN); (2) a conditional observation: the unlocked poll of `SystemState` (`:585`). Edge R5/R6 (O, obs-soft, assumed visibility): the startup thread's store `SystemState ≥ s` → the waiter's return **with `CFE_SUCCESS`**; a timed-out return gives no edge. `CFE_ES_WaitForStartupSync` returns void (`:616–619`).
- **Script-phase barriers** time out softly after `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` each and only log (`cfe_es_start.c:210–213, 225–228`; "soft limit", `cfe_es_internal_cfg.h:866–879`) → O, obs-soft. They test LATE_INIT (before APPS_INIT) and RUNNING (before OPERATIONAL).
- **After OPERATIONAL** every wait returns immediately: restarted, reloaded and commanded apps get no startup ordering ([R02 §1.7]).
- **K-PHASE as consumer-side instances, indexed by level** [D] (fixes [X1-m9], [X2-11]). Provenance: `cfe_es.h:400–403` ("This API assumes that the caller has also been initialized sufficiently to satisfy the global system state it is waiting for"). An instance is `(κ_phase, u, r)`: a consumer's use `u` of a resource `r` provided by app `a`, where `u` follows the consumer's successful `WaitForSystemState(s)`. The barrier passed is fixed by `s` (constant-evaluated): OPERATIONAL requires every app at RUNNING; APPS_INIT requires LATE_INIT. A **declaration** by `a` counts for that barrier only if its raised level is at least the barrier's level (raise level by caller type and `s`, or `RunLoop`'s RUNNING). `Req` is the ordinary readiness conjunct of `r` at `u`'s LP. The only S/O ordering between provision and use is the barrier chain: `a`'s declaration → `MainTaskSyncDelay` success → `SystemState` store → the consumer's wait success. If `a`'s provision follows its counting declaration in program order, the chain does not order provision before `u`, and the instance goes to prediction and exploration like any other; σ_U∞ separates timeout-dependent cases. In-tree deviations ([R03 §4 RC7]): SBN declares before its real init; CS declares even after init failure; an app that exits on error is STOPPED and passes the barrier.

### 3.7 Startup-script apps, libraries, trampoline, child tasks and the loader

- **Script parsing** (`cfe_es_apps.c:115–332`): one line at a time, return status ignored (`:313`), no synchronization between entries ([R02 §1.5]). Which script is parsed is decided by B (§1.7).
- **R3 SCRIPT-SEQ (S)**: library init *k* runs synchronously on the startup thread while the Lib record is RESERVED (`:1013–1021`, published `:1037`) → any later entry. `AppCreate(k)` → `AppCreate(k+1)` orders **creation only**. Init completions interleave out of script order ([R03 §6.1]).
- **App creation** (`cfe_es_apps.c:733–902`): reserve (`:821`) → `CFE_ES_LoadModule` (`OS_ModuleLoad`/`dlopen`, `OS_ModuleSymbolLookup`/`dlsym`, `:499, :522`) → `StartAppTask` (`:848–854`) → publish (`:861–869`) or free on failure (`:893`).
- **Loader symbol scope** (fixes [X2-10]): ES loads **apps with LOCAL symbols** and **libraries with GLOBAL symbols** (`cfe_es_apps.c:470–491`; POSIX `RTLD_LOCAL`/`RTLD_GLOBAL`, `os-impl-posix-dl-loader.c:79–96`). Kind `loader.sym(name, scope)`: the provider effect is `OS_ModuleLoad` with its flags constant-evaluated; a **global** lookup (`OS_SymbolLookup`, used by MD and MM, `md_utils.c:214`, `mm_cmds.c:143`, `mm_utils.c:525`) sees only GLOBAL-scope symbols (core executable and libraries); a module-specific lookup (`OS_ModuleSymbolLookup`) sees its module. An app symbol is therefore never visible to a global lookup: such instances have provider `none` in every B whose script loads that module as an app.
- **Trampoline**: `CFE_ES_TaskEntryPoint` (`:613–641`) → `CFE_ES_GetTaskFunction` (§3.3) → the real entry through a stored function pointer (`:581, :639`).
- **Child tasks** (`cfe_es_api.c:1202–1294`): main-task-only check (`:1268`); the same `StartAppTask`; `ExitChildTask` frees its record and calls `OS_TaskExit` (`:1453–1478`). **Children created during the parent's App-record window see the window** (§3.3).

### 3.8 Restart, reload, delete and teardown

- **R15 CTRL-REQ (O)**: a request is written under the ES lock (`cfe_es_api.c:184, 239, 278`) → the app's `RunLoop` read (`:478`) → `ExitApp` → the scan sees STOPPED. The kill path is also reached by the scan timer without app action: RUNNING + request → WAITING with a kill timer of `CFE_PLATFORM_ES_APP_KILL_TIMEOUT × CFE_PLATFORM_ES_APP_SCAN_RATE` = 5 × 1000 ms (`cfe_es_apps.c:1139–1146`; `cfe_es_internal_cfg.h:198, 228`) ([R02 §1.11]).
- **`CFE_ES_CleanUpApp`** (`:1407–1640`): marks the task and App records RESERVED; runs the `Cleanup` hooks of the core modules **in the generated core-object-table order** of modules that define one (`cfe_es_apps.c:1525–1545`, re-read; the order comes from B, not a hard-coded EVS → SB → TBL → TIME), without the ES lock; deletes tasks (children first, `:1561–1574`); deletes pools; unloads the module; frees the records. **The app's tasks keep running until after the module cleanups** (fixes [X2-14](d)): this interval is the K-LIFE window.
- **Contracts**: `K-LIFE` (use after retire: `ReceiveBuffer` on a deleted pipe; `GetAppID` with a RESERVED task record; a TIME synch callback after its module is unloaded, [R02 §5.3]; uses inside the K-LIFE window above); `K-GEN` (a peer keeps the old generation's ID or address: CF's semaphore ID, MD's resolved address, SBN module symbols; [R03 §4 RC15]); `K-PERSIST` across generations (§3.12).
- **Scenario family** `B_restart(app, request)`: B.Input contains one documented control request for one app, triggered by an event (OPERATIONAL observed). One request is the minimal case that exercises the path; the restarted app's skeleton is duplicated for generation 1 (§3.1). Longer sequences are `unspecified_by_reference`: no length is chosen; if the PI wants them, the length is a recorded deviation.
- **Scenario family** `B_stop`: the stop command and the teardown it triggers (K-TEARDOWN, §3.5).

### 3.9 Software Bus

| Mechanism | Φ entry | Source |
|---|---|---|
| Pipe creation | `sb.pipe`: reserve (`cfe_sb_api.c:153`) → OSAL queue created and named outside the lock (`:163`, queue-visible LP) → commit (`:185–203`, publish LP). `CFE_SB_GetPipeIdByName` finds the OSAL queue before the descriptor commits and fails with an error event (`:802–839`) → `K-REG.sb-pipe-name` | [R02 §1.16, §2.2] |
| Pipe ownership | owner = the AppId from `GetAppID`, status ignored (`:120`); checked on subscribe and delete, **not** on receive → K-ID instance on the stored owner (via a capture point) | [R02 §2.2, §2.5] |
| **Transmit, two phases** (fixes [X2-06]) | **(1) route_lookup LP** under the SB lock in `CFE_SB_TransmitTxn_FindDestinations` (`cfe_sb_priv.c:989–1104`): `CFE_ES_GetAppID` in the caller's context (`:1004`; a K-SELF.es instance); the route snapshot; per destination the **MsgLim check** (`BuffCount >= MsgId2PipeLim` → `MsgLimitErrorCounter++`, `:1058–1064`) and the **optimistic accounting** of `BuffCount`, `CurrentQueueDepth` and `PeakQueueDepth` (`:1071–1073`); no route → `NoSubscribersCounter++` (`:1085`), `SEND_NO_SUBS` event, `CFE_SUCCESS` returned. **(2) one put LP per destination**, outside the lock: `OS_QueuePut` (`:1178`); on failure the accounting is undone (`:1184–1216`): queue full → `PipeOverflowErrorCounter++` (`:1193`); other error (e.g. pipe deleted between snapshot and put) → `InternalErrorCounter++` (`:1199`) (all re-read) | [R02 §2.4]; [R03 §1] |
| R7 SB-DELIVER (O) | the put that enqueued descriptor d on pipe p → the `ReceiveBuffer` on p that dequeued d; FIFO per pipe; no edge for dropped messages | [R02 §2.6] |
| **Message key** (fixes [X2-09]) | `CFE_SB_TransmitMsg` takes no MsgId argument. Φ declares an abstract location **`msg.hdr.MsgId`** of message buffers, with encode/decode summaries for `CFE_MSG_Init`, `CFE_MSG_SetMsgId`, `CFE_MSG_GetMsgId` (and the function code by `CFE_MSG_SetFcnCode`/`GetFcnCode`) as implemented by the mission MSG module in B. A field-sensitive value analysis over message buffers (AST level, the S3a machinery) carries the constant from the init site (e.g. ES HK init `cfe_es_task.c:346–362`) to the transmit site; a **table-packet decoder** decodes command packets stored in tables (SC RTS/ATS, HS MAT, LC, SCH_LAB schedule) using the table type's DWARF layout. The census reports the shares of transmit sites with concrete, symbolic and ⊤ keys | §4.6 |
| Subscribe-before-send | `sb.route(m,p)`; R8 is **state**, not an edge: the route exists or not at route_lookup; destinations are snapshotted; no late-join delivery | [R02 §2.7] |
| **Message provenance** (fixes [X1-M7](a)) | for each pipe p, `Prov(p) = (Routes_B(p) ∩ Tx_B) ∪ Input_B(p)`, computed as a least fixpoint over handler reachability (§4.3); dispatch branches on `(MsgId, function code)` outside `Prov(p)` are folded away in both skeleton views | §4.3 |
| Looper rule | handler-scoped program order; no between-handler HB edge; FIFO constraint in prediction (§1.9) (CAFA [V-abs]; DCatch [P]) | [R04 §2.7, §4 row 8] |
| Buffer lifetime | valid until the next `ReceiveBuffer` on the same pipe or pipe deletion (`cfe_sb.h:449–464`; `cfe_sb_priv.c:1374–1379`) → `K-LIFE.sb-buffer` | [R02 §2.5, §7.4] |
| Events on behalf of callers | SB API code emits events with `CFE_SB_Global.AppId` as `stamp` (a capture point by `derives`), in the caller's task (`cfe_sb_priv.c:786`) → K-ID instances. **Documented drops** (`documented_drop`): the caller has no ES task record (`CFE_ES_TaskID_ToIndex` fails) or its per-task recursion bit is set (`cfe_sb_priv.c:305–328, 773–790`, re-read) (fixes [X2-14](a)) | [R02 §2.8] |
| **Subscription reporting mode** (fixes [X2-05]) | SBN only **transmits** `ENABLE_SUB_REPORTING` and `SEND_PREV_SUBS` commands (`sbn_subs.c:43–60`). SB's **own main task** executes the mode change when it dequeues the first (`CFE_SB_EnableSubReportingCmd` `cfe_sb_task.c:294–298` → `CFE_SB_SetSubscriptionReporting` `:1224–1227`: the LP of `lifecycle(sb.mode.subreport, ON)`), and runs the replay `CFE_SB_SendPrevSubsCmd` (`:1169`; dispatch `cfe_sb_dispatch.c:86–89`) as a **transmit loop in SB's actor**. Subscriber tasks read the flag **after the SB unlock, without a lock** (`cfe_sb_task.c:659` in `CFE_SB_SendSubscriptionReport`, called at `cfe_sb_api.c:1192–1195`): a `use` read tagged `assumed` (A4, §3.14). K-MODE is chained through R7 on **SB's command pipe**, which has its own `cap_B` and MsgLim instances (K-CAP, K-CAP.msglim) | [R03 §2.15, §4 RC13] |
| **Capacity, two levels** | `sb.queue(p)` with `cap_B(p)` (euid-dependent, §1.3) → `K-CAP`; `sb.msglim(m,p)` with the limit constant-evaluated from each `Subscribe`/`SubscribeEx` site (`MsgLim` argument, default `CFE_PLATFORM_SB_DEFAULT_MSG_LIMIT` = 4) → `K-CAP.msglim`. Tier-1 upper bound by maximum-weight closure over the certificate-view MB order of puts and gets (standard algorithmic fact, citation not verified in AR1); a put inside a star-closed loop has unbounded weight, so no protected certificate exists for it | [BM §3.1]; [R03 §4 RC14] |

Route and capacity contracts have `Allowed = unspecified_by_reference` unless a consumer's documentation requires delivery (TO_LAB's documented deferral is `defer_by_design`). Their bad executions carry **LOSS-UNDER-ORDER**, harm `unspecified_by_reference`. **Oracles** ([J2-12]; fixes [X2-06]): `NoSubscribersCounter`, `PipeOverflowErrorCounter`, `MsgLimitErrorCounter`, `InternalErrorCounter`, read from memory at stop by the [BM §2.3] method; per-put outcomes from the trace. `PeakQueueDepth` is labelled **SB-accounted depth** (it counts messages before their put succeeds and can exceed the real `mq` capacity); the real depth is reconstructed from put/get outcomes.

### 3.10 Event Services

- `CFE_EVS_Register` first **memsets the slot** (`cfe_evs.c:56`, re-read) — a D entry for `evs.reg(slot)` — and publishes REG last, without a lock (`cfe_evs.c:42–111`, LP `:106`). "One writer per slot" holds only without restarts; in `B_restart` the memset is a disabler of the previous generation's registration (fixes [X2-14](b)).
- `CFE_EVS_SendEvent` resolves the caller's AppId (a K-SELF.es use). `CFE_EVS_SendEventWithAppID` trusts the passed AppId; its two conjuncts and outcomes are in §1.4 ([R02 §3.2]).
- EVS's own identity is published last (`cfe_evs_task.c:311–312`, re-read) and guarded at its use (`cfe_evs_utils.c:613–623`). A guarded early use is `documented_drop` → refinement `tolerated` ([R02 §3.4]).
- The unlocked filter counters and `EVS_NotRegistered` updates are class M ([R02 §3.3]).

### 3.11 Table Services

| Mechanism | Φ entry | Source |
|---|---|---|
| Register | owner context required (K-SELF.es); under the registry lock; `OwnerAppId` set last (`cfe_tbl_api.c:145–149`) → REGISTERED (LP: that store). Critical tables: `CFE_ES_RegisterCDSEx`; `ALREADY_EXISTS` → restore from CDS (`cfe_tbl_api.c:117–131`) | [R02 §4.2] |
| Share | by full name under the registry lock; not found → `CFE_TBL_ERR_INVALID_NAME` (`cfe_tbl_api.c:212–229`) → `K-TBL.share` | [R02 §4.3] |
| GetAddress / ReleaseAddress | not owned → `ERR_UNREGISTERED`; never loaded → `ERR_NEVER_LOADED` (`cfe_tbl_registry.c:182–196`) → `K-TBL.getaddr`; R11 lock-flag protocol (O) | [R02 §4.5] |
| Load paths | owner `CFE_TBL_Load` releases the registry lock before the check-then-claim of `LoadInProgress` (`cfe_tbl_api.c:305–309`); the ground `LoadCmd` does it under the lock (`cfe_tbl_task_cmds.c:423–431`) → `K-ATOM.tbl-load`; **inactive in the default B by message provenance** (no in-B transmitter of the load command) | [R02 §4.4] |
| Validate / activate | R12 (O): request under the lock → the owner's `Manage` that sees it; R13: result flags without a lock → assumed visibility (§3.14) | [R02 §4.4] |
| Critical tables | registry lock → CDS mutex (lock-order fact for the deadlock check) | [R02 §4.7] |
| Cleanup | `CFE_TBL_CleanUpApp` from the ES background task | [R02 §4.9] |
| TBL identity | `CFE_ES_GetAppID(&CFE_TBL_Global.TableTaskAppId)` (`cfe_tbl_task.c:196`, status ignored) → `cell(CFE_TBL_Global.TableTaskAppId)`; 20 `CFE_EVS_SendEventWithAppID` sites in `tbl/fsw/src` ([R06 §5.5]) → K-ID instances with capture points | §0.5 |

### 3.12 CDS and generic counters

- `CFE_ES_RegisterCDS` needs a valid caller AppId (K-SELF.es) and holds the CDS mutex for the whole operation. A duplicate name is reused and reported as `CFE_ES_CDS_ALREADY_EXISTS`, the restore path (`es/cfe_es_cds.c:311–477`, `:461–472`; [R02 §1.13]). Kind `es.cds` is persistent; **K-PERSIST** checks restore-before-use and reset-type dependence over run sequences **and over generations within one run** (`B_restart`; `cfe_es_cds.c:342–348`) (fixes [X2-12]). INIT for CDS contents is the decoded S0 (§1.7).
- Generic counters are a name-addressed registry with no RESERVED step; `GetGenCount` reads without the lock ([R02 §1.14]) → `K-REG.es-counter`.

### 3.13 Time Services and FS background writes

- `CFE_TIME_TaskInit` registers with EVS, creates its semaphores and two children (inside TIME's App-record window, §3.3), subscribes, and adds a 1 Hz OSAL timer on `cFS-Master` ([R02 §5.1]).
- The 1 Hz pipeline is a chain of real edges: timer callback (timebase thread) → `OS_BinSemGive(LocalSemaphore)` → `Local1HzTask` → SB → TIME main → `Tone1HzISR` → `Tone1HzTask` → SB → TIME main ([R02 §5.1]). Its internal instances should classify protected (calibration).
- `ReferenceState[4]` with `LastVersionCounter` is a seqlock-like single-writer buffer with `volatile` only (`cfe_time_utils.c:538–656`). R14 (O, assumed visibility). Speculative reader loads are not reported (Boehm, MSPC 2012, as cited in [R02 §10]).
- **Synch callbacks** (fixes [X2-14](c)): `time.synchcb` is keyed by **app index, with no generation**; registration is an unlocked check-then-set (`cfe_time_api.c:628–640`, re-read); cleanup clears by index (`cfe_time_utils.c:1026–1034`) before the module is unloaded → `K-LIFE.callback` and `K-ATOM.synchcb` (two registrations for one index in `B_restart`) candidates ([R02 §5.3]). Callbacks run in TIME's context on behalf of the registering app (on-behalf frame).
- FS background requests: producer–consumer edges through the FS mutex (R16); requester callbacks execute in `ES_BG_TASK` (on-behalf frame) ([R02 §6.1]).

### 3.14 Ad hoc synchronization and the memory-model assumption

Poll and retry loops are **recognized by pattern** (a loop whose body contains a Φ delay op and whose exit edges test shared loads, Φ-op results, or a count/time budget; §4.3), never from a list (fixes [X1-M8]). The sites known from [R02 §8.3] and [R03], and their treatment:

| Site | Treatment |
|---|---|
| `WaitForSystemState` poll of `volatile sig_atomic_t SystemState` (`cfe_es_global.h:151`; `cfe_es_api.c:585`) | await with code-extracted predicate; O-edge on exit by condition; tagged `assumed` (A4) |
| `MainTaskSyncDelay` poll under the ES lock | await with code-extracted predicate; core phase: S, obs-fatal (terminal timeout); script phases: O, obs-soft |
| `GetTaskFunction` poll under the ES lock | await; O-edge on exit by condition; timeout → entry never called |
| CF semaphore retry (`cf_cfdp.c:1270–1301`) | delay-paced count-bounded retry, unrolled exactly to the effective bound; patient in σ_U∞; three outcomes: FOUND, other code → break, exhausted |
| OSAL by-ID RESERVED wait (M_impl) | attempts with broadcast or timed wake (§3.2) |
| SB `SubscriptionReporting` read after the SB unlock (`cfe_sb_task.c:659`) | `use` of `sb.mode.subreport`, tagged `assumed` (A4) (fixes [X2-05]) |
| TBL `ValidationResults.State` / `DumpCtrl.State` | O-edge, assumed visibility |
| TIME seqlock | R14, assumed visibility |
| FS `IsPending` pre-check | benign pre-check; no edge (the FS mutex carries the real edges) |
| ES `AppTimerMsec` countdown | time-based transition: nondeterministic in σ_U |
| SCH_LAB first tick ≥ 1 s after init | message edge only from the first 1 Hz packet; the margin ([M] 0.12–0.9 s in 13 boots, [R03 §6.3]) is σ_E evidence, never an edge |
| FM parent ↔ child queue (count semaphore, mutex and a count in `FM_AppData`, `fm_child.c:65–100, 162`) | app-private protocol with no framework rule → `unmodelled-sync` → `unknown(unmodelled)` for instances that depend on it, unless an app-profile entry with provenance is authored (§7.2) (fixes [X2-13]) |

Assumed-visibility edges are valid only under A4: sequential consistency at event granularity on x86-64 TSO at `-O0`. Kairos never reports these sites as defects; memory-model analysis is out of scope ([R06 T8]). Every verdict whose certificate uses such an edge carries `assumed-visibility`.

### 3.15 Contract library: coverage of the 15 readiness-contract kinds of [R03 §4]

"Full" means instances are generated, classified and observable in the default B or a declared variant. "Partial" states what is missing. The coverage was re-assessed after [X2 §C]: about 6 of 15 kinds were full as specified in v1; this table states what the fixes make of each, still **[I] until the census runs**.

| RC | Kind | Templates | Req | Allowed not-ready (source) | Decided by | Oracle | Coverage |
|---|---|---|---|---|---|---|---|
| RC1 | Named OSAL/PSP object lookup | K-REG.osal, K-REG.psp-dev, K-LOCKBAL | `osal.obj(name)` LIVE | per caller: `retry_until_ready` (CF, effective bound), else `returned_and_tested` | census → certificate / prediction (lock reversal) / explorer (M_impl, by-ID vs by-name, timed waits) → hold the provider | return codes; wait-for oracle for K-LOCKBAL | full, given §3.2 and teardown scoping; HS device case → provider `env-absent` |
| RC2 | Named ES lookup at steady state | K-REG.es (apps, libs, counters); **task-name lookups routed to K-REG.osal<task> and K-LOCKBAL** (`CFE_ES_GetTaskIDByName` → `OS_TaskGetIdByName`, `cfe_es_api.c:716–717`; HS XCT, `hs_cmds.c:148`) | `es.app/lib/counter(name)` LIVE by `IsMatch` (sentinel success is not-ready); `osal.obj<task>` LIVE | `returned_and_tested` where handled; HS AppMon counts not-found as "no progress" ([R03 §2.10]) → harm `unspecified_by_reference` | as RC1, plus restart scenarios | return codes; HS action events | full (order) after retyping; harm partial |
| RC3 | Named table sharing | K-TBL.share, K-TBL.getaddr | REGISTERED / LOADED | CS refreshes stale handles (`cs_compute.c:215–226`) | as RC1 | TBL return codes | full (latent by default table: CS) |
| RC4 | Symbol/module by name | K-REG.loader with scope | `loader.sym(name, scope)` DEFINED in the lookup's scope | SBN loads on demand (`sbn_app.c:1060–1088`) | census with constant-evaluated load flags; provider `none` for app symbols under global lookup (MD, MM); else as RC1 | return codes | full after the scope fix |
| RC5 | MsgId subscribe-before-first-send | K-ROUTE | `sb.route(m, p_consumer)` ROUTED at route_lookup | TO_LAB `defer_by_design` (`to_lab_app.c:64–66`); otherwise `unspecified_by_reference` → LOSS-UNDER-ORDER | census of subscribe/transmit pairs with header-carried and table-decoded MsgIds; certificates; prediction; explorer | per-destination puts; `NoSubscribersCounter` | **partial until the concrete/symbolic/⊤ key shares of transmit sites are measured** (§4.6) |
| RC6 | Identity/registration publication | K-ID (capture points), K-SELF.es, K-SPAWN | `cell(loc)` VALID; `evs.reg` REG; `es.self` LIVE | `fallback(CFE_ES_WriteToSysLog)` is a different API (`cfe_evs.h:168–172`) → `none`; EVS own-ID guard and SB recursion guard `documented_drop` | certificate (R2); prediction + explorer (windows); branch-complete chains | EVS and ES return codes | full, given App. A.1 (repaired certificate) and §3.3 |
| RC7 | Phase barrier / readiness declaration | K-PHASE consumer-side instances, level-indexed | `r`'s readiness at `u` | soft timeouts are documented ("soft limit") | barrier chain + prediction/explorer; σ_U∞ for timeout-dependence | ES app-state table | full after lifting and level indexing |
| RC8 | Persistent restore across reset | K-PERSIST over runs and generations | `es.cds`, critical tables RESTORED/REGISTERED before use | `ALREADY_EXISTS` → restore; defaults on failure | run sequences; `B_restart` | CDS status codes | **partial**: crash-point exploration (FCatch-style, ASPLOS 2018 [V-abs]) out of the first scope (D9) |
| RC9 | Parent ↔ child handoff | K-HANDOFF; class M | queue protocol (count, semaphore, mutex) | per app | HB where framework rules apply; FM's protocol → `unknown(unmodelled)` unless an app-profile entry exists | app counters | **partial** (app-private protocols, §3.14) |
| RC10 | Timer callback → task | K-HANDOFF.timer | semaphore LIVE before `OS_TimerSet` | — | certificate (creation order sem → timer add → set, `sch_lab_app.c:168, 184, 303`) with the callback actor's T-SPAWN at its registration | — | full |
| RC11 | Library before client | K-REG.lib | `es.lib` LIVE | — | certificate (R3); expected protected (calibration) | — | full |
| RC12 | File-object handoff (TOCTOU) | K-ATOM.file | check-then-act without interleaving | CF's TOCTOU comment (`cf_cfdp_s.c:447–463`) | serializability of the declared transaction (Velodrome PLDI 2008, AVIO ASPLOS 2006 [V-abs]) | return codes | **partial**: needs inputs; file-content semantics minimal |
| RC13 | Global SB mode change | K-MODE + K-ROUTE + K-CAP (SB command pipe) | `sb.mode.subreport` ON at the subscriber's unlocked read | `SEND_PREV_SUBS` replay in SB's actor | typestate mode via R7 on SB's command pipe + prediction | `SBNSubPipe` overflow; SB command-pipe counters | full after re-attribution to SB's actor |
| RC14 | Bounded-queue capacity under a burst | K-CAP, K-CAP.msglim | `depth(p) < cap_B(p)` at a put; `BuffCount < MsgLim` at route_lookup | `unspecified_by_reference` → LOSS-UNDER-ORDER | max-weight-closure bound (protected test) → prediction/explorer witness | put outcomes; `PipeOverflowErrorCounter`; `MsgLimitErrorCounter`; `InternalErrorCounter`; SB-accounted peak (labelled) | full (B-dependent) after the two-level model |
| RC15 | Stale reference after owner restart | K-GEN | generation g LIVE at use | CS/HS re-resolve (compensated) | restart family; generation-aware keys | return codes; STALE | **partial**: needs restart inputs in B′ |

Counts [I]: 10 kinds full after the fixes (RC1, RC2 with a partial harm element, RC3, RC4, RC6, RC7, RC10, RC11, RC13, RC14); 5 partial (RC5 until the key shares are measured, RC8, RC9, RC12, RC15). Templates beyond the 15 kinds, from [R01]/[R02] findings and the reviews: `K-SPAWN` (OSAL handshakes incl. the timebase thread; ES task record; ES App record, script and core paths), `K-SELF.es`, `K-LIFE`, `K-LOCKBAL`, `K-ATOM.tbl-load`, `K-ATOM.synchcb`, `K-LOCK.cancel`, `K-REG.unique`, `K-TEARDOWN`. Nothing is dropped silently.

### 3.16 HB rule catalogue (summary)

| ID | Rule | Stratum / tag | Matching / condition | Source |
|---|---|---|---|---|
| R1 | Program order | S / unconditional within an actor's init prefix and within one handler; **no edge between handlers on a looper** (FIFO constraint in prediction only) | same actor instance | [R02 §8.1]; CAFA, DCatch |
| R2 / P-TASK | Task create call → child's first event | S / unconditional | `OS_TaskCreate` call → `OS_TaskEntryPoint` | [R01 §2.2] |
| R3 | Startup-script sequencing | S / unconditional | lib init k → later entries; `AppCreate(k)` → `AppCreate(k+1)` (creation only) | [R02 §1.5] |
| R4 / P-MUT | Mutex outermost release → next acquire | O / schedule-chosen | same mutex, `objseq` | [R01 §3] |
| R5 | Polled-flag barrier | O / obs-soft (+assumed for `SystemState`) | write observed → exit by the code-extracted condition; timeout → none | [R02 §8.1] |
| R6 | App-state write → barrier release (composed) | O / obs-soft for script phases | raise → `MainTaskSyncDelay` success → `SystemState` store → waiter success | [R02 §8.1] |
| R7 | SB put → receive | O / schedule-chosen | same descriptor and pipe; delivered only | [R02 §2.6] |
| R8 | Subscribe → later routing | **state, not edge** | route present at route_lookup | [R02 §8.1] |
| R9 / P-BIN, R10 / P-CNT | Semaphore give → take | O | §3.4 | [R01 §3] |
| R11, R12, R13 | TBL lock flag; request → owner; result flags | O (R13 assumed) | §3.11 | [R02 §4] |
| R14 | TIME reference publication | O / assumed | validated reader | [R02 §5.2] |
| R15 | ES control request → exit/teardown | O | §3.8; the timer path gives no edge | [R02 §1.11] |
| R16 | FS request → background → completion | O | FS mutex | [R02 §6.1] |
| R17 / P-TASK-JOIN | Task delete/exit | S / unconditional | the deleter's return | [R01 §2.4] |
| R18 | EarlyInit → all tasks | S / unconditional | — | [R02 §1.3] |
| R19 | Core serialization | S / obs-fatal | T-OBS with complete writer set and code-extracted predicate + T-TERM (§3.6, §4.4) | [R02 §1.3]; `cfe_es_start.c:835, 838–846` |
| SR | Shutdown request → teardown | S / obs-fatal on the PSP main thread | `ShutdownReq` observed → `OS_DeleteAllObjects` | `cfe_psp_start.c:484–507` |
| P-TB | Timer add/set → callbacks | S via T-SPAWN of the callback actor; O between callbacks of different timers | `handler_mutex` | [R01 §3] |
| P-Q | Queue put → get | O | payload, FIFO; drops give no edge | [R01 §3] |
| P-MOD | Module load → symbol lookup | O (reads-from) | success outcome, same scope | [R01 §4] |
| RF | Outcome-implied reads-from | O | successful use ← the publication of the same generation | DefUse [V-abs] |
| — | Priorities, sleeps, script order for execution, timed-out barriers, `volatile` alone, use-site guards, console order, OSAL-internal locks (for shapes), **between-handler program order** | **none** | listed so they cannot be added by mistake | [R02 §8.2]; [R01 §3] |

### 3.17 Site binding and version guards

- **No line-number probes** [D] (ADR-019; [J2-9]). Each internal site is bound by an **AST pattern** in the language of §2.3. `kz-static link` resolves the pattern to gcc instructions through the DWARF line table and subprogram/inlined-subroutine entries of the basis's own binary, and disassembles with MC to obtain the instruction and its cost class.
- **Same-named local copies** of `static inline` helpers exist at `-O0` ([M] [X3, here]: two local `CFE_ES_AppRecordSetUsed` copies, four `CFE_ES_AppRecordIsMatch` copies in `core-cpu1`). They are bound by their DWARF compile unit, and entry+4 probes work on each copy (closes the v1 [U]; fixes [X3-m3]). Example: the core publication is `fn("CFE_ES_CreateObjects").call("CFE_ES_AppRecordSetUsed", arg[1] != CFE_RESOURCEID_RESERVED)`, bound to the copy in the compile unit of `cfe_es_start.c`.
- Sites with several matches or none are listed in the anchor audit ([R06 T7]). Instances depending on an unanchored site are `unknown(model_gap)` for observation; static certificates still apply.
- Every Φ rule and effect carries a **version guard** (e.g. R19 `cfe >= 6.4.2`, the two-phase barrier `cfe >= 6.6.0`, [LED §3.3]). A profile for another revision changes only Φ (hypothesis H-V, §7.6).

---

## 4. Algorithms

### 4.1 Configuration closure and fact extraction (`kz-static basis`, `extract`, `link`)

1. **Translation units.** Take the cpu1 compile DB (1417 entries) and keep the 448 flight entries. Target membership comes from `link.txt` (or the CMake File API), not from paths ([R07 §3.1]).
2. **Parse** with Clang LibTooling using the recorded commands, the two diagnostic-only adaptations (`-Wno-unknown-warning-option`, `-Wno-error`) and `-resource-dir` ([R07 §3.3]). The adaptation is recorded in B as an analysis-only transformation.
3. **Facts per call site**: callee; arguments as constant, symbolic (table field, global) or `⊤`; macro-expansion provenance; enclosing function; loop context; branches that test the call's result; intra-TU flag; whether the result is used or ignored; **`derives(arg, loc, site)`** for every argument with a stamp, key or id role whose value is a copy chain from one shared location (§1.4).
4. **Constants and tables.** Configuration macros are constant-evaluated **in the TU of their use** (effective values of wrappers such as `CF_INTERNAL_CFGVAL(...)`). Table initializers are constant-evaluated and cross-checked byte for byte against the built `.tbl` payload through the gcc DWARF layout ([A §4.1 step 4]); a mismatch is a basis error. On a PROCESSOR-reset basis, critical tables take their values from the decoded CDS image in S0 instead (A11, §6.1).
5. **Doc annotations.** Every `\par Assumptions` block and every `\retval`/`@retval` list of `cfe/modules/core_api/fsw/inc/*.h` and `osal/src/os/inc/*.h` becomes an annotation candidate with file:line and hash ([F] 138 assumption blocks and 429 `\retval` lines in cFE core API headers; 593 retval lines in OSAL, [J1 §1 check 7]).
6. **Link units and the app-unit probe** (fixes [X3-m5]). Emit bitcode and link each unit with archive semantics ([R07 §3.4]). Only `core-cpu1` is known to link closed ([R07 §3.4]); naive linking of the apps fails on `CFE_TBL_FileDef`. An early S3a probe therefore links **each app unit against the core as an external symbol provider**, models library symbol visibility in script order (libraries are loaded GLOBAL, apps LOCAL, §3.7), excludes table objects, and reports the undefined symbols per unit. The call graph is built from these links; indirect calls are resolved by the order of §3.1; residual sites become `unknown_effect`. Cross-check against gcc `-fcallgraph-info` ([R07 §4.4]).
7. **Site binding and anchor audit** (§3.17).
8. **Loop facts.** For every loop that contains a call to a Φ delay op, `extract` serializes the loop's exit conditions and the body's mod set as expression trees in `facts.kz`; `skel` extracts predicates from them (§4.3) and `kz proofcheck` re-derives them independently from the same serialized trees.
9. **Closure boot.** One F0 observation of the default B records every `OS_TaskCreate` (name, entry), child start, `OS_ModuleSymbolLookup` result and **the startup script actually opened**. They must equal the closure (O5); a mismatch is a closure bug, fixed before any verdict is reported ([A §2.5]).

Cost [M]: parse 10 s, bitcode 4.4 s, core link 0.35 s on this VM ([R07 §6]). The app-unit links are unmeasured (S3a).

### 4.2 Writer sets (S3a field-based extraction; S3b optional)

This is the soundness-critical analysis behind T-OBS and every certificate ([J1-2], [J2-6], [J3-M1]). It is built **at the Clang AST level**, where field identity is explicit; LLVM 23 IR has opaque pointers, field identity survives only in GEP source element types and is lost after canonicalizing passes, and `CFE_ES_AppId_t` is a plain `i32` there ([M] [X3, here]) (fixes [X3-M4]).

1. **Abstract locations (field-based).** One abstract location per `(struct type, field path)`, independent of the base object, plus one per non-struct global. Arrays of records collapse into their element type's fields. Record identity is handled separately by keys (step 8).
2. **Writers** of each abstract location, from the AST of every TU in B's link set:
   - assignments, compound assignments, increments and decrements whose lvalue is a `MemberExpr` naming the field (directly or through `->`, `.` and array subscripts);
   - `memset`, `memcpy`, `memmove`, `strncpy` (and builtins), which write every field of the destination's pointee type, recursively. A constant `memset(…, 0, …)` also records the written value 0 for the transition filter (e.g. `cfe_es_start.c:82`, `:823`, `cfe_sb_init.c:50`);
   - **out-pointer stores** through formal parameters, summarized per formal (`*p := value-expr under guard`) and instantiated at each call site whose actual is an address-of expression (`&CFE_SB_Global.AppId` at `cfe_sb_task.c:138` yields two writer entries on `(CFE_SB_Global_t, AppId)`: the success-branch store of a valid ID and the failure-branch store of `CFE_ES_APPID_UNDEFINED`, `cfe_es_api.c:752–760`);
   - external functions with Φ summaries (libc data functions);
   - **`unknown_effect`** sites (unresolved indirect calls, inline assembly, externals without summaries), which may write every support location whose address escapes.
3. **Assumption A-TYPE and its checks.** Every access to a support location goes through its declared type. Two listings make A-TYPE checkable; each hit makes the affected support location's writer set `DISABLED`:
   - **untyped writes**: every write through `char*`, `void*`, a cast, or a union member whose target may be a support type;
   - **address escape**: for each support location, every address-of expression must flow only into formals that have out-pointer summaries and do not let the address escape (stored, returned, or passed to an unsummarized callee).
4. **Support locations of a conjunct**: the field-based read set of the conjunct's oracle functions (§1.6), the locations bound to the conjunct's resource by Φ `bind` declarations, and, for value conjuncts, the `loc` of the `derives` fact.
5. **Writer candidates**: every writer entry of a support location.
6. **Transition-aware filtering** ([J3-M1]). For each candidate (value expression `v`, dominating guard `g` restricted to conditions over the same record, the same location and key-derived values), decide in a small abstract domain (constants and intervals for enum and integer fields; defined/undefined/reserved for resource IDs; record-type tags; per-allocation-record fields, step 8):
   - `W` (may make the conjunct true) iff some pre-state satisfying `g` has the conjunct false and the post-state has it true; `W^must` (definitely establishes it) iff every pre-state satisfying `g` leads to a post-state with the conjunct true;
   - `D` (may make it false) symmetrically; neutral otherwise.

   Undecidable cases are kept (sound over-approximation). Example: the scan's `AppState = WAITING` store is guarded by `AppState == RUNNING` on the same record (`cfe_es_apps.c:1144`): neutral for "used → AppState ≥ RUNNING".
7. **Mapping to Φ effects (O7)** and the **transition status** of each entry:
   - `COMPUTED`: established by steps 2–6, and Φ agrees;
   - `DECLARED_VERIFIED`: Φ's guard was confirmed by step 6;
   - `DECLARED_TRUSTED`: Φ's guard excludes a site that step 6 cannot exclude (e.g. the relational guard of `CleanUpApp`, §3.6). The entry is listed in every certificate that uses it and is refutable by O3/O4/O9;
   - `DISABLED`: an untyped or escaping write, an `unknown_effect`, or an undeclared writer. T-OBS is disabled for the conjunct; instances depending on it become `unknown(writer_incomplete)`; the count is reported.
8. **Keys, allocation records and the key status** [D] (fixes [X1-M3]).
   - Each writer entry has a **key expression** (from Φ: `owner(exec)`, `arg.AppID`, a constant name, a slot index, a record pointer).
   - **Allocation records.** Every unrolled **reservation node** (a call that selects a free record and marks it used: `CFE_ResourceId_FindNext` + `SetUsed(RESERVED)` at `cfe_es_start.c:789/803` in each core-loop iteration; `cfe_es_apps.c:821` per script line) is an allocation site; the record it allocates is the abstract record `ρ(n)`. Two assumptions, both `DECLARED_TRUSTED` and refutable by observation (O9): **A-FRESH** — distinct reservation nodes allocate distinct records, never used earlier in the run (in the default B, records are freed only on paths that end in the panic, or in teardown); **A-OWNER** — the record that `GetAppRecordByContext` returns for a task created by `StartAppTask(…, ParentAppId)` is `ρ(n)` for the reservation node `n` whose `PendingAppId` flows to `ParentAppId`. The flow itself is **computed** by def-use (`CFE_ES_APPID_C(PendingAppId)` at `cfe_es_start.c:814`); only the task-record → App-record step is trusted.
   - **Key status** of a writer entry w.r.t. an instance key: `MUST_KEY` (the same constant, the same symbolic value, or the same allocation record), `MAY_KEY` (may be equal), `NO_KEY` (provably different, e.g. two distinct allocation records under A-FRESH, or two different constants). `NO_KEY` entries are dropped from the instance's writer set; PROTECTED (b) needs a `MUST_KEY` must-writer (§4.5).
   - Per-allocation-record fields set in the reservation's critical section with `MUST_KEY` (e.g. `Type = CORE` at `cfe_es_start.c:795`) enter the step-6 domain as record tags.
9. **INIT, initial falsity and falsifiers.** INIT is the program-entry state (§1.7): the static-initializer image for process-scope locations (e.g. `CFE_SB_Global` is a zero-initialized global, `cfe_sb_task.c:40`, re-read), decoded S0 for persistent ones. `INIT_FALSE(P_j)` holds if the conjunct is false in INIT; EarlyInit stores are ordinary writers. A **falsifier** `f` for an observation `o` is an event in `o`'s actor that dominates `o` (modulo T-TERM) and definitely makes the observed conjunct false for its key. Example: `SetUsed(ρ(f), RESERVED)` at `cfe_es_start.c:803` makes `ρ(f)` used while its `AppState` still has its INIT value UNDEFINED, given A-FRESH and that every writer of that field that may target `ρ(f)` is ordered after `f` or excluded (checked with PWB, §4.5).
10. **S3b (conditional).** An Andersen-style inclusion analysis over the closed IR (Andersen 1994 [V-abs]), with a pinned pass pipeline without InstCombine and SROA, is built only if the S3a O7 report shows `DISABLED` conjuncts that block certificates. Its gain is measured by ablation 17.

Outputs (`facts.kz` writer sections, `WRITERS`, `ALLOCS`): per conjunct, W, W^must and D with transition and key statuses, support locations, `unknown_effect` sites, the A-TYPE listings, the O7 report and the counts of disabled conjuncts. Cost: linear in AST size for S3a [I]; measured at S3a.

### 4.3 Skeleton construction (`kz-static skel`): two views, provenance, predicates

For each static actor (each generation copy in `B_restart`), the inlined, context-qualified CFG is projected onto **relevant nodes**: Φ effects at their LPs (per conjunct, incl. capture points), support-location loads and stores, spawn sites, terminal operations, handler marks, loop heads, and explicit local choice points [D].

**Two views** [D] (fixes [X1-C1]):
- **Certificate view `Sk^cert`.** Every loop that is not unrolled exactly to a reference bound is **star-closed**: a loop-head node, the back edge, and an explicit iterate/exit choice; message-loop dispatch is folded by message provenance and the rest is a local choice. There is no horizon and no truncation. `inactive`, provider `none`, MustBefore, once facts, PWB, the frontier rule and the K-CAP bound are computed **only** on this view.
- **Exploration view `Sk^h`.** Star-closed loops are unrolled to h iterations; the region beyond h is marked `TRUNC`. It is used only by the explorer (incl. its σ_U∞ runs) and by O8. Every verdict derived from it carries h in b. `kz proofcheck` rejects any certificate step that names a node of `Sk^h` or a `TRUNC` region.

**Guards.** A branch on an operation's outcome becomes an outcome-labelled transition. Branches on B constants are folded (CF's empty `sem_name` makes the lookup unreachable, `cf_def_config.c:60, 78`). Dispatch on `(MsgId, function code)` is folded by message provenance (below). Other data branches become **explicit local choices** (over-approximation, visible to the explorer).

**Terminal operations** (T-TERM, §4.4): Φ declares terminal ops per PSP with a citation: `CFE_PSP_Panic` and `abort` on pc-linux, and **`CFE_PSP_Restart` for its caller** (§3.5). Calls to them have no successor edge, even though the C code continues after them under unit test (`cfe_es_start.c:110`; `cfe_es_api.c:363`).

**Message provenance** [D] (fixes [X1-M7](a)). For each pipe `p` (created at a site with a resolved key):
- `Routes_B(p)` = MsgIds subscribed to `p` at subscribe sites reachable in `Sk^cert`;
- `Tx_B` = keys of transmit sites reachable in `Sk^cert` (header-carried keys, §3.9; a `⊤` key contributes every MsgId routed anywhere);
- `Input_B(p)` = MsgIds of B.Input routed to `p`;
- `Prov(p) = (Routes_B(p) ∩ Tx_B) ∪ Input_B(p)`, with function codes carried the same way (`⊤` if not constant).

Reachability and `Prov` depend on each other (a transmit inside a handler counts only if its handler is reachable), so they are computed as the **least fixpoint** of: start from the code reachable without dispatch; add the handlers whose `(MsgId, code)` is in the current `Prov`; recompute `Tx_B` and `Prov`; repeat. Handlers outside the final `Prov` are folded away in both views. This is sound under A9 (B.Input is the complete set of external events) and the transmit-key analysis: every message that reaches a pipe was transmitted by a reachable site or injected by B.Input. Expected effects [I]: `K-ATOM.tbl-load` and HS AppMon (default tables `NOACT`) become `inactive` in the default B; ES's reset commands are unreachable without an input.

**Loops** (no arbitrary unrolling):
- **Poll loops, recognized by pattern** [D] (fixes [X1-M8]): a loop whose body calls a Φ delay op and whose exit edges test shared loads, results of Φ operations, or a count/time budget. The loop becomes `await(Pred_code) {success} {timeout}` where **`Pred_code` is extracted** from the serialized exit-by-condition branch by symbolic evaluation over the step-6 domain. Recognized forms: (a) a direct test of a shared load or a Φ-op result (`SystemState < MinSystemState`); (b) a counter-zero test after a constant-trip loop, `cnt := 0; for i in [0,N): if φ(i) then cnt++; exit iff cnt = 0`, which yields `∀ i < N: ¬φ(i)`; (c) Boolean combinations of these. Anything else → `unmodelled-sync`. The summary is valid (Lemma L1, §6.2) only if (i) the body's mod set contains no relevant location other than the counter and budget, and (ii) all reads of the test happen inside one critical section of one lock, or the test reads one location atomically (`MainTaskSyncDelay`: inside the ES lock; `WaitForSystemState`: one `volatile sig_atomic_t` read). If (i) or (ii) fails, the loop is not summarized: it is unrolled to its reference bound if it has one, else star-closed, and T-OBS does not apply to it. Under M_impl, loops containing OSAL lookups are not summarized, because lookups can leak the lock and their unlock broadcasts consume waiters' attempts ([A §4.2]).
- **Count-bounded retry loops** → unrolled exactly to the bound **constant-evaluated in the build** (CF: effective `CF_STARTUP_SEM_MAX_RETRIES`; OSAL: 5 attempts, `osapi-idmap.c:475–480`). The exhaustion branch is reachable only after the last attempt, in every σ (fixes [X1-M5](c)). If the iterations are paced by a Φ delay op, the loop is tagged **delay-paced**: under σ_U∞ it is patient (§1.8). A non-constant bound gives `unknown(bound)`.
- **Constant-trip loops** over B constants with relevant events in the body (the core object table loop of `CFE_ES_CreateObjects`; the startup script; record-table scans) → unrolled.
- **Message loops** (`while (CFE_ES_RunLoop(...)) { ReceiveBuffer; dispatch }` and core main loops) → a handler automaton with `handler_begin/end` marks: star-closed in `Sk^cert`, unrolled to h in `Sk^h`.
- **Other loops** containing relevant events → star-closed in `Sk^cert`, unrolled to h in `Sk^h`.

**Once facts.** For every static actor, once(τ) is decided by §1.2 on `Sk^cert`; for every node, once(n) is "once(actor) and not inside a star-closed region". The facts and their derivations go to `skel.kz`.

**Recursion** makes the affected summary `unknown`.

**Inlining and its fallback** (fixes [X1-m7]). Full call-string inlining is worst-case exponential in call depth. If skeleton construction for an actor exceeds its work-unit cap (a deviation, §4.15), the functions responsible fall back to **per-(actor, function) summaries**: one copy of the function body per actor, shared by all its call sites in that actor. The fallback is recorded per function. Precision lost under it, all in the sound direction: T-PO sees spurious paths through the shared body (smaller MB); `Anc(u)` grows (fewer frontier nodes); nodes in a shared body called from more than one site are not once; templates inside the shared body are no longer per-context, which can withhold R2-style certificates for uses inside shared SB functions. Fallback counts and their effect on certificates are measured at S3a.

**Output** `skel.kz`: both views, sizes, local-choice counts, loop summaries with their bounds and policy status, extracted predicates, provenance sets, once facts, fallback list.

Assumption **A-SKEL**: `Sk^cert` over-approximates each actor instance's projected event sequences under B. O1/O2 test it on every trace. `Sk^h` is an under-approximation beyond h by construction and is never used for universal claims.

### 4.4 The MustBefore fixpoint (`kz-static static`)

**Graph.** The nodes of `Sk^cert` for all actors; intra-actor edges plus spawn edges (spawn site → child entry). MB is computed per M (M_impl skeletons differ on the M_impl-relevant operations).

**Definition.** `a ∈ MB(n)` means: in every execution of `Exec(B,σ_U,M)`, at every occurrence of `n`, some occurrence of `a` has occurred earlier. MB is the **greatest** solution of the equations below [D] ([A §4.4], corrected per [J1-1], [J2-2], [X1-m2], [X2-01]).

**Base cases** (fixes [X1-m2]): `MB(entry) := ∅` for the initial actor (PSP main), for environment roots, for asynchronous contexts without an in-model registration site (signal handlers, the exception path), and for every actor whose in-model spawn-site list is empty. An empty meet in T-SPAWN is ∅, never ⊤. All other nodes start at ⊤.

| Rule | Equation contribution | Basis |
|---|---|---|
| T-PO | `in_PO(n) = ∩_{p ∈ pred(n)} (MB(p) ∪ {p})` over `Sk^cert` predecessors (including back edges of star-closed loops); terminal ops have no successors (T-TERM) | program order; dominance modulo terminal branches |
| T-SPAWN | for an actor entry `n_τ`: `in(n_τ) = ∩_{c ∈ SpawnSites(τ)} (MB(c) ∪ {c})`, a meet over **all** spawn sites (registration sites for callback actors). If any spawn site of τ is unresolved (`unknown_effect`), `in(n_τ) = ∅` | `OS_TaskCreate` → child start ([R01 §2.2]) |
| T-OBS | for an observing exit `o` with observed conjunct `C` (an instance implied by the extracted `Pred_code`), falsifier `f` (§4.2 step 9) and the writer set `W_o(MB)` defined below: `in(o) = in_PO(o) ∪ ∩_{w ∈ W_o(MB)} (MB(w) ∪ {w})`. Applies only if `W_C` has no `DISABLED` entry, `Pred_code ⇒ C` is established, and `C` is false right after `f` (or at INIT when no falsifier exists). If `W_o = ∅` at the fixpoint, `o` is MB-unreachable and marked | reads-from: some writer between `f` and `o` made `C` true |
| T-TERM | terminal operations have no successor edges | panic and the restart requester end ([J1 §1 check 1]; §3.5) |
| (none) | lock acquisition alone gives no must-order | locks give atomicity, not order ([R04 L5]) |

**The writer set of an observation** [D]. `W_o(MB)` contains every context-qualified node of every writer site in `W_C` (all contexts of each writer site, fixes [X1-m2]), **except** those removed by one of four rules, each re-checked by `kz proofcheck`:

1. **OWN-NOPATH** (fixes [X2-01]): `w` lies in `o`'s own actor, `once(actor(o))` holds, and `Sk^cert` has no path `w →* o`. *Soundness:* if `w` occurred between `f` and an occurrence `o_k` in that actor's (unique) run, the run would contain `w … o_k`, so by A-SKEL the skeleton would contain a path `w →* o`. Example: `memset(AppRecPtr, 0, …)` at `cfe_es_start.c:823`, whose only continuation is the panic.
2. **PWB-before-falsifier**: `PWB_MB(w, f)`, by the frontier rule or by once-PWB (§4.5), using the current MB.
3. **PWB-after-observation**: `PWB_MB(o, w)`.
4. **KEY-SEP**: the key status of `w` w.r.t. `C`'s key is `NO_KEY` (§4.2 step 8).

`W_o` is **recomputed on every iteration** from the current MB. The operator stays monotone: as MB shrinks, fewer PWB facts hold, `W_o` grows, and the meet over more and smaller sets shrinks. Starting from ⊤ (where the PWB rules may prune everything and the empty meet is ⊤), iteration converges to the greatest fixpoint.

Priority rules of σ_R1 are **not** in MB; σ_R1 is handled by the explorer.

```
for each actor τ (certificate view, per M):
    MB(entry_τ) := ∅  if τ is the initial actor, an environment root, an asynchronous context without
                      in-model registration, or has no in-model spawn site;   MB(n) := ⊤ otherwise
repeat
  for each actor skeleton, for n in reverse post-order:
     in := in_PO(n)                                         # T-PO; terminal ops have no successors (T-TERM)
     if n is an actor entry with spawn sites: in := ∩_{c ∈ SpawnSites} (MB(c) ∪ {c})   # T-SPAWN
     if n is an observing exit with a valid writer set:
         W := W_o(MB)                                       # recomputed: OWN-NOPATH, PWB (frontier, once), KEY-SEP
         in := in ∪ ∩_{w ∈ W} (MB(w) ∪ {w})                 # T-OBS
     MB(n) := in
until no set changes      # monotone decreasing from ⊤: terminates at the greatest fixpoint
```

**Complexity** ([A §4.4]). With N nodes and R relevant events (bitset width), each update costs `O((indeg + |W_o|) · R/64)` word operations, plus the PWB checks for `W_o` (one reachability query per frontier candidate, cached per iteration). Sets only shrink, so there are at most N·R changing updates. MB is computed once per (B, Φ, M).

**Justifications.** For each fact a certificate uses, the derivation (rule, predecessor, writer exclusions) is reconstructed on demand by backward search over the final equations.

### 4.5 Certificates and the independent checker

Let `Anc(x)` be the set of nodes of `x`'s actor that have a `Sk^cert` path to `x` (including `x`). All certificates are computed per M on `Sk^cert`.

**PWB certificates** [D] (fixes [X1-M2]):
- **FRONTIER**: `PWB(a, b)` if `once(actor(a))` and `∃ m` in `actor(a)` with `m ∉ Anc(a)` and `m ∈ MB(b)` (L-PWB, §6.2).
- **ONCE-PWB**: `PWB(a, b)` if `once(a)` and `a ∈ MB(b)`: `a` occurs at most once, and an occurrence of `a` precedes every occurrence of `b`, so that single occurrence precedes all of them (L-ONCE, §6.2). Example: the raise of ES main (`cfe_es_task.c:134`) is once and in `MB(o_ES) ⊆ MB(f_EVS)`, so `PWB(raise_ES, f_EVS)` (App. A.1).

**SCOPE exclusion** [D] (fixes [X1-M6], [X2-04]): a writer or disabler `d` with `SHUTDOWN_REQ ∈ MB(d)` can be followed only by out-of-scope occurrences of any use; certificates for in-scope occurrences drop it (L-SCOPE). SCOPE is **not** used inside T-OBS, because MB facts hold for all executions, teardown included.

**FIXED_ORDER certificate** for instance `i` at level M ([J2-1], [J3-M1]). Choose a conjunct `P_j`, with its LP node `ℓ_j` at the use (the capture point for value conjuncts). All of the following must hold:

1. `INIT_FALSE(P_j)` for the instance's key.
2. `W_j` (after `NO_KEY` and SCOPE exclusion) has no `DISABLED` entry.
3. If `W_j = ∅`: provider `none` (or `env-absent` if Φ guards the provider with an absent env capability); the class follows from badness (item 5).
4. **Frontier**: `once(actor(ℓ_j))` and `F = (Nodes(actor(ℓ_j)) \ Anc(ℓ_j)) ∩ ⋂_{x ∈ W_j} MB(x) ≠ ∅`. Pick `m ∈ F` (closest to `ℓ_j`). Equivalently, per writer, `PWB(ℓ_j, x)` by FRONTIER or ONCE-PWB.
5. **Badness**: every not-ready outcome of the use is outside `Allowed` on every `Sk^cert` continuation within the horizon (trivial for `Allowed = none`). If only some continuations are disallowed, the certificate establishes "fixed not-ready" (shape WRONG-WAY or ABSENT) and the class stays open.
6. **Reach**: `proved` from an observed occurrence (run id) or a confirmed model path; otherwise `reach = assumed` ("fixed-order if reached").

Certificate = (INIT_FALSE proof; `derives` fact if `P_j` is a value conjunct; `W_j` with transition and key statuses; the once facts used; the frontier node with its no-path proof, or the ONCE-PWB facts; one MB derivation per writer, each including its T-OBS writer exclusions; badness proof; reach). The proof never needs the use to dominate anything, so uses behind early returns are covered.

**PROTECTED certificate.** For every conjunct `P_j`, one of:

- (a) `P_j` is true at INIT and every `d ∈ D_j` is removed by NO_KEY, SCOPE, or is MB-unreachable; or
- (b) `∃ x* ∈ W_j^must` with key status `MUST_KEY` and transition status `COMPUTED` or `DECLARED_VERIFIED`, with `x* ∈ MB(ℓ_j)`, and every remaining `d ∈ D_j` satisfies `PWB(d, x*)` or `PWB(ℓ_j, d)` (by FRONTIER or ONCE-PWB), or is removed by NO_KEY or SCOPE (fixes [X1-M3](a)).

No `DISABLED` writer may exist; reach as above. The refinement `tolerated` needs, in addition, a witness of an allowed not-ready occurrence.

**INACTIVE**: no `Sk^cert` path from any actor entry reaches the template under B constants and message provenance. Proof = the folded path condition plus the provenance sets used. `Sk^cert` over-approximates paths (A-SKEL), so "no path" is sound; `Sk^h` is never used for INACTIVE.

**Provider axis**: `none` iff `P_j` is INIT_FALSE and `W_j = ∅` with no `DISABLED` entry; `env-absent` iff the only `W_j` entries are Φ effects guarded by an `env` predicate that B.Env records false (status `DECLARED_TRUSTED`) and observation agrees (O4).

**Independent checker `kz proofcheck`** ([A §4.4]; [J1-1]). It shares no code with `static`, links no LLVM, trusts `facts.kz` (extraction) and re-validates each step:
- **view**: every node is a `Sk^cert` node; any `Sk^h` or `TRUNC` node → reject (fixes [X1-C1]);
- **T-PO**: dominance modulo terminal ops, by its own dominator routine;
- **T-SPAWN**: the spawn-site list equals the closure's list; base cases ∅;
- **T-OBS**: the falsifier condition; **the predicate**: re-derive `Pred_code` from the serialized loop-exit expressions with its own symbolic evaluator, check the L1 preconditions, and check `Pred_code ⇒ C` (fixes [X1-M8]); `W_o` recomputed from the writer sections, with each exclusion (OWN-NOPATH by its own reachability; PWB; KEY-SEP) re-checked;
- **ONCE**: spawn-site regions and pairwise exclusivity by its own reachability (fixes [X1-M2]);
- **DERIVES**: the copy chain from the call argument to `loc` in the serialized AST facts (fixes [X1-M1]);
- **FRONTIER**: no path `m →* ℓ_j`;
- **SCOPE**: `SHUTDOWN_REQ ∈ MB(d)` re-derived;
- **T-TERM**: Φ declares the op terminal for this PSP, with a citation;
- **statuses**: transition and key statuses consistent with the writer sections; the list of `DECLARED_TRUSTED` entries is complete;
- **reach**: the run exists and contains the occurrence.

**Certificate-mutation tests.** A mutation engine perturbs valid certificates: drop a spawn site; delete a writer; swap `m` for a node in `Anc(ℓ_j)`; mark a non-terminal op terminal; replace COMPUTED by DISABLED; **drop a once fact** (e.g. the one that prunes `raise_ES` from `W_o(o_EVS)`); **re-insert `cfe_es_start.c:823` into `W_o`** without its OWN-NOPATH proof; **move a step into a `TRUNC` region**; **replace the extracted predicate by a stronger one** (e.g. one that forgets that RESERVED counts as used); **drop a `derives` link**; change a `MAY_KEY` into `MUST_KEY`. Every mutant must be rejected; the kill rate is a reported checker metric. It tests the checker, not the analysis: the analysis is tested by program mutants (§8.5).

### 4.6 Census and candidate generation

```
for each M ∈ {M_api, M_impl (if the instance's cone is M_impl-relevant)}:
 for each contract κ in Φ (frozen set), each context-qualified template t matching κ.U (certificate view):
   key := value analysis of κ.sel at t       # concrete | symbolic | ⊤; header MsgIds and table packets (§3.9)
   for each conjunct P_j: (W_j, W_j^must, D_j, statuses) := writer sets (§4.2), keyed by key
   provider := none | env-absent | present | unknown        (§4.5)
   if t unreachable in Sk^cert under B constants and provenance:  emit inactive(B) + proof
   elif key = ⊤:                                                   open(key_unresolved) → probe plan
   elif FIXED_ORDER certificate exists:     emit fixed-order(σ_U, M), reach proved|assumed, provider
   elif PROTECTED certificate exists:       emit protected(σ_U, M), reach proved|assumed
   else: open(reason ∈ {unordered, O-ordered-only, conditional-observation, writer_incomplete, horizon,
                         unmodelled}) → probe plan, prediction, exploration
```

Every certificate goes through `kz proofcheck` before the verdict is stored. All census instances get a class or `unknown(reason)` per (σ, M). Latent instances get a model verdict, not just "unobserved" ([J2-7]). Dynamic instance sources (§4.8 step 9, §7.1) add instances to the same census. The census reports the **shares of concrete, symbolic and ⊤ keys** per kind, in particular for transmit sites (RC5) (fixes [X2-09]).

### 4.7 Probe-plan synthesis (`kz-static plan`)

1. **Mandatory synchronization skeleton** for all actors: the anchors of every S and O rule (OSAL task, mutex, semaphore, queue, timer and module operations; ES lock wrappers, waits, `RunLoop`, `ExitApp`, app/task creation; SB create, subscribe, route_lookup, put, receive; the shutdown store). Keeping all of it avoids the third-party-synchronization slicing error noted in v0.1 §7.3.
2. **Candidate obligations**: the LP intervals of each conjunct of open instances, including **capture sites** (§4.8 step 4); outcome capture at contract-use sites only; the results of guards that decide `Allowed`; write watchpoints on readiness fields that no API boundary covers; user stacks where identity frames need them; the writers named in A-COMM checks of open prediction candidates (§4.9).
3. **Joint debug-register allocator** [D] (fixes [X3-m1]). HW watchpoints (perf breakpoint PMU) and ptrace gates share the same four per-thread debug registers [I: through the kernel's hw_breakpoint layer]. `plan` and `kz hold` use one allocator: watchpoints plus gates ≤ 4 per thread; **per-task events only** (a per-CPU breakpoint would take a slot on every thread on that CPU); **write-only watchpoints**, except in a declared experiment (RW watchpoints fire on every read at ≈34 µs, e.g. on `AppTable` at every `GetAppID`, [R07] L339).
4. **Cost**: each site has an instruction class and a unit cost from the **calibration table measured on this VM** (§5.10). The plan records `ΔC_i = Σ_j n_ij · c_j` per thread (v0.1 §11.2).
5. **Selection**: greedy weighted set cover of obligations by sites; optimality not claimed. The plan's hash goes into every trace.

### 4.8 Trace analysis (`kz trace`)

1. **Decode and loss.** Decode raw records with the site decoders (site id = attach cookie). Check per-thread `tseq` gaps and per-CPU drop counters. Verdicts whose evidence depends on an interval with loss become `unknown(loss)`.
2. **Identity join, offline**; nothing is looked up in the process (§5.7).
3. **HB.** Compute `VC` (all definite edges) and `VC_R` (unconditional and obs-fatal edges), **with the looper rule: no edge between handlers of one looper** (§1.9). Matching keys: mutexes by `objseq`; queues by descriptor plus FIFO; SB by descriptor plus pipe; semaphores by conservative join; polls only on a success return; registries by outcome-implied reads-from. Ambiguous matches weaken the evidence of the verdicts that use them.
4. **LP intervals per conjunct** (fixes [X1-M1]). Every LP occurrence gets an interval `[the last observed event of its actor before the LP, the first observed event after it]`. For a value conjunct, the LP is the capture point: since only argument evaluation separates the capture load from the call, the interval is `[the actor's previous observed event, the call-entry event]`; where that is too wide for a decision, the plan probes the capture site itself. Two LPs are **HB-decided** if their intervals are HB-ordered or linked by an outcome-implied reads-from. Otherwise their order is open; it is **outcome-decided** if Φ's outcome map determines it from the code, else ambiguous.
5. **Per-use evaluation and routing** ([J3] weakness 6). With L⁻, L⁺ and L∥ the lifecycle LPs on `sel(u)` before, after and unordered with each conjunct's LP:

| Case | Result | Route |
|---|---|---|
| L∥ = ∅, every conjunct holds | READY (or TOLERATED if allowed) | supporting evidence only |
| L∥ = ∅, some conjunct fails, every ordering edge used is S (VC_R order) | observed not-ready; Allowed evaluated | ask `static` for a FIXED_ORDER certificate on the template; `VC_R` order is the prefilter |
| L∥ = ∅, but the order between a conjunct's LP and the deciding lifecycle LP needs ≥ 1 O-edge (lock, message, semaphore, soft poll, RF) | observed outcome in that order | **prediction**: is the reverse order feasible? (C1, C5, both App-record windows) |
| L∥ ≠ ∅ | outcome-decided order | prediction for the other order |

   The program's own outcome code (`ILLEGAL_APP_ID`, `NAME_NOT_FOUND`, `NEVER_LOADED`, `RESOURCEID_NOT_VALID`, `Q_FULL`, sentinel success) must agree with the computed state (O4(a)). **O4's checking power is reported only over HB-decided uses**; outcome-decided uses are excluded from that count, because there the state was derived from the code being checked (fixes [X1-m4]).
6. **Allowed** is evaluated on the actor's continuation within the horizon (§1.6). If the trace ends first, the result is `unknown(horizon)`.
7. **Race coverage** (EventRacer OOPSLA 2013 [V-abs]): if reversing a pair needs an ad hoc synchronization pair reversed first, that pair is reported as the root and the dependents are attached.
8. **Violation chains** from frame containment and Φ `emits` annotations (e.g. a `SEND_NO_SUBS` event emitted inside a transmit with no route).
9. **Template-free sources**: outcome mining and key-pair enumeration (§7.1 items 2–3).
10. **Obligations** O3–O5 and O9 here; O1/O2 in `kz conform`.
11. **Scope**: occurrences after an observed SHUTDOWN_REQ LP are K-TEARDOWN instances (§3.5).

Complexity: `O(N·|T|)` for N events and |T| actors ([M] 27 threads per boot, [BM §0]).

### 4.9 Prediction (`kz predict`)

**Question.** Given trace π and a routed candidate (e.g. publisher `p` before use `u`, observed lock-ordered), is there a feasible execution in which the deciding conjunct's LP at `u` precedes `p`'s LP, or, for capacity, in which a put overflows?

**Model `Pred(π)`** (maximal causal models, RV 2012 [V-abs]; RVPredict's control-flow abstraction, PLDI 2014 [V-abs]; properties as constraints, GPredict ICSE 2015 [V-abs]):
1. **Cone C**: per-actor prefixes up to `u` and `p`, closed backward under S-edges and under the reads-from of control-relevant events.
2. **Constraints**:
   - per-actor prefix order (including the sequential execution of handlers on a looper, and dequeue order = enqueue order per pipe, §1.9);
   - S-edges;
   - every control-relevant event in `C \ {u}` keeps its observed outcome class: a successful lookup needs a preceding publication and no retire; a poll exit by condition needs the enabling write first; **an acquire needs the lock free** (it is held by no other actor at that point; fixes [X1-M4]);
   - Φ primitive semantics: mutual exclusion with recursion, binsem saturation and flush, non-negative counts, queue FIFO with capacity `cap_B`, MsgLim;
   - the target order or outcome flip.
   - **C_σ is never added.** σ_R1 feasibility is decided only by the explorer on fully modelled cones, never by combining C_σ with an observed σ_E prefix (fixes [X1-m5]).
3. **Lock reversal is allowed** ([J1-7], [J2-3], [J3-M2]). Critical sections are atomic blocks and the registry key is the conflict object; two critical sections on one lock may be reordered whenever constraint 2 still holds. This is what C1, C5 and the App-record windows need, and what sync-preserving prediction forbids ([R04 §2.5]); it is the RVPredict/GPredict-style treatment ([R04 L259, L492]).
4. **A-COMM check and labels** [D] (fixes [X1-M4]). Kairos traces contain the synchronization skeleton, LP intervals and contract-site outcomes, not every shared access. Before a witness is labelled `predicted`, `kz predict` checks **A-COMM** using the read and write sets from S3a: for each cone actor's code between its observed events up to `u` (resp. `p`), every location it may read that another cone actor may write must be either (a) a Φ support location whose writer LPs in the cone are **all observed** in π, or (b) provably constant over the cone (no writer reachable in any cone actor after INIT), or (c) read only through an observed outcome that the witness keeps. If the check passes, the witness is `predicted` (trace-sound relative to A4, A7 and Φ's primitive semantics, L-PRED). If it fails, the witness is **`predicted-modelled`** and ranks with model evidence; the failing locations are listed, and the plan adds probes for them in the next loop iteration.
5. **Search**: depth-first over linearizations of C with sleep-set partial-order reduction. Independence comes from Φ: operations on different keys, locations and objects commute; critical sections on one lock commute as wholes unless they conflict on a key or location. States are memoized. The search stops at the first witness.
6. **Prefilters only.** SHB (OOPSLA 2018 [V-abs]) for unordered pairs, and WCP (PLDI 2017 [V-abs]) / SyncP (POPL 2021 [V-abs]) for pairs ordered through non-conflicting critical sections, run first where their domain applies. Their soundness transfers to the abstract trace only under A-API. Any reversal they propose is re-validated by step 2's checker (Vindicator's candidate-plus-vindication split, PLDI 2018 [V-abs]). Their silence means nothing.
7. **Summary-guided extrapolation** ([B §4.4 (e)]). A retry or poll site observed once may be extended by further not-ready iterations up to its **effective reference bound**. Such a witness is **`predicted-extrapolated`**: model-based, admitted only where the loop summary passed O2 ([J1-7]).
8. **Budget**: a deterministic work-unit cap (search nodes), a **deviation** (reason: finite resources; effect: `unknown(budget)`; necessity: no reference defines it). Results are reported as an **anytime curve**: the decided share against work units (fixes [X3-M9], [X3-M10]).
9. **Result**: a witness with its realization label, or "not predictable from π within the cap". **Neither is a protected verdict.**
10. **Optional SMT cross-check** after PI decision P-Z3: an order-variable encoding in the style of Alglave, Kroening and Tautschnig (CAV 2013 [V-abs]) and Wang et al. (FM 2009 [V-abs]), emitted as SMT-LIB2 for small cones; never on the critical path.

Complexity: prediction with even one synchronization reversal is NP-complete and W[1]-hard (SyncP [V-abs]); the explicit search is exponential in |C| in the worst case.

### 4.10 Bounded explorer (`kz explore`; stage S7, gated)

1. **Closed model per candidate.**
   - The **cone** ([A §4.5]): actors containing `u`, `W`, `D` or support objects; every actor that writes a cone location or synchronizes on a cone object; the spawn chains that reach them; repeated to a fixpoint. The full synchronization skeleton of the cone actors is kept. **Under σ_R1, every actor whose priority is at least the minimum cone priority is kept** (it interferes through the CPU; fixes [X1-m5]).
   - B constants; Φ semantics at level M; awaits per σ; retries unrolled to effective bounds; inputs from B.Input; the exploration view `Sk^h` (h in b).
   - **Trace-anchored windows** (optional, [B §4.4]): the prefix is fixed to an observed run up to the latest common S-predecessor of {p, u}; verdicts are labelled "within window W of run r".
2. **Transition system.** A state is (actor control states, resource states, synchronization states, relevant locals, a discrete clock under σ_R1). A transition is one effect at its LP, or an explicit local choice: a havoc value (finite domain from value analysis, else `unknown`); an await outcome (success enabled iff `Pred_code` holds; timeout always enabled under σ_U, never under σ_U∞); a retry exhaustion (the failure of the last unrolled attempt; under σ_U∞, for delay-paced loops, enabled only at quiescence); a star-closure iterate or exit within h ([J1 X6]).
3. **Dependence** `D(t₁, t₂)` holds iff both belong to the same actor; or they touch the same key, location or object and at least one changes it; or one may enable or disable the other (lock/unlock of the same lock; give/take of the same semaphore; put/get on the same queue incl. capacity and MsgLim; an await versus any write to its predicate's support; a spawn versus the child's first transition; a join versus the victim's last transition). Under σ_R1, every transition that can change the readiness of a higher-priority actor is dependent with all lower-priority transitions (conservative, [A §4.5]).
4. **Two engines** [D] (fixes [X1-M9]):
   - **`exh`**: stateful exhaustive search over the finite closed model with a visited-state set and no partial-order reduction. Its correctness is that of reachability on a finite graph: every reachable state within b is visited. Results from `exh` carry G5 relative to the model (§6.3).
   - **`dpor`**: source-DPOR with sleep sets (Abdulla et al., POPL 2014 [V-abs]), with blocking handled by the enabling/disabling dependences of item 3. Its correctness for systems with disabling is the theorem of the JACM 2017 version (source-DPOR extended to disabling), whose details are **status P** in [R05 §2.2]. Until that theorem's statement is verified against the paper (PI decision P-PAPERS), **G5 for `dpor` results is labelled unverified**. The differential test against `exh` on every model small enough for `exh` is **validation evidence only**, not a proof. Optimal DPOR (wakeup trees, "one execution per class") is not adopted and not claimed.
   - Every verdict records its engine.
5. **Reductions**, each switchable and ablated:
   - **L1** (await summarization with code-extracted predicates; §4.3, §6.2);
   - **L2** (critical sections irrelevant to the cone are removed, [A §4.2]);
   - **L3** (**critical sections as mover-reduced atomic blocks**, fixes [X1-M10]). A critical section on lock ℓ becomes one atomic transition only if a static check shows: the acquire is a right mover and the release a left mover; every access inside to a location protected by ℓ is a both-mover; **at most one non-mover** inside (a blocking call, an operation on another synchronization object, or an access to an unprotected shared location); **the lock is released on every path** (paths that leak the lock, such as K-LOCKBAL under M_impl, fail the check); and **no nested blocking acquire**. `CFE_ES_DeleteChildTask` (cancel and join under the ES lock, `cfe_es_api.c:1348, 1383`) fails the check and stays fine-grained, so K-LOCK.cancel executions are not removed. Critical sections that fail stay acquire/release transitions. The left/right-mover reduction theorem is used as proof obligation L3; its citation is an open literature check (not verified in R04/R05).
6. **Bounds.** Horizon h and work-unit caps: h is not chosen; the explorer reports results for each h it completes within the work-unit cap, which is a **deviation** (finite resources), with anytime curves (decided share against work units). Wall time and peak memory are costs only. No preemption bound by default; if one is introduced, BPOR (Coons et al. OOPSLA 2013 [V-abs]) is used, because naive DPOR plus a bound is unsound within the bound, and the bound is swept from 0 upward until the cap (SCTBench norm, [R05 §2.1]).
7. **Two queries** ([R05 §2.3]): `Q_bad` = some execution reaches `u` and is bad (or stalls before `u`, §1.8); `Q_good` = some execution reaches `u` and is good. (yes, no) → fixed-order within b; (no, yes) → protected within b; (yes, yes) → schedule-dependent; (no, no) → `u` unreachable in the model, compared with the static verdict (a disagreement is O6); cap exhausted → `unknown(budget)`. Both queries run under σ_U and σ_U∞ (refinements, §1.8), and under σ_R1[P_sched] when requested. **Each σ's class is computed independently**; refinements are derived afterwards from the per-σ results and stored with their own bounds.
8. **Monitor** per execution: Req at every conjunct LP; Allowed within the horizon; the shape (looper rule, API-level edges); a **deadlock check** (all unfinished cone actors blocked, at least one on an object no unfinished actor can release; exposes OSAL F1 under M_impl); `stall` detection at quiescence.
9. **Kill criterion** ([J3-M3]). At the S7 review the explorer is run on the development-set cones that lack a certificate (both App-record windows, the C1-derived lookup, the K-PHASE CF instance, the `SBNSubPipe` K-CAP instance) up to the PI-set work-unit cap (deviation; decision D15). If none of these cones is decided, or if `dpor` and `exh` disagree on any small model, the explorer is frozen; its outputs are then reported only as an ablation.
10. **Fidelity.** Explorer verdicts carry `model(b)` realization. They rest on O1, O2, O6 and O8, and are never labelled realized.

### 4.11 Conformance (`kz conform`)

- **O1/O2, per actor, on every trace** ([J3-M9]). Each actor's decoded event sequence is run through its `Sk^cert` automaton by NFA simulation over the explicit local choices; linear in events × automaton states. A failure gives the shortest rejected prefix.
- **O8, global, from stage S5** (fixes [X3-M5]). The question is whether some model execution's projection equals all per-actor sequences, with outcomes and matched synchronization. It is answered by a **trace-constrained search built on S5's prediction engine** (the trace fixes each actor's sequence; the search looks for a consistent interleaving under Φ semantics), not by the gated explorer. Timestamps are only the search order heuristic ([A §4.8]). The sampling rule is pre-registered (the first trace of each pool, plus every trace with an O1–O7 or O9 failure); the work-unit cap is a deviation. Results: conforming, non-conforming (with the region), undecided. Before S5, O8 is reported as `NOT_RUN`.

### 4.12 Combining evidence into verdicts

1. **Cells.** Each instance has one cell per analysed (σ, M). σ_U first, then σ_U∞, σ_R1[P_sched] on request; σ_E last. Classes are computed per cell; refinements are derived from the cells and stored with their bounds.
2. **Universal classes** (`protected`, `fixed-order`) need a certificate (b = ∞ on `Sk^cert`) or exhaustive exploration within b (`exh`, or `dpor` labelled unverified). Observations are supporting evidence only.
3. **`schedule-dependent`** needs a bad (or stall) witness and a good witness in the same cell; the record keeps the weaker of the two **per evidence axis** (realization, naturalness).
4. **σ_E cells** come from transfer rule 2 (universal classes, only from M_impl or M-agreement) or from **natural realized witnesses on both sides** (`schedule-dependent(σ_E)`). Realized witnesses with held or gated naturalness give `σ_E(held)`. Frequencies (`n of N`) are pooled only over identical pool keys (B hash, plan hash, stratum) and are labelled `σ_E(plan P)`; equivalence to unobserved runs is claimed only on the measured uninstrumented marginals (§8.6) (fixes [X3-M2]). Absence of bad witnesses is written "no σ_E bad witness in n of N runs (pool key); σ_E class unknown" (fixes [X1-M11]).
5. **Contradictions** trigger O3/O4/O9: the rule is suspended, a model-fault record is written, the affected verdicts become `unknown(model_fault)`, and the loop ends for this Φ version. An M_api-only certificate contradicted by the binary gives `portability-hazard-evidence` instead (§1.4).
6. **Harm** comes from Allowed and the observed or model continuation; with Allowed unspecified, harm is `unspecified_by_reference`, and route/capacity verdicts carry LOSS-UNDER-ORDER.
7. **M levels**: M_api always; M_impl for instances whose cone is M_impl-relevant. Disagreements give `portability-hazard` or `implementation-defect-candidate`.
8. **Perturbation qualifier** (fixes [X3-M2]): every realized witness carries its computed perturbation ratio (§4.13); no threshold is applied.

### 4.13 Confirmation on the real binary (`kz hold`, `kz gate`)

**Which verdicts get controlled confirmation** ([A §4.9]):

| Verdict | Action |
|---|---|
| fixed-order (certificate) | passive observation (reach and outcome). A good counterpart exists only under a changed basis B′ (e.g. another core order): a configuration-change experiment, labelled as such |
| protected | none beyond conformance |
| schedule-dependent / timeout-dependent | directed confirmation of the bad witness; if natural runs show only the bad order, also of the good witness |
| unknown(model_gap, writer_incomplete, conformance) | directed attempts at predicted witnesses, as evidence; the class stays unknown until the model is fixed |
| unknown(budget) | F1 PCT runs if D3 is approved; otherwise remains unknown |
| LOSS-UNDER-ORDER | observation through SB counters and put outcomes |

**Gate placement** ([J2-10], [J3-M10]). To force "event `a` of thread A after event `b` of thread B", hold A at a point before its first event of the pair where:
1. **A's program-lock set is empty**, by the observed lock events and the static lockset (incl. the ES lock, the SB lock, TBL locks, and the window between a `CFE_ES_DeleteChildTask` lock and its delete);
2. the point is **not** inside a timebase callback (`handler_mutex` held), a binsem internal section (the internal 2 s `timedlock`, [R01 §6.3]), a signal handler, or before `OS_TaskPrepare` registration;
3. the point is not inside an OSAL RESERVED window, except as a **declared experiment** (App. A.4);
4. the joint debug-register allocator (§4.7) has a free slot on A.

If no point qualifies, the rule is applied symmetrically: hold B before its enabling event.

**Attach strategy** (fixes [X3-m1]): threads that exist at plan time are seized individually (`PTRACE_SEIZE`). A gate on a thread created later (e.g. App. A.4's provider) needs auto-attach at clone (`PTRACE_O_TRACECLONE` on its creator); each new thread then stops once at creation. That stop is an unmeasured perturbation inside the app-start windows; S6 measures its cost, and runs that use it record it.

**Release** is event-based: when the awaited event (with its outcome) appears in the live stream of `kz observe`, or when the other party is blocked or has exited ([POL] L22).

**Watchdog** = the smallest reference timeout among the waits that the hold can make expire, read from Φ and B: e.g. `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` for a held startup thread whose child polls `GetTaskFunction`; `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` for core serialization; **`CFE_PSP_RESTART_DELAY` when the held schedule can lead to a restart request** (the requester aborts after it, §3.5); effective retry bound × effective delay for CF. If the watchdog fires, the result is `inconclusive(await-timeout)`. If a program timeout fires during a hold (seen in the return status), the run's naturalness is `timeout-exercised`.

**Normalization after abnormal exits** [D] (fixes [X3-M3]). A run that did not end with the console line "Exiting cFE with POWERON Reset status" (`cfe_psp_support.c:64`) — a panic, a PROCESSOR restart, a requester abort, a harness kill of a deadlock — leaves the SysV segments and a boot record that makes the next boot a PROCESSOR reset (`cfe_psp_start.c:411–434`). After every such run:
1. one **unobserved normalization boot** is made, ending with the CI POWERON restart already used as E11(a) (`.github/workflows/build-run-app-reusable.yml` L209–214; [BM] L29), which deletes the segments (`cfe_psp_support.c:62–69`) without any manual cleaning ([R2g]'s "not cleaned (policy)" practice is kept);
2. the persist snapshot after it must equal the canonical S0 (content hash, no mtimes);
3. the normalization boot is excluded from every pool;
4. cold runs (the first after a VM reboot) form their own stratum and are never pooled with warm runs.

**Mechanisms.**
- **F0 `kz hold`** (default): `PTRACE_SEIZE` by root; an **execution breakpoint in a debug register** (DR0–DR3, set through `PTRACE_POKEUSER`) at the gate, so the text is never modified ([B §4.6]). Only that thread stops. On release, the debug register is cleared. Measured unit costs: #DB trap ≈34 µs; gdb breakpoint ≈203 µs ([R07 §4.7]). The holder's stop/resume cost and its interaction with uprobes and signals (the timebase's real-time signal, `SIGUSR1`) are **unmeasured** S6 exit items.
- **F1 `kz gate`** (only after D3): handwritten `__wrap_*` gates for the synchronization-skeleton APIs, in one C99 file built with the cFS flags; the controller enforces witness schedules or PCT. PCT gives probability at least `1/(n·k^(d−1))` per run for a depth-d bug ([V-abs; mechanism P]), with n and k measured per run and d increased from 1 within the PI-set run budget (a deviation), relative to the gated scheduling points.

**Oracles.** `kz trace` re-evaluates the confirmation trace with the same contracts: the target use must show the not-ready state at its LP and the predicted outcome; harm follows Allowed. For deadlocks and lock leaks (K-LOCKBAL) the oracle is a **wait-for fact**, never a timeout: the blocked thread's state from `/proc/<tid>/wchan` or futex tracepoints, while the lock's last acquirer has already returned from its API ([C §4.9]).

**Labels.** `realized` with naturalness `held-F0` | `gated-F1` | `timeout-exercised`; flag `derived` (kept apart from original-case results, [R06 S10]; [LED] X5); `refuted` (the order was realized and the outcome was good: the model or a summary is wrong; a model-fault record is written); `inconclusive`.

**Perturbation accounting per run** (fixes [X3-M2]): holds (count, site, duration); probe hits by cost class; observer CPU time and placement; the uninstrumented-observable measures of §8.6; and, **per witness**, the **perturbation ratio** = (probe and hold time added on the involved threads between the two deciding events, from the run's own hits × the S1 class costs, plus hold durations) / (the code-derived separation that protects the other order: e.g. the 50 ms poll delay of `CFE_ES_GetTaskFunction`, `cfe_es_apps.c:559–596`; the ~100 ms core-serialization spacing, [R2g §2]; SCH_LAB's first tick at least 1 s after init). The ratio is reported with no threshold.

### 4.14 The feedback loop, its reproducibility and its termination (`kz loop`)

```
fix (B, Φ, tool);  K0 := static closure of B;  census0 := kz-static static(K0)
i := 0
repeat
   plan_i   := kz-static plan(open instances of census_i ∪ mandatory sync skeleton ∪ A-COMM probes)
   runs_i   := kz observe(B, plan_i) × n_i      # n_i: deviation (PI-set, D14); run IDs; normalization boots
   check O1–O5, O9 (+ O8 on the sample, from S5); a suspension ENDS the loop for this Φ version
   K_{i+1}  := K_i ⊔ keys, targets and paths learned from runs_i                  # F1, F2
   census_{i+1} := kz-static static(K_{i+1}) ⊔ kz trace(runs_i)
                   [⊔ kz predict(runs_i)          from S5]
                   [⊔ kz explore(K_{i+1})          from S7, if not frozen]
   [for each witness w: π_w := kz hold/gate(B, holdplan(w)); census_{i+1} ⊔= kz trace(π_w)   from S6]  # F3
   write loop-i.kz: run IDs, B/plan/Φ/tool hashes, S0 hashes, deltas, obligation results, work units
   i := i + 1
until state(census_i) = state(census_{i-1}) and K_i = K_{i-1}
report the fixed point together with its run set ∪_i runs_i
```

- **Staged availability** (fixes [X3-M5]): at S4 the loop runs with `static` and `trace` only (O1–O7, O9); `predict` joins at S5 (and O8), `hold` at S6, `explore` at S7 if it passes its kill criterion.
- **The fixed-point test** (fixes [X1-m8]) compares `state(census)`: per instance, the per-(σ, M) class and refinements, the provider and reach axes, the **set** of realization and naturalness values present, the key resolution, and K. It excludes counts (`n of N`), pool keys, run IDs and plan hashes, which change every iteration.
- **Lattice.** Per cell: `unknown(·)` is below every class; the classes `inactive`, `protected`, `fixed-order` and `schedule-dependent` are pairwise incomparable, so a cell moves at most once, from `unknown` to a class (a universal class later contradicted by a witness is a contradiction handled by O3/O4 and a suspension, never a lattice step); refinements and the provider and reach axes move only from unset to set (`reach`: `assumed < proved`); evidence sets are ordered by inclusion; keys `⊤ < symbolic < concrete`; K by inclusion. Within one Φ version every non-suspending iteration moves up or stays.
- **Suspensions are non-monotone**, so a suspension ends the loop for that Φ version; the fix needs a human Φ edit, which produces a new Φ hash and a new loop.
- **Termination**: under fixed B and Φ, the universe of sites, keys (from code constants and finite tables), instances and evidence values is finite, and every non-final iteration strictly moves up the lattice or stops [I].
- **Reproducibility** ([J3-M7]; fixes [X3-M10]): every analysis is a pure function of stored artifacts (sorted iteration, deterministic search order, work-unit budgets, no wall-clock dependence), so re-running from stored traces reproduces the verdict store byte for byte; this is a test. A result produced under a wall-clock cap (only if D15 ever allows one) is labelled and excluded from the replay test. σ_E frequencies are pooled only over identical pool keys. The fixed point is reported **with its run set**.

### 4.15 Complexity and bounds

| Stage | Cost | Bounds and their sources |
|---|---|---|
| Closure and extraction | linear in TUs; [M] 10 s parse, 4.4 s bitcode, 0.35 s core link | — |
| S3a writer extraction | linear in AST size [I]; measured at S3a | — |
| S3b points-to (if built) | Andersen worst case cubic ([R05 §2.5]) | work-unit cap (deviation) |
| Skeletons | worst case **exponential in call depth** under full call-string inlining; per-(actor, function) summary fallback when the work-unit cap is reached (deviation) | poll loops summarized (L1); retry loops at effective constant bounds (CF: effective `CF_STARTUP_SEM_MAX_RETRIES`; OSAL 5, `osapi-idmap.c:475–480`; `GetTaskFunction` ≤ 21, `cfe_es_apps.c:570–595`) |
| Message provenance | least fixpoint over handlers; at most one round per handler | — |
| MustBefore | `O(N·R·(d̄+w̄)·R/64)` plus cached PWB queries | — |
| Certificates | reachability per frontier; derivations | — |
| Trace HB and contracts | O(N × number of actors) | — |
| Prediction | NP-complete in general (SyncP); exponential worst case | work-unit cap (deviation); anytime curve |
| Exploration | `exh`: linear in reachable states, exponential in the model; `dpor`: exponential worst case; **no "one execution per class" claim** | h not chosen (results per completed h); work-unit cap (deviation); preemption bound none by default (BPOR and sweep if used); restart depth 1 per documented request |
| Conformance | O1/O2 linear; O8 a search (from S5) | work-unit cap (deviation) |
| Confirmation | one run per witness, plus a normalization boot after abnormal exits; PCT ≥ `1/(n·k^(d−1))` per run | watchdogs from platform constants; PCT depth d increased from 1 within the run budget (deviation) |
| Feedback loop | ≤ the lattice height under (B, Φ) | runs per iteration: deviation (D14) |

---

## 5. Observation and instrumentation design

### 5.1 Mechanisms and fidelity classes

Fidelity classes are those of [R07 §7]: F0 = unmodified binary and run conditions, timing perturbed only; F1 = same sources and compiler, link or code-generation option changed; F2 = sources rewritten; F3 = another compiler.

| Mechanism | Class | Role | Unit cost ([M] [R07 §4.7, §5.1] unless stated) | Notes |
|---|---|---|---|---|
| perf-PMU uprobes + eBPF, root-side, own raw loader, **attach cookies** | F0 | **production observation** of API-boundary and function-boundary events in `core-cpu1` and app `.so` files | 0.57 µs/hit at entry+4 (emulated `push %rbp`); 31.7 µs at `endbr64`, at `mov %rsp,%rbp` (+5) or any non-emulated instruction; BPF body **unmeasured** | callee-side probes also see the 88 intra-TU API sites that `--wrap` misses |
| **perf-sample uprobe backend** (sample flags `PERF_SAMPLE_TID`, `PERF_SAMPLE_TIME`, `PERF_SAMPLE_CPU`, `PERF_SAMPLE_REGS_USER`; `PERF_RECORD_LOST` for loss) | F0 | **zero-install bring-up backend** (fixes [X3-m2]); fd-scoped, cleans itself up | same trap path; sample cost unmeasured | measured to work without tracefs and with registration before the file is mapped ([R07] L340) |
| tracefs dynamic uprobe events | F0 | optional only; if used, the rules of §5.4 apply | same trap path | event definitions are kernel-global and outlive a private mount namespace [I] |
| HW write watchpoints (perf breakpoint PMU, per task) | F0 | readiness-field stores with no API boundary | ≈34 µs/hit; shares the 4 debug registers with gates (§4.7) | only for fields written a handful of times per boot; write-only except in declared experiments |
| ptrace + debug-register execution breakpoints | F0 | confirmation holds (`kz hold`) | #DB trap ≈34 µs; gdb breakpoint ≈203 µs; purpose-built holder **unmeasured** | no Yama; root on a uid-1000 process works |
| sched/syscall tracepoints | F0 | clone cross-check; RacePro-style baseline observation model (§8.3) | not measured | exist on this kernel |
| `ld --wrap` gates | F1 + deviation | PCT and many-point schedules, **only if D3 approves**; perturbation group W | handoff 1.1 µs same CPU / 10.5 µs across CPUs | apps relinked; misses intra-TU calls |
| Source probes | F2 | not planned | — | 12.4% of API sites are inside macro expansions ([R07 §4.6]) |
| Clang-built execution | F3 | never | — | — |
| `LD_PRELOAD` / `LD_AUDIT` | — | rejected: cannot interpose symbols defined in the PIE executable; refused by `run_cfs.sh:154–158` | — | [R07 §4.3] |
| gdb | F0 | reference observation only (the R2 records; ran with ASLR disabled, [R2g]) | +0.6 to +1.1 s to OPERATIONAL ([R2g §1]) | — |
| valgrind helgrind / DRD | F0 binary, serialized threads | **labelled external reference point** (§8.3), run through `RUN_CFS_WRAP` like gdb | large slowdown, not measured on cFS ([R07] L466) | memory-level only; valgrind serializes threads, so its schedule is not README-mode scheduling |

The observer runs as root next to the README-mode `ubuntu` process. This needs a new root-side hook in `env/run_cfs.sh` (PI decision D1, [R07 §9 item 1]).

### 5.2 Probe placement and return-value capture

- **Function entries at entry+4** [D]. gcc emits `endbr64; push %rbp; mov %rsp,%rbp` at `-O0`; checked on nine core functions and one app entry ([B §A.2]) and on `CFE_EVS_SendEventWithAppID` ([X3, here]). At entry+4 the argument registers still hold the arguments.
- **Locks and queues without return probes** ([B §5.5]; [J3-M4]). Mutex order from `objseq` at `OS_MutSemGive` entry; queue edges from put entry and the get outcome where an edge or a contract needs it.
- **Return values only at contract-use sites** and where an outcome decides an edge. The capture mechanism is **chosen per site by measurement in stage S1**:
  - (R-a) uretprobe on the callee. This kernel has the uretprobe syscall trampoline (`__x64_sys_uretprobe`, `arch_uretprobe_trampoline` in `/proc/kallsyms`, [X3, here]), so R-a may be cheaper than the #DB class [I];
  - (R-b) uprobe at the caller's instruction after the `call` (typically `mov %eax,-0x..(%rbp)`), inferred to be in the 31.7 µs class like the measured `mov` at +5 [I];
  - (R-c) uprobe at the callee's epilogue `leave`: class unmeasured;
  - (R-d) **deferred read**: out-pointer targets captured at entry are dereferenced at the same thread's next probe hit; valid only where the static check shows that no event of that thread between the return and the next probe can write the target. Used at sites that ignore the status, e.g. `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` at `cfe_sb_task.c:138` (fixes [X3-m3]);
  - (**R-e**) **the first emulated instruction after the call while the value is still in a frame slot** (fixes [X3-m3]). At `-O0`, gcc spills the result and tests it: e.g. after `call CFE_ES_GetAppID` in `CFE_EVS_TaskInit`, `mov %eax,-0x124(%rbp)`, `cmpl $0x0,-0x124(%rbp)`, `je` ([M] [X3, here], `0x2aca1`–`0x2acae`). Branches are emulated by x86 uprobes (`branch_emulate_op` present), so a probe on the `je` that reads the slot at `rbp-0x124` is plausibly in the 0.57 µs class [I].

  Each planned probe records its cost class. No overhead claim is made before S1.
- **Internal points** (publication stores not at an API boundary): an entry probe on the out-of-line local copy of the `static inline` helper, bound by DWARF compile unit (copies exist, §3.17); otherwise a write watchpoint; otherwise the nearest instruction of a cheap class.
- **Capture sites** (value conjuncts, §4.8 step 4): the call-entry probe of the consuming API usually brackets the capture point closely enough; the plan adds a probe at the capture load only where a decision needs it.
- **Site validator.** Every planned address is disassembled with LLVM MC and looked up in the calibration table (§5.10). Expensive sites are reported, never accepted silently.
- **App `.so` files: register by inode, system-wide, before launch** (fixes [X3-M1], [X3-m4]). Perf-PMU uprobes are attached to (file inode, offset) for every process, before `core-cpu1` starts; this was shown to count hits for a file mapped after registration ([R07] L340, tested on the main binary only; S1 tests it on an app `.so`). The v1 fallback (holding the startup thread at each `OS_ModuleLoad` return) is **removed from the default design**: it delays app loading, the very variable that changed the init interleaving in [BM] (849 ms load span instead of 2.2–3.3 ms; 10 apps initialized before the last was loaded). If it is ever used, its traces are excluded from σ_E pools and say so.

### 5.3 eBPF data path

- **Site keying by attach cookie, never by runtime address** [D] (fixes [X3-M1]). ASLR is on (`randomize_va_space` = 2, [M] [X3, here]; `mmap_rnd_bits` 28, [R07] L77); `core-cpu1` is a PIE and apps are `dlopen`ed at random bases. A map keyed by the probed runtime IP would have to be filled per module after mapping, racing first execution (all 15 apps load within 2.2–3.3 ms, [BM] L278). Instead each site's **attach cookie** equals its site id:
  - option 1: one perf uprobe fd per site, linked with `BPF_LINK_CREATE` carrying the cookie;
  - option 2: one `uprobe_multi` link per binary with per-offset cookies.

  The program reads the cookie with `bpf_get_attach_cookie`. The kernel symbols for both options exist (`bpf_perf_link_attach`, `bpf_get_attach_cookie_pe`, `bpf_get_attach_cookie_uprobe_multi`, [M] [X3, here]; presence only). S1 probes both functionally.
- **One generic BPF program per phase** (entry, point, return, watch). The cookie indexes a **site-descriptor array map**: registers to copy, stack slots and bounded dereferences, whether to read a bounded string (bounds `OS_MAX_API_NAME` 20 and `OS_MAX_PATH_LEN` 64 from the built `osconfig.h`, [B §A.2]), whether to assign an `objseq`, whether to capture a user stack. Descriptors are **data**; there is no per-site code generation ([C §5.3]).
- **Within verifier limits** [I] (fixes [X3-M1]): `pt_regs` is read only at constant offsets, so the program copies a fixed register set at constant offsets and then selects with bounded code; stack slots and dereferences are read with `bpf_probe_read_user`; strings with a constant maximum.
- **No libbpf** ([R07 §2.3]): helper prototypes are written by hand against the UAPI `linux/bpf.h` (present, [J3 §2.4]); the BPF ELF object (system clang-18 `-target bpf`) is parsed with glibc `<elf.h>`. Neither needs an install.
- **Per-thread `tseq`** in task-local storage (or a map keyed by tid). **Per-object `objseq`** by atomic add on a per-object map entry, assigned only at in-critical-section points [I].
- **Buffers**: per-CPU ring buffers (or a perf event array). Nothing is shared with the traced program in user space; the traced threads make no extra system calls and take no user-space lock ([R06 F10]).
- **Stacks**: `bpf_get_stack` with `BPF_F_USER_STACK` (frame-pointer unwinding of gcc `-O0` code), at sites that need on-behalf frames; depth from the maximum static call depth, capped by `perf_event_max_stack`; truncation is flagged.
- **Feasibility** [U]. Raw `bpf()` load and attach to a uprobe were probed ([R07 §4.7]). Ring buffers, task storage, `bpf_get_stack`, attach cookies and `uprobe_multi` were **not** probed. S1 tests each one. The fallback is the perf-sample backend.

### 5.4 The tracefs option (only if used)

tracefs is no longer the bring-up path (fixes [X3-m2]). If it is used anyway: a **dedicated tracing instance**; an explicit cross-CPU trace clock set for that instance (the default "local" clock is per CPU [I]); every event definition removed at exit; the use recorded in RUNMETA as a **host-state change**. Per-thread order then comes from timestamps under assumption **A-CLK-PO** (clocksource `tsc` with `constant_tsc` and `nonstop_tsc` on all four vCPUs, [B §A.2]). Cross-thread order comes only from matched protocol keys; lock order inferred from timestamps is `clock-ordered` evidence, excluded from VC, VC_R, certificates and prediction constraints ([J2-4]).

### 5.5 Event record and trace container

The raw record `kz_raw_ev` (64 bytes, §2.3) carries a timestamp (evidence only), tid, `tseq`, site id (= cookie), phase, flags, `objseq` and four decoded values. Strings and stacks go into side sections keyed by `(tid, tseq)`.

A `.kzt` container has these sections: `SITES` (copied from the plan, with the binary build-id, inode and offsets); `RAW_EVENTS`, `STRINGS`, `STACKS`; `LOSS`; `RUNMETA` (B hash, plan hash, S0 hash, stratum, kernel version, backend, holds, observer CPU time and placement, harness version); `PERTURBATION` (per-run accounting, §4.13).

`kz trace` decodes `RAW_EVENTS` into typed `EVENTS`: actor instance, operation, resource, outcome, identity quadruple, template link and per-conjunct LP intervals.

### 5.6 Loss accounting

Ring-buffer reservation failures increment per-CPU, per-site counters (perf backend: `PERF_RECORD_LOST`). Per-thread `tseq` gaps identify the lost records of each actor. Any loss marks the affected actor's interval as incomplete, and every verdict whose evidence depends on that interval becomes `unknown(loss)`. **Loss is never treated as the absence of events.** Buffer size is set from a calibration run's high-water mark and recorded ([B §5.2]).

### 5.7 Identity bootstrap without `GetAppID`

No in-process lookup is ever made by the instrumentation; all joins are offline. An instrument that called an OSAL name lookup could trigger the F1 lock leak it is meant to detect ([R01 §6.3]); calling `GetAppID` early would itself hit the App-record window ([v0.1] §8.4).

1. Every record carries the kernel tid.
2. Creator side: `OS_TaskCreate` entry (name, entry pointer, priority) and its out-parameter.
3. Child side: `OS_TaskEntryPoint(osal_id)` (exported, `osapi-task.c:124`) binds tid ↔ OSAL id before `OS_TaskPrepare`; `OS_PthreadTaskEntry` (`os-impl-tasks.c:131`) is a `.symtab` fallback ([J3 §2.4]).
4. ES side: `CFE_ES_StartAppTask` arguments (`ParentAppId`), the task-record publication, and the App-record publications (AST-bound sites, §3.17). These joins also check A-OWNER and A-FRESH (O9).
5. Timebase thread: `OS_TimeBase_CallbackThread(timebase_id)` (`osapi-timebase.c:365`); callbacks attributed through `OS_TimerAdd` records.
6. Raw threads by creation site; PSP-set kernel thread names (15 characters, `cfe_psp_start.c:197–201`) are an independent second check.
7. Generations from OSAL and cFE serials; across runs, `(type, name, creator, creation ordinal)`.
8. `svc` frames by symbolizing user stacks against the gcc binary; `stamp` from arguments, using Φ's parameter roles.

### 5.8 Causal tokens versus timestamps

Cross-thread order comes **only** from matched synchronization: `objseq` for mutexes; descriptor plus FIFO for queues; descriptor plus pipe for SB; outcome-implied reads-from for registries; the recorded exit status for polls; OSAL ID for task creation. Semaphore matching uses the conservative join. Where a protocol key is insufficient, a timestamp window may be used as *matching evidence*: the edge is then ambiguous and the evidence weakened. **`CLOCK_MONOTONIC` values never become HB edges** ([R04 §2.1, §4 row 16]; v0.1 §9.3).

### 5.9 Safety rules for observation and control

- eBPF runs in kernel context. It calls no OSAL API, takes no user lock, never prints, and cannot be cancelled mid-record, which removes by construction the in-process hazards of [R01 §6.3].
- What remains is timing. A trap inside an OSAL table-locked region lengthens the hold and could consume bounded RESERVED-wait attempts (§3.2: by broadcast **or** by timeout). The planner prefers entry probes outside OSAL internal locks and flags any probe inside an idmap critical section, a timebase callback or the binsem internal section.
- Holds follow §4.13. Observer CPU time and placement are recorded in every run, because the drainer competes for the 4 vCPUs during the startup burst ([BM §1.3] could not separate this effect for a root-side hook).

### 5.10 Cost model and measurement obligations

Per thread `i`: `ΔC_i = Σ_j n_ij · c_j` (v0.1 §11.2); per confirmation run add `Σ_holds (c_hold + hold duration)`. **Six costs are reported separately**: analysis, exploration, recording, control, profile authoring (person-hours), and triage (rater hours, §8.5) ([R06 R4]; [J1-15]; [X3-M7]).

| Quantity | Value | Status |
|---|---|---|
| `c_trap` at entry+4 (`push %rbp`) | 0.57 µs | [M] |
| `c_trap` at `endbr64`, `mov %rsp,%rbp` (+5) or another non-emulated instruction | 31.7 µs | [M] |
| R-b post-call `mov %eax,-0x..(%rbp)` | inferred 31.7 µs class | [I]; S1 |
| R-e emulated branch after the spill | inferred 0.57 µs class | [I]; S1 |
| R-a uretprobe (syscall trampoline present); R-c `leave` | — | unmeasured; S1 |
| `c_bpf` (program body, cookie lookup, string fetch, stack capture) | — | unmeasured; S1 |
| `c_wp` (HW watchpoint hit) | ≈34 µs | [M] |
| `c_hold` (gdb breakpoint) | ≈203 µs | [M]; purpose-built holder and auto-attach at clone unmeasured (S6) |
| ptrace × uprobe and ptrace × signal interactions | — | unmeasured; S6 |
| gate handoff (F1) | 1.1 µs same CPU / 10.5 µs across CPUs | [M] |
| `n_ij` per boot | — | unmeasured for the full sync skeleton; for scale only: the retired CORD recorder logged 626–632 events per boot ([R2c §4.1]); 240–258 `sb.appid` uses per boot ([R2c §7]) |
| static API call sites | 3,762 | [M] static count ([R07 §4.2]) |

Arithmetic on measured values only, not a prediction [I]: 626 hits at 0.57 µs is about 0.36 ms per boot; at 31.7 µs about 20 ms. OPERATIONAL is about 1.6 s ([M] 1613–1636 ms, [R2g §1]); CORE_READY → OPERATIONAL is 103.2–104.6 ms in cFE time across nine runs ([R2c §4.4]). The startup burst (`SBNSubPipe` overflow about 47–49 ms after OPERATIONAL, [BM §1.3]) is where probe time concentrates and where the per-witness perturbation ratio (§4.13) will be largest.

**Calibration table.** S1 measures each instruction class with a micro-benchmark on this VM, by the method of [R07 §4.7]: `push`, `mov`, `leave`, the emulated branch (R-e), uretprobe, the BPF body with and without string and stack capture, cookie lookup, the perf-sample path, and the holder's stop/resume. The table is versioned by kernel. **Group A is re-measured on `-fc-v114`** before any comparison ([R07 §1]; D11).

### 5.11 Untested items and fallbacks

| Item | Status | Fallback |
|---|---|---|
| inode-registered uprobes on an app `.so` before `dlopen` | untested on a `.so` ([R07 §9 item 6]); works on the main binary | none needed by design; the startup-thread hold is excluded from pools if ever used |
| attach cookies (`BPF_LINK_CREATE` / `uprobe_multi`) | kernel symbols present; not functionally probed | perf-sample backend (site from the event's perf fd) |
| ring buffer, task storage, `bpf_get_stack` | not probed | perf-sample records with registers and user stack |
| return-capture mechanisms (R-a…R-e) | unmeasured | calibration decides per site; deferred read only where statically valid |
| `c_bpf` with stack capture | unmeasured | capture stacks only at sites that need frames |
| purpose-built ptrace holder cost; auto-attach at clone; ptrace × uprobe interaction | unmeasured | gdb-equivalent bound (≈203 µs per stop) for planning only |
| BPF ELF loading without libbpf | design inference | install libbpf (P-LIBBPF) |
| out-of-line copies of `static inline` helpers at `-O0` | **closed**: copies exist ([X3, here]) | — |

---

## 6. Guarantees

All statements are relative to the basis B, the profile Φ (hashed, every rule cited), the model level M, the semantics σ, the bound b, the skeleton view, and the assumptions A below. Terminology follows [R05 §0.5]: "over-approximate relative to M and A", "complete up to b"; never a bare "sound". This is "soundy" in the sense of Livshits et al. (CACM 2015 [V-abs]).

### 6.1 Assumptions A (the soundiness manifest)

| Id | Assumption | Effect if violated | Mitigation / visibility |
|---|---|---|---|
| A1 | Rule validity: every Φ rule, effect and binding is correct for the pinned revision | wrong edges or effects | citations; O3/O4/O7 refute; certificates list the rules used |
| A2 | Binding correctness: Clang sites and AST patterns map to the gcc binary's instructions | wrong probes or anchors | anchor audit; DWARF compile-unit binding |
| A3 | Closure: actors come only from the startup script **selected by B**, the generated object table, `CreateChildTask` sites, OSAL timebase/timer creation and OSAL-internal threads, PSP threads and environment inputs; `dlsym` targets are resolved by configuration; residual indirect calls are `unknown_effect`; code outside the compile DB is out of model | missed actors or targets | closure boot incl. the opened script (O5); `unknown_effect` → T-SPAWN ∅, writer sets DISABLED |
| A4 | Memory model: sequential consistency at event granularity on x86-64 TSO at `-O0`; ad hoc flag synchronization is an assumed-visibility edge | weak-memory effects missed | edges tagged `assumed`; sub-label; D10 |
| A5 | API-level edges only for shapes; OSAL-internal locks constrain interleavings only under M_impl | — | ablation 11 |
| A6 | Observation completeness for planned sites, with loss accounting | missed events | `tseq` gaps; `unknown(loss)` |
| A7-impl | Φ_L-1 semantics (pthread mutex, condvar incl. timed wait, `sem_*`, `mq_*`, `clock_nanosleep`, thread create/cancel/join) over-approximate kernel, libc and OSAL POSIX behaviour on the M_impl paths at LP granularity (equal-priority FIFO for `mq` is UNVERIFIED, [R04 §4 row 7]; OSAL always uses priority 1) | containment fails | litmus suite on the real OSAL (§9.2); O4 |
| A7-api | Every OSAL primitive **not** covered by M_impl (A10) is linearizable at its LP on the real backend | containment fails for those primitives | litmus tests; O4 |
| A8 | Out of model: inline assembly, signals beyond the PSP exception path and `SIGUSR1` shutdown, ISRs, DMA, weak-memory effects | — | listed |
| A9 | Inputs: B.Input is the complete set of external events of the run (message provenance relies on it) | ground-driven paths latent; handlers wrongly folded | scenario families as B′; O5 (an observed message outside `Prov(p)` is a closure failure) |
| A10 | M_impl covers the OSAL idmap, task, binsem and queue paths only | other primitives at API level in both | listed; A7-api |
| A11 | Table values: from `.tbl` on POWERON; **from the decoded CDS image for critical tables on PROCESSOR resets**; runtime table loads only via B.Input | wrong table contents | table cross-check; CDS decoding in `basis` |
| A12 | One process; SBN peers and cpu2 out of model | cross-instance contracts not covered | listed ([R03 §9 item 8]) |
| A-SKEL | `Sk^cert` over-approximates each actor instance's projected behaviour under B | spurious or missing model behaviour | O1, O2, O8 |
| A-WS | Writer-set completeness: S3a (and S3b) find every writer of every support location, given A-TYPE, and every `DECLARED_TRUSTED` guard is correct | unsound T-OBS | O7; trusted entries listed per certificate; O3/O4/O9 refute; ablation 17 |
| A-TYPE | Typed access: every access to a support location goes through its declared type | untyped writes missed | the untyped-write and address-escape listings (§4.2 step 3): each hit disables the location |
| A-FRESH | Distinct reservation nodes allocate distinct, previously unused records | wrong key separation | `DECLARED_TRUSTED` per use; O9 |
| A-OWNER | The App record resolved for a task created by `StartAppTask(…, ParentAppId)` is the record of the reservation whose `PendingAppId` flowed to `ParentAppId` | wrong ownership | `DECLARED_TRUSTED` per use; O9 |
| A-COMM | Per predicted witness: every location read in a cone actor's unobserved code that another cone actor may write is observed, constant, or read through a kept outcome | a "predicted" witness infeasible | checked per witness (§4.9); failure → `predicted-modelled` |
| A-API | The abstract trace is faithful enough for SHB/WCP/SyncP soundness to transfer | prefilter proposals wrong | always re-validated (§4.9 step 6) |
| A-CLK-PO | tracefs option only: a cross-CPU-consistent clock for per-thread order | wrong program order | recorded; the eBPF and perf-sample backends remove it |
| A-ZT | σ_R1 only: computation between blocking points takes zero time | σ_R1 orders that depend on execution time are wrong | sub-label on every σ_R1 verdict that depends on a delay |
| L1, L2, L3 | Lemma obligations (await summarization with extracted predicates; irrelevant critical sections; mover-reduced critical sections) | model behaviour changed | each switchable per instance; ablated; L1's preconditions checked by `kz proofcheck`; proofs written before F1 |
| T-JACM | The correctness theorem of source-DPOR with sleep sets extended to disabling (JACM 2017), status P | `dpor` results unverified | `dpor` verdicts labelled unverified; `exh` mode available; differential test as validation |

### 6.2 Lemmas and proof sketches

The model at level M: per-actor certificate-view skeleton automata plus Φ semantics at level M. `Exec(B,σ_U,M)` is the set of interleavings consistent with them.

**L-MB (MustBefore soundness).** For every execution `e ∈ Exec(B,σ_U,M)`, every node `n`, every occurrence `n_k` of `n` in `e` and every `a ∈ MB(n)`, an occurrence of `a` precedes `n_k` in `e`.

*Sketch.* Induction on the position of `n_k` in `e`; the fixpoint equations hold for the computed MB. Case by the equation that kept `a` in MB(n):
- **Base cases.** Entries with `MB = ∅` contain nothing to prove (fixes [X1-m2]: no empty meet yields ⊤).
- **(T-PO)** The actor reached `n_k` from some `Sk^cert` predecessor occurrence `p_j`, which is earlier: in `Sk^cert` every loop is either unrolled exactly to a reference bound or star-closed, so **every** actual predecessor of `n_k` (also in any iteration of a message loop) is a skeleton predecessor (A-SKEL; this is where the h-cut of v1 broke the argument, [X1-C1]). Terminal ops have no successors. Since `a ∈ MB(p) ∪ {p}`, the hypothesis on `p_j` gives an earlier occurrence of `a`.
- **(T-SPAWN)** An actor's entry is preceded by one of its spawn sites (A3: the list is complete). `a` is in the meet over all spawn sites.
- **(T-OBS)** `o_k` observed `Pred_code` true; by L1, the observation is equivalent to an atomic test at the await's success point, and `Pred_code ⇒ C`. `C` was false right after the latest occurrence `f_j` of `f` before `o_k` (falsifier obligation; `f` dominates `o` modulo T-TERM). So some write `w_l` between `f_j` and `o_k` made `C` true. By A-WS, `w ∈ W_C`. It is not removed from `W_o`: (OWN-NOPATH) if `w` were in `o`'s once actor with no path `w →* o`, its occurrence between `f_j` and `o_k` in that actor's unique run would give such a path; (PWB-before-falsifier) `PWB(w, f)` would put `w_l` before every occurrence of `f`, contradicting `f_j < w_l`; (PWB-after-observation) `PWB(o, w)` would put `w_l` after `o_k`; (KEY-SEP) a `NO_KEY` writer cannot change `C`'s key. Each PWB fact used was derived from the same fixpoint and is justified by the induction hypothesis at earlier occurrences ([X1 §1]). So `w ∈ W_o`, and `a ∈ MB(w) ∪ {w}` gives an occurrence of `a` before `w_l`, hence before `o_k`.

Locks contribute nothing. ∎

**L-ONCE.** If `once(a)` and `a ∈ MB(b)`, then `PWB(a, b)`.

*Sketch.* By L-MB, every occurrence of `b` is preceded by an occurrence of `a`. `a` occurs at most once (its actor has at most one instance, and `a` lies outside every star-closed region, so it occurs at most once in that instance's run). That single occurrence therefore precedes every occurrence of `b`. ∎

**L-PWB (frontier).** If `once(actor(a))`, `m` is in `actor(a)`, `m ∉ Anc(a)` and `m ∈ MB(b)`, then in every execution containing occurrences of `a` and `b`, every occurrence of `a` precedes every occurrence of `b`.

*Sketch.* By L-MB, an occurrence of `m` precedes each occurrence of `b`. If some occurrence of `a` came after an occurrence of `m`, the actor's unique run (once) would contain `m … a`, and by A-SKEL `Sk^cert` would contain a path `m →* a`, contradicting `m ∉ Anc(a)`. So every `a` precedes every `m`, hence every `b`. Without once, `m` and `a` could lie in different instances, which is why once is a premise (fixes [X1-M2]). ∎

**L-SCOPE.** If `SHUTDOWN_REQ ∈ MB(d)`, every occurrence of a use after an occurrence of `d` is out of scope.

*Sketch.* By L-MB an occurrence of the shutdown store precedes `d`, hence precedes the use's LP; the scope conjunct (§1.6) excludes that occurrence. ∎

**L-DERIVE.** If `derives(arg, loc, c)` holds, the value the API checks at its slot LP equals the value of `loc` at `c`.

*Sketch.* The argument is a copy chain from the load at `c` through stores, loads of thread-local temporaries, parameter passing and casts without arithmetic; none of these reads shared memory again. ∎ Hence a value conjunct evaluated at `c` decides the API's outcome (fixes [X1-M1]).

**L-FO (fixed-order certificate).** Suppose a FIXED_ORDER certificate (§4.5) exists for instance `i` at level M with conjunct `P_j` and LP node `ℓ_j`. Then in every `e ∈ Exec(B,σ_U,M)` and at every in-scope occurrence of the use, `P_j` is false at `ℓ_j`; with the badness condition, every execution reaching the use in scope is bad.

*Sketch.* Suppose `P_j` true at an occurrence `ℓ_{j,k}`. `P_j` was false at INIT, so some writer `x ∈ W_j` (complete by A-WS; `NO_KEY` writers cannot change the key's state; SCOPE-excluded writers precede only out-of-scope occurrences, L-SCOPE) occurred before `ℓ_{j,k}`. By L-PWB or L-ONCE, every occurrence of `ℓ_j` precedes every occurrence of `x`. Contradiction. For a value conjunct, L-DERIVE transfers the result to the API's check. No dominance of the use is needed, so early returns are covered ([J2-1]). ∎

**L-PR (protected certificate).** Case (a) is immediate (every disabler is NO_KEY, SCOPE-excluded or MB-unreachable). Case (b): by L-MB `x*` occurred before `ℓ_{j,k}`; as a `MUST_KEY` must-writer it established `P_j` **for this key** (fixes [X1-M3](a)). Every remaining disabler `d` satisfies `PWB(d, x*)` (it occurred before every `x*`) or `PWB(ℓ_j, d)` (it occurs after every `ℓ_j`). Hence no disabler of the key lies between the last `x*` and `ℓ_{j,k}`, and `P_j` holds there. ∎

**L-CONT (containment, per level) and transfer** (fixes [X1-C2]).
- `σ_U∞` removes timeout transitions, restricts when the last attempt of a delay-paced retry may fail (only at quiescence; until then that attempt is simply not scheduled, which σ_U allows because delays are yields), and adds a fairness restriction. Each is a restriction of σ_U at the same M, so its executions are executions of σ_U.
- `σ_R1[P_sched]` only restricts which enabled transition fires (A-ZT adds no behaviour).
- `σ_E` against `M_impl`: by A7-impl, every real step on an M_impl path is a step of Φ_L-1 at LP granularity, including timed-wait timeouts and broadcast wakes; by A7-api the other primitives are atomic at their LPs; a real timeout is one of the nondeterministic timeout transitions of σ_U; by A-SKEL each actor's real sequence is a `Sk^cert` path; by A3 and A9 no actor or input is missing; by A4 interleaving at event granularity is adequate. Hence `Exec(B,σ_E) ⊆ Exec(B,σ_U,M_impl)`.
- `σ_E` against `M_api` is **not** claimed: the real POSIX backend has executions (e.g. `INCORRECT_OBJ_STATE`) that M_api excludes by design.
- Transfer follows: a universal class over σ_U at level M holds over any subset at the same M and b, for the executions in the subset that reach the use; σ_E inherits universal classes only from M_impl (or by L-MLEVEL); a witness in a subset is a witness in σ_U at the same M; a frequency is a measure on σ_E and is not defined by σ_U. ∎

**L-MLEVEL** [I]. If an instance's cone and certificate contain no M_impl-relevant operation, then the projections of `Exec(B,σ,M_api)` and `Exec(B,σ,M_impl)` onto the cone's events coincide.

*Sketch.* The two levels differ only in the transitions of M_impl-relevant operations (§1.4). If none occurs in the cone, the cone's transitions and their enabling conditions are identical at both levels; actors outside the cone affect the cone only through cone objects (by the cone's closure), which are not M_impl-relevant. ∎ The lemma is an obligation to prove in the paper; until then transfer rule 2(i) is applied with the sub-label `mlevel-lemma`.

**L-REF (refinements are well defined)** (fixes [X1-M5]). Each σ's class is computed independently over its own maximal executions with outcomes good/bad/stall. timeout-dependent, timeout-masked-liveness and timeout-rescued are predicates over the σ_U executions and their σ_U∞ continuations from named pre-timeout states, so each is defined whenever the per-σ results exist; in particular a patient retry's last attempt is enabled as a success whenever its predicate holds and as a failure at quiescence, so it never leaves its actor without an enabled transition while the predicate can still become true (the v1 gap). ∎

**L-DPOR (explorer correctness statement)** (fixes [X1-M9]). Local nondeterminism (havoc values, await outcomes, retry exhaustion, star-closure choices) is made into explicit transitions of the acting actor, so each actor is deterministic given its choice transitions; dependence is as in §4.10 item 3.
- **`exh`**: every state reachable in the finite closed model within b is visited (finite-graph reachability with a visited set), so `Q_bad` and `Q_good` are decided exactly within b. No external theorem is needed.
- **`dpor`**: every equivalence class with a bad (or good) occurrence of `u` within b is represented **if** the JACM 2017 theorem for source-DPOR with sleep sets extended to disabling holds for this transition system class (assumption T-JACM, status P in [R05 §2.2]). `dpor` verdicts are therefore labelled **unverified** until the theorem's statement is checked (P-PAPERS). With a preemption bound, BPOR ([V-abs]) is used and the claim is "under bound p". Under σ_R1, the conservative priority dependences keep coverage under the scheduler restriction [I].
- The differential test (`dpor` vs `exh` on every model small enough for `exh`) is validation evidence, not proof. No "one execution per class" (optimality) claim is made.

**L1 (await summarization with extracted predicates).** If the poll loop satisfies the preconditions of §4.3 (body mod set free of relevant locations other than the counter and budget; all test reads inside one critical section of one lock, or one atomic read), then every exit by condition of the real loop corresponds to an `await(Pred_code)` success transition at the test's atomic read point, and every exit by budget to its timeout transition. *Sketch:* between tests the loop changes no relevant state; the test is evaluated atomically, so its value equals `Pred_code` in the state at the read point; `Pred_code` is extracted from the same exit branch. ∎ The extraction is re-derived by `kz proofcheck`; the lemma is still written in full before F1.

**L2, L3.** L2 is stated as in [A §4.2]. **L3** (mover reduction) is stated with the conditions of §4.10 item 5: right-mover acquire, left-mover release, both-mover protected accesses, at most one non-mover, release on every path, no nested blocking acquire. Until proved in the paper they are listed as assumptions, switchable per instance, and ablated (§8.4). The reduction theorem's citation is an open literature check.

**L-PRED (prediction).** Every `predicted` witness is a linearization of Pred(π) that passed the A-COMM check. Under A4, A7 and Φ's primitive semantics it is feasible for every program that produces π **and** whose unobserved shared reads in the cone are covered by A-COMM (maximal causal models, RV 2012; RVPredict PLDI 2014 [V-abs], whose guarantees concern traces that contain the shared accesses). A `predicted-modelled` witness has no such guarantee and ranks with model evidence; a `predicted-extrapolated` witness is additionally relative to A-SKEL for the extrapolated loop (fixes [X1-M4]).

**L-LOOP (termination).** Under fixed (B, Φ, tool), the loop's comparable state (§4.14) lives in a finite lattice, every non-final iteration moves strictly up or stops, and a suspension ends the loop. A Φ edit starts a new loop. ∎

### 6.3 Guarantee statements

- **G1 (census over-approximation, soundy).** For B, Φ, M and A, every contract use reachable in `Sk^cert` yields an instance with a class or `unknown(reason)` per analysed (σ, M). `inactive` is issued only from `Sk^cert`. Instances outside the closure are counted and listed. Claim template C-1 of [R05 §5.2].
- **G2 (certified verdicts, per M).** A fixed-order or protected verdict with realization `certificate` at level M holds for every in-scope execution in `Exec(B,σ_U,M)` under A, by L-FO / L-PR; by L-CONT it holds under σ_U∞ and σ_R1 at the same M and b. **It holds under σ_E only if M = M_impl, or if M = M_api and the cone contains no M_impl-relevant operation (L-MLEVEL) or the M_impl verdict agrees.** Otherwise it is a claim relative to the OSAL API contract. Each certificate passed `kz proofcheck`, names its view (`Sk^cert`) and M, and lists its trusted entries. `kz_verdict` stores per-M results.
- **G3 (observed violations).** An "observed violation" is reported only when, in the recorded trace, the state at some conjunct's LP computed from HB-ordered lifecycle LPs is outside Req, **and** the program's own outcome code agrees (A2, A6, O4). No claim that the contract is the "right" requirement; its provenance is shown.
- **G4 (predictive soundness relative to the trace model).** Every `predicted` witness is in Pred(π) and passed A-COMM (L-PRED); `predicted-modelled` and `predicted-extrapolated` witnesses are model evidence. Prediction is not maximal; "not predictable" is never "protected".
- **G5 (bounded model classification).** Within the closed model, σ, M and b, `exh` classes hold exactly (L-DPOR). `dpor` classes hold if T-JACM holds and are labelled **unverified** until then. C-2 of [R05 §5.2]. Fidelity evidence: O1, O2, O6, O8.
- **G6 (confirmation).** A `realized` verdict was reproduced on the gcc-built binary of B under the declared control, with the contract oracle evaluated on the confirmation trace, its naturalness label, and its perturbation ratio (C-3 of [R05 §5.2]; precedents RacePro, DESCRY, CTrigger [V-abs]). Realized reports contain no false alarm relative to the recorder's oracle. A σ_E class needs natural witnesses on both sides.
- **G7 (PCT, if D3 is approved).** Each F1 run finds a depth-d bug, relative to the gated scheduling points, with probability at least `1/(n·k^(d−1))`, with n and k measured (C-4 of [R05 §5.2]).
- **G8 (loop termination and reproducibility).** Under fixed (B, Φ, tool) the loop terminates (L-LOOP). Re-running from stored artifacts reproduces the verdict store byte for byte, because every budget is a deterministic work-unit count.
- **G9 (inconsistency surfacing).** Every contradiction between layers (O1–O9, CC) is reported and suspends or downgrades the rule or model involved; a suspension ends the loop for that Φ version. None is resolved silently.
- **G10 (transparency of trusted facts).** Every certificate lists every writer entry it relied on, with its transition and key statuses, every once fact, every `derives` fact, its extracted predicates, and every `DECLARED_TRUSTED` entry (A-FRESH, A-OWNER, relational guards). The census reports the number of conjuncts with T-OBS disabled.

### 6.4 Not claimed

1. "Sound", "complete", "detects all races" or "no false positives", without the qualifiers above ([R06 §6.3 item 1]).
2. "Safe" or "no violation exists" from the absence of an alarm; from a bounded exploration beyond its bound; from prediction or prefilter silence; or from a model whose obligations failed.
3. **"Timing-protected in σ_E"** or any σ_E class from the absence of bad runs. Only "no σ_E bad witness in n of N runs (pool key); σ_E class unknown" ([X1-M11]).
4. "Fixed-order" or "deterministic" from repeated failing runs; only from a σ- and M-stated certificate or exhaustive bounded exploration ([R06 S9]).
5. "Fixed-order" from a trace-local robust chain (VC_R) without a static certificate ([J1] §8).
6. Universal verdicts from the h-cut exploration view without h in b ([X1-C1]).
7. σ_E validity of an M_api-only verdict whose cone is M_impl-relevant ([X1-C2]).
8. "Trace-sound" for a predicted witness that failed A-COMM ([X1-M4]).
9. "Exact within b" for `dpor` while T-JACM is unverified; "one execution per Mazurkiewicz class" at all ([X1-M9]).
10. A σ_E frequency or class from a σ_U prediction or model witness; a σ_E class from held, gated or timeout-exercised runs (only `σ_E(held)`); any priority-based verdict carried between σ_R1 and σ_E ([R06 §6.3 item 14]).
11. "No transfer across σ" (too weak), or σ_U-to-σ_E frequency transfer (too strong).
12. "R2 is a race": its evidence and proof shape make it a fixed-order dependency error ([R06 S6]). Nor that R2's certificate comes from a generic template alone: it uses the generic K-ID contract **and** the framework rule R19 (§7.2 refinement accounting, [X1-M8]).
13. "C3 or C5 reproduced on v7.0.1": their buggy code is absent there ([R06 §6.3 item 9]).
14. "Existing tools fail on C1–R2": every external process-level tool is `NOT_RUN`; NOT_RUN is never a false negative ([R06 S5]).
15. "MLIR improves detection or precision", unless G-MLIR shows a measured win.
16. Generality from the five design cases; "new bugs" for derived, role-derived, seeded, mutant or configuration-changed tests ([R06 §6.3 items 7–8]).
17. Flight-representative timing or overhead from Linux `SCHED_OTHER` in permissive mode ([R06 §6.3 item 10]); RT verdicts for RTEMS/VxWorks before their profiles exist.
18. Novelty of HB/vector clocks, DPOR, PCT, lockset, delay injection, predictive analysis, SMT encodings, points-to analysis, **key-conflict lock reversal** (RVPredict/GPredict), **late-registration loss** (InitRacer) or **predicted use-before-init** (ExceptioNULL) as mechanisms. Novelty is claimed only for the combination around the cFS semantics; the novelty of the G3 formalization is an open literature check ([R04 §8 item 5]; [X1-M12]).
19. Anything about F Prime or other frameworks beyond hypothesis H-F (§7.6).
20. Memory-model correctness of the ad hoc synchronization sites (assumed, A4).
21. Any R2 count as a Kairos result before Kairos's own recorder reproduces the per-use set ([R06 §6.3 item 13]); "24" for any basis other than one with ENV's version table.
22. "Lost packets" for rejected event requests: EVS rejects them before any packet is built ([R06 §6.3 item 15]).
23. OSAL F1 (lock leak), F7 (silent task loss), the timebase handshake loss or either App-record window as bugs before confirmation runs; derived confirmations are labelled derived.
24. "First process-race detector" ([R06 §6.3 item 11]).
25. A recall figure for real ordering bugs in general: the recall protocol measures detection of history-derived program mutants and pre/post-fix pairs, labelled synthetic (§8.5).

---

## 7. Openness

### 7.1 Discovery sources (the union)

Contracts are written per **API and resource kind**, never per case, and the census instantiates them at every context-qualified site in cFE core and the 15 bundled apps. Candidates come from the union of these sources ([J1-13]):

| # | Source | Mechanism | Output | Origin |
|---|---|---|---|---|
| 1 | Static census | every Φ template at every matching context-qualified site; key resolution by value analysis, header-MsgId analysis and table decoding | instances with a class or `unknown(reason)` | [A], [C] |
| 2 | Outcome mining | every not-ready outcome code observed at the API boundary (vocabulary from the extracted `\retval`/`@retval` lists) creates an instance even when no template names the API | `template: none` instances, triaged | [B §4.1] |
| 3 | Key-pair enumeration | for every resource key with lifecycle transitions and uses in different actors, the pairs Φ marks as conflicting | candidates | [B §4.1] |
| 4 | Retry/timeout-loop-derived contracts | every loop recognized by the poll/retry pattern around an operation implies a contract for it, with `Allowed = retry_until_ready(effective bound)`; provenance kind `code_pattern` | templates | [C §7.1] |
| 5 | Lifecycle symmetry | every kind with create and delete operations automatically gets K-REG, K-LIFE and K-GEN | templates | [C §7.1] |
| 6 | Context-lookup symmetry | every caller of a context-resolving helper (`GetAppRecordByContext`, `GetTaskRecordByContext`) gets a K-SELF instance with its own not-ready behaviour (§3.3) | templates | [X2-02] |
| 7 | API documentation | every `\par Assumptions` block (138 in cFE core API headers) is an annotation candidate; a human turns it into a Φ entry and the effort is counted. Examples: `cfe_evs.h:168–172` (K-ID); `cfe_es.h:400–403` (K-PHASE); `cfe_sb.h:449–464` (buffer lifetime) | templates | [R06 §5.3] |
| 8 | Code admissions | comments stating an ordering assumption or an acknowledged race: CF "start up race condition" (`cf_cfdp.c:1272–1278`); CF TOCTOU (`cf_cfdp_s.c:447–463`); the OSAL lock invariant (`osapi-idmap.c:384–385`); the PSP exception-ring race (`cfe_psp_exception.c:72–86`); ES's "informational" wake (`cfe_es_backgroundtask.c:254–257`); the EVS guard (`cfe_evs_utils.c:613–623`); the TBL lock rationale (`cfe_tbl_api.c:145–149`); TIME's unregistration comment (`cfe_time_tone.c:1322–1325`); the PSP restart comment "if problems occur (e.g. deadlock) eventually this timeout will expire" (`cfe_psp_support.c:91–98`) | templates | [A §7.1], [C §7.1] |
| 9 | Belief deviance | for each API, the fraction of call sites that test its status; deviants are ranked (e.g. `cfe_sb_task.c:138`, `cfe_tbl_task.c:196` ignore `GetAppID`'s status). Engler et al. SOSP 2001 [V-me]. **A ranking only, never a verdict** | probe priorities | all |
| 10 | Always-one-order pairs | pairs observed in only one order across runs become flip candidates for prediction (CloudRaid ESEC/FSE 2018 [V-abs]) | candidates | [B §7.2] |
| 11 | Per-app profiling under variant bases | one-app-at-a-time runs collect potential publish → use communications independent of boot order (Snowboard SOSP 2021 [V-abs]); candidates only; also a cross-check of static publisher sets | candidates | [A §7.3] |
| 12 | Watchpoint discovery | during a publication window, a write watchpoint on the field records stores by other actors (DataCollider OSDI 2010 [V-abs]); read discovery only as a declared experiment (RW watchpoints cost ≈34 µs per read) | candidates | [B §7.2] |
| 13 | Configuration variants | new bases B′ (§7.4) | per-B′ results | all |
| 14 | Held-out protocol | §7.5 | held-out results | [R06 §5.6] |

Program mutants and pre/post-fix pairs (§8.5) are an **evaluation** instrument, not a discovery source: they measure what the union of sources misses.

### 7.2 Contract governance and anti-circularity

- **Two readiness oracles.** Φ-declared effects and bindings (the working model) and framework validity predicates (an independent oracle: support locations statically, outcome codes dynamically). Agreement is checked by O7 statically and O4 dynamically ([J1-12]; [J1 X9]).
- **Provenance per Φ entry.** Kind (`api_doc`, `code_admission`, `code_pattern`, `issue`, `researcher`), `path:line`, sha256 of the cited text, author, authoring time, and which design cases the author had inspected for that API ([R06 §5.3 item 4]).
- **App-profile layer** (fixes [X2-13]). App-private protocols (e.g. FM's parent–child queue) are never encoded in engines. If they are modelled at all, they go into `profiles/apps/<app>/*.kzp` with the same provenance and authoring record, and their authoring cost is reported separately from the framework profile. Without such an entry, instances that depend on them are `unknown(unmodelled)`.
- **Freezes by hash** ([J3-M11]):
  - **F1**: Φ, extractor, engines, calibration table, lints **and all comparator implementations** (§8.3), frozen before the first evaluation run on the development cases;
  - **F2**: frozen before any held-out item is opened; **the mutation operators, the history query (keywords and date cut), the expected-verdict rule and the equivalent-mutant exclusion rule** (§8.5) are frozen before F2.
- **Template-only first.** The first evaluation run uses no case-specific entry. Later refinements are labelled `oracle refinement`, and the verdicts they change are reported separately ([R06 T2]).
- **Refinement accounting names framework rules** (fixes [X1-M8]). Every verdict lists the Φ entries its proof used, split into generic contracts, generic machinery and **framework rules authored with a case known**. R2's certificate is reported as "generic contract K-ID + framework rule R19 (CORE-SERIAL)": R19 binds the loop, its stratum and its terminal escape, while its predicate is extracted from the code and re-derived by the checker.
- **Honesty about blindness.** The authors already know R2 and the windows, so a template-only run on the development set is not blind; there, anti-circularity rests on header provenance and code-extracted predicates. Blindness is claimed only for held-out sources and for the recall mutants (whose expected verdicts come from fix texts, §8.5).
- **Authoring effort** (person-hours, number of entries, lines of Φ, per layer) is measured and reported ([R06 R4]).
- **Lints** ([A §9.3]): constants in tool code only as `KZ_REF(value, "path:line")`; no case identifiers or case-specific symbols (e.g. `CFE_SB_Global`, `sem_name`, `C1`) in engine code outside `profiles/` and `tests/`.

### 7.3 Candidates beyond the five cases

Every row comes from a generic template applied by the census, not from case knowledge. Each stays a **candidate** until the loop produces evidence. A confirmation that needs a constructed schedule is labelled derived. All expected verdicts are [I].

| Candidate | Template | Source facts | Expected first verdict [I] | Path to evidence |
|---|---|---|---|---|
| **Core-task App-record window** (EVS, ES, SB, TBL, TIME, and their children created in TaskInit, before `cfe_es_start.c:818`) | K-SELF.es (all seven callers), K-SPAWN.app-record | §3.3; [J1 §1.1]; [LED §3.4] | open: O-ordered (ES lock) → prediction/explorer; `schedule-dependent(σ_U, M_api and M_impl)` and also under σ_U∞ (only a 50 ms yield separates them); σ_R1[`P_sched^posix`]: [I] protected on one CPU (the startup thread outranks the tasks); σ_E: no bad witness expected in natural runs, so "no σ_E bad witness in n of N runs; σ_E class unknown" | lock-reversing prediction from default boots; derived F0 hold between `:810` returning and `:818` (D16) |
| **SB/TBL identity never published** (consequence of the window; status ignored at `cfe_sb_task.c:138`, `cfe_tbl_task.c:196`) | K-ID with shape ABSENT; chain root = the window instance | §0.5 | the post-publication SB-AppId uses (211–229 ordered per boot, [R2c §7]) and TBL identity uses: no protected certificate; schedule-dependent(σ_U), ABSENT, chained | as above; the K-ID oracle (`ILLEGAL_APP_ID` on every later SB event) |
| Script-app App-record window; `GetAppIDByName` success with `0xFFFFFFFF`; `RunLoop` false → zombie | K-SELF.es, K-REG.es | §3.3 | open: O-ordered → prediction; schedule-dependent(σ_U, σ_U∞); harms per chain | App. A.3 |
| OSAL lookup of a RESERVED record leaks the type lock (F1) | K-LOCKBAL (M_impl, by-name path) + K-REG.osal | `osapi-idmap.c:384–385` vs `:401–404` | implementation-defect candidate under M_impl if reachable; protected pairs where creation is certified earlier | explorer deadlock check; declared-experiment hold + wait-for oracle (App. A.4) |
| Silent task loss after the OSAL bounded wait (F7) | K-SPAWN.osal (M_impl) | `osapi-idmap.c:475–480`; `os-impl-idmap.c:138–170` | schedule-dependent under σ_U; under σ_U∞ still reachable through four broadcast wakes from unrelated task-table unlocks [I] — not timeout-dependent | hold the creator between thread creation and finalize until the child's preparation fails |
| **Timebase callback thread loses its handshake** | K-SPAWN.osal (M_impl) | `osapi-timebase.c:383–390` | open; harm `task_never_runs` (no 1 Hz) | hold `OS_TimeBaseCreate`'s creator inside the RESERVED window (declared experiment) |
| Cancellation while a program lock is held | K-LOCK.cancel | `cfe_es_api.c:1348, 1383`; [R01 §2.4] | open; needs a delete input (B′); not hidden by L3 (§4.10) | `B_restart` family |
| SB pipe name visible before the descriptor commits | K-REG.sb-pipe-name | `cfe_sb_api.c:153, 163, 190–194, 802–839` | open where a by-name lookup exists | observation; prediction |
| SB pipe deleted between route snapshot and put | K-LIFE.sb-put | `cfe_sb_priv.c:1195–1200` | open; needs a delete input | `B_restart`; `InternalErrorCounter` |
| TBL owner load vs ground load claim | K-ATOM.tbl-load | `cfe_tbl_api.c:305–309`; `cfe_tbl_task_cmds.c:423–431` | `inactive` in the default B (message provenance) | input variant B′ |
| TIME synch callback after its module is unloaded; two registrations on one index | K-LIFE.callback, K-ATOM.synchcb | `cfe_time_api.c:628–640`; `cfe_time_utils.c:1026–1034`; `cfe_es_apps.c:1525–1545` | open; restart family | `B_restart` |
| Post-OPERATIONAL burst: SB's subscription replay (triggered by SBN's commands) vs TO_LAB's 37 subscriptions | K-MODE + K-CAP + K-CAP.msglim | §3.9; [R03 §5.2]; [M] `SBNSubPipe` overflowed in 5 of 6 M1 runs ([BM §3.3]) | schedule-dependent(σ_E) if both natural outcomes are re-observed; LOSS-UNDER-ORDER | uncontrolled runs give σ_E witnesses; App. A.5 |
| SCH_LAB wakeups vs late subscribers | K-ROUTE | SCH_LAB's first send ≥ 1 s after its init; [M] margin 0.12–0.9 s ([R03 §6.3]) | schedule-dependent(σ_U), LOSS-UNDER-ORDER; "no σ_E bad witness in n of N runs" so far | prediction; a hold of the subscriber |
| K-PHASE: CF's retry window vs the soft startup barriers | K-PHASE (consumer side, level-indexed) + K-REG.osal | [R03 §2.6]; `cfe_es_start.c:210–228` | in a C1-derived B′: timeout-dependent (violation needs a soft-barrier timeout or retry exhaustion) | explorer under σ_U vs σ_U∞ |
| HS AppMon counts "name not found" as "no progress" | K-REG.es (harm unspecified) | `hs_monitors.c:69–160` ([R03 §2.10]) | `inactive` with default tables (all `NOACT`) via table decoding | table variant B′ |
| HS XCT task-name lookup | K-REG.osal<task> + K-LOCKBAL | `hs_cmds.c:148`; `cfe_es_api.c:716–717` | inactive with default tables [I] | table variant B′ |
| MD/MM symbol lookups of app symbols | K-REG.loader, global scope | `md_utils.c:214`; `mm_cmds.c:143`; `mm_utils.c:525` | provider `none` for app symbols (LOCAL); tolerated or fixed-order by Allowed | census; table variant |
| HS system-monitor device start fails | (outcome mining) | `hs_sysmon.c:50–72`; `psp_module_list.cmake:9` | provider `env-absent` | App. A.6 |

### 7.4 Configuration variants and scenario families

The dependency graph is configuration-driven: six apps subscribe from tables, several take names or symbols from tables, and the schedule decides who wakes when ([R03 §7]). Each variant is a new basis with its own manifest and provenance status, never pooled with the default B. Nothing found only in a variant is a "new bug" of the default configuration (D18):

- tables that activate latent paths (CF `sem_name` with a provider, public-code-derived from nasa/bp [LED §1a]; LC watchpoints/actionpoints; MD dwell symbols; HS AMT/EMT/XCT; CS tables);
- alternate schedule tables, including the wakeups absent from the default schedule ([R03 §6.2]);
- startup-script order; `MISSION_CORE_MODULES` order (a configuration-change experiment, never schedule evidence);
- the cpu2 target present in the build tree;
- effective queue depth: `msg_max` (D19), and **euid** (running as root removes the truncation, §1.3, so any root-run arm is a new B′);
- **reset type**: POWERON vs PROCESSOR with a preserved CDS, a volatile startup script on `/ram`, and restored critical tables (run sequences);
- inputs: restart/reload/delete (`B_restart`), table load/validate/activate, SB pipe info, the stop command (`B_stop`, K-TEARDOWN).

### 7.5 Held-out protocol

Adopted from [R06 §5.6]:
1. **Freeze F2** (§7.2) before any held-out item is opened.
2. **Held-out sources**, each counted separately and never added to "new real bugs":
   - (a) other cFS revisions (version transfer; the profile is re-bound by AST pattern);
   - (b) other configurations (cpu2; table variants);
   - (c) issues mined with a query fixed before F2 (keywords and date cut), excluding C1, C2, C3, C5 and R2;
   - (d) **the recall protocol's program mutants and pre/post-fix pairs** (§8.5), labelled synthetic;
   - (e) in-code admissions not used during design.
3. Per-class metrics with denominators (§8.5); template-only vs refined on the five design cases.
4. Who runs the issue and history queries, and when relative to F2, is PI decision D8.

### 7.6 Generalization hypotheses (stated, not claimed)

- **H-V (cFS versions).** Only Φ and the scheduling profile `P_sched` are version- and backend-specific; rules carry version guards. Test: with Φ_6.4.x, the 6.4.x backend's cited `P_sched` and unchanged engines, C3 at 6.4.1 should classify as fixed-order under σ_R1 and schedule-dependent under σ_U, and C5 at 6.4.1 as schedule-dependent ([R06 §4 T5]; needs D13). If an engine change is needed, H-V is refuted in its strong form. Without a cited `P_sched` for that backend, no σ_R1 claim is made.
- **H-C (configurations).** Verdicts change only through B. Tested by §7.4.
- **H-F (other flight frameworks).** A profile for another framework (e.g. F Prime) would need actor classes, kinds, operations with bindings and LPs, rules, contracts and configuration readers. F Prime's mechanisms were **not examined** in R01–R07, so nothing is claimed.

### 7.7 Openness indicators reported

Following [R06 §5.7]: contract kinds covered vs kinds found in the census; the share of instances discovered automatically (sources 1–6, 10–12) vs authored; the share of verdicts that needed a case-specific refinement or a framework rule authored with a case known; held-out and mutant results kept apart from design-case results; instances per variant B′, kept apart from the default B; recall per layer (§8.5).

---

## 8. Evaluation plan

### 8.1 Research questions

- **RQ1 Openness.** How many contract instances exist in cFE core and the 15 apps under the default B and its variants, per kind and RC? What share is decided by certificate, observation, prediction, exploration and confirmation, per (σ, M)? What share stays unknown, and why? What are the concrete/symbolic/⊤ key shares (esp. transmit sites)?
- **RQ2 Development cases.** With the template-only profile, what verdict records does Kairos produce, and which change with refinements? R2 at v7.0.1; C1 in a derived configuration; C2 role-derived; C3 and C5 at 6.4.1 (if D13 is approved); the two App-record windows (C5-shaped residuals at v7.0.1, derived).
- **RQ3 Held-out generalization.** On the frozen tool (F2), what does Kairos find in sources (a)–(e), with what sampled triaged precision, realized share and unknown share?
- **RQ4 Model fidelity.** O2 conformance on every trace and O8 on the sample; obligation failures per iteration; checker pass rate and certificate-mutation kill rate; T-OBS disablement, A-TYPE listing and `DECLARED_TRUSTED` counts; O9 refutations; anchor-audit coverage; closure resolution per rule; havoc and local-choice counts; summary-fallback counts ([J1-14]).
- **RQ5 Measured phenomena** ([A §8.1]; [J2-12]). Does Kairos classify the order-dependent losses measured in the default bundle (queue-full drops 4–49 per boot; `NoSubscribersCounter` 15–26; overflows of `SBNSubPipe`, `TO_LAB_TLM_PIPE` and `DS_CMD_PIPE`, [BM §0, §3.3]) at both capacity levels (queue depth and MsgLim)? Does it predict which pipes can overflow? Oracles: put outcomes and the four SB counters; `PeakQueueDepth` only as SB-accounted depth. Label: LOSS-UNDER-ORDER, harm unspecified.
- **RQ6 Value of each layer and of the loop**, scored on labelled instances (§8.4).
- **RQ7 Cost and perturbation.** The six costs; uninstrumented-observable shifts; per-witness perturbation ratios (§8.6).
- **RQ8 Comparison** with established ordering, prediction and observation models on the same labelled instances, with a common metric (§8.3).
- **RQ9 Recall.** On pre-registered program mutants and pre/post-fix pairs, what share does each layer detect, leave unknown (by reason), or miss, and how many certificates are contradicted (soundness failures)? (§8.5)

### 8.2 Subjects

- **Development set** (labels per [LED], [R06 S10]): R2 (current code; observed with gdb and with the retired recorder; 24 uses per boot, ES 20, EVS 4, in the basis with ENV's version table); C1 derived (public-code-derived); C2 role-derived (a constructed app pair, synthetic control); C3 and C5 at the 6.4.1 tarballs on the shared derived 6.4.x platform ([LED] X3; D13). C3 and R2 share a resource family and are not counted as independent ([LED §3.1]). **R2 is the only natural, reproduced case on the target revision, and it is fixed-order**; the schedule-dependent class has no natural ground truth on the target binary. That is why RQ9 exists ([X3-C1]).
- **Census subject**: the v7.0.1 bundle under the default B, plus the variants of §7.4.
- **Recall subjects** (§8.5): program mutants of v7.0.1 and pre/post-fix pairs, labelled synthetic.
- **Held-out**: §7.5 (a)–(e), each counted separately.

### 8.3 Baselines and reference detectors

Rules: original tool, extension and reimplementation are always distinguished; `NOT_RUN` is never a false negative ([R06 S5]); thread data-race tools are not baselines (the user's decision, [R06 S13]) and appear only as labelled reference points ([J3-M12]); the norm is to compare against the strongest runnable tool of the same family on the same subjects ([R05 §6.3] L446).

**Common metric** [D] (fixes [X3-M6]). For every labelled instance (development cases, recall mutants and pairs, the triage sample of §8.5), each comparator yields **alarm yes/no** on the instance. Comparators are compared with Kairos per instance with a paired test for binary outcomes; the test is chosen and **recorded as an assumption** before F1 ([POL] L17). Predictions of comparator outcomes are not written down in advance.

**Implementation discipline.** Every reimplemented comparator is frozen at F1, has golden tests taken from the worked examples of its original paper (where the paper is accessible; P-PAPERS), and records who implemented it. Every comparator that runs over Kairos's traces depends on Kairos's probe plan; this dependence is stated in every comparison, and the external reference points below do not have it.

| Comparator | Kind | Input | Notes |
|---|---|---|---|
| FastTrack-style HB (PLDI 2009 [V-abs]): lifecycle LPs as writes, uses as reads, lock edges | reimplementation, reference point | Kairos traces | unordered-pair detector on the same evidence |
| WCP- and SHB-style relations (PLDI 2017, OOPSLA 2018 [V-abs]) | reimplementation, reference point | Kairos traces | lock-order-preserving prediction |
| **MCM/GPredict-style lock-reversing predictor** (RV 2012, ICSE 2015 [V-abs]): key as conflict object, lock as atomicity, generic property = Req, **without** Φ outcome semantics, Allowed, capture points or certificates | reimplementation, **same-family baseline** (fixes [X1-M12]) | same cones | explicit equivalent search; SMT-LIB variant after P-Z3 |
| **Plain typestate runtime monitor**: the Φ automata checked on observed traces, no certificates, no prediction, no σ | reimplementation | Kairos traces | shows what certificates and prediction add (e.g. for R2) |
| **DefUse-style learned def-use invariant** (OOPSLA 2010 [V-abs]): invariants learned from N natural runs, violations flagged | reimplementation | Kairos traces | [I] expected to learn R2's wrong definition as normal; a contrast for reference-based contracts |
| DCatch-style framework HB without contracts, outcomes or conditional edges (ASPLOS 2017 [P]) | reimplementation | Kairos traces | the value of contracts and outcome semantics |
| RacePro-style syscall model (SOSP 2011 [V-abs]) over syscall tracepoints of the same runs | reimplementation of the observation model | tracepoints | the records found zero syscalls for an OSAL name create plus lookup ([R06 §6.2 C5]) |
| Uncontrolled repeated boots with SB/EVS counters | reference observation | runs | what counters reveal without order |
| gdb records ([R2g]) | reference observation | — | the reporter's method and its perturbation (ASLR disabled) |
| **valgrind helgrind and DRD** on the unmodified binary through `RUN_CFS_WRAP` | **external reference point, actually run** (fixes [X3-M6]); deviation: wrapped run command, as for gdb ([R2g] L11); P-VALGRIND | the binary | memory-level only; valgrind serializes threads, so schedules are not README-mode; expected to answer a different question ([R05 §6.3 (c)]) |
| ThreadSanitizer (gcc variant) | reference point, only if P-TSAN approves the rebuild deviation | rebuilt binary | memory-level races |
| Original process-level tools (RacePro, SimRacer, …) | `NOT_RUN` with recorded blockers ([R06 K6]) | — | — |
| Nidhugg | `NOT_RUN`; **blocker: an install decision** (`llvm-18-dev`; Nidhugg's README supports LLVM 8–19 and LLVM 18 is installed as runtime only, [R05 F11]; [R07] L126–127). After P-NIDHUGG: an optional cross-check of the explorer on the litmus programs that use only pthread create, join and mutex, on the same closed harnesses ([R05 §6.3] L452) | — | — |
| GenMC, CBMC | `NOT_RUN` (GenMC's README requires LLVM 19–22, not available; CBMC not installed) | — | — |

### 8.4 Ablations

One element is removed at a time, with the same engine and the same traces ([R06 S11]; [J1-14]; [J3-M8]). **Every ablation is scored on the labelled set** (development cases, recall mutants and pairs, the triage sample): per-instance correctness deltas against the full system, with a paired test and an effect size (test and effect size recorded as assumptions), plus the decided share, the unknown share by reason, the false candidates removed by O1–O9, and the cost change (fixes [X3-M8]).

1. **Static-only** (formerly "≈ A"): census + certificates (+ explorer if S7 passed); no traces. *Conditional on S7 for its explorer part.*
2. **Dynamic-only** (formerly "≈ B"): trace HB + prediction + confirmation; no census, no certificates.
3. **No feedback loop**: a single forward pass.
4. **No obligations** (O1–O9 off).
5. **σ_U∞ off**: no refinements.
6. **Robustness tags off**: no VC_R prefilter.
7. **M_api only** vs **M_impl**.
8. **Collapsed identities** (one identity per event).
9. **Atomic APIs** (no LPs; whole API at entry); **no capture points** (value conjuncts at the API's check).
10. **Conditional edges made unconditional** (soft barriers as must-edges).
11. **Implementation-level OSAL locks as synchronization**.
12. **No certificates first** (explore everything). *Conditional on S7.*
13. **No prediction**.
14. **Configuration closure off** (pointer analysis only); **message provenance off**.
15. **Looper restriction off** (thread program order between handlers).
16. **Contracts off** (pure HB race reporting).
17. **Writer sets from Φ only** vs **from S3a only** vs **S3a + S3b** (the value of two oracles and of points-to).
18. **L1, L2, L3 off**. *Conditional on S7 for L2 and L3.*
19. **`dpor` vs `exh`** on small cones. *Conditional on S7.*
20. **Syscall-only observation; memory-only HB**.
21. **Once and KEY-SEP off** (frontier only on single-spawn actors with no key separation): what the X1-M2/M3 machinery buys.
22. **A-COMM check off** (every prediction labelled `predicted`): how many witnesses change label, and how many of those are refuted on the binary.

For the loop ablation, it also reports keys and indirect targets resolved, unknowns removed and model gaps detected per iteration.

### 8.5 Metrics

**Recall protocol** [D] (fixes [X3-C1]); frozen before F2 (§7.2):
1. **Operators** ([R06 §5.6]; [R05 §6.2]): remove a wait; move a publication after a spawn; swap object-table entries; empty a retry bound (a single attempt, as at C1's buggy revision); drop a status check.
2. **History query** over the offline cFE (2047 commits, full history, [M] [X3, here]), OSAL and app histories: keywords and a date cut fixed before F2 (e.g. the 23 cFE subjects matching `race|startup sync|order` are a starting set whose relevance is untriaged, [X3 §1]; examples `6932f1fe` "Fix #950, race condition in control request", `58abd38d` "Fix #978, store task parameters in task record"). For each relevant fix whose code is present at the pin, the **inverse** of the fix is re-created on v7.0.1 as a mutant.
3. **Expected verdict** of each mutant: taken from the fix's issue text and commit (POL reference kinds 1–2, L7–8): the contract violated, the instance, and the expected direction (e.g. "a use can precede the publication").
4. **Scoring per mutant and per layer**: `detected` (a fixed-order or schedule-dependent class with the expected instance and a compatible shape); `unknown(reason)`; `missed` (protected, inactive, or no instance). Reported with **exact binomial confidence intervals**, per layer (static, trace, prediction, exploration, confirmation) and in total.
5. **Soundness failures**: a `protected` or `fixed-order` certificate on a mutant whose fix text documents the opposite behaviour is reported as a soundness failure (and, where possible, confirmed on the mutant binary). This is the program-level test of the certificates; the certificate-mutation kill rate (§4.5) only tests the checker.
6. **Equivalent mutants**: excluded only by a rule pre-registered before F2: the mutated site is `inactive(B)` with its proof, or the mutant's objects are byte-identical to the original's.
7. **Pre-fix/post-fix pairs** ([R05 §6.1 (iii)] L431) where fixes exist: the verdict must move in the direction the fix implies.
8. **Time to first detection**: for σ_E observation, the number of natural runs until the first bad witness per mutant; for confirmation, the number of hold attempts; distributions, not means ([R05 §6.5]).
9. **Build budget**: mutants are built incrementally in **one variant tree** (one full serial build ≈15 min, [R07] L423; one variant tree ≈0.8 GB, [R07] L420; 18 GB free, [R07 §1] and measured here); per mutant only the binaries, the compile DB and the hashes are archived. All mutants are labelled synthetic, never "new bugs".

**Triage** [D] (fixes [X3-M7]):
- **Reported** = class `fixed-order` or `schedule-dependent` with outcome label `violation` or `liveness`.
- Scale [M] [X3, here]: 1,208 `CFE_EVS_SendEvent` and 133 `CFE_SB_TransmitMsg`/`Subscribe*` call sites in `cfe/modules` and `apps` (tests and stubs excluded, link membership not filtered), before context qualification; thousands of instances are likely [I]. Triage therefore uses a **stratified random sample per (class, RC kind, deciding layer)**; the sample size is a **deviation** (finite rater time); exact CIs are reported.
- **Raters**: two, blinded to the producing layer, to the ablation, and to each other; rater identity and independence from the Φ authors are PI decision P-TRIAGE. The agreement statistic is recorded as an assumption before triage.
- **LOSS-UNDER-ORDER** verdicts are not rated: their agreement with the SB-counter and put-outcome oracle is reported instead. `not-ready-unjudged` verdicts are counted, not rated (their "correctness" is undefined by construction).
- **Rater effort** is reported with the other costs.

**Other metrics**:
- Instances per class, refinement, σ, M, provider, reach, evidence (both axes), RC kind and source (§7.1), always with denominators ([R06 R3]). `NOT_RUN`, `NOT_MEASURED` and `NOT_FROZEN` are never zero or "pass" ([R06 R2]).
- Decided share per layer and per (σ, M); unknown share by reason; **anytime curves** (decided share against work units) for prediction, exploration and O8.
- Confirmation: realized (by naturalness), refuted and inconclusive shares; gate-unreached share; holds per confirmation; perturbation ratios.
- Static coverage ([R06 T3]): entry functions resolved; indirect call sites resolved per rule; key shares; message-provenance folding counts.
- Fidelity: as RQ4.
- Cost: work units, wall time and peak memory per stage; the six costs separately; profile size and authoring hours.

### 8.6 Overhead and perturbation protocol

Groups ([J3-M6]; [A §5.8]; [B §5.6]):

| Group | Configuration | Separates |
|---|---|---|
| A | unmodified binary, no observer, **re-measured on `-fc-v114`** | reference |
| B | uprobes registered with empty BPF programs | trap cost |
| C′ | minimal R2 plan (R2 sites and identity bootstrap only) | cost of a narrow plan |
| C | full plan | recording cost |
| D | C plus holds, **analysed per hold plan** against C runs with the same plan hash (fixes [X3-M2]) | control effect |
| W | the F1 `--wrap` build with gates disabled (only if D3 is approved) | code-layout effect of the variant |

**Measures, split by what can observe them** (fixes [X3-M2]):
- **Uninstrumented-observable** (compare A against B, C′ and C): from the console, in cFE time, **CORE_READY → OPERATIONAL** ([R2c §4.4]: 103.2–104.6 ms across nine runs, a tight measure) and the **span of the 15 "Loading file … APP:" lines** ([BM] L278: 2.2–3.3 ms warm); per-app init offsets; the OPERATIONAL line's wall time; counters read at stop ([BM §2.3]): `PipeOverflowErrorCounter` (4–49 in six identical runs, low power), `NoSubscribersCounter` (15–26), `MsgLimitErrorCounter`, `InternalErrorCounter`; which pipes overflowed.
- **Recorder-only** (compare C′, C and D): the per-use R2 set (task, SB function, EID), probe hits per site, drops, observer CPU time and placement.

Distributions are compared, not means. **Per-witness perturbation ratios** (§4.13) are reported with every realized witness, with no threshold. Every σ_E frequency is labelled `σ_E(plan P)`; equivalence to unobserved runs is claimed only for the uninstrumented-observable marginals that were measured.

**Controls**:
- **strata**: cold (first run after a VM reboot) and warm are separate strata, never pooled ([BM]: 2570 ms vs 1613–1633 ms; 849 ms vs 2.2–3.3 ms load span);
- groups are run **interleaved**, not in blocks; the interleaving order is recorded as an **assumption** with its reason (the host is shared and drifts, [R07 §1]); no order is presented as from-reference;
- **normalization boots** after every abnormal exit, excluded from pools; Init checked against the canonical S0 by content hash (§4.13);
- observer CPU time and placement recorded in every run.

### 8.7 Statistics and reproducibility

- **N** (runs per group, per loop iteration, per σ_E frequency) is a **deviation** (fixes [X3-M9]): reason, finite resources; effect, bounded results reported with exact intervals or as `unknown(budget)`; necessity, no reference defines it. **The N for S1's characterisation is decided at S1** (decision D14 moved to S1); later Ns are decided before S9. Event-based stopping is preferred wherever it exists: exhaustion, fixed point, loop termination.
- **Statistical procedure**, recorded as an **assumption** (methodological literature is not a reference kind under [POL] L5–10): full distributions; Mann–Whitney U for times and counts; Fisher's exact test for incidence; a paired test for per-instance binary comparisons (§8.3); standardized effect sizes; exact binomial confidence intervals for proportions. Arcuri and Briand (ICSE 2011 [V-abs]) are cited as guidance; Klees et al.'s about 30 trials are known only from a secondary summary ([R05 §6.5], P) and are not used as a value.
- σ_E frequencies are pooled only over identical pool keys (B hash, plan hash, stratum).
- Every reported number can be regenerated from stored artifacts: the deterministic replay test of §4.14.

### 8.8 Threats to validity

- **Internal**: profile errors (certificates, checker, O3/O4/O7/O9); Clang/gcc differences (anchor audit); model fidelity (O1, O2, O6, O8); probe perturbation (§8.6, perturbation ratios); kernel drift and a shared host; persistent state across runs (normalization boots); `DECLARED_TRUSTED` entries (listed; O9); comparator quality depending on Kairos's own probe plan (stated; helgrind/DRD as an independent point).
- **External**: one platform (Linux `SCHED_OTHER`, 4 vCPU, permissive mode, `msg_max` 10, euid ≠ 0); one OSAL backend (POSIX); one bundle version; RT behaviour only in the model (σ_R1[P_sched]); derived cases for C1, C2, C3 and C5; recall measured on synthetic mutants.
- **Construct**: contracts are authored, so the oracle is only as good as its provenance (shown per verdict); harm is judged only from documented allowed outcomes; route and capacity losses are not violations without a must-deliver reference (LOSS-UNDER-ORDER); mutants are inverses of fixes, which may not represent unfixed bugs.
- **Conclusion**: few and partly dependent development cases; possible held-out scarcity; N, sample sizes and caps are deviations and are reported.

---

## 9. Implementation roadmap

### 9.1 Directory layout (to be created later; nothing created now)

```
kairos/
  CMakeLists.txt                     # find_package(LLVM, Clang) from the pinned LLVM 23 build 1053047a; MLIR only after G-MLIR
  include/kz/schema/kz_schema.h      # X-macro field lists: the only record-layout definition (C, _Static_assert)
  include/kz/core/*.hpp              # KEM types, Φ core expression AST, abstract-domain interface, component interfaces
  profiles/cfs-v7.0.1/*.kzp          # Φ: kinds, ops, effects+per-conjunct LPs, bindings, rules, contracts, constants
  profiles/apps/<app>/*.kzp          # optional app-profile layer (provenance, separate authoring cost)
  profiles/freeze/                   # F1/F2 freeze manifests (hashes), incl. comparators and recall protocol
  lib/core/      container I/O, Φ parser, one evaluator (3 domains), rule engine, typestate, vector clocks, verdicts
  lib/llvm/      basis, extract (AST facts, derives, S3a writers, loop-exit trees), link (IR call graph, DWARF, MC,
                 anchor audit, app-unit probe), skel (two views, provenance, once, predicate extraction), modref (S3b)
  lib/static/    MustBefore (per M), PWB/once, certificates, census, plan (joint debug-register allocator)
  lib/proofcheck/ independent checker (no code shared with lib/static; no LLVM) + certificate-mutation engine
  lib/trace/     decode, identity, HB (VC, VC_R, looper rule), per-conjunct LP intervals, routing, chains, mining
  lib/conform/   per-actor acceptance (O1/O2); global conformance O8 (on lib/predict's engine, from S5)
  lib/predict/   cone, A-COMM check, explicit search, prefilters, extrapolation, SMT-LIB2 emitter (optional)
  lib/explore/   model builder, exh and dpor engines, σ variants (σ_U, σ_U∞, σ_R1[P_sched]), monitor, BPOR, L3 checks
  lib/confirm/   hold plans, ptrace + debug-register holder, attach-at-clone, wait-for oracle, F1 controller, PCT,
                 normalization-boot driver, perturbation ratio
  lib/baselines/ comparator reimplementations (frozen at F1; golden tests from the papers' examples)
  probe/bpf/     kz_probe.bpf.c (one generic program per phase; cookie-indexed descriptors; clang-18 -target bpf)
  probe/loader/  raw bpf()/perf_event_open loader (hand-written helper prototypes, <elf.h>), drainer;
                 perf-sample backend; optional tracefs backend
  gate/          kz_wrap.c (C99, cFS flags; F1 only, after D3)
  tools/kz/        core-only CLI: trace|conform|predict|explore|proofcheck|observe|hold|gate|loop|report|dump
  tools/kz-static/ LLVM CLI: basis|extract|link|modref|skel|static|plan
  tests/unit  tests/golden (canonical dumps)  tests/litmus (gcc-built C99 programs on the real OSAL, expected
              classes per σ and M; incl. App. A.8)  tests/mutation (certificates)  tests/property (evaluator
              agreement, HB, containment, loop monotonicity, dpor vs exh)  tests/recall (mutant manifests)
              tests/cfs (integration)
  docs/          formal model, Φ reference (grammar + semantics), provenance rules, ADRs (§11), diagrams (§12)
```

### 9.2 Engineering discipline (how messy code is avoided)

- **Interfaces first.** `kz_schema.h`, the Φ grammar **and its semantics** (core expressions, bindings, patterns), the verdict record and the component interfaces are reviewed before any implementation (S0).
- **One library per component plus a thin CLI subcommand**; components communicate only through KZ containers. Two binaries (§2.2).
- **No generated code** ([R06 K8]): X-macros instead of a schema compiler; no TableGen/ODS unless G-MLIR passes; a generic data-driven BPF program; handwritten gate wrappers; the explorer **interprets** the model.
- **No case-specific code; no bare constants** (§7.2 lints).
- **Tests at six levels**: unit tests; golden tests with canonical dumps (every Φ rule has one); a **litmus suite** on the real OSAL with expected classes per σ and M (incl. App. A.8); **property tests** (evaluator agreement across the three domains; HB transitivity; containment on small random models; loop monotonicity; `dpor` vs `exh`); **certificate-mutation tests**; and the **recall suite** of program mutants (§8.5).
- LLVM conventions for code linked with LLVM (no exceptions; `LLVM_ENABLE_EH=OFF`, [R07 §2.1]).
- **Provenance in every artifact**: B, Φ, tool and parent hashes; LLVM `1053047a`; gcc 13.3.0; kernel; backend; fidelity class.
- **Deterministic outputs**: sorted, hashed, work-unit budgets; golden tests and replay are stable.

### 9.3 Stages and exit gates

Each stage is publishable on its own. The recorder stays narrow and model-driven (v0.1 §15; [J3-M5]; [J1-11]; [J2-15]). **The thin minimum viable core (MVC) is S0 + S1 + S2 + S3a** (fixes [X3-M5]). Effort estimates are **[I] planning guesses by the architect, not research conditions, not measured, and not used by any tool**; they exist so that the PI can cut scope.

| Stage | Deliverable | Exit gate (all items required) | Needs | Estimate [I] |
|---|---|---|---|---|
| **S0** | Formats and formal core: `kz_schema.h` (X-macros), KZ container, Φ grammar + **semantics** (core expressions, bindings, pattern match), one evaluator with three domains, verdict record, certificate format, lints, golden harness; Φ_cFS skeleton for K-ID, K-SELF, K-SPAWN, K-REG with provenance | containers round-trip with stable hashes and zeroed padding; the evaluator agreement property test passes; golden tests for the rule catalogue pass on synthetic traces; lints active; no cFS involved | — | 3–4 person-weeks; 6–9 kLOC |
| **S1** | F0 observer: perf-sample backend first (bring-up), then the raw `bpf()` loader with attach cookies and inode registration; entry-only lock tokens; identity join; calibration table; normalization-boot driver | **R2 re-observed by Kairos's own recorder on the unmodified binary**: the **per-use set** (task, SB function, EID) agrees with [R2g §2]'s table and with the expectation computed from B (24 = ES 20 + EVS 4 with ENV's version table; 11 without it); any mismatch is investigated and reported, never tuned away; backend recorded (fixes [X3-m8]); loss zero or accounted; R-a…R-e, BPF-body, cookie and perf-sample costs measured; inode registration on an app `.so` tested; group A re-measured on `-fc-v114` with strata; **N for S1 decided as a deviation (D14 moved here)**; groups A, B, C′ compared on uninstrumented-observable measures | D1; D14 (S1 part); P-LIBBPF only if the own loader fails | 5–7 pw; 6–9 kLOC (C + C++) |
| **S2** | Trace HB and dynamic contract checking: VC/VC_R with the looper rule, per-conjunct LP intervals incl. capture points, routing, outcome agreement (O4 power on HB-decided uses), chains, outcome mining, scope | every R2 use classified; every later use ordered, **including** those ordered only by the system-state wait ([R2c §4.3]); O3–O5, O9 pass on default traces; the 17 `SEND_NO_SUBS` uses chained to K-ROUTE | S1 | 3–5 pw; 5–7 kLOC |
| **S3a** | Static closure (incl. the app-unit link probe), S3a writer extraction with A-TYPE listings, `derives`, two skeleton views, message provenance, predicate extraction, once facts, MustBefore per M, census (K-ID, K-SELF, K-SPAWN, K-REG first, then all templates), certificates, `kz proofcheck`, certificate mutations, Clang→gcc anchor audit, O1/O2/O7 | **R2 FIXED_ORDER certificates for the ES and EVS uses accepted by `kz proofcheck`** at M_api and M_impl, with every trusted entry listed (App. A.1), including the early-return regression and the `:823` and once-fact mutations rejected; timebase lookups **protected outside teardown** (App. A.7); CF lookup `inactive` in the default B; `K-ATOM.tbl-load` `inactive` by message provenance; the core-task window and the SB/TBL ABSENT consequence present as open (or classified); litmus App. A.8 passes; O7 report with disablement counts; app-unit undefined-symbol report | S0; S1 for reach and O1/O2 | 8–12 pw; 12–18 kLOC (of which `kz proofcheck` 3–5) |
| **S3b** (conditional) | Andersen-style points-to over IR with a pinned pipeline | built only if the S3a O7 report shows `DISABLED` conjuncts blocking certificates; gain measured by ablation 17 | S3a | 4–8 pw |
| **S4** | Feedback loop with **O1–O7 and O9** (O8 deferred to S5) and reproducibility; **no predict or hold yet** | fixed point on the default B with per-iteration deltas and its run set; deterministic replay test passes byte for byte | S1–S3a | 2–3 pw; 2–3 kLOC |
| **S5** | Trace prediction (lock-reversing, A-COMM labels, prefilters, extrapolation labels) and **O8 on the same engine** | synthetic suites pass; a C1-derived trace yields a reversal witness; both App-record windows yield witnesses from default boots or are reported `unknown(budget)` with anytime curves; O8 runs on the pre-registered sample | S2 (+S3a for A-COMM) | 5–7 pw; 6–9 kLOC |
| **S6** | F0 confirmation (`kz hold`) | the App-record window probe executed with event release, reference watchdog, attach strategy and recorded perturbation ratio; result realized, refuted or inconclusive, labelled derived; holder cost, attach-at-clone cost and ptrace×uprobe interaction measured | S5; D16 | 3–5 pw; 3–5 kLOC |
| **S7** | Explorer (gated): `exh` first, then `dpor` | `dpor` vs `exh` agree on all small models; two-query classification on the development cones; O6; kill criterion applied and recorded | S3a | 6–10 pw; 8–12 kLOC |
| **S8** | G-MLIR gate (optional) | decision recorded with measurements on the stated criteria | D6 | 3–4 pw |
| **S9** | Freeze F1, then F2; evaluation incl. the recall protocol, baselines, triage | template-only runs recorded before refinements; §8 executed; held-out and recall items opened only after F2 | D8, D14, P-RECALL, P-TRIAGE | 8–12 pw (baselines ≈1 pw each [I]) |
| **S10** (PI-gated arms) | F1 `--wrap` gates and PCT; native σ_R1 arm; 6.4.x basis for H-V | as approved | D3, D4, D13 | 4–8 pw each |

**Claims available at each cut** [I]:

| Cut after | What the paper can claim |
|---|---|
| S0 + S1 | a zero-install F0 recorder for cFS on the unmodified gcc binary; R2's per-use set re-observed with measured costs and perturbation |
| + S2 | dynamic contract checking with per-conjunct LPs and handler-scoped HB; R2 classified per use; chains; LOSS-UNDER-ORDER counts with SB oracles |
| + S3a (**thin MVC**) | the census; R2 certified fixed-order (σ_U, M_api and M_impl) with an independent checker; inactive and provider verdicts from configuration closure and message provenance; recall of the static and trace layers on program mutants |
| + S4 | reproducible fixed points; model fidelity (O1–O7, O9) |
| + S5 | schedule-dependent witnesses (predicted / predicted-modelled) for the windows and C1-derived; recall of the prediction layer |
| + S6 | realized (held) witnesses with perturbation ratios |
| + S7 | bounded classes in M under b; σ_U∞ refinements beyond certificates |

**Cut list, in the order scope would be dropped** (lowest claim value first) [I]: S8 (MLIR) → S10 arms → S3b → S7 (explorer; refinements then come only from certificates and prediction) → O8 → S6 → S5. The thin MVC and the recall protocol for its layers are never cut.

### 9.4 What is built first

S0, because every later component depends on the shared formal core and the formats. Then S1, because ground truth must come from the new recorder before any analysis is trusted. S2 and S3a follow and can overlap: S3a needs only S0, plus S1's traces for reach and O1/O2. The publishable core (the R2 certificate, the census, the recorder, and recall for those layers) does not wait for prediction, confirmation or the explorer.

---

## 10. Risks and open decisions for the PI

### 10.1 Decisions requested

The full list, with options, references, a recommendation and what each decision blocks, is in **`PI_DECISIONS.md`**. Summary (v1 IDs kept; renamed or new decisions use `P-` names):

| Id | Decision | Recommendation (short) | Blocks |
|---|---|---|---|
| D1 | Root-side observer and holder hook in `run_cfs.sh` | approve | S1 |
| P-LIBBPF (ex-D2) | install libbpf/bpftool | no; own loader | nothing unless S1 fails |
| D3 | F1 `--wrap` gates, PCT, group W | defer until S6 | S10 PCT arm |
| D4 | native σ_R1 arm | model-only; a native arm would record OSAL's own RR/root-max choice and run as root (euid 0 → a new B′) | S10 σ_R1 arm |
| P-Z3 (ex-D5) | install Z3 | no | SMT cross-checks only |
| D6 | MLIR gate | defer to S8 | S8 |
| P-TSAN (ex-D7) | TSan rebuild as reference point | omit unless asked | that reference point |
| P-VALGRIND | run helgrind/DRD via `RUN_CFS_WRAP` | approve | that reference point (S9) |
| P-NIDHUGG | install `llvm-18-dev` for Nidhugg | defer to S7 review | explorer cross-check |
| P-PAPERS | access to primary papers (JACM DPOR; DCatch/CAFA rule tables; mover reduction; FG05 guarantee) | approve | G5 for `dpor`; L3 citation; golden tests |
| D8 / P-RECALL | held-out sources; history query, operators, expected-verdict and exclusion rules; who runs them | all five sources; queries frozen before F2 by a person who does not author Φ | S9 |
| P-TRIAGE | raters, blinding, sample sizes | two raters, at least one not a Φ author | S9 |
| D9–D13, D16–D19 | scope; memory-model stance; re-baselining; retired recorder; 6.4.x platform; App-record probes; stop method; variants; `msg_max` | as in `PI_DECISIONS.md` | various |
| D14 | N (now a deviation), **S1 part decided at S1** | decide at S1 | S1, S9 |
| D15 | work-unit caps (deviations) | set before S5/S7 | S5, S7 kill criterion |
| P-NAME | the tool's name | keep "Kairos" as a working label; check for collisions before publication | publication |
| P-TRACEFS | allow tracefs dynamic events (a host-state change) | no; perf-sample backend instead | nothing |
| P-STATS | pre-register the statistical procedure as an assumption | approve before F1 | S9 analysis |

### 10.2 Risks

| Risk | Likelihood / impact [I] | Mitigation | Retired by |
|---|---|---|---|
| S3a writer extraction too coarse: many conjuncts DISABLED by A-TYPE listings or escapes, so few certificates | medium / high | Φ effects with keys and guards; transition and key statuses; allocation records; S3b only if measured necessary; disablement counts reported | S3a O7 report |
| A `DECLARED_TRUSTED` entry (A-FRESH, A-OWNER, relational guards) is wrong | low / high | listed per certificate; O9 and O3/O4 refute; ablation 17 | S4 |
| Skeleton infidelity (havoc and star closure too coarse; inlining fallback) | medium / high | O1, O2 on every trace; O8 from S5; fallback counts; `unknown` on doubt | S4–S5 |
| Predicate extraction fails on a needed loop | medium / medium | recognized forms (a)–(c); else `unmodelled-sync`; R19's loop is form (b) | S3a |
| Message provenance too coarse (⊤ transmit keys) | medium / medium | header-MsgId analysis; table-packet decoder; key shares reported; RC5 partial until measured | S3a |
| Observation perturbation | medium / medium | entry-only tokens; R-e capture; calibration first; per-witness ratios; `σ_E(plan P)` labels | S1 |
| eBPF features not probed (cookies, ring buffer, task storage, stack capture) | medium / medium | perf-sample backend; P-LIBBPF | S1 |
| Persistent-state contamination across runs | high (by design of confirmations) / high | normalization boots; canonical Init hash; strata | S1, S6 |
| Predictor and explorer state explosion | high / medium | cones, L1–L3, key-level dependence, windows; work-unit caps with anytime curves; kill criterion | S5, S7 |
| `dpor` remains unverified (T-JACM status P) | medium / low | `exh` mode; label; P-PAPERS | S7 |
| Holds perturb other threads (binsem 2 s timedlock, timer callbacks, signals, attach at clone) | low / medium | gate-placement rules; joint allocator; watchdogs; `timeout-exercised` label; costs measured | S6 |
| Recall subjects scarce or unrepresentative | medium / medium | history query plus operators; pre/post-fix pairs; labelled synthetic; equivalent-mutant rule | S9 |
| Triage effort at census scale | high / medium | stratified sampling; oracle agreement for LOSS-UNDER-ORDER; rater effort reported | S9 |
| Profile authoring effort and errors | medium / medium | per-API templates; header extraction; loop-, symmetry- and context-derived templates; effort measured | S3a, S9 |
| Integration breadth (two binaries, ~18 subcommands, three interpreters) | medium / high | thin MVC; staged gates; cut list; one formal core | S3a |
| Route/capacity verdicts read as "bugs" | medium / reputational | LOSS-UNDER-ORDER; harm unspecified without a must-deliver reference | report review |
| 6.4.1 bases lack a compile DB | high / medium (H-V, RQ2 for C3/C5) | reconstruct from the 6.4.x make system, or code-reading evidence only (D13) | S10 |
| Kernel drift and host sharing | medium / medium | re-baseline; interleaved groups; strata; work-unit budgets for analyses | S1 |

---

## 11. Architecture decision records

Format: **Context**; **Decision**; **Alternatives**; **Consequences**. Status: *accepted for SPEC FINAL*. ADRs that depend on a PI decision say so; ADRs changed by the reviews name the issue.

**ADR-001 — C/C++ core and typed artifacts; no Python/JSON pipeline.** Context: the user discarded the Python/JSON prototype (CORD, [R06 K7]). Decision: all components in C++17 (C for BPF and gate objects); all artifacts in the typed KZ container; JSON appears only as CMake's input `compile_commands.json`. Alternatives: Python orchestration over C++ kernels; a schema compiler. Consequences: one toolchain (LLVM 23 + gcc 13.3 + clang-18 for BPF); review via `kz dump`; no generated code.

**ADR-002 — One formal core interpreted three ways.** Context: static, trace and model layers must agree ([J1], [J2], [J3]). Decision: one KEM type set, one Φ, **one evaluator with three abstract domains** (revised by [X3-M11]), and a concretization relation `t ⊒ e`. Alternatives: separate models per layer. Consequences: O1–O9 are possible; `libkz-core` stays LLVM-free; an agreement property test is mandatory.

**ADR-003 — Φ is an interpreted DSL with provenance and a specified semantics.** Context: framework knowledge must be versioned, cited and reusable (H-V); the three interpreters need one meaning per predicate ([X3-M11]). Decision: a hand-parsed language with three sublanguages fixed at S0: a typed core expression language over resource state (bounded quantifiers over declared kinds only), bindings from C fields to resources (checked by O7), and an AST-pattern language with a match semantics. Alternatives: C++ rule classes; MLIR ODS; quoted predicate strings (v1). Consequences: engines are framework-neutral; a Φ edit restarts the loop.

**ADR-004 — KZ container with X-macro schema, typed references and per-section versions.** Context: BPF, gate objects and C++ must share layouts; v1's layout had implicit padding and bare references ([X3-m6]). Decision: X-macro field lists expanded into struct, dump printer and offset asserts; explicit reserved fields; `sizeof` equals the sum of fields; typed references `(artifact sha256, section, index)`; Merkle parent hashes. Alternatives: schema compiler; JSON lines. Consequences: layout changes touch one list; hashes never depend on uninitialized bytes.

**ADR-005 — Static ordering is a MustBefore greatest fixpoint on the certificate view.** Context: v1 cut message loops at h while claiming b = ∞ ([X1-C1]); base cases and W_o recomputation were unstated ([X1-m2]). Decision: T-PO, T-SPAWN, T-OBS, T-TERM on `Sk^cert` (every unbounded loop star-closed); ∅ at every root; W_o recomputed per iteration with four checked exclusions (OWN-NOPATH, PWB, KEY-SEP); no lock rule. Alternatives: may-HB over templates (C); h-cut skeletons (v1). Consequences: sound but incomplete; loops cost precision (frontier nodes after loops only).

**ADR-006 — Certificates by PWB with once(τ), an independent checker, and program-level falsification.** Context: dominance fails on early returns ([J2-1]); PWB needs one instance per actor ([X1-M2]); v1's R2 certificate missed a writer ([X2-01]). Decision: FRONTIER and ONCE-PWB rules with proved once facts; MUST_KEY must-writers for PROTECTED; SCOPE exclusion; `kz proofcheck` re-derives everything except extraction; certificate mutations test the checker; program mutants test the analysis. Alternatives: `u ∈ MB(x)` (A); trace-local VC_R certificates (B). Consequences: R2 certifiable with listed trusted entries; once analysis and checker are extra code.

**ADR-007 — Writer sets from S3a AST extraction plus Φ effects, with key statuses and allocation records.** Context: v1 specified a from-scratch Andersen analysis over opaque-pointer IR inside the MVC ([X3-M4]); key semantics were may-only ([X1-M3]). Decision: field-based writer extraction at the AST level under A-TYPE with untyped-write and escape listings; out-pointer summaries; transition-aware filter; MUST_KEY / MAY_KEY / NO_KEY; allocation records with A-FRESH and A-OWNER as listed trusted facts refuted by O9; S3b points-to only if measured necessary. Alternatives: Andersen first (v1); declared effects only (C). Consequences: the MVC is smaller; ablation 17 measures what S3b would add.

**ADR-008 — Semantics family with per-M containment and well-defined refinements.** Context: v1's containment ignored M ([X1-C2]); σ_U∞ was ill-defined for exhausted retries and liveness ([X1-M5]); σ_R1 ignored OSAL's RT policy ([X2-07]). Decision: σ_U with exact retry bounds; σ_U∞ with patient retries (exhaustion at quiescence), strong fairness and good/bad/stall outcomes; σ_R1[P_sched] with cited profiles and blocking delays (A-ZT); `Exec(σ_E) ⊆ Exec(σ_U, M_impl)`; M_api as API-contract claims; refinements per σ with their own bounds. Alternatives: σ_U + σ_E only (A); v1's family. Consequences: two analyses per OSAL-touching cone; no σ_R1 verdicts for backends without a cited profile.

**ADR-009 — A linearization point per conjunct, with capture points.** Context: v1 evaluated value-carried conjuncts at the wrong LP ([X1-M1]). Decision: every conjunct declares its LP; value conjuncts use the capture point found by `derives`; T-DERIVE is the generic translation from a stamp parameter to `cell(loc)`; proofcheck re-checks `derives`. Alternatives: one LP per use (v1). Consequences: traces bracket capture points; O4 maps outcome codes per conjunct.

**ADR-010 — Two OSAL model levels with faithful M_impl waits.** Context: POSIX internals create windows, timed waits and a lock leak ([R01 §7 item 6]; [X2-03]). Decision: M_api atomic at LPs; M_impl projects OSAL shared + POSIX source onto Φ_L-1, with by-ID vs by-name token conversion and attempts woken by broadcast or timed wake. Labels portability-hazard, implementation-defect-candidate, portability-hazard-evidence. Alternatives: API level only; always M_impl. Consequences: litmus tests validate Φ_L-1.

**ADR-011 — A verdict record with orthogonal axes and no "safe".** Context: [R06] R1–R7, S1; v1 mixed causes with outcomes and realization with naturalness ([X1-m3], [X1-M11]). Decision: per-(σ, M) cells; provider and reach axes; scope; realization and naturalness axes; σ_E classes only from natural witnesses on both sides. Alternatives: a single severity ladder; v1's single evidence ranking. Consequences: reports are multi-axis; tests check that no path produces "safe" or "timing-protected".

**ADR-012 — Context-qualified templates with a summary fallback.** Context: SB event sites are shared by all callers; full inlining is exponential ([X1-m7]). Decision: templates `(actor instance, call string, site)`; per-(actor, function) summaries when the skeleton work-unit cap is reached, with the precision loss stated. Alternatives: site-only templates. Consequences: per-context certificates; fallback counts measured.

**ADR-013 — F0 observation with perf-PMU uprobes and an own raw loader; perf-sample bring-up.** Context: highest fidelity on the unmodified binary; no libbpf ([R07 §4.7]); tracefs changes global state ([X3-m2]). Decision: uprobes at entry+4 registered by inode before launch; one generic BPF program per phase; perf-sample backend for bring-up; tracefs optional with host-state rules. Alternatives: `--wrap` (F1); source probes (F2); gdb; libbpf. Consequences: S1 must probe cookies, ring buffers, task storage and stack capture.

**ADR-014 — Causality only from tokens and protocol keys.** Context: timestamps give no HB ([R04 §2.1]; [J2-4]). Decision: per-thread `tseq`; per-object `objseq`; descriptor/FIFO; outcome-implied reads-from; poll exit status. Alternatives: clock reconstruction as HB. Consequences: no take-return probes on hot sites.

**ADR-015 — Return values captured only where needed, by a mechanism chosen by measurement.** Context: post-call `mov` probes are likely in the 31.7 µs class ([J3 §2.3]); a cheaper emulated-branch option exists ([X3-m3]). Decision: capture only at contract-use and edge-deciding sites; R-a…R-e measured per class in S1; deferred reads where statically valid. Alternatives: caller-side probes everywhere. Consequences: S1 produces the calibration table before a full plan.

**ADR-016 — Prediction by explicit-state, lock-reversing search with an A-COMM check.** Context: the central cFS class needs lock reversal; no solver is installed; sparse traces weaken MCM/RVPredict guarantees ([X1-M4]). Decision: Pred(π) with the key as conflict object and the lock as atomicity; "an acquire needs the lock free"; A-COMM check per witness, else `predicted-modelled`; prefilters with re-validation; work-unit caps with anytime curves; SMT-LIB only after P-Z3. Alternatives: SMT-only; sync-preserving prediction only. Consequences: no install; the predictor's soundness is stated relative to A-COMM.

**ADR-017 — Explorer with a verified exhaustive mode and a conditionally verified DPOR mode; gated.** Context: a test is not a proof; the disabling theorem is status P ([X1-M9]); v1's L3 condition was insufficient ([X1-M10]). Decision: `exh` (stateful exhaustive search) and `dpor` (source-DPOR + sleep sets, labelled unverified until T-JACM is checked); L3 as a mover reduction with statically checked conditions; differential test as validation; no optimality claim; stage S7 with a kill criterion. Alternatives: optimal DPOR claimed exact; no explorer. Consequences: the explorer may end as an ablation; refinements then come from certificates and prediction.

**ADR-018 — MLIR behind gate G-MLIR.** Unchanged from v1: plain C++ skeletons and core; G-MLIR at S8 with criteria (verdict equality, provenance completeness, code size, runtime); no ODS before the gate passes.

**ADR-019 — Version-robust site binding.** Context: line-number probes break across revisions ([J2-9]); same-named local copies exist ([X3-m3]). Decision: AST patterns with a match semantics, resolved through each basis's gcc DWARF (compile unit for local copies) and MC; version guards; anchor audit. Consequences: unresolved patterns listed.

**ADR-020 — Confirmation by F0 holds with explicit safety rules and normalization.** Context: constructed schedules must be feasible, safe and honestly labelled ([J2-10]; [J3-M10]); abnormal exits contaminate later runs ([X3-M3]); debug registers are shared ([X3-m1]). Decision: ptrace with debug-register breakpoints; empty program-lock set; placement rules; event release; reference watchdogs incl. `CFE_PSP_RESTART_DELAY`; wait-for oracle; joint debug-register allocator; attach at clone measured; a normalization boot after every abnormal exit; naturalness labels and perturbation ratios. Alternatives: `--wrap` gates by default; fixed sleeps (rejected by [POL] L22). Consequences: at most 4 debug registers per thread for watchpoints plus gates.

**ADR-021 — Consistency obligations O1–O9 and the certificate check.** Context: authored rules need a "how do you know the model is right" answer ([J1 §6]). Decision: per-actor acceptance; outcome agreement with power reported on HB-decided uses; writer-set agreement incl. A-TYPE; allocation/ownership refutation (O9); global conformance from S5; contradictions suspend rules and end the loop. Consequences: obligation failures are a fidelity metric.

**ADR-022 — A feedback loop with reproducibility.** Context: v1's fixed-point test included counts and never converged ([X1-m8]); budgets were wall-clock ([X3-M10]). Decision: per-iteration records; a fixed-point test over the comparable state only; an explicit lattice; suspension ends the loop; work-unit budgets; replay byte for byte; staged availability of components. Consequences: reproducible numbers.

**ADR-023 — Clang/LLVM for analysis only; the gcc binary is what runs.** Unchanged: Clang facts and IR for analysis; DWARF/MC anchoring to the gcc binary; two diagnostic-only flag adaptations recorded in B; IR used for the call graph only (pinned pipeline if ever used for writes).

**ADR-024 — Openness through a union of discovery sources, with governance.** Context: five cases cover 3 of 15 contract kinds ([R06 §5.1]). Decision: the 14 sources of §7.1; freezes F1 (incl. comparators) and F2 (incl. the recall protocol); template-only first; refinement accounting that names framework rules; an app-profile layer. Consequences: authoring effort is real and is measured.

**ADR-025 — Baselines with a common metric, frozen code and an external reference point.** Context: v1's comparators were all self-reimplemented with no metric, and the strongest same-family comparators were missing ([X3-M6]; [X1-M12]). Decision: alarm yes/no per labelled instance with paired tests; comparators frozen at F1 with golden tests from their papers; an MCM/GPredict-style lock-reversing predictor without Φ outcomes, a typestate monitor and a DefUse-style learner added; helgrind/DRD run through `RUN_CFS_WRAP`; the Nidhugg blocker stated as an install decision. Consequences: comparisons answer "what each model sees on the same instances", with Kairos-plan dependence stated.

**ADR-026 — A staged build with a thin MVC, estimates and a cut list.** Context: v1's MVC included the whole L1 layer and Andersen; stages had dependency inversions ([X3-M5]). Decision: thin MVC = S0 + S1 + S2 + S3a; S4 without O8, predict and hold; O8 on S5's engine; D14 at S1; [I] estimates and a cut list. Consequences: the explorer, S3b and F1 can be cut without losing the core contribution.

**ADR-027 — Two skeleton views.** Context: [X1-C1]. Decision: `Sk^cert` (star-closed, for every universal claim and `inactive`) and `Sk^h` (h-cut, for exploration only, h in b); proofcheck rejects `Sk^h`/`TRUNC` steps; litmus App. A.8. Consequences: universal verdicts are horizon-free; exploration results are bounded.

**ADR-028 — Teardown as a scope, modelled as on pc-linux.** Context: v1 modelled restart as non-terminal with `OS_DeleteAllObjects` concurrent with tasks, defeating the calibration certificates ([X1-M6]; [X2-04]). Decision: `es.shutdown` kind; requester terminal after `CFE_PSP_RESTART_DELAY`; teardown only on the PSP main thread after `CFE_ES_Main` returns; contracts scoped "before the shutdown request"; K-TEARDOWN family; SCOPE exclusion in certificates. Consequences: default-B certificates are stated outside teardown; every experiment's stop is a teardown and is reported separately.

**ADR-029 — Message provenance and the looper rule.** Context: [X1-M7]. Decision: `Prov(p) = (Routes_B(p) ∩ Tx_B) ∪ Input_B(p)` as a least fixpoint over handler reachability, folding dispatch in both views; no between-handler HB edge; FIFO only as a prediction constraint. Consequences: command-only paths are `inactive` without input; shapes follow CAFA/DCatch.

**ADR-030 — Code-extracted await predicates.** Context: v1's T-OBS rested on authored predicate strings ([X1-M8]). Decision: poll loops recognized by pattern; predicates extracted by symbolic evaluation of the exit branch (three recognized forms); re-derived by proofcheck; L1 preconditions checked; Φ binds only names, strata and escapes. Consequences: R2's proof no longer depends on an authored predicate; unextractable loops are `unmodelled-sync`.

**ADR-031 — Generic K-SELF contract and branch-complete chains.** Context: [X2-02]. Decision: `es.self(exec) = LIVE` instantiated at all seven context-resolving callers with per-caller outcomes and harms; consequence chains record every branch, labelled [I] until run. Consequences: new harms (`app_zombie`, `service_task_absent`, `process_abort`, `panic`).

**ADR-032 — Two-phase SB transmit and two-level capacity.** Context: [X2-06], [X2-05], [X2-09]. Decision: route_lookup LP under the lock plus one put LP per destination; K-CAP and K-CAP.msglim; four counters as oracles, `PeakQueueDepth` labelled SB-accounted; header-carried MsgIds with encode/decode summaries and a table-packet decoder; K-MODE in SB's actor via R7 on SB's command pipe. Consequences: RC5 stays partial until key shares are measured.

**ADR-033 — Basis closure covers reset type, script selection, euid, CWD and CDS images; canonical Init.** Context: [X2-08], [X3-M3]. Decision: these items in B with sources; the script-selection rule evaluated in `basis`; `cap_B` conditioned on euid; A11 split by reset type; content-only Init hash; strata; normalization boots. Consequences: root-run arms are new bases.

**ADR-034 — Deterministic work-unit budgets and POL-compliant labels.** Context: [X3-M9], [X3-M10]. Decision: every analysis budget is a work-unit count with a deterministic search order; wall time and memory are costs only; PI-chosen N, caps and sample sizes are deviations; event-based stopping preferred; anytime curves; statistics as an assumption. Consequences: byte-exact replay; honest bounds.

**ADR-035 — Recall measured on program mutants and pre/post-fix pairs.** Context: v1 measured nothing about misses; one natural case ([X3-C1]). Decision: §8.5 protocol frozen before F2; soundness failures counted; equivalent-mutant rule pre-registered; one incremental variant tree. Consequences: RQ9; mutants labelled synthetic.

**ADR-036 — Attach cookies and inode registration.** Context: IP-keyed descriptors break under ASLR; the `.so` fallback perturbed the phase under study ([X3-M1]; [X3-m4]). Decision: site id as attach cookie (per-site perf links or `uprobe_multi`), registration by inode before launch, verifier-conscious generic program, hand-written helper prototypes and `<elf.h>`. Consequences: no per-module map filling; the hold fallback is excluded from pools if ever used.

**ADR-037 — Corrected prior-work positioning.** Context: [X1-M12]. Decision: credit InitRacer (late registration), RVPredict/GPredict (key-conflict lock reversal), ExceptioNULL (predicted use-before-init) and DefUse; record G3 novelty as an open literature check; claim only the combination. Consequences: §1.11 and §6.4 item 18.

**ADR-038 — Triage by stratified, blinded sampling with oracle agreement for losses.** Context: [X3-M7]. Decision: "reported" defined; stratified random sample per (class, RC kind, deciding layer); blinded raters; LOSS-UNDER-ORDER checked against the SB oracle; rater effort as a cost. Consequences: precision with exact CIs instead of full triage.

---

## 12. Diagrams to draw

Each diagram lists the nodes and edges it must show. Edge labels follow a colon. Dashed edges are PI-gated or feedback paths.

### D1 — System context
- **Nodes**: PI/Researcher; `kz` and `kz-static` (root session); pinned cFS tree (read-only); `build-native_std` (compile DB, `link.txt`, generated files, `.tbl`, gcc `core-cpu1` and app `.so` with DWARF); LLVM 23 build `1053047a`; system clang-18 (BPF backend); Linux kernel `6.18.44-fc-v114` (perf uprobe PMU with attach cookies, `bpf()`, perf breakpoint PMU, ptrace, tracepoints); run harness `env/run_cfs.sh` with the root-side hook (D1) and `RUN_CFS_WRAP` (gdb, valgrind); cFS process `core-cpu1` (README mode, user `ubuntu`, 27 threads); persistent state (`/dev/shm/osal:RAM`, `EEPROM.DAT`, SysV segments, key files); research records; KZ artifact store; dashed PI-gated nodes: Z3 (P-Z3), libbpf (P-LIBBPF), `llvm-18-dev` + Nidhugg (P-NIDHUGG), F1 `--wrap` variant (D3), MLIR (D6), TSan variant (P-TSAN).
- **Edges**: PI → kz: Φ edits, decisions, freezes F1/F2; cFS tree → kz-static: sources; build tree → kz-static: compile DB, link, tables, binaries + DWARF; LLVM 23 → kz-static: libraries; clang-18 → kz: BPF object; harness → cFS process: launch as `ubuntu`, stop method (D17), normalization boots; kz → kernel: perf_event_open (uprobes by inode, breakpoints), `bpf()`, ptrace; kernel → cFS process: traps, holds; kernel → kz: ring buffers / perf samples; cFS ↔ persistent state; kz → persistent state: S0 snapshot (read-only); records → kz: reference values, policy; kz → store → PI: reports; dashed: the gated nodes → kz.

### D2 — Components and data flow
- **Nodes (components)**: `kz-static` {basis, extract, link, modref (dashed, S3b), skel, static, plan}; `kz` {proofcheck, observe, trace, conform, predict, explore (dashed border, S7), hold, gate (dashed, D3), loop, report, dump}; `libkz-core` (shared).
- **Nodes (artifacts)**: `basis.kz`, `profile.kzp/.kz`, `facts.kz` (incl. derives, writers, loop-exit trees), `anchors.kz`, `skel.kz` (cert view, exploration view, once, provenance), `census.kz`, `plan.kz`, `run-*.kzt`, `hb-*.kz`, `witness-*.kz`, `holdplan-*.kz`, `verdicts.kz`, `loop-*.kz`.
- **Edges**: basis → `basis.kz` → extract, link, skel; extract → `facts.kz`; link → `anchors.kz`; facts + anchors + profile → skel → `skel.kz`; skel + facts + profile → static → `census.kz` (annotate O7); census → proofcheck: certificates; proofcheck → census: accept/reject (CC); census + anchors → plan → `plan.kz`; plan → observe → `run-*.kzt`; run → trace → `hb-*.kz` (O3–O5, O9); run + skel → conform (O1, O2; O8 from S5); hb + facts → predict (A-COMM) → witness; skel + census → explore → witness (O6); witness → holdplan → hold/gate → `run-*.kzt`; analyses → `verdicts.kz` → report; loop orchestrates; dashed feedback F1 (keys, targets, opened script) → basis/skel; F2 (model gaps) → skel; F3 confirmation runs → trace; F4 obligation failures → profile (human review); `libkz-core` linked by everything except BPF.

### D3 — Analysis workflow (per contract instance and (σ, M) cell)
- **Decision nodes**: "template matched (certificate view)"; "key resolved?"; "reachable under B + provenance?"; "writer sets complete? (A-TYPE, O7)"; "provider: none / env-absent / present"; "FIXED_ORDER certificate? (once, frontier/once-PWB, derives)"; "PROTECTED certificate? (MUST_KEY, SCOPE)"; "kz proofcheck"; "open: probe obligations"; "observation"; "routing" (conforming / wrong-way / O-ordered / unordered); "prediction (A-COMM label)"; "explorer (exh | dpor; σ_U, σ_U∞, σ_R1[P_sched]; exploration view, h in b)"; "confirmation (naturalness, perturbation ratio)"; "combine per cell (§4.12)"; "transfer to σ_E (M_impl or M agreement)"; "obligation failure".
- **Terminal nodes**: `inactive`, `fixed-order`, `protected`/`tolerated`, `schedule-dependent`/`timeout-dependent`/`timeout-masked-liveness`, `unknown(reason)`; axes provider, reach, scope; `σ_E(held)`.
- **Edges** as in v1, plus: provider none → class by badness (not a separate terminal); M_api-only certificate contradicted by binary → portability-hazard-evidence; σ_E cell → `unknown(model_level)` when transfer rule 2 fails; suspension → loop end.

### D4 — Research workflow
- **Nodes**: Φ authoring (provenance, authoring records, refinement accounting); stages S0 → S1 → S2 → S3a (thin MVC) → S4 → S5 (+O8) → S6 → S7 (kill criterion) → S8; S3b (dashed, conditional); freeze F1 (incl. comparators); template-only development runs; labelled refinements; freeze F2 (incl. recall protocol); held-out sources (a)–(e); recall mutants and pre/post-fix pairs; sampled blinded triage; ablations (22, scored on labels); baselines (common metric) and helgrind/DRD; perturbation groups A, B, C′, C, D (per plan), W; normalization boots; statistics (assumptions; N and caps as deviations); claims table; PI decisions; deviation records.
- **Edges**: the stage sequence; S3a → F1 → template-only → refinements → F2 → held-out and recall; S1 → perturbation groups; stages → ablations, baselines, triage → statistics → claims table; decisions → the stages they gate (D1 → S1; D14 → S1, S9; D15 → S5/S7; D16 → S6; P-RECALL, P-TRIAGE → S9; D3, D4, D13 → S10); every deviation → deviation record.

### D5 — cFS layer mapping
- **Left (cFS)**: PSP (main thread → `OS_Application_Run`, `soft_timebase`/`cFS-Master`, reset decision and boot record, persistence areas, `CFE_PSP_Panic` (terminal), `CFE_PSP_Restart` (terminal for the requester; ShutdownReq; teardown on the main thread), exception ring); OSAL (idmap with by-ID/by-name tokens and timed waits, tasks, mutex (PI), binsem, countsem, queues (`msg_max`, euid), timebase/timers, modules/symbols with scope, files); cFE ES (startup thread, `CreateObjects` (EarlyInit; core loop: reserve `:803`, start `:810`, publish `:818`, clear `:823`, sync `:835`, panic), script selection and `StartApplications`, `AppCreate` (reserve `:821`, publish `:869`, free `:893`), trampoline, App/Task/Lib records, `SystemState`, `WaitForSystemState`, `MainTaskSyncDelay`, background task, `CleanUpApp` (core-table cleanup order)); EVS (register with memset, `SendEventWithAppID` two conjuncts, own-ID guard); SB (pipes, routes, route_lookup and puts, MsgLim, receive, subscription reporting in SB's task, header MsgIds, recursion guard); TBL; TIME (1 Hz chain, seqlock, synch slots by index); FS background; CDS; apps with tables and the selected script.
- **Right (Φ)**: kinds (incl. `es.self`, `cell(loc)`, `sb.msglim`, `es.shutdown`, `loader.sym`); operations with per-conjunct LPs; rules R1–R19, SR, P-*, RF; contracts (K-REG, K-ID, K-SELF, K-SPAWN, K-PHASE, K-ROUTE, K-CAP, K-CAP.msglim, K-MODE, K-TBL, K-PERSIST, K-HANDOFF, K-LIFE, K-GEN, K-LOCKBAL, K-LOCK.cancel, K-ATOM, K-TEARDOWN); actor classes; model levels; P_sched profiles.
- **Edges** (examples): `:835` + panic → R19 (S, obs-fatal, predicate from code); `:803/:818/:823` → `es.app` RESERVED/LIVE/clear + allocation record ρ; `cfe_sb_task.c:138` → `cell(CFE_SB_Global.AppId)` + K-ID capture point; `OS_QueuePut` full → K-CAP; MsgLim → K-CAP.msglim; `osapi-idmap.c:384–385` vs `:401–404` → K-LOCKBAL (M_impl, by-name); `cfe_psp_support.c:87` → `es.shutdown` SHUTDOWN_REQ.

### D6 — Verdict state machine (per cell)
- **States**: `candidate`, `open`; `inactive`; `protected[cert]`, `protected[model b]`, `tolerated`; `fixed-order[cert]`, `fixed-order[model b]`; `schedule-dependent` with refinements; `unknown(not_reached | model_gap | bound | budget | key_unresolved | loss | unmodelled | writer_incomplete | horizon | conformance | model_fault | model_level)`.
- **Parallel regions**: evidence realization (`none` → `predicted-modelled` | `predicted-extrapolated` | `model(b)` | `predicted` → `realized`); naturalness (`natural` | `held-F0` | `gated-F1` | `timeout-exercised`; `derived`); provider; reach (`assumed` → `proved`); scope.
- **Transitions**: candidate → inactive / fixed-order[cert] / protected[cert] (static, per M); candidate → open; open → schedule-dependent (bad or stall + good witness); open → fixed-order/protected[model b]; schedule-dependent → refinement (derived from per-σ cells); open → unknown(budget); any → unknown(model_fault) (O3/O4/O9; loop ends); M_api cert + contradicting observation → portability-hazard-evidence (no state change at M_api); unknown(model_gap | model_fault) → candidate (new Φ hash; new loop); evidence refuted → model-fault record; protected → tolerated.
- **Invariant**: no state named "safe" or "timing-protected"; no transition from "no alarm" to a universal class; σ_E universal states only via transfer rule 2.

### D7 — Observation pipeline
- **Nodes**: `plan.kz` (sites, cookies, decoders, cost classes, debug-register allocation); loader (raw `bpf()`/`perf_event_open`, `<elf.h>`) | perf-sample backend | optional tracefs instance; uprobe registrations by (inode, offset) on `core-cpu1` and app `.so` (before launch); per-task write watchpoints; BPF programs (entry, point, return, watch); site-descriptor array indexed by cookie; per-thread `tseq`; per-object `objseq`; per-CPU ring buffers / perf buffers; drop counters / `PERF_RECORD_LOST`; drainer (CPU time and placement recorded); `run-*.kzt` (SITES, RAW_EVENTS, STRINGS, STACKS, LOSS, RUNMETA, PERTURBATION); `kz trace`; `kz hold` (ptrace, DR0–DR3, attach at clone); harness root hook; normalization-boot driver; cFS threads.
- **Edges**: plan → loader: register by inode, load descriptors; thread hits probe → BPF: trap (0.57 µs at entry+4; 31.7 µs class otherwise); BPF → cookie → descriptor; BPF → `tseq`/`objseq`; BPF → buffer (failure → drop counter); buffer → drainer → `run-*.kzt`; drainer → `kz hold`: live stream (release on event); `kz hold` → thread: stop/continue; harness hook → loader: start before launch; abnormal exit → normalization boot (unobserved, excluded); `run-*.kzt` → `kz trace`.

### D8 — The R2 certificate (ES and EVS halves)
- **Nodes**: capture points `c` (loads of `CFE_SB_Global.AppId` at argument evaluation in the SB event sites inlined in `CFE_ES_TaskInit` / `CFE_EVS_TaskInit`) and the slot checks; `m_ES` (return of the `CFE_ES_TaskInit` call, `cfe_es_task.c:114`), `m_EVS` (`cfe_evs_task.c:213`); raises `cfe_es_task.c:134`, `cfe_evs_task.c:229`; falsifiers `f_ES`, `f_EVS` (`cfe_es_start.c:803` per iteration) with allocation records ρ_ES, ρ_EVS; failure clears `:823` per iteration; observations `o_ES`, `o_EVS` (`:835`) with the extracted predicate; panic branches; SB spawn (`:810`, SB iteration); SB entry; writer `x` (`cfe_sb_task.c:138`); INIT (zero-initialized `CFE_SB_Global`).
- **Edges**: `c` ↛ `m` (no path: FRONTIER); `m` →T-PO raise →T-OBS `o`; `:823` ✕ (OWN-NOPATH); `raise_ES` ✕ from `W_o(o_EVS)` (ONCE-PWB); `o` →T-PO (panic cut by T-TERM) SB spawn →T-SPAWN SB entry →T-PO `x`; INIT → "AppId = 0" (INIT_FALSE); `derives(c, CFE_SB_Global.AppId)`; a box listing every writer entry with transition and key status and every trusted entry (App. A.1).

### D9 — Teardown scoping
- **Nodes**: requester actor (`CFE_ES_ResetCFE` → `CFE_PSP_Restart`: shm delete if POWERON → boot record → `ShutdownReq` store → `SIGUSR1` → delay `CFE_PSP_RESTART_DELAY` → `abort`, terminal); PSP main thread (`CFE_ES_Main` → return → `OS_Application_Run` sigwait loop → observe `ShutdownReq` → delay 100 → `OS_DeleteAllObjects`); in-scope uses; K-TEARDOWN uses; next run's B.Init.
- **Edges**: `ShutdownReq` store →T-OBS `OS_DeleteAllObjects` (SR ∈ MB); `OS_DeleteAllObjects` ⇢ disabler of every `osal.obj` (SCOPE-excluded from in-scope certificates); uses after SR → K-TEARDOWN; POWERON → clean Init; other exits → PROCESSOR reset → normalization boot.

### D10 — Two skeleton views
- **Nodes**: an actor CFG with init prefix, message loop (handlers H1, H2), exit; `Sk^cert` (loop head, back edge, iterate/exit choice, dispatch folded by `Prov`); `Sk^h` for h = 1, 2 (unrolled copies, `TRUNC` marker); consumers: MustBefore/certificates/inactive (from `Sk^cert` only), explorer/O8 (from `Sk^h`, h in b), proofcheck view check.
- **Edges**: CFG → `Sk^cert`; CFG → `Sk^h`; `Sk^cert` → static; `Sk^h` → explore; certificate step naming a `Sk^h` node → proofcheck: reject; litmus App. A.8 annotated "writer only in handler iteration 2".

---

## 13. Glossary

| Term | Meaning in this specification |
|---|---|
| **Actor / actor instance** | a thread of execution (OSAL task, raw thread or environment root); an instance is (static actor, generation) |
| **Allocation record ρ(n)** | the abstract record allocated by reservation node n (§4.2) |
| **Allowed** | the documented not-ready behaviours of a contract, from a finite vocabulary, each with a reference horizon (§1.6) |
| **Anchor / anchor audit** | the mapping of a source site or AST pattern to an instruction of the gcc binary; the audit lists sites without a unique anchor |
| **A-COMM** | the per-witness check that unobserved shared reads in the cone are observed, constant or covered by kept outcomes (§4.9) |
| **Assumed visibility** | an edge that holds only under A4; flagged on every verdict that uses it |
| **A-TYPE, A-FRESH, A-OWNER** | typed-access, allocation-freshness and ownership assumptions (§6.1) |
| **B (basis)** / **B′** | everything fixed except the schedule (§1.7) / a configuration or input variant: a new basis, never a "new bug" |
| **Bad / good / stall** | outcomes of a maximal execution relative to an instance (§1.6, §1.8) |
| **Capture point** | the load from which a value-carried argument derives; the LP of a value conjunct (§1.4) |
| **Census** | the instantiation of every contract at every context-qualified site, each with a class or `unknown(reason)` per (σ, M) |
| **Certificate** | a checkable proof object (rule instances, writer entries with statuses, once facts, derives facts, extracted predicates, frontier, reach) for a fixed-order or protected verdict |
| **Certificate view `Sk^cert` / exploration view `Sk^h`** | skeleton with every unbounded loop star-closed / skeleton unrolled to horizon h with truncation marks (§4.3) |
| **Cell** | the part of a verdict record for one (σ, M) |
| **Chain** | a violation link `i₁ ⇝ i₂`, recorded with every branch |
| **Cone** | the closed set of actors and objects relevant to a candidate (§4.10) |
| **C_σ** | schedule constraints of σ_R1; never HB; never combined with an observed prefix |
| **DECLARED_TRUSTED** | a writer, allocation or ownership entry that rests on a Φ guard or assumption the analysis could not verify; listed in every certificate that uses it |
| **derives(arg, loc, site)** | the fact that an argument's value is a copy chain from one shared location read at site |
| **Evidence: realization / naturalness** | how a class is supported (certificate … realized) / how natural a realized run was (natural … timeout-exercised) |
| **exh / dpor** | the explorer's exhaustive stateful engine / source-DPOR engine (unverified while T-JACM is P) |
| **F0–F3** | fidelity classes of [R07 §7] |
| **Falsifier** | an event in the observer's actor that dominates an observation and definitely makes the observed conjunct false |
| **Fixed-order** | class: every execution reaching the use (in scope) is bad |
| **Frontier node m** | a node in the use's once actor with no path to the conjunct's LP and `m ∈ MB(x)` for every writer x |
| **Generation** | an incarnation of a resource or actor |
| **HB_π, VC, VC_R** | happens-before on a trace (looper rule applied); vector clocks over all definite edges and over unconditional/obs-fatal edges |
| **Horizon h** | message-loop iterations in the exploration view; not chosen; results reported per completed h |
| **Inactive** | class: no execution reaches the use under B (certificate view, with message provenance) |
| **KEM** | Kairos Event Model |
| **Key / key status** | the identity of a resource within its kind / MUST_KEY, MAY_KEY or NO_KEY of a writer w.r.t. an instance key |
| **K-SELF.es** | the generic contract "the caller's own App and task records are LIVE", at seven call sites |
| **K-TEARDOWN** | the family of uses after the shutdown request |
| **KZ container** | the typed binary artifact format (§2.3) |
| **L1/L2/L3** | lemma obligations: await summarization with extracted predicates; removal of irrelevant critical sections; mover-reduced critical sections |
| **LOSS-UNDER-ORDER** | outcome label for route/capacity contracts whose Allowed is unspecified by reference |
| **LP** | linearization point, declared per conjunct |
| **M_api / M_impl** | OSAL model levels: API-contract model atomic at LPs / OSAL POSIX internals analysed |
| **M_impl-relevant** | an operation modelled differently at the two levels (idmap, task, binsem, queue paths) |
| **MB (MustBefore)** | static must-precedence, the greatest fixpoint of §4.4 |
| **Message provenance Prov(p)** | the MsgIds (and codes) that can reach pipe p under B (§4.3) |
| **Normalization boot** | an unobserved boot ending with the POWERON restart, run after an abnormal exit (§4.13) |
| **O1–O9, CC** | consistency obligations and the certificate check (§2.5) |
| **once(τ), once(n)** | the actor has at most one instance per execution / the node occurs at most once per execution (§1.2) |
| **On-behalf frame** | an identity frame for code run in one actor on behalf of another |
| **Perturbation ratio** | probe and hold time added between two deciding events, over the code-derived separation protecting the other order (§4.13) |
| **Φ (profile)** | the declarative cFS knowledge base, with core expressions, bindings and patterns |
| **P_sched** | a cited target scheduling profile for σ_R1 |
| **Portability hazard (-evidence)** | protected under M_impl but violable under M_api / an M_api-only certificate contradicted by the binary |
| **Pred(π)** | the predictive model of a trace |
| **Protected / tolerated** | class: no bad and no stalled execution / protected with allowed not-ready occurrences |
| **Provider axis** | present, none, env-absent, unknown |
| **PWB** | precedes-whenever-both-occur (FRONTIER or ONCE-PWB) |
| **Reach** | proved (run or confirmed model path) or assumed |
| **Recall protocol** | detection measured on history-derived program mutants and pre/post-fix pairs (§8.5) |
| **Robustness tag** | per trace edge: unconditional, obs-fatal, obs-soft, schedule-chosen, assumed |
| **Schedule-dependent / refinements** | class: good and bad (or stall) executions exist / timeout-dependent, timeout-masked-liveness; flag timeout-rescued |
| **Scope** | contracts apply before the shutdown request; later uses are K-TEARDOWN |
| **Shape** | WRONG-WAY, UNORDERED, ABSENT, STALE |
| **σ_U, σ_U∞, σ_R1[P_sched], σ_E** | untimed; patient (no timeouts, patient retries, strongly fair, stall); fixed-priority one core under a profile; the real environment |
| **σ_E(held), σ_E(plan P)** | realized but not natural / a σ_E statement under observation plan P |
| **Support locations** | the memory a conjunct's oracle reads, its bindings, or its `derives` location |
| **T-PO, T-SPAWN, T-OBS, T-TERM** | the MustBefore rules (§4.4) |
| **Template-only run** | an evaluation run with the frozen profile and no case-specific refinement |
| **Transition status** | COMPUTED, DECLARED_VERIFIED, DECLARED_TRUSTED, DISABLED |
| **unspecified_by_reference** | no reference fixes the value, and no value is chosen; chosen values are deviations |
| **Work unit** | a deterministic count (states, transitions, search nodes, fixpoint updates) used for every analysis budget |
| **Writer set W / W^must / disabler set D** | stores that can make a conjunct true / definitely make it true / can make it false, for a key |

---

## Appendix A. Worked examples (expected behaviour; nothing here was run)

### A.1 R2: fixed-order from a generic contract plus framework rule R19, fully written out

1. **Census.** `K-ID.send-with-appid` (provenance `cfe_evs.h:168–172`) is instantiated at every context-qualified `CFE_EVS_SendEventWithAppID` site. The SB sites pass `CFE_SB_Global.AppId` as `stamp` (39 SB sites, [R2c §2]). Contexts inlined from `CFE_ES_TaskInit` and `CFE_EVS_TaskInit` are distinct templates (ADR-012). No SB-specific contract exists.
2. **Conjuncts and T-DERIVE** (fixes [X1-M1]). `Req = valid(arg.AppID) ∧ state(evs.reg(arg.AppID)) = REG`. At each SB event site, `derives(arg AppID, (CFE_SB_Global_t, AppId), c)` holds, where `c` is the load at argument evaluation (e.g. `cfe_sb_api.c:226–228`, re-read). The value conjunct `V` is evaluated at `c` over `cell(CFE_SB_Global.AppId)` (L-DERIVE). The registry conjunct plays no role here: EVS tests index validity first (`cfe_evs.c:185–189`), and `V` already fails.
3. **INIT and the writer set of V.** INIT: `CFE_SB_Global` is a zero-initialized global (`cfe_sb_task.c:40`, re-read), so `AppId = 0 = CFE_ES_APPID_UNDEFINED`: `INIT_FALSE(V)`. S3a writers of `(CFE_SB_Global_t, AppId)`:
   - `x`: the success-branch out-pointer store of `CFE_ES_GetAppID` instantiated at `cfe_sb_task.c:138` — W, COMPUTED, MUST_KEY (same location);
   - the failure-branch store of `UNDEFINED` (`cfe_es_api.c:758–760`) — neutral;
   - the EarlyInit `memset(&CFE_SB_Global, 0, …)` (`cfe_sb_init.c:50`, re-read) — writes 0, neutral;
   - A-TYPE listings must show no untyped write and no escaping address (`&CFE_SB_Global.AppId` is taken only at `:138`, into `GetAppID`'s summarized formal); otherwise the writer set is DISABLED and the certificate is withheld. Any further store found is listed.
4. **Observation `o_ES`** (ES iteration of the unrolled core loop; fixes [X2-01], [X1-M8]). `o_ES` is the success exit of `MainTaskSyncDelay` at `cfe_es_start.c:835`. `Pred_code` is extracted (form (b), §4.3): `∀ r ∈ AppTable: ¬(IsUsed(r) ∧ r.AppState < RUNNING)`. `C_ES` is its instance at `ρ_ES`, the record allocated by the falsifier `f_ES` = `SetUsed(RESERVED)` at `:803`. Right after `f_ES`, `ρ_ES` is used and its `AppState` is 0 (UNDEFINED): before `f_ES` no task exists, and the only `AppState` writer that can precede it is the zeroing memset at `:82`, so no freshness assumption is needed here.

   | Writer | Site | Actor | Treatment | Status |
   |---|---|---|---|---|
   | raise | `cfe_es_api.c:573` via `cfe_es_task.c:134` | ES main | kept; `m_ES ∈ MB` (T-PO) | COMPUTED |
   | `ExitApp` STOPPED | `cfe_es_api.c:399` via `cfe_es_task.c:200` | ES main | kept or excluded — either way after `raise_ES` and `m_ES` (T-PO) | — |
   | `RunLoop` raise | `cfe_es_api.c:471` | none of the concurrent actors | absent (call graph) | COMPUTED |
   | scan WAITING | `cfe_es_apps.c:1144` | `ES_BG_TASK` | neutral (same-record guard) | COMPUTED |
   | `CleanUpApp` record free | `cfe_es_apps.c:1407–1640` | `ES_BG_TASK` | excluded (external app with `AppState > RUNNING`; relational guard) | **DECLARED_TRUSTED** |
   | failure clear `memset` | `cfe_es_start.c:823` (ES iteration) | startup thread | OWN-NOPATH (only the panic follows) | COMPUTED |
   | EarlyInit `memset` | `cfe_es_start.c:82` | startup thread | ONCE-PWB(w, `f_ES`) | COMPUTED |
   | later startup-thread writers (`:823` of later iterations, `cfe_es_apps.c:893`, script reservations) | — | startup thread | OWN-NOPATH (no path back to `o_ES`; the core loop is unrolled) | COMPUTED |
   | writers in EVS, SB, TBL, TIME, script apps and their children | — | spawned after `o_ES` | FRONTIER: `PWB(o_ES, w)` | COMPUTED |
   | `cFS-Master`, console | — | raw threads | no writers (S3a over their call graphs) | COMPUTED |

   Result: `m_ES ∈ MB(o_ES)` and `raise_ES ∈ MB(o_ES)`.
5. **Observation `o_EVS`** (EVS iteration; fixes [X1-M3](b)). Falsifier `f_EVS` = `:803` in the EVS iteration, record `ρ_EVS`. Right after `f_EVS`, `ρ_EVS` is used and `AppState(ρ_EVS)` = 0: this needs **A-FRESH** (`ρ_EVS` ≠ `ρ_ES` and never used before) and **A-OWNER** (ES main's and `ES_BG_TASK`'s `AppState` writers target `ρ_ES`, hence `NO_KEY` for `ρ_EVS`).

   | Writer | Site | Actor | Treatment | Status |
   |---|---|---|---|---|
   | raise | `cfe_es_api.c:573` via `cfe_evs_task.c:229` | EVS main | kept; `m_EVS ∈ MB` | COMPUTED |
   | `ExitApp` stores | `cfe_evs_task.c:220, :257` | EVS main | kept; after `m_EVS` (no-ops inside the window, §3.3) | COMPUTED |
   | raise | `cfe_es_task.c:134` | ES main | **ONCE-PWB**: `once(raise_ES)` and `raise_ES ∈ MB(o_ES) ⊆ MB(f_EVS)` ⇒ `PWB(raise_ES, f_EVS)` | COMPUTED |
   | `ExitApp` STOPPED | `cfe_es_task.c:200` → `cfe_es_api.c:399` | ES main | excluded: guard `Type != CORE` on the owner record, which is `ρ_ES` (A-OWNER) with `Type = CORE` (MUST_KEY store at `:795`, ES iteration); also KEY-SEP vs `ρ_EVS` under A-FRESH. Not dominated by `m_EVS` and not PWB-ordered (an early `ExitApp` after a timed-out CORE_READY wait is reachable in σ_U) | **DECLARED_TRUSTED** (A-OWNER) |
   | scan WAITING | `cfe_es_apps.c:1144` | `ES_BG_TASK` | neutral | COMPUTED |
   | `CleanUpApp` record free | `cfe_es_apps.c:1407–1640` | `ES_BG_TASK` | excluded (relational guard) | **DECLARED_TRUSTED** |
   | failure clears `memset` | `cfe_es_start.c:823` (ES and EVS iterations) | startup thread | OWN-NOPATH | COMPUTED |
   | EarlyInit `memset` | `cfe_es_start.c:82` | startup thread | ONCE-PWB | COMPUTED |
   | writers in SB, TBL, TIME, script apps | — | spawned after `o_EVS` | FRONTIER | COMPUTED |

   Result: `m_EVS ∈ MB(o_EVS)`.
6. **MustBefore chain.** `m_ES ∈ MB(raise_ES)` (T-PO) → T-OBS → `m_ES ∈ MB(o_ES)` → T-PO along the startup thread (panic branches have no successors, T-TERM; the failure branch through `:823` leads only to the panic) → `spawn_SB` (SB iteration) → T-SPAWN (single spawn site; `once(SB main)`) → SB entry → T-PO → `x`. Hence `m_ES ∈ MB(x)`. The EVS half is the same with `m_EVS` through `o_EVS`; the EVS iteration precedes the SB iteration in the generated core order ([R02 §1.3]).
7. **Frontier and once.** `once(ES main)` and `once(EVS main)`: each has one spawn node in the unrolled core loop, on the once startup thread, outside every star-closed region. `m_ES ∉ Anc(c)` for every capture point `c` inside `CFE_ES_TaskInit` (called once, before the message loop; in `Sk^cert` the loop's back edge returns to the loop head after `m_ES`). **The early returns do not matter**: `CFE_ES_TaskInit` returns at L340, L371, L381 or L393 after failures, and `CFE_ES_TaskMain` still raises at L134 (re-read); the proof never needs the use to dominate the raise ([J2-1]).
8. **Badness.** `Allowed = none` (the header names the system log, a different API, as the alternative); harm `event_lost_silently`; every SB caller discards `ILLEGAL_APP_ID`.
9. **Verdict.** `fixed-order(σ_U, M_api)` and `fixed-order(σ_U, M_impl)`: the certificate contains task creation, an M_impl-relevant operation, so the M_impl certificate is computed on the M_impl skeleton, where T-SPAWN still holds. σ_U∞ and σ_R1 cells by transfer rule 1 (same M, b = ∞); the σ_E cell by transfer rule 2 from M_impl, "wherever reached", with `reach = proved` once S1 re-observes an occurrence. Realization `certificate`, plus `realized/natural` observations `n of N` with their pool key. Shape WRONG-WAY where SB publishes later; ABSENT where SB's own window is hit (both bad).
   - **Trusted entries**: ES half — the `CleanUpApp` relational guard. EVS half — A-FRESH, A-OWNER and the `CleanUpApp` guard. All are listed in the certificates (G10) and refutable by O9 and O3/O4.
10. **Refinement accounting** ([X1-M8]): "generic contract K-ID + framework rule R19". R19 binds `MainTaskSyncDelay`'s loop at `:835`, its stratum and its terminal escape; the predicate is extracted from the code and re-derived by `kz proofcheck`.
11. **S3a exit mutations** (each must be rejected): `m_ES` replaced by a node inside `CFE_ES_TaskInit` before `c` (in `Anc(c)`); the `once(ES main)` fact dropped; the ONCE-PWB fact for `raise_ES` dropped from the EVS certificate; `:823` re-inserted into `W_o(o_ES)` without its OWN-NOPATH proof; the extracted predicate replaced by one that forgets that RESERVED counts as used; the `derives` link dropped; a certificate step moved into a `TRUNC` region.
12. **Expected per-use set** (a check of basis sensitivity and the S1 gate, not new evidence): ES 20 = 17 `SEND_NO_SUBS` + 2 `SUBSCRIPTION_RCVD` + 1 `PIPE_ADDED`; EVS 4 = 2 + 1 + 1; total 24 per boot with ENV's version table ([R2g §2]; [R2c §7]); 11 (ES 7, EVS 4) in a basis whose version table is NULL ([R2c §4.2]).
13. **Chain.** The 17 `SEND_NO_SUBS` uses are linked to K-ROUTE instances for ES's event packets (MsgId 0x808), transmitted before any subscription ([R2g §2]); LOSS-UNDER-ORDER, harm unspecified.
14. **Not claimed**: that R2 is a race; any count as a Kairos result before S1; determinism from repeated runs; that the certificate uses no framework rule.

### A.2 The core-task App-record window and the unpublished SB/TBL identity (INFERENCE until run; [J1 §1.1], [LED §3.4])

1. **Instances.** K-SELF.es at every context-resolving call made by a core task (and by children it creates in TaskInit) before `cfe_es_start.c:818`: EVS `GetAppID` (`cfe_evs_task.c:273`); ES's `CFE_EVS_Register` in `CFE_ES_TaskInit`; SB `GetAppID` (`cfe_sb_task.c:138`); TBL (`cfe_tbl_task.c:196`); TIME's, incl. its `CreateChildTask` calls (`cfe_time_task.c:160, 173`); `ES_BG_TASK`'s creation (`cfe_es_backgroundtask.c:214`); every `WaitForSystemState` raise.
2. **Static.** `W` = `CFE_ES_AppRecordSetUsed(ρ, PendingAppId)` at `:818` (startup thread). The child's MB contains the spawn and, through T-OBS at `GetTaskFunction`, the task-record fill (`cfe_es_apps.c:694–703`), but not `:818`. No certificate either way → open, routed **O-ordered** (both LPs under the ES lock).
3. **Prediction.** From a default boot, reverse the startup thread's critical section at `:816–818` and the child's `GetAppID` critical section; the child's poll keeps its success outcome because the task record was filled earlier. A witness is expected → `schedule-dependent(σ_U, M_api and M_impl)`; A-COMM decides between `predicted` and `predicted-modelled`. Under σ_U∞ no timeout is involved → still schedule-dependent (no refinement). Under σ_R1[`P_sched^posix`] the startup thread outranks the tasks, so [I] the window cannot open on one CPU.
4. **Consequences** (every branch, §3.3): EVS (a1) `service_task_absent`, (a2) `panic` after 30 s, (b) `process_abort` after `CFE_PSP_RESTART_DELAY` = 10 s (the abort precedes the 30 s panic); ES/SB/TBL/TIME raise skipped → `panic`; SB/TBL identity UNDEFINED for the run → every later SB/TBL `SendEventWithAppID` use bad with shape ABSENT, chained to this root (the root is reported, not the 211–229 dependents); TIME's children → `task_never_runs` if `CreateChildTask` fails.
5. **Confirmation (D16, derived).** Hold the startup thread after `CFE_ES_StartAppTask` returns (`cfe_es_start.c:810–814`) and before `CFE_ES_LockSharedData` at `:816`; its program-lock set is empty there. Release when the child's `GetAppID` returns.
   - Watchdog: the smallest reference timeout the hold can make expire — the already-created core tasks wait in `WaitForSystemState(CORE_READY, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` = 30000 ms (`default_cfe_core_private_internal_cfg.h:68`), and where the predicted continuation includes a restart request, `CFE_PSP_RESTART_DELAY` = 10000 ms (`pc-linux/inc/cfe_psp_config.h:57`) → 10000 ms for EVS's window.
   - Oracle: the `GetAppID` outcome; for SB, `ILLEGAL_APP_ID` on every later SB event; for EVS, which branch of the chain occurred (abort, panic or missing EVS task).
   - The run may end abnormally, so a **normalization boot** follows (§4.13). Labels: `realized`, `held-F0`, `derived`, with the perturbation ratio over the 50 ms poll separation.
6. **σ_E**: no natural bad witness is expected; the record reads "no σ_E bad witness in n of N runs (pool key); σ_E class unknown". No claim of timing protection.

### A.3 The script-app App-record window (correcting [B §7.2])

1. **Instances.** K-SELF.es at each script app's context-resolving calls (`CFE_EVS_Register` → caller context → `GetAppID`; `RunLoop`; `WaitForSystemState`; `CreateChildTask`, e.g. FM at `fm_child.c:86`); K-REG.es at peers' `GetAppIDByName`, whose oracle classifies the sentinel success as not-ready ([J1 §1 check 4]).
2. **Trace routing.** The publication (`cfe_es_apps.c:861–869`) and the use's LP are both under the ES lock: the **O-ordered** route to prediction, not "unordered → SHB" ([J1 §1 check 3]; [J3 §2.2]).
3. **Expected**: `schedule-dependent(σ_U, σ_U∞)`, evidence `predicted` or `predicted-modelled`; harms per branch: registration failure → app error exit; `RunLoop` false with the window open → `app_zombie` plus two soft-barrier timeouts; with the window closed → `app_error_exit`. σ_E: "no σ_E bad witness in n of N runs; σ_E class unknown".
4. **Confirmation** ([B §7.2], corrected placement). Hold the startup thread in `CFE_ES_AppCreate` after `CFE_ES_StartAppTask` returns (`:848–854`) and before the lock at `:861`; its lockset is empty there. Release when the child's first context-resolving call returns.
   - Watchdog: the smallest timeout among the waits of already-started actors that the held startup thread can make expire (their `WaitForSystemState`/`WaitForStartupSync` arguments, constant-evaluated; the script barrier `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` once the script phase has begun).
   - Oracle: `CFE_ES_ERR_RESOURCEID_NOT_VALID` (`cfe_es_api.c:757–761`); `RunLoop` returning false; the sentinel success for peers. Labelled derived; normalization boot if the run ends abnormally.

### A.4 C1 in a derived configuration, and the OSAL lock-leak path under M_impl

1. **Default B.** CF's lookup sits behind a non-empty `sem_name` test (`cf_cfdp.c:1270`); both default entries are empty (`cf_def_config.c:60, 78`) → `inactive(B)`; the proof is the folded path condition in `Sk^cert`.
2. **B′** (public-code-derived, [LED §1a]): a non-empty name and a provider app that creates the counting semaphore in its init. Neither app waits on a phase → no MB relation → open. Observation shows either success (outcome-implied reads-from from the provider's publication, an O-edge through the countsem table lock) or `NAME_NOT_FOUND` followed by retries.
3. **M_api.**
   - **σ_U**: the retry loop is unrolled exactly to the effective `CF_STARTUP_SEM_MAX_RETRIES`; exhaustion is reachable only after the last attempt. The explorer finds an execution in which every attempt precedes the creation (delays are yields) → `CF_INIT_SEM_ERR_EID` (`cf_cfdp.c` ~L1295) and engine initialization fails (consequence per [A §A.2]) → `schedule-dependent(σ_U)`. Prediction may extend an observed retry to the effective bound; such a witness is `predicted-extrapolated`.
   - **σ_U∞**: the loop is delay-paced, hence patient: exhaustion is enabled only at quiescence with the lookup still failing, which cannot happen while the provider can still run. Only good executions remain. Every σ_U bad execution takes the exhaustion transition after the reference bound, and the σ_U∞ continuation from the state before it reaches only good outcomes → refinement **timeout-dependent** (§1.8, L-REF) [I]. At the buggy revision (a single attempt) the violation needs no timeout and stays schedule-dependent under σ_U∞: the fix of C1 ([LED §1b]) turned an ordering-logic violation into a timing-margin dependence. If the provider could never run (e.g. a provider spawn lost to F7), σ_U∞ would reach `stall` → **timeout-masked-liveness**. A single early lookup is `tolerated` (`retry_until_ready`).
4. **M_impl.** The loop is unrolled, not summarized. `OS_CountSemGetIdByName` uses a **by-name** token, so in the RESERVED window with the name visible it does not wait: it returns `OS_ERR_INCORRECT_OBJ_STATE` at once (`osapi-idmap.c:401–404`), which ends CF's loop (`cf_cfdp.c:1285–1288`, [R01 §1.8]); by K-LOCKBAL the countsem table lock stays held (vs the documented invariant `:384–385`); the provider then blocks in `FinalizeNew` → deadlock check → **implementation-defect candidate** (F1). The M_api result for this instance is not transferred to σ_E (the cone is M_impl-relevant and the levels disagree): `portability-hazard` is not the label here, since the violation exists only under M_impl.
5. **Confirmation plans.** For (3): hold the provider before its create call (lockset empty) until CF's loop exit is observed; the provider is created after observation starts, so the holder auto-attaches at clone on the startup thread (§4.13), and that stop is recorded; watchdog = effective retry bound × effective delay. For (4), a **declared experiment**: hold the provider at `OS_CountSemCreate_Impl` entry (inside the RESERVED window, name visible, type lock **not** held, [R01 §1.5 step f]) until CF's lookup returns, then release; the oracle is a wait-for fact (the provider blocked on the countsem table lock by futex/wchan while CF has already returned). The harness kill of the deadlocked run is followed by a normalization boot. Labelled derived.

### A.5 Startup-burst capacity: `SBNSubPipe` (K-MODE + K-CAP + K-CAP.msglim, LOSS-UNDER-ORDER)

1. **Instances** (re-stated per [X2-05], [X2-06]).
   - **K-MODE**: SBN transmits `ENABLE_SUB_REPORTING` and `SEND_PREV_SUBS` to SB's command pipe (`sbn_subs.c:43–60`); SB's main task flips `sb.mode.subreport` (LP `cfe_sb_task.c:1226`, reached from `:296`) and runs the replay `CFE_SB_SendPrevSubsCmd` (`:1169`) as a transmit loop in **SB's actor**. Subscribers that subscribe after the flip read the flag after the SB unlock without a lock (`cfe_sb_task.c:659`, `assumed`) and transmit a `ONESUB` report from **their own task** (`cfe_sb_api.c:1192–1195`). The chain runs through R7 on SB's command pipe, with K-CAP and K-CAP.msglim instances for that pipe.
   - **K-CAP** at each put into `SBNSubPipe` (effective capacity 10, [BM §3.1]); the puts come from SB's replay loop and from subscriber tasks (TO_LAB's 37 global subscriptions from its table, [R03 §2.3, §2.15]).
   - **K-CAP.msglim** for SBN's subscriptions to the report MsgIds, with the limit constant-evaluated at SBN's subscribe sites.
2. **Tier 1.** The maximum-weight-closure backlog bound over the certificate-view MB order of puts and gets exceeds 10 (TO_LAB alone can add up to 37 reports, and the replay loop is star-closed), so there is no protected certificate → open. The attribution of the burst between SB's replay and the subscribers' reports was an inference in [R03 §5.2] ([J2 §7]); the census's instance identification checks it.
3. **σ_E.** Overflow was observed in 5 of 6 M1 runs, and the sixth (A3) showed a different interleaving with `TO_LAB_TLM_PIPE` overflowing instead ([BM §3.3]). These are prior observations by the M1 harness (D12); Kairos's recorder re-observes them in S1/S2.
4. **Verdict.** If both outcomes are re-observed in natural runs: `schedule-dependent(σ_E)`, transferred up to σ_U; label LOSS-UNDER-ORDER, harm `unspecified_by_reference`. Oracles: per-put outcomes, `PipeOverflowErrorCounter` and `MsgLimitErrorCounter`; `PeakQueueDepth` only as SB-accounted depth. Part of RQ5.

### A.6 HS system monitor: provider `env-absent` (correcting [A], [R03 §3 row 20])

`HS_SysMonInit` finds the `linux_sysmon` device: the module is in the PSP's fixed list (`psp_module_list.cmake:9`), so `FindByName` succeeds; a failure there would be handled (`hs_sysmon.c:50–54`). The `SET_RUNNING` command then fails, and HS returns `CFE_STATUS_EXTERNAL_RESOURCE_FAIL` (`:65–72`), because `/proc/schedstat` is absent on this kernel ([R01 §2.6]; [R07 §1]).

Φ declares the device's RUNNING transition guarded by `env(/proc/schedstat present)`, and B.Env records it absent. Outcome mining finds the not-ready code, and O4 agrees → provider `env-absent`. With no writer enabled in B, every reaching execution has the not-ready outcome: class `fixed-order` with outcome label `not-ready-unjudged` (Allowed is `unspecified_by_reference`), reach proved by the observed failure. This is not an order problem. It is reported separately, with the measured consequences: HS init stops at `HS_SysMonInit`; its startup sync is skipped; CPU utilisation is reported as `0xFFFFFFFF` ([BM §0]).

### A.7 Calibration: timebase lookups are protected outside teardown

`cFS-Master` is created and started by the PSP's module init on the main thread before `CFE_ES_Main` (§3.5). `x*` (its publication LP, name constant `"cFS-Master"`) is a `W^must` entry with `MUST_KEY`, COMPUTED, and is in MB of every cFE task entry by T-PO on the PSP main thread and T-SPAWN.

Disablers of `osal.obj<timebase>("cFS-Master")` (fixes [X1-M6], [X2-04]):
- `OS_DeleteAllObjects` (`cfe_psp_start.c:507`): `SHUTDOWN_REQ ∈ MB` by T-OBS on the `ShutdownReq` loop → **SCOPE-excluded**;
- any `OS_TimeBaseDelete` site found by the census: none is expected in the default B [I]; any found must be PWB-ordered, or the certificate is withheld.

TIME's and SCH_LAB's lookups (`cfe_time_task.c:311`, `sch_lab_app.c:176`) are therefore `protected(σ_U, M_api)` and, since the record is already LIVE when any lookup runs (`x*` is the final publication), `protected(σ_U, M_impl)` [I]; the σ_E cell follows from M_impl by transfer rule 2. All are stated **outside teardown**. SCH_LAB's lookup may run after `CFE_ES_Main` returns past a soft timeout (`cfe_es_start.c:225–228`); it is still in scope and still protected, because teardown needs a shutdown request. Command-triggered resets need no special treatment: their teardown disablers are SCOPE-excluded too, and with an empty B.Input the reset handlers are inactive by message provenance anyway. The timebase thread's own handshake (K-SPAWN.osal, §3.3) is a separate instance. This is the check that the tool does not invent races where the code orders.

### A.8 Litmus programs added by the reviews (gcc-built C99 on the real OSAL; expected classes per σ and M)

| Litmus | Program shape | Expected result | Checks |
|---|---|---|---|
| **L-H2** (fixes [X1-C1]) | a looper whose handler for message M1 sets a flag; its handler for M2 creates resource R only if the flag is set; another actor sends M1 then M2 (in-B transmits); a consumer looks R up | provider `present`, not `none`; not `inactive`; no certificate in either direction from `Sk^cert`; explorer: with h = 1 the writer is not reached (result carries h = 1, `unknown(bound)` for the good side), with h = 2 schedule-dependent | the certificate view star-closes the loop; `kz proofcheck` rejects a fabricated certificate that names a node beyond h |
| **L-ONCE** (X1-M2) | a spawn site inside a loop creates two instances of one entry; a frontier argument would put a use in instance 1 before a writer in instance 2 | once(τ) fails; frontier and once-PWB forbidden; schedule-dependent | the once analysis and its proofcheck re-validation |
| **L-OWN** (X2-01) | an observer with a falsifier and a writer on a failure branch that ends in `abort()` | the certificate passes with OWN-NOPATH; a variant in which the failure branch rejoins the observation must lose the certificate | OWN-NOPATH and its reachability re-check |
| **L-SCOPE** (X1-M6) | a disabler that runs only after a shutdown-request store; a use before and a use after the store | the first use protected outside teardown; the second a K-TEARDOWN instance | SCOPE exclusion |
| **L-PROV** (X1-M7) | a command-only handler that writes a resource; B.Input empty vs one command | inactive vs active | message provenance and its fixpoint |
| **L-CAPTURE** (X1-M1) | a value loaded from a shared cell, then the publisher writes, then the API checks the copy | the outcome follows the capture point; O4 agrees; a predictor that targeted the check LP would accept a false good witness | per-conjunct LPs and `derives` |
| **L-PATIENT** (X1-M5) | a delay-paced retry with a provider; variant with a provider that never runs | variant 1: σ_U schedule-dependent, σ_U∞ only good → timeout-dependent; variant 2: σ_U∞ stall → timeout-masked-liveness | σ_U∞ quiescence rule; refinements |
| **L-MIMPL** (X1-C2) | a by-name lookup racing a creation (RES_CN window) | M_api protected only if certified; M_impl: `INCORRECT_OBJ_STATE` reachable; no σ_E transfer from M_api; an observed `INCORRECT_OBJ_STATE` yields `portability-hazard-evidence`, not a model fault | transfer rule 2; O4(b) at M_api |
| **L-L3** (X1-M10) | a critical section containing a cancel-and-join of another thread (as in `CFE_ES_DeleteChildTask`) | the L3 check fails; the section stays fine-grained; the cancellation-with-lock-held execution is reachable | mover conditions |
| **L-TIMEDWAIT** (X2-03) | a by-ID lookup of a RESERVED record with unrelated unlocks of the same type | σ_U: exhaustion by timed or broadcast wakes; σ_U∞: exhaustion by broadcast wakes only | the two wake causes |

---

## Appendix B. Literature relied on (status as verified in [R04], [R05], [R06])

- Lamport 1978; Fidge 1988; Mattern 1989; Netzer & Miller 1992; Lu et al. 2008 — V-abs.
- FastTrack 2009; ThreadSanitizer 2009/2011; Eraser 1997; O'Callahan & Choi 2003 — V-abs (Djit+ 2003: P).
- CP 2012; WCP 2017; SHB 2018; M2 2020; SyncP 2021; Vindicator 2018; maximal causal models 2012; RVPredict 2014; GPredict 2015; **ExceptioNULL 2012** — V-abs.
- DefUse 2010; AVIO 2006; Velodrome 2008; ConMem 2010; CTrigger 2009; Xiong et al. 2010; Tian et al. 2008 — V-abs.
- EventRacer 2013; DroidRacer 2014; CAFA 2014; **InitRacer 2017** — V-abs. DCatch 2017 — P.
- CloudRaid 2018; Snowboard 2021; FCatch 2018; DataCollider 2010 — V-abs. KCSAN design document — V-doc.
- RacePro 2011; DESCRY 2017; RaceFuzzer 2008; Portend 2012 — V-abs.
- DPOR 2005 (guarantee wording UNVERIFIED); source/optimal DPOR POPL 2014 (JACM extension to disabling: P); BPOR 2013; fair stateless model checking 2008 (strongly fair scheduler); PCT 2010 (mechanism P); SCTBench 2014 — V-abs.
- Alglave, Kroening & Tautschnig 2013; Wang et al. 2009 — V-abs.
- SIERRA 2018; SDV 2006; Holzmann & Joshi 2004 — V-abs. FlowDroid 2014 — V-me.
- Andersen 1994; SVF 2016; MLTA 2019 — V-abs.
- Soundiness 2015; Arcuri & Briand 2011; Klees et al. 2018 (trial count: P) — V-abs. Methodological literature is used for assumptions, never as a `from-reference` value ([POL] L5–10).
- Strom & Yemini 1986 (typestate); Engler et al. 2001 — V-me.
- Boehm 2012 (seqlocks), cited only as verified in [R02 §10].
- Tool documentation (V-doc in [R05]): Nidhugg README (LLVM 8–19; deterministic pthread threads); GenMC README (LLVM 19–22).
- Standard algorithmic fact used without a verified citation: maximum-weight closure via minimum cut (K-CAP bound), as in [A §4.6].
- **Open literature checks** (no claim rests on them until done): the left/right-mover reduction theorem behind L3; the JACM 2017 statement of source-DPOR with disabling (T-JACM); the novelty of timeout-conditional barrier edges (G3, [R04 §8 item 5]); the HB rule tables of DroidRacer, CAFA, EventRacer and DCatch ([R04 §8 item 3]).

No other literature is relied on.

# SPEC v1 — Kairos: final architecture of a detector for process-level readiness and order violations in cFS

- Date: 2026-10-12. Role: lead architect of panel AR2. Status: **architecture specification only**. Nothing was built, installed, run against cFS, modified or committed for this document. The only file written is this one.
- Name: **Kairos** (tool prefix `kz`), kept from proposal C because C is the spine of this synthesis. The name is a label only.
- Inputs and how they are cited:
  - AR1 reports [R01]–[R07] in `scratchpad/arch/`, cited with their own section and `path:line` citations.
  - Proposals [A] (`proposal_A-static-model.md`, CAIRN), [B] (`proposal_B-dynamic-predictive.md`, Sextant), [C] (`proposal_C-hybrid-layered.md`, Kairos).
  - Judgments [J1] (formal), [J2] (cFS), [J3] (methods). All three chose C. Score totals: C 146, A 138, B 129.
  - Research records: `CONDITIONS_POLICY.md` [POL], `cases/ledger.md` [LED], `results/R2_gdb_v701.md` [R2g], `results/R2_cord_v701.md` [R2c], `env/BASELINE_MEASURE.md` [BM]; the user's v0.1 design [v0.1].
  - The pinned tree `/home/user/work/procrace/cfs_ref/cFS` (cFS `088b2fa8`; cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`). Code paths are relative to it. Lines re-read for this document are marked "(re-read)".
- Labels: **[F]** fact with citation (code, record or report). **[M]** measured, in [R07] or a record. **[I]** inference or design reasoning; not executed. **[D]** design decision of this specification (an ADR in §11 records it). **[U]** unverified. Literature is cited only as [R04]/[R05]/[R06] verified it, with their status tags (V-doc, V-abs, V-me, P, UNVERIFIED).
- Policy [POL]: every bound, budget and parameter is `from-reference` (code constant or document, cited), `deviation` (recorded with its reason), or `unspecified_by_reference` (no value chosen; swept and reported, or left to the PI). Tool constants appear in code only as `KZ_REF(value, "path:line")` (§9.2).

---

## 0. Summary

### 0.1 What Kairos decides

For every **readiness-contract instance** in a cFS build (a use of a framework resource that requires that resource to be in some lifecycle state), Kairos produces a verdict record with orthogonal axes (§1.10):

`⟨class, σ, M, b, B-hash, Φ-hash, evidence, shape, outcome-label, consequence, harm, chain⟩`

- `class ∈ {inactive, no-provider, capability-absent, protected (refinement: tolerated), fixed-order, schedule-dependent (refinement: timeout-dependent), unknown(reason)}`.
- `σ ∈ {σ_U, σ_U∞, σ_R1, σ_E}` names the scheduling and timing semantics; `M ∈ {M_api, M_impl}` names the OSAL model level; `b` is the bound.
- Evidence is separate from class: `static-certificate`, `model(b)`, `predicted` (trace-sound), `predicted-extrapolated` (model-based), `observed (n of N)`, `confirmed (F0|F1)` with `derived` and `timeout-exercised` flags.

**No class means "safe".** The absence of an alarm is the absence of evidence. It maps to `unknown(reason)`.

### 0.2 The formal core on one page

1. **One typed event model (KEM) and one framework profile Φ_cFS.** Φ is written once in a small declarative DSL. It declares resource kinds with lifecycle automata, operations with effects and a **linearization point (LP)** per effect, HB rules with stratum and robustness tag, and contracts with a required state, a framework-predicate oracle, allowed not-ready outcomes with a reference horizon, harm and provenance.
2. **Φ is interpreted three ways over the same event type**:
   - **abstractly**, over configuration-closed per-actor event skeletons: census, MustBefore fixpoint, checkable certificates;
   - **concretely**, over traces of the unmodified gcc binary: HB with two vector clocks, contract checking at LPs, lock-reversing prediction;
   - **operationally**, in a bounded per-candidate explorer.

   Confirmation runs on the real binary produce traces that re-enter the concrete interpreter.
3. **Static ordering is a must-analysis.** MustBefore is a greatest fixpoint with meets over predecessors (rule T-PO) and over all spawn sites (T-SPAWN). Observation edges (T-OBS) require a complete, transition-aware writer set, including the initial value. Branches that can only end in a terminal event are excluded (T-TERM). Locks alone give no order.
   - Fixed-order and protected are certified over **precedes-whenever-both-occur** (PWB), never over "the use dominates the writer".
   - Every certificate is re-validated by an **independent checker** with mutation tests.
4. **Writer sets have two oracles.** A field-based, memset-aware, out-pointer-aware mod-ref analysis over the closed LLVM IR establishes completeness. Φ-declared effects supply keys and transition guards. Agreement between the two is an obligation (O7).
5. **Four semantics** σ_U, σ_U∞ (timeout-free, weakly fair, `stall` outcome), σ_R1, σ_E, with a containment lemma under an explicit over-approximation assumption:
   - universal verdicts transfer down where the use is reachable;
   - witnesses transfer up;
   - nothing becomes a σ_E frequency.
6. **Strata.** S (structural, must-order), O (observed synchronization, reorderable in prediction), C_σ (schedule constraints of σ_R1, never HB). In traces, B's robustness tags are the image of the strata: VC_R over S-edges is a cheap prefilter. Exec-level claims come only from static certificates.
7. **Layers check each other.** Eight consistency obligations O1–O8 plus the certificate checker. A contradiction suspends the rule or model involved, sets `unknown(model_gap)` with a model-fault record, and is never resolved silently.

### 0.3 What was taken from where

| From | Grafted element | Where |
|---|---|---|
| C (spine) | KEM + Φ interpreted three ways; S/O/C_σ strata; σ_U∞ and containment; obligations; feedback loop; on-behalf frames; K-LOCKBAL, K-LOCK.cancel, retry- and symmetry-derived templates; generic data-driven BPF program; solver-free predictor; loop ablation; H-V/H-C/H-F | §1, §2, §3, §4.8–4.9, §4.14, §7 |
| A | MustBefore fixpoint (T-PO, T-SPAWN, T-OBS, T-TERM); first-occurrence argument; independent certificate checker with mutation tests; M_api/M_impl with portability-hazard and implementation-defect labels; framework validity predicates as oracle; NO_PROVIDER; LOSS-UNDER-ORDER; shapes WRONG-WAY/UNORDERED/ABSENT/STALE; K-CAP max-weight-closure bound; conformance; Lemmas L1/L2; litmus suite; `CAIRN_REF`-style and case-identifier lints; perturbation group W; Merkle parent hashes; table cross-check against `.tbl` bytes; G-MLIR gate | §1.4, §1.10, §4.2–4.5, §4.10–4.11, §8, §9 |
| B | VC/VC_R with robustness tags; `timeout-dependent` and `tolerated` classes; outcome mining and key-pair enumeration; entry-only per-object lock tokens (in-critical-section probe); entry+4 placement with MC-decoded cost classes; per-thread tokens and loss; tracefs zero-install fallback; debug-register execution breakpoints; hold-until-event; summary-guided extrapolation (labelled model-based); baselines reimplemented over the same traces; group C′; R2 re-observation as first gate; canonical text dumps | §1.9, §4.6–4.9, §4.13, §5, §8 |
| Judges | precedes-whenever-both-occur certificates; transition-aware writer sets; LPs; Allowed horizons; DPOR correctness statement; key-level dependence; explorer kill criterion; staged build; perturbation protocol with cold/warm state; reproducibility of the loop; corrected facts | throughout; §0.4 |

### 0.4 Disposition of the judges' must-fix items

Every item is fixed here. The table says how and where. "J1-n" is item n of [J1 §7]; "J2-n" of [J2 §5]; "J3-Mn" of [J3 §6].

| Item | Problem | How it is fixed | Where |
|---|---|---|---|
| J1-1, J2-2 | C's `≺_S` ("vector clocks over templates") is a may-analysis; no algorithm on branching/looping skeletons | Replaced by A's MustBefore greatest fixpoint, with meets over predecessors (T-TERM excluded), T-SPAWN as a meet over all spawn sites, T-OBS only with complete writer sets including the initial value, and no lock rule. Complexity stated. Independent checker with mutation tests. Soundness lemma stated and sketched | §1.9, §4.4, §4.5, §6.2 L-MB |
| J2-1, J3-M1 | A's FIXED_ORDER (use ∈ MustBefore(writer), dominance) fails on R2's early returns | FIXED_ORDER certified by a **frontier event** m in the use's actor instance with no skeleton path m →* u and m ∈ MustBefore(x) for every writer x (PWB). The R2 regression (early returns at `cfe_es_task.c` L340–393; raise after init failure at L114–134, re-read) is an S3 exit test | §4.5, §6.2 L-FO, App. A.1 |
| J1-2, J2-6, J3-M1 | Writer sets and mod-ref unspecified; not transition-aware | `kz-modref`: Andersen-style, field-based, flow-insensitive points-to over the closed IR; memset/memcpy-aware; interprocedural out-pointer summaries (`CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, `cfe_sb_task.c` L138, re-read); transition-aware writer filtering by guards. Cross-checked against Φ effects (O7). T-OBS disabled when an `unknown_effect` may write; disablement counts reported | §4.2, §2.5 |
| J1-3 | Semantics set; fairness; stall; containment; transfer | σ_U, σ_U∞ (weak fairness, `stall` outcome), σ_R1, σ_E; timeout-dependent refinement; containment under A7; transfer rules (down for universal verdicts, up for witnesses, never into σ_E frequencies) | §1.8, §6.2 |
| J1-4, J2-5 | LPs undefined; M_api vs M_impl | Every Φ effect declares an LP; Ready is evaluated at the LP; outcome codes are a consistency check (O4). M_api makes each API atomic at its LP; M_impl splits OSAL idmap/task/binsem/queue paths and makes internal locks constrain interleavings, never shapes. Labels portability-hazard and implementation-defect-candidate | §1.4, §3.2, §4.8 |
| J1-5, J2-12 | Allowed outcomes need a reference horizon; route/capacity harm | Finite Allowed vocabulary, each entry with a horizon from a reference (retry bound, next operation on the same resource, task exit, main-loop horizon h). `unspecified_by_reference` → harm unspecified; LOSS-UNDER-ORDER for route/capacity, with SB counters as oracle | §1.6, §3.9 |
| J1-6, J2-13 | DPOR correctness with local nondeterminism and blocking | Local choices are explicit transitions; dependence includes await-predicate reads vs enabling writes and all enabling/disabling pairs; source-DPOR with sleep sets and explicit enabledness (JACM disabling extension cited as P only); conservative priority dependence under σ_R1; BPOR whenever a preemption bound is used; every bound swept; differential test against naive enumeration | §4.10, §6.2 L-DPOR |
| J1-7, J2-3, J3-M2 | Lock reversal must not need a solver; SHB/WCP/SyncP as deciders | Explicit-state, lock-reversing search over Pred(π) with the registry key as conflict object is the decider. SHB/WCP/SyncP are prefilters inside their soundness domain only; "decide" wording removed. Extrapolated witnesses labelled model-based. SMT-LIB only as an optional cross-check after PI decision D5 | §4.9 |
| J3-M2 | B's routing of the App-record window; O-ordered pairs | Script-app window recorded as ES-lock-ordered (stratum O, conflicting critical sections). Routing rule: pairs ordered only through O-edges go to prediction explicitly | §4.8 step 5, App. A.3 |
| J1-8, J3-M9 | Model–binary link | Per-actor skeleton acceptance (O1/O2) on every trace; global conformance (O8) within a recorded budget on a sampled set. Non-conformance → `unknown(model_gap)`; an observation contradicting a model class → `unknown` + model-fault record | §2.5, §4.11 |
| J1-9, J2-7 | Verdict record; latent instances | Orthogonal axes as listed in §0.1; every census instance gets a class or `unknown(reason)`; restart, ground-command and table variants are B.Input or a new basis B′ | §1.10, §7.4 |
| J1-10 | Facts and examples | Corrected: script-app window is O-ordered; core-task window added as an open-census test (INFERENCE until run); 429 `\retval`; SB-AppId uses 240–258 total, 211–229 ordered; CF bound is the build's effective `CF_INTERNAL_CFGVAL` value | §0.5, App. A |
| J2-8 | HS `linux_sysmon` is not NO_PROVIDER | New class `capability-absent(B)`: provider present, platform capability absent in B.Env. Erratum for [R03 §3 row 20] recorded here (inputs are read-only) | §1.10, §0.5, App. A.6 |
| J2-9 | Version-robust site binding | No line-number probes. Internal sites are bound by AST pattern (function, field, operation) plus the gcc DWARF of each basis; profile rules carry version guards | §3.17, §4.1 step 7 |
| J2-10, J3-M10 | Confirmation safety | Held thread's program-lock set empty (incl. ES shared-data lock, `DeleteChildTask` under the ES lock); no hold in timebase `handler_mutex` callbacks, binsem internal sections or OSAL RESERVED windows (except declared experiments); watchdog = smallest reference timeout the hold can make expire; debug-register gates; wait-for oracle; labels `derived`, `timeout-exercised (σ_U witness)` vs natural σ_E | §4.13, §5.9 |
| J2-11, J1-15, J3-M4 | Observation obligations; return-value cost | Stage S1 must measure return-probe, BPF-body and controller costs, test `.so` pre-registration, decide D1/D2, re-baseline group A on `-fc-v114`. Entry-only lock/queue reconstruction; return capture only at contract-use sites; capture mechanism chosen by measurement; per-probe cost class | §5.2, §5.10, §9.3 |
| J2-4 | Never HB from timestamps | Timestamps are evidence only. Per-object order from in-critical-section tokens or matched protocol keys. In the tracefs fallback, clock-ordered lock edges are excluded from HB and certificates | §5.4, §5.8 |
| J2-14, J1-11 | MLIR gate; staged MVC | MLIR only behind G-MLIR with stated criteria; no ODS/TableGen unless it passes; contribution independent of MLIR. Minimum viable core S0–S3 | §2.6, §9.3 |
| J2-15, J3-M5 | Scope order | S0 formats → S1 observer (R2 re-observed: 24/boot, ES 20, EVS 4) → S2 trace HB → S3 static census and certificates → S4 loop → S5 prediction → S6 confirmation → S7 explorer (gated) → S8 MLIR gate | §9.3 |
| J1-12, J2-16, J3-M11 | Anti-circularity and governance | Two readiness oracles with agreement checked; provenance, authoring time and cases inspected per Φ entry; freeze F1 before case runs and F2 before held-out runs; template-only first; refinements labelled; authoring effort measured; lints | §7.2 |
| J1-13 | Openness | Union of discovery sources (11 sources) | §7.1 |
| J1-14, J3-M8, J3-M12 | Evaluation, ablations, baselines | Fidelity metrics; ablations including no-trace (≈A), no-static (≈B), loop, obligations, σ_U∞, robustness tags, M levels, identities, conditional edges, atomic APIs, implementation-level locks as synchronization; baselines reimplemented over the same traces; external tools `NOT_RUN`; two raters | §8 |
| J3-M3 | Explorer state explosion | Key-level dependence (locks as atomicity, Lemma L3) plus L2, cones and trace-anchored windows; budgets and preemption bounds swept; decided share per budget; gated late stage with a kill criterion | §4.10 |
| J3-M6 | Perturbation and statistics | Groups A, B, C′, C, D, W; distributions; cold/warm state; interleaved order (assumption recorded); Mann–Whitney U / Fisher with effect sizes and binomial CIs; N pre-registered by the PI | §8.6, §8.7 |
| J3-M7 | Loop reproducibility | Run IDs, B hash and plan hash per iteration; fixed point stated with its run set; σ_E pooling only over identical (B, plan hash); persistent state S0 snapshotted per run; deterministic replay from stored traces | §4.14, §8.7 |
| J2-17 | Identity and persistence | Four identities plus on-behalf frames; B.Init snapshots; run sequences PO→PR→PO; cross-run keys (type, name, creator, creation ordinal) | §1.2, §1.7, §3.5 |
| J3-M12, J1-14 | NOT_RUN never a false negative | Stated in metrics and claims | §8.3, §6.4 |

### 0.5 Errata to the inputs (recorded here; the inputs are read-only)

| Input | Statement | Correction | Evidence |
|---|---|---|---|
| [B §7.2] | Script-app App-record window is "unordered → P1 (SHB)" | The publication (`cfe_es_apps.c` L861–869) and `CFE_ES_GetAppID` (`cfe_es_api.c` L738–765, re-read) both hold the ES shared-data lock. In any trace with mutex probes the pair is ordered by a lock edge (stratum O). Its reversal reverses conflicting critical sections and needs the lock-reversing predictor | [J1 §1 check 3]; [J3 §2.2] |
| [B §2.5] | "452 `\retval`" | 429 `\retval` and 138 `\par Assumptions` in `cfe/modules/core_api/fsw/inc`; 452 counts `\retval` + `@retval` | [J1 §1 check 7]; [J2 §1 check 8] |
| [C §5.9] | "211–258 SB-AppId uses per boot" | 240–258 total `sb.appid` uses per boot, of which 211–229 were ordered (CORD r2_c4–r2_c6); 199/205/205 ordered under gdb | [R2c §7]; [R2g §2] |
| [B], [C] | CF retry bound "25, `cf_internal_cfg.h:173`" | `CF_STARTUP_SEM_MAX_RETRIES` is `CF_INTERNAL_CFGVAL(STARTUP_SEM_MAX_RETRIES)`; 25 is `DEFAULT_CF_STARTUP_SEM_MAX_RETRIES` (re-read). The bound used is the value constant-evaluated in the build's own TU | [J1 §1 check 6] |
| [A §1.6, §3.17], [R03 §3 row 20] | HS's `linux_sysmon` lookup is NO_PROVIDER | `linux_sysmon` is in the PSP fixed module list (`psp/fsw/pc-linux/psp_module_list.cmake` L9, re-read). A failed `CFE_PSP_IODriver_FindByName` is handled (syslog, `CFE_SUCCESS`, `hs_sysmon.c` L50–54, re-read). The measured failure is the `SET_RUNNING` command returning failure → `CFE_STATUS_EXTERNAL_RESOURCE_FAIL` (L65–72), because `/proc/schedstat` is absent ([R01 §2.6]). Class: `capability-absent(B)` | [J2 §1 check 13] |
| [A §A.1 step 6], [R06 §5.5] | Post-publication SB-AppId uses (and TBL identity uses) are expected PROTECTED by certificate | Not provable as stated: SB's and TBL's own `CFE_ES_GetAppID` (`cfe_sb_task.c` L138; `cfe_tbl_task.c` L196, both re-read, both ignoring the status) can fall in the **core-task App-record window** (`cfe_es_start.c` L810–818, re-read; the child's poll checks only the task record, `cfe_es_apps.c` L570–590). `GetAppID` then writes `CFE_ES_APPID_UNDEFINED` (`cfe_es_api.c` L758–760, re-read) and the identity stays unset for the run. Expected first verdict: `schedule-dependent(σ_U)`, shape ABSENT, chained to the window instance. **INFERENCE until the census runs** | [J1 §1.1]; [LED §3.4], X5 |
| [C §3.6] | K-PHASE provenance from the state-enum comments | Primary provenance is the documented API assumption `cfe_es.h` L400–403 (re-read) | [J2 §1 check 7] |
| [B §4.4 P2] | WCP "decides" reversibility | WCP is sound and linear-time; only SyncP is sound and complete for its (sync-preserving) class | [J1 §1 check 12]; [R04 §2.5] |

---

## 1. Problem definition

### 1.1 Scope: "process-level"

Following v0.1 §2.1 and [R06 F1], *process-level* means ordering between the logical work of different cFS applications, service tasks and execution contexts, independent of OS process boundaries. In the native POSIX port they are all threads of one process ([F] [R05 §1 F1]; `osal/src/os/posix/src/os-impl-tasks.c:568`). The objects of interest are **framework resources**: named OSAL objects, ES app/task/lib records, service identities, SB routes and pipes, pipe capacity, TBL registry entries, phases and app states, CDS blocks, callbacks, loader symbols and files. Raw memory words are not objects of interest.

A memory-level data race found in passing (e.g. the unlocked EVS filter counters, [R02 §3.3]) goes into a separate class **M**. It is never counted as an order violation ([R02 §8.4 item 7]).

### 1.2 Actors, instances and the four identities

**Actors** `T = T_osal ∪ T_raw ∪ T_env` [F] ([R01 §2.6]; [R02 §1.1]):

- `T_osal`: tasks created by `OS_TaskCreate`. These are cFE core main tasks, app main tasks and child tasks (`ES_BG_TASK`, TIME's children, FM's child, …).
- `T_raw`: threads without an OSAL task record. These are the PSP main thread (the "ES startup thread" running `CFE_ES_Main`, later the idle loop), the console writer, the timebase thread `cFS-Master`, signal-handler contexts, and the `linux_sysmon` worker (absent here, [R01 §2.6]).
- `T_env`: environment roots (ground commands, SBN peers, the stop command). They have no incoming HB edges ([R04 §4 row 17]).

A **static actor** is a closure entry: a spawn site plus an entry function (§3.1). An **actor instance** is `(static actor, generation)`. A restart creates a new generation. Under the default basis every static actor has exactly one instance. Certificates are stated per instance (§4.5).

**Identity record** per instance: `⟨kind, osal_id, serial, tid, name, creator_event, generation⟩`. OSAL IDs are `(type << 16) | serial` and change across runs. Cross-run correlation therefore uses `(type, name, creator, creation ordinal)`, never IDs ([F] [R01 §1.2, §6.2 item 3]).

**Four identity fields per event**, never merged ([R06 F1]; v0.1 §2.1):

| Field | Definition | Why it is separate (cFS fact) |
|---|---|---|
| `exec(e)` | the actor instance that executes `e` | SB code runs in the caller's task ([R02 §2.4, §2.8]) |
| `owner(e)` | the ES app whose task set contains `exec(e)` at `e` (generation-aware); `⊥` for raw threads | an app has a main task and child tasks ([R02 §1.12]) |
| `svc(e)` | the stack of active frames: **API frames** (SB, EVS, TBL, …) and **on-behalf frames** `cb(registrant)` | TIME synch callbacks run in TIME's task on behalf of the registering app ([R02 §5.3]); FS write callbacks run in `ES_BG_TASK` on behalf of SB/TBL ([R02 §6.1]); OSAL timer callbacks run in the timebase thread on behalf of the timer's creator ([R01 §3]) |
| `stamp(e)` | an identity value the code passes or stores explicitly (`AppID` argument of `CFE_EVS_SendEventWithAppID`; pipe owner stored by `CFE_SB_CreatePipe`) | R2: the ES task executes SB code that stamps SB's unpublished AppId ([R2g §2]) |

### 1.3 Resources, kinds, lifecycle automata, generations and keys

A resource is `r = ⟨kind, scope, key, gen⟩`.
- `scope ∈ {process, persistent}`. Persistent resources survive the process: CDS, boot record, reset area, files on `/dev/shm/osal:RAM`, `EEPROM.DAT` ([R01 §5.3]).
- `key` is a name, numeric ID, `(MsgId, pipe)` pair, path or symbol. In static mode a key may be **symbolic** (e.g. "the string in CF table field `chan[i].sem_name`") or `⊤` (unresolved).
- `gen` is the OSAL serial, the cFE resource-ID serial ([R02 §6.2]) or a restart count.

Each kind `k` has a lifecycle automaton `A_k = (Q_k, q0_k, Σ_k, δ_k)` (typestate; Strom & Yemini, IEEE TSE 1986 [V-me, R06 §9]). Anchors are in [R01 §1.5–1.7] and [R02 §1–7]; the full table is in §3. Examples:

| Kind | States | Publication anchor (LP) |
|---|---|---|
| `osal.obj<T>` | FREE → ALLOC → RES_C (name invisible) → RES_CN (name visible, still RESERVED, type lock **not** held) → LIVE → RES_D → FREE | final ID write under the type lock, `osapi-idmap.c:1092`, unlock `:1105` ([R01 §1.5]) |
| `es.app` / `es.task` / `es.lib` | FREE → RESERVED → LIVE → RESERVED (cleanup) → FREE | script apps: reserve `cfe_es_apps.c:821`, publish `:869`; core apps: reserve `cfe_es_start.c:803`, publish `:818` (re-read); task record `cfe_es_apps.c:694–703` |
| `svc.identity(S)` | UNSET → SET → REGISTERED | SB `cfe_sb_task.c:138` then `:155`; EVS `cfe_evs_task.c:312`; TBL `cfe_tbl_task.c:196` |
| `evs.reg(app)` | UNREG → REG | `cfe_evs.c:106`, no lock ([R02 §3.1]) |
| `sb.pipe` | FREE → RES (OSAL queue name visible) → LIVE → RES_D → FREE | `cfe_sb_api.c:153,163,190–194` |
| `sb.route(m,p)` | NONE ↔ ROUTED | `CFE_SB_SubscribeFull` under the SB lock ([R02 §2.3]) |
| `sb.queue(p)` | depth `0..cap_B(p)` | `cap_B(p) = min(requested, msg_max)`; [M] `msg_max` 10, 22 of 26 pipes truncated ([BM §3.1]) |
| `es.phase` | EARLY_INIT < CORE_STARTUP < CORE_READY < APPS_INIT < OPERATIONAL (monotone) | `cfe_es_start.c:87,183,194,216,234` |
| `es.appstate(app)` | UNDEFINED < EARLY_INIT < LATE_INIT < RUNNING < WAITING < STOPPED | `cfe_es_api.c:571–574`, `:469–472`, `:399` (external apps only, re-read); scan `cfe_es_apps.c:1139–1146` (external apps only, from RUNNING, re-read) |
| `tbl.table(name)` | FREE → REGISTERED → LOADED → ACTIVE, with pending substates and an access lock flag | [R02 §4.2–4.5] |
| `es.cds(name)` (persistent) | ABSENT → REGISTERED(new \| restored) | `es/cfe_es_cds.c:311–477` |
| `time.synchcb(app)` | EMPTY → SET → EMPTY | `cfe_time_api.c:614–645`; cleanup `cfe_time_utils.c:1018–1041` |

### 1.4 Operations, effects, linearization points and model levels

An **operation** `op ∈ Φ.ops` has a binding (symbol or AST pattern, §3.17), parameters with roles (name, id, out-id, AppId, buffer), an outcome map from result codes to `{ready, not_ready(q), error}`, and a list of **effects**. An effect is one of:

- `lifecycle(r, a ∈ Σ_k) when guard`;
- `use(r)`;
- `sync(obj, role)`, with `role ∈ {acq, rel, give, take, flush, put, get, create, start, exit, join, observe, timeout}`;
- `env(input)`;
- `mark(handler_begin | handler_end)`.

**Every effect declares its linearization point (LP).** The LP is the instruction-level event inside the API at which the effect takes place [D] (fixes [J1 X1]). Examples [F]:

| API | Effect | LP |
|---|---|---|
| `OS_*Create` | `lifecycle(osal.obj, name_visible)` | `OS_OBJECT_INIT` writes `name_entry` (`os-shared-idmap.h:106–111`), type lock released |
| | `lifecycle(osal.obj, publish)` | final ID write under the type lock, `osapi-idmap.c:1092` |
| `OS_*GetIdByName` | `use(osal.obj(name))` | the name search under the type lock |
| `CFE_ES_GetAppID` | `use(es.self(exec))`; out-pointer store | `CFE_ES_GetAppRecordByContext()` under the ES lock (`cfe_es_api.c:748–763`, re-read) |
| `CFE_SB_CreatePipe` | queue-visible, then commit | `OS_QueueCreate` (`cfe_sb_api.c:163`), then commit under the SB lock (`:185–203`) |
| `CFE_EVS_Register` | `lifecycle(evs.reg(owner), REG)` | store at `cfe_evs.c:106` |
| `CFE_EVS_SendEventWithAppID` | `use(evs.reg(arg.AppID))` | the slot check, `cfe_evs.c:185–194` |
| `CFE_ES_WaitForSystemState` | raise (`lifecycle(es.appstate(owner))`), then `observe(SystemState)` | the store under the ES lock at `cfe_es_api.c:571–574`, only if `GetAppRecordByContext()` ≠ NULL (L533–576, re-read); then each unlocked read of `SystemState` (L585) |

**Readiness is evaluated at the use's LP.** The program's own outcome code at the use is a consistency check on the computed state (obligation O4, §2.5).

**Model levels** (A's M_api/M_impl, [A §1.5]) [D]:

- **M_api**: each OSAL API is one atomic transition at its main LP. Create is atomic with respect to lookup; lookups return found or not found. This level is portable across OSAL backends.
- **M_impl**: for the OSAL idmap, task, binsem and queue paths, the skeleton builder projects the OSAL shared and POSIX source itself onto a lower vocabulary Φ_L-1 (pthread mutex and condvar, `sem_*`, `mq_*`, `clock_nanosleep`, `pthread_create/cancel/join`). The API is split at its LPs, the RESERVED windows appear, the bounded RESERVED wait consumes attempts on every unlock of the type, and the lock-leak path exists ([R01 §1.5–1.8, §2.2]).
- OSAL-internal locks (type tables, binsem internal mutex, console, timebase `handler_mutex` except where it is the documented ordering mechanism) **constrain interleavings only under M_impl**. They **never** create shape-HB edges at either level ([R01 §3, §6.2 item 2]).
- When the levels disagree, the report says so:
  - PROTECTED under M_impl but violable under M_api → **portability hazard** (the protection depends on POSIX implementation detail);
  - violable only under M_impl → **implementation-defect candidate** of OSAL POSIX at the pin.

### 1.5 Events, templates, traces and concretization

An event is `e = ⟨id, actor, seq, op, phase, args, outcome, keys, site, ι(e), lp_interval, t⟩`:
- `seq` is the per-actor index (the program-order token);
- `phase ∈ {call, return, point}`;
- `outcome` is the result the program itself received ([R06 F11]);
- `ι(e)` is the identity quadruple;
- `lp_interval` brackets the effect's LP between two observed points (§4.8);
- `t` is a timestamp kept **as evidence only**: physical time gives no HB (Lamport, CACM 1978 [V-abs]).

A **trace** π is a sequence of events in which each actor's events appear in program order.

A **static template** has the same type with symbolic fields and a path condition. Templates are **context-qualified**: `(static actor, inlining call string, site)` [D]. The SB event site inside `CFE_SB_CreatePipe` reached from `CFE_ES_TaskInit` is therefore a different template from the same site reached from ES's command loop. Recursion makes the affected summary `unknown`.

A template `t` *concretizes* to an event `e` (`t ⊒ e`) iff the actor and call string of `e` match `t`, `site(e) = site(t)`, and the fields of `e` satisfy `t`'s constraints. This single type lets the static, trace and model layers share one checker.

### 1.6 Readiness contracts

A contract is `κ = ⟨id, U, sel, Req, Oracle, Allowed, harm, prov, authored, guard⟩`:

- `U`: a use pattern (operation plus site predicate).
- `sel`: a resource selector from event fields to a key (e.g. `osal.countsem(arg.name)`, `evs.reg(arg.AppID)`).
- `Req`: a conjunction `P_1 ∧ … ∧ P_n` of required lifecycle predicates on the selected resource(s), kept in conjunctive form.
- `Oracle`: the **framework validity predicate** for each conjunct, as a function of the analysed program. Examples: `CFE_ES_AppRecordIsMatch(CFE_ES_LocateAppRecordByID(a), a)`; `EVS_GetAppDataByID(a) != NULL && EVS_AppDataIsMatch(...)` (function names from [A §7.2], checked there in the pinned headers). The oracle is never executed by Kairos. Statically, its read set defines the **support locations** of the conjunct (§4.2). Dynamically, the program evaluates it itself, and its outcome code is the observed image of the conjunct (O4) ([J1 X9]: declared effects as the working model, framework predicates as an independent oracle).
- `Allowed`: documented not-ready behaviours from a **finite vocabulary**. Each entry has a **continuation horizon taken from a reference** [D] (fixes [J1 X2]):

| Allowed entry | Meaning | Horizon (reference) | Static check | Trace check | Explorer check |
|---|---|---|---|---|---|
| `none` | no not-ready outcome is acceptable | the use itself | — | — | — |
| `documented_drop(src)` | the API drops the request silently by documented design (e.g. EVS's own-ID guard, `cfe_evs_utils.c:613–623`) | the use itself | outcome class | outcome code | outcome |
| `returned_and_tested(codes)` | the use returns a documented code; the caller branches on it and reaches its normal continuation | until the next operation of the same actor on the same resource, or actor exit, whichever comes first | path check on the skeleton within the horizon | continuation events in the trace; trace ends early → `unknown(horizon)` | suffix |
| `retry_until_ready(codes, bound)` | the caller retries on the not-ready codes | the retry loop's bound, constant-evaluated in the build (e.g. CF's effective `CF_STARTUP_SEM_MAX_RETRIES`, §0.5) | loop summary (§4.3) | later iteration observed ready within the bound | suffix within the unrolled bound |
| `fallback(op′)` | the caller uses a documented alternative | the next operation of the actor | path check | next event | suffix |
| `defer_by_design(src)` | the reference documents the deferral (TO_LAB subscribes after OPERATIONAL, `to_lab_app.c:64–66`) | the documented deferral point | path check | event | suffix |
| `exit(code, src)` | a documented exit | actor exit | path check | event | suffix |
| `unspecified_by_reference` | no reference says whether the not-ready outcome is acceptable | — | — | — | — |

  `unspecified_by_reference` sets harm `unspecified_by_reference`. For route and capacity contracts with no must-deliver reference it also sets the outcome label **LOSS-UNDER-ORDER**.

- `harm`: the effect class when the outcome is not allowed. Vocabulary: `event_lost_silently`, `status_ignored`, `wrong_identity_stamped`, `app_error_exit`, `processor_reset_requested`, `task_never_runs`, `hang_or_deadlock`, `message_lost`, `unspecified_by_reference`.
- `prov`: provenance records `(kind ∈ {api_doc, code_admission, code_pattern, issue, researcher}, path:line, sha256 of the text)`.
- `authored`: authoring time, author, and which design cases (C1, C2, C3, C5, R2) the author had inspected for that API ([R06 §5.3 item 4]).
- `guard`: version guard (e.g. `cfe ≥ 6.6.0`) for profile reuse across revisions (§7.6).

**Instances.** An instance is `i = (κ, t, key)` for a context-qualified template `t` matching `U`. For a linearization λ of an execution and an occurrence `u` of `t`, let `state_λ(r, u)` be the state of `A_k` after the lifecycle effects on `r` (same generation) whose LPs precede `u`'s LP. The **per-occurrence outcome** is:

- `READY` iff `state_λ(sel(u), u) ⊨ Req`;
- `TOLERATED` iff not ready and the outcome and continuation satisfy `Allowed` within its horizon;
- `VIOLATING` iff not ready and not allowed;
- `NOT_READY_UNJUDGED` iff not ready and `Allowed = unspecified_by_reference`.

An execution is **bad** for `i` iff some occurrence is `VIOLATING`, or, for contracts whose Allowed is `unspecified_by_reference`, `NOT_READY_UNJUDGED`. The outcome label says which meaning of "bad" was used.

**Violation chains** ([v0.1] §8; [R06 §3]): `i₁ ⇝ i₂` when an occurrence of `u₂` exists only because of `u₁`'s not-ready outcome (control or data dependence; for traces, on-behalf/API frame containment plus Φ `emits` annotations). Example [F]: 17 of R2's 24 identity uses are `SEND_NO_SUBS` events from ES's EVS packets (MsgId 0x808) transmitted before any route ([R2g §2]).

### 1.7 The analysis basis B

`B = ⟨Build, Config, Init, Env, Input⟩`. Every item is recorded with its value, its source and its policy status (`from-reference` / `deviation` / `unspecified_by_reference`):

- **Build**: commits; gcc 13.3.0 and the recorded flags ([R06 K1]); `link.txt` archive semantics (whole-archive core, lazy BSP members, [R07 §3.4]); generated files by hash. These include at least `cfe_static_module_list.c` (core order, [R02 §1.3]) and the generated module version table, whose absence changes R2's count from 24 to 11 ([R2c §4.2, §7]).
- **Config**: the generated startup script (2 libraries + 15 apps, [R03 §6.1]); built tables (`*.tbl`) and their source initializers, cross-checked byte for byte against the gcc DWARF layout ([A §4.1 step 4]); platform constants as constant-evaluated in their TU; `osconfig.h` limits; the PSP module list.
- **Init**: persistent state before the run (boot record, CDS and reset-area segments, `/dev/shm/osal:RAM`, `EEPROM.DAT`) and the resulting reset type ([R01 §5.2–5.3]). A snapshot `S0` is taken per run ([BM §2.5] method).
- **Env**: kernel (now `6.18.44-fc-v114`, drifted from the recorded `-fc-v80`, [R07 §1]); 4 vCPU; `SCHED_OTHER` for all 27 threads ([M] [BM §0]); affinity (the PSP pins `CFE_*` tasks to CPU 0, `cfe_psp_start.c:184–189`, not yet measured, [R06 K5]); effective queue depth (`msg_max` 10); platform capabilities relevant to providers (`/proc/schedstat` absent, [R01 §2.6]); run user; cold or warm start ([BM §0]: the first run after a reboot reached OPERATIONAL at 2570 ms against 1613–1633 ms).
- **Input**: external commands with **event-based** triggers, and the stop method (still a PI decision, D17).

**Run sequences.** For cross-run contracts (K-PERSIST), the end state of run *n* becomes `B_{n+1}.Init`. The reset type of run *n+1* depends on how run *n* ended ([M] PO → PR → PO after signal stops, [R01 §5.2]).

Every verdict carries the hash of B. **A configuration variant is a new basis B′, never a "new bug"** ([R06 §5.4]).

### 1.8 Scheduling and timing semantics σ

`Exec(B, σ, M)` is the set of executions. Transitions are effects at LP granularity [D] (fixes [J1 X3, X10]):

| σ | Definition | Role |
|---|---|---|
| **σ_U** (untimed) | Any interleaving of enabled transitions. `OS_TaskDelay` is a yield. Every timed wait and every bounded retry may take its timeout branch at any point after it starts. Polls read the current value. Priorities and affinity impose nothing. `CFE_PSP_Panic` (→ `abort()`, `psp/fsw/pc-linux/src/cfe_psp_support.c:116–121`, re-read) is terminal. `CFE_ES_ResetCFE` → `CFE_PSP_Restart` is **not** terminal: the caller and the other threads may continue until the PSP's shutdown completes or aborts ([R01 §5.5]; [J2 §1 check 4]) | Reference semantics for certificates and model classification |
| **σ_U∞** (patient) | σ_U without timeout transitions: no timed wait times out, and no **delay-paced retry loop** (a count-bounded loop whose iterations are separated by `OS_TaskDelay`, e.g. CF's semaphore retry) takes its exhaustion branch. An await is enabled only when its predicate holds. Non-terminating loops are cut by the main-loop horizon h (§4.3) and assume weak fairness (every continuously enabled transition is eventually taken; fair stateless model checking, Musuvathi & Qadeer PLDI 2008 [V-abs, R05 §2.1]). A maximal execution in which some actor waits forever on a false predicate is a **`stall`** outcome, which is neither violating nor conforming | Separates "violable only through a soft timeout" from "violable in the ordering logic" |
| **σ_R1** (RT, one core) | Fixed-priority preemptive scheduling on one CPU. Priorities are those requested by the startup script and the core table ([R02 §0]). FIFO among equal priorities. Timeouts as in σ_U. Model-only by default. A native realization needs root-imposed `SCHED_FIFO` with single-CPU affinity, a deviation from E1/E4 ([R07 §5.2]; D4) | Platform classes where priorities order (C3 at 6.4.1, [LED §1a]) |
| **σ_E** (ENV) | The executions of the real binary under the recorded Env. Not modelled; sampled by observation and confirmation runs. Capacities and configuration enter the model as B values | Empirical witnesses and frequencies |

**Containment lemma** (proof sketch in §6.2, L-CONT). Under assumption A7 (Φ's operation semantics over-approximate kernel, libc and OSAL behaviour at LP granularity), A3 (closure), A4 (memory model), A9 (inputs) and A-SKEL (skeletons over-approximate each actor's behaviour under B):

`Exec(B,σ_U∞) ⊆ Exec(B,σ_U)`, `Exec(B,σ_R1) ⊆ Exec(B,σ_U)`, `Exec(B,σ_E) ⊆ Exec(B,σ_U)`.

**Transfer rules** [D]:

1. A universally quantified class (protected, tolerated, fixed-order) proved under σ_U holds under every σ′ ⊆ σ_U, for every execution of σ′ that reaches the use. If no execution of σ′ reaches the use, the σ′ class is `inactive_σ′`.
2. A witness found under any σ′ is a witness under σ_U.
3. Nothing transfers *into* σ_E as a frequency. A σ_U schedule-dependent verdict says nothing about σ_E frequency until σ_E witnesses exist.
4. A blanket "no transfer across σ" ([A §6.3 item 3]) is dropped as too weak. B's unsupported transfer of a trace-local certificate ([B §4.3]) is dropped as too strong.

**Timeout-dependent refinement.** For instance `i`, if its σ_U class is schedule-dependent and σ_U∞ has no bad execution within b (only good executions or `stall`), the σ_U class is refined to **timeout-dependent**: the violation needs a soft-barrier or poll timeout, or the exhaustion of a delay-paced retry. If σ_U∞ also has bad executions, the class stays schedule-dependent: the violation lives in the ordering logic itself. Confirmation runs in which a hold makes a timeout fire are labelled "timeout-exercised (σ_U witness)", never natural σ_E.

### 1.9 Ordering relations

Φ's HB rules are inference rules `e₁ → e₂` with a matching condition, a stratum, a robustness tag and a code citation.

**Strata** ([C §0]) [D]:

| Stratum | Meaning | Used for |
|---|---|---|
| **S** (structural) | Holds in every execution that contains both endpoints on the given path: program order within an actor (inside one handler for loopers), task create → child start, join, EarlyInit → tasks, script sequencing, and **guarded observation edges** whose writer set is complete and whose only alternative continuation is terminal | MustBefore and certificates |
| **O** (observed synchronization) | Holds in the observed execution only: outermost mutex release → next acquire, semaphore give → take, delivered queue/SB message, poll exit by condition with a soft (non-terminal) alternative, timer add/set → callback, TBL request and flag protocols, FS request → completion, looper order between handlers, outcome-implied reads-from | HB of a trace; reorderable in prediction, subject to the primitive's semantics |
| **C_σ** (schedule constraint) | Fixed-priority preemption on one CPU, affinity | Feasibility under σ_R1 only; never an HB edge ([R04 §4 row 20]; [R05 §2.7]) |

**Trace-side image: robustness tags** ([B §4.2]). Each trace edge carries one tag:
- `unconditional` (S-rules that need no observation);
- `obs-fatal` (S, guarded observation with terminal escape);
- `obs-soft` (O, log-and-continue escape);
- `schedule-chosen` (O: locks, semaphores, messages);
- `assumed` (holds only under assumption A4: volatile flags, seqlock, TBL result flags).

**Relations computed:**

1. **MustBefore** (static; §4.4). `a ∈ MustBefore(n)` iff in every execution of the model under σ_U in which `n` occurs, every occurrence of `n` is preceded by an occurrence of `a`. It is a greatest fixpoint over the per-actor skeletons, using S-rules only.
2. **Precedes-whenever-both-occur** `PWB(a, b)`: in every execution containing both, every occurrence of `a` precedes every occurrence of `b`. It is certified by the frontier rule of §4.5: `∃ m` in `a`'s actor instance with no skeleton path `m →* a` and `m ∈ MustBefore(b)`. Fixed-order and protected are certified with PWB and MustBefore, never by membership of the use in MustBefore(writer) ([J2-1], [J3-M1]).
3. **HB_π** on a trace π: the transitive closure of S- and O-edges, computed with vector clocks (Fidge 1988; Mattern 1989 [V-abs]). Two clocks are kept per event ([B §4.2]):
   - `VC` over all definite edges;
   - `VC_R` over the edges tagged `unconditional` or `obs-fatal`.

   `a ≺_R b` is a cheap trace-side prefilter for a static certificate. It is **never** an Exec-level claim by itself ([J1 X5]).
4. **C_σ** constraints only restrict σ_R1 feasibility.

Rules not in Φ are listed as **unmodelled**. A static loop that looks like synchronization but matches no rule (a poll on a shared field with `OS_TaskDelay`) becomes an `unmodelled-sync` mark, and every instance whose decision depends on it is `unknown(unmodelled)`. This is the KCSAN lesson that an HB model missing one ordering mechanism floods false positives (`kcsan.rst` L366–373 [V-doc, R04 §2.4]).

### 1.10 Classes, refinements, shapes, evidence and the verdict record

Fix instance `i`, basis B, semantics σ, level M and bound b. Let `E_i` be the executions of `Exec_b(B,σ,M)` that reach the use.

| Class | Definition | Proof obligation |
|---|---|---|
| `inactive(B)` | no execution reaches the use | static: the template is unreachable in the configuration-closed skeleton (path condition as proof). An unobserved use is **not** inactive; it is `unknown(not_reached)` |
| `no-provider(B)` | the use is reachable, some conjunct is false initially, and no transition of the model can make it true | empty complete writer set for that conjunct (§4.5) |
| `capability-absent(B)` | a provider transition exists in code, but its enabling depends on an environment capability recorded as absent in B.Env | Φ effect guarded by an `env` predicate; B.Env record; outcome agreement (O4). Example: HS `SET_RUNNING` (§0.5, App. A.6) |
| `protected(B,σ,M,b)` | `E_i ≠ ∅` and no execution in `E_i` is bad | certificate (§4.5, b = ∞) or exhaustive exploration within b |
| ↳ refinement `tolerated` | protected, and some execution has a `TOLERATED` occurrence | as protected, plus a tolerated witness |
| `fixed-order(B,σ,M,b)` | `E_i ≠ ∅` and every execution in `E_i` is bad | certificate (§4.5, b = ∞) or exhaustive exploration within b. **Never** from k-of-k failing runs ([R06 S9]) |
| `schedule-dependent(B,σ,M,b)` | `E_i` contains bad and good executions | two witnesses, each predicted, model-derived, observed or confirmed. The record keeps the weakest evidence of the pair |
| ↳ refinement `timeout-dependent` | schedule-dependent under σ_U, and no bad execution under σ_U∞ within b | §1.8 |
| `unknown(reason)` | none of the above established | reason ∈ {`not_reached`, `model_gap`, `bound`, `budget`, `key_unresolved`, `loss`, `unmodelled`, `writer_incomplete`, `horizon`, `conformance`, `model_fault`} |

**Shape** ([A §1.7]), for a bad occurrence `u` with `p*` the first transition that would make the conjunct true:
- **WRONG-WAY**: `u →hb p*`;
- **UNORDERED**: `u ∥hb p*`;
- **ABSENT**: no `p*` in the execution;
- **STALE**: the conjunct held earlier, then a disabling transition occurred before `u` (generation mismatch).

Shapes always use API-level edges.

**Evidence** (separate axis, may hold several values) ([R06 S1]):
- `static-certificate(id)`;
- `model(b)`;
- `predicted` (a linearization of Pred(π), trace-sound);
- `predicted-extrapolated` (uses summary-guided extrapolation beyond π; model-based);
- `observed(n of N)` with the pool key (B hash, plan hash);
- `confirmed(F0|F1)` with flags `derived` and `timeout-exercised`.

**Outcome label**: `violation` (Allowed specified) / `LOSS-UNDER-ORDER` (route or capacity, Allowed unspecified) / `not-ready-unjudged` (other contracts, Allowed unspecified).

**Consequence** ([A §1.3], effect-oriented as in ConMem and Portend [V-abs, R04 §2.6, §2.10]): crash or undefined behaviour; deadlock; app error exit; processor reset requested; status ignored and effect lost silently; event then continue; delayed but recovered.

**Sub-labels**: `portability-hazard`, `implementation-defect-candidate` (§1.4); `assumed-visibility` when any certificate edge is tagged `assumed` (§3.14).

**The verdict record** (`kz_verdict`, §2.3) holds:
- instance id, `σ`, `M`, `b` (reference to a BOUNDS record listing every bound and its policy status), B hash, Φ hash, tool hash;
- class and refinement;
- evidence bitmask with references (certificate, witnesses, runs);
- shape, outcome label, consequence, harm;
- chain links;
- the reason (if `unknown`);
- per-σ results, and the M level at which each holds.

There is never a "safe" field.

### 1.11 Why plain data-race HB detection is insufficient, and what theory is extended

| Phenomenon in cFS | What an HB or lockset detector does | What Kairos adds |
|---|---|---|
| **R2**: all 24 violating uses per boot are HB-ordered **before** the publication by core serialization ([R2g §2]; [R2c §7]) | Reports nothing. FastTrack (PLDI 2009 [V-abs]), TSan and Djit+ report unordered conflicting accesses; CP, WCP, SHB, M2 and SyncP (all [V-abs]) report reorderable pairs only ([R04 §3 L4]) | Contract checking on **ordered** pairs, and a PWB certificate that lifts "ordered the wrong way" to `fixed-order(σ_U)` |
| **C1 / C5** and both App-record windows: create/lookup or publish/`GetAppID` under the same lock ([LED §1a]; §0.5) | HB orders them through the lock in the observed direction; lockset sees consistent protection ([R04 §2.3, §3 L5]) | The registry **key** is the conflict object and the lock gives atomicity, not order. Lock edges are stratum O and reorderable in prediction. Lock-order reversal is outside SyncP's tractable class; in general it is NP-complete and W[1]-hard ([R04 §2.5]) |
| Losses: no-subscriber drop returns success; queue full drops silently ([R03 §1]; [M] 4–49 drops per boot, `NoSubscribersCounter` 15–26, [BM §0]) | No HB rule covers them (gap G5, [R04 §5]) | Outcomes are first-class: route and capacity states in the typestate; prediction and exploration ask whether a feasible order changes the outcome |
| Soft, opt-in, timed barriers (`WaitForStartupSync` returns void, `cfe_es_api.c:616–619`) | Unconditional edges or false positives (gap G3) | Conditional O-edges, guarded S-edges, σ_U/σ_U∞ split, timeout-dependent refinement |
| Executing task ≠ owner ≠ service ≠ stamped AppId (gap G6) | thread identity only | four identity fields and on-behalf frames |
| Configuration decides reachability and capacity (gap G7) | configuration taken as given | B with configuration closure; `inactive`, `no-provider`, `capability-absent` |
| Priority order only on RT targets (gap G8) | no edge, but no verdict either | σ_R1 as a schedule constraint, verdict per σ |

**Established theory extended.** No mechanism is claimed as new ([R04 §5], last paragraph):

| Element | Source (status) |
|---|---|
| HB as a partial order; vector clocks | Lamport 1978; Fidge 1988; Mattern 1989 [V-abs] |
| Feasible vs apparent races; order violations as a major class | Netzer & Miller LOPLAS 1992 [V-abs]; Lu et al. ASPLOS 2008 [V-abs] |
| Framework/event HB rules (looper, post/begin, RPC/event) | DroidRacer PLDI 2014 [V-abs]; CAFA PLDI 2014 [V-abs]; DCatch ASPLOS 2017 [P] |
| Typestate contracts | Strom & Yemini 1986 [V-me] |
| Def-use invariants; generic predictive properties | DefUse OOPSLA 2010 [V-abs]; GPredict ICSE 2015 [V-abs] |
| Sound prediction with control-flow abstraction; candidate + vindication | Maximal causal models RV 2012 [V-abs]; RVPredict PLDI 2014 [V-abs]; Vindicator PLDI 2018 [V-abs] |
| Ad hoc synchronization; race coverage | Xiong et al. OSDI 2010 [V-abs]; Tian et al. ISSTA 2008 [V-abs]; EventRacer OOPSLA 2013 [V-abs] |
| Initialization races | InitRacer OOPSLA 2017 [V-abs] |
| Systematic exploration | DPOR POPL 2005 [V-abs; guarantee wording UNVERIFIED]; source/optimal DPOR POPL 2014 [V-abs; JACM extension P]; BPOR OOPSLA 2013 [V-abs]; fair SMC PLDI 2008 [V-abs]; PCT ASPLOS 2010 [V-abs; mechanism P]; SCTBench PPoPP 2014 [V-abs] |
| Static action HB; environment models; harnesses | SIERRA ASPLOS 2018 [V-abs]; SDV EuroSys 2006 [V-abs]; Holzmann & Joshi SPIN 2004 [V-abs]; FlowDroid PLDI 2014 [V-me] |
| Points-to and indirect calls | Andersen 1994 [V-abs]; SVF CC 2016 [V-abs]; MLTA CCS 2019 [V-abs] |
| Triggering and confirmation; outcome triage | CTrigger ASPLOS 2009, RaceFuzzer PLDI 2008, RacePro SOSP 2011, Portend ASPLOS 2012, DataCollider OSDI 2010 [all V-abs] |
| Log/order mining and potential communications | CloudRaid ESEC/FSE 2018 [V-abs]; Snowboard SOSP 2021 [V-abs] |
| Belief deviance | Engler et al. SOSP 2001 [V-me] |
| Soundiness | Livshits et al. CACM 2015 [V-abs] |

---

## 2. Architecture

### 2.1 Layers and principles

```
L5  Orchestration & reporting      kz loop (feedback driver), kz report, kz dump, kz proofcheck (independent)
L4  Real-binary observation/control kz observe (F0: uprobes + eBPF raw loader | tracefs fallback),
                                   kz hold (F0: ptrace + debug-register gates), kz gate (F1, PI-gated)
L3  Analyses (three interpreters)   abstract: kz static (census, MustBefore, certificates)
                                   concrete: kz trace (identity, HB, contracts, routing), kz predict, kz conform
                                   operational: kz explore (gated, late)
L2  Formal core                     libkz-core: KEM types, Φ DSL parser/interpreter, rule engine, typestate,
                                   vector clocks, verdict lattice, provenance, KZ container I/O  (no LLVM dependency)
L1  Static closure & skeletons      kz basis, kz extract (Clang), kz link (LLVM IR + DWARF/MC binder),
                                   kz modref (points-to, mod-ref, writer sets), kz skel   (libkz-llvm)
L0  Inputs                          basis B (build tree, configuration, Env, Init, Input), profile Φ_cFS (text)
```

Principles [D]:

1. **One formal core.** Every component reads and writes KEM types through `libkz-core`. No component has a private notion of event, resource, edge or verdict.
2. **The profile is data, not code.** Framework knowledge lives in versioned Φ files with provenance. Engines are framework-neutral, and a lint forbids case identifiers in engine code (§9.2).
3. **Proof-carrying verdicts.** Every certificate is a list of rule instances and writer-set entries with code sites. `kz proofcheck` shares no code with `kz static` and re-validates every certificate. Mutation tests check that a corrupted certificate edge is rejected.
4. **Observation first, control second.** F0 observation of the unmodified gcc binary is the default ([R07 §7]). Control is used only for confirmation, and every hold is recorded.
5. **Layers check each other.** Obligations O1–O8 (§2.5) run after every run and every rebuild of the model.
6. **Honest unknowns.** Every gap produces `unknown(reason)`.
7. **Zero-install core.** Every core capability works without new packages: raw `bpf()` loader or tracefs, explicit search instead of SMT, plain C++ instead of MLIR. libbpf, Z3, the F1 `--wrap` variant and native `SCHED_FIFO` PCT are recorded PI decisions (§10.1).

### 2.2 Components

| Component | Layer | Language, libraries | Inputs → outputs | Why this technology (measured or argued) |
|---|---|---|---|---|
| `libkz-core` | L2 | C++17, standard library only | — | shared formal core; independent of LLVM, so that trace and model tools do not link Clang |
| `kz basis` | L1 | C++17 | build tree, `link.txt`, generated files, startup script, `.tbl`, Env/Init snapshots → `basis.kz` | `compile_commands.json` and CMake files are *inputs* produced by the reference build, not tool formats |
| `kz extract` | L1 | C++17, Clang LibTooling + ASTMatchers (LLVM 23 `1053047a`) | 448 FSW compile commands → `facts.kz` | [M] 448/448 TUs parse in 10 s with two diagnostic-only adaptations plus `-resource-dir` ([R07 §3.3]); macro provenance (467 of 3,762 API call sites in macro expansions, [R07 §4.6]); constant evaluation of tables and of `CF_INTERNAL_CFGVAL`-style macros in their TU; Doxygen `\retval`/`\par Assumptions` extraction |
| `kz link` | L1 | C++17, LLVM IR linker semantics, LLVM DebugInfo/DWARF, Object, MC | per-unit bitcode; the **gcc** `core-cpu1` and app `.so` → call graph, `anchors.kz` (site → gcc address, instruction cost class), anchor audit | [M] bitcode 4.4 s, closed `core-cpu1.bc` 0.35 s with 0 undefined `CFE_`/`OS_` symbols ([R07 §3.4]); DWARF and MC bind Clang sites to the executed gcc binary ([R06 T7]) |
| `kz modref` | L1 | C++17 over LLVM IR | closed IR + closure + Φ → points-to, mod/ref summaries, support locations, writer/disabler sets, O7 report | §4.2. Andersen-style inclusion analysis ([V-abs, R05 §2.5]); SVF ([V-abs]) is the design reference, not a dependency (SVF README lists LLVM 21–22; the build is LLVM 23) |
| `kz skel` | L1 | C++17 | IR + closure + Φ → per-actor skeleton automata `skel.kz` | projection of the inlined CFG onto Φ effects, support-location accesses and handler marks; loop summaries (§4.3) |
| `kz static` | L3 | C++17 on `libkz-core` | skeletons + writer sets + Φ → census, MustBefore, certificates, tier-1 verdicts, open candidates | §4.4–4.6 |
| `kz proofcheck` | L5 | C++17, separate module | certificates + skeletons + facts → accept/reject per edge | independent re-validation; mutation-tested |
| `kz plan` | L3 | C++17 | open candidates + mandatory sync skeleton + anchors + calibration table → `plan.kz` | weighted set cover with a per-probe cost class (§4.7) |
| `kz observe` | L4 | C (one generic BPF program per phase, system clang-18 `-target bpf`); C++17 loader on raw `bpf()`/`perf_event_open`; tracefs backend | plan + binaries → `run-*.kzt` | [M] uprobe 0.57 µs at entry+4; eBPF attaches to uprobes; root observer next to the README-mode `ubuntu` process ([R07 §4.7]); libbpf headers absent, UAPI `linux/bpf.h` present ([J3 §2.4]) |
| `kz trace` | L3 | C++17 | trace + Φ + anchors → decoded events, identities, `HB_π` (VC, VC_R), per-use LP states, observed violations, routed candidates, chains | §4.8 |
| `kz conform` | L3 | C++17 | trace + skeletons → O1/O2 per-actor acceptance; O8 global conformance within budget | §4.11 |
| `kz predict` | L3 | C++17; optional SMT-LIB2 emitter | trace cone + Φ → predicted witnesses or `unknown(budget)` | explicit search; no solver needed (Z3 not installed, [R07 §2.4]) |
| `kz explore` | L3 | C++17 | closed candidate model + σ + M + b → class within b, witnesses | source-DPOR with sleep sets (§4.10); no external model checker fits this LLVM (GenMC 19–22, Nidhugg 8–19, [R05 §2.2]) |
| `kz hold` | L4 | C++17, ptrace, x86 debug registers | hold plan → controlled run + trace | [M] ptrace from root works on uid 1000, no Yama; debug-exception trap ≈34 µs; gdb ≈203 µs per hit ([R07 §4.7]) |
| `kz gate` | L4 (PI-gated, D3) | C99 `__wrap_*` objects under cFS flags; C++17 controller | variant link → gated runs (directed, PCT) | [M] `--wrap` misses 88 of 3,762 intra-TU sites; handoff 1.1/10.5 µs ([R07 §4.2, §5.1]); a link-option deviation |
| `kz loop` | L5 | C++17 | all artifacts → iteration log, fixed point with run set | §4.14 |
| `kz report`, `kz dump` | L5 | C++17 | `verdicts.kz` and any artifact → Markdown/CSV views; canonical one-record-per-line text for review and golden tests | humans read rendered views; the store stays binary and typed |

All subcommands live in one driver binary `kz` (one LibTooling link, [R07 §2.2]: 56.7 MB static). The trace and model subcommands do not depend on `libkz-llvm`.

### 2.3 Typed formats

**One container format, KZ** [D]. Every artifact is a KZ container:
- a header carrying magic, ABI version, schema hash, B hash, Φ hash and the hash of every **parent artifact** (Merkle-style derivation, [A §2.3]);
- a section table; each section is an array of fixed-layout little-endian records with a CRC;
- a string/blob pool.

Record layouts are declared once, in `include/kz/schema/kz_schema.h`. That C header is included by the BPF program, the gcc-built gate objects and the C++ tools, with `_Static_assert` on every size and offset. There is no schema compiler and no generated code. `kz dump` prints a canonical text form.

```c
/* kz_schema.h (excerpt; layouts final at S0 review) */
typedef struct kz_file_header {
  uint32_t magic;                 /* 'K','Z','0','1' */
  uint16_t abi_major, abi_minor;
  uint32_t kind;                  /* BASIS, FACTS, ANCHORS, SKEL, CENSUS, PLAN, TRACE, HB, CERT, VERDICTS, ... */
  uint8_t  schema_sha256[32], basis_sha256[32], profile_sha256[32], tool_sha256[32];
  uint32_t nparents, nsections;   /* followed by nparents x sha256 of parent artifacts */
} kz_file_header;

typedef struct kz_section { uint32_t type, rec_size; uint64_t count, offset; uint32_t crc32c, rsv; } kz_section;

typedef struct kz_raw_ev {        /* written by the BPF program; 64 bytes */
  uint64_t t_ns;                  /* CLOCK_MONOTONIC: evidence and search heuristic only, never HB */
  uint32_t tid;                   /* kernel thread id */
  uint32_t tseq;                  /* per-thread sequence assigned in kernel: program-order token, loss detector */
  uint32_t site;                  /* probe-site id -> SITES (binary build-id, offset, decoder, cost class) */
  uint16_t phase;                 /* ENTRY | POINT | RET | WATCH | GATE | MARK */
  uint16_t flags;                 /* STR_FOLLOWS, STACK_FOLLOWS, TRUNC, OBJSEQ_VALID, ... */
  uint64_t objseq;                /* per-object token taken inside the object's critical section; 0 = none */
  uint64_t a[4];                  /* raw values per site decoder: registers, stack slots, bounded derefs */
} kz_raw_ev;

typedef struct kz_cert_edge {     /* one step of a certificate */
  uint32_t rule;                  /* T_PO | T_SPAWN | T_OBS | T_TERM | FRONTIER | WRITER | INIT_FALSE | ... */
  uint32_t from_node, to_node;    /* skeleton nodes (context-qualified templates) */
  uint32_t site_from, site_to;    /* source sites -> ANCHORS (Clang location + gcc address) */
  uint32_t phi_entry;             /* Φ rule / effect id used */
  uint16_t status;                /* COMPUTED | DECLARED_VERIFIED | DECLARED_TRUSTED | ASSUMED_VISIBILITY */
  uint16_t rsv;
} kz_cert_edge;

typedef struct kz_verdict {
  uint64_t instance;              /* (contract, context-qualified template, key) */
  uint32_t sigma;                 /* SIGMA_U | SIGMA_U_INF | SIGMA_R1 | SIGMA_E */
  uint8_t  mlevel;                /* M_API | M_IMPL */
  uint8_t  cls;                   /* INACTIVE | NO_PROVIDER | CAP_ABSENT | PROTECTED | FIXED | SCHED_DEP | UNKNOWN */
  uint8_t  refinement;            /* NONE | TOLERATED | TIMEOUT_DEP */
  uint8_t  unknown_reason;
  uint16_t evidence;              /* bitmask: CERT | MODEL | PREDICTED | PRED_EXTRAP | OBSERVED | CONF_F0 | CONF_F1 */
  uint8_t  conf_flags;            /* DERIVED | TIMEOUT_EXERCISED */
  uint8_t  shape;                 /* WRONG_WAY | UNORDERED | ABSENT | STALE | NA */
  uint8_t  outcome_label;         /* VIOLATION | LOSS_UNDER_ORDER | NOT_READY_UNJUDGED */
  uint8_t  consequence, harm, sublabels;   /* PORTABILITY_HAZARD | IMPL_DEFECT_CAND | ASSUMED_VIS */
  uint32_t bound;                 /* -> BOUNDS record (every bound with its policy status) */
  uint32_t observed_n, observed_N;/* pooled only over identical (B, plan) hashes */
  uint64_t certificate, witness_bad, witness_good, chain, runs;  /* section references */
} kz_verdict;
```

Section types: `SCHEMA, STRINGS, BASIS, FACTS, SITES, ANCHORS, TEMPLATES, SKELETONS, MODREF, WRITERSETS, ACTORS, RESOURCES, RAW_EVENTS, EVENTS, STACKS, EDGES, INSTANCES, CERTIFICATES, WITNESSES, HOLDPLANS, VERDICTS, BOUNDS, LOSS, RUNMETA, PROVENANCE, OBLIGATIONS`.

| Artifact | Producer → consumer | Content |
|---|---|---|
| `basis.kz` | kz basis → all | B items with value, source, policy status; file hashes; S0 snapshot; Env record incl. cold/warm |
| `profile.kzp` (text) → `profile.kz` | human → libkz-core | Φ: kinds, ops, effects with LPs, rules, contracts, bindings, constants, version guards; per entry provenance and authoring record |
| `facts.kz`, `anchors.kz` | kz extract, kz link | call sites (direct, indirect, intra-TU, macro-origin), arguments (constant, symbolic, ⊤), guards, table values, doc annotations; site → gcc address and cost class; audit exceptions |
| `modref.kz` | kz modref | points-to sets, mod/ref summaries, support locations per conjunct, writer/disabler sets with Φ mapping and status, `unknown_effect` sites, O7 report |
| `skel.kz` | kz skel | per-actor automata, loop summaries with their bounds and policy status, context-qualified templates |
| `census.kz` | kz static | instances, MustBefore (bitsets), certificates, tier-1 verdicts, open candidates |
| `plan.kz` | kz plan | probe sites (file, offset, instruction class, decoder), watchpoints, expected cost per site and per thread |
| `run-<id>.kzt` | kz observe / kz hold / kz gate | raw events, strings, stacks, loss table, run metadata (B hash, plan hash, S0 hash, holds, harness version, kernel) |
| `hb-<id>.kz` | kz trace | decoded events, identities, edges with tags, per-use LP states, routed candidates, chains |
| `witness-<cand>.kz`, `holdplan-<cand>.kz` | kz predict / kz explore | bad/good schedules; hold points, release events, watchdog with reference |
| `verdicts.kz` | all analyses → kz report | append-only verdict store, versioned by B, Φ and tool hashes |
| `loop-<iter>.kz` | kz loop | run IDs, B/plan/Φ hashes, deltas, obligation results |

**The profile DSL Φ** merges C's profile language and A's RCL. It is a small declarative language parsed by a hand-written recursive-descent parser in `libkz-core` and **interpreted, never compiled** [D]. Excerpt (syntax illustrative; the S0 review fixes it; citations from §3):

```
profile cfs "cFS 088b2fa8 / cFE c5fb2b4d / OSAL d2d877a6 / PSP c4b3b0b6";

const CORE_MAX_STARTUP_MSEC = ref eval "CFE_PLATFORM_CORE_MAX_STARTUP_MSEC"
                                  in "cfe/modules/es/fsw/src/cfe_es_start.c";   # constant-evaluated in its TU
const CF_SEM_RETRIES        = ref eval "CF_STARTUP_SEM_MAX_RETRIES" in "apps/cf/fsw/src/cf_cfdp.c";

kind evs.reg(app: AppId)        { states UNREG, REG; initial UNREG; }
kind svc.identity(svc: Symbol)  { states UNSET, SET, REGISTERED; initial UNSET; scope process; }

op CFE_ES_GetAppID(out AppIdPtr) {
  bind symbol "CFE_ES_GetAppID";
  effect use es.self(exec) lp "read GetAppRecordByContext under ES lock" at pattern
         call("CFE_ES_GetAppRecordByContext") in fn("CFE_ES_GetAppID");          # cfe_es_api.c:748-763
  effect store *AppIdPtr := id(owner(exec))      when result == CFE_SUCCESS;     # out-pointer publication
  effect store *AppIdPtr := CFE_ES_APPID_UNDEFINED when result != CFE_SUCCESS;   # not a publisher
  outcome CFE_ES_ERR_RESOURCEID_NOT_VALID : not_ready(es.self);
}

op CFE_EVS_SendEventWithAppID(EventID, EventType, AppID, Spec) {
  bind symbol "CFE_EVS_SendEventWithAppID";
  effect use evs.reg(arg.AppID) lp "slot check" at pattern fn("CFE_EVS_SendEventWithAppID") first_call;
  stamp arg.AppID;
  outcome CFE_EVS_APP_ILLEGAL_APP_ID : not_ready(UNSET);                         # cfe_evs.c:185-189
  outcome CFE_EVS_APP_NOT_REGISTERED : not_ready(SET);                           # cfe_evs.c:190-194
}

rule CORE-SERIAL stratum S robust obs_fatal guard cfe >= "6.4.2" {
  observe return of fn("CFE_ES_MainTaskSyncDelay") == CFE_SUCCESS
          predicate "forall used AppRecord r: r.AppState >= RUNNING";            # cfe_es_start.c:862-917
  falsifier store AppRecordSetUsed(rec, RESERVED) dominating;                    # cfe_es_start.c:803
  else terminal call("CFE_PSP_Panic");                                           # cfe_es_start.c:838-846
  writers from modref;                                                           # completeness checked by O7
  source code "cfe/modules/es/fsw/src/cfe_es_start.c:776-847";
}

contract K-ID.send-with-appid {
  use op CFE_EVS_SendEventWithAppID;
  requires svc_valid(arg.AppID) && evs.reg(arg.AppID) in {REG};
  oracle   call "EVS_GetAppDataByID"(arg.AppID) != NULL
           && call "EVS_AppDataIsMatch"(call "EVS_GetAppDataByID"(arg.AppID), arg.AppID);
  allowed  none;  harm event_lost_silently;
  source   api_doc "cfe/modules/core_api/fsw/inc/cfe_evs.h:168-172" sha256 <h>;
  authored 2026-10-12 by <author> inspected { R2, C3 };                         # anti-circularity record
}

contract K-REG.osal-countsem-lookup {
  use op OS_CountSemGetIdByName;
  requires osal.obj<countsem>(arg.name) in {LIVE};
  allowed  retry_until_ready({OS_ERR_NAME_NOT_FOUND}, bound = per_site_loop_bound);   # derived from the caller's loop
  harm     app_error_exit;
  source   api_doc "osal/src/os/inc/osapi-countsem.h:<retvals>" sha256 <h>,
           code_admission "apps/cf/fsw/src/cf_cfdp.c:1272-1278" sha256 <h>;
}
```

### 2.4 Data flow

Numbered flow [D] (the drawable version is diagram D2 in §12):

1. `kz basis` records B. `kz extract` and `kz link` produce facts, the configuration-resolved call graph and the gcc anchor table, and run the anchor audit.
2. `kz modref` computes points-to, mod/ref summaries and writer/disabler sets for every Φ conjunct, and runs O7 against Φ's declared effects.
3. `kz skel` builds one skeleton per static actor instance (context-qualified templates, loop summaries).
4. `kz static` computes MustBefore, instantiates every contract at every matching template (census), issues certificates and tier-1 verdicts, and lists open candidates. `kz proofcheck` re-validates every certificate.
5. `kz plan` selects probes: the mandatory synchronization skeleton plus the obligations of open candidates.
6. `kz observe` runs the unmodified binary under B with the plan and writes a trace.
7. `kz trace` joins identities, builds `HB_π`, evaluates every use at its LP, and routes each use: conforming; wrong-way (asks `kz static` for a certificate); O-ordered or unordered (to prediction).
8. `kz conform` checks O1/O2 on every trace and O8 on a sample. O3–O5 run in `kz trace`. Model gaps and new keys go back to steps 1–3 (feedback F1/F2).
9. `kz predict` searches Pred(π) for reversals. `kz explore` (stage S7, gated) classifies candidates on closed models. O6 compares the two.
10. Witnesses become hold plans. `kz hold` (F0), or `kz gate` (F1, if approved), runs them. Confirmation traces re-enter step 7 (feedback F3).
11. Inconsistencies are reported against Φ, the extractor or the model, as findings about the tool (F4).
12. `kz loop` stops at a fixed point of the evidence for a fixed (B, Φ, tool) and records its run set. `kz report` renders the verdicts.

### 2.5 Consistency obligations between layers

| ID | Obligation | Checked | Failure means | Action |
|---|---|---|---|---|
| O1 | Every decoded event concretizes some template of its actor's skeleton | every trace | closure or extraction gap | `unknown(model_gap)` for instances touching the actor; feedback F2 |
| O2 | Each actor's observed event sequence is accepted by its skeleton automaton (per-actor, cheap) | every trace | path abstraction or loop summary wrong | as O1; model evidence of instances touching the actor dropped |
| O3 | No observed order contradicts a MustBefore fact or a certificate edge on the observed path | every trace | an S-rule, writer set or binding is invalid | the rule is suspended; every certificate using it is withdrawn; affected instances `unknown(model_fault)` with a model-fault record |
| O4 | (a) The program's outcome code at each use agrees with the state computed at the LP. (b) No bad occurrence observed at a certified-protected instance; no good occurrence at a certified-fixed-order instance | every trace | rule, effect, LP, contract or binding error | as O3; never silently overridden |
| O5 | Every observed resource key is in the static key universe | every trace | closure incomplete (e.g. an undecoded table) | feedback F1 |
| O6 | A predicted witness is admitted by the explorer's model; an explorer witness projected on observed actors is admitted by Pred(π) | when both exist | model too strict or predictor too loose | `unknown(model_gap)` for the pair; report |
| O7 | The writer sites computed by `kz modref` for each conjunct's support locations equal the LP stores of Φ-declared effects of that kind (no undeclared writer; no declared effect without a store) | every model build | Φ effect missing or wrong, or mod-ref imprecision | T-OBS disabled for that conjunct (`unknown(writer_incomplete)`); disagreement listed; disablement counts reported |
| O8 | Global conformance: some model execution's projection equals the observed per-actor sequences with their outcomes (A's trace-inclusion check, [A §4.8]), within a recorded budget, on a sampled set of traces | sampled; budget `unspecified_by_reference`, recorded | cross-actor inconsistency the per-actor check cannot see | `unknown(conformance)` for verdicts whose slice touches the non-conforming region; budget exhaustion is recorded as "conformance undecided" |
| CC | Every certificate passes `kz proofcheck` | every census | certificate wrong | rejected; the instance goes to tier 2 |

Conformance is necessary evidence, not sufficient: an over-approximate model (star closure, havoc) accepts more traces ([J1 X8]).

### 2.6 Technology decisions

- **Clang/LLVM for analysis only** [D] (ADR-023). Measured cheap and complete on this tree ([R07 §3]). Clang provides macro provenance, constant evaluation of tables and configuration macros, and source locations. Whole-link-unit IR gives call-graph closure with archive semantics and the substrate for mod-ref. DWARF and MC bind every fact to the gcc binary that runs. Clang never builds the executed binary (that would be fidelity class F3).
- **gcc is the executed compiler.** gcc 13.3.0, `-O0 -g` ([R06 K1]). A recompiled object is byte-identical to the build tree's ([R07 §3.5]).
- **MLIR is optional, behind gate G-MLIR** [D] (ADR-018). The skeleton and the formal core are plain C++. At stage S8 tier-1 classification is implemented on an MLIR dialect and compared with the plain C++ arm, on these criteria in order: verdict equality on the development set (required); certificate provenance completeness; analysis code size; runtime ([A §2.4]). MLIR is adopted only if it wins. No ODS/TableGen enters the build before the gate passes, and no generated code is ever checked in ([R06 K8]). The contribution does not depend on MLIR.
- **SMT solver optional** [D] (ADR-014). `kz predict` and `kz explore` run without a solver. An SMT-LIB2 emitter exists for cross-checks on small cones after PI decision D5 (Z3 4.8.12 is the apt candidate, [R07 §2.4]).
- **eBPF userland** [D] (ADR-011). Kairos's own minimal loader uses raw `bpf()` and `perf_event_open` (both probed, [R07 §4.7]), with BPF C compiled by system clang-18 against the kernel UAPI header. tracefs dynamic uprobe events are the zero-install bring-up backend. Installing libbpf is PI decision D2; it would only shorten work.
- **No Python, no JSON in the tool.** All components are C/C++. Reports are rendered by `kz report`. The existing harness (`env/run_cfs.sh`) remains the run driver; Kairos adds one root-side observer hook (PI decision D1, [R07 §9 item 1]).

---

## 3. cFS mapping: the profile Φ_cFS for v7.0.1

Each mechanism becomes Φ entries: resource kinds, operations (binding, effects, LPs, outcomes), HB rules (stratum, robustness tag) and contracts. Rule IDs R1–R19 are those of [R02 §8.1]; P-* are the OSAL primitive rules of [R01 §3, §6.1]. **Edges for shapes are API-level only**; OSAL-internal locks constrain interleavings only under M_impl (§1.4).

### 3.1 Actor inventory and closure

| Actor class | Source in B | Entry resolution | Observation anchor |
|---|---|---|---|
| ES startup thread (PSP main) | `bsp_start.c` → `CFE_PSP_MAIN_FUNCTION` ([R01 §5.1]) | static | `CFE_ES_Main` entry |
| Core main tasks ES, EVS, SB, TBL, TIME | generated `cfe_static_module_list.c` (`MISSION_CORE_MODULES`, `cfe/cmake/mission_defaults.cmake:26–40`; [R02 §1.3]). The table loop in `CFE_ES_CreateObjects` is unrolled exactly (constant trip count from the generated table) | `TaskMain` field of the constant object table | `OS_TaskCreate` in `CFE_ES_StartAppTask`; child `OS_TaskEntryPoint` (exported, `osapi-task.c:124`; [J3 §2.4]) |
| App main tasks (15) | generated startup script ([R03 §6.1]) | entry symbol → definition in the module's `.so` link unit (static analogue of `dlsym`, [R05 §2.5 item 1]); confirmed by the observed `OS_ModuleSymbolLookup` result (O5) | as above |
| Child tasks | `CFE_ES_CreateChildTask` sites with constant function arguments (`cfe_es_api.c:1202–1294`) | constant argument; owner = creating app | as above |
| Timebase `cFS-Master` | PSP `soft_timebase` module (`cfe_psp_soft_timebase.c:53`), PSP module list | OSAL timebase | `OS_TimeBase_CallbackThread` (`osapi-timebase.c:365`) |
| Callback contexts | `OS_TimerAdd`, `CFE_TIME_RegisterSynchCallback`, FS background requests, TBL validation functions | registration sites indexed by actor ([R05 §2.5 item 3]); otherwise type layers (MLTA [V-abs]); otherwise `unknown_effect` | on-behalf frames from user stacks |
| Console writer, signal contexts | OSAL console create; PSP exception handlers ([R01 §2.6, §5.4]) | static | creation sites |
| Environment roots | B.Input | — | CI_LAB receive returns |

The 110 indirect call sites of the FSW set ([R07 §4.2]) are resolved in this order: configuration (task entries, trampoline `RealEntryFunc`, core `TaskMain`/`EarlyInit`, `CFE_Config_IterateAll` callbacks with the iteration count from the generated version table, SBN module ops via configuration table → module → symbol); then registration sites; then struct-layer types; then `unknown_effect`. The closure report gives the count resolved by each rule.

### 3.2 OSAL ID map lifecycle and lookups

- **Kind** `osal.obj<T>` with the automaton of §1.3. The publication LP is the final ID write under the type lock (`osapi-idmap.c:1092`, unlock `:1105`), not the `OS_EVENT_RESOURCE_CREATED` callback (`:864`), which runs after the unlock ([R01 §6.1]).
- **M_api**: create is one transition `ABSENT → LIVE`; by-name lookup returns FOUND or `NAME_NOT_FOUND`.
- **M_impl** ([R01 §1.5–1.8]): ALLOC under the type lock → `ConvertToken(EXCLUSIVE)` sets RESERVED and **releases** the lock → `OS_OBJECT_INIT` makes the name visible → implementation call (`pthread_create`, `mq_open`, …) → `FinalizeNew` relocks and publishes.
  - By-name lookup outcomes by state: `NAME_NOT_FOUND` (name invisible), `OS_ERR_INCORRECT_OBJ_STATE` (name visible, RESERVED), FOUND (LIVE).
  - The `INCORRECT_OBJ_STATE` path returns before the unlock logic (`osapi-idmap.c:401–404`), while the header says "Upon failure, the global table lock is always released for all lock modes other than OS_LOCK_MODE_NONE" (`:384–385`) ([J1 §1 check 5]). Contract **K-LOCKBAL** (the API returns with the type lock released) is derived from that documented invariant.
  - GLOBAL/REFCOUNT/EXCLUSIVE lookups of a RESERVED record: await with up to 5 attempts (`osapi-idmap.c:475–480`). Every unlock of the type broadcasts and consumes an attempt. On exhaustion → `OS_ERR_OBJECT_IN_USE`.
- **Observation**: entry of each `OS_*Create`, `OS_*GetIdByName`, `OS_*Delete`; entry of `OS_ObjectIdFinalizeNew` (publication interval); return values at contract-use sites only (§5.2).
- **Outcome-implied reads-from (O)**: a successful lookup reads from the publication of the same generation. Timestamps are not used to order them ([A §5.6]).
- **Contracts**: `K-REG.osal-lookup` (Req LIVE; Allowed per caller: `retry_until_ready` with the caller's loop bound, else `returned_and_tested`); `K-LOCKBAL` (M_impl); `K-REG.unique` (duplicate names under concurrent creation, [R01 §1.8 F2]); `K-LIFE.osal` (NONE-mode use concurrent with deletion, F4). rwlock and condvar are unmodelled: no non-test caller ([R01 §0 item 6]).

### 3.3 Task create, start, delete; ES handshakes; the App-record windows

- **P-TASK / R2 (S, unconditional)**: the creator's events before the `OS_TaskCreate` call → the child's first event ([R01 §2.2]). Nothing the creator does after `OS_TaskCreate` returns is ordered before the child without further synchronization.
- **K-SPAWN.osal (F7, M_impl)**: the child's `OS_TaskPrepare` waits at most 4 times for the record to leave RESERVED. Every task-table unlock in the process consumes an attempt. On the 5th attempt the entry never runs, although the creator got `OS_SUCCESS` ([R01 §2.2]). Harm `task_never_runs`.
- **ES task-record handshake (O, conditional; S for the guarded part)**: `CFE_ES_StartAppTask` creates the task before taking the ES lock (`cfe_es_apps.c:664–672`) and fills the task record afterwards (`:694–703`). The child polls in `CFE_ES_GetTaskFunction`: a 50 ms delay, then a locked check that the task record has a defined AppId and entry function, with a budget of `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` = 1000 ms, i.e. at most 21 checks (`cfe_es_apps.c:559–596`, re-read; [R05 §1 F4]). Edge: task-record fill → the poll iteration that saw it, only on exit by condition. On timeout the entry is never called.
- **App-record windows (K-SPAWN.app-record, K-REG.es).** `CFE_ES_GetAppID` needs the App record to be final (`cfe_es_resource.c:308–341`; `cfe_es_api.c:738–765`, re-read). The trampoline checks only the task record. Two creation paths have the same structure ([LED §3.4]):
  - **Script apps**: publication at `cfe_es_apps.c:869` under the ES lock (`:861–869`), after the 50 ms-separated poll can already have succeeded ([R02 §1.6]). During the window `CFE_ES_GetAppIDByName` returns `CFE_SUCCESS` with `0xFFFFFFFF` (`cfe_es_api.c:637–656`; `cfe_resourceid_api_typedefs.h:74`) ([J1 §1 check 4]). The contract's framework oracle (`CFE_ES_AppRecordIsMatch`) makes that success a not-ready outcome.
  - **Core apps**: reservation at `cfe_es_start.c:803`, `StartAppTask` at `:810`, publication `CFE_ES_AppRecordSetUsed(AppRecPtr, PendingAppId)` at `:818` (re-read). A core task's `*_TaskInit` can call `GetAppID` before `:818`. EVS does so as its first action (`cfe_evs_task.c:273`, re-read).
  - **Both are O-ordered, not unordered.** The use's LP and the publication are both under the ES lock, so a trace orders them by a lock edge. Deciding them needs lock reversal (§4.9) or the explorer (§4.10) ([J1 §1 check 3]).
  - **Consequences** (INFERENCE from code, [J1 §1.1]; [LED §3.4]): EVS init failure → `CFE_ES_ExitApp(CORE_APP_INIT_ERROR)` → `CFE_ES_ResetCFE(PROCESSOR)` (`cfe_evs_task.c:214–221`; `cfe_es_api.c:333–362`, re-read). If a core task reaches its `WaitForSystemState` inside its window, the raise is skipped (`cfe_es_api.c:533–576`), and the core sync times out and panics. If SB's or TBL's own `GetAppID` (`cfe_sb_task.c:138`; `cfe_tbl_task.c:196`, status ignored) falls inside the window, the identity stays `CFE_ES_APPID_UNDEFINED` for the whole run.
- **Delete/exit (P-TASK-JOIN / R17, S)**: `OS_TaskDelete` cancels and joins (`os-impl-tasks.c:693–723`); after it returns, the victim executes nothing more. Deferred cancellation leaves a program lock held if the victim holds it at a cancellation point ([R01 §2.4]) → **K-LOCK.cancel** on `CFE_ES_DeleteChildTask`, which deletes under the ES lock (`cfe_es_api.c:1348, 1383`).

### 3.4 OSAL primitives

| Primitive | Rule (stratum, tag) | Matching condition | Outcomes recorded | Probe anchors (entry+4 unless stated) |
|---|---|---|---|---|
| Mutex (`OS_MutSem*`, recursive, `os-impl-mutex.c:94`) | P-MUT / R4 (O, schedule-chosen) | outermost release → next outermost acquire of the same id+gen; nested operations add nothing | result at contract-use sites only; recursion depth | `OS_MutSemTake` entry; `OS_MutSemGive` entry with **`objseq` assigned in kernel at give entry**, which runs inside the critical section. Give order equals critical-section order, and each acquire is completed by the acquirer's next event ([B §5.5]). No take-return probe needed |
| Binary semaphore | P-BIN / R9 (O) | give → the take that consumed it; conservatively the join of all gives since the last consuming take; flush → each flushed take; timeout → none. Exact coalescing semantics in prediction | consumed vs flushed; timeout; `OS_SEM_FAILURE` from the internal 2 s `timedlock` (`os-impl-binsem.c:50, 60–77`) | give/flush entry; take return only where the outcome is contract-relevant |
| Counting semaphore | P-CNT / R10 (O) | conservative join of prior gives (the TSan model, [R01 §3]); exact counts in prediction | result; EINTR failure (`os-impl-countsem.c:145–148`) | give entry; take as above |
| Queue (all 26 are SB pipes, [M] [BM §0]) | P-Q (O) | put(d) → get(d) for the same 8-byte descriptor value (`mq_msgsize` 8); FIFO per queue; `OS_QUEUE_FULL` → **drop outcome, no edge** | depth, drop, payload | put entry and outcome; get outcome (§5.2) |
| Timebase and timer | P-TB (O); callback chain (S within the timebase thread) | `OS_TimerAdd`/`Set` (`handler_mutex`, `osapi-time.c:185, 345`) → later callbacks (`osapi-timebase.c:448`); callback k → k+1; callbacks attributed to the timer's creator; `OS_TaskGetId` returns the timebase ID inside callbacks (`osapi-timebase.c:383`) | callback begin/end | timer add/set; callback entry |
| Modules and symbols | P-MOD (O, reads-from) | `OS_ModuleLoad` success → successful symbol lookup | names, results | load/lookup entry and outcome |
| Files, dirs, paths | none (shared state) | path as resource; `OS_FileOpenCheck` is check-then-act ([R01 §3]) | path, op, result | API entry and outcome |
| Sockets | P-NET (O) | datagram send → receive; external senders are `T_env` | payload identity | receive outcome |
| `OS_printf` | **none** | implementation-level serialization only; must be ignored ([R01 §3]) | — | not probed |
| `OS_TaskDelay` | **none** | a yield in σ_U | delay calls (a timing-based protocol is a race signature) | entry |
| Condvar, rwlock | unmodelled | no non-test caller | any observed call → `unknown(unmodelled)` | — |

### 3.5 PSP startup, persistence, restart and panic

- **Startup order (S, program order on the PSP main thread)** ([R01 §5.1]): `OS_API_Init` → event-handler registration → reserved memory → `OS_FileSysAddFixedMap` → `CFE_PSP_ModuleInit` (creates and starts `cFS-Master`) → `sleep(1)` → reset-type decision → `CFE_PSP_InitProcessorReservedMemory` → `CFE_ES_Main` → idle loop. Hence `cFS-Master` is LIVE before any cFE task exists. SCH_LAB's and TIME's timebase lookups are therefore expected to be **protected by certificate** in the default B (a calibration check that the tool does not invent races where the code orders).
- **PSP task-startup hook**: on `OS_EVENT_TASK_STARTUP` the PSP calls `OS_GetResourceName` (task-table GLOBAL lock; an attempt consumer for F7) and pins `CFE_*` tasks to CPU 0 (`cfe_psp_start.c:174, 184–189`; an Env fact to measure).
- **Panic is terminal**: `CFE_PSP_Panic` → `abort()` after `OS_TaskDelay(CFE_ES_PANIC_DELAY)` = 500 ms (`cfe_es_start.c:55`; `cfe_psp_support.c:116–121`, re-read). Other threads still run during the delay ([J1 §1 check 1]). T-TERM excludes only the panicking thread's terminal branch from meets in that thread's CFG; other threads are handled by their own MustBefore.
- **Restart is not terminal**: `CFE_PSP_Restart` → `ShutdownReq` + `SIGUSR1` → `OS_DeleteAllObjects` concurrently with running tasks, with a 10 s abort ([R01 §5.5]). The model treats `CFE_ES_ResetCFE` as "the caller may return or block; other actors continue; the process ends nondeterministically" (over-approximation).
- **Persistence (scope = persistent)**: boot record, CDS, reset area, user-reserved shm (keyed by `ftok` of CWD key files), `/dev/shm/osal:RAM`, `EEPROM.DAT` ([R01 §5.3]). They belong to B.Init and are snapshotted per run. Cross-run contracts (`K-PERSIST`) are checked over run sequences (§1.7).
- **Asynchronous PSP paths**: the exception ring (4-entry lock-free ring with an acknowledged race, `cfe_psp_exception.c:72–86`) → `SIGUSR1` → `SystemNotify` → ES. These are fault/teardown scenarios outside the first scope. Their kinds and edges are in Φ, so they are not silently unordered.

### 3.6 ES phases, soft barriers and core serialization

- **Core object order** comes from the generated `cfe_static_module_list.c` (CONFIG, ES, EVS, FS, SB, TBL, TIME, MSG, SBR, RESOURCEID), not from `cfe_es_objtab.c` ([R02 §1.3]). Changing `MISSION_CORE_MODULES` gives a new B.
- **R18 EarlyInit → all tasks (S)**: phase 1 runs every `EarlyInit` in table order on the startup thread before the first core task is created (`cfe_es_start.c:752–773`). An EarlyInit failure panics.
- **R19 CORE-SERIAL (S, obs-fatal)**: per core entry with a `TaskMain` (`cfe_es_start.c:776–847`, re-read):
  1. reserve the App record under the lock (`:787–806`, `SetUsed(RESERVED)` at `:803`, `Type = CORE` at `:795`);
  2. `StartAppTask` (`:810`);
  3. publish (`:818`) or memset on failure (`:823`);
  4. `MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` (`:835`);
  5. on any failure, `CFE_PSP_Panic` (`:838–846`).
- **The observation behind R19.** `MainTaskSyncDelay` polls under the ES lock until no used record (RESERVED counts as used) has `AppState < RUNNING` (`cfe_es_start.c:862–917`, re-read). The T-OBS rule needs the complete writer set of the conjunct for the record reserved at `:803` (§4.2). Expected entries:
  - the raise in `CFE_ES_WaitForSystemState` executed by a task owned by that app. It is conditional on `GetAppRecordByContext()` ≠ NULL (`cfe_es_api.c:533–576`, re-read);
  - `CFE_ES_RunLoop`'s raise by a task owned by that app (`:469–472`); no core task calls `RunLoop` (comment at `cfe_es_task.c:143–145`);
  - `ExitApp`'s STOPPED store, external apps only (`cfe_es_api.c:399`, re-read);
  - the background scan's WAITING store, external apps only and only from RUNNING (`cfe_es_apps.c:1110, 1139–1146`, re-read). It is excluded by the transition-aware filter (§4.2);
  - record freeing in `CFE_ES_CleanUpApp`, reached from the scan only for external apps with `AppState > RUNNING` (`cfe_es_apps.c:1110–1113`). That guard is relational across the scan's timeout list, so it enters as a Φ-declared guard with status `DECLARED_TRUSTED` (refutable by O3/O4) unless the analysis verifies it.

  Expected result for the ES and EVS entries [I]: every remaining writer is dominated by the return of the owning task's `*_TaskInit` call. `CFE_ES_BackgroundTask` calls neither `WaitForSystemState` nor `RunLoop` (grep of `cfe_es_backgroundtask.c`, re-read); `kz modref` must establish this over the call graph.
- **`CFE_ES_WaitForSystemState(s, T)`** is two effects ([R02 §1.7]):
  1. a raise under the ES lock, per the caller-type table (`cfe_es_api.c:546–576`);
  2. a conditional observation: the unlocked poll of `SystemState` (`:585`). Edge R5/R6 (O, obs-soft, assumed visibility): the startup thread's store `SystemState ≥ s` → the waiter's return **with `CFE_SUCCESS`**. A return with `CFE_ES_OPERATION_TIMED_OUT` gives no edge. `CFE_ES_WaitForStartupSync` returns void (`:616–619`), so the observer reads the inner return.
- **Script-phase barriers** time out softly after `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` each and only log (`cfe_es_start.c:210–213, 225–228`; "soft limit", `cfe_es_internal_cfg.h:866–879`) → O, obs-soft.
- **After OPERATIONAL** every wait returns immediately: restarted, reloaded and commanded apps get no startup ordering at all ([R02 §1.7]).
- **Contract K-PHASE.declare-after-provide**. Provenance: `cfe_es.h` L400–403 (re-read): "This API assumes that the caller has also been initialized sufficiently to satisfy the global system state it is waiting for". Requirement: for each app *a*, every publication by *a* of a resource consumed by another app after a barrier precedes, in *a*'s program order, *a*'s declaration (its raise, or its first `RunLoop`). In-tree deviations ([R03 §4 RC7]): SBN declares before its real init; CS declares even after init failure; an app that exits on error is STOPPED and passes the barrier.

### 3.7 Startup-script apps, libraries, trampoline and child tasks

- **Script parsing** (`cfe_es_apps.c:115–332`): one line at a time, return status ignored (`:313`), no synchronization between entries ([R02 §1.5]).
- **R3 SCRIPT-SEQ (S)**: library init *k* runs synchronously on the startup thread while the Lib record is RESERVED (`:1013–1021`, published `:1037`) → any later entry. `AppCreate(k)` → `AppCreate(k+1)` orders **creation only**. Init completions interleave out of script order ([R03 §6.1]).
- **App creation** (`cfe_es_apps.c:733–902`): reserve (`:821`) → `CFE_ES_LoadModule` (`OS_ModuleLoad`/`dlopen`, `OS_ModuleSymbolLookup`/`dlsym`, `:499, :522`) → `StartAppTask` (`:848–854`) → publish (`:861–869`).
- **Trampoline**: `CFE_ES_TaskEntryPoint` (`:613–641`) → `CFE_ES_GetTaskFunction` (§3.3) → the real entry through a stored function pointer (`:581, :639`). The closure links each actor to its entry. The first observed event inside the entry confirms the mapping (O1).
- **Child tasks** (`cfe_es_api.c:1202–1294`): main-task-only check (`:1268`); the same `StartAppTask`; the parent's App record is already final, so the App-record window does not apply ([R02 §1.10]). `ExitChildTask` frees its record and calls `OS_TaskExit` (`:1453–1478`).

### 3.8 Restart, reload, delete and teardown

- **R15 CTRL-REQ (O)**: a request is written under the ES lock (`cfe_es_api.c:184, 239, 278`) → the app's `RunLoop` read (`:478`) → `ExitApp` → the background scan sees STOPPED. The kill path is also reached by the scan timer without any app action: RUNNING + request → WAITING with a kill timer of `CFE_PLATFORM_ES_APP_KILL_TIMEOUT × CFE_PLATFORM_ES_APP_SCAN_RATE` = 5 × 1000 ms (`cfe_es_apps.c:1139–1146`; `cfe_es_internal_cfg.h:199, 229`) → teardown with no edge from the app ([R02 §1.11]).
- **`CFE_ES_CleanUpApp`** (`:1407–1640`): marks the task and App records RESERVED; runs module cleanups EVS → SB → TBL → TIME **without** the ES lock; deletes tasks (children first); deletes pools; unloads the module; frees the records.
- **Contracts**: `K-LIFE` (use after retire: `ReceiveBuffer` on a deleted pipe; `GetAppID` with a RESERVED task record; a TIME synch callback after its module is unloaded, [R02 §5.3]); `K-GEN` (a peer keeps the old generation's ID or address: CF's semaphore ID, MD's resolved address, SBN module symbols; [R03 §4 RC15]).
- **Scenario family** `B_restart(app, request)`: B.Input contains one documented control request (restart, reload, delete) for one app, triggered by an event (OPERATIONAL observed). One request per scenario is the minimal case that exercises the path. Longer sequences are `unspecified_by_reference` and are swept only if the budget allows, with coverage reported. Results are reported for that B′.

### 3.9 Software Bus

| Mechanism | Φ entry | Source |
|---|---|---|
| Pipe creation | `sb.pipe`: reserve (`cfe_sb_api.c:153`) → OSAL queue created and named outside the lock (`:163`, queue-visible LP) → commit (`:185–203`, publish LP). `CFE_SB_GetPipeIdByName` finds the OSAL queue before the descriptor commits and fails with an error event (`:802–839`) → `K-REG.sb-pipe-name` | [R02 §1.16, §2.2] |
| Pipe ownership | owner = the AppId from `GetAppID`, status ignored (`:120`); checked on subscribe and delete, **not** on receive → K-ID instance on the stored owner | [R02 §2.2, §2.5] |
| Subscribe-before-send | `sb.route(m,p)`; R8 is **state**, not an edge: the route exists or not at the transmit's locked route lookup (LP); destinations are snapshotted under the SB lock (`cfe_sb_priv.c:1010–1079`); no late-join delivery | [R02 §2.7] |
| Transmit outcomes | per destination: delivered, `MsgLim` exceeded (`:1058–1064`), queue full (`:1176–1194`), inactive, deleted; no route → `NoSubscribersCounter++`, `SEND_NO_SUBS` event, **`CFE_SUCCESS` returned** (`:1083–1086`) | [R02 §2.4]; [R03 §1] |
| R7 SB-DELIVER (O) | transmit that enqueued descriptor d on pipe p → the `ReceiveBuffer` on p that dequeued d; FIFO per pipe; no edge for dropped messages | [R02 §2.6] |
| Looper rule (O) | an app main loop over `ReceiveBuffer` is a looper: program order holds inside one handler; between handlers it is observed order, reorderable subject to per-pipe FIFO (CAFA [V-abs]; DCatch Rule-Pnreg [P]) | [R04 §2.7, §4 row 8] |
| Buffer lifetime | valid until the next `ReceiveBuffer` on the same pipe or pipe deletion (`cfe_sb.h:449–464`; `cfe_sb_priv.c:1374–1379`) → `K-LIFE.sb-buffer` | [R02 §2.5, §7.4] |
| Events on behalf of callers | SB API code emits events with `CFE_SB_Global.AppId` as `stamp`, in the caller's task (`cfe_sb_priv.c:786`) → K-ID instances | [R02 §2.8] |
| Subscription reporting mode | `sb.mode.subreport`; when ON, every global subscribe transmits a `ONESUB` packet from the subscriber's task (`cfe_sb_api.c:1192–1195`); SBN enables it after OPERATIONAL and replays earlier subscriptions (`sbn_subs.c:43–60`) → `K-MODE` | [R03 §2.15, §4 RC13] |
| Capacity | `sb.queue(p)` with `cap_B(p) = min(requested, msg_max)`; [M] 22 of 26 pipes truncated, 247 of 584 slots → `K-CAP`. Tier-1 upper bound by maximum-weight closure over the MustBefore order of puts and gets ([A §4.6]; standard algorithmic fact, citation not verified in AR1) | [BM §3.1]; [R03 §4 RC14] |

Route and capacity contracts have `Allowed = unspecified_by_reference` unless a consumer's documentation requires delivery (TO_LAB's documented deferral is `defer_by_design`). Their bad executions carry the label **LOSS-UNDER-ORDER**, harm `unspecified_by_reference`. The oracle is the SB counters: `NoSubscribersCounter`, `PipeOverflowErrorCounter` and the per-pipe peak, read from memory at stop by the [BM §2.3] method ([J2-12]).

### 3.10 Event Services

- `CFE_EVS_Register` publishes `evs.reg(owner)` last, without a lock (`cfe_evs.c:42–111`, LP `:106`); one writer per slot ([R02 §3.1]).
- `CFE_EVS_SendEvent` resolves the caller's AppId. `CFE_EVS_SendEventWithAppID` trusts the passed AppId, with three outcomes ([R02 §3.2]): index invalid → `ILLEGAL_APP_ID`, silent (`cfe_evs.c:185–189`); slot holds another or no ID → `NOT_REGISTERED` plus one event per (slot, caller) (`:190–194`); registered → filter, squelch, generate. These outcomes are the cheapest oracle for K-ID.
- EVS's own identity is published last (`cfe_evs_task.c:311–312`, re-read) and guarded at its use (`cfe_evs_utils.c:613–623`). A guarded early use is `documented_drop` → class refinement `tolerated` ([R02 §3.4]).
- The unlocked filter counters and `EVS_NotRegistered` updates are class M ([R02 §3.3]).

### 3.11 Table Services

| Mechanism | Φ entry | Source |
|---|---|---|
| Register | owner context required; under the registry lock; `OwnerAppId` set last (`cfe_tbl_api.c:145–149`) → `REGISTERED` (LP: that store) | [R02 §4.2] |
| Share | by full name under the registry lock; not found → `CFE_TBL_ERR_INVALID_NAME` (`cfe_tbl_api.c:212–229`) → `K-TBL.share` (Req REGISTERED) | [R02 §4.3] |
| GetAddress / ReleaseAddress | not owned → `ERR_UNREGISTERED`; never loaded → `ERR_NEVER_LOADED` (`cfe_tbl_registry.c:182–196`) → `K-TBL.getaddr` (Req LOADED); R11 lock-flag protocol (O) | [R02 §4.5] |
| Load paths | owner `CFE_TBL_Load` releases the registry lock before the check-then-claim of `LoadInProgress` (`cfe_tbl_api.c:305–309`); the ground `LoadCmd` does it under the lock (`cfe_tbl_task_cmds.c:423–431`) → `K-ATOM.tbl-load` (at most one claim per table), latent without ground input | [R02 §4.4] (a candidate there, not a finding) |
| Validate / activate | R12 (O): request under the lock → the owner's `Manage` that sees it; R13: result flags without a lock → assumed visibility (§3.14) | [R02 §4.4] |
| Critical tables | registry lock → CDS mutex (lock-order fact for the deadlock check) | [R02 §4.7] |
| Cleanup | `CFE_TBL_CleanUpApp` from the ES background task | [R02 §4.9] |
| TBL identity | `CFE_ES_GetAppID(&CFE_TBL_Global.TableTaskAppId)` (`cfe_tbl_task.c:196`, status ignored, re-read) → `svc.identity(TBL)`; 20 `CFE_EVS_SendEventWithAppID` sites in `tbl/fsw/src` ([R06 §5.5]) → K-ID instances | §0.5 |

### 3.12 CDS and generic counters

- `CFE_ES_RegisterCDS` needs a valid caller AppId and holds the CDS mutex for the whole operation. A duplicate name is reused and reported as `CFE_ES_CDS_ALREADY_EXISTS`, which is the restore path (`es/cfe_es_cds.c:311–477`, `:461–472`; [R02 §1.13]). Kind `es.cds` is persistent; `K-PERSIST` checks restore-before-use and reset-type dependence over run sequences.
- Generic counters are a name-addressed registry with no RESERVED step; `GetGenCount` reads without the lock ([R02 §1.14]) → `K-REG.es-counter`.

### 3.13 Time Services and FS background writes

- `CFE_TIME_TaskInit` registers with EVS, creates its semaphores and children, subscribes, and adds a 1 Hz OSAL timer on `cFS-Master` ([R02 §5.1]).
- The 1 Hz pipeline is a chain of real edges: timer callback (timebase thread) → `OS_BinSemGive(LocalSemaphore)` → `Local1HzTask` → SB → TIME main → `Tone1HzISR` → `Tone1HzTask` → SB → TIME main ([R02 §5.1]). Its internal instances should classify protected (calibration).
- `ReferenceState[4]` with `LastVersionCounter` is a seqlock-like single-writer buffer with `volatile` only (`cfe_time_utils.c:538–656`). R14 (O, assumed visibility): the writer's counter store → a reader that validated the version. Speculative reader loads are not reported (Boehm, MSPC 2012, as cited in [R02 §10]).
- Synch callbacks run in TIME's context on behalf of the registering app (on-behalf frame). Cleanup clears the slot before the module is unloaded (`cfe_es_apps.c:1526–1545`, then `:1595–1609`) → `K-LIFE.callback` candidate ([R02 §5.3]).
- FS background requests: producer–consumer edges through the FS mutex (R16); requester callbacks execute in `ES_BG_TASK` (on-behalf frame) ([R02 §6.1]).

### 3.14 Ad hoc synchronization and the memory-model assumption

| Site ([R02 §8.3]; [R03]) | Treatment |
|---|---|
| `WaitForSystemState` poll of `volatile sig_atomic_t SystemState` (`cfe_es_global.h:151`; `cfe_es_api.c:585`) | await summary; O-edge on exit by condition; tagged `assumed` (A4) |
| `MainTaskSyncDelay` poll under the ES lock | await summary; core phase: S, obs-fatal (terminal timeout); script phases: O, obs-soft |
| `GetTaskFunction` poll under the ES lock | await summary; O-edge on exit by condition; timeout → entry never called |
| CF semaphore retry (`cf_cfdp.c:1270–1301`) | count-bounded retry unrolled to the effective bound (not summarized under M_impl, because OSAL lookups can leak the lock, [A §4.2]); three outcomes: FOUND, other code → break, exhausted |
| OSAL `TaskPrepare` RESERVED wait (M_impl) | await, 5 attempts, broadcast-consumed |
| TBL `ValidationResults.State` / `DumpCtrl.State` | O-edge, assumed visibility |
| TIME seqlock | R14, assumed visibility |
| FS `IsPending` pre-check | benign pre-check; no edge (the FS mutex carries the real edges) |
| ES `AppTimerMsec` countdown | time-based transition: nondeterministic in σ_U |
| SCH_LAB first tick ≥ 1 s after init | message edge only from the first 1 Hz packet; the margin ([M] 0.12–0.9 s in 13 boots, [R03 §6.3]) is σ_E evidence, never an edge |

Assumed-visibility edges are valid only under A4: sequential consistency at event granularity on x86-64 TSO at `-O0`. Kairos never reports these sites as defects; memory-model analysis is out of scope ([R06 T8]). Every verdict whose certificate uses such an edge carries the sub-label `assumed-visibility`.

### 3.15 Contract library: coverage of the 15 readiness-contract kinds of [R03 §4]

"Full" means instances are generated, classified and observable in the default B or in a declared variant. "Partial" states what is missing.

| RC | Kind | Templates | Req | Allowed not-ready (source) | Decided by | Oracle | Coverage |
|---|---|---|---|---|---|---|---|
| RC1 | Named OSAL/PSP object lookup | K-REG.osal, K-REG.psp-dev, K-LOCKBAL | `osal.obj(name)` LIVE | per caller: `retry_until_ready` (CF, effective bound), else `returned_and_tested` | census (names from tables) → certificate / prediction (lock reversal) / explorer (M_impl) → hold the provider | return codes; wait-for oracle for K-LOCKBAL | full; HS device case reclassified as `capability-absent` |
| RC2 | Named ES lookup at steady state | K-REG.es | `es.app/task/lib/counter(name)` LIVE by the `IsMatch` oracle (sentinel success is not-ready) | `returned_and_tested` where the caller handles it; HS AppMon counts not-found as "no progress" ([R03 §2.10]) → harm `unspecified_by_reference` | as RC1, plus restart scenarios | return codes; HS action events | full (order); harm partial |
| RC3 | Named table sharing | K-TBL.share, K-TBL.getaddr | REGISTERED / LOADED | CS refreshes stale handles (`cs_compute.c:215–226`) | as RC1 | TBL return codes | full (latent by default table: CS) |
| RC4 | Symbol/module by name | K-REG.loader | `osal.module` LIVE; symbol defined | SBN loads on demand (`sbn_app.c:1060–1088`) | as RC1 | return codes | full |
| RC5 | MsgId subscribe-before-first-send | K-ROUTE | `sb.route(m, p_consumer)` ROUTED at the transmit's lookup | TO_LAB `defer_by_design` (`to_lab_app.c:64–66`); otherwise `unspecified_by_reference` → LOSS-UNDER-ORDER | census of subscribe/transmit pairs with table-derived MsgIds; certificates; prediction; explorer | per-destination puts; `NoSubscribersCounter` | full (harm unspecified) |
| RC6 | Identity/registration publication | K-ID, K-SPAWN | `svc.identity` SET; `evs.reg` REG; `es.self` LIVE | `fallback(CFE_ES_WriteToSysLog)` is a different API (`cfe_evs.h:168–172`) → `none`; EVS own-ID guard `documented_drop` | certificate (R2); prediction + explorer (windows) | EVS and ES return codes | full |
| RC7 | Phase barrier / readiness declaration | K-PHASE | provision before declaration | soft timeouts are documented ("soft limit") | per-app program-order check + barrier HB; σ_U∞ for timeout-dependence | ES app-state table | full |
| RC8 | Persistent restore across reset | K-PERSIST | `es.cds`, critical tables RESTORED/REGISTERED before use | `ALREADY_EXISTS` → restore; defaults on failure | run sequences | CDS status codes | **partial**: crash-point exploration (FCatch-style, ASPLOS 2018 [V-abs]) out of the first scope (D9) |
| RC9 | Parent ↔ child handoff | K-HANDOFF; class M | queue protocol (count, semaphore, mutex) | per app | HB; the unlocked count read (`fm_child.c:162`) is class M | app counters | full (order); class M separate |
| RC10 | Timer callback → task | K-HANDOFF.timer | semaphore LIVE before `OS_TimerSet` | — | certificate (creation order sem → timer add → set, `sch_lab_app.c:168, 184, 303`) + P-TB | — | full |
| RC11 | Library before client | K-REG.lib | `es.lib` LIVE | — | certificate (R3); expected protected (calibration) | — | full |
| RC12 | File-object handoff (TOCTOU) | K-ATOM.file | check-then-act without interleaving | CF's TOCTOU comment (`cf_cfdp_s.c:447–463`) | serializability of the declared transaction (Velodrome PLDI 2008, AVIO ASPLOS 2006 [V-abs]) | return codes | **partial**: needs inputs; file-content semantics minimal |
| RC13 | Global SB mode change | K-MODE + K-ROUTE | `sb.mode.subreport` and the ONESUB emission | `SEND_PREV_SUBS` replay (`sbn_subs.c:43–60`) | typestate mode + prediction | `SBNSubPipe` overflow | full |
| RC14 | Bounded-queue capacity under a burst | K-CAP | `depth(p) < cap_B(p)` at a delivered put | `unspecified_by_reference` → LOSS-UNDER-ORDER; capacity source recorded (env-truncated) | max-weight-closure bound (protected test) → prediction/explorer witness | `PipeOverflowErrorCounter`, put outcome, per-pipe peak | full (B-dependent) |
| RC15 | Stale reference after owner restart | K-GEN | generation g LIVE at use | CS/HS re-resolve (compensated) | restart scenario family; generation-aware keys | return codes; generation mismatch (STALE) | **partial**: needs restart inputs in B′ |

Counts: 11 kinds full, 4 with a stated partial element (RC2 harm, RC8, RC12, RC15). Templates beyond the 15 kinds, from [R01]/[R02] findings: `K-SPAWN` (OSAL bounded wait; ES task record; ES App record, script and core paths), `K-LIFE`, `K-LOCKBAL`, `K-ATOM.tbl-load`, `K-LOCK.cancel`, `K-REG.unique`. Nothing is dropped silently.

### 3.16 HB rule catalogue (summary)

| ID | Rule | Stratum / tag | Matching / condition | Source |
|---|---|---|---|---|
| R1 | Program order | S / unconditional (init, within one handler); O / schedule-chosen (between handlers on a looper) | same actor instance | [R02 §8.1]; CAFA, DCatch |
| R2 / P-TASK | Task create call → child's first event | S / unconditional | `OS_TaskCreate` call → `OS_TaskEntryPoint` | [R01 §2.2] |
| R3 | Startup-script sequencing | S / unconditional | lib init k → later entries; `AppCreate(k)` → `AppCreate(k+1)` (creation only) | [R02 §1.5] |
| R4 / P-MUT | Mutex outermost release → next acquire | O / schedule-chosen | same mutex, `objseq` | [R01 §3] |
| R5 | Polled-flag barrier | O / obs-soft (+assumed for `SystemState`) | write observed → loop exit by condition; timeout → none | [R02 §8.1] |
| R6 | App-state write → barrier release (composed) | O / obs-soft for script phases | raise → `MainTaskSyncDelay` success → `SystemState` store → waiter success | [R02 §8.1] |
| R7 | SB transmit → receive | O / schedule-chosen | same descriptor and pipe; delivered only | [R02 §2.6] |
| R8 | Subscribe → later routing | **state, not edge** | route present at the transmit's locked lookup | [R02 §8.1] |
| R9 / P-BIN, R10 / P-CNT | Semaphore give → take | O | §3.4 | [R01 §3] |
| R11, R12, R13 | TBL lock flag; request → owner; result flags | O (R13 assumed) | §3.11 | [R02 §4] |
| R14 | TIME reference publication | O / assumed | validated reader | [R02 §5.2] |
| R15 | ES control request → exit/teardown | O | §3.8; the timer path gives no edge | [R02 §1.11] |
| R16 | FS request → background → completion | O | FS mutex | [R02 §6.1] |
| R17 / P-TASK-JOIN | Task delete/exit | S / unconditional | the deleter's return | [R01 §2.4] |
| R18 | EarlyInit → all tasks | S / unconditional | — | [R02 §1.3] |
| R19 | Core serialization | S / obs-fatal | T-OBS with complete writer set + T-TERM (§3.6, §4.4) | [R02 §1.3]; `cfe_es_start.c:835, 838–846` (re-read) |
| P-TB | Timer add/set → callbacks | O | `handler_mutex` | [R01 §3] |
| P-Q | Queue put → get | O | payload, FIFO; drops give no edge | [R01 §3] |
| P-MOD | Module load → symbol lookup | O (reads-from) | success outcome | [R01 §4] |
| RF | Outcome-implied reads-from | O | successful use ← the publication of the same generation | DefUse [V-abs] |
| — | Priorities, sleeps, script order for execution, timed-out barriers, `volatile` alone, use-site guards, console order, OSAL-internal locks (for shapes) | **none** | listed so they cannot be added by mistake | [R02 §8.2]; [R01 §3] |

### 3.17 Site binding and version guards

- **No line-number probes** [D] (ADR-019; [J2-9]). Each internal site is bound by an **AST pattern**: enclosing function, operation (call, store to a named field, memset of a named type), argument shape, ordinal within the function. `kz link` resolves the pattern to gcc instructions through the DWARF line table and subprogram/inlined-subroutine entries of the basis's own binary, and disassembles with MC to obtain the instruction and its cost class. Example: the core publication is "the call of `CFE_ES_AppRecordSetUsed` with a second argument that is not `CFE_RESOURCEID_RESERVED`, in `CFE_ES_CreateObjects`". The helper is `static inline` (`cfe_es_resource.h:204`, re-read). At `-O0` gcc is expected to emit a local copy per TU [U]; the binder records whether the call or the store is anchored, and lists failures in the audit.
- Sites with several matches or none are listed in the anchor audit ([R06 T7]). Instances depending on an unanchored site are `unknown(model_gap)` for observation, though static certificates still apply.
- Every Φ rule and effect carries a **version guard** (e.g. R19 `cfe >= 6.4.2`, the two-phase barrier `cfe >= 6.6.0`, [LED §3.3]). A profile for another revision changes only Φ (hypothesis H-V, §7.6).

---

## 4. Algorithms

### 4.1 Configuration closure and fact extraction (`kz basis`, `kz extract`, `kz link`)

1. **Translation units.** Take the cpu1 compile DB (1417 entries) and keep the 448 flight entries. Target membership comes from `link.txt` (or the CMake File API), not from paths, because the FSW set still contains test modules and header checks ([R07 §3.1]).
2. **Parse** with Clang LibTooling using the recorded commands, the two diagnostic-only adaptations (`-Wno-unknown-warning-option`, `-Wno-error`) and `-resource-dir` ([R07 §3.3]). The adaptation is recorded in B as an analysis-only transformation.
3. **Facts per call site**: callee; arguments as constant, symbolic (table field, global) or `⊤`; macro-expansion provenance; enclosing function; loop context; branches that test the call's result; intra-TU flag; whether the result is used or ignored.
4. **Constants and tables.**
   - Platform and app configuration macros are constant-evaluated **in the TU of their use**. This yields the build's effective value for wrappers such as `CF_INTERNAL_CFGVAL(...)` (§0.5).
   - Table initializers (`sample_defs/tables/*.c`, app `fsw/tables/*.c`) are constant-evaluated and cross-checked byte for byte against the built `.tbl` payload, using the table type's layout from the gcc DWARF ([A §4.1 step 4]). A mismatch is a basis error.
5. **Doc annotations.** Every `\par Assumptions` block and every `\retval`/`@retval` list of `cfe/modules/core_api/fsw/inc/*.h` and `osal/src/os/inc/*.h` becomes an annotation candidate with file:line and hash ([F] 138 assumption blocks and 429 `\retval` lines in cFE core API headers; 593 retval lines in OSAL, [J1 §1 check 7]). The `\retval` vocabulary feeds outcome mining (§4.6).
6. **Link units.** Emit bitcode and link each unit with archive semantics (whole-archive core, `--only-needed` for lazy BSP/PSP members) ([R07 §3.4]). Build the call graph and resolve indirect calls by the order of §3.1; residual sites become `unknown_effect`. Cross-check against gcc `-fcallgraph-info` ([R07 §4.4]).
7. **Site binding and anchor audit.** Bind every Clang site and every Φ AST pattern (§3.17) to gcc instructions through the DWARF line table, subprogram and inlined-subroutine entries, and MC disassembly. Record the instruction class and the expected cost class from the calibration table (§5.10). Sites without a unique match are listed ([R06 T7]).
8. **Closure boot.** One F0 observation of the default B records every `OS_TaskCreate` (name, entry), child start and `OS_ModuleSymbolLookup` result. They must equal the closure (O5); a mismatch is a closure bug, fixed before any verdict is reported ([A §2.5]).

Cost [M]: parse 10 s, bitcode 4.4 s, core link 0.35 s on this VM ([R07 §6]).

### 4.2 Mod-ref analysis and writer sets (`kz modref`)

This is the soundness-critical analysis behind T-OBS and every certificate ([J1-2], [J2-6], [J3-M1]). It runs over the **closed IR** (core unit plus all app units, cross-linked by the closure) [D].

1. **Abstract locations.** Globals are field-sensitive: `(symbol, field path)`. Arrays of records (`AppTable[]`, `TaskTable[]`, registry arrays) are index-collapsed: `(symbol, [*], field path)`. Heap objects are `(allocation site or pool API, field path)`. Stack objects that escape through out-pointers are `(function, alloca, field path)`.
2. **Points-to.** An Andersen-style inclusion analysis (Andersen 1994 [V-abs, R05 §2.5]), flow-insensitive and context-insensitive, field-based for struct-typed pointers. Function-pointer targets are resolved by the closure (§3.1) and type layers (MLTA [V-abs]). SVF ([V-abs]) is the design reference.
3. **Mod/ref summaries**, bottom-up over the call graph:
   - **stores**;
   - **memset/memcpy/memmove/strncpy** (and their builtins), which write every field of every object the destination may point to. Constant `memset(…, 0, …)` also records the written value 0, for transition filtering (e.g. `memset(&CFE_ES_Global, 0, …)` at `cfe_es_start.c:82`, `memset(AppRecPtr, 0, …)` at `:823`, re-read);
   - **out-pointer stores** through formal parameters, summarized per formal (`*p.field := value-expr under guard`) and instantiated at each call site with the actual argument's points-to set. This is a one-level, call-site-sensitive treatment, so `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` (`cfe_sb_task.c:138`) yields two writer sites on `CFE_SB_Global.AppId`: the success-branch store of a valid ID and the failure-branch store of `CFE_ES_APPID_UNDEFINED` (`cfe_es_api.c:752–760`, re-read);
   - external functions with Φ summaries (libc data functions);
   - **`unknown_effect`** for unresolved indirect calls, inline assembly and externals without summaries. It may write every escaped location.
4. **Support locations of a conjunct.** The ref-set of the conjunct's framework oracle function (§1.6), plus the locations Φ's typestate mapping reads. Example: `CFE_ES_AppRecordIsMatch` reads `(AppTable[*], .AppId)`; the K-ID stamp conjunct reads `CFE_SB_Global.AppId`.
5. **Writer candidates.** Every store, memory-intrinsic or out-pointer summary whose may-target intersects the support locations.
6. **Transition-aware filtering** ([J3-M1]). For each candidate store `s` (value expression `v`, dominating guard `g` restricted to conditions over the same location, the same record and key-derived values), decide in a small domain (constants and intervals for enum/integer fields; defined/undefined/reserved for resource IDs; record-type tags):
   - `s ∈ W` (may make the conjunct true) iff some pre-state satisfying `g` has the conjunct false and the post-state has it true;
   - `s ∈ D` (may make it false) symmetrically;
   - neutral otherwise.

   Undecidable cases are kept (an over-approximation, hence sound). Example: the background scan's `AppState = WAITING` store is guarded by `Type == EXTERNAL ∧ AppState == RUNNING` (`cfe_es_apps.c:1111, 1139–1146`, re-read). For the conjunct "used → AppState ≥ RUNNING" the pre-state already satisfies the conjunct, so the store is neutral.
7. **Mapping to Φ effects (obligation O7).** Every W/D site must lie at the LP of a Φ-declared effect of the conjunct's kind (by its AST binding), and every Φ effect must contain at least one W/D site. The Φ effect contributes its **key expression** (e.g. `owner(exec)`, `arg.AppID`) and any **declared guard**. Each writer entry gets one status:
   - `COMPUTED`: the store and the transition were established by steps 5–6, and Φ agrees;
   - `DECLARED_VERIFIED`: Φ's guard was confirmed by step 6;
   - `DECLARED_TRUSTED`: Φ's guard excludes a site that step 6 cannot exclude, e.g. a relational guard across the scan's timeout list (§3.6). The entry is carried in every certificate that uses it and is refutable by O3/O4;
   - `DISABLED`: an `unknown_effect` may write the support locations, or an undeclared writer exists. T-OBS is disabled for the conjunct, instances depending on it become `unknown(writer_incomplete)`, and the count is reported.
8. **Key unification.** A writer site targets the instance's key if its key expression may equal that key. Ownership (`owner(exec)`) comes from the closure: tasks created by an app's `CreateChildTask` sites are owned by that app. Static value analysis supplies concrete keys where possible. A writer whose key cannot be separated from the instance's key is kept.
9. **Initial value and falsifiers.** `INIT` is the state after static initializers, EarlyInit memsets and EarlyInit stores on the startup thread before its first spawn. It is computed by abstract interpretation of that straight-line prefix in the same small domain. A **falsifier** `f` for an observation `o` is an event in `o`'s actor that dominates `o` and definitely makes the observed conjunct false. Example: `CFE_ES_AppRecordSetUsed(rec, RESERVED)` at `cfe_es_start.c:803` makes a record used while its `AppState` still has the value it held in INIT: UNDEFINED from the startup memset, provided no writer of that field can precede `f` (checked with PWB, §4.5).

Outputs (`modref.kz`): per conjunct, W and D with statuses, support locations, `unknown_effect` sites, the O7 agreement report, and counts of disabled conjuncts. Cost: Andersen-style analysis is cubic in the worst case ([R05 §2.5]); the cost on this tree is measured at stage S3, and no figure is assumed.

### 4.3 Skeleton construction (`kz skel`)

For each static actor instance, the inlined (context-qualified) CFG is projected onto **relevant nodes**: Φ effects at their LPs, support-location loads and stores, spawn sites, terminal operations, handler marks, and explicit local choice points. The result is one automaton `Sk_τ` per actor instance [D].

- **Guards.** A branch on an operation's outcome becomes an outcome-labelled transition. Branches on B constants are folded (CF's empty `sem_name` makes the lookup unreachable, `cf_def_config.c:60, 78`). Other data branches become **explicit local choices** (over-approximation, visible to the explorer).
- **Terminal operations** (T-TERM, §4.4): Φ declares terminal ops per PSP with a citation (`CFE_PSP_Panic` and `abort` on pc-linux). Calls to them have **no successor edge**, even though the C code continues after `CFE_PSP_Panic` (the comments say it returns under unit test, `cfe_es_start.c:110`). `CFE_ES_ResetCFE` is not terminal (§3.5).
- **Loops** (no arbitrary unrolling):
  - *poll loops* (an `OS_TaskDelay` plus an exit test on a shared field or API result; recognized instances `GetTaskFunction`, `WaitForSystemState`, `MainTaskSyncDelay`, OSAL `WaitForStateChange`) → `await(pred) {success} {timeout}`. The replacement is justified by Lemma L1 (§6.2), which is switchable per instance; when it is off, the loop is unrolled to its reference bound. Under M_impl, loops containing OSAL lookups are not summarized, because lookups can leak the lock and their unlock broadcasts consume waiters' attempts ([A §4.2]);
  - *count-bounded retry loops* → unrolled exactly to the bound **constant-evaluated in the build** (CF: effective `CF_STARTUP_SEM_MAX_RETRIES`; OSAL: 5 attempts, `osapi-idmap.c:475–480`). A non-constant bound gives `unknown(bound)`. If the iterations are paced by `OS_TaskDelay`, the exhaustion branch is tagged as a timeout transition (disabled under σ_U∞, §1.8);
  - *constant-trip loops* over B constants with relevant events in the body (the core object table loop of `CFE_ES_CreateObjects`; record-table scans) → unrolled when the trip count is a B constant;
  - *message loops* (`while (CFE_ES_RunLoop(...)) { ReceiveBuffer; dispatch }` and core main loops) → a handler automaton with `handler_begin/end` marks, cut at the **main-loop horizon h**. h is `unspecified_by_reference`; it is swept 0, 1, 2, … within the budget, and the largest completed h is part of b;
  - *other loops* containing relevant events → star-closure with explicit iterate/exit choices.
- **Recursion** makes the affected summary `unknown`.
- **Output** `skel.kz`: automata, sizes, counts of local choices, and loop summaries with their bounds and policy status.

Assumption **A-SKEL**: each automaton over-approximates its actor instance's projected event sequences under B. O1/O2 test it on every trace.

### 4.4 The MustBefore fixpoint (`kz static`)

**Graph.** The nodes are the relevant nodes of all skeletons. The edges are intra-actor skeleton edges plus spawn edges (spawn site → child entry).

**Definition.** `a ∈ MB(n)` means: in every execution of the model under σ_U, at every occurrence of `n`, some occurrence of `a` has occurred earlier. MB is the **greatest** solution of the following equations, starting from `MB(n) = ⊤` (all relevant nodes) for every node except the initial actor's entry [D] ([A §4.4], corrected per [J1-1], [J2-2]):

| Rule | Equation contribution | Basis |
|---|---|---|
| T-PO | `in_PO(n) = ∩_{p ∈ pred(n)} (MB(p) ∪ {p})`. Predecessors are skeleton predecessors only; terminal ops have no successors (T-TERM) | program order; dominance modulo terminal branches |
| T-SPAWN | for an actor entry `n_τ`: `in(n_τ) = ∩_{c ∈ SpawnSites(τ)} (MB(c) ∪ {c})`, a meet over **all** spawn sites. If any spawn site of τ is unresolved (`unknown_effect`), `in(n_τ) = ∅` | `OS_TaskCreate` → child start ([R01 §2.2]) |
| T-OBS | for an observing exit `o` (await success, successful lookup, consuming take, queue get, test of an `assumed` flag) with observed conjunct `C`, falsifier `f` (§4.2 step 9) and writer set `W_o = {w ∈ W_C : w may occur after f}` (writers with `PWB(w, f)` or `PWB(o, w)` are removed): `in(o) = in_PO(o) ∪ ∩_{w ∈ W_o} (MB(w) ∪ {w})`. Applies only if `W_C` has no `DISABLED` entry and `C` is false right after `f` (or at INIT when no falsifier exists). If `W_o = ∅`, `o` is unreachable and marked. Entries with status `DECLARED_TRUSTED` are allowed and recorded | reads-from: some writer between `f` and `o` made `C` true |
| T-TERM | terminal operations (`CFE_PSP_Panic` → `abort()` on pc-linux, cited) have no successor edges | panic ends the process ([J1 §1 check 1]) |
| (none) | lock acquisition alone gives no must-order | locks give atomicity, not order ([R04 L5]) |

Priority rules of σ_R1 are **not** in MB; σ_R1 is handled by the explorer.

```
MB(entry of the initial actor) := ∅;  MB(n) := ⊤ for all other n
repeat
  for each actor skeleton, for n in reverse post-order:
     in := in_PO(n)                                   # T-PO; terminal ops have no successors (T-TERM)
     if n is an actor entry:   in := in ∩ T_SPAWN(n)   # meet over all spawn sites
     if n is an observing exit with a valid writer set: in := in ∪ T_OBS(n)
     MB(n) := in
until no set changes        # sets only shrink from ⊤: terminates; result is the greatest fixpoint
```

**Complexity** ([A §4.4]). With N nodes and R relevant events (bitset width), each update costs `O((indeg + |W_o|) · R/64)` word operations. Sets only shrink, so there are at most N·R changing updates. The worst case is `O(N·R·(d̄ + w̄)·R/64)`. MB is computed once per (B, Φ, M), and all instances are classified from it.

**Justifications.** For each fact used by a certificate, the derivation (rule and predecessor) is reconstructed on demand by backward search over the equations. Only facts that certificates need are materialized.

### 4.5 Certificates and the independent checker

Let `Anc(u)` be the set of nodes of `u`'s actor instance that have a skeleton path to `u` (including `u`).

**PWB certificate.** `PWB(a, b)` holds if `∃ m` in `a`'s actor instance with `m ∉ Anc(a)` and `m ∈ MB(b)`. Proof sketch: if `a` occurred after `m` in the same instance, the run would contain a path `m →* a`. So every occurrence of `a` precedes the first occurrence of `m`, and `m` precedes every occurrence of `b` (L-PWB, §6.2).

**FIXED_ORDER certificate** for instance `i` (fixes [J2-1], [J3-M1]). Choose a conjunct `P_j` such that all of the following hold:

1. `P_j` is false at INIT for the instance's key (`INIT_FALSE`).
2. `W_j` has no `DISABLED` entry.
3. `W_j ≠ ∅` (otherwise the class is `no-provider`, or `capability-absent` if Φ guards the provider with an absent env capability).
4. **Frontier**: `F = (Nodes(actor(u)) \ Anc(u)) ∩ ⋂_{x ∈ W_j} MB(x) ≠ ∅`. Pick `m ∈ F` (closest to `u`, for readability).
5. **Badness**: every not-ready outcome of `u` is outside `Allowed` on every skeleton continuation within the horizon (trivial for `Allowed = none`). If only some continuations are disallowed, the certificate establishes "fixed not-ready" (shape WRONG-WAY or ABSENT) and the class goes to tier 2.
6. **Reachability**: `REACH`, from an observed occurrence (run id) or a confirmed model path. Without it the record says "fixed-order if reached".

Certificate = (INIT_FALSE proof, W_j with statuses, the frontier node m with the no-path proof, one MB derivation `m ∈ MB(x)` per writer x, badness proof, REACH). The proof does not need `u` to dominate anything, so uses behind early returns are covered.

**PROTECTED certificate.** For every conjunct `P_j`, one of:

- (a) `P_j` is true at INIT and `D_j` is empty or unreachable; or
- (b) `∃ x* ∈ W_j^must` (a writer whose store definitely establishes `P_j` for the key, status `COMPUTED` or `DECLARED_VERIFIED`) with `x* ∈ MB(u)`, and every reachable `d ∈ D_j` satisfies `PWB(d, x*) ∨ PWB(u, d)`.

In both cases no `DISABLED` writer may exist, and `REACH` is required as above. The refinement `tolerated` needs, in addition, a witness of an allowed not-ready occurrence.

**INACTIVE**: no skeleton path from any actor entry reaches the template under B constants. Proof = the folded path condition. The skeleton over-approximates paths, so "no path" is sound.

**NO_PROVIDER**: `P_j` is INIT_FALSE and `W_j = ∅` with no `DISABLED` entry.

**CAPABILITY_ABSENT**: the only W_j entries are Φ effects guarded by an `env` predicate that B.Env records as false (status `DECLARED_TRUSTED`), and observation agrees (O4).

**Independent checker `kz proofcheck`** ([A §4.4]; [J1-1]). It shares no code with `kz static`. It reads `skel.kz`, `modref.kz`, `facts.kz` and Φ, and re-validates each step:
- **T-PO**: dominance modulo terminal ops on the actor fragment, recomputed by its own dominator routine;
- **T-SPAWN**: the spawn-site list equals the closure's list for that actor;
- **T-OBS**: the falsifier condition, and W_o recomputed from `modref.kz`;
- **T-TERM**: Φ declares the op terminal for this PSP, with a citation;
- **FRONTIER**: no path `m →* u`, by its own reachability;
- **writer statuses**: consistent with `modref.kz`;
- **REACH**: the run exists and contains the occurrence.

**Mutation tests.** A mutation engine perturbs valid certificates: it drops a spawn site, deletes a writer, swaps m for a node in `Anc(u)`, marks a non-terminal op terminal, or replaces a COMPUTED status by DISABLED. Every mutant must be rejected. The mutation kill rate is a reported fidelity metric. A certificate that fails the check demotes the verdict to tier 2.

### 4.6 Census and candidate generation

```
for each contract κ in Φ (frozen set), each context-qualified template t matching κ.U:
   key := value analysis of κ.sel at t          # concrete | symbolic | ⊤
   for each conjunct P_j: (W_j, D_j, status) := writer sets (§4.2), keyed by key
   if t unreachable under B constants:                 emit inactive(B) + path-condition proof
   elif key = ⊤:                                        open(key_unresolved) → probe plan
   elif ∃ j: INIT_FALSE(P_j) ∧ W_j = ∅ (complete):      emit no-provider(B) or capability-absent(B)
   elif FIXED_ORDER certificate exists:                emit fixed-order(σ_U) [+ REACH when available]
   elif PROTECTED certificate exists:                  emit protected(σ_U)
   else: open(reason ∈ {unordered, O-ordered-only, conditional-observation, writer_incomplete, horizon}) 
         → probe plan, prediction, exploration
```

Every certificate goes through `kz proofcheck` before the verdict is stored. All census instances get a class or `unknown(reason)`. Latent instances get a model verdict (inactive, no-provider, a model class or unknown), not just "unobserved" ([J2-7]). Dynamic instance sources (§4.8 step 9, §7.1) add instances to the same census, keyed by template id, or as `template: none`.

### 4.7 Probe-plan synthesis (`kz plan`)

1. **Mandatory synchronization skeleton** for all actors: the anchors of every S and O rule (OSAL task, mutex, semaphore, queue, timer and module operations; ES lock wrappers, waits, `RunLoop`, `ExitApp`, app/task creation; SB create, subscribe, transmit, receive). The set is small and closed ([R01 §0 item 6]). Keeping all of it avoids the third-party-synchronization slicing error noted in v0.1 §7.3.
2. **Candidate obligations**:
   - the LP intervals of publish and use effects of open instances;
   - outcome capture at **contract-use sites only**;
   - the results of guards that decide `Allowed`;
   - write watchpoints (at most 4 per thread, the x86 debug-register limit) on readiness fields that no API boundary covers;
   - user stacks where identity frames need them.
3. **Cost**: each site has an instruction class and a unit cost from the **calibration table measured on this VM** (§5.10). Classes not yet measured are measured in S1 before use ([J3-M4]). The plan records the expected per-thread cost `ΔC_i = Σ_j n_ij · c_j` (v0.1 §11.2): `n_ij` comes from the previous trace or, on the first iteration, from static counts.
4. **Selection**: greedy weighted set cover of obligations by sites. Optimality is not claimed. The plan's hash goes into every trace.

### 4.8 Trace analysis (`kz trace`)

1. **Decode and loss.** Decode raw records with the site decoders. Check per-thread `tseq` gaps and per-CPU drop counters. Mark actor intervals with loss: verdicts whose evidence depends on such an interval become `unknown(loss)`.
2. **Identity join, offline**; nothing is looked up in the process (§5.7).
3. **HB.** Compute `VC` (all definite edges) and `VC_R` (unconditional and obs-fatal edges). Matching keys:
   - mutexes: `objseq` taken at give entry, inside the critical section;
   - queues: descriptor value plus FIFO; SB: descriptor plus pipe;
   - semaphores: conservative join;
   - polls: only on a success return;
   - registries: outcome-implied reads-from for successful uses.

   Ambiguous matches are recorded and weaken the evidence of the verdicts that use them.
4. **LP intervals.** Every effect occurrence gets an interval `[the last observed event of its actor before the LP, the first observed event after it]`. Two LPs are ordered if their intervals are HB-ordered, or by an outcome-implied reads-from. Otherwise their order is open and is decided by the outcome code where Φ's outcome map determines it; else it is ambiguous. This makes exact LP probes unnecessary ([A §5.6]).
5. **Per-use evaluation and routing** (fixes [J3] weakness 6). With L⁻, L⁺ and L∥ the lifecycle LPs on `sel(u)` before, after and unordered with `u`'s LP:

| Case | Result | Route |
|---|---|---|
| L∥ = ∅, state ⊨ Req | READY (or TOLERATED if allowed) | supporting evidence only |
| L∥ = ∅, state ⊭ Req, every ordering edge used is S (VC_R order) | observed not-ready; Allowed evaluated | ask `kz static` for a FIXED_ORDER certificate on the template; `VC_R` order is the prefilter |
| L∥ = ∅, state ⊭ Req or ⊨ Req, but the order between `u` and the deciding lifecycle LP needs ≥ 1 O-edge (lock, message, semaphore, soft poll, RF) | observed outcome in that order | **prediction**: is the reverse order feasible? (C1, C5, both App-record windows) |
| L∥ ≠ ∅ | the outcome code says which order happened | prediction for the other order |

   The program's own outcome code (`ILLEGAL_APP_ID`, `NAME_NOT_FOUND`, `NEVER_LOADED`, `RESOURCEID_NOT_VALID`, `Q_FULL`, sentinel success) must agree with the computed state (O4).
6. **Allowed** is evaluated on the actor's continuation within the horizon (§1.6). If the trace ends first, the result is `unknown(horizon)`.
7. **Race coverage** (EventRacer OOPSLA 2013 [V-abs]): if reversing a pair needs an ad hoc synchronization pair reversed first, that pair is reported as the root and the dependents are attached.
8. **Violation chains** from on-behalf/API frame containment and Φ `emits` annotations (e.g. a `SEND_NO_SUBS` event emitted inside a transmit with no route).
9. **Template-free sources**: outcome mining and key-pair enumeration (§7.1 items 2–3).
10. Obligations O3–O5 here; O1/O2 in `kz conform`.

Complexity: `O(N·|T|)` for N events and |T| actors ([M] 27 threads per boot, [BM §0]).

### 4.9 Prediction (`kz predict`)

**Question.** Given trace π and a routed candidate (e.g. publisher `p` before use `u`, observed lock-ordered), is there a feasible execution in which `u`'s LP precedes `p`'s LP, or, for capacity, in which a delivered put overflows?

**Model `Pred(π)`** (maximal causal models, RV 2012 [V-abs]; RVPredict's control-flow abstraction, PLDI 2014 [V-abs]; properties as constraints, GPredict ICSE 2015 [V-abs]):
1. **Cone C**: per-actor prefixes up to `u` and `p`, closed backward under S-edges and under the reads-from of control-relevant events.
2. **Constraints**:
   - per-actor prefix order;
   - S-edges;
   - every control-relevant event in `C \ {u}` keeps its observed outcome class: a successful lookup needs a preceding publication and no retire; a poll exit by condition needs the enabling write first; an acquire needs the previous release;
   - Φ primitive semantics: mutual exclusion with recursion, binsem saturation and flush, non-negative counts, queue FIFO with capacity `cap_B`;
   - `C_σ` for σ_R1 when requested;
   - the target order or outcome flip.
3. **Lock reversal is allowed** ([J1-7], [J2-3], [J3-M2]). Critical sections are atomic blocks, and the registry key is the conflict object. Two critical sections on one lock may be reordered whenever constraint 2 still holds. This is exactly what C1, C5 and the App-record windows need, and what sync-preserving prediction forbids ([R04 §2.5]).
4. **Search**: depth-first over linearizations of C with sleep-set partial-order reduction. Independence comes from Φ: operations on different keys, locations and objects commute, and critical sections on one lock commute as wholes unless they conflict on a key or location. States are memoized (actor positions, resource states, synchronization states). The search stops at the first witness.
5. **Prefilters only.** SHB (OOPSLA 2018 [V-abs]) for unordered pairs, and WCP (PLDI 2017 [V-abs]) / SyncP (POPL 2021 [V-abs]) for pairs ordered through non-conflicting critical sections, are run first where their domain applies. Their soundness transfers to the abstract trace only under assumption A-API (the abstraction is faithful). Any reversal they propose is re-validated as a concrete linearization by the constraint checker of step 2: Vindicator's candidate-plus-vindication split (PLDI 2018 [V-abs]). Their silence means nothing: the explicit search still runs, and nothing is ever classified protected or "not reversible" because a prefilter found nothing ([J1 X7]).
6. **Summary-guided extrapolation** ([B §4.4 (e)]). A retry or poll site observed once may be extended by further not-ready iterations up to its **effective reference bound**: CF's effective `CF_STARTUP_SEM_MAX_RETRIES`; `GetTaskFunction` ≤ 21 checks; OSAL ≤ 5 attempts; `WaitForSystemState` `⌈T/poll⌉`. A witness that uses extrapolation is labelled **`predicted-extrapolated`**: it is model-based and is admitted only where the loop summary passed O2 ([J1-7]).
7. **Budget** is a resource parameter, `unspecified_by_reference`. It is swept, and the number of `unknown(budget)` results is reported as a function of the budget.
8. **Result**: a witness in Pred(π), sound in the predictive sense relative to A4, A-API and A7 (G5, §6.3), or "not predictable from π within budget". **Neither outcome is a protected verdict.**
9. **Optional SMT cross-check** after PI decision D5: an order-variable encoding in the partial-order style of Alglave, Kroening and Tautschnig (CAV 2013 [V-abs]) and of Wang et al.'s symbolic predictive analysis (FM 2009 [V-abs]), emitted as SMT-LIB2 for small cones. It is never on the critical path.

Complexity: prediction with even one synchronization reversal is NP-complete and W[1]-hard (SyncP [V-abs]); the explicit search is exponential in |C| in the worst case.

### 4.10 Bounded explorer (`kz explore`; stage S7, gated)

1. **Closed model per candidate.**
   - The **cone** ([A §4.5]): actors containing `u`, `W`, `D` or support objects; every actor that writes a cone location or synchronizes on a cone object; the spawn chains that reach them; repeated to a fixpoint. The full synchronization skeleton of the cone actors is kept, never sliced.
   - B constants; Φ semantics at level M; awaits per σ; retries unrolled to effective bounds; inputs from B.Input.
   - **Trace-anchored windows** (optional, [B §4.4]): the prefix is fixed to an observed run up to the latest common S-predecessor of {p, u}. Verdicts then hold "within window W of run r" and are labelled so.
2. **Transition system.** A state is (actor control states, resource states, synchronization states, relevant locals). A transition is one effect at its LP, **or an explicit local choice**: a havoc value (finite domain from value analysis, else `unknown`); an await outcome (success enabled iff the predicate holds; timeout always enabled under σ_U, never under σ_U∞); a star-closure iterate or exit ([J1 X6]).
3. **Dependence** `D(t₁, t₂)` holds iff:
   - both belong to the same actor; or
   - they touch the same key, location or object and at least one changes it; or
   - one may enable or disable the other: lock/unlock of the same lock; give/take of the same semaphore; put/get on the same queue, including capacity; an await versus any write to its predicate's support; a spawn versus the child's first transition; a join versus the victim's last transition.

   Under σ_R1, every transition that can change the readiness of a higher-priority actor is dependent with all lower-priority transitions (a conservative over-approximation, [A §4.5]).
4. **Algorithm.** Source-DPOR with sleep sets (Abdulla et al., POPL 2014 [V-abs]). Disabled transitions are handled through the enabling/disabling dependences of item 3. Correctness does **not** rest on the JACM disabling extension, whose details are status P in [R05 §2.2]. It is the stated obligation L-DPOR (§6.2), validated by differential testing against naive exhaustive enumeration on every model small enough to enumerate (litmus models and random small models). Wakeup trees (optimal DPOR) are an optimization adopted only after the differential test passes.
5. **Reductions**, each switchable and ablated:
   - **L1** (await summarization);
   - **L2** (critical sections irrelevant to the cone are removed, [A §4.2]);
   - **L3** (**lock-atomic macro-steps**). A critical section on lock ℓ whose tracked locations are accessed only under ℓ (checked by a static lockset analysis over the closed IR) is one atomic transition. Two such transitions are dependent only through key or location conflicts. This is the "lock as atomicity, key as conflict" principle in the explorer ([J3-M3]). Without L3, locks are acquire/release transitions, and all critical sections on the ES and SB locks are mutually dependent.
6. **Bounds.** Horizon h (swept). No preemption bound by default; if one is introduced for scalability, BPOR (Coons et al. OOPSLA 2013 [V-abs]) is used, because naive DPOR plus a bound is unsound within the bound, and the bound is swept from 0 upward (SCTBench norm, [R05 §2.1]). Time, memory and execution budgets are `unspecified_by_reference` and swept. The **decided share per budget** is reported.
7. **Two queries** ([R05 §2.3]):
   - `Q_bad` = some execution reaches `u` and is bad;
   - `Q_good` = some execution reaches `u` and is good;
   - (yes, no) → fixed-order within b; (no, yes) → protected within b; (yes, yes) → schedule-dependent; (no, no) → `u` unreachable in the model, compared with the static verdict (a disagreement is O6); budget exhausted → `unknown(budget)`.

   Both queries run under σ_U and σ_U∞ (timeout refinement), and under σ_R1 when requested.
8. **Monitor** per execution:
   - Req at every use LP; Allowed within the horizon;
   - the shape, from vector clocks over API-level edges;
   - a **deadlock check**: all unfinished cone actors are blocked and at least one waits on an object no unfinished actor can release. This generic property exposes OSAL F1 under M_impl;
   - `stall` under σ_U∞.
9. **Kill criterion** ([J3-M3]). At the S7 review, the explorer is run on the development-set cones that lack a certificate (both App-record windows, the C1-derived lookup, the K-PHASE CF instance, the SBNSubPipe K-CAP instance), with budgets swept up to the **maximum budget allocated by the PI** (unspecified_by_reference; decision D15). If none of these cones is decided at that budget, or if the L-DPOR differential test fails, the explorer is frozen. Its outputs are then reported only as an ablation, and verdicts come from certificates, prediction and confirmation.
10. **Fidelity.** Explorer verdicts carry `model(b)` evidence. They rest on O1, O2, O6 and O8, and they are never labelled observed or confirmed.

### 4.11 Conformance (`kz conform`)

- **O1/O2, per actor, on every trace** ([J3-M9]). Each actor's decoded event sequence is run through its skeleton automaton by NFA simulation over the explicit local choices. The cost is linear in events × automaton states. A failure gives the shortest rejected prefix as the model-gap report.
- **O8, global, within a budget, on a sample** ([J1-8]). The question is whether some model execution's projection equals all per-actor sequences, with their outcomes and the matched synchronization. It is answered with the explorer's engine constrained by the trace. Timestamps are used only as the search order heuristic ([A §4.8]). The sampling rule is pre-registered: the first trace of each (B, plan hash) pool, plus every trace with an O1–O7 failure. The budget is recorded. Results are conforming, non-conforming (with the region), or undecided.

### 4.12 Combining evidence into verdicts

1. σ_U first; then σ_U∞ (timeout refinement); σ_R1 on request. σ_E classes come only from observed or confirmed witnesses. σ_E frequencies are pooled only over runs with identical (B hash, plan hash) ([J3-M7]).
2. `protected` and `fixed-order` need a certificate (b = ∞) or exhaustive exploration within b. Observations are supporting evidence only.
3. `schedule-dependent` needs a bad and a good witness. The evidence recorded is the weaker of the two. The evidence ranking used for "weaker" is: `model(b)` and `predicted-extrapolated` (model-based) < `predicted` (trace-sound) < `observed` < `confirmed`.
4. Contradictions trigger O3/O4: the rule is suspended, a model-fault record is written, and the affected verdicts become `unknown(model_fault)`. Nothing is averaged or silently overridden ([A §2.5]).
5. Harm comes from Allowed and the observed or model continuation. With Allowed unspecified, harm is `unspecified_by_reference`, and route/capacity verdicts carry LOSS-UNDER-ORDER.
6. M levels: M_api always; M_impl for instances whose cone touches OSAL idmap, task, binsem or queue paths. Disagreements give the portability-hazard or implementation-defect-candidate sub-labels.

### 4.13 Confirmation on the real binary (`kz hold`, `kz gate`)

**Which verdicts get controlled confirmation** ([A §4.9]):

| Verdict | Action |
|---|---|
| fixed-order (certificate) | passive observation (reachability and outcome). A good counterpart exists only under a changed basis B′ (e.g. another core order): a configuration-change experiment, labelled as such |
| protected | none beyond conformance |
| schedule-dependent / timeout-dependent | directed confirmation of the bad witness; if natural runs show only the bad order, also of the good witness |
| unknown(model_gap, writer_incomplete, conformance) | directed attempts at predicted witnesses, as evidence; the class stays unknown until the model is fixed |
| unknown(budget) | F1 PCT runs if D3 is approved; otherwise remains unknown |
| LOSS-UNDER-ORDER | observation through SB counters |

**Gate placement** ([J2-10], [J3-M10]). To force "event `a` of thread A after event `b` of thread B", hold A at a point before its first event of the pair where all of these hold:

1. **A's program-lock set is empty**, by the observed lock events of the run and by the static lockset. This includes the ES shared-data lock, the SB lock, TBL locks, and the window between a `CFE_ES_DeleteChildTask` lock and its delete (`cfe_es_api.c:1348, 1383`).
2. The point is **not** inside a timebase callback (`handler_mutex` held), a binsem internal section (the internal 2 s `timedlock` would turn other givers and takers into `OS_SEM_FAILURE`, [R01 §6.3]), a signal handler, or before `OS_TaskPrepare` registration.
3. The point is not inside an OSAL RESERVED window, except as a **declared experiment** (e.g. the K-LOCKBAL probe of App. A.4), because a hold there changes other lookups' outcomes ([B §4.6]).
4. At most 4 gate points per thread (debug registers).

If no point qualifies, the rule is applied symmetrically: hold B before its enabling event.

**Release** is event-based: when the awaited event (with its outcome) appears in the live stream of `kz observe`, or when the other party is blocked or has exited ([POL] tool-parameter rule).

**Watchdog** = the smallest reference timeout among the waits that the hold can make expire, read from Φ and B: e.g. `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` for a held startup thread whose child polls `GetTaskFunction`; `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` for core serialization; effective retry bound × effective delay for CF. If the watchdog fires, the result is `inconclusive(await-timeout)`. If a program timeout fires during a hold (seen in the return status), the run is labelled **timeout-exercised (σ_U witness)**: it is realized on the binary, but it is not a natural σ_E schedule.

**Mechanisms.**
- **F0 `kz hold`** (default): `PTRACE_SEIZE` of the target thread by root. An **execution breakpoint in a debug register** (DR0–DR3, set through `PTRACE_POKEUSER`) is placed at the gate, so the text is never modified ([B §4.6]). Only that thread stops (ptrace stops are per thread); the observer keeps recording the others. On release, the debug register is cleared and the thread continues. Measured unit costs: debug-exception trap ≈34 µs; gdb breakpoint ≈203 µs ([R07 §4.7]). The purpose-built holder's stop/resume cost and its interaction with uprobes and with signals (the timebase's real-time signal, `SIGUSR1`) are **unmeasured** and are S6 exit items.
- **F1 `kz gate`** (only after PI decision D3): handwritten `__wrap_*` gates for the synchronization-skeleton APIs, in one C99 file built with the cFS flags; the controller enforces witness schedules or PCT. PCT gives probability at least `1/(n·k^(d−1))` per run for a depth-d bug ([V-abs; mechanism P]), with n and k measured per run and d swept. The guarantee holds only for the gated scheduling points.

**Oracles.** `kz trace` re-evaluates the confirmation trace with the same contracts: the target use must show the not-ready state at its LP and the predicted outcome, and harm follows Allowed. For deadlocks and lock leaks (K-LOCKBAL) the oracle is a **wait-for fact**, never a timeout: the blocked thread's state from `/proc/<tid>/wchan` or futex tracepoints, while the lock's last acquirer has already returned from its API ([C §4.9]).

**Labels.**
- `confirmed(F0|F1)`;
- `derived`: the schedule was constructed; kept apart from original-case results ([R06 S10]; [LED] X5);
- `timeout-exercised`;
- `refuted`: the order was realized and the outcome was good. The model or an API summary is wrong; a model-fault record is written;
- `inconclusive`.

**Perturbation accounting per run**: holds (count, site, duration), probe hits by cost class, OPERATIONAL time, and the order-sensitive outcomes the run did not target (drop counters, init-completion order, the R2 use set), compared with group A (§8.6).

### 4.14 The feedback loop, its reproducibility and its termination (`kz loop`)

```
fix (B, Φ, tool);  K0 := static closure of B;  census0 := kz static(K0)
i := 0
repeat
   plan_i   := kz plan(open instances of census_i ∪ mandatory sync skeleton)
   runs_i   := kz observe(B, plan_i) × n_i           # n_i: unspecified_by_reference, swept; run IDs recorded
   check O1–O5 (+ O8 on the sample); record failures
   K_{i+1}  := K_i ⊔ keys, targets and paths learned from runs_i          # F1, F2
   census_{i+1} := kz static(K_{i+1}) ⊔ kz trace(runs_i) ⊔ kz predict(runs_i) [⊔ kz explore(K_{i+1})]
   for each witness w: π_w := kz hold/kz gate(B, holdplan(w));  census_{i+1} ⊔= kz trace(π_w)   # F3
   write loop-i.kz: run IDs, B/plan/Φ/tool hashes, S0 hashes, deltas, obligation results
   i := i + 1
until census_i = census_{i-1} and K_i = K_{i-1}
report the fixed point together with its run set ∪_i runs_i
```

- **Monotonicity** within one Φ version: keys and targets only become more resolved, and evidence only accumulates. A suspended rule needs a human Φ edit, which produces a new Φ hash and **restarts** the loop. The termination argument is therefore per Φ version ([J1] C assessment).
- **Termination**: under fixed B and Φ, the universe of sites, keys (from code constants and finite tables) and instances is finite, so there are finitely many strict refinements [I].
- **Reproducibility** ([J3-M7]):
  - every analysis is a pure function of stored artifacts (sorted iteration, no wall-clock dependence), so re-running from stored traces must reproduce the verdict store byte for byte; this is a test;
  - σ_E frequencies are pooled only over identical (B hash, plan hash);
  - S0 is snapshotted per run;
  - the fixed point is reported **with its run set**, because observation runs are nondeterministic and a different run set can reach a different fixed point.

### 4.15 Complexity and bounds

| Stage | Cost | Bounds and their sources |
|---|---|---|
| Closure and extraction | linear in TUs; [M] 10 s parse, 4.4 s bitcode, 0.35 s link | — |
| Mod-ref | Andersen worst case cubic ([R05 §2.5]); measured at S3 | — |
| Skeletons | linear in CFG size after inlining | poll loops summarized (L1); retry loops at effective constant bounds (CF: effective `CF_STARTUP_SEM_MAX_RETRIES`; OSAL 5, `osapi-idmap.c:475–480`; `GetTaskFunction` ≤ 21, `cfe_es_apps.c:570–595`); h `unspecified_by_reference`, swept |
| MustBefore | `O(N·R·(d̄+w̄)·R/64)` worst case | — |
| Certificates | O(number of actor nodes) reachability per frontier, plus derivations | — |
| Trace HB and contracts | O(N × number of actors) | — |
| Prediction | NP-complete in general (SyncP); exponential worst case | budget `unspecified_by_reference`, swept |
| Exploration | one execution per Mazurkiewicz class only under L-DPOR; exponential worst case | h swept; preemption bound none by default (BPOR and sweep if used); budgets swept; restart depth 1 per documented request |
| Conformance | O1/O2 linear; O8 a search | O8 budget recorded |
| Confirmation | one run per witness; PCT ≥ `1/(n·k^(d−1))` per run | watchdogs from platform constants; d swept |
| Feedback loop | ≤ size of the finite universe under (B, Φ) | runs per iteration `unspecified_by_reference`, swept |

---

## 5. Observation and instrumentation design

### 5.1 Mechanisms and fidelity classes

Fidelity classes are those of [R07 §7]: F0 = unmodified binary and run conditions, timing perturbed only; F1 = same sources and compiler, link or code-generation option changed; F2 = sources rewritten; F3 = another compiler.

| Mechanism | Class | Role | Unit cost ([M] [R07 §4.7, §5.1] unless stated) | Notes |
|---|---|---|---|---|
| uprobes (perf PMU) + eBPF, root-side, own raw loader | F0 | **default observation** of API-boundary and function-boundary events in `core-cpu1` and app `.so` files | 0.57 µs/hit at entry+4 (emulated `push %rbp`); 31.7 µs at `endbr64`, at `mov %rsp,%rbp` (+5) or any non-emulated instruction; BPF body **unmeasured** | callee-side probes also see the 88 intra-TU API sites that `--wrap` misses |
| tracefs dynamic uprobe events | F0 | **zero-install bring-up backend** (§5.4) | same trap path; fetch cost unmeasured | mountable in a private mount namespace ([R07 §4.7]) |
| HW write watchpoints (perf breakpoint PMU) | F0 | readiness-field stores with no API boundary; DataCollider-style discovery of direct reads during a publication window ([R04 §2.4]) | ≈34 µs/hit; 4 per thread | only for fields written a handful of times per boot |
| ptrace + debug-register execution breakpoints | F0 | confirmation holds (`kz hold`) | #DB trap ≈34 µs; gdb breakpoint ≈203 µs; purpose-built holder **unmeasured** | no Yama; root on a uid-1000 process works |
| sched/syscall tracepoints | F0 | clone cross-check; RacePro-style baseline observation model (§8.3) | not measured | exist on this kernel |
| `ld --wrap` gates | F1 + deviation | PCT and many-point schedules, **only if D3 approves**; perturbation group W | handoff 1.1 µs same CPU / 10.5 µs across CPUs; call overhead "a few ns plus handler" (inference in [R07 §7]) | apps relinked; misses intra-TU calls |
| Source probes | F2 | not planned; fallback only if a needed site cannot be probed at F0 | — | 12.4% of API sites are inside macro expansions ([R07 §4.6]) |
| Clang-built execution | F3 | never | — | — |
| `LD_PRELOAD` / `LD_AUDIT` | — | rejected: cannot interpose symbols defined in the PIE executable; refused by `run_cfs.sh:154–158` | — | [R07 §4.3] |
| gdb | F0 | reference observation only (the R2 records) | +0.6 to +1.1 s to OPERATIONAL ([R2g §1]) | — |

The observer runs as root next to the README-mode `ubuntu` process. This needs a new root-side hook in `env/run_cfs.sh` (PI decision D1, [R07 §9 item 1]).

### 5.2 Probe placement and return-value capture

- **Function entries at entry+4** [D]. gcc emits `endbr64; push %rbp; mov %rsp,%rbp` at `-O0`; this was checked on nine core functions and one app entry ([B §A.2]). At entry+4 the argument registers still hold the arguments.
- **Locks and queues without return probes** ([B §5.5]; [J3-M4]). Mutex order comes from `objseq` at `OS_MutSemGive` entry, inside the critical section. Queue edges come from put entry (descriptor value) and the get outcome only where an edge or a contract needs it.
- **Return values only at contract-use sites** and where an outcome decides an edge: a poll's success return, a lookup's success for reads-from, a put's full/delivered outcome. The capture mechanism is **chosen per site by measurement in stage S1**, never assumed:
  - (R-a) uretprobe on the callee: cost and `ret` emulation are unmeasured ([R07 §4.7]);
  - (R-b) uprobe at the caller's instruction after the `call`: at `-O0` this is typically `mov %eax,-0x..(%rbp)` ([J3 §2.3], objdump). It is inferred to be in the single-step class (31.7 µs) like the measured `mov` at +5 [I];
  - (R-c) uprobe at the callee's epilogue `leave` (epilogues are `leave; ret`, [J3 §2.3]), where `%eax` already holds the return value: class unmeasured;
  - (R-d) **deferred read** [I, to be validated]: out-pointer targets captured at entry are dereferenced at the same thread's next probe hit. This is valid only where the static check shows that no event of that thread between the return and the next probe can write the target.

  Each planned probe records its cost class. If the cheapest valid mechanism for a site is in the 31.7 µs class, the plan reports it, and the perturbation protocol measures its effect (§8.6). No overhead claim is made before S1.
- **Internal points** (publication stores not at an API boundary, e.g. the `static inline` `CFE_ES_AppRecordSetUsed`, §3.17): use an entry probe on an out-of-line local copy if DWARF shows one [U]; otherwise a write watchpoint (these stores occur a handful of times per boot); otherwise a probe on the nearest instruction of a cheap class.
- **Site validator.** Every planned address is disassembled with LLVM MC and looked up in the calibration table (§5.10). Expensive sites are reported, never accepted silently.
- **App `.so` files.** Uprobes are registered by file and offset before launch. Pre-registration was tested only on the main binary ([R07 §4.7, §9 item 6]); S1 tests it on a not-yet-`dlopen`ed `.so`. Fallback: hold the ES startup thread (F0) at the return of `OS_ModuleLoad` for that module until the probes are attached. The hold is recorded as perturbation.

### 5.3 eBPF data path

- **One generic BPF program per phase** (entry, point, return, watch). It looks up a **site descriptor** in a hash map keyed by the probed instruction address. The descriptor holds the site id, the registers, stack slots and bounded dereferences to capture, whether to read a bounded string (bounds `OS_MAX_API_NAME` 20 and `OS_MAX_PATH_LEN` 64 from the built `osconfig.h`, [B §A.2]), whether to assign an `objseq`, and whether to capture a user stack. Descriptors are **data** loaded by the user-space loader; there is no per-site code generation ([C §5.3]).
- **Per-thread `tseq`** in task-local storage (or a map keyed by tid). **Per-object `objseq`** by atomic add on a per-object map entry, assigned only at in-critical-section points, so that counter order equals acquisition order [I].
- **Buffers**: per-CPU ring buffers (or a perf event array). No lock is shared across CPUs, nothing is shared with the traced program in user space, and the traced threads make no extra system calls and take no user-space lock. v0.1's per-task-buffer goals ([R06 F10]) are met by keeping the buffer outside the process.
- **Stacks**: `bpf_get_stack` with `BPF_F_USER_STACK` (frame-pointer unwinding of gcc `-O0` code), at sites that need on-behalf frames. Depth: the maximum static call depth from task entries to the probed site, capped by `perf_event_max_stack`. Truncation is flagged.
- **Feasibility** [U]. Raw `bpf()` program load and attach to a uprobe were probed ([R07 §4.7]); the UAPI header with ring-buffer and task-storage map types is present ([J3 §2.4]). Ring buffers, task storage, `bpf_get_stack` and IP-keyed lookups were **not** probed. S1 tests each one. The fallback is the perf sample path (`PERF_SAMPLE_REGS_USER`, `PERF_SAMPLE_STACK_USER`).

### 5.4 The tracefs fallback (zero install, bring-up)

- Dynamic uprobe events with fetch-args (registers, bounded dereferences, strings), in a private mount namespace (mountability probed, [R07 §4.7]).
- There are no in-kernel `tseq` or `objseq` in this mode. Per-thread order is taken from the thread's own events in timestamp order, under the recorded assumption **A-CLK-PO**: a cross-CPU-consistent monotonic clock. The clocksource is `tsc` with `constant_tsc` and `nonstop_tsc` on all four vCPUs ([B §A.2]).
- **Cross-thread order comes only from matched protocol keys**: queue descriptors, outcome-implied reads-from, and poll success returns. Lock order inferred from the timestamps of in-critical-section give probes is recorded as `clock-ordered` evidence. It is **excluded** from VC, VC_R, certificates and prediction constraints ([J2-4]). Verdicts that would need it are weakened (the edge is "ambiguous").
- Purpose: bring-up, and R2 re-observation in stage S1. R2's evaluation needs no lock edges: the uses carry their own `ILLEGAL_APP_ID` outcome, and their order relative to the publication follows from program order, task creation and the core sync's success return. The raw `bpf()` loader is the production backend. Both backends need no installation.

### 5.5 Event record and trace container

The raw record `kz_raw_ev` (64 bytes, §2.3) carries a timestamp (evidence only), tid, `tseq`, site id, phase, flags, `objseq` and four decoded values. Strings and stacks go into side sections keyed by `(tid, tseq)`.

A `.kzt` container has these sections:
- `SITES`: copied from the plan, with the binary build-id and site addresses;
- `RAW_EVENTS`, `STRINGS`, `STACKS`;
- `LOSS`;
- `RUNMETA`: B hash, plan hash, S0 hash, kernel version, backend, cold/warm state, holds, harness version.

`kz trace` decodes `RAW_EVENTS` into typed `EVENTS`: actor instance, operation, resource, outcome, identity quadruple, template link and LP interval.

### 5.6 Loss accounting

Ring-buffer reservation failures increment per-CPU, per-site counters. Per-thread `tseq` gaps identify the lost records of each actor. Any loss marks the affected actor's interval as incomplete, and every verdict whose evidence depends on that interval becomes `unknown(loss)`. **Loss is never treated as the absence of events.** Buffer size is set from a calibration run's high-water mark and recorded ([B §5.2]).

### 5.7 Identity bootstrap without `GetAppID`

No in-process lookup is ever made by the instrumentation; all joins are offline. An instrument that called an OSAL name lookup could trigger the very F1 lock leak it is meant to detect ([R01 §6.3]). Calling `GetAppID` early would itself hit the App-record window ([v0.1] §8.4).

1. Every record carries the kernel tid.
2. Creator side: `OS_TaskCreate` entry (name, entry pointer, priority) and its out-parameter.
3. Child side: `OS_TaskEntryPoint(osal_id)` (exported, `osapi-task.c:124`) runs in the child before `OS_TaskPrepare` and binds tid ↔ OSAL id. The static `OS_PthreadTaskEntry` (`os-impl-tasks.c:131`) is a `.symtab` fallback ([J3 §2.4]).
4. ES side: `CFE_ES_StartAppTask` arguments (`ParentAppId`), the task-record publication, and the App-record publications (AST-bound sites, §3.17). ES task IDs derive arithmetically from OSAL IDs ([R02 §6.2]).
5. Timebase thread: `OS_TimeBase_CallbackThread(timebase_id)` (`osapi-timebase.c:365`). Callbacks are attributed through `OS_TimerAdd` records (creator → on-behalf frame).
6. Raw threads by creation site. Kernel thread names set by the PSP (15 characters, `cfe_psp_start.c:197–201`) are a second, independent check only.
7. Generations from OSAL and cFE serials. Across runs, actors and resources are matched by `(type, name, creator, creation ordinal)`.
8. `svc` frames are recovered by symbolizing user stacks against the gcc binary. `stamp` comes from arguments, using Φ's parameter roles.

### 5.8 Causal tokens versus timestamps

Cross-thread order comes **only** from matched synchronization: `objseq` for mutexes; descriptor plus FIFO for queues; descriptor plus pipe for SB; outcome-implied reads-from for registries; the recorded exit status for polls; OSAL ID for task creation. Semaphore give/take matching uses the conservative join. Where a protocol key is insufficient, a timestamp window may be used as *matching evidence*: the edge is then marked ambiguous and the affected evidence is weakened. **`CLOCK_MONOTONIC` values never become HB edges** ([R04 §2.1, §4 row 16]; v0.1 §9.3).

### 5.9 Safety rules for observation and control

- eBPF runs in kernel context. It calls no OSAL API, takes no user lock, never prints, and cannot be cancelled mid-record. This removes, by construction, the hazards of in-process probes listed in [R01 §6.3]: self-deadlock on NORMAL table mutexes, console serialization, cancellation while a probe lock is held.
- What remains is timing. A trap inside an OSAL table-locked region lengthens the hold and could consume bounded RESERVED-wait attempts. The planner therefore prefers entry probes outside OSAL internal locks, and it flags any probe inside an idmap critical section, a timebase callback or the binsem internal section.
- Holds follow the gate-placement rule (§4.13): empty program-lock set; not in `handler_mutex` callbacks, binsem internal sections, signal handlers or before `TaskPrepare`; RESERVED windows only as declared experiments; reference watchdog; event release.

### 5.10 Cost model and measurement obligations

Per thread `i`: `ΔC_i = Σ_j n_ij · c_j` (v0.1 §11.2); per confirmation run add `Σ_holds (c_hold + hold duration)`. **Five costs are reported separately**: analysis, exploration, recording, control, and profile authoring (person-hours) ([R06 R4]; [J1-15]).

| Quantity | Value | Status |
|---|---|---|
| `c_trap` at entry+4 (`push %rbp`) | 0.57 µs | [M] |
| `c_trap` at `endbr64`, `mov %rsp,%rbp` (+5) or another non-emulated instruction | 31.7 µs | [M] |
| post-call `mov %eax,-0x..(%rbp)` | inferred 31.7 µs class | [I]; measured in S1 |
| uretprobe; uprobe at `leave`; `ret` emulation | — | unmeasured; S1 |
| `c_bpf` (program body, string fetch, stack capture) | — | unmeasured; S1 |
| `c_wp` (HW watchpoint hit) | ≈34 µs | [M] |
| `c_hold` (gdb breakpoint) | ≈203 µs | [M]; purpose-built holder unmeasured (S6) |
| ptrace × uprobe and ptrace × signal interactions | — | unmeasured; S6 |
| gate handoff (F1) | 1.1 µs same CPU / 10.5 µs across CPUs | [M] |
| `n_ij` per boot | — | unmeasured for the full sync skeleton. For scale only: the retired CORD recorder logged 626–632 events per boot with a limited probe set ([R2c §4.1]); there are 240–258 `sb.appid` uses per boot ([R2c §7]) |
| static API call sites | 3,762 | [M] a static count, not a dynamic one ([R07 §4.2]) |

Arithmetic on measured values only, not a prediction [I]: 626 hits at 0.57 µs is about 0.36 ms per boot; at 31.7 µs it is about 20 ms ([B §5.6]; [J3 §2.3]). OPERATIONAL is about 1.6 s ([M] 1613–1636 ms without a debugger, [R2g §1]). The startup burst, where the order-sensitive outcomes live (`SBNSubPipe` overflow about 47–49 ms after OPERATIONAL, [BM §1.3]), is where probe time concentrates. This is why S1 measures before any plan is fixed.

**Calibration table.** S1 measures each instruction class with a micro-benchmark on this VM, by the method of [R07 §4.7]: `push`, `mov`, `leave`, `ret`, uretprobe, BPF body with and without string and stack capture, and the holder's stop/resume. The table is versioned by kernel. **Group A is re-measured on `-fc-v114`** before any comparison ([R07 §1]; D11).

### 5.11 Untested items and fallbacks

| Item | Status | Fallback |
|---|---|---|
| uprobe pre-registration on an app `.so` before `dlopen` | untested ([R07 §9 item 6]) | hold the startup thread at `OS_ModuleLoad` return, attach, release (recorded) |
| ring buffer, task storage, `bpf_get_stack`, IP-keyed descriptor lookup | not probed | perf sample records with registers and user stack |
| return-capture mechanisms (R-a…R-d) | unmeasured | calibration decides per site; deferred read only where statically valid |
| `c_bpf` with stack capture | unmeasured | capture stacks only at sites that need frames |
| purpose-built ptrace holder cost; ptrace × uprobe interaction | unmeasured | gdb-equivalent bound (≈203 µs per stop) for planning only |
| BPF ELF loading without libbpf | design inference | install libbpf (D2) |
| out-of-line copies of `static inline` publication helpers at `-O0` | [U] | write watchpoint at the store |

---

## 6. Guarantees

All statements are relative to the basis B, the profile Φ (hashed, every rule cited), the model level M, the semantics σ, the bound b, and the assumptions A below. Terminology follows [R05 §0.5]: "over-approximate relative to M and A", "complete up to b"; never a bare "sound". This is "soundy" in the sense of Livshits et al. (CACM 2015 [V-abs]).

### 6.1 Assumptions A (the soundiness manifest)

| Id | Assumption | Effect if violated | Mitigation / visibility |
|---|---|---|---|
| A1 | Rule validity: every Φ rule and effect is correct for the pinned revision | wrong edges or effects | each carries a citation; O3/O4 refute; certificates list the rules used |
| A2 | Binding correctness: Clang sites and AST patterns map to the gcc binary's instructions | wrong probes or anchors | anchor audit (§4.1 step 7) |
| A3 | Closure: actors come only from the startup script, the generated object table, `CreateChildTask` sites, OSAL timebase/timer creation, PSP threads and environment inputs; `dlsym` targets are resolved by configuration; residual indirect calls are `unknown_effect`; code outside the compile DB is out of model | missed actors or targets | closure boot (O5); `unknown_effect` → T-OBS disabled / T-SPAWN empty |
| A4 | Memory model: sequential consistency at event granularity on x86-64 TSO at `-O0`; ad hoc flag synchronization is an assumed-visibility edge | weak-memory effects missed | edges tagged `assumed`; sub-label on affected verdicts ([R06 T8]); D10 |
| A5 | API-level edges only for shapes; OSAL-internal locks constrain interleavings only under M_impl | — | ablation "implementation-level locks as synchronization" |
| A6 | Observation completeness for planned sites, with loss accounting | missed events | `tseq` gaps; `unknown(loss)` |
| A7 | Operation semantics: Φ's semantics of pthread, semaphores, message queues, the kernel and OSAL at LP granularity over-approximate the real behaviour (equal-priority FIFO for `mq` is UNVERIFIED, [R04 §4 row 7]; OSAL always uses priority 1) | containment fails | litmus suite on the real OSAL (§9.2); O4 |
| A8 | Out of model: inline assembly, signals beyond the PSP exception path, ISRs, DMA, weak-memory effects | — | listed |
| A9 | Inputs: B.Input is the complete set of external events of the run | ground-driven paths latent | scenario families as B′ |
| A10 | M_impl covers the OSAL idmap, task, binsem and queue paths only | other primitives at API level in both | listed |
| A11 | Table values from the basis; runtime table loads only via B.Input | — | table cross-check against `.tbl` |
| A12 | One process; SBN peers and cpu2 out of model | cross-instance contracts not covered | listed ([R03 §9 item 8]) |
| A-SKEL | Skeletons over-approximate each actor instance's projected behaviour under B | spurious or missing model behaviour | O1, O2, O8 |
| A-WS | Writer-set completeness: `kz modref` is sound for the closed IR, and every `DECLARED_TRUSTED` guard is correct | unsound T-OBS | O7; trusted guards listed per certificate; O3/O4 refute; ablation "Φ-only" vs "mod-ref-only" |
| A-API | The abstract trace is faithful enough for the soundness of SHB/WCP/SyncP to transfer | prefilter proposals wrong | prefilter outputs are always re-validated (§4.9 step 5) |
| A-CLK-PO | tracefs backend only: a cross-CPU-consistent monotonic clock for per-thread order | wrong program order | recorded in RUNMETA; the eBPF backend removes it |
| L1, L2, L3 | Lemma obligations (await summarization; irrelevant critical sections; lock-atomic macro-steps) | model behaviour changed | each switchable per instance; ablated; proofs written before the evaluation freeze |

### 6.2 Lemmas and proof sketches

The model: per-actor skeleton automata plus Φ semantics. `Exec(B,σ_U)` is the set of interleavings consistent with them. By A-SKEL and A7 it contains the real executions.

**L-MB (MustBefore soundness).** For every execution `e ∈ Exec(B,σ_U)`, every node `n`, every occurrence `n_k` of `n` in `e` and every `a ∈ MB(n)`, an occurrence of `a` precedes `n_k` in `e`.

*Sketch.* Induction on the position of `n_k` in `e`. Take finite prefixes; the fixpoint equations hold for the computed MB. Case by the equation that kept `a` in MB(n):
- **(T-PO)** The actor reached `n_k` from some skeleton predecessor occurrence `p_j`, which is earlier. Terminal ops have no successors, so `p_j`'s node is a predecessor in the equation. Since `a ∈ MB(p) ∪ {p}`, the induction hypothesis on `p_j` gives an earlier occurrence of `a`.
- **(T-SPAWN)** An actor's entry is preceded by one of its spawn sites (A3: the list is complete). `a` is in the meet over all spawn sites, so the hypothesis applies to whichever site occurred.
- **(T-OBS)** `o_k` observed `C` true. `C` was false right after the latest occurrence of `f` before `o_k` (obligation), and `f` precedes `o_k` (T-PO). So some write between them made `C` true. By A-WS it is in `W_C`; it may occur after `f`, so it is in `W_o`. `a` is in the meet over `W_o`, so the hypothesis applies to that write's occurrence.

Locks contribute nothing. ∎

**L-PWB.** If `m` is in actor instance `τ` of `a`, `m ∉ Anc(a)`, and `m ∈ MB(b)`, then in every execution containing occurrences of `a` and `b`, every occurrence of `a` precedes every occurrence of `b`.

*Sketch.* By L-MB, an occurrence of `m` precedes each occurrence of `b`. If some occurrence of `a` came after an occurrence of `m`, then `τ`'s run would contain `m … a`, and by A-SKEL the skeleton would contain a path `m →* a`, contradicting `m ∉ Anc(a)`. So every `a` precedes every `m`, hence every `b`. ∎

**L-FO (fixed-order certificate).** Suppose a FIXED_ORDER certificate (§4.5) exists for instance `i` with conjunct `P_j`. Then in every `e ∈ Exec(B,σ_U)` and at every occurrence of `u`, `P_j` is false at `u`'s LP, so `u` is not ready. With the badness condition, every execution reaching `u` is bad.

*Sketch.* Suppose `P_j` were true at an occurrence `u_k`. `P_j` was false at INIT (INIT_FALSE), so some writer `x ∈ W_j` (complete by A-WS) occurred before `u_k`. By L-PWB, with the frontier `m ∈ ⋂ MB(x)`, every occurrence of `u` precedes every occurrence of `x`. Contradiction. The proof never requires `u` to dominate anything, so uses behind early returns are covered ([J2-1]). ∎

**L-PR (protected certificate).** Case (a) is immediate. Case (b): by L-MB, `x*` occurred before `u_k` and established `P_j`. Every disabler `d` satisfies `PWB(d, x*)`, so it occurred before every `x*`; or `PWB(u, d)`, so it occurs after every `u`. Hence no disabler lies between the last `x*` and `u_k`, and `P_j` holds at `u_k`. ∎

**L-CONT (containment) and transfer.**
- `σ_U∞` keeps all transitions of σ_U except timeouts and adds a fairness restriction, so its executions are executions (or prefixes) of σ_U.
- `σ_R1` only restricts which enabled transition fires.
- For `σ_E`: by A7, every real step is a step allowed by Φ's semantics at LP granularity, and a real timeout is one of the nondeterministic timeout transitions of σ_U. By A-SKEL each actor's real sequence is a skeleton path. By A3 and A9 no actor or input is missing. By A4 interleaving at event granularity is adequate. Hence `Exec(B,σ_E) ⊆ Exec(B,σ_U)`.
- Transfer follows: a universal class over σ_U holds over any subset, for the executions in the subset that reach the use; an existential witness in a subset is a witness in σ_U. A frequency is a measure on σ_E and is not defined by σ_U. ∎

**L-DPOR (explorer correctness statement)** ([J1-6]; [J2-13]). Local nondeterminism (havoc values, await outcomes, star-closure choices) is made into explicit transitions of the acting actor, so each actor is deterministic given its choice transitions. Dependence is as in §4.10 item 3. The explorer then claims: every state reachable in the closed model within b is reachable through some explored execution that is Mazurkiewicz-equivalent to it, and every equivalence class with a bad (or good) occurrence of `u` within b is represented. For source-DPOR with sleep sets on deterministic transition systems this is the guarantee of Abdulla et al. (POPL 2014 [V-abs]); its extension to disabling is cited only with status P ([R05 §2.2]). The claim is therefore made **conditionally on the differential test** (equality of the sets of reached (class, shape, outcome) results with naive exhaustive enumeration on every model small enough to enumerate). Results are reported as C-2 claims of [R05 §5.2]: "fixed-order / protected / schedule-dependent in M under b". With a preemption bound, BPOR (V-abs) is used and the claim is "under bound p". Under σ_R1, the conservative priority dependences keep coverage under the scheduler restriction [I].

**L1 (await summarization), L2 (irrelevant critical sections), L3 (lock-atomic macro-steps)** are proof obligations, stated as in [A §4.2] for L1 and L2 and in §4.10 for L3. Until they are proved in the paper they are listed as assumptions, are switchable per instance, and are ablated (§8.4).

**L-PRED (prediction).** Every predicted witness is a linearization of Pred(π). It is therefore feasible for any program able to produce π, under A4, A7 and Φ's primitive semantics (maximal causal models, RV 2012; RVPredict PLDI 2014 [V-abs]). A `predicted-extrapolated` witness is additionally relative to A-SKEL for the extrapolated loop.

**L-LOOP (termination).** Under fixed (B, Φ, tool), the loop's state lives in a finite lattice (sites, keys from constants and finite tables, instances, evidence flags), and every iteration either strictly refines it or stops. A Φ edit starts a new loop.

### 6.3 Guarantee statements

- **G1 (census over-approximation, soundy).** For B, Φ, M and A, every contract use reachable in the configuration-closed skeletons yields an instance with a class or `unknown(reason)`. Instances outside the closure are counted and listed. This is claim template C-1 of [R05 §5.2].
- **G2 (certified verdicts).** A fixed-order or protected verdict with evidence `static-certificate` holds for every execution in `Exec(B,σ_U)` under A, by L-FO / L-PR. By L-CONT it also holds under σ_U∞, σ_R1 and σ_E wherever the use is reachable. Each certificate has passed `kz proofcheck` and lists its trusted entries.
- **G3 (observed violations).** An "observed violation" is reported only when, in the recorded trace, the state at the use's LP computed from HB-ordered lifecycle LPs is outside Req, **and** the program's own outcome code agrees (A2, A6, O4). It makes no claim that the contract is the "right" requirement; its provenance is shown.
- **G4 (predictive soundness relative to the trace model).** Every `predicted` witness is in Pred(π) (L-PRED). Prediction is not maximal; "not predictable" is never "protected".
- **G5 (bounded model classification).** Within the closed model, σ and b, and conditional on L-DPOR's differential test, explorer classes hold in M under σ within b (C-2 of [R05 §5.2]). Fidelity evidence: O1, O2, O6, O8.
- **G6 (confirmation).** A `confirmed` verdict was reproduced on the gcc-built binary of B under the declared control (F0 holds; or F1 gates if approved), with the contract oracle evaluated on the confirmation trace and the perturbation reported (C-3 of [R05 §5.2]; precedents RacePro, DESCRY, CTrigger [V-abs]). Confirmed reports therefore contain no false alarm relative to the recorder's oracle.
- **G7 (PCT, if D3 is approved).** Each F1 run finds a depth-d bug, relative to the gated scheduling points, with probability at least `1/(n·k^(d−1))`, with n and k measured (C-4 of [R05 §5.2]).
- **G8 (loop termination and reproducibility).** Under fixed (B, Φ, tool) the loop terminates (L-LOOP). Re-running the analysis from stored artifacts reproduces the verdict store.
- **G9 (inconsistency surfacing).** Every contradiction between layers (O1–O8, CC) is reported and suspends or downgrades the rule or model involved. None is resolved silently.
- **G10 (writer-set transparency).** Every certificate lists every writer entry it relied on, with its status (`COMPUTED`, `DECLARED_VERIFIED`, `DECLARED_TRUSTED`). The census reports the number of conjuncts with T-OBS disabled.

### 6.4 Not claimed

1. "Sound", "complete", "detects all races" or "no false positives", without the qualifiers above ([R06 §6.3 item 1]).
2. "Safe" or "no violation exists" from the absence of an alarm; from a bounded exploration beyond its bound; from prediction or prefilter silence; or from a model whose obligations failed.
3. "Fixed-order" or "deterministic" from repeated failing runs; only from a σ-stated certificate or exhaustive bounded exploration ([R06 S9]).
4. "Fixed-order" from a trace-local robust chain (VC_R) without a static certificate ([J1] §8).
5. "Exact within b" for DPOR without the local-nondeterminism treatment and the differential test.
6. A σ_E frequency or class from a σ_U prediction or model witness; any priority-based verdict carried between σ_R1 and σ_E ([R06 §6.3 item 14]).
7. "No transfer across σ" (too weak), or σ_U-to-σ_E frequency transfer (too strong).
8. "R2 is a race": its evidence and proof shape make it a fixed-order dependency error ([R06 S6]).
9. "C3 or C5 reproduced on v7.0.1": their buggy code is absent there ([R06 §6.3 item 9]).
10. "Existing tools fail on C1–R2": every external process-level tool is `NOT_RUN`; NOT_RUN is never a false negative ([R06 S5]).
11. "MLIR improves detection or precision", unless G-MLIR shows a measured win on a stated metric.
12. Generality from the five design cases; "new bugs" for derived, role-derived, seeded or configuration-changed tests ([R06 §6.3 items 7–8]).
13. Flight-representative timing or overhead from Linux `SCHED_OTHER` in permissive mode ([R06 §6.3 item 10]).
14. Novelty of HB/vector clocks, DPOR, PCT, lockset, delay injection, predictive analysis, SMT encodings or points-to analysis as mechanisms. Novelty is claimed only for the combination around the cFS semantics (gaps G1–G8 of [R04 §5]).
15. Anything about F Prime or other frameworks beyond hypothesis H-F (§7.6).
16. Memory-model correctness of the ad hoc synchronization sites (assumed, A4).
17. "24 R2 violations without a debugger" for any build other than one with ENV's version table, and any R2 count as a Kairos result before Kairos's own recorder reproduces it ([R06 §6.3 item 13]).
18. "Lost packets" for rejected event requests: EVS rejects them before any packet is built ([R06 §6.3 item 15]).
19. OSAL F1 (lock leak), F7 (silent task loss) or either App-record window as bugs before confirmation runs; derived confirmations are labelled derived.
20. "First process-race detector" ([R06 §6.3 item 11]).

---

## 7. Openness

### 7.1 Discovery sources (the union)

Contracts are written per **API and resource kind**, never per case, and the census instantiates them at every context-qualified site in cFE core and the 15 bundled apps. Candidates come from the union of these sources ([J1-13]):

| # | Source | Mechanism | Output | Origin |
|---|---|---|---|---|
| 1 | Static census | every Φ template at every matching context-qualified site; key resolution by value analysis and table evaluation | instances with a class or `unknown(reason)` | [A], [C] |
| 2 | Outcome mining | every not-ready outcome code observed at the API boundary (vocabulary from the extracted `\retval`/`@retval` lists) creates an instance even when no template names the API | `template: none` instances, triaged | [B §4.1] |
| 3 | Key-pair enumeration | for every resource key with lifecycle transitions and uses in different actors, the pairs Φ marks as conflicting | candidates | [B §4.1] |
| 4 | Retry/timeout-loop-derived contracts | every recognized retry or poll loop around an operation implies a contract for it, with `Allowed = retry_until_ready(effective bound)`; provenance kind `code_pattern` | templates | [C §7.1] |
| 5 | Lifecycle symmetry | every kind with create and delete operations automatically gets K-REG, K-LIFE and K-GEN | templates | [C §7.1] |
| 6 | API documentation | every `\par Assumptions` block (138 in cFE core API headers) is an annotation candidate. A human turns it into a Φ entry, and the effort is counted. Examples: `cfe_evs.h:168–172` (K-ID); `cfe_es.h:400–403` (K-PHASE); `cfe_sb.h:449–464` (buffer lifetime) | templates | [R06 §5.3] |
| 7 | Code admissions | comments stating an ordering assumption or an acknowledged race: CF "start up race condition" (`cf_cfdp.c:1272–1278`); CF TOCTOU (`cf_cfdp_s.c:447–463`); the OSAL lock invariant (`osapi-idmap.c:384–385`); the PSP exception-ring race (`cfe_psp_exception.c:72–86`); ES's "informational" wake (`cfe_es_backgroundtask.c:254–257`); the EVS guard (`cfe_evs_utils.c:613–623`); the TBL lock rationale (`cfe_tbl_api.c:145–149`); TIME's unregistration comment (`cfe_time_tone.c:1322–1325`) | templates | [A §7.1], [C §7.1] |
| 8 | Belief deviance | for each API, the fraction of call sites that test its status; deviants are ranked (e.g. `cfe_sb_task.c:138`, `cfe_tbl_task.c:196` ignore `GetAppID`'s status). Engler et al. SOSP 2001 [V-me]. **A ranking only, never a verdict** | probe priorities | all |
| 9 | Always-one-order pairs | pairs observed in only one order across runs become flip candidates for prediction (CloudRaid ESEC/FSE 2018 [V-abs]) | candidates | [B §7.2] |
| 10 | Per-app profiling under variant bases | one-app-at-a-time runs collect potential publish → use communications independent of boot order (Snowboard SOSP 2021 [V-abs]). Candidates only; also a cross-check of static publisher sets | candidates | [A §7.3] |
| 11 | Watchpoint discovery | during a publication window, a HW watchpoint on the field records accesses by other actors (DataCollider OSDI 2010 [V-abs]) | candidates | [B §7.2] |
| 12 | Configuration variants | new bases B′ (§7.4) | per-B′ results | all |
| 13 | Held-out protocol | §7.5 | held-out results | [R06 §5.6] |

### 7.2 Contract governance and anti-circularity

- **Two readiness oracles.** Φ-declared effects (the working model) and framework validity predicates (an independent oracle: support locations statically, outcome codes dynamically). Agreement is checked by O7 statically and O4 dynamically ([J1-12]; [J1 X9]).
- **Provenance per Φ entry.** Kind (`api_doc`, `code_admission`, `code_pattern`, `issue`, `researcher`), `path:line`, sha256 of the cited text, author, authoring time, and which design cases the author had inspected for that API ([R06 §5.3 item 4]).
- **Freezes by hash** ([J3-M11]):
  - **F1**: Φ, extractor, engines, calibration table and lints, frozen before the first evaluation run on the development cases;
  - **F2**: frozen before any held-out item is opened.
- **Template-only first.** The first evaluation run uses no case-specific entry. Later refinements are labelled `oracle refinement`, and the verdicts they change are reported separately ([R06 T2]).
- **Honesty about blindness.** The authors already know R2 and the windows, so a template-only run on the development set is not blind. There, anti-circularity rests on header provenance. Blindness is claimed only for held-out sources.
- **Authoring effort** (person-hours, number of entries, lines of Φ) is measured and reported ([R06 R4]).
- **Lints** ([A §9.3]): constants in tool code only as `KZ_REF(value, "path:line")`; no case identifiers or case-specific symbols (e.g. `CFE_SB_Global`, `sem_name`, `C1`) in engine code outside `profiles/` and `tests/`.

### 7.3 Candidates beyond the five cases

Every row comes from a generic template applied by the census, not from case knowledge. Each stays a **candidate** until the loop produces evidence. A confirmation that needs a constructed schedule is labelled derived. All expected verdicts are [I].

| Candidate | Template | Source facts | Expected first verdict [I] | Path to evidence |
|---|---|---|---|---|
| **Core-task App-record window** (EVS, ES, SB, TBL, TIME `GetAppID` before `cfe_es_start.c:818`) | K-SPAWN.app-record | §3.3; [J1 §1.1]; [LED §3.4] | open: O-ordered (ES lock) → prediction/explorer; schedule-dependent under σ_U and σ_U∞ (only a 50 ms yield separates them); timing-protected in σ_E | lock-reversing prediction from default boots; derived F0 hold of the startup thread between `:810` returning and `:818` (D16) |
| **SB/TBL identity never published** (consequence of the window; status ignored at `cfe_sb_task.c:138`, `cfe_tbl_task.c:196`) | K-ID with shape ABSENT; chain root = the window instance | §0.5 | the post-publication SB-AppId uses (211–229 ordered per boot, [R2c §7]) and TBL identity uses: no protected certificate; schedule-dependent(σ_U), ABSENT, chained (race coverage: root reported) | as above; the K-ID oracle (`ILLEGAL_APP_ID` on every later SB event) |
| Script-app App-record window; `GetAppIDByName` success with `0xFFFFFFFF` | K-SPAWN.app-record, K-REG.es | §3.3 | open: O-ordered → prediction; schedule-dependent(σ_U, σ_U∞) | App. A.3 |
| OSAL lookup of a RESERVED record leaks the type lock (F1) | K-LOCKBAL (M_impl) + K-REG.osal | `osapi-idmap.c:384–385` vs `:401–404` | implementation-defect candidate under M_impl if reachable; protected pairs where creation is certified earlier | explorer deadlock check; declared-experiment hold inside the creation window + wait-for oracle (App. A.4) |
| Silent task loss after the OSAL bounded wait (F7) | K-SPAWN.osal (M_impl) | `osapi-idmap.c:475–480`; `osapi-task.c:106–109` | schedule-dependent in the model under σ_U | hold the creator between thread creation and finalize until the child's preparation fails |
| Cancellation while a program lock is held | K-LOCK.cancel | `cfe_es_api.c:1348, 1383`; [R01 §2.4] | open; needs a delete input (B′) | `B_restart` family |
| SB pipe name visible before the descriptor commits | K-REG.sb-pipe-name | `cfe_sb_api.c:153, 163, 190–194, 802–839` | open where a by-name lookup exists | observation; prediction |
| TBL owner load vs ground load claim | K-ATOM.tbl-load | `cfe_tbl_api.c:305–309`; `cfe_tbl_task_cmds.c:423–431` | inactive in the default B | input variant B′ |
| TIME synch callback after its module is unloaded | K-LIFE.callback | `cfe_time_tone.c:1310–1333`; `cfe_es_apps.c:1526–1545, 1595–1609` | open; restart family | `B_restart` |
| Post-OPERATIONAL burst: SBN's reporting toggle vs TO_LAB's 37 subscriptions | K-MODE + K-CAP | [R03 §5.2]; [M] `SBNSubPipe` overflowed in 5 of 6 M1 runs ([BM §3.3]) | schedule-dependent in σ_E (both outcomes observed); LOSS-UNDER-ORDER | uncontrolled runs give σ_E witnesses; App. A.5 |
| SCH_LAB wakeups vs late subscribers | K-ROUTE | SCH_LAB's first send ≥ 1 s after its init; [M] margin 0.12–0.9 s ([R03 §6.3]) | schedule-dependent(σ_U), LOSS-UNDER-ORDER; good in all recorded σ_E runs | prediction; a hold of the subscriber |
| K-PHASE: CF's retry window vs the soft startup barriers | K-PHASE + K-REG.osal | [R03 §2.6]; `cfe_es_start.c:210–228` | in a C1-derived B′: timeout-dependent (violation needs a soft-barrier timeout) | explorer under σ_U vs σ_U∞ |
| HS AppMon counts "name not found" as "no progress" | K-REG.es (harm unspecified) | `hs_monitors.c:69–160` ([R03 §2.10]) | inactive with default tables (all `NOACT`) | table variant B′ |
| HS system-monitor device start fails | (outcome mining) | `hs_sysmon.c:50–72`; `psp_module_list.cmake:9` | `capability-absent(B)` | App. A.6 |

### 7.4 Configuration variants and scenario families

The dependency graph is configuration-driven: six apps subscribe from tables, several take names or symbols from tables, and the schedule decides who wakes when ([R03 §7]). Each variant is a new basis with its own manifest and provenance status, never pooled with the default B. Nothing found only in a variant is a "new bug" of the default configuration (D18):

- tables that activate latent paths (CF `sem_name` with a provider, public-code-derived from nasa/bp [LED §1a]; LC watchpoints/actionpoints; MD dwell symbols; HS AMT/EMT; CS tables);
- alternate schedule tables, including the wakeups absent from the default schedule ([R03 §6.2]);
- startup-script order; `MISSION_CORE_MODULES` order (a configuration-change experiment, never schedule evidence);
- the cpu2 target present in the build tree;
- effective queue depth (`msg_max`, D19);
- inputs: restart/reload/delete (`B_restart`), table load/validate/activate, SB pipe info;
- initial state: POWERON vs PROCESSOR reset with preserved CDS (run sequences).

### 7.5 Held-out protocol

Adopted from [R06 §5.6]:
1. **Freeze F2** (§7.2) before any held-out item is opened.
2. **Held-out sources**, each counted separately and never added to "new real bugs":
   - (a) other cFS revisions (version transfer; the profile is re-bound by AST pattern, not by line number);
   - (b) other configurations (cpu2; table variants);
   - (c) issues mined with a query fixed before F2 (keywords and date cut), excluding C1, C2, C3, C5 and R2;
   - (d) seeded ordering mutations drawn from the real fix history (remove a wait, move a publication, swap object-table entries), labelled synthetic;
   - (e) in-code admissions not used during design.
3. Per-class metrics with denominators (§8.5); template-only vs refined on the five design cases.
4. Who runs the issue query, and when relative to F2, is PI decision D8.

### 7.6 Generalization hypotheses (stated, not claimed)

- **H-V (cFS versions).** Only Φ is version-specific; rules carry version guards. Test: with Φ_6.4.x and unchanged engines, C3 at 6.4.1 should classify as fixed-order under σ_R1 and schedule-dependent under σ_U, and C5 at 6.4.1 as schedule-dependent ([R06 §4 T5]; needs D13). If an engine change is needed, H-V is refuted in its strong form.
- **H-C (configurations).** Verdicts change only through B. Tested by §7.4.
- **H-F (other flight frameworks).** A profile for another framework (e.g. F Prime) would need actor classes, kinds, operations with bindings and LPs, rules, contracts and configuration readers. F Prime's mechanisms were **not examined** in R01–R07, so nothing is claimed. The test, after the cFS evaluation, is to write such a profile without engine changes.

### 7.7 Openness indicators reported

Following [R06 §5.7]:
- contract kinds covered vs kinds found in the census;
- the share of instances discovered automatically (sources 1–5, 9–11) vs authored;
- the share of verdicts that needed a case-specific refinement;
- held-out results kept apart from design-case results;
- instances per variant B′, kept apart from the default B.

---

## 8. Evaluation plan

### 8.1 Research questions

- **RQ1 Openness.** How many contract instances exist in cFE core and the 15 apps under the default B and its variants, per kind and per RC? What share is decided by certificate, observation, prediction, exploration and confirmation? What share stays unknown, and why?
- **RQ2 Development cases.** With the template-only profile, what verdict records does Kairos produce, and which change when refinements are added? The cases:
  - R2 at v7.0.1;
  - C1 in a derived configuration;
  - C2 role-derived;
  - C3 and C5 at 6.4.1 (if D13 is approved);
  - the two App-record windows (C5-shaped residuals at v7.0.1, derived, not C5 itself).
- **RQ3 Held-out generalization.** On the frozen tool (F2), what does Kairos find in sources (a)–(e), with what triaged precision, confirmed share and unknown share?
- **RQ4 Model fidelity.** Conformance rate (O2 on every trace, O8 on the sample); obligation failures per iteration; certificate-checker pass rate and mutation kill rate; T-OBS disablement counts and `DECLARED_TRUSTED` counts; anchor-audit coverage; closure resolution per rule; havoc and local-choice counts ([J1-14]).
- **RQ5 Measured phenomena** ([A §8.1]; [J2-12]). Does Kairos classify the order-dependent losses measured in the default bundle: queue-full drops 4–49 per boot; `NoSubscribersCounter` 15–26; overflows of `SBNSubPipe`, `TO_LAB_TLM_PIPE` and `DS_CMD_PIPE` ([BM §0, §3.3])? Does it predict which pipes can overflow? Oracle: SB counters and per-pipe peaks. Label: LOSS-UNDER-ORDER, harm unspecified.
- **RQ6 Value of each layer and of the loop** (ablations, §8.4).
- **RQ7 Cost and perturbation.** The five costs, and the shift of order-sensitive distributions (§8.6).
- **RQ8 Comparison with established ordering and observation models** applied to the same traces (§8.3).

### 8.2 Subjects

- **Development set** (labels per [LED], [R06 S10]): R2 (current code; previously observed with gdb and with the retired recorder; 24 uses per boot, ES 20, EVS 4); C1 derived (public-code-derived); C2 role-derived (a constructed app pair, labelled synthetic control); C3 and C5 at the 6.4.1 tarballs on the shared derived 6.4.x platform ([LED] X3; D13). C3 and R2 share a resource family and are not counted as independent ([LED §3.1]).
- **Census subject**: the v7.0.1 bundle under the default B, plus the variants of §7.4.
- **Held-out**: §7.5 (a)–(e), each counted separately.

### 8.3 Baselines and reference detectors

Rules: original tool, extension and reimplementation are always distinguished; `NOT_RUN` is never a false negative ([R06 S5]); thread data-race tools are not baselines (the user's decision, [R06 S13]) and appear only as labelled reference points ([J3-M12]).

| Comparator | Kind | What it shows |
|---|---|---|
| FastTrack-style HB (PLDI 2009 [V-abs]) over the same traces (lifecycle LPs as writes, uses as reads, lock edges) | reimplementation, reference point | what an unordered-pair detector reports on the same evidence (expected: nothing for R2, [R04 §3 L4]) |
| WCP- and SHB-style relations (PLDI 2017, OOPSLA 2018 [V-abs]) over the same traces | reimplementation, reference point | lock-order-preserving prediction vs Kairos's lock-reversing search |
| DCatch-style framework HB without contracts, outcomes or conditional edges (ASPLOS 2017 [P]) | reimplementation | the value of contracts and outcome semantics on top of framework HB |
| RacePro-style syscall model (SOSP 2011 [V-abs]) over syscall tracepoints of the same runs (candidate classes as recorded in `baselines/availability.md`) | reimplementation of the observation model | process-level syscall observation on cFS (the records found zero syscalls for an OSAL name create plus lookup, [R06 §6.2 C5]) |
| Uncontrolled repeated boots with SB/EVS counters | reference observation | what counters reveal without order |
| gdb records ([R2g]) | reference observation | the reporter's method and its perturbation |
| ThreadSanitizer (gcc variant) | reference point, only if D7 approves the deviation | memory-level races; expected silence on lock-protected order violations ([R05 §6.3 (c)]) |
| Original process-level tools (RacePro, SimRacer, …) | `NOT_RUN` with recorded blockers ([R06 K6]) | — |
| Nidhugg, GenMC, CBMC | `NOT_RUN` (not installed; GenMC needs LLVM 19–22, Nidhugg 8–19, [R05 §2.2]) | — |

### 8.4 Ablations

One element is removed at a time, with the same engine and the same traces ([R06 S11]; [J1-14]; [J3-M8]):

1. **No-trace layer** (≈ A): static census + certificates + explorer only.
2. **No-static layer** (≈ B): trace HB + prediction + confirmation only; no census, no certificates.
3. **No feedback loop**: a single forward pass.
4. **No obligations** (O1–O8 off).
5. **σ_U∞ off**: no timeout-dependent refinement.
6. **Robustness tags off**: no VC_R prefilter.
7. **M_api only** vs **M_impl**.
8. **Collapsed identities** (one identity per event).
9. **Atomic APIs** (no LPs; whole API at entry).
10. **Conditional edges made unconditional** (soft barriers as must-edges).
11. **Implementation-level OSAL locks as synchronization**.
12. **No tier 1** (explore everything).
13. **No prediction**.
14. **Configuration closure off** (pointer analysis only).
15. **Looper restriction off** (thread program order instead of handler-scoped order).
16. **Contracts off** (pure HB race reporting).
17. **Writer sets from Φ only** vs **from mod-ref only** (the value of two oracles).
18. **L1, L2, L3 off**.
19. **DPOR vs naive enumeration** on small cones.
20. **Syscall-only observation; memory-only HB**.

Each ablation reports:
- what moved between classes;
- the decided share and the unknown share by reason;
- false candidates removed by O1–O8;
- the cost change.

For the loop ablation, it also reports keys and indirect targets resolved, unknowns removed and model gaps detected per iteration.

### 8.5 Metrics

- Instances per class, refinement, σ, M, evidence level, RC kind and source (§7.1), always with denominators ([R06 R3]). `NOT_RUN`, `NOT_MEASURED` and `NOT_FROZEN` are never zero or "pass" ([R06 R2]).
- Decided share per layer; unknown share by reason.
- Triaged precision: two independent raters classify every reported instance against the contract and the code; disagreements are logged; an agreement statistic, chosen and pre-registered with the PI, is reported with the triage protocol.
- Confirmation: confirmed, refuted and inconclusive shares; gate-unreached share; holds per confirmation.
- Static coverage ([R06 T3]): entry functions resolved; indirect call sites resolved per rule; concrete, symbolic and ⊤ keys.
- Fidelity: as RQ4.
- Cost: time and memory per stage; the five costs separately; profile size and authoring hours.

### 8.6 Overhead and perturbation protocol

Groups ([J3-M6]; [A §5.8]; [B §5.6]):

| Group | Configuration | Separates |
|---|---|---|
| A | unmodified binary, no observer, **re-measured on `-fc-v114`** | reference |
| B | uprobes registered with empty BPF programs | trap cost |
| C′ | minimal R2 plan (R2 sites and identity bootstrap only) | cost of a narrow plan |
| C | full plan | recording cost |
| D | C plus holds | control effect |
| W | the F1 `--wrap` build with gates disabled (only if D3 is approved) | code-layout effect of the variant |

**Measures per run**:
- time to the OPERATIONAL line;
- `PipeOverflowErrorCounter` (baseline 4–49) and `NoSubscribersCounter` (15–26), read from memory at stop ([BM §2.3]);
- which pipes overflowed (`SBNSubPipe`, `TO_LAB_TLM_PIPE`, `DS_CMD_PIPE`);
- the app init-completion order;
- the R2 use set (24 per boot: ES 20, EVS 4);
- probe hits per site and drops.

Distributions are compared, not means.

**Controls**:
- **cold vs warm state** is recorded per run ([BM §0]: the first run after a reboot reached OPERATIONAL at 2570 ms vs 1613–1633 ms);
- groups are run **interleaved**, not in blocks. The interleaving order is an `unspecified_by_reference` assumption, recorded with its reason: the host is shared and drifts ([R07 §1]);
- persistent state S0 is snapshotted per run and recorded as B.Init.

### 8.7 Statistics and reproducibility

- The number of runs per group, per loop iteration, and for σ_E frequencies is `unspecified_by_reference` and is pre-registered by the PI (D14). Klees et al.'s practice of about 30 trials is known only from a secondary summary ([R05 §6.5], status P) and is cited only as a literature norm.
- Analysis follows Arcuri and Briand (ICSE 2011 [V-abs]): full distributions; Mann–Whitney U for times and counts; Fisher's exact test for incidence; standardized effect sizes; exact binomial confidence intervals for proportions.
- σ_E frequencies are pooled only over runs with identical (B hash, plan hash).
- Every reported number can be regenerated from stored artifacts: the deterministic-replay test of §4.14.

### 8.8 Threats to validity

- **Internal**: profile errors (mitigated by certificates, the checker, O3/O4, O7); Clang/gcc differences (anchor audit); model fidelity (O1, O2, O6, O8); probe perturbation (§8.6); kernel drift and a shared host; persistent state across runs; `DECLARED_TRUSTED` guards.
- **External**: one platform (Linux `SCHED_OTHER`, 4 vCPU, permissive mode, `msg_max` 10); one OSAL backend (POSIX); one bundle version; RT behaviour only in the model (σ_R1); derived cases for C1, C2, C3 and C5.
- **Construct**: contracts are authored, so the oracle is only as good as its provenance (shown per verdict); harm is judged only from documented allowed outcomes; route and capacity losses are not violations without a must-deliver reference (LOSS-UNDER-ORDER).
- **Conclusion**: few and partly dependent development cases; possible held-out scarcity; N set by the PI and reported.

---

## 9. Implementation roadmap

### 9.1 Directory layout (to be created later; nothing created now)

```
kairos/
  CMakeLists.txt                     # find_package(LLVM, Clang) from the pinned LLVM 23 build 1053047a; MLIR only after G-MLIR
  include/kz/schema/kz_schema.h      # the only record-layout definition (C, _Static_assert); shared by BPF, gate objects, C++
  include/kz/core/*.hpp              # KEM types and component interfaces (pure functions over typed artifacts)
  profiles/cfs-v7.0.1/*.kzp          # Φ: kinds, ops, effects+LPs, rules, contracts, constants, version guards, provenance
  profiles/freeze/                   # F1/F2 freeze manifests (hashes)
  lib/core/      container I/O, Φ parser/interpreter, rule engine, typestate, vector clocks, verdict lattice, provenance
  lib/llvm/      basis, extract (Clang), link (IR, DWARF, MC binder, anchor audit), modref, skel        # libkz-llvm
  lib/static/    MustBefore, certificates, census, plan
  lib/proofcheck/ independent certificate checker (no code shared with lib/static) + mutation engine
  lib/trace/     decode, identity, HB (VC, VC_R), LP intervals, contract check, routing, chains, mining
  lib/conform/   per-actor acceptance (O1/O2), global conformance (O8)
  lib/predict/   cone, explicit search, prefilters (SHB/WCP/SyncP-style), extrapolation, SMT-LIB2 emitter (optional)
  lib/explore/   model builder, source-DPOR + sleep sets, σ variants, monitor, deadlock check, BPOR
  lib/confirm/   hold plans, ptrace + debug-register holder, wait-for oracle, F1 controller, PCT
  probe/bpf/     kz_probe.bpf.c (one generic program per phase; system clang-18 -target bpf)
  probe/loader/  raw bpf()/perf_event_open loader and drainer; tracefs backend
  gate/          kz_wrap.c (C99, cFS flags; F1 only, after D3)
  tools/kz/      one CLI: kz basis|extract|link|modref|skel|static|proofcheck|plan|observe|trace|conform|
                          predict|explore|hold|gate|loop|report|dump
  tests/unit  tests/golden (canonical text dumps)  tests/litmus (gcc-built C99 programs on the real OSAL,
              expected classes per σ and M)  tests/mutation  tests/property  tests/cfs (integration)
  docs/          formal model, Φ reference, provenance rules, ADRs (§11), diagrams (§12)
```

### 9.2 Engineering discipline (how messy code is avoided)

- **Interfaces first.** `kz_schema.h`, the Φ grammar, the verdict record and the component interfaces are reviewed before any implementation (stage S0).
- **One library per component plus a thin CLI subcommand**; components communicate only through KZ containers.
- **No generated code** ([R06 K8]): no schema compiler; no TableGen/ODS unless G-MLIR passes; a generic, data-driven BPF program; handwritten gate wrappers for the small synchronization API set; the explorer **interprets** the model (no harness C code is generated).
- **No case-specific code; no bare constants** (the lints of §7.2).
- **Tests at five levels**:
  - unit tests;
  - golden tests with canonical text dumps (every Φ rule has one: a tiny program or synthetic trace with expected edges and verdicts);
  - a **litmus suite** of small gcc-built C99 programs on the real OSAL, with expected classes per σ and M ([A §9.3]);
  - **property tests**: HB transitivity; the containment lemma on small random models; loop monotonicity; DPOR vs naive enumeration;
  - **mutation tests** for certificates and containers.
- LLVM conventions for code linked with LLVM (no exceptions; the build has `LLVM_ENABLE_EH=OFF`, [R07 §2.1]).
- **Provenance in every artifact**: B, Φ, tool and parent hashes; LLVM `1053047a`; gcc 13.3.0; kernel version; backend; fidelity class.
- **Deterministic outputs**: sorted and hashed, so golden tests and replay are stable.

### 9.3 Stages and exit gates

Each stage is publishable on its own. The recorder stays narrow and model-driven, respecting v0.1 §15 ([J3-M5]; [J1-11]; [J2-15]). **The minimum viable core (MVC) is S0–S3.**

| Stage | Deliverable | Exit gate (all items required) | Needs |
|---|---|---|---|
| **S0** | Formats and formal core: `kz_schema.h`, KZ container, Φ grammar and parser, verdict record, certificate format, lints, golden-test harness; Φ_cFS skeleton for K-ID, K-SPAWN, K-REG with provenance | containers round-trip with stable hashes; golden tests for the rule catalogue of §3.16 pass on synthetic traces; lints active; no cFS involved | — |
| **S1** | F0 observer: tracefs backend first (bring-up), then the raw `bpf()` loader with entry-only lock tokens; identity join; calibration table | **R2 re-observed by Kairos's own recorder on the unmodified binary**: 24 violating uses per boot (ES 20, EVS 4), the record value to match, not a result to assume; loss zero or accounted; uretprobe, post-call `mov`, `leave` and BPF-body costs measured; `.so` pre-registration tested; group A re-measured on `-fc-v114` with cold/warm recorded; groups A, B, C′ compared | D1 (root-side hook); D2 only if the own loader fails |
| **S2** | Trace HB and dynamic contract checking: VC/VC_R, LP intervals, per-use states, routing, outcome agreement, chains, outcome mining | every R2 use classified; every later use ordered, **including** those ordered only by the system-state wait (the gap of [R2c §4.3]); O3–O5 pass on default traces; the 17 `SEND_NO_SUBS` uses chained to K-ROUTE | S1 |
| **S3** | Static closure, mod-ref, skeletons, MustBefore, census (K-ID, K-SPAWN, K-REG first, then all templates), certificates, `kz proofcheck` with mutation tests, Clang→gcc anchor audit, O1/O2 | **R2 FIXED_ORDER certificate for the ES and EVS uses accepted by `kz proofcheck`**, including the early-return regression; mutation kill rate reported; timebase lookups protected by certificate; CF lookup inactive in the default B; the core-task window and the SB/TBL ABSENT consequence present in the census as open (or classified); O7 report with disablement counts | S0; S1 for REACH and O1/O2 |
| **S4** | Feedback loop with O1–O8 and reproducibility | fixed point on the default B with per-iteration deltas and its run set; deterministic replay test passes | S1–S3 |
| **S5** | Trace prediction (explicit, lock-reversing; prefilters; extrapolation labels) | synthetic suites pass; a C1-derived trace yields a reversal witness; both App-record windows yield witnesses from default boots or are reported `unknown(budget)` with the budget curve | S2 |
| **S6** | F0 confirmation (`kz hold`) | the App-record window probe executed with event release, reference watchdog and recorded perturbation, result confirmed, refuted or inconclusive, labelled derived; holder cost and ptrace×uprobe interaction measured | S5; D16 |
| **S7** | Explorer (gated) | L-DPOR differential test passes; two-query classification on the development cones; agreement with prediction (O6); kill criterion of §4.10 applied and recorded | S3 |
| **S8** | G-MLIR gate (optional) | decision recorded with measurements on the stated criteria | D6 |
| **S9** | Freeze F1, then F2; evaluation | template-only runs recorded before refinements; §8 executed; held-out sources opened only after F2 | D8, D14 |
| **S10** (PI-gated arms) | F1 `--wrap` gates and PCT; native σ_R1 arm; 6.4.x basis for H-V | as approved | D3, D4, D13 |

### 9.4 What is built first

S0, because every later component depends on the shared formal core and the formats. Then S1, because ground truth must come from the new recorder before any analysis is trusted. S2 and S3 follow, and they can overlap: S3's static chain needs only S0, plus S1's traces for REACH and O1/O2. The publishable core, the R2 fixed-order certificate plus the census plus the recorder, therefore does not wait for prediction, confirmation or the explorer.

---

## 10. Risks and open decisions for the PI

### 10.1 Decisions requested

| Id | Decision | Options | Recommendation |
|---|---|---|---|
| D1 | Root-side observer and holder attached to the README-mode process; new `run_cfs.sh` hook ([R07 §9 item 1]) | approve; or not (then only F1/F2 mechanisms remain) | approve: the highest fidelity available |
| D2 | eBPF userland | own raw loader (no install); or install libbpf/bpftool (recorded package decision) | own loader; install only if S1 shows it is infeasible |
| D3 | F1 `--wrap` gate variant (link-option deviation) and group W | approve for PCT and many-point schedules; or F0 holds only | defer until S6 shows whether F0 holds suffice |
| D4 | σ set in the paper; native σ_R1 arm | σ_U, σ_U∞, σ_E required; σ_R1 model-only, or also native with root-imposed `SCHED_FIFO` on one CPU (deviation from E1/E4) | σ_R1 model-only |
| D5 | SMT solver | Z3 4.8.12 from apt for cross-checks; or explicit search only | explicit search only |
| D6 | MLIR | run G-MLIR at S8; or defer | defer to S8 |
| D7 | ThreadSanitizer reference point | gcc variant build as a labelled deviation; or omit | omit unless a reviewer asks |
| D8 | Held-out sources (a)–(e); who runs the issue query and when relative to F2 | per §7.5 | all five, counted separately |
| D9 | Scope: RC8, RC9, RC10, RC12 as process-level; reporting class M; route and capacity in the first paper ([R06 §8 Q6]) | include with labels; or exclude | include, with LOSS-UNDER-ORDER and class M separate |
| D10 | Memory-model stance for ad hoc synchronization | assumed visibility (A4), flagged per verdict; or report the sites | assumed visibility, flagged |
| D11 | Re-baselining after the kernel change | re-measure group A on `-fc-v114` before any comparison | required by S1 |
| D12 | Citing the retired recorder's observations | as prior observations; or only after Kairos re-observes them | prior observations only; S1 re-observes |
| D13 | The 6.4.x derived platform for C3/C5 and H-V ([LED] X3) | build it; or code-reading evidence only | decide before S9 |
| D14 | N: runs per group, per loop iteration, and for σ_E frequencies | `unspecified_by_reference`; PI sets and pre-registers it | pre-register before S9 |
| D15 | Maximum analysis budgets (prediction, exploration, conformance) used by sweeps and by the explorer kill criterion | resource caps; `unspecified_by_reference` | set before S5/S7 |
| D16 | The derived App-record probes on both creation paths ([LED] X5) | pursue, labelled derived; or exclude | pursue (S6) |
| D17 | Stop method for experiments (the records' D3/E11) | per records | decide before S1 |
| D18 | Which configuration and input variants B′ to run | per §7.4 | tables activating latent paths + `B_restart` first |
| D19 | `msg_max` variant (the records' D2) | keep 10; or a recorded variant | keep 10 for the default B; a variant B′ only for RQ5 sensitivity |

### 10.2 Risks

| Risk | Likelihood / impact [I] | Mitigation | Retired by |
|---|---|---|---|
| Mod-ref too imprecise: many conjuncts DISABLED or W too large, so few certificates | medium / high | Φ-declared effects with keys and guards; transition-aware filtering; `DECLARED_TRUSTED` entries listed; disablement counts reported; certificates are still sound | S3 O7 report |
| A `DECLARED_TRUSTED` guard is wrong | low / high | listed in every certificate; refuted by O3/O4; ablation 17 | S4 |
| Skeleton infidelity (havoc and star closure too coarse) | medium / high | O1, O2 on every trace; O8 sample; havoc counts; `unknown` on doubt | S4 |
| Observation perturbation (return probes in the 31.7 µs class) | medium / medium | entry-only lock/queue tokens; return capture only at contract-use sites; calibration first; groups B, C′, C | S1 |
| eBPF features not probed (ring buffer, task storage, stack capture) | medium / medium | perf-sample fallback; tracefs backend; D2 | S1 |
| `.so` pre-registration not working | medium / medium | hold at `OS_ModuleLoad` return (recorded) | S1 |
| Predictor and explorer state explosion | high / medium | cones, L1–L3, key-level dependence, trace-anchored windows; budget sweeps; kill criterion; fallback to prediction plus confirmation | S5, S7 |
| L-DPOR differential test fails | low / medium | the explorer stays an ablation (kill criterion) | S7 |
| Holds perturb other threads (binsem 2 s timedlock, timer callbacks, signals) | low / medium | gate-placement rules; watchdogs; `timeout-exercised` label; interaction measured | S6 |
| Profile authoring effort and errors | medium / medium | per-API templates; header extraction; loop- and symmetry-derived templates; effort measured | S3, S9 |
| Integration breadth (about 18 subcommands, three interpreters) | medium / high | staged gates; MVC S0–S3 publishable alone; one formal core | S3 |
| Route/capacity verdicts read as "bugs" | medium / reputational | LOSS-UNDER-ORDER; harm unspecified without a must-deliver reference | report review |
| Held-out scarcity | medium / medium | five source types counted separately; seeded mutations labelled synthetic | S9 |
| 6.4.1 bases lack a compile DB | high / medium (H-V, RQ2 for C3/C5) | reconstruct from the 6.4.x make system, or code-reading evidence only (D13) | S10 |
| Kernel drift and host sharing | medium / medium | re-baseline; interleaved groups; cold/warm recorded | S1 |

---

## 11. Architecture decision records

Format: **Context** (why a decision was needed), **Decision**, **Alternatives** considered, **Consequences** (including costs and what must be watched). Status of every ADR: *accepted for SPEC v1*. Those that depend on a PI decision say so.

**ADR-001 — C/C++ core and typed artifacts; no Python/JSON pipeline.**
- Context: the user discarded the Python/JSON prototype (CORD, [R06 K7]) and asked for a principled core.
- Decision: all components in C++17 (C for BPF and gate objects); all artifacts in the typed KZ container; JSON appears only as CMake's input `compile_commands.json`.
- Alternatives: Python orchestration over C++ kernels; a schema compiler (FlatBuffers/Cap'n Proto).
- Consequences: one toolchain (pinned LLVM 23 + gcc 13.3 + system clang-18 for BPF); review via `kz dump`; no generated code.

**ADR-002 — One formal core interpreted three ways.**
- Context: static, trace and model layers must agree on events, resources, edges and verdicts ([J1], [J2], [J3] all chose C's spine).
- Decision: one KEM type set and one profile Φ, interpreted abstractly (skeletons), concretely (traces) and operationally (explorer), with a concretization relation `t ⊒ e`.
- Alternatives: separate models per layer (A's MLIR model + B's trace IR).
- Consequences: obligations O1–O8 become possible. `libkz-core` must stay LLVM-free.

**ADR-003 — Φ is an interpreted DSL with provenance.**
- Context: framework knowledge must be versioned, cited and reusable across revisions (H-V); no messy code generation ([R06 K8]).
- Decision: a hand-parsed declarative language for kinds, operations with effects and LPs, rules, contracts and constants, with provenance, authoring records and version guards. It is never compiled to C++.
- Alternatives: C++ rule classes; MLIR ODS; RCL-only contracts with hard-coded semantics (A).
- Consequences: engines are framework-neutral (case-identifier lint). A Φ edit restarts the loop (new hash).

**ADR-004 — KZ container with one C schema header and Merkle parent hashes.**
- Context: BPF, gate objects and C++ must share record layouts exactly; verdicts must trace to exact inputs.
- Decision: fixed-layout little-endian records declared once in `kz_schema.h` with `_Static_assert`s; parent hashes in every header ([A §2.3]; [C §2.3]).
- Alternatives: per-component formats (B); JSON lines.
- Consequences: an ABI version bump on layout change; round-trip tests.

**ADR-005 — Static ordering is a MustBefore greatest fixpoint.**
- Context: C's "vector clocks over templates" is a may-order ([J1] fatal flaw 1; [J2-2]).
- Decision: T-PO with meets over predecessors, terminal ops without successors (T-TERM), T-SPAWN as a meet over all spawn sites, T-OBS only with complete transition-aware writer sets and a falsifier, no lock rule (§4.4).
- Alternatives: may-HB over templates (C); SIERRA-style static action HB alone.
- Consequences: sound but incomplete; precision depends on ADR-007. Complexity `O(N·R·(d̄+w̄)·R/64)`.

**ADR-006 — Certificates by precedes-whenever-both-occur, with an independent checker.**
- Context: dominance-based FIXED_ORDER fails on R2's early returns ([J2-1]; [J3-M1]).
- Decision: a frontier `m` with no path `m →* u` and `m ∈ MB(x)` for every writer (L-PWB, L-FO); PROTECTED by a must-writer plus PWB-ordered disablers (L-PR); every certificate re-validated by `kz proofcheck` (no shared code) with mutation tests.
- Alternatives: A's `u ∈ MustBefore(x)`; B's trace-local VC_R certificates.
- Consequences: R2 is certifiable at b = ∞; the checker and the mutation engine are extra code; the mutation kill rate is a metric.

**ADR-007 — Writer sets from mod-ref plus Φ effects, with an agreement obligation.**
- Context: T-OBS needs complete writer sets including the initial value ([J1 X4]); field-based mod-ref cannot separate records, while Φ can be wrong.
- Decision: `kz modref` (Andersen-style, field-based, memset-aware, out-pointer summaries, transition-aware filter) establishes completeness. Φ effects supply keys and guards. O7 checks agreement, and statuses `COMPUTED` / `DECLARED_VERIFIED` / `DECLARED_TRUSTED` / `DISABLED` are recorded per entry.
- Alternatives: declared effects only (C); computed only, via an IR interpreter (A).
- Consequences: trusted guards are visible in certificates; disablement counts are reported; ablation 17 measures the value of two oracles.

**ADR-008 — Semantics family σ_U, σ_U∞, σ_R1, σ_E with containment and transfer.**
- Context: σ_U alone floods the app level with schedule-dependent verdicts ([J1 X3]; [R06 T5]).
- Decision: four semantics, weak fairness and `stall` for σ_U∞, the timeout-dependent refinement, the containment lemma under A7, and the transfer rules of §1.8.
- Alternatives: σ_U + σ_E only (A); σ_ENV as σ_U plus frequencies (B).
- Consequences: two explorer runs per instance where needed; verdicts carry σ; a native σ_R1 arm only after D4.

**ADR-009 — A linearization point per effect.**
- Context: where a non-atomic API reads or writes the registry decides the windows ([J1 X1]).
- Decision: every Φ effect declares its LP; Ready is evaluated at the LP; traces use LP intervals; outcome codes must agree (O4).
- Alternatives: evaluate at API entry or return.
- Consequences: Φ authoring cost per API; exact LP probes become unnecessary (LP intervals plus outcome-implied reads-from).

**ADR-010 — Two OSAL model levels.**
- Context: OSAL has several backends; POSIX internals create windows and a lock-leak path ([R01 §7 item 6]).
- Decision: M_api (atomic at the LP) by default; M_impl (OSAL shared+POSIX source projected onto Φ_L-1) for idmap, task, binsem and queue paths. Shapes use API-level edges only. Labels portability-hazard and implementation-defect-candidate.
- Alternatives: API level only (C, B); always M_impl.
- Consequences: two analyses for OSAL-touching cones; litmus tests on the real OSAL validate Φ_L-1.

**ADR-011 — A verdict record with orthogonal axes and no "safe".**
- Context: the reporting requirements R1–R7 and S1 of [R06]; [J1-9].
- Decision: class (+ refinement), σ, M, b, hashes, evidence, shape, outcome label, consequence, harm and chain are kept separate; classes include `no-provider` and `capability-absent`; `unknown` always has a reason.
- Alternatives: a single severity ladder.
- Consequences: reports are multi-axis; `kz report` renders views; tests check that no path ever produces "safe".

**ADR-012 — Context-qualified templates.**
- Context: SB event sites are shared by all callers; R2's uses are SB code in ES and EVS context ([R2g §2]).
- Decision: templates are `(actor instance, inlining call string, site)`; recursion → `unknown`.
- Alternatives: site-only templates.
- Consequences: larger skeletons; per-context certificates (necessary for frontier arguments).

**ADR-013 — F0 observation with an own raw loader; tracefs fallback.**
- Context: highest fidelity on the unmodified gcc binary; no libbpf ([R07 §4.7]).
- Decision: uprobes at entry+4 with one generic data-driven BPF program per phase, loaded by raw `bpf()`/`perf_event_open`; tracefs dynamic events for zero-install bring-up.
- Alternatives: `--wrap` (F1); source probes (F2); gdb; install libbpf (D2).
- Consequences: an own loader to write; ring buffers, task storage and stack capture to be probed in S1, with the perf-sample path as fallback.

**ADR-014 — Causality only from tokens and protocol keys.**
- Context: timestamps give no HB ([R04 §2.1]; [J2-4]).
- Decision: per-thread `tseq`; per-object `objseq` at in-critical-section entry probes (give entry); descriptor/FIFO, outcome-implied reads-from, poll exit status. Clock-ordered lock edges in the tracefs backend are evidence only.
- Alternatives: B's A-CLK reconstruction as HB.
- Consequences: no take-return probes on hot sites; weaker verdicts in tracefs mode.

**ADR-015 — Return values captured only where needed, by a mechanism chosen by measurement.**
- Context: post-call `mov` probes are likely in the 31.7 µs class; uretprobe cost is unknown ([J3 §2.3]).
- Decision: return capture only at contract-use and edge-deciding sites; R-a…R-d measured per class in S1; a cost class per probe.
- Alternatives: caller-side return probes everywhere (A, C).
- Consequences: S1 must produce the calibration table before a full plan is fixed.

**ADR-016 — Prediction by explicit-state, lock-reversing search.**
- Context: the central cFS class needs lock reversal (C1, C5, App-record windows), outside SyncP's class; no solver is installed ([J1-7]; [J2-3]; [J3-M2]).
- Decision: Pred(π) with the key as conflict object and the lock as atomicity, sleep sets and memoization. SHB/WCP/SyncP are prefilters with re-validation. Extrapolation is labelled model-based. SMT-LIB is an optional cross-check after D5.
- Alternatives: B's P1–P3 with SMT; SMT-only.
- Consequences: budget sweeps and `unknown(budget)` counts are reported; no install needed.

**ADR-017 — Explorer: source-DPOR with explicit local choice and key-level dependence; gated.**
- Context: DPOR exactness requires deterministic threads ([J1 X6]); the unsliced ES and SB locks explode ([J3-M3]).
- Decision: explicit choice transitions; dependence including enabling/disabling and await-predicate pairs; L1–L3 switchable; BPOR when bounded; differential test against naive enumeration; stage S7 with a kill criterion.
- Alternatives: optimal DPOR claimed exact (A, C); no explorer (B).
- Consequences: the explorer may end as an ablation only; prediction plus confirmation carry the schedule-dependent classes in that case.

**ADR-018 — MLIR behind gate G-MLIR.**
- Context: the facts are not a program to be lowered; ODS/TableGen conflicts with K8 ([R06 T1]).
- Decision: plain C++ skeletons and core; G-MLIR at S8 with criteria (verdict equality, provenance completeness, code size, runtime); no ODS before the gate passes; never checked-in generated code.
- Alternatives: MLIR-first (v0.1, A).
- Consequences: no claim that MLIR helps unless the gate measures a win.

**ADR-019 — Version-robust site binding.**
- Context: line-number probes break across revisions ([J2-9]).
- Decision: AST patterns resolved through each basis's gcc DWARF and MC; version guards on Φ entries; anchor audit.
- Alternatives: line-number probes (B).
- Consequences: a pattern language in Φ; unresolved patterns listed.

**ADR-020 — Confirmation by F0 holds with explicit safety rules.**
- Context: constructed schedules must be feasible, safe and honestly labelled ([J2-10]; [J3-M10]).
- Decision: ptrace with debug-register execution breakpoints; empty program-lock set; no holds in `handler_mutex` callbacks, binsem internals, signal handlers or (except declared experiments) RESERVED windows; event release; the smallest reference timeout as watchdog; a wait-for oracle for deadlocks; labels derived / timeout-exercised. F1 gates and PCT only after D3.
- Alternatives: `--wrap` gates by default (C, A); sleeps of chosen length (rejected by [POL]).
- Consequences: at most 4 gate points per thread; controller cost to be measured.

**ADR-021 — Consistency obligations O1–O8 and the certificate check.**
- Context: authored rules need a "how do you know the model is right" answer ([J1 §6]).
- Decision: per-actor acceptance on every trace; global conformance on a budgeted sample; outcome agreement; writer-set agreement; contradictions suspend rules and produce model-fault records.
- Alternatives: conformance only (A); confirmation-only refutation (B).
- Consequences: obligation failures are a reported fidelity metric.

**ADR-022 — A feedback loop with reproducibility.**
- Context: the fixed point depends on the sampled runs; plans change between iterations ([J3-M7]).
- Decision: per-iteration records (run IDs, B/plan/Φ/tool hashes, S0); fixed point reported with its run set; σ_E pooling only over identical (B, plan); deterministic replay test.
- Alternatives: forward pipeline (v0.1); an unrecorded loop (C).
- Consequences: more bookkeeping; reproducible numbers.

**ADR-023 — Clang/LLVM for analysis only; the gcc binary is what runs.**
- Context: the reference build is gcc 13.3 ([R06 K1]); a Clang-built executable would be fidelity F3.
- Decision: Clang facts plus IR for analysis; DWARF/MC anchoring to the gcc binary; an anchor audit.
- Alternatives: a Clang-built executable with Clang instrumentation.
- Consequences: two diagnostic-only flag adaptations recorded in B; an audit stage.

**ADR-024 — Openness through a union of discovery sources, with governance.**
- Context: five cases cover 3 of 15 contract kinds ([R06 §5.1]).
- Decision: the 13 sources of §7.1; freezes F1/F2; template-only first; provenance and authoring records; held-out (a)–(e).
- Alternatives: case-derived rules (v0.1).
- Consequences: authoring effort is real and is measured.

**ADR-025 — Baselines reimplemented over the same traces.**
- Context: no external process-level tool runs here ([R06 K6]); thread data-race tools are excluded as baselines ([R06 S13]).
- Decision: FastTrack/WCP/SHB/DCatch-style/RacePro-style reimplementations over the same traces, always labelled; external tools `NOT_RUN`, never false negatives.
- Alternatives: no comparison; running TSan as a baseline.
- Consequences: comparisons answer "what does each ordering model see on the same evidence", not tool superiority.

**ADR-026 — A staged build with a minimum viable core.**
- Context: scope risk of three interpreters ([J1-11]; [J2-15]; [J3-M5]).
- Decision: stages S0–S10 with exit gates; MVC = S0–S3 (formats, recorder with R2 re-observed, trace HB, census and the R2 certificate).
- Alternatives: model-first (A, recorder at M5); trace-first only (B).
- Consequences: the explorer and F1 can be cut without losing the core contribution.

---

## 12. Diagrams to draw

Each diagram lists the exact nodes and edges it must show. Edge labels are given after a colon. Dashed edges are PI-gated or feedback paths.

### D1 — System context

- **Nodes**:
  - PI/Researcher;
  - `kz` driver (root session);
  - pinned cFS tree (read-only);
  - `build-native_std` (compile DB, `link.txt`, generated files, `.tbl`, gcc `core-cpu1` and app `.so` with DWARF);
  - LLVM 23 build `1053047a` (Clang/LLVM libraries);
  - system clang-18 (BPF backend);
  - Linux kernel `6.18.44-fc-v114` (uprobe PMU, `bpf()`, perf breakpoint PMU, ptrace, tracefs, sched/syscall tracepoints);
  - run harness `env/run_cfs.sh` with the root-side observer hook (D1);
  - cFS process `core-cpu1` (README mode, user `ubuntu`, 27 threads);
  - persistent state (`/dev/shm/osal:RAM`, `EEPROM.DAT`, SysV segments, key files);
  - research records (`CONDITIONS_POLICY.md`, ledger, results, `BASELINE_MEASURE.md`);
  - KZ artifact store;
  - dashed PI-gated nodes: Z3 (D5), libbpf (D2), F1 `--wrap` variant build (D3), MLIR (D6).
- **Edges**:
  - PI → `kz`: Φ edits, PI decisions D1–D19, freezes F1/F2;
  - cFS tree → `kz`: sources (read-only);
  - `build-native_std` → `kz`: compile DB, `link.txt`, generated files, `.tbl`, binaries + DWARF;
  - LLVM 23 → `kz`: linked libraries;
  - clang-18 → `kz`: BPF object;
  - harness → cFS process: launch as `ubuntu`, stop method (D17);
  - `kz` → kernel: perf_event_open (uprobes, breakpoints), `bpf()`, ptrace/debug registers, tracefs;
  - kernel → cFS process: traps, holds;
  - kernel → `kz`: ring buffers / trace events;
  - cFS process ↔ persistent state: read at boot, written at stop;
  - `kz` → persistent state: S0 snapshot (read-only);
  - research records → `kz`: reference values, policy;
  - `kz` → artifact store; artifact store → PI: reports, claims table;
  - dashed: Z3 → `kz` (SMT-LIB cross-check); libbpf → `kz` (loader); `--wrap` build → cFS process (F1 runs); MLIR → `kz` (G-MLIR arm).

### D2 — Components and data flow

- **Nodes (components)**: `kz basis`, `kz extract`, `kz link`, `kz modref`, `kz skel`, `kz static`, `kz proofcheck`, `kz plan`, `kz observe`, `kz trace`, `kz conform`, `kz predict`, `kz explore` (stage S7, dashed border), `kz hold`, `kz gate` (dashed, D3), `kz loop`, `kz report`, `libkz-core` (shared core).
- **Nodes (artifacts)**: `basis.kz`, `profile.kzp/.kz`, `facts.kz`, `anchors.kz`, `modref.kz`, `skel.kz`, `census.kz`, `plan.kz`, `run-*.kzt`, `hb-*.kz`, `witness-*.kz`, `holdplan-*.kz`, `verdicts.kz`, `loop-*.kz`.
- **Edges**:
  - basis → `basis.kz` → extract, link, modref, skel;
  - extract → `facts.kz`; link → `anchors.kz`;
  - facts + anchors + profile → modref → `modref.kz` (annotate O7);
  - facts + modref + profile → skel → `skel.kz`;
  - skel + modref + profile → static → `census.kz`;
  - census → proofcheck: certificates; proofcheck → census: accept/reject (annotate CC);
  - census + anchors → plan → `plan.kz`;
  - plan → observe → `run-*.kzt`;
  - run → trace → `hb-*.kz` (annotate O3–O5);
  - run + skel → conform (annotate O1, O2, O8);
  - hb → predict → witness; static + hb → explore → witness (annotate O6);
  - witness → `holdplan` → hold (or gate) → `run-*.kzt`;
  - static, trace, predict, explore → `verdicts.kz` → report;
  - loop orchestrates all;
  - dashed feedback: F1 trace → skel/link (keys, targets, `dlsym` results); F2 conform → skel (model gaps); F3 confirmation runs → trace → census; F4 obligation failures → profile (human review);
  - `libkz-core` linked by every component except BPF.

### D3 — Analysis workflow (per contract instance)

- **Nodes (decision flow)**:
  - "template matched at context-qualified site";
  - "key resolved?" (concrete/symbolic/⊤);
  - "reachable under B?";
  - "writer sets complete?" (DISABLED?);
  - "INIT_FALSE ∧ W = ∅?";
  - "env guard false?";
  - "FIXED_ORDER certificate?";
  - "PROTECTED certificate?";
  - "kz proofcheck";
  - "open: probe obligations";
  - "observation";
  - "routing" (conforming / wrong-way / O-ordered / unordered);
  - "prediction" (prefilter → explicit search);
  - "explorer" (if S7 passed; σ_U, σ_U∞, σ_R1);
  - "confirmation";
  - "combine evidence (§4.12)";
  - "obligation failure".
- **Terminal nodes**: `inactive`, `no-provider`, `capability-absent`, `fixed-order`, `protected`/`tolerated`, `schedule-dependent`/`timeout-dependent`, `unknown(reason)`.
- **Edges**:
  - not reachable → inactive;
  - ⊤ → unknown(key_unresolved) + probe obligations;
  - DISABLED → unknown(writer_incomplete);
  - INIT_FALSE ∧ W = ∅ → no-provider; env guard false → capability-absent;
  - certificate → proofcheck → fixed-order / protected (reject → open);
  - open → probe → observation → routing;
  - wrong-way → certificate request (back to "FIXED_ORDER certificate?");
  - O-ordered or unordered → prediction;
  - prediction witness → confirmation; no witness → unknown(budget) or explorer;
  - explorer → two queries → class within b; σ_U∞ → timeout-dependent refinement;
  - any obligation failure → unknown(model_gap / model_fault / conformance);
  - all classes → verdict record.

### D4 — Research workflow

- **Nodes**:
  - Φ authoring (provenance, authoring records);
  - S0 formats → S1 recorder (R2 re-observed; calibration; group A re-measured) → S2 trace HB → S3 census + certificates (MVC milestone) → S4 loop → S5 prediction → S6 confirmation → S7 explorer (kill criterion) → S8 G-MLIR;
  - freeze F1;
  - template-only development runs;
  - labelled refinements;
  - freeze F2;
  - held-out sources (a)–(e);
  - ablations (20);
  - reimplemented baselines;
  - perturbation groups A, B, C′, C, D, W (interleaved; cold/warm);
  - statistics (pre-registered N, D14);
  - claims table (made / not made);
  - PI decisions D1–D19;
  - deviation records ([POL]).
- **Edges**:
  - the stage sequence;
  - S3 → F1 → template-only → refinements → F2 → held-out;
  - S1 → perturbation groups;
  - stages → ablations and baselines → statistics → claims table;
  - PI decisions → the stages they gate (D1 → S1; D3 → W, S10; D13 → H-V; D15 → S5/S7; D16 → S6);
  - every deviation → deviation record.

### D5 — cFS layer mapping

- **Left column (cFS)**:
  - **PSP**: main thread, `soft_timebase` / `cFS-Master`, reset decision, persistence areas, `CFE_PSP_Panic` (terminal), `CFE_PSP_Restart` (non-terminal), exception ring;
  - **OSAL**: idmap (ALLOC/RESERVED/LIVE, type locks), tasks (create/prepare/delete), mutex, binsem, countsem, queues (`msg_max`), timebase/timers, modules/symbols, files;
  - **cFE ES**: startup thread, `CreateObjects` (EarlyInit; core loop: reserve `:803`, start `:810`, publish `:818`, sync `:835`, panic), `StartApplications` (script), `AppCreate` (reserve `:821`, publish `:869`), trampoline (`GetTaskFunction`), App/Task/Lib records, `SystemState`, `WaitForSystemState`, `MainTaskSyncDelay`, background task, `CleanUpApp`;
  - EVS (register, `SendEventWithAppID`, own-ID guard); SB (pipes, routes, transmit outcomes, receive, subscription-reporting mode, capacity); TBL (register, share, load paths, address lock); TIME (1 Hz chain, seqlock, synch callbacks); FS background; CDS;
  - **Apps** (15) with their tables and the startup script.
- **Right column (Φ)**:
  - kinds (`osal.obj`, `es.app/task/lib`, `svc.identity`, `evs.reg`, `sb.pipe`, `sb.route`, `sb.queue`, `sb.mode`, `tbl.table`, `es.phase`, `es.appstate`, `es.cds`, `time.synchcb`);
  - operations with LPs;
  - rules R1–R19, P-*, RF with stratum and tag;
  - contracts (K-REG, K-ID, K-SPAWN, K-PHASE, K-ROUTE, K-CAP, K-MODE, K-TBL, K-PERSIST, K-HANDOFF, K-LIFE, K-GEN, K-LOCKBAL, K-LOCK.cancel, K-ATOM);
  - actor classes (`T_osal`, `T_raw`, `T_env`);
  - model levels M_api/M_impl.
- **Edges**: each cFS mechanism → the Φ elements it becomes, labelled with the rule or contract ID. For example:
  - `:835` + panic → R19 (S, obs-fatal);
  - `:803/:818` → `es.app` RESERVED/LIVE + K-SPAWN.app-record (core path);
  - `:821/:869` → K-SPAWN.app-record (script path);
  - `cfe_sb_task.c:138` → `svc.identity(SB)` publication + K-ID;
  - `OS_QueuePut` full → `sb.queue` + K-CAP;
  - `osapi-idmap.c:384–385` vs `:401–404` → K-LOCKBAL (M_impl only).

### D6 — Verdict state machine

- **States**:
  - `candidate`, `open`;
  - `inactive`, `no-provider`, `capability-absent`;
  - `protected[cert]`, `protected[model b]`, `tolerated`;
  - `fixed-order[cert]`, `fixed-order[model b]`;
  - `schedule-dependent`, `timeout-dependent`;
  - `unknown(not_reached | model_gap | bound | budget | key_unresolved | loss | unmodelled | writer_incomplete | horizon | conformance | model_fault)`.
- **Evidence sub-machine** (parallel region): `none` → `predicted` | `predicted-extrapolated` | `model(b)` → `observed(n of N)` → `confirmed(F0|F1)` with flags `derived`, `timeout-exercised`; `refuted`, `inconclusive`.
- **Transitions**:
  - candidate → inactive / no-provider / capability-absent / fixed-order[cert] / protected[cert] (static);
  - candidate → open;
  - open → schedule-dependent (bad + good witness);
  - open → fixed-order[model b] / protected[model b] (explorer within b);
  - schedule-dependent → timeout-dependent (σ_U∞ shows no bad execution);
  - open → unknown(budget) (budget exhausted);
  - any → unknown(model_fault) (O3/O4 contradiction; rule suspended);
  - unknown(model_gap | model_fault) → candidate (new Φ hash or model fix; loop restart);
  - evidence refuted → model-fault record → unknown(model_fault);
  - protected → tolerated (allowed not-ready witness).
- **Invariant**: no state named "safe"; no transition from "no alarm" to any universal class.

### D7 — Observation pipeline

- **Nodes**:
  - `plan.kz` (sites, decoders, cost classes);
  - loader (raw `bpf()`/`perf_event_open`) | tracefs backend (fallback);
  - uprobe registrations (file, offset; entry+4) on `core-cpu1` and app `.so`;
  - perf breakpoint watchpoints (≤ 4 per thread);
  - BPF programs (entry, point, return, watch);
  - site-descriptor map (IP → descriptor);
  - per-thread `tseq` (task storage);
  - per-object `objseq` map;
  - per-CPU ring buffers;
  - drop counters;
  - drainer;
  - `run-*.kzt` (SITES, RAW_EVENTS, STRINGS, STACKS, LOSS, RUNMETA);
  - `kz trace` (decode → identity join → HB → LP evaluation);
  - `kz hold` (ptrace, DR0–DR3);
  - harness root hook;
  - cFS process threads;
  - `.so` fallback hold at the `OS_ModuleLoad` return.
- **Edges**:
  - plan → loader: register sites, load descriptors;
  - loader → uprobes/watchpoints: attach;
  - thread hits probe → BPF program: trap (0.57 µs at entry+4; 31.7 µs class otherwise);
  - BPF program → descriptor map: lookup;
  - BPF → `tseq` / `objseq`: increment;
  - BPF → ring buffer: record (failure → drop counter);
  - ring buffer → drainer → `run-*.kzt`;
  - drainer → `kz hold`: live event stream (release on event);
  - `kz hold` → thread: stop/continue (debug-register gate);
  - harness hook → loader: start before launch;
  - `OS_ModuleLoad` return → fallback hold → attach `.so` probes → release;
  - `run-*.kzt` → `kz trace`.

### D8 (optional) — The R2 certificate

- **Nodes**: `u` (SB event sites inlined in `CFE_ES_TaskInit` and `CFE_EVS_TaskInit`); `m_ES` (return of the `CFE_ES_TaskInit` call, `cfe_es_task.c:114`); `m_EVS` (`cfe_evs_task.c:213`); raises at `cfe_es_task.c:134` and `cfe_evs_task.c:229`; falsifiers `cfe_es_start.c:803` (per iteration); `MainTaskSyncDelay` success exits `o_ES`, `o_EVS` (`:835`); panic branches (`:838–846`); SB spawn (`:810`, SB iteration); SB entry; publication `x` (`cfe_sb_task.c:138`); INIT (`memset` of SB global in EarlyInit).
- **Edges**: `u` ↛ `m` (no path, FRONTIER); `m` →T-PO raise →T-OBS `o`; `o` →T-PO (panic branches cut by T-TERM) SB spawn →T-SPAWN SB entry →T-PO `x`; INIT → "AppId = 0" (INIT_FALSE); writer-set box listing each entry with its status.

---

## 13. Glossary

| Term | Meaning in this specification |
|---|---|
| **Actor / actor instance** | a thread of execution (OSAL task, raw thread or environment root); an instance is (static actor, generation) |
| **Allowed** | the documented not-ready behaviours of a contract, from a finite vocabulary, each with a reference horizon (§1.6) |
| **Anchor / anchor audit** | the mapping of a source site or AST pattern to an instruction of the gcc binary; the audit lists sites without a unique anchor |
| **Assumed visibility** | an edge that holds only under A4 (unlocked `volatile` flag synchronization); flagged on every verdict that uses it |
| **B (basis)** | everything fixed except the schedule: Build, Config, Init, Env, Input (§1.7) |
| **B′** | a configuration or input variant: a new basis, never a "new bug" |
| **Bad execution** | an execution with a VIOLATING occurrence (or NOT_READY_UNJUDGED where Allowed is unspecified) |
| **Capability-absent** | class: a provider exists in code, but an environment capability it needs is absent in B.Env |
| **Census** | the instantiation of every contract at every context-qualified site, each with a class or `unknown(reason)` |
| **Certificate** | a checkable proof object (rule instances, writer entries, frontier, REACH) for a fixed-order or protected verdict |
| **Chain** | a violation link `i₁ ⇝ i₂`: `u₂` exists because of `u₁`'s not-ready outcome |
| **Cone** | the closed set of actors and objects relevant to a candidate (§4.10) |
| **Confirmed / derived / timeout-exercised** | reproduced on the gcc binary under declared control / the schedule was constructed / a program timeout fired during a hold |
| **Context-qualified template** | (actor instance, inlining call string, site) |
| **C_σ** | schedule constraints of σ_R1; never HB |
| **DECLARED_TRUSTED** | a writer entry whose exclusion rests on a Φ guard the analysis could not verify; listed in every certificate that uses it |
| **Evidence** | the axis that says how a class is supported: certificate, model(b), predicted, predicted-extrapolated, observed, confirmed |
| **F0–F3** | fidelity classes of [R07 §7]: unmodified binary; link/codegen option changed; sources rewritten; other compiler |
| **Falsifier** | an event in the observer's actor that dominates an observation and definitely makes the observed conjunct false |
| **Fixed-order** | class: every execution reaching the use is bad |
| **Frontier node m** | an event in the use's actor instance with no path to the use and `m ∈ MB(x)` for every writer x |
| **Generation** | an incarnation of a resource or actor (OSAL serial, cFE resource-ID serial, restart count) |
| **HB_π, VC, VC_R** | happens-before on a trace; vector clocks over all definite edges and over unconditional/obs-fatal edges |
| **Horizon h** | main-loop iterations explored after init; `unspecified_by_reference`, swept |
| **Inactive** | class: no execution reaches the use under B |
| **KEM** | Kairos Event Model: the shared event, resource and template types |
| **Key** | the identity of a resource within its kind (name, ID, MsgId, path); concrete, symbolic or ⊤ |
| **KZ container** | the typed binary artifact format (§2.3) |
| **L1/L2/L3** | lemma obligations: await summarization; removal of irrelevant critical sections; lock-atomic macro-steps |
| **LOSS-UNDER-ORDER** | outcome label for route/capacity contracts whose Allowed is unspecified by reference |
| **LP (linearization point)** | the point inside an API where an effect takes place; readiness is evaluated there |
| **LP interval** | the observed bracket around an LP in a trace |
| **M_api / M_impl** | OSAL model levels: API atomic at its LP / OSAL POSIX internals analysed |
| **MB (MustBefore)** | static must-precedence, the greatest fixpoint of §4.4 |
| **Model-fault record** | the report written when an observation contradicts a model class or rule |
| **No-provider** | class: the use is reachable, but no model transition can make a required conjunct true |
| **O-edge / S-edge** | observed (reorderable) / structural (must) ordering edges |
| **O1–O8, CC** | consistency obligations and the certificate check (§2.5) |
| **On-behalf frame** | an identity frame for code run in one actor on behalf of another (TIME synch callbacks, FS callbacks, timer callbacks) |
| **Outcome mining** | creating instances from observed not-ready codes without a template |
| **Φ (profile)** | the declarative cFS knowledge base: kinds, operations, effects, LPs, rules, contracts, constants |
| **Portability hazard** | protected under M_impl but violable under M_api |
| **Pred(π)** | the predictive model of a trace (maximal causal model with control-flow abstraction) |
| **Protected / tolerated** | class: no bad execution / protected with allowed not-ready occurrences |
| **PWB** | precedes-whenever-both-occur: every occurrence of a precedes every occurrence of b |
| **REACH** | the reachability component of a certificate (observed or confirmed occurrence) |
| **Robustness tag** | per trace edge: unconditional, obs-fatal, obs-soft, schedule-chosen, assumed |
| **Schedule-dependent / timeout-dependent** | class: bad and good executions exist / and no bad one under σ_U∞ |
| **Shape** | WRONG-WAY, UNORDERED, ABSENT, STALE |
| **σ_U, σ_U∞, σ_R1, σ_E** | untimed; patient (no timeouts, fair, stall); fixed-priority one core; the real environment |
| **Stall** | a σ_U∞ outcome in which an actor waits forever on a false predicate |
| **Support locations** | the memory a conjunct's framework oracle reads |
| **T-PO, T-SPAWN, T-OBS, T-TERM** | the MustBefore rules (§4.4) |
| **Template-only run** | an evaluation run with the frozen profile and no case-specific refinement |
| **Transition-aware writer** | a store that can change a conjunct from false to true (W) or true to false (D) under its guard |
| **unspecified_by_reference** | a policy status: no reference fixes the value; not chosen silently; swept or left to the PI |
| **Writer set W / disabler set D** | stores that can make a conjunct true / false for a key |

---

## Appendix A. Worked examples (expected behaviour; nothing here was run)

### A.1 R2: fixed-order under σ_U from a generic template, with the early-return regression

1. **Census.** `K-ID.send-with-appid` (provenance `cfe_evs.h:168–172`) is instantiated at every context-qualified `CFE_EVS_SendEventWithAppID` site. The SB sites pass `CFE_SB_Global.AppId` as `stamp` (39 SB sites, [R2c §2]). Contexts inlined from `CFE_ES_TaskInit` and `CFE_EVS_TaskInit` are distinct templates (ADR-012). No SB-specific contract exists.
2. **Conjunct and writer set.** Conjunct `svc.identity(SB)` SET, support location `CFE_SB_Global.AppId`.
   - INIT: SB's EarlyInit clears its global ([A §A.1] cites `sb/cfe_sb_init.c:50`), so `AppId = 0 = CFE_ES_APPID_UNDEFINED` → INIT_FALSE.
   - `W`: the success-branch out-pointer store of `CFE_ES_GetAppID` instantiated at `cfe_sb_task.c:138` (COMPUTED).
   - The failure-branch store writes UNDEFINED (`cfe_es_api.c:758–760`) and is neutral under the transition filter.
   - `kz modref` must show that no other store writes a valid value; otherwise it lists the store.
3. **Observation writer set at the ES iteration.** `o_ES` is the success exit of `MainTaskSyncDelay` at `cfe_es_start.c:835` in the ES iteration of the unrolled core loop. Its falsifier `f_ES` is `SetUsed(RESERVED)` at `:803`; the record's `AppState` is UNDEFINED from INIT, and no `AppState` writer can precede `f_ES`, because no task exists yet. `W_o` contains:
   - the raise in `WaitForSystemState` executed by ES-owned actors (only ES main reaches it, at `cfe_es_task.c:134`);
   - `ExitApp`'s STOPPED store (external apps only; kept or excluded, either way dominated);
   - not the scan's WAITING store (neutral by the transition filter);
   - not `CleanUpApp`'s free (external apps with `AppState > RUNNING`; `DECLARED_TRUSTED`).

   Every remaining entry is in ES main and dominated by `m_ES`, the return of the `CFE_ES_TaskInit` call at `cfe_es_task.c:114`.
4. **MustBefore chain**:
   - `m_ES ∈ MB(raise)` (T-PO), hence `m_ES ∈ MB(o_ES)` (T-OBS);
   - `o_ES ∈ MB(spawn_SB)` (T-PO along the startup thread; the panic branches at `:838–846` have no successors, T-TERM);
   - the SB entry has the single spawn site `spawn_SB` (T-SPAWN);
   - `x` follows the SB entry (T-PO).

   Hence `m_ES ∈ MB(x)`. EVS's uses get `m_EVS`, the return of `CFE_EVS_TaskInit` at `cfe_evs_task.c:213`, by the same argument through `o_EVS`.
5. **Frontier and the regression.** `m_ES ∉ Anc(u)` for every use inlined in `CFE_ES_TaskInit` (called once, no loop around it), so the FIXED_ORDER certificate applies (L-FO).
   - **The early returns do not matter.** `CFE_ES_TaskInit` returns at L340, L371, L381 or L393 after failures. `CFE_ES_TaskMain` still raises at L134 (re-read). The proof never needs `u` to dominate the raise ([J2-1]).
   - S3's exit test checks this with a mutation: replacing `m_ES` by a node inside `TaskInit` that precedes `u` must be rejected.
6. **Verdict**: `fixed-order(σ_U, M_api and M_impl)`.
   - Evidence: `static-certificate` + `observed(n of N)` once S1 re-observes the uses.
   - Outcome `ILLEGAL_APP_ID`, discarded by every SB caller; Allowed = none (the header names the system log as the alternative); harm `event_lost_silently`.
   - Shape WRONG-WAY where SB publishes, ABSENT where SB's own window is hit (both bad).
   - By L-CONT it holds under σ_U∞, σ_R1 and σ_E wherever the use is reached.
7. **Expected count** (a check of basis sensitivity, not new evidence of the violation):
   - ES 20 = 17 `SEND_NO_SUBS` + 2 `SUBSCRIPTION_RCVD` + 1 `PIPE_ADDED`; EVS 4 = 2 + 1 + 1. Total 24 per boot with ENV's version table ([R2g §2]; [R2c §7]);
   - 11 (ES 7, EVS 4) in a basis whose version table is NULL ([R2c §4.2]).
8. **Chain.** The 17 `SEND_NO_SUBS` uses are linked to K-ROUTE instances for ES's event packets (MsgId 0x808), transmitted before any subscription ([R2g §2]). They carry LOSS-UNDER-ORDER, harm unspecified.
9. **Not claimed**: that R2 is a race; any count as a Kairos result before S1; determinism from repeated runs.

### A.2 The core-task App-record window and the unpublished SB/TBL identity (INFERENCE until run; [J1 §1.1], [LED §3.4])

1. **Instances.** `K-SPAWN.app-record` at the `GetAppID` LP of each core task's first identity-dependent call: EVS `cfe_evs_task.c:273`; ES's `CFE_EVS_Register` in `CFE_ES_TaskInit`; SB `cfe_sb_task.c:138`; TBL `cfe_tbl_task.c:196`; TIME's.
2. **Static.**
   - `W` = `CFE_ES_AppRecordSetUsed(rec, PendingAppId)` at `cfe_es_start.c:818` (startup thread).
   - The child's MB contains the spawn and, through T-OBS at `GetTaskFunction`, the task-record fill (`cfe_es_apps.c:694–703`), but not `:818`. Nothing else orders them; only the 50 ms delay separates them.
   - No certificate either way → open, routed **O-ordered**: both LPs are under the ES lock.
3. **Prediction.** From a default boot, reverse the startup thread's critical section at `:816–818` and the child's `GetAppID` critical section. The child's poll keeps its success outcome, because the task record was filled earlier. A witness is expected → `schedule-dependent(σ_U)`, evidence `predicted`. Under σ_U∞ no timeout is involved, so the class stays schedule-dependent, not timeout-dependent.
4. **Consequences**, by code reading:
   - EVS: `RESOURCEID_NOT_VALID` → init error → `ExitApp(CORE_APP_INIT_ERROR)` → `ResetCFE(PROCESSOR)` (harm `processor_reset_requested`);
   - ES: `CFE_EVS_Register` fails → early return; if the raise also falls inside the window it is skipped → the core sync times out → panic;
   - SB and TBL: the status is ignored, so the identity stays UNDEFINED for the run. Every later SB/TBL identity use becomes bad with shape ABSENT, chained to this root (EventRacer-style race coverage reports the root, not the 211–229 dependents).
5. **Confirmation (D16, derived).** Hold the startup thread after `CFE_ES_StartAppTask` returns (`cfe_es_start.c:810–814`) and before `CFE_ES_LockSharedData` at `:816`; its program-lock set is empty there. Release when the child's `GetAppID` returns.
   - The watchdog is the smallest reference timeout the hold can make expire: the already-created core tasks wait in `WaitForSystemState(CORE_READY, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` = 30000 ms (`default_cfe_core_private_internal_cfg.h:68`), and the startup thread itself waits on nothing while held.
   - Oracle: the `GetAppID` outcome; for SB, `ILLEGAL_APP_ID` on every later SB event.
   - Labelled derived.
6. **σ_E**: timing-protected by the 50 ms delay; frequency unknown; no claim.

### A.3 The script-app App-record window (correcting [B §7.2])

1. **Instances.** `K-SPAWN.app-record` at each script app's first `GetAppID`-dependent call (e.g. `CFE_EVS_Register` → caller context → `GetAppID`). `K-REG.es` at peers' `GetAppIDByName`, whose oracle (`CFE_ES_AppRecordIsMatch`) classifies the success with `0xFFFFFFFF` as not-ready ([J1 §1 check 4]).
2. **Trace routing.** The publication (`cfe_es_apps.c:861–869`) and the use's LP are both under the ES lock. A trace orders them by a lock edge, so this is the **O-ordered** route to prediction, not "unordered → SHB" ([J1 §1 check 3]; [J3 §2.2]). SHB, WCP and SyncP cannot reverse conflicting critical sections; the explicit lock-reversing search can.
3. **Expected**: `schedule-dependent(σ_U, σ_U∞)`, evidence `predicted`; in σ_E, timing-protected by the 50 ms delay.
4. **Confirmation** ([B §7.2], corrected placement). Hold the startup thread in `CFE_ES_AppCreate` after `CFE_ES_StartAppTask` returns (`:848–854`) and before the lock at `:861`; its lockset is empty there. Release when the child's first `GetAppID`-dependent call returns.
   - The watchdog is computed by the planner as the smallest timeout among the waits of already-started actors that the held startup thread can make expire (their `WaitForSystemState`/`WaitForStartupSync` call-site arguments, constant-evaluated; the startup-script barrier `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` once the script phase has begun).
   - Oracle: `CFE_ES_ERR_RESOURCEID_NOT_VALID` (`cfe_es_api.c:757–761`); `GetAppIDByName` success with the sentinel for peers.
   - Labelled derived.

### A.4 C1 in a derived configuration, and the OSAL lock-leak path under M_impl

1. **Default B.** CF's lookup sits behind a non-empty `sem_name` test (`cf_cfdp.c:1270`). Both default entries are empty (`cf_def_config.c:60, 78`) → `inactive(B)`; the proof is the folded path condition.
2. **B′** (public-code-derived, [LED §1a]): a non-empty name and a provider app that creates the counting semaphore in its init. Neither app waits on a phase → no MB relation → open. Observation shows either success (outcome-implied reads-from from the provider's publication, an O-edge through the countsem table lock) or `NAME_NOT_FOUND` followed by retries.
3. **M_api.** Prediction reverses the lock-mediated order. The retry loop is extrapolated up to the **effective** bound (`CF_STARTUP_SEM_MAX_RETRIES`, constant-evaluated; §0.5); such a witness is labelled `predicted-extrapolated`. The explorer finds an execution in which every attempt precedes the creation (delays are yields) → CF sends `CF_INIT_SEM_ERR_EID` (`cf_cfdp.c` ~L1295, re-read) and its engine initialization fails (consequence per [A §A.2]: engine disabled, CF exits) → `schedule-dependent(σ_U)`. The loop is delay-paced, so its exhaustion branch is a timeout transition: under σ_U∞ no bad execution remains, and the σ_U class is refined to **timeout-dependent** [I]. The fix of C1 (the bounded retry, [LED §1b]) thus turned an ordering-logic violation into a timing-margin dependence; at the buggy revision (a single attempt) the violation needs no timeout and stays schedule-dependent under σ_U∞. A single early lookup is `tolerated` (`retry_until_ready`).
4. **M_impl.** The loop is unrolled, not summarized. The explorer also reaches the lookup while the provider's record is RESERVED with its name visible:
   - the lookup returns `OS_ERR_INCORRECT_OBJ_STATE`, which ends CF's loop (`cf_cfdp.c:1285–1288`, [R01 §1.8]);
   - by K-LOCKBAL, the countsem table lock stays held (`osapi-idmap.c:401–404` vs the documented invariant `:384–385`);
   - the provider then blocks in `FinalizeNew` → deadlock check → **implementation-defect candidate** (F1).
5. **Confirmation plans.** For (3): hold the provider before its create call (lockset empty) until CF's loop exit is observed; the watchdog is the effective retry bound × effective delay. For (4), a **declared experiment**: hold the provider at `OS_CountSemCreate_Impl` entry (inside the RESERVED window, name visible, type lock **not** held, [R01 §1.5 step f]) until CF's lookup returns, then release. The oracle is a wait-for fact: the provider is blocked on the countsem table lock (futex/wchan) while CF has already returned from its API. This is the experiment [R01 §7 item 1] proposes; here the planner derives it from the model. Labelled derived.

### A.5 Startup-burst capacity: `SBNSubPipe` (K-MODE + K-CAP, LOSS-UNDER-ORDER)

1. **Instances.** K-CAP at each delivered put into `SBNSubPipe` (effective capacity 10, [BM §3.1]). K-MODE for SBN's subscription-reporting enable versus TO_LAB's 37 global subscriptions from its table ([R03 §2.3, §2.15]).
2. **Tier 1.** The maximum-weight-closure backlog bound over the MB order of puts and gets is up to 37 > 10, so there is no protected certificate → open. The attribution of the burst to SBN's toggle versus TO_LAB is an inference in [R03 §5.2] ([J2 §7]); the census instance identification checks it.
3. **σ_E.** Overflow was observed in 5 of 6 M1 runs, and the sixth (A3) showed a different interleaving with `TO_LAB_TLM_PIPE` overflowing instead ([BM §3.3]). These are prior observations by the M1 harness (D12); Kairos's recorder re-observes them in S1/S2.
4. **Verdict.** With both outcomes observed: `schedule-dependent` (σ_E witnesses, transferred up to σ_U), label LOSS-UNDER-ORDER, harm `unspecified_by_reference`. Oracle: `PipeOverflowErrorCounter` and the per-pipe peak. This is part of RQ5.

### A.6 HS system monitor: `capability-absent(B)` (correcting [A], [R03 §3 row 20])

`HS_SysMonInit` finds the `linux_sysmon` device: the module is in the PSP's fixed list (`psp_module_list.cmake:9`), so `FindByName` succeeds; a failure there would be handled (`hs_sysmon.c:50–54`). The `SET_RUNNING` command then fails, and HS returns `CFE_STATUS_EXTERNAL_RESOURCE_FAIL` (`:65–72`), because `/proc/schedstat` is absent on this kernel ([R01 §2.6]; [R07 §1]).

Φ declares the device's RUNNING transition guarded by `env(/proc/schedstat present)`, and B.Env records it absent. Outcome mining finds the not-ready code, and O4 agrees → `capability-absent(B)`. This is not an order problem. It is reported separately, with the measured consequences: HS init stops at `HS_SysMonInit`; its startup sync is skipped; CPU utilisation is reported as `0xFFFFFFFF` ([BM §0]).

### A.7 Calibration: timebase lookups are protected

`cFS-Master` is created and started by the PSP's module init on the main thread before `CFE_ES_Main` (§3.5). `x` (its publication LP) is in MB of every cFE task entry, by T-PO on the PSP main thread and T-SPAWN. The disabler set is empty in the default B. TIME's and SCH_LAB's lookups (`cfe_time_task.c:311`, `sch_lab_app.c:176`) are therefore `protected` by certificate under σ_U. This is a check that the tool does not invent races where the code orders.

---

## Appendix B. Literature relied on (status as verified in [R04], [R05], [R06])

- Lamport 1978; Fidge 1988; Mattern 1989; Netzer & Miller 1992; Lu et al. 2008 — V-abs.
- FastTrack 2009; ThreadSanitizer 2009/2011; Eraser 1997; O'Callahan & Choi 2003 — V-abs (Djit+ 2003: P).
- CP 2012; WCP 2017; SHB 2018; M2 2020; SyncP 2021; Vindicator 2018; maximal causal models 2012; RVPredict 2014; GPredict 2015 — V-abs.
- DefUse 2010; AVIO 2006; Velodrome 2008; ConMem 2010; CTrigger 2009; Xiong et al. 2010; Tian et al. 2008 — V-abs.
- EventRacer 2013; DroidRacer 2014; CAFA 2014; InitRacer 2017 — V-abs. DCatch 2017 — P.
- CloudRaid 2018; Snowboard 2021; FCatch 2018; DataCollider 2010 — V-abs. KCSAN design document — V-doc.
- RacePro 2011; DESCRY 2017; RaceFuzzer 2008; Portend 2012 — V-abs.
- DPOR 2005 (guarantee wording UNVERIFIED); source/optimal DPOR POPL 2014 (JACM extension: P); BPOR 2013; fair stateless model checking 2008; PCT 2010 (mechanism P); SCTBench 2014 — V-abs.
- Alglave, Kroening & Tautschnig 2013; Wang et al. 2009 — V-abs.
- SIERRA 2018; SDV 2006; Holzmann & Joshi 2004 — V-abs. FlowDroid 2014 — V-me.
- Andersen 1994; SVF 2016; MLTA 2019 — V-abs.
- Soundiness 2015; Arcuri & Briand 2011; Klees et al. 2018 (trial count: P) — V-abs.
- Strom & Yemini 1986 (typestate); Engler et al. 2001 — V-me.
- Boehm 2012 (seqlocks), cited only as verified in [R02 §10].
- Standard algorithmic fact used without a verified citation: maximum-weight closure via minimum cut (§3.9, K-CAP bound), as in [A §4.6].

No other literature is relied on.

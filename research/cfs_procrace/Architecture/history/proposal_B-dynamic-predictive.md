# Proposal B — "Sextant": a trace-first, predictive readiness-contract detector for cFS

- Date: 2026-10-12. Angle B of the AR2 architecture panel ("trace-first"). Design only: nothing was built, installed, run against cFS, modified or committed for this document.
- Evidence base: reports [R01]–[R07] in `scratchpad/arch/`, the policy `research/cfs_procrace/CONDITIONS_POLICY.md` (cited as **[POL]**), `cases/ledger.md` (**[LED]**), `results/R2_gdb_v701.md` (**[R2G]**), `results/R2_cord_v701.md` (**[R2C]**), `env/BASELINE_MEASURE.md` (**[BM]**), and the user's v0.1 design (**[v0.1]**). Code paths are relative to the pinned tree `/home/user/work/procrace/cfs_ref/cFS` (cFS v7.0.1; cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`).
- Name: **Sextant** (a navigation instrument that fixes position by observing, without touching what it measures). The name is a proposal only.

**Labels.** **[F]** fact with a citation (code, build artifact, record, or report). **[M]** measured on this VM, from [R07] or from a read-only spot check made for this document (§A.2). **[I]** my inference; not executed. **[U]** unverified. **[D]** a design decision of this proposal. Literature is cited only as verified in [R04]/[R05]/[R06], with their status tags (V-doc, V-abs, V-me, P, UNVERIFIED) kept.

---

## 0. The design on one page

**Thesis.** In cFS the defects that matter are uses of framework resources (registries, identities, phases, routes, capacities, tables, lifecycles) in a state the API contract forbids. Many of these uses are *ordered the wrong way* by the framework's own startup protocol (R2), many others are *lock-protected order violations* (C1, C5), and the measured losses in the default bundle are *outcomes* of lossy queues (no subscriber, queue full) rather than conflicting accesses ([R03] §5.2; [BM] §0). Sextant therefore observes the **unmodified reference binary** at the framework API boundary, lifts the observation into a typed event graph with **four identities per event** and **generation-aware resources**, builds a **cFS-aware happens-before (HB)** with event-driven rules and **conditional (timeout-aware) edges**, checks **readiness contracts on ordered and unordered pairs**, **predicts** reversed orders soundly from the observed trace, and **confirms** predictions on the real binary by holding a thread at an exact point until a named event occurs.

**Pipeline (all core components in C++20; no Python/JSON in the core).**

```
 static (probe placement + API summaries only)          dynamic (observation)                model-based                         confirmation
 ┌───────────────────────────┐   probe.plan    ┌────────────────────────────┐  trace.cft  ┌──────────────┐ ceir ┌──────────────┐ verdict.db ┌───────────────┐
 │ sx-plan                   │ ──────────────▶ │ sx-observe (root side)     │ ──────────▶ │ sx-lift      │ ───▶ │ sx-analyze   │ ─────────▶ │ sx-confirm    │
 │ api.cat, site.cen,        │                 │ uprobes on unmodified      │             │ identities,  │      │ HB+robust HB │  witness   │ hold-until-   │
 │ model map M, DWARF of gcc │                 │ core-cpu1 + app .so        │             │ resources,   │      │ contracts    │ ─────────▶ │ event gates   │
 │ binary, contract library  │                 │ (F0 fidelity)              │             │ lifecycles   │      │ prediction   │            │ (F0)          │
 └───────────────────────────┘                 └────────────────────────────┘             └──────────────┘      └──────────────┘            └───────┬───────┘
              ▲                                                 ▲                                                        │ confirm.cft             │
              └──────────── coverage gaps, new probe sites ─────┴──────────── refuted witnesses (model feedback) ◀──────┴─────────────────────────┘
```

**What is new (the claimed combination, not any single mechanism; [R04] §5 G1–G8; [R06] §6.1):** (i) contracts with *allowed not-ready outcomes*, instantiated from API documentation at every observed use; (ii) verdicts quantified over a recorded basis **B** and a named scheduling semantics **σ**, with a **robust-HB certificate** for fixed-order claims; (iii) **conditional edges** for soft, opt-in, timed-out barriers; (iv) **user-space registries with reserve/finalize/reuse** as the conflict object, locks as atomicity; (v) **lossy queues** as predictable outcomes; (vi) **layered identity** (executing task, owning app, called service, stamped identity); (vii) **F0 observation** of the reference gcc binary with measured per-probe cost.

**What is deliberately not done.** No whole-program static race analysis, no harness generation, no exhaustive model checking of cFS (static analysis is confined to probe placement and API/site summaries, per the angle). No MLIR in the core (§2.6). No compiler change to the executed binary.

---

## 1. Problem definition

### 1.1 Executions, contexts and the four identities

Let a run of cFS under basis B produce a trace T. An **event** is

`e = ⟨eid, ctx, tseq, kind, site, res, args, outcome, ids⟩`

- **ctx** — the *executing context*: an OSAL task (a pthread with an OSAL task record), or a raw context with no task record: the PSP main thread that runs `CFE_ES_Main` (the "ES startup thread"), the console writer thread, the OSAL timebase thread (which registers the **timebase** ID, not a task ID), and signal-handler contexts [F] ([R01] §2.6, §2.3; [R02] §1.1). `OS_TaskGetId()` returns 0 for raw threads and a timebase ID inside timer callbacks [F] ([R01] §2.3).
- **tseq** — the position of e in ctx's program order (exact, per thread).
- **kind** — a semantic event kind from the model M (§3), e.g. `osal.create.reserve`, `osal.create.finalize`, `osal.lookup`, `es.apprec.publish`, `sb.subscribe`, `sb.put`, `sb.get`, `evs.send_with_appid`, `es.wait_state.return`.
- **site** — the code location (file:line in the pinned tree, and the instruction address in the gcc-built binary).
- **res** — the resource touched (§1.2), if any.
- **outcome** — the return code or effect the program itself used ([v0.1] §9.3; [R06] F11).
- **ids = ⟨exec, owner, service, stamp⟩**, four separate fields ([v0.1] §2.1; [R06] F1):
  - **exec**: the executing task (ctx resolved to an OSAL task or raw context);
  - **owner**: the cFE app that owns exec at e, from the ES task record (published at `cfe_es_apps.c:703`), or `pending(app)` before that record is published, or `none` for raw contexts [F] ([R02] §1.6);
  - **service**: the framework module of the innermost observed API frame on ctx's call stack (app code, ES, SB, EVS, TBL, TIME, FS, OSAL, PSP). cFE service APIs run in the *caller's* thread [F] ([R02] §2.4, §2.8, §3.5);
  - **stamp**: the identity value e writes into a record or passes on someone's behalf (e.g. the `AppID` argument of `CFE_EVS_SendEventWithAppID`, the pipe owner stored by `CFE_SB_CreatePipe`) [F] ([R02] §2.2, §3.2).

R2 is decided by these four fields: the ES task (exec) runs SB code (service) and stamps SB's not-yet-published AppId 0 (stamp) [F] ([R2G] §2).

### 1.2 Resources with lifecycle generations

A **resource** is `r = ⟨ns, key, gen⟩`:

- **ns** — a namespace: OSAL object type (task, queue, countsem, binsem, mutex, timebase, timer, stream, module); ES app / task / lib / generic counter; CDS block; TBL registry entry; SB pipe; SB route `(MsgId, pipe)`; EVS registration slot; a *service identity* global (`CFE_SB_Global.AppId`, `CFE_EVS_Global.EVS_AppID`, `CFE_TBL_Global.TableTaskAppId`); ES system state; app state; loader symbol; file path; global SB mode (subscription reporting).
- **key** — a name (string bounded by reference constants: `OS_MAX_API_NAME` 20, `OS_MAX_PATH_LEN` 64 in the built `osconfig.h` L171, L162 [M]) or a numeric key (MsgId, AppId index).
- **gen** — the incarnation: the OSAL serial (`(type<<16)|serial`, slot = `serial % OS_MAX_<type>`) or the cFE resource-ID serial; a stale ID fails the match check [F] ([R01] §1.2; [R02] §6.2). IDs are not stable across runs, so cross-run correlation uses `(ns, name, creator, creation ordinal)` [F]/[I] ([R01] §1.2, §6.2 item 3).

Each namespace has a **lifecycle LTS** (a typestate machine; Strom & Yemini, IEEE TSE 1986 [R06 V-me]). The common shape is `ABSENT → RESERVED_c → LIVE → RESERVED_d → FREE`, with namespace-specific refinements [F] ([R01] §1.5–1.7; [R02] §1.16):

- OSAL: `RESERVED_c` splits into *name-invisible* (steps b–d) and *name-visible* (steps e–f, after `OS_OBJECT_INIT`), and **during RESERVED the type table lock is not held**; LIVE begins at `osapi-idmap.c:1092` with release at `:1105` [F] ([R01] §1.5).
- ES app record: RESERVED at `cfe_es_apps.c:821`, LIVE at `:869`; during RESERVED `CFE_ES_GetAppIDByName` returns `CFE_SUCCESS` with `0xFFFFFFFF` [F] ([R02] §1.6).
- SB pipe: OSAL queue visible before the SB descriptor commits (`cfe_sb_api.c:163` vs `:190-194`) [F] ([R02] §1.16).
- Service identity: `UNSET → SET → REGISTERED(EVS)` (SB: `cfe_sb_task.c:138`, `:155`) [F] ([R02] §2.8, §7.1).

Lookups are **predicates over the LTS state** with namespace-specific outcomes (e.g. OSAL by-name: `NAME_NOT_FOUND` if name-invisible, `INCORRECT_OBJ_STATE` if name-visible RESERVED, success if LIVE) [F] ([R01] §1.7).

### 1.3 Readiness contracts

A **contract template** is

`K = ⟨id, ns, U, Req, Allow, Out, prov⟩`

- **U** — a use pattern: an API (or a modeled internal operation) with argument bindings that identify the resource (e.g. `CFE_EVS_SendEventWithAppID(AppID=$a)` binds `r = identity_of($a)`).
- **Req ⊆ Q_ns** — the **required precondition** on the resource state at the use (e.g. `{REGISTERED}`).
- **Allow** — the **allowed not-ready behaviour**: a set of outcomes or recovery shapes that the reference documents as acceptable, e.g. `retry(≤25, NAME_NOT_FOUND)` for CF's semaphore lookup (`cf_cfdp.c:1270-1303`; `cf_internal_cfg.h:172-185`) [F] ([R03] §2.6), `defer-by-design` for TO_LAB's post-OPERATIONAL subscription (`to_lab_app.c:64-66`) [F] ([R03] §2.3), or empty.
- **Out** — the outcome map from observable results to `ready` / `not_ready(q)` (e.g. `CFE_EVS_APP_ILLEGAL_APP_ID ↦ not_ready(UNSET)`, `CFE_EVS_APP_NOT_REGISTERED ↦ not_ready(SET)`) [F] ([R02] §3.2).
- **prov** — provenance: header file:line, code comment, issue, or researcher hypothesis, plus authoring timestamp and hash ([POL]; [R06] §5.3).

An **instance** is `κ = ⟨K, u, r⟩` for each event u matching U. In an execution ρ:

- `state_ρ(r, u)` = the LTS state of r after the transitions that precede u in ρ;
- **ready** iff `state_ρ(r,u) ∈ Req`;
- **violation** `V_ρ(κ)` iff not ready and the outcome of u is not in Allow;
- **tolerated not-ready** iff not ready and the outcome is in Allow (reported, never counted as a violation).

The error return itself is not assumed to be forbidden ([v0.1] §2.2). Whether a not-ready outcome is acceptable is taken from the reference; when no reference says, the field is `unspecified_by_reference` ([POL]).

Multi-resource contracts (e.g. TBL `GetAddress` requires *registered and loaded*) use a conjunction of per-resource predicates ([R02] §4.5, §4.8).

### 1.4 The analysis basis B and the semantics σ

**B** fixes everything except the schedule ([v0.1] §2.3, extended per [R06] T5):

`B = ⟨binaries (sha256 of core-cpu1 and each .so), configuration C, inputs I, initial persistent state S0, environment E⟩`

- **C**: the generated startup script (`build-native_std/exe/cpu1/cf/cfe_es_startup.scr`), the generated core object list (`cfe_static_module_list.c`, from `MISSION_CORE_MODULES`), the built tables, platform constants as built [F] ([R02] §1.3; [R05] F2, F5).
- **I**: ground inputs with **event-based** trigger points (e.g. the CI ES-restart stop of [BM] §1.1).
- **S0**: `EEPROM.DAT`, `/dev/shm/osal:RAM` files, SysV segments, key files, and the reset type they imply [F] ([R01] §5.2–5.3; [BM] §6).
- **E**: kernel version (now `6.18.44-fc-v114`, drifted from the recorded `-fc-v80`) [M] ([R07] §1), CPU count and affinity (including the PSP's CPU-0 pinning of `CFE_*` tasks, `cfe_psp_start.c:184-189`, not yet recorded) [F] ([R01] §2.3), effective policy (all threads `SCHED_OTHER`, priorities overridden to 255) [F] ([R01] §2.5), effective queue depths (`min(requested, msg_max=10)`) [F] ([BM] §3.1).

**σ** names the scheduling and timing semantics under which a verdict holds ([R06] T5):

| σ | Definition | Use |
|---|---|---|
| **σ_U** (untimed) | Any interleaving consistent with M's synchronization semantics; every timeout, poll expiry and bounded wait may fire nondeterministically; a panic branch is terminal. | Default for feasibility. |
| **σ_ENV** | The measured platform: `SCHED_OTHER`, 4 vCPU, recorded affinity, reference timing constants. **[I]** `SCHED_OTHER` gives no bounded-progress guarantee, so σ_ENV admits the same interleavings as σ_U; what σ_ENV adds is a **measured frequency** over N observed runs. | Frequency statements only; never a feasibility proof. |
| **σ_RT1** | Fixed-priority preemptive on one core, with the requested priorities. | Model-only (priority constraints in the predictive encoding, §4.5) unless the PI approves a native strict-priority arm (§10). Reproduces the C3 class ([LED] §1a C3). |

### 1.5 Verdicts

For each contract family (template × site × resource class), Sextant emits

`verdict = ⟨class, σ, β, evidence, certificate | witnesses, assumptions⟩`

where β is the **bound**: traces analysed (count, coverage of the site census), the predictive window, the extrapolation bounds used (each tied to a reference constant), and the solver budget.

| class | Meaning (relative to M, B, σ, β) |
|---|---|
| `inactive(B)` | The site cannot execute under C (requires a static configuration summary, e.g. CF's empty `sem_name`, `cf_def_config.c:60,78`) [F] ([R03] §2.6). |
| `unobserved` | No instance in the traces. A coverage gap, never "safe". |
| `protected` | Every predicted execution has the use ready. Carries a certificate (robust chain `p ≺_R u`, or UNSAT of the violation query) and its assumptions. Never phrased as "safe". |
| `fixed-order` | Every predicted execution in which u executes violates. Carries a **robust-HB certificate** (`u ≺_R p`, §4.3) or an UNSAT core of the conforming query, plus the path-stability assumption. |
| `schedule-dependent` | A violating and a conforming execution both exist; each side is labelled *observed*, *predicted*, or *confirmed*. |
| `timeout-dependent` | Violation requires an escape branch of a soft barrier or poll (e.g. the 1000 ms startup sync, `cfe_es_start.c:210-228`) [F] ([R02] §1.2). |
| `tolerated` | Not ready, but the outcome is in Allow. |
| `unknown(reason)` | Unmodeled mechanism on a relevant path, trace loss, unresolved identity, solver budget, or a model refuted by confirmation. |

Evidence levels follow [R06] S1: candidate → model witness → observed → confirmed → (for fixed-order) certified.

### 1.6 Why plain data-race HB detection is insufficient

1. **Wrong-direction ordered uses (R2).** All 24 violating uses per boot (ES 20, EVS 4) are HB-ordered *before* SB's publication by the S1 core serialization [F] ([R2G] §2; [R2C] §7; [R02] §1.3 R19). HB and predictive race detectors report only unordered conflicting pairs; by definition they report nothing here ([R04] L4, G2). A **contract check over ordered pairs** is required.
2. **Lock-protected order violations (C1, C5).** C1's create and lookup, and C5's registration and lookup, are each inside critical sections of the same lock [F] ([LED] §1a). HB adds the lock edge in the observed order and hides the other order; lockset sees consistent protection ([R04] L5; [R05] §2.4). The conflict is on a **registry key**; the lock gives atomicity, not order.
3. **Losses are outcomes, not conflicts.** No-subscriber sends return success and are only counted (`cfe_sb_priv.c:1083-1086`); queue-full drops are non-blocking (`os-impl-queues.c:300-315`) [F]. Measured: 4–49 queue-full drops per boot, `NoSubscribersCounter` 15–26 [F] ([BM] §0). No surveyed HB model treats "lost under some feasible order" as a property ([R04] G5).
4. **Ad hoc synchronization.** `SystemState` is a `volatile sig_atomic_t` written and polled without a lock (`cfe_es_global.h:151`; `cfe_es_api.c:585`) [F]. A memory-level detector flags the intended synchronization; an HB model that omits it flags everything after it ([R04] §2.4 KCSAN's argument against KTSAN [V-doc]; [R02] §8.3).
5. **The relevant operations are often not memory conflicts at all**: an OSAL name lookup, an ES record lookup, a value returned through an out-pointer ([R04] L1).

**Theory extended (each with status).** Lamport's HB (CACM 1978 [V-abs]) and vector clocks (Fidge 1988, Mattern 1989 [V-abs]); event-driven HB with queue and handler rules (DroidRacer PLDI 2014 [V-abs], CAFA PLDI 2014 [V-abs], DCatch ASPLOS 2017 [P]); ad hoc synchronization recognition and race coverage (Xiong et al. OSDI 2010 [V-abs]; Tian et al. ISSTA 2008 [V-abs]; EventRacer OOPSLA 2013 [V-abs]); typestate contracts (Strom & Yemini 1986 [V-me]) checked at run time (KCSAN `ASSERT_EXCLUSIVE_*` as precedent [V-doc]); def-use order violations (DefUse OOPSLA 2010 [V-abs]; Lu et al. ASPLOS 2008 [V-abs]); initialization races (InitRacer OOPSLA 2017 [V-abs]); sound predictive analysis over a trace (maximal causal models RV 2012 [V-abs]; RVPredict PLDI 2014 [V-abs]; GPredict ICSE 2015 [V-abs]; WCP PLDI 2017, SHB OOPSLA 2018, M2 POPL 2020, SyncP POPL 2021 [all V-abs]); directed confirmation (CTrigger ASPLOS 2009, RaceFuzzer PLDI 2008, DataCollider OSDI 2010 [V-abs]; PCT ASPLOS 2010 [V-abs; mechanism P]).

---

## 2. Architecture

### 2.1 Components

| # | Component | Kind | Language | Inputs → outputs | Responsibility |
|---|---|---|---|---|---|
| C1 | `sx-plan` | **static** (probe placement + summaries only) | C++20, Clang LibTooling + LLVM DebugInfo/Object/MC | compile DB (448 FSW TUs), headers, gcc binaries (DWARF), model map M, contract library → `api.cat`, `site.cen`, `probe.plan` | API catalogue from doxygen; call-site census; site summaries; probe addresses in the **gcc** binary; probe-site cost class |
| C2 | `sx-observe` | **dynamic** | C++20 (observer), optional restricted C for eBPF programs compiled by system clang-18 | `probe.plan`, B → `trace.cft` | Root-side, out-of-process observation of the unmodified README-mode process |
| C3 | `sx-lift` | dynamic → model | C++20 | `trace.cft`, `api.cat`, M → `run.ceir` | Thread/identity resolution, resource and lifecycle reconstruction, event nesting, handler segmentation |
| C4 | `sx-analyze` | **model-based** | C++20 | `run.ceir` (one or more runs), contract library → `verdict.db`, `witness.sched` | HB (definite and robust), contract instantiation, tier-1 classification, prediction, σ classification |
| C5 | `sx-smt` | model-based | C++20 | encoding requests → SMT-LIB 2 text → external solver process | Isolated solver boundary (§4.5); Z3 is not installed (PI decision) [M] ([R07] §2.4) |
| C6 | `sx-confirm` | **confirmation** | C++20 | `witness.sched`, `probe.plan` → `confirm.cft`, confirmation records | Hold-until-event gates on the unmodified binary; perturbation accounting |
| C7 | `sx-report` | reporting | C++20 | `verdict.db` → Markdown/HTML views | Human views; never the source of truth |
| C8 | `sx-dump` | tooling | C++20 | any artifact → canonical text | Review, diffs, golden tests |

### 2.2 Typed artifacts (no ad hoc JSON)

All persistent formats share one container: little-endian, a 64-byte header (`magic`, `format version`, `schema hash`, hashes of B, M, contract library and probe plan), typed sections with explicit record sizes, a string pool, and a CRC per section. One C++ header per format defines the record structs and a reader/writer; `sx-dump` prints a canonical one-record-per-line text form for review and golden tests [D].

| Artifact | Content | Producer → consumer |
|---|---|---|
| `B.manifest` | Every element of B with value, reference and status (`from-reference` / `deviation` / `unspecified_by_reference`) per [POL] | harness → all |
| `M.model` | The cFS semantic model: event kinds, LTS per namespace, HB rules with robustness tags, API summaries, poll/retry summaries, internal-site patterns. Versioned per pinned revision; every rule cites code | authored + C1 → C1, C3, C4 |
| `api.cat` | Per API: module, parameters with direction (`[in]`/`[out]`) and role (name, id, out-id, AppId, buffer), return-code vocabulary, header file:line | C1 → C1, C3, C4 |
| `site.cen` | Every public-API call site (file:line, caller, callee, macro-expanded, intra-TU, return value used or ignored) and site summaries (enclosing retry/poll loop with its bound constant; control dependence on shared reads) | C1 → C2 (coverage), C4 |
| `contracts/*.crl` | Contract templates in the typed contract language (§7.1) | authored → C4 |
| `probe.plan` | Probe records: binary build-id, file offset, phase (entry/return/line/watch), typed fetch spec (registers, bounded dereference, string with a reference bound), semantic kind, site id, measured cost class | C1 → C2, C6 |
| `trace.cft` | Binary trace (§5.3) | C2 → C3 |
| `run.ceir` | cFS Event IR: threads, apps, resources with generations and lifecycles, events with four identities, nesting, handler ids, HB edges with rule id and robustness class, contract instances | C3/C4 → C4, C7 |
| `verdict.db` | Verdict records (§1.5) with certificates, witnesses, assumptions, β | C4 → C6, C7 |
| `witness.sched` | Predicted schedules as minimal gate constraints `hold(ctx, point) until event` | C4 → C6 |
| `confirm.cft` | Trace of a confirmation run plus gate records (hold start/end, release reason) | C6 → C3/C4 |

### 2.3 Data flow (drawable as a diagram)

1. **Plan.** C1 reads the compile DB and headers (Clang), the `link.txt` archive semantics, and the DWARF line tables and symbols of the **gcc-built** `core-cpu1` and app `.so` files (LLVM DebugInfo/Object). It writes `api.cat`, `site.cen`, `probe.plan`.
2. **Observe.** The harness launches `./core-cpu1` exactly as in ENV (user `ubuntu`, `build-native_std/exe/cpu1`, no options) [F] ([R2G] §1). C2 runs in the root session beside it and writes `trace.cft`.
3. **Lift.** C3 turns raw probe records into semantic events with identities and resources (`run.ceir`).
4. **Analyze.** C4 builds HB, instantiates contracts, classifies (tier 1), predicts (tier 2, through C5 when needed), and writes `verdict.db` and `witness.sched`.
5. **Confirm.** C6 replays each witness on the unmodified binary under hold-until-event gates and writes `confirm.cft`, which re-enters step 3.
6. **Feedback.** Coverage gaps (census sites never observed) and refuted witnesses return to C1 (new probe sites, new configuration variants) and to M (rule corrections, recorded as model revisions).

### 2.4 Static / dynamic / model / confirmation split

- **Static (C1) — only placement and summaries.** No race detection, no points-to analysis, no harness. Static results never decide a verdict alone, with one exception: `inactive(B)` requires a configuration summary.
- **Dynamic (C2, C3).** All behavioural facts come from the reference binary.
- **Model-based (C4, C5).** Reasoning over the observed traces under M and σ.
- **Confirmation (C6).** Real-binary executions under declared, event-based control.

### 2.5 Why C++ and where LLVM/Clang is used

- **Clang LibTooling** is justified by measurement: all 448 FSW TUs parse in 10 s single-threaded with two diagnostic-only flag adaptations [M] ([R07] §3.3). It supplies (a) doxygen parameter directions and return-code lists for `api.cat` — the headers carry 74 `\param[out]` and 452 `\retval` entries in cFE `core_api` and 76 / 593 in OSAL `inc`, plus 138 `\par Assumptions` blocks in cFE `core_api` [M] (§A.2); (b) the call-site census (3,762 public-API sites, 88 intra-TU, 467 in macro expansions) [M] ([R07] §4.2, §4.6); (c) site summaries (loops around lookups with `OS_TaskDelay`, ignored return values).
- **LLVM DebugInfo/Object** reads the DWARF and symbol tables of the *gcc* binary, so probe addresses come from the reference build, not from a Clang rebuild ([R06] T7). **LLVM MC** decodes the instruction at each probe site to assign its cost class, because the cost difference between probe sites is a measured 55× (0.57 µs vs 31.7 µs) [M] ([R07] §4.7).
- Everything else is plain C++20 with the standard library. The LLVM 23 build is consumed out-of-tree, pinned at `1053047a` [M] ([R07] §2.1).

### 2.6 MLIR: not used [D]

Under this angle the IR is a fact graph built from traces, not a program to lower. MLIR would add a TableGen/ODS layer and a toolchain dependency without a demonstrated benefit ([R06] T1; [v0.1] §5.2 itself says MLIR adds no detection power). SMT-LIB text is emitted directly by C5 (a small emitter), so the MLIR SMT dialect, although available in the LLVM 23 build [M] ([R07] §2.2), is not needed. If the PI wants MLIR, an adapter can consume `run.ceir` later behind the gate test of [R06] T1; the contribution does not depend on it.

---

## 3. cFS mapping

Each row gives the observed events (probe phase in brackets: `E` entry, `R` return, `L` line probe after a store, `W` watchpoint), the lifecycle/state effect, the HB rules, and the contracts. Rule names refer to §4.2. All code facts are from the cited reports.

### 3.1 OSAL

| Mechanism | Observed events | Model | HB / robustness | Contracts and notes |
|---|---|---|---|---|
| **idmap lifecycle and the RESERVED window** | `OS_ObjectIdAllocateNew` [R: type, status], `OS_ObjectIdFinalizeNew` [E: status in; R], `OS_ObjectIdFinalizeDelete` [E/R]; per-type `*_Impl` entry (marks step f, name already visible) | OSAL LTS with name-invisible / name-visible RESERVED; LIVE at FinalizeNew (`osapi-idmap.c:1090-1105`) [F] ([R01] §1.5); delete path RESERVED → FREE ([R01] §1.6) | OSAL table locks are **implementation-level** and are not synchronization (API-level HB only, [R01] §3, §6.2) | K-REG on all `OS_*GetIdByName`, `OS_TaskFindIdBySystemData`, filesystem searches. Outcome map includes `INCORRECT_OBJ_STATE`, whose code-derived consequence is the F1 lock-leak hypothesis ([R01] §1.8 [I]) |
| **Lookups** | `OS_ObjectIdFindByName`, `OS_ObjectIdGetBySearch` [E: name string ≤ `OS_MAX_API_NAME`; R: status, out id] | predicate on LTS state ([R01] §1.7) | none (lookup is a read of the key) | outcome is the oracle; no contract authoring needed to see an anomalous code |
| **Task create / start** | creator: `OS_TaskCreate` [E: name, entry; R: status, out id]; child: `OS_TaskEntryPoint` [E: osal id] in the child's own thread | task resource RESERVED → LIVE; binds child TID ↔ OSAL id ([R01] §2.2) | **FORK** from creator's events up to `OS_TaskCreate` entry → child's first event (robust) ([R01] §2.2 HB edge 1–2) | K-SPAWN; bounded child wait (≤4 waits, fail on the 5th, `osapi-idmap.c:475-480`) modeled as a poll with a fatal-to-task escape (F7, [R01] §2.2) |
| **Task delete / exit** | `OS_TaskDelete` [E/R], `OS_TaskExit` [E] | RESERVED_d → FREE; generation ends | **JOIN**: victim's last event → deleter's post-return events (robust) | K-LIFE (use after teardown) |
| **Mutex (recursive)** | `OS_MutSemTake` [E], `OS_MutSemGive` [E] | per-thread recursion depth | **LOCK**: outermost give → next outermost take completion on the same id+gen; *schedule-chosen* (non-robust) ([R01] §3; [R02] R4) | locks give atomicity; order only together with a state check (§4.2 OBS) |
| **Binary semaphore** | `OS_BinSemGive` [E], `OS_BinSemFlush` [E], `OS_BinSemTake`/`TimedWait` [E; R: status] | value saturates at 1 (gives coalesce); flush releases waiters without consuming ([R01] §3) | **BINSEM**: definite edge only from gives whose return precedes the take's entry; flush → flushed takes; timeout → none; coalesced gives encoded precisely only in SMT | K-HANDOFF (timer/ISR → task, RC10) |
| **Counting semaphore** | `OS_CountSemGive` [E/R], `OS_CountSemTake`/`TimedWait` [E; R: status] | count | **CNTSEM**: definite edges from gives completed before the take started; the disjunctive "some overlapping give" is encoded only in SMT | K-REG for named lookup (C1); K-HANDOFF (FM child, RC9) |
| **Queue (with drop)** | `OS_QueuePut` [E: id, 8-byte descriptor value; R: status incl. `OS_QUEUE_FULL`], `OS_QueueGet` [R: status, descriptor] | depth per queue; capacity = `min(requested, msg_max)` measured per run [F] ([BM] §3.1) | **MSG**: put(d) → the get that dequeued d (d unique while in flight); FIFO per queue; **no edge for a failed put** ([R01] §3; [R02] R7) | K-CAP; queue full is silent to the SB sender ([R03] §1) |
| **Timebase / timer callbacks** | `OS_TimerAdd`, `OS_TimerSet` [E: callback, arg; R]; `OS_TimeBase_CallbackThread` [E: timebase id]; callback function entries (addresses learned from `OS_TimerAdd` arguments) | callback attributed to the timer's creator; timebase thread is its own context ([R01] §2.3, §3) | **TIMER**: TimerAdd/Set return → later callbacks (through `handler_mutex`); callback k → k+1 by timebase-thread PO | K-HANDOFF (SCH_LAB timer → main task, RC10, `sch_lab_app.c:143-146`) |
| **Modules / symbols** | `OS_ModuleLoad` [E: path; R], `OS_SymbolLookup`, `OS_ModuleSymbolLookup` [E: name; R: status, address] | loader namespace | load → successful lookup (observe-then-act) | K-REG(ns=loader) (RC4: MD, MM, SBN) |
| **Files** | `OS_OpenCreate`, `OS_close`, `OS_remove`, `OS_rename`, `OS_FileOpenCheck`, `OS_ForEachObject` [E/R] | path-keyed resource | none (data via the filesystem) | K-FILE (RC12; CF's TOCTOU comment `cf_cfdp_s.c:447-463`) |
| **Console / `OS_printf`, `OS_TaskDelay`** | not synchronization; `OS_TaskDelay` [E] recorded as a timing marker only | — | **none** (implementation-level or timing only, [R01] §3, §6.2 item 4) | delay loops are inputs to poll/retry summaries |
| **Condvar, rwlock** | not probed: no non-test caller [F] ([R01] §0 item 6) | listed as *unmodeled-absent*; any call observed raises `unknown(unmodeled)` | — | — |

### 3.2 PSP

- **Startup** runs single-threaded on the main thread until `CFE_ES_Main` creates tasks (`cfe_psp_start.c:352-449`) [F] ([R01] §5.1). PO plus FORK make every PSP-created object (e.g. timebase `cFS-Master`, `cfe_psp_soft_timebase.c:53`) robustly ordered before every cFE and app lookup of it (`cfe_time_task.c:311`, `sch_lab_app.c:176`) [I] ([R01] §4) → `protected` with a robust certificate. PSP events probed: `CFE_PSP_ModuleInit` [E/R], `CFE_PSP_InitProcessorReservedMemory` [E: reset type], `CFE_PSP_Restart` [E].
- **Persistence** belongs to B: S0 is recorded before every run (the [BM] §2.5 snapshot method), and the observed reset type is an event. Cross-run producer→consumer pairs (CDS written in run n, restored in run n+1 on a processor reset) are linked by `(ns=CDS, name)` across two traces (FCatch-style fault-aware ordering, ASPLOS 2018 [V-abs]) — K-PERSIST, phase 2 (§9).
- **Exception ring and SIGUSR1 to the idle thread** ([R01] §5.4): modeled as a signal edge from the faulting context to the idle thread; out of first scope (no exception in the target runs); listed as unmodeled.

### 3.3 ES

| Mechanism | Events | Rule / class | Contracts |
|---|---|---|---|
| **Core startup serialization** (order from `MISSION_CORE_MODULES` via the generated `cfe_static_module_list.c`, [R02] §1.3) | ES startup thread: EarlyInit calls [E/R]; `CFE_ES_StartAppTask` [E/R]; `CFE_ES_MainTaskSyncDelay` (a local symbol, probe-able from `.symtab` [M] §A.2) [E: required state, timeout; R: status]; core record publish `cfe_es_start.c:818` [L] | **OBS-FATAL**: core-phase sync with panic on timeout (`cfe_es_start.c:835-846`) → robust (§4.2) | K-ID (service identities), K-SPAWN; R2 is decided here (§4.3 worked example) |
| **Phases and soft barriers** | `SystemState` stores `cfe_es_start.c:87,183,194,216,234` [L, value]; `CFE_ES_WaitForSystemState` [E: MinState, timeout; R: status]; `CFE_ES_WaitForStartupSync` is void, so its exit reason is taken from the inner `WaitForSystemState` return [F] ([R02] §1.7) | **POLL-COND**: the first store with value ≥ Min → waiter's return **only if** status = success; timeout → no edge; soft escape → non-robust; assumption A-VIS (§6) | K-PHASE (RC7): declaration ≠ readiness (SBN declares before init, CS after failure, STOPPED counts as past the barrier, [R03] §1, §4) |
| **App state** | writes in `WaitForSystemState` (`cfe_es_api.c:571-574`), `RunLoop` (`:469-472`), `ExitApp` (`:399`), background scan (`cfe_es_apps.c:1139-1146`) [E of the API + L where needed] | under the ES lock → LOCK edges; the startup-phase `MainTaskSyncDelay` is **OBS-SOFT** (1000 ms, log-only escape) | K-PHASE |
| **Startup-script apps** (dlopen/dlsym and the trampoline) | `CFE_ES_ParseFileEntry` [E], `CFE_ES_AppCreate` [E: name; R], app record RESERVED `cfe_es_apps.c:821` [L] and publish `:869` [L], task record publish `:703` [L], `OS_ModuleLoad` / `OS_ModuleSymbolLookup` [R], `CFE_ES_TaskEntryPoint` [E], `CFE_ES_GetTaskFunction` [R: status] | Script order orders **creation only** (startup-thread PO, [R02] §1.5 R3); library init runs synchronously before later entries (robust, RC11); `GetTaskFunction` is a poll with at most 21 checks (1000 ms / 50 ms + 1, `cfe_es_apps.c:570-595`) [M] (§A.2) | K-SPAWN, K-REG(ns=ES app): the RESERVED window between `:722` and `:869` ([R02] §1.6) |
| **Child tasks** | `CFE_ES_CreateChildTask` [E/R], `CFE_ES_DeleteChildTask`, `CFE_ES_ExitChildTask` | FORK; parent App record already LIVE ([R02] §1.10) | K-HANDOFF (RC9) |
| **Restart / reload / delete teardown** | `CFE_ES_RestartApp`/`ReloadApp`/`DeleteApp` [E]; scan transitions [L]; `CFE_ES_CleanUpApp` [E/R]; module cleanups `CFE_EVS_CleanUpApp`, `CFE_SB_CleanUpApp`, `CFE_TBL_CleanUpApp`, `CFE_TIME_CleanUpApp` [E]; task deletes; `OS_ModuleUnload` | Control request via ES lock (R15); **timer-driven kill** (5 scans × 1000 ms, `cfe_es_internal_cfg.h:199,229`) gives no edge from the app ([R02] §1.11); generations advance | K-LIFE (RC15: stale CF sem id, MD address, SBN symbols; TIME callback after unload, [R02] §5.3). Requires inputs I (a restart command); not in default B |
| **Generic counters, CDS** | `CFE_ES_RegisterGenCounter`, `GetGenCounterIDByName`; `CFE_ES_RegisterCDS(Ex)` [R: status incl. `ALREADY_EXISTS`], `GetCDSBlockIDByName` | name-addressed, lock-atomic | K-REG, K-PERSIST (RC8) |

### 3.4 SB

| Mechanism | Events | Rule | Contracts |
|---|---|---|---|
| **Pipes** | `CFE_SB_CreatePipe` [E: name, depth; R: status, out PipeId], `CFE_SB_DeletePipe`, `CFE_SB_GetPipeIdByName` [E/R] | two-registry publication (OSAL queue before SB descriptor) | K-REG(ns=SB pipe) |
| **Subscribe-before-send** | `CFE_SB_Subscribe*` / `Unsubscribe*` [E: MsgId, PipeId; R] | route state `NO_ROUTE → ROUTED(pipe)` per `(MsgId, pipe)`; a subscribe is a **state transition, not an edge** (R8, [R02] §2.7) | K-ROUTE (RC5): 48 subscribe / 41 transmit sites, 6 table-driven subscribers [F] ([R03] §4) |
| **Transmit and drop semantics** | `CFE_SB_TransmitMsg` / `TransmitBuffer` [E: MsgId read from the message header]; per-destination `OS_QueuePut` [E/R]; no-subscriber branch `cfe_sb_priv.c:1083-1086` [L]; MsgId-limit branch `:1058-1064` [L] | outcome per destination: delivered / queue full / limit / no route | K-ROUTE, K-CAP (RC14) |
| **Receive** | `CFE_SB_ReceiveBuffer` [E: PipeId, timeout; R: status, descriptor → MsgId] | **MSG** edge only for delivered descriptors; FIFO per pipe; no reverse edge (R7) | looper segmentation (§4.2 LOOP) |
| **Global mode** | SB subscription-reporting flag write [L], triggered by SBN's command ([R03] §2.15) | global mode resource | K-MODE (RC13): the set of effects of later subscribes depends on the mode; measured as run-varying `SBNSubPipe` overflow ([R03] §5.2) |

### 3.5 EVS, TBL, CDS, TIME

- **EVS registration and identity.** `CFE_EVS_Register` [E/R]; `EVS_AppDataSetUsed` store `cfe_evs.c:106` [L]; EVS identity publish `cfe_evs_task.c:312` [L]; `CFE_EVS_SendEventWithAppID` [E: AppID = stamp; R: status]; `CFE_EVS_SendEvent` [R]. Contract K-ID from the header precondition `cfe_evs.h:168-172` (§7.1). The EVS guard (`cfe_evs_utils.c:613-623`) turns early internal events into silent drops → `tolerated` only if a reference allows the drop; otherwise reported as "guarded early use" ([R02] §3.4).
- **TBL.** `CFE_TBL_Register` [E: name; R: status, handle], `Share` [E: name; R], `Load`, `Validate`, `Manage`, `Update`, `GetAddress` / `ReleaseAddress` [R: status incl. `NEVER_LOADED`, `ERR_UNREGISTERED`]. Registry-mutex edges are LOCK edges; the validation result flags (`ValidationResults.State`) are an unlocked handshake (R13) → modeled as an *assumed* edge under A-VIS and flagged ([R02] §4.4). Contract K-TBL (RC3: CS shares `SAMPLE_APP.ExampleTable`, latent). The owner-load vs ground-load check-then-claim candidate ([R02] §4.4) is an atomicity question: it is encoded as two claims on `LoadInProgress` and predicted like any lifecycle conflict.
- **CDS.** Registration under the CDS mutex is lock-atomic and idempotent by name; restore depends on reset type (part of B) → K-PERSIST.
- **TIME.** `CFE_TIME_RegisterSynchCallback` [E: function pointer], `CFE_TIME_NotifyTimeSynchApps` [E] and callback entries (dynamic addresses) — callbacks run in TIME's context (owner = app, exec = TIME task) ([R02] §5.3). The 1 Hz chain (timer callback → binsem → tone tasks → SB) uses TIMER, BINSEM and MSG edges ([R02] §5.1). The reference-state seqlock (R14) is **unmodeled in the first scope**: no first-scope contract depends on it ([R02] §5.4); if an ordering path would need it, the verdict is `unknown(unmodeled: TIME seqlock)`.

### 3.6 The 15 readiness-contract kinds of [R03] §4

| RC | Kind ([R03]) | Template | Observable oracle | Default-config status | Coverage |
|---|---|---|---|---|---|
| RC1 | Named OSAL/PSP object lookup | K-REG(ns=OSAL, PSP device) | lookup status | SCH_LAB timebase active; CF sem latent (C1); HS device fails here | **covered** (C1 needs a derived configuration) |
| RC2 | Named ES lookup at steady state | K-REG(ns=ES) | `GetAppIDByName` status / `RESERVED` value | HS XCT active | **covered** |
| RC3 | Named table sharing | K-TBL | `Share` status `ERR_INVALID_NAME` | latent (CS) | covered when a variant activates it |
| RC4 | Symbol / module by name | K-REG(ns=loader) | `OS_SymbolLookup` status | SBN active; MD/MM latent | **covered** |
| RC5 | Subscription before first send | K-ROUTE | no-subscriber branch, per-destination puts | active (SCH_LAB→18 consumers, →TO_LAB 37, →DS 14, →HK 5) | **covered** |
| RC6 | Identity / registration publication | K-ID, K-SPAWN | `ILLEGAL_APP_ID` / `NOT_REGISTERED`; `ERR_RESOURCEID_NOT_VALID` | active in core (R2) | **covered** |
| RC7 | Phase barrier / readiness declaration | K-PHASE | wait return status; app-state writes | 5 sites in 4 apps | **covered** (C2 needs a role-derived configuration) |
| RC8 | Persistent restore across reset | K-PERSIST | `ALREADY_EXISTS`, restore status | power-on: create path only | covered in phase 2 (needs two-boot traces, §9 M8) |
| RC9 | Parent ↔ child handoff | K-HANDOFF | FM queue counters, sem outcomes | FM active | **covered** for the order part; the unlocked count read (`fm_child.c:162`) is a memory-level race → **out of scope** (memory class, [R02] §8.4 item 7) |
| RC10 | Timer/ISR callback → task | K-HANDOFF + TIMER | sem outcomes | SCH_LAB active | **covered** |
| RC11 | Library before client | (robust PO certificate) | — | guaranteed | **covered** as `protected` with certificate |
| RC12 | File-object handoff | K-FILE | `OS_FileOpenCheck` / iterator results | latent (commands) | covered when inputs I trigger it; harm needs file semantics → harm `unknown` |
| RC13 | Global SB mode change | K-MODE | mode write, ONESUB transmissions | active (SBN, DS) | **covered** |
| RC14 | Queue capacity vs burst | K-CAP | `OS_QUEUE_FULL` per put; depth reconstruction | active (3 pipes overflow) | **covered** |
| RC15 | Stale reference after restart | K-LIFE | generation mismatch, `ERR_UNREGISTERED` | needs restart inputs | covered when I includes restarts; otherwise `unobserved` |

No kind is dropped silently. The only scope cut is RC9's memory-level counter read, because it is a data race, not a readiness contract.

---

## 4. Algorithms

### 4.1 Candidate generation

Candidates come from four sources, all generic:

1. **Template instantiation.** Every observed event matching a template's use pattern U creates an instance κ; its producers are the lifecycle transitions of the bound resource r in the same run (def-use on the resource key, DefUse-style [V-abs]).
2. **Outcome mining.** Every observed not-ready outcome code at the API boundary (the vocabulary comes from `api.cat`) creates an instance even when no template names that API; it is labelled `template: none` and enters triage. This finds contracts nobody wrote down.
3. **Key-pair enumeration.** For every resource key with both transitions and uses across different contexts, the (transition, use) pairs that M marks as conflicting are candidates, whether or not a template exists.
4. **Configuration closure.** `site.cen` lists every census site of a templated API. Sites never observed are reported as `unobserved` (with the census denominator), or as `inactive(B)` when a configuration summary proves it. Configuration variants that activate latent paths (tables, schedule, script) are new bases B′, never "new bugs" ([R06] §5.4).

### 4.2 Ordering analysis: the cFS-aware HB

**Definite HB (≺).** The transitive closure of the following rules over T. Each rule carries a **robustness tag**: *unconditional* (the edge exists in every execution in which both endpoints execute), *observe-then-act with fatal escape*, *observe-then-act with soft escape*, *schedule-chosen*, or *assumed* (holds only under an explicit assumption).

| Rule | Edge | Tag | Source |
|---|---|---|---|
| PO | program order in a context, except inside looper regions (LOOP) | unconditional | Lamport [V-abs] |
| FORK | creator's events up to `OS_TaskCreate` entry → child's first event | unconditional | [R01] §2.2; [R02] R2 |
| JOIN | victim's last event → deleter after `OS_TaskDelete` returns | unconditional | [R02] R17 |
| LOCK | outermost release → next outermost acquire of the same mutex id+gen | schedule-chosen | [R02] R4 |
| BINSEM, CNTSEM | definite give → take edges as in §3.1 | schedule-chosen | [R01] §3 |
| MSG | delivered put(d) → get(d); FIFO per queue; none for failed puts | schedule-chosen (delivery) | [R02] R7; DCatch Rule-Eenq [P] |
| TIMER | TimerAdd/Set return → later callbacks | unconditional once both occur | [R01] §3 |
| OBS-FATAL | a poll that exits only by observing state s, whose escape is panic or prevents the later event: the write that established s → the poll exit | **unconditional** relative to the guarded continuation | [R02] R19, R18 |
| OBS-SOFT | a poll with a log-and-continue escape: the write → the poll exit **only if** the exit was by condition | soft (conditional) | [R02] R5; [R04] L3 |
| POLL-COND | `SystemState` store ≥ Min → `WaitForSystemState` success return | soft + assumed (A-VIS) | [R02] §1.7; [R06] T8 |
| ASSUMED-FLAG | unlocked flag handshakes (TBL `ValidationResults.State`, FS `IsPending`) | assumed (A-VIS), flagged | [R02] R13, §6.1 |
| LOOP | inside an app main loop, PO is kept only *within* one handler (from a `ReceiveBuffer` return to the next `ReceiveBuffer` entry); handlers on the same task are ordered only by FIFO on the same pipe, by an HB path from one handler's end to the other's triggering put, or by timeout handlers' PO | — | CAFA [V-abs]; DCatch Rule-Pnreg [P] |

Two vector clocks are maintained per event (Fidge/Mattern [V-abs]): **VC** over all definite rules, and **VC_R** over *unconditional* rules only (PO outside looper relaxations, FORK, JOIN, TIMER, OBS-FATAL). `a ≺_R b` (robust order) iff `VC_R(a) ≤ VC_R(b)` componentwise.

**Rule coverage report.** Every synchronization primitive observed is matched to a rule; anything else is listed as *unmodeled* (the KCSAN candour rule, [R04] L2). An unmodeled mechanism never yields `protected`; it yields `unknown` on the affected pairs.

**API level, not implementation level.** OSAL table locks, the console lock and the binsem internal mutex are excluded as synchronization ([R01] §3). Including them is an ablation (§8.5).

### 4.3 Contract checking on ordered and unordered pairs (tier 1)

For each instance κ = ⟨K, u, r⟩ and each producer transition p that changes r into or out of Req:

| VC relation | Tier-1 result | Next step |
|---|---|---|
| `p ≺_R u` (robust, p makes r ready) | `protected` candidate with a robust certificate | done (subject to path stability) |
| `u ≺_R p` (robust, u happens before the readiness transition) | `fixed-order` candidate with a robust certificate | done; no exploration needed |
| `p ≺ u` or `u ≺ p` only via non-robust edges | ordered in T, reversibility unknown | tier 2: is the reversal feasible? |
| `p ∥ u` (unordered) | race-like candidate | tier 2: are both orders feasible? |

**Certificate.** A robust certificate is an explicit chain of cited rule instances. **Worked example, R2 under σ_U** (from code; consistent with [R06] T5 and the HB chain rebuilt in [R2C] §4.3): ES-task uses inside `CFE_ES_TaskInit` →PO the ES task's `WaitForSystemState(CORE_READY)` app-state write (`cfe_es_task.c:134`; `cfe_es_api.c:571-574`) →OBS-FATAL the ES startup thread's successful `MainTaskSyncDelay` (`cfe_es_start.c:835`, panic on timeout `:838-846`) →PO `CFE_ES_StartAppTask` for SB (`:810`) →FORK the SB task →PO the store at `cfe_sb_task.c:138`. Every edge is unconditional, so `u ≺_R p` and R2's ES and EVS uses are **fixed-order under σ_U** (and therefore under σ_ENV and σ_RT1). The certificate also covers the 17 `SEND_NO_SUBS` uses, which form a **violation chain**: a K-ROUTE not-ready send produces the event whose stamped identity violates K-ID ([R2G] §2; [R06] §3 v0.1 §8). Chains are recorded as links between instances whose use events nest on the same call stack.

**Path stability.** A robust certificate claims "in every execution where these events execute". That inherits an assumption: the same per-thread paths lead to u and p. `site.cen` supplies, for each probed site, whether reaching it is control-dependent on a shared read; where it is not, the assumption is discharged; otherwise it is listed in the verdict ([D]; the static part stays a site summary).

**Ordered pairs are first-class.** Tier 1 runs on *all* instances, so wrong-way ordered violations (R2) are found without any race reasoning ([R04] G2).

### 4.4 Predictive feasibility of reversed orders (tier 2)

**Predictive model.** `Pred_{M,σ,β}(T)` is the set of executions ρ such that: (a) each context's projection of ρ is a prefix of its projection in T, optionally ending in one *divergent* event whose outcome differs from T (the control-flow abstraction of RVPredict [V-abs] and the maximal causal model [V-abs]); (b) ρ respects M's synchronization semantics (mutual exclusion per OSAL mutex, semaphore counts and coalescing, queue FIFO and capacity, fork/join, barrier predicates); (c) every event that is followed by further events of its context keeps its observed outcome; (d) σ's constraints hold (none extra for σ_U; priority constraints for σ_RT1); (e) **summary-guided extrapolation**: a call site inside a retry or poll loop (from `site.cen`) may be extended by further iterations up to its **reference bound** (e.g. CF: 25 lookups, `cf_internal_cfg.h:173`; ES `GetTaskFunction`: 21 checks; OSAL child wait: 4 waits; `WaitForSystemState`: `ceil(timeout / 50 ms)` polls), each iteration repeating the observed lookup with a not-ready outcome. Without (e), a single observed successful lookup could never be shown to exhaust its retries.

**Engines, cheapest first [D].** Each is applied to an abstract trace in which lifecycle transitions are writes and uses are reads of a per-resource variable, OSAL mutex operations are acquires/releases, and API operations are atomic except where M splits them (creation into reserve and finalize). The soundness of each engine transfers to this abstraction only if the abstraction is faithful **[I]**; this is assumption A-API (§6).

1. **P1 — schedulable unordered pairs (SHB, OOPSLA 2018 [V-abs]).** For `p ∥ u`, SHB identifies exactly the schedulable races with a linear-time vector-clock algorithm; a schedulable pair yields a predicted reversal without a solver.
2. **P2 — lock-order-preserving reversals (WCP, PLDI 2017; SyncP, POPL 2021 [V-abs]).** For pairs ordered only through locks whose critical sections do not conflict, WCP (linear time) and SyncP (near-linear, sound and complete for sync-preserving races) decide reversibility without reversing lock order. M2 (POPL 2020 [V-abs]) is complete for two-thread traces and is used when the relevant slice has two contexts.
3. **P3 — reversals that need lock reversal, counting, capacity or extrapolation (SMT).** Many readiness reversals *are* lock-order reversals (C5-style registration vs lookup under the ES lock), which SyncP's class excludes by definition; beyond sync preservation the problem is NP-complete and W[1]-hard (SyncP [V-abs]; [R04] L5). P3 encodes the window as an order-variable formula in the style of Alglave et al.'s partial-order encoding (CAV 2013 [R05 V-abs]) and Wang et al.'s symbolic predictive analysis (FM 2009 [R05 V-abs]), with properties as constraints in the GPredict style [V-abs]: integer order variables per event, PO and FORK/JOIN constraints, pairwise mutual exclusion of critical sections on the same mutex, semaphore matching variables, per-queue FIFO and capacity counters (a put that would exceed capacity becomes a drop; drop outcomes are free because the SB sender does not observe them, [R03] §1), the LTS state at each use, and the contract predicate.

**Window.** The encoding contains the events between the latest common robust predecessor and the earliest common robust successor of {p, u}, plus the **synchronization skeleton** of every context in that interval (all sync events, never sliced; [R06] T4). The window is defined by HB, not by a constant.

**Two queries per instance (classification split).** `Q_viol = SAT(Φ ∧ ¬Ready(κ) ∧ outcome ∉ Allow)` and `Q_conf = SAT(Φ ∧ Ready(κ) ∧ u executes)`. The observed trace already decides one of them. This is the two-query split of [R05] §2.3, applied over `Pred(T)` instead of a code model.

### 4.5 Classification

| Q_viol | Q_conf | Class |
|---|---|---|
| SAT | SAT | `schedule-dependent` (sides labelled observed / predicted) |
| SAT | UNSAT | `fixed-order` relative to Pred(T); upgraded to Exec-level only with a robust certificate |
| UNSAT | SAT | `protected` relative to Pred(T); with a robust certificate where one exists |
| SAT only through an OBS-SOFT / POLL-COND escape | — | `timeout-dependent` |
| budget exhausted | — | `unknown(budget, β)` |

- Under σ_RT1, P3 adds priority constraints: on one core, a ready higher-priority context runs before a lower one. Ready/blocked intervals come from the trace (entries and returns of blocking calls). This decides the C3 class as a model-only verdict.
- Under σ_ENV, the class is the σ_U class plus the observed frequency over N runs (§8.6).
- `fixed-order` is never inferred from repeated failing runs ([R06] S9; [v0.1] §2.3).

### 4.6 Directed confirmation on the real binary

**Goal.** For each predicted side of a `schedule-dependent` or `timeout-dependent` verdict, produce an execution of the **unmodified** binary in which the predicted order occurs and the contract oracle is observed. RacePro's replay-and-go-live, CTrigger's minimal perturbation, RaceFuzzer's directed scheduling and DataCollider's stall-at-the-point are the precedents ([R04] §2.4, §2.9, §2.10).

**Gate derivation.** From a witness ρ, compute the *critical reversals* (pairs whose order in ρ differs from T). Each becomes a gate `hold(ctx_p, point_p) until event(u)`: the thread that must come later is held at a point before its first event of the pair until the other side's event (or its outcome) is observed. Policy: **hold until an event**, never "sleep for a constant" ([POL] tool-parameter rule).

**Gate-placement rule** (from the trace's per-context held-lock sets, [R06] §3 v0.1 §10):

1. the gate point must be probe-able and precede the held event in PO;
2. the held context must hold **no program lock** at the gate point (otherwise move the gate earlier, before the acquisition);
3. holding inside the OSAL RESERVED window is allowed only as an explicit experiment, because it changes other lookups' outcomes (bounded wait, F1) ([R01] §1.8, §6.3).

**Mechanism [D].** The controller attaches to the running process as a debugger (the same OS facility gdb used in [R2G]) and arms an execution breakpoint *in a debug register* at each gate point, so the binary's text is not modified (x86 has 4 debug-address registers per thread [M] ([R07] §4.7)). When the thread reaches the point, it stops; the observer's uprobes keep recording all other threads; the controller releases the thread when the awaited event appears in the trace stream. Measured unit costs: a debug-exception trap ≈ 34 µs; a gdb breakpoint round trip ≈ 203 µs [M] ([R07] §4.7). The controller's own stop/resume cost is **unmeasured** and is an M6 exit item. **[I]** Interactions to measure before trusting results: debugger stops on signals, the timebase thread's real-time signal, and the OSAL bounded waits that real time drives.

**Watchdog.** If the awaited event does not occur, the hold is released at a bound taken from the reference that governs the held party (e.g. the 1000 ms startup-script timeout for a held new task, `cfe_es_internal_cfg.h:879`; `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` 30000 for the whole startup, `default_cfe_core_private_internal_cfg.h:68`) [F] ([R02] §0), and the result is `inconclusive(await-timeout)`.

**Outcomes.** `confirmed` (order realized, violation observed by the oracle), `refuted` (order realized, outcome conforming → the model or an API summary is wrong; the witness and trace go back to M), `inconclusive` (gate unsatisfiable, watchdog fired, or perturbation beyond declared limits).

**Why a held thread gives a valid execution [I].** Holding a thread is indistinguishable, to the program, from the OS not scheduling it; `SCHED_OTHER` gives no progress bound. A confirmed execution is therefore in Exec(B, σ_U) and in Exec(B, σ_ENV). It says nothing about frequency. If a hold outlasts a program timeout of another party, the run is flagged `timeout-influenced`.

**PCT arm (secondary).** PCT gives a per-run bound `≥ 1/(n·k^(d−1))` for depth-d bugs (ASPLOS 2010 [V-abs; mechanism P]). Natively it needs strict priorities on one CPU, which the README-mode user cannot set; root can impose `SCHED_FIFO` and affinity on the process from outside [M] ([R07] §5.1–5.2). That arm deviates from E1/E4 and is a PI decision (§10). n and k are measured per run; d is swept, never fixed (SCTBench norm, [R05] §2.1).

**Fixed-order verdicts** need no confirmation run (the violation is observed). A *conforming* counterpart exists only under a changed basis B′ (e.g. a different `MISSION_CORE_MODULES` order), which is reported as a configuration experiment ([R06] §3 v0.1 §10).

### 4.7 Complexity and bounds

| Step | Cost | Bound source |
|---|---|---|
| Lifting | O(N) for N records | — |
| VC and VC_R | O(N·C) time and space, C contexts (27 threads measured, [BM] §0) | — |
| Tier 1 | O(C) per (u, p) pair; pairs ≤ Σ_u |P(r(u))| | — |
| P1 (SHB), P2 (WCP) | linear in N (as published) | [V-abs] |
| P2 (SyncP) | near-linear (as published) | [V-abs] |
| P3 (SMT) | NP-complete in general; encoding size O(w + Σ_l cs_l²) for window size w and cs_l critical sections per mutex in the window | SyncP hardness [V-abs] |
| Extrapolation | ≤ the reference retry/poll bound per loop: CF 25 (`cf_internal_cfg.h:173`); `GetTaskFunction` 21 (`cfe_es_apps.c:570-595`, cfg `:860,:879`); OSAL child wait 4 (`osapi-idmap.c:475-480`); `MainTaskSyncDelay` 30000/50 core, 1000/50 startup (`cfe_es_start.c:835,210,225`); `WaitForSystemState` `ceil(timeout/50)` (`cfe_es_api.c:584-605`) | references cited |
| Capacity | `min(requested, msg_max)` per queue, measured from the process (the [BM] §2.2 method) | [BM] §3.1 |
| String fetch | `OS_MAX_API_NAME` 20, `OS_MAX_PATH_LEN` 64, `CFE_MISSION_MAX_API_LEN` 20 | built headers [M]; [BM] §2.4 |
| Solver budget | `unspecified_by_reference` → **swept**; every result is reported as a function of budget; an exhausted budget gives `unknown`, never `protected` | [POL]; [R05] §6.5 |
| PCT depth d | swept | [R05] §2.1 |
| Confirmation | one controlled boot per witness; ≤ 4 gate points per thread (debug registers) | [R07] §4.7 |

---

## 5. Observation and instrumentation design

### 5.1 Mechanism choice

| Mechanism | Fidelity ([R07] §7) | Unit cost [M] ([R07]) | Coverage | Role in Sextant |
|---|---|---|---|---|
| **uprobes on the gcc binary, root side** | **F0** (binary and run conditions unchanged; timing perturbed) | 0.57 µs at entry+4 (emulated `push %rbp`); 31.7 µs at entry+0 (`endbr64`) or other single-stepped instructions | Callee side: catches **intra-TU calls and static functions** (symbols present: the binary is not stripped, [R07] §4.1), which `--wrap` misses (88 / 3,762 sites, 2.3%) | **Primary** [D] |
| eBPF programs on those uprobes | F0 | not separately measured | bounded user-memory reads (names, out-pointer targets), in-kernel sequence tokens, per-thread loss counters | preferred backend; needs `libbpf-dev` (apt candidate 1:1.3.0 [M] §A.2) or a minimal own loader → PI decision |
| tracefs dynamic uprobe events (fetch-args) | F0 | same trap path | fixed-format fetches (registers, bounded dereference, strings, return value); tracefs is mountable in a private mount namespace [M] ([R07] §4.7) | **zero-install fallback backend** |
| HW watchpoints | F0 | ≈ 34 µs per hit; 4 per thread | data accesses by any code, including direct global reads | targeted discovery (§7.3) and gate points (§4.6) only |
| sched / syscall tracepoints | F0 | low (not measured) | clone parent→child, syscalls | FORK cross-check; the RacePro-style baseline (§8.4) |
| `ld --wrap` | F1 (+link-option deviation) | a few ns + handler | cross-module calls only | **not used** in this angle |
| source probes, `-finstrument-functions` | F2 / F1 | +15.7% `.text` for the latter | — | not used |
| gdb | F0 | ≈ 203 µs per breakpoint; R2 OPERATIONAL 2244–2693 ms vs 1613–1636 ms | — | the historical baseline method ([R2G] §1) |

**Probe-site rule [D].** Every function in the reference binary starts `endbr64; push %rbp; mov %rsp,%rbp` at `-O0`; this was checked for nine core functions and one app entry (§A.2). Entry probes go at **entry+4**. Every other site (return points, line probes) is classified by C1's instruction decoder; a site class without a measured cost on this kernel is micro-benchmarked before use (as [R07] did) and its cost class is written into `probe.plan`. Return-probe cost was **not measured** in [R07]; measuring it is an M1 exit criterion.

**Probe safety.** uprobes execute in the kernel: they call no OSAL function, take no user lock, never touch the console, and cannot be cancelled mid-record. This removes the hazards listed in [R01] §6.3 (self-deadlock on NORMAL table mutexes, console serialization, cancellation with a held buffer lock) by construction. The remaining effect is a delay of 0.57 µs (or ~32 µs at single-stepped sites) per hit, which matters inside OSAL table locks only if it approached the 10–160 ms bounded-wait steps ([R01] §1.3) **[I]**.

### 5.2 Buffers and loss accounting

- **Per-CPU kernel buffers, per-thread streams [D].** Probe output goes to per-CPU perf buffers (no cross-CPU lock inside the target, satisfying the rationale of [v0.1] §9.2). The observer drains them and demultiplexes by TID into per-thread streams.
- **Per-thread sequence tokens.** The eBPF backend assigns each record a gapless per-thread sequence number (`tseq`) in kernel at the probe instant. Per-thread order is then exact without clocks, and a gap identifies a lost record exactly.
- **Loss.** Kernel lost-record notices are counted per CPU; eBPF output failures are counted per thread. A gap in any context marks every instance that needs an event from that interval `unknown(loss)`. Loss is never treated as absence.
- **Buffer size** is set from the measured high-water mark of M1 runs, not chosen in advance ([v0.1] §11.2; [POL]). For scale: the narrower CORD recorder logged 626–632 events per boot with no drops [F] ([R2C] §4.1).

### 5.3 Event record format (`trace.cft`)

| Field | Bytes | Meaning |
|---|---|---|
| `tid` | 4 | kernel thread id |
| `tseq` | 4 | per-thread sequence (eBPF backend; 0 = absent in the fallback backend) |
| `t_ns` | 8 | `CLOCK_MONOTONIC` time — diagnostic and fallback ordering only |
| `probe_id` | 2 | index into `probe.plan` (site, phase, semantic kind, fetch spec) |
| `phase` | 1 | ENTRY / RETURN / LINE / WATCH / SCHED / GATE |
| `flags` | 1 | payload present, token valid, string truncated, lost-before |
| `obj_seq` | 4 | per-object sequence token (§5.5); 0 = none |
| `a[3]` | 24 | fetched values typed by the fetch spec (arguments, return value, out-pointer target) |

48 bytes, naturally aligned. Strings (names, paths) go to a side pool keyed by (tid, tseq), bounded by the reference constants of §4.7. The file header carries B, plan and backend hashes, the clock id, the CPU count and the thread table (TID → kernel thread name read from `/proc/<pid>/task/<tid>/comm`, first/last seen, clone parent) [D].

### 5.4 Bootstrapping identity without `GetAppID`

No identity comes from cFE services; everything is derived from events the program performs anyway ([v0.1] §9.2; [R06] F10):

1. **TID ↔ OSAL task.** The child's own `OS_TaskEntryPoint(task_id)` entry carries the OSAL id, and the record carries the TID. This binding happens in the child before any cFE call ([R01] §2.2).
2. **Creator ↔ child.** `OS_TaskCreate` (creator side: name, entry; return: out id) matched by OSAL id within the run; cross-checked by the clone tracepoint.
3. **Task ↔ app (owner).** ES task-record publish (`cfe_es_apps.c:703`) and app-record RESERVED/publish (`:821`, `:869`) line probes; core records at `cfe_es_start.c:803`, `:818`.
4. **Raw contexts.** Main thread (TID = PID) = PSP main / ES startup thread; timebase thread from `OS_TimeBase_CallbackThread(timebase_id)`; console thread from its creation path or kernel thread name. The PSP sets kernel thread names to the task name truncated to 15 characters at task startup (`cfe_psp_start.c:197-201`) [F] ([R01] §2.3), which gives a second, independent attribution check.
5. **Service** = module of the innermost observed API frame (entry/return nesting per thread); **stamp** = the identity argument or stored owner field from the API summary.

Every event either has all four identities resolved or carries an explicit `unknown` with the reason.

### 5.5 Timestamps versus causal tokens

HB never comes from timestamps ([R04] §2.1 Lamport; [v0.1] §9.3). Order information comes from:

1. **Per-thread tokens** (`tseq`): exact program order.
2. **Per-object tokens** (`obj_seq`), assigned in kernel at the probe instant, for operations that lie *inside* the object's own critical section: e.g. `OS_MutSemGive` entry runs while the mutex is held, so the token order of gives on one mutex equals the real lock order. With entry probes on take and give only, the lock order is the order of gives, and each acquire is completed by the next event of the acquiring thread [D].
3. **Value identity**: SB buffer-descriptor values (unique while in flight) match puts to gets; OSAL ids with generations identify resources; observed returned values identify reads-from (e.g. a `GetAppID` result equal to the published value).
4. **Fallback backend (no eBPF):** per-object order is reconstructed from timestamps of in-critical-section probe instants, under assumption **A-CLK** (a cross-CPU-consistent monotonic clock). The clocksource is `tsc` with `constant_tsc` and `nonstop_tsc` on all 4 vCPUs [M] (§A.2); A-CLK is recorded in B and the verdicts that depend on it say so.

### 5.6 Cost model

Per context i: `ΔC_i = Σ_j n_ij · c_j` ([v0.1] §11.2), with measured unit costs [M] ([R07] §4.7, §5.1):

| c_j | Value |
|---|---|
| entry+4 uprobe | 0.57 µs |
| single-stepped probe (entry+0, entry+5, most line sites) | 31.7 µs |
| HW watchpoint hit / debug-register gate trap | ≈ 34 µs |
| gdb breakpoint (comparison) | ≈ 203 µs |
| return probe, eBPF program body, string fetch, controller stop/resume | **not measured** (M1, M6) |

The counts n_ij are measured in M1; none is assumed here. As arithmetic on measured values only: if the 626–632 events per boot that CORD recorded ([R2C] §4.1) were all entry+4 probes, they would add ≈ 0.36 ms of probe time per boot; if all were single-stepped sites, ≈ 20 ms. The four costs (analysis, exploration, recording, control) and the specification effort are reported separately ([v0.1] §11.1; [R06] R4).

**Perturbation is measured as distributions, not means** ([R06] T6). Groups: A (no observer), C (full plan), C′ (minimal plan: R2 sites only), D (confirmation with gates). Metrics: time to the OPERATIONAL line, the R2 use set (expected 24: ES 20, EVS 4, [R2C] §7), queue-full drop counts (baseline 4–49, [BM] §0), `NoSubscribersCounter` (15–26), and the app init-completion order. The A baseline must be re-measured on the current kernel (`-fc-v114`) before comparison ([R07] §1).

---

## 6. Guarantees

Let M be the versioned model, A its assumptions, B the basis, σ the semantics, β the bound. **A** includes at least: A-API (API operations are atomic except where M splits them, and summaries are faithful), A-SC (sequential consistency at API-event granularity), A-VIS (volatile flag stores become visible to pollers, i.e. x86-TSO at `-O0`, [R06] T8), A-CLK (fallback backend only), path stability for robust certificates, and the list of unmodeled mechanisms (TIME seqlock, exception ring, condvar/rwlock, signals).

| # | Claim | Status |
|---|---|---|
| G1 | **Observation fidelity.** Each record corresponds to an executed probe in the unmodified reference binary run under ENV conditions; per-thread order is exact (eBPF backend); every loss is detected per thread (eBPF) or per CPU interval (fallback). | by construction; perturbation is timing only and is measured |
| G2 | **No fabricated order.** Every definite HB edge exists in the observed execution under M's rule semantics and A. Edges tagged *assumed* are reported as such. | relative to M, A |
| G3 | **Exact tier-1 status on T.** The ordered/unordered status of every instance pair is exact relative to the definite HB. | relative to M |
| G4 | **Robust fixed-order / protected certificates.** If `u ≺_R p` (resp. `p ≺_R u`), then in every execution of B under any σ in which the certified events execute on the same paths, u precedes p (resp. follows). | relative to M, A, path stability |
| G5 | **Predictive soundness.** Every predicted witness belongs to Exec(B, σ_U) if M and A hold (maximal-causal-model style, [R04] §2.5). | relative to M, A |
| G6 | **No false alarms among `confirmed` reports.** Each was reproduced on the unmodified binary with only declared gates, with the contract oracle observed (the C-3 template of [R05] §5.2). | by construction |
| G7 | **PCT arm.** Per run, depth-d bugs are found with probability ≥ 1/(n·k^(d−1)), with n and k measured and only relative to the controlled scheduling points. | if the arm is approved |

**Not claimed.**

- Soundness or completeness of detection overall; "detects all races"; "no false positives" outside `confirmed` ([R06] §6.3).
- Absence of violations from no alarm: `unobserved`, `unknown` and budget limits are reported with their causes. Prediction is **not complete**: Pred(T) is limited by the observed paths, the window, the extrapolation bounds and the budget, and it is not maximal under the API abstraction.
- Exec-level `fixed-order` without a robust certificate (an UNSAT result is relative to Pred(T)).
- Frequencies for flight targets; any σ_RT1 verdict carried to σ_ENV or the reverse.
- Memory-level data races, weak-memory effects, ISRs, DMA.
- That existing tools fail on the cases (all `NOT_RUN`); novelty of any single mechanism; "first process-race detector".

---

## 7. Openness

### 7.1 A generic contract library

Contracts are written per **API and resource class**, never per case, in a small typed language `.crl` with a grammar and a type checker in C++ (every name must resolve against `api.cat` and M's namespaces). Example, written only from the header:

```
template K_ID.on_behalf_event
  provenance doc "cfe/modules/core_api/fsw/inc/cfe_evs.h:168-172"   # "The Application ID must correspond to a registered application or core service."
  resource   identity(app_or_service)
  use        call CFE_EVS_SendEventWithAppID(AppID = $a)  bind r = identity_of($a)
  requires   state(r) in { REGISTERED }
  allow      { }                                            # the header points early users to CFE_ES_WriteToSysLog instead
  outcome    CFE_EVS_APP_ILLEGAL_APP_ID -> not_ready(UNSET)
             CFE_EVS_APP_NOT_REGISTERED -> not_ready(SET)
```

This template finds R2 with no SB-specific knowledge, and the same template classifies TBL's on-behalf events as `protected` when every caller runs after `CORE_READY` ([R06] §5.5 [I]). Library templates: K-REG, K-ID, K-PHASE, K-ROUTE, K-CAP, K-SPAWN, K-TBL, K-PERSIST, K-HANDOFF ([R06] §5.2), plus K-LIFE, K-MODE, K-FILE added here for RC15, RC13, RC12.

**Sources, in priority order** ([R06] §5.3):

1. **API documentation**: `\par Assumptions`, `\param[out]`, `\retval` blocks (counts in §2.5).
2. **Code admissions**: CF's "There is a start up race condition because CFE starts all apps at the same time" (`cf_cfdp.c:1272-1279`), CF's TOCTOU comment (`cf_cfdp_s.c:447-463`), TO_LAB's deferral comment (`to_lab_app.c:64-66`), the EVS guard comment (`cfe_evs_utils.c:613-623`), ES's "informational" wake comment (`cfe_es_backgroundtask.c:254-257`) [F].
3. **Retries and timeouts**: every bounded retry or poll in `site.cen` is evidence that its author expected a not-ready state, and it supplies Allow with its reference bound.
4. **Error paths**: not-ready return codes in `api.cat` populate the outcome maps.
5. **Belief inference** (Engler et al. SOSP 2001 [V-me]): ignored statuses and deviant call orders across the census, ranked, as hypotheses only.
6. **Issues**, labelled historical.

**Anti-circularity.** Each template has an authoring timestamp and hash; the library is frozen (F1) before the case evaluation and again (F2) before held-out evaluation. Case-specific refinements live in an appendix and every verdict that used one says so ([R06] T2).

### 7.2 Discovering unknown candidates

- **Outcome mining** (§4.1 item 2) needs no template.
- **Key-pair enumeration** covers all four namespaces of [R03] §7 (OSAL names, ES names, TBL names, loader symbols) plus MsgIds and files.
- **"Always observed in one order" pairs** across runs become flip candidates (CloudRaid, ESEC/FSE 2018 [V-abs]).
- **Isolated per-app profiling** under variant bases (Snowboard-style potential communications, SOSP 2021 [V-abs]) generates candidates only, never verdicts.
- **Ad hoc synchronization discovery**: repeated identical API calls separated by `OS_TaskDelay` on one thread are recognized as polls or retries (Tian et al. [V-abs]); unknown ones become candidates, and race coverage (EventRacer [V-abs]) reports the root rather than every downstream pair.

**Worked example 1 — the ES RESERVED window (R02 §1.6; [LED] C5 D4).** In every default boot the trace contains, per script app: the app record RESERVED (`:821`), the task record publish (`:703`), the child's successful `GetTaskFunction`, the child's first `GetAppID`-dependent call (e.g. `CFE_EVS_Register`), and the app record publish (`:869`). No HB edge links `:869` to the child's first use (only the 50 ms `OS_TaskDelay`, which is not synchronization) → tier 1: unordered → P1 schedulable → **predicted schedule-dependent under σ_U**, timing-protected in practice. Confirmation gate: hold the ES startup thread after `CFE_ES_StartAppTask` returns in `CFE_ES_AppCreate` (it holds no ES lock there, `cfe_es_apps.c:722`) until the child's first `GetAppID`-dependent call returns. Expected oracle per code: `CFE_ES_ERR_RESOURCEID_NOT_VALID` inside that call, and `GetAppIDByName` returning `CFE_SUCCESS` with `0xFFFFFFFF` for peers in the window [F]/[I] ([R02] §1.6). The tool reaches this without being told about C5.

**Worked example 2 — the OSAL lock-leak hypothesis (F1, [R01] §1.8).** K-REG over OSAL with the code-derived consequence of `INCORRECT_OBJ_STATE` in the API summary. Prediction asks, for every by-name lookup in the traces (e.g. `CFE_SB_GetPipeIdByName`, `CFE_ES_GetTaskIDByName`, `OS_*GetIdByName`) and every same-type create or delete of the same name, whether the lookup can fall between name visibility and finalize. If a default trace has no such overlap (e.g. `cFS-Master` is created robustly before any lookup), the verdict is `protected` for that pair and the hypothesis is reported as reachable only under named variants (teardown inputs, derived providers). Confirmation follows [R01] §7 item 1: hold the creator at the `*_Impl` entry (step f, name visible, table lock released) until the lookup returns, then release and watch whether the creator returns. The watchdog bound is a reference timeout; a non-returning creator is reported with its thread state as evidence. The hypothesis is never claimed as a bug before that run.

**Discovery of direct global reads** (DataCollider-style, [R04] §2.4): during a publication window (e.g. between reserve and publish of a service identity), a HW watchpoint on the field records any access by another context, with its PC mapped to a site. The hold is event-bounded or reference-bounded, never a chosen delay.

### 7.3 Configuration variants and held-out evaluation

Variants (each a recorded B′ with every changed element marked `deviation` or derived): CF with a non-empty `sem_name` and a provider (C1, public-code-derived from nasa/bp as in [LED] §1a); LC/MD tables and an extended SCH_LAB schedule that activate latent paths ([R03] §6.2); a reordered `MISSION_CORE_MODULES`; the cpu2 target; restart inputs. The held-out protocol is in §8.2.

---

## 8. Evaluation plan

### 8.1 Research questions

- **RQ1 (design cases, template-only).** With the frozen generic library (F1) and no case-specific contract, does Sextant assign the expected class to each design case: R2 `fixed-order` with a robust certificate; C1 `schedule-dependent` (derived configuration); C2 under its role-derived configuration; C3/C5 patterns via seeded re-introductions on v7.0.1 (their buggy code is absent there, [LED] §1b)? Which verdicts change when refinements are added?
- **RQ2 (openness, held-out).** After freeze F2, what does it find on held-out sources, with what triaged precision, and what share is `unknown` and why?
- **RQ3 (prediction).** How many violations are predicted from correct runs and then confirmed, compared with violations merely observed?
- **RQ4 (what the cFS model buys).** Ablations (§8.5).
- **RQ5 (cost and perturbation).** The four costs and the distribution shift of order-sensitive outcomes (§5.6).
- **RQ6 (confirmation).** Share of predicted witnesses confirmed, refuted, inconclusive; gate perturbation.

### 8.2 Subjects

- **Development set:** the five cases (C1, C2, C3, C5, R2), with each input labelled original / public-code-derived / role-derived / synthetic ([R06] S10). C3 and R2 share a resource family and are not counted as independent ([LED] §3.1).
- **Census subject:** the whole v7.0.1 bundle (15 apps, core, PSP) under the default B; every instance gets a verdict.
- **Held-out** (frozen first; [R06] §5.6): (a) other cFS revisions (version transfer; site patterns in M are re-resolved by AST pattern, not by line number); (b) other configurations (cpu2; variants of §7.3); (c) issues mined by a query pre-registered before F2, excluding the five cases; (d) seeded ordering mutations from the real fix history (remove a wait, move a publication, swap object-table entries), labelled synthetic; (e) in-code admissions. Each source is counted separately and nothing derived is called a "new bug".

### 8.3 Ground truth and triage

Two independent raters classify every reported instance with the verdict classes; disagreements are logged. Confirmed reports count as ground truth only for the confirmed property.

### 8.4 Baselines

Per [R06] S5/S13, external process-level tools are `NOT_RUN` ([R05] §2.9; K6), so comparisons are **reimplementations over the same traces**, labelled as such:

| Baseline | Observation model | Expected limitation (hypothesis) |
|---|---|---|
| FastTrack-style HB (PLDI 2009 [V-abs]) over the same API-level events | lifecycle transitions as writes, uses as reads, thread PO, lock edges | silent on R2 (ordered) and on lock-protected C1/C5 orders |
| WCP and SHB relations (sound predictive races) | same | silent on wrong-way ordered uses; no lock reversal |
| DCatch-style HB (ASPLOS 2017 [P]) | message and handler rules, no contracts, no conditional edges | no notion of allowed outcomes; barrier timeouts misread |
| RacePro-style syscall model (SOSP 2011 [V-abs]) | syscall tracepoints from the same runs; HB sources PO, fork/clone, wait, pipe/socket, signal; candidate classes RESOURCE, EXIT-WAIT, SIGNAL, TOCTOU | registry contracts are invisible (an OSAL name create+lookup makes zero syscalls, [R06] §5.6 citing `baselines/availability.md` §4.2) |
| gdb / manual | the reporter's method ([R2G]) | perturbation +0.6–1.1 s to OPERATIONAL |
| Counter-only repeated boots | SB/EVS housekeeping counters ([R03] §7) | detects that losses happened, not which order |
| TSan (gcc `-fsanitize=thread` variant), **only if the PI allows** | memory accesses | a different question (S13); reported as a reference measurement, not a baseline |

### 8.5 Ablations (one element at a time, same engine, same traces)

1. Conditional edges treated as unconditional (expect false `fixed-order` / `protected`).
2. Barrier rules removed (expect false candidates; CORD showed both edge kinds were needed, [R2C] §4.3).
3. Identities collapsed to one field (expect misattributed R2).
4. Creation treated as atomic (no RESERVED window: expect worked example 1 missed).
5. Thread PO instead of handler-scoped PO (expect fewer predicted handler reorderings).
6. Implementation-level HB (OSAL table, console locks as synchronization: expect masked candidates).
7. No prediction (tier 1 only); prediction with P3 only (time cost of skipping P1/P2).
8. No robust certificate (fixed-order only from UNSAT).
9. Templates only vs refined.
10. Observation models: syscall-only; memory-only.

### 8.6 Metrics and statistics

- Per class: instances, tier-1-decided share, predicted, confirmed, refuted, unknown (by reason), triaged precision, coverage of the census (observed sites / census sites).
- Costs: analysis time and memory per stage; solver time per query and the decided share as a function of budget; recording cost per thread; control cost per confirmation.
- Repeated runs: N is `unspecified_by_reference`; N and the test procedure are pre-registered. Comparisons use non-parametric two-sample tests and effect sizes, following Arcuri & Briand (ICSE 2011 [R05 V-abs]); the Klees et al. practice of about 30 trials with a Mann-Whitney U test is known only from a slide summary ([R05] §6.5, status P) and is cited as such.

### 8.7 Threats to validity

One VM with a drifted kernel; `SCHED_OTHER` only and `msg_max` 10 (results do not transfer to RT targets); probe perturbation (measured, not zero); unmeasured return-probe and controller costs; assumptions A-VIS, A-SC, A-API; Clang-derived summaries vs the gcc binary (mitigated by taking addresses from gcc DWARF); cases not independent; derived configurations; the persistent state carried between runs ([BM] §6).

---

## 9. Implementation roadmap

### 9.1 Directory layout (new tool; the retired CORD code is not reused, [R06] K7)

```
sextant/
  docs/            design, model rule table, .crl grammar, format specs, B.manifest guide
  include/sx/      one header per format: trace.hpp, ceir.hpp, verdict.hpp, plan.hpp, apicat.hpp
  lib/
    format/        container, readers/writers, CRC, canonical text dump
    model/         M: event kinds, LTS per namespace, rules + robustness tags, summaries
    lift/          demux, identity resolution, resource/lifecycle reconstruction, handler segmentation
    hb/            VC, VC_R, rule engine, coverage report
    contracts/     .crl parser, type checker, instantiation, outcome mining
    predict/       P1 (SHB), P2 (WCP, SyncP, M2), P3 encoder, extrapolation, witness extraction
    smt/           SMT-LIB emitter, solver process driver, budget sweep
    confirm/       gate derivation, placement checks, controller, watchdog
  tools/           sx-plan sx-observe sx-lift sx-analyze sx-confirm sx-report sx-dump
  bpf/             restricted-C eBPF programs (only if the eBPF backend is approved)
  contracts/       template library (.crl), frozen sets by hash
  models/          v701.model (site patterns + cited rules)
  tests/
    unit/          per library
    golden/        small gcc-built programs against the reference OSAL library in a scratch copy,
                   with known traces and expected verdicts
    e2e/           ENV boots through run_cfs.sh with a root-side observer hook
```

### 9.2 Milestones and exit criteria

| M | Deliverable | Exit criterion |
|---|---|---|
| **M0** | Format headers, container library, `.crl` grammar, verdict schema; canonical dumps | round-trip tests for every format; schema hashes stable; no cFS involved |
| **M1** | `sx-plan` (probe placement from gcc DWARF, cost classes) and `sx-observe` (fallback backend first) on the unmodified binary | **R2 re-observed by the new recorder on the unmodified binary**: 24 violating uses (ES 20, EVS 4) per boot in N runs; return-probe and site-class costs measured; n_ij counts and loss = 0 or accounted; groups A/C/C′ compared on the new kernel |
| **M2** | `sx-lift`: identities and resources | all 27 threads attributed; every app and task with owner; every API event with four identities or an explicit unknown |
| **M3** | HB engine (all rules of §4.2) with coverage report | on R2 traces every ordered use explained, now including the system-state wait that CORD did not model ([R2C] §4.3); ablations 1–2 reproduce the expected effects |
| **M4** | Contract library v1 and tier 1; **freeze F1** | template-only run: R2 `fixed-order` with the robust certificate of §4.3; census verdicts for the whole bundle |
| **M5** | Prediction P1–P3 (P3 needs the solver decision) | worked example 1 predicted from default boots; C1 (derived configuration) `schedule-dependent`; budget curves reported |
| **M6** | `sx-confirm` | worked examples 1 and 2 confirmed, refuted or inconclusive with perturbation accounting; controller cost measured |
| **M7** | **Freeze F2**; held-out evaluation; baselines; ablations | §8 tables complete |
| **M8** (phase 2) | K-PERSIST over two-boot traces; restart scenarios (K-LIFE); σ_RT1 model; optional PCT arm | as approved |

**What is built first:** M0 then M1 — the formats and an F0 observer that re-establishes the strongest existing ground truth (R2) with the new tool.

### 9.3 Keeping the code clean

Interfaces before implementations: each library exposes one header whose contracts are documented and unit-tested; tools are thin drivers. No code generation (no TableGen/ODS, no schema compilers). Deterministic outputs (sorted, hashed) so golden tests are stable. M rules and API summaries are data files with citations, not code. Every probe plan, model, library and binary is recorded by hash in each artifact header.

---

## 10. Risks and open decisions for the PI

### 10.1 Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Return probes or line probes land on the single-stepped path (~32 µs) | perturbation at hot sites (mutexes, SB) | entry-only lock reconstruction (§5.5); measure in M1; restrict line probes to once-per-boot publication sites |
| eBPF backend not approved | no per-object tokens; timestamp reconstruction under A-CLK | tracefs fallback with explicit A-CLK in verdicts |
| No solver installed | P3 unavailable | P1/P2 still run; P3 instances become `unknown(no-solver)` |
| API summaries or M rules wrong | false predictions | confirmation refutes them; refuted witnesses feed model revisions; rule coverage report |
| Prediction incomplete (paths not observed) | missed violations | census coverage reported; variants; never "safe" |
| Controller interactions (signals, real-time waits) | inconclusive confirmations | M6 measurement; `timeout-influenced` flag |
| Probe pre-registration on dlopen'ed `.so` untested ([R07] §9 item 6) | missed early app events | test in M1; otherwise register on the loader event |
| Kernel drift since the baseline | invalid comparisons | re-measure group A on `-fc-v114` |
| Scope (seven tools) vs timeline | late results | M1–M4 already decide R2 and every ordered instance, which is publishable alone ([R06] T12) |

### 10.2 Decisions requested

1. **Root-side observer** as the condition of observation runs, with a new `run_cfs.sh` hook beside `RUN_CFS_WRAP`/`RUN_CFS_HOOK` ([R07] §9 item 1).
2. **Installs**: `libbpf-dev` (eBPF backend) and a solver (`z3`/`libz3-dev` 4.8.12 or `cvc5`) as recorded analysis-host installs ([R07] §2.4).
3. **Confirmation controller** attaching as a debugger to the README-mode process (F0 binary, timing perturbed).
4. **Native PCT arm** with root-imposed `SCHED_FIFO` on one CPU, as a separate deviation arm (E1, E4).
5. **σ set** for the paper: σ_U + σ_ENV (frequency) only, or also σ_RT1 (model-only).
6. **A-VIS policy**: treat volatile polling and unlocked flag handshakes as assumed edges (recommended) or report them.
7. **Scope**: route, capacity and mode classes in the first scope (recommended, because they are the measured phenomena, [R03] §5.2); restart/teardown inputs (not from a reference, so a labelled variant).
8. **Held-out sources** (a)–(e) and who runs the pre-registered issue query, relative to F2.
9. **CORD's recorded observations**: citable as prior observations, or re-established only by Sextant (M1 does the latter in any case).
10. **TSan** as a reference measurement (a heavy code-generation variant), given S13.

---

## Appendix A

### A.1 Literature used (status as given in [R04], [R05], [R06])

Lamport 1978 [V-abs]; Fidge 1988, Mattern 1989 [V-abs]; Netzer & Miller 1992 [V-abs]; FastTrack 2009 [V-abs]; ThreadSanitizer 2009/2011 [V-abs]; DataCollider 2010 [V-abs]; KCSAN doc [V-doc]; CP 2012 [V-abs]; MCM 2012 [V-abs]; RVPredict 2014 [V-abs]; WCP 2017 [V-abs]; SHB 2018 [V-abs]; M2 2020 [V-abs]; SyncP 2021 [V-abs]; GPredict 2015 [V-abs]; Lu et al. 2008 [V-abs]; CTrigger 2009 [V-abs]; DefUse 2010 [V-abs]; Xiong et al. 2010 [V-abs]; Tian et al. 2008 [V-abs]; EventRacer 2013 [V-abs]; DroidRacer 2014 [V-abs]; CAFA 2014 [V-abs]; InitRacer 2017 [V-abs]; DCatch 2017 [P]; FCatch 2018 [V-abs]; CloudRaid 2018 [V-abs]; RacePro 2011 [V-abs]; Snowboard 2021 [V-abs]; RaceFuzzer 2008 [V-abs]; PCT 2010 [V-abs; mechanism P]; SCTBench 2014 [R05 V-abs]; Alglave et al. 2013 [R05 V-abs]; Wang et al. 2009 [R05 V-abs]; Arcuri & Briand 2011 [R05 V-abs]; Klees et al. 2018 [R05 V-abs; trial count P]; Strom & Yemini 1986 [R06 V-me]; Engler et al. 2001 [R06 V-me]; Go memory model [R04 V-doc] (shape of init/once rules only).

### A.2 Read-only spot checks made for this document (2026-10-12)

| Check | Result |
|---|---|
| `cfe_evs.h:168-172` | precondition text as quoted in §7.1 |
| doxygen counts | cFE `core_api/fsw/inc`: 74 `\param[out]`, 278 `\param[in]`, 452 `\retval`, 138 `\par Assumptions`; OSAL `src/os/inc`: 76 `\param[out]`, 593 `\retval` (grep counts) |
| `CFE_ES_GetTaskFunction` (`cfe_es_apps.c:559-605`) | delay 50 ms, check, stop when ready or `Timeout <= 0`, `Timeout -= 50` from 1000 → at most 21 checks |
| `CFE_ES_WaitForSystemState` (`cfe_es_api.c:584-605`) and `WaitForStartupSync` (`:616-619`) | poll with `OS_TaskDelay`, `CFE_ES_OPERATION_TIMED_OUT` on expiry; the startup-sync wrapper returns void |
| Prologues in `core-cpu1` (`OS_TaskCreate`, `OS_ObjectIdFinalizeNew`, `OS_ObjectIdFindByName`, `CFE_ES_WaitForSystemState`, `CFE_ES_GetTaskFunction`, `CFE_EVS_SendEventWithAppID`, `OS_QueuePut`, `OS_TaskEntryPoint`, `OS_MutSemTake`) and `sch_lab.so` (`SCH_LAB_AppMain`) | all `endbr64; push %rbp; mov %rsp,%rbp` |
| Symbols | 1212 dynamic `T` symbols in `core-cpu1`; `CFE_ES_MainTaskSyncDelay` is local (`t`), probe-able from `.symtab`; `CFE_SB_TransmitTxn_FindDestinations`, `CFE_ES_AppCreate`, `CFE_ES_StartAppTask`, `CFE_ES_TaskEntryPoint`, `OS_ObjectIdAllocateNew`, `OS_ObjectIdGetBySearch`, `CFE_TIME_NotifyTimeSynchApps` exported; the timebase thread function is `OS_TimeBase_CallbackThread` |
| Built `osconfig.h` | `OS_MAX_FILE_NAME` 20 (L150), `OS_MAX_PATH_LEN` 64 (L162), `OS_MAX_API_NAME` 20 (L171) |
| Host | clocksource `tsc`; `constant_tsc`, `nonstop_tsc` on 4 CPUs; `CONFIG_UPROBES`, `CONFIG_UPROBE_EVENTS`, `CONFIG_BPF_SYSCALL`, `CONFIG_BPF_JIT`, `CONFIG_HAVE_HW_BREAKPOINT` set; `libbpf-dev` not installed (candidate 1:1.3.0-2build2); 18 GB free on `/` |

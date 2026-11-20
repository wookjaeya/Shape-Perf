# Proposal A ("model-first"): CAIRN, a configuration-closed semantic model and readiness-contract checker for cFS process-level order violations

- Date: 2026-10-12. Panel angle: **A, model-first**. Status: architecture design only. Nothing was built, installed, run against cFS, modified or committed for this document. Code facts were spot-checked read-only in the pinned tree; the spot checks are listed in §0.3.
- Name proposal: **CAIRN** (Contract-Anchored Interleaving and Readiness aNalysis). The name is a label only.
- Evidence base: the seven AR1 reports, cited as **[R01]**–**[R07]** with their own section and `path:line` citations:
  [R01] `01_cfs_osal_psp.md`, [R02] `02_cfs_cfe_core.md`, [R03] `03_cfs_app_patterns.md`, [R04] `04_lit_dynamic.md`, [R05] `05_lit_systematic_static.md`, [R06] `06_critique_v01.md`, [R07] `07_env_feasibility.md` (all under `scratchpad/arch/`).
  Research records: `CONDITIONS_POLICY.md` (cited **[POL]**), `cases/ledger.md` (**[LED]**), `results/R2_gdb_v701.md` (**[R2g]**), `results/R2_cord_v701.md` (**[R2c]**), `env/BASELINE_MEASURE.md` (**[BM]**), and the user's design v0.1 (**[v0.1]**, cited by section).
- Code paths are relative to the pinned tree `/home/user/work/procrace/cfs_ref/cFS` (nasa/cFS v7.0.1 `088b2fa8`; cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`).

## 0. Reading guide, labels and summary

### 0.1 Labels

- **FACT** — stated by code, a build artifact, a research record or an AR1 report, with the citation.
- **INFERENCE** — my reasoning from cited facts; not executed.
- **DESIGN** — a design decision of this proposal, with its rationale.
- **OPEN** — needs a PI decision or an experiment; listed again in §10.
- Literature is cited only as verified in [R04]/[R05]/[R06], with their status tags (V-doc, V-abs, P, UNVERIFIED). No other literature is cited. Where a standard algorithmic fact is used whose citation was not verified in AR1, the text says "citation not verified in AR1".
- Numeric bounds are either taken from a reference (code constant, generated file, record) and cited, or marked `unspecified_by_reference` and handled by sweeping and reporting, never by choosing a value silently ([POL] L14-22).

### 0.2 Summary (one page)

**Thesis.** In cFS most order bugs are uses of a *framework resource* (a registry entry, a service identity, a phase, a route, a queue slot) in a state the framework's own contract forbids. Many such uses are *happens-before (HB) ordered in the wrong direction* by the startup protocol itself (R2: 24 uses per boot, all ordered before the publication [R2g] §2, [R2c] §7), and others are *protected by locks that give atomicity but not order* (C1, C5 [LED] §1a). Memory-level HB or lockset detectors are silent on both by construction ([R04] L4, L5). The right object of analysis is therefore a **typestate-style readiness contract** checked over **all executions of a configuration-closed model of the real build**, with verdicts that state the semantics they hold under.

**Approach (model-first).** CAIRN builds, from the reference build itself, a *closed* model of one cFS configuration and decides contract instances inside that model; execution is used only to (a) validate the model against the real binary and (b) confirm verdicts where the model is uncertain.

1. **Basis and closure (static, C++/Clang/LLVM).** Read the compile database, `link.txt` archive semantics, the generated core object table and startup script, the built tables, platform constants and the measured environment record. Resolve every task entry, callback and loader edge from configuration, not from pointer analysis ([R05] §2.5, [R06] T3). Anchor every source fact to the **GCC** binary's DWARF so that every probe and every report points at the code that actually runs ([R07] §3.5, [R06] T7).
2. **Concurrency skeleton (static, MLIR).** Import whole-program LLVM IR into MLIR and lower it by slicing passes into a small `cfsk` dialect: per-task programs containing only lifecycle, synchronization, registry, message and contract-relevant data operations, with polling loops summarized as `await` operations whose timeout branch is explicit and whose bounds cite platform constants.
3. **Contracts (DSL).** A typed contract language (RCL) states, per framework API, the required resource state and the allowed not-ready outcomes, each with provenance (header line, code admission, issue). Readiness predicates *call the framework's own validity predicates* (for example `CFE_ES_AppRecordIsMatch`), so "ready" is defined by cFE/OSAL code, not by the tool author. Templates are instantiated automatically at every matching call site in the closed program (the census).
4. **Tier-1 classification (static, model-based).** A must-happen-before analysis over the skeleton, with cFS-aware rules (spawn, observation-conditioned barrier edges, panic-on-timeout), decides many instances without search and emits **checkable certificates** (R2 becomes FIXED_ORDER with a cited edge chain; [R06] T5).
5. **Tier-2 bounded exploration (model-based).** Remaining instances are explored with optimal DPOR over the skeleton slice (cone of influence over synchronization objects) under a named semantics σ (untimed, or fixed-priority single-core); the two-query split (violating execution exists / conforming execution exists) gives FIXED_ORDER vs SCHEDULE_DEPENDENT vs PROTECTED within the bound. An SMT backend (MLIR `smt` dialect → SMT-LIB) is optional.
6. **Observation and selective confirmation (dynamic).** A root-side observer attaches perf uprobes at function entry+4 with eBPF to the **unmodified** README-mode GCC binary (0.57 µs per hit measured, [R07] §4.7) to (a) check that observed traces are traces of the model (conformance) and (b) observe contract outcomes. Only uncertain verdicts (SCHEDULE_DEPENDENT, UNKNOWN, conformance doubts) get *controlled* confirmation: per-thread ptrace holds at model-chosen lock-free gate sites (F0), or an `ld --wrap` gate variant for PCT (F1, a recorded deviation).

**Verdict** = (class ∈ {INACTIVE, NO_PROVIDER, PROTECTED, FIXED_ORDER, SCHEDULE_DEPENDENT, UNKNOWN}, semantics σ, model level M ∈ {M_api, M_impl}, bound b, basis B, evidence level). "No alarm" is never "safe": unexplored, unresolved or over-budget instances are UNKNOWN with a reason.

**What is new against v0.1** (details in Appendix B): the problem is defined over (B, σ, M, b), not over an undefined `Exec(B)`; HB-ordered wrong-way violations are first-class; scope is the 15 contract kinds of [R03] §4, not the five cases; MLIR holds *code* (the skeleton) rather than a re-encoded fact graph, behind a measured gate; contracts are generic templates with anti-circularity bookkeeping; a model-conformance loop ties the model to the real binary; observation uses the unmodified GCC binary.

**What is not claimed** (§6.3): soundness or completeness without qualification, absence of races, transfer of verdicts across σ, novelty of any individual mechanism, superiority over tools that were not run.

### 0.3 Spot checks made for this document (read-only, pinned tree)

| Fact used | Checked at |
|---|---|
| `CFE_EVS_SendEventWithAppID` precondition "The Application ID must correspond to a registered application or core service … #CFE_ES_WriteToSysLog can be used" | `cfe/modules/core_api/fsw/inc/cfe_evs.h` L168-172 |
| `CFE_ES_WaitForSystemState` assumption "the caller has also been initialized sufficiently to satisfy the global system state it is waiting for"; timeout retval | `cfe/modules/core_api/fsw/inc/cfe_es.h` L400-403, L411-412 |
| SB receive buffer lifetime "valid only until the next call to CFE_SB_ReceiveBuffer for the same pipe" | `cfe/modules/core_api/fsw/inc/cfe_sb.h` L449-464 |
| Header contract material: 138 `\par Assumptions` blocks and 429 `\retval` lines in cFE core API headers; 593 `@retval` lines in OSAL public headers (no `\par Assumptions` blocks there) | `grep -c` over `cfe/modules/core_api/fsw/inc/*.h`, `osal/src/os/inc/*.h` |
| `WaitForSystemState` polling loop and timeout branch | `cfe/modules/es/fsw/src/cfe_es_api.c` L584-605 |
| Core task: `MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)`, panic on failure | `cfe/modules/es/fsw/src/cfe_es_start.c` L832-846 |
| SB publishes its AppId through an out-pointer, status ignored | `cfe/modules/sb/fsw/src/cfe_sb_task.c` L137-138 |
| New-task trampoline: `OS_TaskDelay(POLL)` then check task record under ES lock, timeout | `cfe/modules/es/fsw/src/cfe_es_apps.c` L570-596; poll 50 / timeout 1000 at `cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h` L859-860, L878-879 |
| `CFE_ES_StartAppTask(TaskIdPtr, TaskName, EntryFunc, Params, ParentAppId)` | `cfe/modules/es/fsw/src/cfe_es_apps.c` L649-653 |
| CF "start up race condition" comment and retry stopping on any code other than `OS_ERR_NAME_NOT_FOUND` | `apps/cf/fsw/src/cf_cfdp.c` L1270-1301 |
| OSAL `ConvertToken` returns `OS_ERR_INCORRECT_OBJ_STATE` before the unlock path; `GetBySearch` cancels only on not-found | `osal/src/os/shared/src/osapi-idmap.c` L401-404, L935-947 |
| Generated core object table order CONFIG, ES, EVS, FS, SB, TBL, TIME, MSG, SBR, RESOURCEID | `build-native_std/native/default_cpu1/cpu1/cfe_static_module_list.c` L36-47 |
| Startup script: 2 libraries, 15 apps, entry symbols per line | `build-native_std/exe/cpu1/cf/cfe_es_startup.scr` L1-17 |

---

## 1. Problem definition

### 1.1 Execution substrate and events

**FACT.** Native cFS is one process; every OSAL task is a pthread ([R05] §1 F1; `osal/src/os/posix/src/os-impl-tasks.c:568`). Besides OSAL tasks there are the PSP main ("idle") thread that runs `CFE_ES_Main`, the console writer thread, one thread per OSAL timebase, and raw pthreads ([R01] §2.6). In the ENV build all 27 threads run `SCHED_OTHER`, nice 0, and OSAL records priority 255 for every task ([R01] §2.5; [BM] §0).

**DESIGN (event model).** An execution is a set of threads Θ and, per thread, a sequence of events. Each event `e` carries:

| Field | Meaning | Why it is a separate field |
|---|---|---|
| `θ(e)` ∈ Θ | **executing task** (the thread) | SB code runs in the caller's thread ([R02] §2.4, §2.8) |
| `α(e)` | **owning app**: the ES app record that owns `θ(e)`, as a generation-qualified identity `(slot, serial)`; ⊥ for threads with no ES record (PSP main, console, timebase); for timer callbacks a separate attribution `α_attr(e)` = the app that created the timer | cFE task IDs and app IDs carry serials ([R02] §6.2); callbacks run in foreign threads ([R01] §2.3, [R02] §5.3) |
| `ς(e)` | **called service**: the cFE/OSAL/PSP module whose code contains the event's site, from link-unit membership | ES task executing SB code ([R2g] §2) |
| `ι(e)` | **stamped identity**: the AppId value the effect carries (the `AppID` argument of `CFE_EVS_SendEventWithAppID`, the AppId stored as pipe owner by `CFE_SB_CreatePipe`, a caller AppId resolved inside an API) | R2's rejected events carry SB's unpublished AppId 0 while executing in ES ([R2g] §2); gap G6 in [R04] §5 |
| `op(e)`, `args(e)`, `res(e)` | model operation, arguments, result (status code, value read) | contract outcomes are results ([R06] F11) |
| `site(e)` | source site plus its GCC-binary anchor (§2.3) | reports and probes must name code that runs ([R06] T7) |
| `seq(e)` | per-thread index | program order without timestamps ([R04] §2.1 Lamport) |

The four identities are never merged. Ablation "collapse identities" (§8.4) measures what merging costs.

### 1.2 Resources, namespaces, lifecycle generations and typestate

**DESIGN.** A *namespace* `N` is a framework-defined map from keys to records. A *resource* is `r = (N, key, g)` where `g` is a lifecycle generation. Namespaces at the pin:

| Namespace | Key | Generation source | Record states (typestate) | Source |
|---|---|---|---|---|
| OSAL idmap, per object type (task, queue, countsem, binsem, mutex, timebase, timer, stream, module, …) | name, or slot + serial | OSAL serial in the ID (`(type<<16)|serial`) | FREE → ALLOC (table lock held) → RESERVED_C (name invisible) → RESERVED_C+N (name visible, still RESERVED) → LIVE → RESERVED_D → FREE | [R01] §1.2, §1.5 table, §1.6 |
| ES app / task / lib / generic counter tables | name; resource ID | cFE resource-ID serial | FREE → RESERVED → LIVE → RESERVED (teardown) → FREE; counters: FREE → LIVE in one critical section | [R02] §1.6, §1.14, §1.16, §6.2 |
| CDS registry | "App.Name" | — (idempotent by name) | ABSENT → LIVE (restored or new) | [R02] §1.13 |
| TBL registry | "App.Table" | registry slot reuse + owner AppId | ABSENT → REGISTERED → LOADED → (VALIDATION_PENDING → VALIDATED) → ACTIVE; access descriptor `LockFlag` | [R02] §4 |
| SB pipes | name; PipeId | pipe-ID serial | FREE → RESERVED → LIVE → RESERVED (delete) → FREE | [R02] §1.16, §2.2 |
| SB routes | MsgId | — | NO_ROUTE → ROUTED(set of pipes) | [R02] §2.3, §2.7 |
| SB pipe capacity | pipe | — | depth counter 0..C (C = effective kernel depth) | [R02] §2.4; [BM] §3.1 |
| Loader symbols / modules | symbol name, module name | load count | UNLOADED → LOADED (symbols resolvable) → UNLOADED | [R01] §3 (modules row), §4 |
| Service identity globals | global symbol (e.g. `CFE_SB_Global.AppId`) | boot | UNSET (EarlyInit zero) → SET(value) | [R02] §1.16 last rows |
| EVS registrations | AppId slot (`AppId % 32`) | AppId serial | UNREGISTERED → REGISTERED | [R02] §3.1, §6.2 |
| TIME synch callback slots | app slot | AppId serial | EMPTY → SET → EMPTY | [R02] §5.3 |
| Files / OSAL open-file table | path | — | ABSENT / OPEN(by stream) / CLOSED | [R01] §3 (files row); [R03] RC12 |
| PSP IODriver devices | device name | — | absent / present (fixed module list) | [R03] §2.10 |
| System and app states | — | boot | the ordered enum of `CFE_ES_SystemState`; per-app `AppState` | [R02] §1.2, §1.8 |
| Persistent areas across runs | area (CDS, reset area, boot record, `/dev/shm/osal:RAM`, `EEPROM.DAT`) | run index | per area | [R01] §5.3, §5.5 |

**DESIGN (state = predicate).** The state of a resource is *not* a separately maintained label. It is a **predicate over program state, evaluated in the model**, preferably by calling the framework's own validity function. Examples: ES app `a` is LIVE iff `CFE_ES_AppRecordIsMatch(CFE_ES_LocateAppRecordByID(a), a)` ([R02] §1.6: `es/cfe_es_resource.h:253-256`); an OSAL record is LIVE iff `active_id` equals its ID ([R01] §1.7). This gives three benefits: the definition of "ready" comes from cFS code (less author bias); RESERVED windows appear automatically because the predicate is evaluated on the actual fields; and the **publishers** of a readiness condition are computed, not declared: they are the stores that can make the predicate true.

### 1.3 Readiness contracts

**DESIGN (definition).** A contract template is

τ = ⟨ pattern, ρ, Ready, Allowed, Prov ⟩

- `pattern`: an API or site pattern with argument binders, e.g. `call CFE_EVS_SendEventWithAppID(eid, etype, a, …)`.
- `ρ`: a key expression from the binders and state to a resource, e.g. `EvsIdentity(a)`.
- `Ready`: the predicate of §1.2 that must hold when the use executes (the *required precondition*).
- `Allowed`: the set of **allowed not-ready behaviours**, each a predicate over the use's result and the bounded continuation of `θ(u)`: for example "returns a documented `\retval` code and the caller tests it and later reaches its normal continuation" (handled), "returns `OS_ERR_NAME_NOT_FOUND` and the caller retries within the documented bound" (CF), "documented drop", or `unspecified` when no reference says.
- `Prov`: provenance records (kind ∈ {api-doc, code-admission, issue, researcher}; `path:line`; sha256 of the text; authoring time; whether any design case had been inspected for this template at authoring time).

An **instance** is κ = (τ, s_u, key valuation), with the key concrete where value analysis resolves it, symbolic otherwise.

For an execution `e` and an occurrence `u` of `s_u` in `e`:

- `ready_e(u)` ⇔ `Ready(ρ)` holds in the model state immediately before `u` (the model's semantics is an interleaving semantics, so the state before `u` is defined).
- `handled_e(u)` ⇔ ¬`ready_e(u)` ∧ `outcome_e(u)` ∈ `Allowed`.
- **Violation** `V_κ(e)` ⇔ ∃ `u` ∈ occ(s_u, e): ¬`ready_e(u)` ∧ `outcome_e(u)` ∉ `Allowed`.

The **consequence** of a violation is a separate axis, from the outcome of `u` and its continuation, following the effect-oriented classification of ConMem and Portend ([R04] §2.6, §2.10, V-abs): crash or undefined behaviour; deadlock; app error exit; status ignored and effect lost silently; event then continue; delayed but recovered. This keeps "order violation" separate from "error" ([R06] S7).

### 1.4 The analysis basis B

**DESIGN.** B = ⟨ Src, Bld, Cfg, Plat, In, Init ⟩. Every component is recorded with value, reference and status `from-reference` / `deviation` / `unspecified_by_reference` ([POL] L12-17).

| Component | Content at the pin | Source |
|---|---|---|
| Src | bundle and submodule commits | [R03] header; [R01] header |
| Bld | `build-native_std/native/default_cpu1/compile_commands.json` (1417 entries; 448 FSW entries); `link.txt` of `core-cpu1` and of each app `.so` (whole-archive core, lazy BSP members); compiler gcc 13.3.0, `-g`, no `-O`; generated sources (`cfe_static_module_list.c`, `cfe_module_version_table.c`) | [R07] §2.5, §3.1, §3.4; [R2c] §1, §7 (the version table changes R2's count from 24 to 11) |
| Cfg | startup script (17 lines); built tables (`build-native_std/exe/cpu1/cf/*.tbl`) and their sources (`sample_defs/tables/*.c` overriding app defaults); platform constants as evaluated macros (`cfe_es_internal_cfg.h`, `cfe_sb_internal_cfg.h`, `osconfig.h`, app `*_internal_cfg.h`); `MISSION_CORE_MODULES` as realized in the generated table | [R02] §0, §1.3; [R03] §6.1-6.2; [R01] header |
| Plat | OSAL backend posix; permissive mode, task priorities disabled (all 255); `SCHED_OTHER`; 4 vCPU; `CFE_*` tasks pinned to CPU 0 by the PSP startup hook (code fact, not yet measured); `msg_max` 10 so effective pipe depth = min(requested, 10); kernel `6.18.44-fc-v114` (drift from `-fc-v80` recorded at build time) | [R01] §2.3, §2.5; [BM] §3.1, §4; [R07] §1 |
| In | external inputs and their timing: none by default until the stop command; ground commands (restart, table load, SB pipe info) only as declared scenario inputs | [BM] §1.1; [R03] §3 rows 17, 22 |
| Init | persisted state: boot record → reset type; CDS and reset-area SysV segments; files on `/dev/shm/osal:RAM`; `EEPROM.DAT` | [R01] §5.2-5.3; [BM] §6 |

A change to any component is a new basis. A verdict under a modified basis (for example SB moved before EVS in the core table) is a **configuration-change experiment**, never "another schedule of the same B" ([v0.1] §8.5; [R06] §3 v0.1 §10).

### 1.5 Scheduling and timing semantics σ, and model levels M

**FACT.** cFS startup is built from timed polling loops with soft timeouts (`cfe_es_start.c:860-918`; `cfe_es_api.c:584-605`; `cfe_es_apps.c:570-596`; `osapi-idmap.c:475-480`; CF retry `cf_cfdp.c:1280-1291`) ([R06] §0 item 2). C3's misorder came from RT priorities on its original platform; on the ENV priorities have no effect ([LED] §1a C3; [R04] G8).

**DESIGN.** Every verdict names one semantics:

- **σ_U (untimed interleaving).** Sequentially consistent interleaving of the model's atomic transitions. Every enabled transition may fire. A delay (`OS_TaskDelay`) is a scheduling point with no ordering effect. The timeout outcome of every timed wait or bounded retry is enabled once the wait has started, independent of other threads. `CFE_PSP_Panic` and `abort` are terminal. Priorities and affinity impose nothing. No fairness assumption is needed, because every loop inside the analysis horizon is bounded (§4.2).
- **σ_R1 (fixed-priority preemptive, one CPU).** σ_U restricted so that a thread may take a step only if no ready thread has strictly higher priority; priorities are those the configuration requests (startup script column 5, core task defaults [R02] §0); delayed or waiting threads become ready nondeterministically (untimed); equal priorities are unconstrained. This is the semantics under which C3-class priority orders exist ([R05] §2.7 lessons from Schwarz et al. POPL 2011 and Tulsyan et al. FSTTCS 2020, V-abs). It does **not** describe the ENV, where OSAL disables priorities ([R01] §2.5).
- **σ_E (ENV, observed).** Not a model semantics. It is the distribution of native executions under the ENV platform. Claims under σ_E are empirical frequencies with confidence intervals ("violated in k of n boots"), never "fixed" or "protected".

A timed model semantics with execution-time intervals would need durations that no reference gives; it is out of scope and would be labelled empirical if ever added (OPEN-10).

**DESIGN (model levels).** OSAL can be modelled at two levels, and a verdict names which:

- **M_api**: OSAL primitives follow their API contract (create is atomic with respect to lookup; lookups return found or not-found). Portable across OSAL backends.
- **M_impl**: the OSAL POSIX implementation code at the pin is analysed too, on top of pthread/semaphore/mqueue semantics. It exposes the idmap RESERVED window, the bounded RESERVED wait, the non-recursive table mutex and the lock-leak path ([R01] §1.5-1.8).

[R01] §7 item 6 asks which level is ground truth. CAIRN reports both where they differ: **PROTECTED under M_impl but violable under M_api** is a *portability hazard* (protection that depends on POSIX implementation detail); **violable only under M_impl** is an *implementation defect candidate* of OSAL POSIX at the pin.

### 1.6 Verdicts

**DESIGN.** Fix κ, B, σ, M and a bound b (the analysis horizon and any loop bounds, §4.10). Let `Exec_b(B, σ, M)` be the model's executions within b, and `E_κ` those that reach `s_u`.

| Class | Definition | Notes |
|---|---|---|
| **INACTIVE(B)** | no execution of the closed program reaches `s_u` (static reachability under Cfg), or none within b (explorer; labelled INACTIVE_b) | e.g. CF's semaphore lookup with the default empty `sem_name` ([LED] §1b C1; [R03] §2.6) |
| **NO_PROVIDER(B)** | `s_u` is reachable but no transition of the model can make `Ready(ρ)` true | a dependency or configuration error, not an order problem: e.g. HS's lookup of PSP device `linux_sysmon`, which fails on every ENV boot ([R03] §3 row 20) |
| **PROTECTED(B, σ, M, b)** | `E_κ` ≠ ∅ and no `e` ∈ `E_κ` satisfies `V_κ` | holds within M, σ, b only |
| **FIXED_ORDER(B, σ, M, b)** | `E_κ` ≠ ∅ and every `e` ∈ `E_κ` satisfies `V_κ` | a tier-1 certificate makes b = ∞ (all executions of M under σ) |
| **SCHEDULE_DEPENDENT(B, σ, M, b)** | ∃ `e₁`, `e₂` ∈ `E_κ`: `V_κ(e₁)` ∧ ¬`V_κ(e₂)` | the pair is the witness |
| **UNKNOWN(reason)** | model incompleteness touching κ (unresolved indirect call or havoc in its slice, non-conforming trace region, unsupported semantics) or budget exhaustion | never reported as safe |

Sub-labels, reported separately and never merged into the classes:

- **HANDLED**: not-ready uses occurred but every outcome was in `Allowed` (e.g. the EVS `EVS_AppID` guard drops an internal event, [R02] §3.4; CF's retry recovers).
- **LOSS-UNDER-ORDER**: for route and capacity contracts whose `Allowed` set is `unspecified_by_reference` (the SB header documents no rule on sending to a MsgId with no subscriber; checked at `cfe_sb.h`), CAIRN reports "message lost under some (or every) execution" without calling it a violation.
- **portability hazard / implementation defect candidate** (§1.5).

**Evidence level** is an orthogonal axis: `certificate` (tier-1 proof), `model-witness` (explorer execution), `observed` (native σ_E boot under F0 observation), `confirmed-controlled` (native, with declared holds at declared sites). Every report carries both axes ([R06] S1).

### 1.7 Ordered-wrong-way versus unordered violations, and violation chains

**DESIGN.** For a violating occurrence `u` in execution `e`, let `p*` be the first transition in `e` that makes `Ready(ρ)` true. The *shape* of the violation is:

- **WRONG-WAY**: `u →hb p*` (HB per the model's sync semantics, §3);
- **UNORDERED**: `u ∥hb p*`;
- **ABSENT**: no `p*` in `e` (the provider never publishes in this execution);
- **STALE**: `Ready(ρ)` held earlier, then a disabling transition (delete, restart, unload) occurred before `u` (generation mismatch).

A FIXED_ORDER instance whose shape is WRONG-WAY in every execution is the R2 pattern. An instance that is UNORDERED in some execution is race-shaped. HB race detectors can only ever report UNORDERED shapes on memory conflicts ([R04] L4; gap G2 in [R04] §5).

**Violation chains.** κ₁ ⇝ κ₂ when an occurrence of `u₂` is control- or data-dependent on a not-ready outcome of `u₁`. FACT (record): 17 of R2's 24 uses are `SEND_NO_SUBS` events; the record infers, from the event ID and later console lines, that they exist because ES transmits its EVS event packets (MsgId 0x808) before any route exists ([R2g] §2; [R06] §3 v0.1 §8). CAIRN represents this as a route-readiness instance (LOSS-UNDER-ORDER) chained to identity-readiness violations.

### 1.8 Why plain data-race happens-before detection is insufficient

1. **R2: ordered the wrong way.** FACT (records): all 24 violating uses per boot precede SB's AppId store, in 3/3 boots under gdb and 3/3 without a debugger; the records attribute the order to the core-startup serialization (S1), and the CORD analysis rebuilt the HB chain for the ordered uses ([R2g] §2-3; [R2c] §4.3, §7). A vector-clock detector (Djit+, FastTrack, TSan; [R04] §2.2-2.3, V-abs/P) reports only *unordered* conflicting accesses; an ordered pair is not a race by definition. Predictive detectors (CP, WCP, SHB, M2, SyncP; [R04] §2.5, V-abs) widen the set of *unordered* pairs; none reports a pair that every execution orders the same way. Result: silence on the only case observed in the testbed.
2. **C1 and C5: protected by locks that give atomicity, not order.** FACT: C1's create and lookup both run under the OSAL table lock; C5's registration and lookup both run under the ES shared-data lock ([LED] §1a). A lockset detector (Eraser, [R04] §2.3 V-abs) sees consistent protection. An HB detector adds a release→acquire edge in whatever order the run took, which hides the other order (the HB false negative that O'Callahan–Choi describe, [R04] §2.3 V-abs; L5). CP and WCP reorder only critical sections that do not conflict; here they conflict on the registry. SyncP keeps the observed lock order, which forbids exactly this reversal, and beyond that restriction prediction is NP-complete and W[1]-hard ([R04] §2.5, V-abs). The conflict object must be the **registry key**, with the lock as atomicity.
3. **Losses are outcomes, not edges.** FACT: SB returns success when a message has no route and silently drops on a full queue; 4–49 queue-full drops and 15–26 no-subscriber drops per boot in the default configuration ([BM] §0; [R03] §5.2). No memory conflict exists to report ([R04] L6, G5).
4. **Ad hoc timed synchronization.** FACT: `SystemState` is a `volatile sig_atomic_t` written and polled without a lock ([R02] §1.2). A C11-level detector would report this intended synchronization (INFERENCE, [R04] §2.3), while the real edge exists only when the poll exits by observation, not by timeout ([R04] L3, G3).
5. **Layered identity and configuration.** Whether a use violates depends on *whose* identity is stamped (G6) and on the configuration (startup order, tables, effective queue depth, effective priorities; G7, G8) ([R04] §5).

### 1.9 Established theory that CAIRN extends

| Ingredient | Established source (status) | What CAIRN adds for cFS |
|---|---|---|
| General races vs data races; feasible vs apparent races | Netzer & Miller, LOPLAS 1992 ([R04] §2.1, V-abs) | readiness contracts are general-race (order) requirements over framework resources |
| Typestate: allowed operations depend on object state | Strom & Yemini, IEEE TSE 1986 ([R06] §9, V-me) | typestate of *shared* framework resources checked at a use in *another* task; states as framework predicates; allowed not-ready outcomes |
| Happens-before and vector clocks | Lamport 1978; Fidge 1988; Mattern 1989 ([R04] §2.1, V-abs) | API-level cFS HB rules (§3), observation-conditioned barrier edges |
| Framework-aware HB for event systems | DroidRacer, CAFA (PLDI 2014), DCatch (ASPLOS 2017, P), SIERRA's static action HB (ASPLOS 2018) ([R04] §2.7-2.8; [R05] §2.4) | SB delivered-send→receive only; ES phase barriers with timeout branch; spawn edge at `OS_TaskCreate`, not at ES bookkeeping |
| Ad hoc synchronization recognition | Tian et al. ISSTA 2008; Xiong et al. OSDI 2010; EventRacer race coverage OOPSLA 2013 ([R04] §2.6-2.7, V-abs) | polling loops summarized as `await` with an explicit timeout branch; edge only on the observing exit |
| Def-use and use-before-initialization | DefUse OOPSLA 2010 ([R04] §2.6, V-abs); UBITect ESEC/FSE 2020 ([R05] §2.4, V-abs); ExceptioNULL FSE 2012 ([R04] §2.5) | cross-task, value-carried readiness (the AppId argument's reads-from) |
| Stateless model checking with optimal DPOR; bounded POR; fair SMC | Abdulla et al. POPL 2014; Coons et al. OOPSLA 2013; Musuvathi & Qadeer PLDI 2008 ([R05] §2.1-2.2, V-abs) | exploration of a configuration-closed cFS skeleton with OSAL semantics; classification by the two-query split |
| Partial-order SMT encodings of concurrency | Alglave et al. CAV 2013 ([R05] §2.3, V-abs) | optional backend for the same two queries |
| Predictive analysis over a trace | maximal causal models (RV 2012), RVPredict (PLDI 2014), GPredict (ICSE 2015) ([R04] §2.5, V-abs) | reversed-order feasibility of observed pairs with lock = atomicity and key-level conflict, used to seed confirmation |
| Environment models and closed harnesses | SDV EuroSys 2006; Holzmann & Joshi SPIN 2004; Goblint ASE 2016 lesson on environment assumptions ([R05] §2.4, §2.8, §4, V-abs) | the configuration closure (loader, object table, tables) as the environment model, taken from the generated build |
| Soundiness | Livshits et al. CACM 2015 ([R05] §0.5, §2.4, V-abs) | the enumerated unsound choices in §6.2 |
| Confirmation by directed scheduling | CHESS (OSDI 2008), PCT (ASPLOS 2010), CTrigger (ASPLOS 2009), RaceFuzzer (PLDI 2008), RacePro replay (SOSP 2011) ([R04] §2.10; [R05] §2.1, §2.9, V-abs) | holds placed by the model at lock-free gate sites of the unmodified binary |

[R04] §5 concludes that no individual mechanism is novel; the claimable contribution is the combination around the cFS semantics (gaps G1–G8) and the contract formulation. CAIRN follows that framing.

---

## 2. Architecture

### 2.1 Data flow

```
 ════════════════════════ STATIC + MODEL (analysis host; C++17 against LLVM 23 build 1053047a) ═══════════════════════
  pinned tree, build-native_std,        ┌──────────────┐  basis.ctc   ┌───────────────────┐  facts.ctc
  ENV records, persist snapshot ──────▶ │ S0 Basis     │────────────▶ │ S1 Fact Extractor │───────────┐
                                        │   Assembler  │              │ Clang LibTooling  │           │
                                        └──────┬───────┘              │ + LLVM IR per     │           │
                                               │                      │   link unit       │           │
                                               │                      └─────────┬─────────┘           │
                                               │  GCC core-cpu1, app .so       │ core-cpu1.bc,       │
                                               ▼  (DWARF)                       │ app .bc             ▼
                                        ┌──────────────┐  anchors.ctc ┌───────────────────┐  closed.ctc ┌──────────────────┐
                                        │ S2 Anchor    │────────────▶ │ S3 Configuration  │───────────▶ │ S4 Skeleton      │
                                        │   Mapper     │              │   Closure (loader,│             │   Builder (MLIR: │
                                        └──────────────┘              │   tables, ind.    │             │ llvm → cfsk)     │
                                                                      │   calls, tasks)   │             └────────┬─────────┘
  contracts/*.rcl ──▶ ┌──────────────┐  templates.ctc                  └───────────────────┘   model.cfsk.mlirbc │
  (provenance,        │ C0 Contract  │──────────────────────────────────────────────┐                        │
   freeze hashes)     │   Compiler   │                                              ▼                        ▼
                      └──────────────┘                                     ┌──────────────────────────────────────┐
                                                                           │ S5 Instantiator / Census             │
                                                                           │ (instances κ, INACTIVE, NO_PROVIDER) │
                                                                           └───────────┬──────────────────────────┘
                                                              ┌────────────────────────┴────────────────────────┐
                                                              ▼                                                 ▼
                                                 ┌─────────────────────────┐                      ┌──────────────────────────┐
                                                 │ T1 Must-HB Classifier   │── undecided ───────▶ │ T2 Bounded Explorer      │
                                                 │ (certificates)          │                      │ optimal DPOR + monitor;  │
                                                 └───────────┬─────────────┘                      │ K-CAP backlog; opt. SMT  │
                                                             │                                    └────────────┬─────────────┘
                                                             └──────────────────┬──────────────────────────────┘
                                                                                ▼
                                                                  verdicts.ctc (class, σ, M, b, B, evidence, witness)
                                                                                │
                                                                                ▼
                                                                  ┌───────────────────────────┐   plan.ctc (probes, gates,
                                                                  │ P0 Probe & Gate Planner   │── watchdog bounds) ─────┐
                                                                  └───────────────────────────┘                         │
 ══════════════════════ DYNAMIC (root-side; unmodified GCC binary; README-mode user `ubuntu`) ══════════════════════════│═══
                                                                                                                         ▼
     ┌───────────────────────────────────────────────┐            ┌──────────────────────────────────────────────────────┐
     │ O1 Observer (F0): perf uprobes @entry+4 +      │──trace.ctc▶│ D1 Decoder + Conformance Checker (trace ⊆ L(M))      │
     │    eBPF → ring buffer → trace writer           │            │ D2 Predictive feasibility (trace-constrained DPOR)   │
     ├───────────────────────────────────────────────┤            └───────────────┬──────────────────────────────────────┘
     │ K1 Confirmer: ptrace per-thread holds (F0) or  │──trace.ctc──────────────────┤
     │    --wrap gate runtime + PCT controller (F1)   │                             │  feedback: closure mismatches,
     └───────────────────────────────────────────────┘                             │  conformance faults → S3/S4;
                                                                                    │  observed outcomes → evidence
                                                                                    ▼
                                                                  ┌───────────────────────────┐
                                                                  │ R0 Reporter (multi-axis   │──▶ report views (Markdown/HTML),
                                                                  │   records, certificates,  │    certificate listings, costs
                                                                  │   costs)                  │
                                                                  └───────────────────────────┘
```

Edge list (for redrawing): S0→S1, S0→S2, S0→S3; S1→S2 (source sites), S1→S3 (facts, bitcode); S2→S3 (anchors); S3→S4; C0→S5; S4→S5; S5→T1; T1→T2 (undecided instances); T1,T2→verdicts; verdicts→P0; P0→O1, P0→K1; O1→D1; K1→D1; D1→D2; D1→S3/S4 (feedback); D1,D2,verdicts→R0.

### 2.2 Components

| Id | Component | Kind | Language and libraries | Inputs → outputs | Technique borrowed |
|---|---|---|---|---|---|
| S0 | Basis Assembler | static | C++17; `clang::tooling::JSONCompilationDatabase` (reads CMake's input file); own parsers for `link.txt`, the startup script and the cFE table file format | build tree, generated files, `.tbl`, ENV records, persist snapshot → `basis.ctc` | SDV-style environment model from the real build ([R05] §4) |
| S1 | Fact Extractor | static | C++17; Clang LibTooling + ASTMatchers; Clang constant evaluator (`APValue`) for table initializers and macros; `RawComment` for API doc comments; clang-23 `-emit-llvm -O0 -Xclang -disable-O0-optnone -g`; `llvm-link` emulating whole-archive/lazy semantics | compile DB, sources, `link.txt` → `facts.ctc`, `core-cpu1.bc`, `<app>.bc` | CodeQL precedent of extracting from the real build ([R06] §3 v0.1 §5) |
| S2 | Anchor Mapper | static | C++17; `LLVMDebugInfoDWARF`, `LLVMObject`, `LLVMMC` disassembler (all in the LLVM build) | GCC `core-cpu1`, app `.so`, source sites → `anchors.ctc` + audit report | — (engineering; R06 T7 mitigation) |
| S3 | Configuration Closure | static | C++17 over facts | basis + facts + anchors → `closed.ctc`: task set, entries, call graph, resolved indirect calls, module membership, table values | configuration-driven `dlsym` resolution, MLTA-style type layers ([R05] §2.5) |
| S4 | Skeleton Builder | static → model | C++17; MLIR (`mlir-translate --import-llvm` equivalent in-process; own `cfsk` dialect; pattern-rewrite passes; verifiers) | bitcode + closure + templates → `model.cfsk.mlirbc` (+ textual `.mlir` for review) | program slicing; harness-around-unmodified-code idea ([R05] §2.8 Holzmann–Joshi) realized as an abstraction instead of generated C |
| C0 | Contract Compiler | static | C++17 hand-written recursive-descent parser for RCL (§7.2) | `contracts/*.rcl` → `templates.ctc` (typed AST, hashes, freeze record) | typestate ([R06] §9) |
| S5 | Instantiator / Census | model | C++17 + MLIR | templates + model → instances, INACTIVE / NO_PROVIDER verdicts | Engler-style belief inference for unwritten contracts ([R06] §5.3) |
| T1 | Must-HB Classifier | model | C++17; MLIR dataflow framework (dense forward solver) or plain worklist | model + instances → verdicts with certificates; undecided set | SIERRA-style static action HB ([R05] §2.4) |
| T2 | Bounded Explorer | model | C++17 interpreter of `cfsk`; optimal DPOR (source sets + wakeup trees); monitor; K-CAP backlog solver; optional MLIR `smt` + ExportSMTLIB | undecided instances + slices → verdicts with witnesses | optimal DPOR, BPOR ([R05] §2.2); Alglave encoding ([R05] §2.3) |
| P0 | Probe & Gate Planner | model → dynamic | C++17 | model + verdicts + anchors → `plan.ctc` | CTrigger minimal perturbation; DataCollider/KCSAN stall-at-one-side ([R04] §2.4, §2.6) |
| O1 | Observer | dynamic (F0) | C++17 loader using raw `perf_event_open(2)` and `bpf(2)`; BPF programs in restricted C compiled by system clang-18 `-target bpf` | plan + running `core-cpu1` → `trace.ctc` | — (R07 measured mechanism) |
| K1 | Confirmer | confirmation | C++17 ptrace controller (F0); C99 gate runtime built by **gcc** and linked with `--wrap` in a recorded variant (F1); C++ PCT controller | plan + witness → controlled `trace.ctc` | CHESS interception, PCT, RaceFuzzer, DCatch triggering ([R04] §2.10; [R05] §2.1) |
| D1 | Decoder + Conformance | dynamic → model | C++17 | trace + model → per-thread sequences, matched sync, conformance verdict | trace-inclusion checking against the model |
| D2 | Predictive feasibility | model over trace | C++17 (trace-constrained DPOR); optional SMT | trace + model → reversed-order witnesses | RVPredict/GPredict control-flow abstraction ([R04] §2.5) |
| R0 | Reporter | report | C++17 | all artifacts → report views | multi-axis record ([v0.1] §13, [R06] R1) |

**DESIGN.** All components are linked into **one driver binary** `cairn` with subcommands (`basis`, `extract`, `anchor`, `close`, `skeleton`, `census`, `classify`, `explore`, `plan`, `observe`, `confirm`, `conform`, `report`). R07 measured 56.7 MB per statically linked LibTooling binary ([R07] §2.2); one binary pays this once. There is no Python and no JSON in the tool's data path. The only JSON the tool reads is CMake's `compile_commands.json`, which is an input produced by the reference build.

### 2.3 Typed artifacts

**DESIGN.** Two artifact families, both typed, versioned and content-addressed:

1. **CAIRN Typed Container (`.ctc`)** for basis, facts, anchors, closure, templates, instances, verdicts, plans and traces. Layout:
   - header: magic `CAIRN\0\0\0`, `u16 abi_major`, `u16 abi_minor`, `u32 kind`, `u8 sha256[32]` of the payload, and the sha256 of every **parent artifact** (basis, templates, model, plan), so each verdict is traceable to its exact inputs (Merkle-style derivation);
   - sections: `{u32 tag, u32 record_size, u64 count, payload}`; records are fixed-layout little-endian C structs declared once in `include/cairn/abi/*.h` with `static_assert` on size and offsets; variable-length data lives in a string/blob table section referenced by offset;
   - footer: section index and, for traces, loss counters.

   The same C headers are included by the BPF programs, the gcc-built gate runtime and the C++ analyzer. No serializer is generated; readers and writers are hand-written against the struct layouts and round-trip-tested.
2. **The model** as MLIR bytecode (`model.cfsk.mlirbc`) of the `cfsk` dialect, with a textual form for review and golden tests. MLIR bytecode is typed and versioned by the dialect; the verifier runs on load.

Artifact kinds:

| Artifact | Kind | Key records |
|---|---|---|
| `basis.ctc` | basis | component values with reference and status (`from-reference` / `deviation` / `unspecified_by_reference`); file hashes; platform record; persisted-state snapshot |
| `facts.ctc` | facts | functions, call sites (direct, indirect, intra-TU flag, macro-origin flag), global and field accesses, constant arguments, table initializer values, API doc comments |
| `anchors.ctc` | anchors | site id → (file, line, col, function, link unit) → GCC binary (build id, function symbol, address ranges from `.debug_line`, probe address, instruction class: emulated / single-step) |
| `closed.ctc` | closure | task set with entry, creator site, owning-app expression; resolved indirect edges with the resolving rule; unresolved sites; module membership |
| `templates.ctc` | templates | typed contract ASTs, provenance, freeze hash, authoring time |
| `verdicts.ctc` | verdicts | instance id, class, σ, M, b, evidence level, shape, certificate or witness reference, consequence, chain links, costs |
| `plan.ctc` | plan | probe list (address, record kind, argument extraction), gate list (site, lockset proof, release condition, watchdog bound with its reference) |
| `trace.ctc` | trace | 64-byte event records (§5.3), string records, loss counters, probe hit counts, mechanism class F0/F1, kernel version |

### 2.4 Use of Clang, LLVM and MLIR: justification and gate

| Use | Benefit (measured or argued) | Alternative considered |
|---|---|---|
| Clang LibTooling on the reference compile DB | **Measured**: 448/448 FSW TUs parse in 10 s on one thread with two diagnostic-only adaptations (`-Wno-unknown-warning-option`, `-Wno-error`) plus `-resource-dir` ([R07] §3.3). Gives macro provenance (12.4% of API call sites are in macro expansions, [R07] §4.6), doc comments for contract mining, and exact constant evaluation of table initializers and platform macros | gcc has no plugin headers installed ([R07] §4.5); `-fcallgraph-info` gives only coarse graphs ([R07] §4.4) — kept as a cross-check of the call graph |
| LLVM IR per link unit, linked with `link.txt` semantics | **Measured**: 448 bitcode files in 4.4 s; closed `core-cpu1.bc` in 0.35 s with 0 undefined `CFE_`/`OS_` symbols and 1222 defined functions ([R07] §3.4). SSA def-use is the substrate for slicing | analysis on the AST only (no def-use across functions) |
| MLIR for the skeleton | **Measured**: import of `core-cpu1.bc` into the LLVM dialect in 0.78 s; an out-of-tree dialect plus driver builds in about 7 s ([R07] §2.2, §3.4). **Argued**: the skeleton is *code* (per-task control flow, loops, calls), so MLIR is used for what it is built for — progressive lowering by passes from the imported LLVM dialect to `cfsk`; verifiers enforce model well-formedness (every `await` has a timeout region and a cited bound; every sync op names a declared object); `FusedLoc` carries the Clang location, the GCC anchor and the semantic-rule id into every certificate edge; the upstream dataflow framework (V-doc, [R05] §2.6) can host the tier-1 analysis; the upstream `smt` dialect with SMT-LIB export works out of tree (probe in [R07] §2.2) | a C++ CFG data structure over LLVM IR with the same op vocabulary (the gate's control arm) |
| `LLVMDebugInfoDWARF`, `LLVMMC` | **Argued**: read the GCC binary's line tables and classify probe instructions (emulated vs single-step, which decides 0.57 µs vs 31.7 µs per hit, [R07] §4.7) without new packages | `gdb` batch queries as used by M1 ([BM] §2.3): slower, not reusable |
| System clang-18 BPF backend | **Measured** available ([R07] §2.3) | none without installs |

This answers [R06] T1 for this angle: MLIR is not a container for a re-encoded fact graph; it carries the abstracted program that the explorer executes. It does not add detection power by itself ([v0.1] §5.2; [R05] §2.6 (c)), and the paper's contribution does not depend on it.

**DESIGN (gate G-MLIR, at milestone M3).** Implement tier-1 classification twice over the same facts and slices: (a) on `cfsk`/MLIR, (b) on a plain C++ graph over LLVM IR with the same op vocabulary. Decide by stated criteria, in order: verdict equality on the development set (required); certificate provenance completeness (each edge has a source site, a GCC anchor and a rule id); analysis code size; runtime. If (b) is at least as good on every criterion, MLIR is dropped and the SMT backend emits SMT-LIB directly. The decision is recorded. Costs of keeping MLIR: dependence on the unreleased LLVM trunk `1053047a` with headers from the source tree ([R07] §2.1), and TableGen/ODS generation at build time (standard MLIR practice, not checked-in generated code; still OPEN-2 against the "no messy code generation" requirement, [R06] K8).

### 2.5 Static ↔ dynamic loop

**DESIGN.** v0.1 is strictly forward ([R06] §3 v0.1 §1). CAIRN closes the loop:

1. **Closure boot.** One F0 observation boot records `OS_ModuleLoad`/`OS_SymbolLookup` results, every `OS_TaskCreate` (name, entry pointer) and every child-thread start. D1 compares them with S3's task set and entry resolution; a mismatch is a closure bug, fixed before any verdict is reported.
2. **Conformance.** Every observed boot is checked against the model (§4.8). A trace region the model cannot produce turns every verdict whose slice touches that region into UNKNOWN(conformance) until the model is fixed.
3. **Evidence.** Observed outcomes upgrade evidence levels; they never change a model class silently. If an observation contradicts a model class (e.g. a violation observed for a PROTECTED instance), the model is wrong by definition and the instance goes to UNKNOWN with a model-fault record.

---

## 3. cFS mapping: how each mechanism is modelled

### 3.1 Semantic layers

**DESIGN.**

| Layer | What | How it enters the model | Used in |
|---|---|---|---|
| L-1 | POSIX primitives under OSAL: pthread mutex (NORMAL for idmap tables, RECURSIVE for OSAL mutexes), condvar timed wait, `sem_*`, `mq_*` (zero-timeout send), `pthread_create/cancel/join/detach/exit`, `clock_nanosleep`, POSIX timers + `sigwait` | hand-written interpreter rules, each citing the OSAL call site that uses it and the POSIX behaviour relied on ([R01] §1.4, §3) | M_impl only |
| L0 | OSAL public API (tasks, mutex, binsem, countsem, queue, timebase/timer, delay, idmap lookups, module load / symbol lookup, files, sockets, select) | hand-written rule per API with a doc and code citation; this is the closed synchronization vocabulary ([R01] §0 item 6: condvars and rwlocks have no non-test caller) | M_api; in M_impl replaced by analysed OSAL code for idmap, task, binsem and queue paths |
| L1 | cFE core, PSP and app code | **analysed automatically** into `cfsk` skeletons (§4.2); cFE API semantics come from cFE code, not from hand summaries | both |
| L1′ | libc data functions (`memcpy`, `strncpy`, …) | pure data effects on tracked locations | both |
| Env | sockets, ground commands, files written by tools, signals | environment events declared in `In`; otherwise havoc | both |

The API-level HB recommendation of [R01] §3 is kept as the rule for *shapes* (§1.7): OSAL-internal locks (idmap table locks, binsem internal mutex, console lock, timebase `handler_mutex` except where it is the documented ordering mechanism) never create shape-HB edges, although under M_impl they do constrain which interleavings exist.

### 3.2 OSAL idmap lifecycle and the RESERVED window

FACT ([R01] §1.3-1.8): creation passes ALLOC under the type lock, then `ConvertToken(EXCLUSIVE)` sets `active_id := RESERVED` and **releases** the lock; `OS_OBJECT_INIT` then writes `name_entry`; the implementation call (`pthread_create`, `mq_open`, …) runs; `FinalizeNew` relocks and writes the final ID (osapi-idmap.c L1090-1093, release L1105). Comments claiming the lock stays held are wrong (F3). Lookups by state are in [R01] §1.7: by-name on a RESERVED record with `name_entry` set returns `OS_ERR_INCORRECT_OBJ_STATE` **and leaves the type lock held** (F1, code reading). GLOBAL/REFCOUNT/EXCLUSIVE lookups of a RESERVED record wait at most 5 attempts (10+40+90+160 ms) and then fail with `OS_ERR_OBJECT_IN_USE`; every unlock of the type broadcasts and costs waiters an attempt. NONE-mode operations check `active_id` without a lock.

| Element | M_api | M_impl |
|---|---|---|
| States | ABSENT → LIVE → ABSENT (atomic create/delete w.r.t. lookups) | FREE → ALLOC → RESERVED_C → RESERVED_C+N → LIVE → RESERVED_D → FREE, per slot, with `name_entry`, `active_id`, `refcount` and the type lock as model state |
| `OS_*Create` | one transition: key LIVE with a fresh generation | analysed OSAL code; the two lock sections and the unlocked middle are separate transitions |
| `OS_*GetIdByName` | outcome FOUND / `NAME_NOT_FOUND` | analysed code: FOUND / `NAME_NOT_FOUND` / `INCORRECT_OBJ_STATE` with the lock still held |
| By-ID GLOBAL/REFCOUNT/EXCLUSIVE on RESERVED | not applicable | `await(state ≠ RESERVED)` with up to 5 attempts; each attempt is consumed by a timeout **or** by any unlock of the type (broadcast); then `OS_ERR_OBJECT_IN_USE` |
| NONE-mode use (Give/Take/Put/Get) | requires LIVE, else `OS_ERR_INVALID_ID` | same, lock-free check; concurrent `*_Delete_Impl` is a use-after-destroy event (F4) |
| Generation | serial in the ID; slot reuse after ~65535/`OS_MAX_<type>` allocations ([R01] §1.2) | same |
| Cross-run identity | by (type, name, creator, creation ordinal), never by ID ([R01] §1.2, §6.2 item 3) | same |
| Deadlock | — | reachable states where a thread waits forever on a NORMAL mutex are reported by the explorer's deadlock check (§4.5) |

Contract templates touched: `osal.lookup_by_name.live` (K-REG), `osal.api.returns_unlocked` (lock discipline, M_impl only; source: the function's own header comment "Upon failure, the global table lock is always released", osapi-idmap.c L384-385, [R01] §1.8 step 3), `osal.create.entry_runs` (task, §3.3).

### 3.3 Task create, start, delete and exit

FACT ([R01] §2.1-2.4; [R02] §1.6, §1.10): `pthread_create` runs while the record is RESERVED; the child's `OS_TaskPrepare` does a GLOBAL `GetById` and so waits for `FinalizeNew` (bounded, 5 attempts); if it gives up, the child exits without running its entry and the record stays active (F7). The PSP startup hook runs in the new task (`OS_EVENT_TASK_STARTUP`: `OS_GetResourceName`, CPU-0 pinning of `CFE_*`, thread name). cFE then adds a second handshake: `CFE_ES_StartAppTask` calls `OS_TaskCreate` **before** taking the ES lock and fills the ES task record afterwards; the child's trampoline `CFE_ES_TaskEntryPoint` → `CFE_ES_GetTaskFunction` delays 50 ms, then polls under the ES lock until its task record has a defined `AppId` and `EntryFunc`, with a 1000 ms timeout (`cfe_es_apps.c` L570-596). The App record is published later (`:869`) and is not checked by the trampoline.

| Event | Model operation | HB (shape) rule | Outcomes |
|---|---|---|---|
| `OS_TaskCreate` call | `cfsk.spawn(entry, name)` → child becomes runnable at the `pthread_create` point (M_impl) or at the call (M_api) | creator's events before the call → child's first event ([R01] §2.2 edges 1-2; [R02] §8.1 R2) | status; `*task_id` |
| child `OS_TaskPrepare` wait (M_impl) | `await(record LIVE)`, 5 attempts, broadcast-consumed | edge from `FinalizeNew` unlock on the observing exit only | success / silent exit (F7) |
| PSP startup hook | analysed PSP code in the child | none to other tasks (implementation-level console/table locks excluded) | — |
| ES task-record poll | `cfsk.await(TaskRec.AppId defined ∧ EntryFunc ≠ 0)` with timeout region; bound cites `cfe_es_internal_cfg.h` L860, L879 | ES-lock-protected write → observing poll exit ([R02] §8.1 R5) | entry called / task never calls its entry (`:625`) |
| entry call | `call @<resolved entry>`; resolution by S3 from the script entry symbol or core table (§3.8) | program order | — |
| `OS_TaskDelete` | `cfsk.kill(t)`: victim stops at its next cancellation point (model: at its next blocking op or delay); deleter blocks until then; locks held by the victim stay held ([R01] §2.4) | victim's last event → deleter's return ([R02] §8.1 R17) | held-lock leak is a reportable consequence |
| `OS_TaskExit` | `cfsk.exit`; record freed before `pthread_exit` ([R01] §2.4) | — | — |

Contract templates: `es.task.self_identity` (K-SPAWN: a task's first `CFE_ES_GetAppID` requires `CFE_ES_GetAppRecordByContext() != NULL`, i.e. task **and** App record LIVE — [R02] §1.6), `osal.create.entry_runs` (`OS_TaskCreate` success ⇒ entry eventually runs; provenance class: researcher/code-reading, because no API doc states it; F7 is the counterexample shape).

### 3.4 OSAL primitives

All rows are FACT from [R01] §3 (table) unless marked; the model column is DESIGN.

| Primitive | Facts that matter | Model state and transitions | Shape-HB rule (API level) | Outcomes recorded |
|---|---|---|---|---|
| **Mutex** (`OS_MutSem*`) | POSIX `PTHREAD_MUTEX_RECURSIVE` with priority inheritance; owner tracking is a debug warning only | owner, depth; lock blocks unless free or owned | outermost unlock → next outermost lock of the same mutex; nested lock/unlock add nothing ([R02] §8.1 R4). Locks give atomicity; order only together with a state check inside ([R02] §1.15) | status |
| **Binary semaphore** | value saturates at 1 (gives **coalesce**); give wakes one waiter; take blocks at 0 and sets 0; **flush** wakes all current waiters without consuming; internal 2 s `timedlock` → `OS_SEM_FAILURE`; timeouts on `CLOCK_REALTIME` | value ∈ {0,1}; waiter set; `give` (0→1 or coalesced no-op), `take`, `timedtake` (success / timeout), `flush` | the give that performed 0→1 → the take that consumed it; coalesced gives create no edge; flush → each flushed take; timeout → none ([R02] §8.1 R9) | status, consumed vs flushed (not distinguishable to the caller, but the model knows) |
| **Counting semaphore** | counts accumulate; no FIFO among waiters; `sem_wait` interrupted by a signal returns failure | count; `give`, `take`, `timedtake` | join of all prior gives → take (as TSan models `sem_post`/`sem_wait`, [R01] §3 row) — a model choice that can only add order, never states | status |
| **Queue** | `OS_QueuePut` never blocks (`mq_timedsend`, zero timeout, priority 1); full → `OS_QUEUE_FULL`, message dropped; FIFO with one priority; copy semantics; Get modes PEND / CHECK / timed; **effective capacity = min(requested, `msg_max` 10)** while OSAL records the requested depth | FIFO of message identities, capacity from Plat (§1.4; [BM] §3.1) | put(m) → get(m) of the same message only; no edge on drop | delivered / `QUEUE_FULL`; get timeout |
| **Timebase and timers** | one thread per timebase; POSIX timer on `CLOCK_MONOTONIC` → RT signal → `sigwait`; each tick takes `handler_mutex`, walks the callback ring and calls callbacks with it held; `TimerAdd`/`TimerSet`/`TimerDelete` take `handler_mutex`; callbacks see `OS_TaskGetId()` = the timebase ID; timebase delete cancels without join; overruns not counted | a timebase task (spawned at `OS_TimeBaseCreate`) whose loop is: tick (nondeterministic in σ_U) → for each registered timer due: run callback region | `TimerAdd`/`TimerSet` (app task) → later callbacks; callback k → callback k+1 on the same timebase; typical chain tick → callback → `OS_CountSemGive` → app task ([R01] §3 row; `sch_lab_app.c` L143-146; `cfe_time_tone.c` L952-955) | callback begin/end, attributed to the timer's creator (`α_attr`) |
| **`OS_TaskDelay`** | `clock_nanosleep` | scheduling point, no effect in σ_U; makes the thread not-ready until a nondeterministic wake in σ_R1 | **none** — sleeps are never ordering ([R02] §8.2) | duration argument (evidence of a timing-based protocol) |
| **Console / `OS_printf`** | serializes all callers through the console table lock | havoc-free data effect | none at API level ([R01] §3 row) | — |
| **Select, sockets** | readiness only; external peers | environment events from `In` | datagram send → receive only for in-process pairs | readiness result |
| **Files, dirs, filesys** | no synchronization semantics; persistent shared state (`/cf`, `/ram`); `OS_FileOpenCheck` is check-then-act; iterators include RESERVED streams | file namespace (path → OPEN/CLOSED, content generation) | none | path, op, status |
| **Modules and symbols** | `dlopen`/`dlsym`; lookup succeeds only after the defining module is loaded | loader namespace (§1.2) | load → successful lookup (observation rule, §4.4) | names, status |
| **Condvar, rwlock** | compiled, no non-test caller in cFE, apps or PSP; rwlock has no POSIX table lock (F6) | not modelled; any reachable call becomes `unknown_effect` | — | — |

### 3.5 PSP startup and persistence

FACT ([R01] §5.1-5.5): `main` → `OS_BSP_Initialize` (reads `msg_max` as non-root) → argument parsing → `OS_API_Init` → `OS_RegisterEventHandler(CFE_PSP_OS_EventHandler)` → reserved memory (SysV segments keyed by `ftok` of CWD key files) → fixed module list init (`soft_timebase` creates and starts timebase `cFS-Master` at 10 ms) → `sleep(1)` → boot record decides the reset type → `CFE_PSP_InitProcessorReservedMemory` → `CFE_ES_Main` → idle loop on `sigwait(SIGUSR1)`. The boot record, CDS, reset area, user-reserved area, `/dev/shm/osal:RAM` files and `EEPROM.DAT` persist across runs; how run *n* ended decides run *n+1*'s reset type (PO → PR → PO cycle after signal stops, [R01] §5.2).

DESIGN:

- The PSP main thread is an analysed task (L1); `soft_timebase` init is a `cfsk.spawn` of a timebase task plus timer creation; `sleep(1)` is a delay (no ordering, σ_U); the reset-type decision reads `Init`.
- `cFS-Master` publish → TIME and SCH_LAB lookups are ordered by program order and spawn: a tier-1 PROTECTED certificate is expected ([R01] §4 table, first row) — a check that the tool does not invent races where the code orders.
- **Persistence (K-PERSIST).** `Init` is part of B. For cross-run contracts the explorer takes `Init` from a declared set: {POWERON (all areas cleared, [R01] §5.2), PROCESSOR with areas as left by a previous model run}. A two-run scenario composes the end state of run *n*'s model with run *n+1*'s start. For observed runs, `Init` comes from the persist snapshot that the harness already records ([BM] §2.5).
- Exception path (signal handler → 4-entry ring in shm → `SIGUSR1` to the idle thread → `SystemNotify` → ES pops entries with a search lookup, [R01] §5.4): modelled as environment events only (soundiness item A5, §6.2); the search lookup is in scope for the M_impl lock-leak check.

### 3.6 ES phases, soft barriers and `WaitForSystemState`

FACT ([R02] §1.2, §1.4, §1.7; spot check §0.3): `SystemState` is written by the ES startup thread without a lock and polled without a lock every `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` (50) until the caller's timeout; `CFE_ES_WaitForSystemState` first raises the caller's own `AppState` under the ES lock (RUNNING for core callers at CORE_READY; LATE_INIT for APPS_INIT; RUNNING for OPERATIONAL), then polls; on timeout it returns `CFE_ES_OPERATION_TIMED_OUT`; `CFE_ES_WaitForStartupSync` returns `void`. `CFE_ES_MainTaskSyncDelay` polls, under the ES lock, the number of *used* records (including RESERVED ones) whose `AppState` is below the target; the two script-phase syncs only log on timeout and continue. An app that exits by `CFE_ES_ExitApp` is STOPPED, which is above RUNNING, so it satisfies the barrier; an app that returns from its main without `ExitApp` stays below and makes the barrier time out ([R03] §1).

DESIGN (model):

| cFS element | `cfsk` form | Shape-HB rule |
|---|---|---|
| `SystemState := s` (ES startup thread) | `cfsk.store @CFE_ES_Global.SystemState` tagged `assumed_sync` (volatile flag; soundiness A1) | release |
| `WaitForSystemState(s, T)` | `cfsk.store AppState(self) := f(s)` under ES lock (the *declaration*), then `cfsk.await(SystemState ≥ s)` with `success` and `timeout` regions; bound `⌈T/50⌉` cites the call-site argument and `cfe_es_internal_cfg.h` L860 | edge from the store that the observing exit read; **no edge on the timeout exit** ([R04] L3) |
| `WaitForStartupSync(T)` | as above; the status is discarded in the program, but the model keeps the exit reason as a hidden variable and the recorder observes it at `WaitForSystemState`'s return (§5.3) | — |
| `MainTaskSyncDelay(X, T)` | `cfsk.await(∀ used records: AppState ≥ X)` under the ES lock; participant set = records used at check time; timeout region = panic (core phase) or continue (script phases) | edge from each observed `AppState` write on the observing exit |
| `RunLoop` first call | `AppState(self) := RUNNING` under ES lock (implicit declaration, [R03] §1) | — |
| After OPERATIONAL | every wait returns immediately (`SystemState ≥` any target) | apps started later get no barrier ([R02] §1.7) |

Contract template `es.phase.declaration_implies_ready` (K-PHASE) comes from the `cfe_es.h` L400-403 assumption: a declaration (the own-state store inside `WaitForSystemState`, or the first `RunLoop`) must come after every publication by that app that another app consumes after its own wait for a state the declaration satisfies. Violation shapes: declare-then-publish (SBN declares before loading its table and initializing, [R03] §2.15; CS declares even after init failure, [R03] §2.14), and the barrier's timeout path (σ_U), which releases waiters before slow providers publish (CF's retry window of up to 2.5 s exceeds the 2 × 1 s soft budget, [R03] §2.6 inference).

### 3.7 Core startup serialization

FACT ([R02] §1.3; spot check §0.3): the core object order is the CMake list `MISSION_CORE_MODULES`, realized in the generated `cfe_static_module_list.c`; `CFE_ES_CreateObjects` runs every `EarlyInit` in table order in the startup thread (failure → panic), then for each entry with a `TaskMain`: reserve the App record under the lock, `CFE_ES_StartAppTask`, publish the record, `MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC = 30000)`, panic on timeout. A core task becomes RUNNING inside its own `WaitForSystemState(CORE_READY, …)` call, after its `*_TaskInit`.

DESIGN: S3 reads the order from the **generated** table by constant evaluation of `CFE_CORE_MODULE_OBJTAB` (never hard-coded; [R02] §1.3 correction of `mapping_v701.md`), and cross-checks it against `cfe/cmake/mission_defaults.cmake` L26-40. Because the timeout branch of each core sync is a panic, the tier-1 rule "observation edge on the only continuing branch" (§4.4) yields the fixed core order ES-TaskInit → EVS-TaskInit → SB-TaskInit → TBL → TIME as a certificate, and with it R2's FIXED_ORDER verdict (Appendix A.1). A configuration that reorders `MISSION_CORE_MODULES` is a new B.

### 3.8 Startup-script apps, libraries, loader and the trampoline

FACT ([R02] §1.5; [R03] §6.1; [R05] §1 F2-F4): `CFE_ES_StartApplications` reads the script one byte at a time and processes `;`-terminated lines in order; a `CFE_LIB` line loads the module and runs its init function **synchronously** in the startup thread while the Lib record is RESERVED; a `CFE_APP` line reserves the App record, loads the module (`OS_ModuleLoad` → `dlopen(RTLD_NOW|RTLD_GLOBAL)`), resolves the entry symbol (`OS_ModuleSymbolLookup`), starts the task through the trampoline, publishes the App record, and moves to the next line without waiting. Failed entries do not stop later ones.

DESIGN:

- **Loader model (S3).** For each script line: module file → the app `.so`'s link unit (its own `link.txt`), entry symbol → the defining function in that unit's bitcode; the task's `EntryFunc` in the model is that function. The analogous static resolution of `OS_ModuleSymbolLookup`, `dlsym` and the trampoline's `(*RealEntryFunc)()` call removes the call-graph gap that a points-to analysis cannot close ([R05] §2.5 item 1; [R06] T3).
- **Order facts.** Program order of the startup thread gives exactly two real orders ([R02] §1.5): every library init completes before any later line starts; `OS_TaskCreate` of app *k* precedes that of app *k+1*. App *k*'s execution is unordered with app *k+1*'s.
- **Trampoline.** §3.3 (ES task-record poll). The App-record window between `cfe_es_apps.c:722` and `:869` is closed only by the 50 ms delay (sleep, not sync; [R02] §1.6), so under σ_U the explorer may run the new task's first `CFE_ES_GetAppID` before `:869` (Appendix A.3).
- **`GetAppIDByName` during the window** returns `CFE_SUCCESS` with `0xFFFFFFFF` because "used" includes RESERVED ([R02] §1.6, new detail). The K-REG template for ES by-name lookups uses `CFE_ES_AppRecordIsMatch`, not "used", as its readiness predicate, so a successful return with the sentinel is classified as not-ready.
- **SBN lazy modules** (`OS_SymbolLookup`, then `OS_ModuleLoad("/cf/sbn_udp.so")`, `sbn_app.c` L1060-1088): resolved by reading SBN's configuration table values (S1 constant evaluation) to module file and symbol names, then linking to the module's bitcode; ops-struct calls through those modules are resolved by type layers plus configuration ([R05] §1 F6).

### 3.9 Child tasks

FACT ([R02] §1.10): `CFE_ES_CreateChildTask` requires the caller to be the app's main task and uses the same `CFE_ES_StartAppTask`; the App record is already LIVE, so the App-record window does not apply; `DeleteChildTask` calls `OS_TaskDelete` while holding the ES lock; `ExitChildTask` frees its record and calls `OS_TaskExit`.

DESIGN: `cfsk.spawn` from the parent's skeleton, entry resolved from the constant function-pointer argument; K-HANDOFF instances for parent↔child protocols (FM: queue + countsem + mutex, with the count read without the mutex, [R03] §2.8). Deleting a child while holding the ES lock is checked by the "victim holds a lock" consequence (§3.3).

### 3.10 Restart, reload, delete and teardown

FACT ([R02] §1.11-1.12): requests set `ControlReq` under the ES lock; the ES main task wakes the background task (binsem give, "informational"); the background scan, also timer-driven every 1000 ms, moves a RUNNING app with a request to WAITING with a kill timer of 5 scans; the app's `RunLoop` sees the request and exits, or the timer expires and teardown proceeds while the app's tasks still run; `CFE_ES_CleanUpApp` marks task and App records RESERVED, runs every core module's `Cleanup` (EVS → SB → TBL → TIME) **without** the ES lock, deletes tasks (children first), deletes pools, unloads the module, frees records; restart/reload then call `AppCreate` again with a new AppId and **no** startup barrier.

DESIGN: a **scenario family** R(app, kind ∈ {restart, reload, delete}) driven by an input event in `In` (a ground command, or HS's action, [R03] §3 row 17) at a nondeterministic point after OPERATIONAL (σ_U). The background scan's timer is a nondeterministic tick; the kill timer is `await` with a timeout region (5 scans, cited from `cfe_es_internal_cfg.h` L229). Generations increment on re-create. Contracts reopened by a restart: every K-REG/K-ROUTE/K-TBL instance the app provides, STALE shapes for resolved references held by others (RC15: CF semaphore ID, MD symbol address, SBN symbols; [R03] §4), TIME synch callback called after unload ([R02] §5.3 TOCTOU), SB receive on a pipe deleted by teardown while the task still runs ([R02] §1.11 inference).

### 3.11 Software Bus

FACT ([R02] §2): pipes are created in three steps (reserve descriptor under the SB lock; `OS_QueueCreate` outside the lock, which makes the OSAL name visible; commit under the lock); `CFE_SB_CreatePipe` ignores the status of its `GetAppID` and stores the result as owner; `Subscribe` runs all checks and updates under one SB lock; transmit snapshots destinations under the SB lock, then calls `OS_QueuePut` per destination outside the lock; outcomes: delivered, queue full (accounting undone, `Q_FULL` event), no route (`NoSubscribersCounter++`, `SEND_NO_SUBS` event, **success** returned), MsgId limit, inactive destination; receive releases the pipe's previous buffer, gets from the queue and validates; no owner check on receive; FIFO per pipe; send → receive is ordered through the SB mutex for delivered buffers; no reverse edge; subscription reporting is a global mode that makes every later global subscribe transmit a `ONESUB` packet ([R03] §1, RC13).

| SB element | Model | Contracts |
|---|---|---|
| `CreatePipe` | three transitions (reserve, OSAL create, commit); owner = value returned by `GetAppID` (may be invalid) | K-REG (SB pipe names, `GetPipeIdByName` sees the OSAL queue before the descriptor commits, [R02] §1.16), K-ID (owner AppId) |
| `Subscribe(m, p)` | route(m) ∪= {p} under SB lock; emits events with `CFE_SB_Global.AppId` in the caller's thread | K-ID (SB identity at event sites), K-ROUTE publisher |
| `TransmitMsg/Buffer(m)` | buffer allocation requires a valid caller AppId; destinations snapshot (read of route(m)); per destination `queue.put` with outcome | K-ROUTE use (`route(m) ≠ ∅` and, if a consumer is named by configuration, `p_consumer ∈ route(m)`), K-CAP use (pipe depth < C), K-ID use (caller AppId valid) |
| `ReceiveBuffer(p)` | release previous buffer; `queue.get`; buffer valid until the next receive on p | buffer-lifetime template from `cfe_sb.h` L449-464 (use after implicit release) |
| Subscription reporting | global flag; when set, every global subscribe transmits `ONESUB` | RC13 (K-ROUTE extension): the order of SBN's enable vs TO_LAB's 37 subscribes decides `SBNSubPipe` load ([R03] §5.2 inference) |
| Shape-HB | delivered put → get of the same buffer descriptor (8-byte message value); per-pipe FIFO | — |

Route and capacity instances get `Allowed = unspecified` and the LOSS-UNDER-ORDER sub-label (§1.6) unless the consumer's documentation requires delivery. They are still classified (FIXED / SCHEDULE_DEPENDENT / PROTECTED loss), because the measured losses of the default bundle live here ([R03] §5.2).

### 3.12 Event Services

FACT ([R02] §3): `CFE_EVS_Register` memsets and fills `AppData[AppId % 32]` and publishes "used" last, **without** a lock; `SendEvent` resolves the caller's AppId (unresolvable → `ILLEGAL_APP_ID`); `SendEventWithAppID` trusts the passed AppId: invalid index → `ILLEGAL_APP_ID` (silent), valid index but unregistered or stale → `EVS_NotRegistered` (one event per slot and caller), registered → filter, squelch, generate; filter counters are incremented without a lock; EVS's own `EVS_AppID` is published at the end of its init and guarded at use ("which can happen if this is called by some other thread before CFE_EVS_TaskInit() runs", `evs/cfe_evs_utils.c` L613-623); event generation runs in the caller's context and calls ES (name), TIME (time) and SB (transmit).

DESIGN: template `evs.with_appid.identity` (K-ID), readiness predicate = EVS's own check (`EVS_GetAppDataByID(a) != NULL` and the slot's ID matches `a`), outcomes mapped to two distinct not-ready kinds ([R02] §3.2: "identity not yet published" vs "published but not registered"); `Allowed` = ∅ because the header names `CFE_ES_WriteToSysLog` as the channel for unregistered contexts (`cfe_evs.h` L168-172). The `EVS_AppID` guard is a HANDLED outcome (drop), reported separately ([R02] §3.4). Unlocked filter counters are memory-level findings (§3.18), not contract instances.

### 3.13 Table Services

FACT ([R02] §4): `Register` runs under the registry lock and sets `OwnerAppId` last; `Share` looks up by full name under the lock (not found → `INVALID_NAME`, event, bad handle); owner `Load` releases the lock **before** the check-then-claim of `LoadInProgress`, while the ground `LoadCmd` does the same check **under** the lock (atomicity candidate); `Validate` runs the app's validation function in the owner's task and writes the result without a lock, and the TBL task collects results by reading `State` then `Result` without a lock (ad hoc flag handshake); activation is applied by the owner's next `Manage`; `GetAddress` sets `LockFlag` and returns `NEVER_LOADED` before the first load; `ReleaseAddress` → `Update` is a real lock-flag protocol; lock order registry mutex → CDS mutex.

| Mechanism | Model | Contracts |
|---|---|---|
| Register / Share / GetAddress | registry namespace (§1.2) with states from the registry fields; outcomes from code | K-TBL: owner `Register` before sharer `Share` (RC3); owner loaded before sharer `GetAddress` |
| Owner load vs ground load | analysed code; the two check-then-claim paths are separate transitions | atomicity template `tbl.load.single_claim` (at most one `LoadInProgress` claim per table); needs a ground input in `In`, so latent in the default scenario |
| Validation / dump flags | `cfsk.store`/`load` tagged `assumed_sync` | handshake reported in the ad hoc class (§3.16), not as a violation |
| Lock flag | mutex + state; edge `ReleaseAddress` → `Update` that observed the flag clear ([R02] §8.1 R11) | buffer-lifetime: reads between `GetAddress` and `ReleaseAddress` are protected; reads after release are not |
| Notify-by-message | TBL → app MsgId transmit | K-ROUTE (each app subscribes before it registers the notification, [R03] §3 row 24) |

### 3.14 Critical Data Store and generic counters

FACT ([R02] §1.13-1.14): CDS registration holds the CDS mutex for the whole operation, is idempotent by name ("App.Name"), and is how restore after reset works; `CDSIsAvailable` is set in EarlyInit, which precedes every task; generic counters are registered in one critical section, looked up by name under the ES lock, and read without the lock.

DESIGN: CDS registry namespace with LIVE(restored) / LIVE(new) states depending on `Init` (K-PERSIST: RC8 sites in HS, DS, CS, MD; [R03] §4); the "register before lookup" order of generic counters is a K-REG instance (no producer/consumer pair exists in the bundle by default; HS XCT looks counters up by name, [R03] §2.10).

### 3.15 Time Services and FS background writes

FACT ([R02] §5, §6.1): TIME's internal 1 Hz pipeline is a chain of real edges (timer callback → binsem give → `Local1HzTask` → SB → TIME main → `Tone1HzISR` → binsem → tone task → SB); synch callbacks are stored without a lock and called in TIME's context; `CFE_TIME_CleanUpApp` clears the slot before the module is unloaded; the reference state is a 4-slot seqlock-like versioned buffer with a single writer in the default configuration; the FS background-write queue is a mutex-protected ring with callbacks run in the ES background task.

DESIGN: the 1 Hz pipeline is analysed code over L0 primitives (no special rules; it should classify its own internal instances PROTECTED — a calibration check); synch callbacks are `cfsk.call_indirect` resolved by the registration sites (actor-indexed call graph, [R05] §2.5 item 3), with the callback's events carrying `θ` = TIME and `α_attr` = the registering app; the seqlock is `assumed_sync` with "acquire only on validated read" ([R02] §8.1 R14); FS background requests are producer–consumer edges through the FS mutex, with requester callbacks executing in ES's background task (identity layering).

### 3.16 Ad hoc synchronization inventory

FACT: [R02] §8.3 lists eight sites; [R03] adds CF's retry loop and SCH_LAB's timer-margin protocol.

| Site | Model treatment | Edge |
|---|---|---|
| `WaitForSystemState` poll (`cfe_es_api.c` L584-605) | `await` + timeout region | observing exit only |
| `MainTaskSyncDelay` (`cfe_es_start.c` L871-915) | `await` under lock + timeout region (panic or continue) | observing exit only |
| `GetTaskFunction` (`cfe_es_apps.c` L570-596) | `await` under lock + timeout region | observing exit only |
| CF semaphore retry (`cf_cfdp.c` L1280-1291) | `await` with three outcomes (FOUND, `NAME_NOT_FOUND` after 25, other code → break) | observing exit only |
| OSAL `TaskPrepare` RESERVED wait | `await`, 5 attempts, broadcast-consumed (M_impl) | observing exit only |
| TBL validation/dump flags | `assumed_sync` store/load | assumed edge, flagged |
| TIME seqlock | `assumed_sync`, validated read | assumed edge, flagged |
| FS `IsPending` pre-check | unlocked read of a mutex-protected flag | none (benign pre-check) |
| ES `AppTimerMsec` countdown | timer-driven state transition (`await` with timeout) | none (time-based) |
| SCH_LAB first tick 1 s after init, first send only after a 1 Hz packet | timer callback task + data gate on a received message | message edge from the first 1 Hz packet only; the 1 s margin is **not** an edge |

Under σ_U every timeout region is reachable, so instances whose protection rests only on a margin (SCH_LAB's schedule reaching late-created apps, [R03] §6.3) are SCHEDULE_DEPENDENT under σ_U while observed protected under σ_E; both are reported, with the measured margin (0.12–0.9 s in 13 boots, [R03] §6.3) as σ_E evidence.

### 3.17 Coverage of the 15 readiness-contract kinds of [R03] §4

| RC | Kind ([R03] §4) | CAIRN class / template | Coverage | Notes |
|---|---|---|---|---|
| RC1 | Named OSAL/PSP object lookup | K-REG over OSAL namespaces and the PSP IODriver namespace (`osal.lookup_by_name.live`, `psp.iodriver.present`) | **covered** (M_api; RESERVED-window outcomes in M_impl) | C1; SCH_LAB timebase (expected PROTECTED); HS `linux_sysmon` (NO_PROVIDER under ENV Plat) |
| RC2 | Named ES lookup at steady state | K-REG over ES namespaces with `IsMatch` predicate; consequence analysis of HS AppMon's "not found = hung" | **covered** | RESERVED sentinel returned as success ([R02] §1.6) |
| RC3 | Named table sharing | K-TBL `tbl.share.registered` | **covered** | CS (latent by default table) |
| RC4 | Symbol / module by name | K-REG over the loader namespace | **covered** | MD dwell symbols (13th app vs later modules), MM on command, SBN lazy load |
| RC5 | MsgId subscribe-before-send | K-ROUTE (`sb.route.first_send`) | **covered**, LOSS-UNDER-ORDER unless a consumer requires delivery | 48 subscribe / 41 transmit sites; table-driven subscriptions resolved by table evaluation |
| RC6 | Identity / registration publication | K-ID (`evs.with_appid.identity`, `sb.caller_appid.valid`), K-SPAWN (`es.task.self_identity`) | **covered** | C3, C5, R2 |
| RC7 | ES phase barrier / readiness declaration | K-PHASE (`es.phase.declaration_implies_ready`) from `cfe_es.h` L400-403 | **covered** | SBN, CS, HS declaration patterns; timeout path under σ_U |
| RC8 | Persistent state restore across reset | K-PERSIST over `Init` and two-run composition | **partially covered**: two-run scenarios; CDS corruption cases are out (no reference defines them) | HS complement check; MD critical tables |
| RC9 | Parent ↔ child handoff | K-HANDOFF via L0 sync + tracked memory | **covered** for sync order; the unlocked count read is a memory-level finding (§3.18) | FM |
| RC10 | Timer / callback → task | K-HANDOFF; timebase task model | **covered** | SCH_LAB callback → countsem → main |
| RC11 | Library before client | program order of the startup thread | **covered as guaranteed** (expected PROTECTED certificate; a calibration check) | sample_lib |
| RC12 | File-object handoff (TOCTOU) | atomicity template over the file namespace (`osal.file.check_then_use`) | **partially covered**: needs ground commands or a polling-directory configuration in `In`; content of files written by external tools is havoc | CF's TOCTOU comment (`cf_cfdp_s.c` L447-463) is the code admission |
| RC13 | Global SB mode change | K-ROUTE extension (subscription-reporting mode) | **covered** | SBN enable vs TO_LAB subscriptions |
| RC14 | Bounded queue capacity vs burst | K-CAP with the max-backlog analysis (§4.6) and explorer witnesses | **covered**; capacity from Plat | 3 pipes measured overflowing ([BM] §3.3) |
| RC15 | Stale resolved reference after restart | K-REG with generations in the restart scenario family | **covered in scenario R(app, kind)** | CF semaphore ID, MD address, SBN symbols uncompensated ([R03] §4) |

Explicitly **out of scope**, with reason: cross-instance contracts over SBN peers and cpu2 (no peer ran in any recorded boot, [R03] §9 item 8; another process and network model would be needed); ISR, DMA and weak-memory effects (not present on the native POSIX target; [R05] §5.3); long-run resource exhaustion ([v0.1] §3.2).

### 3.18 Memory-level findings

FACT: [R01] §6.5 and [R02] §8.4 item 7 list intentional or acknowledged unsynchronized accesses (the lock-free `active_id` check, `name_entry` write, EVS filter counters, `EVS_NotRegistered` flag, task `ExecutionCounter`, `GetGenCount`, TIME flags, exception-ring counters, `ShutdownReq`). DESIGN: CAIRN is not a memory-race detector. Accesses to tracked locations that are conflicting and unordered in an explored execution are listed in a **separate memory-level class** with the `assumed_sync` tags shown, never counted as order violations, so that counts are not inflated ([R02] §8.4 item 7).

---

## 4. Algorithms

### 4.1 A1: basis assembly and configuration closure

**Input:** B. **Output:** `closed.ctc`. **DESIGN.**

1. **Link units.** For `core-cpu1` and each app/lib `.so`, read `link.txt`: whole-archive members are linked with `llvm-link`; lazily linked archive members (the BSP and PSP module archives) are added with `--only-needed` ([R07] §3.4 measured: closed core with 0 undefined `CFE_`/`OS_` symbols). Target membership comes from the CMake target of each compile command, never from directory names ([R07] §3.1).
2. **Task set T(B).**
   - PSP main thread (runs `CFE_ES_Main`, then the idle loop).
   - Core tasks: constant-evaluate `CFE_CORE_MODULE_OBJTAB` from the generated `cfe_static_module_list.c`; each entry with `TaskMain` gives a task created at `cfe_es_start.c:810` in table order.
   - Script apps and libraries: parse `cfe_es_startup.scr`; each `CFE_APP` line gives (module file, entry symbol, name, priority, stack); each `CFE_LIB` line gives an init function run in the startup thread.
   - Children: every reachable `CFE_ES_CreateChildTask` site with a constant entry argument.
   - Timebase tasks and timer callbacks: every reachable `OS_TimeBaseCreate` site; every `OS_TimerAdd` / `OS_TimerCreate` site (`osal/src/os/inc/osapi-timer.h` L88, L135) with its callback argument.
   - Library-internal threads not created through OSAL (e.g. `linux_sysmon`'s worker, [R01] §2.6): from `pthread_create` sites in PSP modules, flagged as raw threads.
3. **Indirect calls**, resolved in this order, each resolution recorded with its rule:
   1. configuration (task entries; trampoline `RealEntryFunc`; core `TaskMain`/`EarlyInit`; `CFE_Config_IterateAll` callbacks with the iteration count taken from the generated version table; SBN module ops structs via configuration table → module → symbol);
   2. registration sites (timer callbacks, TIME synch callbacks, TBL validation functions, FS background callbacks): the call target set is the set of functions registered at reachable registration sites, indexed by actor ([R05] §2.5 item 3);
   3. type layers: targets limited to functions whose address is stored into the same struct type and field as the call loads (the MLTA idea, [R05] §2.5, V-abs);
   4. unresolved → `unknown_effect(site)`; every instance whose slice contains it is UNKNOWN unless the unresolved call provably touches no tracked location (it has no reachable stores to globals and no sync calls in any candidate target).

   [R07] §4.2 counted 110 indirect call sites in the FSW set; the closure report states how many each rule resolves.
4. **Tables and constants.** Table initializers in `sample_defs/tables/*.c` and app `fsw/tables/*.c` are constant-evaluated by Clang; the result is cross-checked byte for byte against the built `.tbl` payload using the DWARF type layout of the GCC binary. A mismatch is a basis error. Platform macros are evaluated in the TU context of their use.
5. **Cross-check** (closure boot, §2.5): every observed `OS_TaskCreate` name/entry and every `OS_ModuleSymbolLookup` result must match T(B).

### 4.2 A2: skeleton extraction (LLVM dialect → `cfsk`)

**DESIGN.** Per link unit, the imported LLVM-dialect module is lowered by passes:

1. **Criteria.** Slicing criteria C = all OSAL API call sites (L0 vocabulary) ∪ cFE public API entries and returns (boundary markers, for identity and outcome) ∪ contract use sites ∪ stores to *support locations* (locations read by any instantiated `Ready` predicate, plus the locations from which bound arguments are loaded) ∪ reads performed by framework predicate functions referenced by contracts.
2. **Backward slice** over data and control dependence from C. Unsliced computation is removed. A value needed by the slice that cannot be computed from sliced code becomes `cfsk.havoc` (nondeterministic choice over its type, or over a finite domain when value analysis bounds it). Each havoc is counted per instance and reported.
3. **OSAL lowering.** Calls to the L0 vocabulary become `cfsk` sync/registry ops (M_api) or stay as analysed OSAL code with L-1 primitives (M_impl, for idmap, task, binsem, queue paths).
4. **Loops** are classified by pattern, each classification recorded:
   - *pure poll* — the sliced body is (delay)?(lock m)? reads of tracked locations (unlock m)? test-and-exit, with only a local counter written → `cfsk.await(pred) {success} {timeout}`; the bound attribute cites its reference constants;
   - *pure retry* — as pure poll, but the body calls a lookup whose outcomes are reads only (M_api OSAL lookups) → `cfsk.await` with one success region per outcome class (CF: FOUND, other-code, exhausted);
   - *constant trip count* — e.g. loops over `CFE_PLATFORM_ES_MAX_APPLICATIONS` records or over table entries → kept, bounded by the constant;
   - *main loop* — the app's `while (CFE_ES_RunLoop(...))` or receive loop → cut by the horizon h (§4.10);
   - *other* — bounded by a swept bound with UNKNOWN on bound hit.
   Under M_impl, OSAL lookups are not read-only (they can leak the table lock, [R01] F1), so loops containing them are **not** summarized there; they are unrolled to their reference bound (CF: 25, `cf_internal_cfg.h` L173).
5. **Inlining** of sliced callees into task skeletons where no recursion exists; recursion (if found) is summarized or turned into UNKNOWN.
6. **Verification.** The `cfsk` verifier checks: every `await` has a timeout region and a bound with provenance; every sync op names a declared object; every task has an entry and an owner expression; every `use` op names an instance; every op carries a `FusedLoc` with a GCC anchor or an "unanchored" flag.

**Lemma L1 (await summarization, σ_U), proof obligation.** If a loop matches *pure poll* or *pure retry*, then replacing it by `await` preserves the set of reachable model states and contract outcomes of all threads other than the poller, and the poller's states at loop exit. Sketch: failed iterations write no tracked location; their only interaction with other threads is a lock acquisition released in the same iteration, which can delay but not disable another thread's transitions in an untimed semantics, and cannot create a deadlock because no other lock is held across it (checked syntactically). The timeout exit is reachable in both versions after the first iteration under σ_U. To be proved in full in the paper; until then it is listed as an assumption (§6.2 A15).

**Lemma L2 (irrelevant critical sections), proof obligation.** A critical section whose body touches no object of the instance's cone (§4.5) and contains no blocking operation can be removed without changing reachable states or outcomes on the cone under σ_U. This is the same independence intuition that CP and WCP use for non-conflicting critical sections ([R04] §2.5, V-abs), applied to model exploration. Listed as A16 until proved.

### 4.3 A3: candidate generation (census)

```
for τ in templates (frozen set, §7.1):
  for s in call sites of the closed program matching τ.pattern (direct, or indirect resolved to the API):
    if s unreachable in the closed call graph from T(B) entries: emit INACTIVE(B), reason = blocking branch or missing entry
    key  ← value analysis of τ.ρ at s   // constants, table values, string literals, value flow from globals; else symbolic
    supp ← locations read by τ.Ready at s, plus the locations the bound arguments were loaded from
    for each conjunct P_i of τ.Ready (Ready is kept in conjunctive form):
      W_i ← model writes / registry transitions that can make P_i true for this key   // symbolic key ⇒ all writes in the namespace
      D_i ← writes / transitions that can make P_i false (delete, retire, reset, overwrite, teardown)
    if some P_i is false initially and W_i = ∅: emit NO_PROVIDER(B) naming P_i
    emit κ = (τ, s, key, {W_i}, {D_i})
```

Value analysis is interprocedural constant propagation over the IR with S1's table values; strings are compared by content; MsgIds come from evaluated macros. **Cost:** linear in the number of matching call sites times the cost of value queries, which are memoized per function. The census covers the core and all 15 apps of the ENV script; every instance is listed whatever its class ([R06] §5.4).

### 4.4 A4: tier-1 must-happen-before classification with certificates

**Definition.** For events `a`, `b` of the closed skeleton, `a ∈ MustBefore(b)` means: in every execution of M under σ_U in which `b` occurs, some occurrence of `a` occurred earlier and is HB-before it. MustBefore is computed as a greatest fixpoint over the skeleton CFGs of all tasks.

**Rules** (each has an id that certificates cite with the code site):

| Id | Rule | Semantic basis |
|---|---|---|
| T-PO | `a` dominates `b` in `b`'s task (every valid path from the task entry to `b` passes `a`) ⇒ `a ∈ MustBefore(b)` | program order |
| T-SPAWN | for task `t` created only at spawn sites `c₁…c_k`: `MustBefore(entry_t) ⊇ ∩ᵢ (MustBefore(cᵢ) ∪ {cᵢ})` | `OS_TaskCreate` → child start ([R01] §2.2; [R02] §8.1 R2) |
| T-OBS | for an *observing* event `o` (await success exit, successful lookup, consuming semaphore take, queue get, read in an `assumed_sync` flag test) on the branch where the observation succeeded, with complete writer set `W_o` (every model write that can produce the observed value): `MustBefore(o) ⊇ ∩_{x∈W_o} (MustBefore(x) ∪ {x})` | reads-from: some writer in `W_o` precedes `o`, and whatever precedes all of them precedes `o` |
| T-TERM | a CFG branch that reaches only terminal events (`CFE_PSP_Panic`, `abort`, exit with no relevant event) is excluded from meets at merge points | panic ends the execution ([R02] §1.3: core sync timeout → panic) |
| (no rule) | a lock acquisition alone yields no must-order | atomicity, not order ([R04] L5) |

If `W_o` is incomplete (an `unknown_effect` may write the location), T-OBS is disabled for `o`. Priority rules for σ_R1 are **not** in tier 1; σ_R1 is handled by the explorer.

```
MustBefore(entry of PSP main) ← ∅;  MustBefore(n) ← ⊤ (all relevant events) for every other node n
repeat
  for each task skeleton, for node n in reverse post-order:
    in ← ∩ over predecessors p not excluded by T-TERM of (MustBefore(p) ∪ {p})
    if n is a task entry:      in ← in ∩ T-SPAWN contribution        (the meet over its spawn sites)
    if n is an observing exit: in ← in ∪ T-OBS contribution
    MustBefore(n) ← in
until no set changes
```

**Classification** of κ with conjuncts `P_i`, writers `W_i`, disablers `D_i`:

- **FIXED_ORDER (certificate)** if for some conjunct `P_i` false initially: `W_i ≠ ∅` and `s_u ∈ MustBefore(x)` for every `x ∈ W_i`, and the not-ready path of `s_u` yields an outcome ∉ `Allowed` on every skeleton path (local check after `s_u`: status discarded, error exit, crash). Argument: if some `x ∈ W_i` occurred before the first occurrence of `s_u`, an occurrence of `s_u` would precede `x`, contradicting "first"; so at the first occurrence `P_i` is false. Certificate: for each `x`, the chain of rule applications from `s_u` to `x`.
- **PROTECTED (certificate)** if for every conjunct: either `P_i` holds initially and `D_i` is unreachable, or some `x ∈ W_i` has `x ∈ MustBefore(s_u)` and every reachable `d ∈ D_i` satisfies `d ∈ MustBefore(x)`.
- **INACTIVE / NO_PROVIDER** as in A3.
- Otherwise the instance goes to tier 2.

**Certificate checking.** Certificates are re-checked by a small independent checker (separate module, no shared code with the fixpoint solver) that verifies each edge against the model: T-PO by path enumeration on the edge's skeleton fragment, T-SPAWN by the spawn site list, T-OBS by recomputing `W_o`. A certificate that fails the check demotes the verdict to tier 2.

**Complexity.** Let N be the number of skeleton nodes and R the number of relevant events (bitset width). Each node update costs O((indegree + |W_o|) · R/64) word operations. Sets only shrink, so the total number of changing updates is at most N·R; worst case O(N·R·(d̄ + w̄)·R/64). The skeleton is per configuration, not per instance, so the fixpoint is computed once and all instances are classified from it.

### 4.5 A5: tier-2 bounded exploration

**Slice.** For instance κ, the *cone* is computed as a fixpoint: start from the tasks containing `s_u`, `W`, `D` and the objects in the support; add every task that writes a cone location or performs a sync operation on a cone object; add the sync objects and spawn chains through which those tasks reach their cone operations; repeat. This keeps third-party synchronization that the producer–consumer pair depends on, which [v0.1] §7.3 identified as the slicing risk, and keeps the full synchronization skeleton of the relevant tasks ([R06] T4 mitigation 2). Lemma L2 removes critical sections that are irrelevant to the cone.

**Engine.** Optimal DPOR with source sets and wakeup trees (Abdulla, Aronis, Jonsson, Sagonas, POPL 2014; "provably optimal", V-abs in [R05] §2.2) over the `cfsk` interpreter:

- *Transition*: one op of one task: a sync op, a tracked load or store, an `await` outcome, a spawn, a registry op, a queue op. Boundary markers and local computation are invisible.
- *Dependence*: two transitions are dependent if they act on the same object (mutex, semaphore, queue, registry key, tracked location) and at least one changes it, or one can enable or disable the other (spawn, give/take, put/get, lock/unlock, await vs the writes its predicate reads). Under σ_R1, every transition that can change a higher-priority thread's readiness is also dependent on all lower-priority transitions (a conservative over-approximation that keeps DPOR's coverage under the scheduler constraint).
- *Monitor*: at each `cfsk.use` of κ, evaluate `Ready` on the current state; after the use's callee returns, evaluate the outcome against `Allowed`; per execution record violating / conforming / handled, the shape (§1.7) from vector clocks of the shape-HB relation, and the consequence.
- *Deadlock check*: a state in which every unfinished cone task is blocked and at least one is blocked on an object no unfinished task can release is reported as a deadlock (generic property, independent of contracts). This is how M_impl exposes the OSAL lock-leak path F1 (Appendix A.2).
- *Classification*: once one violating and one conforming execution reaching `s_u` are found, the instance is SCHEDULE_DEPENDENT and exploration may stop (existence witnesses). FIXED_ORDER and PROTECTED require the exploration to complete within b. Budget exhaustion before that gives UNKNOWN(budget) with the partial counts.

**Two-query view.** The explorer answers Q₁ = "∃ e ∈ E_κ: V_κ(e)" and Q₂ = "∃ e ∈ E_κ: ¬V_κ(e)" ([R05] §2.3): (yes, no) FIXED_ORDER; (yes, yes) SCHEDULE_DEPENDENT; (no, yes) PROTECTED; (no, no) INACTIVE_b.

**Optional SMT backend** (OPEN-3: no solver is installed, [R07] §2.4). Encoding over the unrolled slice in the style of Alglave et al. (CAV 2013, V-abs): an integer clock per event; program-order constraints; reads-from choice variables with coherence ("no intervening write") constraints; sync constraints (lock exclusion as non-overlap of critical-section intervals, give/take and put/get matching, spawn); typestate evaluated from reads-from; Q₁ and Q₂ as two satisfiability queries. Emitted through the MLIR `smt` dialect and `ExportSMTLIB` (probe works, [R07] §2.2). Used as a cross-check of the explorer on small slices, and for slices with symbolic table inputs (e.g. a non-empty versus empty CF `sem_name`), which SMT handles naturally ([R05] §2.3).

**Complexity.** Optimal DPOR explores exactly one execution per Mazurkiewicz trace; the number of traces can be exponential in the number of dependent event pairs in the slice. Each explored execution costs O(L · n) for vector-clock maintenance, with L events and n tasks. The SMT encoding has O(|E|²) ordering atoms plus O(|R|·|W|) reads-from atoms; deciding it is NP-complete in general (prediction beyond sync-preservation is NP-complete, [R04] §2.5 SyncP, V-abs). Cones, Lemma L1 and Lemma L2 are the reductions; their effect is reported per instance (explored executions, time, bound reached).

**No preemption bound by default.** The horizon already makes the slice finite. If a preemption bound p is introduced for scalability, BPOR (Coons et al. OOPSLA 2013, V-abs) is used because naive DPOR plus a bound is unsound within the bound ([R05] §2.2), and p is swept from 0 upward and reported, as SCTBench recommends ([R05] §2.1).

### 4.6 A6: capacity (K-CAP) analysis

**DESIGN.** For pipe q with effective capacity C_q (Plat; [BM] §3.1), an overflow is a delivered-attempt `put` that finds depth = C_q.

1. **Tier-1 upper bound.** Take the partial order of q's put and get occurrences induced by MustBefore. The largest backlog any execution can reach is at most the maximum, over downward-closed sets I of that order, of (puts in I − gets in I), because MustBefore under-approximates the real ordering constraints. That maximum is a maximum-weight closure problem, solvable by a reduction to minimum cut (standard algorithmic fact; citation not verified in AR1). If the bound is ≤ C_q, the instance is PROTECTED (sound, since the bound over-approximates).
2. **Tier-2 witness.** Otherwise the explorer runs a directed search for a schedule realizing the overflow (puts early, gets late, within real constraints), giving SCHEDULE_DEPENDENT or FIXED_ORDER loss with a witness, or UNKNOWN(budget).
3. **Observation.** SB's `PipeOverflowErrorCounter` and per-pipe peak depth read from memory at stop (the M1 method, [BM] §2.3) are the cheap oracle; observed counts per boot (4–49 in the default configuration, [BM] §0) are σ_E evidence.

### 4.7 A7: predictive feasibility of reversed orders over observed traces

**DESIGN.** For a pair observed in one order (a CloudRaid-style "always one order" pair, [R04] §2.8; e.g. a lookup that always succeeds after its create), D2 asks whether the reverse order is an execution of M consistent with the trace:

1. Build a *trace-constrained model*: each thread follows its observed path up to and including the first event whose outcome could change under reordering (a tracked read, a lookup, a receive, a semaphore take); after that event it may follow any skeleton path. This is the control-flow abstraction that makes predictions producible in RVPredict and maximal causal models ([R04] §2.5, V-abs).
2. Search with DPOR for an execution in which the target pair is reversed (lookup before create; use before publish; send before subscribe; second put before the drain).
3. The output is a witness execution of M (no false positive relative to M); whether it is feasible on the binary is decided only by confirmation (A9). The registry key is the conflict object and locks are atomicity, so lock-order reversals are allowed, unlike sync-preserving prediction ([R04] L5).

Predictions seed and localize confirmation; they never set the model class by themselves.

### 4.8 A8: model conformance checking

**DESIGN.** For an observed boot, D1 rebuilds per-thread sequences (by per-thread sequence numbers), identifies objects by OSAL IDs and messages by their 8-byte descriptor values, and asks: is there an execution of M whose projection onto the probed event alphabet equals the observed per-thread sequences, with the observed outcomes? The check is a search with the explorer, restricted to the observed per-thread sequences, using timestamps only as a heuristic search order (try timestamp order first; [R04] §2.1: physical time is evidence of a schedule, not an ordering guarantee). Results:

- *conforming*: a witness linearization exists; the boot is a member of `L(M)`;
- *non-conforming*: the shortest per-thread prefix that no model execution produces is reported as a model fault; affected instances become UNKNOWN(conformance);
- *undecided* (budget): conformance UNKNOWN for that boot.

Conformance is what makes the model's verdicts about the real binary defensible: it is the measured link between M and the GCC executable ([R06] T3 "cross-check against one boot trace").

### 4.9 A9: confirmation planning and execution

**Which verdicts get controlled confirmation** (DESIGN):

| Verdict | Action |
|---|---|
| FIXED_ORDER (certificate) | passive observation only (reachability and outcome). A conforming execution does not exist under the same B; a "violating vs conforming" pair needs a configuration-change experiment, labelled as such ([v0.1] §8.5) |
| PROTECTED | none beyond conformance on ordinary boots |
| SCHEDULE_DEPENDENT | directed confirmation of the violating witness; if natural boots show only the violating order, directed confirmation of the conforming witness too |
| UNKNOWN(model: havoc, unresolved, conformance) | observation plus directed attempts at predicted witnesses; results are evidence; the class stays UNKNOWN until the model is fixed |
| UNKNOWN(budget) | PCT runs (F1 variant) as probabilistic search |
| LOSS-UNDER-ORDER | observation through SB counters |

Order of work: by consequence (crash or deadlock > app error exit > silent loss > event then continue), then by model uncertainty. The confirmation budget is a resource cap, recorded.

**Gate placement rule** (DESIGN; answers [R06] §3 v0.1 §10 "Missing"). To force "event `a` of thread A after event `b` of thread B":

```
candidates ← sites g in A's skeleton between A's last event in MustBefore(b) and a, such that
   lockset(A, g) = ∅ under M_impl            // no program lock, no idmap table lock, no binsem internal mutex,
                                             // no timebase handler_mutex, no console lock ([R01] §6.3)
   g is not in a signal handler or a timer callback, and not before OS_TaskPrepare registration
   g has a GCC anchor with a probe-safe address (function entry+4, or an emulated instruction; [R07] §4.7)
if candidates = ∅: apply the rule symmetrically (hold B before b's enabling event instead)
choose g closest to a (shortest hold)
release when: b is observed by the recorder, or B is blocked or has exited
watchdog: release after T_ref (the reference timeout of the protocol being bypassed, §4.10); record "gate-unreached"
```

**Mechanisms.**

- **F0 directed holds (default).** A root-side ptrace controller seizes the threads, sets an int3 at the gate address in the unmodified GCC binary, and keeps only the gated thread stopped (ptrace stops are per thread; [R07] §5.2). A directed confirmation needs a handful of stops, so even the measured gdb cost of about 203 µs per stop is negligible per run ([R07] §4.7). Stepping over the breakpoint uses the expensive debug-exception path (about 32–34 µs); it happens once per hold.
- **F1 gate variant (for PCT and systematic native exploration).** A recorded deviation: `-Wl,--wrap=<sym>` on the core and every app link, plus one gcc-built C99 object defining `__wrap_*` gates in `core-cpu1` ([R07] §4.2). Gates block in the calling thread until the controller releases them (handoff 1.1 µs same CPU, 10.5 µs across CPUs, [R07] §5.1). The 88 intra-TU API call sites (2.3%) that `--wrap` cannot see ([R07] §4.2) are covered by F0 uprobes for observation; they cannot be gated in F1 and are listed.
- **PCT under F1.** The controller assigns random priorities to the n gated threads and d−1 priority-change points among k gated events (n and k measured in a calibration run); at most one gated thread runs between gate points. Each run finds a depth-d bug with probability ≥ 1/(n·k^(d−1)), relative to the gate granularity (PCT, ASPLOS 2010, V-abs; mechanism P in [R05]; claim template C-4 in [R05] §5.2). Lu et al.'s finding that 92% of studied non-deadlock bugs need ordering of at most 4 accesses ([R04] §2.6, V-abs) motivates small d; it is a hypothesis for cFS, not a fact.

**Timeouts under control.** Holding a thread past a real-time timeout produces an execution that σ_U allows (timeouts are nondeterministic there). Such a run is labelled "confirmed-controlled (σ_U)". Its frequency in natural ENV boots is a separate, empirical question.

**Perturbation accounting.** Each controlled run records hold durations, probe hits by cost class, and the order-sensitive outcomes it did not target (drop counters, init completion order, the R2 use set), so that collateral changes are visible.

### 4.10 Bounds and their references

| Bound | Value | Reference | Status |
|---|---|---|---|
| ES task-record poll in the trampoline | at most 21 checks, each after a 50 ms delay (timeout 1000 ms decremented by 50 per failed check; summarized as `await` under σ_U) | `cfe_es_internal_cfg.h` L860, L879; `cfe_es_apps.c` L570-596; [R05] §1 F4 | from-reference |
| `WaitForSystemState(s, T)` | ⌈T / 50⌉ polls; T from the call site (TO_LAB 10000, HS 65000, CS 60000, SBN 10000 and an unbounded retry loop around a 1000 ms wait) | `cfe_es_api.c` L584-605; [R03] §2.3, §2.10, §2.14, §2.15 | from-reference |
| Script-phase syncs | 1000 ms each, two phases, log and continue | `cfe_es_internal_cfg.h` L878-879; `cfe_es_start.c` L210-228 | from-reference |
| Core-phase sync | 30000 ms, panic on timeout | `default_cfe_core_private_internal_cfg.h` L68; `cfe_es_start.c` L835-846 | from-reference |
| CF semaphore retry | 25 attempts × 100 ms | `cf_internal_cfg.h` L172-185 | from-reference |
| OSAL RESERVED wait | 5 attempts (10, 40, 90, 160 ms timeouts, then fail) | `osapi-idmap.c` L475-480; `os-impl-idmap.c` L151-169 | from-reference |
| ES kill timer | 5 scans × 1000 ms | `cfe_es_internal_cfg.h` L199, L229 | from-reference |
| Effective pipe capacity | min(requested depth, 10) per pipe | [BM] §3.1 (measured), `os-impl-queues.c` L59-63, L94-105 | from-reference (ENV Plat) |
| Name copies in probes | `OS_MAX_API_NAME` 20; paths `CFE_MISSION_MAX_PATH_LEN` 64 | build `osconfig.h` L45-233 ([R01] header); [BM] §2.4 | from-reference |
| Table sizes (task, queue, app, pipe, route, destination, TBL) | `OS_MAX_TASKS` 64, `OS_MAX_QUEUES` 64, `MAX_APPLICATIONS` 32, SB 64 pipes / 256 MsgIds / 16 destinations, TBL 128 / 256 / 4 | [R01] header; [R02] §0 | from-reference |
| Version-event callback count | number of entries in the generated module version table | `cfe_module_version_table.c` of the basis ([R2c] §4.2, §7) | from-reference (per B) |
| Main-loop horizon h | iterations of each task's main loop after its init | none | **unspecified_by_reference**: swept h = 0, 1, 2, … until the budget; the largest completed h is part of b |
| Preemption bound p | none by default; if used, swept from 0 | none | unspecified_by_reference (swept, BPOR) |
| Exploration and confirmation budgets (time, memory, runs) | resource caps | none | recorded resource choices; exhaustion yields UNKNOWN, never a class |
| Gate watchdog T_ref | the reference timeout of the protocol the hold bypasses (rows above), or the startup bound 30000 ms when no narrower one applies | as in the rows above | from-reference ([POL] L22) |

---

## 5. Observation and instrumentation

### 5.1 Mechanism choice by fidelity

FACT ([R07] §7 fidelity scale): F0 = unmodified binary and conditions, only timing perturbed; F1 = same sources and compiler, link flags or a code-generation option changed; F2 = sources rewritten; F3 = a different compiler for the executed binary.

| Purpose | Mechanism | Fidelity | Unit cost (measured, [R07] §4.7, §5.1) | Decision |
|---|---|---|---|---|
| Observation of API events, closure boot, conformance, contract outcomes | perf uprobes at **function entry+4** (after `endbr64`, on the emulated `push %rbp`) with eBPF programs reading arguments | **F0** | 0.57 µs per hit at entry+4; 31.7 µs at `endbr64` or a non-emulated instruction; eBPF program cost on top: not measured | **default** |
| Return values | uretprobe, or a uprobe at the caller's instruction after the `call` | F0 | not measured | OPEN-7: measure in M4; prefer callee-side return probes only where emulated |
| Plain stores not covered by any API (e.g. `SystemState` writes) | uprobe at the store instruction, value read from the source register as decoded by the planner | F0 | 31.7 µs per hit (store instructions are not emulated; INFERENCE) | used for the handful of such stores per boot |
| Targeted value watch during confirmation | perf hardware breakpoint | F0 | about 34 µs per hit; 4 debug registers | confirmation only |
| Directed holds | ptrace per-thread stop | F0 | about 203 µs per stop with gdb; a purpose-built controller not measured | default for confirmation |
| Systematic control, PCT | `--wrap` gates | F1 (deviation: link options) | 1.1–10.5 µs per handoff plus controller | only for PCT / systematic native exploration (OPEN-4) |
| Not used | `LD_PRELOAD`/`LD_AUDIT` (cannot interpose executable-defined API symbols; refused by the harness, `env/run_cfs.sh` L154-158), source probes (F2; CORD-style patches), clang-built executables (F3), `-finstrument-functions` (+15.7% `.text`, function-level only), rr (no hardware PMU), kprobes (not configured) | — | — | rejected with [R07] §4.3-4.4, §4.7 reasons |

DESIGN: observation runs never load tool code into the cFS process. The observer runs as root alongside the README-mode `ubuntu` process ([R07] §4.7; root-side observer acceptance is OPEN-5).

### 5.2 Probe planner (model-derived)

**DESIGN.** P0 derives the probe set from the model, never from a case list:

1. **Conformance alphabet:** every L0 OSAL API entry and return reachable from the tasks in the census cones (sync, registry, queue, timer, module ops).
2. **Contract outcomes:** every instance's use site (API entry with the bound arguments; return with status) and every publisher write that is not already an API event.
3. **Identity bootstrap** (§5.5).
4. **Await exit reasons:** returns of `CFE_ES_WaitForSystemState` (status), so the timeout of the void `WaitForStartupSync` becomes visible.

Each probe has: link unit and build id; address (from S2, entry+4 or an emulated instruction where possible); record kind; argument extraction recipe (x86-64 SysV argument registers at entry+4, which `push %rbp` does not change; dereference recipes for out-pointers captured at entry and read at return; bounded string copies with `bpf_probe_read_user_str`, bounded by the reference lengths of §4.10). The plan is hashed into every trace header.

**Placement safety** (from [R01] §6.3): BPF code runs in the kernel and cannot call into the process, so the classic hazards of in-process probes (calling OSAL lookups, `OS_printf`, taking a lock inside an OSAL table-locked region, cancellation while holding a probe lock) do not arise. What remains is timing: a trap inside a table-locked region lengthens the hold and can consume OSAL's bounded RESERVED-wait attempts; the planner therefore prefers entry probes (outside OSAL's internal locks) and flags any probe whose address lies inside an idmap critical section, a timebase callback (`handler_mutex` held) or the binsem internal mutex section.

### 5.3 Event record format

```c
/* include/cairn/abi/trace_abi.h — C99, shared by BPF programs (clang-18 -target bpf),
   the gcc-built gate runtime and the C++ analyzer. Little-endian. */
#include <stdint.h>
struct cairn_ev {
    uint16_t kind;     /* CAIRN_EV_* */
    uint16_t flags;    /* ENTER / EXIT, TRUNCATED, MECH_F0 / MECH_F1, ... */
    uint32_t tid;      /* kernel thread id */
    uint64_t tseq;     /* per-thread sequence number: the program-order token */
    uint64_t t_ns;     /* CLOCK_MONOTONIC; evidence and search heuristic only, never HB */
    uint32_t site;     /* site id from the anchor map (probe address -> source site) */
    uint32_t obj;      /* OSAL/cFE object id when the event names one */
    uint64_t a[4];     /* arguments, return value, observed value, message descriptor */
};
/* static_assert(sizeof(struct cairn_ev) == 64) in every consumer */
```

| Kind | Payload |
|---|---|
| `THREAD_START` | tid, OSAL id from `OS_PthreadTaskEntry`'s argument (child side) |
| `TASK_CREATE` (enter/exit) | creator tid; name (string record); entry pointer; priority; status; new OSAL id |
| `ES_START_TASK` (enter/exit) | name; `EntryFunc`; `ParentAppId`; returned ES TaskId |
| `REG_CREATE`, `REG_LOOKUP`, `REG_DELETE` | namespace; key (string record or id); status; returned id |
| `MUTEX`, `BINSEM`, `COUNTSEM` ops | object id; op; status (timeout, flushed) |
| `QUEUE_PUT`, `QUEUE_GET` | queue id; the 8-byte message value (SB buffer-descriptor pointer); status (delivered, full, timeout) |
| `TIMER_ADD`, `TIMER_SET`, `TB_CALLBACK` begin/end | timer id; callback pointer; creator tid |
| `MODULE_LOAD`, `SYMBOL_LOOKUP` | names; status; address |
| `API` enter/exit | API id; up to four bound arguments; return status (for contract templates) |
| `STORE` | site; value written (planned stores only) |
| `AWAIT_EXIT` | site; exit reason (observed / timeout) |
| `STR` | up to 32 bytes inline; chained for longer names |
| `MARK` | run-phase markers from the harness (launch, OPERATIONAL line read, stop sent) |

### 5.4 Per-task ordering, buffers and loss accounting

**DESIGN.** BPF programs write records into a BPF ring buffer drained continuously by the observer. Per-task order is carried by `tseq`, kept in a BPF hash map keyed by tid and incremented on every record of that thread; a gap in `tseq` is a detected loss for that thread. A failed ring-buffer reservation increments a per-CPU loss counter. The trace footer carries both. Ring-buffer size is `unspecified_by_reference`: it is chosen from a calibration run's hit counts so that the calibration run loses nothing, and both the size and the calibration result are recorded. A verdict whose relevant window contains a loss on a relevant thread is downgraded to "observation incomplete".

For the F1 gate variant only, the gcc-built runtime keeps per-thread preallocated buffers with the same record layout (no heap, no file I/O, no EVS/SB, no global lock on the hot path, drop-and-count on overflow; [v0.1] §9.2). Whether `__thread` storage is accepted under the build's `-std=c99 -pedantic -Werror` is to be checked (OPEN-7); the fallback is a slot array indexed at task start.

### 5.5 Identity bootstrap without `GetAppID`

**DESIGN.** Identity is reconstructed offline from creation events, never by calling cFS from the probe (calling `GetAppID` early would itself hit the C5-class window, [v0.1] §8.4):

1. PSP main thread: tid = pid of `core-cpu1`.
2. `TASK_CREATE` exit gives (creator tid, name, entry, OSAL id).
3. `THREAD_START` in the child gives (child tid, OSAL id) before the child's `OS_TaskPrepare` registration.
4. `ES_START_TASK` gives (name, `EntryFunc`, `ParentAppId`, ES TaskId); ES TaskIds derive arithmetically from OSAL ids ([R02] §6.2).
5. Timebase threads: the OSAL timebase thread entry gives (tid, timebase id); callbacks are attributed through `TIMER_ADD` records (creator tid → `α_attr`).
6. Raw threads (console, `linux_sysmon` worker) are identified by their creation site.
7. `ς(e)` comes from the anchor map (site → link unit → module); `ι(e)` from the bound arguments (e.g. `CFE_EVS_SendEventWithAppID`'s `AppID`).

### 5.6 Timestamps versus causal tokens; reconstructing HB

**DESIGN.** Program order comes from `tseq`. Cross-thread order comes only from matched synchronization interpreted by M: a queue put and get are matched by the descriptor value and per-pipe FIFO position; semaphore and mutex operations by object id and the model's matching rules (§3.4); task creation by OSAL id. **Outcome-anchored reads-from** removes the need to place probes at exact linearization points: a successful lookup must read from a publication that precedes it under M, even if the probe at the publication fired later than the lookup's probe (the case [R01] §6.1 warns about: `OS_EVENT_RESOURCE_CREATED` fires after the lock release). Likewise R2's use passes AppId 0, which only the EarlyInit zero can supply, so in that execution the use reads from the initial value and precedes the publication. Timestamps are used for perturbation statistics, as evidence of a schedule, and as a search heuristic in conformance checking; never as HB ([R04] §2.1, L1).

### 5.7 Cost model with measured unit costs

**DESIGN.** Following [v0.1] §11.2, the added execution time of thread i is planned as ΔC_i ≈ Σ_j n_ij · c_j, with c_j from [R07]:

| c_j | Value | Source |
|---|---|---|
| uprobe at entry+4 (trap + emulation) | 0.57 µs | [R07] §4.7 |
| uprobe at a non-emulated instruction | 31.7 µs | [R07] §4.7 |
| HW watchpoint hit | about 34 µs | [R07] §4.7 |
| ptrace stop (gdb) | about 203 µs | [R07] §4.7 |
| gate handoff (F1) | 1.1 µs same CPU, 10.5 µs across CPUs | [R07] §5.1 |
| eBPF program body per hit | **not measured** | to be measured in M4 |

Worked estimates (INFERENCE, not measurements):

- *R2-relevant probes only.* About 230–260 SB-API uses per boot ([R2c] §7: 240–258 `sb.appid` uses), two publications, 24 rejections, ~25 task creations: on the order of 300 entry+4 hits, i.e. about 0.2 ms of traps per boot, against an OPERATIONAL time of 1613–1636 ms ([R2c] §4.4). This is two to three orders of magnitude below gdb, consistent with [R07] §8 item 2.
- *Full L0 + cFE boundary plan.* The narrow CORD probe set logged 626–632 events per boot ([R2c] §4.1); a generic boundary plan will log more. [R06] T6 guesses 10–100× without measurement. At 0.57 µs per hit, 63,000 hits would be about 36 ms of traps spread over 27 threads. Whether that shifts the order-sensitive outcomes is exactly what §5.8 measures; no overhead claim is made before that.

### 5.8 Perturbation protocol

**DESIGN.** Groups (extending [v0.1] §11.4):

| Group | Configuration | Separates |
|---|---|---|
| A | unmodified binary, no observer | reference |
| B | uprobes registered with empty BPF programs | trap cost |
| C | full observation plan | recording cost |
| D | directed holds (F0) or gates (F1) | control effect |
| W | the F1 `--wrap` build with gates disabled | code-layout effect of the variant |

Measures per run: OPERATIONAL time; `PipeOverflowErrorCounter` and `NoSubscribersCounter` read from memory at stop ([BM] §2.3); which pipes overflowed (`SBNSubPipe` vs `TO_LAB_TLM_PIPE`, [BM] §3.3); app init-completion order (distance from script order); the R2 use set (24 per boot in the ENV basis). Distributions are compared between groups (Mann–Whitney U for counts and times, Fisher's exact test for incidence), following the guidance of Arcuri & Briand (ICSE 2011, V-abs in [R05] §6.5). The number of runs per group is `unspecified_by_reference`; Klees et al. used about 30 trials (P, [R05] §6.5) — cited as a literature norm, final N is OPEN-9. Kernel drift (`-fc-v80` → `-fc-v114`) requires re-measuring group A before comparisons ([R07] §1).

---

## 6. Guarantees

All statements are relative to: the basis B; the model M (version-hashed, every rule cited) at level M_api or M_impl; the semantics σ; the bound b; and the enumerated assumptions A of §6.2. Terminology follows [R05] §0.5: "over-approximate relative to M and A", "complete up to b in M"; never a bare "sound".

### 6.1 Claims CAIRN is designed to support

- **G1 — Census coverage (soundy over-approximation).** Every call site in the closed program of B that matches a frozen template is in the census with a class, or is listed with the reason it could not be instantiated (unresolved entry, unresolved indirect call, macro-origin site without an anchor). This is claim template C-1 of [R05] §5.2, weakened by A.
- **G2 — Certificates.** Every tier-1 FIXED_ORDER or PROTECTED verdict comes with a certificate whose edges are each justified by a rule of §4.4 and a code site, and which an independent checker has re-validated. Such a verdict holds for **all** executions of M under σ_U (no bound), relative to A. R2's FIXED_ORDER verdict is of this kind (Appendix A.1).
- **G3 — Bounded exactness of tier-2 classes.** Within M, σ and b, the explorer explores one execution per Mazurkiewicz trace (optimal DPOR, V-abs), so FIXED_ORDER, PROTECTED and SCHEDULE_DEPENDENT are exact for the bounded model, given Lemmas L1–L2. This is claim template C-2 of [R05] §5.2.
- **G4 — Conformance.** M has been checked against every observed boot used in the evaluation; non-conforming regions are reported, and verdicts depending on them are UNKNOWN.
- **G5 — Confirmed reports.** A report labelled `confirmed-controlled` was produced on the unmodified GCC binary of B with only the declared ptrace holds at the listed sites (F0), or on the declared F1 variant, and the contract outcome was observed by the recorder. Confirmed reports therefore contain no false alarm relative to the recorder's outcome oracle ([R05] §5.2 C-3, the replay-validation argument of RacePro and DESCRY).
- **G6 — Probabilistic search.** Each PCT run with measured n and k finds a depth-d bug (with respect to gated events) with probability at least 1/(n·k^(d−1)) ([R05] §5.2 C-4).

### 6.2 Soundiness manifest: the enumerated unsound or limiting choices A

| Id | Choice | Effect | Mitigation / visibility |
|---|---|---|---|
| A1 | Sequentially consistent interleaving semantics | C11 data races used as synchronization (`SystemState`, TIME seqlock, TBL flags) are treated as synchronization | tagged `assumed_sync`; listed in every certificate that uses them ([R06] T8) |
| A2 | Facts from the Clang front end, executed binary from GCC | possible front-end differences | anchor audit (§4.1, M0 exit criterion); exceptions listed |
| A3 | Indirect calls resolved by configuration, registration sites and type layers | missed targets if resolution is wrong | closure boot cross-check; unresolved sites → UNKNOWN |
| A4 | libc and kernel behaviour summarized (pthread, sem, mq, timers) | e.g. equal-priority FIFO of POSIX queues is not verified against the standard ([R04] §8 Q2) | the SB mutex makes the `mq_*` memory-synchronization question moot for delivered messages ([R02] §2.6) |
| A5 | Signals and exception handling as environment events only | PSP exception path, `SIGUSR1` notifications | listed; out of the first scope |
| A6 | Time abstracted (σ_U, σ_R1 untimed); no execution-time bounds | timing margins are not protection in the model | σ_E observations report margins separately |
| A7 | External inputs only as declared `In` | ground-driven paths are latent unless a scenario declares them | scenario families §7.4 |
| A8 | Inline header helpers assumed pure | they are not linker symbols ([R07] §4.2) | the list of helpers checked in [R07] §4.2 |
| A9 | Horizon b (main-loop iterations h) | contracts beyond h are not decided | UNKNOWN_b; h reported |
| A10 | M_impl covers OSAL idmap, task, binsem and queue paths only | other primitives at API level in both | listed |
| A11 | Table values from the basis; runtime table loads only via `In` | — | — |
| A12 | Late-loaded modules resolved from configuration | wrong configuration → wrong targets | closure boot |
| A13 | Single process; cross-instance (SBN peers, cpu2) out of model | — | listed as out of scope (§3.17) |
| A14 | No weak memory, ISR or DMA | — | out of scope ([R05] §5.3) |
| A15 | Lemma L1 (await summarization) assumed until proved | — | can be switched off per instance (unroll to the reference bound) to test its effect |
| A16 | Lemma L2 (irrelevant critical sections) assumed until proved | — | can be switched off per instance |

### 6.3 What is not claimed

1. Soundness or completeness without the qualifiers above; "detects all races"; "no false positives" for model verdicts ([R06] §6.3 item 1).
2. That an instance without an alarm is safe. INACTIVE, NO_PROVIDER and UNKNOWN are not "safe", and PROTECTED holds only within (B, σ, M, b).
3. Any transfer of verdicts across semantics (σ_R1 ↔ σ_U ↔ σ_E) or across model levels (M_api ↔ M_impl) ([R06] §6.3 item 14).
4. Determinism from k-of-k runs; determinism is claimed only through a certificate ([R06] S9).
5. That R2 is a race; it is classified FIXED_ORDER, a dependency error ([R06] §6.3 item 5).
6. That existing tools fail on any case; every baseline cell is `NOT_RUN` until run ([R06] §6.3 item 3).
7. That MLIR improves detection or precision ([R06] §6.3 item 2); the gate in §2.4 compares engineering criteria only.
8. Generality from the five design cases; frequency claims for cFS ([R06] §6.3 items 4, 7).
9. "New bugs" for derived, role-based, seeded or configuration-changed tests ([R06] §6.3 item 8).
10. That C3 or C5 are reproduced on v7.0.1; their buggy code is absent there ([R06] §6.3 item 9).
11. Flight-representative timing or overhead from Linux `SCHED_OTHER` in permissive mode ([R06] §6.3 item 10).
12. "Lost packets" for rejected event requests: EVS rejected the request before any packet was built ([R2g] §2; [R06] §6.3 item 15).
13. Novelty of HB/VC, DPOR, PCT, SMT prediction, lockset or delay injection as mechanisms ([R04] §5).
14. That OSAL F1 (lock leak) or F7 (silent task loss) are bugs, before the confirmation experiments of [R01] §7 are run.
15. Detection of memory-level data races in general (§3.18).

---

## 7. Openness

### 7.1 Contract library: sources and anti-circularity bookkeeping

**DESIGN.** Templates are written per **API and resource class**, never per case, and instantiated automatically (§4.3). Sources, in priority order ([R06] §5.3):

1. **API documentation at the pin.** cFE core API headers: 138 `\par Assumptions` blocks and 429 `\retval` lines; OSAL public headers: 593 `@retval` lines (spot check §0.3). Each `\retval`/`@retval` defines a documented outcome; each assumption block is a candidate precondition. Examples used here: `cfe_evs.h` L168-172 (identity precondition and the syslog alternative), `cfe_es.h` L400-403 (declaration implies readiness), `cfe_sb.h` L449-464 (receive-buffer lifetime). S1 extracts these blocks with their location and hash; a researcher turns each into an RCL clause, recording the hash of the source text.
2. **Code admissions.** CF's "start up race condition" comment (`cf_cfdp.c` L1272-1278); CF's TOCTOU comment (`cf_cfdp_s.c` L447-448); EVS's guard comment (`evs/cfe_evs_utils.c` L613-623); TBL's "other APIs do not have to lock registry" rationale (`tbl/cfe_tbl_api.c` L145-149); TIME's concurrent-unregistration comment (`time/cfe_time_tone.c` L1322-1325) ([R02], [R03]).
3. **Retry and timeout patterns** in callers as evidence that a not-ready outcome is expected (they define `Allowed` when the caller's own documentation is silent), labelled `code-pattern`.
4. **Issues**, labelled historical; never applied to the pinned version automatically ([v0.1] §6.2).
5. **Belief inference** for unwritten contracts (Engler et al. SOSP 2001, V-me in [R06]): for each API, the fraction of callers that test its status; callers that do not (e.g. `cfe_sb_task.c` L138 ignoring `CFE_ES_GetAppID`'s status) are ranked as deviant. This proposes templates; it never creates verdicts by itself.

**Bookkeeping.** Every template records its author, authoring time, source hashes and which design cases (C1, C2, C3, C5, R2) the author had inspected for that API. The library is **frozen by hash** before any design-case evaluation. The first evaluation run is **template-only**; any case-specific refinement made afterwards is labelled `oracle refinement` and reported separately with the verdicts it changed ([R06] T2, §5.6 item 5).

### 7.2 RCL: the Readiness Contract Language

**DESIGN.** A small, typed, declarative text language parsed by a hand-written recursive-descent parser into a typed AST (hashed). Grammar sketch (EBNF):

```
file        = "rcl" INT ";" { decl } ;
decl        = namespace | template ;
namespace   = "namespace" QNAME "{" "key" IDENT ":" CTYPE ";" { pred } "}" ;
pred        = "state" IDENT "(" IDENT ")" ":=" fexpr ";" ;
fexpr       = fterm { ("&&" | "||") fterm } ;
fterm       = "!" fterm | "call" "@" CSYMBOL "(" [ args ] ")" [ cmp ] | IDENT cmp | "(" fexpr ")" ;
template    = "template" QNAME "{"
                 "use" "call" "@" CSYMBOL "(" binders ")" [ "->" IDENT ] ";"
                 "requires" conj ";"
                 "allowed" "{" outcome { "," outcome } "}" ";"
                 "class" CLASSID ";"                       (* K-REG, K-ID, K-PHASE, ... *)
                 "provenance" "{" prov { prov } "}"
              "}" ;
conj        = QNAME "." IDENT "(" expr ")" { "&&" QNAME "." IDENT "(" expr ")" } ;
outcome     = "none" | "unspecified" | "documented_drop"
            | "returns" "(" CODE { "|" CODE } ")" [ "tested_by_caller" ] [ "retry_within" bound ] ;
bound       = "ref" "(" CMACRO ")" | INT "from" PATH ":" LINE ;
prov        = ( "api_doc" | "code_admission" | "code_pattern" | "issue" | "researcher" )
              PATH ":" LINE [ "-" LINE ] "sha256" HEX ";" ;
```

`call @SYMBOL(...)` in a namespace predicate names a function of the analysed program (typically a framework validity predicate); the interpreter evaluates it on the model state. Examples (function names checked in the pinned headers: `cfe_es_resource.h` L65, L253, L659; `evs/cfe_evs_utils.h` L72, L164; `cfe_tbl_regrec.h` L196, L222; `cfe_sbr.h` L71, L140):

```
rcl 1;

namespace es.app {
  key a : CFE_ES_AppId_t;
  state LIVE(a) := call @CFE_ES_AppRecordIsMatch(call @CFE_ES_LocateAppRecordByID(a), a);
}
namespace es.self {
  key t : void;                     // the calling task
  state REGISTERED(t) := call @CFE_ES_GetAppRecordByContext() != null;
}
namespace evs.app {
  key a : CFE_ES_AppId_t;
  state RESOLVABLE(a) := call @EVS_GetAppDataByID(a) != null;
  state REGISTERED(a) := call @EVS_AppDataIsMatch(call @EVS_GetAppDataByID(a), a);
}

template evs.with_appid.identity {
  use call @CFE_EVS_SendEventWithAppID(eid, etype, a, spec) -> st;
  requires evs.app.RESOLVABLE(a) && evs.app.REGISTERED(a);
  allowed { none };                 // the header names CFE_ES_WriteToSysLog for unregistered contexts
  class K-ID;
  provenance { api_doc cfe/modules/core_api/fsw/inc/cfe_evs.h:168-172 sha256 <hash>; }
}

template es.task.self_identity {
  use call @CFE_ES_GetAppID(out) -> st;
  requires es.self.REGISTERED(self);
  allowed { returns(CFE_ES_ERR_RESOURCEID_NOT_VALID) tested_by_caller };
  class K-SPAWN;
  provenance { api_doc cfe/modules/core_api/fsw/inc/cfe_es.h:<GetAppID retvals> sha256 <hash>; }
}

template osal.countsem.lookup_by_name {
  use call @OS_CountSemGetIdByName(out, name) -> st;
  requires osal.countsem.LIVE(name);
  allowed { returns(OS_ERR_NAME_NOT_FOUND) tested_by_caller retry_within ref(CF_STARTUP_SEM_MAX_RETRIES) };
  class K-REG;
  provenance { api_doc osal/src/os/inc/osapi-countsem.h:<retvals> sha256 <hash>;
               code_admission apps/cf/fsw/src/cf_cfdp.c:1272-1278 sha256 <hash>; }
}

template sb.route.first_send {
  use call @CFE_SB_TransmitMsg(msg, inc) ;
  requires sb.route.ROUTED(msgid_of(msg));      // CFE_SBR_IsValidRouteId(CFE_SBR_GetRouteId(m)) and an active destination
  allowed { unspecified };                       // no reference defines the no-subscriber rule → LOSS-UNDER-ORDER
  class K-ROUTE;
  provenance { researcher cfe/modules/sb/fsw/src/cfe_sb_priv.c:1083-1086 sha256 <hash>; }
}
```

(The `retry_within` of the countsem template is attached per caller from the caller's own code admission; for callers without one, `allowed` is the documented retval plus `tested_by_caller`. `<…>` placeholders are filled from S1's extraction at authoring time.)

### 7.3 Discovery of candidates nobody registered

**DESIGN.** Discovery is the census itself plus three generic properties; the five cases are only tests that a class must pass.

| Source of discovery | Mechanism | Concrete candidates expected at the pin (all INFERENCE until run) |
|---|---|---|
| Census over all templates | every instance gets a class | SB's identity at all 39 event sites (R2 family, [R2c] §2); TBL's 20 `CFE_EVS_SendEventWithAppID` sites, most passing `TableTaskAppId`, expected PROTECTED because no TBL API call appears in the ES, EVS or SB task files ([R06] §5.5); route readiness of every scheduled MsgId |
| M_impl + deadlock property | explorer deadlock check over analysed OSAL code | **OSAL F1**: a by-name lookup during a creation or deletion window returns `INCORRECT_OBJ_STATE` and leaves the type lock held; the creator then blocks forever ([R01] §1.8); reachable in the C1 derived configuration through CF's lookup loop, which stops on that code ([R01] F1 call sites) |
| M_impl + `osal.create.entry_runs` | explorer | **OSAL F7**: the child's bounded RESERVED wait gives up after 5 attempts and the entry never runs while the creator saw success ([R01] §2.2) |
| K-SPAWN and ES K-REG templates | tier 1 → tier 2 | **ES RESERVED window**: a new main task's `GetAppID` before `cfe_es_apps.c:869`, closed only by a 50 ms sleep; `GetAppIDByName` returning `CFE_SUCCESS` with `0xFFFFFFFF` during the window ([R02] §1.6) |
| K-PHASE template | tier 1 → tier 2 | SBN declares before its real init; CS declares after init failure; HS does not declare after failure but keeps running; CF's retry outlasting the soft barrier ([R03] §2.6, §2.10, §2.14, §2.15) |
| K-ROUTE and timing margins | σ_U vs σ_E | SCH_LAB's schedule reaching CF, SC, CS, SBN by margin only ([R03] §6.3) |
| K-CAP + RC13 | backlog analysis + explorer | SBN's enable-reporting vs TO_LAB's 37 subscribes and `SBNSubPipe` overflow ([R03] §5.2) |
| Atomicity templates (with `In`) | explorer with ground inputs | TBL owner load vs ground load check-then-claim ([R02] §4.4); file TOCTOU in CF and FM ([R03] RC12) |
| Restart scenario family | explorer | TIME synch callback called after unload; stale CF semaphore ID; MD address; SBN symbols ([R02] §5.3; [R03] RC15) |
| Belief deviance | ranking only | callers ignoring `GetAppID`, `CreatePipe`, `Subscribe` statuses |
| Snowboard-style per-app profiling | O1 runs of single apps (config variant with one app in the script) to collect potential publish→use pairs independent of the boot order ([R04] §2.9, V-abs) | cross-check of the static publisher sets |

### 7.4 Configuration variants

**DESIGN.** Each variant is a new B with its own manifest; results are never pooled with the default B, and nothing found only in a variant is a "new bug" of the default configuration.

| Dimension | Variants (each from a reference) |
|---|---|
| Tables | CF `sem_name` non-empty with a role-based provider (C1 derived, labelled derived, [LED] §1b); LC/MD tables enabling watchpoints/dwells; SCH_LAB schedules including the unscheduled MsgIds ([R03] §6.2) |
| Script | permutations of app order; adding or removing apps (each from the bundle's own `targets.cmake` mechanism) |
| Core order | `MISSION_CORE_MODULES` permutations (configuration-change experiments, never schedule evidence) |
| Platform | σ_R1 analysis with the configured priorities (model only, or with root-imposed `SCHED_FIFO` on one CPU as a recorded deviation, [R07] §5.2); `msg_max` > 10 (pending decision D2 in the records) |
| Inputs | ground commands: restart/reload/delete, table load/validate/activate, SB pipe info |
| Initial state | POWERON vs PROCESSOR reset with preserved CDS |
| Revisions | other cFS/cFE revisions for held-out evaluation (§7.5) |

### 7.5 Held-out evaluation protocol

Adopted from [R06] §5.6 with the CAIRN-specific freeze points:

1. **Freeze** the template library, RCL compiler, extractor, closure rules, `cfsk` lowering, tier-1 rules and explorer by hash before any held-out item is opened.
2. **Held-out sources**, counted separately: (a) other cFS revisions named in the ledger (version transfer); (b) the cpu2 target and table variants of this build; (c) issues mined with a pre-registered query (keywords and date cut fixed before the freeze; C1, C2, C3, C5, R2 excluded); (d) seeded ordering mutations derived from real fix history (move a publication after a spawn, drop a wait, empty a retry bound), labelled synthetic; (e) in-code admissions not used during design.
3. **Metrics** per class (§8.5), with denominators.
4. **Template-only vs refined** runs on the five design cases.

---

## 8. Evaluation plan

### 8.1 Research questions

| RQ | Question | Primary evidence |
|---|---|---|
| RQ1 Openness | Across the core and the 15 bundled apps, how many contract instances exist per class (K-REG … K-PERSIST) and per RC kind, and what share does each tier decide? | census table; tier-1 decided share (a headline number, [R06] T4) |
| RQ2 Design cases | Does a **template-only** library classify the five design cases as the code evidence predicts, and what changes when case refinements are added? | R2 FIXED_ORDER certificate and observed 24/24; C1 derived SCHEDULE_DEPENDENT (σ_U) and default INACTIVE; ES RESERVED window (C5-shaped residual at v7.0.1, not C5 itself); C3 σ_R1 vs σ_U on its historical basis (if OPEN-8 allows); C2 only as a role-derived K-PHASE test |
| RQ3 Held-out | On frozen tools and templates, what are triaged precision, confirmed share and UNKNOWN share on held-out sources (§7.5)? | triage records; confirmation records |
| RQ4 Model fidelity | How well does M match the binary? | conformance rate over observed boots; anchor audit coverage; closure resolution statistics (indirect calls resolved per rule, unresolved); havoc counts; certificate-checker pass rate |
| RQ5 Measured phenomena | Does CAIRN classify the order-dependent losses measured in the default configuration (queue-full 4–49 per boot, no-subscriber 15–26) and predict which pipes can overflow? | K-CAP/K-ROUTE verdicts vs SB counters and per-pipe peaks ([BM] §3) |
| RQ6 Cost | What do analysis, exploration, observation, confirmation and contract authoring cost? | per-stage time and memory; per-instance exploration statistics; perturbation (§5.8); person-hours and template counts ([R06] R4) |
| RQ7 Ablations | Which model elements matter? | §8.4 |

### 8.2 Subjects

- **Primary target (open):** the v7.0.1 bundle in the ENV basis, core plus 15 apps; every report triaged.
- **Development set:** the five design cases ([LED]); they are never reported as held-out, and C3 and R2 are not counted as independent ([R06] S4).
- **Held-out:** §7.5 (a)–(e), each counted separately.
- **Configuration variants:** §7.4, each its own basis.

### 8.3 Baselines and comparison points

| Comparator | Status here | How it is used |
|---|---|---|
| Process-level tools (RacePro, SimRacer, SIMEXPLORER, RRF, DESCRY, SysPro) | `NOT_RUN`: not runnable here ([R06] K6) | never reported as failing; their **observation model** is reimplemented (next row) |
| Syscall-level observation model à la RacePro (reimplementation) | runnable: tracepoint syscall traces of the same boots ([R07] §4.7) | shows which instances are visible at the syscall layer (R2: none, [R2g] §5) — labelled "observation-model reimplementation" |
| Memory-level HB detectors: FastTrack-style, WCP, SHB (reimplementations over the same traces' tracked-location and lock events) | runnable | answer a different question (unordered memory conflicts); expected silent on WRONG-WAY and lock-protected instances — a finding about the question, not tool quality ([R05] §6.3 (c)). The user's roadmap excludes thread data-race tools as *baselines* ([R06] S13), so they are presented as reference points unless the PI decides otherwise (OPEN-11) |
| ThreadSanitizer (gcc `-fsanitize=thread`, works here, [R07] §4.8) | requires a variant build (deviation) | reference point for memory-level races, including the expected `SystemState` report |
| DCatch-style framework HB without contracts | runnable as ablation | framework HB rules, unordered pairs only (labelled DCatch-style reimplementation) |
| Stress testing (natural boots under σ_E) | runnable | frequency of observed violations; the uncontrolled baseline |
| PCT on the F1 variant | runnable if OPEN-4 approved | randomized controlled baseline ([R05] §2.1 SCTBench norm) |
| gdb, manual | existing records ([R2g]) | method of the R2 reporter |
| Nidhugg, GenMC, CBMC on the same closed harness | not installed; GenMC needs LLVM 19–22, Nidhugg 8–19 ([R05] §1 F11) | `NOT_RUN` unless installed by decision |

### 8.4 Ablations (same engine, one element removed at a time; [R06] S11)

1. No configuration closure (pointer analysis only): app entry code unreachable, indirect calls unresolved.
2. No cFS HB rules (generic pthread semantics only: no observation-conditioned edges, no panic rule).
3. Collapsed identities (one identity per event instead of four).
4. Atomic APIs (M_api everywhere; no RESERVED windows, no spawn-before-return).
5. No contracts (memory conflicts on tracked locations only).
6. No tier 1 (explore every instance).
7. No DPOR (naive enumeration, on slices small enough to finish).
8. Lemma L1 off (unroll polls to reference bounds) and Lemma L2 off.
9. M_api vs M_impl.
10. σ_U vs σ_R1.
11. No conformance feedback (verdicts without the model-fault downgrade).

Each ablation reports what moved between classes, what became UNKNOWN, and the cost change.

### 8.5 Metrics and statistics

- Per class and per RC kind: instances; INACTIVE / NO_PROVIDER / PROTECTED / FIXED_ORDER / SCHEDULE_DEPENDENT / UNKNOWN (with reason); HANDLED and LOSS-UNDER-ORDER sub-labels; evidence levels. `NOT_RUN`, `NOT_MEASURED`, `NOT_FROZEN` are never zero ([R06] R2).
- Precision: two independent raters triage reports against the contract and code; disagreements logged; agreement statistic reported.
- Confirmation: share of SCHEDULE_DEPENDENT verdicts confirmed in each direction; time and runs to confirmation; gate-unreached share.
- Proportions with binomial confidence intervals; between-group comparisons as in §5.8; PCT runs with measured n, k and the resulting bound.
- Denominators everywhere; independent cases kept separate from repeated runs ([R06] R3).

### 8.6 Threats to validity

- **Construct.** Contract authoring bias (mitigated by header-sourced templates, framework predicates, freeze and template-only runs); route and capacity losses are not violations without a must-deliver reference (LOSS-UNDER-ORDER keeps them separate); the definition of "ready" via framework predicates inherits their semantics.
- **Internal.** Model fidelity (conformance checking, anchor audit, closure boot); Clang vs GCC (A2); probe perturbation (§5.8); kernel drift and a shared host ([R07] §1; [BM] §1); persisted state across runs ([BM] §6) recorded per run.
- **External.** One OSAL backend (POSIX), Linux `SCHED_OTHER` in permissive mode, one bundle revision, derived cases for C1/C2/C3/C5; σ_R1 is a model of RT targets, not a measurement on one.
- **Conclusion.** Few design cases with dependencies between them; held-out sources may be small; statistics are reported with their N.

---

## 9. Implementation roadmap

### 9.1 Directory layout (to be created later; nothing created now)

```
cairn/
  CMakeLists.txt                  # out-of-tree against LLVM build 1053047a (find_package(LLVM/Clang/MLIR))
  include/cairn/abi/              # C99 headers: ctc container, trace_abi.h, facts/anchors/verdict records
  include/cairn/                  # C++ interfaces of every stage (pure functions over typed artifacts)
  dialect/cfsk/                   # ODS (.td), ops, verifiers, textual syntax
  lib/basis/                      # S0
  lib/extract/                    # S1 (LibTooling, bitcode, doc-comment extraction)
  lib/anchor/                     # S2 (DWARF, MC disassembly, audit)
  lib/close/                      # S3 (task set, loader model, indirect calls, tables)
  lib/skeleton/                   # S4 (import, slicing, loop classification, lowering)
  lib/semantics/                  # L0 / L-1 rule implementations with citation tables
  lib/rcl/                        # C0 parser, typed AST, freeze records
  lib/census/                     # S5
  lib/tier1/                      # must-HB fixpoint + independent certificate checker
  lib/explore/                    # interpreter, optimal DPOR, monitor, deadlock, K-CAP backlog
  lib/smt/                        # optional: cfsk → smt dialect → SMT-LIB
  lib/observe/                    # planner, raw perf/bpf loader, trace writer
  bpf/                            # BPF programs (restricted C; system clang-18 -target bpf)
  lib/confirm/                    # ptrace controller, PCT controller
  wrap-rt/                        # gcc-built C99 gate runtime (F1 variant only)
  lib/conform/                    # D1, D2
  lib/report/                     # R0
  tools/cairn/                    # single driver binary
  contracts/                      # *.rcl with provenance; freeze manifest
  test/litmus/                    # small gcc-built C99 programs on real OSAL with expected classes per σ, M
  test/golden/                    # textual cfsk and verdict golden files
  test/abi/                       # container round-trip tests
```

### 9.2 Milestones and exit criteria

| M | Deliverable | Exit criterion |
|---|---|---|
| **M0** | Interfaces: ABI headers, `cfsk` op vocabulary, RCL grammar, verdict record, certificate format; litmus suite skeleton with expected answers; basis assembler | interfaces reviewed; the ENV basis is assembled twice with identical hashes; every basis component has a status |
| **M1** | Fact extractor and anchor mapper | 448/448 FSW TUs extracted; anchor audit lists every public-API call site with a GCC address or an exception; table values equal the built `.tbl` payloads |
| **M2** | Closure, with a minimal observer (task creation, thread start, module/symbol probes) | the 5 core and 15 app entries, children and timer callbacks resolved; closure-boot cross-check passes on one F0 boot; resolution statistics for the 110 indirect sites |
| **M3** | Skeleton builder and tier 1; gate G-MLIR | R2's FIXED_ORDER certificate passes the independent checker and matches the hand-derived chain of [R06] T5; litmus tier-1 answers correct; G-MLIR decision recorded with its measurements |
| **M4** | Template library v1 and census | library frozen by hash with provenance and authoring records; census of core + 15 apps with classes; template-only run on the design cases recorded **before** any refinement |
| **M5** | Full observer and conformance | R2's 24 violating uses per boot re-established on the unmodified binary without a debugger by the new recorder; perturbation groups A–C measured; every observed boot conforms, or model faults are filed and fixed |
| **M6** | Tier-2 explorer (+ optional SMT) | litmus tier-2 answers correct; C1 derived configuration classified with witnesses; ES RESERVED-window instances classified; OSAL F1 found or refuted under M_impl; per-instance cost reported |
| **M7** | Confirmation | every SCHEDULE_DEPENDENT instance with a lock-free gate site attempted under F0, with the outcome recorded; F1 variant and PCT calibration (n, k) if OPEN-4 is approved |
| **M8** | Freeze and evaluation | all hashes frozen; held-out runs; ablations; report with claims table including the claims not made |

**Order of construction.** Interfaces and the litmus suite first (M0); then the static chain (M1–M3) with only the minimal observer needed for the closure cross-check; the full recorder after the model exists (M5), as [v0.1] §15 already demands ("recorder를 먼저 크게 만들고 나중에 의미를 추정하는 개발 순서로 돌아가지 않는다").

### 9.3 Rules that keep the code clean

1. **Interfaces before implementations.** Every stage is a function from typed, hashed inputs to typed, hashed outputs; no stage reads another's internals.
2. **No checked-in generated code.** The only generation is MLIR ODS at build time, and only if G-MLIR keeps MLIR. No harness C code is generated: the explorer interprets the skeleton ([R05] §4 "no messy code generation" alternative (b)).
3. **No case-specific code.** A lint fails the build if case identifiers or case-specific symbols (e.g. `CFE_SB_Global`, `sem_name`) appear in `lib/` outside `contracts/` and `test/`.
4. **Every constant carries its reference.** Constants in the tool are written as `CAIRN_REF(value, "path:line")`; a lint rejects bare numeric constants in semantic code ([POL] L22).
5. **Tests at three levels.** Litmus programs (gcc-built, real OSAL) with expected classes per σ and M; golden textual models and verdicts; round-trip and mutation tests for containers and certificates (a corrupted certificate edge must be rejected).
6. **One driver binary**, library modules with unit tests, reproducible builds pinned to LLVM `1053047a`.

---

## 10. Risks and open decisions for the PI

### 10.1 Risks

| Risk | Likelihood / impact (INFERENCE) | Mitigation | Retire by |
|---|---|---|---|
| Skeleton infidelity (slicing drops something that matters; havoc too coarse) | medium / high | conformance checking on every observed boot; havoc counts per instance; UNKNOWN on doubt | M5 conformance results |
| State explosion in tier 2 (ES lock and SB lock touched by almost every task) | medium / medium | tier 1 first; cones; Lemma L2; await summarization; BPOR if a bound is needed; UNKNOWN(budget) | M6 per-instance statistics |
| Lemmas L1/L2 turn out false in some pattern | low / high | switchable per instance; ablation 8 measures their effect | proofs written before M8 |
| Clang vs GCC divergence on a contract-relevant site | low / medium | anchor audit at M1; GCC `-fcallgraph-info` cross-check | M1 audit |
| LLVM trunk dependency (`1053047a`, headers from the source tree) | medium / medium | pin and record; single driver; G-MLIR fallback to plain LLVM IR | M3 gate |
| eBPF tooling without libbpf, bpftool or bpftrace | medium / medium | own minimal loader over raw syscalls; BPF C with hand-declared helpers; UAPI headers from the distribution's kernel headers (presence to be checked) | M2 minimal observer |
| Return-probe cost and placement unmeasured | medium / low | measure in M4/M5; caller-side probes as fallback | M5 |
| Uprobe pre-registration on not-yet-loaded app `.so` untested ([R07] §9 item 6) | medium / medium | register after `dlopen` (observed by a module-load probe) if pre-registration fails | M2 |
| Ring-buffer losses at startup burst | low / medium | calibration; per-thread `tseq` gaps; verdict downgrade | M5 |
| ptrace holds perturb other threads (e.g. binsem internal 2 s timed lock, [R01] §6.3) | low / medium | gate placement rule (lockset = ∅ under M_impl) | M7 |
| Contract authoring effort larger than expected | medium / medium | per-API templates, header extraction, framework predicates; effort measured | M4 |
| Route/capacity verdicts read as "bugs" | medium / reputational | LOSS-UNDER-ORDER label; no violation claim without a must-deliver reference | report review |
| Historical bases (6.4.1 for C3/C5) lack a compile database | high / medium (RQ2 scope) | reconstruct compile commands from the 6.4.x make system, or restrict C3/C5 to code-reading evidence | OPEN-8 |

### 10.2 Decisions requested from the PI

| Id | Decision | Options | This proposal's recommendation |
|---|---|---|---|
| OPEN-1 | Semantics set for the paper | σ_U + σ_E only; or also σ_R1 (model only, or with a root-imposed `SCHED_FIFO` single-CPU arm as a recorded deviation) | σ_U + σ_E required; σ_R1 as model-only for C3-class verdicts |
| OPEN-2 | MLIR | keep (after G-MLIR) with build-time ODS generation; or plain LLVM-IR skeleton | decide by G-MLIR at M3 |
| OPEN-3 | Solver install (Z3 4.8.12 candidate is old, [R07] §2.4) | none (DPOR only); install for the optional SMT backend | DPOR only until a slice needs symbolic table inputs |
| OPEN-4 | F1 `--wrap` gate variant as a separate `MISSIONCONFIG` copy | approve as recorded deviation; or ptrace-only (F0) | approve for PCT only; observation stays F0 |
| OPEN-5 | Root-side observer attached to the README-mode process; new `run_cfs.sh` hook | approve; or not | approve (highest fidelity available, [R07] §8 item 2) |
| OPEN-6 | Scope: route and capacity classes in the first paper | include (the measured phenomena); or exclude | include, labelled LOSS-UNDER-ORDER |
| OPEN-7 | Measurements before M5: return-probe cost, BPF body cost, `__thread` under `-pedantic` | — | run as M2/M4 probes |
| OPEN-8 | Historical bases for C3/C5 | reconstruct a compile DB for 6.4.1/6.4.2; or code-reading only | attempt reconstruction; otherwise report as code-reading |
| OPEN-9 | Number of runs per perturbation group and for σ_E frequencies | literature norm (~30, P) or other | decide with the statistics plan |
| OPEN-10 | Timed model semantics | out of scope; or empirical-timed extension | out of scope |
| OPEN-11 | Memory-level detectors (TSan, FastTrack/WCP/SHB reimplementations) | reference points only; or baselines | reference points (roadmap's exclusion of thread data-race tools as baselines) |
| OPEN-12 | Citing CORD observations | as prior observations with the retired tool; or redo everything | redo with the new recorder (M5) and cite CORD records as prior evidence only |

---

## Appendix A. Worked examples (INFERENCE: designed outputs, not results)

### A.1 R2 (cFE #2663): FIXED_ORDER with a certificate, plus a chained route loss

1. **Closure.** The core order ES, EVS, FS, SB, TBL, TIME comes from the generated `CFE_CORE_MODULE_OBJTAB` (`cfe_static_module_list.c` L36-47). The `CFE_Config_IterateAll` callback count comes from the generated version table (13 "Version Info: Module …" events in the ENV basis, [R2c] §4.2).
2. **Census.** Template `evs.with_appid.identity` instantiates at the 39 SB sites that pass `CFE_SB_Global.AppId` ([R2c] §2). Value analysis binds `a` to a load of `CFE_SB_Global.AppId`. Conjunct `RESOLVABLE(a)` has writer set W = {the out-pointer store inside `CFE_ES_GetAppID` called at `cfe_sb_task.c` L138}; the EarlyInit zeroing (`sb/cfe_sb_init.c:50`) does not satisfy it. D = ∅.
3. **Tier 1, uses in the ES and EVS tasks.** For a use `u` in `CFE_ES_TaskInit` (use sites `cfe_es_task.c` L367-387, L466, [R06] T5):
   - T-PO: `u` → ES's `CFE_ES_WaitForSystemState(CORE_READY, …)` at `cfe_es_task.c` L134, whose entry stores `AppState(ES) := RUNNING` under the ES lock (`cfe_es_api.c` L528, L547-549, L571-576);
   - T-OBS: the startup thread's `MainTaskSyncDelay` success exit (`cfe_es_start.c` L878-888) observes that store; its writer set (the stores in `WaitForSystemState` and `RunLoop` of the ES task) all follow `u` in ES's program order, so `u` is in the meet;
   - T-TERM: the timeout branch panics (`cfe_es_start.c` L838-846) and is excluded;
   - T-PO through the unrolled table loop to the EVS and then SB entries; T-SPAWN at `cfe_es_start.c` L810 for SB;
   - T-PO in the SB task to `cfe_sb_task.c` L138.
   Hence `u ∈ MustBefore(x)` for the only writer: **FIXED_ORDER(B, σ_U, M_api and M_impl, b = ∞), shape WRONG-WAY**. The not-ready outcome is `ILLEGAL_APP_ID`, which every SB caller discards: consequence "status ignored, effect lost silently". EVS uses (`cfe_evs_task.c` L213 before L229) get the same certificate.
4. **Predicted count.** The certificate covers every occurrence on the ES and EVS init paths: in ES, 17 `SEND_NO_SUBS` uses (4 for ES's own init events, 13 for the version events) plus 3 pipe and subscription uses; in EVS, 4 uses → 24 per boot in the ENV basis, and 11 (ES 7, EVS 4) in a basis whose version table is NULL. Both numbers equal the observations ([R2g] §2; [R2c] §4.2, §7), which is a check of the basis sensitivity, not new evidence of the violation.
5. **Chain.** The 17 `SEND_NO_SUBS` uses come from ES transmitting its event packets (MsgId 0x808) before any subscription: template `sb.route.first_send` gives a FIXED_ORDER LOSS-UNDER-ORDER for those transmissions (subscribers are script apps, created after CORE_READY), chained to the identity violations.
6. **Other uses** (TBL, TIME, SB itself, all 15 apps): PROTECTED certificates through SB's `WaitForSystemState` RUNNING store → `MainTaskSyncDelay` → later spawns → use, the chain CORD's analyzer rebuilt ([R2c] §4.3). Unlike CORD's model, CAIRN models the system-state wait (T-OBS on `SystemState` with the startup thread's stores as writers), so ES or EVS uses *after* the publication, ordered only through that wait, are not false positives — the gap [R2c] §4.3 names.
7. **Confirmation.** Passive observation only (FIXED_ORDER): the new recorder must re-establish 24/24 on the unmodified binary (M5 exit). A "conforming" execution needs a changed core order, a configuration-change experiment.

### A.2 C1 derived configuration, and the OSAL lock-leak path under M_impl

1. **Default B.** CF's lookup sits behind `if (… sem_name[0])` (`cf_cfdp.c` L1270); both default table entries are empty (`cf_def_config.c` L60, L78) → **INACTIVE(B)**.
2. **Derived B′** (labelled derived, [LED] §1b): a non-empty `sem_name` and a role-based provider app that creates the counting semaphore in its init; neither app waits on a phase.
3. **M_api, σ_U.** Writers of `osal.countsem.LIVE(name)` = the provider's create; no must-order between it and CF's lookup loop → tier 2. The explorer finds (a) provider created before some retry → FOUND (HANDLED via `retry_within 25`), and (b) provider delayed past the `await` timeout → `CF_INIT_SEM_ERR_EID`, engine disabled, CF exits → violation (app error exit) → **SCHEDULE_DEPENDENT(B′, σ_U, M_api)**.
4. **M_impl, σ_U.** The lookup loop is unrolled to 25 (not summarized, §4.2). The explorer also reaches the lookup while the provider's record is RESERVED with `name_entry` set: outcome `OS_ERR_INCORRECT_OBJ_STATE`, which ends CF's loop (`cf_cfdp.c` L1285-1288), and the countsem table lock stays held; the provider then blocks forever in `FinalizeNew` → **deadlock** report, an implementation defect candidate of OSAL POSIX at the pin ([R01] F1).
5. **Confirmation plan** (P0 output): to confirm (3b), hold the provider at its init before the create call (lockset empty) until CF's lookup-loop exit is observed, watchdog T_ref = 25 × 100 ms (`cf_internal_cfg.h` L172-185). To confirm (4), hold the provider at `OS_CountSemCreate_Impl` entry — inside the RESERVED window, where the type lock is **not** held ([R01] §1.5 step f) — until CF's lookup returns. This is exactly the confirmation experiment [R01] §7 item 1 proposes, here derived by the planner from the model.

### A.3 The ES App-record window at v7.0.1 (C5-shaped residual, not C5 itself)

1. **Instance.** `es.task.self_identity` at the first `CFE_ES_GetAppID` reached by each script app (for SAMPLE_APP through `CFE_EVS_Register` → `EVS_GetCurrentContext`, [R02] §3.1). Readiness = `CFE_ES_GetAppRecordByContext() != null`, i.e. task record **and** App record LIVE.
2. **Tier 1.** The trampoline's `await` observes only the task record (published under the ES lock at `cfe_es_apps.c` L703, released L722); the App record is published at L861-869; nothing orders L869 before the child's use → tier 2.
3. **Tier 2, σ_U.** The 50 ms delay is a no-op; the explorer runs the child past its successful poll while the startup thread sits between L722 and L861 → `GetAppID` fails → `CFE_EVS_Register` fails → SAMPLE_APP init error, `APP_ERROR`, exit (violation, app error exit); the conforming execution exists too → **SCHEDULE_DEPENDENT(B, σ_U)**. Under σ_E the delay makes it rare; frequency is empirical.
4. **Companion instance.** A peer's `CFE_ES_GetAppIDByName` during the same window returns `CFE_SUCCESS` with `0xFFFFFFFF` ([R02] §1.6): K-REG with the `IsMatch` predicate classifies it as not-ready. In the default configuration the peers that look up apps by name are HS AppMon (all `NOACT`) and CS (disabled entries) → INACTIVE(B); a table variant activates them.
5. **Confirmation plan.** Hold the startup thread at a lock-free site after L722 and before the lock at L861 until the child's `GetAppID` return is observed; watchdog T_ref = `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` (1000 ms), after which the child's poll would itself time out.

### A.4 C3 (cFE #73) on its historical basis: verdicts depend on σ

On cFE 6.4.1 there is no per-core-task serialization; the configured priorities are TIME 60, EVS 61, SB 64 (lower number = higher priority), and TIME's init calls `CFE_SB_CreatePipe`, whose event uses SB's unset AppId ([LED] §1a C3). Expected verdicts ([R06] T5, INFERENCE): **FIXED_ORDER under σ_R1** (single CPU, TIME's higher priority runs its init first once all three are ready) and **SCHEDULE_DEPENDENT under σ_U**; under σ_E (permissive Linux) only frequencies. This requires a 6.4.1 basis (OPEN-8). The 6.4.2 fix F3 (per-core-task barrier, the ancestor of S1) turns the σ_U verdict into the FIXED_ORDER-for-early-tasks shape of A.1 ([LED] §3.1).

### A.5 Startup-burst capacity and the SCH_LAB margin

1. **`SBNSubPipe`.** SBN enables SB subscription reporting after OPERATIONAL; TO_LAB, released by the same `SystemState` transition, performs 37 global subscriptions from its table ([R03] §2.3, §2.15). Each subscription made after the enable transmits a `ONESUB` packet into SBN's subscription pipe (effective depth 10 in the ENV Plat). K-CAP tier 1 bounds the backlog by up to 37 > 10 → tier 2 finds an overflow witness and a non-overflow witness (enable after the burst, recovered by `SEND_PREV_SUBS`) → **SCHEDULE_DEPENDENT loss (σ_U)**; σ_E evidence: overflow in 5 of 6 M1 runs, and the run without it showed a different interleaving ([R03] §5.2; [BM] §3.3).
2. **SCH_LAB wakeups.** SCH_LAB's first send needs one TIME 1 Hz packet and a tick of its timer (first expiry 1 s after its init); CF, SC, CS and SBN subscribe in their own inits with no order to SCH_LAB ([R03] §6.3). Under σ_U ticks are untimed → **SCHEDULE_DEPENDENT loss (σ_U)**; under σ_E the margin measured from logs is 0.12–0.9 s in 13 boots, so the loss has not been observed. The report states both, and the margin is σ_E evidence, not protection.

---

## Appendix B. From v0.1 to CAIRN

| v0.1 element / [R06] finding | Disposition in CAIRN |
|---|---|
| §1 pipeline "facts → MLIR model → classification → exploration → execution" | **kept as a spine**, made a loop (closure boot, conformance, evidence feedback; §2.5) |
| §1 first scope from the five cases ([R06] finding 1) | scope = the 15 contract kinds of [R03] §4 via nine generic classes (§3.17); the cases are tests in RQ2 |
| §2.3 `Exec(B)` undefined ([R06] finding 2, T5) | B made explicit (§1.4); verdict = (class, σ, M, b, B) with σ_U / σ_R1 / σ_E (§1.5-1.6) |
| §2.2 three-part target (requirement, actual order, allowed outcome) | **kept and formalized** as typestate-style contracts with `Allowed` (§1.3) |
| HB-ordered wrong-way class missing ([R06] finding 3) | first-class shape WRONG-WAY; tier-1 FIXED_ORDER certificates (§1.7, §4.4) |
| §5 MLIR as a fact container ([R06] finding 4, T1) | MLIR carries the *abstracted program* lowered from imported LLVM IR; gate G-MLIR with stated criteria and a plain-IR fallback (§2.4) |
| Static precision is a configuration problem ([R06] finding 5, T3) | configuration closure first, registration-site and type-layer resolution next, unresolved → UNKNOWN (§4.1) |
| Compiler mismatch ([R06] finding 6, T7) | Clang for analysis only, with two documented diagnostic adaptations; every fact anchored to the GCC binary's DWARF; observation on the unmodified GCC binary (§2.4, §5.1) |
| Contract circularity ([R06] finding 7, T2) | per-API templates from headers; framework predicates define "ready"; freeze by hash; template-only first; refinements labelled (§7.1) |
| No held-out protocol ([R06] finding 8) | §7.5 |
| v0.1 stale on R2 ([R06] finding 9) | R2's existing non-debugger observation is the starting fact; M5 must re-establish it with the new recorder |
| [R6] RacePro title ([R06] finding 10) | corrected in citations (Appendix C) |
| §6.1 resource key with generation | **kept**; namespaces and generations per §1.2 |
| §7.1 required order never as HB | **kept**; contracts are monitors, never edges |
| §7.2 first classification | tier 1 with certificates (§4.4) |
| §7.3 slicing caveat (third-party sync) | cone of influence over sync objects; Lemma L2 (§4.5) |
| §7.4 operation boundaries | M_impl splits OSAL create/delete; ES reserve/publish sites are separate transitions (§3.2-3.3, §3.8) |
| §7.5 bounded model checking / SMT | optimal DPOR by default, BPOR if bounded, SMT optional (§4.5) |
| §9 selective instrumentation | principles kept; mechanism changed to F0 uprobes/eBPF on the unmodified binary; model-derived probe plan (§5) |
| §10 counterexample verification | kept; gate-placement rule and reference-bounded watchdog added (§4.9) |
| §11 overhead groups | kept and extended with trap-only and variant-layout groups and order-sensitive outcome distributions (§5.8) |
| §12 ENV controls | kept; CPU-0 pinning, kernel drift, effective capacities in Plat (§1.4) |
| §13 reporting | kept; σ, M, b, evidence level, shape, chain, provenance added (§1.6, §2.3) |
| §14 evaluation | modified: open census, held-out protocol, observation-model reimplementations, ablations (§8) |
| §15 implementation order | class-first, interfaces first, MLIR after its gate (§9.2) |

---

## Appendix C. Literature used, with verification status as recorded in AR1

| Key | Reference | Status (source) |
|---|---|---|
| Lamport78 | Time, Clocks, and the Ordering of Events in a Distributed System. CACM 21(7), 1978 | V-abs ([R04]) |
| Fidge88, Mattern89 | vector clocks | V-abs ([R04]) |
| NetzerMiller92 | What Are Race Conditions? LOPLAS 1(1), 1992 | V-abs ([R04]) |
| Typestate86 | Strom & Yemini, IEEE TSE 12(1), 1986 | V-me ([R06]) |
| FastTrack09 | Flanagan & Freund, PLDI 2009 | V-abs ([R04]) |
| PoznianskySchuster03 (Djit+) | PPoPP 2003 | P ([R04]) |
| Eraser97 | TOCS 15(4), 1997 | V-abs ([R04]) |
| OCallahanChoi03 | Hybrid Dynamic Data Race Detection, PPoPP 2003 | V-abs ([R04]) |
| TSan09, TSan11 | WBIA 2009; RV 2011 | V-abs ([R04]) |
| CP12, WCP17, SHB18, M2_20, SyncP21 | predictive race detection | V-abs ([R04]) |
| MCM12, RVPredict14, GPredict15 | maximal causal models; RVPredict; GPredict | V-abs ([R04]) |
| ExceptioNULL12 | FSE 2012 | V-abs ([R04]) |
| Lu08 | Learning from Mistakes, ASPLOS 2008 | V-abs ([R04]) |
| DefUse10 | OOPSLA 2010 | V-abs ([R04]) |
| ConMem10, CTrigger09 | ASPLOS 2010; ASPLOS 2009 | V-abs ([R04]) |
| Tian08, AdHoc10 | ISSTA 2008; OSDI 2010 | V-abs ([R04]) |
| EventRacer13, DroidRacer14, CAFA14 | OOPSLA 2013; PLDI 2014; PLDI 2014 | V-abs ([R04]) |
| InitRacer17 | OOPSLA 2017 | V-abs ([R04]) |
| DCatch17 | ASPLOS 2017 | P ([R04]) |
| CloudRaid18, Snowboard21 | ESEC/FSE 2018; SOSP 2021 | V-abs ([R04]) |
| DataCollider10, KCSANdoc | OSDI 2010; Linux kcsan.rst | V-abs; V-doc ([R04]) |
| RaceFuzzer08, Portend12, Narayanasamy07 | PLDI 2008; ASPLOS 2012; PLDI 2007 | V-abs ([R04]) |
| RacePro11 | Laadan et al., *Pervasive Detection of Process Races in Deployed Systems*, SOSP 2011 | V-abs ([R04], [R05]) |
| CHESS08, PCT10 | OSDI 2008; ASPLOS 2010 | V-abs (PCT mechanism: P) ([R04], [R05]) |
| ICB07, FairSMC08, DelayBound11, SCTBench14 | context bounding; fair SMC; delay bounding; SCTBench | V-abs ([R05]) |
| DPOR05, ODPOR14, BPOR13, MCR15 | DPOR; optimal DPOR; bounded POR; MCR | V-abs ([R05]; DPOR guarantee wording UNVERIFIED) |
| Nidhugg15, GenMC19/21 | SMC for C | V-abs; README V-doc ([R05]) |
| Alglave13, CBMC04, ESBMC11, LazyCSeq14, QadeerRehof05 | BMC and SMT encodings | V-abs ([R05]) |
| SIERRA18, UBITect20, Goblint16, Whoop15, RELAY07, Locksmith06 | static analyses | V-abs ([R05]) |
| SVF16, FSAM16, MLTA19 | pointer analysis | V-abs ([R05]) |
| MLIR21, MLIR-DF, MLIR-SMT, ClangIR | MLIR paper; dataflow framework; SMT dialect; ClangIR | V-abs; V-doc ([R05]) |
| Schwarz11, Tulsyan20, AstreeA15 | priority-aware and flag-aware static analysis; AstréeA | V-abs ([R05]) |
| SDV06, HolzmannJoshi04, Havelund01 | environment models; harness around unmodified code; SPIN on DS1 | V-abs ([R05]), V-me ([R06]) |
| Soundiness15 | In Defense of Soundiness, CACM 58(2), 2015 | V-abs ([R05]) |
| Engler01 | Bugs as Deviant Behavior, SOSP 2001 | V-me ([R06]) |
| ArcuriBriand11, Klees18 | statistics for randomized algorithms; evaluating fuzz testing | V-abs (Klees trial count: P) ([R05]) |

Standard algorithmic facts used without a verified citation in AR1: maximum-weight closure via minimum cut (§4.6).


# 06 — Critique of the user's design proposal v0.1 for a new cFS process-level race detector

- Date: 2026-10-12. Investigation only. Nothing was built, run, installed, modified, deleted or committed. No scratch experiment was run; the task did not ask for one.
- Subject: `39d4fa4a-cFS_Process_Level_Race__________20261008.md` ("cFS Process-Level Race 신규 탐지기 설계", v0.1, written 2026-10-08). Below it is called **v0.1**, and its sections are cited as `v0.1 §n`.
- Pinned code: `/home/user/work/procrace/cfs_ref/cFS` = nasa/cFS `088b2fa8` (v7.0.1), cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`, CF `15a871e6`. I checked these with `git rev-parse`, and the top-level `git status --porcelain` printed nothing. Code paths below are relative to this root. Generated build files are cited under `build-native_std/`.

## 0. Verdict on one page

**Overall.** v0.1 gets the epistemics of the problem right. It keeps three things apart: the order that is required, the order that is actually enforced, and the order that was observed. It defines verdict classes that do not merge "no alarm" into "safe". It keeps cost reporting honest, and it guards against over-claiming from the five cases. As a tool architecture it is **under-specified in exactly the places that decide whether the tool is open and academically defensible**. My recommendation is **keep the frame, modify the engine, demote MLIR**. The top findings follow.

1. **The five cases drive the design (risk of a tailored tool).** v0.1 §1 sets the first scope from "what the five cases have in common", and v0.1 §8 organises the design requirements case by case. A survey of all 15 bundled apps found 15 kinds of readiness contract, and the five cases cover 3 of them (`03_cfs_app_patterns.md` §4–§5). The only order-dependent losses actually measured in the default configuration are SB subscription and pipe-capacity losses: 4–49 queue-full drops per boot (`env/BASELINE_MEASURE.md` §3.3) and `NoSubscribersCounter` 15–26 (§3.5). Neither is in v0.1's first scope.
2. **"Fixed-order" versus "schedule-dependent" is not yet well defined.** Both are defined relative to `Exec(B)` (v0.1 §2.3). The proposal never says which scheduler and which timing semantics generate `Exec(B)`. cFS startup is built from **timed polling loops with soft timeouts**:
   - `CFE_ES_MainTaskSyncDelay`, `cfe/modules/es/fsw/src/cfe_es_start.c:860-918`;
   - the 1000 ms soft limit, `cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h:866-879`;
   - the OSAL bounded wait, `osal/src/os/shared/src/osapi-idmap.c:475-480`;
   - CF's 25 × 100 ms retry, `apps/cf/fsw/inc/cf_internal_cfg.h:172-185`.

   Under untimed semantics nearly every soft timeout becomes reachable. Under timed semantics the verdict depends on platform constants and execution-time bounds. The same instance can also change class with the platform: C3 is priority-ordered on an RT single core but not under `SCHED_OTHER` (ledger §1a; `env/BASELINE_MEASURE.md` §4). **Verdicts must carry the semantics they hold under.** §4 T5 gives a concrete definition.
3. **The proposal does not yet say that HB-ordered violations ("ordered the wrong way") are the central class.** R2 is a real, observed violation in 3/3 boots under gdb and 3/3 without a debugger: 24 uses per boot (`results/R2_gdb_v701.md` §2; `results/R2_cord_v701.md` §7). Every one of those uses is happens-before-ordered before the publication by S1. A data-race or unordered-pair detector therefore reports **nothing** for R2. Contract checking over ordered pairs has to be a first-class mechanism, not a side branch.
4. **MLIR has no demonstrated benefit and a real cost.** v0.1 §5.2 itself says MLIR adds no detection power. The user's own earlier note asks for a comparison against a plain graph (`fc87c4b6…` §9.4). MLIR is **not installed**. Ubuntu `mlir-18-tools` 18.1.3 is available but not installed, so using it needs an install decision. Whether the SMT dialect that v0.1 [R9] cites exists in MLIR 18 is **UNVERIFIED**. There is no C→MLIR frontend in the analysis path: v0.1 §5.1 extracts facts, and the facts are then *re-encoded*. MLIR would hold a fact graph, not code. Recommendation: make the semantic model a schema that does not depend on its serialization. Keep MLIR as an optional back end, gated by a measured comparison.
5. **Static call-graph and resource resolution are mostly a configuration problem, not an alias-analysis problem.**
   - App entry points are strings in the generated startup script, resolved by `dlsym` (`build-native_std/exe/cpu1/cf/cfe_es_startup.scr:3-17`; `cfe/modules/es/fsw/src/cfe_es_apps.c:499,522`). They are called through a function pointer that is stored in a shared table (`cfe_es_apps.c:581,639`).
   - The core task order is a CMake-generated array (`build-native_std/native/default_cpu1/cpu1/cfe_static_module_list.c:36-48`).
   - Semaphore names and MsgIds come from tables (`apps/cf/fsw/src/cf_cfdp.c:1270-1283`).
   - 13 of R2's 24 uses go through a callback (`cfe/modules/config/fsw/src/cfe_config_get.c:184` → `cfe/modules/es/fsw/src/cfe_es_task.c:235-243`). How many there are depends on build-generated data (`R2_cord_v701.md` §4.2, §7).

   LLVM's alias analysis (v0.1 §5.1, [R8]) is not the binding constraint. A **loader and configuration model**, together with API mod/ref summaries, is.
6. **The compiler mismatch is real but manageable.** The reference binary is built by GCC 13.3 in a `debug` build with no `-O`. The flags include `-std=c99 -pedantic -Wall -Werror` and GCC-only warning options (`build-native_std/native/default_cpu1/compile_commands.json`, 1417 entries, all `/usr/bin/cc` → `x86_64-linux-gnu-gcc-13`). Clang/LLVM 18.1.3 is installed. Fact extraction with Clang is acceptable if equivalence is checked. **Instrumentation must stay GCC-built**, or a Clang-built variant must be recorded as a `deviation` (policy L14-21).
7. **Contract authoring is circular unless controlled.** v0.1 §6.3's example contract `sb_event_identity_ready` is an R2-specific restatement of a known answer. It should instead be one instance of a **generic, documentation-sourced template**. Example: the `CFE_EVS_SendEventWithAppID` precondition "The Application ID must correspond to a registered application or core service" (`cfe/modules/core_api/fsw/inc/cfe_evs.h:169-172`). That template finds R2 without any SB-specific knowledge (§5.5).
8. **The evaluation is not yet "held-out".** v0.1 §14.2 correctly says the five cases are design cases. It gives no protocol for obtaining cases the tool has not seen, and the external baselines cannot be run here (`baselines/availability.md`, "Status at a glance": 8 tools, 0 runnable). A pre-registered held-out protocol is needed, plus internal ablations that stand in for the baselines' *observation models* (§5.6).
9. **v0.1 is stale on R2.** It plans "비디버거 선택 계측으로 원래 읽은 값과 반환 경로를 확인" (v0.1 §8.5 step 5) as future work. That observation already exists: 24/24 uses, ES 20 and EVS 4, without a debugger, runs r2_c4–r2_c6 (`results/R2_cord_v701.md` §7). It was made with the now-retired CORD recorder. The new tool must re-establish it with its own recorder, but the design should start from the fact.
10. **Citation errors.** v0.1 [R6] gives RacePro the wrong title. The SOSP 2011 paper is *Pervasive Detection of Process Races in Deployed Systems* (roadmap §13.2 [P1]; also corrected in `04_lit_dynamic.md` §0.4). v0.1 [E1] (`…5개사례_근거정리_20261008(1).md`) is not among the files available to me, so claims that rest only on [E1] could not be checked.

---

## 1. Inputs, labels and limits

### 1.1 Inputs read (sha256 prefix, mtime UTC)

| File | sha256 | mtime |
|---|---|---|
| v0.1 `39d4fa4a-…20261008.md` | c06211565b9c | 10-12T01:02Z |
| Review `473ecbab-…20261008.md` | c605a26b51be | 10-08T05:28Z |
| `roadmap_input_20261007.md` | f869a6cb5ca6 | 10-07T07:18Z |
| `CONDITIONS_POLICY.md` | 3957093c4be3 | 10-07T07:25Z |
| `cases/ledger.md` | 14783bfa9847 | 10-08T03:56Z |
| `cases/mapping_v701.md` | 33ecd7766dd8 | 10-08T06:13Z |
| `results/R2_gdb_v701.md` | f69d5eddf9c2 | 10-08T06:42Z |
| `results/R2_cord_v701.md` | 0451101d552b | 10-08T07:13Z |
| `env/BASELINE_MEASURE.md` | 95c7b1082567 | 10-08T06:38Z |
| `env/CONDITIONS.md` | 960f070d3d57 | 10-08T07:13Z |
| `baselines/availability.md` | 46d94f30e02b | 10-08T02:35Z |

Research-record paths are relative to `/home/user/Shape-Perf/research/cfs_procrace/`.

I also used three companion reports from this investigation round:
- `01_cfs_osal_psp.md`, `02_cfs_cfe_core.md` and `03_cfs_app_patterns.md`. I re-checked at the pinned revision every code line I cite from them.
- `04_lit_dynamic.md`, for literature that it verified at abstract level.
- The user's earlier research note `fc87c4b6-…MLIR_Flight_Software_Race_Research_Note_202609302-2.md` §9, for the user's own position on MLIR.

### 1.2 Labels

- **FACT (code)**: the pinned code or a generated build file says it (`path:line`).
- **FACT (record)**: one of our research records says it.
- **FACT (doc)**: v0.1, the review or the roadmap says it, quoted or paraphrased.
- **FACT (lit)**: a paper. Each one carries a verification tag in §9.
- **INFERENCE**: my reasoning. It has not been executed.
- **UNVERIFIED**: not checked. Never used as the basis of a conclusion.

### 1.3 Limits

- **Web search.** The shared web-search budget ran out after 11 verification queries in this report. `mcp__Scholar_Gateway__semanticSearch` returned `ACCESS_DENIED`. Papers I did not verify myself either:
  - carry the tag "verified in 04 (V-abs)", if `04_lit_dynamic.md` verified them; or
  - are marked **UNVERIFIED** and are used only as pointers.
- **v0.1 [E1].** The five-case evidence summary `…5개사례_근거정리_20261008(1).md` is not in the uploads directory, so I could not check it. The case facts used here come from `cases/ledger.md` and `cases/mapping_v701.md`.
- **No new runs.** All statements about Clang behaviour on this code base (for example GCC-only warning flags under `-Werror`) are INFERENCE. They need the confirmation test listed in §4 T7.

---

## 2. Part 1 — Requirements extracted from all inputs

ID prefixes: **F** = functional, **S** = scientific validity, **R** = reporting, **K** = constraint.

"Task context" means the computed task text of this workflow, which states what the user wants: a best, open, academically valid tool, consistent with established techniques, built around cFS's real architecture, with no messy code generation. It is listed as context and is not treated as user authority beyond the relayed request.

### 2.1 Functional

| ID | Requirement | Source(s) |
|---|---|---|
| F1 | Use the task as the execution unit and the app as the owner. Keep four identities as separate fields: **executing task, owning app, called service, and the AppId stamped on an event**. | roadmap §1.2; v0.1 §2.1; ledger §3.1 ("SB API calls … run in the **caller's** task"); `R2_gdb_v701.md` §2 (ES task runs SB code with SB's AppId) |
| F2 | Resources are not only memory: named OSAL objects, ES app/task records, service identity globals, registration and route tables. A resource must be representable **before it exists** and across **generations**. | roadmap §1.4 item 1, §5 "자원 identity 연결"; v0.1 §6.1; ledger §1a |
| F3 | Publication is not the same as API return. Model the reserve → finalize window and the point where a spawned task becomes able to run. | roadmap §6.3 item 1; v0.1 §7.4; FACT (code) `cfe/modules/es/fsw/src/cfe_es_start.c:803,810,818` (reserve, create, publish); `osal/src/os/shared/src/osapi-idmap.c:475-480` |
| F4 | Keep the **required order** (with its provenance: code, API doc, issue, spec) apart from the **actual order**. Never add a required edge as an HB edge. | roadmap §5 first row, §6.1; v0.1 §7.1; policy L7-10 |
| F5 | The actual-order model must include program order, task creation, locks, ES polling barriers, system-state waits, delivered SB messages only, and timeouts. | v0.1 §7.1; `R2_cord_v701.md` §4.3 (both the create edges and the app-state edges were needed; the system-state wait was not modelled and is a false-positive risk); FACT (code) `cfe_es_start.c:878-888`, `cfe_es_api.c:585` |
| F6 | Status, guard and retry semantics: separate a transient failure, an allowed rejection, a final failure and an ignored status. | roadmap §5 "status·guard·retry"; review §4.2; FACT (code) `apps/cf/fsw/src/cf_cfdp.c:1279-1301`; `cfe_sb_task.c:138` (return value ignored) |
| F7 | Classify each instance as inactive, protected, fixed-order violation, schedule-dependent candidate, or unknown. | v0.1 §2.3, §7.2; roadmap §1.4, §7.3 |
| F8 | Detect violations that are **HB-ordered in the wrong direction**, not only unordered pairs. | INFERENCE from `R2_gdb_v701.md` §2 and `R2_cord_v701.md` §4.3, §7 (24 uses, all ordered by S1) |
| F9 | Configuration awareness:<br>- the generated startup script and core object table;<br>- built tables;<br>- the **effective** priority, policy and affinity;<br>- the effective queue depth.<br>Report "inactive under B" separately from "no violation". | v0.1 §4.1; review §4.1; `mapping_v701.md` §2; `env/CONDITIONS.md` E4, E5; FACT (code) `build-native_std/native/default_cpu1/cpu1/cfe_static_module_list.c:36-48`, `psp/fsw/pc-linux/src/cfe_psp_start.c:184-189` |
| F10 | A recorder that works during initialization. It must not depend on EVS, SB, TIME or `GetAppID`. Per-task buffers, a bootstrap token, and loss accounting. | roadmap §5 last row, §6.3 item 5; review §8.3; v0.1 §8.4, §9.2 |
| F11 | Record the value or return code that the program itself used. Never re-read shared state for logging. | roadmap §6.3 item 4; v0.1 §9.3 |
| F12 | Cover dynamically loaded modules: apps and libraries loaded by ES, and SBN modules loaded later. | roadmap §6.3 item 8; v0.1 §4.1; FACT (code) `cfe_es_apps.c:499,522`; `apps/sbn/fsw/src/sbn_app.c:1065-1088` |
| F13 | Never produce infeasible executions: no run before creation, no lock bypass. Control must be event-based. | roadmap §6.4; v0.1 §10; policy L22 |
| F14 | Three modes: observation, exploration and prediction. | roadmap §6.4 |
| F15 | Discover candidates that were **not** registered beforehand. Rules must be reusable beyond the five cases. | task context; roadmap §2.3 "[미정]", §6.1 last paragraph, §9.3; v0.1 §14.1 last row |
| F16 | Treat lossy-channel outcomes as first-class results: no subscriber (success returned to the sender) and queue full. | INFERENCE from FACT (code) `cfe/modules/sb/fsw/src/cfe_sb_priv.c:1083-1086` and FACT (record) `env/BASELINE_MEASURE.md` §3.3, §3.5 |

### 2.2 Scientific validity

| ID | Requirement | Source(s) |
|---|---|---|
| S1 | Graded evidence levels:<br>- candidate;<br>- model witness;<br>- observed;<br>- verified order violation;<br>- verified scheduling race;<br>- error-causing violation;<br>- deterministic dependency error;<br>- protected;<br>- unknown. | roadmap §10.1; v0.1 §2.3, §13 last paragraph |
| S2 | No arbitrary conditions. Every condition is `from-reference`, `deviation` or `unspecified_by_reference`. | policy L3, L14-17 |
| S3 | Tool parameters (delays, budgets, gates) are not arbitrary constants. Use event-based control or reference constants. | policy L22 |
| S4 | The five cases are design cases, not a held-out set. C3 and R2 are related. Derived tests are not new bugs. | roadmap §2 opening line, §9.3; v0.1 §8 opening, §14.2; ledger §3.1 |
| S5 | Comparisons with existing tools must separate the original tool, an extension and a reimplementation. Unsupported or not-run is never a false negative. | roadmap §3.5, §4.7; review §8.1; `baselines/availability.md` "Status at a glance" |
| S6 | If R2 is deterministic, it leaves the race-TP denominator and is reported as a dependency error. | roadmap §4.7 last paragraph, §7.3 |
| S7 | "Ordering varied" ≠ "order violation" ≠ "error". | review §3; roadmap §1.4 item 3 |
| S8 | Quantify perturbation against an uninstrumented run. The debugger shifts OPERATIONAL from 1613–1636 ms to 2244–2693 ms. | `R2_gdb_v701.md` §1; `R2_cord_v701.md` §4.4; review §8.3; v0.1 §11.4 |
| S9 | k of k runs is not proof of determinism. Determinism needs a model or code argument. | `mapping_v701.md` §1.3; `R2_gdb_v701.md` §3; v0.1 §2.3 |
| S10 | Label each input as original, public-code-derived, role-derived, or synthetic control. | roadmap §7.1; ledger §1b |
| S11 | Ablations remove one model element at a time on the same engine. | roadmap §9.2; v0.1 §14.4 |
| S12 | Freeze rules and contracts, then evaluate on cases not used for design. | roadmap §9.3, RQ3; v0.1 §14.1 last row, §15 step 7 |
| S13 | External baselines are limited to process-level and system-level concurrency tools. Thread data-race tools are excluded **as baselines**. | roadmap §3.1 (user decision) |
| S14 | Each new feature traces to an established gap. No superiority claims where existing tools suffice. | roadmap §5, §8.2, §11; review §8.2 |

### 2.3 Reporting

| ID | Requirement | Source(s) |
|---|---|---|
| R1 | A multi-axis result record (identity, context, target, contract, reachability, order, model, observation, impact, reproduction kind, cost). | v0.1 §13; roadmap §10.3 (YAML) |
| R2 | `NOT_RUN`, `NOT_MEASURED` and `NOT_FROZEN` are never zero or "pass". | roadmap §10.3 |
| R3 | Report the denominators. Keep independent cases separate from repeated runs. | roadmap §10.2 |
| R4 | Report four costs separately (analysis, exploration, recording, control), plus the manual specification effort. | v0.1 §6.2 last paragraph, §11.1; roadmap RQ5 |
| R5 | A claims table that includes the claims not made. | review §11; `R2_gdb_v701.md` §5; `R2_cord_v701.md` §8 |
| R6 | Record the basis B (build, configuration, environment manifest) with every verdict. | v0.1 §2.3, §4.1 |
| R7 | Source-linked explanations. | roadmap §11 row 4; v0.1 §4.2 |

### 2.4 Constraints

| ID | Constraint | Source |
|---|---|---|
| K1 | The reference build uses GCC 13.3 (`/usr/bin/cc` → `x86_64-linux-gnu-gcc-13`), `CMAKE_BUILD_TYPE=debug`, no `-O`, `-g`, `-std=c99 -pedantic -Wall -Werror`, and GCC-specific `-Wno-stringop-*` and `-Wcast-align=strict`. | FACT (code/build) `build-native_std/native/default_cpu1/CMakeCache.txt:283,289`; `compile_commands.json` (1417 entries, all `/usr/bin/cc`); `sample_defs/arch_build_custom.cmake:28-33`; `sample_defs/arch_build_custom_native.cmake:8-11` |
| K2 | Clang/LLVM 18.1.3, clang-tidy-18 and clang-tools-18 are installed. **MLIR is not installed** (0 MLIR libraries in `/usr/lib/llvm-18/lib`). `mlir-18-tools` 18.1.3 is available from Ubuntu universe. Installing anything needs a recorded decision. | FACT (host, `llvm-config --version`, `dpkg -l`, `apt-cache policy`); task rules |
| K3 | Host: 4 vCPU, 15 GiB RAM, 18 GB free disk. | FACT (host, `nproc`, `free`, `df`) |
| K4 | ENV facts:<br>- all threads run `SCHED_OTHER` and OSAL overrides every priority to 255;<br>- `msg_max` is 10 (247 of 584 requested slots);<br>- `/dev/shm/osal:RAM` and `EEPROM.DAT` persist;<br>- one cFS process at a time (E13). | `env/CONDITIONS.md` E4, E5, E7, E13; `env/BASELINE_MEASURE.md` §0, §3.1, §4 |
| K5 | The PSP pins every task whose name starts with `CFE_` to CPU 0. This is **not in the environment records**, and whether the call succeeds is not measured. | FACT (code) `psp/fsw/pc-linux/src/cfe_psp_start.c:184-189` (return value ignored) |
| K6 | No existing process-race tool can run here. RacePro needs an i386 Linux 2.6.35 kernel; the other seven have no reachable implementation. | `baselines/availability.md` "Status at a glance", §0 A3 |
| K7 | CORD is retired. Its code may not be reused; its *results* stay as recorded observations. | task context; v0.1 §1 table ("계승하지 않는 것") |
| K8 | No messy code generation. The architecture must be clean and reviewable. | task context |
| K9 | The pinned tree is read-only. Instrumented builds use copies, recorded as such. | task rules; precedent `env/CONDITIONS.md` H-9, H-10 |

---

## 3. Part 2 — Section-by-section review of v0.1

Each subsection has four parts: **Sound**, **Risky or under-specified**, **Missing**, and **Recommendation**.

### v0.1 §1 Design conclusion (pipeline, decision table)

- **Sound.**
  - The overall pattern is to find candidates first, statically or from specifications, and then verify only the needed points at run time. This is an established design. Examples:
    - RaceMob runs a static race detector and then validates candidates dynamically (Kasikci, Zamfir, Candea, SOSP 2013; verified, §9).
    - RacePro validates candidates by replay (verified in 04).
  - "Do not record everything first" is right. So is listing CORD as not inherited.
- **Risky.**
  - The first scope is "what the five cases need in common" (v0.1 §1 third paragraph; decision table row "첫 적용 범위"). That scopes the tool to the cases (see §5).
  - The pipeline is strictly forward: static → model → classification → exploration → execution. Nothing feeds observed traces back into the static model, yet cFS needs that. Entry functions, `dlsym` results, table contents and the effective configuration are cheapest to read from one real boot.
- **Missing.**
  - A **decision gate** per stage: what result would make us drop MLIR, drop SMT, or drop exploration?
  - A statement of which established technique each stage borrows.
- **Recommendation: Modify.**
  - Keep the "candidates, then selective verification" spine.
  - Make it a loop: static census ↔ boot trace.
  - Define the first scope by **generic contract classes**, not by cases (§5.2).

### v0.1 §2 Terms and the targets of judgement

- **Sound.**
  - "Process-level" is defined as logical work across apps and tasks, independent of PIDs. This matches roadmap §1.2.
  - The four-identity separation (§2.1) is right. R2 depends on it: the ES task executes SB code and stamps SB's AppId (`R2_gdb_v701.md` §2).
  - The three-part definition (requirement, actual order, allowed outcome) in §2.2 is right. It matches the "readiness contract with legal outcomes" gap (G1) in `04_lit_dynamic.md` §5.
  - "Repeated failure does not prove fixed order" and "missing HB does not prove a feasible race" are both right. They match S9 and Netzer–Miller's distinction between feasible and apparent races (verified in 04).
- **Risky.**
  - `Exec(B)` is undefined. B fixes build, configuration, inputs and initial state, but not the **scheduler model** or the **timing model**. Both decide membership in `Exec(B)` (§4 T5).
  - "Fixed-order defect … 순서 근거가 필요하다" is the right obligation, but the proof form is unspecified.
- **Missing.**
  - A formal contract definition. The natural established concept is **typestate**: the operations allowed on an object depend on its state (Strom and Yemini, IEEE TSE 1986; verified, §9). A readiness contract is a typestate precondition on a shared resource, checked at a use event in another task.
  - A definition of the **HB-ordered-wrong-way** class, as distinct from an unordered race.
- **Recommendation: Keep, but formalize.** Concretely:
  - contract instance κ = (use site u, resource r, required state set S, allowed-outcome set A);
  - violation predicate V(e) over an execution e;
  - verdicts are quantified over `Exec(B, σ)`, where σ is a **named scheduler and timing semantics** (§4 T5).

### v0.1 §3 Scope and assumptions

- **Sound.** v0.1 lists what is out of scope and says "이 항목이 후보에 영향을 주면 생략해서 안전 판정을 내리지 않는다" (missing semantics never produce a "safe" verdict). That is the right rule, the same candour as KCSAN's documented limits (verified in 04). The note on sequential-consistency assumptions (§3.2, last paragraph) is right.
- **Risky.**
  - The first scope covers startup readiness, named lookups and registration. It omits SB route readiness (subscribe-before-send) and pipe capacity. Yet these are the only *measured* order-dependent losses in the default configuration (`env/BASELINE_MEASURE.md` §3.3: drops of 33, 37, 4, 49, 45, 41 per run; run A3 was "a different interleaving"; §3.5: `NoSubscribersCounter` 15–26).
  - A reviewer will ask why the tool ignores the visible phenomena in its own testbed.
- **Missing.**
  - Ad hoc synchronization that real cFE uses. `CFE_ES_Global.SystemState` is a `volatile sig_atomic_t` (`cfe/modules/es/fsw/src/cfe_es_global.h:151`). It is written without a lock (`cfe_es_start.c:183,194,216,234`) and polled without a lock (`cfe/modules/es/fsw/src/cfe_es_api.c:585`). The model must treat this as a sync edge under an explicit visibility assumption, and must not report it as a bug.
  - Ad hoc synchronization is a known source of both false alarms and real bugs (Xiong et al., OSDI 2010; verified in 04).
- **Recommendation: Modify.** Add two contract classes to the first scope: route readiness and capacity (RC5/RC14 in `03_cfs_app_patterns.md` §4). Their observation is cheap: the SB housekeeping counters are an existing oracle (`env/BASELINE_MEASURE.md` §2.3, §3.2). Add an explicit row for ad hoc synchronization.

### v0.1 §4 Overall structure, inputs and outputs

- **Sound.**
  - The manifests (source, build, configuration, environment) are listed.
  - "When docs and the generated files differ, the generated files win" is right. R2's order really comes from a CMake-generated array: `build-native_std/native/default_cpu1/cpu1/cfe_static_module_list.c:36-48` lists ES, EVS, FS, SB, TBL, TIME, … and `cfe_es_start.c:754,778` consumes it.
  - Flagging missing dynamically loaded apps is right.
- **Risky.** A single mermaid flow hides the fact that **classification needs both** static facts (for "all paths") **and** a boot trace (for the effective configuration).
- **Missing.**
  - In the environment manifest:
    - the **effective affinity**: the PSP pins `CFE_*` tasks to CPU 0 (`cfe_psp_start.c:184-189`), and this is not recorded anywhere;
    - the effective policy and priority, which v0.1 §12 mentions;
    - per-pipe effective depth, which v0.1 §12 also mentions.
  - A versioned **contract library** as an input, separate from the "계약" (contract) row: templates plus API annotations, each with provenance and a hash. This is needed for S12.
  - A **loader model**: startup script → module → entry symbol → task.
- **Recommendation: Keep, with these additions.**

### v0.1 §5 Roles of Clang/LLVM and MLIR

- **Sound.**
  - "LLVM and MLIR are not either/or." "MLIR does not by itself create cross-layer meaning or race verdicts." "Region nesting is not concurrency; SSA is not inter-app synchronization." "MLIR adds no detection power over the same graph." "The first implementation is a small dialect." All of this is correct.
  - It also agrees with the user's earlier note (`fc87c4b6…` §9.2, §9.4).
  - Using `compile_commands.json` is correct: it exists at `build-native_std/native/default_cpu1/compile_commands.json`.
- **Risky.**
  - **(a) MLIR's role reduces to a container.** In v0.1 §5.1 MLIR receives extracted *facts*, not code. So MLIR's main strengths are not used: progressive lowering of programs, and dataflow over SSA code. Left are verifiers, a pass manager and location tracking. A typed graph schema with a validator, or a relational fact base, gives the same (§4 T1).
  - **(b) Cost.**
    - MLIR is not installed (K2).
    - A dialect needs ODS/TableGen definitions and a C++ build against `libmlir`.
    - That adds code-generation machinery to the toolchain, which pulls against K8.
    - Whether the MLIR SMT dialect named in v0.1 [R9] exists in the installable MLIR 18.1.3 is **UNVERIFIED**.
  - **(c) [R8] LLVM alias analysis** is built for intraprocedural, optimization-oriented queries. The precision problems that matter here are elsewhere (§4 T3).
- **Missing.**
  - A statement of the **compiler mismatch** and the equivalence check (§4 T7).
  - The fact that the code base is strict C99 (`-std=c99 -pedantic`), which lowers front-end risk.
  - The fact that upstream ships an example Clang toolchain (`sample_defs/toolchain-i686-linux-clang.cmake:12-13`).
  - The fact that nasa/cFS already runs **CodeQL** over these sources in CI with the normal build (`.github/workflows/codeql-reusable.yml:144-158`). That is a working precedent for "extract facts from the real build, query relationally".
- **Recommendation: Modify.**
  - Keep Clang/LLVM fact extraction.
  - Make the semantic model a **serialization-independent schema**: entities, events, edges, contracts, provenance.
  - Implement the first version as plain typed data with a validator.
  - Run the user's own proposed comparison (`fc87c4b6…` §9.4), MLIR versus a plain graph, as a gate, and keep MLIR only if it wins on a stated criterion: lines of code, analysis reuse, provenance fidelity, or runtime.
  - Do not let the paper's contribution depend on MLIR.

### v0.1 §6 Resource identity and contract authoring

- **Sound.**
  - The resource key includes scope and lifecycle generation, and "absent" is a representable state (§6.1).
  - "Do not use one pointer address or one OSAL ID as a permanent identifier" is correct: OSAL IDs carry serial numbers and are reused (`01_cfs_osal_psp.md` §1.2).
  - The provenance table in §6.2 is excellent and maps directly onto policy L7-10.
  - "Count manual contract time as cost" is right (R4).
- **Risky.**
  - **Circularity.** The example contract `sb_event_identity_ready` (§6.3) is written knowing R2's answer. If evaluation uses it, detecting R2 is guaranteed.
  - "자동 추출의 목표는 모든 계약을 발명하는 것이 아니다 … 소수의 근거 있는 계약" (the goal is not to invent every contract; start with a few well-founded ones) is reasonable. Without a rule for *which* few, though, they become the five cases' contracts.
  - Code-inferred conventions are only "hypotheses" (§6.2 row 4). That is safe, but the design gives no mechanism to generate them.
- **Missing.**
  - **Generic templates** per resource class, instantiated automatically at every API call site (§5.2).
  - A **documentation-first** source: cFE API headers state preconditions. Example: `cfe_evs.h:169-172`, "The Application ID must correspond to a registered application or core service. For messages outside the context of a registered application (for example early in app initialization …) #CFE_ES_WriteToSysLog can be used". That sentence also defines R2's *allowed-outcome* question: early use should go to the syslog, not to the event API.
  - Instance discovery by belief inference, for contracts nobody wrote down (Engler et al., SOSP 2001; verified, §9).
- **Recommendation: Modify.**
  - Contracts become **templates plus API annotations**, authored with a timestamp and hash *before* case-specific evaluation.
  - Case-specific contracts move to the evaluation appendix as "oracle refinements", labelled as such.

### v0.1 §7 Actual-order model and exploration

- **Sound.** All of the following are correct and match established practice:
  - required, actual and observed order are separate, and "adding required order as HB erases the bug" (§7.1);
  - not every give is linked to every take, and priority and sleep are not synchronization (§7.1);
  - the first classification is used to save exploration (§7.2);
  - the slicing caveat about third-party tasks (§7.3);
  - APIs are not treated as atomic (§7.4);
  - partial-order reduction only for truly independent steps (§7.5).

  The established practice includes DPOR (Flanagan and Godefroid, POPL 2005) and PCT (Burckhardt et al., ASPLOS 2010), both verified in 04.
- **Risky.**
  - **(a) No scheduler or timing semantics** (§4 T5).
  - **(b) Model fidelity.** "Bounded model checking per candidate" explores an *abstraction* that the extractor builds, not the C code. The proposal does not say how a task's code becomes a transition system. Concretely: which statements are events, how polling loops are summarised (`cfe_es_api.c:585-605`, `cfe_es_apps.c:573-596`), and how unbounded loops are bounded.
  - **(c) State explosion** (§4 T4).
  - **(d) Ordering on "SAT witness" before native confirmation.**
- **Missing.**
  - A **cheap first-tier classifier** from HB facts alone, before any search:
    - use ≺ publication on all paths → fixed-order candidate;
    - publication ≺ use on all paths → protected;
    - otherwise → unordered, so a race candidate.

    R2 is fully decided at this tier (§4 T5).
  - A **sync skeleton rule.** The set of OSAL and cFE synchronization APIs is small and closed, so all sync events of all tasks are always kept, and only data events are sliced. `01_cfs_osal_psp.md` §0 item 6 lists the primitives in use; condition variables and rwlocks have no non-test caller.
- **Recommendation: Modify.**
  - Use two tiers: HB classification, then exploration only for unordered candidates.
  - Name σ for every verdict.
  - SMT is optional, used only for prediction over a recorded trace with a stated encoding (the RVPredict and GPredict style, verified in 04).

### v0.1 §8 Design requirements from the five cases

- **Sound.** The case-by-case cautions are accurate and consistent with the ledger:
  - C1's path is latent: `apps/cf/fsw/src/cf_cfdp.c:1270` with empty names in the default table;
  - C5 is not the child-task API;
  - R2 must be checked for fixed order first;
  - a derived test is not a reproduction;
  - "24 rejected requests" is not "24 lost packets".
- **Risky.** Structuring *design* requirements per case makes the architecture look reverse-engineered from the answers.
- **Missing.**
  - v0.1 §8.5 step 5 (observe R2 without a debugger) was **already done**: 24/24 per boot in 3/3 runs, r2_c4–r2_c6 (`R2_cord_v701.md` §7). The design should cite this as an existing observation, made with a retired tool.
  - Also already known: 17 of the 24 R2 uses are `SEND_NO_SUBS` events. They exist only because ES's EVS packets are sent before anything subscribes (`R2_gdb_v701.md` §2). That is a **second, different contract** (route readiness) violated upstream of the identity contract. The design needs **violation chains**: one violation causing the use that violates another.
- **Recommendation: Modify.**
  - Move §8 into the evaluation plan.
  - In the design, keep only generic classes, and map each case onto them (§5.2).

### v0.1 §9 Selective instrumentation

- **Sound.** Per-task pre-allocated buffers, no heap, no file I/O and no EVS/SB in the hot path, no global mutex, drop-and-mark on overflow, a bootstrap token independent of `GetAppID`, a static side table for source mapping, and "a later timestamp is not HB" are all correct. They are also consistent with the CORD measurement: 626–632 events per boot, 0 drops, no measurable startup delay (`R2_cord_v701.md` §4.1, §4.4).
- **Risky.**
  - **Probe placement mechanism.** An LLVM-pass probe yields a Clang-built binary, which is a deviation from K1. Source patches are the CORD precedent but scale poorly.
  - **Lock context.** Probes must not run inside OSAL table locks and must not call OSAL name lookups (`01_cfs_osal_psp.md` §6.3).
  - **Crash flush.** The PSP replaces the recorder's signal handlers (`R2_cord_v701.md` §1, "Recorder side effects"), so an at-exit flush can lose data on a crash.
- **Missing.**
  - An **API-boundary observation layer** as the generic default. Every contract template is anchored on an API (OSAL, ES, SB, EVS, TBL), so wrapping those APIs observes all instances, not only case sites.
  - INFERENCE, to be tested: with the static link of the core (`cfe/cmake/target/CMakeLists.txt:185-191`, `--whole-archive`), GNU `ld --wrap=<symbol>` could interpose cross-object calls in the GCC build without any source edit. Intra-object calls would not be interposed, so they need a coverage check.
  - Out-pointer publications such as `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` (`cfe_sb_task.c:138`) can be observed at the wrapped callee's return: the pointer and the value.
- **Recommendation: Keep the principles, and Modify the mechanism.**
  - Default to API-boundary wrapping in the GCC build.
  - Add source-level probes only for global stores that no API call covers.
  - Record every probe set by hash.

### v0.1 §10 Counterexample verification and schedule control

- **Sound.**
  - Observation mode and control mode are separate.
  - Gates respect real creation, locks and waits.
  - No long sleeps.
  - The goal is a *pair* of executions (violating and conforming).
  - SAT and native reproduction are reported separately.

  This matches policy L22, CTrigger, RaceFuzzer and DCatch-style triggering (verified in 04).
- **Risky.** For fixed-order instances a conforming execution under the same B does not exist by definition. Any "pair" needs a changed B, which v0.1 §8.5 correctly labels "구성 수정 실험" (a configuration-change experiment).
- **Missing.**
  - A gate-placement rule: hold the **publisher** after reserve and before finalize, or hold the **consumer** before use, chosen from the HB analysis.
  - A deadlock watchdog whose bound comes from the reference: the cFE platform timeouts, not invented constants (S3).
- **Recommendation: Keep.** Add the gate-placement rule and the reference-bound watchdog.

### v0.1 §11 Overhead design

- **Sound.** Four costs are reported separately; the A/B/C/D groups are defined; no "<1%" target is chosen in advance; Linux numbers are not presented as RTOS guarantees.
- **Missing.** **Perturbation of order-sensitive outcomes**, not only time. Queue-full drops vary 4–49 between identical runs (`env/BASELINE_MEASURE.md` §3.3). Instrumentation can shift that distribution, so compare distributions (counts, init interleavings), not means.
- **Recommendation: Keep, and add perturbation metrics.**

### v0.1 §12 Controls for the current environment

- **Sound.** Every row matches M1:
  - 1.6 s versus 2.2–2.7 s under gdb;
  - 27 `SCHED_OTHER` threads;
  - 247 of 584 queue slots;
  - drops 33, 37, 4, 49, 45, 41;
  - `/dev/shm` persistence;
  - the cold first run.
- **Missing.**
  - CPU-0 pinning of `CFE_*` tasks (K5).
  - HS's failing init (`env/CONDITIONS.md` D-3).
  - The shared-container glibc upgrade note (`baselines/availability.md` §0, "Note for the ENV owner").
- **Recommendation: Keep, and add these.**

### v0.1 §13 Result reporting format

- **Sound.** It is a superset of the roadmap's YAML (§10.3), and it separates model counterexample, observed early use, requirement violation, schedule dependence and external failure.
- **Missing.**
  - A field for the **semantics σ** under which the order verdict holds.
  - A **violation-chain** link.
  - **Contract provenance**: template ID, annotation hash, authored before or after the case was seen.
- **Recommendation: Keep, and add these fields.**

### v0.1 §14 Evaluation design

- **Sound.** The RQs map to ablations. The design cases are not held-out. C3 and R2 are not double-counted. There is no rate without a denominator. The baseline-comparison checklist in §14.3 matches roadmap §3.5.
- **Risky.**
  - **No held-out protocol**: no source, no selection rule, no freeze point.
  - **Baselines are not runnable** (K6). §14.3 cannot be executed as written.
- **Missing.**
  - Internal **observation-model ablations** that stand in for the baselines' models, without calling them baselines (S13):
    - (i) syscall-only observation: the OSAL name probe showed zero syscalls for create plus lookup (`baselines/availability.md` §4.2);
    - (ii) memory-access-only HB with no framework rules;
    - (iii) framework HB without contracts.
  - A **template-only** run on the five cases, to show the cases are found without case-specific contracts.
- **Recommendation: Modify** (§5.6).

### v0.1 §15 Implementation order and completion criteria

- **Sound.** Deliverable-based gates, a small contract set, and not building the recorder first.
- **Risky.**
  - Steps 3–6 are ordered per case (R2 → C1 → C5 → C2/C3). That repeats the tailoring risk.
  - "작은 MLIR dialect" (a small MLIR dialect) appears in the prototype decomposition before any gate.
- **Recommendation: Modify.** Suggested order:
  1. manifests and the loader/configuration model;
  2. schema and generic templates, with API annotations taken from the headers;
  3. a static census of all instances in the 15 apps and the core;
  4. API-boundary recorder in the GCC build, with an equivalence check;
  5. a tier-1 HB classifier;
  6. tier-2 exploration for unordered candidates;
  7. freeze;
  8. evaluation: the five cases, then held-out.

### v0.1 §16 Open decisions and risks

- **Sound.** The listed items are real.
- **Missing.** Compiler mismatch, the MLIR install decision, the timing semantics σ, contract circularity, ad hoc synchronization and memory visibility, the held-out source, and the loader model.
- **Recommendation: Keep, and extend** with §4 of this report.

### v0.1 §17 References

- **Wrong.** [R6] title (see §0 item 10).
- **Unverifiable here.** [E1] (not provided); the [R5] comment anchor `#issuecomment-5588955819` (UNVERIFIED).
- **Sound.** [R7]–[R10] are pointers to official documentation and are used with appropriate caveats.
- **Recommendation: Modify.**

---

## 4. Part 3 — Specific technical risks

Each risk has the same layout: evidence, impact, mitigation, and the test that retires it. None of the tests was run.

### T1. MLIR dialect: cost versus benefit

- **Evidence.**
  - FACT (doc): v0.1 §5.2 says "동일한 의미 그래프를 정확히 구현했다면, MLIR을 사용했다는 이유만으로 탐지 능력이 더 높아지지는 않는다" (if the same semantic graph is implemented correctly, using MLIR does not by itself raise detection capability). The user's note `fc87c4b6…` §9.4 asks for a comparison with a plain graph.
  - FACT (host): MLIR is not installed. `mlir-18-tools` 18.1.3 is available.
  - FACT (lit): MLIR is an infrastructure for building reusable compiler IRs (Lattner et al., CGO 2021; verified, §9).
  - The extracted facts are not a program to be lowered (v0.1 §5.1, last paragraph).
- **Impact.**
  - Build and toolchain weight.
  - A TableGen/C++ code-generation layer, which pulls against K8.
  - A reviewer may see MLIR as incidental complexity.
  - Risk of a pinned-version mismatch between Clang 18 and the MLIR version.
- **Mitigation.**
  - A schema-first semantic model.
  - MLIR is optional and behind an adapter.
  - If MLIR is kept, justify it by something measurable: reuse of the MLIR dataflow framework for the tier-1 classifier, verifier-checked provenance, or a smaller analysis code size.
- **Retire by.** Implement tier-1 classification on (a) a plain typed graph and (b) the MLIR dialect, over the same facts. Compare lines of code, runtime, provenance completeness and verdict equality. Decide by the stated criterion. Installing MLIR packages is a PI decision recorded under the policy.

### T2. Contract authoring: burden and circularity

- **Evidence.**
  - FACT (doc): v0.1 §6.3's contract is R2-specific.
  - FACT (record): the five cases cover 3 of 15 contract kinds (`03_cfs_app_patterns.md` §5.1).
  - FACT (code): cFE headers carry preconditions in prose (`cfe_evs.h:168-172`). Return codes are enumerated per API (`cfe_evs.h:196-200`).
- **Impact.**
  - A guaranteed detection of the design cases.
  - Detection power that cannot be stated for unknown instances.
  - Unbounded manual effort if contracts are written per instance.
- **Mitigation.**
  - Contracts are per **API and resource class**, not per instance. Instances are found automatically.
  - Every template carries provenance: header line, issue, or code comment (for example the CF comment at `cf_cfdp.c:1272-1278`, an in-code admission of a race).
  - Record each template's authoring time relative to when each case was used.
  - Measure the effort: number of templates, annotated APIs and person-hours (R4).
- **Retire by.** A template-only run that finds R2, and C1 in its derived configuration, with zero case-specific contracts. Report which verdicts change when the case refinements are added.

### T3. Static precision: function pointers, dynamically loaded apps, configuration-driven identity

- **Evidence.** All FACT (code):
  - All app and core task bodies are reached through a stored function pointer: `CFE_ES_TaskEntryPoint` → `CFE_ES_GetTaskFunction` reads `TaskRecPtr->EntryFunc` → `(*RealEntryFunc)()` (`cfe_es_apps.c:581,639`).
  - App entries come from `OS_ModuleLoad` plus `OS_ModuleSymbolLookup` on the script's entry name (`cfe_es_apps.c:499,522`; `build-native_std/exe/cpu1/cf/cfe_es_startup.scr:3-17`, column 3, for example `CF_AppMain`).
  - Core entries come from `GLOBAL_CONFIGDATA.CoreObjectTable` (`cfe_es_start.c:754,778`), a CMake-generated array (`cfe_static_module_list.c:36-48`).
  - Callbacks:
    - `CFE_Config_IterateAll(NULL, CFE_ES_ModSrcVerCallback)` (`cfe_es_task.c:265`; `cfe_config_get.c:184`). It produced 13 of R2's 24 uses, and its iteration count depends on the generated version table (`R2_cord_v701.md` §4.2, §7).
    - TBL validation callbacks (`cfe/modules/core_api/fsw/inc/cfe_tbl.h:200,238`).
    - OSAL timer callbacks (`osal/src/os/inc/osapi-timer.h:34`).
  - Dynamic modules after startup: SBN loads modules and looks up function tables by name from its configuration (`apps/sbn/fsw/src/sbn_app.c:1065-1088`).
  - MD and MM resolve symbols by name at run time (`apps/md/fsw/src/md_utils.c:214`; `apps/mm/fsw/src/mm_cmds.c:143`).
  - Out-pointer publications such as `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` (`cfe_sb_task.c:138`).
  - Names come from tables: `CF_AppData.config_table->chan[i].sem_name` (`cf_cfdp.c:1270,1283`).
- **Impact.**
  - A naive call graph has **no edges** from `main` into any app or core task.
  - A whole-program points-to analysis would merge all entries through `EntryFunc`, which is imprecise. Without table values, name and MsgId identity is unknown.
- **Mitigation, in order of payoff (INFERENCE).**
  1. A **loader and configuration model** built from the generated script, the core object table, the built tables and the module list. It gives exact entry functions per task and is deterministic per B.
  2. **API mod/ref summaries** taken from header annotations. For example, `CFE_ES_GetAppID` writes `*AppIdPtr`. This avoids most interprocedural pointer analysis.
  3. **Value analysis** over table initializers and macros, for names and MsgIds.
  4. Only then, a whole-program points-to analysis for residual indirect calls and fields. SVF is the established LLVM-based option (Sui and Xue, CC 2016; verified, §9). Unresolved effects become `unknown_effect`, as v0.1 §5.3 says.
  5. Cross-check against one boot trace: `dlsym` results, the actual task to entry mapping, observed resource names.

  The existing static race detectors for C show what precision costs. LOCKSMITH (PLDI 2006), RELAY (ESEC/FSE 2007) and Goblint (ASE 2016) were all verified, and Goblint's evaluation says practical use "requires inferring environmental and domain-specific assumptions". That is exactly the cFS loader and phase model.
- **Retire by.** A coverage report:
  - the share of the 20 apps' and core tasks' entry functions resolved by the loader model;
  - the share of indirect call sites resolved;
  - the share of contract-relevant resource keys that are concrete versus `unknown`.

### T4. State explosion in bounded exploration

- **Evidence.**
  - FACT (record): 27 threads per boot; 20 apps plus child tasks (`env/BASELINE_MEASURE.md` §4).
  - FACT (record): the limited CORD recorder logged 626–632 events per boot (`R2_cord_v701.md` §4.1).
  - FACT (code): polling loops with 50 ms delays and timeouts: `cfe_es_apps.c:573-596`, `cfe_es_api.c:585-605`, `cfe_es_start.c:871-915`; and the OSAL retry loop `osapi-idmap.c:475-485`.
  - FACT (record): app inits overlap. Script order does not order init steps (`03_cfs_app_patterns.md` §6.1).
- **Impact.** Exhaustive interleaving of a whole boot is infeasible. Per-candidate slicing can drop a third task's synchronization (v0.1 §7.3 notes this).
- **Mitigation (INFERENCE).**
  1. Tier-1 HB classification removes every ordered instance without search.
  2. Keep the **sync skeleton** of all tasks; slice data events only.
  3. **Summarise polling loops** as a "wait until predicate, or timeout" step with reference bounds.
  4. Search with DPOR (exhaustive within a bound) or PCT (probabilistic, small bug depth), both verified in 04. Context bounding is another option but is **UNVERIFIED** here.
  5. Always report the bound and the timeout as `unknown`.
- **Retire by.** For each unordered candidate in the census, report states explored, bound reached or not, and time. The share of instances decided at tier 1 is a headline number.

### T5. Separating "fixed-order" from "schedule-dependent"

- **Evidence.**
  - FACT (doc): v0.1 §2.3 defines both relative to `Exec(B)`.
  - FACT (code), in the startup protocols:
    - the core serialization `CFE_ES_MainTaskSyncDelay(RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` (`cfe_es_start.c:835`), with panic on failure (`:838-845`);
    - the app barriers are **soft**. On timeout ES logs and continues (`cfe_es_start.c:210-213,225-228`; doc comment `cfe_es_internal_cfg.h:866-870`).
  - FACT (record): priorities do not take effect here (E4), and `CFE_*` tasks are pinned to CPU 0 (K5). C3's misorder came from RT priorities: TIME 60 versus EVS 61 and SB 64 (ledger §1a).
- **Impact.**
  - Without a stated σ, verdicts are either trivial or unsound:
    - untimed semantics makes every soft timeout reachable, so almost everything becomes "schedule-dependent";
    - observed timing makes things look fixed that are not.
  - Verdicts do not transfer between Linux `SCHED_OTHER` and an RT target.
- **Mitigation.** A proposed definition (INFERENCE):
  - Let `Exec(B, σ)` be the executions allowed by build, configuration and initial state B under the scheduling and timing semantics σ.
  - Define σ in **at least two** variants:
    - **σ_untimed**: any fair interleaving; every timeout or poll may expire nondeterministically; panic is a terminal outcome.
    - **σ_ENV**: the measured platform (policy, affinity, CPU count), with timing bounds taken only from reference constants; when timing matters, the result is reported as empirical.
    - Optionally **σ_RT1**: fixed-priority preemptive on a single core, which reproduces the C3 class.
  - For a contract instance κ with use u:
    - **inactive(B)** if no e reaches u;
    - **fixed-order(B, σ)** if every e that reaches u violates;
    - **protected(B, σ)** if no e violates;
    - **schedule-dependent(B, σ)** if both kinds exist.
  - A verdict is a pair (class, σ).
- **Worked example: R2 under σ_untimed** (INFERENCE from code; consistent with the HB chain rebuilt in `R2_cord_v701.md` §4.3).
  - ES-task uses happen inside `CFE_ES_TaskInit` (`cfe/modules/es/fsw/src/cfe_es_task.c:114`; use sites `:367-387`, `:466`). They come before that task's `CFE_ES_WaitForSystemState(CORE_READY, …)` (`cfe_es_task.c:134`), which raises its AppState to RUNNING under the ES lock (`cfe_es_api.c:528,547-549,571-576`). EVS has the same shape: `cfe/modules/evs/fsw/src/cfe_evs_task.c:213` (init), then `:229` (wait).
  - The ES main task reads that state under the same lock (`cfe_es_start.c:878-888`).
  - The ES main task then creates SB later in program order (`cfe_es_start.c:810`).
  - SB stores its AppId (`cfe_sb_task.c:138`).
  - If the wait times out, ES panics (`cfe_es_start.c:838-845`), so SB is never created.
  - Therefore no execution exists in which the store comes before those uses. R2's ES and EVS uses are **fixed-order even under σ_untimed**. This is a code-level proof shape, much stronger than "3/3 runs".
- **Worked example: C1** (derived configuration, INFERENCE). Under σ_untimed the provider can be delayed past 25 × 100 ms, so C1 is **schedule-dependent**. Under σ_ENV its frequency is empirical.
- **Worked example: C3 at 6.4.1** (INFERENCE from ledger §1a). Under σ_RT1 the TIME-first order is **fixed**. Under σ_ENV it is **schedule-dependent**.
- **Retire by.** The classifier emits (class, σ) for the five cases and the census. The R2 HB proof is shown as a path of cited edges.

### T6. Instrumentation perturbation

- **Evidence.**
  - FACT (record): gdb moves OPERATIONAL from 1613–1636 ms to 2244–2693 ms.
  - FACT (record): CORD at about 630 events showed no measurable shift: 1628–1635 ms (`R2_cord_v701.md` §4.4, §7).
  - FACT (record): the startup-burst losses vary 4–49 with no instrumentation (`env/BASELINE_MEASURE.md` §3.3).
- **Impact.** Generic API-boundary recording will log 10–100× more events. INFERENCE: this order of magnitude is not measured. Any global serialization creates false synchronization (roadmap §6.3 item 3).
- **Mitigation.** v0.1 §9.2 already has the right rules. In addition:
  - compare *distributions* of order-sensitive outcomes (overflow counts, init interleavings, the R2 use set) across the A, B and C groups;
  - keep probes out of OSAL table locks.
- **Retire by.** The A/B/C groups of v0.1 §11.4, extended with the perturbation metrics from §3 (v0.1 §11).

### T7. Compiler mismatch: GCC reference versus LLVM tooling

- **Evidence.** FACT:
  - the reference is GCC 13.3, debug, no `-O` (K1);
  - Clang 18.1.3 is available (K2);
  - the code is strict C99 with `-pedantic` (`sample_defs/arch_build_custom.cmake:28-33`);
  - `common_types.h` uses `__GNUC__`-guarded attributes (`osal/src/os/inc/common_types.h:60-68`), and Clang also takes that branch (INFERENCE: Clang defines `__GNUC__`);
  - GCC-only flags are present (`sample_defs/arch_build_custom_native.cmake:9`, `-Wcast-align=strict`; `sample_defs/mission_build_custom.cmake:31-36`, `-Wno-stringop-*` and `-Wno-format-*`);
  - upstream keeps a Clang toolchain example (`sample_defs/toolchain-i686-linux-clang.cmake:12-13`).
- **Impact (INFERENCE).**
  - Reusing the compile commands verbatim with Clang under `-Werror` may fail on the unknown warning options.
  - Facts are taken from a different front end, so macro expansion, constant folding and IR shape can differ.
  - An LLVM-instrumented binary would not be the reference binary.
- **Mitigation.**
  - Use Clang **for analysis only**: strip or translate the GCC-only flags, and record the transformation.
  - Anchor every fact to a source line plus the DWARF line table of the **GCC** binary. `mapping_v701.md` header shows the gdb `info line` precedent.
  - Instrument the GCC build: API-boundary wrapping (§3, v0.1 §9) or source probes.
  - If a Clang-built variant is ever run, it is a `deviation`, with a behavioural-equivalence check: same R2 use set, OPERATIONAL within the ENV range, same queue structure.
- **Retire by.** A one-time extraction audit: for every contract-relevant call site, Clang's and GCC's DWARF agree on file:line; list the exceptions.

### T8. Memory model and ad hoc synchronization

- **Evidence.** FACT (code):
  - `SystemState` is `volatile sig_atomic_t` (`cfe_es_global.h:151`), written and polled without a lock (`cfe_es_start.c:183-234`; `cfe_es_api.c:585`);
  - core identity globals are plain stores (`cfe_sb_task.c:138`);
  - `MainTaskSyncDelay` polls under the ES lock (`cfe_es_start.c:878-888`).
- **Impact.**
  - A memory-level detector flags `SystemState` (C11 data race) even though it is the intended synchronization.
  - Conversely, treating volatile polling as sequentially consistent HB is an assumption that may not hold on weak-memory multicore targets.
- **Mitigation.** Model volatile polling as an **assumed sync edge** with an explicit flag in the report (v0.1 §3.2 already hints at this). Keep lock-mediated edges as proven. Never report ad hoc edges as defects without a separate memory-model analysis.

### T9. Lossy queues and capacities truncated by the environment

- **Evidence.**
  - FACT (record): 22 of 26 pipes are truncated; 247 of 584 slots (`env/BASELINE_MEASURE.md` §3.1).
  - FACT (code): queue-full is non-blocking and only counted (`cfe_sb_priv.c:1177-1194`, per `env/BASELINE_MEASURE.md` §3.2); a send with no subscriber returns success (`cfe_sb_priv.c:1083-1086`).
- **Impact.** Whether a capacity contract is violated depends on the environment, so a verdict found under `msg_max` 10 does not transfer.
- **Mitigation.** Capacity is part of B. Report capacity violations as `(class, σ, capacity source)`.

### T10. Coverage holes from late module loading and restarts

- **Evidence.** FACT (code):
  - SBN loads modules on demand after OPERATIONAL (`sbn_app.c:1065-1088`);
  - HS can restart apps;
  - OSAL IDs and names are reused (`01_cfs_osal_psp.md` §1.2).
- **Mitigation.** Use generation-aware resource keys (v0.1 §6.1 already has these). Mark loader events in the trace. Report the configured modules that never loaded as coverage gaps.

### T11. Ground-truth scarcity and unrunnable baselines

- **Evidence.**
  - FACT (record): every baseline cell is `NOT_RUN` (`baselines/availability.md`).
  - FACT (record): the five cases are not independent: C3 and R2 share the resource family (ledger §3.1), and no case has an original reproduction (ledger §1b, last paragraph).
- **Mitigation.** The held-out protocol in §5.6. Internal observation-model ablations, not presented as baselines (S13). Wording rules from `baselines/availability.md` §5.4.

### T12. Scope and engineering risk against the paper timeline

- **INFERENCE.** Building all five stages (extractor, dialect, explorer, recorder, controller) before the first result is the main schedule risk.
- **Mitigation.** Tier 1 (census plus HB classification plus API-boundary observation) is publishable on its own, because it decides R2 and every ordered instance. Tier 2 (exploration) adds C1, C3 and C5-class results.

---

## 5. Part 4 — Making the tool open rather than tailored to five cases

### 5.1 Evidence that the v0.1 framing is at risk

- The first scope is derived from the five cases (v0.1 §1 table). The design requirements are listed per case (v0.1 §8). The only contract example is R2's (v0.1 §6.3). Steps 3–6 of the implementation order are per case (v0.1 §15).
- The five cases occupy 3 of the 15 contract kinds found in the bundle (`03_cfs_app_patterns.md` §5.1).
- Three of the uncovered kinds already show measured, run-varying losses in the default configuration (`03_cfs_app_patterns.md` §5.2; `env/BASELINE_MEASURE.md` §3.3, §3.5).

### 5.2 Generic resource and contract classes

These are typestate-style templates (INFERENCE; they build on `02_cfs_cfe_core.md` §1.16, §2.6–2.8 and `03_cfs_app_patterns.md` §4). Each one is defined once, on the framework API, and instantiated automatically at every call site.

| Class | State machine (sketch) | Required state at use, and legal outcomes | Example sites at the pin | Cases covered | Cheap oracle |
|---|---|---|---|---|---|
| **K-REG** Named registry object (OSAL objects; ES app/task/lib/counter; TBL registry; SB pipe names; loader symbols) | ABSENT → RESERVED → LIVE → (RESERVED) → FREE, with a generation number | LIVE for lookup or use. Legal: `NAME_NOT_FOUND` with a bounded retry (if documented), or an explicit error | OSAL `osapi-idmap.c:475-480`; ES `cfe_es_start.c:803/818`; CF `cf_cfdp.c:1282`; SCH_LAB and TIME timebase lookups (`03` §1) | C1, C5 (ES record), R2-S7 (inference) | API return codes |
| **K-ID** Service identity publication (a global ID written by the owner task, read by APIs acting on its behalf) | UNSET → SET(owner) → REGISTERED(EVS) | SET and REGISTERED before any on-behalf event. Legal: syslog instead of the event API (`cfe_evs.h:169-172`) | SB `cfe_sb_task.c:138,155`; TBL `cfe/modules/tbl/fsw/src/cfe_tbl_task.c:196`; EVS's own ID (`02` §1.16) | C3, R2 | `CFE_EVS_SendEventWithAppID` return code |
| **K-PHASE** Phase or readiness declaration | EARLY_INIT → LATE_INIT → RUNNING (→ STOPPED); system states CORE_STARTUP … OPERATIONAL | The declarer's readiness must imply the service is ready (C2's gap). Soft timeouts are legal | `cfe_es_api.c:469-471,566-576`; `cfe_es_start.c:210-234`; 5 app sites (`03` §4 RC7) | C2 | ES app-state table |
| **K-ROUTE** Pub/sub route readiness | NO_ROUTE → ROUTED(pipe) | ROUTED before a send that must be delivered. Legal: drop only if documented as acceptable | `cfe_sb_priv.c:1083-1086`; 48 subscribe and 41 transmit sites (`03` §4 RC5) | none (but 17 of R2's 24 uses are downstream of it) | SB `NoSubscribersCounter` |
| **K-CAP** Bounded-queue capacity under a burst | depth ≤ capacity | No `Q_FULL` for messages that must be delivered | `cfe_sb_priv.c:1177-1194`; 3 pipes overflow (`env/BASELINE_MEASURE.md` §3.3) | none | SB `PipeOverflowErrorCounter` |
| **K-SPAWN** Task spawn and registration | CREATED(runnable) → REGISTERED | REGISTERED before the task's first self-lookup | `cfe_es_apps.c:573-596,664-722` | C5 | `GetAppID` return code |
| **K-TBL** Table lifecycle | REGISTERED → LOADED → VALIDATED → ACTIVE; address lock flag | LOADED/ACTIVE before `GetAddress` | `02` §4 | none | TBL return codes |
| **K-PERSIST** CDS and critical-table restore | ABSENT → RESTORED / DEFAULTED | Defined by documented behaviour on reset type | `03` §4 RC8 | none | ES CDS status |
| **K-HANDOFF** Parent↔child, file and timer-callback handoffs | per pattern | per pattern | FM child, CF file TOCTOU comment, SCH_LAB timer (`03` §4 RC9, RC10, RC12) | none | app counters |

Openness then means two things. First, the classes come from the **framework API and its documentation**. Second, a case is only a test that a class must pass.

### 5.3 Contract sourcing without circularity

1. **Headers first.** Mine the `\par Assumptions` and the `\retval` blocks of `cfe/modules/core_api/fsw/inc/*.h` and `osal/src/os/inc/*.h` into machine-readable annotations. Example: `cfe_evs.h:168-172,196-200`. Each annotation keeps its file:line, which fits policy L9 ("고정 commit의 공식 문서", official documentation at the pinned commit).
2. **Code admissions second.** Example: the CF comment "There is a start up race condition…" at `cf_cfdp.c:1272-1278`.
3. **Issues last**, labelled "historical", as v0.1 §6.2 says.
4. **Timestamps and hashes.** Each template and annotation records when it was authored, relative to when each case was used. The paper reports which verdicts used a post-case refinement.
5. **Belief inference for unwritten contracts** (Engler et al., SOSP 2001; verified). Example (INFERENCE): most callers of `CFE_ES_GetAppID` check its status, but `cfe_sb_task.c:138` ignores it. A deviance ranking surfaces that site without knowing R2. The proportion of checked callers is not counted here.

### 5.4 Discovery of unknown candidates

- **Static census.** Every instance of every class in the core and the 15 apps, under B, gets a tier-1 verdict.
- **Dynamic candidate sources.** Both verified in 04:
  - pairs that were always observed in one order (CloudRaid style);
  - sequential per-app profiling to find potential publish→use communications independent of the observed boot order (Snowboard style).
- **Oracles that need no contract authoring.** API return codes at the boundary (`ILLEGAL_APP_ID`, `NAME_NOT_FOUND`, `ERR_RESOURCEID_NOT_VALID`) and the SB and EVS counters.
- **Configuration variation as an input dimension.** The tables and the schedule decide which paths are active (`03_cfs_app_patterns.md` §6.2, §7), and so does the script order. Each variant is a new B, never a "new bug".

### 5.5 Worked example of openness (INFERENCE; not run)

Take the documented `CFE_EVS_SendEventWithAppID` precondition (`cfe_evs.h:169`) as the only template input, and monitor it at the API boundary.

- Every R2 use would be flagged, because each call returns `CFE_EVS_APP_ILLEGAL_APP_ID` (`cfe/modules/evs/fsw/src/cfe_evs.c:185-188`). No SB-specific contract is needed.
- Static def-use then explains *which* publication was missing: `CFE_SB_Global.AppId`, published at `cfe_sb_task.c:138`.
- Instantiating the K-ID template over all services finds the same shape in TBL. `CFE_TBL_Global.TableTaskAppId` is published at `cfe_tbl_task.c:196`, and `tbl/fsw/src` has 20 `CFE_EVS_SendEventWithAppID` call sites by grep count, most of which pass that field.
- No TBL API call appears in the ES, EVS or SB task files (grep). Tier 1 would then classify the TBL instances as **protected under B**: every caller runs after CORE_READY.

This illustrates the intended output: one generic template, one known violation family (SB), and further instances classified with evidence, not ignored.

### 5.6 Held-out evaluation protocol (proposal)

1. **Freeze.** Templates, annotations, extractor and classifier versions, each recorded by hash, before any held-out item is examined.
2. **Held-out sources**, each counted separately and never added to "new real bugs":
   - (a) other cFS revisions: the cFE 6.4.x/6.5/6.6 tarballs and git revisions already identified in the ledger, as a version-transfer test. The ledger's 6.4.1 → 6.4.2 diff contains three fixes;
   - (b) other app sets and configurations, for example the cpu2 target present in `build-native_std/native/default_cpu2`, or tables that activate latent paths;
   - (c) issues mined with a **pre-registered** query on nasa/cFE, OSAL, PSP and apps (keywords and date cut fixed before the freeze). C1, C2, C3, C5 and R2 are excluded;
   - (d) seeded mutations, labelled synthetic: remove a wait, move a publication, swap two object-table entries;
   - (e) in-code admissions such as the CF comments.
3. **Metrics.** Per class:
   - instances found;
   - tier-1 decided share;
   - manually triaged precision on candidates, with the triage protocol stated;
   - confirmed observations;
   - unknown share and its causes;
   - cost.
4. **Ablations** on the same engine (S11):
   - remove the loader model;
   - remove the API summaries;
   - remove the phase edges;
   - collapse identities (one identity instead of four);
   - make APIs atomic;
   - syscall-only observation;
   - memory-only HB.
5. **Template-only versus refined** on the five design cases.

### 5.7 Openness indicators to report

- Classes covered versus classes found in the census.
- The share of instances discovered automatically versus authored.
- The share of verdicts that needed a case-specific refinement.
- Held-out results kept separate from design-case results.

---

## 6. Part 5 — Research-contribution framing

### 6.1 The strongest framing likely to survive review (INFERENCE)

> **Checking readiness contracts in framework-structured flight software.** In cFS, most order bugs are not unordered memory accesses. They are uses of framework resources (registries, identities, phases, routes, capacities) in a state that the documented API contract forbids. Many of these uses are *ordered the wrong way* by the framework's own startup protocol. We present a hybrid detector with three parts. (i) It builds a loader- and configuration-aware model of cFS from the real build. (ii) It instantiates documentation-sourced, typestate-style contracts at every API site. (iii) It classifies each instance, under a recorded execution basis and a named scheduling semantics, as inactive, protected, fixed-order violation, schedule-dependent candidate, or unknown, and it confirms the candidates by event-controlled native execution with low-perturbation API-boundary recording.

Why this framing is defensible:
- It builds on established mechanisms: typestate; HB and vector clocks; framework-aware HB rules (DroidRacer, CAFA, DCatch, verified in 04); static-then-dynamic validation (RaceMob); and environment models for static analysis of framework code. Two verified precedents for the last: SDV's OS and environment models with API usage rules (Ball et al., EuroSys 2006), and FlowDroid's lifecycle model (Arzt et al., PLDI 2014).
- It claims novelty only for the **combination and the cFS semantics**: gaps G1–G8 in `04_lit_dynamic.md` §5. 04 concludes that no individual mechanism is novel.
- Flight software has a direct precedent for model-checking-based concurrency analysis that found real errors: Havelund, Lowry and Penix, SPIN on the Deep Space 1 Remote Agent, IEEE TSE 27(8), 2001 (verified). That supports the domain motivation without claiming novelty for model checking.

### 6.2 Contributions and the evidence each one needs

| Contribution | Evidence required |
|---|---|
| C1 The formulation: a contract with legal outcomes; a verdict quantified over (B, σ); the HB-ordered-wrong-way class | Definitions; R2 proved fixed-order from code edges (§4 T5); C1 and C3 shown σ-dependent |
| C2 The cFS semantic model: loader and configuration model, HB rules, template library with provenance | Coverage numbers from T3; template count and authoring effort (R4) |
| C3 The tool | Two tiers; the A/B/C/D overhead and perturbation results |
| C4 An empirical study on the v7.0.1 bundle | Census per class; R2 confirmed by the new recorder; route and capacity losses quantified; new instances classified; held-out results |
| C5 An observation-model analysis of existing process-level approaches | Labelled "analysis plus probe, not run", e.g. "zero syscalls for an OSAL name create and lookup" (`baselines/availability.md` §4) |

### 6.3 Claims that must not be made

1. "Sound", "complete", "detects all races", or "no false positives". The R2 HB model, for instance, did not cover the system-state wait (`R2_cord_v701.md` §4.3, §8).
2. "MLIR improves detection or precision", unless T1's comparison shows a measured win, and then only on that metric.
3. "Existing tools fail to detect C1–R2". Every cell is `NOT_RUN` (`baselines/availability.md`; review §11).
4. "Five independent harmful races", or any frequency claim for cFS (roadmap §2; v0.1 §8).
5. "R2 is a race". The evidence and the code proof shape say it is fixed-order (`R2_gdb_v701.md` §3; §4 T5 here). It belongs to the dependency-error class (roadmap §7.3).
6. "Deterministic" based on k-of-k runs. Determinism is claimed only from the σ-stated code proof.
7. "Generalizes", based on the design cases.
8. "New bugs", for derived, role-based or seeded tests.
9. "C3 or C5 reproduced on v7.0.1". Their buggy code is absent there (`mapping_v701.md` §3).
10. "Flight-representative timing or overhead" from Linux `SCHED_OTHER` in permissive mode (v0.1 §11.4; E4).
11. "First process-race detector". RacePro, SimRacer and others exist (roadmap §3.2).
12. Novelty for HB/VC, DPOR, PCT, lockset, delay injection or SMT prediction as mechanisms (`04_lit_dynamic.md` §5, last paragraph).
13. "24 R2 violations without a debugger" for the build without `.git`. That build gave 11. The 24 holds for r2_c4–r2_c6 with the restored version table (`R2_cord_v701.md` §4.2, §7). Neither number is a result of the new tool until its own recorder reproduces it.
14. Any priority-based verdict carried from σ_RT1 to σ_ENV, or the reverse.
15. "Lost events" stated as "lost packets". EVS rejected the request before any packet was built (v0.1 §8.5; `R2_gdb_v701.md` §2).

---

## 7. Consolidated recommendation per v0.1 section

| v0.1 § | Keep / Modify / Drop | One-line reason |
|---|---|---|
| 1 Design conclusion | **Modify** | Keep "candidates, then selective verification". Make it a static↔trace loop and scope it by class, not by case |
| 2 Terms | **Keep, formalize** | Right distinctions. Add typestate-style contracts, the σ-quantified verdict, and the HB-ordered-wrong-way class |
| 3 Scope | **Modify** | Add route, capacity and ad hoc synchronization. They are the measured phenomena |
| 4 Structure, I/O | **Keep, extend** | Add the loader model, effective affinity, and a contract library with provenance |
| 5 Clang/LLVM/MLIR | **Modify** | Keep Clang fact extraction. Schema-first model. MLIR optional, behind a gate. State the compiler mismatch |
| 6 Resources, contracts | **Modify** | Generic templates from headers; automatic instances; anti-circularity bookkeeping |
| 7 Order model, exploration | **Modify** | Tier-1 HB classifier before search; sync skeleton; named σ; SMT optional |
| 8 Five cases | **Modify (move)** | Into evaluation. Record R2's existing non-debugger observation and the violation chain |
| 9 Instrumentation | **Keep principles, modify mechanism** | API-boundary wrapping in the GCC build by default. No probes inside OSAL locks |
| 10 Verification, control | **Keep** | Add the gate-placement rule and the reference-bound watchdog |
| 11 Overhead | **Keep, extend** | Add the perturbation of order-sensitive distributions |
| 12 ENV controls | **Keep, extend** | Add CPU-0 pinning of `CFE_*` tasks, the HS init failure and the glibc note |
| 13 Reporting | **Keep, extend** | Add σ, violation chain and contract provenance |
| 14 Evaluation | **Modify** | Pre-registered held-out protocol; internal observation-model ablations; template-only runs |
| 15 Implementation order | **Modify** | Class-first order; MLIR after its gate |
| 16 Decisions, risks | **Keep, extend** | Add T1–T12 |
| 17 References | **Modify** | Fix the RacePro title; supply or retire [E1] |

Nothing in v0.1 should be **dropped outright**. The MLIR-first commitment is demoted, not dropped, because the user may still want it once the gate test has run.

---

## 8. Open questions for the PI (decisions, not chosen here)

1. **MLIR.** May `mlir-18-tools` and `libmlir-18-dev` be installed (an environment change recorded under the policy), or is MLIR deferred until the T1 comparison?
2. **σ set.** Which semantics are required in the paper: σ_untimed and σ_ENV only, or also σ_RT1, which needs either a root/RT-capable environment (D-4) or a pure model?
3. **Instrumentation mechanism.** Approve testing linker wrapping (`--wrap`) on a recorded copy of the ENV build (precedent H-9/H-10), versus source probes?
4. **Clang for analysis.** Approve the flag-filtering transformation for the analysis-only Clang runs (recorded; not a build of the reference)?
5. **Held-out sources.** Which of §5.6 (a)–(e) are acceptable? Who runs the pre-registered issue query, and when relative to the freeze?
6. **Route and capacity classes in the first scope.** Including them changes what the paper is about. It moves from "startup identity bugs" to "framework readiness contracts".
7. **[E1].** Provide `…5개사례_근거정리_20261008(1).md`, or confirm that `cases/ledger.md` supersedes it.
8. **Evidence from CORD.** May the retired tool's recorded observations (`R2_cord_v701.md`) be cited in the paper as prior observations, or must every observation be redone with the new tool?

---

## 9. References and verification status

Tags:
- **V-me**: verified by me in this report through WebSearch (bibliographic data and abstract-level content).
- **V-04**: verified in `04_lit_dynamic.md` (V-abs or V-doc); not re-verified here.
- **UNVERIFIED**: used only as a pointer.

| Key | Reference | Status |
|---|---|---|
| RaceMob13 | B. Kasikci, C. Zamfir, G. Candea. RaceMob: Crowdsourced Data Race Detection. SOSP 2013, pp. 406–422. | V-me |
| RELAY07 | J. W. Voung, R. Jhala, S. Lerner. RELAY: Static Race Detection on Millions of Lines of Code. ESEC/FSE 2007, pp. 205–214. | V-me |
| Locksmith06 | P. Pratikakis, J. S. Foster, M. Hicks. LOCKSMITH: Context-Sensitive Correlation Analysis for Race Detection. PLDI 2006, pp. 320–331. Journal version: Locksmith: Practical Static Race Detection for C, ACM TOPLAS 2011. | V-me |
| Goblint16 | V. Vojdani, K. Apinis, V. Rõtov, H. Seidl, V. Vene, R. Vogler. Static Race Detection for Device Drivers: The Goblint Approach. ASE 2016, pp. 391–402, DOI 10.1145/2970276.2970337. | V-me |
| SVF16 | Y. Sui, J. Xue. SVF: Interprocedural Static Value-Flow Analysis in LLVM. CC 2016, pp. 265–266. | V-me |
| Havelund01 | K. Havelund, M. Lowry, J. Penix. Formal Analysis of a Space-Craft Controller Using SPIN. IEEE TSE 27(8):749–765, 2001, DOI 10.1109/32.940728. | V-me |
| SDV06 | T. Ball et al. Thorough Static Analysis of Device Drivers. EuroSys 2006, pp. 73–85. | V-me (full author list not checked) |
| FlowDroid14 | S. Arzt et al. FlowDroid: Precise Context, Flow, Field, Object-sensitive and Lifecycle-aware Taint Analysis for Android Apps. PLDI 2014, pp. 259–269, DOI 10.1145/2594291.2594299. | V-me |
| Engler01 | D. Engler, D. Y. Chen, S. Hallem, A. Chou, B. Chelf. Bugs as Deviant Behavior: A General Approach to Inferring Errors in Systems Code. SOSP 2001. | V-me |
| Typestate86 | R. E. Strom, S. Yemini. Typestate: A Programming Language Concept for Enhancing Software Reliability. IEEE TSE 12(1):157–171, 1986. | V-me |
| MLIR21 | C. Lattner, M. Amini, U. Bondhugula, A. Cohen, A. Davis, J. Pienaar, R. Riddle, T. Shpeisman, N. Vasilache, O. Zinenko. MLIR: Scaling Compiler Infrastructure for Domain Specific Computation. CGO 2021. | V-me (author list differs between sources; core authors confirmed) |
| RacePro11 | O. Laadan, N. Viennot, C.-C. Tsai, C. Blinn, J. Yang, J. Nieh. Pervasive Detection of Process Races in Deployed Systems. SOSP 2011. | V-04; also roadmap §13.2 [P1] |
| DPOR05 | C. Flanagan, P. Godefroid. Dynamic Partial-Order Reduction for Model Checking Software. POPL 2005. | V-04 |
| PCT10 | S. Burckhardt, P. Kothari, M. Musuvathi, S. Nagarakatte. A Randomized Scheduler with Probabilistic Guarantees of Finding Bugs. ASPLOS 2010. | V-04 |
| NetzerMiller92 | R. H. B. Netzer, B. P. Miller. What Are Race Conditions? ACM LOPLAS 1(1), 1992. | V-04 |
| AdHoc10 | W. Xiong et al. Ad Hoc Synchronization Considered Harmful. OSDI 2010. | V-04 |
| RVPredict14, GPredict15 | J. Huang et al., PLDI 2014; J. Huang, Q. Luo, G. Roșu, ICSE 2015. | V-04 |
| DroidRacer14, CAFA14, DCatch17 | PLDI 2014; PLDI 2014; ASPLOS 2017. | V-04 (DCatch: P) |
| CloudRaid18, Snowboard21 | ESEC/FSE 2018; SOSP 2021. | V-04 |
| CTrigger09, RaceFuzzer08 | ASPLOS 2009; PLDI 2008. | V-04 |
| KCSANdoc | Linux `Documentation/dev-tools/kcsan.rst`. | V-04 (V-doc) |
| MLIR SMT dialect in MLIR 18.1.3 | v0.1 [R9] links the docs page. Whether it ships in the installable 18.1.3 is unknown. | UNVERIFIED |
| ClangIR (CIR) status as a C→MLIR frontend | — | UNVERIFIED (web budget exhausted) |
| LLVM whole-program alias analyses in LLVM 18 (e.g. the status of CFL-AA) | — | UNVERIFIED |
| Context-bounded model checking (Qadeer and Rehof; Musuvathi and Qadeer); CBMC; Lazy-CSeq; Nidhugg; GenMC; Soufflé; CodeQL/QL | — | UNVERIFIED; pointers only |

Code and record citations are inline. The Sources consulted for V-me entries were:
- [EPFL DSLab RaceMob page](https://dslab.epfl.ch/research/racemob)
- [DBLP Kasikci](https://dblp.org/pid/44/6654)
- [RELAY (Lerner)](https://www.cs.cornell.edu/~lerner/papers/relay.html)
- [Locksmith (UMD)](https://www.cs.umd.edu/~mwh/papers/pratikakis06locksmith.html)
- [Goblint ASE 2016 (TUM portal)](https://portal.fis.tum.de/en/publications/static-race-detection-for-device-drivers-the-goblint-approach/)
- [SVF (UTS)](https://opus.lib.uts.edu.au/handle/10453/145627)
- [Havelund et al. (Utah TSE index)](https://ftp.math.utah.edu/pub/tex/bib/idx/ieeetranssoftweng2000/27/8/749_765.html)
- [SDV EuroSys 2006 (Microsoft Research)](https://www.microsoft.com/en-us/research/?p=154676)
- [FlowDroid (Paderborn)](https://ris.uni-paderborn.de/record/5189)
- [Engler et al. SOSP 2001](https://www.sosp.org/2001/papers/yu-chen.pdf)
- [Typestate (Frama-C page)](https://frama-c.com/fc-plugins/typestates.html)
- [MLIR CGO 2021 (Google Research)](https://research.google/pubs/mlir-scaling-compiler-infrastructure-for-domain-specific-computation/)

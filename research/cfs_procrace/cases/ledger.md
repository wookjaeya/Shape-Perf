# Case ledger: C1, C2, C3, C5, R2

- Record date: 2026-10-08. Policy: `../CONDITIONS_POLICY.md`.
- **Scope.** This ledger summarizes and cross-references the five case records. It adds **no new facts**. Every fact comes from a case file, and its section is cited as `C1 §4`, `R2 §6 S7` and so on. Permalinks, line pins and sha256 values are in the case files.
- **Labels.**
  - **Fact**: stated as fact in a case file.
  - **Inference**: labeled as inference in a case file, or a cross-case combination made here. Combinations made here are marked "(ledger inference)".
  - **Assumption**: something a derived experiment would have to choose. The case files list these as `A1`, `A2`, … and no value is chosen here.
- **Inputs.** The files as read for this ledger (mtime in UTC on 2026-10-08, first 12 hex of sha256). If any of them has changed since, re-check the affected rows.

  | File | mtime | sha256 | File | mtime | sha256 |
  |---|---|---|---|---|---|
  | C1.md | 03:29 | ff5cad99fb88 | C1.yaml | 03:29 | 26ea286161a5 |
  | C2.md | 03:14 | 4fef459c7d50 | C2.yaml | 03:17 | 5bf10328841d |
  | C3.md | 03:32 | dff850fb120f | C3.yaml | 03:32 | 4b89c0dad04b |
  | C5.md | 03:42 | f2ba1bea67cb | C5.yaml | 03:41 | cb1b48aad712 |
  | R2.md | 03:44 | 491537db8a8a | R2.yaml | 03:46 | 77d5268088bd |

  I spot-checked the YAML fields used here (revisions, platform, fix, reproduction, assumptions, decisions) against the `.md` files and found no disagreement.

**Revision shorthands.** cFE SHAs are nasa/cFE and CF SHAs are nasa/CF; `@b051eb0` is nasa/bp.

| Shorthand | Commit | What it is |
|---|---|---|
| **B** | `fc8c48ad` | cFE `main` on 2025-08-06, R2's candidate buggy revision (inference) |
| **M** | `12cb84fc` | cFE `main` HEAD, 2026-06-25 |
| **D** | `f22e538c` | cFE `dev` HEAD, 2026-10-05 |
| `SF-6.4.x` | — | official SourceForge cFE tarballs. 6.4.1: sha256 `ec26e33b…`. 6.4.2: sha256 `c64ed2aa…`. The same hashes appear in C2, C3 and C5. |

---

## 1. Cross-case table

This is one table, split in two for width. 1a covers what and where; 1b covers the fix, the revisions and reproduction.

### 1a. Resource, sites, synchronization, required order

| Case | Resource | Publisher task / site | User task / site | Existing sync at the buggy revision | Required order + provenance |
|---|---|---|---|---|---|
| **C1** CF #184 | An OSAL counting semaphore, found by the name in CF table field `chan[i].sem_name` (C1 §4). | **Provider app.** Original: unidentified ("CI/TO or some other dedicated I/O app"). Public: nasa/bp `BP_AppMain` → `AppInit` → `BP_FlowInit` → `initialize_throttling` → `OS_CountSemCreate` (bp_flow.c L121-L144 @b051eb0), before the first `RunLoop`. At OSAL level, the name becomes usable after `OS_ObjectIdFinalizeNew` (inference) (C1 §4). | **CF main task.** `CF_AppMain` → `CF_Init` → `CF_CFDP_InitEngine` → `OS_CountSemGetIdByName` (cf_cfdp.c L1012-L1023 @2a024d8). One attempt; on failure `CF_EID_ERR_INIT_SEM` and an `APP_ERROR` exit. Other uses: re-bind via `CF_CmdEnableEngine`; take in `CF_CFDP_MsgOutGet` (C1 §5). | **Nothing orders create before lookup** (C1 §6). Neither CF nor BP calls `WaitForStartupSync`. ES script order fixes only when each task starts. ES startup states bind only callers of `WaitForSystemState`. Priorities are not guaranteed to take effect (permissive mode, no RT). The OSAL table lock makes each call atomic but does not order them. | Creation of `sem_name` has returned before CF's lookup, for each channel with a non-empty name. After the fix: before the last of up to 25 lookups. Provenance: #184 body; #178; CF guide @2a024d8 L48-L57; code @2a024d8; fix comment @833fdbb. The current docs contradict the code and are not used (C1 §7). |
| **C2** cFE #198 | Readiness of the late-init app P after its "late" init (issue sentence 4). What that init sets up is private (C2 §4). | **P** ("one app", EVA CWS, private). The end of P's late init; location unknown. At the buggy revision cFE exposes only RUNNING, which P's own `WaitForStartupSync` sets **before** late init. ES main aggregates RUNNING into OPERATIONAL (cfe_es_start.c L281-L291 @b2765d9f) (C2 §4). | **R** ("other applications", private): they "were calling other functions" (sentence 7). R is released from `WaitForStartupSync` when `SystemState ≥ OPERATIONAL` (cfe_es_start.c L1017-L1019 @b2765d9f). That R's calls targeted P is inference (C2 §5). | A single-state startup sync: `WaitForStartupSync` + `ApplicationSyncDelay` → OPERATIONAL. **Both P and R used it** (from-reference). Other mechanisms: soft timeouts; the ES shared-data mutex (bookkeeping only); creation order; priorities (C2 §6). | P's late init completes before R's calls (sentences 4, 5b-5c, 7). The 6.6.0 state definitions corroborate it. "R called P" is inference (C2 §7). |
| **C3** cFE #73 | Two separate fields: the SB AppId `CFE_SB.AppId` (R-SB) and EVS's own `EVS_AppID` (R-EVSID). Also the EVS registration flags (R-EVSREG) (C3 §4). | **SB task:** `CFE_SB_AppInit` → `CFE_ES_GetAppID(&CFE_SB.AppId)` (SF-6.4.1 sb/cfe_sb_task.c L198); EVS registration at L257. **EVS task:** `CFE_EVS_TaskInit` → `EVS_GetAppID` (L385); registration at L393 (C3 §4). | **TIME task.** `CFE_TIME_TaskInit` → `CFE_SB_CreatePipe` (L308) → `CFE_EVS_SendEventWithAppID(CFE_SB.AppId = 0)` → `EVS_NotRegistered` → `EVS_SendEvent` → `EVS_IsFiltered(0xFFFFFFFF)`, with no bounds check (evs/cfe_evs_utils.c L244) (C3 §5). | Object-table creation order (EVS, SB, ES, TIME, TBL) orders creation, not execution. **The priorities cause the bad order:** TIME 60 outranks EVS 61 and SB 64. The ES lock across create + TaskTable protects R-ES only. The 6.4.1 startup sync covers script apps only; core tasks do not call it (C3 §6). | Each core app reaches RunLoop before the next starts (issue list 1 item 1; 6.4.2 VDD Table 1.2-2 row 1); this is a sufficient condition. Fine-grained form (inference): SB L198 + L257 and EVS L385 + L393 happen before TIME's CreatePipe event (C3 §7). |
| **C5** cFE #72 | The ES task-registration entry `CFE_ES_Global.TaskTable[TaskId]` (`RecordUsed`, `AppId`), found through `OS_TaskGetId()` (C5 §4.1). | **Creating ES context:** `CFE_ES_Main` for script apps, or the `CFE_ES` task for Start/Restart/Reload. In `CFE_ES_AppCreate` it writes SF-6.4.1 cfe_es_apps.c L649-L655 after re-locking at L637, and releases at L673. The task is runnable from `OS_TaskCreate` (L614) on (C5 §4.2). | **New app main task.** `CFE_ES_GetAppID` (cfe_es_api.c L846-L871) returns `CFE_ES_ERR_APPID` if the entry is not `RecordUsed`. The same applies to lock-protected users of `GetAppIDInternal` (RunLoop, WaitForStartupSync, …). Example: sample_app's `CFE_EVS_Register` and `CFE_SB_CreatePipe` (C5 §5). | The ES mutex is released at L610, before `OS_TaskCreate`, and re-taken at L637. The OSAL task-table mutex protects OSAL's own table only. Startup sync does not order a task against its own registration (C5 §6). | The registration is complete and released before the task's first GetAppID. **Violation** = GetAppID fails for a task that is about to be registered; merely running early is not a violation. Provenance: #72 body; code; 6.4.2 VDD (Trac #41); `0749d692` design comment (C5 §7). |
| **R2** cFE #2663 | The SB AppId `CFE_SB_Global.AppId`, the event-source ID of SB API events (R2 §4). | **SB task:** `CFE_SB_AppInit` → `CFE_ES_GetAppID(&CFE_SB_Global.AppId)` (B cfe_sb_task.c L126; M L138; D L128). The return value is ignored. EVS registration at L193 (R2 §4). | **EVS task (B); ES and EVS tasks (M, D).** In `*_TaskInit`: `CFE_SB_CreatePipe`/`Subscribe` and event transmit (`SEND_NO_SUBS`) → `CFE_EVS_SendEventWithAppID` → `EVS_GetAppDataByID` returns NULL → `ILLEGAL_APP_ID` (B cfe_evs.c L182-L186). The status is discarded (R2 §5). | S1 serialized core startup: an order guarantee, but it forces EVS (and ES) init **before** SB. S3 creation order. S4 priorities do not matter under S1. S5 EarlyInit zeroing. S6 EVS validity check (a guard). S7 task-function poll (a delay). No lock orders the store against the reads (R2 §6). | SB AppId stored (and SB registered with EVS) before any task runs an SB API path that emits an event. Provenance: `cfe_evs.h` L165-L172 API note; code; the issue (hedged); [R2-analysis] snippets (code reading, not execution) (R2 §7). |

### 1b. Fix, revisions, platform, reproduction

| Case | Fix (kind) | Buggy / fixed / current revisions | Platform | Reproduction kind | Conditions still `unspecified_by_reference` (summary; full list in §4) |
|---|---|---|---|---|---|
| **C1** | **Retry**, bounded: 25 × `OS_TaskDelay(100)`, on `OS_ERR_NAME_NOT_FOUND` only. Commit 833fdbb (PR #342), merged as 2ffbecc on 2022-12-01. Not an order guarantee (C1 §8). | **Buggy:** CF 2a024d8 (reported), or 281a941 (fix base; 32a8b03 has identical code). **Fixed:** 833fdbb. **Current:** CF dev 35f408d; main = v7.0.1 15a871e. Retry still present, no sync, default names `""`, so the path is latent (C1 §3, §9). | **Original:** Ubuntu 21.10, core revisions unknown. **Fix test:** Ubuntu 22.04. **Public BP CI:** `ubuntu-22.04`, native, tx/rx targets, mutable PSP patch branch (C1 §3). | public_code_derived | Original provider, names, channel count, core revisions, build configuration, startup order and priorities; CPU count; RT enforcement; provider delay amount; BP CI component SHAs; CF startup entry for 2a024d8; failure output; frequency. |
| **C2** | **Order guarantee**: new system state `APPS_INIT`, app state `LATE_INIT`, `CFE_ES_WaitForSystemState`, two-phase ES barrier. Opt-in, with soft timeouts. Shipped in v6.6.0a. Trac commits 465a958/b227d57 are not public. #198 = Trac #167 is inference (C2 §8). | **Buggy:** 6.4.2 tarball (named by the issue), or v6.5.0a b2765d9f (sync logic equivalent; inference). **Fixed:** v6.6.0a 8811c920 (first public commit 2661d19f). **Current:** D. Same design; `WaitForStartupSync` still returns void (C2 §3, §9). | **EVA CWS:** nothing stated. Only per-revision documented pairings exist: 6.4.2 with OSAL 4.1.1 / PSP 1.2.0.0; 6.5.0a with OSAL 7139592f + PSP 1.3.0.0; 6.6.0 with OSAL 4.2.1 (C2 §3). | role_derived (original unavailable) | Everything about EVA CWS: apps, count, order, priorities; late-init content and duration; R's calls, their mechanism and target; cFE version; platform; timeout arguments and configuration; reset; frequency. |
| **C3** | **Three parts in 6.4.2** (C3 §8): **F3** order guarantee (Trac #40: ES waits for each core task; panic on timeout); **F1** guard (range check before `EVS_IsFiltered`); **F2** publish-last (`EVS_AppID` set last). No public commit; the Trac IDs are 2633a3d, 0c00114 and e08eada. | **Buggy:** 6.4.1 tarball. **Fixed:** 6.4.2 tarball; first git revision with the fix is 792f5e35 (tag v6.5.0a = b2765d9f). **Current:** D. F1–F3 present in rewritten form; crash path absent; residual SB-AppId path present for ES and EVS (C3 §3, §9). | **Original:** Xilinx Microblaze (EVA team, GRC); OS, PSP, OSAL and toolchain unknown. **Derived, from the 6.4.1 references** (a deviation from Microblaze): 32-bit x86 Linux, `-m32 -g -O0`, posix/pc-linux, `cpu1`, power-on (PO) reset, root needed for RT (C3 §10). | public_code_derived (original unavailable) | EVA's cFE version, OS/PSP/OSAL/compiler, word size, the priorities actually used, creator priority; CPU count; `MAX_APPLICATIONS`; fault site and logs; reset; frequency; startup script; whether `AppData[-1]` faults. |
| **C5** | **6.4.2:** order guarantee by **lock-scope extension**: one critical section covers `OS_TaskCreate` and the registration. **Current code:** a **guard**. Since 0749d692/58abd38d the entry wrapper waits for the task record, with a timeout (C5 §8, §9). | **Buggy:** 6.4.0 / 6.4.1 tarballs (`cfe_es_apps.c` identical in both). **Fixed:** 6.4.2. No git revision contains the buggy code. **Current:** M and D. Original pattern absent; possible residual window (inference) (C5 §3, §9). | **Observation:** not stated; 6.4.1 is the build in which the "series" was found. **Derived:** pc-linux 32-bit (`-m32`), OSAL 4.1.1 SourceForge tarball (`8f0f2b23…`), PSP 1.2.0.0, one of the listed build hosts, PO reset (C5 §10). | public_code_derived | Observer's OSAL, target and host; app creation path; victim app; priorities; root/RT; CPU count; downstream effects; log and frequency; watchdog context (unread comment); timing control; handling of the existing `osal/` directory; `OS_TaskGetId` ordering. |
| **R2** | **None**; the issue is open. PR #2820 (a use-site guard) was closed unmerged as "vacuous" (R2 §8). | **Buggy:** B (inference: `main` on 2025-08-06; decision D1). **Fixed:** none. **Current:** M and D. Present; ES and EVS precede SB since b9bec567 (R2 §3, §9). | Ubuntu 22.04.5 LTS, `SIMULATION=native`, gdb. The PSP is only a candidate (pc-linux) (R2 §3). | public_code_derived (B is inference) | cFE/OSAL/PSP commits; PSP; bundle setup; BUILDTYPE/ARCH/O; OMIT_DEPRECATED; unit tests; platform configuration; creation order in the reporter's run; compiler/gdb versions; privileges; CPU count; startup script; gdb mode and hit count; binary options; number of runs. |

No case has an `original` reproduction. Every case lacks the reporter's exact platform. Every case except C2 is missing some comment text: C2's issue has no comments, but the original Trac record is unreachable (§4.4).

---

## 2. Per case: what is established, what is open

**C1 (CF #184, throttle semaphore).**
- *Established:*
  - The issue body is verbatim.
  - At 2a024d8, CF does one name lookup and exits with `APP_ERROR` on failure.
  - At the buggy revision the default table has empty names, so the lookup never runs in a default build.
  - The fix, 833fdbb, is a bounded retry: 25 × 100 ms, on `NAME_NOT_FOUND` only. It is unchanged in CF dev 35f408d and v7.0.1 15a871e, still with no startup sync.
  - The public nasa/bp integration (b051eb0) supplies:
    - the names `BP0_tsem`/`BP1_tsem`;
    - CF started before BP;
    - priorities 65/55;
    - a full CI build recipe.
  - Neither to_lab nor ci_lab ever creates a counting semaphore.
- *Open:*
  - The five comments of #184, including [C1-BP], are unread. The roadmap's "BP started first" claim is therefore unverified; the public CI starts CF first.
  - The reporter's core revisions, provider and names.
  - The BP CI component SHAs (mutable refs), and whether BP CI failed without the patch.
  - The OSAL RESERVED-window hypothesis. It is code reading only: possibly `OS_ERR_INCORRECT_OBJ_STATE` and a count-sem table lock left held.
  - The core pairing (A1).
  - The 2a024d8 path does not fit the BP CI tables: the table layout and the app name differ.

**C2 (cFE #198, late-init readiness).**
- *Established:*
  - The issue body, as WebFetch fragments.
  - The single-OPERATIONAL barrier in 6.4.2 and v6.5.0a.
  - The 6.6.0a fix (`APPS_INIT`, `LATE_INIT`, `WaitForSystemState`, two-phase barrier). It is unchanged in D.
  - The fix is opt-in. Its timeouts are soft and invisible to callers of `WaitForStartupSync`. #1466 and #1467 are still open.
  - No in-tree app demonstrates the `APPS_INIT` pattern.
- *Open:*
  - #198 = Trac #167 is inference (20 title pairs with a +31 offset).
  - The Trac commits are not public.
  - Everything about EVA CWS: apps, functions, version, platform.
  - Whether R targeted P.
  - The R→P mechanism (D4).
  - Only a role-derived experiment is possible.

**C3 (cFE #73, core-app startup crash).**
- *Established:*
  - #73 = Trac #42. This is a fact, from the migrated header comment.
  - The whole reported call chain exists verbatim in the 6.4.1 tarball, along with the creation order and the priorities.
  - The three-part 6.4.2 fix (F1–F3), with its Trac commit IDs (not public objects).
  - F1–F3 are present in rewritten form in D, and the crash path is gone.
  - A residual "SB AppId read before SB publishes it" path survives. It is version-dependent: EVS only in 6.4.2 and at B; ES and EVS since b9bec567.
  - A reference-following derived platform (6.4.1 pc-linux, `-m32 -g -O0`, `cpu1`, PO) is fixed by the 6.4.1 files.
- *Open:*
  - The full text of the 14 comments; 4 of them are unidentified.
  - The Microblaze OS, PSP, OSAL and toolchain, and the cFE build EVA actually ran.
  - Where the segfault occurred, and whether `AppData[-1]` faults under `-m32`.
  - How to produce the order: priority with root plus a single CPU, or an event-based hold (D3).
  - The primary outcome (D4).
  - The scope of the residual path (D5).

**C5 (cFE #72, AppCreate unlock-relock).**
- *Established:*
  - The bug is in 6.4.0 and 6.4.1 (byte-identical file): the lock is released before `OS_TaskCreate`, and the registration happens only after re-locking.
  - 6.4.2 fixes it by extending the lock scope. The 6.4.2 VDD describes the fix as Trac #41.
  - No nasa/cFE git revision contains the buggy code.
  - Current code was redesigned (0749d692, then 58abd38d) with a wait guard.
  - The OSAL 4.1.1 SourceForge artifact is identified by sha256. The git tag differs only in `osapi-version.h`.
  - The readme build procedure is recorded.
- *Open:*
  - The nine comments, including [C5-fix] and [C5-context], are unread, so the "watchdog" context is unverified.
  - The observer's platform, app, creation path and priorities.
  - The OSAL 4.1.1 `OS_TaskGetId` ordering, which decides between "fails" and "wrong AppID" (A8).
  - The possible residual window in M and D (inference; D4).
  - The build host distribution (D3a).
  - How to place OSAL given the existing `osal/` directory.

**R2 (cFE #2663, SB AppId before publication).**
- *Established:*
  - The issue body fragments: Ubuntu 22.04.5, native build, gdb stop at `main#L185`.
  - At B, the `ILLEGAL_APP_ID` derivation from the undefined SB AppId.
  - S1 serializes startup, so code predicts the hit on every boot (deterministic). This is not executed.
  - No fix: PR #2820 was closed as vacuous.
  - The path is present in M and D, affecting ES and EVS since b9bec567.
  - The two comments are mostly readable as search snippets.
- *Open:*
  - B is an inference (D1).
  - At B, code predicts EVS only, but the report says "the ES and the EVS tasks" (Q1).
  - Race or deterministic needs execution (D2, Q3). The predicted hit counts are lower bounds: at least 4 at B, at least 9 at M/D.
  - The reporter's environment (Q4).
  - The elided snippet spans and the comment timestamps (D3).
  - The S7 variant (D5).
  - The intended fix semantics (Q5).

---

## 3. Relations between cases

### 3.1 C3 ↔ R2: SB AppId readiness

**Shared (facts from C3 §4–§9 and R2 §4–§9):**
- **The resource family.** It is the SB task's own AppId global. In 6.4.1 it is the file-scope `CFE_SB.AppId`, whose static zero holds until init. At B, M and D it is `CFE_SB_Global.AppId`, zeroed by `CFE_SB_EarlyInit`.
- **The publication site.** `CFE_SB_AppInit` → `CFE_ES_GetAppID(&…AppId)`: 641 L198; B L126; M L138; D L128.
- **The use pattern.** SB API calls (`CFE_SB_CreatePipe`, `CFE_SB_Subscribe`) run in the **caller's** task and emit events with SB's AppId through `CFE_EVS_SendEventWithAppID`.
- **The second readiness step.** SB's own registration with EVS: 641 L257; B L193.
- **The default core priorities.** EVS 61, SB 64, ES 68, TIME 60, TBL 70, the same in 6.4.1 `cpu1` and in B's module defaults (C3 §10 row 9; R2 §6 S4). C3 §9 records the same default values in D's `example_platform_cfg.h`.
- **The fix lineage.**
  - C3's F3 (6.4.2 per-core-task startup barrier) is the ancestor of R2's S1. Both the C3 §9 "F3 present, rewritten" entry and the R2 §6 S1 entry cite `CFE_ES_MainTaskSyncDelay` + `CFE_PSP_Panic`.
  - C3's F1 range check is the ancestor of R2's S6. R2 S6 says to "compare C3's 6.4.2 range check".
  - R2 §8 lists C3 as "earlier, related fix lineage".
- **The open question.** C3 D5 and R2 Q7 ask the same thing: count the two as one "SB AppId before publication" pattern or not. Roadmap §9.3, as quoted in R2 Q7, says they are "not treated as fully independent generalization cases". Both files label the C3–R2 link as **inference**.

**Not shared (facts):**
- **The failing caller.** C3: TIME, at 6.4.1. R2: EVS at B; ES and EVS at M and D. C3's own residual after the fix is EVS (C3 §8.3).
- **What decides the order.**
  - C3, at 6.4.1, has no core serialization. TIME's priority (60) lets it run before EVS and SB; this is scheduling-dependent.
  - R2's S1 serialization plus table order forces EVS (and ES) init before SB. Code predicts this is deterministic (R2 §6.1, not executed).
- **The value read.**
  - In C3, SB AppId 0 is a **valid** AppId, EVS's own slot, so the path goes to `EVS_NotRegistered`.
  - In R2, 0 is `CFE_ES_APPID_UNDEFINED`, an **invalid** resource ID (since 96d82c53, 2020-09-22), so the result is `ILLEGAL_APP_ID`.
- **What causes harm.**
  - C3's crash comes from a **second resource**: EVS's own `EVS_AppID = 0xFFFFFFFF`, indexed unchecked in `EVS_IsFiltered`.
  - R2 has no crash. The status is discarded and events are lost, including INFO `SEND_NO_SUBS` events, which are not masked by default (R2 §5).
- **The outcome of the same residual call in 6.4.2.** C3 §8.3 records it as misattributed to EVS (AppId 0), with the DEBUG event dropped. That is not `ILLEGAL_APP_ID`.
- **Revisions and evidence.**
  - The revisions do not overlap: 6.4.1/6.4.2 tarballs for C3; git B, M and D for R2.
  - C3's report is a Microblaze segfault. R2's is a gdb breakpoint stop on native Ubuntu.
- **How a reproduction gets the order.**
  - C3 needs order control, either priority (root plus a single CPU) or an event hold (C3 D3).
  - R2's default configuration is predicted to hit without any control. R2 A10 says a reordered "publish-first" control is a constructed configuration and is not evidence of a race (roadmap §6.4).

**Ledger inference.** C3's F3 turned the scheduling-dependent misorder into a fixed creation-order dependency. For tasks created before SB, the "use before publication" became forced instead of possible. C3 §8.3 and R2 §6.1 both state the pieces of this. If the paper counts C3 and R2 as one pattern, the counted unit is the SB-AppId publication point, observed in two eras with different ordering causes and different consequences.

### 3.2 C3 ↔ C5: the 6.4.1 startup races and the ES TaskTable

- **Shared (facts).**
  - **Revisions and release documents.** The same buggy and fixed tarballs (6.4.1 `ec26e33b…`, 6.4.2 `c64ed2aa…`). The same 6.4.2 VDD sentence ("a series of startup race conditions that were found in cFE build 6.4.1"). The same Trac → GitHub offset: #41 → #72 and #42 → #73.
  - **The derived pc-linux platform**, built from the 6.4.1 readme and PSP files:
    - `-m32`, `posix`/`pc-linux`, `cpu1`;
    - power-on reset when no `-R`/`--reset` argument is given;
    - root needed for RT priorities;
    - `msg_max` or root needed for message queues.

    See C3 §10 rows 21, 27–29 and C5 §10 rows 6, 10, 17–21.
  - **Siblings.** Both are siblings of #71 (Trac #40). #71 and #73 name the GRC EVA Microblaze; #72 names no platform (C5 F12).
- **Not shared (facts).**
  - **The resource.** C3's ES resource (R-ES, the TaskTable link for **core** tasks) is protected: `CFE_ES_CreateObjects` holds the lock across `OS_TaskCreate` and the TaskTable fill (C3 §6). C5 F7 confirms the issue's claim that this path is safe. C5's window is in `CFE_ES_AppCreate`, which serves **script, command, restart and reload apps** (C5 F8). C3's core tasks are therefore not exposed to C5's window, and C5 does not involve SB or EVS AppIds.
  - **The kind of trigger.** C3 is a multi-task order across three core tasks. C5 is creator versus a newly created task.
- **Cross-file discrepancy (needs reconciliation; §5 X3).**
  - C3 fetched OSAL from the git tag `osal-4.1.1` (`2f1fdb0c`) and recorded its identity with the SourceForge tarball as "unverified" (C3 §11 A2).
  - C5 later established the difference. The git tag differs from SourceForge `osal-4.1.1-release.tar.gz` (sha256 `8f0f2b23…`) only in `osapi-version.h`: the tag reports 4.1.0. C5 records building from the tag as a `deviation` (C5 §3, §11 A2).
  - The two records should use the same OSAL artifact.
- **Cross-file framing difference.**
  - C3 §10 row 29 records "root" as from-reference for RT scheduling on the derived platform.
  - C5 §10 row 10 records root/SCHED_FIFO as `unspecified_by_reference` for the observation, with two reference-sanctioned options (raise `msg_max`, or run as root).
  - Both quote the same readme lines. They differ on whether "use RT" is a value or a choice.
  - Ledger inference: one shared row would remove the ambiguity: "RT needs root (from-reference); whether the derived run uses RT is a decision". See §5 X3.

### 3.3 C2 ↔ C3 ↔ C5: the startup-sync lineage across 6.4.1 → 6.4.2 → 6.6.0

- **Facts.**
  - The 6.4.1 → 6.4.2 diff contains three tickets' changes at once:
    - Trac #40, the polling `WaitForStartupSync` with core apps calling it. This is C3's F3.
    - Trac #41, C5's lock-scope fix, plus `++AppStartedCount` moved before task creation.
    - Trac #42, C3's F1 and F2.

    See C3 §3 (changed-file list) and §8.2, and C5 F9.
  - C2's **buggy** side is the 6.4.2 single-OPERATIONAL startup sync: the same polling machinery, the same `AppStartedCount`/`AppReadyCount` counters (C2 §1.2).
  - C2 §1.4 itself names #72 and #73 as "the other startup-race tickets fixed in 6.4.2 … cases C5 and C3".
  - C5 F9 calls the sync rewrite "the C2 subject". Precisely, it is C2's buggy-side baseline and C3's F3.
  - C2's **fix** (6.6.0: `WaitForSystemState`, two-phase barrier) is the API that current core tasks use for F3: `WaitForSystemState(CORE_READY)` (C3 §9).
- **Ledger inference.**
  - The 6.4.2 tarball plays two roles: it is the **fixed** control for C3 and C5 and a **buggy** candidate for C2. One 6.4.2 build could serve both roles, if D2 in C2/C3/C5 is decided consistently.
  - A plain 6.4.1-vs-6.4.2 comparison changes all three tickets together. Isolating one fix needs constructed subsets of the diff. C3 A8 already labels these as constructed ablations. C5 would need the same label if it isolates Trac #41.

### 3.4 C5 ↔ R2: task record versus app record in current code (both inference, unverified)

- **Shared (facts from code reading).**
  - Since `58abd38d`, the new task's `CFE_ES_GetTaskFunction` waits only until its **task** record is defined.
  - `CFE_ES_GetAppID` needs the **app** record to have left `RESERVED`. The creator does that later, in a separate step (`CFE_ES_AppRecordSetUsed`).
- **Two creation paths with the same structure.**
  - C5 §9: the `CFE_ES_AppCreate` path for external apps (main L722 → L861/L869).
  - R2 §6 S7: the `CFE_ES_CreateObjects` path for core tasks at B (`cfe_es_start.c` L767-L780). R2 D5 itself says S7 "is related to C5 (#72)".
- **Not shared.** The consequence if the window is hit:
  - C5: one app's `GetAppID` returns `CFE_ES_ERR_RESOURCEID_NOT_VALID`.
  - R2: `CFE_SB_AppInit` ignores the failure, so SB's AppId would stay `UNDEFINED` for the whole run and every later SB event would get `ILLEGAL_APP_ID`.
- **Shared need.** A derived probe needs an event-based hold of the creator between task-record publication and app-record finalization (C5 A9, R2 A9). Neither file reports an execution or an upstream report.

### 3.5 C1 ↔ C2: app-level readiness and `CFE_ES_WaitForStartupSync`

- **Shared (facts).**
  - In both, one application publishes something during its init and another application uses it during its init. Both are app-to-app, not core.
  - In both, the issue text names `CFE_ES_WaitForStartupSync` in connection with the remedy:
    - #184: "Adding a call to CFE_ES_WaitForStartupSync() before starting the engine might help too…".
    - #198: P and R both already used it, and it was insufficient.
  - In both, the timeout argument of that call is `unspecified_by_reference` (C1 A10; C2 §10 row 11, A7).
  - C1 §6 states its ES facts at cFE `98f78e8`: two sequential waits, first for all apps at least `LATE_INIT`, then for all apps `RUNNING`. That is C2's fixed (6.6.0) design.
- **Not shared.**
  - **The resource.** C1: an OSAL named object, looked up by name. C2: private app-level readiness.
  - **The fix.** C1: an app-side retry. C2: a cFE API change.
  - **The repository.** CF versus cFE core.
  - **The default configuration.** C1's default configuration never executes the path (empty names, C1 §10 row 17). C2's apps are unknown.
- **Ledger inference.** The remedy #184 suggests (CF calls `WaitForStartupSync` before `InitEngine`) would inherit the gaps C2 documents:
  - `WaitForStartupSync` returns void;
  - a soft ES phase timeout releases the waiter with `CFE_SUCCESS`;
  - #1466/#1467 are open.

  C1 §6 already says that its order would hold only once OPERATIONAL is reached, "or both ES timeouts expiring".

### 3.6 C1, C5, R2: "reserve, then finalize" publication (pattern family; all hypotheses)

| Case | Layer | Reader during `RESERVED` gets | Status |
|---|---|---|---|
| C1 §4 | OSAL object table (`active_id = OS_OBJECT_ID_RESERVED` between `OS_OBJECT_INIT` and `FinalizeNew`) | `OS_ERR_INCORRECT_OBJ_STATE`; the count-sem table lock may also stay held, so the provider would hang | Code reading of five OSAL revisions; unverified (C1 A7, D4) |
| C5 §9 | cFE ES app record | `GetAppID` fails | Inference; unverified (C5 D4) |
| R2 §6 S7 | cFE ES app record, for SB | SB AppId `UNDEFINED` for the whole run | Inference; unverified (R2 D5) |

The three share only the shape: a record is visible or needed before its final ID is written. They differ in layer, in oracle (C1 needs a hang oracle) and in whether a retry could help: C1's fix retries only on `NAME_NOT_FOUND`.

### 3.7 Common evidence base and common gaps (facts)

- **Trac migration offset.** GitHub# = Trac# + 31.
  - C2 §1.3 builds the table: 20 pairs from three VDDs.
  - C3 has direct proof for #73 (the "Imported from trac issue 42" header).
  - C5 relies on the offset for #72 → Trac #41 ("strongly supported").
  - C2's #198 → Trac #167 is inference (C2 D1).
- **Trac and Wayback are unreachable** in every case that needs them (C2 §13, C3 §13, C5 §14).
- **GitHub comment text** is blocked for nasa/cFE and nasa/CF at the REST/MCP level in every case. C3 and R2 recovered partial text through logged-out global-search snippets.
- **Verbatim issue bodies.** C1 (CF #184) and C5 (cFE #72) obtained them through GitHub MCP `search_issues` (C1 §0, C5 F1; C5 §14 also lists #71 and #73 metadata through that route). C2, C3 and R2 used ≤125-character WebFetch fragments for their bodies.

---

## 4. Conditions a reproduction needs that no reference determines

Each item lists the case-file rows or assumptions where it appears. No value is chosen. Items marked "→ A…" are the case file's labeled assumption for a derived experiment. The policy requires event-based control instead of fixed delays (CONDITIONS_POLICY); where a case needs timing control, the hold point is itself unspecified.

### 4.1 Cross-case conditions (needed by two or more cases)

| ID | Condition | Cases (rows / assumptions) | What references do give |
|---|---|---|---|
| U-CPU | CPU count, affinity, preemption model | C1 §10 #23, A4; C2 #10a; C3 #14, A10; C5 #11; R2 "CPU count / affinity", A5 | None in any case. C3: a priority-driven order would need a single CPU or an equivalent restriction (inference). |
| U-RT | Whether OS real-time priorities take effect (root or not; permissive mode) | C1 #22, A4; C3 #12 (Microblaze creator), #29, D3; C5 #10; R2 "Privileges", A5 | RT on POSIX needs root or the capability; the 6.4.1 readme and OSAL 4.1.1 say root. The cFE native sample config is PERMISSIVE. Whether the observation ran with RT: nowhere. |
| U-PRIO | Priorities and stack sizes of the apps in the observation | C1 #21; C2 #3; C3 #10, #26; C5 #9 | Sample/default values only: C3/R2 core priorities; C5 sample script; C1 BP CI 65/55. |
| U-REV | Exact cFE/OSAL/PSP (and CF/bundle) revisions of the observation | C1 #5, A1; C2 #7; C3 #5; C5 #3 (scope), #4; R2 "cFE commit", "OSAL / PSP commits", A1 | Candidates only: C1 caelum-rc4 or cFS 1df262a/1b0b338; C2 6.4.2 or v6.5.0a; C3/C5 6.4.1 by the VDD "series" sentence; R2 B. |
| U-PLAT | OS, board, PSP, compiler, word size of the observation | C1 #10; C2 #10a; C3 #7, #27; C5 #5, #20; R2 "Host CPU architecture", "PSP", "Compiler / gdb versions" | Stated: C1 Ubuntu 21.10; C3 Microblaze; R2 Ubuntu 22.04.5. Nothing else. |
| U-RESET | Reset type and command-line options of the observed run | C2 #17; C3 #21 (EVA); C5 #21 (observation); R2 "gdb launch mode … `core-cpu1` command-line options" | For derived 6.4.1 pc-linux runs, PO is from-reference (C3, C5). |
| U-SCRIPT | Startup script / app set / creation path of the observation | C1 #18, #38; C2 #3, #16; C3 #23 (EVA); C5 #7, #8; R2 "Startup script / external apps" | C1 public BP script; C3/C5 6.4.1 sample script for derived runs. In R2, apps start after CORE_READY, so the script cannot reach the use sites (inference). |
| U-FREQ | Failure frequency, number of runs, logs, observed output | C1 #29, #30; C2 #18; C3 #20, #22; C5 #13; R2 "gdb mode … hit count", "Number of runs" | C1 "not readily reproducible"; C3 "segfaults"; R2 "stopped at the breakpoint". No log anywhere. |
| U-HOLD | Order/timing control: mechanism and hold point | C1 #24, A5, A6; C2 A5; C3 A3 (D3); C5 #15, A6; R2 A9 (D5 only) | The policy requires event-based control. C1's issue suggests "an artificial delay" with no amount or mechanism. |
| U-ORACLE | Oracle beyond the referenced symptom (primary observable; what counts as failure) | C1 A8; C2 #6, A4; C3 D4, A4; C5 #12, A7; R2 D2, A6 | Referenced symptoms only: C1 `CF_EID_ERR_INIT_SEM` + exit; C2 "calling other functions before…"; C3 crash; C5 GetAppID fails; R2 breakpoint at `ILLEGAL_APP_ID`. |
| U-SYNCTO | `TimeOutMilliseconds` passed to `CFE_ES_WaitForStartupSync` by apps | C1 A10 (derived control only); C2 #11, A7 | The doc's "at least 1000 … rounded up" is not enforced in code (C2 §1.2). |

### 4.2 Case-specific conditions

**C1** (C1 §10, §11):
- Provider app (#11; A2).
- Semaphore names (#15; A3) and number of channels with a semaphore (#25) in the original.
- Original build configuration (#10).
- Actual original startup order (#18; only the hint "start CF before the provider" exists).
- Provider delay amount (#24).
- BP CI component SHAs: cFS, bplib, CF branch content before 2022-11-17 (#7, #8), and the 2022 SHA of the `jphickey/PSP@techdev-iodriver` patch (#32).
- CF startup entry (name, entry point, priority, stack) for the 2a024d8 path (#38). The `CF_APP` table-name constraint is inference.
- Bound for calling the OSAL RESERVED probe a hang (A7).
- Trigger and timing of a provider restart for the secondary problem (A9).
- Observed failure output (#29).

**C2** (C2 §10, §11):
- App names, count, script order, priorities and stacks (#3; A2).
- Content and duration of P's late init (#4).
- What R called, by which mechanism, and whether the target was P (#5; A3; D4).
- An observable readiness indicator inside P (A3).
- EVA CWS values of `CFE_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`, `…SYNC_POLL_MSEC` and `CFE_CORE_MAX_STARTUP_MSEC` (#12–#14).
- Whether the apps were startup-script apps (#16).
- Whether P's late init sends requests to R (A9).
- Symptoms beyond the early call (#6).

**C3** (C3 §10, §11):
- The cFE build EVA ran (#5).
- Microblaze OS/RTOS, BSP/PSP, OSAL, compiler (#7) and word size (#27).
- Whether EVA used the default priorities (#10).
- The creator thread's priority on Microblaze (#12).
- EVA's `CFE_ES_MAX_APPLICATIONS` (#17).
- Fault address, signal and logs (#20).
- Whether `AppData[0xFFFFFFFF]` (→ `AppData[-1]` under `-m32`) faults (A4).
- EVA platform configuration (#28; A5), startup script (#23) and TIME child-task priorities (#26).

**C5** (C5 §10, §11):
- The observer's OSAL (#4), target (#5) and build host (#20).
- How the readme's `mv osal-4.1.1-release osal` resolves, given that the tarball already has `osal/` (#6).
- App creation path (#7; A5).
- Victim app and its first GetAppID-dependent call (#8; A4).
- Creator and new-task priorities (#9).
- Downstream "snowball" (#12).
- The OSAL 4.1.1 `pthread_create` vs `OS_TaskRegister` ordering, which decides between "fails" and "wrong AppID" (A8).
- Reachability of a stale `RecordUsed` entry (A8).

**R2** (R2 §10, §11):
- Host CPU architecture and virtualization.
- cFE, OSAL, PSP and bundle commits (A1).
- PSP.
- Use of the bundle README setup (A2).
- `BUILDTYPE`, `ARCH`, `O` (A3).
- `OMIT_DEPRECATED` (the outcome is independent of it per R2 §4) and `ENABLE_UNIT_TESTS`.
- Platform configuration values in effect (A2).
- Core creation order in the reporter's run.
- Compiler and gdb versions (A4).
- gdb launch mode, all-stop/non-stop, thread scope, hit count (A6).
- Run directory and binary.
- Breakpoint revision.
- Number of runs.
- Absence of a startup timeout (A7).

### 4.3 Determined only up to a referenced set (a choice, not a missing value)

These have reference-backed options. Picking one is a decision; it is not `unspecified_by_reference`.

- **C1.**
  - The CF revision: 2a024d8 versus the 281a941/833fdbb path. This choice also fixes the table layout and the app name (C1 §10 #38–#39, D2).
  - Within each path, the core pairing candidates (A1, D3; inference).
- **C2.**
  - Buggy side: 6.4.2 or v6.5.0a. Fixed side: v6.6.0a or D (D2, A1).
  - OSAL and PSP must follow the per-revision pairing (§10 #10b).
- **C3.**
  - Buggy side: 6.4.1, or v6.5.0a with the fix reverted (a constructed deviation). Fixed side: 6.4.2 or v6.5.0a (D2).
  - OSAL "4.1.1 or higher"; 4.1.1 is the tested version (#25).
  - Order control: priority or event hold (D3).
- **C5.**
  - 6.4.1 or 6.4.0, which are equivalent (D2).
  - The build host: one of the four listed hosts; any other is a deviation (D3a).
  - Raise `msg_max` or run as root (#10).
- **R2.**
  - B, or current code only (D1).
  - D or M as the current revision (D4).

### 4.4 A reference exists but was not retrieved

These conditions might be determined if the text were read.

| Case | Unread reference | What it might settle |
|---|---|---|
| C1 | 5 comments of CF #184, incl. `issuecomment-1202543669` [C1-BP] | Provider (BP version), start order ("BP started first"), names, failure log |
| C2 | Trac #167 (host unreachable); #198 REST timeline | Reporter, EVA CWS details, closing event |
| C3 | Full text of #73's 14 comments, 4 unidentified; Trac #42 | Platform details, close record, the full fix discussion |
| C5 | 9 comments of #72 incl. [C5-fix] `536673810`, [C5-context] `536673820`; Trac #41 | Reporter, platform, runtime-load/watchdog context |
| R2 | 3 elided spans and timestamps of the 2 comments | Verbatim analysis text; whether it addresses Q1 |

---

## 5. Decisions needed (consolidated across cases)

Per the user's instruction, intermediate conclusions that need a decision are collected here. Each item merges decisions already open in the case files. Nothing is chosen.

**X1. GitHub comment access for nasa/cFE and nasa/CF.**
- Merges C1 D1, C2 D3, C3 D1, C5 D1 and R2 D3.
- Options:
  - (a) Consent to a credentialed `add_repo` with `access:"push"`. The case files record that it may be refused (C1 §0).
  - (b) You paste the comment texts: CF #184 (5); cFE #72 (9), #73 (14), #2663 (2).
  - (c) Proceed without them.
- Effect:
  - (a) or (b) would settle [C1-BP], [C5-fix]/[C5-context], C3's 4 missing comments and R2's elided spans (§4.4).
  - (c) leaves the roadmap claims that rest on them unverified. These are C1's "BP를 먼저 시작해도" and C5's watchdog context.

**X2. Policy status of search-snippet text.** Raised by R2 D3's sub-decision; affects C3.
- C3 treats Trac-comment text recovered through logged-out global search as from-reference: the Trac creation date, merge date and commit IDs (C3 §2, §8.1).
- R2 treats the same kind of text as "retrieved, truncated, verify before quoting" and sets no condition from it.
- One rule is needed for both:
  - (a) snippets count as references, with their truncation recorded; or
  - (b) snippets are leads only, and C3's affected rows are downgraded until verified.

**X3. One shared 6.4.x derived platform for C3 and C5**, and C2 if 6.4.2 is chosen.
- Merges C3 D2/D3/A1/A2, C5 D2/D3a/D3b and C2 D2.
- Points to decide once:
  - **OSAL artifact.** C5's SourceForge `osal-4.1.1-release.tar.gz` (sha256 `8f0f2b23…`) is from-reference. C3's git tag build would be a deviation under C5's finding (§3.2).
  - **Build host.** One of the four hosts listed in the 6.4.1 readme, or a deviation (C5 D3a).
  - **Binary width.** Keep `-m32` (C3 A1, C5 D3b).
  - **Privileges.** Root (RT priorities and message queues) or `msg_max` only (C3 #29/D3, C5 #10).
  - **Order control for C3.** Priority-driven (root plus a single CPU) or event hold (C3 D3).

  Container feasibility of these choices is tracked in `../STATUS_20261008.md` and `../env/`, not here.

**X4. Counting C3 and R2.** Merges C3 D5 and R2 Q7, and depends on R2 Q1.
- (a) One "SB AppId before publication" pattern, observed in two eras.
- (b) Two cases, with the shared dependency disclosed.
- The roadmap (§2.2, per C3 D5) says not to rewrite R2's observation as C3's crash.

**X5. The ES "task record before app record" probe.** Merges C5 D4 and R2 D5.
- (a) Pursue it as one derived probe on both creation paths (`CFE_ES_AppCreate` for C5; `CFE_ES_CreateObjects`/SB for R2), labeled derived and kept out of the original-case results.
- (b) Exclude it.
- No upstream report exists in either file.

**X6. Current revision of record.** Merges R2 D4 and the "current" columns of C2, C3 and C5.
- C2 and C3 checked D only. C5 and R2 checked M and D. C1 checked CF dev `35f408d` and CF v7.0.1 `15a871e`, which C1 records as the CF pinned by cFS v7.0.1.
- The case files do not record the cFE commit pinned by the ENV build (cFS `v7.0.1`; see `../STATUS_20261008.md`). R2 records cFE tag `v7.0.1` = `c5fb2b4d`, which is not M (`12cb84fc` = v7.0.1-2).
- Decide whether "present in current code" must also be checked at the ENV build's cFE commit.

**X7. Oracle policy across cases.** Merges C3 D4, C5 A7, R2 D2, C1 A8 and C2 A4.
- (a) The primary outcome is the order violation observed at the use site:
  - C1: lookup before create;
  - C2: R's request before P's late-init end;
  - C3: OOB index reaching `EVS_IsFiltered`;
  - C5: GetAppID failure for an unregistered task;
  - R2: the per-boot hit set at U0.

  Crashes and other symptoms are secondary and labeled derived.
- (b) Decide per case.

Still open in a single case only; see the case files: C1 D2–D4, C2 D1/D4, R2 D1/D2. X3 also requires C2's D2 to be decided consistently with C3/C5.

---

## 6. Follow-ups that need no decision

- **Ledger inference.** C1 and C5 got verbatim issue bodies through GitHub MCP `search_issues`, including for nasa/cFE (#72, and metadata for #71 and #73). The same route might return verbatim bodies for #73, #198 and #2663. Those three bodies are currently ≤125-character WebFetch fragments (C2 §1.1, C3 §1.1, R2 §1.1). This is untested.
- **C3 §11 A2.** Update it with C5's OSAL comparison: the tag and the SourceForge tarball differ only in `osapi-version.h`.
- **C3 and C5 derived-platform rows.** Align their wording (§3.2) so the same readme lines carry the same status.

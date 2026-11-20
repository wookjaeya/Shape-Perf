# Case-to-code mapping at the ENV build (cFS v7.0.1)

- Date: 2026-10-08. Policy: `../CONDITIONS_POLICY.md`.
- Purpose: step 2 of the review's follow-up order ("사례별 원 조건과 현재 코드 연결"). For each case, this note locates the dependency events in the code that the ENV build actually runs. That code is cFS `088b2fa8` (v7.0.1), with cFE `c5fb2b4d`, CF `15a871e6` and OSAL `d2d877a6`. The note also says whether the path can execute in the default configuration.
- Status of every statement: **code reading at a pinned revision**. Nothing here is an execution result. Execution evidence for R2 is planned under `../tool/gdb/r2_trace.py`; see §1.4.
- Line numbers were read from `/home/user/work/procrace/cfs_ref/cFS` (the ENV tree). Each breakpoint line in §1.4 was also checked against the debug line table of the ENV `core-cpu1` with `gdb info line`.

## 0. Revision relation (answers decision X6)

- The ENV cFE `c5fb2b4d` is tag `v7.0.1` ("Merge pull request #2716 from nasa/dev").
- It is an ancestor of `M` = `12cb84fc`, the cFE `main` HEAD used in `R2.md`. `M` adds two commits: `d67c8c60` "Fix #2695, Updates in-range external MET state path TIME UT", and its merge.
- `git diff --stat c5fb2b4d 12cb84fc` is empty for these files:
  - `modules/sb/fsw/src/cfe_sb_task.c`
  - `modules/evs/fsw/src/cfe_evs.c`
  - `modules/es/fsw/src/cfe_es_task.c`
  - `modules/evs/fsw/src/cfe_evs_task.c`
  - `modules/es/fsw/src/cfe_es_start.c`
- **So every R2 code fact recorded for `M` in `R2.md` applies unchanged to the ENV build.** This includes the line numbers cited in the [R2-analysis] comment snippet: `cfe_sb_task.c:138`, `cfe_es_task.c:367` and `cfe_evs_task.c:289`.

## 1. R2 (cFE #2663): SB AppId used before SB publishes it

### 1.1 Events at `c5fb2b4d`

| Role | Site | Note |
|---|---|---|
| Initial value | `modules/sb/fsw/src/cfe_sb_init.c` L50 `memset(&CFE_SB_Global, 0, …)` in SB EarlyInit | `CFE_ES_AppId_t` is `unsigned int` in this build (`ptype`), so the value is 0 = `CFE_ES_APPID_UNDEFINED` (R2 §4) |
| **Publication 1** (AppId) | `modules/sb/fsw/src/cfe_sb_task.c` L138 `CFE_ES_GetAppID(&CFE_SB_Global.AppId);` in `CFE_SB_AppInit` (L130) | The return value is ignored |
| **Publication 2** (SB registered with EVS) | same file, L155 `CFE_EVS_Register(CFE_SB_Global.EventFilters, …)` | Between L138 and L155, events carrying SB's AppId take the `EVS_NotRegistered` branch |
| Use: SB API events carrying SB's AppId | `modules/sb/fsw/src/cfe_sb_api.c`. `CFE_SB_CreatePipe` success event `CFE_SB_PIPE_ADDED_EID` L275-281 (DEBUG). `CFE_SB_SubscribeFull` success event `CFE_SB_SUBSCRIPTION_RCVD_EID` L1183-1186 (DEBUG). The transmit path reports `CFE_SB_SEND_NO_SUBS_EID` through `CFE_SB_MessageTxn_ReportEvents` → `cfe_sb_priv.c` L786 `CFE_EVS_SendEventWithAppID(EventId, EventType, CFE_SB_Global.AppId, …)`; it is set at L1086 when a packet has no subscriber | All of these run in the **caller's** task |
| Use sites in ES and EVS init (run before SB) | `modules/es/fsw/src/cfe_es_task.c` L367 `CFE_SB_CreatePipe`, L377 and L387 `CFE_SB_Subscribe` (in `CFE_ES_TaskInit`, L306). `modules/evs/fsw/src/cfe_evs_task.c` L289 `CFE_SB_CreatePipe`, L297 and L304 `CFE_SB_Subscribe` (in `CFE_EVS_TaskInit`, L266) | Events that ES and EVS send during init are themselves transmitted on SB. With no subscriber yet, they can produce `SEND_NO_SUBS`, which carries SB's AppId (inference from the path above) |
| Use outcome | `modules/evs/fsw/src/cfe_evs.c`, in `CFE_EVS_SendEventWithAppID`. L185 `AppDataPtr = EVS_GetAppDataByID(AppID);`, then **L188 `Status = CFE_EVS_APP_ILLEGAL_APP_ID;`**. L193 `Status = EVS_NotRegistered(AppDataPtr, AppID);` | Callers discard the status, so the event is lost silently. The reporter's breakpoint "main#L185" was this assignment at the report date (R2 §5). At `c5fb2b4d` the assignment is **L188** |
| Ordering mechanism (S1) | `modules/es/fsw/src/cfe_es_start.c` L810 `CFE_ES_StartAppTask` and L835 `CFE_ES_MainTaskSyncDelay(CFE_ES_AppState_RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)`, once per core-object entry. The order is fixed by `modules/es/fsw/src/cfe_es_objtab.c`; the ENV console shows ES, EVS, SB, TBL, TIME | Each core task must reach RUNNING before the next one is created. ES and EVS are therefore initialized, and use SB APIs, before the SB task exists (R2 §6 S1) |

### 1.2 Executability in the default configuration

- **Yes, by code reading.** No configuration switch guards the path. The core object order is compiled in.
- The DEBUG type of `PIPE_ADDED`/`SUBSCRIPTION_RCVD` does not matter. The AppId check (L185-188) runs before any type or filter check (L195 `EVS_IsFiltered`).
- Expected hit set: every `CFE_EVS_SendEventWithAppID` call made with SB's AppId while it is still 0. At least six such calls come from the ES and EVS `CreatePipe`/`Subscribe` calls above. `SEND_NO_SUBS` events may add more. R2 §9 gives "≥ 9 at M/D" as a lower bound. **Not executed.**
- Visibility without a debugger: none found. The status is discarded. Nothing is printed. No counter is incremented on the `ILLEGAL_APP_ID` branch (L186-189). No syscall is involved.

### 1.3 Conditions for an R2 execution on the ENV build

| Condition | Value | Reference | Status |
|---|---|---|---|
| Build, run user, working directory, command | ENV E1–E3, unchanged | `../env/CONDITIONS.md` | from-reference |
| Observation method | gdb breakpoint at the `ILLEGAL_APP_ID` assignment | Issue #2663 body: gdb, breakpoint `main#L185` (R2 §1.1, §5) | from-reference. The line moved to **L188** at `c5fb2b4d` by content; see §1.1 |
| gdb version | 15.1 (Ubuntu 24.04 package) | The reporter's version is unknown (R2 §10, A4) | unspecified_by_reference (recorded) |
| gdb mode | all-stop. Each breakpoint runs a Python `stop()` that records the hit and returns False, so the program continues | The reporter's mode and hit-count handling are unknown (R2 A6) | unspecified_by_reference. **Assumption.** It perturbs timing; the effect must be reported |
| Extra observation points | `cfe_sb_task.c` L138 (before) and L142 (after) Publication 1; L156 (after Publication 2); entry of `CFE_EVS_SendEventWithAppID`; `cfe_evs.c` L193 | None. These are this study's observation points | Tool setting. Not a reference condition |
| Signals | gdb passes `SIGALRM`, `SIGUSR1/2`, `SIGPIPE`, `SIGCHLD` and `SIG34`–`SIG64` silently | None. This keeps gdb from stopping on OSAL and glibc timer signals | Tool setting |
| Run length and stop | ENV E10 `operational` (the README L114 line) and E11 (a) `ci-es-restart-poweron` | E10, E11. All R2 uses occur before CORE_READY (S1) | from-reference. The stop method for experiments is still PI decision D3 |
| Number of runs | — | The reporter's count is unknown (R2 "Number of runs") | unspecified_by_reference. To be reported as "observed in k of k runs" only; no claim of determinism |
| Reset and persisted data | Recorded before and after each run (M1 method) | Review §6 | — |

### 1.4 Planned observation (not yet run)

- Script: `../tool/gdb/r2_trace.py`. Each hit writes one JSON line: kind, LWP, thread name, values and a stack.
- Verdict per use (P4 (a), pending), with "use" meaning one `SEND_WITH_APPID` hit whose caller is an SB function:
  - **observed violation (AppId)**: before `SB_PUB`, and the outcome hit is `OUT_ILLEGAL_APP_ID`;
  - **observed violation (registration)**: after `SB_PUB` but before `SB_EVS_REGISTERED`, and the outcome hit is `OUT_NOT_REGISTERED`;
  - **ordered**: after both.
- Path-execution evidence (review §4, §10): hits at L138/L142 and at the use sites, each with its task identity taken from the stack (`CFE_ES_TaskMain`, `CFE_EVS_TaskMain`, `CFE_SB_TaskMain`).
- Wait until the M1 measurement runs have finished. Only one cFS process may run at a time (E13).

## 2. C1 (CF #184): the path is latent in the default configuration

- The use site is `apps/cf/fsw/src/cf_cfdp.c` L1270. `if (…chan[i].sem_name[0])` guards the lookup retry loop at L1279-1291: `OS_CountSemGetIdByName`, then `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)`, up to `CF_STARTUP_SEM_MAX_RETRIES` times; it retries only on `OS_ERR_NAME_NOT_FOUND`. The code's own comment at L1272-1278 reads: "There is a start up race condition because CFE starts all apps at the same time, and if this sem is instantiated by another app, it may not be created yet." If the result is still not `OS_SUCCESS`, the code sends `CF_INIT_SEM_ERR_EID` and takes the error path at L1293-1301.
- The default table `apps/cf/fsw/tables/cf_def_config.c` has `""` at L60 and L78. **The use site never runs** (ENV D4, D7).
- No app in the ENV bundle creates a counting semaphore that CF could name. Only `sch_lab` (its own timer semaphore) and `fm_child` call `OS_CountSemCreate` (ENV D4).
- Executing C1 therefore needs a derived configuration: a non-empty `sem_name` plus a provider app. That is decision P7. The three outcomes to record separately are first lookup failure, success after retry, and final failure (review §4.2).

## 3. C2, C3, C5: not present in the ENV build

- **C3 and C5.** Their buggy code exists only in the 6.4.0/6.4.1 tarballs (`C3.md` §3, `C5.md` §3).
  - At `c5fb2b4d` the C3 crash path is gone. F1 (range check) and F3 (S1 above) are present. The residual "SB AppId before publication" path is R2 (§1).
  - C5's lock-scope bug is replaced by the wait guard (`C5.md` §9). The possible residual window is decision X5.
- **C2.** The single-barrier design is replaced by `CFE_ES_WaitForSystemState` and the `APPS_INIT` phase (`C2.md` §9). The original apps are not public.
- These cases need the 6.4.x derived platform (decision P6) or a role-derived setup (C2).

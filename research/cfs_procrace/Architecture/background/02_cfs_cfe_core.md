# 02 — Concurrency-relevant architecture of the cFE core services (cFS v7.0.1)

- Date: 2026-10-12. Investigation only. No cFS tree was modified, nothing was built or run for this report.
- Pinned tree: `/home/user/work/procrace/cfs_ref/cFS` (nasa/cFS v7.0.1, cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`). Build used for configuration values: `build-native_std` (README mode, normal user).
- Labels used throughout:
  - **FACT**: the code, a generated build file, a config header, or a research record says so (cited).
  - **INFERENCE**: my reasoning from the facts. Not executed, not verified by experiment.
  - **UNVERIFIED**: a claim about literature or standards I could not check.
- Research records used: `research/cfs_procrace/cases/{ledger.md, mapping_v701.md, R2.md, C5.md}`, `results/R2_gdb_v701.md`, `env/CONDITIONS.md`, `env/BASELINE_MEASURE.md`, and the design proposal v0.1 (`39d4fa4a-...md`, §6–§7).

## 0. Path abbreviations and configuration in effect

All paths are relative to the pinned tree. In citations:

| Prefix | Directory |
|---|---|
| `es/` | `cfe/modules/es/fsw/src/` |
| `sb/` | `cfe/modules/sb/fsw/src/` |
| `evs/` | `cfe/modules/evs/fsw/src/` |
| `tbl/` | `cfe/modules/tbl/fsw/src/` |
| `time/` | `cfe/modules/time/fsw/src/` |
| `fs/` | `cfe/modules/fs/fsw/src/` |
| `rid/` | `cfe/modules/resourceid/fsw/src/` (`rid-inc/` = `.../resourceid/fsw/inc/`) |
| `api/` | `cfe/modules/core_api/fsw/inc/` |
| `gen/` | `build-native_std/native/default_cpu1/cpu1/` (CMake-generated) |

Configuration values that matter for concurrency (FACT, all defaults; the ENV build uses no overrides for these, `build-native_std/native/default_cpu1/inc/cfe_es_internal_cfg_values.h` simply includes the module default `cfe/modules/es/config/default_cfe_es_internal_cfg_values.h:36`):

| Value | Setting | Source |
|---|---|---|
| ES / EVS / SB / TIME / TBL main-task priority | 68 / 61 / 64 / 60 / 70 | `cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h:44`; `.../evs/fsw/inc/cfe_evs_internal_cfg.h:44`; `.../sb/fsw/inc/cfe_sb_internal_cfg.h:340`; `.../time/fsw/inc/cfe_time_internal_cfg.h:234`; `.../tbl/fsw/inc/cfe_tbl_internal_cfg.h:51` |
| TIME tone / local-1Hz child priority | 25 / 25 | `.../time/fsw/inc/cfe_time_internal_cfg.h:237,240` |
| ES background child priority | 200 (`PERF_CHILD_PRIORITY`) | `.../es/fsw/inc/cfe_es_internal_cfg.h:617`; `es/cfe_es_backgroundtask.c:43` |
| `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` | 30000 | `cfe/modules/core_private/config/default_cfe_core_private_internal_cfg.h:68` |
| `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` | 1000 | `.../es/fsw/inc/cfe_es_internal_cfg.h:879` |
| `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` | 50 | `.../es/fsw/inc/cfe_es_internal_cfg.h:860` |
| `MAX_APPLICATIONS` / `MAX_GEN_COUNTERS` | 32 / 8 | `.../es/fsw/inc/cfe_es_internal_cfg.h:107,179` |
| `APP_SCAN_RATE` / `APP_KILL_TIMEOUT` | 1000 ms / 5 scans | `.../es/fsw/inc/cfe_es_internal_cfg.h:199,229` |
| SB `MAX_MSG_IDS` / `MAX_PIPES` / `MAX_DEST_PER_PKT` / `DEFAULT_MSG_LIMIT` / `HIGHEST_VALID_MSGID` | 256 / 64 / 16 / 4 / 0x1FFF | `.../sb/fsw/inc/cfe_sb_internal_cfg.h:56,74,91,107,156` |
| SB route map implementation | HASH (default) | `cfe/modules/sbr/CMakeLists.txt:18-21` |
| EVS burst / events-per-second | 32 / 15 | `.../evs/fsw/inc/cfe_evs_internal_cfg.h:87,100` |
| TBL tables / handles / simultaneous loads | 128 / 256 / 4 | `.../tbl/fsw/inc/cfe_tbl_internal_cfg.h:128,157,173` |
| TIME server / client / signal / fake tone | true / false / false / true | `.../time/fsw/inc/cfe_time_internal_cfg.h:53,56,93`; `.../time/fsw/inc/cfe_time_interface_cfg.h:70` |
| Core pipe depths requested (ES/EVS/SB/TBL/TIME) | 12 / 32 / 32 / 12 / 12 (kernel truncates to 10 in README mode) | `es/cfe_es_task.h:50`, `evs/cfe_evs_task.h:58`, `sb/cfe_sb_priv.h:81`, `tbl/cfe_tbl_task.h:60`, `time/cfe_time_utils.h:70`; truncation: `env/CONDITIONS.md` E5 |
| Effective scheduling | all 27 threads `SCHED_OTHER`, nice 0; requested priorities have no effect | `env/CONDITIONS.md` E4, M1-5 |
| OSAL mutex type (POSIX) | recursive, priority-inheritance | `osal/src/os/posix/src/os-impl-mutex.c:82,94` |

---

## 1. ES (Executive Services)

### 1.1 Execution contexts that exist

FACT unless marked.

| Context | Created by | Body |
|---|---|---|
| **ES startup thread** (the PSP's main thread; not a cFE task, has no ES task record) | PSP | `CFE_ES_Main` `es/cfe_es_start.c:73-235` |
| Core main tasks ES, EVS, SB, TBL, TIME | ES startup thread, `CFE_ES_CreateObjects` | `es/cfe_es_start.c:776-847` |
| ES background child `ES_BG_TASK` | ES main task, `CFE_ES_BackgroundInit` | `es/cfe_es_backgroundtask.c:201-229`, body `:126-193` |
| TIME children "tone 1Hz" and "local 1Hz" | TIME main task | `time/cfe_time_task.c:160-184` |
| External app main tasks | ES startup thread (script), ES main task (StartApp command), or ES background task (restart/reload) | `es/cfe_es_apps.c:733-902`, `:1183-1399` |
| App child tasks | the app's main task only | `es/cfe_es_api.c:1202-1294` (main-task check at `:1268`) |
| OSAL timebase/timer thread (runs `CFE_TIME_Local1HzTimerCallback`) | OSAL, if the PSP created timebase "cFS-Master" | `time/cfe_time_task.c:311-318`, `time/cfe_time_tone.c:952-955` |

INFERENCE: library init functions (`CFE_LIB` lines) and the startup script run in the ES startup thread, which is not a registered cFE task, so `CFE_ES_GetAppID` cannot succeed there (see §1.5).

### 1.2 System states: where they are set

The enum is ordered so relational comparison is valid (`cfe/modules/es/config/default_cfe_es_extern_typedefs.h:184-219`, note `:224-228`).

| State | Meaning (header comment) | Set at | Writer |
|---|---|---|---|
| `EARLY_INIT` (1) | single-threaded set-up of cFE | `es/cfe_es_start.c:87` (after `memset` of the whole global at `:82`) | ES startup thread |
| `CORE_STARTUP` (2) | core apps starting, multi-threaded | `es/cfe_es_start.c:183` | ES startup thread |
| `CORE_READY` (3) | core ready, starting external apps/libs | `es/cfe_es_start.c:194` (after `CFE_ES_CreateObjects` returns, `:188`) | ES startup thread |
| `APPS_INIT` (4) | startup apps completed early init | `es/cfe_es_start.c:216` (after sync to `LATE_INIT`, `:210`) | ES startup thread |
| `OPERATIONAL` (5) | normal operation, all apps RUNNING | `es/cfe_es_start.c:234` (after sync to `RUNNING`, `:225`) | ES startup thread |
| `SHUTDOWN` (6) | "reserved for future use" | never set in v7.0.1 (only referenced in the wait mapping `es/cfe_es_api.c:552`) | — |

- FACT: `SystemState` is `volatile sig_atomic_t` (`es/cfe_es_global.h:151`). Every write above is a plain store **without** the ES shared-data lock. Readers in `CFE_ES_WaitForSystemState` poll it **without** the lock (`es/cfe_es_api.c:585`).
- INFERENCE: in C11 terms this is a data race on a non-atomic object (volatile is not an atomic). In practice on x86-64 with `-O0` (`env/CONDITIONS.md` C5) the store becomes visible. A detector should model it as a release/acquire *flag*, not as a lock-protected variable (see §8).
- FACT: the two startup syncs that gate `APPS_INIT` and `OPERATIONAL` only log on timeout and continue (`es/cfe_es_start.c:210-213`, `:225-228`). So the system can reach `OPERATIONAL` while some apps are still in `EARLY_INIT`/`LATE_INIT`.

### 1.3 Core object table, two phases, and the S1 serialization

FACT:
- The core object order is the CMake list `MISSION_CORE_MODULES` (`cfe/cmake/mission_defaults.cmake:26-40`; comment at `:26`: "the order of this list is the order in which they will be initialized"). CMake turns it into `CFE_CORE_MODULE_OBJTAB` (`cfe/cmake/target/CMakeLists.txt:55-100`). The generated table for cpu1 is `gen/cfe_static_module_list.c`: CONFIG, ES, EVS, FS, SB, TBL, TIME, MSG, SBR, RESOURCEID.
- Each module contributes one `Target_ObjectTable_t` entry with `EarlyInit`, `TaskMain`, `Cleanup`: ES `es/cfe_es_objtab.c:35-42` (EarlyInit = `CFE_ES_CDS_EarlyInit`, Cleanup NULL); EVS `evs/cfe_evs_objtab.c:34-39`; SB `sb/cfe_sb_objtab.c:33-38`; TBL `tbl/cfe_tbl_objtab.c:33-38`; TIME `time/cfe_time_objtab.c:33-38`; FS (EarlyInit only) `fs/cfe_fs_objtab.c:32-37`; CONFIG (EarlyInit only) `cfe/modules/config/fsw/src/cfe_config_objtab.c:33-38`; RESOURCEID (nothing) `rid/cfe_resourceid_objtab.c:32-37`.
- `CFE_ES_CreateObjects` (`es/cfe_es_start.c:741-850`):
  - **Phase 1** (`:752-773`): every `EarlyInit` in table order, in the ES startup thread. Failure → panic (`:769-770`). Order: `CFE_Config_Init`, `CFE_ES_CDS_EarlyInit`, `CFE_EVS_EarlyInit`, `CFE_FS_EarlyInit`, `CFE_SB_EarlyInit`, `CFE_TBL_EarlyInit`, `CFE_TIME_EarlyInit`.
  - **Phase 2** (`:776-847`), for each entry with a `TaskMain`, in order ES, EVS, SB, TBL, TIME:
    1. under the ES lock, find a free App slot, fill it, mark it `RESERVED` (`:788-806`, `:803`);
    2. `CFE_ES_StartAppTask` (`:810-814`);
    3. under the lock, mark the record with its real ID (`:815-825`, `:818`);
    4. `CFE_ES_MainTaskSyncDelay(CFE_ES_AppState_RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` (`:835`). Timeout → panic (`:838-846`).
- A core task becomes `RUNNING` when it calls `CFE_ES_WaitForSystemState(CORE_READY, ...)`, because for `CORE` apps any `MinSystemState >= CORE_READY` maps to `RequiredAppState = RUNNING` (`es/cfe_es_api.c:545-551`) and the caller's own state is raised (`:571-574`). The call sites are `es/cfe_es_task.c:134`, `sb/cfe_sb_task.c:93`, `evs/cfe_evs_task.c:229`, `tbl/cfe_tbl_task.c:74`, `time/cfe_time_task.c:88`. Each call follows the task's `*_TaskInit`.
- Consequence (INFERENCE, matches R2 S1 and `results/R2_gdb_v701.md` §2): `ES_TaskInit` → ES RUNNING → EVS created → `EVS_TaskInit` → EVS RUNNING → SB created → ... This is a **real** order with a hard bound (panic on timeout), so the core init sequence is a *fixed order* rather than a schedule-dependent one. It is also the reason ES and EVS use SB APIs before SB's task has published `CFE_SB_Global.AppId` (R2).
- Correction to a research record (FACT): `cases/mapping_v701.md` row S1 says "The order is fixed by `modules/es/fsw/src/cfe_es_objtab.c`". That file defines only the ES entry. The order comes from `MISSION_CORE_MODULES` via the generated `gen/cfe_static_module_list.c`. The conclusion (ES, EVS, SB, TBL, TIME) is unchanged, but the order is a **build-time configuration** that a mission can change. A detector should read it from the generated file, not hard-code it.

### 1.4 `CFE_ES_MainTaskSyncDelay`

FACT (`es/cfe_es_start.c:860-918`):
- It counts, under the ES lock, the App records that are "used" and whose `AppState < AppStateId` (`:878-888`, test at `:882`).
- "Used" means `AppId` is defined (`es/cfe_es_resource.h:166-169`). `CFE_RESOURCEID_RESERVED` = `0xFFFFFFFF` (`api/cfe_resourceid_api_typedefs.h:74`) counts as defined (`api/cfe_resourceid.h:180`). So a record that is still `RESERVED` (state `UNDEFINED` from the `memset`) also blocks the sync.
- It polls every 50 ms with `OS_TaskDelay` until the timeout (`:900-914`). Callers: core phase 2 (30000 ms, panic on timeout), and startup `LATE_INIT` and `RUNNING` (1000 ms each, log-only on timeout).
- INFERENCE: it is a *barrier implemented by polling a lock-protected state*. Each observed `AppState` write (done under the lock by the app) happens-before the poll that sees it, through the ES mutex. With an expired timeout there is no edge.

### 1.5 Startup script parsing and app/library creation

FACT:
- `CFE_ES_StartApplications` (`es/cfe_es_apps.c:115-332`) opens the volatile script on a processor reset, falling back to the non-volatile one (`:134-203`). It reads one byte at a time and calls `CFE_ES_ParseFileEntry` per `;`-terminated line (`:222-326`, call at `:313`). `!` ends the script (`:319-325`).
- `CFE_ES_ParseFileEntry` (`es/cfe_es_apps.c:340-445`): for `CFE_APP` it clamps priority (`:393-402`) and calls `CFE_ES_AppCreate` (`:427`). For `CFE_LIB` it calls `CFE_ES_LoadLibrary` (`:436`). The return status is ignored by the caller (`:313`), so a failed entry does not stop later entries.
- **No synchronization between script entries.** After `CFE_ES_AppCreate` returns, the startup thread goes straight to the next line. Contrast with core phase 2, which waits for `RUNNING` after each task (`es/cfe_es_start.c:835`). The only startup-script syncs come after **all** entries (`es/cfe_es_start.c:210`, `:225`).
- `CFE_ES_LoadLibrary` (`es/cfe_es_apps.c:910-1055`) runs the library init function **synchronously in the ES startup thread** (`:1013-1021`) while the Lib record is still `RESERVED` (`:989`). The record gets its real ID afterwards (`:1037`).
- The current ENV script (`build-native_std/exe/cpu1/cf/cfe_es_startup.scr`) loads 2 libraries and 15 apps (sch_lab 35, ci_lab 40, to_lab 45, sample_app 50, then lc/cf/ds/fm/hk/hs/mm/sc/md/cs/sbn at 70). Priorities have no scheduling effect here (`env/CONDITIONS.md` E4).
- INFERENCE: "script order" is a real order for exactly two things: (a) each library init completes before any later script entry starts (program order in one thread); (b) the `OS_TaskCreate` of app *k* precedes the `OS_TaskCreate` of app *k+1*. Script order does **not** order the *execution* of app *k*'s main task relative to app *k+1*'s.

### 1.6 Task start handshake and the RESERVED window (S7)

FACT:
- `CFE_ES_AppCreate` (`es/cfe_es_apps.c:733-902`):
  1. under the lock: duplicate-name check (`:777`), slot found, record filled, `AppId = RESERVED` (`:821`), unlock (`:827`);
  2. `CFE_ES_LoadModule` (`:841`);
  3. `CFE_ES_StartAppTask` (`:848-854`);
  4. under the lock: `AppId = real ID` (`:861-869`) or free (`:893`).
- `CFE_ES_StartAppTask` (`es/cfe_es_apps.c:649-725`) calls `OS_TaskCreate` **before** taking the lock (`:664-670`, lock at `:672`). It then fills the Task record and marks it used (`:694-703`).
- The new thread starts in `CFE_ES_TaskEntryPoint` (`:613-641`), which waits in `CFE_ES_GetTaskFunction` (`:559-605`): `OS_TaskDelay(50)` first (`:575`), then, under the lock, it checks that its **task** record has a defined `AppId` and `EntryFunc` (`:577-587`). The timeout is the 1000 ms script timeout (`:570`).
- It does **not** check the **App** record. `CFE_ES_GetAppID` needs `CFE_ES_GetAppRecordByContext`, which requires the App record to match the task's AppId (`es/cfe_es_resource.c:308-341`, `CFE_ES_AppRecordIsMatch` at `es/cfe_es_resource.h:253-256`). While the App record is still `RESERVED`, `GetAppID` returns `CFE_ES_ERR_RESOURCEID_NOT_VALID` (`es/cfe_es_api.c:757-761`).
- INFERENCE: between `es/cfe_es_apps.c:722` (task record published, lock released) and `:869` (App record published), a new main task that has passed `GetTaskFunction` would get `GetAppID` failures. Then `CFE_EVS_Register`, `CFE_SB_CreatePipe` (which stores the undefined AppId as pipe owner, `sb/cfe_sb_api.c:120,192`), `CFE_TBL_Register`, etc. would all be affected. Only the 50 ms `OS_TaskDelay` keeps this window closed in practice. This is the same window recorded as C5 D4 and R2 D5/S7 (`cases/C5.md` §9 L425-433; `cases/R2.md` §6 S7), confirmed here at v7.0.1 line numbers. It is a **sleep-based ordering**, not a real one.
- New detail found here (FACT + INFERENCE): `CFE_ES_LocateAppRecordByName` matches any record that is "used" (`es/cfe_es_resource.c:97`), and `RESERVED` counts as used. So during the same window `CFE_ES_GetAppIDByName` returns **`CFE_SUCCESS` with `AppId = 0xFFFFFFFF`** (`es/cfe_es_api.c:639-653`). A peer that looks up a starting app by name can therefore receive a "successful" sentinel that is not a valid ID. Downstream (FACT): `CFE_ES_AppID_ToIndex` rejects it (`rid/cfe_resourceid_api.c:132-137`), and EVS returns `ILLEGAL_APP_ID` (`evs/cfe_evs_utils.c:53-60`). The same applies during app **deletion**, when the record is set back to `RESERVED` (`es/cfe_es_apps.c:1498`).

### 1.7 `CFE_ES_WaitForSystemState` / `CFE_ES_WaitForStartupSync`

FACT (`es/cfe_es_api.c:514-619`):

| Caller type | `MinSystemState` | Caller's `AppState` raised to | Source |
|---|---|---|---|
| CORE | `>= CORE_READY` | `RUNNING` | `:545-551` |
| CORE | `< CORE_READY` | `EARLY_INIT` | `:536` |
| EXTERNAL | `>= SHUTDOWN` | `STOPPED` | `:552-555` |
| EXTERNAL | `>= OPERATIONAL` | `RUNNING` | `:556-559` |
| EXTERNAL | `>= APPS_INIT` | `LATE_INIT` | `:560-563` |
| EXTERNAL | otherwise | `EARLY_INIT` | `:536` |

- The raise happens under the ES lock and only upward (`:528`, `:571-576`).
- Then it polls `CFE_ES_Global.SystemState < MinSystemState` with `OS_TaskDelay(50)` until the timeout (`:584-605`). On timeout it returns `CFE_ES_OPERATION_TIMED_OUT` (`:599`).
- `CFE_ES_WaitForStartupSync(T)` = `WaitForSystemState(OPERATIONAL, T)` and **returns void** (`:616-619`). A caller cannot tell a timeout from success.
- **After startup** (`SystemState == OPERATIONAL`), every wait returns immediately. Apps started later by command, or restarted/reloaded by the background task, get no startup synchronization at all.
- INFERENCE (semantics a detector must keep):
  1. The wait is **opt-in**. Only callers are ordered. An app that never calls it is ordered with nothing (C1 ledger row).
  2. The call is both a *publish* (my state ≥ X) and a *consume* (system ≥ Y). It is the only cFE API that couples an app's own readiness to the global barrier, and it is coarse. "RUNNING" is set at the call, which may be **before** the app's service is actually ready (C2 pattern: work done after the wait is not covered).
  3. Edge: ES startup thread's state write at `es/cfe_es_start.c:216` or `:234` → the waiter's loop exit at `es/cfe_es_api.c:585`. That edge exists only when the loop actually observed the state; on timeout it does not.

### 1.8 App states: transitions and writers

FACT (enum `cfe/modules/es/config/default_cfe_es_extern_typedefs.h:264-294`, intended progression at `:300-301`).

| Transition | Where | Writer context | Lock |
|---|---|---|---|
| → `UNDEFINED` (memset at create) | `es/cfe_es_apps.c:799`; core `es/cfe_es_start.c:792-803` record fill | creator | ES lock |
| → `EARLY_INIT` / `LATE_INIT` / `RUNNING` / `STOPPED` via wait | `es/cfe_es_api.c:571-574` | the app itself | ES lock |
| → `RUNNING` via `CFE_ES_RunLoop` | `es/cfe_es_api.c:469-472` | the app itself | ES lock |
| `RUNNING` → `WAITING` + kill timer when `ControlReq > APP_RUN` | `es/cfe_es_apps.c:1139-1146` | ES background task | ES lock |
| → `STOPPED` in `CFE_ES_ExitApp` (external apps) | `es/cfe_es_api.c:399` | the app itself | ES lock |
| record → `RESERVED` → free (cleanup) | `es/cfe_es_apps.c:1498`, `:1632-1635` | ES background task | ES lock |

- FACT: restart/reload/delete requests are accepted only when `AppState == RUNNING` (`es/cfe_es_apps.c:88`, `CFE_ES_LockUserAppRecord`), and the background scan acts on a request only from `RUNNING` (`:1139-1140`). An app stuck in `LATE_INIT` cannot be restarted by command.

### 1.9 `CFE_ES_RunLoop`

FACT (`es/cfe_es_api.c:426-506`): increments the caller's task execution counter **without** a lock (`:441`; `CFE_ES_IncrementTaskCounter` `:1302-1324` documents why). Under the lock it raises the app to `RUNNING` (`:469-472`) and returns true iff `ControlReq.AppControlRequest == APP_RUN` (`:478-492`). INFERENCE: `RunLoop` is how the control-request protocol (§1.11) reaches the app. ES's write of `AppControlRequest` (under the lock) happens-before the app's `RunLoop` that reads it (under the lock). The app then leaves its loop and calls `ExitApp`.

### 1.10 Child tasks

FACT: `CFE_ES_CreateChildTask` (`es/cfe_es_api.c:1202-1294`) requires the caller to be the app's main task (`:1255-1274`). It then uses the same `CFE_ES_StartAppTask` (`:1290`), so the child's entry waits in `GetTaskFunction` exactly like a main task (§1.6). `CFE_ES_DeleteChildTask` calls `OS_TaskDelete` while holding the ES lock (`:1348`, `:1383`). `CFE_ES_ExitChildTask` frees its own record under the lock and calls `OS_TaskExit` (`:1453-1478`).
INFERENCE: for child tasks the parent's App record is already final, so the RESERVED window of §1.6 does not apply. Task create → child start is a real edge (`OS_TaskCreate` → `pthread_create`, see §8.1 R2).

### 1.11 Restart / reload / delete / exit (control-request protocol)

FACT:
1. **Request.** `CFE_ES_RestartApp` / `ReloadApp` / `DeleteApp` (`es/cfe_es_api.c:165-284`), usually from ES command handlers in the ES main task, set `ControlReq.AppControlRequest` to `SYS_RESTART` / `SYS_RELOAD` / `SYS_DELETE` under the lock (`:184`, `:239`, `:278`). `ExitApp` stores the app's own exit status if no request is pending (`:325-328`).
2. **Wake.** The ES main task wakes the background task after every command (`es/cfe_es_task.c:175`, `CFE_ES_BackgroundWakeup` = `OS_BinSemGive`, `es/cfe_es_backgroundtask.c:252-259`). The comment calls this wake "informational and not strictly required" (`:254-257`): the scan also runs on a timer.
3. **Scan.** `CFE_ES_RunAppTableScan` in the background task (`es/cfe_es_apps.c:1063-1175`), under the lock: `RUNNING` + request → `WAITING` with timer `5 × 1000 ms` (`:1139-1146`). A state `> RUNNING` counts the timer down. At 0 the app is queued (`:1114-1137`), then `CFE_ES_ProcessControlRequest` runs outside the lock (`:1159-1167`).
4. **Teardown.** `CFE_ES_ProcessControlRequest` (`:1183-1399`) snapshots the record under the lock (`:1216-1231`), calls `CFE_ES_CleanUpApp` (`:1249`), and for restart/reload calls `CFE_ES_AppCreate` again (`:1257`, new AppId). It then sends an event from the background task context (`:1398`).
5. **`CFE_ES_CleanUpApp`** (`:1407-1640`):
   - under the lock: collect the app's tasks and pools; mark every task record and the App record `RESERVED` (`:1433-1508`, `:1467`, `:1498`);
   - **without** the ES lock: call every core module's `Cleanup` in object-table order, i.e. EVS → SB → TBL → TIME (`:1526-1545`);
   - delete tasks, children first, main last (`:1561-1574`); delete pools (`:1579-1590`); unload the module (`:1595-1609`);
   - under the lock: free the records (`:1615-1637`).

INFERENCE (important for a lifecycle model):
- The kill path is **timer-driven**: an app that never calls `RunLoop`/`ExitApp` is torn down after about 5 s while its tasks are still running. In that case the module `Cleanup`s (SB pipes deleted, EVS registration freed, TBL handles released, TIME callback cleared) run **before** the tasks are deleted. The live tasks can observe torn-down resources: SB `ReceiveBuffer` on a deleted pipe gets `CFE_SB_BAD_ARGUMENT`; `GetAppID` fails because the task record is `RESERVED`.
- Restarted/reloaded apps run through `AppCreate` from the background task during `OPERATIONAL`. Their init races freely with every running app (no barrier, §1.7).
- Generic counters are **not** cleaned up per app: `CleanUpApp` handles tasks, pools and module cleanups only (`:1433-1609`). Counters have no owner field (`es/cfe_es_global.h:55-60`).

### 1.12 Background task

FACT: `ES_BG_TASK` runs a fixed job table (`es/cfe_es_backgroundtask.c:74-116`): the app table scan (`CFE_ES_RunAppTableScan`), the perf log dump, the PSP exception scan, and FS background file writes (`CFE_FS_RunBackgroundFileDump`, §6.1). It sleeps in `OS_BinSemTimedWait(WorkSem, NextDelay)` (`:185`), so it wakes on a give or on a timeout.
INFERENCE: everything done "by the background task" runs in **ES's** identity. Events sent there carry ES's AppId; FS write callbacks registered by SB/TBL/ES run in this foreign context (§6.1).

### 1.13 CDS registration

FACT:
- `CFE_ES_CDS_EarlyInit` (phase 1) creates the CDS mutex and validates, rebuilds or clears the CDS (`es/cfe_es_cds.c:51-150`). It sets `CDSIsAvailable = true` on success (`:144`).
- `CFE_ES_RegisterCDS` (`es/cfe_es_api.c:1566-1643`) needs a valid caller AppId (`:1576`, `:1587`), builds "AppName.Name" (`:1614`), and calls `CFE_ES_RegisterCDSEx(..., false)` (`:1617`). On error it sends an event with the caller's AppId (`:1633-1639`).
- `CFE_ES_RegisterCDSEx` (`es/cfe_es_cds.c:311-477`) holds the **CDS mutex** (not the ES lock) for the whole operation (`:337`, `:449`): name lookup (`:342`), allocate/reuse, mark used (`:438`), persist the registry (`:444`). A duplicate name is reused and reported as `CFE_ES_CDS_ALREADY_EXISTS` (`:461-472`). That code is how restore-after-reset works.
- `CFE_ES_GetCDSBlockIDByName` holds the CDS mutex (`es/cfe_es_api.c:1665-1680`). Block read and write hold the CDS mutex and re-validate the handle (`es/cfe_es_cds_mempool.c:178-244`, `:278-337`).
- TBL uses CDS in its EarlyInit (critical-table registry `"CFE_TBL.CritReg"`, `tbl/cfe_tbl_internal.c:123-126`) and in critical-table registration (§4.7).
- INFERENCE: CDS registration is lock-atomic and idempotent by name. The cross-app ordering issue is the *name* (any app that knows "App.Name" can look it up), and the publication is "visible once the CDS mutex is released". Fixed order (FACT): the CDS must exist before TBL's EarlyInit, which the object order ES (CDS) → … → TBL guarantees.

### 1.14 Generic counters

FACT (`es/cfe_es_api.c:1764-1953`):
- `RegisterGenCounter` writes the name and value and sets the real ID in one critical section (`:1780-1816`, `:1810`), with no `RESERVED` step.
- `Increment` and `Set` lock (`:1861-1867`, `:1883-1889`). `GetGenCount` reads **without** the lock (`:1899-1911`).
- `GetGenCounterIDByName` locks (`:1932-1950`).
- INFERENCE: counters are a name-addressed cross-app registry (typical use: one app registers, others find it by name). The only order is "register before lookup", and nothing in cFE enforces it.

### 1.15 The ES shared-data lock: what it protects and what it does not

FACT: `SharedDataMutex` (`es/cfe_es_global.h:141`) is created first (`es/cfe_es_start.c:93`). `CFE_ES_LockSharedData`/`Unlock` are `OS_MutSemTake`/`Give` (`es/cfe_es_api.c:2075-2117`). On POSIX it is recursive (`os-impl-mutex.c:94`).
- **Protected:** App/Task/Lib/Counter tables, App state, control requests, registration counters (all the sites above).
- **Not protected (FACT):** `SystemState` (§1.2); task `ExecutionCounter` (`es/cfe_es_api.c:1318-1323`); `GetGenCount` (`:1905-1907`); the generic-counter `Locate...ByID` before the lock (`:1860`, harmless); the background `NumJobsRunning` (`es/cfe_es_backgroundtask.c:183`).
- INFERENCE: the lock gives **atomicity**, not **order**, between two apps' API calls (the ledger says the same for C1/C2/C5). It creates HB edges only between a release and the *next* acquisition, so it orders things only when the program logic also checks state (e.g. `AppState`, record IDs).

### 1.16 "Reserve, then set used" publication patterns (all services)

| Record | Reserve | Fill outside lock | Publish real ID | Lookups that see the intermediate state |
|---|---|---|---|---|
| ES App (external) | `es/cfe_es_apps.c:821` | load module, create task (`:841-854`) | `:869` | `GetAppIDByName` returns `RESERVED` (§1.6); `GetAppID` from the new task fails; `MainTaskSyncDelay` counts it as not ready |
| ES App (core) | `es/cfe_es_start.c:803` | create task (`:810`) | `:818` | same |
| ES Lib | `es/cfe_es_apps.c:989` | load + **init function** (`:1010-1022`) | `:1037` | `GetLibIDByName` returns `RESERVED` (INFERENCE: same `Locate...ByName` pattern, `es/cfe_es_resource.c:115`) |
| ES Task | — (set used after `OS_TaskCreate`) | — | `es/cfe_es_apps.c:703` | the new thread waits on this record (`:577-587`) |
| ES App/Task (teardown) | `es/cfe_es_apps.c:1467`, `:1498` | module cleanups, deletes | free at `:1625`, `:1634` | same as above, reversed |
| SB Pipe | `sb/cfe_sb_api.c:153` | `OS_QueueCreate` (`:163`), the OSAL name becomes visible | `:190-194` | `GetPipeIdByName` finds the OSAL queue but no matching descriptor → error event (`sb/cfe_sb_api.c:802-839`) |
| SB Pipe (delete) | `sb/cfe_sb_api.c:409` | drain the queue | free later in `DeletePipeFull` | — |
| CDS block | none; the whole operation is under the CDS mutex | — | `es/cfe_es_cds.c:438` | none |
| Gen counter | none; one critical section | — | `es/cfe_es_api.c:1810` | none |
| EVS AppData | none, **no lock at all** | `memset` + fields (`evs/cfe_evs.c:56-104`) | `EVS_AppDataSetUsed` (`:106`) | concurrent `SendEventWithAppID` for the same AppId sees a half-built record (INFERENCE; for an app's own AppId this is single-writer) |
| TBL registry entry | `RegId` set under the registry lock (`tbl/cfe_tbl_registry.c:281-283`) | buffers and CDS set up inside the same lock | `OwnerAppId` set last, still inside the lock (`tbl/cfe_tbl_api.c:145-149`) | none, under the lock |
| Core identity `CFE_SB_Global.AppId` | zero from EarlyInit (`sb/cfe_sb_init.c:50`) | — | `sb/cfe_sb_task.c:138` | every SB event before this point (R2) |
| Core identity `CFE_EVS_Global.EVS_AppID` | zero (`evs/cfe_evs_task.c:63`) | register, pipe, subs | `evs/cfe_evs_task.c:312` ("now that the rest of initialization is done") | `EVS_SendEvent` guards it (`evs/cfe_evs_utils.c:613-623`) |
| Core identity `CFE_TBL_Global.TableTaskAppId` | zero (`tbl/cfe_tbl_internal.c:57`) | — | `tbl/cfe_tbl_task.c:196` (before its EVS registration at `:124`) | TBL API error events from apps (§4) |

INFERENCE: the common shape is *two-step publication of a composite object* (allocate+mark, fill, commit). Lookups that test only "slot used" see the middle state (ES by-name, SB by-name). Lookups that test "ID matches" do not. For a detector this suggests modeling each such object as a small state machine `FREE → RESERVED → LIVE → RESERVED → FREE` with explicit lookup predicates, instead of a single "created" event (this refines proposal §7.4 "operation boundaries").

---

## 2. SB (Software Bus)

### 2.1 Data structures and limits

FACT:
- Global: `SharedDataMutexId`, `AppId`, `StopRecurseFlags[OS_MAX_TASKS]`, `PipeTbl[64]`, `LastPipeId`, the `InTransitList` and `ZeroCopyList` buffer lists (`sb/cfe_sb_priv.h:239-259`). The pipe descriptor holds `AppId`, `SysQueueId`, `CurrentQueueDepth`, `LastBuffer` (`sb/cfe_sb_priv.h:172-180`).
- `CFE_SB_EarlyInit` zeroes the global, creates the SB mutex, the buffer pool and the routing tables (`sb/cfe_sb_init.c:44-82`).
- Routing: a MsgId is valid iff it is not invalid and `<= 0x1FFF` (`sb/cfe_sb_msg_id_util.c:200-204`). At most 256 routes (`CFE_SBR_AddRoute`, `cfe/modules/sbr/fsw/src/cfe_sbr_route_unsorted.c:94`), 16 destinations per route (`sb/cfe_sb_api.c:1032`), 64 pipes.

### 2.2 Pipes

FACT:
- `CFE_SB_CreatePipe` (`sb/cfe_sb_api.c:95-288`) gets the caller's AppId and **ignores the status** (`:120`). It reserves a descriptor (`:135-157`), creates the OSAL queue outside the lock (`:163`), commits under the lock (`:185-203`), then emits `CFE_SB_PIPE_ADDED_EID` with `CFE_SB_Global.AppId` (`:275-281`).
- Ownership is checked on `Subscribe` (`:976-980`) and `DeletePipe` (`:380-384`), **not** on receive (§2.5).
- `CFE_SB_DeletePipeFull` (`:352-...`) removes the pipe from all routes and marks it `RESERVED` under the lock (`:370-412`), then drains and deletes the queue outside the lock.
- `CFE_SB_GetPipeIdByName` (`:780-892`) asks OSAL for the queue by name, then scans `PipeTbl` for the same `SysQueueId` under the SB lock (`:802-839`).

### 2.3 Subscriptions and routing tables

FACT (`CFE_SB_SubscribeFull`, `sb/cfe_sb_api.c:937-1208`): all checks and updates happen under one SB lock (`:967-1091`): pipe valid and owned by the caller, MsgId valid, route created if absent (`:990-1013`), duplicate and limit checks, new destination appended with `MsgId2PipeLim` (`:1043-1072`). Events are sent after the unlock (`:1103-1205`); a global-scope subscription also triggers `CFE_SB_SendSubscriptionReport` (`:1192-1195`).

### 2.4 Transmit semantics

FACT:
- `CFE_SB_TransmitMsg` (`sb/cfe_sb_api.c:1587-1646`) allocates a buffer via `CFE_SB_AllocateMessageBuffer`, which **requires a valid caller AppId** (`:1465`, `:1479`), copies the message (`:1628`), and runs `CFE_SB_TransmitTxn_Execute` (`:1633`).
- Zero-copy: `AllocateMessageBuffer` (`:1453-1512`) tags the buffer with the AppId and puts it on `ZeroCopyList`; `TransmitBuffer` (`:1554-1579`) sends it; after `TransmitBuffer` the caller must not touch it (`sb/cfe_sb_priv.c:1280-1286`).
- `CFE_SB_TransmitTxn_FindDestinations` (`sb/cfe_sb_priv.c:989-1121`), **under the SB lock** (`:1007-1104`):
  - look up the route (`:1010`); on an origination, increment and stamp the per-route sequence counter (`:1016-1022`);
  - for each active destination whose pipe matches, apply the MsgId limit (`:1058-1064`) and optimistically increment the buffer use count and depth (`:1067-1074`);
  - if there is no route: `NoSubscribersCounter++` and event `CFE_SB_SEND_NO_SUBS_EID` (`:1081-1087`);
  - move the buffer to `InTransitList` and clear its owner (`:1095-1102`).
- Queue writes happen **outside** the lock, with `OS_QueuePut` in POLL mode (transmit transactions never set a timeout; the zero value is `TimeoutMode_POLL`, `sb/cfe_sb_priv.h:275`, `sb/cfe_sb_priv.c:541`, `:1148-1150`). On failure the accounting is undone under the lock and a `Q_FULL` event is queued (`:1163-1221`).
- Transmit-side events are reported in the **caller's** task, with `CFE_SB_Global.AppId` as the event source (`sb/cfe_sb_priv.c:763-794`, `:786`). Per-task recursion flags avoid loops (`:305-353`). Those flags are indexed by the caller's own task, so no lock is needed.

### 2.5 Receive semantics

FACT:
- `CFE_SB_ReceiveBuffer` (`sb/cfe_sb_api.c:1408-1445`) first releases the pipe's `LastBuffer` from the previous call, under the lock (`sb/cfe_sb_priv.c:1346-1382`). It then does `OS_QueueGet` with `PEND`, `POLL` or `TIMED` (`:1475-1479`). It then re-validates the pipe and stores the buffer as `LastBuffer`, updating depth and limit counts under the lock (`:1399-1458`).
- There is **no owner check** on receive. Any task holding a PipeId can receive from it. The comment at `:1362-1372` says the implicit "release previous buffer" design prevents sharing a pipe among workers.
- INFERENCE: a received buffer stays valid until the **same pipe's** next `ReceiveBuffer` call (or pipe deletion). Holding a pointer across that call is a use-after-release candidate. This is an SB-specific lifetime rule a detector should know.

### 2.6 What ordering SB delivery induces

INFERENCE, derived from the code above:
1. **Send → receive happens-before, per delivered buffer.** The sender writes the message content (memcpy at `sb/cfe_sb_api.c:1628`, or zero-copy fill) → releases the SB mutex at the end of `FindDestinations` (`sb/cfe_sb_priv.c:1104`) → `OS_QueuePut` (`:1177-1178`). The receiver: `OS_QueueGet` returns that descriptor (`:1475`) → acquires the SB mutex in `ExportReference` (`:1402`). The put precedes the get, so the sender's release precedes the receiver's acquire. That gives a mutex-based HB edge that does not depend on whether POSIX `mq_send`/`mq_receive` are memory-synchronizing. (UNVERIFIED: I did not find those calls in the POSIX memory-synchronization list; the SB mutex makes the question moot here.)
2. **The edge exists only for messages actually delivered to that pipe.** No edge when: there is no route yet (the message is dropped with `SEND_NO_SUBS`); the MsgId limit is exceeded (`:1058-1064`); the queue is full (`:1190-1194`, observed 4–49 drops per boot, `env/BASELINE_MEASURE.md` §3.2); the destination is inactive; or the pipe is deleted.
3. **Order guarantees:** FIFO per pipe (the OSAL queue is FIFO; INFERENCE, see report 01 for OSAL). One sender's messages to one pipe arrive in send order. No order across pipes, across MsgIds from different senders, or between one message's copies in different pipes. The per-route sequence counter is incremented under the lock (`:1018`), so stamped sequence numbers are totally ordered per route.
4. **A receive gives no edge back to the sender.** There is no reply or ack in SB. "Command → done" is ordered only if the receiver sends a telemetry or event message back and someone receives it.

### 2.7 Subscription-before-send dependencies

FACT: destinations are snapshotted under the lock at transmit time (`sb/cfe_sb_priv.c:1010-1079`). A subscription added later does not receive earlier messages; there is no retained or late-join delivery.
INFERENCE: every publish/subscribe pair has an implicit requirement **"subscriber's `Subscribe` returns before the producer's send"**, otherwise the message is silently lost (counted in `NoSubscribersCounter`). Observed: EVS event packets (MsgId 0x808) sent by ES before anything subscribes (`results/R2_gdb_v701.md` §2, 17 `SEND_NO_SUBS` uses). cFE itself provides no order for this pair except the startup barrier for apps that use it. TO_LAB subscribes only after OPERATIONAL (`env/CONDITIONS.md` D8), so everything sent before that is undelivered to TO by design. A detector needs a per-MsgId "route exists for pipe P" state and the drop outcome. This is the `subscription → send` contract.

### 2.8 Events SB emits on behalf of callers

FACT: SB API code emits events with `CFE_SB_Global.AppId` as the source, **in the caller's thread**, from:
- `CreatePipe` (`sb/cfe_sb_api.c:226-281`);
- `DeletePipe` error paths;
- `Subscribe`/`Unsubscribe` (`:1106-1205`);
- `GetPipeIdByName` (`:862-888`);
- every transmit/receive transaction error and `SEND_NO_SUBS` (`sb/cfe_sb_priv.c:786`).

`CFE_SB_Global.AppId` is 0 from EarlyInit (`sb/cfe_sb_init.c:50`) until SB's task stores it (`sb/cfe_sb_task.c:138`). SB registers with EVS at `:155`. Before that store, `CFE_EVS_SendEventWithAppID` gets AppId 0, `EVS_GetAppDataByID` returns NULL, and the result is `CFE_EVS_APP_ILLEGAL_APP_ID` (`evs/cfe_evs.c:185-189`). The SB callers discard that status. Observed: 24 such uses per boot from ES (20) and EVS (4), lost silently (`results/R2_gdb_v701.md` §2).
INFERENCE: this is a **fixed-order** violation (S1 forces ES/EVS init before SB), not a schedule-dependent race, as the R2 record already argues. In the proposal's classification (§7.2) it is a "고정 순서 후보" (fixed-order candidate) and needs no schedule exploration.

---

## 3. EVS (Event Services)

### 3.1 Registration

FACT (`CFE_EVS_Register`, `evs/cfe_evs.c:42-111`):
- The caller's context must resolve (`EVS_GetCurrentContext`: `CFE_ES_GetAppID` + index check, `evs/cfe_evs_utils.c:71-104`).
- The slot `AppData[AppIndex]` (`evs/cfe_evs_task.h:116`, indexed by `AppId % 32`) is `memset` and filled. The ID is published last (`evs/cfe_evs.c:106`, `EVS_AppDataSetUsed` = plain store `evs/cfe_evs_utils.h:134-137`).
- **No lock.** EVS has one mutex (`EVS_SharedDataMutexID`, created in EarlyInit `evs/cfe_evs_task.c:113`). It protects only the event log, the squelch tokens and the send counters (`evs/cfe_evs_log.c:43-100`, `evs/cfe_evs_utils.c:281-349`, `:517-535`).
- Cleanup: `CFE_EVS_CleanUpApp` frees the slot if the ID matches (`evs/cfe_evs_task.c:178-198`), from the ES background task.

### 3.2 `SendEvent`, `SendEventWithAppID`, `SendTimedEvent`, and the AppID checks

FACT:
- `CFE_EVS_SendEvent` / `SendTimedEvent` (`evs/cfe_evs.c:119-161`, `:222-264`) resolve the **caller's** AppId. An unresolvable caller gets `CFE_EVS_APP_ILLEGAL_APP_ID` (`evs/cfe_evs_utils.c:88-92`).
- `CFE_EVS_SendEventWithAppID` (`evs/cfe_evs.c:169-214`) trusts the **passed** AppId and has three outcomes:
  1. index invalid (undefined `0`, `RESERVED`, wrong base) → `ILLEGAL_APP_ID` (`:185-189`), **silent**;
  2. index valid but the slot holds a different or no ID (unregistered app, or a stale ID after restart) → `EVS_NotRegistered` (`:190-194`). That function sends **one** `CFE_EVS_ERR_UNREGISTERED_EVS_APP` event per (slot, caller) and a syslog line (`evs/cfe_evs_utils.c:153-182`), then returns `CFE_EVS_APP_NOT_REGISTERED`;
  3. registered → filter, squelch, generate.
- `ILLEGAL_APP_ID` vs `NOT_REGISTERED` is the only way an observer can tell "identity not yet published" from "identity published but not registered". R2 hit case 1 only (`results/R2_gdb_v701.md` §2: 0 uses between publication and registration).
- INFERENCE: `EVS_NotRegistered`'s check-and-set of `UnregAppID` (`:158-164`) and its counter increment (`:161`) are unlocked read-modify-writes.

### 3.3 Filtering and squelch

FACT:
- `EVS_IsFiltered` (`evs/cfe_evs_utils.c:190-241`) reads `ActiveFlag` and the event-type mask, finds the binary filter, and **increments `FilterPtr->Count` without a lock** (`:220-224`). EVS ground commands modify the same `AppData` (filters, enable flags) from the EVS task (`evs/cfe_evs_task.c:436-1295`), also without a lock: the only `OS_MutSem` call in that file is the creation at `:113`.
- Squelch is a token bucket per AppData slot, under the EVS mutex (`evs/cfe_evs_utils.c:249-361`). The "Events squelched" event is sent after the unlock (`:351-358`). Observed: SBN squelched 13–18 times per boot (`env/CONDITIONS.md` M1-4).
- INFERENCE: `SendEventWithAppID(CFE_SB_Global.AppId, ...)` is called from **many** tasks concurrently (every task that calls SB APIs), so SB's `AppData` filter counters are a genuine multi-writer unprotected counter. That is a classic memory-level data race. It is benign-looking (the filter count can be off by one), but it is the kind of finding a memory-race tool would report. A process-level detector should classify it separately from order violations.

### 3.4 EVS's own identity

FACT: `CFE_EVS_Global.EVS_AppID` is stored only at the end of `CFE_EVS_TaskInit` (`evs/cfe_evs_task.c:311-312`). `EVS_SendEvent` (internal events: unregistered-app notices, filter-locked, squelch) checks it explicitly: "Must check that EVS_AppID is valid, which can happen if this is called by some other thread before CFE_EVS_TaskInit() runs" (`evs/cfe_evs_utils.c:613-623`). Before that point such internal events are silently dropped.
INFERENCE: this is the "publish-last + use-site guard" fix lineage of C3 (F1/F2 in `cases/ledger.md` 1b). The guard converts an order violation into a silent drop. A detector must treat **guarded early use** as a distinct outcome ("not-ready, handled by drop") and report it as allowed or not according to a contract (proposal §6.3), not as a crash.

### 3.5 Event generation runs in the caller's context

FACT: `EVS_GenerateEventTelemetry` (`evs/cfe_evs_utils.c:437-536`) calls `CFE_ES_GetAppName` (`:480`), writes the local event log under the EVS mutex (`:490`, `evs/cfe_evs_log.c`), writes ports, calls `CFE_SB_TransmitMsg` (`:498` / `:513`), and takes the time from `CFE_TIME_GetTime()` in the callers (`evs/cfe_evs.c:146`, `:200`).
INFERENCE: one `SendEvent` therefore also touches ES (name lookup under the ES lock), TIME (reference read, §5.2) and SB (transmit, which itself requires the caller's AppId to allocate a buffer, `sb/cfe_sb_api.c:1465-1479`). An event from a context with no AppId fails at the EVS check before reaching SB.

---

## 4. TBL (Table Services)

### 4.1 Data and locks

FACT:
- Global (`tbl/cfe_tbl_task.h`): `TableTaskAppId` (`:171`), `RegistryMutex` and `WorkBufMutex` (`:178-179`), shared `LoadBuffs[4]` (`:181`), `Handles[256]` access descriptors (`:187`), `Registry[128]` (`:188`), `CritReg` (`:190`), `ValidationResults` (`:193`), `DumpControlBlocks[4]` (`:194`).
- Access descriptor: `LockFlag`, `Updated`, `BufferIndex` (`tbl/cfe_tbl_accdesc.h:61-63`). Registry record: `OwnerAppId` (`tbl/cfe_tbl_regrec.h:118`), `Notify` (`:129`).
- Transactions take the registry mutex once per transaction, with a counted nesting level (`tbl/cfe_tbl_registry.c:41-66`).

### 4.2 Register

FACT (`CFE_TBL_Register`, `tbl/cfe_tbl_api.c:48-176`): needs a valid caller AppId (`:68`, via `tbl/cfe_tbl_transaction.c:44-63`). Under the registry lock (`:88-153`):
- duplicate check (`:90`; same owner and same size = warning, another owner = error; `tbl/cfe_tbl_registry.c:309-350`);
- descriptor allocation, buffer setup, registry record setup;
- for critical tables, CDS registration and restore (`:117-143`);
- **`OwnerAppId` set last** (`:145-149`; the comment says "other APIs do not have to lock registry ... unowned entries are not checked").

On error it sends an event sourced as `CFE_TBL_Global.TableTaskAppId` (`:162-168`).
INFERENCE: in v7.0.1 `Share` and `GetAddress` do take the registry lock (§4.3, §4.5), so the "no lock needed" comment is a legacy rationale. Publication is lock-ordered.

### 4.3 Share across apps

FACT (`CFE_TBL_Share`, `tbl/cfe_tbl_api.c:184-236`): `CFE_TBL_TxnStartFromName` (`tbl/cfe_tbl_transaction.c:104-124`) locks the registry and looks the table up by its **full name** (`tbl/cfe_tbl_registry.c:228-248`; `LocateRegRecByName` tests "used" via `RegId`, `tbl/cfe_tbl_regrec.c:133-145`). If found, it allocates and connects an access descriptor. Not found → `CFE_TBL_ERR_INVALID_NAME`, a syslog line, an event, and a bad handle (`tbl/cfe_tbl_api.c:212-229`). There is no retry or wait inside cFE.
Cross-app order (INFERENCE): **owner's `Register` must complete before the sharer's `Share`**. The registry lock makes each call atomic and the outcome binary, but nothing orders the two apps except the startup barrier (if both use it, and only if the owner registers before calling the wait). Many apps share tables owned by other apps; mapping that is out of scope here (report on app patterns).

### 4.4 Load, validate, activate, update, manage

FACT:
- **Load by the owner** (`CFE_TBL_Load`, `tbl/cfe_tbl_api.c:287-348`): owner context required. The registry lock is **released** (`:305-306`, comment "As all ops are confined to this registry entry alone") **before** the state check `CFE_TBL_ValidateLoadRequest` (`:309`). The data is then loaded into the working buffer (`:315-324`) and validated (`:330`). `CFE_TBL_LoadFinish` completes the activation (`:344`).
- **Load by ground** (`CFE_TBL_LoadCmd`, TBL task, `tbl/cfe_tbl_task_cmds.c:392-480`): name lookup and the same `CFE_TBL_ValidateLoadRequest` **under** the registry lock (`:423-431`). The file is then read into the working buffer outside the lock (`CFE_TBL_LoadContentFromFile`, `:443`).
- `CFE_TBL_ValidateLoadRequest` (`tbl/cfe_tbl_load.c:764-820`) rejects the request if a load is already in progress (`:778-783`), otherwise **claims** a working buffer through `CFE_TBL_GetWorkingBuffer` (`:813`). That function checks `GetLoadInProgressBuffer` and then sets `LoadInProgress` (`tbl/cfe_tbl_internal.c:425-448`). A shared global load buffer is picked under `WorkBufMutex` only (`tbl/cfe_tbl_internal.c:379-412`).
- INFERENCE (candidate, from code; **not executed**): in the owner path the check-then-claim of `LoadInProgress` runs **without** the registry lock, while the ground path runs it **with** the lock. One lock-holder does not exclude a non-holder. An owner `CFE_TBL_Load` interleaved with a ground `LoadCmd` on the same table can therefore have both see "no load in progress" and both claim a buffer (the second `SetLoadInProgress` overwrites the first) and fill it. A check-then-act atomicity violation at process level (owner app task vs TBL task). Whether it is reachable in practice depends on an owner that loads at run time while ground loads the same table. Recorded as a candidate, not a finding.
- **Validate** (`CFE_TBL_Validate`, `tbl/cfe_tbl_api.c:603-723`): owner context. It picks the pending request and buffer under the lock, **releases the lock** (`:644`), runs the app's **validation function in the owner's task** (`:663`), then writes `IsValid`, `Result` and `State = CFE_TBL_VALIDATION_PERFORMED` without a lock (`:651-703`).
- **Validate request** comes from ground through the TBL task: `CFE_TBL_ValidateCmd` (`tbl/cfe_tbl_task_cmds.c:553-610`) creates the request under the lock (`:572-587`) and optionally sends the owner a notification message (`:595`). The TBL task **collects** results in its HK handler by reading `ValidationResults[i].State` / `Result` **without the registry lock** (`tbl/cfe_tbl_task_cmds.c:191-240`). Dump-only tables use the same flag pattern (`DumpCtrlPtr->State == CFE_TBL_DUMP_PERFORMED`, `:118-133`).
- **Activate** (`CFE_TBL_ActivateCmd`, `:618-662`) marks the load buffer `ActivateReq` under the lock and sends the optional notification (`tbl/cfe_tbl_internal.c:748-782` → SB). The owner's next `CFE_TBL_Manage` → `GetStatus` → `Update` performs the switch (`tbl/cfe_tbl_api.c:731-781`).
- **Update** (`CFE_TBL_UpdateInternal`, `tbl/cfe_tbl_internal.c:471-558`), under the registry lock (held by `CFE_TBL_Update`, `tbl/cfe_tbl_api.c:369-379`): double-buffered tables flip; single-buffered tables need an exclusive inactive buffer, else `CFE_TBL_INFO_TABLE_LOCKED` and retry later (`:503-516`). Then it sets the active buffer, records the load time from `CFE_TIME_GetTime()`, marks all users `Updated` (`:542-546`), updates the CDS for critical tables (`:549-552`), and discards the working buffer.
- INFERENCE: the validate/activate workflow is a **two-party handshake across tasks**: TBL task (request) ↔ owner app (performs, in `Manage`) ↔ TBL task (collects). The forward direction is ordered by the registry lock (request created → owner picks it up). The backward direction (owner's result → TBL HK reads it) is an **unprotected flag handshake** (`State` written after `Result`; the reader checks `State` then reads `Result`). This is ad hoc synchronization in the sense of Xiong et al. (OSDI 2010, §10). It works on x86 TSO at `-O0`, but a C11-level detector would flag it. Progress depends on the owner calling `Manage` periodically; a request to an owner that never calls it simply stays pending (a liveness property, not a race).

### 4.5 GetAddress / ReleaseAddress: the table lock-flag protocol

FACT: `CFE_TBL_GetAddress` (`tbl/cfe_tbl_api.c:439-470`) runs under the registry lock (via `TxnStartFromHandle`, `tbl/cfe_tbl_transaction.c:132-225`, lock at `:166`). `CFE_TBL_TxnGetTableAddress` (`tbl/cfe_tbl_registry.c:174-220`):
- not owned → `CFE_TBL_ERR_UNREGISTERED` (`:182-191`);
- never loaded → `CFE_TBL_ERR_NEVER_LOADED` (`:192-196`);
- otherwise sets `LockFlag = true`, records `BufferIndex`, and returns the pointer (`:204-210`).

`CFE_TBL_ReleaseAddress` clears `LockFlag` under the lock (`tbl/cfe_tbl_api.c:478-509`, `:489`). `Update` refuses to overwrite a buffer that a locked descriptor references (`tbl/cfe_tbl_internal.c:508-515`; helper `tbl/cfe_tbl_regrec.c:298`).
INFERENCE:
- This is a **real ordering protocol**: `ReleaseAddress` (release of the registry mutex after clearing the flag) happens-before the `Update` that observes the flag false (acquire). Reads of table content between `GetAddress` and `ReleaseAddress` are therefore ordered before an overwriting update.
- Reads after `ReleaseAddress`, or by a holder that never called `GetAddress`, are unprotected.
- Cross-app dependency: a sharer's `GetAddress` needs the owner to have **registered and loaded** (else `NEVER_LOADED`). That is a second required order beyond `Register → Share`.

### 4.6 Dump

FACT: a ground dump request (`CFE_TBL_DumpCmd`, `tbl/cfe_tbl_task_cmds.c:485-...`) either snapshots immediately or, for dump-only tables, creates a dump control block that the owner fills in `CFE_TBL_DumpToBuffer` (via `Manage`, `tbl/cfe_tbl_api.c:752-759`). The TBL task writes the file in HK processing when it sees `State == DUMP_PERFORMED` (`tbl/cfe_tbl_task_cmds.c:118-133`). The registry dump uses the FS background writer (`:908`).

### 4.7 Critical tables in CDS

FACT:
- TBL EarlyInit registers and restores the critical-table registry from CDS (`tbl/cfe_tbl_internal.c:123-150`).
- `Register` with the critical option calls `CFE_ES_RegisterCDSEx(..., true)` under the registry lock; on `ALREADY_EXISTS` it restores the table contents from CDS (`tbl/cfe_tbl_api.c:117-131`).
- `Update` writes the CDS copy (`tbl/cfe_tbl_internal.c:549-552`, `:679-747`).

INFERENCE: the lock order is registry mutex → CDS mutex (`RegisterCDSEx` inside the TBL lock). A detector modeling deadlock would need that edge.

### 4.8 Cross-app ordering dependencies (summary)

| Requirement | Enforced by cFE? | Outcome if violated (FACT) |
|---|---|---|
| owner `Register` → sharer `Share` | no | `CFE_TBL_ERR_INVALID_NAME`, event, bad handle (`tbl/cfe_tbl_api.c:212-229`) |
| owner `Load`(+ `Manage`) → sharer `GetAddress` | no | `CFE_TBL_ERR_NEVER_LOADED` (`tbl/cfe_tbl_registry.c:192-196`) |
| TBL validate/activate request → owner `Manage` | lock (request visible); owner progress not enforced | request stays pending |
| owner validation result → TBL HK collection | unprotected flag (§4.4) | stale or partial read (INFERENCE) |
| sharer `ReleaseAddress` → owner `Update` (single-buffer) | yes, lock-flag protocol | `INFO_TABLE_LOCKED`, update deferred |
| TBL task identity published → TBL error events from apps | S1 (TBL core init precedes all apps) | — |
| owner deleted → sharer's next access | lock + stale-handle checks | `ERR_UNREGISTERED` (`tbl/cfe_tbl_transaction.c:188-192`, `tbl/cfe_tbl_registry.c:182-191`) |

### 4.9 Cleanup

FACT: `CFE_TBL_CleanUpApp` (`tbl/cfe_tbl_internal.c:588-645`, ES background task) drops dump requests for tables owned by the app (`:604-620`), then releases every access descriptor the app holds (`:623-642`).

---

## 5. TIME

### 5.1 Tasks and the 1 Hz / tone pipeline (default config: server, fake tone, no signal)

FACT:
- `CFE_TIME_EarlyInit` → `CFE_TIME_InitData` (`time/cfe_time_task.c:49-57`).
- `CFE_TIME_TaskInit` (`:125-331`): register with EVS (`:133`); create `ToneSemaphore` and `LocalSemaphore` (`:140-158`); create children `Tone1HzTask` and `Local1HzTask` (`:160-184`); create a pipe and subscribe, including `SubscribeLocal` for tone/data/1Hz in server mode (`:186-279`); add an OSAL 1 Hz timer on timebase "cFS-Master" if it exists (`:311-327`).
- The pipeline (FACT for each step; the chaining is INFERENCE):
  1. OSAL timer callback (OSAL timebase thread) → `CFE_TIME_Local1HzISR` → `LocalIntCounter++`, `OS_BinSemGive(LocalSemaphore)` (`time/cfe_time_tone.c:952-955`, `:1238-1246`).
  2. `Local1HzTask`: `OS_BinSemTake` → transmit `LocalOneHzCmd` (`:1254-1302`, transmit at `:1290`).
  3. TIME main task receives it → `CFE_TIME_OneHzCmd` → `Local1HzStateMachine()` and, with fake tone, **`CFE_TIME_Tone1HzISR()` in the TIME main task** (`time/cfe_time_task.c:417-439`, `:425`, `:431`).
  4. `Tone1HzISR` validates the interval, updates `VirtualMET`/`IsToneGood`, gives `ToneSemaphore` (`time/cfe_time_tone.c:963-1059`, `:1033`), and **calls registered app synch callbacks** (`:1055`).
  5. `Tone1HzTask`: take → transmit `ToneSignalCmd` and (fake tone) `ToneSendCmd` (`:1067-1115`).
  6. TIME main receives them → `ToneSignal` / `ToneSend` / `ToneData` → reference update (§5.2).
- INFERENCE: TIME is internally a chain of real HB edges (sem give → take, SB send → receive). On this build all those threads run at `SCHED_OTHER` (`env/CONDITIONS.md` E4). Correctness relies on these edges, not on the tone task's priority 25.

### 5.2 Reference state: a lock-free versioned buffer (seqlock-like)

FACT:
- `ReferenceState[4]` (volatile) and `LastVersionCounter` (volatile) (`time/cfe_time_utils.h:97-98`, `:220-221`).
- The writer copies the current state into the next slot, sets `StateVersion` (`time/cfe_time_utils.c:49-72`), modifies it, then publishes by storing `LastVersionCounter = StateVersion` (`time/cfe_time_utils.h:534-537`). Writers: `time/cfe_time_tone.c:935`, `:1159`, `:1191`, `:1224`; `time/cfe_time_utils.c:183`, `:315`, `:780`, `:831`, `:878`, `:912`.
- The reader (`CFE_TIME_GetReference`, `time/cfe_time_utils.c:538-656`) reads the counter, copies the slot, and accepts if the slot's `StateVersion` still equals the counter (`:571`). It retries up to 4 times, then zeroes the output and sets `GetReferenceFail` (`:543`, `:585-612`).
- No mutex, no atomics, no fences: only `volatile`.
- INFERENCE: this is a seqlock variant over 4 slots. Boehm (MSPC 2012) explains why seqlock readers perform racing loads that conflict with data-race-free language memory models unless atomics/fences are used. With the default configuration all writers run in the TIME main task (commands, tone/1 Hz state machine), so there is a single writer. Readers are every task (`CFE_TIME_GetTime` from EVS, SB timestamps, TBL, apps). A detector should model `LastVersionCounter` publication as a release/acquire pair and **not** report the reader's speculative loads as races (or report them as "intentional racy reads"). The external `CFE_TIME_External*` APIs would add writers in other contexts, but `ExternalMET`/`GPS`/`Time` are compiled out by default (`time/cfe_time_api.c:692-732`). `CFE_TIME_ExternalTone` is always compiled (`:600-606`) and calls `Tone1HzISR` in the caller's context, which writes `ToneSignalLatch` etc. without a lock.

### 5.3 Synch callbacks

FACT: `CFE_TIME_RegisterSynchCallback` (`time/cfe_time_api.c:614-645`) stores a function pointer at `SynchCallback[AppIndex]` (one per app slot, no lock). `NotifyTimeSynchApps` reads each pointer once and calls it (`time/cfe_time_tone.c:1310-1333`; comment at `:1322-1325` about concurrent unregistration). `CFE_TIME_CleanUpApp` clears the slot (`time/cfe_time_utils.c:1018-1041`) during ES cleanup, **before** the app's module is unloaded (`es/cfe_es_apps.c:1526-1545` then `:1595-1609`).
INFERENCE:
- App callbacks run in **TIME's** context (the TIME main task with fake tone; otherwise wherever `Tone1HzISR` is invoked), not in the app's. A callback that calls cFE APIs that resolve the caller's AppId will act as TIME.
- A theoretical TOCTOU: TIME reads `Func`; ES background clears the slot, deletes the app's tasks and unloads the module; TIME then calls `Func`. Reading the pointer once prevents a NULL dereference, not a call into unloaded code. Not verified, low probability; it belongs to the lifecycle class.

### 5.4 TIME state used by others

FACT: `CFE_TIME_GetTime` and friends (`time/cfe_time_api.c:45-400`) read the reference (§5.2). They are called by EVS for every event (`evs/cfe_evs.c:146`, `:200`), by TBL on updates (`tbl/cfe_tbl_internal.c:544`), and through `CFE_SB_TimeStampMsg` by all HK producers (e.g. `tbl/cfe_tbl_task_cmds.c:92`).
INFERENCE: before TIME's first tone, readers get time derived from the initial reference set in EarlyInit (`CFE_TIME_InitData`, `time/cfe_time_utils.c:231-...`). Because EarlyInit precedes every task, there is no "TIME not initialized" window for readers. There is a "clock not yet set/valid" *state* (clock state), which is a value, not an ordering problem.

---

## 6. FS, resource IDs, CONFIG

### 6.1 FS background file-write queue

FACT:
- FS EarlyInit creates an FS mutex (`fs/cfe_fs_priv.c:49-...`, `:55`).
- `CFE_FS_BackgroundFileDumpRequest` (`fs/cfe_fs_api.c:852-...`) checks `Meta->IsPending` **before** locking, then under the FS mutex enqueues into a ring of `CFE_FS_MAX_BACKGROUND_FILE_WRITES`, sets `IsPending = true`, bumps `RequestCount`, and wakes the ES background task (`CFE_ES_BackgroundWakeup`).
- `CFE_FS_RunBackgroundFileDump` (`fs/cfe_fs_api.c:686-...`, ES background task) takes the head under the mutex (`:715-723`), calls the requester's `GetData` / `OnEvent` callbacks, and on completion bumps `CompleteCount` and clears `IsPending` under the mutex (`:831-840`). `CFE_FS_BackgroundFileDumpIsPending` reads the flag without the mutex (`:938-945`).
- Users: SB (`sb/cfe_sb_task.c:821`, `:962`, `:1088`), TBL (`tbl/cfe_tbl_task_cmds.c:908`), ES (`es/cfe_es_task.c:1577`).

INFERENCE: this is a request/complete producer-consumer with a lock-protected ring (real HB edges: request unlock → background lock; completion unlock → requester's next locked read). It has two cross-context features: (1) requester callbacks execute in the ES background task, reading requester-owned state (e.g. SB route snapshots) while the requester keeps running; (2) the unlocked `IsPending` pre-check is a benign flag read.

### 6.2 Resource IDs

FACT:
- An ID = base (per type) + serial (offsets `rid-inc/cfe_core_resourceid_basevalues.h:59-79`, bases `:90-108`). The ES TaskId base is the OSAL task type, i.e. cFE TaskIds are derived from OSAL IDs (`:59`; `CFE_ES_TaskId_FromOSAL`, `es/cfe_es_resource.c:63`).
- `CFE_ResourceId_FindNext` advances the serial from the last issued ID and skips used slots (`rid/cfe_resourceid_api.c:149-196`, `:67-...`).
- `CFE_ResourceId_ToIndex` = `(id - base) % TableSize` with a range check (`rid/cfe_resourceid_api.c:123-141`).
- `UNDEFINED` = 0, `RESERVED` = `0xFFFFFFFF` (`api/cfe_resourceid_api_typedefs.h:65`, `:74`).

INFERENCE:
- Serial IDs give **generation semantics**: a stale ID from a deleted or restarted app maps to the same slot index but fails `IsMatch`. That is why EVS reports `NOT_REGISTERED` (not `ILLEGAL`) for stale AppIds, and why the TIME callback slot and EVS AppData are indexed by `AppId % 32` with an ID check (EVS) or a cleanup guarantee (TIME).
- The proposal's `lifecycle_generation` field (proposal §6.1) maps directly onto the serial. Use `(type, serial)` as the identity; the slot index is not an identity.

### 6.3 CONFIG

FACT: `CFE_Config_Init` is the first EarlyInit (`gen/cfe_static_module_list.c`; `cfe/modules/config/fsw/src/cfe_config_objtab.c:33-38`). It is read-only data afterwards (SB reads `CFE_CONFIGID_PLATFORM_SB_MEM_BLOCK_SIZE` in its EarlyInit, `sb/cfe_sb_init.c:95`). INFERENCE: no concurrency relevance beyond "EarlyInit order".

---

## 7. Catalog: publish/readiness points vs use/consumer points

Columns: **Publish** = the store or API that makes the resource usable. **Consumers** = calls that another task or app can make. **Existing order** = what actually orders them (§8 defines "real"). **Not-ready outcome** = what the consumer observes if it runs first (FACT where cited).

### 7.1 Identity records

| Resource | Publish | Cross-task consumers | Existing order | Not-ready outcome |
|---|---|---|---|---|
| ES Task record (per task) | `es/cfe_es_apps.c:703` (ES lock) | the task itself via `GetTaskFunction` (`:577-587`); `CFE_ES_GetTaskID`; `GetTaskInfo` | lock + poll loop (real once observed; 1000 ms timeout) | task never calls its entry (`:625`) |
| ES App record (per app) | `es/cfe_es_apps.c:869` / core `es/cfe_es_start.c:818` | the app's own `GetAppID` (EVS/SB/TBL/TIME/CDS register paths); peers' `GetAppIDByName`, `GetAppInfo`, `Restart/Reload/Delete` | **none** for the app's own use (50 ms sleep only); lock for peers | `ERR_RESOURCEID_NOT_VALID`; by-name returns success + `RESERVED` (§1.6) |
| ES Lib record | `es/cfe_es_apps.c:1037` | `GetLibIDByName`; apps calling library functions | startup-thread program order: library init and publish precede later script entries | by-name returns `RESERVED` (INFERENCE); calling an uninitialized lib is app-defined |
| `CFE_SB_Global.AppId` | `sb/cfe_sb_task.c:138` (plain store) | every SB API caller (as event source) | S1 orders SB before TBL, TIME and all apps; **forces** ES and EVS before it | `ILLEGAL_APP_ID`, silent (R2) |
| SB's EVS registration | `sb/cfe_sb_task.c:155` | same | same | `NOT_REGISTERED` + one event (none observed) |
| `CFE_EVS_Global.EVS_AppID` | `evs/cfe_evs_task.c:312` | EVS internal events from any thread | S1 + use-site guard (`evs/cfe_evs_utils.c:617-623`) | silent drop |
| `CFE_TBL_Global.TableTaskAppId` | `tbl/cfe_tbl_task.c:196` | TBL API error/info events from any app; "table services root" check (`tbl/cfe_tbl_transaction.c:159-163`) | S1 (TBL before TIME and all apps) | — (not reachable by default; INFERENCE) |
| App's EVS registration (`AppData[i]`) | `evs/cfe_evs.c:106` (no lock) | the app's own sends; `SendEventWithAppID` on its behalf (rare); EVS commands | program order within the app | `NOT_REGISTERED` + one event (`evs/cfe_evs_utils.c:153-182`) |

### 7.2 Name-addressed registries (by-name lookups)

| Registry | Publish (create) | Lookup | Lock | Not-found outcome | Existing cross-app order |
|---|---|---|---|---|---|
| ES apps | `AppCreate` (`es/cfe_es_apps.c:733-902`) | `CFE_ES_GetAppIDByName` (`es/cfe_es_api.c:627-658`) | ES | `ERR_NAME_NOT_FOUND` + undefined ID | script order of **creation** only; no readiness |
| ES libs | `LoadLibrary` | `CFE_ES_GetLibIDByName` (`:666-697`) | ES | same | startup-thread program order |
| ES tasks | `StartAppTask` | `CFE_ES_GetTaskIDByName` → OSAL `OS_TaskGetIdByName` (`:705-730`) | OSAL table lock | same | none (visible as soon as OSAL registers the name, before ES's task record) |
| ES gen counters | `RegisterGenCounter` (`:1764-1820`) | `GetGenCounterIDByName` (`:1919-1953`) | ES | same | none |
| CDS blocks ("App.Name") | `RegisterCDS(Ex)` (`es/cfe_es_cds.c:311-477`) | `CFE_ES_GetCDSBlockIDByName` (`es/cfe_es_api.c:1651-1683`) | CDS mutex | `ERR_NAME_NOT_FOUND` | none |
| SB pipes | `CreatePipe` (`sb/cfe_sb_api.c:95-288`) | `CFE_SB_GetPipeIdByName` (`:780-892`) | OSAL then SB | `BAD_ARGUMENT` + error event | none; two-registry window (§1.16) |
| SB routes (MsgId → pipes) | `Subscribe` (`:937-1208`) | every transmit (`sb/cfe_sb_priv.c:1010`) | SB | silent drop, `NoSubscribersCounter` | none (startup barrier only if both sides use it) |
| TBL tables ("App.Table") | `Register` (`tbl/cfe_tbl_api.c:48-176`) | `CFE_TBL_Share` (`:184-236`); `GetInfo` (`:820`); ground commands by name | TBL registry | `ERR_INVALID_NAME` + event | none |
| OSAL objects (sems, queues, ...) | `OS_*Create` | `OS_*GetIdByName` | OSAL | `OS_ERR_NAME_NOT_FOUND` | none (C1; see report 01) |

### 7.3 State machines exposed to other tasks

| State machine | States / publish points | Consumers | Order mechanism |
|---|---|---|---|
| System state | §1.2 | `WaitForSystemState` callers | polled volatile flag (real only when observed, else timeout) |
| App state | §1.8 | `MainTaskSyncDelay` (ES startup thread), background scan, `LockUserAppRecord`, `GetAppInfo` | ES lock + polling |
| App control request | `es/cfe_es_api.c:184,239,278,327` | the app's `RunLoop` (`:478`), background scan (`es/cfe_es_apps.c:1140`) | ES lock; binsem wake + 1000 ms timer |
| TBL load/validate/activate | §4.4 (`LoadInProgress`, `PendingValId`, `ActivateReq`, `ValidationResults.State`, `DumpCtrl.State`) | owner `Manage`/`GetStatus`; TBL HK | registry lock forward; unprotected flags backward |
| TBL access lock flag | `LockFlag` (§4.5) | `Update` | registry lock (real) |
| TIME reference | `LastVersionCounter` (§5.2) | `GetTime` everywhere | seqlock-like versioning |
| TIME clock/tone flags | `IsToneGood`, `AutoStartFly`, counters (`time/cfe_time_tone.c:1008-1046`, `:1278-1283`) | TIME tasks, HK | single-writer flags, unlocked |
| FS background write | `IsPending`, `RequestCount`/`CompleteCount` (§6.1) | requester, ES background | FS mutex + binsem wake |
| CDS availability | `CDSIsAvailable` (`es/cfe_es_cds.c:144`) | `RegisterCDS` and the CDS APIs | EarlyInit precedes every task |
| SB subscription reporting | `SubscriptionReporting` (`sb/cfe_sb_init.c:60`; SB commands) | `SendSubscriptionReport` | SB task program order |

### 7.4 Buffers, handles and callbacks with lifetimes

| Object | Valid from → until | Cross-task hazard (INFERENCE) |
|---|---|---|
| SB received buffer | `ReceiveBuffer` return → next `ReceiveBuffer` on the same pipe or pipe delete (`sb/cfe_sb_priv.c:1374-1379`) | use after implicit release; pipe shared by two tasks |
| SB zero-copy buffer | `AllocateMessageBuffer` → `TransmitBuffer`/`Release` (`sb/cfe_sb_api.c:1453-1579`) | use after transmit; freed at app cleanup (`sb/cfe_sb_priv.c:121`) |
| TBL table pointer | `GetAddress` → `ReleaseAddress` (§4.5) | read after release while the owner updates |
| TBL handle | `Register`/`Share` → `Unregister` / owner cleanup | stale handle → `ERR_UNREGISTERED` (checked) |
| TIME synch callback | `Register` → `Unregister` / app cleanup | call into unloaded module (§5.3) |
| FS write metadata | `Request` → `IsPending == false` | requester mutates `Meta` while background reads it |

---

## 8. Happens-before rules a cFS-aware detector must model

The design proposal's own rule (§7.1) is: "requirement order" must never be added as an HB edge, and not every give or send matches every take or receive. The table below lists only **real** edges, each with its matching condition. Unless marked, the conditions are FACT from the cited code; that they form HB edges is INFERENCE, based on the standard definitions (Lamport, CACM 1978; mutex/semaphore/fork-join and message rules as in DroidRacer, PLDI 2014, which also combines thread and event-queue HB in one relation).

### 8.1 Real edges

| # | Edge | Matching condition | Caveats |
|---|---|---|---|
| R1 | **Program order** within one thread | same OSAL task | cFE "app" ≠ thread: an app's main task and child tasks are different threads; callbacks run in foreign threads (TIME §5.3, FS §6.1, ES background §1.11) |
| R2 | **Task create → task start** | `OS_TaskCreate` call (`es/cfe_es_apps.c:664`) → first instruction of `CFE_ES_TaskEntryPoint` in the new thread | The edge is from the *call*, not from ES's later record writes (`:694-703`, `:869`). Those reach the new task only through R4/R5 (`GetTaskFunction` lock + poll for the task record; **nothing** for the App record). POSIX lists `pthread_create` as memory-synchronizing (verified only via secondary sources, see §10) |
| R3 | **Startup-thread program order across script entries** | library init `k` → any later script entry; `AppCreate(k)` → `AppCreate(k+1)` | creation only, not app progress (§1.5) |
| R4 | **Mutex release → next acquire** (ES shared, SB, TBL registry, TBL work-buffer, CDS, EVS, FS, ES perf) | same mutex; **outermost** lock/unlock only (OSAL mutexes are recursive, `os-impl-mutex.c:94`; nested acquisitions such as TBL's counted lock `tbl/cfe_tbl_registry.c:41-66` add no edge) | gives order only together with a state check made inside the lock; otherwise it is atomicity, not order |
| R5 | **Polled-flag barriers** (ad hoc sync) | write `X` → the poller's loop exit **that observed** `X` | Three instances: `WaitForSystemState` (`es/cfe_es_api.c:585`, unlocked volatile read); `MainTaskSyncDelay` (`es/cfe_es_start.c:878-895`, locked read); `GetTaskFunction` (`es/cfe_es_apps.c:577-592`, locked read). **Timeout exit → no edge.** Core-phase timeout → panic; startup-script and wait timeouts → continue silently (`WaitForStartupSync` returns void) |
| R6 | **ES state write → barrier release**, composed | app's `AppState` write (under ES lock) → ES startup thread's sync observes all apps (R4+R5) → `SystemState` store → waiter observes (R5) | So app A's code *before* its `WaitForSystemState(APPS_INIT/OPERATIONAL)` call → app B's code *after* its own wait returns without timeout. Code after A's wait call is **not** ordered (C2) |
| R7 | **SB send → receive** | the transmit that enqueued buffer descriptor *d* on pipe *p* → the `ReceiveBuffer` on *p* that dequeued *d* | Per-buffer matching (not "every send → every receive"). Established through the SB mutex (§2.6.1). No edge for dropped messages (no route, limit, full, inactive, deleted). FIFO per pipe. No reverse edge |
| R8 | **Subscribe → later transmit routing** | `Subscribe(m, p)` unlock → a transmit of `m` whose locked route lookup sees *p* | This is a *conditional* edge: if the transmit happened first, the outcome is "dropped". Model it as state (route exists?), not as an edge to add |
| R9 | **Binary semaphore give → take** | give that changed the semaphore 0→1 → the take that consumed it | Binary semantics collapse multiple gives (ES `WorkSem` is "informational", `es/cfe_es_backgroundtask.c:254-257`; TIME tone/local sems). `OS_BinSemTimedWait` timeout → no edge (`:185`) |
| R10 | **Counting semaphore give → take** | k-th give → k-th take (FIFO matching by count), same OSAL ID | app-level (C1); not used inside cFE core |
| R11 | **TBL lock-flag protocol** | `ReleaseAddress` (flag cleared under registry lock) → `Update` that saw no lock (registry lock) | composite of R4 + state; real |
| R12 | **TBL request → owner processing** | `ValidateCmd`/`ActivateCmd` (registry lock) → owner's `Manage`/`GetStatus` that sees the pending request (registry lock); optional SB notification adds R7 | real in the forward direction |
| R13 | **TBL result → TBL HK collection** | owner writes `Result` then `State` (no lock) → TBL task reads `State` then `Result` (no lock) | **no** language-level HB; ad hoc flag sync. Treat as a racy handshake: report or model as acquire/release by policy |
| R14 | **TIME reference publication** | writer's `LastVersionCounter` store → a reader that read that counter and validated `StateVersion` | seqlock: reader's copied data counts as ordered only if validation succeeds; failure → zeroed output (`time/cfe_time_utils.c:601-612`) |
| R15 | **ES control request → app exit / teardown** | request write (ES lock) → app `RunLoop` read (ES lock) → `ExitApp` → background scan sees `STOPPED` (ES lock) | the teardown is also reached by the **timer** without any app action (5 s); then no edge from the app |
| R16 | **FS request → background write → completion** | request unlock → background head read (FS mutex); completion unlock → requester's next locked/flag read | requester callbacks run in the ES background thread |
| R17 | **Task delete / exit** | `OS_TaskDelete` return in the deleter ⇒ the victim executes nothing afterwards (INFERENCE; OSAL semantics in report 01) | `ExitApp` suspends in a delay loop until deleted (`es/cfe_es_api.c:410-413`) |
| R18 | **EarlyInit → all tasks** | every phase-1 EarlyInit completes before the first core task is created (`es/cfe_es_start.c:752-773` then `:776`) | real (single thread + R2) |
| R19 | **Core phase-2 serialization (S1)** | core task *i* sets `RUNNING` (inside its `WaitForSystemState(CORE_READY)`, after its TaskInit) → creation of core task *i+1* | real; 30 s timeout → panic. Makes ES-TaskInit → EVS-TaskInit → SB-TaskInit → TBL → TIME a fixed order (§1.3) |

### 8.2 Things that look like ordering but are not

| Not an order | Why (FACT / INFERENCE) |
|---|---|
| **Task priorities** (startup script, core defaults) | FACT on this build: all threads run `SCHED_OTHER`, nice 0; requested priorities are overridden (`env/CONDITIONS.md` E4, M1-5). Even under RT, on 4 vCPUs a lower-priority task runs concurrently. Model priorities as a scheduling *constraint* only under an explicit RT, single-CPU configuration (proposal §7.1) |
| **`OS_TaskDelay` / sleeps / poll periods** | the 50 ms initial delay in `GetTaskFunction` (`es/cfe_es_apps.c:575`) is the only thing closing the RESERVED window (§1.6); `ExitApp`'s 500 ms loop; app-level retry delays (C1 fix: 25 × 100 ms). A sleep bounds nothing about another thread's progress |
| **Script order** for app execution | creation order only (§1.5, R3) |
| **Object-table order alone** | it orders EarlyInit and *creation*; the real order between core TaskInits comes from S1 (R19), which is a different mechanism. Changing `MISSION_CORE_MODULES` changes it |
| **A timed-out barrier** | `WaitForStartupSync` returns void; startup syncs log and continue (`es/cfe_es_start.c:210-228`); `GetTaskFunction` timeout → task never runs |
| **`WaitForStartupSync` after startup** | returns immediately in `OPERATIONAL` (§1.7); restarted/reloaded/commanded apps are unordered |
| **"RUNNING" app state** | set at the *call* of `WaitForSystemState`/`RunLoop`, not when the app's services are ready (C2) |
| **The ES shared-data lock between two apps' API calls** | atomicity, not order (§1.15) |
| **`volatile`** on `SystemState`, TIME reference, TIME callback pointers | no inter-thread ordering in C11. The edges in R5/R14 come from protocol (observe-then-act, version validation), not from the keyword |
| **ES background wake (binsem give)** | "informational"; the scan also runs on a 1000 ms timer (`es/cfe_es_backgroundtask.c:254-257`, `:74-79`) |
| **Sending an SB message to a MsgId nobody subscribes to yet** | dropped; no edge (R8) |
| **EVS events / syslog / console line order** | console order is not event order (`env/CONDITIONS.md` E8); event loss paths (squelch, filter, `ILLEGAL_APP_ID`, `NO_SUBS`) are silent |
| **Use-site validity guards** (EVS `ILLEGAL_APP_ID`, `EVS_AppID` guard, TBL `NEVER_LOADED`, stale-handle checks) | they turn early use into an error outcome; they do not order anything (R2 S6) |
| **OSAL name-table lock** | makes each create/lookup atomic, does not order a create before a lookup (C1) |

### 8.3 Ad hoc synchronization inventory (for the model)

INFERENCE. Every site below is a hand-written wait or flag that a generic detector would not recognize without a model. Xiong et al. (OSDI 2010) report that ad hoc synchronization is common and error-prone; Raychev et al. (OOPSLA 2013) handle it in event-driven race detection through "race coverage". These sites should be modeled as conditional edges with an explicit timeout or escape branch:

1. `WaitForSystemState` poll (`es/cfe_es_api.c:585-605`).
2. `MainTaskSyncDelay` poll (`es/cfe_es_start.c:871-915`).
3. `GetTaskFunction` poll (`es/cfe_es_apps.c:573-596`).
4. TBL `ValidationResults.State` / `DumpCtrl.State` flags (§4.4, §4.6).
5. TIME seqlock (§5.2).
6. FS `IsPending` (§6.1).
7. ES `AppTimerMsec` countdown (`es/cfe_es_apps.c:1126-1137`): a time-based state transition.
8. TIME `AutoStartFly` / `IsToneGood` flags (single-writer).

### 8.4 Implications for the new detector (INFERENCE; input to the design phase, not decisions)

1. **Fixed-order checks first.** S1 (R19) and phase-1 EarlyInit (R18) make the whole core-init region a single partial order that can be computed from the generated object table and the `*_TaskInit` call graphs, with no schedule exploration. R2 falls here. So would any future "core service used before its identity is published" defect, in either direction. This supports the proposal's §7.2 first-pass classification and §8.5.
2. **Startup-script apps form the main schedule-dependent region.** Their only cross-app edges are R6 (opt-in, coarse) and R7/R8 (SB). Required orders (`Register → Share`, `Create → GetIdByName`, `Subscribe → Send`, `Register → GetAddress(loaded)`) have no real edge unless both apps place them on opposite sides of the barrier. The detector needs: per-app "where is my `WaitForSystemState` call" facts; per-resource publish/use sites; and the barrier timeout branch.
3. **Restart/reload/command start** is a separate region with **no** barrier at all (§1.7, §1.11). It deserves its own scenario family (the app re-runs its init against a running system).
4. **Lifecycle and generation.** Model `FREE → RESERVED → LIVE → RESERVED → FREE` per record (§1.16), with lookup predicates (`IsUsed` vs `IsMatch`). Use `(type, serial)` identities (§6.2). The `RESERVED` sentinel returned by `GetAppIDByName` (§1.6) is a concrete case where a lookup "succeeds" with a non-ID.
5. **Operation boundaries** that matter (refining proposal §7.4):
   - `OS_TaskCreate` (new thread may run before ES's record writes);
   - `es/cfe_es_apps.c:703` vs `:869`;
   - `sb/cfe_sb_api.c:163` vs `:194`;
   - `tbl/cfe_tbl_api.c:149`;
   - `sb/cfe_sb_task.c:138` vs `:155`;
   - `evs/cfe_evs_task.c:312`;
   - TBL "check under lock, act outside" splits (`tbl/cfe_tbl_api.c:305-306`, `:644`; `tbl/cfe_tbl_task_cmds.c:423-431`).
6. **Outcome taxonomy** per consumer, from the code: crash / error status propagated / error status discarded (R2) / silent drop (SB `NO_SUBS`, EVS guards) / event-then-continue / retry (app-level). This matches the proposal's `report_separately` list (§6.3) and is needed for harm classification.
7. **Memory-level races found in passing** (EVS filter counters, `EVS_NotRegistered` flag, task `ExecutionCounter`, `GetGenCount`, TIME flags) should be reported in a separate class, or suppressed by policy. They are not process-level order violations and would inflate counts.

---

## 9. Corrections and confirmations against existing research records

| Record | Statement | Finding here |
|---|---|---|
| `cases/mapping_v701.md` §1.1 row S1 | "The order is fixed by `modules/es/fsw/src/cfe_es_objtab.c`" | **Correction:** the order comes from `MISSION_CORE_MODULES` (`cfe/cmake/mission_defaults.cmake:26-40`) via `gen/cfe_static_module_list.c`; `cfe_es_objtab.c` holds only ES's entry. Resulting order is unchanged |
| `cases/C5.md` §9 / D4; `cases/R2.md` S7 / D5 | possible residual window (task record vs App record) | **Confirmed by code reading at v7.0.1:** `es/cfe_es_apps.c:575-587` (task record only), `:821` / `:869` (App record), `es/cfe_es_resource.c:308-341`. **New detail:** in the same window `CFE_ES_GetAppIDByName` returns success with `0xFFFFFFFF` (§1.6). Not executed |
| `results/R2_gdb_v701.md` | order comes from S1 + object order, not a close race | Consistent with §1.3 / R19. S1 has a 30 s panic bound, so it is a guaranteed order in every non-panicking boot (INFERENCE) |
| `cases/ledger.md` C3 row | "Object-table creation order ... orders creation, not execution" (6.4.1) | At v7.0.1, S1 adds execution ordering among core tasks (R19); script apps still match the old statement |

---

## 10. Literature cited (verified by web search on 2026-10-12)

- L. Lamport, "Time, Clocks, and the Ordering of Events in a Distributed System," *Communications of the ACM* 21(7):558–565, 1978. (happens-before as a partial order)
- C. Flanagan, S. N. Freund, "FastTrack: Efficient and Precise Dynamic Race Detection," PLDI 2009. (vector-clock HB race detection; mentioned as the standard dynamic baseline)
- P. Maiya, A. Kanade, R. Majumdar, "Race Detection for Android Applications," PLDI 2014 (SIGPLAN Notices 49(6):316–325). (HB that combines thread and message-queue/event rules; closest analogue to SB + tasks)
- V. Raychev, M. Vechev, M. Sridharan, "Effective Race Detection for Event-Driven Programs," OOPSLA 2013. (race coverage; handling ad hoc synchronization)
- W. Xiong, S. Park, J. Zhang, Y. Zhou, Z. Ma, "Ad Hoc Synchronization Considered Harmful," OSDI 2010. (ad hoc synchronization is common and error-prone)
- S. Lu, S. Park, E. Seo, Y. Zhou, "Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics," ASPLOS 2008. (order violations as a major non-deadlock bug class)
- H.-J. Boehm, "Can Seqlocks Get Along with Programming Language Memory Models?," MSPC 2012, pp. 12–20. (seqlock readers vs data-race-free memory models)
- POSIX Base Definitions, "Memory Synchronization" (§4.12 in Issue 7): `pthread_create`, mutex lock/unlock and `sem_post`/`sem_wait` synchronize memory; an Austin Group defect report notes that nested recursive-mutex operations do not always synchronize. **Partly UNVERIFIED:** confirmed only through secondary sources (austingroupbugs.net issue 1426, a GCC mailing-list quote), not the current Open Group text. Whether `mq_send`/`mq_receive` are in the list is **UNVERIFIED**.

---

## 11. Open questions

1. **RESERVED window reachability (§1.6).** Can a new main task pass `GetTaskFunction` before `es/cfe_es_apps.c:869` without changing the 50 ms delay, e.g. under heavy load on 4 vCPUs with `SCHED_OTHER`, or with a slow `CFE_ES_LoadModule` for the *next* app? (Load happens before `StartAppTask`, so it does not widen this app's window; whether something else does is open.) This needs a derived experiment, or a model check with the delay removed.
2. **TBL concurrent load (§4.4).** The code shows the owner path checks and claims `LoadInProgress` without the registry lock (`tbl/cfe_tbl_api.c:305-309`, `tbl/cfe_tbl_load.c:778-813`, `tbl/cfe_tbl_internal.c:425-448`). Is the owner-load vs ground-load interleaving reachable with any app in the pinned set (does any app call `CFE_TBL_Load` after init, e.g. on a command)? Is it already reported upstream? Not searched.
3. **TBL validation flag handshake (§4.4, R13).** Should the detector treat unlocked `State`/`Result` flags as acquire/release (assuming x86 TSO + `-O0`), or report them? This is a policy decision for the paper's memory-model assumption.
4. **Which apps in the pinned set rely on cross-app by-name lookups and table sharing**, and do they place publish/consume on opposite sides of `WaitForSystemState`? This is out of scope here and belongs to the app-pattern report.
5. **Timebase "cFS-Master" on the native PSP.** Does it exist (so the local 1 Hz comes from the OSAL timer thread), or does the PSP drive `CFE_TIME_Local1HzISR` some other way? This determines the context in which `Local1HzISR` runs; see report 01.
6. **OSAL queue and task semantics** relied on in R2/R7/R17: whether the new thread can run before `OS_TaskCreate` returns, whether `OS_QueuePut`/`Get` are FIFO, and what `OS_TaskDelete` does to a victim holding the recursive ES mutex (`es/cfe_es_api.c:1383` deletes under the ES lock). Report 01 should confirm.
7. **Restart/reload scenarios.** How many apps in the pinned set re-run init steps that depend on other apps' resources, given that restarts have no barrier?

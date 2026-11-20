# 03 — How the 15 bundled cFS apps depend on each other and on cFE/OSAL (init and steady state)

- Date: 2026-10-12. Policy: `/home/user/Shape-Perf/research/cfs_procrace/CONDITIONS_POLICY.md`. Read-only survey: nothing was built, run, installed or changed.
- **Tree.** `/home/user/work/procrace/cfs_ref/cFS` = nasa/cFS `088b2fa`. Submodules checked out clean at: cfe `c5fb2b4d`, osal `d2d877a6`, psp `c4b3b0b6`, cf `15a871e6`, ci_lab `f5d36625`, cs `8510c5c0`, ds `c31e982e`, fm `ed51c573`, hk `766dab15`, hs `947b9031`, lc `a1c3a47e`, md `7e027886`, mm `f3ee4e67`, sample_app `2f93d1a4`, sbn `3b7b3757`, sc `76db5f7b`, sch_lab `bc848cd8`, to_lab `38f7312e`, sample_lib `2d8b7e86`. I checked each with `git rev-parse` and `git status --porcelain` (0 changes). `apps/sbn_udp` and `apps/sbn_f_remap` are symlinks into `apps/sbn/modules/...`.
- **Citations.** `path:line` is relative to the cFS root above, at these revisions. "Default config" means the tables and configuration that the existing build `build-native_std` actually uses. Where `sample_defs/tables/*.c` overrides an app's own default table, I say so. The build includes `CFS_SBN_ENABLED` for the SCH_LAB and TO_LAB tables (`sample_defs/cpu1/install_custom.cmake:24-37`, and the flag is present in `build-native_std/.../tblobj_cpu1_sch_lab.../flags.make`).
- **Labels.** **Fact (code)** = what the code says. **Fact (measured)** = taken from the existing run records (`env/BASELINE_MEASURE.md`, `env/CONDITIONS.md`, `env/logs/*.log`). **Inference** = my reading, which has not been executed. Paragraphs without a label are code facts.
- **Counting rule** (used in §4). A *site* is one call site in an app's `fsw/src` (or `sbn/modules/*/fsw/src`); unit tests, stubs and `sbn/examples` are excluded. *Active* means the contract is exercised in the default config with no ground command. *Latent* means the code is present but the default table or config disables it, or a command is needed to trigger it.

---

## 0. Summary

1. Bundled apps have **many cross-app readiness dependencies, but very little explicit synchronization**. Only 4 of 15 apps call an ES phase wait (TO_LAB, HS, CS, SBN; 5 sites). The rest become "RUNNING" implicitly on their first `CFE_ES_RunLoop` call (`cfe/modules/es/fsw/src/cfe_es_api.c:469-471`), whether they are ready or not.
2. ES does not wait for an app's init before it creates the next app. Each new main task sleeps 50 ms (`cfe_es_apps.c:571-575`), and all app inits then overlap. Script priorities have no effect in this environment (all threads run SCHED_OTHER; `env/CONDITIONS.md` E4, M1-5). ES's own barrier is **soft**: at most 1 s to reach LATE_INIT plus 1 s to reach RUNNING, after which it goes OPERATIONAL anyway (`cfe_es_start.c:210-234`; `cfe_es_internal_cfg.h:878-879`). An app that exits on an init error is STOPPED, and STOPPED counts as past the barrier (`cfe_es_api.c:399`; `cfe_es_start.c:880-886`).
3. The dominant contract is **MsgId subscription-before-first-send**. A send to a MsgId with no subscriber returns **success to the sender** and is only counted (`cfe/modules/sb/fsw/src/cfe_sb_priv.c:1083-1086`). The code has 48 subscribe sites and 41 transmit sites. Six apps subscribe from tables, so the dependency graph depends on configuration.
4. I classify **15 kinds** of readiness contract (§4). The five known cases (C1, C2, C3, C5, R2) fall into **3** of them: named OSAL object lookup (C1), ES phase / readiness declaration (C2), and identity/registration publication (C3, C5, R2, which are all cFE-core-internal). **12 kinds are not covered by any case.** Three of those 12 already show **measured, run-to-run-varying losses in the default configuration** (§5.2).
5. The SCH_LAB schedule reaches its consumers only because of a **timing margin**: SCH_LAB's timer starts 1 s after its own init, and it sends nothing until the first TIME 1 Hz packet arrives. There is no synchronization. In 13 recorded boots, OPERATIONAL came 0.12 s to about 0.9 s before SCH_LAB's earliest possible first send (inference from log timestamps, §6.3).
6. Five app wakeups/requests are **not in the default schedule** (HS, LC sample, MD, HK combined packet, CI uplink), so those apps' periodic logic never runs, or runs only on SB timeouts (§6.2).

---

## 1. Framework behaviour that every app inherits (code facts)

| Topic | Fact | Cite |
|---|---|---|
| Phase sequence | `CORE_STARTUP` → core objects created → `CORE_READY` → `CFE_ES_StartApplications` (script) → wait for all apps ≥ LATE_INIT (soft) → `APPS_INIT` → wait for all apps ≥ RUNNING (soft) → `OPERATIONAL` | `cfe/modules/es/fsw/src/cfe_es_start.c:183,188,194,201,210,216,225,234` |
| Barrier budget | `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` = 1000 per phase. The doc comment says it is "a soft limit". Poll period is 50 ms. On timeout ES only logs "Startup Sync failed" and continues. | `cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h:859-860,863-879`; `cfe_es_start.c:210-213,225-228`; `cfe_es_start.c:860-918` (`CFE_ES_MainTaskSyncDelay`) |
| Core barrier | Core tasks are serialized with a 30 s hard limit (`CFE_PLATFORM_CORE_MAX_STARTUP_MSEC`) | `cfe/modules/core_private/config/default_cfe_core_private_internal_cfg.h:68`; `cfe_es_start.c:835` |
| App creation | Script lines are processed sequentially. `CFE_LIB` init functions run **synchronously** in the ES main context. `CFE_APP` creates a task and returns at once, with no wait for the app's init. | `cfe_es_apps.c:427,436` (dispatch), `cfe_es_apps.c:1013-1016` (lib init call), `cfe_es_apps.c:849-902` (AppCreate, no wait) |
| New-task start | The entry wrapper delays 50 ms, then polls until the ES task record has an AppId and entry function, with a 1 s budget (the C5-era guard) | `cfe_es_apps.c:559-604,613-640` |
| Implicit RUNNING | `CFE_ES_RunLoop` increments the ExecutionCounter and raises the caller's AppState to RUNNING. If the caller's `RunStatus` is not APP_RUN (an init error), it returns false at once and does not set RUNNING. | `cfe_es_api.c:441,453-456,469-471` |
| Failed app passes the barrier | `CFE_ES_ExitApp` sets an external app's AppState to STOPPED (5), which is above RUNNING (3). The barrier counts only records with a state below the required one, so **an app that exits on an init error satisfies the OPERATIONAL barrier**. An app whose main function simply `return`s (no `ExitApp`) stays in EARLY_INIT, and the barrier waits until it times out. | `cfe_es_api.c:399`; `cfe/modules/es/config/default_cfe_es_extern_typedefs.h:269-289`; `cfe_es_start.c:880-886` |
| Phase wait | `CFE_ES_WaitForSystemState` first raises the caller's own AppState (RUNNING for OPERATIONAL, LATE_INIT for APPS_INIT), then polls `SystemState` every 50 ms until the timeout. `CFE_ES_WaitForStartupSync` is the same call for OPERATIONAL and **returns void**, so its timeout is invisible to the caller. | `cfe_es_api.c:514-607,616-619` |
| SB no subscriber | `NoSubscribersCounter++`, event `SEND_NO_SUBS` (filtered after the first 4), status **CFE_SUCCESS** | `cfe_sb_priv.c:1083-1086`; filter cap measured in `env/BASELINE_MEASURE.md:186` |
| SB queue full | `PipeOverflowErrorCounter++` and event `Q_FULL_ERR` (filtered after 16). Transmit to a pipe is non-blocking. | `cfe_sb_priv.c:1176-1194`; `env/BASELINE_MEASURE.md:136-139` |
| SB subscription report | When reporting is enabled, **every** global subscribe transmits a `CFE_SB_ONESUB_TLM_MID` packet from the subscriber's own task | `cfe_sb_api.c:1192-1195`; `cfe_sb_task.c:651-675` |
| Queue depth (env) | The kernel truncates every queue to 10 in README mode, and 22 of 26 pipes are truncated | `env/CONDITIONS.md` E5; `env/BASELINE_MEASURE.md:11` |
| TIME 1 Hz | TIME looks up the PSP timebase `"cFS-Master"`, adds a 1 Hz timer (first expiry +0.5 s, then every 1 s), and a child task transmits `CFE_TIME_ONEHZ_CMD_MID` | `cfe/modules/time/fsw/src/cfe_time_task.c:311-318`; `cfe_time_tone.c:1254-1290`; `cfe_time_utils.c:359-361` |
| PSP timebase | `"cFS-Master"` is created by the PSP `soft_timebase` module | `psp/fsw/inc/cfe_psp_timertick_api.h:53`; `psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase.c:53` |

---

## 2. Per-app survey

Each subsection lists the init sequence in program order, the cross-app resources the app consumes or produces, its waits, retries and timeouts, and what happens when a resource is not ready. "Pipe d=N" is the requested depth. In this environment the effective depth is min(N, 10).

### 2.1 SCH_LAB (`apps/sch_lab/fsw/src/sch_lab_app.c`)

- **Init (`SCH_LAB_AppInit`, L153):**
  - count sem `"SCH_LAB"` created (L168);
  - **`OS_TimeBaseGetIdByName("cFS-Master")`** (L176);
  - `OS_TimerAdd("SCH_LAB", …, SCH_LAB_LocalTimerCallback)` (L184);
  - `CFE_TBL_Register("Schedule")` (L194) and `CFE_TBL_Load` from `/cf/sch_lab_table.tbl` (L211; `sch_lab/fsw/inc/sch_lab_internal_cfg.h:42`);
  - the command headers are copied out of the table, then the table is released (L224-287);
  - `CFE_SB_CreatePipe("SCH_LAB_CMD_PIPE", 8)` (L290), then subscribe `CFE_TIME_ONEHZ_CMD_MID` (L296);
  - **`OS_TimerSet(…, 1000000, TimerPeriod)`** (L303): the first tick is 1 s after init.
- **No EVS registration and no phase wait** anywhere in the app.
- **Steady state:**
  - the main task pends on the timing semaphore (L98);
  - each tick it polls its pipe for the 1 Hz packet (L102);
  - the table is processed only once **at least one 1 Hz packet has been received** (`SCH_OneHzPktsRcvd > 0`, L116);
  - it then sends each due entry with `CFE_SB_TransmitMsg` (L131).
- **Timer-callback → task handoff:** the callback gives the semaphore (L143-146).
- **Cross-app resources:**
  - consumes the PSP timebase (by name) and the TIME 1 Hz MsgId;
  - produces every scheduled MsgId (§6).
- **Not-ready behaviour:**
  - Timebase missing, or a table error: init returns an error, `RunStatus = APP_ERROR` (L85-90), and the app exits.
  - Pipe or subscribe failure: `OS_printf` only (L291-300). The 1 Hz gate then never opens, and SCH_LAB **silently sends nothing** (inference from L116).

### 2.2 CI_LAB (`apps/ci_lab/fsw/src/ci_lab_app.c`)

- **Init (`CI_LAB_TaskInit`, void, L103):**
  - EVS register (L113);
  - pipe `"CI_LAB_CMD_PIPE"` d=32 (L119);
  - subscribe CMD (L122), SEND_HK (L131), READ_UPLINK (L140);
  - UDP socket open/bind on port 1234 + ProcessorId − 1 (L157-184);
  - delete handler (L192).
- **No phase wait.** Errors are logged as events but never stop the app, because the init function is void.
- **Steady state:**
  - receive with a 500 ms timeout (L68; `ci_lab/fsw/inc/ci_lab_internal_cfg.h:57`);
  - when not "Scheduled", it reads the uplink every loop (L78-81);
  - each ingested packet is sent with `CFE_SB_TransmitBuffer` (L267) to **whatever app subscribes** to that MsgId.
- **Not-ready behaviour (inference from §1):** an uplinked command for an app that has not subscribed yet is dropped with a success status. CI_LAB counts it as `IngestPackets++` (L266).

### 2.3 TO_LAB (`apps/to_lab/fsw/src/to_lab_app.c`)

- **Init (`TO_LAB_init`, L111):**
  - EVS register (L134);
  - `CFE_TBL_Register("Subscriptions", validate)` (L149) and load `/cf/to_lab_sub.tbl` (L167);
  - GetAddress (L181);
  - command pipe d=8 (L198); subscribe CMD and SEND_HK (L211-212; return values ignored);
  - TLM pipe d=`OS_QUEUE_MAX_DEPTH` (L215).
- **Phase wait:** `CFE_ES_WaitForStartupSync(TO_LAB_STARTUP_SYNC_TIMEOUT = 10000)` (L67; `to_lab/fsw/inc/to_lab_internal_cfg.h:79-80`) runs **only if init succeeded**. The in-code comment says this deliberately defers telemetry subscription until OPERATIONAL (L64-66). After the wait comes `TO_LAB_UpdateSubscriptionsFromTable` (L68 → `CFE_SB_SubscribeEx` per entry, L477).
- **Default table:** `sample_defs/tables/to_lab_sub.c:54-102`, **37 MsgIds**:
  - 3 lab;
  - 11 core (including `CFE_EVS_LONG_EVENT_MSG_MID`);
  - 18 app (including `CF_CH0/1_TX_MID`);
  - 5 SBN.
- **Steady state:**
  - drain the TLM pipe with a 50 ms timeout (L317; cfg L50-51);
  - poll commands (L262);
  - `TO_LAB_ManageTables` every loop (L534-560). A table update re-subscribes everything (L468, unsubscribe-all, then resubscribe);
  - `AddPacket` subscribes by command (`to_lab_cmds.c:220`).
- **Cross-app:** TO_LAB consumes telemetry and events from every app and from the core. Its subscription burst also produces SB subscription reports for SBN and DS (§2.15, §4 RC13).
- **Not-ready behaviour:**
  - Before OPERATIONAL, all telemetry and events have no TO_LAB subscriber: no-subscriber drop, by design.
  - If init fails, TO_LAB never calls the startup sync. RunLoop returns false at once, and `CFE_ES_ExitApp` marks it STOPPED, which counts as "ready" for the barrier (§1).

### 2.4 SAMPLE_APP (`apps/sample_app/fsw/src/sample_app.c`)

- **Init (`SAMPLE_APP_Init`, L111):**
  - EVS register (L124);
  - pipe d=32 (L141);
  - subscribe SEND_HK (L158) and CMD (L173);
  - `CFE_TBL_Register("ExampleTable")` (L188) and load `/cf/sample_app_tbl.tbl` (L202).
- No phase wait.
- **Steady state:** `CFE_SB_PEND_FOREVER` (L77), so ExecutionCounter advances only per message.
- **Cross-app:**
  - library call `SAMPLE_LIB_Function()` on a command (`sample_app_cmds.c:143`). The library initializes synchronously, earlier in the script (§1);
  - its table `SAMPLE_APP.ExampleTable` is named in CS's tables table, disabled there (§2.14).
- **Not-ready behaviour:** an init error gives APP_ERROR, and the app exits.

### 2.5 LC (`apps/lc/fsw/src/lc_app.c`)

- **Init (`LC_AppInit`, L187):**
  - EVS (L254);
  - pipe d=12 (L284);
  - subscribe SEND_HK (L298), CMD (L314), SAMPLE_AP (L330);
  - `LC_TableInit` (L350):
    - result tables WRT/ART, `DUMP_ONLY` (L539-576);
    - CDS areas (L786, L822, L858) **only if `LC_SAVE_TO_CDS`**, which is commented out (`lc/fsw/inc/lc_internal_cfg.h:101-102`), so latent;
    - definition tables WDT/ADT, CRITICAL only with CDS (L613-760);
    - load defaults (L917, L956);
  - `LC_CreateHashTable` → **one `CFE_SB_Subscribe` per watchpoint MsgId taken from the WDT** (`lc_watch.c:120-150,225`).
- No phase wait.
- **Default WDT/ADT:** all entries unused; the examples are commented out (`lc/fsw/tables/lc_def_wdt.c:48-76`, `lc_def_adt.c:53-122`).
- **Steady state:** receive with a 1000 ms timeout (L101; `lc_app.h:95`). On timeout it runs `LC_PerformMaintenance` (L116 → `lc_utils.c:176-202`: table manage and CDS update only, no AP sampling).
- **Cross-app:**
  - consumes watchpoint telemetry from any app (table-driven);
  - **produces an RTS request to SC**: `LC_ExecuteRTS` sends `LC_RTS_REQ_MID` 0x18A9, CC 4 (`lc_custom.c:42-53`; `lc_interface_cfg.h:57-58,76-77`). 0x18A9 = `SAMPLE_CPU1_CMD_MID_BASE` 0x1800 | SC topic 0xA9 = `SC_CMD_MID`, and CC 4 = `SC_START_RTS_CC` (`sample_defs/inc/global_core_api_base_msgid_values.h:30`; `sc/fsw/inc/sc_topicids.h:34`; `sc/fsw/inc/sc_fcncodes.h:196`). This is an interface agreement fixed at compile time, and a subscription-before-first-send contract with SC.
- **Not-ready behaviour:**
  - A watchpoint packet that arrives before LC subscribes is simply not seen.
  - An RTS request sent before SC subscribes is dropped with success (inference).
  - A table failure gives APP_ERROR.

### 2.6 CF (`apps/cf/fsw/src/cf_app.c`, `cf_cfdp.c`)

- **Init (`CF_AppInit`, L208):**
  - EVS (L221);
  - pipe d=32 (L228);
  - subscribe CMD, SEND_HK, WAKE_UP (L243);
  - `CF_TableInit`: register (L146), load (L160), GetAddress (L184);
  - `CF_CFDP_InitEngine` (`cf_cfdp.c:1217`):
    - `OS_mkdir(tmp_dir)` (L1239);
    - per channel: pipe (L1244) and `CFE_SB_SubscribeLocal(mid_input)` from the table (L1257);
    - **if `sem_name` is non-empty: `OS_CountSemGetIdByName`, with a retry of 25 × `OS_TaskDelay(100)` on `OS_ERR_NAME_NOT_FOUND` only** (L1270-1303; `cf_internal_cfg.h:172-185`).
  - The in-code comment reads: "There is a start up race condition because CFE starts all apps at the same time" (L1272-1279).
- No phase wait.
- **Default:** `sem_name` is `""` on both channels (`cf/fsw/tables/cf_def_config.c:60,78`), so the lookup does not run (latent; this is C1).
- **Steady state:**
  - receive with a 100 ms timeout (L301; `cf_internal_cfg.h:160-161`);
  - throttle check `OS_CountSemTimedWait(sem_id, 0)` before each PDU (`cf_cfdp_sbintf.c:89-91`);
  - PDUs go out on `CF_CH0/1_TX_MID`, which TO_LAB subscribes only after OPERATIONAL;
  - **file handoff check** `OS_FileOpenCheck(src)` before a transmit. The comment calls it "a race condition (toctou)" (`cf_cfdp_s.c:447-463`);
  - the polling directory is disabled by default (`cf_def_config.c:55`).
- **Not-ready behaviour:**
  - Sem lookup failure after retries: event `CF_INIT_SEM_ERR_EID`, the engine stays disabled, init returns an error, and the app exits.
  - **Inference:** the retry window (up to 2.5 s) exceeds ES's 2 × 1 s soft barrier, so the system could go OPERATIONAL while CF is still retrying.
  - A file still open gives `FILESTORE_REJECTION`.

### 2.7 DS (`apps/ds/fsw/src/ds_app.c`, `ds_table.c`)

- **Init (`DS_AppInitialize`):**
  - EVS (L201);
  - pipe d=45 (L209);
  - subscribe SEND_HK (L224) and CMD (L240);
  - `DS_TableInit` (L256):
    - register DestFile and Filter tables (`ds_table.c:70,111`). CRITICAL only if `DS_MAKE_TABLES_CRITICAL`, default 0 (`ds_table.c:54-56`; `ds_internal_cfg.h:236`);
    - load both tables (`ds_table.c:158,172`);
    - `DS_TableManageFilter` (L188) → **`DS_TableSubscribe` → one `CFE_SB_SubscribeEx` per filter MsgId** (`ds_table.c:334,862-890`);
  - `DS_TableCreateCDS` (L264 → `ds_table.c:926-995`: RegisterCDS, CopyToCDS or RestoreFromCDS).
- No phase wait.
- **Default filter:** 14 MsgIds, including `CFE_EVS_LONG_EVENT_MSG_MID`, `CFE_SB_ALLSUBS_TLM_MID` and `CFE_SB_ONESUB_TLM_MID` (`ds/fsw/tables/ds_filter_tbl.c:86-190`). All destination files are disabled (`ds_file_tbl.c:82-118`).
- **Steady state:** receive with a 1000 ms timeout (L101; `ds_app.h:49`); on timeout, table manage (L123-124). **Every** packet and event goes to the single command pipe (effective depth 10).
- **Not-ready behaviour:**
  - Events sent before DS subscribes never reach DS.
  - Startup bursts overflow `DS_CMD_PIPE`: measured ← HS and ← SBN (`env/BASELINE_MEASURE.md:148-175`).
  - CDS errors are non-fatal (`ds_table.c:975-990`).

### 2.8 FM (`apps/fm/fsw/src/fm_app.c`, `fm_child.c`)

- **Init (`FM_AppInit`, L165):**
  - EVS (L177);
  - pipe d=10 (L186);
  - subscribe SEND_HK (L198) and CMD (L215);
  - `FM_TableInit` (L228 → `fm_table_utils.c:49` register; L58 load, "OK if this fails");
  - **`FM_ChildInit()` with its return value ignored** (L241) → count sem `"FM_CHILD_SEM"` (`fm_child.c:65`), mutex `"FM_QUEUE_SEM"` (L75), child task `"FM_CHILD_TASK"` at priority 205 (L86);
  - compression init (L255).
- No phase wait.
- **Child-task handoff (intra-app, active):**
  - the parent writes `ChildQueue[ChildWriteIndex]`, increments `ChildQueueCount` under the mutex, and gives the semaphore (`fm_cmd_utils.c:435-506`);
  - the child takes the semaphore and checks `ChildQueueCount` **without the mutex** (`fm_child.c:154-162`), then decrements it under the mutex (L286-288);
  - the parent also reads the count without the mutex (`fm_cmd_utils.c:435`);
  - the parent refuses work if `ChildSemaphore` is undefined (L438).
- **Cross-app:** `FM_VerifyFileClosed` scans **all apps'** open files through `OS_ForEachObject(OS_OBJECT_CREATOR_ANY)` (`fm_cmd_utils.c:103,180,364`; used at `fm_cmds.c:317,418,472,478`). This is the same TOCTOU kind as CF's check.
- **Not-ready behaviour:**
  - If child init fails, init still reports success. Child commands are then rejected because the semaphore is undefined.
  - A file open elsewhere causes the command to be rejected.

### 2.9 HK (`apps/hk/fsw/src/hk_app.c`, `hk_utils.c`)

- **Init (`HK_AppInit`, L132):**
  - EVS (L144);
  - pipe d=40 (L152);
  - subscribe SEND_COMBINED_PKT (L163), SEND_HK (L175), CMD (L187);
  - `CFE_ES_PoolCreate` (L199);
  - `HK_TableInit` (L212): register copy and runtime tables (L246, L262), load the copy table (L276), GetAddress (L306, L317);
  - `HK_ProcessNewCopyTable` → **one `CFE_SB_Subscribe` per input MsgId** (`hk_utils.c:322`).
  - The init status **is the return value of the init `CFE_EVS_SendEvent`** (L220-233), so a squelched or failed event fails HK init (intra-app quirk).
- No phase wait.
- **Default copy table:** the 5 cFE core HK MsgIds (`hk/fsw/tables/hk_cpy_tbl.c:39-64`).
- **Steady state:**
  - receive with a 1000 ms timeout (L79; `hk_app.h:52`); on timeout, check the tables (L97-100);
  - a combined packet is sent only on `HK_SEND_COMBINED_PKT_MID`, which is **not in the default schedule** (§6.2);
  - missing input gives `MissingDataCtr++` and a DEBUG event, and the packet is still sent because `HK_DISCARD_INCOMPLETE_COMBO` = 0 (`hk_utils.c:483-499`; `hk_internal_cfg.h:69`).
- **Not-ready behaviour:** an input that has not arrived when the combined packet is requested is reported as "missing data" (a first-data-before-first-request contract).

### 2.10 HS (`apps/hs/fsw/src/hs_app.c`, `hs_monitors.c`, `hs_cmds.c`, `hs_sysmon.c`)

- **Init (`HS_AppInit`, L211):**
  - EVS (L238);
  - **CDS** `RegisterCDS` (L248); if it already exists, `RestoreFromCDS` (L256) with a complement check (L260-261);
  - `HS_SbInit`: pipes CMD d=12, EVENT d=32, WAKEUP d=1 (L372, L383, L394); subscribe SEND_HK, CMD, WAKEUP (L405, L416, L427);
  - `HS_TblInit`: register AMT/EMT/MAT/XCT (L457-512); load XCT/AMT/EMT/MAT (L524, L540, L555, L570). A failed load disables that monitor;
  - **`HS_SysMonInit`** (L333) → **`CFE_PSP_IODriver_FindByName("linux_sysmon")`** (`hs_sysmon.c:51`; `hs_internal_cfg.h:333`).
- **Phase wait:** `CFE_ES_WaitForStartupSync(HS_STARTUP_SYNC_TIMEOUT = 65000)` (L75; `hs_internal_cfg.h:210-211`), **only if init succeeded**. Then the watchdog is armed (L80-83), and, if EventMon is enabled, it subscribes to long and short events (L90, L107). EventMon is **disabled by default** (`hs_internal_cfg.h:237`).
- **Init failure does not set `RunStatus` to error.** The loop runs anyway (L68-75, L128). This is measured here: HS init stops at SysMonInit because `/proc/schedstat` is missing, and the loop keeps running (`env/CONDITIONS.md` D-3, M1-6).
- **Steady state:**
  - receive on the wakeup pipe with a 1200 ms timeout (L145; `hs_internal_cfg.h:86`). `HS_WAKEUP_MID` is not scheduled, so HS runs on the timeout;
  - **AppMon:** `CFE_ES_GetAppIDByName` + `GetAppInfo` per AMT entry (`hs_monitors.c:69-73`). A **name not found is treated like "no progress"**: the countdown decrements and, at 0, the action runs, which can be `ResetCFE`, `RestartApp` or `DeleteApp` (L75-160);
  - EventMon actions use `GetAppIDByName` (L305, L331);
  - **XCT HK:** `CFE_ES_GetTaskIDByName` + `GetTaskInfo` (`hs_cmds.c:148-152`) and `GetGenCounterIDByName` (L161);
  - MAT sends commands to `CFE_TBL_CMD_MID` and `CFE_ES_CMD_MID` (`hs/fsw/tables/hs_mat.c:69-78`; `hs_monitors.c:202,373`).
- **Defaults:** AMT and EMT entries are all `NOACT` (`hs_amt.c:36-96`, `hs_emt.c:42-50`). XCT has the 5 core task names (`hs_xct.c:36-44`), so it is active.
- **Not-ready behaviour:** see AppMon above. An app that is created late, slow to start, or blocked in its own phase wait (it does not call RunLoop) would be counted as hung (inference). An XCT name not found gives `0xFFFFFFFF`.

### 2.11 MM (`apps/mm/fsw/src/mm_app.c`)

- **Init:**
  - EVS (L156);
  - pipe d=12 (L172);
  - subscribe SEND_HK (L185) and CMD (L198).
- No tables, no phase wait.
- **Steady state:** receive with a 1000 ms timeout (L84). On command, it peeks or pokes memory at **`OS_SymbolLookup(SymName)` + offset** (`mm_cmds.c:143`), after `CFE_PSP_MemValidateRange` (`mm_utils.c:142-209`). This gives cross-app memory access by symbol name. It is command-driven (latent).
- **Not-ready behaviour:** a symbol not loaded gives a command error event.

### 2.12 SC (`apps/sc/fsw/src/sc_app.c`, `sc_cmds.c`)

- **Init (`SC_AppInit`, L145):**
  - `AutoStartRTS` is chosen from **`CFE_ES_GetResetType`**: RTS 1 on power-on (L175-182);
  - EVS (L185);
  - pipe d=12 (L193);
  - subscribe SEND_HK, WAKEUP, CMD (L204, L215, L226);
  - `SC_InitTables` (L261): register dump-only and loadable tables (L383-465), load the default RTS files (L624), `CFE_TBL_NotifyByMessage(…, SC_CMD_MID, SC_MANAGE_TABLE_CC)` for each table (L652-700).
- No phase wait.
- **Steady state:**
  - receive with a 1000 ms timeout (L92; `sc_app.h:56`);
  - **the auto-start RTS fires on the first `SC_SEND_HK_MID`** (`sc_cmds.c:489-507`), so its timing is set by the SCH_LAB schedule (62 ticks);
  - RTS/ATS commands are transmitted at `sc_cmds.c:158,349` and `sc_rtsrq.c:660`;
  - the default RTS 1-4 and ATS 1 command **SC itself** (`sc/fsw/tables/sc_rts00[1-4].c`, `sc_ats1.c`). Cross-app targets are latent.
- **Not-ready behaviour:** an RTS command sent to an app that has not subscribed is dropped with success (inference from §1).

### 2.13 MD (`apps/md/fsw/src/md_app.c`, `md_utils.c`, `md_dwell_tbl.c`)

- **Init (`MD_AppInit`, L116):**
  - EVS (L130);
  - pipe d=50 (L219);
  - subscribe SEND_HK, CMD, WAKEUP (L226, L249, L265);
  - 4 dwell tables registered **`CFE_TBL_OPT_CRITICAL`** (L340-345). If `CFE_TBL_INFO_RECOVERED_TBL`, it is restored from CDS and validated; otherwise it is loaded (L425).
  - **Table validation resolves each entry's symbol with `OS_SymbolLookup`** (`md_dwell_tbl.c:258-280` → `md_utils.c:188-224`, lookup at L214).
- No phase wait.
- **Default:** all dwell tables are disabled with empty symbols (`md/fsw/tables/md_dw01.c:35-66`). Measured log line: "Dwell Tables Recovered: 0, Dwell Tables Initialized: 4" (`env/logs/m1_A1_po_fixed30s_esrestart.log:132`).
- **Steady state:**
  - receive with a 1000 ms timeout (L75; `md_app.h:65`);
  - dwell reads happen only on `MD_WAKEUP_MID`, **not scheduled**;
  - it reads raw memory at the address resolved at load time (`md_dwell_tbl.c:340`).
- **Not-ready behaviour:**
  - A symbol not yet loaded gives `MD_RESOLVE_ERROR` and the load is rejected. **Inference:** MD is the 13th app in the script, so a dwell table naming a CS or SBN symbol (both loaded later) could fail at boot, depending on load timing.
  - A resolved address becomes stale if the owning app is reloaded (inference).

### 2.14 CS (`apps/cs/fsw/src/cs_app.c`, `cs_init.c`, `cs_compute.c`)

- **Init (`CS_AppInit`, L154):**
  - EVS (L159);
  - pipe (`cs_init.c:50`);
  - subscribe SEND_HK, BACKGROUND_CYCLE, CMD (`cs_init.c:61,73,87`);
  - **CDS** register/restore states (`cs_app.c:247,262,274`), enabled because `CS_PRESERVE_STATES_ON_PROCESSOR_RESET` = true (`cs_internal_cfg.h:257-258`);
  - 4 definition and result tables. A file load **falls back to a compiled-in default** (`cs_table_processing.c:786,819,827,832`).
- **Phase wait:** `CFE_ES_WaitForStartupSync(CS_STARTUP_TIMEOUT = 60000)` (L77; `cs_internal_cfg.h:166-167`) is called **even if init failed** (L68-77).
- **Steady state:**
  - receive with a 1000 ms timeout (L86; `cs_app.h:86`);
  - on BACKGROUND_CYCLE (scheduled every 5 ticks) it checksums **other apps' code segments by name** (`CFE_ES_GetAppIDByName` / `GetLibIDByName` + `GetModuleInfo`, `cs_compute.c:307-317`) and **other apps' tables by name** (`CFE_TBL_GetInfo(Name)` L191, `CFE_TBL_Share(Name)` L222);
  - it refreshes a stale handle explicitly (L215-226).
- **Default:** the tables table, from the `sample_defs` override, names `SAMPLE_APP.ExampleTable` **DISABLED** (`sample_defs/tables/cs_tablestbl.c:37`). The app table is empty (`cs/fsw/tables/cs_apptbl.c:37-45`). Both are latent.
- **Not-ready behaviour:** not found gives `CS_ERR_NOT_FOUND` / an error event. In practice the OPERATIONAL wait orders CS's first by-name use after all apps are RUNNING, but only within ES's soft budget (§1).

### 2.15 SBN (`apps/sbn/fsw/src/sbn_app.c`, `sbn_subs.c`, modules)

- **Init (`SBN_AppMain`, L1579):**
  - EVS (L1587);
  - `CFE_ES_GetAppID` (L1590) and its own task info (L1601);
  - mutexes `"sbn_send_mutex"` (L1611) and `"sbn_conf_mutex"` (L1620);
  - pipe `"SBNCmdPipe"` d=20 (L1629);
  - subscribe `SBN_CMD_MID` (L1636).
- **Phase wait comes *before* the real init:** `CFE_ES_WaitForStartupSync(10000)` (L1647), then `LoadConfTbl` (L1650 → L1413 register, L1420 load), then `Init()` (L1657):
  - `LoadConf` → for each module, `OS_SymbolLookup`; if it fails, **`OS_ModuleLoad("/cf/sbn_udp.so")`** and look up again (L1060-1088);
  - `InitInterfaces` (UDP socket bind, `sbn_udp_if.c:76-82`);
  - `SetupSubPipe`: pipe d=32 and `SubscribeLocal(ALLSUBS)` / `SubscribeLocal(ONESUB)` (L1465-1481);
  - a second wait, **`while (CFE_ES_WaitForSystemState(OPERATIONAL, 1000) == TIMED_OUT);`**, with no bound (L1536-1538);
  - `SBN_SendSubsRequests` (L1541), which **enables SB subscription reporting globally** and asks for the previous subscriptions (`sbn_subs.c:43-60`).
- The remap filter module registers its own table and mutex `"SBN_F_Remap"` (`sbn_f_remap.c:92-129,340`).
- **Steady state:**
  - the main loop holds `ConfMutex` around `WaitForWakeup(200 ms)` (L1663-1677; `sbn_platform_cfg.h:48`);
  - it drains the subscription pipe and polls peers (L1004-1057);
  - send/recv child tasks are created only for `SBN_TASK_SEND/RECV`, and the default is `SBN_TASK_POLL` (`sbn_conf_tbl.c:43,51`; `sbn_app.c:783,883,913`). Latent.
- **Not-ready behaviour:**
  - Errors before L1647 `return` from the main function **without `CFE_ES_ExitApp`** (L1588-1655). **Inference:** SBN's AppState then stays below LATE_INIT, and ES spends its full 2 × 1 s soft budget, which delays OPERATIONAL for TO_LAB, HS and CS.
  - Subscriptions made by other apps before L1541 are recovered through `SEND_PREV_SUBS`, a compensation mechanism.
  - Measured: SBN's init events are squelched by EVS (13 at S1), and its subscription pipe overflows in 5 of 6 M1 runs (`env/BASELINE_MEASURE.md:13,147-163`).

### 2.16 Libraries (script order, synchronous)

- `cfe_assert` creates the mutex `"CFE_Assert"` (`cfe/modules/cfe_assert/src/cfe_assert_init.c:120`).
- `sample_lib` fills a buffer (`libs/sample_lib/fsw/src/sample_lib.c:46-79`).
- Both run inside the ES main context before any `CFE_APP` line (§1). They are the only dependencies that the script order **guarantees**.

---

## 3. (a) Cross-app dependency table

"Existing ordering" means what, if anything, places the producer step before the consumer step at the pinned revision. "D" = active in the default config, "L" = latent.

| # | Producer app / resource | Consumer | Mechanism | Existing ordering | Failure behaviour if not ready | D/L |
|---|---|---|---|---|---|---|
| 1 | PSP `soft_timebase` → timebase `"cFS-Master"` | SCH_LAB (`sch_lab_app.c:176`), TIME (`cfe_time_task.c:311`) | OSAL name lookup | PSP module init precedes cFE start (inference; not traced) | SCH_LAB init fails and the app exits (L177-181) | D |
| 2 | TIME 1 Hz child → `CFE_TIME_ONEHZ_CMD_MID` | SCH_LAB (sub L296) | MsgId | Core before apps (`cfe_es_start.c:188-201`) | SCH_LAB sends nothing until the first 1 Hz arrives (L116) | D |
| 3 | SCH_LAB schedule, 18 app-targeted entries (§6) | CI_LAB, TO_LAB, SAMPLE_APP, CF, MD, HS, SC, FM, DS, LC, CS, MM, HK, SBN | MsgId | **None.** Timing margin only: the first tick is ≥ 1 s after SCH_LAB init (L303) | No-subscriber drop with success status (`cfe_sb_priv.c:1083-1086`); periodic, so the loss is transient | D |
| 4 | All apps + core: HK, events, CF PDUs (37 MIDs) | TO_LAB | MsgId (table) | TO_LAB subscribes after OPERATIONAL by design (`to_lab_app.c:64-68`) | Pre-OPERATIONAL traffic dropped (no subscriber); TLM pipe overflow measured in run A3 | D |
| 5 | EVS event packets from every app | DS (filter table), TO_LAB, HS (EventMon off) | MsgId | None (DS subscribes during its own init; DS is the 7th app) | Events before the subscription are not seen; `DS_CMD_PIPE` overflow measured ← HS, ← SBN | D |
| 6 | cFE core HK (5 MIDs) | HK (copy table) | MsgId (table) | Producers send only on SCH_LAB HK requests (≥ 40 ticks) | "Missing data" counter and DEBUG event; incomplete packet still sent (L483-499) | D (combined packet never requested) |
| 7 | 13 core / DS telemetry MIDs | DS (filter) | MsgId (table) | As in 6 | Drop or overflow as in 5 | D |
| 8 | SBN (global toggle) + every subscriber task → `ONESUB` / `ALLSUBS` | SBN SubPipe, DS | SB mode + MsgId | SBN toggles after OPERATIONAL; `SEND_PREV_SUBS` replays the missed ones | `SBNSubPipe` overflow in 5 of 6 M1 runs, varying by interleaving | D |
| 9 | Module files `sbn_udp.so`, `sbn_f_remap.so` | SBN | Loader (file + symbol) | Loaded on demand by SBN (`sbn_app.c:1060-1088`) | SBN init fails → APP_ERROR | D |
| 10 | Unidentified provider app: counting sem named in the CF table | CF (`cf_cfdp.c:1282`) | OSAL name lookup | **None.** Bounded retry 25 × 100 ms on NAME_NOT_FOUND only | Error event (L1295), engine not enabled (L1331), CF exits | L (C1) |
| 11 | ES app/task registry (all apps) | HS AppMon / EventMon / XCT | ES name lookup | None (per cycle) | AppMon: not-found counts as hung → Restart / Delete / ResetCFE after CycleCount (`hs_monitors.c:69-160`); XCT `0xFFFFFFFF` | XCT D; AppMon/EventMon L |
| 12 | ES app/lib registry | CS (code checksum) | ES name lookup | CS waits for OPERATIONAL (soft) | Error event, entry not computed | L |
| 13 | TBL registry: `SAMPLE_APP.ExampleTable` | CS | `CFE_TBL_Share` by name | CS waits for OPERATIONAL (soft) | `CS_ERR_NOT_FOUND`; stale handle refreshed (L215-226) | L |
| 14 | Any module's global symbols | MD (dwell), MM (peek/poke) | OSAL symbol lookup | MD resolves during its own init (13th app); MM on command | MD: table rejected (`MD_RESOLVE_ERROR`); MM: command error | L |
| 15 | LC → `SC_CMD_MID` 0x18A9 CC 4 | SC | MsgId (compile-time agreement) | None (LC is the 5th app, SC the 12th) | Dropped with success (inference) | L |
| 16 | HS MAT → `CFE_TBL_CMD_MID`, `CFE_ES_CMD_MID` | TBL, ES (core) | MsgId | Core subscribed before apps | — | L (event-triggered) |
| 17 | HS → ES `RestartApp` / `DeleteApp` / `ResetCFE` of a monitored app | Any app | ES control API | — | Restarts the target, which reopens every contract that target provides (inference) | L |
| 18 | `sample_lib` (library) | SAMPLE_APP | Linked symbol | Script order + synchronous lib init (`cfe_es_apps.c:1013-1016`) | — (guaranteed) | D |
| 19 | Any app's files (e.g. DS output) | CF (poll/tx), FM (file commands) | File system + OSAL open-file table | None; both use a check-then-act test | CF `FILESTORE_REJECTION` (`cf_cfdp_s.c:451-463`); FM command rejected | L |
| 20 | PSP device `"linux_sysmon"` | HS | PSP IODriver by name | PSP module init | HS init fails (measured, D-3); HS runs with no startup sync, watchdog or event monitor | D (fails here) |
| 21 | ES SystemState (barrier) | TO_LAB, HS, CS, SBN | Phase wait | ES main: 1 s + 1 s soft (`cfe_es_start.c:210,225`) | A timeout is invisible (void return); the app proceeds | D |
| 22 | CI_LAB uplink | Any app | MsgId | None | Dropped with success | L (needs ground) |
| 23 | Previous incarnation's CDS / critical tables | HS, DS, CS, MD (LC disabled) | ES CDS / TBL critical | Reset type | Restore failure → defaults; HS checks a complement | D (power-on: create) |
| 24 | TBL "notify by message" | SC, SBN (their own MIDs) | TBL → app MsgId | Each app subscribes before it registers the notification | — | D |

Not cross-app but adjacent (intra-app, task level):

| Producer → consumer | Mechanism | Ordering | Cite |
|---|---|---|---|
| FM parent ↔ FM child | Shared queue + count sem + mutex | Child created after sem and mutex | `fm_child.c:65-100,154-162`; `fm_cmd_utils.c:435-506` |
| SCH_LAB timer callback → main | Count sem | Sem (L168) → timer add (L184) → timer set (L303) | `sch_lab_app.c:98,143-146` |
| SBN main ↔ send/recv children | `SendMutex`, `ConfMutex`, peer pipes | Mutexes created first | `sbn_app.c:562-600,1611-1625`; latent (POLL) |

---

## 4. (b) Taxonomy of readiness-contract kinds, with counts

A **readiness contract** here means: a consumer step requires that a producer step (in another task or app, or an earlier incarnation) has already happened, and nothing in the code makes the order certain except as noted. Counts follow the rule given at the top. The classification and grouping are mine (**inference**). The counts are of code sites and are reproducible with `grep` at the pinned revisions.

| Id | Kind | Sites (active / total) | Instances | Existing protection seen |
|---|---|---|---|---|
| **A. Name-based resolution** | | | | |
| RC1 | Named OSAL/PSP object lookup (sem, timebase, device) | 2 / 3 | SCH_LAB timebase L176 (D); CF count sem `cf_cfdp.c:1282` (L); HS PSP device `hs_sysmon.c:51` (D, fails here) | CF: bounded retry. Others: fail init |
| RC2 | Named ES resource lookup at steady state (app/task/lib/counter → id → info) | 1 / 7 | HS `hs_monitors.c:69,305,331`, `hs_cmds.c:89,148,161`; CS `cs_compute.c:307/311` | Per-cycle lookup. **HS AppMon treats not-found as failure** |
| RC3 | Named table sharing | 0 / 1 (2 call sites) | CS `cs_compute.c:191,222` | CS phase wait + stale-handle refresh |
| RC4 | Symbol / module by name (loader namespace) | 1 / 3 | MD `md_utils.c:214`; MM `mm_cmds.c:143`; SBN `sbn_app.c:1065-1088` | SBN loads on demand; MD/MM fail |
| **B. Message-based** | | | | |
| RC5 | MsgId subscription-before-first-send | 48 subscribe sites / 41 transmit sites in 15 apps; 6 apps subscribe from tables (TO_LAB, LC, DS, HK, CF, SBN). Default active edges (§3 rows 2-8): SCH_LAB→18, TIME→1, →TO_LAB 37, →DS 14, →HK 5, SB reports→2 consumers | §3 rows 2-8, 15, 16, 22 | TO_LAB: deliberate post-OPERATIONAL subscribe. Otherwise none. **Sender always sees success** |
| RC13 | Global SB mode change by one app (subscription reporting) | 1 producer / 3 consumer sites | SBN `sbn_subs.c:43-60`; SBN `sbn_app.c:1473,1481`; DS `ds_filter_tbl.c:166,174` | `SEND_PREV_SUBS` replay |
| RC14 | Bounded-queue capacity vs startup burst (order × capacity) | 3 pipes measured overflowing out of 26 | `SBNSubPipe`, `DS_CMD_PIPE`, `TO_LAB_TLM_PIPE` (`env/BASELINE_MEASURE.md:147-175`) | Per-MsgId limits (`SubscribeEx`), but the kernel depth of 10 bites first |
| **C. Lifecycle / phase** | | | | |
| RC6 | Identity or registration publication before use | **0 app-level producer sites**; 3 core resources in the cases (SB AppId, EVS AppID, ES TaskTable) | C3, C5, R2 (core) | ES core serialization; C5 guard (`cfe_es_apps.c:559-604`) |
| RC7 | ES phase barrier / readiness declaration | 5 sites in 4 apps: `to_lab_app.c:67`, `hs_app.c:75`, `cs_app.c:77`, `sbn_app.c:1647,1536`; + 11 apps that are implicitly RUNNING through RunLoop | — | Soft 1 s + 1 s. **Declaration ≠ readiness:** SBN declares before its init; CS declares even after init failure; HS does not declare after failure but keeps running; an app that exits on error (STOPPED) also satisfies the barrier (`cfe_es_api.c:399`) |
| RC8 | Persistent state restore across reset (CDS, critical tables) | 4 apps active / 9 sites | RegisterCDS: `hs_app.c:248`, `ds_table.c:936`, `cs_app.c:247`, `lc_app.c:786,822,858` (L); critical tables: `md_app.c:340` (D), `lc_app.c:632,685` (L), `ds_table.c:54-56` (L) | HS complement check; others fall back to defaults |
| RC15 | Stale resolved reference after the owner restarts or reloads | 3 uncompensated / 5 | Uncompensated: CF sem id (`cf_cfdp.c:1282`, used at `cf_cfdp_sbintf.c:89-91`), MD address (`md_dwell_tbl.c:340`), SBN module symbols. Compensated: CS (`cs_compute.c:215-226`), HS (per-cycle lookups) | — (inference; not exercised) |
| **D. Task coupling inside an app** | | | | |
| RC9 | Parent ↔ child-task handoff | 1 app active (FM), 1 latent (SBN, 3 creation sites) | `fm_child.c:86`; `sbn_app.c:783,883,913` | Sem + mutex; the count is read without the mutex (`fm_child.c:162`, `fm_cmd_utils.c:435`) |
| RC10 | Timer/ISR callback → task | 1 active | SCH_LAB `sch_lab_app.c:143-146` | Creation order sem → timer → set |
| **E. Static linkage and files** | | | | |
| RC11 | Library before client | 1 active | `sample_app_cmds.c:143` | **Guaranteed** by synchronous lib init + script order |
| RC12 | File-object handoff (file closed/complete before another app uses it) | 0 active / 5 | CF `cf_cfdp_s.c:451`; FM `fm_cmds.c:317,418,472,478` (via `fm_cmd_utils.c:364`) | Check-then-act; **CF's comment calls it TOCTOU** |

Notes on the table:
- RC5 is counted from the consumer side, because the producer side is open: any app, CI_LAB uplink, or SC RTS. A per-MsgId census would need the table contents at run time, which is an argument for a config-aware extractor (inference).
- RC6 has no producer sites in the 15 apps. Every known instance sits in cFE core. Apps are exposed to it only as *callers* of SB/EVS APIs, and by the time apps start, SB has already published its AppId (core serialization). Inference: apps are not affected in the default build.
- Some instances carry over from one kind into others: HS AppMon (RC2) can trigger `RestartApp` (§3 row 17), which reopens RC1/RC4/RC15 contracts for the restarted app.

---

## 5. (c) Which kinds the five known cases cover, and which they do not

### 5.1 Mapping

Case facts are from `cases/ledger.md` §1a and §2. The kind assignment is mine.

| Case | Resource (ledger) | Kind | Layer |
|---|---|---|---|
| C1 CF #184 | OSAL counting sem looked up by table name | RC1 (+ bounded-retry fix) | App ↔ app |
| C2 cFE #198 | Readiness of a late-init app; ES exposed only RUNNING | RC7 (declaration ≠ readiness; fix added APPS_INIT / LATE_INIT) | ES phase protocol |
| C3 cFE #73 | SB AppId, EVS AppID, EVS registration among core tasks | RC6 | Core ↔ core |
| C5 cFE #72 | ES TaskTable registration vs. the newly created task | RC6 (registration publication, creator vs. created) | ES internal |
| R2 cFE #2663 | SB AppId read before SB publishes it | RC6 | Core ↔ core |

**Covered: 3 kinds (RC1, RC6, RC7). Not covered: 12 kinds** (RC2, RC3, RC4, RC5, RC8, RC9, RC10, RC11, RC12, RC13, RC14, RC15). RC11 is guaranteed by construction and is not a defect class; even without it, 11 kinds are uncovered.

### 5.2 Evidence that the uncovered kinds matter in this very build

Measured facts, in the default config, with no fault injected:
- **RC13 + RC14 + RC5:**
  - per-run queue-full drops were 33, 37, 4, 49, 45, 41, all at startup;
  - in run A3 "a different interleaving", `SBNSubPipe` peaked at 5 and did not overflow, while `TO_LAB_TLM_PIPE` did;
  - in the other runs `SBNSubPipe ← TO_LAB` overflowed (`env/BASELINE_MEASURE.md:12,147-163`);
  - the overflow happens about 47-49 ms after OPERATIONAL, when TO_LAB subscribes (`env/BASELINE_MEASURE.md:55`).
  - **Inference** (consistent with the code, not proven): the deciding order is SBN's `ENABLE_SUB_REPORTING` (`sbn_subs.c:43-51`) relative to TO_LAB's 37 `SubscribeEx` calls (`to_lab_app.c:477`). Subscriptions made after the toggle each emit an ONESUB packet into a 10-deep pipe (`cfe_sb_api.c:1192-1195`). Subscriptions made before it arrive later in batches through `SEND_PREV_SUBS`. None of the five cases involves this pair.
- **RC5:** "No subscribers" counter 15-26 per run, mostly EVS event packets (MsgId 0x808) sent before TO_LAB subscribes (`env/BASELINE_MEASURE.md:186`; `env/ENV.md:254`).
- **RC5 + RC14:** `DS_CMD_PIPE` overflows ← HS and ← SBN in several runs (`env/BASELINE_MEASURE.md:148-175`).
- **RC1 / RC7 / RC20-like platform readiness:** HS's PSP device lookup fails on every boot. HS then skips its startup sync and watchdog but keeps running (`env/CONDITIONS.md` D-3, M1-6).

**Inference for the tool:** a detector designed around the five cases (OSAL name lookups plus core identity publication) would miss the only order-dependent losses actually observed in the default bundle. The contract vocabulary must at least include SB subscription and transmit semantics (success on no subscriber), pipe capacity, the global SB modes, ES phase declarations, and name lookups in all four namespaces: OSAL, ES, TBL, and loader symbols.

---

## 6. (d) Startup script order, the SCH_LAB schedule, and how they interact

### 6.1 Startup script (the build's actual file)

`build-native_std/exe/cpu1/cf/cfe_es_startup.scr` (17 lines), generated by `sample_defs/generate_startup.cmake:5-24` from `cpu1_APPLIST` + `MISSION_GLOBAL_APPLIST` (`sample_defs/targets.cmake:89-107,133`):

| # | Type | Name | Prio | Stack |
|---|---|---|---|---|
| 1 | LIB | cfe_assert | 0 | 0 |
| 2 | LIB | sample_lib | 0 | 0 |
| 3 | APP | sch_lab | 35 | 32768 |
| 4 | APP | ci_lab | 40 | 32768 |
| 5 | APP | to_lab | 45 | 32768 |
| 6 | APP | sample_app | 50 | 32768 |
| 7-17 | APP | lc, cf, ds, fm, hk, hs, mm, sc, md, cs, sbn | 70 | 131072 |

- `sbn_udp` and `sbn_f_remap` are built but not in the script. SBN loads them itself after OPERATIONAL (§2.15).
- **Fact (code):** ES processes the lines sequentially. Libraries initialize synchronously. Apps are created without waiting for their init (§1).
- **Fact (measured):**
  - Priorities are not applied: all 27 threads run SCHED_OTHER with nice 0 (`env/CONDITIONS.md` E4, M1-5).
  - Init completions interleave out of script order. Example: SAMPLE_APP's init event comes before SCH_LAB's message, and TO_LAB's before CI_LAB's (`env/logs/m1_A1_po_fixed30s_esrestart.log:80-87`).
  - In the 12 warm-cache boots, all app inits completed within about 50 ms of SCH_LAB's init, and OPERATIONAL followed about 100 ms after SCH_LAB's init. No log shows "Startup Sync failed" (13 logs grepped: `m1_*`, `run01-07`).
- **Consequence (inference):** script order guarantees only "library before app" (RC11) and "created before" (task existence). It does **not** order any app's init step against another app's init step.

### 6.2 SCH_LAB schedule (as built)

The build uses `sample_defs/tables/sch_lab_table.c` (TickRate 10 → 100 ms ticks, L57), not the app's empty default (`apps/sch_lab/fsw/tables/sch_lab_table.c:35-53`). `CFS_SBN_ENABLED` adds two SBN entries. There are 23 entries:

| Rate (ticks) | Period | MsgId → consumer | Consumer's app # in script (1 = sch_lab … 15 = sbn) |
|---|---|---|---|
| 1 | 0.1 s | `CF_WAKE_UP_MID` → CF | 6 |
| 2 | 0.2 s | `SBN_CMD_MID` / `SBN_SCH_WAKEUP_CC` → SBN | 15 |
| 5 | 0.5 s | `CS_BACKGROUND_CYCLE_MID` → CS | 14 |
| 10 | 1.0 s | `SC_WAKEUP_MID` → SC | 12 |
| 40-48 | 4.0-4.8 s | ES, EVS, TIME, SB, TBL `SEND_HK` → core | core |
| 46 | 4.6 s | `SBN_CMD_MID` / `SBN_HK_CC` → SBN | 15 |
| 50-76 | 5.0-7.6 s | `SEND_HK` → CI_LAB, TO_LAB, SAMPLE_APP, CF, MD, HS, SC, FM, DS, LC, CS, MM, HK | 2-14 |

(`sample_defs/tables/sch_lab_table.c:59-83`)

**Not scheduled** (fact: absent from the table). Consequences are from the code cited:
- `HS_WAKEUP_MID`: HS runs on its 1200 ms timeout (`hs_app.c:145`).
- `LC_SAMPLE_AP_MID`: actionpoints are never sampled. Timeouts only do maintenance (`lc_utils.c:176-202`).
- `MD_WAKEUP_MID`: no dwell collection.
- `HK_SEND_COMBINED_PKT_MID`: no combined packets.
- `CI_LAB_READ_UPLINK_MID`: CI reads the uplink on its 500 ms timeout (`ci_lab_app.c:78-81`).

**Inference:** the LC, MD and HK steady-state producer→consumer paths are not exercised by a default boot. A dynamic tool needs configuration variation, such as alternate schedule and LC/MD tables, to reach them. The tables are inputs that change the dependency graph.

### 6.3 Interaction between script order and schedule

1. SCH_LAB is the first app created. Its earliest send is at the first tick (1 s after its init, `sch_lab_app.c:303`) that follows receipt of a TIME 1 Hz packet (`sch_lab_app.c:111-116`). That is at least 1 s and at most about 2 s after its init (inference: 1 Hz period).
2. The fast entries target the apps created **late** in the script: CF (app 6), SC (app 12), CS (app 14), SBN (app 15). There is no synchronization between SCH_LAB and any consumer's subscribe. The only thing that makes "consumer subscribed before first wakeup" hold is the 1 s initial timer delay compared with the total app-init time.
3. **Measured margin (inference from log timestamps; `OS_printf` lines carry no timestamp, so SCH_LAB init is approximated by the preceding stamped line):**

   | Boot | SCH_LAB init ≈ | SBN subscribes `SBN_CMD_MID` | OPERATIONAL | Earliest first send ≈ SCH init + 1 s | Margin after OPERATIONAL |
   |---|---|---|---|---|---|
   | A1 (first after VM reboot) | 20.685 | 21.511 | 21.561 | 21.685 | ~0.12 s |
   | 12 others | 20.48-20.51 (run03: 47.40) | +0.05 s | +0.10 s | +1.0 s | ~0.9 s |

   Source: `env/logs/m1_*_po_fixed30s_esrestart.log`, `env/logs/run0[1-7]_*.log` (lines "SCH Lab Initialized", "SBN subscribed", "CF Initialized", "entering OPERATIONAL"). The A1 times are cFE times in the `1980-012-14:03:` minute.
4. **Inference:** the margin disappears in any of these cases:
   - app init is slowed by a cold cache (A1 already cuts it to 0.12 s);
   - CF's semaphore retry (up to 2.5 s) or HS's or CS's long phase waits;
   - more apps are added to the script;
   - SCH_LAB is given a higher TickRate, or a shorter initial timer delay.

   The lost wakeups are benign for periodic consumers, but they are real RC5 violations, and they are invisible to the sender.
5. **HK-request entries (≥ 40 ticks)** land 4 s or more after SCH_LAB starts, which is after OPERATIONAL in every recorded boot. So HK telemetry starts after TO_LAB has subscribed. This too holds by margin, not by synchronization (inference).
6. **SC's auto-start RTS** is triggered by the first `SC_SEND_HK_MID`, 6.2 s after SCH_LAB starts (`sc_cmds.c:489-507`). The script position of SC does not matter; the schedule sets the timing.
7. **The post-OPERATIONAL burst:** TO_LAB (37 subscribes), SBN (load modules, set up the subscription pipe, enable reporting, request previous subscriptions) and CS (start checksumming) are all released by the same `SystemState` transition. Each polls it every 50 ms (`cfe_es_api.c:585-604`). The measured order-dependent losses happen in exactly this window (§5.2).

---

## 7. Implications for the detector design (inference)

- **The contract kinds are open-ended, and they live in four namespaces:**
  - OSAL object names (sem, queue, timebase);
  - ES names (app, task, lib, counter);
  - TBL names;
  - loader symbols;

  plus MsgIds and the file system. A fixed list taken from the five cases would cover 3 of 15 kinds.
- **The consumer↔producer graph is configuration-driven.** Six apps subscribe from tables. CF, MD, CS, HS and LC take names and symbols from tables. The schedule decides who wakes when. Static facts need the *built* tables (`build-native_std/exe/cpu1/cf/*.tbl`, sources in `sample_defs/tables` and `apps/*/fsw/tables`), not just C code.
- **The semantics the model needs** (all from §1):
  - SB transmit to no subscriber returns success;
  - queue full is silent to the sender;
  - `WaitForStartupSync` returns void and its timeouts are soft;
  - RunLoop implies RUNNING;
  - one app can toggle subscription reporting for everyone.
- **App states are not reliable readiness signals.** SBN, CS and HS each break the "RUNNING means ready" assumption differently (RC7). An app that failed and exited counts as past the barrier (STOPPED ≥ RUNNING). C2 is a historical instance of the same gap.
- **Some losses are observable without a debugger:** SB HK counters (`NoSubscribersCounter`, `PipeOverflowErrorCounter`), EVS squelch counters, and per-app counters. The existing M1 method already reads them (`env/BASELINE_MEASURE.md` §2.3). This is a cheap verification oracle for RC5, RC13 and RC14.
- The code itself documents two races: CF's startup comment (`cf_cfdp.c:1272-1279`) and its TOCTOU comment (`cf_cfdp_s.c:447-448`). These two in-code admissions are candidate ground truth for RC1 and RC12.

---

## 8. Terminology note (literature)

- "Order violation", meaning a required A-before-B order between threads that is not enforced, as distinct from "atomicity violation", is the term used by **S. Lu, S. Park, E. Seo, Y. Zhou, "Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics", ASPLOS 2008**.
  - Bibliographic data verified with WebSearch (Microsoft Research and cs.columbia.edu copies).
  - Its definition of the category was **not re-read in this task (UNVERIFIED here)**.
- Every readiness contract in §4 is an order requirement in that sense. The task-, app- and process-level granularity is this study's own framing.
- No other literature is cited.

---

## 9. Open questions

1. **PSP→cFE ordering for RC1 row 1.** I did not trace the order in which PSP modules initialize relative to `CFE_ES_Main`, so "the `cFS-Master` timebase exists before SCH_LAB looks it up" is an inference. It needs a read of `psp/fsw/pc-linux/src/cfe_psp_start.c`.
2. **SBNSubPipe attribution.** Is the run-to-run difference in `SBNSubPipe` overflow (A3 vs. the others) actually decided by SBN's `ENABLE_SUB_REPORTING` vs. TO_LAB's subscription burst? The plausible alternative is ALLSUBS batch timing. This needs an event-ordered trace of those two sites.
3. **The SCH_LAB margin.** Does it ever go negative in this environment, for example on a cold cache or with CF retries forced? If so, `NoSubscribersCounter` should rise for `CF_WAKE_UP_MID`, `SBN_CMD_MID` and `CS_BACKGROUND_CYCLE_MID`. This is not measured per MsgId: SB counts no-subscriber drops only in aggregate.
4. **SBN early-return cost.** When SBN returns from its main before L1647 (no `ExitApp`), does ES's 2 s barrier really time out and delay OPERATIONAL for TO_LAB, CS and HS? This is predicted from `cfe_es_start.c:860-918`, not executed.
5. **HS AppMon at boot.** With a non-trivial AMT (as in a flight config), would apps created late or blocked in a phase wait be counted as hung, given that not-found counts as no progress? It depends on `CycleCount` against the 1.2 s HS cycle. The defaults are all `NOACT`.
6. **Scope decision for the paper.** Should RC9/RC10 (intra-app task coupling) and RC12 (file TOCTOU) count as "process-level"? They are task- or app-level by the task's own definition, but they are not message- or ES-mediated.
7. **Should the taxonomy count capacity (RC14) as a readiness contract, or as a separate "order × capacity" dimension?** The measured losses need both an order and a bound.
8. **cpu2.** The build also has a cpu2 target, and SBN peers are configured for 127.0.0.1:3234-3236. Cross-instance contracts (peer subscription propagation) are out of scope here because no peer ran in any recorded boot (`env/BASELINE_MEASURE.md:130`).

Sources (web):
- [Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics (Microsoft Research page)](https://www.microsoft.com/en-us/research/?p=1123215)
- [cs.columbia.edu copy of the paper](https://www.cs.columbia.edu/~junfeng/08fa-e6998/sched/readings/concurrency-bugs.pdf)

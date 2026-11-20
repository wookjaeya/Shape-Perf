1. C1 (nasa/CF #184) is a real cross-application order violation on a named OSAL object: CF's main task looks up the counting semaphore "BP0_tsem" by name once during init, BP's main task creates it, and nothing orders the two calls.
2. Absent object: `OS_CountSemGetIdByName` returns `OS_ERR_NAME_NOT_FOUND` (-17); buggy CF (281a941) checks it, sends ERROR event 30, fails init and calls `CFE_ES_ExitApp(APP_ERROR)`; ES deletes the application; nothing heals it; BP creates the semaphore later in the same run.
3. The fix (833fdbb) is a bounded retry in the consumer (25 lookups, 100 ms apart, on -17 only): the dependence stays, is tolerated for about 2.4 s, and fails the same way beyond that (recorded under a hold).
4. Recorded natural runs always had the lookup first (pre: 8/8 exit; post: 8/8 fail once, then succeed); "create first" exists only in researcher-held runs.
5. The consumer's key comes from a table file loaded at run time, the publisher's from a compile-time literal; they meet only as string arguments at the OSAL boundary, so step 2a leaves the pair "key unresolved / undecidable".
6. Negative controls next to C1: BP's own lookup-then-create of the same name; SCH_LAB's same-shape lookup of "cFS-Master" (its publisher runs before any application task exists); TIME's tolerant lookup; CF's designed "no semaphore" paths; the fixed CF itself.
7. C2 (nasa/cFE #198) is not a publish/consume pair in any code on disk: it is an enhancement request for one more startup state, motivated by an order violation seen between private applications whose objects, calls and symptom are not disclosed.
8. C2 framework mechanics (verified): in cFE 6.4.2 one call, `CFE_ES_WaitForStartupSync`, both marks the caller RUNNING and waits for OPERATIONAL, so an application that must wait before finishing its init declares readiness too early; later cFE adds APPS_INIT / LATE_INIT / `CFE_ES_WaitForSystemState` (opt-in, soft timeouts), and the old call still declares RUNNING first.
9. C2 has no run, no profile and no 2a row; its application-level ground truth is unknown. The v7.0.1 tree does contain the C2 call shape (sync call followed by more initialization) in TO_LAB, HS and SBN; none is reported as a defect.
10. Framework vocabulary usable without case knowledge: OSAL "negative = error" rule, cFE severity bits, EVS event types, ES syslog lines for application exit and barrier timeout. The ES event for an error exit has type INFORMATION (EID 14), not ERROR.

# Mechanics cards C1 and C2 (phase 1 grounding dossier, 2026-11-11)

## 0. Conventions

Tags: **[F p:l]** line opened in this run (source or project document). **[F: case file p:l]** fact whose source tree is not on disk. **[M n]** measured in this run, command n in section 3. **[I]** inference. **[U]** unknown.

| Abbreviation | Path | Content |
|---|---|---|
| `R` | /home/user/Shape-Perf/research/cfs_procrace | research root; `C1.md`, `C2.md` = `R/cases/...` |
| `CFpre` | /home/user/work/expect_src/CF_281a941/fsw | CF buggy; same files as /home/user/work/procrace/c1_build/pre/src/cf [M1] |
| `CFpost` | /home/user/work/expect_src/CF_833fdbb/fsw | CF fixed; same files as c1_build/post/src/cf [M1] |
| `BP` | /home/user/work/expect_src/bp_b051eb0 | BP application; same files as c1_build/pre/src/bp [M1] |
| `B1` | /home/user/work/procrace/c1_build/bundle_1df262a | bundle of the 2a builds `c1_pre_tx`, `c1_post_tx`: cFE 6cccb3f, OSAL f34759d |
| `ES1`, `OS1` | `B1/cfe/modules/es/fsw/src`, `B1/osal/src/os` | |
| `V7`, `ES7` | /home/user/work/procrace/cfs_ref/cFS, `V7/cfe/modules/es/fsw/src` | cFS v7.0.1 (cFE c5fb2b4) |
| `S642` | /home/user/work/c5/src/642/cFE-6.4.2-OSS-release/cfe/fsw/cfe-core/src | cFE 6.4.2; same files as the tree of 2a build `c5_642` [M12] |
| `DEF1` | /home/user/work/procrace/c1_build/pre/1df262a/bpxfer_defs/default_platform_cfg.h | platform configuration of the C1 build |
| `W6`, `BM`, `AR`, `IA`, `CS` | `R/plan/` results_w6_20261029.md, C1_build_manifest.md, step2a_results_20261030.md, input_audit_20261030.md, case_setup_C2_C3_C5.md | result and plan documents |
| `REL1`, `REL2` | /home/user/work/redesign/step2a/tests_8c_fix/c1_pre_tx/NP/relations.txt, .../c1_post_tx/NP/relations.txt | 2a relations |

## 1. Card C1 — nasa/CF #184

### 1.1 Source and versions

| Item | Value | Tag |
|---|---|---|
| Issue | nasa/CF #184 "Possible race condition with creation of throttle sem (needs to exist before CF initializes)", label `bug`. Reference sentence: "the CF app expects that semaphore to be created _before_ it starts ... if that fails, CF aborts" | [F C1.md:84, 89] |
| Revision linked by the reporter | CF `2a024d8` (not on disk) | [F: case file C1.md:421] |
| Buggy tree on disk | CF `281a941`, git parent of the fix = `CFpre` | [F C1.md:422] |
| Fixed tree | CF `833fdbb` (merge `2ffbecc`) = `CFpost`. Flight code differs in 2 hunks of 2 files | [F C1.md:424] [M2] |
| Publisher application | nasa/bp `b051eb0` with bplib `d4c6db9` = `BP` | [F /home/user/work/procrace/c1_build/fetch_refs.record:26-27] |
| Framework of the builds | cFS `1df262a` (cFE `6cccb3f`, OSAL `f34759d`) = `B1`; second candidate cFS `1b0b338` (cFE `fef4a0d`, OSAL `b44f4ce`) | [F fetch_refs.record:2-7] |
| 2a builds | `c1_pre_tx` = c1_build/pre/1df262a, `c1_post_tx` = c1_build/post/1df262a, bundle root `B1` | [F .../c1_pre_tx/NP/args.txt:4-5; .../c1_post_tx/NP/args.txt:4-5] |
| cFS v7.0.1 | CF `15a871e` has the fixed loop (fsw/src/cf_cfdp.c:1270-1295) but its default table names are "" (fsw/tables/cf_def_config.c:60, 78): the lookup never runs there | [F /home/user/work/expect_src/CF_15a871e, those lines] |

### 1.2 Actors

| Actor | Created by | Path:line | Tag |
|---|---|---|---|
| ES startup thread | process start | sets `CFE_ES_SystemState_CORE_READY` `ES1/cfe_es_start.c:192`, calls `CFE_ES_StartApplications` `:199` | [F] |
| CF main task, entry `CF_AppMain` | ES startup thread, script line 6 | `ES1/cfe_es_apps.c:258` `CFE_ES_ParseFileEntry` → `:368` `CFE_ES_AppCreate` → `:768` `CFE_ES_StartAppTask` → `:588` `OS_TaskCreate` | [F] |
| BP main task, entry `BP_AppMain` | ES startup thread, script line 7, same chain | same | [F] |
| BP child tasks `BP_CLA_OUT`, `BP_CLA_IN` | BP main task, after the semaphores | `BP/fsw/src/bp_app.c:208`; `bp_cla_bundle_io.c:155, 207` | [F]; not involved |

- Script: `CFE_APP, cf, CF_AppMain, CF, 65, 16384, 0x0, 0;` (line 6) and `CFE_APP, bp, BP_AppMain, BP, 55, 16384, 0x0, 0;` (line 7); lines 1-5 are libs tinycbor, bplib and apps ci_lab, to_lab, sch_lab [F /home/user/work/procrace/c1_build/pre/1df262a/exe/tx/cf/cfe_es_startup.scr:1-7]. Same file in pre, post and both candidates, equal to `BP/.github/buildconfig/cfe_es_startup.scr` [M3].
- ES does not wait for an application's init between script entries: the only `OS_TaskDelay` in `cfe_es_apps.c` is at `:510`, inside the entry helper of the new task [M11].

### 1.3 The shared object

- Kind: named OSAL object, class counting semaphore. Identity = (object class, name string). One record in `OS_common_table` with `active_id`, `name_entry`, `creator`, `refcount` [F `OS1/shared/src/osapi-idmap.c:639-642`], plus the name bytes in the class table [F `OS1/shared/inc/os-shared-idmap.h:105-110`].
- Key values: "BP0_tsem" (channel 0), "BP1_tsem" (channel 1).
- Consumer's key = table loaded at run time: `CF_AppData.config_table->chan[i].sem_name` [F `CFpre/src/cf_cfdp.c:991-994`]. The pointer comes from `CFE_TBL_GetAddress` (`CFpre/src/cf_app.c:184`) after `CFE_TBL_Load(..., CFE_TBL_SRC_FILE, CF_CONFIG_TABLE_FILENAME)` (`:164`); file name "/cf/cf_def_config.tbl" (`CFpre/platform_inc/cf_platform_cfg.h:280`) [F]. Table source of the build: `BP/.github/buildconfig/tx/tables/cf_def_config.c:40` and `:54` [F]; the built table file holds both strings [M4]. CF's own default table has "" twice (`CFpre/tables/cf_def_config.c:59, 78`) [F].
- Publisher's key = literal in code: `#define CF_CHANNEL_0_SEM_NAME "BP0_tsem"`, `CF_CHANNEL_1_SEM_NAME "BP1_tsem"` (`BP/fsw/custom/bp_semcfg.c:44, 48`), elements of the global `BP_Throttles[BP_NUM_SEM_THROTTLES]` (`:68-70`; record type `BP/fsw/src/bp_flow.h:54-61`), read through `BP_GlobalData.Throttles = BP_Throttles` (`BP/fsw/src/bp_app.c:147`). `BP_NUM_SEM_THROTTLES` is 2 (`BP/fsw/platform_inc/bp_platform_cfg.h:29`; default 0 in `BP/fsw/src/bp_cfg.h:191-193`) [F].
- The two names agree only by value, in two unrelated files; no symbol, header or table links them [I].

### 1.4 Publish operation w

- Call: `OS_CountSemCreate(&BP_GlobalData.Throttles[index].Sem, BP_GlobalData.Throttles[index].Name, BP_GlobalData.Throttles[index].Max, 0)` at `BP/fsw/src/bp_flow.c:132-133`, in `initialize_throttling` (`:121`), loop over index 0..1 (`:124`). Initial count 10 (`bp_semcfg.c:37`, `bp_cfg.h:87`) [F].
- Reached by `BP_AppMain` (`bp_app.c:812`) → `AppInit` → `BP_FlowInit` (`bp_app.c:203`) → first statement `initialize_throttling()` (`bp_flow.c:524`), before the first `CFE_ES_RunLoop` (`bp_app.c:818`) [F].
- Init prefix of the same task before w: `CFE_EVS_Register` (`bp_app.c:152`), `CFE_ES_GetAppID` (`:157`), `CFE_SB_CreatePipe` (`:167`), three `CFE_SB_Subscribe`, `BP_StorageService_Init` (`:193`), `BP_SetupLibrary` (`:196`: `bplib_init` `:112`, `bplib_route_alloc_table(10, 16 << 20)` `:119`) [F].
- Inside OSAL, `OS1/shared/src/osapi-countsem.c:83-110` [F]:
  1. `OS_ObjectIdAllocateNew` (`:94`): an existing name gives `OS_ERR_NAME_TAKEN` (`osapi-idmap.c:1170-1179`); a free record gets `active_id = token->obj_id; name_entry = NULL; creator = OS_TaskGetId()` (`:639-641`); the exclusive conversion overwrites `active_id` with `OS_OBJECT_ID_RESERVED` (`:429-433`) and releases the type lock (`:507-510`).
  2. `OS_OBJECT_INIT` (`:100`): copies the name and sets `name_entry`, without the lock.
  3. `OS_CountSemCreate_Impl` (`:103`).
  4. `OS_ObjectIdFinalizeNew` (`:106`) → `OS_ObjectIdTransactionFinish` re-takes the lock (`osapi-idmap.c:1061`) and stores `record->active_id = *final_id` (`:1079`). This store makes the name visible to lookups. `OS_NotifyEvent(OS_EVENT_RESOURCE_CREATED, ...)` follows (`:854`).
- Lock: OSAL global lock of the object class. Actor: BP main task. Phase: application init, after `CORE_READY`.
- BP tests the result against `CFE_SUCCESS`: success → event `BP_SEM_INFO_EID` (15, INFORMATION) "Created semaphore: %s" (`bp_flow.c:135-136`; `BP/fsw/src/bp_events.h:42`); failure → `BP_SEM_ERR_EID` (14, ERROR) (`:140-141`). The function is `void`; BP's init continues either way [F].

### 1.5 Consume operation u

- Call: `ret = OS_CountSemGetIdByName(&CF_AppData.engine.channels[i].sem_id, CF_AppData.config_table->chan[i].sem_name);` at `CFpre/src/cf_cfdp.c:993-994`, in `CF_CFDP_InitEngine` (`:952`), channel loop (`:967`; `CF_NUM_CHANNELS` 2, `CFpre/platform_inc/cf_platform_cfg.h:104`), only when `sem_name[0]` is non-zero (`:991`) [F].
- Callers: `CF_Init` (`CFpre/src/cf_app.c:252`) ← `CF_AppMain` (`:330`); also the ground command `CF_CmdEnableEngine` (`CFpre/src/cf_cmd.c:1117-1119`) [F].
- Init prefix before u: `CFE_EVS_Register` (`cf_app.c:216`), `CFE_SB_CreatePipe` (`:223`), three `CFE_SB_Subscribe` (`:235`), `CF_TableInit` (`:247`; register `:155`, load `:164`, manage `:174`, get address `:184`); in `InitEngine`, per channel, `CFE_SB_CreatePipe` (`cf_cfdp.c:971`) and `CFE_SB_SubscribeLocal` (`:980`) [F].
- Inside OSAL: `osapi-countsem.c:204-215` → `OS_ObjectIdFindByName` (`osapi-idmap.c:962-984`) → `OS_ObjectIdGetByName(OS_LOCK_MODE_GLOBAL, ...)` (`:975`) → `OS_ObjectIdGetBySearch` (`:910-934`) → `OS_ObjectIdFindNextMatch` (`:538-570`). A record matches when `OS_ObjectIdDefined(record->active_id)` and `name_entry != NULL && strcmp(...) == 0` (`:561`; `:269-272`) [F].
- **Absent object yields exactly:** return value `OS_ERR_NAME_NOT_FOUND`, the initial value of `return_code` (`osapi-idmap.c:545`), defined `(-17)` in `OS1/inc/osapi-error.h:113`, documented `@retval` "if the name was not found in the table" (`OS1/inc/osapi-countsem.h:160`) [F]. Class: error, by the OSAL rule "Negative values will always represent errors, while non-negative values indicate success" (`osapi-error.h:73-75`) [F]. OSAL codes have no severity field; the severity bits of `B1/cfe/modules/core_api/fsw/inc/cfe_error.h:115-119` are defined for cFE codes [F].
- Output argument when absent: not written (`*object_id = token.obj_id` only on success, `osapi-idmap.c:976-978`); `sem_id` stays 0 from `memset(&CF_AppData.engine, 0, ...)` (`cf_cfdp.c:965`) [F].
- Third outcome, by code reading only, never observed [I]: a lookup that runs after the creator's step 2 and before its step 4 matches the name while `active_id == OS_OBJECT_ID_RESERVED` (`os-shared-idmap.h:32`); `OS_ObjectIdConvertToken` returns `OS_ERR_INCORRECT_OBJ_STATE` (-35, `osapi-error.h:125`) at `osapi-idmap.c:395-398`, before its unlock block (`:488-521`); `GetBySearch` cancels only on a miss (`:928-931`) and `FindByName` releases only on success (`:976-981`), so the class lock would stay held and the creator would block at `:1061`. Same hypothesis in the case file [F C1.md:492-512]. The fixed CF loop does not retry on -35.

### 1.6 Order enforcement

- No synchronization exists between w and u: neither CF (281a941, 833fdbb, 15a871e) nor BP calls `CFE_ES_WaitForStartupSync` or `CFE_ES_WaitForSystemState` [M5].
- ES creates the two tasks one after the other and waits for nothing in between (1.2). ES's own waits come after all entries are created (`ES1/cfe_es_start.c:208`, `:223`; timeout `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` 1000 and poll `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` 50 at `DEF1:1731`, `:1689`) and only delay ES's own state changes [F].
- The OSAL class lock makes each call atomic and orders nothing [I].
- Both orders are possible; neither is forced [I]. What decides: script order (CF is created first), the length of the two init prefixes (1.4, 1.5), and scheduling.
- Recorded runs: every natural run had u first — pre, 8 records (W6:169); post, 8 records, first lookup -17 (W6:170); build-smoke pre, 4 instances (BM:158-159) [F]. Runs with w first exist only under a researcher hold (W6:172) [F]. The reporter's comments say CF failed although BP was started first, "quite often" [F: case file C1.md:49-50]; the configuration of that report is unknown [U, C1.md:65].
- The script priorities (CF 65, BP 55) were not in effect in the recorded runs [F BM:166].

### 1.7 Consequence of "u before w" in the buggy version

| Step | Code | Tag |
|---|---|---|
| Status is checked | `if (ret != OS_SUCCESS)` `CFpre/src/cf_cfdp.c:995` | [F] |
| Event | `CFE_EVS_SendEvent(CF_EID_ERR_INIT_SEM, CFE_EVS_EventType_ERROR, "CF: failed to get sem id for name %s, error=%ld", ...)` `:997-999`; EID 30 (`CFpre/src/cf_events.h:158`) | [F] |
| Branch | `break` (`:1000`) leaves the channel loop, channel 1 is never looked up; `engine.enabled` stays 0 (`:1029-1032`); `return ret` = -17 (`:1034`) | [F] |
| Init result | `CF_Init` returns -17 (`cf_app.c:252`); event 20 `CF_EID_INF_INIT` is not sent (`:255-259`; `cf_events.h:47`) | [F] |
| Run status | `CF_AppData.run_status = CFE_ES_RunStatus_APP_ERROR` (`cf_app.c:333`); value 3 (`B1/cfe/modules/core_api/fsw/inc/cfe_es_extern_typedefs.h:139`) | [F] |
| Main loop | `CFE_ES_RunLoop` returns false at once for a status other than `APP_RUN`, before it touches the application state (`ES1/cfe_es_api.c:497-500`) | [F] |
| Exit | `CFE_ES_ExitApp(CF_AppData.run_status)` (`cf_app.c:366`) | [F] |
| ES, in the CF task | `AppControlRequest = ExitStatus` (`ES1/cfe_es_api.c:373-376`); syslog "CFE_ES_ExitApp: Application CF called CFE_ES_ExitApp" (`:439-440`); `AppState = CFE_ES_AppState_STOPPED` (`:442`); then a loop of `OS_TaskDelay(500)` (`:453-456`) | [F] |
| ES, background | scan interval `CFE_PLATFORM_ES_APP_SCAN_RATE` 1000 (`DEF1:591`) → `CFE_ES_ProcessControlRequest` (`ES1/cfe_es_apps.c:1091`) → `CFE_ES_CleanUpApp` (`:1157`; table, SB, TIME, EVS cleanup `:1433-1448`, task resources `:1467`, record freed `:1530`) → event `CFE_ES_ERREXIT_APP_INF_EID` (14, `B1/cfe/modules/es/fsw/inc/cfe_es_events.h:184`), type INFORMATION, text "Exit Application CF Completed." (`:1192-1203`, `:1303-1306`) | [F] |

- Application state: the record is cleared at creation (`ES1/cfe_es_apps.c:718`), so the state starts at 0 = UNDEFINED; no creation code assigns `EARLY_INIT` [M6]. CF therefore goes UNDEFINED → STOPPED and is never RUNNING [I].
- Healing: none. One call, no loop (`cf_cfdp.c:993-1001`) [F]. No restart: only `SYS_RESTART` and `SYS_RELOAD` re-create an application (`ES1/cfe_es_apps.c:1163-1166`) [F]. `CF_CmdEnableEngine` needs a live CF [I].
- Later in the same run BP creates both semaphores (event 15 "Created semaphore" twice) [F BM:158; W6:169].
- Steady state compared with "w before u": CF is absent (no task, pipes, table, EVS registration; no event 20); BP and both semaphores exist in both orders [I from the cleanup code; "no CF thread" recorded, F BM:166].

### 1.8 The fix

- `CFpost/src/cf_cfdp.c:1000-1012` [F]: `ret = OS_ERR_NAME_NOT_FOUND;` `for (j = 0; j < CF_STARTUP_SEM_MAX_RETRIES; ++j)` { lookup (`:1003-1004`); `if (ret != OS_ERR_NAME_NOT_FOUND) break;` (`:1006-1009`); `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY);` (`:1011`) }. The failure handling after the loop is unchanged (`:1014-1020`).
- Constants: `CF_STARTUP_SEM_MAX_RETRIES 25`, `CF_STARTUP_SEM_TASK_DELAY 100` (`CFpost/platform_inc/cf_platform_cfg.h:338, 349`); unit milliseconds (`int32 OS_TaskDelay(uint32 millisecond)`, `OS1/inc/osapi-task.h:167`) [F]. They are application constants added by the fix [M2], not framework constants.
- Effect: the dependence is kept and not enforced; it is tolerated by a retry on one status. At most 25 lookups; the last one comes 24 delays (nominally 2400 ms) after the first; a 25th delay precedes the failure event [I from `:1001-1011`]. Any other status ends the loop at once (`:1006-1009`) [F].
- The source comment states the intent: "There is a start up race condition because CFE starts all apps at the same time, and if this sem is instantiated by another app, it may not be created yet." (`:993-999`) [F].

### 1.9 What a generic observer sees in the buggy version

(a) Framework API boundary. Sequence [I from the code above]; statuses and counts [F W6:169]:

| Task | Call | Key argument | Returned |
|---|---|---|---|
| CF | `OS_CountSemGetIdByName` | "BP0_tsem" | -17 |
| CF | `CFE_EVS_SendEvent` | EID 30, type ERROR (3) | |
| CF | `CFE_ES_RunLoop` | `*RunStatus` = 3 | false |
| CF | `CFE_ES_ExitApp` | 3 | does not return |
| BP | `OS_CountSemGetIdByName` | "BP0_tsem" | -17 (BP's own probe, see N1) |
| BP | `OS_CountSemCreate` | "BP0_tsem", 10, 0 | 0 |
| BP | `CFE_EVS_SendEvent` | EID 15, type INFORMATION (2) | |
| BP | the same three calls | "BP1_tsem" | -17, 0 |

(b) Events and syslog: CF 30 ERROR "CF: failed to get sem id for name BP0_tsem, error=-17"; ES "Exit Application CF Completed"; BP 15 "Created semaphore: BP0_tsem", "Created semaphore: BP1_tsem" [F BM:158]. Syslog line of `CFE_ES_ExitApp` [I from `ES1/cfe_es_api.c:439`]. Missing against a good boot: CF 20 "CF Initialized. Version 3.0.99.0" [F BM:160].

(c) Memory level: the CF task loads `active_id`, `name_entry` and the name bytes of each record of the class (`osapi-idmap.c:561`, `:271`) under the class lock. The BP task stores `active_id` (`:639`, `:432`, `:1079`) and `creator` (`:641`) under the lock, and `name_entry` plus the name bytes (`os-shared-idmap.h:107-109`) between the two locked sections [F]. For `active_id` the pair is lock-protected; the order shows only in the search result [I]. `CF_Channel_t.sem_id` is stored only on success (`osapi-idmap.c:978`) [F].

(d) Recorded runs [F W6:169-172, 174, 197]: the four series of 1.10. In the 25-lookup series event 30 appears once. The old tool reached VIOLATING on the natural pre record only with return-code narrowing switched on.

(e) Step 2a on `c1_pre_tx` [F REL1:419-434]: row A21, lookup `cf_cfdp.c:993:19`, "key unresolved" ("read through the pointer field CF_AppData_t.config_table: needs field points-to", `:421`); creators listed: BP `bp_flow.c:132:18` and SCH_LAB `sch_lab_app.c:168:16` (`:423-424`); handling chain up to `CFE_ES_ExitApp` (`:426-428`); "feasibility: undecidable", "violation: unconfirmed", "flip: static-undetermined" (`:431-433`). The results document grades the C1 test PARTIAL and does not count the row as detection evidence [F AR:31, 292].

### 1.10 Ground truth

| Version | Order | CF lookup statuses | Events | CF end | Steady state | Evidence |
|---|---|---|---|---|---|---|
| buggy 281a941 | u before w | BP0: -17 once; BP1: none | CF 30 once; ES exit event; BP 15 twice | `CFE_ES_ExitApp`, never RUNNING | CF absent; BP and 2 semaphores present | [F W6:169; BM:158-159] natural, 8 records and 4 smoke instances; state path [I] |
| buggy | w before u | BP0: 0 once; BP1: 0 once | CF 20 once; BP 15 twice | `CF_Init` = 0, runs | CF and BP present | [F W6:172] 8 records under a researcher hold; no natural record [U] |
| fixed 833fdbb | u before w, w inside the retry window | BP0: -17 once, then 0; BP1: 0 once | CF 20 once; BP 15 twice; no event per retry | `CF_Init` = 0, runs | no difference from "w before u" visible at framework level [I] | [F W6:170; BM:160-161, 164] natural, 8 records |
| fixed | u before w, w later than the window | BP0: -17 twenty-five times; BP1: none | CF 30 once | `CFE_ES_ExitApp` | CF absent | [F W6:171, 174] 12 records under a hold, labelled `timeout-exercised` |
| fixed | w before u | first lookup 0 | CF 20 | runs | CF and BP present | [I from `CFpost/src/cf_cfdp.c:1003-1009`]; recorded for key BP1 only [F W6:170]; for BP0 [U] |

### 1.11 Benign look-alikes (negative controls)

| # | Shape | Site | Why it is not a violation |
|---|---|---|---|
| N1 | lookup by name; on a miss the same task creates the same name | `BP/fsw/src/bp_flow.c:126-133` | same actor, the create is the miss branch; -17 is the normal first-boot result; on a hit: event 15 "Found semaphore: %s" (`:129-130`). 2a excludes it as probe-create [F REL1:408, 412] |
| N2 | single lookup by name; miss → syslog, init error, application exit (the shape of C1) | `B1/apps/sch_lab/fsw/src/sch_lab_app.c:175-181` `OS_TimeBaseGetIdByName(&TimeBaseId, "cFS-Master")` | the publisher is PSP module code: `OS_TimeBaseCreate(..., CFE_PSP_SOFT_TIMEBASE_NAME, NULL)` (`B1/psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase.c:53`; name `B1/psp/fsw/inc/cfe_psp.h:132`), run by `CFE_PSP_ModuleInit()` (`B1/psp/fsw/pc-linux/src/cfe_psp_start.c:371`) before the ES main function is called (`:429`): one thread, program order, w always first. Presence of the module is build configuration [M14], not order |
| N3 | lookup by name; a miss is tolerated | `B1/cfe/modules/time/fsw/src/cfe_time_task.c:286-305` | designed existence check with fallback (comment `:280-285`); returns `CFE_SUCCESS` |
| N4 | bounded retry on the miss status | `CFpost/src/cf_cfdp.c:1000-1012` | the fixed C1: a transient -17 is healed in the same run [F W6:170] |
| N5 | designed absence | `CFpre/src/cf_cfdp.c:991`; `CFpre/src/cf_cfdp_sbintf.c:91-98`; `CFpre/src/cf_cmd.c:920` | an empty name means "no semaphore"; `OS_ObjectIdDefined(c->sem_id)` false means "no throttle" |
| N6 | create of the same class with another name, no foreign lookup | `B1/apps/sch_lab/fsw/src/sch_lab_app.c:168` `OS_CountSemCreate(&SCH_LAB_Global.TimingSem, "SCH_LAB", 0, 0)` | 2a pairs it with CF's lookup only because CF's key is unresolved [F REL1:424] |
| N7 | message sent while nobody is subscribed | `B1/cfe/modules/sb/fsw/src/cfe_sb_api.c:1450, 1506` (`CFE_SB_SEND_NO_SUBS_EID`, INFORMATION) | reported by the framework as information; a normal startup look-alike in another object class |
| N8 | lookup by name inside a ground command | `ES1/cfe_es_task.c:956, 1005, 1054, 1115` (`CFE_ES_GetAppIDByName`) | a miss is an operator error path, not startup order |

Call sites N1-N3 and N8 come from one search over the bundle and the two applications [M13].

### 1.12 What a person wrote for this case in the old path

- A judgement rule per CF revision: `rspec K-REG.osal-countsem-lookup { use op OS_CountSemGetIdByName; key osal.countsem(arg.sem_name); requires state(...) = LIVE; appreq cf_events_sem_eid; allowed none; harm app_error_exit; }` [F R/kairos/profiles/c1-bp-b051eb0/rspec-281a941/40_rspec.kzp:17-27; IA:331-332, 338, 345-346, 353].
- Two roles: CF's lookup as `dependent_use`, BP's lookup as `existence_probe` [F .../10_kinds.kzp:86-102; IA:359-360].
- Seven `--target` names for the extractor and 33/34 observation sites named after the rule [F IA:500, 586, 596-601].
- Hold plans with the literal names and modules, for example `gate G_bp0_create site=osal.countsem_create.entry when name sem_name "BP0_tsem"` and `release R_cf_exit site=es.exit_app.entry ... caller-in cf.so` [F .../hold/hold-cf833fdbb.kzh:22-23; IA:624-627]; hand-written control-flow models of BP and CF [F IA:643-649]; an observation window chosen from the retry time [F IA:668].
- A new method must produce by itself: the object class and key, the pair (w, u) across two applications, the "absent" status and its handling chain to the exit, the exclusion of N1, the two orders and which one fails, and for the fixed version the retry bound.

### 1.13 Classification

Race-like: the order is not enforced and both orders are feasible [I]. Between two applications (CF, BP) through an OSAL named object. One-shot loss in the buggy version (application exit, not healed); healed inside a bound in the fixed version. In the reference configuration the natural order was "u first" in every recorded run, so it behaves there like a premature use, without being enforced [F W6:169-170].

### 1.14 Unknowns

- No natural record of "w before u" in either version; no record with BP before CF in the script [U].
- The reporter's configuration, versions and failure frequency [U, C1.md:65].
- The -35 outcome and the lock left held (1.5): code reading only, never executed [U].
- Whether a clean boot of these builds shows other error statuses or ERROR events (the noise floor of a vocabulary oracle): raw run directories were not opened [U].
- Time between CF's first lookup and BP's create in natural runs: only bounded by "the second lookup, 100 ms later, succeeded" [F W6:170]; no finer value [U].
- Steady-state effects other than the application table (for example BP giving a semaphore nobody takes) are not recorded [U].

## 2. Card C2 — nasa/cFE #198

**What it is.** A feature request (label `enhancement`; proposed solution "add another CFE startup state for this late-phase init") that carries a field observation: "requests can be made to an application that has not been fully initialized", "observed in the EVA CWS project" [F: case file C2.md:52, 69-74]. The observed violation is a use that needs another actor's publication first, but the object, the calls and the symptom are not disclosed and the applications are private [F: case file C2.md:21, 83]. On public code C2 is a property of the framework's startup barrier, not a concrete pair.

### 2.1 Source and versions

| Item | Value | Tag |
|---|---|---|
| Issue | nasa/cFE #198 "Additional CFE start up state for application sync", label `enhancement`, milestone 6.6.0, no comments | [F: case file C2.md:49-58] |
| Origin | very probably the migration of Trac #167, resolved in cFE 6.6.0 before the GitHub issue existed (inference of the case file) | [F: case file C2.md:13-14] |
| Single-barrier framework ("buggy" side) | cFE 6.4.2 = `S642` (on disk); v6.5.0a has the same sync logic (not on disk) | [F lines of 2.4-2.6]; [F: case file C2.md:89] |
| Two-phase framework ("fixed" side) | first public: v6.6.0a `8811c92`, bulk import `2661d19` (not on disk); the mechanism was read at cFE `6cccb3f` (`B1`) and `c5fb2b4` (`V7`) | [F: case file C2.md:287-290]; [F lines of 2.8] |
| 2a builds | `c5_642` is built from a cFE 6.4.2 tree, the single barrier; it has no application with the reported pattern and no relations row on the barrier state. `r2_core`, `c1_pre_tx`, `c1_post_tx` contain the two-phase barrier | [F .../c5_642/NP/args.txt:4-5] [M7] [M9] |
| Runs, profile | none; C2 does not occur in the input audit, the 2a results or the redesign analysis | [M8] |

### 2.2 Actors

- In the report: P ("one app": calls `CFE_ES_WaitForStartupSync()` before its late-phase init) and R ("other applications (also using the same sync call)" that "were calling other functions before the late-phase init had completed") [F: case file C2.md:71-73]. That R's calls target P is an inference of the case file [F C2.md:234].
- In the framework: the ES startup thread sets `CFE_ES_SYSTEM_STATE_CORE_READY` (`S642/es/cfe_es_start.c:268`) and calls `CFE_ES_StartApplications` (`:275`); per script entry `CFE_ES_ParseFileEntry` (`S642/es/cfe_es_apps.c:307`) → `CFE_ES_AppCreate` (`:387`, definition `:426`): `++CFE_ES_Global.AppStartedCount` (`:607`), `AppState = CFE_ES_APP_STATE_INITIALIZING` (`:613`), `OS_TaskCreate` (`:619`) [F].
- Applications of the public 6.4.2 script: sample_app, ci_lab, to_lab, sch_lab [F /home/user/work/c5/src/642/cFE-6.4.2-OSS-release/build/cpu1/exe/cfe_es_startup.scr:2-5]. None calls `CFE_ES_WaitForStartupSync` [M9]; only the five core tasks do (`S642/es/cfe_es_task.c:209`, `evs/cfe_evs_task.c:336`, `sb/cfe_sb_task.c:143`, `time/cfe_time_task.c:209`, `tbl/cfe_tbl_task.c:240`) [F].

### 2.3 The shared object

- Application level: "the readiness of P after its late init"; kind and key not disclosed [F: case file C2.md:231] [U].
- Framework level: ES global state, not keyed objects [F]: (i) `CFE_ES_Global.AppTable[AppID].StateRecord.AppState` (`CFE_ES_APP_STATE_RUNNING` 1, `..._INITIALIZING` 4, `S642/es/cfe_es_apps.h:107-110`), indexed by the caller's own application, computed at run time; (ii) counters `AppStartedCount`, `AppReadyCount` (`S642/es/cfe_es_global.h:176-177`); (iii) `CFE_ES_Global.SystemState` (`CFE_ES_SYSTEM_STATE_OPERATIONAL` 4, `S642/es/cfe_es_apps.h:91-96`).

### 2.4 Publish operation w

- Application level: the end of P's late init; location unknown [U].
- What the barrier takes for "P is ready" [F]: `AppState = CFE_ES_APP_STATE_RUNNING; ++CFE_ES_Global.AppReadyCount;` written by P's own task under `CFE_ES_LockSharedData`, either in `CFE_ES_WaitForStartupSync` (`S642/es/cfe_es_api.c:670-674`; lock `:666`, unlock `:690`) or in its first `CFE_ES_RunLoop` with `CFE_ES_APP_RUN` (`:580-586`).
- Aggregation [F]: the ES startup thread polls `AppReadyCount != AppStartedCount` (`S642/es/cfe_es_start.c:283-284`; loop `:1015-1017`, read without the lock) and then stores `CFE_ES_Global.SystemState = CFE_ES_SYSTEM_STATE_OPERATIONAL` (`:293`), also after a timeout (syslog `:286`; `CFE_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` 30000, /home/user/work/c5/src/642/cFE-6.4.2-OSS-release/cfe/fsw/platform_inc/cpu1/cfe_platform_cfg.h:1744).
- In the reported pattern P's RUNNING is written by its sync call, before the late init that the readiness should cover [F: case file C2.md:71-72]: the publication is early by construction, not by scheduling [I].

### 2.5 Consume operation u

- Application level: R's "other functions": which functions, by which mechanism, with which result — not stated; no crash, status or log is reported [F: case file C2.md:83, 352] [U].
- Framework level: R's return from `CFE_ES_WaitForStartupSync`. `CFE_ES_ApplicationSyncDelay(MinSystemState, TimeOutMilliseconds)` (`S642/es/cfe_es_api.c:693`) polls `CFE_ES_Global.SystemState < MinimumSystemState` every `CFE_ES_STARTUP_SYNC_POLL_MSEC` (50, cfe_platform_cfg.h:1702) (`S642/es/cfe_es_start.c:1015-1036`) [F].
- **Not yet published yields:** more polling; after the caller's timeout `CFE_ES_OPERATION_TIMED_OUT` (`:1031`; `0xc4000027`, `S642/inc/cfe_error.h:487`; severity bits 11 = error, `:132-136`), which `CFE_ES_WaitForStartupSync` drops because it is `void` (`S642/es/cfe_es_api.c:652`, `:693`) [F]. The caller cannot tell a timeout from a reached state.
- Published too early (the reported situation): R returns normally with OPERATIONAL set; nothing at the framework boundary differs from a correct boot [I].

### 2.6 Order enforcement

- Enforced: every started application has written RUNNING, or the ES timeout has expired, before an external waiter is released (`S642/es/cfe_es_start.c:283-293`; `cfe_es_api.c:686, 693`) [F].
- Not enforced: P's late init before R's release, when P's RUNNING is written by the sync call itself [I; stated by the issue, F: case file C2.md:69-73]. P and R are then both released when OPERATIONAL is stored and run concurrently. Frequency in the report: not stated [U].
- Other early releases: the ES timeout (30000 ms sample value) and the waiter's own timeout (`S642/es/cfe_es_start.c:1025-1032`) [F].
- A framework rule that is independent of any application: "This API should only be called as the last item of an Apps initialization." (`S642/inc/cfe_es.h:557`; same sentence at `V7/cfe/modules/core_api/fsw/inc/cfe_es.h:435`) [F]. The implementation comment agrees ("is assumed to have completed its initialization and is ready to run", `S642/es/cfe_es_api.c:660-665`); the function comment above it describes the opposite use ("Called by applications that need to ensure that all other apps are running before completing their initialization process", `:646-647`) [F].

### 2.7 Consequence of "u before w" on the single-barrier framework

- Application level: only the early calls are reported [F: case file C2.md:352]; branch, events, exit, healing and steady-state difference are unknown [U].
- Framework level: no status, event or syslog line marks the situation [I]. The barrier's only failure line, "ES Startup: Startup Sync failed - Applications may not have all started" (`S642/es/cfe_es_start.c:286`), belongs to the timeout path [F].

### 2.8 The fix

Read at `V7` (the same code is in `B1`, for example `ES1/cfe_es_start.c:208-232`):

- States [F `V7/cfe/modules/es/config/default_cfe_es_extern_typedefs.h`]: `CFE_ES_SystemState_APPS_INIT = 4` "startup apps have all completed their early init, but not necessarily operational yet" (`:201-204`); `CFE_ES_SystemState_OPERATIONAL = 5` "normal operation mode; all apps are RUNNING" (`:206-209`); `CFE_ES_AppState_LATE_INIT = 2` "Early/Local initialization is complete. First sync point." (`:271-274`); `CFE_ES_AppState_RUNNING = 3` "All initialization is complete. Second sync point." (`:276-279`).
- `CFE_ES_WaitForSystemState(MinSystemState, TimeOutMilliseconds)` (`ES7/cfe_es_api.c:514`) [F]: under the ES lock it raises the caller's `AppState` to the level implied by the awaited state — APPS_INIT → LATE_INIT (`:560-562`), OPERATIONAL → RUNNING (`:556-558`), store `:573` — then polls `CFE_ES_Global.SystemState < MinSystemState` (`:585`) and returns `CFE_ES_OPERATION_TIMED_OUT` on timeout (`:599`; `0xc4000027`, `V7/cfe/modules/core_api/fsw/inc/cfe_error.h:622`).
- `CFE_ES_WaitForStartupSync` is now `CFE_ES_WaitForSystemState(CFE_ES_SystemState_OPERATIONAL, TimeOutMilliseconds)` and still `void` (`ES7/cfe_es_api.c:616-619`) [F]. The old call pattern therefore still writes RUNNING before the caller's later init [I].
- ES startup thread, two phases [F `ES7/cfe_es_start.c`]: `CFE_ES_MainTaskSyncDelay(CFE_ES_AppState_LATE_INIT, CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC)` (`:210`) → `SystemState = CFE_ES_SystemState_APPS_INIT` (`:216`) → `CFE_ES_MainTaskSyncDelay(CFE_ES_AppState_RUNNING, ...)` (`:225`) → `SystemState = CFE_ES_SystemState_OPERATIONAL` (`:234`). A timeout only writes a syslog line (`:212`, `:227`). Timeout 1000 ms, poll 50 ms (`V7/cfe/cmake/sample_defs/example_platform_cfg.h:856, 838`).
- Kind: the order is enforced by synchronization, only for applications that adopt the new sequence (wait for APPS_INIT, late init, then RUNNING) and only inside soft timeouts [I; same reading F: case file C2.md:311-316]. It is not a retry and not a guard at a use site.
- No application on disk uses `CFE_ES_SystemState_APPS_INIT`: outside tests it appears only in the ES mapping and the ES store [M10].
- Difference from a project document: C2.md:294 says new applications start in `EARLY_INIT` (read at v6.6.0a). At `B1` and `V7` the record is cleared by `memset` (`ES1/cfe_es_apps.c:718`; `ES7/cfe_es_apps.c:799`) and no creation code assigns `EARLY_INIT` [M6]; the state after creation is 0 = `CFE_ES_AppState_UNDEFINED` (`default_cfe_es_extern_typedefs.h:264`) until the application waits, runs or exits [F].

### 2.9 What a generic observer sees

- (a) API boundary, 6.4.2: `CFE_ES_WaitForStartupSync(T)` called and returned by P and by R; no key argument, no status. Two-phase framework: `CFE_ES_WaitForSystemState(state, T)` → `CFE_SUCCESS` or `CFE_ES_OPERATION_TIMED_OUT`. The calls R makes afterwards are unknown [U].
- (b) Syslog: "ES Startup: CFE_ES_Main entering CORE_READY state" (`S642/es/cfe_es_start.c:267`), "... entering OPERATIONAL state" (`:292`), the timeout line (`:286`) [F]. No EVS event belongs to the barrier [I].
- (c) Memory: stores to `AppState` and `AppReadyCount` by each application task under the ES lock (2.4); the store to `SystemState` by the ES startup thread (`:293`) and the loads in the polling loop (`:1015-1017`) are made without the lock [F].
- Recorded runs: none [M8]. The three role-based configurations planned for C2 were never built or executed [F CS:133-136].

### 2.10 Ground truth

| Framework and P's call pattern | Order | Framework-level outcome | Application-level outcome | Tag |
|---|---|---|---|---|
| single barrier (6.4.2), P syncs before late init | R's calls after P's late init | sync returns, OPERATIONAL set, no status or event | not reported | [I]; [U] |
| same | R's calls before P's late init ends | identical at the framework boundary | "calling other functions before the late-phase init had completed"; symptom not reported | [I]; [F: case file C2.md:73, 352]; [U] |
| two-phase, P keeps the old call | either | as above: RUNNING is written at P's sync call (`ES7/cfe_es_api.c:556-558, 573, 618`) | not executed | [I]; [F CS:135]; [U] |
| two-phase, P waits for APPS_INIT, late init, then RUNNING | R released after P | OPERATIONAL is stored after all applications are RUNNING (`ES7/cfe_es_start.c:225-234`) | not executed | [I]; [F CS:136]; [U] |
| two-phase, phase timeout expires first | R released before P | syslog "Startup Sync failed - Applications may not have all started" (`ES7/cfe_es_start.c:227`); R's wait returns `CFE_SUCCESS` | not executed | [I]; [U] |

### 2.11 Look-alikes in public code

All four applications below are started by the script of the v7.0.1 reference build [F V7/build-native_std/exe/cpu1/cf/cfe_es_startup.scr:5, 12, 16, 17].

| # | Site | Shape | Note |
|---|---|---|---|
| L1 | `V7/apps/to_lab/fsw/src/to_lab_app.c:67-68` | sync call, then `TO_LAB_UpdateSubscriptionsFromTable()` (subscription loop `:471-477`) | the shape of P. The comment gives the purpose: "Defer subscribing until the system is operational will avoid possibly seeing MsgLimit errors" (`:64-66`). Timeout 10000 (`V7/apps/to_lab/fsw/inc/to_lab_internal_cfg.h:80`) |
| L2 | `V7/apps/hs/fsw/src/hs_app.c:75-93` | sync call, then watchdog set-up and `CFE_SB_SubscribeEx` of event messages | the shape of P |
| L3 | `V7/apps/sbn/fsw/src/sbn_app.c:1647-1650`, `:1536-1541` | sync call, then `LoadConfTbl()`; later `while (CFE_ES_WaitForSystemState(CFE_ES_SystemState_OPERATIONAL, 1000) == CFE_ES_OPERATION_TIMED_OUT);`, then subscription requests are sent again | the shape of P, plus an unbounded retry on the timeout status |
| L4 | `V7/apps/cs/fsw/src/cs_app.c:77-80` | sync call as the last item before the main loop | the documented use |
| L5 | `ES7/cfe_es_task.c:134`; `V7/cfe/modules/sb/fsw/src/cfe_sb_task.c:93` (also evs `:229`, time `:88`, tbl `:74`) | `CFE_ES_WaitForSystemState(CFE_ES_SystemState_CORE_READY, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` after init, before the main loop | the documented use, core tasks |

No project document reports L1-L3 as defects; whether another application depends on what they publish after the sync call is [U].

### 2.12 What a person wrote, or would have written, in the old path

- Nothing exists: no profile, rule, hold plan or run for C2 [M8].
- The plan was a "role-based verification of the C2 mechanism", explicitly not a reproduction [F CS:70]: a person would write two role applications P and R, the request mechanism from R to P, a readiness marker inside P and the oracle "P's completion event is observed before R's request entry" [F CS:119-120, 168; C2.md:371-387]. None was approved, built or run [F CS:133-136].
- A new method cannot use any of these. On the shipped system it can only derive facts of the kind listed in 2.13.

### 2.13 Classification, and what a method can be asked to find

- Classification: not a race on a concrete object in public code. At report level: an order violation between applications that both used the framework barrier, that is, a barrier that does not cover the dependency (insufficient synchronization), with unknown object and symptom; one-shot or healed is unknown. At framework level: a synchronization API with one state for two meanings ("I am ready" and "wait until all are ready"), plus an application usage that departs from the documented rule of 2.6.
- Targets a method could meaningfully be asked for (facts available in the system under test, not a design):
  - T1. Per application: framework publications (pipes, subscriptions, table registration or load, child tasks, named OSAL objects) made after the application's `AppState` was raised to RUNNING by a wait call and before its main loop. Instances on disk: L1-L3.
  - T2. Cross-application pairs whose publish side lies in such a phase and whose consume side is released by the same barrier: the C2 situation with a concrete object. Whether one exists in the v7.0.1 build is [U].
  - T3. Barrier releases by timeout in a run: the syslog lines at `ES7/cfe_es_start.c:212, 227` and the status `CFE_ES_OPERATION_TIMED_OUT` of `CFE_ES_WaitForSystemState`: the framework's own signal that an order was not enforced.
  - T4. The P/R role construction of 2.12 is written by a person per case and lies outside the input boundary.

### 2.14 Unknowns

- Everything about the reported applications: names, the functions R called, the mechanism, the symptom, the cFE version, the timeouts passed, the frequency [U, C2.md:83, 352].
- The linkage of #198 to Trac #167 and the content of the two Trac commits (inference of the case file, C2.md:13-14, 288) [U].
- Behaviour of v6.5.0a and v6.6.0a: not on disk; all statements about them are [F: case file].
- Whether any pair of type T2 exists in a shipped configuration [U].

## 3. Commands behind the [M] tags

- M1. `cmp -s` of clone against build source: CF_281a941 and CF_833fdbb `fsw/src/{cf_cfdp.c,cf_app.c,cf_events.h}` against c1_build/{pre,post}/src/cf; bp_b051eb0 `fsw/src/{bp_flow.c,bp_app.c}`, `fsw/custom/bp_semcfg.c`, `fsw/platform_inc/bp_platform_cfg.h` against c1_build/pre/src/bp; osal_f34759d `osapi-idmap.c` and cFE_6cccb3f `cfe_es_api.c` against `B1`. All identical.
- M2. `diff -rq CF_281a941 CF_833fdbb`: outside `.git` only `fsw/platform_inc/cf_platform_cfg.h`, `fsw/src/cf_cfdp.c`, `unit-test/cf_cfdp_tests.c` differ; `diff -ru CF_281a941/fsw CF_833fdbb/fsw | grep -c '^@@'` = 2.
- M3. `cmp` of `exe/tx/cf/cfe_es_startup.scr` among c1_build/pre/1df262a, pre/1b0b338, post/1df262a, and of `bpxfer_defs/cfe_es_startup.scr` against `BP/.github/buildconfig/cfe_es_startup.scr`: identical.
- M4. `strings -a /home/user/work/procrace/c1_build/pre/1df262a/exe/tx/cf/cf_def_config.tbl | grep -c tsem` = 2 (BP0_tsem, BP1_tsem).
- M5. `grep -rn 'CFE_ES_WaitForStartupSync\|CFE_ES_WaitForSystemState' <tree> --include=*.c --include=*.h | grep -v 'unit-test\|ut-' | wc -l` = 0 for CF_281a941, CF_833fdbb, CF_15a871e, bp_b051eb0.
- M6. `grep -n 'AppState_EARLY_INIT' <ES source dir>/*.c`: only `cfe_es_api.c:581` (`B1`) and `:536` (`V7`), the default value of `RequiredAppState`.
- M7. `grep -c 'SystemState\|AppReadyCount\|AppStartedCount' /home/user/work/redesign/step2a/tests_8c_fix/c5_642/NP/relations.txt` = 0.
- M8. `ls R/kairos/profiles` = c1-bp-b051eb0, c5-cfe-6.4.x, cfs-v7.0.1; `grep -c 'C2\b'` on `IA`, `AR`, `R/plan/REDESIGN_REVIEW_ANALYSIS_20261030.md` = 0, 0, 0.
- M9. `grep -rn CFE_ES_WaitForStartupSync /home/user/work/c5/src/642/cFE-6.4.2-OSS-release/apps | wc -l` = 0.
- M10. `grep -rn 'SystemState_APPS_INIT' V7/apps V7/libs V7/cfe/modules --include=*.c`: `cfe_es_api.c:560`, `cfe_es_start.c:216`, `cfe_testcase/src/es_behavior_test.c:75`.
- M11. `grep -n OS_TaskDelay ES1/cfe_es_apps.c`: line 510 only.
- M12. `cmp -s` of `S642` against /home/user/work/c5tools/build/642/keep/cFE-6.4.2-OSS-release for `es/cfe_es_api.c`, `es/cfe_es_start.c`, `es/cfe_es_apps.c`, `es/cfe_es_apps.h`, `inc/cfe_es.h`, `inc/cfe_error.h`: identical.
- M13. `grep -rn 'GetIdByName(\|GetAppIDByName(\|GetPipeIdByName(\|GetTaskIDByName(' B1/cfe/modules/*/fsw/src B1/apps/*/fsw/src c1_build/pre/src/cf/fsw/src c1_build/pre/src/bp/fsw/src`.
- M14. `grep soft_timebase /home/user/work/procrace/c1_build/pre/1df262a/build/native/default/compile_commands.json`: the module directory is present.

## 4. Observations a designer should know (facts, not a design)

1. One natural run of the buggy C1 build already contains the whole pattern in framework vocabulary: a lookup returns the "absent" error status, the caller sends an ERROR event and exits, and later another application publishes the same (class, name) [F W6:169; BM:158]. One natural run of the fixed build shows -17 and then 0 at the same call site [F W6:170].
2. A hold on the publisher makes the fixed build fail exactly like the buggy one [F W6:171]: a forced order that outlasts an application retry bound turns "tolerated" into "failed". The bound is in application source (`CFpost/platform_inc/cf_platform_cfg.h:338, 349`); 2a already extracts the loop as "count-bounded trip upper-bound 25" with its `OS_TaskDelay` [F REL2:419-434].
3. BP's probe (N1) returns the same -17 for the same key in every first boot. A status-only rule would flag it; 2a excludes it by the "same actor creates on the miss branch" fact [F REL1:408, 412].
4. The consumer's key cannot be resolved statically (table loaded at run time), yet it is a plain string argument at the OSAL boundary [F REL1:421; `CFpre/src/cf_cfdp.c:993-994`].
5. OSAL has one choke point for all named object classes: `OS_ObjectIdAllocateNew` (create), `OS_ObjectIdFinalizeNew` (publication, with the `OS_EVENT_RESOURCE_CREATED` callback), `OS_ObjectIdFindByName` (lookup). An event handler can be registered with `OS_RegisterEventHandler` (`OS1/inc/osapi-common.h:234`); the PSP registers one (`B1/psp/fsw/pc-linux/src/cfe_psp_start.c:341`) [F].
6. The OSAL record stores the creating task (`creator`, `osapi-idmap.c:641`), so the owner of a published object is a framework fact [F].
7. Holds must sit at API entry: a publisher stopped between steps 2 and 4 of 1.4 exposes the -35 path of 1.5 [I]. The old hold plan chose the entry for this reason [F R/kairos/profiles/c1-bp-b051eb0/hold/hold-cf833fdbb.kzh:13].
8. ES deletes the counting semaphores of a task it cleans up (`ES1/cfe_es_apps.c:1582-1583`) [F]; the issue names the stale identifier after a provider restart as a secondary problem [F C1.md:93]. This retraction is outside the C1 order pair.
9. An error exit is not marked by event type: the ES event is INFORMATION; it differs from a normal exit by EID (14 against 13, `B1/cfe/modules/es/fsw/inc/cfe_es_events.h:184, 170`) and by the syslog line of `CFE_ES_ExitApp` [F].
10. The shipped C1 sources contain requirement text in natural language, not machine-readable order facts: "semaphore needs to exist before engine is initialized" (`CFpre/src/cf_events.h:155-156`) and "If a semaphore is expected and not found, the application will terminate during initialization" (/home/user/work/expect_src/CF_281a941/docs/dox_src/cfs_cf.dox:212-214) [F]. The second sentence documents the exit as intended behaviour.
11. C2 offers no positive ground truth. A method's output on C2 can be checked only against framework facts (T1-T3) and against the look-alikes L1-L5, not against a known violation.

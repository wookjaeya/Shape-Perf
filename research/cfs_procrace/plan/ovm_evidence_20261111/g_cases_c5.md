# Phase 1 grounding dossier: MECHANICS CARD C5 (nasa/cFE #72) and the task-identity problem

## Ten-line summary
1. C5 is a spawn-before-publish window inside one framework function: in cFE 6.4.1 `CFE_ES_AppCreate` releases the ES mutex (cfe_es_apps.c:610), creates the app main task (:614), re-locks (:637) and only then registers the task (:649-655, unlock :673).
2. The consume is the new task's self-identity lookup `CFE_ES_GetAppID` (cfe_es_api.c:846-871): it yields `CFE_ES_ERR_APPID` = 0xc4000001 (cfe_error.h:223, severity bits 0xc0000000 = ERROR, cfe_error.h:132-136) while `TaskTable[OS_TaskGetId()].RecordUsed != TRUE`.
3. Both accesses hold the same mutex in different critical sections: no data race in the lockset sense; the order is simply not enforced in 6.4.1.
4. Fix 6.4.2 = lock-scope extension: one critical section :571 -> :669 covers `OS_TaskCreate` (:619) and the registration (:652-658). Nothing else in `CFE_ES_AppCreate` changed except moving one counter [M1].
5. The actors are the framework creator thread and the new application's own main task; it is not application-vs-application.
6. The lookup heals itself after :673, but consumers that ran inside the window keep one-shot losses (EVS not registered, SB pipe owner 0xFFFFFFFF, `CFE_ES_RunLoop` FALSE).
7. NO recorded run shows the violating order: two boots (6.4.1, 6.4.2) have 0 `CFE_ES_ERR_APPID` in 112 / 249 lookups (C5B L27, L231). All "u before w" consequences below are code reading.
8. Step 2a finds the relation without case input: c5_641 row C31 "possible: lock window", flip static-possible; c5_642 "proven-impossible"; but it is 1 of 178 rows and "violation: unconfirmed" [M3-M5].
9. Identity problem (both versions, observed): SAMPLE_APP's thread reported OSAL task id 7 (assigned 8) and got AppId 5 (SAMPLE_LIB) with `CFE_SUCCESS`, because slot 7 kept the `pthread_t` of an ended thread and the C library returned the same value again (C5B L234-243).
10. Hence self-reported ids (pthread_t, OSAL task id, `OS_TaskGetId`, AppId, event app name) are not a sound actor identity; the old E-ID1 "confirmed" for that thread is invalid (AR L46).

Tags: [M] measured in this run (commands at the end), [F] read at path:line, [I] inference, [U] unknown.
Roots: S641 = /home/user/work/c5/src/641/cFE-6.4.1-OSS-release ; S642 = /home/user/work/c5/src/642/cFE-6.4.2-OSS-release ; core = cfe/fsw/cfe-core/src ; OSAL = /home/user/work/c5/src/osal/osal-4.1.1-release/src/os/posix/osapi.c ; V701 = /home/user/work/procrace/cfs_ref/cFS/cfe/modules/es/fsw/src ; T = /home/user/work/redesign/step2a/tests_8c_fix ; R = /home/user/Shape-Perf/research/cfs_procrace ; CASE = R/cases/C5.md ; C5B = R/plan/results_c5_boot_20261029.md ; AR = R/plan/step2a_results_20261030.md ; IA = R/plan/input_audit_20261030.md. Unless a root is named, es/sb/evs paths are under S641/core.

## 1. Source and versions
| Item | Value | Tag |
|---|---|---|
| Issue | nasa/cFE #72 "Race condition within CFE_ES_AppCreate"; body names unlock-relock around `OS_TaskCreate`, `CFE_ES_GetAppID` "will fail", says `CFE_ES_CreateObjects` and `CFE_ES_CreateChildTask` "are safe" | F CASE:57-70 |
| Buggy | cFE 6.4.1 tarball (6.4.0 `cfe_es_apps.c` byte-identical per CASE:320); tree S641 | F CASE:320; code F below |
| Fixed | cFE 6.4.2 tarball; tree S642. No public commit; fix visible only as tarball diff | F CASE:397-404; M1 |
| OSAL | 4.1.1 (osapi-version.h:38-40 in the OSAL root); `S641/osal` holds only readme.txt | F |
| 2a builds | c5_641 = T/c5_641/NP, c5_642 = T/c5_642/NP; their row positions use the same line numbers as S641/S642 (e.g. cfe_es_apps.c:614, :649; cfe_es_api.c:861) | M5 |
| v7.0.1 | buggy pattern absent; replaced by a bounded wait guard (section 8) | F R/cases/mapping_v701.md:74-76; V701/cfe_es_apps.c:559-606 |

## 2. Actors
| Actor | Role | Created at | Tag |
|---|---|---|---|
| Process initial thread | creator on the startup path: `CFE_ES_Main` -> `CFE_ES_StartApplications` (es/cfe_es_start.c:292) -> `CFE_ES_ParseFileEntry` (es/cfe_es_apps.c:307) -> `CFE_ES_AppCreate` (:387), one call per `CFE_APP` script line | process start; PSP calls `CFE_ES_Main` at cfe_psp_start.c:366 of the PSP directory that the cpu1 build selects (S641/build/cpu1/cfe/cfe-config.mak:32; file = S641/psp/fsw/<that PSP>/src/cfe_psp_start.c) | F |
| CFE_ES core task | creator on the run-time paths: Start App command es/cfe_es_task.c:880; restart / reload es/cfe_es_apps.c:1098, :1134 (from `CFE_ES_ProcessControlRequest` :1015) | `CFE_ES_CreateObjects` es/cfe_es_start.c:875 | F |
| New app main task | consumer; entry = the app's main symbol passed unwrapped to `OS_TaskCreate` (es/cfe_es_apps.c:614-620) and by OSAL to `pthread_create` (OSAL:645-648) | es/cfe_es_apps.c:614 (6.4.2: :619) | F |
| Startup script | 5 lines, all `CFE_APP`: SAMPLE_LIB (entry `SAMPLE_LibInit`, prio 0, stack 0), SAMPLE_APP 50, CI_LAB_APP 60, TO_LAB_APP 70, SCH_LAB_APP 80; identical file in 6.4.2 | S641/build/cpu1/exe/cfe_es_startup.scr:1-5 | F; M2 |
Recorded run: 12 tasks; core AppIds EVS 0, SB 1, ES 2, TIME 3, TBL 4; initial thread at SCHED_FIFO priority 99 while inside `CFE_ES_Main`, app tasks 69/66/62/59; 4 CPUs (C5B L107, L112, L127, L131; PSP cfe_psp_start.c:356-357, :378-379 [F]).

## 3. The shared object
- Kind: ES table record `CFE_ES_Global.TaskTable[TaskId]`, fields `RecordUsed`, `AppId`, `TaskId`, `TaskName` (es/cfe_es_apps.c:649-655) [F]. Not a named OSAL object, not created through an API call: plain global memory guarded by the ES mutex `CFE_ES_Global.SharedDataMutex` (created es/cfe_es_start.c:230, taken in `CFE_ES_LockSharedData` es/cfe_es_api.c:1910-1916) [F].
- Key on the writer side: `TaskId = AppTable[i].TaskInfo.MainTaskId` (:641), i.e. the id OSAL stored through the out argument (`*task_id = possible_taskid`, OSAL:690) = index of the first free slot of `OS_task_table` (OSAL:521-527). Computed at run time [F].
- Key on the reader side: `OS_TaskGetId()` (es/cfe_es_api.c:856) = `pthread_getspecific(thread_key) & 0xFFFF` (OSAL:966-969); the value was stored by `OS_TaskRegister` (OSAL:938) as the index of the first slot with `OS_task_table[i].id == pthread_self()` (OSAL:916-928) [F]. If the thread never registered, the key is 0 [I: null thread-specific value].
- So writer key and reader key are computed independently; their equality is a run-time fact. The 2a row says so: "that the two identities are equal is not decided statically" (T/c5_641/NP/relations.txt:2991) [M5].
- Earlier value of the record: `RecordUsed = FALSE` for all slots in `CFE_ES_Main` (es/cfe_es_start.c:213-216); also reset in task cleanup (es/cfe_es_apps.c:1500) [F].

## 4. Publish operation w (6.4.1)
| Step | path:line | Lock state | Tag |
|---|---|---|---|
| App record `AppTable[i]` filled (RecordUsed, names, state INITIALIZING) | es/cfe_es_apps.c:571-608 | ES mutex held (:571) | F |
| Unlock | :610 | released | F |
| `OS_TaskCreate(&AppTable[i].TaskInfo.MainTaskId, AppName, StartAddr, ...)`; inside: slot reserved (OSAL:551), `pthread_create` (OSAL:645), `*task_id` stored (OSAL:690) | :614 | ES mutex NOT held; `MainTaskId` is written without it | F |
| Re-lock | :637 | held | F |
| Probe of stale record: `if (TaskTable[TaskId].RecordUsed == TRUE)` -> syslog "ES Startup: Error: ES_TaskTable slot in use at task creation!" | :643-646 | held | F |
| w: `RecordUsed = TRUE` :649, `AppId = i` :651, `TaskId` :652, `TaskName` :653-655; syslog "ES Startup: %s loaded and created" :656; counters :662, :663, `AppStartupCounter++` :670 | :649-670 | held | F |
| Unlock = w visible to locked readers | :673 | released | F |
Actor: the creator (section 2). Phase: per script line during startup, or any time on the command paths.

## 5. Consume operation u
| Consumer | path:line | Reads | Yield when record absent | Tag |
|---|---|---|---|---|
| `CFE_ES_GetAppID` | es/cfe_es_api.c:846-871 (lock :851, key :856, check :861, read AppId :863) | `TaskTable[k].RecordUsed`, `.AppId` | returns `CFE_ES_ERR_APPID` (init :848); `*AppIdPtr` left unwritten | F |
| `CFE_ES_GetAppIDInternal` (caller holds lock) | :1873-1893 (check :1886) | same | same (:1875) | F |
| `CFE_ES_RunLoop` | :546 lock, :551 lookup, :658-659 | via Internal | syslog "CFE_ES_RunLoop Error: Cannot get AppID for the caller: RC = %08X", returns FALSE | F |
| `CFE_ES_ExitApp` | :440, :445-446, :528-530 | via Internal | no log; unlocks and returns to caller | F |
| `EVS_GetAppID` <- `CFE_EVS_Register`, `CFE_EVS_SendEvent` | evs/cfe_evs_utils.c:122-140; evs/cfe_evs.c:105-107, :223-225 | via GetAppID | status propagated; registration skipped / event not sent, no log | F |
| `CFE_SB_CreatePipe` | sb/cfe_sb_api.c:173, :224, :227, :237 | via GetAppID, status discarded | AppId stays 0xFFFFFFFF and is stored as pipe owner | F |
`CFE_ES_ERR_APPID` definition: S641/core/inc/cfe_error.h:223 = 0xc4000001; `CFE_SEVERITY_BITMASK` 0xc0000000, `CFE_SEVERITY_ERROR` 0xc0000000 (cfe_error.h:132-136) [F]. Its header comment reads "The given application ID does not reflect a currently active application" (cfe_error.h:219-221): the same code is also the designed answer of argument validation (section 11).
Actor of u: the new app main task. First calls of the shipped apps: `CFE_ES_RegisterApp` then `CFE_EVS_Register` then `CFE_SB_CreatePipe`, return values unused in sample_app (S641/apps/sample_app/fsw/src/sample_app.c:82, :87, :95-97), same order in ci_lab (ci_lab_app.c:146-152) and to_lab (to_lab_app.c:156-177) [F]. `CFE_ES_RegisterApp` itself takes the ES mutex (:771) and calls `OS_TaskRegister` (:776) but does not read `TaskTable`: it is not a consumer [F].

## 6. Order enforcement
- 6.4.1: none between :614 and :637. The mutex is free in that interval, so a child lookup can complete there and see `RecordUsed == FALSE`. Both orders feasible; "u before w" is possible sometimes [F code; I].
- What decides it: whether the child reaches its first lookup (after its own `CFE_ES_RegisterApp` lock/unlock) before the creator re-takes the mutex at :637. Scheduling inputs visible in the system: script priorities, creator priority (initial thread raised to the maximum around `CFE_ES_Main`, PSP cfe_psp_start.c:356-357; command path: CFE_ES task priority), CPU count; OSAL applies priorities only when `geteuid() == 0` (OSAL:587) and creates mutexes with `PTHREAD_PRIO_INHERIT` and `PTHREAD_MUTEX_RECURSIVE` (OSAL:3161, :3177 region) [F]. No reference fixes these for the original report [F CASE:461-463].
- 6.4.2: enforced by synchronization for every mutex-protected read on the path where the registration runs: the child's first call `CFE_ES_RegisterApp` already blocks on the mutex (S642 es/cfe_es_api.c:706-715) until :669 [F; I].
- Not covered by 6.4.2 [F code, I for effect]: (a) `OS_TaskCreate` can return `OS_ERROR` after the thread has started (`pthread_detach` / `pthread_attr_destroy` failure, OSAL:663-672, :675-684); then `CFE_ES_AppCreate` takes the error branch (S642 es/cfe_es_apps.c:628-636) and never registers, while the child runs and gets `CFE_ES_ERR_APPID` for good; (b) readers without the mutex, e.g. `CFE_ES_IncrementTaskCounter` (S641 es/cfe_es_api.c:1203-1209; S642 :1142-1148).

## 7. Consequence of "u before w" in 6.4.1 (all [I] from the code lines cited in section 5; never observed)
| Call that falls in the window | Immediate effect | Later healing | Steady-state difference vs "w before u" |
|---|---|---|---|
| `CFE_EVS_Register` | returns 0xc4000001; `AppData[i].RegisterFlag` stays FALSE (evs/cfe_evs.c:107-128 skipped) | none: apps call it once | every later `CFE_EVS_SendEvent` returns `CFE_EVS_APP_NOT_REGISTERED` 0xc2000002 (cfe_error.h:174); first one emits event `CFE_EVS_ERR_UNREGISTERED_EVS_APP` = 41, type ERROR, "App %s not registered with Event Services. Unable to send event." plus the same syslog line, once per app (evs/cfe_evs_utils.c:194-218; id at inc/cfe_evs_events.h:666) |
| `CFE_SB_CreatePipe` | pipe owner stored as 0xFFFFFFFF, owner name empty (`CFE_ES_GetAppName` rejects ids >= max, es/cfe_es_api.c:886-902) | none | a `CFE_SB_Subscribe` made after the window compares owner with the now valid AppId (sb/cfe_sb_api.c:622, :636) -> event `CFE_SB_SUB_INV_CALLER_EID` = 51, ERROR, "Subscribe Err:Caller(%s) is not the owner of pipe %d,Msg=0x%x", returns `CFE_SB_BAD_ARGUMENT` 0xca000003 (:639-642; ids inc/cfe_sb_events.h:750, cfe_error.h:794); a subscribe still inside the window passes the check (both 0xFFFFFFFF, :610) |
| `CFE_ES_RunLoop` | syslog line, FALSE -> app leaves its loop (sample_app.c:52, :68) | none | app calls `CFE_ES_ExitApp`: after the window -> syslog "Application %s called CFE_ES_ExitApp", state STOPPED, endless `OS_TaskDelay(500)` (es/cfe_es_api.c:507-524); inside the window -> silent return, entry function returns, thread ends with its OSAL slot and ES records still "in use" |
| `CFE_ES_GetAppID` itself | 0xc4000001 | yes: any call after :673 returns `CFE_SUCCESS` | none for the lookup itself |
The creator side shows nothing: it logs "loaded and created" and returns `CFE_SUCCESS` (:656, :683) in both orders [F].

## 8. The fix
- 6.4.2 diff of `CFE_ES_AppCreate` [M1]: unlock before `OS_TaskCreate` removed (6.4.1 :610), both re-locks removed (:628, :637), `AppStartupCounter++` (:670) replaced by `++AppStartedCount` before the spawn (S642 :607). Lock :571, spawn :619, failure unlock :634, registration :652-658, unlock :669 [F].
- Class: dependence kept, order enforced by synchronization (lock-scope extension). Not a retry, not removal. Consumers `sb/cfe_sb_api.c`, `evs/cfe_evs.c`, `sample_app.c` are byte-identical in both versions [M2]; `CFE_ES_GetAppID` has the same body at S642 es/cfe_es_api.c:785-808 (check :800) [F].
- 6.4.2 changed other ES code too (startup sync redesign in `CFE_ES_RunLoop` / `CFE_ES_WaitForStartupSync`): a version diff is not a clean single-fix diff [M1].
- v7.0.1 uses a third mechanism: `OS_TaskCreate` again before the lock (V701/cfe_es_apps.c:664, lock :672), and the entry wrapper `CFE_ES_TaskEntryPoint` (:613) polls `CFE_ES_GetTaskFunction` (:559-606): tolerated by bounded retry, period `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` (50), bound `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` (1000), values at /home/user/work/procrace/cfs_ref/cFS/cfe/cmake/sample_defs/example_platform_cfg.h:838, :856; on timeout returns `CFE_ES_ERR_APP_REGISTER` [F].

## 9. What a generic observer sees in 6.4.1
(a) Framework API boundary: creator: `OS_TaskCreate(out id, name, entry, ...)` returns `OS_SUCCESS`, `CFE_ES_AppCreate` returns `CFE_SUCCESS`; child: `CFE_ES_RegisterApp` -> `CFE_SUCCESS`; `CFE_ES_GetAppID` -> 0xc4000001 (only if called directly by app code; inside `CFE_EVS_Register` / `CFE_SB_CreatePipe` / `CFE_ES_RunLoop` it is an internal call); `CFE_EVS_Register` -> 0xc4000001; `CFE_SB_CreatePipe` -> `CFE_SUCCESS` (sb/cfe_sb_api.c:270) although the owner is invalid, with DEBUG event `CFE_SB_PIPE_ADDED_EID` = 5 "Pipe Created:name %s,id %d,app %s" (:264-268; inc/cfe_sb_events.h:133); later `CFE_SB_Subscribe` -> 0xca000003, `CFE_EVS_SendEvent` -> 0xc2000002. The lookup takes no key argument: the key is computed inside the callee [F code; I].
(b) Events / syslog: none at the moment of the violating lookup, except the RunLoop syslog line. Delayed: event 41 (ERROR, once per app), sent by EVS itself through its internal `EVS_SendEvent` (evs/cfe_evs_utils.c:211), and event 51 (ERROR), sent by SB under `CFE_SB.AppId` (sb/cfe_sb_api.c:639): neither carries the victim as sender; the victim appears only inside the text. The syslog "loaded and created" marks w [F code; I].
(c) Memory: child load of `TaskTable[k].RecordUsed` (es/cfe_es_api.c:861) returning the initial 0, before the creator's store of 1 (es/cfe_es_apps.c:649); both under the same mutex, different critical sections; creator store to `AppTable[i].TaskInfo.MainTaskId` through OSAL:690 with no ES lock [F code; I].
Recorded runs (C5B, instrumented derived builds, one boot each, as root with real-time scheduling): 6.4.1 68 `GetAppID` occurrences + 44 internal lookups, 6.4.2 67 + 182; `CFE_ES_ERR_APPID` 0 in both; first lookups of the four app tasks came +0.024 to +0.152 ms after their own registration (C5B L25-27, L205-212, L231-232). No inter-actor happens-before edge was derived in the replay (C5B L168-169). The old judgement "all READY" must not be quoted as is (AR L46).
Step 2a, no case input: c5_641 = 178 rows (A 66, B 73, C 39), grades possible 11, proven-impossible 6, undecidable 147, not graded 14 (relations.txt:20) [M3]. Row C31 (relations.txt:2986-2996): spawn :614 with "locks held {}", write :649 under lock acquired at :637, read :861, "feasibility: possible: lock window", "flip: static-possible: CFE_ES_GetAppID returns 3288334337 [CFE_ES_ERR_APPID] with the earlier value", "violation: unconfirmed ... no record given" [M5]. c5_642 = 175 rows, possible 0, proven-impossible 14 (relations.txt:20); the matching row is C30 (relations.txt:2940) [M4].

## 10. Ground truth table
| Version | Order | Lookup status / value | Events, syslog | App state / steady state | Tag |
|---|---|---|---|---|---|
| 6.4.1 | w before u | `CFE_SUCCESS`, AppId = i (CI 7, TO 8, SCH 9) | startup events of the app appear; no event 41 / 51 | registered with EVS, pipe owner = i, RunLoop TRUE | M-run: C5B L205-208, L231 |
| 6.4.1 | u before w | `CFE_ES_ERR_APPID` 0xc4000001, out value unwritten | section 7: possibly event 41, event 51, RunLoop syslog; nothing at the lookup itself | section 7 losses persist; lookup heals after :673 | F code for status; I for consequences; U whether ever reached |
| 6.4.2 | w before u | as 6.4.1 row 1 | same | same | M-run: C5B L209-212, L232; F code |
| 6.4.2 | u before w | not reachable for mutex-protected reads when the registration runs | - | - | F code S642 :571-669; M4 (tool "proven-impossible", scope limits AR L231) |
| 6.4.2 | spawn succeeded, creator reports failure (OSAL:663-684) | `CFE_ES_ERR_APPID` permanently | syslog "ES Startup: AppCreate Error: TaskCreate %s Failed" (S642 :630-631) | task runs unregistered | F code; I; U reachability |
| both | identity mix-up (section 15) | `CFE_SUCCESS` with another app's AppId (5 instead of 6) | SAMPLE_APP events printed under name SAMPLE_LIB | pipe owner 5; 18 / 64 such lookups | M-run: C5B L205, L209, L231-243 |

## 11. Benign look-alikes (negative controls)
| # | Look-alike | path:line | Why not a violation | Tag |
|---|---|---|---|---|
| N1 | `CFE_ES_CreateChildTask`: spawn then register, same record type | es/cfe_es_api.c:1076 lock, :1108 spawn, :1116 register, :1145 unlock | lock held across; 2a grades its 6 rows proven-impossible in 6.4.1 too (relations.txt:2669-2832) | F; M4 |
| N2 | `CFE_ES_CreateObjects` (core tasks) | es/cfe_es_start.c:838 lock, :875 spawn, :910-921 register, :933 unlock | lock held across; note 2a cannot resolve its entry (union member), relations.txt:19 | F; M |
| N3 | Creator's own stale-record probe | es/cfe_es_apps.c:643-646 | designed existence check by the writer before its own write | F |
| N4 | App slot reservation: test `RecordUsed == FALSE` then set TRUE | es/cfe_es_apps.c:449-460 | check-then-reserve inside one critical section | F |
| N5 | `CFE_ES_GetAppName(AppId)` returning `CFE_ES_ERR_APPID` | es/cfe_es_api.c:880-902 | validation of a caller-supplied id; same status code, different meaning. 2a rows C19, C20 grade it "possible" although `AppTable[i].RecordUsed` is set before the spawn (:576) | F; M4 |
| N6 | `AppStartupCounter` read in `CFE_ES_RunLoop` | es/cfe_es_api.c:591-600; write es/cfe_es_apps.c:670 | the read runs only after a successful lookup under the same lock (:553), so it cannot precede :673; 2a row C21 still says "possible: lock window" (relations.txt:2878-2884) | F; M4; I |
| N7 | `CFE_ES_GetTaskInfo(TaskId)` reads of the same record | es/cfe_es_api.c:965, :971 | key is an argument, not the caller's identity; rows C23, C32 "possible" | M4 |
| N8 | `EVS_NotRegistered` one-shot event | evs/cfe_evs_utils.c:194-218 | designed reaction to an unregistered sender; fires for any app that never registers | F |
| N9 | `OS_TaskRegister` search returning `OS_ERR_INVALID_ID`; `OS_TaskCreate` free-slot probe then reserve | OSAL:921-933; OSAL:519-551 | designed lookup / probe-create under `OS_task_table_mut` | F |
| N10 | v7.0.1 wait guard | V701/cfe_es_apps.c:559-606 | early read is expected and retried with a bound from configuration | F |
| N11 | A thread whose entry returns at once (SAMPLE_LIB as `CFE_APP`) | S641/apps/sample_lib/fsw/src/sample_lib.c:32-43; script line 1 | never looks up anything; its record stays `RecordUsed` forever | F |
Of the 11 "possible" rows in c5_641 (C19-C25, C31-C34), the case proper is C22, C24, C25, C31, C33, C34 (self-identity reads through `GetAppID`, `ExitApp` :445, `RunLoop` :551); C19, C20, C21, C23, C32 are the neighbours above [M4; I for the split].

## 12. What a person wrote for C5 in the old path
- rspec `K-SELF.es-task` / `K-SELF.es-task-internal`: "use op CFE_ES_GetAppID; key es.task(arg.task_id); requires state(...) = LIVE" (R/kairos/profiles/c5-cfe-6.4.x/40_rspec.kzp:22-42) [F]; kind `es.task` with states FREE < LIVE (10_kinds.kzp:11-12) [F]; a pseudo-parameter `task_id` because the key is computed in the callee (IA:274) [F].
- Ops for lookup, publish and the other writers found by hand (IA:244-246); the role "sb-create-pipe-dependent-lookup" (IA:216); the provenance of callers and result use (IA:230-231) [F].
- `appreq`, `allowed`, `harm` all `unspecified_by_reference` (40_rspec.kzp:26-28) [F]: the old path never had a harm oracle for C5.
- Hand-placed observation sites (instr/sites-641.draft.kzs, 33 sites; IA:22, IA:253) and hand-listed modules for the closure (IA:300) [F].
- A new method must itself produce: the record and its callee-computed key; the writer set and the publish point (store plus lock release); the required state (branch `RecordUsed == TRUE` -> success); that 0xc4000001 is a failure (severity bits); which callers discard the status; where to observe; and the actor identity of section 15.

## 13. Classification
- Race-like order violation of the "spawn before publish" kind: order not enforced in 6.4.1, both orders feasible, no data race (same mutex, two critical sections) [I].
- Inside the framework: writer = ES creator context acting for the application, reader = that application's own task through a framework API. Not between two applications [F code].
- Mixed outcome: the lookup heals; side effects taken inside the window are one-shot losses with no retry in 6.4.1 [I].
- The identity mix-up of section 15 is NOT an order violation (C5B L248): the own record was already registered; the task read another task's record.

## 14. Unknowns
- U1 The violating order was never recorded; its consequences (section 7) are unobserved. One boot per version only (C5B L278-281).
- U2 Reporter's version details, application, creation path, schedule, failure log: none public (F CASE:447-449, :459-465).
- U3 Whether the C library binary of the derived build stores the `pthread_t` out value before the new thread runs (source reading says yes for three library versions, F CASE:490; binary not checked, F CASE:494) and why it returned the same `pthread_t` twice (C5B L284).
- U4 Reachability of the "spawn succeeded, creator sees failure" path and of a stale-`RecordUsed` read (F CASE:497).
- U5 A residual window in v7.0.1 (wait guard checks the task record only) is an unverified inference of the case file (F CASE:41, :435); I verified only the create-then-lock order and the guard loop.
- U6 Why the replay produced 0 inter-actor happens-before edges (C5B L169, L305), and whether the E-ID1 defect was fixed in the engine.
- U7 Contents of the 6.4.2 keep-tree build versus S642 beyond matching line numbers in the rows I read.

## 15. Identity problem (C5B section 4.3; AR section 0 item 6)
Chain of self-reported ids: `pthread_self()` -> first slot with equal `OS_task_table[i].id` (OSAL:916-928) -> thread-specific value (OSAL:938) -> `OS_TaskGetId()` (OSAL:959-972) -> `TaskTable[k].AppId` (es/cfe_es_api.c:863) -> EVS / SB records and printed app name [F].
How a task gets the wrong application:
| Path | Mechanism | Evidence |
|---|---|---|
| P1 reused `pthread_t` (observed, both versions) | SAMPLE_LIB is started as an app task whose entry returns immediately (script line 1; sample_lib.c:32-43). The thread is detached (OSAL:663) and ends; only `OS_TaskDelete` / `OS_TaskExit` clear a slot's `.id` (OSAL:761-767, :793-799), so slot 7 keeps it. The next `pthread_create` (SAMPLE_APP, slot 8) returned the same `pthread_t` (0xf7b42b40 / 0xf7b83b40). `OS_TaskRegister` scans from slot 0, ignores `free`, stops at 7. `GetAppID` reads `TaskTable[7]`: `RecordUsed` TRUE, AppId 5 | F code; M-run C5B L234-243; reuse cause I/U |
| P2 unregistered thread | no `OS_TaskRegister` -> key 0 -> `TaskTable[0]` = first core task (CFE_EVS, AppId 0; first `CFE_ES_CORE_TASK` entry es/cfe_es_objtab.c:120) -> `CFE_SUCCESS` with AppId 0 | F code; I; not observed (C5B L231 last column 0) |
| P3 stale ES record | `RecordUsed` left TRUE from an earlier task in that slot; a read in the window returns the old AppId with `CFE_SUCCESS` | I; F CASE:497; creator probe :643-646 |
Observed effects of P1: 18 (6.4.1) and 64 (6.4.2) successful lookups returning 5 where the creator's own `i` was 6; SB pipe owner 5; console events of SAMPLE_APP printed as "SAMPLE_LIB" (C5B L231-243) [F]. It is independent of the lock-scope fix (C5B L249).
Why the old identity scheme failed: E-ID1 checked one id-assigning call and one announcing actor and never compared the spawn's output value with the child's own thread value, so two spawns with equal output could not be separated; it bound SAMPLE_APP's thread to owner 5 as "confirmed". That result and the READY verdicts built on it are void (AR L46; C5B L300-302). The record did hold discriminating data: announced id 7 vs id assigned by the spawn that created that actor 8 (C5B L302).
Observations for a designer (requirements read off the facts, not a design) [I]:
1. No value the task reports about itself is a sound identity: `pthread_t` and OSAL slot index are reused, `OS_TaskGetId` is derived from them, AppId and the event app name are derived from `OS_TaskGetId`.
2. A sound thread instance must come from the creation edge: the creator's spawn call instance (and its out values) tied to the first execution of the child, with a per-run unique instance number; a second spawn with the same out value is a new instance.
3. Thread end (entry function return) must be observed directly; OSAL and ES tables keep an ended thread "in use".
4. Thread -> application must come from the creator's request chain (`CFE_ES_AppCreate` call instance: slot `i`, name, script line -> `OS_TaskCreate` call instance -> thread instance), not from `CFE_ES_GetAppID`.
5. The record a lookup reads must be taken from the access itself (index actually used at :861), then compared with the record its creator published; "reads another task's record" is a distinct finding from "reads its own record too early".
6. Event-based oracles inherit the fault: events of one application can carry another application's name.

## Observations a designer should know (not a design)
- O1 The publish is not an API call with a status: it is a set of field stores closed by a mutex release; the consume is an API call whose key is implicit (caller identity).
- O2 The failing status never crosses the application/framework boundary in the shipped apps unless the app calls `CFE_ES_GetAppID` itself: it is consumed inside EVS/SB/ES and partly discarded (sb/cfe_sb_api.c:224, :622).
- O3 `CFE_ES_ERR_APPID` is produced by designed validation too (N5) and by any thread that is simply not a cFE task; status alone does not separate C5 from those.
- O4 The framework itself states the dependence in the 6.4.1 header comment of `CFE_ES_RegisterApp` ("must be called in a cFE app before any other cFE function", es/cfe_es_api.c:762-764) but nothing in the source states "registration by the creator precedes the task's first lookup" except the code shape of N1/N2 versus `CFE_ES_AppCreate`: three creators of the same record type, two hold the lock across the spawn, one does not [F].
- O5 Contradiction between documents: C5B L29/L182 ("SAMPLE_APP owner 5 confirmed") is withdrawn by AR L46. Source and CASE agree on every line number I checked; no source/document contradiction found for the C5 mechanism.

## Measurement commands
- M1 `diff <(sed -n '420,720p' S641/core/es/cfe_es_apps.c) <(sed -n '420,716p' S642/core/es/cfe_es_apps.c)` -> 5 hunks: counter added before spawn, unlock :610 removed, re-locks :628 and :637 removed, counter :670 removed; `diff S641/core/es/cfe_es_api.c S642/core/es/cfe_es_api.c` -> RunLoop / WaitForStartupSync rewritten.
- M2 `cmp` 641 vs 642 of core/sb/cfe_sb_api.c, core/evs/cfe_evs.c, apps/sample_app/fsw/src/sample_app.c, build/cpu1/exe/cfe_es_startup.scr -> identical; core/evs/cfe_evs_utils.c first differs at line 615.
- M3 `grep -n 'feasibility: ' T/c5_641/NP/relations.txt | sed ... | sort | uniq -c` -> 14 not graded, 11 possible, 6 proven-impossible, 147 undecidable; `grep -n '^summary' T/c5_64{1,2}/NP/relations.txt`.
- M4 `grep -n 'lock window\|proven-impossible' T/c5_641/NP/relations.txt`; `grep -n '^  C[0-9]* ' ...`; `grep -c proven-impossible T/c5_642/NP/relations.txt` (16 text hits, 14 graded rows per the summary line).
- M5 `sed -n '2984,2996p' T/c5_641/NP/relations.txt`.

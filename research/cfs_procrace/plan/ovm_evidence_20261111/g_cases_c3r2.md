# g_cases_c3r2 — mechanics cards: C3 (nasa/cFE #73) and R2 (nasa/cFE #2663)

## Ten-line summary
1. Both cases turn on one field: the Software Bus (SB) service's own application ID (6.4.1 `CFE_SB.AppId`, v7.0.1 `CFE_SB_Global.AppId`). It is stored only by the SB task in `CFE_SB_AppInit` and loaded, with no synchronization, by every SB API event call, which runs in the CALLER's task.
2. C3: buggy cFE 6.4.1, fixed 6.4.2. Both trees are on disk and are exactly the 2a builds `c5_641` / `c5_642`. No run and no 2a completion test ever targeted C3.
3. R2: the reported revision is not on disk. Verified at cFE c5fb2b4 (v7.0.1 = 2a build `r2_core`), where ES and EVS are created before SB. No fixed version exists (issue open).
4. C3 is race-like: 6.4.1 creates the five core tasks without waiting. If TIME's `CFE_SB_CreatePipe` runs before SB and EVS init, the event path reads the SB ID (0, which is a VALID ID: EVS's), finds that ID unregistered, and then indexes an array with EVS's own placeholder ID 0xFFFFFFFF with no check: out-of-bounds access (a crash in the report).
5. R2 is a fixed-order premature use: serialized core startup forces the whole init of ES and EVS before the SB task exists. The nested `CFE_EVS_SendEventWithAppID(…, AppID 0, …)` returns `CFE_EVS_APP_ILLEGAL_APP_ID` (0xc2000003, error severity), discarded at all 39 sites. Events are lost; nothing else differs.
6. Shared cause: yes for (object, publish site, consume sites); no for what decides the order, for the second object, and for the signal. The 6.4.2 fix left that dependency: EVS's `CFE_SB_CreatePipe` precedes SB's publication on every 6.4.2 boot, silently (status `CFE_SUCCESS`).
7. Recorded evidence: R2 = 24 calls (CFE_ES 20, CFE_EVS 4) returning 0xc2000003 in both W6 recordings. C3 = none; the one natural 6.4.1 boot completed startup.
8. Traps: in C3 no status marks the SB ID as absent, and the fatal read has no framework status at all. In both cases the key at the consume (0) does not name the publisher, and the failing call is framework-to-framework, never an application-facing return.
9. Look-alikes checked in source: TBL's `TableTaskAppId` (same shape, order enforced at v7.0.1), EarlyInit-backed early uses, the lock-protected ES task record, bounded polls, later callers of the same sites.
10. Open: C3's fault behaviour and natural order in the local build; console content of the 6.4.x boots; R2 reporter's revision and its "ES" claim; an unreported creator/child window that would leave the SB ID undefined for a whole run.

## 0. Conventions
- Tags: [F] read by me in this run at the cited path:line (every path:line below was opened in this run; a bare path:line is [F]); [M] measured, command in §M; [I] my inference from the cited lines; [U] unknown; [F: case] taken from a case file only (source not on disk).
- Path prefixes: `641:` = `/home/user/work/c5/src/641/cFE-6.4.1-OSS-release/cfe/fsw/cfe-core/src/`; `642:` = same under `/home/user/work/c5/src/642/cFE-6.4.2-OSS-release/`; `641cfg:` / `642cfg:` = `…/cfe/fsw/platform_inc/cpu1/cfe_platform_cfg.h` of that tree; `701:` = `/home/user/work/procrace/cfs_ref/cFS/cfe/modules/`; `rel(<build>)` = `/home/user/work/redesign/step2a/tests_8c_fix/<build>/NP/relations.txt`; `R/` = `/home/user/Shape-Perf/research/cfs_procrace/`; `W6` = `R/plan/results_w6_20261029.md`; `C5B` = `R/plan/results_c5_boot_20261029.md`; `AR` = `R/plan/step2a_results_20261030.md`.
- Source vs project documents: every 641/642 line pin of `R/cases/C3.md` that I opened matches the trees. One document statement is contradicted by the source: `R/cases/mapping_v701.md:32` says the core order "is fixed by `modules/es/fsw/src/cfe_es_objtab.c`"; at c5fb2b4 that file holds only ES's own entry (701:es/fsw/src/cfe_es_objtab.c:35-42) and the order comes from a generated table (R2 card, field 2).

---

# CARD C3 — nasa/cFE #73 "Race conditions / dependencies between CFE core apps"

**1. Source and versions**
- Issue #73 = migrated Trac #42 [F: case C3.md:28]. Reported outcome: "segfaults and crashes CFE core" on the reporter's hardware platform [F: case C3.md:177]; no log, address or frequency published [F: case C3.md:643].
- Buggy: cFE 6.4.1 (tarball, no git revision), tree `/home/user/work/c5/src/641/cFE-6.4.1-OSS-release` (its `osal/` holds only `readme.txt` [M1]). Fixed: cFE 6.4.2, tree `/home/user/work/c5/src/642/cFE-6.4.2-OSS-release`. "Found in 6.4.1" for this ticket is the case file's inference from a version-document sentence [F: case C3.md:34].
- 2a builds: `c5_641` (buggy) and `c5_642` (fixed); their core source is byte-identical to the two trees above [M1]. They are named after C5 but are C3's two versions as well.
- The 6.4.1→6.4.2 core diff is 13 files [M2] and mixes three tickets (Trac #40, #41, #42) [F C5B:270]: a plain version comparison changes C3's and C5's fixes together.

**2. Actors**
| Actor | Created by / where | Notes |
|---|---|---|
| ES main thread (process initial thread) | runs `CFE_ES_Main` → `CFE_ES_CreateObjects` (641:es/cfe_es_start.c:273, :803) | runs all EarlyInit functions; creates everything else |
| Core tasks CFE_EVS, CFE_SB, CFE_ES, CFE_TIME, CFE_TBL | `OS_TaskCreate` at 641:es/cfe_es_start.c:875, in the loop :812-:992, in table order 641:es/cfe_es_objtab.c:120, :122, :124, :126, :129 | priorities 61 / 64 / 68 / 60 / 70 (641cfg:1115, :1138, :1161, :1188, :1217; lower number = higher priority, :1186). App records of type `CFE_ES_APP_TYPE_CORE` (:845); AppId = first free AppTable slot (:823-:830): EVS 0, SB 1, ES 2, TIME 3, TBL 4 [F C5B:112] |
| TIME child tasks (tone, 1 Hz) | `CFE_ES_CreateChildTask` 641:time/cfe_time_task.c:280, :294, priority 25 (641cfg:1189-:1190) | created before TIME's `CFE_SB_CreatePipe` |
| Startup-script applications | `CFE_ES_StartApplications` 641:es/cfe_es_start.c:292 | started right after `CFE_ES_CreateObjects`, with no wait for core init |
Roles in the issue: consumer = TIME task; publishers = SB task and EVS task; ES main writes the placeholders.

**3. Shared objects** (three; the crash needs all three unpublished)
| Id | Object | Kind, identity | Pre-publication value |
|---|---|---|---|
| O1 | `CFE_SB.AppId` | global field (`uint32`, 641:sb/cfe_sb_priv.h:287; object 641:sb/cfe_sb_task.c:105). Holds SB's ES AppId: computed at run time by ES slot allocation, not a literal, not a table | static zero; never stored before w1: `CFE_SB_EarlyInit` (641:sb/cfe_sb_init.c:121-:166) does not write it and the only writer is 641:sb/cfe_sb_task.c:198 [M3]. **0 is a valid AppId (EVS's)** |
| O2 | `CFE_EVS_GlobalData.EVS_AppID` | global field (641:evs/cfe_evs_task.h:165). EVS's own AppId | `CFE_EVS_UNDEF_APPID` = 0xFFFFFFFF (641:evs/cfe_evs_task.h:105), stored by ES main in `CFE_EVS_EarlyInit` (641:evs/cfe_evs_task.c:190) |
| O3 | `CFE_EVS_GlobalData.AppData[id]` (`RegisterFlag`, `ActiveFlag`, `EventTypesActiveFlag`, `EventCount`) | EVS table record keyed by AppId; array of `CFE_ES_MAX_APPLICATIONS` = 32 (641cfg:570), first member of the global (641:evs/cfe_evs_task.h:154) | zero, by the `CFE_PSP_MemSet` in `CFE_EVS_EarlyInit` (641:evs/cfe_evs_task.c:188) |

**4. Publish operations**
| Id | Operation | Details |
|---|---|---|
| w1 | `CFE_ES_GetAppID(&CFE_SB.AppId);` 641:sb/cfe_sb_task.c:198 | in `CFE_SB_AppInit` (:184), called at :127 from `CFE_SB_TaskMain` (:121), after `CFE_ES_RegisterApp` (:190). The store is `*AppIdPtr = …TaskTable[TaskId].AppId` at 641:es/cfe_es_api.c:863, under the ES shared-data lock (:851-:867). Return value ignored. Actor: SB task |
| w2 | `EVS_GetAppID(&CFE_EVS_GlobalData.EVS_AppID)` 641:evs/cfe_evs_task.c:385 | in `CFE_EVS_TaskInit` (:372, called :320); same store site 641:es/cfe_es_api.c:863 reached through 641:evs/cfe_evs_utils.c:127. Status checked (:386). Actor: EVS task. It is the 2nd step of EVS init: RegisterApp :377, w2 :385, register :393, CreatePipe :401 |
| w3 | `CFE_EVS_Register` 641:evs/cfe_evs.c:95 | `RegisterFlag = TRUE` :128, `ActiveFlag` :129, `EventCount = 0` :130, `EventTypesActiveFlag = CFE_EVS_DEFAULT_TYPE_FLAG` :131 (0xE = Debug off, 641cfg:1419, :1424). No lock. Called by SB for its own ID at 641:sb/cfe_sb_task.c:257 and by EVS for its own ID at 641:evs/cfe_evs_task.c:393, each in its own task |

**5. Consume operation u** (plain nested calls, all in the calling task)
- Path: `CFE_TIME_TaskMain` (641:time/cfe_time_task.c:187) → `CFE_TIME_TaskInit` (:193, :241): `CFE_ES_RegisterApp` :245, `CFE_EVS_Register(NULL,0,0)` :252, `OS_BinSemCreate` :259, :269, `CFE_ES_CreateChildTask` :280, :294, then **`CFE_SB_CreatePipe`** :308.
- `CFE_SB_CreatePipe` (641:sb/cfe_sb_api.c:171) ends with `CFE_EVS_SendEventWithAppID(CFE_SB_PIPE_ADDED_EID, CFE_EVS_DEBUG, CFE_SB.AppId, …)` at :264: the load of O1, after `CFE_SB_UnlockSharedData` (:261). The result is discarded and the function then returns `CFE_SUCCESS` (:270) whatever the event call returned. Event ID 5 (641:inc/cfe_sb_events.h:133).
- `CFE_EVS_SendEventWithAppID` (641:evs/cfe_evs.c:277): `AppID >= CFE_ES_MAX_APPLICATIONS` → `CFE_EVS_APP_ILLEGAL_APP_ID` (:285-:287); else `AppData[AppID].RegisterFlag == FALSE` (load of O3, :289) → `EVS_NotRegistered(AppID)` (:292); else `EVS_IsFiltered` (:294).
- `EVS_NotRegistered` (641:evs/cfe_evs_utils.c:194): only on the first event of that AppId (`EventCount == 0`, :199): `UnregisteredAppCounter++` :202, `EventCount++` :205, `CFE_ES_GetAppName` :208, `EVS_SendEvent(CFE_EVS_ERR_UNREGISTERED_EVS_APP, CFE_EVS_ERROR, "App %s not registered with Event Services. Unable to send event.")` :211 (event ID 41, 641:inc/cfe_evs_events.h:666), same text to the syslog :215. Always returns `CFE_EVS_APP_NOT_REGISTERED` (:218).
- `EVS_SendEvent` (641:evs/cfe_evs_utils.c:608): `EVS_IsFiltered(CFE_EVS_GlobalData.EVS_AppID, …)` at :616 is the load of O2, **with no validity check**. `EVS_IsFiltered` (:235) does `AppDataPtr = &CFE_EVS_GlobalData.AppData[AppID]` (:244) with no bounds check (comment :243: "Caller has verified that AppID is good and has registered with EVS"), reads it (:246) and may write `FilterPtr->Count++` (:313). If that out-of-range record happens to read as "not filtered", `EVS_SendPacket(CFE_EVS_GlobalData.EVS_AppID, …)` follows (:641) and reads and writes `AppData[AppID].EventCount` (:449-:451), again out of range.

What u yields, by the state it meets (the table is [I], derived from the lines above):
| State at u | Yield |
|---|---|
| O1 unpublished (0), O3[0] unregistered, O2 placeholder | undefined behaviour: read (possibly write) of `AppData[0xFFFFFFFF]`. If it returns: `CFE_EVS_APP_NOT_REGISTERED` = 0xc2000002 (641:inc/cfe_error.h:174; severity bits 11 = Error, :90-:104, `CFE_SEVERITY_ERROR` :136), plus a syslog line that names app 0, "CFE_EVS" [I: 641:es/cfe_es_api.c:888-:891 copies the name set at 641:es/cfe_es_start.c:850] |
| O1 unpublished, O2 published, O3[0] unregistered (EVS between :385 and :393) | 0xc2000002 and the syslog line; the ERROR event is dropped because `ActiveFlag` is FALSE (641:evs/cfe_evs_utils.c:246-:249) |
| O1 unpublished, O3[0] registered (EVS past :393) | **`CFE_SUCCESS`**: the SB event is judged against EVS's record and masked as DEBUG. Silent misattribution to app 0; no error anywhere |
| O1 published (1), O3[1] unregistered (SB between :198 and :257) | 0xc2000002; with EVS up: ERROR event ID 41 "App CFE_SB not registered…" and the syslog line; with O2 still a placeholder: out-of-bounds as in row 1 |
| all published | `CFE_SUCCESS`; the DEBUG event is masked by SB's own type flag |
No status ever tells the caller that O1 itself is unpublished: 0 is indistinguishable from a real ID.

**6. Order enforcement in 6.4.1**
- Between core tasks: none. The loop 641:es/cfe_es_start.c:812-:992 creates the five tasks back to back, and no core task calls `CFE_ES_WaitForStartupSync` or `CFE_ES_RunLoop` (only a comment at 641:es/cfe_es_task.c:210) [M4].
- What is enforced: (a) the ES record of a new task: the ES shared-data lock is held from :838 over `OS_TaskCreate` (:875) and `TaskTable[…].AppId = j` (:919) to :933, and the child's first calls take the same lock (`CFE_ES_RegisterApp` 641:es/cfe_es_api.c:771, `CFE_ES_GetAppID` :851); (b) EarlyInit before any task: function entries 641:es/cfe_es_objtab.c:92-:106 precede task entries :120-:129 in one sequential walk, so the placeholders of O2/O3 and SB's tables exist before any task runs.
- The SB mutex in `CFE_SB_CreatePipe` (641:sb/cfe_sb_api.c:182 to :261) covers the pipe table, not O1, which is read after the unlock.
- "u before w": **possible sometimes; both orders are feasible** [I]. It is decided by scheduling: the priority of the creating thread relative to the new tasks, the priorities above (TIME 60 outranks EVS 61 and SB 64), the number of CPUs, and where tasks block. The issue's order: "TIME task will run it's TaskMain first even though it is 4th in the start sequence" [F: case C3.md:173].
- In this tree the thread that calls `CFE_ES_Main` is first raised to maximum priority, "so ES Main completes" (`/home/user/work/c5/src/641/cFE-6.4.1-OSS-release/psp/fsw/pc-linux/src/cfe_psp_start.c:352-366`). Whether the configured priorities are actually applied to the tasks is decided outside the cFE tree, by the abstraction-layer build and the privileges of the run; that source is not in this tree [F: case C3.md:487-488].

**7. Consequence of "u before w" in 6.4.1**
- The status is discarded (641:sb/cfe_sb_api.c:264); `CFE_SB_CreatePipe` returns `CFE_SUCCESS`; TIME's init continues; no code path exits an application.
- Reported harm: a crash [F: case C3.md:177]; its mechanism is the out-of-bounds index of field 5, row 1. Whether the local build faults is [U]; with 4-byte pointers (C5B:72) the index wraps to one element before the array [I].
- Framework-visible traces when it does not fault: one syslog line (641:evs/cfe_evs_utils.c:215), EVS housekeeping counter `UnregisteredAppCounter` +1 (:202), `AppData[id].EventCount` = 1 until that app registers (reset at 641:evs/cfe_evs.c:130), and in one sub-order an ERROR event ID 41.
- `EVS_NotRegistered` acts once per AppId (:199): TIME's later SB calls (`CFE_SB_Subscribe` 641:time/cfe_time_task.c:318 and following) only get 0xc2000002 back, silently, and do not repeat the out-of-bounds read [I].
- Healing: the DEBUG event is never re-sent. The pipe and the subscriptions are created in both orders. Steady state of a non-faulting "u before w" run versus "w before u": same pipes, routes and registrations; the differences are one counter, one syslog line and at most one ERROR event [I]. With the default mask 0xE the lost event would have been masked anyway [I].

**8. The fix (6.4.2)**
- F3, order enforced by synchronization: `AppState = CFE_ES_APP_STATE_INITIALIZING`, `++AppStartedCount` (642:es/cfe_es_start.c:848-:849); after each core task creation `CFE_ES_ApplicationSyncDelay(CFE_ES_SYSTEM_STATE_UNDEFINED, CFE_CORE_MAX_STARTUP_MSEC)` (:936), and on timeout a syslog line and `CFE_PSP_Panic` (:938-:949). The wait is a polling loop (:1007-:1040; period `CFE_ES_STARTUP_SYNC_POLL_MSEC` 50, bound `CFE_CORE_MAX_STARTUP_MSEC` 30000: 642cfg:1702, :1726). Each core TaskMain calls `CFE_ES_WaitForStartupSync` after its init (642:evs/cfe_evs_task.c:336, 642:sb/cfe_sb_task.c:143, 642:es/cfe_es_task.c:209, 642:time/cfe_time_task.c:209, 642:tbl/cfe_tbl_task.c:240), which sets RUNNING and `++AppReadyCount` (642:es/cfe_es_api.c:670-:673).
- F1, guard at the use site: `CFE_EVS_GlobalData.EVS_AppID < CFE_ES_MAX_APPLICATIONS &&` (642:evs/cfe_evs_utils.c:620); otherwise the event is dropped and `CFE_SUCCESS` returned (:649).
- F2, publish last: `EVS_GetAppID(&AppID)` into a local (642:evs/cfe_evs_task.c:394); the global is assigned at :436, after register :402, CreatePipe :410 and the subscriptions :419, :427.
- Unchanged: the SB ID's publication and its 36 consume sites: `sb/cfe_sb_api.c`, `sb/cfe_sb_init.c`, `evs/cfe_evs.c`, `es/cfe_es_objtab.c` are byte-identical in the two trees [M2].
- Kind of fix: for TIME, ES and TBL the dependence is **enforced by synchronization** (fail-stop on timeout; not a retry). For O2 it is **tolerated by a guard** and narrowed by publish-last. For O1 with consumer EVS it is **not removed**: EVS's `CFE_SB_CreatePipe` (642:evs/cfe_evs_task.c:410) now always precedes w1, because the SB task is not yet created; it reads 0 = EVS's own, already registered ID and gets `CFE_SUCCESS` [I].
- Predicted visible residue in 6.4.2 [I]: EVS's startup event (642:evs/cfe_evs_task.c:437) is sent on SB (642:evs/cfe_evs_utils.c:438), has no subscriber, and SB reports `CFE_SB_SEND_NO_SUBS_EID` (14, INFORMATION) with `CFE_SB.AppId` = 0 (641:sb/cfe_sb_api.c:1207-:1211, file identical in 6.4.2 [M2]): an SB event emitted under the name of app 0 (CFE_EVS). Not checked against the recorded 6.4.2 console [U].

**9. What a generic observer sees in 6.4.1, failing order**
- (a) API boundary, TIME task: `CFE_ES_RegisterApp` → 0; `CFE_EVS_Register(NULL,0,0)` → 0; `OS_BinSemCreate` ×2; `CFE_ES_CreateChildTask` ×2; `CFE_SB_CreatePipe` → 0, with nested `OS_QueueCreate`, `CFE_ES_GetAppID`, `CFE_ES_GetAppName`, then `CFE_EVS_SendEventWithAppID(EventID 5, type DEBUG, AppID 0)` → 0xc2000002 if it returns, with nested `CFE_ES_GetAppName(…, 0, …)` and `CFE_ES_WriteToSysLog`. Later: SB task `CFE_ES_GetAppID(out)` → 0 with out value 1, then `CFE_EVS_Register` → 0; EVS task `CFE_ES_GetAppID(out)` → 0 with out value 0, then `CFE_EVS_Register` → 0. The reads of O2 (`EVS_SendEvent`, `EVS_IsFiltered`) are internal functions, not API calls.
- The AppID argument 0 at u names app 0 (EVS), not SB: pairing by key links u to EVS's `CFE_EVS_Register`, not to w1 [I].
- (b) Events and syslog: one syslog line; no event in the crash sub-order; ERROR event ID 41 only in the sub-order "EVS up, SB between :198 and :257".
- (c) Memory: a load of `CFE_SB.AppId` in the TIME task (641:sb/cfe_sb_api.c:264) that no store precedes (zero-initialized static); a load of `AppData[0].RegisterFlag` (641:evs/cfe_evs.c:289) before EVS's store (:128); a load of `EVS_AppID` (641:evs/cfe_evs_utils.c:616) that reads ES main's placeholder store (641:evs/cfe_evs_task.c:190) before the EVS task's store (641:es/cfe_es_api.c:863 via 641:evs/cfe_evs_task.c:385); then a load outside the array bounds at 641:evs/cfe_evs_utils.c:246 and possibly a store at :313.
- Recorded runs: none targeted C3 [M9]. One natural 6.4.1 boot (all 12 tasks under a fixed-priority real-time policy, 4 CPUs) reached "ES Startup: CFE Core Startup Complete" [F C5B:107, :114, :117]: no crash in that boot. Whether any non-fatal sub-order occurred is not reported [U].
- 2a facts for `c5_641` [M5, M6]: rows B64 `cfe_sb_t.AppId -> CFE_EVS_SendEventWithAppID#2 sites=36` (rel(c5_641):2478-:2528, "handling: discarded 36"), B18 `CFE_EVS_GlobalData_t.EVS_AppID -> EVS_IsFiltered#0` (:1675-:1687), B19 (:1688); all three "feasibility: undecidable"; B18 and B19 "flip: none". B64 lists 3 writers: the real one (sb/cfe_sb_task.c:198) and two may-writes from string copies in `CFE_SB_CreatePipe` (:2516-:2518). **The five core tasks are not actors in these facts**: the spawn at es/cfe_es_start.c:875 is unresolved ("a read through another member of a union is not resolved in this revision", rel(c5_641):19), so the reader and writer actor sets of these rows do not contain TIME, EVS or SB. No C3 test exists among T1–T7 [F R/plan/PI_DECISIONS_PENDING_20261030.md:123].

**10. Ground truth (C3)**
| Version | Order | Observable outcome | Tag |
|---|---|---|---|
| 6.4.1 | u (TIME's CreatePipe event) before w1, w2, w3 | out-of-bounds access in `EVS_IsFiltered`; crash in the report; otherwise 0xc2000002 discarded and one syslog line | code path [F]; crash [F: case C3.md:177]; local build [U] |
| 6.4.1 | u after w2, before EVS :393 and w1 | 0xc2000002 discarded; syslog "App CFE_EVS not registered…"; no event | [I] |
| 6.4.1 | u after EVS :393, before w1 | `CFE_SUCCESS`; event judged as app 0's; masked | [I] |
| 6.4.1 | u after w1, before SB :257 | 0xc2000002; ERROR event 41 and syslog "App CFE_SB not registered…" if EVS is up | [I] |
| 6.4.1 | w1, w2, w3 before u | `CFE_SUCCESS`, event masked; nominal | [I]; one natural boot completed startup [F C5B:114] |
| 6.4.2 | TIME's u before w1 | not reachable: TIME is created only after EVS, SB, ES reported ready; a timeout panics | [I from 642:es/cfe_es_start.c:936-:949]; core apps created in order [F C5B:112] |
| 6.4.2 | EVS's u before w1 (every boot) | `CFE_SUCCESS` (0 is EVS's own ID); misattributed INFORMATION event 14 predicted | [I]; console [U] |
| 6.4.2 | an EVS-internal event before :436 | dropped by the guard, `CFE_SUCCESS` | [F 642:evs/cfe_evs_utils.c:620, :649] |

**11. Look-alikes near the case (6.4.1)**
- L1 `CFE_EVS_Register` by TIME, ES, TBL before the EVS task has run (641:time/cfe_time_task.c:252, 641:es/cfe_es_task.c:326, 641:tbl/cfe_tbl_task.c:290): by design; the data is published by `CFE_EVS_EarlyInit` (641:evs/cfe_evs_task.c:188) before any task. Never "u before w".
- L2 `CFE_SB_CreatePipe` as such before SB's task init (641:evs/cfe_evs_task.c:401, 641:es/cfe_es_task.c:357, 641:time/cfe_time_task.c:308, 641:tbl/cfe_tbl_task.c:301): mutex, pipe and routing tables come from `CFE_SB_EarlyInit` ("MUST be called before any SB API's are called", 641:sb/cfe_sb_init.c:116); it returns `CFE_SUCCESS` in both orders.
- L3 A new task's `CFE_ES_RegisterApp` / `CFE_ES_GetAppID` against the creator's TaskTable fill: lock-enforced (641:es/cfe_es_start.c:838-:933).
- L4 `EVS_NotRegistered` itself: the designed answer to "event before registration", throttled to once per app (641:evs/cfe_evs_utils.c:199). The same status, syslog text and event arise when an application simply sends before registering: same vocabulary, not a cross-actor order.
- L5 Re-registration cleanup in `CFE_EVS_Register` (641:evs/cfe_evs.c:110-:113): a check followed by a reset.
- L6 SB's own `CFE_SB_CreatePipe` (641:sb/cfe_sb_task.c:293) after its own :198 and :257: program order.
- L7 Same shape, not in the issue: `CFE_ES_GetAppID(&CFE_TBL_TaskData.TableTaskAppId)` (641:tbl/cfe_tbl_task.c:360), passed to `CFE_EVS_SendEventWithAppID` from the TBL API in the caller's task (641:tbl/cfe_tbl_api.c:559, :662, :731; 22 calls in two files [M11]). In 6.4.1 it is as unsynchronized against early callers as O1 [I]; no report exists.
- L8 Further instances of the SAME pair that the issue does not name: the SB API calls in the inits of EVS, ES, TBL and of script applications before SB's :198 / :257. One root cause gives many (caller, site) instances.

**12. What a person wrote in the old path**: nothing ran for C3. There is no C3 profile, observation plan, hold plan or expectation table (`R/kairos/profiles` holds c1-bp-b051eb0, c5-cfe-6.4.x, cfs-v7.0.1; `R/plan/input_audit_20261030.md` has no C3 row) [M9]. The case file lists the choices a person would have had to make: which artifacts (C3.md:80), how to force the order, by priorities or by holding `CFE_SB_AppInit` and `CFE_EVS_TaskInit` until TIME's event call (C3.md:94), and which outcome counts, the out-of-range index or a crash (C3.md:103). A new method must itself produce the three objects and their publish sites, the consumer set, a schedule in which u comes first, and an oracle that covers a memory-safety failure with no framework status.

**13. Classification**: race-like (no enforcement, both orders feasible, decided by scheduling). Inside the framework: all actors are core service tasks with ES application records of type CORE; no startup-script application is needed. Two-level: an unsynchronized placeholder-then-publish on O1 routes the caller into an unchecked placeholder on O2. One sub-order is fatal; the others lose or misattribute one event and leave the same functional steady state.

**14. Unknowns**: C3 was never executed here; fault or silent out-of-bounds access in the local build; natural order distribution; whether the recorded 6.4.x consoles hold the predicted lines; the reporter's software versions and failure frequency [F: case C3.md:687]; the fix commits (not public); whether 6.4.0 has the same code [F: case C3.md:694].

---

# CARD R2 — nasa/cFE #2663 "Unregistered AppId error with CFE_EVS_SendEventWithAppID() during cFE task startup"

**1. Source and versions**
- Issue #2663, open; no merged fix; the only pull request was closed unmerged by its author as "vacuous" [F: case R2.md:540, :53]. The report is a debugger breakpoint on the `CFE_EVS_APP_ILLEGAL_APP_ID` assignment that "stopped"; no hit count, thread or backtrace [F: case R2.md:88, :170].
- Reported revision: "cFE v6.7" on `main`, mapped by the case file to fc8c48ad, as an inference [F: case R2.md:10]. **Not on disk.**
- Verified here: cFE c5fb2b4 (v7.0.1) in `/home/user/work/procrace/cfs_ref/cFS/cfe`, whose `modules` tree equals `/home/user/work/expect_src/cFE_c5fb2b4/modules` [M7]. It is the 2a build `r2_core` (rel(r2_core) header; `…/r2_core/NP/args.txt`).
- The same pattern is in the cFE of both C1 builds (6cccb3f, fef4a0d): publish at `modules/sb/fsw/src/cfe_sb_task.c:124`, error assignment at `modules/evs/fsw/src/cfe_evs.c:184`, static table order EVS, SB, ES, TIME, TBL (`modules/es/fsw/src/cfe_es_objtab.c:100-125`, read in `/home/user/work/expect_src/cFE_fef4a0d`), and as row B73 in rel(c1_pre_tx):3154 and rel(c1_post_tx):3154 [M8].
- Fixed version: none.

**2. Actors**
- ES main thread: `CFE_ES_Main` → `CFE_ES_CreateObjects` (701:es/fsw/src/cfe_es_start.c:188, :741). Phase 1 calls every module's `EarlyInit` (:751-:773). Phase 2 creates one core task at a time: `CFE_ES_StartAppTask` (:810), then `CFE_ES_MainTaskSyncDelay(CFE_ES_AppState_RUNNING, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)` (:835), panic on failure (:845).
- Order: the generated `CFE_CORE_MODULE_OBJTAB` (`/home/user/work/procrace/cfs_ref/cFS/build-native_std/native/default_cpu1/cpu1/cfe_static_module_list.c:36-48`), produced from `MISSION_CORE_MODULES` (`…/cFS/cfe/cmake/mission_defaults.cmake:27-40`) by `…/cFS/cfe/cmake/target/CMakeLists.txt:55-71`. Entries with a `TaskMain`: ES, EVS, SB, TBL, TIME, in that order; the others have `.TaskMain = NULL` (701:config/fsw/src/cfe_config_objtab.c:35, 701:fs/fsw/src/cfe_fs_objtab.c:34, 701:msg/fsw/src/cfe_msg_objtab.c:34, 701:sbr/fsw/src/cfe_sbr_objtab.c:34, 701:resourceid/fsw/src/cfe_resourceid_objtab.c:34).
- Consumers: the CFE_ES task (`CFE_ES_TaskInit`, 701:es/fsw/src/cfe_es_task.c:306) and the CFE_EVS task (`CFE_EVS_TaskInit`, 701:evs/fsw/src/cfe_evs_task.c:266). Publisher: the CFE_SB task (`CFE_SB_TaskMain`, 701:sb/fsw/src/cfe_sb_task.c:70).
- Startup-script applications start only after the core is ready: `SystemState = CFE_ES_SystemState_CORE_READY` (701:es/fsw/src/cfe_es_start.c:194), then `CFE_ES_StartApplications` (:201).

**3. Shared object**: `CFE_SB_Global.AppId` (`CFE_ES_AppId_t`; object at 701:sb/fsw/src/cfe_sb_task.c:40). It holds SB's ES application ID, a resource ID computed at run time (0x110003 in the recorded runs [F W6:442]). Placeholder: 0 = `CFE_ES_APPID_UNDEFINED` (701:core_api/fsw/inc/cfe_es_api_typedefs.h:220; 701:core_api/fsw/inc/cfe_resourceid_api_typedefs.h:65), written by ES main through `memset(&CFE_SB_Global, 0, sizeof(CFE_SB_Global))` in `CFE_SB_EarlyInit` (701:sb/fsw/src/cfe_sb_init.c:50). Unlike 6.4.1, **0 is not a valid ID**: `CFE_ResourceId_ToIndex` rejects it (701:resourceid/fsw/src/cfe_resourceid_api.c:132-:137; base `CFE_ES_APPID_BASE`, 701:resourceid/fsw/inc/cfe_core_resourceid_basevalues.h:91). Second readiness step: SB's EVS registration (701:sb/fsw/src/cfe_sb_task.c:155).

**4. Publish operation w**: `CFE_ES_GetAppID(&CFE_SB_Global.AppId);` at 701:sb/fsw/src/cfe_sb_task.c:138, the first statement of `CFE_SB_AppInit` (:130), called from `CFE_SB_TaskMain` (:77). The return value is ignored. The store executes at 701:es/fsw/src/cfe_es_api.c:754 (or :759, which stores `CFE_ES_APPID_UNDEFINED` and returns `CFE_ES_ERR_RESOURCEID_NOT_VALID` when the caller's app record is not matched), under the ES shared-data lock (:748-:763). Actor: SB task. Phase: Phase 2, third task. SB becomes RUNNING at `CFE_ES_WaitForSystemState(CFE_ES_SystemState_CORE_READY, …)` (701:sb/fsw/src/cfe_sb_task.c:93).

**5. Consume operation u**
- 39 sites pass `CFE_SB_Global.AppId` as argument 2 of `CFE_EVS_SendEventWithAppID`: 34 in `cfe_sb_api.c`, 1 in `cfe_sb_priv.c`, 4 in `cfe_sb_task.c`; all 39 results are discarded [M8: rel(r2_core):2147-:2204, "handling: discarded 39, used 0, tested 0" at :2198; M10]. They run in the caller's task, after the SB lock is released.
- Sites on the init paths of ES and EVS: the `CFE_SB_CreatePipe` success event `CFE_SB_PIPE_ADDED_EID`, DEBUG (701:sb/fsw/src/cfe_sb_api.c:275-:281); the `CFE_SB_SubscribeFull` success event `CFE_SB_SUBSCRIPTION_RCVD_EID`, DEBUG (:1183-:1189); the transmit path event `CFE_SB_SEND_NO_SUBS_EID`, INFORMATION (set at 701:sb/fsw/src/cfe_sb_priv.c:1086, typed at :655-:657, sent at :786). The last one is reached by every event that ES or EVS emits, because EVS transmits each event on SB (701:evs/fsw/src/cfe_evs_utils.c:498, :513) and nobody has subscribed yet. Event IDs 5, 10, 14 (701:sb/fsw/inc/cfe_sb_eventids.h:86, :144, :188).
- Callers: ES `CFE_SB_CreatePipe` 701:es/fsw/src/cfe_es_task.c:367, `CFE_SB_Subscribe` :377, :387, own events :442, :449, :466, :467; EVS `CFE_SB_CreatePipe` 701:evs/fsw/src/cfe_evs_task.c:289, `CFE_SB_Subscribe` :297, :304, startup event :319.
- Yield while unpublished: in `CFE_EVS_SendEventWithAppID` (701:evs/fsw/src/cfe_evs.c:169) `EVS_GetAppDataByID(AppID)` (:185) returns NULL (701:evs/fsw/src/cfe_evs_utils.c:48-:63), so `Status = CFE_EVS_APP_ILLEGAL_APP_ID` (:188), returned at :213. Value 0xc2000003 (701:core_api/fsw/inc/cfe_error.h:267); severity bits = `CFE_SEVERITY_ERROR` 0xc0000000 (:119). That branch does nothing else: no event, no syslog, no counter (:186-:189).
- A valid but unregistered ID (window SB :138 to :155) would instead take `EVS_NotRegistered` (:193) and yield `CFE_EVS_APP_NOT_REGISTERED` 0xc2000002 (cfe_error.h:258) [I]; ES and EVS init cannot fall into that window under the serialization of field 6.
- Framework documentation of the consumed API (701:core_api/fsw/inc/cfe_evs.h:169-:172): "The Application ID must correspond to a registered application or core service. For messages outside the context of a registered application (for example early in app initialization or if registration fails) #CFE_ES_WriteToSysLog can be used for reporting."; return listed at :199.

**6. Order enforcement**
- Serialized core startup: task k+1 is created only when every used application record is at least RUNNING. Polling loop 701:es/fsw/src/cfe_es_start.c:860-:915; period `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` 50 (701:es/fsw/inc/cfe_es_internal_cfg.h:860), bound `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` 30000 (701:core_private/config/default_cfe_core_private_internal_cfg.h:68); on timeout, panic (:845). A core application becomes RUNNING inside `CFE_ES_WaitForSystemState(CORE_READY)` (701:es/fsw/src/cfe_es_api.c:545-:574), which it calls after its init (ES 701:es/fsw/src/cfe_es_task.c:134, EVS 701:evs/fsw/src/cfe_evs_task.c:229, SB 701:sb/fsw/src/cfe_sb_task.c:93).
- Therefore the whole init of ES and of EVS precedes the creation of the SB task: "u before w" **always** for those two callers, and "w before u" **always** for TBL, TIME, their children and every script application [I from the lines above; confirmed by the recorded runs, field 9]. Nothing on this path depends on scheduling; the default priorities (ES 68, EVS 61, SB 64, TBL 70, TIME 60: 701:es/fsw/inc/cfe_es_internal_cfg.h:44, 701:evs/fsw/inc/cfe_evs_internal_cfg.h:44, 701:sb/fsw/inc/cfe_sb_internal_cfg.h:340, 701:tbl/fsw/inc/cfe_tbl_internal_cfg.h:51, 701:time/fsw/inc/cfe_time_internal_cfg.h:234) play no part.
- The order is a build-time list: "the order of this list is the order in which they will be initialized" (`…/cFS/cfe/cmake/mission_defaults.cmake:26`).
- No lock relates the store (ES lock, 701:es/fsw/src/cfe_es_api.c:748-:763) to the loads.
- One reader actor is outside the serialization: `CFE_ES_BackgroundTask` is listed among the readers of the field (rel(r2_core):2191) [M8]; whether it can emit an event before SB's init is [U].

**7. Consequence of "u before w"**
- The status is ignored at every site; the SB API calls return `CFE_SUCCESS` to ES and EVS; both reach RUNNING; no syslog line, no application exit, no panic.
- Lost: the events. The DEBUG events would be masked by default anyway (`DEFAULT_CFE_PLATFORM_EVS_DEFAULT_TYPE_FLAG` 0xE, 701:evs/fsw/inc/cfe_evs_internal_cfg.h:177). The INFORMATION event "No subscribers for MsgId 0x%x,sender %s" (701:sb/fsw/src/cfe_sb_priv.c:661) would not be: SB's default filter for it is `CFE_EVS_FIRST_4_STOP` (701:sb/fsw/inc/cfe_sb_internal_cfg.h:222, :225). ERROR events of the same SB calls would be lost in the same way [I].
- Healing: none. No retry and no re-send. Later executions of the same sites by later-created tasks succeed.
- Steady state against a hypothetical "w before u": pipes, subscriptions and registrations are the same; the differences are the missing early events and SB's filter count for event 14 [I]. `NoSubscribersCounter` is incremented in both (701:sb/fsw/src/cfe_sb_priv.c:1085).

**8. The fix**: none merged [F: case R2.md:540]. Two ideas appear in the issue discussion, undecided: resolve SB's ID lazily at first use, or record it earlier [F: case R2.md:545]. The descendants of C3's fix are present and shape the case: the serialization forces the order, and the validity check turns the early use into an error code instead of a wrong index.

**9. What a generic observer sees**
- (a) API boundary, CFE_ES task: `CFE_EVS_Register` → 0; `CFE_SB_CreatePipe` → 0 with nested `CFE_EVS_SendEventWithAppID(EventID 5, DEBUG, AppID 0)` → 0xc2000003; `CFE_SB_Subscribe` ×2 → 0, each with nested `(EventID 10, DEBUG, AppID 0)` → 0xc2000003; each `CFE_EVS_SendEvent` → 0 with nested `CFE_SB_TransmitMsg` → 0 and nested `CFE_EVS_SendEventWithAppID(EventID 14, INFORMATION, AppID 0)` → 0xc2000003. The same in the CFE_EVS task. Then the SB task: `CFE_ES_GetAppID(out)` → 0 with out value 0x110003; `CFE_EVS_Register` → 0. From then on the same sites pass AppID 0x110003 and get 0.
- Every application-facing return is success. The failing call is made by one framework module (SB) to another (EVS) on behalf of the caller.
- AppID 0 at u does not name SB. Linking u to w needs (i) the static fact that argument 2 at these sites is loaded from `CFE_SB_Global.AppId`, whose only non-placeholder writer is in `CFE_SB_AppInit` (row B65 states it: rel(r2_core):2187-:2190), or (ii) the address of w's out-pointer, or (iii) the later calls of the same sites.
- (b) Events and syslog: nothing. The evidence is the absence of events.
- (c) Memory: loads of `CFE_SB_Global.AppId` (701:sb/fsw/src/cfe_sb_api.c:277, :1185; 701:sb/fsw/src/cfe_sb_priv.c:786) in the ES and EVS tasks read the placeholder that ES main stored with a whole-structure fill (701:sb/fsw/src/cfe_sb_init.c:50), before the SB task's store (701:es/fsw/src/cfe_es_api.c:754, through the pointer passed at 701:sb/fsw/src/cfe_sb_task.c:138).
- Recorded runs (W6, v7.0.1, two boots, recordings F0 and G-I): 24 calls with AppID 0, all returning 0xc2000003: CFE_ES 20 (event IDs 5 / 10 / 14 = 1 / 2 / 17) and CFE_EVS 4 (1 / 2 / 1); all before OPERATIONAL; identical in both recordings and in all readings [F W6:22, :394, :434, :462, :463]. SB's own 5 sends and 266 / 247 uses by other tasks showed no violation [F W6:395, :396]. No occurrence after the commanded restart [F W6:429]. The 24 are occurrences of one path, not 24 defects [F R/plan/results_pilot_20261025.md:70].
- 2a facts: row B65 is graded "feasibility: undecidable: fixed unconfirmed (creation order + await …)", with the writer at table iteration 4 and "reader actors created before it: CFE_ES_TaskMain (iteration 1), CFE_EVS_TaskMain (iteration 2)" (rel(r2_core):2201); "violation: unconfirmed" (:2202); "flip: static-undetermined" (:2203) [M8]. It is 1 of 338 rows of that build [F AR:313].

**10. Ground truth (R2)**
| Build | Order | Observable outcome | Tag |
|---|---|---|---|
| v7.0.1 (`r2_core`) | u before w: ES and EVS init, every boot | 0xc2000003 on 24 calls, discarded; events lost; no syslog, no exit; ES and EVS RUNNING | recorded [F W6:394, :462, :463]; code [F] |
| v7.0.1 | w before u: TBL, TIME, applications, every boot | `CFE_SUCCESS` or filtered; 266 / 247 occurrences, no violation | [F W6:396] |
| v7.0.1 | w before u for ES and EVS | not realizable in the shipped configuration; only a reordered module list, a constructed configuration, would give it | [I]; [F: case R2.md:631] |
| C1-era cFE (6cccb3f, fef4a0d) | u before w for EVS only | predicted from the table order and the same serialization (`modules/es/fsw/src/cfe_es_start.c:811` in those trees); not measured | [I]; [U] |
| reported revision | per the case file: code predicts EVS only, the report says "the ES and the EVS tasks" | source not on disk | [F: case R2.md:48, :160] |
| fixed | does not exist | — | [F: case R2.md:540] |

**11. Look-alikes near the case (v7.0.1)**
- L1 The same 39 sites executed after the publication, by TBL, TIME and applications: success [F W6:396].
- L2 Same shape, enforced order: `CFE_ES_GetAppID(&CFE_TBL_Global.TableTaskAppId)` (701:tbl/fsw/src/cfe_tbl_task.c:196), passed to `CFE_EVS_SendEventWithAppID` at 19 sites of the TBL API (row B97, rel(r2_core):2652; "reader actors created before it: none", :2686). 2a gives B97 the same grade as B65; only that phrase differs [M8]. Caveat: the `r2_core` facts hold no startup-script application actor ("script-entry 0", rel(r2_core) `actors:` header line), so "none" does not cover the applications that call the TBL API; the recorded runs do (28 uses with TBL's ID, no violation [F W6:396]).
- L3 EVS's own ID: `EVS_SendEvent` checks it before use and drops the event while it is unpublished (701:evs/fsw/src/cfe_evs_utils.c:617-:623); the ID is published last (701:evs/fsw/src/cfe_evs_task.c:312). A designed tolerance that returns `CFE_SUCCESS`.
- L4 `CFE_EVS_Register` by ES before the EVS task exists (701:es/fsw/src/cfe_es_task.c:336): backed by Phase 1 `CFE_EVS_EarlyInit` (701:evs/fsw/src/cfe_evs_task.c:54, :63).
- L5 A bounded retry: `CFE_ES_GetTaskFunction` (701:es/fsw/src/cfe_es_apps.c:559-:605) polls every `CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` up to `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` until the creator has published the new task's record.
- L6 `CFE_SB_SEND_NO_SUBS_EID` from applications after startup (a transmit before any subscriber exists): the count differs between the two recordings (12 and 17) and is attributed to application start order [F W6:414, :448]. An order-dependent INFORMATION event about another kind of object (routes), not this case.
- Nearby, ground truth unknown: `CFE_SB_AppInit` ignores the status of `CFE_ES_GetAppID`. The creator publishes the task record (701:es/fsw/src/cfe_es_apps.c:696-:722) before it finalizes the application record (`CFE_ES_AppRecordSetUsed(AppRecPtr, PendingAppId)`, 701:es/fsw/src/cfe_es_start.c:815-:818). If the SB task's first successful poll (701:es/fsw/src/cfe_es_apps.c:575-:587) and its `CFE_ES_GetAppID` fall between the two, the context lookup returns NULL (701:es/fsw/src/cfe_es_resource.c:330-:333) and SB keeps an undefined ID for the whole run [I; no reference reports it, F: case R2.md:110, :117]. It did not occur in the recorded runs [F W6:310]. A schedule that delays the creator there for longer than the 50 ms poll delay would realize it.

**12. What a person wrote in the old path** (`R/kairos/profiles/cfs-v7.0.1`): the rule `rspec K-ID.send-with-appid` with `use op CFE_EVS_SendEventWithAppID`, `key evs.reg(arg.AppID)`, `requires valid(arg.AppID) && state(evs.reg(arg.AppID)) = REG`, oracle functions, a caller-side requirement text, `allowed none`, `harm event_request_rejected`, written with "cases inspected for this API: R2" (`40_rspec.kzp:18-28`); the resource kind `evs.reg` and the location `CFE_SB_Global.AppId` (input audit rows KND-cfs-evsreg and LOC-sb, `R/plan/input_audit_20261030.md:234, :237`); the happens-before rule CORE-SERIAL that names the poll function and its panic exit (`30_hb.kzp:51`); two declared-trusted assumptions (`50_trusted.kzp:9, :16`); 18 hand-placed observation points and the interval boundaries (rows OBS-cfs-points, OBS-cfs-interval, `input_audit_20261030.md:250, :251`). A new method must derive by itself: which call is the consume and which argument is the key; that 0 means absent; who publishes; the serialization order; and that an ignored error status with no later re-send is the harm.

**13. Classification**: fixed-order premature use (u always before w for ES and EVS), deterministic by the construction of startup, not a race. Inside the framework: core services only, no startup-script application. One-shot loss, never healed, no crash, no functional steady-state difference. It is not "a use that fails because another actor has not published": the consumer's own operation (pipe creation, subscription, transmit) succeeds, and what fails is a side effect (the event report) owned by a third module.

**14. Unknowns**: the reporter's revision and whether ES was involved there [F: case R2.md:48]; behaviour at the reported revision and in the C1 builds (not measured); the creator/child window above; whether `CFE_ES_BackgroundTask` can reach a consume site before SB's init.

---

# C3 and R2: do they share an independent cause?

**Answer: they share one dependency and differ in everything that makes it a failure.** Evidence:
| Aspect | C3 (6.4.1) | R2 (v7.0.1) | Same? |
|---|---|---|---|
| Object | `CFE_SB.AppId` (641:sb/cfe_sb_task.c:105) | `CFE_SB_Global.AppId` (701:sb/fsw/src/cfe_sb_task.c:40) | yes |
| Publish site | `CFE_ES_GetAppID(&…)` in `CFE_SB_AppInit`, 641:sb/cfe_sb_task.c:198 | same call, 701:sb/fsw/src/cfe_sb_task.c:138 | yes |
| Consume sites | 36 calls of `CFE_EVS_SendEventWithAppID` in the caller's task [M3] | 39 such calls [M8] | yes |
| Synchronization of the field | none | none | yes |
| What decides the order | scheduling among unserialized core tasks | creation order plus serialized startup | no |
| Meaning of the placeholder 0 | a valid ID (EVS's): misattribution or `CFE_EVS_APP_NOT_REGISTERED` | undefined ID: `CFE_EVS_APP_ILLEGAL_APP_ID` | no |
| What makes it harmful | a second unpublished object, `EVS_AppID` = 0xFFFFFFFF, indexed unchecked | the discarded error status alone | no |
| Fix | 6.4.2: ordering barrier, guard and publish-last for `EVS_AppID`; the SB field untouched [M2] | none | — |
- The link between the two is in the 6.4.2 source itself: with the barrier (642:es/cfe_es_start.c:936) and the unchanged table order (EVS at 641:es/cfe_es_objtab.c:120 before SB at :122, file identical in 6.4.2 [M2]), EVS's `CFE_SB_CreatePipe` (642:evs/cfe_evs_task.c:410) runs before the SB task exists. C3's fix turned "sometimes, by schedule" into "always, by construction" for the tasks created before SB [I]. That state is R2's shape, with a different status because 0 became an invalid ID later.
- For counting: the two cases are **not independent** as (object, publish site, consume sites) and **independent** as (ordering cause, second object, observable signal). The project's pending decision proposes to count two cases and mark the shared cause [F R/plan/PI_DECISIONS_PENDING_20261030.md:71].
- A method that finds R2 through "error-class status at a consume, same site succeeds after another actor's publication" does not thereby find C3: in 6.4.1 the corresponding consume yields a different status or none, and in 6.4.2 it yields `CFE_SUCCESS`.

# Observations a designer should know (facts, not design)
1. Neither case involves a startup-script application. All actors come from the core object table (641:es/cfe_es_objtab.c:120-:129; generated `CFE_CORE_MODULE_OBJTAB` at v7.0.1).
2. In both cases the status that carries the signal is returned to a framework module and dropped there; no application-facing API returns an error.
3. The key seen at the consume is the placeholder, not the publisher's identity. In 6.4.1 it is even another actor's valid key.
4. An "absent" status for the SB ID exists only where 0 is an invalid ID (v7.0.1). In 6.4.1 the outcomes of the same early read range from `CFE_SUCCESS` to an out-of-bounds access.
5. C3's reported harm has no framework vocabulary: no status, event or syslog line is specific to it; the sub-order that only logs "not registered" produces the same line.
6. For R2 the opposite order cannot be produced by delaying a task: SB is not created until ES and EVS are RUNNING, and delaying a core task makes the creator's timed poll run toward its 30000 ms panic bound (701:es/fsw/src/cfe_es_start.c:835-:845). The same holds for 6.4.2 (642:es/cfe_es_start.c:936-:949).
7. For C3 in 6.4.1 the first framework API calls of each core task are `CFE_ES_PerfLogEntry` (641:sb/cfe_sb_task.c:125, 641:evs/cfe_evs_task.c:318, 641:time/cfe_time_task.c:191) and then `CFE_ES_RegisterApp` (641:sb/cfe_sb_task.c:190, 641:evs/cfe_evs_task.c:377, 641:time/cfe_time_task.c:245); both precede w1 and w2. `CFE_SB_AppInit` and `CFE_EVS_TaskInit` are internal functions, not API calls.
8. One root cause gives many occurrences (24 calls in one boot) and many sites (36 or 39); the counting unit must be chosen.
9. The enforcing waits are timed polls with a fail-stop exit; their constants are in the platform configuration (50 ms period, 30000 ms bound) at both versions.
10. R2 has no fixed version: a control must come from the later callers of the same sites or from the TBL analogue (B97), not from a version pair. C3 has a version pair, but its diff also carries C5's fix.
11. 2a facts: in `c5_641` the core tasks are not actors; in `r2_core` the startup-script applications are not actors, and the true row B65 and the look-alike B97 carry the same grades.

# §M — commands behind the [M] tags (read-only)
- M1: `diff -rq <641 or 642 tree>/cfe/fsw/cfe-core/src /home/user/work/c5tools/build/<641|642>/keep/<same release dir>/cfe/fsw/cfe-core/src` → no output for both versions; `ls <641 tree>/osal` → `readme.txt`.
- M2: `diff -rq 641: 642:` → 13 files differ: es/{cfe_es_api.c, cfe_es_apps.c, cfe_es_apps.h, cfe_es_global.h, cfe_es_start.c, cfe_es_task.c}, evs/{cfe_evs_task.c, cfe_evs_utils.c}, inc/{cfe_error.h, cfe_version.h}, sb/cfe_sb_task.c, tbl/cfe_tbl_task.c, time/cfe_time_task.c; `cmp -s` reports identical: sb/cfe_sb_api.c, sb/cfe_sb_init.c, sb/cfe_sb_priv.h, evs/cfe_evs.c, evs/cfe_evs_task.h, es/cfe_es_objtab.c.
- M3: `grep -c 'CFE_SB\.AppId' 641:sb/*.c` → cfe_sb_api.c 36, cfe_sb_task.c 1; `grep -n '&CFE_SB\.AppId\|CFE_SB\.AppId *=[^=]' 641:sb/*.c` → only cfe_sb_task.c:198.
- M4: `grep -n 'CFE_ES_WaitForStartupSync\|CFE_ES_RunLoop'` over the five 641 core task files → one comment line, es/cfe_es_task.c:210.
- M5: `grep -n '^actors:\|^spawn calls' rel(c5_641)` → lines 17-19 (roots 9: process 1, spawn-entry 3, table-walk-entry 0, script-entry 5).
- M6: `grep -n '^  [ABC][0-9]* .*\(AppId\|EVS_AppID\)' rel(c5_641)` → B18 :1675, B19 :1688, B64 :2478; rows read with `awk`.
- M7: `diff -rq /home/user/work/procrace/cfs_ref/cFS/cfe/modules /home/user/work/expect_src/cFE_c5fb2b4/modules` → no output.
- M8: `grep -n 'CFE_SB_Global_t.AppId\|TableTaskAppId -> CFE_EVS_SendEventWithAppID'` on rel(r2_core), rel(c1_pre_tx), rel(c1_post_tx) → B65 :2147, B97 :2652, B73 :3154 (both C1 builds); rows read with `awk`.
- M9: `grep -c 'C3\b'` on `R/plan/results_c5_boot_20261029.md`, `results_w6_20261029.md`, `results_w4w5_20261028.md`, `step2a_results_20261030.md`, `input_audit_20261030.md`, `REDESIGN_REVIEW_ANALYSIS_20261030.md` → 0 each; `ls R/kairos/profiles` → three profiles, none for C3.
- M10: `grep -c 'CFE_SB_Global\.AppId' 701:sb/fsw/src/*.c` → cfe_sb_api.c 34, cfe_sb_priv.c 1, cfe_sb_task.c 5 (one of them the write at :138).
- M11: `grep -c 'CFE_EVS_SendEventWithAppID' 641:tbl/*.c` → cfe_tbl_api.c 16, cfe_tbl_internal.c 6.

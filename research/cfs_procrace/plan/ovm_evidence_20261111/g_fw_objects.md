# g_fw_objects — cFS framework model, part B (objects, API roles, choke points, channels, identity, observability)

SUMMARY (10 lines)
1. v7.0.1 OSAL has ONE object-table layer: every named create passes `OS_ObjectIdAllocateNew`/`OS_ObjectIdFinalizeNew`, every by-name lookup passes `OS_ObjectIdFindByName`, every by-ID use passes `OS_ObjectIdGetById`. A hook there sees (class, name, ID, status).
2. v7.0.1 cFE has a choke point only for ID allocation (`CFE_ResourceId_FindNext`) and ID-to-slot (`CFE_ResourceId_ToIndex`); by-name lookups are one hand-written loop per class (ES app/lib/counter/CDS, TBL registry). SB pipe and ES task by-name defer to OSAL.
3. cFE 6.4.x + OSAL 4.1.1 have NO generic layer at all: one table and one loop per class per API; IDs are array indexes.
4. "Absent" is a distinct status for OSAL (-17), ES (0xc4000002), TBL (0xcc000002), but NOT for SB pipes (generic `CFE_SB_BAD_ARGUMENT`) and NOT for SB send-without-subscriber (returns `CFE_SUCCESS`).
5. SB send with no subscriber: `CFE_SUCCESS`, counter++, INFORMATION event 14 (default filter first-4-stop), buffer released: nothing is retained for a later subscriber.
6. EVS send has two different failures: caller AppID unresolvable = `CFE_EVS_APP_ILLEGAL_APP_ID`, fully silent; AppID valid but not registered = `CFE_EVS_APP_NOT_REGISTERED` + one ERROR event + syslog + counter.
7. Identity: `CFE_ES_GetAppID` = OS task ID -> ES task record -> app record; all three links are written by the PARENT after the child thread exists. v7.0.1 gates the child's entry on the task record (poll), but the app record is finalized later; 6.4.1 has no gate; 6.4.2 holds the ES lock across creation.
8. Role derivation (publisher/consumer/key/absent status) is mechanical for OSAL and for ES/TBL by-name APIs; the step 2a facts already contain the needed return/when/argument rows, but no class/role table exists (P0 holds only 72 generic primitives).
9. The framework's own dumps carry stable names next to run-varying IDs; numeric IDs are per-class creation serials, so an ID difference between two runs is a creation-order difference.
10. Census: 11 by-name lookup sites in the v7.0.1 apps, all status-checked; 10 of 11 keys are run-time values (table fields/parameters), 1 literal; failing branches: 1 exit, 1 bounded retry, 4 error event, 1 fallback lookup, 3 count/sentinel, 1 error to caller, 0 create-on-miss (BP at the C1 revision is the only create-on-miss).

Tags: [M] measured in this run, [F] read at path:line, [I] inference, [U] unknown. Path roots:
- V7 = /home/user/work/procrace/cfs_ref/cFS ; OS = V7/osal/src/os/shared/src ; OSH = V7/osal/src/os/shared/inc ; OSI = V7/osal/src/os/inc
- ES/SB/EVS/TBL/TIME = V7/cfe/modules/{es,sb,evs,tbl,time}/fsw/src ; ERR = V7/cfe/modules/core_api/fsw/inc/cfe_error.h ; RID = V7/cfe/modules/resourceid/fsw/src/cfe_resourceid_api.c
- G641/G642 = /home/user/work/c5/src/{641/cFE-6.4.1,642/cFE-6.4.2}-OSS-release/cfe/fsw/cfe-core/src ; O411 = /home/user/work/c5/src/osal/osal-4.1.1-release/src/os/posix
- FX = /home/user/work/redesign/step2a/tests_8c_fix/c1_pre_tx/NP/facts.txt (line numbers of that file)

## 0. Corrections to the task text and project documents
- [M] `ls` of G641/../../../osal: the `osal` directory inside both 6.4.x release trees holds only `readme.txt`. The OSAL of that generation is at /home/user/work/c5/src/osal/osal-4.1.1-release (version named in that readme). Command: `ls -la .../cFE-6.4.1-OSS-release/osal`.
- [M] V7/apps holds 17 applications (cf ci_lab cs ds fm hk hs lc md mm sample_app sbn sbn_f_remap sbn_udp sc sch_lab to_lab), not four. Command: `ls V7/apps`. The census (section g) covers all of them.
- [M] The two C1 bundles are not v7.0.1: `OS_ObjectIdFindByName` is at osapi-idmap.c:972-983 in the c1_pre_tx build (FX:77837-77841) and at :975-999 in V7. The same choke-point functions exist in both.
- [F] R/cases/mapping_v701.md:31,39 (ILLEGAL_APP_ID at cfe_evs.c L188, no counter, nothing printed) and :67 (CF retry loop) agree with the source I opened (EVS/cfe_evs.c:186-189; V7/apps/cf/fsw/src/cf_cfdp.c:1270-1303). No contradiction found in the parts I checked.

## a. Object classes (v7.0.1)
Absent-status definitions: OSAL `OS_ERR_NAME_NOT_FOUND` (-17) OSI/osapi-error.h:113; `OS_ERR_INVALID_ID` (-16) :112; `OS_ERR_NAME_TAKEN` (-15) :111. cFE: `CFE_ES_ERR_NAME_NOT_FOUND` 0xc4000002 ERR:350; `CFE_ES_ERR_RESOURCEID_NOT_VALID` 0xc4000001 ERR:342; `CFE_SB_BAD_ARGUMENT` 0xca000003 ERR:750; `CFE_EVS_APP_NOT_REGISTERED` 0xc2000002 ERR:258; `CFE_EVS_APP_ILLEGAL_APP_ID` 0xc2000003 ERR:267; `CFE_TBL_ERR_INVALID_NAME` 0xcc000002 ERR:894; `CFE_TBL_ERR_NEVER_LOADED` 0xcc000005 ERR:921; `CFE_TBL_ERR_UNREGISTERED` 0xcc000009 ERR:958; `CFE_TIME_CALLBACK_NOT_REGISTERED` 0xce000004 ERR:1364.

| Class | Record | PUBLISH | CONSUME | Key | Absent status |
|---|---|---|---|---|---|
| OSAL task | `OS_common_table[OS_TASK_BASE..]` OS/osapi-idmap.c:92 | `OS_TaskCreate` osapi-task.c:164 (alloc :187) | `OS_TaskGetIdByName` :343 (find :351); `OS_TaskGetInfo` :362; self `OS_TaskGetId` :328 | name / osal_id_t | -17 by name; -16 by ID |
| OSAL queue | idmap.c:93 | `OS_QueueCreate` osapi-queue.c:84 (:101) | `OS_QueueGetIdByName` :227 (:235); `OS_QueueGet` :151; `OS_QueuePut` :190 | name / ID | -17 / -16 |
| OSAL binary sem | idmap.c:94 | `OS_BinSemCreate` osapi-binsem.c:91 (:102) | `OS_BinSemGetIdByName` :233 (:241); Give :149, Take :191, TimedWait :212 | name / ID | -17 / -16 |
| OSAL counting sem | idmap.c:95 | `OS_CountSemCreate` osapi-countsem.c:83 (:94) | `OS_CountSemGetIdByName` :204 (:212); Give :141, Take :162, TimedWait :183 | name / ID | -17 / -16 |
| OSAL mutex | idmap.c:96 | `OS_MutSemCreate` osapi-mutex.c:88 (:99) | `OS_MutSemGetIdByName` :212 (:220); Give :146, Take :183 | name / ID | -17 / -16 |
| OSAL timebase | idmap.c:100 | `OS_TimeBaseCreate` osapi-timebase.c:93 (:122) | `OS_TimeBaseGetIdByName` :249 (:268) | name / ID | -17 / -16 |
| OSAL timer | idmap.c:101 | `OS_TimerCreate` osapi-time.c:236, `OS_TimerAdd` :204 (alloc :138) | `OS_TimerGetIdByName` :478 (:497) | name / ID | -17 / -16 |
| OSAL condvar, rwlock | idmap.c:105, :97 | `OS_CondVarCreate` osapi-condvar.c:80; `OS_RwLockCreate` osapi-rwlock.c:88 | `...GetIdByName` condvar :267, rwlock :278 | name / ID | -17 / -16 |
| OSAL module | idmap.c:102 | `OS_ModuleLoad` osapi-module.c:185 (:214) | no by-name ID lookup; `OS_SymbolLookup` :345, `OS_ModuleSymbolLookup` :388 | module name; symbol name | [U] symbol-miss status comes from the impl layer, not opened |
| OSAL file, dir, socket | idmap.c:98-99 | `OS_OpenCreate` osapi-file.c:107 (alloc with NULL name :136); dir :119; socket :129 | by ID only (`OS_read` ...) | path string; not unique in the ID map | [U] |
| ES application | `CFE_ES_Global.AppTable` ES/cfe_es_global.h:165 | `CFE_ES_AppCreate` cfe_es_apps.c:733; core: `CFE_ES_CreateObjects` cfe_es_start.c:788-825 | `CFE_ES_GetAppIDByName` cfe_es_api.c:627; `CFE_ES_GetAppID` :738; `GetAppName` :806; `GetAppInfo` :941 | name / CFE_ES_AppId_t / calling context | by name 0xc4000002 (:646); by ID or context 0xc4000001 (:760) |
| ES library | `LibTable` global.h:172 | `CFE_ES_LoadLibrary` cfe_es_apps.c:910 | `CFE_ES_GetLibIDByName` api.c:666; `GetLibName` :850; `GetLibInfo` :1036 | name / LibId | 0xc4000002 (:685) |
| ES task | `TaskTable[OS_MAX_TASKS]` global.h:157, slot from the OSAL task ID | `CFE_ES_StartAppTask` cfe_es_apps.c:649 (called by AppCreate :849, CreateObjects cfe_es_start.c:810, `CFE_ES_CreateChildTask` api.c:1202) | `CFE_ES_GetTaskIDByName` api.c:705 (calls `OS_TaskGetIdByName` :717); `GetTaskID` :774; `GetTaskInfo` :1129 | name / TaskId / context | 0xc4000002 (:725); context 0xc4000001 (:789) |
| ES CDS block | `CFE_ES_Global.CDSVars.Registry` global.h:183, cfe_es_cds.h:165 | `CFE_ES_RegisterCDS` api.c:1566; full name "App.Name" cfe_es_cds.c:688 | `CFE_ES_GetCDSBlockIDByName` api.c:1651; `CopyToCDS` :1732; `RestoreFromCDS` :1748 | full name / handle | 0xc4000002 (:1677); CDS unavailable `CFE_ES_NOT_IMPLEMENTED` (:1660-1662); pre-existing block on register: `CFE_ES_CDS_ALREADY_EXISTS` 0x4400000d (cfe_es_cds.c:471, ERR:415) |
| ES memory pool | `MemPoolTable` global.h:196 | `CFE_ES_PoolCreate*` cfe_es_mempool.c:150,170,190,207 | `CFE_ES_GetPoolBuf` :455 | handle only (no name) | 0xc4000001 (:477) |
| ES generic counter | `CounterTable` global.h:178 | `CFE_ES_RegisterGenCounter` api.c:1764 | `CFE_ES_GetGenCounterIDByName` :1919; `IncrementGenCounter` :1855; `GetGenCount` :1899 | name / CounterId | by name 0xc4000002 (:1941); by ID `CFE_ES_BAD_ARGUMENT` (:1857) |
| SB pipe | `CFE_SB_Global.PipeTbl` SB/cfe_sb_priv.h:243 + an OSAL queue of the same name | `CFE_SB_CreatePipe` cfe_sb_api.c:95 (`OS_QueueCreate` :163) | `CFE_SB_GetPipeIdByName` :780 (`OS_QueueGetIdByName` :802); `CFE_SB_ReceiveBuffer` :1408; `CFE_SB_Subscribe*` :900-937 | pipe name / PipeId | by name `CFE_SB_BAD_ARGUMENT` (:810, :825) + event 67 + `GetPipeIdByNameErrorCounter` (:843); same status for a NULL argument (:797) |
| SB route (subscription) | SBR `RoutingTbl` V7/cfe/modules/sbr/fsw/src/cfe_sbr_route_unsorted.c:56 | created by the first SUBSCRIBER: `CFE_SB_SubscribeFull` cfe_sb_api.c:937 (`CFE_SBR_AddRoute` :995) | SENDER: `CFE_SB_TransmitMsg` :1587, `TransmitBuffer` :1554 (`CFE_SBR_GetRouteId` cfe_sb_priv.c:1010) | MsgId (inside the message, not an argument) | none: `CFE_SUCCESS` (cfe_sb_priv.c:1085-1086); see section d |
| EVS registration | `CFE_EVS_Global.AppData[]` EVS/cfe_evs_task.h:116, slot from AppId | `CFE_EVS_Register` cfe_evs.c:42 | `CFE_EVS_SendEvent` :119; `SendEventWithAppID` :169; `SendTimedEvent` :222; `ResetFilter` :272 | implicit: caller's AppId (or AppID argument) | 0xc2000002 or 0xc2000003; see section d |
| TBL registry | `CFE_TBL_Global.Registry` TBL/cfe_tbl_task.h:188 | `CFE_TBL_Register` cfe_tbl_api.c:48; full name "App.Tbl" cfe_tbl_internal.c:203; `CFE_TBL_Load` :287 | `CFE_TBL_Share` :184 (by full name); `GetAddress` :439 (by handle) | full name / handle | by name 0xcc000002 (cfe_tbl_registry.c:236); never loaded 0xcc000005 (cfe_tbl_transaction.c:306); handle 0xcc000009 / `CFE_TBL_ERR_INVALID_HANDLE` (transaction.c:144-191) |
| TBL handle | `CFE_TBL_Global.Handles` cfe_tbl_task.h:187 | Register / Share | GetAddress, Manage, GetStatus | handle | as above |
| TIME synch callback | `CFE_TIME_Global.SynchCallback[]` TIME/cfe_time_api.c:632-639, one slot per AppIndex | `CFE_TIME_RegisterSynchCallback` :614 (needs `CFE_ES_GetAppID` :625) | tone task loop cfe_time_tone.c:1320 | caller's AppId | unregister miss 0xce000004 (:674); second register by the same app `CFE_TIME_TOO_MANY_SYNCH_CALLBACKS` (:635) |

Other OSAL facts a designer needs:
- [F] By-ID access to an object that is mid-creation (ID still reserved) is retried and then fails with `OS_ERR_OBJECT_IN_USE`, with the loop bound written as the literal `5` (OS/osapi-idmap.c:475-478). This is a third outcome besides present and absent.
- [F] A by-name match needs `name_entry != NULL` (OS/osapi-idmap.c:277); the name is stored by the class wrapper after allocation (OSH/os-shared-idmap.h:109-110). [U] The exact outcome of a by-name lookup that runs between allocation and finalization was not traced.

6.4.x differences (OSAL 4.1.1 + cFE 6.4.1):
- [F] One static table per class: O411/osapi.c:232-236 (task, queue, bin sem, count sem, mutex); IDs are table indexes (`*task_id = i` inside `OS_TaskGetIdByName`, O411/osapi.c:985-1014).
- [F] By-name APIs: `OS_TaskGetIdByName` O411/osapi.c:985 (-17 at :1014), `OS_QueueGetIdByName` :1935 (:1964), `OS_BinSemGetIdByName` :2483 (:2515), `OS_CountSemGetIdByName` :2984 (:3015), `OS_MutSemGetIdByName` :3375 (:3406), `OS_TimerGetIdByName` O411/ostimer.c:471 (:503). Same value -17 (osal-4.1.1-release/src/os/inc/osapi.h:104).
- [F] ES: `CFE_ES_GetAppIDByName` returns `CFE_ES_ERR_APPNAME` 0xc4000002 (G641/es/cfe_es_api.c:812; inc/cfe_error.h:229); context miss `CFE_ES_ERR_APPID` 0xc4000001 (cfe_es_api.c:1875; cfe_error.h:223). Same numeric values as v7.0.1, different names.
- [M] `CFE_SB_GetPipeIdByName` does not exist in 6.4.1 (`grep -n CFE_SB_GetPipeIdByName G641/sb/cfe_sb_api.c`: no hit). TBL share-miss is `CFE_TBL_ERR_INVALID_NAME` 0xcc000002 (cfe_error.h:932).

## b. Generic choke points
| Generation | Function (signature) | Passes through it | A hook sees |
|---|---|---|---|
| v7 OSAL | `int32 OS_ObjectIdAllocateNew(osal_objtype_t idtype, const char *name, OS_object_token_t *token)` OS/osapi-idmap.c:1159 | every create: 17 call sites in 15 class files [M] (binsem, condvar, countsem, dir, file, filesys x2, module, mutex, printf, queue, rwlock, sockets x2, task, time, timebase) | class, name (NULL for file/dir/socket), new ID in token, status (`OS_ERR_NAME_TAKEN` :1192, `OS_ERR_NO_FREE_IDS` :618) |
| v7 OSAL | `int32 OS_ObjectIdFinalizeNew(int32 operation_status, OS_object_token_t *token, osal_id_t *outid)` :838 | end of every create | final status and final ID; object becomes visible here (`OS_ObjectIdTransactionFinish` :859) |
| v7 OSAL | `int32 OS_ObjectIdFindByName(osal_objtype_t idtype, const char *name, osal_id_t *object_id)` :975 | every public `OS_*GetIdByName`: 10 call sites [M] (binsem, condvar, countsem, mutex, queue, rwlock, sockets, task, time, timebase) | class, name, found ID, status (-17 / 0) |
| v7 OSAL | `int32 OS_ObjectIdGetById(OS_lock_mode_t, osal_objtype_t idtype, osal_id_t id, OS_object_token_t *token)` :1013 | every by-ID use (4 to 10 call sites per class file [M]) | class, ID, status (-16, `OS_ERR_OBJECT_IN_USE`) |
| v7 OSAL | `OS_ObjectIdGetBySearch` :920 / `OS_ObjectIdFindNextMatch` :544 | all name matches incl. filesys and `OS_TaskFindIdBySystemData` | class, match function, argument, status |
| v7 OSAL (built-in) | `OS_NotifyEvent(OS_Event_t, osal_id_t, void *)` OS/osapi-common.c:75; one handler slot set by `OS_RegisterEventHandler` :274-278 | `OS_EVENT_RESOURCE_ALLOCATED` (idmap.c:1209), `_CREATED` (:864), `_DELETED` (:902), `OS_EVENT_TASK_STARTUP` (osapi-task.c:103) | event and ID only (data NULL, OSI/osapi-common.h:47-85); name via `OS_GetResourceName` (idmap.c:1416), class via `OS_IdentifyObject` (:1405). NO event for lookups or for by-ID use. The slot is already taken by the PSP (V7/psp/fsw/pc-linux/src/cfe_psp_start.c:361; its RESOURCE cases are empty :161-169) |
| v7 cFE | `CFE_ResourceId_t CFE_ResourceId_FindNext(CFE_ResourceId_t StartId, uint32 TableSize, CFE_ResourceId_CheckFunc_t CheckFunc)` RID:184 | every cFE ID allocation: app (cfe_es_apps.c:786, cfe_es_start.c:789), lib (:969), counter (cfe_es_api.c:1795), CDS (cfe_es_cds.c:352), pool (cfe_es_mempool.c:279), pipe (cfe_sb_api.c:139), TBL (regrec.c:121, accdesc.c:94, ...) | class base inside StartId, table size, new ID. NO name |
| v7 cFE | `int32 CFE_ResourceId_ToIndex(CFE_ResourceId_t Id, uint32 BaseValue, uint32 TableSize, uint32 *Idx)` RID:123 | every ID-to-slot conversion, hence every `Locate*RecordByID` | ID, class base, slot. Does NOT say whether the slot is in use (callers test `*IsMatch` afterwards, e.g. ES/cfe_es_resource.h:253-256) |
| v7 cFE | per-class locate functions: `CFE_ES_LocateAppRecordByName` cfe_es_resource.c:80, `...LibRecordByName` :115, `...CounterRecordByName` :150, `CFE_ES_LocateCDSBlockRecordByName` cfe_es_cds.c:751, `CFE_TBL_LocateRegRecByName` cfe_tbl_regrec.c:133; by ID: cfe_es_resource.c:185,208,231,254, cfe_es_cds.c:189, cfe_es_mempool.c:127, `CFE_SB_LocatePipeDescByID` cfe_sb_priv.c:225, `CFE_TBL_LocateAccDescByHandle` cfe_tbl_accdesc.c:69, `CFE_TBL_LocateRegRecByID` cfe_tbl_regrec.c:165 | one class each | name or ID, record pointer or NULL |
| v7 cFE | NO choke point for: by-name lookup across classes; SB route lookup (`CFE_SBR_GetRouteId`, two alternative implementations cfe_sbr_map_direct.c:91 / cfe_sbr_map_hash.c:151); EVS registration test (inline `EVS_AppDataIsMatch`, cfe_evs.c:136,190,242); TIME callbacks | | |
| 6.4.x | none. [M] `grep -rl "LocateAppRecord\|ResourceId\|OS_ObjectId" G641` = 0 files; `grep -c OS_RegisterEventHandler osal-4.1.1-release/src/os/inc/osapi.h` = 0. Per-class helpers only: `CFE_TBL_FindTableInRegistry` G641/tbl/cfe_tbl_internal.c:706, `CFE_TBL_ValidateHandle` :386, `CFE_SB_ValidatePipeId` G641/sb/cfe_sb_priv.c:577 | each public API separately | |

## c. Can the role of an API be derived mechanically?
[I] Yes for the two families below, from structure alone; the two seeds needed are framework-level (class b), not per application.

Example 1, OSAL counting semaphore (v7 generation). A static analysis has to see:
- consumer: `OS_CountSemGetIdByName(sem_id, sem_name)` returns the result of `OS_ObjectIdFindByName(LOCAL_OBJID_TYPE, sem_name, sem_id)` (OS/osapi-countsem.c:212). Class = constant argument 0 (0x03 `OS_OBJECT_TYPE_OS_COUNTSEM`, OSI/osapi-idmap.h:41); key = argument 1, a pass-through of parameter 1; ID out = parameter 0; absent = the only negative constant produced by the search itself (-17, OS/osapi-idmap.c:551) as opposed to argument checks (-2, -13).
- publisher: `OS_CountSemCreate` calls `OS_ObjectIdAllocateNew(LOCAL_OBJID_TYPE, sem_name, &token)` (osapi-countsem.c:94), stores the name, calls `OS_ObjectIdFinalizeNew` (same class constant, same name parameter position).
- pairing rule: equal class constant at the two idmap primitives.
- [M] What step 2a already holds for it (grep in FX): FX:77343 `OS_CountSemGetIdByName returns ... call-result OS_ObjectIdFindByName (seq 3 (seq param:sem_name#1 (seq param:sem_id#0)))`; FX:77309 `OS_CountSemCreate returns ... OS_ObjectIdAllocateNew (seq 3 (seq param:sem_name#1 ...))`; FX:77810 `OS_ObjectIdFindNextMatch returns ... const -17 [OS_ERR_NAME_NOT_FOUND]`; FX:77837 `OS_ObjectIdFindByName ... const -17 [ARGCHECK(...)]`; FX:63638 `OS_CountSemCreate field OS_common_record_t.name_entry at osapi-countsem.c:100`; application side FX:9126 `site 5805 ... cf_cfdp.c:993:19 CF_CFDP_InitEngine -> OS_CountSemGetIdByName direct {result-used,result-tested,in-loop}`, FX:3526 (BP site 291), FX:30675 (`CF_Channel_t.sem_id out-pointer ... via OS_ObjectIdFindByName`), FX:65776-65787 (BP guards: lookup result, then `OS_CountSemCreate` under `not (eq lookup 0)`).

Example 2, ES application by name (v7 generation). A static analysis has to see:
- consumer: `CFE_ES_GetAppIDByName(AppIdPtr, AppName)` returns `CFE_ES_ERR_NAME_NOT_FOUND` exactly when `CFE_ES_LocateAppRecordByName(AppName) == NULL` (ES/cfe_es_api.c:640-647); the locate function loops over `CFE_ES_Global.AppTable` with `strcmp(Name, CFE_ES_AppRecordGetName(...))` (cfe_es_resource.c:89-97). Role signature: status depends on a string comparison between a parameter and a field of a global table.
- publisher: `CFE_ES_AppCreate` copies its `AppName` parameter into that field (cfe_es_apps.c:802) and marks the record used (:821 reserved, :869 final); core apps in `CFE_ES_CreateObjects` (cfe_es_start.c:795-818). Role signature: copies a parameter into the same field.
- [M] In FX: FX:79024 `CFE_ES_GetAppIDByName returns 3288334338 [CFE_ES_ERR_NAME_NOT_FOUND] ... when=(and ... (eq (call-result CFE_ES_LocateAppRecordByName (seq param:AppName#1)) 0))`; FX:79093 `CFE_ES_LocateAppRecordByName returns 0 [NULL] ... when=(eq local:Count 0)`; FX:29973 `CFE_ES_AppRecord_t.AppName memcpy at ...cfe_es_apps.c:721:13 in CFE_ES_AppCreate value=top [copy of AppName]`; FX:68696 (reader `CFE_ES_AppRecordGetName`).
- What 2a does NOT hold: any class/role table. [M] The `[p0]` section (FX:65105-65321) has 72 entries in 20 families, all generic primitives (thread-create, mutex-acquire/release, condition-wait/signal, semaphore-wait/post, sleep, process-exit, thread-specific, dynamic-load, symbol-lookup, data-function, ...); no OSAL or cFE object API is in it.

Hard example, SB send without subscriber and its relatives:
- [F] `CFE_SB_TransmitMsg` returns `CFE_SUCCESS` when no route exists (SB/cfe_sb_priv.c:1085-1086): there is no absent status to derive; the outcome exists only as a counter and a filtered event.
- [F] The key (MsgId) is read from the message content, not passed as an argument (cfe_sb_priv.c:1010 uses `TxnPtr->RoutingMsgId`).
- [F] The table entry is created by the receiver (`CFE_SBR_AddRoute` in `CFE_SB_SubscribeFull`, cfe_sb_api.c:990-995), so "publisher of the object" (subscriber) is the opposite of "producer of the data" (sender).
- [F] `CFE_SB_GetPipeIdByName`: absent returns `CFE_SB_BAD_ARGUMENT` (:810, :825), the same constant as a NULL argument (:797); the status constant alone cannot identify "absent".
- [F] EVS and TIME: the key is the caller's identity, obtained inside the API (`EVS_GetCurrentContext` EVS/cfe_evs_utils.c:71-78; cfe_time_api.c:625); no key argument exists.

## d. Cross-application channels and what happens when the provider has not acted yet
| Channel | Provider act | Consumer act too early: outcome | Consumer told? | Trace left |
|---|---|---|---|---|
| OSAL named object | `OS_*Create` | `OS_*GetIdByName` returns -17 | yes, status | none in the framework (no event, no counter) [F OS/osapi-idmap.c:975-999] |
| SB message | receiver `CFE_SB_Subscribe` | sender `CFE_SB_TransmitMsg`: `CFE_SUCCESS`; message dropped | NO | `NoSubscribersCounter++` (cfe_sb_priv.c:1085); event `CFE_SB_SEND_NO_SUBS_EID` = 14 (SB/../inc/cfe_sb_eventids.h:188), type INFORMATION (cfe_sb_priv.c:655-657), text "No subscribers for MsgId 0x%x,sender %s" (:661); default filter `CFE_EVS_FIRST_4_STOP` (V7/cfe/modules/sb/fsw/inc/cfe_sb_internal_cfg.h:222,225). Buffer use count is dropped after the send (cfe_sb_priv.c:1285): [I] nothing is retained for a later subscriber |
| SB pipe by name | `CFE_SB_CreatePipe` | `CFE_SB_GetPipeIdByName`: `CFE_SB_BAD_ARGUMENT` | yes, but ambiguous status | event 67 `CFE_SB_GETPIPEIDBYNAME_NAME_ERR_EID` (cfe_sb_eventids.h:727; sent at cfe_sb_api.c:869-870); `GetPipeIdByNameErrorCounter` (:843) |
| SB subscribe on a missing pipe | `CFE_SB_CreatePipe` | `CFE_SB_BAD_ARGUMENT` (cfe_sb_api.c:971-974) | yes | event `CFE_SB_SUB_INV_PIPE_EID` (:973), `SubscribeErrorCounter` (:1084) |
| Shared table | owner `CFE_TBL_Register` | `CFE_TBL_Share`: 0xcc000002 | yes | syslog "Table '%s' not found in Registry" (cfe_tbl_api.c:219); ERROR event `CFE_TBL_SHARE_ERR_EID` sent with TBL's own AppId (:222-228) |
| Shared table content | owner `CFE_TBL_Load` | `CFE_TBL_GetAddress`: `CFE_TBL_ERR_NEVER_LOADED` (cfe_tbl_transaction.c:303-306) | yes | [U] event not checked |
| CDS | `CFE_ES_RegisterCDS` | `CFE_ES_GetCDSBlockIDByName`: 0xc4000002 (cfe_es_api.c:1677) | yes | none seen at :1651-1682 |
| Library state | `CFE_ES_LoadLibrary` calls the init function synchronously in the loading task (cfe_es_apps.c:1013-1016), before the record is finalized (:1026-1029) | library functions are ordinary symbols: no framework check exists that the init ran | NO | init failure only: syslog (:1019) |
| Child task | parent app main task `CFE_ES_CreateChildTask` (cfe_es_api.c:1202) | caller without a resolvable app context: 0xc4000001 + syslog (:1262-1266); caller is itself a child: `CFE_ES_ERR_CHILD_TASK_CREATE` (:1268-1273) | yes | syslog |
| EVS, app not registered | `CFE_EVS_Register` | `CFE_EVS_SendEvent`: `CFE_EVS_APP_NOT_REGISTERED` (EVS/cfe_evs_utils.c:181) | yes (status usually discarded by callers [I]) | once per app: `UnregisteredAppCounter++` (:161), EVS ERROR event `CFE_EVS_ERR_UNREGISTERED_EVS_APP` = 41 (:170-173; EVS/../inc/cfe_evs_eventids.h:457), syslog (:176) |
| EVS, caller identity unresolved | ES finalizing the app record (section e) | `CFE_EVS_SendEvent`/`SendEventWithAppID`: `CFE_EVS_APP_ILLEGAL_APP_ID` (cfe_evs_utils.c:91; cfe_evs.c:186-189) | yes, status only | NOTHING: no event, no syslog, no counter on that branch [F cfe_evs.c:131-158, 183-189] |
| Service's own AppId global | the service task storing its AppId (R2: SB) | events sent on behalf of the service with an undefined AppId take the row above | no | nothing |
| TIME synch callback | `CFE_TIME_RegisterSynchCallback` | not registered: callback simply not called (cfe_time_tone.c:1320 loop) | no | none |

6.4.1 differences: [F] no-subscriber path is the same shape: `NoSubscribersCnt++` (G641/sb/cfe_sb_api.c:1197), INFORMATION event `CFE_SB_SEND_NO_SUBS_EID` (:1207-1209). EVS: `EVS_NotRegistered` does counter + ERROR event + syslog (G641/evs/cfe_evs_utils.c:202-218); unresolved caller returns the status of `CFE_ES_GetAppID` or `CFE_EVS_APP_ILLEGAL_APP_ID` for an out-of-range ID (:127-134). Library init runs synchronously too (G641/es/cfe_es_apps.c:882).

## e. Application identity
v7.0.1:
- [F] `CFE_ES_GetAppID` (ES/cfe_es_api.c:738) = `CFE_ES_GetAppRecordByContext` (cfe_es_resource.c:308): `OS_TaskGetId()` -> `CFE_ES_LocateTaskRecordByID` -> test `TaskRecPtr->TaskId == TaskID` (:285-294) -> `TaskRecPtr->AppId` -> `CFE_ES_LocateAppRecordByID` -> test `AppRecPtr->AppId == TaskRecPtr->AppId` (inside :308-342). Any failed link gives `CFE_ES_ERR_RESOURCEID_NOT_VALID` (cfe_es_api.c:760).
- [F] `OS_TaskGetId` reads a per-thread value (OS/osapi-task.c:328-332; V7/osal/src/os/posix/src/os-impl-tasks.c:863) that the OSAL task wrapper sets itself before the entry function runs: `OS_TaskEntryPoint` -> `OS_TaskPrepare` (osapi-task.c:78-109: waits on the task ID with a global lock :84, `OS_TaskRegister_Impl` :97, `OS_EVENT_TASK_STARTUP` :103) -> `(*task_entry)()` (:132). So the OS task ID is valid at the first instruction of the entry function.
- [F] Parent order in `CFE_ES_StartAppTask`: `OS_TaskCreate(..., TaskName, CFE_ES_TaskEntryPoint, ...)` at cfe_es_apps.c:664-670 with NO ES lock; lock at :672; task record filled :694-703 (`AppId` :696, `SetUsed` :703); unlock :722.
- [F] Child order: every ES task starts in `CFE_ES_TaskEntryPoint` (:613), which calls `CFE_ES_GetTaskFunction` (:625): loop of `OS_TaskDelay(CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC)` (:575), lock, `CFE_ES_GetTaskRecordByContext`, exit when `AppId` defined and `EntryFunc != 0` (:582) or when the budget `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` is used up (:570, :589, :595); then `(*RealEntryFunc)()` (:639). Both constants are framework configuration names.
- [F] The app record is finalized later: reserved at cfe_es_apps.c:821, `CFE_ES_LoadModule` :841, `CFE_ES_StartAppTask` :849-854, real ID set under lock at :861-869. Core apps: reserved cfe_es_start.c:803, start :810, real ID :818.
- [I] Therefore a window exists in v7.0.1 between cfe_es_apps.c:722 and :869 (core: cfe_es_start.c:810 to :818) in which the child's real entry function may already run while `CFE_ES_GetAppID` fails on the app-record link. R/cases/R2.md:114 states the same reading.
6.4.1 / 6.4.2:
- [F] `OS_TaskGetId` returns the value of a thread key (O411/osapi.c:959-971) that is set only by `OS_TaskRegister` (:906-946), which the application calls itself through `CFE_ES_RegisterApp` (G641/es/cfe_es_api.c:776) or `CFE_ES_RegisterChildTask` (:1161-1171). [I] Before that call the key is unset and the function yields 0.
- [F] `CFE_ES_GetAppIDInternal`: `TaskId = OS_TaskGetId(); if (TaskTable[TaskId].RecordUsed == TRUE) AppId = TaskTable[TaskId].AppId` else `CFE_ES_ERR_APPID` (G641/es/cfe_es_api.c:1873-1891).
- [F] 6.4.1 `CFE_ES_AppCreate`: unlock at G641/es/cfe_es_apps.c:610, `OS_TaskCreate(&...MainTaskId, AppName, (void*)StartAddr, ...)` at :614-620 (the application entry point is the thread entry, no gate), lock :637, task record written :643-655. The child's first instruction can precede the record.
- [F] 6.4.2: lock at G642/es/cfe_es_apps.c:571 is held across `OS_TaskCreate` (:619) and the record write (:646-655), unlock :669. [I] The child can still execute code that does not take the ES lock before the record exists.
- [F] 6.4.1 `CFE_ES_CreateChildTask` already holds the lock across creation and record write (G641/es/cfe_es_api.c:1076, :1108, :1116-1117, :1145).
- [F] OSAL 4.1.1 writes the task name into its table after the thread is created (`pthread_create` O411/osapi.c:645; `strcpy(...name...)` :698).
Identity that does not depend on those records:
- [F] Creation-call arguments: task name (argument 2 of `OS_TaskCreate`; for a main task it equals the app name, cfe_es_apps.c:850; 6.4.1 :615) and the real entry function (v7: argument `EntryFunc` of `CFE_ES_StartAppTask`, = `LoadStatus.InitSymbolAddress` :851-852, i.e. the entry-point symbol from the startup script; 6.4.1: argument 3 of `OS_TaskCreate` :616).
- [F] v7 only: the OSAL record of every object stores the creator task ID (`obj->creator = OS_TaskGetId()`, OS/osapi-idmap.c:647) and the task name is in the OSAL table before the thread starts (`OS_OBJECT_INIT` at osapi-task.c:192-193, before `OS_TaskCreate_Impl`).
- [F] v7: the OS-level entry function is the same for every ES task (`CFE_ES_TaskEntryPoint`, cfe_es_apps.c:666), so it does not identify the application.

## f. What the framework exposes about itself (raw material for comparing two runs)
| Source | Where defined | Stable names | Run-varying |
|---|---|---|---|
| ES `QUERY_ALL` (file of `CFE_ES_AppInfo_t`) | ES/../inc/cfe_es_fcncodes.h:430; struct V7/cfe/modules/es/config/default_cfe_es_extern_typedefs.h:378-424 | Name, EntryPoint, FileName, Type, StackSize, Priority, ExceptionAction, MainTaskName, NumOfChildTasks | ResourceId, MainTaskId, Code/Data/BSS/Start addresses, ExecutionCounter |
| ES `QUERY_ALL_TASKS` (`CFE_ES_TaskInfo_t`) | fcncodes.h:1034; typedefs.h:439-447 | TaskName, AppName, StackSize, Priority | TaskId, AppId, ExecutionCounter |
| ES `QUERY_ONE`, `WRITE_SYS_LOG`, `WRITE_ER_LOG`, `DUMP_CDS_REGISTRY` | fcncodes.h:388, :508, :585, :992; `CFE_ES_CDSRegDumpRec_t` typedefs.h:461 | CDS Name, Size, Table flag; syslog message text | CDS Handle; syslog time stamps [U: line format not opened] |
| SB `WRITE_ROUTING_INFO` (`CFE_SB_RoutingFileEntry_t`) | SB/../inc/cfe_sb_fcncodes.h:194; V7/cfe/modules/sb/config/default_cfe_sb_msgdefs.h:210 | MsgId, AppName, PipeName, State | PipeId, MsgCnt |
| SB `WRITE_PIPE_INFO` (`CFE_SB_PipeInfoEntry_t`) | fcncodes.h:318; msgdefs.h:143 | PipeName, AppName, MaxQueueDepth, Opts | PipeId, AppId, CurrentQueueDepth, PeakQueueDepth, SendErrors |
| SB `WRITE_MAP_INFO` (`CFE_SB_MsgMapFileEntry_t`) | fcncodes.h:364; msgdefs.h:225 | MsgId | Index (route slot; [I] first-subscription order) |
| SB `SEND_SB_STATS`, `SEND_PREV_SUBS` | fcncodes.h:149, :462 | | counts, depths |
| TBL `DUMP_REGISTRY` (`CFE_TBL_RegDumpRec_t`), `SEND_REGISTRY` | TBL/../inc/cfe_tbl_fcncodes.h:345, :380; TBL/cfe_tbl_task.h:113 | Name, OwnerAppName, Size, DumpOnly, DoubleBuffered, CriticalTable, ValidationFunc present, LastFileLoaded, TableLoadedOnce | TimeOfLastUpdate, FileTime, NumUsers, LoadInProgress, LoadPending |
| EVS `WRITE_LOG_DATA_FILE` (event packets) | EVS/../inc/cfe_evs_fcncodes.h:804; `CFE_EVS_PacketID_t` V7/cfe/modules/evs/config/default_cfe_evs_msgdefs.h:215-232 | AppName, EventID, EventType, SpacecraftID, ProcessorID, Message text | packet time and sequence; numbers printed inside the text |
| EVS `WRITE_APP_DATA_FILE` (`CFE_EVS_AppDataFile_t`) | fcncodes.h:769; EVS/cfe_evs_task.h:111 | AppName, ActiveFlag, EventTypesActiveFlag, Filters | EventCount, SquelchedCount |
| Housekeeping counters | ES msgdefs default_cfe_es_msgdefs.h:359; SB default_cfe_sb_msgdefs.h:107; EVS default_cfe_evs_msgdefs.h:211; TBL default_cfe_tbl_msgdefs.h:234 | field names | values: ES RegisteredCoreApps, RegisteredExternalApps, RegisteredTasks, RegisteredLibs, SysLogEntries, ERLogEntries; SB NoSubscribersCounter, MsgSendErrorCounter, CreatePipeErrorCounter, SubscribeErrorCounter, DuplicateSubscriptionsCounter, GetPipeIdByNameErrorCounter, PipeOverflowErrorCounter, MsgLimitErrorCounter; EVS UnregisteredAppCounter, MessageSendCounter, MessageTruncCounter, AppData[].AppMessageSentCounter; TBL NumTables, NumLoadPending |
| Internal tables readable directly | `OS_common_table` slices OS/osapi-idmap.c:92-105 (fields name_entry, active_id, creator, refcount: OSH/os-shared-idmap.h:39-42); `CFE_ES_Global` TaskTable/AppTable/LibTable/CounterTable/CDSVars/MemPoolTable (ES/cfe_es_global.h:157-196); `CFE_SB_Global.PipeTbl` (SB/cfe_sb_priv.h:243); SBR `RoutingTbl` (cfe_sbr_route_unsorted.c:56); `CFE_EVS_Global.AppData` (EVS/cfe_evs_task.h:116); `CFE_TBL_Global.Handles/Registry` (TBL/cfe_tbl_task.h:187-188); `CFE_TIME_Global.SynchCallback` (TIME/cfe_time_api.c:632) | name strings | IDs, pointers, counters |
| Public iteration API | `OS_ForEachObject` OS/osapi-idmap.c:1360, `OS_ForEachObjectOfType` :1376, `OS_GetResourceName` :1416, `OS_IdentifyObject` :1405 | names, class | IDs |
- [F] ID structure. OSAL: ID = class << 16 | serial (OSH/os-shared-idmap.h:266-268; OSI/osapi-idmap.h:32-33); serial = last issued of that class + 1 (OS/osapi-idmap.c:620, :625, :642). cFE: ID = class base + serial, serial = last ID + 1 (RID:79-87; start value `CFE_ES_Global.LastAppId`, cfe_es_apps.c:786, :822). [I] Within one boot without deletions the numeric ID is the creation ordinal inside its class: comparing IDs across two runs compares creation order, and the name is the stable join key.
- 6.4.x: [F] IDs are table indexes of the first free slot (O411/osapi.c:232-236 tables; G641/es/cfe_es_apps.c:451-456 app slot).

## g. Census of by-name lookup call sites [M]
Command (per root, tests and stubs filtered out): `grep -rnE 'OS_[A-Za-z]*GetIdByName *\(|CFE_ES_Get[A-Za-z]*IDByName *\(|CFE_SB_GetPipeIdByName *\(|CFE_TBL_Share *\(' --include=*.c --include=*.h <root> | grep -v "unit-test|ut-stubs|/eds/|..."`. Each site was then opened.
Scope A: V7/apps (17 apps) and V7/libs: 11 sites (cf 1, cs 3, hs 6, sch_lab 1; libs 0).
| # | Caller | path:line (under V7/apps) | API | Key | Status checked | Failing branch |
|---|---|---|---|---|---|---|
| A1 | `HS_SendHkCmd` | hs/fsw/src/hs_cmds.c:89 | `CFE_ES_GetAppIDByName` | table field `EMTablePtr[i].AppName` | yes | counts `InvalidEventMonCount` only |
| A2 | `HS_SendHkCmd` | hs_cmds.c:148 | `CFE_ES_GetTaskIDByName` | table field `XCTablePtr[i].ResourceName` | yes | value stays `HS_INVALID_EXECOUNT` (sentinel) |
| A3 | `HS_SendHkCmd` | hs_cmds.c:161 | `CFE_ES_GetGenCounterIDByName` | same field | yes | sentinel |
| A4 | `HS_MonitorApplications` | hs/fsw/src/hs_monitors.c:69 | `CFE_ES_GetAppIDByName` | table field `AMTablePtr[i].AppName` | yes | ERROR event `HS_APPMON_APPNAME_ERR_EID` the first time, DEBUG event afterwards; continues |
| A5 | `HS_MonitorEvent` | hs_monitors.c:305 | `CFE_ES_GetAppIDByName` | table field | yes | ERROR event `HS_EVENTMON_NOT_RESTARTED_ERR_EID` |
| A6 | `HS_MonitorEvent` | hs_monitors.c:331 | `CFE_ES_GetAppIDByName` | table field | yes | ERROR event `HS_EVENTMON_NOT_DELETED_ERR_EID` |
| A7 | `CS_RefreshTableHandleAndAddress` | cs/fsw/src/cs_compute.c:222 | `CFE_TBL_Share` | function parameter `Name` | yes (`< CFE_SUCCESS`) | handle set to `CFE_TBL_BAD_TABLE_HANDLE`, status returned to the caller |
| A8 | `CS_RefreshAppSizeAndAddr` | cs_compute.c:307 | `CFE_ES_GetAppIDByName` | function parameter `AppName` | yes (`== CFE_ES_ERR_NAME_NOT_FOUND`) | fallback: second lookup A9 |
| A9 | `CS_RefreshAppSizeAndAddr` | cs_compute.c:311 | `CFE_ES_GetLibIDByName` | same parameter | yes | ERROR event `CS_COMPUTE_APP_ERR_EID` |
| A10 | `CF_CFDP_InitEngine` | cf/fsw/src/cf_cfdp.c:1282 | `OS_CountSemGetIdByName` | table field `config_table->chan[i].sem_name` | yes | retry while -17, bound `CF_STARTUP_SEM_MAX_RETRIES` with `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)` (:1280-1291), then ERROR event `CF_INIT_SEM_ERR_EID` and break (:1293-1301); init fails, `RunStatus = APP_ERROR` (cf_app.c:291-294) |
| A11 | `SCH_LAB_AppInit` | sch_lab/fsw/src/sch_lab_app.c:176 | `OS_TimeBaseGetIdByName` | LITERAL "cFS-Master" | yes | syslog + return `CFE_STATUS_EXTERNAL_RESOURCE_FAIL` (:177-181); `RunStatus = APP_ERROR` (:85-89) |
Counts for scope A (n = 11): key literal 1, table field 7, function parameter 3. Status checked 11, ignored 0. Failing branch: exit 1 (A11), bounded retry then exit 1 (A10), error event and continue 4 (A4, A5, A6, A9), fallback lookup 1 (A8), count or sentinel only 3 (A1, A2, A3), error returned to caller 1 (A7), create on miss 0.
Scope B: the two C1 bundles (apps ci_lab sample_app sch_lab to_lab; libs sample_lib): 1 site each, `apps/sch_lab/fsw/src/sch_lab_app.c:176`, same shape as A11. The file is identical in the two bundles (`cmp`), textually different from V7 at line 2.
Scope C: CF and BP at the C1 revisions (not in the bundles; /home/user/work/expect_src; `cmp` shows c1_build/pre/src/cf = CF_281a941, post/src/cf = CF_833fdbb, pre/src/bp = bp_b051eb0):
| # | Caller | path:line | Key | Checked | Failing branch |
|---|---|---|---|---|---|
| C-1 | `CF_CFDP_InitEngine` | CF_281a941/fsw/src/cf_cfdp.c:993 | table field `sem_name` (type at cf_tbldefs.h:74) | yes | ERROR event `CF_EID_ERR_INIT_SEM` + break (:994-1000); NO retry |
| C-2 | `CF_CFDP_InitEngine` | CF_833fdbb/fsw/src/cf_cfdp.c:1003 | same | yes | bounded retry, `CF_STARTUP_SEM_MAX_RETRIES 25`, `CF_STARTUP_SEM_TASK_DELAY 100` (CF_833fdbb/fsw/platform_inc/cf_platform_cfg.h:338, :349), then event + break |
| C-3 | `CF_CFDP_InitEngine` | CF_15a871e/fsw/src/cf_cfdp.c:1282 | same | yes | same as A10 |
| C-4 | `initialize_throttling` | bp_b051eb0/fsw/src/bp_flow.c:126 | static array field `BP_Throttles[i].Name` (/home/user/work/procrace/c1_build/pre/src/bp/fsw/custom/bp_semcfg.c:68) | yes (compared with `CFE_SUCCESS`, :127) | CREATE ON MISS: `OS_CountSemCreate` (:132); ERROR event only if the create also fails (:140) |
bplib_d4c6db9: 0 sites.
Other framework-internal by-name lookups (not applications): TIME/cfe_time_task.c:311 `OS_TimeBaseGetIdByName(&TimeBaseId, "cFS-Master")` (name macro V7/psp/fsw/inc/cfe_psp_timertick_api.h:53); cfe_sb_api.c:802; cfe_es_api.c:705-725.
Size of the wider consume/publish surface in V7/apps (non-test .c files, same filter) [M]: `CFE_SB_Subscribe*` 49, `CFE_SB_CreatePipe` 22, `CFE_SB_Transmit*` 43, `CFE_TBL_Register` 29, `CFE_TBL_GetAddress` 47, `CFE_ES_RegisterCDS` 6, `CFE_ES_CreateChildTask` 11, `CFE_EVS_Register` 15, `OS_*Create` 14, `OS_SymbolLookup` 5, `CFE_ES_WaitForSystemState|WaitForStartupSync` 5.

## Observations for designers (not a design)
1. [I] In C1 the same class and key are touched by a lookup-only consumer (CF) and a lookup-then-create participant (BP). The shipped software itself shows three consumer shapes for one object family: fail (C-1), bounded retry (C-2), create on miss (C-4).
2. [I] By-name lookup is the smallest cross-application channel in the shipped apps (11 sites) compared with SB (49 subscribe / 43 transmit sites) and TBL; SB and EVS are the channels where early use is silent or returns success.
3. [F] The key at 10 of 11 sites is not a literal at the call site; in C1 it is a table image on one side and a static initializer on the other. Matching publisher and consumer needs the value of table/initializer data, not call-site text.
4. [F] The retry bound in C-2 (25, 100) and the gate constants in v7 ES (`CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC`, `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`) are configuration macros of the app or the framework; the OSAL in-use retry bound is the literal 5 (OS/osapi-idmap.c:476).
5. [F] The only framework notification hook (`OS_NotifyEvent`) covers creation, deletion and task start, never lookups; it has one slot, already used by the PSP.
6. [F] A failing `CFE_EVS_SendEvent` caused by unresolved identity leaves no trace anywhere in the framework, so absence of evidence in EVS log, syslog and counters is not evidence of absence for that failure.

## Unknowns
- [U] Status returned by `OS_SymbolLookup` / `OS_ModuleSymbolLookup` on a missing symbol, and by file opens on a missing path (impl layer not opened).
- [U] Outcome of `OS_*GetIdByName` racing with a create of the same name between allocation and finalization.
- [U] Whether `CFE_TBL_GetAddress` on a never-loaded table sends an event; whether A7's caller retries on a later cycle.
- [U] ES syslog and ER log record formats (which fields carry time), and the exact file header of each dump file.
- [U] Value read by `OS_TaskGetId` in OSAL 4.1.1 before `OS_TaskRegister` (inferred 0, not executed).
- [U] Whether the app-record window of section e is reachable in practice at v7.0.1 for external apps (code reading only; nothing was run).
- [U] Completeness of the census filter: files in directories named test/stubs were excluded by path pattern; fsw sources were not excluded (11 raw hits = 11 kept hits in V7/apps).

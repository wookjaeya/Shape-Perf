# 01 — Concurrency architecture of OSAL (POSIX + shared layer) and PSP (pc-linux)

Scope: the parts of OSAL and PSP at the pinned revision that decide **when an OS resource exists, when it can be
found, how tasks start and stop, and which operations order one task's actions before another's**. This is the
ground layer a process-level race detector for cFS has to model or observe.

Pinned revision: nasa/cFS v7.0.1 tree at `/home/user/work/procrace/cfs_ref/cFS`; osal `d2d877a69cff`,
psp `c4b3b0b65b11`, cfe `c5fb2b4d540b` (verified with `git -c safe.directory='*' log -1`). All paths below are
relative to that tree. Line numbers are at that revision.

Build facts used (from the built tree `build-native_std`, not from defaults):
`build-native_std/native/default_cpu1/osal/inc/osconfig.h` L21-28 defines `OSAL_CONFIG_DEBUG_PERMISSIVE_MODE`,
`OSAL_CONFIG_DEBUG_PRINTF`, `OSAL_CONFIG_CONSOLE_ASYNC`, the dynamic loader and network. Limits: `OS_MAX_TASKS` 64,
`OS_MAX_QUEUES` 64, `OS_MAX_BIN_SEMAPHORES` 20, `OS_MAX_COUNT_SEMAPHORES` 20, `OS_MAX_MUTEXES` 40,
`OS_MAX_CONDVARS` 4, `OS_MAX_TIMEBASES` 5, `OS_MAX_TIMERS` 10, `OS_MAX_API_NAME` 20, `OS_QUEUE_MAX_DEPTH` 50
(L45-233). `CMakeCache.txt` L573 `OSAL_CONFIG_RWLOCK:BOOL=TRUE`, L503 `OSAL_CONFIG_INCLUDE_SHELL:BOOL=FALSE`.

Labels used throughout:

* **[F]** fact: the code, a build artifact or a document says this (citation given).
* **[E]** environment fact taken from our own records (`env/CONDITIONS.md`, `env/BASELINE_MEASURE.md`).
* **[I]** my inference from the cited facts. Not observed at run time unless stated.
* **[U]** unverified.

No experiment was run for this report (the task did not ask for one). Section 7 lists the experiments that would
confirm the inferences.

---

## 0. Summary of what matters for the detector

1. **[F] Every OSAL resource goes through three ID-table states**: free (`active_id == 0`), RESERVED
   (`active_id == 0xFFFFFFFF`) and active (a real ID). Creation and deletion pass through RESERVED, and during
   RESERVED the per-type table lock is **not** held (Section 1.5). The resource becomes findable by ID and by name
   exactly when `OS_ObjectIdTransactionFinish` writes the final ID under the type lock
   (`osal/src/os/shared/src/osapi-idmap.c` L1090-1093, release at L1105).
2. **[I] A by-name or by-search lookup that hits a RESERVED record leaks the type's table lock** (Finding F1,
   Section 1.8). `OS_ObjectIdConvertToken` returns early without unlocking (osapi-idmap.c L401-404) and
   `OS_ObjectIdGetBySearch` does not cancel in that case (L939-944). The table mutex is a non-recursive
   `PTHREAD_MUTEX_NORMAL` (`osal/src/os/posix/src/os-impl-idmap.c` L222). The creator or deleter then blocks
   forever in `OS_Lock_Global`. I found no upstream issue on this. It needs the experiment in Section 7 before it is
   claimed as a bug.
3. **[F] A new task can run before its creator finishes, but OSAL holds it back with a bounded wait.**
   `pthread_create` runs while the record is RESERVED. The child's `OS_TaskPrepare` waits on the type's condition
   variable for at most 4 waits (10+40+90+160 ms, ~300 ms) and gives up on the 5th attempt
   (osapi-idmap.c L475-480; os-impl-idmap.c L153-169). **[I]** Each task-table unlock anywhere in the process also
   uses up one attempt. If the child gives up, it exits without running its entry point and leaves a zombie record
   (Finding F7).
4. **[F] cFE ES's own task bookkeeping comes after `OS_TaskCreate` returns** (`cfe/modules/es/fsw/src/cfe_es_apps.c`
   L664-722), so it is not ordered before the child. ES closes that gap by polling under its shared-data lock
   (L573-596). Section 2.7.
5. **[F] Priorities have no effect in this environment.** Permissive mode overrides every task priority to 255 and
   threads keep `SCHED_OTHER` (os-impl-tasks.c L429-439, L612-615). **[E]** This was measured for all 27 threads
   (CONDITIONS M1-5). **[F] The PSP pins every OSAL task whose name starts with `CFE_` to CPU 0** in its
   task-startup hook (`psp/fsw/pc-linux/src/cfe_psp_start.c` L184-189). **[E]** This is not yet recorded in our
   environment records. Only the launcher's `Cpus_allowed_list 0-3` is recorded.
6. **[F] The OSAL primitives in use by cFS are tasks, mutexes, binary and counting semaphores, queues, timers and
   timebases, files and dirs, sockets and select, and modules and symbols.** Condvars and rwlocks are compiled but
   have no non-test caller in cFE, apps or PSP (grep, Section 3). Sections 3 and 6 give each primitive's
   happens-before (HB) relation at the API level, which differs from the implementation level.
7. **[F] Name-based discovery is a real pattern in this tree.** Examples: the `cFS-Master` timebase is created by
   the PSP and found by TIME and SCH_LAB, CF looks up a semaphore by name with an explicit "start up race" retry
   loop, SB looks up pipes by name, ES looks up tasks by name, and the PSP maps a crashed thread back to a task ID by
   search (Section 4).
8. **[F] PSP state that persists across runs**: SysV shm segments (CDS, reset area with boot record and exception
   log, user-reserved area) keyed by `ftok` of CWD key files, `EEPROM.DAT`, and files under `/dev/shm/osal:RAM`.
   The reset type of run *n+1* depends on how run *n* ended (Section 5).
9. **Observation constraints.** [F] OSAL, PSP and cFE core are linked statically (`--whole-archive`) into
   `core-cpu1` with `--export-dynamic`. [I] So an `LD_PRELOAD` interposer sees app→OSAL calls but **not**
   cFE-core→OSAL calls (Section 6.4). [I] Instrumentation must not run inside a table lock, must not call OSAL name
   or search lookups or `OS_printf`, and must be cancellation-safe (Section 6.3).

---

## 1. OSAL object model and ID map

### 1.1 Object types and the common table

* **[F]** The public type codes are task 0x01, queue 0x02, countsem 0x03, binsem 0x04, mutex 0x05, stream 0x06,
  dir 0x07, timebase 0x08, timecb 0x09, module 0x0A, filesys 0x0B, console 0x0C, condvar 0x0D, rwlock 0x0E, and user
  types from 0x10 (`osal/src/os/inc/osapi-idmap.h` L38-53).
* **[F]** One static array `OS_common_table[OS_MAX_TOTAL_RECORDS]` (osapi-idmap.c L76) is divided into per-type
  slices by `OS_ObjectIndex_t` (`osal/src/os/shared/inc/os-shared-idmap.h` L45-62). Each common record holds
  `name_entry` (a pointer into the type-specific record), `active_id`, `creator` and `refcount`
  (os-shared-idmap.h L37-43).
* **[F]** Type-specific shared records (e.g. `OS_task_table`, osapi-task.c L62) and implementation records
  (e.g. `OS_impl_task_table`, os-impl-tasks.c L63) are parallel arrays indexed by the same slot index.
* **[F]** Per-type state `OS_objtype_state[]` holds `last_id_issued`, `transaction_count` and `owner_key`
  (osapi-idmap.c L78-90).

### 1.2 IDs, serial numbers and reuse

* **[F]** An ID is `(type << 16) | serial` (os-shared-idmap.h L266-269; `OS_OBJECT_TYPE_SHIFT` 16 and
  `OS_OBJECT_INDEX_MASK` 0xFFFF at osapi-idmap.h L32-33). The slot index is `serial % OS_MAX_<type>`
  (osapi-idmap.c L1495).
* **[F]** The value 0 is "undefined". `OS_ObjectIdDefined(id)` is `id != 0` (osapi-idmap.h L151-154).
  `OS_OBJECT_ID_RESERVED` is 0xFFFFFFFF (os-shared-idmap.h L32). It counts as *defined* but not *valid*
  (`OS_ObjectIdIsValid` requires `0 < type < 0x10`, L285-289).
* **[F]** Allocation (`OS_ObjectIdFindNextFree`, osapi-idmap.c L593-661) starts at `last_id_issued + 1`
  (L620-625), wraps the serial at 0xFFFF (L626-630), takes the first slot whose `active_id` is 0 (L632), and records
  the new ID as `last_id_issued` (L651). It does this under the type lock.
* **[I] Generation counter.** A stale ID is rejected because its serial no longer matches the slot's `active_id`
  (`OS_ERR_INVALID_ID`, ConvertToken L460-465). The same slot gets the same ID again only after about
  65535/`OS_MAX_<type>` allocations of that type: ~1023 for tasks and queues, ~3276 for semaphores. ABA on IDs is
  therefore practically impossible within one boot.
* **[I]** IDs depend on the global allocation order of each type. **IDs are not stable identities across runs**: a
  detector must correlate resources across runs by name (plus creator and type), not by ID. cFE task IDs are derived
  from OSAL IDs (`CFE_ES_TaskId_FromOSAL`, cfe_es_apps.c L681), so they inherit this.

### 1.3 Record states, refcount and lock modes

**[F]** The lock modes (os-shared-idmap.h L67-75; logic in `OS_ObjectIdConvertToken`, osapi-idmap.c L388-530):

| Mode | Table lock while the caller holds the token | Effect on record | Used by (examples) |
|---|---|---|---|
| `NONE` | never taken | lock-free `active_id == id` check only (L424 comment: "a single integer, we can do this check regardless") | Give/Take/Flush/TimedWait of semaphores and mutexes, QueueGet/Put, CondVar ops (osapi-binsem.c L155/176/197/218, osapi-queue.c L162/200) |
| `GLOBAL` | held until Release | none | GetInfo, by-name lookup (L988), TaskPrepare (osapi-task.c L84), TimerSet (osapi-time.c L330), DirectoryRead (osapi-dir.c L177), console write (osapi-printf.c L201) |
| `REFCOUNT` | released after `++refcount` (L500, L513-516) | refcount pins the object against EXCLUSIVE | file read/write/seek (osapi-file.c L194, L231, L331), socket recv/send/accept, SelectSingle (osapi-select.c L65), timer→timebase reference (osapi-time.c L130-131) |
| `EXCLUSIVE` | released after the ID is set to RESERVED and refcount is 0 (L435-449) | `active_id := RESERVED` | all Delete/close paths, and AllocateNew for creation (L1171, L1214) |
| `RESERVED` | held | requires `active_id == RESERVED` | only `osal/src/os/vxworks/src/os-impl-timebase.c` L211 (not POSIX) |

**[F] Contention handling.** If the record is RESERVED (for any mode except NONE), or the refcount is nonzero for
EXCLUSIVE, the caller waits in `OS_WaitForStateChange` (L485). That function clears and restores `owner_key`
(L802-819) and calls `pthread_cond_timedwait` on the type's condvar (os-impl-idmap.c L169). The timeout is
`attempts² × 10 ms` on `CLOCK_REALTIME` (L151-167). On the 5th attempt the caller fails with
`OS_ERR_OBJECT_IN_USE` (osapi-idmap.c L475-480). **[F]** Every `OS_Unlock_Global_Impl` broadcasts that condvar
(os-impl-idmap.c L116), so any unlock of the same type wakes waiters and costs them an attempt.

### 1.4 Per-type table locks (POSIX)

* **[F]** There is one `{pthread_mutex_t, pthread_cond_t}` per type (os-impl-idmap.c L37-66). Each is initialized
  with `PTHREAD_PRIO_INHERIT` (L210) and **`PTHREAD_MUTEX_NORMAL`** (L222; comment: "There should not be any
  instances of OSAL locking its own table more than once").
* **[F]** POSIX: for a NORMAL mutex, "deadlock detection shall not be provided. Attempting to relock the mutex causes
  deadlock" (Open Group, `pthread_mutex_lock`). Error returns from lock and unlock are only printed with `OS_DEBUG`
  (os-impl-idmap.c L92-96, L123-127).
* **[F]** Ownership is tracked only for diagnostics: `owner_key` is the task ID XOR'd with `transaction_count`
  (osapi-idmap.c L712-728). A mismatch produces a debug message, not an error (L717-724, L758-766).
* **[F] The POSIX lock table has no entry for `OS_OBJECT_TYPE_OS_RWLOCK`** (os-impl-idmap.c L51-66), although rwlock
  support is compiled (`OSAL_CONFIG_RWLOCK TRUE`). `OS_Lock_Global_Impl` silently does nothing when the entry is
  NULL (L88-97). `OS_WaitForStateChange_Impl` dereferences the entry without a NULL check (L142-169). **[I]** So
  rwlock object management runs without a table lock and would crash on contention. **[F]** No cFE, app or PSP code
  outside tests calls `OS_RwLock*` (grep), so this is latent (Finding F6).

### 1.5 Creation path and its state timeline

**[F]** The pattern is the same for every type. Example `OS_TaskCreate` (osapi-task.c L164-211):

| Step | Code | Table lock | `active_id` | `name_entry` | Findable by name? | Findable by ID? |
|---|---|---|---|---|---|---|
| a | `OS_ObjectIdAllocateNew` → `TransactionInit(EXCLUSIVE)` locks (osapi-idmap.c L1171, L329-332) | held | 0 | — | — | — |
| b | duplicate-name check `FindNextMatch` (L1181-1193); `FindNextFree` sets `active_id = new id`, `name_entry = NULL`, `creator = OS_TaskGetId()`, `refcount = 0` (L645-648) | held | new id | NULL | no | no other thread can look, because the lock is held |
| c | `OS_NotifyEvent(OS_EVENT_RESOURCE_ALLOCATED)` (L1209) | **held** | new id | NULL | — | — |
| d | `ConvertToken(EXCLUSIVE)`: `active_id := RESERVED`, `++refcount`, **unlock** (L435-439, L500, L513-516) | released | RESERVED | NULL | not found (`OS_ERR_NAME_NOT_FOUND`) | `OS_ERR_INVALID_ID` (NONE mode) or bounded wait then `OS_ERR_OBJECT_IN_USE` |
| e | `OS_OBJECT_INIT` writes the type record and then `name_entry` (os-shared-idmap.h L106-111; osapi-task.c L193) | **not held** | RESERVED | set | **matches, then F1** (Section 1.8) | as d |
| f | `OS_TaskCreate_Impl` (pthread_create, or sem_init, mq_open, ...) (osapi-task.c L204) | not held | RESERVED | set | as e | as d |
| g | `OS_ObjectIdFinalizeNew` → `TransactionFinish`: relock, `--refcount`, `active_id := final` (L1072-1093), unlock (L1105) | held, then released | final id | set | yes | yes |
| h | `OS_NotifyEvent(OS_EVENT_RESOURCE_CREATED)` (L864); `*outid` written (L870) | not held | final id | set | yes | yes |

**[F] The source comments say the table stays locked during initialization, but the code releases it.**
osapi-idmap.c L1144-1145 says "The global table is remains in a locked state if this returns OS_SUCCESS".
osapi-task.c L186 says "the common ObjectIdAllocate routine will lock the object type and leave it locked".
os-impl-timebase.c L343-344 says "the thread will not actually start running until this function exits and
releases the global table lock", and L377-379 says "This is all done while the global lock is held". The code
releases the lock in step d. The unit test agrees with the code: "Global should be released"
(`osal/src/unit-test-coverage/shared/src/coveragetest-idmap.c` L309-322). **[I]** Any reasoning (human or tool)
built on those comments is wrong: initialization is protected only by the RESERVED state, not by mutual exclusion
(Finding F3).

### 1.6 Deletion path

* **[F]** `GetById(EXCLUSIVE)` sets `active_id := RESERVED` under the lock, waits for `refcount == 0` with the
  bounded retry, then unlocks (L429-449, L513-516). Next comes `*_Delete_Impl`. Then `OS_ObjectIdFinalizeDelete`
  relocks, sets `active_id := 0` on success (or restores the ID on failure), and unlocks (L881-897, L1090-1105).
  `OS_EVENT_RESOURCE_DELETED` is sent outside the lock (L900-903).
* **[F]** `name_entry` is not cleared on delete. The record stops matching only because `active_id` becomes 0
  (`FindNextMatch` requires `OS_ObjectIdDefined`, L567).
* **[F]** NONE-mode operations take no refcount, so EXCLUSIVE does not wait for them. **[I]** A `Give` or `Take` that
  passed the lock-free ID check just before the deleter set RESERVED runs concurrently with `*_Delete_Impl`, which
  destroys the pthread or POSIX object (e.g. os-impl-binsem.c L255-273). That is use-after-destroy undefined
  behavior. A task blocked in `mq_receive` while the queue is `mq_close`d (os-impl-queues.c L169) is the same case
  (Finding F4).

### 1.7 Lookups by state

| Lookup | Entry | Free slot | RESERVED (creating or deleting) | Active |
|---|---|---|---|---|
| By ID, NONE | `GetById` (L1013-1045) | `OS_ERR_INVALID_ID` | `OS_ERR_INVALID_ID` immediately (L460-465) | success, no lock |
| By ID, GLOBAL, REFCOUNT or EXCLUSIVE | same | `OS_ERR_INVALID_ID` | bounded wait (≤ ~300 ms), then success or `OS_ERR_OBJECT_IN_USE`; the lock is released on failure (L1039-1042) | success |
| By name | `OS_ObjectIdFindByName` → `GetByName(GLOBAL)` → `GetBySearch` (L975-997, L961-964, L920-947) | `OS_ERR_NAME_NOT_FOUND` | if `name_entry` is still NULL (steps b-d): `OS_ERR_NAME_NOT_FOUND`; if set (steps e-f, or during delete): `OS_ERR_INCORRECT_OBJ_STATE` **and the lock stays held** (F1) | success; the lock is released by `OS_ObjectIdRelease` (L993) |
| By custom search | `GetBySearch` (e.g. `OS_TaskFindIdBySystemData`, osapi-task.c L448-452; `OS_TranslatePath`, osapi-filesys.c L735) | not found | same as by name if the match function matches | success |
| Iterator (`OS_ObjectIdIterateActive`) | L1272-1275, filter L1263-1266 | skipped | **included** (filter is `Defined`) | included |
| `OS_ForEachObject` | L1360-1397, filter L221-232 | skipped | **excluded** (filter is `IsValid`) | included; the table is unlocked during each callback (L1329-1331) |

**[F]** The unit test checks that a mid-creation object is not found by name, but only with `OS_LOCK_MODE_NONE`,
where no lock exists to leak (coveragetest-idmap.c L785-792).

### 1.8 Findings in the ID map (code reading, not run)

**F1 — Lock leak on name or search lookup of a RESERVED record. [I], high confidence from code; needs the
Section 7 experiment.**

The sequence of facts:

1. `GetBySearch(GLOBAL)` locks the type (L928 → L329-332).
2. `FindNextMatch` accepts a record with `active_id == RESERVED` and `name_entry` set (L567) and copies
   `token->obj_id = RESERVED` (L570).
3. `ConvertToken` returns `OS_ERR_INCORRECT_OBJ_STATE` at L401-404, **before** the unlock logic at L494-527. Its
   header comment says "Upon failure, the global table lock is always released" (L384-385).
4. `GetBySearch` calls `TransactionCancel` only on the not-found branch (L941-944).
5. `OS_ObjectIdFindByName` releases only on success (L989-994).

Result: the caller returns with the type's NORMAL mutex held and `owner_key` set. The creator (step g) or deleter
then blocks forever in `OS_Lock_Global` → `pthread_mutex_lock`. Every other thread using a GLOBAL, REFCOUNT or
EXCLUSIVE operation on that type blocks too, and the leaking thread deadlocks itself on its next lookup of that type.

Exposed public APIs: all `OS_*GetIdByName` (task, queue, binsem, countsem, mutex, condvar, timebase, timer,
socket), `OS_TaskFindIdBySystemData`, and the filesys searches (`OS_TranslatePath`, `OS_unmount`, `OS_chkfs`, ...).

Window: for creation, from `OS_OBJECT_INIT` to the relock in FinalizeNew. For task creation this includes
`pthread_create`. For deletion, from EXCLUSIVE to FinalizeDelete. For `OS_TaskDelete` this includes
`pthread_cancel` and `pthread_join` (os-impl-tasks.c L693-723), which can last as long as the victim takes to
reach a cancellation point.

cFS call sites that can race with a creation or deletion of the same name: `CFE_ES_GetTaskIDByName`
(cfe_es_api.c L717), `CFE_SB_GetPipeIdByName` (cfe_sb_api.c L802), CF's semaphore lookup (apps/cf/fsw/src/cf_cfdp.c
L1282; its retry loop stops on any code other than NAME_NOT_FOUND, L1285-1288), and the PSP exception summary
(psp/fsw/shared/src/cfe_psp_exceptionstorage.c L173).

Upstream search: a semantic search of nasa/osal issues ("GetIdByName … lock not released", "GetBySearch deadlock …
reserved") returned no matching issue (searched 2026-10-12). [U] whether a later OSAL release changed this.

**F2 — Duplicate names under concurrent creation. [I]** The uniqueness check (L1181-1193) runs under the lock, but
the new record's `name_entry` stays NULL until `OS_OBJECT_INIT` runs after the unlock (L646, step e). Two concurrent
creations with the same name can therefore both pass the check. Later by-name lookups return the lower slot.
For queues the kernel name `/<pid>.<name>` (os-impl-queues.c L111) is opened with `O_CREAT` without `O_EXCL`
(L116) and unlinked right away (L142). Depending on timing, the two OSAL queues share one kernel queue or get
separate ones. Low practical relevance in cFS, where names are per app, but OSAL name uniqueness is not
linearizable.

**F3 — Comments contradict code about locking during creation.** See Section 1.5.

**F4 — NONE-mode operations against concurrent deletion.** See Section 1.6. Also, **[F]** `OS_SelectMultiple`
takes no refcount on its descriptors (osapi-select.c L103-107: "a file/socket can be closed while actively inside
a OS_SelectMultiple() call") and reads the impl table without a lock (os-impl-bsd-select.c L90-96).

**F5 — Shutdown-state searches run unlocked. [F]** `GetBySearch` ignores the return of `TransactionInit` (L928). In
shutdown state that function returns before locking (L311-314), and `memset` leaves `obj_idx = 0` (L296), so the
search starts at index 1. Relevant only during teardown.

**F6 — rwlock type has no POSIX table lock.** See Section 1.4. Latent: no non-test users.

---

## 2. Task lifecycle

### 2.1 Creation chain (facts)

`OS_TaskCreate` (osapi-task.c L164)
→ `OS_ObjectIdAllocateNew` (L187)
→ `OS_OBJECT_INIT` and field setup (L193-198)
→ `OS_TaskCreate_Impl` (os-impl-tasks.c L596-625). If priorities are disabled this first writes
`task->priority = OS_MAX_TASK_PRIORITY` (L612-615).
→ `OS_Posix_InternalTaskCreate_Impl` (L457-588): attributes, stack +`PTHREAD_STACK_MIN` (L494-499), joinable
(L511), explicit RT policy and priority **only if** `EnableTaskPriorities` (L523-563)
→ `pthread_create(&impl->id, …, OS_PthreadTaskEntry, id)` (L568)
→ back in shared: `OS_ObjectIdFinalizeNew` (osapi-task.c L207).

### 2.2 When the new thread runs relative to the creator

* **[F]** The child runs `OS_PthreadTaskEntry` (os-impl-tasks.c L131-140) → `OS_TaskEntryPoint`
  (osapi-task.c L124-138) → `OS_TaskPrepare` (L78-112). `OS_TaskPrepare` calls `GetById(GLOBAL)` (L84),
  `OS_TaskMatch_Impl` (compares `pthread_self()` with `impl->id`, os-impl-tasks.c L659-671), reads the entry pointer
  (L90), calls `OS_TaskRegister_Impl` (L97) and then `OS_EVENT_TASK_STARTUP` (L103), and finally calls the user
  entry (L132).
* **[F]** While the creator is in steps d-f, the record is RESERVED. The child's `GetById(GLOBAL)` takes the bounded
  wait path (osapi-idmap.c L460-485).
* **[I] HB edges at task start:**
  1. `pthread_create`: creator's actions before the call → the child's first action (POSIX memory
     synchronization).
  2. The creator's `TransactionFinish` unlock (L1105) → the child's successful lock in `OS_TaskPrepare`. Everything
     the creator did inside `OS_TaskCreate`, including the `impl->id` write by `pthread_create`, is therefore
     ordered before the child's entry function.
  3. **Nothing** the creator does after `OS_TaskCreate` returns is ordered before the child without further
     synchronization.
* **F7 — Silent task loss. [I]** If the child's wait reaches 5 attempts, `OS_TaskPrepare` returns an error and
  `*entrypt = NULL` (L106-109). Attempts are used up by each 10/40/90/160 ms timeout **and** by every broadcast from
  any unlock of the task table, e.g. another new task's PSP startup hook calling `OS_GetResourceName`
  (cfe_psp_start.c L174). `OS_TaskExit` then runs. The thread was never registered, so `OS_TaskGetId_Impl` returns 0
  (os-impl-tasks.c L858-866), `GetById` fails, and the thread exits (osapi-task.c L263-273). The OSAL record stays
  active (a zombie): the creator got `OS_SUCCESS` and the entry point never runs. This needs the creator to be
  delayed between `pthread_create` and FinalizeNew, which is plausible only under heavy host load
  (**[E]** the host is shared, CONDITIONS B3). Not observed.
* **[F]** The timebase thread has the same handshake (osapi-timebase.c L386-390). It silently `return`s on failure,
  which would leave a timebase with no servicing thread.

### 2.3 Registration, self-identification and startup hook

* **[F]** Registration is `pthread_setspecific(ThreadKey, id)` plus cancel state ENABLE/DEFERRED
  (os-impl-tasks.c L817-850). `OS_TaskGetId` reads the key (L858-866; osapi-task.c L328-335).
  `OS_TaskRegister` is not a public API at this revision (grep of `osal/src/os/inc` finds nothing).
* **[F] Context IDs that are not task IDs:**
  * The root (main) thread, the console thread and other raw pthreads return 0 (comment at osapi-idmap.c
    L698-703).
  * The timebase callback thread registers **the timebase ID** (osapi-timebase.c L383). Timer callbacks therefore
    see `OS_TaskGetId()` with type `TIMEBASE`. The timer APIs use this to refuse calls from callbacks
    (osapi-time.c L119-123, L324-328; osapi-timebase.c L115-119).
  * **[F]** The `creator` stamped on objects created from such contexts (osapi-idmap.c L647) is 0 or a timebase ID.
    `OS_ForEachObject` cleanup by creator (used by ES, cfe_es_apps.c L1759) relies on this attribution.
* **[F]** The PSP registers one global OSAL event handler (cfe_psp_start.c L361; single slot, osapi-common.c
  L274-280). On `OS_EVENT_TASK_STARTUP` it reads the task name with `OS_GetResourceName` (task-table GLOBAL lock),
  **pins names starting with `CFE_` to CPU 0** with `pthread_setaffinity_np` (L184-189), and sets the kernel thread
  name truncated to 15 characters (L197-201).

### 2.4 Delete, exit and delete hooks

* **[F]** `OS_TaskDelete` (osapi-task.c L219-250): EXCLUSIVE (RESERVED) → `pthread_cancel` + `pthread_join`
  (os-impl-tasks.c L693-723) → FinalizeDelete → delete hook called **after** unlock and only on success (L244-247).
  Cancellation is deferred (L830-831), so the deleter blocks until the victim reaches a cancellation point. A victim
  in a busy loop blocks the deleter (e.g. ES) indefinitely.
* **[F]** Cleanup handlers release only OSAL's internal mutexes in condvar waits (binsem L383/L435; table wait
  os-impl-idmap.c L149/L171; condvar L245-247). **[I]** A task cancelled at a cancellation point while holding any
  other lock (an OSAL mutex taken via `OS_MutSemTake`, the ES shared-data lock, a table lock held across blocking
  I/O such as `OS_DirectoryRead` at osapi-dir.c L177) leaves that lock held forever.
* **[F]** `OS_TaskExit` (L258-276) detaches (os-impl-tasks.c L633-651) and frees the record **before**
  `pthread_exit`. The delete hook is not called on exit.

### 2.5 Priorities and the permissive fallback

* **[F]** At init OSAL picks SCHED_FIFO or SCHED_RR if either has a spread of at least 4 (os-impl-tasks.c L332-382)
  and tries `pthread_setschedparam` on the main thread (L409). Only on success does it set
  `EnableTaskPriorities = true` (L420-421). Without `OSAL_CONFIG_DEBUG_PERMISSIVE_MODE`, failure is fatal
  (L429-439). With it (this build), tasks inherit the default policy and the recorded priority becomes 255 (L612-615).
* **[E]** `pthread_setschedparam` fails with EPERM (`RLIMIT_RTPRIO` 0, no `CAP_SYS_NICE`). All 27 threads run
  `SCHED_OTHER`, nice 0 (CONDITIONS E4, M1-5).
* **[I]** Any cFS ordering that relies on priority preemption ("ES runs before apps") is not enforced here. Only
  explicit synchronization and CPU-0 pinning of `CFE_*` tasks (if it takes effect) shape interleavings. The detector
  must not take priority-based ordering as given.

### 2.6 Thread inventory at OSAL and PSP level

| Thread | Created by | OSAL task record? | `OS_TaskGetId()` |
|---|---|---|---|
| root/main (PSP "idle task") | process start (osal/src/bsp/generic-linux/src/bsp_start.c L196-248) | no | 0 |
| console writer | `OS_ConsoleCreate_Impl` → `OS_Posix_InternalTaskCreate_Impl` (os-impl-console.c L131); waits in `sem_wait` (L93) | no | 0 |
| timebase "cFS-Master" | `OS_TimeBaseCreate_Impl` (os-impl-timebase.c L350-355) | no (timebase record) | timebase ID |
| OSAL tasks (cFE core and apps, child tasks) | `OS_TaskCreate` | yes | task ID |
| linux_sysmon worker | `pthread_create` (psp/fsw/modules/linux_sysmon/linux_sysmon.c L329), started by HS through IODriver (apps/hs/fsw/src/hs_sysmon.c L66) | no | 0 |

**[E]** `/proc/schedstat` is absent on this kernel (CONDITIONS B3). **[F]** So `linux_sysmon_Start` fails at the
`open` (linux_sysmon.c L321) and no worker thread starts here.

### 2.7 Boundary with cFE ES (summary; details belong to the cFE report)

**[F]** `CFE_ES_StartAppTask` fills the ES task record under the ES lock **after** `OS_TaskCreate` returns
(cfe_es_apps.c L664-722). The child, in `CFE_ES_TaskEntryPoint` → `CFE_ES_GetTaskFunction`, polls every
`CFE_PLATFORM_ES_STARTUP_SYNC_POLL_MSEC` under the ES lock until `AppId` and `EntryFunc` are set, or the
startup-script timeout expires (L573-596). **[I]** cFS therefore has a two-level start handshake: OSAL's bounded
RESERVED wait, then ES's polling. A detector has to model both. The ES-level edge is an HB edge only through the ES
mutex, i.e. through the observed poll that succeeded.

---

## 3. Synchronization and communication primitives

Lamport defines HB for message passing as send → receive. For shared-memory synchronization I use the standard
analog **release → acquire** (as in vector-clock detectors such as FastTrack and ThreadSanitizer; references in
Section 8).

**[I] Two HB levels.** The *implementation-level* HB (what the POSIX code actually synchronizes) is a superset of
the *API-level* HB (what the OSAL contract promises on every OS backend). Two examples:

* Every operation on a binary semaphore takes the semaphore's internal mutex. At implementation level, even a
  timed-out take or an unconsumed give is ordered.
* `OS_printf` serializes all callers through the console table lock.

A detector that uses implementation-level edges hides races that would appear on other OSAL backends or under other
timings, the classic masking problem of HB detectors. **Recommendation [I]: use API-level edges only**, and treat
OSAL-internal locks as non-synchronizing.

**[F] Non-test use in cFE, apps and PSP** (grep): mutex ~42 take/give sites, countsem (TIME, SCH_LAB, FM, CF),
binsem (a few), queues (all are SB pipes: **[E]** BASELINE_MEASURE: "26 POSIX queues per run, all SB pipes"),
timers and timebase (TIME, SCH_LAB, PSP), sockets (CI_LAB, TO_LAB, SBN), select (SBN), files and dirs (widely),
modules and symbols (ES, MM, MD, SBN). Condvar and rwlock: no non-test users.

| Primitive | API → impl (lines) | Blocking and ordering semantics [F] | API-level HB edge [I] | What a detector must record [I] |
|---|---|---|---|---|
| **Mutex** (`OS_MutSem*`) | osapi-mutex.c Give L146-175, Take L183-204; os-impl-mutex.c: `PTHREAD_PRIO_INHERIT` L82, **`PTHREAD_MUTEX_RECURSIVE`** L94, unlock L158, lock L183 | blocks until free; recursive; owner tracking is a debug warning only (L161-169) | unlock(m) → next lock(m); only the outermost unlock of a recursive hold is a release | task, mutex ID, op, recursion depth, result |
| **Binary semaphore** | osapi-binsem.c Give L149 (NONE L155), Flush L170, Take L191, TimedWait L212; os-impl-binsem.c Give L285-320, Flush L328-352, Take L362-438 | value saturates at 1 (**gives coalesce**, L312); give signals one waiter (L315, wake order unspecified); take blocks while value is 0 (L409-422) and sets value to 0 (L425-428); **flush** wakes all current waiters, who return `OS_SUCCESS` **without consuming** (L344-347, L425); initial value above 1 becomes 1 (L133-136); acquiring the internal mutex is a 2 s `timedlock` → `OS_SEM_FAILURE` (L50, L60-77); timeouts use `CLOCK_REALTIME` (os-impl-common.c L147-161); not usable from signal handlers (L92-97) | give → the take that consumes it; conservatively, the join of all gives since the last consuming take; flush → each flushed take; timeout → none | op, result, **consumed vs flushed (indistinguishable to the caller)**, number of gives coalesced |
| **Counting semaphore** | osapi-countsem.c Give L141, Take L162, TimedWait L183; os-impl-countsem.c `sem_post` L125, `sem_wait` L145, `sem_timedwait` L172 | counts accumulate (no coalescing); no FIFO among waiters; `sem_wait` interrupted by a signal returns `OS_SEM_FAILURE` with no retry (L145-148) | post → wait, conservatively the join of all prior posts (as ThreadSanitizer models `sem_post`/`sem_wait`) | op, result, count if needed (`sem_getvalue` L202) |
| **Condition variable** | osapi-condvar.c L138-262; os-impl-condvar.c lock L173, unlock L151, signal L195, broadcast L217, wait L246, timedwait L275 | built-in mutex plus pthread condvar; spurious wakeups possible; signal wakes one, broadcast wakes all | through its mutex only: unlock → lock, including the unlock and relock inside wait | (unused in cFS) |
| **Queue** | osapi-queue.c Create L84-120 (depth ≤ 50 checked L98; requested depth stored L109), Put L190-219, Get L151-182; os-impl-queues.c Create L80-153, Put L285-323, Get L188-277 | Put **never blocks**: `mq_timedsend(…, prio 1, zero timeout)` (L300-305) and a full queue returns `OS_QUEUE_FULL` (L312-315), dropping the message; Get modes are `OS_PEND` (EINTR loop L207-210), `OS_CHECK` and a millisecond timeout (`CLOCK_REALTIME`, L223-239); FIFO, since all messages have one priority; copy semantics; Get requires a buffer ≥ `max_size` (L167-174); depth is truncated to the BSP limit in permissive non-root mode (L59-63, L102-105; bsp_start.c L66-78) | put(m) → get(m) for the same message (send → receive); no edge for `OS_QUEUE_FULL` | message identity (SB passes a buffer-descriptor pointer, **[E]** `mq_msgsize` 8), drops, depth; **[E]** depth is min(requested, 10) here (CONDITIONS E5) |
| **Timebase and timer** | osapi-timebase.c Create L93-148, Set L156-207, CallbackThread L365-510; osapi-time.c DoTimerAdd L90-196, TimerSet L306-368, TimerDelete L376-; os-impl-timebase.c handler mutex L105-127, SigWait L134-180, Create L321-482, Set L490-537, Delete L545-570 | one thread per timebase; POSIX timer on `CLOCK_MONOTONIC` → RT signal → `sigwait` (L150, L458); each tick takes `handler_mutex` (L448), checks that the timebase still exists (L454-458), walks the callback ring and calls callbacks **with `handler_mutex` held** (L461-505); callbacks with an interval shorter than the tick are clipped (`backlog_resets`, L477-481); TimerAdd, TimerSet and TimerDelete take `handler_mutex` (osapi-time.c L164-185, L335-345); timebase delete cancels the thread **without join** (os-impl-timebase.c L552) | TimerAdd or TimerSet (app task) → later callbacks (through `handler_mutex`); callback k → callback k+1 on the same timebase (one thread); typical chain: tick → callback → `OS_CountSemGive` → app task (apps/sch_lab/fsw/src/sch_lab_app.c L143-146; cfe/modules/time/fsw/src/cfe_time_tone.c L952-955) | callback start and end in the timebase thread, attributed to the timer's creator (record `creator`); **[I]** lost ticks: OSAL never calls `timer_getoverrun`, so overruns while the thread is delayed are not counted ([U] POSIX one-signal-per-timer rule not checked against the spec) |
| **Select** | osapi-select.c SelectSingle L57-80 (REFCOUNT), SelectMultiple L93-111 (no refcount); os-impl-bsd-select.c `select` L211 with EINTR/EAGAIN retry L213 | readiness only; fd sets are indexed by slot, not full ID | none by itself | readiness result; the stale-index risk is F4 |
| **Sockets** | osapi-sockets.c Open L119 (allocated without a name), Accept L284-369, RecvFrom L492- (REFCOUNT L510), SendTo L552- (REFCOUNT L563) | BSD sockets; reads with a timeout use select when the handle is selectable (os-impl-posix-io.c L181-190) | datagram send → receive, possibly across processes (ground tools ↔ CI_LAB, TO_LAB, SBN peers) | payload identity; treat external senders as an environment thread |
| **Files, dirs, filesys** | osapi-file.c OpenCreate L107-153 (path translation L131, stream allocated without a name L136 but `name_entry` set to the path by `OS_OBJECT_INIT` L142), close EXCLUSIVE L167, read/write REFCOUNT L194/L231, remove L396-409 (no open-file check), rename L416-453 (renames open streams), FileOpenCheck L587-613; osapi-filesys.c TranslatePath `GetBySearch` L735; osapi-dir.c DirectoryRead GLOBAL L177 | no synchronization semantics; persistent shared state (`/cf`, `/ram` → `/dev/shm/osal:RAM`, os-impl-filesys.c L134-136, L181-190); `OS_FileOpenCheck` is check-then-act (used by CF, apps/cf/fsw/src/cf_cfdp_s.c L451); the iterator includes RESERVED streams (Section 1.7) | none (data flows through the FS; model paths as shared locations) | path, op, offset or size where relevant, success |
| **Modules and symbols** | osapi-module.c Load L185-266, Unload EXCLUSIVE L282, SymbolLookup L345, ModuleSymbolLookup GLOBAL L401 | `dlopen`/`dlsym`; a symbol lookup succeeds only after the defining module is loaded | load(lib) → lookup(sym) succeeds; publish then discover (Section 4) | module and symbol names, results |
| **Console / `OS_printf`** | osapi-printf.c ConsoleWrite L194-250 (GLOBAL L201, wakeup inside the lock L243), `OS_printf` L263-320 | serializes all callers; ring buffer overflow increments `OverflowEvents` (L230) | **none at API level** (implementation-level edge only; must be ignored) | — |
| **`OS_TaskDelay`** | os-impl-tasks.c L744-772 (`clock_nanosleep` `CLOCK_MONOTONIC` L761) | timing only | none | delay calls, because timing-based "synchronization" is a race signature |

---

## 4. Name-based lookup and other "publish then discover" patterns

**[F] Public name-lookup APIs** all go through `OS_ObjectIdFindByName` (GLOBAL):
`OS_TaskGetIdByName` (osapi-task.c L343-354), `OS_QueueGetIdByName` (osapi-queue.c L227-238),
`OS_BinSemGetIdByName` (osapi-binsem.c L233-244), `OS_CountSemGetIdByName` (osapi-countsem.c L204-215),
`OS_MutSemGetIdByName` (osapi-mutex.c L212-223), `OS_CondVarGetIdByName` (osapi-condvar.c L267-278),
`OS_TimeBaseGetIdByName` (osapi-timebase.c L249-271), `OS_SocketGetIdByName` (osapi-sockets.c L589-599), plus
timer get-by-name. Search-based: `OS_TaskFindIdBySystemData` (osapi-task.c L433-461) and filesystem path
resolution (`OS_TranslatePath` and others in osapi-filesys.c). Iterator-based: `OS_FileOpenCheck`,
`OS_CloseFileByName`, `OS_ForEachObject`.

**[F] Semantics by state:** see the table in Section 1.7. The outcome of a lookup is one of:

* found: the creator's finalize happened before the lookup (HB through the type lock);
* `OS_ERR_NAME_NOT_FOUND`: the lookup came before the name was published, or after the delete finished;
* `OS_ERR_INCORRECT_OBJ_STATE`: mid-create or mid-delete, together with F1.

**[F] Publish/discover pairs in this tree** (non-test code):

| Resource | Publisher | Discoverer | How the order is ensured [F/I] |
|---|---|---|---|
| timebase `cFS-Master` | PSP `soft_timebase_Init` (psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase.c L53), name from `cfe_psp_timertick_api.h` L53 | `CFE_TIME` (cfe/modules/time/fsw/src/cfe_time_task.c L311), SCH_LAB (sch_lab_app.c L176) | [F] created on the main thread in `CFE_PSP_ModuleInit` (cfe_psp_start.c L391) before `CFE_PSP_MAIN_FUNCTION` (L449). [I] Ordered by program order and `pthread_create`; fixed order, not a race unless creation fails |
| CF throttle semaphore (name from the CF table, "throttling semaphore in TO", apps/cf/config/default_cf_tbldefs.h L76) | another app (by design) | CF init (cf_cfdp.c L1270-1290) | [F] comment: "There is a start up race condition because CFE starts all apps at the same time … delay a bit and try again", with bounded retries. [F] Default table: empty name, no lookup (apps/cf/fsw/tables/cf_def_config.c L60, L78). [I] A classic order violation guarded by retries; the retry stops on the F1 code |
| SB pipe by name | `CFE_SB_CreatePipe` (`OS_QueueCreate`) | `CFE_SB_GetPipeIdByName` (cfe_sb_api.c L802) | [I] app-dependent; no OSAL-level ordering |
| task by name | ES / `OS_TaskCreate` | `CFE_ES_GetTaskIDByName` (cfe_es_api.c L717) | [I] app-dependent |
| crashed thread → task ID | OSAL task record (`impl->id`) | PSP `CFE_PSP_Exception_GetSummary` (cfe_psp_exceptionstorage.c L173) | [I] runs after an exception, possibly while ES deletes the task (F1 window) |
| symbols | `OS_ModuleLoad` (ES, startup-script order) | `OS_SymbolLookup` / `OS_ModuleSymbolLookup` (cfe_es_apps.c L522; MM, MD, SBN) | [I] ES loads sequentially, so the order is fixed by the script; SBN loads modules lazily (apps/sbn/fsw/src/sbn_app.c L1065-1082) |
| open-file state | any task's `OS_OpenCreate` / `OS_close` | `OS_FileOpenCheck` (CF), `OS_ForEachObject` (FM, apps/fm/fsw/src/fm_cmd_utils.c L103, L180) | [I] check-then-act; the snapshot can be stale immediately |

**[I] The pattern most often missed: ID hand-off through a shared variable.** A NONE-mode user (Give, Take, Put,
Get) needs an ID. If that ID reaches it through a global written by the creator after `*Create` returns (e.g.
`SCH_LAB_Global.TimingSem`, sch_lab_app.c L168), the publication edge is application-level. With no synchronization
the consumer sees ID 0 and gets `OS_ERR_INVALID_ID`, because type 0 has max 0 (osapi-idmap.c L1481-1491). For
SCH_LAB the hand-off to the callback is ordered by `OS_TimerAdd` and `handler_mutex`. The detector should treat
"first use of an ID variable" as a discover event and look for a matching publish edge.

---

## 5. PSP (pc-linux)

### 5.1 Startup sequence (facts, in order)

1. `main` (osal/src/bsp/generic-linux/src/bsp_start.c L196) → `OS_BSP_Initialize`. As non-root it reads
   `/proc/sys/fs/mqueue/msg_max` into `MaxQueueDepth` (L66-78) → `OS_Application_Startup` (L237).
2. Parse `-R/-S/-C/-I/-N` (cfe_psp_start.c L264-328) and apply defaults (L334, L569-599).
3. `OS_API_Init` (L352): initializes all ID tables and per-type impl state (osapi-common.c L103-246), installs the
   signal masks (os-impl-tasks.c L262-312), selects the scheduler (L326-439) and creates the console thread.
4. `OS_RegisterEventHandler(CFE_PSP_OS_EventHandler)` (L361).
5. `CFE_PSP_SetupReservedMemoryMap` (L366): creates the key files and attaches the shm segments
   (cfe_psp_memory.c L603-636).
6. Record the idle-thread ID (L371-372).
7. `OS_FileSysAddFixedMap("./cf", "/cf")` (L378).
8. `CFE_PSP_ModuleInit` (L391; psp/fsw/shared/src/cfe_psp_module.c L122-129) initializes the fixed module list
   (psp/fsw/pc-linux/psp_module_list.cmake): `soft_timebase` (creates and starts `cFS-Master` at 10 ms,
   cfe_psp_config.h L73), `timebase_posix_clock`, `eeprom_mmap_file`, `port_notimpl`, `iodriver`, `linux_sysmon`,
   `endian_api`.
9. `sleep(1)` (L393). The timebase is already ticking.
10. Read the boot record and decide the reset type (L398-434).
11. `CFE_PSP_InitProcessorReservedMemory(reset_type)` (L439).
12. `CFE_PSP_MAIN_FUNCTION` = `CFE_ES_Main` (L449), which creates all cFE and app tasks and returns.
13. `OS_Application_Run` (L452-508): unblocks SIGQUIT and SIGTERM, then loops on `sigwait(SIGUSR1)` and calls
    `GLOBAL_CFE_CONFIGDATA.SystemNotify()` (L488-499). On `ShutdownReq` it calls `OS_DeleteAllObjects` (L507).

### 5.2 Reset types and the boot record

* **[F]** Types: `CFE_PSP_RST_TYPE_PROCESSOR` 1 ("Volatile disk, CDS and User Reserved memory may be valid") and
  `POWERON` 2. Subtypes 1-9 (psp/fsw/inc/cfe_psp_watchdog_api.h L68-98).
* **[F]** The boot record `{ValidityFlag, NextResetType}` (cfe_psp_config.h L89-90) lives in the reset-area shm
  segment (cfe_psp_memory.c L369).
* **[F]** At boot, without `-R`, the reset type is `NextResetType` if the flag is VALID or INVALID (L412-419);
  otherwise POWERON (L429-433).
* **[F]** `CFE_PSP_InitProcessorReservedMemory` clears CDS, reset area, user-reserved area, boot record and
  exception storage on POWERON and sets `NextResetType = PROCESSOR` (L646-664). On PROCESSOR it sets
  `NextResetType = POWERON` (L672). It always sets `ValidityFlag = INVALID` (L687).
* **[F]** `CFE_PSP_Restart` on POWERON deletes the segments (cfe_psp_support.c L62-69). It always writes
  `NextResetType` and `ValidityFlag = VALID` (L78-79), sets `ShutdownReq` and signals the idle thread (L87-88), then
  waits 10 s and `abort()`s if it is still alive (L99-107).
* **[I]** A crash or signal stop leaves `INVALID` and the next type as written at boot: PO → PR → PO ... **[E]**
  This matches the observed PO → PR → PR → PO cycle after SIGINT/SIGTERM stops (CONDITIONS E7). cFE also enforces a
  maximum processor-reset count.

### 5.3 Persistent memory

| Area | Backing | Key | Survives | Cleared by |
|---|---|---|---|---|
| CDS | SysV shm (cfe_psp_memory.c L142-175) | `ftok(".cdskeyfile",'R')` (L65, L149) | process exit, unless POWERON restart deleted it | POWERON boot memset (L649) |
| Reset area: boot record, exception storage, ES reset area | one SysV shm, page-aligned fixed layout then reset area (L317-374) | `.resetkeyfile` (L66) | same | POWERON (L651-658) |
| User reserved | SysV shm (L447-480) | `.reservedkeyfile` (L67) | same | POWERON (L653) |
| Volatile disk | `/dev/shm/osal:RAM` (OSAL RAM volume; os-impl-filesys.c L134-136, L181-190) | — | until host reboot (**[E]** CONDITIONS M1-7, M1-8) | nothing in PSP |
| EEPROM | `EEPROM.DAT` in the CWD, `open(O_CREAT)` + `ftruncate` + `mmap(MAP_SHARED)` (psp/fsw/modules/eeprom_mmap_file/cfe_psp_eeprom_mmap_file.c L41, L55-69), 512 KiB (L159) | — | always (a file) | never by PSP |

**[F]** CDS read and write are plain `memcpy` with a bounds check and no lock (cfe_psp_memory.c L235-285;
`memcpy` at L250 and L285). Locking belongs to cFE ES.

### 5.4 Exceptions, watchdog, timebase modules

* **[F] Exceptions.** `CFE_PSP_AttachExceptions` installs handlers for SIGINT and SIGTERM (non-suspending) and later
  SIGFPE through `CFE_PSP_SetDefaultExceptionEnvironment` (suspending variant) (cfe_psp_exception.c L198-245,
  L253-267). ES calls the latter per task under the ES lock (cfe_es_apps.c L632-634).
* **[F]** The handler writes a context entry into a 4-entry ring in the reset-area shm (`NumWritten` and `NumRead`
  are plain counters; psp/fsw/shared/src/cfe_psp_exceptionstorage.c L74-119) and sends `pthread_kill(idle,
  SIGUSR1)` (cfe_psp_exception.c L109). The code admits a race between concurrent exceptions (L72-86).
* **[F]** The idle thread calls cFE's `SystemNotify` (cfe_psp_start.c L497). ES later pops entries with
  `CFE_PSP_Exception_GetSummary`, which does a search lookup (L173, F1 exposure).
* **[F]** Entries left over from the previous lifetime are discarded at attach (`CFE_PSP_Exception_Reset`, L54-59,
  called at cfe_psp_exception.c L244).
* **[F] Watchdog.** No-op on pc-linux. Enable, Disable and Service are empty; `WatchdogValue` is a plain global
  (cfe_psp_watchdog.c L61-129).
* **[F] Timebase modules.** `soft_timebase` creates the OSAL timebase `cFS-Master` with a 10 ms start and interval
  (cfe_psp_soft_timebase.c L46-83). `timebase_posix_clock` only reads `CLOCK_MONOTONIC` (L60, L81) and starts no
  thread.

### 5.5 Cross-run and cross-task ordering dependencies created by PSP

| Mechanism | Kind | Dependency [F → I] |
|---|---|---|
| Boot record (shm) | cross-run | how run *n* ended decides run *n+1*'s reset type and so whether CDS, ES reset area, ER log and syslog are preserved (Section 5.2) |
| CDS and user-reserved shm | cross-run | on PROCESSOR reset, data written in run *n* is read in run *n+1*: a write→read dependency across process lifetimes |
| `EEPROM.DAT`, `/dev/shm/osal:RAM` files, `./cf` | cross-run | always persist ([E] M1-7, M1-8: `cfe_sb_pipe.dat` from one run was visible to later runs) |
| SysV key files in the CWD | cross-run, cross-instance | **[I]** two instances started in the same CWD share the shm segments |
| Exception ring + SIGUSR1 to the idle thread | cross-task, async | handler (faulting thread) → idle thread → `SystemNotify` → ES; lock-free ring with an acknowledged race |
| `CFE_PSP_Restart` → `ShutdownReq` + SIGUSR1 → `OS_DeleteAllObjects` on the idle thread | cross-task | teardown runs concurrently with still-running tasks; the caller aborts after 10 s |
| `cFS-Master` timebase | cross-task, fixed order | created before cFE starts (Section 4) |
| Timer callbacks under `handler_mutex` | cross-task | all callbacks on the 10 ms timebase are serialized; a slow callback delays every timer |
| CPU-0 pinning of `CFE_*` tasks | scheduling | **[I]** core tasks contend for one CPU while apps float; this changes interleaving distributions. Not yet measured |
| Message queues | none across runs | **[F]** unlinked right after `mq_open` and named with the pid (os-impl-queues.c L111, L142) |

---

## 6. Implications for the detector

### 6.1 Observation points

**Resource lifecycle ("becomes usable" / "stops being usable")**

| Event | Exact point | Hook available? |
|---|---|---|
| ID allocated (name not yet visible) | osapi-idmap.c L1196 → event `OS_EVENT_RESOURCE_ALLOCATED` L1209 | yes (OSAL event), **but it runs inside the table lock** |
| name visible, object RESERVED | `OS_OBJECT_INIT` in each Create (task osapi-task.c L193; queue osapi-queue.c L107; binsem osapi-binsem.c L108; countsem osapi-countsem.c L100; mutex osapi-mutex.c L105; timebase osapi-timebase.c L128; timer osapi-time.c L145; stream osapi-file.c L142) | no; needs instrumentation |
| **usable (publish)** | osapi-idmap.c L1092 (`active_id := final`, under lock), release at L1105 | no; `OS_EVENT_RESOURCE_CREATED` (L864) comes *after* the release, so consumers may act before the event is logged ([I] a trace ordered by time would show use-before-create; the edge must be anchored at L1105) |
| delete begins | ConvertToken EXCLUSIVE sets RESERVED (L438) | no |
| delete complete | L1092 with the final ID 0; event `OS_EVENT_RESOURCE_DELETED` L902 (outside the lock) | yes (event) |
| task registered and started | `OS_TaskPrepare` after L97, event `OS_EVENT_TASK_STARTUP` L103, user entry L132 | yes (event, in task context) |
| task ended | `OS_TaskExit` L258; `OS_TaskDelete` L219 and `pthread_join` (os-impl-tasks.c L716) | no |

[F] The PSP already owns the single event-handler slot (cfe_psp_start.c L361; osapi-common.c L278). [I] A detector
using events must chain to the PSP handler.

**Use and discover**

* `OS_ObjectIdGetById` entry and result per API (osapi-idmap.c L1013). The result codes are themselves signals:
  `OS_ERR_INVALID_ID` (never published or already deleted, or ID 0), `OS_ERR_OBJECT_IN_USE` (bounded wait
  expired), `OS_ERR_INCORRECT_OBJ_STATE` (mid-state).
* `OS_ObjectIdFindByName` (L975) and `OS_ObjectIdGetBySearch` (L920) for discovery.

**Ordering edges (API level)**

| Edge | Release site | Acquire site |
|---|---|---|
| mutex | os-impl-mutex.c L158 | L183 |
| binsem give / flush | os-impl-binsem.c L312-317, L344-347 | L425-428 (consume) or flushed return |
| countsem | os-impl-countsem.c L125 | L145, L172 |
| queue | os-impl-queues.c L305 (success) | L209, L238 (success), matched by message identity |
| timer config → callback | osapi-time.c L185, L345 (unlock `handler_mutex`) | osapi-timebase.c L448 |
| callback execution | osapi-timebase.c L491 (in the timebase thread) | — |
| task create | osapi-idmap.c L1105 (in the creator's `OS_TaskCreate`) | osapi-task.c L84 (child) |
| task delete | target termination | os-impl-tasks.c L716 (join returns) |
| ES start handshake | ES lock in `CFE_ES_StartAppTask` (cfe_es_apps.c L722) | successful poll in `CFE_ES_GetTaskFunction` (L577-587) |

### 6.2 HB-model recommendations [I]

1. Model OSAL objects as **typed resources with a lifecycle**: free, then allocated, then named while RESERVED,
   then active, then RESERVED for deletion, then free. Model each process-level operation as an access to that
   resource. Order violations ("use or discover before publish", "use after delete") then become conflicts between
   lifecycle events and accesses that are not HB-ordered.
2. Use **API-level** edges (Section 3 table), not implementation-level ones. Exclude OSAL table locks, the console
   lock and the binsem internal mutex as synchronization.
3. Key objects by **(type, name, creator, creation ordinal)** across runs, and by ID only within a run (Section 1.2).
4. Treat `OS_TaskDelay` polling, bounded retries (OSAL's 5 attempts, CF's retry loop, ES polling) and
   priority-based assumptions as **timing-dependent orderings**, not HB.
5. Treat the timebase thread as its own thread whose callbacks are attributed to the timer's creator, and the PSP
   idle thread plus signal handlers as separate execution contexts.

### 6.3 Where instrumentation is unsafe

* **Inside a table-locked region.** This means everything between `OS_Lock_Global` and `OS_Unlock_Global`: the
  ALLOCATED event, `FindNextFree`, iterator loops (`OS_FileOpenCheck`, `OS_rename`), GLOBAL-mode API bodies
  (`OS_TaskGetInfo`, `OS_DirectoryRead`, console write). Any OSAL call on the same type self-deadlocks (NORMAL
  mutex, Section 1.4). Any delay lengthens the lock hold. Because waiters retry at most 5 times, probe delay can turn
  successes into `OS_ERR_OBJECT_IN_USE` or cause F7. This is probe effect that changes behavior, not only timing.
* **The probe must never call OSAL name or search lookups** (`OS_*GetIdByName`, `OS_TaskFindIdBySystemData`)
  or `OS_GetResourceName` from inside other locked regions. These can trigger F1 and make the detector cause the
  deadlock it reports. Resolve thread identity with `gettid`/`pthread_self` and map it offline, or read the tables
  read-only from outside the process.
* **The probe must not call `OS_printf` or console APIs.** They add a process-wide total order through the console
  lock (osapi-printf.c L201-243), which masks races, and are not async-signal-safe.
* **Before task registration** (child, osapi-task.c L84-97): `OS_TaskGetId` returns 0 and attribution is wrong.
  Raw pthreads (console, timebase, sysmon) and the root thread have no task ID at all (Section 2.6).
* **Before `OS_API_Init`** (cfe_psp_start.c L352): OSAL transactions fail with `OS_ERROR` (osapi-idmap.c
  L301-305).
* **Timebase callback context:** `handler_mutex` is held (osapi-timebase.c L448-508) and timer APIs are refused
  there. A slow probe delays every timer on `cFS-Master` (10 ms period).
* **Signal handlers** (cfe_psp_exception.c L67-140): only async-signal-safe operations.
* **Cancellation.** `OS_TaskDelete` cancels at the next cancellation point. A probe that calls `write`, `fwrite`,
  `printf` or `nanosleep` while holding its own buffer lock, or while the application holds an OSAL, ES or table
  lock, can leave that lock held when the task is deleted. Disable cancellation around probe bodies
  (`pthread_setcancelstate`) or use lock-free per-thread buffers.
* **Binsem internal 2 s timedlock** (os-impl-binsem.c L50, L71). Stalling a holder (e.g. at a gdb breakpoint) makes
  other givers and takers fail with `OS_SEM_FAILURE`. This matters for gdb-based observation such as the R2 run.

### 6.4 Observation mechanism constraints

* **[F]** `core-cpu1` links `libosal.a`, `libpsp-pc-linux.a` and all cFE core libraries with `--whole-archive`, and
  links with `-rdynamic` / `--export-dynamic`
  (`build-native_std/native/default_cpu1/cpu1/CMakeFiles/core-cpu1.dir/link.txt`). **[E]** Apps are `-fPIC`
  shared objects (CONDITIONS C5).
* **[I]** An `LD_PRELOAD` library that defines `OS_*` would interpose calls coming from app `.so` files, which
  resolve through the dynamic symbol table. It would **not** see calls from cFE core (SB's `OS_QueuePut`, ES's
  `OS_TaskCreate`) or OSAL-internal calls. Those are direct calls inside one executable.
* **[I]** Full coverage needs one of:
  * a separate instrumented build: link-time `--wrap=OS_…` works for cross-object references, or a compiler pass;
  * dynamic probes: uprobes or gdb breakpoints, as in R2;
  * read-only inspection of the OSAL tables (`OS_common_table`) from outside the process.

  The pinned tree must stay unmodified, so an instrumented build has to live in a separate build directory.

### 6.5 Noise a memory-level detector would report [I]

These are intentional or acknowledged unsynchronized accesses that a word-level data-race detector (e.g.
ThreadSanitizer) would flag. They should be suppressed or modeled, and they are a reason to work at the OSAL API
level:

* the lock-free `active_id` check (osapi-idmap.c L422-424);
* the `name_entry` write in step e (os-shared-idmap.h L110) read under the lock by searchers;
* `OS_BinSemGetInfo_Impl` reading `current_value` (os-impl-binsem.c L482);
* `OS_TimeBaseGetFreeRun` (osapi-timebase.c L337-342);
* the exception ring counters (cfe_psp_exceptionstorage.c L79-118);
* `ShutdownReq` (`volatile`, cfe_psp_config.h L106);
* linux_sysmon's `num_cpus` poll (linux_sysmon.c L341-345).

---

## 7. Open questions and proposed confirmation experiments (not run)

1. **F1 confirmation.** In a scratch build linked against the built `libosal.a`, or with gdb on an existing OSAL test
   binary, hold a creator thread at `OS_CountSemCreate_Impl` (step f), call `OS_CountSemGetIdByName` with the same
   name from a second thread, then release the creator. Expected per code: the lookup returns
   `OS_ERR_INCORRECT_OBJ_STATE` and the creator hangs in `pthread_mutex_lock` inside `OS_Lock_Global`. Repeat for the
   delete window and with `OS_TaskGetIdByName` during `OS_TaskDelete`.
2. **F7 confirmation.** Delay the creator between `pthread_create` and FinalizeNew (breakpoint at
   os-impl-tasks.c L581) for more than 300 ms, or generate more than 4 task-table unlocks in that window. Check that
   the child never reaches its entry point and that the record stays active.
3. **CPU-0 pinning.** Record `Cpus_allowed_list` for every thread in `/proc/<pid>/task/*/status` during a run to
   confirm the `CFE_*` affinity from cfe_psp_start.c L184-189. Add it to the environment record (E-series).
4. **Tick loss under load.** Compare `freerun_time` with elapsed `CLOCK_MONOTONIC` to see whether timer overruns
   drop ticks (Section 3, timebase row).
5. **Which OSAL version first changes F1/F3?** Read the nasa/osal history after `d2d877a6`; issue search only so far.
6. **Design decision for the team:** is the detector's ground truth the API-level HB (portable, my recommendation)
   or the POSIX implementation-level HB (closer to the observed binary)? The choice changes which races are
   reportable.

---

## 8. References (verified)

* L. Lamport, "Time, Clocks, and the Ordering of Events in a Distributed System," *Communications of the ACM*
  21(7):558-565, 1978. Verified via the ACM-hosted PDF (amturing.acm.org/p558-lamport.pdf) and course mirrors. One
  mirror lists pp. 558-564.
* C. Flanagan and S. N. Freund, "FastTrack: Efficient and Precise Dynamic Race Detection," *PLDI 2009*. Verified via
  the authors' site and the CACM research highlight.
* K. Serebryany and T. Iskhodzhanov, "ThreadSanitizer – data race detection in practice," *WBIA '09*, pp. 62-71,
  2009. Verified via the Google Research listing.
* S. Lu, S. Park, E. Seo and Y. Zhou, "Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug
  Characteristics," *ASPLOS 2008*. About one third of the non-deadlock bugs studied are order violations. Verified
  via author-hosted copies.
* The Open Group Base Specifications, `pthread_mutex_lock` (Issue 6/7): for `PTHREAD_MUTEX_NORMAL`, "Attempting to
  relock the mutex causes deadlock." Verified via pubs.opengroup.org search results and mirrors.
* [U] The POSIX rule that only one signal is queued per timer, with overruns counted by `timer_getoverrun`, was used
  only as an inference (Section 3) and not checked against the spec text.

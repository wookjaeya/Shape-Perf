# Review X2: cFS fidelity of SPEC_v1 (Kairos)

- Reviewer role: adversarial, cFS fidelity. I read only and wrote only this file.
- Target: `scratchpad/arch/design/SPEC_v1.md` (2332 lines), mainly §1.3–1.8, §3, §4.2–4.6, App. A.
- Method: I re-read every mechanism in §3 and App. A against the pinned tree `/home/user/work/procrace/cfs_ref/cFS` (paths below are relative to it). I also cross-checked [R01], [R02] and [R03]. **[F]** = I re-read the code for this review. **[I]** = my inference.
- Verdict in one line: the mapping is detailed and mostly accurate line by line. But one flaw is critical: the flagship R2 certificate (the S3 exit gate) does not go through as specified. Nine more issues are major: mechanisms that are mis-modelled or missing in the App-record windows, OSAL M_impl waits, restart/shutdown, SB transmit/capacity/mode, σ_R1, and configuration closure. They change verdicts, consequences or coverage claims.

---

## A. What I checked and found correct (no action)

Spot checks that match the spec's citations [F]:
- **OSAL ID map.** Header invariant `osapi-idmap.c:384–385`; early return at `:401–404`; 5 attempts at `:475–480`; final ID write `:1092`; unlock `:1105`; `RESOURCE_CREATED` callback after the unlock (`:858–865`); AllocateNew EXCLUSIVE→ConvertToken (`:1171–1214`).
- **ES core path.** `cfe_es_start.c:82` memset; phase stores `:87, :183, :194, :216, :234`; EarlyInit loop `:752–773`; reserve `:803`; `StartAppTask` `:810`; publish `:818`; failure memset `:823`; core sync `:835`; panic `:838–846`; `MainTaskSyncDelay` `:860–918`; soft barriers `:210, :225`.
- **ES apps.** `GetTaskFunction` (`cfe_es_apps.c:559–605`: delay first, then a locked check; the 21-check bound follows from 1000/50); `StartAppTask` (`:664–703`); script reserve `:821` and publish `:869`; library init while RESERVED (`:1010–1037`); scan (`:1110–1146`); `CleanUpApp` (`:1407–1640`).
- **ES API.** `ExitApp` STOPPED store `:399`; `RunLoop` raise `:469–472`; `WaitForSystemState` raise and poll (`:528–607`); `WaitForStartupSync` returns void (`:616–619`); `GetAppIDByName` returns RESERVED (`:637–656` with `cfe_es_resource.c:97`); `CreateChildTask` (`:1202–1294`); `DeleteChildTask` deletes under the ES lock (`:1383`); `ExitChildTask` (`:1453–1478`).
- **SB.** `cfe_sb_task.c:138/155`; `cfe_sb_api.c:120, 153, 163, 185–203, 1192–1195`; `cfe_sb_priv.c:786, 1010–1086, 1177–1194`.
- **EVS.** `cfe_evs.c:106, 185–194`; `cfe_evs_task.c:213–221, 273, 311–312`; `cfe_evs_utils.c:613–623`.
- **TBL.** `cfe_tbl_task.c:196`; `cfe_tbl_api.c:145–149, 212–229, 305–309`; `cfe_tbl_registry.c:182–196`; `cfe_tbl_task_cmds.c:423–431`.
- **CDS and TIME.** CDS `cfe_es_cds.c:311–477`; TIME `cfe_time_api.c:614–645`, `cfe_time_utils.c:1018–1041`, `cfe_time_tone.c:1310–1333`.
- **PSP.** `cfe_psp_support.c:116–121` (Panic → abort; the 500 ms delay is in the ES callers, `cfe_es_start.c:55, 769, 844`); `cfe_psp_start.c:174, 184–189, 197–201`; `psp_module_list.cmake` lists `linux_sysmon`.
- **Apps.** CF loop and macro (`cf_cfdp.c:1270–1301`, `cf_internal_cfg.h:172–173`); `sch_lab_app.c:168, 176, 184, 303`; `to_lab_app.c:64–67`; `hs_sysmon.c:50–72`.
- **Header provenance.** `cfe_es.h:400–403`, `cfe_evs.h:168–172`, `cfe_sb.h:449–464`.
- **Constants.** 50 / 1000 / 30000 ms and kill 5 × 1000, via the CFGVAL wrappers at `cfe_es_internal_cfg.h:198, 228, 859, 878–879` (the spec cites `:199, :229`, off by one) and `default_cfe_core_private_internal_cfg.h:68`.

Errata §0.5 rows 1, 4, 5 and 6 are confirmed.

---

## B. Issues (most severe first)

### X2-01 [critical] The R2 FIXED_ORDER certificate does not go through: T-OBS's writer set omits the startup thread's own failure-path record clear (§4.4 T-OBS, §4.2 step 6, App. A.1 step 3, S3 gate)

**Problem.** The observation `o_ES` is the success exit of `MainTaskSyncDelay`. Its predicate is "no used record with `AppState < RUNNING`" (`cfe_es_start.c:878–888`). On the `StartAppTask` failure branch the startup thread runs `memset(AppRecPtr, 0, …)` at `cfe_es_start.c:823` [F]. That store un-uses the RESERVED record, so it makes the observed predicate true. It is therefore a W entry under the spec's own transition-aware filter (§4.2 step 6), and it occurs after `f = :803`.

The W_o filter removes only writers with `PWB(w,f)` or `PWB(o,w)`. `PWB(o_ES, :823)` needs a frontier node on the failure branch strictly before the memset. The skeleton (§4.3) turns outcome branches into *transitions*, not nodes, so no such node exists. The shared lock acquire at `:815` is in `Anc(o_ES)`. So `:823` stays in W_o. The meet `∩ MB(w)` then drops `m_ES`, `m_ES ∉ MB(o_ES)`, and the certificate fails. A.1 step 3 does not list this writer at all.

**Fix.**
- Add to T-OBS (and to `kz proofcheck`): a writer `w` in o's own actor instance with no skeleton path `w →* o` is excluded. This is sound by A-SKEL: such a `w` cannot lie between `f` and `o_k` in that actor's run. The checker re-validates it by reachability.
- List `:823` (and `:893` for the script phases) explicitly in A.1.
- Add a mutation test that re-inserts it.
- Re-derive the EVS-iteration certificate the same way.

### X2-02 [major] App-record windows: wrong child-task claim, missing uses, single-branch consequences (§3.3 "Consequences", §3.7 "Child tasks", App. A.2 step 4, A.3, §7.3)

**Problem [F].** Seven call sites use `GetAppRecordByContext` or `GetTaskRecordByContext`: `ExitApp` (`cfe_es_api.c:312`), `RunLoop` (`:463`), `WaitForSystemState` (`:533`), `GetAppID` (`:750`), `GetTaskID` (`:785`), `CreateChildTask` (`:1262`) and `ExitChildTask` (`:1459`). All see the window.
- §3.7 says the parent's record "is already final, so the App-record window does not apply" to child tasks. That is false for core apps that create children in TaskInit: TIME at `cfe_time_task.c:160, 173`, and ES's background task at `cfe_es_backgroundtask.c:214`. It is also false for script apps (FM `fm_child.c:86`).
- **`ExitApp` in the window is a silent no-op.** With `AppRecPtr == NULL` it only unlocks and returns (`cfe_es_api.c:312–313, 417`). So EVS's "ExitApp will not return" (`cfe_evs_task.c:219–220`) is false in the window. EVS then continues to `WaitForSystemState` (raise only if the window has closed), skips its loop, and calls `ExitApp(RUNTIME_ERROR)` → `OS_TaskExit` (`:257`, `cfe_es_api.c:365–379`). That leaves either a running system without an EVS task, or a 30 s core-sync panic.
- If the window closes before the first `ExitApp`, `ResetCFE` → `CFE_PSP_Restart` aborts after 10 s (X2-04). That abort can come before the 30 s panic.
- **`RunLoop` in the window returns false** (`cfe_es_api.c:494–500`). A script app then leaves its loop, its `ExitApp` is a no-op, its task returns, and its record stays used with `AppState` ≤ RUNNING forever. The scan acts only on `> RUNNING` (`cfe_es_apps.c:1112–1146`). The result is a zombie app and two soft-barrier timeouts.

The spec's single harm label `processor_reset_requested` (A.2) covers one branch only.

**Fix.**
- Make `es.self(exec) LIVE` a generic contract instantiated at every caller listed above, each with its own not-ready outcome: `GetAppID`/`CreateChildTask` error; `RunLoop` false; `WaitForSystemState` skips the raise; `ExitApp` silent no-op.
- Delete the §3.7 sentence.
- Model consequences as chains with all branches, using the harm vocabulary plus `task_never_runs`/`app_zombie`.
- Mark each branch [I] until it is run.

### X2-03 [major] OSAL M_impl RESERVED wait: the timed-wait component is omitted, and the timebase thread is a missing instance (§1.4 M_impl, §3.2, §3.3 K-SPAWN.osal, §3.14, §4.3)

**Problem [F].**
- `OS_WaitForStateChange_Impl` waits with `pthread_cond_timedwait` for `attempts² × 10 ms` (`posix/src/os-impl-idmap.c:151–169`). An attempt is therefore consumed by a timeout *or* by the broadcast that every type unlock sends (`:106–129`). [R01 §2.2] says so explicitly. The spec says only "every unlock of the type broadcasts and consumes an attempt". As a result:
  - (a) the σ_U∞ treatment of this implementation timeout is undefined, which decides F7's timeout-dependent refinement;
  - (b) F7 needs only a creator delay of about 300 ms in σ_U.
- §3.2 also says "GLOBAL/REFCOUNT/EXCLUSIVE lookups of a RESERVED record: await". That holds only for by-ID tokens. A by-name or search token copies `active_id = RESERVED` and fails at `:401–404` with no wait.
- The timebase callback thread does the same `GetById(GLOBAL)` handshake. It silently `return`s on failure (`osapi-timebase.c:386–390`), leaving `cFS-Master` without callbacks, so 1 Hz never arrives. §3.3 names only `OS_TaskPrepare` as an instance.

**Fix.**
- Model the attempt as `await(state ≠ RESERVED; wake ∈ {broadcast(type unlock), timeout_k})`, with `timeout_k` from `os-impl-idmap.c:153–162`.
- Record whether σ_U∞ disables it (a deviation, with its reason).
- Split by-ID from by-name/search in the M_impl automaton.
- Add the timebase thread (and any OSAL-internal thread with the handshake) to the K-SPAWN.osal census.

### X2-04 [major] Restart/shutdown is mis-modelled, which contradicts A.7 and the S3 gate "timebase lookups protected by certificate" (§1.8, §3.5, App. A.7, A.2)

**Problem [F].**
- `CFE_PSP_Restart` writes the boot record, sets `ShutdownReq`, and sends `pthread_kill(SIGUSR1)` to the PSP main thread. It then sleeps `CFE_PSP_RESTART_DELAY` = 10000 ms and calls `abort()` (`cfe_psp_support.c:78–107`; `pc-linux/inc/cfe_psp_config.h:57`). The caller never returns.
- SIGUSR1 is blocked process-wide (`os-impl-tasks.c:262–291`). It is consumed only by the `sigwait` loop of `OS_Application_Run`, which starts after `CFE_ES_Main` returns (`cfe_psp_start.c:449, 452–508`). `OS_DeleteAllObjects` runs only there (`:507`).

The spec models "the caller may return … other actors continue … OS_DeleteAllObjects concurrently with running tasks". Under that over-approximation, `OS_DeleteAllObjects` is a reachable, unordered disabler of every `osal.obj`. ResetCFE is reachable in σ_U through the EVS window (X2-02). So the PROTECTED case (b) for `cFS-Master` lookups needs `PWB(d,x*) ∨ PWB(u,d)` and cannot be certified, yet A.7 claims "the disabler set is empty". Even with faithful modelling, SCH_LAB's lookup can follow a soft-timed-out return of `CFE_ES_Main` (`cfe_es_start.c:225–228`). So "protected" holds only with teardown scoped out.

**Fix.**
- Encode three effects:
  - requester: `ShutdownReq` store → `OS_TaskDelay(RESTART_DELAY)` → `abort` (terminal, T-TERM);
  - PSP main, after `CFE_ES_Main` returns: observe `ShutdownReq` → `OS_TaskDelay(100)` → `OS_DeleteAllObjects`;
  - `CFE_PSP_Restart(POWERON)` deletes the shm (`:62–69`), which feeds B.Init of the next run.
- Treat teardown as its own scenario family (or as a contract horizon "until ShutdownReq"), so that default-B certificates are stated "outside teardown" and K-LIFE floods during shutdown are kept apart.
- Correct A.7 and A.2's watchdog and harm (abort after 10 s versus panic after 30 s).

### X2-05 [major] K-MODE (RC13): SB's own task executes the mode change and the replay, not SBN (§3.9 row "Subscription reporting mode", §3.15 RC13, §3.14, App. A.5)

**Problem [F].**
- SBN only *transmits* two commands, `ENABLE_SUB_REPORTING` and `SEND_PREV_SUBS` (`sbn_subs.c:43–60`).
- SB's main task sets `SubscriptionReporting` when it dequeues the first command (`cfe_sb_task.c:294–298, 1224–1226`) and runs the replay `CFE_SB_SendPrevSubsCmd` in its own context (`cfe_sb_task.c:1169`; dispatch `cfe_sb_dispatch.c:86–89`).
- Subscriber tasks read the flag after the SB unlock, without a lock (`cfe_sb_task.c:659`), inside `SendSubscriptionReport`, called at `cfe_sb_api.c:1192–1195`.

So the LP of the mode transition is in SB's actor, reached through R7 over SB's command pipe, which has its own capacity and MsgLim. The flag read is an ad hoc unlocked read that is missing from §3.14. The ALLSUBS burst comes from SB's actor. A.5's attribution, and the K-CAP put set for `SBNSubPipe`, depend on all three facts.

**Fix.**
- Add Φ entries:
  - SBN transmit (command) → SB handler effect `lifecycle(sb.mode.subreport, ON)` with LP `cfe_sb_task.c:296`;
  - a `use` read at `:659`, tagged `assumed` (A4) and listed in §3.14;
  - the replay as a transmit loop in SB's actor.
- Chain K-MODE through R7 on SB's command pipe, with a K-CAP/MsgLim instance for that pipe.
- Re-state A.5 with SB's actor as the replay source.

### X2-06 [major] SB transmit is two-phase; capacity has two levels; the K-CAP oracle is SB accounting, not queue depth (§3.9, §3.15 RC14, App. A.5, §8.1 RQ5, §8.6)

**Problem [F].**
- `FindDestinations` snapshots the route and increments `BuffCount`/`CurrentQueueDepth`/`PeakQueueDepth` optimistically, under the SB lock (`cfe_sb_priv.c:1007–1104`, peak at `:1071–1073`).
- The `OS_QueuePut` calls run later, outside the lock (`:1177–1178`), and are undone on failure (`:1184–1216`).
- Consequences:
  - (a) `PeakQueueDepth` counts messages before the put succeeds, so it can exceed the effective `mq` capacity and include later Q_FULL drops. It is not a valid "per-pipe peak" oracle for `depth(p) < cap_B(p)`.
  - (b) A pipe deleted between snapshot and put gives `Q_WR_ERR`/`InternalErrorCounter` (`:1195–1200`), an outcome missing from the vocabulary.
  - (c) There is a second capacity per (MsgId, pipe): `MsgId2PipeLim`, default `CFE_PLATFORM_SB_DEFAULT_MSG_LIMIT` = 4 (`cfe_sb_internal_cfg.h:106–107`), checked at the snapshot (`:1058–1064`, `MsgLimitErrorCounter`). K-CAP and RQ5 omit it.
  - (d) Every transmit also calls `GetAppID` in the caller's context (`:1004`), which is an extra identity use.

**Fix.**
- Split transmit into a `route_lookup` LP (under the lock) and one `put` LP per destination.
- Add `K-CAP.msglim`, with the limit constant-evaluated from each Subscribe/SubscribeEx site.
- Add `MsgLimitErrorCounter` and `InternalErrorCounter` to the oracles.
- Label `PeakQueueDepth` as SB-accounted depth, and take real depth from put/get outcomes.

### X2-07 [major] σ_R1 and the native RT arm (D4) do not match OSAL POSIX real-time behaviour (§1.8 σ_R1, §10.1 D4, §7.6 H-V)

**Problem [F].**
- When it may use RT scheduling, OSAL picks `SCHED_RR`, not FIFO, if FIFO's priority spread is ≤ `OS_MAX_TASK_PRIORITY` = 255 (`os-impl-tasks.c:348–365`; `osapi-task.h:32`). That is always the case on Linux (1..99).
- OSAL gives the root thread, which runs `CFE_ES_Main`, the highest priority (`os-impl-tasks.c:391–392, 409`).
- OSAL compresses OSAL priorities many-to-one (`:80–106`).
- OSAL mutexes use `PTHREAD_PRIO_INHERIT` (`os-impl-mutex.c:79–82`).

σ_R1 ("priorities from script and core table; FIFO among equal priorities") omits the startup thread's priority and priority inheritance. Under the root priority, for example, the core App-record window cannot open while the startup thread is runnable on one CPU [I]. So σ_R1 classes, and the C3-at-6.4.1 test of H-V, can be wrong in either direction. D4's "root-imposed SCHED_FIFO" would be overridden by OSAL's own policy selection at `OS_API_Init`.

**Fix.**
- Parameterize σ_R1 by a cited target scheduling profile: policy and timeslice, priority-mapping function, startup-thread priority, priority inheritance on mutexes, per OSAL backend (POSIX versus RTEMS/VxWorks for 6.4.x).
- For the native arm, record OSAL's own choice (RR plus root at maximum) as the realized σ, or state the override as a deviation.

### X2-08 [major] Configuration closure misses items that change actors, capacities and table contents (§1.7 B, A3, A11, §4.1)

**Problem [F].**
- (a) **Startup script.** On a PROCESSOR reset ES first opens the volatile `/ram/cfe_es_startup.scr` (`cfe_es_apps.c:134–165`). `/ram` is `/dev/shm/osal:RAM`, which persists across runs [R01 §5.3]. A3 and B.Config name only the generated script, but the actor set depends on B.Init and the reset type, and the reset type itself comes from the boot record or `-R` (`cfe_psp_start.c:264–328, 411–434`).
- (b) **Queue depth.** `cap_B = min(requested, msg_max)` holds only for euid ≠ 0. The BSP reads `msg_max` only when not root (`osal/src/bsp/generic-linux/src/bsp_start.c:56–74`; truncation `os-impl-queues.c:98–105`).
- (c) **Critical tables.** These are restored from CDS on PROCESSOR resets (`cfe_tbl_api.c:117–131`), so their values come from B.Init (e.g. MD's active critical table, [R03 RC8]). A11 assumes `.tbl` values.
- (d) **CWD.** The working directory fixes the `/cf` mapping (`cfe_psp_start.c:378`), the SysV key files and `EEPROM.DAT` [R01 §5.3], yet it is not in B.Env.

**Fix.**
- Add the reset-type input (argv plus boot record), the script-selection rule, `/ram` contents, euid, CWD and decoded CDS/critical-table images to B, each with its source.
- Evaluate the selection rule in `kz basis`.
- Record the actually opened script path in the closure boot (O5).
- Condition `cap_B` on euid.
- Extend A11 to "table values from `.tbl` on POWERON, from the CDS image on PROCESSOR".

### X2-09 [major] RC5 key resolution: transmit MsgIds live in message headers, and the spec has no rule for them; "full" is overstated (§3.15 RC5, §4.2, §4.6, §7.1 source 1)

**Problem [F].**
- `CFE_SB_TransmitMsg` takes no MsgId argument. The key is written earlier into the message header by `CFE_MSG_Init`/`SetMsgId` (e.g. ES HK init at `cfe_es_task.c:346–362`, transmitted later from a command handler), encoded by the mission MSG module.
- SC RTS/ATS, HS MAT and LC tables carry whole command packets.
- §4.6 resolves keys by "value analysis of κ.sel at t", and §4.2 tracks only support locations of readiness conjuncts. Most transmit instances would therefore get key ⊤, i.e. `open(key_unresolved)`.
- RC5 is the kind with the measured losses (RQ5).

**Fix.**
- Declare in Φ an abstract location "message-header MsgId", with encode/decode summaries for `CFE_MSG_Init`/`SetMsgId`/`GetMsgId`.
- Run a field-sensitive value analysis over message buffers, and add a table-packet decoder.
- Report the share of transmit sites with concrete, symbolic and ⊤ keys.
- Mark RC5 "partial" until that share is measured.

### X2-10 [major] RC2 and RC4 template mis-typing: task lookups are OSAL lookups, and loader symbols have a visibility scope (§3.15 RC2, RC4)

**Problem [F].**
- `CFE_ES_GetTaskIDByName` defers to `OS_TaskGetIdByName` (`cfe_es_api.c:716–717`). Its readiness is `osal.obj<task>`, including RES_CN `INCORRECT_OBJ_STATE` and the type-lock leak (R01 F1 lists it). It is not an ES record with an `IsMatch` oracle. HS's XCT calls it (`hs_cmds.c:148`).
- ES loads app modules with LOCAL symbols and libraries with GLOBAL symbols (`cfe_es_apps.c:470–491`). MD and MM resolve symbols with the global `OS_SymbolLookup` (`md_utils.c:214`, `mm_cmds.c:143`, `mm_utils.c:525`). So an app symbol is never globally visible, and those instances are `no-provider(B)`, not order-dependent.

RC4's Req ("module LIVE; symbol defined") has no scope.

**Fix.**
- Route task-name lookups to `osal.obj<task>`, so they get K-REG.osal and K-LOCKBAL.
- Add a scope (LOCAL/GLOBAL) to the loader kind. The provider effect is `OS_ModuleLoad` with constant-evaluated flags; the lookup kind is global versus module-specific.
- Re-state RC2 and RC4 coverage.

### X2-11 [minor] K-PHASE is not indexed by state level (§3.6 K-PHASE, §3.15 RC7)

**Problem [F].** The raise level depends on the caller type and the requested system state (`cfe_es_api.c:545–563`). For an external app, CORE_READY raises to EARLY_INIT, APPS_INIT to LATE_INIT, and OPERATIONAL to RUNNING. The two script barriers test LATE_INIT and RUNNING (`cfe_es_start.c:210, 225`). The spec's "declaration = its raise or first RunLoop" can credit a declaration against a barrier it does not satisfy.

**Fix.** Index declarations and barriers by AppState level. Derive the consumer's barrier from its `WaitForSystemState` argument, constant-evaluated.

### X2-12 [minor] Persistence: generation-crossing CDS reuse within one run, and INIT for persistent locations (§1.7, §3.12, §4.2 step 9)

**Problem [F].**
- `ALREADY_EXISTS` and restore also happen within one run after an app restart or reload (`cfe_es_cds.c:342–348, 461–472`), but K-PERSIST is defined over run sequences only.
- INIT is computed from static initializers and EarlyInit stores "before the startup thread's first spawn". But the PSP spawns the `cFS-Master` thread before `CFE_ES_Main` (`cfe_psp_start.c:391`; `cfe_psp_soft_timebase.c:53`), and `/ram`, CDS and reset-area contents come from S0, not from initializers.

**Fix.**
- Define K-PERSIST over generations (`B_restart`) as well as runs.
- Take INIT for persistent-scope locations from the decoded S0.
- Define INIT as the program-entry state, with EarlyInit stores as ordinary writers.

### X2-13 [minor] RC9 "full" is overstated: FM's child queue is app-private memory, outside Φ's framework vocabulary (§3.15 RC9)

**Problem [F].** The FM protocol is a counting semaphore, a mutex and a count in `FM_AppData` (`fm_child.c:65–100, 162`). The spec gives no generic rule that recognizes it without app-specific Φ entries, which the §7.2 lints forbid in engines. If entries go in `profiles/`, they need provenance.

**Fix.** Either treat it via `unmodelled-sync` → `unknown(unmodelled)`, or add an app-profile layer with provenance and authoring cost. Mark RC9 "partial".

### X2-14 [minor] Small key and disabler errors in SB, EVS, TIME and teardown (§1.3, §3.8, §3.9, §3.10, §3.13)

**Problem [F].**
- (a) SB events are dropped when the caller's ES task record is missing, or when the per-task recursion bit is set (`cfe_sb_priv.c:305–328, 773–790`). This is a documented drop, but it is absent from Allowed.
- (b) `CFE_EVS_Register` first memsets the slot (`cfe_evs.c:56`). That is a disabler of `evs.reg(a)` on re-registration, so "one writer per slot" holds only without restarts.
- (c) TIME synch slots are keyed by app *index*, with no generation, and set by an unlocked check-then-set (`cfe_time_api.c:628–640`); they are cleared by index (`cfe_time_utils.c:1026–1034`).
- (d) The cleanup order is the generated core-object-table order of modules with a `Cleanup` hook (`cfe_es_apps.c:1525–1545`), not a hard-coded EVS→SB→TBL→TIME. App tasks keep running until after module cleanup (`:1561–1574`).

**Fix.**
- Add `documented_drop(SB recursion guard)`.
- Add the memset as a D entry.
- Key `time.synchcb` by slot index.
- Derive the cleanup order from B, and state the task-alive interval as the K-LIFE window.

---

## C. Contract-kind coverage (the 15 RCs of [R03 §4]) after this review

| RC | Spec claim | Reviewer assessment |
|---|---|---|
| RC1 | full | plausible; needs X2-03 (timed waits) and X2-04 (teardown) |
| RC2 | full (order) | **partial**: task lookups mis-typed (X2-10) |
| RC3 | full | plausible |
| RC4 | full | **partial**: symbol scope missing (X2-10) |
| RC5 | full | **partial** until transmit-key resolution exists (X2-09) |
| RC6 | full | uses and consequences incomplete (X2-02); R2 certificate broken until X2-01 |
| RC7 | full | level indexing missing (X2-11) |
| RC8 | partial | still partial; also generation crossing (X2-12) |
| RC9 | full | **partial** (X2-13) |
| RC10 | full | plausible, if callback contexts are actors whose T-SPAWN sites are the registration calls (§3.1); otherwise the O-stratum P-TB blocks a certificate |
| RC11 | full | plausible |
| RC12 | partial | agreed |
| RC13 | full | **mis-modelled** (X2-05) |
| RC14 | full | **partial**: second capacity and oracle (X2-06) |
| RC15 | partial | agreed |

Net: about 6 of 15 are full as specified. Fixing X2-05, X2-06, X2-09 and X2-10 would bring most of the rest to full.

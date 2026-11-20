# R2 (cFE #2663) on a CORD-instrumented copy of the ENV build: observation without a debugger

- Date: 2026-10-08. Policy: `../CONDITIONS_POLICY.md`. Revised 2026-10-12 in answer to `../Architecture/history/review_EXT_20261012.md` §2.3. §8 was rewritten because it still said 11 uses and "24 not run" after §7 had recorded 24. Stale statements in §1–§7 and §9 were corrected or labelled by run group. No run was added, and no earlier number was changed. Numbers added for r2_c4–r2_c6 in §1 and §3–§6 come from their existing logs in `../env/logs/`.
- **Status (2026-10-12): CORD is retired** (`../tool/RETIRED.md`). Its runs are kept as records and are cited **only as earlier observations**. They are not results of the new detector (Kairos, not implemented). How the paper cites them is PI decision D12, still pending; the recommendation is "prior observations only" (`../Architecture/spec/PI_DECISIONS.md` D12).
  - The retired files named below were removed from the tree: everything under `../tool/cord/`, `../tool/DESIGN.md` and `../env/run_r2_cord.sh`. They are still available with `git show 62e05a9:research/cfs_procrace/<path>`.
  - The instrumented copy `/home/user/work/procrace/cfs_cord` and the raw-trace directory `/home/user/work/procrace/cord_runs` were deleted on 2026-10-12 (`../env/HOST_CLEANUP_20261012.md`). Copies of the raw traces are in `../env/logs/r2_c*_cord_trace.jsonl`.
- **What this is.** Case R2 observed in real cFS v7.0.1 (cFE `c5fb2b4d`) **without gdb**, by the CORD event recorder compiled into `core-cpu1`, and judged by `../tool/cord/cord_analyze.py` (HB model of `../tool/DESIGN.md` §3).
  - Build: a copy of the ENV source with a small instrumentation patch (`../tool/cord/patches/cfe_v701_r2.patch`), built and run under the ENV conditions.
  - Runs, in **two groups that are not pooled** (§8):
    - `r2_c1`–`r2_c3`: first build. Its module version table differs from ENV's (all `NULL`), so it gives 11 violating uses per boot (§1, §4).
    - `r2_c4`–`r2_c6`: second build, with ENV's version table restored. It gives 24 per boot (§7).
    - Both groups used `../env/run_r2_cord.sh` and `../env/run_cfs.sh` v5.
- **What this is not.**
  - Not a run of the unmodified ENV binary. That run is `R2_gdb_v701.md`, under gdb, and it is a separate experimental group.
  - Not a witness/delay experiment: `CORD_DELAY` was not set; no predicted violation existed to witness.
  - Not a comparison with existing tools.
- Logs:
  - `../env/logs/cord_build_*`: first build, with copy manifest, build and configuration comparison.
  - `../env/logs/cord_build_v2_*` and `cord_build_v1_core_sha256_before_removal.txt`: second build.
  - `../env/logs/r2_c{1,…,6}_*`: console, `.meta`, persisted-state snapshots, driver record, raw trace `_cord_trace.jsonl`, analyzer output `_cord_sb.{txt,json}` and `_cord_all.txt`, and the R2 summary `_cord_summary.{txt,json}` from `../tool/cord/r2_cord_summary.py`.

## 1. Conditions

| Item | Value | Status |
|---|---|---|
| Source | `/home/user/work/procrace/cfs_cord/cFS`: copy of the ENV tree (`cfs_ref/cFS`, bundle `088b2fa8`, cFE `c5fb2b4d`) without `.git` and `build-native_std`. Before patching, all 4316 entries (files with sha256 + mode, symlinks with target) and the directory list were identical (`cord_build_00_copy_manifest.txt`, manifest sha256 `64267a8f…`). Owned by `ubuntu`, as the ENV tree (ENV D-1). The tree was deleted on 2026-10-12 (`../env/HOST_CLEANUP_20261012.md`). | from-reference (ENV A1–A5) for the content |
| Copy without `.git` | As instructed. **Not neutral:** cFE generates its module version table at build time with `git describe` (`cfe/cmake/generate_git_module_version.cmake` L35-52, selected by `cfe/cmake/mission_build.cmake` L130-138). Without `.git` every entry is `NULL` (ENV: `git:v7.0.1`; sample_lib `git:v7.0.0-10-g2d8b7e8`). ES then sends 4 instead of 17 init events: the 13 "Version Info: Module …" events are missing (console: 1 "Version Info" line vs 14 in ENV run06 and in r2_g1). This changes the R2 count (§4.2). | **deviation for r2_c1–r2_c3 only** (copy method). Corrected before r2_c4–r2_c6 by restoring ENV's generated table through cFE's user-supplied hook (§7; `../env/CONDITIONS.md` H-10: "deviation in mechanism only; the output equals the reference"). |
| Copy tool | GNU tar 1.35 with the same excludes; `rsync` is not installed and was not installed. Equality is proven by the manifest. | harness (no effect on content) |
| Instrumentation | `../tool/cord/patches/cfe_v701_r2.patch` (§2) | tool, not a reference condition |
| Build | `build_cfs.sh` v4 `existing`: `make native_std.prep`, `make native_std.install` (README L104-105), as `ubuntu` via `runuser`, same environment refusal list and records as the ENV build (ENV C2, C3, C4, C11). First build (r2_c1–r2_c3): logs `cord_build_00_env.log`, `_00_steps`, `_04_prep`, `_05_install`, `_06_outputs`, `_07_record`. Second build (r2_c4–r2_c6), same steps after the version-table hook was added: `cord_build_v2_*` (§3, §7). | from-reference |
| Build configuration | First build: identical to ENV except the instrumentation (§3; `cord_build_08_config_compare.txt`). Second build: the same steps plus the two version-table `sample_defs` files. The configuration comparison was not repeated for it (§3). | from-reference + instrumentation (+ H-10 for the second build) |
| Run user, cwd, command | `ubuntu` via `setpriv`, `build-native_std/exe/cpu1` of the copy, `./core-cpu1` without options (ENV E1–E3) | from-reference |
| Run length, stop | E10 `operational` (README L114 line, event-based), E11 (a) `ci-es-restart-poweron` | E10 unspecified_by_reference (as ENV); **stop method still PI decision D3** |
| Queue depth | README mode, `msg_max` 10 (ENV E5) | from-reference (README L102); **D2 pending** |
| Process environment | ENV E12 plus `CORD_OUT=/home/user/work/procrace/cord_runs/<tag>.jsonl` (directory owned by `ubuntu`). cFS does not read it. `CORD_DELAY`, `CORD_BUF`, `CORD_DUMP_AFTER_MS` unset. | **deviation** from E12, instrumented build only (H-9) |
| Recorder settings | Default buffer 65536 events per thread (`cord.c`); trace written at exit (atexit) | tool default (unspecified_by_reference) |
| Recorder side effects | Constructor installs handlers for SIGSEGV/SIGBUS/SIGABRT/SIGTERM/SIGINT/SIGFPE (the PSP later replaces SIGINT, SIGTERM, SIGFPE: `cfe_psp_exception.c` L232-233, L266); per-thread buffers of 65536 × 120 B are `calloc`ed lazily (≈7.5 MiB virtual per thread; RSS not measured) | instrumentation effect (recorded) |
| Isolation | Passed before every run (ENV E13); one cFS process at a time | from-reference (harness rule H-5) |
| Persisted state | Copy's `exe/cpu1` had no `EEPROM.DAT` and no keyfiles before r2_c1 (created by r2_c1; ENV runs had their own). The second build replaced the build tree, so they were absent again before r2_c4 and were created by r2_c4 (`r2_c4_persist_before.txt`). `/dev/shm/osal:RAM/cfe_sb_pipe.dat` (1624 B, from M1 B3) present in all runs, shared with the ENV runs. No SysV segment before/after. All boots POWER ON. | recorded (not cleaned, policy) |
| Runs | 6, sequential, in two groups. r2_c1–r2_c3 (first build, `core-cpu1` sha256 `1040d64e…`): 07:02:30Z, 07:04:09Z, 07:04:13Z. r2_c4–r2_c6 (second build, `7d618152…`): 07:13:17Z, 07:13:20Z, 07:13:24Z (driver records `r2_c*_driver.txt`). | count unspecified_by_reference (assumption, same as the gdb runs) |

## 2. Instrumentation (patch summary)

`../tool/cord/patches/cfe_v701_r2.patch` (sha256 `c6a8b1a4…`) is the complete `diff -ruN` between the ENV source and the copy: 14 files, 2 new (`cfe/cord/cord.c`, `cord.h`, verbatim copies of `../tool/cord/`, same sha256). Its header lists every site with original and patched line numbers. Summary:

| Event | Site (original line, cFE `c5fb2b4d`) |
|---|---|
| PUB `sb.appid` (a = stored AppId) | `cfe_sb_task.c` L138, right after `CFE_ES_GetAppID(&CFE_SB_Global.AppId);` |
| PUB `sb.evsreg` | `cfe_sb_task.c` after the successful `CFE_EVS_Register` check (L155-160) |
| USE `sb.appid` + USE `sb.evsreg` (a = AppId read) | all **39** SB call sites that pass `CFE_SB_Global.AppId` to `CFE_EVS_SendEventWithAppID`: `cfe_sb_api.c` 34 (CreatePipe 6, DeletePipeFull 3, SetPipeOpts 3, GetPipeOpts 3, GetPipeName 2, GetPipeIdByName 3, SubscribeFull 9, UnsubscribeFull 5), `cfe_sb_priv.c` L786 (MessageTxn_ReportSingleEvent), `cfe_sb_task.c` L673, L744, L755, L765. Replaced mechanically by `CFE_SB_CORD_APPID()` (`cfe_sb_module_all.h`), which records both USEs at the call site and returns the same value. The count of 39 was re-checked in the ENV source on 2026-10-12 (`R2_gdb_v701.md` §6). |
| NOTE `evs.illegal_appid` / `evs.not_registered` (a = AppID) | `cfe_evs.c` L188 / L193 |
| TCREATE `task:<name>` | `cfe_es_apps.c` `CFE_ES_StartAppTask`, before `OS_TaskCreate` (L664) |
| TSTART `task:<name>` + TASKNAME | `cfe_es_apps.c` `CFE_ES_TaskEntryPoint`, after `CFE_ES_GetTaskFunction` succeeded, in the existing ES lock section (L631-633); name read from the task record that the parent wrote before marking it used |
| SETSTATE `es.appstate:<app>` (a = new state) | `cfe_es_api.c` L399 (ExitApp, STOPPED), L471 (RunLoop, RUNNING), L573 (WaitForSystemState, required state); `cfe_es_apps.c` L1144 (table scan, WAITING). Not instrumented: implicit writes of 0 by `memset` of whole records. |
| WAITRET `es.appstate:<app>` (a = waited-for state, b = read) | `cfe_es_start.c` `CFE_ES_MainTaskSyncDelay`, for every used record in the check that succeeded (L882-887; callers L210, L225, L835). `CFE_ES_WaitForSystemState` got no WAITRET: it waits on `CFE_ES_Global.SystemState`, not on app records. |

Build integration: `cord.c` is added to the `es` library only (`target_sources`) with `-std=c11` for that file (it uses `_Atomic`; under the build's `-std=c99 -pedantic -Werror` it does not compile). `CORD_ENABLE` is a source-file property of exactly the 7 instrumented files, so the coverage-test targets, which copy the module's compile definitions, are built without it.

## 3. Build and configuration comparison (`cord_build_*`)

The bullets below describe the **first build** (`cord_build_*`), used by r2_c1–r2_c3. The second build (`cord_build_v2_*`), used by r2_c4–r2_c6, comes after them.

- prep exit 0 in 10.0 s (6 CMake deprecation warnings, as ENV); install exit 0 in 211.5 s, 0 compiler warnings (ENV: 126.6 s and 794.7 s; host timing is not controlled, B3).
- After path normalization, identical to ENV: `stamp.prep` (the cmake `-D` options), the mission and cpu1 `CMakeCache.txt`, `core-cpu1` `link.txt` and `flags.make`, all 14 host-tool compile commands, and 1344 of 1417 cpu1 compile commands.
- Differences, all instrumentation: 7 commands gained `-DCORD_ENABLE -I…/cfe/cord` (the instrumented files); 66 gained only `-I…/cfe/cord` (other es/sb/evs sources and their coverage targets; none of these got `CORD_ENABLE`); 1 new command (`cord.c`, the target's flags plus `-std=c11`). No `-O` anywhere and `-g` unchanged (ENV C5).
- Outputs: every `.tbl` byte-identical; the 18 loaded app/lib `.so` files have identical `.text`, `.rodata` and `.data`; the rest differs only through source paths in debug info / `__FILE__`. `core-cpu1` 1,555,256 B (ENV 1,531,256 B). `BUILDDATE` differs (202610080657). **In this first build the module version table is `NULL` (no `.git`, §1)**, and the "cFE chksm" in ES event 2 differs, as it must for changed core code.

**Second build (`cord_build_v2_*`, r2_c4–r2_c6).**
- Same steps on the same copy. The two added `sample_defs` files install ENV's version table (§7).
- prep exit 0 in 7.6 s (6 CMake warnings); install exit 0 in 213.7 s, 0 compiler warnings (`cord_build_v2_00_steps.log`).
- `core-cpu1` sha256 `7d618152…` (driver records r2_c4–r2_c6). The first build's `1040d64e…` is kept in `cord_build_v1_core_sha256_before_removal.txt`.
- The configuration comparison (`_08_config_compare`) was **not repeated** for the second build; there is no `cord_build_v2_08_*` record. The equality of the version table with ENV's rests on the record in §7 and on the console, which shows 14 "Version Info" lines in r2_c4–r2_c6, as ENV does.

## 4. Results

§4.1–§4.3 report the **first group, r2_c1–r2_c3** (first build, `NULL` module version table). The second group, r2_c4–r2_c6, is reported in §7. §4.4 and §5 cover both groups.

### 4.1 Per run

| | r2_c1 | r2_c2 | r2_c3 |
|---|---|---|---|
| exit / reset / isolation | 0 / POWER ON / passed | 0 / POWER ON / passed | 0 / POWER ON / passed |
| OPERATIONAL line read (ms after launch) | 1632 | 1632 | 1628 |
| events / threads with events | 626 / 25 | 632 / 25 | 632 / 25 |
| recorder meta: overflow threads, dropped-event lines | 0, none | 0, none | 0, none |
| PUB `sb.appid` (value) / PUB `sb.evsreg` | 1 (1114115) / 1 | 1 (1114115) / 1 | 1 (1114115) / 1 |
| `sb.appid` uses | 233 | 236 | 236 |
| observed_violation | **11** (ES 7, EVS 4) | **11** (ES 7, EVS 4) | **11** (ES 7, EVS 4) |
| predicted_violation | **0** | **0** | **0** |
| ordered | 217 | 220 | 220 |
| same_task (SB itself) | 5 | 5 | 5 |
| `sb.evsreg`: verdict counts | identical to `sb.appid` | identical | identical |
| uses between the two PUBs | 0 | 0 | 0 |
| NOTE `evs.illegal_appid` (a) / `evs.not_registered` | 11 (all a = 0) / 0 | 11 / 0 | 11 / 0 |
| last violating use → PUB `sb.appid` | 100.25 ms | 100.32 ms | 100.32 ms |
| HB edges used: create / state / signal | 24 / 55 / 0 | 24 / 55 / 0 | 24 / 55 / 0 |

### 4.2 The violating uses

Identical in all three runs. Every one passed AppId 0 and was followed on the same thread by NOTE `evs.illegal_appid`, i.e. EVS rejected it (`CFE_EVS_APP_ILLEGAL_APP_ID`) and the event was lost.

| Task | SB function (use site) | Count | gdb (r2_g1–g3) |
|---|---|---|---|
| CFE_ES | `CFE_SB_MessageTxn_ReportSingleEvent` (`cfe_sb_priv.c` L786; SEND_NO_SUBS for ES's own EVS packets) | **4** | 17 |
| CFE_ES | `CFE_SB_SubscribeFull` (L1185, SUBSCRIPTION_RCVD) | 2 | 2 |
| CFE_ES | `CFE_SB_CreatePipe` (L277, PIPE_ADDED) | 1 | 1 |
| CFE_EVS | `CFE_SB_SubscribeFull` | 2 | 2 |
| CFE_EVS | `CFE_SB_CreatePipe` | 1 | 1 |
| CFE_EVS | `CFE_SB_MessageTxn_ReportSingleEvent` | 1 | 1 |
| **total** | | **11** (ES 7, EVS 4) | **24** (ES 20, EVS 4) |

The whole difference is 13 ES `SEND_NO_SUBS` uses. In the gdb traces those 13 come from `CFE_ES_ModSrcVerCallback` (`CFE_Config_IterateAll` → `CFE_ES_GenerateVersionEvents`, `cfe_es_task.c` L466), one per "Version Info: Module …" event. The copy's build has a `NULL` version table (§1), so ES sends none of those events; the console confirms it. The 4 remaining ES events (EID 1, 2, 91 "Mission", 92 "Build") match the 4 remaining ES uses. So the count differs because of the build input (the missing `.git`), not because of the observation method. By code reading, a copy whose version table equals ENV's would give 24. **This was run later, in r2_c4–r2_c6 (§7): 24 per boot in 3 of 3 runs, with the same per-site split as gdb.** (The earlier sentence "That was not run" was true when §4 was written; it was superseded on 2026-10-08 by §7.)

### 4.3 Does the HB model explain the ordered uses?

**Yes, in all three runs: 0 predicted violations.** (The same holds in r2_c4–r2_c6: 229/211/225 ordered uses, 0 predicted, §7. Zero predicted violations is not a "safe" verdict for anything; it only means that this model ordered the uses it saw.) Every one of the 217/220/220 ordered uses is reached by the same chain, which `r2_cord_summary.py` rebuilds from the analyzer's vector clocks:

`SB: PUB sb.appid → (program order) SETSTATE es.appstate:CFE_SB = RUNNING (CFE_ES_WaitForSystemState) → ES main: WAITRET es.appstate:CFE_SB (MainTaskSyncDelay, cfe_es_start.c L835) → TCREATE task:<X> → <X>: TSTART task:<X> → USE`

The use tasks are TBL, TIME and all 15 apps (CF, CI_LAB, CS, DS, FM, HK, HS, LC, MD, MM, SAMPLE_APP, SBN, SC, SCH_LAB, TO_LAB). Edge counts per run: 24 create (all 24 TCREATE/TSTART pairs matched, including the child tasks ES_BG_TASK, TIME_TONE_TASK, TIME_ONEHZ_TASK and FM_CHILD_TASK) and 55 state (15 + 20 + 20 WAITRETs from the three `MainTaskSyncDelay` calls). The SETSTATEs were 20 RUNNING writes: 8 from `CFE_ES_WaitForSystemState` and 12 from `CFE_ES_RunLoop`. The STOPPED and WAITING sites did not run.

Ablation on the same traces (analysis only; reported for r2_c1–r2_c3 only, and no ablation is reported for r2_c4–r2_c6; its output is recorded only in this paragraph, not in a log file under `../env/logs/`): with the SETSTATE/WAITRET events removed, **all** 217/220/220 ordered uses become predicted violations. With TCREATE/TSTART removed, the same happens. So the S1 serialization is visible to the analyzer only through **both** edge kinds together, and both are needed.

What the model does not cover, and these runs did not exercise:
- The system-state wait. `CFE_ES_WaitForSystemState` reads `CFE_ES_Global.SystemState`; there is no SETSTATE/WAITRET for it.
- SB message edges (no SIG/WAIT).
- Threads not created through ES.

A use of SB's AppId by the ES or EVS task *after* the publication would be ordered in cFE only through the system-state wait (CORE_READY). The model would therefore flag it as a predicted violation, a false positive. No such use occurred here, nor in the gdb runs. The SB background-file event sites (`cfe_sb_task.c` L744-765, run by ES_BG_TASK, created before SB) would be flagged too; they run only after SB file-write commands, which were not sent.

### 4.4 Instrumentation overhead

| Build / method | OPERATIONAL line read (ms) | cFE time CORE_READY → OPERATIONAL (ms) |
|---|---|---|
| ENV, no debugger, POWER ON (run04, run06, run07, M1 A2–B3) | 1613–1633 | 103.2–104.6 |
| ENV, PROCESSOR reset (run05) | 1636 | 104.2 |
| ENV, first run after the VM reboot (M1 A1) | 2570 | 1058.9 |
| **CORD copy, first build (r2_c1–c3)** | **1628–1632** | **103.4–103.6** |
| **CORD copy, second build (r2_c4–c6)** | **1629–1635** | **103.7–104.0** |
| ENV under gdb (r2_g1–g3) | 2244–2693 | 453–621 |

The CORD runs of both groups fall inside the ENV range. Any overhead is below the run-to-run spread of these measures. The recorder handled 626–632 events per boot in r2_c1–c3 and 653–689 in r2_c4–c6. ENV was not rerun. The r2_c4–c6 row was added on 2026-10-12 from the existing logs: the `.meta` `operational_line_read_ms`, and the console timestamps of the CORE_READY and OPERATIONAL lines.

## 5. Comparison with `R2_gdb_v701.md`

The three groups differ in binary, observation method or build input. They are compared column by column and never pooled (§8).

| Item | gdb, r2_g1–g3 (unmodified ENV binary) | CORD, r2_c1–c3 (instrumented copy, first build, `NULL` version table) | CORD, r2_c4–c6 (instrumented copy, second build, ENV's version table) |
|---|---|---|---|
| Publication | once, SB task, value 1114115; registration status 0 right after | once, SB task, value 1114115; `sb.evsreg` right after | same as r2_c1–c3 |
| Uses before publication | 24 per run (ES 20, EVS 4), all AppId 0 → `ILLEGAL_APP_ID` | 11 per run (ES 7, EVS 4), all AppId 0 → NOTE `evs.illegal_appid` (11) | 24 per run (ES 20, EVS 4), all AppId 0 → NOTE `evs.illegal_appid` (24) |
| Uses between publication and registration | 0 | 0 | 0 |
| Affected tasks | ES, EVS | ES, EVS | ES, EVS |
| Per-site equality with gdb (task × SB function × statement) | — | equal for every site except the 13 ES module-version events, which are absent from this build (§4.2) | equal at every site: ES 1 / 2 / 17 and EVS 1 / 2 / 1 at `CreatePipe` / `SubscribeFull` / `ReportSingleEvent`, at the same 3 statements. Re-checked from the raw traces on 2026-10-12. |
| Last violating use → publication | 96.9–97.4 ms (under gdb) | 100.25–100.32 ms | 100.28–100.47 ms |
| Ordered uses (incl. SB itself) | 199 / 205 / 205 | 222 / 225 / 225 (TO_LAB 110 vs 80–91; the other tasks differ by 0–10 per task, e.g. SBN 15 in r2_c3 vs 5–6). These are uses after OPERATIONAL; their number depends on how long the process runs before the stop lands, which gdb changes. | 234 / 216 / 230 (229 / 211 / 225 ordered + 5 SB) |
| Same result in all runs | 3 of 3 | 3 of 3 | 3 of 3 |
| OPERATIONAL | 2244–2693 ms | 1628–1632 ms (ENV without debugger: 1613–1636) | 1629–1635 ms |

## 6. Review state table (검토 §8.1)

| State | R2 with CORD, r2_c1–r2_c3 (first build) | R2 with CORD, r2_c4–r2_c6 (second build, ENV's version table) |
|---|---|---|
| install/run failure | no: build exit 0; 3/3 runs exit 0 through the POWERON restart; trace written in 3/3 | no: build exit 0; 3/3 runs exit 0 through the POWERON restart; trace written in 3/3 |
| case path not executed | no: publication (1 per run) and 11 violating uses (at 3 use statements) executed in 3/3 | no: publication (1 per run) and 24 violating uses (at the same 3 use statements) executed in 3/3 |
| events not recorded | no: 0 dropped events, 0 overflow threads; each of the 11 violating USEs is followed by its outcome NOTE. Partial: the 13 version-event uses are absent because their path did not run in this build (missing `.git`), not because recording failed | no: 0 dropped events, 0 overflow threads; each of the 24 violating USEs is followed by its outcome NOTE |
| recorded but not judged | no: every `sb.*` USE has a verdict (no `no_publication_in_run`) | no: every `sb.*` USE has a verdict |
| order violation detected | **yes**: 11 observed violations per run (ES 7, EVS 4), all rejected by EVS; 0 predicted; 217–220 ordered, each with an HB chain | **yes**: 24 observed violations per run (ES 20, EVS 4), all rejected by EVS; 0 predicted; 211–229 ordered, each with an HB chain |

These are the states of a retired tool's earlier observations. They are not results of the new detector (header, §8).

## 7. Copy without `.git`: corrected (runs r2_c4–r2_c6)

**Cause.** Runs r2_c1–r2_c3 used a copy that had no `.git`, because the lead's instructions said to leave it out. Without `.git`, cFE's git-based version script leaves every module version NULL. ES then sends 4 init events instead of 17, so those runs showed 11 violating uses instead of 24.

**Correction (lead's decision, 2026-10-08).** The ENV build is the reference, so this restores equality with it. No new condition was chosen. Option (b) was used:
- `tool/cord/patches/version_table/generate_module_version.cmake` was installed as `sample_defs/generate_module_version.cmake` in the copy. cFE uses that file instead of its git script when it exists ("If this script exists in the mission defs directory (user-supplied) then use that", `cfe/cmake/mission_build.cmake` L130-137).
- The script installs the ENV build's generated table `env_module_version_table.c` byte for byte. Its sha256 is `4615470b…`.
- The copy's earlier build tree was removed; it was our own build output, and its `core-cpu1` sha256 `1040d64e…` is kept in `env/logs/cord_build_v1_core_sha256_before_removal.txt`. The copy was then rebuilt fresh with the same steps (`env/logs/cord_build_v2_*`: prep and install, exit 0, 0 warnings).
- The generated `cfe_module_version_table.c` of the new build has sha256 `4615470b…`, **identical to ENV**. The new `core-cpu1` has sha256 `7d618152…`.
- The only remaining difference from ENV's inputs is these two added `sample_defs` files. Their content reproduces ENV's generated file exactly.
- Re-check on 2026-10-12. The copy is deleted, so its generated file cannot be re-hashed. What can still be checked agrees with the record above:
  - `env_module_version_table.c` in commit `62e05a9` has sha256 `4615470b…`.
  - ENV's `build-native_std/src/cfe_module_version_table.c` still has `4615470b…`.
  - The consoles of r2_c4–r2_c6 show 14 "Version Info" lines, as ENV does; r2_c1–r2_c3 show 1.

**Results of r2_c4–r2_c6** (operational mode, CI ES-restart stop, no debugger, 3 runs):

| Measure | r2_c4 | r2_c5 | r2_c6 |
|---|---|---|---|
| "Version Info" lines on the console | 14 | 14 | 14 (ENV: 14) |
| `sb.appid` uses | 258 | 240 | 254 |
| observed violations (AppId 0 → `evs.illegal_appid`) | **24** (ES 20, EVS 4) | **24** (ES 20, EVS 4) | **24** (ES 20, EVS 4) |
| uses between the two publications | 0 | 0 | 0 |
| predicted violations | 0 | 0 | 0 |
| ordered | 229 | 211 | 225 |
| same task (SB) | 5 | 5 | 5 |
| last violating use → publication | 100.5 ms | 100.3 ms | 100.3 ms |
| OPERATIONAL line read | 1635 ms | 1629 ms | 1634 ms (ENV 1613–1636) |

- **The violating set equals the gdb result** (`R2_gdb_v701.md`): 24 per boot, 20 from ES and 4 from EVS, in 3 of 3 runs on each side. It is equal per site, too: the same task × SB function counts at the same 3 statements. This was re-checked from the raw traces of both groups on 2026-10-12 (`R2_gdb_v701.md` §7).
- Every ordered use is explained by the chain SETSTATE(`es.appstate:CFE_SB`=RUNNING) → WAITRET (ES main) → TCREATE → TSTART of the using task.
- **On this instrumented binary without a debugger, the violating set is the same as on the unmodified binary under gdb.** OPERATIONAL timing stays within the ENV range at this resolution. The two groups differ in binary and observation method. Their agreement is a cross-method observation, not a measurement of the unmodified binary without a debugger, and the runs are not pooled (§8).

## 8. Claims

Revised 2026-10-12. The earlier version of this section said that only 11 uses had been observed and that 24 had not been run. It was written before §7 and was not updated afterwards (`../Architecture/history/review_EXT_20261012.md` §2.3, §8 item P2). This version replaces it.

**Status of the evidence.** CORD is retired (`../tool/RETIRED.md`). Everything below is an earlier observation by a retired tool. It is not a result of the new detector and is not counted as one. How the paper cites it is PI decision D12 (pending; the recommendation is "prior observations only", `../Architecture/spec/PI_DECISIONS.md` D12).

**Experimental groups.** There are three groups, and numbers are never pooled across them.

| Group | Runs | Binary | Observation | Module version table | Violating uses per boot |
|---|---|---|---|---|---|
| G | r2_g1–r2_g3 (`R2_gdb_v701.md`) | unmodified ENV `core-cpu1` | gdb breakpoints, all-stop | ENV's (`git:v7.0.1`, …) | 24 (ES 20, EVS 4) |
| C-init | r2_c1–r2_c3 (§4) | instrumented copy, first build (`1040d64e…`) | CORD recorder, no debugger | all `NULL` (copy without `.git`); differs from ENV | 11 (ES 7, EVS 4) |
| C-restored | r2_c4–r2_c6 (§7) | instrumented copy, second build (`7d618152…`) | CORD recorder, no debugger | ENV's, restored byte for byte (`4615470b…`; `../env/CONDITIONS.md` H-10) | 24 (ES 20, EVS 4) |

The units are those of `R2_gdb_v701.md` §6:
- one defect cause;
- 3 static SB call sites reached before the publication, out of the 39 that pass SB's AppId;
- 6 task × SB-function contexts;
- 24 dynamic occurrences per boot (11 in C-init);
- 3 runs per group.

24 is not 24 independent defects.

**Supported (current scope).**
- [M] **C-restored.** This group used cFS v7.0.1 (cFE `c5fb2b4d`), the ENV run conditions and the instrumented copy whose version table equals ENV's, **without a debugger**. The recorder saw the R2 order violation in 3 of 3 boots (§7):
  - ES and EVS used SB's AppId 24 times per boot (ES 20, EVS 4) before SB stored it.
  - Every such use passed AppId 0 and was followed on the same thread by EVS's `ILLEGAL_APP_ID` rejection, so the event was lost silently.
  - No use fell between the two publications.
- [M] **C-init.** With all module versions `NULL`, the same violation appeared in 3 of 3 boots, with 11 uses per boot (ES 7, EVS 4) (§4). The 13 missing uses are exactly the ES "Version Info: Module …" events, which that build does not send (§4.2). [I] So the per-boot count depends on a build input (the version table), not only on the defect.
- [M] **Agreement across groups.** Re-checked from the raw traces on 2026-10-12 (§5):
  - C-restored equals G at every site: the same task × SB function counts at the same 3 statements.
  - C-init equals G at every site except the 13 version-event occurrences.
  - [I] So the violating set seen under gdb does not come from the debugger's pauses, for this pair of binaries and these runs. This is agreement between two methods on different binaries, not an observation of the unmodified binary without a debugger.
- [M] **The retired HB model.** With program order, task-creation and app-state reads-from edges, CORD's analyzer ordered every post-publication use in all six runs: 217/220/220 in C-init and 229/211/225 in C-restored. It reported 0 predicted violations. In the C-init ablation it needed both the creation and the state edges (§4.3). Zero predicted violations is not a "safe" verdict. The model is not part of the new design (`../tool/RETIRED.md`).
- [M] **Startup cost.** In neither group did the instrumentation measurably delay startup (§4.4):
  - OPERATIONAL was read at 1628–1635 ms (ENV without a debugger: 1613–1636 ms).
  - CORE_READY → OPERATIONAL took 103.4–104.0 ms (ENV: 103.2–104.6 ms).
  - The recorder dropped no events.

**Limits: claims not supported.**
- **The unmodified ENV binary without a debugger.** No in-process observation of it exists. C-restored is a modified binary: the instrumentation patch of §2, two added `sample_defs` files and a different `core-cpu1`. [I] Code reading (S1) predicts 24 per boot for it, but this was not measured.
- **Pooling.** No claim such as "24 in 6 of 6 runs" (G + C-restored) or "in 9 runs" (all groups). The groups differ in binary and observation method; C-init also differs in a build input.
- **Determinism or a fixed order.** Each group is 3 boots from the same persisted state with the same result. Repetition does not prove that the order holds in every execution (`R2_gdb_v701.md` §6). A fixed-order claim needs the static argument with its assumptions, which is not implemented: the generic K-ID contract plus framework rule R19 (`../Architecture/spec/SPEC_FINAL.md` §7.2, App. A.1).
- **Independent defects.** The 24 (or 11) occurrences come from one cause at 3 static sites.
- **Kairos.** Nothing here is a Kairos detection, verdict or certificate. Kairos is not implemented.
- **False positives of the HB model in general.** Uses ordered only by the system-state wait or by SB messages are not modeled (§4.3) and did not occur here.
- **Predictive detection of R2.** Every R2 violation was *observed*. The predicted-violation path and the witness step were not exercised.
- Anything about existing tools, other cases, or overhead beyond startup time (memory, steady state).

## 9. Files

All CORD code below was retired on 2026-10-12 and removed from the tree. Retrieve it with `git show 62e05a9:research/cfs_procrace/<path>` (`../tool/RETIRED.md`).
- Retired code (paths relative to `research/cfs_procrace/`):
  - patch: `tool/cord/patches/cfe_v701_r2.patch` (sha256 `c6a8b1a4…`);
  - version-table hook: `tool/cord/patches/version_table/generate_module_version.cmake` and `env_module_version_table.c` (sha256 `4615470b…`);
  - recorder: `tool/cord/cord.c` and `cord.h`;
  - analyzer: `tool/cord/cord_analyze.py`;
  - summary script: `tool/cord/r2_cord_summary.py`;
  - driver: `env/run_r2_cord.sh`;
  - HB model description: `tool/DESIGN.md` §3.
- Harness changes (still in the tree): `../env/run_cfs.sh` v5, `../env/build_cfs.sh` v4 (`existing`), `../env/lib_record.sh` (`CORD_*` values recorded), `../env/CONDITIONS.md` rows H-9 and H-10.
- Logs (still in the tree):
  - First build: `../env/logs/cord_build_00_copy_manifest.txt`, `cord_build_00_env.log`, `cord_build_00_steps.log`, `cord_build_04_prep.log`, `cord_build_05_install.log`, `cord_build_06_outputs.log`, `cord_build_07_record.log`, `cord_build_08_config_compare.txt`, `cord_build_console.log`.
  - Second build: `cord_build_v1_core_sha256_before_removal.txt`, `cord_build_v2_00_env.log`, `cord_build_v2_00_steps.log`, `cord_build_v2_04_prep.log`, `cord_build_v2_05_install.log`, `cord_build_v2_06_outputs.log`, `cord_build_v2_07_record.log`, `cord_build_v2_console.log`.
  - Runs: `r2_c{1,…,6}_*`, including the raw traces `r2_c*_cord_trace.jsonl`.
- Trees, deleted on 2026-10-12 (`../env/HOST_CLEANUP_20261012.md`):
  - `/home/user/work/procrace/cfs_cord/cFS` (copy + build, `ubuntu`);
  - `/home/user/work/procrace/cord_runs` (raw traces, `ubuntu`; copies are in `../env/logs/`).

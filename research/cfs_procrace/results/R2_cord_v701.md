# R2 (cFE #2663) on a CORD-instrumented copy of the ENV build: observation without a debugger

- Date: 2026-10-08. Policy: `../CONDITIONS_POLICY.md`.
- **What this is.** Case R2 observed in real cFS v7.0.1 (cFE `c5fb2b4d`) **without gdb**, by the CORD event recorder compiled into `core-cpu1`, and judged by `../tool/cord/cord_analyze.py` (HB model of `../tool/DESIGN.md` §3).
  - Build: a copy of the ENV source with a small instrumentation patch (`../tool/cord/patches/cfe_v701_r2.patch`), built and run under the ENV conditions.
  - Runs: `r2_c1`, `r2_c2`, `r2_c3` (`../env/run_r2_cord.sh`, `../env/run_cfs.sh` v5).
- **What this is not.**
  - Not a run of the unmodified ENV binary (that is `R2_gdb_v701.md`, under gdb).
  - Not a witness/delay experiment: `CORD_DELAY` was not set; no predicted violation existed to witness.
  - Not a comparison with existing tools.
- Logs: `../env/logs/cord_build_*` (copy manifest, build, configuration comparison) and `../env/logs/r2_c{1,2,3}_*` (console, `.meta`, persisted-state snapshots, driver record, raw trace `_cord_trace.jsonl`, analyzer output `_cord_sb.{txt,json}` and `_cord_all.txt`, R2 summary `_cord_summary.{txt,json}` from `../tool/cord/r2_cord_summary.py`).

## 1. Conditions

| Item | Value | Status |
|---|---|---|
| Source | `/home/user/work/procrace/cfs_cord/cFS`: copy of the ENV tree (`cfs_ref/cFS`, bundle `088b2fa8`, cFE `c5fb2b4d`) without `.git` and `build-native_std`. Before patching, all 4316 entries (files with sha256 + mode, symlinks with target) and the directory list were identical (`cord_build_00_copy_manifest.txt`, manifest sha256 `64267a8f…`). Owned by `ubuntu`, as the ENV tree (ENV D-1). | from-reference (ENV A1–A5) for the content |
| Copy without `.git` | As instructed. **Not neutral:** cFE generates its module version table at build time with `git describe` (`cfe/cmake/generate_git_module_version.cmake` L35-52, selected by `cfe/cmake/mission_build.cmake` L130-138). Without `.git` every entry is `NULL` (ENV: `git:v7.0.1`; sample_lib `git:v7.0.0-10-g2d8b7e8`). ES then sends 4 instead of 17 init events: the 13 "Version Info: Module …" events are missing (console: 1 "Version Info" line vs 14 in ENV run06 and in r2_g1). This changes the R2 count (§4.2). | **deviation** (copy method). Options in §7, **PI decision pending** |
| Copy tool | GNU tar 1.35 with the same excludes; `rsync` is not installed and was not installed. Equality is proven by the manifest. | harness (no effect on content) |
| Instrumentation | `../tool/cord/patches/cfe_v701_r2.patch` (§2) | tool, not a reference condition |
| Build | `build_cfs.sh` v4 `existing`: `make native_std.prep`, `make native_std.install` (README L104-105), as `ubuntu` via `runuser`, same environment refusal list and records as the ENV build (ENV C2, C3, C4, C11). Logs `cord_build_00_env.log`, `_00_steps`, `_04_prep`, `_05_install`, `_06_outputs`, `_07_record`. | from-reference |
| Build configuration | Identical to ENV except the instrumentation (§3; `cord_build_08_config_compare.txt`) | from-reference + instrumentation |
| Run user, cwd, command | `ubuntu` via `setpriv`, `build-native_std/exe/cpu1` of the copy, `./core-cpu1` without options (ENV E1–E3) | from-reference |
| Run length, stop | E10 `operational` (README L114 line, event-based), E11 (a) `ci-es-restart-poweron` | E10 unspecified_by_reference (as ENV); **stop method still PI decision D3** |
| Queue depth | README mode, `msg_max` 10 (ENV E5) | from-reference (README L102); **D2 pending** |
| Process environment | ENV E12 plus `CORD_OUT=/home/user/work/procrace/cord_runs/<tag>.jsonl` (directory owned by `ubuntu`). cFS does not read it. `CORD_DELAY`, `CORD_BUF`, `CORD_DUMP_AFTER_MS` unset. | **deviation** from E12, instrumented build only (H-9) |
| Recorder settings | Default buffer 65536 events per thread (`cord.c`); trace written at exit (atexit) | tool default (unspecified_by_reference) |
| Recorder side effects | Constructor installs handlers for SIGSEGV/SIGBUS/SIGABRT/SIGTERM/SIGINT/SIGFPE (the PSP later replaces SIGINT, SIGTERM, SIGFPE: `cfe_psp_exception.c` L232-233, L266); per-thread buffers of 65536 × 120 B are `calloc`ed lazily (≈7.5 MiB virtual per thread; RSS not measured) | instrumentation effect (recorded) |
| Isolation | Passed before every run (ENV E13); one cFS process at a time | from-reference (harness rule H-5) |
| Persisted state | Copy's `exe/cpu1` had no `EEPROM.DAT` and no keyfiles before r2_c1 (created by r2_c1; ENV runs had their own). `/dev/shm/osal:RAM/cfe_sb_pipe.dat` (1624 B, from M1 B3) present in all runs, shared with the ENV runs. No SysV segment before/after. All boots POWER ON. | recorded (not cleaned, policy) |
| Runs | 3, sequential (07:02:30Z, 07:04:09Z, 07:04:13Z) | count unspecified_by_reference (assumption, same as the gdb runs) |

## 2. Instrumentation (patch summary)

`../tool/cord/patches/cfe_v701_r2.patch` (sha256 `c6a8b1a4…`) is the complete `diff -ruN` between the ENV source and the copy: 14 files, 2 new (`cfe/cord/cord.c`, `cord.h`, verbatim copies of `../tool/cord/`, same sha256). Its header lists every site with original and patched line numbers. Summary:

| Event | Site (original line, cFE `c5fb2b4d`) |
|---|---|
| PUB `sb.appid` (a = stored AppId) | `cfe_sb_task.c` L138, right after `CFE_ES_GetAppID(&CFE_SB_Global.AppId);` |
| PUB `sb.evsreg` | `cfe_sb_task.c` after the successful `CFE_EVS_Register` check (L155-160) |
| USE `sb.appid` + USE `sb.evsreg` (a = AppId read) | all **39** SB call sites that pass `CFE_SB_Global.AppId` to `CFE_EVS_SendEventWithAppID`: `cfe_sb_api.c` 34 (CreatePipe 6, DeletePipeFull 3, SetPipeOpts 3, GetPipeOpts 3, GetPipeName 2, GetPipeIdByName 3, SubscribeFull 9, UnsubscribeFull 5), `cfe_sb_priv.c` L786 (MessageTxn_ReportSingleEvent), `cfe_sb_task.c` L673, L744, L755, L765. Replaced mechanically by `CFE_SB_CORD_APPID()` (`cfe_sb_module_all.h`), which records both USEs at the call site and returns the same value. |
| NOTE `evs.illegal_appid` / `evs.not_registered` (a = AppID) | `cfe_evs.c` L188 / L193 |
| TCREATE `task:<name>` | `cfe_es_apps.c` `CFE_ES_StartAppTask`, before `OS_TaskCreate` (L664) |
| TSTART `task:<name>` + TASKNAME | `cfe_es_apps.c` `CFE_ES_TaskEntryPoint`, after `CFE_ES_GetTaskFunction` succeeded, in the existing ES lock section (L631-633); name read from the task record that the parent wrote before marking it used |
| SETSTATE `es.appstate:<app>` (a = new state) | `cfe_es_api.c` L399 (ExitApp, STOPPED), L471 (RunLoop, RUNNING), L573 (WaitForSystemState, required state); `cfe_es_apps.c` L1144 (table scan, WAITING). Not instrumented: implicit writes of 0 by `memset` of whole records. |
| WAITRET `es.appstate:<app>` (a = waited-for state, b = read) | `cfe_es_start.c` `CFE_ES_MainTaskSyncDelay`, for every used record in the check that succeeded (L882-887; callers L210, L225, L835). `CFE_ES_WaitForSystemState` got no WAITRET: it waits on `CFE_ES_Global.SystemState`, not on app records. |

Build integration: `cord.c` is added to the `es` library only (`target_sources`) with `-std=c11` for that file (it uses `_Atomic`; under the build's `-std=c99 -pedantic -Werror` it does not compile). `CORD_ENABLE` is a source-file property of exactly the 7 instrumented files, so the coverage-test targets, which copy the module's compile definitions, are built without it.

## 3. Build and configuration comparison (`cord_build_*`)

- prep exit 0 in 10.0 s (6 CMake deprecation warnings, as ENV); install exit 0 in 211.5 s, 0 compiler warnings (ENV: 126.6 s and 794.7 s; host timing is not controlled, B3).
- After path normalization, identical to ENV: `stamp.prep` (the cmake `-D` options), the mission and cpu1 `CMakeCache.txt`, `core-cpu1` `link.txt` and `flags.make`, all 14 host-tool compile commands, and 1344 of 1417 cpu1 compile commands.
- Differences, all instrumentation: 7 commands gained `-DCORD_ENABLE -I…/cfe/cord` (the instrumented files); 66 gained only `-I…/cfe/cord` (other es/sb/evs sources and their coverage targets; none of these got `CORD_ENABLE`); 1 new command (`cord.c`, the target's flags plus `-std=c11`). No `-O` anywhere and `-g` unchanged (ENV C5).
- Outputs: every `.tbl` byte-identical; the 18 loaded app/lib `.so` files have identical `.text`, `.rodata` and `.data`; the rest differs only through source paths in debug info / `__FILE__`. `core-cpu1` 1,555,256 B (ENV 1,531,256 B). `BUILDDATE` differs (202610080657). **The module version table is `NULL` (no `.git`, §1)**, and the "cFE chksm" in ES event 2 differs, as it must for changed core code.

## 4. Results

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

The whole difference is 13 ES `SEND_NO_SUBS` uses. In the gdb traces those 13 come from `CFE_ES_ModSrcVerCallback` (`CFE_Config_IterateAll` → `CFE_ES_GenerateVersionEvents`, `cfe_es_task.c` L466), one per "Version Info: Module …" event. The copy's build has a `NULL` version table (§1), so ES sends none of those events; the console confirms it. The 4 remaining ES events (EID 1, 2, 91 "Mission", 92 "Build") match the 4 remaining ES uses. So the count differs because of the build input (the missing `.git`), not because of the observation method. By code reading, a copy whose version table equals ENV's would give 24. **That was not run.**

### 4.3 Does the HB model explain the ordered uses?

**Yes, in all three runs: 0 predicted violations.** Every one of the 217/220/220 ordered uses is reached by the same chain, which `r2_cord_summary.py` rebuilds from the analyzer's vector clocks:

`SB: PUB sb.appid → (program order) SETSTATE es.appstate:CFE_SB = RUNNING (CFE_ES_WaitForSystemState) → ES main: WAITRET es.appstate:CFE_SB (MainTaskSyncDelay, cfe_es_start.c L835) → TCREATE task:<X> → <X>: TSTART task:<X> → USE`

The use tasks are TBL, TIME and all 15 apps (CF, CI_LAB, CS, DS, FM, HK, HS, LC, MD, MM, SAMPLE_APP, SBN, SC, SCH_LAB, TO_LAB). Edge counts per run: 24 create (all 24 TCREATE/TSTART pairs matched, including the child tasks ES_BG_TASK, TIME_TONE_TASK, TIME_ONEHZ_TASK and FM_CHILD_TASK) and 55 state (15 + 20 + 20 WAITRETs from the three `MainTaskSyncDelay` calls). The SETSTATEs were 20 RUNNING writes: 8 from `CFE_ES_WaitForSystemState` and 12 from `CFE_ES_RunLoop`. The STOPPED and WAITING sites did not run.

Ablation on the same traces (analysis only): with the SETSTATE/WAITRET events removed, **all** 217/220/220 ordered uses become predicted violations. With TCREATE/TSTART removed, the same happens. So the S1 serialization is visible to the analyzer only through **both** edge kinds together, and both are needed.

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
| **CORD copy (r2_c1–c3)** | **1628–1632** | **103.4–103.6** |
| ENV under gdb (r2_g1–g3) | 2244–2693 | 453–621 |

The CORD runs fall inside the ENV range. Any overhead is below the run-to-run spread of these measures. The recorder handled 626–632 events per boot. ENV was not rerun.

## 5. Comparison with `R2_gdb_v701.md`

| Item | gdb (unmodified ENV binary) | CORD (instrumented copy) |
|---|---|---|
| Publication | once, SB task, value 1114115; registration status 0 right after | once, SB task, value 1114115; `sb.evsreg` right after |
| Uses before publication | 24 per run (ES 20, EVS 4), all AppId 0 → `ILLEGAL_APP_ID` | 11 per run (ES 7, EVS 4), all AppId 0 → NOTE `evs.illegal_appid` (11) |
| Uses between publication and registration | 0 | 0 |
| Affected tasks | ES, EVS | ES, EVS |
| Per-site equality | — | equal for every site except the 13 ES module-version events, which are absent from this build (§4.2) |
| Last violating use → publication | 96.9–97.4 ms (under gdb) | 100.25–100.32 ms |
| Ordered uses (incl. SB itself) | 199 / 205 / 205 | 222 / 225 / 225 (TO_LAB 110 vs 80–91; the other tasks differ by 0–10 per task, e.g. SBN 15 in r2_c3 vs 5–6). These are uses after OPERATIONAL; their number depends on how long the process runs before the stop lands, which gdb changes. |
| Same result in all runs | 3 of 3 | 3 of 3 |
| OPERATIONAL | 2244–2693 ms | 1628–1632 ms (ENV without debugger: 1613–1636) |

## 6. Review state table (검토 §8.1)

| State | R2 with CORD on the instrumented copy |
|---|---|
| install/run failure | no: build exit 0; 3/3 runs exit 0 through the POWERON restart; trace written in 3/3 |
| case path not executed | no: publication (1 per run) and 11 use sites executed in 3/3 |
| events not recorded | no: 0 dropped events, 0 overflow threads; each of the 11 violating USEs is followed by its outcome NOTE. Partial: the 13 version-event uses are absent because their path did not run in this build (missing `.git`), not because recording failed |
| recorded but not judged | no: every `sb.*` USE has a verdict (no `no_publication_in_run`) |
| order violation detected | **yes**: 11 observed violations per run (ES 7, EVS 4), all rejected by EVS; 0 predicted; 217–220 ordered, each with an HB chain |

## 7. Open decision (copy without `.git`)

The copy is not equal to ENV in one build input: the generated version table. Options, for the PI:

- **(a)** Copy the `.git` metadata too (about 265 MB; disk after this build: 1.1 GB free). `git describe --dirty` would then report `-dirty` for repositories with patched files, so the strings would still differ from ENV, but all 14 "Version Info" events would be sent.
- **(b)** Supply `sample_defs/generate_module_version.cmake` (a hook documented in `cfe/cmake/mission_build.cmake` L130-138) that writes ENV's exact values (`build-native_std/src/cfe_module_version_table.c` of ENV). It reproduces the ENV table, but adds a file to `sample_defs`, which ENV A5 uses as shipped.
- **(c)** Keep the copy as is, and report R2 counts as "build without version metadata" (this document).

Nothing was changed on the PI's behalf.

## 8. Claims

Supported:
- On cFS v7.0.1 (cFE `c5fb2b4d`) with the ENV build and run conditions, **without a debugger**, a recorder inside the process observed the R2 order violation in 3 of 3 boots: ES and EVS used SB's AppId (still 0) before SB stored it, and EVS rejected every such use silently (11 per boot in this build).
- The violating set is the same as under gdb, site by site, except for the ES version events that this build does not send. The gdb result is therefore not an artifact of the debugger's pauses, for the sites both builds execute.
- With program order, task-creation and app-state reads-from edges, the analyzer ordered all 217–220 later uses and reported no false predicted violation in these runs. It needed both the creation and the state edges to do so.
- The instrumentation did not measurably delay startup (OPERATIONAL within the ENV range), and it recorded every event with no drops.

Not supported:
- "24 violations without a debugger." This build gives 11. 24 is expected only by code reading for a build with ENV's version table (§4.2, §7).
- That R2 is deterministic. The evidence is 3 runs from the same persisted state, with the same result each time.
- That the model is free of false positives in general. Uses ordered only by the system-state wait or by SB messages are not modeled (§4.3) and did not occur here.
- Predictive detection of R2. Every R2 violation was *observed*; the predicted-violation path and the witness step were not exercised.
- Anything about existing tools, other cases, or overhead beyond startup time (memory, steady state).

## 9. Files

- Patch: `../tool/cord/patches/cfe_v701_r2.patch`. Summary script: `../tool/cord/r2_cord_summary.py`. Driver: `../env/run_r2_cord.sh`.
- Harness changes: `../env/run_cfs.sh` v5, `../env/build_cfs.sh` v4 (`existing`), `../env/lib_record.sh` (`CORD_*` values recorded), `../env/CONDITIONS.md` row H-9.
- Logs: `../env/logs/cord_build_00_copy_manifest.txt`, `cord_build_00_env.log`, `cord_build_00_steps.log`, `cord_build_04_prep.log`, `cord_build_05_install.log`, `cord_build_06_outputs.log`, `cord_build_07_record.log`, `cord_build_08_config_compare.txt`, `cord_build_console.log`, `r2_c{1,2,3}_*`.
- Trees: `/home/user/work/procrace/cfs_cord/cFS` (copy + build, `ubuntu`), `/home/user/work/procrace/cord_runs` (raw traces, `ubuntu`).

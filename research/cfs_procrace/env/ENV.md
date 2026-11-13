# cFS native reference build: what was done and what was observed

Date: 2026-10-08 (UTC). Policy: `../CONDITIONS_POLICY.md`. Every condition and its reference is in `CONDITIONS.md`; IDs such as A2 or D-3 below point to rows there. Logs are in `logs/`. This file was revised the same day after an audit; §9 lists the audit findings and what was done with each.

## Summary

- **Bundle:** `nasa/cFS` `088b2fa828db9ff7e00733f1908e0eeb59f66ce3`, which is `main` HEAD and tag `v7.0.1` (commit date 2026-05-13T08:34:06-04:00). The 25 submodule commits are listed in `CONDITIONS.md`. There are no nested submodules (A3).
- **Build:** README L102-105 exactly: `make native_std.prep`, then `make native_std.install`, as a normal user, with the bundle's own `Makefile` and `sample_defs/` and no edits. Both steps exited 0 with 0 compiler warnings. The code is compiled with `-g` and no `-O` option, i.e. at `-O0` (C5).
- **Unit tests (README L106):** run as the normal user. **Failed:** `network-api-test` cannot open an IPv6 socket because the VM kernel has no IPv6. 68 tests passed before it, and make stopped, so 129 of 198 did not run (C12). Coverage (README L107) was not run because `lcov` is missing and the tests did not pass (D-2).
- **CF:** built, staged, loaded and initialised as part of the documented default configuration. Nothing was added or changed. **Case C1's code path is not executed with the default CF table**, because both channels have an empty throttle-semaphore name (D7).
- **Runs:** seven runs are recorded. All reached "CFE_ES_Main entering OPERATIONAL state" about 1.6 s after launch, all 15 script apps loaded, and CF printed "CF Initialized. Version 7.0.1.0". run01-run03 used the first harness (v1); run04-run07 use the revised harness (v2), which detects the OPERATIONAL line by an event, refuses to start when another cFS/OSAL activity is present, records the credentials, limits and environment of the `core-cpu1` process, and requires an explicit stop method. Both referenced stop methods work here: SIGINT (CTRL+C) exits with PROCESSOR reset status, and the bundle CI's ES restart command exits with POWERON status, after which the next boot is a POWER ON reset.
- **Container effects**, each recorded with its reference and none worked around:
  - task priorities are best effort and queue depths are truncated to 10, as README L102 documents for a normal user (E4, E5);
  - `/proc/schedstat` is missing, so the HS system monitor fails (D-3);
  - no IPv6, so one OSAL unit test fails (C12);
  - root lacks `CAP_SYS_RESOURCE`, so a root run would fail to create its queues (D-4);
  - the clone was made by root and handed to the normal user; this was a convenience, not a necessity (D-1).
- **Open decisions for the PI** (§8): bundle branch (A2); queue-depth mode, README `msg_max` 10 versus the bundle CI's 64 (E5); stop method and reset policy (E7, E11); the failed unit tests (C12); where C1's semaphore conditions come from (D7).

## 1. Archive of the earlier non-reference attempt (task step 1)

An earlier, interrupted run of this workflow had already archived the attempt at 2026-10-07T07:27Z (UTC), before this session started:

- `/home/user/work/procrace/cfs/` no longer exists. Its `src` tree was deleted to free disk.
- `/home/user/work/procrace/cfs_nonref_attempt/` holds the record:
  - `logs/`: `01_prep.log`, `01_prep_FAILED_no_MISSIONCONFIG.log`, `02_make.log`, `03_install.log`
  - `patches/cf_integration.diff`
  - `runs/run01_root_plain.log`
  - `src_state_at_deletion.txt`: bundle HEAD `13ebc15` on `dev`, `git status`, submodule list and diffstat
  - `src_worktree_diff_at_deletion.diff`
- `src_state_at_deletion.txt` confirms the non-reference choices: `sample_defs/` was replaced by `cfe/cmake/sample_defs` (37 files changed) and `Makefile` was modified.
- `runs/run01_root_plain.log` shows that the attempt ran as root with RT scheduling ("Selected policy 2 for RT tasks", L7, no `EPERM`) and failed with `OS_QueueCreate Error. errno = 22 (Invalid argument)` (L41, L49). **Corrected cause:** OSAL reads `msg_max` only when `geteuid() != 0` (`osal/src/bsp/generic-linux/src/bsp_start.c` L66-78), so as root it truncates nothing, permissive mode or not. `mq_open` with a depth above `msg_max` (10 here) then needs `CAP_SYS_RESOURCE`, which this container removes even from root. The earlier text blamed the missing permissive mode of the replacement `sample_defs`; that was not the mechanism (D-4, `logs/f_root_capability_probe.log`).

None of the attempt's choices were reused here.

The interrupted run had also left a plain clone (branch `dev` at `13ebc15`, submodules not initialised) at `/home/user/work/procrace/cfs_ref/cFS`. It was deleted at 2026-10-08T01:09Z and replaced by a fresh clone at the pinned commit (§2).

## 2. Source (task step 2)

`build_cfs.sh` (v1) did the following, with output in `logs/build_00_steps.log`, `build_01_clone.log`, `build_02_submodules.log` and `build_03_pinned_state.log`:

```
git clone https://github.com/nasa/cFS.git                         # README L91
git checkout --detach 088b2fa828db9ff7e00733f1908e0eeb59f66ce3    # A2
git submodule init && git submodule update                        # README L93-94 (152 s)
```

- The checked-out state matches the pinned list, and the working tree is clean.
- `git describe --tags` gives `v7.0.1`.
- Remote refs at 2026-10-08T01:31:52Z (`logs/build_01b_remote_refs.log`): `HEAD -> dev` at `01e8416ca001fbbfff8beadb988d628c8bbc88b3`; `main` and `v7.0.1` at `088b2fa828db9ff7e00733f1908e0eeb59f66ce3`. `dev` is the v7.0.2rc1 release candidate and drops MM from the default targets (A2).
- **Nested submodules: none.** `apps/hk/.gitmodules` names `unit-test/ut_utils_lib`, but HK at `766dab1` has no gitlink for it, and no other submodule has a gitlink either (`logs/build_07_record_existing.log`). A recursive clone would give the same tree (A3). The v1 log line "nested .gitmodules inside submodules (non-empty means nested submodules exist): apps/hk/.gitmodules" (`build_03_pinned_state.log` L29-30) used the wrong test; v3 checks gitlinks.
- **Disk:**

  | Item | Size |
  |---|---|
  | Clone including `.git` | 328 MB (`.git` 265 MB) |
  | `build-native_std` | 454 MB (`exe` 108 MB), including the README L106 test results |
  | Total | 782 MB, within the 1.2 GB budget |

  The root filesystem had 2.4 GB free afterwards.

## 3. Documents read (task steps 3 and 4)

**Bundle `README.md` at the pinned commit (followed):**

- L89: "Ensure the following software are installed: Make, CMake, GCC, and Git."
- L91-94: `git clone https://github.com/nasa/cFS.git` / `cd cFS` / `git submodule init` / `git submodule update`
- L96: "The default Makefile and sample_defs/ directory should already be located at the top-level of this repository with the default configurations for building the open source applications." Both are present, so nothing was copied.
- L102: "To prep, compile, and run on the host (from cFS directory above) as a normal user (best effort message queue depth and task priorities):"
- L104-107: `make native_std.prep` / `make native_std.install` / `make native_std.runtest` / `make native_std.lcov`. L104-106 were run (C2, C12); L107 was not (D-2).
- L109-112: "In order to boot CFE, the default linux PSP requires that the working directory be set to the location of the staged binaries:" / `cd build-native_std/exe/cpu1/` / `./core-cpu1`
- L114: "Should see startup messages, and CFE_ES_Main entering OPERATIONAL state."
- L142-143: `runtest` "Executes all the unit tests" (`-j` recommended, not required); `lcov` "Requires/depends on "runtest" completing and passing".
- L184: an app "must be added to targets.cmake to be compiled and to the .scr file to be dynamically loaded at runtime".

**`cfe/cmake/README.md`:** describes the generic single-configuration wrapper (`make SIMULATION=native prep`, `make install`). In the bundle that wrapper is `simple.mk -> cfe/cmake/Makefile.sample` with `simple_defs -> cfe/cmake/sample_defs`. The bundle README (L96) names its own top-level `Makefile` and `sample_defs/` as the default, so those were used. `cfe/README.md` L7 points back to the bundle for build and run instructions.

**The bundle's own CI at the pinned commit** (read after the audit; it is the bundle's tested way to run cFS):

- `.github/workflows/test-cfs-qemu.yml`: builds `native_std` and runs it in the `cfsexec-linux` image (matrix L129-133), started by `actions/start-cfs-container` (L172-177) with `--sysctl fs.mqueue.msg_max=64` (`actions/start-cfs-container/action.yml` L29-34). It waits for the OPERATIONAL line by polling `docker logs` every 2 s up to 30 times (`actions/healthcheck-logs/action.yml` L27-34), sends an ES no-op with `cmd_send` to ES MID `0x1806` (L226, L236-239), and finally runs `docker stop` (L289-293, `actions/stop-cfs-container/action.yml` L12-15). L262-268 list two known issues that it filters out: "the HS system monitor may not initialize depending on platform support" and "SBN does not service the SBNSubPipe well enough".
- `.github/workflows/build-run-app-reusable.yml` (the workflow app repositories use): same container start (L163-168), and the shutdown "Shut down CFE (STD)" at L209-214: `./cmd_send -v --host=... --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002`, i.e. an ES Restart command with RestartType POWERON.

These give a second reference for the stop method (E11) and a conflicting reference for the queue-depth mode (E5).

**CF documentation** (`apps/cf/README.md`; `apps/cf/docs/dox_src/cfs_cf.dox`, Deployment Guide L481-538 and Constraints L1406-1466):

- Configuration: a wakeup from SCH at a fixed rate (L494-496); per-channel flow control by an optional semaphore that, "by default", TO provides (L505-515); a stack of at least 16384 bytes (L1418-1421).
- Integration (L517-536): Software Bus (per-channel input and output MIDs), Scheduler (`ticks_per_second` must match SCH), Endianness (nothing to configure), and **"Integration with TO": "TO's pipe needs to be able to receive packets of #CF_MAX_PDU_SIZE"** (L534-536). This last item was missing from the first version of this file; it is now row D8 and is met for message size.
- No separate procedure for adding CF to a bundle.

The bundle's `sample_defs` already include CF:

- `targets.cmake` L91 (`MISSION_GLOBAL_APPLIST cf`)
- `generate_startup.cmake` L15, L21 (startup entry)
- `cpu1/install_custom.cmake` L13, L46-47
- `tables/sch_lab_table.c` L67-68 (wakeup at 10 Hz, HK every 5.6 s)
- `tables/to_lab_sub.c` L79-82 (CF HK, EOT, and both channels' PDU output)

CF is therefore part of the documented default configuration and no deviation is proposed (D1-D8). Being part of the configuration does not make case C1 occur (D7).

**OSAL and PSP sources** were read for the normal-user behaviour, the console stop keys, the ES restart path, the reset-type logic and the host resources they share (E4, E5, E7, E11, E12, E13, D-4). They were not used to choose any setting.

## 4. Build (task step 5, first half)

`build_cfs.sh` was started as root with `CFS_NORMAL_USER=ubuntu`, so prep, install and runtest ran through `runuser -u ubuntu` after the tree was handed over (D-1).

| Step | Command | Wall time | Exit | Warnings | Errors | Log | Script |
|---|---|---|---|---|---|---|---|
| clone + pin | `git clone ...`, `git checkout --detach 088b2fa8` | 5 s | 0 | – | – | `build_01_clone.log` | v1 |
| submodules | `git submodule init && git submodule update` | 152.0 s | 0 | – | – | `build_02_submodules.log` | v1 |
| prep | `make native_std.prep` | 126.6 s | 0 | 6 CMake deprecation warnings (`apps/sbn`, `sbn_udp`, `sbn_f_remap` `CMakeLists.txt:1`, "Compatibility with CMake < 3.5 will be removed") | 0 | `build_04_prep.log` | v1 |
| install | `make native_std.install` | 794.7 s | 0 | 0 compiler warnings (`-Wall -Werror` in effect) | 0 | `build_05_install.log` | v1 |
| outputs | find/cat/du | – | 0 | – | – | `build_06_outputs.log` | v2 (block re-run by hand) |
| record | effective settings of the existing build | – | 0 | – | – | `build_07_record_existing.log` | v3 |
| runtest | `make native_std.runtest` | 60.8 s | 2 | 0 | `network-api-test` failed (C12) | `build_08_runtest.log`, `build_08_runtest_host_state.log`, `build_08_runtest_env.log` | v3 |

**Which script produced which log.** `build_cfs.sh` changed three times, and the first build was not repeated:

- **v1** (2026-10-08T01:10Z) produced `build_00_env.log`, `build_01_clone.log`, `build_02_submodules.log`, `build_03_pinned_state.log`, `build_04_prep.log`, `build_05_install.log`, `build_06_outputs_FAILED_wrong_scr_path.log`, `build_console.log` and lines 1-10 of `build_00_steps.log`.
- **v2** was edited after v1 had run. All its edits, including the two that the first version of this file did not disclose:
  1. the output record read `cpu1/cfe_es_startup.scr`; the generated file is `cpu1/cf/cfe_es_startup.scr` (disclosed). The block was re-run by hand to produce `build_06_outputs.log`.
  2. the warning/error counts used plain `grep warning`/`error` and counted file names such as `osapi-error-stubs.c`; v2 uses compiler-specific patterns (disclosed). v1's summary line read "warnings N, 'error' lines N"; v2's reads "compiler/CMake warnings N, compiler/make errors N" (**not disclosed before**; `build_00_steps.log` L8 and L10 are v1 output).
  3. the step label "(C-REV)" became "(CONDITIONS.md A2)" (**not disclosed before**; `build_00_steps.log` L2 is v1 output).
  Line 11 of `build_00_steps.log` is a hand-written note about edits 1 and 2.
- **v3** (after the audit) adds subcommands (`build`, `runtest`, `record`), the extended environment refusal list and environment dump (C4, H-6), the gitlink check for nested submodules (A3), README L106 (C12), and a `record` step for an existing build. Its log lines in `build_00_steps.log` carry the tag `[build_cfs.sh v3 ...]`. The first `runtest` invocation stopped inside the harness before make ran (`lsof` exits 1 when no UDP socket exists, which `pipefail` turned into an error); the snapshot function was fixed and the step re-run, overwriting the two partial logs.

**Why the build was not repeated with v3.** None of the audit findings changes a build input: the compile flags were mis-recorded, not mis-set (C5), and the variables the build reads were empty (C4). The effective settings are recorded from the artifacts in `build_07_record_existing.log`, and v3's source checks were re-run there. A fresh `build` would first delete the clone, which other workflows read (`../baselines/*.md` cite it), and would take about 18 minutes. So the full `build` subcommand of v3 has **not** been executed end to end; its clone, pin, submodule, prep, install and output steps are the v1/v2 code with v3's checks added.

Notes:

- **Effective CMake call** (`build_04_prep.log` L5; L4 is the `mkdir -p`):
  `cmake -DENABLE_UNIT_TESTS=TRUE -DSIMULATION=native -DCFE_EDS_ENABLED=OFF -DMISSIONCONFIG=sample -DCMAKE_BUILD_TYPE=debug -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DCMAKE_BUILD_TYPE=debug -S ".../cFS/cfe" -DCMAKE_INSTALL_PREFIX=/exe -B "build-native_std"`
- **Effective compiler options** (C5, from `compile_commands.json`): FSW `-g [-fPIC] -Wstrict-prototypes -Wwrite-strings -Wpointer-arith -Wno-format-overflow -Wno-format-truncation -Wno-stringop-overflow -Wno-stringop-truncation -std=c99 -pedantic -Wall -Werror`, no `-O`. `sample_defs/arch_build_custom_native.cmake` (`-Wcast-align=strict -fno-common`) ships with the bundle but is never included, because `TARGETSYSTEM` is `native_default_cpu1`.
- **Build timings are not controlled.** The host was shared with other workloads (load average about 3), and the build was serial because the README uses no `-j`.
- **Build environment:** `runuser` applies PAM (`pam_limits`), which set the open-file soft limit to 1024 for the build user, while the run user (via `setpriv`) has 20000 (C11). Neither affects the build output.
- **Outputs** (`build_06_outputs.log`): `exe/cpu1/core-cpu1`, `exe/cpu1/cf/*.so`, `exe/cpu1/cf/*.tbl`, the generated `exe/cpu1/cf/cfe_es_startup.scr`, the same set for `cpu2`, host tools in `exe/host` (`elf2cfetbl`, `cmd_send`, `tlm_recv`, `cfe_ts_crc`), and staged unit-test executables.
- **Build identity printed at runtime:** "Build 202610080115 by ubuntu@vm, config sample", "cFE chksm 63092", and all modules "git:v7.0.1".

Generated startup script (`exe/cpu1/cf/cfe_es_startup.scr`), in the order ES processes it. CF is line 8, the 6th `CFE_APP` entry:

```
CFE_LIB, cfe_assert,  CFE_Assert_LibInit, ASSERT_LIB,    0,   0,     0x0, 0;
CFE_LIB, sample_lib,  SAMPLE_LIB_Init,    SAMPLE_LIB,    0,   0,     0x0, 0;
CFE_APP, sch_lab,     SCH_LAB_AppMain,    SCH_LAB,      35,   32768, 0x0, 0;
CFE_APP, ci_lab,      CI_LAB_AppMain,     CI_LAB,       40,   32768, 0x0, 0;
CFE_APP, to_lab,      TO_LAB_AppMain,     TO_LAB,       45,   32768, 0x0, 0;
CFE_APP, sample_app,  SAMPLE_APP_Main,    SAMPLE_APP,   50,   32768, 0x0, 0;
CFE_APP, lc,  LC_AppMain, LC, 70,  131072, 0x0, 0;
CFE_APP, cf,  CF_AppMain, CF, 70,  131072, 0x0, 0;
CFE_APP, ds,  DS_AppMain, DS, 70,  131072, 0x0, 0;
CFE_APP, fm,  FM_AppMain, FM, 70,  131072, 0x0, 0;
CFE_APP, hk,  HK_AppMain, HK, 70,  131072, 0x0, 0;
CFE_APP, hs,  HS_AppMain, HS, 70,  131072, 0x0, 0;
CFE_APP, mm,  MM_AppMain, MM, 70,  131072, 0x0, 0;
CFE_APP, sc,  SC_AppMain, SC, 70,  131072, 0x0, 0;
CFE_APP, md,  MD_AppMain, MD, 70,  131072, 0x0, 0;
CFE_APP, cs,  CS_AppMain, CS, 70,  131072, 0x0, 0;
CFE_APP, sbn,  SBN_AppMain, SBN, 70,  131072, 0x0, 0;
```

`sbn_udp` and `sbn_f_remap` are built but are not in the script. SBN loads them at runtime from its own table.

**README L106 unit tests** (C12): the isolation check passed, then `make native_std.runtest` ran 69 tests in list order and stopped at the first failure. `network-api-test` passed 183 of 185 assertions; the two failures are the IPv6 socket open and the close that follows (`build-native_std/test-results/network-api-test.log.tmp` L61-62). The VM kernel has no IPv6 (`socket(AF_INET6)` gives `EAFNOSUPPORT`). The tests ran in their own build directories, but one of them used `/dev/shm/osal:RAM`, the same directory that backs `core-cpu1`'s `/ram` (mtime 02:08:16Z in `build_08_runtest_host_state.log`). Running the remaining 129 tests would need `make -k` or a host with IPv6; neither was done (§8).

## 5. Runs (task step 5, second half)

`run_cfs.sh` is started as root with `CFS_NORMAL_USER=ubuntu`. Each run:

- records the host state and refuses to start if another cFS/OSAL activity is present (v2 only; E13);
- `cd build-native_std/exe/cpu1`;
- `setsid setpriv --reuid=ubuntu --regid=1000 --init-groups -- ./core-cpu1 </dev/null >LOG 2>&1`;
- waits for the README L114 line (event) or for the fixed duration, then stops with the named method;
- writes `<log>.meta` with the run record. From run04 on it includes `/proc/<pid>/status` (credentials and capabilities), `/proc/<pid>/limits`, the cgroup, the environment (names, and values of listed variables), `TERM`/`TMPDIR`/`SHELL`, the stdout target, and, in fixed mode, the thread table.

| Run | Harness | Mode | Stop | Reset type at start | OPERATIONAL line read | Stop at | Exit | End | Exit reset status | Log |
|---|---|---|---|---|---|---|---|---|---|---|
| run01 | v1 | operational (50 ms poll) | SIGINT | POWER ON | 1613 ms | 1618 ms | 0 | 1783 ms | PROCESSOR | `run01_operational.log` |
| run02 | v1 | fixed 30 s | SIGINT | PROCESSOR | 1654 ms | 30023 ms | 0 | 30186 ms | PROCESSOR | `run02_fixed30s.log` |
| run03 (audit) | v1 | operational (50 ms poll) | SIGINT | PROCESSOR | 1662 ms | 1667 ms | 0 | 1835 ms | POWERON ("Maximum Processor Reset count reached (2)") | `run03_audit_operational.log` |
| run04 | v2 | operational (event) | `console-sigint` | POWER ON | 1630 ms | 1632 ms | 0 | 1758 ms | PROCESSOR | `run04_po_operational_sigint.log` |
| run05 | v2 | operational (event) | `ci-es-restart-poweron` | PROCESSOR | 1636 ms | 1637 ms | 0 | 2186 ms | POWERON | `run05_pr_operational_esrestart.log` |
| run06 | v2 | operational (event) | `ci-es-restart-poweron` | POWER ON | 1625 ms | 1626 ms | 0 | 2188 ms | POWERON | `run06_po_operational_esrestart.log` |
| run07 | v2 | fixed 30 s | `ci-es-restart-poweron` | POWER ON | 1624 ms | 30007 ms | 0 | 30573 ms | POWERON | `run07_po_fixed30s_esrestart.log` |

All times are wall-clock milliseconds after launch. run03 was run by the auditor with harness v1; its log and `.meta` were copied from the audit's scratch area into `logs/` so that the record does not depend on it. The time to OPERATIONAL includes the PSP's unconditional `sleep(1)` before the reset-type message (`psp/fsw/pc-linux/src/cfe_psp_start.c` L393).

**Conditions are not the same across these runs.** They differ in reset type, stop method, stop timing and harness. Comparisons below are made only between runs that share the relevant condition. In particular, the first version of this file compared run01 and run02 and attributed their different startup order to "the default Linux policy". That attribution is withdrawn: run01 was a POWER ON boot stopped 3-5 ms after OPERATIONAL, while TO_LAB and SBN were still initialising; run02 was a PROCESSOR reset boot, which runs different initialisation paths (EVS log restored, volatile disk reused, `/ram/cfe_es_startup.scr` probed, MD dwell tables recovered from CDS), and it ran for 30 s.

The reset-type sequence shows both stop paths (E7, E11): run01 PO → run02 PR → run03 PR, which hit the maximum of two processor resets and exited with POWERON; run04 PO (clean IPC) → SIGINT → run05 PR → ES restart → run06 PO → ES restart → run07 PO → ES restart. No SysV segment is left now.

### Observed startup sequence

1. **PSP and OSAL.**
   - "Maximum user msg queue depth = 10" (E5).
   - The scheduler probe selects policy 2, then "Could not setschedparam in main thread: Operation not permitted (1)" (E4). The run07 thread table shows all 27 threads in `SCHED_OTHER` with nice 0.
   - PSP modules are initialised in this order: `soft_timebase` (10000 us), `timebase_posix_clock`, `eeprom_mmap_file`, `port_notimpl`, `iodriver`, `linux_sysmon`, `endian_api`, `ram_notimpl`.
2. **Reset type.** PO boots print "Clearing out CFE CDS/Reset/User Reserved Shared memory segment" and "POWER ON RESET due to Power Cycle"; PR boots print "Normal exit from previous cFE instance" and "Volatile Disk has 100 Percent free space".
3. **ES.** EARLY_INIT, then CORE_STARTUP. EarlyInit runs for CONFIG, ES, EVS, FS, SB, TBL and TIME.
4. **Core apps.** They report "Initialized" in the order ES, EVS, SB, TBL, TIME, about 100 ms apart. Then CORE_READY.
5. **Startup script.** PR boots first try `/ram/cfe_es_startup.scr` and fall back to `/cf`. `cfe_assert` loads (prints "[BEGIN] CFE FUNCTIONAL TEST", "[BEGIN] 01 CFE-STARTUP"), then `sample_lib`. Then the apps are created in script order: SCH_LAB, CI_LAB, TO_LAB, SAMPLE_APP, LC, CF, DS, FM, HK, HS, MM, SC, MD, CS, SBN, all within about 2.2 ms.
6. **App initialisation.** Completion order, taken from cFE timestamps (not line order, E8; "SCH Lab Initialized." has no timestamp and is left out):

   | Run | Reset | Order of completion messages |
   |---|---|---|
   | run01 | PO | TO_LAB, SAMPLE_APP, LC, **CF**, FM, MM, CI_LAB, SBN, DS, HK, MD, [HS error 74], SC, CS, FM child |
   | run04 | PO | TO_LAB, **CF**, DS, FM, CI_LAB, MM, SAMPLE_APP, SBN, HK, LC, MD, [HS error 74], SC, CS, FM child |
   | run06 | PO | TO_LAB, SAMPLE_APP, LC, **CF**, FM, CI_LAB, MM, SBN, DS, HK, MD, [HS error 74], SC, CS, FM child |
   | run07 | PO | SAMPLE_APP, TO_LAB, **CF**, LC, CI_LAB, MM, SBN, FM, HK, DS, MD, [HS error 74], SC, CS, FM child |
   | run02 | PR | TO_LAB, SAMPLE_APP, LC, **CF**, DS, CI_LAB, FM, MM, SBN, HK, [HS error 74], MD, SC, CS, FM child |
   | run03 | PR | TO_LAB, SAMPLE_APP, **CF**, LC, FM, CI_LAB, MM, SBN, HK, DS, MD, [HS error 74], SC, CS, FM child |
   | run05 | PR | TO_LAB, SAMPLE_APP, LC, **CF**, FM, DS, HK, MM, [HS error 74], SC, CI_LAB, SBN, MD, CS, FM child |

   No two runs have the same order, including the four POWER ON runs, whose initialisation (all of which happens before any stop) shares reset type and binary. CF completes before LC in run03, run04 and run07, and after it in the others; the earlier statement "CF completed its initialisation right after LC in both runs" does not hold. **No ordering claim and no cause is stated here.** The task priorities have no effect (E4) and the host is shared (B3), but seven runs neither characterise the distribution nor isolate a cause. Ordering experiments need a fixed reset type (E7) and stop method (E11).
7. **ES state.** APPS_INIT, then OPERATIONAL, at cFE time 20.6046-20.6064 in all runs that started from the 1980-012-14:03:20 epoch (run03 started at a different cFE time).
8. **After OPERATIONAL.**
   - SBN loads `sbn_udp.so` and `sbn_f_remap.so`. Each symbol lookup prints a series of OSAL debug lines "undefined symbol: SBN_UDP_Ops/SBN_F_Remap" while the loaded modules are searched (`OSAL_CONFIG_DEBUG_PRINTF TRUE`, C6). SBN then configures peers CPU 1-3 on 127.0.0.1:3234-3236, after which EVS squelches SBN events (event 44).
   - TO_LAB subscribes to the 37 messages of its table (event 19) 47-49 ms of cFE time after OPERATIONAL.
   - **Pipe overflows (CFE_SB event 25)** differ from run to run in the same mode:

     | Run | `SBNSubPipe` (sender TO_LAB, MID 0x80e) | `DS_CMD_PIPE` (MID 0x808, EVS long event) |
     |---|---|---|
     | run01 | 16 | 0 |
     | run02 | 16 | 0 |
     | run03 | 15 | 1 (sender HS) |
     | run04 | 9 | 7 (sender SBN) |
     | run05 | 8 | 8 (sender SBN) |
     | run06 | 16 | 0 |
     | run07 | 11 | 5 (senders HS, MD, SC, DS, CS) |

     `SBNSubPipe` requests 32 and `DS_CMD_PIPE` 45; both are truncated to 10 (E5). The bundle CI treats the `SBNSubPipe` overflow as a known issue even at `msg_max` 64 (`test-cfs-qemu.yml` L262-268).
   - What an `operational` run contains after OPERATIONAL depends on the stop: in run04 "Caught SIGINT" came 19 ms (cFE time) after OPERATIONAL, so TO_LAB subscribed and the `SBNSubPipe` overflows happened during shutdown; with the ES restart (run05, run06) the restart came about 450 ms after OPERATIONAL, because CI_LAB reads its UDP socket once per 500 ms SB timeout (E10).
9. **Steady state** (run02 PR and run07 PO, 30 s each).
   - CFE_TIME flywheel events.
   - SC auto-starts **RTS 2 after a PR boot (run02) and RTS 1 after a PO boot (run07)** (`apps/sc/fsw/src/sc_app.c` L174-182), then runs its no-op commands; run07 also shows "Enabled RTS 002" and "RTS 001 Execution Completed".
   - No further SB overflows after the initial burst.
   - No CF events after "CF Initialized"; no CFDP transaction was requested.
   - A few bare "." characters appear on the console; their source was not identified.
10. **Shutdown.**
    - SIGINT (run01-run04): "CFE_ES_RunExceptionScan: ExceptionID ... Caught SIGINT", then "Processor Reset count not reached (n/2)" and "CFE_PSP: Exiting cFE with PROCESSOR Reset status." (run03: "Maximum Processor Reset count reached (2)" and a POWERON exit that removes the three segments).
    - ES restart (run05-run07): "CFE_ES_ResetCFE: POWERON RESET called from CFE_ES_ResetCFE (Commanded).", "CFE_PSP: Exiting cFE with POWERON Reset status.", and "Critical Data Store / Reset Area / User Reserved Area Shared memory segment removed".
    - Then "CFE_PSP: Shutdown initiated - Exiting cFE"; CI and TO close their sockets; the process exits 0 in every run. No escalation was needed.

### Errors and anomalies observed (none worked around)

| Message | Runs | Cause | Reference | Recorded as |
|---|---|---|---|---|
| `Could not setschedparam in main thread: Operation not permitted (1)` | all | Normal user, `RLIMIT_RTPRIO` 0, no `CAP_SYS_NICE` | README L102; `osal/default_config.cmake` L173-177 | E4 (documented behaviour) |
| `Maximum user msg queue depth = 10` | all | Normal user, `msg_max` 10 | README L102; `osal/default_config.cmake` L174-175 | E5 |
| `Pipe Overflow ... SBNSubPipe, sender TO_LAB` ×8-16 | all | TO_LAB's 37 subscriptions in a burst; pipe truncated 32 → 10; also a known SBN issue | E5; `test-cfs-qemu.yml` L262-268 | E5 (documented behaviour; varies per run) |
| `Pipe Overflow ... DS_CMD_PIPE` ×0-8 | run03-run05, run07 | EVS event bursts into DS's pipe, truncated 45 → 10 | `ds_internal_cfg.h` L223; E5 | E5 (varies per run) |
| `open(/proc/schedstat): No such file or directory`, `HS_SysMonInit(): Unable to start device 0110ff05`, `HS 74: Error in system monitor initialization, RC=0xC8000005` | all | The VM kernel exposes no `/proc/schedstat` | `psp/fsw/modules/linux_sysmon/linux_sysmon.c` L321-325; known issue in `test-cfs-qemu.yml` L262-268 | D-3. HS skips startup sync and the watchdog (`apps/hs/fsw/src/hs_app.c` L68-125, L333-341). |
| `CFE_PSP: Normal exit from previous cFE instance`; `CFE_ES_InitializeFileSystems: Volatile Disk has 100 Percent free space.`; `CFE_EVS_EarlyInit: Event Log restored, ...`; `OS_FileOpen_Impl(): open(/dev/shm/osal:RAM/cfe_es_startup.scr): No such file or directory` + `Cannot Open Volatile Startup file: /ram/cfe_es_startup.scr, Trying Nonvolatile.`; `MD 3: Recovered Dwell Table #1..#4`, `MD 7: Dwell Tables Recovered: 4, Dwell Tables Initialized: 0`; `SC 73: RTS Number 002 Started` | PR boots only (run02, run03, run05) | PROCESSOR reset: boot record, CDS and volatile disk are reused | `cfe_psp_start.c` L396-434; `cfe_evs_task.c` L159; `cfe_es_apps.c` L153; `cfe_es_start.c` L614; `md_app.c` L455; `sc_app.c` L174-182 | E7 (reset-type dependent; not an error) |
| `Clearing out CFE ... Shared memory segment` ×3; `POWER ON RESET due to Power Cycle`; `Event Log cleared following power-on reset`; `MD 7: Dwell Tables Recovered: 0, Dwell Tables Initialized: 4`; `SC 73: RTS Number 001 Started` | PO boots only | POWER ON reset | `cfe_psp_start.c` L433; `cfe_evs_task.c` L137; `md_app.c` L455; `sc_app.c` L174-182 | E7 |
| `CFE_SB 14: No subscribers for MsgId 0x808` (core apps, TO_LAB) | all | Events sent before anything subscribes to the event message ID | none needed | informational |
| `OS_GenericSymbolLookup_Impl(): ... undefined symbol` | all | OSAL debug printing while SBN searches loaded modules for its plug-in symbols | C6 (`OSAL_CONFIG_DEBUG_PRINTF TRUE`) | informational |
| `LC 23 "LC use of Critical Data Store disabled"`, `CS 127 "OS Text Segment disabled due to platform"` | all | App configuration defaults | app tables and platform configuration at the pinned commits | informational |
| One console line out of timestamp order (`...46149` before `...46148`) | run03 | `OS_printf` output is written by OSAL's utility task | `cfe_psp_start.c` L243-245; E8 | E8 (order by timestamps) |

## 6. State left behind (affects the next run)

- **System V shared memory:** none. run07 ended with the ES POWERON restart, which removed the three segments (keys `0x5200e5c5`, `0x5200e5c6`, `0x5200e5c7`). The next `./core-cpu1` without options will therefore start with a POWER ON reset, unless another process recreates those keys first.
- **`/dev/shm/osal:RAM`** (empty, mtime 02:08:16Z from the README L106 tests): backs `core-cpu1`'s `/ram`. OSAL's POSIX `mkfs` is a no-op, so anything written here survives every boot, POWER ON included (E7, E13).
- **`/dev/shm/osal:RAM3`, `RAM4`, `RAM5`** (empty, 01:45:52Z): left by OSAL test processes of another workflow, not by this build.
- **In `build-native_std/exe/cpu1`:** `.cdskeyfile`, `.reservedkeyfile`, `.resetkeyfile`, `EEPROM.DAT` and `cf/tmp/`. `EEPROM.DAT` persists across all boots.
- **In `build-native_std`:** `test-list.json`, `test-deps.mk`, `test-results/` (68 `.log` files and `network-api-test.log.tmp`) from C12.
- Nothing was cleaned, because cleaning is not a documented step.

## 7. Reproduction

```
# as root in this container (the README asks for a normal user; see CONDITIONS.md C11/E1/D-1)
CFS_NORMAL_USER=ubuntu ./build_cfs.sh build   /home/user/work/procrace/cfs_ref ./logs   # README L89-105 (fresh clone only)
CFS_NORMAL_USER=ubuntu ./build_cfs.sh record  /home/user/work/procrace/cfs_ref ./logs   # effective settings of an existing build
CFS_NORMAL_USER=ubuntu ./build_cfs.sh runtest /home/user/work/procrace/cfs_ref ./logs   # README L106
CFS_NORMAL_USER=ubuntu ./run_cfs.sh operational console-sigint        ./logs/run04_po_operational_sigint.log
CFS_NORMAL_USER=ubuntu ./run_cfs.sh operational ci-es-restart-poweron ./logs/run05_pr_operational_esrestart.log
CFS_NORMAL_USER=ubuntu ./run_cfs.sh operational ci-es-restart-poweron ./logs/run06_po_operational_esrestart.log
CFS_NORMAL_USER=ubuntu ./run_cfs.sh 30          ci-es-restart-poweron ./logs/run07_po_fixed30s_esrestart.log
# as a normal user on a host where that user can reach GitHub: omit CFS_NORMAL_USER
```

The reset type of each run depends on how the previous one ended (E7), so the runs above reproduce their recorded reset types only in this order and from a host without the three SysV segments.

The clone `/home/user/work/procrace/cfs_ref/cFS` is owned by `ubuntu` (D-1). Read-only git commands run as root need `git -c safe.directory='*' ...`; otherwise git reports "dubious ownership".

`build_cfs.sh build` refuses to run if the clone directory exists or if a build-affecting variable is exported (C4). `run_cfs.sh` has no default duration and no default stop method, refuses `LD_*`, `MALLOC_*` and `GLIBC_TUNABLES`, and refuses to start when the isolation check fails (exit 8).

## 8. Open items for the PI

1. **A2, bundle branch.** The build uses `main` = `v7.0.1` (`088b2fa8`), as the task asked. The README's plain clone gives `dev` (`01e8416c`, 139 commits ahead), which is the v7.0.2rc1 release candidate and drops MM from the default apps. Confirm `main`, or rebuild with `dev` (new `BUNDLE_COMMIT` and submodule list).
2. **E5, queue-depth mode.** README mode (`msg_max` 10, every queue deeper than 10 silently truncated, overflow counts varying per run) or the bundle CI's execution environment (`msg_max` 64, no queue truncated). Queue loss and ordering are what the paper measures, so this decides what is measured. Setting `msg_max` would change a host-shared sysctl; whether this container allows it was not tested.
3. **E7/E11, stop method and reset policy.** Options, all from references: (a) the bundle CI's ES POWERON restart, which gives a POWER ON boot every time without harness cleanup, but lands up to one CI_LAB read period (500 ms) after it is sent; (b) SIGINT (console CTRL+C), the PSP's exception path, which cycles PO → PR → PR → PO; (c) `-R PO` at start. With any of them, `/dev/shm/osal:RAM` and `EEPROM.DAT` persist across runs.
4. **C12, unit tests.** README L106 stopped at the first failure, caused by the missing IPv6. Options: accept as is; run the remaining tests with `make -k` (a deviation); or run on a host with IPv6.
5. **D7, case C1.** The default CF table never executes C1's semaphore lookup. The semaphore name and the provider app must come from C1's references (`cases/C1.md` F9, §12), not from this environment.
6. **D-3, HS without `/proc/schedstat`.** If a case involves HS startup or watchdog behaviour, this container cannot reproduce the reference behaviour.
7. **D-4, root mode.** A root ("flight-like") run with RT priorities fails here at queue creation unless `msg_max` is raised to at least 50 or `CAP_SYS_RESOURCE` is granted. Either is a deviation unless a case reference prescribes it.
8. **E13, isolation.** Other workflows run OSAL processes as the same account (`ubuntu`) and use the same `/dev/shm/osal:*` names. `run_cfs.sh` now refuses to start while such activity is visible, but it cannot see activity between its check and the run, or OSAL processes that hold none of the checked resources at that moment.
9. **D-1, optional.** The clone could be redone as `ubuntu` with the CA-bundle variables unset. This changes ownership only and would delete the tree other workflows read.

## 9. Audit findings (2026-10-08) and what was done

| # | Finding | Applied? | What changed |
|---|---|---|---|
| 1 | E11/E7: the bundle CI's stop (ES restart, POWERON via `cmd_send`) was not considered | Applied, with one correction | E11 now lists three referenced stop methods and E7 the reset policy options; both are marked PI decision pending. `run_cfs.sh` v2 implements `ci-es-restart-poweron` and `console-sigint` and has no default. run05-run07 show that the ES restart exits with POWERON status and that the next boot is POWER ON. **Correction to the finding:** the ES restart is the stop of `build-run-app-reusable.yml` (L209-214); the bundle's own `native_std` job in `test-cfs-qemu.yml` sends only a no-op through `cmd_send` (L226, L236-239) and stops with `docker stop` (L289-293), i.e. SIGTERM, which the PSP treats like SIGINT (`cfe_psp_exception.c` L230-233). Also recorded: the ES restart does not reset `/dev/shm/osal:RAM` or `EEPROM.DAT`. |
| 2 | E5/E6/D-3: conflicting `msg_max` reference from the bundle CI; known issues not cited; DS and TO_LAB missing from the truncated list | Applied | E5 cites `actions/start-cfs-container/action.yml` L29-34 next to README L102 and is marked PI decision pending; it lists every truncated pipe, including DS 45 → 10 and TO_LAB TLM 50 → 10. D-3 and E5 cite `test-cfs-qemu.yml` L262-268. The run-to-run variation of overflows is in §5. |
| 3 | D1-D6: C1's code path is not executed; doc-versus-table contradiction; TO integration requirement unchecked | Applied | New D7 (C1 not executable with the default table; conditions must come from C1's references) and D8 (`CF_MAX_PDU_SIZE` 512 requirement: met for size; TO_LAB drains its pipe; depth truncated to 10). D4 records the contradiction. §3 lists CF's Integration section. |
| 4 | C5: recorded flags were not the effective flags | Applied | C5 rewritten from `compile_commands.json` (`logs/build_07_record_existing.log`): `-g`, no `-O`, no `-Wcast-align=strict`/`-fno-common` for FSW; host tools add `-Wcast-align`. |
| 5 | ENV.md §5: run01/run02 comparison and causal attribution | Applied | §5 labels every run with its reset type, stop method and harness, withdraws the attribution to scheduling, orders events by timestamps, adds the PR-only and PO-only messages, and makes no ordering claim. |
| 6 | §1 and E4/E6/§8: root failure mechanism; capabilities not recorded | Applied | §1 corrected; new F row D-4; E1/E6 record the capabilities of `core-cpu1` in every `.meta` from run04; `logs/f_root_capability_probe.log`. |
| 7 | A3: nested-submodule statement false; wrong check | Applied | A3 and §2 corrected; `build_cfs.sh` v3 checks gitlinks (mode 160000); result in `build_07_record_existing.log`. |
| 8 | D-1: "unavoidable" basis refuted | Applied (reworded) | D-1 is now "a convenience, not unavoidable", with the working alternative recorded in `logs/d1_normal_user_fetch_probe.log`. The clone was not redone (§8 item 9). |
| 9 | D-2: runtest was runnable; stated reason inaccurate | Applied | README L106 was run (C12): it fails on IPv6, which is a container limitation, and its shared-state side effect on `/dev/shm/osal:RAM` is recorded. D-2 now covers only L107. |
| 10 | H-3/E10: 50 ms poll; run isolation missing | Applied | `run_cfs.sh` v2 detects the README L114 line by an event (`tail -f`, inotify) and has no polling constant; the CI's 2 s × 30 interval is cited. New E13/H-5: isolation check with refusal, and host-state snapshots before and after every run and before the unit tests. |
| 11 | C4/E1/E8: environment not checked or recorded; line order | Applied | `build_cfs.sh` v3 refuses the extended list of variables and dumps the environment; C4 states the effective values from the build artifacts. `run_cfs.sh` v2 records the run process's environment and `TERM`/`TMPDIR`/`SHELL` (E12). E8 now says that order must come from timestamps. |
| 12 | Citation and record accuracy (D2, C3, step log, A2) | Applied | D2: 6th `CFE_APP` line, line 8. C3 and §4: `build_04_prep.log` L5. §4 lists every post-run edit of `build_cfs.sh` and which version produced which log. A2 records that `dev` is v7.0.2rc1 and drops MM. |

No finding was skipped. Finding 1 was applied with the correction above, because the reference does not support "the bundle CI stops native cFS with an ES Restart command" for the bundle's own `native_std` job.

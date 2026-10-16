# M1: measured baseline conditions of the native cFS run

Measured 2026-10-08 (06:16–06:31 UTC) under `../CONDITIONS_POLICY.md`, in answer to the external review that asked for the baseline conditions to be measured instead of asserted. Paths and line numbers follow the conventions of `CONDITIONS.md` (pinned revisions there; `cfe/`, `osal/`, `psp/`, `apps/<x>/` resolve against the pinned repositories). Kernel sources are cited from upstream Linux tag `v6.18` (`ipc/mqueue.c`, `ipc/mq_sysctl.c`, `include/linux/ipc_namespace.h`, `include/uapi/linux/nsfs.h`, fetched from raw.githubusercontent.com); the running kernel is the vendor build `6.18.44-fc-v80`, whose source is not available here, so every kernel statement below is also checked by a measurement.

Nothing in the cFS build, `sample_defs`, tables, sysctls, rlimits or capabilities was changed. Nothing was deleted. The only additions to host state are listed in §6.4.

## 0. Key results

| Question from the review | Measured answer |
|---|---|
| Queue capacity, requested vs actual | 26 POSIX queues per run, all SB pipes (no other OSAL queue). OSAL keeps the requested depth in its own record; the kernel queue has `mq_maxmsg` = min(requested, 10) and `mq_msgsize` = 8. 22 of 26 pipes are truncated; total capacity 247 messages instead of the requested 584. Identical in all 12 snapshots. (§3.1) |
| Losses | True queue-full drops (`PipeOverflowErrorCounter`) per run: **33, 37, 4, 49, 45, 41** (A1–A3, B1–B3). All of them happen before S1 (the counter does not change between S1 and S2). The console shows at most 16 because of the event filter, and fewer than the drops even below the cap (A3: 4 drops, 2 lines). run01–run07 all show exactly 16, i.e. all seven were capped, and their true totals are unknown. **SB `SendErrors` (pipe-info file) does not count queue-full drops**, so the file cannot give per-pipe losses. (§3.2–3.4) |
| Other loss paths | EVS squelch: SBN loses 13 events before S1 and 17–18 by S2 in every run (the console shows a single "Events squelched" line). SBN's own init-complete event is never printed in any of 13 logs. "No subscribers" events are capped at 4 (counter 15–26). The EVS local log (20 entries, discard mode) overflows 89–103 times. (§3.5) |
| Scheduling, requested vs actual | Three levels: ES/startup-script priority (25…205) → OSAL record 255 for every task → kernel `SCHED_OTHER`, rt_priority 0, nice 0 for all 27 threads, in all 12 snapshots. Each run is its own autogroup. (§4) |
| HS state and impact | HS init stops at `HS_SysMonInit`; HS still runs its main loop (+23 iterations between S1 and S2, about 28 s; the 1200 ms wakeup timeout), prints the aliveness dot 4 times, reports CPU utilisation as `0xFFFFFFFF`, emits no event after event 74 and never exits. The skipped steps (startup sync, watchdog, event subscriptions) have no effect on this platform with the default tables, except the missing init event and the missing startup sync. (§5) |
| Reset type and persisted data | Every M1 boot is POWER ON (console) after the ES POWERON stop. Persisting across processes: `EEPROM.DAT` (all zero bytes in all 12 records, mtime refreshed at every boot), the keyfiles (same ftok keys as before the reboot), and files on `/dev/shm/osal:RAM` (B1's and B2's `cfe_sb_pipe.dat` were still there when the next run booted with a POWER ON reset). No SysV segment exists between runs. **The first run after the reboot (A1) starts differently**: app loading took 849 ms instead of 2–3 ms, so app initialisation overlapped loading, and the OPERATIONAL line came at 2570 ms instead of 1613–1633 ms. (§6) |
| App status (a)/(b)/(c) | (a) all 20 apps created in all 13 logs; (b) init-complete report present for 18 apps, absent for HS (init failed) and for SBN (its report is squelched; the main loop runs, so init succeeded); (c) all 20 main tasks alive at S2, AppState RUNNING, main-loop counters advanced in all 6 M1 runs. (§7) |
| D2 feasibility (nothing changed) | `msg_max` is 0644 root:root; root may write it (the identical value 10 was accepted). This session is in the VM's **initial** IPC namespace, so a change would apply to every process in the VM. A full queue set at depth 64 would charge 173 056 B against the 819 200 B `RLIMIT_MSGQUEUE` (60 736 B at the requested depths). (§8) |

## 1. Method

### 1.1 What was run

Six runs of the documented executable (`./core-cpu1` in `build-native_std/exe/cpu1`, run user `ubuntu`, CONDITIONS.md E1–E3), each started by `env/measure_baseline.sh <A|B> <i>`, which calls `env/run_cfs.sh` v3:

| Condition | Value | Reference | Status |
|---|---|---|---|
| Window | fixed, 30 s from launch (`run_cfs.sh 30`) | `CFE_PLATFORM_CORE_MAX_STARTUP_MSEC` = 30000, `cfe/modules/core_private/config/default_cfe_core_private_internal_cfg.h` L68, read from source by the script. The constant used in E10/run07. | unspecified_by_reference (the README gives no run length; E10) |
| Stop | `ci-es-restart-poweron` (E11 candidate (a)) | `.github/workflows/build-run-app-reusable.yml` L209-214 | from-reference as a candidate. **The stop method for experiments is still PI decision D3/E11**; it is used here only for characterisation. |
| S1 | Right after the console line "CFE_ES_Main entering OPERATIONAL state" is read (event-based; the existing `tail -f` reader of run_cfs.sh) | README L114; `CONDITIONS_POLICY.md` L22 | Hook point of run_cfs.sh v3 (H-7) |
| S2 | End of the 30 s window, immediately before the stop command | E10 fixed window | H-7 |
| Variant A "probe-only" | At S1 and S2: thread table, mq probe, memory read (§2). No command except the stop. | — | harness |
| Variant B "with SB pipe info" | As A, plus at S2: SB Write-Pipe-Info command, event-based wait for the file, copy, parse, second memory read (S2b), then stop | §2.4 | harness; a perturbation (§1.3) |
| Failure bound for the file wait | 5000 ms = `CFE_PLATFORM_ES_APP_KILL_TIMEOUT` × `CFE_PLATFORM_ES_APP_SCAN_RATE` (the GRACE bound of run_cfs.sh) | `cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h` L199, L229 | unspecified_by_reference (H-4 assumption; a failure bound only, never reached: 339–391 ms) |
| Number of runs | 3 per variant | none | **unspecified_by_reference**. A characterisation assumption, not an experiment N. |
| Order | A1, A2, A3, then B1, B2, B3, back to back | none | **unspecified_by_reference**. Assumption: A first, so that no A run starts with the file a B run leaves on the persistent RAM volume (§6.3). A1 is the first cFS process after the VM reboot at 06:00:21. |

All six runs passed the E13 isolation check, booted with a POWER ON reset, exited with status 0 and "Exiting cFE with POWERON Reset status" (`logs/m1_*_po_fixed30s_esrestart.log.meta`).

### 1.2 Timing (ms since launch, from the `.meta`)

| Run | OPERATIONAL line read | S1 hook | S2 hook | Stop sent | Process end |
|---|---|---|---|---|---|
| A1 | **2570** | 2587–2663 | 30006–30080 | 30082 | 30236 |
| A2 | 1633 | 1642–1715 | 30006–30077 | 30079 | 30574 |
| A3 | 1626 | 1635–1708 | 30007–30076 | 30078 | 30634 |
| B1 | 1613 | 1623–1714 | 30007–30637 | 30639 | 31097 |
| B2 | 1629 | 1638–1711 | 30007–30554 | 30556 | 31073 |
| B3 | 1624 | 1633–1721 | 30007–30590 | 30591 | 31103 |

The snapshot itself (all reads of one snapshot) took 4.4–7.2 ms; most of each hook is Python start-up. run04–run07 read the OPERATIONAL line at 1624–1636 ms.

### 1.3 How much the measurement disturbs the run

* **A (probe-only):** read-only access from a separate root process: procfs files, `mq_getattr(3)` on a second open file description of each queue (no message received or sent; tested in `logs/m1_tooltest.log` §1), and `pread` on `/proc/<pid>/mem` (no ptrace attach, so no thread is stopped). These take kernel locks for microseconds. The hook process competes for the 4 vCPUs (all `SCHED_OTHER`) for about 70–90 ms right after the OPERATIONAL line is read. Its snapshot runs 36–56 ms after that line, which overlaps the startup burst: TO_LAB subscribes, and SBNSubPipe overflows, about 47–49 ms after OPERATIONAL in cFE time (CONDITIONS.md D8). The overflows are complete by the time the S1 memory read runs (§3.3). Whether that CPU competition changes the drop counts cannot be separated with this design, because A and B probe the same way at S1, and run01–run07, which had no S1 probe, were all capped at 16 on the console.
* **B adds:** one command through CI_LAB to SB (SB `CommandCounter` 0→1), one background file write by `ES_BG_TASK`, a 1624-byte file on `/dev/shm/osal:RAM` that persists (§6.4), and a stop about 475–560 ms later than in A (stop sent at 30556–30639 ms vs 30078–30082 ms). No pipe statistic changed between S2 and S2b (§3.4).

## 2. Instruments

All tools are in `env/tools/`. `measure_baseline.sh` records their sha256 values in `logs/m1_<run>_driver.txt`.

### 2.1 `run_cfs.sh` v3 hook

`RUN_CFS_HOOK=<executable>` makes run_cfs.sh call `<hook> S1|S2 <pid> <log> <ms>` synchronously at S1 (fixed mode only, after the existing thread capture) and at S2 (before the stop). Hook output goes to `<log>.hook`. `RUN_CFS_HOOK` is removed from the environment before `core-cpu1` is launched (`export -n`), so the cFS process environment is unchanged. With the variable unset, v3 behaves like v2, and the `.meta` gains no new line (only the version string differs). The diff against v2 is limited to the header, the version string, the usage range, the hook function, two call sites and two `.meta` lines that are printed only when the hook is set.

### 2.2 `mq_probe` (kernel queue attributes)

OSAL names each queue `/<pid>.<name>` and `mq_unlink`s it right after `mq_open` (`osal/src/os/posix/src/os-impl-queues.c` L111, L116, L142), so it cannot be opened by name. `mq_probe <pid>` reads the link target of every `/proc/<pid>/fd/<n>`, and opens only those that look like an OSAL queue (`/<digits>.<name> (deleted)`) with `O_RDONLY|O_NONBLOCK`. It checks that the descriptor is on the mqueue filesystem (`fstatfs` magic `0x19800202`), calls `mq_getattr`, and closes the descriptor. Tool test (`logs/m1_tooltest.log` §1): a test queue holding 3 messages showed `mq_curmsgs` 3 on two probes, and the owner then still read 3, so nothing was consumed.

### 2.3 `/proc/<pid>/mem` reader (cFE/OSAL/HS tables)

`cfs_layout.py` runs gdb in batch mode on the **files** `core-cpu1` and `cf/hs.so` (no process is attached). It extracts addresses, strides and field offsets of `CFE_SB_Global.PipeTbl` and `.HKTlmMsg.Payload`, `OS_common_table` (queue and task ranges), `OS_queue_table`, `OS_impl_queue_table`, `OS_task_table`, `CFE_ES_Global.AppTable/TaskTable/SystemState`, `CFE_EVS_Global.AppData` and `.EVS_TlmPkt.Payload`, and `HS_AppData`. The build carries `-g` (CONDITIONS.md C5), so the debug info describes the code that runs. Output: `logs/m1_layout.json`, with the sha256 of both binaries. `cfs_snapshot.py` adds the load base from `/proc/<pid>/maps` and reads each table with one `pread`. The joins are by identity, not by name: SB pipe `SysQueueId` = OSAL `active_id` → OSAL impl `mqd_t` = the kernel descriptor number → `mq_probe` row. The copy is not atomic (no cFE lock is taken).

**Validation against cFE's own dump:** in B1–B3 the pipe-info file written by cFE (§2.4) and the memory read agree on all 26 pipes × {MaxQueueDepth, CurrentQueueDepth, PeakQueueDepth, SendErrors, AppName}: 0 differences against S2b, and one expected difference against S2 (B3 `SCH_LAB_CMD_PIPE` CurrentQueueDepth 1 at S2, 0 in the file) (`logs/m1_analysis.txt` §5).

### 2.4 SB Write Pipe Info (variant B)

* Command: `cmd_send -v --host=127.0.0.1 --endian=LE --pktid=0x1803 --cmdcode=7 --string="64:"`, run as the run user, in the style of the CI's ES restart (`build-run-app-reusable.yml` L213-214). MID 0x1803 = `CFE_PLATFORM_CMD_MID_BASE` 0x1800 | SB command topic 3 (`cfe/modules/core_api/config/default_cfe_core_api_msgid_mapping.h` L49-50; `cfe/modules/sb/fsw/inc/cfe_sb_topicids.h` L37-38). Function code 7 = `CFE_SB_FunctionCode_WRITE_PIPE_INFO` (`cfe/modules/sb/config/default_cfe_sb_fcncode_values.h` L45).
* `cmd_send` accepts `NNN:` with an empty string and zero-pads it to NNN bytes (`tools/commandline-tools/src/passthru_encode.c` L734-777). The packet was 72 bytes (CCSDS length field 0x41, in the hook log), which equals `sizeof(CFE_SB_WritePipeInfoCmd_t)` = 8 + `CFE_MISSION_MAX_PATH_LEN` 64 in the binary (gdb), so the SB length check passes. SB `CommandCounter` went 0→1 and `CommandErrorCounter` stayed 0 in all three runs.
* Empty file name → `CFE_PLATFORM_SB_DEFAULT_PIPE_FILENAME` = `"/ram/cfe_sb_pipe.dat"` (`cfe/modules/sb/fsw/inc/cfe_sb_internal_cfg.h` L185-186; `cfe_sb_task.c` L952-958). `/ram` is the OSAL volume "RAM", mounted at `/dev/shm/osal:RAM` (`cfe/modules/es/fsw/src/cfe_es_start.c` L523-527, L580; `cfe_es_internal_cfg.h` L89-90; `os-impl-filesys.c` L134-136, L181-190).
* **The completion event cannot be used:** `CFE_SB_SND_RTG_EID` "… written:Size=…" is a DEBUG event (`cfe_sb_task.c` L741-749), and DEBUG output is off by default (`CFE_PLATFORM_EVS_DEFAULT_TYPE_FLAG` 0xE, `cfe/modules/evs/fsw/inc/cfe_evs_internal_cfg.h` L160-177). It never appeared on the console. The wait is therefore event-based on the file: inotify `IN_CLOSE_WRITE` on `/dev/shm/osal:RAM` (`inotify_wait.py`, armed before the command is sent). The background writer closes the file before it raises COMPLETE (`cfe/modules/fs/fsw/src/cfe_fs_api.c` L818, L822). Command to close took 391, 339 and 379 ms; CI_LAB polls its socket once per 500 ms SB timeout (E10).
* File format (`sb_pipeinfo_parse.py`): `CFE_FS_Header_t`, 64 bytes, big-endian (byte-swapped on little-endian hosts, `cfe_fs_api.c` L234-242; ContentType `0x63464531`, SubType 20 = `CFE_FS_SubType_SB_PIPEDATA`, Description "SB Pipe Information"), then one 60-byte `CFE_SB_PipeInfoEntry_t` per pipe in use, in host order. Layout from the binary: PipeId@0, AppId@4, PipeName[20]@8, AppName[20]@28, MaxQueueDepth@48, CurrentQueueDepth@50, PeakQueueDepth@52, SendErrors@54, Opts@56, Spare[3] (`default_cfe_sb_msgdefs.h` L143-155, `CFE_MISSION_MAX_API_LEN` 20). Measured file: 1624 B = 64 + 26 × 60.

### 2.5 Persisted-data records

`persist_snapshot.sh` (read-only) runs before and after every run (`logs/m1_<run>_persist_{before,after}.txt`). It records: `EEPROM.DAT` size, mtime, sha256 and number of non-zero bytes; the three keyfiles with inode and the computed glibc `ftok(…,'R')` key; `ipcs -m`; `/dev/shm`; every `/dev/shm/osal:*` directory with each file's size, mtime and sha256; and `cf/tmp`.

### 2.6 Analysis

`m1_analyze.py logs > logs/m1_analysis.txt` builds every table below from the logs.

## 3. Queues and losses

### 3.1 Requested vs OSAL vs kernel capacity

Mechanism: SB passes the requested depth to `OS_QueueCreate` with message size `sizeof(CFE_SB_BufferD_t *)` (`cfe/modules/sb/fsw/src/cfe_sb_api.c` L163) and stores it as `MaxQueueDepth` (L191). OSAL checks `depth <= OS_QUEUE_MAX_DEPTH` 50 (`osal/src/os/shared/src/osapi-queue.c` L98) and stores the requested depth in its record (L109). Only the posix implementation truncates the `mq_attr` it passes to `mq_open`, to the BSP limit in permissive mode (`os-impl-queues.c` L59-63, L94-105). For a non-root user the BSP limit is read from `/proc/sys/fs/mqueue/msg_max` (`osal/src/bsp/generic-linux/src/bsp_start.c` L66-78; console "Maximum user msg queue depth = 10").

Measured (identical at S1 and S2 of all six runs; columns: SB request = `PipeTbl[].MaxQueueDepth`, OSAL record = `OS_queue_table[].max_depth`, kernel = `mq_getattr`):

| Pipe | App | SB request | OSAL record | Kernel `mq_maxmsg` | `mq_msgsize` |
|---|---|---|---|---|---|
| ES_CMD_PIPE | CFE_ES | 12 | 12 | 10 | 8 |
| EVS_CMD_PIPE | CFE_EVS | 32 | 32 | 10 | 8 |
| SB_CMD_PIPE | CFE_SB | 32 | 32 | 10 | 8 |
| TBL_CMD_PIPE | CFE_TBL | 12 | 12 | 10 | 8 |
| TIME_CMD_PIPE | CFE_TIME | 12 | 12 | 10 | 8 |
| CI_LAB_CMD_PIPE | CI_LAB | 32 | 32 | 10 | 8 |
| SAMPLE_APP_CMD_PIPE | SAMPLE_APP | 32 | 32 | 10 | 8 |
| TO_LAB_CMD_PIPE | TO_LAB | 8 | 8 | 8 | 8 |
| SCH_LAB_CMD_PIPE | SCH_LAB | 8 | 8 | 8 | 8 |
| TO_LAB_TLM_PIPE | TO_LAB | 50 | 50 | 10 | 8 |
| LC_CMD_PIPE | LC | 12 | 12 | 10 | 8 |
| CF_CMD_PIPE | CF | 32 | 32 | 10 | 8 |
| CF_CHAN_0 | CF | 16 | 16 | 10 | 8 |
| CF_CHAN_1 | CF | 16 | 16 | 10 | 8 |
| DS_CMD_PIPE | DS | 45 | 45 | 10 | 8 |
| FM_CMD_PIPE | FM | 10 | 10 | 10 | 8 |
| HK_CMD_PIPE | HK | 40 | 40 | 10 | 8 |
| HS_CMD_PIPE | HS | 12 | 12 | 10 | 8 |
| HS_EVENT_PIPE | HS | 32 | 32 | 10 | 8 |
| HS_WAKEUP_PIPE | HS | 1 | 1 | 1 | 8 |
| MM_CMD_PIPE | MM | 12 | 12 | 10 | 8 |
| SC_CMD_PIPE | SC | 12 | 12 | 10 | 8 |
| MD_CMD_PIPE | MD | 50 | 50 | 10 | 8 |
| CS_CMD_PIPE | CS | 12 | 12 | 10 | 8 |
| SBNCmdPipe | SBN | 20 | 20 | 10 | 8 |
| SBNSubPipe | SBN | 32 | 32 | 10 | 8 |
| **Total (26)** | | **584** | 584 | **247** | |

The requests match the list in CONDITIONS.md E5 with one exception: SBN's per-peer pipes (`SBN_<cpu>_<sc>_Pipe`) are created only when a peer connects (`apps/sbn/fsw/src/sbn_app.c` L161-181, `SBN_Connected`). No peer runs here, so they do not exist. There is no OSAL queue other than the 26 SB pipes (`other_queues` empty), and `mq_probe` found exactly 26 queues in every snapshot. Backlog at S1: `SBNSubPipe` held 10 messages (full) in A1, A2 and B1–B3, and 5 in A3. Every queue was empty at S2 except an occasional 1 in `SCH_LAB_CMD_PIPE`.

### 3.2 Loss counters: what each one counts

| Counter | What increments it | Where |
|---|---|---|
| SB HK `PipeOverflowErrorCounter` (POEC) | every `OS_QueuePut` that returns `OS_QUEUE_FULL` (transmit is non-blocking, `OS_CHECK`) | `cfe/modules/sb/fsw/src/cfe_sb_priv.c` L1177-1194; timeout mode L1129-1155 |
| Pipe `SendErrors` (pipe-info file, SB pipe table) | **only** the per-MsgId buffer limit (`BuffCount >= MsgId2PipeLim`, event `CFE_SB_MSGID_LIM_ERR_EID` 17). A full queue does **not** increment it. | `cfe_sb_priv.c` L1058-1063 (the only `++PipeDscPtr->SendErrors`) |
| Pipe `PeakQueueDepth` | optimistic: raised before `OS_QueuePut` (L1070-1074), and `CurrentQueueDepth` is decremented again after a failed put (L1202-1206) or after a receive (L1441-1444). A peak above the kernel depth therefore means that SB counted more buffers in flight than the queue can hold. That is consistent with a drop on that pipe but is not proof of one: it can also happen when a receive has taken a message from the kernel and not yet decremented the count. | `cfe_sb_priv.c` |
| Console "Pipe Overflow" (SB event 25) | sent with SB's AppId (`cfe_sb_priv.c` L784-790). SB's binary filter `CFE_EVS_FIRST_16_STOP` applies to all pipes together, so at most 16 per process (`cfe_sb_internal_cfg.h` L239-243). Attempts are also dropped before reaching EVS when the sending task is already in an SB event send (`CFE_SB_RequestToSendEvent`, L784). | |

There is **no per-pipe counter of queue-full drops** in this cFE. Per-pipe losses are visible only as console events (first 16 attempts that reach EVS) and as `PeakQueueDepth` above the kernel depth.

### 3.3 Measured per run (memory at S1 and S2; console)

| Run | POEC at S1 | POEC at S2 | `MsgSendErrorCounter` | `MsgLimitErrorCounter` | EVS filter count of event 25 | Console "Pipe Overflow" lines | At the 16 cap? |
|---|---|---|---|---|---|---|---|
| A1 | 33 | 33 | 33 | 0 | 32 | 16 (SBNSubPipe ← TO_LAB ×16) | yes |
| A2 | 37 | 37 | 37 | 0 | 34 | 16 (SBNSubPipe 13, DS_CMD_PIPE 3) | yes |
| A3 | **4** | 4 | 4 | 0 | 2 | **2** (TO_LAB_TLM_PIPE ← SBN ×2) | no |
| B1 | 49 | 49 | 40 | 0 | 40 | 16 (SBNSubPipe 12, DS_CMD_PIPE 4) | yes |
| B2 | 45 | 45 | 45 | 0 | 39 | 16 (SBNSubPipe 8, DS_CMD_PIPE 8) | yes |
| B3 | 41 | 41 | 38 | 0 | 35 | 16 (SBNSubPipe 13, DS_CMD_PIPE 3) | yes |

* All drops happen before the S1 memory read, which ran 36–56 ms after the OPERATIONAL line was read (computed from the hook log and the snapshot clock), and POEC is the same at S1 and S2 in every run. Some happen even before OPERATIONAL, during app initialisation (e.g. run07: DS_CMD_PIPE ← HS at 20.561 s, OPERATIONAL at 20.605 s cFE time). Between S1 and S2 (about 28 s) there were **0** queue-full drops in all six runs.
* `MsgLimitErrorCounter` = 0 and every `SendErrors` = 0, so no MsgId-limit loss occurred.
* The spread (4 to 49) between runs of identical configuration is the run-to-run variation of the startup burst. A3 was a different interleaving: SBNSubPipe peaked at 5 and did not overflow, while TO_LAB_TLM_PIPE did.
* `MsgSendErrorCounter` < POEC in B1 and B3: one transmit can overflow more than one pipe.

Per-pipe `PeakQueueDepth` at S2 (memory read; `*` = above the kernel depth; all `SendErrors` 0; all other pipes ≤ 2):

| Pipe (kernel depth) | A1 | A2 | A3 | B1 | B2 | B3 |
|---|---|---|---|---|---|---|
| SBNSubPipe (10) | 11* | 11* | 5 | 11* | 11* | 11* |
| DS_CMD_PIPE (10) | 10 | 11* | 5 | 12* | 12* | 12* |
| TO_LAB_TLM_PIPE (10) | 11* | 9 | 12* | 11* | 4 | 11* |

### 3.4 run01–run07 (console only; re-derived)

| Log | "Pipe Overflow" lines (pipe ← sender) | At the 16 cap? |
|---|---|---|
| run01 | 16 (SBNSubPipe ← TO_LAB 16) | yes |
| run02 | 16 (SBNSubPipe 16) | yes |
| run03 | 16 (SBNSubPipe 15, DS_CMD_PIPE ← HS 1) | yes |
| run04 | 16 (SBNSubPipe 9, DS_CMD_PIPE ← SBN 7) | yes |
| run05 | 16 (SBNSubPipe 8, DS_CMD_PIPE ← SBN 8) | yes |
| run06 | 16 (SBNSubPipe 16) | yes |
| run07 | 16 (SBNSubPipe 11, DS_CMD_PIPE ← CS, DS, HS, MD, SC 1 each) | yes |

All seven logs are at the cap, so the per-pipe numbers in E5 ("SBNSubPipe 8–16, DS_CMD_PIPE 0–8") are lower bounds on the first 16 reported attempts, not loss counts. The true totals of run01–run07 are unknown. In M1 they were between 4 and 49.

### 3.5 Other loss paths found

| Path | Reference | Measured |
|---|---|---|
| EVS squelch (per-app token bucket: burst `CFE_PLATFORM_EVS_MAX_APP_EVENT_BURST` 32, refill `CFE_PLATFORM_EVS_APP_EVENTS_PER_SEC` 15/s; checked after the binary filter; one "Events squelched" event (`CFE_EVS_SQUELCHED_ERR_EID` 44) when an app crosses the threshold; `SquelchedCount` saturates at 255) | `cfe/modules/evs/fsw/inc/cfe_evs_internal_cfg.h` L86-100; `cfe/modules/evs/fsw/src/cfe_evs.c` L141-157; `cfe_evs_utils.c` L249-361; `cfe_evs_task.h` L61 | Console: exactly 1 "Events squelched, AppName = SBN" in all 13 logs. Memory: SBN `SquelchedCount` **13 at S1, 17 (18 in A3) at S2** in all six runs; no other app squelched. So at least 13 SBN events are lost from all outputs before S1 and 4–5 more during the window. SBN's own init-complete event ("initialized (ProcessorID=…", `sbn_app.c` L1520-1528) appears in **none** of the 13 logs. |
| "No subscribers" (SB event 14, filter `CFE_EVS_FIRST_4_STOP`) | `cfe_sb_internal_cfg.h` L221-225; `cfe_sb_priv.c` L1083-1086 | Console: 4 in every log (the cap). `NoSubscribersCounter` 15–26 per M1 run. These are messages with no destination (mostly EVS event packets 0x808 before TO_LAB subscribes), not queue drops. |
| EVS local event log (20 entries, discard mode) | `cfe_evs_internal_cfg.h` L127-128, L193-194 | `LogFullFlag` 1 and `LogOverflowCounter` 89–103 at S2 (both are copied into the HK packet only at EVS housekeeping, every 4.2 s, so S1 shows 0). Events are still sent to the console; only the local log loses them. |
| Event-recursion guard | `cfe_sb_priv.c` L784-790 | POEC exceeds the event-25 filter count by 1–10 per run (e.g. A3: 4 drops, 2 attempts reached EVS), so some overflows never produce an event attempt at all. |

## 4. Scheduling: requested vs actual

Mechanism: OSAL asks for `SCHED_RR` for the main thread; `pthread_setschedparam` fails with EPERM (`RLIMIT_RTPRIO` 0, no `CAP_SYS_NICE`; console "Could not setschedparam in main thread: Operation not permitted (1)"), so `EnableTaskPriorities` stays false (`osal/src/os/posix/src/os-impl-tasks.c` L409-421). Every `OS_TaskCreate` then overwrites the task's priority with `OS_MAX_TASK_PRIORITY` 255 (L612-615) and creates the thread with default attributes (L523-563 apply only when priorities are enabled).

| Task | Requested | Source of the request | OSAL record (memory) | Kernel (all 12 snapshots) |
|---|---|---|---|---|
| CFE_ES | 68 | `cfe_es_objtab.c` L40; `cfe_es_internal_cfg.h` L43-44 | 255 | SCHED_OTHER, rt 0, nice 0 |
| CFE_EVS | 61 | `cfe_evs_objtab.c` L38; `cfe_evs_internal_cfg.h` L43-44 | 255 | same |
| CFE_SB | 64 | `cfe_sb_objtab.c` L37; `cfe_sb_internal_cfg.h` L339-340 | 255 | same |
| CFE_TBL | 70 | `cfe_tbl_objtab.c` L37; `cfe_tbl_internal_cfg.h` L50-51 | 255 | same |
| CFE_TIME | 60 | `cfe_time_objtab.c` L37; `cfe_time_internal_cfg.h` L233-234 | 255 | same |
| TIME_TONE_TASK, TIME_ONEHZ_TASK | 25, 25 | `cfe_time_task.c` L160-180; `cfe_time_internal_cfg.h` L236-240 | 255 | same |
| ES_BG_TASK | 200 | `cfe_es_backgroundtask.c` L40-43, L214-220 (`CFE_PLATFORM_ES_PERF_CHILD_PRIORITY`, `cfe_es_internal_cfg.h` L616-617) | 255 | same |
| SCH_LAB, CI_LAB, TO_LAB, SAMPLE_APP | 35, 40, 45, 50 | `build-native_std/exe/cpu1/cf/cfe_es_startup.scr` L3-6 | 255 | same |
| LC, CF, DS, FM, HK, HS, MM, SC, MD, CS, SBN | 70 each | `cfe_es_startup.scr` L7-17 | 255 | same |
| FM_CHILD_TASK | 205 | `apps/fm/fsw/src/fm_child.c` L86-92; `fm_internal_cfg.h` L237-238 | 255 | same |
| main thread (`core-cpu1`, tid = pid; blocked in `rt_sigtimedwait`) | RT max (99) requested and refused | `os-impl-tasks.c` L391, L409-414 | — | same |
| `core-cpu1` thread blocked in `futex` (inferred: OSAL console/utility task) | 10 (`OS_UTILITYTASK_PRIORITY`, `OSAL_CONFIG_UTILITYTASK_PRIORITY`) | `os-impl-console.c` L46, L131-135; generated `osconfig.h` L205 | (internal thread, not in the task table) | same |
| `core-cpu1` thread blocked in `rt_sigtimedwait`, about 10 wakeups/s (inferred: PSP soft timebase "cFS-Master" handler) | 0 (OSAL "elevated") | `os-impl-timebase.c` L350-355 | (internal) | same |

The requested priorities are in ES's own task record (`TaskTable[].StartParams.Priority`, read from memory), so the "requested" column is measured too. The identification of the three unnamed threads is inferred from their blocking system call and creation order; it is not read from OSAL. `kernel.sched_autogroup_enabled` = 1 and run_cfs.sh uses `setsid`, so each run was its own autogroup (`/proc/<pid>/autogroup`: autogroup-143, 147, 148, 150, 152, 153, all nice 0). The CFS scheduler thus shares the CPUs between the cFS session as a whole and the other sessions on the VM.

## 5. HS: code path and measured state

**Code path** (`apps/hs/fsw/src/hs_app.c`):

1. `HS_AppMain` L57: `RunStatus = APP_RUN`. L68 `HS_AppInit()`.
2. `HS_AppInit` initialises all of `HS_AppData` first (memset and defaults L218-233), then EVS registration L238, CDS L248-310, SB pipes and subscriptions L315 (`HS_SbInit` L362-442), and table registration and load L324 (`HS_TblInit` L449-586). **Only then** comes `HS_SysMonInit` (L333). It fails, HS sends event 74 (L336-339) and returns early (L340). The "HS Initialized" event `HS_INIT_INF_EID` (L346-352) is never sent. No field of `HS_AppData` is left uninitialised by the early return.
3. `HS_SysMonInit` (`hs_sysmon.c` L46-116) finds the PSP device "linux_sysmon" and stores its ID (L51, L58-60). `SET_RUNNING` then fails, because `linux_sysmon_Start` cannot open `/proc/schedstat` (`psp/fsw/modules/linux_sysmon/linux_sysmon.c` L318-325, after `memset(state)` L318), so `SysMonSubsystemId`/`SubchannelId` keep 0 (L66-73).
4. Back in `HS_AppMain`, L73-123 are skipped: `CFE_ES_WaitForStartupSync`, `CFE_PSP_WatchdogSet/Service/Enable`, and the event-message subscriptions. The loop L128-172 runs anyway. Its first `CFE_ES_RunLoop` sets HS's AppState to RUNNING (`cfe/modules/es/fsw/src/cfe_es_api.c` L465-471), and every call increments HS's `ExecutionCounter` (L441).
5. Each iteration waits on `HS_WAKEUP_PIPE` with `HS_WAKEUP_TIMEOUT` 1200 ms (L145; `hs_internal_cfg.h` L86). SCH_LAB never sends `HS_WAKEUP_MID` (`sample_defs/tables/sch_lab_table.c` has only `HS_SEND_HK_MID`, L70), so HS runs on the timeout. `HS_ProcessMain` (L593-658) does the following:
   * CPU utilisation every 30 cycles (L615-620; `HS_CPU_UTILIZATION_CYCLES_PER_INTERVAL` 30). `HS_SysMonGetCpuUtilization` (`hs_sysmon.c` L143-176) calls the device with subsystem 0 ("aggregate"). `linux_sysmon_calc_aggregate_cpu` returns `CFE_PSP_ERROR` because `num_cpus` is 0 (`linux_sysmon.c` L386-401). HS then stores `0xFFFFFFFF` as average and peak (`hs_monitors.c` L421-428). No event and no syslog entry results, and CPU hogging can never trigger (L436). The failing read happens once per 30 cycles (36 s), so once per 30 s window.
   * App monitoring is enabled (`HS_APPMON_DEFAULT_STATE`, `hs_internal_cfg.h` L224), but every entry of the default AMT table is `HS_AMTActType_NOACT` (`apps/hs/fsw/tables/hs_amt.c` L36-90), and NOACT entries are skipped (`hs_monitors.c` L67). No monitoring action can occur.
   * Event monitoring is **disabled by default** (`HS_EVENTMON_DEFAULT_STATE` = `HS_State_DISABLED`, `hs_internal_cfg.h` L237), so the skipped event subscriptions change nothing in the default configuration.
   * Aliveness is enabled: an `OS_printf(".")` every `HS_CPU_ALIVE_PERIOD` 5 cycles (L633-642; `hs_internal_cfg.h` L100, L115).
   * Watchdog: `CFE_PSP_WatchdogService()` is called every cycle (L652-655). On pc-linux, `CFE_PSP_WatchdogEnable` and `CFE_PSP_WatchdogService` are empty and `WatchdogSet` only stores the value (`psp/fsw/pc-linux/src/cfe_psp_watchdog.c` L85-87, L105-107, L126-129). Skipping set/enable changes nothing on this platform.

**Measured (all six M1 runs identical; `logs/m1_analysis.txt` §8):**

| Item | S1 | S2 |
|---|---|---|
| HS thread | present, `S` | present, `S` (not exited) |
| ES AppState | RUNNING | RUNNING |
| HS `ExecutionCounter` | 1 | 24 (+23 in about 28 s, i.e. one loop per ≈1.2 s, the wakeup timeout) |
| `RunStatus` | 1 (APP_RUN) | 1 |
| `SysMonPspModuleId` / Subsystem / Subchannel | 0x0110FF05 / 0 / 0 | same |
| `UtilCpuAvg` / `UtilCpuPeak` | 0 / 0 (not yet computed) | 0xFFFFFFFF / 0xFFFFFFFF |
| `UtilizationCycleCounter` / `CurrentCPUUtilIndex` | 0 / 0 | 7 / 1 (one utilisation read in the window) |
| `CurrentAppMonState`, `CurrentEventMonState`, `CurrentCPUHogState`, `ServiceWatchdogFlag` | 1, 0, 1, 1 | same |
| `AlivenessCounter`; console dots | 0 | 3; 4 dots per run (cycles 5, 10, 15, 20) |
| HS events after event 74 | — | **0** in all 13 logs (run01–run07 and M1) |
| HS exit or termination lines ("Application Terminating", HS syslog, ES restart of HS) | — | none in all 13 logs |

**Impact:** HS is alive and loops normally. Compared with a successful init, the observable differences on this platform with the default tables are: (1) no "HS Initialized" event; (2) no `CFE_ES_WaitForStartupSync`, so HS's loop does not wait for the other apps (HS had completed 1 RunLoop call by S1); (3) CPU utilisation is reported as unknown and hogging detection is inactive. HS's own pipes exist with the depths in §3.1.

## 6. Reset type and persisted data

### 6.1 What can persist, from source

| Item | Behaviour | Reference |
|---|---|---|
| `EEPROM.DAT` (run dir) | opened with `O_CREAT`, `ftruncate` to 512 KiB at every boot (which also refreshes its mtime), `mmap MAP_SHARED`. Nothing in the default run writes to it (only CS/MM commands would). Survives everything, including a VM reboot (on disk). | `psp/fsw/modules/eeprom_mmap_file/cfe_psp_eeprom_mmap_file.c` L41, L55-69, L145-170 |
| Keyfiles `.cdskeyfile`, `.resetkeyfile`, `.reservedkeyfile` | created if missing (`O_CREAT`) and used only for `ftok(file,'R')` | `psp/fsw/pc-linux/src/cfe_psp_memory.c` L65-67, L149, L328, L454, L611-615 |
| SysV segments (CDS 128 KiB, reset area, user reserved) | created with `IPC_CREAT`; on a POWER ON boot the PSP clears them (L640-660); the ES POWERON restart deletes them (`cfe_psp_support.c` L60-69). Never survive a VM reboot. | `cfe_psp_memory.c` L149-193, L328-392, L454-498, L640-660 |
| Boot record → reset type | without `-R`, the next reset type comes from the boot record in the reset-area segment; if the segment is new, the boot is POWER ON | `cfe_psp_start.c` L396-434 |
| `/dev/shm/osal:RAM` (`/ram`) | created by `mkdir(…, 0700)` if missing; "format" is a no-op, so files survive every boot. ES formats on POWER ON (L523) and only re-initialises on PROCESSOR (L545), which makes no difference here. Being tmpfs, it is emptied by a VM reboot. | `osal/src/os/posix/src/os-impl-filesys.c` L223-235, L254-256; `cfe/modules/es/fsw/src/cfe_es_start.c` L521-527, L545-562, L580 |
| POSIX queues | cannot persist (unlinked at creation) | `os-impl-queues.c` L142 |

### 6.2 Measured before and after every run (`logs/m1_*_persist_*.txt`)

| Run | Before | After |
|---|---|---|
| A1 (first cFS process after the reboot at 06:00:21) | no `/dev/shm/osal:*` at all; no SysV segment; EEPROM.DAT mtime 02:10:34 (from run07, before the reboot), sha256 `07854d2f…`, 0 non-zero bytes; keyfiles inode 2155973–5 (mtime 01:29:46), ftok keys 0x5200e5c5/6/7, the same keys as the pre-reboot segments in run02's `.meta` | `/dev/shm/osal:RAM` created (06:22:53, ubuntu, 0700, empty); no SysV segment; EEPROM.DAT same sha256, mtime 06:22:52 |
| A2, A3 | `osal:RAM` empty; no segment | same; EEPROM.DAT same sha256, mtime advanced |
| B1 | `osal:RAM` empty | `osal:RAM/cfe_sb_pipe.dat` 1624 B (06:25:29, sha256 `f63dd1cf…`) |
| B2 | **B1's `cfe_sb_pipe.dat` present** | overwritten (truncate) by B2's dump, sha256 `b317ec2e…` |
| B3 | B2's file present | B3's dump, sha256 `b67e40b2…` (06:26:41) |

* EEPROM.DAT kept sha256 `07854d2fef297a06…` and 0 non-zero bytes in all 12 records. Only its mtime changes (the `ftruncate` at boot), so mtime is not evidence of a content change.
* No SysV segment existed before or after any run. The PSP removed them at every stop (console "Critical Data Store Shared memory segment removed", …). So with the CI stop, the boot record never survives, and every boot is POWER ON ("Starting the cFE with a POWER ON reset", "POWER ON RESET due to Power Cycle") in 6/6.
* The run directory gained nothing but the EEPROM.DAT mtime (`files_created_or_modified_under_cwd_during_run` in each `.meta`). `cf/tmp` stayed empty.

### 6.3 The first run after the reboot (A1) starts differently

The console structure of A1 (reset lines and the set of events) is the same as in A2 and A3. The **timing and interleaving** are not:

| | A1 | A2, A3, B1–B3, run04, run06, run07 |
|---|---|---|
| Span of the 15 "Loading file … APP:" lines (cFE time) | **849.3 ms** | 2.2–3.3 ms |
| CORE_READY → OPERATIONAL (cFE time) | **1058.9 ms** | 103.2–104.6 ms |
| OPERATIONAL line read (ms since launch) | **2570** | 1613–1636 |
| App init reports printed before the last app was loaded | 10 | 0 |

In A1, the early apps (SCH_LAB, SAMPLE_APP, CI_LAB, CF, LC, …) initialised while later apps were still being loaded. In every other run, all apps were loaded before any of them initialised. The cause was not measured. It is consistent with a cold file cache after the reboot (the `.so` files must be read from disk), but no claim beyond the timing is made. **For the research this is a persisted-state effect outside cFS:** the first run after a VM reboot has a different startup order from the following runs.

### 6.4 Host state added by M1 (not removed, per the rules)

* `/dev/shm/osal:RAM/` (created by A1; this happens to the first cFS run after any reboot).
* `/dev/shm/osal:RAM/cfe_sb_pipe.dat` (1624 B, from B3). It stays visible to every later cFS run as `/ram/cfe_sb_pipe.dat` until the VM reboots. Nothing in the default configuration reads it, and a POWER ON boot does not inspect `/ram` (the free-space check runs only on PROCESSOR resets, `cfe_es_start.c` L602). It is still a difference in the initial state of later runs.
* `env/tools/bin/mq_probe` (build artifact) and `env/tools/__pycache__/` (left by a `py_compile` check).

## 7. App status split: (a) created, (b) own init completed, (c) running at S2

Init-complete report used for (b), per app (exact console event):

| App | (b) evidence | App | (b) evidence |
|---|---|---|---|
| CFE_ES | `CFE_ES 1: cFE ES Initialized` | DS | `DS 1: Application initialized` |
| CFE_EVS | `CFE_EVS 1: cFE EVS Initialized` | FM | `FM 1: Initialization complete` (child: `FM 72: Child Task initialization complete`) |
| CFE_SB | `CFE_SB 1: cFE SB Initialized` | HK | `HK 1: HK Initialized.` |
| CFE_TBL | `CFE_TBL 1: cFE TBL Initialized` | **HS** | `HS 1: HS Initialized` (`HS_INIT_INF_EID`) — **never present; init failed (event 74)** |
| CFE_TIME | `CFE_TIME 1: cFE TIME Initialized` | MM | `MM 1: MM Initialized.` |
| SCH_LAB | `SCH Lab Initialized.` (`OS_printf`; no EVS event and no timestamp) | SC | `SC 9: SC Initialized.` |
| CI_LAB | `CI_LAB 3: CI Lab Initialized.` | MD | `MD 1: MD Initialized.` |
| TO_LAB | `TO_LAB 1: TO Lab Initialized.` | CS | `CS 1: CS Initialized.` |
| SAMPLE_APP | `SAMPLE_APP 1: Sample App Initialized.` | **SBN** | `SBN 4: initialized (ProcessorID=…` (`sbn_app.c` L1520-1525) — **never present: squelched (§3.5)** |
| LC | `LC 2: LC Initialized.` | CF | `CF 20: CF Initialized.` |

Results (`logs/m1_analysis.txt` §7):

* **(a) created:** 20/20 apps in all 13 logs (ES "Loading file … APP: X" for the 15 script apps; "Calling EarlyInit for CFE_X" for the 5 core apps). In M1, a thread with each app's name existed at S1 (27 threads: 24 named tasks + 3 unnamed).
* **(b) init completed (console):** 18/20 in all 13 logs. Missing: **HS**, whose init failed (§5), and **SBN**, whose report is lost to squelch. SBN's init did complete: `SBN_AppMain` enters its loop with `RunStatus = APP_RUN` only if `Init()` succeeded (`sbn_app.c` L1657-1663), and its `ExecutionCounter` grows (below). So for SBN, (b) rests on memory evidence, not on the console.
* **(c) at S2 (M1 runs):** all 20 main tasks present (state `S`), AppState RUNNING, and the main-loop counter advanced from S1 to S2 in every run. Typical deltas: CFE_ES/EVS/SB/TBL +5–6, CFE_TIME +115–119, SCH_LAB +274–276, CI_LAB +59–61, TO_LAB +542–567, CF +552–564, SBN +279–286, HS +23, DS +58–60, SC +60, CS +57–59, HK +45–46, LC/FM/MM +29–30, MD +28–29, SAMPLE_APP +5. Child tasks (no RunLoop counter): ES_BG_TASK, TIME_TONE_TASK and TIME_ONEHZ_TASK gained 31–51 voluntary context switches. FM_CHILD_TASK is alive but idle (no context switch; it waits on its semaphore because no FM command is sent).
* run01–run07: (a) and (b) as above; (c) was not measured.

## 8. D2 feasibility facts (nothing was changed)

From `logs/m1_d2_feasibility.log`, written after all runs:

| Fact | Value | Reference |
|---|---|---|
| `/proc/sys/fs/mqueue/msg_max` | mode 0644, owner root:root, value 10 (= kernel default `DFLT_MSGMAX` 10); `msgsize_max` 8192, `queues_max` 256 | `include/linux/ipc_namespace.h` L119-122 |
| Can root write it? | **Yes**: writing the identical value `10` as root (euid 0) returned success and the value read back 10. The sysctl grants owner write to the namespace's root uid (`mq_permissions`), with limits `MIN_MSGMAX` 1 to `HARD_MSGMAX` 65536. | `ipc/mq_sysctl.c` L17-18, L32-38, L92-108 |
| Root capabilities | `CapEff 000001fffeffffff` = all except `CAP_SYS_RESOURCE`. Without it, root is also bound by `msg_max` (the bypass up to `HARD_MSGMAX` requires `CAP_SYS_RESOURCE`) and by `queues_max`. | `ipc/mqueue.c` L352-359, L582-586; CONDITIONS.md D-4 |
| IPC namespace | this session: `ipc:[4026531839]` = `IPC_NS_INIT_INO` 0xEFFFFFFF, i.e. **the initial IPC namespace of the VM kernel** (likewise `user:[4026531837]` = `USER_NS_INIT_INO`). PID 1 (`process_api`, uid 0): its `/proc/1/ns/*` links cannot be read even by root (permission denied), so equality with PID 1 could not be read directly; it follows from the inode being the initial-namespace constant. | `include/uapi/linux/nsfs.h` L48-50 |
| Consequence | `msg_max` is per IPC namespace and this is the initial one, so a change would apply to every process in the VM, including other workflows. The bundle CI's `--sysctl fs.mqueue.msg_max=64` applies to a container's own IPC namespace instead (`actions/start-cfs-container/action.yml` L29-34). A private IPC namespace for cFS (e.g. `unshare --ipc`; root has `CAP_SYS_ADMIN`) would also make the PSP's SysV segments private. That was **not tested**. | — |
| `RLIMIT_MSGQUEUE` of the run user | 819 200 B soft and hard (as `core-cpu1` sees it in every `.meta`) | — |
| Kernel charge per queue | `mq_maxmsg × sizeof(struct msg_msg) + min(mq_maxmsg, MQ_PRIO_MAX) × sizeof(struct posix_msg_tree_node) + mq_maxmsg × mq_msgsize`. **Measured** on this kernel as `mq_maxmsg × (96 + mq_msgsize)` for maxmsg 1, 5, 10 and msgsize 8, 64, 8192 (binary search on the soft limit as ubuntu, `logs/m1_tooltest.log` §2). The formula is the one documented for `RLIMIT_MSGQUEUE` in getrlimit(2) (the lead's note cited mq_overview(7), which does not carry it). | `ipc/mqueue.c` L365-371, L377-378 |
| Charge of the measured queue set (26 queues, msgsize 8) | now (depth ≤ 10): 247 × 104 = **25 688 B**; at `msg_max` 64 with the requested depths (none above 50, so none truncated): 584 × 104 = **60 736 B** (7.4 % of the limit); upper bound with all 26 at depth 64: 26 × 64 × 104 = **173 056 B** (21 %). `queues_max` 256 ≥ 26. OSAL would still reject a request above `OS_QUEUE_MAX_DEPTH` 50 (`osapi-queue.c` L98). | computed from §3.1 and the measured charge |

## 9. What this changes in CONDITIONS.md (proposals; the existing rows were not edited)

New rows were added to CONDITIONS.md, marked "added 2026-10-08 (M1)": H-7 (the hook) and a section "M. Measured baseline (M1)" with rows M1-1 to M1-9. The rows below are proposed replacement texts for the PI and the lead.

**E4 (Task priorities), proposed Value/Notes:**
> Best effort, measured. Three levels per task: (1) requested: ES record `StartParams.Priority`, equal to the startup script (35–70) and core/child constants (ES 68, EVS 61, SB 64, TBL 70, TIME 60, TIME tone/1Hz 25, ES_BG_TASK 200, FM_CHILD_TASK 205); (2) OSAL record: 255 for every task, because `OS_TaskCreate_Impl` overwrites the priority when `pthread_setschedparam` failed for the main thread (`os-impl-tasks.c` L409-421, L612-615); (3) kernel: `SCHED_OTHER`, rt_priority 0, nice 0 for all 27 threads (24 named tasks, main thread, two internal OSAL/PSP threads) in all 12 M1 snapshots. Each run is its own autogroup (`sched_autogroup_enabled` 1, `setsid`). Status: from-reference (README L102), measured (BASELINE_MEASURE.md §4).

**E5 (Message-queue depth), proposed replacement of the "Run-to-run effect observed" sentence and the SBN item:**
> Measured kernel capacity (`mq_getattr` through `/proc/<pid>/fd`): 26 queues, all SB pipes, `mq_msgsize` 8; `mq_maxmsg` = min(request, 10); total 247 instead of 584; OSAL keeps the requested depth in its own record. SBN per-peer pipes are created only when a peer connects (`sbn_app.c` L161-181) and do not exist here. **Losses:** the SB counter `PipeOverflowErrorCounter` (every `OS_QUEUE_FULL`) was 33, 37, 4, 49, 45 and 41 in six identical 30 s runs (M1), all at startup (before the S1 read, at most 56 ms after the OPERATIONAL line was read) and 0 in the following 28 s. The console "Pipe Overflow" event is filtered `CFE_EVS_FIRST_16_STOP` for all pipes together, so run01–run07 (16 lines each) were capped and their per-pipe counts (SBNSubPipe 8–16, DS_CMD_PIPE 0–8) are lower bounds, not totals. No per-pipe drop counter exists: the pipe-info `SendErrors` counts only MsgId-limit errors (`cfe_sb_priv.c` L1058-1063), which were 0. Pipes with `PeakQueueDepth` above the kernel depth: SBNSubPipe (5 of 6 runs), DS_CMD_PIPE (4), TO_LAB_TLM_PIPE (4).

**E7 (Reset type), proposed addition to Notes:**
> Measured (M1, BASELINE_MEASURE.md §6): after the ES POWERON stop no SysV segment remains, so every following boot is POWER ON (6/6). `EEPROM.DAT` is all zero (sha256 `07854d2f…`, unchanged in 12 records); its mtime changes at every boot through `ftruncate`, not through a write. Files on `/dev/shm/osal:RAM` survive POWER ON boots (B1's pipe-info file was present at the start of B2). A VM reboot empties `/dev/shm` and the SysV segments but keeps `EEPROM.DAT` and the keyfiles (same ftok keys). **The first run after a reboot has a different startup order:** apps were loaded over 849 ms instead of 2–3 ms and initialised while loading; OPERATIONAL came at 2570 ms instead of 1613–1636 ms.

**E11 (Stop method), proposed addition to Notes:**
> (a) was used for the M1 characterisation runs (6/6 exit status 0, POWERON status; the process ended 154–556 ms after the stop command was sent, within one CI_LAB read period, E10). This does not decide D3/E11. With a measurement hook at S2 the stop comes after the hook returns (`run_cfs.sh` v3, H-7).

**D-3 (`/proc/schedstat`), proposed replacement of "Impact":**
> **Impact (measured, BASELINE_MEASURE.md §5):** `HS_AppInit` returns at `HS_SysMonInit` after all of `HS_AppData`, the pipes, the subscriptions and the tables are set up, so nothing is left uninitialised. HS skips only the init event, `CFE_ES_WaitForStartupSync`, the watchdog set/enable (empty functions on pc-linux, `cfe_psp_watchdog.c` L85-129) and the event-message subscriptions (event monitoring is disabled by default, `hs_internal_cfg.h` L237). HS runs its main loop on the 1200 ms wakeup timeout (+23 iterations between S1 and S2, about 28 s), prints the aliveness dot, reports CPU utilisation as `0xFFFFFFFF` (one failing read per 30 cycles; no event), never detects CPU hogging, emits no event after event 74 and does not exit (13 logs, 6 memory-checked runs).

**ENV.md §6 (state left behind):** should list `/dev/shm/osal:RAM/cfe_sb_pipe.dat` (§6.4).

## 10. Claims we still must not make

* **Any per-pipe loss count.** cFE has no per-pipe queue-full counter. The console lists only the first 16 attempts that reach EVS, and `PeakQueueDepth` above the kernel depth is an indicator, not a count. Only the per-run total (POEC) is measured.
* **True loss totals for run01–run07.** All seven logs are at the 16 cap.
* **That the drop counts are a property of the configuration.** They ranged from 4 to 49 in six identical runs, and the S1 probe ran concurrently with the drop burst (§1.3); n = 3 per variant is a characterisation assumption.
* **That the window has no loss of any kind.** The window had no queue-full drop, but EVS squelch (SBN), the EVS local log and the "No subscribers" path still lose events or messages during it.
* **That priorities, `msg_max` 64 or root mode would remove the drops.** Nothing was run in those modes.
* **That A1's slower start is caused by the file cache.** Only the timing was measured.
* **That the memory read is atomic or exactly time-aligned with the console.** It is a lock-free copy taken 5–7 ms long; it agrees with cFE's own dump (§2.3), but values that change during the copy can be off by those changes.
* **That PID 1 is in the same IPC namespace.** This was inferred from the initial-namespace inode constant; it could not be read.
* **Anything about the CI environment's queue behaviour** (`msg_max` 64, fresh container per run). Nothing here was run that way.
* **That SBN's init-complete event was emitted.** It was suppressed; SBN's init is inferred from its running loop.

## 11. Files

* `env/run_cfs.sh` (v3: optional `RUN_CFS_HOOK`), `env/measure_baseline.sh`
* `env/tools/`: `mq_probe.c` (built to `tools/bin/mq_probe`), `cfs_layout.py`, `cfs_snapshot.py`, `sb_pipeinfo_parse.py`, `inotify_wait.py`, `m1_hook.sh`, `persist_snapshot.sh`, `m1_analyze.py`, `tooltest_mqdummy.c`, `tooltest_mqacct.c`
* `env/logs/`: `m1_{A1,A2,A3,B1,B2,B3}_po_fixed30s_esrestart.log` (+ `.meta`, `.hook`, `.hookconf`, `.S1/.S2[/.S2b].{json,txt}`, `.pipeinfo.{dat,json,txt}` for B), `m1_<run>_persist_{before,after}.txt`, `m1_<run>_driver.txt`, `m1_layout.json`, `m1_analysis.txt`, `m1_tooltest.log`, `m1_d2_feasibility.log`

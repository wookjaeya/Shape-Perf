# R2 (cFE #2663) on the ENV build: execution evidence under gdb

- Date: 2026-10-08. Policy: `../CONDITIONS_POLICY.md`.
- **What this is.** Step 3 of the review's order: one case run in real cFS with path-execution evidence.
  - Build: the **unmodified ENV build** (cFS v7.0.1, cFE `c5fb2b4d`).
  - Method: the **reporter's method**, gdb breakpoints.
- **What this is not.**
  - Not a CORD result.
  - Not an existing-tool result.
  - Not evidence about unperturbed schedules: every breakpoint hit stops all threads for a moment.
- Conditions, sites and verdict rules: `../cases/mapping_v701.md` §1. Script: `../tool/gdb/r2_trace.py` (sha256 `b0605ca6…`). Summarizer: `../tool/gdb/r2_summarize.py`. Driver: `../env/run_r2_gdb.sh`, which uses `../env/run_cfs.sh` v4 with `RUN_CFS_WRAP`, the new row H-8.
- Logs: `../env/logs/r2_g{1,2,3}_*`. Per run these are:
  - the console;
  - `.meta`;
  - the persisted-state snapshots before and after;
  - the raw trace (`_trace.jsonl`);
  - the summary (`_summary.txt`, `_summary.json`).

## 1. Conditions actually used

| Item | Value | Status |
|---|---|---|
| Build, user, cwd, command | ENV E1–E3 (`ubuntu`, `build-native_std/exe/cpu1`, `./core-cpu1`, no options) | from-reference |
| Observation | gdb 15.1, `-batch -nx`, all-stop. Each breakpoint records a JSON line and continues. gdb defaults are kept: ASLR disabled, start through a shell. Real-time and timer signals pass silently. The inferior environment has `R2_TRACE_OUT`, `LINES` and `COLUMNS` removed, so it equals E12. | Method from-reference (issue: gdb breakpoint). Version and mode `unspecified_by_reference` (assumption; R2 A4, A6) |
| Reporter's breakpoint | `cfe_evs.c` L188 (`Status = CFE_EVS_APP_ILLEGAL_APP_ID;`). At the report date this statement was `main#L185`; it was moved by content (mapping §1.1) | from-reference (moved) |
| Run length, stop | E10 `operational`, E11 (a) CI ES restart (POWERON) | from-reference. The experimental stop method is still PI decision D3 |
| Runs | 3 (r2_g1–r2_g3), sequential, each booting POWER ON | Count `unspecified_by_reference` (assumption) |
| Persisted state at start | `/dev/shm/osal:RAM/cfe_sb_pipe.dat` (1624 B) left by M1 run B3. EEPROM.DAT all zero. No SysV segments (`../env/logs/r2_g*_persist_before.txt`) | Recorded. Not cleaned (policy) |

Each run passed the isolation check and exited with status 0, through POWERON reset. gdb printed `[Inferior 1 (process N) exited normally]`.

OPERATIONAL was read at 2693, 2244 and 2257 ms. Without gdb it was 1613–1636 ms (ENV run04–run07; M1 runs A2–B3). This is the measured slowdown from the debugger.

## 2. Results

**Path execution.** In all three runs every observation point was hit.
- The publication (`cfe_sb_task.c` L138 → L142) ran once, on the SB task's thread (`CFE_SB_TaskMain` in the stack). The stored value was `1114115` in all runs.
- SB's EVS registration (L155) followed immediately; status 0.

**Uses and verdicts** (identical in r2_g1, r2_g2 and r2_g3):

| Verdict | Count | Task | SB call | SB event ID |
|---|---|---|---|---|
| use before publication → `ILLEGAL_APP_ID` | 17 | `CFE_ES_TaskMain` | `CFE_SB_MessageTxn_ReportSingleEvent` | 14 `SEND_NO_SUBS` |
| | 2 | `CFE_ES_TaskMain` | `CFE_SB_SubscribeFull` | 10 `SUBSCRIPTION_RCVD` |
| | 1 | `CFE_ES_TaskMain` | `CFE_SB_CreatePipe` | 5 `PIPE_ADDED` |
| | 2 | `CFE_EVS_TaskMain` | `CFE_SB_SubscribeFull` | 10 |
| | 1 | `CFE_EVS_TaskMain` | `CFE_SB_CreatePipe` | 5 |
| | 1 | `CFE_EVS_TaskMain` | `CFE_SB_MessageTxn_ReportSingleEvent` | 14 |
| **total violating uses** | **24** (ES 20, EVS 4) | | | |
| use between publication and registration | 0 | | | |
| ordered (after both) | 199 / 205 / 205 | TBL, TIME, SB itself and all 15 apps | | |

- Every violating use passed `AppID = 0` (`CFE_ES_APPID_UNDEFINED`, the EarlyInit value). Every one reached `cfe_evs.c` L188, so the outcome was `CFE_EVS_APP_ILLEGAL_APP_ID`. The SB callers discard this status, so **24 SB events were lost silently in every run.** Nothing was printed, no counter changed, and no syscall was made.
- Sequence: all violating uses are hits 1–47. The publication is hit 50. The first ordered use is hit 52.
  - The last violating use came ~97 ms (96.9–97.4 ms) before the publication, under gdb. This matches the ~100 ms spacing between core-task initializations on the console. That spacing comes from the S1 serialization (`CFE_ES_MainTaskSyncDelay`, `cfe_es_start.c` L835).
  - Inference: the order comes from S1 plus the core object order, not from a close race.
- The 17 `SEND_NO_SUBS` uses from ES follow ES's own init events. Those are EVS event packets, MsgId 0x808, transmitted on SB before anything subscribes to them. Inference, from the event ID and the console's later "No subscribers for MsgId 0x808" lines.

## 3. Comparison with the case record

- **R2 §9** predicted a lower bound of "≥ 9 at M/D", from code reading. **Observed: 24**, with ES and EVS as the affected tasks.
- **R2 Q1** was the report's "the ES and the EVS tasks". At `c5fb2b4d` both are affected, consistent with the report and with R2 §3 ("ES and EVS precede SB since b9bec567").
- **Race or deterministic (R2 D2, roadmap §7.3).** The same 24 uses were seen in 3 of 3 runs under gdb, at the same hit positions. This supports the code-reading account (S1 forces the order), but **it is not a proof of determinism.** The runs are few, they are under gdb, and they all start from POWER ON with the same persisted state.

## 4. Review state table (검토 §8.1)

| State | R2 under gdb on the ENV build |
|---|---|
| install/run failure | no |
| case path not executed | no: publication and use sites hit in 3/3 |
| events not recorded | no: all six points recorded |
| recorded but not judged | no |
| order violation detected | **yes**: 24 observed violating uses per run, by the oracle of P4 (a), pending |

## 5. Claims this supports, and claims it does not

- Supported:
  - On cFS v7.0.1 (cFE `c5fb2b4d`), default configuration, normal user, under gdb, every boot in 3 of 3 runs had 24 SB events emitted by the ES and EVS tasks with SB's unpublished AppId. All were rejected by EVS (`ILLEGAL_APP_ID`) and lost silently.
  - The loss is invisible on the console, in counters and at the syscall level. Only in-process observation shows it: here a debugger, the reporter's method.
- Not supported:
  - Anything about runs without gdb. The violating set is expected to be the same by code reading, because S1 orders it, but this was not measured.
  - That the order is deterministic.
  - That existing tools miss it. No tool was run.
  - Anything about C3, which happens on a different revision and has a different cause (X4).

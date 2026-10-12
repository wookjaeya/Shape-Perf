# R2 (cFE #2663) on the ENV build: execution evidence under gdb

- Date: 2026-10-08. Policy: `../CONDITIONS_POLICY.md`. Revised 2026-10-12: §5 updated, §6 (units of observation) and §7 (independent recount of the raw traces) added, in answer to `../Architecture/history/review_EXT_20261012.md` §2.2 and §5.4. §1–§4 are unchanged.
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
- The 24 are dynamic occurrences in one boot. They are not 24 defects. For the other units (cause, static sites, contexts, runs) see §6. For the 2026-10-12 recount of the raw traces see §7.

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
  - Anything about this unmodified binary without gdb. The violating set is expected to be the same by code reading, because S1 orders it, but on this binary it was not measured. The CORD runs r2_c4–r2_c6 saw the same violating set without a debugger, but on a modified (instrumented) binary with a different observation method. They are a separate experimental group, not pooled with these runs. CORD is retired, so they are cited only as an earlier observation (`R2_cord_v701.md` §7, §8; `../tool/RETIRED.md`).
  - That the order is deterministic or fixed for all executions. Three runs with the same result do not prove it (§6).
  - That the 24 occurrences are 24 independent defects (§6).
  - That Kairos detected or certified R2. Kairos is not implemented.
  - That existing tools miss it. No tool was run.
  - Anything about C3, which happens on a different revision and has a different cause (X4).

## 6. Units of observation

Added 2026-10-12, in answer to the external review (`../Architecture/history/review_EXT_20261012.md` §2.2 last paragraph, §5.4). The numbers in this document count different things, and they must not be added together or used as each other's sample size.

| Unit | Count | What is counted | Evidence |
|---|---:|---|---|
| Defect cause | **1** | SB stores its AppId only in `CFE_SB_AppInit` (`cfe_sb_task.c` L138), when the SB task runs. Until then the cell holds 0 from EarlyInit's `memset` (`cfe_sb_init.c` L50). The ES and EVS tasks are created and run to RUNNING before SB (S1: `cfe_es_start.c` L810, L835, core object order). During init they call SB APIs whose events carry `CFE_SB_Global.AppId`. | [F] `../cases/mapping_v701.md` §1.1; `../cases/R2.md` §7. [I] It is one cause because every violating occurrence reads the same unpublished cell and fails the same check (`cfe_evs.c` L188). |
| Static SB call sites that pass `CFE_SB_Global.AppId` to `CFE_EVS_SendEventWithAppID` | 39 | `cfe_sb_api.c` 34; `cfe_sb_priv.c` 1 (L786); `cfe_sb_task.c` 4 (call statements at L671, L742, L753, L763) | [F] ENV source at cFE `c5fb2b4d`, recounted 2026-10-12. Same number as the retired CORD patch (`R2_cord_v701.md` §2). |
| … of these, executed in r2_g1–r2_g3 | 5 | `cfe_sb_api.c` L275, L1183, L1199; `cfe_sb_priv.c` L786; `cfe_sb_task.c` L671 | [M] §7 |
| … of these, executed **before the publication** (the violating sites) | **3** | `cfe_sb_api.c` L275 (`CFE_SB_CreatePipe`, PIPE_ADDED); `cfe_sb_api.c` L1183 (`CFE_SB_SubscribeFull`, SUBSCRIPTION_RCVD); `cfe_sb_priv.c` L786 (`CFE_SB_MessageTxn_ReportSingleEvent`, SEND_NO_SUBS) | [M] §7. gdb's caller frame gives the first line of the call statement. The AppId argument of the same statement is on L277 and L1185, which are the lines the CORD records use. |
| Context-qualified templates, coarse (calling task × SB function) | **6** | ES × {CreatePipe, SubscribeFull, ReportSingleEvent}; EVS × the same three. Counts per boot: ES 1 / 2 / 17, EVS 1 / 2 / 1 (§2 table) | [M] §7 |
| Distinct recorded call strings (task main → SB site) | 12 | ES: CreatePipe 1, SubscribeFull 2 (`cfe_es_task.c` L377, L387), ReportSingleEvent 5. EVS: CreatePipe 1, SubscribeFull 2 (`cfe_evs_task.c` L297, L304), ReportSingleEvent 1. One ES string, `CFE_ES_GenerateVersionEvents` → `CFE_Config_IterateAll` → `CFE_ES_ModSrcVerCallback` → … → L786, ran **13 times** per boot, once per module "Version Info" event. The other 11 strings ran once each. | [M] from the gdb stacks; §7. [I] This is the granularity of the spec's templates, `(static actor, inlining call string, site)` (`../Architecture/spec/SPEC_FINAL.md` §1.5, ADR-012). Whether Kairos's templates coincide with these 12 is [U]: Kairos is not implemented, and its treatment of the callback through `CFE_Config_IterateAll` is not fixed. |
| Dynamic violating occurrences per boot | **24** | ES 20, EVS 4. Each is followed by an `ILLEGAL_APP_ID` result on the same thread. | [M] §2, §7 |
| Runs | **3** | r2_g1–r2_g3: unmodified ENV binary under gdb, sequential, each POWER ON from the same persisted state | [M]. The count is `unspecified_by_reference` (§1). |

What follows from this:
- **24 is not 24 independent defects.** All 24 occurrences come from one cause. They reach 3 static sites in 6 task × function contexts, or 12 call strings, and 13 of them are iterations of a single call string [M][I]. The per-boot number also depends on a build input. With a `NULL` module version table, the same cause gave 11 per boot, because the 13 version events are not sent (`R2_cord_v701.md` §4.2) [M]. So 24 is a property of this build and configuration, not a count of faults.
- **3 runs do not prove a fixed order.** They are 3 boots under an all-stop debugger, from the same persisted state, on one host. The same result in 3 of 3 runs agrees with the code-reading account (S1 forces the order), but it does not prove that the order holds in every execution [I]. A fixed-order claim needs the static argument and its stated assumptions: the generic K-ID contract plus framework rule R19 (`../Architecture/spec/SPEC_FINAL.md` §7.2, App. A.1). That argument is designed but not implemented [D].
- **The runs are not independent samples of one population.** Same binary, same host, same persisted state, same perturbation by gdb [I]. No rate or probability is estimated from them.
- **Groups are not pooled.** The CORD runs (r2_c1–r2_c3 with a `NULL` version table; r2_c4–r2_c6 with ENV's table restored) used a modified binary and a different observation method. Agreement between the groups is a cross-method observation, not 3 + 3 = 6 runs of one experiment (`R2_cord_v701.md` §8).

## 7. Cross-check: independent recount of the raw traces (2026-10-12)

**Source of the numbers checked.** The external review (`../Architecture/history/review_EXT_20261012.md` §2.2) recounted the three raw traces. Its numbers: 223 / 229 / 229 SB-caller uses; 24 uses before the publication in each run; AppId 0 in all 24; 24 matched `ILLEGAL_APP_ID` results; per caller function CreatePipe 2, SubscribeFull 4, ReportSingleEvent 18.

**Method.**
- A new read-only script was written for this check. It is not derived from `../tool/gdb/r2_summarize.py`. It was run with `python3 -I` from a scratch directory and is not part of the repository. The traces were not modified (sha256 below).
- Definitions:
  - *SB-caller use*: a `SEND_WITH_APPID` record whose `stack[1].func` starts with `CFE_SB_`.
  - *Before the publication*: `seq` lower than that of the single `SB_PUB` record.
  - *AppId 0*: `int(app_id) == 0`. The field is stored as a JSON string (`"0"`).
  - *Matched result*: an `OUT_ILLEGAL_APP_ID` record on the same LWP after the use and before the next use on that LWP. Two closing rules were tried: the next `SEND_WITH_APPID` from any caller (the summarizer's rule), and the next SB-caller use only. They give the same counts.
  - Each matched result was also checked to carry the same `app_id` and `event_id` as its use.
  - *Task*: the outermost `*TaskMain` frame of the stack. It is present in all 24 violating stacks. Stacks are capped at 14 frames (`r2_trace.py` `frames()`), which still reaches `OS_TaskEntryPoint` or `start_thread`.

**Result per run** [M]:

| Measure | r2_g1 | r2_g2 | r2_g3 | Review §2.2 | Verdict |
|---|---:|---:|---:|---|---|
| trace sha256 (first 8 hex) | `34759887` | `5dfc9a46` | `7b959389` | — | — |
| records (excluding the meta line) | 278 | 284 | 284 | — | — |
| `SEND_WITH_APPID` hits, all callers | 251 | 257 | 257 | — | — |
| … of which caller is not SB (all `CFE_TBL_LoadFinish`; none before `SB_PUB`) | 28 | 28 | 28 | — | — |
| **SB-caller uses** | **223** | **229** | **229** | 223 / 229 / 229 | confirmed |
| `SB_PUB` (count, seq, value, task) | 1, 50, 1114115, `CFE_SB_TaskMain` | same | same | — | — |
| `SB_EVS_REGISTERED` (count, seq, status) | 1, 51, 0 | same | same | — | — |
| **uses before `SB_PUB`** | **24** | **24** | **24** | 24 each | confirmed |
| … with AppId 0 (string `"0"`; `sb_appid_now` also 0) | 24 | 24 | 24 | 24 | confirmed |
| … with a **matched `OUT_ILLEGAL_APP_ID`** (same LWP, before the next use) | 24 | 24 | 24 | 24 | confirmed. Same AppId and event ID in all 24. |
| `OUT_ILLEGAL_APP_ID` records in total / unmatched | 24 / 0 | 24 / 0 | 24 / 0 | — | — |
| `OUT_NOT_REGISTERED` records | 0 | 0 | 0 | — | — |
| uses between `SB_PUB` and `SB_EVS_REGISTERED` | 0 | 0 | 0 | — | — |
| uses after both (ordered, including SB's own 5) | 199 | 205 | 205 | — | equals §2 |
| uses before `SB_PUB`, by SB function: `CreatePipe` / `SubscribeFull` / `MessageTxn_ReportSingleEvent` | 2 / 4 / 18 | 2 / 4 / 18 | 2 / 4 / 18 | 2 / 4 / 18 | confirmed |
| uses before `SB_PUB`, by task: ES / EVS | 20 / 4 | 20 / 4 | 20 / 4 | — | equals §2 |
| distinct static SB call sites executed by any SB-caller use (`cfe_sb_api.c` L275, L1183, L1199; `cfe_sb_priv.c` L786; `cfe_sb_task.c` L671) | 5 | 5 | 5 | — | — |
| uses before `SB_PUB`: distinct static SB call sites / task × function contexts / call strings | 3 / 6 / 12 | 3 / 6 / 12 | 3 / 6 / 12 | — | identical across runs (§6) |
| hit positions: violating uses / `SB_PUB` / first ordered use | 1–47 / 50 / 52 | same | same | — | equals §2 |
| last violating use → `SB_PUB` (gdb clock, under gdb) | 97.38 ms | 97.21 ms | 96.88 ms | — | equals §2 (96.9–97.4 ms) |

**Verdict.**
- Every number in review §2.2 is confirmed. None is refuted.
- The review's per-function split (2 / 4 / 18) and the per-task table of §2 group the same 24 occurrences in two ways. In §2, ES has `ReportSingleEvent` 17, `SubscribeFull` 2 and `CreatePipe` 1; EVS has `SubscribeFull` 2, `CreatePipe` 1 and `ReportSingleEvent` 1.
- The totals 223 / 229 / 229 count only SB-caller uses. The 28 `CFE_TBL_LoadFinish` hits per run pass TBL's identity, not SB's, and all come after the publication. They are outside R2.
- The difference between r2_g1 (199 ordered uses) and r2_g2/r2_g3 (205) lies only in ordered uses by apps, 0.6–1.0 s after the publication on the gdb clock. Most are TO_LAB `SendSubscriptionReport` and `ReportSingleEvent` uses: 22 + 16 in r2_g1, 26 + 21 in r2_g2, 27 + 22 in r2_g3. The rest are 0–3 `ReportSingleEvent` uses each by CS, HS, MD, HK and SBN. The gdb trace does not mark OPERATIONAL, so these uses are not placed relative to it. The violating set is identical in all three runs.
- What this check does **not** add: it re-reads the same three recorded executions. It is not a new run, and it does not widen any claim of §5.

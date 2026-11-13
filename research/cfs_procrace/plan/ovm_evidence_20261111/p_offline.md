# p_offline — offline probe of the SLF method (phase 5)

Subject: `R/plan/ORDER_VIOLATION_METHOD_20261111.md` (sections 3, 5, 7), R = `/home/user/Shape-Perf/research/cfs_procrace`.
Nothing was built or booted. No existing file was changed. All scripts and their outputs are in
`R/plan/ovm_evidence_20261111/probe/` (PR below; 79 KB in total).

Tags: `[M]` measured in this run (command id K1..K17, listed in section 6), `[F]` read in a file by me, `[I]` my inference, `[U]` unknown.
Path prefixes: V = `/home/user/work/procrace/cfs_ref/cFS` (cFS v7.0.1), B = `/home/user/work/redesign/step2a/tests_8c_fix`, W = `/home/user/work/w6`, C5R = `/home/user/work/c5run`, `641:` = `/home/user/work/c5/src/641/cFE-6.4.1-OSS-release/cfe/fsw/cfe-core/src/`.

Reading rule for section 2: the method has no classifier yet. I applied its S1 label definitions (doc 3.3) to the text of the relation rows with a script (PR/s1.py). The mapping from definition to row text is mine and is stated in 2.1. Where the row text cannot decide a label, the script says so instead of guessing.

## 0. Table: prediction of the method / what the artifacts show / evidence

| # | Prediction or assumption of the method (doc section) | What the artifacts show | Verdict | Evidence |
|---|---|---|---|---|
| 1 | The failure class can be read from the value alone: cFE severity bits 11 = fail, 01 = info; OSAL negative = fail; event type 3/4; exit status 3/4/8/9 (3.1 R-CLASS) | cFE v7.0.1: 129 status codes; severity 00 = 1, 01 = 17, 10 = 0, 11 = 111. OSAL: 42 negative, 1 zero, 0 positive. PSP: 15 negative. One test (bits 31-30 = 11) grades all 168 fail codes of the three headers. Event types are 1-4, run status 0-9 | CONFIRMED for "value gives class". Limits: 2 cFE fail-class codes are documented as routine poll or time-out outcomes and 7 OSAL negative names contain TIMEOUT, EMPTY or FULL; 91 value-returning public functions return no status | K1, K2, K3; section 1 |
| 2 | 6.4.x has the same layout and event type values (3.1) | `641:inc/cfe_error.h:132-136` same masks; `641:inc/cfe_evs.h:111-114` types 0x01-0x04; 125 literal codes (111 fail, 13 info, 1 success); 27 `CFE_OS_*` codes are aliases of OSAL names with no literal in that header | CONFIRMED | K4 |
| 3 | The known-case rows pass the S1 filter (7.4 item 1; falsified if a known row is filtered out) | A21 (c1_pre_tx) = R with a creator in another actor; B65 (r2_core) = J; B64 (c5_641) = J; C31 (c5_641) carries a fail-class absence constant. 0 of the known rows is filtered out | NOT FALSIFIED, with two reservations: rows 6 and 9 below | K5 |
| 4 | c1_pre_tx: 6 of 127 path A rows have a terminal chain (5.1) | 6 rows (A21, A45, A46, A47, A48, A125) over 5 call sites; only A21 has a creator in another actor | CONFIRMED | K5 |
| 5 | C1 fixed version: label H, bound B = 25 lookups x 100 ms read from the code (5.1) | Row A21 of c1_post_tx: loop `count-bounded trip upper-bound 25`, exit `(ne local:ret -17)`, delay call at `cf_cfdp.c:1011:17`; facts: `arg 0:const 100 CF_STARTUP_SEM_TASK_DELAY` | CONFIRMED (both numbers are in the facts) | K5, K7 |
| 6 | The two touchstones read correctly from the present facts: TIME's timebase check = T, B65 = J (3.3) | B65: `discarded 39, used 0, tested 0` = J. TIME: the row of the TIME check (r2_core A10) and the row of the SCH_LAB lookup that exits on failure (c1_pre_tx A24) have the same handling and flip text. RET_CONSTS gives a fail-class constant under `(ne local:OsStatus 0)` for both functions | B65 CONFIRMED. TIME = T is NOT readable from the rows; T and R are not separable with the present row text | K5, K9; 2.4 G2 |
| 7 | S1 leaves few rows; "the number of survivors is not measured" (0, 7.2) | Rows whose label the row text decides: 39/431, 39/431, 19/338, 56/178, 55/175. Rows labelled R or J (or path C with a fail-class absence constant): 18, 17, 13, 24, 23. Rows the method must send to S3 as UNDECIDED-EVAL: 267, 267, 242, 75, 74 (137, 137, 116, 28, 27 if a path B row without a callee check counts as label 0) | MEASURED. S1 on the present rows does not shrink the observation set below 55-75 % of the rows (29-38 % under the lenient reading) | K5; 2.2 |
| 8 | r2_core: 25 path B rows have a reader actor created before the writer; R2 is the one with label J (3.4, 5.5) | 25 rows; label J and fixed = B65 only; 2 are T?, 2 are 0, 20 have no decidable label. The order grade of all 25 is "fixed unconfirmed" | CONFIRMED (count); clause B item 1 is not met by the present facts, as the doc says | K5 |
| 9 | C3: 6.4.1 unordered, 6.4.2 ENFORCED (5.3) | Row B64 (c5_641) and row B62 (c5_642) are the same in actors, handling, feasibility and flip. The SB and EVS tasks are no actors in either build; the writer of `cfe_sb_t.AppId` is attributed to the four script applications and `main`. The fail-class constant in the row (`CFE_EVS_APP_ILLEGAL_APP_ID` when AppID >= 32) is not the absence result of the case (0xc2000002) | NOT DERIVABLE from the present facts; the two versions are not told apart. The doc lists the missing actors; it does not say that the row's constant is the wrong one | K5, K7, K9; 2.3 |
| 10 | C5: 6.4.1 row C31 possible (lock window), 6.4.2 row C30 proven-impossible (5.4) | Confirmed. On the same element 16 rows carry `CFE_ES_ERR_APPID` as absence result: 6.4.1 = 3 possible, 11 undecidable, 2 proven-impossible; 6.4.2 = 0, 11, 5 | CONFIRMED for the case row. 11 rows stay undecidable on the fixed version | K5 |
| 11 | Gate points hold no lock: C31 (doc: known), C1 and C3 (doc: [U]) (3.2, 7.4 item 4) | LOCKSET `held=0` at `cfe_es_apps.c:614:25` and `:637:15` (6.4.1), `cf_cfdp.c:993:19`, `bp_flow.c:132:18` (C1), `cfe_sb_task.c:198:5`, `cfe_evs_task.c:385:13` (6.4.1) | CONFIRMED at function level (locks held by callers were not checked) | K8 |
| 12 | C1 buggy, natural run: lookup -17 once, CF exits, BP creates the same name later (5.1 S3) | Present in 2 of 2 natural pre-fix records: return 0xffffffef, key text "BP0_tsem", `CFE_ES_ExitApp` call, later publish by BP | CONFIRMED. Not in the record: event type, exit status value | K12; 3.1 |
| 13 | C1 fixed, natural run: -17 once, then 0; nothing stands (5.1) | 2 of 2 natural post-fix records: `#46 0xffffffef; #75 0; #79 0`, no `CFE_ES_ExitApp` call | CONFIRMED | K12 |
| 14 | C1 opposite order: lookup 0, CF initialises; failures present in both orders cancel (3.3) | Existing forced-order record of the buggy build: CF lookups 0 for both keys, console "CF Initialized". BP's own probe lookups return -17 in the natural and in the forced record | CONFIRMED as a signal. The record was made with a hand-written hold plan and hand-placed sites, so it is not evidence produced by the method | K12 |
| 15 | R2 clause B items 2-4: every use before w fails, none after w returns that status, the failing uses are not repeated (5.5, 7.4 item 3) | Two records. Before w: 24 uses, 24 with 0xc2000003, threads CFE_ES 20 and CFE_EVS 4. After w: 271 and 252 uses, all result 0. CFE_ES and CFE_EVS have no use after w | NOT REFUTED on both records | K11; 3.2 |
| 16 | C5: the violating order is not in any record (5.4) | 6.4.1 natural boot: 112 lookups, 0 with 0xc4000001; 78 `result 0` lines, 0 other | CONFIRMED absent; a directed run is needed | K13 |
| 17 | C3: no run exists (5.3) | The 33-site list of the C5 boots has no site on an event-send function | CONFIRMED absent | K13 |
| 18 | Bounded waits come out of the facts (7.4 item 6) | ES soft bound 1000 and core bound 30000 are constant arguments of call sites in the facts; CF 25 and 100 too. TO_LAB 10000 and HS 65000 cannot be checked: no such call site in these five builds | PARTLY CONFIRMED | K17 |
| 19 | By-name lookups are a small channel; the method turns the failing ones into candidates (5.6) | C1 system: 3 call sites (SCH_LAB, CF, BP). Handling: exit 2, bounded retry then exit 1 (fixed CF, replaces the CF site), create on miss 1, ignored 0. Candidates with the present rows: 1 (CF) before the fix, 0 after | CONFIRMED; SCH_LAB's exit is not seen by the row | K14; section 4 |
| 20 | No noise floor exists for any build (3.2 S3) | No record gives one. Text proxy only: the R2 baseline console has 1 error-like event line unrelated to R2 (HS 74); C1 natural pre has 1 (the case), post 0 | STILL UNMEASURED (the text port has no type field) | K15 |

## 1. Failure vocabulary [M K1-K4]

cFE v7.0.1, `V/cfe/modules/core_api/fsw/inc/cfe_error.h`:
- 144 `#define` lines. 3 are not values (include guard, the macro `CFE_STATUS_C(X)`, `CFE_STATUS_STRING_LENGTH 11`). 12 are masks or field values (`CFE_SEVERITY_*` 4, `CFE_SERVICE_BITMASK`, 7 service ids). 129 are status codes, `CFE_SUCCESS` included.
- Cast: 125 `(CFE_Status_t)`, 4 `(int32)` (lines 203, 211, 219, 227).
- Severity bits 31-30: 00 = 1 (`CFE_SUCCESS`, line 147), 01 = 17, 10 = 0, 11 = 111.
- Reserved bits 29-28 non-zero: 0. Mission bits 24-16 non-zero: 0. 129 distinct values, no duplicate.
- Service bits 27-25: none 1, EVS 10, ES 39, FS 4, generic 10, SB 14, TBL 45, TIME 6.
- Name against class: 0 fail-class codes with INFO or WARN in the name; 1 info-class code with ERR in the name (`CFE_ES_ERR_SYS_LOG_TRUNCATED 0x44000029`, line 640). A reader of names would misgrade it; the value rule does not.
- The layout comment (lines 74-110) and the masks (lines 115-119) are the reference for the rule [F].

OSAL, `V/osal/src/os/inc/osapi-error.h`: 46 `#define` lines; 42 negative values, -1 to -108, no duplicate; `OS_SUCCESS (0)` line 96; 0 positive; 3 other (guard, two lengths). Rule text at lines 73-75, recommended test `status < OS_SUCCESS` at line 83 [F].

PSP, `V/psp/fsw/inc/cfe_psp_error.h`: 15 negative codes written `CFE_PSP_STATUS_C(-n)`, minimum -30 [M K2].

One test for all three: every negative 32-bit value between -1 and -108 has bits 31-30 = 11, and so has every cFE severity-error code. So "bits 31-30 = 11" grades the 111 + 42 + 15 = 168 fail codes without knowing which API returned the value [M K2]. The families can also be told apart by bits 29-28 (00 in all 129 cFE codes, 11 in a small negative) [M K1].

Event types (`V/cfe/modules/evs/config/default_cfe_evs_extern_typedefs.h`): DEBUG 1 (line 84), INFORMATION 2 (89), ERROR 3 (94), CRITICAL 4 (99). Run status (`V/cfe/modules/es/config/default_cfe_es_extern_typedefs.h`): 0-9 at lines 116-161; APP_ERROR 3, SYS_EXCEPTION 4, CORE_APP_INIT_ERROR 8, CORE_APP_RUNTIME_ERROR 9 [M K1].

Where the value does not settle the matter:
1. Fail-class codes that are routine: `CFE_SB_TIME_OUT 0xca000001` (line 732, "a packet has not been received in the time given") and `CFE_SB_NO_MESSAGE 0xca000002` (line 741, polling found no message) [F]. In OSAL, 7 negative names contain TIMEOUT, EMPTY or FULL: `OS_ERROR_TIMEOUT -4`, `OS_SEM_TIMEOUT -7`, `OS_QUEUE_EMPTY -8`, `OS_QUEUE_FULL -9`, `OS_QUEUE_TIMEOUT -10`, `OS_ERR_SEM_NOT_FULL -18`, `OS_ERR_EMPTY_SET -44` [M K1]. The method relies on the two-order difference to cancel these; the vocabulary alone does not.
2. Functions with no status. Regex census of prototypes after comment stripping [M K3]: cFE (9 public headers) 204 prototypes: status 133 (120 `CFE_Status_t`, 13 `int32`), void 18, bool 8, pointer 7, other value types 38. OSAL (`osapi-*.h`) 205 prototypes: `int32` status 151, void 16, bool 4, pointer 3, other 31. So 53 cFE and 38 OSAL value-returning functions (91 of 409) give R-CLASS nothing to grade. Examples: `CFE_ES_RunLoop` (bool), `CFE_SB_AllocateMessageBuffer` (pointer), `OS_TaskGetId` (id). The method names `CFE_ES_RunLoop` returning FALSE as a consequence in C5 (5.4); R-CLASS has no grade for it.
3. In the shipped call sites the event type is a constant almost everywhere: 484 of 486 event-send sites (c1_pre_tx), 196 of 204 (r2_core), 288 of 289 (c5_641) [M K6].
4. `CFE_ES_ExitApp` argument: constant at 8 of 14 call sites (c1_pre_tx), 8 of 9 (r2_core), 8 of 13 (c5_641); at the others it is a variable (`top ~RunStatus`, or a field) [M K6]. For CF the row gives the stored value (`CF_AppData_t.run_status := CFE_ES_RunStatus_APP_ERROR at cf_app.c:333:9`); for the lab applications the variable is a local and its value is not in the facts.
5. Application-defined status values were not surveyed [U].

6.4.1 [M K4]: `641:inc/cfe_evs.h:111-114` defines the four types as 0x01-0x04; `641:inc/cfe_error.h:132-136` has the same severity masks; 125 literal status defines (111 fail, 13 info, 1 success); 27 `CFE_OS_*` names are defined as OSAL names.

## 2. Static side on the step 2a facts

### 2.1 The query [M K5]

`python3 -I PR/s1.py B/<build>/NP/relations.txt` (output `PR/s1_<build>.out`). Rules, all on the text of one row:
- Fail-class constant: value < 0, or 0xC0000000 <= value <= 0xFFFFFFFF.
- Path A (a lookup use). Absence constants = the constants after "search returns:" on the flip line. H if there is a "probe-create" line, or an enclosing loop line with a delay whose exit compares a local with a fail-class constant. Else R if there is a "handling: terminal" line. Else J if a fail-class absence constant and `result=discarded`. Else T? if a fail-class absence constant and `result=tested`. Else 0 if constants are given and none is fail-class. Else E.
- Path B (a shared field passed as argument). Absence constants = constants of the "callee-check: F returns V" lines. J if a fail-class constant and discarded > 0; T? if fail-class, discarded = 0, tested > 0; 0 if a callee check without fail-class constant; E if no callee-check line or only "used".
- Path C (write after spawn, read by the child). F if the flip line gives a fail-class constant "with the earlier value"; 0 if constants and none fail-class; E otherwise.
- Order column: A unordered (every path A row is "undecidable"); B "fixed?" when a "reader actors created before it:" clause names an actor; C enforced when "proven-impossible".
- "T?" means: the status is tested and the row shows no terminal chain. The body of the failing branch is not in the row, so T and R cannot be told apart (2.4 G2). "E" means the row gives no absence result; under doc 3.2 such a site is UNDECIDED-EVAL and is observed in S3.
- R by "sends an ERROR event" is not applied: relation rows do not mention event sends (2.4 G1).

### 2.2 Counts per build [M K5]

| Build | Rows (A / B / C) | A: R, H, J, T?, 0, E | B: J, T?, 0, E | C: F, 0, E | Label decided by the row text | R or J (or C with F) | of them not enforced, and B flip not "none" | Distinct (path, element) in that set | UNDECIDED-EVAL (E) | E, lenient reading |
|---|---|---|---|---|---|---|---|---|---|---|
| c1_pre_tx | 431 (127 / 170 / 134) | 6, 1, 7, 103, 9, 1 | 13, 22, 3, 132 | 0, 0, 134 | 39 | 18 (A 5, B 13) | 12 (A 5, B 7) | 9 | 267 | 137 |
| c1_post_tx | 431 (127 / 170 / 134) | 4, 3, 7, 103, 9, 1 | 13, 22, 3, 132 | 0, 0, 134 | 39 | 17 (A 4, B 13) | 11 (A 4, B 7) | 9 | 267 | 137 |
| r2_core | 338 (68 / 155 / 115) | 0, 0, 1, 67, 0, 0 | 13, 10, 5, 127 | 0, 0, 115 | 19 | 13 (B 13) | 5 (B 5) | 5 | 242 | 116 |
| c5_641 | 178 (66 / 73 / 39) | 0, 0, 5, 30, 26, 5 | 8, 17, 1, 47 | 16, 0, 23 | 56 | 24 (B 8, C 16) | 22 (B 8, C 14) | 8 | 75 | 28 |
| c5_642 | 175 (66 / 71 / 38) | 0, 0, 5, 30, 26, 5 | 7, 16, 1, 47 | 16, 0, 22 | 55 | 23 (B 7, C 16) | 18 (B 7, C 11) | 7 | 74 | 27 |

Notes on the columns:
- "R or J" for path A counts only rows that have a creator in another actor (R rows without one: A45-A48 and A125 in c1_pre_tx).
- "Lenient reading": a path B row with no callee-check line is taken as label 0 instead of E (130, 130, 126, 47, 47 such rows). The doc does not say which reading it wants; the callee check looks one call level deep only, so "no callee check" is not a proof that the callee has no failing answer [I].
- S3 observation set by the method's own rule (R, J, F plus UNDECIDED-EVAL): 285, 284, 255, 99, 97 rows = 66 %, 66 %, 75 %, 56 %, 55 % of the rows. Lenient: 155, 154, 129, 52, 50 = 36 %, 36 %, 38 %, 29 %, 29 %.
- T? rows (not in either set): 125, 125, 77, 47, 46. Of the path A ones, 31 rows over 27 sites (c1), 20 rows over 17 sites (r2_core), 2 rows (c5) have a creator in another actor.
- Path A rows are not call sites: 127 rows over 113 sites in c1_pre_tx; `cf_cfdp.c:993:19` is row A21 (namespace `OS_common_record_t.name_entry`, 2 creators) and row A48 (namespace `OS_filesys_internal_record_t.virtual_mountpt`, no creator).
- Path C: 134 rows over 4 spawn sites and 13 fields (c1_pre_tx); 112 are "possible", over 12 (spawn, field) pairs; flip "none" 97, "static-undetermined" 37; no row of a v7-based build has an evaluated absence constant (0 of 134, 0 of 115).

### 2.3 The five cases [M K5, K7-K10]

| Case, build | Publish site in the facts | Consume site in the facts | Failing branch: status checked, what follows | Order information |
|---|---|---|---|---|
| C1 buggy, c1_pre_tx | yes: `bp_flow.c:132:18 -> OS_CountSemCreate` (4 lines in relations, 11 in facts), key image "BP0_tsem", "BP1_tsem" | yes: `cf_cfdp.c:993:19` (row A21; call site 5805) | checked (`result=tested`); return chain `CF_CFDP_InitEngine` -> `CF_Init`; terminal `CFE_ES_ExitApp` at `cf_app.c:366:5` with `run_status := CFE_ES_RunStatus_APP_ERROR`. The ERROR event at `cf_cfdp.c:997:17` is in the facts (`arg 1:const 3 CFE_EVS_EventType_ERROR`) with a branch condition on one of its arguments (facts.txt:66943, guard contains `(ne local:ret 0)`); it is not in the row | "undecidable"; consumer key "unresolved", so the row pairs CF with both BP and SCH_LAB |
| C1 fixed, c1_post_tx | same | yes: `cf_cfdp.c:1003:23` (A21) | retry loop bound 25, delay constant 100, then the same terminal chain | same |
| R2, r2_core | yes: `cfe_sb_task.c:138:5` (writer line of B65) | yes: 39 sites (B65) | `discarded 39, used 0, tested 0`; callee check `returns 3254779907 [CFE_EVS_APP_ILLEGAL_APP_ID] when ... (eq (call-result EVS_GetAppDataByID ...) 0)` | "fixed unconfirmed"; ES and EVS created before SB; flip "static-undetermined" |
| C3 buggy, c5_641 | site yes: `cfe_sb_task.c:198:5`, `cfe_evs_task.c:190:4` (value 4294967295) and `:385:13`. Actor no: SB and EVS tasks are not actors (9 roots; unresolved spawn `cfe_es_start.c:875:29`) | partly: `cfe_sb_api.c:264` is one of the 36 sites of B64; the TIME task's call `cfe_time_task.c:308` is in the facts (34 lines) and in no row | `discarded 36`. The absence result of the case is not in the row. It is in the facts in two steps: RETURNS `alt 2/3 call-result EVS_NotRegistered(AppID)` and RET_CONSTS `EVS_NotRegistered returns 3254779906 ... when=always`. Unchecked index: PARAM_FLOW `subscript ... cfe_evs_utils.c:244:18` | "undecidable: no static order"; same text in c5_642 (B62) |
| C3 fixed, c5_642 | same shape | same shape | range check read `cfe_evs_utils.c:620:27 ctx=(lt field:...EVS_AppID 32)` is in the facts | not derived (no HB-await; core tasks not actors) |
| C5 buggy, c5_641 | yes: spawn `cfe_es_apps.c:614:25` (locks `{}`), write `:649:18` (lock from `:637:15`) | yes: `cfe_es_api.c:861:41` (row C31) | absence constant in the row (`CFE_ES_ERR_APPID` with the earlier value). What the caller does with it is not in the row; it is in the facts: `sample_app.c:87:5 SAMPLE_AppInit -> CFE_EVS_Register direct {}` (no result flag), same for CI_LAB and TO_LAB; the five core tasks test it. `CFE_ES_GetAppID` itself: 17 of 22 call sites have no result flag | "possible: lock window" |
| C5 fixed, c5_642 | yes (`cfe_es_apps.c:619`) | yes | same | C30 "proven-impossible"; 14 such rows in the build |
| C2 | no pair in any build | — | — | nothing to test |

### 2.4 Where the facts or the rows do not contain what the method assumes

- G1. Event sends have no branch condition. The `[calls]` section has no guard column. A condition exists only on a field read that is an argument of the send. ERROR or CRITICAL sends with such a condition: 144 of 346 (c1_pre_tx), 26 of 123 (r2_core), 96 of 188 (c5_641) [M K6]. Label R by "sends an ERROR event on the absence branch" is therefore derivable for under half of the sends. C1's own send has one.
- G2. T against R. For a tested status the row says only "result tested; no return of it and no terminal operation under it found". This text is identical for the TIME check (true T, `V/cfe/modules/time/fsw/src/cfe_time_task.c:311-330` [F]) and for the SCH_LAB lookup (exits on failure, `sch_lab_app.c:176-181`, `:87-91` [F]). RET_CONSTS has `SCH_LAB_AppInit returns 3355443205 [CFE_STATUS_EXTERNAL_RESOURCE_FAIL] ... when=(or (ne local:OsStatus 0) ...)` and also `CFE_TIME_TaskInit returns 3355443205 ... when=(or (and (not (ne local:Status 0)) (ne local:OsStatus 0)) ...)` [M K9]. In TIME that condition belongs to earlier assignments of the same local. A join on the local's name would label TIME R. The classifier needs the definition that reaches the condition; RET_CONSTS conditions name locals without it [I]. The doc's item "conditional return constant is missing" is inexact: the constant is in the facts, the link to the lookup's result is not.
- G3. Path C rows do not carry the caller's handling. The `[calls]` flags do (2.3, C5).
- G4. Path B evaluates constants one call level deep. C3's absence result needs two levels (2.3). 130 of 170, 126 of 155, 47 of 73 path B rows have no callee check at all.
- G5. Actors. c5_641 and c5_642: 9 roots, core tasks missing. r2_core: `script-entry 0`. c1 builds: 5 `table-image-entry` roots with no creation order, so the R2 element shows as B73 `discarded 50`, feasibility "undecidable: no static order derived", in both C1 builds.
- G6. Order. Every path A row is "undecidable"; path B gives "fixed unconfirmed" at best; only path C proves an order, by lock sets. No row states ENFORCED for a process-root publisher (A24, A10: creator in `main (process)`, still "undecidable").
- G7. Initialisation interval. Rows do not say whether a use lies before the actor's run loop. TRANSIENT cannot be separated; the 4 path A rows labelled J with a creator in c1_pre_tx are at `cf_cfdp.c:1600:17` and `:1604:17` in `CF_CFDP_ResetTransaction` [M K5].
- G8. Key. A21's consumer key is "unresolved (needs field points-to)"; the pairing with BP rather than SCH_LAB must come from a run, as the doc says.

## 3. Run-time side on existing records

### 3.1 C1 [M K12]

Natural run of the buggy build, `W/run/c1nat/f0tx/pre_1b0b338/`:
- failing return of the consume: `replay.txt:283` `role_fact cf-dependent-lookup purpose dependent_use actor CF OS_CountSemGetIdByName call #46 return #48 result 0xffffffef site #49`
- key: `replay.txt:263` `narrowing OS_ERR_NAME_NOT_FOUND under api OS_CountSemGetIdByName, ... sem_name text "BP0_tsem", ...`
- exit: `replay.txt:132` `#52 CF CFE_ES_ExitApp call`; `inst/tx/console_output.log:66` `1980-012-14:03:20.55672 CFE_ES_ExitApp: Application CF called CFE_ES_ExitApp`
- event text: `inst/tx/console_output.log:65` `EVS Port1 185/1/CF 30: CF: failed to get sem id for name BP0_tsem, error=-17`
- later publish by another thread: `replay.txt:272` `provider BP #59 OS_CountSemCreate.publish on osal.countsem[0x5be5dc6a107c0903] (after the decision: a late provider)`; `console_output.log:68` `EVS Port1 185/1/BP 15: Created semaphore: BP0_tsem`
- order in the trace: `replay.txt` `#46 CF OS_CountSemGetIdByName call`, `#48 ... ret`, `#51 CF CF_Init ret`, `#52 CF CFE_ES_ExitApp call`, `#58 BP OS_CountSemCreate call`.

Across the four natural tx records: `pre_1b0b338` and `pre_1df262a`: one CF lookup, 0xffffffef, one `CFE_ES_ExitApp` call. `post_1b0b338` and `post_1df262a`: CF lookups `#46 0xffffffef`, `#75 0`, `#79 0`, no `CFE_ES_ExitApp` call (`post_1b0b338/replay.txt:343, 346, 347`; console line 78 `CF 20: CF Initialized`).

Forced orders (hold plan written by a person, `W/run/c1ctl/f0/`):
- buggy build, publish first, `C_pre_1b0b338_tx/replay.txt:314-315`: CF lookups `result 0` twice; console line 73 `CF 20: CF Initialized. Version 3.0.99.0`.
- fixed build, publish held past the bound, `B_post_1b0b338_tx`: 25 CF lookups with 0xffffffef; console lines 87-88 event 30 and the `CFE_ES_ExitApp` line.
- BP's own lookups return 0xffffffef in the natural record (`pre_1b0b338/replay.txt:284-285`) and in the forced one (`C_pre_1b0b338_tx/replay.txt:312-313`): a fail-class operation of another thread present in both orders.

Signals the existing observation did not record:
- the event-send call and its type argument (no site on an event-send function among the 33 sites; the console text has no type field);
- the value of the `CFE_ES_ExitApp` argument (entry site only; no value in the text views) [U whether the container holds it];
- the return values of `CF_Init` and `CF_CFDP_InitEngine` in the text views (sites exist) [U];
- the key string of the create in the text view (the resource appears as a hash).
The 33 sites, the identity rule and the hold plan came from a case profile. Under the input boundary a new observation generated from the rows is needed even where the signal is already known to exist.

`/home/user/work/procrace/c1_runs` holds 4 empty directories and 0 files [M K16].

### 3.2 R2 [M K11]

`python3 -I PR/clauseB.py W/replay/r2/f0/cur.txt` and the same on `gi/cur.txt` (output `PR/clauseB.out`).

| | F0 record | G-I record |
|---|---|---|
| occurrences of `CFE_EVS_SendEventWithAppID` | 295 | 276 |
| publication w (first `sb_publication` point) | trace #111, thread CFE_SB (`cur.txt:214`) | trace #111 (`cur.txt:217`) |
| before w | 24: CFE_ES 20, CFE_EVS 4; AppID argument 0 in all; result 0xc2000003 in all | 24: a2 20, a4 4; same |
| after w | 271; AppID 0x110003 x 243, 0x110004 x 28; result 0 in all | 252; 224 and 28; result 0 in all |
| after w with 0xc2000003 or any fail class | 0 | 0 |

Quote, `W/replay/r2/f0/cur.txt:2694-2698`: `occurrence K-ID.send-with-appid actor CFE_ES ordinal 1 key [0] call #15 return #16` ... `result 0xc2000003 outcome agrees`.
"Stands": in `W/replay/r2/intervals/f0/occ.tsv` CFE_ES has 20 rows and CFE_EVS 4, all of class appid0; neither thread has a later use. Event ids of the 24: 5 x 2, 10 x 4, 14 x 18.
Console (`W/run/r2/f0/w6_r2_f0_perf_operational_esrestart.log`, 233 lines, 113 event lines): 0 lines mention the status; 1 error-like line, `:117` `HS 74: Error in system monitor initialization, RC=0xC8000005`, unrelated [M K15].

So the record holds the failing return status, the thread, the argument value and the position of w. It holds no event, no log line and no exit for the failure, which matches the card (the branch leaves no trace).

Not recorded, or not from the method:
- per-site points exist for 3 call sites (`sb_api_275`, `sb_api_1183`, `sb_priv_786`, `W/replay/r2/f0/sbsites.tsv`); the method wants the 39 sites of row B65. Whether the callee-entry site lets the caller site be recovered was not checked [U];
- clause B item 1 (the static proof) is in no record;
- "every reference boot": two records here (F0 and G-I).

### 3.3 C5 and C3 [M K13]

- `C5R/replay/tables/outcomes.tsv:2`: `641 112 68 44 0 0 18 a9:0x5!=0x6 0 0 20 0 0 0 a9` (lookups 112, `O1_ERR_APPID` 0). Line 3, 6.4.2: 249 lookups, 0.
- `C5R/replay/641/cur.txt`: 78 lines `result 0`, 0 lines `result 0xc4000001`.
- First lookup of the sample application, `cur.txt:988-991`: `occurrence K-SELF.es-task actor a9 ordinal 1 key [7] call #84 return #88` / `status READY ...` / `result 0 outcome narrowed ...`.
- Wrong identity is in the record: `C5R/replay/tables/tasks.tsv:10` `641 a9 25280 SAMPLE_APP 0x8 0x7 0x6 0x6 yes 6 12 0x7 0x5 ...` (own task id 0x8, id read 0x7, application id returned 0x5, own 0x6); console `C5R/641/console.typescript:43` `EVS Port1 66/1/SAMPLE_LIB 1: SAMPLE App Initialized. Version 1.0.0.0`. This is the identity confusion of G5 §15, not an order violation, as the doc says.

Not recorded: any run in which the child reads before the creator's write (0 runs); the returns of `CFE_EVS_Register`, `CFE_EVS_SendEvent` and `CFE_ES_RunLoop` (no such site in the list at `cur.txt:11-43`); for C3, any site on `CFE_EVS_SendEventWithAppID` in a 6.4.x build. The oracle signal for C5 and C3 is in no existing record.

## 4. Census check [M K14]

Command: the pattern of GO §g on the roots below, tests and stubs filtered out; each site opened [F].

| Root | Sites | Site | Failure handling |
|---|---|---|---|
| `c1_build/bundle_1b0b338/apps` (ci_lab, sample_app, sch_lab, to_lab) | 1 | `sch_lab/fsw/src/sch_lab_app.c:176` `OS_TimeBaseGetIdByName(&TimeBaseId, "cFS-Master")` | checked; syslog line, `return CFE_STATUS_EXTERNAL_RESOURCE_FAIL` (:177-181); caller sets `RunStatus = CFE_ES_RunStatus_APP_ERROR` (:87-91). No event |
| `c1_build/bundle_1b0b338/libs` (sample_lib) | 0 | | |
| `expect_src/CF_281a941` | 1 | `fsw/src/cf_cfdp.c:993` | checked; ERROR event, `break` (:995-1000); no retry |
| `expect_src/CF_833fdbb` | 1 | `fsw/src/cf_cfdp.c:1003` | retry while -17, bound `CF_STARTUP_SEM_MAX_RETRIES`, `OS_TaskDelay(CF_STARTUP_SEM_TASK_DELAY)` (:1001-1012); then ERROR event, `break` |
| `expect_src/bp_b051eb0` | 1 | `fsw/src/bp_flow.c:126` | create on miss (`OS_CountSemCreate`, :132); ERROR event only if the create fails (:140) |

Counts by handling over the sites: exit 2 (SCH_LAB, CF before the fix), bounded retry then exit 1 (CF after the fix), create on miss 1 (BP), error event and continue 0, ignored 0. A shipped system has 3 of these sites (one CF version).

What the method makes of them with the present rows [M K5]:
- CF before the fix: row A21, label R, a creator in another actor: candidate. 1 of 3.
- CF after the fix: row A21, label H (bound 25, delay 100): TOLERATED(B). 0 of 3.
- BP: row A20, "not a pair: probe-create": excluded, as predicted.
- SCH_LAB: row A24. The source exits on failure; the row says "no terminal operation under it found", so the label is T?, not R. Its creator is in `main (process)` and the row is "undecidable", flip "static-possible". The doc expects ENFORCED by HB-spawn from the process root; neither that nor the R label is derivable now. If the R label is added before the process-root rule, A24 becomes a second candidate [I].

The census covers 3 sites; path A of the tool covers 113 sites in the same build (2.2), because its definition of a lookup is structural and includes framework-internal searches.

## 5. Summary of what a new observation or a new fact would have to add

1. Facts: a branch condition per call site (G1); the definition that reaches a condition's local (G2); the caller-handling of the reader's status for path C (G3); deeper evaluation of the callee's result (G4); the missing actors (G5); the order closure (G6); the initialisation interval (G7).
2. Observation: sites generated from rows instead of the profile's; the return of the consume and of the consequence calls, the type argument of event sends, the argument of `CFE_ES_ExitApp`; for R2 the 39 call sites; for C5 and C3 a directed run, none exists.
3. Counts to pre-register against: the S3 observation set measured here (285, 284, 255, 99, 97 rows, or 155, 154, 129, 52, 50) and the tight candidate set (12, 11, 5, 22, 18 rows; 9, 9, 5, 8, 7 elements).

## 6. Commands

PR = the probe directory named at the top. All Python is run with `python3 -I`.
- K1 `PR/vocab.py V/cfe/modules/core_api/fsw/inc/cfe_error.h V/osal/src/os/inc/osapi-error.h V/cfe/modules/evs/config/default_cfe_evs_extern_typedefs.h V/cfe/modules/es/config/default_cfe_es_extern_typedefs.h V/psp/fsw/inc/cfe_psp_error.h` -> `PR/vocab.out`
- K2 `grep -c 'CFE_PSP_STATUS_C(-' V/psp/fsw/inc/cfe_psp_error.h`; `grep -o 'CFE_PSP_STATUS_C(-[0-9]*)' ... | grep -o '\-[0-9]*' | sort -n | head -1`; one-line Python check that every value -108..-1 has bits 31-30 = 11
- K3 `PR/apiret.py cFE V/cfe/modules/core_api/fsw/inc/cfe_{es,evs,sb,tbl,time,fs,msg,resourceid,config}.h`; `PR/apiret.py OSAL V/osal/src/os/inc/osapi-*.h` -> `PR/apiret.out`
- K4 `grep -n '#define CFE_EVS_\(DEBUG\|INFORMATION\|ERROR\|CRITICAL\)\b' 641:inc/cfe_evs.h`; `grep -n CFE_SEVERITY 641:inc/cfe_error.h`; inline Python over `641:inc/cfe_error.h` counting literal defines by bits 31-30
- K5 `PR/s1.py B/<build>/NP/relations.txt --show A21,A24,B65,B97,B56,B64,B18,B19,C31,C30,B73,B99` -> `PR/s1_<build>.out`; `PR/s1.py ... --rows | awk` for the decided and undecided totals; `grep -cE '^  [ABC][0-9]+ ' B/*/NP/relations.txt`; `awk` extraction of single rows (A10, A21, A24, A47, A48, A85, B18, B19, B62, B64, B65, C31)
- K6 `PR/evguard.py B/<build>/NP/facts.txt` -> `PR/evguard.out`
- K7 `grep -c -- '<file:line:col>' B/<build>/NP/relations.txt B/<build>/NP/facts.txt` for the sites of 2.3; `grep -n -A2 'site [0-9]* .*cf_cfdp.c:1011:17 ' B/c1_post_tx/NP/facts.txt`; `sed -n 66943p B/c1_pre_tx/NP/facts.txt`
- K8 `awk` between `[lockset]` and `[fstatus]` of facts.txt, `grep -E` on the site positions of row 11
- K9 `awk` between `[retconsts]` and `[loopexits]` (and `[returns]`) of facts.txt, `grep` on `SCH_LAB_AppInit`, `CFE_TIME_TaskInit`, `EVS_NotRegistered`, `CFE_EVS_SendEventWithAppID`, `CFE_ES_GetAppID`
- K10 `awk` between `[calls]` and `[derives]` of `B/c5_641/NP/facts.txt`, lines `^  site `, `grep -- '-> CFE_EVS_Register '` and `'-> CFE_ES_GetAppID '` -> `PR/c5_641_sites.txt`
- K11 `PR/clauseB.py W/replay/r2/f0/cur.txt`; same on `gi/cur.txt` -> `PR/clauseB.out`; `awk -F'\t'` on `W/replay/r2/intervals/f0/occ.tsv`; `head W/replay/r2/f0/sbsites.tsv`
- K12 `grep -n` on `replay.txt`, `run.kzt.record.txt`, `hits.txt` and `inst/tx/console_output.log` under `W/run/c1nat/f0tx/{pre,post}_{1b0b338,1df262a}` and `W/run/c1ctl/f0/{C_pre,B_post}_1b0b338_tx`
- K13 `head C5R/replay/tables/outcomes.tsv`; `grep -o 'result ...' C5R/replay/641/cur.txt | sort | uniq -c`; `grep -n '^site ' C5R/replay/641/cur.txt`; `grep -n` on `tasks.tsv` and `C5R/641/console.typescript`
- K14 `grep -rnE 'OS_[A-Za-z]*GetIdByName *\(|CFE_ES_Get[A-Za-z]*IDByName *\(|CFE_SB_GetPipeIdByName *\(|CFE_TBL_Share *\(' --include=*.c --include=*.h <root> | grep -vE 'unit-test|ut-stubs|/eds/|ut_|/ut-|coveragetest|_tests?/|/test/'`; `sed -n` on the five sites
- K15 `grep -c 'EVS Port1' <console>`; `grep -ci 'error\|fail\|invalid' <console>`
- K16 `find /home/user/work/procrace/c1_runs -type f | wc -l`
- K17 `grep -n -A3 -E '^  site [0-9]+ \S+ \S+ -> (CFE_ES_WaitForSystemState|CFE_ES_WaitForStartupSync|CFE_ES_MainTaskSyncDelay) ' B/{c1_pre_tx,r2_core}/NP/facts.txt`: `cfe_es_start.c:210:9` and `:225:9` `arg 1:const 1000 CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`; r2_core `cfe_es_start.c:835:26` `arg 1:const 30000 CFE_PLATFORM_CORE_MAX_STARTUP_MSEC`; c1_pre_tx `cfe_es_start.c:811:25` `arg 1:const 30000000 CFE_PLATFORM_CORE_MAX_STARTUP_MSEC * 1000`; 0 call sites of `CFE_ES_WaitForStartupSync` from an application in c1_pre_tx

Not done in this probe: no check of locks held by callers at the gate points; no survey of application-defined status values; no re-measurement of the 11-site census of the v7.0.1 applications (GO §g); 7.4 item 5 (deriving GS §B E1-E10 by hand from the four order rules) was not attempted.

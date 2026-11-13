# c_oracle — Phase 3, critic K2: oracle validity, feasibility and cost of D0, D1, D2, D3

Date 2026-11-12. Tags: [M] measured by me in this run (commands in section 9), [F own] read by me in a source or tool file, [F Gx §] fact taken from a dossier at the place named, [I] my inference, [U] unknown.
Dossiers: GS = g_fw_startup.md, GO = g_fw_objects.md, G1 = g_cases_c1c2.md, G3 = g_cases_c3r2.md, G5 = g_cases_c5.md. S4 = scratchpad/wbs/s4.md. OBS = R/kairos/docs/observer.md, EVT = R/kairos/docs/event_semantics.md. V = /home/user/work/procrace/cfs_ref/cFS (v7.0.1). REL(b) = /home/user/work/redesign/step2a/tests_8c_fix/b/NP/relations.txt. W6 = R/plan/results_w6_20261029.md.
"Effect class K0..K4" always means D1's classes; "critic K2" is only my role name.
Nothing was booted or built. Every statement about what a run "would show" is [I] unless it cites a record.

## 0. Verdict in ten lines

1. The four designs share one oracle core: an outcome that the framework vocabulary classes as failure appears when the publication is absent at the use, does not appear when it is present, and is not healed. They differ in where the label is read and in how the order is varied. None of them infers intent; all of them import the developers' own labels.
2. The core has three holes that hit all four designs: (X2) a one-shot probe whose miss is tolerated by design cannot be told from R2 by status class; (X3) R2 is singled out among 25 structurally equal fixed-order rows only because the framework returns an error constant there [M]; (X4) the horizon constant exists with a usable value only in two of the five builds.
3. Strongest oracle where both orders can be run: D3 (first class-divergent operation of the consumer thread, fail in one order only, and it stands). It is under-specified at two verdict-changing places (alignment and insertions; key coarseness) and its discovery schedules collide with the startup barrier.
4. Cheapest and most feasible skeleton: D1 (static candidates, one passive boot, one directed boot per surviving row). Its static verdicts are predictions, not evidence, and its evaluator does not exist.
5. D0 is D3 with a looser, set-based comparison and no cost bound. D2's black box reaches one case of five and its add-on for R2 has concrete false positives in the shipped trees.
6. Ranking as a base for the final method: D1, D3, D0, D2. On oracle validity alone the order is D3, D1, D0, D2.
7. Base: D1's pipeline. The run-time decision rule must be D3's (minimal flip, zero-margin release, "stands"), tightened by D1's reading of the caller's reaction. A static verdict alone is never a detection.
8. No design has a referenced value for the number of repetitions, for the hold ceiling of a script application in 6.4.x, or for the alignment and key rules. They must be pre-registered and must not be tuned on the five cases.
9. Measured in this run: D1's two filter claims hold as stated [M], but they cover effect class K1 only (c1_pre_tx) and a build whose facts hold no script application actor (r2_core).
10. The final method must not claim five of five, must not claim detection of C3's reported harm through the framework vocabulary, and must not present the fixed-order class as interventional evidence.

## 1. Facts I checked myself (rule 2)

| # | Fact | Tag |
|---|---|---|
| K-1 | REL(c1_pre_tx): exactly 6 path A rows carry a line "handling: terminal" (A21, A45, A46, A47, A48, A125); only A21 has "creator(s) of this partition in other actors"; 0 path B and 0 path C rows carry a terminal. D1's M2 is confirmed, and it is a class-K1-only filter on path A | [M] |
| K-2 | REL(r2_core): 55 rows carry a creation-order fact; 25 of them, all path B, name at least one "reader actor created before" the writer (B30, B31, B37-B41, B47, B56, B65, B69, B74, B76, B129, B130, B132, B136, B138-B140, B145-B147, B154, B155). B65 is one of 25. D1's "only B65" needs its second criterion (error-class callee constant) | [M] |
| K-3 | Relation headers: r2_core "script-entry 0"; c5_641 has the core spawn unresolved (cfe_es_start.c:875 in "spawn calls with no resolved entry"); c1_pre_tx "flip static-possible 2, static-undetermined 102, none 327" of 431 rows; "with a creator of the same partition in another actor 36" of 127 path A rows | [M] |
| K-4 | TIME's time-base probe: `OS_TimeBaseGetIdByName(&TimeBaseId, "cFS-Master")` at V/cfe/modules/time/fsw/src/cfe_time_task.c:311; the timer is created only in the success branch (:312-:328); the function returns `CFE_SUCCESS` in every case (:330); the comment (:303-:310) says any error means the other mechanism is used | [F own] |
| K-5 | ES main sync: `CFE_ES_MainTaskSyncDelay` counts every used application record with `AppState < AppStateId` (V/cfe/modules/es/fsw/src/cfe_es_start.c:880-:886) and returns `CFE_ES_OPERATION_TIMED_OUT` when the budget is used up (:869, :900-:917) | [F own] |
| K-6 | v7.0.1 entry poll: first a 50 ms delay, then a check per poll, budget `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC`; on exhaustion `CFE_ES_TaskEntryPoint` does not call the entry function and writes nothing (V/cfe/modules/es/fsw/src/cfe_es_apps.c:569-:595, :613-:641) | [F own] |
| K-7 | Barrier users and their own timeouts: TO_LAB `CFE_ES_WaitForStartupSync(TO_LAB_STARTUP_SYNC_TIMEOUT)` then subscriptions (V/apps/to_lab/fsw/src/to_lab_app.c:64-:68), default 10000 (V/apps/to_lab/fsw/inc/to_lab_internal_cfg.h:80); HS (V/apps/hs/fsw/src/hs_app.c:75), default 65000 (V/apps/hs/fsw/inc/hs_internal_cfg.h:211). In the C1 bundle no application calls a sync API (grep over bundle_1df262a/apps: no hit) | [F own], [M] |
| K-8 | v7.0.1 reference script: 2 libraries, 15 `CFE_APP` entries (lines 3-17), to_lab on line 5, hs on line 12 (V/build-native_std/exe/cpu1/cf/cfe_es_startup.scr) | [F own] |
| K-9 | HS application monitor: lookup by name at V/apps/hs/fsw/src/hs_monitors.c:69; miss gives an ERROR event the first time (:80-:83), the countdown is decremented all the same (:107), and at zero the table action runs, which can be `CFE_ES_ResetCFE` (:121-:138). The shipped default table has `HS_AMTActType_NOACT` in every entry (V/apps/hs/fsw/tables/hs_amt.c:36-:98), so the path is not active with default tables | [F own] |
| K-10 | SB sender-side errors: `CFE_SB_MSGID_LIM_ERR_EID` and `CFE_SB_Q_FULL_ERR_EID` are typed ERROR (V/cfe/modules/sb/fsw/src/cfe_sb_priv.c:678-:680, :690-:692), raised when a destination does not drain (:1057-:1060, :1192); default message limit 4 (V/cfe/modules/sb/fsw/inc/cfe_sb_internal_cfg.h:107) | [F own] |
| K-11 | Routine error-class results in cyclic code: `CFE_SB_ReceiveBuffer(..., CFE_SB_POLL)` at to_lab_app.c:262, hs_app.c:677, :706; timed receive at cf_app.c:300 with the explicit tolerance `status != CFE_SB_TIME_OUT && status != CFE_SB_NO_MESSAGE` (:307) | [F own] |
| K-12 | Start by command: zero stack size is replaced by a default, priority and exception action are validated, then `CFE_ES_AppCreate` (V/cfe/modules/es/fsw/src/cfe_es_task.c:731-:763) | [F own] |
| K-13 | Public API surface: 188 cFE `core_api` prototypes and 174 OSAL prototypes matched by a return-type pattern, about 360 functions; a lower bound, multi-line prototypes are missed | [M] |
| K-14 | Hold mechanism limits: `KZ_RT_MAX_POINTS 32`, `KZ_RT_MAX_ACTORS 64`, `gates_per_actor` 1 (R/kairos/runtime/include/kz_rt.h:61, :73, :280); F0 hold accepts at most 4 distinct control addresses and has no time limit of its own; runtime points have `timeout_ns` (EVT §4, §6) | [F own] |
| K-15 | F0 observer: reads argument registers and the first stack words; a value behind a pointer is read only through a fetch slot at entry; it cannot read at return through a pointer saved at entry; probe cost was never measured (OBS §13) | [F own] |
| K-16 | Standing decisions and state of the unbuilt stages: "a plan in which the parent holds a lock while waiting for the child's progress is not used" is a PI decision (S4 4.5.4, citing PD29:18); the number of boots is a research condition to ask the PI (S4 4.3.2, 4.5.6); instrumented-path holds need hand-written control lines today (S4 4.5.7); the C5 configuration never had a hold run and its runtime adapter never ran on the target (S4 4.3.4); hand manifests had 33-44 points, rows name 629-1,003 locations per build (S4 4.3.2) | [F own] |
| K-17 | Recorded C1 series: natural pre "BP0_tsem: -17x1 ; ends: CF=x"; natural post "-17x1 0x1"; post under hold "-17x25", exit; pre under hold "0x1", `CF_Init=0` (W6:169-:172) | [F own] |

## 2. Holes shared by all four designs

**X1. One oracle, four readings.** D1 reads the label statically on the absent path including the caller's reaction. D3 reads the class of the first divergent API operation of the consumer thread. D0 compares three sets (outcome of u, failure-vocabulary set, steady state). D2 compares a harm set at quiescence. The inference "failure class under one order only, so the other order is required" is the same in all four, and it is exactly as good as the labels the developers wrote.

**X2. Tolerated-by-design one-shot probe.** K-4: TIME's probe returns -17 (error by the OSAL sign rule, GS §C), the present-path effect (the 1 Hz timer) is skipped, the caller returns success, and the code comment documents this as intended. R2 has the same shape: error-class status, skipped effect (the event), caller returns success (G3 R2 §5, §7). In the shipped systems the time base is published before any task exists (G1 §1.11 N2, N3), so no design reports it. The rule itself cannot separate the two:
- D1: by its own definition of K4 ("error-class status produced and an effect of the present-state path skipped") the probe is K4, yet D1's table calls it NO-REQUIREMENT (D1 §6, row "C1 N3"). The definition and the table disagree [I].
- D3: a fail-class operation that stands; V1 as soon as the publisher is unordered. D3 admits it (D3 §7 "designed one-shot fallback").
- D0: P3 as written gives V1; D0 §6 says "V2 or V4" without a rule that yields it.
- D2: nothing visible unless the caller exits.
A mechanical separator exists in the 2a facts: the status is TESTED at K-4 (`if (OsStatus == OS_SUCCESS)`) and DISCARDED at R2 ("handling: discarded 39, used 0, tested 0", REL(r2_core):2198, G3 R2 §5). "Tested, with an empty miss branch" is a decision the code made; "discarded" is not. No design states this rule.

**X3. R2 is one of 25.** K-2. The fixed-order class "reader actor created before the writer" has 25 path B rows in r2_core. Every design reports only B65, because the nested call returns 0xc2000003 there. The other 24 are premature reads with no error constant. Their ground truth is unknown [U]. The 6.4.2 residue of C3 is a known member of that silent type (EVS's `CFE_SB_CreatePipe` reads 0, gets `CFE_SUCCESS`, the event is misattributed; G3 C3 §8). So the fixed-order verdict is "premature read to which the framework answered with an error", not "premature read".

**X4. The horizon constant.** `CFE_PLATFORM_ES_STARTUP_SCRIPT_TIMEOUT_MSEC` is 1000 in v7.0.1 and the C1 bundles, 30000 in 6.4.2 where it equals the core fail-stop bound, and does not exist in 6.4.1 (GS §G). C3 and C5 live on 6.4.1 and 6.4.2. D2's T_F and D3's H therefore have a referenced value for C1 and R2 only. Its documented meaning is ES's own soft limit after the script was parsed (GS §A4, CFG:868-870); ES applies it twice in a row (GS §A1 step 4: cfe_es_start.c:210, :225); no sentence makes it a bound on how late one application may be relative to another. Using it as "legal lateness" is an inference of the designs.

**X5. Barrier interaction of any long hold.** K-5 with GS §A4: an application's record stays below `LATE_INIT` until it calls a wait API or reaches its first `CFE_ES_RunLoop`. A script application held for about the soft timeout keeps both ES waits from completing early, so OPERATIONAL is reached by timeout, and every barrier user (K-7) continues while the held application is still initializing. The tool then manufactures the C2 situation.

**X6. Unknown noise floor.** "Whether a clean boot of these builds shows other error statuses or ERROR events: raw run directories were not opened" (G1 §1.14). The framework documents event floods at initialization (GS §D, cfe_sb.dox:494-495). No design has a measured count of fail-class operations in a clean boot.

**X7. Nothing was executed for the new path.** C5's violating order was never recorded (G5 §14 U1). C3 was never executed and whether the local build faults is unknown (G3 C3 §14). For both, every design's verdict is a prediction from code reading.

## 3. D0 — lead's flip-differential at the framework boundary

(a) Benign order dependence reported.
- The failure-vocabulary set is keyed by (application, call site or event ID) and includes ERROR events of any thread (D0 §4 item 2). A hold of a subscribed receiver makes senders emit `CFE_SB_MSGID_LIM_ERR_EID` or `CFE_SB_Q_FULL_ERR_EID`, both ERROR (K-10). That is a one-sided failure-vocabulary difference produced by the hold, and D0's list of hold outcomes has no entry for it [I].
- A monitor fires by design: with an HS table that names an application and an action, the held or late application gives an ERROR event the first time (K-9). D0 would call the lookup healed but has no rule for an ERROR event that was already sent: P3 says "does not disappear by the system's own later behaviour", and a sent event does not disappear [I]. Not active with the default table (K-9).
- X2: the rule and the walk-through disagree.

(b) Harm missed.
- C3's fatal sub-order needs O1, O2 and O3 unpublished together (G3 C3 §5 yield row 1). D0 §7 puts "more than one flipped pair" out of scope. F2 "child first" at the 6.4.1 core spawns is absorbed by the ES lock held from :838 to :933 (G3 C3 §6 (a)), so it reorders nothing. D0 reaches only the sub-order "SB between its two publications" (0xc2000002, event 41), not the reported crash. D0 says so; the consequence is that D0's C3 verdict is for a different pair than the issue's [I].
- Silent differences end as "unoriented"; their number per system is unknown (D0 §7).

(c) Artifacts of the intervention.
- No hold cap is stated; "the soft 1000 ms startup timeouts and the 30000 ms core wait are the known ones" is a hand list. Application timeouts are in the code under test (K-7: 10000, 65000; C1 fix: 25 x 100, G1 §1.8) and must come from the loop facts, not from a list.
- F3 by the ES start command changes barrier semantics (GS §A5 iii) and the creator thread (G5 §6: command path = the ES task).
- "Consumer blocked on a lock owned by the held task: enforced" is observed by holding a lock owner. That conflicts with the standing PI decision (K-16).

(d) Noise. "Components that differ between repeated control runs are excluded"; the number of repetitions is open. The recorded count of event 14 differs 12 against 17 between two recordings of one build (G3 R2 §11 L6).

(e) Values. Repetitions, observation horizon and the definition of quiescence are all open (D0 §7, last line). Nothing is invented, but nothing is fixed either.

(f) Feasibility and cost. Recording every public API entry and exit with out values needs about 360 functions (K-13) against 33-44 hand points (K-16), and out values cannot be read on the F0 path (K-15), so a derived build with generated manifests is required. Cost: controls, plus one run per flip: F3 n runs, F2 two per spawn, F1 one per unordered pair, F4 one per field. No bound is given; with 36 path A rows that have a foreign creator in c1_pre_tx (K-3) and 25 fixed-order rows in r2_core (K-2) the count can scale with the rows the PI wants to escape [I].

(g) Legitimacy. Sound where a flip is realized. Two quiet assumptions: "late start goes through the same creation function" is used to justify holds between two API calls in the middle of an init, which is a stronger schedule assumption than "X starts last"; and the disjunct "or its framework-observable steady state differs" is a second oracle with no direction.

## 4. D1 — static requirement derivation (KZ-S) with a minimal run-time tier

(a) Benign order dependence reported.
- UNENFORCED is "no edge under four rules". D1 names the unmodelled mechanisms itself (the 6.4.1 startup semaphore, message-triggered code). Each of them turns a correct system into a finding.
- X2: effect class K4 as defined swallows every tested-and-tolerated probe.
- Intended exits: CF's documentation states the exit as intended behaviour (G1 §4 item 10). D1 reports it as a violation. This is the PI's ground truth for C1, so it is acceptable, but the label must be "failure-labelled and unenforced", not "defect".
- sigma_bot may combine absences that never occur together; with k unordered elements the 2^k presence states include unreachable ones (D1 §7 admits).

(b) Harm missed.
- X3: the 24 other fixed-order rows end as NO-REQUIREMENT, a decided negative. For the 6.4.2 residue D1 prints NO-REQUIREMENT for a known leftover of the bug (D1 §6 "C3 fixed"). The word asserts more than the analysis shows.
- Publications inside calls the facts cannot follow, keys from outside the shipped data, the third object state (-35, G1 §1.5), message loss on the software bus, C2.
- "ENFORCED for TBL, TIME and every script application" in R2 cannot be derived from facts with "script-entry 0" (K-3; G3 R2 §11 L2 caveat). The C3 verdict needs the core tasks as actors, which the c5_641 facts do not have (K-3).

(c) Artifacts of the intervention. The best guard of the four: a run counts only if its trace equals the statically predicted signature, and plans are refused statically when the lock set at the hold point is not empty (D1 §5). Two gaps: the equality relation is not defined, although recorded traces of one site differ in length ("-17x1 0x1" against "-17x25", K-17); and the hold ceiling "smallest terminal wait bound that depends on the held actor" does not exist for a script application in v7.0.1 (only soft waits) or for anything in 6.4.1 (GS §G), so the ceiling is pre-registered in most real cases.

(d) Noise. One passive boot is justified only for the deterministic fixed-order class (24 calls identical in two recordings, G3 R2 §9). REALIZED for an unenforced row depends on the natural order; that is handled by the directed boot, so it is not an error source, only a cost.

(e) Values. From the code: B (25, 100), soft and hard wait bounds. Pre-registered: evaluator depth and fork limits, which classes count, counting unit, survivor limit, passive ceiling, hold ceiling where no bound exists. No invented value found.

(f) Feasibility and cost.
- The deciding component does not exist. Today's flip evaluation decides 2 of 431 rows in c1_pre_tx and leaves 102 "static-undetermined" (K-3), including A21 and B65 (G1 §1.9e, G3 R2 §9). If the new evaluator returns UNDECIDED-EVAL for most sites, the result is step 2a's ending under a new name. D1 gives no measurement that it will not.
- The measured filters are narrower than the method: K-1 is K1 only, on path A only; K-2 shows that "only B65" comes from the error-constant criterion in a build without script actors. The survivor count with K3 and K4 and with all actors present is unmeasured [U].
- T-BIND is a rule a person writes once for the table service. It is framework-level, so inside the boundary, but not automatic. It is not needed: the key is a plain string at the OSAL boundary (G1 §4 item 4) and one passive boot reads it [I].
- Run cost is the lowest: 1 passive boot per build plus 1 directed boot per surviving unrealized row. A directed plan is one gate and one release, which fits both hold mechanisms (K-14) [I]. Confirmation of an effect of class K2 needs the value of a subscript at a statement; whether the present anchors can read it is [U] (OBS §5: a line-start anchor does not read the stored value).

(g) Legitimacy. "The consumer's error handling is the specification" holds for C1 (the caller reacts with an ERROR event and an exit). For R2 the caller's handling is to discard the status, so by the same principle the code tolerates the outcome. D1 nevertheless reports K4. The label there comes from the CALLEE: a framework API rejected the call. The sound statement for that class is: "a framework API refused the call by its own precondition check, the refusal was caused by a value another actor had not yet published, and no code examined the refusal". The framework documents the precondition (cfe_evs.h:169-172, G3 R2 §5). D1 does not make this distinction and so uses two label sources under one name. Second quiet assumption: UNENFORCED is treated as feasible.

## 5. D2 — determinism as specification, black-box late start (Layer A) plus Layer B

(a) Benign order dependence reported.
- The V1/V2 line compares an application constant with a framework constant that the framework never related to it (X4). A fixed application with a retry shorter than T_F is V1; fixed CF is V2 only because 2400 > 1000, and V1 under 30000 (D2 admits).
- The prefix record is "the world without X for at least W = 6000 ms". A consumer that correctly waits for X shows structural loss (H3) there. D2 has no label that separates "waits correctly" from "depends": both are an edge until the lag probe is run.
- Add-on E2 ("error-class returns of a site all precede its successes") matches, in the shipped trees: the fixed CF loop (-17 then 0 at one site, K-17); SBN's loop on `CFE_ES_OPERATION_TIMED_OUT` (G1 §2.11 L3); every timed receive that times out until the scheduler's first message arrives (K-11: cf_app.c:300, with `CFE_SB_TIME_OUT` in the error class, GS §C). E2 has no static link to a publisher, so nothing removes these [I].
- Monitors, admitted by D2 (K-9 gives the exact lines and the default-table caveat).

(b) Harm missed. Layer A reaches one case of five (D2 §10). A variant of C1 in which the consumer continues without its semaphore passes. Libraries cannot be moved. Interleavings inside two inits are out of reach.

(c) Artifacts of the intervention.
- "Script minus X" with X = the scheduler application removes every periodic wake-up for at least W. Applications that run only on scheduled messages stop cycling; a monitor with an action counts down and can reset the processor (K-9: :107-:138; not with default tables). The harm then comes from removing a scheduler for six seconds, not from an init order [I].
- The driver application is a new script entry. Its record is counted by the ES sync (K-5), so its own state moves the time at which OPERATIONAL is reached in every run. D2 declares the effect and does not bound it.
- Events sent before the driver has subscribed are not delivered to it: the software bus retains nothing for a later subscriber (GO §d row 2). The core startup events are always before it. D2 falls back to the local log (first 20 events, D2 §8) and the console text, which has no type field (GS §C). The H4 class is therefore incomplete exactly at startup [I].
- Start by command: other creator thread (G5 §6), default stack on zero (K-12), barrier calls return at once (GS §A5 iii). D2 flags the last one; a flag does not remove it.

(d) Noise. The event multiset is keyed by `AppName`, which was wrong for one application in both 6.4.x boots (G5 §15). The volatile mask is learned from k runs, k unreferenced.

(e) Values. By reference: poll period only. From the build, with a rule that is the designer's inference: W, T_F, D_boot. Pre-registered: k, r, the postulate itself. 6.4.1: marker line unknown, D_boot and T_F absent. The decisive values are almost all pre-registered.

(f) Feasibility and cost. No probe in the system under test, which is its one real advantage. New: a driver application for each framework generation, dump collectors per generation, canonicalizer, differ, lag bisection. Boots: k + r x [n + v(n-1) + e(1 + 7)]; C1 build k + 17r; the v7.0.1 reference script has n = 15 (K-8), so at least k + 15r before any edge. Layer B is the same unbuilt machinery as D0 and D3.

(g) Legitimacy. The argument for H2 is the cleanest of the four (the script is the shipped statement of what must run), but cES1508.1 says "create and start", not "keep running" (GS §D), and CF documents its exit as intended. The headline premise is refuted by D2's own check and replaced by a postulate "to pre-register": the oracle then rests on a PI decision, not on the system. For R2 the design states that it is "not a violation under the definition": the method's definition excludes a development case.

## 6. D3 — stale-decision differential

(a) Benign order dependence reported. X2, admitted. The verdict consults only the first fail-class operation; the caller's reaction is never read, so every one-shot probe with a designed fallback and an unordered publisher is V1.

(b) Harm missed.
- "Stands" is judged per (thread, site, key) and R-KEY keeps only string and enum arguments. For `CFE_EVS_SendEventWithAppID` the key is then (type, format string). A later pipe created by the same thread at the same site "heals" the lost event of an earlier pipe. D3 notes the R2 instance (17 of 20 early ES failures at one site). The proxy confuses instances in general [I].
- An event send whose type argument is the constant ERROR is fail-class in both runs whenever it executes. It can differ only by being present or absent, which D3's text files under ORDER-DEPENDENT-UNDIRECTED ("executed sites"). Whether an inserted fail-class operation counts is left to the alignment rule "to pre-register". That rule decides verdicts [I].
- The first divergence can be a timing-dependent routine result (K-11) and hide the real one. Inside once-only init code this is rare; in cyclic code it is the normal case. D3 does not restrict the comparison to init.
- Uses with no API call and no 2a row (D3's own example: no row has `EVS_AppData_t.RegisterFlag`).

(c) Artifacts of the intervention.
- T-last and T-first hold up to H = soft timeout minus one poll. By X5 the held application must then reach its first `CFE_ES_RunLoop` within about one poll period plus the rest of the script, or ES's first wait times out. D3's own rule (iii) rejects a run in which a bounded wait exits by its bound. The main discovery schedule therefore sits on the edge of its own rejection rule in every build that has the constant [I from K-5].
- "Until every other thread has reached init-end": a barrier user reaches init-end only after OPERATIONAL, which waits for the gated thread. In the v7.0.1 reference build (to_lab, hs, cs, sbn; GS §A5) the release condition is never met and every T-last gate runs to H. In the C1 bundle no application uses the barrier (K-7), so C1 itself is not affected [I].
- S1 "one boot covers every spawn" fails in v7.0.1: while the creator is gated at the return of `OS_TaskCreate` the child cannot leave its entry poll (K-6), so every gate runs to H. Twelve spawns follow to_lab in the reference script (K-8); 12 x 950 ms exceeds TO_LAB's own 10000 ms barrier timeout (K-7), which then fires because of the tool. S1 needs one boot per spawn, or per few spawns [I].
- ENFORCED (lock) is observed by gating a lock owner and watching the child block. That is the plan shape the PI excluded (K-16). D1's static refusal does the same job without the run.

(d) Noise. R-MASK is computed from N_ref runs, N_ref unreferenced and "at least 2". An entry that varies in a minority of boots passes a two-run mask and later looks like a divergence. The repetition count of a directed run is also unreferenced.

(e) Values. From the build: R-SNAP (the rule is an inference), H where the constant exists. Pre-registered: H for 6.4.x, N_ref, repetitions, R-KEY, alignment, run budget. Two of these (R-KEY, alignment) are not numbers but rules that change verdicts.

(f) Feasibility and cost. Entry and exit probes on about 360 functions (K-13); generated manifests and plans; gating of many threads at thread start against "one gate per actor, one held thread per gate, 32 points" (K-14); a creation token passed through the spawn; lock wait-for detection; comparator; search driver. Boots by D3's formula: 27 for 12 tasks; for the v7.0.1 reference system (15 applications, 5 core tasks, their children) more than 60 before localization, plus the S1 split above [I]. Wall time per boot is small; the engineering and the unmeasured probe effect (K-15) are the cost.

(g) Legitimacy. The best argument of the four: attribution by intervention, direction from framework classes, "stands" for healing. Quiet assumptions: any thread may be slower by up to H (X4); fail-class means undesired (X2); and the property "either order must settle in the same place" is the designer's postulate, the same one D2 had to pre-register. For the fixed-order class there is no intervention at all; D3 says so.

## 7. Provenance of every value and rule (question e)

| Knob | D0 | D1 | D2 | D3 |
|---|---|---|---|---|
| Failure vocabulary | framework headers | framework headers, four rules transcribed once | framework headers | framework headers |
| Repetitions and reference runs | open | 1 passive (assumes determinism) | k, r pre-register | N_ref, repetitions pre-register |
| Lateness horizon | none stated | none needed; B from code | T_F: build constant, rule inferred | H: build constant, rule inferred |
| Same for 6.4.1 / 6.4.2 | n/a | n/a | absent / equals fail-stop bound | absent / pre-register |
| Hold length | event-based, no cap | event-based; cap from hard bound, else pre-register | cap = T_F minus poll | cap = H |
| End of observation | open | event-based, ceiling pre-register | W: build constants, rule inferred | R-SNAP: build constants plus loop facts |
| Key and alignment rules | "renamed identities", unspecified | the code's own comparison | framework record types plus learned mask | R-KEY, alignment pre-register |
| Volume or budget limit | none | survivor limit, evaluator limits pre-register | none | run budget pre-register |
| Invented values found | none | none | none | none |

No design invents a number. The weak point is elsewhere: rules whose only source is the designer (T_F, H, W, alignment, R-KEY, the survivor limit) are verdict-changing, and all four designs were written with the five cases in view.

## 8. Ranking, base, grafts

| Criterion | D0 | D1 | D2 | D3 |
|---|---|---|---|---|
| Oracle decidable as written | partly (three sets, "unoriented") | yes, statically; run rule vague | yes for Layer A | yes, except alignment |
| Evidence grade when it fires | two runs | prediction; one or two runs for confirmed rows | two runs, system level | two runs, operation level |
| Development cases reached (C2 has no code) | C1, R2 with records; C5 predicted; C3 only for another pair than the issue's | C1, R2 with records; C3, C5 predicted | C1 only in Layer A | C1, R2 with records; C3, C5 predicted |
| Fixed-order class | single run | proof plus one boot | outside definition | single run plus row |
| Guard against its own artifacts | hold-outcome list | prediction match, static lock refusal | flags | acceptance rules (i)-(iii) |
| Fits existing mechanisms | no | mostly (few sites, one gate) | Layer A yes (no probe) | no |
| Boots for one system | unbounded | 1 + survivors | k + r(n + ...) | N_ref + threads x 2 + localization |

Ranking as a base: **D1, D3, D0, D2.**

Base: **D1**, for four reasons. It answers the PI's question in the PI's terms (which row). It gives every run a prediction to match, which is the only defence against artifacts and noise that does not depend on an unreferenced number of reference runs. Its run-time needs are small enough for the mechanisms that exist. It handles the fixed-order class by proof instead of by correlation alone. It is not accepted as it stands: see section 10.

Grafts onto D1.
- From D3: the run-time decision rule (minimal single-gate flip, release as soon as u has returned, first class-divergent operation of the consumer thread, "stands"); the acceptance rules for a produced order; the in-run split for the fixed-order class; the output SILENT-PREMATURE; thread identity by a creation token; a per-spawn children-first run as a row-free sweep, reported at a lower grade when no row explains the witness.
- From D0: recording at nested framework-to-framework calls (R2's only signal is there, G3 R2 §9); the single-run trigger "a failure that stands, followed in the same run by another actor's publication of the same key" (G1 §4 item 1), which finds C1 in the passive boot and reads the key at run time; the taxonomy of hold outcomes.
- From D2: late start by the ES start command as a whole-application flip that needs no probe, for systems where no gate can be placed; the measured tolerance d* next to the bound B read from code; the harm class "script-requested application absent or exited with an error run status" as supporting evidence; the flag "barrier semantics changed".

## 9. What the final method must NOT claim

1. That it detects order violations in general, or the required order in the sense of intent. It detects an unenforced or fixed order whose violation produces an outcome labelled as failure in the framework vocabulary and not healed. Violations answered with a success status are not detected: C2, a message sent before the subscription, C3's sub-order with misattribution, the 6.4.2 residue.
2. Five of five. C2 has no code. C3 and C5 are predictions until a directed run exists; C5's violating order was never recorded.
3. Detection of C3's reported harm through the framework vocabulary. The crash path has no status, event or syslog line of its own (G3 observation 5). A report on C3 is either a generic memory-safety pattern (D1's class K2) or a different, non-fatal signal (0xc2000002); the paper must say which.
4. That the fixed-order verdict (R2) is interventional. It is one run plus static data flow, and R2 is 1 of 25 fixed-order rows in its build [M]; the other 24 are unlabelled, not benign.
5. That thresholds come from the framework where they do not: the lateness horizon (absent in 6.4.1, equal to the fail-stop bound in 6.4.2, repurposed in v7.0.1), the observation window rule, repetition counts, alignment and key rules, evaluator and survivor limits.
6. That a reported row is a defect. UNENFORCED is the absence of a proof of order; CF documents its exit as intended behaviour.
7. That a produced order occurs in the shipped configuration. A gate shows that no synchronization excludes the order; the priorities written in the startup script may still make it rare or absent (G1 §1.6, G3 C3 §6). No frequency claim.
8. A volume reduction from 1,553 rows to a handful, until it is measured with effect classes K3 and K4 and with all actors present. Today's measurements are K1 only on path A (K-1) and a build without script actors (K-2, K-3).
9. "No human input" without listing the framework-level rules a person writes once: the four vocabulary rules, the phase-boundary calls, the table-binding rule if kept, the primitive table.
10. That ENFORCED or NO-REQUIREMENT are proofs of safety. ENFORCED rests on writer-set completeness and on the primitive table; NO-REQUIREMENT means only "no labelled failure on the absent path".
11. That C3, C5 and R2 are violations "between applications" in the plain sense. Their actors are core services, or the framework creator and the application's own task; only C1 is application against application.
12. Generalization. All five cases are the development set; no held-out system was examined.

## 10. Must be fixed in the synthesis

1. Verdict words: a static verdict is PREDICTED. Only a run-matched row is CONFIRMED (two runs) or FIXED-ORDER (one run plus proof). C3 and C5 stay PREDICTED until a directed run exists.
2. Effect class K4 must be split by the caller's handling: status DISCARDED on the path (candidate) against status TESTED with an empty or fallback miss branch (TOLERATED-BY-DESIGN, listed, never counted). Check it on K-4 (must be tolerated) and on B65 (must be a candidate).
3. State the label source per class: caller's reaction (C1 type) or framework API refusal with nobody reading it (R2 type). Do not sell both as "the consumer's error handling".
4. Rename NO-REQUIREMENT. Fixed-order rows without an error constant are SILENT-PREMATURE and are listed with their count (24 in r2_core today).
5. Define "the trace equals the predicted signature" as a pattern (site, status class, key, order; repetition counts free), and define whether an inserted fail-class operation counts. Fix both before any run.
6. Restrict the run-time comparison to once-only (init) code up to the first run-loop entry, exit or unbounded wait; cyclic code goes to a separate transient list.
7. No hold of a script application for a time near the soft startup timeout. Holds are released by the event "u has returned". A run in which any bounded wait exits by its bound is discarded, with the bounded waits taken from the loop facts of the build, including application timeouts.
8. Lock-held holds are refused from the static lock set. Enforcement by a lock is a static verdict, not a run.
9. No multi-spawn hold in one boot; one spawn per boot for the row-free sweep.
10. The table-binding rule is optional. Keys are read at run time in the passive boot.
11. The concrete evaluator is not a precondition. Where the natural order already takes the absent path (C1, R2), the passive boot supplies the outcome. The fraction of rows left UNDECIDED-EVAL is a primary reported number.
12. Before any claim on volume: re-measure survivors with K3 and K4, with script actors in r2_core and core task actors in c5_641.
13. Pre-register with the PI, as rules and not as tuned values: number of passive and directed boots, hold ceiling where no hard bound exists, counting unit (one finding per publisher and element), key rule, which effect classes count.
14. Measure the clean-boot noise floor (fail-class operations and ERROR events in a boot with no gate) before the first directed run.
15. State the actor pair of every finding (application, core service, framework creator) and do not count framework-internal findings as application pairs.

## 11. Commands behind [M]

- K-1: awk over REL(c1_pre_tx): current row = line matching `^  [ABC][0-9]+ `; rows with a line `^ +handling: terminal`; their `dependency:` line.
- K-2: awk over REL(r2_core): rows with the phrase "reader actors created before it:", split by "none" or not, counted per path letter.
- K-3: `sed -n 17,20p` of the three relation files (actors, spawn, summary lines).
- K-7: `grep -rn "CFE_ES_WaitForStartupSync\|CFE_ES_WaitForSystemState"` over bundle_1df262a/apps (no hit); `grep -rln WaitForStartupSync` over the bundle (ES source and its coverage test only).
- K-13: `grep -rhoE` of prototypes with a framework return type in V/cfe/modules/core_api/fsw/inc/*.h and V/osal/src/os/inc/*.h, unique names, counted by prefix.

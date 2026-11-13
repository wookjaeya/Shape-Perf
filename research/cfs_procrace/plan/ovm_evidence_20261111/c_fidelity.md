# K1 critique (phase 3): case fidelity and hidden human input

Critic K1, 2026-11-12. Designs: D0 = d0_lead.md (lead), D1 = d1.md (static first), D2 = d2.md (black box first), D3 = d3.md (first principles).
Tags: [M] measured by me in this run (commands in section 7), [F] read in a file, [I] my inference, [U] unknown.
Cards: G1 = g_cases_c1c2.md, G3 = g_cases_c3r2.md, G5 = g_cases_c5.md, GS = g_fw_startup.md, GO = g_fw_objects.md. "641:" = /home/user/work/c5/src/641/cFE-6.4.1-OSS-release/cfe/fsw/cfe-core/src/. REL(b) = /home/user/work/redesign/step2a/tests_8c_fix/b/NP/relations.txt.
Nothing was booted by any designer or by me. Every "found" below means "the walk-through survives a replay against the cards and the source", not "observed".

## 0. Result in twelve lines

1. Ranking by fidelity: D3 > D1 > D2 > D0. No design detects C2; all four say so, correctly (G1 2.13, 2.14).
2. D3 is the only design whose replay reaches C1, C3 (the reported fatal order), C5 and R2 with generated schedules and two oracle clauses, and whose C3 localization agrees with the card's yield table line by line.
3. D1 reaches the same four on paper and is the only design that names C3's reported harm (the unchecked index), but each case needs a different unbuilt static capability, and its "minimal required set" rule is contradicted by the C3 yield table.
4. D2's black box reaches C1 only (its own statement). Its C3 localization claim is wrong against the card. Its R2 add-on E2, as defined, also flags the FIXED C1.
5. D0 as written misses C5: the creator is released after the child's first framework call, and that call is `CFE_ES_PerfLogEntry`, two calls before the consuming one [M1]. D0 also excludes by its own scope the two-publisher order that C3's crash needs.
6. Hidden input, worst item per design. D0: the object class and role table for exactly the classes where roles are not derivable (EVS registration, ES task record), which is the old kinds file under a new name; and the core serialization taken as a given fact. D1: none per case, but four effect classes that map one to one onto the four cases, plus two inferred pruning rules. D2: the horizon rule T_F and the window rule W, on which the verdict for the fixed C1 depends. D3: the gate length H (same dependence), the key rule R-KEY (C3 aligns only because the AppID argument is dropped), two hand lists of framework function names.
7. The verdict "fixed C1 is not a violation" is robust in D0 and D1 (bound read from the code) and depends on an invented horizon rule in D2 and D3.
8. Every runtime design masks whatever varies between repeated reference runs. A naturally flaky race is exactly such a variation; the mask turns the strongest evidence into "undecided".
9. The failure vocabulary is not "automatic" for the 6.4.x generation: event types and run statuses are `#define` macros and the type argument is `uint16` [M5]. D2 and D3 claim an enum parse; D1 states the transcription.
10. Event 41 ("not registered", ERROR) is emitted by the EVS-internal `EVS_SendEvent`, which is in no public header [M2]. A vocabulary bound to the three public send prototypes (D1 V0c, D3 I6) does not see it.
11. Silent fixed-order early reads are common: 25 of the 155 path B rows of REL(r2_core) have a reader actor created before a writer actor, and only one (B65 = R2) has an error-class effect [M4]. Any "unoriented / silent premature" list starts at about two dozen entries per build.
12. The noise floor of a clean boot (how many error-class returns and ERROR events occur with no defect) is unmeasured for every build (G1 1.14 [U]; W6 holds only the case sites [M6]). All mask and volume claims of D0, D2, D3 rest on it.

## 1. D0 (lead): flip-differential at the framework boundary

### 1a. Replay

| Case | Result | Replay against the cards |
|---|---|---|
| C1 buggy | found | Natural run holds lookup "BP0_tsem" -17, ERROR event 30, exit status 3, then BP's create of the same string (G1 1.9a). Roles of the two OSAL calls are derivable (GO c, example 1). F1 control holds CF at the lookup entry until the create returns: 0, CF runs (G1 1.10 row 2, recorded only under a researcher hold). One-sided, not healed: V1. BP's own probe cancels (N1). |
| C1 fixed | no report (correct) | -17 then 0 at one site: V2, bound from the loop facts (REL(c1_post_tx), G1 4 item 2). Does not depend on a horizon because F1 releases as soon as u returned. |
| C2 | not found | Stated by D0. |
| C3 buggy | not found for the reported order; a neighbouring sub-order found only with an addition | The crash needs the SB ID, EVS's own ID and EVS's registration all unpublished (G3 C3 field 5, row 1), i.e. two publishers late. D0 section 7 puts "more than one flipped pair" out of scope. What D0 claims as V1 (SB between its two publications: 0xc2000002, ERROR event 41) is row 4 of the yield table and is reachable by F1 only if the tool knows that `CFE_EVS_Register` publishes a record keyed by the caller's identity. That role is not derivable: "the key is the caller's identity, obtained inside the API; no key argument exists" (GO c, hard example). Addition: a hand-written EVS registration role, or D3's thread-level family. Also the core tasks are not actors in the c5_641 facts (G3 C3 field 9), so F4's static field set is incomplete there. |
| C3 fixed | no violation; one "unoriented" entry | Barrier makes the flips unrealizable (G3 C3 field 8). EVS's fixed early read is listed as V3s. |
| C5 buggy | not found as written; found with an addition | F2 "child first" releases the creator when "the child has completed its first framework call or blocks". First framework call of the shipped applications is `CFE_ES_PerfLogEntry` (sample_app.c:45, ci_lab_app.c:96), then `CFE_ES_RegisterApp` (:82, :146), and only then the consumer `CFE_EVS_Register` (:87, :148) [M1]. `CFE_ES_PerfLogAdd` touches neither the task table nor the ES lock [M1]. The creator is therefore released two calls before the consume, and the recorded gap between a child's registration and its first lookup is 0.024 to 0.152 ms (G5 field 9): the flip is not realized. Addition: release when the child reaches the end of init or blocks on a lock the creator owns (D3 S1). |
| C5 fixed | no report (correct, for the wrong reason) | With the same release rule the creator is released before the child reaches the ES mutex; the outcome "consumer blocked on the held task's lock" is never observed. No difference, no report. |
| R2 | found only with additions | The nested call is observable (D0 section 2). But the key 0 does not name SB (G3 R2 field 9); the link needs the static field. D0's field class is "a placeholder and ONE later non-constant store"; row B65 says "writers 3" with writer actors {CFE_SB_TaskMain, main} [M4]. And "the order is forced by construction" is taken from the list of enforced orders in D0 section 2, not derived. Additions: field class defined by writer values (D1's wording), and a derived proof of the fixed order (D1 HB-await or D3 gate (d)). |
| Look-alikes | mostly correct | N1 V4, N2 ordered by creation, N4 V2, N5/N8 not exercised: consistent with G1 1.11. N7 and R2 L6 (send without subscriber) satisfy P1 and P2 and have no status: each such pair is an "unoriented" entry (D0 section 2 admits the oracle has only the run difference there). C3 L7 and the R2 "nearby" window are not analysed. |

### 1b. Hidden inputs and parameters

1. Object class list L1 and the role table (publish, consume, key argument, "the status that means absent"). GO c says roles are mechanical for OSAL and for ES/TBL by-name APIs only; for EVS, TIME and SB there is no key argument or no absent status. The classes D0 lists beyond that ("ES application/library/task records", "EVS registrations") are exactly the kinds a person wrote in the old path: `es.task` with states FREE < LIVE and a pseudo-parameter (G5 field 12), `evs.reg` "written with cases inspected: R2" (G3 R2 field 12). Framework-level, so inside the boundary, but hand-written and chosen with three of the five cases in view. D0's sentence "derivable mechanically" covers C1 only.
2. "The status that means absent" per API is an expected-result table. It is also unnecessary: D0's own P1 uses the status class differentially.
3. Enforced-order list: "the framework's startup serialization as it is in the tested version (core tasks one at a time with a wait in v7.0.1 and 6.4.2, none in 6.4.1 ...)". Read from the dossier per version. The old path had this as the hand-written rule CORE-SERIAL (G3 R2 field 12, `30_hb.kzp:51`). It must be derived or tested, not listed.
4. F2 release condition "first framework call": a count of 1 with no reference, and wrong for C5 [M1].
5. "The soft 1000 ms startup timeouts and the 30000 ms core wait are the known ones": a list of known timeouts. Applications bring their own (TO_LAB 10000, SBN 1000, G1 2.11). Artifact detection must be generic (D3 rule (iii), D1 wait-loop list).
6. Steady-state comparison (oracle item 3) names record kinds but no way to obtain them. D2 specifies this; D0 does not.
7. Repetitions, observation horizon, quiescence: declared open, no value proposed. Not invented, but nothing can be pre-registered from D0 as written.
8. P3 is internally inconsistent: its second disjunct ("or its framework-observable steady state differs") would make a pure state difference a violation, while the next paragraph calls the same situation "unoriented".

### 1c. On an unseen system

Boots: controls + one per script application (F3) + two per spawn (F2) + one per pair (F1) + one per static field (F4). Reports [I]: V1 a handful at most; V3 at least one framework-internal finding that repeats on every system of that framework version (R2); V3s about two dozen per build from fixed-order early reads without an error class [M4]; "unoriented" entries for every send-before-subscribe pair that F3 moves (43 transmit and 49 subscribe sites in the 17 applications of the v7.0.1 tree, GO g). The unoriented and V3s lists have no ranking and need a reader who knows the framework internals and the applications. That reader is the human oracle again, moved from the input side to the output side.

## 2. D1: static requirement derivation (KZ-S) with a small run-time tier

### 2a. Replay

| Case | Result | Replay against the cards |
|---|---|---|
| C1 buggy | found (static verdict; the bad order is the natural one in all recorded boots) | Row A21 exists with the chain to `CFE_ES_ExitApp` (G1 1.9e). Key by T-BIND: the compile database has exactly two units named cf_def_config.c, tx at :1391 and rx at :1397, and none for CF's own default table [M3], so the target must be selected as D1 says. -17 negative = error; event type constant ERROR (:997-999); exit (G1 1.7): K3 and K1. No healing, no order (G1 1.6, 1.7). UNENFORCED. A passive boot gives the predicted signature (G1 1.10 row 1, 8 of 8). T-BIND is not required for the verdict: D1's passive boot also resolves the key. |
| C1 fixed | no report (correct) | TOLERATED(B) from loop bound and delay constant (G1 1.8). Independent of any horizon. |
| C2 | not found | Lists the publications after a ready declaration (G1 2.13 T1). Honest. |
| C3 buggy | found only with additions | Additions: core tasks as actors (spawn 641:es/cfe_es_start.c:875 unresolved, G3 C3 field 9); a concrete evaluator four calls deep; the K2 rule. With them the replay matches the card step by step: 0 < 32 (:285), RegisterFlag FALSE (:289), `EVS_NotRegistered(0)`, EventCount 0 (:199), `EVS_SendEvent` (:211), `EVS_IsFiltered(0xFFFFFFFF)` (:616), unchecked `AppData[AppID]` (:244): G3 C3 field 5. D1 is the only design that reports the reported harm itself. Static only; whether the local build faults is [U]. |
| C3 fixed | no report (correct) | Barrier (642:es/cfe_es_start.c:936) and guard (642:evs/cfe_evs_utils.c:620). The misattribution residue is missed, as D1 says. |
| C5 buggy | found (static verdict, never observed) | Row C31 exists: spawn with no lock, flip static-possible with 0xc4000001 (G5 field 9). K4 holds on severity bits. The rise to K3 does not follow from V0 as cited: event 41 is sent by `EVS_SendEvent`, declared only in the internal header (641:evs/cfe_evs_utils.h:94; v7.0.1 cfe_evs_utils.h:263) [M2], while V0(c) is bound to the public prototypes. The verdict stands on K4 and K1 alone. No run has ever shown the order (G5 U1). |
| C5 fixed | no report (correct) | Existing "proven-impossible" row C30 (G5 field 9). |
| R2 | found only with an addition | Addition: evaluation through `EVS_GetAppDataByID`; today the row says "flip: static-undetermined ... the result of EVS_GetAppDataByID" and "fixed unconfirmed (creation order + await)" [M4]. With it: FIXED-PREMATURE, K4, 39 sites discarded, matching the 24 recorded calls (G3 R2 field 9). D1's P2 is confirmed independently: 25 path B rows have a reader created before a writer; 2 of them carry an error-class constant (B56, B65); B56's flip is "none" [M4]. |
| Look-alikes | correct, with two reports of unknown truth | N1, N3, N5, N6, N7 as the cards say. N2 (SCH_LAB) depends on two parts that do not exist: row A24 is "undecidable" today. C3 L7 and the R2 "nearby" window would be reported; ground truth unknown. HS and CS lookups go to a TRANSIENT list. |

Two steps of the walk-through do not hold against the cards:
- O7 ("the required publications are those whose presence lowers the class") assumes that presence can only help. The C3 yield table refutes it: with the SB ID unpublished and EVS registered the call returns `CFE_SUCCESS` (row 3); with the SB ID published and SB not yet registered it returns 0xc2000002 with ERROR event 41 (row 4). Publishing the SB ID alone never lowers the class from any state and raises it from one. A mechanical O7 would name EVS's registration and EVS's own ID as required and not the SB ID, which is the object the card treats as the shared cause of C3 and R2. The headline (all three absent gives K2) survives; the "required order" sentence does not follow from the rule.
- The separation of the C5 neighbours C19, C20, C23, C32 ("the key is a caller-supplied argument") is marked [I] by D1. For N5 the card's reason is different: the application record is set before the spawn (641:es/cfe_es_apps.c:576), an order fact. The rule gives the right answer on the development rows for a reason the card does not support.

### 2b. Hidden inputs and parameters

1. T-BIND: one hand-written rule about the table service, "NOT automatic" (D1's words), introduced because C1 needs it. Framework-level, independent of any application: inside the boundary. It is one data path among several (CDS restore, command, message); D1 scopes the others to UNDECIDED-KEY.
2. Effect classes K1 to K4 are fitted one to one: K1/K3 carry C1, K2 exists only for C3 ("caught through one memory-safety pattern rather than any framework signal", D1 section 10), K4 carries R2 and C5. No class is per case, but the class set has not met a case it was not built for. "Which classes count" is declared as a value to pre-register; that choice decides whether R2 and C5 are detections.
3. O4, "when the published value is symbolic the present path is the callee's success return" [I]: an evaluation rule with no reference. It makes V2 true by assumption where presence has a second step (SB's ID is valid but SB is not yet registered: G3 R2 field 5).
4. The caller-supplied-key pruning rule above [I].
5. V0(d), "run-status constants with an error meaning": a subset chosen from enum comments (GS C: 3, 4, 8, 9). Shared with D2 and D3; framework-level; transcribed per generation, which D1 states.
6. L0 (script line order, synchronous library init): hand-stated layer, framework-level, already in 2a.
7. One passive boot per build "because the predictions are deterministic": true for R2; for C1 it holds because the natural order was the bad one 8 of 8 times in this configuration (G1 1.10), while the reporter saw the failure "quite often", not always (G1 1.6).
8. Evaluator limits, counting unit, survivor limit, hold ceiling without a bound: declared to pre-register. Not invented.
No input names a resource, a pair, an order, an event of one application or a probe site.

### 2c. On an unseen system

Static output per build [I]: order findings few. For the 17 applications of the v7.0.1 tree D1 itself predicts no UNENFORCED once-only site among the 11 by-name lookups (P8), one TOLERATED (CF with non-empty names), four or five TRANSIENT (HS, CS), one FIXED-PREMATURE that is framework-internal and repeats on every system (R2), and one report of unknown truth (the "nearby" window). The risk is the other list: UNDECIDED-KEY and UNDECIDED-EVAL. Step 2a today grades 147 of 178 rows of c5_641 "undecidable" (G5 field 9); D1's claim that V2 decides most rows is measured only by two crude filters on two development builds. Readers: an application or framework developer for each finding (code sites and labelled branch are given); the undecided list goes to the tool builder, not the user. A static UNENFORCED with no run behind it (C3, C5) asks the reader to believe an absence of proof.

## 3. D2: late-start black box (Layer A) plus a minimal look inside (Layer B)

### 3a. Replay

| Case | Result | Replay against the cards |
|---|---|---|
| C1 buggy | found by Layer A at application granularity | Baseline: CF absent, event 30 ERROR, 8 of 8 (G1 1.10 row 1). CF started last finds both names: runs [I; order recorded only under a hold]. Leave-two-out names bp (the lookup misses exactly when the name is absent, G1 1.5). V1, grade P, "CF requires BP first". The object comes only from Layer B (first divergent return, G1 1.9a). |
| C1 fixed | no violation in this build; verdict depends on an invented rule | V2 because d* about 2400 ms is above T_F = 1000 (DEF1:1731). With the 6.4.2-style value 30000 the same code is V1 (D2 section 10). Finding the edge at all needs W (6000) above the retry bound (2400): a consumer that retries longer than W shows no harm in the prefix record. |
| C2 | not found | Stated. |
| C3 buggy | found only with Layer B; localization claim wrong | "TIME first" reaches row 1 of the yield table: fault (H1) or 0xc2000002 in the trace (G3 C3 field 5). But "delaying SB alone or EVS alone gives the non-fatal rows 2-4, so the report is TIME requires {EVS, SB}" contradicts the card. SB alone late is row 3: `CFE_SUCCESS`, silent. EVS alone late leaves the SB ID valid and registered: the call reads only SB's own record, the DEBUG type is masked, `CFE_SUCCESS` (same outcome as row 5; 641:evs/cfe_evs.c:289-294 and evs/cfe_evs_utils.c:244-262, opened by me [F]). Rows 2 and 4 need a hold between two API calls inside EVS's or SB's init; Layer B delays only at task entry or after a spawn. Addition: a subset search over delayed tasks (D3's delta debugging). |
| C3 fixed | no report (correct) | Entry delay stalls the creator (642:es/cfe_es_start.c:936): absorbed. |
| C5 buggy | found only with Layer B | Post-spawn delay with the ES mutex free (G5 field 4): 0xc4000001, lost registration, RunLoop false (G5 field 7). Delay cap for 6.4.1: to pre-register. |
| C5 fixed | no report (correct) | Child blocks on the mutex (G5 field 6). |
| R2 | not found by the method; a candidate from the add-on E2 | D2 states that R2 "is not a violation at all" under its definition. E2 (a site whose error-class returns all precede all its successes) has no fixed-order condition and no static field behind it. Replayed on the FIXED C1 it fires: the lookup site returns -17 once and then 0 (G1 1.10 row 3). It also fires on any poll that first times out (`CFE_SB_TIME_OUT`, `OS_SEM_TIMEOUT` carry the error class, GS C). |
| Look-alikes | correct in Layer A | Everything identical in both orders cancels. Monitors (HS A4-A6, CS A9) are reported at grade T, as D2 says. |

### 3b. Hidden inputs and parameters

1. T_F rule ("harm at lag below the startup script timeout is a violation"): the constant has a reference (CFG:868-870) but its role is invented. The framework comment is about ES waiting for all applications after the whole script is parsed, not about a lag between two applications. The rule decides the verdict on the fixed C1. Declared [I], to pre-register.
2. W rule ((kill timeout + 1) x scan rate = 6000 ms): reference for the constants, invented role. It happens to exceed C1's retry bound.
3. Layer B delay cap "T_F minus one poll period, so the v7.0.1 entry guard is not broken": a value set so that one look-alike (G5 N10) comes out right.
4. H3 record list (task, pipe, route, table entry "or its TableLoadedOnce", EVS registration, CDS): a hand choice of dump records and of one field. Framework-level.
5. H2 run-status subset {3, 4, 8, 9}: as D1 item 5. In 6.4.x these are `#define` constants and event types are `#define` 0x01..0x04 with a `uint16` argument (641:inc/cfe_es.h:185-193, 641:inc/cfe_evs.h:111-114, :326-329) [M5]: "automatic from the enum" does not hold for that generation.
6. The postulate (harm-class steady state must not depend on an open order): no framework document states it (GS D). Declared.
7. The driver application in the script: generic, but it changes the system under test (one more task, pipe, subscription to the event message, shifted creation serials). "Declared, not measured."
8. Event multiset keyed by AppName: the name is derived from the caller's self-lookup, which is unsound (SAMPLE_APP printed as SAMPLE_LIB, G5 field 15). D2 notes it.
9. E2: introduced "only because of R2" (D2's words). A clause added for one case.
10. k, r: declared to pre-register.
No input names a resource, a pair or an expected result.

### 3c. On an unseen system

Layer A boots: k + r x [n + v(n-1) + e(1 + 7)] for n script applications. Reports [I]: one entry per (victim, provider) with a harm class; true one-shot dependencies are few; every application that a monitor table lists yields one grade T entry when it is the late one (HS application monitor sends an ERROR event the first time, GO g A4); tolerated dependencies appear as V2 with a measured tolerance. The report names two applications and a harm item, not an object; a reader must find the object, or Layer B must. Reader: the system integrator (who also owns the startup script). E2's volume is unbounded until the noise floor is measured [U].

## 4. D3: stale-decision differential (SDD)

### 4a. Replay

| Case | Result | Replay against the cards |
|---|---|---|
| C1 buggy | found | S0: one fail-class operation of CF that stands (W6:169, G1 1.10 row 1). S2 CF-last: BP creates the names well inside H (the second lookup 100 ms later succeeded, G1 1.14), so the gate ends by its condition; lookup 0. Localization by the shared key string (G1 4 item 4). Minimal flip at the lookup entry. V1. |
| C1 fixed | no report in this build; verdict depends on H | TOLERATED because H = 950 ms is below 25 x 100 ms. D3 states that a larger script timeout turns it into V1 "insufficient tolerance" (prediction 7). |
| C2 | not found | Stated. S2 with the boundary "return of the sync call" is the only schedule in the four designs that would lengthen the C2 window on a system that had the pair (G1 2.4). |
| C3 buggy | found only with additions | Additions: gating several threads at thread start (the runtime allows one held thread per gate, D3 section 8); H for 6.4.1 has no constant (GS G). Replay: TIME-first gives row 1 (fault = no-return, else 0xc2000002 = fail at 641:sb/cfe_sb_api.c:264). Delta debugging to {EVS, SB}: SB alone late is row 3 (success), EVS alone late has the outcome of row 5 (success, see 3a): correct. Bisection: releasing EVS after its `CFE_EVS_Register` (:393) gives row 3; releasing SB after :198 still fails (row 4 with the EVS ID a placeholder), after :257 not: correct against G3 C3 field 5. The evidence is the status or the fault; the unchecked index itself is not identified. The verdict depends on R-KEY dropping the untyped AppID argument (0 against 1); kept, the two runs would not align at that site. |
| C3 fixed | no report (correct) | ENFORCED (await). EVS's residue is listed SILENT-PREMATURE, not counted. |
| C5 buggy | found | S1 gates the creator at the return of `OS_TaskCreate` until the child reaches the end of init, blocks on a creator-owned lock, or H. Child: `CFE_ES_PerfLogEntry`, `CFE_ES_RegisterApp` (not a consumer, G5 field 5), `CFE_EVS_Register` = 0xc4000001 [M1 for the call order]. One call site per application: stands. w is not an API operation (stores closed by an unlock, G5 O1), so bisection over operations cannot name it; the row C31 lock acquisition does (D3 3.3). |
| C5 fixed | no report (correct) | The gate would hold a lock owner; child blocks: ENFORCED (lock). |
| R2 | found, in the weaker class V2 | Row B65 exists [M4]. (a) 24 of 24 early uses fail, (b) no such status after the store, (c) events 5 and 10 never repeated, (d) gate shows SB is not created while ES is held (G3 R2 fields 6, 9). The 17 failures at the one event-14 site may not "stand" (D3 says so). |
| Look-alikes | correct | N10 has a margin of one poll period against H. N9 gets no verdict. HS-type cyclic lookups do not stand: TOLERATED. |

Points where D3 is weaker than it reads:
- R-MASK excludes every entry whose class sequence differs between reference runs and reports it UNDECIDED. In a race-like case that occurs naturally in some boots, the fail/non-fail variation at one site IS the two orders, observed without intervention. As written, D3 would mask a naturally flaky C3 or C5.
- "One boot covers every spawn" (S1) does not hold in general [I]. In 6.4.1 a child that runs its whole init inside the window leaves through `CFE_ES_RunLoop` FALSE and a silent `CFE_ES_ExitApp` (G5 field 7); its thread ends with its slot in use; the next spawn can receive the same thread handle and read the dead task's record with `CFE_SUCCESS` (the mechanism of G5 field 15, P1), which hides the violation for the next application. In the v7.0.1 generation every child polls for its record up to 1000 ms (G5 field 8), so a gate of H = 950 ms per spawn accumulates over tens of applications and exhausts earlier applications' own sync timeouts; D3's rule (iii) then rejects the run.
- A gate that runs to H = 950 ms leaves 50 ms before the ES soft wait of 1000 ms expires (GS A4); such a run fails rule (iii) [I]. Only gates released by their condition give verdicts; the rest become UNDECIDED.
- I6 and I7 are called "automatic (enum parse)"; not so for 6.4.x [M5]. R-KEY "keep enum-typed arguments" keeps nothing there.

### 4b. Hidden inputs and parameters

1. H = script timeout minus one poll period: reference for both constants, invented role; decides the fixed C1 and protects one look-alike (N10). 6.4.x: to pre-register. Declared.
2. R-KEY: a rule "to pre-register"; the C3 verdict depends on one of its choices (drop untyped IDs).
3. I8, phase-boundary calls (four ES API names): "listed once per framework version, NOT automatic". Defines init-end, on which every gate release depends.
4. I10, creation requests: `CFE_ES_AppCreate`, `CFE_ES_StartAppTask` are internal functions whose names differ per generation: a hand list.
5. D7 (a)-(d): a second oracle clause that exists for R2. Its conditions are general, but only R2 exercises it.
6. Alignment rule (collapse immediate repetitions), N_ref, repetitions, run budget: declared to pre-register.
7. R-MASK: learned, not written; see 4a.
No input names a resource, a pair, an order, an event of one application or a probe position.

### 4c. On an unseen system

Boots: N_ref + 1 + about 2 to 3 per thread, plus localization per witness: for tens of applications with child tasks, on the order of 100 boots [I]. Reports [I]: V1 few (true one-shot dependencies, including "designed" exits such as CF's documented one, G1 4 item 10); V2 at least one framework-internal finding repeated on every system of that framework version; TOLERATED one per retry loop exercised; SILENT-PREMATURE about two dozen per build [M4]; ORDER-DEPENDENT-UNDIRECTED one per order-dependent count or key (the no-subscriber count alone differed 12 against 17 between two recordings, G3 R2 L6); UNDECIDED = mask plus rejected gates, size unknown. D3 says the last three lists are "never counted as detections". Then either nobody reads them (and silent harm stays silent) or a framework expert does. Reader of V1/V2: an application developer; each comes with two traces and a gate record, the best evidence of the four designs.

## 5. Comparison

### 5a. Design x case

"+" found; "(+)" found only with the addition named; "-" not found. Fixed versions and look-alikes: "ok" = no report, "ok*" = no report but for a fragile reason.

| Case | D0 | D1 | D2 | D3 |
|---|---|---|---|---|
| C1 buggy | + | + (static; key by T-BIND or one passive boot) | + (application pair; object needs Layer B) | + |
| C1 fixed | ok | ok | ok* (T_F rule) | ok* (H rule) |
| C2 | - | - (lists T1 facts) | - | - (has the schedule that would exercise it) |
| C3 buggy, reported order | - (out of scope; a neighbour sub-order with a hand-written EVS role) | (+) core actors, evaluator, K2; names the harm; static only | (+) Layer B "X first"; minimization wrong | (+) multi-thread gates, H for 6.4.1; localization correct |
| C3 fixed | ok (one unoriented entry) | ok | ok | ok (one listed entry) |
| C5 buggy | - as written; (+) release rule | + (static; never observed) | (+) Layer B | + |
| C5 fixed | ok* | ok | ok | ok |
| R2 | (+) field class, derived fixed order | (+) nested evaluation | - (E2 candidate; E2 also flags fixed C1) | + (weaker class V2; needs row B65) |
| Look-alikes | ok; send-without-subscriber pairs become unoriented entries | ok; 2 extra reports of unknown truth; N2 needs two fixes | ok; monitors at grade T | ok; N10 thin margin |
| Oracle clauses needed for the four code cases | 4 (F1, F2, F4, no-flip) | 4 effect classes | 3 layers (A, B, E2) | 2 (V1, V2) |

### 5b. Ranking by fidelity

1. D3. Every step of its four walk-throughs holds against the cards, including the one place where the cards are subtle (C3 sub-orders). One interventional clause covers C1, C3, C5; the second, weaker clause is declared as such. Its hidden inputs are parameters with a declared status, not knowledge of cases. Its faults (mask, S1 at scale, H) are repairable without changing the oracle.
2. D1. Same case coverage on paper, the only naming of C3's harm, the only horizon-free treatment of the fixed C1 next to D0, and the only design with measured discrimination on existing facts (P1, P2; P2 confirmed by my count). Below D3 because C3 and C5 stay verdicts without any execution, because each case leans on a different unbuilt static component, and because two of its rules (O7, caller-supplied key) fail the replay.
3. D2. Honest and fully parameterized; Layer A is the cleanest replay of all on C1. But the method's own definition excludes R2, the black box excludes C3 and C5, the C3 minimization is wrong, and E2 misfires on a fixed version.
4. D0. The architecture is the one D3 completes. As written it has a release rule that misses C5 [M1], a scope rule that excludes C3's reported order, a role table whose derivability is overstated for three of four cases, a fixed-order fact taken as given, and no proposed value for any parameter.

### 5c. Ideas of the weaker designs that D3 lacks

From D1:
- A bound read from the code decides "tolerated" (loop trip count and delay argument), instead of a gate length. Removes D3's dependence on H for the fixed C1 and for N10.
- A static proof of enforced or fixed order (must-happen-before closure with hard and soft wait edges). D3 has only the gate experiment (d); a static proof is cheaper and is not bounded by H.
- Prediction of the failure signature before the run, and acceptance of a gated run only if its trace equals the prediction: a principled artifact filter, stronger than D3's rule (iii).
- The placeholder-to-unchecked-subscript check (K2): the only way any design identifies C3's actual harm rather than a status next to it.
- Static resolution of table-borne keys and the list of consume sites that no boot exercised (ground-command paths): D3 reports "not exercised" without knowing what is there.
- Once-only versus cyclic regions as the definition of "cannot heal", independent of whether a site happened to repeat within the snapshot.
- A proposed counting unit (one finding per publisher and element), needed for C3 and R2 where one cause has 36 or 39 sites.
From D2:
- The harm item "an application the script requested is absent at quiescence", anchored in requirement cES1508.1 (GS D). D3 compares only per-thread operation classes and has no steady-state check; it also cannot use the exit event, which is typed INFORMATION (G1 summary line 10).
- Late start through the framework's own start command: both orders of whole inits with no probe inside the system; an independent confirmation channel and a cheap first sweep.
- A measured tolerance (lag bisection at the framework poll quantum) instead of a binary verdict at one horizon, and the mirror probe ("head start") that shows when no script order is safe (the C1 reporter's observation, G1 1.6).
- The category "harm that varies between repetitions of one schedule" (N) as a finding. This is the direct repair of D3's mask.
- The restart probe for stale identifiers (G1 4 item 8), outside all five cases but inside the topic.
- An explicit list of what differs between a script start and a command start, checked per run.
From D0:
- The time margin between w and u in the natural run, reported with every finding, so a reader can judge how likely the bad order is.
- Comparison of the arguments that follow from u (renamed identities): the only signal for C3's silent misattribution (event attributed to application 0). D3 files this under UNDIRECTED without saying what is compared.

## 6. What the synthesis must fix

1. Creator gate after a spawn: release on "child reached the end of init, or blocks on a creator-owned lock, or bound", never on "first framework call" [M1]. Use zero-margin release for the verdict run and one spawn per boot where a child can die inside the window or where children poll with a bound.
2. C3 needs several publishers late at once and a subset search; single-pair flips and single-task delays do not reach the reported order, and single delays of SB or EVS show nothing (G3 C3 field 5 row 3; 641:evs/cfe_evs.c:289-294).
3. The verdict on a tolerant consumer (fixed C1) must come from the code's own bound, with the measured margin as an annotation. A horizon derived from the script timeout may limit gates but must not decide "violation or tolerated". If a horizon rule stays, pre-register it and state that the fixed C1 flips with it.
4. Fixed-order premature use (R2) must be conditioned on a static shared-field relation and on a derived or tested proof that the order is fixed. A bare "failures precede successes at a site" rule flags the fixed C1 and routine timeouts. The field class must admit several writers (B65 has 3 [M4]).
5. No hand-listed framework orders. Core serialization, script order and library init must be derived (wait-loop rule) or tested (gate), per build.
6. No role or kind table for classes whose roles are not derivable (EVS registration, ES task record, SB routes). Detection must not need object identity there; D3 shows it need not.
7. Natural class variation between reference runs is a finding, not a mask entry. Mask only non-class variation.
8. Vocabulary: transcribe per framework generation and say so [M5]; decide whether the EVS-internal sender counts and how it is observed [M2]; state that routine error-class statuses are handled only differentially.
9. Identity from the creation edge everywhere, including the victim name in any event-based comparison (G5 field 15).
10. State in the paper that each clause of the oracle is exercised by at most two development cases, freeze every rule and parameter before any held-out build, and pre-register how reports of unknown ground truth (C3 L7, R2 "nearby" window, CF's documented exit) are scored.
11. Counting unit and de-duplication: one cause, many sites and occurrences (36, 39, 24); framework-internal findings repeat on every system of that framework version.
12. Measure the noise floor of a clean boot on each build before any volume claim [M6].
13. The "minimal required set" must not assume that publication only helps (C3 rows 3 and 4).
14. Every list that is "not counted as a detection" needs either a ranking or an explicit statement that it is not read; otherwise the human oracle returns as the reader of two dozen silent entries per build [M4].
15. C2: say plainly that no design detects it; the only related outputs are the list of publications after a ready declaration (D1) and the gate at the return of the sync call (D3).

## 7. Commands behind [M]

- M1. `grep -n 'CFE_ES_\|CFE_EVS_\|CFE_SB_'` on 6.4.1 apps/sample_app/fsw/src/sample_app.c (:45 PerfLogEntry, :82 RegisterApp, :87 EVS_Register, :95 CreatePipe) and apps/ci_lab/fsw/src/ci_lab_app.c (:96, :146, :148, :152); `awk '/^void CFE_ES_PerfLogAdd/,/^}/' 641:es/cfe_es_perf.c | grep 'TaskTable\|GetAppID\|LockSharedData\|OS_TaskGetId'`: no line.
- M2. `grep -rn 'EVS_SendEvent *('` in 641:inc/*.h and 641:evs/*.h: only evs/cfe_evs_utils.h:94 (the public header has `CFE_EVS_SendEvent` at inc/cfe_evs.h:275); v7.0.1: cfe/modules/evs/fsw/src/cfe_evs_utils.h:263. `sed -n '194,219p' 641:evs/cfe_evs_utils.c`: `EVS_SendEvent(CFE_EVS_ERR_UNREGISTERED_EVS_APP, CFE_EVS_ERROR, ...)`.
- M3. `grep -n '"file"' /home/user/work/procrace/c1_build/pre/1df262a/build/native/default/compile_commands.json | grep cf_def_config`: :1391 (tx) and :1397 (rx) only; `ls exe/*/cf/` shows cf_def_config.tbl in both targets.
- M4. awk over REL(r2_core), rows starting "  B<n> ": 155 path B rows; 55 with the phrase "reader actors created before it:"; 25 where that phrase is followed by an actor name; of those, 2 with an error-class constant (value at or above 0xC0000000) anywhere in the row: B56 and B65. B56 "flip: none: the initial value and the written constants give the same result of the callee check". B65: "writers 3 from WRITERS", writers {CFE_SB_TaskMain, main (process)}, "handling: discarded 39", "feasibility: undecidable: fixed unconfirmed (creation order + await ...)", "flip: static-undetermined ... the result of EVS_GetAppDataByID". My pattern for "error-class constant" is crude (decimal or hex value in the row text); it agrees with D1's count but is not D1's command.
- M5. `grep -rn 'define CFE_EVS_\(DEBUG\|INFORMATION\|ERROR\|CRITICAL\)'` 641:inc/cfe_evs.h:111-114 (0x01..0x04); prototype :326-329 (`uint16 EventType`, `uint32 AppID`); run statuses `#define` at 641:inc/cfe_es.h:185-193.
- M6. `grep -n -i '0xc[0-9a-f]\{7\}\|TIME_OUT\|NO_MESSAGE\|-17x'` on R/plan/results_w6_20261029.md: only the case rows (:169-171, :279, :394, :465). No count of other error-class statuses of a clean boot exists in that document.

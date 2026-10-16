# Review X1: formal soundness and claims of SPEC v1 (Kairos)

- Date: 2026-10-12. Reviewer role: adversarial reviewer (formal soundness, claims, novelty). Read-only review. The only file written is this one.
- Target: `scratchpad/arch/design/SPEC_v1.md` (2332 lines), cited as `SPEC:line`.
- Evidence base: [R01]–[R07] (`scratchpad/arch/0*.md`, cited with line numbers as `R04:259`), and the pinned tree `/home/user/work/procrace/cfs_ref/cFS`. Code paths below are relative to it. Every code line cited here was re-read for this review.
- Labels: **[F]** fact checked in code or a report; **[I]** inference by the reviewer, not executed. Literature is cited only as R04/R05 verified it, with their status tags.

---

## 0. Verdict

**The spec is not yet sound as written. It is repairable without changing the architecture.**

**What holds.** The headline R2 argument (ES half) survives an adversarial re-check against the code (§1). The MustBefore proof is a correct induction on execution position. Separating class from evidence is right. The honesty rules (no "safe", NOT_RUN is never a false negative, budgets swept) are mostly respected.

**Two critical defects invalidate guarantee G2 as stated:**
- **X1-C1.** Skeletons cut message loops at the horizon h. Yet certificates and the classes `inactive` and `no-provider` are claimed at b = ∞ and transferred to σ_E.
- **X1-C2.** The containment lemma and G2 ignore the model level M. M_api deliberately is **not** an over-approximation of the POSIX OSAL, and the spec itself says so.

**Twelve major issues.** They cover:
- contract semantics: where the stamp/argument LP sits; key semantics for record-indexed resources;
- the multi-instance precondition of PWB;
- prediction soundness on sparse API-level traces;
- the σ_U∞ / timeout-dependent semantics;
- teardown disablers, which defeat the S3 calibration gate;
- message-loop provenance and the looper rule;
- await predicates that are declared but never checked;
- a DPOR guarantee conditioned on a test;
- lemma L3;
- σ_E wording that breaks the no-"safe" rule;
- novelty and baselines against the closest prior work (RVPredict/GPredict, InitRacer, ExceptioNULL).

---

## 1. Checks that the spec passes (adversarial re-verification)

| Claim | Check | Result |
|---|---|---|
| T-TERM cut in R19 (A.1 step 4) | `cfe/modules/es/fsw/src/cfe_es_start.c:810–846`. On StartAppTask failure the record is memset (`:823`) and `MainTaskSyncDelay` is skipped. `ReturnCode != CFE_SUCCESS` leads to `CFE_PSP_Panic` (`:845`) on every failure path. | [F] holds. The failure-branch writer `:823` is removable by PWB(o, ·) through the frontier "failure branch". |
| ES raise after `TaskInit` on all paths (A.1 step 5) | `cfe_es_task.c:114–134`. TaskInit failure only sets `AppRunStatus`, and `WaitForSystemState` (`:134`) still runs. | [F] holds. The early-return regression argument is correct. |
| EVS raise after `TaskInit` on all paths | `cfe_evs_task.c:213–229`. On failure, `CFE_ES_ExitApp(CORE_APP_INIT_ERROR)` → `CFE_ES_ResetCFE` → **returns** (`cfe_es_api.c:363`), then the raise at `:229`. | [F] holds under the spec's non-terminal `ResetCFE` model (§3.5). `m_EVS` still precedes the raise. |
| L-MB proof | Each rule's justification refers to a strictly earlier event, so the time induction is well founded. PWB pruning inside T-OBS is justified by the induction hypothesis at the earlier writer occurrence. | Sound, given A-SKEL and A-WS. |
| L-PWB, L-FO, L-PR (single-instance actors, must-keys) | Proof sketches. | Correct under the stated premises (but see X1-M2 and X1-M3 for missing premises). |
| GetTaskFunction bound "≤ 21 checks" | `cfe_es_apps.c:570–595`: delay, check, break when `Timeout <= 0`, step 50 from 1000. | [F] 21. |

---

## 2. Issues (most severe first)

### X1-C1 — CRITICAL — Message loops cut at h break A-SKEL, while universal classes claim b = ∞

- **Section:** §4.3, §1.8, §1.10, §4.5, §6.1–6.3.
- **Problem.**
  - Message loops are "cut at the main-loop horizon h", and σ_U∞ also cuts non-terminating loops at h (SPEC:924, SPEC:271). Only "other loops" are star-closed (SPEC:925).
  - A-SKEL nonetheless claims that every skeleton over-approximates its actor (SPEC:929, SPEC:1364).
  - The L-MB proof step "the actor reached n_k from some skeleton predecessor" (SPEC:1377) fails for occurrences beyond h.
  - These verdicts are all stated without a bound or at b = ∞, and G2 transfers them to σ_E (SPEC:1410):
    - certificates "(§4.5, b = ∞)" (SPEC:329, SPEC:331);
    - INACTIVE ("the skeleton over-approximates paths, so 'no path' is sound", SPEC:986);
    - NO_PROVIDER (`W_j = ∅`);
    - the K-CAP closure bound (SPEC:755).
- **Evidence [F].** Route writers live inside message-loop handlers. TO_LAB's `TO_LAB_AddPacketCmd` (`apps/to_lab/fsw/src/to_lab_cmds.c:170`, dispatched at `to_lab_dispatch.c:58–59`) calls `CFE_SB_SubscribeEx` at `to_lab_cmds.c:220`. SBN loads modules on demand inside its loop (SPEC RC4 cites `sbn_app.c:1060–1088`).
- **Failure scenario.**
  - With h = 0 (the sweep starts at 0, SPEC:924), the `:220` writer has no skeleton node. A K-ROUTE use for a stream that only that handler subscribes then gets `W = ∅` → `no-provider(B)`, a universal class.
  - A use reached only in the second handler invocation gets `inactive(B)`.
  - For PWB: with m in iteration 1 and a in iteration 2, no skeleton path m →* a exists, so PWB(a, b) is certified falsely.
  - The K-CAP "protected" bound misses puts beyond h.
- **Fix.**
  1. Build two skeleton views. The **certificate view** star-closes every message loop: back edge, explicit iterate/exit choice, dispatch as a local choice. INACTIVE, NO_PROVIDER, MB, PWB, the frontier rule and the K-CAP bound are computed only on this view. The h-cut view is used only by the explorer and σ_U∞ runs, and every verdict from it carries h in b.
  2. Add a proofcheck rule that rejects any certificate step whose node lies in an h-truncated region.
  3. Add a litmus test with a writer reached only in handler iteration 2. The expected result is "not no-provider, not inactive".

### X1-C2 — CRITICAL — The containment lemma and G2 ignore M, but M_api is not an over-approximation of the binary

- **Section:** §1.4, §1.8, §4.12 item 6, §6.1 A7, §6.2 L-CONT, §6.3 G2.
- **Problem.**
  - Containment (SPEC:277), L-CONT (SPEC:1393–1397), the model statement "by A-SKEL and A7 it contains the real executions" (SPEC:1372) and G2 (SPEC:1410) are all stated for `Exec(B,σ_U)` without M.
  - M_api makes each OSAL API atomic: "lookups return found or not found" (SPEC:188). The spec defines verdicts "violable only under M_impl" as implementation-defect candidates (SPEC:193). Executions that exist only under M_impl are therefore real POSIX executions **outside** `Exec(B,σ_U,M_api)`. Examples: `OS_ERR_INCORRECT_OBJ_STATE` from a RESERVED record, the type-lock leak, and silent task loss after the bounded wait (SPEC:657–660, SPEC:668).
  - A7 ("over-approximate the real behaviour", SPEC:1358) is false for M_api by design. M_api is also the default level for every cone (SPEC:1131).
- **Failure scenario.** A `protected(σ_U, M_api)` certificate for an OSAL by-name lookup is transferred by G2 to σ_E. The real binary can return `INCORRECT_OBJ_STATE` (App. A.4), so the transferred universal claim is false. O4(b) (SPEC:614) then also flags the observation as a model fault and suspends a correct rule.
- **Fix.**
  1. State containment per level: `Exec(B,σ_E) ⊆ Exec(B,σ_U,M_impl)` under A7 restricted to the M_impl paths and A10.
  2. Re-scope M_api verdicts as relative to the OSAL **API contract** (a portability claim). They transfer to σ_E only if (i) the cone contains no M_impl-relevant operation, or (ii) the M_impl verdict agrees.
  3. G2 must name M, and `kz_verdict` must carry per-M results.
  4. O4(b) against an M_api-only certificate becomes `portability-hazard` evidence, not `model_fault`.

### X1-M1 — MAJOR — Value-carried conjuncts (stamps, keys from shared loads) are evaluated at the wrong LP; the argument → resource translation is unspecified

- **Section:** §1.2 (`stamp`), §1.6, §2.3 Φ excerpt, §4.8, App. A.1.
- **Problem.**
  - K-ID's LP is EVS's "slot check" (SPEC:555), and readiness is the state after lifecycle LPs that precede **u's LP** (SPEC:239–241).
  - The value checked is a copy loaded earlier by the caller [F]: SB passes `CFE_SB_Global.AppId` by value at argument evaluation (`cfe/modules/sb/fsw/src/cfe_sb_api.c:226–228`), and EVS tests the copy (`cfe/modules/evs/fsw/src/cfe_evs.c:185–194`).
  - In addition, the contract says `svc_valid(arg.AppID) && evs.reg(arg.AppID)` (SPEC:572), while A.1 silently switches to "conjunct `svc.identity(SB)` SET, support location `CFE_SB_Global.AppId`" (SPEC:2223). The step that turns a parameter with role `stamp` into the resource whose state matters is not defined anywhere. Yet that step is exactly what makes the R2 claim "generic" ("No SB-specific contract exists", SPEC:2222).
- **Failure scenario [I].** SB publishes between the caller's load and the slot check (a reachable interleaving for any post-init SB event). Then:
  - the state at the LP is SET, but the outcome is `ILLEGAL_APP_ID`;
  - O4 fires a false model fault and suspends a correct rule;
  - `kz predict`, which targets the order of LP versus publication (SPEC:1065), accepts this as a **good** witness, so a fixed-order instance can be mislabeled schedule-dependent; it can also miss bad witnesses in the windows.
  - Fixed-order certificates survive only because the conjunct is monotone. A PROTECTED certificate whose disabler set is checked against the slot-check node, not the load, is unsound if a disabler can fall between the load and the check.
- **Fix.**
  1. Give each conjunct its own LP.
  2. For a parameter with role `stamp` or key whose value comes from a shared load, define the **capture point** as that load. It is found by def-use over the closed IR from the call argument, and is emitted as a fact `derives(arg, loc, site)` by `kz extract`/`kz modref`.
  3. Evaluate Req for that conjunct at the capture point. Make the `derives` fact part of the certificate, checked by `kz proofcheck`.
  4. In traces, probe the capture site, or treat the LP interval as [capture, check] and require both ends to be ordered.

### X1-M2 — MAJOR — PWB and the frontier rule assume one instance per static actor; this is never established

- **Section:** §1.2, §4.4 T-SPAWN, §4.5, §6.2 L-PWB.
- **Problem.**
  - "Under the default basis every static actor has exactly one instance" (SPEC:124) is asserted, not derived.
  - The L-PWB proof uses "τ's run would contain m … a" (SPEC:1385), which needs m and a in the **same** run of the actor.
  - Skeleton nodes are per static actor, and T-SPAWN takes a meet over spawn sites (SPEC:940), which allows spawn sites inside loops and handlers. `CreateChildTask` sites are resolved by constant argument (SPEC:645), and B_restart creates new generations (SPEC:740).
- **Failure scenario.** Two instances τ1 and τ2 share nodes. m occurs in τ1, and a occurs in τ2 after m. Then `m ∉ Anc(a)` and `m ∈ MB(b)` both hold, so PWB(a, b) is certified, but a follows b. This yields a false FIXED_ORDER verdict, a false disabler exclusion in PROTECTED (b), and false writer pruning in T-OBS.
- **Fix.**
  1. Add the precondition `once(τ)`: every spawn site of τ executes at most once per execution, and spawn sites are pairwise exclusive. Prove it by an at-most-once analysis (spawn site outside every loop and handler region, plus PWB between sites). Record it in the certificate and re-check it in `kz proofcheck`.
  2. Otherwise, index nodes by instance (finite generations from B.Input) or forbid frontier arguments for τ.
  3. The same analysis enables a cheap, computed rule `once(a) ∧ a ∈ MB(b) ⇒ PWB(a, b)` (see X1-M3).

### X1-M3 — MAJOR — Key semantics for record-indexed resources: PROTECTED needs must-keys; writer exclusion rests on unlisted relational facts

- **Section:** §4.2 steps 1, 6, 8; §4.5; App. A.1 steps 3–4; G10.
- **Problem (a), must-key.**
  - PROTECTED (b) needs a writer that "definitely establishes P_j **for the key**" (SPEC:982).
  - Key unification is may-alias only ("may equal", SPEC:909), arrays of records are index-collapsed (SPEC:888), and the entry statuses describe guards, not keys (SPEC:904–908).
  - [F] Core App records are chosen at run time by `CFE_ResourceId_FindNext` (`cfe_es_start.c:789`), so `SetUsed(AppRecPtr, PendingAppId)` (`:818`) is only a may-writer of any given app's record.
- **Problem (b), writer exclusion in the R2 EVS half.** For `o_EVS`, ES-owned writers can occur after `f_EVS`, because ES main runs concurrently with the startup thread:
  - ES main's raise (`cfe_es_task.c:134`);
  - ES main's `CFE_ES_ExitApp` (`cfe_es_task.c:200`), reachable early under σ_U (TaskInit failure → CORE_READY wait times out → loop skipped). Its STOPPED store sits in the external-app branch of `cfe_es_api.c:292–400`.

  The frontier rule cannot prune the raise: no node of ES main after the raise is in `MB(f_EVS)`. Pruning therefore depends on (i) the mapping from task to app record (`owner(exec)` resolved through the task record's AppId, which is set from `PendingAppId`), (ii) the freshness of `FindNext`, and (iii) the per-record `Type == CORE` (`cfe_es_start.c:795`). None of these is in the abstract domain of step 6, and none is listed as a DECLARED_TRUSTED entry. A.1 says only "by the same argument through `o_EVS`" (SPEC:2241).
- **Failure scenario.** Without those facts, `W_o(o_EVS)` keeps ES's writers, T-OBS gives nothing about `m_EVS`, and the EVS part of the S3 exit gate (4 of 24 uses) cannot be certified. If the facts are used silently, G10 ("every writer entry it relied on") is violated.
- **Fix.**
  1. Add a key status `MUST_KEY` (same constant name or the same symbolic value as the instance key), required for x*.
  2. Model the reservation as an **allocation**: one abstract record per unrolled reservation node, with freshness DECLARED_TRUSTED and refutable by observed IDs.
  3. Add the computed rule `once(a) ∧ a ∈ MB(b) ⇒ PWB(a,b)`. It prunes `raise_ES` from `W_o(o_EVS)` without trusted facts, because `raise_ES ∈ MB(o_ES) ⊆ MB(f_EVS)`.
  4. Write out the `o_EVS` writer set in A.1 and list every trusted entry. Add a mutation that drops the once-fact.

### X1-M4 — MAJOR — Prediction is called "trace-sound", but Kairos traces are sparse API-level projections

- **Section:** §4.9, §6.2 L-PRED, §6.3 G4, §4.12 item 3.
- **Problem.**
  - L-PRED says a witness is "feasible for any program able to produce π" (SPEC:1403), citing maximal causal models and RVPredict. Those guarantees concern traces that contain the shared accesses and the branch/control abstraction ([R04] `04_lit_dynamic.md:221–225`).
  - Kairos traces contain the synchronization skeleton, LP intervals and outcomes at contract sites only (SPEC:1022–1028, SPEC:1243).
  - Shared reads that steer control inside the cone are unconstrained. Examples: direct reads of globals, table memory, unprobed fields.
  - Yet `predicted` is ranked above model evidence as "trace-sound" (SPEC:1128).
  - Separately, the constraint "an acquire needs the previous release" (SPEC:1072) literally forbids the lock reversal that step 3 allows (SPEC:1076).
- **Failure scenario [I].** A witness reverses two critical sections. An unobserved read in the consumer's prefix, of a field written by the moved critical section, takes the other branch, so the use is never reached. The record still says `predicted` (trace-sound).
- **Fix.**
  1. Add assumption A-COMM and a static check from `kz modref`. For each cone actor's prefix up to u, every location read that another cone actor may write must be a Φ support location whose writers are all observed, or must be provably constant. Otherwise label the witness `predicted-modelled` and rank it with model evidence.
  2. Reword the constraint to "an acquire needs the lock free".

### X1-M5 — MAJOR — σ_U∞ and the timeout-dependent refinement are ill-defined and mislabel liveness failures

- **Section:** §1.8 (σ_U, σ_U∞, refinement), §4.3 retry loops, App. A.2, A.4.
- **Problem (a), no-transition states.** σ_U∞ disables the exhaustion branch of delay-paced count-bounded retries (SPEC:271, SPEC:922). After the last failed attempt the actor has **no** enabled transition even when the predicate has become true. That is not a "stall" (defined as "waits forever on a false predicate", SPEC:271), and it is neither good nor bad. The refinement test "only good executions or stall" (SPEC:286) is therefore undefined exactly for the C1 claim (SPEC:2288).
- **Problem (b), permanent stalls.** A stall whose predicate can **never** become true counts as non-bad, so the instance is refined to "timeout-dependent". Examples:
  - a raise skipped inside the core window, so `MainTaskSyncDelay` can never succeed (SPEC:674, SPEC:2266);
  - a provider task that never runs (K-SPAWN F7).

  The root cause there is ordering logic or liveness, not a timing margin.
- **Problem (c), contradictory retry semantics.** σ_U lets "every bounded retry ... take its timeout branch at any point" (SPEC:270). That contradicts exact unrolling to the reference bound (SPEC:922) and makes CF's bound irrelevant in σ_U.
- **Problem (d), one-sided refinement.** Nothing represents "good only through a timeout".
- **Fix.**
  1. Treat delay-paced retries in σ_U∞ as patient awaits: exhaustion is enabled only if the retried predicate is permanently false.
  2. Split outcomes into good / bad / `stall-permanent`. `stall-permanent` keeps the class schedule-dependent with harm `task_never_runs` or `hang_or_deadlock`.
  3. Define timeout-dependent as: every σ_U bad execution takes a timeout transition, and from the state before that timeout its σ_U∞ continuation reaches only good outcomes.
  4. Compute and report the class independently under each σ.
  5. Make σ_U's retry exhaustion "after the reference number of attempts".

### X1-M6 — MAJOR — Teardown disablers defeat the expected PROTECTED certificates (A.7, S3 exit gate)

- **Section:** §3.5, §4.3 guards, §4.5 PROTECTED (b), App. A.7, §9.3 S3.
- **Problem.**
  - `CFE_ES_ResetCFE` is modelled as non-terminal, with `OS_DeleteAllObjects` running "concurrently with running tasks" (SPEC:698). [F] `psp/fsw/pc-linux/src/cfe_psp_start.c:507`; [R01] `01_cfs_osal_psp.md:524`.
  - Branches on API outcomes are explicit choices (SPEC:918), so a core init failure → `ExitApp` → `ResetCFE` is reachable in the skeleton. [F] `ExitApp` returns after `ResetCFE`: `cfe_es_api.c:363`.
  - `OS_DeleteAllObjects` is therefore a reachable disabler of every OSAL object, unordered with app lookups. PROTECTED (b) needs every reachable disabler PWB-ordered (SPEC:982), and that fails.
  - The S3 exit gate requires "timebase lookups protected by certificate" (SPEC:1724, SPEC:2310).
- **Failure scenario [I].** The calibration instance stays open (or becomes schedule-dependent with a teardown witness), and the MVC exit gate cannot pass for a reason the spec does not anticipate. Worse, real startup instances get mixed with teardown races.
- **Fix.**
  1. Add a **shutdown epoch** to Φ. `CFE_PSP_Restart` / `ShutdownReq` enters phase SHUTDOWN.
  2. Give every contract an implicit scope conjunct "not after the shutdown request". Uses after the request become a separate K-TEARDOWN family with its own class.
  3. Alternatively, record treating restart as terminal for contract scope as a deviation.
  4. Re-derive A.7 afterwards. This also needs X1-M7, so that command-triggered resets are excluded under an empty B.Input.

### X1-M7 — MAJOR — Message loops: dispatch is over-approximated (no message provenance); between-handler program order inside VC brings back CAFA's false dependency

- **Section:** §4.3, §3.9 looper rule, §1.9 VC, §1.10 shapes, §7.3.
- **Problem (a), dispatch.**
  - Dispatch on the received MsgId is a data branch, so it is an explicit local choice (SPEC:918, SPEC:924). Command-only handlers are therefore reachable in every skeleton even when B.Input is empty.
  - The expected verdicts "K-ATOM.tbl-load: inactive in the default B" (SPEC:1492) and "HS AppMon: inactive" (SPEC:1497) cannot be produced by the specified algorithm.
  - ES's reset commands also become reachable, which feeds X1-M6.
- **Problem (b), looper order in VC.**
  - Between-handler program order is an O-edge inside VC, "all definite edges" (SPEC:751, SPEC:311–313), and shapes are taken from that HB (SPEC:336–342).
  - Two handlers on one looper are then never `∥hb`, so the UNORDERED/WRONG-WAY split inherits the false dependency that CAFA and DCatch Rule-Pnreg remove ([R04] `04_lit_dynamic.md:364–366`, `:528`).
- **Fix.**
  1. Specify a message-provenance analysis. The MsgIds that can reach pipe p are the routes of B intersected with the transmit sites reachable in B, plus the MsgIds in B.Input. Fold dispatch with it.
  2. Exclude between-handler program order from the HB used for shapes and for the UNORDERED route. Keep it only as a FIFO/send-derived constraint in prediction.

### X1-M8 — MAJOR — T-OBS rests on await predicates that Φ declares and nobody checks against the code

- **Section:** §2.3 (rule CORE-SERIAL), §4.3 poll loops, §4.5 checker, §6.2 L1.
- **Problem.**
  - R19's predicate and falsifier are authored strings (SPEC:561–568).
  - Poll loops are "recognized instances" from a fixed list (SPEC:921).
  - `kz proofcheck` re-checks the falsifier and `W_o` but not the predicate (SPEC:994–995). L1 is an unproved assumption (SPEC:1401).
  - The R2 certificate's central step is this T-OBS. The "generic template" claim (SPEC:2222) hides that the ordering proof depends on a framework rule authored with R2 known.
- **Failure scenario [I].** A declared predicate stronger than the loop's real exit condition (for example, one that forgets RESERVED counts as used, or that freed records satisfy it) lets T-OBS derive MB facts the code does not guarantee. Only an observed contradiction (O3) would expose it.
- **Fix.**
  1. Extract the await predicate from the IR exit branch of the loop, by symbolic evaluation over the step-6 domain. Φ only names or binds the loop.
  2. `kz proofcheck` re-derives the predicate and checks that the code's exit condition implies the observed conjunct C.
  3. Recognize poll loops by pattern (delay plus exit test), not by list.
  4. Report R2 as "generic contract + framework rule R19", in the refinement accounting of §7.2.

### X1-M9 — MAJOR — The DPOR guarantee is conditioned on a differential test

- **Section:** §4.10 item 4, §6.2 L-DPOR, §6.3 G5, §4.15.
- **Problem.**
  - "The claim is therefore made conditionally on the differential test" (SPEC:1399), and G5 is "conditional on L-DPOR's differential test" (SPEC:1413). Passing enumeration tests on small models does not establish coverage on larger models.
  - The theorem for blocking and disabling is only status P (JACM, [R05] `05_lit_systematic_static.md:139`).
  - §4.15 claims "one execution per Mazurkiewicz class" (SPEC:1211). That is optimality, which source-DPOR without wakeup trees does not give.
- **Fix.**
  1. State G5 as conditional on a named correctness theorem for the exact variant used. If it stays P, label G5 unverified.
  2. Alternatively, use an encoding whose proof the spec writes out (locks and awaits as always-enabled attempt transitions with explicit blocked states). Keep the differential test as validation evidence only.
  3. Drop "one per class" unless wakeup trees are adopted.

### X1-M10 — MAJOR — Lemma L3 (lock-atomic macro-steps) is stated with an insufficient condition

- **Section:** §4.10 item 5.
- **Problem.** L3 makes a critical section atomic when its tracked locations are accessed only under the lock (SPEC:1103). That is not enough. A critical section may contain:
  - blocking operations;
  - operations on other synchronization objects;
  - paths that never release the lock.

  [F] `CFE_ES_DeleteChildTask` calls `OS_TaskDelete` (cancel + join) inside the ES lock (`cfe_es_api.c:1348` lock, `:1383` delete; SPEC:675). K-LOCKBAL paths leak the lock (SPEC:659).
- **Failure scenario [I].** Atomic macro-steps remove the interleavings in which the joined victim, or a waiter, runs inside the critical section. This hides K-LOCK.cancel and deadlock executions and can yield a false `protected` within b.
- **Fix.** State L3 as a Lipton/mover reduction:
  - acquire is a right mover and release a left mover;
  - lock-protected accesses are both-movers;
  - at most one non-mover (a blocking call, an operation on another synchronization object, or an unprotected shared access);
  - release on every path (leak paths excluded under M_impl);
  - no nested blocking acquire.

  Check these conditions statically. Critical sections that fail stay fine-grained.

### X1-M11 — MAJOR — σ_E wording breaks the no-"safe" rule; the evidence ranking mixes realization with naturalness

- **Section:** §7.3, App. A.2/A.3, §4.12, §1.8.
- **Problem.**
  - "Timing-protected in σ_E" appears in an expected verdict (SPEC:1485) and in the worked examples (SPEC:2272, SPEC:2278). No σ_E protected class can come from the absence of observed bad runs. This turns "no alarm" into "safe", which [POL] and the user's rules forbid.
  - The single ranking "… < observed < confirmed" (SPEC:1128) puts derived and held confirmations above natural observations. "σ_E classes come only from observed or confirmed witnesses" (SPEC:1126) then lets a derived hold plus a natural good run produce `schedule-dependent(σ_E)`. §1.8 itself separates held/timeout-exercised runs from natural σ_E (SPEC:286, SPEC:1157).
- **Fix.**
  1. Replace the wording with "no σ_E bad witness in n of N runs (pool key); σ_E class unknown".
  2. Split evidence into two axes: realization (model / predicted / realized on the binary) and schedule naturalness (natural / held F0 / gated F1 / timeout-exercised).
  3. A σ_E class needs natural witnesses on both sides. Otherwise label it `σ_E(held)`.

### X1-M12 — MAJOR — Novelty and baselines misattribute prior work, and the strongest same-family comparators are missing

- **Section:** §1.11, §8.3, §6.4 item 14.
- **Problem (a), loss row.** §1.11 files SB no-subscriber drops under G5 with "No HB rule covers them" (SPEC:375). R04 maps the same drop to **InitRacer's late-event-handler registration** (`04_lit_dynamic.md:367`, `:529`).
- **Problem (b), lock reversal.** §1.11's "What Kairos adds" lists the key as conflict object and reorderable lock edges (SPEC:374). R04 attributes key-conflict, lock-as-atomicity reversal to RVPredict/GPredict-style constraint encodings (`:259`, `:492`). GPredict already checks generic (typestate-like) properties on predicted traces ([R04] `:249–251`).
- **Problem (c), missing citation.** ExceptioNULL (FSE 2012, V-abs, "use before initialized", SMT plus static lockset pruning, `:253–255`, `:534`) is the closest predictive work for readiness and is not cited.
- **Problem (d), G3 novelty unsearched.** R04 records that the novelty of timeout-conditional barrier edges (G3) was **not searched** (`:673`). The spec does not carry that caveat.
- **Problem (e), weak baselines.** The baselines (SPEC:1566–1580) compare against lock-order-preserving WCP/SHB, which are known not to reverse conflicting critical sections. They omit the lock-reversing family, despite the norm "compare against the strongest runnable tool of the same family" ([R05] `05_lit_systematic_static.md:446`).
- **Fix.**
  1. Correct the §1.11 attributions. Cite InitRacer and ExceptioNULL as the closest prior work for late registration and use-before-init. Record G3 novelty as an open literature check.
  2. Add three reimplemented baselines over the same traces:
     - (i) an MCM/GPredict-style lock-reversing predictor over the same cone, without Φ outcome semantics, using the SMT-LIB emitter after D5 or an explicit equivalent;
     - (ii) a plain typestate runtime monitor on the observed traces, which shows what the certificate adds for R2;
     - (iii) a DefUse-style learned def-use invariant. [I] It should learn the **wrong** invariant for R2, because every run reads the EarlyInit value. That is a useful contrast for reference-based contracts.

### X1-m1 — MINOR — The INIT definition excludes EarlyInit

- **Problem.** INIT is the state "after static initializers, EarlyInit memsets and EarlyInit stores on the startup thread before its first spawn" (SPEC:910). [F] The PSP main thread spawns OSAL/PSP threads **before** `CFE_ES_Main`: `OS_API_Init` at `psp/fsw/pc-linux/src/cfe_psp_start.c:352` (console create, `osal/src/os/shared/src/osapi-printf.c:110`), and `CFE_PSP_ModuleInit` at `:391`, which creates the timebase (`psp/fsw/modules/soft_timebase/cfe_psp_soft_timebase.c:53`). Literally read, INIT therefore contains no EarlyInit store, yet A.1 relies on SB's EarlyInit clear (SPEC:2224).
- **Fix.** Make INIT the static-initializer image only, and treat EarlyInit stores as ordinary W/D entries ordered by MB/PWB. Or define INIT at a point p such that every actor spawned before p is proved not to write the support locations.

### X1-m2 — MINOR — Fixpoint base cases, and recomputing W_o

- **Problem.** Only the initial actor's entry starts at ∅ (SPEC:935, SPEC:948). Actors with no in-model spawn site get ⊤ from the empty meet of T-SPAWN, which is unsound: environment roots and signal-handler contexts (SPEC:121–122). The pseudo-code (SPEC:953) does not say that W_o's PWB pruning is recomputed with the current MB on every iteration. If W_o is computed once from ⊤, every writer is pruned and o is wrongly marked unreachable. W_o must also range over all context-qualified nodes of each writer site.
- **Fix.** Set MB(entry) := ∅ for environment roots and asynchronous contexts. Recompute W_o per iteration (the operator stays monotone). Make the node set of W_o explicit.

### X1-m3 — MINOR — The class axis mixes cause with outcome; conditional verdicts and refinement bounds cannot be represented

- **Problem.**
  - `no-provider` and `capability-absent` overlap with protected/tolerated/fixed-order: a no-provider use with `returned_and_tested` is tolerated.
  - The census emits no-provider and fixed-order without REACH (SPEC:1011–1012), although the definitions need `E_i ≠ ∅` (SPEC:329–331).
  - "Fixed-order if reached" (SPEC:976) has no field in `kz_verdict` (SPEC:497–512).
  - The refinement bound is not stored separately from the class bound.
  - Transfer rule 1 (SPEC:281) is not qualified by b.
- **Fix.** Add a separate provider axis {present, none, env-absent}, a `reach` flag {proved, assumed}, a bound per refinement, and "same b" in transfer rule 1.

### X1-m4 — MINOR — O4 is vacuous in the outcome-decided case; O6 is ill-posed in one direction

- **Problem.** When `L∥ ≠ ∅`, the order of the LPs is decided from the outcome code (SPEC:1044), and O4 then checks the code against that state (SPEC:614). This cannot fail. O6 requires an explorer witness to be admitted by Pred(π) (SPEC:616), but witnesses on another control path are outside Pred(π) by construction. That produces spurious `unknown(model_gap)` results.
- **Fix.** Report O4's power only on uses whose order HB decides. Restrict O6's second half to witnesses that keep every observed outcome class in the cone.

### X1-m5 — MINOR — σ_R1 lacks a delay semantics; cone slicing is unsound under priorities

- **Problem.**
  - σ_R1 inherits σ_U's delay-as-yield (SPEC:272). Under fixed priority, a yield by the highest-priority ready task is a no-op, so RT orderings that rely on delays are not modelled. That covers C3 at 6.4.1 and the H-V test (SPEC:1527).
  - Cones drop non-interfering actors (SPEC:1089). Under σ_R1, higher-priority actors interfere through the CPU.
  - Prediction can add `C_σ` to a σ_E prefix (SPEC:1074) whose order is not σ_R1-feasible.
- **Fix.** Give delays a blocking semantics with a discrete-time wake order derived from reference constants, or restrict σ_R1 verdicts to delay-free cones. Keep every actor with priority at least the minimum cone priority. Never combine `C_σ` with an observed σ_E prefix.

### X1-m6 — MINOR — Literature accuracy

- **Problem.**
  - σ_U∞ cites fair stateless model checking for **weak** fairness (SPEC:271). R05 describes a **strongly** fair scheduler (`05_lit_systematic_static.md:111`).
  - Erratum row 8 says "only SyncP is sound and complete for its (sync-preserving) class" (SPEC:104). R04 records SHB as "identifies exactly the schedulable races" (`04_lit_dynamic.md:234`) and M2 as complete for two-thread traces (`:242`).
- **Fix.** Cite strong fairness, or justify weak fairness by predicate monotonicity as an obligation. Reword the erratum.

### X1-m7 — MINOR — The skeleton complexity claim hides the blow-up from call-string inlining

- **Problem.** Context-qualified templates inline full call strings (SPEC:207, ADR-012). "Linear in CFG size after inlining" (SPEC:1206) hides a worst case that is exponential in call depth.
- **Fix.** State the worst case. Define a fallback (per-(actor, function) summaries) and say explicitly which certificate rules lose precision under it. Measure at S3.

### X1-m8 — MINOR — The loop's fixed-point test and the termination argument

- **Problem.** The loop stops when `census_i = census_{i-1}` (SPEC:1188). The verdict record carries `observed_n/N` (SPEC:510), and plan hashes change between iterations, so the census changes on every iteration and the loop never reaches a fixed point. Rule suspensions within one Φ version are non-monotone (SPEC:1192), so "evidence only accumulates" is false.
- **Fix.** Exclude counts and pool keys from the fixed-point test. Define the lattice order explicitly. Make a suspension end the loop for that Φ version.

### X1-m9 — MINOR — K-PHASE is not a contract instance in the §1.6 sense

- **Problem.** K-PHASE is a per-app program-order property over "resources consumed by another app after a barrier" (SPEC:725, SPEC:821). There is no use, selector or LP, so the class/σ machinery does not apply as defined.
- **Fix.** Lift it to consumer-side instances (κ_phase, the consumer's use after the barrier, r) whose only ordering is the barrier edge. Or classify it as a lint ("belief") outside the verdict classes.

---

## 3. Claims to remove or qualify (index)

| Spec text | Why | Issue |
|---|---|---|
| "certificate (§4.5, b = ∞)"; INACTIVE "sound" | message loops cut at h | X1-C1 |
| G2 "also holds under σ_E" (no M) | M_api is not an over-approximation | X1-C2 |
| A.1 "No SB-specific contract exists" (as the full story) | relies on the unspecified stamp → resource step and on the hand-declared R19 predicate | X1-M1, X1-M8 |
| A.1 "EVS … by the same argument" | needs key or once facts that are not listed | X1-M3 |
| `predicted` = "trace-sound" | sparse traces | X1-M4 |
| A.4 "timeout-dependent" for C1 | refinement undefined for exhausted retries | X1-M5 |
| A.7 / S3 "timebase lookups protected by certificate" | teardown disablers | X1-M6 |
| "inactive in the default B" for command-only paths | no message provenance | X1-M7 |
| G5 "conditional on the differential test"; "one execution per class" | a test is not a proof; optimality not provided | X1-M9 |
| "timing-protected in σ_E" | no-"safe" rule | X1-M11 |
| §1.11 "No HB rule covers" no-subscriber loss; lock reversal under "What Kairos adds" | InitRacer; RVPredict/GPredict | X1-M12 |

---

## 4. Novelty against the closest prior work (from R04/R05 only)

| Prior work (status) | What it already covers | What remains for Kairos (if the fixes above land) |
|---|---|---|
| DCatch (P) | framework HB with handler-scoped program order; trace → HB → triage → triggering | readiness contracts with documented Allowed outcomes; static must-order certificates for HB-ordered (fixed-order) violations; σ family |
| CAFA, DroidRacer (V-abs) | event-queue causality; no program order between handlers | the spec should adopt this fully for shapes (X1-M7) |
| InitRacer (V-abs) | late-handler registration and access-before-definition at startup; adverse execution | reference-sourced contracts; registry keys as conflict objects; certificates; lossy-queue capacity (G5 is genuinely open only for **capacity**, not for no-subscriber loss) |
| GPredict, RVPredict, MCM (V-abs) | generic properties on predicted traces; lock reversal through constraints; soundness on complete traces | explicit search without a solver is an engineering choice, not novelty; Kairos's soundness is weaker on sparse traces (X1-M4) |
| SyncP, WCP, SHB (V-abs) | tractable classes that preserve lock order | prefilters only (already so in the spec) |
| DefUse (V-abs) | def-use invariants learned from runs | reference-based contracts. [I] DefUse would learn R2's wrong definition as normal, which makes a strong contrast |
| ExceptioNULL (V-abs) | predicting use-before-init with SMT plus static lockset | must be cited; Kairos adds framework typestate, outcomes and certificates |
| Source/optimal DPOR, BPOR (V-abs; JACM P) | coverage and optimality per class; bound-aware POR | G5 must cite the theorem it relies on (X1-M9) |

Defensible contribution after the fixes [I]:
- a **σ- and M-qualified must-order certificate** that lifts an observed HB-ordered contract violation to a universal fixed-order class;
- with documented Allowed outcomes and per-conjunct LPs;
- combined with lock-reversing prediction and held confirmation;
- around the cFS semantics G1–G8.

Every individual mechanism has prior art. The G3 formalization still needs the literature search that R04 says was not done.

# D0 — Lead's proposal: flip-differential detection at the framework boundary (FDF), revision 2

Status: a design by the lead. Revision 2 was written after reading the grounding dossiers (g_cases_c1c2.md,
g_cases_c3r2.md, g_cases_c5.md, g_fw_startup.md, g_fw_objects.md); code facts below cite a dossier's key facts
and are to be checked there. D0 competes with D1–D3 on equal terms.

## 1. The move that removes the human oracle

The classic definition of an order violation needs a "desired order" (the programmer's intent). That is exactly
what a person had to write. FDF does not infer the desired order. It replaces it by three properties a machine
can observe:

- P1 dependence: two operations on the same framework-mediated object, a publish w by one actor and a consume u
  by another actor, such that the outcome of u (its status class, the identity it yields, or the arguments it
  then passes to the next framework call) differs between the two orders.
- P2 no enforcement: the system does not force w before u, or it forces u before w ("fixed-order premature use").
- P3 unhealed harm asymmetry: in the order "u before w" the execution shows a failure in the framework's OWN
  vocabulary that the other order does not show, or its framework-observable steady state differs, and the
  difference does not disappear by the system's own later behaviour (retry, re-read, periodic resend).

A triple (w, u, object) with P1, P2, P3 is reported as an order violation; P1 and P2 without P3 as a tolerated
dependence. Nobody states the required order: the failing, non-healing side identifies it. If the two orders
differ and neither side shows a failure-vocabulary signal, the report says "order-dependent outcome, unoriented".

Legitimacy. The dossiers found NO document sentence that forbids assuming a start order (g_fw_startup, key fact
"No document sentence found"). So FDF does not lean on a documented rule. It leans on three code facts: the
framework leaves the order open (script applications are created one after the other with no wait; core tasks in
6.4.1 too); the developers already labelled outcomes per call (status severity bits, OSAL sign rule, event type,
run status); and late start by command goes through the same creation function as the script, so "X initializes
after the others" is a schedule the framework itself offers.

The vocabulary is used DIFFERENTIALLY, never absolutely. An error-class status is not a failure by itself: BP's own
lookup-then-create returns -17 on every first boot, CFE_SB_TIME_OUT and CFE_SB_NO_MESSAGE carry error severity,
OS_QUEUE_EMPTY and OS_SEM_TIMEOUT are negative (g_cases_c1c2 negative controls; g_fw_startup failure vocabulary).
Only a signal present under one order and absent under the other counts.

## 2. Objects, events, identities (all generic)

- Objects: (L1) entities the framework manages and that are addressed by a key at an API boundary: named OSAL
  objects, ES application/library/task records, CDS blocks, SB pipes and routes, EVS registrations, TBL entries.
  (L2) framework or library fields with a placeholder value and one later non-constant store by a task's init
  ("placeholder then publish"), read through API calls that run in OTHER tasks. Example of the L2 class from the
  dossiers: the SB service's own application ID, stored only in SB's AppInit and loaded unsynchronized by SB API
  event calls that run in the caller's task (g_cases_c3r2).
- Events are recorded at EVERY public framework API entry and exit, including calls that one framework service
  makes to another inside a caller's task: (task, API, key arguments, returned status, out values). This matters:
  in R2 every application-facing return stays CFE_SUCCESS and the only error status (0xc2000003) is returned by
  the nested EVS call and discarded (g_cases_c3r2, "R2 yield"). Keys are read at run time, so a key that comes
  from a configuration table (C1's semaphore name) is resolved without static analysis (g_cases_c1c2).
- API roles (publish / consume, key argument, the status that means "absent") come from the framework source per
  version. g_fw_objects confirms the shape: the v7.0.1 OSAL has one object-table layer through which every named
  create and every by-name lookup passes (allocate-new / finalize-new / find-by-name); cFE has such a layer only
  for ID allocation, and by-name lookups are one function per class; the 6.4.x generation has no generic layer.
  Roles are derivable mechanically for OSAL and for the ES/TBL by-name APIs (the step 2a facts already hold the
  rows), but no class/role table exists yet: it is a framework-level artifact to generate once per version.
  Not every absence has a status: an SB send with no subscriber returns CFE_SUCCESS (an INFORMATION event and a
  counter only), and an absent SB pipe name returns the same status as a bad argument. For those classes the
  oracle can only use the difference between the two runs, not a status.
- Actor identity comes from the creation chain observed at the boundary (the creator's create call, its output
  ID, the entry point, and the child's first event), never from self-reported IDs: in C5's boots a thread adopted
  another task's slot through a reused thread handle and received another application's ID with CFE_SUCCESS
  (g_cases_c5, identity problem). Numeric IDs are renamed to stable names before any comparison.
- Enforced order: task creation, successful blocking waits (per call outcome), message delivery, and the
  framework's startup serialization as it is in the tested version (core tasks one at a time with a wait in
  v7.0.1 and 6.4.2, none in 6.4.1; library init synchronous; script applications concurrent: g_fw_startup).
  Mutual exclusion alone orders nothing: C5's writer and reader hold the same mutex in different sections.

## 3. Flip classes (the only schedule perturbations; no case knowledge)

- F1 pair flip: for a cross-actor pair (w, u) on the same key that is not ordered, hold the publisher at the
  ENTRY of w until u has returned, then release at once (minimal flip). The control run enforces w before u the
  same way. Holds sit at API entry because a lookup that falls inside a creator's reserved window returns another
  status and can leave a class lock held (g_cases_c1c2, hold hazard).
- F2 spawn flip: at every task-creation call, "child first" (hold the creator at the return of the create call
  until the child has completed its first framework call or blocks) and "creator first" (hold the child at entry).
  This reaches spawn-before-publish windows without knowing the record (C5: unlock, create, re-lock, register).
- F3 application-level sweep: for each application X of the startup script, one run in which X is started after
  the others (held at entry, or started by the ES start command). n runs give both orders of every pair of
  applications at whole-init granularity. Aligning each other task's event stream with the baseline localizes u
  (first differing event) and its key identifies w.
- F4 (L2 fields): hold the publishing store or the reading call for fields found by the static facts.
- No flip (fixed order): in ONE natural run, a consume that returns an error-class status or reads the placeholder,
  followed later in the same run by the publish of the same object by another actor, with the same call sites
  succeeding after the publish and the early effect never redone. R2 is this class: the order is forced by
  construction, 24 early calls return 0xc2000003, later calls from the same sites succeed (g_cases_c3r2).

Every hold has a recorded outcome: realized; path not reached; consumer blocked on a lock or wait owned by the
held task (the order is enforced: not a candidate; this is what 6.4.2's single critical section produces for C5);
a framework timeout fired because of the hold (artifact: discarded and reported; the soft 1000 ms startup timeouts
and the 30000 ms core wait are the known ones, g_fw_startup); system did not quiesce.

## 4. The oracle

For a realized flip, compare the flipped run with the control run on:
1. the outcome of u: status class, renamed identity, and the arguments of the framework calls that follow from it;
2. the failure-vocabulary set: error-class statuses returned at any boundary (including nested calls), error and
   critical events, application run status at exit, keyed by (application, call site or event ID);
3. steady-state facts with stable identities: application -> final state; (class, name, owner) of registered
   objects; routes; tables; EVS registrations.
Benign nondeterminism is not hand-masked: components that differ between repeated control runs are excluded and
listed. The number of repetitions and the observation horizon are parameters to pre-register. The healing horizon
comes from the code where it is derivable (C1's fix retries 25 times, 100 ms apart: application constants).
"OPERATIONAL" is not a quiescence proof (it is reached on a soft timeout), so the end of observation needs its
own definition from framework facts. [open: g_fw_startup section on quiescence]

Verdicts: V1 order violation (P1, P2, P3; two concrete runs); V2 tolerated dependence (u's outcome flips, the
system converges; report the retry bound); V3 fixed-order premature use (single-run evidence); V3s premature read
without any failure signal (placeholder read, later publish, no status: reported as unoriented); V4 no
dependence; V5 unresolved (with the hold outcome). Each report carries the two code sites, the object and key,
the two event streams around the divergence, and the margin (time between w and u in the natural run).

## 5. What static analysis is for (not the oracle)

- generate the boundary instrumentation and the L2 field set;
- drop pairs ordered by the framework's startup order before running;
- classify the failing branch of each consume (exit, error event, retry with bound, create on miss, ignored) to
  rank flips, to derive the healing horizon, and to explain a verdict;
- list consume sites whose publisher is in another application but which no run exercised (coverage gaps).
The step 2a facts and relation rows become the ranking and explanation layer: a row turns into a run, not into a
reading task for a person.

## 6. Expected behaviour on the development set (from the mechanics cards; nothing was run)

- C1 before the fix: the natural order already has the lookup first (CF is line 6, BP line 7 of the script):
  -17, ERROR event 30, exit with run status APP_ERROR, CF deleted, never healed. F1 control (create first) gives
  0: V1. After the fix: the lookup fails once, a retry succeeds: V2 with bound 25 x 100 ms; with the publisher
  held beyond the bound the recorded outcome was -17 x 25 and exit, which FDF reports as "tolerated up to the
  code's bound", not as a new violation.
- Look-alikes: BP's lookup-then-create returns -17 in both orders and converges: V4. SCH_LAB's time-base lookup
  has its publisher in PSP module init before ES main: ordered, not a candidate. TIME tolerates the miss: V2 or V4.
- R2: V3 from one natural run (section 3, "no flip").
- C3 (6.4.1, core tasks not serialized): F2/F4 flips among core tasks. Sub-orders differ (g_cases_c3r2):
  SB between its two publications gives 0xc2000002, ERROR event 41 and a syslog line (V1 by vocabulary); EVS
  registered but SB unpublished gives CFE_SUCCESS with the SB event attributed to application 0 (caught only by
  item 1 of the oracle, the argument of the nested call: unoriented divergence); the out-of-bounds read needs a
  second unpublished object and has no status (missed unless it crashes). In 6.4.2 the barrier makes the flips
  unrealizable; the remaining silent fixed-order use is V3s.
- C5 (6.4.1): F2 "child first" at the create call inside application creation: the child's CFE_ES_GetAppID
  returns CFE_ES_ERR_APPID (0xc4000001), EVS registration is skipped, the pipe owner is wrong, CFE_ES_RunLoop
  returns FALSE; the lookup itself heals but these are one-shot losses: V1. In 6.4.2 the child blocks on the ES
  mutex held by the held creator: enforced, not a candidate. No recorded run contains the violating order.
- C2: an enhancement request with no concrete pair in any code on disk (g_cases_c1c2). FDF has nothing to detect
  there; it can only list applications that declare themselves ready and then continue initializing.

## 7. Known weak points

- Coverage: only executed startup paths of the configured applications.
- A hold changes timing; framework timeouts fired by a hold must be detected and discarded.
- Differences confined to private memory that never reach a framework boundary are invisible (C3's out-of-bounds
  access unless it faults).
- "Unoriented" reports (V3s, silent misattribution) still need a reader; their number per system is unknown.
- Schedule feasibility under a given scheduling policy is assumed (a task may be delayed at any point); the
  margin is reported so a reader can judge.
- Cost is counted in boots: controls plus one per flip (F3 is a cheap first sweep).
- Violations needing more than one flipped pair, and atomicity violations, are out of scope.
- The repetition count, the observation horizon and the definition of quiescence need a reference or a PI decision.

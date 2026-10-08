# CORD: detecting task-level order violations in cFS (design, v0)

Status: design draft, 2026-10-07. Nothing here is implemented or measured yet.
Labels: **[fact]** checked in a source; **[design]** proposal of this project; **[hypothesis]** to be tested.

## 1. Problem

The five cases (C1 CF#184, C2 cFE#198, C3 cFE#73, C5 cFE#72, R2 cFE#2663) share one shape
(**[hypothesis]**, to be confirmed per case in `cases/ledger.md`):

- a task *publishes* a framework-managed resource: a named OSAL object becomes visible
  (C1), an ES app/task record becomes valid (C5), a core service sets its AppId or
  registration (C3, R2), an app finishes its late initialization (C2);
- another task *uses* the resource: name lookup, `GetAppID`, an SB/EVS call that reads the
  AppId, a service request;
- nothing that the framework provides forces the publication to happen before the use, so
  some schedule runs the use first.

These resources live in user space: OSAL ID tables, cFE global data, app state.
**[hypothesis]** Two consequences, to be checked in `baselines/`:

1. Syscall-level process-race tools such as RacePro model kernel objects. OSAL creates and
   looks up named objects in user-space tables over pthreads, so the name never reaches a
   syscall. The resource is invisible to that model.
2. Thread-level data-race detectors (TSan) see the memory accesses. But the publication and
   the lookup are both inside the same lock (OSAL ID map lock, ES shared-data lock), so
   there is no data race. The bug is an *order* violation between correctly locked critical
   sections.

## 2. Events **[design]**

Every event records: global sequence number, task token (OSAL task id and pthread id),
owning app (when known), source site (file:line, function), event kind, resource key,
observed value or status.

| Kind | Emitted at | Key |
| --- | --- | --- |
| `PUB(k)` | the point where resource k becomes usable by other tasks (the commit inside the critical section, not the function return) | resource key |
| `USE(k, status)` | the point where a task depends on k (lookup, AppId read, request entry), with the result | resource key |
| `TCREATE(t)` / `TSTART(t)` | parent before the create call; child at its entry | child task |
| `SIG(o)` / `WAIT(o)` | give/post and successful take of semaphores, queue send/receive, SB transmit/receive of a message instance | sync object and message instance |
| `SETSTATE(v, x)` / `WAITRET(v, x)` | writes of designated state variables (ES system state, app run status) and the read that ends a wait loop on them | state variable |
| `LOCK(l)` / `UNLOCK(l)` | OSAL mutexes and cFE internal locks | lock |

Resource keys are normalized so that the publisher and the user name the same thing:
OSAL objects by type and name (`countsem:"CF_THROTTLE"`) and by ID after creation; ES records
by AppID and TaskID, with the TaskID→AppID link; core service AppIds by service name; app
readiness by an explicit annotation `CORD_PUBLISH("APP.ready")` / `CORD_USE(...)`.

## 3. Ordering model **[design]**

Happens-before `→` is the transitive closure of:

1. program order within a task;
2. `TCREATE(t) → TSTART(t)`;
3. `SIG(o) → WAIT(o)` for the matching signal (semaphore post consumed by that take, message
   instance sent then received);
4. `SETSTATE(v, x) → WAITRET(v, y)` when the wait returned because it read the value
   written by that `SETSTATE` (reads-from on designated state variables only).

Lock release→acquire is **not** an ordering edge. A lock makes the two critical sections
atomic. It does not decide which one runs first. This is the usual choice in predictive
race analysis, and it is what lets a single benign run expose the reversed order.

## 4. Detection **[design]**

- **Observed violation:** `USE(k)` occurs before any `PUB(k)` in the run, or `USE` returns
  a not-found / invalid status for k that a later `PUB(k)` makes valid.
- **Predicted violation:** `PUB(k)` occurs before `USE(k)` in the run, but `PUB(k) → USE(k)`
  does not hold. Under the model, some feasible schedule runs the use first.
- Each report carries the two sites, the tasks, and the HB path that is missing. The
  ordering that does exist is reported too (for example "ordered only by startup
  priority", "ordered only by a sleep").

## 5. Witness and outcome **[design]**

For each predicted violation, CORD reruns with a delay injected just before `PUB(k)` in
the publishing task. A delay is always a feasible schedule under a preemptive scheduler,
so it cannot create an execution the system could not have. The witness run checks that
`USE(k)` now precedes `PUB(k)` and classifies the outcome:

- `harmful`: an error status propagates (init failure, app exit, event error), or a crash
  or invalid access happens;
- `recovered`: the user retries or waits and later succeeds;
- `benign`: the use tolerates the missing resource;
- `infeasible-here`: the order cannot be produced even with the delay. This means some
  ordering edge was not modeled, so it is reported as a model gap.

## 6. Evaluation plan **[design]**

- Cases: buggy and fixed revisions of C1, C3, C5, R2; C2 as a role-based derived app
  (its provider app is private).
- Baselines on the same inputs:
  1. stress testing (repeat runs, count failures);
  2. random delay injection at the same instrumentation points;
  3. TSan on the same build;
  4. a syscall-level model in the style of RacePro: what `strace -f` shows about the
     involved resources;
  5. the original tools, where they can run (see `baselines/availability.md`).
- Metrics:
  - detection from one benign run;
  - runs needed to the first harmful witness;
  - false candidates after triage;
  - instrumentation coverage and overhead;
  - new candidates in the current cFS bundle apps, confirmed by witnesses.
- Ablations: remove the resource-identity linking, the state reads-from edges, or the
  witness step; or treat locks as ordering edges.

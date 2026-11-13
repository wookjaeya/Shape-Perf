# 04: Dynamic race and order-violation detection — a literature survey for a cFS process-level detector

- Date: 2026-10-12. This is investigation only. No cFS tree was modified, nothing was built or run, and nothing was committed.
- Pinned tree for code citations: `/home/user/work/procrace/cfs_ref/cFS` (nasa/cFS v7.0.1; cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`). Paths in citations are relative to that tree.
- Research records used: `research/cfs_procrace/cases/ledger.md`, `results/R2_gdb_v701.md`, `baselines/availability.md`, `env/CONDITIONS.md`, `env/BASELINE_MEASURE.md`, and the user's design proposal v0.1 (`39d4fa4a-…20261008.md`).

## 0. Method, labels and limits

### 0.1 How the citations were checked

| Channel | Result |
|---|---|
| `WebSearch` | Worked. Each paper below was checked for authors, title, venue and year against at least one publisher, author, DBLP or institutional page. The abstract-level claims come from the same results. |
| `mcp__Scholar_Gateway__semanticSearch` | **Denied** (`ACCESS_DENIED`) on every call. |
| Primary PDFs (`curl`, `WebFetch`) | **Blocked by this session's egress policy.** Blocked hosts: arxiv.org, www.cs.columbia.edu, web.eecs.umich.edu, dl.acm.org, usenix.org, people.cs.uchicago.edu, go.dev, docs.kernel.org, pubs.opengroup.org and others. The proxy returned 403 `connect_rejected`. I did not try to get around this. |
| raw.githubusercontent.com | Worked for two primary documents: the Linux KCSAN design doc (`torvalds/linux` master `Documentation/dev-tools/kcsan.rst`, 377 lines, sha256 `6fdf5914…`) and the Go memory model (`golang/go` master `doc/go_mem.html`, "Version of June 6, 2022", sha256 `a7372b4f…`). Both were read in full. Local copies are in `scratchpad/arch/probe_lit04/docs/`. |

### 0.2 What the verification level means

Every reference in §9 carries one of these tags:
- **V-doc**: I read the primary text myself.
- **V-abs**: I verified the bibliographic data and the claims in the abstract or summary through search results that quote the publisher or author page.
- **P**: partly verified. The details that were not verified are named.
- **UNVERIFIED**: not checked. Never relied on for a conclusion.

**Consequence.** No full paper body could be opened. Statements about each paper's ordering rules beyond its abstract therefore come from search excerpts of the paper or its slides, and they say so. The exact rule tables of DroidRacer, CAFA, EventRacer and DCatch are **not** reproduced here; only their general shape, as the excerpts give it.

### 0.3 Labels in the text

- **FACT (paper)**: the paper or its abstract says it (V-abs or V-doc).
- **FACT (code)**: the pinned cFS code says it (`path:line`).
- **FACT (record)**: one of our research records says it.
- **INFERENCE**: my reasoning. It has not been executed or measured.

### 0.4 Corrections to the task's citation list

These are the places where the task text and the verified record disagree.

| Task text | Verified record |
|---|---|
| "EventRacer PLDI 2013" | **OOPSLA 2013**, pp. 151–166 (Raychev, Vechev, Sridharan). The PLDI paper is WebRacer's: Petrov et al., PLDI 2012. |
| "CloudRaid EuroSys 2019" | **ESEC/FSE 2018**, pp. 3–14 (Jie Lu, Feng Li, Lian Li, Xiaobing Feng; DOI 10.1145/3236024.3236071). The EuroSys 2019 paper in that group's area is FlyMC (Lukman et al.). |
| FCatch authors | The ASPLOS 2018 byline is Haopeng Liu, Xu Wang, Guangpu Li, Shan Lu, Feng Ye, Chen Tian. Lukman and Gunawi are not on it. DCatch (ASPLOS 2017) does include Lukman and Gunawi. |
| SHB authors | Mathur, **Kini**, Viswanathan, OOPSLA 2018 (PACMPL 2, Article 145). This matches the task. |
| Proposal v0.1 [R6] "RacePro: A Tool for Testing and Diagnosing Process Races in Deployed Systems" | The SOSP 2011 title is **"Pervasive Detection of Process Races in Deployed Systems"** (Laadan, Viennot, Tsai, Blinn, Yang, Nieh). |

### 0.5 Terminology warning: "sound" and "complete"

Different communities use these words in opposite ways. Each paper's own guarantee is quoted below in its own terms.
- **Predictive-analysis papers (PL community).** "Sound" means every reported race is real, i.e. no false positives (CP, WCP, SHB, M2, SyncP, Vindicator).
- **KCSAN doc (V-doc), "Key Properties" item 6.** It uses the opposite convention: "the analysis is *unsound* (false negatives possible), but aims to be complete (no false positives)" (`kcsan.rst` L356-358).
- **This report** says "no false alarms (NFA)" and "no missed races (NMR)", each relative to a stated model and an observed trace.

Netzer and Miller (LOPLAS 1992, V-abs) separate **general races**, which apply to programs meant to be deterministic, from **data races**, which apply to nondeterministic programs with critical sections. They also separate feasible, apparent and actual races.
- cFS readiness violations are general races or order violations in this sense, not necessarily data races (INFERENCE).
- That is the reason this survey covers more than data-race detectors.

---

## 1. The cFS concurrency substrate a dynamic detector must observe

This section gives only what the survey needs. Reports 01 and 02 in this folder cover the architecture in depth. Everything here is FACT (code) unless marked.

| Mechanism | Code | Why it matters for dynamic detection |
|---|---|---|
| Apps and core services are OSAL tasks, i.e. threads of one process. In the ENV build all 27 threads are `SCHED_OTHER` with nice 0, so requested priorities have no effect. | FACT (record): `env/CONDITIONS.md` E4, M1-5 | Thread-level tools apply directly. Priorities cannot be treated as ordering (INFERENCE). |
| **SB send.** Under the SB lock, SB finds destinations and pre-counts the queue depth (`cfe/modules/sb/fsw/src/cfe_sb_priv.c:1058-1074`). It then calls `OS_QueuePut` of a buffer-descriptor pointer (`:1177-1178`). | | |
| POSIX `OS_QueuePut` calls `mq_timedsend` with a **zero timeout** and constant priority 1. A full queue returns `OS_QUEUE_FULL` (`osal/src/os/posix/src/os-impl-queues.c:300-315`). SB turns that into a drop plus `CFE_SB_Q_FULL_ERR_EID` (`cfe_sb_priv.c:1188-1193`). | | The send never blocks, and losses are an order-dependent outcome. A capacity-based edge, as in a blocking channel, does not exist (§4 row 6). |
| **No subscriber.** SB drops the message, counts it, raises `CFE_SB_SEND_NO_SUBS_EID` and returns **success** (`cfe_sb_priv.c:1082-1087`). | | A subscribe that comes late becomes silent loss. This is the analogue of InitRacer's "late event handler registration" (§2.7). |
| **SB receive**: `OS_QueueGet` → `mq_receive` / `mq_timedreceive` (`os-impl-queues.c:209,238`). | | This is the receive endpoint of a message edge. |
| **ES system state** is `volatile sig_atomic_t SystemState` (`cfe/modules/es/fsw/src/cfe_es_global.h:151`). | | |
| ES main writes it **without a lock** at `cfe_es_start.c:87,183,194,216,234`. `CFE_ES_WaitForSystemState` polls it **without a lock** with `OS_TaskDelay` and a timeout (`cfe_es_api.c:585-603`). The ES lock is held only for that function's earlier app-state update (`:528-576`). | | This is ad hoc synchronization (§2.6). |
| `CFE_ES_WaitForStartupSync` discards the timeout status: it returns `void` (`cfe_es_api.c:616-619`). | | The barrier edge is conditional on not timing out. |
| `CFE_ES_MainTaskSyncDelay` polls app states under the ES lock (`cfe_es_start.c:860-900`). | | |
| Core apps are started one at a time; each must reach RUNNING (`cfe_es_start.c:835`). All five core tasks call `CFE_ES_WaitForSystemState(CORE_READY)` after their init (`cfe_es_task.c:134`, `cfe_evs_task.c:229`, `cfe_sb_task.c:93`, `cfe_time_task.c:88`, `cfe_tbl_task.c:74`; all under `cfe/modules/*/fsw/src/`). That call **also publishes the caller's own app state**, raising it to the required state (RUNNING for CORE_READY) under the ES lock before polling (`cfe_es_api.c:528-576`, comment at `:566-569`). | | One call is both a "signal" (my state) and a "wait" (system state). An HB model must split it into a release part and a conditional acquire part (INFERENCE). |
| **Named-object registries.** OSAL `OS_ObjectIdFindByName` (`osal/src/os/shared/src/osapi-idmap.c:975`). A name becomes valid at `OS_ObjectIdFinalizeNew` (`:838-870`). | | Create-versus-lookup by name is a key-based def-use relation on a user-space table. |
| Other name lookups: `CFE_ES_GetAppIDByName` (`cfe_es_api.c:627`), `CFE_TBL_Share` by name (`cfe/modules/tbl/fsw/src/cfe_tbl_api.c:184`), and the SB pipe-name lookup `OS_QueueGetIdByName` (`cfe_sb_api.c:802`). | | |
| CF's own source states the race and handles it with a bounded retry: "There is a start up race condition because CFE starts all apps at the same time…" (`apps/cf/fsw/src/cf_cfdp.c:1272-1290`). | | Lookup failure can be a contract-permitted outcome, not a bug. |
| **R2 pair.** SB stores its AppId at `cfe_sb_task.c:138` and registers with EVS at `:155`. EVS rejects an unknown AppId with `CFE_EVS_APP_ILLEGAL_APP_ID` (`cfe/modules/evs/fsw/src/cfe_evs.c:185-187`). | | |
| **Tables**: `CFE_TBL_GetAddress` / `ReleaseAddress` (`cfe_tbl_api.c:439,478`; `LockFlag` cleared at `:489`) and `CFE_TBL_Manage` (`:731`). | | Reader-region protocol (§4 row 15). |
| **Child tasks**: `CFE_ES_CreateChildTask` (`cfe_es_api.c:1202`). | | Fork edge. |

**FACT (record), observed behavior:**
- **R2.** In every gdb run, 24 SB events came from the ES task (20) and the EVS task (4) with `AppID = 0` before SB published its AppId. EVS rejected all of them (`results/R2_gdb_v701.md` §2).
  - All violating uses were hits 1–47, and the publication was hit 50.
  - The order comes from S1 serialization plus the core object order. The record labels this an inference.
- **Queue-full drops** at startup: 4–49 per run (`env/CONDITIONS.md` M1-3).

---

## 2. Survey by family

Each entry has the same fields:
- **Idea**
- **Order model**
- **Guarantee** (the paper's own terms)
- **Cost** (as reported)
- **cFS: sees / misses**
- **Borrow**

The cFS fields are INFERENCE unless marked.

### 2.1 Foundations

**Lamport, "Time, Clocks, and the Ordering of Events in a Distributed System", CACM 21(7):558–565, 1978. [V-abs]**
- **Idea.** "Happened before" is a partial order on events: program order within a process, plus send→receive of messages, closed under transitivity. Logical clocks are consistent with it. The paper also treats physical clocks and bounds their skew.
- **Order model.** Program order (PO) ∪ message edges, transitively closed.
- **cFS.**
  - SB send→receive is a direct instance (§4 row 6).
  - Physical timestamps from per-task recorders give no HB edge. The user's proposal says this too (v0.1, around L431).
- **Borrow.** HB is the backbone. Wall-clock order is only evidence for a *schedule*, never an *ordering guarantee*.

**Fidge, "Timestamps in Message-Passing Systems That Preserve the Partial Ordering", 11th Australian Computer Science Conference, 1988; Mattern, "Virtual Time and Global States of Distributed Systems", Workshop on Parallel and Distributed Algorithms, 1989 (Elsevier, pp. 215–226). [V-abs; the page range of Fidge is given as 55–66 or 56–66 by different sources]**
- **Idea.** Vector clocks characterize HB exactly: a ≺ b iff VC(a) < VC(b).
- **Borrow.** VC for the HB engine. Startup traces are small (hundreds of high-level events per boot in R2), so FastTrack-style compression is optional (INFERENCE).

**Netzer and Miller, "What Are Race Conditions? Some Issues and Formalizations", ACM LOPLAS 1(1):74–88, 1992. [V-abs]**
- **Idea.**
  - General races (where the access order is not guaranteed) are distinct from data races (non-atomic conflicting accesses).
  - Feasible races are distinct from apparent races. An apparent race may be ruled out by implicit synchronization.
  - Locating all races is NP-hard for synchronization that can implement mutual exclusion. The NP-hardness statement comes from Netzer's thesis, as summarized in the search.
- **cFS.** The "required order" of the five cases is a general-race or order notion. An apparent race on `SystemState` polling is exactly the "implicit synchronization" case.
- **Borrow.** The vocabulary of the verdict classes: feasible versus apparent, and general versus data.

### 2.2 Happens-before (vector-clock) detectors

**Djit+: Pozniansky and Schuster, "Efficient On-the-Fly Data Race Detection in Multithreaded C++ Programs", PPoPP 2003. [P: venue and title verified through citing reference lists; the paper body was not seen. "Djit+" is attributed to it by FastTrack's abstract.]**
- **Idea.** A precise VC-based HB detector with per-location read and write vector clocks. The details are UNVERIFIED here.
- **Guarantee.** Precise, i.e. no false alarms relative to observed HB. This is FastTrack's characterization of VC detectors.
- **cFS: sees.** Unordered conflicting memory accesses between tasks, once lock and queue edges are modeled.
- **cFS: misses.**
  - HB-ordered wrong-direction uses (R2).
  - Schedule-dependent pairs hidden behind a lock acquired in the observed order (C1: both the OSAL create and the lookup take the OSAL table lock). See §3 L5.

**FastTrack: Flanagan and Freund, "FastTrack: Efficient and Precise Dynamic Race Detection", PLDI 2009; CACM 53(11):93–101, 2010. [V-abs]**
- **Idea.** "Epochs" (clock@thread) replace VCs for writes and for most reads. Reads switch adaptively to VCs when read-shared. Most operations take O(1).
- **Guarantee.** "Precise": no false alarms, the same precision as VC detectors.
- **Cost.** About an order of magnitude faster than a traditional VC detector, and about 2× faster than DJIT+ (abstract; CACM text says 2.3×).
- **cFS.** As Djit+. Cost is irrelevant at startup scale, and very relevant if memory accesses are instrumented across all apps.
- **Borrow.** The epoch optimization only if the tool instruments memory. The core contribution, HB precision with no false alarms, is the baseline semantics.

### 2.3 Lockset and hybrid detectors

**Eraser: Savage, Burrows, Nelson, Sobalvarro, Anderson, "Eraser: A Dynamic Data Race Detector for Multithreaded Programs", ACM TOCS 15(4):391–411, 1997 (also SOSP 1997). [V-abs]**
- **Idea.** The lockset discipline: each shared variable must be consistently protected by some lock. The candidate set C(v) shrinks on each access. Binary rewriting monitors every shared reference. A state machine handles the initialization and read-shared patterns, because the plain discipline is, in the paper's word, "too strict".
- **Guarantee.** Discipline checking, not HB. It reports violations of the locking discipline, including ones that cannot occur.
- **Cost.** UNVERIFIED. The commonly cited "10–30×" was not confirmed.
- **cFS: sees.** Unprotected shared fields (ES global state, OSAL tables) regardless of schedule.
- **cFS: misses.** Order violations entirely, because every access in C1 and C5 *is* locked. Message-passing and polling protocols produce false positives.
- **Borrow.** Only as a pruning filter, the way ExceptioNULL uses static lockset (§2.5). Not as a detector.

**O'Callahan and Choi, "Hybrid Dynamic Data Race Detection", PPoPP 2003. [V-abs]**
- **Idea.** Lockset and a limited HB, used together. A two-phase approach focuses instrumentation.
- **Guarantee.** In the paper's own comparison, HB reports fewer false positives than lockset "but can report more false negatives".
- **Borrow.** The principle that HB's false negatives come from *coincidental* lock order. This motivates predictive relaxations (§2.5).

**RaceTrack: Yu, Rodeheffer, Chen, "RaceTrack: Efficient Detection of Data Race Conditions via Adaptive Tracking", SOSP 2005, pp. 221–234. [V-abs]**
- **Idea.** A hybrid algorithm inside the CLR VM that adaptively tracks suspicious areas. A post-processing step correlates and ranks warnings.
- **Cost.** About 3× slowdown on memory-intensive programs, typically under 2× otherwise; memory typically under 1.2×.
- **Borrow.** Warning ranking, and adaptive focusing on suspicious objects. This maps to "focus on registries and readiness fields".

**MultiRace (Pozniansky and Schuster).** It is often attributed to the PPoPP 2003 paper above, with a journal version "MultiRace: Efficient On-the-Fly Data Race Detection in Multithreaded C++ Programs" in *Concurrency and Computation: Practice and Experience*, 2007. **UNVERIFIED**: the journal record was not found. It is not relied on.

**ThreadSanitizer v1: Serebryany and Iskhodzhanov, "ThreadSanitizer – Data Race Detection in Practice", WBIA 2009, pp. 62–71. [V-abs]**
- **Idea.** A hybrid HB and lockset algorithm on Valgrind, used on Google's C++ code. **Dynamic annotations** let users tell the detector about custom synchronization.

**ThreadSanitizer v2: Serebryany, Potapenko, Iskhodzhanov, Vyukov, "Dynamic Race Detection with LLVM Compiler", RV 2011, LNCS 7186, pp. 110–114. [V-abs]**
- **Idea.** Compile-time instrumentation. With sampling, the abstract claims slowdown "less than 1.5×".
- **Cost of current TSan.** The Clang documentation states a typical slowdown of 5×–15× and memory overhead of 5×–10× [V-abs via the search summary of clang.llvm.org; the primary page was not opened].
- **cFS: sees.**
  - Real memory races among the 27 threads, out of the box: OSAL on pthreads is intercepted.
  - Likely the `SystemState` polling pair as a race. This is INFERENCE: the writes at `cfe_es_start.c:216,234` and the read at `cfe_es_api.c:585` are plain `volatile` accesses with no lock or atomic, and C11 `volatile` is not synchronization.
- **cFS: misses.**
  - Readiness semantics.
  - HB-ordered wrong-direction uses (R2): ordered accesses are not races.
  - Whether `mq_send`/`mq_receive` create HB in TSan is **UNVERIFIED**. If they do not, TSan would report SB payload accesses as races (false positives).
- **Borrow.** The annotation idea: emit HB edges at the framework level (SB delivered-send → receive; ES state publish → successful wait). Possibly reuse TSan itself as a low-level "memory-race layer" under a cFS-semantic layer.

### 2.4 Sampling and watchpoint detectors

**LiteRace: Marino, Musuvathi, Narayanasamy, "LiteRace: Effective Sampling for Lightweight Data-Race Detection", PLDI 2009, pp. 134–143. [V-abs]**
- **Idea.** HB detection with sampled memory accesses; synchronization is always logged.
- **Guarantee and cost.** It found more than 70% of data races while sampling less than 2% of memory accesses (Phoenix-based; Microsoft programs, Apache, Firefox).

**Pacer: Bond, Coons, McKinley, "PACER: Proportional Detection of Data Races", PLDI 2010, pp. 255–268. [V-abs]**
- **Idea.** Sampled FastTrack.
- **Guarantee.** It detects any race with probability equal to the sampling rate. Overheads at 1–3% sampling are low enough to consider in production.

**DataCollider: Erickson, Musuvathi, Burckhardt, Olynyk, "Effective Data-Race Detection for the Kernel", OSDI 2010. [V-abs]**
- **Idea.**
  - It needs no knowledge of the synchronization protocol.
  - It randomly samples a small fraction of accesses, sets a hardware data breakpoint and **delays**. If another thread touches the address, a race happened.
- **Results.** Windows 7 kernel: 25 confirmed erroneous races, 12 already fixed. The authors claim negligible overhead. A third-party Java comparison reporting 96% overhead is secondary and not relied on.

**KCSAN: Linux "Kernel Concurrency Sanitizer (KCSAN)" design doc. [V-doc]**
- **Idea.**
  - Compile-time instrumentation of plain accesses plus "soft watchpoints".
  - Periodically, a plain access sets a watchpoint and stalls for a short randomized delay. Any concurrent matching access, at least one of them a write, is a race.
  - A value change across the delay means "a race of unknown origin" (L244-280).
  - It optionally simulates reordering of one plain access to model weak memory (L281-329).
  - `ASSERT_EXCLUSIVE_*` macros check user-declared exclusivity (L237-241).
- **Guarantee.**
  - "unsound (false negatives possible), but aims to be complete (no false positives)".
  - "aware of only a subset of LKMM ordering rules; this may result in missed data races" (L353-358).
  - It explicitly rejects an HB design (KTSAN) because "any omission [of ordering rules] leads to large numbers of false positives" (L366-373).
- **Cost.** 5.0× slowdown with the default configuration, 2.8× from the fast path only, on kernel boot with 8 CPUs (L338-344).

**What sampling and watchpoints mean for cFS (INFERENCE).**
- Sampling is a poor fit for the target class. Startup readiness pairs execute **once per boot**: R2 has 24 uses per boot. A sampled detector misses a once-per-boot pair with probability (1 − rate).
- The **delay-at-a-watchpoint** idea of DataCollider and KCSAN is directly useful for *confirmation*: stall one side of a candidate pair and see whether the other side lands. This is the "selective execution verification" stage.
- KCSAN's argument against KTSAN carries over. An HB model that omits even one framework ordering mechanism floods false positives. A cFS HB model must therefore be **complete over the framework's synchronization mechanisms** (§4), or it must fall back to watchpoint-style observation for anything it does not model.
- `ASSERT_EXCLUSIVE_*` is the closest established precedent for **declared contracts checked at run time**.

### 2.5 Predictive analysis (reordering an observed trace)

**Causally-Precedes (CP): Smaragdakis, Evans, Sadowski, Yi, Flanagan, "Sound Predictive Race Detection in Polynomial Time", POPL 2012. [V-abs]**
- **Idea.** CP relaxes HB's lock edges: release→acquire on the same lock orders only when the critical sections conflict, plus composition rules.
- **Guarantee.** It "generalizes happens-before to observe more races without sacrificing soundness". "Sound and of polynomial complexity".
- **Result.** On Apache FtpServer, one CP run found races that 10 HB runs did not.

**Maximal causal models: Șerbănuță, Chen, Roșu, "Maximal Causal Models for Sequentially Consistent Systems", RV 2012, LNCS 7687, pp. 136–150. [V-abs]**
- **Idea.** From one trace, take the maximal set of traces that every program able to produce the trace can also produce. Sound (every trace in the model is producible) and maximal.

**RVPredict: Huang, Meredith, Roșu, "Maximal Sound Predictive Race Detection with Control Flow Abstraction", PLDI 2014, pp. 337–348. [V-abs]**
- **Idea.** Abstracted control flow plus minimal feasibility constraints become first-order formulas, and race detection becomes constraint solving.
- **Guarantee.** "Maximal detection capability for any sound dynamic race detector working on the same input trace", under sequential consistency.
- **Scale.** "Tens of millions of critical events".

**WCP: Kini, Mathur, Viswanathan, "Dynamic Race Prediction in Linear Time", PLDI 2017. [V-abs]**
- **Idea.** Weak-causally-precedes: weaker than CP, still sound, computed in **linear time** with vector clocks.

**SHB: Mathur, Kini, Viswanathan, "What Happens-After the First Race? Enhancing the Predictive Power of Happens-Before Based Dynamic Race Detection", OOPSLA 2018 (PACMPL 2, Art. 145). [V-abs]**
- **Idea.** HB's soundness covers only the *first* race. Later HB-unordered pairs may not be schedulable.
- **Guarantee.** Schedulable HB (SHB) "identifies exactly the schedulable races", with a linear-time VC algorithm.

**DC and Vindicator: Roemer, Genç, Bond, "High-Coverage, Unbounded Sound Predictive Race Detection", PLDI 2018. [V-abs]**
- **Idea.** An unsound "doesn't-commute" (DC) analysis finds candidates. A second algorithm *vindicates* each candidate by constructing a witness. The pipeline is sound.

**Also from the same group, PARTIALLY VERIFIED (named in the search summary of the Vindicator page; not checked separately):** Genç, Roemer, Xu, Bond, "Dependence-Aware, Unbounded Sound Predictive Race Detection", OOPSLA 2019; Roemer, Genç, Bond, "SmartTrack", PLDI 2020.

**M2: Pavlogiannis, "Fast, Sound, and Effectively Complete Dynamic Race Prediction", POPL 2020. [V-abs]**
- **Guarantee.** Polynomial time and "no false positives". It provably detects all races for two-thread traces, and gives sufficient criteria for completeness with more threads.

**SyncP: Mathur, Pavlogiannis, Viswanathan, "Optimal Prediction of Synchronization-Preserving Races", POPL 2021 (PACMPL 5, Art. 36). [V-abs]**
- **Idea.** A race is sync-preserving if a witness keeps the observed order of lock acquires and releases. This class strictly subsumes HB races.
- **Guarantee.** Sound and complete for that class, in near-linear time and space.
- **Complexity.** Even one reversal of synchronization makes prediction NP-complete and W[1]-hard, so sync-preservation marks "the tractability boundary".

**GPredict: Huang, Luo, Roșu, "GPredict: Generic Predictive Concurrency Analysis", ICSE 2015, pp. 847–857. [V-abs]**
- **Idea.** *Generic* properties, not only races, are written as constraints over events together with thread causality. An SMT solver predicts violations allowed by the causal model. It needs only per-thread local traces.
- **Cost.** It cuts the runtime overhead of existing techniques by 54% on DaCapo.

**ExceptioNULL: Farzan, Madhusudan, Razavi, Sorrentino, "Predicting Null-Pointer Dereferences in Concurrent Programs", FSE 2012. [V-abs]**
- **Idea.** From one execution, predict interleavings that cause null dereferences. It abstracts shared communication, prunes with static lockset and uses SMT. "Use before initialized" is its target in spirit.

**What predictive analysis means for cFS (INFERENCE).**
- **C1, create-versus-lookup by name.** Both OSAL calls are atomic under the OSAL table lock. HB orders them in whatever order the observed run took, so plain HB cannot predict the other order. CP, WCP and SyncP can reorder critical sections that do not conflict *as seen by the lock*. But here the two critical sections *do* conflict, on the name table, so pure lock-reordering relations still order them.
  - The right model treats the registry key as the conflicting object, and the lock as atomicity, not order.
  - That is a GPredict or RVPredict-style constraint encoding: "is there a feasible reordering in which lookup(k) precedes insert(k)?"
  - Note: SyncP's "sync-preserving" restriction forbids exactly this reversal of lock order. Prediction for C1 is in the NP-hard regime in general. Startup traces are small, so SMT is plausible.
- **R2.** Every model above reports nothing for the uses that HB orders *before* the store. They are ordered, not racing. Predictive analysis cannot help with a fixed-order defect (§3 L4).
- **What to borrow:**
  - GPredict's property-as-constraint formulation, for readiness contracts.
  - RVPredict and MCM's control-flow abstraction, so that a predicted reordering is producible. A predicted lookup-before-insert must respect the branch on the lookup's result.
  - Vindicator's split: an over-approximate candidate generator plus a witness check.

### 2.6 Order-violation and atomicity-violation detection

**Lu, Park, Seo, Zhou, "Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics", ASPLOS 2008. [V-abs]**
- **Findings.** The study covers 105 bugs from MySQL, Apache, Mozilla and OpenOffice.
  - About **one third of non-deadlock bugs are order violations**, which locks and transactional memory do not express easily.
  - About 34% of non-deadlock bugs involve multiple variables.
  - About 73% of non-deadlock bugs were not fixed by adding or changing locks.
  - About 92% can be triggered reliably by enforcing orders among at most 4 memory accesses.
  - The often-quoted "97% are atomicity or order violations" was **not** confirmed; it is not relied on.
- **cFS.** All five cases are order violations or readiness violations. Fixes in the ledger were retry (C1), a new phase (C2), serialization plus guards (C3), lock-scope extension then a guard (C5), and none (R2). That spread agrees with "not fixed by locks" (INFERENCE from `ledger.md` §1b).
- **Borrow.** The small-depth premise (≤ 4 accesses) supports bounded exploration and PCT-style depth arguments (§2.10).

**AVIO: Lu, Tucek, Qin, Zhou, "AVIO: Detecting Atomicity Violations via Access Interleaving Invariants", ASPLOS 2006, pp. 37–48. [V-abs]**
- **Idea.** Learn "access interleaving invariants" (pairs of accesses never interleaved unserializably in correct runs), then flag unserializable interleavings that violate them. The overhead numbers were not verified.

**CTrigger: Park, Lu, Zhou, "CTrigger: Exposing Atomicity Violation Bugs from Their Hiding Places", ASPLOS 2009, pp. 25–36. [V-abs]**
- **Idea.** Trace analysis finds feasible unserializable interleavings with low occurrence probability. Minimal execution perturbation exercises them.
- **Result.** Bugs exposed in 1–235 s, two to four orders of magnitude faster than stress testing.
- **Borrow.** "Find rare feasible orderings from a correct run, then force them with minimal perturbation". This is the template for the confirmation stage.

**ConMem: Zhang, Sun, Lu, "ConMem: Detecting Severe Concurrency Bugs through an Effect-Oriented Approach", ASPLOS 2010. [V-abs]**
- **Idea.** Effect-oriented: interleavings that directly cause memory errors (null dereference, dangling pointer, uninitialized read). ConMem-v validates candidates by forcing the interleaving.
- **Result.** 8 of 9 bugs detected (ASPLOS version).
- **Borrow.** Classify a candidate by its **effect**. In cFS the effects are: event lost, error status, wrong AppId stamped, `APP_ERROR` exit.

**DefUse: Shi, Park, Yin, Lu, Zhou, Chen, Zheng, "Do I Use the Wrong Definition? DefUse: Definition-Use Invariants for Detecting Concurrency and Sequential Bugs", OOPSLA 2010, pp. 160–174. [V-abs]**
- **Idea.** Three kinds of definition-use invariants, learned from runs, detect order violations among others. 19 bugs detected, including 2 new, with few false positives.
- **cFS.** This is the closest established technique to R2 and C5. "This use reads the definition from that writer" is exactly what fails: EVS's read of `CFE_SB_Global.AppId` reads the EarlyInit zero instead of SB's store (INFERENCE).
- **Borrow.** Def-use pairs on readiness fields and registry keys as the unit of checking. Use a reference (the API docs) instead of learned invariants, to meet `CONDITIONS_POLICY.md`.

**Falcon: Park, Vuduc, Harrold, "Falcon: Fault Localization in Concurrent Programs", ICSE 2010. [V-abs]**
- **Idea.** It ranks data-access patterns by suspiciousness from pass and fail runs.

**Unicorn: Park, Vuduc, Harrold, "A Unified Approach for Localizing Non-deadlock Concurrency Bugs", ICST 2012, pp. 51–60. [P]**
- The venue is verified. The Georgia Tech abstract that names UNICORN matches the ICST abstract text, so the name-to-paper link is likely but not confirmed.
- **Idea.** Combine access pairs into patterns covering order violations and single- and multi-variable atomicity violations, then rank them.

**PSet: Yu and Narayanasamy, "A Case for an Interleaving Constrained Shared-Memory Multi-Processor", ISCA 2009, pp. 325–336. [P: bibliography verified; mechanism from course slides]**
- **Idea.** Hardware restricts execution to tested interleavings, using per-section predecessor sets.

**Bugaboo: Lucia and Ceze, "Finding Concurrency Bugs with Context-Aware Communication Graphs", MICRO 2009. [V-abs]**
- **Idea.** Context-aware communication graphs and anomaly detection.

**Recon: Lucia, Wood, Ceze, "Isolating and Understanding Concurrency Errors Using Reconstructed Execution Fragments", PLDI 2011. [V-abs; Grossman is not an author]**
- **Idea.** Reconstructions of the communication around the root cause, ranked by machine learning.
- **Cost.** 5×–20× graph-collection overhead, as low as 13%.
- **cFS.** PSet, Bugaboo and Recon are invariant or statistics-based, trained on many runs. They conflict with the reference-only policy (`CONDITIONS_POLICY.md`) unless the invariants come from documentation.
- **Borrow.** Communication graphs (producer→consumer of a value) are a good *report* format.

**Velodrome: Flanagan, Freund, Yi, "Velodrome: A Sound and Complete Dynamic Atomicity Checker for Multithreaded Programs", PLDI 2008, pp. 293–303. [V-abs]**
- **Idea.** A transactional HB graph. A cycle means a non-serializable trace.
- **Guarantee.** "The first dynamic analysis for atomicity that is both sound and complete" (for the observed trace).
- **Cost.** 12.7× reported, per a secondary DoubleChecker talk.
- **Borrow.** Treat a *logical operation* that spans several calls (e.g. create → register → publish) as a transaction. Check serializability of readiness transactions.

**Ad hoc synchronization:**
- **Xiong, Park, Zhang, Zhou, Ma, "Ad Hoc Synchronization Considered Harmful", OSDI 2010. [V-abs]** 229 instances in 12 programs; 22–67% introduced bugs or performance problems. SyncFinder found about 96% of them with about 6% false positives.
- **Tian, Nagarajan, Gupta, Tallam, "Dynamic Recognition of Synchronization Operations for Improved Data Race Detection", ISSTA 2008, pp. 143–154. [V-abs]** Spinning-read loops are the core of user-defined synchronization and are recognized dynamically.
- **cFS.** `CFE_ES_WaitForSystemState` (`cfe_es_api.c:585-603`), `CFE_ES_MainTaskSyncDelay` (`cfe_es_start.c:860ff`) and CF's name-lookup retry (`cf_cfdp.c:1280-1290`) are spinning-read or retry loops with sleeps (FACT, code).
- **Borrow.** Recognize them as synchronization, and model the edge **only when the loop exits by its condition** (§3 L3).

### 2.7 Event-driven systems (single-threaded loopers and queues)

**WebRacer: Petrov, Vechev, Sridharan, Dolby, "Race Detection for Web Applications", PLDI 2012, pp. 251–262. [V-abs; the tool name "WebRacer" is UNVERIFIED]**
- **Idea.** An HB model for browser event execution (parsing, scripts, events).

**EventRacer: Raychev, Vechev, Sridharan, "Effective Race Detection for Event-Driven Programs", OOPSLA 2013, pp. 151–166. [V-abs]**
- **Idea.**
  - **Race coverage.** Race a covers race b iff treating a as synchronization eliminates b. This exposes ad hoc synchronization and cuts false positives.
  - Chain decomposition reduces VC cost for many short events.
- **Borrow.** Race coverage is the principled way to handle cFS polling barriers: report the root "synchronization race", not every downstream pair.

**DroidRacer: Maiya, Kanade, Majumdar, "Race Detection for Android Applications", PLDI 2014; SIGPLAN Notices 49(6):316–325. [V-abs]**
- **Idea.** It formalizes Android concurrency semantics and defines an HB that generalizes multithreaded and single-threaded event-driven HB.
  - Trace operations named in a secondary summary: fork, post, begin, end, acquire, release, read, write, enable. "Enable" links lifecycle enabling to later posts.
  - Systematic UI testing generates the traces. 15 apps were analyzed.
- The exact FIFO and delayed-post rules are **UNVERIFIED** here.

**CAFA: Hsiao, Yu, Narayanasamy, Kong, Pereira, Pokam, Chen, Flinn, "Race Detection for Event-Driven Mobile Applications", PLDI 2014, pp. 326–336. [V-abs; the author order follows a citing reference list]**
- **Idea.**
  - A causality model that "accounts for the causal order due to the event queues".
  - It checks races between **high-level operations** rather than memory accesses.
  - Thread-based tools "infer false dependencies between unrelated events handled sequentially by the same thread".
- That CAFA's operation class is "use versus free" is **UNVERIFIED**.

**InitRacer: Adamsen, Møller, Tip, "Practical Initialization Race Detection for JavaScript Web Applications", OOPSLA 2017 (PACMPL 1, Art. 66). [V-abs]**
- **Idea.** Three startup error types: form-input-overwritten, **late-event-handler-registration** and **access-before-definition**. Detection combines adverse and approximate execution.
- **Result.** 1085 races on 100 sites; of 218 reviewed, 111 caused uncaught exceptions.

**Go memory model ("Version of June 6, 2022"). [V-doc]** These are canonical statements of queue and initialization HB rules:
- "A send on a channel is synchronized before the completion of the corresponding receive" (`go_mem.html` L373-374).
- "The *k*th receive from a channel with capacity *C* is synchronized before the completion of the *k*+*C*th send" (L456).
- "The completion of all `init` functions is synchronized before the start of the function `main.main`" (L300-301).
- "The completion of a single call of `f()` from `once.Do(f)` is synchronized before the return of any call of `once.Do(f)`" (L560).

**What event-driven work means for cFS (INFERENCE).**
- A cFS app main loop (`CFE_SB_ReceiveBuffer` → dispatch → handle) is a looper over one or more SB pipes.
- CAFA's central lesson applies directly. Program order on the app's task would order every message the app handles, and so hide races between messages from different senders.
  - PO must be restricted to **within one message handler**, plus explicit edges.
  - DCatch makes the same point for RPC and event handlers (§2.8, Rule-Pnreg).
- **SB subscribe versus the first publish.** A no-subscriber message is dropped with a success return (`cfe_sb_priv.c:1082-1087`). That is InitRacer's "late event handler registration". R2's 17 `SEND_NO_SUBS` uses during ES init are of this shape (FACT, record, R2 §2).
- **Readiness defects** are InitRacer's "access-before-definition", lifted from DOM names to cFS registries.
- **Go's capacity rule does not transfer.** SB sends never block: zero timeout, drop on full. Capacity creates no edge. A full queue produces a loss instead, which is an *outcome* that depends on the order of receives and sends.
- **Go's init and once rules** are the right shape for ES phase barriers. Unlike Go, cFS barriers are *soft* (timeouts) and *opt-in*. At v7.0.1 the core tasks wait only for CORE_READY, not for OPERATIONAL (§1). The ledger's statement that core tasks do not call the startup sync refers to 6.4.1 (`ledger.md` C3 row). App tasks decide for themselves whether to call `WaitForStartupSync`. In the pinned tree only TO_LAB, HS, CS and SBN call it or `WaitForSystemState`, outside unit tests; CF does not (grep over `apps/*/fsw/src`). In C1's public integration, neither CF nor BP calls it (`ledger.md` C1).

### 2.8 Distributed concurrency bugs

**TaxDC: Leesatapornwongsa, Lukman, Lu, Gunawi, "TaxDC: A Taxonomy of Non-Deterministic Concurrency Bugs in Datacenter Distributed Systems", ASPLOS 2016. [V-abs]**
- **Scope.** 104 bugs in Cassandra, Hadoop MapReduce, HBase and ZooKeeper; 2,083 labels covering the triggering timing condition, preconditions, symptoms and fixes.
- **Triggers.** Untimely message arrivals, crashes and reboots. The per-class percentages were **not** verified.

**DCatch: Liu, Li, Lukman, Li, Lu, Gunawi, Tian, "DCatch: Automatically Detecting Distributed Concurrency Bugs in Cloud Systems", ASPLOS 2017. [P]**
- The author list is from the DBLP key cited by a later paper (LiuLLLLGT17). The rule names are from a search excerpt of the authors' preprint and a patent text.
- **Order model.** HB rules cover messages between nodes, thread creation across nodes, events and program order.
  - Rule-Rrpc abstracts away RPC-library queues.
  - Rule-Eenq abstracts away event dispatching between enqueue and handler start.
  - In event-handler and RPC threads, "ordering is guaranteed only when both operations are inside the same event handler function or RPC function" (Rule-Pnreg).
- **Pipeline.** Tracing → HB analysis for concurrent conflicting accesses → triage (prune bugs with only local effect and no run-time error) → **triggering**.
- **Borrow.**
  - The most transferable HB *design*: abstract framework-internal queues into one send→handle edge; restrict PO to within a handler.
  - Triage by "does it have a non-local effect?"
  - A triggering module for confirmation.

**FCatch: Liu, Wang, Li, Lu, Ye, Tian, "FCatch: Automatically Detecting Time-of-Fault Bugs in Cloud Systems", ASPLOS 2018; SIGPLAN Notices 53(2):419–431. [V-abs]**
- **Idea.** Time-of-fault (TOF) bugs are a concurrency class predicted from *correct* runs.
  - Crash-regular: the crash comes before a write, and a timeout is the recovery mechanism.
  - Crash-recovery: the crash comes after a write, and a sanity check is the recovery mechanism.
  - "Fault-aware logical time" over conflicting accesses on shared state. The detail is from slides.
- **cFS.** App restart and reload, processor reset and CDS or persistent RAM (`env/CONDITIONS.md` M1-7: files on `/dev/shm/osal:RAM` survive POWER ON boots) are the cFS analogues (INFERENCE).

**CloudRaid: Lu, Li, Li, Feng, "CloudRaid: Hunting Concurrency Bugs in the Cloud via Log-Mining", ESEC/FSE 2018, pp. 3–14. [V-abs]**
- **Idea.** Mine logs for message pairs that are feasible but always arrived in one order, then flip them.
- **Result.** 28 bugs including 8 new; 40 versions tested in 35 hours.
- **Borrow.** "Observed-always-one-order" pairs as candidates. Our R2 runs are such a pair. Logs-as-traces fits cFS's EVS and syslog.

**FlyMC: Lukman, Ke, Gunawi et al., "FlyMC: Highly Scalable Testing of Complex Interleavings in Distributed Systems", EuroSys 2019, 20:1–20:16. [V-abs]**
- **Idea.** Model checking messages and faults with state symmetry, event independence and parallel flips. On average 16× (up to 78×) faster than the prior state of the art.
- **Borrow.** Independence-based reduction for the bounded-exploration stage.

### 2.9 OS, kernel and process-level detection

**RacePro: Laadan, Viennot, Tsai, Blinn, Yang, Nieh, "Pervasive Detection of Process Races in Deployed Systems", SOSP 2011. [V-abs]**
- **Idea.**
  - Record processes on deployed systems.
  - Find races among **system calls** that may concurrently touch shared kernel objects, modeling under-specified syscalls as load and store micro-operations.
  - **Replay and go-live** separates harmful races from benign ones.
  - The SOSP slides add wait-wakeup races (a pipe read woken by any writer, `waitpid` by any child).
- **Cost.** "Modest" recording overhead.
- **FACT (record), code-derived** (`baselines/availability.md` §RacePro row):
  - HB sources are program order, fork/clone, wait, pipe/socket and signal.
  - Candidate classes are RESOURCE, EXIT-WAIT, SIGNAL and TOCTOU.
  - Memory events never become candidates.
  - RacePro gives no candidate whose racing pair is the dependency of any of the five cases. It is `not_runnable_here` (i386 Linux 2.6.35 with Scribe).
- **Borrow.**
  - The micro-op modeling of a *namespace* (a create and a lookup of the same name as store and load). Apply it to OSAL and ES registries, which live in user memory and are invisible to RacePro.
  - Replay plus go-live as the confirmation pattern.

**Process-level follow-ups from the Yu group (`baselines/availability.md`; FACT, record, unless tagged):**

| Tool | Venue and status | Idea |
|---|---|---|
| SimRacer | DOI 10.1145/2483760.2483771, ISSTA 2013 [P: venue inferred from the DOI prefix and the availability record]; abstract V-abs | Potential process races from traces of existing tests, then controlled process scheduling on a virtual platform. |
| SIMEXPLORER | STVR 2017 [V-abs] | |
| DESCRY | Yu, Zaman, Wang, ESEC/FSE 2017 [V-abs] | Reproduces system-level concurrency failures from default field logs. |
| RRF | ISSRE 2016 per the record [UNVERIFIED here] | |
| SysPro | Zaman et al., arXiv 2601.09616, accepted in JSS, 2026 [V-abs, secondary] | |

All of these model processes and syscalls. The record finds the five cases' dependencies to be memory stores, not syscalls.

**Razzer: Jeong, Kim, Shivakumar, Lee, Shin, "Razzer: Finding Kernel Race Bugs through Fuzzing", IEEE S&P 2019, pp. 754–768. [V-abs; the task's author list was wrong]**
- **Idea.** Static analysis finds potentially racy locations. A fuzzer is steered toward them. A **hypervisor-level deterministic interleaving** reproduces each race.
- **Result.** 30 new races, 16 confirmed and patched.

**SKI: Fonseca, Rodrigues, Brandenburg, "SKI: Exposing Kernel Concurrency Bugs through Systematic Schedule Exploration", OSDI 2014, pp. 415–431. [V-abs]**
- **Idea.** A VMM controls which virtual CPU proceeds, giving systematic kernel interleaving exploration without modifying the kernel.

**Snowboard: Gong, Altınbüken, Fonseca, Maniatis, "Snowboard: Finding Kernel Concurrency Bugs through Systematic Inter-thread Communication Analysis", SOSP 2021. [V-abs]**
- **Idea.** "Potential memory communication" (PMC): run tests sequentially, pair writer and reader tests on the same region, and use each PMC as a scheduling hint.
- **Result.** 14 new bugs, 12 confirmed.
- **Borrow.** Profile each app's startup **alone** (sequentially), compute the potential communications (publish→use of the same field or key), then schedule pairs. This avoids depending on the one schedule the normal boot happens to produce.

**KCSAN and DataCollider:** see §2.4.

**Embedded (interrupt-driven): SDRacer, Wang, Gao, Wang, Yu, Wang, Zhao, Li, ISSTA 2017; extended in arXiv 2305.17869 (2023). [V-abs]**
- **Idea.** Static analysis plus symbolic execution find potential races. A **virtual platform forces interrupts** at the racing points to validate them.
- **cFS.** cFS on POSIX has no interrupt handlers in the apps. The confirmation pattern ("force the other side to arrive at the racing point") is the same.

### 2.10 Schedule control, confirmation and triage

| Paper | Tag | Idea | Borrow |
|---|---|---|---|
| **RaceFuzzer**: Sen, "Race Directed Random Testing of Concurrent Programs", PLDI 2008, pp. 11–21 | V-abs | Potential races from an imprecise detector steer a random scheduler so that the pair occurs; the outcome is then resolved randomly. | Separate real candidates from false ones without manual inspection. |
| **CHESS**: Musuvathi, Qadeer, Ball, Basler, Nainar, Neamtiu, "Finding and Reproducing Heisenbugs in Concurrent Programs", OSDI 2008 | V-abs | Takes control of thread scheduling and searches interleavings systematically. Reproduces each bug found. | |
| **PCT**: Burckhardt, Kothari, Musuvathi, Nagarakatte, "A Randomized Scheduler with Probabilistic Guarantees of Finding Bugs", ASPLOS 2010 | V-abs | Bug depth is the minimum number of scheduling constraints that expose the bug. A run finds a depth-*d* bug with probability ≥ 1/(n·k^(d−1)). The paper hypothesizes that ordering bugs have small depth. | The natural explorer for cFS (§3 L7). Ordering violations are depth 1 or 2 (INFERENCE). |
| **DPOR**: Flanagan and Godefroid, "Dynamic Partial-Order Reduction for Model Checking Software", POPL 2005, pp. 110–121 | V-abs | | Exhaustive bounded exploration of the high-level event interleavings of a candidate. |
| **Narayanasamy, Wang, Tigani, Edwards, Calder**, "Automatically Classifying Benign and Harmful Data Races Using Replay Analysis", PLDI 2007 | V-abs | Replay each race in both orders and compare. | |
| **Portend**: Kasikci, Zamfir, Candea, "Data Races vs. Data Race Bugs: Telling the Difference with Portend", ASPLOS 2012, pp. 185–198 | V-abs | Classify by consequence over multiple paths and schedules. 76–90% of true races found by detectors are harmless. 92 of 93 classified correctly. | Our verdict classes (inactive path / protected in model / fixed-order defect / schedule-dependent candidate / unconfirmed; proposal §1) need exactly this outcome-based triage, with the outcome defined by the API contract, not by "crash or not". |

---

## 3. Cross-cutting lessons for a cFS detector (INFERENCE unless marked)

**L1. The unit of analysis is a framework-level operation, not a memory access.**
- CAFA, DCatch, RacePro (syscall micro-ops) and GPredict (generic events) all lift the trace.
- For cFS the events are: task create and start; app or task registration; app-state and system-state transitions; registry insert, finalize and lookup with result; SB pipe create, subscribe, send (delivered / queue-full / no-subscriber) and receive; EVS register and send (accepted / rejected); TBL register, share, load, manage, get-address and release; lock and semaphore operations; wait-for-state with exit reason; retries.
- Memory accesses are added only where a contract names a field: `CFE_SB_Global.AppId`, `TaskTable[]`.

**L2. HB must be complete over the framework's synchronization, or it will flood.**
- This is KCSAN's argument against KTSAN (V-doc).
- §4 lists every mechanism. Anything not modeled must be marked "unmodeled", not silently treated as unordered.

**L3. Polling barriers create conditional edges.**
- `WaitForSystemState` gives HB from ES main's store of state *s* to the waiter's return **only if** the loop exited because `SystemState ≥ s` (`cfe_es_api.c:585`), not by timeout (`:597-600`).
- `WaitForStartupSync` hides which one happened (`void`, `:616-619`), so the detector must record the exit reason itself.
- EventRacer's race coverage and Tian et al. / SyncFinder are the established ways to keep such loops from becoming false positives.

**L4. HB races are only unordered pairs. Many readiness defects are HB-ordered in the wrong direction.**
- In R2, the uses before SB's store are ordered before it by S1 serialization (the R2 record labels this inference).
- No HB or predictive race detector (Djit+ through SyncP) reports an ordered pair, by definition.
- A **contract check over the HB-ordered (and predicted) trace** is required, in the style of runtime verification, DefUse or GPredict.
- This is consistent with the proposal's separation of "required order" from "guaranteed order". Adding required orders as HB edges "can erase the defect from the model" (proposal v0.1, around L254).

**L5. Locks give atomicity, not order.**
- C1's create and lookup, and C5's registration and lookup, are each lock-protected (FACT, record, `ledger.md`).
- Under plain HB the lock edge records the observed order and hides the other order. This is the HB false negative of O'Callahan–Choi, CP and WCP.
- For registry operations, model the lock as an atomic region and the **key** as the conflicting object. Ask a constraint solver whether the reverse order is feasible (RVPredict and GPredict style). The sync-preserving restriction (SyncP) is too strong here because the reversal *is* a lock-order reversal.

**L6. Losses are outcomes, not edges.**
- Queue-full drops (`cfe_sb_priv.c:1188-1193`), no-subscriber drops (`:1082-1087`) and EVS filtering or squelch are order-dependent outcomes.
- Prior event-driven HB models assume reliable delivery or blocking channels (Go, V-doc). For cFS, "loss under some feasible order" is a reportable property.

**L7. Exploration and confirmation.**
- PCT's depth bound, together with Lu et al.'s finding that 92% need ≤ 4 ordered accesses, justifies bounded exploration with small *d*.
- cFS priorities can seed PCT's priority assignment without being treated as edges. Under the ENV's `SCHED_OTHER` they are not enforced anyway (E4).
- Confirmation borrows:
  - delay at one endpoint (DataCollider and KCSAN stall; CTrigger minimal perturbation);
  - directed scheduling (RaceFuzzer; DCatch triggering);
  - both-order replay with consequence classification (Narayanasamy et al.; Portend; RacePro go-live).

**L8. Sampling does not fit once-per-boot pairs.**
- LiteRace and Pacer give guarantees that are proportional to the sampling rate. Startup readiness events are few, so full tracing of high-level events costs little.
- Startup trace volume is roughly hundreds of events: about 230 SB-API uses per boot in R2 (`R2_gdb_v701.md` §2).

**L9. State guarantees relative to the model.**
- Following SHB, SyncP and Vindicator, state claims as "no false alarm relative to the modeled rules and the observed basis B". State the unmodeled mechanisms explicitly. KCSAN's doc is a good model of candor about which ordering rules are covered.

---

## 4. Mapping: cFS mechanism → best-matching established rule → source

The "cFS site" column is FACT (code). The rule match and the caveats are INFERENCE. "Source" names the paper whose rule is closest. It does not claim that the paper analyzed cFS.

| # | cFS mechanism (site) | Best-matching established HB or predictive rule | Source(s) | Caveat for cFS |
|---|---|---|---|---|
| 1 | Program order inside one task's init sequence (`*_TaskInit`, app `*_Init`) | Program order | Lamport 1978; all VC detectors | Restrict it inside main loops (row 8). |
| 2 | Task creation: `OS_TaskCreate` in ES app create; `CFE_ES_CreateChildTask` (`cfe_es_api.c:1202`) | Fork: create → first event of the child | Djit+ / FastTrack fork rule; DroidRacer fork; Go `go` statement (`go_mem.html` L307-308) | The edge starts at the create call, *not* at the later ES registration. C5's window lies exactly between the two (`ledger.md` C5). |
| 3 | Task exit, app delete or restart, generation reuse | Join / exit-wait | Djit+ join; RacePro EXIT-WAIT (`availability.md`) | IDs and names are reused across incarnations. Events need (id, generation) identity. FCatch crash-recovery is the analogue for restart. |
| 4 | OSAL mutex, `CFE_ES_LockSharedData`, `CFE_SB_LockSharedData` (`cfe_sb_priv.c:132-160`) | HB: release → acquire. For prediction: CP / WCP / SyncP reordering of critical sections | Djit+, FastTrack; Smaragdakis 2012; Kini 2017; Mathur 2021 | Locks give atomicity, not readiness order (L5). Registry operations need key-based conflict and SMT feasibility (RVPredict, GPredict). |
| 5 | OSAL counting and binary semaphores (give/take) | Signal → wait edge (release → acquire on a synchronization object) | Generic HB in Djit+ and TSan; DroidRacer and DCatch treat signal-wait in their thread rules (detail UNVERIFIED) | A give with no matching take creates no edge. A counting semaphore matches the Go capacity idiom only conceptually. |
| 6 | **SB send → receive** of the same buffer (`OS_QueuePut` at `cfe_sb_priv.c:1177-1178`; `OS_QueueGet` → `mq_receive`, `os-impl-queues.c:209,238`) | Message send → receive; enqueue → handler-begin | Lamport 1978; DroidRacer post → begin; CAFA event-queue causality; DCatch Rule-Eenq; Go "send … before completion of the corresponding receive" (L373) | Only **delivered** sends create edges. A zero-timeout `mq_timedsend` (`os-impl-queues.c:300-305`) drops on full, so Go's capacity rule (L456) does **not** apply. |
| 7 | FIFO within one pipe (constant mq priority 1, `os-impl-queues.c:305`) | Single-consumer FIFO: enq(m1) ≺ enq(m2) ⇒ end(handle m1) ≺ begin(handle m2) | DroidRacer and CAFA looper FIFO rules (exact form UNVERIFIED) | POSIX equal-priority FIFO is UNVERIFIED here (standard not reachable). The rule holds per pipe only. Apps with several pipes or receive timeouts get no cross-pipe order. |
| 8 | App main-loop dispatch (one message handled at a time per app task) | PO only inside one handler; handlers on a looper are atomic but **unordered** unless an edge exists | CAFA (false dependencies from thread PO); DCatch Rule-Pnreg | Without this, thread-level HB orders all messages an app handles and hides races between senders. |
| 9 | **SB subscribe vs first publish** (no-subscriber drop with success, `cfe_sb_priv.c:1082-1087`) | Initialization race: late-event-handler-registration | InitRacer (Adamsen 2017); EventRacer | The outcome is silent loss with a success return. R2 observed `SEND_NO_SUBS` during init (R2 §2). |
| 10 | **ES phase barriers**: `CFE_ES_WaitForSystemState` / `WaitForStartupSync` polling `volatile SystemState` (`cfe_es_api.c:585-619`; writes at `cfe_es_start.c:87-234`) | Barrier / init / once edge; ad hoc synchronization recognition | Go init and `once.Do` rules (L300-301, L560); Tian 2008; Xiong 2010; EventRacer race coverage | A **conditional** edge, valid only on exit by condition, not by timeout. Opt-in per app. Core tasks wait for CORE_READY only (§1). The wait call also publishes the caller's own state (`cfe_es_api.c:566-574`). Aggregation is over apps registered at check time (`cfe_es_start.c:878-888`). |
| 11 | Serialized core startup (`CFE_ES_MainTaskSyncDelay(...RUNNING...)` per core app, `cfe_es_start.c:835`) | Barrier per core app: child reaches RUNNING → ES main creates the next app | Same as row 10 | Gives a *fixed* order. A defect under it is HB-ordered in the wrong direction (R2), so it is out of reach of race detectors (L4). |
| 12 | **Named registries**: `OS_ObjectIdFindByName` (`osapi-idmap.c:975`) vs `OS_ObjectIdFinalizeNew` (`:838`); `CFE_ES_GetAppIDByName`; `CFE_TBL_Share` (`cfe_tbl_api.c:184`); SB pipe-name lookup (`cfe_sb_api.c:802`) | Access-before-definition, as key-based def-use on a namespace (insert = store, lookup = load) | InitRacer; DefUse; RacePro load/store micro-ops on kernel namespaces | The registries are in user memory, invisible to RacePro (`availability.md`). Lookup failure may be contract-permitted (CF retry, `cf_cfdp.c:1272-1290`). |
| 13 | Task registration (`TaskTable` written by the creator after `OS_TaskCreate`; the child's `CFE_ES_GetAppID`) | Order violation (def-use) | Lu 2008 order-violation class; DefUse; Unicorn | Fork edge placement (row 2) decides whether it is unordered (a race) or ordered (impossible). |
| 14 | Readiness fields: `CFE_SB_Global.AppId` store (`cfe_sb_task.c:138`) and EVS registration (`:155`) vs `CFE_EVS_SendEventWithAppID` reads → `ILLEGAL_APP_ID` (`cfe_evs.c:185-187`) | Def-use invariant, i.e. the use must read the publishing definition; a generic predictive property | DefUse (Shi 2010); GPredict (Huang 2015); ExceptioNULL (use before init) | R2's uses are HB-ordered before the store, so a contract check is needed, not race detection. |
| 15 | Tables: `CFE_TBL_GetAddress` … `ReleaseAddress` (`cfe_tbl_api.c:439,478`; `LockFlag` `:489`); updates applied in the owner's `CFE_TBL_Manage` (`:731`) | Atomic region (reader section) plus update-deferral; serializability of reader regions vs updates | Velodrome; AVIO | It is a protocol, not a lock. Model GetAddress → Release as a region, and Manage-applied update → next GetAddress as an edge. |
| 16 | Periodic wakeups (SCH_LAB), SB receive timeouts, `OS_TaskDelay` | Timer events are events with HB from registration to firing, never from wall-clock time | WebRacer and EventRacer timers; DroidRacer delayed post; Lamport (physical clocks ≠ HB) | Never derive HB from time. Timing affects only which schedule is observed. |
| 17 | External input (CI_LAB UDP commands, ground) | Environment events are roots with no incoming HB | EventRacer; CAFA; DCatch (messages from outside) | Basis *B* must fix inputs and their timing (proposal §2.3). |
| 18 | Losses: queue full (`cfe_sb_priv.c:1188-1193`), message-limit (`:1058-1063`), EVS filter or squelch | No established HB rule. An outcome predicate over orders. | — (gap G5) | The property is "lost under some feasible order". |
| 19 | App restart / reload; processor reset; CDS and persistent RAM (`env/CONDITIONS.md` M1-7) | Fault-aware logical time; crash-regular and crash-recovery TOF | FCatch; TaxDC (reboot timing) | Out of the first scope (startup), but it is the same machinery. |
| 20 | Priorities (startup script, core config) | **No HB.** Use only as an exploration bias. | PCT (priority-based randomized scheduler) | ENV: `SCHED_OTHER`, priorities not enforced (E4). C3's order came from priorities on its original RTOS, so priorities belong to basis *B*, not to HB. |

---

## 5. The gap: what no surveyed technique models and cFS requires (INFERENCE)

Each item names the closest prior work and says why it does not cover the item.

**G1. Framework-level readiness contracts with legal outcomes.**
- **The contract:** "operation O on resource R requires R in state S; if S does not hold, the allowed outcomes are {wait, retry, explicit error}".
- **Closest prior work:**
  - InitRacer: three web-specific error types, fixed heuristics.
  - DefUse and AVIO: invariants learned from runs.
  - GPredict: user-written generic properties, Java, no notion of a legal error outcome.
  - KCSAN `ASSERT_EXCLUSIVE_*`: declared exclusivity, not readiness.
- **Not covered:**
  - Contracts drawn from **framework API documentation**, as `CONDITIONS_POLICY.md` requires. Example: the `cfe_evs.h` API note that R2 cites.
  - Treating a documented error return (`NAME_NOT_FOUND` followed by a bounded retry in CF) as conforming, not as a race report.

**G2. Violations that are HB-ordered in the wrong direction.**
- Every HB and predictive race detector reports unordered pairs only. R2's readiness violation is ordered by framework startup serialization.
- No surveyed detector combines race prediction (for schedule-dependent cases such as C1 and C5) with contract checking over ordered pairs (for fixed-order cases such as R2), and classifies the result into the proposal's verdict classes.

**G3. Phased startup with soft, opt-in, timed-out barriers.**
- Go-style init and once edges are unconditional. EventRacer and Tian et al. recognize ad hoc synchronization but do not model **timeout-conditional** edges.
- They also do not model a barrier whose participant set is "apps registered at check time" (`cfe_es_start.c:878-888`), or a barrier that only some tasks honor: CF and BP in C1 do not wait at all, and at v7.0.1 the core tasks wait for CORE_READY only. They also do not model a wait call that doubles as a state publication (`cfe_es_api.c:566-574`).
- C2 (late init after RUNNING) and C3 (core tasks outside the startup sync) live in exactly this gap.

**G4. User-space named-object registries with reserve, finalize and reuse.**
- RacePro models kernel namespaces through syscalls. cFS registries (the OSAL idmap, the ES app and task tables, the TBL registry, SB pipe names) are user-space tables behind locks. A name becomes visible only at finalize (`osapi-idmap.c:838`).
- IDs and names are reused across restarts.
- No surveyed tool treats "registry key" as the conflict object, with lock = atomicity and finalize = publication.

**G5. Lossy, bounded, non-blocking queues as a race outcome.**
- Event-driven and distributed HB models assume delivery: Android loopers, the browser event loop, blocking Go channels, RPC.
- In cFS, queue-full and no-subscriber drops return status or count silently. Our ENV shows 4–49 queue-full drops per boot (M1-3).
- No surveyed technique predicts "message lost under some feasible order" as a first-class property.

**G6. Layered identity in a library-OS: executing task vs owning app vs called service vs stamped AppId.**
- cFE service APIs run in the caller's thread, e.g. ES's task executing SB code that calls EVS with SB's AppId (R2).
- CAFA and DroidRacer separate thread from event. DCatch abstracts RPC thread pools.
- None tracks "on whose behalf" a library call runs separately from the thread that runs it. Yet that is exactly what R2's rejected events turn on (proposal §2.1).

**G7. Configuration as part of the execution basis.**
- Several configuration items decide whether a path is active at all:
  - startup-script order;
  - table contents (CF's `sem_name` is empty by default, so C1's path is latent: `ledger.md` C1);
  - effective versus requested priorities;
  - truncated pipe depths (E5).
- Dynamic tools take configuration as given. A cFS detector must report "inactive path" separately from "no race", and record the basis *B*.

**G8. Priority-based ordering that holds only on some platforms.**
- C3's bad order came from priorities on an RTOS. On the ENV the same priorities have no effect.
- No dynamic HB model treats priorities as edges, and that is correct.
- The gap is a *verdict* that says "ordered on platform class X (RT, single core), unordered on Y". The proposal's basis *B* is where this belongs.

**Summary of the gap.** The established techniques supply the parts:
- HB/VC, predictive SMT, and framework-aware HB rules (DroidRacer, CAFA, DCatch);
- an initialization-race taxonomy (InitRacer);
- def-use invariants (DefUse);
- triggering and confirmation (RaceFuzzer, CTrigger, DCatch, RacePro, Portend).

Nothing surveyed combines them around the cFS-specific semantics G1–G8. A paper contribution can claim that combination, the cFS HB rule set (§4) and the contract formulation. It cannot claim novelty for any individual mechanism in §4 rows 1–17.

---

## 6. What the new detector should borrow (INFERENCE; for the AR2 design phase)

1. **Trace layer.**
   - High-level cFS events (L1), recorded with per-task pre-allocated buffers. The proposal already says this.
   - Optionally, TSan underneath for memory races, with framework edges supplied through annotations (the TSan WBIA idea).
2. **HB engine.**
   - Vector clocks with the §4 rules.
   - PO limited to one handler in app main loops (CAFA, DCatch).
   - Conditional barrier edges (L3).
   - Every unmodeled mechanism listed explicitly (KCSAN candor).
3. **Prediction.**
   - A constraint encoding in the RVPredict / GPredict style: key-based conflicts, locks as atomicity, control-flow abstraction for feasibility.
   - It asks whether a feasible reordering exists in which a use precedes its publication, or a send precedes the subscription or the receiver's drain.
   - It runs per candidate (Vindicator: over-approximate, then vindicate).
4. **Contract checking** over the observed and predicted traces, for HB-ordered violations (G2). Contracts come from API documentation, as the reference policy requires.
5. **Exploration.**
   - PCT with small *d* for probabilistic coverage, and DPOR for exhaustive bounded coverage of one candidate's events.
   - Priorities serve only as a PCT seed (row 20).
6. **Confirmation.**
   - Delay injection at one endpoint (DataCollider, KCSAN, CTrigger), or directed scheduling (RaceFuzzer, DCatch triggering).
   - Then classify by consequence against the contract's legal outcomes (Portend, Narayanasamy et al.), never by "crashed or not".
7. **Profiling for candidates.** Snowboard-style sequential per-app startup profiling to compute potential publish→use communications independently of the one observed boot order. CloudRaid-style "always observed in one order" pairs as a cheap candidate source.

---

## 7. Runtime-cost and result figures as reported (for the related-work section)

All figures are as the papers state them, verified at the V-abs or V-doc level. "secondary" means the figure comes from another paper's report.

| Technique | Reported figure |
|---|---|
| FastTrack | About 10× faster than traditional VC; about 2× (CACM: 2.3×) faster than DJIT+ |
| RaceTrack | About 3× slowdown (memory-intensive), < 2× otherwise; memory < 1.2× |
| TSan (Clang docs) | 5×–15× slowdown, 5×–10× memory (via search summary); TSan v2 with sampling < 1.5× (RV 2011) |
| LiteRace | > 70% of races at < 2% of accesses sampled |
| Pacer | Detection probability = sampling rate; 1–3% rates suitable for production |
| DataCollider | "negligible" overhead claimed; 25 confirmed races, 12 fixed |
| KCSAN | 5.0× (default), 2.8× (fast path), kernel boot on 8 CPUs (V-doc) |
| RacePro | "modest" recording overhead (abstract) |
| Velodrome | 12.7× (secondary) |
| GPredict | 54% lower overhead than prior PTA on DaCapo |
| Recon | 5×–20×, as low as 13% |
| CTrigger | Bugs exposed in 1–235 s, 10²–10⁴× faster than stress testing |
| ConMem | 8 of 9 bugs found |
| DefUse | 19 bugs found, 2 new |
| Portend | 92 of 93 races classified correctly |
| InitRacer | 1085 reports; 111 of 218 reviewed caused uncaught exceptions |
| CloudRaid | 28 bugs (8 new), 40 versions in 35 h |
| FlyMC | 16× (up to 78×) faster than prior model checkers |
| Razzer | 30 new races, 16 confirmed |
| Snowboard | 14 new bugs, 12 confirmed |
| Lu et al. 2008 | 105 bugs. About one third of non-deadlock bugs are order violations; 34% multi-variable; 73% not fixed by locks; 92% triggerable by ordering ≤ 4 accesses |
| Ad hoc sync (OSDI 2010) | 229 instances; 22–67% buggy or slow; SyncFinder about 96% recall, about 6% false positives |
| TaxDC | 104 bugs, 4 systems, 2,083 labels |
| PCT | P(find depth-*d* bug) ≥ 1/(n·k^(d−1)) per run |
| SyncP | Beyond sync-preservation: NP-complete and W[1]-hard |

---

## 8. Open questions

1. **POSIX memory synchronization for `mq_send`/`mq_receive`.** Is the pair listed in XBD §4.12/4.15? Is it intercepted as an HB edge by TSan? Neither was verifiable here (pubs.opengroup.org and the LLVM docs are unreachable). This decides whether TSan can serve as the memory layer without false positives on SB payloads.
2. **Equal-priority FIFO of POSIX message queues** (row 7). The standard text is not reachable. The OSAL always passes priority 1 (`os-impl-queues.c:305`).
3. **Exact HB rule tables** of DroidRacer, CAFA, EventRacer and DCatch. The primary PDFs are blocked. If the paper needs a rule-by-rule comparison, someone must supply the PDFs (`research/cfs_procrace/paper/` is the place `availability.md` A1 proposes) or allowlist the hosts.
4. **Should TSan be run on the ENV build** as a measured baseline?
   - It would provide the "state-of-the-art memory-race detector on cFS" row and quantify the `SystemState` polling report (INFERENCE §2.3).
   - It needs a rebuild with `-fsanitize=thread`, i.e. a deviation recorded under `CONDITIONS_POLICY.md`, and a PI decision.
5. **Timeout-conditional barrier edges (G3).** Is the formalization novel? A targeted search of runtime-verification venues (RV, TACAS) for "timeout" plus "happens-before" was not done.
6. **MultiRace journal (CCPE 2007), the Unicorn = ICST 2012 link, and the WebRacer tool name.** Each is UNVERIFIED or partly verified, and none is relied on. Verify them before citing in the paper.

---

## 9. References with verification status

| Key | Reference | Status |
|---|---|---|
| Lamport78 | L. Lamport. Time, Clocks, and the Ordering of Events in a Distributed System. CACM 21(7):558–565, 1978. | V-abs |
| Fidge88 | C. J. Fidge. Timestamps in Message-Passing Systems That Preserve the Partial Ordering. 11th Australian Computer Science Conf., 1988 (pp. 56–66 or 55–66). | V-abs (pages vary) |
| Mattern89 | F. Mattern. Virtual Time and Global States of Distributed Systems. Proc. Workshop on Parallel and Distributed Algorithms, Elsevier, pp. 215–226, 1989. | V-abs |
| NetzerMiller92 | R. H. B. Netzer, B. P. Miller. What Are Race Conditions? Some Issues and Formalizations. ACM LOPLAS 1(1):74–88, 1992. | V-abs |
| PoznianskySchuster03 | E. Pozniansky, A. Schuster. Efficient On-the-Fly Data Race Detection in Multithreaded C++ Programs. PPoPP 2003. | P |
| MultiRace07 | Pozniansky, Schuster. MultiRace…, CCPE 2007. | UNVERIFIED |
| FastTrack09 | C. Flanagan, S. N. Freund. FastTrack: Efficient and Precise Dynamic Race Detection. PLDI 2009; CACM 53(11):93–101, 2010. | V-abs |
| Eraser97 | S. Savage, M. Burrows, G. Nelson, P. Sobalvarro, T. Anderson. Eraser: A Dynamic Data Race Detector for Multithreaded Programs. ACM TOCS 15(4):391–411, 1997. | V-abs |
| OCallahanChoi03 | R. O'Callahan, J.-D. Choi. Hybrid Dynamic Data Race Detection. PPoPP 2003. | V-abs |
| RaceTrack05 | Y. Yu, T. Rodeheffer, W. Chen. RaceTrack: Efficient Detection of Data Race Conditions via Adaptive Tracking. SOSP 2005, pp. 221–234. | V-abs |
| TSan09 | K. Serebryany, T. Iskhodzhanov. ThreadSanitizer – Data Race Detection in Practice. WBIA 2009, pp. 62–71. | V-abs |
| TSan11 | K. Serebryany, A. Potapenko, T. Iskhodzhanov, D. Vyukov. Dynamic Race Detection with LLVM Compiler. RV 2011, LNCS 7186, pp. 110–114. | V-abs |
| ClangTSanDoc | Clang documentation, ThreadSanitizer (overhead figures). | V-abs (search summary; page not opened) |
| LiteRace09 | D. Marino, M. Musuvathi, S. Narayanasamy. LiteRace: Effective Sampling for Lightweight Data-Race Detection. PLDI 2009, pp. 134–143. | V-abs |
| Pacer10 | M. D. Bond, K. E. Coons, K. S. McKinley. PACER: Proportional Detection of Data Races. PLDI 2010, pp. 255–268. | V-abs |
| DataCollider10 | J. Erickson, M. Musuvathi, S. Burckhardt, K. Olynyk. Effective Data-Race Detection for the Kernel. OSDI 2010. | V-abs |
| KCSANdoc | Linux kernel, Documentation/dev-tools/kcsan.rst (torvalds/linux master, fetched 2026-10-12, sha256 6fdf5914…). | V-doc |
| GoMem22 | The Go Memory Model, Version of June 6, 2022 (golang/go master doc/go_mem.html, sha256 a7372b4f…). | V-doc |
| CP12 | Y. Smaragdakis, J. Evans, C. Sadowski, J. Yi, C. Flanagan. Sound Predictive Race Detection in Polynomial Time. POPL 2012. | V-abs |
| MCM12 | T. F. Șerbănuță, F. Chen, G. Roșu. Maximal Causal Models for Sequentially Consistent Systems. RV 2012, LNCS 7687, pp. 136–150. | V-abs |
| RVPredict14 | J. Huang, P. O. Meredith, G. Roșu. Maximal Sound Predictive Race Detection with Control Flow Abstraction. PLDI 2014, pp. 337–348. | V-abs |
| WCP17 | D. Kini, U. Mathur, M. Viswanathan. Dynamic Race Prediction in Linear Time. PLDI 2017. | V-abs |
| SHB18 | U. Mathur, D. Kini, M. Viswanathan. What Happens-After the First Race? … OOPSLA 2018 (PACMPL 2, Art. 145). | V-abs |
| Vindicator18 | J. Roemer, K. Genç, M. D. Bond. High-Coverage, Unbounded Sound Predictive Race Detection. PLDI 2018. | V-abs |
| SDP19 / SmartTrack20 | Genç et al. OOPSLA 2019; Roemer et al. PLDI 2020. | P (named in a search summary only) |
| M2_20 | A. Pavlogiannis. Fast, Sound, and Effectively Complete Dynamic Race Prediction. POPL 2020. | V-abs |
| SyncP21 | U. Mathur, A. Pavlogiannis, M. Viswanathan. Optimal Prediction of Synchronization-Preserving Races. POPL 2021. | V-abs |
| GPredict15 | J. Huang, Q. Luo, G. Roșu. GPredict: Generic Predictive Concurrency Analysis. ICSE 2015, pp. 847–857. | V-abs |
| ExceptioNULL12 | A. Farzan, P. Madhusudan, N. Razavi, F. Sorrentino. Predicting Null-Pointer Dereferences in Concurrent Programs. FSE 2012. | V-abs |
| Lu08 | S. Lu, S. Park, E. Seo, Y. Zhou. Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics. ASPLOS 2008. | V-abs |
| AVIO06 | S. Lu, J. Tucek, F. Qin, Y. Zhou. AVIO: Detecting Atomicity Violations via Access Interleaving Invariants. ASPLOS 2006, pp. 37–48. | V-abs |
| CTrigger09 | S. Park, S. Lu, Y. Zhou. CTrigger: Exposing Atomicity Violation Bugs from Their Hiding Places. ASPLOS 2009, pp. 25–36. | V-abs |
| ConMem10 | W. Zhang, C. Sun, S. Lu. ConMem: Detecting Severe Concurrency Bugs through an Effect-Oriented Approach. ASPLOS 2010. | V-abs |
| DefUse10 | Y. Shi, S. Park, Z. Yin, S. Lu, Y. Zhou, W. Chen, W. Zheng. Do I Use the Wrong Definition? DefUse… OOPSLA 2010, pp. 160–174. | V-abs |
| Falcon10 | S. Park, R. W. Vuduc, M. J. Harrold. Falcon: Fault Localization in Concurrent Programs. ICSE 2010. | V-abs |
| Unicorn12 | S. Park, R. Vuduc, M. J. Harrold. A Unified Approach for Localizing Non-deadlock Concurrency Bugs. ICST 2012, pp. 51–60. | P |
| PSet09 | J. Yu, S. Narayanasamy. A Case for an Interleaving Constrained Shared-Memory Multi-Processor. ISCA 2009, pp. 325–336. | P |
| Bugaboo09 | B. Lucia, L. Ceze. Finding Concurrency Bugs with Context-Aware Communication Graphs. MICRO 2009. | V-abs |
| Recon11 | B. Lucia, B. P. Wood, L. Ceze. Isolating and Understanding Concurrency Errors Using Reconstructed Execution Fragments. PLDI 2011. | V-abs |
| Velodrome08 | C. Flanagan, S. N. Freund, J. Yi. Velodrome: A Sound and Complete Dynamic Atomicity Checker for Multithreaded Programs. PLDI 2008, pp. 293–303. | V-abs |
| AdHoc10 | W. Xiong, S. Park, J. Zhang, Y. Zhou, Z. Ma. Ad Hoc Synchronization Considered Harmful. OSDI 2010. | V-abs |
| Tian08 | C. Tian, V. Nagarajan, R. Gupta, S. Tallam. Dynamic Recognition of Synchronization Operations for Improved Data Race Detection. ISSTA 2008, pp. 143–154. | V-abs |
| WebRacer12 | B. Petrov, M. Vechev, M. Sridharan, J. Dolby. Race Detection for Web Applications. PLDI 2012, pp. 251–262. | V-abs (tool name UNVERIFIED) |
| EventRacer13 | V. Raychev, M. Vechev, M. Sridharan. Effective Race Detection for Event-Driven Programs. OOPSLA 2013, pp. 151–166. | V-abs |
| DroidRacer14 | P. Maiya, A. Kanade, R. Majumdar. Race Detection for Android Applications. PLDI 2014; SIGPLAN Notices 49(6):316–325. | V-abs |
| CAFA14 | C.-H. Hsiao, J. Yu, S. Narayanasamy, Z. Kong, C. L. Pereira, G. A. Pokam, P. M. Chen, J. Flinn. Race Detection for Event-Driven Mobile Applications. PLDI 2014, pp. 326–336. | V-abs |
| InitRacer17 | C. Q. Adamsen, A. Møller, F. Tip. Practical Initialization Race Detection for JavaScript Web Applications. OOPSLA 2017 (PACMPL 1, Art. 66). | V-abs |
| TaxDC16 | T. Leesatapornwongsa, J. F. Lukman, S. Lu, H. S. Gunawi. TaxDC… ASPLOS 2016. | V-abs |
| DCatch17 | H. Liu, G. Li, J. F. Lukman, J. Li, S. Lu, H. S. Gunawi, C. Tian. DCatch: Automatically Detecting Distributed Concurrency Bugs in Cloud Systems. ASPLOS 2017. | P (authors from a DBLP key; rules from a preprint excerpt) |
| FCatch18 | H. Liu, X. Wang, G. Li, S. Lu, F. Ye, C. Tian. FCatch: Automatically Detecting Time-of-Fault Bugs in Cloud Systems. ASPLOS 2018. | V-abs |
| CloudRaid18 | J. Lu, F. Li, L. Li, X. Feng. CloudRaid: Hunting Concurrency Bugs in the Cloud via Log-Mining. ESEC/FSE 2018, pp. 3–14. | V-abs |
| FlyMC19 | J. F. Lukman, H. Ke, … H. S. Gunawi. FlyMC: Highly Scalable Testing of Complex Interleavings in Distributed Systems. EuroSys 2019. | V-abs (full author list not checked) |
| RacePro11 | O. Laadan, N. Viennot, C.-C. Tsai, C. Blinn, J. Yang, J. Nieh. Pervasive Detection of Process Races in Deployed Systems. SOSP 2011. | V-abs |
| SimRacer13 | T. Yu, W. Srisa-an, G. Rothermel. SimRacer… DOI 10.1145/2483760.2483771 (ISSTA 2013). | P |
| SimExplorer17 | T. Yu, W. Srisa-an, G. Rothermel. STVR 2017. | V-abs |
| DESCRY17 | T. Yu, T. S. Zaman, C. Wang. DESCRY: Reproducing System-Level Concurrency Failures. ESEC/FSE 2017. | V-abs |
| SysPro26 | T. S. Zaman et al. SysPro… arXiv 2601.09616 (accepted in JSS). | V-abs (secondary) |
| Razzer19 | D. R. Jeong, K. Kim, B. Shivakumar, B. Lee, I. Shin. Razzer: Finding Kernel Race Bugs through Fuzzing. IEEE S&P 2019, pp. 754–768. | V-abs |
| SKI14 | P. Fonseca, R. Rodrigues, B. B. Brandenburg. SKI… OSDI 2014, pp. 415–431. | V-abs |
| Snowboard21 | S. Gong, D. Altınbüken, P. Fonseca, P. Maniatis. Snowboard… SOSP 2021. | V-abs |
| SDRacer17 | Y. Wang, F. Gao, L. Wang, T. Yu, K. Wang, J. Zhao, X. Li. ISSTA 2017; arXiv 2305.17869. | V-abs |
| RaceFuzzer08 | K. Sen. Race Directed Random Testing of Concurrent Programs. PLDI 2008, pp. 11–21. | V-abs |
| CHESS08 | M. Musuvathi, S. Qadeer, T. Ball, G. Basler, P. A. Nainar, I. Neamtiu. Finding and Reproducing Heisenbugs in Concurrent Programs. OSDI 2008. | V-abs |
| PCT10 | S. Burckhardt, P. Kothari, M. Musuvathi, S. Nagarakatte. A Randomized Scheduler with Probabilistic Guarantees of Finding Bugs. ASPLOS 2010. | V-abs |
| DPOR05 | C. Flanagan, P. Godefroid. Dynamic Partial-Order Reduction for Model Checking Software. POPL 2005, pp. 110–121. | V-abs |
| Narayanasamy07 | S. Narayanasamy, Z. Wang, J. Tigani, A. Edwards, B. Calder. Automatically Classifying Benign and Harmful Data Races Using Replay Analysis. PLDI 2007. | V-abs |
| Portend12 | B. Kasikci, C. Zamfir, G. Candea. Data Races vs. Data Race Bugs: Telling the Difference with Portend. ASPLOS 2012, pp. 185–198. | V-abs |

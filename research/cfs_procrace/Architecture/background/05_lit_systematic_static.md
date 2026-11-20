# 05: Systematic testing, model checking, static analysis and flight-software verification — a literature survey for a cFS process-level detector

- Date: 2026-10-12. Investigation only. No cFS tree was modified, nothing was installed, built or run against cFS, and nothing was committed.
- Pinned tree for code citations: `/home/user/work/procrace/cfs_ref/cFS` (nasa/cFS v7.0.1; cFE `c5fb2b4d`, OSAL `d2d877a6`, PSP `c4b3b0b6`). Code paths below are relative to that tree.
- Research records used: `research/cfs_procrace/CONDITIONS_POLICY.md`, `baselines/availability.md` and `baselines/yu-group.md`, `results/R2_gdb_v701.md` (as summarized in the task), and the user's design proposal v0.1 (`39d4fa4a-…20261008.md`) and status review (`473ecbab-…20261008.md`).
- Companion report: `04_lit_dynamic.md` covers the dynamic families: happens-before and lockset detectors, sampling, predictive analysis, event-driven and distributed race detection, and KCSAN. This report does not repeat them. It covers systematic testing, model checking, static analysis, embedded and RTOS analysis, flight-software verification, and the process-level baseline tools.

---

## 0. Method, labels and limits

### 0.1 How the citations were checked

| Channel | Result |
|---|---|
| `WebSearch` | Worked for all bibliographic checks in this report. The per-turn search budget ran out on the last two queries: BenchExec, and a re-check of Chopra et al. Those items are marked below. |
| `mcp__Scholar_Gateway__semanticSearch` | **Denied** (`ACCESS_DENIED`). |
| `WebFetch` | Failed with DNS errors (`getaddrinfo ENOTFOUND`) for static.usenix.org and people.cs.rutgers.edu. |
| `curl` to publisher and project hosts | **Blocked by the egress proxy** (`CONNECT tunnel failed, response 403`). Blocked hosts: ntrs.nasa.gov, usenix.org, dl.acm.org, plv.mpi-sws.org, hal.science, eprints.soton.ac.uk, doc.ic.ac.uk, cfs.gsfc.nasa.gov, software.nasa.gov, nasa.github.io, mlir.llvm.org, llvm.org, clang.llvm.org and discourse.llvm.org. I did not try to get around the block. |
| `raw.githubusercontent.com` | **Worked.** I read 15 primary tool and infrastructure documents in full and cached them in `scratchpad/arch/probe_lit05/docs/`. The table below lists them. `gh api` returned 403 (repository not enabled for this session) and was not used. |

Primary documents read (default branch as of 2026-10-12; first 12 hex digits of each sha256):

| File | Source | sha256 |
|---|---|---|
| `genmc_README.md` | MPI-SWS/genmc master README.md | `f4458a354e3f` |
| `nidhugg_README` | nidhugg/nidhugg master README | `893313526185` |
| `cbmc_README.md` | diffblue/cbmc develop README.md | `a5608ac082dd` |
| `esbmc_README.md` | esbmc/esbmc master README.md | `496c504a68e6` |
| `goblint_README.md` | goblint/analyzer master README.md | `f6d21b9700e4` |
| `svf_README.md` | SVF-tools/SVF master README.md | `05bf90abddc3` |
| `llvm_AliasAnalysis.md`, `llvm_AliasAnalysis.h` | llvm-project main `llvm/docs/AliasAnalysis.md`, `llvm/include/llvm/Analysis/AliasAnalysis.h` | `6047ba751f70`, `3e7daade2b18` |
| `mlir_DataFlowFramework.h`, `mlir_DataFlowAnalysis.md` | llvm-project main `mlir/include/mlir/Analysis/DataFlowFramework.h`, `mlir/docs/Tutorials/DataFlowAnalysis.md` | `c88ae6e60b35`, `92e3e5b40c2d` |
| `mlir_SMTDialect.td`, `mlir_SMT.td`, `mlir_ExportSMTLIB.h` | llvm-project main `mlir/include/mlir/Dialect/SMT/IR/…`, `mlir/include/mlir/Target/SMTLIB/ExportSMTLIB.h` | `96d6ab0f70ec`, `229135b91d4e`, `08d4ff7ebcda` |
| `cir_CIRDialect.td`, `cir_Passes.td` | llvm-project main `clang/include/clang/CIR/Dialect/IR/CIRDialect.td`, `clang/include/clang/CIR/Dialect/Passes.td` | `0b3cec4f3473`, `41f3ce625d4a` |

**Consequence.** I could not open the body of any paper. Each paper claim below comes from its abstract or publisher or author metadata, as quoted in search results. Where I state a guarantee that is only in a paper body, I mark it UNVERIFIED. Tool facts marked V-doc come from the README or source files listed above.

### 0.2 Verification levels (used in §8)

- **V-doc**: I read the primary text myself (a README, documentation page or source header).
- **V-abs**: the authors, title, venue and year, and the abstract-level claims, were confirmed by search results that quote a publisher, author, DBLP, institutional or conference page.
- **P**: partly verified. The detail that is not verified is named.
- **UNVERIFIED**: not checked. Nothing in a conclusion depends on it.

### 0.3 Labels in the text

- **FACT (paper / doc)**: the paper's abstract (V-abs) or the tool's own document (V-doc) says it.
- **FACT (code)**: the pinned cFS code says it (`path:line`).
- **FACT (record)**: one of our research records says it.
- **INFERENCE**: my reasoning. It was not executed or measured.

### 0.4 Corrections to the task's list and to common citations

| As commonly cited, or as in the task | Verified record |
|---|---|
| SDRacer as "ISSTA 2017" | ISSTA 2017 talk title: "Automatic Detection and Validation of Race Conditions in Interrupt-Driven Embedded Software". The extended version is arXiv 2305.17869 (2023), "Automatic Detection, Validation and Repair of Race Conditions in Interrupt-Driven Embedded Software" (Yu Wang, Fengjuan Gao, Linzhang Wang, Tingting Yu, Ke Wang, Jianhua Zhao, Xuandong Li). |
| SIMEXPLORER, STVR 2017 | The title is "An automated framework to support testing for process-level race conditions" (T. Yu, W. Srisa-an, G. Rothermel), STVR 27(4–5), e1634, 2017. |
| RRF, ISSRE 2016 | "RRF: A Race Reproduction Framework for Use in Debugging Process-Level Races", by **S. Rattanasuksun**, T. Yu, W. Srisa-an and G. Rothermel. Zaman and Wang are not authors. |
| SimEvo, ICSME 2017 | "SimEvo: Testing Evolving Multi-Process Software Systems", ICSME 2017, pp. 204–215. The search record lists **Tingting Yu as the sole author**; co-authors are unconfirmed. |
| ReDPro, ACMSE 2022 | **No record found.** A search on the DOI given in our baseline record (`10.1145/3476883.3520207`) returned nothing. UNVERIFIED. |
| SimRacer, ISSTA 2013 | The DOI `10.1145/2483760.2483771` comes from our baseline record and a springerprofessional listing. The exact ISSTA title is **not confirmed**. Status: P. |
| Regehr, "Thread verification vs. interrupt verification" (TV06) | DBLP indexes the TV06 entry as "Interrupt Verification via Thread Verification" (J. Regehr, N. Cooprider). The PDF on Regehr's page carries the other title. Status: P. |
| Miné, ESOP 2011 | The verified version is A. Miné, "Static Analysis of Run-Time Errors in Embedded Real-Time Parallel C Programs", LMCS 8(1:26), 2012. Its acknowledgements mention ESOP'11 reviewers. The ESOP 2011 title is not confirmed. |
| Brat et al., Mars rover V&V experiment, "FMSD 2004" | Confirmed as NASA Ames technical report NTRS 20040010327. The journal venue is **not confirmed**. |
| ESBMC | Verified for concurrency: Cordeiro & Fischer, ICSE 2011, pp. 331–340. The original ESBMC paper is not verified. |
| RacerD for C | The OOPSLA'18 abstract describes a **Java** analyzer. C/C++ support is UNVERIFIED. |
| SIERRA (static event-race detection, ASPLOS 2018) | The authors are **Yongjian Hu and Iulian Neamtiu**. |

### 0.5 Terminology: "sound" and "complete"

- **Static analysis and verification.** "Sound" means no missed bugs relative to the semantics; the analysis over-approximates. "Complete" means no false alarms. Abstract interpreters such as AstréeA and Goblint aim for soundness.
- **Bug finding, bounded model checking (BMC) and stateless model checking (SMC).** A counterexample is a real execution of the *model*, so there are no false alarms *in the model*. Results are complete only up to the bound. Qadeer and Rehof (V-abs) say context-bounded analysis is unsound overall but sound up to the bound.
- **Predictive analysis (report 04).** "Sound" means no false positives.
- **"Soundy"** (Livshits et al., CACM 2015, V-abs): mostly sound, with specific, well-identified unsound choices. This is the honest category for a C whole-program analyzer of cFS (§5).
- **This report** says "over-approximate relative to model M and assumptions A" or "complete up to bound b in model M". It never uses a bare "sound".

---

## 1. cFS facts that decide which technique applies

| # | Fact | Evidence | Consequence (INFERENCE) |
|---|---|---|---|
| F1 | Native cFS is one process. Every OSAL task is a pthread. | `osal/src/os/posix/src/os-impl-tasks.c:568` (`pthread_create`) | Thread-level tools (SMC, BMC, static race detectors) fit the substrate. Process-level tools that observe only syscalls do not see it (see `baselines/availability.md` §3). |
| F2 | Apps and libraries come from a **generated** startup script, not from source. | Parsing: `cfe/modules/es/fsw/src/cfe_es_apps.c:115`, called at `cfe/modules/es/fsw/src/cfe_es_start.c:201`. Generated script: `build-native_std/exe/cpu1/cf/cfe_es_startup.scr:1-17`. It has 2 `CFE_LIB` and 15 `CFE_APP` lines, for example `SAMPLE_APP_Main`, `CF_AppMain`, `SBN_AppMain`. | The set of threads is **configuration data**. A source-only analysis cannot know which apps exist, in what order or at what priority. |
| F3 | Modules are loaded with `dlopen` and entry symbols are resolved with `dlsym` by name. | `cfe_es_apps.c:499` (`OS_ModuleLoad`), `:522` (`OS_ModuleSymbolLookup`); `osal/src/os/portable/os-impl-posix-dl-loader.c:100` (`dlopen`); `os-impl-posix-dl-symtab.c:96` (`dlsym`) | To a pointer analysis, a `dlsym` result points to an unknown external object. |
| F4 | Every app's main task starts in a **generic trampoline**. The trampoline polls under a lock until its task record holds an AppId and an entry pointer. | `cfe_es_apps.c:663-669` creates the task with entry `CFE_ES_TaskEntryPoint`. The creator stores `TaskRecPtr->EntryFunc` at `:697`. The new task loops in `CFE_ES_GetTaskFunction` (`:559`): `OS_TaskDelay` at `:575`, reads `EntryFunc` at `:581`, checks the timeout at `:589` and `:595`; `CFE_ES_TaskEntryPoint` (`:613`) calls the real entry only if this succeeds (`:625`). The defaults are a 50 ms poll and a 1000 ms timeout (`cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h:859-860, 878-879`); no override was found in `sample_defs/`. | (a) A plain static call graph from `main` never reaches `SAMPLE_APP_Main` and the other app entries. The edge goes through `dlsym` → task table → loaded function pointer. (b) This is an **ad-hoc polling handshake**: at most 21 polls, by reading `:570-595`. Bounded tools need a loop bound for it, and here the bound comes from configuration, not from an arbitrary choice. |
| F5 | Core services start from a function-pointer table. | `cfe/modules/es/fsw/src/cfe_es_objtab.c:35-42` (`.EarlyInit`, `.TaskMain`); `CFE_ES_CreateObjects` at `cfe_es_start.c:741`, phases at `:751` and `:775`, `Entry->TaskMain` passed at `:810-814` | Resolving core-task entries needs a field-sensitive analysis of a `const` table. This is easy for a points-to analysis (constant initializers). |
| F6 | Plugins are loaded by table data, and their behaviour is reached through ops structs of function pointers. | SBN `LoadConf_Module`: `apps/sbn/fsw/src/sbn_app.c:1060-1094` (`OS_SymbolLookup` at `:1065` and `:1082`, `OS_ModuleLoad` at `:1075`). Ops fields: `apps/sbn/fsw/platform_inc/sbn_interfaces.h:71-443`, e.g. `:286` `SendNetMsg`, `:369` `PollPeer`, `:386` `Send`. | This is the hardest indirect-call case: the target is chosen by table content and reached through `dlsym`, then through struct fields. Type-based filtering (MLTA, §2.5) and configuration together are needed. |
| F7 | Callback registries are used. | `cfe/modules/time/fsw/src/cfe_time_api.c:614`, store at `:639`; TBL validation pointer at `cfe/modules/tbl/fsw/src/cfe_tbl_api.c:52`; OSAL iterator callbacks at `osal/src/os/shared/src/osapi-idmap.c:1321`, `:1392`; `CFE_ES_CreateChildTask` at `cfe/modules/es/fsw/src/cfe_es_api.c:1202` | These are more indirect calls, and some run in another task's context (TIME synch callbacks). |
| F8 | Name-based publish and discover, with a bounded retry. | `apps/cf/fsw/src/cf_cfdp.c:1270-1290` (`OS_CountSemGetIdByName` at `:1282`, retry plus `OS_TaskDelay`). Defaults: 25 retries (`apps/cf/fsw/inc/cf_internal_cfg.h:173`) at 100 ms (`:185`). | The comment at `:1272-1278` itself names "a start up race condition". The retry bound is a reference constant usable as an exploration bound (§5, §6). |
| F9 | The OSAL backend is chosen at build time. | `osal/src/os/{posix,rtems,vxworks,vxworks-rtp,qnx,shared,portable}`; `OSAL_SYSTEM_OSTYPE` at `osal/CMakeLists.txt:48` | The OSAL API is a narrow, documented concurrency interface. A model, or a controlled scheduler, can sit exactly there (§4). |
| F10 | Size, as physical lines (`wc -l`, comments included). | cFE `modules/*/fsw/src`: 39,658. `osal/src/os` (all ports): 26,147. `psp/fsw`: 16,024. `apps/*/fsw/src`: 53,521. The compile DB `build-native_std/native/default_cpu1/compile_commands.json` has 1,417 entries. | This is far below the 1–5 MLOC that RELAY, RacerX and MLTA report handling. Scale is not the obstacle; semantics (F2–F7) are. |
| F11 | Local toolchain. | Packages `llvm-18`, `libclang-cpp18`, clang 18.1.3. No LLVM/MLIR development headers, no `mlir-opt`, no Z3 Python module. None of SVF, Goblint, CBMC, ESBMC, GenMC or Nidhugg is on PATH. | Any prototype needs an environment decision, which is out of scope for this task (no installs). GenMC's README requires LLVM 19–22 (V-doc), so the installed LLVM 18 does not satisfy it. Nidhugg's README supports LLVM 8–19 (V-doc). |

**INFERENCE, summary.** Every family surveyed below assumes a *closed program*: the threads, entry points and inputs are known. In cFS, closing the program is itself a semantic task. It needs the startup script, the object table, the module and symbol map, and the platform constants (F2–F6). This is the first place where a cFS-specific contribution is needed and defensible.

---

## 2. Survey by family

Each subsection gives a table of works with their verified core claims, then guarantees, limits, applicability to cFS and what to borrow.

### 2.1 Systematic concurrency testing (stateless, bounded, randomized)

| Work | Core idea | Status |
|---|---|---|
| **VeriSoft** (Godefroid, POPL 1997, pp. 174–186) | Systematic state-space exploration of C/C++ processes that controls and re-executes the processes. This is the origin of *stateless* model checking. | V-abs |
| **Iterative context bounding** (Musuvathi & Qadeer, PLDI 2007) | Explore executions in order of the number of *preempting* context switches. Bounding preemptions controls state explosion without limiting execution depth. In the PLDI version, every one of the 9 bugs found was exposed with at most 2 preemptions (the MSR TR version reports 7 bugs). | V-abs |
| **CHESS** (Musuvathi, Qadeer, Ball, Basler, Nainar, Neamtiu, OSDI 2008) | Takes control of thread scheduling and searches the interleavings. Every bug found comes with a reproducing execution. It was used in daily testing at Microsoft. | V-abs. How CHESS intercepts APIs and what it requires of tests: UNVERIFIED (paper body not opened). |
| **Fair stateless model checking** (Musuvathi & Qadeer, PLDI 2008, pp. 362–371) | Earlier SMC handled only safety on *terminating* programs. A strongly fair but still nondeterministic scheduler lets SMC handle non-terminating programs and find all livelocks in finite-state programs. Singularity was booted under CHESS. | V-abs |
| **Delay bounding** (Emmi, Qadeer, Rakamarić, POPL 2011) | Models the scheduler as deterministic with a bounded number of "delays" (skips). The cost does not depend on the number of tasks or context switches, and dynamic task creation is supported. | V-abs |
| **PCT** (Burckhardt, Kothari, Musuvathi, Nagarakatte, ASPLOS 2010; SIGPLAN Notices 45(3):167–178) | A randomized priority scheduler. One run of a program with n threads and k steps finds a bug of depth d with probability **≥ 1/(n·k^(d−1))**. Bug depth is the minimum number of scheduling (ordering) constraints needed to expose the bug. | V-abs. The mechanism (d−1 random priority-change points) comes from a secondary source: P. |
| **SCTBench study** (Thomson, Donaldson, Betts, PPoPP 2014, pp. 15–28; journal version TOPC 2(4), 2016) | Independent empirical study of 52 buggy benchmarks (49 in TOPC). Most bugs need a bound of 1–3, and at least one needs 5 preemptions. Delay bounding beats preemption bounding, and both beat unbounded DFS. A naive random scheduler is surprisingly strong. In TOPC, **PCT with d=3 found every bug the other techniques found plus 3 more, and missed one.** | V-abs |
| **P# methodology** (Deligiannis, McCutchen, Thomson, Chen, Donaldson, Erickson, Huang, Lal, Mudduluru, Qadeer, Schulte, FAST 2016) | Systematic testing of message-passing services against high-level specifications, using controlled scheduling of asynchronous actors. Applied to three Azure storage systems; bugs were found, reproduced and fixed. | V-abs |

- **Guarantees.**
  - Systematic exploration with a bound is exhaustive up to the bound, *in the controlled program* (the context-bounding papers, V-abs). It says nothing beyond the bound.
  - PCT's guarantee is probabilistic per run. It holds only for bugs whose depth is d, and n and k must be known or estimated.
- **Limits.**
  - These methods need a controllable, repeatable test: closed inputs and deterministic replay.
  - They insert scheduling points only at operations the tool controls. A shared-memory store between two such points is not interleaved (INFERENCE from the stateless model).
  - Non-terminating loops need fairness or bounds (fair SMC).
- **Applicability to cFS (INFERENCE).**
  - cFS startup is a natural "test" with closed inputs. The pain points are F4 and F8: polling loops with `OS_TaskDelay`, and run loops that never terminate (`CFE_ES_RunLoop`, see report 02).
  - Fair SMC addresses the polling loops. Without fairness, an explorer can starve the producer and report a spurious "timeout" violation.
  - Message pipes on SB look like P#'s actor inboxes. The P# methodology (specifications plus systematic scheduling of message delivery) is the closest analog for SB-mediated ordering.
- **Borrow.**
  - **Preemption and delay bounding** as the exploration budget, swept rather than fixed (SCTBench norm).
  - **PCT** for real-binary validation, because it carries a stated probability bound.
  - **Fairness** for polling loops.
  - **Scheduling points at the OSAL API boundary**, the analog of CHESS's synchronization interception. Here F9 makes this architecturally clean.

### 2.2 Partial-order reduction and stateless model checkers for C

| Work | Core idea | Status |
|---|---|---|
| **DPOR** (Flanagan & Godefroid, POPL 2005, pp. 110–121) | Start from one arbitrary interleaving, track interactions between threads at run time, and add backtracking points only where dependent transitions could be reordered. This gives large reductions where static POR gives little. | V-abs. The formal guarantee (deadlock and assertion preservation for acyclic state spaces) is in the paper body: UNVERIFIED. |
| **Source-set and optimal DPOR** (Abdulla, Aronis, Jonsson, Sagonas, POPL 2014) | Source sets replace persistent sets. Wakeup trees make the algorithm **provably optimal**: a minimal number of executions, one per equivalence class. A JACM 2017 version names the algorithms source-DPOR and optimal-DPOR and extends them to disabling (locks). | V-abs (POPL). JACM details: P. |
| **Bounded POR** (Coons, Musuvathi, McKinley, OOPSLA 2013, pp. 833–848) | Combining DPOR naively with a preemption bound is **unsound within the bound**. BPOR corrects for bound dependences and is proved to give sound coverage. | V-abs |
| **MCR** (Huang, PLDI 2015) | Maximal causality reduction: a provably minimal number of executions, each from a distinct maximal causal model of a trace, built with constraints. Orders of magnitude fewer executions than iterative context bounding (ICB) or ICB+DPOR. SC only; an OOPSLA 2016 follow-up covers TSO and PSO. | V-abs |
| **Nidhugg** (Abdulla, Aronis, Atig, Jonsson, Leonardsson, Sagonas, TACAS 2015, LNCS 9035, pp. 353–367) | SMC for C/pthreads at the LLVM IR level under SC, TSO and PSO, using "chronological traces". | V-abs. Pages: P (secondary citation). README V-doc: supports SC, TSO, PSO, POWER and ARM; "Target programs should use pthreads … and each thread should be deterministic when run in isolation"; LLVM 8–19. |
| **GenMC** (Kokologiannakis, Raad, Vafeiadis, PLDI 2019; tool paper Kokologiannakis & Vafeiadis, CAV 2021) | SMC parametric in the memory model (RC11, IMM, LKMM). Sound, complete and optimal under stated conditions on the memory model. Works on LLVM IR. The tool paper adds POSIX I/O system calls, barriers and better error detection. | V-abs. README V-doc: C/C++ and Rust; LLVM 19–22; "This repository mirrors an internal repository and is only updated periodically." |
| **TruSt** (Kokologiannakis, Marmanis, Gladstein, Vafeiadis, POPL 2022, PACMPL 6, Art. 49) | Optimal DPOR with **linear memory**: one execution per class, no stored state, parallelizable. Formalized in Coq for SC, TSO, PSO and RC11. | V-abs |

- **Guarantees (V-abs as stated).** Optimal DPOR (and GenMC under its conditions) explores exactly one execution per equivalence class. For a closed, terminating, bounded program, "no violation found" therefore means no violation in any execution of that program under the modelled memory model.
- **Limits.**
  - The program must be closed, with deterministic threads (Nidhugg README) and bounded loops (INFERENCE: an SMC run must terminate).
  - Real C state explodes with the number of threads and shared accesses; this is the reason for bounding.
  - External calls (I/O, `dlopen`, `mq_*`, time) need models (INFERENCE; GenMC's CAV'21 paper mentions POSIX I/O support).
- **Applicability to cFS (INFERENCE).**
  - Running Nidhugg or GenMC on the whole cFS executable is not realistic. There are about 20 threads, `dlopen` (F3), mqueues, timing loops (F4, F8), and a non-terminating steady state.
  - They are realistic on a **per-candidate, closed harness**: the relevant tasks' code paths plus OSAL/cFE API models, bounded by reference constants (F4, F8).
  - DPOR's equivalence classes give a principled reading of design v0.1's *fixed-order versus schedule-dependent* distinction within a bounded model. If every class violates the contract, the violation is fixed-order *in that model and bound*. If some classes violate and some do not, it is schedule-dependent.
- **Borrow.**
  - Optimal or source-set DPOR (or TruSt for memory) as the exploration core.
  - The BPOR lesson: when the explorer is bounded, use a bound-aware POR or the reduction is unsound within the bound.
  - The Nidhugg/GenMC choice to work at LLVM IR with native pthread semantics, transposed to OSAL semantics.

### 2.3 Bounded model checking and SMT encodings of concurrency

| Work | Core idea | Status |
|---|---|---|
| **CBMC** (Clarke, Kroening, Lerda, TACAS 2004, LNCS 2988, pp. 168–176) | BMC for ANSI-C. Unwinds the program and checks a SAT formula. | V-abs. README V-doc: C89, C99, most of C11, C17 and C23, and most gcc extensions; "performed by unwinding the loops … and passing the resulting equation to a decision procedure". |
| **Context-bounded model checking** (Qadeer & Rehof, TACAS 2005) | Checking concurrent boolean programs is undecidable in general but **decidable when the number of context switches is bounded**, even with unbounded parallelism. The analysis is unsound overall but sound up to the bound. | V-abs. LNCS volume: P. |
| **Lal & Reps** (CAV 2008; FMSD 2009) | Reduce concurrent analysis under a context bound K to *sequential* analysis. | V-abs |
| **Lazy-CSeq** (Inverso, Tomasco, Fischer, La Torre, Parlato, CAV 2014, LNCS 8559, pp. 585–602) | Lazy sequentialization of SC pthread C programs into a nondeterministic sequential program. It preserves reachability for all round-robin schedules up to a bound on rounds, and existing BMC tools serve as backends. It won the SV-COMP 2014 concurrency category and later medals. | V-abs |
| **ESBMC** (Cordeiro & Fischer, ICSE 2011, pp. 331–340) | SMT-based context-bounded model checking of multithreaded C with a model of pthread primitives. Three strategies: lazy (one interleaving per query), schedule recording (all interleavings in one formula), and under-approximation with widening. | V-abs. README V-doc: "Concurrent software (using the pthread API) is verified by explicitly exploring interleavings"; checks deadlock (pthread mutexes and condition variables only) and "Data races (i.e., competing writes)"; incremental BMC and k-induction. |
| **Partial orders for BMC** (Alglave, Kroening, Tautschnig, CAV 2013, LNCS 8044, pp. 141–157) | Encode concurrent executions as **partial orders** in integer difference logic, separately from per-thread data and control flow. Implemented in CBMC for SC, x86 and Power. Evaluated on core parts of PostgreSQL, the Linux kernel and Apache httpd. | V-abs |
| **Symbolic predictive analysis** (Wang, Kundu, Ganai, Gupta, FM 2009; FAC 23(6):781–805, 2011) | A symbolic causal model from source plus observed events. SMT checks whether any feasible permutation of a trace violates a property, with bounded context switches. | V-abs |

- **How the SMT encoding of happens-before works.** This is general background; the structure is as Alglave et al. describe it (V-abs), and the details are INFERENCE.
  - Each relevant event gets an integer clock variable.
  - Program order gives `c_a < c_b` constraints.
  - Each read is matched to the write it reads from, with "no intervening write" constraints.
  - Each synchronization (lock, semaphore, queue send/receive, thread spawn) adds order constraints.
  - The property is asserted negated.
  - **For design v0.1's classification, two queries per candidate suffice:** SAT(violation ∧ model) asks whether a violating execution exists, and SAT(¬violation ∧ model) asks whether a non-violating one exists. "Only the first is SAT" is fixed-order within the model; "both SAT" is schedule-dependent.
- **Limits.** Loop unwinding and the context bound must be chosen. Formulas grow with events and bounds (Alglave's encoding is cubic in the worst case, per a search summary: P). Library and OS calls need models.
- **Applicability to cFS (INFERENCE).** Good for *per-candidate* queries over a sliced model with reference-derived unwinding (21 polls for F4, 25 retries for F8). It is poor as whole-program BMC of cFS, for the same closure reasons as §2.2. SMT handles symbolic table and config inputs naturally (e.g. CF `sem_name` empty or non-empty), which is relevant for the *inactive path* class (C1 in the status review).
- **Borrow.** The partial-order (happens-before) SMT encoding as an alternative backend to explicit DPOR, the two-query classification, and ESBMC/CBMC-style pthread modelling, transposed to OSAL.

### 2.4 Static race detection and static ordering analyses

| Work | Core idea | Language / scale | Status |
|---|---|---|---|
| **RacerX** (Engler & Ashcraft, SOSP 2003; OSR 37(5):237–252) | Flow-sensitive, interprocedural lockset analysis. Infers which locks guard what and ranks errors by severity. | C. 1.8 MLOC in about 2–14 minutes; Linux, FreeBSD and a commercial OS. | V-abs |
| **Locksmith** (Pratikakis, Foster, Hicks, PLDI 2006, pp. 320–331) | Context-sensitive, constraint-based *correlation* analysis: is every shared location consistently protected by a lock? | C. Servers and Linux drivers; "modest number of false alarms". | V-abs |
| **Chord** (Naik, Aiken, Whaley, PLDI 2006) | Multi-stage static analyses that narrow the set of racing access pairs. | Java. Tens to hundreds of new bugs, few false alarms. | V-abs. DOI varies between sources. |
| **RELAY** (Voung, Jhala, Lerner, ESEC/FSE 2007) | Static, scalable race detection in which **unsoundness is modularized to a few identifiable sources**. | C. Linux kernel, about 4.5 MLOC (2.6.15). | V-abs. Algorithmic details beyond the abstract: UNVERIFIED. |
| **Goblint** (Vojdani, Apinis, Rõtov, Seidl, Vene, Vogler, ASE 2016, pp. 391–402) | Combines pointer and value analyses for race analysis. Handles dynamically allocated locks and locks in arrays. The abstract's key lesson: "**practical use … requires inferring environmental and domain-specific assumptions**". | C. Linux drivers. | V-abs. README V-doc: OCaml/opam build; example `./goblint tests/regression/04-mutex/01-simple_rc.c`. |
| **Goblint, thread-modular abstract interpretation** (Schwarz, Saan, Seidl, Apinis, Erhard, Vojdani, SAS 2021, pp. 359–383) | Thread-modular non-relational value analyses as abstractions of a local-trace semantics. Generalizes Goblint's analysis and Miné's, and gives a refinement more precise than both. | C. | V-abs |
| **Whoop** (Deligiannis, Donaldson, Rakamarić, ASE 2015, pp. 166–177) | **Symbolic pairwise lockset** analysis over driver entry points. It "can soundly detect all potential races" without reasoning about interleavings, then uses Corral to confirm. | C. 16 Linux 4.0 drivers; 1.5–20× faster than Corral alone. | V-abs |
| **RacerD** (Blackshear, Gorogiannis, O'Hearn, Sergey, OOPSLA 2018, PACMPL 2, Art. 144) | Compositional static race detection in Infer. Run at Facebook for over a year; more than 2,500 issues flagged before production. | Java. | V-abs |
| **RacerF** (Dacík & Vojnar, arXiv 2502.04905, 2025) | Frama-C plugin on the EVA abstract interpreter. Under-approximation finds races; over-approximation shows their absence. "Mostly heuristic without providing formal guarantees". | C, pthreads. | V-abs (arXiv). ECOOP 2025 LIPIcs venue: P (seen only as a URL in the results). |
| **SIERRA** (Hu & Neamtiu, ASPLOS 2018) | Static detection of Android **event-based races**. "Concurrency actions" reify threads, events and messages, and system and user actions, with a **statically derived happens-before order between actions**. | Android, 194 apps. On 20 apps: 29.5 true races and 3.5 false positives on average, against 4 found by a dynamic detector. | V-abs |
| **UBITect** (Zhai et al., incl. Yu Hao, Zhiyun Qian; ESEC/FSE 2020) | **Use-before-initialization** detection: a flow-sensitive type-qualifier analysis narrows the candidates, then symbolic execution checks them. | Linux 4.14. 190 reported, 78 true positives, 52 confirmed by maintainers. | V-abs |
| **Soundiness manifesto** (Livshits et al., CACM 58(2):44–46, 2015) | Whole-program analyses of real languages are inevitably unsound in places. State the unsound choices explicitly ("soundy"). | Position paper. | V-abs |

- **Guarantees.**
  - Lockset tools report accesses not consistently protected. Over-approximating tools such as Whoop and Goblint claim to find all *potential data races* relative to their models and assumptions.
  - None of them claims to find *order violations between correctly locked accesses*.
- **Limits.**
  - False positives from imprecise points-to and from synchronization the tool does not understand (ad-hoc flags, polling; see report 04 on ad-hoc synchronization).
  - Missed races from unmodelled entry points and environment. Goblint's own abstract warns about the environment.
- **Applicability to cFS (INFERENCE; consistent with `baselines/availability.md` §3).**
  - The five case dependencies live in user memory guarded by locks such as ES `SharedDataMutex`. A pure lockset detector would classify the ES table accesses in C5 as *protected* and stay silent. That is correct for data races and irrelevant to the readiness contract.
  - Lockset and thread-modular analyses are still useful as **sub-analyses**: to show that a state transition is published under a lock, and to compute may-values at a use site (e.g. "`CFE_SB_Global.AppId` may be undefined at this use").
  - The ordering question needs an **action-level happens-before** in the style of SIERRA, extended with cFS actions (startup phases, spawn, registration, publication, SB send and receive). The value question needs a **use-before-initialization** style analysis (UBITect) over *cross-task* state.
- **Borrow.**
  - Goblint's thread-modular abstract interpretation (flow-insensitive global invariants refined by local traces) for value facts at a use site.
  - SIERRA's action happens-before built statically.
  - UBITect's two-stage pattern: a cheap qualifier analysis selects candidates, then precise symbolic checking confirms them.
  - Whoop's per-entry-point pairwise check followed by a precise checker.
  - RELAY's and the manifesto's discipline of *enumerating* the unsound sources.

### 2.5 Pointer analysis for C, and the function-pointer problem

| Work | Core idea | Status |
|---|---|---|
| **Andersen** (PhD thesis, DIKU 1994, report 94/19) | Inclusion-based (subset), interprocedural points-to analysis for C. Cubic worst case. | V-abs |
| **Steensgaard** (POPL 1996) | Unification-based, flow-insensitive points-to analysis in almost linear time. | V-abs |
| **Hind** (PASTE 2001, pp. 54–61) | "Haven't we solved this problem yet?": a survey of open problems after about 75 papers and 9 theses. | P. The IBM listing has a mismatched byline. |
| **Hardekopf & Lin** (CGO 2011, best paper) | Staged, sparse flow-sensitive points-to analysis that scales to millions of LOC. | V-abs |
| **SVF** (Sui & Xue, CC 2016, pp. 265–266) | Interprocedural static value-flow analysis in LLVM. Takes points-to results (e.g. Andersen) and builds interprocedural memory SSA. | V-abs. README V-doc: "reusable abstractions, graphs, and solvers for analyzing LLVM IR"; supports LLVM-21 and LLVM-22; MTA module "value-flows for multithreaded programs (CGO'16)" and MSli on-demand slicing for multithreaded programs. |
| **FSAM** (Sui, Di, Xue, CGO 2016, pp. 160–170) | Sparse flow-sensitive points-to analysis for **multithreaded C (pthreads)**, built on SVF, using thread-interference analysis phases. On 10 programs: 12× faster and 28× less memory on average; it finished two programs where the baseline timed out at 2 h. | V-abs |
| **MLTA / TypeDive** (Lu & Hu, CCS 2019, pp. 1867–1881) | Refine indirect-call targets with **multi-layer type analysis**: the struct types that hold a function pointer constrain its targets more than its signature alone. Evaluated on Linux, FreeBSD and Firefox. | V-abs. Reduction figures: UNVERIFIED. |
| **LLVM alias analysis** (in-tree documentation) | `basic-aa` is an aggressive *local* analysis. `globalsmodref-aa` covers non-address-taken globals. `steens-aa` and `ds-aa` (Data Structure Analysis) "are available in the optional 'poolalloc' module. It is not part of the LLVM core." `AliasResult` is one of `NoAlias`, `MayAlias`, `PartialAlias`, `MustAlias`. | V-doc (`llvm_AliasAnalysis.md` L463–535; `llvm_AliasAnalysis.h` L92–105) |

- **Precision limits that matter for cFS (INFERENCE grounded in F3–F7).**
  1. **`dlsym` results (F3, F4, F6).** Any standard points-to analysis treats them as unknown external memory. A sound treatment would make every app entry point a possible target of `RealEntryFunc()`, or of every address-taken function of matching type. The usual "soundy" treatment drops the edge, which loses all app code. Neither is acceptable. **Resolve by configuration**: map the symbol name in the startup script (F2) to its definition in the module's object file. This is a static analog of `OS_ModuleSymbolLookup`, and it is the same step as "closing the program" in §2.2.
  2. **Signature-only matching for ops structs (F6).** Many SBN ops functions share signatures such as `SBN_Status_t (*)(SBN_PeerInterface_t *)`. MLTA-style struct-layer typing narrows the targets. Configuration narrows further, because the module table selects which plugin is loaded.
  3. **Callback registries (F7).** These are stored into global arrays indexed by AppId. A field-sensitive Andersen analysis resolves them as "any registered callback". Which app registered which callback is data, so the call graph should be indexed by actor.
  4. **LLVM core AA is intraprocedural.** It cannot provide the cross-task "may these two accesses touch the same object" answer. A whole-program points-to analysis (SVF or Goblint's own) is needed.
- **Borrow.**
  - SVF (Andersen, plus FSAM for threads) as the fact source on LLVM IR.
  - MLTA's layered typing as a filter.
  - Configuration-driven resolution of `dlsym`.
  - The standard precision metrics: number of indirect call sites, average targets per site, unresolved sites. These are the metrics MLTA and SVF papers evaluate with (INFERENCE from their stated goals).
  - Report unresolved sites as `unknown_effect`, as design v0.1 already proposes.

### 2.6 LLVM, MLIR and ClangIR as analysis infrastructure (maturity as of 2026-10)

| Item | What exists (FACT) | Status |
|---|---|---|
| **LLVM** (Lattner & Adve, CGO 2004, pp. 75–86) | SSA IR with a language-independent type system and typed address arithmetic, designed for lifelong analysis and transformation. | V-abs |
| **MLIR** (Lattner, Amini, Bondhugula, Cohen, Davis, Pienaar, Riddle, Shpeisman, Vasilache, Zinenko, CGO 2021, pp. 2–14) | Reusable, extensible compiler infrastructure: dialects, operations, regions, passes. | V-abs |
| **MLIR dataflow framework** | "A generic framework for writing data-flow analysis in MLIR. The framework consists of a solver, which runs the fixed-point iteration and manages analysis dependencies, and a data-flow analysis class" (`DataFlowFramework.h`). Sparse and dense analysis headers exist. The tutorial explains lattices with `uninitialized` and `top`/`overdefined` and a monotone `join`. | V-doc |
| **MLIR SMT dialect** | `SMTDialect.td`: name `smt`, summary "a dialect that models satisfiability modulo theories". Upstreamed from CIRCT in llvm-project commit `de67293` on **2025-04-11** (PR #131480). The initial commit had only the dialect, ops, types, attributes and tests; conversions were deferred to later PRs. `mlir/include/mlir/Target/SMTLIB/ExportSMTLIB.h` exists upstream ("Run the ExportSMTLIB pass"). | V-doc (files). Commit date and scope: V-abs (mailing list). Which conversions *into* SMT exist upstream: not verified. |
| **ClangIR (CIR)** | `CIRDialect.td` summary: "A high-level dialect for analyzing and optimizing Clang supported languages". Upstream `Passes.td` lists canonicalization, simplification, alloca hoisting, CFG flattening, goto solving, ABI/target/EH/call-convention lowering, lowering-prepare, idiom recognition and library optimization. **It lists no concurrency, alias or lifetime analysis pass.** | V-doc |
| ClangIR status | LLVM Discourse progress reports (2026): the incubator was frozen for new development from 2026-02-20. About 64% of incubator code was upstreamed (Jan 2026). About 97% of gcc-c-torture tests pass, with inline assembly the main gap. SingleSource pass rate was 97.5% on 2026-08-25. Remaining failures are mainly in libc++ and self-hosting. | P (search summaries of Discourse posts; posts not opened) |
| **Polygeist** (Moses, Chelini, Zhao, Zinenko, PACT 2021, pp. 45–59) | A C/C++ front end into MLIR (affine and polyhedral). | V-abs |
| Local environment | LLVM 18.1.3 runtime packages only. No MLIR. The SMT dialect landed after LLVM 18, so the installed toolchain cannot provide it (INFERENCE from the dates). | FACT (local) plus INFERENCE |

- **Assessment (INFERENCE).**
  - (a) In 2026 MLIR gives us a solid *container*: a custom dialect with verifiers, a dataflow solver, and an SMT dialect with SMT-LIB export.
  - (b) It gives **no** C-level points-to analysis, concurrency semantics or race analysis. ClangIR, the C-to-MLIR route, is maturing but upstream has no analysis passes for our purpose. So C facts must still come from Clang AST plus LLVM IR (plus SVF), as design v0.1 §5.1 already says.
  - (c) MLIR does not add detection power by itself; design v0.1 §5.2 says the same.
  - (d) Using MLIR requires a toolchain at least as new as LLVM 21 (INFERENCE from the 2025-04 commit date), which is an environment decision.
- **Borrow.** If MLIR is used: a *small* cFS semantic dialect (actors, resources, transitions, sync, contracts), the upstream dataflow solver for happens-before and state propagation, and `smt` plus ExportSMTLIB for the §2.3 queries. Otherwise an equivalent typed event graph with a direct SMT-LIB emitter gives the same analytical content.

### 2.7 Interrupt-driven, RTOS and embedded concurrency analysis

| Work | Core idea | Status |
|---|---|---|
| **Regehr (with Cooprider)**, TV06 (FLoC 2006) | Compares threads and interrupts for race-freedom verification. Proposes source-to-source transformations so that thread verifiers can check interrupt code. Interrupts are not threads: priority preemption is one-way, and equal priorities do not preempt. | P (title discrepancy, §0.4) |
| **Schwarz, Seidl, Vojdani, Lammich, Müller-Olm**, POPL 2011, pp. 93–104 | Static analysis of programs under priority-driven preemptive scheduling with the **immediate ceiling priority protocol**: data races between tasks of different priorities, transactional execution of procedures, affine equalities. The first author's dissertation (TUM 2014) places this work in OSEK. | V-abs (OSEK link: P) |
| **Miné**, LMCS 8(1:26), 2012 | Thread-modular abstract interpretation for run-time errors in embedded real-time parallel C (no recursion, no dynamic allocation). Prototype Thésée, applied to an industrial program. | V-abs |
| **AstréeA** (Miné & Delmas, EMSOFT 2015, pp. 65–74) | Sound, scalable analysis of concurrent embedded avionics C that "reports soundly all run-time errors with few false positives". Case studies include large **ARINC 653 and POSIX threads applications** and part of an OS. | V-abs |
| **Kroening, Liang, Melham, Schrammel, Tautschnig**, DATE 2015, pp. 229–234 (extended: TECS 17(2), 2017) | Symbolic execution for **nested interrupts**, building on encodings of execution traces of concurrent threads. Beats the source-to-source approach on embedded code and drivers. | V-abs (TECS: P) |
| **Sung, Kusano, Wang**, ASE 2017 | Modular abstract interpretation: analyze each interrupt handler as a sequential program, propagate the results, and iterate to a fixed point. 35 applications, 22,541 LOC. | V-abs |
| **SDRacer** (ISSTA 2017; arXiv 2305.17869, 2023) | Static analysis finds racing locations. Symbolic execution generates inputs and interrupt interleavings. A virtual platform forces interrupts at the racing points. The 2023 version adds repair. 9 real embedded C programs. | V-abs |
| **Chopra, Pai, D'Souza**, ESOP 2019 | Defines data races and happens-before for interrupt-driven kernels, using "disjoint blocks" for synchronizes-with. Lockset-based race analysis applied to the **FreeRTOS** kernel library. | V-abs (from the conference listing via search; a re-check was not possible because the search budget was exhausted) |
| **Mukherjee, Kumar, D'Souza**, VMCAI 2017 | Model checking for *high-level* races (atomicity violations) in the FreeRTOS kernel. The thesis summary reports 38 such races, 16 harmful. | P |
| **Tulsyan, Pai, D'Souza**, FSTTCS 2020 (LIPIcs 182, 57:1–57:20) | Static race detection for **RTOS applications** using an "occurs-in-between" relation. Handles priority changes, flag-based synchronization and interrupt disabling. | V-abs |
| **intAtom** (ISSTA 2022) and **intAtom-S** (Yu Tingting, Li Chao, Wang Boxiang, Chen Rui, Jiang Yunsong; J. Computer Research and Development 60(2):294–310, 2023) | Static atomicity-violation detection for interrupt-driven programs. intAtom-S builds on an **empirical study of 82 atomicity violations in aerospace embedded software**. It reports an 8.9% false-positive rate and 23 developer-confirmed defects. | intAtom-S: V-abs (the figures come from the Chinese abstract). intAtom authors: P. |
| **Embedded sanitizers** | No peer-reviewed embedded *race* sanitizer was verified in this pass. BaseSAFE (arXiv 2020) adds sanitizer-like *memory* checks to emulation-based firmware fuzzing. KCSAN is in report 04. | BaseSAFE: P. The rest: none found. |

- **Applicability to cFS (INFERENCE).**
  - Native cFS on Linux has no ISRs in the five cases. But cFS also targets RTEMS and VxWorks (F9, plus the `pc686_rtems5-test.mk` target in the bundle), where priorities are real and preemptive.
  - Two lessons carry over directly:
    1. Under SCHED_FIFO-like priorities, **priority is a partial synchronization**: a higher-priority ready task is not interleaved by a lower one on one CPU (Schwarz et al.; Tulsyan et al.). Our environment is SCHED_OTHER on 4 vCPUs (`env/BASELINE_MEASURE.md`), where it is not. Priority reasoning must therefore be **conditional on the platform record**. This matches design v0.1 §7.1.
    2. The **flag/polling synchronization** that Tulsyan et al. handle statically is exactly F4 and F8.
  - The thread-modular abstract interpretation line (Miné, AstréeA, Goblint) is the established sound-by-construction approach for *embedded C with POSIX threads*. AstréeA's ARINC 653 and POSIX case studies are the closest industrial analog to flight software.
- **Borrow.**
  - Priority and scheduling-policy-aware happens-before as a *platform-conditional* rule set.
  - Static treatment of flag and polling handshakes (Tulsyan) instead of treating them as unsynchronized.
  - Thread-modular abstract interpretation for value facts (Miné, Goblint).
  - SDRacer's pipeline shape (static candidates → input and interleaving generation → forced schedule on a platform) as precedent for design v0.1's static → explore → execute pipeline.

### 2.8 NASA and flight-software verification

| Work | What it shows | Status |
|---|---|---|
| **Havelund, Lowry, Penix**, "Formal analysis of a space-craft controller using SPIN", IEEE TSE 27(8):749–765, 2001 | SPIN analysis of the Remote Agent executive (Deep Space 1) found 5 errors, 4 important. NASA reports state that a **deadlock occurred in flight in May 1999** in a sibling subsystem, and that the error was **isomorphic to one found earlier**. A "clean-room" re-verification found it quickly. | V-abs (in-flight details from NASA/RIACS documents in search results: P) |
| **Visser, Havelund, Brat, Park, Lerda**, "Model checking programs", ASE Journal 10:203–232, 2003 | Java PathFinder: model checking Java bytecode with POR, slicing and abstraction. Applied to a Honeywell real-time avionics OS and a spacecraft controller model. | V-abs |
| **Brat et al.**, Mars rover V&V tools experiment (NASA Ames TR, NTRS 20040010327) | Controlled experiment comparing static analysis (PolySpace), runtime analysis and model checking with traditional testing, using **defects seeded from a profile of real CVS-log defects**, over about 400 hours of human use. | P (journal venue unconfirmed) |
| **Venet & Brat**, PLDI 2004, pp. 231–242 | C Global Surveyor: static array-bound checking on Mars mission flight software, up to 280 KLOC, "pointer intensive, heavily multithreaded". About 80% precision. | V-abs |
| **IKOS** (Brat, Navas, Shi, Venet, SEFM 2014) | NASA Ames abstract-interpretation framework for C/C++. DO-333 context. Buffer-overflow analysis on flight control systems. Open source and still maintained. | V-abs |
| **Holzmann & Joshi**, "Model-driven software verification", SPIN 2004, LNCS 2989, pp. 76–91 | Put **unmodified application C code** into a test harness and let SPIN drive it through its states; only the harness is written in the model checker's language. Modex model extraction is covered separately in a Holzmann–Ruys SPIN 2005 tutorial. | V-abs (Modex tutorial: P) |
| **Holzmann**, "Mars Code", CACM 57(2):64–73, 2014 | Reliability practice for the Curiosity flight software. A reader comment says critical components were checked for races with assertions and a model checker. | V-abs (race detail: P, from a comment only) |
| **Butterfield & Tuong**, "Applying Formal Verification to an Open-Source Real-Time Operating System", LNCS 14080, pp. 348–366, 2023 | Promela/SPIN models of **RTEMS** managers, used to *generate tests* for ESA qualification. RTEMS is a cFS target OS. | V-abs |
| NASA/TM-20205011566 (NESC-RP-20-01515), "Cyclomatic Complexity and Basis Path Testing Study", 2020 (incl. G. J. Holzmann) | An NESC assessment. A secondary summary says it assessed the **Core Flight Software** and found low complexity but that not all paths were tested. | V-abs (record). cFS finding: P (secondary). |
| NASA/CR-20205010026, "Open Source Core Flight System (cFS) Flight Software (FSW) Final Report", 2020 | A cFS V&V review and tailoring template. Notes the expansion of cFS into Class A use. | P (title from the NTRS URL; authors not confirmed) |
| arXiv 2608.14532 (2026), "Trust Without Boundaries: An Architectural Analysis of Satellite Flight Software" | Decomposes cFS into execution, identity, communication, observability and persistence elements, and tests trust boundaries in NOS3. A security architecture study, not a concurrency study. | P (authors not retrieved) |
| Ogma / Copilot (NASA) | Ogma generates cFS applications that host Copilot runtime monitors generated from FRET requirements. | P (tool documentation; no peer-reviewed paper verified) |

- **Finding (FACT, about the search).** I found **no peer-reviewed publication on concurrency verification of cFS, cFE or OSAL**, whether by model checking, static race analysis or systematic testing. The nearest items are the generic flight-software work above, the RTEMS work, and NASA reports on cFS process, complexity and V&V. This is a gap our paper can name. It must be phrased as "we found none", not "none exists".
- **Applicability (INFERENCE).** The NASA lineage supports four of our choices:
  1. Model checking finds real flight concurrency bugs that testing misses (Remote Agent).
  2. Harnessing unmodified C beats re-modelling by hand (Holzmann & Joshi). This supports "no messy code generation": generate only the *environment/harness*, not a re-implementation.
  3. Seeded faults drawn from a real defect profile are an accepted NASA evaluation method (Brat et al.).
  4. Abstract interpretation at mission scale is established (CGS, IKOS).
- **Borrow.** The harness-around-unmodified-code principle, the seeded-defect evaluation drawn from real defect history, and the Remote Agent narrative as motivation, with its in-flight details marked P.

### 2.9 Process-level race tools in our baseline list

These summaries come only from verifiable sources. They are consistent with `baselines/availability.md`, and none of the tools was run.

| Tool | Verified bibliographic record | Verified core claim | Event model (status) | Relation to the families above (INFERENCE) |
|---|---|---|---|---|
| **RacePro** | Laadan, Viennot, Tsai, Blinn, Yang, Nieh, "Pervasive Detection of Process Races in Deployed Systems", SOSP 2011 | Records deployed executions; finds races among concurrent **kernel-object** accesses; replays alternative orderings to find harmful ones; Linux; modest recording overhead | Kernel objects and syscalls (V-abs; code-derived per `racepro.md`) | Dynamic detection plus *replay validation*. The same pipeline shape as ours, at the syscall layer. |
| **SimRacer** | ISSTA 2013, DOI 10.1145/2483760.2483771 (title unconfirmed) | Computes potential races from traces of existing tests, then **controls process scheduling** around them; runs on a commercial virtual platform; 16 applications | Inferred to be syscalls (`yu-group.md`) | Trace-guided controlled scheduling, a CHESS/PCT relative at process level. |
| **SIMEXPLORER** | Yu, Srisa-an, Rothermel, STVR 27(4–5) e1634, 2017 | Dynamic analysis finds program locations of interest and oracle faults; virtualization provides controllability; 24 applications | Excerpt only | Same as SimRacer. |
| **RRF** | Rattanasuksun, Yu, Srisa-an, Rothermel, ISSRE 2016 | Reproduces *user-reported* process-level races using static analysis, kernel event reporting and **yield points** | Kernel events (V-abs) | Yield points are a form of schedule forcing. Applicable in principle at OSAL call sites. |
| **DESCRY** | Yu, Zaman, Wang, ESEC/FSE 2017, pp. 694–704 | Reproduces system-level concurrency failures from **default logs only**, using static and dynamic analysis and symbolic execution, with deterministic replay on a virtual platform; 22 multi-process Linux applications, 236,875 LOC | Syscalls with fork, wait, pipe and signal relations (paper read in full per `descry.md`) | Symbolic execution plus schedule synthesis: an SDRacer relative for processes. |
| **SysPro** | Zaman, Yan, Wang, Islam, Shi, Yu, arXiv 2601.09616 (2026; "Accepted in JSS" per the arXiv comment) | Extracts syscall names from bug reports, locates them by static analysis, generates inputs, reproduces with dynamic binary instrumentation | Syscalls (V-abs) | Report-guided reproduction. |
| **SimEvo** | "SimEvo: Testing Evolving Multi-Process Software Systems", ICSME 2017, pp. 204–215 (author list as in §0.4) | Impact analysis of **system-level concurrent events** affected by a change; reuses and generates tests; called "the first regression testing techniques for multi-process applications" | Not stated | Regression and change-impact; relevant to evaluating across cFS versions. |
| **ReDPro** | UNVERIFIED | Not filled (roadmap §4.6) | — | — |

- **INFERENCE.** All verified process-level tools observe **syscall or kernel-object events** and control **process-level scheduling** on a virtual or instrumented platform. Methodologically they are dynamic or hybrid (trace → candidates → forced schedules), not static or model-checking tools. The cFS dependencies live in user-space memory inside one process (F1). This is the gap the baseline record documents, and it explains why the closest *techniques* for us come from §2.1–§2.4, not from the process-race line.

---

## 3. Assumption matrix: what each family needs and what cFS provides (INFERENCE)

| Requirement | SMC (Nidhugg, GenMC) | BMC (CBMC, ESBMC, Lazy-CSeq) | Static race (Goblint, Locksmith, RELAY) | Abstract interpretation (AstréeA, IKOS) | SCT (CHESS, PCT) | What cFS provides |
|---|---|---|---|---|---|---|
| Closed program and entry points | Required | Required | Required (entry points) | Required | Required (test) | **Only via configuration** (F2–F6) |
| Bounded loops and termination | Required | Unwinding bound | Not required | Widening | Fairness or bound | Polling loops bounded by reference constants (F4: 21 polls; F8: 25 retries). Run loops are unbounded. |
| Thread API semantics | pthreads built in | pthreads model | pthreads model | POSIX/ARINC model | Wrapped APIs | **OSAL API** (F9) over pthreads, mqueues and semaphores |
| Indirect calls | Concrete at run time | Over-approximated | Points-to | Points-to | Concrete | `dlsym` plus ops structs plus registries (F3, F6, F7) |
| Time | Not modelled | Not modelled | Not modelled | Not modelled | Real time | `OS_TaskDelay` polls, timeouts, the TIME service |
| Property | Assertions, data races | Assertions, races, deadlock | Data races | Run-time errors | Assertions, crashes | **Readiness and order contracts** (design v0.1 §6). These must be written; none of the tools has them. |

The last row is the core point. Every family can *check* a property, but none supplies cFS's readiness contracts or happens-before rules. Those, together with closing the program by configuration, are where the research contribution lies.

---

## 4. What a cFS detector should borrow, mapped to design v0.1 stages (INFERENCE)

| Design v0.1 stage | Borrow from | Concretely |
|---|---|---|
| Inputs: source, build, configuration | SDV environment models (Ball et al., EuroSys 2006, V-abs: "SDV includes models of the OS and the environment of the device driver" plus API rules); Goblint's environmental-assumptions lesson; Holzmann & Joshi harness | A **configuration-derived launcher model**. Read the generated startup script (F2), the core object table (F5), the module tables (F6) and the platform constants (F4, F8). Every value is recorded `from-reference` per `CONDITIONS_POLICY.md`. |
| Clang/LLVM fact extraction | SVF (Andersen/FSAM), MLTA, LLVM AA | Whole-program points-to on the compile DB (F10), with **`dlsym` resolved by configuration** and ops structs filtered by type layers. Unresolved sites become `unknown_effect`. |
| cFS semantic model | Nidhugg/GenMC and ESBMC pthread semantics; SIERRA action happens-before; Chopra et al. synchronizes-with for kernels; Tulsyan et al. flag synchronization | **OSAL/cFE API operational semantics** as the single source of truth. Examples: `OS_TaskCreate` has a `spawn_enable` point before it returns (C5); `OS_*Create` publishes into the name registry (C1); mutex, binary-semaphore and count-semaphore give/take; queue put/get (SB pipes); `OS_TaskDelay` is a yield plus a time step, **not** synchronization; startup phases (`WaitForSystemState`) behave like barriers. Each is a happens-before rule with a code citation. |
| Configuration and fixed-order classification | Goblint thread-modular abstract interpretation; UBITect; constraint propagation | Value facts at use sites ("AppId may be undefined"), inactive-path pruning by configuration (C1 `sem_name` empty), and fixed-order proofs via program order plus phase barriers. |
| Bounded state and schedule exploration | Optimal/source DPOR, BPOR, fair SMC, delay and preemption bounding; Alglave-style happens-before SMT | Per candidate, on a sliced closed model. Bounds come from reference constants and are **swept**. The fixed-order versus schedule-dependent split comes from DPOR classes or the two SMT queries (§2.3). |
| Selective execution verification | CHESS-style interception; PCT; RRF yield points; SDRacer and DESCRY forced schedules | A **controlled scheduler at the OSAL boundary** (F9) that enforces a model counterexample's order of API events on the real binary. Plus PCT runs that carry a stated probability bound. Any OSAL change is a recorded `deviation`. |
| Report | RELAY and the soundiness manifesto; MLTA metrics | An explicit list of unsound choices, unresolved indirect calls, and bounds reached. |

**On "no messy code generation".**
- Two families need a closed C program: SMC on LLVM IR, and BMC. Feeding them requires a generated harness.
- The Holzmann–Joshi principle keeps that acceptable: generate **only** the environment and launcher, and run the *unmodified* app and core code.
- An alternative avoids C generation entirely: explore the semantic model itself (own DPOR over model events) or emit SMT. The cost is that model fidelity must then be argued (§5.3).
- This is a design decision for AR2.

---

## 5. An academically defensible combination, and the claims we can honestly make

### 5.1 The combination (INFERENCE)

1. **Configuration-closed program (soundy, enumerated assumptions).** Facts come from Clang, LLVM IR and SVF. The launcher comes from the generated configuration.
2. **Contract-driven candidate generation.** Readiness contracts come from API documentation and code (design v0.1 §6). For each contract, find producers and consumers. Prune using happens-before rules from OSAL/cFE semantics and Goblint-style value facts.
3. **Per-candidate bounded exploration.** Use DPOR-based SMC over a slice that keeps third-party synchronization (design v0.1 §7.3). The SMT happens-before encoding is an alternative. Classify each candidate as protected-in-model, fixed-order-in-model, schedule-dependent-in-model, or unknown (bound or timeout).
4. **Real-binary confirmation.** Use OSAL-level controlled scheduling to replay counterexamples, and PCT to search more widely with a probability bound. Instrumentation follows report 04.

This mirrors established, published pipeline shapes:
- SDRacer: static → symbolic and interleaving generation → forced execution.
- Whoop + Corral: over-approximate pairwise → precise confirmation.
- UBITect: cheap qualifier stage → precise symbolic stage.
- RacePro: candidate detection → replay validation.

That lineage is itself an argument for academic validity.

### 5.2 Claim templates we can make honestly

Let B be the analysed configuration (commits, compile DB, generated startup script, platform constants), M the OSAL/cFE semantic model (versioned, every rule cited), A the enumerated unsound choices, and b the exploration bounds.

- **C-1 (candidate over-approximation, soundy).** "For configuration B, model M and assumptions A, every pair of producer and consumer events of a contract K that M does not order is reported as a candidate or listed as `unknown_effect`."
  - This is the AstréeA/Goblint style of claim, weakened to "soundy" by A.
  - A must list at least: inline assembly and compiler built-ins; code outside the compile DB; `dlsym` targets resolved only by configuration; library functions taken from summaries; the memory model (SC assumed, design v0.1 §3.2); signals and exceptions; time abstraction.
- **C-2 (bounded completeness of classification).** "Within M, for each candidate slice and bounds b (e.g. polls ≤ 21 per F4, CF retries ≤ 25 per F8, preemptions ≤ p), the explorer enumerates every equivalence class of executions."
  - This is the optimal-DPOR guarantee, or SMT satisfiability, and with BPOR-style correction if p is bounded.
  - Therefore "violation in all classes" means *fixed-order in M under b*, and "violation in some classes" means *schedule-dependent in M under b*.
- **C-3 (no false alarms among confirmed reports).** "Every report labelled *confirmed* was reproduced on the unmodified binary of B, with only the declared scheduling control at the OSAL boundary, and with the contract oracle observed."
  - This holds by construction, like RacePro's and DESCRY's replay validation.
  - The perturbation introduced by the controller is reported.
- **C-4 (probabilistic search on the real binary).** "Each PCT run with n tasks and k steps detects a depth-d bug with probability ≥ 1/(n·k^(d−1))."
  - Report n and k as measured, and how they were measured. The bound applies only to bugs of depth ≤ d.

### 5.3 Claims we must not make

- "Sound" or "detects all races", with no qualification. The `dlsym`, assembly, library-summary and memory-model assumptions make this false.
- "No violation exists" from a bounded exploration (Qadeer & Rehof: unsound beyond the bound), or from a model whose fidelity was not checked against the binary.
- "Fixed-order" from repeated failing runs (design v0.1 §2.3 already says this). A fixed-order claim is valid only as C-2 in M under b, together with the ordering proof (program order plus phase barriers) that the classifier produced.
- Detection of ISR, DMA or multicore weak-memory effects. These are out of model; AstréeA and Miné handle some of them, and we do not.
- That existing process-level tools "fail" on our cases. They are `NOT_RUN` (`availability.md`).
- "Generality" from five cases alone. The open-tool claim requires running on the whole bundle, with every report triaged (§6).

### 5.4 How the recorded R2 observation fits (INFERENCE)

- The record says that in every boot, 24 SB events (ES 20, EVS 4) are emitted with SB's not-yet-published AppId and rejected by EVS, both with gdb and with an instrumented build.
- If M's startup-phase rules order ES's and EVS's calls before SB's AppId publication on every path in B, C-2 would label R2 *fixed-order in M under b*. Separately, the ordering proof would come from the core object table (F5) and the phase waits.
- That is exactly the case design v0.1 §8.5 wants handled first. It shows the classifier distinguishing a configuration-order defect from a schedule-dependent race, which no surveyed tool's output distinguishes directly.

---

## 6. Evaluation methodology norms in these communities

### 6.1 Benchmarks

- **Systematic concurrency testing.** SCTBench (52 and 49 buggy programs, V-abs) is the reference empirical set. Results are reported as bugs found versus bound and versus number of schedules, compared across DFS, preemption bounding, delay bounding, random and PCT.
- **Model checking.** SV-COMP (Beyer, TACAS 2024 report, LNCS 14572, pp. 299–329, V-abs). Per a search summary of the 2025 report, there are six specifications, including **data races** (P).
- **Dynamic race tools.** DataRaceBench (Liao, Lin, Asplund, Schordan, Karlin, SC 2017, V-abs) is a curated race suite for OpenMP, evaluated with Helgrind, TSan, Archer and Inspector.
- **Static tools.** Real systems at scale: the Linux kernel (RELAY, RacerX, UBITect), drivers (Locksmith, Goblint, Whoop) and Android apps (SIERRA).
- **INFERENCE for us.** None of these benchmarks contains cFS-style readiness contracts. A cFS evaluation needs its own subject set. The minimum is:
  - (i) the whole v7.0.1 bundle (15 apps plus core, F2), for openness;
  - (ii) the historical cases at their pinned revisions (`cases/ledger.md`);
  - (iii) pre-fix and post-fix pairs where fixes exist.

### 6.2 Ground truth

- **Developer-confirmed bugs.** Common practice in static work: UBITect (52 confirmed), intAtom-S (23), RacerD (production issues). All V-abs.
- **Seeded defects from a real defect profile.** The NASA rover experiment (P) seeded defects from the project's CVS defect profile.
- **Known historical bugs with fix commits.** SCTBench, DataRaceBench, and the process-race line (DESCRY: 22 applications, SimRacer: 16, SIMEXPLORER: 24).
- **INFERENCE for us.**
  - Label each case and each report with the design v0.1 classes (inactive, protected, fixed-order, schedule-dependent, unknown).
  - Use two independent raters with a disagreement log.
  - Treat **derived** cases (e.g. a role-based BP provider for C1) as seeded, never as original reproductions (status review §4).
  - Seeded mutations should be ordering mutations taken from the real fix history: move registration after spawn, drop a phase wait, empty a retry bound.

### 6.3 Baselines

- **Norms.** Compare against the strongest runnable tool of the same family on the same subjects. SCTBench compares bounding against random and PCT. MCR compares against ICB and ICB+DPOR. FSAM compares against a non-sparse flow-sensitive analysis. SIERRA compares against a dynamic detector.
- **INFERENCE for us.**
  - (a) Process-level tools stay `NOT_RUN` with their recorded blockers.
  - (b) Runnable general baselines need an installation decision with official-document references, per policy:
    - Goblint and/or RacerF (static data races);
    - ThreadSanitizer (dynamic; report 04);
    - Nidhugg, GenMC or CBMC/ESBMC on the *same* closed harnesses (to separate "harness quality" from "explorer quality");
    - repeated-boot stress testing and PCT (uncontrolled and randomized baselines).
  - (c) State clearly that data-race tools answer a different question (§2.4). Expected silence on protected readiness violations is a *finding about the question*, not about tool quality.

### 6.4 Ablations

- **Norms.** Precision features are switched off one at a time (Goblint/SAS'21 precision comparisons, FSAM versus baseline). Bounds are swept (SCTBench). Reduction strategies are compared (DPOR versus source/optimal DPOR versus BPOR).
- **INFERENCE for us.** Remove each of the following and measure the change:
  - configuration closure (F2–F6), which measures reachable app code and resolved indirect calls;
  - the OSAL/cFE happens-before rules, falling back to generic pthread semantics;
  - the contracts, falling back to generic data-race or assertion properties;
  - static pruning (explore all pairs);
  - DPOR (naive enumeration);
  - the bound sweep (p = 0..3+, polls up to the reference maximum).

### 6.5 Metrics, overhead and statistics

- **Static analysis.**
  - Number of warnings, true and false positives after triage (or sampled triage, with the sampling stated).
  - Analysis time and memory per stage, on stated hardware.
  - Indirect-call resolution statistics (§2.5).
- **Model checking.** Executions or classes explored, time per candidate, bound reached or not, and timeouts reported as *unknown*, never as safe.
- **Testing on the real binary.**
  - Repeated trials with a statistical treatment. Arcuri & Briand (ICSE 2011, V-abs) found randomness often mishandled in SE studies and give guidelines. Klees et al. (CCS 2018, V-abs) found problems in every one of the 32 fuzzing evaluations they examined; their own experiments, per a slide summary (P), used about 30 trials and a Mann-Whitney U test.
  - Time-to-first-detection distributions.
  - PCT's n and k as measured.
- **Instrumentation overhead and perturbation.**
  - Boot time to OPERATIONAL with and without instrumentation, and per-event cost.
  - Whether the instrumentation changes the order distribution being studied (status review §8.3). Report the R2 event counts with and without instrumentation, as the existing records already do for gdb versus the instrumented build.
- **Benchmarking hygiene.** Fixed CPU set and limits, warm and cold runs, environment manifest. BenchExec-style tooling is the SV-COMP norm; I could not verify the citation before the search budget ran out, so it is UNVERIFIED.
- **Environment caveats from our records.** These belong in threats to validity: 4 vCPU, SCHED_OTHER only, mqueue depth truncated to 10, startup queue overflows of 4–49 per run (`env/CONDITIONS.md`, `env/BASELINE_MEASURE.md`).

---

## 7. Open questions

1. **Harness generation versus model exploration.** Should bounded exploration run (a) existing SMC/BMC tools on a generated launcher plus unmodified code (Holzmann–Joshi style), or (b) our own DPOR/SMT over the semantic model? (a) gives stronger fidelity but needs C harness generation and a toolchain decision (GenMC needs LLVM ≥ 19). (b) avoids code generation, but model fidelity must be validated separately.
2. **Toolchain.** Is installing a newer LLVM (≥ 21 for the MLIR SMT dialect), SVF, Z3 and one SMC tool acceptable as a recorded `deviation` or with `from-reference` versions? Today only LLVM 18 runtime packages are present (F11).
3. **Controlled scheduler at the OSAL boundary.** Should it be a new OSAL OS-type backend (F9) or wrappers? Either changes the binary, so how should the perturbation be bounded and reported?
4. **Contract sources.** Which readiness contracts can be cited from cFE/OSAL API documentation at the pinned revisions, and which would be researcher-written? The latter must be labelled per design v0.1 §6.2.
5. **Quantification.** How many indirect call sites does the cFS bundle have, and how many does SVF resolve with and without configuration closure? This needs an approved measurement on the compile DB (F10). Not run here.
6. **Remaining citation checks** once the search budget or egress allows: ReDPro, SimRacer's ISSTA title, the Regehr/Cooprider title, Brat et al.'s venue, the authors of NASA/CR-20205010026, the authors of arXiv 2608.14532, BenchExec, intAtom (ISSTA 2022) authors, and DPOR's formal guarantee wording.
7. **Platforms.** Should RTOS-target semantics (RTEMS priorities, F9) be in scope for the paper, or explicitly out of model (§5.3)?

---

## 8. References with verification status

| Key | Reference | Status |
|---|---|---|
| VeriSoft97 | P. Godefroid. Model Checking for Programming Languages using VeriSoft. POPL 1997, pp. 174–186. | V-abs |
| ICB07 | M. Musuvathi, S. Qadeer. Iterative Context Bounding for Systematic Testing of Multithreaded Programs. PLDI 2007. | V-abs |
| CHESS08 | M. Musuvathi, S. Qadeer, T. Ball, G. Basler, P. A. Nainar, I. Neamtiu. Finding and Reproducing Heisenbugs in Concurrent Programs. OSDI 2008. | V-abs |
| FairSMC08 | M. Musuvathi, S. Qadeer. Fair Stateless Model Checking. PLDI 2008, pp. 362–371. | V-abs |
| DelayBound11 | M. Emmi, S. Qadeer, Z. Rakamarić. Delay-Bounded Scheduling. POPL 2011. | V-abs |
| PCT10 | S. Burckhardt, P. Kothari, M. Musuvathi, S. Nagarakatte. A Randomized Scheduler with Probabilistic Guarantees of Finding Bugs. ASPLOS 2010; SIGPLAN Notices 45(3):167–178. | V-abs (mechanism: P) |
| SCTBench14 | P. Thomson, A. F. Donaldson, A. Betts. Concurrency Testing Using Schedule Bounding: an Empirical Study. PPoPP 2014, pp. 15–28. TOPC 2(4), 2016: "Concurrency Testing Using Controlled Schedulers: An Empirical Study". | V-abs |
| PSharp16 | P. Deligiannis, M. McCutchen, P. Thomson, S. Chen, A. F. Donaldson, J. Erickson, C. Huang, A. Lal, R. Mudduluru, S. Qadeer, W. Schulte. Uncovering Bugs in Distributed Storage Systems during Testing (not in Production!). FAST 2016. | V-abs |
| DPOR05 | C. Flanagan, P. Godefroid. Dynamic Partial-Order Reduction for Model Checking Software. POPL 2005, pp. 110–121. | V-abs (guarantee wording: UNVERIFIED) |
| ODPOR14 | P. Abdulla, S. Aronis, B. Jonsson, K. Sagonas. Optimal Dynamic Partial Order Reduction. POPL 2014. | V-abs (JACM 2017 version: P) |
| BPOR13 | K. E. Coons, M. Musuvathi, K. S. McKinley. Bounded Partial-Order Reduction. OOPSLA 2013, pp. 833–848. | V-abs |
| MCR15 | J. Huang. Stateless Model Checking Concurrent Programs with Maximal Causality Reduction. PLDI 2015. | V-abs |
| Nidhugg15 | P. Abdulla, S. Aronis, M. F. Atig, B. Jonsson, C. Leonardsson, K. Sagonas. Stateless Model Checking for TSO and PSO. TACAS 2015, LNCS 9035, pp. 353–367. | V-abs (pages: P); README V-doc |
| GenMC19 | M. Kokologiannakis, A. Raad, V. Vafeiadis. Model Checking for Weakly Consistent Libraries. PLDI 2019. | V-abs; README V-doc |
| GenMC21 | M. Kokologiannakis, V. Vafeiadis. GenMC: A Model Checker for Weak Memory Models. CAV 2021. | V-abs |
| TruSt22 | M. Kokologiannakis, I. Marmanis, V. Gladstein, V. Vafeiadis. Truly Stateless, Optimal Dynamic Partial Order Reduction. PACMPL 6 (POPL 2022), Art. 49. | V-abs |
| CBMC04 | E. Clarke, D. Kroening, F. Lerda. A Tool for Checking ANSI-C Programs. TACAS 2004, LNCS 2988, pp. 168–176. | V-abs; README V-doc |
| CBMC-CS05 | S. Qadeer, J. Rehof. Context-Bounded Model Checking of Concurrent Software. TACAS 2005 (DOI 10.1007/978-3-540-31980-1_7). | V-abs (LNCS volume: P) |
| LalReps08 | A. Lal, T. Reps. Reducing Concurrent Analysis Under a Context Bound to Sequential Analysis. CAV 2008; FMSD 2009. | V-abs |
| LazyCSeq14 | O. Inverso, E. Tomasco, B. Fischer, S. La Torre, G. Parlato. Bounded Model Checking of Multi-threaded C Programs via Lazy Sequentialization. CAV 2014, LNCS 8559, pp. 585–602. | V-abs |
| ESBMC11 | L. Cordeiro, B. Fischer. Verifying Multi-threaded Software using SMT-based Context-Bounded Model Checking. ICSE 2011, pp. 331–340. | V-abs; README V-doc |
| Alglave13 | J. Alglave, D. Kroening, M. Tautschnig. Partial Orders for Efficient Bounded Model Checking of Concurrent Software. CAV 2013, LNCS 8044, pp. 141–157. | V-abs |
| Wang09 | C. Wang, S. Kundu, M. Ganai, A. Gupta. Symbolic Predictive Analysis for Concurrent Programs. FM 2009; FAC 23(6):781–805, 2011 (with R. Limaye). | V-abs |
| RacerX03 | D. Engler, K. Ashcraft. RacerX: Effective, Static Detection of Race Conditions and Deadlocks. SOSP 2003; OSR 37(5):237–252. | V-abs |
| Locksmith06 | P. Pratikakis, J. S. Foster, M. Hicks. LOCKSMITH: Context-Sensitive Correlation Analysis for Race Detection. PLDI 2006, pp. 320–331. | V-abs |
| Chord06 | M. Naik, A. Aiken, J. Whaley. Effective Static Race Detection for Java. PLDI 2006. | V-abs |
| RELAY07 | J. W. Voung, R. Jhala, S. Lerner. RELAY: Static Race Detection on Millions of Lines of Code. ESEC/FSE 2007. | V-abs |
| Goblint16 | V. Vojdani, K. Apinis, V. Rõtov, H. Seidl, V. Vene, R. Vogler. Static Race Detection for Device Drivers: The Goblint Approach. ASE 2016, pp. 391–402. | V-abs; README V-doc |
| Goblint21 | M. Schwarz, S. Saan, H. Seidl, K. Apinis, J. Erhard, V. Vojdani. Improving Thread-Modular Abstract Interpretation. SAS 2021, pp. 359–383. | V-abs |
| Whoop15 | P. Deligiannis, A. F. Donaldson, Z. Rakamarić. Fast and Precise Symbolic Analysis of Concurrency Bugs in Device Drivers. ASE 2015, pp. 166–177. | V-abs |
| RacerD18 | S. Blackshear, N. Gorogiannis, P. W. O'Hearn, I. Sergey. RacerD: Compositional Static Race Detection. PACMPL 2 (OOPSLA 2018), Art. 144. | V-abs |
| RacerF25 | T. Dacík, T. Vojnar. RacerF: Lightweight Static Data Race Detection for C Code. arXiv 2502.04905, 2025. | V-abs (ECOOP 2025 venue: P) |
| SIERRA18 | Y. Hu, I. Neamtiu. Static Detection of Event-based Races in Android Apps. ASPLOS 2018. | V-abs |
| UBITect20 | Y. Zhai, Y. Hao, …, Z. Qian, et al. UBITect: A Precise and Scalable Method to Detect Use-before-Initialization Bugs in Linux Kernel. ESEC/FSE 2020. | V-abs (full author list: P) |
| SDV06 | T. Ball, E. Bounimova, B. Cook, V. Levin, J. Lichtenberg, C. McGarvey, B. Ondrusek, S. K. Rajamani, A. Ustuner. Thorough Static Analysis of Device Drivers. EuroSys 2006, pp. 73–85. | V-abs |
| Soundiness15 | B. Livshits, M. Sridharan, Y. Smaragdakis, O. Lhoták, J. N. Amaral, B.-Y. E. Chang, S. Z. Guyer, U. P. Khedker, A. Møller, D. Vardoulakis. In Defense of Soundiness: A Manifesto. CACM 58(2):44–46, 2015. | V-abs |
| Andersen94 | L. O. Andersen. Program Analysis and Specialization for the C Programming Language. PhD thesis, DIKU, 1994. | V-abs |
| Steensgaard96 | B. Steensgaard. Points-to Analysis in Almost Linear Time. POPL 1996. | V-abs |
| Hind01 | M. Hind. Pointer Analysis: Haven't We Solved This Problem Yet? PASTE 2001, pp. 54–61. | P |
| HardekopfLin11 | B. Hardekopf, C. Lin. Flow-Sensitive Pointer Analysis for Millions of Lines of Code. CGO 2011. | V-abs |
| SVF16 | Y. Sui, J. Xue. SVF: Interprocedural Static Value-Flow Analysis in LLVM. CC 2016, pp. 265–266. | V-abs; README V-doc |
| FSAM16 | Y. Sui, P. Di, J. Xue. Sparse Flow-Sensitive Pointer Analysis for Multithreaded Programs. CGO 2016, pp. 160–170. | V-abs |
| MLTA19 | K. Lu, H. Hu. Where Does It Go? Refining Indirect-Call Targets with Multi-Layer Type Analysis. CCS 2019, pp. 1867–1881. | V-abs |
| LLVM-AA | LLVM documentation `llvm/docs/AliasAnalysis.md`; header `llvm/include/llvm/Analysis/AliasAnalysis.h` (llvm-project main, fetched 2026-10-12). | V-doc |
| LLVM04 | C. Lattner, V. Adve. LLVM: A Compilation Framework for Lifelong Program Analysis & Transformation. CGO 2004, pp. 75–86. | V-abs |
| MLIR21 | C. Lattner, M. Amini, U. Bondhugula, A. Cohen, A. Davis, J. Pienaar, R. Riddle, T. Shpeisman, N. Vasilache, O. Zinenko. MLIR: Scaling Compiler Infrastructure for Domain Specific Computation. CGO 2021, pp. 2–14. | V-abs |
| MLIR-DF | `mlir/include/mlir/Analysis/DataFlowFramework.h`; `mlir/docs/Tutorials/DataFlowAnalysis.md`. | V-doc |
| MLIR-SMT | `mlir/include/mlir/Dialect/SMT/IR/SMTDialect.td`, `SMT.td`; `mlir/include/mlir/Target/SMTLIB/ExportSMTLIB.h`; commit de67293 (2025-04-11, PR #131480). | V-doc (files); V-abs (commit) |
| ClangIR | `clang/include/clang/CIR/Dialect/IR/CIRDialect.td`, `clang/include/clang/CIR/Dialect/Passes.td`; LLVM Discourse ClangIR progress reports, Jan–Sep 2026. | V-doc (files); P (reports) |
| Polygeist21 | W. S. Moses, L. Chelini, R. Zhao, O. Zinenko. Polygeist: Raising C to Polyhedral MLIR. PACT 2021, pp. 45–59. | V-abs |
| Regehr06 | J. Regehr (and N. Cooprider). Thread Verification vs. Interrupt Verification / Interrupt Verification via Thread Verification. TV06 (FLoC 2006). | P |
| Schwarz11 | M. D. Schwarz, H. Seidl, V. Vojdani, P. Lammich, M. Müller-Olm. Static Analysis of Interrupt-Driven Programs Synchronized via the Priority Ceiling Protocol. POPL 2011, pp. 93–104. | V-abs |
| Mine12 | A. Miné. Static Analysis of Run-Time Errors in Embedded Real-Time Parallel C Programs. LMCS 8(1:26), 2012. | V-abs |
| AstreeA15 | A. Miné, D. Delmas. Towards an Industrial Use of Sound Static Analysis for the Verification of Concurrent Embedded Avionics Software. EMSOFT 2015, pp. 65–74. | V-abs |
| Kroening15 | D. Kroening, L. Liang, T. Melham, P. Schrammel, M. Tautschnig. Effective Verification of Low-Level Software with Nested Interrupts. DATE 2015, pp. 229–234. | V-abs |
| Sung17 | C. Sung, M. Kusano, C. Wang. Modular Verification of Interrupt-Driven Software. ASE 2017. | V-abs |
| SDRacer17 | Y. Wang, F. Gao, L. Wang, T. Yu, K. Wang, J. Zhao, X. Li. Automatic Detection and Validation of Race Conditions in Interrupt-Driven Embedded Software. ISSTA 2017; extended arXiv 2305.17869 (2023). | V-abs |
| Chopra19 | N. Chopra, R. Pai, D. D'Souza. Data Races and Static Analysis for Interrupt-Driven Kernels. ESOP 2019. | V-abs (search summary of the conference listing; first names not checked) |
| Mukherjee17 | S. Mukherjee, A. Kumar, D. D'Souza. Detecting All High-Level Dataraces in an RTOS Kernel. VMCAI 2017. | P |
| Tulsyan20 | R. Tulsyan, R. Pai, D. D'Souza. Static Race Detection for RTOS Applications. FSTTCS 2020, LIPIcs 182, 57:1–57:20. | V-abs |
| intAtomS23 | Yu Tingting, Li Chao, Wang Boxiang, Chen Rui, Jiang Yunsong. (intAtom-S) J. Computer Research and Development 60(2):294–310, 2023. | V-abs (English title not recorded) |
| intAtom22 | Precise and Efficient Atomicity Violation Detection for Interrupt-driven Programs via … ISSTA 2022. | P (title truncated; authors not checked) |
| BaseSAFE20 | BaseSAFE: Baseband SAnitized Fuzzing through Emulation. arXiv 2005.07797. | P |
| Havelund01 | K. Havelund, M. Lowry, J. Penix. Formal Analysis of a Space-Craft Controller Using SPIN. IEEE TSE 27(8):749–765, 2001. | V-abs (in-flight deadlock details: P) |
| JPF03 | W. Visser, K. Havelund, G. Brat, S. Park, F. Lerda. Model Checking Programs. Automated Software Engineering 10:203–232, 2003. | V-abs |
| BratRover04 | G. Brat et al. Experimental Evaluation of Verification and Validation Tools on Martian Rover Software. NASA Ames TR (NTRS 20040010327). | P |
| CGS04 | A. Venet, G. Brat. Precise and Efficient Static Array Bound Checking for Large Embedded C Programs. PLDI 2004, pp. 231–242. | V-abs |
| IKOS14 | G. Brat, J. A. Navas, N. Shi, A. Venet. IKOS: A Framework for Static Analysis Based on Abstract Interpretation. SEFM 2014. | V-abs |
| HolzmannJoshi04 | G. J. Holzmann, R. Joshi. Model-Driven Software Verification. SPIN 2004, LNCS 2989, pp. 76–91. | V-abs |
| MarsCode14 | G. J. Holzmann. Mars Code. CACM 57(2):64–73, 2014. | V-abs |
| RTEMS23 | A. Butterfield, F. Tuong. Applying Formal Verification to an Open-Source Real-Time Operating System. LNCS 14080, pp. 348–366, 2023. | V-abs |
| NESC20 | M. D. Squire, L. A. Maynard-Nelson, T. A. Brown, R. T. Crumbley, G. J. Holzmann, M. Jennings, et al. Cyclomatic Complexity and Basis Path Testing Study. NASA/TM-20205011566 (NESC-RP-20-01515), 2020. | V-abs (record); cFS findings: P |
| cFSCR20 | Open Source Core Flight System (cFS) Flight Software (FSW) Final Report. NASA/CR-20205010026, 2020. | P |
| TrustCFS26 | Trust Without Boundaries: An Architectural Analysis of Satellite Flight Software. arXiv 2608.14532, 2026. | P |
| RacePro11 | O. Laadan, N. Viennot, C.-C. Tsai, C. Blinn, J. Yang, J. Nieh. Pervasive Detection of Process Races in Deployed Systems. SOSP 2011. | V-abs |
| SimRacer13 | T. Yu, W. Srisa-an, G. Rothermel. (SimRacer) ISSTA 2013, DOI 10.1145/2483760.2483771. | P |
| SimExplorer17 | T. Yu, W. Srisa-an, G. Rothermel. An Automated Framework to Support Testing for Process-Level Race Conditions. STVR 27(4–5), e1634, 2017. | V-abs |
| RRF16 | S. Rattanasuksun, T. Yu, W. Srisa-an, G. Rothermel. RRF: A Race Reproduction Framework for Use in Debugging Process-Level Races. ISSRE 2016. | V-abs |
| DESCRY17 | T. Yu, T. S. Zaman, C. Wang. DESCRY: Reproducing System-Level Concurrency Failures. ESEC/FSE 2017, pp. 694–704. | V-abs |
| SysPro26 | T. S. Zaman, Z. Yan, C. Wang, C. Islam, J. Shi, T. Yu. SysPro: Reproducing System-level Concurrency Bugs from Bug Reports. arXiv 2601.09616 (accepted in JSS per arXiv). | V-abs |
| SimEvo17 | (T. Yu et al.) SimEvo: Testing Evolving Multi-Process Software Systems. ICSME 2017, pp. 204–215. | V-abs (authors: P) |
| ReDPro22 | ReDPro, ACMSE 2022 (DOI per baseline record). | UNVERIFIED |
| SVCOMP24 | D. Beyer. State of the Art in Software Verification and Witness Validation: SV-COMP 2024. TACAS 2024, LNCS 14572, pp. 299–329. | V-abs |
| DataRaceBench17 | C. Liao, P.-H. Lin, J. Asplund, M. Schordan, I. Karlin. DataRaceBench: A Benchmark Suite for Systematic Evaluation of Data Race Detection Tools. SC 2017, 11:1–11:14. | V-abs |
| Klees18 | G. Klees, A. Ruef, B. Cooper, S. Wei, M. Hicks. Evaluating Fuzz Testing. CCS 2018. | V-abs |
| ArcuriBriand11 | A. Arcuri, L. Briand. A Practical Guide for Using Statistical Tests to Assess Randomized Algorithms in Software Engineering. ICSE 2011, pp. 1–10. | V-abs |
| BenchExec | D. Beyer, S. Löwe, P. Wendler. Reliable Benchmarking: Requirements and Solutions. (STTT 2019?) | UNVERIFIED (search budget exhausted) |

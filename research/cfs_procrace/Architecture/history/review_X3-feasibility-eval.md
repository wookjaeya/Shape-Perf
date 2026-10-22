# Review X3: feasibility and evaluation of SPEC v1 (Kairos)

- Date: 2026-10-12. Role: adversarial reviewer. This review covers four questions:
  - Can each component be built with the stated tools in this environment, with sane effort?
  - Is observation perturbation bounded and measured?
  - Are the interfaces clean?
  - Would the evaluation convince a reviewer, and does it comply with `CONDITIONS_POLICY.md`?
- Inputs. The specification is cited as `SPEC:line`. Also used: [R01]–[R07]; [POL] `research/cfs_procrace/CONDITIONS_POLICY.md`; [LED] `cases/ledger.md`; [R2g] `results/R2_gdb_v701.md`; [R2c] `results/R2_cord_v701.md`; [BM] `env/BASELINE_MEASURE.md`; and the pinned tree `/home/user/work/procrace/cfs_ref/cFS`.
- Sibling reviews X1 (soundness) and X2 (cFS fidelity) were read so this review does not repeat them. Where an issue touches theirs, the overlap is noted.
- What I ran. Everything was read-only. Outputs are only under `design/x3_probe/`. Nothing in the cFS trees was modified, nothing was installed, and no git config was changed.
  - `/proc/sys/kernel/randomize_va_space`, and symbol lookups in `/proc/kallsyms`.
  - `nm` and `objdump` on the gcc-built `build-native_std/exe/cpu1/core-cpu1`.
  - A count of GEP forms in R07's probe IR (`probe_env/ir/cfe_sb_api.ll`).
  - `git -c safe.directory='*' log` on the pinned cFE submodule.
  - A gcc compile of a `kz_schema.h` excerpt under the cFS C99 flag set (`x3_probe/hdr.c`).
  - grep counts of API call sites.
- Labels:
  - **[F]** fact with a citation;
  - **[M]** measured, either here (marked "here") or in a record;
  - **[I]** inference, not executed.

---

## 0. Verdict

**The design is buildable with the stated toolchain, but not as specified in three places. The evaluation cannot yet show detection ability beyond R2.**

- **Feasibility.** R07's toolchain claims hold. Three designs fail outright or are much harder than the spec says:
  1. The BPF site lookup is keyed by the runtime instruction address. It breaks under ASLR (X3-M1).
  2. The soundness-critical mod-ref analysis is specified as a from-scratch Andersen analysis over opaque-pointer LLVM 23 IR, and it sits inside the minimum viable core. A field-based writer scan at the AST level yields the writer sets the R2 certificate needs at a fraction of the cost (X3-M4).
  3. The stage plan has dependency inversions. O8 at S4 needs the gated S7 explorer (X3-M5).
- **Perturbation.** It is measured, which is good. It is not bounded, and it is not carried into the evidence of any verdict. The chosen measures are the noisiest available. Persistent state left by confirmation runs contaminates later runs (X3-M2, X3-M3).
- **Evaluation.** Five gaps:
  - There is no false-negative measure and one natural case (X3-C1).
  - Every baseline is a self-reimplementation over Kairos's own abstraction, and no comparison metric is defined (X3-M6).
  - Ablations have no labelled outcome (X3-M8).
  - Triage at census scale is unplanned (X3-M7).
  - Several bounds are mislabelled under [POL] (X3-M9), and time budgets contradict the byte-for-byte replay claim (X3-M10).
- **Interfaces.** The KZ container is principled. The Φ predicate and pattern sublanguages are unspecified, and they are the interface all three interpreters must agree on (X3-M11).
- **Counts:** 1 critical, 11 major, 8 minor.

---

## 1. What holds (checked)

- **Static front end.** 448/448 TUs parse, the core IR links closed, and LibTooling links statically ([R07 §3.3–3.4] [M]). The 56.7 MB static driver is a real but acceptable cost.
- **Shared C schema header under gcc C99.** `x3_probe/hdr.c` contains `_Static_assert`s and the `kz_verdict` layout. gcc 13 compiled it with `-std=c99 -pedantic -Wall -Werror` (rc 0, [M] here). ADR-004 is therefore feasible for the gate objects too.
- **Out-of-line copies of `static inline` helpers exist at -O0** ([M] here, `nm core-cpu1`):
  - two local `CFE_ES_AppRecordSetUsed` copies (`0x1ce69`, `0x24b76`);
  - four local `CFE_ES_AppRecordIsMatch` copies;
  - API prologues are `endbr64; push %rbp; mov %rsp,%rbp` (e.g. `CFE_EVS_SendEventWithAppID` at `0x29c2e`).

  This closes the [U] at SPEC:861 and SPEC:1340: entry+4 probes work on each copy.
- **Kernel features the observer needs exist** ([M] here; presence in `/proc/kallsyms` only, not functionally probed):
  - `bpf_perf_link_attach`, `bpf_get_attach_cookie_pe`, `bpf_get_attach_cookie_uprobe_multi`;
  - `__x64_sys_uretprobe`, `arch_uretprobe_trampoline`;
  - `branch_emulate_op`, `push_emulate_op`;
  - `bpf_find_vma`.
- **Fix history is available offline** ([M] here). The cFE submodule has the full history: 2047 commits, not shallow. 23 commit subjects match `race|startup sync|order`; relevance is not triaged. Examples: `6932f1fe Fix #950, race condition in control request`; `58abd38d Fix #978, store task parameters in task record`. This is a ready source for seeded mutations.
- **The spec's posture is right in several places:**
  - honest `NOT_RUN` and no "safe";
  - five separate costs;
  - staged gates with a kill criterion;
  - distributions, not means;
  - interleaved groups;
  - cold/warm recorded.

---

## 2. Issues (most severe first)

### X3-C1 — CRITICAL — No false-negative measure, and no natural ground truth beyond R2

- **Sections:** §8.1–§8.5, §8.2, §7.5.
- **Problem.**
  - The metrics (SPEC:1615–1623) are instance counts, decided shares, triaged precision, confirmation shares, coverage, fidelity and cost. **None measures what Kairos misses.**
  - The development set (SPEC:1562) has exactly one natural, reproduced case on the target revision: R2. R2 is fixed-order, a deterministic dependency error.
    - C3 shares R2's resource family and is not independent ([LED §3.1]).
    - C1 is public-code-derived. C2 is role-derived, a synthetic control.
    - C3 and C5 need the 6.4.x platform, whose missing compile DB the spec rates "high" likelihood (SPEC:1782).
    - Both App-record windows are inferences (SPEC:1485–1487).
  - So the **schedule-dependent class has no natural ground truth on the target binary.** That class is the reason for the lock-reversing predictor, the explorer and the confirmation stack.
  - §7.5 lists seeded mutations (SPEC:1520), but no metric uses them.
  - The mutation kill rate (SPEC:1001) mutates certificates, not programs. It tests `kz proofcheck`, not the analysis it checks.
- **Failure scenario [I].** A Kairos that classifies every instance `protected` or `unknown`, except R2, scores the same on every §8.5 metric. Only the denominator of triaged precision would change. The reviewer's first question, "what share of real ordering bugs does it find?", has no answer.
- **Fix.**
  1. **Recall protocol on program mutations, frozen before F2:**
     - **Operators** ([R06] L673; [R05 §6.2]): remove a wait; move a publication after a spawn; swap object-table entries; empty a retry bound; drop a status check.
     - **History query:** keywords and date cut, over the offline cFE, OSAL and app histories. Re-create each fix's inverse on v7.0.1.
     - **Expected verdict:** taken from the fix's issue text and commit (POL reference kinds 1–2).
     - **Per-mutant result:** `detected` (non-protected class with the expected shape), `unknown(reason)`, or `missed`.
     - **Soundness failure:** a `protected` or `fixed-order` certificate on a mutant whose fix text contradicts it is a soundness failure and must be reported as one.
     - **Reporting:** per layer, with exact binomial CIs.
     - **Equivalent mutants:** a pre-registered exclusion rule, e.g. "mutated site `inactive(B)`, with its proof".
  2. **Pre-fix/post-fix pairs** as subjects where fixes exist ([R05 §6.1 (iii)], `05_lit_systematic_static.md:431`). The verdict must move in the direction the fix implies.
  3. **Time-to-first-detection** distributions for σ_E and confirmation runs ([R05 §6.5]).
  4. **Build budget.**
     - A full serial build is about 15 min ([R07] L423).
     - Disk has 18 GB free; one variant tree is about 0.8 GB ([R07 §6]).
     - Build mutants incrementally in a single variant tree. Archive per mutant only the binaries, the compile DB and the hashes.
     - Label all mutants synthetic, never "new bugs".

### X3-M1 — MAJOR — The BPF site lookup by runtime instruction address breaks under ASLR

- **Sections:** §5.3 (SPEC:1256), §2.3 `kz_raw_ev.site`, §5.2 (SPEC:1252).
- **Problem.**
  - ASLR is on: `randomize_va_space` = 2 ([M] here) and `mmap_rnd_bits` 28 ([R07] L77).
  - `core-cpu1` is a PIE ([R07 §4.1]). Apps are `dlopen`ed at random bases ([R07] L250).
  - The baseline memory reader already has to read each load base from `/proc/<pid>/maps` ([BM §2.3]).
  - A map keyed by the probed IP must therefore be filled per module after it is mapped. For app `.so` files this races the first execution: all 15 apps load within 2.2–3.3 ms and start their init at once ([BM] L278).
  - The gdb records ran with ASLR disabled ([R2g] L24), so they never showed the problem.
- **Fix.**
  1. Drop the IP-keyed map. Give each site an **attach cookie** equal to its site id.
     - Option 1: `BPF_LINK_CREATE` on each perf uprobe fd, with `bpf_get_attach_cookie` in the program.
     - Option 2: one `uprobe_multi` link per binary, with per-offset cookies.
     - The kernel symbols for both exist ([M] here). S1 must probe them functionally.
  2. Register by inode, system-wide, before launch. This removes the `.so` pre-registration question and its harmful fallback (X3-m4).
  3. Design the generic program within verifier limits [I].
     - The verifier accepts `pt_regs` (ctx) reads only at constant offsets. Copy a fixed register set at constant offsets, then select with bounded code.
     - Read stack slots and dereferences with `bpf_probe_read_user`. Read strings with a constant maximum.
  4. Write the helper prototypes by hand (no libbpf headers, [R07 §2.3]). Parse the BPF ELF with glibc `<elf.h>`. Neither needs an install.

### X3-M2 — MAJOR — Perturbation is measured but not bounded, not propagated, and measured with the noisiest outcomes

- **Sections:** §8.6 (SPEC:1625–1651), §5.10, §1.10 evidence (SPEC:349), §4.12 (SPEC:1126).
- **Problem.**
  1. **No link to verdicts.** No rule relates measured perturbation to the validity of a verdict. `observed(n of N)` and σ_E classes carry no perturbation qualifier.
  2. **Low-power measures.**
     - The primary measures are noisy. `PipeOverflowErrorCounter` ranged **4–49 in six identical runs** ([BM §3.3] L143–157), and `NoSubscribersCounter` 15–26.
     - Distribution tests on them have little power at any affordable N [I].
     - Tight measures that need no instrumentation are missing from the list:
       - CORE_READY→OPERATIONAL in cFE time: **103.2–104.6 ms** across nine runs. [R2c §4.4] (L114–120) already uses it as the overhead metric.
       - The app-load span: 2.2–3.3 ms ([BM] L278–279).
  3. **Undefined measures.** "The R2 use set" (SPEC:1643) and probe hits do not exist in groups A and B, which have no recorder.
  4. **Observer CPU.** The observer's drainer competes for the 4 vCPUs during the burst.
     - [BM §1.3] (L55) could not separate exactly this effect for a root-side hook.
     - Group B uses empty programs, so it has no drainer load.
  5. **Group D.** "C plus holds" pools different interventions into one distribution.
- **Fix.**
  1. **Split the measures:**
     - *uninstrumented-observable:* cFE-time intervals from the console, per-app init offsets, and counters read at stop. Use them for A against B, C′ and C.
     - *recorder-only:* the R2 use set, hits and drops. Use them for C′ against C and D.
  2. **Per-witness perturbation ratio, computed, not chosen.**
     - Numerator: the probe time added on the involved threads between the two deciding events, from the run's own hits times the S1 class costs.
     - Denominator: the code-derived separation that protects the other order. Examples: the 50 ms poll delay of `CFE_ES_GetTaskFunction` (`cfe_es_apps.c:559–596`); the ~100 ms core-serialization spacing ([R2g] L56); SCH_LAB's first tick, at least 1 s after init.
     - Report the ratio, with no threshold. [I] Most margins are tens of ms against sub-µs probes, so the ratio will usually be small and persuasive. The `SBNSubPipe` burst (47–49 ms after OPERATIONAL, [BM]) is the case to watch.
  3. **Label σ_E evidence as σ_E(plan P).** Claim equivalence to unobserved runs only on the measured uninstrumented marginals.
  4. **Record the observer's own CPU time and placement** in every run.
  5. **Compare group D per hold plan**, against C runs with the same plan hash.

### X3-M3 — MAJOR — Persistent state left by confirmation runs contaminates later runs and pools

- **Sections:** §1.7 Init, §4.13, §4.14 (SPEC:1180–1197), §8.6 controls (SPEC:1651).
- **Problem [F].**
  - An exit that does not go through `CFE_PSP_Restart(POWERON)` leaves the SysV segments in place. Such exits include:
    - `CFE_PSP_Panic` → `abort()` (`psp/fsw/pc-linux/src/cfe_psp_support.c:116–121`);
    - a PROCESSOR restart (`:60–79`, which keeps the SHM);
    - the harness killing a deadlocked run.
  - The boot record then carries `NextResetType = PROCESSOR` (`cfe_psp_memory.c:664, 687`), so the next boot is a PROCESSOR reset (`cfe_psp_start.c:412–434`).
  - Confirmation runs are built to cause such exits. A.2 lists `processor_reset_requested` and panic; A.4 uses a deadlock with a wait-for oracle.
  - The loop interleaves confirmation runs with observation runs (SPEC:1180–1185).
  - S0 is snapshotted (SPEC:1197, 1651) but never restored, and the record practice is "Not cleaned (policy)" ([R2g] L28).
  - Two further effects:
    - The first run after a VM reboot starts in a different order ([BM] L278–281).
    - `EEPROM.DAT`'s mtime changes at every boot ([BM] L268). A naive Init hash would therefore split every pool.
- **Fix.**
  1. **Normalization boot.** After every run that did not end with "Exiting cFE with POWERON Reset status":
     - run one unobserved boot that ends with the CI POWERON restart already used as E11(a) (`.github/workflows/build-run-app-reusable.yml` L209–214, [BM] L29). That restart deletes the segments (`cfe_psp_support.c:62–69`). It is reference-based and cleans nothing by hand.
     - verify with the persist snapshot that Init equals the canonical S0;
     - exclude the normalization boot from every pool.
  2. **Canonical Init hash:** content hashes only, no mtimes.
  3. **Cold runs** (the first after a VM reboot) form their own stratum and are never pooled.
  4. **D17** (stop method) must be decided with this in view.

### X3-M4 — MAJOR — The soundness-critical mod-ref is specified as the hardest possible build, inside the MVC

- **Sections:** §4.2 (SPEC:884–912; "field-based for struct-typed pointers", SPEC:889), §2.2, ADR-007, §9.3 S3.
- **Problem.**
  - The spec asks for an Andersen-style, field-sensitive inclusion analysis for C, written from scratch (SVF lists only LLVM 21–22, SPEC:439), over LLVM 23 IR. It gates the MVC's headline result, the R2 certificate.
  - IR facts ([M] here):
    - **Pointers are opaque** (`ptr noundef` in R07's probe IR), so "struct-typed pointers" has no referent in the IR.
    - Field identity survives only in GEP source element types. `cfe_sb_api.ll` has 94 `getelementptr inbounds nuw %struct.` instructions.
    - Field identity is lost once a canonicalizing pass such as InstCombine runs [I]. The spec pins no IR pass pipeline.
    - `CFE_ES_AppId_t` is a plain `i32` in this build (`%AppId = alloca i32`). The out-pointer writes that matter, such as `CFE_ES_GetAppID(&CFE_SB_Global.AppId)`, are therefore untyped scalar stores.
- **Failure scenario [I].** S3 slips behind the points-to implementation. Or a quick version loses field identity after a canonicalizing pass, every T-OBS becomes `DISABLED`, and no certificate exists.
- **Fix.** Split the work into S3a and S3b.
  - **S3a: field-based writer extraction at the Clang AST level.**
    - One abstract location per (struct type, field), independent of the base object. This is the standard meaning of "field-based".
    - Writers: `MemberExpr`→`FieldDecl` stores, compound assignments, increments, and `memset`/`memcpy` by pointee type.
    - Per-formal out-pointer store summaries.
    - An address-escape check per support location. If the address flows only into non-escaping formals, use the summary; otherwise mark the location `DISABLED`.
    - A listing of every `char*`, `void*` or cast write that may reach a support type. Each such write makes the location `DISABLED`.
    - This is sound under an explicit typed-access assumption **A-TYPE**, which the listing checks.
    - It covers `CFE_SB_Global.AppId` (address taken once, `cfe_sb_task.c:138`) and `AppTable[*].AppState`.
    - Keep LLVM IR for call-graph closure. If IR is used for writes, pin a pipeline without InstCombine or SROA.
  - **S3b: Andersen**, only if the O7 report shows `DISABLED` conjuncts blocking certificates. Measure its gain with ablation 17.

### X3-M5 — MAJOR — Stage plan: dependency inversions, no size estimates, and an MVC that is not minimal

- **Sections:** §9.3 (SPEC:1715–1731), §4.11 (SPEC:1122), §4.14 (SPEC:1180–1185), §10.1 D14 (SPEC:1758), §10.2.
- **Problem.**
  1. **Dependency inversions.**
     - S4 delivers the loop "with O1–O8" (SPEC:1725).
     - O8 is answered "with the explorer's engine constrained by the trace" (SPEC:1122). The explorer is S7, gated, and may be frozen (SPEC:1116).
     - The loop body calls `kz predict` (S5) and `kz hold`/`kz gate` (S6) (SPEC:1184–1185).
  2. **N comes too late.** S1's exit compares groups A, B and C′ (SPEC:1722), which needs N. N is decision D14, scheduled "before S9".
  3. **The MVC is large.** S0–S3 includes:
     - the whole L1 layer: extraction, the DWARF/MC binder, mod-ref, and skeletons with call-string inlining and loop summaries;
     - MustBefore and the certificates;
     - an independent checker with its own dominator and reachability code;
     - a mutation engine, and O1/O2.
  4. **No sizing.** No stage has a size or effort estimate. §10.2 rates "integration breadth" medium/high and gives no numbers. On top of the MVC come five reimplemented baselines, three more requested by X1-M12, a litmus suite on the real OSAL, and a golden test per rule.
- **Fix.**
  1. Re-scope S4 to O1–O7. Either mark O8 "deferred", or give it its own trace-constrained search built in S5's engine. State that S4's loop omits predict and hold until S5 and S6 exist.
  2. Move D14 to S1, at least for S1's characterisation N, recorded per X3-M9.
  3. Define a thin MVC: S0 + S1 + S2 + S3a (X3-M4). State what the paper can claim at each cut point.
  4. Add per-stage size and effort estimates, labelled [I], and a cut list ordered by claim value.

### X3-M6 — MAJOR — Baselines are self-reimplemented over Kairos's own abstraction, with no comparison metric; a runnable external reference point is omitted

- **Sections:** §8.3 (SPEC:1566–1580), ADR-025.
- **Problem.**
  - **Shared abstraction.** Every comparator runs "over the same traces" with Kairos's event abstraction: "lifecycle LPs as writes, uses as reads" (SPEC:1572). Each comparator's quality therefore depends on Kairos's own probe plan and LP choices.
  - **Outcomes written in advance.** The spec already states results ("expected: nothing for R2", SPEC:1572).
  - **No metric.** The last column of the table is "What it shows", not a metric.
  - **A runnable reference point is omitted.**
    - Valgrind 3.22 with helgrind and DRD is installed and ran a test program ([R07] L156, L346).
    - It can run the unmodified binary through the existing `RUN_CFS_WRAP` hook used for gdb ([R2g] L11, H-8). That is a smaller deviation than TSan, which needs a rebuild (D7).
    - It is excluded *as a baseline* by S13 ([R06] L123), but allowed as a labelled reference point.
  - **Misstated blocker for Nidhugg.**
    - Nidhugg's README range is LLVM 8–19 ([R05] L94, L142), and LLVM 18 is installed (runtime only).
    - The real blocker is an `llvm-18-dev` install decision, not incompatibility as SPEC:1580 implies.
    - R05's norm is to run such tools "on the same closed harnesses" ([R05] L452).
  - X1-M12 already asks for lock-reversing comparators; that request is not repeated here.
- **Fix.**
  1. **A common metric:** for each labelled instance, record alarm yes/no per comparator. Labelled instances are the development cases, the X3-C1 mutants and the X3-M7 triage sample. Compare per instance with paired tests; record the test choice as an assumption ([POL]).
  2. **Freeze comparator code at F1**, with golden tests taken from the original papers' worked examples. Record who implemented each comparator.
  3. **Run helgrind and DRD via `RUN_CFS_WRAP`** as a labelled reference point (deviation: wrapped run command, as for gdb).
  4. **Restate the Nidhugg/GenMC blocker** precisely. Optionally, after an install decision, cross-check the explorer with Nidhugg on the litmus programs that use only pthread create, join and mutex.

### X3-M7 — MAJOR — Triage of "every reported instance" by "two independent raters" is unplanned at census scale and partly ill-defined

- **Sections:** §8.5 (SPEC:1619), §7.1.
- **Problem.**
  - **Scale** ([M] here). By grep over `cfe/modules` and `apps`, excluding test and stub directories and not filtered by link membership, there are **1,208 `CFE_EVS_SendEvent` call sites** and **133 `CFE_SB_TransmitMsg`/`Subscribe*` call sites**, before context qualification. With context-qualified templates (ADR-012) and K-ROUTE pairs, instances will number in the thousands [I].
  - **"Reported" is undefined.**
  - **Raters.** Their identity, blinding and independence from the authors are unspecified.
  - **No "correct" label for some outcomes.** For `LOSS-UNDER-ORDER` and `not-ready-unjudged`, "correct" is undefined by construction, because Allowed is unspecified.
- **Fix.**
  1. Define "reported" as `fixed-order` or `schedule-dependent` with outcome label `violation`.
  2. Triage a stratified random sample per (class, RC kind, deciding layer). Record the sample size as a deviation or assumption ([POL]) and report exact CIs.
  3. Blind raters to the producing layer, to the ablation, and to each other.
  4. Check `LOSS-UNDER-ORDER` verdicts against the SB-counter oracle (agreement rate) instead of rating them.
  5. Report rater effort with the other costs.

### X3-M8 — MAJOR — Ablations have no labelled outcome measure, and several depend on the explorer

- **Sections:** §8.4 (SPEC:1582–1613).
- **Problem.**
  - Each ablation reports "what moved between classes" (SPEC:1608), the decided share and the cost. Without labels, a move cannot be judged better or worse.
  - Four ablations need S7, which may be frozen:
    - 1, no-trace ("≈ A"), which includes the explorer;
    - 12, no tier 1;
    - 18, L3 off;
    - 19, DPOR against naive enumeration.
  - The names "≈ A" and "≈ B" invite readers to take them as comparisons with designs that were never implemented as designed.
- **Fix.**
  1. Score every ablation on the labelled set: the X3-C1 mutants, the development cases and the X3-M7 sample. Report per-instance correctness deltas with paired tests and effect sizes.
  2. Mark the explorer-dependent ablations conditional on S7.
  3. Rename "≈ A" and "≈ B" to "static-only" and "dynamic-only".

### X3-M9 — MAJOR — Bounds, N and budgets are mislabelled under CONDITIONS_POLICY

- **Sections:** SPEC:12, §4.3 (SPEC:924), §4.10 item 6, §4.11 (SPEC:618), §8.6 (SPEC:1650), §8.7 (SPEC:1655), §10.1 D14/D15 (SPEC:1758–1759).
- **Problem [F].**
  - [POL] L17: `unspecified_by_reference` means that no value is chosen ("값을 고르지 않고"); if needed, it is marked as an assumption.
  - [POL] L18–21: a workaround is only ever a `deviation`, with its reason, its effect and why it is unavoidable.
  - The spec labels as `unspecified_by_reference` several values that someone then chooses:
    - N (SPEC:1655);
    - maximum budgets (SPEC:1759);
    - the O8 budget (SPEC:618);
    - sweep grids and the cut-off of h (SPEC:924);
    - the interleaving order (SPEC:1650).
  - [POL]'s reference kinds (L5–10) are case reports, fix commits and source, pinned official docs, and tool docs. Methodological literature is not among them. Arcuri & Briand and Klees can justify an assumption, never `from-reference`.
- **Fix.**
  1. Record PI-chosen caps and N as `deviation`:
     - reason: finite resources;
     - effect: results are bounded and reported as `unknown(budget)`;
     - necessity: no reference defines them.
  2. Prefer event-based stopping wherever it exists: exhaustion, fixed point, loop termination.
  3. Report anytime curves (decided share against work units), so readers can see each cap's effect.
  4. Record the statistical procedure as an assumption.

### X3-M10 — MAJOR — Time and memory budgets contradict the byte-for-byte replay claim (G8)

- **Sections:** §4.9 item 7, §4.10 item 6 (SPEC:1104), §4.11, §4.14 (SPEC:1195), §6.3 G8.
- **Problem.**
  - "Time, memory and execution budgets are … swept" (SPEC:1104). The predictor and O8 also run within budgets.
  - A wall-clock or RSS budget on a shared host ([R07 §1]) makes the explored frontier load-dependent.
  - Re-running from stored traces then cannot reproduce `unknown(budget)` results byte for byte. That contradicts SPEC:1195 and G8.
- **Fix.**
  1. Express every analysis budget in deterministic work units (states expanded, transitions executed, search nodes), with a deterministic search order.
  2. Record wall time and peak memory as costs only.
  3. If a wall-clock cap is ever needed (D15), label the results produced under it and exclude them from the replay test.

### X3-M11 — MAJOR — The Φ predicate and pattern sublanguages, the interface of all three interpreters, are unspecified

- **Sections:** §2.3 (SPEC:532–588), §3.17, §4.2 step 4, §9.3 S0 (SPEC:1721).
- **Problem.**
  - R19's observed predicate is a quoted string that quantifies over a C array: `predicate "forall used AppRecord r: r.AppState >= RUNNING"` (SPEC:563).
  - Oracles are calls into C functions (SPEC:573–574).
  - Patterns use ad hoc forms: `first_call`, `call(...) in fn(...)`, ordinals (SPEC:546–555, 861).
  - The abstract interpreter (IR locations), the concrete one (events) and the operational one (explorer states) must give each predicate the same meaning. Nothing defines that meaning.
  - The syntax is "illustrative", yet S0's exit gate requires golden tests for the whole rule catalogue.
  - X1-M8 notes that nobody checks the predicate against the code. This issue is the interface defect underneath.
- **Fix.**
  1. At S0, define a typed core expression language **over Φ resource state only**:
     - sorts: keys, ids, enum states with declared orders, integers;
     - boolean connectives;
     - bounded quantifiers over declared resource kinds, never over C arrays.
  2. Let C fields enter only through effect bindings, which O7 checks.
  3. Define the AST-pattern language with a match semantics: function, callee, argument predicates, ordinal, and a macro-expansion flag. A pattern with zero or several matches goes to the audit.
  4. Implement one evaluator in `libkz-core`, behind an abstract-domain interface that all three interpreters use. Add a property test that the three agree on small models.

### X3-m1 — minor — The debug-register budget is shared; RW watchpoints are not cheap; attaching to later-created threads is unspecified

- **Sections:** §4.7 item 2 (SPEC:1027), §4.13 (SPEC:1151, 1160), §7.1 source 11.
- **Problem [I].**
  - Perf HW breakpoints and ptrace debug-register gates share the 4 per-thread slots through the kernel's hw_breakpoint layer. A per-CPU breakpoint takes a slot on every thread that runs on that CPU.
  - DataCollider-style read discovery needs RW watchpoints. Each fires on reads at ≈34 µs ([R07] L339). For example, `AppTable` is read under the ES lock by every `GetAppID`.
  - A gate on a thread created later, such as A.4's provider, needs auto-attach at clone. Each new thread then stops once, which is unmeasured perturbation inside the app-start windows.
- **Fix.**
  1. One joint debug-register allocator in `kz plan` and `kz hold`: watchpoints plus gates ≤ 4 per thread.
  2. Per-task events only.
  3. Write-only watchpoints, except in declared experiments.
  4. An explicit attach strategy, with its cost measured in S6.

### X3-m2 — minor — The tracefs fallback changes global kernel tracing state and needs an explicit clock

- **Sections:** §5.4 (SPEC:1262–1267), A-CLK-PO.
- **Problem [I].**
  - Uprobe event definitions written through tracefs are kernel-global and outlive the private mount namespace. R07 only mounted tracefs and created no events ([R07] L342).
  - ftrace's default trace clock is the per-CPU "local" clock.
  - The perf-PMU uprobe path is fd-scoped, cleans itself up, and was measured to work without tracefs ([R07] L340).
- **Fix.**
  1. Make a perf-sample uprobe backend the zero-install bring-up path: `PERF_SAMPLE_TID|TIME|CPU|REGS_USER`, with `PERF_RECORD_LOST` for loss.
  2. If tracefs is used anyway:
     - use a dedicated instance;
     - set a cross-CPU trace clock;
     - remove the events at exit;
     - record it as a host-state change.

### X3-m3 — minor — Return-value capture misses a likely cheap option, and two [U] items can be closed

- **Sections:** §5.2 (SPEC:1243–1250), §5.10, §5.11.
- **Evidence ([M] here).**
  - After `call CFE_ES_GetAppID` in `CFE_EVS_TaskInit`, gcc emits `mov %eax,-0x124(%rbp)`, then `cmpl $0x0,-0x124(%rbp)`, then `je` (`0x2aca1`–`0x2acae`).
  - Branches are emulated by x86 uprobes (`branch_emulate_op` is present) [I]. A probe on that `je`, reading the spilled value at `rbp-0x124`, is plausibly in the 0.57 µs class.
  - This kernel has the uretprobe syscall trampoline (`__x64_sys_uretprobe`), so option R-a may not be in the #DB class at all [I].
  - Out-of-line helper copies exist (§1), which closes the [U].
- **Fix.**
  1. Add R-e to S1's calibration: "the first emulated instruction after the call while the value is still in a frame slot".
  2. At sites that ignore the status (`cfe_sb_task.c:138`), use the deferred read of the out-pointer target.
  3. Bind same-named local copies through their DWARF compile unit.

### X3-m4 — minor — The `.so` pre-registration fallback perturbs the very phase under study

- **Sections:** §5.2 (SPEC:1252), §5.11.
- **Problem.** The fallback holds the startup thread at each `OS_ModuleLoad` return. That delays app loading, which is exactly the variable that changed the init interleaving in [BM]: with an 849 ms load span instead of 2–3 ms, 10 apps initialised before the last one was loaded (L278–281).
- **Fix.** Register by inode, system-wide, before launch (X3-M1). If the fallback is ever used, exclude its traces from σ_E pools and say so.

### X3-m5 — minor — Closed IR for the app units is unproven

- **Sections:** §4.1 step 6, §4.2.
- **Problem [F].**
  - R07 linked only `core-cpu1` ([R07 §3.4]).
  - Naive linking fails on `CFE_TBL_FileDef`.
  - Apps are loaded `RTLD_GLOBAL` ([R07] L250), so symbol binding depends on load order.
- **Fix.** Run an early S3 probe:
  - link each app unit against the core as an external symbol provider;
  - model the `RTLD_GLOBAL` order from the startup script;
  - exclude table objects;
  - report undefined symbols per unit.

### X3-m6 — minor — KZ schema: churn, implicit padding, and untyped cross-artifact references

- **Sections:** §2.3 (SPEC:458–515), ADR-004.
- **Problem.**
  - `kz_verdict` has 4 bytes of implicit padding before `certificate` (gcc `-Wpadded` on `x3_probe/hdr.c`, [M] here). CRCs and hashes then depend on uninitialised bytes unless every writer zeroes its records.
  - "Layouts final at S0 review" (SPEC:466) comes before any contact with cFS, so the layouts will churn. With no schema compiler, every change touches the header, writer, reader, dump and golden tests by hand.
  - `certificate`, `witness_bad` and the other references are bare `uint64` values pointing across separate files (SPEC:511, 528).
- **Fix.**
  1. Put X-macro field lists in `kz_schema.h` and expand them into the struct, the dump printer and the offset asserts. No files are generated and no external compiler is involved.
  2. Add explicit reserved fields, and assert `sizeof` equals the sum of the field sizes.
  3. Use typed references: (artifact sha256, section, index).
  4. Version each section independently.

### X3-m7 — minor — One driver binary contradicts "trace and model tools do not link Clang"

- **Sections:** §2.2: SPEC:435 against SPEC:454.
- **Fix.** Build two binaries, `kz-static` (with `libkz-llvm`) and a core-only `kz`, or drop the claim.

### X3-m8 — minor — S1's exit gate targets a known count

- **Sections:** §9.3 S1 (SPEC:1722).
- **Problem.**
  - "24 violating uses (ES 20, EVS 4)" as the value to match invites tuning the recorder until it matches.
  - The count also depends on B: 11 instead of 24 without the version table ([R2c §4.2]).
- **Fix.**
  1. Gate on agreement of the per-use set (task, SB function, EID) with [R2g §2]'s table, and with the expectation computed from B.
  2. Investigate and report any mismatch; never tune it away.
  3. Record which backend produced the result.

---

## 3. What reviewers will attack first (ranked), and what answers it

1. **"Where are the bugs?"** There is one natural case, and it is deterministic. All schedule-dependent evidence is derived. *Answer:* X3-C1, with recall on history-derived mutants plus pre/post-fix pairs.
2. **Oracle circularity.** The authors write the contracts and triage the results. *Answer:* header provenance (§7.2), plus the blinded, sampled triage of X3-M7.
3. **Strawman baselines.** *Answer:* X3-M6 and X1-M12: a common metric, frozen comparators, and one real external reference point actually run.
4. **Machinery against payoff.** About 18 subcommands, three interpreters and about 20 assumptions produce a certificate for a 24-event deterministic order. *Answer:* the thin MVC (X3-M5), recall figures, and census scale.
5. **Observation perturbation.** *Answer:* X3-M2 (per-witness ratio, σ_E(plan) labels) and X3-M3 (Init normalization).
6. **"Soundy" with many assumptions** (A1–A12, A-SKEL, A-WS, A-API, L1–L3, A-TYPE). *Answer:* X1, plus program-mutation falsification tests of the certificates (X3-C1, item 1).
7. **External validity.** One platform, `SCHED_OTHER`, and "process-level" meaning threads in one process (§1.1). *Answer:* already in §8.8. Keep it prominent.

---

## 4. Component feasibility (stated tools, this VM)

| Component | Buildable here? | Main risk | Evidence | First probe |
|---|---|---|---|---|
| `libkz-core` (C++17) | yes | Φ semantics unspecified (X3-M11) | — | typed expression core plus golden tests |
| `kz basis` | yes | Init canonicalization (X3-M3) | [BM §6] | content-hash S0 |
| `kz extract` | yes [M] | macro-expansion sites (12.4 %) | [R07 §3.3, §4.6] | — |
| `kz link` (DWARF, MC) | yes; the X86 MC disassembler is built | same-named locals; macro sites | §1 of this review (2 `SetUsed` copies) | bind both copies |
| `kz modref` as specified | high risk (X3-M4) | opaque pointers; from-scratch Andersen | probe IR (`ptr`, GEPs) | S3a field-based scan |
| `kz skel` | yes | call-string blow-up (X1-m7) | — | size report |
| `kz static`, `kz proofcheck` | yes | the checker trusts `skel` and `modref` | SPEC:992–999 | program mutants (X3-C1) |
| `kz observe` (BPF, own loader) | yes, with the X3-M1 fix | ASLR keying; verifier limits | kallsyms; `randomize_va_space`=2 | cookies plus ringbuf in S1 |
| `kz trace`, `kz conform` (O1/O2) | yes | O8 tied to the explorer (X3-M5) | SPEC:1122 | — |
| `kz predict` | yes; exponential | budgets must be in work units (X3-M10) | SPEC:1084 | budget curve |
| `kz explore` | large; gated | kill criterion; Nidhugg cross-check only after an install | SPEC:1116 | differential test |
| `kz hold` (ptrace, debug registers) | yes [M] | shared debug registers; attach at clone (X3-m1) | [R07] L338–339 | holder cost in S6 |
| `kz gate` (`--wrap`) | yes, after D3 | about 15 min per variant build | [R07] L423 | — |
| Baselines (5, plus X1's 3) | yes | effort; fairness (X3-M6) | — | golden tests from the papers |

---

## 5. Minimum changes to the evaluation plan (consolidated)

1. **Add a recall section (§8.5).** Program mutants from the frozen history query, pre/post-fix pairs, per-layer detection with exact CIs, and a certificate-falsification count (X3-C1).
2. **Define the comparison metric (alarm per labelled instance) and freeze the comparators at F1.** Add helgrind/DRD as a reference point that is actually run (X3-M6).
3. **Triage.** A stratified, blinded sample with a defined "reported" set. LOSS-UNDER-ORDER is checked by its oracle, not by raters (X3-M7).
4. **Score ablations on labels.** Mark the S7-dependent ones conditional (X3-M8).
5. **Perturbation:**
   - uninstrumented console intervals as the primary measure;
   - per-witness perturbation ratio against code-derived margins;
   - σ_E(plan) labels;
   - observer CPU recorded;
   - group D per hold plan;
   - normalization boots after abnormal exits;
   - a canonical Init hash (X3-M2, X3-M3).
6. **[POL] labels.** PI-chosen N and caps become `deviation`. Event-based stopping where possible. Anytime curves. Statistics recorded as an assumption (X3-M9).
7. **Budgets in work units**, so that the replay test holds (X3-M10).

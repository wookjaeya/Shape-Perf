# PI decisions requested for Kairos (SPEC FINAL)

- Date: 2026-10-12. Companion of `SPEC_FINAL.md` (§10.1). Each decision lists the question, the options, the references, a recommendation, what stays blocked until it is decided, and what happens by default if it is not decided.
- IDs: v1's D-numbers are kept where the decision is unchanged in substance. Renamed or new decisions have `P-` names (P-LIBBPF was D2, P-Z3 was D5, P-TSAN was D7).
- Policy: every value a person chooses because resources are finite (N, caps, sample sizes) is recorded as a **deviation** with reason, effect and necessity ([POL] L16, L18–21); `unspecified_by_reference` means no value is chosen ([POL] L17). Installs are package decisions recorded under [POL] L18–21.
- Nothing has been installed, run or changed for this document.

## Summary

| Id | Decision | Recommendation | Blocks until decided | Needed by |
|---|---|---|---|---|
| D1 | Root-side observer and holder hook in `env/run_cfs.sh` | approve | S1 and everything that needs traces | S1 |
| D17 | Stop method for experiments, also used for normalization boots | candidate (a): CI ES POWERON restart command | S1 pooled runs; S6 | S1 |
| D14 | N per purpose (deviation); S1's part decided at S1 | decide S1's N at S1, later Ns before S9 | S1 exit gate; S9 | S1 |
| D11 | Re-baseline group A on `-fc-v114` | required | any perturbation comparison | S1 |
| P-LIBBPF | Install libbpf / bpftool | no; own raw loader | nothing unless S1 fails | S1 (only on failure) |
| P-TRACEFS | Allow tracefs dynamic events (a host-state change) | no; perf-sample backend instead | nothing | — |
| D10 | Memory-model stance for ad hoc synchronization | assumed visibility (A4), flagged per verdict | final wording of A4 | S2 |
| D9 | Scope of the first paper (RC8, RC9, RC10, RC12; class M; route/capacity) | include with labels | RQ1/RQ5 scope | S3a |
| D12 | Citing the retired recorder's observations | prior observations only | wording of RQ5/A.5 | S2 |
| D15 | Work-unit caps (prediction, exploration, O8, skeleton construction) as deviations | set before S5/S7, in work units only | S5, S7 kill criterion | S5 |
| D16 | Derived App-record probes on both creation paths | pursue, labelled derived | S6 | S6 |
| D3 | F1 `--wrap` gates, PCT, group W | defer until S6 shows whether F0 holds suffice | S10 PCT arm; G7; group W | after S6 |
| P-PAPERS | Access to primary papers (JACM DPOR, framework-HB rule tables, mover reduction, FG05, G3 search) | approve | G5 for `dpor` stays unverified; L3 citation; comparator golden tests; G3 novelty wording | S7 / S9 |
| P-NIDHUGG | Install `llvm-18-dev` and build Nidhugg | defer to the S7 review | optional explorer cross-check | S7 |
| P-Z3 | Install Z3 | no | SMT cross-checks and the SMT-LIB variant of one baseline | — |
| D6 | MLIR gate G-MLIR | defer to S8 | S8 | S8 |
| D4 | Native σ_R1 arm | model-only; if native, record OSAL's own RR/root-max choice as a new B′ (euid 0) | S10 σ_R1 arm | S10 |
| D13 | 6.4.x derived platform for C3/C5 and H-V (with a cited `P_sched`) | decide before S9 | RQ2 for C3/C5; H-V | S9 |
| D18 | Which configuration and input variants B′ to run | latent-path tables and `B_restart` first | §7.4 results | S9 |
| D19 | `msg_max` variant | keep 10; variant only for RQ5 sensitivity | RQ5 sensitivity | S9 |
| D8 / P-RECALL | Held-out sources; recall protocol (operators, history query, expected-verdict rule, equivalent-mutant rule); who runs the queries | all five sources; recall protocol frozen before F2; queries run by a person who does not author Φ | RQ3, RQ9; F2 | before F2 |
| P-TRIAGE | Raters, blinding, sample sizes, agreement statistic | two blinded raters, at least one not a Φ author; stratified sample (deviation) | RQ3 precision | S9 |
| P-STATS | Pre-registration of the statistical procedure (an assumption) | approve the procedure of SPEC §8.7 before F1 | S9 analysis | before F1 |
| P-VALGRIND | Run helgrind and DRD via `RUN_CFS_WRAP` as a labelled reference point | approve | the external reference row of §8.3 | S9 |
| P-TSAN | ThreadSanitizer rebuild as a reference point | omit unless a reviewer asks | that reference row | S9 |
| P-NAME | The tool's name | keep "Kairos" as a working label; check for collisions before publication | publication only | before submission |

## Tool installs (summary)

Nothing must be installed for the thin MVC (S0 + S1 + S2 + S3a) or for S4–S6: the pinned LLVM 23 build (Clang LibTooling, IR, DWARF, MC), system clang-18 (BPF backend), gcc 13.3, valgrind 3.22 and the kernel facilities are present ([R07 §2], §4.7, L156). Possible installs, each a recorded package decision:

| Package | For | Decision | Default |
|---|---|---|---|
| libbpf / bpftool | a shorter BPF loader | P-LIBBPF | not installed |
| Z3 4.8.12 (apt candidate, [R07 §2.4]) | SMT-LIB cross-checks | P-Z3 | not installed |
| `llvm-18-dev` (apt candidate, [R07] L126–127) + a Nidhugg build | explorer cross-check on pthread-only litmus programs | P-NIDHUGG | not installed |
| compiler-rt (build from the LLVM tree; time untested, [R07 §9 item 6]) or a gcc `-fsanitize=thread` variant | TSan reference point | P-TSAN | not built |
| GenMC | — | not possible: its README requires LLVM 19–22 ([R05 F11]) | — |

---

## D1 — Root-side observer and holder hook

- **Question.** May a root process attach perf-PMU uprobes, eBPF programs, per-task watchpoints and ptrace holds to the README-mode `ubuntu` process? This needs one new hook in `env/run_cfs.sh` next to `RUN_CFS_WRAP` / `RUN_CFS_HOOK`.
- **Options.** (a) Approve (fidelity class F0: unmodified binary and run conditions; only timing perturbed). (b) Refuse: only F1 (`--wrap`, a link-option deviation) or F2 (source probes) remain.
- **References.** [R07 §9 item 1], [R07] L355 (uprobe PMU needs `CAP_PERFMON` or `CAP_SYS_ADMIN`), L500; [R07 §4.7] (measured: uprobe 0.57 µs at entry+4; ptrace from root on uid 1000 works, no Yama); SPEC §5.1, §5.9.
- **Recommendation.** (a).
- **Blocked until decided.** S1 and everything that consumes traces (S2; reach and O1/O2 in S3a; S4–S6; RQ5, RQ7).
- **Default if undecided.** No Kairos observation runs.

## D17 — Stop method (and normalization boots)

- **Question.** Which stop method ends experiment runs, and which method normalizes persistent state after an abnormal exit?
- **Options.** (a) The CI ES POWERON restart command (`.github/workflows/build-run-app-reusable.yml` L209–214; E11 candidate (a) in [BM] L29, L343–344): it ends through `CFE_PSP_Restart(POWERON)`, which deletes the SysV segments (`cfe_psp_support.c:62–69`). (b) A signal stop: leaves the segments, so the next boot is a PROCESSOR reset ([R01 §5.2]: PO → PR → PO after signal stops). (c) Another documented method.
- **References.** [BM] L29, L343–344; [R2g] (runs ended by E11(a)); SPEC §3.5, §4.13 (normalization), §1.7.
- **Recommendation.** (a) for both the stop of observation runs and the normalization boot after every run that did not end with the POWERON line; record it as a from-reference candidate. Note that every stop is a teardown, so uses after it are K-TEARDOWN instances, reported apart.
- **Blocked until decided.** Pooled runs in S1; confirmation runs in S6 (they are designed to end abnormally).
- **Default if undecided.** Runs are made one at a time with S0 recorded, but not pooled.

## D14 — N (runs per purpose), as a deviation

- **Question.** How many runs per perturbation group, per loop iteration and per σ_E frequency?
- **Options.** The PI sets N per purpose as a recorded **deviation** (reason: finite resources; effect: bounded results with exact intervals or `unknown(budget)`; necessity: no reference defines N). Event-based stopping where it exists (loop fixed point, exhaustion). Methodological literature (Arcuri & Briand [V-abs]; Klees et al.'s ~30 trials, known only from a secondary summary, P) may motivate the choice but is never `from-reference` ([POL] L5–10).
- **References.** [POL] L16–21; [X3-M9]; SPEC §8.7.
- **Recommendation.** Decide **S1's characterisation N at S1** (it is needed by S1's exit gate); decide the evaluation Ns before S9.
- **Blocked until decided.** S1 exit gate (groups A, B, C′ comparison); S9.
- **Default if undecided.** No group comparison is reported.

## D11 — Re-baselining after the kernel change

- **Question.** Re-measure group A on `6.18.44-fc-v114` before any comparison? (The records were made on `-fc-v80`.)
- **References.** [R07 §1]; [BM]; SPEC §5.10.
- **Recommendation.** Required, in S1.
- **Blocked until decided.** Every perturbation comparison.

## P-LIBBPF (was D2) — eBPF userland

- **Options.** (a) Kairos's own raw `bpf()`/`perf_event_open` loader with hand-written helper prototypes and a glibc `<elf.h>` ELF reader (no install). (b) Install libbpf/bpftool (recorded package decision).
- **References.** [R07 §2.3, §4.7] (libbpf headers absent; raw `bpf()` load and uprobe attach probed); [J3 §2.4] (UAPI `linux/bpf.h` present); SPEC §5.3.
- **Recommendation.** (a); install only if S1 shows that attach cookies, ring buffers or task storage cannot be used without it.
- **Blocked until decided.** Nothing, unless S1 fails.

## P-TRACEFS — tracefs dynamic events

- **Question.** May Kairos create tracefs uprobe events? They are kernel-global and outlive a private mount namespace [I] (a host-state change).
- **References.** [R07 §4.7] (tracefs mountable in a private namespace; no events created); [R07] L340 (perf-PMU uprobes work without tracefs); [X3-m2]; SPEC §5.4.
- **Recommendation.** No; the perf-sample backend is the zero-install bring-up path. If ever approved: a dedicated instance, an explicit cross-CPU clock, events removed at exit, a recorded host-state change.
- **Blocked until decided.** Nothing.

## D10 — Memory-model stance

- **Options.** Assumed visibility (A4: sequential consistency at event granularity on x86-64 TSO at `-O0`), flagged on every verdict that uses an `assumed` edge; or report the ad hoc synchronization sites as findings.
- **References.** [R06 T8]; [R02 §8.3]; SPEC §3.14.
- **Recommendation.** Assumed visibility, flagged.
- **Blocked until decided.** The final wording of A4 and of the `assumed-visibility` sub-label.

## D9 — Scope of the first paper

- **Question.** Are RC8 (persistence), RC9 (parent ↔ child), RC10 (timer → task), RC12 (file TOCTOU) in scope as process-level; how is class M (memory-level races found in passing) reported; are route/capacity losses (LOSS-UNDER-ORDER) in the first paper?
- **References.** [R06 §8 Q6]; [R03 §4]; SPEC §3.15.
- **Recommendation.** Include, with the labels: RC8, RC9, RC12 marked partial; class M and LOSS-UNDER-ORDER reported separately.
- **Blocked until decided.** The denominators of RQ1 and RQ5.

## D12 — Citing the retired recorder's observations

- **Options.** Cite CORD's and the M1 harness's observations as prior observations; or only after Kairos re-observes them.
- **References.** [R2c]; [BM §3.3]; SPEC App. A.5.
- **Recommendation.** Prior observations only; S1/S2 re-observe.
- **Blocked until decided.** The wording of RQ5 and A.5.

## D15 — Work-unit caps, as deviations

- **Question.** The caps on prediction search nodes, explorer states/transitions, O8 search and skeleton construction, also used by the explorer's kill criterion.
- **Options.** PI-set caps in deterministic work units, each a deviation (reason: finite resources; effect: `unknown(budget)` with anytime curves; necessity: no reference defines them). A wall-clock cap only as a labelled exception excluded from the byte-exact replay test.
- **References.** [X3-M9], [X3-M10]; SPEC §4.9 step 8, §4.10 items 6 and 9, §4.11, §4.15.
- **Recommendation.** Work units only; set before S5 (prediction, O8) and before the S7 review (explorer).
- **Blocked until decided.** S5's budget curves; the S7 kill criterion.

## D16 — Derived App-record probes

- **Question.** Run the derived F0 holds that open the core-task and script-app App-record windows ([LED] X5)?
- **References.** [LED §3.4, X5]; SPEC App. A.2, A.3; §4.13 (placement, watchdogs incl. `CFE_PSP_RESTART_DELAY`, normalization boot).
- **Recommendation.** Pursue, labelled derived, with normalization boots after abnormal exits.
- **Blocked until decided.** S6's exit gate.

## D3 — F1 `--wrap` gates, PCT, perturbation group W

- **Options.** Approve the link-option deviation for PCT and many-point schedules (and group W, which measures the variant's code-layout effect); or F0 holds only.
- **References.** [R07 §4.2] (`--wrap` misses 88 of 3,762 intra-TU sites), §5.1 (handoff 1.1/10.5 µs), L423 (≈15 min per variant build); PCT (ASPLOS 2010 [V-abs; mechanism P]); SPEC §4.13, §8.6.
- **Recommendation.** Defer until S6 shows whether F0 holds suffice; this is the "native PCT arm".
- **Blocked until decided.** S10's PCT arm; G7; group W.

## P-PAPERS — Access to primary papers

- **Question.** Supply primary PDFs (or allowlist hosts) so that statuses P / UNVERIFIED can be checked.
- **Items.** (1) The JACM 2017 version of source/optimal DPOR with disabling (T-JACM; status P in [R05 §2.2]); (2) the HB rule tables of DroidRacer, CAFA, EventRacer and DCatch ([R04 §8 item 3]; DCatch is P); (3) a citation for the left/right-mover reduction behind L3 (not verified in R04/R05); (4) the guarantee wording of DPOR (POPL 2005, UNVERIFIED); (5) a targeted search of RV/TACAS for timeout-conditional HB edges (G3 novelty, [R04 §8 item 5]); (6) the papers' worked examples for the comparators' golden tests.
- **References.** [R04 §8]; [R05 §2.2, §8]; `availability.md` A1 proposes `research/cfs_procrace/paper/` as the place.
- **Recommendation.** Approve.
- **Blocked until decided.** G5 for the `dpor` engine stays "unverified" (the `exh` engine is unaffected); L3's citation; comparator golden tests; the G3 novelty statement (until then: "open literature check").

## P-NIDHUGG — Explorer cross-check with Nidhugg

- **Question.** Install `llvm-18-dev` and build Nidhugg to cross-check the explorer on litmus programs that use only pthread create, join and mutex?
- **References.** [R05 F11] (Nidhugg README supports LLVM 8–19; "each thread should be deterministic when run in isolation"); [R07] L126–127 (`llvm-18-dev` is an apt candidate, not installed); [R05 §6.3] L452 (run such tools on the same closed harnesses); GenMC needs LLVM 19–22 (not possible).
- **Recommendation.** Defer to the S7 review; install only if the explorer survives its kill criterion.
- **Blocked until decided.** The optional cross-check only.

## P-Z3 (was D5) — SMT solver

- **Options.** Z3 4.8.12 from apt for SMT-LIB cross-checks on small cones; or explicit search only.
- **References.** [R07 §2.4]; SPEC §4.9 step 10, §8.3.
- **Recommendation.** Explicit search only. The MCM/GPredict-style baseline is implemented as an explicit equivalent search; its SMT-LIB variant waits for this decision.
- **Blocked until decided.** SMT cross-checks only.

## D6 — MLIR gate

- **Options.** Run G-MLIR at S8 (verdict equality required; provenance completeness, code size, runtime); or defer.
- **References.** [R06 T1, K8]; SPEC §2.6, ADR-018.
- **Recommendation.** Defer to S8; no ODS/TableGen before the gate passes.
- **Blocked until decided.** S8 only.

## D4 — Native σ_R1 arm

- **Question.** Realize σ_R1 natively, in addition to the model?
- **Facts.** When RT scheduling is permitted, OSAL POSIX selects `SCHED_RR` on Linux (FIFO's spread is never > 255), gives the root thread (`CFE_ES_Main`) the highest priority, compresses priorities, and uses priority-inheritance mutexes (`os-impl-tasks.c:78–106, 348–365, 391–409`; `os-impl-mutex.c:82`). A root-imposed `SCHED_FIFO` would be overridden by OSAL's own selection. Running with RT privileges as root also sets euid 0, which removes the `msg_max` queue truncation (`bsp_start.c:56–74`), so it is a new basis B′.
- **Options.** (a) Model-only σ_R1[`P_sched^posix`]. (b) Native, recording OSAL's own choice (RR, root at max) as the realized σ, as a B′. (c) Native with an override of OSAL's policy (a deviation).
- **References.** [X2-07]; [R07 §5.2]; SPEC §1.8, §7.4.
- **Recommendation.** (a); if a native arm is wanted, (b), never (c).
- **Blocked until decided.** S10's σ_R1 arm; native evidence for H-V.

## D13 — The 6.4.x derived platform

- **Question.** Build the shared derived 6.4.x platform for C3/C5 at 6.4.1 and for H-V?
- **Facts.** The 6.4.1 bases lack a compile DB (risk rated high); an RT claim also needs a **cited scheduling profile** for that backend (no σ_R1 verdict without it).
- **References.** [LED] X3; [R06 §4 T5]; SPEC §7.6, §10.2.
- **Options.** Build (reconstruct the compile DB from the 6.4.x make system) and cite its `P_sched`; or code-reading evidence only.
- **Recommendation.** Decide before S9.
- **Blocked until decided.** RQ2 for C3 and C5; H-V.

## D18 — Configuration and input variants B′

- **Options.** Per SPEC §7.4: tables that activate latent paths (CF `sem_name` with a provider, LC, MD, HS AMT/EMT/XCT, CS); alternate schedules; script and core orders; cpu2; `msg_max` and euid; reset type (PROCESSOR with preserved CDS and a volatile script); inputs (`B_restart`, table commands, `B_stop`).
- **Recommendation.** Latent-path tables and `B_restart` first; nothing found only in a variant is a "new bug" of the default configuration.
- **Blocked until decided.** §7.4 results.

## D19 — `msg_max` variant

- **Options.** Keep 10 (the recorded Env, [BM §3.1]); or a recorded variant.
- **Recommendation.** Keep 10 for the default B; a variant B′ only for RQ5 sensitivity.
- **Blocked until decided.** RQ5 sensitivity only.

## D8 / P-RECALL — Held-out sources and the recall protocol

- **Question.** Which held-out sources are used; what exactly is frozen for the recall protocol; who runs the queries and when relative to F2?
- **Items to freeze before F2.** (1) The mutation operators (remove a wait; move a publication after a spawn; swap object-table entries; empty a retry bound; drop a status check). (2) The history query over the offline cFE (2047 commits, full history), OSAL and app histories: keywords and a date cut (23 cFE subjects match `race|startup sync|order` as an untriaged starting set, [X3 §1]). (3) The expected-verdict rule (from the fix's issue text and commit; [POL] reference kinds L7–8). (4) The equivalent-mutant exclusion rule (mutated site `inactive(B)` with proof, or byte-identical objects). (5) The held-out issue query (keywords, date cut), excluding C1, C2, C3, C5 and R2.
- **References.** [R06 §5.6]; [R05 §6.1 (iii), §6.2]; [X3-C1]; SPEC §7.5, §8.5.
- **Options.** All five held-out sources plus the recall protocol; or a subset. Who: an author of Φ, or someone who did not author Φ.
- **Recommendation.** All five sources and the full recall protocol, frozen before F2, with the queries run by a person who does not author Φ.
- **Blocked until decided.** F2; RQ3; RQ9.

## P-TRIAGE — Raters and sampling

- **Question.** Who rates, how they are blinded, how large the stratified samples are, which agreement statistic is used.
- **References.** [X3-M7]; [R05 §6.2] (two independent raters with a disagreement log); SPEC §8.5.
- **Options.** Two raters (authors or not), blinded to the producing layer, the ablation and each other; sample size per (class, RC kind, deciding layer) stratum as a deviation; agreement statistic recorded as an assumption.
- **Recommendation.** Two blinded raters, at least one not an author of Φ; sample sizes recorded as deviations with exact CIs; LOSS-UNDER-ORDER checked against the SB oracle, not rated.
- **Blocked until decided.** Triaged precision (RQ3) and the triage-based labels used by ablations and baselines.

## P-STATS — Pre-registration of the statistical procedure

- **Question.** Approve the procedure of SPEC §8.7 as a recorded **assumption** (methodological literature is not a reference kind under [POL] L5–10): full distributions; Mann–Whitney U for times and counts; Fisher's exact test for incidence; a paired test for per-instance binary comparisons; standardized effect sizes; exact binomial CIs; interleaved group order (also an assumption).
- **Recommendation.** Approve before F1.
- **Blocked until decided.** S9 analysis.

## P-VALGRIND — helgrind and DRD as an external reference point

- **Question.** Run valgrind's helgrind and DRD on the unmodified binary through `RUN_CFS_WRAP`, as gdb was run ([R2g] L11)?
- **Facts.** valgrind 3.22 with helgrind and DRD is installed and ran a test program ([R07] L156, L346); it serializes threads, so its schedules are not README-mode scheduling ([R07] L466); slowdown on cFS unmeasured. It is excluded as a *baseline* by the user's decision ([R06 S13]) but allowed as a labelled reference point.
- **Options.** Approve as a labelled reference point (deviation: wrapped run command); or omit.
- **Recommendation.** Approve: it is the only external tool that runs here without an install, and it gives one comparison that does not depend on Kairos's own probe plan.
- **Blocked until decided.** The external reference row of SPEC §8.3.

## P-TSAN (was D7) — ThreadSanitizer reference point

- **Options.** Omit; a gcc `-fsanitize=thread` variant build as a labelled deviation; or clang TSan after building compiler-rt (absent, [R07] L129; build time untested).
- **References.** [R04 §8 item 4]; [R05 §6.3 (c)]; [R07 §2].
- **Recommendation.** Omit unless a reviewer asks; P-VALGRIND covers the memory-level reference point.
- **Blocked until decided.** That reference row only.

## P-NAME — Naming

- **Question.** Keep "Kairos" (prefix `kz`)?
- **Facts.** The name is a working label taken from proposal C; no name-collision or trademark search was made.
- **Recommendation.** Keep it as a working label; run a collision check before any publication or public repository.
- **Blocked until decided.** Publication only.

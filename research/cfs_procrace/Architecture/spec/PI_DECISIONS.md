# PI decisions for Kairos (SPEC FINAL): questions and resolutions

- Date: 2026-10-12. Companion of `SPEC_FINAL.md` (§10.1). Each decision lists the question, the options, the references, a recommendation, what stays blocked until it is decided, and, where SPEC states one, what happens by default if it is not decided.
- **Revision 1.2 (2026-10-15).** The PI's resolved decisions of 2026-10-15 are recorded in this file. Their source is `Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md` (full path `/home/user/Shape-Perf/research/cfs_procrace/Architecture/spec/PI_DECISIONS_RESOLVED_20261015.md`, here "the resolved-decisions record"). Every decision that record resolves has a **Resolution** line with its value, the date 2026-10-15 and the section of the record; a decision that the record does not address is marked **Open** (D18 except its C1 part, P-TRACEFS, and the details of D8/P-RECALL). A resolution is a decision [D] on the research plan, its scope and its wording. It is not a verified fact about a tool, a paper or a source, and not an execution record: nothing was built, installed or run. In each entry below, the Resolution line is new; the rest (question, options, facts, references, recommendation, "blocked until decided", "default if undecided") is the question as it was asked before the resolution. The Resolution supersedes the recommendation, and a resolved decision no longer blocks what it blocked. SPEC rev 1.2 applies the resolutions (SPEC §0.10, §10.1; ADR-046 to ADR-051).
- **Revision for SPEC_FINAL rev 1.1** (2026-10-15). SPEC rev 1.1 answers the external review `history/review_EXT_20261012.md` (amendment EXT-20261012, SPEC §0.7; ADR-039 to ADR-044). The rows of SPEC §10.1 marked new or candidate, and the QT- and Q-items, are added here under the same IDs, with their options, references, recommendations and blocks as SPEC states them. Existing rows are updated to SPEC §10.1. Nothing else is decided here.
- IDs: v1's D-numbers are kept where the decision is unchanged in substance. Renamed or new decisions have `P-` names (P-LIBBPF was D2, P-Z3 was D5, P-TSAN was D7). Exceptions: the formal amendment's candidates D20–D22 and the flagged items QT-n and Qn of EXT-20261012 keep the IDs of their drafts (SPEC §10.1).
- Decisions of `STATUS_20261012.md` that SPEC rev 1.1 changes: **G0** (the thin MVC S0–S3a as the first target) is replaced by **P-LADDER**; **T2** (the provenance kind `requirement`, and the SB no-subscriber loss as an allowed drop) is re-asked as **QT-2**. **T1** (renaming "contract") was decided as (a) on 2026-10-15 (entry T1 below); the rename is a dedicated step that is not applied yet, so SPEC and this file still say "contract" (SPEC §0.7 item 4). **T3** (an AI-style learned-invariant comparator in the variants AI-all and AI-lc, and the related-work additions AI, Joshi & Sen, Lifestate and Dwyer et al.) is not a row of SPEC §10.1, and no PI answer to it is recorded, so it is still pending as STATUS §3.1 asked it. SPEC rev 1.1 neither adopts nor rejects it: §8.3 has no AI-style row (its learned-invariant comparator is the DefUse-style row), and SPEC cites neither AI (FSE 2014) nor Dwyer et al. Only Joshi & Sen (ASE 2008) and Lifestate (ECOOP 2019) are in SPEC rev 1.1, credited as prior art in answer to EXT-20261012 §3 (§0.8, §1.11, §6.4 item 18, ADR-037 as revised by ADR-042, App. B); reading them before any difference is claimed is item (8) of P-PAPERS. Adding the comparator would change SPEC §8.3, and F1 freezes every comparator implementation (§7.2); the proposal itself is in `background/08_terminology.md` §6.2 and §7 item 10.
- Policy: every value a person chooses because resources are finite (N, caps, sample sizes) is recorded as a **deviation** with reason, effect and necessity ([POL] L16, L18–21); `unspecified_by_reference` means no value is chosen ([POL] L17). Installs are package decisions recorded under [POL] L18–21.
- Nothing has been installed, run or changed for this document.

## Summary

| Id | Decision | Resolution (2026-10-15; section of the resolved-decisions record) | Recommendation before the resolution | Blocked until decided (before the resolution) | Needed by |
|---|---|---|---|---|---|
| T1 | The name of the judged object | (a) readiness specification; the rename is not applied yet (§2) | (a) (the question was asked in `STATUS_20261012.md` and `DECISIONS_20261015.md`) | the rename step | the rename step |
| P-LADDER | The first target: the validation ladder V0–V4 with the case tracks (replaces STATUS's G0); effort for V1 or any rung (none exists); the order of V2, V3 and the comparison; the expansion rule | Approved (§2) | the proposal of SPEC ADR-044; no effort figure | V1 and every later rung | V1 |
| D20 | Option A (safety statements; progress `unknown(not_checked)` wherever `exh` does not run) or option B (a progress obligation on certificates) for the first implementation | A (§2, §2.1) | A | the reading of every certified verdict (ADR-039); V0 | V0 |
| QT-2 | STATUS decision T2, re-asked: the provenance kind `requirement`; the SB no-subscriber loss reported in three judgements, not as an allowed drop | Approved; the SB rule for a send with no subscriber and R2's AppId error kept apart (§2) | as proposed (ADR-043) | the caller-level Allowed (SPEC §1.6) and ADR-043 | V0 |
| QT-3 | Adopt the minimal three-judgement design of ADR-043 | Approved (§2) | as proposed (ADR-043) | the caller-level Allowed and ADR-043 | V0 |
| QT-4 | Whether `rule`-only Allowed entries decide statuses, reported as rule-backed, or are `unspecified_by_reference` | Strict alternative (§4.1) | `rule`-only entries decide and are reported as rule-backed | the caller-level Allowed and ADR-043 | V0 |
| D1 | Root-side observer and holder hook in `env/run_cfs.sh` | Approved; an observation-effect check replaces "only time changes" (§2, §2.2) | approve | S1, V1's S1-R2 sub-gate, and everything that needs traces | V1, S1 |
| D17 | Stop method for experiments, also used for normalization boots | (a) (§2) | candidate (a): CI ES POWERON restart command | V1's pooled runs; S6 | V1, S1 |
| D14 | N per purpose (deviation); S1's part decided at S1, including the `n of N` label of V1 | N per purpose at S1, frozen before the main experiment (§2) | decide S1's N at S1; V1's reach can be proved from the first observed occurrence with an event-based stop; later Ns before S9 | V1; S1 exit gate; S9 | V1, S1 |
| D11 | Re-baseline group A on `-fc-v114` | Required, before the S1 observation-effect comparison (§2.2, §4.1) | required | V1b (the S1 gate); every perturbation comparison | V1b (S1) |
| P-LIBBPF | Install libbpf / bpftool | Existing environment first (§4.4) | no; own raw loader | nothing unless S1 fails | S1 (only on failure) |
| P-TRACEFS | Allow tracefs dynamic events (a host-state change) | Open | no; perf-sample backend instead | nothing | — |
| D10 | Memory-model stance for ad hoc synchronization | Assumptions marked; guarantee limited (§4.1) | assumed visibility (A4), flagged per verdict | final wording of A4 | S2 |
| D9 | Scope of the first paper (RC8, RC9, RC10, RC12; class M; route/capacity) | Limited to the five cases and direct controls (§4.1) | include with labels | RQ1/RQ5 scope; V1b | V1b (S3a) |
| D12 | Citing the retired recorder's observations | CORD is not evidence in current documents (§4.1, §6) | prior observations only | wording of RQ5/A.5 | S2 |
| QT-5 | Which text prevails when an `appreq` and a `caller` entry, or two `caller` entries, conflict | Conflict stated; judgement withheld (§4.1) | none chosen | the caller-level Allowed and ADR-043; C1's statuses once any of its other texts is authored | before any other C1 text is authored |
| QT-9 | K-SPAWN.osal and K-LOCKBAL: moved to an implementation-defect family, or given a researcher-authored `appreq` | Implementation-defect family (§4.1) | none chosen | the labels of K-SPAWN.osal and K-LOCKBAL (SPEC §7.3) | before those labels are reported |
| Q15 | The normal continuation of `returned_and_tested`: the default of SPEC §1.6, or every entry names its continuation from a reference | Strict alternative (§4.1) | the default of SPEC §1.6 | the outcome labels of the rows that rest on the default (App. A.2 step 3, App. A.4 step 3) | before those labels are reported |
| P-R2CTRL | R2's control basis B′ for V2: (a) none; (b) PR #2820 head backported; (c1) `v7.0.0`; (c2) `b9bec567^`/`b9bec567`; (c3) pre-`b9bec567` core order in v7.0.1's `MISSION_CORE_MODULES`; (d) SB-first order; (e) lazy resolution implemented; (f) model-level fix-direction mutant | (c2) first, (c1) secondary, (b) optional (§3.1) | none; reference status per option below | V2 (EXT-20261012 §7 step 3) | V2 |
| D22 | C1's derived bases B′ and B′_buggy; D18's C1 part folded in | Public BP basis; CF `281a941` → `833fdbb` (§3.2) | none chosen | V3 (needed before it, not by S9); C1 rows of App. A.4 and §7.3 | V3 |
| P-C1HOLD | C1's holds: provider hold before its create; the declared RESERVED-window experiment and its hang bound; the watchdog of App. A.4 step 5 reconciled with §4.13 | Hold before creation, release on an event; the RESERVED experiment is a follow-up (§3.3) | decide before V3's S6 part | V3; S6 for C1 | V3's S6 part |
| D13 | 6.4.x derived platform for C3/C5 and H-V, with Q16; (a) analysis-only parse; (b) compile-DB inputs; (c) placement before F1 or C3 and C5 post-freeze by design | (a) accepted; (b)(i) first, (ii) on sensitivity; (c) before F1; Q16 (§3.4) | raise now; no option recommended | VP-D3; RQ2 for C3/C5; H-V; P-F1GATE | now |
| P-C2 | C2's role-derived pair, C2-D4, Q22, the buggy/fixed revisions and their configure decision, and which contrast of the 2 × 2 counts for RQ9 | Role reconstruction; Q22: a normally completing provider first (§3.5) | none until decided | VP-D4; C2's rows of RQ2 and RQ9 | VP-D4 |
| P-RGIT | `58abd38d^`/`58abd38d` as a basis on the A.3 residual line | Auxiliary control (§3.6) | none until decided | VP-D5 | VP-D5 |
| D15 | Work-unit caps (prediction, exploration, O8, skeleton construction) as deviations | Work-unit budgets, frozen after the stage pilot (§4.2) | set before S5/S7, in work units only | S5, S7 kill criterion; the only model progress route; the closed cone (D21) | S5, S7 |
| D21 | The cone on which the S7 kill criterion is assessed | Closed cone (§4.2) | closed cone, caps unchanged | S7 kill criterion; every model progress result | S7 |
| P-D18GATES | (a) LP-CASE-C1 for the S5 gate's "C1-derived trace" until D22 lands; (b) the S7 review with or without D22, and whether it includes, or waits for, every case-track cone that exists at review time (C2's role-derived pair under P-C2; C3 and C5 on the 6.4.x pair under D13); a decision taken without the cones of C2, C3 and C5 is reported with that fact (SPEC §4.10 item 9) | (a) development test; (b) preliminary against final judgement (§4.2) | none until decided | the S5 gate; the S7 kill criterion | S5, S7 |
| D16 | Derived App-record probes on both creation paths (not C1's holds, see P-C1HOLD) | Approved, labelled derived (§4.1) | pursue, labelled derived | S6 | S6 |
| D3 | F1 `--wrap` gates, PCT, group W | Later (§4.2) | defer until S6 shows whether F0 holds suffice | S10 PCT arm; G7; group W | after S6 |
| P-PAPERS | Access to primary papers (JACM DPOR; framework-HB rule tables; mover reduction; FG05 guarantee; G3 search; GPredict §II.B property formalism; Joshi & Sen ASE 2008 and Lifestate ECOOP 2019) | Approved; start now (§4.4) | approve | G5 for `dpor`; L3 citation; comparator golden tests; G3 novelty wording; the GPredict characterization of SPEC §1.11 and its same-information encoding (§8.3); the framework-typestate difference claimed in §1.11 | S7 / S9 (V4a) |
| P-NIDHUGG | Install `llvm-18-dev` and build Nidhugg | Held (§4.4) | defer to the S7 review | optional explorer cross-check | S7 |
| P-Z3 | Install Z3 | Not a first-implementation condition (§4.4) | no | SMT cross-checks and the SMT-LIB variant of one comparator | — |
| D6 | MLIR gate G-MLIR | Excluded (§4.2) | defer to S8 | S8 | S8 |
| D4 | Native σ_R1 arm | Model-based first (§4.2) | model-only; a native arm would record OSAL's own RR/root-max choice and run as root (euid 0 → a new B′) | S10 σ_R1 arm; VP-D3 lists D4 among its needs | S10; VP-D3 |
| D18 | Which configuration and input variants B′ to run (C1's part moved to D22) | Open (C1 part folded into D22) | latent-path tables and `B_restart` first | §7.4 results (S9); P-R2CTRL options (c3), (d) | S9; V2 if (c3) or (d) |
| D19 | `msg_max` variant | `msg_max` 10 kept (§4.1) | keep 10; variant only for RQ5 sensitivity | RQ5 sensitivity | S9 |
| P-F1GATE | F1 waits until D13, D22 and P-C2 are each decided or explicitly declined with the consequence recorded, and each design case has template-only output on its own basis or a recorded reason why it has none | Approved (§3.7) | proposal; no date or threshold | F1 | F1 |
| P-CLASSGT | How a reference's class statement ("race (condition)") maps to the per-(σ, M) classes | No automatic class from the word "race" (§4.3) | none until decided: the mapping is `unspecified_by_reference` | class-correct counts (§8.5); RQ2, RQ8, RQ9; the C3 contrast | F1 (frozen as `unspecified_by_reference` while undecided); S9 |
| P-STATS | Pre-registration of the statistical procedure (an assumption), with the unit and independence assumption of every test and the handling of clustering by cause and fix | Descriptive first (§4.3) | approve before F1; the dependence handling is chosen from the options of SPEC §8.7, none recommended here | S9 analysis | before F1 |
| D8 / P-RECALL | Held-out sources; history query (keywords, matched field and case, date cut), operators, expected-verdict and exclusion rules; whether the queries exclude the design cases' fixes and named items; the exposure and freeze record; who runs them | Five cases first; extended evaluation separate; details open (§4.3) | all five sources; queries frozen before F2 by a person who does not author Φ; no blind claim before the exposure record exists; until decided, named items are kept and labelled non-blind; the exclusion is the PI's choice | S9; RQ3, RQ9; F2 | before F2 |
| P-TRIAGE | Raters, masking, sample sizes, rater exposure, agreement statistic | Two independent raters for quantitative judgements (§4.3) | two masked raters, at least one not a Φ author; each rater's exposure recorded; stratified sample (deviation) | RQ3 precision | S9 |
| P-VALGRIND | Run helgrind and DRD via `RUN_CFS_WRAP` as a labelled reference point | Excluded (§4.4) | approve | that reference point (S9) | S9 (V4a) |
| P-TSAN | ThreadSanitizer rebuild as a reference point | Excluded (§4.4) | omit unless a reviewer asks | that reference row | S9 |
| P-ABL-ALLOWED | Add the single-element ablations "Allowed off" and "outcome semantics off", and keep the former stripped GPredict-style row as a labelled bundle | Two single-element ablations (§4.3) | none until the PI decides | the candidate ablations and the bundle (§8.3); none of them is in §8.4 until decided | S9 |
| P-NAME | The tool's name | Kairos kept (§2) | keep "Kairos" as a working label; check for collisions before publication | publication only | before submission |

## Ladder view (SPEC §9.3, ADR-044)

The decisions each rung and case track needs, as the "Needs" column of SPEC §9.3's ladder table states them.

| Rung or track | What it validates | Needs |
|---|---|---|
| V0 formal fixes (with V0-P) | the definitions of ADR-039–ADR-043 | D20 (A); the PI items behind ADR-043 (QT-2, QT-3, QT-4; SPEC §0.7 item 4), all resolved on 2026-10-15 |
| V1 R2's K-ID chain end to end on the ENV basis | R2 (certificate route, recorder, O4); the FIXED_ORDER direction of R2's in-basis negative control (E-post and E-TBL) | D1, D17, D14 (S1 part), P-LADDER |
| V1b the rest of the thin MVC gates on the ENV basis | generic layers on the v7.0.1 counterparts of C1, C2 and C5; calibration; the PROTECTED direction of R2's negative control; the app-unit closure and app `.so` probe registration that V3 needs | D1, D17, D14, D11, D9 |
| V2 R2's control basis B′ (parallel with V3) | that the fixed-order verdict is withdrawn where the publication order is corrected | P-R2CTRL; D18 for options (c3) and (d); D1, D17, D14 for runs |
| V3 C1 on its B′ (parallel with V2) | C1's own phenomenon | D22 (with D18's C1 part), C1 D2–D4, P-C1HOLD, D15, D21, P-D18GATES (a) |
| VP-D3 C3 and C5 on the 6.4.x pair | C3's and C5's own phenomena; H-V | D13 (a)–(c), Q16, D15, D4 |
| VP-D4 C2 role-derived | C2's own phenomenon, as a synthetic control | P-C2, D15; D13 for a 6.4.2 row |
| VP-D5 the A.3 line | a PROTECTED certificate that appears on `58abd38d^` and is withheld on `58abd38d` | P-RGIT |
| V4a comparison with existing process-level race work | the six analysis questions and one result state per (tool, case) (SPEC §8.3, revision 1.2); before it, the three outcomes per tool and labelled unit | P-STATS, P-CLASSGT, P-PAPERS (access routes first), P-F1GATE |
| V4b held-out and pre/post-fix pairs | §7.5 sources (a)–(e); recall (§8.5) | D8/P-RECALL, P-TRIAGE, P-STATS, D14, P-CLASSGT |

The paper track (reading at pinned revisions, and an expectation sheet frozen by hash before the first tool output on each case, for R2, C1, C2, C3 and C5) needs no stage and no decision (SPEC §9.3). Every decision named in the Needs column was resolved on 2026-10-15 except D18, which is open and is needed only if P-R2CTRL option (c3) or (d) is added later; the names in the column are those of the question as asked (for example, V4a's P-VALGRIND is resolved as excluded, and the rung now compares existing process-level race work, SPEC §9.3).

## Tool installs (summary)

Nothing must be installed for the thin MVC (S0 + S1 + S2 + S3a) or for S4–S6: the pinned LLVM 23 build (Clang LibTooling, IR, DWARF, MC), system clang-18 (BPF backend), gcc 13.3, valgrind 3.22 and the kernel facilities are present ([R07 §2], §4.7, L156). Possible installs, each a recorded package decision:

| Package | For | Decision | Default |
|---|---|---|---|
| libbpf / bpftool | a shorter BPF loader | P-LIBBPF (resolved: the existing environment first; a library only if a needed function is shown missing) | not installed |
| Z3 4.8.12 (apt candidate, [R07 §2.4]) | SMT-LIB cross-checks | P-Z3 (resolved: not a condition of the first implementation) | not installed |
| `llvm-18-dev` (apt candidate, [R07] L126–127) + a Nidhugg build | explorer cross-check on pthread-only litmus programs | P-NIDHUGG (resolved: held) | not installed |
| compiler-rt (build from the LLVM tree; time untested, [R07 §9 item 6]) or a gcc `-fsanitize=thread` variant | TSan reference point | P-TSAN (resolved: excluded; no rebuild step) | not built |
| what nasa/bp's CI uses for C1's provider: TinyCBOR installed into `/`, a mutable PSP branch, bplib (SPEC §10.2) | C1's provider on the public BP basis | D22 (resolved: the public BP basis; installs are package decisions and are not made here) | not installed |
| GenMC | — | not possible: its README requires LLVM 19–22 ([R05 F11]) | — |

---

## P-LADDER — The first target: the validation ladder with case tracks

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table).** **Approved.** The validation ladder V0–V4 and the case paths are the first target. The R2 processing path is implemented in full first, while the reference sources and configurations of the other four cases are prepared; the earlier large minimal bundle is not required as a whole as the first completion condition. No effort figure is given for any rung. Effect in SPEC rev 1.2: §9.3, §9.4, §10.1, ADR-044 (approved, amended by ADR-050).

- **Question.** Approve the validation ladder V0–V4 with the case tracks as the first target (SPEC §9.3; ADR-044)? It replaces decision G0 of `STATUS_20261012.md` ("the first target is the thin MVC S0–S3a"). ADR-044 rejects the thin MVC as the first target, because its exit gates contain non-R2 items and the review's scale concern stands.
- **Items** (SPEC §10.1).
  - (a) The ladder with the case tracks as the first target.
  - (b) Effort for V1 or any rung. None exists: SPEC §9.3 estimates whole stages only, as [I]; a rung estimate is `unspecified_by_reference`.
  - (c) The order of V2, V3 and the comparison.
  - (d) The expansion rule.
- **The proposal** (SPEC §9.3, §9.4, ADR-044).
  - V0 comes first: the formal amendment (ADR-039–ADR-043). Its proof item **V0-P**: before any V1 result is worded as a certificate or a transferred cell, the lemma obligations of that wording are written and reviewed (R2's trust ledger, App. A.1 step 9a); the rest are written before F1.
  - V1 is R2's K-ID chain end to end on the ENV basis, through the sub-gates S0-R2, S3a-R2, S1-R2 and S2-R2, which take from the stage gates only the items R2's chain needs. The full gates of S0–S3a are unchanged and form rung V1b, which overlaps V1.
  - V2 (R2's control basis, P-R2CTRL) and V3 (C1 on its B′, D22) both follow V1 and V1b and do not wait for each other. The order of EXT-20261012 §7 (step 3, then step 4) is the order in which their results are reported, not a dependency.
  - V4a compares with same-information comparators after F1; V4b evaluates held-out sources and pre/post-fix pairs after F2.
  - Case tracks run beside the ladder: the paper track for all five cases now; C3 and C5 on the 6.4.x pair (VP-D3, D13); C2 role-derived (VP-D4, P-C2); the A.3 line (VP-D5, P-RGIT). A result on a v7.0.1 counterpart of C1, C2, C3 or C5 is never reported as evidence on that case.
  - Expansion rule: MLIR (S8), SMT (P-Z3), DPOR (`dpor` after `exh` in S7) and Φ templates that no rung, track or step needs are added only after a rung (V1–V3), a case track (VP-D3, VP-D4, VP-D5) or an ENV-window step (S5 and S6 on the App-record windows, S7 on the development cones) shows a need. Kinds without an authored template are reported as not authored (§7.7, RQ1), never omitted.
- **Options.** Approve or decline each item. The alternatives SPEC records, and rejects, in ADR-044: the thin MVC as the first target; a strict R2 → C1 ladder as the only validation; C1 first; V2 strictly before V3, as a dependency; summing the estimates into a plan figure.
- **Facts.** The external review's sum of the S0–S3a estimates (19–28 person-weeks) is correct arithmetic on [I] inputs. It is effort, not duration (S2 and S3a can overlap, §9.4), and it covers the thin MVC only. It is not used as a plan value (SPEC §9.3).
- **References.** SPEC §0.7 item 7, §9.3 (ladder table, V1 sub-gates, expansion rule, per-case paths and controls), §9.4, §10.2 (risks "Integration breadth" and "Validation concentrated on one case"), ADR-044; EXT-20261012 §7, §8; `STATUS_20261012.md` §3.1 (G0).
- **Recommendation.** The proposal of ADR-044; no effort figure.
- **Blocked until decided.** V1 and every later rung.

## T1 — Name of the judged object (decision of `STATUS_20261012.md`)

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table).** **(a)** The general name of the judged object is **"readiness specification" (준비 상태 명세)**; the precondition Req becomes "readiness precondition" and a violation "readiness violation". The source's term, such as "API contract", is kept only when the obligation of a documented API itself is described. A judgement made by the researcher is not called a "contract violation". The rename is done in one dedicated step (SPEC stage Spec-C and the mirror stage) and **is not applied to SPEC rev 1.2 or to this file**, which still say "contract".
- **Question.** Which name for the judged object: (a) readiness specification, with the grade of evidence, "API contract" only for what is documented; (b) keep "readiness contract"; (c) "readiness property"?
- **References.** `background/08_terminology.md`; `STATUS_20261012.md` §3.1 (T1).

## D20 — Safety statements and progress in the first implementation

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table, §2.1).** **Option A.** A certificate proves safety statements only; progress is reported separately, from bounded exhaustive search or from real evidence of permanent waiting; a safety verdict is never read as the absence of deadlock. Additional completion conditions (§2.1), applied in SPEC §0.9: a proof status per lemma (`proved`, `depends-on-stated-assumption`, `unproved`); a lemma only listed as an obligation is never printed as proved; the early-return `NOT_EVALUATED` is consistent with the transfer to real runs; L-CONT (c) is in the R2 trust ledger; the inference steps that the checker re-validated are told apart from the extracted facts, shared code and assumptions that it trusts. Effect: SPEC §10.1, ADR-039, ADR-045, ADR-049.

- **Question.** For the first implementation: option A or option B?
- **Options.**
  - (A) Certificates prove safety statements only (`¬OK_i`, `¬NR_i`); progress is a separate per-cell result, `unknown(not_checked)` wherever `exh` does not run (SPEC §1.10, ADR-039).
  - (B) Keep `S_i = ∅` in protected and add a progress obligation to certificates. ADR-039 rejects it for the first implementation, for four reasons: it needs a new lock-order analysis, must-occurrence with progress, spawn completion and checker steps of its own; it would withhold protected wherever M_impl models F7-type task loss; `S_i` is not preserved across σ; star-closure exits in `Sk^cert` make it unprovable for actors that wait on loopers.
- **Facts.** A progress certificate can be added later as a new progress value without changing any class (ADR-039).
- **References.** SPEC §0.7 item 1, §1.8, §1.10, §6.3 G2 and G11, §6.4 item 26, §9.3 (V0 row), ADR-039; EXT-20261012 §4.1.
- **Recommendation.** A.
- **Blocked until decided.** The reading of every certified verdict (ADR-039); V0's exit gate ("D20 decided", SPEC §9.3).

## QT-2 — STATUS decision T2, re-asked

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table).** **Approved, with the interpretation of the evidence corrected.** The evidence kind `requirement` is added. The SB rule for a send with no subscriber and R2's invalid-AppId error are kept apart: the first is not a statement that R2's rejected requests are no error. Effect: SPEC §1.6, ADR-048.

- **Question.** (a) Add the provenance kind `requirement`: a cFS functional requirement (`cfe/docs/cFE_FunctionalRequirements.csv`, an application's `*_FunctionalRequirements.csv`; SPEC §1.6 `prov`)? (b) Report the SB no-subscriber loss in three judgements, not as an allowed drop: a framework-level `documented-not-error` result (cSB4305.5) beside a caller-level LOSS-UNDER-ORDER, unless a consumer or mission reference decides, with the observed loss recorded (SPEC §0.7 item 4, §3.9, App. A.1 step 13a)?
- **Facts.** STATUS T2 recommended adding the kind and classifying the loss as allowed (`documented_drop`). SPEC rev 1.1 adds the kind and reports the loss as in (b), not as a `documented_drop`: a supplier's documented result never makes an occurrence TOLERATED (§1.6, ADR-043).
- **References.** SPEC §0.7 item 4, §1.6 ("Three judgements", `prov`), §3.9, App. A.1 steps 13 and 13a, ADR-043; EXT-20261012 §6.4; `STATUS_20261012.md` §3.1 (T2).
- **Recommendation.** As proposed (ADR-043).
- **Blocked until decided.** The caller-level Allowed (§1.6) and ADR-043; V0 (SPEC §9.3). ADR-043 is revised if the PI decides otherwise.

## QT-3 — Adopt the minimal three-judgement design of ADR-043

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table).** **Approved.** Three layers are kept apart: the framework's documentation and actual API behaviour; the caller's requirement; the observed result. That a framework allows a returned result does not by itself satisfy the caller's requirement. Effect: SPEC §1.6, ADR-043 (accepted), ADR-048.

- **Question.** Adopt ADR-043: `side ∈ {supplier, caller, rule}` on every provenance record; entries with a `caller` record are horizon monitors; `rule`-only entries are monitors under the default (QT-4) and reported as rule-backed; supplier-only entries decide nothing; a new field `appreq` with a precedence rule (it prevails over rules; a conflict with a `caller` entry leaves the occurrence NOT_READY_UNJUDGED until the PI sets a precedence, QT-5); framework-level `fw_doc` per instance and `fw_conf` per cell; `consequence` always filled; outcome-label names kept. No status value, class, horizon kind or monitor kind is added (SPEC §0.7 item 4).
- **Options** (the alternatives of ADR-043). Adopt; a full split of Allowed into two fields (more churn, same information); keep the mix (the documented error return of R2 and the documented drop of SB then decide nothing consistently); treat the generic rules as caller-side (then a documented return code plus a branch discharges the caller's obligation with no caller text).
- **Facts** (ADR-043 consequences). Under the defaults no status of the five cases' rows changes, by a walk-through of the five cases [I]; supplier-postcondition contracts without an `appreq` become `not-ready-unjudged`; one litmus expectation is restated; tiers and rule-backed acceptances are reported apart; the census reports Allowed entries by side.
- **References.** SPEC §0.7 item 4, §1.6, §1.10, App. A.8 (LP-ALLOW), ADR-043; EXT-20261012 §6.4.
- **Recommendation.** As proposed (ADR-043).
- **Blocked until decided.** The caller-level Allowed (§1.6) and ADR-043; V0.

## QT-4 — Rule-backed acceptance

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **Strict alternative adopted.** A researcher's rule alone never makes an occurrence accepted or normal; entries without an original-source basis stay unjudged (`unspecified_by_reference`); code state changes and error returns are still reported. Some labels disappear (rule-backed TOLERATED, VIOLATING that rested on a rule's REJECT); the observation of order, return value, recovery and final failure is not erased. Effect: SPEC §1.6, §3.3, §7.1, App. A.2, A.4, A.8, ADR-043 (amended), ADR-048. The text below is the question as asked; its recommendation (the default) is superseded.

- **Question.** Do `rule`-only Allowed entries decide statuses, with their acceptances reported as rule-backed, or are they `unspecified_by_reference`?
- **Facts.** Under the default a rule, not a caller reference, decides the statuses of `rule`-only entries: their ACCEPTs are reported as rule-backed TOLERATED, counted apart from caller-backed ones, tier `code-derived`, and a documented return code never discharges the caller's obligation by itself. Under the strict alternative, `rule`-only entries are `unspecified_by_reference` (SPEC §1.6). Example: litmus LP-ALLOW (b1r) is rule-backed TOLERATED under the default and NOT_READY_UNJUDGED under the strict alternative (App. A.8). Retry/timeout-loop-derived contracts are `rule`-side (§7.1 source 4).
- **References.** SPEC §0.7 item 4, §1.6 ("Caller-level Allowed"), §7.1, App. A.8, ADR-043.
- **Recommendation.** `rule`-only entries decide and are reported as rule-backed.
- **Blocked until decided.** The caller-level Allowed (§1.6) and ADR-043; V0.

## QT-5 — Precedence among caller-level texts

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **The conflict is stated and the final judgement withheld.** The version, target and scope of the documents are checked first; while the conflict is not resolved by them, no priority is invented. Effect: SPEC §1.6 (`appreq`), ADR-048.

- **Question.** Which text prevails when an `appreq` and a `caller` entry, or two `caller` entries, conflict? Named cases: C1's references; SB's guard drops, if cSB4305.3–.5 are recorded as SB's `appreq`.
- **Facts.** An `appreq` prevails over `rule`-only entries. A conflict with a `caller` entry is recorded, and the occurrence stays NOT_READY_UNJUDGED with the conflict named until the PI sets a precedence (SPEC §1.6, §13 `appreq`; litmus LP-ALLOW (a3) and (d5)).
- **References.** SPEC §1.6, §13, App. A.8, ADR-043.
- **Recommendation.** None chosen.
- **Blocked until decided.** The caller-level Allowed (§1.6) and ADR-043; C1's statuses once any of its other texts is authored.

## QT-9 — K-SPAWN.osal and K-LOCKBAL

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **Separated as an implementation-defect family.** No researcher requirement is added to turn K-SPAWN.osal and K-LOCKBAL into violations of a caller requirement; only the part that links to an order violation is reported, on a separate basis. Effect: SPEC §1.6 (`harm`), §3.3, §7.3, ADR-048.

- **Question.** Move K-SPAWN.osal and K-LOCKBAL to an implementation-defect family, or give them a researcher-authored `appreq`?
- **Facts.** Their `Allowed = none` rests on supplier-side texts only. Under the caller-level rule (`Allowed = none` requires a specified `appreq`) they are NOT_READY_UNJUDGED with harm `unspecified_by_reference`, and the effect is recorded as their consequence, until an `appreq` is recorded or the PI moves them (SPEC §1.6 `harm`). The timebase-handshake candidate is affected (§7.3).
- **References.** SPEC §1.6, §3.3, §7.3, ADR-043.
- **Recommendation.** None chosen.
- **Blocked until decided.** The labels of K-SPAWN.osal and K-LOCKBAL (§7.3).

## Q15 (design item) — The normal continuation of `returned_and_tested`

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **Strict alternative adopted.** The code fact that a follow-up path exists (extracted) is told apart from the judgement that the path is an allowed recovery; without a basis the judgement is not made. A `returned_and_tested` entry accepts only at a cited continuation; the nearest post-dominator of the test is an extracted fact, `cont_candidate`. Effect: SPEC §1.6, App. A.2, A.4, A.8, ADR-039 (revised), ADR-048.

- **Question.** Use the default of SPEC §1.6, or require every entry to name its continuation from a reference, with entries without one `unspecified_by_reference`?
- **Facts.** The default: the cited continuation, or else the nearest post-dominator of the test, per loop iteration, that is not the function's return and at which neither the status nor the not-ready branch's writes are live; a REJECT at the same node wins; an error exit or an error return is a REJECT; a documented propagation needs a reference (ADR-039, §13). At C1's buggy revision the stricter reading makes the entry `unspecified_by_reference` and the occurrence NOT_READY_UNJUDGED, with the same class (App. A.4 step 3). C1's pre/post-fix direction holds under either reading (§8.5 item 7).
- **References.** SPEC §1.6, App. A.2 step 3, App. A.4 step 3, §8.5 item 7, ADR-039.
- **Recommendation.** The default of §1.6.
- **Blocked until decided.** The outcome labels of the rows that rest on the default (App. A.2 step 3, App. A.4 step 3).
- Q16 is part of D13, Q22 of P-C2.

## D1 — Root-side observer and holder hook

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table, §2.2).** **Approved, explanation corrected.** A root observer attaches hooks to the cFS process; the cFS user and the reference binary are kept. The explanation "only time changes" is not asserted, not even as an assumption; an observation-effect check replaces it. The cFS user ID, command, priority and scheduling policy, environment and initial state are recorded. The observer and the control functions are kept apart; runs that use a hold, stop or release are labelled. Record loss, event-matching failures, boot and progress time and externally visible functional results are compared with an unobserved reference run; for internal events with no such reference, "no effect" is not concluded. Effect: SPEC §5.1, §5.9, §8.6, ADR-049.

- **Question.** May a root process attach perf-PMU uprobes, eBPF programs, per-task watchpoints and ptrace holds to the README-mode `ubuntu` process? This needs one new hook in `env/run_cfs.sh` next to `RUN_CFS_WRAP` / `RUN_CFS_HOOK`.
- **Options.** (a) Approve (fidelity class F0: unmodified binary and run conditions; only timing perturbed). (b) Refuse: only F1 (`--wrap`, a link-option deviation) or F2 (source probes) remain.
- **References.** [R07 §9 item 1], [R07] L355 (uprobe PMU needs `CAP_PERFMON` or `CAP_SYS_ADMIN`), L500; [R07 §4.7] (measured: uprobe 0.57 µs at entry+4; ptrace from root on uid 1000 works, no Yama); SPEC §5.1, §5.9, §9.3 (V1 and V1b rows).
- **Recommendation.** (a).
- **Blocked until decided.** S1 and V1's S1-R2 sub-gate, and everything that consumes traces (S2; reach and O1/O2 in S3a; S4–S6; V2's runs; RQ5, RQ7).
- **Default if undecided.** No Kairos observation runs.

## D17 — Stop method (and normalization boots)

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table).** **(a).** The CI ES POWERON restart command is the reference for the stop of normal terminations and for the normalization boot after abnormal terminations. The initial state and the termination record are kept. POWERON is not read as resetting every persisted file, file cache or shared file system. Effect: SPEC §3.5, §4.13, ADR-049.

- **Question.** Which stop method ends experiment runs, and which method normalizes persistent state after an abnormal exit?
- **Options.** (a) The CI ES POWERON restart command (`.github/workflows/build-run-app-reusable.yml` L209–214; E11 candidate (a) in [BM] L29, L343–344): it ends through `CFE_PSP_Restart(POWERON)`, which deletes the SysV segments (`cfe_psp_support.c:62–69`). (b) A signal stop: leaves the segments, so the next boot is a PROCESSOR reset ([R01 §5.2]: PO → PR → PO after signal stops). (c) Another documented method.
- **References.** [BM] L29, L343–344; [R2g] (runs ended by E11(a)); SPEC §3.5, §4.13 (normalization), §1.7, §9.3 (V1 row).
- **Recommendation.** (a) for both the stop of observation runs and the normalization boot after every run that did not end with the POWERON line; record it as a from-reference candidate. Note that every stop is a teardown, so uses after it are K-TEARDOWN instances, reported apart.
- **Blocked until decided.** V1's pooled runs and pooled runs in S1; confirmation runs in S6 (they are designed to end abnormally).
- **Default if undecided.** Runs are made one at a time with the Init snapshot S0 recorded, but not pooled.

## D14 — N (runs per purpose), as a deviation

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table).** **N is decided per purpose at S1** (S1 is the first stage that characterizes the observer): a functional check, the observation-effect comparison, and the evaluation of occurrence frequency are told apart. Each N is set from the pilot's results and the resource budget and is frozen before the corresponding main experiment. It remains a deviation; no value is chosen. Effect: SPEC §8.7, §9.3 S1, ADR-049.

- **Question.** How many runs per perturbation group, per loop iteration and per σ_E frequency?
- **Options.** The PI sets N per purpose as a recorded **deviation** (reason: finite resources; effect: bounded results with exact intervals or `unknown(budget)`; necessity: no reference defines N). Event-based stopping where it exists (loop fixed point, exhaustion). Methodological literature (Arcuri & Briand [V-abs]; Klees et al.'s ~30 trials, known only from a secondary summary, P) may motivate the choice but is never `from-reference` ([POL] L5–10).
- **References.** [POL] L16–21; [X3-M9]; SPEC §8.7, §9.3 (V1 row).
- **Recommendation.** Decide **S1's characterisation N at S1** (it is needed by S1's exit gate), including the `n of N` label of V1; V1's reach can be proved from the first observed occurrence with an event-based stop. Decide the evaluation Ns before S9.
- **Blocked until decided.** V1 (its `n of N` label); S1 exit gate (groups A, B, C′ comparison); S9.
- **Default if undecided.** No group comparison is reported.

## D11 — Re-baselining after the kernel change

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2.2, §4.1).** **Required, and before the S1 observation-effect comparison.** Results measured on the earlier kernel are not added to the current reference runs. Effect: SPEC §5.10, §8.6, §9.3 S1, ADR-049.

- **Question.** Re-measure group A on `6.18.44-fc-v114` before any comparison? (The records were made on `-fc-v80`.)
- **References.** [R07 §1]; [BM]; SPEC §5.10, §9.3 (V1b row).
- **Recommendation.** Required, in S1.
- **Blocked until decided.** V1b (the S1 gate); every perturbation comparison.

## P-LIBBPF (was D2) — eBPF userland

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.4).** **Proceed with the existing environment first.** Writing the loader must not become a separate large research task. If a needed function is shown to be missing, a maintained public library is considered and the environment change is recorded. Effect: SPEC §2.6, §5.11.

- **Options.** (a) Kairos's own raw `bpf()`/`perf_event_open` loader with hand-written helper prototypes and a glibc `<elf.h>` ELF reader (no install). (b) Install libbpf/bpftool (recorded package decision).
- **References.** [R07 §2.3, §4.7] (libbpf headers absent; raw `bpf()` load and uprobe attach probed); [J3 §2.4] (UAPI `linux/bpf.h` present); SPEC §5.3.
- **Recommendation.** (a); install only if S1 shows that attach cookies, ring buffers or task storage cannot be used without it.
- **Blocked until decided.** Nothing, unless S1 fails.

## P-TRACEFS — tracefs dynamic events

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md`).** **Open.** The resolved-decisions record does not address this decision; the recommendation below stands as the proposal.

- **Question.** May Kairos create tracefs uprobe events? They are kernel-global and outlive a private mount namespace [I] (a host-state change).
- **References.** [R07 §4.7] (tracefs mountable in a private namespace; no events created); [R07] L340 (perf-PMU uprobes work without tracefs); [X3-m2]; SPEC §5.4.
- **Recommendation.** No; the perf-sample backend is the zero-install bring-up path. If ever approved: a dedicated instance, an explicit cross-CPU clock, events removed at exit, a recorded host-state change.
- **Blocked until decided.** Nothing.

## D10 — Memory-model stance

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **Assumptions are marked and the guarantee is limited.** Every result that uses the event-granularity visibility assumption is marked. The x86 TSO assumption alone does not establish the correctness of the source language, the compiler or the whole execution. Where it is unclear whether the assumption holds, the result is undecided. Effect: SPEC §3.14, §6.1 A4, §6.4 item 20.

- **Options.** Assumed visibility (A4: sequential consistency at event granularity on x86-64 TSO at `-O0`), flagged on every verdict that uses an `assumed` edge; or report the ad hoc synchronization sites as findings.
- **References.** [R06 T8]; [R02 §8.3]; SPEC §3.14.
- **Recommendation.** Assumed visibility, flagged.
- **Blocked until decided.** The final wording of A4 and of the `assumed-visibility` sub-label.

## D9 — Scope of the first paper

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **The first paper's required scope is limited to the five cases and the controls they directly need.** New families for persistent state, timers and files are recorded where a case needs them as model input, and are not enlarged into independent goals; message-capacity loss does not replace the validation of a case. Effect: SPEC §8.1, §7.3.

- **Question.** Are RC8 (persistence), RC9 (parent ↔ child), RC10 (timer → task), RC12 (file TOCTOU) in scope as process-level; how is class M (memory-level races found in passing) reported; are route/capacity losses (LOSS-UNDER-ORDER) in the first paper?
- **References.** [R06 §8 Q6]; [R03 §4]; SPEC §3.15, §9.3 (V1b row).
- **Recommendation.** Include, with the labels: RC8, RC9, RC12 marked partial; class M and LOSS-UNDER-ORDER reported separately.
- **Blocked until decided.** The denominators of RQ1 and RQ5; V1b.

## D12 — Citing the retired recorder's observations

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1, §6).** **CORD results are not used as evidence in current research documents.** The input's recommendation to cite them as earlier observations is changed. The R2 execution evidence is the raw gdb records (`env/logs/r2_g1..3_*`, `results/R2_gdb_v701.md`). The citation cleanup (§6.2) is a separate step and is not done in SPEC rev 1.2; the policy to remove the CORD products from the working tree (§6.1) is decided, and its execution is not recorded here. Effect: SPEC header (Inputs), ADR-049.

- **Options.** Cite CORD's and the M1 harness's observations as prior observations; or only after Kairos re-observes them.
- **References.** [R2c]; [BM §3.3]; SPEC App. A.5.
- **Recommendation.** Prior observations only; S1/S2 re-observe.
- **Blocked until decided.** The wording of RQ5 and A.5.

## D15 — Work-unit caps, as deviations

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.2).** **Budgets are in deterministic units of work** (predicted nodes, search states, transitions). Each cap is frozen after that stage's pilot; an excess is reported as `unknown(budget)`. No value is chosen. Effect: SPEC §4.10 item 9, ADR-049.

- **Question.** The caps on prediction search nodes, explorer states/transitions, O8 search and skeleton construction, also used by the explorer's kill criterion.
- **Options.** PI-set caps in deterministic work units, each a deviation (reason: finite resources; effect: `unknown(budget)` with anytime curves; necessity: no reference defines them). A wall-clock cap only as a labelled exception excluded from the byte-exact replay test.
- **Facts.** The caps also govern the only model progress route (the progress candidates of SPEC §4.6, run by the explorer) and apply to the closed cone of §4.10 item 1 (D21). V3, VP-D3 and VP-D4 list D15 among their needs (§9.3).
- **References.** [X3-M9], [X3-M10]; SPEC §4.6, §4.9 step 8, §4.10 items 1, 6 and 9, §4.11, §4.15, §9.3.
- **Recommendation.** Work units only; set before S5 (prediction, O8) and before the S7 review (explorer).
- **Blocked until decided.** S5's budget curves; the S7 kill criterion; every model progress result.

## D16 — Derived App-record probes

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **Approved; labelled derived.** The validation of C5's original phenomenon goes first; the present residual windows are a separate validation; neither is confused with C1's holds (P-C1HOLD). Effect: SPEC §9.3 S6.

- **Question.** Run the derived F0 holds that open the core-task and script-app App-record windows ([LED] X5)?
- **Scope.** App-record probes only (SPEC App. A.2, A.3). C1's holds (App. A.4 step 5) are P-C1HOLD.
- **References.** [LED §3.4, X5]; SPEC App. A.2, A.3; §4.13 (placement, watchdogs incl. `CFE_PSP_RESTART_DELAY`, normalization boot).
- **Recommendation.** Pursue, labelled derived, with normalization boots after abnormal exits.
- **Blocked until decided.** S6's exit gate.

## D3 — F1 `--wrap` gates, PCT, perturbation group W

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.2).** **Later (low priority).** Whether the present event-based holds give the needed checks is evaluated first; the gates and PCT are added when an insufficient question is confirmed. Effect: SPEC §9.3 S10, §8.6 (group W), §6.4 item 56.

- **Options.** Approve the link-option deviation for PCT and many-point schedules (and group W, which measures the variant's code-layout effect); or F0 holds only.
- **References.** [R07 §4.2] (`--wrap` misses 88 of 3,762 intra-TU sites), §5.1 (handoff 1.1/10.5 µs), L423 (≈15 min per variant build); PCT (ASPLOS 2010 [V-abs; mechanism P]); SPEC §4.13, §8.6.
- **Recommendation.** Defer until S6 shows whether F0 holds suffice; this is the "native PCT arm".
- **Blocked until decided.** S10's PCT arm; G7; group W.

## P-PAPERS — Access to primary papers

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.4).** **Approved; start immediately.** Accessible routes are used first (author copies, institutional repositories, linked scholarly search). That one working environment is blocked does not make a PDF supplied by the user a precondition of starting. The record states that the author-public texts of RacePro and GPredict were confirmed in the review before the decision; the accessibility of other papers is not assumed, and the expressiveness or observation range of an unread tool is not asserted from search excerpts. Effect: SPEC §1.11, §8.3, ADR-046.

- **Question.** Supply primary PDFs (or allowlist hosts) so that statuses P / UNVERIFIED can be checked.
- **Items.** (1) The JACM 2017 version of source/optimal DPOR with disabling (T-JACM; status P in [R05 §2.2]); (2) the HB rule tables of DroidRacer, CAFA, EventRacer and DCatch ([R04 §8 item 3]; DCatch is P); (3) a citation for the left/right-mover reduction behind L3 (not verified in R04/R05); (4) the guarantee wording of DPOR (POPL 2005, UNVERIFIED; FG05); (5) a targeted search of RV/TACAS for timeout-conditional HB edges (G3 novelty, [R04 §8 item 5]); (6) the papers' worked examples for the comparators' golden tests; (7) GPredict §II.B, its property formalism: whether it has each feature of the expressibility table of SPEC §8.3, on which the faithfulness of the same-information encoding depends ([U] until read); (8) Joshi & Sen (ASE 2008) and Lifestate (ECOOP 2019), before any difference from them is claimed (SPEC §1.11).
- **References.** [R04 §8]; [R05 §2.2, §8]; SPEC §1.11, §8.3, ADR-037, ADR-042; `availability.md` A1 proposes `research/cfs_procrace/paper/` as the place.
- **Recommendation.** Approve.
- **Blocked until decided.** G5 for the `dpor` engine stays "unverified" (the `exh` engine is unaffected); L3's citation; comparator golden tests (V4a); the G3 novelty statement (until then: "open literature check"); the GPredict characterization of SPEC §1.11 and its same-information encoding (§8.3); the framework-typestate difference claimed in §1.11.

## P-NIDHUGG — Explorer cross-check with Nidhugg

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.4).** **Held for the first implementation.** It is introduced later only when the need for a cross-check on small models and the input range it supports are confirmed; it does not replace the comparison among process-level tools. Effect: SPEC §8.3, ADR-046.

- **Question.** Install `llvm-18-dev` and build Nidhugg to cross-check the explorer on litmus programs that use only pthread create, join and mutex?
- **References.** [R05 F11] (Nidhugg README supports LLVM 8–19; "each thread should be deterministic when run in isolation"); [R07] L126–127 (`llvm-18-dev` is an apt candidate, not installed); [R05 §6.3] L452 (run such tools on the same closed harnesses); GenMC needs LLVM 19–22 (not possible).
- **Recommendation.** Defer to the S7 review; install only if the explorer survives its kill criterion.
- **Blocked until decided.** The optional cross-check only.

## P-Z3 (was D5) — SMT solver

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.4).** **Not a necessary condition of the first implementation.** Introduction is considered when a concrete reason is found why a prediction or search question of the specification cannot be handled by the present implementation. Effect: SPEC §2.6, §9.3 (Expansion).

- **Options.** Z3 4.8.12 from apt for SMT-LIB cross-checks on small cones; or explicit search only.
- **References.** [R07 §2.4]; SPEC §4.9 step 10, §8.3.
- **Recommendation.** Explicit search only. The GPredict-style same-information comparator is implemented as an explicit equivalent search; its SMT-LIB variant waits for this decision.
- **Blocked until decided.** SMT cross-checks only.

## D6 — MLIR gate

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.2).** **Excluded from this roadmap.** The compiler-intermediate-representation stage is not revived as a mandatory or an optional step: S8 and the G-MLIR gate, with their claims, ablations and risks, are removed. Effect: SPEC §2.6, §9.1, §9.2, §9.3, §6.4 item 15, ADR-018 (superseded), ADR-051.

- **Options.** Run G-MLIR at S8 (verdict equality required; provenance completeness, code size, runtime); or defer.
- **References.** [R06 T1, K8]; SPEC §2.6, ADR-018.
- **Recommendation.** Defer to S8; no ODS/TableGen before the gate passes.
- **Blocked until decided.** S8 only.

## D4 — Native σ_R1 arm

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.2).** **Initially a model-based single-core real-time interpretation.** A real-priority execution is validated separately, with its own configuration and conditions, after its necessity is confirmed. A model result is never labelled a real real-time execution result. Effect: SPEC §9.3 S10, §8.8, §6.4 item 56.

- **Question.** Realize σ_R1 natively, in addition to the model?
- **Facts.** When RT scheduling is permitted, OSAL POSIX selects `SCHED_RR` on Linux (FIFO's spread is never > 255), gives the root thread (`CFE_ES_Main`) the highest priority, compresses priorities, and uses priority-inheritance mutexes (`os-impl-tasks.c:78–106, 348–365, 391–409`; `os-impl-mutex.c:82`). A root-imposed `SCHED_FIFO` would be overridden by OSAL's own selection. Running with RT privileges as root also sets euid 0, which removes the `msg_max` queue truncation (`bsp_start.c:56–74`), so it is a new basis B′.
- **Options.** (a) Model-only σ_R1[`P_sched^posix`]. (b) Native, recording OSAL's own choice (RR, root at max) as the realized σ, as a B′. (c) Native with an override of OSAL's policy (a deviation).
- **References.** [X2-07]; [R07 §5.2]; SPEC §1.8, §7.4, §9.3 (VP-D3 row).
- **Recommendation.** (a); if a native arm is wanted, (b), never (c).
- **Blocked until decided.** S10's σ_R1 arm; native evidence for H-V. SPEC §9.3 also lists D4 among the needs of case track VP-D3.

## D18 — Configuration and input variants B′

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md`).** **Open**, except for its C1 part, which is folded into D22. The resolved-decisions record does not address the other variants (tables that activate latent paths, alternate schedules, script and core orders, cpu2, `msg_max` and euid, reset type, inputs). P-R2CTRL options (c3) and (d) depend on it only if they are added later.

- **Options.** Per SPEC §7.4: tables that activate latent paths (CF `sem_name` with a provider, decided under D22 before V3; LC, MD, HS AMT/EMT/XCT, CS); alternate schedules; script and core orders; cpu2; `msg_max` and euid; reset type (PROCESSOR with preserved CDS and a volatile script); inputs (`B_restart`, table commands, `B_stop`).
- **Core orders.** A `MISSION_CORE_MODULES` order is a configuration-change experiment, never schedule evidence. For P-R2CTRL option (c3), one v7.0.1 list drives both the EarlyInit and the TaskMain phase, so a transplant of the pre-`b9bec567` task order also sets an EarlyInit order and the positions of `config`, `fs`, `osal`, `psp`, `msg`, `sbr` and `resourceid`; these are `unspecified_by_reference` and are set by the PI as recorded deviations under D18 (SPEC §7.4).
- **References.** SPEC §7.4, §9.3 (V2 row).
- **Recommendation.** Latent-path tables and `B_restart` first; nothing found only in a variant is a "new bug" of the default configuration.
- **Blocked until decided.** §7.4 results (S9); P-R2CTRL options (c3) and (d).

## D19 — `msg_max` variant

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.1).** **The current reference value `msg_max` = 10 is kept.** Queue loss is recorded together with the configuration. If a loss obstructs the target path, a sensitivity configuration is designed separately; the value is never changed silently. Effect: SPEC §7.4, §8.1 (RQ5).

- **Options.** Keep 10 (the recorded Env, [BM §3.1]); or a recorded variant.
- **Recommendation.** Keep 10 for the default B; a variant B′ only for RQ5 sensitivity.
- **Blocked until decided.** RQ5 sensitivity only.

## D13 — The 6.4.x derived platform

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §3.4).** **(a) accepted:** an analysis-only parse of a reconstructed compile DB is accepted as code-reading evidence, as an analysis-only condition that differs from the original; it is not accepted as a reproduction of the original platform's run. **(b) starts with (i):** the first parse uses this container's 32-bit headers and is recorded as a deviation (the container is not the build host that the 6.4.1 documentation lists); the generation settings, the include list, the integer and pointer sizes, the source paths that macros select and the interpretation of structures and return values are checked. If the relevant paths or type interpretations prove sensitive to headers or environment, it switches to **(ii)**, obtaining the literature host's sysroot (an install decision); until the sensitivity is resolved, the affected results are withheld. **(c) before F1.** **Q16:** the version-by-version correspondences of states, return values and source locations are stated and tested; a failure of input or correspondence is not counted as a failure of the technique; an actual execution on a past code base is reported as a product separate from analysis-only work. Effect: SPEC §8.2, §9.3 S10 and VP-D3, §6.4 item 51, ADR-050.

- **Question.** Build the shared derived 6.4.x platform for C3/C5 at 6.4.1 and for H-V?
- **Facts.** The 6.4.1 bases lack a compile DB (risk rated high); an RT claim also needs a **cited scheduling profile** for that backend (no σ_R1 verdict without it).
- **Options.** Build (reconstruct the compile DB from the 6.4.x make system) and cite its `P_sched`; or code-reading evidence only.
- **Sub-items** (SPEC §10.1).
  - Q16: the Φ_6.4.x outcome map and binding, and the 6.4.1 references for the Allowed entries of C3 and C5.
  - (a) Does an analysis-only parse of a reconstructed compile DB count as "code-reading evidence only"?
  - (b) The compile-DB inputs: OSAL artifact and placement, host, gcc version, include set, each listed with its reference status (SPEC §10.2); and C3's derived basis recorded as a `deviation` from the Microblaze platform its issue reports ([LED §1b]), carried into C3's RQ2 and H-V reporting (SPEC §10.1).
  - (c) Placement of the basis before F1, or C3 and C5 recorded as post-freeze by design (SPEC §8.2). The 6.4.x basis is the S10 arm (case track VP-D3) and is placed before F1 only if (c) so decides (§9.3). Cutting S10 removes C3's and C5's tool-level evidence and C2's 6.4.2 row, unless (c) has placed the basis before F1 (§9.3 cut list).
- **References.** [LED] X3; [R06 §4 T5]; SPEC §7.6, §8.2, §9.3 (S10 row, VP-D3 row, per-case rows of C3 and C5, cut list), §10.2.
- **Recommendation.** Raise now; no option recommended.
- **Blocked until decided.** VP-D3 (C3's and C5's first tool-level evidence); RQ2 for C3 and C5; H-V; P-F1GATE. Also C2's 6.4.2 row (VP-D4), which needs D13 (SPEC §8.2 C2 row, §9.3 VP-D4 row and cut list); SPEC §10.1's D13 row does not list it.

## D8 / P-RECALL — Held-out sources and the recall protocol

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.3).** **The five cases first; the additional evaluation procedures are separate** (the extended evaluation). The five design cases and their fixes are excluded from the unobserved evaluation sets. The query, the exposure and the freeze of new material are recorded. Results on variant examples are reported as results limited to the variant set. **Still open:** the mutation operators, the history query (keywords, matched field, date cut), the expected-verdict and equivalent-mutant rules, who runs the queries, and whether other items that are only named in the specification are excluded. Effect: SPEC §7.5 item 4, §8.1, §8.5, ADR-051.

- **Question.** Which held-out sources are used; what exactly is frozen for the recall protocol; who runs the queries and when relative to F2?
- **Items to freeze before F2.**
  1. The mutation operators (remove a wait; move a publication after a spawn; swap object-table entries; empty a retry bound; drop a status check). Each is shaped on a design case and recall is reported per operator with that shape named (SPEC §8.5 item 1).
  2. The history query over the offline cFE (2047 commits, full history), OSAL and app histories: keywords, matched field and case, and a date cut. The 23 cFE commits whose full message matches `race|startup sync|order` case-insensitively at the pin are a starting set whose relevance is untriaged; matching subjects only gives 8 case-sensitive or 11 case-insensitive; over the CF history the same keywords select C1's own fix `833fdbb` (SPEC §8.5 item 2; [X3 §1]).
  3. The expected-verdict rule (from the fix's issue text and commit; [POL] reference kinds L7–8), including whether the expectation concerns readiness or progress, and any class statement of the fix text recorded with its citation (SPEC §8.5 item 3; P-CLASSGT).
  4. The equivalent-mutant exclusion rule (mutated site `inactive(B)` with proof, or byte-identical objects).
  5. The held-out issue query (keywords, date cut), excluding C1, C2, C3, C5 and R2. An issue or fix already named in SPEC or in the case records is labelled non-blind (SPEC §7.5 item 2 (c)).
  6. The exposure and freeze record of every held-out and recall item (SPEC §7.5 item 5).
- **Exclusion question** (SPEC §7.5 item 4). For both queries alike: do they exclude the design cases' fixes that lie in the queried histories (CF #184's `833fdbb`; cFE #198's barrier commits; the 6.4.2 fixes of #72 and #73 predate the git import) and the items already named in SPEC or in the case records? Until decided, such items are kept and labelled non-blind.
- **References.** [R06 §5.6]; [R05 §6.1 (iii), §6.2]; [X3-C1]; SPEC §7.2, §7.5, §8.5.
- **Options.** All five held-out sources plus the recall protocol; or a subset. Who: an author of Φ, or someone who did not author Φ. Exclusion: yes or no, per query.
- **Recommendation.** All five sources and the full recall protocol, frozen before F2, with the queries run by a person who does not author Φ; no blind claim before the exposure record exists; until decided, named items are kept and labelled non-blind; the exclusion is the PI's choice.
- **Blocked until decided.** S9; F2; RQ3; RQ9.

## P-TRIAGE — Raters and sampling

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.3).** **Two independent raters for any quantitative judgement that is feasible.** The panel is composed so that at least one rater is not an author of the relevant rules; prior exposure and disagreements are recorded. Before the people are secured, an independent review is not claimed as done and the corresponding quantitative precision is not claimed. Effect: SPEC §8.5 (triage), ADR-051.

- **Question.** Who rates, how they are masked, how large the stratified samples are, how each rater's exposure is recorded, which agreement statistic is used.
- **Facts.** Rater masking is assessor masking, not dataset blinding; each rater's prior exposure to the design cases, their fixes and Φ is entered in the exposure record (SPEC §7.2, §7.5 item 5, §8.5).
- **References.** [X3-M7]; [R05 §6.2] (two independent raters with a disagreement log); SPEC §8.5.
- **Options.** Two raters (authors or not), masked to the producing layer, the ablation and each other; sample size per (class, RC kind, deciding layer) stratum as a deviation; agreement statistic recorded as an assumption.
- **Recommendation.** Two masked raters, at least one not an author of Φ; each rater's exposure recorded; sample sizes recorded as deviations with exact CIs; LOSS-UNDER-ORDER checked against the SB oracle, not rated.
- **Blocked until decided.** Triaged precision (RQ3) and the triage-based labels used by ablations and comparators.

## P-STATS — Pre-registration of the statistical procedure

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.3).** **The first case evaluation uses descriptive statistics and per-case evidence.** Cause, fix, static site, occurrence and run counts are kept apart; no automatic significance test treats the five cases as independent samples. A separate quantitative experiment fixes its unit of analysis and sample design first, and its procedure is frozen after that. Effect: SPEC §8.7, §8.4, ADR-051.

- **Question.** Approve the procedure of SPEC §8.7 as a recorded **assumption** (methodological literature is not a reference kind under [POL] L5–10): full distributions; Mann–Whitney U for times and counts over runs; Fisher's exact test for incidence over runs; a paired test for per-unit detection comparisons (§8.3, §8.4); standardized effect sizes; exact binomial CIs; interleaved group order (also an assumption). Every test and interval states its unit (U1–U6, §8.2) and its independence assumption, and the handling of clustering by cause and fix is recorded.
- **Options for the dependence handling** (SPEC §8.7; none chosen). Paired detection comparison and ablation deltas: aggregate to U1/U2 clusters before testing, or report per unit with a cluster-level summary, or descriptive only. Recall intervals: report per mutant and per U2/U1, and read the interval only for the frozen mutant population. Triage precision: a design-based estimate for the instance population, with distinct causes reported beside it; no interval over causes unless the PI adopts one. Fisher's exact test and Mann–Whitney U: test only under the recorded assumption, otherwise "n of N (pool key)". Time to first detection: descriptive distributions.
- **References.** SPEC §8.2, §8.3, §8.4, §8.7, ADR-042.
- **Recommendation.** Approve before F1; the dependence handling is chosen from the options of SPEC §8.7, none recommended here.
- **Blocked until decided.** S9 analysis (V4a, V4b).

## P-VALGRIND — helgrind and DRD as an external reference point

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.4).** **Excluded.** Helgrind and DRD are not added as external reference points. Effect: SPEC §8.3, §5.1, ADR-046.

- **Question.** Run valgrind's helgrind and DRD on the unmodified binary through `RUN_CFS_WRAP`, as gdb was run ([R2g] L11)?
- **Facts.** valgrind 3.22 with helgrind and DRD is installed and ran a test program ([R07] L156, L346); it serializes threads, so its schedules are not README-mode scheduling ([R07] L466); slowdown on cFS unmeasured. It is excluded as a *baseline* by the user's decision ([R06 S13]) but allowed as a labelled reference point.
- **Options.** Approve as a labelled reference point (deviation: wrapped run command); or omit.
- **Recommendation.** Approve: it is the only external tool that runs here without an install, and it gives one comparison that does not depend on Kairos's own probe plan.
- **Blocked until decided.** The external reference row of SPEC §8.3 (S9).

## P-TSAN (was D7) — ThreadSanitizer reference point

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.4).** **Excluded.** There is no rebuild step for a ThreadSanitizer comparison. Effect: SPEC §8.3, ADR-046.

- **Options.** Omit; a gcc `-fsanitize=thread` variant build as a labelled deviation; or clang TSan after building compiler-rt (absent, [R07] L129; build time untested).
- **References.** [R04 §8 item 4]; [R05 §6.3 (c)]; [R07 §2].
- **Recommendation.** Omit unless a reviewer asks; P-VALGRIND covers the memory-level reference point.
- **Blocked until decided.** That reference row only.

## P-NAME — Naming

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §2 table).** **Kairos is kept as the working name.** A name-collision check is made just before publication. No expansion of the name and no academic meaning of the name is invented. Effect: SPEC header.

- **Question.** Keep "Kairos" (prefix `kz`)?
- **Facts.** The name is a working label taken from proposal C; no name-collision or trademark search was made.
- **Recommendation.** Keep it as a working label; run a collision check before any publication or public repository.
- **Blocked until decided.** Publication only.

## D21 — The cone of the S7 kill criterion

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.2).** **The closed dependence scope is adopted:** the search covers the tasks and resources that affect the use. A result obtained on an arbitrarily smaller scope is not used with the same guarantee. Effect: SPEC §4.10 item 9, ADR-049.

- **Question.** Is the S7 kill criterion assessed on the closed cone of SPEC §4.10 item 1; on the v1 cone, as an ablation labelled under-approximate; or on a reduction specified first with its own proof?
- **Facts.** The closed cone is computed in S3a for every instance and reused by the explorer, L-MLEVEL, the reach checks and the prefix probes (§9.3 S3a row). Its expected size is [I]: the closed cones of the five kill-criterion instances contain the startup thread and every core main task created before the instance's actor; the C1-derived, K-PHASE and `SBNSubPipe` cones also contain most script apps. This is not measured; S7's anytime curves will show it (§4.10 item 1). No cap is chosen here (D15).
- **References.** SPEC §4.10 items 1 and 9, §9.3 (S7 and V3 rows).
- **Recommendation.** The closed cone, caps unchanged.
- **Blocked until decided.** The S7 kill criterion; every model progress result.

## D22 — C1's derived bases

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §3.2).** **The public BP code basis is adopted; the candidate pair for the before/after comparison is CF `281a941` → `833fdbb`.** Values that the original sources show, for the public configuration (nasa/bp `b051eb0`): the semaphore names `BP0_tsem` and `BP1_tsem` (the public CF configuration table, `.github/buildconfig/tx/tables/cf_def_config.c` L40 and L54, re-read for this file); the startup order CF then BP and the configured priorities CF 65 and BP 55 (`.github/buildconfig/cfe_es_startup.scr` L6 and L7, re-read for this file). The case record is `cases/C1.md`; the fix is `833fdbb`. **Correction:** the statement that the name, the order and the priority have no reference is withdrawn for the public configuration; the values of the public configuration and the values of the original reporter's environment are told apart. **Not confirmed automatically:** the full version combination of cFS, cFE, OSAL and PSP and the actual content of the mutable references at the time; whether the reporter's provider is identical to the public BP configuration; the real-time priority application, CPU count and affinity in the real operating system; the configuration and code differences that moving to other versions would need. **Order of work:** the version compatibility of the public integration is checked first; if it cannot be built as it stands, the differences of a move to other versions are recorded; the reporter's CF `2a024d8` is not mixed with the public BP table; the v7.0.1 default configuration (empty names) is used only as a path-inactive negative control. Effect: SPEC §9.3 V3, App. A.4 steps 2 and 6, ADR-044, ADR-050.

- **Question as asked.** C1's derived bases B′ and B′_buggy. The provider, the name, the startup order and the priorities were stated as `unspecified_by_reference`. **That statement is withdrawn for the public configuration** (see the resolution above): the public values of nasa/bp `b051eb0` are from-reference for that configuration, and stay unconfirmed for the original reporter's environment.
- **Options** (SPEC §10.1, §8.2, App. A.4 steps 2 and 6).
  - A BP-based option and a role-derived option, on a derived B′: the bug-bearing subject is CF `2a024d8` (the reported revision) or `281a941` (the fix base); the control is CF `833fdbb`, the minimal pair with `281a941`.
  - A v7.0.1 option: CF `15a871e` on a v7.0.1 B′ (App. A.4 step 2). From references: the names `BP0_tsem`/`BP1_tsem` (nasa/bp `b051eb0`, for another CF revision and table layout); the provider protocol, create in init (`OS_CountSemCreate` from `BP_FlowInit`, initial count 10); CF before the provider in BP's CI script; CF `15a871e`'s 25 × 100 ms retries (`cf_internal_cfg.h` L172–L173, L184–L185); CF's ENV startup entry at priority 70. `unspecified_by_reference` at v7.0.1: the provider's build (nasa/bp needs a TinyCBOR install, a mutable PSP branch and bplib) or a role-derived provider's code, name, entry, stack, priority and script position; the channel count; RT enforcement, CPU count and affinity; any provider delay (event-based holds only); the hang bound of App. A.4 step 5's declared experiment.
  - (iv) Only if the PI wants arm A4 (final failure with no provider for the name) realized: a configured name with no provider, with every condition `unspecified_by_reference` and set as a recorded deviation. Without it, A4 has no referenced basis and is shown by litmus L-PATIENT variant 2 and LP-CASE-C1, or recorded as "not realized: no referenced configuration" (App. A.4 step 6).
- **Folded in.** D18's C1 part (CF `sem_name` with a provider, SPEC §7.4).
- **Facts.** In the default B both CF lookups are `inactive(B)` (`cf_def_config.c:60, 78`); that result tests configuration closure only and is not C1 evidence (§9.3). C1's own rung V3 also needs C1 D2–D4 (`cases/C1.md`), P-C1HOLD, D15, D21 and P-D18GATES (a). Choosing nasa/bp's provider needs package decisions (§10.2).
- **References.** SPEC §7.3 (K-PHASE row), §7.4, §8.2 (C1 row), §9.3 (V3 row; C1 row of the per-case table), §10.2 (risks on C1's provider and on the S5/S7 dependence), App. A.4 steps 2 and 6.
- **Recommendation.** None chosen.
- **Blocked until decided.** V3 (needed before it, not by S9); the C1 rows of App. A.4 and §7.3; the S5 gate's C1 item and two of the five S7 kill-criterion cones, unless P-D18GATES decides otherwise.

## P-R2CTRL (new) — R2's corrected-order control basis (ladder V2)

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §3.1).** **(c2) first, (c1) secondary, (b) as an optional control.** (1) First: the natural regression pair `b9bec567^` and `b9bec567`; the related start order, the affected tasks and any concurrent change are checked first. (2) Secondary: `v7.0.0` against the current baseline; the whole distribution differs, so it is not interpreted as an experiment that changed the start order alone. (3) Optional: PR #2820, worded as **the change applied as a no-change control** and not as "the PR reverted"; the original PR's change and the change ported to the current version are told apart. (4) Directly built core orders (c3, d), the lazy-resolution implementation (e) and model variants (f) are not required in the first validation; they are added when a question remains that the natural pair cannot answer. **Criterion:** whether the earlier fixed-order verdict is withdrawn, as appropriate, where the order evidence differs; the withdrawal alone does not establish overall safety, and a configuration or version difference is not evidence of the schedule dependence of the same binary. Effect: SPEC §7.4, §8.2, §9.3 (V2 and R2 row), App. A.1 step 15, ADR-050.

- **Question.** Which basis, if any, is R2's control with the publication order corrected (EXT-20261012 §7 step 3)?
- **Facts.** R2 has **no merged fix** [F]: #2663 is open and no nasa/cFE commit references it, so it has no pre/post-fix pair (SPEC §8.2, §9.3). The v7.0.1 core order comes from `MISSION_CORE_MODULES`; the mechanism is documented ("Missions may add/remove/replace components in this list as needed. For the CFE core, the order of this list is the order in which they will be initialized", `cfe/cmake/mission_defaults.cmake` L25–L26). No reference gives an order with `sb` before `es` and `evs` (SPEC §7.4).
- **Options, with reference status** (SPEC §10.1, §7.4, §8.2, §9.3, App. A.1 step 15).
  - (a) None. V2's default is "no referenced control" (ADR-044).
  - (b) The PR #2820 head (`5bc6a792`, closed unmerged [F]) backported: a no-change control whose verdict must not move; fixed-order is expected to be kept.
  - (c1) The bundle tag `v7.0.0`, under its README: a public bundle tag [F]; the tags `v7.0.0` → v7.0.1 are a substitute pair.
  - (c2) `b9bec567^`/`b9bec567` ("Fix #197, dynamic object table creation"): a natural regression pair for the ES half, i.e. the inverse of a fix [F].
  - (c3) The pre-`b9bec567` core order written into v7.0.1's `MISSION_CORE_MODULES`. The only order with public precedent is the **task** order EVS, SB, ES, TIME, TBL before `b9bec567`, which corrects R2's ES half only. The precedent covers the task order only: one v7.0.1 list drives both the EarlyInit phase and the TaskMain phase (`cfe_es_start.c:751`, `:775`), and ES's entry carries the CDS EarlyInit (`cfe_es_objtab.c`), so a transplant also sets an EarlyInit order and the positions of `config`, `fs`, `osal`, `psp`, `msg`, `sbr` and `resourceid`, which are `unspecified_by_reference` and set by the PI as recorded deviations under D18.
  - (d) An SB-first `MISSION_CORE_MODULES` order: the mechanism is documented, but no reference gives such an order; a D18 variant.
  - (e) The lazy-resolution proposal implemented: a code change whose expectation is written and frozen only after the code exists, because the halves it moves and whether the App. A.2 chain exists depend on the code written.
  - (f) A model-level fix-direction mutant: only the static part applies; no run, O4 or reach is exercised.
- **Expected record** (pre-registered, [I]; App. A.1 step 15). On a half whose order is corrected (options (c1)–(d)): fixed-order is not issued; the verdict is not protected; the half is open, chained to the App. A.2 root, with class `unknown` until S5/S7 and schedule-dependent(σ_U) with shape ABSENT expected after them; natural runs give "no σ_E bad witness in n of N runs; σ_E class unknown". On a half whose order is unchanged, fixed-order is expected again; on the no-change control (b), it is kept. Line numbers are v7.0.1's and are rebound per basis by the anchor audit.
- **Label.** Every V2 result is configuration or revision sensitivity, never schedule evidence and never evidence that the default configuration races; never protected (SPEC §9.3 V2 row).
- **References.** SPEC §4.13 (fixed-order row), §7.2 (R2's control items are non-blind), §7.4, §8.2, §9.3 (V2 row; R2 row of the per-case table), §10.2 (risk "No referenced corrected-order basis for R2"), App. A.1 step 15, ADR-044.
- **Recommendation.** None; the reference status of each option is given above.
- **Blocked until decided.** V2 (EXT-20261012 §7 step 3). Options (c3) and (d) also need D18; runs need D1, D17 and D14.
- **Default if undecided.** No referenced control (ADR-044).

## P-C1HOLD (new) — C1's holds (ladder V3, S6)

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §3.3).** **The provider is held immediately before its creation and released on an event;** the RESERVED-window experiment is separated as a follow-up. The normal path records creation and lookup events directly. For the failure path, the provider is held just before creation, and the release is the event of the end of CF's retries or the final initialization result. The retry count and the delay are read from the chosen CF version; 25 or 100 ms are not fixed for all versions. The product of retry count and delay is not taken as a safe wall-clock waiting limit: the observation effect and the real end time are checked in a pilot, and the stop conditions and the watchdog limit are recorded before the main experiment. A failure made by the hold is told apart from a natural run, and a timeout observation alone never decides a permanent deadlock. The lookup/lock-retention experiment on a RESERVED record is treated as an additional check of an OSAL implementation-defect candidate: it is not a start condition of C1's validation and is not added to the number of the five cases' results. Effect: SPEC §4.13, §9.3 V3, App. A.4 steps 5 and 6, §7.3, §6.4 item 52, ADR-050.

- **Question.** C1's holds (SPEC App. A.4 step 5): (i) the provider hold before its create, until CF's loop exit is observed (arm A3, labelled `held-F0`, `timeout-exercised`, `σ_E(held)`); (ii) the declared RESERVED-window experiment, which holds the provider at `OS_CountSemCreate_Impl` entry until CF's lookup returns, with a wait-for oracle (arm A5), and its hang bound, which is `unspecified_by_reference`; (iii) the watchdog of App. A.4 step 5 ("effective retry bound × effective delay") reconciled with SPEC §4.13.
- **Facts.** The provider is created after observation starts, so the holder auto-attaches at clone on the startup thread, and that stop is recorded. The declared experiment confirms CF's VIOLATING occurrence and the leaked lock (safety results); it is a realized progress `stall-witness` only where it also shows permanence. The harness kill of the deadlocked run is followed by a normalization boot. Both are labelled derived. D16 covers the App-record probes only.
- **References.** SPEC App. A.4 steps 4–6, §4.13, §9.3 (V3 row).
- **Recommendation.** Decide before V3's S6 part.
- **Blocked until decided.** V3 (its held arms); S6 for C1.

## P-C2 (new) — C2's role-derived pair (case track VP-D4)

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §3.5).** **Role reconstruction:** start from code reading and static analysis and go on to the role-reconstruction experiment. The provider's late initialization and the consumer's dependent use are told apart; the 2 × 2 of single-stage/two-stage start synchronization and old/new provider roles is prepared. The request mode, the initialization content, the priority and the timeout are choices of the reconstruction, recorded as such and not cited as the reporter's values. **Q22:** the first control uses a provider whose late initialization completes normally; a configuration in which initialization can fail is a separate auxiliary experiment; the assumption that initialization does not fail applies to the reconstruction only and is not generalized to cFS, and a model classification is reported with whether the assumption is included. Every cell is shown as it is; the four cells are not counted as four independent real defects. The result is named **a role-based validation of the C2 mechanism** and is not used as a reproduction of the private original system. Effect: SPEC §8.2, §9.3 VP-D4 and C2 row, §6.4 item 50, ADR-050.

- **Question.** C2's role-derived pair: the apps, the script order, the priorities, the request mechanism (C2-D4), the oracle and the timeouts; Q22 (may the provider's late init fail?); the buggy and fixed revisions; the configure decision for 6.5.0a/6.6.0a; and which contrast of the 2 × 2 counts for RQ9.
- **Facts.** C2's own instance is absent at v7.0.1 (SPEC §9.3). The control is a 2 × 2, not a pair: the single-OPERATIONAL counter barrier (the 6.4.2 tarball, or v6.5.0a) against the two-phase barrier (v6.6.0a), and the old against the new role-derived provider on the ENV cFE (§8.2, §9.3). Whether the buggy and fixed designs can be told apart depends on Q22: C2's fixed design can be `timeout-dependent` only if the provider's late init cannot fail (§7.3; litmus LP-CASE-C2). VP-D4 records template-only `unknown(unmodelled)` first and reports classes with and without Q22; its results are synthetic and role-derived, never a reproduction of #198. A 6.4.2 row needs D13 (§8.2).
- **References.** SPEC §7.3, §8.2 (C2 row), §9.3 (VP-D4 row; C2 row of the per-case table), App. A.8 (LP-CASE-C2); `cases/C2.md` (C2-D4).
- **Recommendation.** None until decided.
- **Blocked until decided.** VP-D4; C2's rows of RQ2 and RQ9.

## P-RGIT (new) — `58abd38d^`/`58abd38d` on the A.3 line (case track VP-D5)

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §3.6).** **Approved as an auxiliary control; it is not used as a substitute validation of the original C5.** The past-version lock-gap validation of the original C5 (#72) goes first; this pair is run after the additional input-correspondence cost is confirmed. It is a source-history pair that checks the generation and withdrawal of the evidence that the present design expects; it is a pair already known in the design documents, so it is not reported as a blind result. Effect: SPEC §8.2, §9.3 VP-D5 and C5 row, ADR-050.

- **Question.** Use cFE `58abd38d^`/`58abd38d` ("Fix #978, store task parameters in task record") as a basis on the A.3 residual line?
- **Facts.** Expected [I]: a PROTECTED certificate that appears on `58abd38d^` and is withheld on `58abd38d`, by S3a on both (§9.3 VP-D5 row). The pair lies on the A.3 line, not on #72 (C5). It is non-blind: SPEC §8.5 item 2 names it, and it is in the C5 case record (§7.2). Every condition of the basis is `unspecified_by_reference`.
- **References.** SPEC §7.2, §7.3, §8.2 (C5 row), §8.5 item 2, §9.3 (VP-D5 row; C5 row of the per-case table).
- **Recommendation.** None until decided.
- **Blocked until decided.** VP-D5.

## P-F1GATE (new) — Case decisions before F1

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §3.7).** **Approved, and the substantive readiness of the five cases is recorded together.** The choices of D13, D22 and P-C2, and the details they need, are recorded before F1. Each case has its own reference configuration, an expectation table, the implemented functions it needs and its validation stages. A case that could not be done is stated with its reason and the limit on the research scope; it is not completed by the result of other code, and a record that a case will not be done does not complete the five-case validation. The expectation table is frozen before the first output on that case; the evaluation engine and rules are frozen before the main evaluation; the records due before the first output are not lost by waiting for F1. Effect: SPEC §7.2, §9.3, §6.4 item 57, ADR-050.

- **Question.** Should F1 wait until D13, D22 and P-C2 are each decided or explicitly declined with the consequence recorded, and each design case has template-only output on its own basis or a recorded reason why it has none?
- **Facts.** F1 freezes the pre-registered expectation sheet "before the first evaluation run on the development cases and, if the PI adopts P-F1GATE, not before D13, D22 and P-C2 are each decided or declined with the consequence recorded" (SPEC §7.2). A case whose basis is not decided at F1 is reported as not evaluated at tool level, with the recorded reason, and never through its v7.0.1 counterpart (§8.1 RQ2, §8.2). The S9 gate reports each design case on its own phenomenon, or with the recorded reason why it has no tool-level output (§9.3).
- **References.** SPEC §0.7 item 7, §7.2, §8.1, §8.2, §9.3 (S9 row, V4a row), §10.2 (risk "Validation concentrated on one case"), ADR-044.
- **Recommendation.** Proposal; no date or threshold.
- **Blocked until decided.** F1.

## P-D18GATES (new) — Gates that depend on C1's B′ and on the case-track cones

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.2).** **(a)** The small C1 example is allowed as a development test and does not replace the validation of the real C1. **(b)** The preliminary performance judgement and the final judgement are separated: the explorer's cost may be measured when only some cases are ready; the final evaluation and adoption judgement includes the relevant scope of the five cases that are prepared, and a missing case is recorded as a limit. Effect: SPEC §4.10 item 9, §9.3 S5 and S7, ADR-050.

- **Question.** (a) Does litmus LP-CASE-C1 satisfy the S5 gate's "C1-derived trace" until D22 lands? (b) Does the S7 review wait for D22, or is the kill criterion reported on the reduced cone set (both App-record windows A.2 and A.3, and `SBNSubPipe`, none of them a design case's instance), with that set named? And does the review include, or wait for, every case-track cone that exists at review time (C2's role-derived pair under P-C2; C3's and C5's instances on the 6.4.x pair under D13)? A keep-or-freeze decision taken without the cones of C2, C3 and C5 is reported with that fact (SPEC §4.10 item 9, §10.1).
- **Facts.** Two of the five development cones of the S7 kill criterion, the C1-derived lookup and the K-PHASE CF instance, exist only in C1's derived basis (SPEC §4.10 item 9). The S5 gate's C1 item needs D22, or P-D18GATES (a) (§9.3 S5 row). The litmus programs of App. A.8 are evidence about the definitions, not about the cases. The case-track cones of C2 (VP-D4, under P-C2) and of C3 and C5 (VP-D3, under D13) need those decisions (§9.3 VP-D3 and VP-D4 rows); without them, and without D22, the S7 review rests only on the reduced set, whose cones contain no design case's instance. A keep-or-freeze decision taken without the cones of C2, C3 and C5 is reported with that fact (§4.10 item 9; §10.2 risk "Validation concentrated on one case").
- **References.** SPEC §4.10 item 9, §7.3, §9.3 (S5, S7, V3, VP-D3 and VP-D4 rows), §10.1 (P-D18GATES, P-C2 and D13 rows), §10.2 (risks "The S5 gate and the S7 kill criterion depend on C1's B′" and "Validation concentrated on one case"), App. A.8.
- **Recommendation.** None until decided.
- **Blocked until decided.** The S5 gate; the S7 kill criterion, including whether its keep-or-freeze decision covers the case-track cones of C2, C3 and C5 or is reported without them (SPEC §4.10 item 9).

## P-CLASSGT (new) — Class statements of references

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.3).** **The source's word "race" is not converted into a tool class automatically.** `fixed-order` and `schedule-dependent` are judged from the stated model and configuration and from real evidence. Agreement with the source is reported separately, and no accuracy denominator is made without a basis. Effect: SPEC §7.2 (F1), §8.5, §6.4 item 41, ADR-051.

- **Question.** How does a reference's class statement ("race (condition)" for C1, C2, C3, C5 and some fix texts; SPEC §8.2, §8.5 item 3) map to the per-(σ, M) classes? Whether a progress label needs a stated stall kind; how items without a label are scored for class.
- **Options** (SPEC §10.1). For example: to `schedule-dependent` under σ_U only; under σ_R1 with the reported platform's cited `P_sched`; or under σ_E on the reported platform; to "not `fixed-order` under σ_U"; or to no label.
- **Facts.** The reference statements: CF #184's title and body, CF's v7.0.1 comment at `cf_cfdp.c:1273` and the fix commit `833fdbb` (C1); cFE #198's body (C2); cFE #73's and #72's titles and the 6.4.2 VDD (C3, C5); some recall fix texts (`6932f1fe`, `58abd38d`). R2 has no reference class statement (§8.2). C3 is a case where a reference statement and a Kairos expectation can disagree: its references say "race", while H-V expects `fixed-order` under σ_R1 (§7.6). The class-label rule is frozen at F1, as `unspecified_by_reference` while undecided (§7.2).
- **References.** SPEC §6.4 item 41, §7.2, §7.6, §8.2, §8.3, §8.5 items 3–4, §9.3 (S9, V4a and V4b rows), ADR-042.
- **Recommendation.** None until decided: the mapping is `unspecified_by_reference`; classification is reported against the pre-registered expectation sheet, and agreement or disagreement with each reference statement is reported per option as an outcome, never as accuracy.
- **Blocked until decided.** Class-correct counts (§8.5); RQ2, RQ8, RQ9; the C3 contrast (reference "race" vs H-V's σ_R1 fixed-order).

## P-ABL-ALLOWED (new) — Single-element ablations of Allowed and outcome semantics

- **Resolution (2026-10-15; source: `PI_DECISIONS_RESOLVED_20261015.md` §4.3).** **Two single-element ablations are adopted:** removing the information about allowed handling, and removing the meaning of returned values and results, each tested separately; the judgement, the undecided share and the cost change are reported. The bundle named after GPredict that removed several elements together is not kept as a comparison. Effect: SPEC §8.4 (ablations 23 and 24), ADR-051.

- **Question.** Add the single-element ablations "Allowed off" and "outcome semantics off", and keep the former stripped GPredict-style row as a labelled bundle?
- **Facts.** The former variant without Φ outcome semantics, Allowed, capture points and certificates is not a baseline; whether it is kept as a labelled bundle ablation is this decision (SPEC §8.3, GPredict-style row). No single-element ablation of Allowed exists unless this decision adds one (§8.3, DCatch-style row). An ablation measures what an element adds to Kairos and says nothing about what a prior technique can express (§8.4, ADR-042).
- **References.** SPEC §0.3 (X1 row), §8.3, §8.4, ADR-025, ADR-042; EXT-20261012 §5.1.
- **Recommendation.** None until the PI decides.
- **Blocked until decided.** The candidate ablations "Allowed off" and "outcome semantics off" and the bundle (§8.3); none of them is in §8.4 until decided.

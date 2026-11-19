# closure-review-bootfix-20261119 — the separate review of the fixes of results 26.5 and the fixes before the first boot (no boot)

Lead ruling 26.7 (라) asked for the fixes of results 26.5 (judgement part, evaluation script revision 2, the run script,
boot declaration 3, predictions 3, protocol 14.13, spec 11.5) to be reviewed and verified separately before the boots.
The review gave 18 findings; the verification judged every one real and 10 of them to be fixed before the first boot.
What was done with each is results 26.9 (`plan/full_stage_results_20261116.md`), with the finding id.  No cFS boot was
run in this step.

- `findings.tsv`: one line per finding: id, severity (the review's), `real` and `before_boot` (the verification's), the
  claim and the verification's reading in this record's words, and what 26.9 does with it.  The full texts of the review
  and of its verification are not copied.  Measurements that the reviewer made in a scratch directory and removed are not
  cited anywhere as evidence; the verification re-derived each claim from committed files and kept outputs.
- The ids `fix-judge-NN` and `fix-decl-NN` are this review's.  They are not the ids of `../closure-review-20261119/`
  (`rules-NN`, ...) or of `../closure-review-preboot-20261119/` (`pre-...`).
- Counts (from `findings.tsv`, its `lines` row): 18 findings, blocking 1, major 5, minor 12; every one real; 10 before the
  first boot.  The judgement part, the evaluation script, the expectation files and the freeze are not changed by this
  step (no finding before the boot touches them; fix-judge-03 and fix-judge-04 are dated notes for section 27).  The run
  script, its tests and the commands change, so a new boot declaration (`boot/declaration4.tsv`) supersedes declaration 3.
- `out/`: the start and end measurements of this step (free space, ledger and registry checks) and the outputs of the
  runs of the revised run script's tests and of the verify step; `guard/`: the space guard logs of every run that writes
  outputs.  The space step of fix-judge-02 and fix-decl-03 has its own record directory, `../closure-space3-20261119/`.

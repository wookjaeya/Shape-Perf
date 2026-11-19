# closure-preboot-20261119 — the re-freeze, the expectations from the rules and the new boot declaration (no boot)

Results: `plan/full_stage_results_20261116.md` 26.4. Rules: results 26.0 (a)-(i), (k), (l); judgement rules 8.13-8.14;
evaluation protocol 5-7 and 14; explanation spec revision 4 section 11. No cFS boot was run; nothing was pushed. Every
run that writes outputs ran under `env/tools/space_guard.sh`; its log is in `guard/`. Paths outside the repository that
are scratch of this step are written `<scratch>`.

| path | what |
|---|---|
| `freeze.tsv` | the new freeze of the explanation script (reader, judge, driver, branch generator, summary, expectation comparison, falsifier and evaluation script, the two fixture files, revision 3); replaces `../closure-explain-rev4-20261119/freeze.tsv` (kept) for the boots of this cycle |
| `out/freeze_sha_before.txt`, `out/freeze_sha_after.txt` | sha256 of the frozen files before and after the fixture runs (the same) |
| `out/fixtures.txt`, `out/eval_fixtures.txt`, `guard/fixtures.log`, `guard/evalfx.log` | `test_explain4.py` 193/193 and `test_explain_eval.py` 92/92 on the frozen bytes |
| `out/free_*.txt` | free space at the start and the end of the step |
| `scripts/runs.sh` | the runs of this step (fixtures, evalfx, runcycle, expect, compare) |
| `out/expect_write2.txt`, `out/expect_form.txt`, `guard/expect*.log`, `scripts/expect_changes.py` | the expectations from the rules (`../closure-c1prep-20261119/scripts/expect_write2.py` -> `expect/expect_c1_2.tsv`, `elements_by_rule_2.tsv`), their form check and the change list `expect/changes_2.tsv` against `expect_c1.tsv` |
| `compare/synth_<build>_<order>.txt`, `compare/static_<build>.txt`, `guard/compare-*.log`, `out/compare_*.log` | the frozen `explain_expect.py` with `expect_c1_2.tsv` on the synthetic explanation outputs and the static dry runs of results 26.3 (each output holds one build, so the other build's lines are `no_record`) |
| `scripts/classify_compare.py`, `compare/classify_synth_<build>_<order>.tsv`, `guard/classify-*.log` | every disagreeing line of the four synthetic comparisons with its kind and its evidence from the record (protocol 14.12 item 3); first trials kept as `*.trial1_*` (a sort of None stopped two of them; the other two were redone with the fixed script) and `*.trial2_t6_basis5.*` (the result test read key texts from basis 5 grounds, which a held item does not carry) |
| `scripts/elements_cross.py`, `compare/elements_vs_fixed_static.txt` | after the commit of the expectations: the element values by rule against the fixed judge's static table of 26.3 (960 cells, 0 differing) |
| `out/eval_synth_<build>_script.log`, `out/eval_synth_<build>_script_tail.txt`, `guard/eval-synth-*.log` | the frozen `explain_eval.py` with `expect_c1_2.tsv` on two synthetic outputs (a form check of the class column in F1); the outputs (about 1 MB each) were in `<scratch>` and removed after their heads, counts and sha256 were copied |

Later commits of the same step add the re-measurement of the old declaration, the new declaration and the verify run;
this table is extended with them.

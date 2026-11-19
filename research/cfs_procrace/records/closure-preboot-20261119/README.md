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

Later commits of the same step add the expectation comparison (`compare/`), the re-measurement of the old declaration
and the verify run; this table is extended with them.

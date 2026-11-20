# closure-explain-fix-20261119 — the code of explanation spec 11 before the first boot (no boot)

Results: `plan/full_stage_results_20261116.md` 26.3. Rules: results 26.0 (b)-(j); judgement rules 8.14; evaluation
protocol 14; explanation spec revision 4 sections 11 and 11.4 (11.4 committed before the code, `cee2ae9`; the code
`dc879c6`).  Nothing here is frozen; no cFS boot; nothing pushed.

| path | what |
|---|---|
| `out/free_start.txt`, `out/free_end.txt` | free space at the start and at the end of the step |
| `guard/g01_named_calls.log`, `out/named_calls_run.txt` | `../closure-c1prep-20261119/scripts/named_calls.py` writing `../closure-c1prep-20261119/candidates/named_calls.tsv` |
| `guard/g02_candidates_regen.log`, `out/candidates_regen_run.txt`, `out/candidates_regen_compare.txt` | the unchanged `candidates.py` re-run into `obs1/xr5_candidates_regen`; 64 of 64 files byte-identical to the committed candidate list and admission files |
| `guard/dev*.log`, `out/dev*.txt` | development trials of the fixtures, the evaluation fixtures and the run-script tests before the code commit (scratch outputs removed; paths written `<scratch>`) |
| `scripts/runs.sh` | the runs after the code commit, each under the space guard (fixtures, evalfx, runcycle, unit, regress, static, rtread, synth, eval) |
| `out/fixtures.txt`, `out/eval_fixtures.txt`, `out/run_cycle_tests.txt`, `out/unit_tools.txt`, `out/unit_step2a.txt` | test_explain4.py 193/193, test_explain_eval.py 92/92, test_run_cycle.sh 7/7, run_unit.sh 20/20, step2a_tests_unit.sh 7/7 |
| `out/regress.log`, `scripts/compare_records.py`, `out/compare_regress.txt` | the 2026-11-16 inputs (`obs1/xr5_regress_rev4fix`) against the frozen revision 4 output `obs1/xr4_regress_rev4_2`: axes and verdicts 45/45 in the eight D1 branches; 13 differing path classes |
| `out/dry_static_<build>.log`, `scripts/elements.py`, `out/static_elements_16_fixed.tsv` (and its first trial `…trial1_root_none.tsv`) | the static dry runs with the 16 branches (`obs1/xr5_dry_static_<build>`) and their element values against `../closure-c1prep-20261119/scratch/branches16/static_elements_16.tsv` (26 differing cells per build) |
| `out/dry_rt_<name>.log` | the runtime step's test dumps (`obs1/xr5_dry_rt_<name>`) |
| `scripts/synth_record.py`, `out/synth_<build>_<order>.log`, `scripts/path_a_uses.py`, `out/synth_path_a_rows.txt` | SYNTHETIC use events on the real plans (test inputs, never evidence; `obs1/xr5_synth_<build>_<order>`, first trial `…_trial1`) and their path A rows |
| `out/test_expect_c1_with_class.tsv` | a TEST INPUT: the committed expect_c1.tsv with the class column by the rule of spec 11 item 12 (not an expectation file of the cycle) |
| `out/eval_*.log`, `out/eval_dry_tails.txt` | explain_eval.py on the six dry-run outputs; the outputs themselves are in `obs1/xr5_eval_dry` (about 1 MB each), their heads and tails here |
| `<output>.sha256` | the file lists of the twelve kept outputs `obs1/xr5_*` (ledger rows `OBS_xr5_*`) |
| `out/ledger_check.txt`, `out/tools_check.txt` | the ledger (205 rows) and the registry (59 rows) at the end, 0 failing |

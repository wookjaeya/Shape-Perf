# review-fixes-2-20261118

Records of the second group of review fixes (results document `plan/full_stage_results_20261116.md` section 12).
No build of the MC tree was made: the kz-static of the MC tree stays `1a47abd0`.

| file | what |
|---|---|
| `commands.txt` | what was run, in order, with the mistakes |
| `prebuild/prebuild_regress.sh`, `prebuild/prebuild_regress.txt`, `df_prebuild_regress.log` | the default-path relations and order regression of the present tool (71 OK) |
| `rf2_pre_relcheck.sha256`, `rf2_pre_order.sha256`, `rf2_pre_final_order.sha256` | hash lists of its three output directories under `/home/user/work/redesign/obs1/` (ledger rows) |
| `recheck/` | corrected, new-named copies of three earlier comparison scripts and their outputs (`recheck_rounds.txt`, `recheck_plans.txt`, `recheck_measure.txt`, `measure/`); `df_recheck_*.log` |
| `scratch_guard/` | the space-guard logs of every scratch cmake, make, ctest and run (24) |
| `scratch_ctest_static.log`, `scratch_ctest_static_2.log` | ctest -V of the static and lint tests in the scratch check builds (no evidence) |
| `scratch_measure.txt` | the scratch tool on the kept facts of the five builds (no evidence) |
| `golden_scratch/`, `golden_scratch.sha256` | the 17 goldens the scratch tool writes differently from HEAD (no evidence; for the cmp after the declared build) |
| `ledger_check_end.txt`, `tools_check_end.txt`, `df_tools_check_end.log` | the cumulative byte-identity list and the tool registry at the end |
| `unit_tools.txt`, `unit_step2a.txt`, `mc_lint_present_tool.log`, their `df_*.log` | unit checks and the lint of the changed tree with the present MC lint binary |

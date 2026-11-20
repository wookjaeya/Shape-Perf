# Records of the result flow step (RESULT_FLOW and RESULT_EFFECT, relations rules revision 10, order rules revision 3)

Results: `../../plan/full_stage_results_20261116.md` section 9.  Order of the work: the code, the fixture, the tests and
the documents (`kairos/docs/static_facts.md` 5u); a scratch check build in the session's scratchpad (no evidence, removed);
the declaration of the build (section 9.0) with the scripts and lists here, committed before the build; then the tool
copy, the build of the build tree in place, the checks and the runs.  Text records only.  No cFS boot.  No push.

| file | what |
|---|---|
| `declared_before_build.txt` | the declaration before the build (the text of results section 9.0); its sha256 is the first line of `commands.txt` |
| `commands.txt` | commands, rc, free space |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh` | the five builds: relations, order (and the keys on re-extracted facts), re-extraction (option off, on), plans |
| `compare.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py` | the default path against the semantics step's outputs |
| `check_head_rf.py`, `section_text_cmp.py`, `section_text_sha_2.py`, `compare_rf.py`, `compare_rel_rf.py`, `compare_ord_rf.py`, `measure_rf.py` | the option-on path: facts header, sections by record text, facts views, relations text, order text and edge listing, the measurements |
| `relink_expected.sha256`, `not_relink.sha256` | sha256 before the build: the 14 binaries that relink, the 110 that must not |
| `golden_scratch.sha256` | the sha256 of the two goldens made in advance by the scratch check build |
| `tool_copy/` | the copy's round trip before the tool is overwritten |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | the registry and the cumulative list before the build |
| `pre_mc.txt`, `post_mc.txt`, `build_start.txt`, `identity_kz-static.txt`, `relink_result.txt`, `make_*.log`, `df_build_*.log` | the build: the MC before and after, the new identity, which of the 14 relinked binaries changed, the space guard of each make |
| `ctest_first.log`, `golden_update.log`, `ctest_full.log`, `ctest_counts.log`, `unit_tools.txt`, `unit_step2a.txt` | the tests: the first ctest (two declared failures), the goldens written, the full ctest, the check counts, the unit checks |
| `tools_check_after_copy.txt`, `tools_check_after_build.txt`, `ledger_check_after_build.txt` | the registry and the cumulative list after the copy and after the build |
| `*_run.txt`, `df_rerun.log`, `df_order*.log`, `df_extract_*.log`, `df_plans*.log` | the five builds' runs and their space guard |
| `compare_*.txt`, `compare/`, `df_compare_*.log` | the comparisons (`compare.sh` steps); `compare_extract_on.txt` holds the three differences outside the declaration (results 9.0a), `compare_extract_on_rel.txt` the re-comparison with `compare_rel_rf_2.py`; `compare_measure.txt`, `compare/measure_<b>.txt` and the probe outputs `compare/s1_*` the measurements |
| `compare_rel_rf_2.py` | the script of addendum 9.0a (a copy; `compare_rel_rf.py` is unchanged) |
| `section_text_on.txt` | the per-section record-text sha256 of this intermediate option-on round |
| `rf_*.sha256` | the file lists of the nine output directories (ledger rows `OBS_rf_*`) |
| `ledger_check_end.txt`, `tools_check_end.txt`, `name_check_tree.txt` | the end checks and the name check of the outputs, the copy and this directory |

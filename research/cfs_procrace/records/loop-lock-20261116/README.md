# Records of the loop flag and lock handle step (LOOP_DEFS, LOCK_HANDLES, relations rules revision 11, order rules revision 4, the final option-on round)

Results: `../../plan/full_stage_results_20261116.md` section 10.  Order of the work: the code, the fixture, the tests and the
documents (`kairos/docs/static_facts.md` 5v); a scratch check build in the session's scratchpad (no evidence, removed); the
declaration of the build (section 10.0) with the scripts and lists here, committed before the build; then the tool copy, the
build of the build tree in place, the checks and the runs.  Text records only.  No cFS boot.  No push.

| file | what |
|---|---|
| `declared_before_build.txt` | the declaration before the build (the text of results section 10.0); its sha256 is the first line of `commands.txt` |
| `commands.txt` | commands, rc, free space |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh` | the five builds: relations, order (and the keys and the caller-dependent report on re-extracted facts), re-extraction (option off, the final round), plans |
| `compare.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py` | the default path against the result flow step's outputs |
| `check_head_rf.py`, `section_text_cmp_ll.py`, `section_text_sha_2.py`, `compare_ll.py`, `compare_rel_ll.py`, `compare_ord_ll.py` | the final round: facts header, sections by record text, facts views, relations text, order text and edge listing |
| `rounds_cmp.py` | the final round against the intermediate rounds' kept per-section record-text sha256 (plan 4.3) |
| `measure_ll.py` | the measurements |
| `relink_expected.sha256`, `not_relink.sha256` | sha256 before the build: the 14 binaries that relink, the 110 that must not |
| `golden_scratch.sha256` | the sha256 of the two goldens made in advance by the scratch check build |
| `df_scratch_*.log` | the space guard of the scratch check build |
| `tool_copy/` | the copy's round trip before the tool is overwritten |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | the registry and the cumulative list before the build |
| `pre_mc.txt`, `post_mc.txt`, `build_start.txt`, `identity_kz-static.txt`, `relink_result.txt`, `make_*.log`, `df_build_*.log` | the build: the MC before and after, the new identity, which of the 14 relinked binaries changed, the space guard of each make |
| `ctest_first.log`, `golden_update.log`, `ctest_full.log`, `ctest_counts.log`, `unit_tools.txt`, `unit_step2a.txt` | the tests: the first ctest (two declared failures), the goldens written, the full ctest, the check counts, the unit checks |
| `tools_check_after_copy.txt`, `tools_check_after_build.txt`, `ledger_check_after_build.txt` | the registry and the cumulative list after the copy and after the build |
| `*_run.txt`, `df_rerun.log`, `df_order*.log`, `df_extract_*.log`, `df_plans*.log` | the five builds' runs and their space guard (`rerun_run.txt` is empty: the relations runs write only into their output directory) |
| `compare_*.txt`, `compare/`, `df_compare_*.log` | the comparisons (`compare.sh` steps): the default path (`compare_relations.txt`, `compare_order.txt`, `compare_extract.txt`, `compare_onextract.txt`, `compare_plans.txt`), the final option-on round (`compare_final.txt`, `compare_final_order.txt`, `compare_plans_final.txt`), the intermediate rounds (`compare_rounds.txt`, `compare/rounds_*`), the measurements (`compare_measure.txt`, `compare/measure_<b>.txt`, `compare/loopdefs_<b>.txt`) |
| `section_text_final.txt` | the per-section record-text sha256 of the final option-on round |
| `ll_*.sha256` | the file lists of the nine output directories (ledger rows `OBS_ll_*`) |
| `ledger_check_end.txt`, `tools_check_end.txt`, `name_check_tree.txt` | the end checks and the name check of the outputs, the copy and this directory |
| `verify_after_restart.txt`, `ledger_check_verify.txt`, `tools_check_verify.txt` | the check after the session restart: the MC kz-static against `post_mc.txt` and `identity_kz-static.txt`, the nine output directories against `ll_*.sha256`, the ledger and the registry re-run (new file names; the earlier outputs are kept) |

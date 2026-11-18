# Records of the manifest revision 4 step (kzs-sites 4, plan rules revision 3: the path A control selection, value requests, the wait and object rules)

Results: `../../plan/full_stage_results_20261116.md` section 11.  Order of the work: the control selection rule text
(`kairos/docs/plan_manifest.md` section 7, committed alone before any plan used it); the code, the fixture, the tests and
the documents; a scratch check build in the session's scratchpad (no evidence, removed before the build; its space guard
logs in `scratch_guard/`); the declaration of the build (section 11.0) with the scripts and lists here, committed before
the build; then the tool copies, the old tools' regression run, the build of the build tree in place, the checks and the
runs.  Text records only.  No cFS boot, no derived build, no plugin build.  No push.

| file | what |
|---|---|
| `declared_before_build.txt` | the declaration before the build (the text of results section 11.0); its sha256 is the first line of `commands.txt` |
| `commands.txt` | commands, rc, free space |
| `plan_one.sh`, `plans.sh` | the plans of the five builds: the default path (revision 2) on the tests_8c_fix facts and on the final option-on round, revision 3 (`v4`) and its measurement with the tier budget all (`v4all`) |
| `compare_plans.py` | the default path's plans against the loop flag and lock handle step's |
| `regress/` | versions 1-3 parse as before: the printer program, its two builds (the parser of d56ca0d and the new one), the manifest groups, the comparison |
| `regress_tools.sh`, `compare_tools.py` | the derived bindings of the kept plans (kz-rt-dump, kz convert, kz-dump) with the tools before and after the build |
| `measure_v4.py` | the measurements of the revision-3 plans |
| `compare.sh` | the comparison steps (`compare_<step>.txt`, `compare/`) |
| `relink_expected.sha256`, `not_relink.sha256` | sha256 before the build: the 50 binaries that relink, the 74 that must not |
| `golden_scratch.sha256` | the sha256 of the golden made in advance by the scratch check build |
| `tool_copy/` | the copies' round trip before the tools are overwritten |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | the registry and the cumulative list before the build |
| `scratch_guard/` | the space guard logs of the scratch check build and its runs |
| `declared_addendum_1.txt` | addendum 1 of the declaration (results 11.0.1): the session stop on 2026-11-17, the order of the runs, the added per-path measurement, the ledger rows; committed before any plan of this step was generated |
| `measure_paths.py`, `measure_paths.sh` | the added measurement: per path (A, B, C) the relation rows, the rows planned by the tier order, by the control selection, by neither (grouped by reason, tier, disposition and label), and the labels and tiers of every row |
| `pre_mc.txt`, `post_mc.txt`, `build_start.txt`, `identity_kz_pre.txt`, `identity_kz-static.txt`, `identity_kz_post.txt`, `relink_result.txt`, `make_*.log`, `df_build_*.log` | the build: the build tree before and after, the new identities of kz-static and kz, which of the 50 relinked binaries changed, the space guard of each make |
| `make_dryrun_after_build.log`, `df_make_dryrun.log`, `df_make_question.log` | the check after the session stop that the build was complete (a dry run names nothing to compile or link) |
| `ctest_first.log`, `golden_update.log`, `ctest_full.log`, `ctest_counts.log`, `unit_tools.txt`, `unit_step2a.txt`, `df_ctest_*.log`, `df_golden_update.log`, `df_unit_*.log` | the tests: the first ctest (the one declared failure), the golden written, the full ctest, the check counts, the unit checks |
| `tools_check_after_copy.txt`, `tools_check_after_build.txt`, `ledger_check_after_build.txt`, `df_tools_check_after_build.log`, `df_copy.log` | the registry and the cumulative list after the copy and after the build |
| `regress_tools_old_run.txt`, `df_regress_tools_old.log` | the old tools' regression run before the build |
| `declared_addendum_2.txt`, `compare_tools_2.py` | addendum 2 of the declaration (results 11.0.2): the first comparison of the derived bindings stopped on four `artifact_sha256` lines outside the declaration (`compare_tools.txt`, rc 1); the cause, the declared class and the comparison script that allows it and compares the two containers as bytes |

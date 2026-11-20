# obs1-20261116-plangen: the plan generator (`kz-static plan`), the framework-layer data and its lint, the plans

Results: `plan/observation_path_results_20261116.md` section 4.  Rules and formats: `kairos/docs/plan_manifest.md`.

| File | What |
|---|---|
| `declared_before_build.txt`, `commands.txt` | the declaration as committed before the build (its sha256 is the first line of commands.txt); every command with rc and free space |
| `framework_trees.tsv`, `framework_scan.txt`, `framework_check.txt` | the version-pinned public headers scanned, the scan, the provenance check of `kairos/semantics/framework_api_roles.txt` |
| `tool_copy/` | the copies of kz-static 5d4a1d7f (before the build) and de1315b3 (the generator, after): lists and round trips |
| `pre_mc.txt`, `post_mc_build1.txt`, `pre_mc_build2.txt`, `post_mc.txt`, `make_build{1,2}.log`, `df_build{1,2}.log`, `build{1,2}_start.txt` | the two builds of the merge-check tree |
| `ctest_first_build1.log`, `ctest_first.log`, `golden_update.log`, `ctest_full.log`, `skip_reason.log`, `unit_tools.txt`, `unit_step2a.txt` | tests |
| `rerun.sh`, `df_rerun.log`, `byte_identity.txt`, `relcheck.sha256` | relations of the five builds and of the sensitivity facts with the new tool |
| `bref_allow.tsv`, `bref_run.txt`, `bref_compare.tsv`, `bref_facts_view.txt`, `bref.sha256` | c1_pre_tx re-extracted with the new tool, against tests_8c_fix |
| `plans.sh`, `df_plans.log`, `plans.sha256`, `plans/` | the plans of the five builds (copies of c1_pre_tx and c5_641) |
| `parse_check.txt`, `priority_agreement.py`, `priority_agreement.txt`, `measure.py`, `measure.txt`, `hand_compare.py`, `hand_compare_c1_pre.txt`, `hand_compare_c5_641.txt` | measurements of the plans |
| `regen_c1_pre_tx.txt`, `regen_c5_641.txt`, `df_regen_*.log` | the regeneration check with the generator copy |
| `tools_check.txt`, `ledger_check.txt` | registry and cumulative byte-identity list |

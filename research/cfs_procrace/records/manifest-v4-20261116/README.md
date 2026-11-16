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

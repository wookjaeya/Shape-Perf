# Records of the declared kz-static build of the second review-fix round (2026-11-19)

Results: `plan/full_stage_results_20261116.md` section 12.0 (나-1) (the declaration, committed before the build) and
section 12.11 (what the run did). No cFS boot. Outputs outside the repository under `/home/user/work/redesign/obs1/rf2_*`
(new directories). Scratch paths in the kept logs are written `<scratch>`.

## Committed with the declaration (before the tool copy and the build)

| file | what |
|---|---|
| `run.sh` | the runs of the five builds, one step at a time, each into a new directory (relcheck, order, extract_off, extract_final, off_order, final_order, plans_t8cf, plans_final, plans_v4, plans_v4_all, plans_v4_kept) |
| `plan_one.sh` | one plan with the arguments of the earlier plan steps; the control-budget reason in the wording of results 12.10 item 3 |
| `compare.sh` | the comparisons against the kept outputs of kz-static 1a47abd0, the twin check against the scratch check build, and the `declared` check of every report against `declared/` |
| `cdiff.py` | two text outputs compared as multisets of lines; every differing line in a class of a rules file or `other` |
| `rules/*.rules` | the declared classes per output kind |
| `sections_cmp.py` | two containers compared section by section by kz-dump record text |
| `twin.py`, `selftest_twin.txt` | the twin check and its self-test on a modified scratch copy |
| `declared/` | 275 reports: the scratch check build's outputs against the kept outputs ([I], not evidence) |
| `declared_summary.txt` | one line per report of `declared/` |
| `relink_expected.sha256`, `not_relink.sha256` | the 14 binaries that relink (dry run and the link files naming libkz-llvm.a) and the 110 that must not, with their sha256 before the build |
| `make_dryrun_before_build.log`, `df_make_dryrun.log` | the dry run under the guard |
| `ctest_first_expected_fail.txt` | the 16 tests expected to fail on the first ctest |
| `ctest_nonstatic_prebuild.log`, `df_ctest_nonstatic_prebuild.log` | the 190 tests outside the static and lint set, run before the build on the present MC tree |
| `pre_mc.txt` | sha256 and `--identity` of the MC tools and of the scratch kz-static, CMakeCache.txt sha256 |
| `ledger_check_pre.txt`, `tools_check_pre.txt` | the checks before the build |
| `data_inputs.sha256` | the data files the runs name, checked before every run step |
| `tool_copy/copy.sh` | the xz copy of kz-static 1a47abd0 with its round trip |
| `scratch/` | the scratch check build's first ctest, make and cmake logs, run lines and the scratch comparison step outputs (`decl_*.txt`; [I]) |
| `scratch_guard/` | the guard logs of the scratch work: 001-016 by the agent stopped on 2026-11-18 (build, runs, a first comparison), 017-029 by this run (two views, the declared comparisons) |

## Taken from the stopped agent's uncommitted draft

`run.sh`, `compare.sh`, `cdiff.py`, `sections_cmp.py`, `plan_one.sh`, `data_inputs.sha256` and the relations rules were
left uncommitted on 2026-11-18 in `records/review-fixes-2-20261118/build/`, a directory the repository's `.gitignore`
(`build/`) leaves out. They were copied here and changed:
- `compare.sh`: the step `twin`; the step `declared` skips `twin.txt`; the status check reads `result PASS` (the
  draft looked for `^OK`, which `one_build.sh` never writes; the ten scratch status checks are re-run in
  `scratch/decl_status_recheck.txt`); header lines.
- `run.sh`: header line only.
- `rules/`: all rules files except the relations rules were empty drafts and are written here; the view rules start
  from the draft's four masks.

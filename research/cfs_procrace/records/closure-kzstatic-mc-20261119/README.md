# closure-kzstatic-mc-20261119: records of the declared kz-static build of the C1 closure (results section 21)

Results: `plan/full_stage_results_20261116.md` section 21.0 (the declaration, committed before the tool copy and the
build) and the sections after it (what the run did).  Outputs outside the repository under
`/home/user/work/redesign/obs1/cz_*` (new directories).  No cFS boot, no push.  Scratch paths in kept logs are written
`<scratch>`.

## Committed with the declaration

| Path | Content |
|---|---|
| `free_start.txt` | free space at the start of the run |
| `run.sh` | the runs of the five builds by the MC kz-static, one step at a time, each into a new `cz_<step>` directory: the arguments of `records/closure-kzstatic-20261119/scripts/scratch6_run.sh` (the scratch check build's runner) with the output root and the tool directory changed |
| `compare.sh` | the comparisons against the kept outputs of kz-static bf2ec2ee (`rf2_*`), the steps of `scratch6_compare.sh` with `cz_` outputs, and the steps `declared` (every report against the committed declared reports `records/closure-kzstatic-20261119/scratch6/compare_2/`), `twin` and `again_rm` |
| `declared_reports.sha256` | the sha256 of the 260 declared reports (the `.samples` files are not compared) |
| `twin_sha.py`, `twin_map.tsv`, `selftest_twin_sha.txt` | the twin check against the sha256 of the scratch outputs (`scratch6/s6_outputs.sha256`), its map of the three declared strings, and its self-test on the kept plan sidecar copies |
| `data_inputs.sha256` | the data files of `kairos/semantics` the runs read (a copy of the sixth run's list) |
| `ctest_first_expected_fail.txt` | the six tests expected to fail on the first ctest |
| `golden_expected.sha256` | the seven goldens to commit and the sha256 of their scratch copies |
| `tool_copy/copy.sh` | the xz copy of kz-static bf2ec2ee with its round trip |
| `prebuild/` | the dry run (`make_dryrun.log`, the same bytes as the sixth run's), the 14 binaries that relink (`relink_expected.txt`, `.sha256`) and the 110 that must not (`not_relink_paths.txt`, `not_relink.sha256`), `pre_mc.txt` (sha256 and `--identity` of the MC tools), `cmakecache.sha256`, the 187 tests outside the scratch check build's set run on the present MC tree (`ctest_prebuild_187.log`, `mc_tests_not_in_scratch.txt`, `scratch_checkbuild_tests_84.txt`), `ledger_check_pre.txt`, `tools_check_pre.txt`, `unit_tools_pre.txt`, `unit_step2a_pre.txt` |
| `guard/` | the space guard logs (`d01` dry run, `p01`-`p05` the checks before the build) |

## After the declaration (results 21.1-21.8)

| Path | Content |
|---|---|
| `tool_copy/ROUNDTRIP.txt` | the round trip of the xz copy of kz-static bf2ec2ee (`/home/user/work/preserve/tools-20261119-b/`) |
| `make_kz-static.log`, `make_all.log` | the MC build |
| `identity_kz-static.txt`, `relink_result.txt` | the new kz-static's sha256 and `--identity`; the 14 that relinked, the 110 that did not, CMakeCache.txt and the other tools |
| `ctest_first.log`, `ctest_first_failed_names.txt`, `ctest_first_check.txt` | the first ctest with `-V` and its failures against `ctest_first_expected_fail.txt` |
| `golden_update.log`, `golden_cmp.txt`, `golden_git_status.txt` | the golden update of the six tests, the seven goldens against their scratch copies, `git status` of `kairos/` |
| `ctest_full.log`, `ctest_counts.txt` | the first full ctest after the update (it stopped on the `test_static_reasons` count, addendum 21.0 (차)) |
| `ctest_full_2.log`, `ctest_counts_2.txt` | the full ctest after the addendum (the one the results cite) |
| `unit_tools.txt`, `unit_step2a.txt` | the unit checks of `env` |
| `ledger_check_after_build.txt`, `tools_check_after_build.txt`, `ledger_check_end.txt`, `tools_check_end.txt` | the cumulative byte-identity checks after the build and at the end |
| `run_all.txt`, `run_<step>.txt` | the rc of every run step |
| `compare_<step>.txt`, `compare/` | the comparisons and their reports (`compare_declared.txt`: 260 of 260 the same bytes as the declared reports; `compare_twin.txt`: 320 of 320 the same as the scratch twins; `compare_again_rm.txt`) |
| `cz_<step>.sha256` | the sha256 of every kept output file (ledger rows `OBS_cz_*`) |
| `outputs_sizes.txt` | the sizes of the kept outputs |
| `exit_checks.sh`, `exit/` | the exit checks: keys (`keys_compare.txt`), pair files (`pair_check_*.txt`), the new lines per build (`success_path_*`, `absence_origins_*`, `holder_*`, `alternatives_*`, `buffer_*`, `buffer_lines_*`, `heads_*`), lead's ruling (e) (`address_passing_*`, `open_other_*`, `ruling_e_views.sha256`) |
| `probe_vs_tool.py`, `exit/probe_vs_tool.txt` | the kept probe outputs (`records/closure-kzstatic-20261119/probes/out`) beside the tool's lines for the same objects |
| `guard/` | all space guard logs of the run (`b*` build and checks, `r_*` runs, `k_*` comparisons, `e_*` exit checks, `z*` end checks) |
| `free_end.txt` | free space at the end |

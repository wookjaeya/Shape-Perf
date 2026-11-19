# closure-runtime-20261119: records of the runtime, dump, convert and plugin build of the C1 closure (results section 23)

Results: `plan/full_stage_results_20261116.md` section 23 (23.0 the declaration, committed before the tool copies and the
build; the sections after it what the run did).  Format text: `kairos/docs/event_semantics.md` 13.5 and 13.6 (committed
before the code).  Outputs outside the repository under `/home/user/work/redesign/obs1/rtb_*` (new directories).  No cFS
boot, no push.  Scratch paths in kept logs are written `<scratch>`.

## Committed with the declaration

| Path | Content |
|---|---|
| `free_start.txt` | free space at the start of the build part |
| `prebuild/` | the MC dry run (`make_dryrun.log`), the 61 link targets that relink (`relink_expected.txt`) with the two files kz-instr-clang's test command makes (`rebuilt_by_command.txt`) and their sha256 before the build (`relink_expected.sha256`), the 51 link targets and 10 command-built test programs that must not change (`not_relink_paths.txt`, `not_relink_other_paths.txt`, `not_relink.sha256`), `cmakecache.sha256`, `pre_mc.txt` (sha256 and `--identity` of the MC tools), the 164 MC tests outside the scratch check build's passed set run on the present tree (`mc_tests_not_in_scratch_164.txt`, `ctest_prebuild_164.log`), `ledger_check_pre.txt`, `tools_check_pre.txt` |
| `scripts/` | `regress.sh` and `compare_regress.py` (the kept boots' 1.4 dumps re-read by the old and the new tools), `regress_manifest.sh` and `compare_manifest.py` (versions 1-3 manifests bound by the old and the new GCC plugin), `bind_check.py` (the bind check of the two C1 plans' design and control rows) |
| `scratch/` | the scratch check build of the complete source (`checkbuild_targets.txt`, `checkbuild_ctest.log`, `checkbuild_tests_passed_112.txt`, `checkbuild_tests_failed_10.txt`), the plugin and runtime trials (`plugin_and_runtime_trials.txt`), a trial of the bind check on c1_pre_tx and of the versions 1-3 comparison (no evidence) |
| `scratch_guard/` | the space guard logs of the scratch builds and runs (`s*` runtime and plugins, `c*` check build, `x*` trials) |
| `guard/` | `d01` the dry run, `p01`-`p03` the checks before the build |

## After the declaration (results 23.0.1-23.7)

| Path | Content |
|---|---|
| `scripts/copy_tools.sh`, `tool_copy/ROUNDTRIP.txt` | the xz copies of kz, kz-rt-dump and kz-instr-clang (`/home/user/work/preserve/tools-20261119-c/`) and their round trip |
| `tools_check_after_copy.txt` | the registry after the copy rows |
| `build_start.txt`, `make_all.log`, `relink_result.txt`, `post_mc.txt` | the MC build; which binaries changed; the tools' sha256 and `--identity` after it |
| `ctest_first.log` | the first (and full) ctest with `-V`: 276 passed, one skipped as before |
| `tools_check_after_build.txt`, `ledger_check_after_build.txt`, `unit_tools.txt`, `unit_step2a.txt` | the checks after the build |
| `regress/` | `chain_check_old.py` (the chain check before this build, from fa31655), the run logs of `regress.sh` and `regress_manifest.sh` before and after, `compare_regress.txt`, `compare_manifest.txt` |
| `plugins/` | the build logs and ctest logs of kzgcc 0.4 and kzllvm 0.2 in their new directories |
| `bind/` | `report_c1_*.txt` (the first bind check, row= sites only) and `report2_c1_*.txt` (addendum 23.0.1: row= sites and the sites the rows' planned needs name, the rows' gaps) |
| `scripts/bind_check_2.py` | the second bind check (committed with addendum 23.0.1, before it ran) |
| `rtb_*.sha256` | the file lists of the eight output directories (ledger rows `OBS_rtb_*`) |
| `ledger_check_end.txt`, `ledger_check_end_2.txt`, `tools_check_end.txt`, `free_end.txt` | the end checks and the free space at the end |
| `guard/` | all space guard logs of the run (`b*` copy, build, checks, runs; `z*` end checks) |

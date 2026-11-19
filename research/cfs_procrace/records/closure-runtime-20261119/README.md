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

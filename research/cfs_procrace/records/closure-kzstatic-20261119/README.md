# closure-kzstatic-20261119: records of the kz-static step of the C1 closure (results section 15)

No cFS boot, no push.  Scratch paths in kept logs are written `<scratch>`.

| Path | Content |
|---|---|
| `free_start.txt` | free space at the start of the step |
| `probes/scripts/` | copies of the probe scripts of `records/closure-texts-20261118/scripts`; only the input directory names differ (`rf2_final_order`, `rf2_plans_v4`, `rf2_final_extract`: the outputs of the fixed kz-static `bf2ec2ee…`, results 12.11) and, in `run_probes.sh`, the depth of the repository root (one directory deeper) |
| `probes/tool_sha_before_views.txt`, `_after_views.txt` | sha256 of the in-place kz-static before and after the two views (`bf2ec2ee…`) |
| `probes/views.sha256` | sha256 and size of the two `kz-static show` views of the final option-on facts (made under the guard, removed after the run) |
| `probes/out_trial1_wrong_root/` | the first run: stopped at its 15th command (rc 1) because the runner computed the repository root one directory too high; kept, not used |
| `probes/out/` | the run on the fixed tool's outputs: 29 of 29 commands rc 0 (`commands_rc.txt`) |
| `probes/normcmp.py` | the outputs of the same scripts compared after masking what follows from the input change alone (view and relations line numbers, a listed row's leading line number, the directory names) |
| `probes/normcmp.txt`, `normcmp_2.txt`, `normcmp_3.txt` | three runs of it with a growing mask (each kept; `normcmp_3.txt` is the one the results cite) |
| `guard/` | space guard logs |
| `free_after_scratch_removed.txt` | free space after this run's scratch was removed |
| `scripts/pair_check.py` | the checks of a pair file against the order text, edges.tsv and plan sidecars of the same facts (a check, not a tool of the method) |
| `scripts/keys_compare.py` | the use-site key listings of two tools per build, and the key texts of the two C1 builds against each other |
| `scratch/` | outputs of the scratch Release build of the source snapshots ([I], not of a declared tool): `golden_scratch/` the three order goldens with the pair section, `fixture_pairs/` and `pairs_trial1/`, `pairs_trial2/`, `pairs_check3/`, `pairs_check4/` the pair checks in the order they were run (`pairs_check4` is the one the results cite; `pairs_sha256_size.txt` the pair files' sha256 and sizes), `keys/` the source-tree items and keys, `facts_cmp/` the facts compared section by section with the rf2 final round, `plans_v4/` the sidecar differences, `absence/` the absence origins counts, `logs/` cmake, make, ctest and run logs |
| `scratch_guard/` | the guard logs of the scratch build, ctest and runs (`s01`-`s20`) |

Second run (2026-11-19, results section 16; the MC tool is still `bf2ec2ee…`):

| Path | Content |
|---|---|
| `free_start_attempt2.txt`, `free_end_attempt2.txt` | free space at the start and the end of the second run (the end after its scratch was removed) |
| `scratch_guard2/` | the guard logs of the second run's scratch build, view, relations runs and ctest (`t01`-`t20`) |
| `scratch2/success_path/` | the success path lines of the scratch tool over the five builds' kept final facts (`lines_<b>.txt`: the summary line, then the namespace-block line of every creator callee), `summary.txt`, and the fixture's relations text |
| `scratch2/holder/` | the holder lines over the five builds (`lines_<b>.txt`) and their counts (`counts_<b>.txt`, by `scripts/holder_counts.py`) |
| `scratch2/golden_scratch/` | the golden of the new fixture case `closure_np`: `closure_np.golden` (with the success path line only) and `closure_np.golden_2` (with the holder cases too; the one to commit with the declared build) |
| `scratch2/logs/` | ctest logs of the scratch build: the goldens, fixture compiles and lint (`ctest_golden_lint_2.log`: 3 failures, the order goldens), the reasons test |
| `scripts/holder_counts.py` | counts of the holder lines of a relations text (a check, not a tool of the method) |

Third run (2026-11-19, results section 17; the MC tool is still `bf2ec2ee…`):

| Path | Content |
|---|---|
| `free_start_attempt3.txt`, `free_end_attempt3.txt` | free space at the start and the end of the third run (the end after its scratch was removed) |
| `scratch_guard3/` | the guard logs of the third run's scratch build, re-extraction of the five builds, relations runs and ctest (`u01`-`u08`) |
| `scratch3/facts_scratch_sha256.txt` | sha256 and size of the five builds' facts re-extracted by the scratch tool (removed after the run) |
| `scratch3/alternatives/` | the alternatives lines of the scratch tool over the five re-extracted builds (`lines_<b>.txt`: row id, line), the header line of the stored-field rule (`header_<b>.txt`) and the counts (`counts_<b>.txt`, by `scripts/alt_counts.py`) |
| `scratch3/relations_text_diff.txt` | the scratch relations texts against `rf2_final_order`: lines gone (only the facts line) and lines added |
| `scratch3/golden_scratch/` | `closure_np.golden_3` (the new fixture cases) and `resultflow_np.golden` (one chain chunk more): to commit with the declared build |
| `scratch3/logs/ctest_golden_lint_3.log` | goldens, fixture compiles and lint of the scratch build: 5 failures (the three order goldens, resultflow_np, closure_np) |
| `scripts/alt_counts.py` | counts of the alternatives lines of a relations text (a check, not a tool of the method) |

Fourth run (2026-11-19, results section 18; the MC tool is still `bf2ec2ee…`, `scratch4/mc_tool_sha256_start.txt`, `_end.txt`):

| Path | Content |
|---|---|
| `free_start_attempt4.txt`, `free_end_attempt4.txt` | free space at the start and the end of the fourth run (the end after its scratch was removed) |
| `scratch_guard4/` | the guard logs of the fourth run's scratch builds, views, re-extraction, relations runs and ctest (`v01`-`v19`) |
| `scratch4/probe/` | a probe fixture of element stores, decays and writes through a pointer parameter, and its view (cited by the eighth addendum) |
| `scratch4/facts_scratch_sha256.txt`, `facts_same_as_scratch3.txt` | the five builds re-extracted by this run's scratch tool: the same sha256 as the third run's (the new lines are relations only) |
| `scratch4/buffer/` | the new lines of the scratch tool over the five builds and the fixture (`lines_<b>.txt`: namespace, line), their counts (`counts_<b>.txt`, `counts_all.txt`, by `scripts/buffer_counts.py`) |
| `scratch4/buffer_trial_before_pointer_key/` | the counts and the text difference of the trial before the name store was limited to a pointer key (kept, not cited for the result) |
| `scratch4/relations_text_diff.txt` | the scratch relations texts against `rf2_final_order`: lines gone (only the facts line) and lines added by kind |
| `scratch4/array_flag_only_c1_pre_tx.txt` | the FIELD_STATUS rows of c1_pre_tx with the scalar-array flag and how many have no other cause |
| `scratch4/golden_scratch/` | `closure_np.golden_4` (to commit with the declared build), the sha256 of the five scratch goldens and which equal earlier scratch goldens |
| `scratch4/logs/` | cmake, make, extraction and ctest logs; `ctest_golden_lint_4d.log` and `reasons_4c.log` are the ones the results cite (earlier trials kept under their names) |
| `scripts/buffer_counts.py` | counts of the stored-name buffer and lifetime-end lines of a relations text (a check, not a tool of the method) |

Fifth run (2026-11-19, results section 19; the MC tool is still `bf2ec2ee…`, `scratch5/mc_tool_sha256_start.txt`, `_end.txt`):

| Path | Content |
|---|---|
| `free_start_attempt5.txt`, `free_end_attempt5.txt` | free space at the start and the end of the fifth run (the end after its scratch was removed) |
| `scratch_guard5/` | the guard logs of the fifth run's scratch cmake, builds, re-extraction, relations, plan runs and ctest (`w01`-`w25`) |
| `scratch5/scratch_tool_sha256.txt` | the sha256 of the last scratch kz-static of the run (the one that made `plans/` and the scratch goldens) |
| `scratch5/facts_scratch_sha256.txt` | sha256 and size of the five builds' facts re-extracted by this run's scratch tool (`run.sh extract_final`; removed after the run) |
| `scratch5/plans/` | the plans of the last scratch tool compared with the kept revision-4 plans `rf2_plans_v4` (by `scripts/plan_rev5_compare.py`): `rev1_keptfacts_vs_kept_v4.txt` (revision-1 data on the kept facts: byte comparison apart from the generator line), `rev2_keptfacts_vs_kept_v4_<b>.txt`, `rev1_scratchfacts_vs_kept_v4_<b>.txt`, `rev2_scratchfacts_vs_kept_v4_<b>.txt`, `rev2_vs_rev1_scratchfacts_<b>.txt` (what revision 5 adds on the same facts); the counts of 10.7 (`counts_rev2_scratchfacts.txt`, `counts_rev2_keptfacts.txt`, by `scripts/plan_rev5_counts.py`) |
| `scratch5/plans_trial1_before_largest_extent/` | the same outputs of the scratch tool before the extent bound took the largest stored-name extent (kept, not cited for the result) |
| `scratch5/plans_new_lines/` | the new sidecar lines of the revision-5 plans on the re-extracted facts per build, the sha256 of every scratch plan file, one argument file |
| `scratch5/golden_scratch/` | `planv5_np.plan.golden` (first, plan id not masked) and `planv5_np.plan.golden_2` (the one to commit with the declared build), `planv4_np.plan.golden` and `_2` (the plan id masked; `_2` to commit), their differences from the repository's planv4 golden, and the run of test_static_plan against a scratch data directory holding the `_2` goldens (389 checks passed) |
| `scratch5/fixture/` | the relations text and the revision-5 sidecar of planv5.c (`planv5_relations_trial.txt`: an earlier scratch tool, kept, not cited) |
| `scratch5/logs/` | cmake, make, extraction and ctest logs (`ctest5d.log` is the one the results cite; earlier trials kept under their names) |
| `scripts/plan_one_ns.sh` | one plan with `--manifest 4` and the namespace data files of coverage rule revision 1 or 2 (a scratch runner) |
| `scripts/plan_rev5_compare.py`, `scripts/plan_rev5_counts.py` | the comparison of a revision-5 plan with a kept revision-4 plan by site keys, and the counts of 10.7 (checks, not tools of the method) |

Sixth run (2026-11-19, results section 20: the revision names, the remaining fixtures and the scratch check build of the complete source; the MC tool is still `bf2ec2ee…`, `scratch6/mc_tool_sha256_start.txt`, `_end.txt`):

| Path | Content |
|---|---|
| `free_start_attempt6.txt`, `free_end_attempt6.txt` | free space at the start and the end of the sixth run (the end after its scratch was removed) |
| `scratch_guard6/` | the guard logs of the sixth run: trial builds and ctest (`x01`-`x05`), the check build (`c01`-`c18`), the runs of the five builds (`r01`-`r10`), the comparisons (`k21`-`k30` first labels, `k41`-`k50` the reports cited), the dry run (`d01`), one view (`v01`), and the guard logs of `one_build.sh` (`one_build_<final|off>_<b>.log`) |
| `scratch6/trials/` | the trial build of the uncommitted source: its logs, the relations texts of the two fixture tries for the absence origins (`fixture/fx1_peek_in_search`: an early return in the search left the absence value unevaluated; `fx2_wrapper`: the wrapper form that was committed) and its scratch goldens |
| `scratch6/checkbuild/` | the check build of the committed source (commit `aa8968f`, a `git archive` of `kairos/`, `logs/src_files.sha256`): `scratch_tool_identity.txt`, `logs/` (cmake, make, the first ctest with `-V` of the static and lint set `cb_ctest_first_V.log`, the batches of the other static test executables `cb_ctest_g5-7.log`, the identity tests, the golden updates into a scratch data directory and the run of the 36 golden, plan and reasons commands against it `cb_golden_scratchdata.log`), `golden_scratch/` the seven goldens to commit with the declared build and their sha256 |
| `scratch6/data_inputs.sha256` | the data files of `kairos/semantics` the runs read (the kept build's list and the five revision-2 name-space files) |
| `scratch6/s6_outputs.sha256` | the sha256 of every output file of the scratch runs of the five builds (relative paths; the outputs were removed after the run) |
| `scratch6/runs/` | the rc lines of `scripts/scratch6_run.sh` per step and of `scripts/scratch6_compare.sh` per step (`cmp_*`: the first labels, `cmp2_*`: the reports cited) |
| `scratch6/compare/`, `compare_table.txt`, `compare_summary.txt` | the first reports (labels before absolute paths were folded; kept, not cited) |
| `scratch6/compare_2/`, `compare_2_table.txt` | the reports cited: every output file of the five builds by the scratch tool against the kept output of the fixed tool (`diffkinds.py` for text, `sections_cmp.py` of `records/review-fixes-2-mc-20261119` for containers); the `*_pairs_*` reports have no counterpart (the pair file is new) and give the head, the tail and the counts by kind and value |
| `scratch6/keys/keys_compare.txt` | the use-site keys of the final-option facts against the fixed tool's, and the two C1 builds against each other |
| `scratch6/pairs/` | `scripts/pair_check.py` on the pair files of the final-option and default-path facts, with the two revision-4 plans of the final facts |
| `scratch6/heads/` | the header lines of the relations texts and of the pair files of the final facts |
| `scratch6/plans_v4_ns2/`, `plans_v4_ns2_counts.txt`, `plans_v4_ns2_counts_vs_scratch5.txt` | the sidecars and requests of the revision-5 plans on the revision-2 name-space data (scratch paths written `<scratch>`), their counts of plan_manifest 10.7 and the comparison with the fifth run's counts |
| `scratch6/dryrun/` | the dry run of the MC tree (`make -n all`), the 14 binaries that relink (`relink_expected.txt`, `_now.sha256`), the 110 that must not (`not_relink_paths.txt`, `not_relink_now.sha256`: the same sha256 as before the fixed-tool build) |
| `scratch6/address_passing_sites.txt`, `open_other_c1_*.txt` | lead's ruling (e): the address-passing call sites of the two C1 builds and the kinds of the `other` open-reason class (`scripts/open_other.py`; `_2` with the kinds) |
| `scratch6/output_sizes.txt` | the sizes of the scratch outputs per step, with and without the second runs (`again/`), and of the kept outputs of the fixed-tool build |
| `scripts/scratch6_run.sh`, `scripts/scratch6_compare.sh`, `scripts/diffkinds.py`, `scripts/compare_table.py`, `scripts/open_other.py` | the runner, the comparison, the line-kind difference, the table and the `other` class reader of this run (checks, not tools of the method) |

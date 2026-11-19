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

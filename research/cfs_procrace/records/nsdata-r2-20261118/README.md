# Name-space data under coverage rule revision 2 (2026-11-19)

The scan of `kairos/docs/semantics_coverage.md` section 8 (revision 2) and the five data files it gives,
`kairos/semantics/framework_namespaces2_<component>.txt`.  The five revision-1 files `framework_namespaces_<component>.txt`
(and the other eight data files) are untouched.  No tool build, no MC binary run.  Declaration: `declaration.md`
(committed with the scan change in 5d74967, before any revision-2 data file).  Commands and rc: `commands.txt`.

## Files

| file | what it is |
|---|---|
| `declaration.md` | what changes, what does not, the checks, the expectations, the space |
| `semantics_trees_r2.tsv` | the component list of `records/semantics-20261116/semantics_trees.tsv` with the three revision-2 anchors on the two components of the object-id generation |
| `report_ns2.py` | the counts of rule 8.5, from the data files and the scan's own functions (imports `env/tools/semantics_scan.py`) |
| `out/start_state.txt`, `out/data_sha256_before.txt`, `out/scan_check_before.txt` | state before the change; the unchanged scan's check (PASS, 13 files) |
| `out/scan_generate.txt` | the generate run (into scratch; the five revision-2 files copied into `kairos/semantics`, sha256 `out/data2_sha256.txt`) |
| `out/scan_check.txt` | **the scan check, rc 0: 18 files (13 revision 1, 5 revision 2), 0 failures** |
| `out/fixture_check.txt` | the fixture check, rc 0 |
| `out/scan_check_old_list.txt` | the check with the old component list: rc 1 on the two revision-2 files of generation N (that list has no revision-2 anchors); declared (check 6) |
| `out/regenerated_rev1_vs_repo.txt`, `out/data_sha256_after.txt`, `out/data_sha256_compare.txt`, `out/git_diff_semantics.txt` | byte identity of the 13 revision-1 files (and the role file): identical, git diff empty |
| `out/plan_pins_check.txt` | the five revision-1 name-space sha256 against the `input namespace-data` pins of the five kept revision-4 plans: 25/25 equal |
| `out/lint_*` | `case-ids-data` and `semantics-names` of a scratch build of `kz_lint` (HEAD source, `out/lint_source_sha256.txt`) on a scratch copy of the HEAD tree, with and without the new files: all rc 0; selftest 114/0; a planted new name is found (the lint reads the new files) |
| `out/report/` | `layer_functions.txt`, `kinds_per_component.txt`, `ops_per_kind.tsv`, `unresolved.txt`, `writers.txt`, `entry_diff.txt`, `get_by_name.txt` |
| `guard/` | space guard logs (trial runs 1-3 are the scratch trials before the declaration) |

## Reading notes

- **The present plan generator does not read the new files.**  Its reader (`lib/llvm/plan_gen.cpp` `read_namespaces`, plan
  rules revisions 3 and 4) refuses the top-level words `layer-fn`, `layer-op`, `kind-unresolved` and the indented
  `rollback` and `not-covered` lines; every revision-2 file of generation N has them, so the reader refuses the file as a
  whole.  The head of each new file says so.  The reader comes with the kz-static build of plan rules revision 5.
- **Kinds are entries per kind constant**, not per source file: the stream kind of revision 1 had two entries (file and
  socket sources); in revision 2 it is one entry with the operations of both files.  The `kind` line has no `source=`
  field (the kind no longer comes from a per-file macro).  Every constant of the kind block is listed; one without any
  operation, or without a public lookup or create, has a `not-covered` line saying which and naming the non-public function.
- **The delete of a kind** is the delete-finalize call, with the kind of the get call in the same function that shares an
  argument (the token) with it; **the rollback** is the create-finalize call after the create call that shares an argument
  with it.  `status-arg`/`out-arg` are the indices of the create-finalize parameter compared with the success constant and
  the pointer parameter stored through, read from its body.
- **`kind-unresolved`**: 7 lines per component, all in the layer's own file: six functions of the layer's file that pass their own kind
  parameter on and one call whose kind argument is a function call (`out/report/unresolved.txt`).  None is in a function
  of a kind's own file.
- **Name lookups through the layer's get functions are not `lookup` lines.**  The rule's lookup role is the find-by-name
  function.  The file-system kind looks a device name up through the get-by-name function (in the mount function
  and the file-system removal function, `out/report/get_by_name.txt`, 2 calls per component); these stay outside the lookup lines and are reported.
- **Generation T (osal-4.1.1, cfe-6.4.1, cfe-6.4.2) is unchanged by revision 2**: the revision-2 files carry the revision-1
  entries under the revision-2 head (entry lines equal, `out/report/kinds_per_component.txt`).  The writer check of these
  tables (`out/report/writers.txt`) finds lines that write a stored-name path and that no entry line names: in osal-4.1.1
  five clears of the file-descriptor path written as a copy of `"\0"` (the T rule's clear form is `""` or `[0] = 0`); in
  cfe-6.4.1 and 6.4.2 the copy of the start-up object name into the application table at start-up (not a copy of a
  parameter) and two terminator stores.  Revision 2 does not change generation T; these are recorded, not fixed.

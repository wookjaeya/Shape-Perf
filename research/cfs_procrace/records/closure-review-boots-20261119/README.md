# closure-review-boots-20261119 — the review of results 27 (the C1 cycle boots and their report) and its corrections

The review read results 27.1-27.4 (`plan/full_stage_results_20261116.md`) against the committed boot records, the kept
outputs and the texts that govern the report (lead ruling 26.7 (마), evaluation protocol 3, 6, 8, 10, 12, 14, judgement
rules 8.13, 8.14, spec 11).  It gave 22 findings; the verification judged every one real and to be corrected before the
report goes to the PI.  What was done with each is in `findings.tsv` (one line per finding, with its group) and in results
27.4, where every corrected place carries the finding id.  No boot is run in this step; no boot of the cycle is left.

**What changes and what does not.** Only the report text changes: results 27.4 and the heads of the expectation columns
of the 27.1 and 27.2 tables.  The diff of the results document is kept as `out/results27.diff`.  The boot records, the
judgement part (`env/obs1/explain_judge.py`), the evaluation and expectation scripts, the expectation files, boot
declaration 4, the rules and `env/tools/chain_check.py` stay as they are (evaluation protocol 12; the frozen counts are kept
as printed).  Three findings name a defect outside the report text (groups G1, G2, G3 of `findings.tsv`: the object path
match of the chain check and its rule text, the judge's `found` for a path-B use without an out value together with the
eval script's F4 on it, and the union compare of pair lines in the expectation script).  27.4 reports each with its effect
on this cycle's counts and names it as a fix target for the next cycle, which the lead decides; nothing is fixed in place.

**The check script** `scripts/review27.py` (written after this text, run once under the space guard).  It reads only.
Inputs: the committed boot records (`--records`: `records/closure-c1prep-20261119`), the kept outputs under `--obs1`, the
builds and their work-directory infix (`--builds c1_pre_tx=pre,c1_post_tx=post`), the boot count (`--boots 5`) and the
directory of `chain_check.py` (`--tools`), whose own `build_id()` (readelf -n) it imports.  Every kept file it reads is
checked first: the dump text, the explanation records and `eval.txt` against the keep lists of their boot
(`boot/<build>/boot<N>/c1cyc_<s>_{boot,explain,eval}<N>.sha256`), the sidecar against the sha256 the eval head names, and
every executable of the build record against the record's sha256; one difference ends the run with rc 1 and nothing
written.  The paths of the build record, the build tree, the dump text and `--main` come from the head line of each boot's
committed `chain.txt`.  It counts and copies record fields; it does not judge, does not recompute a check or an
expectation, and takes no row name or case value as input.  Output: a new directory `out/review27/` (refused if it exists):

- `r1_buildid.tsv`: per build and boot, every `buildid` line of the dump text (object, path, id, note), the
  `same_build_object` verdict of `chain.txt` for that object, the record path that the matching expression of
  `chain_check.py` 344-346 gives for the dump path (repeated in the script, since it is local to `main()`), the record
  paths whose end matches the dump path with a leading `./` removed, and the readelf build-id of that file in the build
  tree with its equality to the dump id; count lines per boot.
- `r2_found_uses.tsv`: `D1-R0`, every use event with outcome `found`, grouped per build, row and path: whether `out.read`
  is set, whether `out.value` is null, whether `field_value` is set, whether the after-call has an `out0` value, the
  instance-link reason, and (path A) the creates of `publish_events` whose `out.raw` or `out.value` equals the found id
  with their `out.read` and `why`; counts per boot.
- `r3_eval_lines.tsv`: per build and boot, from `eval.txt`: the `D1-R0` F4 units per path of the record (the record's
  `path` field) and result; the `D1-R0` F3 pass notes; the F8a notes; and the number of lines of the whole file that
  contain 'slot clear' or 'weak'.
- `r4_order_ground.tsv`: per build, boot and branch: the records per `order_ground` value (the record's and
  `order.order_ground`), the records whose `order_ground` names `fixed_early` or `hard_before`, and the number of
  `order.pairs` entries.
- `r5_plan_paths.tsv`: per build, from the sidecar the eval head names: per path A/B/C the rows, the rows planned by the
  tier order (`state=planned`), by the control selection (`control-state=planned`), by both, by either; and the path A
  rows planned by either with or without a `namespace-entry` line.
- `r6_expect_branch.tsv`: per build, boot and branch, from the committed `expect_compare.txt`: the lines per build of the
  line (own or other), class and result.

**The development runs of report27.py** (finding boot-claims-11).  The report step ran its counting script twice before
committing it, into a scratch directory of that step outside the repository.  This step copies both outputs and their
guard logs, unchanged, to `/home/user/work/redesign/obs1/c1cyc_report27_dev/` (`dev1/`, `dev2/`, `guard/`), checks each
copy against its source by sha256, writes the list `lists/c1cyc_report27_dev.sha256` and adds the ledger row
`OBS_c1cyc_report27_dev`.  The scratch originals are not this step's and are left alone.  `scripts/compare_dev.sh`
(written after this text, run under the guard) writes `out/dev/compare_dev.txt`: for every file name of `dev1`, `dev2` and
the committed `../closure-report-20261119/out/`, its sha256 and line count in each, and for a file that differs from the
committed one the number of lines of `diff`.  The earlier text of report27.py that made `dev1` was not kept by the report
step and cannot be recovered; this record says so instead of claiming it.

**Other files.** `out/free_*.txt`, `out/ledger_check_*.txt`, `out/tools_check_deep_end.txt`: start and end checks;
`guard/`: the space-guard log of every run of this step that writes outputs; `commands.txt`: what was run, in order, with
each rc.

**Size of the results document.** It was 994,533 B before this step.  27.3 keeps it under 10^6 B; the corrections replace
the wrong sentences in place and cite the files above for line values, so the document stays under that bound (its size
after the step is in `commands.txt`).

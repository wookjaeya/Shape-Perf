# closure-prebootfix-20261119 — the fixes of the review after the re-freeze, the third freeze and boot declaration 3 (no boot)

Results: `plan/full_stage_results_20261116.md` 26.5. Review: `../closure-review-preboot-20261119/` (ids `pre-…`). Rules:
evaluation protocol 14.13, explanation spec 11.5, judgement rules 8.14 (one dated note). No cFS boot was run; nothing was
pushed. Every run that writes outputs ran under `env/tools/space_guard.sh`; its log is in `guard/`. Paths outside the
repository that are scratch of this step are written `<scratch>`. Outputs over about 1 MB (explanation records, evaluation
outputs) were written into `<scratch>`, compared with the kept outputs of results 26.3 and removed; their comparisons,
tails and sha256 lists are here. English records cite results 26.0 by its own labels (가)–(더); the earlier English
records cite it by position: (a)=(가), (b)=(나), (c)=(다), (d)=(라), (e)=(마), (f)=(바), (g)=(사), (h)=(아), (i)=(자),
(j)=(차), (k)=(카), (l)=(타), (m)=(파), (n)=(하), (o)=(거), (p)=(너), (q)=(더).

| path | what |
|---|---|
| `scripts/runs.sh` | the runs of this step (fixtures, evalfx, runcycle, unit, regress, static, rtread, synth, loss, eval, summary, namedcalls, expectregen, classify2, sizes) |
| `out/dev_*.txt`, `guard/dev_*.log` | development runs before the code commit: the new judge fixtures on the new judge (206/206) and on the frozen judge 1d0b4ae0 (5 checks fail: the loss cases), the new eval fixtures (a first run 115/116 with a wrong tail check, then 116/116) and on the frozen eval f6b72fc4 (the new checks fail), the run-script tests (a first run 10/11 with a wrong expected row count, then 11/11) |
| `out/fixtures.txt`, `out/eval_fixtures.txt`, `out/run_cycle_tests.txt`, `out/unit_tools.txt`, `out/unit_step2a.txt` | the fixture and unit sets on the committed code ef6d38f: 206/206, 116/116, 11/11, 20/20, 7/7 |
| `compare/regress.txt`, `out/regress.log`, `out/regress_sha256.txt` | the 2026-11-16 inputs, eight D1 branches, against `obs1/xr5_regress_rev4fix`: only the script identity differs |
| `compare/static_<build>.txt`, `out/dry_static_<build>*` | the static dry runs with the 16 branches against `obs1/xr5_dry_static_<build>`: only the script identity differs |
| `compare/rt_<dump>.txt`, `out/dry_rt_<dump>*` | the two runtime test dumps (their kept dump texts and stand-ins) against `obs1/xr5_dry_rt_<dump>/judge`: only the script identity differs |
| `compare/synth_<build>_<order>_records.txt`, `..._expect.txt`, `out/synth_*` | the four kept SYNTHETIC records explained again (test inputs, never evidence): only the record label and the script identity differ; the expectation comparison with `expect_c1_2.tsv` gives the counts of results 26.4 |
| `compare/loss_<tag>_*`, `out/loss_<tag>*` | the synthetic c1_pre_tx script record with one loss line added (spec 11.5 item 1): `no-build-id` gives the same records; `after-stop` gives the loss rule (name states, link reasons, (ma)) |
| `out/eval_<name>_tail.txt`, `out/eval_<name>.log` | the falsifier and evaluation script revision 2 on six kept outputs with `expect_c1_2.tsv`: head, tail per branch, every `expect_discriminate` line, the main branch's F3/F4/F9 lines, the full output's sha256 |
| `out/summary_synth_<build>*` | the frozen summary (5e19b419, the same before and after) over the two kept synthetic outputs of each build as stand-in boots, and its `--check-summary` |
| `out/named_calls_rerun_after_commit.tsv`, `out/named_calls_rerun_cmp.txt` | the committed `named_calls.py` (dc879c6) re-run with the arguments of results 26.3's g01 run: byte-identical to `candidates/named_calls.tsv` |
| `out/expect_regen*.txt` | `expect_write2.py` with `predictions3.tsv`: `expect_c1_2.tsv` and `elements_by_rule_2.tsv` byte-identical |
| `scripts/classify_compare2.py`, `compare/classify2_*.tsv` | the classifier of results 26.4 with its T4 test checking the record; the counts unchanged |
| `out/xr5_sizes.txt` (`out/xr5_sizes.trial1_du_option.txt`: a first run with a wrong `du` option, its last line missing) | du -sb and du -sB1 of `obs1/xr5_*` |
| `freeze.tsv`, `out/freeze_*` | the third freeze and its exit checks on the frozen bytes |
| `scripts/declaration3.py`, `declaration/*`, `out/declaration3_installed_crosscheck.txt`, `scripts/no_expect_check3.sh` | boot declaration 3 (`../closure-c1prep-20261119/boot/declaration3.tsv`), the re-measure of declaration 2, the changes, the cross-check of the 20 installed rows, the spec 11 item 15 check |
| `guard/verify-<build>.log` | the verify step once per build with N = 0 (`../closure-c1prep-20261119/boot/<build>/boot0/verify.txt`: rows 159, ok 159) |
| `out/ledger_check_end.txt`, `out/tools_check_end.txt` (`*.trial1_path.*`: a first call with a wrong path, rc 127) | the ledger (205 rows) and the registry (59 rows) at the end, 0 failing |
| `out/free_*.txt` | the free space at the start of the runs, for the declaration and at the end |

# closure-c1prep-20261119

Preparation of one automatic C1 cycle (results `plan/full_stage_results_20261116.md` section 25; lead's decisions
section 12.10 item 4 (7)): the plans of c1_pre_tx and c1_post_tx from the kz-static step's extraction, the
regeneration check, the runtime trees, the two derived builds with kzgcc 0.4, the binding reports, the derived facts
and the base-to-derived comparison, then before the boot declaration the list of pre-defined uses, the static part of
the adjudication sheets, the candidate list and the evaluation-branch file, the expectations, and the boot
declaration. No cFS boot. Nothing is pushed.

| path | what |
|---|---|
| `out/free_start.txt`, `out/space_need.txt` | free space at the start; the step's space need, written before any action |
| `out/ledger_check_start.txt`, `out/tools_check_deep_start.txt` | the cumulative byte-identity list and the tool registry at the start |
| `reclaim/archive.sh`, `reclaim/roundtrip_<name>.txt` | the replacement of the four option-off extract rounds by xz tar archives (round trip file by file) |
| `reclaim/ledger_rewrite.txt`, `reclaim/ledger_check_after_archive.txt`, `reclaim/free_after.txt`, `guard/a_*.log` | the four rows rewritten (method parts on the archive directory); the ledger after the replacement; free space after |
| `scripts/run.sh` | the steps of the plans and the derived builds (budgets, regen, runtime, prepare, ref, instr, check, rx, buildids, bprime, bindings, compare), each under the space guard |
| `out/plan_budgets.txt` | the budget lines of the two plans (value, reason, truncation, unresolved, sensitivity; the control selection) |
| `out/pre_mc.txt` | the MC tree before the derived builds (build_record.sh) |
| `derived/declared_diff.tsv` | the declared base-to-derived differences, committed before the derived builds |
| `derived/<pre|post>/` | per build: regen.txt, runtime.txt, prepare.txt, build_ref.txt, build_instr.txt, check.txt, rx_*.txt, bincheck_rx.txt, exe_buildids.tsv, bprime_run.txt, module_flags.txt, bindings_report.tsv, compare.txt |

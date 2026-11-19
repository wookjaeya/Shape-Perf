# closure-kzstatic-exit-20261119: the remaining exit checks of the declared kz-static closure build (results section 22)

Results: `plan/full_stage_results_20261116.md` section 22.  This step builds nothing and re-extracts nothing: the five
builds were re-extracted once and run by the declared tool (kz-static `6f40dd60…`) in section 21
(`records/closure-kzstatic-mc-20261119/`, outputs `/home/user/work/redesign/obs1/cz_*`).  It reads those kept outputs
and writes only into this directory.  No new output directory, no tool change, no cFS boot, no push.  Every command ran
under `env/tools/space_guard.sh` (logs in `guard/`).

| Path | Content |
|---|---|
| `free_start.txt`, `free_end.txt` | free space at the start and at the end |
| `out/ledger_check_start.txt`, `out/tools_check_deep_start.txt` | the ledger and the tool registry (deep) at the start |
| `scripts/section_sha.py` | a copy of `records/semantics-20261116/section_sha.py` (per section of a container: count, crc32 and the sha256 of its kz-dump text) |
| `out/kz-dump.sha256`, `out/section_sha_final.txt`, `out/section_sha_off.txt` | the kz-dump used and the per-section sha256 of the ten facts and basis containers of `cz_final_extract` and of `cz_off_extract` |
| `scripts/pair_measure.py` | the kept pair measure (`cdr.txt`), the sum of the `pairs:` lines (`order.txt`) and the complete pair file (`pairs.tsv`) of one build, with three identities and the per-planned-row accounting |
| `out/pair_measure_<build>.txt` | its first run; the selection column wrote `control` for a row that is both tier-planned and a control (A21); kept unchanged |
| `out/pair_measure_2_<build>.txt` | the run after the label fix (`planned+control`); every other byte the same as the first run |
| `scripts/planned_same_thread.py`, `out/planned_same_thread_<c1 build>.txt` | the same_thread lines of the planned rows by kind and path, counted or not by the text |
| `scripts/design_rows.py`, `out/design_rows_<c1 build>.txt` | the new lines of every planned row of the two C1 plans (the rows taken from the sidecar) and the stored-name buffer and lifetime-end lines of the row's namespace block |
| `out/plan_rev5_counts_v4_ns2.txt` | `records/closure-kzstatic-20261119/scripts/plan_rev5_counts.py` on the five plans of `cz_plans_v4_ns2` |
| `out/tool_lines.txt` | the `tool kz-static` line of relations, order and pair file of the five builds and the sha256 of the tool in place |
| `out/ledger_check_end.txt`, `out/tools_check_deep_end.txt` | the same checks at the end |
| `guard/` | the space guard logs |

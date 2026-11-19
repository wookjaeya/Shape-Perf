# closure-explain-fix-20261119 — the code of explanation spec 11 before the first boot (no boot)

Results: `plan/full_stage_results_20261116.md` 26.3. Rules: results 26.0 (b)-(j); judgement rules 8.14; evaluation
protocol 14; explanation spec revision 4 sections 11 and 11.4 (11.4 committed before the code).

| path | what |
|---|---|
| `out/free_start.txt` | free space at the start of the step |
| `guard/g01_named_calls.log`, `out/named_calls_run.txt` | `../closure-c1prep-20261119/scripts/named_calls.py` writing `../closure-c1prep-20261119/candidates/named_calls.tsv` |
| `guard/g02_candidates_regen.log`, `out/candidates_regen_run.txt`, `out/candidates_regen_compare.txt` | the unchanged `candidates.py` re-run into `obs1/xr5_candidates_regen`; 64 of 64 files byte-identical to the committed candidate list and admission files |
| `guard/dev*.log`, `out/dev*.txt` | development trials of the fixtures, the evaluation fixtures and the run-script tests before the code commit (scratch outputs removed; paths written `<scratch>`) |

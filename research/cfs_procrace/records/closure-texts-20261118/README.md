# closure-texts-20261118: probe scripts and their re-run outputs (records, no tool)

Made on 2026-11-18 by the step that committed the rule texts and formats of the C1 cycle before the code that reads
them (judgement rules 8.13, semantics coverage rule revision 2, plan rules revision 5 as text, the formats of
`kairos/docs/static_facts.md` 5x and `kairos/docs/event_semantics.md`, the evaluation protocol, the specification of
the explanation script revision 4).  Nothing here is a tool of the method: the scripts are reading aids for a person,
they take a row number only to print that row, and no tool output depends on them.

## What the scripts read

- The final option-on facts of tool kz-static `1a47abd0…` (`/home/user/work/redesign/obs1/ll_final_extract/<build>/<build>/NP/facts.kz`)
  through a text view (`kz-static show`), the relations and order texts of the same facts
  (`/home/user/work/redesign/obs1/ll_final_order/<build>/`), the revision-4 plans (`…/mv_plans_v4/<build>/`), one kept
  boot record (`…/c1pre_boot/tx/tx.dump.txt`) and the name-space data file of the C1 component.
- These are outputs of the tool **before** the review-fix build.  The numbers are preliminary: **the kz-static step
  re-runs the scripts on the facts of the fixed tool** and keeps those outputs in its own records directory.

## How the views were made

The in-place tool was not run (another step was rebuilding it).  A copy of the registered file was taken to scratch
under the space guard (`guard/copy_tool.log`), its sha256 checked against the registry row before and after
(`tool_sha_before_views.txt`, `tool_sha_after_views.txt`: `1a47abd0…`), the two views made with the copy under the guard
(`guard/view_c1_pre_tx.log`, `guard/view_c1_post_tx.log`, `verdict=completed`), their sha256 kept (`views.sha256`,
43,967,951 and 44,062,934 bytes), then the copy and the views were removed.  No copy of that tool was under the
preserve directory at that time, which is why the copy was taken from the in-place file; the sha256 ties it to the
registry row.

## Files

| Path | Content |
|---|---|
| `scripts/run_probes.sh` | every command of the re-run; the rc of each is checked before the next (`out/commands_rc.txt`: 29 of 29 rc=0) |
| `scripts/p1_alternatives.py` | per caller context of a path A row: what the effects of the absence value reach; the stored-field call-result rule (judgement rules 8.13 item 17) as a prototype.  Its item 5 compares source line numbers, not paths, and its item 6 prints the writer premises: both are limits of the prototype |
| `scripts/p2_success_path.py` | per creator callee: the return alternatives that can carry the success value, the name store and the out-id store on those paths (8.13 item 8, parts (a)-(c) only; guards compared as texts; part (d) is not in the prototype) |
| `scripts/p3_buffer_writers.py`, `p3b_pointer_uses.py` | the stored-name buffer field of every `stored-name` line: FIELD_STATUS, writers by class; the uses of the name pointer field's value (8.13 item 11) |
| `scripts/p4_branch_i.py` | the holder of a lookup's out id: writers, FIELD_STATUS, later readers (8.13 item 15) |
| `scripts/p5_pairs.py` | pairs per build in the order text: all against listed; planned rows (8.13 item 19) |
| `scripts/p6_keys.py` | use-site keys whose text holds an absolute position, and the key texts of the two C1 builds under the source-tree token |
| `scripts/p7_items.py` | per control row of a revision-4 plan: which name-binding items have static inputs |
| `scripts/row_extract.py` | the lines of one relations row with line numbers |
| `scripts/open_reason_population.py` | rows and distinct call sites of the open reason the stored-field rule answers, per build |
| `scripts/ns_statement_check.py` | a Python mirror of the three tests of the present name-space data reader (`kairos/lib/llvm/plan_gen.cpp` `read_namespaces`), run on the statement forms of coverage rule revision 2 (`out/ns_statement_check_input.txt` → `out/ns_statement_check.txt`).  A mirror is not the tool |
| `scripts/dup_outputs.py` | the listing of byte-identical kept directories; **not re-run**: the directories it found were removed by the reclaim step (records/reclaim-20261118 holds that step's own listing) |
| `out/` | the outputs of the second run (29 commands) and of the statement check |
| `out_trial1/` | the outputs of the first run (27 commands, two rows fewer for `p4_branch_i.py`); every file of the same name is byte-identical to `out/` |
| `compare_with_first_run.txt` | `out/` against the outputs of the same scripts made before this step (not in the repository): 16 files byte-identical; 6 differ, each explained below |
| `guard/` | space guard logs (scratch paths written `<scratch>`); `probes_refused_at_start.log` is a refused start (free space under floor + need while another step was building; rc 75, nothing ran) |

Differences against the first run of the scripts: `p1_alternatives_*_A21.txt` 2 lines each (the rule's label in the
output line was renamed); `p3_buffer_writers_*.txt` 2 lines each (the header line names the data file by its
repository path); `rel_pre_A26.txt` and `rel_pre_A63.txt` (the earlier extracts ran past the row into the following
block; `row_extract.py` stops at the row's last line).

## Numbers used by the texts (all preliminary, tool `1a47abd0…`)

| Number | Value | File |
|---|---|---|
| call sites the stored-field call-result rule can decide, per build | c1_pre_tx 1 (2 rows), c1_post_tx 1 (1 row), r2_core 0, c5_641 0, c5_642 0 | `out/open_reason_population.txt` |
| creator callees of the C1 path A rows with parts (a)-(c) of the success-path line shown by the prototype | 1 of 5 in each C1 build | `out/p2_success_path_c1_pre_tx.txt`, `_c1_post_tx.txt` |
| stored-name buffer fields with FIELD_STATUS per-field `DISABLED` | 11 of 11 | `out/p3_buffer_writers_c1_pre_tx.txt` |
| pairs per build (sum of the `pairs:` lines) / listed pair lines | 24,796 / 1,988 (both C1 builds) | `out/p5_pairs.txt` |
| pairs of the 13 planned rows / listed | 1,087 / 66 | `out/p5_pairs.txt` |
| keys with an absolute position | 3, 3, 0, 0, 0 | `out/p6_keys.txt` |

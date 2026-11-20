# obs1-20261116-boot: one natural boot of B' (the derived C1 build made from the generated plan of c1_pre_tx), its conversion and its chain

Results: `plan/observation_path_results_20261116.md` section 6.  Text records only (hashes, lists, small excerpts); the boot
directory, the dump and the run container are outside the repository (`/home/user/work/redesign/obs1/c1pre_boot/`,
`/home/user/work/redesign/obs1/c1pre_convert/`).  No Kairos tool was built.

| File | What |
|---|---|
| `declared_before_boot.txt`, `commands.txt` | the declaration as committed before the boot (its sha256 is the first line of commands.txt); every command with rc and free space |
| `run_boot.sh` | the steps (boot, control, rtdump-mc, convert, kzt-text, chain, stop, conditions); boot and convert under the space guard |
| `regen_c1_pre_tx.txt`, `df_regen.log` | the regeneration check of the plan with the copied generator, before the boot |
| `pre_mc.txt`, `post_mc.txt` | the merge-check tree before and after (no tool may change) |
| `unit_tools.txt`, `unit_step2a.txt` | unit checks before the declaration (14 of 14, 7 of 7) |
| `boot.txt`, `df_boot.log`, `run_meta_excerpt.txt`, `conditions.txt`, `cf_console.txt` | the boot: harness summary, guard samples, run.meta without the environment-names line and the host line, the harness's conditions, the CF console lines |
| `control_check.txt`, `dump_nonevent_lines.txt` | observation only (no ctl, plan or point record); every non-event line of the dump text (run, caps, runx, module, actor, check) |
| `rtdump_compare.txt` | the registry's kz-rt-dump (MC 4390d4f1) against the harness's (runtime tree eb12d09e) on the same dump: byte-identical |
| `convert.log`, `convert.sha256`, `df_convert.log` | kz convert (MC kz 931dd39c) into /home/user/work/redesign/obs1/c1pre_convert/ and the hashes of its outputs |
| `chain.txt` | chain_check.py over plan, sidecar, plan inputs, build record, instance list and tree, B' basis and facts, run.meta, dump, bindings, run.kzt |
| `stop_eval.tsv` | the stop event evaluated afterwards from the record (stop_eval.py) |
| `conditions_compare.txt` | conditions.txt against the earlier G-I natural boot of the same configuration; host line fields compared by name |
| `c1pre_boot.sha256`, `c1pre_convert.sha256`, `ledger_check.txt`, `tools_check.txt` | every file of the two output trees (ledger rows OBS_c1pre_boot, OBS_c1pre_convert); cumulative list and registry |

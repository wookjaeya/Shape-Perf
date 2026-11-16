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

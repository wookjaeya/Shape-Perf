# obs1-20261116-derived: the derived C1 build B' made from the generated plan of c1_pre_tx (no Kairos tool built)

Results: `plan/observation_path_results_20261116.md` section 5.  Text records only (hashes, lists, small excerpts); the
build trees are outside the repository (`/home/user/work/redesign/obs1/c1pre_rt/`, `c1pre_derived/`, `c1pre_bprime/`).

| File | What |
|---|---|
| `declared_before_build.txt`, `commands.txt` | the declaration as committed before the derived build (its sha256 is the first line of commands.txt); every command with rc and free space |
| `declared_diff.tsv` | the declared B to B' differences, in the condition form `env/redesign/compare_derived.py` reads (written before the build) |
| `run_derived.sh` | the steps, each under the space guard (runtime, prepare, ref, instr, check, bindings, rx, bprime, compare, buildids) |
| `pre_mc.txt`, `post_mc.txt` | the merge-check tree before and after (no tool may change) |
| `runtime.txt`, `prepare.txt`, `build_ref.txt`, `build_instr.txt`, `check.txt`, `df_*.log` | outputs of the steps and the guard samples |
| `w/` | copies of the small records of the work space: `path_map.tsv`, `prepare.record`, `build_ref.record`, `build_instr.record`, `check.record`, `merge.txt`, `objects_differ.txt`, `hookcheck_basis.txt`, `bincheck_basis.txt`, `install_inventory.tsv`, `runtime.record` |
| `module_flags.txt`, `bindings_report.tsv` | the plugin's module options and the binding report of every planned site |
| `rx_files.txt`, `bincheck_rx.txt`, `rx_check.txt` | the rx instance of the same derived tree against the plan's group |
| `bprime_run.txt`, `bprime.sha256` | B' basis and facts (single-build runner, derived-workspace mode) |
| `compare.tsv` | B to B' record by record after the path map, every finding against `declared_diff.tsv` |
| `exe_buildids.tsv` | sha256 and GNU build-id of every installed ELF file of B' |
| `unit_tools.txt`, `unit_step2a.txt`, `tools_check.txt`, `ledger_check.txt` | unit checks, registry and cumulative byte-identity list |

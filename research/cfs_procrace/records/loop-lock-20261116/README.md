# Records of the loop flag and lock handle step (LOOP_DEFS, LOCK_HANDLES, relations rules revision 11, order rules revision 4, the final option-on round)

Results: `../../plan/full_stage_results_20261116.md` section 10.  Order of the work: the code, the fixture, the tests and the
documents (`kairos/docs/static_facts.md` 5v); a scratch check build in the session's scratchpad (no evidence, removed); the
declaration of the build (section 10.0) with the scripts and lists here, committed before the build; then the tool copy, the
build of the build tree in place, the checks and the runs.  Text records only.  No cFS boot.  No push.

| file | what |
|---|---|
| `declared_before_build.txt` | the declaration before the build (the text of results section 10.0); its sha256 is the first line of `commands.txt` |
| `commands.txt` | commands, rc, free space |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh` | the five builds: relations, order (and the keys and the caller-dependent report on re-extracted facts), re-extraction (option off, the final round), plans |
| `compare.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py` | the default path against the result flow step's outputs |
| `check_head_rf.py`, `section_text_cmp_ll.py`, `section_text_sha_2.py`, `compare_ll.py`, `compare_rel_ll.py`, `compare_ord_ll.py` | the final round: facts header, sections by record text, facts views, relations text, order text and edge listing |
| `rounds_cmp.py` | the final round against the intermediate rounds' kept per-section record-text sha256 (plan 4.3) |
| `measure_ll.py` | the measurements |
| `relink_expected.sha256`, `not_relink.sha256` | sha256 before the build: the 14 binaries that relink, the 110 that must not |
| `golden_scratch.sha256` | the sha256 of the two goldens made in advance by the scratch check build |
| `df_scratch_*.log` | the space guard of the scratch check build |
| `tool_copy/` | the copy's round trip before the tool is overwritten |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | the registry and the cumulative list before the build |

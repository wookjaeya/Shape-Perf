# Records of the result flow step (RESULT_FLOW and RESULT_EFFECT, relations rules revision 10, order rules revision 3)

Results: `../../plan/full_stage_results_20261116.md` section 9.  Order of the work: the code, the fixture, the tests and
the documents (`kairos/docs/static_facts.md` 5u); a scratch check build in the session's scratchpad (no evidence, removed);
the declaration of the build (section 9.0) with the scripts and lists here, committed before the build; then the tool
copy, the build of the build tree in place, the checks and the runs.  Text records only.  No cFS boot.  No push.

| file | what |
|---|---|
| `declared_before_build.txt` | the declaration before the build (the text of results section 9.0); its sha256 is the first line of `commands.txt` |
| `commands.txt` | commands, rc, free space |
| `rerun.sh`, `order.sh`, `extract.sh`, `plans.sh`, `plan_one.sh` | the five builds: relations, order (and the keys on re-extracted facts), re-extraction (option off, on), plans |
| `compare.sh`, `check_head.py`, `check_whole.py`, `compare_plans.py` | the default path against the semantics step's outputs |
| `check_head_rf.py`, `section_text_cmp.py`, `section_text_sha_2.py`, `compare_rf.py`, `compare_rel_rf.py`, `compare_ord_rf.py`, `measure_rf.py` | the option-on path: facts header, sections by record text, facts views, relations text, order text and edge listing, the measurements |
| `relink_expected.sha256`, `not_relink.sha256` | sha256 before the build: the 14 binaries that relink, the 110 that must not |
| `golden_scratch.sha256` | the sha256 of the two goldens made in advance by the scratch check build |
| `tool_copy/` | the copy's round trip before the tool is overwritten |
| `tools_check_pre.txt`, `ledger_check_pre.txt` | the registry and the cumulative list before the build |

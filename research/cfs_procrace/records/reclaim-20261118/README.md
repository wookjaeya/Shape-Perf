# reclaim-20261118

Removal of twelve kept output directories under `/home/user/work/redesign/obs1/` whose every file is byte-identical to
the file of an older kept directory at the same path (results document `plan/full_stage_results_20261116.md`
section 12.10 item 2; note in section 12.6). No build, no extraction, no boot. No extract round is touched.

| file | what |
|---|---|
| `pairs.tsv` | the twelve candidates, the kept directory each is compared with, the two ledger rows |
| `identity_listing.py` | sha256 of every file of a candidate and of its reference; verdict per pair |
| `selftest.sh`, `selftest.txt` | the listing script on a made-up tree (one eligible, five that must not be) |
| `identity_summary_before.txt`, `listing_before_<candidate>.txt`, `identity_listing_before.log` | the listing before the removal: per file the size and sha256 on both sides, and the modification time and mode of the candidate's file (a removal loses those two) |
| `ledger_rewrite.py`, `ledger_rewrite.txt` | the ledger rows of the removed directories: the row and its list stay, the root becomes the kept directory (the convention is stated in the header of `../LEDGER.tsv`) |
| `ledger_check_start.txt` | the cumulative byte-identity list before any change |
| `ledger_check_proposed_before_removal.txt` | the rewritten ledger checked while the candidates were still there |
| `reclaim.sh`, `reclaim.txt` | the removal (each candidate compared with its reference once more by `diff -rq` just before) |
| `ledger_check_after_removal.txt` | the cumulative byte-identity list after the removal |
| `free_start.txt`, `free_end.txt` | `df -B1 /` at the start and at the end |
| `commands.txt` | what was run, in order |
| `name_check_*.txt` | the name checks of this directory and of the commits |

What a removed directory was is still readable: its ledger row names it, its hash list (in the records directory of
the step that made it) names every file with its sha256, and the kept directory holds those bytes at the same
relative paths. The comparison scripts of earlier steps that read a removed directory by path
(`compare.sh`, `rerun.sh`, `order.sh` of the steps of 2026-11-16, `prebuild/prebuild_regress.sh` of
`review-fixes-2-20261118`) do not run again as they are; their kept outputs are unchanged.

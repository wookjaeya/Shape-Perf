#!/usr/bin/env bash
# rerun.sh -- relations of the five builds on the tests_8c_fix NP facts with the new kz-static (relations rules revision 7),
# with the use-site key listing; into a new directory.  The facts are those the plan generator build read (not re-extracted
# here; extract.sh re-extracts them separately).
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
T=/home/user/work/redesign/step2a/tests_8c_fix
O=/home/user/work/redesign/obs1/relrev7_check
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  mkdir -p "$O/$b"
  "$KS" relations "$T/$b/NP/facts.kz" -o "$O/$b/relations.txt" --keys "$O/$b/keys.txt" > "$O/$b/relations.log" 2>&1 || rc=1
done
exit $rc

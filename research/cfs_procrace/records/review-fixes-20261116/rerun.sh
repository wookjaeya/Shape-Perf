#!/usr/bin/env bash
# rerun.sh -- relations of the five builds on the tests_8c_fix NP facts with the kz-static of the review fixes
# (relations rules revision 9), with the use-site key listing, into a new directory; compared with extfix_relcheck by
# compare.sh relations.
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
T=/home/user/work/redesign/step2a/tests_8c_fix
O=${O:-/home/user/work/redesign/obs1/revfix_relcheck}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  mkdir -p "$O/$b"
  "$KS" relations "$T/$b/NP/facts.kz" -o "$O/$b/relations.txt" --keys "$O/$b/keys.txt" > "$O/$b/relations.log" 2>&1 || rc=1
done
exit $rc

#!/usr/bin/env bash
# rerun.sh -- relations of the five builds on the tests_8c_fix NP facts with the new kz-static (facts not re-extracted),
# with the use-site key listing, and of the facts of the image-budget sensitivity run; into a new directory.
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
T=/home/user/work/redesign/step2a/tests_8c_fix
S=/home/user/work/redesign/obs1/relincomplete_sens/c1_pre_tx/NP
O=/home/user/work/redesign/obs1/plangen_relcheck
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  mkdir -p "$O/$b"
  "$KS" relations "$T/$b/NP/facts.kz" -o "$O/$b/relations.txt" --keys "$O/$b/keys.txt" > "$O/$b/relations.log" 2>&1 || rc=1
done
mkdir -p "$O/sens_c1_pre_tx"
"$KS" relations "$S/facts.kz" -o "$O/sens_c1_pre_tx/relations.txt" > "$O/sens_c1_pre_tx/relations.log" 2>&1 || rc=1
exit $rc

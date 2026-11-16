#!/usr/bin/env bash
# relations of the five builds with the new kz-static on the tests_8c_fix facts, plain and with --keys
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
T=/home/user/work/redesign/step2a/tests_8c_fix
O=/home/user/work/redesign/obs1/relrows_check
[ -e "$O" ] && { echo "exists $O"; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  mkdir -p "$O/$b"
  "$KS" relations "$T/$b/NP/facts.kz" -o "$O/$b/relations.txt" > "$O/$b/relations.log" 2>&1; r1=$?
  "$KS" relations "$T/$b/NP/facts.kz" -o "$O/$b/relations_k.txt" --keys "$O/$b/keys.txt" > "$O/$b/relations_k.log" 2>&1; r2=$?
  echo "$b rc $r1 $r2"; [ $r1 -eq 0 ] && [ $r2 -eq 0 ] || rc=1
done
exit $rc

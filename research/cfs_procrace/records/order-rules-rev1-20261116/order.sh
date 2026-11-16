#!/usr/bin/env bash
# order.sh [FACTS_ROOT OUT] -- `kz-static order` of the five builds, with the edge listing, into a new directory; each
# build run twice (the second into OUT/<b>/again/, removed by compare.sh after the byte comparison).  Default facts: the
# tests_8c_fix NP facts; with FACTS_ROOT the facts are FACTS_ROOT/<b>/<b>/NP/facts.kz (the re-extraction of extract.sh).
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
FR=${1:-}
O=${2:-/home/user/work/redesign/obs1/order_rev1}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  if [ -n "$FR" ]; then F=$FR/$b/$b/NP/facts.kz; else F=/home/user/work/redesign/step2a/tests_8c_fix/$b/NP/facts.kz; fi
  mkdir -p "$O/$b/again"
  "$KS" order "$F" -o "$O/$b/order.txt" --edges "$O/$b/edges.tsv" > "$O/$b/order.log" 2>&1 || rc=1
  "$KS" order "$F" -o "$O/$b/again/order.txt" --edges "$O/$b/again/edges.tsv" > /dev/null 2>&1 || rc=1
  echo "$b rc=$rc"
done
exit $rc

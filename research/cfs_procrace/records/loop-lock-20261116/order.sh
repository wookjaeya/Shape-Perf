#!/usr/bin/env bash
# order.sh [FACTS_ROOT OUT [report]] -- `kz-static order` of the five builds, with the edge listing, into a new directory;
# each build run twice (the second into OUT/<b>/again/, removed by compare.sh after the byte comparison).  Default facts:
# the tests_8c_fix NP facts; with FACTS_ROOT the facts are FACTS_ROOT/<b>/<b>/NP/facts.kz (a re-extraction of extract.sh),
# and the relations of those facts with the use-site key listing go beside (relations.txt must equal the re-extraction's).
# With `report` both runs also write the caller-dependent report (--caller-dependent-report OUT/<b>/cdr.txt; the order
# text and the edges are not changed by it).  (A copy of the result flow step's order.sh with the report.)
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
FR=${1:-}
O=${2:-/home/user/work/redesign/obs1/ll_order}
REP=${3:-}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  if [ -n "$FR" ]; then F=$FR/$b/$b/NP/facts.kz; else F=/home/user/work/redesign/step2a/tests_8c_fix/$b/NP/facts.kz; fi
  mkdir -p "$O/$b/again"
  X1=(); X2=()
  if [ "$REP" = report ]; then X1=(--caller-dependent-report "$O/$b/cdr.txt"); X2=(--caller-dependent-report "$O/$b/again/cdr.txt"); fi
  "$KS" order "$F" -o "$O/$b/order.txt" --edges "$O/$b/edges.tsv" "${X1[@]}" > "$O/$b/order.log" 2>&1 || rc=1
  "$KS" order "$F" -o "$O/$b/again/order.txt" --edges "$O/$b/again/edges.tsv" "${X2[@]}" > /dev/null 2>&1 || rc=1
  if [ -n "$FR" ]; then "$KS" relations "$F" -o "$O/$b/relations.txt" --keys "$O/$b/keys.txt" > "$O/$b/relations.log" 2>&1 || rc=1; fi
  echo "$b rc=$rc"
done
exit $rc

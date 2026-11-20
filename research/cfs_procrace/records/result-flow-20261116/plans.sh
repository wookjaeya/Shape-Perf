#!/usr/bin/env bash
# plans.sh [FACTS_ROOT OUT] -- the plans of the five builds with the kz-static of the result flow step
# (plan_one.sh), into a new directory.  Default: on the tests_8c_fix NP facts and basis; with FACTS_ROOT on
# FACTS_ROOT/<b>/<b>/NP/{facts,basis}.kz.
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FR=${1:-}
O=${2:-/home/user/work/redesign/obs1/rf_plans}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  if [ -n "$FR" ]; then D=$FR/$b/$b/NP; else D=/home/user/work/redesign/step2a/tests_8c_fix/$b/NP; fi
  PLANS_FACTS=$D/facts.kz PLANS_BASIS=$D/basis.kz bash "$HERE/plan_one.sh" "$KS" "$O/$b"; r=$?
  echo "$b rc=$r"
  [ $r = 0 ] || rc=1
done
exit $rc

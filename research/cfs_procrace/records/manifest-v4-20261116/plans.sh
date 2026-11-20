#!/usr/bin/env bash
# plans.sh FACTS_ROOT OUT [v4|v4all] -- the plans of the five builds with the MC kz-static of the manifest revision 4
# step (plan_one.sh), into a new directory OUT.  FACTS_ROOT: t8cf (the tests_8c_fix NP facts and basis) or a directory
# with <b>/<b>/NP/{facts,basis}.kz (the final option-on round ll_final_extract).
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FR=${1:?usage: plans.sh FACTS_ROOT OUT [v4|v4all]}; O=${2:?}; MODE=${3:-}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  if [ "$FR" = t8cf ]; then D=/home/user/work/redesign/step2a/tests_8c_fix/$b/NP; else D=$FR/$b/$b/NP; fi
  PLANS_FACTS=$D/facts.kz PLANS_BASIS=$D/basis.kz bash "$HERE/plan_one.sh" "$KS" "$O/$b" $MODE; r=$?
  echo "$b rc=$r"
  [ $r = 0 ] || rc=1
done
exit $rc

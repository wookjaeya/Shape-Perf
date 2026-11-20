#!/usr/bin/env bash
# plans.sh -- the plans of the five builds with the new kz-static, with the arguments of the plan generator build
# (records/obs1-20261116-plangen/plans.sh: the same facts, basis, framework-layer data, selection, order, budget, budget
# reason and disclosure), into a new directory; args.txt per build for env/tools/plan_regen.sh.
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
T=/home/user/work/redesign/step2a/tests_8c_fix
F=/home/user/Shape-Perf/research/cfs_procrace/kairos/semantics/framework_api_roles.txt
O=/home/user/work/redesign/obs1/relrev7_plans
REASON="the record pool is fixed (KZ_RT_POOL_BYTES = KZ_RT_MAX_ACTORS x 65536, kairos/runtime/include/kz_rt.h:61-64) and no event volume of a generated plan is measured yet: the priority tier is planned whole, the next tier is not"
DISCLOSURE="the value was chosen knowing which rows the priority tier holds on these builds (plan/ovm_rev1_20261116/S1_RETALLY.md 3.1 and its probe tables); which rows are in the plan is an output of the rule, checked after the generation"
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  mkdir -p "$O/$b"
  A=(plan "$T/$b/NP/facts.kz" --basis "$T/$b/NP/basis.kz" --framework "$F" --select observation-set --order method-priority --budget tier:D07
     --budget-reason "$REASON" --budget-disclosure "$DISCLOSURE" -o "$O/$b/plan.kzs" --sidecar "$O/$b/plan.side" --modules-out "$O/$b/plan.mod" --exes-out "$O/$b/plan.exe")
  printf '%s\n' "${A[@]}" > "$O/$b/args.txt"
  "$KS" "${A[@]}" > "$O/$b/plan.log" 2>&1; r=$?
  echo "$b rc=$r"
  [ $r = 0 ] || rc=1
done
exit $rc

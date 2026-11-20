#!/usr/bin/env bash
# plan_one.sh KS OUT -- one plan with the arguments of the plan generator build (records/obs1-20261116-plangen/plans.sh
# and records/schema-rev12-20261116/plans.sh: framework-layer data, selection, order, budget, budget reason and
# disclosure) on PLANS_FACTS and PLANS_BASIS, into the new directory OUT; args.txt for env/tools/plan_regen.sh.
set -uo pipefail
KS=$1; O=$2
F=/home/user/Shape-Perf/research/cfs_procrace/kairos/semantics/framework_api_roles.txt
REASON="the record pool is fixed (KZ_RT_POOL_BYTES = KZ_RT_MAX_ACTORS x 65536, kairos/runtime/include/kz_rt.h:61-64) and no event volume of a generated plan is measured yet: the priority tier is planned whole, the next tier is not"
DISCLOSURE="the value was chosen knowing which rows the priority tier holds on these builds (plan/ovm_rev1_20261116/S1_RETALLY.md 3.1 and its probe tables); which rows are in the plan is an output of the rule, checked after the generation"
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
A=(plan "$PLANS_FACTS" --basis "$PLANS_BASIS" --framework "$F" --select observation-set --order method-priority --budget tier:D07
   --budget-reason "$REASON" --budget-disclosure "$DISCLOSURE" -o "$O/plan.kzs" --sidecar "$O/plan.side" --modules-out "$O/plan.mod" --exes-out "$O/plan.exe")
printf '%s\n' "${A[@]}" > "$O/args.txt"
"$KS" "${A[@]}" > "$O/plan.log" 2>&1

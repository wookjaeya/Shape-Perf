#!/usr/bin/env bash
# plan_one.sh KS OUT [v4|v4all] -- one plan on PLANS_FACTS and PLANS_BASIS into the new directory OUT, with the
# arguments of the earlier plan steps (records/manifest-v4-20261116/plan_one.sh: framework-layer data, selection, order,
# budget tier:D07, budget reason and disclosure); args.txt for env/tools/plan_regen.sh.
#   (none)  the plan without --manifest: the regression of the default path
#   v4      --manifest 4: the control budget all with its reason, the five namespace data files named below, the value
#           requests in OUT/plan.req
#   v4all   as v4 with the tier budget all (a measurement of the rules on every row; not a plan for a boot)
# Two differences from the earlier step's script: the namespace data files are named one by one (no file-name pattern),
# and the reason of the control budget says what the boots of these rows are: control runs of design rows, not a test of
# the name-binding rule (lead decision, plan/full_stage_results_20261116.md 12.10 item 3).
set -uo pipefail
KS=$1; O=$2; MODE=${3:-}
K=/home/user/Shape-Perf/research/cfs_procrace/kairos
F=$K/semantics/framework_api_roles.txt
REASON="the record pool is fixed (KZ_RT_POOL_BYTES = KZ_RT_MAX_ACTORS x 65536, kairos/runtime/include/kz_rt.h:61-64) and no event volume of a generated plan is measured yet: the priority tier is planned whole, the next tier is not"
DISCLOSURE="the value was chosen knowing which rows the priority tier holds on these builds (plan/ovm_rev1_20261116/S1_RETALLY.md 3.1 and its probe tables); which rows are in the plan is an output of the rule, checked after the generation"
CREASON="the path A control selection (lead decisions, plan/full_stage_results_20261116.md 6.10 item 2 and 12.10 item 3; kairos/docs/plan_manifest.md section 7) is planned whole: the boots of these rows are control runs of design rows, rows whose text and earlier records were read when the rule was written, not a test of the name-binding rule; no event volume of a generated plan is measured yet, so no row is cut by a volume estimate"
BUDGET=tier:D07
[ "$MODE" = v4all ] && { BUDGET=all; REASON="a measurement of the plan rules on every row of the build (not a plan for a boot)"; DISCLOSURE="none: no row is cut"; }
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
A=(plan "$PLANS_FACTS" --basis "$PLANS_BASIS" --framework "$F" --select observation-set --order method-priority --budget "$BUDGET"
   --budget-reason "$REASON" --budget-disclosure "$DISCLOSURE" -o "$O/plan.kzs" --sidecar "$O/plan.side" --modules-out "$O/plan.mod" --exes-out "$O/plan.exe")
if [ -n "$MODE" ]; then
  A+=(--manifest 4 --control-budget all --control-budget-reason "$CREASON" --requests-out "$O/plan.req")
  for n in framework_namespaces_cfe-6.4.1.txt framework_namespaces_cfe-6.4.2.txt framework_namespaces_osal-4.1.1.txt framework_namespaces_osal-7.0.1.txt framework_namespaces_osal-f34759d.txt; do A+=(--namespaces "$K/semantics/$n"); done
fi
printf '%s\n' "${A[@]}" > "$O/args.txt"
"$KS" "${A[@]}" > "$O/plan.log" 2>&1

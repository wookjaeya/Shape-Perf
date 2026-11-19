#!/usr/bin/env bash
# plan_one_ns.sh KS OUT NSREV -- one plan with --manifest 4 (the arguments of records/review-fixes-2-mc-20261119/plan_one.sh
# v4: budget tier:D07, the control budget all with the reason in the words "control runs of design rows") on PLANS_FACTS
# and PLANS_BASIS into the new directory OUT, with the namespace data files of coverage rule revision NSREV (1: the five
# framework_namespaces_*.txt the kept revision-4 plans name; 2: the five framework_namespaces2_*.txt), named one by one.
# A scratch runner of the plan rules revision 5 step (results section 19), not a tool of the method.
set -uo pipefail
KS=$1; O=$2; NSREV=$3
K=/home/user/Shape-Perf/research/cfs_procrace/kairos
F=$K/semantics/framework_api_roles.txt
REASON="the record pool is fixed (KZ_RT_POOL_BYTES = KZ_RT_MAX_ACTORS x 65536, kairos/runtime/include/kz_rt.h:61-64) and no event volume of a generated plan is measured yet: the priority tier is planned whole, the next tier is not"
DISCLOSURE="the value was chosen knowing which rows the priority tier holds on these builds (plan/ovm_rev1_20261116/S1_RETALLY.md 3.1 and its probe tables); which rows are in the plan is an output of the rule, checked after the generation"
CREASON="the path A control selection (lead decisions, plan/full_stage_results_20261116.md 6.10 item 2 and 12.10 item 3; kairos/docs/plan_manifest.md section 7) is planned whole: the boots of these rows are control runs of design rows, rows whose text and earlier records were read when the rule was written, not a test of the name-binding rule; no event volume of a generated plan is measured yet, so no row is cut by a volume estimate"
case "$NSREV" in 1) P=framework_namespaces_ ;; 2) P=framework_namespaces2_ ;; *) echo "NSREV is 1 or 2" >&2; exit 2 ;; esac
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
A=(plan "$PLANS_FACTS" --basis "$PLANS_BASIS" --framework "$F" --select observation-set --order method-priority --budget tier:D07
   --budget-reason "$REASON" --budget-disclosure "$DISCLOSURE" -o "$O/plan.kzs" --sidecar "$O/plan.side" --modules-out "$O/plan.mod" --exes-out "$O/plan.exe"
   --manifest 4 --control-budget all --control-budget-reason "$CREASON" --requests-out "$O/plan.req")
for c in cfe-6.4.1 cfe-6.4.2 osal-4.1.1 osal-7.0.1 osal-f34759d; do A+=(--namespaces "$K/semantics/$P$c.txt"); done
printf '%s\n' "${A[@]}" > "$O/args.txt"
"$KS" "${A[@]}" > "$O/plan.log" 2>&1

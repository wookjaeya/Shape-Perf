#!/usr/bin/env bash
# compare.sh STEP -- the comparisons and measurements of the manifest revision 4 step (plan/full_stage_results_20261116.md
# section 11), each step's text into compare_<step>.txt beside this script (refused when it exists) and per build into
# compare/<step>_<b>.txt:
#   manifest     versions 1-3 parse as before: regress/regress.sh with the two printers of regress/build_printers.sh
#                (PRINTERS=<dir>) into /home/user/work/redesign/obs1/mv_regress_manifest
#   tools        the derived bindings of the kept plans: compare_tools.py mv_regress_tools_old mv_regress_tools_new
#   plans_v3     the default path: compare_plans.py t8cf ll_plans mv_plans_v3_t8cf and t8cf ll_plans_final mv_plans_v3_final
#                (the same facts on both sides; only the generator identity and the sidecar sha256 lines may differ)
#   regen        env/tools/plan_regen.sh on every plan of mv_plans_v4 (with the value requests) into a scratch dir
#   parse_v4     the new printer on every plan of mv_plans_v4 and mv_plans_v4_all: parses as version 4 (PRINTERS=<dir>)
#   measure      measure_v4.py mv_plans_v4/<b> ll_plans_final/<b> mv_plans_v4_all/<b>
set -uo pipefail
STEP=${1:?usage: compare.sh manifest|tools|plans_v3|regen|parse_v4|measure}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; R="$(cd "$HERE/../.." && pwd)"
O=/home/user/work/redesign/obs1; BS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
OUT=$HERE/compare_$STEP.txt
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
mkdir -p "$HERE/compare"
rc=0
{
case "$STEP" in
  manifest)
    bash "$HERE/regress/regress.sh" "${PRINTERS:?PRINTERS=<dir of build_printers.sh>}" "$O/mv_regress_manifest" || rc=1 ;;
  tools)
    python3 -I "$HERE/compare_tools.py" "$O/mv_regress_tools_old" "$O/mv_regress_tools_new" || rc=1 ;;
  plans_v3)
    for b in $BS; do
      python3 -I "$HERE/compare_plans.py" t8cf "$O/ll_plans/$b" "$O/mv_plans_v3_t8cf/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" t8cf "$O/ll_plans_final/$b" "$O/mv_plans_v3_final/$b" | sed "s/^/$b final /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
    done ;;
  regen)
    SC=${SCRATCH:?SCRATCH=<a new scratch dir>}
    [ -e "$SC" ] && { echo "refusing: $SC exists"; exit 2; }
    for b in $BS; do
      bash "$R/env/tools/plan_regen.sh" /home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static "$O/mv_plans_v4/$b/args.txt" "$SC/$b" | sed "s/^/$b /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
    done ;;
  parse_v4)
    for d in mv_plans_v4 mv_plans_v4_all; do for b in $BS; do
      v=$("${PRINTERS:?}/print_new" "$O/$d/$b/plan.kzs" 2> "$HERE/compare/parse_${d}_$b.v4" | head -1)
      echo "$d/$b: $v; $(cat "$HERE/compare/parse_${d}_$b.v4")"; [ "$v" = "version 4" ] || rc=1
    done; done ;;
  measure)
    for b in $BS; do
      python3 -I "$HERE/measure_v4.py" "$O/mv_plans_v4/$b" "$O/ll_plans_final/$b" "$O/mv_plans_v4_all/$b" > "$HERE/compare/measure_$b.txt" || rc=1
      echo "$b measure rc=$? $(wc -l < "$HERE/compare/measure_$b.txt") lines"
    done ;;
  *) echo "unknown step $STEP"; exit 64 ;;
esac
echo "compare.sh $STEP rc=$rc"
} > "$OUT" 2>&1
cat "$OUT"
exit $rc

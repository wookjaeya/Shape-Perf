#!/usr/bin/env bash
# recheck.sh STEP -- re-runs of earlier comparisons with corrected, new-named scripts (plan/full_stage_results_20261116.md
# 12.7 builds-02, builds-08, builds-09); the earlier scripts and outputs stay as they are.  Each step writes
# recheck_<step>.txt beside this script (refused when it exists).
#   rounds   rounds_cmp_2.py (also the count) on the kept per-section sha256 lists of the three intermediate rounds
#            against the final round's
#   plans    compare_plans_2.py (no allowance on the rules lines) on the kept plan pairs of the semantics, result flow,
#            loop-lock and manifest steps
#   measure  measure_v4.py of the manifest step again, the status of each build captured before it is printed; the new
#            per-build files must be the same bytes as the kept compare/measure_<b>.txt
set -uo pipefail
STEP=${1:?usage: recheck.sh rounds|plans|measure}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; RC="$(cd "$HERE/../.." && pwd)"
O=/home/user/work/redesign/obs1; BS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
OUT=$HERE/recheck_$STEP.txt
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
rc=0
{
case "$STEP" in
  rounds)
    python3 -I "$HERE/rounds_cmp_2.py" "$RC/semantics-20261116/section_text_old_2.txt" "$RC/semantics-20261116/section_text_new_2.txt" \
      "$RC/result-flow-20261116/section_text_on.txt" "$RC/loop-lock-20261116/section_text_final.txt"; r=$?; [ $r -eq 0 ] || rc=1; echo "rounds_cmp_2 rc=$r" ;;
  plans)
    while read -r mode old new; do
      for b in $BS; do
        python3 -I "$HERE/compare_plans_2.py" "$mode" "$O/$old/$b" "$O/$new/$b" | sed "s|^|$old $new $b |"; r=${PIPESTATUS[0]}; [ "$r" -eq 0 ] || rc=1
      done
    done <<'PAIRS'
t8cf objacc_plans sem_plans
extract objacc_plans_off sem_plans_off
extract objacc_plans_on sem_plans_on
t8cf sem_plans rf_plans
extract sem_plans_off rf_plans_off
extract sem_plans_on rf_plans_on
t8cf rf_plans ll_plans
extract rf_plans_off ll_plans_off
extract rf_plans_on ll_plans_final
t8cf ll_plans mv_plans_v3_t8cf
t8cf ll_plans_final mv_plans_v3_final
PAIRS
    ;;
  measure)
    mkdir -p "$HERE/measure"
    for b in $BS; do
      python3 -I "$RC/manifest-v4-20261116/measure_v4.py" "$O/mv_plans_v4/$b" "$O/ll_plans_final/$b" "$O/mv_plans_v4_all/$b" > "$HERE/measure/measure_$b.txt"; r=$?
      [ $r -eq 0 ] || rc=1
      if cmp -s "$HERE/measure/measure_$b.txt" "$RC/manifest-v4-20261116/compare/measure_$b.txt"; then s="same bytes as the kept file"; else s="DIFFERS from the kept file"; rc=1; fi
      echo "$b measure rc=$r $(wc -l < "$HERE/measure/measure_$b.txt") lines; $s"
    done ;;
  *) echo "unknown step $STEP"; exit 64 ;;
esac
echo "recheck.sh $STEP rc=$rc"
} > "$OUT" 2>&1
cat "$OUT" | tail -4
exit $rc

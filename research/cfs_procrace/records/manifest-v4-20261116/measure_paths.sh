#!/usr/bin/env bash
# measure_paths.sh -- the per-path measurement of the manifest revision 4 step (declared in addendum 1 of results
# section 11.0): measure_paths.py on each build's plans, into compare/paths_<b>.txt beside this script and the summary
# compare_paths.txt (refused when it exists).  Per build, four plans of the same facts (the final option-on round):
#   ll_plans_final/<b>     plan rules revision 2, tier budget tier:D07 (the loop flag and lock handle step's kept plan)
#   mv_plans_v3_final/<b>  the same with the tool of this step (the default path)
#   mv_plans_v4/<b>        plan rules revision 3, tier budget tier:D07, control budget all
#   mv_plans_v4_all/<b>    plan rules revision 3, tier budget all (a measurement, not a plan for a boot)
# Reads the sidecars only.  rc 0 when every run is rc 0.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
O=/home/user/work/redesign/obs1; BS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
OUT=$HERE/compare_paths.txt
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
mkdir -p "$HERE/compare"
rc=0
{
for b in $BS; do
  [ -e "$HERE/compare/paths_$b.txt" ] && { echo "refusing: compare/paths_$b.txt exists"; rc=1; continue; }
  python3 -I "$HERE/measure_paths.py" "$O/ll_plans_final/$b" "$O/mv_plans_v3_final/$b" "$O/mv_plans_v4/$b" "$O/mv_plans_v4_all/$b" > "$HERE/compare/paths_$b.txt"; r=$?
  [ $r = 0 ] || rc=1
  echo "$b measure_paths rc=$r $(wc -l < "$HERE/compare/paths_$b.txt") lines"
  awk -v b="$b" '/^plan /{n=split($2,a,"/"); d=a[n-1]} /^[ABC] [0-9]+ [0-9]+ [0-9]+ [0-9]+ [0-9]+$/ || /^all [0-9]/ {print b, d, $0}' "$HERE/compare/paths_$b.txt"
done
echo "columns of the lines above: build plan path rows tier-planned control-planned either neither"
echo "measure_paths.sh rc=$rc"
} > "$OUT" 2>&1
cat "$OUT"
exit $rc

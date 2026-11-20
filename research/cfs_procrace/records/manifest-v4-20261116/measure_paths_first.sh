#!/usr/bin/env bash
# measure_paths_first.sh -- measure_paths.py on the kept plans of the first observation path (plangen/<b>: plan rules
# of that step, tier budget tier:D07, the facts of that step), into compare/paths_first_<b>.txt and the summary
# compare_paths_first.txt (refused when it exists).  Declared in addendum 3 (results 11.0.3).  Reads the sidecars only.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
O=/home/user/work/redesign/obs1; BS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
OUT=$HERE/compare_paths_first.txt
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
mkdir -p "$HERE/compare"
rc=0
{
for b in $BS; do
  [ -e "$HERE/compare/paths_first_$b.txt" ] && { echo "refusing: compare/paths_first_$b.txt exists"; rc=1; continue; }
  python3 -I "$HERE/measure_paths.py" "$O/plangen/$b" > "$HERE/compare/paths_first_$b.txt"; r=$?
  [ $r = 0 ] || rc=1
  echo "$b measure_paths rc=$r $(wc -l < "$HERE/compare/paths_first_$b.txt") lines"
  awk -v b="$b" '/^[ABC] [0-9]+ [0-9]+ [0-9]+ [0-9]+ [0-9]+$/ || /^all [0-9]/ {print b, "plangen", $0}' "$HERE/compare/paths_first_$b.txt"
  grep '^path [ABC]:' "$HERE/compare/paths_first_$b.txt" | sed "s/^/$b plangen /"
done
echo "columns of the count lines: build plan path rows tier-planned control-planned either neither"
echo "measure_paths_first.sh rc=$rc"
} > "$OUT" 2>&1
cat "$OUT"
exit $rc

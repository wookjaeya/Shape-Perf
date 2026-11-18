#!/usr/bin/env bash
# parse_v4_full.sh PRINTERS -- every plan of mv_plans_v4 and mv_plans_v4_all read by the new printer to its end
# (addendum 3 of the declaration, results 11.0.3: compare.sh parse_v4 pipes the printer into `head -1`, which closes the
# pipe before the printer reaches its summary of the version 4 statements, so nine of its ten summary files are empty).
# Per plan: the printer's stdout is counted and not kept (its first line must be "version 4"), its summary line (stderr)
# goes to compare/parsefull_<dir>_<b>.v4, and the summary's numbers are compared with counts taken from the plan text by
# grep: need statements, gap statements, spawn statements, sites with place=load, sites with skey=, and the plan id.
# Output: compare_parse_v4_full.txt beside this script (refused when it exists).  rc 0 when every plan agrees.
set -uo pipefail
P=${1:?usage: parse_v4_full.sh PRINTERS}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
O=/home/user/work/redesign/obs1; BS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
OUT=$HERE/compare_parse_v4_full.txt
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
mkdir -p "$HERE/compare"
rc=0
{
for d in mv_plans_v4 mv_plans_v4_all; do for b in $BS; do
  f=$O/$d/$b/plan.kzs; s=$HERE/compare/parsefull_${d}_$b.v4
  [ -e "$s" ] && { echo "refusing: $s exists"; rc=1; continue; }
  first=$("$P/print_new" "$f" 2> "$s" | { IFS= read -r l; echo "$l"; cat > /dev/null; })
  nl=$("$P/print_new" "$f" 2> /dev/null | wc -l)
  want="v4 plan=$(grep -m1 '^plan ' "$f" | awk '{print $2}') needs=$(grep -c '^need ' "$f") gaps=$(grep -c '^gap ' "$f") spawn=$(grep -c '^spawn ' "$f") load=$(grep -c '^site .* place=load ' "$f") skey=$(grep -c '^site .* skey=' "$f")"
  got=$(cat "$s")
  if [ "$first" = "version 4" ] && [ "$got" = "$want" ]; then v=OK; else v=BAD; rc=1; fi
  echo "$v $d/$b: first line '$first', printed lines $nl, sites in the text $(grep -c '^site ' "$f"); parser: $got; text: $want"
done; done
echo "parse_v4_full.sh rc=$rc"
} > "$OUT" 2>&1
cat "$OUT"
exit $rc

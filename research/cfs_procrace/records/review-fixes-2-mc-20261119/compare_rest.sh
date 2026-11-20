#!/usr/bin/env bash
# compare_rest.sh SCRATCH_VIEWS SCRATCH_OUTPUTS -- after addendum 12.0 (나-2): the steps twin_2 and declared of
# compare.sh, under the space guard, reports into compare/ (the existing reports are not touched); one line per step in
# compare_rest.txt; stops at the first rc that is not 0.
N="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; G=/home/user/Shape-Perf/research/cfs_procrace/env/tools/space_guard.sh
V=${1:?usage}; X=${2:?usage}
[ -e "$N/compare_rest.txt" ] && { echo "refusing: compare_rest.txt exists" >&2; exit 2; }
export CMP=$N/compare SCRATCH=$V TWIN_SCRATCH=$X
export TWIN_SCRATCH_TOOL=$(dirname "$X")/sbuild/tools/kz-static/kz-static
export TWIN_SCRATCH_TOOL_SHA256=$(grep -oE 'Release\): file sha256 [0-9a-f]{64}' "$N/pre_mc.txt" | grep -oE '[0-9a-f]{64}$')
for st in twin_2 declared; do
  [ -e "$N/compare_$st.txt" ] && { echo "refusing: compare_$st.txt exists" >&2; exit 2; }
  bash "$G" --need 150000000 --kind "compare-$st" --log "$N/df_compare_$st.log" -- bash -c "bash '$N/compare.sh' $st > '$N/compare_$st.txt' 2>&1"; r=$?
  echo "$st rc=$r OK=$(grep -c '^OK' "$N/compare_$st.txt") BAD=$(grep -c '^BAD' "$N/compare_$st.txt") $(date -u +%FT%TZ)" >> "$N/compare_rest.txt"
  [ $r = 0 ] || exit 1
done

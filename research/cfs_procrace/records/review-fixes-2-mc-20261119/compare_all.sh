#!/usr/bin/env bash
# compare_all.sh SCRATCH_VIEWS SCRATCH_OUTPUTS -- the steps of compare.sh on the MC build's outputs, each under the space
# guard (need 150,000,000), reports into compare/ (new), stopping at the first step whose rc is not 0; one line per step
# in compare_all.txt.  The twin step reads the scratch check build's outputs (SCRATCH_OUTPUTS) and the path and file
# sha256 of its kz-static as pre_mc.txt records them.
N="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; G=/home/user/Shape-Perf/research/cfs_procrace/env/tools/space_guard.sh
V=${1:?usage}; X=${2:?usage}
[ -e "$N/compare_all.txt" ] && { echo "refusing: compare_all.txt exists" >&2; exit 2; }
[ -e "$N/compare" ] && { echo "refusing: compare/ exists" >&2; exit 2; }
export CMP=$N/compare SCRATCH=$V TWIN_SCRATCH=$X
export TWIN_SCRATCH_TOOL=$(dirname "$X")/sbuild/tools/kz-static/kz-static
export TWIN_SCRATCH_TOOL_SHA256=$(grep -oE 'Release\): file sha256 [0-9a-f]{64}' "$N/pre_mc.txt" | grep -oE '[0-9a-f]{64}$')
[ ${#TWIN_SCRATCH_TOOL_SHA256} = 64 ] || { echo "no scratch tool sha256 in pre_mc.txt" >&2; exit 2; }
for st in relations order extract_off extract_final off_order final_order plans_t8cf plans_final plans_v4 plans_v4_all plans_v4_kept twin declared; do
  bash "$G" --need 150000000 --kind "compare-$st" --log "$N/df_compare_$st.log" -- bash -c "bash '$N/compare.sh' $st > '$N/compare_$st.txt' 2>&1"; r=$?
  echo "$st rc=$r OK=$(grep -c '^OK' "$N/compare_$st.txt") BAD=$(grep -c '^BAD' "$N/compare_$st.txt") $(date -u +%FT%TZ)" >> "$N/compare_all.txt"
  [ $r = 0 ] || exit 1
done

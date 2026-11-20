#!/usr/bin/env bash
# run_all.sh -- the run steps of run.sh one after another, each under the space guard (need 120,000,000), stopping at
# the first step whose rc is not 0; one line per step in run_all.txt
N="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; G=/home/user/Shape-Perf/research/cfs_procrace/env/tools/space_guard.sh
[ -e "$N/run_all.txt" ] && { echo "refusing: run_all.txt exists" >&2; exit 2; }
for st in relcheck order extract_off extract_final off_order final_order plans_t8cf plans_final plans_v4 plans_v4_all plans_v4_kept; do
  bash "$G" --need 120000000 --kind "run-$st" --log "$N/df_run_$st.log" -- bash -c "bash '$N/run.sh' $st > '$N/run_$st.txt' 2>&1"; r=$?
  echo "$st rc=$r $(date -u +%FT%TZ)" >> "$N/run_all.txt"
  [ $r = 0 ] || exit 1
done

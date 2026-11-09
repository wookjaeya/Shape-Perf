#!/usr/bin/env bash
# re-run of step2a_tests.sh all into a new output directory (RV4 analysis 7.2 step 4)
L=${SCRATCH:?scratchpad directory}/rerun_rv4
echo $$ > "$L/pgid"
R=/home/user/Shape-Perf/research/cfs_procrace
O=/home/user/work/redesign/step2a/tests_rv4
KZB=/home/user/work/merge-check/kz-llvm/tools
{ date -u +%FT%TZ; df -B1 --output=avail / | tail -1; git -C "$R" rev-parse HEAD; sha256sum "$R/env/redesign/step2a_tests.sh" "$R/env/redesign/step2a_tests_count.py" "$KZB/kz-static/kz-static" "$KZB/kz-dump/kz-dump"; } > "$L/before.txt"
env -u STEP2A_ALLOW_TABLE -u STEP2A_RUNS KZB="$KZB" bash "$R/env/redesign/step2a_tests.sh" all "$O" > "$L/all.log" 2>&1
rc=$?
{ date -u +%FT%TZ; df -B1 --output=avail / | tail -1; } > "$L/after.txt"
echo $rc > "$L/all.rc"

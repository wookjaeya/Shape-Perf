#!/usr/bin/env bash
# run_r2_gdb.sh -- one R2 (cFE #2663) observation run on the unmodified ENV build, under gdb (the reporter's
# method). Conditions: cases/mapping_v701.md §1.3. Observation points: tool/gdb/r2_trace.py.
#
#   run_r2_gdb.sh <tag>        e.g. r2_g1
#
# Steps: persisted-state snapshot (M1 tool) -> run_cfs.sh v4 in `operational` mode with the CI ES-restart stop
# (E10, E11 (a); stop method still PI decision D3) and RUN_CFS_WRAP=gdb -> snapshot -> copy the trace into logs/
# -> tool/gdb/r2_summarize.py. Must be started as root with CFS_NORMAL_USER set (run_cfs.sh rule, ENV E1).
set -euo pipefail
TAG="${1:?tag}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGS="${HERE}/logs"
EXE_DIR="/home/user/work/procrace/cfs_ref/cFS/build-native_std/exe/cpu1"
RUNDIR="/home/user/work/procrace/r2_runs"            # owned by the run user, so gdb can write the trace
SCRIPT_SRC="${HERE}/../tool/gdb/r2_trace.py"
: "${CFS_NORMAL_USER:?set CFS_NORMAL_USER (ENV E1)}"

install -o "${CFS_NORMAL_USER}" -m 0644 "${SCRIPT_SRC}" "${RUNDIR}/r2_trace.py"
TRACE="${RUNDIR}/${TAG}.jsonl"
rm -f "${TRACE}"                                      # this run's own output file only
{
    echo "run_r2_gdb.sh tag=${TAG} start_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)"
    echo "r2_trace.py sha256=$(sha256sum "${SCRIPT_SRC}" | cut -c1-64)"
    echo "gdb=$(gdb --version | head -1)"
    echo "gdb defaults kept: disable-randomization (on), startup-with-shell (on), all-stop mode"
} > "${LOGS}/${TAG}_driver.txt"

"${HERE}/tools/persist_snapshot.sh" "before ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_before.txt" 2>&1
R2_TRACE_OUT="${TRACE}" \
RUN_CFS_WRAP="gdb -batch -nx -x ${RUNDIR}/r2_trace.py -ex run --args" \
    "${HERE}/run_cfs.sh" operational ci-es-restart-poweron "${LOGS}/${TAG}_gdb_operational_esrestart.log" \
    >> "${LOGS}/${TAG}_driver.txt" 2>&1 || echo "run_cfs.sh exit=$?" >> "${LOGS}/${TAG}_driver.txt"
"${HERE}/tools/persist_snapshot.sh" "after ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_after.txt" 2>&1
cp "${TRACE}" "${LOGS}/${TAG}_trace.jsonl"
python3 "${HERE}/../tool/gdb/r2_summarize.py" "${LOGS}/${TAG}_trace.jsonl" --json "${LOGS}/${TAG}_summary.json" \
    | tee "${LOGS}/${TAG}_summary.txt"

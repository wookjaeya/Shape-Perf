#!/usr/bin/env bash
# run_r2_cord.sh -- one R2 (cFE #2663) observation run on the CORD-instrumented copy, without a debugger.
# Conditions: cases/mapping_v701.md §1.3 and results/R2_cord_v701.md §1. Instrumentation: tool/cord/patches/cfe_v701_r2.patch.
#
#   run_r2_cord.sh <tag>        e.g. r2_c1
#
# Steps (same structure as run_r2_gdb.sh): persisted-state snapshot (M1 tool) -> run_cfs.sh v5 with
# CFS_DIR=<instrumented copy>, `operational` mode and the CI ES-restart stop (E10, E11 (a); stop method still PI
# decision D3) -> snapshot -> copy the CORD trace into logs/ -> cord_analyze.py (--key-prefix sb. with --json, and
# --all) -> r2_cord_summary.py. Must be started as root with CFS_NORMAL_USER set (run_cfs.sh rule, ENV E1).
#
# CORD settings: only CORD_OUT is set (the trace file, in a directory owned by the run user). CORD_DELAY,
# CORD_BUF and CORD_DUMP_AFTER_MS are not set (no witness delay; recorder default buffer of 65536 events per
# thread; trace written at process exit by the recorder's atexit handler). CORD_OUT is part of core-cpu1's
# environment: a deviation from E12 for the instrumented build only (CONDITIONS.md H-9).
set -euo pipefail
TAG="${1:?tag}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGS="${HERE}/logs"
TOOL="${HERE}/../tool/cord"
CORD_CFS_DIR="/home/user/work/procrace/cfs_cord/cFS"
EXE_DIR="${CORD_CFS_DIR}/build-native_std/exe/cpu1"
RUNDIR="/home/user/work/procrace/cord_runs"          # owned by the run user, so core-cpu1 can write the trace
: "${CFS_NORMAL_USER:?set CFS_NORMAL_USER (ENV E1)}"
[ -d "${RUNDIR}" ] && [ "$(stat -c %U "${RUNDIR}")" = "${CFS_NORMAL_USER}" ] || { echo "ERROR: ${RUNDIR} must exist and be owned by ${CFS_NORMAL_USER}" >&2; exit 2; }
for v in $(compgen -e | grep -E '^CORD_' || true); do echo "ERROR: ${v} is already set; only CORD_OUT is set, by this script" >&2; exit 2; done

TRACE="${RUNDIR}/${TAG}.jsonl"
rm -f "${TRACE}"                                      # this run's own output file only
{
    echo "run_r2_cord.sh tag=${TAG} start_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)"
    echo "cfs_dir=${CORD_CFS_DIR}"
    echo "patch sha256=$(sha256sum "${TOOL}/patches/cfe_v701_r2.patch" | cut -c1-64)"
    echo "cord.c in tree sha256=$(sha256sum "${CORD_CFS_DIR}/cfe/cord/cord.c" | cut -c1-64) tool/cord/cord.c sha256=$(sha256sum "${TOOL}/cord.c" | cut -c1-64)"
    echo "core-cpu1 sha256=$(sha256sum "${EXE_DIR}/core-cpu1" | cut -c1-64)"
    echo "cord_analyze.py sha256=$(sha256sum "${TOOL}/cord_analyze.py" | cut -c1-64)"
    echo "CORD environment for core-cpu1: CORD_OUT=${TRACE} (CORD_DELAY, CORD_BUF, CORD_DUMP_AFTER_MS unset)"
} > "${LOGS}/${TAG}_driver.txt"

"${HERE}/tools/persist_snapshot.sh" "before ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_before.txt" 2>&1
CFS_DIR="${CORD_CFS_DIR}" CORD_OUT="${TRACE}" \
    "${HERE}/run_cfs.sh" operational ci-es-restart-poweron "${LOGS}/${TAG}_cord_operational_esrestart.log" \
    >> "${LOGS}/${TAG}_driver.txt" 2>&1 || echo "run_cfs.sh exit=$?" >> "${LOGS}/${TAG}_driver.txt"
"${HERE}/tools/persist_snapshot.sh" "after ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_after.txt" 2>&1
if [ -s "${TRACE}" ]; then
    cp "${TRACE}" "${LOGS}/${TAG}_cord_trace.jsonl"
    python3 -I "${TOOL}/cord_analyze.py" "${LOGS}/${TAG}_cord_trace.jsonl" --json "${LOGS}/${TAG}_cord_sb.json" --key-prefix sb. \
        > "${LOGS}/${TAG}_cord_sb.txt" 2>&1
    python3 -I "${TOOL}/cord_analyze.py" "${LOGS}/${TAG}_cord_trace.jsonl" --all > "${LOGS}/${TAG}_cord_all.txt" 2>&1
    python3 -I -B "${TOOL}/r2_cord_summary.py" "${LOGS}/${TAG}_cord_trace.jsonl" --json "${LOGS}/${TAG}_cord_summary.json" \
        | tee "${LOGS}/${TAG}_cord_summary.txt"
else
    echo "NO TRACE at ${TRACE}" | tee -a "${LOGS}/${TAG}_driver.txt"
fi

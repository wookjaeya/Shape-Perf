#!/usr/bin/env bash
# observe_cfs_smoke.sh -- one observation of the unmodified cFS by `kz observe` (sub-gate S1-R2 smoke run).
#
#   observe_cfs_smoke.sh <tag>        e.g. s1r2_p1
#
# Must be started as root with CFS_NORMAL_USER set (the rule of env/run_cfs.sh).  Only ONE cFS run may
# be active on the host; run_cfs.sh refuses to start if another instance exists.  This script changes
# nothing of the run's conditions: the cFS command is env/run_cfs.sh with the arguments of the reference
# gdb runs (env/run_r2_gdb.sh), minus the gdb wrapper.
#
# Steps (every output is a new file in env/logs/ with the prefix <tag>_):
#   1. persisted-state snapshot (env/tools/persist_snapshot.sh), as env/run_r2_gdb.sh does
#   2. the basis of core-cpu1 for the observation profile (kz anchor; reads the files, changes none)
#   3. the conditions file of the run (what the caller declares; deviations with reason and effect)
#   4. kz observe -- env/run_cfs.sh operational ci-es-restart-poweron <log>
#   5. persisted-state snapshot again
#   6. counts from the run container (kz hits)
# Nothing is analysed here: the counts are for the next stage's comparison with the reference.
set -euo pipefail
TAG="${1:?tag}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KAIROS="$(cd "${HERE}/../.." && pwd)"
ENV_DIR="$(cd "${KAIROS}/../env" && pwd)"
LOGS="${ENV_DIR}/logs"
CFS_DIR="${CFS_DIR:-/home/user/work/procrace/cfs_ref/cFS}"
EXE_DIR="${CFS_DIR}/build-native_std/exe/cpu1"
KZ="${KZ:?set KZ to the kz executable}"
PROFILE="${KAIROS}/profiles/cfs-v7.0.1/observe/sites.kzp"
: "${CFS_NORMAL_USER:?set CFS_NORMAL_USER (env/ENV.md E1)}"
[ "$(id -u)" = 0 ] || { echo "must run as root (the observer is root: D1)" >&2; exit 2; }
for f in driver.txt persist_before.txt persist_after.txt basis.kz conditions.tsv kzt kzt.record.txt hits.txt; do
    [ ! -e "${LOGS}/${TAG}_${f}" ] || { echo "refusing to overwrite ${LOGS}/${TAG}_${f}" >&2; exit 2; }
done
DRIVER="${LOGS}/${TAG}_driver.txt"
CFS_LOG="${LOGS}/${TAG}_perf_operational_esrestart.log"
[ ! -e "${CFS_LOG}" ] || { echo "refusing to overwrite ${CFS_LOG}" >&2; exit 2; }

{
    echo "observe_cfs_smoke.sh tag=${TAG} start_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)"
    echo "kz=${KZ} sha256=$(sha256sum "${KZ}" | cut -c1-64)"
    echo "profile=${PROFILE} sha256=$(sha256sum "${PROFILE}" | cut -c1-64)"
    echo "CFS_NORMAL_USER=${CFS_NORMAL_USER}  CFS_DIR=${CFS_DIR}"
    echo "core-cpu1 sha256=$(sha256sum "${EXE_DIR}/core-cpu1" | cut -c1-64)"
} > "${DRIVER}"

# 1. persisted state before
"${ENV_DIR}/tools/persist_snapshot.sh" "before ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_before.txt" 2>&1

# 2. basis
"${KZ}" anchor "${EXE_DIR}/core-cpu1" "${PROFILE}" -o "${LOGS}/${TAG}_basis.kz" --source-root "${CFS_DIR}" >> "${DRIVER}" 2>&1

# 3. conditions declared by the caller (tab separated; kairos/probe/loader/run_record.hpp)
T=$'\t'
BOOT="$(uptime -s)"
{
    echo "# conditions of ${TAG}: declared by the caller of kz observe"
    echo "item${T}cfs.mode${T}operational (env/run_cfs.sh: stop when the console shows CFE_ES_Main entering OPERATIONAL state)${T}from-reference"
    echo "item${T}cfs.stop${T}ci-es-restart-poweron (the bundle CI's ES restart command, RestartType 2; PI decision D3 on the stop method is open)${T}from-reference"
    echo "item${T}cfs.user${T}${CFS_NORMAL_USER} (run_cfs.sh drops to it with setpriv; README: run as a normal user)${T}from-reference"
    echo "item${T}cfs.binary${T}build-native_std/exe/cpu1/core-cpu1, unmodified${T}from-reference"
    echo "item${T}cfs.working_directory${T}${EXE_DIR} (README L111)${T}from-reference"
    echo "item${T}observer.user${T}root (PI decision D1: the observer runs as root, cFS as the normal user)${T}from-reference"
    echo "item${T}observer.method${T}kz observe, perf-sample backend, fidelity F0 (uprobes by file and offset; the file on disk is not modified)${T}deviation"
    echo "item${T}run.count${T}1${T}deviation"
    echo "item${T}run.stratum${T}cold: the first cFS run since the VM boot at ${BOOT} UTC (declared, not detected)${T}declared-assumption"
    echo "item${T}persist.before${T}${TAG}_persist_before.txt sha256 $(sha256sum "${LOGS}/${TAG}_persist_before.txt" | cut -c1-64)${T}unset"
    echo "item${T}host.kernel_vs_reference${T}$(uname -r) (reference runs: 6.18.44-fc-v80, env/logs/r2_g1_gdb_operational_esrestart.log.meta)${T}deviation"
    echo "deviation${T}observer.method${T}gdb -batch breakpoints under RUN_CFS_WRAP, all-stop (env/run_r2_gdb.sh)${T}kz observe perf-sample uprobes, no stop of the process${T}this run exercises the new observer (sub-gate S1-R2 smoke run)${T}the two methods perturb the run differently (gdb stops the process at every hit, uprobes do not); no equivalence of the two observations is claimed${T}no"
    echo "deviation${T}run.count${T}3 (r2_g1, r2_g2, r2_g3)${T}1${T}the task is one smoke run; N per purpose is decided at S1 from a pilot (PI decision D14)${T}no frequency, spread or comparison with the three reference runs can be claimed from one run${T}no"
    echo "deviation${T}host.kernel_vs_reference${T}6.18.44-fc-v80 (reference runs, 2026-10-08)${T}$(uname -r)${T}the VM kernel changed since the reference runs; group A is not re-measured yet (PI decision D11)${T}timing and scheduling may differ from the reference runs; not quantified${T}no"
    echo "deviation${T}run.stratum${T}cold = file cache not warmed since the VM boot (env/BASELINE_MEASURE.md)${T}cold by the first-run criterion; the cache state was neither measured nor controlled: core-cpu1 was read by sha256sum, objdump and kz anchor before the run${T}the cache of the executable is warm; reading the apps' .so files before the run was not done by this procedure, other readers since the boot are unknown${T}the label cold is not a measurement; the startup order and duration may differ from a cold boot${T}yes"
} > "${LOGS}/${TAG}_conditions.tsv"

# 4. the observation
set +e
env CFS_NORMAL_USER="${CFS_NORMAL_USER}" "${KZ}" observe "${LOGS}/${TAG}_basis.kz" "${PROFILE}" \
    -o "${LOGS}/${TAG}.kzt" --record "${LOGS}/${TAG}.kzt.record.txt" --conditions "${LOGS}/${TAG}_conditions.tsv" --stratum cold --run-ordinal 0 \
    -- "${ENV_DIR}/run_cfs.sh" operational ci-es-restart-poweron "${CFS_LOG}" >> "${DRIVER}" 2>&1
RC=$?
set -e
echo "kz observe exit=${RC} end_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)" >> "${DRIVER}"

# 5. persisted state after
"${ENV_DIR}/tools/persist_snapshot.sh" "after ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_after.txt" 2>&1

# 6. counts
if [ -e "${LOGS}/${TAG}.kzt" ]; then
    {
        "${KZ}" hits "${LOGS}/${TAG}.kzt"
        echo
        "${KZ}" hits "${LOGS}/${TAG}.kzt" --site evs.send.entry --before sb.appid.store --group-slot app_id --group-return-slot status
    } > "${LOGS}/${TAG}_hits.txt" 2>&1 || true
fi
exit "${RC}"

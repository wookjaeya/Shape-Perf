#!/usr/bin/env bash
# measure_baseline.sh -- measurement M1 (BASELINE_MEASURE.md): one fixed-window run of the documented cFS
# executable through run_cfs.sh v3, with event-based snapshots and persisted-data records before and after.
#
# Usage: CFS_NORMAL_USER=ubuntu measure_baseline.sh <A|B> <run index>
#   A  "probe-only": /proc reads, mq_getattr on reopened queue descriptors, /proc/<pid>/mem reads. No command is
#      sent to cFS except the stop.
#   B  "with SB pipe info": as A, plus at S2 the SB Write-Pipe-Info command and the file it writes to /ram.
#
# What is fixed and where it comes from (nothing else is chosen here):
#   window   fixed, N = CFE_PLATFORM_CORE_MAX_STARTUP_MSEC / 1000 = 30 s, read from the pinned cFE source; the same
#            reference constant as CONDITIONS.md E10 (run07).  The S2 hook runs at the end of the window and the
#            stop follows it.
#   stop     ci-es-restart-poweron (CONDITIONS.md E11 candidate (a); the experiments' stop method is still PI
#            decision D3 / E11 -- this run uses (a) for characterisation only).
#   S1, S2   event-based (run_cfs.sh v3 hook points); see run_cfs.sh header.
#   failure bound for waiting on the pipe-info file: GRACE_MSEC of run_cfs.sh (5000 ms, H-4 assumption).
#   run count and A-before-B order: unspecified_by_reference (characterisation assumption; BASELINE_MEASURE.md §1).
# Outputs: logs/m1_<V><i>_po_fixed30s_esrestart.log (+ .meta, .hook, .S1/.S2[/.S2b].{json,txt}, .pipeinfo.*),
#          logs/m1_<V><i>_persist_{before,after}.txt, logs/m1_layout.json, tools/bin/mq_probe (built if missing).
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CFS_DIR="${CFS_DIR:-/home/user/work/procrace/cfs_ref/cFS}"
EXE_DIR="${CFS_DIR}/build-native_std/exe/cpu1"
HOST_DIR="${CFS_DIR}/build-native_std/exe/host"
TOOLS="${SCRIPT_DIR}/tools"
LOGS="${SCRIPT_DIR}/logs"
[ $# -eq 2 ] || { sed -n '2,24p' "$0" >&2; exit 2; }
VARIANT="$1"; IDX="$2"
case "${VARIANT}" in A|B) ;; *) echo "variant must be A or B" >&2; exit 2 ;; esac
[ -n "${CFS_NORMAL_USER:-}" ] || { echo "CFS_NORMAL_USER must be set (README L102, run_cfs.sh)" >&2; exit 2; }

# reference constants, read the same way as run_cfs.sh does
CORE_MAX_STARTUP_MSEC="$(grep -h -E '^#define[[:space:]]+(DEFAULT_)?CFE_PLATFORM_CORE_MAX_STARTUP_MSEC[[:space:]]+[0-9]+' \
    "${CFS_DIR}/cfe/modules/core_private/config/default_cfe_core_private_internal_cfg.h" | awk '{print $3}')"
KILL="$(grep -h -E '^#define[[:space:]]+(DEFAULT_)?CFE_PLATFORM_ES_APP_KILL_TIMEOUT[[:space:]]+[0-9]+' "${CFS_DIR}/cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h" | awk '{print $3}')"
SCAN="$(grep -h -E '^#define[[:space:]]+(DEFAULT_)?CFE_PLATFORM_ES_APP_SCAN_RATE[[:space:]]+[0-9]+' "${CFS_DIR}/cfe/modules/es/fsw/inc/cfe_es_internal_cfg.h" | awk '{print $3}')"
N=$(( CORE_MAX_STARTUP_MSEC / 1000 ))
GRACE_MSEC=$(( KILL * SCAN ))

# tools
mkdir -p "${TOOLS}/bin"
PROBE="${TOOLS}/bin/mq_probe"
if [ ! -x "${PROBE}" ] || [ "${TOOLS}/mq_probe.c" -nt "${PROBE}" ]; then
    gcc -O0 -g -Wall -Wextra -o "${PROBE}" "${TOOLS}/mq_probe.c" -lrt
fi
LAYOUT="${LOGS}/m1_layout.json"
if [ ! -s "${LAYOUT}" ] || ! python3 -I -c 'import json,hashlib,sys
L=json.load(open(sys.argv[1]))
for o in L["objects"].values():
    if hashlib.sha256(open(sys.argv[2]+"/"+o["file"],"rb").read()).hexdigest()!=o["sha256"]: sys.exit(1)' "${LAYOUT}" "${EXE_DIR}"; then
    python3 -I "${TOOLS}/cfs_layout.py" "${EXE_DIR}" "${LAYOUT}"
fi

TAG="m1_${VARIANT}${IDX}"
LOG="${LOGS}/${TAG}_po_fixed${N}s_esrestart.log"
[ ! -e "${LOG}" ] || { echo "ERROR: ${LOG} exists; refusing to overwrite" >&2; exit 2; }
"${TOOLS}/persist_snapshot.sh" "before ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_before.txt" 2>&1
cat > "${LOG}.hookconf" <<EOF
VARIANT=${VARIANT}
TOOLS=${TOOLS}
LAYOUT=${LAYOUT}
PROBE=${PROBE}
HOST_DIR=${HOST_DIR}
RUN_USER=${CFS_NORMAL_USER}
GRACE_MSEC=${GRACE_MSEC}
EOF
{
    echo "measure_baseline.sh ${TAG}: variant=${VARIANT} window=${N}s (CFE_PLATFORM_CORE_MAX_STARTUP_MSEC=${CORE_MAX_STARTUP_MSEC}) stop=ci-es-restart-poweron grace=${GRACE_MSEC}ms"
    echo "mq_probe sha256=$(sha256sum "${PROBE}" | cut -d' ' -f1) layout=$(basename "${LAYOUT}")"
    echo "tools sha256: $(cd "${TOOLS}" && sha256sum mq_probe.c cfs_layout.py cfs_snapshot.py sb_pipeinfo_parse.py inotify_wait.py m1_hook.sh persist_snapshot.sh | awk '{printf "%s=%s ", $2, substr($1,1,16)}')"
    echo "run_cfs.sh sha256=$(sha256sum "${SCRIPT_DIR}/run_cfs.sh" | cut -d' ' -f1)"
} > "${LOGS}/${TAG}_driver.txt"
rc=0
RUN_CFS_HOOK="${TOOLS}/m1_hook.sh" "${SCRIPT_DIR}/run_cfs.sh" "${N}" ci-es-restart-poweron "${LOG}" > /dev/null || rc=$?
echo "run_cfs.sh exit=${rc}" >> "${LOGS}/${TAG}_driver.txt"
"${TOOLS}/persist_snapshot.sh" "after ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_after.txt" 2>&1
echo "${TAG} done (run_cfs.sh exit ${rc}); log ${LOG}"
exit "${rc}"

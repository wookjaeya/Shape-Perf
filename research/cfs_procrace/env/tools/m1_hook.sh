#!/usr/bin/env bash
# m1_hook.sh -- RUN_CFS_HOOK for measurement M1 (BASELINE_MEASURE.md).  Called by run_cfs.sh v3 as
#   m1_hook.sh <S1|S2> <pid> <log_file> <ms since launch>
# Configuration comes from <log_file>.hookconf (written by measure_baseline.sh), not from the environment, so that
# nothing is added to core-cpu1's environment.
#
#   S1, S2 (both variants): cfs_snapshot.py -> <base>.S1.{json,txt}, <base>.S2.{json,txt}
#   S2, variant B only, after the S2 snapshot:
#     1. start inotify_wait.py on /dev/shm/osal:RAM for IN_CLOSE_WRITE of cfe_sb_pipe.dat; wait for its READY
#     2. send SB "Write Pipe Info" with the bundle's cmd_send in the CI style (as the run user, like run_cfs.sh's
#        ES restart): --pktid=0x1803 --cmdcode=7 --string="64:"  (empty file name -> CFE_PLATFORM_SB_DEFAULT_PIPE_FILENAME
#        "/ram/cfe_sb_pipe.dat")
#     3. wait for the close event (bound = GRACE_MSEC, a failure bound only), copy the file, parse it
#     4. cfs_snapshot.py again -> <base>.S2b (state right after the command, for the file-vs-memory comparison)
set -uo pipefail
point="$1"; pid="$2"; log="$3"; ms="$4"
# shellcheck disable=SC1090
. "${log}.hookconf"
base="${log%.log}"
mono() { python3 -I -c 'import time; print("%.6f" % time.clock_gettime(time.CLOCK_MONOTONIC))'; }
echo "== hook ${point} pid=${pid} at ${ms} ms since launch, mono=$(mono)"
snap() { python3 -I "${TOOLS}/cfs_snapshot.py" "${pid}" "${LAYOUT}" "${PROBE}" "${base}.$1" "$1"; }
snap "${point}"
if [ "${point}" = S2 ] && [ "${VARIANT}" = B ]; then
    ramdir=/dev/shm/osal:RAM
    pre="absent"; [ -e "${ramdir}/cfe_sb_pipe.dat" ] && pre="present size=$(stat -c %s "${ramdir}/cfe_sb_pipe.dat") mtime=$(date -u -r "${ramdir}/cfe_sb_pipe.dat" +%Y-%m-%dT%H:%M:%S.%NZ)"
    echo "pipe file before command: ${pre}"
    coproc W { python3 -I "${TOOLS}/inotify_wait.py" "${ramdir}" cfe_sb_pipe.dat "${GRACE_MSEC}"; }
    IFS= read -r ready <&"${W[0]}"
    echo "waiter: ${ready} mono=$(mono)"
    t_cmd="$(mono)"
    echo "cmd_send at mono=${t_cmd}:"
    setpriv --reuid="${RUN_USER}" --regid="$(id -g "${RUN_USER}")" --init-groups -- \
        "${HOST_DIR}/cmd_send" -v --host=127.0.0.1 --endian=LE --pktid=0x1803 --cmdcode=7 --string="64:" 2>&1 | sed 's/^/  /'
    echo "cmd_send exit=${PIPESTATUS[0]}"
    IFS= read -r ev <&"${W[0]}"
    wait "${W_PID}" 2>/dev/null; wrc=$?
    echo "waiter: ${ev} (rc=${wrc}); command-to-close_ms=$(python3 -I -c "import sys; e=sys.argv[1].split(); print('%.1f' % ((float(e[2])-float(sys.argv[2]))*1000) if e and e[0]=='CLOSE_WRITE' else 'n/a')" "${ev}" "${t_cmd}")"
    if [ -e "${ramdir}/cfe_sb_pipe.dat" ]; then
        cp -p "${ramdir}/cfe_sb_pipe.dat" "${base}.pipeinfo.dat"
        echo "copied: size=$(stat -c %s "${base}.pipeinfo.dat") sha256=$(sha256sum "${base}.pipeinfo.dat" | cut -d' ' -f1)"
        python3 -I "${TOOLS}/sb_pipeinfo_parse.py" "${base}.pipeinfo.dat" "${base}.pipeinfo"
    fi
    snap S2b
fi
echo "== hook ${point} end mono=$(mono)"

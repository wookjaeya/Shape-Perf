#!/usr/bin/env bash
# run_cfs.sh -- run the documented cFS executable once, stop it by a named and referenced method,
#               and record the run (console log + .meta).
#
# What is run (nothing else is set; CONDITIONS.md section E):
#   README.md at nasa/cFS 088b2fa828db9ff7e00733f1908e0eeb59f66ce3
#     L102  "To prep, compile, and run on the host (from cFS directory above) as a normal user
#            (best effort message queue depth and task priorities):"
#     L109  "In order to boot CFE, the default linux PSP requires that the working directory be set to
#            the location of the staged binaries:"
#     L111  cd build-native_std/exe/cpu1/
#     L112  ./core-cpu1                     (no command-line options)
#     L114  "Should see startup messages, and CFE_ES_Main entering OPERATIONAL state."
#
# Usage:
#   run_cfs.sh MODE STOP [LOG_FILE]
#     MODE  "operational": stop as soon as the README L114 line is read from the console stream (event-based;
#           the stream is followed with GNU tail, which uses inotify), bounded by
#           CFE_PLATFORM_CORE_MAX_STARTUP_MSEC (read from the pinned cFE source).
#           N (positive integer): stop N seconds after launch. The caller chooses N and must record its source.
#     STOP  (no default: the stop method decides the reset type of the next boot; CONDITIONS.md E7, E11)
#           "console-sigint"         SIGINT to the process group, i.e. what CTRL+C at a console delivers.
#                                    psp/fsw/pc-linux/src/cfe_psp_exception.c L223-233. The PSP treats it as an
#                                    exception (L188-195: the handler exists "to exercise and test the exception
#                                    handling"), and the process exits with PROCESSOR reset status until
#                                    CFE_PLATFORM_ES_MAX_PROCESSOR_RESETS is reached.
#           "ci-es-restart-poweron"  ES Restart command, RestartType 2 = POWERON, sent with the bundle's own cmd_send
#                                    exactly as the bundle CI does (.github/workflows/build-run-app-reusable.yml
#                                    L213-214: ./cmd_send -v --host=<cpu1> --endian=LE --pktid=0x1806 --cmdcode=2
#                                    --half=0x0002). <cpu1> is 127.0.0.1, cmd_send's own default host
#                                    (tools/commandline-tools/src/cmd_send.c L52-53), because core-cpu1 runs on
#                                    this host instead of in a separate container. CI_LAB listens on UDP 1234.
#                                    psp/fsw/pc-linux/src/cfe_psp_support.c L62-69 deletes the reserved-memory
#                                    SHM on POWERON, so the next boot is a POWER ON reset.
#           If the process has not exited within GRACE (below), the console keys are escalated: SIGINT (only
#           after the ES command), SIGQUIT (CTRL+\, cfe_psp_start.c L467-474), then SIGKILL. All recorded.
#     LOG_FILE  console log path (default <this script dir>/logs/run_<UTC timestamp>.log). A companion
#           <LOG_FILE>.meta holds the run record.
#   Environment:
#     CFS_DIR          clone location (default /home/user/work/procrace/cfs_ref/cFS)
#     CFS_NORMAL_USER  required when started as root: the existing non-root account to run as (README L102).
#                      core-cpu1 (and cmd_send) are exec'ed with setpriv: uid, gid and supplementary groups of that
#                      account only; no capability, rlimit, scheduler or environment change (CONDITIONS.md E1, E12).
#
# Isolation (CONDITIONS.md E13): before launch, the script records the host-shared state (/dev/shm, SysV IPC,
# UDP sockets, the run user's processes) and refuses to start if another cFS instance or OSAL process is active
# (lib_record.sh: rec_osal_conflicts). It does not clean anything.
#
# Harness-only aspects (not cFS settings; CONDITIONS.md H-*): setsid (H-1), stdin /dev/null and console to a
# regular file (H-2), event-based detection of the README L114 line with tail -f (H-3), GRACE bound (E11).
#
# Optional measurement hook (v3; CONDITIONS.md H-7, BASELINE_MEASURE.md): if RUN_CFS_HOOK names an executable,
# it is called synchronously as  <hook> <point> <pid> <log_file> <ms since launch>  at two points:
#   S1  fixed MODE only: right after the README L114 line is read (after the existing thread capture);
#   S2  every MODE: when the wait ends (duration elapsed / README L114 line in operational mode), immediately before
#       the stop action. Not called if core-cpu1 exited by itself.
# Hook output goes to <LOG_FILE>.hook, never to the console log. RUN_CFS_HOOK is removed from the environment
# before core-cpu1 is launched (export -n), so the process environment is the same as without the hook. While a
# hook runs, the wait loop does not run; in fixed mode the stop therefore comes after the S2 hook returns, and the
# .meta records both times. With RUN_CFS_HOOK unset or empty, v3 behaves exactly as v2 and writes the same .meta
# lines (only the version string differs).
#
# Optional launch wrapper (v4; CONDITIONS.md H-8): if RUN_CFS_WRAP is set, its words are placed between the
# user switch and ./core-cpu1, e.g. RUN_CFS_WRAP="gdb -batch -nx -x <script> -ex run --args" (case R2 uses the
# reporter's method, a gdb breakpoint). PID is then the wrapper's; the core-cpu1 child PID is recorded in the .meta.
# RUN_CFS_WRAP is removed from the environment before launch. With RUN_CFS_WRAP unset or empty, v4 behaves exactly
# as v3.
#
# Script history: v1 (2026-10-08T01:29Z) produced logs/run01_*, run02_* and the audit's run03; it polled the log
# every 50 ms and had SIGINT as its only stop. v2 (after the audit): STOP argument, event-based detection,
# isolation check, process credentials/limits/environment in the .meta; produced run04-run07. v3 (2026-10-08,
# measurement M1): optional RUN_CFS_HOOK only. v4 (this file, 2026-10-08, case R2): optional RUN_CFS_WRAP only.

set -euo pipefail

SCRIPT_VERSION="v4"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib_record.sh
. "${SCRIPT_DIR}/lib_record.sh"
CFS_DIR="${CFS_DIR:-/home/user/work/procrace/cfs_ref/cFS}"
EXE_DIR="${CFS_DIR}/build-native_std/exe/cpu1"            # README L111
HOST_DIR="${CFS_DIR}/build-native_std/exe/host"
CFE_DIR="${CFS_DIR}/cfe"
OPER_LINE="CFE_ES_Main entering OPERATIONAL state"      # README L114

if [ $# -lt 2 ]; then
    sed -n '2,73p' "$0" >&2
    exit 2
fi
DURATION="$1"
STOP="$2"
LOG_FILE="${3:-${SCRIPT_DIR}/logs/run_$(date -u +%Y%m%dT%H%M%SZ).log}"
HOOK="${RUN_CFS_HOOK:-}"
export -n RUN_CFS_HOOK 2>/dev/null || true       # never part of core-cpu1's environment
if [ -n "${HOOK}" ] && [ ! -x "${HOOK}" ]; then echo "ERROR: RUN_CFS_HOOK=${HOOK} is not executable" >&2; exit 2; fi
WRAP_STR="${RUN_CFS_WRAP:-}"
export -n RUN_CFS_WRAP 2>/dev/null || true       # never part of core-cpu1's environment
WRAP=()
[ -z "${WRAP_STR}" ] || read -r -a WRAP <<< "${WRAP_STR}"
mkdir -p "$(dirname "${LOG_FILE}")"
LOG_FILE="$(cd "$(dirname "${LOG_FILE}")" && pwd)/$(basename "${LOG_FILE}")"   # absolute: the script cd's to EXE_DIR
META="${LOG_FILE}.meta"
case "${STOP}" in
    console-sigint|ci-es-restart-poweron) ;;
    *) echo "ERROR: STOP must be console-sigint or ci-es-restart-poweron (no default; CONDITIONS.md E11)" >&2; exit 2 ;;
esac

# ---------------------------------------------------------------- reference constants from pinned cFE source
cfgval() {  # name -> value of DEFAULT_<name> or plain #define
    local name="$1" v
    v="$(grep -rh -E "^#define[[:space:]]+(DEFAULT_)?${name}[[:space:]]+[0-9]+" \
           "${CFE_DIR}/modules/es/fsw/inc/cfe_es_internal_cfg.h" \
           "${CFE_DIR}/modules/core_private/config/default_cfe_core_private_internal_cfg.h" 2>/dev/null \
         | awk '{print $3}' | head -1)"
    [ -n "$v" ] || { echo "ERROR: cannot read ${name} from cFE source" >&2; exit 3; }
    echo "$v"
}
CORE_MAX_STARTUP_MSEC="$(cfgval CFE_PLATFORM_CORE_MAX_STARTUP_MSEC)"
APP_KILL_TIMEOUT="$(cfgval CFE_PLATFORM_ES_APP_KILL_TIMEOUT)"
APP_SCAN_RATE="$(cfgval CFE_PLATFORM_ES_APP_SCAN_RATE)"
GRACE_MSEC=$(( APP_KILL_TIMEOUT * APP_SCAN_RATE ))

case "${DURATION}" in
    operational) MODE=operational; LIMIT_MSEC="${CORE_MAX_STARTUP_MSEC}" ;;
    ''|*[!0-9]*) echo "ERROR: MODE must be 'operational' or a positive integer (seconds)" >&2; exit 2 ;;
    *) [ "${DURATION}" -gt 0 ] || { echo "ERROR: duration must be > 0" >&2; exit 2; }
       MODE=fixed; LIMIT_MSEC=$(( DURATION * 1000 )) ;;
esac

# ---------------------------------------------------------------- user (README L102)
LAUNCH=()
if [ "$(id -u)" -eq 0 ]; then
    if [ -z "${CFS_NORMAL_USER:-}" ] || ! id -u "${CFS_NORMAL_USER}" >/dev/null 2>&1 \
       || [ "$(id -u "${CFS_NORMAL_USER}")" -eq 0 ]; then
        echo "ERROR: started as root; README L102 prescribes a normal user. Set CFS_NORMAL_USER to an existing non-root account." >&2
        exit 2
    fi
    LAUNCH=(setpriv --reuid="${CFS_NORMAL_USER}" --regid="$(id -g "${CFS_NORMAL_USER}")" --init-groups --)
    RUN_USER="${CFS_NORMAL_USER}"
else
    RUN_USER="$(id -un)"
fi

[ -x "${EXE_DIR}/core-cpu1" ] || { echo "ERROR: ${EXE_DIR}/core-cpu1 not found; run build_cfs.sh first" >&2; exit 3; }
[ "${STOP}" != ci-es-restart-poweron ] || [ -x "${HOST_DIR}/cmd_send" ] || { echo "ERROR: ${HOST_DIR}/cmd_send not found" >&2; exit 3; }

# The documented run sets none of these; ld.so(8) and glibc read them in every process.
for v in $(compgen -e | grep -E '^(LD_|MALLOC_)' || true) GLIBC_TUNABLES; do
    if printenv "${v}" >/dev/null 2>&1; then
        echo "ERROR: environment variable ${v} is set; the documented run does not set it." >&2; exit 4
    fi
done

now_us() { echo "${EPOCHREALTIME/./}"; }
rel_ms() { echo $(( ($1 - T0_US) / 1000 )); }
PRIV="$(mktemp -d)"; chmod 700 "${PRIV}"     # raw /proc copies (the raw environment may hold credentials)
cleanup() { rm -rf "${PRIV}"; }
trap cleanup EXIT

# ---------------------------------------------------------------- isolation check (CONDITIONS.md E13)
{
    echo "run_cfs.sh ${SCRIPT_VERSION} metadata"
    echo "start_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)"
    echo "mode=${MODE} limit_msec=${LIMIT_MSEC} stop=${STOP} grace_msec=${GRACE_MSEC}"
    echo "ref.CFE_PLATFORM_CORE_MAX_STARTUP_MSEC=${CORE_MAX_STARTUP_MSEC}"
    echo "ref.CFE_PLATFORM_ES_APP_KILL_TIMEOUT=${APP_KILL_TIMEOUT} ref.CFE_PLATFORM_ES_APP_SCAN_RATE=${APP_SCAN_RATE}"
    echo "cwd=${EXE_DIR}"
    echo "command=./core-cpu1"
    echo "launcher=setsid ${LAUNCH[*]}"
    echo "run_user=${RUN_USER} ($(id "${RUN_USER}"))"
    echo "bundle=$(git -c safe.directory='*' -C "${CFS_DIR}" rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "core-cpu1 sha256=$(sha256sum "${EXE_DIR}/core-cpu1" | cut -d' ' -f1) mtime=$(date -u -r "${EXE_DIR}/core-cpu1" +%Y-%m-%dT%H:%M:%SZ)"
    echo "uname=$(uname -a)"
    echo "nproc=$(nproc)"
    echo "mqueue.msg_max=$(cat /proc/sys/fs/mqueue/msg_max) mqueue.msgsize_max=$(cat /proc/sys/fs/mqueue/msgsize_max) mqueue.queues_max=$(cat /proc/sys/fs/mqueue/queues_max)"
    echo "kernel.sched_rt_runtime_us=$(cat /proc/sys/kernel/sched_rt_runtime_us 2>/dev/null || echo n/a)"
    echo "kernel.sched_autogroup_enabled=$(cat /proc/sys/kernel/sched_autogroup_enabled 2>/dev/null || echo n/a)"
    echo "keyfiles_in_cwd_before=$(cd "${EXE_DIR}" && ls -a .cdskeyfile .resetkeyfile .reservedkeyfile 2>/dev/null | tr '\n' ' ')"
    rec_host_snapshot "before run" "${RUN_USER}"
} > "${META}"
CONFLICTS="$(rec_osal_conflicts "${CFS_DIR}" "${RUN_USER}")"
if [ -n "${CONFLICTS}" ]; then
    { echo "isolation_check=REFUSED"; echo "${CONFLICTS}" | sed 's/^/  /'; } >> "${META}"
    echo "ERROR: another cFS/OSAL activity is present; run refused (see ${META}):" >&2
    echo "${CONFLICTS}" >&2
    exit 8
fi
echo "isolation_check=passed (no other cFS instance, no process using /dev/shm/osal:* or an OSAL-style mqueue, no executable from this build tree, UDP ${REC_UDP_PORTS} free, no attached SysV shm of ${RUN_USER})" >> "${META}"
STAMP="${PRIV}/stamp"; : > "${STAMP}"

# ---------------------------------------------------------------- launch (README L111-112)
cd "${EXE_DIR}"
: > "${LOG_FILE}"
FIFO="${PRIV}/console.fifo"; mkfifo "${FIFO}"
T0_US="$(now_us)"
setsid "${LAUNCH[@]}" "${WRAP[@]}" ./core-cpu1 < /dev/null > "${LOG_FILE}" 2>&1 &
PID=$!
tail -n +1 --pid="${PID}" -f -- "${LOG_FILE}" > "${FIFO}" 2>/dev/null &
TAILPID=$!
exec {RD}<"${FIFO}"

capture_static() {   # raw copies first (fast); they are processed after the run
    T_CAP_US="$(now_us)"
    cat "/proc/${PID}/status"  > "${PRIV}/status"  2>/dev/null || true
    cat "/proc/${PID}/limits"  > "${PRIV}/limits"  2>/dev/null || true
    cat "/proc/${PID}/environ" > "${PRIV}/environ" 2>/dev/null || true
    cat "/proc/${PID}/cgroup"  > "${PRIV}/cgroup"  2>/dev/null || true
    tr '\0' ' ' < "/proc/${PID}/cmdline" > "${PRIV}/cmdline" 2>/dev/null || true
    readlink "/proc/${PID}/exe" "/proc/${PID}/cwd" "/proc/${PID}/fd/0" "/proc/${PID}/fd/1" "/proc/${PID}/fd/2" > "${PRIV}/links" 2>/dev/null || true
    [ ${#WRAP[@]} -eq 0 ] || pgrep -a -P "${PID}" > "${PRIV}/children" 2>/dev/null || true
    T_CAP_END_US="$(now_us)"
}
HOOK_LOG=""
run_hook() {   # $1 = S1 | S2; synchronous; output to <LOG_FILE>.hook
    [ -n "${HOOK}" ] || return 0
    local t0 t1 r=0
    t0="$(now_us)"
    "${HOOK}" "$1" "${PID}" "${LOG_FILE}" "$(rel_ms "${t0}")" >> "${LOG_FILE}.hook" 2>&1 < /dev/null || r=$?
    t1="$(now_us)"
    HOOK_LOG="${HOOK_LOG:+${HOOK_LOG},}$1@$(rel_ms "${t0}")-$(rel_ms "${t1}")ms(rc=${r})"
}
capture_threads() {
    ps -L -o tid,cls,rtprio,ni,pri,psr,comm -p "${PID}" > "${PRIV}/threads" 2>&1 || true
    T_THR_US="$(now_us)"
}

# ---------------------------------------------------------------- wait: README L114 event or MODE bound
OPER_US=""; FIRST_US=""; T_CAP_US=""; T_CAP_END_US=""; T_THR_US=""
DEADLINE_US=$(( T0_US + LIMIT_MSEC * 1000 ))
stop_reason=""
while :; do
    rem=$(( DEADLINE_US - $(now_us) ))
    if [ "${rem}" -le 0 ]; then
        if [ "${MODE}" = operational ]; then stop_reason="limit_reached_without_operational"; else stop_reason="duration_elapsed"; fi
        break
    fi
    rc=0
    IFS= read -r -t "$(printf '%d.%06d' $(( rem / 1000000 )) $(( rem % 1000000 )))" -u "${RD}" line || rc=$?
    if [ "${rc}" -eq 0 ]; then
        if [ -z "${FIRST_US}" ]; then FIRST_US="$(now_us)"; capture_static; fi
        if [ -z "${OPER_US}" ] && [[ "${line}" == *"${OPER_LINE}"* ]]; then
            OPER_US="$(now_us)"
            if [ "${MODE}" = operational ]; then stop_reason="operational_seen"; break; fi
            capture_threads
            run_hook S1
        fi
    elif [ "${rc}" -gt 128 ]; then
        continue            # read timed out; the deadline test at the top of the loop decides
    else
        stop_reason="exited_by_itself"; break     # EOF: tail ended because core-cpu1 exited
    fi
done

# ---------------------------------------------------------------- stop
EXIT_RC=""
wait_exit() {   # $1 = bound in ms; returns 0 once core-cpu1 has exited (EXIT_RC set)
    local s w r=0
    sleep "$(printf '%d.%03d' $(( $1 / 1000 )) $(( $1 % 1000 )))" &
    s=$!
    wait -n -p w "${PID}" "${s}" || r=$?
    if [ "${w}" = "${PID}" ]; then
        EXIT_RC="${r}"; kill "${s}" 2>/dev/null || true; wait "${s}" 2>/dev/null || true; return 0
    fi
    return 1
}
STOP_LOG=""
T_STOP_US=""
signal_group() {
    STOP_LOG="${STOP_LOG:+${STOP_LOG},}SIG$1@$(rel_ms "$(now_us)")ms"
    kill -s "$1" -- "-${PID}" 2>/dev/null || true
}
CMD_SEND_RC="n/a"
TAIL_FDS=""
if [ "${stop_reason}" != "exited_by_itself" ]; then
    run_hook S2
    T_STOP_US="$(now_us)"
    case "${STOP}" in
        console-sigint)
            signal_group INT ;;
        ci-es-restart-poweron)
            STOP_LOG="cmd_send@$(rel_ms "${T_STOP_US}")ms"
            CMD_SEND_RC=0
            "${LAUNCH[@]}" "${HOST_DIR}/cmd_send" -v --host=127.0.0.1 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 \
                > "${PRIV}/cmd_send.out" 2>&1 || CMD_SEND_RC=$?
            ;;
    esac
    # evidence that tail follows the console with inotify (an anon_inode:inotify descriptor); taken after the
    # stop action so that it cannot delay it
    TAIL_FDS="$(ls -l "/proc/${TAILPID}/fd" 2>/dev/null | awk 'NR>1 {print $NF}' | tr '\n' ' ' || true)"
    if ! wait_exit "${GRACE_MSEC}"; then
        [ "${STOP}" = ci-es-restart-poweron ] && { signal_group INT; wait_exit "${GRACE_MSEC}" || true; }
        if [ -z "${EXIT_RC}" ]; then signal_group QUIT; wait_exit "${GRACE_MSEC}" || true; fi
        if [ -z "${EXIT_RC}" ]; then signal_group KILL; wait_exit "${GRACE_MSEC}" || true; fi
    fi
fi
if [ -z "${EXIT_RC}" ]; then r=0; wait "${PID}" || r=$?; EXIT_RC="${r}"; fi
T_END_US="$(now_us)"
kill "${TAILPID}" 2>/dev/null || true; wait "${TAILPID}" 2>/dev/null || true
exec {RD}<&-
leftover="$(pgrep -g "${PID}" 2>/dev/null | tr '\n' ' ' || true)"

# ---------------------------------------------------------------- record
{
    echo "pid=${PID} pgid=${PID} (setsid)"
    if [ ${#WRAP[@]} -gt 0 ]; then echo "wrapper=${WRAP_STR} (pid is the wrapper's)"; echo "wrapper_children_at_first_console_line:"; sed 's/^/  /' "${PRIV}/children" 2>/dev/null || true; fi
    echo "stop_reason=${stop_reason}"
    echo "first_console_line_read_ms=$( [ -n "${FIRST_US}" ] && rel_ms "${FIRST_US}" || echo none)"
    echo "operational_line_read_ms=$( [ -n "${OPER_US}" ] && rel_ms "${OPER_US}" || echo not_seen)"
    echo "stop_initiated_ms=$( [ -n "${T_STOP_US}" ] && rel_ms "${T_STOP_US}" || echo none)"
    if [ -n "${HOOK}" ]; then echo "hook=${HOOK}"; echo "hook_calls=${HOOK_LOG:-none} (output in $(basename "${LOG_FILE}").hook)"; fi
    echo "stop_actions=${STOP_LOG:-none}"
    echo "cmd_send_exit=${CMD_SEND_RC}"
    if [ -s "${PRIV}/cmd_send.out" ]; then echo "cmd_send_output:"; sed 's/^/  /' "${PRIV}/cmd_send.out"; fi
    echo "exit_status=${EXIT_RC}"
    echo "process_end_ms=$(rel_ms "${T_END_US}")"
    echo "leftover_processes_in_group=${leftover:-none}"
    echo "console.reset_type_at_start=$(grep -o -m1 -E 'Starting the cFE with a (POWER ON|PROCESSOR) reset' "${LOG_FILE}" | sed 's/Starting the cFE with a //' || echo unknown)"
    echo "console.exit_reset_status=$(grep -o -m1 -E 'Exiting cFE with (POWERON|PROCESSOR) Reset status' "${LOG_FILE}" | sed -E 's/Exiting cFE with (.*) Reset status/\1/' || echo none)"
    echo "event_detection=tail -n +1 -f (GNU coreutils $(tail --version | head -1 | awk '{print $NF}')); tail fds while following: ${TAIL_FDS:-n/a}"
    echo "harness_capture_static_ms=$( [ -n "${T_CAP_US}" ] && echo "$(rel_ms "${T_CAP_US}")-$(rel_ms "${T_CAP_END_US}")" || echo none) (raw /proc copies taken when the first console line was read)"
    echo "process.links (exe, cwd, fd0, fd1, fd2):"; sed 's/^/  /' "${PRIV}/links" 2>/dev/null || true
    echo "process.cmdline=$(cat "${PRIV}/cmdline" 2>/dev/null)"
    echo "process.credentials_and_capabilities (/proc/<pid>/status):"
    grep -E '^(Uid|Gid|Groups|CapInh|CapPrm|CapEff|CapBnd|CapAmb|NoNewPrivs|Seccomp|Cpus_allowed_list):' "${PRIV}/status" 2>/dev/null | sed 's/^/  /'
    echo "process.limits (/proc/<pid>/limits):"; sed 's/^/  /' "${PRIV}/limits" 2>/dev/null || true
    echo "process.cgroup:"; sed 's/^/  /' "${PRIV}/cgroup" 2>/dev/null || true
    echo "process.environment (lib_record.sh rules: values only for listed names; cFS reads TERM, TMPDIR, SHELL -- CONDITIONS.md E12):"
    if [ -s "${PRIV}/environ" ]; then rec_env_dump < "${PRIV}/environ" | sed 's/^/  /'; fi
    for v in TERM TMPDIR SHELL; do
        val="$(tr '\0' '\n' < "${PRIV}/environ" 2>/dev/null | grep -m1 "^${v}=" | cut -d= -f2- || true)"
        echo "process.env.${v}=${val:-<unset>}"
    done
    if [ -s "${PRIV}/threads" ]; then
        echo "process.threads at $(rel_ms "${T_THR_US}")ms (fixed mode only; CLS TS = SCHED_OTHER, RR/FF = real time):"
        sed 's/^/  /' "${PRIV}/threads"
    else
        echo "process.threads=not captured (operational mode: capturing would delay the stop)"
    fi
    echo "files_created_or_modified_under_cwd_during_run:"; find . -newer "${STAMP}" 2>&1 | sed 's/^/  /'
    echo "keyfiles_in_cwd_after=$(ls -a .cdskeyfile .resetkeyfile .reservedkeyfile 2>/dev/null | tr '\n' ' ')"
    rec_host_snapshot "after run" "${RUN_USER}"
    echo "end_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)"
} >> "${META}"
cat "${META}"

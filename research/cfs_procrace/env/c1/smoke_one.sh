#!/usr/bin/env bash
# smoke_one.sh CFG CAND -- the build-smoke of one C1 P-REC build (label: build-smoke), reconstructed 2026-10-29 from
# plan/C1_build_manifest.md section 7 and DV-P7 (env/REBUILD_20261029.md).  NOT executed in the 2026-10-29 rebuild
# (the rebuild workflow boots no cFS); it is kept so that the procedure exists in the repository.
#
#   smoke_one.sh <pre|post> <1df262a|1b0b338> [C1_DIR]     run as root; the instances run as CFS_NORMAL_USER (ubuntu)
#
# Steps (CI = nasa/bp b051eb0 .github/workflows/build-run-bp-common.yml):
#   L133-L135  tar of exe/ (cfs-bpapp.tar)                 L154-L155  untar into <ws>/smoke
#   L165-L166  mkdir -p ./tx/storage ./rx/storage
#   L168-L174  ./core-tx and ./core-rx in the background, console to <inst>/console_output.log
#   L176-L177  sleep 5, then the OPERATIONAL line in tx/console_output.log
#   (manifest section 7) thread table and process state at that point
#   L193-L197  ES restart, cmdcode 2, POWERON (0x0002), to UDP 1234 and 1235, after checking that the owner of each port is
#              an instance of this run (manifest section 7)
#   L199-L200  sleep 5
#   The transfer steps (L157-L163, L179-L191, L210-L212) are left out (DV-P7).
# Run user: README.md L107 of both bundle candidates ("as a normal user (best effort message queue depth and task
# priorities)"), by setpriv as env/run_cfs.sh (CONDITIONS.md E1).  No CPU, affinity, priority or sysctl is set.
# Reconstruction choices (RC-6): stdin /dev/null and setsid (CONDITIONS.md H-1, H-2); the port owner check through
# /proc/net/udp and /proc/<pid>/fd (the original used ss, which this host lacks: the DV-P7 defect); chown of the smoke
# directory to the run user (cFS writes EEPROM.DAT, key files and cf/ in its working directory); SIGKILL of instances
# still alive after the last sleep (the stage F procedure, plan/results_instr_gate_20261026.md section 0); the fields of
# smoke.meta.
set -uo pipefail
CFG=${1:?pre|post}; CAND=${2:?1df262a|1b0b338}; C1=${3:-/home/user/work/procrace/c1_build}
U=${CFS_NORMAL_USER:-ubuntu}
WS=$C1/$CFG/$CAND; S=$WS/smoke
[ "$(id -u)" = 0 ] || { echo "run as root (instances run as $U through setpriv)" >&2; exit 2; }
[ -x "$WS/exe/tx/core-tx" ] || { echo "no build at $WS" >&2; exit 2; }
[ ! -e "$S" ] || { echo "refusing: $S exists" >&2; exit 2; }
UID_U=$(id -u "$U"); GID_U=$(id -g "$U")
port_owner() {  # <udp port> -> pids that hold a socket bound to it
  local hex inodes
  hex=$(printf '%04X' "$1")
  inodes=$(awk -v h=":$hex" 'NR > 1 && substr($2, length($2) - 4) == h {print $10}' /proc/net/udp)
  for i in $inodes; do find /proc/[0-9]*/fd -lname "socket:\[$i\]" 2>/dev/null | cut -d/ -f3; done | sort -u
}
mkdir -p "$S"
( cd "$WS/exe" && tar -cf "$S/cfs-bpapp.tar" . )                       # CI L133-L135
( cd "$S" && tar -xf cfs-bpapp.tar )                                    # CI L154-L155
mkdir -p "$S/tx/storage" "$S/rx/storage"                                # CI L165-L166
chown -R "$U:" "$S"
M=$S/smoke.meta
{ echo "# smoke_one.sh $CFG $CAND $(date -u +%FT%TZ) kernel $(uname -r) user $U"
  echo "core-tx sha256 $(sha256sum "$S/tx/core-tx" | cut -c1-64)  core-rx sha256 $(sha256sum "$S/rx/core-rx" | cut -c1-64)"
  echo "msg_max $(cat /proc/sys/fs/mqueue/msg_max) sched_rt_runtime_us $(cat /proc/sys/kernel/sched_rt_runtime_us) sched_autogroup_enabled $(cat /proc/sys/kernel/sched_autogroup_enabled 2>/dev/null)"; } > "$M"
for inst in tx rx; do                                                   # CI L168-L174
  ( cd "$S/$inst" && setsid setpriv --reuid="$UID_U" --regid="$GID_U" --init-groups -- ./core-$inst \
        < /dev/null > console_output.log 2>&1 & echo $! > "$S/$inst.pid" )
done
sleep 5                                                                 # CI L176-L177
if grep -q 'CFE_ES_Main entering OPERATIONAL state' "$S/tx/console_output.log"; then echo "operational tx yes" >> "$M"; else echo "operational tx NO" >> "$M"; fi
grep -q 'CFE_ES_Main entering OPERATIONAL state' "$S/rx/console_output.log" && echo "operational rx yes" >> "$M" || echo "operational rx NO" >> "$M"
for inst in tx rx; do
  p=$(cat "$S/$inst.pid")
  { echo "## $inst pid $p"; grep -E '^(Uid|Gid|Cap(Inh|Prm|Eff|Bnd|Amb)|Cpus_allowed_list|NoNewPrivs):' "/proc/$p/status"
    grep -E 'Max (realtime priority|nice priority|msgqueue size)' "/proc/$p/limits"
    ps -L -o tid,cls,rtprio,ni,comm -p "$p"; } >> "$M" 2>&1
done
H=$S/host; i=0                                                          # CI L193-L197
for inst in tx rx; do
  port=$((1234 + i)); i=$((i + 1)); p=$(cat "$S/$inst.pid")
  owners=$(port_owner "$port" | tr '\n' ' ')
  if echo " $owners" | grep -q " $p "; then
    ( cd "$H" && ./cmdUtil --port=$port --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) >> "$M" 2>&1
    echo "stop $inst: ES restart POWERON sent to UDP $port (owner $owners)" >> "$M"
  else
    echo "stop $inst: UDP $port owner '$owners' is not this run's pid $p; ES command not sent" >> "$M"
  fi
done
sleep 5                                                                 # CI L199-L200
for inst in tx rx; do
  p=$(cat "$S/$inst.pid")
  if kill -0 "$p" 2>/dev/null; then kill -KILL "$p"; echo "end $inst: still alive after the last sleep; SIGKILL" >> "$M"
  else wait "$p" 2>/dev/null; echo "end $inst: exited" >> "$M"; fi
done
echo "sysv segments of $U after the run: $(ipcs -m | awk -v u="$U" '$3 == u' | wc -l)" >> "$M"
cat "$M"

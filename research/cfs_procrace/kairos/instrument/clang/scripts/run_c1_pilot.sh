#!/usr/bin/env bash
# run_c1_pilot.sh -- one pilot boot of the derived C1 build (label: pilot/dev; not a main-experiment result) for the
# kz-instr-clang fitness check.  The start/stop steps are those of kairos/probe/scripts/observe_c1_smoke.sh
# --instances (the W3 build-smoke of plan/C1_build_manifest.md section 7: CI L165-L200), without the F0 observer.
# Differences from that procedure (written to <run>/conditions.txt): no kz observe and no anchors; the files are the
# derived build; the environment of tx and rx adds KZ_RT_MODE=record, KZ_RT_DUMP, KZ_RT_RUN_ID, KZ_RT_INSTANCE
# (tx 1, rx 2), read by the runtime's start-up object; each instance writes its dump at exit.
#   run_c1_pilot.sh <derived workspace (…/post/1df262a)> <run dir> <run id>
set -uo pipefail
WS=$1; RUN=$2; RID=$3
U=ubuntu
[ "$(id -u)" = 0 ] || { echo "must run as root (setpriv to $U, as observe_c1_smoke.sh)" >&2; exit 2; }
[ ! -e "$RUN" ] || { echo "refusing to overwrite $RUN" >&2; exit 2; }
port_owner() { local hex inode f; hex=$(printf '%04X' "$1")
  inode=$(awk -v h=":$hex" 'NR>1 && substr($2, length($2)-4)==h {print $10; exit}' /proc/net/udp)
  [ -z "$inode" ] && return 0
  for f in /proc/[0-9]*/fd/*; do [ "$(readlink "$f" 2>/dev/null)" = "socket:[$inode]" ] && { echo "pid=$(echo "$f" | cut -d/ -f3)"; return 0; }; done; }
if [ -n "$(port_owner 1234)$(port_owner 1235)" ]; then echo "UDP 1234/1235 busy; not starting" >&2; exit 3; fi
mkdir -p "$RUN"; M="$RUN/smoke.meta"
{ echo "label=pilot/dev (kz-instr-clang fitness check; not a main-experiment result)"; echo "kernel=$(uname -r)"; echo "nproc=$(nproc)"
  echo "run_user=$U ($(id $U))"; echo "mqueue.msg_max=$(cat /proc/sys/fs/mqueue/msg_max)"; } > "$M"
( cd "$WS/exe" && tar -cf "$RUN/cfs-bpapp.tar" . )
mkdir "$RUN/inst" && chown $U:$U "$RUN/inst"
( cd "$RUN/inst" && setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups -- tar -xf ../cfs-bpapp.tar && \
  setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups -- mkdir -p ./tx/storage ./rx/storage )
rm -f "$RUN/cfs-bpapp.tar"
for f in tx/core-tx tx/cf/cf.so tx/cf/bp.so rx/core-rx; do echo "file $f sha256=$(sha256sum "$RUN/inst/$f" | cut -c1-64)" >> "$M"; done
I="$RUN/inst"; AS=(setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups --)
echo "instances.start_utc=$(date -u +%FT%T.%NZ)" >> "$M"
( cd "$I/tx" && exec env KZ_RT_MODE=record KZ_RT_DUMP="$I/tx/kz_rt.dump" KZ_RT_RUN_ID="$RID" KZ_RT_INSTANCE=1 "${AS[@]}" ./core-tx > console_output.log 2>&1 < /dev/null ) & TX=$!
( cd "$I/rx" && exec env KZ_RT_MODE=record KZ_RT_DUMP="$I/rx/kz_rt.dump" KZ_RT_RUN_ID="$RID" KZ_RT_INSTANCE=2 "${AS[@]}" ./core-rx > console_output.log 2>&1 < /dev/null ) & RX=$!
echo "tx_pid=$TX rx_pid=$RX" >> "$M"
sleep 5   # CI L176 (reference value)
for n in tx rx; do grep -q 'CFE_ES_Main entering OPERATIONAL state' "$I/$n/console_output.log" && echo "$n.operational_at_5s=yes" >> "$M" || echo "$n.operational_at_5s=no" >> "$M"; done
for p in $TX $RX; do { echo "## threads of $p ($(readlink /proc/$p/exe))"; ps -L -o tid,cls,rtprio,ni,pri,psr,comm -p $p; } >> "$M" 2>&1; done
own1234=$(port_owner 1234); own1235=$(port_owner 1235); echo "udp1234_owner=$own1234 udp1235_owner=$own1235" >> "$M"
if [ "$own1234" = "pid=$TX" ] && [ "$own1235" = "pid=$RX" ]; then
  ( cd "$I/host" && ./cmdUtil --port=1234 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002; ./cmdUtil --port=1235 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) >> "$M" 2>&1
  echo "stop=ci-es-restart-poweron (CI L196-L197)" >> "$M"
else echo "stop=skipped-port-owner-mismatch 1234:$own1234 1235:$own1235" >> "$M"; fi
sleep 5   # CI L200 (reference value)
for p in $TX $RX; do if kill -0 $p 2>/dev/null; then echo "pid $p alive after 5 s -> SIGKILL" >> "$M"; kill -KILL $p; fi; done
wait $TX; TXE=$?; wait $RX; RXE=$?
echo "tx_exit=$TXE rx_exit=$RXE" >> "$M"
echo "instances.end_utc=$(date -u +%FT%T.%NZ)" >> "$M"
for n in tx rx; do [ -f "$I/$n/kz_rt.dump" ] && echo "$n.dump=yes bytes=$(stat -c %s "$I/$n/kz_rt.dump")" >> "$M" || echo "$n.dump=no" >> "$M"; done
cat > "$RUN/conditions.txt" <<EOT
# conditions of this pilot run (CONDITIONS_POLICY: values from the reference procedure or recorded as differences)
cfs.build      derived build $WS (kz-instr-clang; differences in the build record build_diff.txt)       deviation (instrumented, derived configuration)
cfs.start_stop CI L165-L200 as observe_c1_smoke.sh --instances: tx and rx in the background, sleep 5, OPERATIONAL check, ES restart POWERON to UDP 1234/1235, sleep 5   from-reference
cfs.user       $U (setpriv; cFS README L107)                                                        from-reference
observer       none (the F0 observer is not attached); the in-process runtime records                 deviation
environment    + KZ_RT_MODE=record KZ_RT_DUMP=<instance dir>/kz_rt.dump KZ_RT_RUN_ID=$RID KZ_RT_INSTANCE=1|2   deviation (read by kz_rt_posix_auto.c)
rx             runs the derived build too (its own files) and records with instance 2                deviation (in F0 runs rx was unprobed)
run.count      this run alone (pilot/dev)                                                             unspecified_by_reference
run.stratum    not established (files read by tar and sha256sum before the run)                       deviation
EOT
exit 0

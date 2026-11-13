#!/usr/bin/env bash
# run_c1.sh <exe dir> <out dir> <kz-rt-dump> <kz-events> [<binding file>] -- ONE pilot boot of a clang-built C1 tree (pilot/dev,
# not a main-experiment result).  The start/stop steps are those of kairos/probe/scripts/observe_c1_smoke.sh
# --instances (c1_build/smoke_one.sh: nasa/bp b051eb0 build-run-bp-common.yml L165-L200) without the F0
# observer: tar/untar of the installed tree, tx and rx in the background as the existing non-root account
# ubuntu (cFS README L107), sleep 5 (CI L176), OPERATIONAL check, ES restart POWERON to UDP 1234/1235
# (CI L196-L197), sleep 5 (CI L200), SIGKILL of a survivor.  Must run as root (setpriv), as the smoke script.
# Differences from the F0 smoke3 run, written into <out>/conditions.txt:
#   - no kz observe (no uprobes, no tracefs, the instances are not children of an observer)
#   - the binaries are the clang build of build_c1.sh (L-I with the plugin, or L-A without; without a binding file
#     the dump step is skipped: L-A has no runtime)
#   - tx environment += KZ_RT_DUMP, KZ_RT_RUN_ID, KZ_RT_INSTANCE (the runtime's start object reads them);
#     rx environment += KZ_RT_MODE=off (its hooks return at once: rx is not observed, as in F0 where only
#     tx was probed; P7)
set -uo pipefail
EXE=${1:?usage}; OUT=${2:?usage}; RTDUMP=${3:?kz-rt-dump}; EVENTS=${4:?kz-events}; B=${5:-}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITES="$(cd "$HERE/../../gcc/sites" && pwd)/c1_post_1df262a.kzs"
U=ubuntu
[ "$(id -u)" = 0 ] || { echo "must run as root (setpriv)" >&2; exit 2; }
[ ! -e "$OUT" ] || { echo "refusing to overwrite $OUT" >&2; exit 2; }
port_owner() { # UDP port -> pid via /proc/net/udp inode and /proc/*/fd (as observe_c1_smoke.sh)
  local hex inode f; hex=$(printf '%04X' "$1")
  inode=$(awk -v h=":$hex" 'NR>1 && substr($2, length($2)-4)==h {print $10; exit}' /proc/net/udp)
  [ -z "$inode" ] && return 0
  for f in /proc/[0-9]*/fd/*; do [ "$(readlink "$f" 2>/dev/null)" = "socket:[$inode]" ] && { echo "pid=$(echo "$f" | cut -d/ -f3)"; return 0; }; done
}
if [ -n "$(port_owner 1234)$(port_owner 1235)" ] || pgrep -x 'core-(tx|rx|cpu1)' > /dev/null; then echo "another cFS instance or UDP 1234/1235 busy; not starting" >&2; exit 3; fi
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
M="$OUT/run.meta"
( cd "$EXE" && tar -cf "$OUT/cfs-bpapp.tar" . )
mkdir "$OUT/inst" "$OUT/dump" && chown $U:$U "$OUT/inst" "$OUT/dump"
AS=(setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups --)
( cd "$OUT/inst" && "${AS[@]}" tar -xf ../cfs-bpapp.tar && "${AS[@]}" mkdir -p ./tx/storage ./rx/storage )
rm -f "$OUT/cfs-bpapp.tar"
{ echo "label=pilot/dev boot of the clang-built C1 tree $EXE (LLVM pass-plugin candidate) (stage C fitness check; not a main-experiment result)"
  echo "kernel=$(uname -r) nproc=$(nproc) run_user=$U ($(id $U)) mqueue.msg_max=$(cat /proc/sys/fs/mqueue/msg_max)"
  for f in tx/core-tx tx/cf/cf.so tx/cf/bp.so rx/core-rx; do echo "file $f sha256=$(sha256sum "$OUT/inst/$f" | cut -c1-64)"; done
  echo "manifest $SITES sha256=$(sha256sum "$SITES" | cut -c1-64)"
  echo "environment names of this script (values not recorded): $(env | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort | paste -sd' ')"
  echo "instances.start_utc=$(date -u +%FT%T.%NZ)"; } > "$M"
( cd "$OUT/inst/tx" && exec env KZ_RT_DUMP="$OUT/dump/tx.kzrt" KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=1 "${AS[@]}" ./core-tx > console_output.log 2>&1 < /dev/null ) & TX=$!
( cd "$OUT/inst/rx" && exec env KZ_RT_MODE=off KZ_RT_INSTANCE=2 "${AS[@]}" ./core-rx > console_output.log 2>&1 < /dev/null ) & RX=$!
echo "tx_pid=$TX rx_pid=$RX" >> "$M"
sleep 5   # CI L176 (reference value)
for n in tx rx; do grep -q 'CFE_ES_Main entering OPERATIONAL state' "$OUT/inst/$n/console_output.log" && echo "$n.operational_at_5s=yes" >> "$M" || echo "$n.operational_at_5s=no" >> "$M"; done
for p in $TX $RX; do { echo "## threads of $p ($(readlink /proc/$p/exe))"; ps -L -o tid,cls,rtprio,ni,pri,psr,comm -p $p; } >> "$M" 2>&1; done
own1234=$(port_owner 1234); own1235=$(port_owner 1235); echo "udp1234_owner=$own1234 udp1235_owner=$own1235" >> "$M"
if [ "$own1234" = "pid=$TX" ] && [ "$own1235" = "pid=$RX" ]; then
  ( cd "$OUT/inst/host" && ./cmdUtil --port=1234 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002; ./cmdUtil --port=1235 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) >> "$M" 2>&1
  echo "stop=ci-es-restart-poweron (CI L196-L197)" >> "$M"
else echo "stop=skipped-port-owner-mismatch 1234:$own1234 1235:$own1235" >> "$M"; fi
sleep 5   # CI L200 (reference value)
for p in $TX $RX; do if kill -0 $p 2>/dev/null; then echo "pid $p alive after 5 s -> SIGKILL" >> "$M"; kill -KILL $p; fi; done
wait $TX; TXE=$?; wait $RX; RXE=$?
echo "tx_exit=$TXE rx_exit=$RXE instances.end_utc=$(date -u +%FT%T.%NZ)" >> "$M"
# the dump (written by the runtime's start object at exit) and its check against the manifests
[ -n "$B" ] || { echo "no binding file: dump step skipped (L-A)" >> "$M"; ls -la "$OUT/dump" >> "$M"; grep -E "operational|stop=|exit" "$M"; exit 0; }
if [ -s "$OUT/dump/tx.kzrt" ]; then
  echo "dump tx.kzrt bytes=$(stat -c %s "$OUT/dump/tx.kzrt") sha256=$(sha256sum "$OUT/dump/tx.kzrt" | cut -c1-64)" >> "$M"
  "$RTDUMP" --manifest "$SITES" --manifest "$B" "$OUT/dump/tx.kzrt" > "$OUT/tx.dump.txt" 2>&1; echo "kz-rt-dump exit=$?" >> "$M"
  "$EVENTS" "$OUT/dump/tx.kzrt" > "$OUT/tx.events.tsv" 2>&1; echo "kz-events exit=$?" >> "$M"
else echo "dump tx.kzrt missing" >> "$M"; fi
ls -la "$OUT/dump" >> "$M"
cat "$M" | grep -v '^ ' | grep -E 'operational|stop=|exit|dump'

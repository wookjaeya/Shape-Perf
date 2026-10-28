#!/usr/bin/env bash
# run_gi_c1.sh <W> <CFG pre|post> <CAND 1df262a|1b0b338> <INST tx|rx> <out dir> <kz-rt-dump>
# ONE pilot/dev boot of a stage D derived (kzgcc) C1 build in group G-I (KZ_RT_MODE=record: recording on, no gate;
# SPEC rev 1.4 8.6, docs/event_semantics.md 8.2).  Not a main-experiment result.  Must run as root (setpriv), as the F0
# scripts.  One record per instance (P7): the instance named by INST records, and the start/stop steps are those of the F0
# record this boot is compared with:
#   tx  the two-instance steps of kairos/probe/scripts/observe_c1_smoke.sh --instances (F0 smoke3; c1_build/smoke_one.sh,
#       nasa/bp b051eb0 build-run-bp-common.yml L165-L200): tar/untar of the installed tree, tx and rx in the background as
#       ubuntu (cFS README L107), sleep 5 (CI L176), OPERATIONAL check, ES restart POWERON to UDP 1234/1235 (CI L196-L197),
#       sleep 5 (CI L200), SIGKILL of a survivor.  rx runs with KZ_RT_MODE=off (F0 smoke3 did not probe rx).
#   rx  the one-instance steps of kairos/probe/scripts/observe_c1_hold.sh --one --no-hold (F0 confirm1 N_<cfg>_<cand>_rx):
#       tar/untar of exe/rx and exe/host, rx alone, sleep 8, OPERATIONAL check, ES restart POWERON to UDP 1235, sleep 5,
#       SIGKILL of a survivor.
# Start state: the reference procedure itself fixes it for C1 (a fresh untar of the installed tree into a new directory and
# empty storage directories).  It is recorded with env/tools/persist_snapshot.sh on each instance directory before the start
# and after the stop (read only; an added read of the pre-run window, not in the F0 scripts).
# Differences from the F0 record, written into <out>/conditions.txt: no kz observe (no uprobes, no tracefs; the instances
# are not children of an observer); the binaries are the derived build (derive_c1.sh instr); the recording instance's
# environment += KZ_RT_MODE=record KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE, the other's += KZ_RT_MODE=off KZ_RT_INSTANCE.
set -uo pipefail
W=${1:?usage}; CFG=${2:?cfg}; CAND=${3:?cand}; INST=${4:?inst}; OUT=${5:?out}; RTDUMP=${6:?kz-rt-dump}
case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "cfg pre|post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a|1b0b338) ;; *) echo "cand 1df262a|1b0b338" >&2; exit 64 ;; esac
case "$INST" in tx|rx) ;; *) echo "inst tx|rx" >&2; exit 64 ;; esac
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
SITES="$KZ/profiles/c1-bp-b051eb0/instr/sites-cf$REV.kzs"
SNAP="$(cd "$KZ/../env/tools" && pwd)/persist_snapshot.sh"
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
M="$OUT/run.meta"; C="$OUT/conditions.txt"
if [ "$INST" = tx ]; then ( cd "$W/ws/exe" && tar -cf "$OUT/cfs.tar" . ); INSTS="tx rx"; else ( cd "$W/ws/exe" && tar -cf "$OUT/cfs.tar" rx host ); INSTS="rx"; fi
mkdir "$OUT/inst" "$OUT/dump" && chown $U:$U "$OUT/inst" "$OUT/dump"
AS=(setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups --)
( cd "$OUT/inst" && "${AS[@]}" tar -xf ../cfs.tar && for n in $INSTS; do "${AS[@]}" mkdir -p "./$n/storage"; done )
rm -f "$OUT/cfs.tar"
{ echo "label=pilot/dev boot of the derived (kzgcc) C1 build $W ($CFG/$CAND), group G-I (record), instance $INST recorded (not a main-experiment result)"
  echo "kernel=$(uname -r) nproc=$(nproc) run_user=$U ($(id $U)) mqueue.msg_max=$(cat /proc/sys/fs/mqueue/msg_max)"
  for n in $INSTS; do for f in $n/core-$n $n/cf/cf.so $n/cf/bp.so; do echo "file $f sha256=$(sha256sum "$OUT/inst/$f" | cut -c1-64) (build $(sha256sum "$W/ws/exe/$f" | cut -c1-64))"; done; done
  echo "manifest $SITES sha256=$(sha256sum "$SITES" | cut -c1-64)"
  echo "bindings $W/binds_instr.kzs sha256=$(sha256sum "$W/binds_instr.kzs" | cut -c1-64)"
  echo "environment names of this script (values not recorded): $(env | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort | paste -sd' ')"; } > "$M"
TB=$'\t'
{ echo "# conditions of the G-I boot $CFG/$CAND/$INST (pilot/dev; derived configuration of c1_build/$CFG/$CAND)"
  echo "item${TB}run.group${TB}G-I: KZ_RT_MODE=record for $INST (recording on, no gate, no plan)${TB}unset"
  echo "item${TB}run.count${TB}this boot alone; repetition counts are the PI's (P8, D14)${TB}unspecified_by_reference"
  echo "item${TB}cfs.build${TB}$W (derive_c1.sh instr; differences D1-D4 in $W/build_instr.record)${TB}deviation"
  echo "item${TB}cfs.user${TB}$U (setpriv; cFS README L107)${TB}from-reference"
  if [ "$INST" = tx ]; then
    echo "item${TB}cfs.start_stop${TB}observe_c1_smoke.sh --instances: tx and rx in the background, sleep 5, OPERATIONAL check, ES restart POWERON to UDP 1234/1235, sleep 5 (CI L165-L200)${TB}from-reference"
    echo "deviation${TB}runtime.mode.rx${TB}F0 smoke3 ran rx from unprobed reference files${TB}rx runs the derived rx binary with KZ_RT_MODE=off (G-I0)${TB}one binary per instance; the hooks of rx return at once${TB}rx code layout is the derived one; rx not recorded${TB}yes"
  else
    echo "item${TB}cfs.start_stop${TB}observe_c1_hold.sh --one (no hold): rx alone, sleep 8, OPERATIONAL check, ES restart POWERON to UDP 1235, sleep 5${TB}from-reference"
    echo "deviation${TB}observer.instances${TB}the W3 smoke started tx and rx together${TB}rx alone, as the F0 natural rx records of confirm1${TB}P7 separate records; same steps as its F0 counterpart${TB}tx not run in this record${TB}yes"
  fi
  echo "deviation${TB}observer.method${TB}F0: kz observe (perf-sample uprobes)${TB}no observer; compiler-inserted hooks (kzgcc) and the kzrt runtime record into a static pool, dumped at exit${TB}stage D G-I boot${TB}hook code in the instrumented units; runtime bss 4 MiB; not measured${TB}no"
  echo "deviation${TB}cfs.environment${TB}the F0 instance environment${TB}+= KZ_RT_MODE KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE (read by the runtime start object; cFS does not read them)${TB}the runtime is configured from the environment${TB}none expected; not verified${TB}no"
  echo "item${TB}start_state${TB}fresh untar of the installed tree into a new directory, empty storage (the reference procedure); persist_snapshot.sh before/after${TB}from-reference"
  echo "deviation${TB}procedure.pre_run_reads${TB}F0: tar, untar, kz anchor, start${TB}tar, untar, sha256sum, persist_snapshot.sh (read only), start${TB}record S0${TB}page cache warm; not measured${TB}yes"
} > "$C"
for n in $INSTS; do "$SNAP" "before G-I boot ($n)" "$OUT/inst/$n" > "$OUT/persist_before_$n.txt" 2>&1; done
echo "instances.start_utc=$(date -u +%FT%T.%NZ)" >> "$M"
if [ "$INST" = tx ]; then
  ( cd "$OUT/inst/tx" && exec env KZ_RT_MODE=record KZ_RT_DUMP="$OUT/dump/tx.kzrt" KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=1 "${AS[@]}" ./core-tx > console_output.log 2>&1 < /dev/null ) & TX=$!
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
  echo "tx_exit=$TXE rx_exit=$RXE" >> "$M"
else
  ( cd "$OUT/inst/rx" && exec env KZ_RT_MODE=record KZ_RT_DUMP="$OUT/dump/rx.kzrt" KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=2 "${AS[@]}" ./core-rx > console_output.log 2>&1 < /dev/null ) & P=$!
  echo "rx_pid=$P" >> "$M"
  sleep 8   # observe_c1_hold.sh --one (the F0 natural rx records)
  grep -q 'CFE_ES_Main entering OPERATIONAL state' "$OUT/inst/rx/console_output.log" && echo "rx.operational=yes" >> "$M" || echo "rx.operational=no" >> "$M"
  ps -L -o tid,cls,rtprio,ni,pri,psr,comm -p $P >> "$M" 2>&1 || true
  own=$(port_owner 1235); echo "udp1235_owner=$own" >> "$M"
  if [ "$own" = "pid=$P" ]; then
    ( cd "$OUT/inst/host" && ./cmdUtil --port=1235 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) >> "$M" 2>&1
    echo "stop=ci-es-restart-poweron (CI L196-L197)" >> "$M"
  else echo "stop=skipped-port-owner-mismatch 1235:$own" >> "$M"; fi
  sleep 5
  if kill -0 $P 2>/dev/null; then echo "pid $P alive after 5 s -> SIGKILL" >> "$M"; kill -KILL $P; fi
  wait $P; E=$?; echo "rx_exit=$E" >> "$M"
fi
echo "instances.end_utc=$(date -u +%FT%T.%NZ)" >> "$M"
for n in $INSTS; do "$SNAP" "after G-I boot ($n)" "$OUT/inst/$n" > "$OUT/persist_after_$n.txt" 2>&1; done
grep -n -iE 'failed to get sem id|CF Initialized|Created semaphore|Found semaphore|terminating|CF:' "$OUT/inst/$INST/console_output.log" > "$OUT/cf_console.txt" 2>/dev/null || true
D="$OUT/dump/$INST.kzrt"
if [ -s "$D" ]; then
  echo "dump $INST.kzrt bytes=$(stat -c %s "$D") sha256=$(sha256sum "$D" | cut -c1-64)" >> "$M"
  "$RTDUMP" --manifest "$SITES" --manifest "$W/binds_instr.kzs" "$D" > "$OUT/$INST.dump.txt" 2>&1; echo "kz-rt-dump exit=$?" >> "$M"
else echo "dump $INST.kzrt missing" >> "$M"; fi
grep -v '^ ' "$M" | grep -E 'operational|stop=|exit|dump|alive'

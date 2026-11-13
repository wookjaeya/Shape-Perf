#!/usr/bin/env bash
# run_gih_c1.sh <W> <CFG pre|post> <CAND 1df262a|1b0b338> <INST tx|rx> <plan.kzh> <label> <tag> <out root> <kz> <kz-rt-dump>
# run_gih_c1.sh --one <run dir> <INST> <meta> <instance id>          (internal: start one instance, wait, stop)
# ONE controlled pilot/dev boot of a stage F derived (kzgcc) C1 build in group G-I-H (the same binary as G-I0/G-I, with
# KZ_RT_MODE=control and the plan given through KZ_RT_PLAN; SPEC rev 1.4 4.13 cooperative gate, 8.6; event_semantics.md
# sections 4, 5, 11).  Not a main-experiment result.  Must run as root (setpriv), as the F0 scripts.
# The plan is a kzp-hold 1 file of confirm2 (predict1/predict2 .kzh, F0 site labels); `kz rtplan` converts it for this build
# (W's manifest and binding list) into <run>/plan.kzrtp before the start; a plan it refuses is not run.
# Start/stop: the one-instance steps of the F0 hold runs (kairos/probe/scripts/observe_c1_hold.sh --one, confirm2): tar/untar
# of exe/<inst> and exe/host, the instance alone as ubuntu, sleep 8, OPERATIONAL check, ES restart POWERON to its UDP port,
# sleep 5, SIGKILL of a survivor; the whole --one wrapped in `timeout 150` (the pilot safeguard of the F0 hold runs,
# researcher choice; a run it ends is INCONCLUSIVE).  After the run: the normalization check of confirm2/scripts/run_confirm.sh
# (POWERON exit line in the console, no SysV segment of ubuntu left); a failure is recorded (normalize.txt) and the caller
# makes the normalization boot.  S0 recorded with env/tools/persist_snapshot.sh before and after (read only).
# Differences from the F0 record (conditions.txt): no kz hold (no ptrace, no debug registers), no kz observe; the control is
# the runtime's cooperative gate at the compiler-inserted control hooks; the binaries are the derived build; the instance's
# environment += KZ_RT_MODE=control KZ_RT_PLAN KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE.
set -uo pipefail
U=ubuntu
SAFEGUARD_S=150   # the F0 hold runs' pilot safeguard (observe_c1_hold.sh SAFEGUARD_S, researcher choice)
port_owner() { local hex inode f; hex=$(printf '%04X' "$1")
  inode=$(awk -v h=":$hex" 'NR>1 && substr($2, length($2)-4)==h {print $10; exit}' /proc/net/udp)
  [ -z "$inode" ] && return 0
  for f in /proc/[0-9]*/fd/*; do [ "$(readlink "$f" 2>/dev/null)" = "socket:[$inode]" ] && { echo "pid=$(echo "$f" | cut -d/ -f3)"; return 0; }; done
}
if [ "${1:-}" = "--one" ]; then
  RUN="$2"; INST="$3"; M="$4"; IID="$5"
  case "$INST" in tx) PORT=1234 ;; rx) PORT=1235 ;; *) echo "bad inst" >&2; exit 64 ;; esac
  AS=(setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups --)
  echo "instance.start_utc=$(date -u +%FT%T.%NZ)" >> "$M"
  ( cd "$RUN/inst/$INST" && exec env KZ_RT_MODE=control KZ_RT_PLAN="$RUN/plan.kzrtp" KZ_RT_DUMP="$RUN/dump/$INST.kzrt" KZ_RT_RUN_ID=1 \
      KZ_RT_INSTANCE="$IID" "${AS[@]}" ./core-$INST > console_output.log 2>&1 < /dev/null ) & P=$!
  echo "${INST}_pid=$P" >> "$M"
  sleep 8   # observe_c1_hold.sh --one
  grep -q 'CFE_ES_Main entering OPERATIONAL state' "$RUN/inst/$INST/console_output.log" && echo "$INST.operational=yes" >> "$M" || echo "$INST.operational=no" >> "$M"
  ps -L -o tid,cls,rtprio,ni,pri,psr,comm -p $P >> "$M" 2>&1 || true
  own=$(port_owner $PORT); echo "udp${PORT}_owner=$own" >> "$M"
  if [ "$own" = "pid=$P" ]; then
    ( cd "$RUN/inst/host" && ./cmdUtil --port=$PORT --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) >> "$M" 2>&1
    echo "stop=ci-es-restart-poweron (CI L196-L197)" >> "$M"
  else echo "stop=skipped-port-owner-mismatch $PORT:$own" >> "$M"; fi
  sleep 5
  if kill -0 $P 2>/dev/null; then echo "pid $P alive after 5 s -> SIGKILL" >> "$M"; kill -KILL $P; fi
  wait $P; E=$?; echo "${INST}_exit=$E" >> "$M"
  echo "instance.end_utc=$(date -u +%FT%T.%NZ)" >> "$M"
  exit 0
fi
W=${1:?usage}; CFG=${2:?cfg}; CAND=${3:?cand}; INST=${4:?inst}; KZH=${5:?plan}; LABEL=${6:?label}; TAG=${7:?tag}; ROOT=${8:?out root}
KZB=${9:?kz}; RTDUMP=${10:?kz-rt-dump}
case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "cfg pre|post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a|1b0b338) ;; *) echo "cand 1df262a|1b0b338" >&2; exit 64 ;; esac
case "$INST" in tx) PORT=1234; IID=1 ;; rx) PORT=1235; IID=2 ;; *) echo "inst tx|rx" >&2; exit 64 ;; esac
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; SELF="$HERE/$(basename "${BASH_SOURCE[0]}")"
KZ="$(cd "$HERE/../../.." && pwd)"
SITES="$KZ/profiles/c1-bp-b051eb0/instr/sites-cf$REV.kzs"
SNAP="$(cd "$KZ/../env/tools" && pwd)/persist_snapshot.sh"
[ "$(id -u)" = 0 ] || { echo "must run as root (setpriv)" >&2; exit 2; }
mkdir -p "$ROOT"; ROOT="$(cd "$ROOT" && pwd)"
RUN="$ROOT/${TAG}_${CFG}_${CAND}_${INST}"
[ ! -e "$RUN" ] || { echo "refusing to overwrite $RUN" >&2; exit 2; }
if [ -n "$(port_owner 1234)$(port_owner 1235)" ] || pgrep -x 'core-(tx|rx|cpu1)' > /dev/null; then echo "another cFS instance or UDP 1234/1235 busy; not starting" >&2; exit 3; fi
mkdir -p "$RUN"
M="$RUN/run.meta"; C="$RUN/conditions.txt"
"$KZB" rtplan "$KZH" --manifest "$SITES" --binds "$W/binds_instr.kzs" --out "$RUN/plan.kzrtp" --record "$RUN/rtplan.txt" > /dev/null 2> "$RUN/rtplan.err" ||
  { echo "kz rtplan refused the plan: $(cat "$RUN/rtplan.err")" | tee "$RUN/refused.txt"; exit 4; }
cp -p "$KZH" "$RUN/plan.kzh"
( cd "$W/ws/exe" && tar -cf "$RUN/cfs.tar" "$INST" host )
mkdir "$RUN/inst" "$RUN/dump" && chown $U:$U "$RUN/inst" "$RUN/dump"
AS=(setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups --)
( cd "$RUN/inst" && "${AS[@]}" tar -xf ../cfs.tar && "${AS[@]}" mkdir -p "./$INST/storage" )
rm -f "$RUN/cfs.tar"
{ echo "label=controlled ($LABEL): pilot/dev boot of the stage F derived (kzgcc) C1 build $CFG/$CAND, group G-I-H, instance $INST (not a main-experiment result)"
  echo "kernel=$(uname -r) nproc=$(nproc) run_user=$U ($(id $U)) mqueue.msg_max=$(cat /proc/sys/fs/mqueue/msg_max)"
  for f in core-$INST cf/cf.so cf/bp.so; do echo "file $INST/$f sha256=$(sha256sum "$RUN/inst/$INST/$f" | cut -c1-64) (build $(sha256sum "$W/ws/exe/$INST/$f" | cut -c1-64))"; done
  echo "manifest $SITES sha256=$(sha256sum "$SITES" | cut -c1-64)"
  echo "bindings $W/binds_instr.kzs sha256=$(sha256sum "$W/binds_instr.kzs" | cut -c1-64)"
  echo "plan $KZH sha256=$(sha256sum "$KZH" | cut -c1-64) -> plan.kzrtp sha256=$(sha256sum "$RUN/plan.kzrtp" | cut -c1-64)"
  echo "kz $KZB sha256=$(sha256sum "$KZB" | cut -c1-64)"
  echo "environment names of this script (values not recorded): $(env | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort | paste -sd' ')"; } > "$M"
TB=$'\t'
{ echo "# conditions of the G-I-H boot $CFG/$CAND/$INST (controlled, $LABEL; pilot/dev; derived configuration of c1_build/$CFG/$CAND)"
  echo "item${TB}run.group${TB}G-I-H: KZ_RT_MODE=control, KZ_RT_PLAN=plan.kzrtp (kz rtplan of $(basename "$KZH"))${TB}unset"
  echo "item${TB}run.label${TB}controlled ($LABEL)${TB}from-reference"
  echo "item${TB}run.count${TB}this boot alone; repetition counts are the PI's (P8, D14)${TB}unspecified_by_reference"
  echo "item${TB}cfs.build${TB}$W (derive_c1.sh instr; differences D1-D4 in $W/build_instr.record)${TB}deviation"
  echo "item${TB}cfs.user${TB}$U (setpriv; cFS README L107)${TB}from-reference"
  echo "item${TB}cfs.start_stop${TB}observe_c1_hold.sh --one: $INST alone, sleep 8, OPERATIONAL check, ES restart POWERON to UDP $PORT, sleep 5${TB}from-reference"
  echo "item${TB}hold.safeguard${TB}--one wrapped in timeout ${SAFEGUARD_S}s (the F0 hold runs' pilot safeguard, researcher choice); a run it ends is INCONCLUSIVE${TB}deviation"
  echo "item${TB}hold.timeout_ns${TB}0 for every gate (no runtime timeout; the F0 holder had none)${TB}unspecified_by_reference"
  echo "deviation${TB}control.method${TB}F0 confirm2: kz hold (ptrace, debug-register breakpoints) around kz observe${TB}the runtime's cooperative gate: kz_rt_ctl at the manifest's control bindings, POSIX adapter (private pthread mutex and condition variables), plan from KZ_RT_PLAN${TB}stage F (IR 15 F)${TB}no ptrace stops; hook code in the instrumented units; runtime bss; not measured${TB}yes"
  echo "deviation${TB}observer.method${TB}F0: kz observe (perf-sample uprobes)${TB}compiler-inserted hooks (kzgcc) recording into the runtime's static pool, dumped at exit${TB}stage F${TB}as G-I${TB}no"
  echo "deviation${TB}cfs.environment${TB}the F0 instance environment${TB}+= KZ_RT_MODE KZ_RT_PLAN KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE (read by the runtime start object; cFS does not read them)${TB}the runtime is configured from the environment${TB}none expected; not verified${TB}no"
  echo "item${TB}start_state${TB}fresh untar of the installed tree into a new directory, empty storage (the reference procedure); persist_snapshot.sh before/after${TB}from-reference"
  echo "deviation${TB}procedure.pre_run_reads${TB}F0: tar, untar, kz anchor, start${TB}kz rtplan, tar, untar, sha256sum, persist_snapshot.sh (read only), start${TB}record S0${TB}page cache warm; not measured${TB}yes"
} > "$C"
"$SNAP" "before G-I-H boot ($INST)" "$RUN/inst/$INST" > "$RUN/persist_before_$INST.txt" 2>&1
timeout "$SAFEGUARD_S" "$SELF" --one "$RUN" "$INST" "$M" "$IID"; RC=$?
echo "timeout+one exit=$RC (124 = pilot safeguard fired -> INCONCLUSIVE)" >> "$M"
"$SNAP" "after G-I-H boot ($INST)" "$RUN/inst/$INST" > "$RUN/persist_after_$INST.txt" 2>&1
con="$RUN/inst/$INST/console_output.log"; ok=1
grep -q 'Exiting cFE with POWERON Reset status' "$con" 2>/dev/null || ok=0
[ -z "$(ipcs -m | awk -v u=$U '$3==u')" ] || ok=0
[ $ok = 1 ] && echo "normal end (POWERON exit, no SysV segment left)" > "$RUN/normalize.txt" || echo "ABNORMAL END: normalization boot needed" > "$RUN/normalize.txt"
grep -n -iE 'failed to get sem id|CF Initialized|Created semaphore|Found semaphore|terminating|CF:' "$con" > "$RUN/cf_console.txt" 2>/dev/null || true
D="$RUN/dump/$INST.kzrt"
if [ -s "$D" ]; then
  echo "dump $INST.kzrt bytes=$(stat -c %s "$D") sha256=$(sha256sum "$D" | cut -c1-64)" >> "$M"
  "$RTDUMP" --manifest "$SITES" --manifest "$W/binds_instr.kzs" --plan "$RUN/plan.kzrtp" "$D" > "$RUN/$INST.dump.txt" 2>&1; echo "kz-rt-dump exit=$?" >> "$M"
else echo "dump $INST.kzrt missing" >> "$M"; fi
{ cat "$RUN/normalize.txt"; grep -v '^ ' "$M" | grep -E 'operational|stop=|exit|dump|alive'; } | tr '\n' ' '; echo

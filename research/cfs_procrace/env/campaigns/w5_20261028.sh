#!/usr/bin/env bash
# w5_20261028.sh -- the 2026-10-28 functional comparison campaign (PI decision 2026-10-28 Q3, Q4;
# Architecture/spec/PI_DECISIONS_RESOLVED_20261028.md section 5 item 4): fresh F0 and instrumented runs of C1 and R2 in
# the same current environment, to check that execution and the core judgements hold.  Pilot/dev and controlled runs
# only; not main-experiment results.  One boot per configuration (P8: a run is evidence of its path, not of a frequency;
# the repetition count of later quantitative comparisons is set when they are designed, Q4).
#
# What runs, in this order, each through the existing driver of its earlier counterpart (nothing of their conditions is
# changed here):
#   R2  F0   kairos/probe/scripts/observe_cfs_smoke.sh --purpose pilot --start poweron   (counterpart: v153_pilot3)
#       G-I  kairos/instrument/gcc/scripts/run_gi_r2.sh ... poweron on the re-derived stage D build (counterpart: d4 run1)
#       Both with the R2 common Power-On start (plan/r2_start_poweron_20261028.md).
#   C1 natural, per build (pre/post x 1df262a/1b0b338):
#       F0 tx   observe_c1_smoke.sh (counterpart: smoke3)          G-I tx  run_gi_c1.sh tx (counterpart: d3 c1 runs)
#       F0 rx   observe_c1_hold.sh --no-hold (counterpart: confirm1 N_*_rx)   G-I rx  run_gi_c1.sh rx
#   C1 controlled: the nine confirm2 plans x four instances, F0 (observe_c1_hold.sh, as confirm2/scripts/run_confirm.sh)
#       and G-I-H (run_gih_confirm2.sh, as stage F).
# The instrumented C1 runs use the stage F derived builds (instr/gcc/f1/c1/*), one binary for G-I and G-I-H (the mode
# decides the group, SPEC rev 1.4 8.6).  The R2 instrumented build is re-derived from the d4 inputs (the d4 tree was not
# kept): same manifest (sha256 897c7510...), plugin (d2/plugin-build/kzgcc.so) and runtime (d4/rt-r2); its binary hash
# is recorded and compared with d4's (the build date/user/host strings differ, derive_r2.sh D4).
# Harness choices (unspecified_by_reference): the order above, the disk floor, the quiet-machine record.
#
# Usage: w5_20261028.sh <out root> [r2|c1nat|c1ctl]...   (run as root with CFS_NORMAL_USER=ubuntu; nothing else may
#        build or run cFS meanwhile)
set -uo pipefail
OUT=${1:?out root}; shift
PARTS=("$@"); [ ${#PARTS[@]} -gt 0 ] || PARTS=(r2 c1nat c1ctl)
R=/home/user/Shape-Perf/research/cfs_procrace
K=$R/kairos
KZ=/home/user/work/kairos-build/tools/kz/kz
F1=/home/user/work/instr/gcc/f1
WR2=/home/user/work/w5/r2_derived
RTD_R2=/home/user/work/instr/gcc/d4/rt-r2/kz-rt-dump
P1D=/home/user/work/procrace/c1_runs/predict1
P2D=/home/user/work/procrace/c1_runs/predict2
U=${CFS_NORMAL_USER:?set CFS_NORMAL_USER}
[ "$(id -u)" = 0 ] || { echo "must run as root" >&2; exit 2; }
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"; LOG=$OUT/campaign.log
TFS=$OUT/tracefs
want() { local p; for p in "${PARTS[@]}"; do [ "$p" = "$1" ] && return 0; done; return 1; }
quiet() {  # record the machine state before a phase
  { echo "## $1 $(date -u +%FT%T.%3NZ) kernel=$(uname -r)"; uptime; ps -eo pid,pcpu,comm --sort=-pcpu | head -6; df -BM --output=avail /home/user | tail -1; } >> "$LOG"
}
floor() { local df; df=$(df --output=avail -BM /home/user | tail -1 | tr -dc 0-9); [ "$df" -gt 3000 ] || { echo "disk below 3 GB, stop" >> "$LOG"; exit 5; }; }
{ echo "# w5_20261028.sh start $(date -u +%FT%T.%3NZ) parts=${PARTS[*]}"
  echo "repo $(git -C $R rev-parse HEAD) diff_kairos_lines=$(git -C $R diff HEAD -- kairos | wc -l)"
  echo "kz $KZ sha256=$(sha256sum $KZ | cut -c1-64)"
  echo "uname $(uname -a)"; } >> "$LOG"
mkdir -p "$TFS"; mountpoint -q "$TFS" || mount -t tracefs nodev "$TFS"

if want r2; then
  quiet "R2"
  floor
  echo "$(date -u +%T) R2 F0" >> "$LOG"
  mkdir -p "$OUT/r2/f0"
  env -u KZ CFS_NORMAL_USER="$U" "$K/probe/scripts/observe_cfs_smoke.sh" w5_r2_f0 "$KZ" 0 --out-dir "$OUT/r2/f0" --purpose pilot --start poweron >> "$LOG" 2>&1
  echo "  rc=$?" >> "$LOG"
  floor
  echo "$(date -u +%T) R2 G-I" >> "$LOG"
  { echo "derived core-cpu1 sha256=$(sha256sum $WR2/cFS/build-native_std/exe/cpu1/core-cpu1 | cut -c1-64) (d4 run1: 3695a7a1182a3513...)"; } >> "$LOG"
  CFS_NORMAL_USER="$U" "$K/instrument/gcc/scripts/run_gi_r2.sh" "$WR2" "$OUT/r2/gi" "$RTD_R2" poweron >> "$LOG" 2>&1
  echo "  rc=$?" >> "$LOG"
fi

if want c1nat; then
  quiet "C1 natural"
  for CFG in pre post; do for CAND in 1df262a 1b0b338; do
    REV=833fdbb; [ "$CFG" = pre ] && REV=281a941
    floor
    echo "$(date -u +%T) C1 natural $CFG $CAND F0 tx" >> "$LOG"
    "$K/probe/scripts/observe_c1_smoke.sh" $CFG $CAND "$KZ" --out-dir "$OUT/c1nat/f0tx" --tracefs "$TFS" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    echo "$(date -u +%T) C1 natural $CFG $CAND G-I tx" >> "$LOG"
    "$K/instrument/gcc/scripts/run_gi_c1.sh" "$F1/c1/${CFG}_$CAND" $CFG $CAND tx "$OUT/c1nat/gi/${CFG}_${CAND}_tx" "$F1/rt-c1-cf$REV/kz-rt-dump" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    echo "$(date -u +%T) C1 natural $CFG $CAND F0 rx" >> "$LOG"
    "$K/probe/scripts/observe_c1_hold.sh" $CFG $CAND rx "$KZ" --out-dir "$OUT/c1nat/f0rx" --tracefs "$TFS" --no-hold >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    echo "$(date -u +%T) C1 natural $CFG $CAND G-I rx" >> "$LOG"
    "$K/instrument/gcc/scripts/run_gi_c1.sh" "$F1/c1/${CFG}_$CAND" $CFG $CAND rx "$OUT/c1nat/gi/${CFG}_${CAND}_rx" "$F1/rt-c1-cf$REV/kz-rt-dump" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
  done; done
fi

if want c1ctl; then
  quiet "C1 controlled F0"
  # F0: the run_confirm.sh loop (confirm2/scripts/run_confirm.sh one() and normalize()), output under $OUT/c1ctl/f0
  CF=$OUT/c1ctl/f0; mkdir -p "$CF"; CLOG=$CF/run_confirm.log
  PC="P-C1HOLD"; RI="researcher intervention, outside P-C1HOLD"
  normalize() {  # CFG CAND INST RUN
    local CFG=$1 CAND=$2 INST=$3 RUN=$4 port=1234
    [ "$INST" = rx ] && port=1235
    local con="$RUN/inst/$INST/console_output.log" ok=1
    grep -q 'Exiting cFE with POWERON Reset status' "$con" 2>/dev/null || ok=0
    [ -z "$(ipcs -m | awk -v u=$U '$3==u')" ] || ok=0
    if [ $ok = 1 ]; then echo "  normal end (POWERON exit, no SysV segment left)" >> $CLOG; return; fi
    echo "  ABNORMAL END -> one unobserved normalization boot" >> $CLOG
    local N=$CF/normalize/$(basename $RUN); mkdir -p $N; chown $U:$U $N
    ( cd /home/user/work/procrace/c1_build/$CFG/$CAND/exe && tar -cf $N/cfs.tar $INST host )
    ( cd $N && setpriv --reuid=$(id -u $U) --regid=$(id -g $U) --init-groups -- tar -xf cfs.tar && rm -f cfs.tar && \
      setpriv --reuid=$(id -u $U) --regid=$(id -g $U) --init-groups -- mkdir -p $INST/storage )
    ( cd $N/$INST && exec setpriv --reuid=$(id -u $U) --regid=$(id -g $U) --init-groups -- ./core-$INST > console_output.log 2>&1 < /dev/null ) & local P=$!
    sleep 8
    ( cd $N/host && ./cmdUtil --port=$port --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) > $N/stop.txt 2>&1
    sleep 5; kill -0 $P 2>/dev/null && kill -KILL $P; wait $P
    { grep -m1 -E 'Starting the cFE with' $N/$INST/console_output.log; grep -m1 'Exiting cFE with POWERON' $N/$INST/console_output.log; ipcs -m | awk -v u=$U '$3==u'; } >> $CLOG 2>&1
    rm -rf $N/$INST/cf $N/host
  }
  one() {  # CFG CAND INST TAG PLANFILE LABEL
    local CFG=$1 CAND=$2 INST=$3 TAG=$4 PLAN=$5 LABEL=$6
    floor
    echo "$(date -u +%T) $TAG $CFG $CAND $INST plan=${PLAN} label=${LABEL}" >> $CLOG
    "$K/probe/scripts/observe_c1_hold.sh" $CFG $CAND $INST "$KZ" --out-dir $CF --tracefs "$TFS" --plan $PLAN --label "$LABEL" --tag $TAG >> $CLOG 2>&1
    echo "  rc=$?" >> $CLOG
    normalize $CFG $CAND $INST $CF/${TAG}_${CFG}_${CAND}_${INST}
  }
  for CAND in 1df262a 1b0b338; do
    for INST in tx rx; do
      one post $CAND $INST W1  $P1D/post_$CAND/cur/W1.kzh  "$PC"
      one pre  $CAND $INST W2  $P1D/pre_$CAND/cur/W2.kzh   "$RI"
      one post $CAND $INST W10 $P1D/post_$CAND/cur/W10.kzh "$RI"
      for W in W5 W6 W7 W8 W11 W12; do one post $CAND $INST $W $P2D/post_$CAND/cur/$W.kzh "$RI"; done
    done
  done
  echo "$(date -u +%T) F0 controlled done" >> "$LOG"
  quiet "C1 controlled G-I-H"
  "$K/instrument/gcc/scripts/run_gih_confirm2.sh" "$OUT/c1ctl/gih" "$KZ" >> "$LOG" 2>&1
  echo "$(date -u +%T) G-I-H controlled done rc=$?" >> "$LOG"
fi
umount "$TFS" 2>/dev/null
echo "# w5_20261028.sh end $(date -u +%FT%T.%3NZ)" >> "$LOG"

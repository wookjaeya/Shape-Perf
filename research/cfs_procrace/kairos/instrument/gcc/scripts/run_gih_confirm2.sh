#!/usr/bin/env bash
# run_gih_confirm2.sh <out root> <kz> [plan tag]...  -- the G-I-H counterparts of the confirm2 runs (stage F,
# plan/instrumentation_selection_20261026.md 5.4 F5): the same plan files, labels, build x instance order and checks as
# /home/user/work/procrace/c1_runs/confirm2/scripts/run_confirm.sh, each run by run_gih_c1.sh on the stage F derived builds
# /home/user/work/instr/gcc/f1/c1/<cfg>_<cand> (KZ_RT_MODE=control, the plan converted by kz rtplan).  Without plan tags all
# nine (W1 W2 W10 W5 W6 W7 W8 W11 W12); with tags only those (the representative plans first, then the rest).
# After every run the normalization check of confirm2: the console must show the POWERON exit and no SysV segment may be
# left; otherwise one unobserved normalization boot of the reference build is made, as confirm2 did.  Pilot/dev only.
set -uo pipefail
OUT=${1:?out root}; KZB=${2:?kz}; shift 2
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
F1=/home/user/work/instr/gcc/f1
P1D=/home/user/work/procrace/c1_runs/predict1
P2D=/home/user/work/procrace/c1_runs/predict2
U=ubuntu
PC="P-C1HOLD"; RI="researcher intervention, outside P-C1HOLD"
TAGS=("$@"); [ ${#TAGS[@]} -gt 0 ] || TAGS=(W1 W2 W10 W5 W6 W7 W8 W11 W12)
mkdir -p "$OUT"; LOG=$OUT/run_gih.log
want() { local t; for t in "${TAGS[@]}"; do [ "$t" = "$1" ] && return 0; done; return 1; }
normalize() {  # CFG CAND INST RUN  (confirm2/scripts/run_confirm.sh normalize(), on the reference build)
  local CFG=$1 CAND=$2 INST=$3 RUN=$4 port=1234
  [ "$INST" = rx ] && port=1235
  grep -q '^normal end' "$RUN/normalize.txt" 2>/dev/null && { echo "  normal end (POWERON exit, no SysV segment left)" >> $LOG; return; }
  echo "  ABNORMAL END -> one unobserved normalization boot" >> $LOG
  local N=$OUT/normalize/$(basename $RUN); mkdir -p $N; chown $U:$U $N
  ( cd /home/user/work/procrace/c1_build/$CFG/$CAND/exe && tar -cf $N/cfs.tar $INST host )
  ( cd $N && setpriv --reuid=$(id -u $U) --regid=$(id -g $U) --init-groups -- tar -xf cfs.tar && rm -f cfs.tar && \
    setpriv --reuid=$(id -u $U) --regid=$(id -g $U) --init-groups -- mkdir -p $INST/storage )
  ( cd $N/$INST && exec setpriv --reuid=$(id -u $U) --regid=$(id -g $U) --init-groups -- ./core-$INST > console_output.log 2>&1 < /dev/null ) & local P=$!
  sleep 8
  ( cd $N/host && ./cmdUtil --port=$port --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) > $N/stop.txt 2>&1
  sleep 5; kill -0 $P 2>/dev/null && kill -KILL $P; wait $P
  { grep -m1 -E 'Starting the cFE with' $N/$INST/console_output.log; grep -m1 'Exiting cFE with POWERON' $N/$INST/console_output.log; ipcs -m | awk -v u=$U '$3==u'; } >> $LOG 2>&1
  rm -rf $N/$INST/cf $N/host
}
one() {  # CFG CAND INST TAG PLANFILE LABEL
  local CFG=$1 CAND=$2 INST=$3 TAG=$4 PLAN=$5 LABEL=$6 REV=833fdbb
  [ "$CFG" = pre ] && REV=281a941
  local df; df=$(df --output=avail -BM /home/user | tail -1 | tr -dc 0-9); [ "$df" -gt 3500 ] || { echo "disk below 3.5 GB, stop" >> $LOG; exit 5; }
  echo "$(date -u +%T) $TAG $CFG $CAND $INST plan=${PLAN} label=${LABEL}" >> $LOG
  "$HERE/run_gih_c1.sh" $F1/c1/${CFG}_$CAND $CFG $CAND $INST $PLAN "$LABEL" $TAG $OUT $KZB $F1/rt-c1-cf$REV/kz-rt-dump >> $LOG 2>&1
  echo "  rc=$?" >> $LOG
  normalize $CFG $CAND $INST $OUT/${TAG}_${CFG}_${CAND}_${INST}
}
for CAND in 1df262a 1b0b338; do
  for INST in tx rx; do
    want W1 && one post $CAND $INST W1 $P1D/post_$CAND/cur/W1.kzh "$PC"
    want W2 && one pre $CAND $INST W2 $P1D/pre_$CAND/cur/W2.kzh "$RI"
    want W10 && one post $CAND $INST W10 $P1D/post_$CAND/cur/W10.kzh "$RI"
    for W in W5 W6 W7 W8 W11 W12; do
      want $W && one post $CAND $INST $W $P2D/post_$CAND/cur/$W.kzh "$RI"
    done
  done
done
echo "$(date -u +%T) done (${TAGS[*]})" >> $LOG

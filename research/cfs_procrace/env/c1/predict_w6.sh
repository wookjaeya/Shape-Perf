#!/usr/bin/env bash
# predict_w6.sh -- the C1 control plans for the W6 controlled re-verification (2026-10-29), by the recorded procedure of
# kairos/docs/predict.md section 2 on the four W6 F0 natural tx records (env/campaigns/w6_20261029.sh c1nat), with the
# profiles of the current repository (PD29 D8 changed the rspecs, so the closures are extracted again).
# Not a run: no cFS boots.  Plans are what `kz predict` writes; nothing here edits a plan.
#
#   predict_w6.sh closures               the static closures of the four reference builds, tx and rx, by env/c1/kz_run_c1.sh
#                                        unchanged -> /home/user/work/w6/closures/<cfg>_<cand>/c1prof[_rx]/
#   predict_w6.sh replay                 kz replay of each F0 tx record with those closures (current reading)
#                                        -> /home/user/work/w6/predict/<cfg>_<cand>/replay/
#   predict_w6.sh predict                kz predict, current reading -> /home/user/work/w6/predict/<cfg>_<cand>/cur/
#   predict_w6.sh check <cfg> <W>        for both candidates: kz rtplan of cur/<W>.kzh for the f1 derived build (as
#                                        instrument/gcc/scripts/run_gih_c1.sh), and kz hold's own plan reading with the
#                                        record's bases for the F0 path (tx: c1nat/f0tx, rx: c1nat/f0rx), the supervised
#                                        command being /bin/true -> /home/user/work/w6/predict/<cfg>_<cand>/check/<W>/
#
# Fixed by the records:
#   closures   env/c1/kz_run_c1.sh (env/REBUILD_20261029.md section 3.6; plan/results_pilot_20261025.md section 0: tx with
#              closure.kzb, rx with closure_rx.kzb).  kz_run_c1.sh writes into c1_build/kz/<cfg>_<cand>/; so that the
#              rebuild's own closures there stay as they are, the script runs in a private mount namespace in which
#              /home/user/work/w6/closures/<cfg>_<cand> (holding byte copies of closure.kzb, closure_rx.kzb) is bind-mounted
#              on c1_build/kz/<cfg>_<cand>.  Paths seen by kz-static are the recorded ones (harness choice, recorded).
#   replay     the current reading of kairos/probe/scripts/observe_c1_smoke.sh step 5 with --closure (profiles, the two
#              constants, --basis of the run, --facts/--static-basis of the closure, --compare-narrowing)
#   predict    kairos/docs/predict.md section 2 (the same inputs as the replay; models 00_common.kzm + cf-<rev>.kzm;
#              --prov-root c1_runs/prov_roots/<cfg>_<cand>; --observe-profile observe/sites-cf<rev>.kzp; default
#              --max-states, as recorded)
#   rtplan     kz rtplan <plan.kzh> --manifest instr/sites-cf<rev>.kzs --binds <W>/binds_instr.kzs (run_gih_c1.sh L55-L64)
# Harness choices (unspecified_by_reference): the output layout, sha256.txt, the /bin/true command of the hold check.
set -uo pipefail
R=/home/user/Shape-Perf/research/cfs_procrace
K=$R/kairos
P=$K/profiles/c1-bp-b051eb0
KZ=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
C1=/home/user/work/procrace/c1_build
W6=/home/user/work/w6
NAT=$W6/run/c1nat
CL=$W6/closures
PR=$W6/predict
F1=/home/user/work/instr/gcc/f1/c1
PROV=/home/user/work/procrace/c1_runs/prov_roots
CONSTS=(--const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20)
rev() { case "$1" in pre) echo 281a941 ;; post) echo 833fdbb ;; esac; }
prof() { local r; r=$(rev "$1"); echo "--profile $P --profile $P/cf-$r --profile $P/bundle-$2 --profile $P/rspec-$r"; }
inputs() { # cfg cand -> the replay/predict inputs of the tx record
  local RUN=$NAT/f0tx/$1_$2 D=$CL/$1_$2/c1prof
  echo "--basis $RUN/basis_core-tx.kz --facts $D/closure.facts.kz --static-basis $D/closure.basis.kz"
}
step=${1:?closures|replay|predict|check}; shift
[ ! -e /home/user/work/.campaign_running ] || { echo "a campaign is running (/home/user/work/.campaign_running); wait" >&2; exit 75; }
case "$step" in
closures)
  for CFG in pre post; do for CAND in 1df262a 1b0b338; do
    KD=$C1/kz/${CFG}_$CAND; W=$CL/${CFG}_$CAND; mkdir -p "$W"
    for f in closure.kzb closure_rx.kzb; do [ -f "$W/$f" ] || cp -p "$KD/$f" "$W/$f"; cmp -s "$KD/$f" "$W/$f" || { echo "copy differs: $f" >&2; exit 2; }; done
    for INST in tx rx; do
      W_ARG=; [ $INST = rx ] && W_ARG=rx
      unshare -m --propagation private bash -c 'mount --bind "$1" "$2" && exec "$3" "$4" "$5" $6' _ \
        "$W" "$KD" "$R/env/c1/kz_run_c1.sh" $CFG $CAND "$W_ARG" | tee -a "$CL/closures.log"
    done
  done; done
  { echo "# predict_w6.sh closures $(date -u +%FT%T.%3NZ) repo $(git -C $R rev-parse HEAD)"
    for CFG in pre post; do for CAND in 1df262a 1b0b338; do for S in "" _rx; do
      sha256sum $CL/${CFG}_$CAND/c1prof$S/closure.basis.kz $CL/${CFG}_$CAND/c1prof$S/closure.facts.kz; done; done; done
    sha256sum $P/rspec-281a941/40_rspec.kzp $P/rspec-833fdbb/40_rspec.kzp; } > "$CL/sha256.txt"
  ;;
replay)
  for CFG in pre post; do for CAND in 1df262a 1b0b338; do
    O=$PR/${CFG}_$CAND/replay; mkdir -p "$O"; RUN=$NAT/f0tx/${CFG}_$CAND
    "$KZ" replay "$RUN/run.kzt" $(prof $CFG $CAND) "${CONSTS[@]}" $(inputs $CFG $CAND) --compare-narrowing \
        --out "$O/cur.kz" > "$O/cur.txt" 2>&1; echo "replay $CFG $CAND exit=$?" | tee -a "$PR/replay.log"
  done; done
  ;;
predict)
  for CFG in pre post; do for CAND in 1df262a 1b0b338; do
    r=$(rev $CFG); O=$PR/${CFG}_$CAND/cur; mkdir -p "$O"; RUN=$NAT/f0tx/${CFG}_$CAND
    "$KZ" predict "$RUN/run.kzt" $(prof $CFG $CAND) "${CONSTS[@]}" $(inputs $CFG $CAND) \
        --model $P/predict/00_common.kzm --model $P/predict/cf-$r.kzm \
        --prov-root $PROV/${CFG}_$CAND --observe-profile $P/observe/sites-cf$r.kzp --out-dir "$O" > "$O.log" 2>&1
    echo "predict $CFG $CAND exit=$?" | tee -a "$PR/predict.log"
  done; done
  { echo "# predict_w6.sh inputs $(date -u +%FT%T.%3NZ) repo $(git -C $R rev-parse HEAD) kairos_diff_lines=$(git -C $R diff HEAD -- kairos | wc -l)"
    sha256sum "$KZ"
    for CFG in pre post; do for CAND in 1df262a 1b0b338; do r=$(rev $CFG)
      sha256sum $NAT/f0tx/${CFG}_$CAND/run.kzt $NAT/f0tx/${CFG}_$CAND/basis_core-tx.kz; done; done
    sha256sum $P/predict/*.kzm $P/observe/*.kzp $P/rspec-281a941/*.kzp $P/rspec-833fdbb/*.kzp
    for CFG in pre post; do for CAND in 1df262a 1b0b338; do sha256sum $PR/${CFG}_$CAND/cur/*; done; done; } > "$PR/sha256.txt"
  ;;
check)
  CFG=${1:?cfg}; WN=${2:?W<n>}; r=$(rev $CFG)
  for CAND in 1df262a 1b0b338; do
    PLAN=$PR/${CFG}_$CAND/cur/$WN.kzh; O=$PR/${CFG}_$CAND/check/$WN; mkdir -p "$O"
    "$KZ" rtplan "$PLAN" --manifest $P/instr/sites-cf$r.kzs --binds $F1/${CFG}_$CAND/binds_instr.kzs \
        --out "$O/plan.kzrtp" --record "$O/rtplan.txt" > "$O/rtplan.out" 2>&1; echo "rtplan $CFG $CAND $WN exit=$?" | tee -a "$PR/check.log"
    for INST in tx rx; do
      if [ $INST = tx ]; then RUN=$NAT/f0tx/${CFG}_$CAND; B=$RUN/basis_core-tx.kz; else RUN=$NAT/f0rx/N_${CFG}_${CAND}_rx; B=$RUN/basis_core.kz; fi
      T=$C1/$CFG/$CAND/exe/$INST   # the build outputs the F0 driver untars (the records' inst/<inst>/cf/ were emptied)
      "$KZ" hold "$B" $P/observe/sites-cf$r.kzp "$PLAN" --basis "$RUN/basis_cf.so.kz" --basis "$RUN/basis_bp.so.kz" \
          --binary core-tx="$T/core-$INST" --binary cf.so="$T/cf/cf.so" --binary bp.so="$T/cf/bp.so" \
          -o "$O/hold_$INST.kzr" -- /bin/true > "$O/hold_$INST.out" 2>&1; echo "hold-check $CFG $CAND $WN $INST exit=$?" | tee -a "$PR/check.log"
    done
    sha256sum "$PLAN" "$O"/* > "$O/sha256.txt"
  done
  ;;
*) echo "step: closures|replay|predict|check" >&2; exit 64 ;;
esac

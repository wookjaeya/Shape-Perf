#!/usr/bin/env bash
# ctl_w6_predict.sh -- the C1 controlled runs of the W6 predict plans (plan/predict_w6_20261029.md sections 4-6) and
# their F0 replays with the W6 closures.  Pilot/dev and controlled runs only; not main-experiment results.
#
#   ctl_w6_predict.sh run <out root>                as root, CFS_NORMAL_USER=ubuntu.  The c1ctl block of
#       env/campaigns/w6_20261029.sh (L59-L73) with that script's wrapper (L29-L42: the flag file and its trap, quiet,
#       floor, the log header, the tracefs mount) and its per-run normalization line (L68), unchanged except
#         - the plan, label and tag: post W1 (/home/user/work/w6/predict/post_<cand>/cur/W1.kzh, P-C1HOLD, tag W1) and
#           pre W3 (/home/user/work/w6/predict/pre_<cand>/cur/W3.kzh, researcher intervention, outside P-C1HOLD, tag W3);
#           the plan is per build (the two candidates' plans differ in comments only), so it is set inside the loop;
#         - the log is <out root>/ctl_predict.log, so that the campaign's campaign.log stays as it was recorded;
#         - two refusals before anything starts: a flag file already present (another campaign is running), and a plan
#           whose sha256 is not the one /home/user/work/w6/predict/sha256.txt recorded for kz predict's output.
#       The G-I-H call of pre W3 is made as the campaign made the refused stage B/C calls: kz rtplan refuses the plan
#       (predict_w6_20261029.md section 5), run_gih_c1.sh writes refused.txt and exits 4 without starting cFS.
#   ctl_w6_predict.sh replay <out root> [run dir]...  after all runs (no replay overlaps a run; results_pilot_20261025.md
#       section 5.6.0).  Default run dirs: <out root>/c1ctl/f0/W1_post_* and W3_pre_*.  For each F0 controlled record:
#       kz convert run.kzt --hold hold.kzr (the realized control as NCTL; f0ctl_confirm2.sh, w4w5_c1_replay.sh f0ctl),
#       then kz replay of the converted container with the four profiles, the two constants, --basis <run>/basis_core.kz,
#       and --facts/--static-basis of /home/user/work/w6/closures/<cfg>_<cand>/c1prof (tx) or c1prof_rx (rx).
#       Readings, each with --out and kz occ show:
#         cur (--compare-narrowing; the current reading: P1 conditional, RF on, owner identity on, control order on =
#         the P11 reading decided in PD26 section 3), p1off, rfoff, rfoff_p1off, idoff, cw_counted, cw_excluded (the
#         readings of results_pilot_20261025.md sections 0 and 5.6.0), nocode (w4w5_c1_replay.sh), ctloff and
#         ctloff_p1off (P11 sensitivity, results_instr_gate_20261026.md section 3); then --emit-trace.
#       -> <out root>/c1ctl/replay/<run>/.  The G-I-H record of post W1 needs B' of the f1 build with the current
#       profiles (w4w5_c1_closures.sh bp f1 / basis_gi.sh); it is not made here.
# Harness choices (unspecified_by_reference): the log name, the two refusals, the replay output layout.
set -uo pipefail
STEP=${1:?run|replay}; OUT=${2:?out root}; shift 2
R=/home/user/Shape-Perf/research/cfs_procrace
K=$R/kairos
KZ=/home/user/work/kairos-build/tools/kz/kz
I=/home/user/work/instr/gcc
PR=/home/user/work/w6/predict
CL=/home/user/work/w6/closures
FLAG=/home/user/work/.campaign_running
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
case "$STEP" in
run)
  U=${CFS_NORMAL_USER:?set CFS_NORMAL_USER}
  [ "$(id -u)" = 0 ] || { echo "must run as root" >&2; exit 2; }
  [ ! -e "$FLAG" ] || { echo "$FLAG exists (a campaign is running); not starting" >&2; exit 75; }
  for CAND in 1df262a 1b0b338; do for W in post/W1 pre/W3; do
    PLAN=$PR/${W%/*}_$CAND/cur/${W#*/}.kzh
    grep -F "  $PLAN" "$PR/sha256.txt" | sha256sum -c --quiet - || { echo "plan differs from the kz predict output: $PLAN" >&2; exit 6; }
  done; done
  LOG=$OUT/ctl_predict.log
  TFS=$OUT/tracefs
  quiet() { { echo "## $1 $(date -u +%FT%T.%3NZ) kernel=$(uname -r)"; uptime; ps -eo pid,pcpu,comm --sort=-pcpu | head -6; df -BM --output=avail /home/user | tail -1; } >> "$LOG"; }
  floor() { local df; df=$(df --output=avail -BM /home/user | tail -1 | tr -dc 0-9); [ "$df" -gt 3000 ] || { echo "disk below 3 GB, stop" >> "$LOG"; exit 5; }; }
  : > "$FLAG"; trap 'rm -f "$FLAG"; umount "$TFS" 2>/dev/null' EXIT
  { echo "# ctl_w6_predict.sh run start $(date -u +%FT%T.%3NZ)"
    echo "repo $(git -C $R rev-parse HEAD) diff_kairos_lines=$(git -C $R diff HEAD -- kairos | wc -l)"
    echo "kz $KZ sha256=$(sha256sum $KZ | cut -c1-64)"
    echo "uname $(uname -a)"; } >> "$LOG"
  mkdir -p "$TFS"; mountpoint -q "$TFS" || mount -t tracefs nodev "$TFS"
  quiet "C1 controlled (W6 predict plans)"
  for CFG in post pre; do
    if [ $CFG = post ]; then TAG=W1; LABEL="P-C1HOLD"; else TAG=W3; LABEL="researcher intervention, outside P-C1HOLD"; fi
    for CAND in 1df262a 1b0b338; do for INST in tx rx; do
      PLAN=$PR/${CFG}_$CAND/cur/$TAG.kzh
      floor
      echo "$(date -u +%T) C1 controlled $TAG $CFG $CAND $INST F0 plan=$PLAN label=$LABEL" >> "$LOG"
      "$K/probe/scripts/observe_c1_hold.sh" $CFG $CAND $INST "$KZ" --out-dir "$OUT/c1ctl/f0" --tracefs "$TFS" --plan "$PLAN" --label "$LABEL" --tag $TAG >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
      { grep -m1 -E 'Starting the cFE with' "$OUT/c1ctl/f0/${TAG}_${CFG}_${CAND}_${INST}/inst/$INST/console_output.log"; grep -m1 'Exiting cFE with POWERON' "$OUT/c1ctl/f0/${TAG}_${CFG}_${CAND}_${INST}/inst/$INST/console_output.log"; ipcs -m | awk -v u=$U '$3==u'; } >> "$LOG" 2>&1
      echo "$(date -u +%T) C1 controlled $TAG $CFG $CAND $INST G-I-H" >> "$LOG"
      "$K/instrument/gcc/scripts/run_gih_c1.sh" "$I/f1/c1/${CFG}_$CAND" $CFG $CAND $INST "$PLAN" "$LABEL" $TAG "$OUT/c1ctl/gih" "$KZ" "$I/f1/rt-$CFG/kz-rt-dump" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    done; done
  done
  echo "# ctl_w6_predict.sh run end $(date -u +%FT%T.%3NZ)" >> "$LOG"
  ;;
replay)
  [ ! -e "$FLAG" ] || { echo "$FLAG exists (a campaign is running); wait" >&2; exit 75; }
  P=$K/profiles/c1-bp-b051eb0
  RUNS=("$@"); [ ${#RUNS[@]} -gt 0 ] || RUNS=("$OUT"/c1ctl/f0/W1_post_* "$OUT"/c1ctl/f0/W3_pre_*)
  for RUN in "${RUNS[@]}"; do
    b=$(basename "$RUN"); x=${b#*_}; CFG=${x%%_*}; CAND=$(echo "$x" | cut -d_ -f2); INST=${b##*_}
    case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "bad run name $b" >&2; exit 64 ;; esac
    SUF=; [ "$INST" = rx ] && SUF=_rx
    C=$CL/${CFG}_$CAND/c1prof$SUF; O=$OUT/c1ctl/replay/$b; mkdir -p "$O"; : > "$O/exit.txt"
    "$KZ" convert "$RUN/run.kzt" --hold "$RUN/hold.kzr" --out "$O/run.kzt" > "$O/convert.txt" 2>&1; echo "convert $?" >> "$O/exit.txt"
    A=("$O/run.kzt" --profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV
       --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20
       --basis "$RUN/basis_core.kz" --facts $C/closure.facts.kz --static-basis $C/closure.basis.kz)
    rd() { local n=$1; shift
      "$KZ" replay "${A[@]}" "$@" --out "$O/$n.kz" > "$O/$n.txt" 2>&1; echo "$n $?" >> "$O/exit.txt"
      "$KZ" occ show "$O/$n.kz" > "$O/occ_$n.txt" 2>&1; echo "occ_$n $?" >> "$O/exit.txt"; }
    rd cur --compare-narrowing
    rd p1off --outcome-narrowing off
    rd rfoff --reads-from-edges off
    rd rfoff_p1off --reads-from-edges off --outcome-narrowing off
    rd idoff --owner-identity-targets off
    rd cw_counted --clearing-writes counted
    rd cw_excluded --clearing-writes excluded
    rd nocode --outcome-narrowing off --reads-from-outcome off
    rd ctloff --control-order off
    rd ctloff_p1off --control-order off --outcome-narrowing off
    "$KZ" replay "${A[@]}" --emit-trace > "$O/trace.txt" 2>&1; echo "trace $?" >> "$O/exit.txt"
    sha256sum "$RUN/run.kzt" "$RUN/hold.kzr" "$RUN/basis_core.kz" $C/closure.facts.kz $C/closure.basis.kz "$KZ" "$O"/*.kzt "$O"/*.kz "$O"/*.txt 2>/dev/null \
      | grep -v '/sha256.txt$' > "$O/sha256.txt"
    echo "replay $b $(tr '\n' ' ' < "$O/exit.txt")"
  done
  ;;
*) echo "usage: ctl_w6_predict.sh run|replay <out root> [run dir]..." >&2; exit 64 ;;
esac

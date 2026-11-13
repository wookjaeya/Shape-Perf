#!/usr/bin/env bash
# w4w5_c1_replay.sh <kind> <record dir> <out dir> <closure root> -- W4/W5 (2026-10-28) C1: one record read with the fixed
# analysis configuration (one kz, the current profiles, the constants of results_pilot_20261025.md section 0, the
# closures and B' of w4w5_c1_closures.sh).  Pilot/dev and controlled runs only; nothing is run but kz.
#
# kinds (the instance tx|rx comes from the directory name; tx and rx are separate instances, P7, never merged):
#   f0nat     an F0 natural record (smoke3/<cfg>_<cand>, confirm1/N_<cfg>_<cand>_rx, W5 c1nat/f0tx|f0rx): its run.kzt
#             with the run's own core basis (basis_core-tx.kz or basis_core.kz) and the F0 closure
#   f0ctl     an F0 controlled record (confirm2/<W>_<cfg>_<cand>_<inst>, W5 c1ctl/f0): kz convert run.kzt --hold hold.kzr
#             (the realized control as NCTL, docs/replay_output.md section 6; the script f0ctl_confirm2.sh), then as f0nat
#   gid2      a stage D G-I record (instr/gcc/d3/c1/<cfg>_<cand>_<inst>, build d2): kz convert --kzrt with the manifest the
#             build used (d4/sites-cf<rev>.ebaf358.kzs, run.meta sha256) and the harness pid (run.meta tx_pid/rx_pid,
#             dump 1.1 records no process; results_instr_pilot_20261026.md section 8.2 item 1), B' of d2
#   gif1      a G-I or G-I-H record on a stage F build (f1/gih/*, W5 c1nat/gi/*, W5 c1ctl/gih/*): kz convert --kzrt with the
#             profile manifest (= run.meta sha256) and the f1 binding list, B' of f1
# readings: cur (defaults, with --compare-narrowing as in every earlier C1 replay script), p1off (--outcome-narrowing off),
#   nocode (--outcome-narrowing off --reads-from-outcome off); controlled records (with NCTL) also ctloff_p1off
#   (--control-order off --outcome-narrowing off) and ctloff (--control-order off; the stage F and f0ctl_confirm2.sh
#   sensitivity reading, read here only to compare with the earlier results that had no control input).  Each with --out.
# Also: the trace (--emit-trace, default reading) and its class (confirm2 classify.awk for F0, classify_gih.awk for the
# instrumented path), kz occ show of every container, the realized control lines (control_cmp.sh) of a controlled run.
set -uo pipefail
KIND=${1:?kind}; R=${2:?record}; O=${3:?out}; CR=${4:?closure root}
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
P=$KZ/profiles/c1-bp-b051eb0
b=$(basename "$R"); x=${b#N_}; case "$x" in W*) x=${x#*_} ;; esac
CFG=${x%%_*}; CAND=$(echo "$x" | cut -d_ -f2); INST=$(echo "$x" | cut -d_ -f3); [ -n "$INST" ] || INST=tx
case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "bad record name $b" >&2; exit 64 ;; esac
SUF=; [ "$INST" = rx ] && SUF=_rx
mkdir -p "$O"; : > "$O/exit.txt"
PROF=(--profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV
      --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20)
IN=()   # input files for sha256.txt
CTL=0
case "$KIND" in
  f0nat|f0ctl)
    BAS=$R/basis_core.kz; [ -f "$R/basis_core-$INST.kz" ] && BAS=$R/basis_core-$INST.kz
    CL=$CR/f0/${CFG}_${CAND}$SUF
    T=$R/run.kzt; IN+=("$R/run.kzt" "$BAS")
    if [ "$KIND" = f0ctl ]; then
      CTL=1; T=$O/run.kzt; IN+=("$R/hold.kzr")
      $KZ_BIN convert "$R/run.kzt" --hold "$R/hold.kzr" --out "$T" > "$O/convert.txt" 2>&1; echo "convert $?" >> "$O/exit.txt"
    fi
    A=("$T" "${PROF[@]}" --basis "$BAS" --facts $CL/closure.facts.kz --static-basis $CL/closure.basis.kz)
    IN+=($CL/closure.facts.kz $CL/closure.basis.kz) ;;
  gid2|gif1)
    if [ "$KIND" = gid2 ]; then
      W=/home/user/work/instr/gcc/d2/c1/${CFG}_${CAND}; MAN=/home/user/work/instr/gcc/d4/sites-cf$REV.ebaf358.kzs; BP=$CR/bprime_d2/${CFG}_${CAND}_$INST
      PID=$(sed -n "s/.*${INST}_pid=\([0-9]*\).*/\1/p" "$R/run.meta" | head -1); PROC=(--process "$PID")
    else
      W=/home/user/work/instr/gcc/f1/c1/${CFG}_${CAND}; MAN=$P/instr/sites-cf$REV.kzs; BP=$CR/bprime_f1/${CFG}_${CAND}_$INST; PROC=()
    fi
    want=$(sed -n 's/^manifest .* sha256=\([0-9a-f]*\)$/\1/p' "$R/run.meta" | head -1); have=$(sha256sum "$MAN" | cut -c1-64)
    [ "$want" = "$have" ] || { echo "manifest mismatch: run.meta $want, $MAN $have" >> "$O/exit.txt"; exit 65; }
    T=$O/run.kzt; IN+=("$R/dump/$INST.kzrt" "$MAN" "$W/binds_instr.kzs" $BP/basis.kz $BP/facts.kz)
    $KZ_BIN convert --kzrt "$R/dump/$INST.kzrt" --manifest "$MAN" --manifest "$W/binds_instr.kzs" --basis $BP/basis.kz "${PROC[@]}" \
        --out "$T" > "$O/convert.txt" 2>&1; echo "convert $?" >> "$O/exit.txt"
    grep -q 'control records [1-9]' "$O/convert.txt" && CTL=1
    A=("$T" "${PROF[@]}" --basis $BP/basis.kz --facts $BP/facts.kz --static-basis $BP/basis.kz) ;;
  *) echo "usage: w4w5_c1_replay.sh f0nat|f0ctl|gid2|gif1 <record> <out> <closure root>" >&2; exit 64 ;;
esac
rd() {  # name args...
  local n=$1; shift
  $KZ_BIN replay "${A[@]}" "$@" --out "$O/$n.kz" > "$O/$n.txt" 2>&1; echo "$n $?" >> "$O/exit.txt"
  $KZ_BIN occ show "$O/$n.kz" > "$O/occ_$n.txt" 2>&1; echo "occ_$n $?" >> "$O/exit.txt"
}
rd cur --compare-narrowing
rd p1off --outcome-narrowing off
rd nocode --outcome-narrowing off --reads-from-outcome off
if [ $CTL = 1 ]; then
  rd ctloff_p1off --control-order off --outcome-narrowing off
  rd ctloff --control-order off
fi
$KZ_BIN replay "${A[@]}" --emit-trace > "$O/trace.txt" 2>&1; echo "trace $?" >> "$O/exit.txt"
case "$KIND" in
  f0*) PLAN=/dev/null; HK=/dev/null
       if [ "$KIND" = f0ctl ]; then PLAN=$(sed -n 's/^hold plan=\([^ ]*\) sha256=.*/\1/p' "$R/driver.txt" | head -1); HK=$R/hold.kzr; IN+=("$PLAN"); fi
       awk -f /home/user/work/procrace/c1_runs/confirm2/scripts/classify.awk "$PLAN" "$HK" "$O/trace.txt" > "$O/class.txt" 2>&1 ;;
  *)   awk -f "$HERE/classify_gih.awk" "$O/trace.txt" > "$O/class.txt" 2>&1 ;;
esac
echo "classify $?" >> "$O/exit.txt"
if [ $CTL = 1 ]; then
  case "$KIND" in f0ctl) "$HERE/control_cmp.sh" f0 "$R" ;; *) "$HERE/control_cmp.sh" gih "$R" ;; esac > "$O/control.txt" 2>&1
  echo "control $?" >> "$O/exit.txt"
fi
sha256sum "${IN[@]}" "$O"/*.kz "$O"/*.txt "$KZ_BIN" $( [ -f "$O/run.kzt" ] && echo "$O/run.kzt") 2>/dev/null | grep -v '/sha256.txt$' > "$O/sha256.txt"
echo "$KIND $b $(tr '\n' ' ' < "$O/exit.txt")"

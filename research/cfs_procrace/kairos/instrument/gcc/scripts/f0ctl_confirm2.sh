#!/usr/bin/env bash
# f0ctl_confirm2.sh <out dir> -- the F0 confirm2 controlled runs with their own hold records as realized control (NCTL),
# next to the G-I-H runs of stage F, read by one kz and one profile snapshot: the comparison "the same control facts given
# to both paths" (docs/replay_output.md section 6).  Controlled runs, pilot/dev tooling; no new run is made.
#
# Per run <W>_<cfg>_<cand>_<inst> of /home/user/work/procrace/c1_runs/confirm2:
#   F0     kz convert run.kzt --hold hold.kzr (kz/trace/hold_nctl.hpp), then the stage F readings of replay_gih.sh f0 (the
#          closures confirm2 used, c1prof_i1025[_rx]): cur (control order on, P1 on, --compare-narrowing), p1off (control
#          order on, P1 off), ctloff (control order off), ctloff_p1off (control order off, P1 off = the F0 reading p1off of
#          results_instr_gate_20261026.md section 3), each with --out; the realized control lines (control_cmp.sh f0);
#   G-I-H  the stage F converted record (f1/gih/<run>/replay/run.kzt, B' f1/bprime) read again with the same kz: cur, p1off,
#          ctloff_p1off with --out;
#   occ    kz occ diff of F0 against G-I-H per reading (verdict and basis differences apart).
# The profiles are the stage F snapshot (97a24d8), as for the G-I-H numbers it is compared with.
# Writes <out>/<run>/{run.kzt,convert.txt,<reading>.txt,<reading>.kz,gih_<reading>.txt,gih_<reading>.kz,occ_<reading>.txt,
# control.txt,exit.txt,sha256.txt}.  KZ_BIN: the kz to use (default the default build).
set -uo pipefail
O=${1:?out dir}
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
F1=/home/user/work/instr/gcc/f1
C2=/home/user/work/procrace/c1_runs/confirm2
SNAP=$F1/snap_97a24d8/research/cfs_procrace/kairos/profiles
mkdir -p "$O"
for R in "$C2"/W*_*; do
  b=$(basename "$R"); x=${b#*_}; CFG=${x%%_*}; CAND=$(echo "$x" | cut -d_ -f2); INST=${b##*_}
  SUF=; [ "$INST" = rx ] && SUF=_rx
  case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "bad run name $b" >&2; exit 64 ;; esac
  P=$SNAP/c1-bp-b051eb0; D=$O/$b; mkdir -p "$D"; : > "$D/exit.txt"
  CL=/home/user/work/procrace/c1_build/kz/${CFG}_${CAND}/c1prof_i1025$SUF
  PROF=(--profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20)
  $KZ_BIN convert "$R/run.kzt" --hold "$R/hold.kzr" --out "$D/run.kzt" > "$D/convert.txt" 2>&1; echo "convert $?" >> "$D/exit.txt"
  A=("$D/run.kzt" "${PROF[@]}" --basis "$R/basis_core.kz" --facts $CL/closure.facts.kz --static-basis $CL/closure.basis.kz)
  $KZ_BIN replay "${A[@]}" --compare-narrowing --out "$D/cur.kz" > "$D/cur.txt" 2>&1; echo "cur $?" >> "$D/exit.txt"
  $KZ_BIN replay "${A[@]}" --outcome-narrowing off --out "$D/p1off.kz" > "$D/p1off.txt" 2>&1; echo "p1off $?" >> "$D/exit.txt"
  $KZ_BIN replay "${A[@]}" --control-order off --out "$D/ctloff.kz" > "$D/ctloff.txt" 2>&1; echo "ctloff $?" >> "$D/exit.txt"
  $KZ_BIN replay "${A[@]}" --control-order off --outcome-narrowing off --out "$D/ctloff_p1off.kz" > "$D/ctloff_p1off.txt" 2>&1; echo "ctloff_p1off $?" >> "$D/exit.txt"
  G=$F1/gih/$b/replay; BP=$F1/bprime/${CFG}_${CAND}_$INST
  GA=("$G/run.kzt" "${PROF[@]}" --basis $BP/basis.kz --facts $BP/facts.kz --static-basis $BP/basis.kz)
  $KZ_BIN replay "${GA[@]}" --compare-narrowing --out "$D/gih_cur.kz" > "$D/gih_cur.txt" 2>&1; echo "gih_cur $?" >> "$D/exit.txt"
  $KZ_BIN replay "${GA[@]}" --outcome-narrowing off --out "$D/gih_p1off.kz" > "$D/gih_p1off.txt" 2>&1; echo "gih_p1off $?" >> "$D/exit.txt"
  $KZ_BIN replay "${GA[@]}" --control-order off --outcome-narrowing off --out "$D/gih_ctloff_p1off.kz" > "$D/gih_ctloff_p1off.txt" 2>&1; echo "gih_ctloff_p1off $?" >> "$D/exit.txt"
  for r in cur p1off ctloff_p1off; do
    $KZ_BIN occ diff "$D/$r.kz" "$D/gih_$r.kz" > "$D/occ_$r.txt" 2>&1; echo "occ_$r $?" >> "$D/exit.txt"
  done
  "$HERE/control_cmp.sh" f0 "$R" > "$D/control.txt" 2>&1; echo "control $?" >> "$D/exit.txt"
  sha256sum "$R/run.kzt" "$R/hold.kzr" "$D"/*.kzt "$D"/*.kz $CL/closure.facts.kz $CL/closure.basis.kz "$KZ_BIN" > "$D/sha256.txt"
  echo "$b $(tr '\n' ' ' < "$D/exit.txt")"
done

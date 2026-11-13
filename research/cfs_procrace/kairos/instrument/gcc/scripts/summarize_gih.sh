#!/usr/bin/env bash
# summarize_gih.sh <G-I-H root> <F0 re-replay root> -- per stage F run, the IR 13.3 units next to its F0 confirm2
# counterpart (the replays of replay_gih.sh, one kz and one profile snapshot): the realized control (control_cmp.sh lines,
# same or not), the class (CF's lookup results per key and CF's end), the hold durations, the judgments per occurrence
# (name and its ordinal per name, status, summary, basis) in the readings
#   F0 cur            confirm2's reading B (no control order: F0 records have no NCTL)
#   ctloff            the G-I-H run with --control-order off (the sensitivity reading = reading B)
#   cur               the G-I-H run with the P11 decided reading (control order on)
#   p1off             the G-I-H run, control order on, outcome narrowing off
# and whether ctloff equals F0 cur.  Pilot/dev tooling; writes <G-I-H root>/summary.txt and per run <run>/replay/cmp.txt.
set -uo pipefail
G=${1:?gih root}; F=${2:?f0 re-replay root}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OCC=/home/user/work/procrace/integ_20261025/occ.awk
norm() {  # occ.awk lines -> "<name>#<k> <status> <summary> basis=<b>" (actor ids and event numbers left out)
  awk -F' [|] ' '{ split($1, a, " "); n = a[3]; k[n]++; b = $4; sub(/^basis=/, "", b); print n "#" k[n] " " $2 " " $3 " basis=" b }' | sort
}
combos() {  # the basis combinations of the first summary counts section (the reading of that replay; --compare-narrowing adds a second)
  awk '/^summary counts/ { n++ } n == 1 && /^  basis combination / { sub(/^  basis combination /, ""); print }' "$1" | paste -sd';'; }
holds_gih() {  # gate name -> hold duration ms (gate-hold t to resume t of the same point)
  awk '/^plan point /{ for (k = 1; k <= NF; k++) if ($k ~ /^name=/) { n = $k; sub(/^name=/, "", n); N[$3] = n } }
       /^ctl / { w = ""; p = ""; t = ""; for (k = 1; k <= NF; k++) { if ($k ~ /^what=/) w = substr($k, 6); if ($k ~ /^point=/) p = substr($k, 7); if ($k ~ /^t=/) t = substr($k, 3) }
                 if (w == "gate-hold") H[p] = t; if (w == "resume") R[p] = t }
       END { for (p in H) printf "%s=%.1fms ", N[p], (p in R) ? (R[p] - H[p]) / 1e6 : -1 }' "$1"
}
holds_f0() { awk '$1 == "hold" { g = ""; d = ""; for (k = 2; k <= NF; k++) { if ($k ~ /^gate=/) g = substr($k, 6); if ($k ~ /^hold_duration_ns=/) d = substr($k, 18) } printf "%s=%.1fms ", g, d / 1e6 }' "$1"; }
: > "$G/summary.txt"
for R in "$G"/W*_*; do
  b=$(basename "$R"); O="$R/replay"; FO="$F/$b"; C2=/home/user/work/procrace/c1_runs/confirm2/$b
  for r in cur ctloff p1off ctloff_p1off; do awk -f $OCC "$O/$r.txt" | norm > "$O/norm_$r.txt"; done
  for r in cur p1off; do awk -f $OCC "$FO/$r.txt" | norm > "$FO/norm_$r.txt"; done
  same_ctl=$(cmp -s "$O/control.txt" "$FO/control.txt" && echo same || echo DIFFERENT)
  cg=$(sed -n 's/^class: //p' "$O/class.txt")
  cf=$(sed -n 's/^class: //p' "$C2/replay_c1025/class.txt" | sed 's/ \[[^]]*\]//g')
  same_cls=$([ "$cg" = "$cf" ] && echo same || echo DIFFERENT)
  sb=$(cmp -s <(cut -d' ' -f1-3 "$O/norm_ctloff.txt") <(cut -d' ' -f1-3 "$FO/norm_cur.txt") && echo same || echo DIFFERENT)
  sbb=$(cmp -s "$O/norm_ctloff.txt" "$FO/norm_cur.txt" && echo same || echo DIFFERENT)
  {
    echo "run $b"
    echo "  control: $same_ctl ($(tr '\n' ';' < "$O/control.txt"))"
    echo "  class: $same_cls (G-I-H $cg | F0 $cf)"
    echo "  holds: G-I-H $(holds_gih "$O/../$(basename "$(ls "$R"/*.dump.txt)")") | F0 $(holds_f0 "$C2/hold.kzr")"
    echo "  judgments ctloff vs F0 cur: status/summary $sb, with basis $sbb"
    echo "  basis F0 cur: $(combos "$FO/cur.txt")"
    echo "  basis G-I-H ctloff: $(combos "$O/ctloff.txt")"
    echo "  basis G-I-H cur (P11 decided): $(combos "$O/cur.txt")"
    echo "  basis G-I-H p1off (P11 decided): $(combos "$O/p1off.txt")"
    echo "  basis G-I-H ctloff_p1off: $(combos "$O/ctloff_p1off.txt")"
    echo "  basis F0 p1off: $(combos "$FO/p1off.txt")"
    echo "  edges: $(grep -E '^realized control' "$O/cur.txt" | sed 's/^realized control: //')"
  } | tee "$O/cmp.txt" >> "$G/summary.txt"
done
grep -c 'control: same' "$G/summary.txt" | sed 's/^/runs with the same realized control: /'
grep -c 'class: same' "$G/summary.txt" | sed 's/^/runs with the same class: /'
grep -c 'status/summary same' "$G/summary.txt" | sed 's/^/runs whose ctloff judgments equal F0 cur: /'

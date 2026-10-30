#!/usr/bin/env bash
# f0ctl_summary.sh <f0ctl out dir> -- the counts of f0ctl_confirm2.sh: per plan and instance, the occurrences that the F0
# reading without control and without P1 (ctloff_p1off; results_instr_gate_20261026.md section 3, "P1 off: F0") leaves
# INCONCLUSIVE(horizon-open), and how many of those (matched by the occurrence key, docs/replay_output.md section 3) the
# reading with control order and without P1 (p1off) judges, for F0 with its hold record and for G-I-H; then the
# per-reading occ diff results (verdict and basis differences between the two paths).  Pilot/dev tooling.
set -uo pipefail
O=${1:?f0ctl out dir}
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
# kz occ show -> "key<TAB>verdict" (key = rspec | op | owner | resource | ordinal)
rows() { "$KZ_BIN" occ show "$1" | awk -F' [|] ' '/^#[0-9]+ / { k = $1; sub(/^#[0-9]+ /, "", k); print k " | " $2 " | " $3 " | " $4 " | " $5 "\t" $7 }'; }
judged() {  # <withheld reading .kz> <judging reading .kz>: "withheld judged"
  join -t $'\t' <(rows "$1" | awk -F'\t' '$2 ~ /INCONCLUSIVE\{[^}]*horizon-open/' | sort) <(rows "$2" | sort) |
    awk -F'\t' '{ n++; if ($3 !~ /INCONCLUSIVE/) j++ } END { printf "%d %d\n", n, j }'
}
printf "%-22s %-28s %-28s %s\n" run "F0+hold: withheld judged" "G-I-H: withheld judged" "occ diff F0 vs G-I-H (cur/p1off/ctloff_p1off): verdict,basis"
declare -A FW FJ GW GJ
for D in "$O"/W*_*; do
  b=$(basename "$D"); w=${b%%_*}
  read fw fj < <(judged "$D/ctloff_p1off.kz" "$D/p1off.kz")
  read gw gj < <(judged "$D/gih_ctloff_p1off.kz" "$D/gih_p1off.kz")
  d=""
  for r in cur p1off ctloff_p1off; do
    v=$(sed -n 's/^verdict differences (status, summary view): //p' "$D/occ_$r.txt"); s=$(sed -n 's/^basis differences (basis kinds, assumptions, outcome check, edge rules): //p' "$D/occ_$r.txt")
    d="$d ${v:-?},${s:-?}"
  done
  printf "%-22s %-28s %-28s %s\n" "$b" "$fw $fj" "$gw $gj" "$d"
  FW[$w]=$(( ${FW[$w]:-0} + fw )); FJ[$w]=$(( ${FJ[$w]:-0} + fj )); GW[$w]=$(( ${GW[$w]:-0} + gw )); GJ[$w]=$(( ${GJ[$w]:-0} + gj ))
done
echo
echo "per plan (sum of the 4 instances; per instance = sum / 4): plan F0-withheld F0-judged G-I-H-withheld G-I-H-judged"
tw=0; tj=0; uw=0; uj=0
for w in W1 W2 W10 W5 W6 W7 W8 W11 W12; do
  echo "$w ${FW[$w]:-0} ${FJ[$w]:-0} ${GW[$w]:-0} ${GJ[$w]:-0}"
  tw=$(( tw + ${FW[$w]:-0} )); tj=$(( tj + ${FJ[$w]:-0} )); uw=$(( uw + ${GW[$w]:-0} )); uj=$(( uj + ${GJ[$w]:-0} ))
done
echo "total $tw $tj $uw $uj"

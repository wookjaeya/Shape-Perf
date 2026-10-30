#!/usr/bin/env bash
# w4w5_c1_textcmp.sh <out root> -- W4 (2026-10-28) C1: the full replay text of the unified-config replay of each preserved
# record against the text an earlier replay of the same record and reading left, where no container exists for that
# reading (nocode) or as a check beside kz occ diff.  Lines compared after removing the command line, artifact hashes,
# paths and sha256 values (they name other files by construction).  Writes <out root>/occ/text.tsv and the diffs.
set -uo pipefail
O=${1:?out root}; O="$(cd "$O" && pwd)"; RP=$O/replay; D=$O/occ/text; mkdir -p "$D"
IG=/home/user/work/instr/gcc; PR=/home/user/work/procrace/c1_runs
norm() { grep -vE '^(kz replay|artifact_sha256|wrote )' "$1" | sed -E 's#/home/user/[^ ]*##g; s/[0-9a-f]{64}//g; s/[0-9a-f]{16}…//g'; }
printf 'earlier\trun\treading\tdiff_lines\n' > "$O/occ/text.tsv"
cmp1() {  # tag run reading unified earlier
  [ -f "$5" ] || return
  diff <(norm "$5") <(norm "$4") > "$D/$1__$2__$3.diff"; n=$(grep -c '^[<>]' "$D/$1__$2__$3.diff")
  printf '%s\t%s\t%s\t%s\n' "$1" "$2" "$3" "$n" >> "$O/occ/text.tsv"
}
for c in pre post; do for d in 1df262a 1b0b338; do
  for r in cur p1off nocode; do
    cmp1 d4_f0check ${c}_${d}_tx $r $RP/p_f0tx/${c}_$d/$r.txt $IG/d4/f0check/smoke3_${c}_$d/$r.txt
    cmp1 d4_f0check ${c}_${d}_rx $r $RP/p_f0rx/N_${c}_${d}_rx/$r.txt $IG/d4/f0check/Nrx_${c}_$d/$r.txt
    cmp1 i1025 ${c}_${d}_tx $r $RP/p_f0tx/${c}_$d/$r.txt $PR/smoke3/${c}_$d/replay_i1025/$r.txt
    cmp1 c1025 ${c}_${d}_rx $r $RP/p_f0rx/N_${c}_${d}_rx/$r.txt $PR/confirm1/N_${c}_${d}_rx/replay_c1025/$r.txt
    for i in tx rx; do cmp1 d4 ${c}_${d}_$i $r $RP/p_gi/${c}_${d}_$i/$r.txt $IG/d4/c1/${c}_${d}_$i/replay/$r.txt; done
  done
done; done
for x in $RP/p_f0ctl/W*; do b=$(basename $x)
  for r in cur p1off ctloff ctloff_p1off; do cmp1 f0ctl $b $r $x/$r.txt $IG/f0ctl/$b/$r.txt; done
  for r in cur p1off ctloff_p1off; do cmp1 f0ctl_gih $b $r $RP/p_gih/$b/$r.txt $IG/f0ctl/$b/gih_$r.txt; done
done
awk -F'\t' 'NR > 1 { k = $1 " " $3; n[k]++; if ($4 != 0) d[k]++ } END { for (k in n) printf "%-22s records %3d  differing %3d\n", k, n[k], d[k] }' "$O/occ/text.tsv" | sort

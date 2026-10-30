#!/usr/bin/env bash
# w4w5_c1_occ.sh <out root> -- W4/W5 (2026-10-28) C1: per-occurrence comparisons (kz occ diff, docs/replay_output.md
# section 4) of the containers of w4w5_c1_all.sh.  Exit 3 of kz occ diff (a difference or an ambiguous key) is recorded,
# not hidden.  The instance of both sides is the same in every pair (tx with tx, rx with rx; P7).
# Groups (<out root>/occ/<group>/<A run>__<reading>.txt, one TSV line each in <out root>/occ/occ.tsv):
#   env_<path>   preserved (kernel v153) vs W5 (v180) record of the same path and configuration/plan/instance:
#                f0tx, f0rx, gi (also build d2 -> f1), f0ctl, gih
#   path_w5      W5 F0 vs W5 instrumented of the same configuration/plan/instance (natural tx, rx; controlled with the F0
#                hold record as NCTL, the same control facts); path_p the same for the preserved records
#   w4_<earlier> unified-config replay of a preserved record vs a result recorded earlier for that record:
#                i1025 (smoke3 replay_i1025), c1025 (confirm1/confirm2 replay_c1025), d3 (stage D), d4 (stage D leftovers),
#                f0re (stage F F0), f1 (stage F G-I-H), f0ctl / f0ctl_gih (2026-10-28 NCTL); earlier readings without
#                control input are compared with the unified ctloff / ctloff_p1off.
set -uo pipefail
O=${1:?out root}; O="$(cd "$O" && pwd)"; RP=$O/replay; OC=$O/occ
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
PR=/home/user/work/procrace/c1_runs; IG=/home/user/work/instr/gcc
mkdir -p "$OC"; : > "$OC/pairs.txt"
pair() {  # group A.kz B.kz name reading
  echo "$1 $2 $3 $4 $5" >> "$OC/pairs.txt"
}
for c in pre post; do for d in 1df262a 1b0b338; do
  for r in cur p1off nocode; do
    pair env_f0tx $RP/p_f0tx/${c}_$d/$r.kz $RP/w_f0tx/${c}_$d/$r.kz ${c}_${d}_tx $r
    pair env_f0rx $RP/p_f0rx/N_${c}_${d}_rx/$r.kz $RP/w_f0rx/N_${c}_${d}_rx/$r.kz ${c}_${d}_rx $r
    for i in tx rx; do pair env_gi $RP/p_gi/${c}_${d}_$i/$r.kz $RP/w_gi/${c}_${d}_$i/$r.kz ${c}_${d}_$i $r; done
    pair path_w5 $RP/w_f0tx/${c}_$d/$r.kz $RP/w_gi/${c}_${d}_tx/$r.kz nat_${c}_${d}_tx $r
    pair path_w5 $RP/w_f0rx/N_${c}_${d}_rx/$r.kz $RP/w_gi/${c}_${d}_rx/$r.kz nat_${c}_${d}_rx $r
    pair path_p $RP/p_f0tx/${c}_$d/$r.kz $RP/p_gi/${c}_${d}_tx/$r.kz nat_${c}_${d}_tx $r
    pair path_p $RP/p_f0rx/N_${c}_${d}_rx/$r.kz $RP/p_gi/${c}_${d}_rx/$r.kz nat_${c}_${d}_rx $r
  done
  for r in cur p1off; do
    pair w4_i1025 $RP/p_f0tx/${c}_$d/$r.kz $PR/smoke3/${c}_$d/replay_i1025/$r.kz ${c}_${d}_tx $r
    pair w4_c1025 $RP/p_f0rx/N_${c}_${d}_rx/$r.kz $PR/confirm1/N_${c}_${d}_rx/replay_c1025/$r.kz ${c}_${d}_rx $r
    for i in tx rx; do
      pair w4_d3 $RP/p_gi/${c}_${d}_$i/$r.kz $IG/d3/c1/${c}_${d}_$i/replay/$r.kz ${c}_${d}_$i $r
      pair w4_d4 $RP/p_gi/${c}_${d}_$i/$r.kz $IG/d4/c1/${c}_${d}_$i/replay/$r.kz ${c}_${d}_$i $r
    done
  done
done; done
for D in $RP/p_f0ctl/W*; do b=$(basename $D)
  for r in cur p1off nocode ctloff_p1off ctloff; do
    pair env_f0ctl $D/$r.kz $RP/w_f0ctl/$b/$r.kz $b $r
    pair env_gih $RP/p_gih/$b/$r.kz $RP/w_gih/$b/$r.kz $b $r
    pair path_w5 $RP/w_f0ctl/$b/$r.kz $RP/w_gih/$b/$r.kz ctl_$b $r
    pair path_p $D/$r.kz $RP/p_gih/$b/$r.kz ctl_$b $r
  done
  pair w4_c1025 $D/ctloff.kz $PR/confirm2/$b/replay_c1025/cur.kz $b ctloff=cur
  pair w4_c1025 $D/ctloff_p1off.kz $PR/confirm2/$b/replay_c1025/p1off.kz $b ctloff_p1off=p1off
  pair w4_f0re $D/ctloff.kz $IG/f1/f0re/$b/cur.kz $b ctloff=cur
  pair w4_f0re $D/ctloff_p1off.kz $IG/f1/f0re/$b/p1off.kz $b ctloff_p1off=p1off
  for r in cur p1off ctloff ctloff_p1off; do pair w4_f0ctl $D/$r.kz $IG/f0ctl/$b/$r.kz $b $r; done
  for r in cur p1off ctloff; do pair w4_f1 $RP/p_gih/$b/$r.kz $IG/f1/gih/$b/replay/$r.kz $b $r; done
  for r in cur p1off ctloff_p1off; do pair w4_f0ctl_gih $RP/p_gih/$b/$r.kz $IG/f0ctl/$b/gih_$r.kz $b $r; done
done
printf 'group\tname\treading\trc\tocc_a\tocc_b\tmatched\tonly_a\tonly_b\tambiguous\twithout_owner\tby_text\tverdict_diff\tbasis_diff\treading_differs\tkeyed\ttask_in_key\n' > "$OC/occ.tsv"
while read -r g a bb n r; do
  mkdir -p "$OC/$g"; f=$OC/$g/${n}__$r.txt
  $KZ_BIN occ diff "$a" "$bb" > "$f" 2>&1; rc=$?
  awk -v g=$g -v n=$n -v r=$r -v rc=$rc '
    /^occurrences A / { s = $0; gsub(/[,;]/, "", s); split(s, w, " "); oa = w[3]; ob = w[5]; m = w[7]; ia = w[11]; ib = w[15]
      am = (match($0, /ambiguous keys [0-9]+/) ? substr($0, RSTART + 15, RLENGTH - 15) : 0)
      wo = (match($0, /matched without the owner [0-9]+/) ? substr($0, RSTART + 26, RLENGTH - 26) : 0)
      bt = (match($0, /matched by the text arguments [0-9]+/) ? substr($0, RSTART + 30, RLENGTH - 30) : 0) }
    /^verdict differences/ { vd = $NF } /^basis differences/ { bd = $NF } /^reading differs/ { rd++ }
    /^matching: by the occurrence key/ { k = "key" } /^matching: by position/ { k = "position" }
    /^task in the key: not used/ { t = "no" }
    END { printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%d\t%s\t%s\n", g, n, r, rc, oa, ob, m, ia, ib, am, wo, bt, vd, bd, rd, k, (t == "" ? "yes" : t) }' "$f" >> "$OC/occ.tsv"
done < "$OC/pairs.txt"
awk -F'\t' 'NR > 1 { k = $1; n[k]++; if ($4 != 0) x[k]++; v[k] += $13; b[k] += $14; a[k] += $10 + $11 + $12 + $8 + $9 }
  END { for (k in n) printf "%-14s pairs %3d  exit!=0 %3d  verdict-diff %4d  basis-diff %4d  unpaired/ambiguous/other %4d\n", k, n[k], x[k], v[k], b[k], a[k] }' "$OC/occ.tsv" | sort

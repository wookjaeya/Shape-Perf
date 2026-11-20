#!/usr/bin/env bash
# compare.sh STEP -- the outputs of run.sh (the MC kz-static of the declared kz-static closure build, results section
# 21) against the kept outputs of the fixed tool kz-static bf2ec2ee (/home/user/work/redesign/obs1/rf2_*), one step at a
# time, one report per output file and build in compare/ beside this script (never overwritten).  The steps are those of
# records/closure-kzstatic-20261119/scripts/scratch6_compare.sh (diffkinds.py and sections_cmp.py are the same files,
# read in place), so that every report has a twin among the declared reports; three steps are added:
#   declared     every report of compare/ (not the .samples files) against the report of the same name in
#                records/closure-kzstatic-20261119/scratch6/compare_2/ (the declared reports, pinned by
#                declared_reports.sha256): the same bytes, and the same set of names
#   twin         twin_sha.py: every output file of the ten cz_ directories against the sha256 of its scratch twin
#                (records/closure-kzstatic-20261119/scratch6/s6_outputs.sha256) after the declared map (twin_map.tsv)
#   again_rm     removes the again/ directories of cz_order, cz_off_order, cz_final_order (after their checks)
# Environment: KZB the tool directory (default the MC tree), SCRATCH a scratch directory for the facts views (the
# extract steps).  rc 0 when every byte check holds and every report was written, 1 otherwise.
# The other steps (relcheck, order, off_order, final_order, extract_off, extract_final, plans_t8cf, plans_final,
# plans_v4, plans_v4_ns2) are described in the scratch script.
set -uo pipefail
W=/home/user/work/redesign/obs1
O=$W
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
C=$HERE/compare
SCR=/home/user/Shape-Perf/research/cfs_procrace/records/closure-kzstatic-20261119/scripts
DECL=/home/user/Shape-Perf/research/cfs_procrace/records/closure-kzstatic-20261119/scratch6/compare_2
S6=/home/user/Shape-Perf/research/cfs_procrace/records/closure-kzstatic-20261119/scratch6/s6_outputs.sha256
SC=/home/user/Shape-Perf/research/cfs_procrace/records/review-fixes-2-mc-20261119/sections_cmp.py
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
STEP=${1:?usage}
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
mkdir -p "$C"
dk() { # OLD NEW REPORT [--tsv]
  local rep=$C/$3.txt
  [ -e "$rep" ] && { bad "$3: report exists"; return; }
  python3 -I "$SCR/diffkinds.py" "$1" "$2" ${4:-} --samples "$rep.samples" > "$rep" && ok "$3: $(head -1 "$rep")" || bad "$3: diffkinds failed"
}
sec() {
  local rep=$C/$3.txt
  [ -e "$rep" ] && { bad "$3: report exists"; return; }
  python3 -I "$SC" "$KD" "$1" "$2" > "$rep" && ok "$3: $(tail -1 "$rep")" || bad "$3: sections_cmp failed"
}
same() { cmp -s "$1" "$2" && ok "$3" || bad "$3: differs"; }
empty() { [ -e "$1" ] && [ ! -s "$1" ] && ok "$2 empty" || bad "$2 missing or not empty"; }
pairhead() { # PAIRS REPORT: the header lines and the tail of a pair file
  local rep=$C/$2.txt
  [ -e "$rep" ] && { bad "$2: report exists"; return; }
  { echo "lines $(wc -l < "$1") bytes $(wc -c < "$1") sha256 $(sha256sum < "$1" | cut -c1-64)"; head -7 "$1" | sed -E 's/^(facts|tool kz-static) .*/\1 <masked>/'; tail -2 "$1"
    echo "value-counts:"; awk -F'\t' 'NF>=10 {v=$10; sub(/\(.*/, "", v); n[$6 "\t" v]++} END {for (k in n) print n[k] "\t" k}' "$1" | sort -t$'\t' -k2; } > "$rep" && ok "$2 written" || bad "$2"
}
BUILDS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
case "$STEP" in
  relcheck)
    for b in $BUILDS; do
      dk "$W/rf2_relcheck/$b/relations.txt" "$O/cz_relcheck/$b/relations.txt" "relcheck_relations_$b"
      dk "$W/rf2_relcheck/$b/keys.txt" "$O/cz_relcheck/$b/keys.txt" "relcheck_keys_$b"
      empty "$O/cz_relcheck/$b/relations.log" "$b relations.log"
    done ;;
  order|off_order|final_order)
    case "$STEP" in
      order) P=$W/rf2_order; N=$O/cz_order; X= ;;
      off_order) P=$W/rf2_off_order; N=$O/cz_off_order; X=$O/cz_off_extract ;;
      final_order) P=$W/rf2_final_order; N=$O/cz_final_order; X=$O/cz_final_extract ;;
    esac
    for b in $BUILDS; do
      fs="order.txt edges.tsv pairs.tsv"; [ "$STEP" = final_order ] && fs="$fs cdr.txt"
      for f in $fs; do same "$N/$b/$f" "$N/$b/again/$f" "$b $f two runs the same bytes"; done
      dk "$P/$b/order.txt" "$N/$b/order.txt" "${STEP}_order_$b"
      dk "$P/$b/edges.tsv" "$N/$b/edges.tsv" "${STEP}_edges_$b" --tsv
      pairhead "$N/$b/pairs.tsv" "${STEP}_pairs_$b"
      [ "$STEP" = final_order ] && dk "$P/$b/cdr.txt" "$N/$b/cdr.txt" "${STEP}_cdr_$b"
      empty "$N/$b/order.log" "$b order.log"
      if [ -n "$X" ]; then
        same "$N/$b/relations.txt" "$X/$b/$b/NP/relations.txt" "$b relations.txt = the re-extraction's"
        dk "$P/$b/keys.txt" "$N/$b/keys.txt" "${STEP}_keys_$b"
        empty "$N/$b/relations.log" "$b relations.log"
      fi
    done ;;
  extract_off|extract_final)
    SCRATCH=${SCRATCH:?SCRATCH}
    if [ "$STEP" = extract_off ]; then P=$W/rf2_off_extract; N=$O/cz_off_extract; else P=$W/rf2_final_extract; N=$O/cz_final_extract; fi
    for b in $BUILDS; do
      p=$P/$b/$b/NP; n=$N/$b/$b/NP
      sec "$p/basis.kz" "$n/basis.kz" "${STEP}_basis_sections_$b"
      sec "$p/facts.kz" "$n/facts.kz" "${STEP}_facts_sections_$b"
      if "$KS" show "$p/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$n/facts.kz" > "$SCRATCH/new_$b.txt"; then
        dk "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" "${STEP}_view_$b"
      else bad "$b show"; fi
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt"
      for f in extract.txt relations.txt basis.txt sha256.txt args.txt; do dk "$p/$f" "$n/$f" "${STEP}_${f%.txt}_$b"; done
      same "$p/relations.log" "$n/relations.log" "$b relations.log the same bytes as the kept one"
      grep -qx 'result PASS' "$n/status.txt" 2> /dev/null && ok "$b status.txt result PASS" || bad "$b status.txt"
    done ;;
  plans_t8cf|plans_final|plans_v4|plans_v4_ns2)
    case "$STEP" in
      plans_t8cf) P=$W/rf2_plans_t8cf; N=$O/cz_plans_t8cf; fs="plan.kzs plan.side plan.mod plan.exe args.txt" ;;
      plans_final) P=$W/rf2_plans_final; N=$O/cz_plans_final; fs="plan.kzs plan.side plan.mod plan.exe args.txt" ;;
      plans_v4) P=$W/rf2_plans_v4; N=$O/cz_plans_v4; fs="plan.kzs plan.side plan.req plan.mod plan.exe args.txt" ;;
      plans_v4_ns2) P=$W/rf2_plans_v4; N=$O/cz_plans_v4_ns2; fs="plan.kzs plan.side plan.req plan.mod plan.exe args.txt" ;;
    esac
    for b in $BUILDS; do
      for f in $fs; do dk "$P/$b/$f" "$N/$b/$f" "${STEP}_$(echo "$f" | tr . _)_$b"; done
      same "$P/$b/plan.log" "$N/$b/plan.log" "$b plan.log the same bytes as the kept one"
    done ;;
  declared)
    ( cd "$DECL" && sha256sum -c --quiet "$HERE/declared_reports.sha256" ) && ok "declared reports match declared_reports.sha256" || bad "declared reports changed"
    n=0; m=0
    for r in $(cd "$DECL" && ls *.txt | grep -v '\.samples$'); do
      n=$((n+1))
      if [ ! -e "$C/$r" ]; then bad "declared $r: no report in compare/"; continue; fi
      cmp -s "$DECL/$r" "$C/$r" && m=$((m+1)) || bad "declared $r: differs from the declared report"
    done
    for r in $(cd "$C" && ls *.txt | grep -v '\.samples$'); do [ -e "$DECL/$r" ] || bad "declared $r: not a declared report"; done
    echo "summary declared reports $n, the same bytes $m" ;;
  twin)
    python3 -I "$HERE/twin_sha.py" "$S6" "$W" "$HERE/twin_map.tsv" --scratch "${TWIN_SCRATCH:?TWIN_SCRATCH: the scratch directory of the sixth run}" || rc=1 ;;
  again_rm)
    for s in cz_order cz_off_order cz_final_order; do for b in $BUILDS; do
      if [ -d "$W/$s/$b/again" ]; then rm -r "$W/$s/$b/again" && ok "removed $s/$b/again" || bad "rm $s/$b/again"; else bad "$s/$b/again missing"; fi
    done; done ;;
  *) echo "usage: compare.sh relcheck|order|off_order|final_order|extract_off|extract_final|plans_t8cf|plans_final|plans_v4|plans_v4_ns2|declared|twin|again_rm" >&2; exit 64 ;;
esac
exit $rc

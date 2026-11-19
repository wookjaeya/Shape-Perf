#!/usr/bin/env bash
# scratch6_compare.sh STEP -- the outputs of scratch6_run.sh (the scratch check build of the complete source of the
# kz-static closure build) against the kept outputs of the fixed tool kz-static bf2ec2ee (/home/user/work/redesign/obs1/
# rf2_*), one step at a time, one report per output file and build in CMP (never overwritten): diffkinds.py for text,
# sections_cmp.py (records/review-fixes-2-mc-20261119) for containers.  The reports are [I] for the declaration of the
# build: they say which lines and sections differ and how many, by kind; they judge nothing.  A check of results
# section 20, not a tool of the method.
#   relcheck     s6_relcheck/<b> against rf2_relcheck/<b>: relations.txt, keys.txt; relations.log empty
#   order        s6_order/<b> against rf2_order/<b>: two runs the same bytes (order.txt, edges.tsv, pairs.tsv); order.txt,
#                edges.tsv (--tsv); the pair file's head and tail; order.log empty
#   off_order    s6_off_order/<b> against rf2_off_order/<b>: as order, and keys.txt; relations.txt the same bytes as the
#                re-extraction's; relations.log empty
#   final_order  s6_final_order/<b> against rf2_final_order/<b>: as off_order, and cdr.txt (two runs, and against the kept)
#   extract_off  s6_off_extract/<b>/<b>/NP against rf2_off_extract: basis.kz, facts.kz by section; the facts views (kz-static
#                show of both by the scratch tool, made in SCRATCH and removed); extract.txt, relations.txt, basis.txt,
#                sha256.txt, args.txt; relations.log the same bytes; status.txt result PASS
#   extract_final the same against rf2_final_extract
#   plans_t8cf / plans_final / plans_v4   s6_plans_<x>/<b> against rf2_plans_<x>/<b>: plan.kzs plan.side plan.mod plan.exe
#                args.txt (and plan.req for v4); plan.log the same bytes
#   plans_v4_ns2 s6_plans_v4_ns2/<b> against rf2_plans_v4/<b> (the kept revision-4 plans): the same files
# Environment: OROOT the scratch output root, KZB the scratch tool directory, CMP the report directory, SCRATCH a
# scratch directory for the views.  rc 0 when every byte check holds and every report was written, 1 otherwise.
set -uo pipefail
W=/home/user/work/redesign/obs1
O=${OROOT:?OROOT}
KZB=${KZB:?KZB}
KS=$KZB/kz-static/kz-static
C=${CMP:?CMP}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
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
  python3 -I "$HERE/diffkinds.py" "$1" "$2" ${4:-} --samples "$rep.samples" > "$rep" && ok "$3: $(head -1 "$rep")" || bad "$3: diffkinds failed"
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
      dk "$W/rf2_relcheck/$b/relations.txt" "$O/s6_relcheck/$b/relations.txt" "relcheck_relations_$b"
      dk "$W/rf2_relcheck/$b/keys.txt" "$O/s6_relcheck/$b/keys.txt" "relcheck_keys_$b"
      empty "$O/s6_relcheck/$b/relations.log" "$b relations.log"
    done ;;
  order|off_order|final_order)
    case "$STEP" in
      order) P=$W/rf2_order; N=$O/s6_order; X= ;;
      off_order) P=$W/rf2_off_order; N=$O/s6_off_order; X=$O/s6_off_extract ;;
      final_order) P=$W/rf2_final_order; N=$O/s6_final_order; X=$O/s6_final_extract ;;
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
    if [ "$STEP" = extract_off ]; then P=$W/rf2_off_extract; N=$O/s6_off_extract; else P=$W/rf2_final_extract; N=$O/s6_final_extract; fi
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
      plans_t8cf) P=$W/rf2_plans_t8cf; N=$O/s6_plans_t8cf; fs="plan.kzs plan.side plan.mod plan.exe args.txt" ;;
      plans_final) P=$W/rf2_plans_final; N=$O/s6_plans_final; fs="plan.kzs plan.side plan.mod plan.exe args.txt" ;;
      plans_v4) P=$W/rf2_plans_v4; N=$O/s6_plans_v4; fs="plan.kzs plan.side plan.req plan.mod plan.exe args.txt" ;;
      plans_v4_ns2) P=$W/rf2_plans_v4; N=$O/s6_plans_v4_ns2; fs="plan.kzs plan.side plan.req plan.mod plan.exe args.txt" ;;
    esac
    for b in $BUILDS; do
      for f in $fs; do dk "$P/$b/$f" "$N/$b/$f" "${STEP}_$(echo "$f" | tr . _)_$b"; done
      same "$P/$b/plan.log" "$N/$b/plan.log" "$b plan.log the same bytes as the kept one"
    done ;;
  *) echo "usage: scratch6_compare.sh relcheck|order|off_order|final_order|extract_off|extract_final|plans_t8cf|plans_final|plans_v4|plans_v4_ns2" >&2; exit 64 ;;
esac
exit $rc

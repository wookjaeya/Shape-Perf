#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration (plan/full_stage_results_20261116.md 5.0), one step at a time:
#   relations  extfix_relcheck/<b>/{relations.txt,keys.txt} against schema12_relcheck/<b>/: only line 3 (the rules
#              revision 7 -> 8) differs; relations.log empty
#   order      extfix_order/<b>/{order.txt,edges.tsv}: two runs the same bytes; against schema12_order/<b>/ only line 3 of
#              order.txt (relations rules revision) differs, edges.tsv byte-identical; order.log empty
#   extract    extfix_extract/<b>/<b>/NP against schema12_extract/<b>/<b>/NP: basis.kz record by record (check_head.py
#              --whole), facts.kz header (check_head.py), the facts view (kz-static show, both read by the new tool; views
#              written to SCRATCH and removed after) with compare_classes.py facts_allow.tsv, extract.txt with
#              extract_txt_allow.tsv, relations.txt with relations_allow.tsv, basis.txt and sha256.txt only the hash lines,
#              args.txt and relations.log the same; the FACT_LOCS and FIELD_STATUS rows of the renamed types into
#              compare/names_<b>.txt
#   onextract  extfix_on_extract/<b>: order two runs the same bytes, order.txt with order_allow.tsv against
#              schema12_on_extract/<b>/order.txt, edges.tsv byte-identical, relations.txt the same bytes as the
#              re-extraction's; key_map.py extfix_pre_onextract/<b>/keys.txt -> extfix_on_extract/<b>/keys.txt into
#              key_map_<b>.tsv
#   plans      compare_plans.py t8cf schema12_plans/<b> extfix_plans/<b>; compare_plans.py extract
#              extfix_pre_onextract/<b>/plan extfix_plans_on_extract/<b>
# One line per check; rc 0 when every check holds, 1 otherwise.  Tools: the build tree's (MC).
set -uo pipefail
O=/home/user/work/redesign/obs1
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
only_line3() { # A B PATTERN: equal apart from line 3, and line 3 of B matches PATTERN
  cmp -s <(sed 3d "$1") <(sed 3d "$2") && sed -n 3p "$2" | grep -qE "$3"; }
cls() { python3 -I "$HERE/compare_classes.py" "$@"; }
mkdir -p "$HERE/compare"
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  case "${1:-}" in
    relations)
      for f in relations.txt keys.txt; do
        only_line3 "$O/schema12_relcheck/$b/$f" "$O/extfix_relcheck/$b/$f" '^rules relations rules revision 8 \(2026-11-16\)$' && ok "$b $f same as schema12_relcheck apart from line 3 (rules revision 8)" || bad "$b $f differs from schema12_relcheck beyond line 3"
      done
      [ -s "$O/extfix_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do
        cmp -s "$O/extfix_order/$b/$f" "$O/extfix_order/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"
      done
      only_line3 "$O/schema12_order/$b/order.txt" "$O/extfix_order/$b/order.txt" '^relations relations rules revision 8 \(2026-11-16\)$' && ok "$b order.txt same as schema12_order apart from line 3" || bad "$b order.txt differs from schema12_order beyond line 3"
      cmp -s "$O/schema12_order/$b/edges.tsv" "$O/extfix_order/$b/edges.tsv" && ok "$b edges.tsv same as schema12_order" || bad "$b edges.tsv differs from schema12_order"
      [ -s "$O/extfix_order/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/extfix_order/$b/again" ;;
    extract)
      N=$O/extfix_extract/$b/$b/NP; P=$O/schema12_extract/$b/$b/NP; C=$HERE/compare
      python3 -I "$HERE/check_head.py" "$KD" "$P/basis.kz" "$N/basis.kz" --whole > "$C/basis_$b.txt"; [ $? = 0 ] && ok "$b basis.kz: $(head -1 "$C/basis_$b.txt")" || bad "$b basis.kz (compare/basis_$b.txt)"
      python3 -I "$HERE/check_head.py" "$KD" "$P/facts.kz" "$N/facts.kz" > "$C/facts_head_$b.txt"; [ $? = 0 ] && ok "$b facts.kz: $(head -1 "$C/facts_head_$b.txt")" || bad "$b facts.kz header (compare/facts_head_$b.txt)"
      "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
      cls "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" --allow "$HERE/facts_allow.tsv" --build "$b" --list 40 > "$C/facts_$b.txt"; [ $? = 0 ] && ok "$b facts view: $(tail -1 "$C/facts_$b.txt")" || bad "$b facts view (compare/facts_$b.txt)"
      { echo "# FACT_LOCS and FIELD_STATUS rows of record types without a file-scope name, before (schema12_extract) and after"
        for side in old new; do echo "## $side"; awk '/^\[locs\]|^\[fstatus\]/{s=1;print;next} /^\[/{s=0} s' "$SCRATCH/${side}_$b.txt" | grep -E '<anonymous|::|@[^ ]*_[a-z]+:[0-9]' ; done; } > "$C/names_$b.txt"
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt"
      cls "$P/extract.txt" "$N/extract.txt" --allow "$HERE/extract_txt_allow.tsv" --build "$b" > "$C/extract_txt_$b.txt"; [ $? = 0 ] && ok "$b extract.txt: $(tail -1 "$C/extract_txt_$b.txt")" || bad "$b extract.txt (compare/extract_txt_$b.txt)"
      cls "$P/relations.txt" "$N/relations.txt" --allow "$HERE/relations_allow.tsv" --build "$b" > "$C/relations_$b.txt"; [ $? = 0 ] && ok "$b relations.txt: $(tail -1 "$C/relations_$b.txt")" || bad "$b relations.txt (compare/relations_$b.txt)"
      cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
      cmp -s "$P/args.txt" "$N/args.txt" && ok "$b args.txt same" || bad "$b args.txt differs"
      cmp -s "$P/relations.log" "$N/relations.log" && ok "$b relations.log same" || bad "$b relations.log differs" ;;
    onextract)
      E=$O/extfix_on_extract/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      cls "$O/schema12_on_extract/$b/order.txt" "$E/order.txt" --allow "$HERE/order_allow.tsv" --build "$b" > "$HERE/compare/order_$b.txt"; [ $? = 0 ] && ok "$b order.txt: $(tail -1 "$HERE/compare/order_$b.txt")" || bad "$b order.txt (compare/order_$b.txt)"
      cmp -s "$O/schema12_on_extract/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv same as schema12_on_extract" || bad "$b edges.tsv differs from schema12_on_extract"
      cmp -s "$E/relations.txt" "$O/extfix_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt same as the re-extraction's" || bad "$b relations.txt differs from the re-extraction's"
      echo "OK $b key map: $(python3 -I "$HERE/key_map.py" "$O/extfix_pre_onextract/$b/keys.txt" "$E/keys.txt" "$HERE/key_map_$b.tsv")"
      rm -r "$E/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" t8cf "$O/schema12_plans/$b" "$O/extfix_plans/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" extract "$O/extfix_pre_onextract/$b/plan" "$O/extfix_plans_on_extract/$b" | sed "s/^/$b extract /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    *) echo "usage: compare.sh relations|order|extract|onextract|plans" >&2; exit 64 ;;
  esac
done
exit $rc

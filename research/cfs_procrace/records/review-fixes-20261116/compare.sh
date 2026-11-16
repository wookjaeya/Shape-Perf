#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration of the review-fix build (plan/full_stage_results_20261116.md 6.0 (나)),
# one step at a time, against the extractor-fix step's outputs:
#   relations  revfix_relcheck/<b>: relations.txt against extfix_relcheck with compare_relations.py; keys.txt the same
#              apart from line 3 (rules revision 8 -> 9); relations.log empty
#   order      revfix_order/<b>: two runs the same bytes; against extfix_order with compare_order.py; order.log empty
#   extract    revfix_extract/<b>/<b>/NP against extfix_extract: basis.kz record by record (check_head.py --whole: only
#              tool_sha256, artifact_sha256, basis_sha256 and parent[0] may differ), facts.kz record by record (check_whole.py:
#              the same and the STRINGS crc32 and extract.units_from basis hash, addendum 6.0 (나-1)), the facts views
#              (kz-static show, both read by the new tool, written to SCRATCH and removed after) only the extract.units_from
#              line, extract.txt apart from line 1 (facts hash), basis.txt and sha256.txt only the hash lines, relations.txt
#              against extfix_extract's with compare_relations.py --facts-line, args.txt and relations.log the same
#   onextract  revfix_on_extract/<b>: order two runs the same; order.txt and edges.tsv against extfix_on_extract with
#              compare_order.py --facts-line; edges.tsv = revfix_order's; relations.txt = the re-extraction's; keys.txt =
#              revfix_relcheck's apart from line 2
#   plans      compare_plans.py t8cf extfix_plans/<b> revfix_plans/<b>; compare_plans.py extract
#              extfix_plans_on_extract/<b> revfix_plans_on_extract/<b>
# One line per check; rc 0 when every check holds, 1 otherwise.  Tools: the build tree's (MC).
set -uo pipefail
O=/home/user/work/redesign/obs1
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OLDREC="$(cd "$HERE/../extract-fixes-20261116" && pwd)"
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
except_line() { # N A B: equal apart from line N
  cmp -s <(sed "$1d" "$2") <(sed "$1d" "$3"); }
mkdir -p "$HERE/compare"
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  C=$HERE/compare
  case "${1:-}" in
    relations)
      python3 -I "$HERE/compare_relations.py" "$O/extfix_relcheck/$b/relations.txt" "$O/revfix_relcheck/$b/relations.txt" > "$C/relations_$b.txt"; [ $? = 0 ] && ok "$b relations.txt: $(head -1 "$C/relations_$b.txt")" || bad "$b relations.txt (compare/relations_$b.txt)"
      except_line 3 "$O/extfix_relcheck/$b/keys.txt" "$O/revfix_relcheck/$b/keys.txt" && sed -n 3p "$O/revfix_relcheck/$b/keys.txt" | grep -qx 'rules relations rules revision 9 (2026-11-16)' && ok "$b keys.txt same apart from line 3 (rules revision 9)" || bad "$b keys.txt differs beyond line 3"
      [ -s "$O/revfix_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do cmp -s "$O/revfix_order/$b/$f" "$O/revfix_order/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      python3 -I "$HERE/compare_order.py" "$O/extfix_order/$b" "$O/revfix_order/$b" > "$C/order_$b.txt"; [ $? = 0 ] && ok "$b order: $(head -1 "$C/order_$b.txt" | cut -c1-400)" || bad "$b order (compare/order_$b.txt)"
      [ -s "$O/revfix_order/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/revfix_order/$b/again" ;;
    extract)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/revfix_extract/$b/$b/NP; P=$O/extfix_extract/$b/$b/NP
      python3 -I "$OLDREC/check_head.py" "$KD" "$P/basis.kz" "$N/basis.kz" --whole > "$C/basis_$b.txt"; [ $? = 0 ] && ok "$b basis.kz: $(head -1 "$C/basis_$b.txt")" || bad "$b basis.kz (compare/basis_$b.txt)"
      python3 -I "$HERE/check_whole.py" "$KD" "$P/facts.kz" "$N/facts.kz" > "$C/facts_whole_$b.txt"; [ $? = 0 ] && ok "$b facts.kz: $(head -1 "$C/facts_whole_$b.txt")" || bad "$b facts.kz (compare/facts_whole_$b.txt)"
      "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
      d=$(diff "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" | grep '^[<>]' | sed -E 's/^([<>])   extract\.units_from = basis [0-9a-f]{16}  \(unset\)$/\1 UNITS/' | sort | uniq -c | tr -s ' ' | tr '\n' ';')
      [ "$d" = " 1 < UNITS; 1 > UNITS;" ] && ok "$b facts view: only the extract.units_from line ($(wc -l < "$SCRATCH/new_$b.txt") lines)" || bad "$b facts view differs beyond the extract.units_from line ($d)"
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt"
      except_line 1 "$P/extract.txt" "$N/extract.txt" && ok "$b extract.txt same apart from line 1" || bad "$b extract.txt differs beyond line 1"
      python3 -I "$HERE/compare_relations.py" "$P/relations.txt" "$N/relations.txt" --facts-line > "$C/extract_relations_$b.txt"; [ $? = 0 ] && ok "$b relations.txt against extfix_extract's: $(head -1 "$C/extract_relations_$b.txt")" || bad "$b relations.txt against extfix_extract's (compare/extract_relations_$b.txt)"
      cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
      cmp -s "$P/args.txt" "$N/args.txt" && ok "$b args.txt same" || bad "$b args.txt differs"
      cmp -s "$P/relations.log" "$N/relations.log" && ok "$b relations.log same" || bad "$b relations.log differs" ;;
    onextract)
      E=$O/revfix_on_extract/$b
      if [ -d "$E/again" ]; then for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      else ok "$b two runs compared in the first run of this step (first/compare_onextract_first.txt), again/ removed then"; fi
      python3 -I "$HERE/compare_order.py" "$O/extfix_on_extract/$b" "$E" --facts-line > "$C/onextract_order_$b.txt"; [ $? = 0 ] && ok "$b order against extfix_on_extract: $(head -1 "$C/onextract_order_$b.txt" | cut -c1-300)" || bad "$b order against extfix_on_extract (compare/onextract_order_$b.txt)"
      cmp -s "$O/revfix_order/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv = revfix_order's" || bad "$b edges.tsv differs from revfix_order's"
      cmp -s "$E/relations.txt" "$O/revfix_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the re-extraction's" || bad "$b relations.txt differs from the re-extraction's"
      except_line 2 "$O/revfix_relcheck/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = revfix_relcheck's apart from line 2" || bad "$b keys.txt differs from revfix_relcheck's beyond line 2"
      rm -rf "$E/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" t8cf "$O/extfix_plans/$b" "$O/revfix_plans/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" extract "$O/extfix_plans_on_extract/$b" "$O/revfix_plans_on_extract/$b" | sed "s/^/$b extract /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    *) echo "usage: compare.sh relations|order|extract|onextract|plans" >&2; exit 64 ;;
  esac
done
exit $rc

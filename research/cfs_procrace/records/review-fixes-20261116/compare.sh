#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration of the review-fix build (plan/full_stage_results_20261116.md 6.0 (나)),
# one step at a time, against the extractor-fix step's outputs:
#   relations  revfix_relcheck/<b>: relations.txt against extfix_relcheck with compare_relations.py; keys.txt the same
#              apart from line 3 (rules revision 8 -> 9); relations.log empty
#   order      revfix_order/<b>: two runs the same bytes; against extfix_order with compare_order.py; order.log empty
#   extract    revfix_extract/<b>/<b>/NP against extfix_extract: basis.kz and facts.kz record by record (check_head.py
#              --whole: only tool_sha256, artifact_sha256, basis_sha256 and parent[0] may differ), the facts views (kz-static
#              show, both read by the new tool, written to SCRATCH and removed after) byte-identical, extract.txt apart from
#              line 1 (facts hash), basis.txt and sha256.txt only the hash lines, relations.txt = revfix_relcheck's apart
#              from line 2, args.txt and relations.log the same
#   onextract  revfix_on_extract/<b>: order two runs the same; order.txt = revfix_order's apart from line 2 (facts hash),
#              edges.tsv the same bytes; relations.txt = the re-extraction's; keys.txt = revfix_relcheck's apart from line 2
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
      python3 -I "$OLDREC/check_head.py" "$KD" "$P/facts.kz" "$N/facts.kz" --whole > "$C/facts_head_$b.txt"; [ $? = 0 ] && ok "$b facts.kz: $(head -1 "$C/facts_head_$b.txt")" || bad "$b facts.kz header (compare/facts_head_$b.txt)"
      "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
      cmp -s "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" && ok "$b facts view byte-identical ($(wc -l < "$SCRATCH/new_$b.txt") lines)" || bad "$b facts view differs"
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt"
      except_line 1 "$P/extract.txt" "$N/extract.txt" && ok "$b extract.txt same apart from line 1" || bad "$b extract.txt differs beyond line 1"
      except_line 2 "$O/revfix_relcheck/$b/relations.txt" "$N/relations.txt" && ok "$b relations.txt = revfix_relcheck's apart from line 2" || bad "$b relations.txt differs from revfix_relcheck's beyond line 2"
      cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
      cmp -s "$P/args.txt" "$N/args.txt" && ok "$b args.txt same" || bad "$b args.txt differs"
      cmp -s "$P/relations.log" "$N/relations.log" && ok "$b relations.log same" || bad "$b relations.log differs" ;;
    onextract)
      E=$O/revfix_on_extract/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      except_line 2 "$O/revfix_order/$b/order.txt" "$E/order.txt" && ok "$b order.txt = revfix_order's apart from line 2" || bad "$b order.txt differs from revfix_order's beyond line 2"
      cmp -s "$O/revfix_order/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv = revfix_order's" || bad "$b edges.tsv differs from revfix_order's"
      cmp -s "$E/relations.txt" "$O/revfix_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the re-extraction's" || bad "$b relations.txt differs from the re-extraction's"
      except_line 2 "$O/revfix_relcheck/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = revfix_relcheck's apart from line 2" || bad "$b keys.txt differs from revfix_relcheck's beyond line 2"
      rm -r "$E/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" t8cf "$O/extfix_plans/$b" "$O/revfix_plans/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" extract "$O/extfix_plans_on_extract/$b" "$O/revfix_plans_on_extract/$b" | sed "s/^/$b extract /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    *) echo "usage: compare.sh relations|order|extract|onextract|plans" >&2; exit 64 ;;
  esac
done
exit $rc

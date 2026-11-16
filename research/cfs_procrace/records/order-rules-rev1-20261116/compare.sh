#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration (plan/full_stage_results_20261116.md 3.0), one step at a time:
#   relations  orderrev1_relcheck/<b>/{relations.txt,keys.txt} byte-identical to relrev7_check/<b>/, relations.log empty
#   order      order_rev1/<b>/{order.txt,edges.tsv} byte-identical to the second run (again/, removed after), order.log empty
#   extract    order on the re-extracted facts (order_rev1_on_extract/<b>/) equal to order_rev1/<b>/ apart from line 2 of
#              order.txt (the facts hash), edges.tsv byte-identical; the extraction's relations.txt equal to
#              orderrev1_relcheck/<b>/relations.txt apart from line 2 (compare_build.py is run separately)
#   plans      compare_plans.py relrev7_plans/<b> order_rev1_plans/<b>
# One line per check; rc 0 when every check holds, 1 otherwise.
set -uo pipefail
O=/home/user/work/redesign/obs1
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
same_but_line2() { cmp -s <(sed 2d "$1") <(sed 2d "$2"); }
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  case "${1:-}" in
    relations)
      for f in relations.txt keys.txt; do cmp -s "$O/relrev7_check/$b/$f" "$O/orderrev1_relcheck/$b/$f" && ok "$b $f same as relrev7_check" || bad "$b $f differs from relrev7_check"; done
      [ -s "$O/orderrev1_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do cmp -s "$O/order_rev1/$b/$f" "$O/order_rev1/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      [ -s "$O/order_rev1/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/order_rev1/$b/again" ;;
    extract)
      same_but_line2 "$O/order_rev1/$b/order.txt" "$O/order_rev1_on_extract/$b/order.txt" && ok "$b order.txt on the re-extraction same apart from line 2" || bad "$b order.txt on the re-extraction differs beyond line 2"
      cmp -s "$O/order_rev1/$b/edges.tsv" "$O/order_rev1_on_extract/$b/edges.tsv" && ok "$b edges.tsv on the re-extraction same" || bad "$b edges.tsv on the re-extraction differs"
      [ -d "$O/order_rev1_on_extract/$b/again" ] && rm -r "$O/order_rev1_on_extract/$b/again"
      same_but_line2 "$O/orderrev1_relcheck/$b/relations.txt" "$O/order_rev1_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt of the re-extraction same apart from line 2" || bad "$b relations.txt of the re-extraction differs beyond line 2" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" "$O/relrev7_plans/$b" "$O/order_rev1_plans/$b" | sed "s/^/$b /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    *) echo "usage: compare.sh relations|order|extract|plans" >&2; exit 64 ;;
  esac
done
exit $rc

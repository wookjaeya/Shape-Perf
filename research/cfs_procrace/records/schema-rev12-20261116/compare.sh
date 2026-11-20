#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration (plan/full_stage_results_20261116.md 4.0), one step at a time:
#   relations  schema12_relcheck/<b>/{relations.txt,keys.txt} byte-identical to orderrev1b_relcheck/<b>/ (kz-static b52db538)
#              and to relrev7_check/<b>/, relations.log empty
#   order      schema12_order/<b>/{order.txt,edges.tsv} byte-identical to the second run (again/, removed after) and to
#              order_rev1b/<b>/ (b52db538), order.log empty
#   extract    check_extract.py on schema12_extract/<b>/<b>/NP against tests_8c_fix/<b>/NP (record by record);
#              compare_build.py with extract_allow.tsv (file level, the practice of the earlier steps); the
#              re-extraction's relations.txt equal to schema12_relcheck/<b>/relations.txt apart from line 2; order on the
#              re-extracted facts (schema12_on_extract/<b>/) equal to schema12_order/<b>/ apart from line 2 of order.txt,
#              edges.tsv byte-identical
#   plans      compare_plans.py order_rev1b_plans/<b> schema12_plans/<b>
# One line per check; rc 0 when every check holds, 1 otherwise.  The kz-dump is the build tree's (MC).
set -uo pipefail
O=/home/user/work/redesign/obs1
T=/home/user/work/redesign/step2a/tests_8c_fix
R=/home/user/Shape-Perf/research/cfs_procrace
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
same_but_line2() { cmp -s <(sed 2d "$1") <(sed 2d "$2"); }
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  case "${1:-}" in
    relations)
      for f in relations.txt keys.txt; do
        cmp -s "$O/orderrev1b_relcheck/$b/$f" "$O/schema12_relcheck/$b/$f" && ok "$b $f same as orderrev1b_relcheck" || bad "$b $f differs from orderrev1b_relcheck"
        cmp -s "$O/relrev7_check/$b/$f" "$O/schema12_relcheck/$b/$f" && ok "$b $f same as relrev7_check" || bad "$b $f differs from relrev7_check"
      done
      [ -s "$O/schema12_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do
        cmp -s "$O/schema12_order/$b/$f" "$O/schema12_order/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"
        cmp -s "$O/order_rev1b/$b/$f" "$O/schema12_order/$b/$f" && ok "$b $f same as order_rev1b" || bad "$b $f differs from order_rev1b"
      done
      [ -s "$O/schema12_order/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/schema12_order/$b/again" ;;
    extract)
      N=$O/schema12_extract/$b/$b/NP
      python3 -I "$HERE/check_extract.py" "$KD" "$N" "$T/$b/NP" > "$HERE/compare/check_extract_$b.txt"; r=$?
      [ $r = 0 ] && ok "$b check_extract: only declared differences" || bad "$b check_extract rc $r (compare/check_extract_$b.txt)"
      python3 -I "$R/env/redesign/compare_build.py" "$KD" "$HERE/compare/extract_$b.tsv" "$N" "$T/$b/NP" --allow "$HERE/extract_allow.tsv" > "$HERE/compare/extract_$b.log" 2>&1; r=$?
      [ $r = 0 ] && ok "$b compare_build: not allowed 0" || bad "$b compare_build rc $r"
      same_but_line2 "$O/schema12_relcheck/$b/relations.txt" "$N/relations.txt" && ok "$b relations.txt of the re-extraction same apart from line 2" || bad "$b relations.txt of the re-extraction differs beyond line 2"
      same_but_line2 "$O/schema12_order/$b/order.txt" "$O/schema12_on_extract/$b/order.txt" && ok "$b order.txt on the re-extraction same apart from line 2" || bad "$b order.txt on the re-extraction differs beyond line 2"
      cmp -s "$O/schema12_order/$b/edges.tsv" "$O/schema12_on_extract/$b/edges.tsv" && ok "$b edges.tsv on the re-extraction same" || bad "$b edges.tsv on the re-extraction differs"
      [ -d "$O/schema12_on_extract/$b/again" ] && rm -r "$O/schema12_on_extract/$b/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" "$O/order_rev1b_plans/$b" "$O/schema12_plans/$b" | sed "s/^/$b /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    *) echo "usage: compare.sh relations|order|extract|plans" >&2; exit 64 ;;
  esac
done
exit $rc

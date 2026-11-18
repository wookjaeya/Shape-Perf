#!/usr/bin/env bash
# prebuild_regress.sh -- the default-path relations and order regression of the PRESENT kz-static (the tool of the manifest
# revision 4 build, 1a47abd0), which that step did not run (plan/full_stage_results_20261116.md 12.7 builds-05).  Three new
# output directories; every file is compared by cmp with the kept output of the loop flag and lock handle step:
#   rf2_pre_relcheck/<b>     relations --keys on the tests_8c_fix NP facts      = ll_relcheck/<b> (relations.txt, keys.txt)
#   rf2_pre_order/<b>        order --edges on the same facts                    = ll_order/<b> (order.txt, edges.tsv)
#   rf2_pre_final_order/<b>  order --edges --caller-dependent-report and relations --keys on the final option-on round
#                            ll_final_extract                                    = ll_final_order/<b> (five files)
# The logs must be empty.  Output: OK/BAD lines; exit 1 on any BAD.
set -uo pipefail
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
WANT=1a47abd05b8289bce90e6da8906d3b84d9bec48befc99634295d5350cd261086
O=/home/user/work/redesign/obs1
T=/home/user/work/redesign/step2a/tests_8c_fix
got=$(sha256sum "$KS" | cut -d' ' -f1)
[ "$got" = "$WANT" ] || { echo "BAD tool sha256 $got (expected $WANT)"; exit 1; }
echo "OK tool kz-static $got"
for d in rf2_pre_relcheck rf2_pre_order rf2_pre_final_order; do [ -e "$O/$d" ] && { echo "refusing: $O/$d exists" >&2; exit 2; }; done
rc=0
same() { if cmp -s "$1" "$2"; then echo "OK $3 same bytes"; else echo "BAD $3 differs"; rc=1; fi; }
empty() { if [ -e "$1" ] && [ ! -s "$1" ]; then echo "OK $2 empty"; else echo "BAD $2 not empty or missing"; rc=1; fi; }
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  A=$O/rf2_pre_relcheck/$b; mkdir -p "$A"
  "$KS" relations "$T/$b/NP/facts.kz" -o "$A/relations.txt" --keys "$A/keys.txt" > "$A/relations.log" 2>&1 || { echo "BAD $b relations rc"; rc=1; }
  same "$A/relations.txt" "$O/ll_relcheck/$b/relations.txt" "$b t8cf relations.txt"
  same "$A/keys.txt" "$O/ll_relcheck/$b/keys.txt" "$b t8cf keys.txt"
  empty "$A/relations.log" "$b t8cf relations.log"
  B=$O/rf2_pre_order/$b; mkdir -p "$B"
  "$KS" order "$T/$b/NP/facts.kz" -o "$B/order.txt" --edges "$B/edges.tsv" > "$B/order.log" 2>&1 || { echo "BAD $b order rc"; rc=1; }
  same "$B/order.txt" "$O/ll_order/$b/order.txt" "$b t8cf order.txt"
  same "$B/edges.tsv" "$O/ll_order/$b/edges.tsv" "$b t8cf edges.tsv"
  empty "$B/order.log" "$b t8cf order.log"
  C=$O/rf2_pre_final_order/$b; mkdir -p "$C"; F=$O/ll_final_extract/$b/$b/NP/facts.kz
  "$KS" order "$F" -o "$C/order.txt" --edges "$C/edges.tsv" --caller-dependent-report "$C/cdr.txt" > "$C/order.log" 2>&1 || { echo "BAD $b final order rc"; rc=1; }
  "$KS" relations "$F" -o "$C/relations.txt" --keys "$C/keys.txt" > "$C/relations.log" 2>&1 || { echo "BAD $b final relations rc"; rc=1; }
  for f in order.txt edges.tsv cdr.txt relations.txt keys.txt; do same "$C/$f" "$O/ll_final_order/$b/$f" "$b final $f"; done
  same "$C/relations.txt" "$O/ll_final_extract/$b/$b/NP/relations.txt" "$b final relations.txt = the re-extraction's"
  empty "$C/order.log" "$b final order.log"; empty "$C/relations.log" "$b final relations.log"
done
echo "prebuild_regress rc=$rc"
exit $rc

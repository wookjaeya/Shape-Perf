#!/usr/bin/env bash
# keys_check.sh -- the check of the relations rules revision 7 declaration (plan/full_stage_results_20261116.md 2.0) that
# was not run (results 2.7): the re-extraction's keys.  The preserved kz-static 038cd9e3 (the tool of that step) is
# restored from its xz copy into SCRATCH (round trip: stream sha256 = 038cd9e3...), run with --keys on the facts the step
# re-extracted (relrev7_extract/<b>/<b>/NP/facts.kz) into a new directory, and compared:
#   keys.txt       = relrev7_check/<b>/keys.txt apart from line 2 (the facts hash)
#   relations.txt  = relrev7_check/<b>/relations.txt apart from line 2, and = relrev7_extract/<b>/<b>/NP/relations.txt
#   relations.log  empty
# One line per check; rc 0 when every check holds.  The restored binary is removed at the end.
set -uo pipefail
O=/home/user/work/redesign/obs1
XZ=/home/user/work/preserve/tools-20261116-d/kz-static.038cd9e3.xz
WANT=038cd9e36af4d4ded9f8801346405f0ceaa38025a2d603ac35974ea48097d0ce
SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the restored tool}
OUT=$O/revfix_relrev7_extract_keys
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
KS=$SCRATCH/kz-static.038cd9e3
xz -dc "$XZ" > "$KS" && chmod +x "$KS" || { echo "BAD restore"; exit 1; }
got=$(sha256sum "$KS" | cut -d' ' -f1)
[ "$got" = "$WANT" ] && ok "restored kz-static sha256 $got" || { bad "restored kz-static sha256 $got != $WANT"; rm -f "$KS"; exit 1; }
mkdir -p "$OUT"
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  mkdir -p "$OUT/$b"
  "$KS" relations "$O/relrev7_extract/$b/$b/NP/facts.kz" -o "$OUT/$b/relations.txt" --keys "$OUT/$b/keys.txt" > "$OUT/$b/relations.log" 2>&1 || bad "$b relations rc $?"
  for f in keys.txt relations.txt; do
    cmp -s <(sed 2d "$O/relrev7_check/$b/$f") <(sed 2d "$OUT/$b/$f") && ok "$b $f = relrev7_check apart from line 2" || bad "$b $f differs from relrev7_check beyond line 2"
  done
  cmp -s "$OUT/$b/relations.txt" "$O/relrev7_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = relrev7_extract's (the single-build runner's)" || bad "$b relations.txt differs from relrev7_extract's"
  sed -n 2p "$OUT/$b/keys.txt" | grep -q "^facts $(sed -n 2p "$O/relrev7_extract/$b/$b/NP/relations.txt" | cut -d' ' -f2)$" && ok "$b keys.txt line 2 names the re-extracted facts" || bad "$b keys.txt line 2"
  [ -s "$OUT/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty"
done
rm -f "$KS"
exit $rc

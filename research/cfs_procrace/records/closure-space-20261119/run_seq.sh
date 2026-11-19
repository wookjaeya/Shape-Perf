#!/usr/bin/env bash
# run_seq.sh FROM TO -- the archives of chosen.tsv lines FROM..TO (tier 1 and 2 only), one directory at a time, in the
# committed order (results 26.2).  Before each line: if the free space (df -B1 /) has reached target_free of need.txt,
# stop (rc 0, "need met").  For each line:
#   1. archive.sh NAME LIST under space_guard.sh (need 20,000,000 B; log guard/a_<seq>_<name>.log); rc 0 required
#   2. ledger_rewrite.py archive ROW NAME into a proposed ledger; ledger_check.sh on it (ledger/check_<seq>_<name>.txt);
#      rc 0 required; only then the proposed ledger replaces records/LEDGER.tsv (cmp after the copy)
#   3. one line in progress.tsv: seq, name, row, free before, free after, archive bytes, the guard's lowest free
# The first failure stops the loop with rc 1 and nothing further is touched.  The ledger list of a row is read from the
# ledger (column 4), never given by hand.
set -uo pipefail
FROM=${1:?from}; TO=${2:?to}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
LED=$R/records/LEDGER.tsv
TMP=${SPACE_TMP:?set SPACE_TMP to a scratch directory}
free() { df -B1 --output=avail / | tail -1 | tr -d ' '; }
T=$(sed -n 's/^target_free \([0-9]*\)$/\1/p' "$HERE/need.txt"); [ -n "$T" ] || { echo "no target_free" >&2; exit 2; }
mkdir -p "$HERE/ledger" "$HERE/guard" "$TMP" || exit 2
[ -f "$HERE/progress.tsv" ] || printf 'seq\tname\trow\tfree_before\tfree_after\tarchive_bytes\tguard_lowest\tledger_check\n' > "$HERE/progress.tsv"
for s in $(seq "$FROM" "$TO"); do
  line=$(awk -F'\t' -v s="$s" '$1==s' "$HERE/chosen.tsv"); [ -n "$line" ] || { echo "no line $s" >&2; exit 1; }
  tier=$(cut -f2 <<< "$line"); dir=$(cut -f3 <<< "$line"); row=$(cut -f4 <<< "$line"); name=${dir#redesign/obs1/}
  case "$tier" in 1|2) ;; *) echo "line $s is tier $tier: not archived by this script" >&2; exit 1 ;; esac
  f0=$(free)
  if [ "$f0" -ge "$T" ]; then echo "need met before line $s: free $f0 >= target $T"; exit 0; fi
  list=$(awk -F'\t' -v r="$row" '$1==r {print $4}' "$LED"); [ -n "$list" ] || { echo "no ledger list for $row" >&2; exit 1; }
  bash "$R/env/tools/space_guard.sh" --need 20000000 --kind archive --log "$HERE/guard/a_${s}_${name}.log" -- \
    bash "$HERE/archive.sh" "$name" "$list" > /dev/null; rc=$?
  [ $rc -eq 0 ] || { echo "line $s $name: archive rc $rc (see archive/roundtrip_$name.txt, guard/a_${s}_${name}.log)" >&2; exit 1; }
  python3 -I "$HERE/ledger_rewrite.py" "$LED" "$TMP/LEDGER_proposed.tsv" archive "$row" "$name" >> "$HERE/ledger/rewrite.txt" || { echo "line $s: rewrite failed" >&2; exit 1; }
  bash "$R/env/tools/ledger_check.sh" "$TMP/LEDGER_proposed.tsv" > "$HERE/ledger/check_${s}_${name}.txt"; lc=$?
  [ $lc -eq 0 ] || { echo "line $s: ledger_check of the proposed ledger rc $lc (ledger/check_${s}_${name}.txt)" >&2; exit 1; }
  cp "$TMP/LEDGER_proposed.tsv" "$LED" && cmp -s "$TMP/LEDGER_proposed.tsv" "$LED" || { echo "line $s: copy of the ledger failed" >&2; exit 1; }
  rm -f "$TMP/LEDGER_proposed.tsv"
  f1=$(free); z=$(stat -c %s "/home/user/work/preserve/archives-20261119/$name/$name.tar.xz")
  low=$(sed -n 's/^end .* lowest=\([0-9]*\).*/\1/p' "$HERE/guard/a_${s}_${name}.log" | tail -1)
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$s" "$name" "$row" "$f0" "$f1" "$z" "$low" "$(tail -1 "$HERE/ledger/check_${s}_${name}.txt")" >> "$HERE/progress.tsv"
  echo "line $s $name: archived ($z bytes), ledger rewritten, free $f0 -> $f1"
done
f=$(free); if [ "$f" -ge "$T" ]; then echo "need met after line $TO: free $f >= target $T"; else echo "lines $FROM-$TO done; free $f < target $T (short $((T - f)))"; fi

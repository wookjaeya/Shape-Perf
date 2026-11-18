#!/usr/bin/env bash
# reclaim.sh ROOT SUMMARY -- remove the candidate directories that SUMMARY (identity_summary_<tag>.txt of
# identity_listing.py) calls identical-no-extra.  ROOT must be /home/user/work/redesign/obs1.
# Immediately before each removal the candidate is compared with its reference again (diff -rq: every file of
# both, byte by byte, and no entry on one side only; no symbolic link in the candidate); a candidate that is not
# the same any more is left alone and the script goes on to rc 1.  One line per candidate with the free bytes
# (df -B1 /) before and after; a summary line.  rc 0 when every eligible candidate was removed.
set -uo pipefail
ROOT=${1:-}; SUM=${2:-}
[ "$ROOT" = /home/user/work/redesign/obs1 ] && [ -f "$SUM" ] || { echo "usage: reclaim.sh /home/user/work/redesign/obs1 SUMMARY" >&2; exit 64; }
free() { df -B1 --output=avail / | tail -1 | tr -d ' '; }
n=0; nr=0; nf=0; f0=$(free)
echo "start $(date -u +%FT%TZ) free $f0"
while IFS=$'\t' read -r cand ref crow rrow files bytes rest; do
  case "$cand" in ''|'#'*) continue ;; esac
  verdict=${rest##*$'\t'}
  [ "$verdict" = identical-no-extra ] || { echo "LEFT $cand: $verdict"; continue; }
  n=$((n + 1)); c="$ROOT/$cand"; r="$ROOT/$ref"
  case "$cand$ref" in */*|*..*) echo "FAIL $cand: bad name"; nf=$((nf + 1)); continue ;; esac
  if [ -L "$c" ] || [ ! -d "$c" ] || [ -L "$r" ] || [ ! -d "$r" ] || [ "$c" -ef "$r" ]; then
    echo "FAIL $cand: candidate or reference is not a separate directory"; nf=$((nf + 1)); continue; fi
  links=$(find "$c" ! -type f ! -type d | wc -l); have=$(find "$c" -type f | wc -l)
  d=$(diff -rq "$c" "$r" 2>&1); drc=$?
  if [ $drc -ne 0 ] || [ -n "$d" ] || [ "$links" -ne 0 ] || [ "$have" -ne "$files" ]; then
    echo "FAIL $cand: not the same as $ref any more (diff rc $drc, other entries $links, files $have of $files)"; nf=$((nf + 1)); continue; fi
  b=$(free); rm -rf -- "$c"; rrc=$?; a=$(free)
  if [ $rrc -ne 0 ] || [ -e "$c" ]; then echo "FAIL $cand: rm rc $rrc"; nf=$((nf + 1)); continue; fi
  nr=$((nr + 1)); echo "REMOVED $cand ($crow): $files files $bytes bytes, same as $ref ($rrow) by diff -rq just before; free $b -> $a"
done < "$SUM"
f1=$(free)
echo "end $(date -u +%FT%TZ) free $f1 (change $((f1 - f0)))"
echo "reclaim: $n eligible, $nr removed, $nf failing"
[ $nf -eq 0 ] && [ $nr -eq $n ]

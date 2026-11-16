#!/usr/bin/env bash
# tools_check.sh [--deep] [REGISTRY] -- check the tool registry (default records/TOOLS.tsv beside this repository).
# For every row: in-place -> the file at location has the row's sha256; copy -> the .xz at copy exists and passes
# `xz -t`; in-place+copy -> both; gone -> nothing to read (listed).  --deep also streams every copy
# (xz -dc | sha256sum) and compares it with the row's sha256.  A duplicated sha256 row is a failure.
# One line per row (OK / FAIL / LISTED) and a summary; rc 0 when no row fails, 1 otherwise, 64 on usage.  Reads only.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEEP=0
[ "${1:-}" = --deep ] && { DEEP=1; shift; }
REG=${1:-$HERE/../../records/TOOLS.tsv}
[ -f "$REG" ] || { echo "usage: tools_check.sh [--deep] [REGISTRY]" >&2; exit 64; }
nf=0; n=0
dups=$(grep -vE '^(#|sha256\b)' "$REG" | cut -f1 | sort | uniq -d)
[ -z "$dups" ] || { echo "FAIL duplicated sha256 rows: $dups"; nf=$((nf + 1)); }
while IFS=$'\t' read -r h tool state loc copy first note; do
  case "$h" in ''|'#'*|sha256) continue ;; esac
  n=$((n + 1)); msg=; bad=0
  case "$h" in *[!0-9a-f]*) bad=1; msg+=" bad-sha256" ;; esac
  [ ${#h} -eq 64 ] || { bad=1; msg+=" bad-sha256-length"; }
  case "$state" in
    in-place|in-place+copy)
      if [ -f "$loc" ]; then [ "$(sha256sum < "$loc" | cut -c1-64)" = "$h" ] && msg+=" file=same" || { bad=1; msg+=" file=DIFFERS"; }
      else bad=1; msg+=" file=MISSING"; fi ;;
  esac
  case "$state" in
    copy|in-place+copy)
      if [ -f "$copy" ]; then
        if xz -t "$copy" 2> /dev/null; then msg+=" xz-t=ok"; else bad=1; msg+=" xz-t=FAIL"; fi
        if [ $DEEP -eq 1 ]; then [ "$(xz -dc "$copy" | sha256sum | cut -c1-64)" = "$h" ] && msg+=" stream=same" || { bad=1; msg+=" stream=DIFFERS"; }; fi
      else bad=1; msg+=" copy=MISSING"; fi ;;
    gone) msg+=" listed (no file, no copy)" ;;
    in-place) ;;
    *) bad=1; msg+=" unknown-state=$state" ;;
  esac
  if [ "$state" = gone ]; then echo "LISTED ${h:0:16} $tool$msg"
  elif [ $bad -eq 0 ]; then echo "OK ${h:0:16} $tool $state$msg"
  else echo "FAIL ${h:0:16} $tool $state$msg"; nf=$((nf + 1)); fi
done < "$REG"
echo "tools_check: $n rows, $nf failing$([ $DEEP -eq 1 ] && echo ' (deep)')"
[ $nf -eq 0 ]

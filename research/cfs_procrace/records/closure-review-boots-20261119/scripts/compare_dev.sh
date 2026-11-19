#!/usr/bin/env bash
# compare_dev.sh DEV COMMITTED -- the two development runs of report27.py (DEV/dev1, DEV/dev2, kept copies) against the
# committed run (COMMITTED, records/closure-report-20261119/out): for every file name of the three, its sha256 and line
# count in each ('-' when absent), and for a file of a development run that differs from the committed one, the number of
# lines diff prints.  Reads only; writes to stdout.  rc 0; 2 on a usage error.
set -uo pipefail
[ $# -eq 2 ] && [ -d "$1/dev1" ] && [ -d "$1/dev2" ] && [ -d "$2" ] || { echo "usage: compare_dev.sh DEV COMMITTED" >&2; exit 2; }
DEV=$1; COM=$2
cell() { if [ -f "$1" ]; then printf '%s %s' "$(sha256sum < "$1" | cut -c1-16)" "$(wc -l < "$1")"; else printf -- '- -'; fi; }
dl() { if [ -f "$1" ] && [ -f "$2" ]; then if cmp -s "$1" "$2"; then printf 'same'; else printf 'differs (diff lines %s)' "$(diff "$1" "$2" | wc -l)"; fi; else printf 'absent in one'; fi; }
echo "# compare_dev.sh: file, then for dev1, dev2 and the committed run: sha256 (first 16) and lines; then dev1 and dev2 against the committed file"
echo "# dev = $DEV ; committed = $COM"
{ ls "$DEV/dev1"; ls "$DEV/dev2"; ls "$COM"; } | sort -u | while read -r f; do
  printf '%s\tdev1 %s\tdev2 %s\tcommitted %s\tdev1: %s\tdev2: %s\n' "$f" "$(cell "$DEV/dev1/$f")" "$(cell "$DEV/dev2/$f")" \
    "$(cell "$COM/$f")" "$(dl "$DEV/dev1/$f" "$COM/$f")" "$(dl "$DEV/dev2/$f" "$COM/$f")"
done
for d in dev1 dev2; do
  for p in "$DEV/$d"/*; do
    f=$(basename "$p")
    if [ -f "$COM/$f" ] && ! cmp -s "$p" "$COM/$f"; then
      echo "# $d/$f against the committed $f: the first 6 lines diff prints (each cut at 200 characters)"
      diff "$p" "$COM/$f" | head -6 | cut -c1-200 | sed 's/^/#   /'
    fi
  done
done
exit 0

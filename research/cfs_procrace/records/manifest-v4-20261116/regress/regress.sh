#!/usr/bin/env bash
# regress.sh PRINTERS OUT -- versions 1-3 parse as before: the two printers of build_printers.sh on every group of
# manifests (groups.sh), each output into OUT/old/<n>.txt and OUT/new/<n>.txt (stdout; the new printer's version 4
# summary goes to OUT/new/<n>.v4), and the comparison (cmp) per group into OUT/compare.txt.  OUT is a new directory.
# rc 0 when every group gives the same bytes.
set -uo pipefail
P=${1:?usage: regress.sh PRINTERS OUT}; O=${2:?}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O/old" "$O/new"
bash "$HERE/groups.sh" > "$O/groups.tsv" || exit 2
n=0; same=0; diff=0; err=0
while IFS=$'\t' read -r id files; do
  [ -n "$id" ] || continue
  # shellcheck disable=SC2086
  "$P/print_old" $files > "$O/old/$id.txt" 2> /dev/null
  # shellcheck disable=SC2086
  "$P/print_new" $files > "$O/new/$id.txt" 2> "$O/new/$id.v4"
  n=$((n + 1))
  grep -q '^error ' "$O/old/$id.txt" && err=$((err + 1))
  if cmp -s "$O/old/$id.txt" "$O/new/$id.txt"; then same=$((same + 1)); echo "SAME $id"; else diff=$((diff + 1)); echo "DIFFERS $id"; fi
done < "$O/groups.tsv" > "$O/compare.txt"
echo "groups $n same $same differ $diff (groups whose old parse is an error: $err)" >> "$O/compare.txt"
tail -1 "$O/compare.txt"
[ $diff -eq 0 ]

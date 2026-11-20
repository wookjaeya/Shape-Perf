#!/usr/bin/env bash
# copy.sh DIR -- xz copies of the four tools this build overwrites that have no copy yet (kz, kz-phi, kz-static), each with
# its round trip (xz -t, sha256 of the stream, cmp with the file in place), and the round trip of the existing copy of
# kz-dump; writes ROUNDTRIP.txt beside this script and ORIGINAL.sha256 / XZ_SHA256 in DIR.
set -uo pipefail
D=${1:?usage: copy.sh DIR}; MC=/home/user/work/merge-check/kz-llvm; HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT=$HERE/ROUNDTRIP.txt; rc=0
echo "# round trips of the tool copies before the schema revision 12 build ($(date -u +%FT%TZ))" > "$OUT"
for x in "kz:tools/kz/kz" "kz-phi:tools/kz-phi/kz-phi" "kz-static:tools/kz-static/kz-static"; do
  n=${x%%:*}; p=$MC/${x#*:}; h=$(sha256sum < "$p" | cut -c1-64); f=$D/$n.${h:0:8}.xz
  xz -9 -T4 -c "$p" > "$f" || rc=1
  t=$(xz -t "$f" 2>&1 && echo OK || echo FAIL); s=$(xz -dc "$f" | sha256sum | cut -c1-64)
  c=$(xz -dc "$f" | cmp - "$p" > /dev/null 2>&1 && echo SAME || echo DIFFERS)
  { echo "copy: $f ($(stat -c%s "$f") B)"; echo "xz -t: $t"; echo "sha256 of the xz file: $(sha256sum < "$f" | cut -c1-64)"
    echo "xz -dc | sha256sum: $s"; echo "in place: $h"; echo "xz -dc | cmp in place: $c"; } >> "$OUT"
  [ "$t" = OK ] && [ "$s" = "$h" ] && [ "$c" = SAME ] || rc=1
  echo "$h  $n" >> "$D/ORIGINAL.sha256"
done
P=/home/user/work/preserve/tools-20261109/kz-dump.05623054.xz; h=$(sha256sum < "$MC/tools/kz-dump/kz-dump" | cut -c1-64)
t=$(xz -t "$P" 2>&1 && echo OK || echo FAIL); s=$(xz -dc "$P" | sha256sum | cut -c1-64)
c=$(xz -dc "$P" | cmp - "$MC/tools/kz-dump/kz-dump" > /dev/null 2>&1 && echo SAME || echo DIFFERS)
{ echo "existing copy: $P ($(stat -c%s "$P") B; not copied again)"; echo "xz -t: $t"; echo "xz -dc | sha256sum: $s"; echo "in place: $h"; echo "xz -dc | cmp in place: $c"; } >> "$OUT"
[ "$t" = OK ] && [ "$s" = "$h" ] && [ "$c" = SAME ] || rc=1
(cd "$D" && sha256sum *.xz) > "$D/XZ_SHA256"
exit $rc

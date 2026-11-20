#!/usr/bin/env bash
# copy.sh DIR -- the xz copy of the kz-static this build overwrites (MC tools/kz-static/kz-static), with its round trip
# (xz -t, sha256 of the stream, cmp with the file in place); writes ROUNDTRIP.txt beside this script and ORIGINAL.sha256 /
# XZ_SHA256 in DIR (a new directory).
set -uo pipefail
D=${1:?usage: copy.sh DIR}; MC=/home/user/work/merge-check/kz-llvm; HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[ -e "$D" ] && { echo "refusing: $D exists" >&2; exit 2; }
mkdir -p "$D"
OUT=$HERE/ROUNDTRIP.txt; rc=0
echo "# round trip of the tool copy before the build of the types and object access step ($(date -u +%FT%TZ))" > "$OUT"
p=$MC/tools/kz-static/kz-static; h=$(sha256sum < "$p" | cut -c1-64); f=$D/kz-static.${h:0:8}.xz
xz -9 -T4 -c "$p" > "$f" || rc=1
t=$(xz -t "$f" 2>&1 && echo OK || echo FAIL); s=$(xz -dc "$f" | sha256sum | cut -c1-64)
c=$(xz -dc "$f" | cmp - "$p" > /dev/null 2>&1 && echo SAME || echo DIFFERS)
{ echo "copy: $f ($(stat -c%s "$f") B)"; echo "xz -t: $t"; echo "sha256 of the xz file: $(sha256sum < "$f" | cut -c1-64)"
  echo "xz -dc | sha256sum: $s"; echo "in place: $h"; echo "xz -dc | cmp in place: $c"; } >> "$OUT"
[ "$t" = OK ] && [ "$s" = "$h" ] && [ "$c" = SAME ] || rc=1
echo "$h  kz-static" > "$D/ORIGINAL.sha256"
(cd "$D" && sha256sum *.xz) > "$D/XZ_SHA256"
exit $rc

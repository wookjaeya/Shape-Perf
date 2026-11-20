#!/usr/bin/env bash
# copy.sh DIR -- the xz copy of the tool this build overwrites in place (MC kz-static bf2ec2ee; no other tool of the MC
# tree changes identity: prebuild/pre_mc.txt, prebuild/relink_expected.sha256), with its round trip (xz -t, sha256 of the stream, cmp with the
# file in place); writes ROUNDTRIP.txt beside this script and ORIGINAL.sha256 / XZ_SHA256 in DIR (a new directory).
set -uo pipefail
D=${1:?usage: copy.sh DIR}; MC=/home/user/work/merge-check/kz-llvm; HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[ -e "$D" ] && { echo "refusing: $D exists" >&2; exit 2; }
[ -e "$HERE/ROUNDTRIP.txt" ] && { echo "refusing: $HERE/ROUNDTRIP.txt exists" >&2; exit 2; }
mkdir -p "$D"
OUT=$HERE/ROUNDTRIP.txt; rc=0
echo "# round trip of the tool copy before the declared kz-static closure build (results 21.0) ($(date -u +%FT%TZ))" > "$OUT"
: > "$D/ORIGINAL.sha256"
for rel in tools/kz-static/kz-static; do
  p=$MC/$rel; n=$(basename "$rel"); h=$(sha256sum < "$p" | cut -c1-64); f=$D/$n.${h:0:8}.xz
  [ "$h" = bf2ec2eedc756d72107a16532fa74f784cc7a1fd047d48edce62af20302fd460 ] || { echo "refusing: $p is not bf2ec2ee ($h)" >&2; exit 2; }
  xz -9 -T4 -c "$p" > "$f" || rc=1
  t=$(xz -t "$f" 2>&1 && echo OK || echo FAIL); s=$(xz -dc "$f" | sha256sum | cut -c1-64)
  c=$(xz -dc "$f" | cmp - "$p" > /dev/null 2>&1 && echo SAME || echo DIFFERS)
  { echo "copy: $f ($(stat -c%s "$f") B)"; echo "xz -t: $t"; echo "sha256 of the xz file: $(sha256sum < "$f" | cut -c1-64)"
    echo "xz -dc | sha256sum: $s"; echo "in place: $h"; echo "xz -dc | cmp in place: $c"; } >> "$OUT"
  [ "$t" = OK ] && [ "$s" = "$h" ] && [ "$c" = SAME ] || rc=1
  echo "$h  $n" >> "$D/ORIGINAL.sha256"
done
(cd "$D" && sha256sum *.xz) > "$D/XZ_SHA256"
exit $rc

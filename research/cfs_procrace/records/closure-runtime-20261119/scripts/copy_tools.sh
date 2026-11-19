#!/usr/bin/env bash
# copy_tools.sh DIR -- xz copies of the MC tools this build overwrites (kz, kz-rt-dump, kz-instr-clang) into the new
# directory DIR, each checked by a round trip (xz -dc | sha256sum = the original's sha256); ROUNDTRIP.txt lists them.
set -uo pipefail
D=${1:?usage: copy_tools.sh DIR}; MC=/home/user/work/merge-check/kz-llvm
[ -e "$D" ] && { echo "refusing: $D exists" >&2; exit 2; }
mkdir -p "$D"
rc=0
for t in tools/kz/kz runtime/kz-rt-dump instrument/clang/kz-instr-clang; do
  n=$(basename "$t"); h=$(sha256sum < "$MC/$t" | cut -c1-64)
  xz -9 -c "$MC/$t" > "$D/$n.${h:0:8}.xz" || rc=1
  r=$(xz -dc "$D/$n.${h:0:8}.xz" | sha256sum | cut -c1-64)
  x=$(sha256sum < "$D/$n.${h:0:8}.xz" | cut -c1-64)
  echo "$n original $h copy $n.${h:0:8}.xz xz_sha256 $x roundtrip $r $([ "$r" = "$h" ] && echo same || echo DIFFERENT)"
  [ "$r" = "$h" ] || rc=1
done > "$D/ROUNDTRIP.txt"
cat "$D/ROUNDTRIP.txt"
exit $rc

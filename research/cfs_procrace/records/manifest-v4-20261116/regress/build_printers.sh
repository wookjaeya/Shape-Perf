#!/usr/bin/env bash
# build_printers.sh DIR -- the two manifest printers of the regression check (manifest_print.cpp), each under the space
# guard: DIR/print_old against the host parser of commit d56ca0d (the last commit before the manifest revision 4 step;
# its two parser files are written into DIR/old/ by git show) and DIR/print_new against the parser of the working tree
# (-DKZ_PRINT_V4).  DIR is a new scratch directory; the printers are research check programs, not Kairos tools.
set -uo pipefail
D=${1:?usage: build_printers.sh DIR}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; R="$(cd "$HERE/../../.." && pwd)"; K=$R/kairos
G=$R/env/tools/space_guard.sh; REPO="$(cd "$R/../.." && pwd)"
[ -e "$D" ] && { echo "refusing: $D exists" >&2; exit 2; }
mkdir -p "$D/old"
for f in kz_rt_manifest.hpp kz_rt_manifest.cpp; do
  git -C "$REPO" show "d56ca0d:research/cfs_procrace/kairos/runtime/tools/manifest/$f" > "$D/old/$f" || exit 2
done
sha256sum "$D/old/"* "$K/runtime/tools/manifest/kz_rt_manifest.hpp" "$K/runtime/tools/manifest/kz_rt_manifest.cpp" > "$D/sources.sha256"
rc=0
"$G" --need 100000000 --log "$D/df_print_old.log" --kind scratch-compile -- g++ -std=c++17 -O1 -I"$K/runtime/include" -I"$D/old" \
  "$HERE/manifest_print.cpp" "$D/old/kz_rt_manifest.cpp" -o "$D/print_old" || rc=1
"$G" --need 100000000 --log "$D/df_print_new.log" --kind scratch-compile -- g++ -std=c++17 -O1 -DKZ_PRINT_V4 -I"$K/runtime/include" -I"$K/runtime/tools/manifest" \
  "$HERE/manifest_print.cpp" "$K/runtime/tools/manifest/kz_rt_manifest.cpp" -o "$D/print_new" || rc=1
exit $rc

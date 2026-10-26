#!/usr/bin/env bash
# build_runtime.sh <dir> <manifest> [<plugin.so>] -- the target runtime of a derived (kzgcc) build: kairos/runtime built on its
# own with the target gcc (never with the plugin; event_semantics.md 8.7), and the build identity object kzrt-build-info
# (src/kz_rt_build_info.c with -D values, event_semantics.md 8.5) for the manifest the plugin reads.  Tool capacities are
# the runtime's defaults (KZ_RT_DEFINES not given; they are recorded in every dump).  Pilot/dev tooling.
#   backend   "kzgcc <version> (gcc-plugin)", the plugin's own version string (strings of <plugin.so>, default the
#             plugin build next to <dir>), so the RUN record names the binding that compiled the units
#   compiler  the first line of `gcc --version`;  target  `gcc -dumpmachine`;  manifest  <manifest> (its sha256)
# Writes <dir>/runtime.record (values, sha256 of the libraries and objects the builds link).
set -uo pipefail
D=${1:?usage: build_runtime.sh <dir> <manifest> [<plugin.so>]}; MAN=${2:?manifest}; PLUGIN=${3:-}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RTSRC="$(cd "$HERE/../../../runtime" && pwd)"
CC=/usr/bin/gcc
VER="kzgcc 0.2"
if [ -n "$PLUGIN" ]; then
  VER=$(strings "$PLUGIN" | grep -E '^kzgcc [0-9]+\.[0-9]+$' | sort -u)
  [ -n "$VER" ] && [ "$(echo "$VER" | wc -l)" = 1 ] || { echo "no single version string in $PLUGIN" >&2; exit 2; }
fi
BACKEND="$VER (gcc-plugin)"
COMPILER=$("$CC" --version | head -1)
TARGET=$("$CC" -dumpmachine)
[ ! -e "$D" ] || { echo "refusing: $D exists" >&2; exit 2; }
cmake -S "$RTSRC" -B "$D" -DCMAKE_C_COMPILER="$CC" -DKZ_RT_BUILD_BACKEND="$BACKEND" -DKZ_RT_BUILD_COMPILER="$COMPILER" \
      -DKZ_RT_BUILD_TARGET="$TARGET" -DKZ_RT_BUILD_MANIFEST="$MAN" > "$D.configure.log" 2>&1 || { tail -5 "$D.configure.log"; exit 1; }
cmake --build "$D" -j2 > "$D.build.log" 2>&1 || { tail -5 "$D.build.log"; exit 1; }
{
  echo "# build_runtime.sh $(date -u +%FT%TZ): kairos/runtime at $RTSRC, standalone, $CC"
  echo "build identity: backend=\"$BACKEND\" compiler=\"$COMPILER\" target=\"$TARGET\" manifest=$MAN sha256 $(sha256sum "$MAN" | cut -c1-64)"
  echo "tool capacities: the runtime defaults (KZ_RT_DEFINES empty; recorded in the dump's RUN/RUNX records)"
  grep -E '^CMAKE_(C_FLAGS|BUILD_TYPE|C_COMPILER):' "$D/CMakeCache.txt" | sed 's/^/cache /'
  for f in libkzrt.a libkzrt-posix.a CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o CMakeFiles/kzrt-build-info.dir/src/kz_rt_build_info.c.o kz-rt-dump; do
    echo "$f sha256 $(sha256sum "$D/$f" | cut -c1-64)"; done
} > "$D/runtime.record"
cat "$D/runtime.record"

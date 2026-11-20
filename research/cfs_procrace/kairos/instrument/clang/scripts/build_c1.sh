#!/usr/bin/env bash
# build_c1.sh -- the derived (instrumented) build of C1 post 1df262a for the kz-instr-clang fitness check
# (docs/instr_candidate_clang.md).  The reference procedure is /home/user/work/procrace/c1_build/build_one.sh with
# prep_defs.sh (plan/C1_build_manifest.md section 3); this script runs the same commands on the transformed copy,
# with these differences only (each is recorded in <work>/build_diff.txt):
#   - the sources are the copy <work>/{bundle_1df262a,post/src} (no .git), with the files kz-instr-clang edited;
#   - CFLAGS adds -I<kairos>/runtime/include (the hook declarations) to the reference -Wno-error=restrict;
#   - cfe/cmake/target/CMakeLists.txt of the copy links the runtime into core-${TGTNAME} (whole archive + the
#     POSIX start-up object), so that the core and the loaded modules call one runtime per process.
#   build_c1.sh <work dir> <runtime build dir>
set -euo pipefail
C=$1; RT=$2
REF=/home/user/work/procrace/c1_build
KAIROS="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
[ -f "$C/xform/report.txt" ] || { echo "run kz-instr-clang first ($C/xform/report.txt)" >&2; exit 2; }
for f in libkzrt-posix.a libkzrt.a CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o; do [ -f "$RT/$f" ] || { echo "missing $RT/$f" >&2; exit 2; }; done
TGT="$C/bundle_1df262a/cfe/cmake/target/CMakeLists.txt"
if ! grep -q 'kairos instrumentation' "$TGT"; then
  cat >> "$TGT" <<EOT

# kairos instrumentation (derived build, kz-instr-clang): the runtime once per process, in the core executable;
# whole archive so that the hooks the loaded modules call are in the executable (exported by ENABLE_EXPORTS)
target_link_libraries(core-\${TGTNAME} -Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o)
EOT
fi
WS="$C/post/1df262a"
mkdir -p "$WS"
cd "$WS"
[ -d bpxfer_defs ] || bash "$REF/prep_defs.sh" "$C/bundle_1df262a" "$C/post/src" > prep_defs.log 2>&1
export CFS_APP_PATH="$C/post/src"
export CMAKE_EXPORT_COMPILE_COMMANDS=ON
export CFLAGS="-Wno-error=restrict -I$KAIROS/runtime/include"
{ echo "## cmake"; date -u +%FT%TZ
  cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S "$C/bundle_1df262a/cfe"; echo "cmake rc=$?"
  echo "## make"; date -u +%FT%TZ
  ( cd build && make DESTDIR="$WS" -j2 mission-install ); echo "make rc=$?"; date -u +%FT%TZ
} > build.log 2>&1 || true
grep -E '^(cmake|make) rc=' build.log

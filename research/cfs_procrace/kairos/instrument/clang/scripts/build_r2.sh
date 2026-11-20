#!/usr/bin/env bash
# build_r2.sh -- the derived (instrumented) build of the R2 ENV tree (cFS v7.0.1, native_std) for the kz-instr-clang
# fitness check (docs/instr_candidate_clang.md).  The reference is env/build_cfs.sh (README L104-105: make
# native_std.prep, make native_std.install, as the normal user CFS_NORMAL_USER).  Differences (recorded):
#   - the sources are the copy <work>/cFS (no .git, no build tree), with the files kz-instr-clang edited;
#   - CFLAGS=-I<kairos>/runtime/include (the reference build has no CFLAGS: build_cfs.sh refuses it);
#   - ENABLE_UNIT_TESTS=FALSE instead of TRUE (PREP_OPTS_native_std given on the make command line): the
#     coverage tests compile the same source files with stubs and would need the runtime too; they are not part of
#     the executable that runs.  The flight compile commands are compared with the reference (compare_build.sh);
#   - cfe/cmake/target/CMakeLists.txt of the copy links the runtime into core-${TGTNAME} (whole archive + start-up object).
#   build_r2.sh <work dir> <runtime build dir>      (as root; the build runs as CFS_NORMAL_USER, default ubuntu)
set -euo pipefail
W=$1; RT=$2; U=${CFS_NORMAL_USER:-ubuntu}
KAIROS="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
[ -f "$W/xform/report.txt" ] || { echo "run kz-instr-clang first ($W/xform/report.txt)" >&2; exit 2; }
TGT="$W/cFS/cfe/cmake/target/CMakeLists.txt"
if ! grep -q 'kairos instrumentation' "$TGT"; then
  cat >> "$TGT" <<EOT

# kairos instrumentation (derived build, kz-instr-clang): the runtime once per process, in the core executable;
# whole archive so that the hooks the loaded modules call are in the executable (exported by ENABLE_EXPORTS)
target_link_libraries(core-\${TGTNAME} -Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o)
EOT
fi
chown -R "$U:$U" "$W/cFS"
PREP="-DENABLE_UNIT_TESTS=FALSE -DSIMULATION=native -DCFE_EDS_ENABLED=OFF -DMISSIONCONFIG=sample -DCMAKE_BUILD_TYPE=debug"
for g in prep install; do
  ( cd "$W/cFS" && runuser -u "$U" -- env CFLAGS="-I$KAIROS/runtime/include" make "PREP_OPTS_native_std=$PREP" native_std.$g ) > "$W/build_$g.log" 2>&1
  echo "make native_std.$g rc=$?"
done

#!/usr/bin/env bash
# build_c1.sh -- the C1 post-fix build (CF 833fdbb + BP b051eb0, cFS candidate 1df262a) built with the clang of the
# pinned LLVM build, as a DERIVED configuration of the reference build /home/user/work/procrace/c1_build/post/1df262a,
# for the stage C fitness check of the LLVM pass plugin (kairos/docs/instr_candidate_llvm.md).  Pilot/dev only.
#
#   build_c1.sh prepare <W>                              copies of the reference inputs (the GCC candidate's prepare step)
#   build_c1.sh build <W> la                             L-A: the target code built with clang, no plugin
#   build_c1.sh build <W> li <kzllvm.so> <kzrt dir>      L-I: the same with the plugin and the runtime linked
#
# The commands are those of c1_build/build_one.sh (nasa/bp b051eb0 build-run-bp-common.yml L45-L46, L115-L131;
# plan/C1_build_manifest.md sections 2-3) with the same environment (CFS_APP_PATH, CMAKE_EXPORT_COMPILE_COMMANDS=ON
# = DV-P2, CFLAGS=-Wno-error=restrict = DV-P1).  After the top-level configure (which configures the arch build with
# the host gcc, as in the reference), the arch build build/native/default is configured again from an empty cache
# with the same arguments cFE passes (cfe/cmake/mission_build.cmake L438-L447) plus the compiler.  Differences of the
# derived configuration, all written into <W>/build_<variant>.record:
#   D1  work-space path <W> (path strings, as DV-P3)                                       [as the GCC candidate]
#   D2  copies without git links; module version table restored byte for byte              [as the GCC candidate]
#   D4  build date/user/host strings of cfe_build_env_table.c                               [as the GCC candidate]
#   L1  arch build compiler: CMAKE_C_COMPILER=<LLVM build>/bin/clang instead of /usr/bin/cc (gcc 13.3.0); the
#       mission build (host tools elf2cfetbl, cmdUtil) stays gcc
#   T1  flag translation: CMAKE_C_FLAGS += -Wno-unknown-warning-option (the reference flags name GCC-only warnings
#       -Wno-error=restrict, -Wno-stringop-truncation; clang warns about unknown warning options and -Werror would
#       turn that into an error).  Every other reference flag is passed unchanged.
#   T2  flag translation: -Wno-error= for the four warnings clang reports under the reference -Werror where gcc 13.3 is
#       silent (found with make -k on L-A, 20 errors): strict-prototypes (OSAL os-impl-tasks.c:695, a definition
#       without (void) after a prototype), cast-align (15 in bplib and CF; gcc warns only on strict-alignment targets),
#       unused-function (3, CF cf_codec.c static inline helpers), sometimes-uninitialized (1, bplib
#       v7_dataservice_api.c:653).  They stay warnings in the log; no code generation flag changes.
#   T3  build-system translation: CMAKE_PROJECT_INCLUDE=scripts/cmake_gnu_id_gates.cmake presents the compiler id as
#       GNU to the build's own compiler-id branches (cfe target L159: --whole-archive of the core libraries; with
#       the Clang id the core lacks 47 exported symbols and CF, BP, bplib, TO_LAB, SCH_LAB fail to load).  The
#       compile and link commands are then those of the reference apart from the compiler and T1/T2 (checked).
#   L2  (li) CMAKE_C_FLAGS += the plugin options; CMAKE_EXE_LINKER_FLAGS = the runtime (whole archives
#       libkzrt-posix.a libkzrt.a and the start object kz_rt_posix_auto.c.o)
set -uo pipefail
REF=/home/user/work/procrace/c1_build
CLANG=${KZ_LLVM_CLANG:-/home/user/work/llvm-project/build/bin/clang}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITES="$(cd "$HERE/../../gcc/sites" && pwd)/c1_post_1df262a.kzs"   # the logical sites shared by the three candidates
# T1 and T2 (diagnostics only; the code generation flags are those of the reference)
TRANS="-Wno-unknown-warning-option -Wno-error=strict-prototypes -Wno-error=cast-align -Wno-error=unused-function -Wno-error=sometimes-uninitialized"
CMD=${1:?usage}; W=${2:?usage}

case "$CMD" in
prepare)
  "$HERE/../../gcc/scripts/build_c1.sh" prepare "$W" ;;
build)
  VAR=${3:?la|li}
  cd "$W/ws" || exit 2
  [ ! -e build ] && [ ! -e exe ] || { echo "refusing: $W/ws/build or exe exists (remove the previous variant first)" >&2; exit 2; }
  export CFS_APP_PATH=$W/src
  export CMAKE_EXPORT_COMPILE_COMMANDS=ON
  export CFLAGS="-Wno-error=restrict"
  R="$W/build_$VAR.record"
  { echo "# build_c1.sh build $VAR $(date -u +%FT%TZ) user=$(id -un)"
    echo "host gcc: $(gcc --version | head -1)"; echo "clang: $CLANG $("$CLANG" --version | head -1) sha256 $(sha256sum "$(readlink -f "$CLANG")" | cut -c1-64)"
    echo "env CFS_APP_PATH=$CFS_APP_PATH CMAKE_EXPORT_COMPILE_COMMANDS=$CMAKE_EXPORT_COMPILE_COMMANDS CFLAGS=$CFLAGS"; } > "$R"
  { echo "## cmake"; date -u +%FT%TZ
    cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S $W/bundle_1df262a/cfe; echo "cmake rc=$?"
  } > "build_$VAR.log" 2>&1
  A=build/native/default
  TS=$(sed -n 's/^TARGETSYSTEM:[A-Z]*=//p' "$A/CMakeCache.txt")
  FL="$CFLAGS $TRANS"; LF=""
  if [ "$VAR" = li ]; then
    PLUGIN=${4:?kzllvm.so}; RT=${5:?kzrt build dir}
    OUT="$W/binds_li"; mkdir -p "$OUT"
    FL="$FL -fpass-plugin=$PLUGIN -mllvm -kzllvm-manifest=$SITES -mllvm -kzllvm-out=$OUT -mllvm -kzllvm-module=2:$W/src/cf/ -mllvm -kzllvm-module=3:$W/src/bp/ -mllvm -kzllvm-default-module=1"
    LF="-Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o"
  fi
  # L1: the arch build again from an empty cache, with cFE's own arguments (mission_build.cmake L438-L447)
  rm -rf "$A/CMakeCache.txt" "$A/CMakeFiles"
  { echo "## arch configure with clang (L1, T1-T3${LF:+, L2})"; ( cd "$A" && cmake -G "Unix Makefiles" -DTARGETSYSTEM="$TS" -DMISSION_BINARY_DIR="$W/ws/build" \
      -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/exe -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON \
      -DCMAKE_C_COMPILER="$CLANG" -DCMAKE_PROJECT_INCLUDE="$HERE/cmake_gnu_id_gates.cmake" -DCMAKE_C_FLAGS="$FL" ${LF:+-DCMAKE_EXE_LINKER_FLAGS="$LF"} "$W/bundle_1df262a/cfe" ); echo "arch configure rc=$?"; } >> "build_$VAR.log" 2>&1
  { echo "L1 arch CMAKE_C_COMPILER=$CLANG TARGETSYSTEM=$TS"; echo "T3 CMAKE_PROJECT_INCLUDE=$HERE/cmake_gnu_id_gates.cmake sha256 $(sha256sum "$HERE/cmake_gnu_id_gates.cmake" | cut -c1-64)"; echo "arch CMAKE_C_FLAGS=$FL"; echo "arch CMAKE_EXE_LINKER_FLAGS=$LF"; } >> "$R"
  if [ "$VAR" = li ]; then
    { echo "plugin sha256 $(sha256sum "$PLUGIN" | cut -c1-64) manifest sha256 $(sha256sum "$SITES" | cut -c1-64)"
      for f in libkzrt.a libkzrt-posix.a CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o; do echo "runtime $f sha256 $(sha256sum "$RT/$f" | cut -c1-64)"; done
      grep -E '^CMAKE_(C_FLAGS|BUILD_TYPE|C_COMPILER):' "$RT/CMakeCache.txt" | sed 's/^/runtime cache /'; } >> "$R"
  fi
  { echo "## make"; date -u +%FT%TZ; ( cd build && make DESTDIR=$W/ws -j2 mission-install ); echo "make rc=$?"; date -u +%FT%TZ; } >> "build_$VAR.log" 2>&1
  grep -E '^(cmake|make|arch configure) rc=' "build_$VAR.log" | tee -a "$R"
  echo "compiler warnings in the log: $(grep -c 'warning:' "build_$VAR.log")" >> "$R"
  grep -E '^CMAKE_(C_COMPILER|C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE):' "$A/CMakeCache.txt" | sed 's/^/arch cache /' >> "$R"
  grep -E '^CMAKE_(C_COMPILER|C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE):' build/CMakeCache.txt | sed 's/^/mission cache /' >> "$R"
  for f in exe/tx/core-tx exe/rx/core-rx exe/tx/cf/cf.so exe/tx/cf/bp.so exe/rx/cf/cf.so exe/rx/cf/bp.so build/src/cfe_module_version_table.c build/src/cfe_build_env_table.c; do
    echo "$f sha256 $(sha256sum "$f" 2>/dev/null | cut -c1-64) (reference $(sha256sum "$REF/post/1df262a/$f" 2>/dev/null | cut -c1-64))"; done >> "$R"
  for f in exe/tx/cf/*.tbl exe/rx/cf/*.tbl; do echo "$f sha256 $(sha256sum "$f" | cut -c1-64) (reference $(sha256sum "$REF/post/1df262a/$f" 2>/dev/null | cut -c1-64))"; done >> "$R"
  ( cd "$A" && find . -name '*.o' -type f | sort | xargs sha256sum ) > "$W/objects_$VAR.sha256"
  echo "objects: $(wc -l < "$W/objects_$VAR.sha256")" >> "$R"
  # the compile commands with the compiler and the translation/plugin flags taken out, for the comparison with
  # the reference (every other flag must be the same)
  cp "$A/compile_commands.json" "$W/compile_commands_$VAR.json" ;;
*) echo "usage: build_c1.sh prepare <W> | build <W> la | build <W> li <kzllvm.so> <kzrt dir>" >&2; exit 64 ;;
esac

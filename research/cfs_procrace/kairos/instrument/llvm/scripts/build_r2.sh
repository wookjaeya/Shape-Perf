#!/usr/bin/env bash
# build_r2.sh -- the R2 build (ENV tree, cFS v7.0.1 088b2fa8, native_std) with the cpu1 target code built by the clang of
# the pinned LLVM build, as a DERIVED configuration of the reference build /home/user/work/procrace/cfs_ref/cFS/
# build-native_std, for the stage C fitness check of the LLVM pass plugin (kairos/docs/instr_candidate_llvm.md).
# Pilot/dev only.  Run as root; the build runs as CFS_NORMAL_USER (ubuntu) through runuser, as env/build_cfs.sh.
#
#   build_r2.sh prepare <W>                            copy of the ENV source tree (the GCC candidate's prepare step)
#   build_r2.sh build <W> la                           L-A: cpu1 arch built with clang, no plugin
#   build_r2.sh build <W> li <kzllvm.so> <kzrt dir>    L-I: the same with the plugin and the runtime linked
#
# Steps: make native_std.prep (README L104, env/build_cfs.sh) with the environment refusal list of env/build_cfs.sh
# (CONDITIONS.md C4); then the arch build native/default_cpu1 is configured again from an empty cache with the
# arguments cFE passes (cfe/cmake/mission_build.cmake L644-L655, values read from the cache the prep step wrote)
# plus the compiler; then make native_std.install (README L105).  Differences (recorded in <W>/build_<variant>.record):
#   D1, D2, D4   as the GCC candidate (tree path, copy without .git with the module version table restored, build strings)
#   L1  cpu1 arch build compiler: CMAKE_C_COMPILER=<LLVM build>/bin/clang instead of /usr/bin/cc (gcc 13.3.0); the cpu2
#       arch build and the mission build (host tools) stay gcc
#   T1  CMAKE_C_FLAGS += -Wno-unknown-warning-option (GCC-only warning options of the reference flags under -Werror)
#   T2  CMAKE_C_FLAGS += -Wno-error=<w> for the warnings clang reports under the reference -Werror where gcc is silent
#       (found with make -k on L-A; listed in TRANS below and in the record).  Diagnostics only.
#   T3  CMAKE_PROJECT_INCLUDE=scripts/cmake_gnu_id_gates.cmake: the build's compiler-id branches take the reference's
#       (GNU) branch; here osal/src/bsp/generic-linux/CMakeLists.txt L42 (unit-test coverage flags and gcov link)
#   T4  CMAKE_EXE_LINKER_FLAGS += -resource-dir=<profile runtime dir>: the unit-test executables built with clang's
#       gcov-compatible coverage need clang's profile runtime (libclang_rt.profile), which the pinned LLVM build does
#       not contain; it was built out of tree from the same LLVM source revision (compiler-rt, profile only).  The
#       option only changes where the clang driver looks for its runtime libraries at link time (it serves the 56
#       units built with --coverage, osal/src/bsp/generic-linux/build_options.cmake L16-L17).  T4b:
#       CMAKE_C_STANDARD_LIBRARIES = that libclang_rt.profile archive, the clang counterpart of the GNU branch's "gcov"
#       library (generic-linux/CMakeLists.txt L44) for the 220 units built with -fprofile-arcs: CMake appends it to the
#       end of every link line of cpu1; an executable without coverage objects pulls nothing from it (checked).
#   L2  (li) CMAKE_C_FLAGS += the plugin options (with -kzllvm-skip-coverage: unit-test coverage units are compiled as in
#       L-A); CMAKE_EXE_LINKER_FLAGS += the runtime (whole archives and the start object) for every executable of cpu1
set -uo pipefail
ENV_TREE=/home/user/work/procrace/cfs_ref/cFS
CLANG=${KZ_LLVM_CLANG:-/home/user/work/llvm-project/build/bin/clang}
PROFRT=${KZ_LLVM_PROFILE_RT:-/home/user/work/instr/llvm/crt-profile/resource}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITES="$(cd "$HERE/../../gcc/sites" && pwd)/r2_v701.kzs"   # the logical sites shared by the three candidates
# T1 and T2 (diagnostics only).  T2 found with make -k on L-A (66 errors): unit-test code header-guard 9,
# non-literal-null-conversion 13, strict-prototypes 4, uninitialized-const-pointer 34; flight code unused-function 4
# (CF cf_codec.c:169, :239, twice: cpu1 and its unit-test build), parentheses-equality 2 (CS cs_table_processing.c:932);
# then default-const-init-var-unsafe 3 (CF unit test cf_cmd_tests.c:1061, 1105, 1152), implicit-enum-enum-cast 1
# (CF unit test cf_utils_tests.c:670)
TRANS="-Wno-unknown-warning-option -Wno-error=header-guard -Wno-error=non-literal-null-conversion -Wno-error=strict-prototypes -Wno-error=uninitialized-const-pointer -Wno-error=unused-function -Wno-error=parentheses-equality -Wno-error=default-const-init-var-unsafe -Wno-error=implicit-enum-enum-cast"
U=${CFS_NORMAL_USER:-ubuntu}
CMD=${1:?usage}; W=${2:?usage}
[ "$(id -u)" = 0 ] || { echo "run as root (the build runs as $U through runuser)" >&2; exit 2; }
AS=(runuser -u "$U" --)
REFUSED_VARS="OMIT_DEPRECATED ENABLE_ASAN MISSIONCONFIG SIMULATION ENABLE_UNIT_TESTS CFS_APP_PATH BUILDDATE HOSTNAME
O ARCH PREP_OPTS PLATFORM CPUNAME BUILDTYPE DESTDIR PREP_SOURCE_DIR SUBTGT_PREFIX CFG GOAL STAMPFILE
MAKEFLAGS MFLAGS GNUMAKEFLAGS MAKEFILES CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS VERBOSE
GCC_EXEC_PREFIX COMPILER_PATH LIBRARY_PATH CPATH C_INCLUDE_PATH DEPENDENCIES_OUTPUT SUNPRO_DEPENDENCIES
SOURCE_DATE_EPOCH GCC_COMPARE_DEBUG LD_PRELOAD LD_LIBRARY_PATH GLIBC_TUNABLES"
for v in $REFUSED_VARS; do printenv "$v" > /dev/null 2>&1 && { echo "refused: $v is set (env/build_cfs.sh)" >&2; exit 4; }; done
for v in $(compgen -e | grep -E '^(CMAKE_|CTEST_)' || true); do echo "refused: $v is set" >&2; exit 4; done

case "$CMD" in
prepare)
  "$HERE/../../gcc/scripts/build_r2.sh" prepare "$W" ;;
build)
  VAR=${3:?la|li}
  cd "$W/cFS" || exit 2
  [ ! -e build-native_std ] || { echo "refusing: build-native_std exists (remove the previous variant first)" >&2; exit 2; }
  R="$W/build_$VAR.record"; L="$W/build_$VAR"
  { echo "# build_r2.sh build $VAR $(date -u +%FT%TZ) user=$U host gcc: $(gcc --version | head -1)"
    echo "clang: $CLANG $("$CLANG" --version | head -1) sha256 $(sha256sum "$(readlink -f "$CLANG")" | cut -c1-64)"
    echo "profile runtime (T4): $(sha256sum "$PROFRT"/lib/linux/libclang_rt.profile-x86_64.a | cut -c1-64)"
    echo "environment names passed to make (values not recorded): $("${AS[@]}" env | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort | paste -sd' ')"; } > "$R"
  t0=$(date +%s)
  "${AS[@]}" make native_std.prep > "$L.prep.log" 2>&1; echo "prep rc=$?" | tee -a "$R"
  A=build-native_std/native/default_cpu1
  cv() { sed -n "s/^$1:[A-Z]*=//p" "$A/CMakeCache.txt"; }
  TS=$(cv TARGETSYSTEM); MB=$(cv MISSION_BINARY_DIR); BT=$(cv CMAKE_BUILD_TYPE); IP=$(cv CMAKE_INSTALL_PREFIX); PP=$(cv CMAKE_PREFIX_PATH)
  EC=$(cv CMAKE_EXPORT_COMPILE_COMMANDS); EDS=$(cv CFE_EDS_ENABLED); RCF=$(cv CMAKE_C_FLAGS)
  echo "cpu1 arch cache of the prep step: TARGETSYSTEM=$TS MISSION_BINARY_DIR=$MB CMAKE_BUILD_TYPE=$BT CMAKE_INSTALL_PREFIX=$IP CMAKE_PREFIX_PATH=$PP CMAKE_EXPORT_COMPILE_COMMANDS=$EC CFE_EDS_ENABLED=$EDS CMAKE_C_FLAGS=$RCF" >> "$R"
  FL="$RCF $TRANS"; LF="-resource-dir=$PROFRT"
  if [ "$VAR" = li ]; then
    PLUGIN=${4:?kzllvm.so}; RT=${5:?kzrt build dir}
    OUT="$W/binds_li"; mkdir -p "$OUT"; chown "$U:$U" "$OUT"
    FL="$FL -fpass-plugin=$PLUGIN -mllvm -kzllvm-manifest=$SITES -mllvm -kzllvm-out=$OUT -mllvm -kzllvm-default-module=1 -mllvm -kzllvm-skip-coverage"
    LF="$LF -Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o"
  fi
  rm -rf "$A/CMakeCache.txt" "$A/CMakeFiles"
  ( cd "$A" && "${AS[@]}" cmake -G "Unix Makefiles" -DTARGETSYSTEM="$TS" -DMISSION_BINARY_DIR="$MB" -DCMAKE_BUILD_TYPE="$BT" \
      -DCMAKE_INSTALL_PREFIX="$IP" -DCMAKE_PREFIX_PATH="$PP" -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL="$EC" -DCFE_EDS_ENABLED:BOOL="$EDS" \
      -DCMAKE_C_COMPILER="$CLANG" -DCMAKE_PROJECT_INCLUDE="$HERE/cmake_gnu_id_gates.cmake" -DCMAKE_C_FLAGS="$FL" \
      -DCMAKE_EXE_LINKER_FLAGS="$LF" -DCMAKE_C_STANDARD_LIBRARIES="$PROFRT/lib/linux/libclang_rt.profile-x86_64.a" "$W/cFS/cfe" ) > "$L.archcache.log" 2>&1; echo "arch configure rc=$?" | tee -a "$R"
  { echo "L1 cpu1 CMAKE_C_COMPILER=$CLANG"; echo "T3 CMAKE_PROJECT_INCLUDE=$HERE/cmake_gnu_id_gates.cmake sha256 $(sha256sum "$HERE/cmake_gnu_id_gates.cmake" | cut -c1-64)"
    echo "cpu1 CMAKE_C_FLAGS=$FL"; echo "cpu1 CMAKE_EXE_LINKER_FLAGS=$LF"; echo "T4b cpu1 CMAKE_C_STANDARD_LIBRARIES=$PROFRT/lib/linux/libclang_rt.profile-x86_64.a"; } >> "$R"
  if [ "$VAR" = li ]; then
    { echo "plugin sha256 $(sha256sum "$PLUGIN" | cut -c1-64) manifest sha256 $(sha256sum "$SITES" | cut -c1-64)"
      for f in libkzrt.a libkzrt-posix.a CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o; do echo "runtime $f sha256 $(sha256sum "$RT/$f" | cut -c1-64)"; done; } >> "$R"
  fi
  "${AS[@]}" make ${KZ_MAKE_K:+-k} native_std.install > "$L.install.log" 2>&1; echo "install rc=$?" | tee -a "$R"
  echo "wall seconds $(( $(date +%s) - t0 ))" >> "$R"
  B=build-native_std
  for c in "$A/CMakeCache.txt" "$B/native/default_cpu2/CMakeCache.txt" "$B/CMakeCache.txt"; do
    grep -E '^CMAKE_(C_COMPILER|C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE|C_STANDARD_LIBRARIES):' "$c" | sed "s|^|$c: |" >> "$R"; done
  for f in exe/cpu1/core-cpu1 exe/cpu2/core-cpu2 src/cfe_module_version_table.c src/cfe_build_env_table.c; do
    echo "$f sha256 $(sha256sum "$B/$f" 2>/dev/null | cut -c1-64) (ENV $(sha256sum "$ENV_TREE/$B/$f" 2>/dev/null | cut -c1-64))"; done >> "$R"
  echo "compiler warnings in install log: $(grep -c 'warning:' "$L.install.log")" >> "$R"
  ( cd "$A" && find . -name '*.o' -type f | sort ) | while read -r o; do
    objcopy --strip-debug "$A/$o" "$W/.x.o"; echo "$(sha256sum "$A/$o" | cut -c1-64) $(sha256sum "$W/.x.o" | cut -c1-64) $o"; done > "$W/objects_$VAR.sha256"
  rm -f "$W/.x.o"
  echo "objects (cpu1): $(wc -l < "$W/objects_$VAR.sha256")" >> "$R"
  cp "$A/compile_commands.json" "$W/compile_commands_$VAR.json" ;;
*) echo "usage: build_r2.sh prepare <W> | build <W> la | build <W> li <kzllvm.so> <kzrt dir>" >&2; exit 64 ;;
esac

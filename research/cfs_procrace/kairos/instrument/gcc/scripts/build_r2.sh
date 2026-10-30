#!/usr/bin/env bash
# build_r2.sh -- the R2 build (ENV tree, cFS v7.0.1 088b2fa8, native_std) as a DERIVED configuration of the reference
# build /home/user/work/procrace/cfs_ref/cFS/build-native_std, for the stage C fitness check of the GCC plugin
# (kairos/docs/instr_candidate_gcc.md).  Pilot/dev only.  Run as root; the build runs as CFS_NORMAL_USER (ubuntu)
# through runuser, as env/build_cfs.sh (README L102 "as a normal user", CONDITIONS.md C11, D-1).
#
#   build_r2.sh prepare <W>                            copy the ENV source tree into <W>/cFS (once)
#   build_r2.sh build <W> ref                          README L104-L105 unchanged, in <W>/cFS
#   build_r2.sh build <W> instr <kzgcc.so> <kzrt dir>  the same with the plugin in the cpu1 arch build (G-I)
#
# Steps: make native_std.prep, make native_std.install (README L104-L105, env/build_cfs.sh), with the environment
# refusal list of env/build_cfs.sh (CONDITIONS.md C4).  Differences of the derived configuration (recorded in
# <W>/build_<variant>.record):
#   D1  tree path <W>/cFS instead of cfs_ref/cFS (path strings in __FILE__ and in the debug information)
#   D2  copy without .git and build-native_std; the module version table of the ENV build is installed byte for byte
#       by sample_defs/generate_module_version.cmake (cfe/cmake/mission_build.cmake L130-L137), the method decided
#       for the CORD copy on 2026-10-08 (results/R2_cord_v701.md section 7)
#   D3  (instr) arch build native/default_cpu1 cache, set between prep and install: CMAKE_C_FLAGS = the plugin options
#       (with skip-coverage: unit-test coverage units are compiled as in the reference), CMAKE_EXE_LINKER_FLAGS = the
#       runtime (whole archives and the start object) for every executable of that arch build; default_cpu2 and the
#       mission (host tools) build are unchanged
#   D4  build date/user/host strings (cfe_build_env_table.c, generated at build time)
set -uo pipefail
ENV_TREE=/home/user/work/procrace/cfs_ref/cFS
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITES="$(cd "$HERE/../sites" && pwd)/r2_v701.kzs"
U=${CFS_NORMAL_USER:-ubuntu}
CMD=${1:?usage}; W=${2:?usage}
[ "$(id -u)" = 0 ] || { echo "run as root (the build runs as $U through runuser)" >&2; exit 2; }
AS=(runuser -u "$U" --)
# env/build_cfs.sh REFUSED_VARS (CONDITIONS.md C4): the documented build sets none of these
REFUSED_VARS="OMIT_DEPRECATED ENABLE_ASAN MISSIONCONFIG SIMULATION ENABLE_UNIT_TESTS CFS_APP_PATH BUILDDATE HOSTNAME
O ARCH PREP_OPTS PLATFORM CPUNAME BUILDTYPE DESTDIR PREP_SOURCE_DIR SUBTGT_PREFIX CFG GOAL STAMPFILE
MAKEFLAGS MFLAGS GNUMAKEFLAGS MAKEFILES CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS VERBOSE
GCC_EXEC_PREFIX COMPILER_PATH LIBRARY_PATH CPATH C_INCLUDE_PATH DEPENDENCIES_OUTPUT SUNPRO_DEPENDENCIES
SOURCE_DATE_EPOCH GCC_COMPARE_DEBUG LD_PRELOAD LD_LIBRARY_PATH GLIBC_TUNABLES"
for v in $REFUSED_VARS; do printenv "$v" > /dev/null 2>&1 && { echo "refused: $v is set (env/build_cfs.sh)" >&2; exit 4; }; done
for v in $(compgen -e | grep -E '^(CMAKE_|CTEST_)' || true); do echo "refused: $v is set" >&2; exit 4; done

case "$CMD" in
prepare)
  [ ! -e "$W/cFS" ] || { echo "refusing: $W/cFS exists" >&2; exit 2; }
  mkdir -p "$W"
  ( cd "$(dirname "$ENV_TREE")" && tar --exclude=.git --exclude=cFS/build-native_std -cf - cFS ) | ( cd "$W" && tar -xf - )
  cp -p "$ENV_TREE/build-native_std/src/cfe_module_version_table.c" "$W/cFS/sample_defs/ref_module_version_table.c"
  cat > "$W/cFS/sample_defs/generate_module_version.cmake" <<'EOF'
# D2: installs the module version table of the ENV build (cfs_ref/cFS/build-native_std/src/cfe_module_version_table.c)
# byte for byte; the copy has no git metadata.  cFE uses this file instead of its git script when it exists in the
# mission defs directory (cfe/cmake/mission_build.cmake L130-L137); output path as cfe/cmake/generate_git_module_version.cmake.
configure_file(${CMAKE_CURRENT_LIST_DIR}/ref_module_version_table.c ${BIN}/src/cfe_module_version_table.c COPYONLY)
EOF
  chown -R "$U:$U" "$W/cFS"
  {
    echo "# prepare $(date -u +%FT%TZ): copy of $ENV_TREE without .git and build-native_std (tar --exclude)"
    echo "diff -r (excluding .git, build-native_std): $(diff -rq -x .git -x build-native_std "$ENV_TREE" "$W/cFS" | tr '\n' ' ')"
    echo "ref_module_version_table.c sha256 $(sha256sum "$W/cFS/sample_defs/ref_module_version_table.c" | cut -c1-64)"
  } > "$W/prepare.record"
  cat "$W/prepare.record" ;;
build)
  VAR=${3:?ref|instr}
  cd "$W/cFS" || exit 2
  [ ! -e build-native_std ] || { echo "refusing: build-native_std exists (remove the previous variant first)" >&2; exit 2; }
  R="$W/build_$VAR.record"; L="$W/build_$VAR"
  { echo "# build_r2.sh build $VAR $(date -u +%FT%TZ) user=$U gcc: $(gcc --version | head -1)"
    echo "environment names passed to make (values not recorded): $("${AS[@]}" env | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort | paste -sd' ')"; } > "$R"
  t0=$(date +%s)
  "${AS[@]}" make native_std.prep > "$L.prep.log" 2>&1; echo "prep rc=$?" | tee -a "$R"
  if [ "$VAR" = instr ]; then
    PLUGIN=${4:?kzgcc.so}; RT=${5:?kzrt build dir}
    OUT="$W/binds_instr"; mkdir -p "$OUT"; chown "$U:$U" "$OUT"
    PF="-fplugin=$PLUGIN -fplugin-arg-kzgcc-manifest=$SITES -fplugin-arg-kzgcc-out=$OUT -fplugin-arg-kzgcc-default-module=1 -fplugin-arg-kzgcc-skip-coverage"
    LF="-Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o"
    "${AS[@]}" cmake -DCMAKE_C_FLAGS="$PF" -DCMAKE_EXE_LINKER_FLAGS="$LF" build-native_std/native/default_cpu1 > "$L.archcache.log" 2>&1; echo "arch reconfigure rc=$?" | tee -a "$R"
    { echo "D3 arch default_cpu1 CMAKE_C_FLAGS=$PF"; echo "D3 arch default_cpu1 CMAKE_EXE_LINKER_FLAGS=$LF"
      echo "plugin sha256 $(sha256sum "$PLUGIN" | cut -c1-64) manifest sha256 $(sha256sum "$SITES" | cut -c1-64)"
      for f in libkzrt.a libkzrt-posix.a CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o; do echo "runtime $f sha256 $(sha256sum "$RT/$f" | cut -c1-64)"; done; } >> "$R"
  fi
  "${AS[@]}" make native_std.install > "$L.install.log" 2>&1; echo "install rc=$?" | tee -a "$R"
  echo "wall seconds $(( $(date +%s) - t0 ))" >> "$R"
  B=build-native_std
  for c in "$B/native/default_cpu1/CMakeCache.txt" "$B/native/default_cpu2/CMakeCache.txt" "$B/CMakeCache.txt"; do
    grep -E '^CMAKE_(C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE):' "$c" | sed "s|^|$c: |" >> "$R"; done
  for f in exe/cpu1/core-cpu1 exe/cpu2/core-cpu2 src/cfe_module_version_table.c src/cfe_build_env_table.c; do
    echo "$f sha256 $(sha256sum "$B/$f" | cut -c1-64) (ENV $(sha256sum "$ENV_TREE/$B/$f" 2>/dev/null | cut -c1-64))"; done >> "$R"
  echo "compiler warnings in install log: $(grep -c 'warning:' "$L.install.log")" >> "$R"
  # objects of the cpu1 arch build: raw sha256 and sha256 after objcopy --strip-debug (the DWARF producer string
  # names the plugin options in every unit compiled with them, -grecord-gcc-switches being GCC's default)
  T=$(mktemp -d)
  ( cd "$B/native/default_cpu1" && find . -name '*.o' -type f | sort ) | while read -r o; do
    objcopy --strip-debug "$B/native/default_cpu1/$o" "$T/x.o"
    echo "$(sha256sum "$B/native/default_cpu1/$o" | cut -c1-64) $(sha256sum "$T/x.o" | cut -c1-64) $o"; done > "$W/objects_$VAR.sha256"
  rm -rf "$T"
  echo "objects (cpu1): $(wc -l < "$W/objects_$VAR.sha256")" >> "$R" ;;
*) echo "usage: build_r2.sh prepare <W> | build <W> ref | build <W> instr <kzgcc.so> <kzrt dir>" >&2; exit 64 ;;
esac

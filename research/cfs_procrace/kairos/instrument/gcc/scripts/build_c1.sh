#!/usr/bin/env bash
# build_c1.sh -- the C1 post-fix build (CF 833fdbb + BP b051eb0, cFS candidate 1df262a) as a DERIVED
# configuration of the reference build /home/user/work/procrace/c1_build/post/1df262a, for the stage C fitness
# check of the GCC plugin (kairos/docs/instr_candidate_gcc.md).  Pilot/dev only.
#
#   build_c1.sh prepare <W>                           copy the reference inputs into <W> (once)
#   build_c1.sh build <W> ref                         the reference build, unchanged, in <W> (same path as the
#                                                     instrumented build, so objects compare byte for byte)
#   build_c1.sh build <W> instr <kzgcc.so> <kzrt dir> the instrumented build (G-I): the plugin on every unit of the
#                                                     arch build, the runtime linked into its executables
#
# The commands are those of c1_build/build_one.sh and prep_defs.sh (nasa/bp b051eb0 build-run-bp-common.yml
# L45-L46, L115-L131; plan/C1_build_manifest.md sections 2-3) with the same environment (CFS_APP_PATH,
# CMAKE_EXPORT_COMPILE_COMMANDS=ON = DV-P2, CFLAGS=-Wno-error=restrict = DV-P1).  Differences of the derived
# configuration, all written into <W>/build_<variant>.record:
#   D1  work-space path <W> instead of c1_build/post/1df262a (path strings, as DV-P3)
#   D2  copies without the git worktree links (.git files point into refs/, whose index git would rewrite);
#       the module version table is restored byte for byte from the reference build through cFE's documented
#       hook <mission defs>/generate_module_version.cmake (cfe/cmake/mission_build.cmake L125-L129), as decided
#       for the CORD copy on 2026-10-08 (results/R2_cord_v701.md section 7)
#   D3  (instr) arch build cache: CMAKE_C_FLAGS += the plugin options; CMAKE_EXE_LINKER_FLAGS = the runtime
#       (whole archives libkzrt-posix.a libkzrt.a and the start object kz_rt_posix_auto.c.o); the mission
#       build (host tools) is unchanged
#   D4  build date/user/host strings of cfe_build_env_table.c (generated at build time)
set -uo pipefail
REF=/home/user/work/procrace/c1_build
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITES="$(cd "$HERE/../sites" && pwd)/c1_post_1df262a.kzs"
CMD=${1:?usage}; W=${2:?usage}

case "$CMD" in
prepare)
  [ ! -e "$W/ws" ] || { echo "refusing: $W/ws exists" >&2; exit 2; }
  mkdir -p "$W/ws"
  # rsync is not installed; tar with --exclude keeps modes and times
  ( cd "$REF" && tar --exclude=.git -cf - bundle_1df262a ) | ( cd "$W" && tar -xf - )
  ( cd "$REF/post" && tar --exclude=.git -cf - src ) | ( cd "$W" && tar -xf - )
  ( cd "$REF/post/1df262a" && tar -cf - bpxfer_defs ) | ( cd "$W/ws" && tar -xf - )
  cp -p "$REF/post/1df262a/build/src/cfe_module_version_table.c" "$W/ws/bpxfer_defs/ref_module_version_table.c"
  cat > "$W/ws/bpxfer_defs/generate_module_version.cmake" <<'EOF'
# D2: installs the module version table of the reference build c1_build/post/1df262a byte for byte (the copy has no
# git metadata).  cFE uses this file instead of its git script when it exists in the mission defs directory
# (cfe/cmake/mission_build.cmake L125-L129).  Output path as cfe/cmake/generate_git_module_version.cmake.
configure_file(${CMAKE_CURRENT_LIST_DIR}/ref_module_version_table.c ${BIN}/src/cfe_module_version_table.c COPYONLY)
EOF
  {
    echo "# prepare $(date -u +%FT%TZ): copies of the reference inputs (tar --exclude=.git)"
    echo "diff -r reference bundle_1df262a (excluding .git): $(diff -rq -x .git "$REF/bundle_1df262a" "$W/bundle_1df262a" | wc -l) differences"
    echo "diff -r reference post/src (excluding .git): $(diff -rq -x .git "$REF/post/src" "$W/src" | wc -l) differences"
    echo "diff -r reference bpxfer_defs: $(diff -rq "$REF/post/1df262a/bpxfer_defs" "$W/ws/bpxfer_defs" | tr '\n' ' ')"
    echo "ref_module_version_table.c sha256 $(sha256sum "$W/ws/bpxfer_defs/ref_module_version_table.c" | cut -c1-64)"
  } > "$W/prepare.record"
  cat "$W/prepare.record" ;;
build)
  VAR=${3:?ref|instr}
  cd "$W/ws" || exit 2
  [ ! -e build ] && [ ! -e exe ] || { echo "refusing: $W/ws/build or exe exists (remove the previous variant first)" >&2; exit 2; }
  export CFS_APP_PATH=$W/src
  export CMAKE_EXPORT_COMPILE_COMMANDS=ON
  export CFLAGS="-Wno-error=restrict"
  R="$W/build_$VAR.record"
  { echo "# build_c1.sh build $VAR $(date -u +%FT%TZ) user=$(id -un) host gcc: $(gcc --version | head -1)"
    echo "env CFS_APP_PATH=$CFS_APP_PATH CMAKE_EXPORT_COMPILE_COMMANDS=$CMAKE_EXPORT_COMPILE_COMMANDS CFLAGS=$CFLAGS"; } > "$R"
  { echo "## cmake"; date -u +%FT%TZ
    cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S $W/bundle_1df262a/cfe; echo "cmake rc=$?"
  } > "build_$VAR.log" 2>&1
  if [ "$VAR" = instr ]; then
    PLUGIN=${4:?kzgcc.so}; RT=${5:?kzrt build dir}
    OUT="$W/binds_instr"; mkdir -p "$OUT"
    PF="-fplugin=$PLUGIN -fplugin-arg-kzgcc-manifest=$SITES -fplugin-arg-kzgcc-out=$OUT -fplugin-arg-kzgcc-module=2:$W/src/cf/ -fplugin-arg-kzgcc-module=3:$W/src/bp/ -fplugin-arg-kzgcc-default-module=1"
    LF="-Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o"
    { echo "## arch cache (D3)"; cmake -DCMAKE_C_FLAGS="$CFLAGS $PF" -DCMAKE_EXE_LINKER_FLAGS="$LF" build/native/default; echo "arch reconfigure rc=$?"; } >> "build_$VAR.log" 2>&1
    { echo "D3 arch CMAKE_C_FLAGS=$CFLAGS $PF"; echo "D3 arch CMAKE_EXE_LINKER_FLAGS=$LF"
      echo "plugin sha256 $(sha256sum "$PLUGIN" | cut -c1-64) manifest sha256 $(sha256sum "$SITES" | cut -c1-64)"
      for f in libkzrt.a libkzrt-posix.a CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o; do echo "runtime $f sha256 $(sha256sum "$RT/$f" | cut -c1-64)"; done
      grep -E '^CMAKE_(C_FLAGS|BUILD_TYPE|C_COMPILER):' "$RT/CMakeCache.txt" | sed 's/^/runtime cache /'; } >> "$R"
  fi
  { echo "## make"; date -u +%FT%TZ; ( cd build && make DESTDIR=$W/ws -j2 mission-install ); echo "make rc=$?"; date -u +%FT%TZ; } >> "build_$VAR.log" 2>&1
  grep -E '^(cmake|make|arch reconfigure) rc=' "build_$VAR.log" | tee -a "$R"
  grep -E '^CMAKE_(C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE):' build/native/default/CMakeCache.txt | sed 's/^/arch cache /' >> "$R"
  grep -E '^CMAKE_(C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE):' build/CMakeCache.txt | sed 's/^/mission cache /' >> "$R"
  for f in exe/tx/core-tx exe/rx/core-rx exe/tx/cf/cf.so exe/tx/cf/bp.so exe/rx/cf/cf.so exe/rx/cf/bp.so build/src/cfe_module_version_table.c build/src/cfe_build_env_table.c; do
    echo "$f sha256 $(sha256sum "$f" | cut -c1-64) (reference $(sha256sum "$REF/post/1df262a/$f" 2>/dev/null | cut -c1-64))"; done >> "$R"
  # every object of the arch build, for the comparison of the two variants
  ( cd build/native/default && find . -name '*.o' -type f | sort | xargs sha256sum ) > "$W/objects_$VAR.sha256"
  echo "objects: $(wc -l < "$W/objects_$VAR.sha256")" >> "$R" ;;
*) echo "usage: build_c1.sh prepare <W> | build <W> ref | build <W> instr <kzgcc.so> <kzrt dir>" >&2; exit 64 ;;
esac

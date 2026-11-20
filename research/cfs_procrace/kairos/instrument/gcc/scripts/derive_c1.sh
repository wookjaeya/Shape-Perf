#!/usr/bin/env bash
# derive_c1.sh -- the four C1 builds (pre/post x cFS bundle candidate 1df262a/1b0b338, plan/C1_build_manifest.md) as
# DERIVED configurations of the reference builds /home/user/work/procrace/c1_build/<CFG>/<CAND>, with the stage D site
# manifests kairos/profiles/c1-bp-b051eb0/instr/sites-cf<REV>.kzs (docs/instr_candidate_gcc.md section 11).  Pilot/dev only.
#
#   derive_c1.sh prepare <CFG> <CAND> <W>                      copy the reference inputs into <W> (once)
#   derive_c1.sh build <CFG> <CAND> <W> ref                    the reference build, unchanged, at <W> (the same path as the
#                                                              derived build, so objects compare byte for byte); its
#                                                              build tree is removed after the object hashes are written
#   derive_c1.sh build <CFG> <CAND> <W> instr <kzgcc.so> <rt>  the derived build (G-I0/G-I/G-I-H are runtime modes of it)
#   derive_c1.sh check <CFG> <CAND> <W>                        object comparison, binding merge, link-time hook check
# build instr and check take the plan inputs --manifest FILE --modules TABLE --exes LIST --path-map MAP (derive_args.sh)
# after the other arguments; a generated plan comes in through derive_plan.sh, which requires them.  prepare writes
# <W>/path_map.tsv: one line per tree it copies (reference directory, its copy in <W>), the map that carries the plan's
# reference paths (module prefixes, executables) into <W> (path_map.sh).  Without the plan inputs the fixed values
# below (hand manifest, modules 2:<W>/src/cf/ 3:<W>/src/bp/, default module 1 or 4, the six C1 executables) stay, only
# for reruns of the old hand manifests.
# CFG pre|post (CF 281a941 / 833fdbb), CAND 1df262a|1b0b338.  <rt>: build_runtime.sh <rt> <the CFG's manifest> <kzgcc.so>.
#
# The commands are those of c1_build/build_one.sh (nasa/bp b051eb0 build-run-bp-common.yml L45-L46, L127-L131) with its
# environment (CFS_APP_PATH, CMAKE_EXPORT_COMPILE_COMMANDS=ON = DV-P2, CFLAGS=-Wno-error=restrict = DV-P1).
# Differences of the derived configuration, all written into <W>/build_<variant>.record (as stage C, build_c1.sh):
#   D1  work-space path <W> instead of c1_build/<CFG>/<CAND> (path strings, as DV-P3)
#   D2  copies without the git worktree links; the module version table of the reference build is installed byte for
#       byte through cFE's documented hook <mission defs>/generate_module_version.cmake (cfe/cmake/mission_build.cmake
#       L125-L129), as decided for the CORD copy on 2026-10-08 (results/R2_cord_v701.md section 7)
#   D3  (instr) arch build cache native/default: CMAKE_C_FLAGS += the plugin options; CMAKE_EXE_LINKER_FLAGS = the
#       runtime (whole archives libkzrt-posix.a libkzrt.a, the start object kz_rt_posix_auto.c.o and the build identity
#       object kz_rt_build_info.c.o); the mission build (host tools) is unchanged
#   D4  build date/user/host strings of cfe_build_env_table.c (generated at build time)
# The module of the core is 1 for bundle 1df262a and 4 for 1b0b338 (the manifest declares both), cf.so 2, bp.so 3.
set -uo pipefail
REF=/home/user/work/procrace/c1_build
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
POS=(); OPT=()
for a in "$@"; do if [ ${#OPT[@]} -gt 0 ] || [ "${a#--}" != "$a" ]; then OPT+=("$a"); else POS+=("$a"); fi; done
set -- "${POS[@]}"
. "$HERE/derive_args.sh"
derive_plan_args "${OPT[@]}"
CMD=${1:?usage}; CFG=${2:?pre|post}; CAND=${3:?1df262a|1b0b338}; W=${4:?work dir}
case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "CFG is pre|post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a) MOD=1 ;; 1b0b338) MOD=4 ;; *) echo "CAND is 1df262a|1b0b338" >&2; exit 64 ;; esac
SITES="$KZ/profiles/c1-bp-b051eb0/instr/sites-cf$REV.kzs"
[ "$PLAN_MODE" = 1 ] && SITES=$PLAN_MANIFEST
RB="$REF/$CFG/$CAND"
rel_ws() { case "$1" in "$W/ws/"*) printf '%s\n' "${1#"$W/ws/"}" ;; *) printf '%s\n' "$1" ;; esac; }

case "$CMD" in
prepare)
  [ ! -e "$W/ws" ] || { echo "refusing: $W/ws exists" >&2; exit 2; }
  mkdir -p "$W/ws"
  ( cd "$REF" && tar --exclude=.git -cf - "bundle_$CAND" ) | ( cd "$W" && tar -xf - )
  ( cd "$REF/$CFG" && tar --exclude=.git -cf - src ) | ( cd "$W" && tar -xf - )
  ( cd "$RB" && tar -cf - bpxfer_defs ) | ( cd "$W/ws" && tar -xf - )
  # the copies above, as a path map: the reference build directory (bpxfer_defs; build and exe are made at the same
  # places under ws by the build steps below) and the two source trees
  { echo "# kz path map; derive_c1.sh prepare $CFG $CAND: the reference directories and their copies in $W"
    printf '%s/\t%s/\n' "$REF/bundle_$CAND" "$W/bundle_$CAND" "$REF/$CFG/src" "$W/src" "$RB" "$W/ws"; } > "$W/path_map.tsv"
  cp -p "$RB/build/src/cfe_module_version_table.c" "$W/ws/bpxfer_defs/ref_module_version_table.c"
  cat > "$W/ws/bpxfer_defs/generate_module_version.cmake" <<EOF
# D2: installs the module version table of the reference build c1_build/$CFG/$CAND byte for byte (the copy has no
# git metadata).  cFE uses this file instead of its git script when it exists in the mission defs directory
# (cfe/cmake/mission_build.cmake L125-L129).  Output path as cfe/cmake/generate_git_module_version.cmake.
configure_file(\${CMAKE_CURRENT_LIST_DIR}/ref_module_version_table.c \${BIN}/src/cfe_module_version_table.c COPYONLY)
EOF
  {
    echo "# derive_c1.sh prepare $CFG $CAND $(date -u +%FT%TZ): copies of the reference inputs (tar --exclude=.git)"
    echo "diff -r reference bundle_$CAND (excluding .git): $(diff -rq -x .git "$REF/bundle_$CAND" "$W/bundle_$CAND" | wc -l) differences"
    echo "diff -r reference $CFG/src (excluding .git): $(diff -rq -x .git "$REF/$CFG/src" "$W/src" | wc -l) differences"
    echo "diff -r reference bpxfer_defs: $(diff -rq "$RB/bpxfer_defs" "$W/ws/bpxfer_defs" | tr '\n' ' ')"
    echo "ref_module_version_table.c sha256 $(sha256sum "$W/ws/bpxfer_defs/ref_module_version_table.c" | cut -c1-64)"
    echo "path map $W/path_map.tsv sha256 $(sha256sum "$W/path_map.tsv" | cut -c1-64):"; grep -v '^#' "$W/path_map.tsv" | sed 's/^/  /'
  } > "$W/prepare.record"
  cat "$W/prepare.record" ;;
build)
  VAR=${5:?ref|instr}
  cd "$W/ws" || exit 2
  [ ! -e build ] && [ ! -e exe ] || { echo "refusing: $W/ws/build or exe exists" >&2; exit 2; }
  export CFS_APP_PATH=$W/src
  export CMAKE_EXPORT_COMPILE_COMMANDS=ON
  export CFLAGS="-Wno-error=restrict"
  R="$W/build_$VAR.record"
  { echo "# derive_c1.sh build $CFG $CAND $VAR $(date -u +%FT%TZ) user=$(id -un) host gcc: $(gcc --version | head -1)"
    echo "env CFS_APP_PATH=$CFS_APP_PATH CMAKE_EXPORT_COMPILE_COMMANDS=$CMAKE_EXPORT_COMPILE_COMMANDS CFLAGS=$CFLAGS"; } > "$R"
  { echo "## cmake"; date -u +%FT%TZ
    cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S "$W/bundle_$CAND/cfe"; echo "cmake rc=$?"
  } > "build_$VAR.log" 2>&1
  if [ "$VAR" = instr ]; then
    PLUGIN=${6:?kzgcc.so}; RT=${7:?runtime dir}
    OUT="$W/binds_instr"; mkdir -p "$OUT"
    if [ "$PLAN_MODE" = 1 ]; then
      MF=$(derive_module_flags "$W") || exit 64
      PF="-fplugin=$PLUGIN -fplugin-arg-kzgcc-manifest=$SITES -fplugin-arg-kzgcc-out=$OUT $MF"
    else
      PF="-fplugin=$PLUGIN -fplugin-arg-kzgcc-manifest=$SITES -fplugin-arg-kzgcc-out=$OUT -fplugin-arg-kzgcc-module=2:$W/src/cf/ -fplugin-arg-kzgcc-module=3:$W/src/bp/ -fplugin-arg-kzgcc-default-module=$MOD"
    fi
    LF="-Wl,--whole-archive $RT/libkzrt-posix.a $RT/libkzrt.a -Wl,--no-whole-archive $RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o $RT/CMakeFiles/kzrt-build-info.dir/src/kz_rt_build_info.c.o"
    { echo "## arch cache (D3)"; cmake -DCMAKE_C_FLAGS="$CFLAGS $PF" -DCMAKE_EXE_LINKER_FLAGS="$LF" build/native/default; echo "arch reconfigure rc=$?"; } >> "build_$VAR.log" 2>&1
    { echo "D3 arch CMAKE_C_FLAGS=$CFLAGS $PF"; echo "D3 arch CMAKE_EXE_LINKER_FLAGS=$LF"
      echo "plugin sha256 $(sha256sum "$PLUGIN" | cut -c1-64) manifest sha256 $(sha256sum "$SITES" | cut -c1-64)"
      sed 's/^/runtime /' "$RT/runtime.record"
      [ "$PLAN_MODE" = 0 ] || echo "plan inputs: manifest $SITES sha256 $(sha256sum "$SITES" | cut -c1-64) modules $PLAN_MODULES sha256 $(sha256sum "$PLAN_MODULES" | cut -c1-64) exes $PLAN_EXES sha256 $(sha256sum "$PLAN_EXES" | cut -c1-64) path map ${PLAN_PATH_MAP:-none} sha256 $([ -n "$PLAN_PATH_MAP" ] && sha256sum "$PLAN_PATH_MAP" | cut -c1-64 || echo -)"; } >> "$R"
  fi
  { echo "## make"; date -u +%FT%TZ; ( cd build && make DESTDIR=$W/ws -j2 mission-install ); echo "make rc=$?"; date -u +%FT%TZ; } >> "build_$VAR.log" 2>&1
  grep -E '^(cmake|make|arch reconfigure) rc=' "build_$VAR.log" | tee -a "$R"
  echo "compiler warnings in the build log: $(grep -c 'warning:' "build_$VAR.log")" >> "$R"
  grep -E '^CMAKE_(C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE):' build/native/default/CMakeCache.txt | sed 's/^/arch cache /' >> "$R"
  grep -E '^CMAKE_(C_FLAGS|EXE_LINKER_FLAGS|BUILD_TYPE):' build/CMakeCache.txt | sed 's/^/mission cache /' >> "$R"
  if [ "$PLAN_MODE" = 1 ]; then
    # the plan's executables in this work space (path map) beside the reference file the plan names
    mapfile -t HL < <(derive_exe_paths); mapfile -t HR < <(derive_exe_refs)
    [ ${#HL[@]} = ${#HR[@]} ] || { echo "executable list: mapping failed" >&2; exit 64; }
    HL+=(build/src/cfe_module_version_table.c build/src/cfe_build_env_table.c); HR+=("" "")
  else HL=(exe/tx/core-tx exe/rx/core-rx exe/tx/cf/cf.so exe/tx/cf/bp.so exe/rx/cf/cf.so exe/rx/cf/bp.so build/src/cfe_module_version_table.c build/src/cfe_build_env_table.c); HR=(); fi
  for i in "${!HL[@]}"; do
    f=${HL[$i]}; rf=${HR[$i]:-}; case "$rf" in /*) ;; *) rf=$RB/$f ;; esac
    echo "$(rel_ws "$f") sha256 $(sha256sum "$f" | cut -c1-64) (reference $(sha256sum "$rf" 2>/dev/null | cut -c1-64))"; done >> "$R"
  ( cd build/native/default && find . -name '*.o' -type f | sort | xargs sha256sum ) > "$W/objects_$VAR.sha256"
  echo "objects: $(wc -l < "$W/objects_$VAR.sha256")" >> "$R"
  if [ "$VAR" = ref ]; then
    rm -rf build exe; echo "reference build tree removed after the hashes (it was built only for the byte comparison at the same path)" >> "$R"
  fi ;;
check)
  cd "$W" || exit 2
  # objects that differ between the reference and the derived build at the same path
  join -1 2 -2 2 <(sed 's/  / /' objects_ref.sha256 | sort -k2) <(sed 's/  / /' objects_instr.sha256 | sort -k2) | awk '$2 != $3 {print $1}' > objects_differ.txt
  "$HERE/merge_binds.sh" binds_instr binds_instr.kzs > merge.txt
  units=$(sed -n 's/^# kzgcc.* unit=\([^ ]*\) .*/\1/p' binds_instr/*.kzs | sed "s|^$W/||" | sort)
  {
    echo "# derive_c1.sh check $CFG $CAND $(date -u +%FT%TZ)"
    echo "objects: $(wc -l < objects_instr.sha256), differing: $(wc -l < objects_differ.txt)"
    echo "units with sites (binding files): $(echo "$units" | wc -l)"
    while read -r o; do
      src=$(basename "$o" .o)
      if echo "$units" | grep -q "/$src\$"; then echo "differs (instrumented unit): $o"
      elif [ "$src" = cfe_build_env_table.c ]; then echo "differs (D4 build date/user/host): $o"
      else echo "differs (UNEXPECTED): $o"; fi
    done < objects_differ.txt
    for u in $units; do grep -q "/$(basename "$u").o\$" objects_differ.txt || echo "instrumented unit with an unchanged object: $u"; done
    cat merge.txt
    if [ "$PLAN_MODE" = 1 ]; then
      G0=
      for g in $(derive_exe_groups); do
        mapfile -t GF < <(derive_exe_paths "$g" | while IFS= read -r p; do case "$p" in /*) echo "$p" ;; *) echo "ws/$p" ;; esac; done)
        "$HERE/bincheck.sh" "${GF[@]}" > "bincheck_$g.txt"
        echo "hook calls: $g $(wc -l < "bincheck_$g.txt")"
        if [ -z "$G0" ]; then G0=$g
        else echo "same (function, hook, id) lists $G0 $g: $(cmp -s <(awk '{print $2, $3, $4}' "bincheck_$G0.txt" | sort) <(awk '{print $2, $3, $4}' "bincheck_$g.txt" | sort) && echo yes || echo NO)"; fi
      done
      "$HERE/hookcheck.sh" binds_instr.kzs "bincheck_$G0.txt" > "hookcheck_$G0.txt"; echo "hookcheck $G0 rc=$? $(tail -1 "hookcheck_$G0.txt")"
      grep -vE '^(ok|gap|summary)' "hookcheck_$G0.txt" | sed 's/^/  /'
      # every installed file beside the reference build's file at the same place, and whether the plan's list names it
      mapfile -t PL < <(derive_exe_paths)
      ( cd ws/exe && find . -type f | LC_ALL=C sort ) | while IFS= read -r f; do
        f=${f#./}; d=$(sha256sum "ws/exe/$f" | cut -c1-64); r=$(sha256sum "$RB/exe/$f" 2> /dev/null | cut -c1-64)
        inplan=no; for p in "${PL[@]}"; do [ "$p" = "$W/ws/exe/$f" ] || [ "$p" = "exe/$f" ] && inplan=yes; done
        printf 'exe/%s\t%s\t%s\t%s\t%s\n' "$f" "$d" "${r:--}" "$([ "$d" = "$r" ] && echo same || echo differs)" "$inplan"
      done > install_inventory.tsv
      echo "installed files: $(wc -l < install_inventory.tsv), same as the reference $(awk -F'\t' '$4 == "same"' install_inventory.tsv | wc -l), differing $(awk -F'\t' '$4 == "differs"' install_inventory.tsv | wc -l), in the plan's list $(awk -F'\t' '$5 == "yes"' install_inventory.tsv | wc -l) (install_inventory.tsv)"
    else
    "$HERE/bincheck.sh" ws/exe/tx/core-tx ws/exe/tx/cf/cf.so ws/exe/tx/cf/bp.so > bincheck_tx.txt
    "$HERE/bincheck.sh" ws/exe/rx/core-rx ws/exe/rx/cf/cf.so ws/exe/rx/cf/bp.so > bincheck_rx.txt
    echo "hook calls: tx $(wc -l < bincheck_tx.txt), rx $(wc -l < bincheck_rx.txt); same (function, hook, id) lists: $(cmp -s <(awk '{print $2, $3, $4}' bincheck_tx.txt | sort) <(awk '{print $2, $3, $4}' bincheck_rx.txt | sort) && echo yes || echo NO)"
    "$HERE/hookcheck.sh" binds_instr.kzs bincheck_tx.txt > hookcheck_tx.txt; echo "hookcheck tx rc=$? $(tail -1 hookcheck_tx.txt)"
    grep -vE '^(ok|gap|summary)' hookcheck_tx.txt | sed 's/^/  /'
    fi
  } > check.record
  cat check.record ;;
*) echo "usage: derive_c1.sh prepare|build|check <CFG> <CAND> <W> [ref | instr <kzgcc.so> <rt>]" >&2; exit 64 ;;
esac

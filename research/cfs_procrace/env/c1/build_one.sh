#!/usr/bin/env bash
# build_one.sh CFG CAND -- one C1 P-REC reference build, reconstructed 2026-10-29 from plan/C1_build_manifest.md section 3
# ("build_one.sh CFG CAND") and section 4.2 (env/REBUILD_20261029.md).
#
#   build_one.sh <pre|post> <1df262a|1b0b338> [C1_DIR]       default C1_DIR /home/user/work/procrace/c1_build
#
# Work space c1_build/<CFG>/<CAND> (= CI workspace root), bundle c1_build/bundle_<CAND> (= ./cfs-bundle), apps
# c1_build/<CFG>/src (= ./src).  Steps:
#   prep_defs.sh        CI L117-L124 (prep_defs.sh in this directory)
#   environment         CFS_APP_PATH = <CFG>/src (CI L45-L46); CMAKE_EXPORT_COMPILE_COMMANDS=ON (DV-P2);
#                       CFLAGS=-Wno-error=restrict (DV-P1, accepted 2026-10-25 P5)
#   cmake               CI L127, paths replaced (DV-P3)
#   make                CI L131: (cd build && make DESTDIR=<ws> -j2 mission-install)
# The four builds ran as root (manifest section 3).  TinyCBOR must have been imported first (tinycbor.sh, CI L111).
# Reconstruction choices (RC-5): the log layout of build.log ("## cmake"/"cmake rc=", "## make"/"make rc=", dates; the
# same layout as kairos/instrument/gcc/scripts/derive_c1.sh build, which names this script as its source), the refusal of
# an existing work space, and build.record (hashes of the outputs the manifest section 5 lists).
set -uo pipefail
CFG=${1:?pre|post}; CAND=${2:?1df262a|1b0b338}; C1=${3:-/home/user/work/procrace/c1_build}
case "$CFG" in pre|post) ;; *) echo "CFG is pre|post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a|1b0b338) ;; *) echo "CAND is 1df262a|1b0b338" >&2; exit 64 ;; esac
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS=$C1/$CFG/$CAND
[ ! -e "$WS" ] || { echo "refusing: $WS exists" >&2; exit 2; }
[ -f /usr/local/lib/libtinycbor.so ] || { echo "TinyCBOR is not imported (tinycbor.sh, CI L111)" >&2; exit 2; }
mkdir -p "$WS" && cd "$WS" || exit 2
"$HERE/prep_defs.sh" "$C1/bundle_$CAND" "$C1/$CFG/src" > prep_defs.log 2>&1 || { cat prep_defs.log; exit 1; }
export CFS_APP_PATH=$C1/$CFG/src             # CI L45-L46
export CMAKE_EXPORT_COMPILE_COMMANDS=ON      # DV-P2
export CFLAGS="-Wno-error=restrict"          # DV-P1
{ echo "# build_one.sh $CFG $CAND $(date -u +%FT%TZ) user=$(id -un) $(gcc --version | head -1)"
  echo "env CFS_APP_PATH=$CFS_APP_PATH CMAKE_EXPORT_COMPILE_COMMANDS=$CMAKE_EXPORT_COMPILE_COMMANDS CFLAGS=$CFLAGS"
  echo "## cmake"; date -u +%FT%TZ
  cmake -DCMAKE_BUILD_TYPE=Release -DSIMULATION=native -DCMAKE_INSTALL_PREFIX=/exe -DMISSIONCONFIG=bpxfer \
        -DMISSION_DEFS=$PWD/bpxfer_defs -B build -S $C1/bundle_$CAND/cfe; echo "cmake rc=$?"
  echo "## make"; date -u +%FT%TZ
  (cd build && make DESTDIR=$WS -j2 mission-install); echo "make rc=$?"; date -u +%FT%TZ
} > build.log 2>&1
{ grep -E '^(# build_one|env |cmake rc=|make rc=)' build.log
  echo "warnings: $(grep -c 'warning:' build.log) (of which -Wrestrict: $(grep -c 'Wrestrict' build.log))"
  grep -E '^CMAKE_(C_FLAGS|BUILD_TYPE):' build/CMakeCache.txt | sed 's/^/mission cache /'
  grep -E '^CMAKE_(C_FLAGS|BUILD_TYPE):' build/native/default/CMakeCache.txt | sed 's/^/arch cache /'
  for f in build/native/default/compile_commands.json build/compile_commands.json build/src/cfe_module_version_table.c \
           exe/tx/core-tx exe/rx/core-rx exe/tx/cf/*.so exe/rx/cf/*.so exe/tx/cf/*.tbl exe/rx/cf/*.tbl \
           exe/tx/cf/cfe_es_startup.scr exe/rx/cf/cfe_es_startup.scr exe/host/*; do
    [ -f "$f" ] && echo "$f $(sha256sum "$f" | cut -c1-64)"; done
  grep -h -E '"BUILD(DATE|USER|HOST)"' build/native/default/*/cfe_build_env_table.c 2>/dev/null | sort -u
} > build.record
cat build.record
grep -q '^make rc=0' build.log

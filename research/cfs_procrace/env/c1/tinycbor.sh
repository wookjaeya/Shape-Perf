#!/usr/bin/env bash
# tinycbor.sh -- TinyCBOR for the C1 P-REC builds, reconstructed 2026-10-29 from plan/C1_build_manifest.md section 3
# (first block) and section 4.4 (env/REBUILD_20261029.md).
#
#   tinycbor.sh [C1_DIR]          default /home/user/work/procrace/c1_build; run as root (CI L111 uses sudo)
#
#   build + stage:  nasa/bplib d4c6db9 .github/actions/setup-tinycbor/action.yml L26-L28
#                   (working-directory tinycbor-source: "make all && make DESTDIR=<staging-dir> install")
#   import:         nasa/bp b051eb0 .github/workflows/build-run-bp-common.yml "Import TinyCBOR"
#                   ("sudo cp -rv -t / <staging>/*"); a host change, recorded (manifest section 4.4)
# Reconstruction choices: RC-3 the staging directory c1_build/tinycbor-staging (manifest section 3; the CI uses
# ${{ github.workspace }}/tinycbor-staging), the log file names, and the record of installed files and hashes.
set -euo pipefail
C1=${1:-/home/user/work/procrace/c1_build}
SRC=$C1/tinycbor-source; STG=$C1/tinycbor-staging
[ "$(id -u)" = 0 ] || { echo "run as root (the import copies into /)" >&2; exit 2; }
[ -d "$SRC" ] || { echo "no $SRC (run fetch_refs.sh)" >&2; exit 2; }
[ ! -e "$STG" ] || { echo "refusing: $STG exists" >&2; exit 2; }
{ echo "## make all ($(date -u +%FT%TZ))"; ( cd "$SRC" && make all ); echo "make all rc=$?"
  echo "## make DESTDIR=$STG install"; ( cd "$SRC" && make DESTDIR="$STG" install ); echo "make install rc=$?"
  echo "## cp -rv -t / $STG/*"; cp -rv -t / "$STG"/*; echo "import rc=$?"; } > "$C1/tinycbor.log" 2>&1
{ echo "# tinycbor.sh $(date -u +%FT%TZ): source $(git -C "$SRC" rev-parse HEAD) ($(git -C "$SRC" describe --tags --always --dirty)), $(gcc --version | head -1)"
  grep -E 'rc=' "$C1/tinycbor.log"
  echo "## .config written by the tinycbor Makefile"; cat "$SRC/.config" 2>/dev/null || echo "(none)"
  echo "## staged files (path, sha256 or link target)"
  ( cd "$STG" && find . \( -type f -o -type l \) | sort | while read -r f; do
      if [ -L "$f" ]; then echo "$f -> $(readlink "$f")"; else echo "$f $(sha256sum "$f" | cut -c1-64)"; fi; done ) ; } > "$C1/tinycbor.record"
cat "$C1/tinycbor.record"

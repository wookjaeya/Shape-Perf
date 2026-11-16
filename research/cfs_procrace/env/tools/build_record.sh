#!/usr/bin/env bash
# build_record.sh pre|post TREE OUT [REPO] -- the state of a Kairos build tree before or after a build, as the
# pre_mc.txt / post_mc.txt records of records/step2a-20261110-8cfix/ (same line forms, so the two can be compared).
#
# Writes OUT (refuses an existing OUT):
#   - sha256 of the seven tools (path relative to TREE), then mtime and size of each
#   - kz-static --identity (tool, tool_sha256, tool_source, core_source)
#   - the tool identity defines KZ_TOOL_SOURCE_SHA256 of kz, kz-phi, kz-static, kz-dump (flags.make)
#   - sha256 of CMakeCache.txt and of every tool's flags.make
#   - repository HEAD and the number of uncommitted lines under kairos (git diff -- kairos | wc -l)
#   - free space (df -B1 /)
# A tool that is missing is written "missing <path>" and the script exits 1 after writing everything else.
# Reads only.  BUILD_RECORD_NOW replaces the time stamp (unit tests).
set -uo pipefail
PHASE=${1:-}; TREE=${2:-}; OUT=${3:-}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO=${4:-$(cd "$HERE/../.." && pwd)}
case "$PHASE" in pre|post) ;; *) echo "usage: build_record.sh pre|post TREE OUT [REPO]" >&2; exit 64 ;; esac
[ -d "$TREE" ] || { echo "no tree $TREE" >&2; exit 64; }
[ -n "$OUT" ] && [ ! -e "$OUT" ] || { echo "refusing: OUT missing or exists ($OUT)" >&2; exit 64; }
TOOLS=(tools/kz-static/kz-static tools/kz/kz tools/kz-dump/kz-dump runtime/kz-rt-dump tools/kz-phi/kz-phi tests/lint/kz_lint instrument/clang/kz-instr-clang)
NOW=${BUILD_RECORD_NOW:-$(date -u +%FT%TZ)}
MISS=0
{
  echo "# build_record.sh $PHASE $TREE ($NOW)"
  for t in "${TOOLS[@]}"; do
    if [ -f "$TREE/$t" ]; then (cd "$TREE" && sha256sum "$t"); else echo "missing $t"; MISS=1; fi
  done
  for t in "${TOOLS[@]}"; do
    [ -f "$TREE/$t" ] && (cd "$TREE" && stat -c '%y %s %n' "$t")
  done
  echo "# kz-static --identity"
  if [ -x "$TREE/tools/kz-static/kz-static" ]; then "$TREE/tools/kz-static/kz-static" --identity 2>&1 || echo "kz-static --identity rc=$?"; else echo "kz-static --identity: no tool"; fi
  echo "# tool identity defines (flags.make)"
  for t in kz kz-phi kz-static kz-dump; do
    f="$TREE/tools/$t/CMakeFiles/$t.dir/flags.make"
    if [ -f "$f" ]; then echo "$t $(grep -o 'KZ_TOOL_SOURCE_SHA256=[^ ]*' "$f" | head -1)"; else echo "$t flags.make missing"; fi
  done
  echo "# configuration files"
  (cd "$TREE" && [ -f CMakeCache.txt ] && sha256sum CMakeCache.txt || echo "missing CMakeCache.txt")
  (cd "$TREE" && find . -name flags.make -path '*CMakeFiles*' -print0 | LC_ALL=C sort -z | xargs -0 -r sha256sum | awk '{print "flags.make " $0}' | sha256sum | sed 's/  -$/  (sha256 of the sorted list of every flags.make sha256)/')
  echo "# repository"
  echo "HEAD=$(git -C "$REPO" rev-parse --short HEAD 2> /dev/null || echo '?')"
  echo "uncommitted lines under kairos (git diff -- kairos | wc -l): $(git -C "$REPO" diff -- kairos 2> /dev/null | wc -l)"
  echo "# free (df -B1 /): $(df -B1 / | awk 'NR==2 {print $4}')"
} > "$OUT"
exit $MISS

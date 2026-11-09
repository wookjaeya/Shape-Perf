#!/usr/bin/env bash
# RV4 analysis 7.2 step 1: check PV, round-trip 3ec20abf through a temp file, add the missing items (xz -9), round-trip each.
# Builds nothing. Deletes only the temp files it makes. Prints one line per action.
set -uo pipefail
PV=/home/user/work/preserve/tools-20261109
TMPD=/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/rv4run/tmp
FLOOR=314572800
W=/home/user/work
free() { df -B1 / | awk 'NR==2{print $4}'; }
need() { local n=$1; local f; f=$(free); [ $((f - n)) -ge $FLOOR ] || { echo "SKIP: free $f - need $n < floor $FLOOR"; return 1; }; }
echo "df_before $(date -u +%FT%T.%3NZ) $(free)"
mkdir -p "$TMPD"

# (a) existing PV: XZ_SHA256, and the originals still in place
( cd "$PV" && sha256sum -c XZ_SHA256 ) | sed 's/^/xz_sha256_check: /'
( cd "$W" && sha256sum -c "$PV/ORIGINAL_SHA256" ) | sed 's/^/original_in_place: /'

# (b) round-trip through a temp file: decompress, hash, cmp with the original, delete the temp
rt_file() {  # xzfile original
  local x=$1 o=$2 t="$TMPD/rt.$$"
  need $(stat -c %s "$o") || return 1
  xz -dc "$x" > "$t" || { echo "roundtrip FAIL xz $x"; rm -f "$t"; return 1; }
  local hs ho sz
  hs=$(sha256sum "$t" | cut -d' ' -f1); ho=$(sha256sum "$o" | cut -d' ' -f1); sz=$(stat -c %s "$t")
  if [ "$hs" = "$ho" ] && cmp -s "$t" "$o"; then r=OK; else r=FAIL; fi
  echo "roundtrip_tempfile $r $(basename "$x") sha256=$hs bytes=$sz original=$o free_with_temp=$(free)"
  rm -f "$t"
}
# (c) stream round-trip (no disk): decompress | sha256sum against the listed original hash
rt_stream() {  # xzfile expected_sha
  local h; h=$(xz -dc "$1" | sha256sum | cut -d' ' -f1)
  [ "$h" = "$2" ] && echo "roundtrip_stream OK $(basename "$1") sha256=$h" || echo "roundtrip_stream FAIL $(basename "$1") got=$h want=$2"
}

rt_file "$PV/kz-static.3ec20abf.xz" "$W/c5tools/kz-llvm/tools/kz-static/kz-static"
while read -r h p; do
  case "$p" in c5tools/*) continue ;; esac  # fixed 10:16Z: the first run had */c5tools/*, which missed this relative path
  case "$p" in
    merge-check/kz-llvm/tools/kz-static/kz-static) x=kz-static.db172b74.xz ;;
    merge-check/kz-llvm/tools/kz/kz) x=kz.b254483e.xz ;;
    merge-check/kz-llvm/tools/kz-dump/kz-dump) x=kz-dump.05623054.xz ;;
    kairos-build/tools/kz/kz) x=kz.5d64256a.xz ;;
  esac
  rt_stream "$PV/$x" "$h"
done < "$PV/ORIGINAL_SHA256"

# (d) the missing items: xz -9 -T4 (same as the command that made PV), hash before and after, round-trip each through a temp file
ADD_O="$PV/ADDED_ORIGINAL_SHA256"; ADD_X="$PV/ADDED_XZ_SHA256"
[ ! -e "$ADD_O" ] && [ ! -e "$ADD_X" ] || { echo "refusing: added lists exist"; exit 3; }
for src in merge-check/kz-llvm/runtime/kz-rt-dump merge-check/kz-llvm/CMakeCache.txt \
           merge-check/kz-llvm/tools/kz-static/CMakeFiles/kz-static.dir/flags.make \
           kairos-build/k1r3-llvm/tools/kz-static/kz-static; do
  o="$W/$src"; h0=$(sha256sum "$o" | cut -d' ' -f1); h8=${h0:0:8}
  case "$src" in
    */kz-rt-dump) name=kz-rt-dump.$h8 ;;
    */CMakeCache.txt) name=mc.CMakeCache.txt.$h8 ;;
    */flags.make) name=mc.kz-static.flags.make.$h8 ;;
    */k1r3-llvm/*) name=kz-static.$h8 ;;
  esac
  [ ! -e "$PV/$name.xz" ] || { echo "refusing: $name.xz exists"; continue; }
  need $(( $(stat -c %s "$o") / 2 )) || continue
  xz -9 -T4 -c "$o" > "$PV/$name.xz" || { echo "FAIL xz $src"; continue; }
  h1=$(sha256sum "$o" | cut -d' ' -f1)
  [ "$h0" = "$h1" ] || echo "WARN original changed during copy: $src"
  echo "$h0  $src" >> "$ADD_O"
  ( cd "$PV" && sha256sum "$name.xz" ) >> "$ADD_X"
  echo "added $name.xz xz_bytes=$(stat -c %s "$PV/$name.xz") original_bytes=$(stat -c %s "$o") original_mtime=$(stat -c %y "$o") sha256=$h0"
  rt_file "$PV/$name.xz" "$o"
done
( cd "$PV" && sha256sum -c ADDED_XZ_SHA256 ) | sed 's/^/added_xz_check: /'
rmdir "$TMPD" 2>/dev/null || true
du -sb "$PV" | sed 's/^/pv_du_sb: /'
echo "df_after $(date -u +%FT%T.%3NZ) $(free)"

#!/usr/bin/env bash
# check_buildid.sh -- the build-ids of a test_rt_objects dump against readelf -n of the files (kzrt 0.5.0,
# event_semantics.md 13.5).  Unit-test check.
#   check_buildid.sh <kz-rt-dump> <dump> <test executable> <module with build-id> <module without build-id>
set -u
tool=$1 dump=$2 exe=$3 mod=$4 noid=$5
text=$("$tool" "$dump") || { echo "kz-rt-dump failed"; exit 1; }
rid() { readelf -n "$1" | sed -n 's/^ *Build ID: \([0-9a-f]*\)$/\1/p' | head -1; }
did() { printf '%s\n' "$text" | awk -v o="object=$1" '$1=="buildid" && $2==o {for (i=3;i<=NF;i++) if ($i ~ /^build_id=/) {sub(/^build_id=/,"",$i); print $i}}' | head -1; }
fail=0
check() { # name got want
  if [ "$2" = "$3" ] && [ -n "$3" ]; then echo "same $1 dump=$2 readelf=$3"; else echo "DIFFERENT $1 dump=$2 readelf=$3"; fail=1; fi
}
check main "$(did main)" "$(rid "$exe")"
check module:3 "$(did module:3)" "$(rid "$exe")"
check module:7 "$(did module:7)" "$(rid "$mod")"
[ -z "$(rid "$noid")" ] && echo "module:8 readelf: no build-id" || { echo "module:8 has a build-id in readelf"; fail=1; }
printf '%s\n' "$text" | grep -q '^buildid object=module:8 path="[^"]*" build_id=- note=absent source=registration ' || { echo "module:8 line"; fail=1; }
printf '%s\n' "$text" | grep -q '^buildid object=module:9 path="" build_id=- note=unreadable source=registration ' || { echo "module:9 line"; fail=1; }
printf '%s\n' "$text" | grep -q '^buildid object=main path="[^"]*" build_id=[0-9a-f]* note=present source=start ' || { echo "main line"; fail=1; }
printf '%s\n' "$text" | grep -q '^loss no-build-id actor=- key=8 count=1$' || { echo "loss key 8"; fail=1; }
printf '%s\n' "$text" | grep -q '^check ok' || { echo "check line"; fail=1; }
[ $fail = 0 ] && echo "build-ids: dump = readelf for the executable and the module" && exit 0
exit 1

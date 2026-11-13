#!/bin/sh
# run_transform_test.sh -- ctest instr_clang_transform: kz-instr-clang on tcase.c, gcc with the warning flags of
# the C1 reference build (plan/C1_build_manifest.md section 2: -std=c99 -pedantic -Wall -Wstrict-prototypes
# -Wwrite-strings -Wpointer-arith -Wcast-align -Werror, -O3), the runtime, one run, kz-rt-dump with the manifests.
#   run_transform_test.sh <kz-instr-clang> <cc> <tests dir> <runtime include> <libkzrt-posix.a> <libkzrt.a> <auto.o> <kz-rt-dump> <work dir>
set -eu
TOOL=$1 CC=$2 T=$3 INC=$4 POSIX=$5 CORE=$6 AUTO=$7 DUMPTOOL=$8 W=$9
rm -rf "$W"
mkdir -p "$W/src" "$W/out"
cp "$T/tcase.c" "$W/src/tcase.c"
cp "$T/tcase.c" "$W/out/tcase.c"
FLAGS="-std=c99 -pedantic -Wall -Wstrict-prototypes -Wwrite-strings -Wpointer-arith -Wcast-align -Werror -O3"
fail() { echo "FAIL: $*"; exit 1; }

# the original program builds and passes on its own
$CC $FLAGS -o "$W/orig" "$W/src/tcase.c" || fail "original does not build"
"$W/orig" > "$W/orig.out" || fail "original fails"

"$TOOL" --sites "$T/tcase.kzs" --fixed "$W/src" --map "$W/src=$W/out" --module '1=*' \
    --binds-out "$W/binds.kzs" --report-out "$W/report.txt" -- $FLAGS || fail "transform"
cat "$W/report.txt"
diff -u "$W/src/tcase.c" "$W/out/tcase.c" > "$W/tcase.diff" || true
cat "$W/tcase.diff"
cmp -s "$W/src/tcase.c" "$T/tcase.c" || fail "the source copy was written"

# line numbers are kept: __LINE__ of the last line is the same
[ "$(grep -c '' "$W/src/tcase.c")" -lt "$(grep -c '' "$W/out/tcase.c")" ] || fail "nothing inserted"
$CC $FLAGS -I"$INC" -o "$W/instr" "$W/out/tcase.c" "$AUTO" "$POSIX" "$CORE" -lpthread || fail "instrumented copy does not build with the reference flags"
KZ_RT_DUMP="$W/run.kzrt" KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=1 "$W/instr" > "$W/instr.out" || fail "instrumented program fails (side effects or results differ)"
cmp -s "$W/orig.out" "$W/instr.out" || fail "output differs"
"$DUMPTOOL" --manifest "$T/tcase.kzs" --manifest "$W/binds.kzs" "$W/run.kzrt" > "$W/dump.txt" || { cat "$W/dump.txt"; fail "kz-rt-dump check"; }
cat "$W/dump.txt"

count() { grep -c "^ev .* site=$1\[" "$W/dump.txt" || true; }
[ "$(count 1)" = 1 ] || fail "site 1: $(count 1) records"
[ "$(count 2)" = 1 ] || fail "site 2: $(count 2) records"
[ "$(count 3)" = 0 ] || fail "site 3 (macro body) recorded"
[ "$(count 4)" = 3 ] || fail "site 4: $(count 4) records (3 calls)"
[ "$(count 5)" = 2 ] || fail "site 5: $(count 5) records (the CHECK_PTR return is not instrumented)"
[ "$(count 6)" = 1 ] || fail "site 6: $(count 6) records"
[ "$(count 7)" = 2 ] || fail "site 7: $(count 7) records (one per tapped argument)"
[ "$(count 8)" = 1 ] || fail "site 8: $(count 8) records"
grep -q '^ev .* site=1\[.*str="beta"' "$W/dump.txt" || fail "site 1 string"
grep -q '^ev .* site=2\[.*v=\[0x0,0x7,' "$W/dump.txt" || fail "site 2 values"
grep -q '^ev .* site=6\[.*v=\[0x7,' "$W/dump.txt" || fail "site 6 value"
grep -q 'bind .* site=3 .*status=unsupported' "$W/binds.kzs" || fail "site 3 not unsupported"
[ "$(grep -c "uninstrumented return path" "$W/report.txt")" = 2 ] || fail "uninstrumented returns"
grep -q 'inactive: api_lookup is named in the skipped' "$W/report.txt" || fail "skipped region not reported"
echo "PASS"

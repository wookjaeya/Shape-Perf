#!/usr/bin/env bash
# run_target_test.sh <mode> <clang> <kzllvm.so> <runtime build dir> <kz-events> <test target dir> <work dir>
#   mode O0|O2|O3|O0-g  build kzgcc_target.c (the GCC candidate's test target) with the plugin, run it, check the
#                       dump and the binding file (O0-g: with debug locations, so line= is checked against them)
#   mode untouched      a unit no site names: the object is byte-identical with and without the plugin (-O2, -O0 -g)
#   mode lines          line= with and without debug locations (tests/kzllvm_lines.kzs)
#   mode refuse         a manifest naming a runtime function: the runtime unit is refused, not changed
# Unit test of the plugin; the values are unit-test values.
set -uo pipefail
MODE=$1 CC=$2 PLUGIN=$3 RT=$4 EVENTS=$5 T=$6 W=$7
rm -rf "$W"; mkdir -p "$W/binds"
fail() { echo "FAIL: $*"; exit 1; }
PARGS=(-fpass-plugin="$PLUGIN" -mllvm -kzllvm-manifest="$T/kzgcc_target.kzs" -mllvm -kzllvm-out="$W/binds")
KZRT_SRC="$(cd "$T/../../../runtime" && pwd)"
case "$MODE" in
  untouched)
    for o in "-O2" "-O0 -g"; do
      "$CC" $o -c "$T/kzgcc_plain.c" -o "$W/a.o" || fail "compile without plugin ($o)"
      "$CC" $o -c "$T/kzgcc_plain.c" -o "$W/b.o" "${PARGS[@]}" || fail "compile with plugin ($o)"
      cmp "$W/a.o" "$W/b.o" || fail "objects differ ($o)"
    done
    [ -z "$(ls -A "$W/binds")" ] || fail "a binding file was written for a unit without sites"
    echo "ok: unit without sites is byte-identical at -O2 and at -O0 -g"; exit 0 ;;
  refuse)
    RF=(-O2 -I"$KZRT_SRC/include" -ffreestanding -fno-stack-protector -c "$KZRT_SRC/src/kz_rt.c")
    "$CC" "${RF[@]}" -o "$W/a.o" || fail "compile runtime"
    "$CC" "${RF[@]}" -o "$W/b.o" -fpass-plugin="$PLUGIN" -mllvm -kzllvm-manifest="$T/kzgcc_refuse.kzs" -mllvm -kzllvm-out="$W/binds" \
      || fail "compile runtime with plugin"
    cmp "$W/a.o" "$W/b.o" || fail "the runtime object was changed"
    grep -q "refused: the unit defines runtime function" "$W"/binds/*.kzs || fail "no refusal note"
    grep -q 'status=unsupported reason="unit refused"' "$W"/binds/*.kzs || fail "the site is not declared a gap"
    echo "ok: runtime unit refused and unchanged"; exit 0 ;;
  lines)
    L=(-mllvm -kzllvm-manifest="$(dirname "$0")/kzllvm_lines.kzs")
    "$CC" -O0 -g -c "$T/kzgcc_target.c" -o "$W/g.o" -fpass-plugin="$PLUGIN" "${L[@]}" -mllvm -kzllvm-out="$W/binds" || fail "compile -O0 -g"
    BG=$(ls "$W"/binds/*.kzs); mkdir "$W/binds2"
    grep -q '^bind [0-9]* site=1 .*status=bound' "$BG" || fail "-g: the call at line 64 should bind"
    grep -q '^bind [0-9]* site=2 .*status=unsupported' "$BG" || fail "-g: a wrong line should not bind"
    "$CC" -O2 -c "$T/kzgcc_target.c" -o "$W/n.o" -fpass-plugin="$PLUGIN" "${L[@]}" -mllvm -kzllvm-out="$W/binds2" || fail "compile -O2"
    BN=$(ls "$W"/binds2/*.kzs)
    # without debug locations a line= site binds only as the only call of its callee, and says so (a wrong
    # line is then not detected: the limit this records)
    [ "$(grep -c 'line=unverified(no debug location; the only call of obj_lookup in consumer_init)' "$BN")" = 2 ] || fail "no-debug: unverified notes expected for both sites"
    echo "ok: line= checked with debug locations; without them bound only as the unique call, marked unverified"; exit 0 ;;
esac
OPT=-${MODE%-g}; G=(); [ "$MODE" = O0-g ] && G=(-g)
"$CC" $OPT "${G[@]}" -c "$T/kzgcc_target.c" -o "$W/t.o" "${PARGS[@]}" 2> "$W/cc.err" || { cat "$W/cc.err"; fail "compile"; }
[ -s "$W/cc.err" ] && { cat "$W/cc.err"; fail "compiler diagnostics"; }
"$CC" $OPT "$W/t.o" -o "$W/t" -Wl,--whole-archive "$RT/libkzrt-posix.a" "$RT/libkzrt.a" -Wl,--no-whole-archive \
    "$RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o" -lpthread || fail "link"
KZ_RT_DUMP="$W/dump.bin" "$W/t" > "$W/out.txt" || fail "target exit"
grep -q "consumer 0 id 0x30001 own 0 0x110002" "$W/out.txt" || fail "target output: $(cat "$W/out.txt")"
B=$(ls "$W"/binds/*.kzs); [ -n "$B" ] && [ "$(echo "$B" | wc -l)" = 1 ] || fail "expected one binding file"
"$RT/kz-rt-dump" --manifest "$T/kzgcc_target.kzs" --manifest "$B" "$W/dump.bin" > "$W/dump.txt" || { tail -5 "$W/dump.txt"; fail "kz-rt-dump check"; }
"$EVENTS" "$W/dump.bin" > "$W/events.tsv" || fail "kz-events"
ev() { awk -F'\t' -v s="$1" '$1=="ev" && $4==s' "$W/events.tsv"; }
# the same value checks as the GCC candidate's test (instrument/gcc/tests/run_target_test.sh)
[ "$(ev 1 | awk -F'\t' '$13=="T0_sem"' | wc -l)" -ge 2 ] || fail "consumer lookup: name not copied"
ev 2 | awk -F'\t' '$9=="0xffffffffffffffef"' | grep -q . || fail "consumer lookup: the first status -17 (sign-extended) missing"
ev 2 | awk -F'\t' '$9=="0x0" && $10=="0x30001"' | grep -q . || fail "consumer lookup: success with out id 0x30001 missing"
[ "$(ev 7 | awk -F'\t' '$9=="0x0"' | wc -l)" = 2 ] || fail "provider creates: two successes expected"
ev 6 | awk -F'\t' '$13=="T1_sem" && $11=="0x0"' | grep -q . || fail "provider create T1_sem: name or initial value missing"
ev 8 | awk -F'\t' '$9=="0x110002"' | grep -q . || fail "store of 0x110002 missing"
ev 8 | awk -F'\t' '$9=="0x0"' | grep -q . || fail "store of 0 missing"
ev 9 | awk -F'\t' '$9=="0xfffffffffffffffd" && $10=="0x0"' | grep -q . || fail "unused result of get_own_id not bound"
ev 10 | awk -F'\t' '$9=="0x40009"' | grep -q . || fail "task id at creation missing"
ev 11 | awk -F'\t' '$9=="0x40009"' | grep -q . || fail "task id at entry missing"
ev 12 | awk -F'\t' '$9=="0x0" && $6!=0' | grep -q . && fail "provider_init.ret: link without an entry site"
A10=$(ev 10 | cut -f2); A11=$(ev 11 | cut -f2); [ "$A10" != "$A11" ] || fail "creation and entry on the same actor"
awk -F'\t' '$1=="ev" && ($4==2) { if ($6 == 0) bad = 1 } END { exit bad }' "$W/events.tsv" || fail "consumer after without link"
# string type evidence from the IR (char[20] field, const char[16] field)
grep '^# bind ' "$B" | grep -q 'slot1.string=type:array char\[20\]' || fail "no char[20] evidence for the consumer name"
grep '^# bind ' "$B" | grep -q 'slot1.string=type:array char\[16\]' || fail "no char[16] evidence for the provider name"
grep '^# bind ' "$B" | grep -q 'out0=load-after-call(i32, pointee type confirmed by IR)' || fail "out0 pointee type not confirmed"
# binding statuses
if [ "$MODE" = O0 ] || [ "$MODE" = O0-g ]; then grep -q 'site=4 .*status=bound' "$B" || fail "-O0: probe should stay in init_throttles"; fi
if [ "$MODE" = O2 ] || [ "$MODE" = O3 ]; then grep -Eq 'site=4 .*-> provider_init:(1" status=inlined|[2-9]" status=duplicated)' "$B" || fail "-O2/-O3: probe should be inlined (or inlined and unrolled) into provider_init"; fi
[ "$(grep -c 'site=8 .*status=bound' "$B")" = 2 ] || [ "$(grep -c 'site=8 .*status=\(bound\|inlined\)' "$B")" -ge 2 ] || fail "two store bindings expected"
grep -q 'site=13 .*status=unsupported' "$B" || fail "absent site not declared"
grep -q '^# ctl consumer_init#' "$B" || fail "control hook not counted"
if [ "$MODE" = O0-g ]; then grep -q 'debug_locs=1' "$B" || fail "-g: debug locations expected"; ! grep -q 'line=unverified' "$B" || fail "-g: no unverified line expected"; fi
echo "ok: $MODE"

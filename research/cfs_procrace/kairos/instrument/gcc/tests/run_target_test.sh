#!/usr/bin/env bash
# run_target_test.sh <mode> <gcc> <plugin.so> <runtime build dir> <kzgcc-events> <tests dir> <work dir>
#   mode O0|O2|O3|O2-lto  build kzgcc_target.c with the plugin, run it, check dump and binding file
#   mode untouched        a unit no site names: the object is byte-identical with and without the plugin
#   mode refuse           a manifest naming a runtime function: the runtime unit is refused, not changed
# Unit test of the plugin; the values are unit-test values.
set -uo pipefail
MODE=$1 CC=$2 PLUGIN=$3 RT=$4 EVENTS=$5 T=$6 W=$7
rm -rf "$W"; mkdir -p "$W/binds"
fail() { echo "FAIL: $*"; exit 1; }
PARGS=(-fplugin="$PLUGIN" -fplugin-arg-kzgcc-manifest="$T/kzgcc_target.kzs" -fplugin-arg-kzgcc-out="$W/binds")
KZRT_SRC="$(cd "$T/../../../runtime" && pwd)"
case "$MODE" in
  untouched)
    "$CC" -O2 -c "$T/kzgcc_plain.c" -o "$W/a.o" || fail "compile without plugin"
    "$CC" -O2 -c "$T/kzgcc_plain.c" -o "$W/b.o" "${PARGS[@]}" || fail "compile with plugin"
    cmp "$W/a.o" "$W/b.o" || fail "objects differ"
    [ -z "$(ls -A "$W/binds")" ] || fail "a binding file was written for a unit without sites"
    "$CC" -O0 -g -c "$T/kzgcc_plain.c" -o "$W/c.o" || fail "compile -g without plugin"
    "$CC" -O0 -g -c "$T/kzgcc_plain.c" -o "$W/d.o" "${PARGS[@]}" || fail "compile -g with plugin"
    # with -g the DWARF producer string records the command line (-grecord-gcc-switches, GCC default), so
    # it names the plugin options; everything else must be the same
    norm() { objdump -d -r "$1" | tail -n +3; readelf --debug-dump=info,line,abbrev "$1" | sed -e '/DW_AT_producer/d' -e 's/(indirect string, offset: [0-9a-fx]*)/(indirect string)/' | tail -n +3; }
    cmp <(norm "$W/c.o") <(norm "$W/d.o") || fail "-g objects differ beyond the producer string"
    readelf --debug-dump=info "$W/d.o" | grep DW_AT_producer | grep -q -- -fplugin= || fail "-g: producer string without the plugin options"
    echo "ok: unit without sites is byte-identical at -O2; at -O0 -g it differs only in the DWARF producer string"; exit 0 ;;
  refuse)
    "$CC" -O2 -I"$KZRT_SRC/include" -ffreestanding -fno-stack-protector -c "$KZRT_SRC/src/kz_rt.c" -o "$W/a.o" || fail "compile runtime"
    "$CC" -O2 -I"$KZRT_SRC/include" -ffreestanding -fno-stack-protector -c "$KZRT_SRC/src/kz_rt.c" -o "$W/b.o" -fplugin="$PLUGIN" \
        -fplugin-arg-kzgcc-manifest="$T/kzgcc_refuse.kzs" -fplugin-arg-kzgcc-out="$W/binds" || fail "compile runtime with plugin"
    cmp "$W/a.o" "$W/b.o" || fail "the runtime object was changed"
    grep -q "refused: the unit defines runtime function" "$W"/binds/*.kzs || fail "no refusal note"
    grep -q 'status=unsupported reason="unit refused"' "$W"/binds/*.kzs || fail "the site is not declared a gap"
    echo "ok: runtime unit refused and unchanged"; exit 0 ;;
esac
OPT=-${MODE%-lto}; LTO=(); [ "$MODE" = O2-lto ] && LTO=(-flto)
"$CC" $OPT "${LTO[@]}" -c "$T/kzgcc_target.c" -o "$W/t.o" "${PARGS[@]}" 2> "$W/cc.err" || { cat "$W/cc.err"; fail "compile"; }
[ -s "$W/cc.err" ] && { cat "$W/cc.err"; fail "compiler diagnostics"; }
"$CC" $OPT "${LTO[@]}" "$W/t.o" -o "$W/t" -Wl,--whole-archive "$RT/libkzrt-posix.a" "$RT/libkzrt.a" -Wl,--no-whole-archive \
    "$RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o" -lpthread || fail "link"
KZ_RT_DUMP="$W/dump.bin" "$W/t" > "$W/out.txt" || fail "target exit"
grep -q "consumer 0 id 0x30001 own 0 0x110002" "$W/out.txt" || fail "target output: $(cat "$W/out.txt")"
B=$(ls "$W"/binds/*.kzs); [ -n "$B" ] || fail "no binding file"
"$RT/kz-rt-dump" --manifest "$T/kzgcc_target.kzs" --manifest "$B" "$W/dump.bin" > "$W/dump.txt" || { tail -5 "$W/dump.txt"; fail "kz-rt-dump check"; }
"$EVENTS" "$W/dump.bin" > "$W/events.tsv" || fail "kzgcc-events"
ev() { awk -F'\t' -v s="$1" '$1=="ev" && $4==s' "$W/events.tsv"; }
# values: the consumer's lookups (name copied, status, out id), the provider's creates, the stores, identity
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
A10=$(ev 10 | cut -f2); A11=$(ev 11 | cut -f2); [ "$A10" != "$A11" ] || fail "creation and entry on the same actor"
# links: every after record of a call names the before record of the same actor
awk -F'\t' '$1=="ev" && ($4==2) { if ($6 == 0) bad = 1 } END { exit bad }' "$W/events.tsv" || fail "consumer after without link"
# binding statuses
if [ "$MODE" = O0 ]; then grep -q 'site=4 .*status=bound' "$B" || fail "-O0: probe should stay in init_throttles"; fi
if [ "$MODE" = O2 ] || [ "$MODE" = O3 ]; then grep -q 'site=4 .*-> provider_init:1" status=inlined' "$B" || fail "-O2/-O3: probe should be inlined into provider_init"; fi
if [ "$MODE" = O2-lto ]; then grep -q 'check=deferred-to-lto-link' "$B" || fail "LTO: the check should be deferred"; fi
grep -q 'site=13 .*status=unsupported' "$B" || fail "absent site not declared"
grep -q '^# ctl consumer_init#' "$B" || [ "$MODE" = O2-lto ] || fail "control hook not counted"
echo "ok: $MODE"

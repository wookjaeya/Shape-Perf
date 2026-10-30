#!/usr/bin/env bash
# run_hold_test.sh <plan> <gcc> <plugin.so> <runtime build dir> <kz> <kairos dir> <work dir>
# The cooperative gate of stage F end to end, one kzp-hold 1 rule per plan: tests/cfs-free/target/hold_seq_prog.c (the
# program of F0's kz hold sequence test) built -O2 with the plugin and kzgcc_hold_seq.kzs, linked with the runtime, the
# POSIX start-up object and a build-identity object; the plan converted by `kz rtplan` and given through KZ_RT_PLAN in
# control mode; the program's output and the dump's realized-control records (kz-rt-dump) checked.
#   rearm   F0's hold_rearm.kzh: a released thread fires a later release (H6), releases resume (H4 order)
#   nth     F0's hold_nth.kzh: occurrence n (H2), two gates on one site (H3), counting qualifying hits (H1)
#   defer   F0's hold_defer.kzh: the release of a held hit acts at the resume (H4) and satisfies an unless-released gate (H5)
#   caller  hold_caller.kzh: caller-in on a function entry, evaluated at the call-side binding (H7)
#   refuse  hold_refuse.kzh: a release before its gate is consumed (H5); a gate at ctx=terminating is refused and recorded
# Exit 77 (skipped) without a kz executable.  Unit-test values (delays, retries), not research conditions.
set -uo pipefail
PLAN=$1 CC=$2 PLUGIN=$3 RT=$4 KZ=$5 K=$6 W=$7
[ -x "$KZ" ] || { echo "skip: no kz executable ($KZ)"; exit 77; }
T="$K/instrument/gcc/tests"; P="$K/tests/cfs-free/target"
rm -rf "$W"; mkdir -p "$W/binds"
fail() { echo "FAIL: $*"; [ -f "$W/dump.txt" ] && grep -E '^(ctl|plan|point|check)' "$W/dump.txt" | head -40; exit 1; }
M="$T/kzgcc_hold_seq.kzs"
SHA=$(sha256sum "$M" | cut -c1-64)
( cd "$K/tests/cfs-free" && "$CC" -O2 -c target/hold_seq_prog.c -o "$W/p.o" -fplugin="$PLUGIN" -fplugin-arg-kzgcc-manifest="$M" \
    -fplugin-arg-kzgcc-out="$W/binds" ) 2> "$W/cc.err" || { cat "$W/cc.err"; fail "compile"; }
"$CC" -O2 -c "$K/runtime/src/kz_rt_build_info.c" -I"$K/runtime/include" -o "$W/bi.o" -DKZ_RT_BUILD_BACKEND='"kzgcc unit test"' \
    -DKZ_RT_BUILD_MANIFEST_SHA256="\"$SHA\"" || fail "build identity object"
"$CC" -O2 "$W/p.o" "$W/bi.o" -o "$W/prog" -Wl,--whole-archive "$RT/libkzrt-posix.a" "$RT/libkzrt.a" -Wl,--no-whole-archive \
    "$RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o" -lpthread || fail "link"
B=$(ls "$W"/binds/*.kzs); [ -n "$B" ] || fail "no binding file"
for s in 1 2 3 4 5 6; do grep -Eq "^bind [0-9]+ site=$s .*status=(bound|inlined)" "$B" || fail "site $s not bound: $(grep "site=$s " "$B")"; done
case "$PLAN" in
  rearm) KZH="$P/hold_rearm.kzh"; ARGS=(20 30 0 5) ;;
  nth) KZH="$P/hold_nth.kzh"; ARGS=(20 0 50 20) ;;
  defer) KZH="$P/hold_defer.kzh"; ARGS=(20 30 0 5) ;;
  caller) KZH="$T/hold_caller.kzh"; ARGS=(20 30 0 5) ;;
  refuse) KZH="$T/hold_refuse.kzh"; ARGS=(20 30 0 5) ;;
  *) echo "unknown plan $PLAN"; exit 2 ;;
esac
"$KZ" rtplan "$KZH" --manifest "$M" --binds "$B" --out "$W/plan.kzrtp" > "$W/rtplan.txt" 2>&1 || { cat "$W/rtplan.txt"; fail "kz rtplan"; }
KZ_RT_MODE=control KZ_RT_PLAN="$W/plan.kzrtp" KZ_RT_DUMP="$W/dump.bin" timeout 60 "$W/prog" "${ARGS[@]}" > "$W/out.txt" 2>&1 || fail "program exit (or time limit): $(cat "$W/out.txt")"
"$RT/kz-rt-dump" --manifest "$M" --manifest "$B" --plan "$W/plan.kzrtp" "$W/dump.bin" > "$W/dump.txt" || fail "kz-rt-dump check: $(grep 'check FAIL' "$W/dump.txt")"
grep -q '^plan status=loaded flags=manifest-checked ' "$W/dump.txt" || fail "plan not loaded and checked"
grep -q 'objects=2' "$W/out.txt" || fail "output: $(cat "$W/out.txt")"
# point index of a plan label; the actor of the consumer and of the provider (their first event at their sites)
idx() { sed -n "s/^plan point \([0-9]*\) [a-z]* name=$1 .*/\1/p" "$W/dump.txt"; }
actor_at() { awk -v s="$1" '$1=="ev" { split($4,a,"="); split(a[2],b,"["); if (b[1]==s) { split($2,c,"="); print c[2]; exit } }' "$W/dump.txt"; }
CONS=$(actor_at 2); PROV=$(actor_at 1); [ -z "$PROV" ] && PROV=$(actor_at 6)
ctl() { grep "^ctl .*what=$1 .* point=$2 " "$W/dump.txt"; }
f() { echo "$1" | tr ' ' '\n' | sed -n "s/^$2=//p" | head -1; }
case "$PLAN" in
  rearm)
    grep -q "found0=1 attempts0=0 found1=0 attempts1=20" "$W/out.txt" || fail "output: $(cat "$W/out.txt")"
    hc=$(ctl gate-hold "$(idx G_cons)"); hp=$(ctl gate-hold "$(idx G_prov)"); fm=$(ctl fire "$(idx R_made)"); fd=$(ctl fire "$(idx R_done)")
    [ -n "$hc" ] && [ -n "$hp" ] && [ -n "$fm" ] && [ -n "$fd" ] || fail "rearm: holds and fires expected"
    [ "$(f "$hc" actor)" = "$CONS" ] && [ "$(f "$hp" actor)" = "$PROV" ] || fail "rearm: the consumer and the provider are held"
    [ "$(f "$fm" reason)" = resumed ] && [ "$(f "$fm" other)" = "$CONS" ] || fail "rearm: R_made resumes the consumer: $fm"
    # H6: the release that resumes the provider is fired by the consumer, the thread resumed from G_cons
    [ "$(f "$fd" actor)" = "$CONS" ] && [ "$(f "$fd" reason)" = resumed ] && [ "$(f "$fd" other)" = "$PROV" ] || fail "rearm: R_done by the resumed consumer: $fd" ;;
  nth)
    grep -q "found0=1" "$W/out.txt" && grep -q "found1=0 attempts1=20" "$W/out.txt" || fail "output: $(cat "$W/out.txt")"
    a0=$(sed -n 's/.*attempts0=\([0-9]*\).*/\1/p' "$W/out.txt"); [ "${a0:-0}" -ge 3 ] || fail "nth: the first three lookups of OBJ0 fail"
    h1=$(ctl gate-hold "$(idx G_first)"); h2=$(ctl gate-hold "$(idx G_second)"); f3=$(ctl fire "$(idx R_third)")
    [ "$(f "$h1" hit)" = 1 ] && [ "$(f "$h2" hit)" = 2 ] || fail "nth: G_first holds hit 1, G_second hit 2: $h1 / $h2"
    [ "$(f "$f3" hit)" = 3 ] && [ "$(f "$f3" reason)" = resumed ] && [ "$(f "$f3" other)" = "$PROV" ] || fail "nth: R_third on its 3rd hit resumes the provider: $f3"
    grep -q "^point $(idx G_second) gate .* hits=2 " "$W/dump.txt" || fail "nth: G_second counted two qualifying hits" ;;
  defer)
    grep -q "found0=1 attempts0=0 found1=1" "$W/out.txt" || fail "output: $(cat "$W/out.txt")"
    hc=$(ctl gate-hold "$(idx G_c)"); fs=$(ctl fire "$(idx R_same)")
    [ -n "$hc" ] && [ "$(f "$hc" actor)" = "$CONS" ] || fail "defer: G_c holds the consumer"
    [ "$(f "$(ctl fire "$(idx R_p)")" reason)" = resumed ] || fail "defer: R_p resumes the consumer"
    [ "$(f "$fs" reason)" = "satisfied,deferred" ] && [ "$(f "$fs" subject)" = "$CONS" ] || fail "defer: R_same deferred, satisfies G_x: $fs"
    [ -z "$(ctl gate-hold "$(idx G_x)")" ] || fail "defer: G_x never holds" ;;
  caller)
    grep -q "found0=1 attempts0=[0-9]* found1=0 attempts1=20" "$W/out.txt" || fail "output: $(cat "$W/out.txt")"
    grep -q "^plan point $(idx G_cr) gate name=G_cr label=1 site=6 .* binds=[0-9]" "$W/dump.txt" || fail "caller: G_cr at the call-side site 6 with its bindings"
    hc=$(ctl gate-hold "$(idx G_cr)"); fc=$(ctl fire "$(idx R_cr)")
    echo "$hc" | grep -q 'site=6\[provider.create_call.before\]' && [ "$(f "$hc" actor)" = "$PROV" ] || fail "caller: the provider held at its call: $hc"
    [ "$(f "$fc" reason)" = resumed ] && [ "$(f "$fc" actor)" = "$CONS" ] || fail "caller: the consumer's end resumes it: $fc" ;;
  refuse)
    grep -q "found0=1 attempts0=[0-9]* found1=1" "$W/out.txt" || fail "output: $(cat "$W/out.txt")"
    fe=$(ctl fire "$(idx R_end)"); ge=$(ctl gate-refused "$(idx G_end)")
    [ "$(f "$fe" reason)" = consumed ] || fail "refuse: R_end before its gate is consumed: $fe"
    [ "$(f "$ge" reason)" = context ] && [ "$(f "$ge" ctx)" = terminating ] && [ "$(f "$ge" actor)" = "$CONS" ] || fail "refuse: G_end refused (context, terminating): $ge"
    [ -z "$(ctl gate-hold "$(idx G_end)")" ] || fail "refuse: no hold" ;;
esac
echo "ok: $PLAN ($(grep -c '^ctl ' "$W/dump.txt") control records)"

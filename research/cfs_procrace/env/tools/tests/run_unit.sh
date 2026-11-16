#!/usr/bin/env bash
# run_unit.sh -- synthetic checks of the work scripts under env/tools, env/redesign/one_build.sh and the plan inputs of
# the derive scripts (kairos/instrument/gcc/scripts/derive_plan.sh, derive_args.sh).  Each check runs a clean control
# first (rc 0 / the expected good verdict), then each fault (its expected rc), as env/redesign/step2a_tests_unit.sh.
# Everything runs on stand-ins in a temporary directory under ${TMPDIR:-/tmp}, removed at the end; nothing is built
# or booted and nothing outside that directory is written.  Read only otherwise: the scripts themselves and
# env/redesign/step2a_tests.sh (its inputs table).  These check the scripts; they are not detection experiments.
#   1 space guard: control completes (rc 0); refused at start (rc 75); stopped at the floor (rc 76, writer killed)
#   2 build record: a stub tree gives the record (rc 0, the lines of pre_mc.txt); a missing tool -> rc 1
#   3 ledger: control rc 0; a missing file, a changed byte, a broken bundle part -> rc 1 each
#   4 tool registry: control rc 0; a changed tool byte, a missing copy -> rc 1 each
#   5 single-build runner with a stand-in kz-static: control PASS with the outputs; a failing extract and a failing
#     relations step, a failing sha256 list (recipe missing) -> rc 1 with no facts.kz / relations.txt under a final name; the inputs table equals step2a_tests.sh
#   6 chain checker on a stub chain: consistent -> confirmed (0); a changed executable byte -> refused (1); another
#     manifest -> refused (1); missing B' (no tree, no basis) -> unconfirmed (3)
#   7 harness hashing: every file listed with its sha256, a link in the other list; an existing list -> rc 64
#   8 derive-script arguments on a stub module table: control passes the plan to the derive script; missing sidecar
#     reference, a sidecar with another sha256, a missing plan input -> rc 64; the module options from the table;
#     derive_c1.sh / derive_c5.sh refuse a partial plan (rc 64)
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../../.." && pwd)"
TL=$R/env/tools
GS=$R/kairos/instrument/gcc/scripts
W=$(mktemp -d "${TMPDIR:-/tmp}/tools_unit.XXXXXX") || exit 2
trap 'rm -rf "$W"' EXIT
NF=0; NC=0
res() { # NAME OK(0|1) DETAIL
  NC=$((NC + 1))
  if [ "$2" = 0 ]; then echo "check $NC $1: PASS ($3)"; else echo "check $NC $1: FAIL ($3)"; NF=$((NF + 1)); fi
}

# 1 space guard
X=$W/g; mkdir -p "$X"
echo 900 > "$X/free"
SPACE_GUARD_FREE_CMD="cat $X/free" "$TL/space_guard.sh" --need 100 --floor 500 --interval 0.2 --kind stub --log "$X/c.log" -- bash -c 'echo w > '"$X"'/out; sleep 0.5'; c=$?
SPACE_GUARD_FREE_CMD="cat $X/free" "$TL/space_guard.sh" --need 500 --floor 500 --interval 0.2 --kind stub --log "$X/r.log" -- bash -c 'echo w > '"$X"'/never'; f1=$?
# the stand-in writer lowers "free" by 100 per step; the guard must stop it below the floor 500 (900 -> 400)
echo 900 > "$X/free"
SPACE_GUARD_FREE_CMD="cat $X/free" "$TL/space_guard.sh" --need 100 --floor 500 --interval 0.2 --kind stub --log "$X/s.log" -- \
  bash -c 'for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do echo $(( $(cat '"$X"'/free) - 100 )) > '"$X"'/free; echo $i >> '"$X"'/steps; sleep 0.3; done'; f2=$?
ok=0; [ $c -eq 0 ] && [ -s "$X/out" ] && grep -q 'verdict=completed' "$X/c.log" || ok=1
[ $f1 -eq 75 ] && [ ! -e "$X/never" ] && grep -q 'verdict=refused-at-start' "$X/r.log" || ok=1
[ $f2 -eq 76 ] && grep -q 'verdict=stopped-at-floor' "$X/s.log" && [ "$(wc -l < "$X/steps")" -lt 20 ] || ok=1
res "space guard (stub writer)" $ok "control rc $c; refused rc $f1; floor rc $f2 after $(wc -l < "$X/steps") of 20 writer steps; dip $(sed -n 's/.*dip=\([0-9-]*\).*/\1/p' "$X/s.log")"

# 2 build record
X=$W/b; T=$X/tree; mkdir -p "$T/tools/kz-static/CMakeFiles/kz-static.dir" "$T/tools/kz/CMakeFiles/kz.dir" "$T/tools/kz-dump/CMakeFiles/kz-dump.dir" \
  "$T/tools/kz-phi/CMakeFiles/kz-phi.dir" "$T/runtime" "$T/tests/lint" "$T/instrument/clang"
printf '#!/bin/sh\n[ "$1" = --identity ] && printf "tool=kz-static\\ntool_sha256=00\\ntool_source=11\\ncore_source=22\\n"\n' > "$T/tools/kz-static/kz-static"
chmod +x "$T/tools/kz-static/kz-static"
for t in tools/kz/kz tools/kz-dump/kz-dump runtime/kz-rt-dump tools/kz-phi/kz-phi tests/lint/kz_lint instrument/clang/kz-instr-clang; do echo "$t" > "$T/$t"; done
for t in kz kz-phi kz-static kz-dump; do echo "CXX_DEFINES = -DKZ_TOOL_SOURCE_SHA256=\\\"ab$t\\\"" > "$T/tools/$t/CMakeFiles/$t.dir/flags.make"; done
echo cache > "$T/CMakeCache.txt"
BUILD_RECORD_NOW=T0 "$TL/build_record.sh" pre "$T" "$X/pre.txt" "$X" > /dev/null 2>&1; c=$?
rm "$T/tests/lint/kz_lint"; BUILD_RECORD_NOW=T0 "$TL/build_record.sh" pre "$T" "$X/pre2.txt" "$X" > /dev/null 2>&1; f=$?
ok=0; [ $c -eq 0 ] && [ "$(grep -cE '^[0-9a-f]{64}  (tools|runtime|tests|instrument)/' "$X/pre.txt")" = 7 ] && grep -q '^tool_source=11$' "$X/pre.txt" \
  && grep -q '^kz-static KZ_TOOL_SOURCE_SHA256=' "$X/pre.txt" || ok=1
[ $f -eq 1 ] && grep -q '^missing tests/lint/kz_lint$' "$X/pre2.txt" || ok=1
res "build record (stub tree)" $ok "control rc $c; missing tool rc $f"

# 3 ledger
X=$W/l; mkdir -p "$X/out" "$X/bun"; echo a > "$X/out/a"; echo b > "$X/out/b"; (cd "$X/out" && sha256sum a b > "$X/list")
echo part > "$X/bun/bundle.tar.xz.00"; (cd "$X/bun" && sha256sum bundle.tar.xz.00 > PARTS.sha256)
printf 'id\tkind\tmethod\tlist\troot\tlocal\tverified_copy\tremote\tpublished\tnote\n' > "$X/L.tsv"
printf 'O\toutput\tsha256c\t%s\t%s\tyes\tnone\tnone\tno\tx\nB\tbundle\tparts\t-\t%s\tyes\tnone\tnone\tno\tx\n' "$X/list" "$X/out" "$X/bun" >> "$X/L.tsv"
"$TL/ledger_check.sh" "$X/L.tsv" > "$X/c.txt"; c=$?
mv "$X/out/b" "$X/b.keep"; "$TL/ledger_check.sh" "$X/L.tsv" > "$X/f1.txt"; f1=$?; mv "$X/b.keep" "$X/out/b"
echo c > "$X/out/a"; "$TL/ledger_check.sh" "$X/L.tsv" > "$X/f2.txt"; f2=$?; echo a > "$X/out/a"
printf 'Part' > "$X/bun/bundle.tar.xz.00"; "$TL/ledger_check.sh" "$X/L.tsv" > "$X/f3.txt"; f3=$?
ok=0; [ $c -eq 0 ] && [ $f1 -eq 1 ] && [ $f2 -eq 1 ] && [ $f3 -eq 1 ] || ok=1
res "ledger (temp tree)" $ok "control rc $c; missing file rc $f1; changed byte rc $f2; broken bundle part rc $f3"

# 4 tool registry
X=$W/t; mkdir -p "$X"; echo tool > "$X/tool"; xz -c "$X/tool" > "$X/tool.xz"; h=$(sha256sum < "$X/tool" | cut -c1-64)
printf 'sha256\ttool\tstate\tlocation\tcopy\tfirst_record\tnote\n%s\tstub\tin-place+copy\t%s\t%s\t-\t-\n' "$h" "$X/tool" "$X/tool.xz" > "$X/T.tsv"
"$TL/tools_check.sh" --deep "$X/T.tsv" > "$X/c.txt"; c=$?
echo tooL > "$X/tool"; "$TL/tools_check.sh" "$X/T.tsv" > "$X/f1.txt"; f1=$?; echo tool > "$X/tool"
mv "$X/tool.xz" "$X/keep.xz"; "$TL/tools_check.sh" "$X/T.tsv" > "$X/f2.txt"; f2=$?
ok=0; [ $c -eq 0 ] && [ $f1 -eq 1 ] && [ $f2 -eq 1 ] || ok=1
res "tool registry (temp tree)" $ok "control rc $c; changed tool byte rc $f1; missing copy rc $f2"

# 5 single-build runner
X=$W/o; mkdir -p "$X/bin/kz-static"
cat > "$X/bin/kz-static/kz-static" <<'EOF'
#!/usr/bin/env bash
# stand-in kz-static: writes its -o file; ONE_FAIL=<cmd> makes that command write half a file and fail
cmd=$1; out=
while [ $# -gt 0 ]; do [ "$1" = -o ] && out=${2:-}; shift; done
case "$cmd" in basis|extract|relations) ;; show) echo shown; exit 0 ;; *) exit 2 ;; esac
if [ "${ONE_FAIL:-}" = "$cmd" ]; then echo half > "$out"; echo "$cmd failed" >&2; exit 3; fi
echo "$cmd output" > "$out"; echo "$cmd done"
EOF
chmod +x "$X/bin/kz-static/kz-static"
mkdir -p "$X/ct/641/est" "$X/eb/inputs"; echo '[]' > "$X/ct/641/est/compile_commands.json"; echo recipe > "$X/eb/inputs/c5_641.kzb"
run1() { env KZB="$X/bin" ONE_BUILD_GUARD=0 EB="$X/eb" C1="$X/c1" REF="$X/ref" CT="$X/ct" ONE_FAIL="${2:-}" bash "$R/env/redesign/one_build.sh" c5_641 "$1" > "$1.log" 2>&1; }
run1 "$X/ok"; c=$?
run1 "$X/fx" extract; f1=$?
run1 "$X/fr" relations; f2=$?
mv "$X/eb/inputs/c5_641.kzb" "$X/recipe.keep"; run1 "$X/fs"; f4=$?; mv "$X/recipe.keep" "$X/eb/inputs/c5_641.kzb"
D=$X/ok/c5_641/NP; ok=0
[ $c -eq 0 ] && [ -s "$D/facts.kz" ] && [ -s "$D/relations.txt" ] && [ -s "$D/sha256.txt" ] && grep -q '^result PASS' "$D/status.txt" && ! ls "$D"/*.partial > /dev/null 2>&1 || ok=1
for d in "$X/fx/c5_641/NP" "$X/fr/c5_641/NP" "$X/fs/c5_641/NP"; do
  [ ! -e "$d/facts.kz" ] && [ ! -e "$d/relations.txt" ] && grep -q '^result FAIL' "$d/status.txt" && ! ls "$d"/*.partial > /dev/null 2>&1 || ok=1
done
[ $f1 -eq 1 ] && [ $f2 -eq 1 ] && [ $f4 -eq 1 ] && [ -e "$X/fx/c5_641/NP/facts.kz.failed" ] && [ -e "$X/fr/c5_641/NP/relations.txt.failed" ] || ok=1
t1=$(sed -n '/^inputs_of() {/,/^}/p' "$R/env/redesign/step2a_tests.sh"); t2=$(sed -n '/^inputs_of() {/,/^}/p' "$R/env/redesign/one_build.sh")
[ -n "$t1" ] && [ "$t1" = "$t2" ] || ok=1
run1 "$X/ok"; f3=$?; [ $f3 -eq 2 ] || ok=1
res "single-build runner (stand-in kz-static)" $ok "control rc $c; failing extract rc $f1; failing relations rc $f2; failing sha256 list rc $f4; inputs table same as step2a_tests.sh: $([ "$t1" = "$t2" ] && echo yes || echo NO); non-empty OUT rc $f3"

# 6 chain checker
X=$W/c; mkdir -p "$X/b/exe/tx"
echo "site 1 x" > "$X/plan.kzs"; echo "site 2 y" > "$X/other.kzs"; echo binary > "$X/b/exe/tx/core-tx"; echo module > "$X/b/exe/tx/m.so"
mh=$(sha256sum < "$X/plan.kzs" | cut -c1-64); eh=$(sha256sum < "$X/b/exe/tx/core-tx" | cut -c1-64); oh=$(sha256sum < "$X/b/exe/tx/m.so" | cut -c1-64)
printf 'plugin sha256 %s manifest sha256 %s\nexe/tx/core-tx sha256 %s (reference x)\nexe/tx/m.so sha256 %s (reference y)\n' "$(printf 0%.0s {1..64})" "$mh" "$eh" "$oh" > "$X/build.record"
printf 'file tx/core-tx sha256=%s (build %s)\nfile tx/m.so sha256=%s (build %s)\nmanifest %s sha256=%s\n' "$eh" "$eh" "$oh" "$oh" "$X/plan.kzs" "$mh" > "$X/run.meta"
printf 'run backend="stub" manifest_sha256=%s\nmodule 1 manifest_sha256=%s\n' "$mh" "$mh" > "$X/dump.txt"
printf 'binary.sha256 = %s  (unset)\n' "$eh" > "$X/basis.txt"
cc() { python3 -I "$TL/chain_check.py" --build-record "$X/build.record" --run-meta "$X/run.meta" --dump-text "$X/dump.txt" --main exe/tx/core-tx "$@" > "$X/out.txt" 2>&1; }
cc --plan "$X/plan.kzs" --build-root "$X/b" --basis-text "$X/basis.txt"; c=$?; cv=$(tail -1 "$X/out.txt" | cut -f2)
cp "$X/b/exe/tx/core-tx" "$X/keep"; echo binarY > "$X/b/exe/tx/core-tx"; cc --plan "$X/plan.kzs" --build-root "$X/b" --basis-text "$X/basis.txt"; f1=$?; cp "$X/keep" "$X/b/exe/tx/core-tx"
cc --plan "$X/other.kzs" --build-root "$X/b" --basis-text "$X/basis.txt"; f2=$?
cc --plan "$X/plan.kzs"; f3=$?; f3v=$(grep '^facts_build' "$X/out.txt" | cut -f2)
ok=0; [ $c -eq 0 ] && [ "$cv" = confirmed ] && [ $f1 -eq 1 ] && [ $f2 -eq 1 ] && [ $f3 -eq 3 ] && [ "$f3v" = unconfirmed ] || ok=1
res "chain checker (stub chain)" $ok "consistent rc $c ($cv); changed executable byte rc $f1; other manifest rc $f2; missing B' rc $f3"

# 7 harness hashing
X=$W/h; mkdir -p "$X/inst/tx/cf"; echo a > "$X/inst/tx/core-tx"; echo t > "$X/inst/tx/cf/table.tbl"; ln -s core-tx "$X/inst/tx/link"
line=$("$TL/hash_instance.sh" "$X/inst" "$X/list"); c=$?
"$TL/hash_instance.sh" "$X/inst" "$X/list" > /dev/null 2>&1; f=$?
ok=0; [ $c -eq 0 ] && [ "$(wc -l < "$X/list")" = 2 ] && grep -q "  ./tx/cf/table.tbl$" "$X/list" && grep -q 'link' "$X/list.other" \
  && (cd "$X/inst" && sha256sum -c --quiet "$X/list") && [ $f -eq 64 ] && printf '%s' "$line" | grep -q 'files=2 other=1' || ok=1
res "harness hashing (temp instance dir)" $ok "control rc $c; existing list rc $f"

# 8 derive-script arguments
X=$W/d; mkdir -p "$X"
echo "plan sidecar" > "$X/plan.sidecar"; sh_=$(sha256sum < "$X/plan.sidecar" | cut -c1-64)
echo "site 1 x" > "$X/plan.kzs"
printf '# kz module table; plan sidecar %s sha256=%s\n2\tcf\t@ROOT/src/cf/\n3\tbp\t@ROOT/src/bp/\ndefault\t1\n' "$X/plan.sidecar" "$sh_" > "$X/modules.tsv"
printf 'tx\texe/tx/core-tx\ntx\texe/tx/cf/cf.so\n' > "$X/exes.tsv"
printf '#!/bin/sh\necho "derive stand-in: $*" > %s/called\n' "$X" > "$X/stub.sh"; chmod +x "$X/stub.sh"
dp() { DERIVE_PLAN_SCRIPT="$X/stub.sh" bash "$GS/derive_plan.sh" c1 build pre 1df262a "$X/w" instr p rt "$@" > "$X/dp.log" 2>&1; }
dp --manifest "$X/plan.kzs" --modules "$X/modules.tsv" --exes "$X/exes.tsv"; c=$?; called=$([ -s "$X/called" ] && echo yes || echo no)
sed '1s/.*/# kz module table/' "$X/modules.tsv" > "$X/m_nohdr.tsv"; rm -f "$X/called"
dp --manifest "$X/plan.kzs" --modules "$X/m_nohdr.tsv" --exes "$X/exes.tsv"; f1=$?
sed "1s/sha256=[0-9a-f]*/sha256=$(printf 0%.0s {1..64})/" "$X/modules.tsv" > "$X/m_badsha.tsv"
dp --manifest "$X/plan.kzs" --modules "$X/m_badsha.tsv" --exes "$X/exes.tsv"; f2=$?
dp --manifest "$X/plan.kzs" --modules "$X/modules.tsv"; f3=$?
flags=$(PLAN_MODULES="$X/modules.tsv" bash -c ". '$GS/derive_args.sh'; PLAN_MODULES='$X/modules.tsv'; derive_module_flags /w")
want="-fplugin-arg-kzgcc-module=2:/w/src/cf/ -fplugin-arg-kzgcc-module=3:/w/src/bp/ -fplugin-arg-kzgcc-default-module=1"
bash "$GS/derive_c1.sh" check pre 1df262a "$X/w" --manifest "$X/plan.kzs" > /dev/null 2>&1; f4=$?
bash "$GS/derive_c5.sh" check 1 "$X/w" --modules "$X/modules.tsv" > /dev/null 2>&1; f5=$?
ok=0; [ $c -eq 0 ] && [ "$called" = yes ] && [ ! -e "$X/called" ] && [ $f1 -eq 64 ] && [ $f2 -eq 64 ] && [ $f3 -eq 64 ] && [ "$flags" = "$want" ] \
  && [ $f4 -eq 64 ] && [ $f5 -eq 64 ] || ok=1
res "derive-script arguments (stub module table)" $ok "control rc $c (derive script called: $called); no sidecar reference rc $f1; other sidecar sha256 rc $f2; missing --exes rc $f3; module options $([ "$flags" = "$want" ] && echo as-table || echo "DIFFER: $flags"); partial plan derive_c1 rc $f4 derive_c5 rc $f5"

echo "unit checks: $((NC - NF)) of $NC pass"
exit $((NF > 0))

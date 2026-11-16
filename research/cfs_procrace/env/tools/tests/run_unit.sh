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
#     reference, a sidecar with another sha256, a missing plan input, no path map, an executable list or a manifest
#     that names another sidecar -> rc 64; the module options from the table; derive_c1.sh / derive_c5.sh refuse a
#     partial plan (rc 64)
#   9 path map (kairos/instrument/gcc/scripts/path_map.sh, derive_args.sh): longest prefix wins, a directory equal to a
#     prefix maps, a relative path stays; an absolute path no line covers -> rc 1; malformed maps (relative prefix, no
#     trailing '/', a prefix twice, a third field) are refused; module options and executable paths of a table with the
#     reference build's absolute paths come out mapped, the list's own paths unchanged; an uncovered prefix -> rc 64
#  10 single-build runner, derived-workspace mode (stand-in kz-static): compile db, build dir and bundle root mapped,
#     recipe in place, args.txt names the map; a map that does not cover an input, a mapped input that does not exist,
#     a malformed option -> rc 2
#  11 derived comparison (env/redesign/compare_derived.py, stand-in kz-dump): B' records with the derived paths and a
#     FILE_HASHES order of their own compare equal to B after the map; a declared difference -> rc 0 with it listed;
#     an undeclared record difference, an undeclared text line, a path left outside the map -> rc 1
#  12 binding report (env/tools/plan_bindings.py): every site bound or unsupported with a reason and the module ids
#     of the plugin rule -> rc 0; a site no unit reported, an unsupported site without a reason, a unit with another
#     module id, a unit header with another manifest sha256 -> rc 1 each
#  13 chain checker, the links of a generated plan (sidecar, plan inputs, instance list and tree, B' parent, dump,
#     bindings, run container): a consistent stub chain -> confirmed (0); another sidecar, a changed instance file, an
#     instance list run.meta does not name, a run container with another parent basis, another dump -> refused (1)
#  14 stop-event evaluation (env/obs1/stop_eval.py): apps of a stub startup script reach run loop or exit -> rc 0; an
#     app whose actor never reaches a stop site, an app without a root site in the plan -> rc 3; no RUN line -> rc 2
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
printf '# kz plan\n# sidecar sha256=%s\nsite 1 x\n' "$sh_" > "$X/plan.kzs"
printf '# kz executables to check; plan sidecar %s sha256=%s\ntx\texe/tx/core-tx\ntx\texe/tx/cf/cf.so\n' "$X/plan.sidecar" "$sh_" > "$X/exes.tsv"
printf '/ref/\t/w/\n' > "$X/map.tsv"
printf '#!/bin/sh\necho "derive stand-in: $*" > %s/called\n' "$X" > "$X/stub.sh"; chmod +x "$X/stub.sh"
dp() { DERIVE_PLAN_SCRIPT="$X/stub.sh" bash "$GS/derive_plan.sh" c1 build pre 1df262a "$X/w" instr p rt "$@" > "$X/dp.log" 2>&1; }
dp --manifest "$X/plan.kzs" --modules "$X/modules.tsv" --exes "$X/exes.tsv" --path-map "$X/map.tsv"; c=$?; called=$([ -s "$X/called" ] && echo yes || echo no)
sed '1s/.*/# kz module table/' "$X/modules.tsv" > "$X/m_nohdr.tsv"; rm -f "$X/called"
dp --manifest "$X/plan.kzs" --modules "$X/m_nohdr.tsv" --exes "$X/exes.tsv" --path-map "$X/map.tsv"; f1=$?
sed "1s/sha256=[0-9a-f]*/sha256=$(printf 0%.0s {1..64})/" "$X/modules.tsv" > "$X/m_badsha.tsv"
dp --manifest "$X/plan.kzs" --modules "$X/m_badsha.tsv" --exes "$X/exes.tsv" --path-map "$X/map.tsv"; f2=$?
dp --manifest "$X/plan.kzs" --modules "$X/modules.tsv" --path-map "$X/map.tsv"; f3=$?
dp --manifest "$X/plan.kzs" --modules "$X/modules.tsv" --exes "$X/exes.tsv"; f6=$?
sed "1s/sha256=[0-9a-f]*/sha256=$(printf 0%.0s {1..64})/" "$X/exes.tsv" > "$X/e_badsha.tsv"
dp --manifest "$X/plan.kzs" --modules "$X/modules.tsv" --exes "$X/e_badsha.tsv" --path-map "$X/map.tsv"; f7=$?
sed "2s/sha256=[0-9a-f]*/sha256=$(printf 0%.0s {1..64})/" "$X/plan.kzs" > "$X/p_badsha.kzs"
dp --manifest "$X/p_badsha.kzs" --modules "$X/modules.tsv" --exes "$X/exes.tsv" --path-map "$X/map.tsv"; f8=$?
flags=$(PLAN_MODULES="$X/modules.tsv" bash -c ". '$GS/derive_args.sh'; PLAN_MODULES='$X/modules.tsv'; derive_module_flags /w")
want="-fplugin-arg-kzgcc-module=2:/w/src/cf/ -fplugin-arg-kzgcc-module=3:/w/src/bp/ -fplugin-arg-kzgcc-default-module=1"
bash "$GS/derive_c1.sh" check pre 1df262a "$X/w" --manifest "$X/plan.kzs" > /dev/null 2>&1; f4=$?
bash "$GS/derive_c5.sh" check 1 "$X/w" --modules "$X/modules.tsv" > /dev/null 2>&1; f5=$?
ok=0; [ $c -eq 0 ] && [ "$called" = yes ] && [ ! -e "$X/called" ] && [ $f1 -eq 64 ] && [ $f2 -eq 64 ] && [ $f3 -eq 64 ] && [ "$flags" = "$want" ] \
  && [ $f4 -eq 64 ] && [ $f5 -eq 64 ] && [ $f6 -eq 64 ] && [ $f7 -eq 64 ] && [ $f8 -eq 64 ] || ok=1
res "derive-script arguments (stub module table)" $ok "control rc $c (derive script called: $called); no sidecar reference rc $f1; other sidecar sha256 rc $f2; missing --exes rc $f3; no path map rc $f6; executable list of another sidecar rc $f7; manifest of another sidecar rc $f8; module options $([ "$flags" = "$want" ] && echo as-table || echo "DIFFER: $flags"); partial plan derive_c1 rc $f4 derive_c5 rc $f5"

# 9 path map
X=$W/p; mkdir -p "$X"
printf '# comment\n/ref/b/\t/w/b/\n/ref/b/s/\t/w/s/\n/ref/x/\t/w/ws/\n' > "$X/map.tsv"
pm() { bash -c '. "$1/path_map.sh"; path_map_load "$2" || exit 9; shift 2; for p; do path_map_apply "$p" || echo "rc1:$p"; done' _ "$GS" "$@" 2> /dev/null; }
got=$(pm "$X/map.tsv" /ref/b/a.c /ref/b/s/c.c /ref/b/s /ref/x/exe/tx/core-tx rel/p /other/y | paste -sd' ')
want9="/w/b/a.c /w/s/c.c /w/s /w/ws/exe/tx/core-tx rel/p rc1:/other/y"
bad=0
printf 'ref/b/\t/w/b/\n' > "$X/m1.tsv"; printf '/ref/b\t/w/b/\n' > "$X/m2.tsv"; printf '/ref/b/\t/w/b/\n/ref/b/\t/w/c/\n' > "$X/m3.tsv"; printf '/ref/b/\t/w/b/\textra\n' > "$X/m4.tsv"
for m in m1 m2 m3 m4; do pm "$X/$m.tsv" /ref/b/a > /dev/null; [ $? -eq 9 ] || bad=1; done
echo "plan sidecar" > "$X/side"; shx=$(sha256sum < "$X/side" | cut -c1-64)
printf '# kz module table; plan sidecar %s sha256=%s\n1\tbp\t/ref/b/s/\n2\tci\t/ref/b/apps/ci/\ndefault\t5\n' "$X/side" "$shx" > "$X/mod.tsv"
printf '# kz executables to check; plan sidecar %s sha256=%s\nbasis\t/ref/x/exe/tx/core-tx\nbasis\t/ref/x/exe/tx/cf/bp.so\n' "$X/side" "$shx" > "$X/exe.tsv"
echo m > "$X/plan.kzs"
da() { bash -c '. "$1/derive_args.sh"; shift; derive_plan_args "$@"; echo "F $(derive_module_flags /root)"; echo "P $(derive_exe_paths | paste -sd" ")"; echo "R $(derive_exe_refs | paste -sd" ")"' _ "$GS" "$@" 2> /dev/null; }
out=$(da --manifest "$X/plan.kzs" --modules "$X/mod.tsv" --exes "$X/exe.tsv" --path-map "$X/map.tsv")
wf="F -fplugin-arg-kzgcc-module=1:/w/s/ -fplugin-arg-kzgcc-module=2:/w/b/apps/ci/ -fplugin-arg-kzgcc-default-module=5"
wp="P /w/ws/exe/tx/core-tx /w/ws/exe/tx/cf/bp.so"; wr="R /ref/x/exe/tx/core-tx /ref/x/exe/tx/cf/bp.so"
[ "$(printf '%s\n' "$out" | sed -n 1p)" = "$wf" ] && [ "$(printf '%s\n' "$out" | sed -n 2p)" = "$wp" ] && [ "$(printf '%s\n' "$out" | sed -n 3p)" = "$wr" ] || bad=1
printf '/ref/x/\t/w/ws/\n' > "$X/short.tsv"
da --manifest "$X/plan.kzs" --modules "$X/mod.tsv" --exes "$X/exe.tsv" --path-map "$X/short.tsv" > /dev/null; f1=$?
da --path-map "$X/map.tsv" > /dev/null; f2=$?
ok=0; [ "$got" = "$want9" ] && [ $bad -eq 0 ] && [ $f1 -eq 64 ] && [ $f2 -eq 64 ] || ok=1
res "path map (stub maps and plan tables)" $ok "mapped: $got; malformed maps refused: $([ $bad -eq 0 ] && echo yes || echo NO); plan table mapped: $([ "$(printf '%s\n' "$out" | sed -n 1p)" = "$wf" ] && echo yes || echo NO); uncovered prefix rc $f1; map without plan rc $f2"

# 10 single-build runner, derived-workspace mode
X=$W/q; mkdir -p "$X/bin/kz-static" "$X/c1/pre/1df262a/build/native/default" "$X/c1/bundle_1df262a" "$X/eb/inputs" "$X/w/ws/build/native/default" "$X/w/bundle_1df262a"
cp "$W/o/bin/kz-static/kz-static" "$X/bin/kz-static/kz-static"
echo '[]' > "$X/c1/pre/1df262a/build/native/default/compile_commands.json"; echo '[]' > "$X/w/ws/build/native/default/compile_commands.json"; echo recipe > "$X/eb/inputs/c1_pre_tx.kzb"
printf '%s/\t%s/\n' "$X/c1/bundle_1df262a" "$X/w/bundle_1df262a" "$X/c1/pre/1df262a" "$X/w/ws" > "$X/map.tsv"
printf '%s/\t%s/\n' "$X/c1/pre/1df262a" "$X/w/ws" > "$X/nobundle.tsv"
printf '%s/\t%s/\n' "$X/c1/bundle_1df262a" "$X/w/bundle_1df262a" "$X/c1/pre/1df262a" "$X/w/nows" > "$X/missing.tsv"
run2() { env KZB="$X/bin" ONE_BUILD_GUARD=0 EB="$X/eb" C1="$X/c1" REF="$X/ref" CT="$X/ct" bash "$R/env/redesign/one_build.sh" c1_pre_tx "$@" > "$1.log" 2>&1; }
run2 "$X/ok" --path-map "$X/map.tsv"; c=$?
A=$X/ok/c1_pre_tx/NP/args.txt
okargs=1; grep -qx "compile db: $X/w/ws/build/native/default/compile_commands.json" "$A" && grep -qx "build dir: $X/w/ws/build" "$A" && grep -qx "bundle root: $X/w/bundle_1df262a" "$A" \
  && grep -qx "recipe: $X/eb/inputs/c1_pre_tx.kzb" "$A" && grep -q "^derived workspace: path map $X/map.tsv sha256 " "$A" && okargs=0
run2 "$X/f1" --path-map "$X/nobundle.tsv"; f1=$?
run2 "$X/f2" --path-map "$X/missing.tsv"; f2=$?
run2 "$X/f3" --path-map; f3=$?
ok=0; [ $c -eq 0 ] && [ $okargs -eq 0 ] && [ $f1 -eq 2 ] && [ $f2 -eq 2 ] && [ $f3 -eq 2 ] || ok=1
res "single-build runner, derived workspace (stand-in kz-static)" $ok "control rc $c; inputs mapped, recipe in place: $([ $okargs -eq 0 ] && echo yes || echo NO); uncovered input rc $f1; missing mapped input rc $f2; malformed option rc $f3"

# 11 derived comparison
X=$W/x; mkdir -p "$X/B" "$X/P"
printf '#!/bin/sh\ncat "$1"\n' > "$X/kzd"; chmod +x "$X/kzd"
mk() { # DIR ROOT ORDER(ab|ba) BINARY EXTRA
  local d=$1 r=$2 o=$3 bin=$4 ex=$5
  { echo "kz-dump 1"; echo "container abi=1 kind=basis size=10"; echo "tool_sha256=t"
    echo "section BASIS type=2 count=2 crc32=0x1"; echo "  BASIS[0] name=\"binary.sha256\" value=\"$bin\""; echo "  BASIS[1] name=\"build.dir\" value=\"$r/ws/build\"$ex"
    echo "section FILE_HASHES type=3 count=2 crc32=0x2"
    if [ "$o" = ab ]; then echo "  FILE_HASHES[0] path=\"$r/src/a.c\" sha256=aa size=1"; echo "  FILE_HASHES[1] path=\"$r/ws/build/b.c\" sha256=bb size=1"
    else echo "  FILE_HASHES[0] path=\"$r/ws/build/b.c\" sha256=bb size=1"; echo "  FILE_HASHES[1] path=\"$r/src/a.c\" sha256=aa size=1"; fi
    local fa=0; [ "$o" = ab ] || fa=1
    echo "section FUNCS type=9 count=1 crc32=0x3"; echo "  FUNCS[0] fn=0 name=\"f\" file=\"$r/src/a.c\" fh=$fa"; } > "$d/basis.kz"
  cp "$d/basis.kz" "$d/facts.kz"
  printf 'basis x\n  build %s/ws/build\n' "$r" > "$d/basis.txt"; echo e > "$d/extract.txt"; echo rel > "$d/relations.txt"; : > "$d/relations.log"; echo a > "$d/args.txt"; echo s > "$d/sha256.txt"
}
mk "$X/B" /ref ab b1 ""; mk "$X/P" /w ba b2 ""
printf '/ref/\t/w/\n' > "$X/map.tsv"
printf 'basis.kz\tBASIS\tboth=name="binary\\.sha256";;only=value\tbuild output\nfacts.kz\tBASIS\tname="binary\\.sha256"\tbuild output\n' > "$X/decl.tsv"
cd_() { python3 -I "$R/env/redesign/compare_derived.py" "$X/kzd" "$X/out.txt" "$X/B" "$1" --path-map "$X/map.tsv" --declared "$X/decl.tsv" > "$X/cd.log" 2>&1; }
cd_ "$X/P"; c=$?; cl=$(grep -c $'\tdeclared:build output' "$X/out.txt")
mkdir -p "$X/P2"; mk "$X/P2" /w ba b1 ' flags=x'; cd_ "$X/P2"; f1=$?
mkdir -p "$X/P3"; mk "$X/P3" /w ba b1 ""; echo e2 > "$X/P3/extract.txt"; cd_ "$X/P3"; f2=$?
mkdir -p "$X/P4"; mk "$X/P4" /v ba b1 ""; cd_ "$X/P4"; f3=$?
ok=0; [ $c -eq 0 ] && [ "$cl" = 2 ] && [ $f1 -eq 1 ] && [ $f2 -eq 1 ] && [ $f3 -eq 1 ] || ok=1
res "derived comparison (stand-in kz-dump)" $ok "control rc $c (declared findings $cl); undeclared record rc $f1; undeclared text line rc $f2; path outside the map rc $f3"

# 12 binding report
X=$W/b; mkdir -p "$X"
printf '# kz plan\nkzs-sites 3\nmodule 1 name=app source=build\nmodule 2 name=core source=build\nsite 1 label=s0001.use-call tu=app/a.c func=f\nsite 2 label=s0002.key-text tu=app/a.c func=g\nsite 3 label=s0003.actor-root tu=core/c.c func=h\n' > "$X/plan.kzs"
msha=$(sha256sum < "$X/plan.kzs" | cut -c1-64)
printf '# kz module table; plan sidecar x sha256=y\n1\tapp\t/ref/app/\ndefault\t2\n' > "$X/plan.mod"
FL="-fplugin-arg-kzgcc-module=1:/w/app/ -fplugin-arg-kzgcc-default-module=2"
bf() { # FILE MSHA MODA REASON2 SITE3
  { echo "kzs-sites 1"; echo "# kzgcc 0.3 backend=gcc-plugin gcc=13 x unit=/w/app/a.c dumpbase=a"; echo "# manifest=m sha256=$2 module=$3"
    echo "bind 65552 site=1 backend=gcc-plugin module=$3 at=\"/w/app/a.c f -> f:1\" status=bound vals=0x1"
    echo "bind 65567 site=2 backend=gcc-plugin module=$3 at=\"/w/app/a.c g\" status=unsupported$4"
    if [ "$5" = 1 ]; then echo "# kzgcc 0.3 backend=gcc-plugin gcc=13 x unit=/w/core/c.c dumpbase=c"; echo "# manifest=m sha256=$msha module=2"
      echo "bind 131120 site=3 backend=gcc-plugin module=2 at=\"/w/core/c.c h -> h:1\" status=bound vals=0x0"; fi; } > "$1"; }
pb() { python3 -I "$R/env/tools/plan_bindings.py" --manifest "$X/plan.kzs" --modules "$X/plan.mod" --bindings "$1" --module-flags "$FL" -o "$X/rep.txt" > /dev/null 2>&1; }
bf "$X/ok.kzs" "$msha" 1 ' reason="a line with two calls"' 1; pb "$X/ok.kzs"; c=$?
bf "$X/f1.kzs" "$msha" 1 ' reason="a line with two calls"' 0; pb "$X/f1.kzs"; f1=$?
bf "$X/f2.kzs" "$msha" 1 '' 1; pb "$X/f2.kzs"; f2=$?
bf "$X/f3.kzs" "$msha" 2 ' reason="a line with two calls"' 1; pb "$X/f3.kzs"; f3=$?
bf "$X/f4.kzs" "$(printf 0%.0s {1..64})" 1 ' reason="a line with two calls"' 1; pb "$X/f4.kzs"; f4=$?
ok=0; [ $c -eq 0 ] && [ $f1 -eq 1 ] && [ $f2 -eq 1 ] && [ $f3 -eq 1 ] && [ $f4 -eq 1 ] || ok=1
res "binding report (stub plan and bindings)" $ok "control rc $c; site not seen rc $f1; unsupported without reason rc $f2; other module id rc $f3; other manifest sha256 rc $f4"

# 13 chain checker, links of a generated plan
X=$W/cc; mkdir -p "$X/b/exe/tx" "$X/inst/tx"
echo side > "$X/side"; ss=$(sha256sum < "$X/side" | cut -c1-64); echo facts > "$X/facts"; echo basis > "$X/basis"
printf 'input facts sha256=%s path=x\ninput basis sha256=%s path=y\n' "$(sha256sum < "$X/facts" | cut -c1-64)" "$(sha256sum < "$X/basis" | cut -c1-64)" >> "$X/side"
ss=$(sha256sum < "$X/side" | cut -c1-64)
printf '# kz plan\n# sidecar sha256=%s\nsite 1 x\n' "$ss" > "$X/plan.kzs"; mh=$(sha256sum < "$X/plan.kzs" | cut -c1-64)
echo binary > "$X/b/exe/tx/core-tx"; echo module > "$X/b/exe/tx/m.so"; cp "$X/b/exe/tx/core-tx" "$X/b/exe/tx/m.so" "$X/inst/tx/"
eh=$(sha256sum < "$X/b/exe/tx/core-tx" | cut -c1-64); oh=$(sha256sum < "$X/b/exe/tx/m.so" | cut -c1-64)
printf 'plugin sha256 %s manifest sha256 %s\nexe/tx/core-tx sha256 %s (reference x)\nexe/tx/m.so sha256 %s (reference y)\n' "$(printf 0%.0s {1..64})" "$mh" "$eh" "$oh" > "$X/build.record"
(cd "$X/inst" && find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$X/list"; lh=$(sha256sum < "$X/list" | cut -c1-64)
echo bp > "$X/bpb"; bph=$(sha256sum < "$X/bpb" | cut -c1-64); echo dump > "$X/d.kzrt"; dh=$(sha256sum < "$X/d.kzrt" | cut -c1-64); echo binds > "$X/binds"; nh=$(sha256sum < "$X/binds" | cut -c1-64)
mk_meta() { printf 'file tx/core-tx sha256=%s (build %s)\nmanifest %s sha256=%s\nbindings %s sha256=%s\ninstance_files dir=i files=2 other=0 list=l list_sha256=%s\ndump tx.kzrt bytes=5 sha256=%s\n' "$eh" "$eh" "$X/plan.kzs" "$mh" "$X/binds" "$nh" "$1" "$2" > "$X/run.meta"; }
printf 'run backend="stub" manifest_sha256=%s\n' "$mh" > "$X/dump.txt"
printf 'binary.sha256 = %s  (unset)\n' "$eh" > "$X/basis.txt"
printf 'container abi=1 kind=facts\n  parent[0] kind=basis role=1 sha256=%s\n' "$bph" > "$X/fh.txt"
mk_kzt() { printf 'container abi=1 kind=run\n  parent[0] kind=basis role=0 sha256=%s\n  BASIS[1] name="convert.dump.sha256" value="%s" policy=unset\n  FILE_HASHES[1] path="%s" sha256=%s size=1 prov=none\n  FILE_HASHES[2] path="%s" sha256=%s size=1 prov=none\n  NRUN[0] backend=kzrt manifest_sha256=%s\n' "$1" "$dh" "$X/plan.kzs" "$mh" "$X/binds" "$nh" "$mh" > "$X/kzt.txt"; }
c2() { python3 -I "$TL/chain_check.py" --plan "$X/plan.kzs" --build-record "$X/build.record" --run-meta "$X/run.meta" --dump-text "$X/dump.txt" --main exe/tx/core-tx --build-root "$X/b" --basis-text "$X/basis.txt" \
  --sidecar "$1" --plan-facts "$X/facts" --plan-basis "$X/basis" --inst-list "$X/list" --inst-tree "$X/b/exe" --dump "$X/d.kzrt" --bindings "$X/binds" --bprime-basis "$X/bpb" --facts-header "$X/fh.txt" --kzt-text "$X/kzt.txt" > "$X/out.txt" 2>&1; }
mk_meta "$lh" "$dh"; mk_kzt "$bph"; c2 "$X/side"; c=$?; cv=$(tail -1 "$X/out.txt" | cut -f2); mso=$(grep -c '^exe .*inst_list' "$X/out.txt")
cp "$X/side" "$X/side2"; echo more >> "$X/side2"; c2 "$X/side2"; f1=$?
echo changed > "$X/inst/tx/m.so"; (cd "$X/inst" && find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$X/list"; mk_meta "$(sha256sum < "$X/list" | cut -c1-64)" "$dh"; c2 "$X/side"; f2=$?
cp "$X/b/exe/tx/m.so" "$X/inst/tx/m.so"; (cd "$X/inst" && find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$X/list"; mk_meta "$(printf 1%.0s {1..64})" "$dh"; c2 "$X/side"; f3=$?
mk_meta "$lh" "$dh"; mk_kzt "$(printf 2%.0s {1..64})"; c2 "$X/side"; f4=$?
mk_kzt "$bph"; mk_meta "$lh" "$(printf 3%.0s {1..64})"; c2 "$X/side"; f5=$?
ok=0; [ $c -eq 0 ] && [ "$cv" = confirmed ] && [ "$mso" = 2 ] && [ $f1 -eq 1 ] && [ $f2 -eq 1 ] && [ $f3 -eq 1 ] && [ $f4 -eq 1 ] && [ $f5 -eq 1 ] || ok=1
res "chain checker, generated-plan links (stub chain)" $ok "consistent rc $c ($cv, $mso executables through the instance list); other sidecar rc $f1; changed instance file rc $f2; list not named rc $f3; other parent rc $f4; other dump rc $f5"

# 14 stop-event evaluation
X=$W/se; mkdir -p "$X"
printf 'stop-event run-loop RL a.c:1 decl\nstop-event app-exit EX a.c:2 decl\n' > "$X/side"
printf 'kzs-sites 3\nsite 1 label=s0001.actor-root op=entry.A_Main phase=before tu=a.c\nsite 2 label=s0002.actor-root op=entry.B_Main phase=before tu=b.c\nsite 3 label=s0003.stop-event op=entry.RL phase=before tu=x.c\nsite 4 label=s0004.stop-event op=entry.EX phase=before tu=x.c\nsite 5 label=s0005.use-call op=at-call.f phase=before tu=x.c\n' > "$X/plan.kzs"
printf 'CFE_LIB, l, NULL, L_LIB, 0, 0, 0x0, 0;\nCFE_APP, a, A_Main, A_APP, 1, 1, 0x0, 0;\nCFE_APP, b, B_Main, B_APP, 1, 1, 0x0, 0;\n!\nCFE_APP, z, Z_Main, Z, 1, 1, 0x0, 0;\n' > "$X/scr"
dmp() { { echo 'run abi=1 mode=stopped'; echo 'run t_init=1000000 t_attach=1 t_stop=9000000 t_dump=9100000'
  echo 'ev actor=1 aseq=1 site=1[x] bind=1 link=0 t=2000000 pc=0x1 key=tid:1 v=[-] flags=clock'
  echo 'ev actor=1 aseq=2 site=3[x] bind=1 link=0 t=3000000 pc=0x1 key=tid:1 v=[-] flags=clock'
  echo 'ev actor=2 aseq=1 site=2[x] bind=1 link=0 t=2500000 pc=0x1 key=tid:2 v=[-] flags=clock'
  echo "ev actor=2 aseq=2 site=$1[x] bind=1 link=0 t=4000000 pc=0x1 key=tid:2 v=[-] flags=clock"; } > "$X/d.txt"; }
se() { python3 -I "$R/env/obs1/stop_eval.py" --manifest "$1" --sidecar "$X/side" --startup-script "$X/scr" --dump-text "$X/d.txt" > "$X/o.txt" 2>&1; }
dmp 4; se "$X/plan.kzs"; c=$?; sm=$(grep '^summary' "$X/o.txt" | cut -f2)
dmp 5; se "$X/plan.kzs"; f1=$?; nr=$(grep -c 'not-reached' "$X/o.txt")
dmp 4; grep -v 'site 2 ' "$X/plan.kzs" > "$X/p2.kzs"; se "$X/p2.kzs"; f2=$?; un=$(grep -c 'unresolved(no root site' "$X/o.txt")
grep -v '^run t_init' "$X/d.txt" > "$X/d2.txt"; mv "$X/d2.txt" "$X/d.txt"; se "$X/plan.kzs"; f3=$?
ok=0; [ $c -eq 0 ] && [ "$sm" = "applications=2 reached_inside_window=2 libraries=1" ] && [ $f1 -eq 3 ] && [ "$nr" = 1 ] && [ $f2 -eq 3 ] && [ "$un" = 1 ] && [ $f3 -eq 2 ] || ok=1
res "stop-event evaluation (stub plan, script and dump)" $ok "control rc $c ($sm); actor never at a stop site rc $f1; no root site rc $f2; no RUN line rc $f3"

echo "unit checks: $((NC - NF)) of $NC pass"
exit $((NF > 0))

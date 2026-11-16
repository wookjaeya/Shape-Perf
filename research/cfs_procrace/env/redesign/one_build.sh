#!/usr/bin/env bash
# one_build.sh BUILD OUT -- the profile-free extraction and relations of ONE reference build, with the inputs and
# arguments of the `run` step of step2a_tests.sh (basis --no-profile, extract --no-profile --targets-rule R-T1,R-T2,
# relations), so that a check that needs one build does not re-run all five.  Static analysis only; no cFS is booted.
#
#   BUILD  c1_pre_tx | c1_post_tx | r2_core | c5_641 | c5_642 (the table below is that of step2a_tests.sh inputs_of;
#          env/tools/tests/run_unit.sh checks that the two tables are the same)
#   OUT    a new or empty directory; the outputs go to OUT/<BUILD>/NP/ with the file names of the driver
#          (args.txt basis.kz basis.txt extract.txt facts.kz relations.txt relations.log sha256.txt)
# Options (environment):
#   KZB             tool directory (default the merge-check tree /home/user/work/merge-check/kz-llvm/tools)
#   ONE_BUILD_SHOW=1  also write facts.txt (kz-static show); off by default: the text view is written only when a check needs it
#   ONE_BUILD_NEED  bytes the space guard requires above the floor before each step (default 20000000)
#   ONE_BUILD_GUARD=0 run without env/tools/space_guard.sh (unit tests with a stub tool only)
#   ONE_BUILD_EXTRACT_EXTRA  extra arguments of the extract step only, after the rule (e.g. "--image-budget 3" for a
#                   sensitivity run of a budget); written into args.txt.  Empty by default: the driver's arguments
#
# Failure rule: every output is written under a temporary name (NAME.partial) and renamed to NAME only after every
# step returned 0 and every output is non-empty.  On a failure the partial files are renamed NAME.failed, nothing is
# left under a final name, status.txt says FAIL with the step, and the script exits 1.  So a failing basis, extract or
# relations step never leaves a facts.kz or relations.txt that looks complete.  rc 2: the run could not start
# (unknown build, OUT not new or empty, no tool, guard refused).
set -uo pipefail
B=${1:-}; OUT=${2:-}
[ -n "$B" ] && [ -n "$OUT" ] || { echo "usage: one_build.sh BUILD OUT" >&2; exit 2; }
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
K=$R/kairos
GUARD=$R/env/tools/space_guard.sh
NEED=${ONE_BUILD_NEED:-20000000}
EB=${EB:-/home/user/work/redesign/extract_baseline}
C1=${C1:-/home/user/work/procrace/c1_build}
REF=${REF:-/home/user/work/procrace/cfs_ref/cFS}
CT=${CT:-/home/user/work/c5tools/build}
RULE=(--targets-rule R-T1,R-T2)
read -r -a EXTRA <<< "${ONE_BUILD_EXTRACT_EXTRA:-}"
RULE+=("${EXTRA[@]}")
# BEGIN inputs table (same as step2a_tests.sh inputs_of)
inputs_of() {
  case "$1" in
    c1_pre_tx)  echo "$C1/pre/1df262a/build/native/default/compile_commands.json $C1/pre/1df262a/build $C1/bundle_1df262a $EB/inputs/c1_pre_tx.kzb" ;;
    c1_post_tx) echo "$C1/post/1df262a/build/native/default/compile_commands.json $C1/post/1df262a/build $C1/bundle_1df262a $EB/inputs/c1_post_tx.kzb" ;;
    r2_core)    echo "$REF/build-native_std/native/default_cpu1/compile_commands.json $REF/build-native_std $REF $EB/inputs/r2_core_min.kzb" ;;
    c5_641)     echo "$CT/641/est/compile_commands.json $CT/641/keep/cFE-6.4.1-OSS-release/build/cpu1 $CT/641/keep/cFE-6.4.1-OSS-release $EB/inputs/c5_641.kzb" ;;
    c5_642)     echo "$CT/642/est/compile_commands.json $CT/642/keep/cFE-6.4.2-OSS-release/build/cpu1 $CT/642/keep/cFE-6.4.2-OSS-release $K/profiles/c5-cfe-6.4.x/basis/core-linux.kzb" ;;
    *) return 1 ;;
  esac
}
# END inputs table
X=$(inputs_of "$B") || { echo "unknown build $B" >&2; exit 2; }
read -r CDB BD BR RC <<< "$X"
[ -x "$KS" ] || { echo "no kz-static at $KS" >&2; exit 2; }
if [ -d "$OUT" ]; then
  shopt -s nullglob dotglob; E=("$OUT"/*); shopt -u nullglob dotglob
  [ ${#E[@]} -eq 0 ] || { echo "refusing: $OUT is not empty" >&2; exit 2; }
fi
D=$OUT/$B/NP
mkdir -p "$D" || { echo "cannot make $D" >&2; exit 2; }
ST=$D/status.txt
KSH=$(sha256sum "$KS" | cut -c1-64)
{ echo "# one_build.sh $B $(date -u +%FT%TZ)"; echo "kz-static $KS sha256 $KSH"
  echo "repo $(git -C "$R" rev-parse HEAD 2> /dev/null || echo '?')"; } > "$ST"
guarded() { # NAME CMD...: the command under the space guard, its output to D/NAME.partial... (by the caller)
  local kind=$1; shift
  if [ "${ONE_BUILD_GUARD:-1}" = 0 ]; then "$@"; return $?; fi
  "$GUARD" --need "$NEED" --kind "$kind" --log "$D/space_guard.log" -- "$@"
}
FINALS=(args.txt basis.kz basis.txt extract.txt facts.kz facts.txt relations.txt relations.log sha256.txt)
RENAMING=0
fail() {
  local f
  echo "FAIL $*" >> "$ST"; echo "one_build.sh: FAIL $*" >&2
  for f in "$D"/*.partial; do [ -e "$f" ] && mv -f "$f" "${f%.partial}.failed"; done
  if [ $RENAMING = 1 ]; then for f in "${FINALS[@]}"; do [ -e "$D/$f" ] && mv -f "$D/$f" "$D/$f.failed"; done; fi
  echo "result FAIL" >> "$ST"
  exit 1
}
printf '%s\n' "basis/extract args: --no-profile" "extract-only args: ${RULE[*]}" "compile db: $CDB" "build dir: $BD" "bundle root: $BR" "recipe: $RC" > "$D/args.txt.partial" || fail "write args.txt"
guarded extract "$KS" basis --no-profile --compile-db "$CDB" --build-dir "$BD" --bundle-root "$BR" --recipe "$RC" --text -o "$D/basis.kz.partial" > "$D/basis.txt.partial" 2>&1
x=$?; echo "basis rc=$x" >> "$ST"; [ $x -eq 0 ] || fail "basis (rc $x)"
guarded extract "$KS" extract --no-profile --basis "$D/basis.kz.partial" "${RULE[@]}" -o "$D/facts.kz.partial" > "$D/extract.txt.partial" 2>&1
y=$?; echo "extract rc=$y" >> "$ST"; [ $y -eq 0 ] || fail "extract (rc $y)"
if [ "${ONE_BUILD_SHOW:-0}" = 1 ]; then
  "$KS" show "$D/facts.kz.partial" > "$D/facts.txt.partial" 2>&1; z=$?; echo "show rc=$z" >> "$ST"; [ $z -eq 0 ] || fail "show (rc $z)"
fi
guarded relations "$KS" relations "$D/facts.kz.partial" -o "$D/relations.txt.partial" > "$D/relations.log.partial" 2>&1
w=$?; echo "relations rc=$w" >> "$ST"; [ $w -eq 0 ] || fail "relations (rc $w)"
for f in args.txt basis.kz basis.txt extract.txt facts.kz relations.txt; do
  [ -s "$D/$f.partial" ] || fail "output $f is missing or empty"
done
# the record of the paths as step2a_tests.sh writes it (sha256.txt names basis.kz and facts.kz by their final names);
# written before any rename, so that a failure here leaves only partial files
( cd "$D" && sha256sum basis.kz.partial facts.kz.partial "$CDB" "$RC" "$KS" ) | sed 's/\.partial$//' > "$D/sha256.txt.partial" || fail "sha256.txt"
RENAMING=1
for f in "$D"/*.partial; do mv -f "$f" "${f%.partial}" || fail "rename $f"; done
echo "result PASS" >> "$ST"
echo "one_build.sh $B: PASS -> $D"

#!/usr/bin/env bash
# step2a_tests_unit.sh [DRIVER] -- the unit checks of the failure handling of step2a_tests.sh (RV4 analysis
# §2 #29, RV3A §2.4; checks 5 and 6 added in the fix stage after c4fe9db).  DRIVER (default: step2a_tests.sh beside this script; a copy of an earlier one needs its
# step2a_tests_count.py beside it) runs on fixtures made here, with stand-ins for kz-static and kz-dump, in a
# temporary directory under ${TMPDIR:-/tmp} that is removed at the end.  Nothing is built or booted and nothing is
# written outside that directory.  Read only: the repository (git; the recipe of c5_642) and, for an earlier driver
# whose input paths are fixed, the compile databases those paths name (hashed).  These check the driver; they are
# not detection experiments and are not counted as such (RV4 L178).
#   1  a required comparison fails (one WRITERS record differs; the table allows nothing) -> s5same rc 1
#   2  a command inside a pipeline fails (kz-dump of one input prints it all and exits 3) -> all rc 1
#   3  an empty dump (kz-dump prints nothing, rc 0) -> s5same rc 1; a kz-dump failure (input missing) -> fixsame rc 1
#   4  an unexpected record change inside an allowed section (the table allows BASIS extract.build_identity only;
#      count.lockset changes too) -> fix3same rc 1
#   5  an allowed record that also changes a field outside its entry's list (the table allows the value of
#      extract.build_identity; its policy changes too) -> fix3same rc 1
#   6  the driver cannot start: no STEP -> rc 2; a single step into an OUT that holds a top-level sha256.txt (a
#      finished output such as T/) -> rc 2; the control is the same step with STEP2A_OVERWRITE=1 -> rc 0
# Each check runs its control first (the same fixtures without the fault), which must give rc 0, so that the rc 1
# (check 6: rc 2) comes from the fault.  One line per check; exit 0 only when all six pass.  A driver before the fix
# stage reads the table without the fields column; checks 4 and 5 then fail in their control.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVER=${1:-$HERE/step2a_tests.sh}
[ -f "$DRIVER" ] || { echo "no driver $DRIVER" >&2; exit 2; }
W=$(mktemp -d "${TMPDIR:-/tmp}/step2a_unit.XXXXXX") || exit 2
trap 'rm -rf "$W"' EXIT
BUILDS=(c1_pre_tx c1_post_tx r2_core c5_641 c5_642)
NFAIL=0

# world X: a complete fixture world in X (tools, inputs, reference outputs, the table)
world() {
  local X=$1 b v d
  mkdir -p "$X/bin/kz-static" "$X/bin/kz-dump" "$X/fx" "$X/in" || return 1
  cat > "$X/bin/kz-static/kz-static" <<'EOF'
#!/usr/bin/env bash
# stand-in for kz-static: its outputs are the fixture texts in ../../fx
FX="$(cd "$(dirname "$0")/../../fx" && pwd)"
case "${1:-}" in
  --identity) echo "tool=kz-static stand-in (step2a_tests_unit.sh)" ;;
  show) cat "$FX/show.txt" ;;
  basis|extract|relations)
    cmd=$1 out=
    while [ $# -gt 0 ]; do [ "$1" = -o ] && out=${2:-}; shift; done
    [ -n "$out" ] || exit 2
    case $cmd in
      basis) cp "$FX/basis.dump" "$out" ;;
      extract) cp "$FX/facts.dump" "$out" ;;
      relations) cp "$FX/relations.txt" "$out" ;;
    esac || exit 1
    echo "$cmd (stand-in)" ;;
  *) exit 2 ;;
esac
EOF
  cat > "$X/bin/kz-dump/kz-dump" <<'EOF'
#!/usr/bin/env bash
# stand-in for kz-dump: a fixture "kz" file is its dump text; FILE.empty: print nothing; FILE.rc: exit with that rc
[ -e "${1:-}" ] || { echo "kz-dump: Io: cannot open ${1:-}" >&2; exit 2; }
[ -e "$1.empty" ] && exit 0
cat "$1" || exit 2
[ -e "$1.rc" ] && exit "$(cat "$1.rc")"
exit 0
EOF
  chmod +x "$X/bin/kz-static/kz-static" "$X/bin/kz-dump/kz-dump" || return 1
  printf '%s\n' 'kz-dump 1' 'container abi=1 kind=facts size=1' "basis_sha256=$(printf '0%.0s' {1..64})" \
    "tool_sha256=$(printf '1%.0s' {1..64})" 'parents=1' "  parent[0] kind=basis role=1 sha256=$(printf '0%.0s' {1..64})" \
    'sections=3' 'section STRINGS type=1 version=1 record=kz_byte record_size=1 count=16 crc32=0x1' \
    'section BASIS type=2 version=1 record=kz_basis_item record_size=28 count=2 crc32=0x2' \
    '  BASIS[0] name="extract.build_identity" value="stand-in" policy=unset prov=none deviation=none' \
    '  BASIS[1] name="count.lockset" value="2 rows" policy=unset prov=none deviation=none' \
    'section WRITERS type=18 version=2 record=kz_fact_writer record_size=112 count=2 crc32=0x3' \
    '  WRITERS[0] fn=1 field="a" kind=assign' '  WRITERS[1] fn=2 field="b" kind=assign' > "$X/fx/facts.dump"
  printf '%s\n' 'kz-dump 1' 'container abi=1 kind=basis size=1' "tool_sha256=$(printf '1%.0s' {1..64})" 'parents=0' \
    'sections=2' 'section STRINGS type=1 version=1 record=kz_byte record_size=1 count=16 crc32=0x1' \
    'section BASIS type=2 version=1 record=kz_basis_item record_size=28 count=1 crc32=0x2' \
    '  BASIS[0] name="front_end.build_identity" value="stand-in" policy=unset prov=none deviation=none' > "$X/fx/basis.dump"
  printf '%s\n' 'facts-view 1' '[basis] 2' '  extract.build_identity = stand-in  (unset)' \
    '  extract.units_from = basis 1111111111111111  (unset)' > "$X/fx/show.txt"
  printf '%s\n' 'summary 0 rows (stand-in)' > "$X/fx/relations.txt"
  echo '[]' > "$X/in/compile_commands.json"; echo 'stand-in recipe' > "$X/in/recipe.kzb"
  for d in c1/pre/1df262a/build/native/default c1/post/1df262a/build/native/default \
           ref/build-native_std/native/default_cpu1 ct/641/est ct/642/est; do
    mkdir -p "$X/$d" && cp "$X/in/compile_commands.json" "$X/$d/" || return 1
  done
  mkdir -p "$X/eb/inputs" || return 1
  echo "tool_sha256=$(printf '2%.0s' {1..64})" > "$X/eb/inputs/inputs.record"
  for b in c1_pre_tx c1_post_tx r2_core_min c5_641; do cp "$X/in/recipe.kzb" "$X/eb/inputs/$b.kzb" || return 1; done
  for b in c1_pre_tx r2_core c5_641; do
    for v in V0 V2; do
      d=$X/eb/$b/$v; mkdir -p "$d" || return 1
      printf '%s\n' "profile: --profile $X/in/profile" 'extract args: ' "compile db: $X/in/compile_commands.json" \
        "build dir: $X/in" "bundle root: $X/in" "recipe: $X/in/recipe.kzb" > "$d/args.txt"
      cp "$X/fx/basis.dump" "$d/basis.kz" && cp "$X/fx/facts.dump" "$d/facts.kz" || return 1
      sed 's/basis 1111111111111111/basis 2222222222222222/' "$X/fx/show.txt" > "$d/facts.txt" || return 1
    done
  done
  for b in "${BUILDS[@]}"; do
    mkdir -p "$X/s4/$b/NP" && cp "$X/fx/basis.dump" "$X/s4/$b/NP/basis.kz" && cp "$X/fx/facts.dump" "$X/s4/$b/NP/facts.kz" || return 1
    for v in s5 fix1 fix2 fix3; do
      mkdir -p "$X/$v/$b/NP" && cp "$X/fx/facts.dump" "$X/$v/$b/NP/facts.kz" && cp "$X/fx/relations.txt" "$X/$v/$b/NP/" || return 1
    done
  done
  echo '# no entry: every difference is unexpected' > "$X/table"
}
# outs O X: the run outputs a comparison step reads (O/<build>/NP/facts.kz), without running the step run
outs() {
  local b
  for b in "${BUILDS[@]}"; do mkdir -p "$1/$b/NP" && cp "$2/fx/facts.dump" "$1/$b/NP/facts.kz" || return 1; done
}
# drv X OUT STEP: the driver on the world X; its output in OUT.log
drv() {
  local X=$1
  env KZB="$X/bin" EB="$X/eb" S4="$X/s4" S5="$X/s5" FIX1="$X/fix1" FIX2="$X/fix2" FIX3="$X/fix3" C1="$X/c1" \
    REF="$X/ref" CT="$X/ct" STEP2A_ALLOW_TABLE="$X/table" STEP2A_RUNS="$X/runs" STEP2A_OVERWRITE="${OVW:-}" \
    bash "$DRIVER" "$3" "$2" > "$2.log" 2>&1
}
# why OUT: the first failure the driver recorded (status.txt), or what its log ends with
why() {
  if [ -s "$1/status.txt" ]; then
    grep -m1 '^FAIL ' "$1/status.txt" || grep -m1 '^result ' "$1/status.txt" || echo '(status.txt without a result)'
    echo "steps: $(sed -n 's/^step \([a-z0-9]*\) \(PASS\|FAIL\).*/\1 \2/p' "$1/status.txt" | tr '\n' ' ')"
  else
    echo "(no status.txt; log ends: $(tail -1 "$1.log" 2> /dev/null | cut -c1-160))"
  fi
}
# result N NAME "CONTROL_RCS" "FAULT_RCS" FAULT_OUT..: controls rc 0, faults rc 1 (FAULT_RC overrides the 1)
result() {
  local n=$1 name=$2 ok=1 c f o
  for c in $3; do [ "$c" -eq 0 ] || ok=0; done
  for f in $4; do [ "$f" -eq "${FAULT_RC:-1}" ] || ok=0; done
  echo "check $n $name: control rc $3; fault rc $4 -> $([ $ok -eq 1 ] && echo PASS || echo FAIL)"
  shift 4
  for o; do why "$o" | sed "s#$W/#<tmp>/#g" | cut -c1-200 | sed "s/^/    ${o##*/}: /"; done
  [ $ok -eq 1 ] || NFAIL=$((NFAIL + 1))
}

echo "# step2a_tests_unit.sh: driver $DRIVER (sha256 $(sha256sum "$DRIVER" | cut -c1-16))"
# 1 a required comparison fails
X=$W/w1; world "$X" || exit 2
outs "$X/ok" "$X" && drv "$X" "$X/ok" s5same; c=$?
sed -i 's/^  WRITERS\[1\] fn=2 field="b" kind=assign$/  WRITERS[1] fn=2 field="b" kind=memset/' "$X/s5/c5_641/NP/facts.kz" || exit 2
outs "$X/bad" "$X" && drv "$X" "$X/bad" s5same; f=$?
result 1 "required comparison fails -> step rc 1" "$c" "$f" "$X/bad"
# 2 a command inside a pipeline fails, under all
X=$W/w2; world "$X" || exit 2
drv "$X" "$X/ok" all; c=$?
echo 3 > "$X/fix2/c5_642/NP/facts.kz.rc" || exit 2
drv "$X" "$X/bad" all; f=$?
result 2 "kz-dump rc 3 inside a pipeline -> all rc 1" "$c" "$f" "$X/bad"
# 3 an empty dump; a kz-dump failure
X=$W/w3; world "$X" || exit 2
outs "$X/ok" "$X" && drv "$X" "$X/ok" s5same; c=$?
outs "$X/ok2" "$X" && drv "$X" "$X/ok2" fixsame; c2=$?
outs "$X/bad" "$X" && touch "$X/s5/c1_pre_tx/NP/facts.kz.empty" "$X/bad/c1_pre_tx/NP/facts.kz.empty" && drv "$X" "$X/bad" s5same; f=$?
outs "$X/bad2" "$X" && rm -f "$X/fix1/r2_core/NP/facts.kz" && drv "$X" "$X/bad2" fixsame; f2=$?
result 3 "empty dump, kz-dump failure -> step rc 1" "$c $c2" "$f $f2" "$X/bad" "$X/bad2"
# 4 an unexpected change inside an allowed section
X=$W/w4; world "$X" || exit 2
printf '%s\n' '# step section change c1pre c1post r2 c5_641 c5_642 ref key' \
  'fix3same BASIS changed 1 1 1 1 1 unit value ^name="extract\.build_identity"' > "$X/table"
for b in "${BUILDS[@]}"; do sed -i 's/value="stand-in"/value="earlier"/' "$X/fix3/$b/NP/facts.kz" || exit 2; done
outs "$X/ok" "$X" && drv "$X" "$X/ok" fix3same; c=$?
sed -i 's/name="count.lockset" value="2 rows"/name="count.lockset" value="3 rows"/' "$X/fix3/c5_641/NP/facts.kz" || exit 2
outs "$X/bad" "$X" && drv "$X" "$X/bad" fix3same; f=$?
result 4 "unexpected change inside an allowed section -> step rc 1" "$c" "$f" "$X/bad"
# 5 an allowed record that also changes a field outside its entry's list
X=$W/w5; world "$X" || exit 2
printf '%s\n' '# step section change c1pre c1post r2 c5_641 c5_642 ref fields key' \
  'fix3same BASIS changed 1 1 1 1 1 unit value ^name="extract\.build_identity"' > "$X/table"
for b in "${BUILDS[@]}"; do sed -i 's/value="stand-in"/value="earlier"/' "$X/fix3/$b/NP/facts.kz" || exit 2; done
outs "$X/ok" "$X" && drv "$X" "$X/ok" fix3same; c=$?
sed -i 's/name="extract.build_identity" value="earlier" policy=unset/name="extract.build_identity" value="earlier" policy=pinned/' \
  "$X/fix3/c5_641/NP/facts.kz" || exit 2
outs "$X/bad" "$X" && drv "$X" "$X/bad" fix3same; f=$?
result 5 "allowed record changes a field outside its list -> step rc 1" "$c" "$f" "$X/bad"
# 6 the driver cannot start
X=$W/w6; world "$X" || exit 2
outs "$X/ok" "$X" && echo 'stand-in list' > "$X/ok/sha256.txt" && OVW=1 drv "$X" "$X/ok" s5same; c=$?
outs "$X/bad" "$X" && echo 'stand-in list' > "$X/bad/sha256.txt" && drv "$X" "$X/bad" s5same; f=$?
env -u STEP2A_OVERWRITE KZB="$X/bin" STEP2A_RUNS="$X/runs" bash "$DRIVER" > "$X/nostep.log" 2>&1; f2=$?
FAULT_RC=2 result 6 "no STEP, or OUT holds sha256.txt -> rc 2 (driver does not start)" "$c" "$f $f2" "$X/bad" "$X/nostep"
echo "unit checks: $((6 - NFAIL)) of 6 pass"
exit $((NFAIL > 0))

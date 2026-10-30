#!/usr/bin/env bash
# step2a_s1.sh STEP [OUT] -- redesign step 2a, stage S1 (plan/extraction_design_20261030.md §6 items 1-2, §5 R9): the
# profile-free mode of kz-static (`basis --no-profile`, `extract --no-profile`: the built-in two-statement profile, the P0
# table, F8 DELAY-W/TERM-W and F7 SPAWN-W) on the reference builds, compared line by line with V1 (G0) and VP of
# extract_baseline.sh.  Static analysis only: no cFS is booted, no campaign is run.
#
#   STEP   identity  the profile-driven mode is unchanged: the V0 and V2 variants of extract_baseline.sh, re-run with this
#                    kz-static from their recorded arguments (EB/<build>/<variant>/args.txt) for c1_pre_tx, r2_core and
#                    c5_641, basis and facts compared section by section with the earlier outputs (kz-dump: the section
#                    lines with their CRC and every record line) -> OUT/identity/
#          run       basis --no-profile + extract --no-profile + show for every build below -> OUT/<build>/NP/; for
#                    c5_642, which extract_baseline.sh does not have, also its VP and V1 (profile-driven, the arguments of
#                    extract_baseline.sh's c5_641 on the 6.4.2 tree) -> OUT/c5_642/{VP,V1}/
#          twice     determinism: extract --no-profile again from the same basis; facts bytes compared -> OUT/<build>/NP2/
#          compare   step2a_s1_compare.py: NP against V1 and VP, line by line (loops, order, spawn targets and parameters,
#                    lock flags, terminals), the derived chains -> OUT/compare/
#          all       the four in this order
#   OUT    default /home/user/work/redesign/step2a/s1
#
# Builds (reference builds only), with the inputs of extract_baseline.sh's VP variant:
#   c1_pre_tx c1_post_tx   procrace/c1_build/<pre|post>/1df262a, recipe EB/inputs/c1_<cfg>_tx.kzb
#   r2_core                procrace/cfs_ref/cFS/build-native_std core-cpu1, recipe EB/inputs/r2_core_min.kzb
#   c5_641 c5_642          c5tools/build/64x/keep (E-ST compile database c5tools/build/64x/est/compile_commands.json),
#                          recipe profiles/c5-cfe-6.4.x/basis/core-linux.kzb (the recipe of both releases; for 6.4.1 the copy
#                          EB/inputs/c5_641.kzb)
# The profile-free runs take no --profile and no vocabulary option: kz-static refuses --target, --delay-op, --lock-op,
# --unlock-op, --spawn-op, --terminal-op and --poll-fn with --no-profile.  The recipes keep their startup-script block
# (the loader semantics L0) and, for C5, the two front_end_diagnostic lines (PI D7); nothing else of a case enters.
set -uo pipefail
STEP=${1:?identity|run|twice|compare|all}
OUT=${2:-/home/user/work/redesign/step2a/s1}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
K=$R/kairos
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
KZD=$KZB/kz-dump/kz-dump
EB=${EB:-/home/user/work/redesign/extract_baseline}
C1=/home/user/work/procrace/c1_build
REF=/home/user/work/procrace/cfs_ref/cFS
CT=/home/user/work/c5tools/build
[ -e /home/user/work/.campaign_running ] && { echo "a campaign is running (/home/user/work/.campaign_running)" >&2; exit 3; }
mkdir -p "$OUT"

# the G0 names, for the V1 variant of c5_642 only (the same list as extract_baseline.sh; profile-driven comparison runs)
G0=(--spawn-op OS_TaskCreate --spawn-op CFE_ES_CreateChildTask --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData
    --unlock-op CFE_ES_UnlockSharedData --terminal-op CFE_ES_ExitApp --terminal-op CFE_PSP_Restart --terminal-op CFE_PSP_Panic
    --terminal-op abort)
P0OPS=(--spawn-op pthread_create --lock-op pthread_mutex_lock --unlock-op pthread_mutex_unlock --delay-op clock_nanosleep
       --delay-op nanosleep --terminal-op pthread_exit --terminal-op exit --terminal-op abort)

# build -> compile db, build dir, bundle root, recipe
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
BUILDS=(c1_pre_tx c1_post_tx r2_core c5_641 c5_642)

# one run: basis + extract + show into D.  Arguments: D cdb build_dir bundle_root recipe -- basis-and-extract args -- extract-only args
one() {
  local D=$1 cdb=$2 bd=$3 br=$4 rc=$5; shift 5
  local both=() ext=() sep=0
  for a in "$@"; do [ "$a" = "--" ] && { sep=$((sep + 1)); continue; }; [ $sep -le 1 ] && both+=("$a") || ext+=("$a"); done
  mkdir -p "$D"
  local t0=$(date +%s.%N)
  "$KS" basis "${both[@]}" --compile-db "$cdb" --build-dir "$bd" --bundle-root "$br" --recipe "$rc" --text -o "$D/basis.kz" > "$D/basis.txt" 2>&1; local x=$?
  local t1=$(date +%s.%N)
  "$KS" extract "${both[@]}" --basis "$D/basis.kz" "${ext[@]}" -o "$D/facts.kz" > "$D/extract.txt" 2>&1; local y=$?
  local t2=$(date +%s.%N)
  "$KS" show "$D/facts.kz" > "$D/facts.txt" 2>&1; local z=$?
  printf '%s\n' "basis/extract args: ${both[*]}" "extract-only args: ${ext[*]}" "compile db: $cdb" "build dir: $bd" "bundle root: $br" "recipe: $rc" > "$D/args.txt"
  ( cd "$D" && sha256sum basis.kz facts.kz "$cdb" "$rc" "$KS" > sha256.txt )
  printf '%s basis=%d extract=%d show=%d basis_s=%.1f extract_s=%.1f\n' "$D" $x $y $z "$(echo "$t1 - $t0" | bc)" "$(echo "$t2 - $t1" | bc)" | tee -a "$OUT/run.log"
}

# the kz-dump text of a container without the lines that name the tool, the whole artifact or its size
dumpn() { "$KZD" "$1" | grep -vE '^(tool_sha256|artifact_sha256)=|^container '; }

identity() {
  local D=$OUT/identity; mkdir -p "$D"; : > "$D/identity.txt"
  {
    echo "# step2a_s1.sh identity $(date -u +%FT%TZ): the profile-driven mode of the new kz-static against the outputs of extract_baseline.sh"
    echo "repo $(git -C "$R" rev-parse HEAD) kairos_uncommitted_lines=$(git -C "$R" diff -- kairos | wc -l)"
    "$KS" --identity
    echo "earlier tool: $(grep -m1 '^tool_sha256=' "$EB/inputs/inputs.record")"
  } >> "$D/identity.txt"
  for b in c1_pre_tx r2_core c5_641; do
    for v in V0 V2; do
      local A=$EB/$b/$v/args.txt
      local prof=($(sed -n 's/^profile: //p' "$A")) ext=($(sed -n 's/^extract args: //p' "$A"))
      local cdb=$(sed -n 's/^compile db: //p' "$A") bd=$(sed -n 's/^build dir: //p' "$A") br=$(sed -n 's/^bundle root: //p' "$A") rc=$(sed -n 's/^recipe: //p' "$A")
      one "$D/$b/$v" "$cdb" "$bd" "$br" "$rc" -- "${prof[@]}" -- "${ext[@]}"
      for art in basis facts; do
        dumpn "$EB/$b/$v/$art.kz" > "$D/$b/$v/$art.old.dump"
        dumpn "$D/$b/$v/$art.kz" > "$D/$b/$v/$art.new.dump"
        diff "$D/$b/$v/$art.old.dump" "$D/$b/$v/$art.new.dump" > "$D/$b/$v/$art.dump.diff"
        # sections whose CRC differs, and the record lines that differ
        local secs=$(diff <(grep '^section ' "$D/$b/$v/$art.old.dump") <(grep '^section ' "$D/$b/$v/$art.new.dump") | sed -n 's/^[<>] section \([A-Z_0-9]*\) .*/\1/p' | sort -u | tr '\n' ' ')
        local nsec=$(grep -c '^section ' "$D/$b/$v/$art.old.dump") nsec2=$(grep -c '^section ' "$D/$b/$v/$art.new.dump")
        local rec=$(grep -c '^[<>]   ' "$D/$b/$v/$art.dump.diff")
        echo "$b $v $art: sections old $nsec new $nsec2; differing sections: ${secs:-none}; differing record lines: $rec" | tee -a "$D/identity.txt"
        grep '^[<>]   ' "$D/$b/$v/$art.dump.diff" | cut -c1-400 | sed 's/^/    /' >> "$D/identity.txt"
      done
      # the show text without the build-identity item
      diff <(grep -v 'build_identity' "$EB/$b/$v/facts.txt") <(grep -v 'build_identity' "$D/$b/$v/facts.txt") > "$D/$b/$v/show.diff"
      echo "$b $v show text (build_identity line removed): diff lines $(grep -c '^[<>]' "$D/$b/$v/show.diff")" | tee -a "$D/identity.txt"
    done
  done
}

run() {
  : > "$OUT/run.log"
  for b in "${BUILDS[@]}"; do
    local in=($(inputs_of "$b"))
    one "$OUT/$b/NP" "${in[@]}" -- --no-profile --
  done
  local in=($(inputs_of c5_642))
  one "$OUT/c5_642/VP" "${in[@]}" -- --profile "$EB/inputs/bare" -- "${P0OPS[@]}"
  one "$OUT/c5_642/V1" "${in[@]}" -- --profile "$EB/inputs/bare" -- "${G0[@]}"
}

twice() {
  for b in "${BUILDS[@]}"; do
    local D=$OUT/$b/NP2; mkdir -p "$D"
    "$KS" extract --no-profile --basis "$OUT/$b/NP/basis.kz" -o "$D/facts.kz" > "$D/extract.txt" 2>&1
    cmp -s "$OUT/$b/NP/facts.kz" "$D/facts.kz" && r=same || r=DIFFERENT
    echo "$b facts twice: $r ($(sha256sum "$D/facts.kz" | cut -c1-16))" | tee -a "$OUT/twice.txt"
  done
}

compare() {
  mkdir -p "$OUT/compare"
  python3 -I "$HERE/step2a_s1_compare.py" "$OUT" "$EB" "$OUT/compare"
  ( cd "$OUT" && sha256sum */NP/basis.kz */NP/facts.kz c5_642/*/facts.kz compare/*.tsv compare/*.txt "$HERE/step2a_s1.sh" "$HERE/step2a_s1_compare.py" "$KS" > sha256.txt )
}

case "$STEP" in
  identity) identity ;;
  run) run ;;
  twice) twice ;;
  compare) compare ;;
  all) identity && run && twice && compare ;;
  *) echo "unknown step $STEP" >&2; exit 64 ;;
esac

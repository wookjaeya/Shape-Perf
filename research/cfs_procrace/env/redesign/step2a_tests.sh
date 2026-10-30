#!/usr/bin/env bash
# step2a_tests.sh STEP [OUT] -- redesign step 2a, stage S5 and its fix stage: the completion tests T1-T7 of
# plan/extraction_design_20261030.md §6, run with no case input (the two-statement built-in profile, P0, L0 and the
# build only; no G0), with the rule targets of ED §6 item 2 (R-T1 ∪ R-T2, shared bases) and `kz-static relations`
# (ED §6 item 3).  Static analysis only: no cFS is booted, no campaign is run.
#
#   STEP   identity  the profile-driven mode is unchanged: the V0 and V2 variants of extract_baseline.sh, re-run with this
#                    kz-static from their recorded arguments (EB/<build>/<variant>/args.txt) for c1_pre_tx, r2_core and
#                    c5_641; basis and facts compared section by section with the step-2 outputs (kz-dump) -> OUT/identity/
#          npsame    the profile-free mode without --targets-rule against S4: extract --no-profile from the S4 basis of
#                    every build, its records compared with S4/<build>/NP/facts.kz -> OUT/npsame/ (after the fix stage:
#                    BASIS and P0_TABLE differ by the P0 table revision 2, LOCK_PATHS is new; every other section the same)
#          s5same    the fix stage against S5: the records of OUT/<build>/NP/facts.kz (step run) compared with those of
#                    S5/<build>/NP/facts.kz, section by section -> OUT/s5same/ (BASIS, P0_TABLE and the new LOCK_PATHS
#                    may differ; the other sections are the same when the fix changed no extraction fact)
#          run       basis --no-profile + extract --no-profile --targets-rule R-T1,R-T2 + show + relations for every build
#                    below -> OUT/<build>/NP/ (relations.txt is the query's output)
#          twice     T7: extract again from the same basis and relations again; bytes compared -> OUT/twice.txt
#          count     step2a_tests_count.py: T1-T7 evidence with line references into relations.txt, the re-measure of
#                    ED §2.3 under the rule targets, the candidate burden (rows per build, case rows among them); case
#                    names are used in the queries of this script only -> OUT/count/
#          sha       sha256 of every output -> OUT/sha256.txt
#          all       identity npsame run s5same twice count sha
#   OUT    default /home/user/work/redesign/step2a/tests
#
# Builds (reference builds only), with the inputs of step2a_s1.sh .. step2a_s4.sh:
#   c1_pre_tx c1_post_tx   procrace/c1_build/<pre|post>/1df262a, recipe EB/inputs/c1_<cfg>_tx.kzb
#   r2_core                procrace/cfs_ref/cFS/build-native_std core-cpu1, recipe EB/inputs/r2_core_min.kzb
#   c5_641 c5_642          c5tools/build/64x/keep (E-ST compile database c5tools/build/64x/est/compile_commands.json),
#                          recipe profiles/c5-cfe-6.4.x/basis/core-linux.kzb (for 6.4.1 the copy EB/inputs/c5_641.kzb)
# The profile-free runs take no --profile, no vocabulary option and no --target list: --targets-rule is a rule of the
# tool, not a list.  The recipes keep their startup-script block (L0) and, for C5, the two front_end_diagnostic lines
# (PI D7); nothing else of a case enters.  The `identity` step uses the case profiles (V2) only to show that the
# profile-driven mode is unchanged.
set -uo pipefail
STEP=${1:?identity|npsame|run|twice|count|sha|all}
OUT=${2:-/home/user/work/redesign/step2a/tests}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
K=$R/kairos
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
KZD=$KZB/kz-dump/kz-dump
EB=${EB:-/home/user/work/redesign/extract_baseline}
S4=${S4:-/home/user/work/redesign/step2a/s4}
S5=${S5:-/home/user/work/redesign/step2a/tests_s5}
C1=/home/user/work/procrace/c1_build
REF=/home/user/work/procrace/cfs_ref/cFS
CT=/home/user/work/c5tools/build
[ -e /home/user/work/.campaign_running ] && { echo "a campaign is running (/home/user/work/.campaign_running)" >&2; exit 3; }
mkdir -p "$OUT"
RULE=(--targets-rule R-T1,R-T2)

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

# one run: basis + extract + show (+ relations for the profile-free mode) into D.
# Arguments: D cdb build_dir bundle_root recipe -- basis-and-extract args -- extract-only args
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
  local w=-
  if [ "${both[0]:-}" = "--no-profile" ]; then "$KS" relations "$D/facts.kz" -o "$D/relations.txt" > "$D/relations.log" 2>&1; w=$?; fi
  local t3=$(date +%s.%N)
  printf '%s\n' "basis/extract args: ${both[*]}" "extract-only args: ${ext[*]}" "compile db: $cdb" "build dir: $bd" "bundle root: $br" "recipe: $rc" > "$D/args.txt"
  ( cd "$D" && sha256sum basis.kz facts.kz "$cdb" "$rc" "$KS" > sha256.txt )
  printf '%s basis=%d extract=%d show=%d relations=%s basis_s=%.1f extract_s=%.1f relations_s=%.1f\n' "$D" $x $y $z "$w" "$(echo "$t1 - $t0" | bc)" "$(echo "$t2 - $t1" | bc)" \
    "$(echo "$t3 - $t2" | bc)" | tee -a "$OUT/run.log"
}

dumpn() { "$KZD" "$1" | grep -vE '^(tool_sha256|artifact_sha256)=|^container '; }

# the record lines of two kz-dump texts, section by section: A B
compare_records() {
  python3 -I - "$@" <<'PY'
import re, sys
def sections(path):
    secs, cur = {}, None
    for line in open(path, encoding='utf-8', errors='replace'):
        m = re.match(r'section (\S+) ', line)
        if m:
            cur = m.group(1); secs[cur] = []; continue
        if cur and line.startswith('  '):
            secs[cur].append(re.sub(r'^  \S+\[\d+\] ', '', line.rstrip('\n')))
    return secs
a, b = sections(sys.argv[1]), sections(sys.argv[2])
same, bad = [], []
for s, rows in sorted(a.items()):
    if s == 'STRINGS':
        continue
    other = b.get(s, [])
    if rows == other:
        same.append(s)
    else:
        diff = [x for x, y in zip(rows, other) if x != y]
        bad.append('%s (%d of %d records differ%s)' % (s, len(diff) + abs(len(rows) - len(other)), len(rows),
                   '; ' + ' | '.join(re.sub(r'\s+', ' ', d)[:160] for d in diff[:3]) if diff else ''))
print('sections with the same records: %d (%s)' % (len(same), ' '.join(same)))
print('sections only in the second: %s; DIFFERENT: %s' % (' '.join(sorted(set(b) - set(a))) or '-', '; '.join(bad) or 'none'))
PY
}

identity() {
  local D=$OUT/identity; mkdir -p "$D"; : > "$D/identity.txt"
  {
    echo "# step2a_tests.sh identity $(date -u +%FT%TZ): the profile-driven mode of this kz-static against the outputs of extract_baseline.sh"
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
        local secs=$(diff <(grep '^section ' "$D/$b/$v/$art.old.dump") <(grep '^section ' "$D/$b/$v/$art.new.dump") | sed -n 's/^[<>] section \([A-Z_0-9]*\) .*/\1/p' | sort -u | tr '\n' ' ')
        local nsec=$(grep -c '^section ' "$D/$b/$v/$art.old.dump") nsec2=$(grep -c '^section ' "$D/$b/$v/$art.new.dump")
        local rec=$(grep -c '^[<>]   ' "$D/$b/$v/$art.dump.diff")
        echo "$b $v $art: sections old $nsec new $nsec2; sections whose CRC differs: ${secs:-none}; differing record lines: $rec" | tee -a "$D/identity.txt"
        grep '^[<>]   ' "$D/$b/$v/$art.dump.diff" | cut -c1-400 | sed 's/^/    /' >> "$D/identity.txt"
      done
      diff <(grep -v 'build_identity' "$EB/$b/$v/facts.txt") <(grep -v 'build_identity' "$D/$b/$v/facts.txt") > "$D/$b/$v/show.diff"
      echo "$b $v show text (build_identity line removed): diff lines $(grep -c '^[<>]' "$D/$b/$v/show.diff")" | tee -a "$D/identity.txt"
      rm -f "$D/$b/$v/basis.old.dump" "$D/$b/$v/basis.new.dump" "$D/$b/$v/facts.old.dump" "$D/$b/$v/facts.new.dump"
    done
  done
}

npsame() {
  local D=$OUT/npsame; mkdir -p "$D"; : > "$D/npsame.txt"
  for b in "${BUILDS[@]}"; do
    mkdir -p "$D/$b"
    "$KS" extract --no-profile --basis "$S4/$b/NP/basis.kz" -o "$D/$b/facts.kz" > "$D/$b/extract.txt" 2>&1
    dumpn "$S4/$b/NP/facts.kz" > "$D/$b/s4.dump"
    dumpn "$D/$b/facts.kz" > "$D/$b/s5.dump"
    compare_records "$D/$b/s4.dump" "$D/$b/s5.dump" | sed "s/^/$b: /" | tee -a "$D/npsame.txt"
    rm -f "$D/$b/s4.dump" "$D/$b/s5.dump" "$D/$b/facts.kz"
  done
}

s5same() {
  local D=$OUT/s5same; mkdir -p "$D"; : > "$D/s5same.txt"
  for b in "${BUILDS[@]}"; do
    mkdir -p "$D/$b"
    dumpn "$S5/$b/NP/facts.kz" > "$D/$b/s5.dump"
    dumpn "$OUT/$b/NP/facts.kz" > "$D/$b/fix.dump"
    compare_records "$D/$b/s5.dump" "$D/$b/fix.dump" | sed "s/^/$b: /" | tee -a "$D/s5same.txt"
    rm -f "$D/$b/s5.dump" "$D/$b/fix.dump"
  done
}

run() {
  : > "$OUT/run.log"
  for b in "${BUILDS[@]}"; do
    local in=($(inputs_of "$b"))
    one "$OUT/$b/NP" "${in[@]}" -- --no-profile -- "${RULE[@]}"
  done
}

twice() {
  : > "$OUT/twice.txt"
  for b in "${BUILDS[@]}"; do
    local D=$OUT/$b/NP2; mkdir -p "$D"
    "$KS" extract --no-profile --basis "$OUT/$b/NP/basis.kz" "${RULE[@]}" -o "$D/facts.kz" > "$D/extract.txt" 2>&1
    "$KS" relations "$D/facts.kz" -o "$D/relations.txt" > /dev/null 2>&1
    cmp -s "$OUT/$b/NP/facts.kz" "$D/facts.kz" && f=same || f=DIFFERENT
    cmp -s "$OUT/$b/NP/relations.txt" "$D/relations.txt" && r=same || r=DIFFERENT
    echo "$b facts twice: $f ($(sha256sum "$D/facts.kz" | cut -c1-16)); relations twice: $r ($(sha256sum "$D/relations.txt" | cut -c1-16))" | tee -a "$OUT/twice.txt"
    rm -f "$D/facts.kz"
  done
}

count() {
  mkdir -p "$OUT/count"
  python3 -I "$HERE/step2a_tests_count.py" "$OUT" "$EB" "$S5" | tee "$OUT/count/summary.txt"
}

sha() {
  ( cd "$OUT" && find . -type f ! -name sha256.txt -print0 | sort -z | xargs -0 sha256sum > sha256.txt )
  echo "sha256 of $(wc -l < "$OUT/sha256.txt") files -> $OUT/sha256.txt"
}

case "$STEP" in
  identity) identity ;;
  npsame) npsame ;;
  s5same) s5same ;;
  run) run ;;
  twice) twice ;;
  count) count ;;
  sha) sha ;;
  all) identity; npsame; run; s5same; twice; count; sha ;;
  *) echo "unknown step $STEP" >&2; exit 2 ;;
esac

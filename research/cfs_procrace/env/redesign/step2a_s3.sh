#!/usr/bin/env bash
# step2a_s3.sh STEP [OUT] -- redesign step 2a, stage S3 (plan/extraction_design_20261030.md §3.0 F4 and F6, §5 R1): the
# extended facts ARGFLOW, PARAM_FLOW, PTR_FIELDS (F4) and GLOBAL_IMAGES (F6) of kz-static, measured on the reference builds
# in the profile-free mode, with the sensitivity of the counts to the tool parameters of the composition.  Static analysis
# only: no cFS is booted, no campaign is run.
#
#   STEP   identity  the profile-driven mode is unchanged: the V0 and V2 variants of extract_baseline.sh, re-run with this
#                    kz-static from their recorded arguments (EB/<build>/<variant>/args.txt) for c1_pre_tx, r2_core and
#                    c5_641, basis and facts compared section by section with the earlier outputs (kz-dump: the section
#                    lines with their CRC and every record line); then the same extraction with --extended-facts from the
#                    same basis, its records compared with the run without it (every earlier section record by record,
#                    FACT_EXPRS as a prefix, BASIS as a subset) and its sections READS, RETURNS, RET_CONSTS, LOOP_EXITS with
#                    the S2 run of the same option (S2/identity/<build>/<variant>x/facts.dump) -> OUT/identity/
#          run       basis --no-profile + extract --no-profile + show for every build below -> OUT/<build>/NP/ (the four
#                    new sections also in argflow.txt, paramflow.txt, ptrfields.txt, gimages.txt)
#          earlier   the sections of S1 and S2 in the profile-free facts: records compared with S2/<build>/NP/facts.kz
#                    (BASIS as a subset, FACT_EXPRS as a prefix) -> OUT/count/earlier.txt
#          twice     determinism: extract --no-profile again from the same basis; facts bytes compared -> OUT/<build>/NP2/
#          bounds    the tool parameters no reference fixes, each value its own extraction from the same basis:
#                    --argflow-depth 1 2 4 8, --argflow-size 4 16 64, --image-budget 64 1024 (the defaults are none, none,
#                    4096: the run step).  Only the counts are kept (the containers are removed) -> OUT/<build>/bounds/
#          count     step2a_s3_count.py: counts per new section, the relation counts and their sensitivity to the bounds,
#                    the expression-depth check, and the spot checks (the C1 BP key through the pointer field and the
#                    image; the items ED §6 T1-T4 will need; case names in the queries only) -> OUT/count/
#          all       identity run earlier twice bounds count
#   OUT    default /home/user/work/redesign/step2a/s3
#
# Builds (reference builds only), with the inputs of step2a_s1.sh and step2a_s2.sh (extract_baseline.sh's VP inputs):
#   c1_pre_tx c1_post_tx   procrace/c1_build/<pre|post>/1df262a, recipe EB/inputs/c1_<cfg>_tx.kzb
#   r2_core                procrace/cfs_ref/cFS/build-native_std core-cpu1, recipe EB/inputs/r2_core_min.kzb
#   c5_641 c5_642          c5tools/build/64x/keep (E-ST compile database c5tools/build/64x/est/compile_commands.json),
#                          recipe profiles/c5-cfe-6.4.x/basis/core-linux.kzb (for 6.4.1 the copy EB/inputs/c5_641.kzb)
# The profile-free runs take no --profile and no vocabulary option.  The recipes keep their startup-script block (L0) and,
# for C5, the two front_end_diagnostic lines (PI D7); nothing else of a case enters.
set -uo pipefail
STEP=${1:?identity|run|earlier|twice|bounds|count|all}
OUT=${2:-/home/user/work/redesign/step2a/s3}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
K=$R/kairos
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
KZD=$KZB/kz-dump/kz-dump
EB=${EB:-/home/user/work/redesign/extract_baseline}
S2=${S2:-/home/user/work/redesign/step2a/s2}
C1=/home/user/work/procrace/c1_build
REF=/home/user/work/procrace/cfs_ref/cFS
CT=/home/user/work/c5tools/build
[ -e /home/user/work/.campaign_running ] && { echo "a campaign is running (/home/user/work/.campaign_running)" >&2; exit 3; }
mkdir -p "$OUT"

# build -> compile db, build dir, bundle root, recipe (as step2a_s1.sh)
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

# compare the record lines of two kz-dump texts section by section: A B [S2-dump] -> report lines
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
new = sorted(set(b) - set(a))
same, prefix, subset, bad = [], [], [], []
for s, rows in a.items():
    if s == 'STRINGS':
        continue
    other = b.get(s, [])
    if rows == other:
        same.append(s)
    elif s == 'FACT_EXPRS' and other[:len(rows)] == rows:
        prefix.append('%s (%d earlier rows a prefix of %d)' % (s, len(rows), len(other)))
    elif s == 'BASIS' and all(r in other for r in rows if not re.search(r'name="extract\.(build_identity|units_from)"', r)):
        moved = [re.search(r'name="([^"]*)"', r).group(1) for r in rows if r not in other]
        subset.append('%s (%d earlier items among %d%s)' % (s, len(rows), len(other), '; differing: ' + ', '.join(moved) if moved else ''))
    else:
        bad.append(s)
print('earlier sections with the same records: %d (%s)' % (len(same), ' '.join(sorted(same))))
print('prefix: %s; subset: %s; new sections: %s; DIFFERENT: %s' % ('; '.join(prefix) or '-', '; '.join(subset) or '-', ' '.join(new) or '-', ' '.join(bad) or 'none'))
if len(sys.argv) > 3:
    c = sections(sys.argv[3])
    for s in ('READS', 'RETURNS', 'RET_CONSTS', 'LOOP_EXITS'):
        print('%s against the S2 run of the same option: %s (%d / %d records)' % (s, 'same' if c.get(s) == b.get(s) else 'DIFFERENT', len(b.get(s, [])), len(c.get(s, []))))
PY
}

identity() {
  local D=$OUT/identity; mkdir -p "$D"; : > "$D/identity.txt"
  {
    echo "# step2a_s3.sh identity $(date -u +%FT%TZ): the profile-driven mode of the new kz-static against the outputs of extract_baseline.sh"
    echo "repo $(git -C "$R" rev-parse HEAD) kairos_uncommitted_lines=$(git -C "$R" diff -- kairos | wc -l)"
    "$KS" --identity
    echo "earlier tool: $(grep -m1 '^tool_sha256=' "$EB/inputs/inputs.record")"
  } >> "$D/identity.txt"
  for b in c1_pre_tx r2_core c5_641; do
    for v in V0 V2; do
      local A=$EB/$b/$v/args.txt
      local prof=($(sed -n 's/^profile: //p' "$A")) ext=($(sed -n 's/^extract args: //p' "$A"))
      local cdb=$(sed -n 's/^compile db: //p' "$A") bd=$(sed -n 's/^build dir: //p' "$A") br=$(sed -n 's/^bundle root: //p' "$A") rc=$(sed -n 's/^recipe: //p' "$A")
      # (1) the profile-driven mode as before: against the earlier tool's outputs
      one "$D/$b/$v" "$cdb" "$bd" "$br" "$rc" -- "${prof[@]}" -- "${ext[@]}"
      for art in basis facts; do
        dumpn "$EB/$b/$v/$art.kz" > "$D/$b/$v/$art.old.dump"
        dumpn "$D/$b/$v/$art.kz" > "$D/$b/$v/$art.new.dump"
        diff "$D/$b/$v/$art.old.dump" "$D/$b/$v/$art.new.dump" > "$D/$b/$v/$art.dump.diff"
        local secs=$(diff <(grep '^section ' "$D/$b/$v/$art.old.dump") <(grep '^section ' "$D/$b/$v/$art.new.dump") | sed -n 's/^[<>] section \([A-Z_0-9]*\) .*/\1/p' | sort -u | tr '\n' ' ')
        local nsec=$(grep -c '^section ' "$D/$b/$v/$art.old.dump") nsec2=$(grep -c '^section ' "$D/$b/$v/$art.new.dump")
        local rec=$(grep -c '^[<>]   ' "$D/$b/$v/$art.dump.diff")
        echo "$b $v $art: sections old $nsec new $nsec2; differing sections: ${secs:-none}; differing record lines: $rec" | tee -a "$D/identity.txt"
        grep '^[<>]   ' "$D/$b/$v/$art.dump.diff" | cut -c1-400 | sed 's/^/    /' >> "$D/identity.txt"
      done
      diff <(grep -v 'build_identity' "$EB/$b/$v/facts.txt") <(grep -v 'build_identity' "$D/$b/$v/facts.txt") > "$D/$b/$v/show.diff"
      echo "$b $v show text (build_identity line removed): diff lines $(grep -c '^[<>]' "$D/$b/$v/show.diff")" | tee -a "$D/identity.txt"
      # (2) the same extraction with --extended-facts from the same basis
      local X=$D/$b/${v}x; mkdir -p "$X"
      "$KS" extract "${prof[@]}" --basis "$D/$b/$v/basis.kz" "${ext[@]}" --extended-facts -o "$X/facts.kz" > "$X/extract.txt" 2>&1
      dumpn "$X/facts.kz" > "$X/facts.dump"
      compare_records "$D/$b/$v/facts.new.dump" "$X/facts.dump" "$S2/identity/$b/${v}x/facts.dump" | sed "s/^/$b ${v}x: /" | tee -a "$D/identity.txt"
      rm -f "$D/$b/$v/basis.old.dump" "$D/$b/$v/basis.new.dump" "$D/$b/$v/facts.old.dump" "$D/$b/$v/facts.new.dump" "$X/facts.dump"
    done
  done
}

run() {
  : > "$OUT/run.log"
  for b in "${BUILDS[@]}"; do
    local in=($(inputs_of "$b"))
    one "$OUT/$b/NP" "${in[@]}" -- --no-profile --
    for s in argflow paramflow ptrfields gimages; do "$KS" show "$OUT/$b/NP/facts.kz" --section "$s" > "$OUT/$b/NP/$s.txt"; done
  done
}

earlier() {
  mkdir -p "$OUT/count"; : > "$OUT/count/earlier.txt"
  for b in "${BUILDS[@]}"; do
    local T=$OUT/$b/NP
    dumpn "$S2/$b/NP/facts.kz" > "$T/s2.dump"
    dumpn "$T/facts.kz" > "$T/s3.dump"
    compare_records "$T/s2.dump" "$T/s3.dump" | sed "s/^/$b: /" | tee -a "$OUT/count/earlier.txt"
    rm -f "$T/s2.dump" "$T/s3.dump"
  done
}

twice() {
  : > "$OUT/twice.txt"
  for b in "${BUILDS[@]}"; do
    local D=$OUT/$b/NP2; mkdir -p "$D"
    "$KS" extract --no-profile --basis "$OUT/$b/NP/basis.kz" -o "$D/facts.kz" > "$D/extract.txt" 2>&1
    cmp -s "$OUT/$b/NP/facts.kz" "$D/facts.kz" && r=same || r=DIFFERENT
    echo "$b facts twice: $r ($(sha256sum "$D/facts.kz" | cut -c1-16))" | tee -a "$OUT/twice.txt"
    rm -f "$D/facts.kz"
  done
}

bounds() {
  : > "$OUT/bounds.log"
  for b in "${BUILDS[@]}"; do
    local B=$OUT/$b/bounds; mkdir -p "$B"
    for opt in "--argflow-depth 1" "--argflow-depth 2" "--argflow-depth 4" "--argflow-depth 8" "--argflow-size 4" "--argflow-size 16" "--argflow-size 64" \
               "--image-budget 64" "--image-budget 1024"; do
      local tag=$(echo "$opt" | sed 's/^--//; s/ /_/')
      local t0=$(date +%s.%N)
      "$KS" extract --no-profile --basis "$OUT/$b/NP/basis.kz" $opt -o "$B/$tag.kz" > "$B/$tag.extract.txt" 2>&1; local y=$?
      local t1=$(date +%s.%N)
      python3 -I "$HERE/step2a_s3_count.py" summarize "$KZD" "$B/$tag.kz" > "$B/$tag.json"
      echo "$b $opt extract=$y extract_s=$(echo "$t1 - $t0" | bc) $(sha256sum "$B/$tag.kz" | cut -c1-16)" | tee -a "$OUT/bounds.log"
      rm -f "$B/$tag.kz"
    done
  done
}

count() {
  mkdir -p "$OUT/count"
  for b in "${BUILDS[@]}"; do
    "$KZD" "$OUT/$b/NP/facts.kz" | grep -c 'FACT_EXPRS\[[0-9]*\] op=top .*name="too deep"' > "$OUT/$b/NP/too_deep.txt"
    python3 -I "$HERE/step2a_s3_count.py" summarize "$KZD" "$OUT/$b/NP/facts.kz" > "$OUT/$b/NP/summary.json"
  done
  python3 -I "$HERE/step2a_s3_count.py" report "$KZD" "$OUT" "$OUT/count"
  ( cd "$OUT" && sha256sum */NP/basis.kz */NP/facts.kz */NP/summary.json */bounds/*.json count/*.tsv count/*.txt "$HERE/step2a_s3.sh" "$HERE/step2a_s3_count.py" "$KS" > sha256.txt )
}

case "$STEP" in
  identity) identity ;;
  run) run ;;
  earlier) earlier ;;
  twice) twice ;;
  bounds) bounds ;;
  count) count ;;
  all) identity && run && earlier && twice && bounds && count ;;
  *) echo "unknown step $STEP" >&2; exit 64 ;;
esac

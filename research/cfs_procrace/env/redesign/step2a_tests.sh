#!/usr/bin/env bash
# step2a_tests.sh STEP [OUT] -- redesign step 2a, stage S5 and its fix stages (fix, fix2, fix3 and its second round): the completion tests T1-T7 of
# plan/extraction_design_20261030.md §6, run with no case input (the two-statement built-in profile, P0, L0 and the
# build only; no G0), with the rule targets of ED §6 item 2 (R-T1 ∪ R-T2, shared bases) and `kz-static relations`
# (ED §6 item 3).  Static analysis only: no cFS is booted, no campaign is run.
#
#   STEP   identity  the profile-driven mode is unchanged: the V0 and V2 variants of extract_baseline.sh, re-run with this
#                    kz-static from their recorded arguments (EB/<build>/<variant>/args.txt) for c1_pre_tx, r2_core and
#                    c5_641; basis and facts compared record by record with the step-2 outputs (kz-dump; ALLOW) -> OUT/identity/
#          npsame    the profile-free mode without --targets-rule against S4: extract --no-profile from the S4 basis of
#                    every build, its records compared with S4/<build>/NP/facts.kz -> OUT/npsame/ (after the fix stage:
#                    BASIS and P0_TABLE differ by the P0 table revision 2, LOCK_PATHS is new; every other section the same)
#          s5same    the fix stage against S5: the records of OUT/<build>/NP/facts.kz (step run) compared with those of
#                    S5/<build>/NP/facts.kz, section by section -> OUT/s5same/ (BASIS, P0_TABLE and the new LOCK_PATHS
#                    may differ; the other sections are the same when the fix changed no extraction fact)
#          fixsame   the second fix stage (fix2) against the first: the records of OUT/<build>/NP/facts.kz compared with
#                    those of FIX1/<build>/NP/facts.kz (the outputs of the first fix stage, kept) -> OUT/fixsame/ (BASIS and
#                    P0_TABLE differ by the P0 table revision 3 and the walk-bound records; every other section the same)
#          fix2same  the third fix stage (fix3) against the second: the records of OUT/<build>/NP/facts.kz compared with
#                    those of FIX2/<build>/NP/facts.kz (the outputs of the second fix stage, kept) -> OUT/fix2same/ (BASIS:
#                    the walk-bound records and the build identity; LOCK_PATHS: entry-release rows of the condition waits on a
#                    lock not held; LOCKSET: the releases through an overlapping identity; FIELD_STATUS (and LOCK_IDS,
#                    AWAIT_L1 through it): the arrays of scalars)
#          fix3same  the second round of the third fix stage against its first: the records of OUT/<build>/NP/facts.kz
#                    compared with those of FIX3/<build>/NP/facts.kz (the outputs of the first round of fix3, kept) ->
#                    OUT/fix3same/ (BASIS: the walk-bound and incomplete-source records and the build identity;
#                    CALL_TARGETS: the incomplete-source rows; LOCKSET, LOCK_PATHS, LOCK_OPS: their lock effect; LOCK_IDS:
#                    objects of internal linkage named in several units)
#          run       basis --no-profile + extract --no-profile --targets-rule R-T1,R-T2 + show + relations for every build
#                    below -> OUT/<build>/NP/ (relations.txt is the query's output)
#          twice     T7: extract again from the same basis and relations again; bytes compared -> OUT/twice.txt (NP2 kept)
#          count     step2a_tests_count.py: T1-T7 evidence with line references into relations.txt, the re-measure of
#                    ED §2.3 under the rule targets, the candidate burden (rows per build, case rows among them); case
#                    names are used in the queries of this script only -> OUT/count/
#          sha       sha256 of every output except status.txt -> OUT/sha256.txt
#          all       identity npsame run s5same fixsame fix2same fix3same twice count sha; OUT/status.txt; rc 1 if a step fails
#   OUT    default: all -> new step2a/tests_runs/run_<UTC>_<kz-static sha 8> (+ link latest), other steps -> latest; never T/ or an OUT with sha256.txt (STEP2A_OVERWRITE=1)
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
STEP=${1:-}; [ -n "$STEP" ] || { echo "usage: step2a_tests.sh identity|npsame|run|s5same|fixsame|fix2same|fix3same|twice|count|sha|all [OUT]" >&2; exit 2; }
OUT=${2:-}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
K=$R/kairos
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
KZD=$KZB/kz-dump/kz-dump
EB=${EB:-/home/user/work/redesign/extract_baseline}
S4=${S4:-/home/user/work/redesign/step2a/s4}
S5=${S5:-/home/user/work/redesign/step2a/tests_s5}
FIX1=${FIX1:-/home/user/work/redesign/step2a/tests_fix1}
FIX2=${FIX2:-/home/user/work/redesign/step2a/tests_fix2}
FIX3=${FIX3:-/home/user/work/redesign/step2a/tests_fix3}
C1=${C1:-/home/user/work/procrace/c1_build}
REF=${REF:-/home/user/work/procrace/cfs_ref/cFS}
CT=${CT:-/home/user/work/c5tools/build}
[ -e /home/user/work/.campaign_running ] && { echo "a campaign is running (/home/user/work/.campaign_running); the driver does not start" >&2; exit 2; }
# OUT (made here before; see main at the end), status.txt and the failure rules: after the functions
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

# ---------------------------------------------------------------- failures (RV3A §2.4, RV4 analysis §2 #29)
# Every status is read in the shell that runs the step, never inside a pipeline or a subshell that loses it: the
# statuses of a pipeline come from PIPESTATUS right after it (pipe_ok), a command substitution's from its assignment.
# A failure is a line "FAIL <step>: ..." in OUT/status.txt and on stderr; a step with a failure is FAIL; `all` and a
# single step exit 1 when a step failed, 2 when the driver could not start (no or an unknown STEP, no tool, a campaign
# running, OUT not writable, or OUT refused: `all` needs a new or empty OUT, and no step writes into T/ or into an OUT
# that holds a top-level sha256.txt unless STEP2A_OVERWRITE=1).  Failures:
#   - a non-zero rc of kz-static, kz-dump, sha256sum, sed, tee, sort, xargs, python3 or a write (grep and diff: 2 or more)
#   - a .txt output that is missing or empty (the size of each is recorded: run.log, status.txt)
#   - kz-dump failing, a dump with no section, or a section whose record lines differ from its head's count (cut)
#   - a comparison (identity, npsame, s5same, fixsame, fix2same, fix3same) that does not meet the table ALLOW below:
#     the records of the two dumps are compared section by section; a differing record is changed (same identity,
#     IDENT, other fields), added or removed, and must be named by exactly one entry of the step (section, change,
#     fields, key); each entry's count for the build must be met exactly.  The allowance is per record and, for a
#     changed record, per field: the fields that differ between its old and new record (name=value pairs; `<text>` for
#     text outside them) must be exactly the entry's field list, so a record that also changes another field fails.
#     A record of an allowed section that no entry names fails, and so does a count that is off by one.  A section in
#     one dump only fails unless a HEADER `added`/`removed` entry names its head with an exact count (an exception to
#     RV4 analysis §2 #29 (다), which fails every one-sided section: LOCK_PATHS is new since the first fix stage).
#     No compared section fails.  Each comparison also prints the field signatures of its changed records
#     ("fields SECTION changed {f,..} key K: n") so that the table can be measured again.
#   - twice: facts or relations not the same; count: step2a_tests_count.py exits 1 (an input missing or empty, a
#     twice line not "same")
# STEP2A_ALLOW_TABLE=FILE replaces the table; only the unit checks (step2a_tests_unit.sh) do that, and status.txt and
# every comparison output then say so.  The counts of the table were measured on 2026-11-09 with kz-static db172b74
# from the kept outputs (T/ against EB, T_S5, T_F1, T_F2, T_F3; npsame: S4 re-extracted by it) [M]; they agree with
# AR §2a where AR gives a number (incomplete-source 44 / 22, FIELD_STATUS 183 / 183 / 147 / 175 / 175, LOCKSET 28 /
# 47, LOCK_PATHS 9408 / 9409 / 5872 / 2364 / 2373 and +75 / +75 / +66 / +29 / +29, count.lock_ops 2519 -> 2450 and
# 1586 -> 1547).  The field lists (column fields) and the split of a changed entry by its field signature were
# measured the same way on 2026-11-09, after c4fe9db, from the "fields" lines of the comparisons [M]; the field
# list says which fields a change touches, not that the change is right (AR §14).  References (column ref):
#   id   profile-driven mode unchanged: build identity, parent basis, units_from (AR §0 item 2)
#   run  this run's tool, basis and lock-set rule revision (extract.build_identity, units_from, extended_facts_f5)
#   p0   P0 table revisions 2 and 3 (AR §2a P0_TABLE, BASIS)      dev  basis deviations copied into the facts
#   new  BASIS items that S4 / S5 / fix1 had not                  wb   walk bounds (AR §2a)
#   lp   the LOCK_PATHS section and its entry-release rows        arr  scalar array fields (FIELD_STATUS, AR §2a)
#   inc  incomplete-source rows of resolved indirect calls and their unknown effect (CALL_TARGETS, LOCKSET,
#        LOCK_OPS, AWAIT_L1, BASIS counts; AR §2a, third fix, second round)
FAIL=0
SBAD=0
CUR=-
STATUS=
bad() {
  SBAD=$((SBAD + 1))
  printf 'FAIL %s: %s\n' "$CUR" "$*" >&2
  [ -z "$STATUS" ] || printf 'FAIL %s: %s\n' "$CUR" "$*" >> "$STATUS"
}
note() { printf '%s\n' "$*" >> "$STATUS" || bad "cannot write $STATUS"; }
# rc_ok RC WHAT: RC must be 0.  rc_le RC MAX WHAT: grep (1: no line) and diff (1: they differ) may give 1.
rc_ok() { [ "$1" -eq 0 ] || bad "$2 (rc $1)"; }
rc_le() { [ "$1" -le "$2" ] || bad "$3 (rc $1)"; }
# pipe_ok NAME1 .. NAMEn -- RC1 .. RCn: the statuses of the pipeline that just ran; a name starting with ~ may give 1
pipe_ok() {
  local w=() s i=0 max name
  while [ $# -gt 0 ] && [ "$1" != -- ]; do w+=("$1"); shift; done
  shift
  for s in "$@"; do
    name=${w[$i]:-command $((i + 1))}; max=0; [ "${name:0:1}" = "~" ] && max=1
    [ "$s" -le $max ] || bad "${name#\~} in a pipeline (rc $s)"
    i=$((i + 1))
  done
  [ $i -eq ${#w[@]} ] || bad "pipeline of ${#w[@]} commands gave $i statuses"
}
# txt_ok FILE..: each output exists and is not empty; its size goes to status.txt
txt_ok() {
  local f n
  for f; do
    if [ -s "$f" ]; then
      n=$(wc -c < "$f") || { bad "wc $f"; continue; }
      note "  output $f $n bytes"
    else
      bad "output $f is missing or empty"
    fi
  done
}

# one run: basis + extract + show (+ relations for the profile-free mode) into D.  Returns 1 when a command failed or
# an output is missing or empty.
# Arguments: D cdb build_dir bundle_root recipe -- basis-and-extract args -- extract-only args
one() {
  local D=$1 cdb=$2 bd=$3 br=$4 rc=$5; shift 5
  local both=() ext=() sep=0 a n0=$SBAD
  for a in "$@"; do [ "$a" = "--" ] && { sep=$((sep + 1)); continue; }; [ $sep -le 1 ] && both+=("$a") || ext+=("$a"); done
  mkdir -p "$D" || { bad "mkdir $D"; return 1; }
  local t0 t1 t2 t3
  t0=$(date +%s.%N) || bad "date"
  "$KS" basis "${both[@]}" --compile-db "$cdb" --build-dir "$bd" --bundle-root "$br" --recipe "$rc" --text -o "$D/basis.kz" > "$D/basis.txt" 2>&1; local x=$?
  t1=$(date +%s.%N) || bad "date"
  "$KS" extract "${both[@]}" --basis "$D/basis.kz" "${ext[@]}" -o "$D/facts.kz" > "$D/extract.txt" 2>&1; local y=$?
  t2=$(date +%s.%N) || bad "date"
  "$KS" show "$D/facts.kz" > "$D/facts.txt" 2>&1; local z=$?
  local w=- txt=(basis.txt extract.txt facts.txt)
  if [ "${both[0]:-}" = "--no-profile" ]; then "$KS" relations "$D/facts.kz" -o "$D/relations.txt" > "$D/relations.log" 2>&1; w=$?; txt+=(relations.txt); fi
  t3=$(date +%s.%N) || bad "date"
  printf '%s\n' "basis/extract args: ${both[*]}" "extract-only args: ${ext[*]}" "compile db: $cdb" "build dir: $bd" "bundle root: $br" "recipe: $rc" > "$D/args.txt" || bad "write $D/args.txt"
  ( cd "$D" && sha256sum basis.kz facts.kz "$cdb" "$rc" "$KS" > sha256.txt ); local h=$?
  rc_ok $x "$D: kz-static basis"
  rc_ok $y "$D: kz-static extract"
  rc_ok $z "$D: kz-static show"
  [ "$w" = - ] || rc_ok "$w" "$D: kz-static relations"
  rc_ok $h "$D: sha256sum"
  local f n sizes= s1 s2 s3 verdict=PASS line
  for f in basis.kz facts.kz; do [ -s "$D/$f" ] || bad "$D/$f is missing or empty"; done
  for f in "${txt[@]}" args.txt sha256.txt; do
    n=0
    if [ -s "$D/$f" ]; then n=$(wc -c < "$D/$f") || bad "wc $D/$f"; else bad "output $D/$f is missing or empty"; fi
    sizes+="${sizes:+,}$f:$n"
  done
  s1=$(awk -v a="$t0" -v b="$t1" 'BEGIN { printf "%.1f", b - a }') || bad "awk"
  s2=$(awk -v a="$t1" -v b="$t2" 'BEGIN { printf "%.1f", b - a }') || bad "awk"
  s3=$(awk -v a="$t2" -v b="$t3" 'BEGIN { printf "%.1f", b - a }') || bad "awk"
  [ $SBAD -eq $n0 ] || verdict=FAIL
  printf -v line '%s basis=%d extract=%d show=%d relations=%s basis_s=%s extract_s=%s relations_s=%s sha256=%d sizes=%s verdict=%s' \
    "$D" $x $y $z "$w" "$s1" "$s2" "$s3" $h "$sizes" "$verdict"
  printf '%s\n' "$line" >> "$OUT/run.log" || bad "write $OUT/run.log"
  printf '%s\n' "$line"
  [ $SBAD -eq $n0 ]
}

# dumpn KZ OUT: the kz-dump text of KZ without the tool and artifact hash lines.  Fails when kz-dump fails or the text
# has no section (an empty dump).
dumpn() {
  "$KZD" "$1" | grep -vE '^(tool_sha256|artifact_sha256)=|^container ' > "$2"
  local s=("${PIPESTATUS[@]}") n0=$SBAD g
  rc_ok "${s[0]}" "kz-dump $1"
  rc_le "${s[1]:-2}" 1 "grep on the dump of $1"
  grep -q '^section ' "$2"; g=$?
  [ $g -eq 0 ] || bad "the dump of $1 has no section (grep rc $g)"
  [ $SBAD -eq $n0 ]
}

# compare_records STEP BUILD A B: the records of two kz-dump texts, section by section, and the per-record verdict
# against ALLOW; rc 0 only for PASS.  The first two lines are those of the earlier driver (records by position).
compare_records() {
  python3 -I - "$@" <<'PY'
import collections, os, re, sys
# compare_records STEP BUILD A B: the records of two kz-dump texts (dumpn), section by section, and the per-record
# verdict against the table ALLOW below.  Exit 0 only when every differing record is named by an entry of the table
# for STEP and every entry's count for BUILD is met exactly; exit 1 otherwise (and on an empty or cut dump).
STEP, BUILD, A, B = sys.argv[1:5]
BUILDS = ['c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641', 'c5_642']
# One line per entry: step, section, change (changed | added | removed), the expected number of records for each of
# the five builds (- = the build is not in the step: expected 0), the reference (legend in the driver's comment), the
# fields (changed: the comma list of the fields that differ, exactly; added, removed: -), and the record key: a Python
# regular expression searched in the record (the new record for changed and added, the old one for removed).  A
# changed record is an old and a new record with the same identity (IDENT) and other fields; within one identity the
# old and new records are paired in sorted order.  Its differing fields are the names whose values differ or that
# one side lacks, and <text> when the text outside the name=value pairs differs.  A changed record is named by an
# entry only when its differing fields equal the entry's list, so one key may need one entry per field signature.
# HEADER is the dump head (basis_sha256, parents, sections=) and the section heads without count and crc32.
# STRINGS is not compared: it is the string table, and every record shows its strings as text.
ALLOW = r'''
# step         section      change  c1pre c1post r2  c5_641 c5_642 ref fields key
identity-basis BASIS        changed 1    -    1    1    -    id   value ^name="front_end\.build_identity"
identity-facts BASIS        changed 2    -    2    2    -    id   value ^name="extract\.(build_identity|units_from)"
identity-facts HEADER       changed 1    -    1    1    -    id   basis_sha256 ^(basis_sha256=|parent\[0\] kind=basis )
identity-facts HEADER       changed 1    -    1    1    -    id   sha256 ^(basis_sha256=|parent\[0\] kind=basis )
npsame         HEADER       changed 1    1    1    1    1    lp   sections ^sections=
npsame         HEADER       added   1    1    1    1    1    lp   - ^section LOCK_PATHS type=
npsame         BASIS        changed 2    2    2    2    2    run  value ^name="extract\.(build_identity|extended_facts_f5)"
npsame         BASIS        changed 3    3    3    3    3    p0   value ^name="(extract\.p0_table|p0\.pthread_create|assume\.lock_acquisition_succeeds)"
npsame         BASIS        changed 2    2    2    0    0    inc  value ^name="count\.(lock_ops|lockset)"
npsame         BASIS        added   34   34   34   34   34   p0   - ^name="p0\.
npsame         BASIS        added   2    2    2    7    7    dev  - ^name="extract\.basis_deviation\[
npsame         BASIS        added   5    5    5    5    5    new  - ^name="(extract\.p0_table_scope|count\.lock_paths|param\.walk_bounds|count\.walk_bounds_hit|count\.indirect_calls_incomplete)"
npsame         P0_TABLE     changed 1    1    1    1    1    p0   source name="pthread_create"
npsame         P0_TABLE     added   34   34   34   34   34   p0   - name="
npsame         CALL_TARGETS added   44   44   22   0    0    inc  - reason=incomplete-source
npsame         FIELD_STATUS changed 99   99   74   136  136  arr  flags flags=\{[^}]*array-element-stores-not-collected
npsame         FIELD_STATUS changed 12   12   4    14   14   arr  flags,status_field flags=\{[^}]*array-element-stores-not-collected
npsame         FIELD_STATUS changed 72   72   69   25   25   arr  flags,status_field,status_type flags=\{[^}]*array-element-stores-not-collected
npsame         LOCKSET      changed 20   20   39   0    0    inc  first,flags,held_root,n_held,opener_root flags=\{unknown-release\}
npsame         LOCKSET      changed 8    8    8    0    0    inc  flags flags=\{unknown-release\}
npsame         LOCK_OPS     changed 5    5    0    0    0    inc  chain flags=\{[^}]*unknown-identity
npsame         LOCK_OPS     changed 47   47   34   0    0    inc  chain,depth flags=\{[^}]*unknown-identity
npsame         LOCK_OPS     removed 115  115  71   0    0    inc  - flags=\{[^}]*unknown-identity
npsame         LOCK_OPS     added   46   46   32   0    0    inc  - flags=\{[^}]*unknown-identity
npsame         LOCK_PATHS   added   9174 9175 5630 2166 2170 lp   - kind=entry-release
npsame         LOCK_PATHS   added   234  234  242  198  203  lp   - kind=after-spawn
npsame         AWAIT_L1     changed 1    1    0    0    0    inc  flags flags=\{[^}]*unresolved-call
npsame         AWAIT_L1     changed 1    1    0    0    0    inc  flags,l1_mod_field,l1_mod_type flags=\{[^}]*unresolved-call
s5same         HEADER       changed 1    1    1    1    1    run  basis_sha256 ^(basis_sha256=|parent\[0\] kind=basis |sections=)
s5same         HEADER       changed 1    1    1    1    1    run  sections ^(basis_sha256=|parent\[0\] kind=basis |sections=)
s5same         HEADER       changed 1    1    1    1    1    run  sha256 ^(basis_sha256=|parent\[0\] kind=basis |sections=)
s5same         HEADER       added   1    1    1    1    1    lp   - ^section LOCK_PATHS type=
s5same         BASIS        changed 3    3    3    3    3    run  value ^name="extract\.(build_identity|units_from|extended_facts_f5)"
s5same         BASIS        changed 3    3    3    3    3    p0   value ^name="(extract\.p0_table|p0\.pthread_create|assume\.lock_acquisition_succeeds)"
s5same         BASIS        changed 2    2    2    0    0    inc  value ^name="count\.(lock_ops|lockset)"
s5same         BASIS        added   34   34   34   34   34   p0   - ^name="p0\.
s5same         BASIS        added   2    2    2    7    7    dev  - ^name="extract\.basis_deviation\[
s5same         BASIS        added   5    5    5    5    5    new  - ^name="(extract\.p0_table_scope|count\.lock_paths|param\.walk_bounds|count\.walk_bounds_hit|count\.indirect_calls_incomplete)"
s5same         P0_TABLE     changed 1    1    1    1    1    p0   source name="pthread_create"
s5same         P0_TABLE     added   34   34   34   34   34   p0   - name="
s5same         CALL_TARGETS added   44   44   22   0    0    inc  - reason=incomplete-source
s5same         FIELD_STATUS changed 99   99   74   136  136  arr  flags flags=\{[^}]*array-element-stores-not-collected
s5same         FIELD_STATUS changed 12   12   4    14   14   arr  flags,status_field flags=\{[^}]*array-element-stores-not-collected
s5same         FIELD_STATUS changed 72   72   69   25   25   arr  flags,status_field,status_type flags=\{[^}]*array-element-stores-not-collected
s5same         LOCKSET      changed 20   20   39   0    0    inc  first,flags,held_root,n_held,opener_root flags=\{unknown-release\}
s5same         LOCKSET      changed 8    8    8    0    0    inc  flags flags=\{unknown-release\}
s5same         LOCK_OPS     changed 5    5    0    0    0    inc  chain flags=\{[^}]*unknown-identity
s5same         LOCK_OPS     changed 47   47   34   0    0    inc  chain,depth flags=\{[^}]*unknown-identity
s5same         LOCK_OPS     removed 115  115  71   0    0    inc  - flags=\{[^}]*unknown-identity
s5same         LOCK_OPS     added   46   46   32   0    0    inc  - flags=\{[^}]*unknown-identity
s5same         LOCK_PATHS   added   9174 9175 5630 2166 2170 lp   - kind=entry-release
s5same         LOCK_PATHS   added   234  234  242  198  203  lp   - kind=after-spawn
s5same         AWAIT_L1     changed 1    1    0    0    0    inc  flags flags=\{[^}]*unresolved-call
s5same         AWAIT_L1     changed 1    1    0    0    0    inc  flags,l1_mod_field,l1_mod_type flags=\{[^}]*unresolved-call
fixsame        HEADER       changed 1    1    1    1    1    run  basis_sha256 ^(basis_sha256=|parent\[0\] kind=basis )
fixsame        HEADER       changed 1    1    1    1    1    run  sha256 ^(basis_sha256=|parent\[0\] kind=basis )
fixsame        BASIS        changed 3    3    3    3    3    run  value ^name="extract\.(build_identity|units_from|extended_facts_f5)"
fixsame        BASIS        changed 3    3    3    3    3    p0   value ^name="(extract\.p0_table|extract\.p0_table_scope|p0\.pthread_create)"
fixsame        BASIS        changed 1    1    1    1    1    lp   value ^name="count\.lock_paths"
fixsame        BASIS        changed 2    2    2    0    0    inc  value ^name="count\.(lock_ops|lockset)"
fixsame        BASIS        added   3    3    3    3    3    new  - ^name="(param\.walk_bounds|count\.walk_bounds_hit|count\.indirect_calls_incomplete)"
fixsame        P0_TABLE     changed 1    1    1    1    1    p0   semantics name="(pthread_create|thrd_create)"
fixsame        P0_TABLE     changed 1    1    1    1    1    p0   source name="(pthread_create|thrd_create)"
fixsame        CALL_TARGETS added   44   44   22   0    0    inc  - reason=incomplete-source
fixsame        FIELD_STATUS changed 99   99   74   136  136  arr  flags flags=\{[^}]*array-element-stores-not-collected
fixsame        FIELD_STATUS changed 12   12   4    14   14   arr  flags,status_field flags=\{[^}]*array-element-stores-not-collected
fixsame        FIELD_STATUS changed 72   72   69   25   25   arr  flags,status_field,status_type flags=\{[^}]*array-element-stores-not-collected
fixsame        LOCKSET      changed 20   20   39   0    0    inc  first,flags,held_root,n_held,opener_root flags=\{unknown-release\}
fixsame        LOCKSET      changed 8    8    8    0    0    inc  flags flags=\{unknown-release\}
fixsame        LOCK_OPS     changed 5    5    0    0    0    inc  chain flags=\{[^}]*unknown-identity
fixsame        LOCK_OPS     changed 47   47   34   0    0    inc  chain,depth flags=\{[^}]*unknown-identity
fixsame        LOCK_OPS     removed 115  115  71   0    0    inc  - flags=\{[^}]*unknown-identity
fixsame        LOCK_OPS     added   46   46   32   0    0    inc  - flags=\{[^}]*unknown-identity
fixsame        LOCK_PATHS   removed 0    0    0    6    6    lp   - kind=entry-release
fixsame        LOCK_PATHS   added   78   78   69   35   35   lp   - kind=entry-release
fixsame        AWAIT_L1     changed 1    1    0    0    0    inc  flags flags=\{[^}]*unresolved-call
fixsame        AWAIT_L1     changed 1    1    0    0    0    inc  flags,l1_mod_field,l1_mod_type flags=\{[^}]*unresolved-call
fix2same       HEADER       changed 1    1    1    1    1    run  basis_sha256 ^(basis_sha256=|parent\[0\] kind=basis )
fix2same       HEADER       changed 1    1    1    1    1    run  sha256 ^(basis_sha256=|parent\[0\] kind=basis )
fix2same       BASIS        changed 3    3    3    3    3    run  value ^name="extract\.(build_identity|units_from|extended_facts_f5)"
fix2same       BASIS        changed 2    2    2    2    2    wb   value ^name="(param\.walk_bounds|count\.walk_bounds_hit)"
fix2same       BASIS        changed 1    1    1    1    1    lp   value ^name="count\.lock_paths"
fix2same       BASIS        changed 2    2    2    0    0    inc  value ^name="count\.(lock_ops|lockset)"
fix2same       BASIS        added   1    1    1    1    1    inc  - ^name="count\.indirect_calls_incomplete"
fix2same       CALL_TARGETS added   44   44   22   0    0    inc  - reason=incomplete-source
fix2same       FIELD_STATUS changed 99   99   74   136  136  arr  flags flags=\{[^}]*array-element-stores-not-collected
fix2same       FIELD_STATUS changed 12   12   4    14   14   arr  flags,status_field flags=\{[^}]*array-element-stores-not-collected
fix2same       FIELD_STATUS changed 72   72   69   25   25   arr  flags,status_field,status_type flags=\{[^}]*array-element-stores-not-collected
fix2same       LOCKSET      changed 20   20   39   0    0    inc  first,flags,held_root,n_held,opener_root flags=\{unknown-release\}
fix2same       LOCKSET      changed 8    8    8    0    0    inc  flags flags=\{unknown-release\}
fix2same       LOCK_OPS     changed 5    5    0    0    0    inc  chain flags=\{[^}]*unknown-identity
fix2same       LOCK_OPS     changed 47   47   34   0    0    inc  chain,depth flags=\{[^}]*unknown-identity
fix2same       LOCK_OPS     removed 115  115  71   0    0    inc  - flags=\{[^}]*unknown-identity
fix2same       LOCK_OPS     added   46   46   32   0    0    inc  - flags=\{[^}]*unknown-identity
fix2same       LOCK_PATHS   removed 0    0    0    6    6    lp   - kind=entry-release
fix2same       LOCK_PATHS   added   78   78   69   35   35   lp   - kind=entry-release
fix2same       AWAIT_L1     changed 1    1    0    0    0    inc  flags flags=\{[^}]*unresolved-call
fix2same       AWAIT_L1     changed 1    1    0    0    0    inc  flags,l1_mod_field,l1_mod_type flags=\{[^}]*unresolved-call
fix3same       HEADER       changed 1    1    1    1    1    run  basis_sha256 ^(basis_sha256=|parent\[0\] kind=basis )
fix3same       HEADER       changed 1    1    1    1    1    run  sha256 ^(basis_sha256=|parent\[0\] kind=basis )
fix3same       BASIS        changed 3    3    3    3    3    run  value ^name="extract\.(build_identity|units_from|extended_facts_f5)"
fix3same       BASIS        changed 2    2    2    2    2    wb   value ^name="(param\.walk_bounds|count\.walk_bounds_hit)"
fix3same       BASIS        changed 1    1    1    1    1    lp   value ^name="count\.lock_paths"
fix3same       BASIS        changed 2    2    2    0    0    inc  value ^name="count\.(lock_ops|lockset)"
fix3same       BASIS        added   1    1    1    1    1    inc  - ^name="count\.indirect_calls_incomplete"
fix3same       CALL_TARGETS added   44   44   22   0    0    inc  - reason=incomplete-source
fix3same       LOCKSET      changed 20   20   39   0    0    inc  first,flags,held_root,n_held,opener_root flags=\{unknown-release\}
fix3same       LOCKSET      changed 8    8    8    0    0    inc  flags flags=\{unknown-release\}
fix3same       LOCK_OPS     changed 5    5    0    0    0    inc  chain flags=\{[^}]*unknown-identity
fix3same       LOCK_OPS     changed 47   47   34   0    0    inc  chain,depth flags=\{[^}]*unknown-identity
fix3same       LOCK_OPS     removed 115  115  71   0    0    inc  - flags=\{[^}]*unknown-identity
fix3same       LOCK_OPS     added   46   46   32   0    0    inc  - flags=\{[^}]*unknown-identity
fix3same       LOCK_PATHS   removed 0    0    0    6    6    lp   - kind=entry-release
fix3same       LOCK_PATHS   added   75   75   66   35   35   lp   - kind=entry-release
fix3same       AWAIT_L1     changed 1    1    0    0    0    inc  flags flags=\{[^}]*unresolved-call
fix3same       AWAIT_L1     changed 1    1    0    0    0    inc  flags,l1_mod_field,l1_mod_type flags=\{[^}]*unresolved-call
'''
TSRC = 'built-in'
if os.environ.get('STEP2A_ALLOW_TABLE'):
    TSRC = os.environ['STEP2A_ALLOW_TABLE'] + ' (replaced; for the unit checks only, never for a measurement)'
    ALLOW = open(os.environ['STEP2A_ALLOW_TABLE'], encoding='utf-8').read()
FIELD = re.compile(r'(\w+)=("(?:[^"\\]|\\.)*"|\{[^}]*\}|\S+)')
IDENT = {'BASIS': ('name',), 'P0_TABLE': ('name',), 'FIELD_STATUS': ('type', 'field'), 'AWAIT_L1': ('loop', 'fn'),
         'LOCKSET': ('fn', 'site', 'read', 'line', 'col', 'fh', 'kind', 'type', 'field', 'file'),
         'LOCK_OPS': ('fn', 'site', 'kind', 'mode', 'ident', 'may', 'primitive')}
problems = []


def fields_of(rec):
    d = collections.defaultdict(list)
    for k, v in FIELD.findall(rec):
        d[k].append(v)
    return d, ' '.join(FIELD.sub(' ', rec).split())


def differing(old, new):
    (da, ta), (db, tb) = fields_of(old), fields_of(new)
    out = sorted(k for k in set(da) | set(db) if da.get(k) != db.get(k))
    if ta != tb:
        out.append('<text>')
    return ','.join(out) or '<none>'


def ident(sec, rec):
    if sec == 'HEADER':
        m = re.match(r'(section \S+|parent\[\d+\]|[^=\s]+)', rec)
        return m.group(1) if m else rec
    f = IDENT.get(sec)
    if not f:
        return rec
    d = dict(FIELD.findall(rec))
    return tuple(d[k] for k in f) if all(k in d for k in f) else rec


def read(path, side):
    secs, cur, count = {'HEADER': []}, 'HEADER', {}
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            line = line.rstrip('\n')
            if not line:
                continue
            m = re.match(r'section (\S+) (.*)', line)
            if m:
                cur = m.group(1)
                if cur in secs:
                    problems.append('%s: section %s twice' % (side, cur))
                secs[cur] = []
                c = re.search(r' count=(\d+)', line)
                count[cur] = int(c.group(1)) if c else None
                secs['HEADER'].append('section %s %s' % (cur, re.sub(r' (count|crc32)=\S+', '', m.group(2))))
            elif cur == 'HEADER':
                secs['HEADER'].append(line.strip())
            elif line.startswith('  '):
                secs[cur].append(re.sub(r'^  \S+\[\d+\] ', '', line))
            else:
                problems.append('%s: a line outside the records of %s: %s' % (side, cur, line[:120]))
    for s, n in count.items():
        if s != 'STRINGS' and n != len(secs[s]):
            problems.append('%s: section %s has %d records, its head says count=%s (cut dump?)' % (side, s, len(secs[s]), n))
    if len(count) == 0:
        problems.append('%s: no section (empty dump)' % side)
    return secs


a, b = read(A, 'first'), read(B, 'second')
# the two lines of the earlier comparison (records by position), kept as they were so that runs can be compared
same, bad = [], []
for s, rows in sorted(a.items()):
    if s in ('STRINGS', 'HEADER'):
        continue
    other = b.get(s, [])
    if rows == other:
        same.append(s)
    else:
        diff = [x for x, y in zip(rows, other) if x != y]
        bad.append('%s (%d of %d records differ%s)' % (s, len(diff) + abs(len(rows) - len(other)), len(rows),
                   '; ' + ' | '.join(re.sub(r'\s+', ' ', d)[:160] for d in diff[:3]) if diff else ''))
only_b = sorted(set(b) - set(a))
only_a = sorted(set(a) - set(b))
print('sections with the same records: %d (%s)' % (len(same), ' '.join(same)))
print('sections only in the second: %s; DIFFERENT: %s' % (' '.join(only_b) or '-', '; '.join(bad) or 'none'))
both = [s for s in a if s in b and s not in ('STRINGS', 'HEADER')]
if not both:
    problems.append('no section compared')
# the per-record verdict
entries = []
for n, line in enumerate(ALLOW.split('\n'), 1):
    if not line.strip() or line.lstrip().startswith('#'):
        continue
    p = line.split(None, 10)
    if len(p) < 11 or p[2] not in ('changed', 'added', 'removed'):
        problems.append('table line %d is not step section change n1..n5 ref fields key' % n)
        continue
    if (p[2] == 'changed') != (p[9] != '-') or not re.match(r'^(-|[\w<>]+(,[\w<>]+)*)$', p[9]):
        problems.append('table line %d: fields %s (changed: a comma list of field names; added, removed: -)' % (n, p[9]))
        continue
    if p[0] != STEP:
        continue
    want = p[3 + BUILDS.index(BUILD)] if BUILD in BUILDS else '-'
    try:
        entries.append({'sec': p[1], 'chg': p[2], 'want': 0 if want == '-' else int(want), 'ref': p[8],
                        'fields': p[9] if p[9] == '-' else ','.join(sorted(set(p[9].split(',')))),
                        'key': p[10], 'rx': re.compile(p[10]), 'got': 0})
    except (ValueError, re.error) as e:
        problems.append('table line %d: %s' % (n, e))
unexpected = collections.Counter()
examples = collections.defaultdict(list)
total = collections.Counter()
sigs = collections.Counter()
for s in sorted(set(a) | set(b)):
    if s == 'STRINGS':
        continue
    ra, rb = a.get(s, []), b.get(s, [])
    if ra == rb:
        continue
    ca, cb = collections.Counter(ra), collections.Counter(rb)
    ri, ai = collections.defaultdict(list), collections.defaultdict(list)
    for r in sorted((ca - cb).elements()):
        ri[ident(s, r)].append(r)
    for r in sorted((cb - ca).elements()):
        ai[ident(s, r)].append(r)
    recs = []
    for k in sorted(set(ri) | set(ai), key=repr):
        x, y = ri.get(k, []), ai.get(k, [])
        m = min(len(x), len(y))
        recs += ([('changed', r, o) for o, r in zip(x[:m], y[:m])] + [('removed', r, None) for r in x[m:]] +
                 [('added', r, None) for r in y[m:]])
    for chg, r, o in recs:
        total[chg] += 1
        sig = differing(o, r) if chg == 'changed' else '-'
        keyed = [e for e in entries if e['sec'] == s and e['chg'] == chg and e['rx'].search(r)]
        if chg == 'changed':
            sigs[(s, ' | '.join(sorted({e['key'] for e in keyed})) or '-', sig)] += 1
        hit = [e for e in keyed if e['fields'] == sig]
        if len(hit) == 1:
            hit[0]['got'] += 1
            continue
        why = ('unexpected' if not keyed else 'unexpected fields') if not hit else 'matched by %d entries' % len(hit)
        unexpected[(s, chg, why, sig)] += 1
        if len(examples[(s, chg, why, sig)]) < 3:
            examples[(s, chg, why, sig)].append(r)
print('per-record table: %s; step %s, build %s; %d entries' % (TSRC, STEP, BUILD, len(entries)))
for e in entries:
    ok = e['got'] == e['want']
    if not ok:
        problems.append('%s %s fields=%s %s: expected %d, found %d' % (e['sec'], e['chg'], e['fields'], e['key'],
                                                                     e['want'], e['got']))
    print('  %s %s %s [%s] fields=%s %s: expected %d, found %d' % ('ok' if ok else 'COUNT', e['sec'], e['chg'], e['ref'],
                                                                 e['fields'], e['key'], e['want'], e['got']))
for (s, k, sig), n in sorted(sigs.items()):
    print('  fields %s changed {%s} key %s: %d' % (s, sig, k, n))
for (s, chg, why, sig), n in sorted(unexpected.items()):
    problems.append('%s %s record(s) %s%s: %d' % (s, chg, why, '' if sig == '-' else ' (fields %s)' % sig, n))
    print('  %s %s %s%s: %d, e.g. %s' % (why.upper() if why.startswith('unexpected') else why, s, chg,
                                         '' if sig == '-' else ' {%s}' % sig, n,
                                         ' | '.join(re.sub(r'\s+', ' ', x)[:200] for x in examples[(s, chg, why, sig)])))
for s in only_b:
    print('  section only in the second: %s' % s)
for s in only_a:
    print('  section only in the first: %s' % s)
for p in problems:
    print('  problem: %s' % p)
print('verdict: %s (%d changed, %d added, %d removed records; %d entries; %d problems)' % (
    'PASS' if not problems else 'FAIL', total['changed'], total['added'], total['removed'], len(entries), len(problems)))
sys.exit(1 if problems else 0)
PY
}

# same_as STEP D BUILD A B: the dumps of the facts A and B, their per-record comparison appended to D/STEP.txt (lines
# prefixed with the build), the dumps removed.  Returns 1 on any failure.
same_as() {
  local st=$1 D=$2 b=$3 A=$4 B=$5 n0=$SBAD
  mkdir -p "$D/$b" || { bad "mkdir $D/$b"; return 1; }
  dumpn "$A" "$D/$b/first.dump"
  dumpn "$B" "$D/$b/second.dump"
  compare_records "$st" "$b" "$D/$b/first.dump" "$D/$b/second.dump" | sed "s/^/$b: /" | tee -a "$D/$st.txt"
  pipe_ok "$b: per-record comparison (see $D/$st.txt)" "$b: sed" "$b: tee $D/$st.txt" -- "${PIPESTATUS[@]}"
  rm -f "$D/$b/first.dump" "$D/$b/second.dump" || bad "rm the dumps of $b"
  [ $SBAD -eq $n0 ]
}

identity() {
  local D=$OUT/identity n0=$SBAD now head kl idt et b v art A P X secs nsec nsec2 rec nd nu line
  mkdir -p "$D" || { bad "mkdir $D"; return 1; }
  : > "$D/identity.txt" || bad "truncate $D/identity.txt"
  now=$(date -u +%FT%TZ) || bad "date"
  head=$(git -C "$R" rev-parse HEAD) || bad "git rev-parse HEAD"
  git -C "$R" diff -- kairos > "$D/kairos.diff"; rc_ok $? "git diff -- kairos"
  kl=$(wc -l < "$D/kairos.diff") || bad "wc $D/kairos.diff"
  [ -s "$D/kairos.diff" ] || rm -f "$D/kairos.diff" || bad "rm $D/kairos.diff"
  idt=$("$KS" --identity) || bad "kz-static --identity"
  et=$(grep -m1 '^tool_sha256=' "$EB/inputs/inputs.record") || bad "grep tool_sha256= $EB/inputs/inputs.record"
  printf '%s\n' "# step2a_tests.sh identity $now: the profile-driven mode of this kz-static against the outputs of extract_baseline.sh" \
    "repo $head kairos_uncommitted_lines=$kl" "$idt" "earlier tool: $et" >> "$D/identity.txt" || bad "write $D/identity.txt"
  for b in c1_pre_tx r2_core c5_641; do
    for v in V0 V2; do
      A=$EB/$b/$v/args.txt
      [ -s "$A" ] || { bad "$A is missing or empty"; continue; }
      local prof=() ext=() cdb bd br rc
      X=$(sed -n 's/^profile: //p' "$A") || bad "sed $A"
      read -r -a prof <<< "$X"
      X=$(sed -n 's/^extract args: //p' "$A") || bad "sed $A"
      read -r -a ext <<< "$X"
      cdb=$(sed -n 's/^compile db: //p' "$A") || bad "sed $A"
      bd=$(sed -n 's/^build dir: //p' "$A") || bad "sed $A"
      br=$(sed -n 's/^bundle root: //p' "$A") || bad "sed $A"
      rc=$(sed -n 's/^recipe: //p' "$A") || bad "sed $A"
      one "$D/$b/$v" "$cdb" "$bd" "$br" "$rc" -- "${prof[@]}" -- "${ext[@]}"
      for art in basis facts; do
        P=$D/$b/$v/$art
        dumpn "$EB/$b/$v/$art.kz" "$P.old.dump"
        dumpn "$D/$b/$v/$art.kz" "$P.new.dump"
        diff "$P.old.dump" "$P.new.dump" > "$P.dump.diff"; rc_le $? 1 "diff of the $art dumps of $b $v"
        grep '^section ' "$P.old.dump" > "$P.old.heads"; rc_le $? 1 "grep the section heads of $P.old.dump"
        grep '^section ' "$P.new.dump" > "$P.new.heads"; rc_le $? 1 "grep the section heads of $P.new.dump"
        diff "$P.old.heads" "$P.new.heads" > "$P.heads.diff"; rc_le $? 1 "diff of the section heads of $b $v $art"
        sed -n 's/^[<>] section \([A-Z_0-9]*\) .*/\1/p' "$P.heads.diff" | sort -u > "$P.crc"
        pipe_ok "sed $P.heads.diff" "sort" -- "${PIPESTATUS[@]}"
        secs=$(tr '\n' ' ' < "$P.crc") || bad "tr $P.crc"
        nsec=$(grep -c '^section ' "$P.old.dump"); rc_le $? 1 "grep -c $P.old.dump"
        nsec2=$(grep -c '^section ' "$P.new.dump"); rc_le $? 1 "grep -c $P.new.dump"
        rec=$(grep -c '^[<>]   ' "$P.dump.diff"); rc_le $? 1 "grep -c $P.dump.diff"
        line="$b $v $art: sections old $nsec new $nsec2; sections whose CRC differs: ${secs:-none}; differing record lines: $rec"
        printf '%s\n' "$line" >> "$D/identity.txt" || bad "write $D/identity.txt"
        printf '%s\n' "$line"
        grep '^[<>]   ' "$P.dump.diff" | cut -c1-400 | sed 's/^/    /' >> "$D/identity.txt"
        pipe_ok "~grep $P.dump.diff" "cut" "sed" -- "${PIPESTATUS[@]}"
        compare_records "identity-$art" "$b" "$P.old.dump" "$P.new.dump" | sed "s/^/$b $v $art: /" | tee -a "$D/identity.txt"
        pipe_ok "$b $v $art: per-record comparison (see $D/identity.txt)" "sed" "tee $D/identity.txt" -- "${PIPESTATUS[@]}"
        rm -f "$P.old.dump" "$P.new.dump" "$P.old.heads" "$P.new.heads" "$P.heads.diff" "$P.crc" || bad "rm the dumps of $b $v $art"
      done
      # the show text: the build_identity line removed, the one line that may differ is extract.units_from (its basis)
      P=$D/$b/$v
      grep -v 'build_identity' "$EB/$b/$v/facts.txt" > "$P/show.old"; rc_le $? 1 "grep -v $EB/$b/$v/facts.txt"
      grep -v 'build_identity' "$P/facts.txt" > "$P/show.new"; rc_le $? 1 "grep -v $P/facts.txt"
      [ -s "$P/show.old" ] && [ -s "$P/show.new" ] || bad "$b $v: a show text is empty"
      diff "$P/show.old" "$P/show.new" > "$P/show.diff"; rc_le $? 1 "diff of the show texts of $b $v"
      nd=$(grep -c '^[<>]' "$P/show.diff"); rc_le $? 1 "grep -c $P/show.diff"
      nu=$(grep -cE '^[<>]   extract\.units_from = basis [0-9a-f]+  \(unset\)$' "$P/show.diff"); rc_le $? 1 "grep -c $P/show.diff"
      line="$b $v show text (build_identity line removed): diff lines $nd"
      printf '%s\n' "$line" >> "$D/identity.txt" || bad "write $D/identity.txt"
      printf '%s\n' "$line"
      if [ "$nd" -eq 2 ] && [ "$nu" -eq 2 ]; then line="$b $v show text verdict: PASS (the differing lines are the extract.units_from line, 1 old and 1 new)"
      else line="$b $v show text verdict: FAIL ($nd differing lines, $nu of them the extract.units_from line; expected 2 and 2)"; bad "$line"; fi
      printf '%s\n' "$line" >> "$D/identity.txt" || bad "write $D/identity.txt"
      printf '%s\n' "$line"
      rm -f "$P/show.old" "$P/show.new" || bad "rm the show texts of $b $v"
    done
  done
  txt_ok "$D/identity.txt"
  [ $SBAD -eq $n0 ]
}

npsame() {
  local D=$OUT/npsame b n0=$SBAD
  mkdir -p "$D" || { bad "mkdir $D"; return 1; }
  : > "$D/npsame.txt" || bad "truncate $D/npsame.txt"
  for b in "${BUILDS[@]}"; do
    mkdir -p "$D/$b" || { bad "mkdir $D/$b"; continue; }
    "$KS" extract --no-profile --basis "$S4/$b/NP/basis.kz" -o "$D/$b/facts.kz" > "$D/$b/extract.txt" 2>&1; rc_ok $? "$b: kz-static extract --no-profile"
    txt_ok "$D/$b/extract.txt"
    same_as npsame "$D" "$b" "$S4/$b/NP/facts.kz" "$D/$b/facts.kz"
    rm -f "$D/$b/facts.kz" || bad "rm $D/$b/facts.kz"
  done
  txt_ok "$D/npsame.txt"
  [ $SBAD -eq $n0 ]
}

# versus STEP REF: the facts of this run (OUT/<build>/NP) against REF/<build>/NP, per record -> OUT/STEP/STEP.txt
versus() {
  local st=$1 REF=$2 D=$OUT/$1 b n0=$SBAD
  mkdir -p "$D" || { bad "mkdir $D"; return 1; }
  : > "$D/$st.txt" || bad "truncate $D/$st.txt"
  for b in "${BUILDS[@]}"; do same_as "$st" "$D" "$b" "$REF/$b/NP/facts.kz" "$OUT/$b/NP/facts.kz"; done
  txt_ok "$D/$st.txt"
  [ $SBAD -eq $n0 ]
}
s5same() { versus s5same "$S5"; }
fixsame() { versus fixsame "$FIX1"; }
fix2same() { versus fix2same "$FIX2"; }
fix3same() { versus fix3same "$FIX3"; }

run() {
  local b X n0=$SBAD
  for b in "${BUILDS[@]}"; do
    local in=()
    X=$(inputs_of "$b") || { bad "inputs_of $b"; continue; }
    read -r -a in <<< "$X"
    [ ${#in[@]} -eq 4 ] || { bad "inputs_of $b gave ${#in[@]} paths"; continue; }
    one "$OUT/$b/NP" "${in[@]}" -- --no-profile -- "${RULE[@]}"
  done
  txt_ok "$OUT/run.log"
  [ $SBAD -eq $n0 ]
}

# T7: the second extraction and relations are kept (NP2/facts.kz too); twice.txt keeps its earlier lines, the rc and
# the full hashes of both runs go to twice_detail.txt
twice() {
  local b D x y cf cr f r hf hr hnf hnr line n0=$SBAD
  : > "$OUT/twice.txt" || bad "truncate $OUT/twice.txt"
  : > "$OUT/twice_detail.txt" || bad "truncate $OUT/twice_detail.txt"
  for b in "${BUILDS[@]}"; do
    D=$OUT/$b/NP2
    mkdir -p "$D" || { bad "mkdir $D"; continue; }
    "$KS" extract --no-profile --basis "$OUT/$b/NP/basis.kz" "${RULE[@]}" -o "$D/facts.kz" > "$D/extract.txt" 2>&1; x=$?
    "$KS" relations "$D/facts.kz" -o "$D/relations.txt" > "$D/relations.log" 2>&1; y=$?
    rc_ok $x "$b: kz-static extract (second)"
    rc_ok $y "$b: kz-static relations (second)"
    txt_ok "$D/extract.txt" "$D/relations.txt"
    cmp -s "$OUT/$b/NP/facts.kz" "$D/facts.kz"; cf=$?
    cmp -s "$OUT/$b/NP/relations.txt" "$D/relations.txt"; cr=$?
    case $cf in 0) f=same ;; 1) f=DIFFERENT; bad "$b: the facts of the two extractions differ" ;; *) f=DIFFERENT; bad "$b: cmp of the facts (rc $cf)" ;; esac
    case $cr in 0) r=same ;; 1) r=DIFFERENT; bad "$b: the relations of the two runs differ" ;; *) r=DIFFERENT; bad "$b: cmp of the relations (rc $cr)" ;; esac
    hf=$(sha256sum "$D/facts.kz") || bad "sha256sum $D/facts.kz"
    hr=$(sha256sum "$D/relations.txt") || bad "sha256sum $D/relations.txt"
    hnf=$(sha256sum "$OUT/$b/NP/facts.kz") || bad "sha256sum $OUT/$b/NP/facts.kz"
    hnr=$(sha256sum "$OUT/$b/NP/relations.txt") || bad "sha256sum $OUT/$b/NP/relations.txt"
    line="$b facts twice: $f (${hf:0:16}); relations twice: $r (${hr:0:16})"
    printf '%s\n' "$line" >> "$OUT/twice.txt" || bad "write $OUT/twice.txt"
    printf '%s\n' "$line"
    printf '%s extract=%d relations=%d facts NP %s NP2 %s relations NP %s NP2 %s\n' "$b" $x $y "${hnf%% *}" "${hf%% *}" \
      "${hnr%% *}" "${hr%% *}" >> "$OUT/twice_detail.txt" || bad "write $OUT/twice_detail.txt"
  done
  txt_ok "$OUT/twice.txt" "$OUT/twice_detail.txt"
  [ $SBAD -eq $n0 ]
}

count() {
  local f n0=$SBAD
  mkdir -p "$OUT/count" || { bad "mkdir $OUT/count"; return 1; }
  python3 -I "$HERE/step2a_tests_count.py" "$OUT" "$EB" "$S5" "$FIX1" "$FIX2" "$FIX3" | tee "$OUT/count/summary.txt"
  pipe_ok "step2a_tests_count.py" "tee $OUT/count/summary.txt" -- "${PIPESTATUS[@]}"
  txt_ok "$OUT/count/summary.txt"
  for f in targets_rule summary burden against_s5 against_fix1 against_fix2 against_fix3; do
    [ -s "$OUT/count/$f.tsv" ] || bad "output $OUT/count/$f.tsv is missing or empty"
  done
  [ $SBAD -eq $n0 ]
}

# sha256 of every output except the sha256.txt lists and status.txt (written after this step)
sha() {
  local n n0=$SBAD
  ( cd "$OUT" || exit 1
    find . -type f ! -name sha256.txt ! -path ./status.txt -print0 | sort -z | xargs -0 sha256sum > sha256.txt
    s=("${PIPESTATUS[@]}")
    for x in "${s[@]}"; do [ "$x" -eq 0 ] || exit 1; done )
  rc_ok $? "sha256 of the outputs (cd, find, sort, xargs sha256sum)"
  txt_ok "$OUT/sha256.txt"
  n=$(wc -l < "$OUT/sha256.txt") || bad "wc $OUT/sha256.txt"
  echo "sha256 of $n files -> $OUT/sha256.txt"
  [ $SBAD -eq $n0 ]
}

# ---------------------------------------------------------------- main
case "$STEP" in identity|npsame|run|s5same|fixsame|fix2same|fix3same|twice|count|sha|all) ;; *) echo "unknown step $STEP" >&2; exit 2 ;; esac
KSH=$(sha256sum "$KS") || { echo "no kz-static at $KS" >&2; exit 2; }
KDH=$(sha256sum "$KZD") || { echo "no kz-dump at $KZD" >&2; exit 2; }
RUNS=${STEP2A_RUNS:-/home/user/work/redesign/step2a/tests_runs}
if [ -z "$OUT" ]; then
  if [ "$STEP" = all ]; then
    STAMP=$(date -u +%Y%m%dT%H%M%SZ) || exit 2
    OUT=$RUNS/run_${STAMP}_${KSH:0:8}
    mkdir -p "$OUT" || { echo "cannot make $OUT" >&2; exit 2; }
    ln -sfn "${OUT##*/}" "$RUNS/latest" || { echo "cannot link $RUNS/latest" >&2; exit 2; }
  else
    OUT=$RUNS/latest
    [ -d "$OUT" ] || { echo "no $OUT: give OUT, or run all first" >&2; exit 2; }
  fi
fi
if [ "$STEP" = all ] && [ -d "$OUT" ]; then
  shopt -s nullglob dotglob; E=("$OUT"/*); shopt -u nullglob dotglob
  [ ${#E[@]} -eq 0 ] || { echo "all writes into a new or empty output directory; $OUT is not empty" >&2; exit 2; }
fi
# every step: T/ (the cited 2a output) and any finished output (a top-level sha256.txt) are not written into, unless
# STEP2A_OVERWRITE=1 (RV4 analysis 7.2 step 4, fix stage)
FROZEN=/home/user/work/redesign/step2a/tests
if [ -d "$OUT" ] && [ "${STEP2A_OVERWRITE:-}" != 1 ]; then
  RO=$(cd "$OUT" && pwd -P) || { echo "cannot resolve $OUT" >&2; exit 2; }
  RF=$(cd "$FROZEN" 2> /dev/null && pwd -P) || RF=
  if [ "$RO" = "$RF" ] || [ -e "$OUT/sha256.txt" ]; then
    echo "refusing: $OUT is T/ or holds a top-level sha256.txt (a finished output); STEP2A_OVERWRITE=1 overrides" >&2; exit 2
  fi
fi
mkdir -p "$OUT" || { echo "cannot make $OUT" >&2; exit 2; }
STATUS=$OUT/status.txt
NOW=$(date -u +%FT%TZ) || NOW=?
HEADREV=$(git -C "$R" rev-parse HEAD) || HEADREV=?
{
  echo "# step2a_tests.sh $STEP $NOW"
  echo "out $OUT"
  echo "kz-static $KS sha256 ${KSH%% *}"
  echo "kz-dump $KZD sha256 ${KDH%% *}"
  echo "repo $HEADREV"
  echo "inputs EB=$EB S4=$S4 S5=$S5 FIX1=$FIX1 FIX2=$FIX2 FIX3=$FIX3 C1=$C1 REF=$REF CT=$CT"
  if [ -n "${STEP2A_ALLOW_TABLE:-}" ]; then echo "allowance table: replaced by $STEP2A_ALLOW_TABLE (unit checks only, not a measurement)"
  else echo "allowance table: built-in"; fi
} >> "$STATUS" || { echo "cannot write $STATUS" >&2; exit 2; }
step() {
  local rc t
  CUR=$1; SBAD=0
  t=$(date -u +%FT%TZ) || t=?
  note "step $1 start $t"
  "$1"; rc=$?
  [ $rc -eq 0 ] || [ $SBAD -gt 0 ] || bad "returned $rc"
  if [ $SBAD -eq 0 ]; then note "step $1 PASS"; else note "step $1 FAIL ($SBAD failures)"; FAIL=$((FAIL + 1)); fi
  CUR=-
}
if [ "$STEP" = all ]; then
  for S in identity npsame run s5same fixsame fix2same fix3same twice count sha; do step "$S"; done
else
  step "$STEP"
fi
if [ $FAIL -eq 0 ]; then note "result $STEP: PASS"; else note "result $STEP: FAIL ($FAIL steps)"; fi
echo "$STEP: $([ $FAIL -eq 0 ] && echo PASS || echo "FAIL ($FAIL steps)") -> $STATUS"
exit $((FAIL > 0))

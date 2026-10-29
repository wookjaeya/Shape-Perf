#!/usr/bin/env bash
# regress_eid1.sh <old kz build> <new kz build> <out> -- the outputs of the analyzer for inputs that exist without the
# E-ID1 change (plan/c5_execution_basis_20261028.md section 10.4), from the build before it and the build with it.  Tool
# check only: no research condition, no case.
#   1  kz replay of the recorded F0 run env/logs/s1r2_p1.kzt on profiles/cfs-v7.0.1 (stdout), and its --out container
#      (and the replay of the converted container of 2, which reads the neutral layer from its sections)
#   2  kz convert of that run (the neutral layer added)
#   3  kz convert --kzrt of the kzrt dumps the new build's runtime tests leave (rt_end_normal, rt_end_kill.rec,
#      rt_serial), with the unit-test manifests their tests use
# Containers are compared as kz-dump text.  A tool's own identity differs between the two builds by construction (its
# source identity and the schema fingerprint go into the tool hash, kairos/cmake/KzSourceIdentity.cmake), so the lines
# that carry it, and the artifact hashes that cover it, are listed apart; every other line must be equal.
set -uo pipefail
OLD=${1:?usage: regress_eid1.sh <old kz build> <new kz build> <out>}; NEW=${2:?new}; O=${3:?out}
R="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
K="$R/kairos"
mkdir -p "$O"
REC="$O/regress_eid1.record"
: > "$REC"
log() { echo "$*" | tee -a "$REC"; }
log "# regress_eid1.sh $(date -u +%FT%TZ): old=$OLD new=$NEW"
for b in "$OLD" "$NEW"; do log "kz $(sha256sum "$b/tools/kz/kz" | cut -c1-64) kz-dump $(sha256sum "$b/tools/kz-dump/kz-dump" | cut -c1-64) ($b)"; done
# a dump line that names a tool hash or an artifact hash
ident='tool_sha256|tool_hash|artifact_sha256|fingerprint|parent|sha256=|kz-core|source='
cmp_text() { # <name> <old file> <new file>; the two runs' own output paths (..._old.* / ....old.*) are named alike
  local n=$1 a="$O/.a" b="$O/.b"
  sed -E 's/([_.])(old|new)([._])/\1W\3/g' "$2" > "$a"
  sed -E 's/([_.])(old|new)([._])/\1W\3/g' "$3" > "$b"
  if cmp -s "$a" "$b"; then log "$n: identical ($(wc -l < "$a") lines)"; return; fi
  local d; d=$(diff "$a" "$b" | grep -E '^[<>]' | grep -vcE "$ident")
  log "$n: $(wc -l < "$a") lines, differs in $(diff "$a" "$b" | grep -cE '^[<>]') lines, of which $d not identity or hash lines"
  diff "$a" "$b" > "$O/$n.diff"
}
P="$K/profiles/cfs-v7.0.1"
RUN="$R/env/logs/s1r2_p1.kzt"
for w in old new; do
  b=$OLD; [ "$w" = new ] && b=$NEW
  rm -f "$O/replay_$w.kz" "$O/convert_$w.kzt"
  # the table size R2's replay gives (instrument/gcc/scripts/replay_gi.sh r2, from its reference)
  "$b/tools/kz/kz" replay "$RUN" --profile "$P" --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --out "$O/replay_$w.kz" > "$O/replay_$w.txt" 2>&1
  echo "replay rc=$?" >> "$O/replay_$w.txt"
  "$b/tools/kz-dump/kz-dump" "$O/replay_$w.kz" > "$O/replay_$w.dump" 2>&1
  "$b/tools/kz/kz" convert "$RUN" --out "$O/convert_$w.kzt" > "$O/convert_$w.txt" 2>&1; echo "convert rc=$?" >> "$O/convert_$w.txt"
  "$b/tools/kz-dump/kz-dump" "$O/convert_$w.kzt" > "$O/convert_$w.dump" 2>&1
  "$b/tools/kz/kz" replay "$O/convert_$w.kzt" --profile "$P" --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 > "$O/replay_conv_$w.txt" 2>&1
  echo "replay rc=$?" >> "$O/replay_conv_$w.txt"
done
cmp_text replay_stdout "$O/replay_old.txt" "$O/replay_new.txt"
cmp_text replay_converted_stdout "$O/replay_conv_old.txt" "$O/replay_conv_new.txt"
cmp_text replay_container "$O/replay_old.dump" "$O/replay_new.dump"
cmp_text convert_stdout "$O/convert_old.txt" "$O/convert_new.txt"
cmp_text convert_container "$O/convert_old.dump" "$O/convert_new.dump"
# kzrt dumps with their unit-test manifests
sed -n '/^const char\* const kManifest =/,/;$/p' "$K/tests/cfs-free/test_kzrt_recovered.cpp" |
  sed -n 's/^ *"\(.*\)\\n";*$/\1/p' | sed 's/\\"/"/g' > "$O/unit_rt_end.kzs"
T="$NEW/runtime/tests"
for x in rt_end_normal.kzrt:unit_rt_end rt_end_kill.rec.kzrt:unit_rt_end rt_serial.kzrt:sites_example; do
  f=${x%%:*}; m=${x#*:}
  man="$O/$m.kzs"; [ "$m" = sites_example ] && man="$K/runtime/tests/data/sites_example.kzs"
  for w in old new; do
    b=$OLD; [ "$w" = new ] && b=$NEW
    rm -f "$O/$f.$w.kzt"
    "$b/tools/kz/kz" convert --kzrt "$T/$f" --manifest "$man" --out "$O/$f.$w.kzt" > "$O/$f.$w.txt" 2>&1; echo "convert rc=$?" >> "$O/$f.$w.txt"
    "$b/tools/kz-dump/kz-dump" "$O/$f.$w.kzt" > "$O/$f.$w.dump" 2>&1
  done
  cmp_text "kzrt_${f}_stdout" "$O/$f.old.txt" "$O/$f.new.txt"
  cmp_text "kzrt_${f}_container" "$O/$f.old.dump" "$O/$f.new.dump"
  log "  NOWNER in the new container: $(grep -c 'NOWNER' "$O/$f.new.dump")"
done

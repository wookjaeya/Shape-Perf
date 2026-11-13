#!/usr/bin/env bash
# regress_eid1.sh <old kz build> <new kz build> <out> -- the outputs of the analyzer for inputs that exist without the
# E-ID1 change (plan/c5_execution_basis_20261028.md section 10.4), from the build before it and the build with it.  Tool
# check only: no research condition, no case.
#   1  kz replay of the recorded F0 run env/logs/s1r2_p1.kzt on profiles/cfs-v7.0.1 (stdout), and its --out container
#      (and the replay of the converted container of 2, which reads the neutral layer from its sections)
#   2  kz convert of that run (the neutral layer added)
#   3  kz convert --kzrt of the kzrt dumps the new build's runtime tests leave (rt_end_normal, rt_end_kill.rec,
#      rt_serial), with the unit-test manifests their tests use
#   4  the W6 G-I dumps of real C1/R2 runs with identity reads (R2 cpu1; C1 natural tx and rx of the four builds): each
#      converted with the manifests and the B' basis its W6 replay used (read from that replay's convert log and
#      sha256.txt, each checked by its hash; no --process, as there) and replayed (reading cur) with the arguments of the
#      W6 replays (main checkout env/r2/replay_w6.sh girec L74-L78, COMMON L66, cur L61; env/c1/replay_w6.sh PROF
#      L110-L111, gif1 L124-L132, cur L140); the old tool's container must be the W6 one (its sha256 in the convert log)
# Containers are compared as kz-dump text and as bytes.  A tool's own identity differs between the two builds by
# construction (its source identity and the schema fingerprint go into the tool hash, kairos/cmake/KzSourceIdentity.cmake),
# and so do the artifact hashes that cover it and the hashes of the parents made with it.  Those lines alone are masked
# (MASK: each pattern anchored to the one line format that carries such a hash); every other line must be equal, and
# the masked comparison says so by a count of 0.
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
# the hash lines of a tool's identity: the container header's tool/artifact/basis hash, a parent's hash, the
# "wrote ... artifact_sha256" / "artifact_sha256" lines of kz convert and kz replay, and the crc of a REF_LISTS section
# (its records, printed in full, name the parent by its hash)
MASK=(-e 's/^(tool_sha256|artifact_sha256|basis_sha256)=[0-9a-f]{64}$/\1=H/'
      -e 's/^(  parent\[[0-9]+\] kind=[a-z]+ role=[0-9]+ sha256=)[0-9a-f]{64}$/\1H/'
      -e 's/^(wrote .* artifact_sha256 )[0-9a-f]{64}$/\1H/'
      -e 's/^artifact_sha256 [0-9a-f]{64}$/artifact_sha256 H/'
      -e 's/^(section REF_LISTS type=[0-9]+ version=[0-9]+ record=kz_ref record_size=[0-9]+ count=[0-9]+ crc32=0x)[0-9a-f]{1,8}$/\1H/')
cmp_text() { # <name> <old file> <new file> [<old input hash> <new input hash>]; the two runs' own output paths (..._old.* /
  # ....old.*) are named alike, and so is the input each tool made itself (a converted run the replay reads)
  local n=$1 a="$O/.a" b="$O/.b" ha=${4:-} hb=${5:-}
  sed -E 's/([_.])(old|new)([._])/\1W\3/g' "$2" > "$a"
  sed -E 's/([_.])(old|new)([._])/\1W\3/g' "$3" > "$b"
  if cmp -s "$a" "$b"; then log "$n: identical ($(wc -l < "$a") lines)"; return; fi
  diff "$a" "$b" > "$O/$n.diff"
  sed -E "${MASK[@]}" "$a" > "$a.m"; sed -E "${MASK[@]}" "$b" > "$b.m"
  if [ -n "$ha" ]; then sed -i "s/$ha/INPUT/g" "$a.m"; sed -i "s/$hb/INPUT/g" "$b.m"; fi
  local left; left=$(diff "$a.m" "$b.m" | grep -cE '^[<>]')
  log "$n: $(wc -l < "$a") lines, $(grep -cE '^[<>]' "$O/$n.diff") differ; with the identity hash lines masked${ha:+ and the input hash named alike}: $left differ"
  [ "$left" = 0 ] || diff "$a.m" "$b.m" > "$O/$n.masked.diff"
  rm -f "$a.m" "$b.m"
}
cmp_bytes() { # <name> <old container> <new container>: sizes, and the differing bytes as ranges of 1-based offsets
  local r; r=$(cmp -l "$2" "$3" 2>/dev/null | awk 'NR==1{s=$1;e=$1;n=1;next} {n++; if ($1<=e+8) e=$1; else {printf "%s%d-%d", sep, s, e; sep=","; s=$1; e=$1}} END{if (n) printf "%s%d-%d", sep, s, e; printf " (%d bytes)", n}')
  log "$1: sizes $(stat -c%s "$2") / $(stat -c%s "$3"); differing bytes ${r:-none}"
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
cmp_bytes replay_container_bytes "$O/replay_old.kz" "$O/replay_new.kz"
cmp_text convert_stdout "$O/convert_old.txt" "$O/convert_new.txt"
cmp_text convert_container "$O/convert_old.dump" "$O/convert_new.dump"
cmp_bytes convert_container_bytes "$O/convert_old.kzt" "$O/convert_new.kzt"
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
  cmp_bytes "kzrt_${f}_container_bytes" "$O/$f.old.kzt" "$O/$f.new.kzt"
  log "  NOWNER in the new container: $(grep -c 'NOWNER' "$O/$f.new.dump")"
done
# W6 G-I dumps (4)
W6R=/home/user/work/w6/replay
for D in "$W6R/r2/gi" "$W6R"/c1/replay/gi/*_*_*; do
  [ -d "$D" ] || continue
  CL="$D/convert.log"; [ -e "$CL" ] || CL="$D/convert.txt"
  case "$D" in */r2/gi) n=w6_r2_gi ;; *) n=w6_c1_gi_$(basename "$D") ;; esac
  dump=$(sed -n '1s/^kz convert --kzrt \(.*\) (instrumented run)$/\1/p' "$CL")
  want=$(sed -n 's/^wrote .* artifact_sha256 \([0-9a-f]*\)$/\1/p' "$CL")
  MAN=(); ok=1
  while read -r m h; do
    [ "$(sha256sum "$m" | cut -c1-64)" = "$h" ] || { log "$n: manifest $m is not the one W6 used ($h)"; ok=0; }
    MAN+=(--manifest "$m")
  done < <(sed -n 's/^manifest \(.*\) sha256 \([0-9a-f]*\)$/\1 \2/p' "$CL")
  BAS=$(awk -v d="$D/" '$2 ~ /\/basis\.kz$/ && index($2, d) != 1 {print $2}' "$D/sha256.txt" | head -1)
  FAC=$(awk -v d="$D/" '$2 ~ /\/facts\.kz$/ && index($2, d) != 1 {print $2}' "$D/sha256.txt" | head -1)
  for f in "$dump" "$BAS" "$FAC"; do
    h=$(awk -v f="$f" '$2 == f {print $1}' "$D/sha256.txt")
    [ -z "$h" ] || [ "$(sha256sum "$f" | cut -c1-64)" = "$h" ] || { log "$n: $f is not the one W6 used"; ok=0; }
  done
  [ $ok = 1 ] && [ -n "$dump" ] && [ -n "$BAS" ] && [ -n "$FAC" ] || { log "$n: inputs incomplete, not compared"; continue; }
  PR=$(dirname "$(dirname "$(sed -n '1s/^manifest \(.*\) sha256 .*$/\1/p' <(grep '^manifest ' "$CL"))")")
  case "$n" in
  w6_r2_gi) A=(--profile "$PR" --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --core-request-caller CFE_ES_CreateObjects) ;;
  *) x=$(basename "$D"); CFG=${x%%_*}; CAND=$(echo "$x" | cut -d_ -f2)
     case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; esac
     A=(--profile "$PR" --profile "$PR/cf-$REV" --profile "$PR/bundle-$CAND" --profile "$PR/rspec-$REV"
        --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20) ;;
  esac
  for w in old new; do
    b=$OLD; [ "$w" = new ] && b=$NEW
    rm -f "$O/$n.$w.kzt" "$O/$n.$w.cur.kz"
    "$b/tools/kz/kz" convert --kzrt "$dump" "${MAN[@]}" --basis "$BAS" --out "$O/$n.$w.kzt" > "$O/$n.$w.convert.txt" 2>&1; echo "convert rc=$?" >> "$O/$n.$w.convert.txt"
    "$b/tools/kz-dump/kz-dump" "$O/$n.$w.kzt" > "$O/$n.$w.dump" 2>&1
    "$b/tools/kz/kz" replay "$O/$n.$w.kzt" "${A[@]}" --basis "$BAS" --facts "$FAC" --static-basis "$BAS" --compare-narrowing --out "$O/$n.$w.cur.kz" > "$O/$n.$w.cur.txt" 2>&1
    echo "replay rc=$?" >> "$O/$n.$w.cur.txt"
    "$b/tools/kz-dump/kz-dump" "$O/$n.$w.cur.kz" > "$O/$n.$w.cur.dump" 2>&1
  done
  log "$n: $dump; the old tool's container is the W6 one: $([ "$(sha256sum "$O/$n.old.kzt" | cut -c1-64)" = "$want" ] && echo yes || echo "NO ($want)"); $(grep -m1 '^note: identity join' "$O/$n.new.convert.txt" | cut -c1-160)"
  cmp_text "${n}_convert_stdout" "$O/$n.old.convert.txt" "$O/$n.new.convert.txt"
  cmp_text "${n}_convert_container" "$O/$n.old.dump" "$O/$n.new.dump"
  cmp_bytes "${n}_convert_container_bytes" "$O/$n.old.kzt" "$O/$n.new.kzt"
  ho=$(sha256sum "$O/$n.old.kzt" | cut -c1-64); hn=$(sha256sum "$O/$n.new.kzt" | cut -c1-64)
  cmp_text "${n}_replay_stdout" "$O/$n.old.cur.txt" "$O/$n.new.cur.txt" "$ho" "$hn"
  cmp_text "${n}_replay_container" "$O/$n.old.cur.dump" "$O/$n.new.cur.dump" "$ho" "$hn"
  cmp_bytes "${n}_replay_container_bytes" "$O/$n.old.cur.kz" "$O/$n.new.cur.kz"
done
rm -f "$O/.a" "$O/.b"

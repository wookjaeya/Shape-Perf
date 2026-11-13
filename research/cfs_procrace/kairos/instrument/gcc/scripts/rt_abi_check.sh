#!/usr/bin/env bash
# rt_abi_check.sh <rt32> <kz> <out> -- the store and dump checks of a 32-bit target runtime (PD29 D5, E-RT; tool tests, no
# research condition, no case).  <rt32>: a runtime build made by build_runtime.sh -m32 whose ctest has run (its rt_end
# tests leave rt_end_<case>.rec.kzrt and rt_end_normal.kzrt in <rt32>/tests); <kz>: a 64-bit Kairos build tree (its own
# runtime in <kz>/runtime, the converter kz, test_kzrt_recovered).  Writes <out>/rt_abi_check.record.
#   1  a store of each build (test_rt_end's own target, case kill: the process ends by SIGKILL and leaves its store file;
#      the environment is the one test_rt_end's harness gives its target, unit-test values) is recovered by the
#      kz-rt-dump of its own build and refused by the other build's; the header fields that differ are printed
#   2  the 64-bit host side reads the 32-bit dumps: kz-rt-dump (64-bit) checks them, test_kzrt_recovered (64-bit) runs on
#      the 32-bit rt_end dumps, kz convert --kzrt (64-bit) converts the recovered kill dump and the normal dump with the
#      unit-test manifest of test_kzrt_recovered.cpp
#   3  the 32-bit and 64-bit dumps of the normal case have the same records apart from the values the run itself
#      gives (thread ids, times, return addresses): compared on kz-rt-dump --events-only with those fields masked
set -uo pipefail
R32=${1:?usage: rt_abi_check.sh <rt32> <kz> <out>}; KZB=${2:?kz build}; O=${3:?out dir}
R64="$KZB/runtime"
mkdir -p "$O"
REC="$O/rt_abi_check.record"
: > "$REC"
log() { echo "$*" | tee -a "$REC"; }
log "# rt_abi_check.sh $(date -u +%FT%TZ): rt32=$R32 kz=$KZB"
for w in 32:"$R32" 64:"$R64"; do
  b=${w%%:*}; d=${w#*:}
  log "build $b: test_rt_end $(file -b "$d/tests/test_rt_end" | cut -d, -f1-2); kz-rt-dump $(file -b "$d/kz-rt-dump" | cut -d, -f1-2) sha256 $(sha256sum "$d/kz-rt-dump" | cut -c1-64)"
done

# 1. stores and the recovery tools
for b in 32 64; do
  d=$R32; [ "$b" = 64 ] && d=$R64
  rm -f "$O/store$b.store"
  ( env KZ_RT_STORE="$O/store$b.store" KZ_RT_MODE=record KZ_RT_RUN_ID=7 "$d/tests/test_rt_end" child kill 1 > /dev/null 2>&1 ) 2> /dev/null
  log "store$b: the target ended with status $? ($(stat -c %s "$O/store$b.store") bytes)"
done
for s in 32 64; do
  for t in 32 64; do
    d=$R32; [ "$t" = 64 ] && d=$R64
    rm -f "$O/rec_s${s}_t$t.kzrt"
    "$d/kz-rt-dump" --recover "$O/store$s.store" --out "$O/rec_s${s}_t$t.kzrt" --signal 9 > "$O/recover_s${s}_t$t.txt" 2>&1
    rc=$?
    log "recover store$s with the $t-bit kz-rt-dump: rc=$rc $( [ -e "$O/rec_s${s}_t$t.kzrt" ] && echo written || echo 'nothing written')"
    sed 's/^/  /' "$O/recover_s${s}_t$t.txt" | grep -E 'recover(:| refused| store)' | tee -a "$REC" > /dev/null
  done
done
rm -f "$O/store32.store" "$O/store64.store"  # as large as the store; the recovered dumps stay

# 2. the 64-bit host side on the 32-bit dumps
for f in "$O/rec_s32_t32.kzrt" "$R32/tests/rt_end_normal.kzrt" "$R32/tests/rt_end_segv.rec.kzrt" "$R32/tests/rt_end_term-kill.rec.kzrt"; do
  "$R64/kz-rt-dump" "$f" > "$O/$(basename "$f").dump64.txt" 2>&1
  log "kz-rt-dump (64-bit) $(basename "$f"): rc=$? $(grep -E '^format ' "$O/$(basename "$f").dump64.txt") | $(grep -E '^runx end=' "$O/$(basename "$f").dump64.txt") | $(tail -1 "$O/$(basename "$f").dump64.txt")"
done
"$KZB/tests/cfs-free/test_kzrt_recovered" "$R32/tests" > "$O/test_kzrt_recovered32.txt" 2>&1
log "test_kzrt_recovered (64-bit) on the 32-bit rt_end dumps: rc=$? ($(tail -1 "$O/test_kzrt_recovered32.txt"))"
# the unit-test manifest of test_kzrt_recovered.cpp (kManifest), as a file
sed -n '/^const char\* const kManifest =/,/;$/p' "$(dirname "$0")/../../../tests/cfs-free/test_kzrt_recovered.cpp" |
  sed -n 's/^ *"\(.*\)\\n";*$/\1/p' | sed 's/\\"/"/g' > "$O/unit_rt_end.kzs"
log "manifest unit_rt_end.kzs (kManifest of tests/cfs-free/test_kzrt_recovered.cpp): $(grep -c . "$O/unit_rt_end.kzs") lines sha256 $(sha256sum "$O/unit_rt_end.kzs" | cut -c1-64)"
for f in "$O/rec_s32_t32.kzrt" "$R32/tests/rt_end_normal.kzrt"; do
  n=$(basename "$f" .kzrt)
  rm -f "$O/$n.kzt"
  "$KZB/tools/kz/kz" convert --kzrt "$f" --manifest "$O/unit_rt_end.kzs" --out "$O/$n.kzt" > "$O/$n.convert.txt" 2>&1
  log "kz convert --kzrt (64-bit) $n: rc=$? | $(grep -E '^neutral layer' "$O/$n.convert.txt") | $(grep -E '^end: ' "$O/$n.convert.txt")"
done

# 3. the normal dumps of the two builds, record by record
mask() { sed -E 's/ t=[0-9]+/ t=T/; s/ pc=0x[0-9a-f]+/ pc=PC/; s/ key=tid:[0-9]+/ key=tid:K/; s/ actor=[0-9]+ / actor=A /' "$1"; }
"$R64/kz-rt-dump" --events-only "$R32/tests/rt_end_normal.kzrt" > "$O/normal32.events" 2>&1
"$R64/kz-rt-dump" --events-only "$R64/tests/rt_end_normal.kzrt" > "$O/normal64.events" 2>&1
if cmp -s <(mask "$O/normal32.events" | sort) <(mask "$O/normal64.events" | sort); then same=yes; else same=NO; fi
log "normal dumps 32 / 64: events $(grep -c '^ev ' "$O/normal32.events") / $(grep -c '^ev ' "$O/normal64.events"); the same records with thread ids, times, return addresses and actor numbers masked: $same"

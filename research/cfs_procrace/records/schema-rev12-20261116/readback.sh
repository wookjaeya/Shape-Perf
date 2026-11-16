#!/usr/bin/env bash
# readback.sh KZDUMP OUT -- preserved containers of each kind read by one kz-dump: for each, the sha256 of the file, the
# rc of `kz-dump --check`, the artifact hash `kz-dump --hash` prints, and the sha256, line and byte counts of the canonical
# dump text (streamed; nothing but OUT is written).  Run with the kz-dump of schema revision 11 (05623054, before the
# build) and with the new one; the two OUT files must be byte-identical apart from their first line (the tool line).
# Reads only the containers.
set -uo pipefail
KD=${1:?usage: readback.sh KZDUMP OUT}; OUT=${2:?out}
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
C=(
  "facts:T8CF c1_pre_tx|/home/user/work/redesign/step2a/tests_8c_fix/c1_pre_tx/NP/facts.kz"
  "basis:T8CF c1_pre_tx|/home/user/work/redesign/step2a/tests_8c_fix/c1_pre_tx/NP/basis.kz"
  "facts:T8CF c5_641|/home/user/work/redesign/step2a/tests_8c_fix/c5_641/NP/facts.kz"
  "run(kzrt, neutral):W6 G-I c1 pre_1df262a_tx|/home/user/work/w6/replay/c1/replay/gi/pre_1df262a_tx/run.kzt"
  "run(F0 converted):W6 f0ctl C_pre_1df262a_tx|/home/user/work/w6/replay/c1/replay/f0ctl/C_pre_1df262a_tx/run.kzt"
  "run(F0 raw):R2 development record|/home/user/Shape-Perf/research/cfs_procrace/env/logs/s1r2_p1.kzt"
  "run(kzrt, neutral):C5 641 boot|/home/user/work/c5run/641/convert/run.kzt"
  "replay:W6 G-I c1 pre_1df262a_tx cur|/home/user/work/w6/replay/c1/replay/gi/pre_1df262a_tx/cur.kz"
  "replay:C5 641 cur|/home/user/work/c5run/replay/641/cur.kz"
  "run(kzrt, neutral):C1 boot of the observation cycle|/home/user/work/redesign/obs1/c1pre_convert/run.kzt"
)
{
  echo "# readback.sh kz-dump $(sha256sum < "$KD" | cut -c1-64) $(date -u +%FT%TZ)"
  rc=0
  for e in "${C[@]}"; do
    kind=${e%%|*}; p=${e#*|}
    fh=$(sha256sum < "$p" | cut -c1-64)
    "$KD" --check "$p" > /dev/null 2>&1; crc=$?
    ah=$("$KD" --hash "$p" 2>&1)
    dh=$("$KD" "$p" 2> /dev/null | tee >(wc -l > "$OUT.l") >(wc -c > "$OUT.c") | sha256sum | cut -c1-64); drc=${PIPESTATUS[0]}
    sleep 0.2
    echo "$kind	$p	file=$fh	check_rc=$crc	hash=$ah	dump_rc=$drc	dump_sha256=$dh	lines=$(cat "$OUT.l")	bytes=$(cat "$OUT.c")"
    [ "$crc" = 0 ] && [ "$drc" = 0 ] || rc=1
  done
  rm -f "$OUT.l" "$OUT.c"
  exit $rc
} > "$OUT"

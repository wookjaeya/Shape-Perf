#!/usr/bin/env bash
# convert_c1boot.sh KZ KZDUMP OUT -- the dump of the observation cycle's C1 boot converted again by KZ with the arguments of
# that conversion (records/obs1-20261116-boot/run_boot.sh step convert: the generated plan's manifest, the derived build's
# binding manifest, the derived build's basis), into the new directory OUT; its kz-dump text and the conversion log are
# compared with the preserved ones (/home/user/work/redesign/obs1/c1pre_convert: run.kzt faa410e1 by kz 931dd39c).
# Allowed: the lines that carry the converting tool's hash and the hashes that cover it (header tool_sha256= and
# artifact_sha256=, the convert log's "wrote ... artifact_sha256 <hash>" line and the output path in it); every other line
# equal.  The containers are also compared byte by byte (ranges of differing 1-based offsets).  rc 0 when only those lines
# differ, 1 otherwise.
set -uo pipefail
KZ=${1:?usage: convert_c1boot.sh KZ KZDUMP OUT}; KD=${2:?kz-dump}; OUT=${3:?out}
O=/home/user/work/redesign/obs1
P=$O/plangen/c1_pre_tx; W=$O/c1pre_derived; BP=$O/c1pre_bprime/c1_pre_tx/NP; RUN=$O/c1pre_boot/tx; REF=$O/c1pre_convert
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >&2; exit 2; }
mkdir -p "$OUT"
"$KZ" convert --kzrt "$RUN/dump/tx.kzrt" --manifest "$P/plan.kzs" --manifest "$W/binds_instr.kzs" --basis "$BP/basis.kz" \
  --out "$OUT/run.kzt" > "$OUT/convert.log" 2>&1; rc=$?
echo "convert rc=$rc" >> "$OUT/convert.log"
"$KD" "$OUT/run.kzt" > "$OUT/run.kzt.txt" 2>&1; drc=$?
MASK=(-e 's/^(tool_sha256|artifact_sha256)=[0-9a-f]{64}$/\1=H/' -e 's/^wrote .* artifact_sha256 [0-9a-f]{64}$/wrote OUT artifact_sha256 H/')
bad=0
for f in run.kzt.txt convert.log; do
  n=$(diff "$REF/$f" "$OUT/$f" | grep -cE '^[<>]')
  m=$(diff <(sed -E "${MASK[@]}" "$REF/$f") <(sed -E "${MASK[@]}" "$OUT/$f") | grep -cE '^[<>]')
  echo "$f: $(wc -l < "$OUT/$f") lines, $n differing lines; with the tool hash lines masked: $m"
  [ "$m" = 0 ] || { bad=1; diff <(sed -E "${MASK[@]}" "$REF/$f") <(sed -E "${MASK[@]}" "$OUT/$f") | head -20; }
done
r=$(cmp -l "$REF/run.kzt" "$OUT/run.kzt" 2> /dev/null | awk 'NR==1{s=$1;e=$1;n=1;next} {n++; if ($1<=e+8) e=$1; else {printf "%s%d-%d", sep, s, e; sep=","; s=$1; e=$1}} END{if (n) printf "%s%d-%d", sep, s, e; printf " (%d bytes)", n}')
echo "run.kzt: sizes $(stat -c%s "$REF/run.kzt") / $(stat -c%s "$OUT/run.kzt"); differing bytes ${r:-none}; convert rc $rc, kz-dump rc $drc"
[ "$rc" = 0 ] && [ "$drc" = 0 ] || bad=1
echo "convert_c1boot: $([ $bad = 0 ] && echo 'only the tool hash lines differ' || echo 'UNDECLARED differences')"
exit $bad

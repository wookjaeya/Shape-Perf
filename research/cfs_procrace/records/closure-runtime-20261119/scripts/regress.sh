#!/usr/bin/env bash
# regress.sh OUT CHAIN_CHECK -- the kept boots' dumps (format 1.4) read by the MC tools as they are now (before the build:
# the old tools; after it: the rebuilt ones), into the new directory OUT (results 23.0):
#   c1.*, c5.*      as records/manifest-v4-20261116/regress_tools.sh: kz-rt-dump --manifest, kz convert (C1 with the B'
#                   basis), kz-dump of the container (kz-dump is not rebuilt)
#   c1.plain.txt, c5.plain.txt, abi32.plain.txt, abi64.plain.txt
#                   kz-rt-dump without a manifest of the two boot dumps and of the two kept ABI dumps of the 32-bit check
#   c1.chain.txt    the chain check CHAIN_CHECK (the script before or after the build) with the arguments of
#                   records/obs1-20261116-boot/chain.txt (its first two lines)
# Each command's rc in OUT/rc.txt.  Compared by compare_regress.py.
set -uo pipefail
O=${1:?usage: regress.sh OUT CHAIN_CHECK}; CC=${2:?usage: regress.sh OUT CHAIN_CHECK}
MC=/home/user/work/merge-check/kz-llvm; W=/home/user/work/redesign/obs1; K=/home/user/Shape-Perf/research/cfs_procrace/kairos
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
C1D=$W/c1pre_boot/tx/dump/tx.kzrt; C1M=("$W/plangen/c1_pre_tx/plan.kzs" "$W/c1pre_derived/binds_instr.kzs"); C1B=$W/c1pre_bprime/c1_pre_tx/NP/basis.kz
C5D=/home/user/work/c5run/641/dump/cpu1.kzrt; C5M=("$K/profiles/c5-cfe-6.4.x/instr/sites-641.draft.kzs" /home/user/work/c5tools/build/641/binds_instr.kzs)
A32=/home/user/work/c5tools/pre/abi/rec_s32_t32.kzrt; A64=/home/user/work/c5tools/pre/abi/rec_s64_t64.kzrt
{
  echo "kz-rt-dump $(sha256sum < $MC/runtime/kz-rt-dump | cut -c1-64) kz $(sha256sum < $MC/tools/kz/kz | cut -c1-64) kz-dump $(sha256sum < $MC/tools/kz-dump/kz-dump | cut -c1-64) chain_check $(sha256sum < "$CC" | cut -c1-64)"
  "$MC/runtime/kz-rt-dump" --manifest "${C1M[0]}" --manifest "${C1M[1]}" "$C1D" > "$O/c1.dump.txt" 2>&1; echo "c1 kz-rt-dump rc=$?"
  "$MC/tools/kz/kz" convert --kzrt "$C1D" --manifest "${C1M[0]}" --manifest "${C1M[1]}" --basis "$C1B" --out "$O/c1.run.kzt" > "$O/c1.convert.log" 2>&1; echo "c1 convert rc=$?"
  "$MC/tools/kz-dump/kz-dump" "$O/c1.run.kzt" > "$O/c1.run.kzt.txt" 2>&1; echo "c1 kz-dump rc=$?"
  "$MC/runtime/kz-rt-dump" --manifest "${C5M[0]}" --manifest "${C5M[1]}" "$C5D" > "$O/c5.dump.txt" 2>&1; echo "c5 kz-rt-dump rc=$?"
  "$MC/tools/kz/kz" convert --kzrt "$C5D" --manifest "${C5M[0]}" --manifest "${C5M[1]}" --out "$O/c5.run.kzt" > "$O/c5.convert.log" 2>&1; echo "c5 convert rc=$?"
  "$MC/tools/kz-dump/kz-dump" "$O/c5.run.kzt" > "$O/c5.run.kzt.txt" 2>&1; echo "c5 kz-dump rc=$?"
  "$MC/runtime/kz-rt-dump" "$C1D" > "$O/c1.plain.txt" 2>&1; echo "c1 plain rc=$?"
  "$MC/runtime/kz-rt-dump" "$C5D" > "$O/c5.plain.txt" 2>&1; echo "c5 plain rc=$?"
  "$MC/runtime/kz-rt-dump" "$A32" > "$O/abi32.plain.txt" 2>&1; echo "abi32 plain rc=$?"
  "$MC/runtime/kz-rt-dump" "$A64" > "$O/abi64.plain.txt" 2>&1; echo "abi64 plain rc=$?"
  B=$K/../records/obs1-20261116-boot/chain.txt
  A1=$(sed -n '1s/^# chain_check.py //p' "$B"); A2=$(sed -n '2s/^# and //p' "$B")
  # shellcheck disable=SC2086
  python3 -I "$CC" $A1 $A2 > "$O/c1.chain.txt" 2>&1; echo "c1 chain rc=$?"
} > "$O/rc.txt"
sed "s#$O#<OUT>#g" -i "$O/c1.convert.log" "$O/c5.convert.log"
cat "$O/rc.txt"

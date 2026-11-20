#!/usr/bin/env bash
# regress_tools.sh OUT -- the derived bindings of the kept version 3 plans with the MC tools as they are now (run before
# the build: the old tools; after it: the relinked ones), into the new directory OUT:
#   c1.dump.txt   kz-rt-dump --manifest (the C1 plan of the first observation path, plangen/c1_pre_tx, and its derived build's binding list) of the C1
#                 boot's dump (as records/obs1-20261116-boot/run_boot.sh rtdump-mc)
#   c1.run.kzt    kz convert of the same dump with the same manifests and the B' basis (as run_boot.sh convert), c1.convert.log
#   c1.run.kzt.txt kz-dump of it (kz-dump is not rebuilt)
#   c5.*          the same for the C5 boot of 6.4.1 (the draft manifest of the C5 profile and its binding list; no basis)
# Each command's rc in OUT/rc.txt.  The old and the new OUT are compared by compare_tools.py.
set -uo pipefail
O=${1:?usage: regress_tools.sh OUT}; MC=/home/user/work/merge-check/kz-llvm; W=/home/user/work/redesign/obs1
K=/home/user/Shape-Perf/research/cfs_procrace/kairos
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
C1D=$W/c1pre_boot/tx/dump/tx.kzrt; C1M=("$W/plangen/c1_pre_tx/plan.kzs" "$W/c1pre_derived/binds_instr.kzs"); C1B=$W/c1pre_bprime/c1_pre_tx/NP/basis.kz
C5D=/home/user/work/c5run/641/dump/cpu1.kzrt; C5M=("$K/profiles/c5-cfe-6.4.x/instr/sites-641.draft.kzs" /home/user/work/c5tools/build/641/binds_instr.kzs)
{
  echo "kz-rt-dump $(sha256sum < $MC/runtime/kz-rt-dump | cut -c1-64) kz $(sha256sum < $MC/tools/kz/kz | cut -c1-64) kz-dump $(sha256sum < $MC/tools/kz-dump/kz-dump | cut -c1-64)"
  "$MC/runtime/kz-rt-dump" --manifest "${C1M[0]}" --manifest "${C1M[1]}" "$C1D" > "$O/c1.dump.txt" 2>&1; echo "c1 kz-rt-dump rc=$?"
  "$MC/tools/kz/kz" convert --kzrt "$C1D" --manifest "${C1M[0]}" --manifest "${C1M[1]}" --basis "$C1B" --out "$O/c1.run.kzt" > "$O/c1.convert.log" 2>&1; echo "c1 convert rc=$?"
  "$MC/tools/kz-dump/kz-dump" "$O/c1.run.kzt" > "$O/c1.run.kzt.txt" 2>&1; echo "c1 kz-dump rc=$?"
  "$MC/runtime/kz-rt-dump" --manifest "${C5M[0]}" --manifest "${C5M[1]}" "$C5D" > "$O/c5.dump.txt" 2>&1; echo "c5 kz-rt-dump rc=$?"
  "$MC/tools/kz/kz" convert --kzrt "$C5D" --manifest "${C5M[0]}" --manifest "${C5M[1]}" --out "$O/c5.run.kzt" > "$O/c5.convert.log" 2>&1; echo "c5 convert rc=$?"
  "$MC/tools/kz-dump/kz-dump" "$O/c5.run.kzt" > "$O/c5.run.kzt.txt" 2>&1; echo "c5 kz-dump rc=$?"
} > "$O/rc.txt"
sed "s#$O#<OUT>#g" -i "$O/c1.convert.log" "$O/c5.convert.log"
cat "$O/rc.txt"

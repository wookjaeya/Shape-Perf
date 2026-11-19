#!/usr/bin/env bash
# listing.sh OUT -- run listing.py with the readers, baselines, exclusions and never list of this step (results 26.2).
#   readers: the scripts and inputs of the steps still to run: the boot step (run_cycle.sh, its commands, the boot
#   declaration), the code and freeze steps (regress.sh and dryrun.sh of the frozen explanation step, the inputs of the
#   2026-11-16 boot that the regression reads), and the candidate list's command inputs (results 25.10).
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
OUT=${1:?out}
P=$R/records/closure-c1prep-20261119
X=$R/records/closure-explain-rev4-20261119
exec python3 -I "$HERE/listing.py" --research "$R" --obs1 /home/user/work/redesign/obs1 --work /home/user/work \
  --ledger "$R/records/LEDGER.tsv" --out "$OUT" \
  --reader "boot=$P/boot/run_cycle.sh" --reader "boot-commands=$P/boot/commands.txt" \
  --reader "declaration=$P/boot/declaration.tsv" \
  --reader "regress=$X/scripts/regress.sh" --reader "dryrun=$X/scripts/dryrun.sh" \
  --reader "regress-inputs=$R/records/obs1-20261116-reviewfix/inputs.txt" \
  --reader "extra=$HERE/readers_extra.txt" \
  --baselines "$X" \
  --twin ll_final_order --reclaim rf2_pre_relcheck --reclaim rf2_pre_order --reclaim rf2_pre_final_order \
  --never 'cz_*=the current round (the last kz-static build) and its plans' \
  --never 'c1p2_*=the derived builds of the cycle' \
  --never 'c1pre_*=the 2026-11-16 boot, its build and its explanations' \
  --exclude expect_src=source-trees --exclude c5/src=source-tree --exclude c5/dl=source-archives \
  --exclude procrace/cfs_ref=source-tree --exclude procrace/refs=source-refs \
  --exclude instr=toolchains --exclude c5tools/kz-llvm=toolchain --exclude c5tools/plugin-build=toolchain \
  --exclude merge-check=toolchain --exclude kairos-build=kairos-build --exclude preserve=preserve \
  --exclude llvm-project=llvm-project \
  --expand redesign --expand c5 --expand c5tools --expand procrace --expand rebuild --expand w6

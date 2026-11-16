#!/usr/bin/env bash
# groups.sh -- the manifest groups of the regression check, one line per group: "<id>\t<file> [<file>...]":
# every site manifest of the repository (git ls-files '*.kzs' under kairos, each alone), every generated plan of the
# earlier steps under /home/user/work/redesign/obs1/*/<build>/plan.kzs (each alone), and the two plans that a derived
# build bound, each with its binding list (as kz-rt-dump and kz convert read them).
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; R="$(cd "$HERE/../../.." && pwd)"; REPO="$(cd "$R/../.." && pwd)"
O=/home/user/work/redesign/obs1
n=0
for f in $(git -C "$REPO" ls-files 'research/cfs_procrace/kairos/*.kzs'); do n=$((n + 1)); printf 'r%03d\t%s\n' $n "$REPO/$f"; done
n=0
for f in $(ls -d $O/*/*/plan.kzs | sort); do n=$((n + 1)); printf 'p%03d\t%s\n' $n "$f"; done
printf 'b001\t%s %s\n' "$O/plangen/c1_pre_tx/plan.kzs" "$O/c1pre_derived/binds_instr.kzs"
printf 'b002\t%s %s\n' "$R/kairos/profiles/c5-cfe-6.4.x/instr/sites-641.draft.kzs" /home/user/work/c5tools/build/641/binds_instr.kzs

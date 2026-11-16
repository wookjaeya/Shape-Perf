#!/usr/bin/env bash
# extract.sh off|final [OUT] -- the five reference builds re-extracted with the kz-static of the loop flag and lock handle
# step through the single-build runner (env/redesign/one_build.sh: basis --no-profile, extract --no-profile --targets-rule
# R-T1,R-T2, relations; ONE_BUILD_SHOW off), each into its own new directory, one after the other; a failing build stops
# nothing else and gives rc 1.  off: no option (the default path).  final: the final option-on round, every extraction
# option of the stage: ONE_BUILD_EXTRACT_EXTRA = --object-access, --semantics with every framework semantics data file and
# the P0 declarations file of kairos/semantics/ (in name order; the name-space files are no input of kz-static),
# --result-flow, --loop-defs and --lock-handles, written into args.txt.  Default OUT: ll_off_extract / ll_final_extract.
set -uo pipefail
REPO=/home/user/Shape-Perf/research/cfs_procrace
M=${1:?usage: extract.sh off|final [OUT]}
case "$M" in
  off) X= ;;
  final) X=--object-access
      for f in "$REPO"/kairos/semantics/framework_semantics_*.txt "$REPO"/kairos/semantics/p0_declarations_*.txt; do X="$X --semantics $f"; done
      X="$X --result-flow --loop-defs --lock-handles" ;;
  *) echo "usage: extract.sh off|final [OUT]" >&2; exit 64 ;;
esac
O=${2:-/home/user/work/redesign/obs1/ll_${M}_extract}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  ONE_BUILD_EXTRACT_EXTRA="$X" bash "$REPO/env/redesign/one_build.sh" "$b" "$O/$b"; r=$?
  echo "$b rc=$r"
  [ $r = 0 ] || rc=1
done
exit $rc

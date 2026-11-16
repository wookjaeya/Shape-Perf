#!/usr/bin/env bash
# extract.sh off|on [OUT] -- the five reference builds re-extracted with the kz-static of the types and object access step
# through the single-build runner (env/redesign/one_build.sh: basis --no-profile, extract --no-profile --targets-rule
# R-T1,R-T2, relations; ONE_BUILD_SHOW off), `on` with ONE_BUILD_EXTRACT_EXTRA=--object-access (the option, written into
# args.txt), each into its own new directory, one after the other; a failing build stops nothing else and gives rc 1.
# Default OUT: objacc_off_extract / objacc_on_extract.
set -uo pipefail
REPO=/home/user/Shape-Perf/research/cfs_procrace
M=${1:?usage: extract.sh off|on [OUT]}
case "$M" in off) X= ;; on) X=--object-access ;; *) echo "usage: extract.sh off|on [OUT]" >&2; exit 64 ;; esac
O=${2:-/home/user/work/redesign/obs1/objacc_${M}_extract}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  ONE_BUILD_EXTRACT_EXTRA="$X" bash "$REPO/env/redesign/one_build.sh" "$b" "$O/$b"; r=$?
  echo "$b rc=$r"
  [ $r = 0 ] || rc=1
done
exit $rc

#!/usr/bin/env bash
# extract.sh -- the five reference builds re-extracted with the new kz-static through the single-build runner
# (env/redesign/one_build.sh: basis --no-profile, extract --no-profile --targets-rule R-T1,R-T2, relations), each into its
# own new directory, one after the other; a failing build stops nothing else and gives rc 1.
set -uo pipefail
REPO=/home/user/Shape-Perf/research/cfs_procrace
O=${O:-/home/user/work/redesign/obs1/order_rev1_extract}
[ -e "$O" ] && { echo "refusing: $O exists" >&2; exit 2; }
mkdir -p "$O"
rc=0
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  bash "$REPO/env/redesign/one_build.sh" "$b" "$O/$b"; r=$?
  echo "$b rc=$r"
  [ $r = 0 ] || rc=1
done
exit $rc

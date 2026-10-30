#!/usr/bin/env bash
# run_r2_pilot.sh -- one pilot boot of the derived R2 build (label: pilot/dev; not a main-experiment result) for the
# kz-instr-clang fitness check.  The cFS command is the one kairos/probe/scripts/observe_cfs_smoke.sh gives kz observe:
# env/run_cfs.sh operational ci-es-restart-poweron <log> (CFS_DIR names the derived tree, run_cfs.sh v5), with the
# persisted-state snapshots before and after (env/tools/persist_snapshot.sh), without the F0 observer.
# Differences from that procedure (written to <run>/conditions.txt): no kz observe and no anchors; the derived build;
# the environment adds KZ_RT_MODE=record, KZ_RT_DUMP, KZ_RT_RUN_ID, KZ_RT_INSTANCE=1 (read by the runtime's start-up
# object), which run_cfs.sh passes on to core-cpu1 as it passes the caller's environment.
#   run_r2_pilot.sh <derived cFS dir> <run dir> <run id>        (as root, CFS_NORMAL_USER set or ubuntu)
set -uo pipefail
CFS=$1; RUN=$2; RID=$3
export CFS_NORMAL_USER=${CFS_NORMAL_USER:-ubuntu}
unset KZ
ENV_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../env" && pwd)"
EXE="$CFS/build-native_std/exe/cpu1"
[ "$(id -u)" = 0 ] || { echo "must run as root (run_cfs.sh drops to CFS_NORMAL_USER)" >&2; exit 2; }
[ ! -e "$RUN" ] || { echo "refusing to overwrite $RUN" >&2; exit 2; }
mkdir -p "$RUN"; chown "$CFS_NORMAL_USER:$CFS_NORMAL_USER" "$RUN"
"$ENV_DIR/tools/persist_snapshot.sh" before "$EXE" > "$RUN/persist_before.txt" 2>&1
echo "core-cpu1 sha256=$(sha256sum "$EXE/core-cpu1" | cut -c1-64)" > "$RUN/files.txt"
KZ_RT_MODE=record KZ_RT_DUMP="$RUN/kz_rt.dump" KZ_RT_RUN_ID="$RID" KZ_RT_INSTANCE=1 CFS_DIR="$CFS" \
  "$ENV_DIR/run_cfs.sh" operational ci-es-restart-poweron "$RUN/cfs_operational_esrestart.log" > "$RUN/run_cfs.out" 2>&1
echo "run_cfs.sh exit=$?" >> "$RUN/files.txt"
"$ENV_DIR/tools/persist_snapshot.sh" after "$EXE" > "$RUN/persist_after.txt" 2>&1
[ -f "$RUN/kz_rt.dump" ] && echo "dump=yes bytes=$(stat -c %s "$RUN/kz_rt.dump")" >> "$RUN/files.txt" || echo "dump=no" >> "$RUN/files.txt"
cat > "$RUN/conditions.txt" <<EOT
# conditions of this pilot run (CONDITIONS_POLICY: values from the reference procedure or recorded as differences)
cfs.build      derived build $CFS (kz-instr-clang; build differences in <work>/build_diff_full.txt)   deviation (instrumented, derived configuration)
cfs.command    env/run_cfs.sh operational ci-es-restart-poweron (as observe_cfs_smoke.sh step 4)       from-reference
cfs.user       $CFS_NORMAL_USER (run_cfs.sh setpriv; README L102)                                      from-reference
observer       none (the F0 observer is not attached); the in-process runtime records                  deviation
environment    + KZ_RT_MODE=record KZ_RT_DUMP=$RUN/kz_rt.dump KZ_RT_RUN_ID=$RID KZ_RT_INSTANCE=1 CFS_DIR=$CFS   deviation
persisted      the derived exe directory is new (no EEPROM.DAT, no key files before the first run; the PSP creates them); pilot3 ran with an all-zero EEPROM.DAT and empty key files (v153_pilot3_persist_before.txt)   deviation
run.count      this run alone (pilot/dev)                                                              unspecified_by_reference
EOT
exit 0

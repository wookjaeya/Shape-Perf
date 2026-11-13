#!/usr/bin/env bash
# run_gi_r2.sh <W> <out dir> <kz-rt-dump> [poweron] -- ONE pilot/dev boot of the stage D derived (kzgcc) R2 build (derive_r2.sh) in
# group G-I (KZ_RT_MODE=record: recording on, no gate; SPEC rev 1.4 8.6).  Not a main-experiment result.  Started the way
# kairos/probe/scripts/observe_cfs_smoke.sh starts cFS (F0 v153_pilot3) but without the F0 observer: env/run_cfs.sh
# operational ci-es-restart-poweron <log> (README L102-L114, the bundle CI's ES restart POWERON stop) with CFS_DIR=<W>/cFS
# (run_cfs.sh v5 override, CONDITIONS.md H-9), and the persisted-state snapshots of env/tools/persist_snapshot.sh before and
# after (S0, as observe_cfs_smoke.sh steps 1 and 5).  Run as root with CFS_NORMAL_USER set (run_cfs.sh rule).
# Start state: whatever the exe directory of <W> holds (the reference procedure does not reset it).  For a derived build
# that has not been booted this is its fresh install (no EEPROM.DAT, no key files), whereas F0 v153_pilot3 started from the
# reference exe directory after earlier boots (zero-filled EEPROM.DAT, empty key files).  Which start state derived runs
# use was a pending PI decision (plan/instrumentation_selection_20261026.md 7, item 2); without the fourth argument this
# script only records it.  With "poweron" (PI decision 2026-10-28 Q3, plan/r2_start_poweron_20261028.md) the R2 common
# Power-On start is used: env/tools/start_state_poweron.sh prepares and records the start state and run_cfs.sh v6 gets
# RUN_CFS_ARGS="--reset PO", exactly as observe_cfs_smoke.sh --start poweron does for F0.
# Differences from v153_pilot3, in <out>/conditions.txt: no kz observe; the derived binary; the environment of core-cpu1
# += KZ_RT_MODE=record KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE (values not in run_cfs.sh's record; cFS does not read them).
set -uo pipefail
W=${1:?usage}; OUT=${2:?usage}; RTDUMP=${3:?kz-rt-dump}; START=${4:-as-is}
case "$START" in as-is|poweron) ;; *) echo "fourth argument is poweron" >&2; exit 2 ;; esac
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"
SITES="$KZ/profiles/cfs-v7.0.1/instr/sites.kzs"
ENV_DIR="$(cd "$KZ/../env" && pwd)"
: "${CFS_NORMAL_USER:?set CFS_NORMAL_USER (env/ENV.md E1)}"
[ "$(id -u)" = 0 ] || { echo "must run as root (run_cfs.sh drops to CFS_NORMAL_USER)" >&2; exit 2; }
[ ! -e "$OUT" ] || { echo "refusing to overwrite $OUT" >&2; exit 2; }
if pgrep -x 'core-(tx|rx|cpu1)' > /dev/null; then echo "another cFS instance runs; not starting" >&2; exit 3; fi
unset KZ
mkdir -p "$OUT/dump"; chown "$CFS_NORMAL_USER:$CFS_NORMAL_USER" "$OUT/dump"; OUT="$(cd "$OUT" && pwd)"
EXE="$W/cFS/build-native_std/exe/cpu1"
M="$OUT/run.meta"; TB=$'\t'
{ echo "label=pilot/dev boot of the stage D derived (kzgcc) R2 build, group G-I (record) (not a main-experiment result)"
  echo "kernel=$(uname -r) nproc=$(nproc) core-cpu1 sha256=$(sha256sum "$EXE/core-cpu1" | cut -c1-64)"
  echo "manifest $SITES sha256=$(sha256sum "$SITES" | cut -c1-64)"
  echo "bindings $W/binds_instr.kzs sha256=$(sha256sum "$W/binds_instr.kzs" | cut -c1-64)"
  echo "start_utc=$(date -u +%FT%T.%3NZ)"; } > "$M"
{ echo "# conditions of the G-I boot of the derived R2 build (pilot/dev; derived configuration of cfs_ref/cFS build-native_std)"
  echo "item${TB}run.group${TB}G-I: KZ_RT_MODE=record (recording on, no gate, no plan)${TB}unset"
  echo "item${TB}run.count${TB}this boot alone; repetition counts are the PI's (P8, D14)${TB}unspecified_by_reference"
  echo "item${TB}cfs.build${TB}$W/cFS (derive_r2.sh instr; differences D1-D4 in $W/build_instr.record)${TB}deviation"
  echo "item${TB}cfs.start_stop${TB}env/run_cfs.sh operational ci-es-restart-poweron (as observe_cfs_smoke.sh)${TB}from-reference"
  if [ "$START" = poweron ]; then
    echo "item${TB}start_state${TB}R2 common Power-On start (PI decision 2026-10-28 Q3, plan/r2_start_poweron_20261028.md): EEPROM.DAT and key files absent and created by the PSP (cfe_psp_eeprom_mmap_file.c L55 L62, cfe_psp_memory.c L611-615), no SysV shm of the run user, /dev/shm/osal:RAM absent or empty (prior_state/)${TB}from-reference"
    echo "deviation${TB}cfs.args${TB}README L112: ./core-cpu1 with no options${TB}./core-cpu1 --reset PO (run_cfs.sh v6 RUN_CFS_ARGS)${TB}PI decision 2026-10-28 Q3: Power-On specified explicitly${TB}the reset type no longer depends on a boot record left in SysV shm (cfe_psp_start.c L408-434)${TB}no"
  else
    echo "item${TB}start_state${TB}the exe directory as it is (persist_before.txt); fresh install of the derived build${TB}unspecified_by_reference"
  fi
  echo "deviation${TB}observer.method${TB}F0: kz observe (perf-sample uprobes)${TB}no observer; compiler-inserted hooks (kzgcc) and the kzrt runtime record into a static pool, dumped at exit${TB}stage D G-I boot${TB}hook code in the instrumented units; runtime bss 4 MiB; not measured${TB}no"
  echo "deviation${TB}cfs.environment${TB}the F0 environment of core-cpu1${TB}+= KZ_RT_MODE KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE (the same kind of difference as CORD_OUT, CONDITIONS.md H-9)${TB}the runtime is configured from the environment${TB}none expected; not verified${TB}no"
} > "$OUT/conditions.txt"
ARGS=""
if [ "$START" = poweron ]; then
  "$ENV_DIR/tools/start_state_poweron.sh" "$EXE" "$OUT/prior_state" "$CFS_NORMAL_USER" >> "$M" 2>&1 \
    || { echo "start state refused ($OUT/prior_state)" >&2; exit 9; }
  ARGS="--reset PO"
fi
"$ENV_DIR/tools/persist_snapshot.sh" "before G-I boot" "$EXE" > "$OUT/persist_before.txt" 2>&1
env CFS_DIR="$W/cFS" CFS_NORMAL_USER="$CFS_NORMAL_USER" RUN_CFS_ARGS="$ARGS" KZ_RT_MODE=record KZ_RT_DUMP="$OUT/dump/cpu1.kzrt" KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=1 \
    "$ENV_DIR/run_cfs.sh" operational ci-es-restart-poweron "$OUT/console.log" > "$OUT/run_cfs.out" 2>&1
echo "run_cfs.sh exit=$?" >> "$M"
"$ENV_DIR/tools/persist_snapshot.sh" "after G-I boot" "$EXE" > "$OUT/persist_after.txt" 2>&1
grep -E '^(stop_reason|exit|operational_line_read_ms|process.exit|exit_status)' "$OUT/console.log.meta" >> "$M" 2>/dev/null
if [ -s "$OUT/dump/cpu1.kzrt" ]; then
  echo "dump cpu1.kzrt bytes=$(stat -c %s "$OUT/dump/cpu1.kzrt") sha256=$(sha256sum "$OUT/dump/cpu1.kzrt" | cut -c1-64)" >> "$M"
  "$RTDUMP" --manifest "$SITES" --manifest "$W/binds_instr.kzs" "$OUT/dump/cpu1.kzrt" > "$OUT/cpu1.dump.txt" 2>&1; echo "kz-rt-dump exit=$?" >> "$M"
else echo "dump cpu1.kzrt missing" >> "$M"; fi
echo "end_utc=$(date -u +%FT%T.%3NZ)" >> "$M"
cat "$M"

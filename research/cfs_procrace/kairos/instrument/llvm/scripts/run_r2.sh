#!/usr/bin/env bash
# run_r2.sh <W> <out dir> <kz-rt-dump> <kz-events> [<binding file>] -- ONE pilot boot of the clang-built R2 tree (pilot/dev, not a
# main-experiment result), started the way kairos/probe/scripts/observe_cfs_smoke.sh starts cFS but without the F0
# observer: env/run_cfs.sh operational ci-es-restart-poweron <log> (README L102-L114, the bundle CI's ES restart
# POWERON stop), with CFS_DIR=<W>/cFS (run_cfs.sh v5 override, CONDITIONS.md H-9) and the persisted-state snapshots
# of env/tools/persist_snapshot.sh before and after.  Run as root with CFS_NORMAL_USER set (run_cfs.sh rule).
# Differences from the F0 run v153_pilot3 (written into <out>/conditions.txt):
#   - no kz observe; the binary is the clang build of build_r2.sh (L-I, or L-A without a binding file: no dump), in <W>/cFS
#   - core-cpu1's environment += KZ_RT_DUMP, KZ_RT_RUN_ID, KZ_RT_INSTANCE (read by the runtime's start object;
#     cFS does not read them) -- the same kind of difference as CORD_OUT (CONDITIONS.md H-9)
#   - the exe directory of the copy starts from its fresh install (no key files of earlier boots)
set -uo pipefail
W=${1:?usage}; OUT=${2:?usage}; RTDUMP=${3:?kz-rt-dump}; EVENTS=${4:?kz-events}; B=${5:-}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITES="$(cd "$HERE/../../gcc/sites" && pwd)/r2_v701.kzs"
ENV_DIR="$(cd "$HERE/../../../../env" && pwd)"
: "${CFS_NORMAL_USER:?set CFS_NORMAL_USER (env/ENV.md E1)}"
[ "$(id -u)" = 0 ] || { echo "must run as root (run_cfs.sh drops to CFS_NORMAL_USER)" >&2; exit 2; }
[ ! -e "$OUT" ] || { echo "refusing to overwrite $OUT" >&2; exit 2; }
unset KZ
mkdir -p "$OUT/dump"; chown "$CFS_NORMAL_USER:$CFS_NORMAL_USER" "$OUT/dump"; OUT="$(cd "$OUT" && pwd)"
EXE="$W/cFS/build-native_std/exe/cpu1"
M="$OUT/run.meta"
{ echo "label=pilot/dev boot of the clang-built R2 tree (LLVM pass-plugin candidate) (stage C fitness check; not a main-experiment result)"
  echo "kernel=$(uname -r) nproc=$(nproc) core-cpu1 sha256=$(sha256sum "$EXE/core-cpu1" | cut -c1-64)"
  echo "manifest $SITES sha256=$(sha256sum "$SITES" | cut -c1-64)"
  echo "start_utc=$(date -u +%FT%T.%3NZ)"; } > "$M"
"$ENV_DIR/tools/persist_snapshot.sh" "before llvm pilot" "$EXE" > "$OUT/persist_before.txt" 2>&1
env CFS_DIR="$W/cFS" CFS_NORMAL_USER="$CFS_NORMAL_USER" KZ_RT_DUMP="$OUT/dump/cpu1.kzrt" KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=1 \
    "$ENV_DIR/run_cfs.sh" operational ci-es-restart-poweron "$OUT/console.log" > "$OUT/run_cfs.out" 2>&1
echo "run_cfs.sh exit=$?" >> "$M"
"$ENV_DIR/tools/persist_snapshot.sh" "after llvm pilot" "$EXE" > "$OUT/persist_after.txt" 2>&1
grep -E '^(stop_reason|exit|operational_line_read_ms|process.exit|exit_status)' "$OUT/console.log.meta" >> "$M" 2>/dev/null
if [ -s "$OUT/dump/cpu1.kzrt" ]; then
  echo "dump cpu1.kzrt bytes=$(stat -c %s "$OUT/dump/cpu1.kzrt") sha256=$(sha256sum "$OUT/dump/cpu1.kzrt" | cut -c1-64)" >> "$M"
  "$RTDUMP" --manifest "$SITES" --manifest "$B" "$OUT/dump/cpu1.kzrt" > "$OUT/cpu1.dump.txt" 2>&1; echo "kz-rt-dump exit=$?" >> "$M"
  "$EVENTS" "$OUT/dump/cpu1.kzrt" > "$OUT/cpu1.events.tsv" 2>&1; echo "kz-events exit=$?" >> "$M"
else echo "dump cpu1.kzrt missing" >> "$M"; fi
echo "end_utc=$(date -u +%FT%T.%3NZ)" >> "$M"
cat "$M"

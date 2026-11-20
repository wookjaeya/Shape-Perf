#!/usr/bin/env bash
# w6_20261029.sh -- the C1/R2 core re-verification campaign after the 2026-10-29 rebuild (PI decision 2026-10-29,
# Architecture/spec/PI_DECISIONS_RESOLVED_20261029.md section 4 step 3; A7: one boot per configuration, P8).
# New runs with new run ids on the rebuilt environment (env/REBUILD_20261029.md); not a restoration of earlier runs.
# Pilot/dev and controlled runs only; not main-experiment results.
#
# What runs, each through the existing driver of its earlier counterpart (their conditions are not changed here):
#   C1 natural, per build (pre = CF 281a941, post = CF 833fdbb; candidates 1df262a, 1b0b338):
#       F0 tx   kairos/probe/scripts/observe_c1_smoke.sh                 G-I tx  instrument/gcc/scripts/run_gi_c1.sh tx
#       F0 rx   kairos/probe/scripts/observe_c1_hold.sh --no-hold        G-I rx  run_gi_c1.sh rx
#   C1 controlled, the recorded stage B and C plans (PD25 section 2 B and C; kairos/docs/c1_hold_runs.md):
#       post builds  hold/hold-cf833fdbb.kzh, label P-C1HOLD (E-06)
#       pre builds   hold/hold-cf281a941.kzh, label researcher intervention, outside P-C1HOLD
#       F0 observe_c1_hold.sh --plan, G-I-H run_gih_c1.sh with the same plan (kz rtplan converts it), tx and rx each
#   R2  F0 observe_cfs_smoke.sh --start poweron, G-I run_gi_r2.sh ... poweron (PD28 Q3, plan/r2_start_poweron_20261028.md)
# The instrumented builds are the 2026-10-29 derived builds (REBUILD section 6, runtime kzrt 0.4.0): C1
# instr/gcc/f1/c1/<cfg>_<cand> with f1/rt-<cfg>/kz-rt-dump, R2 instr/gcc/d4/r2 with d4/rt-r2/kz-rt-dump.
# While the campaign runs, /home/user/work/.campaign_running exists (other work waits for it).
# Harness choices (unspecified_by_reference): the order above, the disk floor, the machine-state record.
#
# Usage: w6_20261029.sh <out root> [c1nat|c1ctl|r2]...   (as root, CFS_NORMAL_USER=ubuntu)
set -uo pipefail
OUT=${1:?out root}; shift
PARTS=("$@"); [ ${#PARTS[@]} -gt 0 ] || PARTS=(c1nat c1ctl r2)
R=/home/user/Shape-Perf/research/cfs_procrace
K=$R/kairos
KZ=/home/user/work/kairos-build/tools/kz/kz
I=/home/user/work/instr/gcc
U=${CFS_NORMAL_USER:?set CFS_NORMAL_USER}
[ "$(id -u)" = 0 ] || { echo "must run as root" >&2; exit 2; }
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"; LOG=$OUT/campaign.log
TFS=$OUT/tracefs
FLAG=/home/user/work/.campaign_running
want() { local p; for p in "${PARTS[@]}"; do [ "$p" = "$1" ] && return 0; done; return 1; }
quiet() { { echo "## $1 $(date -u +%FT%T.%3NZ) kernel=$(uname -r)"; uptime; ps -eo pid,pcpu,comm --sort=-pcpu | head -6; df -BM --output=avail /home/user | tail -1; } >> "$LOG"; }
floor() { local df; df=$(df --output=avail -BM /home/user | tail -1 | tr -dc 0-9); [ "$df" -gt 3000 ] || { echo "disk below 3 GB, stop" >> "$LOG"; exit 5; }; }
: > "$FLAG"; trap 'rm -f "$FLAG"; umount "$TFS" 2>/dev/null' EXIT
{ echo "# w6_20261029.sh start $(date -u +%FT%T.%3NZ) parts=${PARTS[*]}"
  echo "repo $(git -C $R rev-parse HEAD) diff_kairos_lines=$(git -C $R diff HEAD -- kairos | wc -l)"
  echo "kz $KZ sha256=$(sha256sum $KZ | cut -c1-64)"
  echo "uname $(uname -a)"; } >> "$LOG"
mkdir -p "$TFS"; mountpoint -q "$TFS" || mount -t tracefs nodev "$TFS"

if want c1nat; then
  quiet "C1 natural"
  for CFG in pre post; do for CAND in 1df262a 1b0b338; do
    floor
    echo "$(date -u +%T) C1 natural $CFG $CAND F0 tx" >> "$LOG"
    "$K/probe/scripts/observe_c1_smoke.sh" $CFG $CAND "$KZ" --out-dir "$OUT/c1nat/f0tx" --tracefs "$TFS" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    echo "$(date -u +%T) C1 natural $CFG $CAND G-I tx" >> "$LOG"
    "$K/instrument/gcc/scripts/run_gi_c1.sh" "$I/f1/c1/${CFG}_$CAND" $CFG $CAND tx "$OUT/c1nat/gi/${CFG}_${CAND}_tx" "$I/f1/rt-$CFG/kz-rt-dump" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    echo "$(date -u +%T) C1 natural $CFG $CAND F0 rx" >> "$LOG"
    "$K/probe/scripts/observe_c1_hold.sh" $CFG $CAND rx "$KZ" --out-dir "$OUT/c1nat/f0rx" --tracefs "$TFS" --no-hold >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    echo "$(date -u +%T) C1 natural $CFG $CAND G-I rx" >> "$LOG"
    "$K/instrument/gcc/scripts/run_gi_c1.sh" "$I/f1/c1/${CFG}_$CAND" $CFG $CAND rx "$OUT/c1nat/gi/${CFG}_${CAND}_rx" "$I/f1/rt-$CFG/kz-rt-dump" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
  done; done
fi

if want c1ctl; then
  quiet "C1 controlled"
  for CFG in post pre; do
    if [ $CFG = post ]; then REV=833fdbb; TAG=B; LABEL="P-C1HOLD"; else REV=281a941; TAG=C; LABEL="researcher intervention, outside P-C1HOLD"; fi
    PLAN="$K/profiles/c1-bp-b051eb0/hold/hold-cf$REV.kzh"
    for CAND in 1df262a 1b0b338; do for INST in tx rx; do
      floor
      echo "$(date -u +%T) C1 controlled $TAG $CFG $CAND $INST F0 plan=$PLAN label=$LABEL" >> "$LOG"
      "$K/probe/scripts/observe_c1_hold.sh" $CFG $CAND $INST "$KZ" --out-dir "$OUT/c1ctl/f0" --tracefs "$TFS" --plan "$PLAN" --label "$LABEL" --tag $TAG >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
      { grep -m1 -E 'Starting the cFE with' "$OUT/c1ctl/f0/${TAG}_${CFG}_${CAND}_${INST}/inst/$INST/console_output.log"; grep -m1 'Exiting cFE with POWERON' "$OUT/c1ctl/f0/${TAG}_${CFG}_${CAND}_${INST}/inst/$INST/console_output.log"; ipcs -m | awk -v u=$U '$3==u'; } >> "$LOG" 2>&1
      echo "$(date -u +%T) C1 controlled $TAG $CFG $CAND $INST G-I-H" >> "$LOG"
      "$K/instrument/gcc/scripts/run_gih_c1.sh" "$I/f1/c1/${CFG}_$CAND" $CFG $CAND $INST "$PLAN" "$LABEL" $TAG "$OUT/c1ctl/gih" "$KZ" "$I/f1/rt-$CFG/kz-rt-dump" >> "$LOG" 2>&1; echo "  rc=$?" >> "$LOG"
    done; done
  done
fi

if want r2; then
  quiet "R2"
  floor
  echo "$(date -u +%T) R2 F0" >> "$LOG"
  mkdir -p "$OUT/r2/f0"
  env -u KZ CFS_NORMAL_USER="$U" "$K/probe/scripts/observe_cfs_smoke.sh" w6_r2_f0 "$KZ" 0 --out-dir "$OUT/r2/f0" --purpose pilot --start poweron >> "$LOG" 2>&1
  echo "  rc=$?" >> "$LOG"
  floor
  echo "$(date -u +%T) R2 G-I derived core-cpu1 sha256=$(sha256sum $I/d4/r2/cFS/build-native_std/exe/cpu1/core-cpu1 | cut -c1-64)" >> "$LOG"
  CFS_NORMAL_USER="$U" "$K/instrument/gcc/scripts/run_gi_r2.sh" "$I/d4/r2" "$OUT/r2/gi" "$I/d4/rt-r2/kz-rt-dump" poweron >> "$LOG" 2>&1
  echo "  rc=$?" >> "$LOG"
fi
echo "# w6_20261029.sh end $(date -u +%FT%T.%3NZ)" >> "$LOG"

#!/usr/bin/env bash
# G1 pilot driver (design amendment v3): for batches of lengths build both arms, verify
# correctness, measure randomized paired blocks, delete the artifacts (disk allowance).
# Batches are processed one after another; nothing else may run during a measurement.
# DEV-CONTAINER TIMINGS ARE NOT RESULTS.
#
#   SHAPEPERF_WORK=... scripts/run_g1_pilot.sh <out_dir> "<lengths of batch 1>" "<lengths of batch 2>" ...
set -euo pipefail
cd "$(dirname "$0")/.."
WORK="${SHAPEPERF_WORK:-$HOME/shapeperf-work}"
PY="${WORK}/venv/bin/python"
OUT="$1"; shift
PAIRS="${OUT}/pairs"
WARMUP="${G1_WARMUP:-2}"; ITER="${G1_ITERATIONS:-16}"; PROCS="${G1_PROCESSES:-2}"; BLOCKS="${G1_BLOCKS:-5}"
SEED="${G1_SEED:-20260929}"; CPU="${G1_CPU:-3}"
mkdir -p "${OUT}"
n=0
for batch in "$@"; do
  n=$((n+1))
  echo "== batch ${n}: ${batch} ($(date -u +%H:%M:%S))"
  "${PY}" scripts/build_shape_pair.py --lengths ${batch} --out "${PAIRS}" --jobs 3
  "${PY}" scripts/run_paired_benchmark.py verify --pairs "${PAIRS}" --lengths ${batch} --out "${OUT}/verify"
  "${PY}" scripts/run_paired_benchmark.py measure --pairs "${PAIRS}" --lengths ${batch} --out "${OUT}/timing" \
      --warmup "${WARMUP}" --iterations "${ITER}" --processes "${PROCS}" --blocks "${BLOCKS}" \
      --seed $((SEED+n)) --cpu "${CPU}"
  "${PY}" scripts/run_paired_benchmark.py cleanup --pairs "${PAIRS}" --lengths ${batch}
  df -h "${WORK}" | tail -1
done
echo "== pilot finished ($(date -u +%H:%M:%S))"

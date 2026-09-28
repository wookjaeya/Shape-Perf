#!/usr/bin/env bash
# End-to-end FUNCTIONAL smoke run of G3 -> G2.5 -> G4 -> evaluation on real
# compiles, at toy scale, with an UNFROZEN preregistration. Every number it
# produces is labelled dev-smoke / unfrozen-preregistration and is NOT a result.
# Use it on a new machine (or the measurement VM) to check the whole chain
# before spending the real budget.
#
#   SHAPEPERF_WORK=... SHAPEPERF_TARGET_CPU=<cpu> scripts/dev_smoke_pipeline.sh [out_dir]
set -euo pipefail
cd "$(dirname "$0")/.."
WORK="${SHAPEPERF_WORK:-$HOME/shapeperf-work}"
PY="${WORK}/venv/bin/python"
CPU="${SHAPEPERF_TARGET_CPU:?set SHAPEPERF_TARGET_CPU}"
OUT="${1:-results/dev_smoke}"
LO="${SMOKE_LO:-41}"; HI="${SMOKE_HI:-44}"
mkdir -p "${OUT}"

# toy preregistration (never the real one)
PRE="${OUT}/prereg_smoke.json"
"${PY}" - "${PRE}" <<'PY'
import json, sys
p = json.load(open("configs/preregistration.json"))
p["_comment"] = "SMOKE ONLY - toy values, not a preregistration"
p["measurement"].update(warmup_iterations=1, timed_iterations=3, processes_per_shape=2, blocks=1,
                        process_statistic="mean", cpus=None)
p["correctness"]["logit_abs_tolerance"] = None
p["events"].update(delta_min_effect=0.05, alpha=0.05, confidence_level=0.95, match_tolerance_lengths=0)
p["selectors"].update(random_seeds=[1, 2], shape_only_alignment_units=[8], report_overhead_ns=0,
                      compile_probe_budget_fraction=0.3, budgets_ns=[5e11, 5e12])
json.dump(p, open(sys.argv[1], "w"), indent=1)
PY
export SHAPEPERF_PREREG="${PWD}/${PRE}" SHAPEPERF_TARGET_CPU="${CPU}"

echo "== G3 pilot (toy)"
"${PY}" scripts/pilot_g3.py --seed 1 --n-lengths 1 --adjacent-pairs 1 --processes 2 --iterations 6 \
    --blocks 1 --report-overhead-lengths 1 --phase dev-smoke --out "${OUT}/pilot"

echo "== G2.5 census (toy range ${LO}-${HI})"
"${PY}" scripts/run_census.py --lengths "${LO}-${HI}" --units 8 16 32 --keep-raw-ir none --census-root "${OUT}/census"

echo "== G4 discovery + confirmation (toy range)"
"${PY}" scripts/groundtruth_dense.py --role discovery --lengths "${LO}-${HI}" --seed 11 \
    --vm-allocation-id dev-a --allow-unfrozen --out "${OUT}/g4_disc"
"${PY}" scripts/groundtruth_dense.py --role confirmation --lengths "${LO}-${HI}" --seed 12 \
    --vm-allocation-id dev-b --allow-unfrozen --reuse-compile "${OUT}/g4_disc/compile.jsonl" --out "${OUT}/g4_conf"

echo "== evaluation"
"${PY}" evaluate.py answer-table --discovery "${OUT}/g4_disc/measurements.jsonl" \
    --confirmation "${OUT}/g4_conf/measurements.jsonl" --manifest "${OUT}/g4_disc/manifest.json" \
    --allow-unfrozen --out "${OUT}/answer_table.json"
CT="${OUT}/census/A_reexport/default/${CPU}/census_table.json"
"${PY}" evaluate.py census --census-table "${CT}" --answer-table "${OUT}/answer_table.json" \
    --allow-unfrozen --out "${OUT}/census_metrics.json"
"${PY}" evaluate.py compare --compile "${OUT}/g4_disc/compile.jsonl" --measurements "${OUT}/g4_disc/measurements.jsonl" \
    --census "${OUT}/census/A_reexport/default/${CPU}/census.jsonl" --answer-table "${OUT}/answer_table.json" \
    --allow-unfrozen --out "${OUT}/policy_comparison.json"
echo "smoke pipeline finished: ${OUT} (NOT results)"

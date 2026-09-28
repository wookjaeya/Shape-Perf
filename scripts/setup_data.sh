#!/usr/bin/env bash
# Data side of G1/G2 (no compiler needed): fetch pinned artifacts, build the
# official features and shape catalog, check the official artifact's shape
# support, re-export with a variable sequence axis, validate the re-export.
set -euo pipefail
cd "$(dirname "$0")/.."
WORK="${SHAPEPERF_WORK:-$HOME/shapeperf-work}"
PY="${WORK}/venv/bin/python"
PYX="${WORK}/venv-export/bin/python"

if [[ ! -x "${PYX}" ]]; then
  python3 -m venv "${WORK}/venv-export"
  "${PYX}" -m pip install -q --upgrade pip
  "${PYX}" -m pip install -q -r env/requirements-export.in
  "${PYX}" -m pip freeze > "${WORK}/requirements-export.freeze.txt"
fi

"${PY}" scripts/fetch_artifacts.py
"${PY}" prepare_features.py
"${PY}" scripts/g2_check_original_shape.py
"${PYX}" scripts/g2_reexport_tf.py
"${PY}" scripts/g2_validate_reexport.py
echo "next: scripts/run_g1_evals.sh (full-dev-set EM/F1, long)"

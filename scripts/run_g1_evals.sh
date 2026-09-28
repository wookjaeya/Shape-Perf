#!/usr/bin/env bash
# G1/G2 task-level accuracy runs (spec §7.1, §7.4), sequential, reference runtime only.
set -uo pipefail
cd "$(dirname "$0")/.."
PY="${PY:-${SHAPEPERF_WORK:-$HOME/shapeperf-work}/venv/bin/python}"
RE=data/artifacts/derived/bertsquad-12-reexport-dynseq.onnx
"$PY" scripts/g1_reference_eval.py --length-policy official
"$PY" scripts/g1_reference_eval.py --model "$RE" --length-policy natural
"$PY" scripts/g1_reference_eval.py --model "$RE" --length-policy official

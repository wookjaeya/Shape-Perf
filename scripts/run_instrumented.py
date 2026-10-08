#!/usr/bin/env python3
"""Run an instrumented model .so (built with --profile-ir-with-sig=Onnx) in this process; DIAGNOSTIC only.

Instrumentation goes to ONNX_MLIR_INSTRUMENT_FILE and stdout (redirect stdout to a file). Summarize with
<onnx-mlir>/utils/make-report.py -r <log> -w <warmup> -s perf. Instrumented latencies are never results.

  ONNX_MLIR_INSTRUMENT_FILE=<log> python scripts/run_instrumented.py <model.so> <L> <warmup> <iterations> > <stdout file>
"""
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import toolchain  # noqa: E402
from shapeperf.compile import model_def  # noqa: E402
from shapeperf.inputs import inputs_for  # noqa: E402
from shapeperf.squad import load_features  # noqa: E402
from shapeperf.util import REPO_ROOT, read_json  # noqa: E402
so, L, warm, it = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
os.sched_setaffinity(0, {3})
m = model_def("A_reexport")
feat = load_features(REPO_ROOT / m["features"] / "features.npz")
fi = read_json(REPO_ROOT / m["features"] / "catalog.json")["anchor"]["feature_index"]
arrs, _ = inputs_for(m, feat, fi, L)
S = toolchain.import_pyruntime()(shared_lib_path=so)
for _ in range(warm + it):
    S.run(arrs)

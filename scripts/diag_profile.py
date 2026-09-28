#!/usr/bin/env python3
"""Diagnostic per-op profile of one length (spec §6.1 third row, R1 workflow).

Builds a SEPARATE instrumented executable (flag set diag_profile:
--profile-ir-with-sig=Onnx), runs it in a fresh process with
ONNX_MLIR_INSTRUMENT_FILE set, and summarizes the log with the pinned
utils/make-report.py (--stats=sig -l 2). Output is diagnostic evidence for
G7 only; its latencies are instrumented and are never main results.

  python scripts/diag_profile.py --length 129 --warmup W --iterations N --out results/diag/s129
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import toolchain  # noqa: E402
from shapeperf.compile import compile_shape  # noqa: E402
from shapeperf.measure import run_item, worker_env  # noqa: E402
from shapeperf.util import REPO_ROOT, read_json, write_json  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--length", type=int, required=True)
    ap.add_argument("--target-cpu", default=os.environ.get("SHAPEPERF_TARGET_CPU"))
    ap.add_argument("--allow-native", action="store_true")
    ap.add_argument("--warmup", type=int, required=True)
    ap.add_argument("--iterations", type=int, required=True)
    ap.add_argument("--feature", default="anchor")
    ap.add_argument("--focus", default=None, help="make-report -f regexp, e.g. onnx.MatMul")
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    out = Path(a.out)
    c = compile_shape(a.model, a.length, "diag_profile", a.target_cpu, "full", out / "build",
                      allow_native=a.allow_native)
    if c.get("failure_type"):
        sys.exit(f"compile failed: {c['failure_type']}")
    from shapeperf.compile import model_def
    fi = (read_json(REPO_ROOT / model_def(a.model)["features"] / "catalog.json")["anchor"]["feature_index"]
          if a.feature == "anchor" else int(a.feature))
    log = out / "runtime.log"
    env = worker_env(1, {"ONNX_MLIR_INSTRUMENT_FILE": str(log)})
    r = run_item({"artifact": c["artifact_path"], "model_key": a.model, "length": a.length, "feature_index": fi,
                  "warmup": a.warmup, "iterations": a.iterations, "cpus": None}, env)
    cmd = [sys.executable, str(toolchain.work_dir() / "onnx-mlir/utils/make-report.py"), "-r", str(log),
           "-w", str(a.warmup), "--stats=sig", "-l", "2"] + (["-f", a.focus] if a.focus else [])
    rep = subprocess.run(cmd, capture_output=True, text=True)
    (out / "make_report.txt").write_text(rep.stdout + rep.stderr)
    write_json(out / "diag.json", {"label": "DIAGNOSTIC (instrumented) - not a main performance number",
                                   "compile": {k: c.get(k) for k in ["compile_flags", "artifact_hash"]},
                                   "run_failure": r.get("failure_type"), "make_report_cmd": cmd})
    print(rep.stdout[-3000:])


if __name__ == "__main__":
    main()

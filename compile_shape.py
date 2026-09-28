#!/usr/bin/env python3
"""CLI: compile one length with the pinned ONNX-MLIR (spec §12 compile_shape.py).

Example (dev smoke test; measurement VMs must pass their real CPU name):
  python compile_shape.py --model A_reexport --length 128 --flagset default \
      --target-cpu sapphirerapids --mode full --out results/compile/smoke/s128
"""
import argparse
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shapeperf.compile import compile_shape, final_signature  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--length", type=int, required=True)
    ap.add_argument("--batch", type=int, default=1)
    ap.add_argument("--flagset", default="default")
    ap.add_argument("--target-cpu", default=os.environ.get("SHAPEPERF_TARGET_CPU"))
    ap.add_argument("--allow-native", action="store_true")
    ap.add_argument("--mode", choices=["full", "probe"], default="full")
    ap.add_argument("--out", required=True)
    ap.add_argument("--timeout", type=float, default=None)
    ap.add_argument("--final-signature", action="store_true",
                    help="also disassemble the .so (G3 probe/final mismatch check)")
    args = ap.parse_args()
    rec = compile_shape(args.model, args.length, args.flagset, args.target_cpu, args.mode,
                        args.out, batch=args.batch, allow_native=args.allow_native,
                        timeout_s=args.timeout)
    if args.final_signature and not rec.get("failure_type"):
        fsig = final_signature(rec)
        rec["final_signature"] = fsig["hash"] if fsig and fsig.get("available") else None
        Path(args.out, "final_signature.json").write_text(json.dumps(fsig, indent=1))
    keys = ["padded_length", "mode", "failure_type", "compile_wall_ns", "probe_wall_ns",
            "peak_rss_bytes", "ir_signature", "ir_structure_signature", "matmul_path",
            "artifact_hash", "final_signature"]
    print(json.dumps({k: rec.get(k) for k in keys}, indent=1))
    sys.exit(1 if rec.get("failure_type") else 0)


if __name__ == "__main__":
    main()

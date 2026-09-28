#!/usr/bin/env python3
"""CLI: uninstrumented latency measurement (spec §12 measure.py).

Warmup and iteration counts have no defaults on purpose (spec §2, §10.2).

Single artifact:
  python measure.py --artifact results/compile/x/s128/model.so --model A_reexport \
      --length 128 --feature anchor --warmup W --iterations N --processes P \
      --cpus 2 --out results/raw/measurements.jsonl --seed 1

Plan file (list of items with artifact/model_key/length/feature_index/warmup/iterations):
  python measure.py --plan plan.json --out ... --seed 1
Run this only on a machine that satisfies the §5 measurement controls; elsewhere
the output is a functional smoke test (experiment_phase=dev-smoke).
"""
import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shapeperf.measure import run_block  # noqa: E402
from shapeperf.util import REPO_ROOT, read_json, sha256_file  # noqa: E402


def anchor_index(model_key):
    from shapeperf.compile import model_def
    m = model_def(model_key)
    cat = read_json(REPO_ROOT / m["features"] / "catalog.json")
    return cat["anchor"]["feature_index"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--plan")
    ap.add_argument("--artifact")
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--length", type=int)
    ap.add_argument("--feature", default="anchor", help="'anchor' or a feature index")
    ap.add_argument("--warmup", type=int)
    ap.add_argument("--iterations", type=int)
    ap.add_argument("--processes", type=int, default=1, help="fresh processes for the same item")
    ap.add_argument("--cpus", type=int, nargs="*", default=None)
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--seed", type=int, required=True)
    ap.add_argument("--vm-allocation-id", default="unavailable")
    ap.add_argument("--phase", default="dev-smoke")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    if args.plan:
        items = read_json(args.plan)
    else:
        if None in (args.artifact, args.length, args.warmup, args.iterations):
            ap.error("--artifact, --length, --warmup and --iterations are required without --plan")
        fi = anchor_index(args.model) if args.feature == "anchor" else int(args.feature)
        item = {"artifact": str(Path(args.artifact).resolve()), "artifact_hash": sha256_file(args.artifact),
                "model_key": args.model, "length": args.length, "feature_index": fi,
                "warmup": args.warmup, "iterations": args.iterations, "cpus": args.cpus}
        items = [dict(item) for _ in range(args.processes)]
    recs = run_block(items, args.out, args.seed, vm_allocation_id=args.vm_allocation_id,
                     experiment_phase=args.phase, threads=args.threads)
    for r in recs:
        if r["failure_type"]:
            print("FAILED", r["padded_length"], r.get("stderr_tail", "")[-500:])
            continue
        lat = np.array(r["latency_ns"]) / 1e6
        print(f'L={r["padded_length"]} order={r["order_in_block"]} n={lat.size} '
              f'median={np.median(lat):.3f}ms threads={r["threads_observed"]} '
              f'static={r["static_shape_verified"]} stable={r["outputs_stable"]}')


if __name__ == "__main__":
    main()

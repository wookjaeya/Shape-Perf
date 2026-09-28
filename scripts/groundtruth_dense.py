#!/usr/bin/env python3
"""G4 dense evaluator measurement - EVALUATOR ONLY (spec §9.3 answer table A).

Two roles, run as separate invocations (ideally on different VM allocations):
  --role discovery     dense data from which candidates are formed
  --role confirmation  independent run (different seed/order/allocation)

Step 1 compiles every valid length once (full mode) and records the real
compile / extraction / correctness-check cost of each query - these records
are also what the G5 ReplayBackend charges. Step 2 runs `blocks` blocks; each
block contains every successfully compiled and verified length x processes in
a seeded random order (spec §10.1).

Measurement counts come from the frozen preregistration; --allow-unfrozen is
for development smoke runs only and labels the output accordingly.
"""
import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import prereg  # noqa: E402
from shapeperf.compile import compile_shape, model_def  # noqa: E402
from shapeperf.measure import run_block  # noqa: E402
from shapeperf.util import REPO_ROOT, append_jsonl, git_head, new_run_id, read_json, read_jsonl, write_json  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--flagsets", nargs="+", default=["default"],
                    help="flag sets measured in the SAME blocks (default first; alternatives give R(s)/Q(s), spec §9.1)")
    ap.add_argument("--target-cpu", default=os.environ.get("SHAPEPERF_TARGET_CPU"))
    ap.add_argument("--role", choices=["discovery", "confirmation"], required=True)
    ap.add_argument("--lengths", default=None, help="default: [L(anchor), max valid]")
    ap.add_argument("--seed", type=int, required=True)
    ap.add_argument("--vm-allocation-id", required=True)
    ap.add_argument("--reuse-compile", default=None, help="compile.jsonl of an earlier run on the same VM image")
    ap.add_argument("--allow-unfrozen", action="store_true")
    ap.add_argument("--allow-native", action="store_true")
    ap.add_argument("--out", default=None)
    args = ap.parse_args()

    p, label = prereg.require(["measurement.warmup_iterations", "measurement.timed_iterations",
                               "measurement.processes_per_shape", "measurement.blocks",
                               "correctness.logit_abs_tolerance"], allow_unfrozen=args.allow_unfrozen)
    meas = p["measurement"]
    m = model_def(args.model)
    cat = read_json(REPO_ROOT / m["features"] / "catalog.json")
    anchor = cat["anchor"]["feature_index"]
    if args.lengths:
        a, b = (int(x) for x in args.lengths.split("-"))
    else:
        a, b = max(m["valid_lengths"][0], cat["anchor"]["valid_length"]), m["valid_lengths"][1]
    lengths = list(range(a, b + 1))
    run_id = new_run_id(f"g4-{args.role}")
    out = Path(args.out or REPO_ROOT / "results/g4" / run_id)
    out.mkdir(parents=True, exist_ok=True)
    write_json(out / "manifest.json", {"run_id": run_id, "role": args.role, "lengths": [a, b],
                                       "seed": args.seed, "vm_allocation_id": args.vm_allocation_id,
                                       "flagsets": args.flagsets,
                                       "preregistration": label, "harness_commit": git_head(),
                                       "args": vars(args), "evaluator_only": True})

    comp_path = out / "compile.jsonl"
    compiled = {}
    if args.reuse_compile:
        compiled = {(r.get("flagset", "default"), r["padded_length"]): r for r in read_jsonl(args.reuse_compile)}
    for fs in args.flagsets:
        for s in lengths:
            if (fs, s) in compiled:
                continue
            rec = compile_shape(args.model, s, fs, args.target_cpu, "full", out / "artifacts" / fs / f"s{s:04d}",
                                allow_native=args.allow_native)
            rec["flagset"] = fs
            if not rec.get("failure_type"):
                t0 = time.monotonic_ns()
                v = subprocess.run([sys.executable, str(REPO_ROOT / "validate_shapes.py"), "--artifact",
                                    rec["artifact_path"], "--model", args.model, "--length", str(s),
                                    "--out", str(out / "validation.jsonl")], capture_output=True, text=True)
                rec["verify_wall_ns"] = time.monotonic_ns() - t0
                st = (json.loads(v.stdout.strip().splitlines()[-1])["correctness_status"]
                      if v.returncode == 0 else "error")
                rec["correctness_status"] = st
                if st != "pass" and not args.allow_unfrozen:
                    rec["failure_type"] = f"correctness:{st}"
            append_jsonl(comp_path, rec)
            compiled[(fs, s)] = rec
            print("compiled", fs, s, rec.get("failure_type"), rec.get("correctness_status"), flush=True)

    raw = out / "measurements.jsonl"
    for blk in range(meas["blocks"] or 1):
        items = [{"artifact": compiled[(fs, s)]["artifact_path"], "artifact_hash": compiled[(fs, s)]["artifact_hash"],
                  "model_key": args.model, "length": s, "feature_index": anchor, "flagset": fs,
                  "warmup": meas["warmup_iterations"] or 0, "iterations": meas["timed_iterations"] or 1,
                  "cpus": meas["cpus"]}
                 for fs in args.flagsets for s in lengths if not compiled[(fs, s)].get("failure_type")
                 for _ in range(meas["processes_per_shape"] or 1)]
        run_block(items, raw, seed=args.seed * 1000 + blk, vm_allocation_id=args.vm_allocation_id,
                  experiment_phase=f"g4-{args.role} {label}", threads=meas["threads"])
        print("block done", blk, flush=True)
    print("output:", out)


if __name__ == "__main__":
    main()

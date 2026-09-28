#!/usr/bin/env python3
"""G3 measurement pilot (spec §10.2, §11 G3). Produces the data from which the
preregistered numbers are chosen; it chooses none of them itself.

Lengths are drawn with a recorded seed *before* any timing is seen (plus the
endpoints and a few adjacent pairs for the probe/final mismatch check).

Per length:
  * full compile (+ final-artifact disassembly signature) and probe compile:
    compile/probe wall, CPU, peak RSS -> probe : measurement cost ratio (H3)
  * correctness vs ONNX Runtime (validate_shapes.py)
  * P fresh processes x N iterations with *no* warmup, so the warmup trend is
    visible in the raw per-iteration data
Analysis: warmup trend, iteration/process/block variance components, and a
Kalibera-Jones style repetition suggestion (R14) given the measured costs.

  python scripts/pilot_g3.py --target-cpu <cpu> --seed 1 --n-lengths 6 \
      --adjacent-pairs 2 --processes 5 --iterations 200 --blocks 2 --cpus 2
"""
import argparse
import json
import math
import os
import subprocess
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf.compile import compile_shape, final_signature, model_def  # noqa: E402
from shapeperf.measure import run_block  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, new_run_id, read_json, read_jsonl, write_json  # noqa: E402


def choose_lengths(valid, n, pairs, seed):
    rng = np.random.default_rng(seed)
    lo, hi = valid[0], valid[-1]
    pick = set(int(x) for x in rng.choice(valid[1:-1], size=min(n, len(valid) - 2), replace=False))
    adj = []
    for s in rng.choice(valid[:-1], size=pairs, replace=False):
        adj.append((int(s), int(s) + 1))
        pick |= {int(s), int(s) + 1}
    return sorted(pick | {lo, hi}), adj


def variance_components(recs, skip):
    """Iteration (within process) and process (within block) variance of
    latency after dropping the first `skip` iterations; block variance if >1."""
    by_block = {}
    within, proc_means = [], []
    for r in recs:
        x = np.asarray(r["latency_ns"][skip:], float)
        if x.size < 2:
            continue
        within.append(x.var(ddof=1))
        by_block.setdefault(r["block_id"], []).append(x.mean())
        proc_means.append(x.mean())
    out = {"iteration_var": float(np.mean(within)) if within else None,
           "process_var": float(np.var(proc_means, ddof=1)) if len(proc_means) > 1 else None}
    bm = [np.mean(v) for v in by_block.values()]
    out["block_var"] = float(np.var(bm, ddof=1)) if len(bm) > 1 else None
    return out


def kj_iterations(var_iter, var_proc, n_iter, cost_iter_ns, cost_proc_ns):
    """Kalibera & Jones (2013) optimal repetitions at the lower level:
    r = ceil(sqrt(c_proc/c_iter * S2_iter/T2_proc)) with the process-level
    component T2 = S2_proc - S2_iter/n (biased estimator correction)."""
    if None in (var_iter, var_proc) or cost_iter_ns <= 0:
        return None
    t2 = var_proc - var_iter / n_iter
    if t2 <= 0:
        return {"note": "process-level component not detectable at this n (T2<=0)", "T2": t2}
    return {"iterations_per_process": math.ceil(math.sqrt(cost_proc_ns / cost_iter_ns * var_iter / t2)),
            "T2_process_component": t2}


def warmup_profile(recs):
    """Median over processes of each iteration index (ns) - raw trend, no cut-off chosen."""
    n = min(len(r["latency_ns"]) for r in recs)
    m = np.median(np.array([r["latency_ns"][:n] for r in recs], float), axis=0)
    return m.tolist()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--flagset", default="default")
    ap.add_argument("--target-cpu", default=os.environ.get("SHAPEPERF_TARGET_CPU"))
    ap.add_argument("--allow-native", action="store_true")
    ap.add_argument("--seed", type=int, required=True)
    ap.add_argument("--n-lengths", type=int, required=True)
    ap.add_argument("--adjacent-pairs", type=int, required=True)
    ap.add_argument("--processes", type=int, required=True)
    ap.add_argument("--iterations", type=int, required=True)
    ap.add_argument("--blocks", type=int, required=True)
    ap.add_argument("--cpus", type=int, nargs="*", default=None)
    ap.add_argument("--vm-allocation-id", default="unavailable")
    ap.add_argument("--phase", default="pilot")
    ap.add_argument("--analysis-skip", type=int, default=0,
                    help="iterations dropped ONLY for the variance/KJ analysis (recorded; not a warmup decision)")
    ap.add_argument("--out", default=None)
    args = ap.parse_args()

    m = model_def(args.model)
    cat = read_json(REPO_ROOT / m["features"] / "catalog.json")
    anchor = cat["anchor"]["feature_index"]
    lo = max(m["valid_lengths"][0], cat["anchor"]["valid_length"])
    valid = list(range(lo, m["valid_lengths"][1] + 1))
    lengths, adj = choose_lengths(valid, args.n_lengths, args.adjacent_pairs, args.seed)
    run_id = new_run_id("pilot")
    out = Path(args.out or REPO_ROOT / "results/g3" / run_id)
    out.mkdir(parents=True, exist_ok=True)
    plan = {"run_id": run_id, "seed": args.seed, "lengths": lengths, "adjacent_pairs": adj,
            "args": vars(args), "harness_commit": git_head(), "anchor_feature": anchor}
    write_json(out / "plan.json", plan)

    comp = {}
    for s in lengths:
        full = compile_shape(args.model, s, args.flagset, args.target_cpu, "full", out / f"full_s{s}",
                             allow_native=args.allow_native)
        if not full.get("failure_type"):
            fs = final_signature(full)
            full["final_signature"] = fs.get("hash") if fs and fs.get("available") else None
            (out / f"full_s{s}" / "final_signature.json").write_text(json.dumps(fs, indent=1))
        probe = compile_shape(args.model, s, args.flagset, args.target_cpu, "probe", out / f"probe_s{s}",
                              allow_native=args.allow_native)
        comp[s] = {"full": full, "probe": probe}
        v = None
        if not full.get("failure_type"):
            p = subprocess.run([sys.executable, str(REPO_ROOT / "validate_shapes.py"), "--artifact",
                                full["artifact_path"], "--model", args.model, "--length", str(s),
                                "--out", str(out / "validation.jsonl")], capture_output=True, text=True)
            v = json.loads(p.stdout.strip().splitlines()[-1]) if p.returncode == 0 else {"error": p.stderr[-2000:]}
        comp[s]["validation"] = v
        print("compiled", s, full.get("failure_type"), probe.get("failure_type"), flush=True)

    raw = out / "measurements.jsonl"
    for b in range(args.blocks):
        items = [{"artifact": comp[s]["full"]["artifact_path"], "artifact_hash": comp[s]["full"]["artifact_hash"],
                  "model_key": args.model, "length": s, "feature_index": anchor, "warmup": 0,
                  "iterations": args.iterations, "cpus": args.cpus}
                 for s in lengths if not comp[s]["full"].get("failure_type") for _ in range(args.processes)]
        run_block(items, raw, seed=args.seed * 1000 + b, vm_allocation_id=args.vm_allocation_id,
                  experiment_phase=args.phase)

    recs = [r for r in read_jsonl(raw) if not r.get("failure_type")]
    report = {"plan": plan, "per_length": {}, "failures": {}}
    for s in lengths:
        c = comp[s]
        rs = [r for r in recs if r["padded_length"] == s]
        row = {"compile_wall_ns": c["full"].get("compile_wall_ns"), "probe_wall_ns": c["probe"].get("probe_wall_ns"),
               "compile_peak_rss": c["full"].get("peak_rss_bytes"), "probe_peak_rss": c["probe"].get("peak_rss_bytes"),
               "feature_extract_wall_ns": c["full"].get("feature_extract_wall_ns"),
               "report_signature_full": c["full"].get("ir_signature"),
               "report_signature_probe": c["probe"].get("ir_signature"),
               "final_signature": c["full"].get("final_signature"),
               "matmul_path": c["full"].get("matmul_path"),
               "validation": c["validation"]}
        if rs:
            per_iter = np.mean([np.mean(r["latency_ns"]) for r in rs])
            row.update(warmup_profile_ns=warmup_profile(rs),
                       variance_all_iterations=variance_components(rs, 0),
                       measurement_wall_ns_per_process=float(np.mean([r["measurement_wall_ns"] for r in rs])),
                       process_wall_ns=float(np.mean([r["process_wall_ns"] for r in rs])),
                       mean_iteration_ns=float(per_iter),
                       threads_observed=sorted({str(r["threads_observed"]) for r in rs}),
                       static_shape_verified=sorted({str(r["static_shape_verified"]) for r in rs}),
                       outputs_stable=all(r["outputs_stable"] for r in rs))
            vc = variance_components(rs, args.analysis_skip)
            row["variance_after_skip"] = {"skip": args.analysis_skip, **vc}
            n_it = args.iterations - args.analysis_skip
            row["kalibera_jones"] = kj_iterations(
                vc["iteration_var"], vc["process_var"], n_it, per_iter,
                row["process_wall_ns"] - row["measurement_wall_ns_per_process"])
            full_cost = (row["compile_wall_ns"] or 0) + row["process_wall_ns"]
            row["probe_to_measured_query_cost_ratio"] = (row["probe_wall_ns"] / full_cost
                                                         if row["probe_wall_ns"] and full_cost else None)
        report["per_length"][s] = row
    mism = []
    for a, b in adj:
        pa, pb = comp[a]["probe"].get("ir_signature"), comp[b]["probe"].get("ir_signature")
        fa, fb = comp[a]["full"].get("final_signature"), comp[b]["full"].get("final_signature")
        mism.append({"pair": [a, b], "probe_changed": pa != pb, "final_changed": fa != fb,
                     "mismatch": (pa != pb) != (fa != fb)})
    report["probe_vs_final_changepoint_mismatch"] = {
        "pairs": mism, "rate": float(np.mean([x["mismatch"] for x in mism])) if mism else None,
        "note": "tiny sample; the census-scale rate needs full compiles of adjacent lengths"}
    report["probe_vs_full_report_signature_equal"] = {
        s: comp[s]["probe"].get("ir_signature") == comp[s]["full"].get("ir_signature") for s in lengths}
    write_json(out / "pilot_report.json", report)
    print("pilot report:", out / "pilot_report.json")
    print("NOTE: choose warmup/iterations/processes from these data and write them into "
          "configs/preregistration.json + preregistration.md; this script decides nothing.")


if __name__ == "__main__":
    main()

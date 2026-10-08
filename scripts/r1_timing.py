#!/usr/bin/env python3
"""R1 (E-R1-3): timing of the math-lowering interventions. DEVELOPMENT CONTAINER - NOT A RESULT.

Every measured process loads ONE model (fresh exec, CPU 3, 1 thread); arms are visited in a seeded random
order inside each block; the per-block log ratio arm/orig is summarized over blocks (mean, 95% t-interval).
Interpreted only if |effect| >= 10% and the interval excludes 0 (results/r1/protocol_r1.json).

  single op : batch-of-calls on the math_scan artifacts (e.g. Gelu_tanh), `calls` calls per sample
  model     : BERT (A_reexport) at L=256 on the anchor input via shapeperf.measure (warmup + iterations)

  python scripts/r1_timing.py ops   --ops Gelu_tanh Tanh Exp Sigmoid --blocks 5 --out results/r1/timing
  python scripts/r1_timing.py model --blocks 5 --out results/r1/timing
"""
import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf.util import REPO_ROOT, append_jsonl, sha256_file, write_json  # noqa: E402

ARMS = ["orig", "r1a", "r1b", "r1c"]
SCAN = Path("/home/user/work/r1/math_scan")
BERT = Path("/home/user/work/r1/bert/L0256")
ENV = dict(os.environ, OMP_NUM_THREADS="1", MKL_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")


def op_worker(spec):
    from shapeperf import toolchain
    from shapeperf.identity import open_session
    os.sched_setaffinity(0, {spec["cpu"]})
    x = np.load(spec["input"])
    sess = open_session(toolchain.import_pyruntime(), spec["so"], spec["tag"])
    for _ in range(spec["warmup_calls"]):
        out = sess.run([x])
    samples = []
    perf = time.perf_counter_ns
    for _ in range(spec["reps"]):
        t0 = perf()
        for _ in range(spec["calls"]):
            out = sess.run([x])
        samples.append((perf() - t0) / spec["calls"])
    del out
    return {"latency_ns": samples, "pid": os.getpid()}


def summarize(records, key_arm="arm"):
    from scipy import stats
    by = {}
    for r in records:
        by.setdefault(r["block"], {})[r[key_arm]] = float(np.mean(r["latency_ns"]))
    out = {}
    for arm in ARMS:
        d = [np.log(b[arm]) - np.log(b["orig"]) for b in by.values() if arm in b and "orig" in b]
        if arm == "orig" or len(d) < 2:
            continue
        d = np.array(d)
        m, se = d.mean(), d.std(ddof=1) / np.sqrt(d.size)
        t = stats.t.ppf(0.975, d.size - 1)
        out[arm] = {"n_blocks": int(d.size), "relative_effect_pct": float(np.expm1(m) * 100),
                    "ci95_pct": [float(np.expm1(m - t * se) * 100), float(np.expm1(m + t * se) * 100)],
                    "orig_mean_us": float(np.mean([b["orig"] for b in by.values()]) / 1e3),
                    "arm_mean_us": float(np.mean([b[arm] for b in by.values() if arm in b]) / 1e3)}
    return out


def run_ops(a):
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    raw = out / "ops_raw.jsonl"
    rng = np.random.default_rng(a.seed)
    res = {}
    for op in a.ops:
        recs = []
        for b in range(a.blocks):
            for i in rng.permutation(len(ARMS)):
                arm = ARMS[i]
                d = SCAN / op / arm
                spec = {"so": str(d / "model.so"), "tag": f"{op.lower()}_{arm}", "input": str(SCAN / op / "input.npy"),
                        "cpu": a.cpu, "warmup_calls": a.warmup_calls, "reps": a.reps, "calls": a.calls}
                p = subprocess.run([sys.executable, str(Path(__file__).resolve()), "op-worker", json.dumps(spec)],
                                   capture_output=True, text=True, cwd=REPO_ROOT, env=ENV)
                if p.returncode != 0:
                    raise SystemExit(p.stderr[-1500:])
                w = json.loads(p.stdout.strip().splitlines()[-1])
                rec = {"op": op, "arm": arm, "block": b, "so_sha256": sha256_file(spec["so"]), **w,
                       "calls_per_sample": a.calls, "cpu": a.cpu, "note": "development container: NOT a result"}
                append_jsonl(raw, rec)
                recs.append(rec)
        res[op] = summarize(recs)
        print(op, json.dumps(res[op]), flush=True)
    write_json(out / "ops_summary.json", {"design": vars(a) | {"arms": ARMS}, "effects_vs_orig": res,
                                          "note": "development container: NOT a result"})


def run_model(a):
    from shapeperf.compile import model_def
    from shapeperf.measure import run_block
    from shapeperf.util import read_json
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    raw = out / "model_raw.jsonl"
    anchor = read_json(REPO_ROOT / model_def("A_reexport")["features"] / "catalog.json")["anchor"]["feature_index"]
    for b in range(a.blocks):
        items = [{"artifact": str(BERT / arm / "model.so"), "artifact_hash": sha256_file(BERT / arm / "model.so"),
                  "model_key": "A_reexport", "length": 256, "feature_index": anchor, "flagset": arm,
                  "warmup": a.warmup, "iterations": a.iterations, "cpus": [a.cpu]} for arm in ARMS]
        run_block(items, raw, seed=a.seed * 1000 + b, vm_allocation_id="dev-container", threads=1,
                  experiment_phase="R1 DEV-CONTAINER (not a result)", extra={"block_index": b})
    recs = []
    for ln in raw.read_text().splitlines():
        r = json.loads(ln)
        if not r.get("failure_type"):
            recs.append({"arm": r["flagset"], "block": r["extra"]["block_index"] if "extra" in r else r.get("block_index"),
                         "latency_ns": r["latency_ns"]})
    write_json(out / "model_summary.json", {"design": vars(a) | {"arms": ARMS, "length": 256},
                                            "effects_vs_orig": summarize(recs),
                                            "note": "development container: NOT a result"})
    print(json.dumps(summarize(recs), indent=1))


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "op-worker":
        print(json.dumps(op_worker(json.loads(sys.argv[2]))))
        return
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    o = sub.add_parser("ops")
    o.add_argument("--ops", nargs="+", default=["Gelu_tanh", "Tanh", "Exp", "Sigmoid"])
    o.add_argument("--blocks", type=int, default=5)
    o.add_argument("--calls", type=int, default=50)
    o.add_argument("--reps", type=int, default=10)
    o.add_argument("--warmup-calls", type=int, default=20)
    m = sub.add_parser("model")
    m.add_argument("--blocks", type=int, default=5)
    m.add_argument("--warmup", type=int, default=2)
    m.add_argument("--iterations", type=int, default=5)
    for s in (o, m):
        s.add_argument("--out", default=str(REPO_ROOT / "results/r1/timing"))
        s.add_argument("--cpu", type=int, default=3)
        s.add_argument("--seed", type=int, default=20260930)
    a = ap.parse_args()
    {"ops": run_ops, "model": run_model}[a.cmd](a)


if __name__ == "__main__":
    main()

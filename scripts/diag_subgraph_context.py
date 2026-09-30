#!/usr/bin/env python3
"""Does the kernel-level S8-vs-S1 difference depend on WHAT ELSE runs in the process? (design amendment v3, G2)

Times the K transpose of one length in fresh processes under different arm compositions and orders:
  alone       one arm per process (the fresh-process instrument of the sub-graph pilot)
  interleaved both arms in one process, rounds alternating in random order
  blocked     both arms in one process, all rounds of the first arm, then all rounds of the second
The same compiled artifacts are used everywhere. If an arm's time changes with the composition, the
difference is not a property of the arm's code alone.

FOLLOW-UP E1 (2026-09-30): the co-loaded variants of this script were INVALID S8-vs-S1 comparisons. The
v3 artifacts were compiled without --tag (all model.so), and the runtime opens them RTLD_GLOBAL: every
call of the later-loaded arm ran the FIRST loaded arm's compute code (gdb + LD_DEBUG, L=64,
results/v3_followup/e1/identity_orig). The "load order sets the level" observation was the level of
the code that actually ran. The pair worker now refuses such co-loading.

  python scripts/diag_subgraph_context.py --built <g2 dir> --length 64 --blocks 4 --out <json>
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf.util import REPO_ROOT, write_json  # noqa: E402

# name: (load order, order of rounds or None, timed arms or None = all loaded arms)
VARIANTS = {"S1_alone": (["S1"], None, None), "S8_alone": (["S8"], None, None),
            "interleaved": (["S1", "S8"], "random", None),
            "blocked_S1_then_S8": (["S1", "S8"], "blocked", None), "blocked_S8_then_S1": (["S8", "S1"], "blocked", None),
            # factorial: BOTH libraries loaded in every process, in the given order, only one arm timed
            "load_S1_S8__time_S1": (["S1", "S8"], None, ["S1"]), "load_S1_S8__time_S8": (["S1", "S8"], None, ["S8"]),
            "load_S8_S1__time_S1": (["S8", "S1"], None, ["S1"]), "load_S8_S1__time_S8": (["S8", "S1"], None, ["S8"])}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--built", required=True)
    ap.add_argument("--length", type=int, required=True)
    ap.add_argument("--blocks", type=int, default=4)
    ap.add_argument("--calls", type=int, default=500)
    ap.add_argument("--rounds", type=int, default=12)
    ap.add_argument("--cpu", type=int, default=3)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--variants", nargs="+", default=list(VARIANTS))
    ap.add_argument("--env", nargs="*", default=[], help="extra KEY=VALUE for the worker processes")
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    d = Path(a.built) / "K" / f"L{a.length:04d}"
    env = dict(os.environ, OMP_NUM_THREADS="1", MKL_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1",
               **dict(kv.split("=", 1) for kv in a.env))
    rng = np.random.default_rng(a.seed)
    names = list(a.variants)
    res = {v: {"S1": [], "S8": []} for v in names}
    for b in range(a.blocks):
        for i in rng.permutation(len(names)):          # variant order randomized within every block
            v = names[i]
            arms, order, timed = VARIANTS[v]
            spec = {"artifacts": {arm: str(d / f"{arm}_full" / "model.so") for arm in arms},
                    "input_npy": str(d / "input.npy"), "expected_npy": str(d / "expected.npy"),
                    "calls": a.calls, "rounds": a.rounds, "warmup_calls": 200, "cpus": [a.cpu],
                    "seed": a.seed * 1000 + b, "order": order, "timed": timed}
            p = subprocess.run([sys.executable, "-m", "shapeperf.paired", "--pair-worker", json.dumps(spec)],
                               cwd=REPO_ROOT, env=env, capture_output=True, text=True)
            out = json.loads(p.stdout.strip().splitlines()[-1])
            for arm, samples in out["samples"].items():
                res[v][arm].append(float(np.mean(samples)) / 1e3)         # us per call, process mean
    summary = {v: {arm: {"n": len(x), "mean_us": float(np.mean(x)), "sd_us": float(np.std(x, ddof=1)) if len(x) > 1 else None}
                   for arm, x in arms.items() if x} for v, arms in res.items()}
    write_json(a.out, {"length": a.length, "extra_env": a.env, "blocks": a.blocks, "calls": a.calls, "rounds": a.rounds, "summary": summary,
                       "raw_us": res, "note": "development container: not a result"})
    print(f"L={a.length}, {a.blocks} blocks; process-mean us per call (mean +- sd over blocks)")
    for v, arms in summary.items():
        print(f"  {v:20s}", "  ".join(f"{arm}: {s['mean_us']:6.2f} +- {s['sd_us']:.2f}" for arm, s in arms.items()))


if __name__ == "__main__":
    main()

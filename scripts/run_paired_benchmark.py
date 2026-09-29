#!/usr/bin/env python3
"""Correctness check and randomized paired timing of the S8/S1 arms (design amendment v3, G1).

  verify   run every arm on the controlled anchor input and the natural input of the length,
           each in a fresh process; S8 vs S1 must be BITWISE identical (the patch only
           reorders/unrolls a data movement); both are also compared with ORT (descriptive:
           tolerance is not preregistered, spec §7.3). Nothing is timed.
  measure  per length, B blocks; in every block the arms {S8, S1, AA} x P processes run in a
           seeded random order, each in a fresh process with warmup + timed iterations, pinned
           to one CPU. AA is a byte-identical COPY of the S8 artifact under another name: the
           A/A control for spurious differences of the measurement path.
  cleanup  delete the compiled artifacts of the lengths (their hashes stay in pair.json).

DEVELOPMENT-CONTAINER TIMINGS ARE NOT RESULTS (amendment §9.1): the container is not a
controlled measurement VM; the records are labelled accordingly.

  python scripts/run_paired_benchmark.py verify  --pairs <dir> --lengths 41 49 --out <dir>
  python scripts/run_paired_benchmark.py measure --pairs <dir> --lengths 41 49 --out <dir> \
      --warmup 2 --iterations 12 --processes 2 --blocks 3 --seed 1 --cpu 3
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import paired, provenance  # noqa: E402
from shapeperf.compile import model_def  # noqa: E402
from shapeperf.measure import run_block, worker_env  # noqa: E402
from shapeperf.squad import load_features  # noqa: E402
from shapeperf.util import REPO_ROOT, read_json, write_json  # noqa: E402

MODEL = "A_reexport"


def artifact(pairs, length, arm):
    return Path(pairs) / f"L{length:04d}" / f"{arm}_full" / "model.so"


def _inputs(model, feat, length):
    anchor = read_json(REPO_ROOT / model["features"] / "catalog.json")["anchor"]["feature_index"]
    nat = paired.natural_feature_index(feat, length)
    return {"anchor": anchor, "natural": nat}


def cmd_verify(a):
    import onnxruntime as ort
    from shapeperf.inputs import inputs_for
    model = model_def(MODEL)
    feat = load_features(REPO_ROOT / model["features"] / "features.npz")
    so = ort.SessionOptions()
    so.intra_op_num_threads = 1
    ref = ort.InferenceSession(model["abs_path"], so, providers=["CPUExecutionProvider"])
    ref_names = [o.name for o in ref.get_outputs()]
    start, end = model["outputs"]["start"], model["outputs"]["end"]
    out = Path(a.out)
    prov = provenance.freeze(out / "provenance_verify.json", extra={"role": "g1-verify"})
    env = worker_env(1)
    for L in a.lengths:
        rows = []
        for kind, idx in _inputs(model, feat, L).items():
            if idx is None:
                rows.append({"length": L, "input": kind, "skipped": "no natural feature of this length"})
                continue
            valid = int(feat["valid_length"][idx])
            _arrs, named = inputs_for(model, feat, idx, L)
            refo = dict(zip(ref_names, ref.run(None, named)))
            got, flags = {}, {}
            for arm in ("S8", "S1"):
                with tempfile.TemporaryDirectory() as d:
                    spec = {"model_key": MODEL, "artifact": str(artifact(a.pairs, L, arm)), "length": L,
                            "feature_index": idx, "out_npz": str(Path(d) / "o.npz")}
                    p = subprocess.run([sys.executable, "-m", "shapeperf.paired", "--verify-worker", json.dumps(spec)],
                                       cwd=REPO_ROOT, env=env, capture_output=True, text=True)
                    if p.returncode != 0:
                        flags[arm] = {"failure": p.stderr[-1500:]}
                        continue
                    flags[arm] = json.loads(p.stdout.strip().splitlines()[-1])
                    got[arm] = dict(np.load(spec["out_npz"]))
            row = {"length": L, "input": kind, "feature_index": idx, "valid_length": valid, "workers": flags,
                   "provenance_id": prov["provenance_id"]}
            if len(got) == 2:
                row["S8_vs_S1"] = paired.compare_outputs(got["S8"], got["S1"], valid, start, end)
                for arm in ("S8", "S1"):
                    row[f"{arm}_vs_ORT"] = paired.compare_outputs(got[arm], refo, valid, start, end)
            rows.append(row)
            print(f"L={L} {kind}: bitwise S8==S1 {row.get('S8_vs_S1', {}).get('bitwise_identical')}, "
                  f"S8 vs ORT max|d| {row.get('S8_vs_ORT', {}).get('start_max_abs_valid')}", flush=True)
        write_json(out / f"verify_L{L:04d}.json", rows)


def cmd_measure(a):
    model = model_def(MODEL)
    feat = load_features(REPO_ROOT / model["features"] / "features.npz")
    anchor = _inputs(model, feat, 0)["anchor"]
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    prov = provenance.freeze(out / f"provenance_measure_{a.seed}.json",
                             extra={"role": "g1-dev-pilot-timing", "seed": a.seed, "cpu": a.cpu,
                                    "lengths": a.lengths, "note": "development container: NOT a result"})
    raw = out / "measurements.jsonl"
    rng = np.random.default_rng(a.seed)
    order = [a.lengths[i] for i in rng.permutation(len(a.lengths))]      # lengths in random order
    for L in order:
        src = artifact(a.pairs, L, "S8")
        aa = artifact(a.pairs, L, "AA")       # same basename: the entry-point symbols are derived from it
        if not a.no_aa:
            aa.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, aa)                                     # byte-identical A/A arm
        arms = {"S8": src, "S1": artifact(a.pairs, L, "S1")}
        if not a.no_aa:
            arms["AA"] = aa
        hashes = {k: read_json(Path(a.pairs) / f"L{L:04d}" / "pair.json")["full"][("S8" if k == "AA" else k)]
                  ["artifact_hash"] for k in arms}
        for b in range(a.blocks):
            items = [{"artifact": str(p), "artifact_hash": hashes[arm], "model_key": MODEL, "length": L,
                      "feature_index": anchor, "flagset": arm, "warmup": a.warmup, "iterations": a.iterations,
                      "cpus": [a.cpu]}
                     for arm, p in arms.items() for _ in range(a.processes)]
            run_block(items, raw, seed=int(a.seed * 100003 + L * 101 + b), vm_allocation_id=a.allocation_id,
                      experiment_phase="v3-g1 DEV-CONTAINER PILOT (not a result)", threads=1,
                      extra={"provenance_id": prov["provenance_id"], "pilot_seed": a.seed})
        if not a.no_aa:
            shutil.rmtree(aa.parent)
        print(f"L={L} done ({a.blocks} blocks x {len(arms)} arms x {a.processes} processes)", flush=True)


def cmd_cleanup(a):
    n = 0
    for L in a.lengths:
        for p in Path(a.pairs, f"L{L:04d}").glob("*_full/model.so"):
            n += p.stat().st_size
            p.unlink()
        for p in Path(a.pairs, f"L{L:04d}").glob("*_full/model.tmp"):
            p.unlink()
    print(f"deleted {n / 1e9:.2f} GB of artifacts")


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("verify", "measure", "cleanup"):
        s = sub.add_parser(name)
        s.add_argument("--pairs", required=True)
        s.add_argument("--lengths", type=int, nargs="+", required=True)
        if name != "cleanup":
            s.add_argument("--out", required=True)
    m = sub.choices["measure"]
    m.add_argument("--warmup", type=int, required=True)
    m.add_argument("--iterations", type=int, required=True)
    m.add_argument("--processes", type=int, required=True)
    m.add_argument("--blocks", type=int, required=True)
    m.add_argument("--seed", type=int, required=True)
    m.add_argument("--cpu", type=int, required=True)
    m.add_argument("--allocation-id", default="dev-container")
    m.add_argument("--no-aa", action="store_true")
    a = ap.parse_args()
    {"verify": cmd_verify, "measure": cmd_measure, "cleanup": cmd_cleanup}[a.cmd](a)


if __name__ == "__main__":
    if os.environ.get("SHAPEPERF_QUIET_PYC"):
        sys.dont_write_bytecode = True
    main()

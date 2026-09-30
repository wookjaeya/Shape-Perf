#!/usr/bin/env python3
"""R1 (E-R1-2): SQuAD v1.1 dev accuracy of one ONNX-MLIR compiled BERT (static L=256) with the official
postprocessing - the semantic gate of results/r1/protocol_r1.json.

Every feature runs at L=256 (the length of the ORT reference, results/g1/*_official), in `--shards` fresh
worker processes, each loading ONLY this model (with its --tag). Logits are compared with the saved ORT@256
reference logits and, with --compare-to, the predicted answers with another arm's predictions.
Wall time is recorded but is not a benchmark.

Resumable: workers save every `--chunk` features to part files and stop before `--budget-min` minutes;
a rerun with the same --out continues with the missing features. EM/F1 are computed only when all
features are present (exit code 3 = incomplete, run again).

  python scripts/r1_squad_eval.py --so <model.so> --tag <tag> --arm r1b --out results/r1/squad/r1b \
      [--compare-to results/r1/squad/orig] [--shards 4]
"""
import argparse
import json
import os
import pickle
import subprocess
import sys
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import squad  # noqa: E402
from shapeperf.util import REPO_ROOT, sha256_file, write_json  # noqa: E402

FEATURES = REPO_ROOT / "data/features/A"
DATASET = REPO_ROOT / "data/artifacts/squad/dev-v1.1.json"
ORT_REF = REPO_ROOT / "data/reference/bertsquad-12-reexport-dynseq_official/logits.npz"
L = 256


def worker(spec):
    from shapeperf import toolchain
    from shapeperf.compile import model_def
    from shapeperf.identity import open_session
    from shapeperf.inputs import inputs_for
    os.sched_setaffinity(0, {spec["cpu"]})
    model = model_def("A_reexport")
    feat = squad.load_features(FEATURES / "features.npz")
    sess = open_session(toolchain.import_pyruntime(), spec["so"], spec["tag"])
    names = [d["name"] for d in json.loads(sess.output_signature())]
    i_start, i_end = names.index(squad.OUTPUT_NAMES_A["start"]), names.index(squad.OUTPUT_NAMES_A["end"])
    import glob
    import time
    idx = np.arange(spec["shard"], spec["n"], spec["nshards"])
    done = set()
    for f in glob.glob(f"{spec['out_prefix']}_part*.npz"):
        done.update(int(i) for i in np.load(f)["idx"])
    todo = [int(i) for i in idx if int(i) not in done]
    part = len(glob.glob(f"{spec['out_prefix']}_part*.npz"))
    n_done = 0
    for c in range(0, len(todo), spec["chunk"]):
        if time.time() > spec["deadline"]:
            break
        chunk = todo[c:c + spec["chunk"]]
        start = np.zeros((len(chunk), L), np.float32)
        end = np.zeros((len(chunk), L), np.float32)
        for k, i in enumerate(chunk):
            arrs, _ = inputs_for(model, feat, i, L)
            out = sess.run(arrs)
            start[k], end[k] = out[i_start][0], out[i_end][0]
        np.savez(f"{spec['out_prefix']}_part{part:04d}.npz", idx=np.array(chunk), start=start, end=end)
        part += 1
        n_done += len(chunk)
    return {"n_new": n_done, "n_remaining": len(todo) - n_done}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--so")
    ap.add_argument("--tag")
    ap.add_argument("--arm")
    ap.add_argument("--out")
    ap.add_argument("--compare-to", default=None)
    ap.add_argument("--shards", type=int, default=4)
    ap.add_argument("--limit", type=int, default=None, help="smoke test only (no EM/F1)")
    ap.add_argument("--chunk", type=int, default=100)
    ap.add_argument("--budget-min", type=float, default=24.0, help="workers stop starting new chunks after this")
    ap.add_argument("--worker", default=None)
    a = ap.parse_args()
    if a.worker:
        print(json.dumps(worker(json.loads(Path(a.worker).read_text()))))
        return
    out = Path(a.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    feat = squad.load_features(FEATURES / "features.npz")
    n = len(feat["unique_id"]) if a.limit is None else a.limit
    env = dict(os.environ, OMP_NUM_THREADS="1", MKL_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    t0 = time.time()
    procs = []
    for k in range(a.shards):
        spec = {"so": a.so, "tag": a.tag, "shard": k, "nshards": a.shards, "n": n, "cpu": k % os.cpu_count(),
                "out_prefix": str(out / f"shard{k}"), "chunk": a.chunk, "deadline": t0 + a.budget_min * 60}
        (out / f"shard{k}.json").write_text(json.dumps(spec))
        procs.append(subprocess.Popen([sys.executable, str(Path(__file__).resolve()), "--worker", str(out / f"shard{k}.json")],
                                      cwd=REPO_ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True))
    fails = []
    for k, p in enumerate(procs):
        so, se = p.communicate()
        if p.returncode != 0:
            fails.append({"shard": k, "stderr": se[-1500:]})
    wall = time.time() - t0
    with open(out / "runs.jsonl", "a") as f:
        f.write(json.dumps({"started_unix": t0, "wall_seconds": round(wall, 1), "failed_shards": fails}) + "\n")
    if fails:
        raise SystemExit(f"{len(fails)} shard(s) failed: {fails}")
    import glob
    start = np.full((n, L), np.nan, np.float32)
    end = np.full((n, L), np.nan, np.float32)
    have = np.zeros(n, bool)
    for f in sorted(glob.glob(str(out / "shard*_part*.npz"))):
        z = np.load(f)
        start[z["idx"]], end[z["idx"]] = z["start"], z["end"]
        have[z["idx"]] = True
    if not have.all():
        print(f"incomplete: {int(have.sum())}/{n} features done; run again with the same --out", flush=True)
        raise SystemExit(3)
    np.savez_compressed(out / "logits.npz", start=start, end=end)
    for f in glob.glob(str(out / "shard*_part*.npz")):
        os.remove(f)
    runs = [json.loads(ln) for ln in (out / "runs.jsonl").read_text().splitlines()]
    wall = sum(r["wall_seconds"] for r in runs)
    ref = np.load(ORT_REF)
    res = {"arm": a.arm, "so": a.so, "so_sha256": sha256_file(a.so), "tag": a.tag, "length": L, "n_features": n,
           "shards": a.shards, "wall_seconds_not_a_benchmark": round(wall, 1), "n_invocations": len(runs),
           "logits_vs_ort_ref": {"start_max_abs": float(np.abs(start - ref["start"][:n]).max()),
                                 "end_max_abs": float(np.abs(end - ref["end"][:n]).max())}}
    if a.limit is None:
        ros, _, _shim = squad.load_official()
        with open(FEATURES / "features_extra.pkl", "rb") as f:
            ex = pickle.load(f)
        pred_file = squad.write_official_predictions(ros, ex["examples"], ex["extra"], start, end, out,
                                                     lengths=np.full(n, L))
        res["official_eval"] = squad.run_official_eval(str(DATASET), pred_file)
        pred = json.loads(Path(pred_file).read_text())
        ort_pred = json.loads((REPO_ROOT / "results/g1/bertsquad-12-reexport-dynseq_official/predictions.json").read_text())
        res["answers_differing_from_ort_ref"] = sum(pred[q] != ort_pred.get(q) for q in pred)
        if a.compare_to:
            other = Path(a.compare_to).resolve()
            o_pred = json.loads((other / "predictions.json").read_text())
            o_eval = json.loads((other / "eval.json").read_text())
            o_log = np.load(other / "logits.npz")
            res["vs_compare_to"] = {
                "arm": o_eval["arm"], "answers_differing": sum(pred[q] != o_pred.get(q) for q in pred),
                "n_questions": len(pred),
                "delta_em": res["official_eval"]["exact_match"] - o_eval["official_eval"]["exact_match"],
                "delta_f1": res["official_eval"]["f1"] - o_eval["official_eval"]["f1"],
                "start_max_abs": float(np.abs(start - o_log["start"]).max()),
                "end_max_abs": float(np.abs(end - o_log["end"]).max())}
    write_json(out / "eval.json", res)
    print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()

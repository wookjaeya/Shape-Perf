#!/usr/bin/env python3
"""G1: reproduce model-A accuracy at the official length with an independent
reference runtime (ONNX Runtime), spec §4.1 step 4 and §7.1.

1. zoo test_data_set_* check: run the tarball's stored inputs, compare with
   its stored outputs (artifact + runtime sanity; no tolerance is *fixed* here,
   max abs/rel differences are reported).
2. full SQuAD v1.1 dev set at length 256, batch 1, official postprocessing and
   official evaluate_v1.1.py -> EM/F1, compared with the model card value.
Logits are saved for the later padding-semantics checks (§7.2).
"""
import argparse
import glob
import json
import os
import pickle
import sys
import tarfile
import time
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import squad  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, sha256_file, write_json  # noqa: E402

MODEL_CARD_EM = 80.67171  # onnx/models bert-squad README, bertsquad-12 "Accuracy" column


def ort_session(model, threads):
    import onnxruntime as ort
    so = ort.SessionOptions()
    so.intra_op_num_threads = threads
    so.inter_op_num_threads = 1
    return ort.InferenceSession(str(model), so, providers=["CPUExecutionProvider"])


def zoo_testdata_check(sess, tar_path, workdir):
    from onnx import numpy_helper, TensorProto
    workdir.mkdir(parents=True, exist_ok=True)
    with tarfile.open(tar_path) as t:
        members = [m for m in t.getmembers() if "test_data_set" in m.name and m.name.endswith(".pb")]
        t.extractall(workdir, members=members, filter="data")
    report = []
    in_names = [i.name for i in sess.get_inputs()]
    out_names = [o.name for o in sess.get_outputs()]
    for ds in sorted(glob.glob(str(workdir / "**/test_data_set_*"), recursive=True)):
        def load(pattern):
            arrs = []
            for f in sorted(glob.glob(os.path.join(ds, pattern))):
                tp = TensorProto()
                tp.ParseFromString(open(f, "rb").read())
                arrs.append((tp.name, numpy_helper.to_array(tp)))
            return arrs
        ins, outs = load("input_*.pb"), load("output_*.pb")
        feed = {}
        for k, (name, arr) in enumerate(ins):
            feed[name if name in in_names else in_names[k]] = arr
        got = sess.run(None, feed)
        diffs = {}
        for k, (name, ref) in enumerate(outs):
            j = out_names.index(name) if name in out_names else k
            g = got[j]
            if ref.dtype.kind == "f":
                ad = np.abs(g.astype(np.float64) - ref.astype(np.float64))
                diffs[name or out_names[j]] = {"max_abs": float(ad.max()),
                                               "max_rel": float((ad / np.maximum(np.abs(ref), 1e-12)).max()),
                                               "shape": list(ref.shape)}
            else:
                diffs[name or out_names[j]] = {"equal": bool(np.array_equal(g, ref)), "shape": list(ref.shape)}
        report.append({"dataset": os.path.relpath(ds, workdir), "outputs": diffs})
    return report


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default=str(REPO_ROOT / "data/artifacts/bertsquad-12.onnx"))
    ap.add_argument("--features", default=str(REPO_ROOT / "data/features/A"))
    ap.add_argument("--dataset", default=str(REPO_ROOT / "data/artifacts/squad/dev-v1.1.json"))
    ap.add_argument("--out", default=str(REPO_ROOT / "results/g1"))
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--limit", type=int, default=None, help="smoke test only")
    ap.add_argument("--length-policy", choices=["official", "natural"], default="official",
                    help="official: every feature at 256; natural: feature x at L(x) (spec §7.4, "
                         "needs a model with a variable length axis)")
    ap.add_argument("--tag", default=None, help="subdirectory name for outputs")
    args = ap.parse_args()
    import onnxruntime as ort

    tag = args.tag or f"{Path(args.model).stem}_{args.length_policy}"
    out = Path(args.out) / tag
    sess = ort_session(args.model, args.threads)
    res = {"harness_commit": git_head(), "onnxruntime": ort.__version__,
           "threads": args.threads, "model": args.model, "model_sha256": sha256_file(args.model),
           "length_policy": args.length_policy}

    tar = REPO_ROOT / "data/artifacts/bertsquad-12.tar.gz"
    if tar.exists() and args.length_policy == "official":
        res["zoo_test_data"] = zoo_testdata_check(sess, tar, REPO_ROOT / "data/artifacts/zoo_testdata")

    ros, _, shim = squad.load_official()  # must precede unpickling Feature tuples
    feat = squad.load_features(Path(args.features) / "features.npz")
    with open(Path(args.features) / "features_extra.pkl", "rb") as f:
        ex = pickle.load(f)
    n = len(feat["unique_id"]) if args.limit is None else args.limit
    L = feat["input_ids"].shape[1]
    start = np.full((n, L), np.nan, np.float32)
    end = np.full((n, L), np.nan, np.float32)
    lengths = (np.full(n, L) if args.length_policy == "official" else feat["valid_length"][:n]).astype(np.int64)
    I, O = squad.INPUT_NAMES_A, squad.OUTPUT_NAMES_A
    t0 = time.time()
    for i in range(n):
        s = int(lengths[i])
        x = squad.make_padded_inputs(feat, i, s)
        r = sess.run([O["start"], O["end"], O["unique_ids"]], {I[k]: v for k, v in x.items()})
        start[i, :s], end[i, :s] = r[0][0], r[1][0]
        assert int(r[2][0]) == int(feat["unique_id"][i])
        if i and i % 1000 == 0:
            print(f"{i}/{n} {(time.time() - t0) / i:.3f}s/feature", flush=True)
    res["inference_seconds_total_not_a_benchmark"] = round(time.time() - t0, 1)
    ref_dir = REPO_ROOT / "data/reference" / tag
    ref_dir.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(ref_dir / "logits.npz", start=start, end=end, lengths=lengths,
                        unique_id=feat["unique_id"][:n])

    extra = ex["extra"][:n]
    pred = squad.write_official_predictions(ros, ex["examples"], extra, start, end, out, lengths=lengths)
    res["n_features_evaluated"] = n
    if args.limit is None:
        res["official_eval"] = squad.run_official_eval(args.dataset, pred)
        res["model_card_em"] = MODEL_CARD_EM
        res["em_minus_model_card"] = res["official_eval"]["exact_match"] - MODEL_CARD_EM
    else:
        res["official_eval"] = "skipped (limit set; partial predictions would under-count)"
    res["adaptations"] = [a for a in [shim] if a]
    write_json(out / "reference_eval.json", res)
    print(json.dumps({k: res[k] for k in res if k != "zoo_test_data"}, indent=1))


if __name__ == "__main__":
    main()

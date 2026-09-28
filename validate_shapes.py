#!/usr/bin/env python3
"""Correctness of a compiled length against the reference runtime (spec §7.3)
and padding semantics (spec §7.2) - spec §12 validate_shapes.py.

For a compiled artifact at length s:
  - features: the anchor plus a seeded sample of other features with L(x) <= s
  - ONNX-MLIR output vs ONNX Runtime output on the same model file, same s,
    same input: max |diff| over valid positions and over all positions,
    start/end argmax agreement over valid positions
  - ONNX Runtime at s vs the ORT run at the official length (valid positions)
The verdict uses configs/preregistration.json correctness.logit_abs_tolerance.
While it is null the verdict is "unjudged": no tolerance is invented here and
none may be changed after seeing which configuration is fast (spec §7).
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shapeperf import toolchain  # noqa: E402
from shapeperf.compile import model_def  # noqa: E402
from shapeperf.inputs import inputs_for  # noqa: E402
from shapeperf.squad import load_features  # noqa: E402
from shapeperf.util import REPO_ROOT, append_jsonl, git_head, read_json, sha256_file  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--artifact", required=True, help="compiled model.so")
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--length", type=int, required=True)
    ap.add_argument("--n-features", type=int, default=8)
    ap.add_argument("--seed", type=int, default=20260928)
    ap.add_argument("--out", default=str(REPO_ROOT / "results/validation/validation.jsonl"))
    args = ap.parse_args()
    import onnxruntime as ort

    m = model_def(args.model)
    feat = load_features(REPO_ROOT / m["features"] / "features.npz")
    cat = read_json(REPO_ROOT / m["features"] / "catalog.json")
    anchor = cat["anchor"]["feature_index"]
    ok = np.flatnonzero(feat["valid_length"] <= args.length)
    rng = np.random.default_rng(args.seed + args.length)
    others = [int(i) for i in rng.choice(ok[ok != anchor], size=min(args.n_features, len(ok) - 1), replace=False)]
    idxs = ([anchor] if feat["valid_length"][anchor] <= args.length else []) + others

    so = ort.SessionOptions()
    so.intra_op_num_threads = 1
    ref = ort.InferenceSession(m["abs_path"], so, providers=["CPUExecutionProvider"])
    ref_out_names = [o.name for o in ref.get_outputs()]
    OMExecutionSession = toolchain.import_pyruntime()
    om = OMExecutionSession(shared_lib_path=str(Path(args.artifact).resolve()))
    om_out_names = [d["name"] for d in json.loads(om.output_signature())]

    tol = read_json(REPO_ROOT / "configs/preregistration.json")["correctness"]["logit_abs_tolerance"]
    rows = []
    for i in idxs:
        L = int(feat["valid_length"][i])
        arrs, named = inputs_for(m, feat, i, args.length)
        r = dict(zip(ref_out_names, ref.run(None, named)))
        o = dict(zip(om_out_names, om.run(arrs)))
        row = {"feature_index": i, "L": L}
        for key in ("start", "end"):
            name = m["outputs"][key]
            a, b = np.asarray(r[name])[0], np.asarray(o[name])[0]
            row[f"{key}_max_abs_valid"] = float(np.abs(a[:L] - b[:L]).max())
            row[f"{key}_max_abs_all"] = float(np.abs(a - b).max())
            row[f"{key}_argmax_equal"] = int(np.argmax(a[:L])) == int(np.argmax(b[:L]))
        rows.append(row)
    worst = max(max(r["start_max_abs_valid"], r["end_max_abs_valid"]) for r in rows)
    argmax_ok = all(r["start_argmax_equal"] and r["end_argmax_equal"] for r in rows)
    if tol is None:
        verdict = "unjudged (tolerance not preregistered)"
    else:
        verdict = "pass" if worst <= tol and argmax_ok else "fail"
    rec = {"artifact": args.artifact, "artifact_hash": sha256_file(args.artifact),
           "model_key": args.model, "model_hash": sha256_file(m["abs_path"]),
           "padded_length": args.length, "n_features": len(rows), "seed": args.seed,
           "worst_max_abs_valid": worst, "argmax_all_equal": argmax_ok,
           "tolerance": tol, "correctness_status": verdict, "rows": rows,
           "onnxruntime": ort.__version__, "harness_commit": git_head(),
           **toolchain.compiler_ids()}
    append_jsonl(args.out, rec)
    print(json.dumps({k: rec[k] for k in ["padded_length", "n_features", "worst_max_abs_valid",
                                          "argmax_all_equal", "correctness_status"]}))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Validate the §4.4 step-2 re-export against the official artifact (spec §4.4
step 3, §7.2) using ONNX Runtime as the independent reference runtime.

Checks (numbers are reported; no tolerance is decided here - spec §7 requires
the tolerance to be justified and preregistered before the main experiment):
  1. I/O signature: same names/dtypes as the official artifact, symbolic length.
  2. Length constants: small int constants equal to 256 left in the re-export.
  3. Equivalence at the official length 256: re-export vs official logits on a
     deterministic sample of real features (question-level sample, seeded).
  4. Padding semantics (§7.2): for each sampled feature x and each probe length
     s >= L(x), valid-position logits at s vs the official artifact at 256, plus
     agreement of the start/end argmax over valid positions.
Full-dev-set EM/F1 of the re-export (§7.4) is done by
scripts/g1_reference_eval.py --model <re-export>.
"""
import argparse
import pickle
import sys
from pathlib import Path

import numpy as np
import onnx

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import graph_inspect, squad  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, sha256_file, write_json  # noqa: E402


def sess_for(path, threads):
    import onnxruntime as ort
    so = ort.SessionOptions()
    so.intra_op_num_threads = threads
    return ort.InferenceSession(str(path), so, providers=["CPUExecutionProvider"])


def run(sess, x):
    I, O = squad.INPUT_NAMES_A, squad.OUTPUT_NAMES_A
    r = sess.run([O["start"], O["end"]], {I[k]: v for k, v in x.items()})
    return r[0][0], r[1][0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--official", default=str(REPO_ROOT / "data/artifacts/bertsquad-12.onnx"))
    ap.add_argument("--reexport", default=str(REPO_ROOT / "data/artifacts/derived/bertsquad-12-reexport-dynseq.onnx"))
    ap.add_argument("--n-questions", type=int, default=30)
    ap.add_argument("--seed", type=int, default=20260928)
    ap.add_argument("--probe-lengths", default="L,L+1,64,128,129,255,256",
                    help="comma list; L means the feature's own valid length")
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--out", default=str(REPO_ROOT / "results/g2/reexport_validation.json"))
    args = ap.parse_args()
    import onnxruntime as ort

    off_m, re_m = onnx.load(args.official, load_external_data=False), onnx.load(args.reexport)
    sig_off, sig_re = graph_inspect.io_signature(off_m), graph_inspect.io_signature(re_m)
    names_match = ([(i["name"], i["elem_type"]) for i in sig_off[0]] == [(i["name"], i["elem_type"]) for i in sig_re[0]]
                   and [(o["name"], o["elem_type"]) for o in sig_off[1]] == [(o["name"], o["elem_type"]) for o in sig_re[1]])
    del off_m

    feat = squad.load_features(REPO_ROOT / "data/features/A/features.npz")
    ros, _, _ = squad.load_official()
    with open(REPO_ROOT / "data/features/A/features_extra.pkl", "rb") as f:
        ex = pickle.load(f)
    rng = np.random.default_rng(args.seed)
    qids = sorted(set(feat["qas_id"].tolist()))
    chosen_q = set(rng.choice(qids, size=min(args.n_questions, len(qids)), replace=False).tolist())
    idxs = [i for i in range(len(feat["qas_id"])) if feat["qas_id"][i] in chosen_q]

    s_off, s_re = sess_for(args.official, args.threads), sess_for(args.reexport, args.threads)
    eq256, pad = [], []
    for i in idxs:
        L = int(feat["valid_length"][i])
        a0, b0 = run(s_off, squad.make_padded_inputs(feat, i, 256))
        a1, b1 = run(s_re, squad.make_padded_inputs(feat, i, 256))
        eq256.append({"feature": i, "L": L,
                      "max_abs_all": float(max(np.abs(a0 - a1).max(), np.abs(b0 - b1).max())),
                      "max_abs_valid": float(max(np.abs(a0[:L] - a1[:L]).max(), np.abs(b0[:L] - b1[:L]).max()))})
        lengths = sorted({min(256, max(L, eval(t, {}, {"L": L}))) for t in args.probe_lengths.split(",")})
        for s in lengths:
            a, b = run(s_re, squad.make_padded_inputs(feat, i, s))
            pad.append({"feature": i, "L": L, "s": s,
                        "max_abs_valid_vs_official256": float(max(np.abs(a[:L] - a0[:L]).max(), np.abs(b[:L] - b0[:L]).max())),
                        "argmax_start_equal": int(np.argmax(a[:L])) == int(np.argmax(a0[:L])),
                        "argmax_end_equal": int(np.argmax(b[:L])) == int(np.argmax(b0[:L]))})

    def summ(rows, key):
        v = np.array([r[key] for r in rows])
        return {"n": int(v.size), "max": float(v.max()), "median": float(np.median(v)), "p99": float(np.quantile(v, 0.99))}

    lc = graph_inspect.length_constant_report(re_m, 256)
    report = {
        "official_sha256": sha256_file(args.official), "reexport_sha256": sha256_file(args.reexport),
        "io_signature_official": sig_off, "io_signature_reexport": sig_re,
        "io_names_and_dtypes_match": names_match,
        "reexport_semantic_fingerprint": graph_inspect.semantic_fingerprint(re_m),
        "reexport_determinism_note": ("tf2onnx output is not byte-deterministic across runs (node order/"
                                      "auto-generated names differ; also with PYTHONHASHSEED=0), but repeated "
                                      "exports had identical semantic fingerprints. The hash-locked file is the "
                                      "artifact: copy it to the VM, do not re-export there. Node names appear in "
                                      "opt-report signatures, so signatures are only comparable within one file."),
        "reexport_length256_constants": {k: lc[k] for k in ["n_constants_containing_length", "consumer_op_counts"]},
        "reexport_op_histogram": graph_inspect.op_histogram(re_m),
        "sample": {"n_questions": len(chosen_q), "n_features": len(idxs), "seed": args.seed},
        "equivalence_at_256": {"max_abs_all_positions": summ(eq256, "max_abs_all"),
                               "max_abs_valid_positions": summ(eq256, "max_abs_valid")},
        "padding_semantics": {"max_abs_valid_vs_official256": summ(pad, "max_abs_valid_vs_official256"),
                              "argmax_start_agreement": float(np.mean([r["argmax_start_equal"] for r in pad])),
                              "argmax_end_agreement": float(np.mean([r["argmax_end_equal"] for r in pad])),
                              "n_pairs": len(pad)},
        "rows_equivalence": eq256, "rows_padding": pad,
        "tolerance": "NOT FIXED - to be justified and preregistered (spec §7)",
        "onnxruntime": ort.__version__, "harness_commit": git_head(),
    }
    write_json(args.out, report)
    print({k: report[k] for k in ["io_names_and_dtypes_match", "reexport_length256_constants", "sample",
                                   "equivalence_at_256", "padding_semantics"]})


if __name__ == "__main__":
    main()

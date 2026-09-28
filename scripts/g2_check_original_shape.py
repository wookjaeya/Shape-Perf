#!/usr/bin/env python3
"""G2 step 1 (spec §4.4): does the *official* artifact support other lengths?

Evidence recorded:
  - declared input/output signature
  - small integer constants equal to the official length and their consumers
  - runtime test: metadata-only length change + a real feature padded to s,
    run in ONNX Runtime. Any error is recorded verbatim.
The metadata-edited copy is only a probe and is never saved as an artifact.
"""
import argparse
import sys
from pathlib import Path

import onnx

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import graph_inspect, squad  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, sha256_file, write_json  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default=str(REPO_ROOT / "data/artifacts/bertsquad-12.onnx"))
    ap.add_argument("--official-length", type=int, default=256)
    ap.add_argument("--probe-lengths", type=int, nargs="+", default=[255, 128, 41])
    ap.add_argument("--out", default=str(REPO_ROOT / "results/g2/original_artifact_shape_check.json"))
    args = ap.parse_args()
    import numpy as np
    import onnxruntime as ort

    model = onnx.load(args.model)
    ins, outs = graph_inspect.io_signature(model)
    feat = squad.load_features(REPO_ROOT / "data/features/A/features.npz")
    anchor = int(np.argmin(feat["valid_length"]))
    I, O = squad.INPUT_NAMES_A, squad.OUTPUT_NAMES_A

    probes = []
    for s in [args.official_length] + args.probe_lengths:
        m = model if s == args.official_length else graph_inspect.with_input_length(model, s)
        rec = {"length": s, "metadata_only_edit": s != args.official_length}
        try:
            sess = ort.InferenceSession(m.SerializeToString(), providers=["CPUExecutionProvider"])
            x = squad.make_padded_inputs(feat, anchor, s)
            r = sess.run([O["start"], O["end"]], {I[k]: v for k, v in x.items()})
            rec.update(status="ran", output_shapes=[list(a.shape) for a in r])
        except Exception as e:
            rec.update(status="failed", error=str(e)[:2000])
        probes.append(rec)

    report = {
        "model": args.model, "model_sha256": sha256_file(args.model),
        "producer": f"{model.producer_name} {model.producer_version}",
        "opset": [(o.domain, o.version) for o in model.opset_import],
        "inputs": ins, "outputs": outs,
        "op_histogram": graph_inspect.op_histogram(model),
        "length_constants": graph_inspect.length_constant_report(model, args.official_length),
        "runtime_probes": probes,
        "onnxruntime": ort.__version__, "harness_commit": git_head(),
    }
    ok_other = [p for p in probes if p["metadata_only_edit"] and p["status"] == "ran"]
    report["verdict"] = (
        "SUPPORTED_BY_METADATA_EDIT (still needs §7 correctness)" if len(ok_other) == len(args.probe_lengths)
        else "NOT_SUPPORTED: official artifact has length-specific internal constants; "
             "spec §4.4 step 2 (re-export from the same weights) required")
    write_json(args.out, report)
    lc = report["length_constants"]
    print(report["verdict"])
    print("constants containing", args.official_length, ":", lc["n_constants_containing_length"], lc["consumer_op_counts"])
    for p in probes:
        print(p["length"], p["status"], p.get("error", "")[:160])


if __name__ == "__main__":
    main()

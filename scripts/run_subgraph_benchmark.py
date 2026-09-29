#!/usr/bin/env python3
"""Cause isolation on an observed sub-graph (design amendment v3, G2 steps 2-5).

Extracts the transposes of layer 0 of the real model together with their REAL input activation
(captured with ORT at length L on the controlled anchor input), builds a one-op ONNX model for each,
compiles it with both arms (S8 = pinned compiler, S1 = transpose-unroll cap 1) and times it with
batches of calls (amendment §11.6: Python call overhead is constant and cancels in the paired
difference, but dilutes relative effects - so absolute differences are reported too).

  kind K  perm [0,2,3,1], input [1,L,12,64]: the scalar transpose (`scalarTransposeOverOutputs`)
          - the loop the tested decision applies to. Predicted: S8 has 1 unrolled loop iff u(L)>1.
  kind Q  perm [0,2,1,3], same input: keeps the last dimension -> `blockTranspose` path.
          NEGATIVE CONTROL: the patched code is never reached, S8 and S1 must be identical.

Correctness: the compiled output must equal np.transpose(input) BITWISE (exact permutation).

  python scripts/run_subgraph_benchmark.py build   --lengths 41 49 --out <dir>
  python scripts/run_subgraph_benchmark.py measure --lengths 41 49 --out <dir> --calls 400 \
        --reps 15 --warmup-calls 200 --processes 2 --blocks 4 --seed 3 --cpu 3

DEVELOPMENT-CONTAINER TIMINGS ARE NOT RESULTS (amendment §9.1).
"""
import argparse
import hashlib
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import lowering_diff as LD  # noqa: E402
from shapeperf import provenance, toolchain  # noqa: E402
from shapeperf.compile import model_def  # noqa: E402
from shapeperf.measure import run_block  # noqa: E402
from shapeperf.squad import load_features  # noqa: E402
from shapeperf.util import REPO_ROOT, read_json, write_json  # noqa: E402

ARMS = {"S8": "/home/user/work/variants/orig", "S1": "/home/user/work/variants/cap1"}
KINDS = {"K": {"perm": [0, 2, 3, 1], "node": "bert/encoder/layer_0/attention/self/MatMul__324"},
         "Q": {"perm": [0, 2, 1, 3], "node": "bert/encoder/layer_0/attention/self/transpose_2"}}
HEADS, HEAD_DIM = 12, 64


def activations(lengths):
    """Real input tensor of each layer-0 transpose at every length (ORT, controlled anchor input)."""
    import onnx
    import onnxruntime as ort
    from shapeperf.inputs import inputs_for
    model = model_def("A_reexport")
    feat = load_features(REPO_ROOT / model["features"] / "features.npz")
    anchor = read_json(REPO_ROOT / model["features"] / "catalog.json")["anchor"]["feature_index"]
    m = onnx.load(model["abs_path"])
    tensors = {}
    for kind, spec in KINDS.items():
        node = next(n for n in m.graph.node if n.name == spec["node"])
        assert node.op_type == "Transpose" and list(node.attribute[0].ints) == spec["perm"], node.name
        tensors[kind] = node.input[0]
        m.graph.output.append(onnx.helper.make_empty_tensor_value_info(node.input[0]))
    so = ort.SessionOptions()
    so.intra_op_num_threads = 1
    sess = ort.InferenceSession(m.SerializeToString(), so, providers=["CPUExecutionProvider"])
    outs = [o.name for o in sess.get_outputs()]
    res = {}
    for L in lengths:
        _arrs, named = inputs_for(model, feat, anchor, L)
        got = dict(zip(outs, sess.run(None, named)))
        for kind, name in tensors.items():
            x = np.ascontiguousarray(got[name])
            assert x.shape == (1, L, HEADS, HEAD_DIM) and x.dtype == np.float32, (kind, x.shape, x.dtype)
            res[(kind, L)] = x
    return res


def write_single_op_model(path, length, perm):
    import onnx
    from onnx import TensorProto, helper
    out_shape = [1, HEADS, HEAD_DIM, length] if perm == [0, 2, 3, 1] else [1, HEADS, length, HEAD_DIM]
    node = helper.make_node("Transpose", ["x"], ["y"], perm=perm, name="transpose")
    g = helper.make_graph([node], "single_transpose",
                          [helper.make_tensor_value_info("x", TensorProto.FLOAT, [1, length, HEADS, HEAD_DIM])],
                          [helper.make_tensor_value_info("y", TensorProto.FLOAT, out_shape)])
    m = helper.make_model(g, opset_imports=[helper.make_opsetid("", 12)])
    m.ir_version = 7
    onnx.save(m, str(path))


def compile_arm(job):
    kind, length, arm, mode, model_path, out_dir, target = job
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    fs = toolchain.resolve_flagset("default", target)
    binary = toolchain.onnx_mlir_bin(ARMS[arm])
    cmd = [str(binary), *fs["flags"], "-o", str(out_dir / "model"),
           "--EmitLib" if mode == "full" else "--EmitMLIR", str(model_path)]
    p = subprocess.run(cmd, capture_output=True, text=True)
    rec = {"kind": kind, "length": length, "arm": arm, "mode": mode, "returncode": p.returncode,
           "command": cmd, "stderr_tail": p.stderr[-500:] if p.returncode else "",
           **toolchain.compiler_identity(ARMS[arm])}
    so = out_dir / "model.so"
    if mode == "full" and so.exists():
        rec["artifact_hash"] = hashlib.sha256(so.read_bytes()).hexdigest()
        rec["artifact_bytes"] = so.stat().st_size
    return rec


def cmd_build(a):
    out = Path(a.out)
    prov = provenance.freeze(out / "provenance_build.json", compiler_builds=list(ARMS.values()),
                             extra={"role": "g2-subgraph-build", "lengths": a.lengths})
    acts = activations(a.lengths)
    jobs = []
    for (kind, L), x in sorted(acts.items()):
        d = out / kind / f"L{L:04d}"
        d.mkdir(parents=True, exist_ok=True)
        perm = KINDS[kind]["perm"]
        np.save(d / "input.npy", x)
        np.save(d / "expected.npy", np.ascontiguousarray(np.transpose(x, perm)))
        write_single_op_model(d / "model.onnx", L, perm)
        for arm in ARMS:
            for mode in ("probe", "full"):
                jobs.append((kind, L, arm, mode, d / "model.onnx", d / f"{arm}_{mode}", a.target_cpu))
    with ThreadPoolExecutor(max_workers=a.jobs) as ex:
        recs = list(ex.map(compile_arm, jobs))
    by = {(r["kind"], r["length"], r["arm"], r["mode"]): r for r in recs}
    summary = []
    for (kind, L), x in sorted(acts.items()):
        d = out / kind / f"L{L:04d}"
        u = LD.no_leftover_unroll(L)
        text = {arm: (d / f"{arm}_probe" / "model.onnx.mlir").read_text(errors="replace") for arm in ARMS}
        n = {arm: LD.count_unrolled_loops(text[arm], L, u) if u > 1 else 0 for arm in ARMS}
        # K: the scalar transpose is unrolled iff u > 1 (one loop); Q never reaches the patched code
        predicted = {"S8": 1 if (kind == "K" and u > 1) else 0, "S1": 0}
        same_ir = text["S8"] == text["S1"]
        full = {arm: by[(kind, L, arm, "full")] for arm in ARMS}
        entry = {"kind": kind, "length": L, "u": u, "input_sha256": hashlib.sha256(x.tobytes()).hexdigest(),
                 "unrolled_loops": n, "predicted_unrolled_loops": predicted,
                 "ir_identical": same_ir, "predicted_ir_identical": (kind == "Q" or u == 1),
                 "normalized_diff": LD.diff_hunks(text["S8"], text["S1"]),
                 "so_hash": {arm: full[arm].get("artifact_hash") for arm in ARMS},
                 "so_identical": full["S8"].get("artifact_hash") == full["S1"].get("artifact_hash"),
                 "compile_ok": all(r["returncode"] == 0 for r in recs if r["kind"] == kind and r["length"] == L),
                 "provenance_id": prov["provenance_id"]}
        entry["prediction_holds"] = (entry["unrolled_loops"] == predicted and
                                     entry["ir_identical"] == entry["predicted_ir_identical"])
        summary.append(entry)
        write_json(d / "build.json", entry)
        print(f"{kind} L={L} u={u} loops S8/S1={n} predicted={predicted} ir_identical={same_ir} "
              f"so_identical={entry['so_identical']} prediction_holds={entry['prediction_holds']}", flush=True)
    write_json(out / "build_summary.json", summary)


def cmd_measure(a):
    out = Path(a.out)
    prov = provenance.freeze(out / f"provenance_measure_{a.seed}.json",
                             extra={"role": "g2-subgraph-timing", "seed": a.seed, "cpu": a.cpu,
                                    "note": "development container: NOT a result"})
    raw = out / "measurements.jsonl"
    rng = np.random.default_rng(a.seed)
    cells = [(k, L) for k in a.kinds for L in a.lengths]
    for i in rng.permutation(len(cells)):
        kind, L = cells[i]
        d = out / kind / f"L{L:04d}"
        arms = {"S8": d / "S8_full" / "model.so", "S1": d / "S1_full" / "model.so"}
        if not a.no_aa:
            aa = d / "AA_full" / "model.so"
            aa.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(arms["S8"], aa)                # byte-identical copy, same basename
            arms["AA"] = aa
        build = read_json(d / "build.json")
        hashes = {arm: build["so_hash"]["S8" if arm == "AA" else arm] for arm in arms}
        for b in range(a.blocks):
            items = [{"worker_module": "shapeperf.paired", "artifact": str(p), "artifact_hash": hashes[arm],
                      "model_key": f"subgraph:{kind}", "length": L, "feature_index": -1, "flagset": arm,
                      "warmup": a.warmup_calls, "iterations": a.reps, "cpus": [a.cpu],
                      "input_npy": str(d / "input.npy"), "expected_npy": str(d / "expected.npy"),
                      "calls": a.calls, "reps": a.reps, "warmup_calls": a.warmup_calls}
                     for arm, p in arms.items() for _ in range(a.processes)]
            run_block(items, raw, seed=int(a.seed * 100003 + L * 101 + b + (7 if kind == "Q" else 0)),
                      vm_allocation_id=a.allocation_id, threads=1,
                      experiment_phase="v3-g2 DEV-CONTAINER SUBGRAPH PILOT (not a result)",
                      extra={"provenance_id": prov["provenance_id"], "subgraph_kind": kind})
        if not a.no_aa:
            shutil.rmtree(d / "AA_full")
        print(f"{kind} L={L} done", flush=True)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("--lengths", type=int, nargs="+", required=True)
    b.add_argument("--out", required=True)
    b.add_argument("--target-cpu", default="emeraldrapids")
    b.add_argument("--jobs", type=int, default=3)
    m = sub.add_parser("measure")
    m.add_argument("--lengths", type=int, nargs="+", required=True)
    m.add_argument("--kinds", nargs="+", default=["K", "Q"])
    m.add_argument("--out", required=True)
    for k in ("calls", "reps", "warmup-calls", "processes", "blocks", "seed", "cpu"):
        m.add_argument(f"--{k}", type=int, required=True)
    m.add_argument("--allocation-id", default="dev-container")
    m.add_argument("--no-aa", action="store_true")
    a = ap.parse_args()
    {"build": cmd_build, "measure": cmd_measure}[a.cmd](a)


if __name__ == "__main__":
    main()

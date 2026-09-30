#!/usr/bin/env python3
"""R1 (E-R1-0/1/4): intent-to-binary fidelity of math lowering, one ONNX op at a time.

For every op in OPS and every compiler variant (orig, r1a, r1b, r1c) this compiles a one-node ONNX model
([1, 64, 3072] float32, the Gelu tensor size of BERT at L=64) and records
  - the opt-report SIMD claim for the op (`==SIMD-REPORT==` lines of --opt-report=Simd),
  - the libm call sites left in the final .so (objdump: calls to <sym@plt> for known libm math symbols),
  - numerics in a fresh process: max abs / max relative error against a float64 numpy reference and
    against ONNX Runtime on the same input (descriptive, no threshold; protocol_r1.json).
Deterministic except the numerics, which are exact functions of the fixed seeded input. Nothing is timed.

  python scripts/r1_math_scan.py --out results/r1/math_scan [--ops Tanh Gelu_tanh ...]
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf.util import REPO_ROOT, sha256_file, write_json  # noqa: E402

VARIANTS = {k: f"/home/user/work/variants/{k}" for k in ("orig", "r1a", "r1b", "r1c")}
FLAGS = ["-O3", "--march=x86-64", "--mcpu=emeraldrapids", "--opt-report=Simd"]
SHAPE = [1, 64, 3072]
LIBM = ["tanhf", "powf", "expf", "exp2f", "erff", "erfcf", "logf", "log2f", "log1pf", "sinf", "cosf", "tanf",
        "atanf", "atan2f", "asinf", "acosf", "sqrtf", "cbrtf", "expm1f", "sinhf", "coshf", "exp10f"]


def _erf(x):
    from scipy.special import erf
    return erf(x)


# name: (onnx op, attrs, input domain, float64 reference)
OPS = {
    "Tanh": ("Tanh", {}, "wide", np.tanh),
    "Erf": ("Erf", {}, "wide", _erf),
    "Exp": ("Exp", {}, "exp", np.exp),
    "Log": ("Log", {}, "positive", np.log),
    "Sigmoid": ("Sigmoid", {}, "wide", lambda x: 1.0 / (1.0 + np.exp(-x))),
    "Softmax": ("Softmax", {"axis": -1}, "wide",
                lambda x: np.exp(x - x.max(-1, keepdims=True)) / np.exp(x - x.max(-1, keepdims=True)).sum(-1, keepdims=True)),
    "Gelu_tanh": ("Gelu", {"approximate": "tanh"}, "wide",
                  lambda x: 0.5 * x * (1 + np.tanh(np.sqrt(2 / np.pi) * (x + 0.044715 * x ** 3)))),
    "Gelu_none": ("Gelu", {"approximate": "none"}, "wide", lambda x: 0.5 * x * (1 + _erf(x / np.sqrt(2)))),
    "Pow3": ("Pow", {}, "wide", lambda x: x ** 3),
    "Sin": ("Sin", {}, "trig", np.sin),
    "Cos": ("Cos", {}, "trig", np.cos),
    "Sqrt": ("Sqrt", {}, "positive", np.sqrt),
    "Atan": ("Atan", {}, "wide", np.arctan),
}


def make_input(domain, seed=20260930):
    rng = np.random.default_rng(seed)
    if domain == "positive":
        x = rng.uniform(1e-3, 50.0, SHAPE)
    elif domain == "exp":
        x = rng.uniform(-80.0, 80.0, SHAPE)
    elif domain == "trig":
        x = rng.uniform(-50.0, 50.0, SHAPE)
    else:                                  # BERT-like pre-activation range plus tails
        x = rng.normal(0.0, 3.0, SHAPE)
    return x.astype(np.float32)


def write_model(path, name):
    import onnx
    from onnx import TensorProto, helper, numpy_helper
    op, attrs, _dom, _ref = OPS[name]
    inputs, inits = ["x"], []
    if op == "Pow":
        inits.append(numpy_helper.from_array(np.array(3.0, np.float32), "three"))
        inputs.append("three")
    node = helper.make_node(op, inputs, ["y"], name=f"{name.lower()}_node", **attrs)
    g = helper.make_graph([node], f"single_{name}", [helper.make_tensor_value_info("x", TensorProto.FLOAT, SHAPE)],
                          [helper.make_tensor_value_info("y", TensorProto.FLOAT, SHAPE)], initializer=inits)
    m = helper.make_model(g, opset_imports=[helper.make_opsetid("", 20)])
    m.ir_version = 9
    onnx.checker.check_model(m)
    onnx.save(m, str(path))


def libm_calls(so):
    out = subprocess.run(["objdump", "-d", "--no-show-raw-insn", str(so)], capture_output=True, text=True).stdout
    counts = {}
    for sym in re.findall(r"call\s+[0-9a-f]+ <([A-Za-z0-9_]+)@plt>", out):
        if sym in LIBM:
            counts[sym] = counts.get(sym, 0) + 1
    return dict(sorted(counts.items()))


def simd_report(stdout):
    return [ln.strip() for ln in stdout.splitlines() if ln.startswith("==SIMD-REPORT==")]


def worker(spec):
    """Fresh process: run one compiled op on the saved input, save the output."""
    sys.path.insert(0, str(REPO_ROOT))
    from shapeperf import toolchain
    from shapeperf.identity import open_session
    x = np.load(spec["input"])
    sess = open_session(toolchain.import_pyruntime(), spec["so"], spec["tag"])
    y = sess.run([x])[0]
    np.save(spec["output"], y)
    return {"shape": list(y.shape), "dtype": str(y.dtype)}


def errors(y, ref):
    y64, ref64 = y.astype(np.float64), ref.astype(np.float64)
    finite = bool(np.isfinite(y).all())
    ad = np.abs(y64 - ref64)
    rel = ad / np.maximum(np.abs(ref64), 1e-30)
    ulp = ad / np.spacing(np.abs(ref.astype(np.float32))).astype(np.float64)
    return {"finite": finite, "max_abs": float(np.nanmax(ad)), "max_rel": float(np.nanmax(rel)),
            "max_ulp_f32": float(np.nanmax(ulp)), "mean_abs": float(np.nanmean(ad))}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(REPO_ROOT / "results/r1/math_scan"))
    ap.add_argument("--work", default="/home/user/work/r1/math_scan")
    ap.add_argument("--ops", nargs="+", default=list(OPS))
    ap.add_argument("--variants", nargs="+", default=list(VARIANTS))
    ap.add_argument("--worker", default=None)
    a = ap.parse_args()
    if a.worker:
        print(json.dumps(worker(json.loads(Path(a.worker).read_text()))))
        return
    import onnxruntime as ort
    out, work = Path(a.out), Path(a.work)
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    compilers = {v: {"path": f"{VARIANTS[v]}/bin/onnx-mlir", "sha256": sha256_file(f"{VARIANTS[v]}/bin/onnx-mlir")}
                 for v in a.variants}
    for name in a.ops:
        d = work / name
        d.mkdir(parents=True, exist_ok=True)
        write_model(d / "model.onnx", name)
        x = make_input(OPS[name][2])
        np.save(d / "input.npy", x)
        ref = OPS[name][3](x.astype(np.float64))
        so_opts = ort.SessionOptions()
        so_opts.intra_op_num_threads = 1
        y_ort = ort.InferenceSession(str(d / "model.onnx"), so_opts, providers=["CPUExecutionProvider"]).run(None, {"x": x})[0]
        ort_err = errors(y_ort, ref)
        for v in a.variants:
            vd = d / v
            vd.mkdir(exist_ok=True)
            tag = f"{name.lower()}_{v}"
            cmd = [compilers[v]["path"], *FLAGS, f"--tag={tag}", "-o", str(vd / "model"), "--EmitLib", str(d / "model.onnx")]
            p = subprocess.run(cmd, capture_output=True, text=True)
            row = {"op": name, "variant": v, "compiler_sha256": compilers[v]["sha256"], "compile_rc": p.returncode,
                   "command": cmd}
            if p.returncode != 0:
                row["stderr_tail"] = p.stderr[-800:]
                rows.append(row)
                print(name, v, "COMPILE FAILED", flush=True)
                continue
            so = vd / "model.so"
            row.update({"so_sha256": sha256_file(so), "simd_report": simd_report(p.stdout), "libm_calls": libm_calls(so)})
            spec = {"so": str(so), "tag": tag, "input": str(d / "input.npy"), "output": str(vd / "y.npy")}
            (vd / "spec.json").write_text(json.dumps(spec))
            w = subprocess.run([sys.executable, str(Path(__file__).resolve()), "--worker", str(vd / "spec.json")],
                               capture_output=True, text=True, cwd=REPO_ROOT)
            if w.returncode != 0:
                row["run_error"] = w.stderr[-800:]
            else:
                y = np.load(vd / "y.npy")
                row["vs_float64_reference"] = errors(y, ref)
                row["vs_ort"] = errors(y, y_ort)
            row["ort_vs_float64_reference"] = ort_err
            rows.append(row)
            print(f"{name:10s} {v:5s} libm={row['libm_calls']} simd={len(row['simd_report'])} "
                  f"relerr={row.get('vs_float64_reference', {}).get('max_rel', 'n/a')}", flush=True)
    write_json(out / "math_scan.json", {"shape": SHAPE, "flags": FLAGS, "compilers": compilers, "rows": rows,
                                        "note": "static counts and numerics only; nothing timed"})


if __name__ == "__main__":
    main()

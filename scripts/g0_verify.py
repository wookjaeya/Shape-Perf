#!/usr/bin/env python3
"""G0 functional verification of the built toolchain (spec §11 G0, §13 items 5, 7, 13).

Facts established here from real compiles (never from documentation alone):
  1. onnx-mlir --version and the pinned commits
  2. static specialization: --shapeInformation yields an entry point whose
     input signature has the requested static dims (PyRuntime input_signature)
     and a probe IR whose main_graph arguments are static memrefs
  3. SIMD machine model actually used: VL values in --opt-report=Simd for each
     candidate --march setting (x86-64 / unset / native). Facts only - the
     flag set is chosen by the researcher and recorded in preregistration.
  4. matmul/attention lowering path: opt-report records for MatMul/Gemm and the
     .so's dynamic imports (BLAS-like symbols => external library)
  5. correctness of the compiled length vs ONNX Runtime (validate_shapes.py)
Timing numbers produced here (compile wall time) are dev-environment facts, not
performance results.

  python scripts/g0_verify.py --target-cpu <cpu> --length 128
"""
import argparse
import collections
import json
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import signature as S  # noqa: E402
from shapeperf import toolchain  # noqa: E402
from shapeperf.compile import (compile_shape, expected_entry_dims, line_buffered, model_def,  # noqa: E402
                               model_node_names, shape_information)
from shapeperf.util import REPO_ROOT, git_head, write_json  # noqa: E402


def vl_histogram(stdout):
    recs = S.parse_opt_report(stdout)
    h = collections.Counter((r.get("applied"), r.get("value")) for r in recs if r.get("kind") == "SIMD")
    return {f"applied={a},VL={v}": n for (a, v), n in sorted(h.items(), key=str)}, recs


def raw_compile(model, length, flags, out, emit="--EmitMLIR"):
    """Direct onnx-mlir call for the --march comparison (outside flag sets)."""
    out.mkdir(parents=True, exist_ok=True)
    cmd = [str(toolchain.onnx_mlir_bin()), *flags, f"--shapeInformation={shape_information(model, length)}",
           "--opt-report=Simd", emit, "-o", str(out / "model"), model["abs_path"]]
    p = subprocess.run(line_buffered(cmd), capture_output=True, text=True)
    return cmd, p.returncode, p.stdout, p.stderr[-3000:]


def same(a, b):
    """Equality as evidence: None when either value is missing or corrupt."""
    if a is None or b is None or str(a).startswith("CORRUPT:") or str(b).startswith("CORRUPT:"):
        return None
    return a == b


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--target-cpu", default=os.environ.get("SHAPEPERF_TARGET_CPU"), required=False)
    ap.add_argument("--allow-native", action="store_true")
    ap.add_argument("--length", type=int, default=128)
    ap.add_argument("--determinism-lengths", type=int, nargs="*", default=[],
                    help="extra lengths probed twice to check signature determinism (in addition to --length)")
    ap.add_argument("--out", default=str(REPO_ROOT / "results/g0"))
    args = ap.parse_args()
    out = Path(args.out)
    work = REPO_ROOT / "results/compile/g0"
    m = model_def(args.model)
    rep = {"harness_commit": git_head(), "onnx_mlir_version": toolchain.onnx_mlir_version(),
           **toolchain.compiler_ids(), "model": args.model, "length": args.length,
           "target_cpu": args.target_cpu, "role": "dev functional verification"}

    # (spec §13 item 5) --shape-info (RunONNXModel.py, run-time input generation) vs
    # --shapeInformation (compiler): keep the installed version's --help texts as evidence
    out.mkdir(parents=True, exist_ok=True)
    h = subprocess.run([str(toolchain.onnx_mlir_bin()), "--help"], capture_output=True, text=True)
    (out / "onnx-mlir-help.txt").write_text(h.stdout + h.stderr)
    rr = subprocess.run([sys.executable, str(toolchain.work_dir() / "onnx-mlir/utils/RunONNXModel.py"), "--help"],
                        capture_output=True, text=True)
    (out / "RunONNXModel-help.txt").write_text(rr.stdout + rr.stderr)
    rep["help_evidence"] = {"onnx_mlir_has_shapeInformation": "--shapeInformation" in h.stdout,
                            "RunONNXModel_has_shape_info": "--shape-info" in rr.stdout,
                            "note": "static specialization is checked below from the compiled entry signature and IR"}

    # (2)(4) full compile with the default flag set
    full = compile_shape(args.model, args.length, "default", args.target_cpu, "full", work / "full",
                         allow_native=args.allow_native)
    rep["full_compile"] = {k: full.get(k) for k in ["command", "failure_type", "compile_wall_ns", "cpu_user_s",
                                                    "peak_rss_bytes", "artifact_hash", "artifact_bytes",
                                                    "constants_files", "ir_signature", "report_n_records",
                                                    "matmul_path", "matmul_path_evidence", "dynamic_imports",
                                                    "stderr_tail"]}
    probe = compile_shape(args.model, args.length, "default", args.target_cpu, "probe", work / "probe",
                          allow_native=args.allow_native)
    rep["probe_compile"] = {k: probe.get(k) for k in ["failure_type", "probe_wall_ns", "peak_rss_bytes",
                                                      "ir_signature", "ir_structure_signature", "entry_signature",
                                                      "stderr_tail"]}
    rep["probe_and_full_report_signature_equal"] = same(full.get("ir_signature"), probe.get("ir_signature"))
    # signature determinism: the same length probed again must give the same signature
    probe2 = compile_shape(args.model, args.length, "default", args.target_cpu, "probe", work / "probe_repeat",
                           allow_native=args.allow_native)

    def determinism(p1, p2):
        return {"report_equal": same(p1.get("ir_signature"), p2.get("ir_signature")),
                "ir_structure_equal": same(p1.get("ir_structure_signature"), p2.get("ir_structure_signature")),
                "raw_ir_equal": same(p1.get("raw_ir_hash"), p2.get("raw_ir_hash")),
                "report_integrity": [p1.get("report_integrity"), p2.get("report_integrity")]}
    rep["signature_determinism"] = {**determinism(probe, probe2),
                                    "full_report_integrity": full.get("report_integrity"),
                                    "signature_version": probe.get("signature_version")}
    extra = {}
    for L in args.determinism_lengths:
        if L == args.length:
            continue
        a = compile_shape(args.model, L, "default", args.target_cpu, "probe", work / f"det_s{L}_a",
                          allow_native=args.allow_native, keep_ir=False)
        b = compile_shape(args.model, L, "default", args.target_cpu, "probe", work / f"det_s{L}_b",
                          allow_native=args.allow_native, keep_ir=False)
        extra[L] = determinism(a, b)
    rep["signature_determinism_other_lengths"] = extra
    rep["compiler_warnings"] = full.get("compiler_warnings")
    exp = expected_entry_dims(m, args.length)
    rep["expected_entry_dims"] = exp
    if probe.get("entry_signature"):
        rep["probe_ir_static"] = all("?" not in t for t in probe["entry_signature"])

    if not full.get("failure_type"):
        code = ("import sys, json; sys.path.insert(0, %r)\n"
                "from shapeperf import toolchain\n"
                "S = toolchain.import_pyruntime()(shared_lib_path=%r)\n"
                "print(json.dumps({'in': json.loads(S.input_signature()), 'out': json.loads(S.output_signature())}))"
                ) % (str(REPO_ROOT), full["artifact_path"])
        p = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True)
        if p.returncode == 0:
            sig = json.loads(p.stdout.strip().splitlines()[-1])
            rep["pyruntime_signature"] = sig
            rep["static_specialization_verified"] = [d.get("dims") for d in sig["in"]] == exp
        else:
            rep["pyruntime_error"] = p.stderr[-3000:]
        v = subprocess.run([sys.executable, str(REPO_ROOT / "validate_shapes.py"), "--artifact", full["artifact_path"],
                            "--model", args.model, "--length", str(args.length),
                            "--out", str(out / "validation.jsonl")], capture_output=True, text=True)
        rep["correctness"] = json.loads(v.stdout.strip().splitlines()[-1]) if v.returncode == 0 else v.stderr[-3000:]
        fs = S.final_artifact_features(full["artifact_path"])
        rep["final_artifact"] = {k: fs.get(k) for k in ["available", "hash", "calls", "register_class_counts"]}
        write_json(out / "matmul_path.json", {"matmul_path": full.get("matmul_path"),
                                              "evidence": full.get("matmul_path_evidence"),
                                              "dynamic_imports": full.get("dynamic_imports"),
                                              "flags": full.get("compile_flags"), "length": args.length})

    # (3) SIMD machine model per --march candidate, from the report itself
    cpu = [f"--mcpu={args.target_cpu}"] if args.target_cpu else []
    marches = {"march=x86-64": ["-O3", "--march=x86-64", *cpu], "march=unset": ["-O3", *cpu],
               "march=native": ["-O3", "--march=native"]}
    rep["simd_model_by_march"] = {}
    for name, flags in marches.items():
        cmd, rc, so, se = raw_compile(m, args.length, flags, work / name.replace("=", "_"))
        hist, recs = vl_histogram(so)
        rep["simd_model_by_march"][name] = {"command": cmd, "returncode": rc, "vl_histogram": hist,
                                            "n_simd_applied": sum(1 for r in recs if r.get("applied")),
                                            "report_signature": S.report_signature(
                                                recs, model_node_names(m["abs_path"]))["hash"],
                                            "stderr_tail": se if rc else None}
    write_json(out / "g0_verification.json", rep)
    print(json.dumps({"version": rep["onnx_mlir_version"][:200],
                      "full_failure": rep["full_compile"]["failure_type"],
                      "static_specialization_verified": rep.get("static_specialization_verified"),
                      "matmul_path": rep["full_compile"]["matmul_path"],
                      "probe_eq_full": rep["probe_and_full_report_signature_equal"],
                      "determinism": rep["signature_determinism"],
                      "correctness": rep.get("correctness", {}).get("correctness_status")
                      if isinstance(rep.get("correctness"), dict) else rep.get("correctness"),
                      "simd": {k: v["vl_histogram"] for k, v in rep["simd_model_by_march"].items()}}, indent=1))


if __name__ == "__main__":
    main()

"""Compile one (model, length, flag set) with the pinned ONNX-MLIR and collect
compile information and its cost (spec §6.1, §6.2, §8.2b, §8.4).

Two modes
  full   : --EmitLib shared library for timing + opt-report from the same run.
           The report adds no extra compile; its parsing time is charged as
           feature_extract_wall_ns (Compile-guided's marginal cost, §8.4).
  probe  : stops at the probe stage (configs/compile_flags.json "probe"), no
           link, no executable (§8.2b). Everything is charged as probe cost.
Cost of the compiler process tree is taken from wait4() rusage, so wall/CPU
time and the largest single-process RSS include opt/llc/linker children.
"""
import json
import os
import shutil
import signal
import subprocess
import time
from functools import lru_cache
from pathlib import Path

from . import signature as sigmod
from . import toolchain
from .util import REPO_ROOT, read_json, sha256_file, write_json


def model_def(model_key):
    m = read_json(REPO_ROOT / "configs/models.json")["models"][model_key]
    m = dict(m)
    m["key"] = model_key
    m["abs_path"] = str(REPO_ROOT / m["path"])
    return m


@lru_cache(maxsize=4)
def model_node_names(path):
    """ONNX node name -> op_type of the model file (sig-v2/v3 canonicalization)."""
    import onnx
    m = onnx.load(path, load_external_data=False)
    return {n.name: n.op_type for n in m.graph.node if n.name}


def line_buffered(cmd):
    """Run the compiler with line-buffered stdout so the C printf opt-report and
    LLVM's own stream cannot interleave inside a report line (seen at G0)."""
    sb = shutil.which("stdbuf")
    return ([sb, "-oL", "-eL"] + cmd) if sb else cmd


def shape_information(model, length, batch=1):
    """Build onnx-mlir --shapeInformation from the declared axes (spec §4.4:
    all related inputs get the same batch and length)."""
    parts = []
    for idx, inp in enumerate(model["inputs"]):
        dims = []
        for ax in inp["axes"]:
            if ax == "B":
                dims.append(str(batch))
            elif ax == "S":
                dims.append(str(length))
            else:
                dims.append(str(int(ax)))
        parts.append(f"{idx}:{'x'.join(dims)}")
    return ",".join(parts)


def expected_entry_dims(model, length, batch=1):
    return [[batch if a == "B" else length if a == "S" else int(a) for a in inp["axes"]]
            for inp in model["inputs"]]


def _run_tree(cmd, cwd, stdout_path, stderr_path, timeout_s):
    t0 = time.monotonic_ns()
    with open(stdout_path, "w") as fo, open(stderr_path, "w") as fe:
        p = subprocess.Popen(cmd, cwd=cwd, stdout=fo, stderr=fe, start_new_session=True)
        timed_out = False
        deadline = None if timeout_s is None else time.monotonic() + timeout_s
        while True:
            pid, status, ru = os.wait4(p.pid, os.WNOHANG)
            if pid != 0:
                break
            if deadline is not None and time.monotonic() > deadline:
                timed_out = True
                os.killpg(p.pid, signal.SIGKILL)
                pid, status, ru = os.wait4(p.pid, 0)
                break
            time.sleep(0.05)
    t1 = time.monotonic_ns()
    return {"returncode": os.waitstatus_to_exitcode(status), "timed_out": timed_out,
            "wall_ns": t1 - t0, "cpu_user_s": ru.ru_utime, "cpu_sys_s": ru.ru_stime,
            "peak_rss_bytes": ru.ru_maxrss * 1024}


def classify_failure(res, stderr_text):
    if res["timed_out"]:
        return "timeout"
    if res["returncode"] == 0:
        return None
    if res["returncode"] in (-9, 137) or "out of memory" in stderr_text.lower() \
            or "std::bad_alloc" in stderr_text:
        return "oom_or_killed"
    return "compile_error"


def compile_shape(model_key, length, flagset, target_cpu, mode, out_dir, batch=1,
                  allow_native=False, timeout_s=None, keep_ir=True):
    """Returns a record with §12 keys. Never raises on compiler failure; the
    failure is part of the record (spec §7: failures stay in the denominator)."""
    model = model_def(model_key)
    lo, hi = model["valid_lengths"]
    if not lo <= length <= hi:
        raise ValueError(f"length {length} outside valid range {model['valid_lengths']}")
    fs = toolchain.resolve_flagset(flagset, target_cpu, allow_native)
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    base = out_dir / "model"
    shape_info = shape_information(model, length, batch)
    cmd = [str(toolchain.onnx_mlir_bin()), *fs["flags"], f"--shapeInformation={shape_info}",
           "-o", str(base)]
    if fs["report"]:
        cmd.append(f"--opt-report={fs['report']}")
    if mode == "probe":
        pcfg = read_json(REPO_ROOT / "configs/compile_flags.json")["probe"]
        cmd += pcfg.get("print_flags", [])
        cmd.append(pcfg["emit"])
    elif mode == "full":
        cmd.append("--EmitLib")
    else:
        raise ValueError(mode)
    cmd.append(model["abs_path"])

    res = _run_tree(line_buffered(cmd), out_dir, out_dir / "stdout.txt", out_dir / "stderr.txt", timeout_s)
    stderr = (out_dir / "stderr.txt").read_text(errors="replace")
    stdout = (out_dir / "stdout.txt").read_text(errors="replace")
    failure = classify_failure(res, stderr)

    rec = {
        "model_key": model_key, "model_hash": sha256_file(model["abs_path"]),
        "padded_length": length, "batch": batch, "dtype": "float32",
        **toolchain.compiler_ids(), "target": target_cpu, "compile_flags": fs["flags"],
        "flagset": flagset, "shape_information": shape_info, "mode": mode,
        "command": cmd, "launcher": line_buffered([])[:3] or None,
        "returncode": res["returncode"], "failure_type": failure,
        "cpu_user_s": res["cpu_user_s"], "cpu_sys_s": res["cpu_sys_s"],
        "peak_rss_bytes": res["peak_rss_bytes"],
        "compile_wall_ns": res["wall_ns"] if mode == "full" else None,
        "probe_wall_ns": res["wall_ns"] if mode == "probe" else None,
    }
    if failure:
        rec["stderr_tail"] = stderr[-4000:]
        write_json(out_dir / "compile.json", rec)
        return rec

    # ---- information extraction (charged separately, §8.4) ----
    # feature_extract_wall_ns = what the primary signature costs (parse the
    # opt-report + hash); the model's node names are loaded before timing.
    # IR reading/structure/raw hashing is only needed by ablation 11.2-3 and is
    # timed per ablation (ir_structure_extract_wall_ns, raw_ir_extract_wall_ns);
    # the node-name load is recorded as node_names_load_ns (0 once cached).
    t_names = time.monotonic_ns()
    known = model_node_names(model["abs_path"])          # cached per process after the first call
    rec["node_names_load_ns"] = time.monotonic_ns() - t_names
    t0 = time.monotonic_ns()
    recs = sigmod.parse_opt_report(stdout)
    rsig = sigmod.report_signature(recs, known)
    rec["feature_extract_wall_ns"] = time.monotonic_ns() - t0
    rec["ir_signature"] = rsig["hash"] if rsig["integrity"] == "ok" else f"CORRUPT:{rsig['hash']}"
    rec["signature_version"] = rsig["version"]
    rec["signature_stage"] = f"opt-report:{fs['report']}" if fs["report"] else None
    rec["report_n_records"] = rsig["n_records"]
    rec["report_integrity"] = rsig["integrity"]
    rec["compiler_warnings"] = sorted({l.strip() for l in (stdout + stderr).splitlines()
                                       if l.strip().startswith("Warning:")})
    (out_dir / "report_signature.json").write_text(json.dumps(rsig, indent=1))
    if mode == "probe":
        # ablation signatures (11.2-3), each timed with its own IR read so every
        # ablation is charged only for the extraction it needs
        ir_path = Path(str(base) + ".onnx.mlir")
        if ir_path.exists():
            t1 = time.monotonic_ns()
            ir = ir_path.read_text(errors="replace")
            st = sigmod.ir_structure(ir)
            rec["ir_structure_extract_wall_ns"] = time.monotonic_ns() - t1
            t2 = time.monotonic_ns()
            ir = ir_path.read_text(errors="replace")
            rec["raw_ir_hash"] = sigmod.raw_ir_hash(ir)
            rec["raw_ir_extract_wall_ns"] = time.monotonic_ns() - t2
            rec["ir_structure_signature"] = st["hash"]
            rec["entry_signature"] = sigmod.entry_signature_from_ir(ir)     # diagnostic, not charged
            write_json(out_dir / "ir_structure.json", st)
            if not keep_ir:
                ir_path.unlink()
        else:
            rec["ir_structure_signature"] = None
            rec["raw_ir_hash"] = None
            rec["probe_ir_missing"] = str(ir_path)

    if mode == "full":
        so = Path(str(base) + ".so")
        rec["artifact_path"] = str(so)
        rec["artifact_hash"] = sha256_file(so) if so.exists() else None
        consts = list(out_dir.glob("model*.constants.bin"))
        rec["constants_files"] = [c.name for c in consts]
        rec["artifact_bytes"] = (so.stat().st_size if so.exists() else 0) + sum(c.stat().st_size for c in consts)
        imports = sigmod.dynamic_imports(so) if so.exists() else None
        rec["dynamic_imports"] = imports
        rec["matmul_path"], rec["matmul_path_evidence"] = sigmod.matmul_path(imports, recs)
    write_json(out_dir / "compile.json", rec)
    return rec


def final_signature(compile_rec):
    """Final-executable decisions for the probe/final mismatch check (G3).
    Not part of any selector's cost unless a selector asks for it."""
    so = compile_rec.get("artifact_path")
    if not so or not Path(so).exists():
        return None
    return sigmod.final_artifact_features(so)

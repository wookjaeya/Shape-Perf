#!/usr/bin/env python3
"""E0 of the follow-up (docs/followup_v3_review.md): preserve and inventory what the v3 L=64 kernel
comparison actually consisted of, WITHOUT changing results/v3.

Writes results/v3_followup/e0/:
  manifest.json          artifacts, inputs, compile/run argv (recorded or reconstructed, and which),
                         compilers, runtime, logs, timing records, repository state, missing items
  artifacts_L0064/       byte copies of the L=64 K cell (model.onnx, input/expected tensors, S8/S1
                         libraries and probe IR) so the E1 evidence survives the container
  repo_state/            `git diff --binary` and an archive of the untracked files at the time of this run
The audit of every v3 instrument's process model (which models each process loaded, in which order)
and the provenance-recovery attempt are merged from --audit (a JSON produced separately).

  python scripts/record_e0_followup.py --out results/v3_followup/e0 [--audit audit.json] [--recompile-check]
"""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import provenance, toolchain  # noqa: E402
from shapeperf.util import REPO_ROOT, read_json, sha256_file, write_json  # noqa: E402

CELL = Path("/home/user/work/v3/g2/K/L0064")
G2 = Path("/home/user/work/v3/g2")
ARMS = {"S8": "/home/user/work/variants/orig", "S1": "/home/user/work/variants/cap1"}
LOGS = ["v3_g2_build.log", "v3_g2_measure.log", "v3_g2_replicate.log", "v3_g1_pilot.log", "onnx-mlir.cap1.build.log",
        "onnx-mlir.revert.build.log"]


def _file(p):
    p = Path(p)
    if not p.exists():
        return {"path": str(p), "status": "missing"}
    return {"path": str(p), "sha256": sha256_file(p), "bytes": p.stat().st_size, "status": "present"}


def git(*args):
    return subprocess.run(["git", *args], cwd=REPO_ROOT, capture_output=True, text=True).stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--audit", default=None)
    ap.add_argument("--recompile-check", action="store_true")
    a = ap.parse_args()
    out = Path(a.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    prov = provenance.freeze(out / "provenance.json", compiler_builds=list(ARMS.values()),
                             extra={"role": "followup-e0-inventory"})

    # 1. preserve the L=64 cell
    keep = out / "artifacts_L0064"
    names = ["model.onnx", "input.npy", "expected.npy", "build.json", "S8_full/model.so", "S1_full/model.so",
             "S8_probe/model.onnx.mlir", "S1_probe/model.onnx.mlir"]
    cell = {}
    for n in names:
        src = CELL / n
        cell[n] = _file(src)
        if src.exists():
            (keep / n).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, keep / n)
    build = read_json(CELL / "build.json")
    x = np.load(CELL / "input.npy")
    cell["input_bytes_sha256_matches_build_json"] = hashlib.sha256(x.tobytes()).hexdigest() == build["input_sha256"]
    cell["so_sha256_matches_build_json"] = {arm: cell[f"{arm}_full/model.so"].get("sha256") == build["so_hash"][arm]
                                            for arm in ("S8", "S1")}
    cell["AA_full/model.so"] = {"status": "missing (by design)", "why": "cmd_measure copied S8_full/model.so to "
                                "AA_full/model.so before each cell and deleted it afterwards (scripts/run_subgraph_"
                                "benchmark.py cmd_measure); the copy is byte-identical to S8 by construction, and its "
                                "per-record artifact_hash equals the S8 hash (field artifact_hash of the AA records)"}

    # 2. compile argv: not recorded in v3 (build.json has no command) -> reconstructed; optionally re-run
    fs = toolchain.resolve_flagset("default", "emeraldrapids")
    argv = {arm: [str(toolchain.onnx_mlir_bin(b)), *fs["flags"], "-o", f"{CELL}/{arm}_full/model", "--EmitLib",
                  f"{CELL}/model.onnx"] for arm, b in ARMS.items()}
    compile_info = {"status": "reconstructed, not recorded",
                    "source": "scripts/run_subgraph_benchmark.py compile_arm at the build commit "
                              f"{read_json(G2 / 'provenance_build.json')['source']['git_head']} (clean tree)",
                    "argv": argv, "tag": "none (--tag absent): the compiler derives tag 'model' from -o",
                    "compilers": {arm: toolchain.compiler_identity(b) for arm, b in ARMS.items()}}
    if a.recompile_check:
        scratch = Path("/home/user/work/v3_followup/e0_recompile")
        shutil.rmtree(scratch, ignore_errors=True)
        chk = {}
        for arm in ARMS:
            d = scratch / f"{arm}_full"
            d.mkdir(parents=True)
            shutil.copyfile(CELL / "model.onnx", scratch / "model.onnx")
            cmd = argv[arm][:-4] + ["-o", str(d / "model"), "--EmitLib", str(scratch / "model.onnx")]
            p = subprocess.run(cmd, capture_output=True, text=True)
            got = sha256_file(d / "model.so") if (d / "model.so").exists() else None
            chk[arm] = {"argv": cmd, "returncode": p.returncode, "sha256": got,
                        "equals_recorded_so_hash": got == build["so_hash"][arm]}
        compile_info["recompile_check"] = chk

    # 3. run argv (fresh-process timing) - reconstructed from code + per-record fields
    run_info = {"status": "reconstructed from code; per-process fields are in the records",
                "argv": "[python, -m, shapeperf.paired, --worker, <item json>] via shapeperf.measure.run_item "
                        "(subprocess, a new exec per process; one artifact per process)",
                "drivers": {n: _file(f"/home/user/work/v3/{n}") for n in ("run_g2_measure.sh", "run_g2_replicate.sh")},
                "worker": "shapeperf.paired.subgraph_worker: load 1 artifact (no tag) -> 1 correctness call -> "
                          "warmup_calls calls -> reps x (calls consecutive calls, timed as one interval); every "
                          "output kept alive until the next call returns"}

    # 4. records, logs, runtime, repository state
    records = {p.name: _file(p) for p in sorted((REPO_ROOT / "results/v3/g2/timing").glob("*"))}
    records.update({p.name: _file(p) for p in sorted((REPO_ROOT / "results/v3/g2/diagnostics").glob("*"))})
    logs = {n: _file(f"/home/user/work/logs/{n}") for n in LOGS}
    (out / "logs").mkdir(exist_ok=True)
    for n in ("v3_g2_build.log", "v3_g2_measure.log", "v3_g2_replicate.log"):
        if Path(f"/home/user/work/logs/{n}").exists():
            shutil.copyfile(f"/home/user/work/logs/{n}", out / "logs" / n)
    pyrt = sorted(toolchain.pyruntime_dir().glob("PyRuntimeC*.so"))
    runtime = {"pyruntime": [_file(p) | {"mtime_unix": p.stat().st_mtime} for p in pyrt],
               "loader": "dlopen(RTLD_LAZY | RTLD_GLOBAL); entry via dlsym(handle, 'run_main_graph_<tag>') "
                         "(onnx-mlir src/Runtime/ExecutionSession.cpp at 1e017c9f, lines 88-132, 238-247)"}
    rs = out / "repo_state"
    rs.mkdir(exist_ok=True)
    (rs / "git_diff.binary.patch").write_text(git("diff", "--binary"))
    untracked = [u for u in git("ls-files", "--others", "--exclude-standard").splitlines()
                 if not u.startswith("results/v3_followup/")]
    if untracked:           # an archive, not loose files: copies of tests would be collected by pytest
        with tarfile.open(rs / "untracked.tar.gz", "w:gz") as tar:
            for u in untracked:
                tar.add(REPO_ROOT / u, arcname=u)
    repo = {"HEAD": git("rev-parse", "HEAD").strip(), "diff_file": "repo_state/git_diff.binary.patch",
            "untracked_archive": "repo_state/untracked.tar.gz" if untracked else None,
            "untracked_files": {u: sha256_file(REPO_ROOT / u) for u in untracked},
            "dependency_lock": {n: _file(f"/home/user/work/{n}") for n in ("requirements.freeze.txt",
                                                                           "requirements-export.freeze.txt")}}
    audit = json.loads(Path(a.audit).read_text()) if a.audit else None
    missing = [
        "compile argv of the v3 L=64 build (reconstructed from code; see compile.recompile_check)",
        "AA_full/model.so used during timing (deleted by design; byte copy of S8)",
        "per-process stdout/stderr of the timing workers (only the driver logs exist)",
        "PyRuntime SHA-256 at measurement time (not recorded by v3 provenance; current file and mtime recorded)",
        "the exact tree state of dirty v3 runs, unless recovered in the audit (see audit.provenance)",
    ]
    write_json(out / "manifest.json", {
        "purpose": "follow-up E0: inventory of the v3 L=64 kernel comparison; results/v3 is not modified",
        "provenance_id": prov["provenance_id"], "cell": {"dir": str(CELL), "files": cell, "build_json": build,
                                                         "preserved_copy": str(keep.relative_to(REPO_ROOT))},
        "compile": compile_info, "run": run_info, "records": records, "logs": logs, "runtime": runtime,
        "repository": repo, "audit": audit, "missing": missing})
    print("wrote", out / "manifest.json")


if __name__ == "__main__":
    main()

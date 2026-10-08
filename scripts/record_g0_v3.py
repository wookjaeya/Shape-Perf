#!/usr/bin/env python3
"""Record the G0 restoration evidence of design amendment v3 (docs/research_plan_v3.md §10 G0).

Facts recorded (nothing is judged here):
  - the ONNX-MLIR source tree used for the build is at the pinned commit and has no local change
  - SHA-256 of three compiler binaries: the restored main build tree, the saved S8 copy, the saved S1 copy
  - SHA-256 of the patch, and the `git apply --check` result of the patch against the clean tree
  - the outcome of `check-mlir` parsed from its log

  python scripts/record_g0_v3.py --onnx-mlir <src tree> --orig <bin> --cap1 <bin> --check-mlir-log <log> --out <json>
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf.util import REPO_ROOT, sha256_file, write_json  # noqa: E402


def git(src, *args):
    p = subprocess.run(["git", "-C", str(src), *args], capture_output=True, text=True)
    return p.returncode, p.stdout.strip(), p.stderr.strip()


def parse_lit(log):
    text = Path(log).read_text()
    counts = {m.group(1).strip(): int(m.group(2)) for m in re.finditer(r"^\s+([A-Za-z ]+?)\s*:\s+(\d+) \(", text, re.M)}
    total = re.search(r"Total Discovered Tests:\s+(\d+)", text)
    start, end = re.search(r"^start (\S+)", text, re.M), re.search(r"^end (\S+)", text, re.M)
    exit_code = re.search(r"^exit=(\d+)", text, re.M)
    return {"log_sha256": sha256_file(log), "start": start and start.group(1), "end": end and end.group(1),
            "exit_code": int(exit_code.group(1)) if exit_code else None,
            "total_discovered": int(total.group(1)) if total else None, "counts": counts,
            "header": re.search(r"-- Testing: .*", text).group(0) if re.search(r"-- Testing: .*", text) else None}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--onnx-mlir", required=True)
    ap.add_argument("--orig", required=True)
    ap.add_argument("--cap1", required=True)
    ap.add_argument("--check-mlir-log", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    src = Path(a.onnx_mlir)
    patch = REPO_ROOT / "patches/transpose_unroll_cap1.patch"
    main_bin = src / "build/Release/bin/onnx-mlir"
    _, head, _ = git(src, "rev-parse", "HEAD")
    _, status, _ = git(src, "status", "--porcelain")
    rc, _, err = git(src, "apply", "--check", str(patch))
    sha = {"restored_main_build": sha256_file(main_bin), "saved_S8": sha256_file(a.orig), "saved_S1": sha256_file(a.cap1)}
    write_json(a.out, {
        "labels": {"environment": "development container", "not_a_result": True},
        "source": {"path": str(src), "HEAD": head, "porcelain_status": status, "clean": status == ""},
        "patch": {"file": "patches/transpose_unroll_cap1.patch", "sha256": sha256_file(patch),
                  "applies_to_clean_tree": rc == 0, "apply_check_stderr": err},
        "compiler_binary_sha256": sha,
        "restored_build_equals_S8": sha["restored_main_build"] == sha["saved_S8"],
        "S1_differs_from_S8": sha["saved_S1"] != sha["saved_S8"],
        "check_mlir": parse_lit(a.check_mlir_log),
        "check_mlir_scope": "LLVM/MLIR project tests of the pinned LLVM build (target check-mlir); "
                            "the ONNX-MLIR lit tests are check-onnx-lit (v2 results/g0)"})
    print("wrote", a.out)


if __name__ == "__main__":
    main()

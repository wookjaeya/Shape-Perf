"""Locate the pinned ONNX-MLIR build and describe it (spec §5.4, §12 keys)."""
import os
import re
import shlex
import subprocess
import sys
from functools import lru_cache
from pathlib import Path

from .util import REPO_ROOT, read_json


def work_dir():
    return Path(os.environ.get("SHAPEPERF_WORK", str(Path.home() / "shapeperf-work")))


def onnx_mlir_build():
    return Path(os.environ.get("SHAPEPERF_ONNX_MLIR_BUILD",
                               str(work_dir() / "onnx-mlir/build/Release")))


def onnx_mlir_bin():
    return onnx_mlir_build() / "bin" / "onnx-mlir"


def pyruntime_dir():
    return onnx_mlir_build() / "lib"


def import_pyruntime():
    """Import OMExecutionSession from the pinned build (R5)."""
    d = str(pyruntime_dir())
    if d not in sys.path:
        sys.path.insert(0, d)
    from PyRuntime import OMExecutionSession  # noqa: WPS433
    return OMExecutionSession


def toolchain_pins():
    env = {}
    for line in (REPO_ROOT / "env/toolchain.env").read_text().splitlines():
        m = re.match(r"^([A-Z_]+)=(.*)$", line.strip())
        if m:
            env[m.group(1)] = m.group(2)
    return env


@lru_cache(maxsize=1)
def onnx_mlir_version():
    try:
        out = subprocess.run([str(onnx_mlir_bin()), "--version"], capture_output=True,
                             text=True, timeout=60)
        return (out.stdout + out.stderr).strip()
    except Exception as e:
        return f"unavailable: {e!r}"


def compiler_ids():
    pins = toolchain_pins()
    return {"compiler_commit": pins.get("ONNX_MLIR_SHA", "unavailable"),
            "llvm_commit": pins.get("LLVM_SHA", "unavailable")}


def resolve_flagset(name, target_cpu, allow_native=False):
    cfg = read_json(REPO_ROOT / "configs/compile_flags.json")
    fs = cfg["flagsets"][name]
    if not target_cpu:
        raise ValueError("target CPU must be given explicitly (--target-cpu / SHAPEPERF_TARGET_CPU)")
    if target_cpu == "native" and not allow_native:
        raise ValueError("--mcpu=native refused: artifacts would silently depend on the build host "
                         "(spec §5.4 step 6); pass --allow-native for smoke tests only")
    flags = [f.replace("${TARGET_CPU}", target_cpu) for f in fs["flags"]]
    return {"name": name, "flags": flags, "report": fs.get("report"),
            "flags_str": " ".join(shlex.quote(f) for f in flags), "target_cpu": target_cpu}

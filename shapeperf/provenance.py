"""Immutable run provenance (design amendment v3, G0).

`freeze()` is called ONCE at the start of a run. It records what actually determines the
results - source state, dependencies, compiler binaries, host - and returns a content id that
every raw record of the run carries (`provenance_id`). Nothing is re-read later, so a commit or
a package upgrade during a long run cannot relabel earlier records.

The source state is the HEAD plus a hash of the working-tree diff and of untracked, non-ignored
files: two runs with the same id ran the same code.
"""
import hashlib
import json
import os
import platform
import subprocess
import sys
import time
from importlib import metadata
from pathlib import Path

from . import toolchain
from .util import REPO_ROOT, write_json


def _git(*args):
    try:
        return subprocess.run(["git", "-C", str(REPO_ROOT), *args], capture_output=True, text=True,
                              check=True).stdout
    except Exception:
        return None


def source_state():
    head = (_git("rev-parse", "HEAD") or "unavailable").strip()
    diff = _git("diff", "HEAD")
    untracked = sorted((_git("ls-files", "--others", "--exclude-standard") or "").split())
    h = hashlib.sha256()
    for f in untracked:
        try:
            h.update(f.encode() + b"\0" + (REPO_ROOT / f).read_bytes())
        except OSError:
            h.update(f.encode() + b"\0unreadable")
    return {"git_head": head,
            "dirty": bool(diff) or bool(untracked),
            "diff_sha256": hashlib.sha256((diff or "").encode()).hexdigest(),
            "untracked_files": untracked, "untracked_sha256": h.hexdigest()}


def dependencies():
    """name==version of every installed distribution of THIS interpreter, and one hash of the list."""
    pkgs = sorted(f"{d.metadata['Name']}=={d.version}" for d in metadata.distributions()
                  if d.metadata["Name"])
    return {"python": sys.version.split()[0], "n_packages": len(pkgs),
            "packages_sha256": hashlib.sha256("\n".join(pkgs).encode()).hexdigest(),
            "key": {k: v for k, v in (p.split("==") for p in pkgs)
                    if k.lower() in ("numpy", "onnx", "onnxruntime", "scipy", "protobuf")}}


def host():
    def rd(path):
        try:
            return Path(path).read_text().strip()
        except OSError:
            return "unavailable"
    cpu = "unavailable"
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                cpu = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    return {"kernel": platform.release(), "machine": platform.machine(), "cpu_model": cpu,
            "n_cpus": os.cpu_count(), "node": platform.node(),
            "cfs_quota_us": rd("/sys/fs/cgroup/cpu.max")}


def freeze(out_path=None, compiler_builds=(), extra=None):
    """Snapshot now; write it to out_path (if given) and return {'provenance_id', ...}."""
    snap = {"created_unix": time.time(), "source": source_state(), "dependencies": dependencies(),
            "host": host(),
            "compilers": [toolchain.compiler_identity(b) | {"version": toolchain.onnx_mlir_version(b)}
                          for b in compiler_builds],
            "extra": extra or {}}
    body = {k: v for k, v in snap.items() if k not in ("created_unix",)}
    snap["provenance_id"] = hashlib.sha256(json.dumps(body, sort_keys=True, default=str).encode()).hexdigest()[:16]
    if out_path:
        write_json(out_path, snap)
    return snap

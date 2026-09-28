#!/usr/bin/env python3
"""Environment manifest for spec §5.2 (VM / OS / compiler / model / execution /
state layers). Anything that cannot be observed is written as "unavailable" -
never guessed. Provider/region/product/allocation are not observable from inside
a generic VM, so they are taken from CLI flags or left "unavailable".

  python scripts/collect_env.py --role dev --out environment.lock.json
  python scripts/collect_env.py --role measurement --provider X --region Y \
      --instance-type Z --allocation-id ID --out results/env/<id>.json
"""
import argparse
import glob
import json
import os
import platform
import re
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import toolchain  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, read_json, write_json  # noqa: E402

U = "unavailable"


def sh(cmd, timeout=60):
    try:
        p = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)
        out = (p.stdout or p.stderr).strip()
        return out if out else U
    except Exception:
        return U


def rd(path):
    try:
        return Path(path).read_text().strip()
    except Exception:
        return U


def lscpu_json():
    try:
        d = json.loads(sh("lscpu -J"))
        return {e["field"].rstrip(":"): e["data"] for e in d["lscpu"]}
    except Exception:
        return U


def cgroup_info():
    info = {"cgroup_self": rd("/proc/self/cgroup")}
    # v2
    info["cpu.max"] = rd("/sys/fs/cgroup/cpu.max")
    info["memory.max"] = rd("/sys/fs/cgroup/memory.max")
    info["cpuset.cpus.effective"] = rd("/sys/fs/cgroup/cpuset.cpus.effective")
    # v1
    info["cpu.cfs_quota_us"] = rd("/sys/fs/cgroup/cpu/cpu.cfs_quota_us")
    info["cpu.cfs_period_us"] = rd("/sys/fs/cgroup/cpu/cpu.cfs_period_us")
    info["memory.limit_in_bytes"] = rd("/sys/fs/cgroup/memory/memory.limit_in_bytes")
    info["cpuset.cpus"] = rd("/sys/fs/cgroup/cpuset/cpuset.cpus")
    return info


def container_runtime():
    if Path("/.dockerenv").exists():
        return "docker (/.dockerenv present)"
    if Path("/run/.containerenv").exists():
        return "podman (/run/.containerenv present)"
    virt = sh("systemd-detect-virt --container 2>/dev/null")
    return virt if virt not in ("none", U) else "none detected / " + U


def proc_stat_steal():
    line = rd("/proc/stat").splitlines()[0] if rd("/proc/stat") != U else ""
    parts = line.split()
    return {"cpu_total_steal_ticks": int(parts[8]) if len(parts) > 8 else U}


def cpufreq():
    govs = sorted({rd(p) for p in glob.glob("/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor")})
    return {"governors": govs or U,
            "intel_pstate_no_turbo": rd("/sys/devices/system/cpu/intel_pstate/no_turbo"),
            "boost": rd("/sys/devices/system/cpu/cpufreq/boost"),
            "smt_active": rd("/sys/devices/system/cpu/smt/active"),
            "thermal_throttle_core_count_cpu0": rd("/sys/devices/system/cpu/cpu0/thermal_throttle/core_throttle_count")}


def model_layer():
    out = {}
    lock_p = REPO_ROOT / "artifacts.lock.json"
    if lock_p.exists():
        lock = read_json(lock_p)["artifacts"]
        out["artifacts"] = {k: {"sha256": v["sha256"], "url_used": v["url_used"]} for k, v in lock.items()}
    for meta in glob.glob(str(REPO_ROOT / "data/artifacts/derived/*.onnx.json")):
        m = read_json(meta)
        out.setdefault("derived", {})[Path(meta).name[:-5]] = {
            k: m.get(k) for k in ["output_sha256", "source_sha256", "exporter", "tensorflow", "opset", "label"]}
    cat = REPO_ROOT / "data/features/A/catalog.json"
    if cat.exists():
        out["preprocessing"] = read_json(cat)["provenance"]
    return out or U


def compiler_layer():
    pins = toolchain.toolchain_pins()
    work = toolchain.work_dir()
    man = work / "build_manifest.json"
    d = {"pins": pins, "onnx_mlir_bin": str(toolchain.onnx_mlir_bin()),
         "onnx_mlir_version": toolchain.onnx_mlir_version() if toolchain.onnx_mlir_bin().exists() else "not built",
         "build_manifest": read_json(man) if man.exists() else "not built"}
    omp = glob.glob(str(work / "llvm-project/build/**/libomp.so*"), recursive=True)
    d["openmp_runtime"] = sorted(omp)[:4] or "not found"
    d["cc"] = sh("clang --version | head -1")
    d["gcc"] = sh("gcc --version | head -1")
    g0 = REPO_ROOT / "results/g0/matmul_path.json"
    d["matmul_lowering_path"] = read_json(g0) if g0.exists() else "not yet determined (G0: needs a real compile)"
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--role", choices=["dev", "measurement"], required=True)
    ap.add_argument("--provider", default=U)
    ap.add_argument("--region", default=U)
    ap.add_argument("--instance-type", default=U)
    ap.add_argument("--allocation-id", default=U)
    ap.add_argument("--allocation-created", default=U)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    venv_py = toolchain.work_dir() / "venv/bin/python"
    m = {
        "role": args.role,
        "note": ("development environment: functional checks only, not a controlled measurement VM (spec §5.1)"
                 if args.role == "dev" else "measurement VM"),
        "collected_unix": time.time(),
        "harness_commit": git_head(),
        "vm": {"provider": args.provider, "region": args.region, "instance_type": args.instance_type,
               "allocation_id": args.allocation_id, "allocation_created": args.allocation_created,
               "vcpus": os.cpu_count(), "mem_total": sh("grep MemTotal /proc/meminfo"),
               "lscpu": lscpu_json(), "numa": sh("lscpu | grep -i numa"),
               "hypervisor": sh("lscpu | grep -i 'hypervisor vendor'"),
               "virtualization": sh("systemd-detect-virt 2>/dev/null"),
               "cpu_flags_avx": sorted({f for f in rd("/proc/cpuinfo").split() if f.startswith("avx")})},
        "os": {"os_release": rd("/etc/os-release"), "kernel": platform.release(),
               "glibc": " ".join(platform.libc_ver()), "container_runtime": container_runtime(),
               "cgroup": cgroup_info(), "image_id": os.environ.get("SHAPEPERF_IMAGE_ID", U),
               "transparent_hugepage": rd("/sys/kernel/mm/transparent_hugepage/enabled")},
        "compiler": compiler_layer(),
        "python": {"executable": sys.executable, "version": sys.version,
                   "venv_freeze": sh(f"{venv_py} -m pip freeze --all") if venv_py.exists() else U},
        "model": model_layer(),
        "execution": {"affinity": sorted(os.sched_getaffinity(0)),
                      "thread_env": {k: os.environ.get(k, "unset") for k in
                                     ["OMP_NUM_THREADS", "OMP_PROC_BIND", "OMP_PLACES", "OMP_WAIT_POLICY"]},
                      "perf_counter": str(time.get_clock_info("perf_counter"))},
        "state": {"loadavg": os.getloadavg(), "swap": sh("grep -E 'SwapTotal|SwapFree' /proc/meminfo"),
                  **proc_stat_steal(), **cpufreq(),
                  "running_processes_top": sh("ps -eo pcpu,comm --sort=-pcpu | head -8")},
    }
    write_json(args.out, m)
    print("wrote", args.out)


if __name__ == "__main__":
    main()

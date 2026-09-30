"""Uninstrumented warm steady-state latency of one compiled shape (spec §6.1, §6.3,
§10). One call of `worker_main` = one fresh process = one process-level unit.

Timing boundary (identical for every shape and configuration):
    start = perf_counter_ns(); outputs = session.run(inputs); stop = perf_counter_ns()
Session loading, input construction and tokenization happen before timing.
Every output list is kept alive until the next call returns, so allocation /
release policy is the same for every iteration.

Nothing here decides warmup/iteration counts: they are required inputs fixed
by the G3 pilot and the preregistration (spec §10.2).
"""
import hashlib
import json
import os
import platform
import resource
import subprocess
import sys
import time
from pathlib import Path

import numpy as np

from .util import REPO_ROOT, append_jsonl, git_head, new_run_id

THREAD_ENV_KEYS = ["OMP_NUM_THREADS", "OMP_PROC_BIND", "OMP_PLACES", "OMP_WAIT_POLICY",
                   "KMP_AFFINITY", "KMP_BLOCKTIME", "MKL_NUM_THREADS", "OPENBLAS_NUM_THREADS"]


def _read(path, default="unavailable"):
    try:
        return Path(path).read_text().strip()
    except Exception:
        return default


def _proc_stat_cpu(cpus):
    """user,nice,system,idle,iowait,irq,softirq,steal per cpu from /proc/stat."""
    out = {}
    for line in _read("/proc/stat", "").splitlines():
        parts = line.split()
        if parts and parts[0].startswith("cpu") and parts[0] != "cpu":
            c = int(parts[0][3:])
            if c in cpus:
                out[c] = [int(v) for v in parts[1:9]]
    return out


def _cpu_mhz(cpus):
    mhz = {}
    for c in cpus:
        v = _read(f"/sys/devices/system/cpu/cpu{c}/cpufreq/scaling_cur_freq", None)
        if v is not None:
            mhz[c] = int(v) / 1000.0
    if mhz:
        return mhz
    cur = None
    for line in _read("/proc/cpuinfo", "").splitlines():
        if line.startswith("processor"):
            cur = int(line.split(":")[1])
        elif line.startswith("cpu MHz") and cur in cpus:
            mhz[cur] = float(line.split(":")[1])
    return mhz or "unavailable"


def _threads():
    try:
        return len(os.listdir("/proc/self/task"))
    except Exception:
        return "unavailable"


def _out_hash(outs):
    h = hashlib.sha256()
    for o in outs:
        a = np.ascontiguousarray(o)
        h.update(str(a.dtype).encode() + str(a.shape).encode())
        h.update(a.tobytes())
    return h.hexdigest()[:16]


def worker_main(spec):
    """Run inside a fresh process. spec keys: artifact, model_key, length,
    feature_index, warmup, iterations, cpus (list or None)."""
    from . import toolchain
    from .compile import model_def, expected_entry_dims
    from .inputs import inputs_for
    from .squad import load_features

    if spec.get("cpus"):
        os.sched_setaffinity(0, set(spec["cpus"]))
    cpus = sorted(os.sched_getaffinity(0))
    model = model_def(spec["model_key"])
    feat = load_features(REPO_ROOT / model["features"] / "features.npz")
    arrs, _ = inputs_for(model, feat, spec["feature_index"], spec["length"])

    OMExecutionSession = toolchain.import_pyruntime()
    t_load0 = time.monotonic_ns()
    sess = (OMExecutionSession(shared_lib_path=spec["artifact"], tag=spec["tag"]) if spec.get("tag")
            else OMExecutionSession(shared_lib_path=spec["artifact"]))
    load_ns = time.monotonic_ns() - t_load0
    try:
        in_sig = json.loads(sess.input_signature())
        declared = [d.get("dims") for d in in_sig]
    except Exception as e:
        in_sig, declared = f"unavailable: {e!r}", None
    expected = expected_entry_dims(model, spec["length"])
    static_ok = (declared == expected) if declared is not None else None

    ru0 = resource.getrusage(resource.RUSAGE_SELF)
    stat0, mhz0, load0 = _proc_stat_cpu(cpus), _cpu_mhz(cpus), os.getloadavg()
    t_w0 = time.monotonic_ns()
    outs = None
    for _ in range(spec["warmup"]):
        outs = sess.run(arrs)
    warmup_ns = time.monotonic_ns() - t_w0
    threads_after_warmup = _threads()
    first_hash = _out_hash(outs) if outs is not None else None

    samples = []
    stable = True
    perf = time.perf_counter_ns
    t_m0 = time.monotonic_ns()
    for _ in range(spec["iterations"]):
        start = perf()
        new = sess.run(arrs)
        stop = perf()
        samples.append(stop - start)
        outs = new
        if first_hash is None:          # warmup == 0: compare against the first timed output
            first_hash = _out_hash(outs)
    measurement_ns = time.monotonic_ns() - t_m0
    last_hash = _out_hash(outs)
    if first_hash is not None and last_hash != first_hash:
        stable = False
    ru1 = resource.getrusage(resource.RUSAGE_SELF)
    stat1, mhz1, load1 = _proc_stat_cpu(cpus), _cpu_mhz(cpus), os.getloadavg()
    steal = {c: stat1[c][7] - stat0[c][7] for c in stat1 if c in stat0}
    return {
        "latency_ns": samples,
        "load_ns": load_ns, "warmup_wall_ns": warmup_ns, "measurement_wall_ns": measurement_ns,
        "input_signature": in_sig, "static_shape_verified": static_ok,
        "expected_input_dims": expected,
        "output_hash_first": first_hash, "output_hash_last": last_hash, "outputs_stable": stable,
        "threads_observed": threads_after_warmup, "cpus": cpus,
        "minflt": ru1.ru_minflt - ru0.ru_minflt, "majflt": ru1.ru_majflt - ru0.ru_majflt,
        "nvcsw": ru1.ru_nvcsw - ru0.ru_nvcsw, "nivcsw": ru1.ru_nivcsw - ru0.ru_nivcsw,
        "steal_ticks": steal, "cpu_mhz_before": mhz0, "cpu_mhz_after": mhz1,
        "loadavg_before": load0, "loadavg_after": load1,
        "clock": "time.perf_counter_ns", "pid": os.getpid(),
        "peak_rss_bytes": ru1.ru_maxrss * 1024,
    }


def worker_env(threads, extra=None):
    env = dict(os.environ)
    for k in THREAD_ENV_KEYS:
        env.pop(k, None)
    env["OMP_NUM_THREADS"] = str(threads)
    env["MKL_NUM_THREADS"] = "1"
    env["OPENBLAS_NUM_THREADS"] = "1"
    if extra:
        env.update(extra)
    return env


def run_item(item, env):
    """Spawn a fresh worker process for one plan item; returns its record. item['worker_module']
    selects another module with a `--worker <json>` entry point (default: this one)."""
    cmd = [sys.executable, "-m", item.get("worker_module", "shapeperf.measure"), "--worker", json.dumps(item)]
    t0 = time.monotonic_ns()
    p = subprocess.run(cmd, cwd=REPO_ROOT, env=env, capture_output=True, text=True,
                       timeout=item.get("timeout_s"))
    wall = time.monotonic_ns() - t0
    if p.returncode != 0:
        return {"failure_type": "runtime_error", "returncode": p.returncode,
                "stderr_tail": p.stderr[-4000:], "process_wall_ns": wall}
    rec = json.loads(p.stdout.strip().splitlines()[-1])
    rec["process_wall_ns"] = wall
    rec["failure_type"] = None
    return rec


def run_block(items, out_jsonl, seed, block_id=None, vm_allocation_id="unavailable",
              experiment_phase="dev-smoke", threads=1, extra=None):
    """Run plan items in a seeded random order within one block, each in a
    fresh process (spec §10.1). Raw records are appended, never rewritten.
    extra: fields added to every record (e.g. the G4 run id and role)."""
    rng = np.random.default_rng(seed)
    order = rng.permutation(len(items)).tolist()
    block_id = block_id or new_run_id("block")
    env = worker_env(threads)
    thread_env = {k: env.get(k) for k in THREAD_ENV_KEYS}
    common = {"block_id": block_id, "seed": seed, "vm_allocation_id": vm_allocation_id,
              "experiment_phase": experiment_phase, "harness_commit": git_head(),
              "host": platform.node(), "thread_config": {"threads": threads, "env": thread_env},
              **(extra or {})}
    results = []
    for pos, k in enumerate(order):
        item = items[k]
        rec = run_item(item, env)
        rec.update(common)
        rec.update({"run_id": new_run_id("meas"), "order_in_block": pos, "plan_index": k,
                    "artifact": item["artifact"], "artifact_hash": item.get("artifact_hash"),
                    "model_key": item["model_key"], "padded_length": item["length"],
                    "flagset": item.get("flagset", "default"),
                    "feature_index": item["feature_index"], "warmup": item["warmup"],
                    "iterations": item["iterations"], "unix_time": time.time()})
        append_jsonl(out_jsonl, rec)
        results.append(rec)
    return results


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--worker":
        print(json.dumps(worker_main(json.loads(sys.argv[2]))))
    else:
        print("internal entry point; use measure.py", file=sys.stderr)
        sys.exit(2)

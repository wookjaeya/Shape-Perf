"""Paired-contrast support for design amendment v3: input selection, output verification worker,
and the paired analysis (per length, per block: log ratio of arm means).

Unit of comparison: same length L, same input, same VM allocation, same block; arms run as fresh
processes in random order inside the block. a = log T_S8 - log T_S1 > 0 means the original unroll
policy is slower than the patched one. Iterations are never counted as independent samples: the
hierarchy is allocation -> block -> process -> iteration, and inference uses per-block values.
"""
import json
import math
import sys
from collections import defaultdict

import numpy as np
from scipy import stats as st


def natural_feature_index(feat, length):
    """First feature (by (qas_id, window_index)) whose natural length equals `length`: a
    timing-independent rule. None if the length does not occur."""
    idx = np.flatnonzero(np.asarray(feat["valid_length"]) == length)
    if idx.size == 0:
        return None
    return int(sorted(idx, key=lambda i: (str(feat["qas_id"][i]), int(feat["window_index"][i])))[0])


def verify_worker(spec):
    """Fresh process: load one compiled artifact, run one input twice, save all outputs."""
    from . import toolchain
    from .compile import expected_entry_dims, model_def
    from .inputs import inputs_for
    from .squad import load_features
    from .util import REPO_ROOT
    model = model_def(spec["model_key"])
    feat = load_features(REPO_ROOT / model["features"] / "features.npz")
    arrs, _ = inputs_for(model, feat, spec["feature_index"], spec["length"])
    sess = toolchain.import_pyruntime()(shared_lib_path=spec["artifact"])
    declared = [d.get("dims") for d in json.loads(sess.input_signature())]
    first = sess.run(arrs)
    second = sess.run(arrs)
    names = [d["name"] for d in json.loads(sess.output_signature())]
    np.savez(spec["out_npz"], **dict(zip(names, first)))
    return {"static_shape_verified": declared == expected_entry_dims(model, spec["length"]),
            "input_dims": declared, "output_names": names,
            "repeat_identical": all(np.array_equal(a, b) for a, b in zip(first, second)),
            "finite": all(bool(np.isfinite(a).all()) for a in first)}


def compare_outputs(a, b, valid_length, start_name, end_name):
    """S8-vs-S1 (bitwise) or arm-vs-reference (descriptive) comparison of two output dicts."""
    out = {"bitwise_identical": all(np.array_equal(a[k], b[k]) for k in a if k in b) and set(a) == set(b)}
    for key, name in (("start", start_name), ("end", end_name)):
        x, y = np.asarray(a[name])[0], np.asarray(b[name])[0]
        out[f"{key}_max_abs_valid"] = float(np.abs(x[:valid_length] - y[:valid_length]).max())
        out[f"{key}_argmax_equal"] = bool(int(np.argmax(x[:valid_length])) == int(np.argmax(y[:valid_length])))
    return out


def subgraph_worker(spec):
    """Fresh process: time one compiled single-op artifact with batch-of-calls (amendment §11.6).
    A sample is the mean time per call over `calls` consecutive calls; `reps` samples are taken
    after `warmup_calls`. Correctness (exact permutation) is checked once, outside the timed loops.
    spec: artifact, input_npy, expected_npy, calls, reps, warmup_calls, cpus."""
    import os
    import time
    from . import toolchain
    if spec.get("cpus"):
        os.sched_setaffinity(0, set(spec["cpus"]))
    x = np.load(spec["input_npy"])
    expected = np.load(spec["expected_npy"])
    sess = toolchain.import_pyruntime()(shared_lib_path=spec["artifact"])
    out = sess.run([x])
    exact = bool(len(out) == 1 and np.array_equal(out[0], expected) and out[0].dtype == expected.dtype)
    for _ in range(spec["warmup_calls"]):
        out = sess.run([x])
    perf = time.perf_counter_ns
    samples = []
    t_all = time.monotonic_ns()
    for _ in range(spec["reps"]):
        t0 = perf()
        for _ in range(spec["calls"]):
            out = sess.run([x])          # the previous output stays alive until this call returns
        samples.append((perf() - t0) / spec["calls"])
    return {"latency_ns": samples, "measurement_wall_ns": time.monotonic_ns() - t_all,
            "warmup_wall_ns": 0, "exact_permutation": exact, "calls_per_sample": spec["calls"],
            "cpus": sorted(os.sched_getaffinity(0)), "pid": os.getpid(), "clock": "time.perf_counter_ns",
            "static_shape_verified": None, "outputs_stable": True}


# ------------------------------------------------------------------ analysis

PROCESS_STATS = {"mean": np.mean, "median": np.median, "min": np.min}


def process_means(records, skip=0, stat="mean"):
    """{(length, block_id): {arm: [per-process summary of latency after `skip` iterations]}}.
    stat 'mean' is the primary process summary (amendment §11.1); 'median' and 'min' are
    secondary, robust to bursty interference."""
    f = PROCESS_STATS[stat]
    t = defaultdict(lambda: defaultdict(list))
    for r in records:
        if r.get("failure_type"):
            continue
        x = np.asarray(r["latency_ns"][skip:], float)
        if x.size:
            t[(r["padded_length"], r["block_id"])][r.get("flagset", "default")].append(float(f(x)))
    return t


def paired_log_ratios(table, length, arm_a, arm_b):
    """Per block: log(mean of arm_a process means) - log(mean of arm_b process means)."""
    return [math.log(np.mean(d[arm_a]) / np.mean(d[arm_b]))
            for (L, _b), d in table.items() if L == length and d.get(arm_a) and d.get(arm_b)]


def effect_summary(values, conf_level=0.95):
    """Mean, sd, t interval over blocks (one allocation: the claim is limited to it)."""
    x = np.asarray(values, float)
    n = int(x.size)
    if n < 2:
        return {"n_blocks": n, "mean_log": float(x.mean()) if n else None, "sd": None, "ci_log": None}
    sd = float(x.std(ddof=1))
    q = float(st.t.ppf(1 - (1 - conf_level) / 2, n - 1))
    m = float(x.mean())
    return {"n_blocks": n, "mean_log": m, "sd": sd, "ci_log": [m - q * sd / math.sqrt(n), m + q * sd / math.sqrt(n)],
            "relative_effect": math.expm1(m)}


def variance_components(records, skip=0):
    """Iteration (within process) and process (within length x arm x block) variance of log latency."""
    within, groups = [], defaultdict(list)
    for r in records:
        if r.get("failure_type"):
            continue
        x = np.log(np.asarray(r["latency_ns"][skip:], float))
        if x.size < 2:
            continue
        within.append(x.var(ddof=1))
        groups[(r["padded_length"], r.get("flagset"), r["block_id"])].append(x.mean())
    between = [np.var(v, ddof=1) for v in groups.values() if len(v) > 1]
    return {"iteration_var_log": float(np.mean(within)) if within else None,
            "process_var_log": float(np.mean(between)) if between else None,
            "n_processes": len(within)}


def required_blocks(sd_block, delta_log, alpha=0.05, power=0.8):
    """Blocks needed for a two-sided z-approximation of a paired mean to detect `delta_log`."""
    z = st.norm.ppf(1 - alpha / 2) + st.norm.ppf(power)
    return math.ceil((z * sd_block / abs(delta_log)) ** 2)


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--verify-worker":
        print(json.dumps(verify_worker(json.loads(sys.argv[2]))))
    elif len(sys.argv) == 3 and sys.argv[1] == "--worker":
        print(json.dumps(subgraph_worker(json.loads(sys.argv[2]))))
    else:
        print("internal entry point", file=sys.stderr)
        sys.exit(2)

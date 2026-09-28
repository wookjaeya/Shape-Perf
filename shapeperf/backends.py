"""Backends that answer broker queries.

SyntheticBackend - TESTS ONLY. Generated curves; never report its numbers as
                   results (spec §13 item 12).
ReplayBackend    - evaluator-side replay of dense per-length records (compile,
                   probe, extraction, verification and measurement costs, and
                   process-level latency samples) that were really measured.
                   Each query returns one recorded process chosen with a seeded
                   RNG and charges that record's actual cost.
LiveBackend      - really compiles, verifies and measures (G5 on a controlled VM).
"""
import math
import time
from pathlib import Path

import numpy as np


def usable_signature(sig):
    """A corrupt opt-report (compile.py 'CORRUPT:<hash>') is an extraction
    failure: selectors see it as the failure value 'FAILED:corrupt_report', like
    any other failed shape (preregistered failure rule), never as a real value."""
    if isinstance(sig, str) and sig.startswith("CORRUPT:"):
        return "FAILED:corrupt_report"
    return sig


class SyntheticBackend:
    """Latency(s) = base * s-trend * product of step factors, lognormal noise.
    Signatures change at `sig_changes` (sorted list of s where sig(s) != sig(s-1)).
    """
    synthetic = True

    def __init__(self, steps, sig_changes, seed=0, noise=0.01, compile_ns=30e9, probe_ns=5e9,
                 extract_ns=0.2e9, verify_ns=2e9, measure_ns=5e9, fail=()):
        self.steps = dict(steps)            # s -> multiplicative factor applied for lengths >= s
        self.sig_changes = sorted(sig_changes)
        self.rng = np.random.default_rng(seed)
        self.noise = noise
        self.c = dict(compile_ns=int(compile_ns), probe_ns=int(probe_ns), extract_ns=int(extract_ns),
                      verify_ns=int(verify_ns), measure_ns=int(measure_ns))
        self.fail = set(fail)

    def true_latency(self, s):
        t = 1e6 * (1 + 0.01 * s + 1e-4 * s * s)
        for k, f in self.steps.items():
            if s >= k:
                t *= f
        return t

    def signature(self, s):
        return f"sig{sum(1 for c in self.sig_changes if s >= c)}"

    def measure(self, s):
        if s in self.fail:
            return {"failure_type": "compile_error", "compile_ns": self.c["compile_ns"]}
        lat = self.true_latency(s) * math.exp(self.rng.normal(0, self.noise))
        return {"latency_ns": lat, "signature": self.signature(s), "compile_ns": self.c["compile_ns"],
                "extract_ns": self.c["extract_ns"], "verify_ns": self.c["verify_ns"],
                "warmup_ns": self.c["measure_ns"] // 2, "measure_ns": self.c["measure_ns"]}

    def probe(self, s):
        if s in self.fail:
            return {"failure_type": "compile_error", "probe_ns": self.c["probe_ns"]}
        return {"signature": self.signature(s), "probe_ns": self.c["probe_ns"],
                "extract_ns": self.c["extract_ns"]}

    def confirm(self, a, b):
        confirmed = abs(math.log(self.true_latency(b) / self.true_latency(a))) > 0.02
        return {"confirmed": confirmed, "cost_ns": 2 * self.c["measure_ns"]}


class ReplayBackend:
    """dense: {s: {"compile_ns", "extract_ns", "verify_ns", "report_ns", "probe_ns",
    "signature", "probe_signature", "failure_type", "processes": [{"median_ns",
    "warmup_ns", "measure_ns", "process_overhead_ns"} or {"failure_type",
    "wall_ns"} ...]}}; confirm_fn(a, b) -> bool is the evaluator's predefined
    confirmation decision on independent data (answer table A procedure).
    A measure query replays one recorded process attempt chosen uniformly;
    a failed attempt makes the query fail and charges its wall time."""
    synthetic = False

    def __init__(self, dense, seed, confirm_fn=None, confirm_cost_fn=None):
        self.dense = dense
        self.rng = np.random.default_rng(seed)
        self.confirm_fn = confirm_fn
        self.confirm_cost_fn = confirm_cost_fn

    def measure(self, s):
        d = self.dense[s]
        if d.get("failure_type"):
            return {"failure_type": d["failure_type"], "compile_ns": d.get("compile_ns", 0),
                    "extract_ns": d.get("extract_ns", 0) if d.get("compile_ns") else 0,
                    "verify_ns": d.get("verify_ns", 0)}
        p = d["processes"][int(self.rng.integers(len(d["processes"])))]
        base = {"compile_ns": d["compile_ns"], "extract_ns": d.get("extract_ns", 0),
                "report_ns": d.get("report_ns", 0), "verify_ns": d.get("verify_ns", 0)}
        if p.get("failure_type"):
            return {**base, "failure_type": f"runtime:{p['failure_type']}", "measure_ns": p.get("wall_ns", 0)}
        return {**base, "latency_ns": p["median_ns"], "signature": usable_signature(d.get("signature")),
                "warmup_ns": p["warmup_ns"], "measure_ns": p["measure_ns"],
                "process_overhead_ns": p.get("process_overhead_ns", 0)}

    def probe(self, s):
        d = self.dense[s]
        if "probe_ns" not in d:
            raise KeyError(f"no probe data for length {s} (census must cover the whole valid set)")
        if d.get("probe_failure_type"):
            return {"failure_type": d["probe_failure_type"], "probe_ns": d.get("probe_ns", 0)}
        return {"signature": usable_signature(d.get("probe_signature")), "probe_ns": d["probe_ns"],
                "extract_ns": d.get("probe_extract_ns", 0)}

    def confirm(self, a, b):
        ok = bool(self.confirm_fn(a, b)) if self.confirm_fn else False
        cost = self.confirm_cost_fn(a, b) if self.confirm_cost_fn else 0
        return {"confirmed": ok, "cost_ns": int(cost)}


class LiveBackend:
    """Real compile -> verify -> measure on this machine. All numeric settings
    come from the (frozen) preregistration; nothing is defaulted here."""
    synthetic = False

    def __init__(self, model_key, flagset, target_cpu, out_root, prereg, feature_index,
                 cpus=None, threads=1, block_seed=0, vm_allocation_id="unavailable"):
        m = prereg["measurement"]
        for k in ("warmup_iterations", "timed_iterations", "process_statistic"):
            if m.get(k) is None:
                raise ValueError(f"preregistration.measurement.{k} is not fixed (G3)")
        # opt-report emission overhead inside every full compile: the broker
        # moves it from common to Compile-guided's extra cost (as in replay)
        self.report_ns = prereg["selectors"].get("report_overhead_ns")
        if self.report_ns is None:
            raise ValueError("preregistration.selectors.report_overhead_ns is not fixed (G3)")
        self.model_key, self.flagset, self.target_cpu = model_key, flagset, target_cpu
        self.out_root = Path(out_root)
        self.prereg = prereg
        self.feature_index = feature_index
        self.cpus, self.threads = cpus, threads
        self.block_seed = block_seed
        self.vm = vm_allocation_id
        self.n = 0

    def _dir(self, kind, s):
        self.n += 1
        return self.out_root / f"q{self.n:05d}_{kind}_s{s}"

    def measure(self, s):
        from .compile import compile_shape
        from .measure import run_item, worker_env
        d = self._dir("measure", s)
        c = compile_shape(self.model_key, s, self.flagset, self.target_cpu, "full", d)
        out = {"compile_ns": c["compile_wall_ns"], "extract_ns": c.get("feature_extract_wall_ns", 0)}
        if c.get("failure_type"):
            out["failure_type"] = c["failure_type"]
            return out
        out["report_ns"] = int(self.report_ns)
        t0 = time.monotonic_ns()
        v = self._verify(c["artifact_path"], s)
        out["verify_ns"] = time.monotonic_ns() - t0
        if v != "pass":
            out["failure_type"] = f"correctness:{v}"
            return out
        m = self.prereg["measurement"]
        item = {"artifact": c["artifact_path"], "artifact_hash": c["artifact_hash"],
                "model_key": self.model_key, "length": s, "feature_index": self.feature_index,
                "warmup": m["warmup_iterations"], "iterations": m["timed_iterations"], "cpus": self.cpus}
        r = run_item(item, worker_env(self.threads))
        if r.get("failure_type"):
            out["failure_type"] = f"runtime:{r['failure_type']}"
            out["measure_ns"] = r.get("process_wall_ns", 0)      # a failed attempt still costs its time
            return out
        stat = self.prereg["measurement"]["process_statistic"]
        lat = np.asarray(r["latency_ns"], float)
        w, mns = r["warmup_wall_ns"], r["measurement_wall_ns"]
        out.update(latency_ns=float(lat.mean() if stat == "mean" else np.median(lat)),
                   signature=usable_signature(c.get("ir_signature")), warmup_ns=w, measure_ns=mns,
                   process_overhead_ns=max(0, r.get("process_wall_ns", 0) - w - mns))
        return out

    def _verify(self, artifact, s):
        import subprocess
        import sys
        from .util import REPO_ROOT
        p = subprocess.run([sys.executable, str(REPO_ROOT / "validate_shapes.py"), "--artifact", artifact,
                            "--model", self.model_key, "--length", str(s),
                            "--out", str(self.out_root / "validation.jsonl")],
                           capture_output=True, text=True)
        if p.returncode != 0:
            return "error"
        import json
        st = json.loads(p.stdout.strip().splitlines()[-1])["correctness_status"]
        return "pass" if st == "pass" else st

    def probe(self, s):
        from .compile import compile_shape
        c = compile_shape(self.model_key, s, self.flagset, self.target_cpu, "probe", self._dir("probe", s))
        if c.get("failure_type"):
            return {"failure_type": c["failure_type"], "probe_ns": c["probe_wall_ns"]}
        return {"signature": usable_signature(c.get("ir_signature")), "probe_ns": c["probe_wall_ns"],
                "extract_ns": c.get("feature_extract_wall_ns", 0)}

    def confirm(self, a, b):
        raise NotImplementedError("live confirmation procedure is fixed at G3/G4 (preregistration)")

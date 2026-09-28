"""Query broker: runs a selector against a backend under a budget with the cost
accounting of spec §8.4.

Query types
  measure(s): compile (full) -> [information extraction] -> correctness check
              -> warmup -> measurement.  = the basic query of §8.3
  probe(s)  : compile to the probe stage -> information extraction  (§8.2b)
  confirm   : the predefined candidate confirmation procedure (broker-driven,
              recorded as its own timeline entry)

Cost split (§8.4 'compile cost attribution'):
  common_ns       full-compile wall time of measured shapes (minus the
                  opt-report emission overhead, see below), correctness check,
                  warmup, measurement; confirmations. Every policy pays these.
  policy_extra_ns information extraction, and opt-report emission, for policies
                  that consume full-compile signatures (Compile-guided); every
                  probe compile (Compile-probe); once per run, the model
                  node-name load the signature needs (signature_init_ns).
  selection_ns    wall time of the selector's own decision logic (measured in
                  the selector process; IPC excluded).
The backend reports `report_ns` = the opt-report emission overhead contained
in compile_ns (preregistered from the G3 pilot); baselines do not pay it. It is
capped at compile_ns (a compile that died early did not pay more than it took).

Every timeline entry carries query_index (= its position in the run, the spec
§12 join key; timeline_index is the same value).
With charge_policy_extra=False the extra cost is recorded but not charged
(the 'cost virtually excluded' auxiliary analysis, §11.2-2). Actual probe
spend is tracked either way, so Compile-probe's probe cap still applies.

Budget semantics (§8.4): a timeline entry counts for budget B only if its
completion time is <= B. The entry that crosses B is still executed, recorded
with its overrun, and flagged within_budget=False; the run then stops.
Confirmations are separate entries, so a confirmation that crosses B does not
invalidate the measurement (or an earlier confirmation) before it.

Candidate confirmation: when two *adjacent valid* shapes are both measured and
their observed |log ratio| >= confirm_log_threshold, the pair becomes a
candidate (recorded with the entry that created it) and the broker runs the
backend's confirmation procedure as its own entry - unless the budget is
already exhausted, in which case the candidate is recorded with
confirmation_status 'not_run'. A pair is checked once. The selector never
sees confirmation results, and it never sees the cumulative cost (that would
leak measurement durations to policies not entitled to timing, §8.3).

Isolation (§8.3): by default the selector runs in a jailed separate process
(shapeperf/selector_worker.py) and only receives its own View; the run
records the isolation level achieved. 'inprocess' is for debugging only.
"""
import bisect
import json
import math
import os
import select
import shutil
import subprocess
import sys
import tempfile
import time
import warnings

from .guard import SelectorIsolationError, selector_sandbox
from .selectors.base import MEASURE, PROBE, Action, make_view, view_to_json
from .util import REPO_ROOT


def _jsonable(x):
    """Plain-JSON copy of seeds/params (numpy scalars -> Python numbers)."""
    try:
        import numpy as np
        if isinstance(x, np.generic):
            return x.item()
    except ImportError:
        pass
    if isinstance(x, dict):
        return {str(k): _jsonable(v) for k, v in x.items()}
    if isinstance(x, (list, tuple)):
        return [_jsonable(v) for v in x]
    return x


class SelectorSpec:
    """What to run, without running it: selector class + seed + params. The
    broker never instantiates a spec in the evaluator process; in 'process'
    mode the constructor only ever runs inside the jailed worker."""

    def __init__(self, cls, seed=None, **params):
        self.cls, self.seed, self.params = cls, seed, params
        self.name = cls.name
        self.sees_timing, self.sees_signatures = cls.sees_timing, cls.sees_signatures
        self.uses_probe = cls.uses_probe

    @classmethod
    def of(cls, selector):
        """Spec of an already constructed (trusted, in-repo) selector instance."""
        return cls(type(selector), selector.seed, **selector.params)


class _ProcessSelector:
    """Proxy for a selector living in its own jailed process."""

    def __init__(self, spec, timeout_s=120.0):
        from .selectors import REGISTRY
        self.name = spec.name
        self.sees_timing, self.sees_signatures = spec.sees_timing, spec.sees_signatures
        self.uses_probe = spec.uses_probe
        self.described = None
        self.timeout_s = timeout_s
        self.seq = 0
        self.p = None
        self.jail = tempfile.mkdtemp(prefix="selector-jail-")
        os.chmod(self.jail, 0o555)
        req_r, self.req_w = os.pipe()
        self.rep_r, rep_w = os.pipe()
        self.stderr = tempfile.TemporaryFile()
        try:
            self.p = subprocess.Popen(
                [sys.executable, "-m", "shapeperf.selector_worker", str(req_r), str(rep_w), self.jail],
                cwd=REPO_ROOT, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=self.stderr,
                pass_fds=(req_r, rep_w), close_fds=True)
        finally:
            os.close(req_r)
            os.close(rep_w)
        self.win = os.fdopen(self.req_w, "w", buffering=1)
        self.rin = os.fdopen(self.rep_r, "r", buffering=1)
        init = {"cmd": "init", "name": spec.name, "seed": _jsonable(spec.seed),
                "params": _jsonable(spec.params)}
        if REGISTRY.get(spec.name) is not spec.cls:
            init["class_path"] = f"{spec.cls.__module__}:{spec.cls.__qualname__}"
        try:
            r = self._call(init)
        except BaseException:
            self.close()
            raise
        self.isolation_level = r.get("isolation_level", "unknown")
        self.described = r.get("describe")

    def _call(self, msg):
        self.seq += 1
        msg = dict(msg, seq=self.seq)
        try:
            self.win.write(json.dumps(msg) + "\n")
            self.win.flush()
        except BrokenPipeError:
            raise RuntimeError(f"selector process {self.name} died: {self._stderr_tail()}")
        ready, _, _ = select.select([self.rin], [], [], self.timeout_s)
        if not ready:
            raise RuntimeError(f"selector {self.name} did not answer within {self.timeout_s}s")
        line = self.rin.readline()
        if not line:
            raise RuntimeError(f"selector process {self.name} died: {self._stderr_tail()}")
        resp = json.loads(line)
        if resp.get("seq") != self.seq:
            raise SelectorIsolationError(f"selector {self.name}: protocol desync (seq {resp.get('seq')} != {self.seq})")
        if not resp["ok"]:
            if resp.get("isolation"):
                raise SelectorIsolationError(resp["error"])
            raise RuntimeError(f"selector {self.name} failed: {resp['error']}")
        return resp

    def _stderr_tail(self):
        try:
            self.stderr.seek(0)
            return self.stderr.read()[-2000:].decode(errors="replace")
        except Exception:
            return ""

    def next_action(self, view):
        t0 = time.perf_counter_ns()
        r = self._call({"cmd": "next", "view": view_to_json(view)})
        round_trip = time.perf_counter_ns() - t0
        sel = r.get("selection_ns")
        # the worker times the decision; it can never exceed the round trip
        if type(sel) is not int or not 0 <= sel <= round_trip:
            raise SelectorIsolationError(f"selector {self.name}: implausible selection_ns {sel!r} "
                                         f"(round trip {round_trip} ns)")
        act = None
        if r["action"] is not None:
            kind, length = r["action"]
            if kind not in (MEASURE, PROBE) or type(length) is not int:
                raise SelectorIsolationError(f"selector {self.name}: malformed action {r['action']!r}")
            act = Action(kind, length)
        return act, sel, r.get("warnings", [])

    def describe(self):
        return self.described

    def close(self):
        for f in ("win", "rin"):
            try:
                getattr(self, f).close()
            except Exception:
                pass
        if self.p is not None:
            try:
                self.p.wait(timeout=10)
            except Exception:
                self.p.kill()
                self.p.wait()
        try:
            self.stderr.close()
        except Exception:
            pass
        shutil.rmtree(self.jail, ignore_errors=True)


class _InProcessSelector:
    isolation_level = "inprocess (debug only: audit hook, no process or OS boundary)"

    def __init__(self, spec):
        with selector_sandbox():
            selector = spec.cls(seed=spec.seed, **spec.params)
        self.local = selector
        self.name = selector.name
        self.sees_timing, self.sees_signatures = selector.sees_timing, selector.sees_signatures
        self.uses_probe = selector.uses_probe

    def next_action(self, view):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            t0 = time.perf_counter_ns()
            with selector_sandbox():
                act = self.local.next_action(view)
            ns = time.perf_counter_ns() - t0
        return act, ns, [str(x.message) for x in w]

    def describe(self):
        return self.local.describe()

    def close(self):
        pass


def neighbours(valid, s):
    """Previous and next shape in the sorted valid set (None at the ends)."""
    i = bisect.bisect_left(valid, s)
    return (valid[i - 1] if i > 0 else None), (valid[i + 1] if i + 1 < len(valid) else None)


def _is_failure_sig(x):
    return isinstance(x, str) and x.startswith("FAILED:")


class QueryBroker:
    def __init__(self, backend, valid_lengths, charge_policy_extra=True,
                 confirm_log_threshold=None, max_queries=100000, isolation="process",
                 selector_timeout_s=120.0):
        self.backend = backend
        self.valid = sorted(int(v) for v in valid_lengths)
        self.charge_policy_extra = charge_policy_extra
        self.confirm_log_threshold = confirm_log_threshold
        self.max_queries = max_queries
        if isolation not in ("process", "inprocess"):
            raise ValueError(isolation)
        self.isolation = isolation
        self.selector_timeout_s = selector_timeout_s

    def run(self, selector, budget_ns):
        """selector: a SelectorSpec (preferred: nothing runs in this process) or
        a trusted in-repo selector instance (converted to its spec)."""
        spec = selector if isinstance(selector, SelectorSpec) else SelectorSpec.of(selector)
        host = None
        try:
            host = (_ProcessSelector(spec, self.selector_timeout_s) if self.isolation == "process"
                    else _InProcessSelector(spec))
            out = self._run(host, budget_ns)
            out["isolation_level"] = host.isolation_level
            return out
        finally:
            if host is not None:
                host.close()

    def _run(self, selector, budget_ns):
        valid_set = set(self.valid)
        measured, sigs, psigs, failed, probed = {}, {}, {}, {}, set()
        lat = {}   # latency estimates, kept by the broker (timing policies see them via the view)
        spent = probe_spent = 0
        timeline, candidates, sig_pairs = [], [], []
        checked_pairs = set()
        stop = False
        init_paid = False     # one-time node-name load, paid by the first signature-consuming query

        def append(rec, charged):
            nonlocal spent, stop
            spent += charged
            rec.update(charged_ns=charged, cumulative_cost_ns=spent, within_budget=spent <= budget_ns,
                       timeline_index=len(timeline), query_index=len(timeline))
            timeline.append(rec)
            if spent >= budget_ns:
                stop = True
            return rec

        for _ in range(self.max_queries):
            if stop:
                break
            view = make_view(self.valid,
                             {s: (lat.get(s) if selector.sees_timing else None) for s in measured},
                             sigs if selector.sees_signatures else {},
                             failed, probed, budget_ns, probe_spent,
                             psigs if selector.uses_probe else {})
            action, sel_ns, warns = selector.next_action(view)
            if action is None:
                break
            s = action.length
            if s not in valid_set:
                raise ValueError(f"{selector.name} chose invalid length {s}")
            rec = {"selector": selector.name, "action": action.kind, "padded_length": s,
                   "selection_ns": sel_ns}
            if warns:
                rec["selector_warnings"] = warns
            extra = common = 0
            if action.kind == MEASURE:
                if s in measured or s in failed:
                    raise ValueError(f"{selector.name} re-measured {s}")
                r = self.backend.measure(s)
                compile_ns = max(0, r.get("compile_ns", 0))
                report_ns = min(max(0, r.get("report_ns", 0)), compile_ns)
                common = compile_ns - report_ns
                if selector.sees_signatures:
                    extra = r.get("extract_ns", 0) + report_ns
                    if not init_paid and r.get("signature_init_ns"):
                        extra += r["signature_init_ns"]
                        rec["signature_init_ns"] = r["signature_init_ns"]
                        init_paid = True
                common += sum(r.get(k, 0) for k in ("verify_ns", "warmup_ns", "measure_ns", "process_overhead_ns"))
                rec.update(failure_type=r.get("failure_type"))
                if r.get("failure_type"):
                    failed[s] = r["failure_type"]
                    sigs[s] = f"FAILED:{r['failure_type']}"
                else:
                    measured[s] = True
                    lat[s] = r["latency_ns"]
                    if r.get("signature") is not None:
                        sigs[s] = r["signature"]
                    rec["latency_ns"] = r["latency_ns"]
            elif action.kind == PROBE:
                if not selector.uses_probe:
                    raise ValueError(f"{selector.name} is not allowed to probe")
                if s in probed:
                    raise ValueError(f"{selector.name} re-probed {s}")
                r = self.backend.probe(s)
                extra = r.get("probe_ns", 0) + r.get("extract_ns", 0)
                if not init_paid and r.get("signature_init_ns"):
                    extra += r["signature_init_ns"]
                    rec["signature_init_ns"] = r["signature_init_ns"]
                    init_paid = True
                rec.update(failure_type=r.get("failure_type"))
                probed.add(s)
                psigs[s] = f"FAILED:{r['failure_type']}" if r.get("failure_type") else r["signature"]
                probe_spent += extra              # actual spend, charged or not
            else:
                raise ValueError(action.kind)
            rec.update(common_ns=common, policy_extra_ns=extra)
            if s in psigs and s in sigs and not _is_failure_sig(psigs[s]) and not _is_failure_sig(sigs[s]):
                sig_pairs.append({"length": s, "probe": psigs[s], "full": sigs[s]})
            append(rec, common + (extra if self.charge_policy_extra else 0) + sel_ns)

            if action.kind == MEASURE and s in measured and self.confirm_log_threshold is not None:
                prev, nxt = neighbours(self.valid, s)
                for a, b in ((prev, s), (s, nxt)):
                    if a in measured and b in measured and (a, b) not in checked_pairs:
                        checked_pairs.add((a, b))
                        lr = math.log(lat[b] / lat[a])
                        if abs(lr) < self.confirm_log_threshold:
                            continue
                        cand = {"pair": [a, b], "observed_log_ratio": lr,
                                "query_index": rec["query_index"],        # the entry that created it
                                "created_at_index": rec["query_index"],
                                "created_cost_ns": rec["cumulative_cost_ns"],
                                "created_within_budget": rec["within_budget"],
                                "confirmed": None, "confirm_index": None, "confirmation_ns": None,
                                "cumulative_cost_ns": None, "confirmation_status": "not_run (budget exhausted)"}
                        if not stop:
                            c = self.backend.confirm(a, b)
                            crec = append({"selector": selector.name, "action": "confirm", "pair": [a, b],
                                           "padded_length": None, "candidate_query_index": rec["query_index"],
                                           "observed_log_ratio": lr, "confirmed": c["confirmed"],
                                           "common_ns": c["cost_ns"], "policy_extra_ns": 0, "selection_ns": 0},
                                          c["cost_ns"])
                            cand.update(confirmed=c["confirmed"], confirm_index=crec["query_index"],
                                        confirmation_ns=c["cost_ns"],
                                        cumulative_cost_ns=crec["cumulative_cost_ns"],
                                        confirmation_status="done" if crec["within_budget"] else
                                        "done (crossed budget)")
                        candidates.append(cand)
        return {"selector": selector.describe(), "budget_ns": budget_ns, "valid": self.valid,
                "isolation": self.isolation, "charge_policy_extra": self.charge_policy_extra,
                "timeline": timeline, "candidates": candidates,
                "spent_ns": spent, "probe_spent_ns": probe_spent,
                "probe_vs_full_signatures": sig_pairs}

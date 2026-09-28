"""Query broker: runs a selector against a backend under a budget with the cost
accounting of spec §8.4.

Query types
  measure(s): compile (full) -> [information extraction] -> correctness check
              -> warmup -> measurement.  = the basic query of §8.3
  probe(s)  : compile to the probe stage -> information extraction  (§8.2b)

Cost split (§8.4 'compile cost attribution'):
  common_ns       full-compile wall time of measured shapes, correctness check,
                  warmup, measurement, candidate confirmation: every policy pays
                  these for what it measures.
  policy_extra_ns information extraction for signature-consuming policies and
                  every probe compile: the policy-specific marginal cost.
  selection_ns    wall time of the selector's own decision logic (measured in
                  the selector process, IPC excluded).
With charge_policy_extra=False the extra cost is recorded but not charged
(the 'cost virtually excluded' auxiliary analysis, §11.2-2). Actual probe spend
is tracked either way, so Compile-probe's probe cap still applies.

Budget semantics (§8.4): a query counts for budget B only if its completion
time is <= B. The query that crosses B is still executed, recorded with its
overrun, and flagged within_budget=False; the run then stops.

Candidate confirmation: when two *adjacent valid* shapes are both measured and
their observed |log ratio| >= confirm_log_threshold, the broker runs the
backend's predefined confirmation procedure and charges it as common cost. A
pair is checked once, when it first becomes adjacent-and-measured. The
selector never sees confirmation results.

Isolation (§8.3): by default the selector runs in a separate process
(shapeperf/selector_worker.py) and only receives its own View; 'inprocess'
exists for debugging and uses the audit-hook sandbox only.
"""
import bisect
import json
import math
import subprocess
import sys
import time

from .guard import SelectorIsolationError, selector_sandbox
from .selectors.base import MEASURE, PROBE, Action, make_view, view_to_json
from .util import REPO_ROOT


class _ProcessSelector:
    """Proxy for a selector living in its own process."""

    def __init__(self, selector):
        self.local = selector            # used only for its class attributes / describe()
        self.name = selector.name
        self.sees_timing, self.sees_signatures = selector.sees_timing, selector.sees_signatures
        self.uses_probe = selector.uses_probe
        from .selectors import REGISTRY
        init = {"cmd": "init", "name": selector.name, "seed": selector.seed, "params": selector.params}
        if REGISTRY.get(selector.name) is not type(selector):
            init["class_path"] = f"{type(selector).__module__}:{type(selector).__qualname__}"
        self.p = subprocess.Popen([sys.executable, "-m", "shapeperf.selector_worker"], cwd=REPO_ROOT,
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True, bufsize=1)
        self._call(init)

    def _call(self, msg):
        self.p.stdin.write(json.dumps(msg) + "\n")
        self.p.stdin.flush()
        line = self.p.stdout.readline()
        if not line:
            raise RuntimeError(f"selector process {self.name} died")
        resp = json.loads(line)
        if not resp["ok"]:
            if resp.get("isolation"):
                raise SelectorIsolationError(resp["error"])
            raise RuntimeError(f"selector {self.name} failed: {resp['error']}")
        return resp

    def next_action(self, view):
        r = self._call({"cmd": "next", "view": view_to_json(view)})
        act = None if r["action"] is None else Action(r["action"][0], int(r["action"][1]))
        return act, r["selection_ns"], r.get("warnings", [])

    def describe(self):
        return self.local.describe()

    def close(self):
        try:
            self.p.stdin.close()
            self.p.wait(timeout=10)
        except Exception:
            self.p.kill()


class _InProcessSelector:
    def __init__(self, selector):
        self.local = selector
        self.name = selector.name
        self.sees_timing, self.sees_signatures = selector.sees_timing, selector.sees_signatures
        self.uses_probe = selector.uses_probe

    def next_action(self, view):
        t0 = time.perf_counter_ns()
        with selector_sandbox():
            act = self.local.next_action(view)
        return act, time.perf_counter_ns() - t0, []

    def describe(self):
        return self.local.describe()

    def close(self):
        pass


def neighbours(valid, s):
    """Previous and next shape in the sorted valid set (None at the ends)."""
    i = bisect.bisect_left(valid, s)
    return (valid[i - 1] if i > 0 else None), (valid[i + 1] if i + 1 < len(valid) else None)


class QueryBroker:
    def __init__(self, backend, valid_lengths, charge_policy_extra=True,
                 confirm_log_threshold=None, max_queries=100000, isolation="process"):
        self.backend = backend
        self.valid = sorted(valid_lengths)
        self.charge_policy_extra = charge_policy_extra
        self.confirm_log_threshold = confirm_log_threshold
        self.max_queries = max_queries
        if isolation not in ("process", "inprocess"):
            raise ValueError(isolation)
        self.isolation = isolation

    def run(self, selector, budget_ns):
        host = (_ProcessSelector if self.isolation == "process" else _InProcessSelector)(selector)
        try:
            return self._run(host, budget_ns)
        finally:
            host.close()

    def _run(self, selector, budget_ns):
        valid_set = set(self.valid)
        measured, sigs, psigs, failed, probed = {}, {}, {}, {}, set()
        lat = {}   # latency estimates, kept by the broker (timing policies see them via the view)
        spent = probe_spent = 0
        timeline, candidates, sig_pairs = [], [], []
        checked_pairs = set()
        for qi in range(self.max_queries):
            view = make_view(self.valid,
                             {s: (lat.get(s) if selector.sees_timing else None) for s in measured},
                             sigs if selector.sees_signatures else {},
                             failed, probed, spent, budget_ns, probe_spent,
                             psigs if selector.uses_probe else {})
            action, sel_ns, warns = selector.next_action(view)
            if action is None:
                break
            s = action.length
            if s not in valid_set:
                raise ValueError(f"{selector.name} chose invalid length {s}")
            rec = {"query_index": qi, "selector": selector.name, "action": action.kind,
                   "padded_length": s, "selection_ns": sel_ns}
            if warns:
                rec["selector_warnings"] = warns
            extra = common = 0
            if action.kind == MEASURE:
                if s in measured or s in failed:
                    raise ValueError(f"{selector.name} re-measured {s}")
                r = self.backend.measure(s)
                extra = r.get("extract_ns", 0) if selector.sees_signatures else 0
                common = sum(r.get(k, 0) for k in ("compile_ns", "verify_ns", "warmup_ns", "measure_ns"))
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
                rec.update(failure_type=r.get("failure_type"))
                probed.add(s)
                psigs[s] = f"FAILED:{r['failure_type']}" if r.get("failure_type") else r["signature"]
                probe_spent += extra              # actual spend, charged or not
            else:
                raise ValueError(action.kind)
            rec.update(common_ns=common, policy_extra_ns=extra)
            if s in psigs and s in sigs:
                sig_pairs.append({"length": s, "probe": psigs[s], "full": sigs[s]})
            charged = common + (extra if self.charge_policy_extra else 0) + sel_ns
            conf_ns = 0
            if action.kind == MEASURE and s in measured and self.confirm_log_threshold is not None:
                prev, nxt = neighbours(self.valid, s)
                for a, b in ((prev, s), (s, nxt)):
                    if a in measured and b in measured and (a, b) not in checked_pairs:
                        checked_pairs.add((a, b))
                        lr = math.log(lat[b] / lat[a])
                        if abs(lr) >= self.confirm_log_threshold:
                            c = self.backend.confirm(a, b)
                            conf_ns += c["cost_ns"]
                            candidates.append({"pair": [a, b], "observed_log_ratio": lr,
                                               "confirmed": c["confirmed"], "query_index": qi,
                                               "confirmation_ns": c["cost_ns"],
                                               "cumulative_cost_ns": spent + charged + conf_ns})
            charged += conf_ns
            spent += charged
            rec.update(confirmation_ns=conf_ns, charged_ns=charged, cumulative_cost_ns=spent,
                       within_budget=spent <= budget_ns)
            timeline.append(rec)
            if spent >= budget_ns:
                break
        return {"selector": selector.describe(), "budget_ns": budget_ns, "valid": self.valid,
                "isolation": self.isolation, "charge_policy_extra": self.charge_policy_extra,
                "timeline": timeline, "candidates": candidates,
                "spent_ns": spent, "probe_spent_ns": probe_spent,
                "probe_vs_full_signatures": sig_pairs}

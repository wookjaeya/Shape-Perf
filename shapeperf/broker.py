"""Query broker: runs a selector against a backend under a budget with the cost
accounting of spec §8.4.

Query types
  measure(s): compile (full) -> [information extraction] -> correctness check
              -> warmup -> measurement.  = the basic query of §8.3
  probe(s)  : compile to the probe stage -> information extraction  (§8.2b)

Cost split (§8.4 'compile cost attribution'):
  common_ns       full-compile wall time of measured shapes, correctness check,
                  warmup, measurement: every policy pays these for a measured shape.
  policy_extra_ns information extraction for signature-consuming policies and
                  every probe compile: the policy-specific marginal cost.
  selection_ns    wall time of the selector's own decision logic.
With charge_policy_extra=False the extra cost is recorded but not charged
(the 'cost virtually excluded' auxiliary analysis, §11.2-2).

Budget semantics (§8.4): a query counts for budget B only if its completion
time is <= B. The query that crosses B is still executed, recorded with its
overrun, and flagged within_budget=False; the run then stops.

Candidate confirmation: when two adjacent valid shapes are both measured and
their observed |log ratio| >= confirm_log_threshold, the broker runs the
backend's predefined confirmation procedure (fresh processes) and charges it
as common cost. The selector never sees confirmation results.
"""
import math
import time

from .guard import selector_sandbox
from .selectors.base import MEASURE, PROBE, make_view


class QueryBroker:
    def __init__(self, backend, valid_lengths, charge_policy_extra=True,
                 confirm_log_threshold=None, max_queries=100000):
        self.backend = backend
        self.valid = sorted(valid_lengths)
        self.charge_policy_extra = charge_policy_extra
        self.confirm_log_threshold = confirm_log_threshold
        self.max_queries = max_queries

    def run(self, selector, budget_ns):
        measured, sigs, failed, probed = {}, {}, {}, set()
        lat_true = {}   # latency estimates kept by the broker for confirmation logic
        spent = probe_spent = 0
        timeline, candidates = [], []
        confirmed_pairs = set()
        for qi in range(self.max_queries):
            view = make_view(self.valid,
                             {s: (lat_true.get(s) if selector.sees_timing else None) for s in measured},
                             sigs if selector.sees_signatures else {},
                             failed, probed, spent, budget_ns, probe_spent)
            t0 = time.perf_counter_ns()
            with selector_sandbox():
                action = selector.next_action(view)
            sel_ns = time.perf_counter_ns() - t0
            if action is None:
                break
            s = action.length
            if s not in self.valid:
                raise ValueError(f"{selector.name} chose invalid length {s}")
            rec = {"query_index": qi, "selector": selector.name, "action": action.kind,
                   "padded_length": s, "selection_ns": sel_ns}
            if action.kind == MEASURE:
                if s in measured or s in failed:
                    raise ValueError(f"{selector.name} re-measured {s}")
                r = self.backend.measure(s)
                extra = r.get("extract_ns", 0) if selector.sees_signatures else 0
                common = sum(r.get(k, 0) for k in ("compile_ns", "verify_ns", "warmup_ns", "measure_ns"))
                rec.update(common_ns=common, policy_extra_ns=extra, failure_type=r.get("failure_type"))
                if r.get("failure_type"):
                    failed[s] = r["failure_type"]
                    sigs[s] = f"FAILED:{r['failure_type']}"
                else:
                    measured[s] = True
                    lat_true[s] = r["latency_ns"]
                    if r.get("signature") is not None:
                        sigs[s] = r["signature"]
                    rec["latency_ns"] = r["latency_ns"]
            elif action.kind == PROBE:
                if not selector.uses_probe:
                    raise ValueError(f"{selector.name} is not allowed to probe")
                if s in probed or s in measured:
                    raise ValueError(f"{selector.name} re-probed {s}")
                r = self.backend.probe(s)
                extra = r.get("probe_ns", 0) + r.get("extract_ns", 0)
                common = 0
                rec.update(common_ns=0, policy_extra_ns=extra, failure_type=r.get("failure_type"))
                probed.add(s)
                sigs[s] = f"FAILED:{r['failure_type']}" if r.get("failure_type") else r["signature"]
                probe_spent += extra if self.charge_policy_extra else 0
            else:
                raise ValueError(action.kind)
            charged = common + (extra if self.charge_policy_extra else 0) + sel_ns
            # candidate confirmation on newly adjacent measured pairs
            conf_ns = 0
            if action.kind == MEASURE and s in measured and self.confirm_log_threshold is not None:
                for a, b in ((s - 1, s), (s, s + 1)):
                    if a in measured and b in measured and (a, b) not in confirmed_pairs:
                        lr = math.log(lat_true[b] / lat_true[a])
                        if abs(lr) >= self.confirm_log_threshold:
                            c = self.backend.confirm(a, b)
                            conf_ns += c["cost_ns"]
                            confirmed_pairs.add((a, b))
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
        return {"selector": selector.describe(), "budget_ns": budget_ns,
                "charge_policy_extra": self.charge_policy_extra,
                "timeline": timeline, "candidates": candidates,
                "spent_ns": spent, "probe_spent_ns": probe_spent}

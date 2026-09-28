"""Evaluator-side analysis (spec §9, §10.3, §11.1). Selectors never import this.

Everything numeric that defines an event (delta, alpha, bootstrap reps, match
tolerance, process statistic) is passed in from the frozen preregistration.
"""
import math
from collections import defaultdict

import numpy as np

# ---------------------------------------------------------------------------
# Statistics helpers


def holm(pvals):
    """Holm step-down adjusted p-values (R22), same order as input."""
    p = np.asarray(pvals, float)
    order = np.argsort(p)
    m = len(p)
    adj = np.empty(m)
    run = 0.0
    for rank, i in enumerate(order):
        run = max(run, min(1.0, (m - rank) * p[i]))
        adj[i] = run
    return adj


def process_stat(latencies_ns, stat):
    a = np.asarray(latencies_ns, float)
    if stat == "mean":
        return float(a.mean())
    if stat == "median":
        return float(np.median(a))
    raise ValueError(f"unknown process statistic {stat!r}")


def block_table(records, stat):
    """{(allocation, block): {length: [process statistic, ...]}} from raw
    measurement records; failed records are excluded here but must be reported
    by the caller (they stay in the denominator)."""
    t = defaultdict(lambda: defaultdict(list))
    for r in records:
        if r.get("failure_type"):
            continue
        t[(r.get("vm_allocation_id", "unavailable"), r["block_id"])][r["padded_length"]].append(
            process_stat(r["latency_ns"], stat))
    return t


def paired_log_ratios(table, s, t):
    """Per-block paired effect log(T_b(t) / T_b(s)), grouped by allocation."""
    by_alloc = defaultdict(list)
    for (alloc, _block), d in table.items():
        if s in d and t in d:
            by_alloc[alloc].append(math.log(np.mean(d[t]) / np.mean(d[s])))
    return dict(by_alloc)


def hier_bootstrap(by_group, reps, rng):
    """Two-level bootstrap: resample groups (allocations), then units (blocks)
    within each resampled group; statistic = mean of all resampled units."""
    groups = [np.asarray(v, float) for v in by_group.values() if len(v)]
    if not groups:
        return None
    out = np.empty(reps)
    g = len(groups)
    for i in range(reps):
        vals = []
        for k in rng.integers(0, g, g):
            x = groups[k]
            vals.append(x[rng.integers(0, len(x), len(x))])
        out[i] = np.concatenate(vals).mean()
    return out


def effect_test(by_alloc, delta, reps, rng, conf_level):
    """Estimate, CI and bootstrap p-value for H0: |effect| <= log(1+delta)."""
    allv = [x for v in by_alloc.values() for x in v]
    if not allv:
        return None
    est = float(np.mean(allv))
    boot = hier_bootstrap(by_alloc, reps, rng)
    lo, hi = np.quantile(boot, [(1 - conf_level) / 2, 1 - (1 - conf_level) / 2])
    thr = math.log1p(delta)
    if est >= 0:
        p = float(np.mean(boot <= thr))
    else:
        p = float(np.mean(boot >= -thr))
    return {"log_effect": est, "ratio": math.exp(est), "ci_log": [float(lo), float(hi)],
            "p_exceeds_delta": max(p, 1.0 / reps), "n_blocks": len(allv), "n_allocations": len(by_alloc)}


# ---------------------------------------------------------------------------
# Continuous indicators and answer table A (spec §9.1-9.3)


def bert_flops(s, cfg):
    """Forward multiply-add count x2 of the BERT encoder + SQuAD head for one
    sequence of length s (compute-growth reference for A(s); not a time model)."""
    H, I, N = cfg["hidden_size"], cfg["intermediate_size"], cfg["num_hidden_layers"]
    per_layer = 8 * s * H * H + 4 * s * s * H + 4 * s * H * I
    return N * per_layer + 4 * s * H


def continuous_indicators(table, valid, reps, seed, conf_level, delta, flop_cfg=None):
    rng = np.random.default_rng(seed)
    vs = sorted(valid)
    rows = []
    for s, t in zip(vs, vs[1:]):
        r = effect_test(paired_log_ratios(table, s, t), delta, reps, rng, conf_level)
        if r is None:
            rows.append({"s": s, "t": t, "missing": True})
            continue
        r.update(s=s, t=t)
        if flop_cfg:
            f = math.log(bert_flops(t, flop_cfg) / bert_flops(s, flop_cfg))
            r["log_flop_ratio"] = f
            r["log_excess_over_flops"] = r["log_effect"] - f
        rows.append(r)
    return rows


def answer_table_a(discovery_rows, confirmation_table, delta, alpha, reps, seed, conf_level):
    """Candidates: Holm-adjusted p < alpha on discovery data over the family of
    all adjacent pairs. Events: candidates whose effect exceeds delta in the
    same direction on independent confirmation data (Holm over the candidates).
    Signatures play no role (spec §9.3)."""
    rows = [r for r in discovery_rows if not r.get("missing")]
    if rows and reps < len(rows) / alpha:
        # the smallest bootstrap p-value is 1/reps; Holm multiplies it by the family size
        raise ValueError(f"bootstrap reps={reps} cannot reach Holm significance for a family of "
                         f"{len(rows)} at alpha={alpha}; need reps >= {math.ceil(len(rows) / alpha)}")
    adj = holm([r["p_exceeds_delta"] for r in rows]) if rows else []
    cands = []
    for r, a in zip(rows, adj):
        r["p_holm"] = float(a)
        if a < alpha:
            cands.append(r)
    rng = np.random.default_rng(seed + 1)
    conf = []
    for r in cands:
        c = effect_test(paired_log_ratios(confirmation_table, r["s"], r["t"]), delta, reps, rng, conf_level)
        conf.append(c)
    padj = holm([c["p_exceeds_delta"] if c else 1.0 for c in conf]) if conf else []
    events = []
    for r, c, a in zip(cands, conf, padj):
        same_dir = c is not None and np.sign(c["log_effect"]) == np.sign(r["log_effect"])
        ok = bool(c is not None and a < alpha and same_dir)
        events.append({"s": r["s"], "t": r["t"], "discovery": r, "confirmation": c,
                       "confirmation_p_holm": float(a), "confirmed": ok})
    return {"candidates": [e["s"] for e in events], "events": [e["s"] for e in events if e["confirmed"]],
            "detail": events, "family_size": len(rows)}


# ---------------------------------------------------------------------------
# Census metrics (spec §9.4)


def change_points(sig_by_len, valid):
    """Left endpoints s of adjacent valid pairs (s, next) whose signatures differ."""
    vs = [v for v in sorted(valid) if v in sig_by_len]
    return sorted(s for s, t in zip(vs, vs[1:]) if sig_by_len[s] != sig_by_len[t])


def aligned_boundaries(valid, units):
    """B_align: boundaries (s, s+1) with s or s+1 a multiple of a unit."""
    vs = set(valid)
    return sorted(s for s in vs if (s + 1) in vs and any(u > 1 and (s % u == 0 or (s + 1) % u == 0)
                                                           for u in units))


def _match(xs, ys, tol):
    ys = sorted(ys)
    return {x for x in xs if any(abs(x - y) <= tol for y in ys)}


def census_metrics(c_sig, b_align, e_a, tol):
    c_sig, b_align, e_a = set(c_sig), set(b_align), set(e_a)
    c_non = c_sig - b_align
    hit = _match(c_sig, e_a, tol)
    ev_hit = _match(e_a, c_sig, tol)
    ev_non = _match(e_a, c_non, tol)
    return {
        "n_C_sig": len(c_sig), "n_B_align": len(b_align), "n_C_nonalign": len(c_non), "n_E_A": len(e_a),
        "C_nonalign": sorted(c_non),
        "Prec_sig": len(hit) / len(c_sig) if c_sig else None,
        "Rec_sig": len(ev_hit) / len(e_a) if e_a else None,
        "nonalign_contribution": len(ev_non) / len(e_a) if e_a else None,
        "H1_nonanalytic_changes_exist": bool(c_non),
        "match_tolerance": tol,
    }


# ---------------------------------------------------------------------------
# Policy comparison (spec §11.1)


def discoveries(run, events, require_confirmation):
    """Events found by one broker run: both endpoints measured successfully by
    queries completed within budget; optionally also confirmed by the broker's
    confirmation procedure. Returns {event_s: cumulative cost at discovery}."""
    measured, found = set(), {}
    conf_at = {tuple(c["pair"]): c for c in run["candidates"] if c["confirmed"]}
    ev = set(events)
    for q in run["timeline"]:
        if not q["within_budget"]:
            break
        if q["action"] == "measure" and not q.get("failure_type"):
            measured.add(q["padded_length"])
        for s in list(ev - set(found)):
            if s in measured and (s + 1) in measured:
                if not require_confirmation:
                    found[s] = q["cumulative_cost_ns"]
                elif (s, s + 1) in conf_at:
                    found[s] = conf_at[(s, s + 1)]["cumulative_cost_ns"]
    return found


def recall_cost(runs_by_budget, events, require_confirmation):
    """runs_by_budget: {budget_ns: [run per seed]} -> rows with recall stats."""
    rows = []
    for b, runs in sorted(runs_by_budget.items()):
        rec = [len(discoveries(r, events, require_confirmation)) / len(events) if events else float("nan")
               for r in runs]
        cands = [len(r["candidates"]) for r in runs]
        conf = [sum(c["confirmed"] for c in r["candidates"]) for r in runs]
        rows.append({"budget_ns": b, "n_runs": len(runs), "recall_mean": float(np.mean(rec)),
                     "recall_min": float(np.min(rec)), "recall_max": float(np.max(rec)),
                     "candidates_mean": float(np.mean(cands)),
                     "confirmed_fraction": (float(np.sum(conf)) / np.sum(cands)) if np.sum(cands) else None})
    return rows

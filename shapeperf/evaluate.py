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
    if len(groups) == 1:                       # vectorized single-allocation case
        x = groups[0]
        return x[rng.integers(0, len(x), (reps, len(x)))].mean(axis=1)
    out = np.empty(reps)
    g = len(groups)
    for i in range(reps):
        vals = []
        for k in rng.integers(0, g, g):
            x = groups[k]
            vals.append(x[rng.integers(0, len(x), len(x))])
        out[i] = np.concatenate(vals).mean()
    return out


def effect_test(by_alloc, delta, reps, rng, conf_level, direction="two-sided",
                min_blocks=2, min_allocations=1):
    """Estimate, CI and bootstrap p-value for H0: effect within [-thr, thr],
    thr = log(1+delta).

    direction: "two-sided"  H1: |effect| > thr, p = 2 * min of the two tails
               "increase"   H1: effect > thr   (latency grows from s to t)
               "decrease"   H1: effect < -thr
    With fewer than min_blocks paired blocks (or min_allocations allocations)
    the bootstrap is degenerate, so the pair is reported as insufficient."""
    allv = [x for v in by_alloc.values() for x in v]
    n_alloc = sum(1 for v in by_alloc.values() if len(v))
    if len(allv) < max(1, min_blocks) or n_alloc < max(1, min_allocations):
        return {"insufficient": True, "n_blocks": len(allv), "n_allocations": n_alloc}
    est = float(np.mean(allv))
    boot = hier_bootstrap(by_alloc, reps, rng)
    lo, hi = np.quantile(boot, [(1 - conf_level) / 2, 1 - (1 - conf_level) / 2])
    thr = math.log1p(delta)
    floor = 1.0 / reps
    p_up = max(float(np.mean(boot <= thr)), floor)       # evidence for effect > thr
    p_down = max(float(np.mean(boot >= -thr)), floor)    # evidence for effect < -thr
    if direction == "two-sided":
        p = min(1.0, 2 * min(p_up, p_down))
    elif direction == "increase":
        p = p_up
    elif direction == "decrease":
        p = p_down
    else:
        raise ValueError(direction)
    return {"log_effect": est, "ratio": math.exp(est), "ci_log": [float(lo), float(hi)],
            "p_exceeds_delta": p, "direction": direction, "n_blocks": len(allv), "n_allocations": n_alloc}


# ---------------------------------------------------------------------------
# Continuous indicators and answer table A (spec §9.1-9.3)


def bert_flops(s, cfg):
    """Forward multiply-add count x2 of the BERT encoder + SQuAD head for one
    sequence of length s (compute-growth reference for A(s); not a time model)."""
    H, I, N = cfg["hidden_size"], cfg["intermediate_size"], cfg["num_hidden_layers"]
    per_layer = 8 * s * H * H + 4 * s * s * H + 4 * s * H * I
    return N * per_layer + 4 * s * H


def continuous_indicators(table, valid, reps, seed, conf_level, delta, flop_cfg=None,
                          direction="two-sided", min_blocks=2, min_allocations=1):
    """One row per adjacent pair (s, t) of the sorted valid set - the predefined
    test family. Pairs without enough paired data are kept as 'missing'."""
    rng = np.random.default_rng(seed)
    vs = sorted(valid)
    rows = []
    for s, t in zip(vs, vs[1:]):
        r = effect_test(paired_log_ratios(table, s, t), delta, reps, rng, conf_level, direction,
                        min_blocks, min_allocations)
        if r.get("insufficient"):
            rows.append({"s": s, "t": t, "missing": True, **r})
            continue
        r.update(s=s, t=t)
        if flop_cfg:
            f = math.log(bert_flops(t, flop_cfg) / bert_flops(s, flop_cfg))
            r["log_flop_ratio"] = f
            r["log_excess_over_flops"] = r["log_effect"] - f
        rows.append(r)
    return rows


def answer_table_a(discovery_rows, confirmation_table, delta, alpha, reps, seed, conf_level,
                   direction="two-sided", min_blocks=2, min_allocations=1):
    """Candidates: Holm-adjusted p < alpha on discovery data over the family of
    ALL adjacent valid pairs (pairs without data enter with p = 1, so the family
    does not depend on the data, spec §10.3). Events: candidates whose effect
    exceeds delta in the discovery direction on independent confirmation data
    (one-sided in that fixed direction, Holm over the candidates).
    Signatures play no role (spec §9.3)."""
    rows = list(discovery_rows)
    m = len(rows)
    pmin = (2.0 if direction == "two-sided" else 1.0) / reps
    if m and pmin * m >= alpha:
        # the smallest bootstrap p-value times the family size can never reach alpha
        need = math.ceil((2 if direction == "two-sided" else 1) * m / alpha) + 1
        raise ValueError(f"bootstrap reps={reps} cannot reach Holm significance for a family of "
                         f"{m} at alpha={alpha}; need reps >= {need}")
    adj = holm([1.0 if r.get("missing") else r["p_exceeds_delta"] for r in rows]) if rows else []
    cands = []
    for r, a in zip(rows, adj):
        r["p_holm"] = float(a)
        if not r.get("missing") and a < alpha:
            cands.append(r)
    rng = np.random.default_rng(seed + 1)
    conf = []
    for r in cands:
        d = "increase" if r["log_effect"] > 0 else "decrease"
        c = effect_test(paired_log_ratios(confirmation_table, r["s"], r["t"]), delta, reps, rng,
                        conf_level, d, min_blocks, min_allocations)
        conf.append(None if c.get("insufficient") else c)
    padj = holm([c["p_exceeds_delta"] if c else 1.0 for c in conf]) if conf else []
    events = []
    for r, c, a in zip(cands, conf, padj):
        same_dir = c is not None and np.sign(c["log_effect"]) == np.sign(r["log_effect"])
        ok = bool(c is not None and a < alpha and same_dir)
        events.append({"s": r["s"], "t": r["t"], "discovery": r, "confirmation": c,
                       "confirmation_p_holm": float(a), "confirmed": ok})
    return {"candidates": [e["s"] for e in events], "events": [e["s"] for e in events if e["confirmed"]],
            "event_pairs": [[e["s"], e["t"]] for e in events if e["confirmed"]],
            "detail": events, "family_size": m,
            "n_missing_pairs": sum(1 for r in rows if r.get("missing"))}


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


def _next_valid(valid, s):
    i = valid.index(s)
    return valid[i + 1] if i + 1 < len(valid) else None


def discoveries(run, events, require_confirmation):
    """Events found by one broker run. An event is identified by its left
    endpoint s; its right endpoint is the next shape of the valid set. Found =
    both endpoints measured successfully by queries completed within budget
    (and, if required, confirmed by a within-budget confirmation).
    Returns {event_s: cumulative cost at discovery}."""
    valid = run["valid"]
    pairs = {s: _next_valid(valid, s) for s in events if s in valid}
    ok_q = {q["query_index"] for q in run["timeline"] if q["within_budget"]}
    conf_at = {tuple(c["pair"]): c for c in run["candidates"] if c["confirmed"] and c["query_index"] in ok_q}
    measured, found = set(), {}
    for q in run["timeline"]:
        if not q["within_budget"]:
            break
        if q["action"] == "measure" and not q.get("failure_type"):
            measured.add(q["padded_length"])
        for s, t in pairs.items():
            if s in found or t is None:
                continue
            if s in measured and t in measured:
                if not require_confirmation:
                    found[s] = q["cumulative_cost_ns"]
                elif (s, t) in conf_at:
                    found[s] = conf_at[(s, t)]["cumulative_cost_ns"]
    return found


def recall_cost(runs_by_budget, events, require_confirmation):
    """runs_by_budget: {budget_ns: [run per seed]} -> rows with recall stats.
    Candidates count only if their query completed within the budget (§8.4)."""
    rows = []
    for b, runs in sorted(runs_by_budget.items()):
        rec, cands, conf, unused = [], [], [], []
        for r in runs:
            rec.append(len(discoveries(r, events, require_confirmation)) / len(events) if events else float("nan"))
            ok_q = {q["query_index"] for q in r["timeline"] if q["within_budget"]}
            cs = [c for c in r["candidates"] if c["query_index"] in ok_q]
            cands.append(len(cs))
            conf.append(sum(c["confirmed"] for c in cs))
            used = max([q["cumulative_cost_ns"] for q in r["timeline"] if q["within_budget"]] or [0])
            unused.append(1 - used / b if b else 0.0)
        rows.append({"budget_ns": b, "n_runs": len(runs), "recall_mean": float(np.mean(rec)),
                     "recall_min": float(np.min(rec)), "recall_max": float(np.max(rec)),
                     "candidates_mean": float(np.mean(cands)),
                     "confirmed_fraction": (float(np.sum(conf)) / np.sum(cands)) if np.sum(cands) else None,
                     "unused_budget_fraction_mean": float(np.mean(unused))})
    return rows

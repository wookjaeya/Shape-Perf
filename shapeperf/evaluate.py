"""Evaluator-side analysis (spec §9, §10.3, §11.1). Selectors never import this.

Everything numeric that defines an event (delta, alpha, direction, minimum
blocks/allocations, match tolerance, process statistic) is passed in from the
frozen preregistration.

Inference unit (spec §10.1, §10.3): effects are paired within a block (same
process-order block on the same VM allocation). When a pair has data from >= 2
allocations, the unit of the test is the allocation mean of its block effects
(cluster level); otherwise the unit is the block and the claim is limited to
that allocation. The test is a t-based minimum-effect test (H0: |effect| <=
log(1+delta)); the hierarchical bootstrap is kept only for a descriptive CI.
(A percentile-bootstrap p-value was found to be strongly anti-conservative at
Holm's per-test levels with few blocks - see docs/STATUS.md.)
"""
import math
from collections import Counter, defaultdict

import numpy as np
from scipy import stats as st

# ---------------------------------------------------------------------------
# Statistics helpers


def holm(pvals):
    """Holm step-down adjusted p-values (R22), same order as input."""
    p = np.asarray(pvals, float)
    order = np.argsort(p, kind="stable")
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


def block_table(records, stat, flagset=None):
    """{(allocation, block): {length: [process statistic, ...]}} from raw
    measurement records of one flag set (records without a flagset field are
    'default'). Failed records are excluded here; callers report them."""
    t = defaultdict(lambda: defaultdict(list))
    for r in records:
        if r.get("failure_type"):
            continue
        if flagset is not None and r.get("flagset", "default") != flagset:
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
    """Two-level bootstrap (allocations, then blocks) of the mean - descriptive only."""
    groups = [np.asarray(v, float) for v in by_group.values() if len(v)]
    if not groups:
        return None
    if len(groups) == 1:
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


def test_units(by_alloc):
    """Units of inference: allocation means when >=2 allocations, else blocks."""
    allocs = {a: v for a, v in by_alloc.items() if len(v)}
    if len(allocs) >= 2:
        return [float(np.mean(v)) for v in allocs.values()], "allocation"
    if allocs:
        return [float(x) for x in next(iter(allocs.values()))], "block (single allocation)"
    return [], "none"


def effect_test(by_alloc, delta, conf_level, direction="two-sided", min_blocks=2, min_allocations=1,
                reps=0, rng=None):
    """Minimum-effect t test of H0: effect in [-thr, thr], thr = log(1+delta).

    direction: "two-sided" H1: |effect| > thr (p = 2 * min of the one-sided p's)
               "increase"  H1: effect > thr     "decrease" H1: effect < -thr
    Pairs with fewer than min_blocks blocks, fewer than min_allocations
    allocations, fewer than 2 test units, or zero spread are 'insufficient'."""
    blocks = sum(len(v) for v in by_alloc.values())
    n_alloc = sum(1 for v in by_alloc.values() if len(v))
    units, unit_kind = test_units(by_alloc)
    n = len(units)
    base = {"n_blocks": blocks, "n_allocations": n_alloc, "unit": unit_kind, "n_units": n}
    if blocks < max(1, min_blocks) or n_alloc < max(1, min_allocations) or n < 2:
        return {"insufficient": True, **base}
    x = np.asarray(units, float)
    mean, sd = float(x.mean()), float(x.std(ddof=1))
    if sd <= 1e-12 * max(1.0, abs(mean)):
        return {"insufficient": True, "reason": "zero spread", **base}
    se = sd / math.sqrt(n)
    df = n - 1
    thr = math.log1p(delta)
    p_up = float(st.t.sf((mean - thr) / se, df))       # H0: effect <= thr
    p_down = float(st.t.cdf((mean + thr) / se, df))    # H0: effect >= -thr
    if direction == "two-sided":
        p = min(1.0, 2 * min(p_up, p_down))
    elif direction == "increase":
        p = p_up
    elif direction == "decrease":
        p = p_down
    else:
        raise ValueError(direction)
    q = float(st.t.ppf(1 - (1 - conf_level) / 2, df))
    out = {"log_effect": mean, "ratio": math.exp(mean), "ci_log": [mean - q * se, mean + q * se],
           "p_exceeds_delta": p, "direction": direction, "sd_units": sd, **base}
    if reps and rng is not None:
        boot = hier_bootstrap(by_alloc, reps, rng)
        out["bootstrap_ci_log_descriptive"] = [float(v) for v in
                                               np.quantile(boot, [(1 - conf_level) / 2, 1 - (1 - conf_level) / 2])]
    return out


# ---------------------------------------------------------------------------
# Continuous indicators and answer table A (spec §9.1-9.3)


def bert_flops(s, cfg):
    """Forward multiply-add count x2 of the BERT encoder + SQuAD head for one
    sequence of length s (compute-growth reference for A(s); not a time model)."""
    H, I, N = cfg["hidden_size"], cfg["intermediate_size"], cfg["num_hidden_layers"]
    per_layer = 8 * s * H * H + 4 * s * s * H + 4 * s * H * I
    return N * per_layer + 4 * s * H


def adjacent_pairs(valid):
    vs = sorted(int(v) for v in valid)
    return list(zip(vs, vs[1:]))


def continuous_indicators(table, valid, conf_level, delta, flop_cfg=None, direction="two-sided",
                          min_blocks=2, min_allocations=1, reps=0, seed=0):
    """A(s): one row per adjacent pair (s, t) of the EXPLICIT valid set - the
    predefined test family. Pairs without enough paired data are 'missing'."""
    rng = np.random.default_rng(seed)
    rows = []
    for s, t in adjacent_pairs(valid):
        r = effect_test(paired_log_ratios(table, s, t), delta, conf_level, direction, min_blocks,
                        min_allocations, reps, rng)
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


def answer_table_a(valid, discovery_table, confirmation_table, delta, alpha, conf_level,
                   direction="two-sided", min_blocks=2, min_allocations=1, flop_cfg=None, reps=0, seed=0):
    """Candidates: Holm-adjusted p < alpha on discovery data over the family of
    ALL adjacent pairs of the explicit valid set (pairs without data enter with
    p = 1, so the family does not depend on which lengths failed, §10.3).
    Events: candidates whose effect exceeds delta in the discovery direction on
    independent confirmation data (one-sided, Holm over the candidates).
    Signatures play no role (spec §9.3)."""
    rows = continuous_indicators(discovery_table, valid, conf_level, delta, flop_cfg, direction,
                                 min_blocks, min_allocations, reps, seed)
    adj = holm([1.0 if r.get("missing") else r["p_exceeds_delta"] for r in rows]) if rows else []
    cands = []
    for r, a in zip(rows, adj):
        r["p_holm"] = float(a)
        if not r.get("missing") and a < alpha:
            cands.append(r)
    conf = []
    for r in cands:
        d = "increase" if r["log_effect"] > 0 else "decrease"
        c = effect_test(paired_log_ratios(confirmation_table, r["s"], r["t"]), delta, conf_level, d,
                        min_blocks, min_allocations)
        conf.append(None if c.get("insufficient") else c)
    padj = holm([c["p_exceeds_delta"] if c else 1.0 for c in conf]) if conf else []
    events = []
    for r, c, a in zip(cands, conf, padj):
        same_dir = c is not None and np.sign(c["log_effect"]) == np.sign(r["log_effect"])
        ok = bool(c is not None and a < alpha and same_dir)
        events.append({"s": r["s"], "t": r["t"], "discovery": r, "confirmation": c,
                       "confirmation_p_holm": float(a), "confirmed": ok})
    return {"valid_lengths": sorted(int(v) for v in valid),
            "candidates": [e["s"] for e in events], "events": [e["s"] for e in events if e["confirmed"]],
            "event_pairs": [[e["s"], e["t"]] for e in events if e["confirmed"]],
            "detail": events, "continuous": rows, "family_size": len(rows),
            "n_missing_pairs": sum(1 for r in rows if r.get("missing"))}


def check_independent(discovery_records, confirmation_records, require_new_allocation):
    """Spec §9.3: confirmation must come from a separate run/order (and, if
    preregistered, a separate VM allocation). Raises ValueError otherwise."""
    def ids(recs, key):
        return {r.get(key) for r in recs if r.get(key) is not None}
    shared_blocks = ids(discovery_records, "block_id") & ids(confirmation_records, "block_id")
    shared_runs = ids(discovery_records, "run_id") & ids(confirmation_records, "run_id")
    shared_alloc = ids(discovery_records, "vm_allocation_id") & ids(confirmation_records, "vm_allocation_id")
    if shared_blocks or shared_runs:
        raise ValueError(f"confirmation data are not independent: shared blocks {sorted(shared_blocks)[:3]}, "
                         f"shared runs {sorted(shared_runs)[:3]}")
    if require_new_allocation and shared_alloc:
        raise ValueError(f"confirmation shares VM allocation(s) {sorted(shared_alloc)} with discovery")
    return {"shared_allocations": sorted(shared_alloc)}


def per_shape_summary(records, stat):
    """Spec §10.3: per shape n, mean latency and dispersion of process statistics."""
    by = defaultdict(list)
    fails = Counter()
    for r in records:
        if r.get("failure_type"):
            fails[r["padded_length"]] += 1
            continue
        by[r["padded_length"]].append(process_stat(r["latency_ns"], stat))
    out = {}
    for s in sorted(set(by) | set(fails)):
        x = np.asarray(by.get(s, []), float)
        out[str(s)] = {"n_processes": int(x.size), "n_failed": int(fails[s]),
                       "mean_ns": float(x.mean()) if x.size else None,
                       "sd_ns": float(x.std(ddof=1)) if x.size > 1 else None,
                       "min_ns": float(x.min()) if x.size else None, "max_ns": float(x.max()) if x.size else None}
    return out


def alternative_indicators(default_table, alt_table, valid, conf_level):
    """Spec §9.1: R(s) = T_default(s)/T_alt(s) and Q(s) = R(t)/R(s) for adjacent
    (s, t), paired within blocks that measured both configurations."""
    keys = set(default_table) & set(alt_table)
    r_rows, q_rows = [], []

    def ci(x):
        x = np.asarray(x, float)
        if x.size < 2:
            return None
        se = x.std(ddof=1) / math.sqrt(x.size)
        q = float(st.t.ppf(1 - (1 - conf_level) / 2, x.size - 1))
        return [float(x.mean() - q * se), float(x.mean() + q * se)]

    def logr(k, s):
        d, a = default_table[k], alt_table[k]
        if s in d and s in a:
            return math.log(np.mean(d[s]) / np.mean(a[s]))
        return None

    for s in sorted(valid):
        v = [x for x in (logr(k, s) for k in keys) if x is not None]
        r_rows.append({"s": s, "n_blocks": len(v), "log_R": float(np.mean(v)) if v else None,
                       "R": math.exp(np.mean(v)) if v else None, "ci_log_R": ci(v)})
    for s, t in adjacent_pairs(valid):
        v = []
        for k in keys:
            a, b = logr(k, s), logr(k, t)
            if a is not None and b is not None:
                v.append(b - a)
        q_rows.append({"s": s, "t": t, "n_blocks": len(v), "log_Q": float(np.mean(v)) if v else None,
                       "Q": math.exp(np.mean(v)) if v else None, "ci_log_Q": ci(v)})
    return {"R": r_rows, "Q": q_rows,
            "note": "R>1: the alternative is faster at s; Q: change of R across the boundary (spec §9.1)"}


def natural_weighted_impact(event_detail, length_freq):
    """Spec §11.1 secondary: SQuAD-derived impact of confirmed events. An event
    (s, t) with ratio r affects features whose natural length is t; the impact
    is sum over events of freq(t)/N * (r - 1). This is a property of the SQuAD
    preprocessing distribution, not a service effect."""
    n = sum(length_freq.values())
    rows = []
    tot = 0.0
    for e in event_detail:
        if not e.get("confirmed"):
            continue
        f = length_freq.get(str(e["t"]), length_freq.get(e["t"], 0))
        r = e["discovery"]["ratio"]
        w = f / n if n else 0.0
        rows.append({"s": e["s"], "t": e["t"], "ratio": r, "freq_t": f, "weighted_excess": w * (r - 1)})
        tot += w * (r - 1)
    return {"total_weighted_excess": tot, "events": rows}


# ---------------------------------------------------------------------------
# Census metrics (spec §9.4)


def change_pairs(sig_by_len, valid):
    """Adjacent pairs (s, t) of the explicit valid set whose signatures differ.
    Every valid length must have an entry (failures as 'FAILED:<type>')."""
    return [(s, t) for s, t in adjacent_pairs(valid)
            if s in sig_by_len and t in sig_by_len and sig_by_len[s] != sig_by_len[t]]


def change_points(sig_by_len, valid):
    """Left endpoints of change_pairs."""
    return [s for s, _ in change_pairs(sig_by_len, valid)]


def split_failure_boundaries(sig_by_len, valid):
    """(decision change points, failure boundaries): a boundary touching a
    failed length is not a lowering-decision change (spec §1.2 H1)."""
    dec, fail = [], []
    for s, t in change_pairs(sig_by_len, valid):
        a, b = str(sig_by_len[s]), str(sig_by_len[t])
        (fail if a.startswith("FAILED:") or b.startswith("FAILED:") else dec).append(s)
    return dec, fail


def aligned_boundaries(valid, units):
    """B_align: left endpoints s of adjacent valid pairs (s, t) whose closed
    range [s, t] contains a multiple of some unit (> 1). For a contiguous set
    this is 's or s+1 is a multiple'."""
    us = [u for u in units if u > 1]
    return [s for s, t in adjacent_pairs(valid)
            if any(math.floor(t / u) * u >= s for u in us)]


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
    endpoint s; its right endpoint is the next shape of the run's valid set.
    Found = both endpoints measured successfully by entries completed within
    budget; with require_confirmation, a within-budget confirmation entry for
    exactly that pair must also have succeeded. Returns {s: cost at discovery}
    (resolved-only: the measurement completing the pair; confirmed: the
    confirmation entry)."""
    valid = list(run["valid"])
    pairs = {s: _next_valid(valid, s) for s in events if s in valid}
    measured, found = set(), {}
    for q in run["timeline"]:
        if not q["within_budget"]:
            break
        if require_confirmation:
            if q["action"] == "confirm" and q.get("confirmed"):
                a, b = q["pair"]
                if a in pairs and pairs[a] == b and a not in found:
                    found[a] = q["cumulative_cost_ns"]
            continue
        if q["action"] == "measure" and not q.get("failure_type"):
            measured.add(q["padded_length"])
            for s, t in pairs.items():
                if s not in found and t is not None and s in measured and t in measured:
                    found[s] = q["cumulative_cost_ns"]
    return found


def _mean_ci(x, conf_level=0.95):
    x = np.asarray(x, float)
    if x.size < 2 or np.all(x == x[0]):
        return [float(x.mean()), float(x.mean())] if x.size else [None, None]
    se = x.std(ddof=1) / math.sqrt(x.size)
    q = float(st.t.ppf(1 - (1 - conf_level) / 2, x.size - 1))
    return [float(x.mean() - q * se), float(x.mean() + q * se)]


def recall_cost(runs_by_budget, events, require_confirmation, conf_level=0.95):
    """runs_by_budget: {budget_ns: [run per seed]} -> rows with recall stats,
    uncertainty over seeds (§10.3) and the common vs policy-extra cost split
    (§8.4). Only entries completed within the budget count."""
    rows = []
    for b, runs in sorted(runs_by_budget.items()):
        rec, cands, conf, unused, common, extra, sel = [], [], [], [], [], [], []
        for r in runs:
            rec.append(len(discoveries(r, events, require_confirmation)) / len(events) if events else float("nan"))
            ok = [q for q in r["timeline"] if q["within_budget"]]
            cs = [q for q in ok if q["action"] == "confirm"]
            cands.append(len(cs))
            conf.append(sum(bool(q.get("confirmed")) for q in cs))
            used = max([q["cumulative_cost_ns"] for q in ok] or [0])
            unused.append(1 - used / b if b else 0.0)
            common.append(sum(q.get("common_ns", 0) for q in ok))
            extra.append(sum(q.get("policy_extra_ns", 0) for q in ok))
            sel.append(sum(q.get("selection_ns", 0) for q in ok))
        rows.append({"budget_ns": b, "n_runs": len(runs), "recall_mean": float(np.mean(rec)),
                     "recall_min": float(np.min(rec)), "recall_max": float(np.max(rec)),
                     "recall_ci_mean": _mean_ci(rec, conf_level),
                     "recall_q025_q975": [float(np.quantile(rec, 0.025)), float(np.quantile(rec, 0.975))],
                     "candidates_mean": float(np.mean(cands)),
                     "confirmed_fraction": (float(np.sum(conf)) / np.sum(cands)) if np.sum(cands) else None,
                     "unused_budget_fraction_mean": float(np.mean(unused)),
                     "common_ns_mean": float(np.mean(common)), "policy_extra_ns_mean": float(np.mean(extra)),
                     "selection_ns_mean": float(np.mean(sel))})
    return rows

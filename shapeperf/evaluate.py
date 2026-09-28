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
        t[(r.get("vm_allocation_id") or "unavailable", r["block_id"])][r["padded_length"]].append(
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
                   direction="two-sided", min_blocks=2, min_allocations=1, flop_cfg=None, reps=0, seed=0,
                   min_allocations_confirmation=None):
    """Candidates: Holm-adjusted p < alpha on discovery data over the family of
    ALL adjacent pairs of the explicit valid set (pairs without data enter with
    p = 1, so the family does not depend on which lengths failed, §10.3).
    Events: candidates whose effect exceeds delta in the discovery direction on
    independent confirmation data (one-sided, Holm over the candidates).
    Signatures play no role (spec §9.3). The confirmation stage has its own
    allocation minimum (one new allocation is the usual design)."""
    if min_allocations_confirmation is None:
        min_allocations_confirmation = min_allocations
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
                        min_blocks, min_allocations_confirmation)
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
    """Spec §9.3: confirmation must come from a separate run and comparison
    order (and, if preregistered, a separate VM allocation). Raises ValueError
    otherwise.

    run   : the G4 run id (g4_run_id, one per groundtruth_dense invocation), not
            the per-process run_id; records without it make run identity
            'unverified' (callers refuse that for frozen runs)
    order : the block seeds that fix the process order within blocks
    allocation: a missing/empty vm_allocation_id is 'unavailable' (as in
            block_table). When a new allocation is required, 'unavailable' on
            EITHER side fails: an unknown allocation cannot be shown to be new"""
    def ids(recs, key, missing=None):
        out = set()
        for r in recs:
            v = r.get(key)
            if v is None:
                v = missing
            if v is not None:
                out.add(v)
        return out
    shared_blocks = ids(discovery_records, "block_id") & ids(confirmation_records, "block_id")
    d_runs, c_runs = ids(discovery_records, "g4_run_id"), ids(confirmation_records, "g4_run_id")
    shared_runs = d_runs & c_runs
    shared_seeds = ids(discovery_records, "seed") & ids(confirmation_records, "seed")
    def allocs(recs):
        return {r.get("vm_allocation_id") or "unavailable" for r in recs}
    d_alloc, c_alloc = allocs(discovery_records), allocs(confirmation_records)
    shared_alloc = d_alloc & c_alloc
    wrong_role = sorted({r.get("g4_role") for r in confirmation_records if r.get("g4_role") not in (None, "confirmation")}
                        | {str(r.get("experiment_phase")) for r in confirmation_records
                           if str(r.get("experiment_phase", "")).startswith("g4-discovery")})
    problems = []
    if shared_blocks:
        problems.append(f"shared blocks {sorted(shared_blocks)[:3]}")
    if shared_runs:
        problems.append(f"shared G4 runs {sorted(shared_runs)[:3]}")
    if shared_seeds:
        problems.append(f"same block order seeds {sorted(shared_seeds)[:3]}")
    if wrong_role:
        problems.append(f"confirmation records labelled {wrong_role[:3]}")
    if problems:
        raise ValueError("confirmation data are not independent: " + "; ".join(problems))
    if require_new_allocation and "unavailable" in d_alloc | c_alloc:
        raise ValueError("a new VM allocation is required but some records have no vm_allocation_id "
                         "('unavailable'), so the allocations cannot be shown to differ")
    if require_new_allocation and shared_alloc:
        raise ValueError(f"confirmation shares VM allocation(s) {sorted(shared_alloc)} with discovery")
    no_run_id = (any(r.get("g4_run_id") is None for r in discovery_records)
                 or any(r.get("g4_run_id") is None for r in confirmation_records))
    return {"shared_allocations": sorted(shared_alloc),
            "run_identity": "unverified (records without g4_run_id)" if no_run_id else "verified",
            "discovery_g4_runs": sorted(d_runs), "confirmation_g4_runs": sorted(c_runs)}


def select_flagset(records, flagset):
    """Records of one flag set (records without the field are 'default')."""
    if flagset is None:
        return list(records)
    return [r for r in records if r.get("flagset", "default") == flagset]


def per_shape_summary(records, stat, flagset=None):
    """Spec §10.3: per shape n, mean latency and dispersion of process statistics
    for one flag set."""
    by = defaultdict(list)
    fails = Counter()
    for r in select_flagset(records, flagset):
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

    def ci(by_alloc):
        """t interval over the test units (allocation means when >= 2
        allocations, else blocks - the same rule as effect_test)."""
        units, kind = test_units(by_alloc)
        x = np.asarray(units, float)
        info = {"unit": kind, "n_units": int(x.size), "n_allocations": sum(1 for v in by_alloc.values() if v)}
        if x.size < 2:
            return None, info
        se = x.std(ddof=1) / math.sqrt(x.size)
        q = float(st.t.ppf(1 - (1 - conf_level) / 2, x.size - 1))
        return [float(x.mean() - q * se), float(x.mean() + q * se)], info

    def logr(k, s):
        d, a = default_table[k], alt_table[k]
        if s in d and s in a:
            return math.log(np.mean(d[s]) / np.mean(a[s]))
        return None

    def row(by_alloc, name):
        units = test_units(by_alloc)[0]
        c, info = ci(by_alloc)
        m = float(np.mean(units)) if units else None
        return {"n_blocks": sum(len(v) for v in by_alloc.values()), f"log_{name}": m,
                name: math.exp(m) if m is not None else None, f"ci_log_{name}": c, **info}

    for s in sorted(valid):
        by_alloc = defaultdict(list)
        for k in keys:
            x = logr(k, s)
            if x is not None:
                by_alloc[k[0]].append(x)
        r_rows.append({"s": s, **row(by_alloc, "R")})
    for s, t in adjacent_pairs(valid):
        by_alloc = defaultdict(list)
        for k in keys:
            a, b = logr(k, s), logr(k, t)
            if a is not None and b is not None:
                by_alloc[k[0]].append(b - a)
        q_rows.append({"s": s, "t": t, **row(by_alloc, "Q")})
    return {"R": r_rows, "Q": q_rows,
            "note": "R>1: the alternative is faster at s; Q: change of R across the boundary (spec §9.1)"}


def natural_weighted_impact(select_means, eval_means, event_pairs, length_freq, valid):
    """Spec §11.1 secondary, descriptive: natural-length-weighted avoidable
    padding excess, cross-fitted.

    A feature of natural length L can run at any valid padded length s >= L.
    The best padding best(L) = argmin_{s >= L} T_select(s) is chosen on one data
    set (discovery) and the excess T_eval(L) / T_eval(best(L)) - 1 is evaluated
    on the other (independent confirmation), so noise in the minimum does not
    make the excess positive by construction; the estimate can be negative. The
    total weights it by the SQuAD natural-length frequency over lengths in the
    valid set that both data sets measured. The part 'across confirmed events'
    counts lengths whose best padding lies across a confirmed event boundary
    (L <= s < t <= best). A point estimate without an interval; a property of
    the SQuAD preprocessing distribution, not a service effect. Returns
    'unavailable' without a frequency catalog."""
    if not length_freq:
        return {"status": "unavailable (no natural-length frequency catalog)"}
    freq = {int(k): v for k, v in length_freq.items()}
    n = sum(freq.values())
    vs = sorted(int(v) for v in valid)
    vset = set(vs)

    def restrict(m):
        return {int(k): float(v) for k, v in m.items() if v is not None and int(k) in vset}
    sel, ev = restrict(select_means), restrict(eval_means)
    rows, tot, tot_ev, covered = [], 0.0, 0.0, 0
    for L in sorted(freq):
        if L not in sel or L not in ev:
            continue
        cand = [s for s in vs if s >= L and s in sel and s in ev]
        if not cand:
            continue
        best = min(cand, key=lambda s: (sel[s], s))
        ex = ev[L] / ev[best] - 1.0
        w = freq[L] / n if n else 0.0
        across = best > L and any(L <= a and b <= best for a, b in event_pairs)
        covered += freq[L]
        tot += w * ex
        tot_ev += w * ex if across else 0.0
        if best != L:
            rows.append({"L": L, "best_padding": best, "excess": ex, "freq": freq[L],
                         "across_confirmed_event": across})
    return {"status": "ok", "definition": "sum_L freq(L)/N * (T_conf(L)/T_conf(argmin_{s>=L} T_disc(s)) - 1)",
            "total_weighted_excess": tot, "weighted_excess_across_confirmed_events": tot_ev,
            "frequency_covered": covered / n if n else None, "lengths_with_other_best_padding": rows,
            "note": "descriptive point estimate (cross-fitted), no interval"}


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


def _is_failed(x):
    return x is None or str(x).startswith(("FAILED:", "CORRUPT:"))


def split_failure_boundaries(sig_by_len, valid):
    """-> (decision change points, failure boundaries, decision change spans).

    A boundary touching a failed length is not itself a lowering-decision change
    (spec §1.2 H1); it is listed in failure boundaries. The decision is instead
    compared between the nearest successful lengths on either side of a run of
    failed lengths: if they differ, the change lies somewhere in [a, b] and is
    reported as the span (a, b) with change point a. Adjacent successful pairs
    give spans (s, t). Unusable values (None, CORRUPT:) count as failed here."""
    valid = sorted(valid)
    fail = [s for s, t in change_pairs(sig_by_len, valid)
            if str(sig_by_len[s]).startswith("FAILED:") or str(sig_by_len[t]).startswith("FAILED:")]
    ok = [s for s in valid if s in sig_by_len and not _is_failed(sig_by_len[s])]
    spans = [(a, b) for a, b in zip(ok, ok[1:]) if sig_by_len[a] != sig_by_len[b]]
    return [a for a, _ in spans], fail, spans


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


def census_metrics(c_sig, b_align, e_a, tol, units=None):
    """c_sig: change points s (adjacent pair (s, s+1 in the valid set)) or spans
    [a, b] (a change somewhere in [a, b], e.g. across failed lengths). A span
    matches an event e when a - tol <= e <= b - 1 + tol; it is aligned when
    [a, b] contains a multiple of a unit (needs units) - for adjacent pairs this
    is exactly B_align."""
    spans = sorted({(int(c[0]), int(c[1])) if isinstance(c, (list, tuple)) else (int(c), None) for c in c_sig})
    b_align, e_a = set(b_align), set(e_a)
    us = [u for u in (units or []) if u > 1]

    def aligned(a, b):
        if b is None or units is None:
            return a in b_align
        return any(math.floor(b / u) * u >= a for u in us)

    def hits(a, b, e):
        hi = (b - 1) if b is not None else a
        return a - tol <= e <= hi + tol

    non = [(a, b) for a, b in spans if not aligned(a, b)]
    hit = [c for c in spans if any(hits(*c, e) for e in e_a)]
    ev_hit = {e for e in e_a if any(hits(*c, e) for c in spans)}
    ev_non = {e for e in e_a if any(hits(*c, e) for c in non)}
    return {
        "n_C_sig": len(spans), "n_B_align": len(b_align), "n_C_nonalign": len(non), "n_E_A": len(e_a),
        "C_nonalign": sorted(a for a, _ in non),
        "C_nonalign_spans": [[a, b] for a, b in non if b is not None],
        "Prec_sig": len(hit) / len(spans) if spans else None,
        "Rec_sig": len(ev_hit) / len(e_a) if e_a else None,
        "nonalign_contribution": len(ev_non) / len(e_a) if e_a else None,
        "H1_nonanalytic_changes_exist": bool(non),
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
    """t interval of the mean over seeds; None with fewer than 2 values (one
    seed carries no information about seed-to-seed variation)."""
    x = np.asarray(x, float)
    if x.size < 2:
        return None
    if np.all(x == x[0]):
        return [float(x[0]), float(x[0])]
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
            if events:
                rec.append(len(discoveries(r, events, require_confirmation)) / len(events))
            ok = [q for q in r["timeline"] if q["within_budget"]]
            # candidates created by within-budget entries; confirmed = within-budget confirmation succeeded
            cs = [c for c in r.get("candidates", []) if c.get("created_within_budget")]
            cands.append(len(cs))
            conf.append(sum(1 for c in cs if c.get("confirmed") and c.get("confirmation_status") == "done"))
            used = max([q["cumulative_cost_ns"] for q in ok] or [0])
            unused.append(1 - used / b if b else 0.0)
            common.append(sum(q.get("common_ns", 0) for q in ok))
            extra.append(sum(q.get("policy_extra_ns", 0) for q in ok))
            sel.append(sum(q.get("selection_ns", 0) for q in ok))
        rows.append({"budget_ns": b, "n_runs": len(runs),
                     "recall_mean": float(np.mean(rec)) if rec else None,
                     "recall_min": float(np.min(rec)) if rec else None,
                     "recall_max": float(np.max(rec)) if rec else None,
                     "recall_ci_mean": _mean_ci(rec, conf_level) if rec else None,
                     "recall_q025_q975": [float(np.quantile(rec, 0.025)), float(np.quantile(rec, 0.975))]
                     if rec else None,
                     "recall_note": None if events else "E_A empty: recall undefined",
                     "candidates_mean": float(np.mean(cands)),
                     "confirmed_fraction": (float(np.sum(conf)) / np.sum(cands)) if np.sum(cands) else None,
                     "unused_budget_fraction_mean": float(np.mean(unused)),
                     "common_ns_mean": float(np.mean(common)), "policy_extra_ns_mean": float(np.mean(extra)),
                     "selection_ns_mean": float(np.mean(sel))})
    return rows

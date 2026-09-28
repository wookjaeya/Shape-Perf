"""Evaluator statistics on synthetic block data (numbers are synthetic)."""
import math

import numpy as np
import pytest

from shapeperf import evaluate as E


def test_holm_matches_definition():
    p = [0.01, 0.04, 0.03, 0.2]
    adj = E.holm(p)
    # sorted: 0.01*4=0.04, 0.03*3=0.09, 0.04*2=0.08->max 0.09, 0.2*1=0.2
    assert np.allclose(adj, [0.04, 0.09, 0.09, 0.2])


def test_change_points_alignment_and_failures():
    sig = {s: ("a" if s < 10 else "b" if s < 17 else "c") for s in range(1, 33)}
    assert E.change_points(sig, range(1, 33)) == [9, 16]
    b = E.aligned_boundaries(range(1, 33), [8])
    assert 8 in b and 7 in b and 16 in b and 15 in b and 9 not in b
    m = E.census_metrics([9, 16], b, [16], tol=0)
    assert m["C_nonalign"] == [9] and m["Prec_sig"] == 0.5 and m["Rec_sig"] == 1.0
    assert m["nonalign_contribution"] == 0.0 and m["H1_nonanalytic_changes_exist"]
    sig[20] = "FAILED:timeout"
    dec, fail = E.split_failure_boundaries(sig, range(1, 33))
    assert dec == [9, 16] and fail == [19, 20]


def test_aligned_boundaries_across_a_gap():
    valid = [s for s in range(1, 33) if s not in (15, 16, 17)]
    b = E.aligned_boundaries(valid, [8])
    assert 14 in b            # (14, 18) spans the multiple 16
    assert E.aligned_boundaries(range(1, 33), []) == []


def _records(steps, blocks=6, procs=3, seed=0, alloc="vm1", noise=0.01, lengths=range(1, 21), tag="d"):
    rng = np.random.default_rng(seed)
    recs = []
    for b in range(blocks):
        for s in lengths:
            base = 1e6 * (1 + 0.02 * s) * float(np.prod([f for k, f in steps.items() if s >= k]))
            for _ in range(procs):
                lat = base * np.exp(rng.normal(0, noise, 20))
                recs.append({"padded_length": s, "block_id": f"{tag}{seed}-b{b}", "vm_allocation_id": alloc,
                             "latency_ns": lat.tolist(), "failure_type": None})
    return recs


def test_answer_table_finds_step_and_ignores_trend():
    disc = E.block_table(_records({11: 1.3}, seed=1), "mean")
    conf = E.block_table(_records({11: 1.3}, seed=2, tag="c"), "mean")
    tab = E.answer_table_a(range(1, 21), disc, conf, delta=0.1, alpha=0.05, conf_level=0.95)
    assert tab["events"] == [10] and tab["event_pairs"] == [[10, 11]]
    r10 = next(r for r in tab["continuous"] if r["s"] == 10)
    assert abs(r10["ratio"] - 1.3 * (1 + 0.02 * 11) / (1 + 0.02 * 10)) < 0.02
    assert r10["unit"].startswith("block")


def test_answer_table_requires_independent_confirmation():
    disc = E.block_table(_records({11: 1.3}, seed=1), "mean")
    conf = E.block_table(_records({}, seed=2, tag="c"), "mean")          # effect absent in confirmation
    tab = E.answer_table_a(range(1, 21), disc, conf, delta=0.1, alpha=0.05, conf_level=0.95)
    assert tab["candidates"] == [10] and tab["events"] == []


def test_independence_check():
    a = _records({}, seed=1, tag="x")
    with pytest.raises(ValueError):
        E.check_independent(a, a, require_new_allocation=False)            # same blocks
    b = _records({}, seed=2, tag="y")
    assert E.check_independent(a, b, False)["shared_allocations"] == ["vm1"]
    with pytest.raises(ValueError):
        E.check_independent(a, b, require_new_allocation=True)


def test_single_block_and_zero_spread_are_insufficient():
    assert E.effect_test({"vm1": [math.log(1.25)]}, 0.1, 0.95).get("insufficient")
    assert E.effect_test({"vm1": [0.2, 0.2, 0.2]}, 0.1, 0.95).get("insufficient")
    assert E.effect_test({"vm1": [0.2, 0.25]}, 0.1, 0.95, min_blocks=4).get("insufficient")


def test_allocations_become_the_test_unit():
    r = E.effect_test({"a": [0.3, 0.31, 0.29], "b": [0.33, 0.32]}, 0.1, 0.95)
    assert r["unit"] == "allocation" and r["n_units"] == 2


def _null_units(rng, n, eff):
    return {"vm": (eff + rng.normal(0, 1, n)).tolist()}


@pytest.mark.parametrize("n", [3, 5, 10])
def test_minimum_effect_test_size_at_holm_levels(n):
    """Size of the per-pair test at alpha/m with few blocks, at the boundary null
    (true effect exactly thr): must not exceed nominal (t test is exact under normality)."""
    rng = np.random.default_rng(7)
    level, delta = 0.005, 0.5
    thr = math.log1p(delta)
    sims = 4000
    rej = sum(E.effect_test(_null_units(rng, n, thr), delta, 0.95, "increase")["p_exceeds_delta"] < level
              for _ in range(sims))
    assert rej / sims < 2.5 * level, rej / sims


def test_answer_table_fwer_with_few_blocks():
    """Global null, 20 pairs, 4 blocks each: family-wise false-candidate rate stays
    near alpha (the replaced percentile-bootstrap test had FWER 0.2-0.99 here)."""
    rng = np.random.default_rng(11)
    alpha, trials, fw = 0.05, 300, 0
    valid = range(1, 22)
    for _ in range(trials):
        tab = {}
        for b in range(4):
            lat = np.exp(np.cumsum(rng.normal(0, 0.02, 21)))
            tab[("vm", f"b{b}")] = {s: [float(lat[s - 1])] for s in valid}
        rows = E.continuous_indicators(tab, valid, 0.95, 0.0, direction="two-sided")
        adj = E.holm([1.0 if r.get("missing") else r["p_exceeds_delta"] for r in rows])
        fw += bool(np.any(adj < alpha))
    assert fw / trials < 0.09, fw / trials


def test_holm_family_is_fixed_in_advance():
    disc = E.block_table(_records({11: 1.3}, seed=1), "mean")
    tab = E.answer_table_a(range(1, 25), disc, disc, delta=0.1, alpha=0.05, conf_level=0.95)
    assert tab["family_size"] == 23 and tab["n_missing_pairs"] == 4      # 20..24 have no data


def test_per_shape_summary_and_natural_impact():
    recs = _records({11: 1.3}, seed=1, blocks=2, procs=2, lengths=[10, 11])
    recs.append({"padded_length": 12, "block_id": "z", "failure_type": "runtime_error", "latency_ns": []})
    ps = E.per_shape_summary(recs, "mean")
    assert ps["10"]["n_processes"] == 4 and ps["12"]["n_failed"] == 1 and ps["12"]["mean_ns"] is None
    detail = [{"s": 10, "t": 11, "confirmed": True, "discovery": {"ratio": 1.3}},
              {"s": 3, "t": 4, "confirmed": False, "discovery": {"ratio": 2.0}}]
    imp = E.natural_weighted_impact(detail, {"11": 25, "4": 75})
    assert math.isclose(imp["total_weighted_excess"], 0.25 * 0.3)


def test_alternative_indicators_r_and_q():
    recs = []
    for b in range(4):
        for fs in ("default", "alt"):
            for s in (10, 11):
                slow = 1.5 if (fs == "default" and s == 11) else 1.0
                recs.append({"padded_length": s, "block_id": f"b{b}", "flagset": fs, "failure_type": None,
                             "latency_ns": [1e6 * s * slow * (1 + 0.001 * b)]})
    res = E.alternative_indicators(E.block_table(recs, "mean", "default"), E.block_table(recs, "mean", "alt"),
                                   [10, 11], 0.95)
    r = {x["s"]: x for x in res["R"]}
    assert math.isclose(r[10]["R"], 1.0) and math.isclose(r[11]["R"], 1.5)
    assert math.isclose(res["Q"][0]["Q"], 1.5)


def test_flop_reference_is_increasing():
    cfg = {"hidden_size": 768, "intermediate_size": 3072, "num_hidden_layers": 12}
    f = [E.bert_flops(s, cfg) for s in range(1, 257)]
    assert all(b > a for a, b in zip(f, f[1:]))
    assert math.isclose(E.bert_flops(2, cfg) / E.bert_flops(1, cfg), 2.0, rel_tol=0.01)


def test_discoveries_use_valid_neighbours_and_measure_cost():
    run = {"valid": [1, 2, 4, 5], "candidates": [], "timeline": [
        {"action": "measure", "padded_length": 2, "within_budget": True, "cumulative_cost_ns": 10},
        {"action": "measure", "padded_length": 4, "within_budget": True, "cumulative_cost_ns": 20},
        {"action": "confirm", "pair": [2, 4], "confirmed": True, "within_budget": True, "cumulative_cost_ns": 30}]}
    assert E.discoveries(run, [2], require_confirmation=False) == {2: 20}
    assert E.discoveries(run, [2], require_confirmation=True) == {2: 30}

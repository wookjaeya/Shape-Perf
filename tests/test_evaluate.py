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


def test_change_points_and_alignment():
    sig = {s: ("a" if s < 10 else "b" if s < 17 else "c") for s in range(1, 33)}
    assert E.change_points(sig, range(1, 33)) == [9, 16]
    b = E.aligned_boundaries(range(1, 33), [8])
    assert 8 in b and 7 in b and 16 in b and 15 in b and 9 not in b
    m = E.census_metrics([9, 16], b, [16], tol=0)
    assert m["C_nonalign"] == [9] and m["Prec_sig"] == 0.5 and m["Rec_sig"] == 1.0
    assert m["nonalign_contribution"] == 0.0 and m["H1_nonanalytic_changes_exist"]


def _records(steps, blocks=6, procs=3, seed=0, alloc="vm1", noise=0.01):
    rng = np.random.default_rng(seed)
    recs = []
    for b in range(blocks):
        for s in range(1, 21):
            base = 1e6 * (1 + 0.02 * s) * float(np.prod([f for k, f in steps.items() if s >= k]))
            for _ in range(procs):
                lat = base * np.exp(rng.normal(0, noise, 20))
                recs.append({"padded_length": s, "block_id": f"b{b}", "vm_allocation_id": alloc,
                             "latency_ns": lat.tolist(), "failure_type": None})
    return recs


def test_answer_table_finds_step_and_ignores_trend():
    disc = E.block_table(_records({11: 1.3}, seed=1), "mean")
    conf = E.block_table(_records({11: 1.3}, seed=2), "mean")
    rows = E.continuous_indicators(disc, range(1, 21), reps=2000, seed=0, conf_level=0.95, delta=0.1)
    tab = E.answer_table_a(rows, conf, delta=0.1, alpha=0.05, reps=2000, seed=0, conf_level=0.95)
    assert tab["events"] == [10]
    r10 = next(r for r in rows if r["s"] == 10)
    assert abs(r10["ratio"] - 1.3 * (1 + 0.02 * 11) / (1 + 0.02 * 10)) < 0.02


def test_answer_table_requires_independent_confirmation():
    disc = E.block_table(_records({11: 1.3}, seed=1), "mean")
    conf = E.block_table(_records({}, seed=2), "mean")          # effect absent in confirmation
    rows = E.continuous_indicators(disc, range(1, 21), reps=2000, seed=0, conf_level=0.95, delta=0.1)
    tab = E.answer_table_a(rows, conf, delta=0.1, alpha=0.05, reps=2000, seed=0, conf_level=0.95)
    assert tab["candidates"] == [10] and tab["events"] == []


def test_single_block_is_insufficient():
    rng = np.random.default_rng(0)
    r = E.effect_test({"vm1": [math.log(1.25)]}, 0.1, 2000, rng, 0.95)
    assert r.get("insufficient")
    r2 = E.effect_test({"vm1": [math.log(1.25)] * 3}, 0.1, 2000, rng, 0.95, min_blocks=4)
    assert r2.get("insufficient")


def test_two_sided_test_is_calibrated_under_null():
    rng = np.random.default_rng(1)
    alpha, rej, n = 0.1, 0, 300
    for _ in range(n):
        x = rng.normal(0, 1, 30).tolist()
        r = E.effect_test({"vm": x}, 0.0, 400, rng, 0.95, direction="two-sided")
        rej += r["p_exceeds_delta"] < alpha
    assert rej / n < 0.16, rej / n            # the old sign-chosen one-sided rule gave ~0.2


def test_holm_family_is_fixed_in_advance():
    disc = E.block_table(_records({11: 1.3}, seed=1), "mean")
    rows = E.continuous_indicators(disc, range(1, 25), reps=2000, seed=0, conf_level=0.95, delta=0.1)
    assert sum(r.get("missing", False) for r in rows) == 4          # 20..24 have no data
    tab = E.answer_table_a(rows, disc, delta=0.1, alpha=0.05, reps=2000, seed=0, conf_level=0.95)
    assert tab["family_size"] == 23 and tab["n_missing_pairs"] == 4


def test_bootstrap_resolution_guard():
    # with reps < family/alpha no pair can ever reach Holm significance -> refuse
    disc = E.block_table(_records({11: 1.3}, seed=1), "mean")
    rows = E.continuous_indicators(disc, range(1, 21), reps=100, seed=0, conf_level=0.95, delta=0.1)
    with pytest.raises(ValueError):
        E.answer_table_a(rows, disc, delta=0.1, alpha=0.05, reps=100, seed=0, conf_level=0.95)


def test_flop_reference_is_increasing():
    cfg = {"hidden_size": 768, "intermediate_size": 3072, "num_hidden_layers": 12}
    f = [E.bert_flops(s, cfg) for s in range(1, 257)]
    assert all(b > a for a, b in zip(f, f[1:]))
    assert math.isclose(E.bert_flops(2, cfg) / E.bert_flops(1, cfg), 2.0, rel_tol=0.01)

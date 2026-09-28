"""Evaluator statistics on synthetic block data (numbers are synthetic)."""
import math

import numpy as np
import pytest

from shapeperf import evaluate as E
from shapeperf.util import REPO_ROOT as ROOT


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
    dec, fail, spans = E.split_failure_boundaries(sig, range(1, 33))
    assert dec == [9, 16] and fail == [19, 20] and spans == [(9, 10), (16, 17)]


def test_decision_change_hidden_behind_a_failed_length_is_kept():
    sig = {s: ("a" if s < 50 else "b") for s in range(41, 61)}
    sig[50] = "FAILED:compile_error"                 # the change 49 -> 51 sits on a failed length
    dec, fail, spans = E.split_failure_boundaries(sig, range(41, 61))
    assert dec == [49] and spans == [(49, 51)] and fail == [49, 50]
    # a span matches an event anywhere inside it and is non-aligned only if no multiple lies in [a, b]
    m = E.census_metrics([list(x) for x in spans], E.aligned_boundaries(range(41, 61), [11]), [50], 0, units=[11])
    assert m["Rec_sig"] == 1.0 and m["Prec_sig"] == 1.0 and m["n_C_nonalign"] == 1
    m = E.census_metrics([list(x) for x in spans], E.aligned_boundaries(range(41, 61), [5]), [50], 0, units=[5])
    assert m["n_C_nonalign"] == 0                      # 50 is a multiple of 5
    # unusable values (corrupt report, missing) are skipped like failures, not treated as values
    sig2 = {s: "a" for s in range(1, 9)}
    sig2[4], sig2[5] = "CORRUPT:x", None
    assert E.split_failure_boundaries(sig2, range(1, 9))[0] == []
    # everything failed: no decision changes, and nothing to compare
    assert E.split_failure_boundaries({s: "FAILED:x" for s in range(1, 5)}, range(1, 5))[2] == []


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
    res = E.check_independent(a, b, False)
    assert res["shared_allocations"] == ["vm1"] and res["run_identity"].startswith("unverified")
    with pytest.raises(ValueError):
        E.check_independent(a, b, require_new_allocation=True)


def _tag(recs, **kw):
    return [dict(r, **kw) for r in recs]


def test_independence_uses_g4_run_order_and_unknown_allocations():
    a = _tag(_records({}, seed=1, tag="x", blocks=2), g4_run_id="g4-d", seed=11000, vm_allocation_id="A")
    b = _tag(_records({}, seed=2, tag="y", blocks=2), g4_run_id="g4-c", seed=12000, vm_allocation_id="B",
             g4_role="confirmation")
    assert E.check_independent(a, b, True)["run_identity"] == "verified"
    # blocks of ONE G4 run split in two are not independent (per-process run_ids differ, the G4 run does not)
    with pytest.raises(ValueError, match="G4 runs"):
        E.check_independent(a, _tag(b, g4_run_id="g4-d"), False)
    with pytest.raises(ValueError, match="order"):                     # same block seed = same order
        E.check_independent(a, _tag(b, seed=11000), False)
    with pytest.raises(ValueError, match="labelled"):
        E.check_independent(a, _tag(b, g4_role="discovery"), False)
    # missing allocation ids on both sides are the same allocation 'unavailable'
    with pytest.raises(ValueError, match="allocation"):
        E.check_independent(_tag(a, vm_allocation_id=None), _tag(b, vm_allocation_id=None), True)


def test_confirmation_stage_has_its_own_allocation_minimum():
    disc = E.block_table(_records({11: 1.3}, seed=1, alloc="v1") + _records({11: 1.3}, seed=3, alloc="v2", tag="e"),
                         "mean")
    conf = E.block_table(_records({11: 1.3}, seed=2, tag="c", alloc="v3"), "mean")     # one new allocation
    kw = dict(delta=0.1, alpha=0.05, conf_level=0.95, min_allocations=2)
    assert E.answer_table_a(range(1, 21), disc, conf, **kw)["events"] == []              # old behaviour
    assert E.answer_table_a(range(1, 21), disc, conf, min_allocations_confirmation=1, **kw)["events"] == [10]


def test_alternative_indicator_ci_uses_allocations_as_units():
    recs = []
    for alloc, r11 in (("v1", 1.4), ("v2", 1.6)):
        for b in range(3):
            for fs in ("default", "alt"):
                for s in (10, 11):
                    slow = r11 if (fs == "default" and s == 11) else 1.0
                    recs.append({"padded_length": s, "block_id": f"{alloc}b{b}", "vm_allocation_id": alloc,
                                 "flagset": fs, "failure_type": None,
                                 "latency_ns": [1e6 * s * slow * (1 + 0.001 * b)]})
    res = E.alternative_indicators(E.block_table(recs, "mean", "default"), E.block_table(recs, "mean", "alt"),
                                   [10, 11], 0.95)
    r11 = next(r for r in res["R"] if r["s"] == 11)
    assert r11["unit"] == "allocation" and r11["n_units"] == 2 and r11["n_blocks"] == 6
    lo, hi = r11["ci_log_R"]
    assert lo < math.log(1.4) and hi > math.log(1.6)    # the between-allocation spread is not hidden


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
    alt = [dict(r, flagset="alt", latency_ns=[1.0]) for r in recs]
    assert E.per_shape_summary(recs + alt, "mean", "default") == ps     # other flag sets are not pooled


def test_natural_weighted_impact_is_avoidable_padding_excess():
    # an isolated slow length 11 (x1.5): features of length 11 would run faster padded to 12
    T = {10: 100.0, 11: 150.0, 12: 110.0, 13: 115.0}
    imp = E.natural_weighted_impact(T, [[10, 11], [11, 12]], {"11": 20, "12": 30, "5": 50}, [10, 11, 12, 13])
    assert imp["status"] == "ok"
    assert math.isclose(imp["total_weighted_excess"], 0.2 * (150 / 110 - 1))
    assert math.isclose(imp["weighted_excess_across_confirmed_events"], imp["total_weighted_excess"])
    assert imp["frequency_covered"] == 0.5 and imp["lengths_with_excess"][0]["best_padding"] == 12
    assert E.natural_weighted_impact(T, [], {}, [10, 11])["status"].startswith("unavailable")


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


def _dense_inputs():
    import evaluate as EV                     # the CLI module (dense_table lives there)
    comp = [{"padded_length": 1, "compile_wall_ns": 100, "feature_extract_wall_ns": 7, "verify_wall_ns": 5,
             "ir_signature": "A", "failure_type": None},
            {"padded_length": 2, "compile_wall_ns": 30, "failure_type": "compile_error"},
            {"padded_length": 1, "flagset": "O3_nosimd", "compile_wall_ns": 999, "failure_type": None}]
    meas = [{"padded_length": 1, "latency_ns": [10.0], "warmup_wall_ns": 3, "measurement_wall_ns": 4,
             "process_wall_ns": 20, "failure_type": None},
            {"padded_length": 1, "latency_ns": [], "process_wall_ns": 9, "failure_type": "runtime_error"},
            {"padded_length": 1, "flagset": "O3_nosimd", "latency_ns": [99.0], "warmup_wall_ns": 1,
             "measurement_wall_ns": 1, "failure_type": None}]
    return EV, comp, meas


def test_dense_table_keeps_failed_attempts_and_one_flagset():
    EV, comp, meas = _dense_inputs()
    dense, _ = EV.dense_table([1, 2], comp, meas, [], "mean", report_ns=40)
    assert dense[1]["compile_ns"] == 100 and dense[1]["report_ns"] == 40
    assert dense[2]["report_ns"] == 0                        # failed compile: no report overhead to move
    procs = dense[1]["processes"]
    assert len(procs) == 2 and {"failure_type": "runtime_error", "wall_ns": 9} in procs
    ok = next(p for p in procs if "median_ns" in p)
    assert ok["median_ns"] == 10.0 and ok["process_overhead_ns"] == 13
    from shapeperf.backends import ReplayBackend
    seen = {ReplayBackend(dense, seed=k).measure(1).get("failure_type") for k in range(20)}
    assert seen == {None, "runtime:runtime_error"}           # a failed attempt fails the query (and costs)
    conf = EV.confirmation_costs(meas)
    assert conf == {1: 20 + 9}                               # all default-flagset attempts, failures included


def test_corrupt_signatures_are_failure_values_for_selectors():
    from shapeperf.backends import ReplayBackend, usable_signature
    assert usable_signature("CORRUPT:abc") == "FAILED:corrupt_report" and usable_signature("x") == "x"
    dense = {1: {"compile_ns": 1, "signature": "CORRUPT:a", "processes": [
        {"median_ns": 1.0, "warmup_ns": 0, "measure_ns": 0}], "probe_ns": 1, "probe_signature": "CORRUPT:b"}}
    rb = ReplayBackend(dense, seed=0)
    assert rb.measure(1)["signature"] == rb.probe(1)["signature"] == "FAILED:corrupt_report"


def test_census_resume_recompiles_unusable_records():
    import importlib.util
    spec = importlib.util.spec_from_file_location("run_census", ROOT / "scripts/run_census.py")
    rc = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(rc)
    ok = {"ir_signature": "a", "ir_structure_signature": "x", "raw_ir_hash": "h"}
    assert not rc._incomplete(ok)
    assert not rc._incomplete({"failure_type": "timeout"})          # a real failure is a census value
    assert rc._incomplete(dict(ok, ir_signature="CORRUPT:1"))
    assert rc._incomplete(dict(ok, raw_ir_hash=None, probe_ir_missing="/x"))

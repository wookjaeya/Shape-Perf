"""Cost accounting and budget semantics of spec §8.4."""
from shapeperf.backends import SyntheticBackend
from shapeperf.broker import QueryBroker
from shapeperf.evaluate import discoveries
from shapeperf.selectors import CompileGuidedSelector, CompileProbeSelector, UniformSelector


def be():
    return SyntheticBackend({9: 2.0}, [9], noise=0.0, compile_ns=100, probe_ns=30, extract_ns=7,
                            verify_ns=5, measure_ns=10)


def test_common_vs_policy_extra_cost():
    r_u = QueryBroker(be(), range(1, 17)).run(UniformSelector(), 10**9)
    r_c = QueryBroker(be(), range(1, 17)).run(CompileGuidedSelector(), 10**9)
    q_u, q_c = r_u["timeline"][0], r_c["timeline"][0]
    assert q_u["common_ns"] == q_c["common_ns"] == 100 + 5 + 5 + 10   # compile+verify+warmup+measure
    assert q_u["policy_extra_ns"] == 0            # Uniform does not pay for extraction
    assert q_c["policy_extra_ns"] == 7            # Compile-guided pays extraction


def test_probe_is_policy_extra_only():
    r = QueryBroker(be(), range(1, 17)).run(CompileProbeSelector(budget_fraction=1.0), 10**9)
    probes = [q for q in r["timeline"] if q["action"] == "probe"]
    assert probes and all(q["common_ns"] == 0 and q["policy_extra_ns"] == 37 for q in probes)


def test_budget_crossing_query_is_recorded_but_not_counted():
    budget = 3 * 120 + 50
    r = QueryBroker(be(), range(1, 17)).run(UniformSelector(), budget)
    tl = r["timeline"]
    assert tl[-1]["within_budget"] is False and tl[-1]["cumulative_cost_ns"] > budget
    assert all(q["within_budget"] for q in tl[:-1])
    # discovery only counts completed-within-budget queries
    found = discoveries(r, [8], require_confirmation=False)
    assert 8 not in found or found[8] <= budget


def test_cost_free_auxiliary_variant():
    r = QueryBroker(be(), range(1, 17), charge_policy_extra=False).run(CompileGuidedSelector(), 10**9)
    assert all(q["charged_ns"] == q["common_ns"] + q["selection_ns"] for q in r["timeline"])


def test_confirmation_is_charged_and_hidden_from_selector():
    r = QueryBroker(be(), range(1, 17), confirm_log_threshold=0.1).run(UniformSelector(), 10**9)
    assert r["candidates"] and r["candidates"][0]["pair"] == [8, 9]
    q = next(q for q in r["timeline"] if q["confirmation_ns"])
    assert q["charged_ns"] >= q["common_ns"] + q["confirmation_ns"]
    found = discoveries(r, [8], require_confirmation=True)
    assert 8 in found


def test_failures_stay_in_history():
    b = SyntheticBackend({}, [], fail={5}, compile_ns=1, verify_ns=0, measure_ns=0, extract_ns=0)
    r = QueryBroker(b, range(1, 9)).run(UniformSelector(), 10**9)
    f = [q for q in r["timeline"] if q["failure_type"]]
    assert [q["padded_length"] for q in f] == [5]
    assert sorted(q["padded_length"] for q in r["timeline"]) == list(range(1, 9))


def test_cost_free_variant_keeps_probe_cap():
    b = SyntheticBackend({}, [], noise=0.0, compile_ns=10, probe_ns=10, extract_ns=0, verify_ns=0, measure_ns=0)
    charged = QueryBroker(b, range(1, 257)).run(CompileProbeSelector(budget_fraction=0.2), 1000)
    free = QueryBroker(b, range(1, 257), charge_policy_extra=False).run(CompileProbeSelector(budget_fraction=0.2), 1000)
    n = lambda r: sum(q["action"] == "probe" for q in r["timeline"])
    assert n(free) <= n(charged) + 2 and free["probe_spent_ns"] <= 0.2 * 1000 + 10


def test_candidates_after_budget_are_not_counted():
    from shapeperf.evaluate import recall_cost

    def big():  # costs >> real selector wall time (~1e5 ns), so the budget boundary is deterministic
        return SyntheticBackend({9: 2.0}, [9], noise=0.0, compile_ns=1e9, probe_ns=3e8, extract_ns=7e7,
                                verify_ns=5e7, measure_ns=1e8)
    full = QueryBroker(big(), range(1, 17), confirm_log_threshold=0.1).run(UniformSelector(), 10**15)
    cq = full["candidates"][0]["query_index"]
    cut = full["timeline"][cq]["cumulative_cost_ns"] - full["timeline"][cq]["confirmation_ns"] // 2
    r = QueryBroker(big(), range(1, 17), confirm_log_threshold=0.1).run(UniformSelector(), cut)
    assert r["timeline"][-1]["query_index"] == cq and not r["timeline"][-1]["within_budget"]
    row = recall_cost({cut: [r]}, [8], require_confirmation=True)[0]
    assert row["recall_mean"] == 0 and row["candidates_mean"] == 0

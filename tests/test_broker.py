"""Cost accounting and budget semantics of spec §8.4."""
from shapeperf.backends import SyntheticBackend
from shapeperf.broker import QueryBroker
from shapeperf.evaluate import discoveries, recall_cost
from shapeperf.selectors import CompileGuidedSelector, CompileProbeSelector, UniformSelector


def be(**kw):
    c = dict(compile_ns=100, probe_ns=30, extract_ns=7, verify_ns=5, measure_ns=10)
    c.update(kw)
    return SyntheticBackend({9: 2.0}, [9], noise=0.0, **c)


def big(**kw):
    """Costs >> real selector wall time (~1e5 ns): budget boundaries are deterministic."""
    c = dict(compile_ns=1e9, probe_ns=3e8, extract_ns=7e7, verify_ns=5e7, measure_ns=1e8)
    c.update(kw)
    return SyntheticBackend({9: 2.0}, [9], noise=0.0, **c)


class ReportBackend(SyntheticBackend):
    def measure(self, s):
        r = super().measure(s)
        r["report_ns"] = 40            # part of compile_ns spent emitting the opt-report
        return r


def test_common_vs_policy_extra_cost():
    r_u = QueryBroker(be(), range(1, 17)).run(UniformSelector(), 10**9)
    r_c = QueryBroker(be(), range(1, 17)).run(CompileGuidedSelector(), 10**9)
    q_u, q_c = r_u["timeline"][0], r_c["timeline"][0]
    assert q_u["common_ns"] == q_c["common_ns"] == 100 + 5 + 5 + 10   # compile+verify+warmup+measure
    assert q_u["policy_extra_ns"] == 0            # Uniform does not pay for extraction
    assert q_c["policy_extra_ns"] == 7            # Compile-guided pays extraction


def test_report_emission_charged_only_to_signature_consumers():
    mk = lambda: ReportBackend({}, [], noise=0.0, compile_ns=100, extract_ns=7, verify_ns=5, measure_ns=10)
    q_u = QueryBroker(mk(), range(1, 5)).run(UniformSelector(), 10**9)["timeline"][0]
    q_c = QueryBroker(mk(), range(1, 5)).run(CompileGuidedSelector(), 10**9)["timeline"][0]
    q_p = QueryBroker(mk(), range(1, 5)).run(CompileProbeSelector(budget_fraction=1.0), 10**9)["timeline"][0]
    assert q_u["common_ns"] == q_c["common_ns"] == q_p["common_ns"] == 60 + 5 + 5 + 10
    assert q_u["policy_extra_ns"] == 0 and q_c["policy_extra_ns"] == 7 + 40
    assert q_p["policy_extra_ns"] == 0            # Compile-probe does not use full-compile signatures


def test_probe_is_policy_extra_only():
    r = QueryBroker(be(), range(1, 17)).run(CompileProbeSelector(budget_fraction=1.0), 10**9)
    probes = [q for q in r["timeline"] if q["action"] == "probe"]
    assert probes and all(q["common_ns"] == 0 and q["policy_extra_ns"] == 37 for q in probes)


def test_budget_crossing_entry_is_recorded_but_not_counted():
    r0 = QueryBroker(big(), range(1, 17)).run(UniformSelector(), 10**15)
    budget = r0["timeline"][2]["cumulative_cost_ns"] + 1
    r = QueryBroker(big(), range(1, 17)).run(UniformSelector(), budget)
    tl = r["timeline"]
    assert len(tl) == 4 and tl[-1]["within_budget"] is False and tl[-1]["cumulative_cost_ns"] > budget
    assert all(q["within_budget"] for q in tl[:-1])


def test_cost_free_auxiliary_variant():
    r = QueryBroker(be(), range(1, 17), charge_policy_extra=False).run(CompileGuidedSelector(), 10**9)
    assert all(q["charged_ns"] == q["common_ns"] + q["selection_ns"] for q in r["timeline"])


def test_confirmation_is_its_own_entry_and_hidden_from_selector():
    r = QueryBroker(be(), range(1, 17), confirm_log_threshold=0.1).run(UniformSelector(), 10**9)
    assert r["candidates"] and r["candidates"][0]["pair"] == [8, 9]
    c = [q for q in r["timeline"] if q["action"] == "confirm"]
    assert c and c[0]["pair"] == [8, 9] and c[0]["charged_ns"] == c[0]["common_ns"] > 0
    assert 8 in discoveries(r, [8], require_confirmation=True)


def test_cost_free_variant_keeps_probe_cap():
    budget = 3e10
    charged = QueryBroker(big(), range(1, 257)).run(CompileProbeSelector(budget_fraction=0.2), budget)
    free = QueryBroker(big(), range(1, 257), charge_policy_extra=False).run(
        CompileProbeSelector(budget_fraction=0.2), budget)
    n = lambda r: sum(q["action"] == "probe" for q in r["timeline"])
    assert n(charged) >= 3                                   # the cap is actually exercised
    assert free["probe_spent_ns"] <= 0.2 * budget + 3.7e8    # at most one probe past the cap
    assert n(free) <= n(charged) + 2


def test_candidates_after_budget_are_not_counted():
    full = QueryBroker(big(), range(1, 17), confirm_log_threshold=0.1).run(UniformSelector(), 10**15)
    ci = full["candidates"][0]["query_index"]
    cut = full["timeline"][ci]["cumulative_cost_ns"] - full["timeline"][ci]["common_ns"] // 2
    r = QueryBroker(big(), range(1, 17), confirm_log_threshold=0.1).run(UniformSelector(), cut)
    assert r["timeline"][-1]["action"] == "confirm" and not r["timeline"][-1]["within_budget"]
    row = recall_cost({cut: [r]}, [8], require_confirmation=True)[0]
    assert row["recall_mean"] == 0 and row["candidates_mean"] == 0
    # the measurement that made the pair adjacent completed within budget
    assert recall_cost({cut: [r]}, [8], require_confirmation=False)[0]["recall_mean"] == 1


def test_confirmation_within_budget_survives_a_later_crossing_confirmation():
    # steps at 9 and 10: measuring 9 last makes (8,9) and (9,10) adjacent at once
    mk = lambda: SyntheticBackend({9: 2.0, 10: 2.0}, [], noise=0.0, compile_ns=1e9, extract_ns=0,
                                  verify_ns=0, measure_ns=1e8)

    class Order(UniformSelector):
        def next_action(self, view):
            from shapeperf.selectors.base import MEASURE, Action
            for s in (1, 16, 8, 10, 9):
                if s not in view.measured:
                    return Action(MEASURE, s)
            return None
    full = QueryBroker(mk(), range(1, 17), confirm_log_threshold=0.1, isolation="inprocess").run(Order(), 10**15)
    conf = [q for q in full["timeline"] if q["action"] == "confirm"]
    assert [c["pair"] for c in conf] == [[8, 9], [9, 10]]
    cut = conf[1]["cumulative_cost_ns"] - conf[1]["common_ns"] // 2
    r = QueryBroker(mk(), range(1, 17), confirm_log_threshold=0.1, isolation="inprocess").run(Order(), cut)
    found = discoveries(r, [8, 9], require_confirmation=True)
    assert 8 in found and 9 not in found


def test_failures_stay_in_history():
    b = SyntheticBackend({}, [], fail={5}, compile_ns=1, verify_ns=0, measure_ns=0, extract_ns=0)
    r = QueryBroker(b, range(1, 9)).run(UniformSelector(), 10**9)
    f = [q for q in r["timeline"] if q.get("failure_type")]
    assert [q["padded_length"] for q in f] == [5]
    assert sorted(q["padded_length"] for q in r["timeline"]) == list(range(1, 9))


def test_probe_full_mismatch_log_ignores_failures():
    b = SyntheticBackend({}, [], fail={5}, noise=0.0)
    r = QueryBroker(b, range(1, 9)).run(CompileProbeSelector(budget_fraction=1.0), 10**15)
    assert all(not str(p["probe"]).startswith("FAILED") and not str(p["full"]).startswith("FAILED")
               for p in r["probe_vs_full_signatures"])

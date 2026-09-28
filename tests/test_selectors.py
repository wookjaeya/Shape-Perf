"""Policy rules of spec §8.1/§8.2/§8.2b on synthetic backends (numbers are synthetic)."""
import pytest

from shapeperf.backends import SyntheticBackend
from shapeperf.broker import QueryBroker
from shapeperf.selectors import (CompileGuidedSelector, CompileProbeSelector, RandomSelector,
                                 ShapeOnlySelector, TimingAdaptiveSelector, TimingOnlySelector,
                                 UniformSelector)
from shapeperf.selectors.base import intervals, lower_middle, make_view
from shapeperf.selectors.policies import alignment_priority_set

BIG = 10**18


def lengths(run, kind="measure"):
    return [q["padded_length"] for q in run["timeline"] if q["action"] == kind]


def test_lower_middle_and_intervals():
    assert lower_middle([1, 2, 3, 4]) == 2
    assert lower_middle([5]) == 5
    assert intervals(range(1, 11), {1, 10}) == [(1, 10, list(range(2, 10)))]
    assert intervals(range(1, 11), {1, 2, 10}) == [(2, 10, list(range(3, 10)))]


def test_uniform_order_first_queries():
    run = QueryBroker(SyntheticBackend({}, []), range(1, 17)).run(UniformSelector(), BIG)
    seq = lengths(run)
    assert seq[:2] == [1, 16]                       # common endpoints
    assert seq[2] == 8                              # lower middle of 2..15 (14 shapes)
    assert sorted(seq) == list(range(1, 17))


def test_uniform_tie_break_rules():
    # after 1,16,8: intervals (1,8) has 6 unmeasured, (8,16) has 7 -> pick (8,16): middle of 9..15 = 12
    run = QueryBroker(SyntheticBackend({}, []), range(1, 17)).run(UniformSelector(), BIG)
    assert lengths(run)[3] == 12


def test_random_is_seeded_and_complete():
    r1 = QueryBroker(SyntheticBackend({}, []), range(1, 33)).run(RandomSelector(seed=7), BIG)
    r2 = QueryBroker(SyntheticBackend({}, []), range(1, 33)).run(RandomSelector(seed=7), BIG)
    assert lengths(r1) == lengths(r2)
    assert sorted(lengths(r1)) == list(range(1, 33))
    with pytest.raises(ValueError):
        RandomSelector()


def test_shape_only_visits_alignment_set_first():
    pri = alignment_priority_set(range(1, 33), [8])
    assert pri == {7, 8, 9, 15, 16, 17, 23, 24, 25, 31, 32}
    run = QueryBroker(SyntheticBackend({}, []), range(1, 33)).run(ShapeOnlySelector(units=[8]), BIG)
    seq = lengths(run)
    head = seq[2:2 + len(pri - {1, 32})]
    assert set(head) == pri - {1, 32}
    with pytest.raises(ValueError):
        ShapeOnlySelector()


def test_timing_only_focuses_on_jump():
    # one big step at 23 -> Timing-only should resolve 22|23 quickly
    be = SyntheticBackend({23: 2.0}, [], noise=0.0)
    run = QueryBroker(be, range(1, 65)).run(TimingOnlySelector(), BIG)
    seq = lengths(run)
    k = max(seq.index(22), seq.index(23))
    assert k < 12, seq[:k + 1]


def test_compile_guided_uses_signature_difference():
    be = SyntheticBackend({}, [37], noise=0.0)      # signature changes at 37, no timing change
    run = QueryBroker(be, range(1, 129)).run(CompileGuidedSelector(), BIG)
    seq = lengths(run)
    k = max(seq.index(36), seq.index(37))
    assert k < 12, seq[:k + 1]


def test_compile_guided_align_only_ablation_ignores_nonaligned_change():
    be = SyntheticBackend({}, [37], noise=0.0)
    full = lengths(QueryBroker(be, range(1, 129)).run(CompileGuidedSelector(), BIG))
    abl = lengths(QueryBroker(be, range(1, 129)).run(CompileGuidedSelector(align_only_units=[16]), BIG))
    uni = lengths(QueryBroker(be, range(1, 129)).run(UniformSelector(), BIG))
    assert full != uni
    # once the interval around 37 no longer contains a multiple of 16 the ablation behaves like Uniform
    assert max(abl.index(36), abl.index(37)) > max(full.index(36), full.index(37))


def test_compile_probe_bisects_then_measures_both_sides():
    be = SyntheticBackend({}, [37], noise=0.0)
    run = QueryBroker(be, range(1, 129)).run(CompileProbeSelector(budget_fraction=1.0), BIG)
    probes, meas = lengths(run, "probe"), lengths(run, "measure")
    assert meas[:2] == [1, 128]
    assert {36, 37} <= set(meas[2:4])
    # probes never repeat, never touch measured shapes
    assert len(probes) == len(set(probes)) and not set(probes) & set(meas[:2])


def test_compile_probe_respects_probe_budget_fraction():
    be = SyntheticBackend({}, [], noise=0.0, compile_ns=10, probe_ns=10, extract_ns=0,
                          verify_ns=0, measure_ns=0)
    budget = 1000
    run = QueryBroker(be, range(1, 257)).run(CompileProbeSelector(budget_fraction=0.2), budget)
    assert run["probe_spent_ns"] <= 0.2 * budget + 10


def test_compile_probe_requires_fraction():
    with pytest.raises(ValueError):
        CompileProbeSelector()


def test_timing_adaptive_prefers_deviation_from_trend():
    be = SyntheticBackend({50: 1.5}, [], noise=0.0)
    run = QueryBroker(be, range(1, 129)).run(TimingAdaptiveSelector(), BIG)
    seq = lengths(run)
    assert max(seq.index(49), seq.index(50)) < 20


def test_view_hides_timing_and_signatures_from_ineligible_policies():
    seen = {}

    class Spy(UniformSelector):
        def next_action(self, view):
            seen.setdefault("m", []).append(dict(view.measured))
            seen.setdefault("s", []).append(dict(view.signatures))
            return super().next_action(view)

    QueryBroker(SyntheticBackend({}, [5]), range(1, 9)).run(Spy(), BIG)
    assert all(v is None for m in seen["m"] for v in m.values())
    assert all(not s for s in seen["s"])


def test_make_view_is_read_only():
    v = make_view(range(1, 4), {1: 5.0}, {}, {}, set(), 0, 10)
    with pytest.raises(TypeError):
        v.measured[2] = 1.0

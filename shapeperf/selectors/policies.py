"""The compared policies (spec §8.1). All share: the valid shape set, the common
first queries (min and max, §8.2 step 1), interval construction over completed
measurements, lower-middle choice, and tie-breaking by (b) more unmeasured
shapes, (c) smaller left endpoint (§8.2). They differ only in the first
priority key, which is the information the policy is entitled to see.

Conventions fixed here (to be copied into preregistration.md):
  * failed shapes count as queried; for signature comparisons a failure is its
    own signature value "FAILED:<type>"; for timing comparisons an interval
    with a failed endpoint gets first-priority score 0.
  * Random uses numpy's default_rng(seed) permutation of the valid set.
"""
import math

import numpy as np

from .base import (MEASURE, PROBE, Action, Selector, done_set, endpoint_action,
                   intervals, lower_middle)


def _tie_key(interval):
    a, b, inside = interval
    return (-len(inside), a)          # (b) more unmeasured first, (c) smaller left first


def _pick(view, known, score_fn=None):
    ivs = intervals(view.valid_lengths, known)
    if not ivs:
        return None
    if score_fn is None:
        best = min(ivs, key=_tie_key)
    else:
        best = min(ivs, key=lambda iv: (-score_fn(iv),) + _tie_key(iv))
    return lower_middle(best[2])


def _sig(view, s):
    if s in view.failed:
        return f"FAILED:{view.failed[s]}"
    return view.signatures.get(s)


class UniformSelector(Selector):
    """Middle of the widest unmeasured interval."""
    name = "uniform"

    def next_action(self, view):
        a = endpoint_action(view)
        if a:
            return a
        s = _pick(view, done_set(view))
        return Action(MEASURE, s) if s is not None else None


class RandomSelector(Selector):
    """Sampling without replacement from unmeasured shapes, fixed seed."""
    name = "random"

    def __init__(self, seed=None, **params):
        if seed is None:
            raise ValueError("RandomSelector needs a preregistered seed")
        super().__init__(seed, **params)
        self._order = None

    def next_action(self, view):
        a = endpoint_action(view)
        if a:
            return a
        if self._order is None:
            rng = np.random.default_rng(self.seed)
            self._order = [int(x) for x in rng.permutation(sorted(view.valid_lengths))]
        known = done_set(view)
        for s in self._order:
            if s not in known:
                return Action(MEASURE, s)
        return None


def alignment_priority_set(valid, units):
    """Multiples of each unit and their immediate neighbours inside the valid
    set (spec §8.1 Shape-only). Units must come from preregistered target facts."""
    vs = set(valid)
    pri = set()
    for u in units:
        if u <= 1:
            continue
        for m in range(u, max(vs) + 2, u):
            for s in (m - 1, m, m + 1):
                if s in vs:
                    pri.add(s)
    return pri


class ShapeOnlySelector(Selector):
    """Alignment boundaries first (uniform coverage order inside the priority
    set), then Uniform. Units come from params['units'] and are fixed before
    any performance is observed; units=[] means 'assumption invalid' and the
    policy degenerates to Uniform (spec §8.1)."""
    name = "shape_only"

    def __init__(self, seed=None, units=None, **params):
        if units is None:
            raise ValueError("ShapeOnlySelector needs preregistered alignment units")
        super().__init__(seed, units=list(units), **params)
        self.units = list(units)

    def next_action(self, view):
        a = endpoint_action(view)
        if a:
            return a
        known = done_set(view)
        pri = alignment_priority_set(view.valid_lengths, self.units) - known
        if pri:
            ivs = [(a_, b_, [x for x in inside if x in pri])
                   for a_, b_, inside in intervals(view.valid_lengths, known)]
            ivs = [iv for iv in ivs if iv[2]]
            best = min(ivs, key=_tie_key)
            return Action(MEASURE, lower_middle(best[2]))
        s = _pick(view, known)
        return Action(MEASURE, s) if s is not None else None


class TimingOnlySelector(Selector):
    """First priority abs(log(T_right / T_left)) (spec §8.2, last paragraph)."""
    name = "timing_only"
    sees_timing = True

    def next_action(self, view):
        a = endpoint_action(view)
        if a:
            return a

        def score(iv):
            tl, tr = view.measured.get(iv[0]), view.measured.get(iv[1])
            if not tl or not tr:
                return 0.0
            return abs(math.log(tr / tl))
        s = _pick(view, done_set(view), score)
        return Action(MEASURE, s) if s is not None else None


class CompileGuidedSelector(Selector):
    """First priority: the two endpoints' structural signatures differ (§8.2).

    params['align_only_units'] (ablation §11.2-6): if given, a signature
    difference only counts when the interval contains a boundary adjacent to a
    multiple of one of these units, i.e. non-aligned change points are ignored."""
    name = "compile_guided"
    sees_signatures = True

    def __init__(self, seed=None, align_only_units=None, **params):
        super().__init__(seed, align_only_units=align_only_units, **params)
        self.align_only_units = align_only_units

    def _counts(self, view, iv):
        a, b, _ = iv
        if _sig(view, a) == _sig(view, b):
            return 0.0
        if self.align_only_units:
            pri = alignment_priority_set(range(a, b + 1), self.align_only_units)
            return 1.0 if pri else 0.0
        return 1.0

    def next_action(self, view):
        a = endpoint_action(view)
        if a:
            return a
        s = _pick(view, done_set(view), lambda iv: self._counts(view, iv))
        return Action(MEASURE, s) if s is not None else None


class CompileProbeSelector(Selector):
    """Two-stage policy (spec §8.2b).

    1. min/max are probed and measured (common first queries; a full compile
       also yields the signature, so no separate probe is issued for them).
    2. Probe intervals are adjacent *probed-or-measured* shapes with unprobed
       shapes inside; priority: signatures differ, then §8.2 b/c ties.
    3. When an adjacent pair has different signatures and nothing unprobed in
       between, it is a confirmed change point; both sides go to the
       measurement queue. The queue has priority over new probes.
    4. Probing stops once probe spending reaches budget_fraction * budget
       (preregistered from the G3 probe:measure cost ratio).
    params['hybrid_uniform'] (ablation only, §8.2b): fill leftover budget with
    Uniform measurements instead of stopping.
    """
    name = "compile_probe"
    sees_signatures = True
    uses_probe = True

    def __init__(self, seed=None, budget_fraction=None, hybrid_uniform=False, **params):
        if budget_fraction is None:
            raise ValueError("CompileProbeSelector needs a preregistered budget_fraction")
        super().__init__(seed, budget_fraction=budget_fraction, hybrid_uniform=hybrid_uniform, **params)
        self.budget_fraction = float(budget_fraction)
        self.hybrid_uniform = hybrid_uniform

    def next_action(self, view):
        a = endpoint_action(view)
        if a:
            return a
        measured = done_set(view)
        sig_known = set(view.probed) | measured
        # confirmed change points -> measurement queue
        ks = sorted(sig_known)
        vs = set(view.valid_lengths)
        queue = []
        for x, y in zip(ks, ks[1:]):
            gap_unknown = any(v in vs and v not in sig_known for v in range(x + 1, y))
            if not gap_unknown and _sig(view, x) != _sig(view, y):
                for s in (x, y):
                    if s not in measured and s not in queue:
                        queue.append(s)
        if queue:
            return Action(MEASURE, queue[0])
        if view.probe_spent_ns < self.budget_fraction * view.budget_ns:
            s = _pick(view, sig_known, lambda iv: float(_sig(view, iv[0]) != _sig(view, iv[1])))
            if s is not None:
                return Action(PROBE, s)
        if self.hybrid_uniform:
            s = _pick(view, measured)
            return Action(MEASURE, s) if s is not None else None
        return None


class TimingAdaptiveSelector(Selector):
    """[optional baseline, spec §8.1] Change-point acquisition from timing only.

    A low-order polynomial in s (degree params['degree'], default 2 - BERT's
    cost has linear and quadratic terms in the sequence length) is fitted to
    log latency of the measured shapes; an interval's score is how much its
    observed log ratio deviates from the fitted trend's log ratio, i.e. a
    local slope change. Ties as in §8.2."""
    name = "timing_adaptive"
    sees_timing = True

    def __init__(self, seed=None, degree=2, **params):
        super().__init__(seed, degree=degree, **params)
        self.degree = degree

    def next_action(self, view):
        a = endpoint_action(view)
        if a:
            return a
        pts = sorted((s, t) for s, t in view.measured.items() if t)
        coef = None
        if len(pts) > self.degree + 1:
            xs = np.array([p[0] for p in pts], float)
            ys = np.log(np.array([p[1] for p in pts], float))
            coef = np.polyfit(xs, ys, self.degree)

        def score(iv):
            tl, tr = view.measured.get(iv[0]), view.measured.get(iv[1])
            if not tl or not tr:
                return 0.0
            obs = math.log(tr / tl)
            if coef is None:
                return abs(obs)
            trend = np.polyval(coef, iv[1]) - np.polyval(coef, iv[0])
            return abs(obs - trend)
        s = _pick(view, done_set(view), score)
        return Action(MEASURE, s) if s is not None else None

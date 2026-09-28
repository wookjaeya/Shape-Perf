"""Selector interface and the read-only view a selector is allowed to see."""
import bisect
from dataclasses import dataclass, field
from types import MappingProxyType
from typing import Dict, Optional, Tuple

MEASURE = "measure"
PROBE = "probe"


@dataclass(frozen=True)
class Action:
    kind: str          # MEASURE or PROBE
    length: int


@dataclass(frozen=True)
class View:
    """What a selector may know. Built by the broker from *this selector's own*
    completed queries; nothing about unqueried shapes is present (spec §8.3).

    valid_lengths    : the finite valid shape set (public, part of the problem)
    measured         : length -> latency estimate (ns) or None if the policy is
                       not entitled to timing (Uniform/Random/Shape-only/Compile-*)
    signatures       : length -> signature from the FULL compile of a measured
                       shape (only for policies entitled to compile information)
    probe_signatures : length -> signature from a PROBE compile (probe stage);
                       kept separate so the two stages are never compared (§8.2b)
    failed           : length -> failure type of a measurement query
    probed           : lengths that were probed (compile-only)
    spent_ns         : cumulative cost charged so far
    budget_ns        : total budget of this run (used by Compile-probe's split)
    probe_spent_ns   : actual probe cost so far (tracked even when not charged)
    """
    valid_lengths: Tuple[int, ...]
    measured: MappingProxyType
    signatures: MappingProxyType
    failed: MappingProxyType
    probed: frozenset
    spent_ns: int
    budget_ns: int
    probe_spent_ns: int = 0
    probe_signatures: MappingProxyType = field(default_factory=lambda: MappingProxyType({}))


def make_view(valid, measured, signatures, failed, probed, spent, budget, probe_spent=0,
              probe_signatures=None):
    return View(tuple(valid), MappingProxyType(dict(measured)), MappingProxyType(dict(signatures)),
                MappingProxyType(dict(failed)), frozenset(probed), int(spent), int(budget),
                int(probe_spent), MappingProxyType(dict(probe_signatures or {})))


def view_to_json(v):
    """Plain-JSON form of a View for the out-of-process selector runner."""
    return {"valid_lengths": list(v.valid_lengths),
            "measured": [[k, x] for k, x in v.measured.items()],
            "signatures": [[k, x] for k, x in v.signatures.items()],
            "probe_signatures": [[k, x] for k, x in v.probe_signatures.items()],
            "failed": [[k, x] for k, x in v.failed.items()],
            "probed": sorted(v.probed), "spent_ns": v.spent_ns, "budget_ns": v.budget_ns,
            "probe_spent_ns": v.probe_spent_ns}


def view_from_json(d):
    return make_view(d["valid_lengths"], dict(map(tuple, d["measured"])), dict(map(tuple, d["signatures"])),
                     dict(map(tuple, d["failed"])), d["probed"], d["spent_ns"], d["budget_ns"],
                     d["probe_spent_ns"], dict(map(tuple, d["probe_signatures"])))


class Selector:
    """Subclasses implement next_action(view) -> Action | None (None = stop)."""
    name = "base"
    sees_timing = False
    sees_signatures = False
    uses_probe = False

    def __init__(self, seed: Optional[int] = None, **params):
        self.seed = seed
        self.params = params

    def next_action(self, view: View) -> Optional[Action]:  # pragma: no cover
        raise NotImplementedError

    def describe(self) -> Dict:
        return {"name": self.name, "seed": self.seed, "params": dict(self.params),
                "sees_timing": self.sees_timing, "sees_signatures": self.sees_signatures,
                "uses_probe": self.uses_probe}


# ---------------------------------------------------------------------------
# Shared interval rules (spec §8.2 steps 2-5; identical for all policies)

def done_set(view):
    """Shapes whose measurement query has completed (success or failure)."""
    return set(view.measured) | set(view.failed)


def intervals(valid, known):
    """Adjacent pairs of known shapes (sorted) with >=1 unknown valid shape
    strictly between them. Returns list of (left, right, [unknown inside])."""
    ks = sorted(known)
    out = []
    vs = sorted(valid)
    for a, b in zip(ks, ks[1:]):
        i, j = bisect.bisect_right(vs, a), bisect.bisect_left(vs, b)
        inside = [v for v in vs[i:j] if v not in known]
        if inside:
            out.append((a, b, inside))
    return out


def lower_middle(xs):
    """Middle element; lower one for an even count (spec §8.2 step 5)."""
    return xs[(len(xs) - 1) // 2]


def endpoint_action(view):
    """Step 1: the min and max valid shapes are queried first by every policy."""
    known = done_set(view)
    lo, hi = min(view.valid_lengths), max(view.valid_lengths)
    for s in (lo, hi):
        if s not in known:
            return Action(MEASURE, s)
    return None

"""Shape selection policies (spec §8.1, §8.2, §8.2b).

Isolation contract (spec §8.3, §11 G2.5): modules in this package are pure
decision logic. They receive a read-only `View` of their own query history
from the broker and return the next action. They must not import the
evaluator, census, file or process APIs; tests/test_isolation.py checks this
statically, and the broker runs every decision under a runtime guard that
rejects file access and forbidden imports.
"""
from .base import Action, View, Selector  # noqa: F401
from .policies import (  # noqa: F401
    UniformSelector, RandomSelector, ShapeOnlySelector, TimingOnlySelector,
    CompileGuidedSelector, CompileProbeSelector, TimingAdaptiveSelector,
)

REGISTRY = {
    "uniform": UniformSelector,
    "random": RandomSelector,
    "shape_only": ShapeOnlySelector,
    "timing_only": TimingOnlySelector,
    "compile_guided": CompileGuidedSelector,
    "compile_probe": CompileProbeSelector,
    "timing_adaptive": TimingAdaptiveSelector,
}

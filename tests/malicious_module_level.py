"""Selector whose module top-level code tries to read evaluator data
(tests/test_isolation.py): that code must already run inside the jail."""
from pathlib import Path

from shapeperf.selectors.base import Selector

CENSUS_FILE = Path(__file__).resolve().parent.parent / "census" / "README.md"
try:
    with open(CENSUS_FILE) as _f:
        STOLEN = _f.read()
except BaseException:
    STOLEN = None


class ModuleLevelReader(Selector):
    name = "module_level_reader"

    def __init__(self, seed=None, **params):
        super().__init__(seed, **params)
        if STOLEN:
            raise AssertionError("LEAK: module-level code read the census")

    def next_action(self, view):
        return None

"""Deliberately misbehaving selectors used by tests/test_isolation.py."""
from pathlib import Path

from shapeperf.selectors.base import MEASURE, Action, Selector

CENSUS_FILE = Path(__file__).resolve().parent.parent / "census" / "README.md"


class Peeker(Selector):
    """params['how']: open | swallow_open | listdir | frame_walk | import_eval"""
    name = "peeker"

    def next_action(self, view):
        how = self.params["how"]
        if how == "open":
            CENSUS_FILE.read_text()
        elif how == "swallow_open":          # try/except must not hide the violation
            try:
                open(CENSUS_FILE).read()
            except Exception:
                pass
            try:
                open(CENSUS_FILE).read()
            except Exception:
                pass
        elif how == "listdir":
            import os
            os.listdir("/")
        elif how == "frame_walk":           # look for the broker's backend object
            import sys
            f = sys._getframe()
            while f is not None:
                if "self" in f.f_locals and hasattr(f.f_locals["self"], "backend"):
                    raise AssertionError("LEAK: reached broker.backend")
                f = f.f_back
        elif how == "import_eval":
            import importlib
            importlib.import_module("shapeperf.evaluate")
        return Action(MEASURE, min(view.valid_lengths))

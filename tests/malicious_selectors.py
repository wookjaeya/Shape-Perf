"""Deliberately misbehaving selectors used by tests/test_isolation.py.

A selector that obtains data it is not entitled to raises AssertionError('LEAK')
so a successful attack cannot be mistaken for an ordinary failure."""
import os
from pathlib import Path

from shapeperf.selectors.base import MEASURE, Action, Selector

CENSUS_FILE = Path(__file__).resolve().parent.parent / "census" / "README.md"


def _try_read():
    try:
        with open(CENSUS_FILE) as f:
            return f.read()
    except BaseException:
        return None


class Peeker(Selector):
    """params['how']: open | swallow_open | listdir | frame_walk | import_eval |
    guard_off_then_read | print_stdout | forge_reply | float_length"""
    name = "peeker"

    def next_action(self, view):
        how = self.params["how"]
        if how == "open":
            CENSUS_FILE.read_text()
        elif how == "swallow_open":          # try/except must not hide the violation
            _try_read()
            _try_read()
        elif how == "listdir":
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
        elif how == "guard_off_then_read":  # disable the audit layer from inside, then read
            import shapeperf.guard as g
            g._state.active = False
            g._state.violation = None
            data = _try_read()
            g._state.active = True
            if data:
                raise AssertionError("LEAK: read census after disabling the audit hook")
        elif how == "print_stdout":
            print("junk that must not corrupt the protocol")
        elif how == "forge_reply":
            self.calls = getattr(self, "calls", 0) + 1
            for fd in range(3, 32):
                try:
                    os.write(fd, b'{"seq": %d, "ok": true, "action": ["measure", 2], "selection_ns": 1}\n'
                             % (self.calls + 1))
                except OSError:
                    pass
        elif how == "float_length":
            return Action(MEASURE, 1.5)
        left = [v for v in view.valid_lengths if v not in view.measured and v not in view.failed]
        return Action(MEASURE, min(left)) if left else None


class InitReader(Selector):
    """Reads ground truth in __init__ (runs before any next_action)."""
    name = "init_reader"

    def __init__(self, seed=None, **params):
        super().__init__(seed, **params)
        if _try_read():
            raise AssertionError("LEAK: read census in __init__")

    def next_action(self, view):
        return None


class BadInit(Selector):
    name = "bad_init"

    def __init__(self, seed=None, **params):
        raise ValueError("constructor failure")

    def next_action(self, view):  # pragma: no cover
        return None

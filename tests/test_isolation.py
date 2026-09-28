"""Selectors cannot reach the census / evaluator (spec §11 G2.5, §13 item 14).

1. Static: every module in shapeperf/selectors imports only an allow-list and
   never names file/process APIs or the census directory.
2. Runtime: the broker's sandbox rejects file access and forbidden imports made
   from inside a selector decision, and the real policies run fine under it.
"""
import ast
from pathlib import Path

import pytest

from shapeperf.backends import SyntheticBackend
from shapeperf.broker import QueryBroker
from shapeperf.guard import SelectorIsolationError, selector_sandbox
from shapeperf.selectors import REGISTRY
from shapeperf.selectors.base import MEASURE, Action, Selector

SEL_DIR = Path(__file__).resolve().parent.parent / "shapeperf" / "selectors"
ALLOWED_IMPORTS = {"math", "bisect", "dataclasses", "types", "typing", "numpy", "collections"}
FORBIDDEN_NAMES = {"open", "exec", "eval", "compile", "__import__", "globals", "getattr", "input"}
FORBIDDEN_ATTRS = {"read_text", "read_bytes", "load", "loads", "system", "popen", "listdir",
                   "scandir", "walk"}


def _modules():
    return sorted(SEL_DIR.glob("*.py"))


@pytest.mark.parametrize("path", _modules(), ids=lambda p: p.name)
def test_static_imports_and_names(path):
    tree = ast.parse(path.read_text())
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for a in node.names:
                assert a.name.split(".")[0] in ALLOWED_IMPORTS, f"{path.name}: import {a.name}"
        elif isinstance(node, ast.ImportFrom):
            if node.level == 0:
                assert node.module.split(".")[0] in ALLOWED_IMPORTS, f"{path.name}: from {node.module}"
            else:
                assert node.level == 1, f"{path.name}: relative import escapes the package"
        elif isinstance(node, ast.Name):
            assert node.id not in FORBIDDEN_NAMES, f"{path.name}: uses {node.id}"
        elif isinstance(node, ast.Attribute):
            assert node.attr not in FORBIDDEN_ATTRS, f"{path.name}: uses .{node.attr}"
        elif isinstance(node, ast.Constant) and isinstance(node.value, str):
            assert "census" not in node.value.lower() or node is tree.body[0].value, \
                f"{path.name}: mentions census in code"


class _Peeker(Selector):
    name = "peeker"

    def __init__(self, how):
        super().__init__(None)
        self.how = how

    def next_action(self, view):
        if self.how == "open":
            open(Path(__file__).resolve().parent.parent / "census" / "README.md").read()
        elif self.how == "import":
            import importlib
            importlib.import_module("shapeperf.evaluate_does_not_exist")
        elif self.how == "listdir":
            import os
            os.listdir("/")
        return Action(MEASURE, min(view.valid_lengths))


@pytest.mark.parametrize("how", ["open", "listdir"])
def test_runtime_guard_blocks_io(how):
    b = QueryBroker(SyntheticBackend({}, []), range(1, 17))
    with pytest.raises(SelectorIsolationError):
        b.run(_Peeker(how), budget_ns=10**15)


def test_runtime_guard_blocks_evaluator_import():
    with pytest.raises((SelectorIsolationError, ModuleNotFoundError)):
        with selector_sandbox():
            __import__("shapeperf.evaluate_does_not_exist")
    # a real forbidden module that is not imported yet in a fresh interpreter
    import subprocess
    import sys
    code = ("import sys; sys.path.insert(0, %r)\n"
            "from shapeperf.guard import selector_sandbox, SelectorIsolationError\n"
            "try:\n"
            "    with selector_sandbox():\n"
            "        import shapeperf.evaluate\n"
            "    print('LEAK')\n"
            "except SelectorIsolationError:\n"
            "    print('BLOCKED')\n") % str(SEL_DIR.parent.parent)
    out = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True).stdout
    assert out.strip() == "BLOCKED"


def test_guard_is_inactive_outside_sandbox(tmp_path):
    with selector_sandbox():
        pass
    (tmp_path / "x").write_text("ok")          # evaluator code keeps working
    assert (tmp_path / "x").read_text() == "ok"


@pytest.mark.parametrize("name", sorted(REGISTRY))
def test_real_policies_run_under_sandbox(name):
    params = {"random": {"seed": 1}, "shape_only": {"units": [4]},
              "compile_probe": {"budget_fraction": 0.3}}.get(name, {})
    sel = REGISTRY[name](**params)
    b = QueryBroker(SyntheticBackend({40: 1.2}, [33, 40]), range(1, 65))
    run = b.run(sel, budget_ns=int(3e12))
    assert run["timeline"]

"""Selectors cannot reach the census / evaluator (spec §11 G2.5, §13 item 14).

1. Static: every module in shapeperf/selectors imports only an allow-list and
   never names file/process APIs or the census directory.
2. Process boundary (default): the selector runs in its own process and only
   receives its View; file access, frame walking and evaluator imports fail.
3. Sticky sandbox: catching the isolation error does not hide a violation.
"""
import ast
from pathlib import Path

import pytest

from shapeperf.backends import SyntheticBackend
from shapeperf.broker import QueryBroker
from shapeperf.guard import SelectorIsolationError, selector_sandbox
from shapeperf.selectors import REGISTRY
from tests.malicious_selectors import CENSUS_FILE, Peeker

SEL_DIR = Path(__file__).resolve().parent.parent / "shapeperf" / "selectors"
ALLOWED_IMPORTS = {"math", "bisect", "dataclasses", "types", "typing", "numpy", "collections"}
FORBIDDEN_NAMES = {"open", "exec", "eval", "compile", "__import__", "globals", "getattr", "input"}
FORBIDDEN_ATTRS = {"read_text", "read_bytes", "load", "loads", "system", "popen", "listdir",
                   "scandir", "walk", "_getframe", "f_back", "f_locals", "tb_frame"}


def _modules():
    return sorted(SEL_DIR.glob("*.py"))


@pytest.mark.parametrize("path", _modules(), ids=lambda p: p.name)
def test_static_imports_and_names(path):
    tree = ast.parse(path.read_text())
    doc = tree.body[0].value if tree.body and isinstance(tree.body[0], ast.Expr) else None
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
        elif isinstance(node, ast.Constant) and isinstance(node.value, str) and node is not doc:
            assert "census" not in node.value.lower(), f"{path.name}: mentions census in code"


@pytest.mark.parametrize("isolation", ["process", "inprocess"])
@pytest.mark.parametrize("how", ["open", "swallow_open", "listdir", "frame_walk"])
def test_malicious_selector_is_stopped(how, isolation):
    assert CENSUS_FILE.exists()
    b = QueryBroker(SyntheticBackend({}, []), range(1, 17), isolation=isolation)
    with pytest.raises(SelectorIsolationError):
        b.run(Peeker(how=how), budget_ns=10**15)


def test_evaluator_import_blocked_in_process_mode():
    b = QueryBroker(SyntheticBackend({}, []), range(1, 17), isolation="process")
    with pytest.raises(SelectorIsolationError):
        b.run(Peeker(how="import_eval"), budget_ns=10**15)


def test_sticky_violation_survives_except():
    with pytest.raises(SelectorIsolationError):
        with selector_sandbox():
            try:
                open(CENSUS_FILE)
            except SelectorIsolationError:
                pass


def test_guard_is_inactive_outside_sandbox(tmp_path):
    with selector_sandbox():
        pass
    (tmp_path / "x").write_text("ok")          # evaluator code keeps working
    assert (tmp_path / "x").read_text() == "ok"


@pytest.mark.parametrize("name", sorted(REGISTRY))
def test_real_policies_run_in_worker_process(name):
    params = {"random": {"seed": 1}, "shape_only": {"units": [4]},
              "compile_probe": {"budget_fraction": 0.3}}.get(name, {})
    sel = REGISTRY[name](**params)
    run = QueryBroker(SyntheticBackend({40: 1.2}, [33, 40]), range(1, 65)).run(sel, budget_ns=int(3e12))
    assert run["timeline"] and run["isolation"] == "process"
    assert all(q["selection_ns"] > 0 for q in run["timeline"])

"""Selectors cannot reach the census / evaluator (spec §11 G2.5, §13 item 14).

1. Static: every module in shapeperf/selectors imports only an allow-list and
   never names file/process/frame APIs or the census directory.
2. OS boundary (default 'process' mode): the selector runs in a jailed worker
   (chroot to an empty directory, setuid nobody, no network). Even a selector
   that switches the audit layer off cannot read files.
3. Audit layer: clear errors for accidental violations; sticky across except.
4. Protocol: stdout noise is harmless, forged replies are detected.
'inprocess' mode is a debugging aid and is NOT a security boundary (tested).
"""
import ast
import os
from pathlib import Path

import numpy as np
import pytest

from shapeperf.backends import SyntheticBackend
from shapeperf.broker import QueryBroker, SelectorSpec
from shapeperf.guard import SelectorIsolationError, selector_sandbox
from shapeperf.selectors import REGISTRY, UniformSelector
from tests.malicious_selectors import CENSUS_FILE, BadInit, InitReader, Peeker

SEL_DIR = Path(__file__).resolve().parent.parent / "shapeperf" / "selectors"
ALLOWED_IMPORTS = {"math", "bisect", "dataclasses", "types", "typing", "numpy", "collections"}
FORBIDDEN_NAMES = {"open", "exec", "eval", "compile", "__import__", "globals", "getattr", "input"}
FORBIDDEN_ATTRS = {"read_text", "read_bytes", "load", "loads", "system", "popen", "listdir",
                   "scandir", "walk", "_getframe", "f_back", "f_locals", "tb_frame"}
ROOT = os.getuid() == 0


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


def _broker(isolation="process", n=16):
    return QueryBroker(SyntheticBackend({}, []), range(1, n + 1), isolation=isolation)


@pytest.mark.parametrize("isolation", ["process", "inprocess"])
@pytest.mark.parametrize("how", ["open", "swallow_open", "listdir", "frame_walk"])
def test_accidental_violations_are_reported(how, isolation):
    assert CENSUS_FILE.exists()
    with pytest.raises(SelectorIsolationError):
        _broker(isolation).run(Peeker(how=how), budget_ns=10**15)


def test_evaluator_import_blocked_in_process_mode():
    with pytest.raises(SelectorIsolationError):
        _broker().run(Peeker(how="import_eval"), budget_ns=10**15)


@pytest.mark.skipif(not ROOT, reason="OS jail needs root (chroot/setuid)")
def test_os_jail_holds_even_with_audit_layer_disabled():
    run = _broker().run(Peeker(how="guard_off_then_read"), budget_ns=10**15)   # no AssertionError('LEAK')
    assert run["isolation_level"].startswith("os:") and "chroot" in run["isolation_level"]
    assert run["timeline"]


def test_inprocess_mode_is_not_a_security_boundary():
    with pytest.raises(AssertionError, match="LEAK"):
        _broker("inprocess").run(Peeker(how="guard_off_then_read"), budget_ns=10**15)


def test_constructor_runs_jailed_and_sandboxed():
    with pytest.raises(SelectorIsolationError):
        _broker().run(SelectorSpec(InitReader), budget_ns=10**15)


def test_stdout_noise_does_not_break_protocol():
    run = _broker().run(Peeker(how="print_stdout"), budget_ns=10**15)
    assert len(run["timeline"]) == 16


def test_forged_reply_is_detected():
    with pytest.raises(SelectorIsolationError, match="desync"):
        _broker().run(Peeker(how="forge_reply"), budget_ns=10**15)


def test_non_integer_length_rejected():
    with pytest.raises(RuntimeError):
        _broker().run(Peeker(how="float_length"), budget_ns=10**15)


def _children():
    me = str(os.getpid())
    out = []
    for p in Path("/proc").iterdir():
        if p.name.isdigit():
            try:
                if (p / "stat").read_text().split()[3] == me:
                    out.append(p.name)
            except OSError:
                pass
    return set(out)


def test_no_worker_leak_on_init_failure():
    before = _children()
    with pytest.raises(RuntimeError):
        _broker().run(SelectorSpec(BadInit), budget_ns=10**15)
    assert _children() <= before


def test_numpy_typed_inputs_work_in_process_mode():
    b = QueryBroker(SyntheticBackend({}, []), np.arange(1, 9, dtype=np.int64))
    run = b.run(REGISTRY["random"](seed=np.int64(3)), budget_ns=10**15)
    assert sorted(q["padded_length"] for q in run["timeline"]) == list(range(1, 9))


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
def test_real_policies_run_in_jailed_worker(name):
    params = {"random": {"seed": 1}, "shape_only": {"units": [4]},
              "compile_probe": {"budget_fraction": 0.3}}.get(name, {})
    sel = REGISTRY[name](**params)
    run = QueryBroker(SyntheticBackend({40: 1.2}, [33, 40]), range(1, 65)).run(sel, budget_ns=int(3e12))
    assert run["timeline"] and run["isolation"] == "process"
    if ROOT:
        assert run["isolation_level"].startswith("os:")
    assert all(q["selection_ns"] > 0 for q in run["timeline"] if q["action"] != "confirm")


def test_process_and_inprocess_decisions_identical():
    be = lambda: SyntheticBackend({23: 1.5}, [23], noise=0.0)
    for name, kw in [("timing_adaptive", {}), ("compile_probe", {"budget_fraction": 0.5}), ("uniform", {})]:
        a = QueryBroker(be(), range(1, 65)).run(REGISTRY[name](**kw), 10**15)
        b = QueryBroker(be(), range(1, 65), isolation="inprocess").run(REGISTRY[name](**kw), 10**15)
        assert [(q["action"], q["padded_length"]) for q in a["timeline"]] == \
               [(q["action"], q["padded_length"]) for q in b["timeline"]]
    assert UniformSelector.sees_timing is False


@pytest.mark.skipif(not ROOT, reason="OS jail needs root (chroot/setuid)")
def test_class_path_module_code_runs_inside_the_jail():
    from tests.malicious_module_level import ModuleLevelReader
    run = _broker().run(SelectorSpec(ModuleLevelReader), budget_ns=10**15)   # no AssertionError('LEAK')
    assert run["isolation_level"].startswith("os:") and run["isolation_level"].endswith("+test-class")


def test_class_path_selectors_need_explicit_test_opt_in(monkeypatch):
    monkeypatch.delenv("SHAPEPERF_ALLOW_CLASS_PATH", raising=False)
    with pytest.raises(RuntimeError, match="test-only"):
        _broker().run(SelectorSpec(InitReader), budget_ns=10**15)

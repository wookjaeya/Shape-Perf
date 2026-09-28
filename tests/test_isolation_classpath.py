"""Test-only class_path selectors (selectors outside the registry): their code,
including their packages' __init__ and what they import, must not run outside
the jail, and ordinary Python in them must still work (spec §8.3)."""
import importlib
import os
import sys
import textwrap

import pytest

from shapeperf.backends import SyntheticBackend
from shapeperf.broker import QueryBroker, SelectorSpec

ROOT = os.getuid() == 0


def _module(tmp_path, monkeypatch, files):
    for rel, text in files.items():
        p = tmp_path / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(textwrap.dedent(text))
    monkeypatch.syspath_prepend(str(tmp_path))
    monkeypatch.setenv("PYTHONPATH", str(tmp_path) + os.pathsep + os.environ.get("PYTHONPATH", ""))
    importlib.invalidate_caches()


def _run(cls):
    return QueryBroker(SyntheticBackend({}, []), range(1, 9)).run(SelectorSpec(cls), budget_ns=10**15)


SELECTOR = """
    from shapeperf.selectors.base import Selector

    class Sel(Selector):
        name = "cp_sel"

        def next_action(self, view):
            return None
"""


def test_parent_package_init_never_runs_in_the_worker(tmp_path, monkeypatch):
    marker = tmp_path / "ran.txt"
    _module(tmp_path, monkeypatch, {
        "cppkg/__init__.py": f"import os\nopen({str(marker)!r}, 'a').write(str(os.getpid()) + '\\n')\n",
        "cppkg/sel.py": SELECTOR})
    mod = importlib.import_module("cppkg.sel")          # runs __init__ here, in the test process
    try:
        with pytest.raises(RuntimeError, match="regular package"):
            _run(mod.Sel)
        assert set(marker.read_text().split()) == {str(os.getpid())}   # never in the worker
    finally:
        sys.modules.pop("cppkg.sel", None)
        sys.modules.pop("cppkg", None)


def test_dead_code_imports_are_not_preimported(tmp_path, monkeypatch):
    _module(tmp_path, monkeypatch, {"cp_dead.py": "if False:\n    import venv.__main__\n" + textwrap.dedent(SELECTOR)})
    import cp_dead
    before = set(os.listdir(tmp_path))
    run = _run(cp_dead.Sel)                             # the worker did not die running venv
    assert run["isolation_level"].endswith("+test-class") and set(os.listdir(tmp_path)) == before


@pytest.mark.skipif(not ROOT, reason="needs the OS jail")
def test_ordinary_python_works_in_a_class_path_module(tmp_path, monkeypatch):
    _module(tmp_path, monkeypatch, {"cp_dc.py": """
        from __future__ import annotations
        from dataclasses import dataclass

        from shapeperf.selectors.base import Selector


        @dataclass
        class Step:
            step: int = 4


        class DcSel(Selector):
            name = "cp_dc"

            def next_action(self, view):
                Step()
                return None
    """})
    import cp_dc
    run = _run(cp_dc.DcSel)
    assert run["isolation_level"].endswith("+test-class")

"""Loader regression tests (follow-up E1): errors that make an S8-vs-S1 comparison invalid.

1. The mechanism, with two tiny C libraries that mimic ONNX-MLIR's exported symbols: loaded with
   RTLD_GLOBAL, the later library's entry runs the EARLIER library's compute function when both use
   the same tag, and its own when the tags differ.
2. The guard refuses to co-load libraries that share model-specific symbols, before any loading.
3. The pair worker passes each arm's tag to the runtime.
4. With the real L=64 artifacts and gdb (skipped when absent): untagged co-loading reproduces the
   mis-binding, distinct tags remove it, and a tagged artifact loaded without its tag fails loudly.
"""
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
import pytest

from shapeperf import elfinfo, paired
from shapeperf.identity import assert_no_shared_model_symbols, model_symbols

CC = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
FIXTURE_C = r"""
#define CAT(a, b) a##b
#define XCAT(a, b) CAT(a, b)
int XCAT(main_graph_, TAG)(void) { return VALUE; }
int XCAT(run_main_graph_, TAG)(void) { return XCAT(main_graph_, TAG)(); }
"""


def _fixture(tmp_path, name, tag, value):
    src = tmp_path / "fixture.c"
    src.write_text(FIXTURE_C)
    d = tmp_path / name
    d.mkdir()
    out = d / "model.so"
    subprocess.run([CC, "-O0", "-fPIC", "-shared", f"-DTAG={tag}", f"-DVALUE={value}", str(src), "-o", str(out)],
                   check=True, capture_output=True)
    return out


def _calls_through_plt(so, symbol):
    rel = subprocess.run(["readelf", "-rW", str(so)], capture_output=True, text=True).stdout
    return any("JUMP_SLOT" in ln and ln.split()[-3] == symbol for ln in rel.splitlines() if len(ln.split()) >= 3)


pytestmark_cc = pytest.mark.skipif(CC is None or shutil.which("readelf") is None, reason="needs a C compiler and readelf")

CHILD = r"""
import ctypes, json, os, sys
libs = json.loads(sys.argv[1])
handles = [ctypes.CDLL(p, mode=os.RTLD_LAZY | os.RTLD_GLOBAL) for p, _tag in libs]
print(json.dumps([getattr(h, f"run_main_graph_{tag}")() for h, (_p, tag) in zip(handles, libs)]))
"""


def _run_child(libs):
    p = subprocess.run([sys.executable, "-c", CHILD, json.dumps([[str(a), t] for a, t in libs])],
                       capture_output=True, text=True, check=True)
    return json.loads(p.stdout)


@pytestmark_cc
def test_rtld_global_same_tag_runs_first_loaded_compute(tmp_path):
    a = _fixture(tmp_path, "A", "model", 8)
    b = _fixture(tmp_path, "B", "model", 1)
    if not _calls_through_plt(b, "main_graph_model"):
        pytest.skip("this compiler bound the call locally; the fixture cannot show interposition")
    assert _run_child([(a, "model"), (b, "model")]) == [8, 8]      # B's entry ran A's compute
    assert _run_child([(b, "model"), (a, "model")]) == [1, 1]      # and vice versa: load order decides
    assert _run_child([(b, "model")]) == [1]                       # alone: its own


@pytestmark_cc
def test_rtld_global_distinct_tags_run_own_compute(tmp_path):
    a = _fixture(tmp_path, "A", "alpha", 8)
    b = _fixture(tmp_path, "B", "bravo", 1)
    assert _run_child([(a, "alpha"), (b, "bravo")]) == [8, 1]
    assert _run_child([(b, "bravo"), (a, "alpha")]) == [1, 8]


@pytestmark_cc
def test_guard_refuses_shared_model_symbols(tmp_path):
    a = _fixture(tmp_path, "A", "model", 8)
    b = _fixture(tmp_path, "B", "model", 1)
    c = _fixture(tmp_path, "C", "bravo", 1)
    assert {"main_graph_model", "run_main_graph_model"} <= model_symbols(a)
    with pytest.raises(ValueError, match="cannot be co-loaded"):
        assert_no_shared_model_symbols([a, b])
    assert_no_shared_model_symbols([a, c])          # distinct tags
    assert_no_shared_model_symbols([a, a])          # the same file twice is one library


@pytestmark_cc
def test_elfinfo_reads_function_bytes(tmp_path):
    a = _fixture(tmp_path, "A", "alpha", 8)
    info = elfinfo.read_elf(a)
    sym = elfinfo.find_symbol(info, "main_graph_alpha")
    assert sym["type"] == "FUNC" and sym["bind"] == "GLOBAL" and sym["visibility"] == "DEFAULT"
    assert len(elfinfo.function_bytes(a, "main_graph_alpha", info)) == sym["size"] > 0


@pytestmark_cc
def test_pair_worker_refuses_before_loading_and_passes_tags(tmp_path, monkeypatch):
    x = np.arange(6, dtype=np.float32).reshape(2, 3)
    np.save(tmp_path / "x.npy", x)
    np.save(tmp_path / "y.npy", x)
    a = _fixture(tmp_path, "A", "model", 8)
    b = _fixture(tmp_path, "B", "model", 1)
    opened = []

    class FakeSession:
        def __init__(self, shared_lib_path, tag=""):
            opened.append((shared_lib_path, tag))

        def run(self, inputs):
            return [inputs[0]]

    from shapeperf import toolchain
    monkeypatch.setattr(toolchain, "import_pyruntime", lambda: FakeSession)
    spec = {"input_npy": str(tmp_path / "x.npy"), "expected_npy": str(tmp_path / "y.npy"), "calls": 1, "rounds": 1,
            "warmup_calls": 0, "cpus": None, "seed": 0}
    with pytest.raises(ValueError, match="cannot be co-loaded"):
        paired.subgraph_pair_worker(dict(spec, artifacts={"S8": str(a), "S1": str(b)}))
    assert opened == []                              # refused before anything was loaded
    c = _fixture(tmp_path, "C", "alpha", 8)
    d = _fixture(tmp_path, "D", "bravo", 1)
    res = paired.subgraph_pair_worker(dict(spec, artifacts={"S8": {"path": str(c), "tag": "alpha"},
                                                            "S1": {"path": str(d), "tag": "bravo"}}))
    assert opened == [(str(c), "alpha"), (str(d), "bravo")] and res["exact"] == {"S8": True, "S1": True}


# ------------------------------------------------------------- real artifacts (development container only)

REAL = Path("/home/user/work/v3/g2/K/L0064")
TAGGED = Path("/home/user/work/v3_followup/e1/K_L0064_tagged")
needs_real = pytest.mark.skipif(not (REAL / "S8_full/model.so").exists() or not (TAGGED / "S8_alpha_full/model.so").exists()
                                or shutil.which("gdb") is None, reason="needs the L=64 artifacts, the pinned runtime and gdb")


def _identity(tmp_path, set_name, case_id):
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "scripts"))
    import verify_execution_identity as V
    arms, cell = V.arm_sets(str(tmp_path / "work"))[set_name]
    if set_name == "orig":
        Path(arms["AA"]["path"]).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(arms["S8"]["path"], arms["AA"]["path"])
    desc, steps = V.cases(set_name)[case_id]
    res = V.run_case(case_id, desc, steps, arms, cell, tmp_path / "out")
    return V.analyse_case(res, arms, "n/a")


@needs_real
def test_real_untagged_coloading_runs_other_arm(tmp_path):
    rows = _identity(tmp_path, "orig", "C81-1")
    assert [(r["requested_arm"], r["actual_compute_arm"], r["identity_verdict"]) for r in rows] == [
        ("S1", "S8", "verified_other"), ("S8", "S8", "verified_own"),
        ("S1", "S8", "verified_other"), ("S8", "S8", "verified_own")]


@needs_real
def test_real_distinct_tags_run_own_arm(tmp_path):
    rows = _identity(tmp_path, "tagged", "C8a1b-1")
    assert all(r["identity_verdict"] == "verified_own" for r in rows) and len(rows) == 4


@needs_real
def test_real_missing_tag_fails_loudly():
    code = ("import sys; sys.path.insert(0, '.'); from shapeperf import toolchain; "
            f"toolchain.import_pyruntime()(shared_lib_path='{TAGGED / 'S8_alpha_full/model.so'}')")
    env = dict(os.environ, SHAPEPERF_WORK=os.environ.get("SHAPEPERF_WORK", "/home/user/work"))
    p = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, env=env,
                       cwd=Path(__file__).resolve().parent.parent)
    assert p.returncode != 0 and "omQueryEntryPoints_model" in (p.stderr + p.stdout)

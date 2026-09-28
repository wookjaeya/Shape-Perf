"""End-to-end evaluator pipeline on SYNTHETIC records (answer table -> census
metrics -> policy comparison). Exercises the code paths only; numbers are fake."""
import json
import os
import subprocess
import sys
from pathlib import Path

import numpy as np
import pytest

ROOT = Path(__file__).resolve().parent.parent


def _write(path, recs):
    with open(path, "w") as f:
        for r in recs:
            f.write(json.dumps(r) + "\n")


def _meas(seed, step_at, lengths, blocks=3, procs=2):
    rng = np.random.default_rng(seed)
    out = []
    for b in range(blocks):
        for s in lengths:
            base = 1e6 * (1 + 0.01 * s) * (1.5 if s >= step_at else 1.0)
            for k in range(procs):
                out.append({"padded_length": s, "block_id": f"{seed}-b{b}", "vm_allocation_id": f"vm{seed}",
                            "run_id": f"r{seed}-{b}-{s}-{k}",
                            "latency_ns": (base * np.exp(rng.normal(0, 0.005, 10))).tolist(),
                            "failure_type": None, "warmup_wall_ns": 1e6, "measurement_wall_ns": 1e7})
    return out


def _prereg(tmp_path, **sel):
    pre = json.loads((ROOT / "configs/preregistration.json").read_text())
    pre["events"].update(delta_min_effect=0.2, alpha=0.05, confidence_level=0.95, match_tolerance_lengths=0)
    pre["measurement"]["process_statistic"] = "mean"
    pre["selectors"].update(random_seeds=[1, 2], shape_only_alignment_units=[8], report_overhead_ns=0,
                            compile_probe_budget_fraction=0.3, budgets_ns=[3e11, 2e12])
    pre["selectors"].update(sel)
    p = tmp_path / "prereg.json"
    p.write_text(json.dumps(pre))
    return {**os.environ, "SHAPEPERF_PREREG": str(p)}


def _run(env, *args, ok=True):
    p = subprocess.run([sys.executable, str(ROOT / "evaluate.py"), *args, "--allow-unfrozen"],
                       capture_output=True, text=True, cwd=ROOT, env=env)
    if ok:
        assert p.returncode == 0, p.stderr
    return p


@pytest.fixture()
def synthetic(tmp_path):
    lengths = list(range(10, 42))
    failed = 30                                   # compile failure: no measurements at 30
    meas_lengths = [s for s in lengths if s != failed]
    _write(tmp_path / "d.jsonl", _meas(1, 27, meas_lengths))
    _write(tmp_path / "c.jsonl", _meas(2, 27, meas_lengths))
    _write(tmp_path / "compile.jsonl", [
        {"padded_length": s, "compile_wall_ns": 3e10, "feature_extract_wall_ns": 1e8, "verify_wall_ns": 2e9,
         "ir_signature": "A" if s < 27 else "B", "failure_type": "compile_error" if s == failed else None}
        for s in lengths])
    _write(tmp_path / "census.jsonl", [
        {"padded_length": s, "probe_wall_ns": 5e9, "feature_extract_wall_ns": 1e8,
         "ir_signature": "A" if s < 27 else "B", "ir_structure_signature": "X" if s < 27 else "Y",
         "raw_ir_hash": f"h{s}"} for s in lengths])
    (tmp_path / "census_table.json").write_text(json.dumps({"lengths": [10, 41], "valid_lengths": lengths,
                                                             "C_sig": [26], "C_ir_structure": [26],
                                                             "C_raw": lengths[:-1]}))
    return tmp_path


def test_evaluator_pipeline(synthetic):
    t = synthetic
    env = _prereg(t)
    at = t / "at.json"
    _run(env, "answer-table", "--discovery", str(t / "d.jsonl"), "--confirmation", str(t / "c.jsonl"),
         "--valid", "10-41", "--out", str(at))
    tab = json.loads(at.read_text())
    a = tab["by_delta"]["0.2"]["answer_table_A"]
    assert a["events"] == [26] and a["family_size"] == 31 and a["n_missing_pairs"] == 2   # (29,30),(30,31)
    assert "30" not in tab["per_shape"]["discovery"]
    _run(env, "census", "--census-table", str(t / "census_table.json"), "--answer-table", str(at),
         "--out", str(t / "cm.json"))
    cm = json.loads((t / "cm.json").read_text())["by_delta"]["0.2"]
    assert cm["opt_report"]["Prec_sig"] == 1.0 and cm["opt_report"]["C_nonalign"] == [26]
    assert cm["raw_ir_hash (ablation 11.2-3)"]["Prec_sig"] < 0.1
    _run(env, "compare", "--compile", str(t / "compile.jsonl"), "--measurements", str(t / "d.jsonl"),
         "--census", str(t / "census.jsonl"), "--answer-table", str(at), "--out", str(t / "cmp.json"))
    cmp = json.loads((t / "cmp.json").read_text())
    res = cmp["by_delta"]["0.2"]["results"]
    assert {"uniform", "compile_probe", "compile_guided[align_only]", "timing_only[cost-free]",
            "compile_probe[raw-ir]", "compile_probe[ir-structure]"} <= set(res)
    assert res["compile_probe"]["recall_cost"][-1]["recall_mean"] == 1.0
    row = res["uniform"]["recall_cost"][-1]
    assert {"recall_ci_mean", "common_ns_mean", "policy_extra_ns_mean", "unused_budget_fraction_mean"} <= set(row)
    assert Path(cmp["runs_file"]).exists()
    assert all(lv.startswith(("os:", "audit-only", "partial:")) for lv in cmp["isolation_levels"])


def test_compare_without_or_with_partial_census_skips_probe_variants(synthetic):
    t = synthetic
    env = _prereg(t)
    _run(env, "answer-table", "--discovery", str(t / "d.jsonl"), "--confirmation", str(t / "c.jsonl"),
         "--valid", "10-41", "--out", str(t / "at.json"))
    part = [json.loads(line) for line in (t / "census.jsonl").read_text().splitlines()][:10]
    _write(t / "census_part.jsonl", part)
    for extra in ([], ["--census", str(t / "census_part.jsonl")]):
        _run(env, "compare", "--compile", str(t / "compile.jsonl"), "--measurements", str(t / "d.jsonl"),
             "--answer-table", str(t / "at.json"), "--out", str(t / "cmp2.json"), *extra)
        out = json.loads((t / "cmp2.json").read_text())
        assert out["skipped_variants"]
        assert not any(k.startswith("compile_probe") for k in out["by_delta"]["0.2"]["results"])


def test_answer_table_needs_explicit_valid_set_and_independent_data(synthetic):
    t = synthetic
    env = _prereg(t)
    p = _run(env, "answer-table", "--discovery", str(t / "d.jsonl"), "--confirmation", str(t / "c.jsonl"),
             "--out", str(t / "x.json"), ok=False)
    assert p.returncode != 0 and "valid length set must be explicit" in p.stderr
    p = _run(env, "answer-table", "--discovery", str(t / "d.jsonl"), "--confirmation", str(t / "d.jsonl"),
             "--valid", "10-41", "--out", str(t / "x.json"), ok=False)
    assert p.returncode != 0 and "not independent" in p.stderr


def test_empty_preregistered_units_stay_empty(synthetic):
    t = synthetic
    env = _prereg(t, shape_only_alignment_units=[])
    _run(env, "answer-table", "--discovery", str(t / "d.jsonl"), "--confirmation", str(t / "c.jsonl"),
         "--valid", "10-41", "--out", str(t / "at.json"))
    p = _run(env, "census", "--census-table", str(t / "census_table.json"), "--answer-table", str(t / "at.json"),
             "--units", "4", "--out", str(t / "cm.json"), ok=False)
    assert p.returncode != 0 and "conflicts" in p.stderr         # CLI may not override a preregistered []
    _run(env, "census", "--census-table", str(t / "census_table.json"), "--answer-table", str(t / "at.json"),
         "--out", str(t / "cm.json"))
    cm = json.loads((t / "cm.json").read_text())
    assert cm["units"] == [] and cm["by_delta"]["0.2"]["opt_report"]["n_B_align"] == 0

"""End-to-end evaluator pipeline on SYNTHETIC records (answer table -> census
metrics -> policy comparison). Exercises the code paths only; numbers are fake."""
import json
import os
import subprocess
import sys
from pathlib import Path

import numpy as np

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
            for _ in range(procs):
                out.append({"padded_length": s, "block_id": f"{seed}-b{b}", "vm_allocation_id": f"vm{seed}",
                            "latency_ns": (base * np.exp(rng.normal(0, 0.005, 10))).tolist(),
                            "failure_type": None, "warmup_wall_ns": 1e6, "measurement_wall_ns": 1e7})
    return out


def test_evaluator_pipeline(tmp_path):
    lengths = list(range(10, 42))
    _write(tmp_path / "d.jsonl", _meas(1, 27, lengths))
    _write(tmp_path / "c.jsonl", _meas(2, 27, lengths))
    _write(tmp_path / "compile.jsonl", [{"padded_length": s, "compile_wall_ns": 3e10, "feature_extract_wall_ns": 1e8,
                                         "verify_wall_ns": 2e9, "ir_signature": "A" if s < 27 else "B"}
                                        for s in lengths])
    _write(tmp_path / "census.jsonl", [{"padded_length": s, "probe_wall_ns": 5e9, "feature_extract_wall_ns": 1e8,
                                        "ir_signature": "A" if s < 27 else "B"} for s in lengths])
    (tmp_path / "census_table.json").write_text(json.dumps({"lengths": [10, 41], "C_sig": [26],
                                                             "C_ir_structure": [26]}))
    py = sys.executable
    at = tmp_path / "at.json"
    # prereg values are null in the repo -> pass them through a temporary preregistration copy
    pre = json.loads((ROOT / "configs/preregistration.json").read_text())
    pre["events"].update(delta_min_effect=0.2, alpha=0.05, confidence_level=0.95, bootstrap_reps=1000,
                         bootstrap_seed=0, match_tolerance_lengths=0)
    pre["measurement"]["process_statistic"] = "mean"
    pre["selectors"].update(random_seeds=[1, 2], shape_only_alignment_units=[8],
                            compile_probe_budget_fraction=0.3, budgets_ns=[3e11, 2e12])
    env_pre = tmp_path / "prereg.json"
    env_pre.write_text(json.dumps(pre))
    env = {**os.environ, "SHAPEPERF_PREREG": str(env_pre)}

    def run(*args):
        p = subprocess.run([py, str(ROOT / "evaluate.py"), *args, "--allow-unfrozen"], capture_output=True,
                           text=True, cwd=ROOT, env=env)
        assert p.returncode == 0, p.stderr
        return p.stdout

    run("answer-table", "--discovery", str(tmp_path / "d.jsonl"), "--confirmation", str(tmp_path / "c.jsonl"),
        "--out", str(at))
    tab = json.loads(at.read_text())
    assert tab["by_delta"]["0.2"]["answer_table_A"]["events"] == [26]
    run("census", "--census-table", str(tmp_path / "census_table.json"), "--answer-table", str(at),
        "--out", str(tmp_path / "cm.json"))
    cm = json.loads((tmp_path / "cm.json").read_text())["opt_report"]
    assert cm["Prec_sig"] == 1.0 and cm["C_nonalign"] == [26]
    run("compare", "--compile", str(tmp_path / "compile.jsonl"), "--measurements", str(tmp_path / "d.jsonl"),
        "--census", str(tmp_path / "census.jsonl"), "--answer-table", str(at), "--out", str(tmp_path / "cmp.json"))
    cmp = json.loads((tmp_path / "cmp.json").read_text())
    assert {"uniform", "compile_probe", "compile_guided[align_only]", "timing_only[cost-free]"} <= set(cmp["results"])
    last = {k: v["recall_cost"][-1]["recall_mean"] for k, v in cmp["results"].items()}
    assert last["compile_probe"] == 1.0

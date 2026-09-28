#!/usr/bin/env python3
"""Replicate confirmed boundaries on other real inputs (spec §4.3, §11.2-4) -
EVALUATOR ONLY.

For every confirmed event (s, t) of an answer table, draw a seeded question-level
sample of other features with L(x) <= s (one window per question, so windows of
the same question are not counted as independent samples, spec §4.3), measure
lengths s and t for each sampled feature inside the same blocks (paired), and
report the per-feature effect and its spread. An event seen only on the anchor
is reported as an anchor-only case.

  python scripts/replicate_boundaries.py --answer-table results/eval/answer_table_A.json \
      --compile results/g4/<run>/compile.jsonl --features-per-event N --seed S \
      --vm-allocation-id ID
"""
import argparse
import math
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import evaluate as E  # noqa: E402
from shapeperf import prereg  # noqa: E402
from shapeperf.compile import model_def  # noqa: E402
from shapeperf.measure import run_block  # noqa: E402
from shapeperf.squad import load_features  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, new_run_id, read_json, read_jsonl, write_json  # noqa: E402


def sample_features(feat, s, n, rng, exclude):
    """One feature per question among features with L(x) <= s, seeded."""
    by_q = defaultdict(list)
    for i in np.flatnonzero(feat["valid_length"] <= s):
        if int(i) not in exclude:
            by_q[str(feat["qas_id"][i])].append(int(i))
    qs = sorted(by_q)
    pick = rng.choice(len(qs), size=min(n, len(qs)), replace=False) if qs else []
    return [by_q[qs[k]][int(rng.integers(len(by_q[qs[k]])))] for k in pick]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--answer-table", required=True)
    ap.add_argument("--delta", default=None, help="answer-table delta key (default: first)")
    ap.add_argument("--compile", nargs="+", required=True, help="G4 compile.jsonl with the artifacts")
    ap.add_argument("--features-per-event", type=int, default=None,
                    help="dev runs only; frozen runs use replication.features_per_event (spec §4.3)")
    ap.add_argument("--seed", type=int, required=True)
    ap.add_argument("--vm-allocation-id", required=True)
    ap.add_argument("--allow-unfrozen", action="store_true")
    ap.add_argument("--out", default=None)
    a = ap.parse_args()
    p, label = prereg.require(["measurement.warmup_iterations", "measurement.timed_iterations",
                               "measurement.blocks", "measurement.process_statistic",
                               "events.confidence_level", "replication.features_per_event"], a.allow_unfrozen)
    meas = p["measurement"]
    frozen = label.startswith("frozen")
    n_feat = (p.get("replication") or {}).get("features_per_event")
    if n_feat is None:
        if frozen or a.features_per_event is None:
            raise SystemExit("features per event is not preregistered (replication.features_per_event)")
        n_feat = a.features_per_event
    elif a.features_per_event not in (None, n_feat):
        raise SystemExit(f"--features-per-event {a.features_per_event} conflicts with preregistered {n_feat}")
    conf_level = p["events"]["confidence_level"] or 0.95
    at = read_json(a.answer_table)
    if frozen and at.get("preregistration") != label:
        raise SystemExit(f"answer table was produced under {at.get('preregistration')!r}, not {label!r}")
    flagset = at.get("flagset", "default")
    dkey = a.delta or next(iter(at["by_delta"]))
    events = at["by_delta"][dkey]["answer_table_A"]["event_pairs"]
    arts = {r["padded_length"]: r for f in a.compile for r in read_jsonl(f)
            if r.get("flagset", "default") == flagset and not r.get("failure_type")}
    no_art = [[s, t] for s, t in events if s not in arts or t not in arts]
    if no_art and frozen:
        raise SystemExit(f"no compiled artifact for events {no_art[:5]} (pass the G4 compile.jsonl files)")
    m = model_def(a.model)
    feat = load_features(REPO_ROOT / m["features"] / "features.npz")
    anchor = read_json(REPO_ROOT / m["features"] / "catalog.json")["anchor"]["feature_index"]
    rng = np.random.default_rng(a.seed)
    plan = []
    for s, t in events:
        if s not in arts or t not in arts:
            continue
        for fi in sample_features(feat, s, n_feat, rng, {anchor}):
            for L in (s, t):
                plan.append({"artifact": arts[L]["artifact_path"], "artifact_hash": arts[L]["artifact_hash"],
                             "model_key": a.model, "length": L, "feature_index": fi, "flagset": flagset,
                             "warmup": meas["warmup_iterations"] or 0, "iterations": meas["timed_iterations"] or 1,
                             "cpus": meas.get("cpus")})
    run_id = new_run_id("replicate")
    out = Path(a.out or REPO_ROOT / "results/g4" / run_id)
    out.mkdir(parents=True, exist_ok=True)
    raw = out / "measurements.jsonl"
    for blk in range(meas["blocks"] or 1):
        run_block(plan, raw, seed=a.seed * 1000 + blk, vm_allocation_id=a.vm_allocation_id,
                  experiment_phase=f"replication {label}", threads=meas.get("threads") or 1)
    stat = meas["process_statistic"] or "mean"
    recs = [r for r in read_jsonl(raw) if not r.get("failure_type")]
    # paired cells: (feature, length, block) -> process statistics
    cells = defaultdict(list)
    for r in recs:
        cells[(r["feature_index"], r["padded_length"], r["block_id"])].append(E.process_stat(r["latency_ns"], stat))
    summary = []
    for s, t in events:
        if [s, t] in no_art:
            summary.append({"s": s, "t": t, "status": "not measured: artifact missing from --compile"})
            continue
        effects = []
        feats = sorted({fi for (fi, L, _b) in cells if L == s})
        for fi in feats:
            per_block = []
            for (f2, L, b), v in cells.items():
                if f2 == fi and L == s and (fi, t, b) in cells:
                    per_block.append(math.log(np.mean(cells[(fi, t, b)]) / np.mean(v)))
            if per_block:
                effects.append(float(np.mean(per_block)))
        x = np.asarray(effects, float)
        summary.append({"s": s, "t": t, "n_features": int(x.size),
                        "mean_log_effect": float(x.mean()) if x.size else None,
                        "ci_log": E._mean_ci(x.tolist(), conf_level) if x.size >= 2 else None,
                        "confidence_level": conf_level,
                        "fraction_same_direction_as_anchor": None if not x.size else float(np.mean(
                            np.sign(x) == np.sign(next(e["discovery"]["log_effect"] for e in
                                                        at["by_delta"][dkey]["answer_table_A"]["detail"]
                                                        if e["s"] == s))))})
    write_json(out / "replication.json", {"answer_table": a.answer_table, "delta": dkey, "preregistration": label,
                                          "seed": a.seed, "features_per_event": n_feat, "flagset": flagset,
                                          "harness_commit": git_head(), "events": summary,
                                          "note": "an event not reproduced on other inputs is an anchor-only case (§4.3)"})
    print("wrote", out / "replication.json")


if __name__ == "__main__":
    main()

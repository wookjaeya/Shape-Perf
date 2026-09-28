#!/usr/bin/env python3
"""Evaluator CLI (spec §12 evaluate.py). Isolated from the selectors: only this
side reads census/ and the dense G4 data.

  answer-table : continuous indicators A(s) + answer table A (spec §9.1-9.3)
  census       : Prec_sig / Rec_sig / non-aligned contribution (spec §9.4)
  compare      : policy comparison on the recorded dense data (spec §11.1)

All thresholds come from configs/preregistration.json. --allow-unfrozen runs
are labelled and are not results.
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shapeperf import evaluate as E  # noqa: E402
from shapeperf import prereg  # noqa: E402
from shapeperf.backends import ReplayBackend  # noqa: E402
from shapeperf.broker import QueryBroker  # noqa: E402
from shapeperf.selectors import REGISTRY  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, read_json, read_jsonl, write_json  # noqa: E402


def _load(paths):
    return [r for p in paths for r in read_jsonl(p)]


def cmd_answer_table(a):
    p, label = prereg.require(["events.delta_min_effect", "events.alpha", "events.confidence_level",
                               "events.bootstrap_reps", "events.bootstrap_seed",
                               "measurement.process_statistic"], a.allow_unfrozen)
    ev, stat = p["events"], p["measurement"]["process_statistic"] or "mean"
    disc_recs, conf_recs = _load(a.discovery), _load(a.confirmation)
    disc, conf = E.block_table(disc_recs, stat), E.block_table(conf_recs, stat)
    lengths = sorted({r["padded_length"] for r in disc_recs})
    flop_cfg = None
    meta = REPO_ROOT / "data/artifacts/derived/bertsquad-12-reexport-dynseq.onnx.json"
    if meta.exists():
        flop_cfg = read_json(meta)["bert_config_derived_from_artifact"]
    deltas = [ev["delta_min_effect"]] + list(ev.get("sensitivity_deltas") or [])
    out = {"preregistration": label, "harness_commit": git_head(), "process_statistic": stat,
           "n_records": {"discovery": len(disc_recs), "confirmation": len(conf_recs)},
           "failures": {"discovery": sum(1 for r in disc_recs if r.get("failure_type")),
                        "confirmation": sum(1 for r in conf_recs if r.get("failure_type"))},
           "by_delta": {}}
    for d in deltas:
        rows = E.continuous_indicators(disc, lengths, ev["bootstrap_reps"], ev["bootstrap_seed"],
                                       ev["confidence_level"], d, flop_cfg)
        tab = E.answer_table_a(rows, conf, d, ev["alpha"], ev["bootstrap_reps"], ev["bootstrap_seed"],
                               ev["confidence_level"])
        out["by_delta"][str(d)] = {"continuous": rows, "answer_table_A": tab}
    write_json(a.out, out)
    primary = out["by_delta"][str(deltas[0])]["answer_table_A"]
    print(json.dumps({"preregistration": label, "E_A": primary["events"], "candidates": primary["candidates"]}))


def cmd_census(a):
    p, label = prereg.require(["events.match_tolerance_lengths", "selectors.shape_only_alignment_units"],
                              a.allow_unfrozen)
    table = read_json(a.census_table)
    at = read_json(a.answer_table)
    d0 = next(iter(at["by_delta"]))
    e_a = at["by_delta"][d0]["answer_table_A"]["events"]
    lo, hi = table["lengths"]
    units = p["selectors"]["shape_only_alignment_units"] or a.units
    b_align = E.aligned_boundaries(range(lo, hi + 1), units or [])
    res = {"preregistration": label, "units": units, "delta": d0,
           "opt_report": E.census_metrics(table["C_sig"], b_align, e_a, p["events"]["match_tolerance_lengths"] or 0),
           "ir_structure": E.census_metrics(table["C_ir_structure"], b_align, e_a,
                                            p["events"]["match_tolerance_lengths"] or 0)}
    write_json(a.out, res)
    print(json.dumps(res["opt_report"], indent=1))


def dense_table(compile_recs, meas_recs, census_recs, stat):
    """Per length: recorded costs + recorded processes (for ReplayBackend)."""
    dense = {}
    for c in compile_recs:
        s = c["padded_length"]
        dense[s] = {"compile_ns": c.get("compile_wall_ns") or 0, "extract_ns": c.get("feature_extract_wall_ns") or 0,
                    "verify_ns": c.get("verify_wall_ns") or 0, "signature": c.get("ir_signature"),
                    "failure_type": c.get("failure_type"), "processes": []}
    for r in meas_recs:
        s = r["padded_length"]
        if s in dense and not r.get("failure_type"):
            dense[s]["processes"].append({"median_ns": E.process_stat(r["latency_ns"], stat),
                                          "warmup_ns": r["warmup_wall_ns"], "measure_ns": r["measurement_wall_ns"]})
    for r in census_recs:
        s = r["padded_length"]
        if s in dense:
            dense[s]["probe_ns"] = r.get("probe_wall_ns") or 0
            dense[s]["probe_extract_ns"] = r.get("feature_extract_wall_ns") or 0
            dense[s]["probe_signature"] = r.get("ir_signature")
            dense[s]["probe_failure_type"] = r.get("failure_type")
    for s, d in dense.items():
        if not d["failure_type"] and not d["processes"]:
            d["failure_type"] = "no_measurement"
    return dense


def cmd_compare(a):
    p, label = prereg.require(["selectors.random_seeds", "selectors.shape_only_alignment_units",
                               "selectors.compile_probe_budget_fraction", "selectors.budgets_ns",
                               "events.delta_min_effect", "measurement.process_statistic"], a.allow_unfrozen)
    sp, ev = p["selectors"], p["events"]
    stat = p["measurement"]["process_statistic"] or "mean"
    at = read_json(a.answer_table)
    d0 = next(iter(at["by_delta"]))
    tab = at["by_delta"][d0]["answer_table_A"]
    events = tab["events"]
    confirmed = set(events)
    dense = dense_table(_load(a.compile), _load(a.measurements), _load(a.census) if a.census else [], stat)
    valid = sorted(dense)
    conf_cost = {s: float(np.mean([pp["measure_ns"] + pp["warmup_ns"] for pp in d["processes"]]))
                 if d["processes"] else 0.0 for s, d in dense.items()}
    seeds = sp["random_seeds"] or a.seeds
    budgets = sp["budgets_ns"] or a.budgets
    thr = float(np.log1p(ev["delta_min_effect"] or a.delta))
    params = {"random": {}, "shape_only": {"units": sp["shape_only_alignment_units"] or a.units},
              "compile_probe": {"budget_fraction": sp["compile_probe_budget_fraction"] or a.probe_fraction}}
    variants = [(n, {}) for n in REGISTRY]
    variants += [("compile_guided", {"align_only_units": params["shape_only"]["units"], "_tag": "align_only"}),
                 ("compile_probe", {"hybrid_uniform": True, "_tag": "hybrid"})]
    results = {}
    for name, extra in variants:
        tag = name + (f"[{extra.pop('_tag')}]" if "_tag" in extra else "")
        kw = {**params.get(name, {}), **extra}
        for charge in (True, False):
            key = f"{tag}{'' if charge else '[cost-free]'}"
            by_b = {}
            for b in budgets:
                runs = []
                for sd in seeds:
                    sel = REGISTRY[name](seed=sd, **kw)
                    be = ReplayBackend(dense, seed=sd, confirm_fn=lambda x, y: x in confirmed,
                                       confirm_cost_fn=lambda x, y: conf_cost[x] + conf_cost[y])
                    runs.append(QueryBroker(be, valid, charge_policy_extra=charge,
                                            confirm_log_threshold=thr).run(sel, int(b)))
                by_b[int(b)] = runs
            results[key] = {"recall_cost": E.recall_cost(by_b, events, require_confirmation=True),
                            "recall_cost_resolved_only": E.recall_cost(by_b, events, require_confirmation=False),
                            "cost_to_find": {str(e): [E.discoveries(r, [e], True).get(e) for r in by_b[max(by_b)]]
                                             for e in events}}
    out = {"preregistration": label, "harness_commit": git_head(), "E_A": events, "delta": d0,
           "seeds": seeds, "budgets_ns": budgets, "results": results,
           "note": "ReplayBackend charges recorded real costs; common vs policy-extra costs are split per query"}
    write_json(a.out, out)
    for k, v in results.items():
        last = v["recall_cost"][-1]
        print(f"{k:40s} recall@{last['budget_ns']:.3g}ns = {last['recall_mean']:.3f}")


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    s1 = sub.add_parser("answer-table")
    s1.add_argument("--discovery", nargs="+", required=True)
    s1.add_argument("--confirmation", nargs="+", required=True)
    s1.add_argument("--out", default=str(REPO_ROOT / "results/eval/answer_table_A.json"))
    s2 = sub.add_parser("census")
    s2.add_argument("--census-table", required=True)
    s2.add_argument("--answer-table", required=True)
    s2.add_argument("--units", type=int, nargs="*", default=None)
    s2.add_argument("--out", default=str(REPO_ROOT / "results/eval/census_metrics.json"))
    s3 = sub.add_parser("compare")
    s3.add_argument("--compile", nargs="+", required=True)
    s3.add_argument("--measurements", nargs="+", required=True)
    s3.add_argument("--census", nargs="*", default=None)
    s3.add_argument("--answer-table", required=True)
    s3.add_argument("--seeds", type=int, nargs="*", default=None)
    s3.add_argument("--budgets", type=float, nargs="*", default=None)
    s3.add_argument("--units", type=int, nargs="*", default=None)
    s3.add_argument("--probe-fraction", type=float, default=None)
    s3.add_argument("--delta", type=float, default=None)
    s3.add_argument("--out", default=str(REPO_ROOT / "results/eval/policy_comparison.json"))
    for s in (s1, s2, s3):
        s.add_argument("--allow-unfrozen", action="store_true")
    a = ap.parse_args()
    {"answer-table": cmd_answer_table, "census": cmd_census, "compare": cmd_compare}[a.cmd](a)


if __name__ == "__main__":
    main()

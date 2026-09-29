#!/usr/bin/env python3
"""Kernel-level effect of the transpose unroll policy on the observed sub-graph (design amendment v3, G2).

For kind K (the scalar transpose the patch applies to) and Q (negative control, patched code never
reached): per length the paired S8-vs-S1 difference in relative (log) and absolute (us per call)
terms, the byte-identical A/A arm, and the exactness of every output (bitwise np.transpose).

MODEL-LEVEL BOUND: the model has 12 K-type transposes of exactly this shape, so the whole-model
slowdown caused by the policy is at most about 12 * dt / T_model(L), with dt the kernel difference per
call and T_model the model latency at L (from the G1 pilot, arm S1). It ignores the one output
transpose and any cache interaction with the neighbouring ops; it is a bound on the kernel's
contribution, not a measurement of the model.

  python analysis/subgraph_effects.py --raw <sub measurements.jsonl> --g1-raw <G1 measurements.jsonl> --out <json>
"""
import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import paired  # noqa: E402
from shapeperf.util import git_head, read_jsonl, write_json  # noqa: E402

N_K_TRANSPOSES = 12


def model_latency_ns(g1_records):
    """Mean process-mean latency of arm S1 per length in the G1 pilot."""
    by = {}
    for r in g1_records:
        if r.get("failure_type") or r.get("flagset") != "S1":
            continue
        by.setdefault(r["padded_length"], []).append(float(np.mean(r["latency_ns"])))
    return {L: float(np.mean(v)) for L, v in by.items()}


def analyse_kind(records, kind, t_model):
    recs = [r for r in records if r.get("model_key") == f"subgraph:{kind}"]
    exact = [r.get("exact_permutation") for r in recs if not r.get("failure_type")]
    table = paired.process_means(recs, 0, "mean")
    robust = paired.process_means(recs, 0, "median")
    rows = []
    for L in sorted({L for L, _b in table}):
        row = {"length": L}
        row["S8_vs_S1"] = paired.effect_summary(paired.paired_log_ratios(table, L, "S8", "S1"))
        row["S8_vs_S1[median]"] = paired.effect_summary(paired.paired_log_ratios(robust, L, "S8", "S1"))
        row["AA_S8_vs_AA"] = paired.effect_summary(paired.paired_log_ratios(table, L, "S8", "AA"))
        row["abs_S8_minus_S1"] = paired.abs_summary(paired.paired_abs_diffs(table, L, "S8", "S1"))
        row["abs_S8_minus_AA"] = paired.abs_summary(paired.paired_abs_diffs(table, L, "S8", "AA"))
        s1 = [np.mean(d["S1"]) for (LL, _b), d in table.items() if LL == L and d.get("S1")]
        row["S1_us_per_call"] = float(np.mean(s1)) / 1e3 if s1 else None
        if kind == "K" and L in t_model and row["abs_S8_minus_S1"]["mean_ns"] is not None:
            bound = N_K_TRANSPOSES * row["abs_S8_minus_S1"]["mean_ns"] / t_model[L]
            ci = row["abs_S8_minus_S1"]["ci_ns"]
            row["model_level_bound"] = {
                "T_model_ms": t_model[L] / 1e6, "relative": bound,
                "relative_ci": None if ci is None else [N_K_TRANSPOSES * c / t_model[L] for c in ci]}
        rows.append(row)
    return {"kind": kind, "n_processes": len(recs), "n_failed": sum(1 for r in recs if r.get("failure_type")),
            "all_outputs_exact_permutation": bool(exact) and all(exact), "n_exact": int(sum(bool(e) for e in exact)),
            "lengths": rows}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw", required=True, nargs="+")
    ap.add_argument("--g1-raw", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    recs = [r for p in a.raw for r in read_jsonl(p)]
    t_model = model_latency_ns(list(read_jsonl(a.g1_raw)))
    res = {"harness_commit": git_head(), "labels": sorted({str(r.get("experiment_phase")) for r in recs}),
           "note": "development container: not a measurement VM; not a result (amendment §9.1)",
           "kinds": {k: analyse_kind(recs, k, t_model) for k in ("K", "Q")}}
    write_json(a.out, res)
    for k, d in res["kinds"].items():
        print(f"--- kind {k}: {d['n_processes']} processes, failed {d['n_failed']}, exact outputs {d['n_exact']} (all: {d['all_outputs_exact_permutation']})")
        print(f"{'L':>4} {'S1 us':>7} {'S8/S1 %':>8} {'CI95 %':>17} {'dt us':>7} {'A/A %':>7} {'model-bound %':>14}")
        for r in d["lengths"]:
            m, aa, ab = r["S8_vs_S1"], r["AA_S8_vs_AA"], r["abs_S8_minus_S1"]
            mb = r.get("model_level_bound")
            f = lambda v, s=100: "     -" if v is None else f"{v * s:7.2f}"
            ci = "       -        " if not m["ci_log"] else f"[{m['ci_log'][0] * 100:6.2f},{m['ci_log'][1] * 100:6.2f}]"
            print(f"{r['length']:>4} {r['S1_us_per_call']:7.1f} {f(m['mean_log'])} {ci} {f(ab['mean_ns'], 1e-3)} "
                  f"{f(aa['mean_log'])} {'      -' if not mb else f(mb['relative'])}")


if __name__ == "__main__":
    main()

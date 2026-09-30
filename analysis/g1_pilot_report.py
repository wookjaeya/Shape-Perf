#!/usr/bin/env python3
"""Assemble the G1 pilot report (design amendment v3) from the pilot's raw outputs.

Reads: pairs/L*/pair.json (build + IR checks), verify/verify_L*.json (correctness),
timing/measurements.jsonl (raw paired timing), and writes pilot_report.json.

A length is a CANDIDATE for G2 only if, at the same time,
  - the 95% interval of the primary statistic (process mean) excludes 0,
  - the 95% interval of the median-based secondary statistic excludes 0 with the same sign,
  - |mean effect| exceeds twice the pooled A/A block sd (the observed noise of identical binaries).
The rule was written AFTER the per-length table of this pilot had been looked at (only L=147 had a
mean interval excluding 0). It is a screening rule for this pilot, not a hypothesis test, and it is
fixed here before any further data are collected.

  python analysis/g1_pilot_report.py --run <g1 dir> --out <pilot_report.json>
"""
import argparse
import glob
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from analysis.specialization_effects import analyse  # noqa: E402
from shapeperf.util import read_jsonl, write_json  # noqa: E402


def excludes_zero(ci):
    return ci is not None and (ci[0] > 0 or ci[1] < 0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    run = Path(a.run)
    pairs = {}
    for f in sorted(glob.glob(str(run / "pairs/L*/pair.json"))):
        e = json.loads(Path(f).read_text())
        pairs[e["length"]] = e
    verify = [r for f in sorted(glob.glob(str(run / "verify/verify_L*.json"))) for r in json.loads(Path(f).read_text())]
    recs = list(read_jsonl(run / "timing/measurements.jsonl"))
    eff = analyse(recs)
    rows = {r["length"]: r for r in eff["lengths"]}

    ok = [r for r in verify if "S8_vs_S1" in r]
    correctness = {
        "pairs_checked": len(ok), "bitwise_identical_S8_S1": sum(r["S8_vs_S1"]["bitwise_identical"] for r in ok),
        "max_abs_vs_ORT_valid_positions": max(r[f"{arm}_vs_ORT"][f"{k}_max_abs_valid"]
                                              for r in ok for arm in ("S8", "S1") for k in ("start", "end")),
        "argmax_equal_all": all(r[f"{arm}_vs_ORT"][f"{k}_argmax_equal"]
                                for r in ok for arm in ("S8", "S1") for k in ("start", "end")),
        "static_shape_repeat_finite_ok": all(w.get("static_shape_verified") and w.get("repeat_identical") and w.get("finite")
                                             for r in ok for w in r["workers"].values()),
        "tolerance": None, "note": "no tolerance is set (amendment §11.5); ORT differences are descriptive"}
    compile_checks = {
        "lengths": sorted(pairs),
        "u": {str(L): pairs[L]["ir_check"]["u"] for L in sorted(pairs)},
        "ir_predictions_hold": f"{sum(e['ir_check']['prediction_holds'] for e in pairs.values())}/{len(pairs)}",
        "so_identical_as_predicted": f"{sum(e['so_identical'] == e['predicted_so_identical'] for e in pairs.values())}/{len(pairs)}",
        "so_identical_lengths": [L for L, e in sorted(pairs.items()) if e["so_identical"]],
        "so_bytes_S1_minus_S8": {str(L): e["full"]["S1"]["artifact_bytes"] - e["full"]["S8"]["artifact_bytes"]
                                 for L, e in sorted(pairs.items())},
        "normalized_diff_hunks": {str(L): e["ir_check"]["normalized_diff"]["hunks"] for L, e in sorted(pairs.items())}}

    aa_sd = eff["pooled_block_sd_AA"]
    cands = []
    for L, r in rows.items():
        m, md = r["S8_vs_S1"], r["S8_vs_S1[median]"]
        if (excludes_zero(m["ci_log"]) and excludes_zero(md["ci_log"]) and np.sign(m["mean_log"]) == np.sign(md["mean_log"])
                and aa_sd and abs(m["mean_log"]) > 2 * aa_sd):
            cands.append(L)

    def across(stat):
        v = np.array([r[stat]["mean_log"] for r in rows.values() if r[stat]["mean_log"] is not None]) * 100
        return {"n_lengths": int(v.size), "mean_pct": float(v.mean()), "sd_across_lengths_pct": float(v.std(ddof=1)),
                "min_pct": float(v.min()), "max_pct": float(v.max())}
    walls = {L: r["process_wall_s_mean"] * 6 for L, r in rows.items()}       # one block = 3 arms x 2 processes
    report = {
        "labels": {"environment": "development container (not a measurement VM)", "not_a_result": True},
        "design": {"blocks_per_length": 5, "processes_per_arm_per_block": 2, "arms": ["S8", "S1", "AA"],
                   "warmup_iterations": 2, "timed_iterations": 16, "cpu": 3, "input": "controlled anchor"},
        "compile_checks": compile_checks, "correctness": correctness,
        "timing": {"n_records": eff["n_records"] if "n_records" in eff else len(recs), "n_failed": sum(1 for r in recs if r.get("failure_type")),
                   "effects": eff,
                   "across_lengths": {s: across(s) for s in ("S8_vs_S1", "S8_vs_S1[median]", "S8_vs_S1[min]",
                                                             "AA_S8_vs_AA", "AA_S8_vs_AA[median]", "AA_S8_vs_AA[min]")}},
        "cost": {"wall_s_per_block_by_length": {str(L): round(w, 1) for L, w in sorted(walls.items())},
                 "total_wall_s_pilot": round(sum(r["process_wall_s_mean"] * 30 for r in rows.values()), 0)},
        "screening": {"rule": "CI(mean) and CI(median) exclude 0 with the same sign and |mean| > 2 x pooled A/A block sd",
                      "pooled_aa_block_sd": aa_sd, "candidates": cands,
                      "aa_ci_excludes_zero": eff["aa_ci_covers_zero"]["n"] - eff["aa_ci_covers_zero"]["covered"],
                      "s8_vs_s1_ci_excludes_zero": eff["s8_vs_s1_ci_excludes_zero"]["excluded"],
                      "n_lengths": len(rows)},
    }
    write_json(a.out, report)
    print(json.dumps({k: report[k] for k in ("compile_checks", "correctness", "screening")}, indent=1, default=str)[:3500])


if __name__ == "__main__":
    main()

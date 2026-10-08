#!/usr/bin/env python3
"""Paired effects of the S8 vs S1 contrast from raw timing records (design amendment v3).

Per length: a = log T_S8 - log T_S1 per block (positive: the original unroll policy is slower),
its mean, sd and t interval over blocks; the A/A control (S8 vs the byte-identical copy AA) and
S1 vs AA; variance components (iteration, process); the number of blocks a two-sided test with
80% power would need to detect 0.1%, 0.3% and 1% at the observed block sd, and the wall time per
block. Inference units are BLOCKS of one allocation: the claim is limited to that allocation.

  python analysis/specialization_effects.py --raw <measurements.jsonl> --out <summary.json>
"""
import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import paired  # noqa: E402
from shapeperf.util import git_head, read_jsonl, write_json  # noqa: E402

DELTAS = (0.001, 0.003, 0.01)


def analyse(records, skip=0, conf_level=0.95):
    table = paired.process_means(records, skip)
    robust = {s: paired.process_means(records, skip, s) for s in ("median", "min")}
    lengths = sorted({L for L, _b in table})
    rows = []
    for L in lengths:
        row = {"length": L}
        for name, (x, y) in {"S8_vs_S1": ("S8", "S1"), "AA_S8_vs_AA": ("S8", "AA"), "S1_vs_AA": ("S1", "AA")}.items():
            row[name] = paired.effect_summary(paired.paired_log_ratios(table, L, x, y), conf_level)
        for s, tab in robust.items():        # secondary, robust summaries of the same contrasts
            row[f"S8_vs_S1[{s}]"] = paired.effect_summary(paired.paired_log_ratios(tab, L, "S8", "S1"), conf_level)
            row[f"AA_S8_vs_AA[{s}]"] = paired.effect_summary(paired.paired_log_ratios(tab, L, "S8", "AA"), conf_level)
        walls = [r["process_wall_ns"] for r in records if r["padded_length"] == L and not r.get("failure_type")]
        blocks = len({b for (LL, b) in table if LL == L})
        row["process_wall_s_mean"] = float(np.mean(walls)) / 1e9 if walls else None
        row["blocks"] = blocks
        rows.append(row)
    aa = [r["AA_S8_vs_AA"] for r in rows if r["AA_S8_vs_AA"]["sd"] is not None]
    main = [r["S8_vs_S1"] for r in rows if r["S8_vs_S1"]["sd"] is not None]
    pooled_sd = float(np.sqrt(np.mean([m["sd"] ** 2 for m in main]))) if main else None
    aa_sd = float(np.sqrt(np.mean([m["sd"] ** 2 for m in aa]))) if aa else None
    cover = [r["AA_S8_vs_AA"]["ci_log"][0] <= 0 <= r["AA_S8_vs_AA"]["ci_log"][1] for r in rows
             if r["AA_S8_vs_AA"]["ci_log"]]
    excl = [r["S8_vs_S1"]["ci_log"][0] > 0 or r["S8_vs_S1"]["ci_log"][1] < 0 for r in rows
            if r["S8_vs_S1"]["ci_log"]]
    out = {"lengths": rows, "variance": paired.variance_components(records, skip),
           "pooled_block_sd_S8_vs_S1": pooled_sd, "pooled_block_sd_AA": aa_sd,
           "aa_ci_covers_zero": {"n": len(cover), "covered": int(sum(cover))},
           "s8_vs_s1_ci_excludes_zero": {"n": len(excl), "excluded": int(sum(excl))},
           "mean_a_all_lengths": float(np.mean([m["mean_log"] for m in main])) if main else None}
    if pooled_sd:
        blocks_needed = {f"{d * 100:g}%": paired.required_blocks(pooled_sd, d) for d in DELTAS}
        out["blocks_needed_80pct_power"] = blocks_needed
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw", required=True, nargs="+")
    ap.add_argument("--skip", type=int, default=0)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    recs = [r for p in a.raw for r in read_jsonl(p)]
    res = analyse(recs, a.skip)
    res.update(harness_commit=git_head(), n_records=len(recs),
               n_failed=sum(1 for r in recs if r.get("failure_type")),
               labels=sorted({str(r.get("experiment_phase")) for r in recs}),
               note="development container: not a measurement VM; not a result (amendment §9.1)")
    write_json(a.out, res)
    print(f"{'L':>4} {'a=S8/S1 %':>10} {'CI95 %':>18} {'sd %':>6} | {'AA %':>7} {'AA CI %':>18} | blocks")
    for r in res["lengths"]:
        m, aa = r["S8_vs_S1"], r["AA_S8_vs_AA"]
        f = lambda v: "   -  " if v is None else f"{v * 100:7.2f}"
        ci = lambda c: "        -         " if not c else f"[{c[0] * 100:7.2f},{c[1] * 100:7.2f}]"
        print(f"{r['length']:>4} {f(m['mean_log'])} {ci(m['ci_log'])} {f(m['sd'])[1:]} | {f(aa['mean_log'])} "
              f"{ci(aa['ci_log'])} | {r['blocks']}")
    print("pooled block sd (S8 vs S1) %:", None if res["pooled_block_sd_S8_vs_S1"] is None
          else round(res["pooled_block_sd_S8_vs_S1"] * 100, 3), " AA:", res["pooled_block_sd_AA"] and round(res["pooled_block_sd_AA"] * 100, 3))
    print("A/A CI covers 0:", res["aa_ci_covers_zero"], " S8/S1 CI excludes 0:", res["s8_vs_s1_ci_excludes_zero"])
    print("blocks needed (80% power):", res.get("blocks_needed_80pct_power"))


if __name__ == "__main__":
    main()

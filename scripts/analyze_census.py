#!/usr/bin/env python3
"""Describe WHAT changes between lengths in a G2.5 census - EVALUATOR ONLY.

Reads census_table.json and the per-length work/sNNNN/{stdout.txt,ir_structure.json}
written by scripts/run_census.py and reports, without deciding anything:

  - opt-report: whether the decision multiset (kind, op, applied, message, VL),
    ignoring node names and trip counts, differs between lengths, and which
    records have a trip count equal to the length
  - IR structure: which histogram features vary, and for each the smallest
    period p (x(L) = x(L+p)) or, failing that, whether it is determined by the
    divisibility of L by small integers

  python scripts/analyze_census.py --census-dir census/A_reexport/default/<cpu> --out <summary.json>
"""
import argparse
import collections
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import signature as S  # noqa: E402
from shapeperf.util import git_head, write_json  # noqa: E402


def _determined(xs, key):
    seen = {}
    for L, v in xs.items():
        k = key(L)
        if seen.setdefault(k, v) != v:
            return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--census-dir", required=True)
    ap.add_argument("--max-period", type=int, default=64)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    root = Path(a.census_dir)
    table = json.loads((root / "census_table.json").read_text())
    lengths = table["valid_lengths"]
    work = root / "work"

    # opt-report decisions without names and trip counts
    multisets, eq_len = {}, collections.Counter()
    for L in lengths:
        recs = S.parse_opt_report((work / f"s{L:04d}" / "stdout.txt").read_text(errors="replace"))
        multisets[L] = collections.Counter((r.get("kind"), r.get("op"), r.get("applied"), r.get("message"),
                                            r.get("value")) for r in recs if "unparsed" not in r)
        for r in recs:
            if r.get("trip_count") == L:
                eq_len[(r["kind"], r["op"], r["message"], r["value"])] += 1
    distinct = {tuple(sorted(m.items())) for m in multisets.values()}

    # IR structure histograms
    hist = {}
    for L in lengths:
        f = json.loads((work / f"s{L:04d}" / "ir_structure.json").read_text())["functions"]
        h = {}
        for fn, d in f.items():
            for sec in ("ops", "vectors", "loops"):
                for k, v in d.get(sec, {}).items():
                    h[f"{fn}:{sec}:{k}"] = v
        hist[L] = h
    keys = sorted(set().union(*hist.values()))
    features = []
    for k in keys:
        xs = {L: hist[L].get(k, 0) for L in lengths}
        if len(set(xs.values())) == 1:
            continue
        period = next((p for p in range(1, a.max_period + 1) if _determined(xs, lambda L, p=p: L % p)), None)
        divisors = None
        if period is None:
            for m in range(2, a.max_period + 1):
                if _determined(xs, lambda L, m=m: tuple(L % d == 0 for d in range(2, m + 1))):
                    divisors = m
                    break
        features.append({"feature": k, "min": min(xs.values()), "max": max(xs.values()),
                         "period": period, "determined_by_divisibility_up_to": divisors})

    write_json(a.out, {
        "census_dir": str(root), "harness_commit": git_head(), "lengths": [lengths[0], lengths[-1]],
        "table_verdict": table.get("H1_verdict"), "n_C_sig": len(table["C_sig"]),
        "n_C_ir_structure": len(table["C_ir_structure"]), "n_C_raw": len(table.get("C_raw", [])),
        "alignment": {n: {"units": v["units"], "B_align_size": v["B_align_size"],
                          "n_C_sig_nonalign": len(v["C_sig_nonalign"]),
                          "n_C_ir_nonalign": len(v["C_ir_nonalign"])} for n, v in table["alignment"].items()},
        "opt_report": {"n_distinct_decision_multisets": len(distinct),
                       "n_records_per_length": sorted({sum(m.values()) for m in multisets.values()}),
                       "records_with_trip_count_equal_length": [
                           {"kind": k[0], "op": k[1], "message": k[2], "value": k[3], "n": n // len(lengths)}
                           for k, n in eq_len.most_common()]},
        "ir_structure_varying_features": features,
        "note": "descriptive only; evaluator-side; decides nothing",
    })
    print(json.dumps({"n_distinct_decision_multisets": len(distinct),
                      "n_varying_ir_features": len(features),
                      "periods": collections.Counter(f["period"] for f in features)}, default=str))


if __name__ == "__main__":
    main()

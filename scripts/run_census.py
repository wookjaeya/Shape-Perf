#!/usr/bin/env python3
"""G2.5 signature census - EVALUATOR ONLY (spec §11 G2.5, §13 items 14-15).

Compiles every valid length up to the probe stage, extracts signatures and
counts change points, how many sit on alignment boundaries, and which do not
(C_nonalign). Output goes to census/<model>/<flagset>/<target>/ with owner-only
permissions. Selectors must never read it (tests/test_isolation.py).

  python scripts/run_census.py --model A_reexport --flagset default \
      --target-cpu <cpu> --lengths 41-256 [--units 4 16]

--units given here are ANALYSIS units. The primary B_align must come from
configs/preregistration.json selectors.shape_only_alignment_units; if that is
null the table is labelled 'units not preregistered'.
"""
import argparse
import gzip
import json
import os
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import evaluate as E  # noqa: E402
from shapeperf import toolchain  # noqa: E402
from shapeperf.compile import compile_shape  # noqa: E402
from shapeperf.util import CENSUS_DIR, REPO_ROOT, append_jsonl, git_head, read_json, read_jsonl, write_json  # noqa: E402


def parse_range(txt):
    out = []
    for part in txt.split(","):
        if "-" in part:
            a, b = part.split("-")
            out += list(range(int(a), int(b) + 1))
        else:
            out.append(int(part))
    return sorted(set(out))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="A_reexport")
    ap.add_argument("--flagset", default="default")
    ap.add_argument("--target-cpu", default=os.environ.get("SHAPEPERF_TARGET_CPU"))
    ap.add_argument("--allow-native", action="store_true")
    ap.add_argument("--lengths", required=True, help="e.g. 41-256")
    ap.add_argument("--units", type=int, nargs="*", default=[], help="analysis-only alignment units")
    ap.add_argument("--keep-raw-ir", choices=["all", "changepoints", "none"], default="changepoints")
    ap.add_argument("--timeout", type=float, default=None)
    ap.add_argument("--resume", action="store_true")
    args = ap.parse_args()

    root = CENSUS_DIR / args.model / args.flagset / (args.target_cpu or "unset")
    work = root / "work"
    root.mkdir(parents=True, exist_ok=True)
    os.chmod(CENSUS_DIR, 0o700)
    os.chmod(root, 0o700)
    recs_path = root / "census.jsonl"
    done = {r["padded_length"] for r in read_jsonl(recs_path)} if (args.resume and recs_path.exists()) else set()
    lengths = parse_range(args.lengths)

    for s in lengths:
        if s in done:
            continue
        d = work / f"s{s:04d}"
        rec = compile_shape(args.model, s, args.flagset, args.target_cpu, "probe", d,
                            allow_native=args.allow_native, timeout_s=args.timeout)
        rec["census_unix"] = time.time()
        rec["harness_commit"] = git_head()
        ir = d / "model.onnx.mlir"
        if ir.exists():
            with open(ir, "rb") as fi, gzip.open(d / "model.onnx.mlir.gz", "wb") as fo:
                shutil.copyfileobj(fi, fo)
            ir.unlink()
        append_jsonl(recs_path, rec)
        print(s, rec.get("failure_type") or rec.get("ir_signature"), rec.get("probe_wall_ns"), flush=True)

    recs = {r["padded_length"]: r for r in read_jsonl(recs_path)}
    sig = {s: (r.get("ir_signature") if not r.get("failure_type") else f"FAILED:{r['failure_type']}")
           for s, r in recs.items()}
    # failed lengths keep their place in the sequence as their own value, so a
    # failure boundary is reported at its real position
    fail = {s: f"FAILED:{r['failure_type']}" for s, r in recs.items() if r.get("failure_type")}
    sig_ir = {s: fail.get(s, r.get("ir_structure_signature")) for s, r in recs.items()}
    raw = {s: fail.get(s, r.get("raw_ir_hash")) for s, r in recs.items()}
    c_sig = E.change_points(sig, lengths)
    c_ir = E.change_points(sig_ir, lengths)
    c_raw = E.change_points(raw, lengths)

    prereg = read_json(REPO_ROOT / "configs/preregistration.json")
    pre_units = prereg["selectors"]["shape_only_alignment_units"]
    unit_sets = {}
    if pre_units:
        unit_sets["preregistered"] = pre_units
    for u in args.units:
        unit_sets[f"analysis_u{u}"] = [u]
    align = {}
    for name, units in unit_sets.items():
        b = E.aligned_boundaries(lengths, units)
        align[name] = {"units": units, "B_align_size": len(b),
                       "C_sig_nonalign": sorted(set(c_sig) - set(b)),
                       "C_ir_nonalign": sorted(set(c_ir) - set(b))}

    if args.keep_raw_ir != "all":
        keep = set()
        if args.keep_raw_ir == "changepoints":
            for c in set(c_sig) | set(c_ir):
                keep |= {c, next((x for x in lengths if x > c), c)}
        for s in lengths:
            gz = work / f"s{s:04d}" / "model.onnx.mlir.gz"
            if gz.exists() and s not in keep:
                gz.unlink()

    table = {
        "model": args.model, "flagset": args.flagset, "target_cpu": args.target_cpu,
        **toolchain.compiler_ids(), "lengths": [lengths[0], lengths[-1]], "n_lengths": len(lengths),
        "n_failed": sum(1 for r in recs.values() if r.get("failure_type")),
        "signature_primary": "opt-report", "C_sig": c_sig,
        "C_ir_structure": c_ir, "n_raw_ir_hash_changes": len(c_raw),
        "alignment": align,
        "H1_verdict": ("units not preregistered - no H1 verdict" if not pre_units else
                       ("C_nonalign non-empty (H1 not rejected)" if align["preregistered"]["C_sig_nonalign"]
                        else "C_nonalign EMPTY: H1 rejected -> stop before G4 (spec §13 item 15)")),
        "matmul_path": sorted({r.get("matmul_path") for r in recs.values() if r.get("matmul_path")}) or
                       "not available in probe mode (needs full compile: see results/g0)",
        "probe_wall_ns_total": sum(r.get("probe_wall_ns") or 0 for r in recs.values()),
        "normalization": "shapeperf/signature.py NORMALIZATION_RULES[sig-v1]",
        "access": "evaluator only; selectors are blocked from this directory",
    }
    write_json(root / "census_table.json", table)
    for p in root.rglob("*"):
        os.chmod(p, 0o700 if p.is_dir() else 0o600)
    print(json.dumps({k: table[k] for k in ["n_lengths", "n_failed", "C_sig", "C_ir_structure",
                                            "n_raw_ir_hash_changes", "H1_verdict"]}, indent=1))


if __name__ == "__main__":
    main()

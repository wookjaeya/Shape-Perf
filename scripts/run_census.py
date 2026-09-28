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
from shapeperf import prereg, toolchain  # noqa: E402
from shapeperf.compile import compile_shape  # noqa: E402
from shapeperf.signature import SIG_VERSION  # noqa: E402
from shapeperf.util import CENSUS_DIR, append_jsonl, git_head, read_jsonl, write_json  # noqa: E402


def parse_range(txt):
    out = []
    for part in txt.split(","):
        if "-" in part:
            a, b = part.split("-")
            out += list(range(int(a), int(b) + 1))
        else:
            out.append(int(part))
    return sorted(set(out))


def _latest(path):
    """Last record per length (append-only file; retries are appended)."""
    return {r["padded_length"]: r for r in read_jsonl(path)} if path.exists() else {}


def _incomplete(r):
    """A record that does not give a usable census value for its length
    (including one made under another signature version)."""
    if r.get("failure_type"):
        return False                     # a real compile failure is a value (FAILED:<type>)
    return (r.get("signature_version") != SIG_VERSION
            or str(r.get("ir_signature", "")).startswith("CORRUPT:") or r.get("ir_signature") is None
            or r.get("probe_ir_missing") is not None or r.get("ir_structure_signature") is None
            or r.get("raw_ir_hash") is None)


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
    ap.add_argument("--census-root", default=str(CENSUS_DIR),
                    help="evaluator-only output root (smoke runs use a separate directory)")
    args = ap.parse_args()

    croot = Path(args.census_root)
    root = croot / args.model / args.flagset / (args.target_cpu or "unset")
    work = root / "work"
    root.mkdir(parents=True, exist_ok=True)
    os.chmod(croot, 0o700)
    os.chmod(root, 0o700)
    recs_path = root / "census.jsonl"
    lengths = parse_range(args.lengths)
    if recs_path.exists() and not args.resume:
        raise SystemExit(f"{recs_path} exists: pass --resume to continue it, or use a fresh --census-root")
    # --resume: a length is done only if its last record is usable; corrupt
    # reports and missing probe IR are compiled again (the later record wins)
    done = {s for s, r in _latest(recs_path).items() if not _incomplete(r)} if args.resume else set()

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

    wanted = set(lengths)
    recs = {s: r for s, r in _latest(recs_path).items() if s in wanted}   # only the requested lengths
    missing = [s for s in lengths if s not in recs]
    # failed lengths keep their place as their own value; boundaries touching
    # them are listed separately, and the lowering decision is compared between
    # the nearest successful lengths across a failed run (spec §1.2 H1)
    fail = {s: f"FAILED:{r['failure_type']}" for s, r in recs.items() if r.get("failure_type")}
    corrupt = sorted(s for s, r in recs.items() if str(r.get("ir_signature", "")).startswith("CORRUPT:"))
    ir_missing = sorted(s for s, r in recs.items() if not r.get("failure_type") and _incomplete(r)
                        and s not in corrupt)
    sig = {s: fail.get(s, r.get("ir_signature")) for s, r in recs.items()}
    sig_ir = {s: fail.get(s, r.get("ir_structure_signature")) for s, r in recs.items()}
    raw = {s: fail.get(s, r.get("raw_ir_hash")) for s, r in recs.items()}
    c_sig, fail_b, sp_sig = E.split_failure_boundaries(sig, lengths)
    c_ir, _, sp_ir = E.split_failure_boundaries(sig_ir, lengths)
    c_raw, _, sp_raw = E.split_failure_boundaries(raw, lengths)
    n_ok = len(lengths) - len(missing) - len(fail)

    p, label = prereg.require(["selectors.shape_only_alignment_units"], allow_unfrozen=True)
    pre_units = p["selectors"]["shape_only_alignment_units"]      # [] is a valid preregistered value
    unit_sets = {}
    if pre_units is not None:
        unit_sets["preregistered"] = pre_units
    for u in args.units:
        unit_sets[f"analysis_u{u}"] = [u]
    align = {}
    def nonalign(spans, units):
        """Spans with no multiple of a unit in [a, b]: certainly non-aligned."""
        us = [u for u in units if u > 1]
        return [[a, b] for a, b in spans if not any((b // u) * u >= a for u in us)]

    def unresolved(spans, units):
        """Spans across failed lengths that contain a multiple but also a
        non-aligned boundary: the change may be non-aligned - position unknown."""
        b_align = set(E.aligned_boundaries(lengths, units))
        out = []
        for a, b in spans:
            inner = [x for x in lengths if a <= x < b]
            if len(inner) > 1 and not all(x in b_align for x in inner) and [a, b] not in nonalign([(a, b)], units):
                out.append([a, b])
        return out
    for name, units in unit_sets.items():
        b = E.aligned_boundaries(lengths, units)
        align[name] = {"units": units, "B_align_size": len(b),
                       "C_sig_nonalign": [a for a, _ in nonalign(sp_sig, units)],
                       "C_sig_nonalign_spans": nonalign(sp_sig, units),
                       "C_sig_unresolved_spans": unresolved(sp_sig, units),
                       "C_ir_nonalign": [a for a, _ in nonalign(sp_ir, units)]}

    incomplete = bool(missing or corrupt or ir_missing or n_ok < 2)
    keep = set(lengths)
    if args.keep_raw_ir == "changepoints":
        keep = set()
        for a, b in sp_sig + sp_ir:
            keep |= {a, b}
        for c in fail_b:
            keep |= {c, next((x for x in lengths if x > c), c)}
    elif args.keep_raw_ir == "none":
        keep = set()
    # while the census is incomplete the change points are not final: delete nothing yet
    if args.keep_raw_ir != "all" and not incomplete:
        for s in lengths:
            gz = work / f"s{s:04d}" / "model.onnx.mlir.gz"
            if gz.exists() and s not in keep:
                gz.unlink()
    raw_missing = sorted(s for s in keep if s in recs and not recs[s].get("failure_type")
                         and not (work / f"s{s:04d}" / "model.onnx.mlir.gz").exists())
    versions = sorted({str(r.get("signature_version")) for r in recs.values() if not r.get("failure_type")})

    if incomplete:
        verdict = (f"INCOMPLETE census (missing {missing[:5]}, corrupt reports {corrupt[:5]}, "
                   f"probe IR missing {ir_missing[:5]}, successful lengths {n_ok}) - no H1 verdict")
    elif pre_units is None:
        verdict = "units not preregistered - no H1 verdict"
    elif align["preregistered"]["C_sig_nonalign"]:
        verdict = "C_nonalign non-empty (H1 not rejected)"
    elif align["preregistered"]["C_sig_unresolved_spans"]:
        verdict = (f"H1 undetermined: changes across failed lengths "
                   f"{align['preregistered']['C_sig_unresolved_spans'][:5]} may be non-aligned")
    else:
        verdict = "C_nonalign EMPTY: H1 rejected -> stop before G4 (spec §13 item 15)"
    if fail and not verdict.startswith(("INCOMPLETE", "units")):
        verdict += f" [{len(fail)} failed lengths: changes hidden inside a failed run are not observable]"
    table = {
        "model": args.model, "flagset": args.flagset, "target_cpu": args.target_cpu,
        **toolchain.compiler_ids(), "lengths": [lengths[0], lengths[-1]], "valid_lengths": lengths,
        "n_lengths": len(lengths), "missing_lengths": missing, "corrupt_reports": corrupt,
        "probe_ir_missing": ir_missing, "signature_versions": versions,
        "raw_ir_kept": args.keep_raw_ir,
        "raw_ir_missing_for_changepoints": raw_missing if args.keep_raw_ir != "none" else [],
        "n_failed": len(fail), "failed_lengths": sorted(fail),
        "failure_boundaries": fail_b,
        "signature_primary": "opt-report", "C_sig": c_sig, "C_sig_spans": [list(x) for x in sp_sig],
        "C_ir_structure": c_ir, "C_ir_structure_spans": [list(x) for x in sp_ir],
        "C_raw": c_raw, "C_raw_spans": [list(x) for x in sp_raw], "n_raw_ir_hash_changes": len(c_raw),
        "span_note": "a span [a, b] with b > next(a) is a decision change across failed lengths",
        "alignment": align, "preregistration": label,
        "H1_verdict": verdict,
        "matmul_path": sorted({r.get("matmul_path") for r in recs.values() if r.get("matmul_path")}) or
                       "not available in probe mode (needs full compile: see results/g0)",
        "probe_wall_ns_total": sum(r.get("probe_wall_ns") or 0 for r in recs.values()),
        "normalization": "shapeperf/signature.py NORMALIZATION_RULES[" +
                         ",".join(sorted({str(r.get("signature_version")) for r in recs.values()})) + "]",
        "access": "evaluator only; selectors are blocked from this directory",
    }
    write_json(root / "census_table.json", table)
    for p in root.rglob("*"):
        os.chmod(p, 0o700 if p.is_dir() else 0o600)
    print(json.dumps({k: table[k] for k in ["n_lengths", "n_failed", "C_sig", "C_ir_structure",
                                            "n_raw_ir_hash_changes", "H1_verdict"]}, indent=1))


if __name__ == "__main__":
    main()

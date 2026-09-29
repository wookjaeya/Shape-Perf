#!/usr/bin/env python3
"""Build both arms of the primary contrast for given lengths (design amendment v3, G1).

  S8  the pinned compiler (transpose unroll cap 8)
  S1  the same compiler with ONE constant patched (patches/transpose_unroll_cap1.patch): cap 1

Same model file, flags, target and --shapeInformation for both; only the compiler binary differs.
For each length and arm: a full compile (the .so that is timed) and a probe compile (MLIR output
+ opt-report, elided constants). Per length the script checks the PREDICTIONS of the hypothesis
against the IR: with u = getNoLeftoverUnrollFactor(L, 8),
  S8 has 13 unrolled transpose loops (`0 to L step u`) if u > 1, else 0;   S1 has 0;
  for u == 1 the two arms must be identical (IR text and .so bytes): the negative control.

  python scripts/build_shape_pair.py --lengths-file results/v3/g1/g1_lengths.json \
      --out /home/user/work/v3/pairs [--only 41 49] [--jobs 3] [--probe-only]
"""
import argparse
import json
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import lowering_diff as LD  # noqa: E402
from shapeperf import provenance  # noqa: E402
from shapeperf.compile import compile_shape  # noqa: E402
from shapeperf.util import REPO_ROOT, write_json  # noqa: E402

DEFAULT_ARMS = {"S8": "/home/user/work/variants/orig", "S1": "/home/user/work/variants/cap1"}
N_TRANSPOSE_LOOPS = 13          # 12 layers (K transpose) + the last output transpose (census 2026-09-28)


def _compile(job):
    arm, build, length, mode, out_dir, target = job
    rec = compile_shape("A_reexport", length, "default", target, mode, out_dir, compiler_build=build,
                        keep_ir=(mode == "probe"))
    rec["arm"] = arm
    return rec


def check_pair(length, probe_dirs):
    """Compare the probe IR of the two arms with the hypothesis' predictions."""
    u = LD.no_leftover_unroll(length)
    text = {arm: (Path(d) / "model.onnx.mlir").read_text(errors="replace") for arm, d in probe_dirs.items()}
    n8 = LD.count_unrolled_loops(text["S8"], length, u) if u > 1 else 0
    n1 = LD.count_unrolled_loops(text["S1"], length, u) if u > 1 else 0
    diff = LD.diff_hunks(text["S8"], text["S1"])
    expected_identical = (u == 1)
    return {"length": length, "u": u,
            "unrolled_loops": {"S8": n8, "S1": n1},
            "predicted_unrolled_loops": {"S8": N_TRANSPOSE_LOOPS if u > 1 else 0, "S1": 0},
            "ir_identical": text["S8"] == text["S1"], "predicted_ir_identical": expected_identical,
            "normalized_diff": diff,
            "prediction_holds": (n8 == (N_TRANSPOSE_LOOPS if u > 1 else 0) and n1 == 0
                                 and (text["S8"] == text["S1"]) == expected_identical)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lengths-file", default=None, help="JSON with a 'lengths' list")
    ap.add_argument("--only", type=int, nargs="*", default=None, help="subset of the lengths")
    ap.add_argument("--lengths", type=int, nargs="*", default=None)
    ap.add_argument("--out", required=True)
    ap.add_argument("--target-cpu", default="emeraldrapids")
    ap.add_argument("--arms", nargs="*", default=[f"{k}={v}" for k, v in DEFAULT_ARMS.items()])
    ap.add_argument("--jobs", type=int, default=3, help="parallel compiles (each ~1 core, ~2.8 GB RSS)")
    ap.add_argument("--probe-only", action="store_true", help="skip the (slow) full compiles")
    args = ap.parse_args()

    lengths = args.lengths or json.loads(Path(args.lengths_file).read_text())["lengths"]
    if args.only:
        lengths = [L for L in lengths if L in set(args.only)]
    arms = dict(a.split("=", 1) for a in args.arms)
    if set(arms) != {"S8", "S1"}:
        raise SystemExit("exactly the arms S8=<build> and S1=<build> are required")
    out = Path(args.out)
    prov = provenance.freeze(out / "provenance.json", compiler_builds=list(arms.values()),
                             extra={"role": "g1-pair-build", "lengths": lengths})
    print("provenance", prov["provenance_id"], "dirty" if prov["source"]["dirty"] else "clean", flush=True)

    modes = ["probe"] if args.probe_only else ["probe", "full"]
    jobs = [(arm, build, L, mode, str(out / f"L{L:04d}" / f"{arm}_{mode}"), args.target_cpu)
            for L in lengths for mode in modes for arm, build in arms.items()]
    results = {}
    with ProcessPoolExecutor(max_workers=args.jobs) as ex:
        for job, rec in zip(jobs, ex.map(_compile, jobs)):
            arm, _b, L, mode = job[:4]
            rec["provenance_id"] = prov["provenance_id"]
            results[(L, arm, mode)] = rec
            print(f"L={L} {arm} {mode}: {rec.get('failure_type') or 'ok'} "
                  f"{(rec.get('compile_wall_ns') or rec.get('probe_wall_ns') or 0) / 1e9:.1f}s", flush=True)

    g0_path = REPO_ROOT / "results/g0/g0_verification.json"
    g0 = json.loads(g0_path.read_text())["full_compile"]["artifact_hash"] if g0_path.exists() else None
    summary = []
    for L in lengths:
        entry = {"length": L, "provenance_id": prov["provenance_id"]}
        probes = {arm: out / f"L{L:04d}" / f"{arm}_probe" for arm in arms}
        entry["probe_failures"] = {a: results[(L, a, "probe")].get("failure_type") for a in arms}
        if not any(entry["probe_failures"].values()):
            entry["ir_check"] = check_pair(L, probes)
            entry["probe_signatures"] = {a: {"opt_report": results[(L, a, "probe")]["ir_signature"],
                                             "ir_structure": results[(L, a, "probe")]["ir_structure_signature"],
                                             "raw_ir_hash": results[(L, a, "probe")]["raw_ir_hash"]}
                                         for a in arms}
        if not args.probe_only:
            entry["full"] = {a: {k: results[(L, a, "full")].get(k) for k in
                                 ("failure_type", "artifact_path", "artifact_hash", "artifact_bytes",
                                  "compile_wall_ns", "peak_rss_bytes", "compiler_variant",
                                  "compiler_bin_sha256", "compile_flags", "command", "matmul_path")}
                             for a in arms}
            fa = entry["full"]
            if not any(fa[a]["failure_type"] for a in arms):
                entry["so_identical"] = fa["S8"]["artifact_hash"] == fa["S1"]["artifact_hash"]
                entry["predicted_so_identical"] = (LD.no_leftover_unroll(L) == 1)
            if L == 128 and g0:            # environment restoration: same bytes as the v2 G0 artifact
                entry["matches_v2_g0_artifact_hash"] = (fa["S8"]["artifact_hash"] == g0)
        write_json(out / f"L{L:04d}" / "pair.json", entry)
        summary.append(entry)
        chk = entry.get("ir_check")
        print(f"L={L} u={LD.no_leftover_unroll(L)} prediction_holds={chk and chk['prediction_holds']} "
              f"loops S8/S1={chk and chk['unrolled_loops']} so_identical={entry.get('so_identical')}", flush=True)

    write_json(out / f"summary_{'_'.join(map(str, lengths[:3]))}_{len(lengths)}.json", summary)


if __name__ == "__main__":
    main()

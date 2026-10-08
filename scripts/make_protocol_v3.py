#!/usr/bin/env python3
"""Write protocol_v3.json: the DRAFT protocol of design amendment v3 (docs/research_plan_v3.md §10 G3).

Everything decided by the design or already fixed by a recorded artifact is filled in; everything the
pilot or the budget has to decide is null. Nothing is preregistered: `status` says so, and the file
carries no `frozen` hash. Fixing it later is a separate, dated step.

  python scripts/make_protocol_v3.py [--out protocol_v3.json]
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import toolchain  # noqa: E402
from shapeperf.compile import model_def  # noqa: E402
from shapeperf.util import REPO_ROOT, read_json, sha256_file, write_json  # noqa: E402

V2_MAIN_COMMIT = "28382a9deaa502db95098d4140e92b809f52d3df"


def build():
    model = model_def("A_reexport")
    flags = read_json(REPO_ROOT / "configs/compile_flags.json")["flagsets"]["default"]
    lengths = read_json(REPO_ROOT / "results/v3/g1/g1_lengths.json")
    arms = {"S8": "/home/user/work/variants/orig", "S1": "/home/user/work/variants/cap1"}
    pins = toolchain.toolchain_pins()
    return {
        "status": "draft - pilot only; nothing is preregistered (null = to be decided by the pilot/budget)",
        "basis": {"plan": "docs/research_plan_v3.md", "plan_sha256": sha256_file(REPO_ROOT / "docs/research_plan_v3.md"),
                  "design_amendment": "design_amendment.md", "v2_state": {"main_commit": V2_MAIN_COMMIT}},
        "toolchain": {"onnx_mlir_sha": pins["ONNX_MLIR_SHA"], "llvm_sha": pins["LLVM_SHA"],
                      "compilers": {arm: toolchain.compiler_identity(b) for arm, b in arms.items()},
                      "patch": {"file": "patches/transpose_unroll_cap1.patch",
                                "sha256": sha256_file(REPO_ROOT / "patches/transpose_unroll_cap1.patch"),
                                "changes": "Transpose.cpp scalarTransposeOverOutputs: unroll cap 8 -> 1 (one constant)"},
                      "revert_rebuild_hash_equals_original": True},
        "contrast": {"primary": "S8 (pinned compiler) vs S1 (cap 1), same length, same input, same model file",
                     "effect": "a = log T_S8 - log T_S1 per block (positive: original policy slower)",
                     "auxiliary": "D8/D1 (static vs dynamic L): not built in this run",
                     "negative_controls": ["lengths with u(L)=1 (S8 and S1 identical)",
                                           "Q/V-type transposes (blockTranspose path, patch unreachable)"],
                     "aa_control": "byte-identical copy of the S8 artifact as a third arm"},
        "model": {"key": "A_reexport", "path": model["path"], "sha256": sha256_file(model["abs_path"]),
                  "note": "re-export of the bertsquad-12 weights with a variable sequence axis; not the Zoo file"},
        "compile": {"flagset": "default", "flags": flags["flags"], "target_cpu": "emeraldrapids (dev container)",
                    "shape_information": "0:1,1:1xL,2:1xL,3:1xL", "optimization_report": flags.get("report")},
        "lengths": {"rule": lengths["rule"], "g1_pilot_lengths": lengths["lengths"], "u": lengths["u"],
                    "full_range_for_G4": [41, 256], "n_full_range": 216},
        "inputs": {"controlled": "anchor feature 1656 (natural length 41), right-padded to L",
                   "natural": "first feature by (qas_id, window_index) among features of natural length L"},
        "measurement": {"process_statistic": "arithmetic mean of iterations after warmup (primary); median and min secondary",
                        "threads": 1, "affinity": "one fixed CPU", "fresh_process_per_unit": True,
                        "timing_boundary": "perf_counter_ns around session.run(inputs), outputs kept alive to next call",
                        "randomization": "length order, and arm x process order within each block, seeded",
                        "warmup_iterations": None, "timed_iterations": None, "processes_per_arm_per_block": None,
                        "blocks": None, "vm_allocations": None, "cpu_class": None},
        "inference": {"unit": "block within one VM allocation; allocation means when >= 2 allocations",
                      "family_wise_alpha": 0.05, "alpha_source": "researcher's error-control choice (plan §11.3), not a law",
                      "multiplicity": "Holm over the fixed family of confirmed contrasts", "direction": "fixed in discovery, one-sided in confirmation",
                      "min_practical_effect": None, "practical_effect_source": None},
        "correctness": {"intervention_gate": "S8 and S1 outputs bitwise identical (data movement only)",
                        "vs_reference": "ORT: reported descriptively; tolerance not set (no post-hoc tolerance)",
                        "numeric_tolerance": None, "numeric_tolerance_source": None,
                        "semantic_gate": "final answer identical to ORT on every feature (researcher's choice, plan §11.5)"},
        "budget": {"max_run_cost": None, "stopping_rule": None, "held_out_model": None, "held_out_revision": None},
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(REPO_ROOT / "protocol_v3.json"))
    a = ap.parse_args()
    write_json(a.out, build())
    print("wrote", a.out)


if __name__ == "__main__":
    main()

"""Paired analysis, input selection, provenance and compiler identity (design amendment v3)."""
import math
import stat

import numpy as np

from analysis.subgraph_effects import N_K_TRANSPOSES, analyse_kind, model_latency_ns
from shapeperf import paired, provenance, toolchain


def _rec(length, block, arm, lat, kind=None):
    r = {"padded_length": length, "block_id": block, "flagset": arm, "latency_ns": list(lat), "failure_type": None,
         "process_wall_ns": 1e9}
    if kind:
        r.update(model_key=f"subgraph:{kind}", exact_permutation=True)
    return r


def test_natural_feature_is_first_by_question_and_window():
    feat = {"valid_length": np.array([50, 60, 60, 60, 70]),
            "qas_id": np.array(["b", "z", "a", "a", "c"]), "window_index": np.array([0, 0, 1, 0, 0])}
    assert paired.natural_feature_index(feat, 60) == 3            # ('a', 0) before ('a', 1) before ('z', 0)
    assert paired.natural_feature_index(feat, 99) is None


def test_paired_log_ratio_recovers_a_known_effect_and_its_interval():
    rng = np.random.default_rng(0)
    recs = []
    for b in range(12):
        base = 100.0 * math.exp(rng.normal(0, 0.05))                    # block level drift cancels in the pair
        for arm, f in (("S8", 1.03), ("S1", 1.0), ("AA", 1.03)):
            for _ in range(2):
                recs.append(_rec(64, f"b{b}", arm, base * f * np.exp(rng.normal(0, 0.005, 10))))
    t = paired.process_means(recs)
    a = paired.effect_summary(paired.paired_log_ratios(t, 64, "S8", "S1"))
    assert abs(a["mean_log"] - math.log(1.03)) < 0.005 and a["n_blocks"] == 12
    assert a["ci_log"][0] < math.log(1.03) < a["ci_log"][1] and a["ci_log"][0] > 0
    aa = paired.effect_summary(paired.paired_log_ratios(t, 64, "S8", "AA"))
    assert aa["ci_log"][0] < 0 < aa["ci_log"][1]                       # byte-identical arms: no difference
    assert paired.effect_summary([0.1])["ci_log"] is None                # one block gives no interval


def test_robust_statistics_ignore_a_burst():
    lat = [100.0] * 9 + [1000.0]
    t_mean = paired.process_means([_rec(1, "b", "S8", lat)], 0, "mean")
    t_med = paired.process_means([_rec(1, "b", "S8", lat)], 0, "median")
    t_min = paired.process_means([_rec(1, "b", "S8", lat)], 0, "min")
    assert t_mean[(1, "b")]["S8"] == [190.0] and t_med[(1, "b")]["S8"] == [100.0] and t_min[(1, "b")]["S8"] == [100.0]


def test_required_blocks_matches_the_z_formula():
    assert paired.required_blocks(0.03, 0.01) == math.ceil(((1.959964 + 0.841621) * 3) ** 2)
    assert paired.required_blocks(0.03, 0.001) > 90 * paired.required_blocks(0.03, 0.01)


def test_compare_outputs_bitwise_and_valid_positions_only():
    a = {"s": np.array([[1.0, 2.0, 3.0, 9.0]], np.float32), "e": np.array([[3.0, 1.0, 0.0, 5.0]], np.float32)}
    b = {"s": a["s"].copy(), "e": a["e"].copy()}
    assert paired.compare_outputs(a, b, 3, "s", "e")["bitwise_identical"]
    b["s"][0, 3] = 100.0                                                # a padding position differs
    c = paired.compare_outputs(a, b, 3, "s", "e")
    assert not c["bitwise_identical"] and c["start_max_abs_valid"] == 0.0 and c["start_argmax_equal"]
    b["s"][0, 1] = 5.0                                                  # a valid position differs
    assert paired.compare_outputs(a, b, 3, "s", "e")["start_max_abs_valid"] == 3.0


def test_subgraph_analysis_gives_the_model_level_bound():
    recs = []
    for b in range(6):
        for arm, t in (("S8", 30_000.0), ("S1", 20_000.0), ("AA", 30_000.0)):
            recs.append(_rec(64, f"k{b}", arm, [t * (1 + 0.01 * i) for i in range(5)], kind="K"))
    g1 = [_rec(64, "m", "S1", [200e6, 210e6])]
    tm = model_latency_ns(g1)
    res = analyse_kind(recs, "K", tm)
    row = res["lengths"][0]
    assert res["all_outputs_exact_permutation"] and res["n_failed"] == 0
    assert abs(row["abs_S8_minus_S1"]["mean_ns"] - 10_200.0) < 1
    assert abs(row["model_level_bound"]["relative"] - N_K_TRANSPOSES * 10_200.0 / 205e6) < 1e-9


def test_compiler_identity_distinguishes_binaries_by_hash(tmp_path):
    def variant(name, body):
        b = tmp_path / name / "bin"
        b.mkdir(parents=True)
        p = b / "onnx-mlir"
        p.write_text(f"#!/bin/sh\necho {body}\n")
        p.chmod(p.stat().st_mode | stat.S_IEXEC)
        return str(tmp_path / name)
    a, b = variant("orig", "one"), variant("cap1", "two")
    ia, ib = toolchain.compiler_identity(a), toolchain.compiler_identity(b)
    assert ia["compiler_variant"] == "orig" and ib["compiler_variant"] == "cap1"
    assert ia["compiler_bin_sha256"] != ib["compiler_bin_sha256"] and len(ia["compiler_bin_sha256"]) == 64
    assert toolchain.compiler_identity(a) == ia                          # stable
    assert toolchain.onnx_mlir_version(a) == "one" and toolchain.onnx_mlir_version(b) == "two"


def test_provenance_id_is_stable_and_covers_source_dependencies_and_compilers(tmp_path):
    b = tmp_path / "v" / "bin"
    b.mkdir(parents=True)
    exe = b / "onnx-mlir"
    exe.write_text("#!/bin/sh\necho x\n")
    exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
    out = tmp_path / "prov.json"
    s1 = provenance.freeze(out, compiler_builds=[str(tmp_path / "v")], extra={"role": "t"})
    s2 = provenance.freeze(None, compiler_builds=[str(tmp_path / "v")], extra={"role": "t"})
    assert s1["provenance_id"] == s2["provenance_id"] and len(s1["provenance_id"]) == 16 and out.exists()
    assert {"git_head", "dirty", "diff_sha256", "untracked_sha256"} <= set(s1["source"])
    assert s1["dependencies"]["packages_sha256"] and s1["compilers"][0]["compiler_bin_sha256"] != "unavailable"
    s3 = provenance.freeze(None, compiler_builds=[str(tmp_path / "v")], extra={"role": "other"})
    assert s3["provenance_id"] != s1["provenance_id"]


def test_git_head_is_frozen_per_process():
    from shapeperf.util import git_head
    assert git_head() is git_head()                                      # cached: the same object, one computation

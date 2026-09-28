#!/usr/bin/env python3
"""Evaluator CLI (spec §12 evaluate.py). Isolated from the selectors: only this
side reads census/ and the dense G4 data.

  answer-table    : continuous indicators A(s), per-shape summary, answer table A
                    (spec §9.1-9.3, §10.3) over an EXPLICIT valid length set
  alt-indicators  : R(s), Q(s) for a same-shape alternative configuration (§9.1)
  census          : Prec_sig / Rec_sig / non-aligned contribution (§9.4)
  compare         : policy comparison on the recorded dense data (§11.1)

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
from shapeperf.broker import QueryBroker, SelectorSpec  # noqa: E402
from shapeperf.selectors import REGISTRY  # noqa: E402
from shapeperf.util import REPO_ROOT, append_jsonl, git_head, read_json, read_jsonl, write_json  # noqa: E402

DEV_DEFAULTS = {"process_statistic": "mean", "direction": "two-sided", "min_blocks_per_pair": 2,
                "min_allocations_per_pair": 1, "confidence_level": 0.95}


def _load(paths):
    return [r for p in paths for r in read_jsonl(p)]


def parse_lengths(txt):
    out = set()
    for part in txt.split(","):
        if "-" in part:
            a, b = part.split("-")
            out |= set(range(int(a), int(b) + 1))
        elif part:
            out.add(int(part))
    return sorted(out)


def pick(pre_value, cli_value, label, name):
    """Preregistered value wins ([] and 0 are values); a CLI fallback is allowed
    only for unfrozen development runs."""
    if pre_value is not None:
        if cli_value is not None and cli_value != pre_value:
            raise SystemExit(f"{name}: CLI value {cli_value!r} conflicts with preregistered {pre_value!r}")
        return pre_value
    if cli_value is not None and label.startswith("frozen"):
        raise SystemExit(f"{name}: not preregistered; CLI fallbacks are refused for frozen runs")
    return cli_value


def dev_value(section, key, label):
    """Preregistered value, or the labelled dev default for unfrozen runs."""
    v = section.get(key)
    if v is not None:
        return v
    if label.startswith("frozen"):
        raise SystemExit(f"{key} is not preregistered")
    return DEV_DEFAULTS[key]


def valid_from_manifests(paths, cli):
    """Explicit valid set: G4 manifest length ranges (all planned lengths,
    including ones that later failed) or --valid."""
    sets = []
    for p in paths or []:
        a, b = read_json(p)["lengths"]
        sets.append(list(range(a, b + 1)))
    if cli:
        sets.append(parse_lengths(cli))
    if not sets:
        raise SystemExit("the valid length set must be explicit: pass --manifest (G4 manifest.json) or --valid a-b")
    if any(s != sets[0] for s in sets):
        raise SystemExit("manifests / --valid disagree on the valid length set")
    return sets[0]


def cmd_answer_table(a):
    p, label = prereg.require(["events.delta_min_effect", "events.alpha", "events.confidence_level",
                               "events.direction", "events.min_blocks_per_pair",
                               "events.min_allocations_per_pair", "events.confirmation_requires_new_allocation",
                               "measurement.process_statistic"], a.allow_unfrozen)
    ev = p["events"]
    stat = dev_value(p["measurement"], "process_statistic", label)
    delta = pick(ev["delta_min_effect"], a.delta, label, "delta")
    alpha = pick(ev["alpha"], a.alpha, label, "alpha")
    if delta is None or alpha is None:
        raise SystemExit("delta and alpha are required (preregistration, or --delta/--alpha for dev runs)")
    kw = {"direction": dev_value(ev, "direction", label),
          "min_blocks": dev_value(ev, "min_blocks_per_pair", label),
          "min_allocations": dev_value(ev, "min_allocations_per_pair", label)}
    conf_level = dev_value(ev, "confidence_level", label)
    valid = valid_from_manifests(a.manifest, a.valid)
    disc_recs, conf_recs = _load(a.discovery), _load(a.confirmation)
    indep = E.check_independent(disc_recs, conf_recs, bool(ev.get("confirmation_requires_new_allocation")))
    disc = E.block_table(disc_recs, stat, a.flagset)
    conf = E.block_table(conf_recs, stat, a.flagset)
    flop_cfg = None
    meta = REPO_ROOT / "results/g2/bertsquad-12-reexport-dynseq.onnx.export_meta.json"
    if meta.exists():
        flop_cfg = read_json(meta)["bert_config_derived_from_artifact"]
    freq = {}
    cat = REPO_ROOT / "data/features/A/catalog.json"
    if cat.exists():
        freq = read_json(cat)["S_observed_freq"]
    deltas = [delta] + [d for d in (ev.get("sensitivity_deltas") or []) if d != delta]
    out = {"preregistration": label, "harness_commit": git_head(), "process_statistic": stat,
           "flagset": a.flagset, "valid_lengths": valid, "test": "t-based minimum-effect test", **kw,
           "alpha": alpha, "confidence_level": conf_level, "independence": indep,
           "n_records": {"discovery": len(disc_recs), "confirmation": len(conf_recs)},
           "failures": {"discovery": sum(1 for r in disc_recs if r.get("failure_type")),
                        "confirmation": sum(1 for r in conf_recs if r.get("failure_type"))},
           "per_shape": {"discovery": E.per_shape_summary(disc_recs, stat),
                         "confirmation": E.per_shape_summary(conf_recs, stat)},
           "by_delta": {}}
    for d in deltas:
        tab = E.answer_table_a(valid, disc, conf, d, alpha, conf_level, flop_cfg=flop_cfg,
                               reps=ev.get("bootstrap_reps_descriptive_ci") or 0, seed=0, **kw)
        tab["natural_length_weighted_impact"] = E.natural_weighted_impact(tab["detail"], freq)
        out["by_delta"][str(d)] = {"answer_table_A": tab}
    write_json(a.out, out)
    primary = out["by_delta"][str(delta)]["answer_table_A"]
    print(json.dumps({"preregistration": label, "E_A": primary["events"], "candidates": primary["candidates"],
                      "missing_pairs": primary["n_missing_pairs"]}))


def cmd_alt_indicators(a):
    p, label = prereg.require(["measurement.process_statistic", "events.confidence_level"], a.allow_unfrozen)
    stat = dev_value(p["measurement"], "process_statistic", label)
    recs = _load(a.measurements)
    valid = valid_from_manifests(a.manifest, a.valid)
    res = E.alternative_indicators(E.block_table(recs, stat, a.default_flagset),
                                   E.block_table(recs, stat, a.alternative_flagset), valid,
                                   dev_value(p["events"], "confidence_level", label))
    res.update(preregistration=label, default=a.default_flagset, alternative=a.alternative_flagset)
    write_json(a.out, res)
    print("wrote", a.out)


def _answer_table(path, label):
    at = read_json(path)
    if label.startswith("frozen") and at.get("preregistration") != label:
        raise SystemExit(f"answer table was produced under {at.get('preregistration')!r}, not {label!r}")
    return at


def cmd_census(a):
    p, label = prereg.require(["events.match_tolerance_lengths", "selectors.shape_only_alignment_units"],
                              a.allow_unfrozen)
    table = read_json(a.census_table)
    at = _answer_table(a.answer_table, label)
    units = pick(p["selectors"]["shape_only_alignment_units"], a.units, label, "alignment units")
    tol = pick(p["events"]["match_tolerance_lengths"], a.match_tolerance, label, "match tolerance")
    if units is None or tol is None:
        raise SystemExit("alignment units and match tolerance are required (preregistration or CLI for dev runs)")
    valid = at["valid_lengths"]
    if table.get("valid_lengths") and table["valid_lengths"] != valid:
        raise SystemExit("census and answer table cover different valid length sets")
    b_align = E.aligned_boundaries(valid, units)
    res = {"preregistration": label, "units": units, "match_tolerance": tol, "by_delta": {}}
    for d, blk in at["by_delta"].items():
        e_a = blk["answer_table_A"]["events"]
        res["by_delta"][d] = {
            "opt_report": E.census_metrics(table["C_sig"], b_align, e_a, tol),
            "ir_structure": E.census_metrics(table["C_ir_structure"], b_align, e_a, tol),
            "raw_ir_hash (ablation 11.2-3)": E.census_metrics(table.get("C_raw", []), b_align, e_a, tol),
            "failure_boundaries_excluded": table.get("failure_boundaries", []),
        }
    write_json(a.out, res)
    first = next(iter(res["by_delta"].values()))
    print(json.dumps(first["opt_report"], indent=1))


def dense_table(valid, compile_recs, meas_recs, census_recs, stat, report_ns):
    """Per valid length: recorded costs + recorded processes (for ReplayBackend).
    Lengths without a compile record are failures ('not_compiled')."""
    dense = {s: {"failure_type": "not_compiled", "compile_ns": 0, "processes": []} for s in valid}
    for c in compile_recs:
        s = c["padded_length"]
        if s not in dense or c.get("flagset", "default") != "default":
            continue
        dense[s] = {"compile_ns": c.get("compile_wall_ns") or 0, "extract_ns": c.get("feature_extract_wall_ns") or 0,
                    "verify_ns": c.get("verify_wall_ns") or 0, "signature": c.get("ir_signature"),
                    "report_ns": report_ns, "failure_type": c.get("failure_type"), "processes": []}
    for r in meas_recs:
        s = r["padded_length"]
        if s in dense and not r.get("failure_type") and r.get("flagset", "default") == "default":
            dense[s]["processes"].append({"median_ns": E.process_stat(r["latency_ns"], stat),
                                          "warmup_ns": r["warmup_wall_ns"], "measure_ns": r["measurement_wall_ns"]})
    probe_cov = set()
    for r in census_recs:
        s = r["padded_length"]
        if s in dense:
            probe_cov.add(s)
            dense[s]["probe_ns"] = r.get("probe_wall_ns") or 0
            dense[s]["probe_extract_ns"] = r.get("feature_extract_wall_ns") or 0
            dense[s]["probe_signature"] = r.get("ir_signature")
            dense[s]["probe_signature_ir"] = r.get("ir_structure_signature")
            dense[s]["probe_signature_raw"] = r.get("raw_ir_hash")
            dense[s]["probe_failure_type"] = r.get("failure_type")
    for s, d in dense.items():
        if not d["failure_type"] and not d["processes"]:
            d["failure_type"] = "no_measurement"
    return dense, sorted(set(valid) - probe_cov)


class _SigKeyReplay(ReplayBackend):
    """Ablation 11.2-3: Compile-probe driven by another probe signature field."""

    def __init__(self, dense, seed, key, **kw):
        super().__init__(dense, seed, **kw)
        self.key = key

    def probe(self, s):
        r = super().probe(s)
        if not r.get("failure_type"):
            r["signature"] = self.dense[s].get(self.key)
        return r


def cmd_compare(a):
    p, label = prereg.require(["selectors.random_seeds", "selectors.shape_only_alignment_units",
                               "selectors.compile_probe_budget_fraction", "selectors.budgets_ns",
                               "selectors.report_overhead_ns", "events.delta_min_effect",
                               "measurement.process_statistic"], a.allow_unfrozen)
    sp = p["selectors"]
    stat = dev_value(p["measurement"], "process_statistic", label)
    at = _answer_table(a.answer_table, label)
    valid = at["valid_lengths"]
    seeds = pick(sp["random_seeds"], a.seeds, label, "seeds")
    budgets = pick(sp["budgets_ns"], a.budgets, label, "budgets")
    units = pick(sp["shape_only_alignment_units"], a.units, label, "alignment units")
    frac = pick(sp["compile_probe_budget_fraction"], a.probe_fraction, label, "probe budget fraction")
    report_ns = pick(sp["report_overhead_ns"], a.report_overhead_ns, label, "report overhead")
    for name, v in (("seeds", seeds), ("budgets", budgets)):
        if not v:
            raise SystemExit(f"{name} must be a non-empty list")
    if units is None or frac is None:
        raise SystemExit("alignment units and probe budget fraction are required")
    report_ns = int(report_ns or 0)
    dense, probe_missing = dense_table(valid, _load(a.compile), _load(a.measurements),
                                       _load(a.census) if a.census else [], stat, report_ns)
    conf_cost = {s: float(np.mean([pp["measure_ns"] + pp["warmup_ns"] for pp in d["processes"]]))
                 if d["processes"] else 0.0 for s, d in dense.items()}
    variants = [(n, {}, None) for n in REGISTRY]
    variants += [("compile_guided", {"align_only_units": units}, "align_only")]
    probe_variants = [("compile_probe", {"hybrid_uniform": True}, "hybrid"),
                      ("compile_probe", {}, "ir-structure"), ("compile_probe", {}, "raw-ir")]
    skipped = []
    if probe_missing:
        why = f"no probe data for {len(probe_missing)} valid lengths (e.g. {probe_missing[:5]})"
        skipped = [{"variant": f"{n}[{t}]" if t else n, "reason": why}
                   for n, _, t in variants + probe_variants if n == "compile_probe"]
        variants = [v for v in variants if v[0] != "compile_probe"]
    else:
        variants += probe_variants
    params = {"shape_only": {"units": units}, "compile_probe": {"budget_fraction": frac}}
    runs_path = Path(a.out).with_suffix(".runs.jsonl")
    if runs_path.exists():
        runs_path.unlink()
    out = {"preregistration": label, "harness_commit": git_head(), "valid_lengths": valid,
           "skipped_variants": skipped, "seeds": seeds, "budgets_ns": budgets,
           "report_overhead_ns": report_ns, "runs_file": str(runs_path), "by_delta": {},
           "note": "ReplayBackend charges recorded real costs; common vs policy-extra costs are split per entry"}
    iso_levels = set()
    for dkey, blk in at["by_delta"].items():
        events = blk["answer_table_A"]["events"]
        confirmed = set(events)
        thr = float(np.log1p(float(dkey)))
        results = {}
        for name, extra, tag in variants:
            key_base = name + (f"[{tag}]" if tag else "")
            kw = {**params.get(name, {}), **extra}
            for charge in (True, False):
                key = f"{key_base}{'' if charge else '[cost-free]'}"
                by_b = {}
                for b in budgets:
                    runs = []
                    for sd in seeds:
                        sel = SelectorSpec(REGISTRY[name], seed=sd, **kw)
                        ckw = dict(confirm_fn=lambda x, y: x in confirmed,
                                   confirm_cost_fn=lambda x, y: conf_cost[x] + conf_cost[y])
                        if tag in ("ir-structure", "raw-ir"):
                            be = _SigKeyReplay(dense, sd, {"ir-structure": "probe_signature_ir",
                                                           "raw-ir": "probe_signature_raw"}[tag], **ckw)
                        else:
                            be = ReplayBackend(dense, seed=sd, **ckw)
                        run = QueryBroker(be, valid, charge_policy_extra=charge,
                                          confirm_log_threshold=thr).run(sel, int(b))
                        iso_levels.add(run["isolation_level"])
                        append_jsonl(runs_path, {"delta": dkey, "variant": key, "budget_ns": int(b), "seed": sd,
                                                 **run})
                        runs.append(run)
                    by_b[int(b)] = runs
                results[key] = {"recall_cost": E.recall_cost(by_b, events, require_confirmation=True),
                                "recall_cost_resolved_only": E.recall_cost(by_b, events, require_confirmation=False),
                                "cost_to_find": {str(e): [E.discoveries(r, [e], True).get(e) for r in by_b[max(by_b)]]
                                                 for e in events}}
        out["by_delta"][dkey] = {"E_A": events, "results": results}
    out["isolation_levels"] = sorted(iso_levels)
    if label.startswith("frozen") and not all(lv.startswith("os:") for lv in iso_levels):
        raise SystemExit(f"frozen comparison requires OS-level selector isolation; got {sorted(iso_levels)}")
    write_json(a.out, out)
    first = next(iter(out["by_delta"].values()))
    for k, v in first["results"].items():
        last = v["recall_cost"][-1]
        print(f"{k:40s} recall@{last['budget_ns']:.3g}ns = {last['recall_mean']:.3f}")


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    s1 = sub.add_parser("answer-table")
    s1.add_argument("--discovery", nargs="+", required=True)
    s1.add_argument("--confirmation", nargs="+", required=True)
    s1.add_argument("--manifest", nargs="*", default=None, help="G4 manifest.json files (define the valid set)")
    s1.add_argument("--valid", default=None, help="explicit valid set, e.g. 41-256")
    s1.add_argument("--flagset", default="default")
    s1.add_argument("--delta", type=float, default=None)
    s1.add_argument("--alpha", type=float, default=None)
    s1.add_argument("--out", default=str(REPO_ROOT / "results/eval/answer_table_A.json"))
    s4 = sub.add_parser("alt-indicators")
    s4.add_argument("--measurements", nargs="+", required=True)
    s4.add_argument("--manifest", nargs="*", default=None)
    s4.add_argument("--valid", default=None)
    s4.add_argument("--default-flagset", default="default")
    s4.add_argument("--alternative-flagset", required=True)
    s4.add_argument("--out", default=str(REPO_ROOT / "results/eval/alternative_indicators.json"))
    s2 = sub.add_parser("census")
    s2.add_argument("--census-table", required=True)
    s2.add_argument("--answer-table", required=True)
    s2.add_argument("--units", type=int, nargs="*", default=None)
    s2.add_argument("--match-tolerance", type=int, default=None)
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
    s3.add_argument("--report-overhead-ns", type=float, default=None)
    s3.add_argument("--out", default=str(REPO_ROOT / "results/eval/policy_comparison.json"))
    for s in (s1, s2, s3, s4):
        s.add_argument("--allow-unfrozen", action="store_true")
    a = ap.parse_args()
    {"answer-table": cmd_answer_table, "alt-indicators": cmd_alt_indicators, "census": cmd_census,
     "compare": cmd_compare}[a.cmd](a)


if __name__ == "__main__":
    main()

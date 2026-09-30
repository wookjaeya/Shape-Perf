#!/usr/bin/env python3
"""Execution-identity verification (follow-up E1-B/E1-C): which compute code does each call run?

For every case (an explicit sequence of model loads and calls) three FRESH processes are started
with `exec` (never reused, never forked from a process that loaded a model):
  1. under gdb (scripts/gdb_identity_trace.py): breakpoints on the entry, the C wrapper and the
     compute function of every artifact; each hit records the DSO that contains the program counter,
     the module-relative address and the SHA-256 of the function bytes read from process memory;
  2. LD_DEBUG=bindings, default lazy binding: the dynamic linker's own record of which object each
     symbol reference of each artifact was bound to;
  3. LD_DEBUG=bindings with LD_BIND_NOW=1: auxiliary (binding time changed), never a substitute for 2.
Every call also checks its output against the exact expected permutation.

Per call the verdict is
  verified_own    entry, wrapper and compute all ran in the requested artifact, and the executed
                  compute bytes equal that artifact's compute function
  verified_other  the entry is the requested artifact's but the compute code that ran belongs to
                  another loaded artifact (by mapping path AND executed bytes)
  unresolved      anything else (missing or extra hits, tracer error, bytes that match no artifact)
Output agreement is recorded but never used as identity evidence.

  python scripts/verify_execution_identity.py --set orig --out results/v3_followup/e1/identity_orig
  python scripts/verify_execution_identity.py --set tagged --out results/v3_followup/e1/identity_tagged

Nothing here is timed.
"""
import argparse
import glob
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import elfinfo, provenance, toolchain  # noqa: E402
from shapeperf.util import REPO_ROOT, sha256_file, write_json  # noqa: E402

PY = sys.executable
G2_CELL = Path("/home/user/work/v3/g2/K/L0064")
TAGGED_CELL = Path("/home/user/work/v3_followup/e1/K_L0064_tagged")


def arm_sets(work):
    """Artifacts per set. `tag` None = compiled without --tag and loaded without a tag (runtime tag
    'model' from the file name), exactly as in v3."""
    aa = Path(work) / "orig_aa" / "AA_full" / "model.so"      # byte copy of S8, same basename (as in v3)
    orig = {"S8": {"path": str(G2_CELL / "S8_full/model.so"), "tag": None},
            "S1": {"path": str(G2_CELL / "S1_full/model.so"), "tag": None},
            "AA": {"path": str(aa), "tag": None, "copy_of": "S8"}}
    tagged = {"S8a": {"path": str(TAGGED_CELL / "S8_alpha_full/model.so"), "tag": "alpha", "policy": "S8"},
              "S1b": {"path": str(TAGGED_CELL / "S1_bravo_full/model.so"), "tag": "bravo", "policy": "S1"},
              "S8b": {"path": str(TAGGED_CELL / "S8_bravo_full/model.so"), "tag": "bravo", "policy": "S8"},
              "S1a": {"path": str(TAGGED_CELL / "S1_alpha_full/model.so"), "tag": "alpha", "policy": "S1"}}
    return {"orig": (orig, G2_CELL), "tagged": (tagged, TAGGED_CELL)}


def _L(*arms):
    return [["load", a] for a in arms]


def _C(*arms):
    return [["call", a] for a in arms]


def cases(set_name):
    if set_name == "orig":
        return {
            "I8": ("isolated", _L("S8") + _C("S8", "S8")),
            "I1": ("isolated", _L("S1") + _C("S1", "S1")),
            "IAA": ("isolated", _L("AA") + _C("AA", "AA")),
            "C81-8": ("co-loaded", _L("S8", "S1") + _C("S8", "S1", "S8", "S1")),
            "C81-1": ("co-loaded", _L("S8", "S1") + _C("S1", "S8", "S1", "S8")),
            "C18-8": ("co-loaded", _L("S1", "S8") + _C("S8", "S1", "S8", "S1")),
            "C18-1": ("co-loaded", _L("S1", "S8") + _C("S1", "S8", "S1", "S8")),
            "SEQ81": ("co-loaded, load-then-call", _L("S8") + _C("S8") + _L("S1") + _C("S1", "S8")),
            "SEQ18": ("co-loaded, load-then-call", _L("S1") + _C("S1") + _L("S8") + _C("S8", "S1")),
            "SCAN-S8-S1-AA": ("v3 scan harness order", _L("S8", "S1", "AA") + _C("S8", "S1", "AA", "S1", "AA", "S8")),
            "CAA-S8-AA": ("co-loaded A/A", _L("S8", "AA") + _C("AA", "S8")),
        }
    return {
        "I8a": ("isolated", _L("S8a") + _C("S8a", "S8a")),
        "I1b": ("isolated", _L("S1b") + _C("S1b", "S1b")),
        "I8b": ("isolated", _L("S8b") + _C("S8b", "S8b")),
        "I1a": ("isolated", _L("S1a") + _C("S1a", "S1a")),
        "C8a1b-8": ("co-loaded, distinct tags", _L("S8a", "S1b") + _C("S8a", "S1b", "S8a", "S1b")),
        "C8a1b-1": ("co-loaded, distinct tags", _L("S8a", "S1b") + _C("S1b", "S8a", "S1b", "S8a")),
        "C1b8a-8": ("co-loaded, distinct tags", _L("S1b", "S8a") + _C("S8a", "S1b", "S8a", "S1b")),
        "C1b8a-1": ("co-loaded, distinct tags", _L("S1b", "S8a") + _C("S1b", "S8a", "S1b", "S8a")),
        "C8b1a-8": ("co-loaded, tags swapped", _L("S8b", "S1a") + _C("S8b", "S1a", "S8b", "S1a")),
        "C8b1a-1": ("co-loaded, tags swapped", _L("S8b", "S1a") + _C("S1a", "S8b", "S1a", "S8b")),
        "C1a8b-8": ("co-loaded, tags swapped", _L("S1a", "S8b") + _C("S8b", "S1a", "S8b", "S1a")),
        "C1a8b-1": ("co-loaded, tags swapped", _L("S1a", "S8b") + _C("S1a", "S8b", "S1a", "S8b")),
        "SEQ8a1b": ("co-loaded, load-then-call", _L("S8a") + _C("S8a") + _L("S1b") + _C("S1b", "S8a")),
        "SEQ1b8a": ("co-loaded, load-then-call", _L("S1b") + _C("S1b") + _L("S8a") + _C("S8a", "S1b")),
        "CAA-8a8b": ("co-loaded A/A-tag", _L("S8a", "S8b") + _C("S8b", "S8a", "S8b", "S8a")),
        "CAA-8b8a": ("co-loaded A/A-tag", _L("S8b", "S8a") + _C("S8a", "S8b", "S8a", "S8b")),
        "SAMETAG-8a1a-1": ("DIAGNOSTIC ONLY: same tag in one process", _L("S8a", "S1a") + _C("S1a", "S8a")),
        "SAMETAG-1a8a-8": ("DIAGNOSTIC ONLY: same tag in one process", _L("S1a", "S8a") + _C("S8a", "S1a")),
    }


def _names(tag):
    t = tag or "model"
    return {"entry": f"run_main_graph_{t}", "wrapper": f"_mlir_ciface_main_graph_{t}", "compute": f"main_graph_{t}"}


def _real(p):
    return os.path.realpath(p) if p else None


def _worker_env(extra=None):
    env = dict(os.environ, OMP_NUM_THREADS="1", MKL_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1",
               SHAPEPERF_WORK=str(toolchain.work_dir()))
    env.update(extra or {})
    return env


def run_case(case_id, desc, steps, arms, cell, out):
    d = out / case_id
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    used = sorted({a for _op, a in steps})
    spec = {"arms": {a: arms[a] for a in used}, "steps": steps,
            "input_npy": str(cell / "input.npy"), "expected_npy": str(cell / "expected.npy")}
    symbols = sorted({f"{k}={v}" for a in used for k, v in _names(arms[a]["tag"]).items()})
    runs = {}
    # 1. gdb
    spec_g = dict(spec, call_log=str(d / "calls_gdb.jsonl"))
    write_json(d / "spec_gdb.json", spec_g)
    env = _worker_env({"IDENTITY_REPO": str(REPO_ROOT), "IDENTITY_TRACE_OUT": str(d / "gdb_hits.jsonl"),
                       "IDENTITY_SYMBOLS": ",".join(symbols)})
    cmd = ["gdb", "-batch", "-nx", "-x", str(REPO_ROOT / "scripts/gdb_identity_trace.py"), "--args",
           PY, "-m", "shapeperf.identity", f"@{d / 'spec_gdb.json'}"]
    p = subprocess.run(cmd, cwd=REPO_ROOT, env=env, capture_output=True, text=True)
    (d / "gdb_stdout.txt").write_text(p.stdout + p.stderr)
    runs["gdb"] = {"argv": cmd, "returncode": p.returncode}
    # 2./3. LD_DEBUG=bindings, lazy and BIND_NOW
    for mode, extra in (("lazy", {}), ("bindnow", {"LD_BIND_NOW": "1"})):
        spec_l = dict(spec, call_log=str(d / f"calls_ld_{mode}.jsonl"))
        write_json(d / f"spec_ld_{mode}.json", spec_l)
        raw_prefix = d / f"_lddebug_{mode}"
        env = _worker_env({"LD_DEBUG": "bindings", "LD_DEBUG_OUTPUT": str(raw_prefix), **extra})
        cmd = [PY, "-m", "shapeperf.identity", f"@{d / f'spec_ld_{mode}.json'}"]
        p = subprocess.run(cmd, cwd=REPO_ROOT, env=env, capture_output=True, text=True)
        paths = [arms[a]["path"] for a in used]
        lines = []
        for raw in sorted(glob.glob(f"{raw_prefix}.*")):
            with open(raw, errors="replace") as f:
                lines += [ln.rstrip("\n") for ln in f if "binding file" in ln and any(pa in ln for pa in paths)]
            os.remove(raw)                          # raw log (all Python bindings) is not kept; filter below
        (d / f"ld_bindings_{mode}.txt").write_text("\n".join(lines) + "\n")
        runs[f"ld_{mode}"] = {"argv": cmd, "env": {k: v for k, v in extra.items()}, "returncode": p.returncode,
                              "stdout": p.stdout.strip()[-300:], "stderr_tail": p.stderr[-800:],
                              "filter": "lines containing 'binding file' and one of the artifact paths"}
    return {"case_id": case_id, "description": desc, "steps": steps, "arms": {a: arms[a] for a in used},
            "symbols": symbols, "runs": runs, "dir": str(d)}


BIND_RE = re.compile(r"binding file (\S+) \[\d+\] to (\S+) \[\d+\]: normal symbol `([^']+)'")


def _bindings(path):
    out = {}
    for ln in Path(path).read_text().splitlines():
        m = BIND_RE.search(ln)
        if m:
            out.setdefault((_real(m.group(1)), m.group(3)), _real(m.group(2)))
    return out


def analyse_case(res, arms, runtime_sha):
    d = Path(res["dir"])
    comp_sha = {a: elfinfo.function_sha256(arms[a]["path"], _names(arms[a]["tag"])["compute"]) for a in res["arms"]}
    art_sha = {a: sha256_file(arms[a]["path"]) for a in res["arms"]}
    by_path = {_real(arms[a]["path"]): a for a in res["arms"]}
    calls = [json.loads(ln) for ln in (d / "calls_gdb.jsonl").read_text().splitlines()] if (d / "calls_gdb.jsonl").exists() else []
    calls = [c for c in calls if c["op"] == "call"]
    planned = [a for op, a in res["steps"] if op == "call"]
    run_failed = [k for k, r in res.get("runs", {}).items() if r.get("returncode") not in (0, None)]
    if len(calls) != len(planned) or [c["arm"] for c in calls] != planned:
        # the traced worker did not complete: every planned call is unresolved (never silently dropped)
        return [{"case_id": res["case_id"], "call_index": k, "requested_arm": a, "identity_verdict": "unresolved",
                 "output_check": "not run", "reasons": [f"traced worker logged {len(calls)} of {len(planned)} planned "
                                                        f"calls; failed runs: {run_failed or 'none'}"]}
                for k, a in enumerate(planned)]
    hits = [json.loads(ln) for ln in (d / "gdb_hits.jsonl").read_text().splitlines()] if (d / "gdb_hits.jsonl").exists() else []
    groups, cur = [], None
    for h in hits:
        if h["kind"] == "entry":
            cur = {"entry": [h], "wrapper": [], "compute": []}
            groups.append(cur)
        elif cur is not None:
            cur[h["kind"]].append(h)
    binds = {m: _bindings(d / f"ld_bindings_{m}.txt") for m in ("lazy", "bindnow")}
    load_order = [a for op, a in res["steps"] if op == "load"]
    first_calls = []
    for op, a in res["steps"]:
        if op == "call" and a not in first_calls:
            first_calls.append(a)
    rows = []
    for k, c in enumerate(calls):
        req = c["arm"]
        row = {"case_id": res["case_id"], "call_index": k, "requested_arm": req, "load_order": load_order,
               "first_call_order": first_calls, "artifact_sha256": art_sha[req], "runtime_sha256": runtime_sha,
               "tag": arms[req]["tag"] or "model (file name, no --tag)", "entry_symbol": _names(arms[req]["tag"])["entry"],
               "output_check": "exact_permutation" if c["exact_permutation"] else "FAIL",
               "binding_or_trace_evidence_path": str(d.relative_to(REPO_ROOT) if d.is_relative_to(REPO_ROOT) else d)}
        reasons = []
        if len(groups) != len(calls):
            reasons.append(f"{len(groups)} entry hits for {len(calls)} calls")
        g = groups[k] if k < len(groups) else None
        if g is None or len(g["compute"]) != 1 or len(g["wrapper"]) != 1:
            reasons.append("missing or extra wrapper/compute hits for this call")
            row.update(identity_verdict="unresolved", reasons=reasons)
            rows.append(row)
            continue
        e, w, x = g["entry"][0], g["wrapper"][0], g["compute"][0]
        for h in (e, w, x):
            if h.get("tracer_error"):
                reasons.append(f"tracer error: {h['tracer_error']}")
        entry_arm, wrap_arm, comp_arm = (by_path.get(_real(h.get("solib"))) for h in (e, w, x))
        executed = x.get("executed_function_sha256")
        bytes_match = sorted(a for a, s in comp_sha.items() if s == executed)
        row.update({"entry_dso_arm": entry_arm, "wrapper_dso_arm": wrap_arm, "actual_compute_arm": comp_arm,
                    "actual_compute_module_sha256": art_sha.get(comp_arm),
                    "actual_compute_symbol_or_offset": f"{x.get('symbol')}+{x.get('offset_in_symbol')} "
                                                       f"(module-relative {x.get('module_relative_start')})",
                    "executed_compute_sha256": executed, "executed_bytes_match_arms": bytes_match,
                    "ld_debug": {}})
        if len({comp_sha[a] for a in comp_sha}) < len(comp_sha):
            row["note"] = "some loaded artifacts have byte-identical compute functions; the mapping path decides"
        for m, b in binds.items():
            tgt = b.get((_real(arms[req]["path"]), _names(arms[req]["tag"])["wrapper"]))
            row["ld_debug"][m] = {"wrapper_reference_of_requested_bound_to": by_path.get(tgt, tgt)}
        if reasons or entry_arm is None or comp_arm is None:
            verdict = "unresolved"
        elif entry_arm != req:
            verdict, reasons = "unresolved", reasons + [f"entry ran in {entry_arm}, not the requested {req}"]
        elif comp_arm == req and wrap_arm == req and comp_sha[req] == executed:
            verdict = "verified_own"
        elif comp_arm != req and comp_arm in comp_sha and comp_sha[comp_arm] == executed:
            verdict = "verified_other"
        else:
            verdict, reasons = "unresolved", reasons + ["executed bytes do not match the artifact mapped at that address"]
        lazy = row["ld_debug"]["lazy"]["wrapper_reference_of_requested_bound_to"]
        ld_failed = [m for m in ("lazy", "bindnow") if res.get("runs", {}).get(f"ld_{m}", {}).get("returncode") not in (0, None)]
        if lazy is not None and lazy != wrap_arm:
            reasons.append(f"LD_DEBUG lazy binding ({lazy}) disagrees with gdb wrapper DSO ({wrap_arm})")
            verdict = "unresolved"
            row["ld_debug_corroboration"] = "disagrees"
        elif lazy is None or ld_failed:
            # no corroboration is not evidence of correct binding; it is reported, never silently skipped
            row["ld_debug_corroboration"] = "missing"
            reasons.append("LD_DEBUG corroboration missing" + (f" (run failed: {ld_failed})" if ld_failed else
                                                               " (no binding line for the wrapper reference)"))
        else:
            row["ld_debug_corroboration"] = "agrees"
        row.update(identity_verdict=verdict, reasons=reasons)
        rows.append(row)
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--set", choices=["orig", "tagged"], required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--work", default="/home/user/work/v3_followup/e1")
    ap.add_argument("--only", nargs="*")
    a = ap.parse_args()
    out = Path(a.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    arms, cell = arm_sets(a.work)[a.set]
    if a.set == "orig":
        aa = Path(arms["AA"]["path"])
        aa.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(arms["S8"]["path"], aa)
        assert sha256_file(aa) == sha256_file(arms["S8"]["path"])
    prov = provenance.freeze(out / "provenance.json", extra={"role": f"followup-e1-identity-{a.set}", "cell": str(cell),
                                                             "note": "nothing timed; development container"})
    pyrt = sorted(glob.glob(str(toolchain.pyruntime_dir() / "PyRuntimeC*.so")))
    runtime = {"pyruntime": pyrt, "pyruntime_sha256": [sha256_file(p) for p in pyrt], "python": sys.version,
               "libc": subprocess.run(["ldd", "--version"], capture_output=True, text=True).stdout.splitlines()[0],
               "gdb": subprocess.run(["gdb", "--version"], capture_output=True, text=True).stdout.splitlines()[0],
               "kernel": os.uname().release}
    runtime_sha = runtime["pyruntime_sha256"][0] if runtime["pyruntime_sha256"] else "unavailable"
    artifacts = {k: {**v, "sha256": sha256_file(v["path"]), "build_id": elfinfo.read_elf(v["path"])["build_id"],
                     "compute_function_sha256": elfinfo.function_sha256(v["path"], _names(v["tag"])["compute"])}
                 for k, v in arms.items()}
    results, rows = [], []
    for cid, (desc, steps) in cases(a.set).items():
        if a.only and cid not in a.only:
            continue
        res = run_case(cid, desc, steps, arms, cell, out)
        res["rows"] = analyse_case(res, arms, runtime_sha)
        write_json(out / cid / "case.json", res)
        results.append({k: res[k] for k in ("case_id", "description", "steps", "runs")})
        rows += res["rows"]
        verdicts = [f"{r['requested_arm']}->{r.get('actual_compute_arm')}:{r['identity_verdict']}" for r in res["rows"]]
        print(f"{cid:18s} {' '.join(verdicts)}", flush=True)
    summary = {"set": a.set, "cell": str(cell), "provenance_id": prov["provenance_id"], "source": prov["source"],
               "runtime": runtime, "artifacts": artifacts, "cases": results,
               "rows": rows, "verdict_counts": {v: sum(r["identity_verdict"] == v for r in rows)
                                                for v in ("verified_own", "verified_other", "unresolved")},
               "ld_debug_corroboration_counts": {v: sum(r.get("ld_debug_corroboration") == v for r in rows)
                                                 for v in ("agrees", "missing", "disagrees")},
               "method": __doc__.split("\n\n")[1], "note": "nothing timed; development container"}
    write_json(out / "identity_table.json", summary)
    print(json.dumps(summary["verdict_counts"]))


if __name__ == "__main__":
    main()

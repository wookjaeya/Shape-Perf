"""Execution-identity worker (follow-up E1): performs an explicit sequence of model loads and calls in
ONE fresh process, so that a tracer (gdb, LD_DEBUG) attached to that process can record which
compute code each call actually executed.

  python -m shapeperf.identity @<spec.json>      (or the JSON text itself)

spec = {"arms": {"S8": {"path": ".../model.so", "tag": null}, ...},
        "steps": [["load", "S8"], ["load", "S1"], ["call", "S1"], ["call", "S8"], ...],
        "input_npy": ..., "expected_npy": ..., "call_log": "<jsonl path>"}

A "load" creates OMExecutionSession(shared_lib_path, tag=<tag>) (tag omitted when null, i.e. the
runtime derives it from the file name); a "call" runs the arm's session once on the input and checks
that the output is the exact expected permutation. Sessions and outputs are kept alive to the end.
Nothing is timed here: runs under a tracer are never performance data.
"""
import json
import os
import re
import sys

from . import elfinfo

# Model-specific symbols that ONNX-MLIR suffixes with the model tag (`--tag`, default: the output file
# name without extension). Untagged wrappers (run_main_graph, omQueryEntryPoints, ...) and the runtime
# functions are shared by every model library and are not listed.
MODEL_SYMBOL_RE = re.compile(r"^(run_main_graph|_mlir_ciface_main_graph|main_graph|omQueryEntryPoints|"
                             r"omInputSignature|omOutputSignature|omCompilationInfo)_.+")


def model_symbols(path):
    """Exported model-specific function names of one compiled model library."""
    return {n for n in elfinfo.exported_functions(elfinfo.read_elf(path)) if MODEL_SYMBOL_RE.match(n)}


def assert_no_shared_model_symbols(paths):
    """Refuse to put several model libraries into ONE process when they export the same model-specific
    symbols (i.e. they were compiled with the same tag, e.g. both named model.so without --tag).

    The ONNX-MLIR runtime opens every model with dlopen(RTLD_LAZY | RTLD_GLOBAL) and only the entry is
    looked up in the model's own handle; the entry's internal calls go through the PLT and bind to the
    FIRST loaded definition. Follow-up E1 traced this at L=64: the later-loaded arm ran the earlier arm's
    compute code in every co-loaded case (results/v3_followup/e1/identity_orig)."""
    owner, clashes = {}, []
    for p in paths:
        real = os.path.realpath(p)
        for sym in sorted(model_symbols(p)):
            if sym in owner and owner[sym] != real:
                clashes.append(f"{sym}: {owner[sym]} and {real}")
            owner.setdefault(sym, real)
    if clashes:
        raise ValueError("model libraries share model-specific symbols and cannot be co-loaded in one process "
                         "(compile them with distinct --tag values and pass the tags to the runtime): "
                         + "; ".join(clashes[:6]))


def open_session(om, path, tag=None):
    """OMExecutionSession for one artifact; `tag` must be the --tag the artifact was compiled with
    (None: the runtime derives it from the file name, which is correct only for untagged builds)."""
    return om(shared_lib_path=path, tag=tag) if tag else om(shared_lib_path=path)


def identity_worker(spec):
    import numpy as np
    from . import toolchain
    om = toolchain.import_pyruntime()
    x = np.load(spec["input_npy"])
    expected = np.load(spec["expected_npy"])
    sessions, outputs, log = {}, [], []
    n_call = 0
    for i, (op, arm) in enumerate(spec["steps"]):
        cfg = spec["arms"][arm]
        if op == "load":
            sessions[arm] = open_session(om, cfg["path"], cfg.get("tag"))      # deliberately NOT guarded
            log.append({"step": i, "op": "load", "arm": arm, "path": cfg["path"], "tag": cfg.get("tag")})
        elif op == "call":
            out = sessions[arm].run([x])
            outputs.append(out)
            exact = bool(len(out) == 1 and out[0].dtype == expected.dtype and np.array_equal(out[0], expected))
            log.append({"step": i, "op": "call", "arm": arm, "call_index": n_call, "exact_permutation": exact})
            n_call += 1
        else:
            raise ValueError(f"unknown step {op!r}")
    with open(spec["call_log"], "w") as f:
        for rec in log:
            f.write(json.dumps(rec) + "\n")
    return {"pid": os.getpid(), "n_calls": n_call, "all_exact": all(r.get("exact_permutation", True) for r in log)}


def _load_spec(arg):
    if arg.startswith("@"):
        with open(arg[1:]) as f:
            return json.load(f)
    return json.loads(arg)


if __name__ == "__main__":
    print(json.dumps(identity_worker(_load_spec(sys.argv[1]))))

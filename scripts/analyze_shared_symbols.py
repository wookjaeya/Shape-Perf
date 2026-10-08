#!/usr/bin/env python3
"""E1-A for tagged artifacts: which exported symbols are STILL shared between two model libraries
compiled with distinct tags, whether the shared functions are the same code, and which references
actually crossed from one library to the other in the co-loaded identity runs (LD_DEBUG, lazy).

Shared names are candidates, not faults: a cross-library binding matters only if it reaches code or
data that differs between the arms or lies on the compute path.

  python scripts/analyze_shared_symbols.py --identity results/v3_followup/e1/identity_tagged \
      --pair S8a S1b --out results/v3_followup/e1/shared_symbols_tagged.json
"""
import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf import elfinfo  # noqa: E402
from shapeperf.util import write_json  # noqa: E402

BIND_RE = re.compile(r"binding file (\S+) \[\d+\] to (\S+) \[\d+\]: normal symbol `([^']+)'")


def normalized_asm(path, sym, tag):
    out = subprocess.run(["objdump", "-d", "--no-show-raw-insn", path], capture_output=True, text=True).stdout
    lines, on = [], False
    for ln in out.splitlines():
        if ln.endswith(f"<{sym}>:"):
            on = True
            continue
        if on and not ln.strip():
            break
        if on:
            ins = ln.split("\t", 1)[-1].split("#")[0].strip()
            ins = re.sub(r"\b[0-9a-f]{3,}\b <([^>]+)>", r"<\1>", ins)
            ins = re.sub(rf"_{tag}\b", "_TAG", ins)
            lines.append(re.sub(r"0x[0-9a-f]+\(%rip\)", "RIP", ins))
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--identity", required=True)
    ap.add_argument("--pair", nargs=2, required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    table = json.loads((Path(a.identity) / "identity_table.json").read_text())
    arts = {k: table["artifacts"][k] for k in a.pair}
    infos = {k: elfinfo.read_elf(v["path"]) for k, v in arts.items()}
    exported = {k: {s["name"]: s for s in i["symbols"] if s["table"] == "dynsym" and s["defined"]
                    and s["bind"] in ("GLOBAL", "WEAK")} for k, i in infos.items()}
    k0, k1 = a.pair
    shared = sorted(set(exported[k0]) & set(exported[k1]))
    funcs, objects = {}, {}
    for n in shared:
        s0 = exported[k0][n]
        if s0["type"] == "FUNC":
            same = (normalized_asm(arts[k0]["path"], n, arts[k0]["tag"]) ==
                    normalized_asm(arts[k1]["path"], n, arts[k1]["tag"]))
            funcs[n] = {"same_code_modulo_addresses": same}
        else:
            objects[n] = {"type": s0["type"], "size": s0["size"]}
    compute = {k: f"main_graph_{arts[k]['tag']}" for k in a.pair}
    compute_calls = {}
    for k in a.pair:
        asm = normalized_asm(arts[k]["path"], compute[k], arts[k]["tag"])
        compute_calls[k] = sorted({m.group(1) for ln in asm for m in [re.search(r"call\s+<([^>]+)>", ln)] if m})
    by_path = {v["path"]: k for k, v in arts.items()}
    crossed = Counter()
    for case in table["cases"]:
        f = Path(a.identity) / case["case_id"] / "ld_bindings_lazy.txt"
        if not f.exists():
            continue
        for ln in f.read_text().splitlines():
            m = BIND_RE.search(ln)
            if m and m.group(1) in by_path and m.group(2) in by_path and m.group(1) != m.group(2):
                crossed[(case["case_id"], by_path[m.group(1)], by_path[m.group(2)], m.group(3))] += 1
    crossed_syms = sorted({c[3] for c in crossed})
    res = {"pair": a.pair, "artifacts": {k: {"path": v["path"], "tag": v["tag"], "sha256": v["sha256"]} for k, v in arts.items()},
           "shared_exported": {"functions": funcs, "objects": objects},
           "compute_function": compute, "calls_made_by_compute_function": compute_calls,
           "cross_library_bindings_observed": [{"case": c, "from": f, "to": t, "symbol": s, "n": n}
                                               for (c, f, t, s), n in sorted(crossed.items())],
           "crossed_symbols": {s: {"shared_function_same_code": funcs.get(s, {}).get("same_code_modulo_addresses"),
                                   "is_object": s in objects} for s in crossed_syms},
           "crossed_symbol_on_compute_path": sorted(set(crossed_syms) & set(sum(compute_calls.values(), []))),
           "crossed_symbol_with_different_code": sorted(s for s in crossed_syms if s in funcs
                                                        and not funcs[s]["same_code_modulo_addresses"])}
    write_json(a.out, res)
    print(json.dumps({k: res[k] for k in ("calls_made_by_compute_function", "crossed_symbol_on_compute_path",
                                          "crossed_symbol_with_different_code")}, indent=1))
    print("crossed symbols:", len(crossed_syms), "shared funcs:", len(funcs), "shared objects:", list(objects))


if __name__ == "__main__":
    main()

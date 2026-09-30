#!/usr/bin/env python3
"""Final-code evidence for a pair of single-op artifacts (design amendment v3, G2 step 3).

For two compiled libraries (arm S8 and arm S1 of the same op and length) this saves the
disassembly of the compute function(s), and summarizes what the tested decision left in the final
code: instruction counts per mnemonic and per register class, the text size, the sizes of the
compute function, the calls and the number of loops (backward branches). Nothing is interpreted:
whether LLVM re-vectorized or re-unrolled the simplified loop is read off these numbers.

  python scripts/collect_lowering_trace.py --s8 <S8/model.so> --s1 <S1/model.so> --out <dir> [--label K_L49]
"""
import argparse
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from shapeperf.util import write_json  # noqa: E402

FUNC_RE = re.compile(r"^([0-9a-f]+) <([^>]+)>:$")
COMPUTE_RE = re.compile(r"main_graph")


def disassemble(so_path):
    p = subprocess.run(["objdump", "-d", "--no-show-raw-insn", "-M", "att", str(so_path)],
                       capture_output=True, text=True, timeout=1800)
    if p.returncode != 0:
        raise RuntimeError(p.stderr[-500:])
    return p.stdout


def split_functions(text):
    funcs, name, buf = {}, None, []
    for line in text.splitlines():
        m = FUNC_RE.match(line)
        if m:
            if name:
                funcs[name] = buf
            name, buf = m.group(2), []
        elif name and line.strip():
            buf.append(line)
    if name:
        funcs[name] = buf
    return funcs


def summarize(lines):
    mn, cls, calls, back = Counter(), Counter(), Counter(), 0
    addrs = {}
    body = []
    for line in lines:
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        addr = parts[0].strip().rstrip(":")
        ins = parts[-1].strip()
        if not ins:
            continue
        body.append((addr, ins))
    for i, (addr, _ins) in enumerate(body):
        addrs[addr] = i
    for i, (addr, ins) in enumerate(body):
        m = ins.split()[0]
        mn[m] += 1
        ops = ins[len(m):]
        cls["zmm" if "%zmm" in ops else "ymm" if "%ymm" in ops else "xmm" if "%xmm" in ops else "gpr"] += 1
        if m.startswith("call"):
            t = re.search(r"<([^>+]+)", ops)
            calls[t.group(1) if t else "indirect"] += 1
        if m.startswith("j"):
            t = re.match(r"\s*([0-9a-f]+)", ops)
            if t and t.group(1) in addrs and addrs[t.group(1)] <= i:     # backward branch = loop latch
                back += 1
    return {"instructions": len(body), "mnemonics": dict(mn.most_common()), "register_class": dict(cls),
            "calls": dict(calls), "backward_branches": back}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--s8", required=True)
    ap.add_argument("--s1", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--label", default="case")
    a = ap.parse_args()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    res = {"label": a.label, "arms": {}}
    for arm, so in (("S8", a.s8), ("S1", a.s1)):
        text = disassemble(so)
        funcs = split_functions(text)
        compute = {n: l for n, l in funcs.items() if COMPUTE_RE.search(n)}
        (out / f"{arm}.compute.asm").write_text("\n".join(x for n in sorted(compute) for x in [f"## {n}", *compute[n]]) + "\n")
        res["arms"][arm] = {"so_bytes": Path(so).stat().st_size, "functions": len(funcs),
                            "compute_functions": sorted(compute),
                            "compute": summarize([x for n in sorted(compute) for x in compute[n]])}
    m8, m1 = (res["arms"][k]["compute"]["mnemonics"] for k in ("S8", "S1"))
    res["mnemonic_delta_S1_minus_S8"] = {k: m1.get(k, 0) - m8.get(k, 0) for k in sorted(set(m8) | set(m1))
                                         if m1.get(k, 0) != m8.get(k, 0)}
    write_json(out / "trace.json", res)
    for arm in ("S8", "S1"):
        c = res["arms"][arm]["compute"]
        print(arm, "so", res["arms"][arm]["so_bytes"], "instr", c["instructions"], c["register_class"],
              "loops(back branches)", c["backward_branches"])
    print("largest mnemonic changes (S1 - S8):",
          dict(sorted(res["mnemonic_delta_S1_minus_S8"].items(), key=lambda kv: -abs(kv[1]))[:8]))


if __name__ == "__main__":
    main()

"""Structural signatures of compiler decisions (spec §6.2, §9.4, §11.2-3).

Only fields that the pinned ONNX-MLIR actually prints are parsed:
  --opt-report=Simd      ==SIMD-REPORT==, <op>[-simd], <node>, <message>, <VL>, <simd trip count>
  --opt-report=Parallel  ==PAR-REPORT==,  <op>[-par],  <node>, <message>, <loop level>, <par trip count>
(format from src/Conversion/ONNXToKrnl/ONNXToKrnlCommon.cpp at the pinned commit).

Normalization rules are versioned (SIG_VERSION) and must be frozen in
preregistration.md before evaluation data is looked at (spec §8.3). Raw text is
always stored next to the signature so the normalization can be audited.
"""
import hashlib
import json
import re
import subprocess
from collections import Counter

SIG_VERSION = "sig-v1"

NORMALIZATION_RULES = {
    "sig-v1": [
        "opt-report: each record keeps (report kind, lowered op name incl. -simd/-par suffix, "
        "ONNX node name, message with integers replaced by <N>, VL (SIMD) or loop level (PAR)).",
        "opt-report: trip counts are dropped - they are products of input-length literals, "
        "not decisions.",
        "opt-report: records form a sorted multiset; ordering of print statements is ignored.",
        "IR structure (probe IR): per-function histogram of op names, vector types kept verbatim, "
        "loop ops keyed by nesting depth; SSA names, memref/tensor shapes, integer literals, affine "
        "map constants, attributes and locations dropped.",
        "final artifact: set of (instruction mnemonic, widest vector register class) pairs and set of "
        "call targets from objdump; instruction counts are stored but not hashed.",
    ],
}

_REPORT_RE = re.compile(r"^==(SIMD|PAR)-REPORT==, (.*)$")
_INT_RE = re.compile(r"-?\d+")


def _h(obj):
    return hashlib.sha256(json.dumps(obj, sort_keys=True).encode()).hexdigest()[:16]


def parse_opt_report(text):
    """Parse report lines. Node names may in principle contain ', ', so split
    from both ends (op name and messages cannot contain commas)."""
    recs = []
    for line in text.splitlines():
        m = _REPORT_RE.match(line.strip())
        if not m:
            continue
        parts = m.group(2).split(", ")
        if len(parts) < 5:
            recs.append({"kind": m.group(1), "unparsed": line.strip()})
            continue
        op = parts[0]
        suffix = "-simd" if m.group(1) == "SIMD" else "-par"
        recs.append({
            "kind": m.group(1),
            "op": op[:-len(suffix)] if op.endswith(suffix) else op,
            "applied": op.endswith(suffix),
            "node": ", ".join(parts[1:-3]),
            "message": parts[-3],
            "value": int(parts[-2]),      # SIMD: vector length; PAR: loop level
            "trip_count": int(parts[-1]),  # excluded from the signature
        })
    return recs


def report_signature(recs):
    items = sorted(
        (r["kind"], r.get("op", ""), r.get("applied", False), r.get("node", ""),
         _INT_RE.sub("<N>", r.get("message", r.get("unparsed", ""))), r.get("value", 0))
        for r in recs)
    return {"version": SIG_VERSION, "n_records": len(items), "hash": _h(items), "items": items}


def report_summary(recs):
    """Per-op-type decision summary (for humans and for evidence bundles)."""
    out = {}
    for r in recs:
        if "op" not in r:
            continue
        k = f'{r["kind"]}:{r["op"]}'
        d = out.setdefault(k, {"n": 0, "applied": 0, "values": Counter(), "messages": Counter()})
        d["n"] += 1
        d["applied"] += int(r["applied"])
        d["values"][r["value"]] += 1
        d["messages"][_INT_RE.sub("<N>", r["message"])] += 1
    return {k: {"n": v["n"], "applied": v["applied"], "values": dict(v["values"]),
                "messages": dict(v["messages"])} for k, v in sorted(out.items())}


# ---------------------------------------------------------------------------
# Probe IR structure

_LOC_RE = re.compile(r"\s*loc\((?:[^()]|\([^()]*\))*\)")
_OPNAME_RE = re.compile(r'^\s*(?:(?:%[\w#.:]+(?::\d+)?(?:,\s*)?)+\s*=\s*)?"?([a-z_][\w]*\.[\w.]+)"?')
_VEC_RE = re.compile(r"vector<(?:\d+x)*[a-z]+\d*>")
_LOOP_OPS = {"affine.for", "scf.for", "affine.parallel", "scf.parallel", "krnl.iterate",
             "omp.wsloop", "omp.parallel", "omp.loop_nest"}


def strip_locations(ir_text):
    return "\n".join(l for l in (_LOC_RE.sub("", line) for line in ir_text.splitlines())
                     if not l.startswith("#loc"))


def ir_structure(ir_text):
    text = strip_locations(ir_text)
    funcs = {}
    cur = "<module>"
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("func.func") or s.startswith("llvm.func"):
            m = re.search(r"@([\w.$-]+)", s)
            cur = m.group(1) if m else "<anon>"
            funcs.setdefault(cur, {"ops": Counter(), "vectors": Counter(), "loops": Counter()})
            continue
        m = _OPNAME_RE.match(line)
        f = funcs.setdefault(cur, {"ops": Counter(), "vectors": Counter(), "loops": Counter()})
        if m:
            op = m.group(1)
            f["ops"][op] += 1
            if op in _LOOP_OPS:
                depth = (len(line) - len(line.lstrip(" "))) // 2
                f["loops"][f"{op}@{depth}"] += 1
        for v in _VEC_RE.findall(line):
            f["vectors"][v] += 1
    norm = {k: {"ops": dict(sorted(v["ops"].items())), "vectors": dict(sorted(v["vectors"].items())),
                "loops": dict(sorted(v["loops"].items()))} for k, v in sorted(funcs.items())}
    # Function names that embed lengths (none expected) would leak literals: keep names.
    return {"version": SIG_VERSION, "hash": _h(norm), "functions": norm}


def raw_ir_hash(ir_text):
    """Ablation §11.2-3: hash of the IR with locations removed but literals kept."""
    return hashlib.sha256(strip_locations(ir_text).encode()).hexdigest()[:16]


def entry_signature_from_ir(ir_text, entry="main_graph"):
    """Argument types of the entry function, to verify static specialization."""
    m = re.search(r"func\.func @" + re.escape(entry) + r"\((.*?)\)\s*(?:->|attributes|\{)", ir_text, re.S)
    if not m:
        return None
    return re.findall(r"(memref<[^>]*>|tensor<[^>]*>)", m.group(1))


# ---------------------------------------------------------------------------
# Final artifact (shared library)

_BLAS_RE = re.compile(r"cblas|sgemm|dgemm|mkl_|dnnl|openblas|blis|oneDNN", re.I)


def _run(cmd, timeout=600):
    try:
        return subprocess.run(cmd, capture_output=True, text=True, timeout=timeout).stdout
    except Exception as e:
        return f"__unavailable__ {e!r}"


def dynamic_imports(so_path):
    out = _run(["nm", "-D", "--undefined-only", str(so_path)])
    if out.startswith("__unavailable__"):
        return None
    return sorted({l.split()[-1] for l in out.splitlines() if l.strip()})


def final_artifact_features(so_path):
    """Vector register classes per mnemonic and call targets from objdump."""
    out = _run(["objdump", "-d", "--no-show-raw-insn", str(so_path)], timeout=1800)
    if out.startswith("__unavailable__"):
        return {"available": False, "reason": out}
    pairs, counts, calls = set(), Counter(), set()
    for line in out.splitlines():
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        ins = parts[-1].strip()
        if not ins:
            continue
        mnem = ins.split()[0]
        ops = ins[len(mnem):]
        cls = "zmm" if "%zmm" in ops else "ymm" if "%ymm" in ops else "xmm" if "%xmm" in ops else "gpr"
        pairs.add((mnem, cls))
        counts[f"{mnem}:{cls}"] += 1
        if mnem.startswith("call"):
            m = re.search(r"<([^>]+)>", ops)
            if m:
                calls.add(re.sub(r"\+0x[0-9a-f]+$", "", m.group(1)))
    vec_counts = Counter()
    for k, v in counts.items():
        vec_counts[k.split(":")[1]] += v
    return {"available": True, "version": SIG_VERSION,
            "hash": _h({"pairs": sorted(pairs), "calls": sorted(calls)}),
            "calls": sorted(calls), "register_class_counts": dict(vec_counts),
            "instruction_counts": dict(counts)}


def matmul_path(imports, report_recs):
    """Classify how MatMul/Gemm were lowered, from evidence only (spec §6.2, G0).
    Returns (label, evidence)."""
    if imports is None:
        return "unknown", {"reason": "nm unavailable"}
    blas = [s for s in imports if _BLAS_RE.search(s)]
    mm = [r for r in report_recs if r.get("op", "").split(".")[-1] in ("MatMul", "Gemm")]
    evidence = {"blas_like_imports": blas, "n_matmul_gemm_report_records": len(mm),
                "matmul_report_summary": report_summary(mm)}
    if blas:
        return "external-library", evidence
    if mm:
        return "compiler-generated", evidence
    return "unknown", evidence

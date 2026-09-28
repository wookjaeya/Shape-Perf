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

SIG_VERSION = "sig-v3"

NORMALIZATION_RULES = {
    "sig-v3": [
        "sig-v2 rules with two changes to node-name canonicalization (made before any performance "
        "data, after the third code review): (1) counters are stripped REPEATEDLY until a model node "
        "name is reached ('X_5_7' -> 'X_5' -> 'X'); (2) with the model's node op types, a generated "
        "name that equals a DIFFERENT original node of another op type is mapped to its base instead "
        "(e.g. a generated Mul 'mul_3_1' when the model's 'mul_3_1' is an Add). Names derived from "
        "the op name (e.g. 'onnx.MatMul_2') keep only '<N>' for the counter.",
        "report lines with trailing foreign text in the numeric fields are unparsed (report 'corrupt').",
        "known limitation: a generated op that reuses the name of an original node of the SAME op "
        "type (e.g. after that node was removed) cannot be told apart.",
    ],
    "sig-v2": [
        "sig-v1 rules, plus: node names are canonicalized to ONNX node names of the model. ONNX-MLIR "
        "names ops it creates while rewriting as '<original>_<counter>' (fused ops: '<a>-<b>_<counter>'); "
        "the counter differed between two compiles of the SAME length at G0 (e.g. mul_3_12 vs mul_3_13), "
        "so it is removed: a name not in the model loses one trailing _<digits>, each '-'-separated part "
        "is mapped the same way, and any remaining unknown digit run becomes <N>.",
        "report lines that do not parse (e.g. interleaved output) make the report 'corrupt'; the "
        "compiler is run with line-buffered stdout (stdbuf -oL) to prevent interleaving.",
    ],
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
        try:
            value, trip = int(parts[-2]), int(parts[-1])
        except ValueError:                 # interleaved foreign text after a field
            recs.append({"kind": m.group(1), "unparsed": line.strip()})
            continue
        recs.append({
            "kind": m.group(1),
            "op": op[:-len(suffix)] if op.endswith(suffix) else op,
            "applied": op.endswith(suffix),
            "node": ", ".join(parts[1:-3]),
            "message": parts[-3],
            "value": value,               # SIMD: vector length; PAR: loop level
            "trip_count": trip,           # excluded from the signature
        })
    return recs


_SUFFIX_RE = re.compile(r"_\d+$")


_OPV_RE = re.compile(r"V\d+$")


def _op_matches(report_op, onnx_op_type):
    """'onnx.ReduceMeanV13' matches ONNX op_type 'ReduceMean'."""
    if not report_op or not onnx_op_type:
        return True
    return _OPV_RE.sub("", report_op.split(".")[-1]) == onnx_op_type


def canonical_node(name, known, op=None):
    """Map a compiler-generated node name back to ONNX node names (sig-v3).

    known: set of ONNX node names, or dict name -> ONNX op_type (preferred).
    With op types, a generated name that happens to equal a different original
    node (e.g. generated 'mul_3_1' from 'mul_3' while an original 'mul_3_1'
    exists) is recognised by the op mismatch and mapped to its base."""
    if not known:
        return name
    types = known if isinstance(known, dict) else None
    if name in known:
        if types is None or _op_matches(op, types.get(name)):
            return name
        base = _SUFFIX_RE.sub("", name)
        if base in known and _op_matches(op, types.get(base)):
            return base
        return name

    def one(part):
        cur = part
        while True:                  # generated names can carry several counters: X_5_7 -> X_5 -> X
            if cur in known:
                return cur
            nxt = _SUFFIX_RE.sub("", cur)
            if nxt == cur:
                break
            cur = nxt
        return _INT_RE.sub("<N>", part)
    if "-" in name:
        stripped = _SUFFIX_RE.sub("", name)
        parts = stripped.split("-")
        mapped = [one(p) for p in parts]
        if all(m in known for m in mapped):
            return "-".join(mapped)
    return one(name)


def report_signature(recs, known_nodes=None):
    """Sorted multiset of decision tuples. known_nodes: ONNX node names of the
    model (enables sig-v3 canonicalization; without it the result is sig-v1)."""
    items = sorted(
        (r["kind"], r.get("op", ""), r.get("applied", False),
         canonical_node(r.get("node", ""), known_nodes, r.get("op")),
         _INT_RE.sub("<N>", r.get("message", r.get("unparsed", ""))), r.get("value", 0))
        for r in recs)
    unparsed = sum(1 for r in recs if "unparsed" in r)
    return {"version": SIG_VERSION if known_nodes else "sig-v1", "n_records": len(items), "hash": _h(items),
            "n_unparsed": unparsed, "integrity": "ok" if unparsed == 0 else "corrupt", "items": items}


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

_LOC_RE = re.compile(r"\s*(?<![\w.%$@])loc\((?:[^()]|\([^()]*\))*\)")
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

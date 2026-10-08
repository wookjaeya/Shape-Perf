"""Helpers to compare the IR of the two arms of the primary contrast (design amendment v3).

The tested lowering decision: `scalarTransposeOverOutputs` (ONNX-MLIR 1e017c9f, Transpose.cpp)
unrolls the innermost output loop by getNoLeftoverUnrollFactor(tripCount, cap) - the largest
u in [cap..2] that divides a LITERAL trip count, else 1. Arm S8 has cap 8 (the pinned compiler),
arm S1 has cap 1 (one constant patched, no unrolling).
"""
import re
import subprocess
import tempfile
from pathlib import Path

CAP_ORIGINAL = 8


def no_leftover_unroll(trip_count, cap=CAP_ORIGINAL):
    """Mirror of getNoLeftoverUnrollFactor for a literal trip count."""
    for u in range(cap, 1, -1):
        if trip_count % u == 0:
            return u
    return 1


_LOOP_RE = re.compile(r"affine\.for\s+%\S+\s*=\s*0\s+to\s+(\d+)\s+step\s+(\d+)")


def count_unrolled_loops(ir_text, trip_count, step):
    """Loops `affine.for %i = 0 to <trip_count> step <step>`: the shape the unrolled transpose
    loops take in the MLIR output (probe stage, after Krnl->Affine lowering)."""
    return sum(1 for m in _LOOP_RE.finditer(ir_text)
               if int(m.group(1)) == trip_count and int(m.group(2)) == step)


_SSA_RE = re.compile(r"%[\w.#$-]+")
_NUM_RE = re.compile(r"-?\d+")


def normalize_line(line):
    """Surface-independent form of an IR line: SSA names -> %v, numbers -> N."""
    return _NUM_RE.sub("N", _SSA_RE.sub("%v", line.strip()))


def normalized_lines(ir_text):
    """IR without affine-map definitions (their numbering shifts with any change) in normalized form."""
    return [normalize_line(line) for line in ir_text.splitlines() if not line.lstrip().startswith("#map")]


def diff_hunks(text_a, text_b):
    """Number of hunks of a minimal line diff between two IR texts (after normalization) and the
    number of changed lines, using GNU diff (Myers) - fast on ~40k-line files."""
    with tempfile.TemporaryDirectory() as d:
        pa, pb = Path(d, "a"), Path(d, "b")
        pa.write_text("\n".join(normalized_lines(text_a)) + "\n")
        pb.write_text("\n".join(normalized_lines(text_b)) + "\n")
        out = subprocess.run(["diff", "-U0", str(pa), str(pb)], capture_output=True, text=True).stdout
    hunks = sum(1 for line in out.splitlines() if line.startswith("@@"))
    changed = sum(1 for line in out.splitlines()
                  if line[:1] in "+-" and not line.startswith(("+++", "---")))
    return {"hunks": hunks, "changed_lines": changed}

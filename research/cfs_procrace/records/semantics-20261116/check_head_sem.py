#!/usr/bin/env python3
"""check_head_sem.py KZDUMP OLD NEW [--whole] [--allow-size] [--allow-new-section] -- two containers of one build (basis.kz or facts.kz of the re-extraction before
and after the extractor fixes) read by the same kz-dump: the header lines (up to the first `section` line), or with
--whole the whole dump, must be equal apart from tool_sha256=, artifact_sha256= and basis_sha256= (64 hex) and a facts'
parent[0] (the basis).  For a facts container the sections are compared by compare_classes.py over the text view; for a
basis container --whole compares every record.  --allow-size (results 5.0b) also allows the line
`container abi=1 kind=facts size=N` with another N: the byte count of a container whose content differs as declared.  rc 0 when nothing else differs, 1 otherwise.
(results 8.0b) A copy of check_head.py with --allow-new-section: the header line `sections=N` may be `sections=N+1` in NEW,
the one new section the option declares; check_head.py itself is unchanged."""
import re
import subprocess
import sys

H = r"[0-9a-f]{64}"
SIZE = re.compile(r"^container abi=1 kind=facts size=\d+$")
NSEC = re.compile(r"^sections=(\d+)$")
ALLOW = [re.compile(r"^(tool_sha256|artifact_sha256|basis_sha256)=" + H + "$"), re.compile(r"^  parent\[0\] kind=basis role=1 sha256=" + H + "$")]


def dump(kd, path, whole):
    out = subprocess.run([kd, path], capture_output=True, check=True).stdout.decode("utf-8", "surrogateescape").split("\n")
    if whole:
        return out
    head = []
    for l in out:
        if l.startswith("section "):
            break
        head.append(l)
    return head


def main():
    a = sys.argv[1:]
    whole = "--whole" in a
    allow_size = "--allow-size" in a
    allow_new = "--allow-new-section" in a
    a = [x for x in a if x not in ("--whole", "--allow-size", "--allow-new-section")]
    if len(a) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    x, y = dump(a[0], a[1], whole), dump(a[0], a[2], whole)
    bad, allowed = [], 0
    if len(x) != len(y):
        bad.append("line count %d / %d" % (len(x), len(y)))
    for i, (p, q) in enumerate(zip(x, y)):
        if p == q:
            continue
        if any(r.match(p) and r.match(q) and p.split("=")[0] == q.split("=")[0] for r in ALLOW):
            allowed += 1
        elif allow_new and NSEC.match(p) and NSEC.match(q) and int(NSEC.match(q).group(1)) == int(NSEC.match(p).group(1)) + 1:
            allowed += 1
        elif allow_size and SIZE.match(p) and SIZE.match(q):
            allowed += 1
        else:
            bad.append("line %d: %s | %s" % (i + 1, p[:120], q[:120]))
    print("check_head %s: lines %d, allowed %d, other %d" % ("whole" if whole else "header", len(y), allowed, len(bad)))
    for l in bad[:20]:
        print("  " + l)
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())

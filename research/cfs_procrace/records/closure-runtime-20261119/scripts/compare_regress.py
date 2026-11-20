#!/usr/bin/env python3
"""compare_regress.py OLD NEW -- the outputs of regress.sh before and after the build, file by file, with the declared
differences of results 23.0 (allowed differences by output kind):
  rc.txt                    the first line (the hashes of the tools and of the chain check script)
  c1.run.kzt.txt, c5.run.kzt.txt
                            the lines `tool_sha256=..` and `artifact_sha256=..` (as records/manifest-v4-20261116/compare_tools_2.py)
  c1.convert.log, c5.convert.log
                            the line `wrote <OUT>/<c>.run.kzt artifact_sha256 ..`, and ONE added line in NEW
                            `build-id records: none (kzrt-dump 1.4)` (event_semantics.md 13.5 (bar)); nothing else is added
  c1.run.kzt, c5.run.kzt    as bytes: the same size, differing bytes only at offsets 120..155 (tool_sha256, header_crc)
  c1.chain.txt              the line `same_build ...` of OLD is replaced in NEW by `same_build_object` lines (each
                            unconfirmed(no build-id in dump)) and one `same_build unconfirmed(no build-id in dump)` summary
                            line (13.4); every other line the same, the `chain` verdict line included
  c1.dump.txt, c5.dump.txt, c1.plain.txt, c5.plain.txt, abi32.plain.txt, abi64.plain.txt
                            no difference (the text of a 1.4 dump is the same, 13.3)
Prints one line per file and a summary; rc 0 when nothing else differs, 1 otherwise, 2 on a usage error."""
import os
import re
import sys

ALLOW = {
    "rc.txt": [re.compile(r"kz-rt-dump [0-9a-f]{64} kz [0-9a-f]{64} kz-dump [0-9a-f]{64} chain_check [0-9a-f]{64}")],
    "c1.run.kzt.txt": [re.compile(r"tool_sha256=[0-9a-f]{64}"), re.compile(r"artifact_sha256=[0-9a-f]{64}")],
    "c5.run.kzt.txt": [re.compile(r"tool_sha256=[0-9a-f]{64}"), re.compile(r"artifact_sha256=[0-9a-f]{64}")],
    "c1.convert.log": [re.compile(r"wrote <OUT>/c1\.run\.kzt artifact_sha256 [0-9a-f]{64}")],
    "c5.convert.log": [re.compile(r"wrote <OUT>/c5\.run\.kzt artifact_sha256 [0-9a-f]{64}")],
}
ADDED = {"c1.convert.log": re.compile(r"build-id records: none \(kzrt-dump 1\.4\)"),
         "c5.convert.log": re.compile(r"build-id records: none \(kzrt-dump 1\.4\)")}
FILES = ["rc.txt", "c1.dump.txt", "c1.convert.log", "c1.run.kzt.txt", "c5.dump.txt", "c5.convert.log", "c5.run.kzt.txt",
         "c1.plain.txt", "c5.plain.txt", "abi32.plain.txt", "abi64.plain.txt"]
BIN = ["c1.run.kzt", "c5.run.kzt"]
LO, HI = 120, 156


def lines(p):
    with open(p, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    old, new = sys.argv[1:]
    bad = 0
    for name in FILES:
        a, b = lines(os.path.join(old, name)), lines(os.path.join(new, name))
        added = 0
        if name in ADDED:
            keep = [x for x in b if not ADDED[name].fullmatch(x)]
            added = len(b) - len(keep)
            b = keep
        diffs, allowed = [], 0
        if len(a) != len(b):
            diffs.append("line count %d / %d" % (len(a), len(b)))
        for i in range(min(len(a), len(b))):
            if a[i] == b[i]:
                continue
            if any(p.fullmatch(a[i]) and p.fullmatch(b[i]) for p in ALLOW.get(name, [])):
                allowed += 1
                continue
            diffs.append(i + 1)
        if name in ADDED and added != 1:
            diffs.append("added build-id lines %d (1 expected)" % added)
        print("%s %s: lines %d, allowed differences %d, added lines %d, other differences %d%s" % (
            "OK" if not diffs else "BAD", name, len(b) + added, allowed, added, len(diffs), "" if not diffs else " (%s)" % diffs[:10]))
        bad += len(diffs)
    for name in BIN:
        with open(os.path.join(old, name), "rb") as f:
            a = f.read()
        with open(os.path.join(new, name), "rb") as f:
            b = f.read()
        if len(a) != len(b):
            print("BAD %s: size %d / %d" % (name, len(a), len(b)))
            bad += 1
            continue
        d = [i for i in range(len(a)) if a[i] != b[i]]
        out = [i for i in d if not LO <= i < HI]
        print("%s %s: size %d, differing bytes %d (offsets %s), outside tool_sha256 and header_crc %d" % (
            "OK" if not out else "BAD", name, len(a), len(d), "%d..%d" % (d[0], d[-1]) if d else "-", len(out)))
        bad += len(out)
    # the chain check: the same_build line replaced by the object lines and the summary
    a, b = lines(os.path.join(old, "c1.chain.txt")), lines(os.path.join(new, "c1.chain.txt"))
    a_rest = [x for x in a if not x.startswith("same_build\t")]
    b_obj = [x for x in b if x.startswith("same_build_object\t")]
    b_sum = [x for x in b if x.startswith("same_build\t")]
    b_rest = [x for x in b if not x.startswith(("same_build_object\t", "same_build\t"))]
    ok = (a_rest == b_rest and len([x for x in a if x.startswith("same_build\t")]) == 1 and len(b_sum) == 1 and b_obj
          and all(x.split("\t")[1] == "unconfirmed(no build-id in dump)" for x in b_obj)
          and b_sum[0].split("\t")[1] == "unconfirmed(no build-id in dump)")
    print("%s c1.chain.txt: other lines %d the same: %s; object lines %d; summary %s" % (
        "OK" if ok else "BAD", len(b_rest), a_rest == b_rest, len(b_obj), b_sum[0].split("\t")[1] if b_sum else "-"))
    bad += 0 if ok else 1
    print("compare_regress: %s" % ("only declared differences" if bad == 0 else "%d undeclared differences" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

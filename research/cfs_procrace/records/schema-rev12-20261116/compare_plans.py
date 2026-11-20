#!/usr/bin/env python3
"""compare_plans.py OLD NEW -- the plan of one build from kz-static b52db538 (OLD, order_rev1b_plans/<b>) against that of the
kz-static of schema revision 12 (NEW, schema12_plans/<b>), file by file, with the differences the declaration allows
(plan/full_stage_results_20261116.md 4.0): the generator's identity line of the sidecar (line 2: `generator tool=kz-static
tool_sha256=... tool_source=... core_source=...`), the sidecar sha256 in the manifest (line 2) and the first line of the
module table and of the executable list (the sidecar's path and sha256); in args.txt the output directory.  Every other
line must be the same.  Prints one line per file and a summary; rc 0 when nothing else differs, 1 otherwise."""
import os
import re
import sys

ALLOW = {
    "plan.side": {2: re.compile(r"^generator tool=kz-static tool_sha256=[0-9a-f]{64} tool_source=[0-9a-f]{64} core_source=[0-9a-f]{64}$")},
    "plan.kzs": {2: re.compile(r"^# sidecar sha256=[0-9a-f]{64}$")},
    "plan.mod": {1: re.compile(r"^# kz module table; plan sidecar \S+ sha256=[0-9a-f]{64}$")},
    "plan.exe": {1: re.compile(r"^# kz executables to check; plan sidecar \S+ sha256=[0-9a-f]{64}$")},
}


def lines(path):
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 64
    old, new = sys.argv[1], sys.argv[2]
    bad = 0
    for name in ("plan.kzs", "plan.side", "plan.mod", "plan.exe", "plan.log", "args.txt"):
        a, b = lines(os.path.join(old, name)), lines(os.path.join(new, name))
        if name == "args.txt":
            a = [x.replace(old.rstrip("/"), "<OUT>") for x in a]
            b = [x.replace(new.rstrip("/"), "<OUT>") for x in b]
        diffs, allowed = [], 0
        for i in range(max(len(a), len(b))):
            x = a[i] if i < len(a) else None
            y = b[i] if i < len(b) else None
            if x == y:
                continue
            rule = ALLOW.get(name, {}).get(i + 1)
            if rule is not None and x is not None and y is not None and rule.match(x) and rule.match(y):
                allowed += 1
                continue
            diffs.append(i + 1)
        print("%s %s: lines %d, allowed differences %d, other differences %d%s" % (
            "OK" if not diffs else "BAD", name, len(b), allowed, len(diffs), "" if not diffs else " (lines %s)" % diffs[:10]))
        bad += len(diffs)
    print("compare_plans: %s" % ("only declared differences" if bad == 0 else "%d undeclared differences" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

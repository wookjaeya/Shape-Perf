#!/usr/bin/env python3
"""compare_plans.py MODE OLD NEW -- (copied from records/extract-fixes-20261116 for the review fixes) the plan of one build from the kz-static before (OLD) against the
new one's (NEW), file by file, with the differences the declaration allows (plan/full_stage_results_20261116.md 5.0).
A differing line pair is allowed when both lines match one of the patterns of its file:
  both modes   sidecar `generator tool=kz-static tool_sha256=.. tool_source=.. core_source=..`; sidecar
               `rules plan rules revision 1 (2026-11-16); relations rules revision N (2026-11-16)` and the manifest's first
               line with that revision; the manifest's `# sidecar sha256=..`; the first line of the module table and of
               the executable list (the sidecar's path and sha256); in args.txt the output directory
  extract      (MODE extract: OLD on the earlier re-extraction's facts, NEW on the new re-extraction) also the sidecar's `input facts`,
               `input basis`, `facts-artifact`, `basis-artifact` lines, the manifest's `source build rev=basis-..` line, and
               in args.txt the facts and basis paths
Every other line must be the same, and the line counts.  Prints one line per file and a summary; rc 0 when nothing else
differs, 1 otherwise, 2 on a usage error."""
import os
import re
import sys

H = r"[0-9a-f]{64}"
BOTH = {
    "plan.side": [r"generator tool=kz-static tool_sha256=%s tool_source=%s core_source=%s" % (H, H, H),
                  r"rules plan rules revision 1 \(2026-11-16\); relations rules revision [0-9] \(2026-11-16\)"],
    "plan.kzs": [r"# kz plan written by kz-static plan \(plan rules revision 1 \(2026-11-16\); relations rules revision [0-9] \(2026-11-16\)\)",
                 r"# sidecar sha256=%s" % H],
    "plan.mod": [r"# kz module table; plan sidecar \S+ sha256=%s" % H],
    "plan.exe": [r"# kz executables to check; plan sidecar \S+ sha256=%s" % H],
}
EXTRACT = {
    "plan.side": [r"input facts sha256=%s path=\S+" % H, r"input basis sha256=%s path=\S+" % H, r"facts-artifact %s" % H, r"basis-artifact %s" % H],
    "plan.kzs": [r"source build rev=basis-%s ref=.*" % H],
    "args.txt": [r"\S+/facts\.kz", r"\S+/basis\.kz"],
}


def lines(path):
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in ("t8cf", "extract"):
        print(__doc__, file=sys.stderr)
        return 2
    mode, old, new = sys.argv[1:]
    bad = 0
    for name in ("plan.kzs", "plan.side", "plan.mod", "plan.exe", "plan.log", "args.txt"):
        a, b = lines(os.path.join(old, name)), lines(os.path.join(new, name))
        if name == "args.txt":
            a = [x.replace(old.rstrip("/"), "<OUT>") for x in a]
            b = [x.replace(new.rstrip("/"), "<OUT>") for x in b]
        pats = [re.compile(p) for p in BOTH.get(name, []) + (EXTRACT.get(name, []) if mode == "extract" else [])]
        diffs, allowed = [], 0
        if len(a) != len(b):
            diffs.append("line count %d / %d" % (len(a), len(b)))
        for i in range(min(len(a), len(b))):
            x, y = a[i], b[i]
            if x == y:
                continue
            if any(p.fullmatch(x) and p.fullmatch(y) for p in pats):
                allowed += 1
                continue
            diffs.append(i + 1)
        print("%s %s: lines %d, allowed differences %d, other differences %d%s" % (
            "OK" if not diffs else "BAD", name, len(b), allowed, len(diffs), "" if not diffs else " (%s)" % diffs[:10]))
        bad += len(diffs)
    print("compare_plans: %s" % ("only declared differences" if bad == 0 else "%d undeclared differences" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

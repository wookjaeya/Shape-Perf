#!/usr/bin/env python3
"""remeasure.py -- every sha256, build-id and plan-id row of a boot declaration measured again now (results 26.4; results
26.0 (h); spec 11 item 14), by the rules of run_cycle.sh verify: a path is absolute, research/... from the repository
root, records/... from the research root, else under the work directory; a sha256 row re-hashes the file; a row whose
value has ' build-id ' also reads the ELF's GNU build-id with readelf -n; a plan-id row reads the plan-id line of the
same build's plan.side row.  Reads only; one output line per row with declared, measured and ok|differs|missing.

    remeasure.py DECLARATION RESEARCH_ROOT WORK_DIR OUT
"""
import hashlib
import os
import subprocess
import sys


def main(argv):
    if len(argv) != 4:
        print(__doc__, file=sys.stderr)
        return 64
    decl, R, O, out = argv
    repo = os.path.normpath(os.path.join(R, "..", ".."))
    rows = []
    for n, l in enumerate(open(decl, encoding="utf-8"), 1):
        l = l.rstrip("\n")
        if not l or l.startswith("#"):
            continue
        rows.append((n, l.split("\t")))

    def resolve(p):
        if p.startswith("/"):
            return p
        if p.startswith("research/"):
            return os.path.join(repo, p)
        if p.startswith("records/"):
            return os.path.join(R, p)
        return os.path.join(O, p)
    side = {r[1]: r[4] for _, r in rows if r[0] == "plan" and r[2] == "plan.side"}
    res, cnt = [], {"ok": 0, "differs": 0, "missing": 0}
    for n, r in rows:
        kind, build, name, val, path = r[:5]
        if kind == "plan-id":
            f = resolve(side.get(build, "-"))
            got = "missing"
            if os.path.isfile(f):
                got = next((x.split()[1] for x in open(f, encoding="utf-8", errors="replace") if x.startswith("plan-id ")), "missing")
        else:
            f = resolve(path)
            if not os.path.isfile(f):
                got = "missing"
            else:
                with open(f, "rb") as fh:
                    got = hashlib.sha256(fh.read()).hexdigest()
                if " build-id " in val:
                    t = subprocess.run(["readelf", "-n", f], capture_output=True, text=True).stdout
                    bid = next((x.split("Build ID: ")[1].strip() for x in t.splitlines() if "Build ID: " in x), "")
                    got = "%s build-id %s" % (got, bid)
        st = "missing" if got == "missing" else "ok" if got == val else "differs"
        cnt[st] += 1
        res.append("\t".join([str(n), kind, build, name, path, val, got, st]))
    with open(out, "w", encoding="utf-8") as fh:
        fh.write("# remeasure.py: %s (sha256 %s) measured again\n" % (decl, hashlib.sha256(open(decl, "rb").read()).hexdigest()))
        fh.write("line\tkind\tbuild\tname\tpath\tdeclared\tmeasured\tstate\n")
        fh.write("\n".join(res) + "\n")
        fh.write("rows %d ok %d differs %d missing %d\n" % (len(res), cnt["ok"], cnt["differs"], cnt["missing"]))
    print("rows %d ok %d differs %d missing %d" % (len(res), cnt["ok"], cnt["differs"], cnt["missing"]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

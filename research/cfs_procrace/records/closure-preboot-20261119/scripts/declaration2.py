#!/usr/bin/env python3
"""declaration2.py -- writes boot/declaration2.tsv, the boot declaration of the C1 cycle that supersedes declaration.tsv of
860c2cd (results 26.4; results 26.0 (a), (h); protocol 14.0, 14.8; spec 11 items 14-16), and the list of what changed.
Run with python3 -I.  Every value is measured now (sha256 of the file; a build-id row also reads the GNU build-id with
readelf -n; a plan-id row copies the old value, which the verify step reads from plan.side).

    declaration2.py OLD_DECLARATION REMEASURE.tsv HEADER.txt OUT_DECLARATION OUT_CHANGES

Rows: every row of the old declaration (its value measured now; a row whose file changed by a committed change keeps
its place with the new value, and the change list names the commit), then the rows the old one lacked (NEW below).
A row of the old declaration that is missing now stops the script.
"""
import hashlib
import os
import subprocess
import sys

R = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
REPO = os.path.normpath(os.path.join(R, "..", ".."))
O = "/home/user/work/redesign/obs1"
SEM = "research/cfs_procrace/kairos/semantics"
P = "records/closure-c1prep-20261119"
NEW = [  # kind, build, name, path, why
    ("frozen", "*", "eval", "research/cfs_procrace/env/obs1/explain_eval.py", "spec 11 item 13 (26.0 (e)): the falsifier and evaluation script"),
    ("frozen", "*", "eval_fixtures", "research/cfs_procrace/env/obs1/tests/test_explain_eval.py", "its fixtures (92 checks)"),
    ("frozen", "*", "freeze2.tsv", "records/closure-preboot-20261119/freeze.tsv", "the new freeze (results 26.4); freeze.tsv of revision 4 stays listed above"),
    ("explain-input", "*", "reason_vocabulary.txt", "research/cfs_procrace/kairos/tests/static/data/reason_vocabulary.txt", "26.0 (h): the vocabulary the judgement part reads"),
    ("explain-input", "*", "framework_namespaces2_cfe-6.4.1.txt", SEM + "/framework_namespaces2_cfe-6.4.1.txt", "26.0 (h): a namespace data file the sidecars name (input namespace-data)"),
    ("explain-input", "*", "framework_namespaces2_cfe-6.4.2.txt", SEM + "/framework_namespaces2_cfe-6.4.2.txt", "26.0 (h)"),
    ("explain-input", "*", "framework_namespaces2_osal-4.1.1.txt", SEM + "/framework_namespaces2_osal-4.1.1.txt", "26.0 (h)"),
    ("explain-input", "*", "framework_namespaces2_osal-7.0.1.txt", SEM + "/framework_namespaces2_osal-7.0.1.txt", "26.0 (h)"),
    ("explain-input", "*", "framework_namespaces2_osal-f34759d.txt", SEM + "/framework_namespaces2_osal-f34759d.txt", "26.0 (h)"),
    ("plan-input", "*", "framework_api_roles.txt", SEM + "/framework_api_roles.txt", "the sidecars' input framework-layer-data line (a plan input; no boot step reads it)"),
    ("cycle", "*", "named_calls.tsv", P + "/candidates/named_calls.tsv", "spec 11 item 2: the named-calls file the explain step reads"),
    ("cycle", "*", "named_calls.py", P + "/scripts/named_calls.py", "the script that wrote it"),
    ("cycle", "*", "expect_c1_2.tsv", P + "/expect/expect_c1_2.tsv", "the expectations rewritten from the rules (results 26.4); read by the expect step and by F1 of the eval step only"),
    ("cycle", "*", "predictions2.tsv", P + "/expect/predictions2.tsv", "the hand-written boot predictions behind it"),
    ("cycle", "*", "elements_by_rule_2.tsv", P + "/expect/elements_by_rule_2.tsv", "the element values by rule written with it"),
    ("cycle", "*", "expect_write2.py", P + "/scripts/expect_write2.py", "the script that wrote it"),
    ("cycle", "*", "static_elements_16.tsv", P + "/scratch/branches16/static_elements_16.tsv", "the committed basis of 25.11 it reads"),
    ("cycle", "*", "commands.txt", P + "/boot/commands.txt", "the boot commands (verify first)"),
    ("cycle", "*", "test_run_cycle.sh", P + "/boot/test_run_cycle.sh", "the run script's tests (7/7, results 26.4)"),
    ("cycle", "*", "no_expect_input_check2.txt", P + "/boot/no_expect_input_check2.txt", "spec 11 item 15: the expectation files are no input but of the expect step and F1"),
]
WHY = {  # the committed change behind a row whose file changed after 860c2cd
    "run_cycle.sh": "dc879c6 (spec 11 items 14-15: verify, eval, named calls, uses) and results 26.4 (declaration2.tsv, expect_c1_2.tsv)",
    "reader": "dc879c6 (spec 11 item 2)", "judge": "dc879c6 (spec 11 items 1, 3, 4, 5, 6)", "summary": "dc879c6 (spec 11 items 7-9, 11)",
    "expect": "dc879c6 (spec 11 item 12)", "fixtures": "dc879c6 (spec 11 items 1-16)",
    "space_guard.sh": "dc879c6 (results 26.0 (j): below_floor_at_end, rc 77; registry rows of TOOLS.tsv)",
}


def resolve(p):
    if p.startswith("/"):
        return p
    if p.startswith("research/"):
        return os.path.join(REPO, p)
    if p.startswith("records/"):
        return os.path.join(R, p)
    return os.path.join(O, p)


def measure(path, with_bid):
    f = resolve(path)
    if not os.path.isfile(f):
        return None
    with open(f, "rb") as fh:
        v = hashlib.sha256(fh.read()).hexdigest()
    if with_bid:
        t = subprocess.run(["readelf", "-n", f], capture_output=True, text=True).stdout
        v += " build-id " + next((x.split("Build ID: ")[1].strip() for x in t.splitlines() if "Build ID: " in x), "")
    return v


def main(argv):
    if len(argv) != 5:
        print(__doc__, file=sys.stderr)
        return 64
    old, rem, head, out, chg = argv
    for p in (out, chg):
        if os.path.exists(p):
            print("declaration2.py: %s exists" % p, file=sys.stderr)
            return 64
    remv = {}
    for l in open(rem, encoding="utf-8"):
        r = l.rstrip("\n").split("\t")
        if len(r) == 8 and r[0].isdigit():
            remv[int(r[0])] = r
    rows, changes = [], []
    for n, l in enumerate(open(old, encoding="utf-8"), 1):
        l = l.rstrip("\n")
        if not l or l.startswith("#"):
            continue
        kind, build, name, val, path = l.split("\t")[:5]
        if kind == "plan-id":
            rows.append([kind, build, name, val, path])
            continue
        now = measure(path, " build-id " in val)
        if now is None:
            print("declaration2.py: %s is missing now" % path, file=sys.stderr)
            return 1
        if remv.get(n, [None] * 8)[6] != now:
            print("declaration2.py: line %d measured differently from the re-measure file" % n, file=sys.stderr)
            return 1
        if now != val:
            if name not in WHY:
                print("declaration2.py: %s changed and no committed change explains it" % name, file=sys.stderr)
                return 1
            changes.append([str(n), kind, name, path, val, now, "changed", WHY[name]])
        rows.append([kind, build, name, now, path])
    for kind, build, name, path, why in NEW:
        now = measure(path, False)
        if now is None:
            print("declaration2.py: new row %s is missing" % path, file=sys.stderr)
            return 1
        rows.append([kind, build, name, now, path])
        changes.append(["-", kind, name, path, "-", now, "added", why])
    with open(out, "w", encoding="utf-8") as f:
        f.write(open(head, encoding="utf-8").read())
        f.write("# kind\tbuild\tname\tsha256 or value\tpath\n")
        for r in rows:
            f.write("\t".join(r) + "\n")
    with open(chg, "w", encoding="utf-8") as f:
        f.write("# declaration2.py: boot/declaration2.tsv against boot/declaration.tsv of 860c2cd (rows of the old one not listed "
                "here are the same)\n")
        f.write("old_line\tkind\tname\tpath\told\tnew\tchange\twhy\n")
        for r in changes:
            f.write("\t".join(r) + "\n")
        f.write("rows %d (old %d, added %d; changed %d)\n" % (len(rows), len(rows) - len(NEW), len(NEW),
                                                             sum(1 for c in changes if c[6] == "changed")))
    print("declaration2: rows %d (old %d, added %d; changed %d)" % (len(rows), len(rows) - len(NEW), len(NEW),
                                                                    sum(1 for c in changes if c[6] == "changed")))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

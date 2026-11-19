#!/usr/bin/env python3
"""declaration4.py -- writes boot/declaration4.tsv, the boot declaration of the C1 cycle that supersedes declaration3.tsv
of 13cf028 (results 26.9; evaluation protocol 14.14 item 8; spec 11.6 item 4), the re-measure of every old row and the
list of what changed.  A copy of records/closure-prebootfix-20261119/scripts/declaration3.py with this docstring, the
rows it adds, the commits behind the changed rows, the count (every entry that is not a directory, as find ! -type d
counts it; protocol 14.14 item 7) and the need rows (read from records/closure-space3-20261119/need.txt, never typed).
Run with python3 -I.  Every value is measured now (sha256 of the file; a build-id row also reads the GNU build-id with
readelf -n; a plan-id row copies the old value, which the verify step reads from plan.side).

    declaration4.py OLD_DECLARATION HEADER.txt OUT_DECLARATION OUT_REMEASURE OUT_CHANGES

Rows: every row of the old declaration (its value measured now; a row whose file changed by a committed change keeps its
place with the new value, and the change list names the commit), then the rows the old one lacked (NEW below).  A row of
the old declaration that is missing now, or changed with no committed change behind it, stops the script.
"""
import hashlib
import os
import re
import subprocess
import sys

R = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
REPO = os.path.normpath(os.path.join(R, "..", ".."))
O = "/home/user/work/redesign/obs1"
P = "records/closure-c1prep-20261119"
NONELF = ["cfe_es_startup.scr", "bp_flowtable.tbl", "cf_def_config.tbl", "sch_lab_table.tbl", "to_lab_sub.tbl"]
NEED = os.path.join(R, "records/closure-space3-20261119/need.txt")
NEW = []
for name, key in (("floor", "floor"), ("margin", "margin"), ("per boot", "per_boot")):
    NEW.append(("need", "*", name, "need:" + key, "protocol 14.14 item 4: read from records/closure-space3-20261119/need.txt"))
NEW += [("tool", "*", "ledger_check.sh", "research/cfs_procrace/env/tools/ledger_check.sh", "protocol 14.14 item 1: called by the keep step on the proposed ledger"),
        ("cycle", "*", "need.txt", "records/closure-space3-20261119/need.txt", "protocol 14.14 item 4: the per-boot need from kept measurements, the source of the need rows"),
        ("cycle", "*", "no_expect_input_check4.txt", P + "/boot/no_expect_input_check4.txt", "spec 11 item 15 again for the run script of results 26.9")]
WHY = {  # the committed change behind a row whose file changed after 13cf028
    "run_cycle.sh": "adfc67d (spec 11.6 item 1: declaration4.tsv, count of non-directory entries, need rows, eval under the work directory, keep, write once, not booted, space need, pin, verify again)",
    "commands.txt": "adfc67d (spec 11.6 item 3: twelve steps, declaration4.tsv, the space need and the rules of protocol 14.14)",
    "test_run_cycle.sh": "adfc67d and 66397fc (spec 11.6 item 2: 26 checks)",
}


def resolve(p):
    if p.startswith("/"):
        return p
    if p.startswith("research/"):
        return os.path.join(REPO, p)
    if p.startswith("records/"):
        return os.path.join(R, p)
    return os.path.join(O, p)


def need_value(key):
    for l in open(NEED, encoding="utf-8"):
        m = re.match(r"^%s (\d+)(?: |$)" % re.escape(key), l)
        if m:
            return m.group(1)
    return None


def measure(kind, path, with_bid):
    if kind == "need":
        return need_value(path.split(":", 1)[1])
    f = resolve(path)
    if kind == "count":
        if not os.path.isdir(f):
            return None
        n = 0
        for d, ds, fs in os.walk(f):
            n += len(fs) + sum(1 for x in ds if os.path.islink(os.path.join(d, x)))
        return str(n)
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
    old, head, out, rem, chg = argv
    for p in (out, rem, chg):
        if os.path.exists(p):
            print("declaration4.py: %s exists" % p, file=sys.stderr)
            return 64
    rows, changes, remv = [], [], []
    for n, l in enumerate(open(old, encoding="utf-8"), 1):
        l = l.rstrip("\n")
        if not l or l.startswith("#"):
            continue
        kind, build, name, val, path = l.split("\t")[:5]
        if kind == "plan-id":
            rows.append([kind, build, name, val, path])
            remv.append([str(n), kind, build, name, path, val, val, "copied (verify reads it from plan.side)"])
            continue
        now = measure(kind, path, " build-id " in val)
        if now is None:
            print("declaration4.py: %s is missing now" % path, file=sys.stderr)
            return 1
        remv.append([str(n), kind, build, name, path, val, now, "same" if now == val else "changed"])
        if now != val:
            if name not in WHY:
                print("declaration4.py: %s changed and no committed change explains it" % name, file=sys.stderr)
                return 1
            changes.append([str(n), kind, name, path, val, now, "changed", WHY[name]])
        rows.append([kind, build, name, now, path])
    for kind, build, name, path, why in NEW:
        now = measure(kind, path, False)
        if now is None:
            print("declaration4.py: new row %s is missing" % path, file=sys.stderr)
            return 1
        rows.append([kind, build, name, now, "-" if kind == "need" else path])
        changes.append(["-", kind, name, path, "-", now, "added", why])
    with open(out, "w", encoding="utf-8") as f:
        f.write(open(head, encoding="utf-8").read())
        f.write("# kind\tbuild\tname\tsha256 or value\tpath\n")
        for r in rows:
            f.write("\t".join(r) + "\n")
    with open(rem, "w", encoding="utf-8") as f:
        f.write("# declaration4.py: every row of boot/declaration3.tsv (13cf028) measured again\n")
        f.write("old_line\tkind\tbuild\tname\tpath\tdeclared\tmeasured\tstate\n")
        for r in remv:
            f.write("\t".join(r) + "\n")
        f.write("rows %d same %d changed %d copied %d\n" % (len(remv), sum(1 for r in remv if r[7] == "same"),
                                                          sum(1 for r in remv if r[7] == "changed"), sum(1 for r in remv if r[7].startswith("copied"))))
    with open(chg, "w", encoding="utf-8") as f:
        f.write("# declaration4.py: boot/declaration4.tsv against boot/declaration3.tsv of 13cf028 (rows of the old one not "
                "listed here are the same)\n")
        f.write("old_line\tkind\tname\tpath\told\tnew\tchange\twhy\n")
        for r in changes:
            f.write("\t".join(r) + "\n")
        f.write("rows %d (old %d, added %d; changed %d)\n" % (len(rows), len(rows) - len(NEW), len(NEW),
                                                             sum(1 for c in changes if c[6] == "changed")))
    print("declaration4: rows %d (old %d, added %d; changed %d)" % (len(rows), len(rows) - len(NEW), len(NEW),
                                                                    sum(1 for c in changes if c[6] == "changed")))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

#!/usr/bin/env python3
"""declaration3.py -- writes boot/declaration3.tsv, the boot declaration of the C1 cycle that supersedes declaration2.tsv
of 549dc94 (results 26.5; evaluation protocol 14.13 item 9; spec 11.5 item 4), the re-measure of every old row and the
list of what changed.  Run with python3 -I.  Every value is measured now (sha256 of the file; a build-id row also reads
the GNU build-id with readelf -n; a plan-id row copies the old value, which the verify step reads from plan.side; a
count row is the number of regular files under the directory).

    declaration3.py OLD_DECLARATION HEADER.txt OUT_DECLARATION OUT_REMEASURE OUT_CHANGES

Rows: every row of the old declaration (its value measured now; a row whose file changed by a committed change keeps its
place with the new value, and the change list names the commit), then the rows the old one lacked (NEW below).  A row of
the old declaration that is missing now, or changed with no committed change behind it, stops the script.
"""
import hashlib
import os
import subprocess
import sys

R = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
REPO = os.path.normpath(os.path.join(R, "..", ".."))
O = "/home/user/work/redesign/obs1"
P = "records/closure-c1prep-20261119"
NONELF = ["cfe_es_startup.scr", "bp_flowtable.tbl", "cf_def_config.tbl", "sch_lab_table.tbl", "to_lab_sub.tbl"]
NEW = [("frozen", "*", "freeze3.tsv", "records/closure-prebootfix-20261119/freeze.tsv",
        "the third freeze (results 26.5); the two earlier freeze files stay listed above")]
for b, s in (("c1_pre_tx", "pre"), ("c1_post_tx", "post")):
    for inst in ("rx", "tx"):
        for f in NONELF:
            NEW.append(("installed", b, "exe/%s/cf/%s" % (inst, f), "c1p2_%s_derived/ws/exe/%s/cf/%s" % (s, inst, f),
                        "pre-builds-02: a non-ELF installed file the boot copies into the instance (the startup script fixes the start order)"))
    NEW.append(("count", b, "ws/exe files", "c1p2_%s_derived/ws/exe" % s,
                "pre-builds-02: the number of regular files the boot runner tars into the instance (an added file is caught)"))
    NEW.append(("derived", b, "install_inventory.tsv", "c1p2_%s_derived/install_inventory.tsv" % s,
                "pre-builds-02: the derived build's inventory of the installed files (read by no boot step; pinned with them)"))
NEW += [("tool", "*", "hash_instance.sh", "research/cfs_procrace/env/tools/hash_instance.sh", "pre-builds-02: called by run_gi_c1.sh (the instance file list the chain link reads)"),
        ("tool", "*", "persist_snapshot.sh", "research/cfs_procrace/env/tools/persist_snapshot.sh", "pre-builds-02: called by run_gi_c1.sh before the start and after the stop"),
        ("cycle", "*", "predictions3.tsv", P + "/expect/predictions3.tsv", "protocol 14.13 item 8: the predictions with the no-loss precondition (values of predictions2.tsv)"),
        ("cycle", "*", "no_expect_input_check3.txt", P + "/boot/no_expect_input_check3.txt", "spec 11 item 15 again for the run script of results 26.5")]
WHY = {  # the committed change behind a row whose file changed after 549dc94
    "run_cycle.sh": "ef6d38f (spec 11.5 item 4: declaration3.tsv, count rows, the boot step's commit check and verify, loss records named)",
    "judge": "ef6d38f (spec 11.5 item 1)", "eval": "ef6d38f (spec 11.5 item 3: revision 2)",
    "fixtures": "ef6d38f (spec 11.5 items 1-2)", "eval_fixtures": "ef6d38f (spec 11.5 item 3)",
    "commands.txt": "ef6d38f (spec 11.5 item 4: the header names declaration3.tsv)",
    "test_run_cycle.sh": "ef6d38f (spec 11.5 item 4: 11 checks with a git stub)",
}


def resolve(p):
    if p.startswith("/"):
        return p
    if p.startswith("research/"):
        return os.path.join(REPO, p)
    if p.startswith("records/"):
        return os.path.join(R, p)
    return os.path.join(O, p)


def measure(kind, path, with_bid):
    f = resolve(path)
    if kind == "count":
        if not os.path.isdir(f):
            return None
        return str(sum(len([x for x in fs if os.path.isfile(os.path.join(d, x)) and not os.path.islink(os.path.join(d, x))])
                       for d, _, fs in os.walk(f)))
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
            print("declaration3.py: %s exists" % p, file=sys.stderr)
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
            print("declaration3.py: %s is missing now" % path, file=sys.stderr)
            return 1
        remv.append([str(n), kind, build, name, path, val, now, "same" if now == val else "changed"])
        if now != val:
            if name not in WHY:
                print("declaration3.py: %s changed and no committed change explains it" % name, file=sys.stderr)
                return 1
            changes.append([str(n), kind, name, path, val, now, "changed", WHY[name]])
        rows.append([kind, build, name, now, path])
    for kind, build, name, path, why in NEW:
        now = measure(kind, path, False)
        if now is None:
            print("declaration3.py: new row %s is missing" % path, file=sys.stderr)
            return 1
        rows.append([kind, build, name, now, path])
        changes.append(["-", kind, name, path, "-", now, "added", why])
    with open(out, "w", encoding="utf-8") as f:
        f.write(open(head, encoding="utf-8").read())
        f.write("# kind\tbuild\tname\tsha256 or value\tpath\n")
        for r in rows:
            f.write("\t".join(r) + "\n")
    with open(rem, "w", encoding="utf-8") as f:
        f.write("# declaration3.py: every row of boot/declaration2.tsv (549dc94) measured again\n")
        f.write("old_line\tkind\tbuild\tname\tpath\tdeclared\tmeasured\tstate\n")
        for r in remv:
            f.write("\t".join(r) + "\n")
        f.write("rows %d same %d changed %d copied %d\n" % (len(remv), sum(1 for r in remv if r[7] == "same"),
                                                          sum(1 for r in remv if r[7] == "changed"), sum(1 for r in remv if r[7].startswith("copied"))))
    with open(chg, "w", encoding="utf-8") as f:
        f.write("# declaration3.py: boot/declaration3.tsv against boot/declaration2.tsv of 549dc94 (rows of the old one not "
                "listed here are the same)\n")
        f.write("old_line\tkind\tname\tpath\told\tnew\tchange\twhy\n")
        for r in changes:
            f.write("\t".join(r) + "\n")
        f.write("rows %d (old %d, added %d; changed %d)\n" % (len(rows), len(rows) - len(NEW), len(NEW),
                                                             sum(1 for c in changes if c[6] == "changed")))
    print("declaration3: rows %d (old %d, added %d; changed %d)" % (len(rows), len(rows) - len(NEW), len(NEW),
                                                                    sum(1 for c in changes if c[6] == "changed")))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

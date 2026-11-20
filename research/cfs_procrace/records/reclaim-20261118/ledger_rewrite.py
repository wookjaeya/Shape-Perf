#!/usr/bin/env python3
# ledger_rewrite.py LEDGER SUMMARY LISTING_DIR TAG ROOT RESEARCH_DIR DATE OUT
#
# Writes OUT, a copy of the evidence ledger LEDGER in which the row of every candidate that SUMMARY
# (identity_summary_TAG.txt of identity_listing.py) calls identical-no-extra is turned into the row of a removed
# directory.  The ledger check (env/tools/ledger_check.sh) fails a row whose root is missing, and it checks a row
# by `sha256sum -c` of the row's list inside the row's root.  A removed directory that equals a kept one in every
# file therefore keeps its row and its own list, and the row is pointed at the kept directory:
#
#   root   the reference directory (the same relative paths hold the same bytes there, so the check still reads
#          every byte the list names)
#   local  "removed DATE; bytes in <reference row>"
#   note   the removed directory, the file count and bytes, the reference row, the listing, then the old note
#
# Before a row is changed the script checks, and stops with rc 1 when one does not hold:
#   the row exists once, has method sha256c and the root ROOT/<candidate>;
#   the reference row exists and has the root ROOT/<reference>;
#   the row's list names exactly the files of the candidate in the listing (no file more, no file less), with the
#   same sha256 as the listing's candidate column.
# A comment block that states this convention is added after the ledger's header comments.  Nothing else changes;
# a line per changed row and the count of unchanged lines go to standard output.  Refuses to overwrite OUT.
import os
import sys


def main(argv):
    if len(argv) != 9:
        sys.stderr.write("usage: ledger_rewrite.py LEDGER SUMMARY LISTING_DIR TAG ROOT RESEARCH_DIR DATE OUT\n")
        return 64
    ledger, summary, listing_dir, tag, root, research, date, out = argv[1:]
    if os.path.exists(out):
        sys.stderr.write("ledger_rewrite.py: output exists, not overwritten: %s\n" % out)
        return 1
    todo = {}
    with open(summary) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            c = line.rstrip("\n").split("\t")
            if c[-1] == "identical-no-extra":
                todo[c[2]] = dict(cand=c[0], ref=c[1], ref_row=c[3], files=int(c[4]), bytes=int(c[5]))
            else:
                print("not changed (not eligible): %s %s" % (c[2], c[-1]))
    with open(ledger) as f:
        lines = f.read().split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    rows = {}
    for i, line in enumerate(lines):
        if line.startswith("#") or not line:
            continue
        c = line.split("\t")
        if c[0] in rows:
            sys.stderr.write("ledger_rewrite.py: row id twice: %s\n" % c[0])
            return 1
        rows[c[0]] = (i, c)
    rec_rel = os.path.relpath(os.path.abspath(listing_dir), os.path.abspath(research))
    changed = 0
    for rid, t in sorted(todo.items(), key=lambda kv: rows.get(kv[0], (1 << 30,))[0]):
        if rid not in rows or t["ref_row"] not in rows:
            sys.stderr.write("ledger_rewrite.py: row missing: %s or %s\n" % (rid, t["ref_row"]))
            return 1
        i, c = rows[rid]
        rc = rows[t["ref_row"]][1]
        cdir, rdir = os.path.join(root, t["cand"]), os.path.join(root, t["ref"])
        if len(c) != 10 or c[2] != "sha256c" or c[4] != cdir or rc[4] != rdir or rc[2] != "sha256c":
            sys.stderr.write("ledger_rewrite.py: row %s or %s is not the expected sha256c row of its directory\n"
                             % (rid, t["ref_row"]))
            return 1
        lpath = c[3] if c[3].startswith("/") else os.path.join(research, c[3])
        in_list = {}
        with open(lpath) as f:
            for ln in f:
                ln = ln.rstrip("\n")
                if ln:
                    in_list[os.path.normpath(ln[66:])] = ln[:64]
        listing = os.path.join(listing_dir, "listing_%s_%s.txt" % (tag, t["cand"]))
        in_dir = {}
        with open(listing) as f:
            for ln in f:
                if ln.startswith("#"):
                    continue
                s = ln.rstrip("\n").split("\t")
                if s[0] != "same":
                    sys.stderr.write("ledger_rewrite.py: %s has a line that is not `same`\n" % listing)
                    return 1
                in_dir[os.path.normpath(s[1])] = s[3]
        if in_list != in_dir or len(in_dir) != t["files"]:
            sys.stderr.write("ledger_rewrite.py: the list of %s and the listing of %s do not name the same files "
                             "and hashes\n" % (rid, t["cand"]))
            return 1
        old_note = c[9]
        c[4] = rdir
        c[5] = "removed %s; bytes in %s" % (date, t["ref_row"])
        c[9] = ("directory %s removed %s (%d files, %d bytes): every file is byte-identical to the file of %s at "
                "the same path (%s/listing_%s_%s.txt); this row's list is checked in that directory. Before the "
                "removal: %s" % (cdir, date, t["files"], t["bytes"], t["ref_row"], rec_rel, tag, t["cand"], old_note))
        lines[i] = "\t".join(c)
        changed += 1
        print("changed: %s root %s -> %s (list %s: %d files, the files and hashes of the listing)"
              % (rid, cdir, rdir, rows[rid][1][3], len(in_list)))
    head_end = 0
    while head_end < len(lines) and lines[head_end].startswith("#"):
        head_end += 1
    block = [
        "#   A directory removed because every file of it is byte-identical to the file of a kept directory at the same path",
        "#   (first use %s, %s): the row and its list stay; root = the kept directory, so the check still reads every"
        % (date, rec_rel),
        "#   byte the list names; local = \"removed <date>; bytes in <row of the kept directory>\"; the note names the removed",
        "#   directory and the listing that showed the identity. A row is never pointed at a directory that such a row names.",
    ]
    lines[head_end:head_end] = block
    for rid, t in todo.items():
        if rows[t["ref_row"]][0] in [rows[r][0] for r in todo]:
            sys.stderr.write("ledger_rewrite.py: reference row %s is itself a removed row\n" % t["ref_row"])
            return 1
    with open(out, "x") as f:
        f.write("\n".join(lines) + "\n")
    print("rows changed %d; comment lines added %d; other lines unchanged %d"
          % (changed, len(block), len(lines) - changed - len(block)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

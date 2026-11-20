#!/usr/bin/env python3
"""name_stability.py PRE_VIEW POST_VIEW -- the type names with a position (docs/static_facts.md 5q, 5s) in the facts
views (kz-static show) of c1_pre_tx and c1_post_tx extracted with --object-access: does the same definition get the same
name in both builds?  A name with a position is a token "<name>@<file>:<line>[::type]" or "<anonymous@<file>:<line>:<col>>"
whose file part holds no dot (TuReader::name_pos writes '.' as '_'; a function or object key keeps its dots).  Its file
is looked up in the [files] section of the same view (FILE_HASHES: the recorded path and the first 12 hex digits of its
sha256): the recorded path whose spelling in a name (outside the root: "<tree>/<path in the tree>" with the tree of the
nearest directory above the file that holds a `.git` entry on this machine; '.' as '_') is the name's file part.
Declared (plan/full_stage_results_20261116.md 7.0): every name whose file has the same sha256 in both builds is in both
views; a name in one view only lies in a file whose content differs between the two builds (or in no file the [files]
section lists, reported).  Prints the counts and every name of one view only with its file and the two hashes; rc 0 when
the declaration holds, 1 otherwise."""
import os
import re
import sys

NAME = re.compile(r"(?<![\w@])(?:[A-Za-z_]\w*@(?:<[^>\s]+>)?[^\s.@<>]+:\d+(?:::type)?|<anonymous@(?:<[^>\s]+>)?[^\s.@<>]+:\d+:\d+>)")
FILE_LINE = re.compile(r"^  \d+ (\S+) ([0-9a-f]{12}) \d+$")


def tree_spelling(path):
    if not path.startswith("/"):
        return path.replace(".", "_")
    parts = path.split("/")
    for k in range(len(parts) - 1, 1, -1):
        d = "/".join(parts[:k])
        if d and os.path.lexists(os.path.join(d, ".git")):
            return ("<%s>/%s" % (parts[k - 1], "/".join(parts[k:]))).replace(".", "_")
    return path.replace(".", "_")


def load(path):
    names, files, sec = set(), {}, ""
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        for l in f:
            l = l.rstrip("\n")
            m = re.match(r"^\[([a-z_0-9]+)\]", l)
            if m:
                sec = m.group(1)
            if sec == "files":
                fm = FILE_LINE.match(l)
                if fm:
                    files[tree_spelling(fm.group(1))] = (fm.group(1), fm.group(2))
                continue
            for n in NAME.findall(l):
                names.add(n)
    return names, files


def file_part(name):
    m = re.match(r"^(?:<anonymous@|[A-Za-z_]\w*@)(.*?):\d+(?::\d+>|::type)?$", name)
    return m.group(1) if m else None


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    (na, fa), (nb, fb) = load(sys.argv[1]), load(sys.argv[2])
    both = na & nb
    only = [("pre", n) for n in sorted(na - nb)] + [("post", n) for n in sorted(nb - na)]
    bad = 0
    tree = sum(1 for n in na | nb if "@<" in n)
    absolute = sum(1 for n in na | nb if "@/" in n)
    print("names with a position: pre %d, post %d, in both %d, in one only %d (by source tree %d, absolute %d, relative to the root %d)" % (
        len(na), len(nb), len(both), len(only), tree, absolute, len(na | nb) - tree - absolute))
    for side, n in only:
        fp = file_part(n)
        a, b = fa.get(fp), fb.get(fp)
        if a is None or b is None:
            verdict = "file not listed in %s" % ("both" if a is None and b is None else ("pre" if a is None else "post"))
        elif a[1] == b[1]:
            verdict = "SAME FILE CONTENT: undeclared"
            bad += 1
        else:
            verdict = "file content differs (%s / %s)" % (a[1], b[1])
        print("  %s only: %s  [%s]" % (side, n, verdict))
    # the names in both views whose file is outside the root: their files in the two builds
    for n in sorted(both):
        fp = file_part(n)
        if fp is not None and (fp.startswith("<") or fp.startswith("/")):
            a, b = fa.get(fp), fb.get(fp)
            print("  both: %s  [pre %s, post %s]" % (n, a[0] if a else "-", b[0] if b else "-"))
    print("name_stability: %s" % ("declared" if bad == 0 else "%d names differ although their file is the same" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

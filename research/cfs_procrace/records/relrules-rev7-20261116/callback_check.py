#!/usr/bin/env python3
"""callback_check.py -- every (use, namespace) pair relations rules revision 7 leaves out, checked against the source:
the callback argument named by the "left out:" line is that argument of the call in the source file.  Run with python3 -I.

  callback_check.py RELATIONS BUNDLE_ROOT OUT

For each "left out:" line of a namespace block ("<use pos> in <caller> -> <callee>#<k> (lookup|creator): the callback
<X> passed as argument <j> at <pos> in <fn> is not the comparison's function <Y>", or "... passed as argument <j> of the
use ..."), the call at <pos> is read from the source file (a relative path is taken under BUNDLE_ROOT): from the line and
column of the call, the callee name, its parenthesised argument list split at the top-level commas (strings, characters,
comments and nested brackets skipped), and argument j, which must be the identifier X (casts and parentheses around it
allowed).  The namespace's comparison function Y must differ from X.  Writes one line per pair to OUT; rc 0 when every
pair matches, 1 otherwise.
"""
import os
import re
import sys

LEFT = re.compile(r"^      left out: (\S+) in (\S+) -> (\S+)#(\d+) \((lookup|creator)\): the callback (\S+) passed as argument (\d+) (?:at (\S+) in (\S+)|of the use) is not the comparison's function (\S+)$")
POS = re.compile(r"^(.*):(\d+):(\d+)$")


def args_at(text, off):
    """the top-level arguments of the call whose callee starts at `off`"""
    i = text.find("(", off)
    if i < 0:
        return None, None
    name = text[off:i].strip()
    depth, cur, out, j = 0, "", [], i + 1
    while j < len(text):
        c = text[j]
        if text.startswith("/*", j):
            e = text.find("*/", j + 2)
            j = e + 2 if e >= 0 else len(text)
            continue
        if text.startswith("//", j):
            e = text.find("\n", j)
            j = e if e >= 0 else len(text)
            continue
        if c in "\"'":
            e = j + 1
            while e < len(text) and text[e] != c:
                e += 2 if text[e] == "\\" else 1
            cur += text[j:e + 1]
            j = e + 1
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            if depth == 0:
                out.append(cur.strip())
                return name, out
            depth -= 1
        if c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += c
        j += 1
    return name, None


def bare(a):
    a = a.strip()
    while True:
        m = re.match(r"^\(\s*[A-Za-z_][\w\s\*]*\)\s*(.*)$", a)  # a cast
        if m and m.group(1):
            a = m.group(1).strip()
            continue
        if a.startswith("(") and a.endswith(")"):
            a = a[1:-1].strip()
            continue
        if a.startswith("&"):
            a = a[1:].strip()
            continue
        return a


def main():
    rel, root, out_path = sys.argv[1:4]
    ns = None
    results, bad = [], 0
    for l in open(rel, encoding="utf-8", errors="replace"):
        l = l.rstrip("\n")
        if l.startswith("  namespace "):
            ns = l.split()[1]
            continue
        m = LEFT.match(l)
        if not m:
            continue
        use, caller, callee, k, kind, cb, j, at, fn, cmpfn = m.groups()
        where = at if at else use
        pm = POS.match(where)
        path = pm.group(1) if os.path.isabs(pm.group(1)) else os.path.join(root, pm.group(1))
        line, colno = int(pm.group(2)), int(pm.group(3))
        verdict, detail = "MISMATCH", ""
        try:
            src = open(path, encoding="utf-8", errors="replace").read()
            starts = [0]
            for x in re.finditer("\n", src):
                starts.append(x.end())
            off = starts[line - 1] + colno - 1
            name, args = args_at(src, off)
            if args is None:
                detail = "no argument list at the position"
            elif int(j) >= len(args):
                detail = "%s has %d arguments" % (name, len(args))
            else:
                a = bare(args[int(j)])
                detail = "%s argument %s is %r" % (name, j, args[int(j)])
                if a == cb.split("@")[0] and cb.split("@")[0] != cmpfn.split("@")[0]:
                    verdict = "MATCH"
        except OSError as e:
            detail = "cannot read %s: %s" % (path, e)
        if verdict != "MATCH":
            bad += 1
        results.append("%s\t%s\t%s in %s -> %s#%s (%s)\tcallback %s passed at %s\tcomparison %s\t%s" % (verdict, ns, use, caller, callee, k, kind, cb, where, cmpfn, detail))
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("# verdict\tnamespace\tuse\tcallback and where it is passed\tthe namespace's comparison\tsource\n")
        f.write("\n".join(results) + ("\n" if results else ""))
    print("pairs %d, match %d, mismatch %d" % (len(results), len(results) - bad, bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

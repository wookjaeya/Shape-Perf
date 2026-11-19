#!/usr/bin/env python3
"""sections_cmp.py KZDUMP OLD NEW -- two containers (facts.kz or basis.kz) of one build read by the same kz-dump and
compared section by section by record text (the record lines of kz-dump, string references written as text; the
`section` line with its count and crc32 is not a record line), and their header lines (the lines before the first
section) apart.

Output, one line each:
  header same=<n> differing=<n> [<field>...]      header lines by their field name (the text before `=` or the first word)
  section <NAME> old=<n> new=<n> same             the same record lines in the same order
  section <NAME> old=<n> new=<n> differs removed=<r> added=<a> [prefix]
                                                  r record lines only in OLD, a only in NEW (as multisets); `prefix` when
                                                  OLD's record lines are the first lines of NEW's
  section <NAME> old=<n> new=- only-old | old=- new=<n> only-new
  count <NAME> old=<c> new=<c>                    for a section whose count field differs although no record line does
                                                  (STRINGS: the count is the byte size of the pool, it has no record line)
  summary sections old=<n> new=<n> same=<n> differing=<n> only-old=<n> only-new=<n>
It judges nothing: rc 0 when both containers were read, 2 otherwise.  The caller compares the output with the declared
one."""
import collections
import re
import subprocess
import sys


def read(kd, path):
    text = subprocess.run([kd, path], capture_output=True, check=True).stdout.decode("utf-8", "surrogateescape").split("\n")
    head, secs, counts, cur = [], collections.OrderedDict(), {}, None
    for l in text:
        if l.startswith("section "):
            cur = l.split()[1]
            secs[cur] = []
            m = re.search(r"\bcount=(\d+)", l)
            counts[cur] = m.group(1) if m else "?"
        elif cur is None:
            if l:
                head.append(l)
        elif l.startswith("  "):
            secs[cur].append(l)
    return head, secs, counts


def field(l):
    l = l.strip()
    m = re.match(r"([A-Za-z_][A-Za-z0-9_.\[\]]*)", l)
    return m.group(1) if m else l[:20]


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    kd, a, b = sys.argv[1:]
    try:
        ha, sa, ca = read(kd, a)
        hb, sb, cb = read(kd, b)
    except (OSError, subprocess.CalledProcessError) as e:
        print("cannot read: %s" % e, file=sys.stderr)
        return 2
    da = collections.Counter(ha)
    db = collections.Counter(hb)
    only = sorted(set(field(l) for l in (da - db)) | set(field(l) for l in (db - da)))
    print("header same=%d differing=%d %s" % (sum((da & db).values()), max(sum((da - db).values()), sum((db - da).values())), "[" + " ".join(only) + "]"))
    n_same = n_diff = n_old = n_new = 0
    for s in list(sa) + [x for x in sb if x not in sa]:
        if s not in sb:
            print("section %s old=%d new=- only-old" % (s, len(sa[s])))
            n_old += 1
        elif s not in sa:
            print("section %s old=- new=%d only-new" % (s, len(sb[s])))
            n_new += 1
        elif sa[s] == sb[s]:
            print("section %s old=%d new=%d same" % (s, len(sa[s]), len(sb[s])))
            n_same += 1
            if ca[s] != cb[s]:
                print("count %s old=%s new=%s" % (s, ca[s], cb[s]))
        else:
            x, y = collections.Counter(sa[s]), collections.Counter(sb[s])
            pre = " prefix" if sb[s][:len(sa[s])] == sa[s] else ""
            print("section %s old=%d new=%d differs removed=%d added=%d%s" % (s, len(sa[s]), len(sb[s]), sum((x - y).values()), sum((y - x).values()), pre))
            n_diff += 1
    print("summary sections old=%d new=%d same=%d differing=%d only-old=%d only-new=%d" % (len(sa), len(sb), n_same, n_diff, n_old, n_new))
    return 0


if __name__ == "__main__":
    sys.exit(main())

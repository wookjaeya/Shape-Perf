#!/usr/bin/env python3
"""cdiff.py RULES OLD NEW [--sections] [--samples FILE] [--explore] [--max N] -- two text outputs of one build (OLD: the kept output of
the earlier tool, NEW: the output of the rebuilt tool) compared line by line as multisets, every differing line put into a
declared class of the rules file or reported as `other`.

RULES: tab-separated lines (`#` comments), read in order:
  mask  CLASS  SECTION  REGEX  REPLACEMENT   a field that may differ: the regex is replaced in the lines that are still only
                                             in OLD or only in NEW (in the sections SECTION matches), then lines that became
                                             equal cancel in pairs; the class counts the pairs.  Masks add up: a line that
                                             cancels only after two masks is counted by the second
  del   CLASS  SECTION  REGEX                a line of OLD with no counterpart that may be gone
  add   CLASS  SECTION  REGEX                a line of NEW with no counterpart that may be new
SECTION is a regex matched against the whole section name (`.*` for any).  With --sections the text is a `kz-static show`
view: a line `[name] ...` starts the section `name`, and lines cancel only within their section; without it there is one
section `-`.  A line that no del or add rule matches after the masks is `other`.

Output: line 1 `cdiff old_lines=<n> new_lines=<n> only_old=<n> only_new=<n> other=<n>`; then one line per class with a
count, `class <section> <CLASS> pairs|removed|added=<n>` (sections in the order of NEW, classes in the order of RULES);
then the `other` lines (`other <section> <|> text`, at most 200, or --max N).  --samples FILE writes up to three lines of each class
for the reader (not compared).  --explore prints the shapes of the `other` lines (digits as N) by frequency.
rc 0 when other=0, 1 otherwise, 2 on a usage or read error.  The lines are compared as multisets: the order of lines is
not judged here (a moved line is no difference)."""
import collections
import re
import sys


def load(path, sections):
    out, order, cur = collections.OrderedDict(), [], "-"
    out[cur] = []
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        for l in f:
            l = l.rstrip("\n")
            if sections:
                m = re.match(r"^\[([A-Za-z0-9_]+)\]", l)
                if m:
                    cur = m.group(1)
                    out.setdefault(cur, [])
            out[cur].append(l)
    return out


def main():
    a = sys.argv[1:]
    sections = "--sections" in a
    explore = "--explore" in a
    a = [x for x in a if x not in ("--sections", "--explore")]
    samples, cap = None, 200
    if "--max" in a:
        i = a.index("--max")
        cap = int(a[i + 1])
        del a[i:i + 2]
    if "--samples" in a:
        i = a.index("--samples")
        samples = a[i + 1]
        del a[i:i + 2]
    if len(a) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    rules = []
    try:
        with open(a[0], encoding="utf-8") as f:
            for n, l in enumerate(f, 1):
                l = l.rstrip("\n")
                if not l or l.startswith("#"):
                    continue
                p = l.split("\t")
                if p[0] == "mask" and len(p) == 5:
                    rules.append(("mask", p[1], re.compile(p[2]), re.compile(p[3]), p[4]))
                elif p[0] in ("del", "add") and len(p) == 4:
                    rules.append((p[0], p[1], re.compile(p[2]), re.compile(p[3]), None))
                else:
                    print("cdiff: rules line %d is no rule" % n, file=sys.stderr)
                    return 2
        old, new = load(a[1], sections), load(a[2], sections)
    except (OSError, re.error) as e:
        print("cdiff: %s" % e, file=sys.stderr)
        return 2
    counts, others, samp = [], [], collections.OrderedDict()
    tot_o = tot_n = 0
    for sec in list(new) + [s for s in old if s not in new]:
        o = collections.Counter(old.get(sec, []))
        n = collections.Counter(new.get(sec, []))
        oo, nn = o - n, n - o
        tot_o += sum(oo.values())
        tot_n += sum(nn.values())
        if not oo and not nn:
            continue
        orig_o = {k: k for k in oo}  # masked form -> one original line (for the samples)
        orig_n = {k: k for k in nn}
        for kind, cls, sre, rx, rep in rules:
            if not sre.fullmatch(sec):
                continue
            if kind == "mask":
                mo, mn, po, pn = collections.Counter(), collections.Counter(), {}, {}
                for k, c in oo.items():
                    m = rx.sub(rep, k)
                    mo[m] += c
                    po.setdefault(m, orig_o[k])
                for k, c in nn.items():
                    m = rx.sub(rep, k)
                    mn[m] += c
                    pn.setdefault(m, orig_n[k])
                both = mo & mn
                c = sum(both.values())
                if c:
                    counts.append("class %s %s pairs=%d" % (sec, cls, c))
                    s = samp.setdefault((sec, cls), [])
                    for k in list(both)[:3]:
                        s.append("< " + po[k])
                        s.append("> " + pn[k])
                oo, nn, orig_o, orig_n = mo - mn, mn - mo, po, pn
            else:
                side = oo if kind == "del" else nn
                orig = orig_o if kind == "del" else orig_n
                hit = [k for k in side if rx.search(k)]
                c = sum(side[k] for k in hit)
                if c:
                    counts.append("class %s %s %s=%d" % (sec, cls, "removed" if kind == "del" else "added", c))
                    s = samp.setdefault((sec, cls), [])
                    for k in hit[:3]:
                        s.append(("< " if kind == "del" else "> ") + orig[k])
                    for k in hit:
                        del side[k]
        for k, c in oo.items():
            others.extend(["other %s < %s" % (sec, k)] * c)
        for k, c in nn.items():
            others.extend(["other %s > %s" % (sec, k)] * c)
    print("cdiff old_lines=%d new_lines=%d only_old=%d only_new=%d other=%d" %
          (sum(len(v) for v in old.values()), sum(len(v) for v in new.values()), tot_o, tot_n, len(others)))
    for l in counts:
        print(l)
    if explore:
        sh = collections.Counter(re.sub(r"[0-9]+", "N", x)[:170] for x in others)
        for k, c in sh.most_common(60):
            print("%6d %s" % (c, k))
    else:
        for l in others[:cap]:
            print(l[:600])
    if samples:
        with open(samples, "w", encoding="utf-8", errors="surrogateescape") as f:
            for (sec, cls), ls in samp.items():
                f.write("== %s %s\n" % (sec, cls))
                for l in ls:
                    f.write(l[:900] + "\n")
    return 0 if not others else 1


if __name__ == "__main__":
    sys.exit(main())

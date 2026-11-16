#!/usr/bin/env python3
"""compare_classes.py OLD NEW --allow FILE [--build B] [--list N] -- revision 2 (2026-11-16, after review) of the
comparison script of the extractor-fix step (records/extract-fixes-20261116/compare_classes.py, kept as it was): two
text outputs of one build before and after, compared line by line; every difference is put in one class, and a class
outside the allowance FILE is undeclared.  What changed against revision 1 (review findings of the step):
  - a masked line is counted under the masks that act on it (every mask whose substitution changes the old or the new
    line, joined with "+"), not under the first mask that leaves the two lines different;
  - a line of an old record type without a file-scope name ("<anonymous>") that has no counterpart is paired with its
    successors in the same section of the new text: new lines that name a new name and are equal to it after the new
    names are written "<anonymous>" and the fields of the `pair` rows of its section are masked on both sides.  An old
    line with no successor is undeclared (class `other`).  A successor is class `successor:<pair names>` (or `renamed`
    when no pair mask was needed); an old line with successors is class `removed-anon-paired`.  A new line with a new name
    and no old line it succeeds stays `added-named` (a new row of a newly named type).

FILE (tab-separated, '#' comments):
  mask    NAME  REGEX [REPL]     applied to both texts before the comparison (re.sub, REPL default "<NAME>")
  insert  NAME  REGEX  MAX       a new line that matches REGEX in full (after the masks), MAX lines per build or *
  remove  NAME  REGEX  MAX       an old line that matches REGEX in full
  change  NAME  REGEX  MAX       a line pair (old, new) that differ and both match REGEX in full
  pair    NAME  SECTION  REGEX  REPL   a declared field of the lines of SECTION ("*": any) that may differ between an old
                                 "<anonymous>" line and its successor (re.sub on both sides before they are compared)
  only    BUILDS                 the next rows apply only to these builds (comma list, or * for all)
Names: the record type names the new text has and the old one has not (a token with "::", a token "<anonymous@...>",
or a token before ".field" that occurs nowhere in the old text, a prefix "field:" and the like left out).  Prints the
new names, the count per class and section, up to N lines of each undeclared class and the pairs; rc 0 when every class
is declared and within its MAX, 1 otherwise, 2 on a usage error."""
import collections
import difflib
import re
import sys

SCOPED = re.compile(r"[A-Za-z_][\w@/:\-]*::[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*|<anonymous@[^>]+>")
DOTTED = re.compile(r"([A-Za-z_][\w@/:\-]*)\.(?:[A-Za-z_]\w*|\*)")
PREFIX = re.compile(r"^(?:field|global|local|param|element):")


def read(path):
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        return f.read().split("\n")


def load_allow(path, build):
    masks, rules, pairs = [], [], []
    applies = True
    for raw in read(path):
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        f = raw.split("\t")
        if f[0] == "only":
            applies = f[1] == "*" or build in f[1].split(",")
            continue
        if not applies:
            continue
        if f[0] == "mask":
            masks.append((f[1], re.compile(f[2]), f[3] if len(f) > 3 and f[3] else "<%s>" % f[1]))
        elif f[0] in ("insert", "remove", "change"):
            rules.append((f[0], f[1], re.compile(f[2]), None if f[3] == "*" else int(f[3])))
        elif f[0] == "pair":
            pairs.append((f[1], f[2], re.compile(f[3]), f[4] if len(f) > 4 else "<%s>" % f[1]))
        else:
            raise SystemExit("allowance row not understood: " + raw)
    return masks, rules, pairs


def sections(lines):
    cur, out = "head", []
    for l in lines:
        m = re.match(r"^\[([A-Za-z_0-9]+)\]", l)
        if m:
            cur = m.group(1)
        out.append(cur)
    return out


def main():
    a = sys.argv[1:]
    opts = {"--build": "", "--list": "20", "--allow": None}
    pos = []
    i = 0
    while i < len(a):
        if a[i] in opts:
            opts[a[i]] = a[i + 1]
            i += 2
        else:
            pos.append(a[i])
            i += 1
    if len(pos) != 2 or opts["--allow"] is None:
        print(__doc__, file=sys.stderr)
        return 2
    masks, rules, pairs = load_allow(opts["--allow"], opts["--build"])
    old_raw, new_raw = read(pos[0]), read(pos[1])

    def mask(l):
        for n, r, repl in masks:
            l = r.sub(repl, l)
        return l

    old, new = [mask(l) for l in old_raw], [mask(l) for l in new_raw]
    old_text = "\n".join(old)
    names = set()
    for l in new:
        for m in SCOPED.finditer(l):
            t = PREFIX.sub("", m.group(0))
            if t not in old_text:
                names.add(t)
        for m in DOTTED.finditer(l):
            t = PREFIX.sub("", m.group(1))
            if "::" not in t and t not in old_text:
                names.add(t)
    rx = re.compile("|".join(re.escape(n) for n in sorted(names, key=len, reverse=True))) if names else None

    def anon(l):
        return rx.sub("<anonymous>", l) if rx else l

    def paired_key(l, sec):
        used = []
        for n, s, r, repl in pairs:
            if s != "*" and s != sec:
                continue
            m = r.sub(repl, l)
            if m != l:
                used.append(n)
            l = m
        return l, used

    so, sn = sections(old_raw), sections(new_raw)
    cls = collections.Counter()
    used = collections.Counter()
    undeclared = collections.defaultdict(list)
    anon_old = []    # old lines with <anonymous> and no counterpart in their block: (i)
    named_new = []   # new lines with a new name and no counterpart in their block: (j)
    sm = difflib.SequenceMatcher(None, old, new, autojunk=False)
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == "equal":
            for k in range(i2 - i1):
                x, y = old_raw[i1 + k], new_raw[j1 + k]
                if x != y:
                    which = [n for n, r, repl in masks if r.sub(repl, x) != x or r.sub(repl, y) != y]
                    cls[(sn[j1 + k], "mask:" + ("+".join(which) if which else "?"))] += 1
            continue
        olds_anon = collections.Counter(old[i1:i2])
        taken = collections.Counter()
        news_left = []
        for j in range(j1, j2):
            y = new[j]
            ay = anon(y)
            if ay != y and olds_anon[ay] - taken[ay] > 0:
                taken[ay] += 1
                cls[(sn[j], "renamed")] += 1
            else:
                news_left.append(j)
        olds_left = []
        for i in range(i1, i2):
            x = old[i]
            if taken[x] > 0:
                taken[x] -= 1
                continue
            olds_left.append(i)
        rest_new = []
        for j in news_left:
            y = new[j]
            matched = False
            for kind, n, r, mx in rules:
                if kind == "change" and r.fullmatch(y):
                    for q in range(len(olds_left)):
                        x = old[olds_left[q]]
                        if r.fullmatch(x):
                            cls[(sn[j], "change:" + n)] += 1
                            used[("change", n)] += 1
                            del olds_left[q]
                            matched = True
                            break
                if matched:
                    break
            if not matched:
                rest_new.append(j)
        for j in rest_new:
            y = new[j]
            k = next((("insert", n) for kind, n, r, mx in rules if kind == "insert" and r.fullmatch(y)), None)
            if k:
                cls[(sn[j], "insert:" + k[1])] += 1
                used[k] += 1
            elif anon(new[j]) != new[j]:
                named_new.append(j)
            else:
                cls[(sn[j], "other")] += 1
                undeclared["other"].append("+ [%s] %s" % (sn[j], new_raw[j]))
        for i in olds_left:
            x = old[i]
            k = next((("remove", n) for kind, n, r, mx in rules if kind == "remove" and r.fullmatch(x)), None)
            if k:
                cls[(so[i], "remove:" + k[1])] += 1
                used[k] += 1
            elif "<anonymous>" in old[i]:
                anon_old.append(i)
            else:
                cls[(so[i], "other")] += 1
                undeclared["other"].append("- [%s] %s" % (so[i], old_raw[i]))
    # pairing over the whole section: an old <anonymous> line with its successors
    by_key = collections.defaultdict(list)
    for i in anon_old:
        k, _ = paired_key(old[i], so[i])
        by_key[(so[i], k)].append(i)
    succ_of = collections.defaultdict(list)
    for j in named_new:
        k, u = paired_key(anon(new[j]), sn[j])
        olds_here = by_key.get((sn[j], k))
        if olds_here:
            for i in olds_here:
                succ_of[i].append(j)
            u_all = set(u)
            for i in olds_here:
                u_all.update(paired_key(old[i], so[i])[1])
            cls[(sn[j], "successor:" + "+".join(sorted(u_all)) if u_all else "renamed")] += 1
        else:
            cls[(sn[j], "added-named")] += 1
    pair_lines = []
    for i in anon_old:
        if succ_of[i]:
            cls[(so[i], "removed-anon-paired")] += 1
            if old_raw[i] not in (new_raw[j] for j in succ_of[i]):
                pair_lines.append("  - %s" % old_raw[i][:300])
                for j in succ_of[i]:
                    pair_lines.append("    + %s" % new_raw[j][:300])
        else:
            cls[(so[i], "other")] += 1
            undeclared["other"].append("- [%s] (no successor) %s" % (so[i], old_raw[i]))
    bad = len(undeclared["other"])
    for kind, n, r, mx in rules:
        if mx is not None and used[(kind, n)] > mx:
            bad += 1
            undeclared["over"].append("%s %s: %d > %d" % (kind, n, used[(kind, n)], mx))
    print("new names: %d" % len(names))
    for n in sorted(names):
        print("  name %s" % n)
    for (sec, c), v in sorted(cls.items()):
        print("class %-14s %-40s %d" % (sec, c, v))
    print("pairs of an old <anonymous> line and its successors (a successor not equal after the names alone): %d lines" % len(pair_lines))
    for l in pair_lines:
        print(l)
    nl = int(opts["--list"])
    for k, ls in undeclared.items():
        print("undeclared %s: %d" % (k, len(ls)))
        for l in ls[:nl]:
            print("  " + l[:400])
    print("compare_classes: %s" % ("only declared classes" if bad == 0 else "%d undeclared differences" % bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""key_map.py OLD_KEYS NEW_KEYS OUT_TSV -- the use-site keys of the relations rows of one build before the extractor
fixes (OLD: `kz-static relations --keys` of kz-static 5ae220d2 on schema12_extract facts) and after them (NEW: the new
kz-static on the re-extraction), joined into a table old key -> new key (docs/static_facts.md 5l, 5q).

A key is the first 16 hex digits of the sha256 of the key text (path|subject|caller|callee#parameter|n-th call, path C
adds the read and its context).  Rows are joined by key text; a new key text that differs only in the names of record
types the new facts give instead of "<anonymous>" (the names compare_classes.py finds: a token with "::" or
"<anonymous@...>", or a token before ".field" the old texts do not have) is joined to the old key text it then equals.
OUT_TSV: one line per row of either side: status (same, renamed, only-old, only-new), old row, old key, new row, new key,
key text (the new one; for renamed also the old one).  Prints the counts; rc 0 always (the table is the result), 2 on a
usage error."""
import re
import sys

SCOPED = re.compile(r"[A-Za-z_][\w@/:\-]*::[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*|<anonymous@[^>]+>")
DOTTED = re.compile(r"([A-Za-z_][\w@/:\-]*)\.(?:[A-Za-z_]\w*|\*)")


def rows(path):
    out = []
    with open(path, encoding="utf-8") as f:
        for l in f:
            l = l.rstrip("\n")
            m = re.match(r"^([ABC]\d+) ([0-9a-f]{16}) (.*)$", l)
            if m:
                out.append(m.groups())
    return out


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    old, new = rows(sys.argv[1]), rows(sys.argv[2])
    old_text = "\n".join(t for _, _, t in old)
    names = set()
    for _, _, t in new:
        for m in SCOPED.finditer(t):
            if m.group(0) not in old_text:
                names.add(m.group(0))
        for m in DOTTED.finditer(t):
            if "::" not in m.group(1) and m.group(1) not in old_text:
                names.add(m.group(1))
    rx = re.compile("|".join(re.escape(n) for n in sorted(names, key=len, reverse=True))) if names else None
    by_text = {}
    for r in old:
        by_text.setdefault(r[2], []).append(r)
    used = set()
    lines, counts = [], {"same": 0, "renamed": 0, "only-old": 0, "only-new": 0}
    for nr, nk, nt in new:
        cand = [r for r in by_text.get(nt, []) if r[0] not in used]
        status = "same"
        if not cand and rx is not None:
            cand = [r for r in by_text.get(rx.sub("<anonymous>", nt), []) if r[0] not in used]
            status = "renamed"
        if cand:
            o = cand[0]
            used.add(o[0])
            counts[status] += 1
            lines.append("\t".join([status, o[0], o[1], nr, nk, nt] + ([o[2]] if status == "renamed" else [])))
        else:
            counts["only-new"] += 1
            lines.append("\t".join(["only-new", "", "", nr, nk, nt]))
    for r in old:
        if r[0] not in used:
            counts["only-old"] += 1
            lines.append("\t".join(["only-old", r[0], r[1], "", "", r[2]]))
    with open(sys.argv[3], "w", encoding="utf-8") as f:
        f.write("# key_map.py: status\told row\told key\tnew row\tnew key\tkey text (new; renamed: then the old)\n")
        f.write("# new record type names in the new key texts: %d%s\n" % (len(names), "".join(" " + n for n in sorted(names))))
        for l in lines:
            f.write(l + "\n")
    print("rows old %d new %d: same %d, renamed %d (old key -> new key), only-old %d, only-new %d; keys that change %d" % (
        len(old), len(new), counts["same"], counts["renamed"], counts["only-old"], counts["only-new"],
        counts["renamed"] + counts["only-old"] + counts["only-new"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())

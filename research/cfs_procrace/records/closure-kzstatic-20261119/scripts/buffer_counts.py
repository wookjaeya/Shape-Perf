#!/usr/bin/env python3
# Counts of the stored-name buffer and lifetime-end lines of a relations text (relations rules revision 14, static_facts.md
# 5x item 5 and its eighth addendum): a check, not a tool of the method.  Usage: buffer_counts.py RELATIONS LINES_OUT
# Writes the new lines with their namespace to LINES_OUT and the counts to stdout.
import collections, re, sys
rel, lines_out = sys.argv[1], sys.argv[2]
ns = None
keep = []
c = collections.Counter()
for l in open(rel, errors="replace"):
    l = l.rstrip("\n")
    m = re.match(r"  namespace (\S+) ", l)
    if m:
        ns = m.group(1)
        continue
    s = l.strip()
    if s.startswith("summary stored-name buffer"):
        keep.append("header\t" + s)
        continue
    if not (s.startswith("writers [") or s.startswith("stored-name ") or s.startswith("lifetime end [")):
        continue
    keep.append("%s\t%s" % (ns, s))
    if s.startswith("writers ["):
        g = s[len("writers ["):s.index("] of ")]
        c["writer groups: " + ("shared" if g.startswith("shared") else "none" if g.startswith("no key-parameter") else "one partition")] += 1
    elif s.startswith("stored-name address uses"):
        body = s.split(": ", 2)[2] if s.count(": ") >= 2 else ""
        for u in re.split(r"; (?=[A-Za-z_][\w@./:-]* at [\w./@-]+:\d+:\d+: )", body):
            if " at " not in u:
                continue
            t = u.split(": ", 1)[1] if ": " in u else u
            if t.startswith("test"): k = "test (discharged)"
            elif t.startswith("library:") and "out-behaviour" in t and "no entry" not in t and "writes it" not in t: k = "library with an entry (discharged)"
            elif t.startswith("library:"): k = "library, no entry or writes (not discharged)"
            elif t.startswith("body:"): k = "body followed, no write (discharged)"
            elif t.startswith("copied to"): k = "copied (not discharged)"
            elif "writes through it" in t: k = "body writes through it (not discharged)"
            elif "variadic" in t or "indirect" in t: k = "variadic or indirect (not discharged)"
            elif "-> library:" in t: k = "passed on to a library call (not discharged)"
            else: k = "other (not discharged)"
            c["address uses: " + k] += 1
    elif s.startswith("stored-name buffer ["):
        if "]: not decided" in s:
            c["buffers: not decided"] += 1
            continue
        c["buffers"] += 1
        m = re.search(r"; (complete|not complete \(([^;:0-9(]*))", s)
        c["buffers: " + (m.group(1) if m.group(1) == "complete" else "not complete: " + m.group(2).strip())] += 1
        for flag in ["array-element-stores-not-collected", "address-escape", "cast", "union", "unknown-effect", "untyped writes", "untyped-in-program",
                     "writer-function-not-once", "several-instances", "writer-in-loop"]:
            mm = re.search(r"(?:causes: |; )" + re.escape(flag) + r"(?=[ :])", s)
            if mm:
                v = re.search(r": (discharged|not discharged|not a cause)\b", s[mm.end():])
                c["causes: %s: %s" % (flag, v.group(1) if v else "?")] += 1
        if " other " in s.split("; FIELD_STATUS")[0]:
            c["buffers with other writers"] += 1
    else:
        m = re.match(r"lifetime end \[[^\]]*\]: (\S+(?: through a parameter| through parameter \d+)?)", s)
        form = m.group(1) if m else "?"
        if form.startswith("release through"): form = "release through a parameter"
        c["lifetime ends: " + form] += 1
        if form == "clear-slot":
            c["lifetime ends: clear-slot: " + ("free-record condition shown" if "condition shown" in s else "not shown")] += 1
        if form == "release through a parameter":
            c["release-through callers: rollback"] += s.count("[rollback:")
            c["release-through callers: delete"] += s.count("[delete:")
            c["release-through callers: null pointer"] += s.count("[a null pointer")
open(lines_out, "w").write("\n".join(keep) + "\n")
print("# %s" % rel.split("/")[-1])
for k in sorted(c):
    print("%s\t%d" % (k, c[k]))

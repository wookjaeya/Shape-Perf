#!/usr/bin/env python3
"""twin_2.py SCRATCH_ROOT REAL_ROOT MAP DIR... (addendum 12.0 (나-2): twin.py with two changes, marked below)

twin.py: -- the outputs of the MC build (REAL_ROOT/DIR) against the outputs of the
scratch check build of the same committed source (SCRATCH_ROOT/DIR), file by file (declaration 12.0 (b) of
plan/full_stage_results_20261116.md, the twin check).

MAP is a tab-separated file of `old<TAB>new` strings: each `old` (a text of the scratch side: its output root, the path
and the file sha256 of the scratch kz-static, the HEAD commit the scratch runner recorded) is replaced by `new` in every
scratch text file before the comparison.  Besides that, two kinds of line are masked on both sides, as declared:
  status.txt  the first line `# one_build.sh <build> <UTC time>` (the time of the run)
  status.txt  [addendum] the line `repo <commit>` (the repository HEAD at the time of the run; the scratch runs of
              2026-11-18 recorded three commits because other steps committed while they ran)
[addendum] After a first pass, for every file pair that is the same after the replacement but not as bytes, the
sha256 of the scratch file and of the real file are added to MAP, and every file is
compared again with the extended map (a plan sidecar records its input paths; the plan and the two tables name the
sidecar's sha256).
  every file  nothing else
Left out of the comparison, as declared: the runner's space_guard.log (times and free bytes of the run) and the
`again/` directories (the second runs of order, removed by compare.sh after their check).
Containers (*.kz) and all other files are compared as bytes after the replacement (a container holds no path; the tool
identity it records is the same for both builds when the source is the same, kairos/cmake/KzSourceIdentity.cmake).

Output: one line per file, `same <path>` or `DIFF <path> (<first differing line number>)` or `only-scratch <path>` /
`only-real <path>`, then `summary files=<n> same=<n> diff=<n> only-scratch=<n> only-real=<n> skipped=<n>`.
rc 0 when diff, only-scratch and only-real are all 0, 1 otherwise, 2 on a usage or read error."""
import os
import re
import sys

SKIP_NAMES = {"space_guard.log"}
STATUS_TIME = re.compile(rb"^# one_build\.sh (\S+) \S+$")


def files(root):
    out = set()
    for d, dirs, fs in os.walk(root):
        dirs[:] = sorted(x for x in dirs if x != "again")
        for f in fs:
            out.add(os.path.relpath(os.path.join(d, f), root))
    return out


def norm(rel, data, subst):
    for o, n in subst:
        data = data.replace(o, n)
    if os.path.basename(rel) == "status.txt":
        lines = data.split(b"\n")
        if lines and STATUS_TIME.match(lines[0]):
            lines[0] = STATUS_TIME.sub(rb"# one_build.sh \1 TIME", lines[0])
        lines = [b"repo COMMIT" if l.startswith(b"repo ") else l for l in lines]  # [addendum]
        data = b"\n".join(lines)
    return data


def main():
    if len(sys.argv) < 5:
        print(__doc__, file=sys.stderr)
        return 2
    sroot, rroot, mapf = sys.argv[1:4]
    subst = []
    try:
        with open(mapf, "rb") as f:
            for l in f:
                l = l.rstrip(b"\n")
                if not l or l.startswith(b"#"):
                    continue
                o, n = l.split(b"\t")
                subst.append((o, n))
    except (OSError, ValueError) as e:
        print("twin: map: %s" % e, file=sys.stderr)
        return 2
    subst.sort(key=lambda p: -len(p[0]))  # longer strings first (a root path before a shorter prefix)
    import hashlib
    out = []
    for pas in (1, 2):  # [addendum] two passes, the second with the sha256 of the files the first found the same
        res, extra = compare(sys.argv[4:], sroot, rroot, subst)
        if res is None:
            return 2
        if pas == 1:
            for a, b in extra:
                ha, hb = hashlib.sha256(a).hexdigest().encode(), hashlib.sha256(b).hexdigest().encode()
                subst.append((ha, hb))
            subst.sort(key=lambda p: -len(p[0]))
    lines, n, same, diff, only_s, only_r, skipped = res
    for l in lines:
        print(l)
    print("summary files=%d same=%d diff=%d only-scratch=%d only-real=%d skipped=%d (second pass; map entries %d)" % (n, same, diff, only_s, only_r, skipped, len(subst)))
    return 0 if diff == only_s == only_r == 0 else 1


def compare(dirs, sroot, rroot, subst):
    out, extra = [], []
    n = same = diff = only_s = only_r = skipped = 0
    for d in dirs:
        sd, rd = os.path.join(sroot, d), os.path.join(rroot, d)
        if not os.path.isdir(sd) or not os.path.isdir(rd):
            print("twin: %s missing on one side" % d, file=sys.stderr)
            return None, None
        fs, fr = files(sd), files(rd)
        for rel in sorted(fs | fr):
            p = os.path.join(d, rel)
            if os.path.basename(rel) in SKIP_NAMES:
                skipped += 1
                continue
            n += 1
            if rel not in fr:
                out.append("only-scratch %s" % p)
                only_s += 1
                continue
            if rel not in fs:
                out.append("only-real %s" % p)
                only_r += 1
                continue
            with open(os.path.join(sd, rel), "rb") as f:
                a = norm(rel, f.read(), subst)
            with open(os.path.join(rd, rel), "rb") as f:
                rawb = f.read()
                b = norm(rel, rawb, [])
            if a == b:
                out.append("same %s" % p)
                with open(os.path.join(sd, rel), "rb") as f:
                    rawa = f.read()
                if rawa != rawb:
                    extra.append((rawa, rawb))
                same += 1
            else:
                la, lb = a.split(b"\n"), b.split(b"\n")
                k = next((i for i in range(min(len(la), len(lb))) if la[i] != lb[i]), min(len(la), len(lb)))
                out.append("DIFF %s (line %d)" % (p, k + 1))
                diff += 1
    return (out, n, same, diff, only_s, only_r, skipped), extra

if __name__ == "__main__":
    sys.exit(main())

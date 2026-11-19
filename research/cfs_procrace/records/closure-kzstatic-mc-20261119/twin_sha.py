#!/usr/bin/env python3
"""twin_sha.py S6LIST REAL_ROOT MAP --scratch DIR -- the twin check of the declared kz-static closure build (results
section 21.0): every output file of the MC run (REAL_ROOT/cz_<step>/...) against the sha256 of its scratch twin, the
output of the scratch check build of the same committed source (records/closure-kzstatic-20261119/scratch6/
s6_outputs.sha256, paths ./s6_<step>/...).  The scratch outputs themselves were removed after the sixth run; only their
sha256 are kept, so each real file is turned into the bytes its scratch twin must have had and that is hashed.

MAP is a tab-separated file of `real<TAB>scratch` strings.  In both columns `<scratch>` stands for DIR (the scratch
directory of the sixth run; kept logs write it `<scratch>`), and `<mc-kz-static-sha256>` for the sha256 of the MC
kz-static file (read here from /home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static).  Each `real` string is
replaced by its `scratch` string in the real file (longer strings first).  A second pass adds, for every file whose
replaced bytes have the listed sha256 while its own bytes differ, the pair (sha256 of the real file, listed sha256) to
the map and compares every file again: a plan sidecar records its input paths and the plan and the two tables record
the sidecar's sha256 (the same step as records/review-fixes-2-mc-20261119/twin_2.py).

Left out, as declared: `space_guard.log` (times and free bytes of the run), `status.txt` (the time of the run and the
repository HEAD; its `result PASS` line is checked by compare.sh), and the `again/` directories (the second runs, each
checked by compare.sh to be the same bytes as the first run).  Nothing else is masked.

Output: one line per file, `same <path>`, `DIFF <path>`, `only-scratch <path>` (listed, no real file) or `only-real
<path>` (a real file that is not listed), then `summary files=<n> same=<n> diff=<n> only-scratch=<n> only-real=<n>
skipped=<n> (second pass; map entries <n>)`.  rc 0 when diff, only-scratch and only-real are all 0, 1 otherwise, 2 on a
usage or read error."""
import hashlib
import os
import sys

MC_KS = "/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static"
SKIP_NAMES = {"space_guard.log", "status.txt"}


def sha(b):
    return hashlib.sha256(b).hexdigest()


def skipped(rel):
    parts = rel.split("/")
    return parts[-1] in SKIP_NAMES or "again" in parts[:-1]


def main(argv):
    if len(argv) != 6 or argv[4] != "--scratch":
        print(__doc__, file=sys.stderr)
        return 2
    s6list, root, mapf, sdir = argv[1], argv[2], argv[3], argv[5]
    try:
        with open(MC_KS, "rb") as f:
            mcsha = sha(f.read())
        listed = {}
        with open(s6list, encoding="utf-8") as f:
            for l in f:
                h, p = l.rstrip("\n").split("  ", 1)
                assert p.startswith("./s6_"), p
                listed["cz_" + p[len("./s6_"):]] = h
        subst = []
        with open(mapf, encoding="utf-8") as f:
            for l in f:
                l = l.rstrip("\n")
                if not l or l.startswith("#"):
                    continue
                a, b = l.split("\t")
                a = a.replace("<scratch>", sdir).replace("<mc-kz-static-sha256>", mcsha)
                b = b.replace("<scratch>", sdir).replace("<mc-kz-static-sha256>", mcsha)
                if a != b:
                    subst.append((a.encode(), b.encode()))
    except (OSError, ValueError, AssertionError) as e:
        print("twin_sha: %s" % e, file=sys.stderr)
        return 2
    steps = sorted({p.split("/", 1)[0] for p in listed})
    real = set()
    for s in steps:
        for d, dirs, fs in os.walk(os.path.join(root, s)):
            dirs.sort()
            for x in fs:
                real.add(os.path.relpath(os.path.join(d, x), root))
    names = sorted(set(listed) | real)
    for pas in (1, 2):
        subst.sort(key=lambda p: -len(p[0]))
        out, extra = [], []
        n = same = diff = only_s = only_r = skip = 0
        for rel in names:
            if skipped(rel):
                skip += 1
                continue
            n += 1
            if rel not in real:
                out.append("only-scratch %s" % rel)
                only_s += 1
                continue
            if rel not in listed:
                out.append("only-real %s" % rel)
                only_r += 1
                continue
            with open(os.path.join(root, rel), "rb") as f:
                raw = f.read()
            b = raw
            for o, t in subst:
                b = b.replace(o, t)
            if sha(b) == listed[rel]:
                out.append("same %s" % rel)
                same += 1
                if b != raw:
                    extra.append((sha(raw).encode(), listed[rel].encode()))
            else:
                out.append("DIFF %s" % rel)
                diff += 1
        if pas == 1:
            subst.extend(e for e in extra if e not in subst)
    for l in out:
        print(l)
    print("summary files=%d same=%d diff=%d only-scratch=%d only-real=%d skipped=%d (second pass; map entries %d)"
          % (n, same, diff, only_s, only_r, skip, len(subst)))
    return 0 if diff == only_s == only_r == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))

#!/usr/bin/env python3
"""explain.py -- explanation script revision 4: the driver (plan/c1_closure_explain_rev4_spec_20261118.md 2, 10.1).

    python3 -I explain.py <the arguments of explain_read.py except --normal-out> --vocabulary FILE --out NEW_DIR
    python3 -I explain.py --check RECORD.json --vocabulary FILE

It runs the reader part (explain_read.py) into NEW_DIR/normal.json and then the judgement part (explain_judge.py) on that
file into NEW_DIR, and does nothing else.  The two parts carry their own sha256 (recorded in every record).  rc: the
reader's when not 0, else the judge's.  Revision 3 is kept byte for byte as explain_rev3.py.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main(argv):
    if "--check" in argv:
        return subprocess.call([sys.executable, "-I", os.path.join(HERE, "explain_judge.py")] + argv)
    args, vocab, out = [], None, None
    i = 0
    while i < len(argv):
        if argv[i] == "--vocabulary" and i + 1 < len(argv):
            vocab = argv[i + 1]
            i += 2
            continue
        if argv[i] == "--out" and i + 1 < len(argv):
            out = argv[i + 1]
            i += 2
            continue
        args.append(argv[i])
        i += 1
    if vocab is None or out is None:
        print("explain.py: --vocabulary and --out are required", file=sys.stderr)
        return 64
    if os.path.exists(out):
        print("explain.py: %s exists (new directories only)" % out, file=sys.stderr)
        return 64
    os.makedirs(out)
    normal = os.path.join(out, "normal.json")
    rc = subprocess.call([sys.executable, "-I", os.path.join(HERE, "explain_read.py")] + args + ["--normal-out", normal])
    if rc != 0:
        return rc
    return subprocess.call([sys.executable, "-I", os.path.join(HERE, "explain_judge.py"), "--normal", normal,
                            "--vocabulary", vocab, "--out", out])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

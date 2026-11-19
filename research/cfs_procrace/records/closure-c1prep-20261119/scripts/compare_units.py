#!/usr/bin/env python3
"""compare_units.py -- two unit_check.py outputs of the same plan with two plugins (A, B), unit by unit (results 25.5):
the object files byte for byte, and the binding files line by line apart from the plugin's version text in the unit
header ("# kzgcc <version> backend=..." and its dumpbase=, the object path of the run), each unit named by its tu (units.tsv order).  A unit that A failed to compile
is listed apart (its B lines are shown in summary).  Run with python3 -I.  Reads only.

    compare_units.py A_DIR B_DIR
Exit status: 0 when every unit both compiled is the same apart from the version text, 1 otherwise, 2 usage.
"""
import hashlib
import os
import re
import sys


def units(d):
    rows = [l.rstrip('\n').split('\t') for l in open(os.path.join(d, 'units.tsv'), encoding='utf-8')][1:]
    return [(i, r[0], r[1]) for i, r in enumerate(rows)]


def sha(p):
    return hashlib.sha256(open(p, 'rb').read()).hexdigest() if os.path.exists(p) else None


def binds(d, i):
    bd = os.path.join(d, 'binds', '%02d' % i)
    out = []
    for f in sorted(os.listdir(bd)) if os.path.isdir(bd) else []:
        for line in open(os.path.join(bd, f), encoding='utf-8'):
            l = re.sub(r'^# kzgcc [0-9.]+ backend=', '# kzgcc <version> backend=', line.rstrip('\n'))
            out.append(re.sub(r' dumpbase=\S+', ' dumpbase=<output object>', l) if l.startswith('# kzgcc <version> ') else l)
    return out


def main():
    if len(sys.argv) != 3:
        sys.stderr.write('usage: compare_units.py A_DIR B_DIR\n')
        return 2
    A, B = sys.argv[1], sys.argv[2]
    ua, ub = units(A), units(B)
    if [u[1] for u in ua] != [u[1] for u in ub]:
        print('DIFFERENT unit lists')
        return 1
    bad = 0
    same = 0
    for (i, tu, rca), (_, _, rcb) in zip(ua, ub):
        if rca != '0':
            lb = binds(B, i)
            st = {}
            for l in lb:
                m = re.match(r'bind \d+ site=\d+ .*status=([a-z]+)', l)
                if m:
                    st[m.group(1)] = st.get(m.group(1), 0) + 1
            print('A-FAILED %s (A rc=%s, B rc=%s): B binding lines %s' % (tu, rca, rcb, ', '.join('%s %d' % kv for kv in sorted(st.items()))))
            continue
        oa, ob = sha(os.path.join(A, 'obj', '%02d.o' % i)), sha(os.path.join(B, 'obj', '%02d.o' % i))
        ba, bb = binds(A, i), binds(B, i)
        if oa == ob and ba == bb and rcb == '0':
            same += 1
        else:
            bad += 1
            print('DIFFERENT %s object %s bindings %s' % (tu, 'same' if oa == ob else 'differs', 'same' if ba == bb else 'differ'))
    print('units %d: same apart from the version text %d, different %d, failed in A %d' % (len(ua), same, bad, sum(1 for u in ua if u[2] != '0')))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())

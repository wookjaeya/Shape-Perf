#!/usr/bin/env python3
"""compare_manifest.py OLD NEW -- regress_manifest.sh outputs of the old and the new plugin: per group every object the
same bytes and every binding file (matched by its name without the hash part: the plugin names it by the unit and a hash of
the dumpbase, which holds the output path) the same apart from line 2 (the version text "# kzgcc 0.3 ..." / "# kzgcc 0.4
...", which also names the dumpbase).  rc 0 when nothing else differs, 1 otherwise."""
import filecmp
import os
import re
import sys


def by_unit(d):
    return {re.sub(r'\.[0-9a-f]{8}\.kzs$', '', f): f for f in os.listdir(d)}


def main():
    old, new = sys.argv[1:3]
    bad = 0
    for g in ('v1-target', 'v2-shapes', 'v3-owner', 'v3-c1plan'):
        objs = sorted(os.listdir(os.path.join(old, g, 'obj')))
        same_obj = [o for o in objs if filecmp.cmp(os.path.join(old, g, 'obj', o), os.path.join(new, g, 'obj', o), shallow=False)]
        mo, mn = by_unit(os.path.join(old, g, 'binds')), by_unit(os.path.join(new, g, 'binds'))
        bo, bn = sorted(mo), sorted(mn)
        same_b, other = 0, []
        for f in bo:
            if f not in mn:
                other.append(f + ' missing')
                continue
            a = open(os.path.join(old, g, 'binds', mo[f]), encoding='utf-8').read().split('\n')
            b = open(os.path.join(new, g, 'binds', mn[f]), encoding='utf-8').read().split('\n')
            d = [i + 1 for i in range(max(len(a), len(b))) if i >= len(a) or i >= len(b) or a[i] != b[i]]
            if d and d != [2]:
                other.append('%s lines %s' % (f, d[:5]))
            elif d == [2] and not (a[1].startswith('# kzgcc 0.3 ') and b[1].startswith('# kzgcc 0.4 ')):
                other.append('%s line 2 is not the version text' % f)
            else:
                same_b += 1
        ok = len(same_obj) == len(objs) and len(objs) > 0 and not other and bo == bn
        bad += 0 if ok else 1
        print('%s %s: objects %d, the same bytes %d; binding files %d/%d, the same apart from the version line %d%s' % (
            'OK' if ok else 'BAD', g, len(objs), len(same_obj), len(bo), len(bn), same_b, '' if not other else ' (%s)' % other[:5]))
    print('compare_manifest: %s' % ('versions 1-3 bind byte-identically' if bad == 0 else '%d groups differ' % bad))
    return 0 if bad == 0 else 1


if __name__ == '__main__':
    sys.exit(main())

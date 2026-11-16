#!/usr/bin/env python3
# framework_scan.py -- the mechanical scan and the provenance check of the framework-layer data file
# (kairos/semantics/framework_api_roles.txt, read by `kz-static plan`).
#
#   python3 -I framework_scan.py scan  <trees.tsv>               the scan: every function declared in the version-pinned
#                                                                public headers of each tree, with its line and the
#                                                                @brief line of its documentation comment
#   python3 -I framework_scan.py check <trees.tsv> <data file>   the provenance check (rc 0 = every entry passes)
#
# trees.tsv: lines "<version label>\t<tree root>\t<public header, relative to the root>[,<header>...]".
#
# The check, for every entry of the data file:
#   - its declaration file:line is in the scan output of a tree (a declaration of the same function at that line of a
#     public header of the tree), and the header's sha256 is decl_sha256;
#   - its definition file:line defines the function in that tree (the function's name followed by '(' on that line, not
#     ending in ';'), and the file's sha256 is def_sha256;
#   - coverage: exactly one entry per role and version (the versions are told apart by the definition file's sha256);
#     every tree of trees.tsv has one entry of each role.
# The roles are those of the method (run loop: the call an application makes in its main loop to learn whether it is to
# go on; app exit: the call by which an application ends itself).  Which declared function plays a role is read from the
# header's own documentation (the @brief line printed by the scan); the check verifies the provenance, not the reading.
import hashlib
import os
import re
import sys

DECL = re.compile(r'^\s*(?:[A-Za-z_][A-Za-z0-9_]*[\s\*]+)+\**([A-Za-z_][A-Za-z0-9_]*)\s*\(([^;{]*)\)?\s*;')
ENTRY = re.compile(r'^role (run-loop|app-exit) function=(\S+) def=(\S+):(\d+) def_sha256=([0-9a-f]{64}) decl=(\S+):(\d+) decl_sha256=([0-9a-f]{64}) version="([^"]*)"$')
ROLES = ('run-loop', 'app-exit')


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


def trees(path):
    out = []
    for line in open(path):
        line = line.rstrip('\n')
        if not line or line.startswith('#'):
            continue
        label, root, headers = line.split('\t')
        out.append((label, root, headers.split(',')))
    return out


def scan_header(root, rel):
    lines = open(os.path.join(root, rel), errors='replace').read().split('\n')
    found = []
    brief = ''
    for n, l in enumerate(lines, 1):
        m = re.search(r'[@\\]brief\s+(.*)', l)
        if m:
            brief = m.group(1).strip()
        d = DECL.match(l)
        if d and not l.lstrip().startswith(('#', '*', '/')) and 'typedef' not in l:
            found.append((n, d.group(1), brief))
            brief = ''
    return found


def do_scan(tv):
    for label, root, headers in trees(tv):
        for h in headers:
            print('# %s %s sha256=%s' % (label, h, sha(os.path.join(root, h))))
            for n, name, brief in scan_header(root, h):
                print('%s\t%s:%d\t%s\t%s' % (label, h, n, name, brief))
    return 0


def do_check(tv, data):
    ts = trees(tv)
    scan = {}
    for label, root, headers in ts:
        for h in headers:
            for n, name, _ in scan_header(root, h):
                scan[(label, h, n)] = name
    entries = []
    for i, line in enumerate(open(data), 1):
        line = line.rstrip('\n')
        if not line or line.startswith('#'):
            continue
        m = ENTRY.match(line)
        if not m:
            print('FAIL line %d: not an entry' % i)
            return 1
        entries.append((i,) + m.groups())
    bad = 0
    seen = {}
    for (i, role, fn, dfile, dline, dsha, hfile, hline, hsha, version) in entries:
        ok_tree = None
        for label, root, headers in ts:
            p = os.path.join(root, dfile)
            if not os.path.isfile(p) or sha(p) != dsha:
                continue
            ok_tree = (label, root, headers)
        if ok_tree is None:
            print('FAIL line %d: no tree has %s with sha256 %s' % (i, dfile, dsha[:12]))
            bad += 1
            continue
        label, root, headers = ok_tree
        src = open(os.path.join(root, dfile), errors='replace').read().split('\n')
        dl = src[int(dline) - 1] if int(dline) <= len(src) else ''
        if not re.search(r'\b%s\s*\(' % re.escape(fn), dl) or dl.rstrip().endswith(';'):
            print('FAIL line %d: %s:%s does not define %s' % (i, dfile, dline, fn))
            bad += 1
        if hfile not in headers or scan.get((label, hfile, int(hline))) != fn:
            print('FAIL line %d: %s:%s is not a declaration of %s in the scan of %s' % (i, hfile, hline, fn, label))
            bad += 1
        elif sha(os.path.join(root, hfile)) != hsha:
            print('FAIL line %d: %s sha256 differs' % (i, hfile))
            bad += 1
        k = (role, label)
        if k in seen:
            print('FAIL line %d: a second entry of %s for %s (line %d)' % (i, role, label, seen[k]))
            bad += 1
        seen[k] = i
        print('ok   line %d: %s %s %s %s:%s decl %s:%s (%s)' % (i, label, role, fn, dfile, dline, hfile, hline, version))
    for label, _, _ in ts:
        for role in ROLES:
            if (role, label) not in seen:
                print('FAIL coverage: no %s entry for %s' % (role, label))
                bad += 1
    print('%s: %d entries, %d trees, %d failures' % ('PASS' if bad == 0 else 'FAIL', len(entries), len(ts), bad))
    return 0 if bad == 0 else 1


def main(argv):
    if len(argv) == 3 and argv[1] == 'scan':
        return do_scan(argv[2])
    if len(argv) == 4 and argv[1] == 'check':
        return do_check(argv[2], argv[3])
    sys.stderr.write('usage: framework_scan.py scan <trees.tsv> | check <trees.tsv> <data file>\n')
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))

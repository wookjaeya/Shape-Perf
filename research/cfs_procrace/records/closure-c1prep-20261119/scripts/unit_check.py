#!/usr/bin/env python3
"""unit_check.py -- compile EVERY unit that holds a site of a C1 plan (kzs-sites 4) with its kept compile command and a
GCC plugin, into a scratch directory, and list per unit the compiler's exit status, the plugin's binding counts and the
first error line.  A scratch check of a plugin against a whole plan (results section 25.4): it is not a derived build
and writes nothing into the reference build tree (objects and binding files go under --out).

Run with python3 -I.  No row id, key or name is an input: the units are the tu= values of the plan's site lines.

    unit_check.py --plan PLAN.kzs --modules PLAN.mod --compile-db compile_commands.json --plugin kzgcc.so --out DIR
Writes DIR/obj, DIR/binds, DIR/compile.log and DIR/units.tsv (unit, rc, bound, inlined, duplicated, unsupported,
removed, first error line), and prints a summary.  Exit status 0 when every unit compiled, 1 when one failed, 2 usage.
"""
import argparse
import collections
import json
import os
import re
import shlex
import subprocess
import sys


def main():
    ap = argparse.ArgumentParser()
    for o in ('--plan', '--modules', '--compile-db', '--plugin', '--out'):
        ap.add_argument(o, required=True)
    a = ap.parse_args()
    if os.path.exists(a.out):
        sys.stderr.write('unit_check.py: %s exists\n' % a.out)
        return 2
    os.makedirs(os.path.join(a.out, 'obj'))
    units = sorted({m.group(1) for m in (re.search(r' tu=(\S+)', l) for l in open(a.plan, encoding='utf-8') if l.startswith('site '))
                    if m})
    mods, default = [], None
    for line in open(a.modules, encoding='utf-8'):
        if line.startswith('#') or not line.strip():
            continue
        f = line.rstrip('\n').split('\t')
        if f[0] == 'default':
            default = f[1]
        else:
            mods.append((f[0], f[2]))
    db = json.load(open(a.compile_db, encoding='utf-8'))
    rows = []
    log = open(os.path.join(a.out, 'compile.log'), 'w', encoding='utf-8')
    failed = 0
    for n, tu in enumerate(units):
        bd = os.path.abspath(os.path.join(a.out, 'binds', '%02d' % n))
        os.makedirs(bd)
        pargs = ['-fplugin=' + a.plugin, '-fplugin-arg-kzgcc-manifest=' + os.path.abspath(a.plan), '-fplugin-arg-kzgcc-out=' + bd]
        pargs += ['-fplugin-arg-kzgcc-module=%s:%s' % (i, p) for i, p in mods]
        if default:
            pargs.append('-fplugin-arg-kzgcc-default-module=' + default)
        es = [e for e in db if e['file'].endswith('/' + tu)]
        if len(es) != 1:
            log.write('unit %s: %d compile commands (not compiled)\n' % (tu, len(es)))
            rows.append((tu, 'no-command', collections.Counter(), ''))
            failed += 1
            continue
        e = es[0]
        argv = shlex.split(e['command']) if 'command' in e else list(e['arguments'])
        i = argv.index('-o')
        argv[i + 1] = os.path.abspath(os.path.join(a.out, 'obj', '%02d.o' % n))
        argv += pargs
        r = subprocess.run(argv, cwd=e['directory'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        out = r.stdout.decode('utf-8', 'replace')
        log.write('unit %s: rc %d\n%s' % (tu, r.returncode, out))
        cnt = collections.Counter()
        for f in os.listdir(bd):
            for line in open(os.path.join(bd, f), encoding='utf-8'):
                m = re.match(r'bind \d+ site=\d+ .*status=([a-z]+)', line)
                if m:
                    cnt[m.group(1)] += 1
        err = next((l.strip() for l in out.split('\n') if 'error' in l), '')
        rows.append((tu, str(r.returncode), cnt, err))
        failed += r.returncode != 0
    log.close()
    with open(os.path.join(a.out, 'units.tsv'), 'w', encoding='utf-8') as o:
        o.write('unit\trc\tbound\tinlined\tduplicated\tunsupported\tremoved\tfirst_error\n')
        for tu, rc, c, err in rows:
            o.write('%s\t%s\t%d\t%d\t%d\t%d\t%d\t%s\n' % (tu, rc, c['bound'], c['inlined'], c['duplicated'], c['unsupported'], c['removed'], err))
    tot = collections.Counter()
    for _, _, c, _ in rows:
        tot.update(c)
    print('units %d, failed %d; binding lines: %s' % (len(units), failed, ', '.join('%s %d' % kv for kv in sorted(tot.items()))))
    for tu, rc, c, err in rows:
        if rc != '0':
            print('FAILED %s rc=%s: %s' % (tu, rc, err))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())

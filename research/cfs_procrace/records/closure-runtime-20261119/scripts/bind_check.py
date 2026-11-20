#!/usr/bin/env python3
"""bind_check.py -- the bind check of the runtime-side build (results section 23): compiles ONLY the units that hold the
sites of the design rows and the control rows of a C1 plan, with their kept compile commands and the GCC plugin, and
lists every site of those rows with its binding (bound / inlined / duplicated / removed / unsupported and the reason).

Run with python3 -I.  Reads the plan (kzs-sites 4), its sidecar, its module table and the kept compile_commands.json;
writes only under --out (objects, binding files, the compile log, the report).  The rows are the sidecar's row lines with
control-state=planned (the path A control selection, which holds the design rows); no row id, key or name is an input.

    bind_check.py --plan PLAN.kzs --sidecar PLAN.side --modules PLAN.mod --compile-db compile_commands.json
                  --plugin kzgcc.so --out DIR
Exit status: 0 when every unit compiled (whatever the bindings say), 1 when a compile failed, 2 on a usage error.
"""
import argparse
import json
import os
import re
import shlex
import subprocess
import sys


def main():
    ap = argparse.ArgumentParser()
    for o in ('--plan', '--sidecar', '--modules', '--compile-db', '--plugin', '--out'):
        ap.add_argument(o, required=True)
    a = ap.parse_args()
    os.makedirs(os.path.join(a.out, 'obj'), exist_ok=False)
    os.makedirs(os.path.join(a.out, 'binds'))
    rows = []
    for line in open(a.sidecar, encoding='utf-8'):
        if line.startswith('row ') and ' control-state=planned' in line:
            f = line.split()
            rows.append((f[1], f[2]))
    keys = {k for k, _ in rows}
    sites = []  # (id, label, tu, func, line, place/phase, row keys of the rows above)
    for line in open(a.plan, encoding='utf-8'):
        if not line.startswith('site '):
            continue
        sid = int(line.split()[1])
        g = lambda k: (re.search(r'\b%s=("[^"]*"|\S+)' % k, line) or [None, ''])[1]
        rk = [r for r in g('row').split(',') if r]
        sites.append({'id': sid, 'label': g('label'), 'tu': g('tu'), 'func': g('func'), 'line': g('line'), 'place': g('place'),
                      'phase': g('phase'), 'callee': g('callee'), 'col': g('col'), 'nth': g('nth'), 'rows': rk,
                      'design': bool(keys.intersection(rk))})
    units = sorted({s['tu'] for s in sites if s['design']})
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
    pargs = ['-fplugin=' + a.plugin, '-fplugin-arg-kzgcc-manifest=' + os.path.abspath(a.plan),
             '-fplugin-arg-kzgcc-out=' + os.path.abspath(os.path.join(a.out, 'binds'))]
    pargs += ['-fplugin-arg-kzgcc-module=%s:%s' % (i, p) for i, p in mods]
    if default:
        pargs.append('-fplugin-arg-kzgcc-default-module=' + default)
    log = open(os.path.join(a.out, 'compile.log'), 'w', encoding='utf-8')
    failed = 0
    for n, tu in enumerate(units):
        es = [e for e in db if e['file'].endswith('/' + tu)]
        if len(es) != 1:
            log.write('unit %s: %d compile commands (not compiled)\n' % (tu, len(es)))
            failed += 1
            continue
        e = es[0]
        argv = shlex.split(e['command']) if 'command' in e else list(e['arguments'])
        i = argv.index('-o')
        obj = os.path.abspath(os.path.join(a.out, 'obj', '%02d.o' % n))
        argv[i + 1] = obj
        argv += pargs
        r = subprocess.run(argv, cwd=e['directory'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log.write('unit %s: rc %d obj %s\n%s' % (tu, r.returncode, os.path.basename(obj), r.stdout.decode('utf-8', 'replace')))
        failed += r.returncode != 0
    log.close()
    binds = {}
    notes = {}
    head = []
    for f in sorted(os.listdir(os.path.join(a.out, 'binds'))):
        for line in open(os.path.join(a.out, 'binds', f), encoding='utf-8'):
            m = re.match(r'bind (\d+) site=(\d+) .*status=(\S+)(?: vals=\S+)?(?: reason="([^"]*)")?', line)
            if m:
                binds.setdefault(int(m.group(2)), []).append((m.group(3), m.group(4) or ''))
            m = re.match(r'# bind (\d+) kind=\S+ .*notes=(.*)$', line)
            if m:
                notes[int(m.group(1))] = m.group(2).strip()
            if line.startswith(('# unsupported-kind', '# not-applied', '# v4 calls', '# plan')):
                head.append(f.split('.')[0][-40:] + ' ' + line.rstrip('\n'))
    with open(os.path.join(a.out, 'report.txt'), 'w', encoding='utf-8') as o:
        o.write('# bind_check.py: plan %s; rows with control-state=planned: %d (%s)\n' % (a.plan, len(rows), ' '.join(l for _, l in rows)))
        o.write('# units compiled: %d (%s); compile failures: %d\n' % (len(units), ' '.join(units), failed))
        tot = {}
        for s in sites:
            if s['tu'] not in units:
                continue
            st = binds.get(s['id'], [])
            states = sorted({x[0] for x in st}) or ['no-binding-line']
            verdict = 'bound' if any(x in ('bound', 'inlined', 'duplicated') for x in states) else 'not-bound'
            reasons = sorted({x[1] for x in st if x[1]})
            cls = 'row' if s['design'] else 'other'
            tot[(cls, verdict)] = tot.get((cls, verdict), 0) + 1
            o.write('site %d %s %s tu=%s func=%s line=%s %s col=%s nth=%s states=%s%s\n' % (
                s['id'], cls, verdict, s['tu'], s['func'], s['line'], 'place=' + s['place'] if s['place'] else 'phase=' + s['phase'],
                s['col'] or '-', s['nth'] or '-', ','.join(states), (' reason="%s"' % '; '.join(reasons)) if verdict != 'bound' and reasons else ''))
        for k in sorted(tot):
            o.write('count %s %s %d\n' % (k[0], k[1], tot[k]))
        for h in head:
            o.write('unit-head %s\n' % h)
    sys.stdout.write(open(os.path.join(a.out, 'report.txt'), encoding='utf-8').read())
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())

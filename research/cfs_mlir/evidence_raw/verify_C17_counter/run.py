#!/usr/bin/env python3
"""C17 counter-check: gcc README-built DB (probe), non-coverage entries only, original flags
(incl. -Werror, gcc-only -Wno-stringop-*) + -Wno-unknown-warning-option -S -emit-llvm -O0 -g.
Then mlir-translate --import-llvm --mlir-print-debuginfo per file; then llvm-link + import."""
import json, os, re, shlex, subprocess, collections, time
W = os.path.dirname(os.path.abspath(__file__)); N = os.path.join(W, '..')
ROOT = os.path.join(N, 'probe/cFS'); DB = os.path.join(N, 'probe/compile_commands.cpu1.json')
B = '/home/user/work/llvm-project/build/bin'; OUT = os.path.join(W, 'out'); os.makedirs(OUT, exist_ok=True)
sel = re.compile(r'/(apps/[^/]+/fsw/src/|cfe/modules/(sb|es|tbl|evs|time)/fsw/src/)')
ents = [e for e in json.load(open(DB)) if sel.search(e['file']) and '--coverage' not in e['command'] and 'fprofile-arcs' not in e['command']]
rows = []; lls = []
for e in ents:
    a = shlex.split(e['command']); args = []; skip = False
    for x in a[1:]:
        if skip: skip = False; continue
        if x == '-o': skip = True; continue
        if x == '-c': continue
        args.append(x)
    tag = os.path.relpath(e['file'], ROOT).replace('/', '__')
    ll = os.path.join(OUT, tag + '.ll'); ml = os.path.join(OUT, tag + '.mlir')
    r1 = subprocess.run([B+'/clang', *args, '-Wno-unknown-warning-option', '-S', '-emit-llvm', '-O0', '-g', '-o', ll], cwd=e['directory'], capture_output=True, text=True)
    row = dict(file=os.path.relpath(e['file'], ROOT), clang_rc=r1.returncode, clang_stderr=r1.stderr.strip()[-300:])
    if r1.returncode == 0:
        lls.append(ll)
        r2 = subprocess.run([B+'/mlir-translate', '--import-llvm', '--mlir-print-debuginfo', ll, '-o', ml], capture_output=True, text=True)
        row.update(import_rc=r2.returncode, import_stderr=r2.stderr.strip()[-300:])
        if r2.returncode == 0: os.remove(ml)
    rows.append(row)
json.dump(rows, open(os.path.join(W, 'per_file_results.json'), 'w'), indent=1)
print('files', len(rows), 'unique', len(set(r['file'] for r in rows)))
print('clang rc0', sum(r['clang_rc']==0 for r in rows), 'clang stderr nonempty', sum(bool(r['clang_stderr']) for r in rows))
print('import rc0', sum(r.get('import_rc')==0 for r in rows), 'import stderr nonempty', sum(bool(r.get('import_stderr')) for r in rows))
for r in rows:
    if r['clang_rc'] or r['clang_stderr'] or r.get('import_rc') or r.get('import_stderr'): print(r)
t = time.time(); r = subprocess.run([B+'/llvm-link', *lls, '-o', os.path.join(W, 'mission.bc')], capture_output=True, text=True)
print('llvm-link rc', r.returncode, '%.2fs' % (time.time()-t), r.stderr[-500:])
for dbg in (False, True):
    out = os.path.join(W, 'mission%s.mlir' % ('_dbg' if dbg else ''))
    t = time.time(); r2 = subprocess.run([B+'/mlir-translate', '--import-llvm', *(['--mlir-print-debuginfo'] if dbg else []), os.path.join(W, 'mission.bc'), '-o', out], capture_output=True, text=True)
    print('import mission dbg=%s rc=%d %.2fs stderr_lines=%d' % (dbg, r2.returncode, time.time()-t, len(r2.stderr.splitlines())))

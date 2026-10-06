#!/usr/bin/env python3
"""C17 recheck: use the gcc README-built compile DB (probe/compile_commands.cpu1.json),
same file selection regex as rw-mlir/import_exp/run_import.py, but keep all original flags
(incl. -Werror) and add only -Wno-unknown-warning-option -S -emit-llvm -O0 -g.
Then mlir-translate --import-llvm --mlir-print-debuginfo; count diagnostics."""
import json, os, re, shlex, subprocess, collections, time
W = os.path.dirname(os.path.abspath(__file__))
N = os.path.join(W, '..')
ROOT = os.path.join(N, 'probe/cFS')
DB = os.path.join(N, 'probe/compile_commands.cpu1.json')
B = '/home/user/work/llvm-project/build/bin'
OUT = os.path.join(W, 'rerun')
sel = re.compile(r'/(apps/[^/]+/fsw/src/|cfe/modules/(sb|es|tbl|evs|time)/fsw/src/)')
rows = []
for e in json.load(open(DB)):
    f = e['file']
    if not sel.search(f) or '/unit-test' in f or '/ut-' in f or '/unit-test' in e['directory']:
        continue
    args = shlex.split(e['command'])
    a = []; skip = False
    for x in args[1:]:
        if skip: skip = False; continue
        if x == '-o': skip = True; continue
        if x == '-c': continue
        a.append(x)
    tag = os.path.relpath(f, ROOT).replace('/', '__')
    ll = os.path.join(OUT, tag + '.ll'); ml = os.path.join(OUT, tag + '.mlir')
    cmd = [B + '/clang', *a, '-Wno-unknown-warning-option', '-S', '-emit-llvm', '-O0', '-g', '-o', ll]
    r1 = subprocess.run(cmd, cwd=e['directory'], capture_output=True, text=True)
    row = dict(file=os.path.relpath(f, ROOT), dir=e['directory'], clang_rc=r1.returncode, clang_stderr_lines=len(r1.stderr.splitlines()))
    if r1.returncode == 0:
        r2 = subprocess.run([B + '/mlir-translate', '--import-llvm', '--mlir-print-debuginfo', ll, '-o', ml], capture_output=True, text=True)
        row['import_rc'] = r2.returncode
        row['import_stderr_lines'] = len(r2.stderr.splitlines())
        if r2.returncode == 0:
            txt = open(ml).read()
            row['calls'] = len(re.findall(r'llvm\.call ', txt))
            row['indirect_calls'] = len(re.findall(r'llvm\.call %', txt))
            os.remove(ml)
        os.remove(ll)
    else:
        row['clang_err'] = r1.stderr.strip().splitlines()[-3:]
    rows.append(row)
json.dump(rows, open(os.path.join(W, 'rerun_results.json'), 'w'), indent=1)
print('selected', len(rows))
print('clang ok', sum(r['clang_rc']==0 for r in rows), 'clang stderr nonempty', sum(r['clang_stderr_lines']>0 for r in rows))
print('import ok', sum(r.get('import_rc')==0 for r in rows), 'import stderr nonempty', sum(r.get('import_stderr_lines',0)>0 for r in rows))
print('calls', sum(r.get('calls',0) for r in rows), 'indirect', sum(r.get('indirect_calls',0) for r in rows))
dirs = collections.Counter(r['file'].split('/')[0]+'/'+r['file'].split('/')[1 if r['file'].startswith('apps') else 2] for r in rows)
print(len(dirs), dict(dirs))
for r in rows:
    if r['clang_rc'] or r.get('import_rc') or r.get('import_stderr_lines'): print(r)

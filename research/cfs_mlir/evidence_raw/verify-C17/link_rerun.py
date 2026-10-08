#!/usr/bin/env python3
"""C17 recheck 2: gcc README compile DB, non-UT entries, same selection; clang + -Wno-unknown-warning-option
-Wno-error -O0 -g (no -disable-O0-optnone); per-file import w/ debuginfo; then llvm-link + import; count funcs."""
import json, os, re, shlex, subprocess, time
W = os.path.dirname(os.path.abspath(__file__)); N = os.path.join(W, '..')
ROOT = os.path.join(N, 'probe/cFS'); DB = os.path.join(N, 'probe/compile_commands.cpu1.json')
B = '/home/user/work/llvm-project/build/bin'; OUT = os.path.join(W, 'bc'); os.makedirs(OUT, exist_ok=True)
sel = re.compile(r'/(apps/[^/]+/fsw/src/|cfe/modules/(sb|es|tbl|evs|time)/fsw/src/)')
bcs = []; diag = 0; seen=set()
for e in json.load(open(DB)):
    f = e['file']
    if not sel.search(f) or '/unit-test' in f or '/unit-test' in e['directory'] or '/ut-' in e['directory']: continue
    if f in seen: continue
    seen.add(f)
    args = shlex.split(e['command']); a = []; skip = False
    for x in args[1:]:
        if skip: skip = False; continue
        if x == '-o': skip = True; continue
        if x == '-c': continue
        a.append(x)
    bc = os.path.join(OUT, os.path.relpath(f, ROOT).replace('/', '__') + '.bc')
    r = subprocess.run([B+'/clang', *a, '-Wno-unknown-warning-option', '-Wno-error', '-O0', '-g', '-c', '-emit-llvm', '-o', bc], cwd=e['directory'], capture_output=True, text=True)
    assert r.returncode == 0, (f, r.stderr)
    r2 = subprocess.run([B+'/mlir-translate', '--import-llvm', '--mlir-print-debuginfo', bc, '-o', '/dev/null'], capture_output=True, text=True)
    if r2.returncode or r2.stderr.strip(): diag += 1; print('IMPORT DIAG', f, r2.returncode, r2.stderr[:300])
    bcs.append(bc)
print('files', len(bcs), 'files with import rc!=0 or stderr', diag)
r = subprocess.run([B+'/llvm-link', *bcs, '-o', os.path.join(W, 'mission.bc')], capture_output=True, text=True)
print('llvm-link rc', r.returncode, r.stderr[-500:])
t = time.time(); r2 = subprocess.run([B+'/mlir-translate', '--import-llvm', os.path.join(W, 'mission.bc'), '-o', os.path.join(W, 'mission.mlir')], capture_output=True, text=True)
print('mission import rc', r2.returncode, 'secs %.2f' % (time.time()-t), 'stderr', r2.stderr[:300])
txt = open(os.path.join(W, 'mission.mlir')).read()
funcs = re.findall(r'^\s*llvm\.func\b[^\n]*', txt, re.M)
print('defined', sum(f.rstrip().endswith('{') for f in funcs), 'decl', sum(not f.rstrip().endswith('{') for f in funcs),
      'indirect', len(re.findall(r'llvm\.call %', txt)), 'calls', len(re.findall(r'llvm\.call ', txt)))

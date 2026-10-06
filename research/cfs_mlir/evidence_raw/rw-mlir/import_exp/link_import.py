import json, os, re, shlex, subprocess, time
W = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.join(W, '..', 'cfs-bundle')
DB = os.path.join(ROOT, 'build-ana/native/default_cpu1/compile_commands.json'); B = '/home/user/work/llvm-project/build/bin'
OUT = os.path.join(W, 'bc'); os.makedirs(OUT, exist_ok=True)
sel = re.compile(r'/(apps/[^/]+/fsw/src/|cfe/modules/(sb|es|tbl|evs|time)/fsw/src/)')
bcs = []
for e in json.load(open(DB)):
    f = e['file']
    if not sel.search(f) or '/unit-test' in f: continue
    args = shlex.split(e['command']); a = []; skip = False
    for x in args[1:]:
        if skip: skip = False; continue
        if x == '-o': skip = True; continue
        if x == '-c' or x.startswith('-Werror') or x.startswith('-Wno-stringop'): continue
        a.append(x)
    bc = os.path.join(OUT, os.path.relpath(f, ROOT).replace('/', '__') + '.bc')
    subprocess.run([B+'/clang', *a, '-O0', '-g', '-Xclang', '-disable-O0-optnone', '-c', '-emit-llvm', '-o', bc, '-Wno-everything'], cwd=e['directory'], check=True)
    bcs.append(bc)
t = time.time(); r = subprocess.run([B+'/llvm-link', '--only-needed=false', *bcs, '-o', os.path.join(W, 'mission.bc')], capture_output=True, text=True)
print('llvm-link rc', r.returncode, 'secs %.1f' % (time.time()-t)); print(r.stderr[-1500:])
if r.returncode == 0:
    t = time.time(); r2 = subprocess.run(['/usr/bin/time', '-v', B+'/mlir-translate', '--import-llvm', os.path.join(W, 'mission.bc'), '-o', os.path.join(W, 'mission.mlir')], capture_output=True, text=True)
    print('import rc', r2.returncode, 'secs %.1f' % (time.time()-t))
    print('\n'.join(l for l in r2.stderr.splitlines() if 'Maximum resident' in l or 'warning' in l or 'error' in l)[:1500])

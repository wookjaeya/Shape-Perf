#!/usr/bin/env python3
"""For each FSW C file in the cFS cpu1 compile DB: clang -O0 -g -emit-llvm, then
mlir-translate --import-llvm --mlir-print-debuginfo. Record exit codes, import
warnings, op counts, and how many ops carry a file:line:col location."""
import json, os, re, shlex, subprocess, sys, collections
W = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(W, '..', 'cfs-bundle')
DB = os.path.join(ROOT, 'build-ana/native/default_cpu1/compile_commands.json')
B = '/home/user/work/llvm-project/build/bin'
OUT = os.path.join(W, 'out'); os.makedirs(OUT, exist_ok=True)
sel = re.compile(r'/(apps/[^/]+/fsw/src/|cfe/modules/(sb|es|tbl|evs|time)/fsw/src/)')
db = json.load(open(DB))
rows = []
for e in db:
    f = e['file']
    if not sel.search(f) or '/unit-test' in f or '/ut-' in f:
        continue
    args = shlex.split(e['command']) if 'command' in e else e['arguments']
    # drop compiler, -o X, -c, and Werror; add IR emission flags
    a = []; skip = False
    for x in args[1:]:
        if skip: skip = False; continue
        if x == '-o': skip = True; continue
        if x in ('-c',) or x.startswith('-Werror') or x.startswith('-Wno-stringop'): continue
        a.append(x)
    tag = os.path.relpath(f, ROOT).replace('/', '__')
    ll = os.path.join(OUT, tag + '.ll'); ml = os.path.join(OUT, tag + '.mlir')
    r1 = subprocess.run([B + '/clang', *a, '-O0', '-g', '-Xclang', '-disable-O0-optnone', '-S', '-emit-llvm', '-o', ll, '-Wno-everything'],
                        cwd=e['directory'], capture_output=True, text=True)
    row = dict(file=os.path.relpath(f, ROOT), clang_rc=r1.returncode)
    if r1.returncode == 0:
        r2 = subprocess.run([B + '/mlir-translate', '--import-llvm', '--mlir-print-debuginfo', ll, '-o', ml], capture_output=True, text=True)
        row['import_rc'] = r2.returncode
        warns = [l for l in r2.stderr.splitlines() if 'warning:' in l or 'error:' in l]
        row['import_diags'] = len(warns)
        row['diag_kinds'] = collections.Counter(re.sub(r'.*(warning|error): ', r'\1: ', w)[:70] for w in warns).most_common(3)
        if r2.returncode == 0:
            txt = open(ml).read()
            ops = re.findall(r'^\s+(?:%[^=]+= )?"?(llvm\.[a-z_.]+)"?.*loc\((#loc\d*|"[^"]*"[^)]*|fused[^)]*|unknown)\)\s*$', txt, re.M)
            locdefs = dict(re.findall(r'^(#loc\d*) = loc\((.*)\)$', txt, re.M))
            def known(l):
                if l.startswith('#loc'): l = locdefs.get(l, 'unknown')
                return 'unknown' not in l
            row['ops_with_loc_suffix'] = len(ops)
            row['ops_known_loc'] = sum(1 for _, l in ops if known(l))
            row['calls'] = len(re.findall(r'llvm\.call ', txt))
            row['indirect_calls'] = len(re.findall(r'llvm\.call %', txt))
            row['di_members'] = len(re.findall(r'tag = DW_TAG_member', txt))
            os.remove(ll)
    else:
        row['clang_err'] = r1.stderr.strip().splitlines()[-1:] 
    rows.append(row)
json.dump(rows, open(os.path.join(W, 'import_results.json'), 'w'), indent=1)
ok = [r for r in rows if r.get('import_rc') == 0]
print('files selected:', len(rows))
print('clang ok:', sum(r['clang_rc'] == 0 for r in rows), ' import ok:', len(ok))
print('files with import diagnostics:', sum(1 for r in rows if r.get('import_diags')))
tot = sum(r['ops_with_loc_suffix'] for r in ok); kn = sum(r['ops_known_loc'] for r in ok)
print('ops (with loc printed):', tot, ' with known file:line:col:', kn, f'({100*kn/max(tot,1):.1f}%)')
print('direct+indirect calls:', sum(r['calls'] for r in ok), ' indirect calls:', sum(r['indirect_calls'] for r in ok))
for r in rows:
    if r.get('import_rc', 0) != 0 or r['clang_rc'] != 0 or r.get('import_diags'):
        print('  ', r['file'], r.get('clang_rc'), r.get('import_rc'), r.get('diag_kinds') or r.get('clang_err'))

#!/usr/bin/env python3
"""Re-run compile_commands.json entries of one cFS app with clang to emit LLVM IR.
usage: emit_ir.py <app_name> [extra clang flags...]
Writes ir/<app>/<file>.ll and a log of the exact commands."""
import json, shlex, subprocess, sys, os, time
ROOT = os.path.dirname(os.path.abspath(__file__))
CC = '/home/user/work/llvm-project/build/bin/clang'
db = json.load(open(os.path.join(ROOT, 'cFS/build-native_std/native/default_cpu1/compile_commands.json')))
app = sys.argv[1]
extra = sys.argv[2:]
out = os.path.join(ROOT, 'ir', app); os.makedirs(out, exist_ok=True)
sel = [e for e in db if f'/apps/{app}/fsw/src/' in e['file'] and '/unit-test' not in e['directory']]
log = open(os.path.join(out, 'commands.log'), 'w')
for e in sel:
    a = shlex.split(e['command'])
    args = [CC]; skip = False
    for i, x in enumerate(a[1:]):
        if skip: skip = False; continue
        if x == '-o': skip = True; continue
        if x == '-c': continue
        if x == e['file']: continue
        args.append(x)
    base = os.path.basename(e['file'])[:-2]
    ll = os.path.join(out, base + '.ll')
    args += ['-S', '-emit-llvm', '-O0', '-g'] + extra + [e['file'], '-o', ll]
    t = time.time()
    r = subprocess.run(args, cwd=e['directory'], capture_output=True, text=True)
    dt = time.time() - t
    log.write(f"# {base}.c rc={r.returncode} t={dt:.2f}s\ncd {e['directory']} && {shlex.join(args)}\n{r.stderr}\n")
    print(f"{base}.c rc={r.returncode} t={dt:.2f}s stderr_lines={len(r.stderr.splitlines())}")
    if r.stderr: print(r.stderr[:1500])

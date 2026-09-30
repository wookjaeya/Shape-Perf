"""Collect per-artifact static ELF info from readelf output (no project tooling used)."""
import json, re, subprocess, sys

PATHS = [
    '/home/user/work/v3/g2/K/L0064/S8_full/model.so',
    '/home/user/work/v3/g2/K/L0064/S1_full/model.so',
] + [f'/home/user/work/v3_followup/e1/K_L0064_tagged/{n}_full/model.so'
     for n in ('S8_alpha', 'S1_bravo', 'S8_bravo', 'S1_alpha')]


def run(*a):
    return subprocess.run(a, capture_output=True, text=True, check=True).stdout


info = {}
for p in PATHS:
    dyn, funcs = {}, {}
    for line in run('readelf', '-W', '--dyn-syms', p).splitlines():
        f = line.split()
        if len(f) >= 8 and f[0].rstrip(':').isdigit() and f[6] != 'UND':
            name = f[7].split('@')[0]
            dyn[name] = int(f[1], 16)
            funcs[name] = [int(f[1], 16), int(f[2])]
    js, gd = {}, {}
    for line in run('readelf', '-rW', p).splitlines():
        m = re.match(r'^([0-9a-f]{16})\s+[0-9a-f]+\s+(R_X86_64_JUMP_SLOT|R_X86_64_GLOB_DAT)\s+([0-9a-f]+)\s+(\S+)', line)
        if m:
            off, typ, val, name = int(m.group(1), 16), m.group(2), int(m.group(3), 16), m.group(4).split('@')[0]
            # keep only slots whose symbol this model itself DEFINES (the ones that can be interposed
            # by another model); libc imports are irrelevant here.
            if val != 0:
                (js if typ.endswith('JUMP_SLOT') else gd)[name] = off
    plt = None
    for line in run('readelf', '-SW', p).splitlines():
        m = re.search(r'\]\s+\.plt\s+PROGBITS\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)', line)
        if m:
            s = int(m.group(1), 16)
            plt = [s, s + int(m.group(2), 16)]
    info[p] = {'dynsym': dyn, 'funcs': funcs, 'jslot': js, 'gdat': gd, 'plt': plt}
json.dump(info, open(sys.argv[1], 'w'), indent=1)
for p, d in info.items():
    print(p, 'defined', len(d['dynsym']), 'jslot(model-defined)', len(d['jslot']), 'globdat(model-defined)', len(d['gdat']), 'plt', d['plt'])

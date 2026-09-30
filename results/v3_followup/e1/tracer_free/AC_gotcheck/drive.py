import json, os, subprocess, sys

D = os.path.dirname(os.path.abspath(__file__))
PY = '/home/user/work/venv/bin/python'
U = '/home/user/work/v3/g2/K/L0064'
T = '/home/user/work/v3_followup/e1/K_L0064_tagged'
ARTS = {
    'S8': {'path': f'{U}/S8_full/model.so', 'tag': None},
    'S1': {'path': f'{U}/S1_full/model.so', 'tag': None},
    'S8a': {'path': f'{T}/S8_alpha_full/model.so', 'tag': 'alpha'},
    'S1b': {'path': f'{T}/S1_bravo_full/model.so', 'tag': 'bravo'},
    'S1a': {'path': f'{T}/S1_alpha_full/model.so', 'tag': 'alpha'},
}


def L(*a):
    return [['load', k] for k in a]


def C(*a):
    return [['call', k] for k in a]


SC = {
    # (1) claim A: only one artifact in the process
    'ISO_S1': L('S1') + C('S1', 'S1'),
    'ISO_S8': L('S8') + C('S8', 'S8'),
    # (2) claim C: distinct tags, both load orders x both first-call orders
    'T_L8a1b_C8a': L('S8a', 'S1b') + C('S8a', 'S1b', 'S8a', 'S1b'),
    'T_L8a1b_C1b': L('S8a', 'S1b') + C('S1b', 'S8a', 'S1b', 'S8a'),
    'T_L1b8a_C8a': L('S1b', 'S8a') + C('S8a', 'S1b', 'S8a', 'S1b'),
    'T_L1b8a_C1b': L('S1b', 'S8a') + C('S1b', 'S8a', 'S1b', 'S8a'),
    'T_SEQ_8a_1b': L('S8a') + C('S8a') + L('S1b') + C('S1b', 'S8a'),
    'T_SEQ_1b_8a': L('S1b') + C('S1b') + L('S8a') + C('S8a', 'S1b'),
    # (3) claim C second half: same tag in one process
    'SAME_L8a1a_C1a': L('S8a', 'S1a') + C('S1a', 'S8a'),
    'SAME_L8a1a_C8a': L('S8a', 'S1a') + C('S8a', 'S1a'),
    'SAME_L1a8a_C8a': L('S1a', 'S8a') + C('S8a', 'S1a'),
    'SAME_L1a8a_C1a': L('S1a', 'S8a') + C('S1a', 'S8a'),
    # positive control: untagged co-load (method must be able to see a mis-binding)
    'CTRL_L81_C1': L('S8', 'S1') + C('S1', 'S8'),
    'CTRL_L18_C8': L('S1', 'S8') + C('S8', 'S1'),
}

os.makedirs(f'{D}/runs', exist_ok=True)
for mode in ('lazy', 'bindnow'):
    for name, steps in SC.items():
        used = sorted({k for _, k in steps})
        spec = {'name': name, 'arts': {k: ARTS[k] for k in used}, 'steps': steps}
        sp = f'{D}/runs/{name}.spec.json'
        json.dump(spec, open(sp, 'w'))
        env = dict(os.environ)
        env.pop('LD_BIND_NOW', None)
        if mode == 'bindnow':
            env['LD_BIND_NOW'] = '1'
        r = subprocess.run([PY, f'{D}/probe.py', f'{D}/static.json', sp], capture_output=True, text=True, env=env)
        out = f'{D}/runs/{name}.{mode}.json'
        if r.returncode != 0:
            print(name, mode, 'FAILED rc', r.returncode, r.stderr[-2000:])
            continue
        # the runtime may print to stdout; the probe's JSON is the last line
        open(out, 'w').write(r.stdout.strip().splitlines()[-1])
        print(name, mode, 'ok')

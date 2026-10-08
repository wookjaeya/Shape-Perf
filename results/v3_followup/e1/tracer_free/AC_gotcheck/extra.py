import json, os, subprocess
D = os.path.dirname(os.path.abspath(__file__))
T = '/home/user/work/v3_followup/e1/K_L0064_tagged'
A = {'S8a': {'path': f'{T}/S8_alpha_full/model.so', 'tag': 'alpha'}, 'S8b': {'path': f'{T}/S8_bravo_full/model.so', 'tag': 'bravo'},
     'S1a': {'path': f'{T}/S1_alpha_full/model.so', 'tag': 'alpha'}, 'S1b': {'path': f'{T}/S1_bravo_full/model.so', 'tag': 'bravo'}}
SC = {'X_L8b1a_C1a': [['load','S8b'],['load','S1a'],['call','S1a'],['call','S8b']],
      'X_L1a8b_C8b': [['load','S1a'],['load','S8b'],['call','S8b'],['call','S1a']],
      'X_L8a8b_C8b': [['load','S8a'],['load','S8b'],['call','S8b'],['call','S8a']],
      'X_L8b8a_C8a': [['load','S8b'],['load','S8a'],['call','S8a'],['call','S8b']],
      'X_SAME_L1b8b_C8b': [['load','S1b'],['load','S8b'],['call','S8b'],['call','S1b']]}
for name, st in SC.items():
    sp = f'{D}/runs/{name}.spec.json'
    json.dump({'name': name, 'arts': {k: A[k] for k in sorted({k for _, k in st})}, 'steps': st}, open(sp, 'w'))
    env = {k: v for k, v in os.environ.items() if k != 'LD_BIND_NOW'}
    r = subprocess.run(['/home/user/work/venv/bin/python', f'{D}/probe.py', f'{D}/static.json', sp], capture_output=True, text=True, env=env)
    assert r.returncode == 0, r.stderr[-1500:]
    res = json.loads(r.stdout.strip().splitlines()[-1])
    open(f'{D}/runs/{name}.lazy.json', 'w').write(json.dumps(res))
    for e in res['log']:
        if e['when'].startswith('call'):
            c = e['chain_after_call']; k = e['when'].split()[1]
            ks = [x for x in c if x.startswith('ciface_slot_in_')][0]
            print(name, e['when'], 'exact', e['output_exact'], 'entry', c['entry(dlsym handle)']['dso'], 'ciface->', c[ks]['dso'], 'compute->', c['compute_dso'], 'mem==disk of', c['compute_mem_matches_disk_of'], 'OWN' if c['compute_dso'] == k else 'OTHER')

import glob, json, os, sys

D = os.path.dirname(os.path.abspath(__file__))
order = ['ISO_S1', 'ISO_S8', 'T_L8a1b_C8a', 'T_L8a1b_C1b', 'T_L1b8a_C8a', 'T_L1b8a_C1b', 'T_SEQ_8a_1b', 'T_SEQ_1b_8a',
         'SAME_L8a1a_C1a', 'SAME_L8a1a_C8a', 'SAME_L1a8a_C8a', 'SAME_L1a8a_C1a', 'CTRL_L81_C1', 'CTRL_L18_C8']
summary = []
for mode in ('lazy', 'bindnow'):
    print(f'######## {mode}')
    for name in order:
        r = json.load(open(f'{D}/runs/{name}.{mode}.json'))
        print(f'== {name} pid={r["pid"]} bases=' + ', '.join(f'{k}@{v["base"]}' for k, v in r['loaded'].items()))
        for e in r['log']:
            if e['when'].startswith('call'):
                c = e['chain_after_call']
                k = e['when'].split()[1]
                ks = [x for x in c if x.startswith('ciface_slot_in_')][0]
                ms = [x for x in c if x.startswith('main_graph_slot_in_')]
                ent = c['entry(dlsym handle)']
                cif = c[ks]
                mg = c[ms[0]] if ms else None
                own = (ent['dso'] == k and c.get('compute_dso') == k)
                line = (f'   {e["when"]:10s} out_exact={e["output_exact"]}  entry={ent["dso"]}:{ent["sym"]}  '
                        f'{ks}={cif["dso"]}:{cif["sym"]}@{cif["rel"]}  '
                        + (f'{ms[0]}={mg["dso"]}:{mg["sym"]}@{mg["rel"]}  ' if mg else '')
                        + f'compute_mem_matches_disk_of={c.get("compute_mem_matches_disk_of")}  => {"OWN" if own else "OTHER(" + str(c.get("compute_dso")) + ")"}')
                print(line)
                summary.append({'mode': mode, 'case': name, 'call': k, 'entry_dso': ent['dso'], 'ciface_dso': cif['dso'],
                                'compute_dso': c.get('compute_dso'), 'verdict': 'own' if own else 'other',
                                'output_exact': e['output_exact']})
        # final snapshot: where did every model-defined slot of each DSO resolve?
        last = [e for e in r['log'] if 'slots' in e][-1]
        for k, d in last['slots'].items():
            cross = {n: v['dso'] for n, v in d.items() if v['dso'] != k and v['dso'] in r['loaded']}
            unres = [n for n, v in d.items() if v['sym'] and 'UNRESOLVED' in v['sym'][0]]
            print(f'   final slots of {k}: cross-DSO={cross if cross else "none"}; unresolved={unres}')
json.dump(summary, open(f'{D}/summary.json', 'w'), indent=1)

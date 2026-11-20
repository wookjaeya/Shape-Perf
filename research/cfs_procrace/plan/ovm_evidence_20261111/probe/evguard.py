#!/usr/bin/env python3
# Probe 2b: do the facts hold what label R needs for "sends an event whose type argument is the constant ERROR/CRITICAL
# on the absence branch"?  Counts event-send call sites by constant type and whether ANY branch condition is recorded for the site
# (the [calls] section has no guard column; a guard exists only on a READS row that is an argument of the site).
# usage: python3 -I evguard.py <facts.txt>
import re, sys, collections
sec = None; sites = {}; cur = None; guard_sites = set(); read_sites = set(); term = collections.Counter(); exitargs = collections.Counter()
EV = {'CFE_EVS_SendEvent': 1, 'CFE_EVS_SendEventWithAppID': 1, 'CFE_EVS_SendTimedEvent': 2}
flags = collections.Counter()
for line in open(sys.argv[1], errors='replace'):
    if line.startswith('['):
        sec = line.split(']')[0][1:]; cur = None; continue
    if sec == 'calls':
        m = re.match(r'  site (\d+) (\S+) (\S+) -> (\S+) (\S+) \{([^}]*)\}', line)
        if m:
            cur = None
            if m.group(4) in EV: cur = int(m.group(1)); sites[cur] = {'callee': m.group(4), 'pos': m.group(2), 'type': None}
            elif m.group(4) == 'CFE_ES_ExitApp': cur = -int(m.group(1)) - 1; sites[cur] = {'callee': 'CFE_ES_ExitApp', 'pos': m.group(2), 'type': None}
            continue
        m = re.match(r'      arg (\d+):(\S+) ?(.*)$', line)
        if m and cur is not None:
            want = EV.get(sites[cur]['callee'], 0)
            if int(m.group(1)) == want: sites[cur]['type'] = (m.group(2), m.group(3).strip())
    elif sec == 'reads':
        m = re.search(r'arg \d+ of site (\d+) ', line)
        if m:
            read_sites.add(int(m.group(1)))
            if ' guard=' in line and ' guard=-' not in line: guard_sites.add(int(m.group(1)))
ev = {k: v for k, v in sites.items() if k >= 0}
bytype = collections.Counter(); g = collections.Counter()
for k, v in ev.items():
    t = v['type']
    key = (t[1].split()[0] if t and t[0] == 'const' else 'non-constant')
    bytype[key] += 1
    if k in guard_sites: g[key] += 1
print('event-send call sites', len(ev), 'by constant type argument', dict(sorted(bytype.items())))
print('  of them with a branch condition recorded on a field-read argument of the site', dict(sorted(g.items())))
err = [k for k, v in ev.items() if v['type'] and v['type'][0] == 'const' and v['type'][1].split()[0] in ('3', '4')]
print('  ERROR/CRITICAL sends', len(err), 'with a recorded guard', sum(1 for k in err if k in guard_sites), 'without', sum(1 for k in err if k not in guard_sites))
ex = {k: v for k, v in sites.items() if k < 0}
print('CFE_ES_ExitApp call sites', len(ex), 'argument kinds', dict(collections.Counter((v['type'][0] + ' ' + v['type'][1][:40]) if v['type'] else 'none' for v in ex.values())))

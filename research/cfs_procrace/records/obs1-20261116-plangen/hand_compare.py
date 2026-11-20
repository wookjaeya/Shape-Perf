#!/usr/bin/env python3
# hand_compare.py -- site-by-site comparison of a generated plan with a hand-written site manifest.  Reads only.
#   python3 -I hand_compare.py <hand.kzs> <generated plan.kzs> <generated plan.side>
# A hand site and a generated site correspond when the function, the place (or the phase of an entry or return site)
# and the callee are the same, the unit paths end the same way, and (for at-call, after-call and store) the line is the
# same.  The hand manifest's phase=after (the returns of a function, kzs-sites 1 meaning) has no generated counterpart
# kind in this revision (the generator plans after-call sites at the callers instead); it is matched against an
# after-call of the same function in any caller and reported so.
import re, sys

def sites(path):
    out = []
    for line in open(path):
        m = re.match(r'^site (\d+) (.*)$', line.strip())
        if not m:
            continue
        kv = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', m.group(2)))
        kv = {k: v.strip('"') for k, v in kv.items()}
        kv['id'] = m.group(1)
        out.append(kv)
    return out

def tail_eq(a, b):
    return a == b or a.endswith('/' + b) or b.endswith('/' + a)

hand, gen = sites(sys.argv[1]), sites(sys.argv[2])
side = open(sys.argv[3]).read()
used = set()
print('hand %s: %d sites; generated: %d sites' % (sys.argv[1].split('/')[-1], len(hand), len(gen)))
for h in hand:
    place = h.get('place', '')
    found = []
    for g in gen:
        if h['phase'] == 'after' and not place:
            # the returns of h.func: an after-call of it in a caller
            if g.get('place') == 'after-call' and g.get('callee') == h['func']:
                found.append((g['id'], 'after-call of it in %s' % g['func']))
            continue
        if g['func'] != h['func'] or not tail_eq(g['tu'], h['tu']):
            continue
        if place:
            if g.get('place') == place and g.get('callee', '') == h.get('callee', '') and g.get('line') == h.get('line'):
                found.append((g['id'], 'same'))
        elif h['phase'] == 'before' and g['phase'] == 'before' and not g.get('place'):
            found.append((g['id'], 'same (entry)'))
    for gid, _ in found:
        used.add(gid)
    desc = '%s %s %s%s%s' % (h['label'], h['func'], place or h['phase'], (' ' + h['callee']) if h.get('callee') else '', (' line ' + h['line']) if h.get('line') else '')
    print('%-6s hand %3s %-90s %s' % ('MATCH' if found else 'MISS', h['id'], desc, '; '.join('gen %s %s' % f for f in found)))
extra = [g for g in gen if g['id'] not in used]
print('generated sites with no hand counterpart: %d' % len(extra))
import collections
print('  by rule: %s' % dict(sorted(collections.Counter(r for g in extra for r in g['label'].split('.', 1)[1].split('+')).items())))

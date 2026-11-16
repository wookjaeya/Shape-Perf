#!/usr/bin/env python3
# priority_agreement.py -- the agreement of the generator's port of the method priority (the `row` lines of a plan
# sidecar) with the probe's per-row table (plan/ovm_rev1_20261116/probe/s1_rev1_<build>.rows.tsv), row by row (the probe
# names a row by its number in the step-2a relations text, the sidecar by its use-site key and the same number).
#   python3 -I priority_agreement.py <sidecar> <rows.tsv>
# Prints one line per field (label, order class, reach, disposition, tier-1 membership): rows equal / rows differing,
# and every differing row.  rc 0 = all equal.
import sys
side, tsv = sys.argv[1], sys.argv[2]
port = {}
for line in open(side):
    if not line.startswith('row '):
        continue
    f = line.split()
    kv = dict(x.split('=', 1) for x in f[3:] if '=' in x and not x.startswith('why='))
    port[f[2]] = kv
probe = {}
hdr = None
for line in open(tsv):
    c = line.rstrip('\n').split('\t')
    if hdr is None:
        hdr = c
        continue
    probe[c[0]] = dict(zip(hdr, c))
bad = 0
print('rows: port %d, probe %d, in both %d' % (len(port), len(probe), len(set(port) & set(probe))))
if set(port) != set(probe):
    print('DIFF row sets: only port %s, only probe %s' % (sorted(set(port) - set(probe))[:10], sorted(set(probe) - set(port))[:10]))
    bad += 1
for name, pk, qk in (('label', 'label', 'label'), ('order class', 'order', 'order_class'), ('reach', 'reach', 'reach'), ('disposition', 'disposition', 'disposition')):
    diff = [r for r in sorted(set(port) & set(probe)) if port[r][pk] != probe[r][qk]]
    print('%-12s equal %d, differing %d %s' % (name, len(set(port) & set(probe)) - len(diff), len(diff), ['%s %s/%s' % (r, port[r][pk], probe[r][qk]) for r in diff][:20]))
    bad += len(diff)
t1 = [r for r in port if port[r]['tier'] == '1']
d7 = [r for r in probe if probe[r]['priority_candidate'] == 'yes']
print('tier 1 (port) %d, D07 (probe) %d, same set %s' % (len(t1), len(d7), sorted(t1) == sorted(d7)))
if sorted(t1) != sorted(d7):
    bad += 1
rel = [r for r in port if port[r]['order'] == 'RELOC']
drc = [r for r in probe if probe[r]['directed_candidate'] == 'yes']
print('order class RELOC (port) %d, directed candidates (probe) %d, same set %s' % (len(rel), len(drc), sorted(rel) == sorted(drc)))
if sorted(rel) != sorted(drc):
    bad += 1
print('PASS' if bad == 0 else 'FAIL %d' % bad)
sys.exit(0 if bad == 0 else 1)

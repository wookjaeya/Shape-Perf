#!/usr/bin/env python3
# Probe 1: failure vocabulary, extracted mechanically from framework headers.
# usage: python3 -I vocab.py <cfe_error.h> <osapi-error.h> <evs_typedefs.h> <es_typedefs.h> [psp_error.h]
import re, sys, collections
def defs(path):
    out = []
    for n, line in enumerate(open(path, errors='replace'), 1):
        m = re.match(r'\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)(\([^)]*\))?\s*(.*)$', line)
        if m:
            val = re.sub(r'/\*.*', '', m.group(3)).strip()
            out.append((n, m.group(1), m.group(2), val))
    return out
cfe, osal, evs, es = sys.argv[1:5]
print('== cfe_error.h ==')
D = defs(cfe)
print('defines total', len(D))
codes = []; other = []
for n, name, args, val in D:
    m = re.fullmatch(r'\(\((CFE_Status_t|int32)\)\s*(0[xX][0-9a-fA-F]+|\d+)\)', val)
    if m and not args:
        codes.append((n, name, m.group(1), int(m.group(2), 0), m.group(2)))
    else:
        other.append((n, name, args, val))
print('value-cast defines', len(codes), 'other defines', len(other))
for o in other: print('  other:', o)
masks = [c for c in codes if re.match(r'CFE_(SEVERITY_|SERVICE_BITMASK|[A-Z_]+_SERVICE$)', c[1])]
status = [c for c in codes if c not in masks]
print('mask/field defines', len(masks), [m[1] for m in masks])
print('status codes (incl. CFE_SUCCESS)', len(status))
sev = collections.Counter(); srv = collections.Counter(); cast = collections.Counter()
bysev = collections.defaultdict(list)
for n, name, c, v, txt in status:
    s = (v >> 30) & 3
    sev[s] += 1; bysev[s].append((name, txt, n)); cast[c] += 1
    srv[(v >> 25) & 7] += 1
print('by severity bits (31-30): 00=%d 01=%d 10=%d 11=%d' % (sev[0], sev[1], sev[2], sev[3]))
print('cast type:', dict(cast))
print('by service bits (27-25):', dict(sorted(srv.items())))
print('severity 00 members:', bysev[0])
print('severity 10 members:', bysev[2])
print('severity 01 members:'); 
for x in bysev[1]: print('  ', x)
# reserved bits / mission bits nonzero?
res = [(name, txt) for n, name, c, v, txt in status if (v >> 28) & 3]
mis = [(name, txt) for n, name, c, v, txt in status if (v >> 16) & 0x1ff]
print('reserved bits 29-28 nonzero:', res)
print('mission bits 24-16 nonzero:', mis)
# duplicates
vals = collections.defaultdict(list)
for n, name, c, v, txt in status: vals[v].append(name)
dup = {hex(k): v for k, v in vals.items() if len(v) > 1}
print('distinct values', len(vals), 'duplicated values', dup)
# name-vs-class mismatches: severity 11 whose name lacks ERR/error hint is NOT a test; list names with INFO/WARN and severity 11, and names with ERR and severity 01
mm1 = [(name, txt) for n, name, c, v, txt in status if ((v >> 30) & 3) == 3 and re.search(r'INFO|WARN', name)]
mm2 = [(name, txt) for n, name, c, v, txt in status if ((v >> 30) & 3) == 1 and re.search(r'ERR', name)]
print('sev 11 but name has INFO/WARN:', mm1)
print('sev 01 but name has ERR:', mm2)
print('== osapi-error.h ==')
O = defs(osal)
neg = []; zero = []; pos = []; oth = []
for n, name, args, val in O:
    m = re.fullmatch(r'\(\s*(-?\d+)\s*\)', val)
    if m and not args and name.startswith('OS_'):
        v = int(m.group(1)); (neg if v < 0 else zero if v == 0 else pos).append((name, v, n))
    else: oth.append((n, name, args, val))
print('defines total', len(O), 'negative', len(neg), 'zero', len(zero), 'positive', len(pos), 'other', len(oth))
print('zero:', zero); print('positive:', pos); print('other:', oth)
print('min', min(v for _, v, _ in neg), 'max-neg', max(v for _, v, _ in neg))
nv = collections.Counter(v for _, v, _ in neg); print('duplicated negative values:', {k: c for k, c in nv.items() if c > 1})
print('negative names with TIMEOUT/EMPTY/FULL:', [(a, b) for a, b, _ in neg if re.search(r'TIMEOUT|EMPTY|FULL', a)])
def enum(path, pat):
    out = []
    for n, line in enumerate(open(path, errors='replace'), 1):
        m = re.match(r'\s*(' + pat + r'[A-Za-z0-9_]*)\s*=\s*(-?\d+|0[xX][0-9a-fA-F]+)\s*,?', line)
        if m: out.append((m.group(1), int(m.group(2), 0), n))
    return out
print('== EVS event types =='); 
for x in enum(evs, 'CFE_EVS_EventType_'): print('  ', x)
print('== ES RunStatus =='); 
for x in enum(es, 'CFE_ES_RunStatus_'): print('  ', x)
if len(sys.argv) > 5:
    print('== cfe_psp_error.h ==')
    Pd = defs(sys.argv[5]); pn = []
    for n, name, args, val in Pd:
        m = re.fullmatch(r'\(\(int32\)\s*(-?\d+)\)|\(\s*(-?\d+)\s*\)', val)
        if m: pn.append((name, int(m.group(1) or m.group(2)), n))
    print('numeric defines', len(pn), 'negative', sum(1 for _, v, _ in pn if v < 0), 'zero', [x for x in pn if x[1] == 0], 'positive', [x for x in pn if x[1] > 0])

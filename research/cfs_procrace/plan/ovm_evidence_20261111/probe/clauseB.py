#!/usr/bin/env python3
# Probe 3: apply clause B (doc 3.3) items 2-4 to an existing R2 replay text.  usage: python3 -I clauseB.py <cur.txt>
import re, sys, collections
f = sys.argv[1]; pub = None; occ = []; cur = None
for n, line in enumerate(open(f, errors='replace'), 1):
    m = re.match(r'\s+#(\d+) (\S+) sb_publication pt', line)
    if m and pub is None: pub = (int(m.group(1)), m.group(2), n)
    m = re.match(r'occurrence (\S+) actor (\S+) ordinal (\d+) key \[([^\]]*)\] call #(\d+) return #(\d+)', line)
    if m:
        cur = {'kind': m.group(1), 'actor': m.group(2), 'key': m.group(4), 'call': int(m.group(5)), 'ret': int(m.group(6)), 'result': None, 'line': n, 'status': None}
        occ.append(cur); continue
    m = re.match(r'occurrence (\S+) actor (\S+) ordinal (\d+) key \[([^\]]*)\] call #(\d+)', line)
    if m:
        cur = {'kind': m.group(1), 'actor': m.group(2), 'key': m.group(4), 'call': int(m.group(5)), 'ret': None, 'result': None, 'line': n, 'status': None}
        occ.append(cur); continue
    if cur is not None:
        m = re.match(r'\s+result (\S+)', line)
        if m and cur['result'] is None: cur['result'] = m.group(1)
        m = re.match(r'\s+status (\S+)', line)
        if m and cur['status'] is None: cur['status'] = m.group(1)
def cls(r):
    if r is None: return 'no-result-line'
    v = int(r, 16) if r.startswith('0x') else int(r)
    if v >= 0xC0000000 or v < 0: return 'fail'
    if 0x40000000 <= v < 0x80000000: return 'info'
    return 'ok'
print('publication w (first sb_publication point): trace index #%d thread %s (text line %d)' % pub)
print('occurrences', len(occ), 'kinds', dict(collections.Counter(o['kind'] for o in occ)))
before = [o for o in occ if o['call'] < pub[0]]; after = [o for o in occ if o['call'] > pub[0]]
for name, S in (('before w', before), ('after w', after)):
    print(name, len(S))
    print('   by actor', dict(collections.Counter(o['actor'] for o in S).most_common(12)))
    print('   by key (AppID argument)', dict(collections.Counter(o['key'] for o in S)))
    print('   by result', dict(collections.Counter(str(o['result']) for o in S)))
    print('   by grade', dict(collections.Counter(cls(o['result']) for o in S)))
    print('   by replay status', dict(collections.Counter(str(o['status']) for o in S)))
b_fail = [o for o in before if cls(o['result']) == 'fail']
print('clause B item 2 (another actor used the site before w, all such occurrences fail):', 'before-w occurrences', len(before), 'fail', len(b_fail), 'non-fail', len(before) - len(b_fail), 'actors', sorted(set(o['actor'] for o in before)))
print('clause B item 3 (after w not empty, none returned the item-2 status):', 'after-w', len(after), 'with 0xc2000003', sum(1 for o in after if o['result'] == '0xc2000003'), 'with any fail grade', sum(1 for o in after if cls(o['result']) == 'fail'), 'without a result line', sum(1 for o in after if o['result'] is None))
print('first/last before-w text lines', before[0]['line'] if before else None, before[-1]['line'] if before else None, '; first after-w text line', after[0]['line'] if after else None)
print('before-w occurrences with a return event (ret index):', sum(1 for o in before if o['ret'] is not None), '; after-w', sum(1 for o in after if o['ret'] is not None))

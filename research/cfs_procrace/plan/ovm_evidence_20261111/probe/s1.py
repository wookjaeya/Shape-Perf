#!/usr/bin/env python3
# Probe 2: apply the S1 label definitions of the method (doc section 3.3) to the step 2a relation rows,
# using ONLY what a row's text holds.  usage: python3 -I s1.py <relations.txt> [--rows] [--show ROW,ROW]
# Query (the operationalisation is mine; each rule is stated here):
#  fail-class constant (R-CLASS): v < 0, or 0xC0000000 <= v <= 0xFFFFFFFF.  info: 0x40000000 <= v < 0x80000000.
#  path A (lookup use):  absconst = constants after "search returns:" on the flip line
#       H  if a "probe-create" line, or an enclosing "handling: in the loop ... exit E delay D" line whose exit E compares a local with a fail-class constant (retry on the failing status, with a delay)
#       R  else if a "handling: terminal" line                       (terminal chain; ERROR-event sends are not in rows)
#       J  else if a fail-class absconst and result=discarded
#       T? else if a fail-class absconst and result=tested           (branch body not in the row: apparent T)
#       E  else if a fail-class absconst (result=used only)          (UNDECIDED-EVAL)
#       0  else if absconst present but none fail-class;  E if no absconst at all
#  path B (shared field as argument): absconst = constants of "callee-check: F returns V"; handling: discarded/used/tested counts
#       J  if fail-class absconst and discarded>0 ;  T? if fail-class and discarded==0 and tested>0 ; E if fail-class and only used
#       0  if callee-check without fail-class constant ; E if no callee-check line
#  path C (write after spawn / child read): absconst = constants in the flip line "F returns V [N] with the earlier value"
#       F  if a fail-class constant "with the earlier value" (caller handling is NOT in the row -> R/J not decidable from the row)
#       0  if constants but none fail-class with the earlier value ; E if flip gives no constant
#  order column: A = unordered (always "undecidable"); B = fixed? when "reader actors created before it:" names an actor,
#       else unordered; C = enforced when "proven-impossible", else unordered/undecidable.
import re, sys, collections
path = sys.argv[1]; showrows = '--rows' in sys.argv
show = set()
for i, a in enumerate(sys.argv):
    if a == '--show': show = set(sys.argv[i + 1].split(','))
def cls(v):
    if v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF: return 'fail'
    if 0x40000000 <= v < 0x80000000: return 'info'
    return 'ok'
rows = []; cur = None
for line in open(path, errors='replace'):
    m = re.match(r'^  ([ABC])(\d+) (.*)$', line)
    if m:
        cur = {'id': m.group(1) + m.group(2), 'p': m.group(1), 'head': m.group(3).rstrip(), 'lines': []}
        rows.append(cur); continue
    if re.match(r'^\[[ABC]\]', line) or re.match(r'^  namespace ', line): cur = None; continue
    if cur is not None and line.startswith('      '): cur['lines'].append(line.strip())
def consts(text):
    return [(int(v), n) for v, n in re.findall(r'returns (-?\d+) \[([^\]]*)\]', text)]
out = []
for r in rows:
    L = r['lines']; get = lambda pre: [x for x in L if x.startswith(pre)]
    feas = (get('feasibility:') or ['feasibility: ?'])[0][13:]
    flip = (get('flip:') or ['flip: ?'])[0][6:]
    flipg = flip.split(':')[0].split()[0]
    r['flip'] = flipg
    if r['p'] == 'A':
        act = (get('actors:') or [''])[0]
        res = (re.search(r'result=(\w+)', act) or [None, '?'])[1]
        ab = consts(flip.split('search returns:')[1]) if 'search returns:' in flip else []
        failc = [c for c in ab if cls(c[0]) == 'fail']
        term = bool(get('handling: terminal'))
        probe = any('probe-create' in x for x in L if x.startswith('handling:') or x.startswith('not a pair:'))
        def _retry(x):
            if ' delay ' not in x: return False
            ex = x.split(' exit ', 1)[1].split(' delay ', 1)[0] if ' exit ' in x else ''
            return any(cls(int(v)) == 'fail' for v in re.findall(r'\((?:ne|eq) local:\w+ (-?\d+)\)', ex))
        retry = [x for x in get('handling: in the loop') if _retry(x)]
        if probe or retry: lab = 'H'
        elif term: lab = 'R'
        elif failc and res == 'discarded': lab = 'J'
        elif failc and res == 'tested': lab = 'T?'
        elif failc: lab = 'E'
        elif ab: lab = '0'
        else: lab = 'E'
        dep = (get('dependency:') or [''])[0]
        ncre = int((re.search(r'(\d+) creator\(s\)', dep) or [0, 0])[1])
        r.update(lab=lab, res=res, fail=bool(failc), order='unordered', pair=ncre, elem=(re.search(r'namespace (\S+)', r['head']) or [0, '?'])[1],
                 key=('known' if 'key known' in r['head'] else 'unresolved' if 'key unresolved' in r['head'] else '?'), term=term,
                 retry=(re.search(r'kind (\S+) trip (\S+ \S+)', retry[0]).group(0) if retry and re.search(r'kind (\S+) trip (\S+ \S+)', retry[0]) else ('loop+delay' if retry else '')))
    elif r['p'] == 'B':
        h = (get('handling: discarded') or [''])[0]
        m = re.search(r'discarded (\d+), used (\d+), tested (\d+)', h)
        d, u, t = (int(m.group(1)), int(m.group(2)), int(m.group(3))) if m else (0, 0, 0)
        cc = [x for x in L if x.startswith('callee-check:')]
        ab = [c for x in cc for c in consts(x)]
        failc = [c for c in ab if cls(c[0]) == 'fail']
        if failc and d > 0: lab = 'J'
        elif failc and t > 0: lab = 'T?'
        elif failc: lab = 'E'
        elif cc: lab = '0'
        else: lab = 'E'
        cl = re.findall(r'reader actors created before it: ([^;.]*); after it: ([^.]*)\.', feas)
        if any(not b.strip().startswith('none') for b, a in cl): order = 'fixed?'
        elif cl and any(not a.strip().startswith('none') for b, a in cl): order = 'after-only?'
        elif cl: order = 'walk-no-reader'
        else: order = 'unordered'
        r.update(lab=lab, res='d%d/u%d/t%d' % (d, u, t), fail=bool(failc), order=order, pair=1, elem=r['head'].split()[0], key='', term=False, retry='',
                 failnames=sorted(set(n for _, n in failc)))
    else:
        early = re.findall(r'returns (-?\d+) \[([^\]]*)\] with the earlier value', flip)
        allc = consts(flip)
        failc = [(int(v), n) for v, n in early if cls(int(v)) == 'fail']
        lab = 'F' if failc else ('0' if allc else 'E')
        order = 'enforced' if feas.startswith('proven-impossible') else ('unordered' if feas.startswith('possible') else 'undecided')
        r.update(lab=lab, res='(not in row)', fail=bool(failc), order=order, pair=1, elem=(re.search(r'field (\S+?);', r['head']) or [0, '?'])[1], key='', term=False, retry='',
                 failnames=sorted(set(n for _, n in failc)))
    r['feas'] = feas[:60]; r['feasfull'] = feas
print('rows', len(rows), dict(collections.Counter(r['p'] for r in rows)))
for p in 'ABC':
    c = collections.Counter(r['lab'] for r in rows if r['p'] == p)
    print(' path', p, 'labels', dict(sorted(c.items())))
    c2 = collections.Counter((r['lab'], r['order']) for r in rows if r['p'] == p)
    print('        label x order', {'%s/%s' % k: v for k, v in sorted(c2.items())})
A = [r for r in rows if r['p'] == 'A']
print(' A: R rows', [r['id'] for r in A if r['lab'] == 'R'])
print(' A: R rows with a creator in another actor', [r['id'] for r in A if r['lab'] == 'R' and r['pair'] > 0])
print(' A: H rows', [(r['id'], r['retry'] or 'probe-create') for r in A if r['lab'] == 'H'])
print(' A: J rows', [r['id'] for r in A if r['lab'] == 'J'], ' with creator:', [r['id'] for r in A if r['lab'] == 'J' and r['pair'] > 0])
print(' A: T? rows', len([r for r in A if r['lab'] == 'T?']), 'with a creator in another actor', [r['id'] for r in A if r['lab'] == 'T?' and r['pair'] > 0])
print(' A: result column', dict(collections.Counter(r['res'] for r in A)), 'key', dict(collections.Counter(r['key'] for r in A)), 'rows with creator(s) in other actors', sum(1 for r in A if r['pair'] > 0))
site = lambda r: r['head'].split()[1]
print(' A: distinct lookup sites', len(set(site(r) for r in A)), '; sites of R rows', len(set(site(r) for r in A if r['lab'] == 'R')), '; of R rows with a creator in another actor', sorted(set(site(r).split('/')[-1] for r in A if r['lab'] == 'R' and r['pair'] > 0)), '; of J rows with a creator', sorted(set(site(r).split('/')[-1] for r in A if r['lab'] == 'J' and r['pair'] > 0)), '; of T? rows with a creator', len(set(site(r) for r in A if r['lab'] == 'T?' and r['pair'] > 0)))
Bq = [r for r in rows if r['p'] == 'B']
print(' B: J rows', [(r['id'], r['elem'].split('.')[-1], r['res'], r['order'], r['flip'], ','.join(r['failnames'])[:60]) for r in Bq if r['lab'] == 'J'])
print(' B: T? rows', len([r for r in Bq if r['lab'] == 'T?']), ' B: E rows (no callee-check line)', len([r for r in Bq if r['lab'] == 'E' and not any(x.startswith('callee-check') for x in r['lines'])]), ' B: flip', dict(collections.Counter(r['flip'] for r in Bq)))
print(' B: fixed? rows', len([r for r in Bq if r['order'] == 'fixed?']), 'of them J', [r['id'] for r in Bq if r['order'] == 'fixed?' and r['lab'] == 'J'], 'T?', [r['id'] for r in Bq if r['order'] == 'fixed?' and r['lab'] == 'T?'])
Cq = [r for r in rows if r['p'] == 'C']
print(' C: F rows', [(r['id'], r['elem'], r['order'], ','.join(r['failnames'])) for r in Cq if r['lab'] == 'F'])
surv = [r for r in rows if (r['lab'] in ('R', 'J') and r['pair'] > 0) or r['lab'] == 'F']
print(' survivors (A: R|J with a creator in another actor; B: J; C: F):', len(surv), dict(collections.Counter(r['p'] for r in surv)))
print('   by order', dict(collections.Counter((r['p'], r['order']) for r in surv)))
print('   distinct (path, shared element)', len(set((r['p'], r['elem']) for r in surv)))
obs = [r for r in rows if r['lab'] == 'E']
print(' UNDECIDED-EVAL rows (label not decidable from the row; S3 would observe them):', len(obs), dict(collections.Counter(r['p'] for r in obs)))
print(' S3 observation set = survivors + UNDECIDED-EVAL:', len(surv) + len(obs))
surv2 = [r for r in surv if r['order'] != 'enforced' and not (r['p'] == 'B' and r['flip'] == 'none')]
print('   survivors minus enforced and minus B rows with flip none:', len(surv2), dict(collections.Counter(r['p'] for r in surv2)), 'elements', len(set((r['p'], r['elem']) for r in surv2)))
if showrows:
    for r in rows: print('ROW', r['id'], r['lab'], r['order'], r['res'], r['flip'], r['elem'], r['key'], 'pair=%s' % r['pair'])
for r in rows:
    if r['id'] in show: print('SHOW', r['id'], 'label', r['lab'], 'order', r['order'], 'result', r['res'], 'flip', r['flip'], 'elem', r['elem'], 'key', r['key'], 'pair', r['pair'], 'feas', r['feas'])
# --- additions: path C structure and the lenient reading of path B rows without a callee check
spn = lambda r: r['head'].split()[1]
print(' C: rows', len(Cq), 'distinct spawn sites', len(set(spn(r) for r in Cq)), 'distinct fields', len(set(r['elem'] for r in Cq)), 'distinct (spawn, field)', len(set((spn(r), r['elem']) for r in Cq)),
      '; feasibility possible', sum(1 for r in Cq if r['order'] == 'unordered'), 'distinct (spawn, field) among them', len(set((spn(r), r['elem']) for r in Cq if r['order'] == 'unordered')), '; flip', dict(collections.Counter(r['flip'] for r in Cq)))
nocc = [r for r in Bq if r['lab'] == 'E' and not any(x.startswith('callee-check') for x in r['lines'])]
print(' lenient reading (a path B row with no callee-check line counts as label 0): UNDECIDED-EVAL', len(obs) - len(nocc), '; S3 observation set', len(surv) + len(obs) - len(nocc))
print(' T? rows (status tested, no terminal chain in the row; the body of the failing branch is not in the row):', sum(1 for r in rows if r['lab'] == 'T?'))

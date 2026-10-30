#!/usr/bin/env python3
# extract_baseline_count.py OUT COUNTDIR -- counts and samples over the `kz-static show` texts written by
# extract_baseline.sh run (OUT/<build>/<variant>/facts.txt).  Reads text only; writes TSV files and a summary.
#
# Three kinds of output, kept apart:
#   (1) counts per (build, variant): section sizes, call flags, argument kinds, loop kinds and rules, writes,
#       writer sets and their status (counts.tsv, args.tsv, loops.tsv, fwrites.tsv, locs_*.tsv)
#   (2) rule queries over the facts that need no case input (proxies for the extraction paths A/B/C of
#       plan/extraction_design_20261030.md): field -> argument flows, comparison calls in loops, callback search
#       loops, writes after a spawn call in the creator (pB_*.tsv, pA_*.tsv, pC_*.tsv)
#   (3) evaluation lookups that name the known cases (eval_*.txt).  The case names are used here to look at the
#       output, never to produce it.
import collections
import os
import re
import sys

OUT, CD = sys.argv[1], sys.argv[2]
os.makedirs(CD, exist_ok=True)

BUILDS = ['c1_pre_tx', 'c1_pre_rx', 'c1_post_tx', 'c1_post_rx', 'r2_core', 'r2_all', 'c5_641']
VARIANTS = ['V0', 'V0r', 'V0p', 'V1', 'V2', 'V3']
G0 = ['OS_TaskCreate', 'CFE_ES_CreateChildTask', 'OS_TaskDelay', 'CFE_ES_LockSharedData', 'CFE_ES_UnlockSharedData',
      'CFE_ES_ExitApp', 'CFE_PSP_Restart', 'CFE_PSP_Panic', 'abort']
CMP_FNS = {'strcmp', 'strncmp', 'memcmp', 'strcasecmp', 'strncasecmp', 'bcmp'}

RE_SEC = re.compile(r'^\[(\w+)\] (\d+)$')
RE_CALL = re.compile(r'^  site (\d+) (\S+):(\d+):(\d+) (\S+) -> (\S+) (direct|indirect|builtin) \{([^}]*)\}(?: loop=(\d+) depth=(\d+))?')
RE_ARG = re.compile(r'^      arg (\d+):(\S+)(?: (.*))?$')
RE_LOOP = re.compile(r'^  loop (\d+) site (\d+) (\S+):(\d+):(\d+) in (\S+) depth=(\d+) (\S+) trip=(\S+) exits=(\d+) calls=(\d+)(.*)$')
RE_RULE = re.compile(r'^      rule (\S+) \{([^}]*)\}')
RE_FW = re.compile(r'^  (\S+) (field|record|address-argument|untyped)(?: (\S+))? at (\S+):(\d+)$')
RE_FN = re.compile(r'^  fn (\d+) (\S+) (\S+) params=(\d+) \{([^}]*)\}')
RE_LOC = re.compile(r'^  (\S+) (writer-set-complete|DISABLED) writers=(\d+) \(direct (\d+), data-fn (\d+), out-pointer (\d+)\) atype: untyped=(\d+) escape=(\d+) cast=(\d+) union=(\d+) unknown-effect=(\d+) init=(\d+)')
RE_WR = re.compile(r'^  (\S+) (\S+) at (\S+):(\d+):(\d+) in (\S+)(?: via \S+ \S+)? value=(.*)$')
RE_TGT = re.compile(r'^  site (\d+) (\S+) in (\S+) (indirect-call|spawn-entry) -> (\S+) via (.*) of (\d+)$')
RE_DER = re.compile(r'^  site (\d+) arg (\d+) of (\S+): (derived|not-derived)')


def base(name):
    return name.split('@', 1)[0]


def parse(path):
    secs = collections.OrderedDict()
    cur = None
    with open(path, errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            m = RE_SEC.match(line)
            if m:
                cur = m.group(1)
                secs[cur] = {'n': int(m.group(2)), 'lines': []}
                continue
            if cur is not None:
                secs[cur]['lines'].append(line)
    F = {'secs': secs}
    # calls
    calls = []
    for line in secs.get('calls', {}).get('lines', []):
        m = RE_CALL.match(line)
        if m:
            calls.append({'site': int(m.group(1)), 'file': m.group(2), 'line': int(m.group(3)), 'caller': base(m.group(5)),
                          'callee': base(m.group(6)), 'kind': m.group(7), 'flags': set(x for x in m.group(8).split(',') if x),
                          'loop': int(m.group(9)) if m.group(9) else None, 'args': []})
            continue
        m = RE_ARG.match(line)
        if m and calls:
            rest = m.group(3) or ''
            role = None
            rm = re.search(r' role=(\w+)$', rest)
            if rm:
                role = rm.group(1)
                rest = rest[:rm.start()]
            calls[-1]['args'].append({'i': int(m.group(1)), 'kind': m.group(2), 'text': rest, 'role': role})
    F['calls'] = calls
    loops = []
    for line in secs.get('loops', {}).get('lines', []):
        m = RE_LOOP.match(line)
        if m:
            loops.append({'loop': int(m.group(1)), 'file': m.group(3), 'line': int(m.group(4)), 'fn': base(m.group(6)),
                          'kind': m.group(8), 'trip': m.group(9), 'exits': int(m.group(10)), 'ncalls': int(m.group(11)),
                          'tail': m.group(12), 'rule': None, 'lflags': '', 'delay': False, 'lock': None})
            continue
        if loops:
            r = RE_RULE.match(line)
            if r:
                loops[-1]['rule'] = r.group(1)
                loops[-1]['lflags'] = r.group(2)
            elif line.startswith('      delay-call'):
                loops[-1]['delay'] = True
            elif line.startswith('      lock '):
                loops[-1]['lock'] = line.split()[1]
    F['loops'] = loops
    fw = []
    for line in secs.get('fwrites', {}).get('lines', []):
        m = RE_FW.match(line)
        if m:
            fw.append({'fn': base(m.group(1)), 'kind': m.group(2), 'field': m.group(3) or '', 'file': m.group(4), 'line': int(m.group(5))})
    F['fwrites'] = fw
    fns = {}
    for line in secs.get('functions', {}).get('lines', []):
        m = RE_FN.match(line)
        if m:
            fns.setdefault(base(m.group(2)), []).append({'loc': m.group(3), 'flags': m.group(5)})
    F['fns'] = fns
    locs = []
    for line in secs.get('locs', {}).get('lines', []):
        m = RE_LOC.match(line)
        if m:
            locs.append({'loc': m.group(1), 'status': m.group(2), 'writers': int(m.group(3)), 'direct': int(m.group(4)),
                         'datafn': int(m.group(5)), 'outptr': int(m.group(6)), 'untyped': int(m.group(7)), 'escape': int(m.group(8)),
                         'cast': int(m.group(9)), 'union': int(m.group(10)), 'unknown': int(m.group(11)), 'init': int(m.group(12))})
    F['locs'] = locs
    wr = []
    for line in secs.get('writers', {}).get('lines', []):
        m = RE_WR.match(line)
        if m:
            wr.append({'loc': m.group(1), 'how': m.group(2), 'file': m.group(3), 'line': int(m.group(4)), 'fn': base(m.group(6)), 'value': m.group(7)})
    F['writers'] = wr
    tg = []
    for line in secs.get('targets', {}).get('lines', []):
        m = RE_TGT.match(line)
        if m:
            tg.append({'site': int(m.group(1)), 'fn': base(m.group(3)), 'kind': m.group(4), 'target': base(m.group(5)), 'via': m.group(6)})
    F['targets'] = tg
    der = collections.Counter()
    for line in secs.get('derives', {}).get('lines', []):
        m = RE_DER.match(line)
        if m:
            der[m.group(4)] += 1
    F['derives'] = der
    return F


def w(name, header, rows):
    with open(os.path.join(CD, name), 'w') as f:
        f.write('\t'.join(header) + '\n')
        for r in rows:
            f.write('\t'.join(str(x) for x in r) + '\n')


def field_of_arg(a):
    if a['kind'] == 'symbolic' and re.match(r'^[A-Za-z_]\w*\.[A-Za-z_]\w*$', a['text'] or ''):
        return a['text']
    if a['kind'] == 'addr-of-field':
        m = re.search(r' of ([A-Za-z_]\w*\.[A-Za-z_]\w*)$', a['text'] or '')
        if m:
            return m.group(1)
    return None


facts = {}
for b in BUILDS:
    for v in VARIANTS:
        p = os.path.join(OUT, b, v, 'facts.txt')
        if os.path.exists(p):
            facts[(b, v)] = parse(p)

summary = []
def say(s=''):
    summary.append(s)
    print(s)

# ---------------------------------------------------------------- (1) counts
SECS = ['functions', 'calls', 'derives', 'loops', 'iters', 'order', 'init', 'consts', 'writers', 'atype', 'locs', 'targets',
        'spawnparams', 'image', 'terminals', 'outcalls', 'fwrites']
rows = []
for (b, v), F in facts.items():
    rows.append([b, v] + [F['secs'].get(s, {}).get('n', 0) for s in SECS])
w('sections.tsv', ['build', 'variant'] + SECS, rows)

FLAGS = ['result-used', 'result-tested', 'in-loop', 'intra-tu', 'in-macro', 'spawn-op', 'delay-op', 'lock-op', 'unlock-op', 'terminal-op', 'data-fn']
AKINDS = ['const', 'string', 'symbolic-field', 'symbolic-global', 'addr-of-field', 'addr-of-object', 'function', 'top']
rows, arows = [], []
for (b, v), F in facts.items():
    c = collections.Counter()
    ak = collections.Counter()
    roles = 0
    for cl in F['calls']:
        c['calls'] += 1
        c['kind:' + cl['kind']] += 1
        for fl in cl['flags']:
            c[fl] += 1
        if 'result-used' not in cl['flags']:
            c['result-unused'] += 1
        if 'result-used' in cl['flags'] and 'result-tested' not in cl['flags']:
            c['result-used-untested'] += 1
        for a in cl['args']:
            k = a['kind']
            if k == 'symbolic':
                k = 'symbolic-field' if field_of_arg(a) else 'symbolic-global'
            ak[k] += 1
            if a['role']:
                roles += 1
    rows.append([b, v, c['calls'], c['kind:direct'], c['kind:indirect'], c['kind:builtin']] + [c[f] for f in FLAGS] + [c['result-unused'], c['result-used-untested']])
    arows.append([b, v, sum(ak.values())] + [ak[k] for k in AKINDS] + [roles])
w('calls.tsv', ['build', 'variant', 'calls', 'direct', 'indirect', 'builtin'] + FLAGS + ['result-unused', 'result-used-untested'], rows)
w('args.tsv', ['build', 'variant', 'args'] + AKINDS + ['args-with-role'], arows)

rows = []
for (b, v), F in facts.items():
    k = collections.Counter(l['kind'] for l in F['loops'])
    r = collections.Counter(l['rule'] for l in F['loops'])
    rows.append([b, v, len(F['loops']), k['table-walk'], k['constant-trip'], k['count-bounded'], k['poll'], k['other'],
                 r['unroll'], r['await'], r['star-closed'], sum(1 for l in F['loops'] if l['delay']), sum(1 for l in F['loops'] if l['lock'])])
w('loops.tsv', ['build', 'variant', 'loops', 'table-walk', 'constant-trip', 'count-bounded', 'poll', 'other', 'unroll', 'await', 'star-closed', 'with-delay-call', 'with-lock-info'], rows)

rows = []
for (b, v), F in facts.items():
    k = collections.Counter(x['kind'] for x in F['fwrites'])
    flds = set(x['field'] for x in F['fwrites'] if x['field'] and not x['field'].startswith('<global>'))
    glob = set(x['field'] for x in F['fwrites'] if x['field'].startswith('<global>'))
    rows.append([b, v, len(F['fwrites']), k['field'], k['record'], k['address-argument'], k['untyped'], len(flds), len(glob), len(set(x['fn'] for x in F['fwrites']))])
w('fwrites.tsv', ['build', 'variant', 'rows', 'field', 'record', 'address-argument', 'untyped', 'distinct-type-fields', 'distinct-globals', 'functions'], rows)

rows = []
for (b, v), F in facts.items():
    st = collections.Counter(l['status'] for l in F['locs'])
    tg = collections.Counter(t['kind'] for t in F['targets'])
    unres = sum(1 for t in F['targets'] if t['target'].startswith('UNRESOLVED'))
    rows.append([b, v, len(F['locs']), st['writer-set-complete'], st['DISABLED'], len(F['writers']),
                 sum(1 for l in F['locs'] if l['writers'] == 0), F['derives']['derived'], F['derives']['not-derived'], tg['indirect-call'], tg['spawn-entry'], unres])
w('closure.tsv', ['build', 'variant', 'locations', 'complete', 'DISABLED', 'writer-rows', 'locations-without-writer', 'derives-derived', 'derives-not', 'indirect-targets', 'spawn-entry-targets', 'unresolved-targets'], rows)

say('# extract_baseline_count.py: counts (sections.tsv, calls.tsv, args.tsv, loops.tsv, fwrites.tsv, closure.tsv)')
for (b, v), F in facts.items():
    s = F['secs']
    say('%-11s %-3s calls %5d loops %3d iters %3d image %3d targets %3d fwrites %5d | derives %3d writers %4d locs %4d atype %5d init %4d terminals %3d outcalls %4d spawnparams %d' % (
        b, v, s['calls']['n'], s['loops']['n'], s['iters']['n'], s['image']['n'], s['targets']['n'], s['fwrites']['n'],
        s['derives']['n'], s['writers']['n'], s['locs']['n'], s['atype']['n'], s['init']['n'], s['terminals']['n'], s['outcalls']['n'], s['spawnparams']['n']))

# what each variant adds over V0: the lines of each section that V0 lacks.  Lines are compared without the row
# numbers (`site N`, `loop N`, `loop=N`, `parent=N`, `fn N`, `iter`-free), which are positions in a sorted table and
# shift when a sort key changes; a call line therefore differs only by its flags, an argument line only by its role.
RE_NUM = [(re.compile(r'\bsite \d+'), 'site #'), (re.compile(r'\bloop \d+'), 'loop #'), (re.compile(r'\bloop=\d+'), 'loop=#'),
          (re.compile(r'\bparent=\d+'), 'parent=#'), (re.compile(r'^  fn \d+'), '  fn #'), (re.compile(r'\bsite -'), 'site #')]
def norm_lines(lines):
    out = []
    for ln in lines:
        for r, s in RE_NUM:
            ln = r.sub(s, ln)
        out.append(ln)
    return collections.Counter(out)

say('')
say('# section lines a variant has and V0 of the same build lacks (V0 -> Vx), row numbers removed')
rows = []
for (b, v), F in facts.items():
    if v == 'V0' or (b, 'V0') not in facts:
        continue
    F0 = facts[(b, 'V0')]
    diffs = []
    for s in SECS:
        a = norm_lines(F0['secs'].get(s, {}).get('lines', []))
        x = norm_lines(F['secs'].get(s, {}).get('lines', []))
        plus = sum((x - a).values())
        minus = sum((a - x).values())
        if plus or minus:
            diffs.append('%s +%d/-%d' % (s, plus, minus))
            rows.append([b, v, s, plus, minus])
    say('%-11s %-3s %s' % (b, v, '; '.join(diffs) if diffs else 'none'))
w('variant_vs_V0.tsv', ['build', 'variant', 'section', 'lines-added', 'lines-removed'], rows)

# G0 check: is each generic name defined (with a body) in the closure?
rows = []
for (b, v), F in facts.items():
    if v != 'V1':
        continue
    for n in G0 + ['pthread_create']:
        ent = F['fns'].get(n, [])
        defined = [e for e in ent if 'defined' in e['flags']]
        rows.append([b, n, 'defined' if defined else ('declared' if ent else 'absent'), (defined or ent or [{'loc': '-'}])[0]['loc'],
                     sum(1 for c in F['calls'] if c['callee'] == n)])
w('g0_check.tsv', ['build', 'name', 'in-closure', 'location', 'call-sites'], rows)

# ---------------------------------------------------------------- (2) rule queries (no case input)
say('')
say('# path B proxy: a field read passed as a call argument without a copy (arg kind symbolic T.f), V0')
rowsB = []
for (b, v), F in facts.items():
    if v != 'V0':
        continue
    flows = collections.Counter()
    fields = set()
    for c in F['calls']:
        for a in c['args']:
            if a['kind'] == 'symbolic' and field_of_arg(a):
                flows[(c['callee'], a['i'], a['text'])] += 1
                fields.add(a['text'])
    wfield = collections.defaultdict(set)
    for x in F['fwrites']:
        if x['field']:
            wfield[x['field']].add(x['kind'])
        if x['kind'] == 'record' and x['field'].endswith('.*'):
            wfield[x['field'][:-2] + '.*'].add('record')
    n_written = sum(1 for f in fields if wfield.get(f) or wfield.get(f.split('.')[0] + '.*'))
    n_addr = sum(1 for f in fields if 'address-argument' in wfield.get(f, set()))
    say('%-11s flows(callee,arg,field) %5d over %5d call args; distinct fields %4d; with a write in some body %4d (of them via an address argument %3d); no write found %4d' % (
        b, len(flows), sum(flows.values()), len(fields), n_written, n_addr, len(fields) - n_written))
    for (callee, i, fld), n in sorted(flows.items(), key=lambda kv: -kv[1]):
        rowsB.append([b, callee, i, fld, n, ','.join(sorted(wfield.get(fld, set()))) or '-'])
w('pB_field_to_arg.tsv', ['build', 'callee', 'arg', 'field', 'sites', 'write-kinds-of-field'], rowsB)

say('')
say('# path A proxies, V0: (a) a comparison call (strcmp family) inside a loop; (b) a loop whose body makes an indirect call with resolved targets')
rowsA = []
for (b, v), F in facts.items():
    if v != 'V0':
        continue
    lfn = {l['loop']: l for l in F['loops']}
    cmp_loops = collections.defaultdict(list)
    ind_loops = collections.defaultdict(list)
    tg_by_site = collections.defaultdict(list)
    for t in F['targets']:
        if t['kind'] == 'indirect-call':
            tg_by_site[t['site']].append(t['target'])
    for c in F['calls']:
        if c['loop'] is None:
            continue
        if c['callee'] in CMP_FNS:
            cmp_loops[c['loop']].append(c)
        if c['kind'] == 'indirect' and tg_by_site.get(c['site']):
            ind_loops[c['loop']].append((c, tg_by_site[c['site']]))
    for li, cs in sorted(cmp_loops.items()):
        l = lfn.get(li, {})
        kinds = ['/'.join(field_of_arg(a) or a['kind'] for a in c['args'][:2]) for c in cs]
        rowsA.append([b, 'cmp-in-loop', l.get('fn', '?'), '%s:%s' % (l.get('file', '?'), l.get('line', '?')), l.get('kind', '?'), ';'.join(sorted(set(c['callee'] + '(' + k + ')' for c, k in zip(cs, kinds))))])
    for li, cs in sorted(ind_loops.items()):
        l = lfn.get(li, {})
        rowsA.append([b, 'callback-in-loop', l.get('fn', '?'), '%s:%s' % (l.get('file', '?'), l.get('line', '?')), l.get('kind', '?'), ';'.join(sorted(set(t for _, ts in cs for t in ts)))])
    nf_cmp = sum(1 for li, cs in cmp_loops.items() if any(field_of_arg(a) for c in cs for a in c['args'][:2]))
    say('%-11s loops with a strcmp-family call %3d (one argument a field read %3d); loops with a resolved indirect call %3d' % (b, len(cmp_loops), nf_cmp, len(ind_loops)))
w('pA_search_loops.tsv', ['build', 'pattern', 'function', 'loop', 'loop-kind', 'calls-or-targets'], rowsA)

say('')
say('# path C proxy, V1: fields the creator function writes first after the spawn call line (FUNC_WRITES keeps the first write per field only)')
rowsC = []
for (b, v), F in facts.items():
    if v != 'V1':
        continue
    n_sp = 0
    for c in F['calls']:
        if 'spawn-op' not in c['flags']:
            continue
        n_sp += 1
        after = sorted(set(x['field'] for x in F['fwrites'] if x['fn'] == c['caller'] and x['file'] == c['file'] and x['line'] > c['line'] and x['field'] and x['kind'] != 'untyped'))
        ents = sorted(set(t['target'] for t in F['targets'] if t['site'] == c['site'] and t['kind'] == 'spawn-entry'))
        rowsC.append([b, c['caller'], '%s:%d' % (c['file'], c['line']), c['callee'], ','.join(ents) or '-', len(after), ','.join(after)])
    say('%-11s spawn sites %2d; with writes after the call in the creator %2d' % (b, n_sp, sum(1 for r in rowsC if r[0] == b and r[5] > 0)))
w('pC_post_spawn_writes.tsv', ['build', 'creator', 'spawn-site', 'spawn-op', 'entries', 'n-fields-after', 'fields-first-written-after'], rowsC)

# V3: writer sets per rule-derived field
say('')
say('# V3 (rule-derived targets R-T1): writer sets per field')
rows = []
for (b, v), F in facts.items():
    if v != 'V3':
        continue
    st = collections.Counter(l['status'] for l in F['locs'])
    nw = collections.Counter('0' if l['writers'] == 0 else ('1' if l['writers'] == 1 else ('2-5' if l['writers'] <= 5 else '>5')) for l in F['locs'])
    why = collections.Counter()
    for l in F['locs']:
        if l['status'] == 'DISABLED':
            for k in ('untyped', 'escape', 'cast', 'union', 'unknown'):
                if l[k]:
                    why[k] += 1
    say('%-11s fields %4d: complete %4d DISABLED %4d; writers 0:%d 1:%d 2-5:%d >5:%d; DISABLED with untyped %d escape %d cast %d union %d unknown-effect %d' % (
        b, len(F['locs']), st['writer-set-complete'], st['DISABLED'], nw['0'], nw['1'], nw['2-5'], nw['>5'], why['untyped'], why['escape'], why['cast'], why['union'], why['unknown']))
    for l in F['locs']:
        rows.append([b, l['loc'], l['status'], l['writers'], l['direct'], l['datafn'], l['outptr'], l['init'], l['untyped'], l['escape'], l['cast'], l['union'], l['unknown']])
w('locs_V3.tsv', ['build', 'field', 'status', 'writers', 'direct', 'data-fn', 'out-pointer', 'init', 'untyped', 'escape', 'cast', 'union', 'unknown-effect'], rows)

# the status of the case-profile fields when the target set is rule-derived (V2 vs V3)
say('')
say('# fields of the case profiles: status with the case targets (V2) and with the rule-derived targets (V3)')
for b in ('c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641'):
    if (b, 'V2') not in facts or (b, 'V3') not in facts:
        continue
    l3 = {l['loc']: l for l in facts[(b, 'V3')]['locs']}
    for l in facts[(b, 'V2')]['locs']:
        x = l3.get(l['loc'])
        say('  %-11s %-40s V2 %-20s writers %2d | V3 %s' % (b, l['loc'], l['status'], l['writers'],
            '%-20s writers %2d (untyped %d escape %d cast %d unknown %d)' % (x['status'], x['writers'], x['untyped'], x['escape'], x['cast'], x['unknown']) if x else 'not a target (not passed to a call in V0)'))

# ---------------------------------------------------------------- (2b) static actors and rule-generated observation points
# Actor roots come from the build and the generic vocabulary only: `main`, the resolved entries of spawn calls (V1 and
# the pthread_create seed V0p), the task-main of each unrolled table-walk iteration (order view), and the entry symbol
# of each startup-script line (basis startup.entry[k], field 2).  The call graph is the direct calls into bodies plus
# the resolved indirect targets; a spawn is not a call edge.  Module of a function = module of its translation unit
# (basis unit table: `module X`, `member of libX.a`/`X.o`, `named on the link command` -> exe).
RE_TUS = re.compile(r'TUS\[\d+\].*?source="([^"]*)".*?member="([^"]*)"')
RE_ENTRY = re.compile(r'^startup\.entry\[(\d+)\] = (app|library) ([^,]+), *([A-Za-z_]\w*),')
RE_ORD_HDR = re.compile(r'^  loop \d+ .*\(task-main\)')
RE_ORD_IT = re.compile(r'^    iter \d+ \S+ -> (\S+)')

def module_of_member(m):
    mm = re.match(r'module (\S+)', m)
    if mm:
        return os.path.basename(mm.group(1)).replace('.so', '')
    mm = re.match(r'member of (?:lib)?([^ ]+?)(?:\.a|\.o)\b', m)
    if mm:
        return mm.group(1)
    return 'exe'

def load_units(b):
    units = {}
    p = os.path.join(OUT, b, 'V0', 'basis.dump')
    if os.path.exists(p):
        with open(p, errors='replace') as f:
            for line in f:
                m = RE_TUS.search(line)
                if m:
                    units[m.group(1)] = module_of_member(m.group(2))
    entries = []
    with open(os.path.join(OUT, b, 'V0', 'basis.txt'), errors='replace') as f:
        for line in f:
            m = RE_ENTRY.match(line.strip())
            if m:
                entries.append((m.group(2), m.group(3).strip(), m.group(4)))
    return units, entries

def file_module(units, fpath, cache={}):
    key = (id(units), fpath)
    if key in cache:
        return cache[key]
    mod = units.get(fpath)
    if mod is None:
        for src, m in units.items():
            if src.endswith('/' + fpath) or fpath.endswith('/' + src) or src == fpath:
                mod = m
                break
    if mod is None:
        mod = '(header)' if fpath.endswith('.h') else '(no unit)'
    cache[key] = mod
    return mod

def static_actors(b, F, Fsp):
    units, entries = load_units(b)
    fnfile = {}
    defined = set()
    for n, ents in F['fns'].items():
        for e in ents:
            if 'defined' in e['flags']:
                defined.add(n)
                fnfile[n] = e['loc'].rsplit(':', 1)[0]
    cg = collections.defaultdict(set)
    for c in F['calls']:
        if c['kind'] == 'direct' and c['callee'] in defined:
            cg[c['caller']].add(c['callee'])
    for t in F['targets']:
        if t['kind'] == 'indirect-call' and not t['target'].startswith('UNRESOLVED'):
            cg[t['fn']].add(t['target'])
    roots = {}
    if 'main' in defined:
        roots['main'] = 'process'
    for G in Fsp:
        for t in G['targets']:
            if t['kind'] == 'spawn-entry' and not t['target'].startswith('UNRESOLVED') and t['target'] in defined:
                roots.setdefault(t['target'], 'spawn-entry')
        hdr = False
        for line in G['secs'].get('order', {}).get('lines', []):
            if line.startswith('  loop '):
                hdr = bool(RE_ORD_HDR.match(line))
                continue
            m = RE_ORD_IT.match(line)
            if hdr and m and m.group(1) in defined:
                roots.setdefault(m.group(1), 'table-walk task-main')
    for kind, fl, sym in entries:
        if sym in defined:
            roots.setdefault(sym, 'startup-script ' + kind)
    reach = collections.defaultdict(set)
    for r in roots:
        seen, stack = {r}, [r]
        while stack:
            x = stack.pop()
            for y in cg.get(x, ()):
                if y not in seen:
                    seen.add(y)
                    stack.append(y)
        for x in seen:
            reach[x].add(r)
    mod = {n: file_module(units, fnfile[n]) for n in defined}
    unresolved_entries = [e for e in entries if e[2] not in defined]
    return {'cg': cg, 'roots': roots, 'reach': reach, 'mod': mod, 'defined': defined, 'entries': entries, 'unresolved_entries': unresolved_entries}

def reach_within(cg, start, targets, depth):
    frontier, seen = {start}, {start}
    for _ in range(depth):
        nxt = set()
        for x in frontier:
            for y in cg.get(x, ()):
                if y in targets:
                    return True
                if y not in seen:
                    seen.add(y)
                    nxt.add(y)
        frontier = nxt
    return start in targets

MANIFESTS = {'c1_pre_tx': 'c1-bp-b051eb0/instr/sites-cf281a941.kzs', 'c1_post_tx': 'c1-bp-b051eb0/instr/sites-cf833fdbb.kzs',
             'r2_core': 'cfs-v7.0.1/instr/sites.kzs', 'c5_641': 'c5-cfe-6.4.x/instr/sites-641.draft.kzs'}
PROFILES = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'kairos', 'profiles')

say('')
say('# static actors (roots) and rule-generated observation points (prototype over the V1/V0p facts; no case input)')
op_rows, cov_rows, act_rows = [], [], []
for b in BUILDS:
    if (b, 'V1') not in facts:
        continue
    F = facts[(b, 'V1')]
    Fsp = [F] + ([facts[(b, 'V0p')]] if (b, 'V0p') in facts else [])
    A = static_actors(b, F, Fsp)
    reach, cg, mod = A['reach'], A['cg'], A['mod']
    rk = collections.Counter(A['roots'].values())
    n_reached = sum(1 for n in A['defined'] if reach.get(n))
    n_shared = sum(1 for n in A['defined'] if len(reach.get(n, ())) >= 2)
    say('%-11s roots %3d (%s); functions with a body %4d, reached %4d, reached from >=2 roots %4d; startup entries %d, entry symbol without a body in the closure %d' % (
        b, len(A['roots']), ', '.join('%s %d' % kv for kv in sorted(rk.items())), len(A['defined']), n_reached, n_shared, len(A['entries']), len(A['unresolved_entries'])))
    for r, k in sorted(A['roots'].items()):
        act_rows.append([b, r, k, mod.get(r, '?'), sum(1 for n in A['defined'] if r in reach.get(n, ()))])
    ops = collections.OrderedDict()  # (rule, function, site-or-entry) -> note
    # OP-E / OP-S: actor entries and spawn calls
    for r, k in A['roots'].items():
        ops[('OP-E entry/return of an actor root', r, 'entry')] = k
    for G in Fsp:
        for c in G['calls']:
            if 'spawn-op' in c['flags']:
                ops[('OP-S spawn call before/after', c['caller'], '%s:%d %s' % (c['file'], c['line'], c['callee']))] = 'spawn'
    # OP-L: lock and unlock calls (generic vocabulary)
    for c in F['calls']:
        if 'lock-op' in c['flags'] or 'unlock-op' in c['flags']:
            ops[('OP-L lock/unlock call', c['caller'], '%s:%d %s' % (c['file'], c['line'], c['callee']))] = 'lock'
    # OP-T: terminal calls (generic vocabulary)
    for c in F['calls']:
        if 'terminal-op' in c['flags']:
            ops[('OP-T terminal call', c['caller'], '%s:%d %s' % (c['file'], c['line'], c['callee']))] = 'terminal'
    # OP-A: search loops with a field comparison (strcmp family in the loop, or in a resolved callback of the loop),
    # the functions of their module within 2 call levels that reach them or write the compared field, called from another module
    tg_by_site = collections.defaultdict(list)
    for t in F['targets']:
        if t['kind'] == 'indirect-call':
            tg_by_site[t['site']].append(t['target'])
    cmp_fields_of_fn = collections.defaultdict(set)
    for c in F['calls']:
        if c['callee'] in CMP_FNS:
            for a in c['args'][:2]:
                fl = field_of_arg(a)
                if fl:
                    cmp_fields_of_fn[c['caller']].add(fl)
    search_fns, key_fields = set(), set()
    for c in F['calls']:
        if c['loop'] is None:
            continue
        if c['callee'] in CMP_FNS:
            fl = [field_of_arg(a) for a in c['args'][:2] if field_of_arg(a)]
            if fl:
                search_fns.add(c['caller'])
                key_fields.update(fl)
        if c['kind'] == 'indirect':
            for t in tg_by_site.get(c['site'], []):
                if cmp_fields_of_fn.get(t):
                    search_fns.add(c['caller'])
                    key_fields.update(cmp_fields_of_fn[t])
    key_writers = set(x['fn'] for x in F['fwrites'] if x['field'] in key_fields or (x['kind'] == 'record' and x['field'][:-2] in set(k.split('.')[0] for k in key_fields)))
    # the API side: a function of the same module as a search function or key writer that reaches one within two call
    # levels (crude: the parameter that reaches the comparison is not traced; that is the new analysis of path A)
    a_mods = set(mod.get(n) for n in search_fns | key_writers)
    api = set()
    for n in A['defined']:
        if mod.get(n) in a_mods and reach_within(cg, n, search_fns | key_writers, 2):
            api.add(n)
    # the same with four call levels (OP-A4, only what depth 2 does not give): the depth is a parameter of the proxy,
    # and the counts show how much the result depends on it
    api4 = set(n for n in A['defined'] if n not in api and mod.get(n) in a_mods and reach_within(cg, n, search_fns | key_writers, 4))
    for c in F['calls']:
        if mod.get(c['caller']) == mod.get(c['callee']) or mod.get(c['caller']) == '(header)':
            continue
        if c['callee'] in api:
            ops[('OP-A keyed-table API call from another module', c['caller'], '%s:%d %s' % (c['file'], c['line'], c['callee']))] = 'A'
        elif c['callee'] in api4:
            ops[('OP-A4 keyed-table API call from another module (3-4 call levels)', c['caller'], '%s:%d %s' % (c['file'], c['line'], c['callee']))] = 'A4'
    for n in search_fns:
        ops[('OP-A search function entry/return', n, 'entry')] = 'A'
    for n in key_writers:
        ops[('OP-A key-field writer entry/return', n, 'entry')] = 'A'
    # OP-B: a field passed as a call argument with a body write of that field in a function another root reaches
    wfn = collections.defaultdict(set)
    for x in F['fwrites']:
        if x['field'] and x['kind'] in ('field', 'address-argument'):
            wfn[x['field']].add(x['fn'])
    n_b_fields = set()
    for c in F['calls']:
        for a in c['args']:
            fl = field_of_arg(a) if a['kind'] == 'symbolic' else None
            if not fl or fl not in wfn:
                continue
            rr = reach.get(c['caller'], set())
            cross = any(r1 != r2 for r1 in rr for wf in wfn[fl] for r2 in reach.get(wf, set()))
            if cross:
                ops[('OP-B field->argument call (value at the call)', c['caller'], '%s:%d %s arg %d %s' % (c['file'], c['line'], c['callee'], a['i'], fl))] = 'B'
                n_b_fields.add(fl)
    for fl in n_b_fields:
        for wf in wfn[fl]:
            ops[('OP-B writer of a field read at such a call', wf, 'write %s' % fl)] = 'B'
    # OP-C: writes by the creator after its spawn call (first write per field)
    for G in Fsp[:1]:
        for c in G['calls']:
            if 'spawn-op' not in c['flags']:
                continue
            for x in G['fwrites']:
                if x['fn'] == c['caller'] and x['file'] == c['file'] and x['line'] > c['line'] and x['field'] and x['kind'] != 'untyped':
                    ops[('OP-C creator write after the spawn call', c['caller'], 'write %s at %d' % (x['field'], x['line']))] = 'C'
    per_rule = collections.Counter(k[0] for k in ops)
    fns_rule = set(k[1] for k in ops)
    say('%-11s observation points by rule: %s; functions involved %d; search functions %d, key fields %d, keyed-table API functions %d' % (
        b, '; '.join('%s %d' % (r.split(' ')[0], n) for r, n in sorted(collections.Counter(k[0].split(' ')[0] for k in ops).items())), len(fns_rule), len(search_fns), len(key_fields), len(api)) + ' (+%d at 3-4 levels)' % len(api4))
    for k, note in ops.items():
        op_rows.append([b, k[0], k[1], mod.get(k[1], '?'), k[2]])
    # comparison with the hand-chosen manifest
    mf = MANIFESTS.get(b)
    if mf and os.path.exists(os.path.join(PROFILES, mf)):
        man = []
        with open(os.path.join(PROFILES, mf)) as f:
            for line in f:
                m = re.match(r'^site (\d+) label=(\S+) .*?func=(\S+)', line)
                if m:
                    man.append((m.group(2), m.group(3)))
        covered = 0
        for lab, fn in man:
            rules = sorted(set(k[0].split(' ')[0] for k in ops if k[1] == fn))
            called_rules = sorted(set(k[0].split(' ')[0] for k in ops if k[2].endswith(' ' + fn) or (' ' + fn + ' arg') in k[2]))
            st = 'in-rule-set' if rules else ('call-site-in-rule-set' if called_rules else 'not-generated')
            covered += st != 'not-generated'
            cov_rows.append([b, mf, lab, fn, st, ','.join(rules) or '-', ','.join(called_rules) or '-', 'reached by ' + ','.join(sorted(reach.get(fn, set()))[:6]) if reach.get(fn) else 'not reached from a root'])
        cov_no4 = sum(1 for r in cov_rows if r[0] == b and r[4] != 'not-generated' and (set(r[5].split(',')) | set(r[6].split(','))) - {'OP-A4', '-'})
        say('%-11s hand-chosen manifest %s: %d sites, %d generated by a rule (the function or a call to it; %d without OP-A4), %d not' % (b, mf, len(man), covered, cov_no4, len(man) - covered))
w('obs_points_rule.tsv', ['build', 'rule', 'function', 'module', 'point'], op_rows)
w('obs_manifest_coverage.tsv', ['build', 'manifest', 'label', 'function', 'status', 'rules-on-the-function', 'rules-on-calls-to-it', 'static-actors'], cov_rows)
w('static_roots.tsv', ['build', 'root', 'kind', 'module', 'functions-reached'], act_rows)

# ---------------------------------------------------------------- (3) evaluation lookups (case names used to look only)
def dump_calls(F, pred, out, limit=60):
    n = 0
    for c in F['calls']:
        if pred(c):
            n += 1
            if n <= limit:
                out.write('  %s:%d %s -> %s {%s}%s\n' % (c['file'], c['line'], c['caller'], c['callee'], ','.join(sorted(c['flags'])), (' loop=%d' % c['loop']) if c['loop'] is not None else ''))
                for a in c['args']:
                    out.write('      arg %d:%s %s\n' % (a['i'], a['kind'], a['text']))
    out.write('  (%d call sites)\n' % n)

def dump_fw(F, pred, out):
    n = 0
    for x in F['fwrites']:
        if pred(x):
            n += 1
            out.write('  %s %s %s at %s:%d\n' % (x['fn'], x['kind'], x['field'], x['file'], x['line']))
    out.write('  (%d rows)\n' % n)

with open(os.path.join(CD, 'eval_C1.txt'), 'w') as o:
    for b in ('c1_pre_tx', 'c1_post_tx'):
        F = facts[(b, 'V0')]
        o.write('## %s V0: calls of the count-semaphore lookup and create\n' % b)
        dump_calls(F, lambda c: c['callee'] in ('OS_CountSemGetIdByName', 'OS_CountSemCreate'), o)
        o.write('## %s V0: loops in CF_CFDP_InitEngine and initialize_throttling\n' % b)
        for l in F['loops']:
            if l['fn'] in ('CF_CFDP_InitEngine', 'initialize_throttling'):
                o.write('  loop %d %s:%d %s trip=%s exits=%d rule=%s delay=%s\n' % (l['loop'], l['file'], l['line'], l['kind'], l['trip'], l['exits'], l['rule'], l['delay']))
        F1 = facts[(b, 'V1')]
        o.write('## %s V1: the same loops with the generic delay operation\n' % b)
        for l in F1['loops']:
            if l['fn'] in ('CF_CFDP_InitEngine', 'initialize_throttling'):
                o.write('  loop %d %s:%d %s trip=%s exits=%d rule=%s delay=%s\n' % (l['loop'], l['file'], l['line'], l['kind'], l['trip'], l['exits'], l['rule'], l['delay']))
        o.write('## %s V1: terminal-op calls in CF_AppMain and the CF init functions\n' % b)
        dump_calls(F1, lambda c: 'terminal-op' in c['flags'] and c['caller'].startswith('CF_'), o)
        o.write('## %s V0: name-table lookup machinery (search loops and callbacks of the OSAL idmap)\n' % b)
        for l in F['loops']:
            if l['fn'] in ('OS_ObjectIdFindNextMatch', 'OS_ObjectIdFindNextFree', 'OS_ObjectIdSearch'):
                o.write('  loop %d %s:%d in %s %s trip=%s exits=%d rule=%s\n' % (l['loop'], l['file'], l['line'], l['fn'], l['kind'], l['trip'], l['exits'], l['rule']))
        for t in F['targets']:
            if t['fn'] in ('OS_ObjectIdFindNextMatch',):
                o.write('  indirect call in %s -> %s via %s\n' % (t['fn'], t['target'], t['via']))
        o.write('## %s V0: writes of the OSAL common record and the BP throttle record\n' % b)
        dump_fw(F, lambda x: x['field'].startswith('OS_common_record_t.') or x['field'].startswith('BP_Throttle_t.') or x['field'].startswith('CF_Channel_t.sem'), o)
        if (b, 'V3') in facts:
            o.write('## %s V3: writer sets (rule-derived targets) of the key and in-use fields\n' % b)
            for l in facts[(b, 'V3')]['locs']:
                if l['loc'] in ('OS_common_record_t.name_entry', 'OS_common_record_t.active_id', 'BP_Throttle_t.Name', 'BP_Throttle_t.Sem', 'CF_Channel_t.sem_id', 'CF_ChannelConfig_t.sem_name'):
                    o.write('  %s %s writers=%d untyped=%d escape=%d cast=%d unknown=%d\n' % (l['loc'], l['status'], l['writers'], l['untyped'], l['escape'], l['cast'], l['unknown']))

with open(os.path.join(CD, 'eval_R2.txt'), 'w') as o:
    F = facts[('r2_core', 'V0')]
    flows = collections.Counter((c['file'].split('/')[-1], c['callee']) for c in F['calls'] for a in c['args'] if a['kind'] == 'symbolic' and a['text'] == 'CFE_SB_Global_t.AppId')
    o.write('## r2_core V0: call arguments read directly from CFE_SB_Global_t.AppId, by file and callee\n')
    for (fl, ce), n in sorted(flows.items()):
        o.write('  %s %s %d\n' % (fl, ce, n))
    o.write('  total %d\n' % sum(flows.values()))
    o.write('## r2_core V0: arguments of CFE_EVS_SendEventWithAppID position 2, by kind\n')
    k = collections.Counter((a['kind'], a['text'] if a['kind'] == 'symbolic' else '') for c in F['calls'] if c['callee'] == 'CFE_EVS_SendEventWithAppID' for a in c['args'] if a['i'] == 2)
    for (kk, t), n in sorted(k.items()):
        o.write('  %s %s %d\n' % (kk, t, n))
    o.write('## r2_core V0: are the results of those calls used?\n')
    u = collections.Counter('result-used' in c['flags'] for c in F['calls'] if c['callee'] == 'CFE_EVS_SendEventWithAppID' and any(a['i'] == 2 and a['text'] == 'CFE_SB_Global_t.AppId' for a in c['args']))
    o.write('  used %d, discarded %d\n' % (u[True], u[False]))
    o.write('## r2_core V0: body writes of CFE_SB_Global_t fields and of the whole record\n')
    dump_fw(F, lambda x: x['field'].startswith('CFE_SB_Global_t.') or x['field'] == '<global>.CFE_SB_Global', o)
    o.write('## r2_core V0: the callee check: calls inside CFE_EVS_SendEventWithAppID and EVS_GetAppDataByID\n')
    dump_calls(F, lambda c: c['caller'] in ('CFE_EVS_SendEventWithAppID', 'EVS_GetAppDataByID', 'EVS_AppDataIsMatch'), o)
    for v in ('V2', 'V3'):
        o.write('## r2_core %s: writers of CFE_SB_Global_t.AppId\n' % v)
        for x in facts[('r2_core', v)]['writers']:
            if x['loc'] == 'CFE_SB_Global_t.AppId':
                o.write('  %s %s:%d in %s value=%s\n' % (x['how'], x['file'], x['line'], x['fn'], x['value'][:200]))
        for l in facts[('r2_core', v)]['locs']:
            if l['loc'] in ('CFE_SB_Global_t.AppId', 'EVS_AppData_t.AppID'):
                o.write('  loc %s %s writers=%d untyped=%d escape=%d\n' % (l['loc'], l['status'], l['writers'], l['untyped'], l['escape']))
    o.write('## r2_core V1: spawn sites and the core task order (order section)\n')
    F1 = facts[('r2_core', 'V1')]
    dump_calls(F1, lambda c: 'spawn-op' in c['flags'], o)
    for line in F1['secs']['order']['lines']:
        o.write(line + '\n')
    o.write('## r2_core V0p: what the pthread_create seed alone derives\n')
    for line in facts[('r2_core', 'V0p')]['secs']['spawnparams']['lines']:
        o.write(line + '\n')
    for t in facts[('r2_core', 'V0p')]['targets']:
        if t['kind'] == 'spawn-entry':
            o.write('  spawn-entry in %s -> %s via %s\n' % (t['fn'], t['target'], t['via']))

with open(os.path.join(CD, 'eval_C5.txt'), 'w') as o:
    F = facts[('c5_641', 'V0')]
    F1 = facts[('c5_641', 'V1')]
    o.write('## c5_641 V1: spawn sites, entries\n')
    dump_calls(F1, lambda c: 'spawn-op' in c['flags'], o)
    for t in F1['targets']:
        if t['kind'] == 'spawn-entry':
            o.write('  spawn-entry in %s -> %s via %s\n' % (t['fn'], t['target'], t['via']))
    o.write('## c5_641 V0p: the pthread_create seed alone\n')
    for line in facts[('c5_641', 'V0p')]['secs']['spawnparams']['lines']:
        o.write(line + '\n')
    o.write('## c5_641 V1: calls in CFE_ES_AppCreate in source order (lock/unlock, spawn, registration)\n')
    dump_calls(F1, lambda c: c['caller'] == 'CFE_ES_AppCreate' and c['callee'] in ('CFE_ES_LockSharedData', 'CFE_ES_UnlockSharedData', 'OS_TaskCreate', 'OS_SymbolLookup', 'OS_ModuleLoad'), o)
    o.write('## c5_641 V0: body writes in CFE_ES_AppCreate (first write per field)\n')
    dump_fw(F, lambda x: x['fn'] == 'CFE_ES_AppCreate', o)
    o.write('## c5_641 V0: the child-side lookup: calls in CFE_ES_GetAppID, OS_TaskGetId, OS_TaskRegister, CFE_ES_RegisterApp\n')
    dump_calls(F, lambda c: c['caller'] in ('CFE_ES_GetAppID', 'OS_TaskGetId', 'OS_TaskRegister', 'CFE_ES_RegisterApp'), o)
    o.write('## c5_641 V0: loops in OS_TaskRegister, OS_TaskCreate, CFE_ES_AppCreate\n')
    for l in F['loops']:
        if l['fn'] in ('OS_TaskRegister', 'OS_TaskCreate', 'CFE_ES_AppCreate'):
            o.write('  loop %d %s:%d in %s %s trip=%s exits=%d rule=%s\n' % (l['loop'], l['file'], l['line'], l['fn'], l['kind'], l['trip'], l['exits'], l['rule']))
    o.write('## c5_641 V0: writes of OS_task_record_t fields (the OSAL task table) and of CFE_ES_TaskRecord_t\n')
    dump_fw(F, lambda x: x['field'].startswith('OS_task_record_t.') or x['field'].startswith('CFE_ES_TaskRecord_t.'), o)
    for v in ('V2', 'V3'):
        o.write('## c5_641 %s: writer sets of the task-record fields\n' % v)
        for l in facts[('c5_641', v)]['locs']:
            if l['loc'].startswith('CFE_ES_TaskRecord_t.') or l['loc'].startswith('OS_task_record_t.') or l['loc'].startswith('CFE_ES_AppRecord_t.'):
                o.write('  %s %s writers=%d untyped=%d escape=%d cast=%d unknown=%d\n' % (l['loc'], l['status'], l['writers'], l['untyped'], l['escape'], l['cast'], l['unknown']))
        for x in facts[('c5_641', v)]['writers']:
            if x['loc'] in ('CFE_ES_TaskRecord_t.RecordUsed', 'CFE_ES_TaskRecord_t.AppId', 'OS_task_record_t.id', 'OS_task_record_t.free'):
                o.write('  writer %s %s %s:%d in %s value=%s\n' % (x['loc'], x['how'], x['file'], x['line'], x['fn'], x['value'][:160]))

with open(os.path.join(CD, 'summary.txt'), 'w') as f:
    f.write('\n'.join(summary) + '\n')

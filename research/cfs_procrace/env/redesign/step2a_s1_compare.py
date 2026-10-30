#!/usr/bin/env python3
# step2a_s1_compare.py OUT EB CD -- the line comparison of step2a_s1.sh (plan/extraction_design_20261030.md §5 R9): the
# profile-free run NP of each build against V1 (the G0 names on the command line) and VP (the P0 seeds on the command line)
# of extract_baseline.sh (c5_642: the V1 and VP that step2a_s1.sh ran), in the facts that depend on the operation
# vocabulary: loop classes, the task-main order, spawn facts, lock flags, terminal facts, and the call flags of the
# operations.  Row numbers (site, loop) are left out of every comparison; a fact is keyed by its source position.
#
# The G0 names below are used to read the outputs only (which V1 item is about which name); they are no input of the
# profile-free run.  Outputs in CD: summary.txt, items.tsv (one row per V1 or NP item and category: status), chains.txt
# (the derived operations of each build with their chains), g0_names.tsv (what NP derived for each G0 name).
import collections
import os
import re
import sys

OUT, EB, CD = sys.argv[1], sys.argv[2], sys.argv[3]
BUILDS = ['c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641', 'c5_642']
G0 = {'spawn': ['OS_TaskCreate', 'CFE_ES_CreateChildTask'], 'delay': ['OS_TaskDelay'], 'lock': ['CFE_ES_LockSharedData'],
      'unlock': ['CFE_ES_UnlockSharedData'], 'terminal': ['CFE_ES_ExitApp', 'CFE_PSP_Restart', 'CFE_PSP_Panic', 'abort']}
OPFLAGS = ['delay-op', 'terminal-op', 'spawn-op', 'lock-op', 'unlock-op']

RE_SEC = re.compile(r'^\[(\w+)\] (\d+)$')
RE_CALL = re.compile(r'^  site (\d+) (\S+):(\d+):(\d+) (\S+) -> (\S+) (\S+) \{([^}]*)\}')
RE_LOOP = re.compile(r'^  loop (\d+) site (\d+) (\S+) in (\S+) depth=(\d+) (\S+) trip=(\S+) exits=(\d+) calls=(\d+)(.*)$')
RE_TERM = re.compile(r'^  site (\d+) (\S+) at (\S+) in (\S+) depth=(\d+) \{([^}]*)\} guard=(.*)$')
RE_TGT = re.compile(r'^  site (\d+) (\S+) in (\S+) (spawn-entry|indirect-call) -> (\S+) via (.*) of (\d+)$')
RE_SP = re.compile(r'^  (\S+) param (\d+) (\S+): spawn site (\S+)(.*) via (.*)$')
RE_DER = re.compile(r'^  (\S+) (delay|terminal) (must|noreturn|may) \{([^}]*)\}: site (\S+)(?: (\S+))? -> (\S+) depth=(\d+) guard=(.*)$')
RE_SW = re.compile(r'^  (\S+) (entry-argument|called-by-entry|fixed-entry|script-entry)(?: param (\d+))? \{([^}]*)\}: site (\S+)(?: (\S+))? -> (\S+) depth=(\d+)(.*)$')


def base(n):
    return n.split('@', 1)[0]


def parse(path):
    secs = collections.OrderedDict()
    cur = None
    with open(path, errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            m = RE_SEC.match(line)
            if m:
                cur = m.group(1)
                secs[cur] = []
                continue
            if cur is not None:
                secs[cur].append(line)
    F = {'secs': secs}
    site_pos = {}
    calls = {}
    for ln in secs.get('calls', []):
        m = RE_CALL.match(ln)
        if m:
            pos = '%s:%s:%s' % (m.group(2), m.group(3), m.group(4))
            site_pos[m.group(1)] = pos
            flags = set(x for x in m.group(8).split(',') if x)
            calls[(pos, base(m.group(5)), m.group(6))] = flags
    F['site_pos'] = site_pos
    F['calls'] = calls
    loops = {}
    cur = None
    for ln in secs.get('loops', []):
        m = RE_LOOP.match(ln)
        if m:
            key = (m.group(3), base(m.group(4)))
            cur = {'kind': m.group(6), 'trip': m.group(7), 'exits': m.group(8), 'delay': None, 'exit': None, 'pred': None, 'rule': None, 'lock': None}
            loops[key] = cur
            continue
        if cur is None:
            continue
        s = ln.strip()
        if s.startswith('delay-call site '):
            cur['delay'] = site_pos.get(s.split()[-1], s.split()[-1])
        elif s.startswith('exit '):
            cur['exit'] = s[5:]
        elif s.startswith('predicate '):
            cur['pred'] = s[10:]
        elif s.startswith('lock '):
            cur['lock'] = s.split()[1]
        elif s.startswith('rule '):
            cur['rule'] = s[5:]
    F['loops'] = loops
    terms = {}
    for ln in secs.get('terminals', []):
        m = RE_TERM.match(ln)
        if m:
            terms[(m.group(3), m.group(2))] = {'fn': base(m.group(4)), 'flags': m.group(6), 'guard': m.group(7)}
    F['terms'] = terms
    spawns = collections.defaultdict(set)
    for ln in secs.get('targets', []):
        m = RE_TGT.match(ln)
        if m and m.group(4) == 'spawn-entry':
            spawns[(m.group(2), base(m.group(3)))].add(base(m.group(5)))
    F['spawns'] = spawns
    sp = set()
    for ln in secs.get('spawnparams', []):
        m = RE_SP.match(ln)
        if m:
            rest = re.sub(r'called at site \d+ ', 'called at ', m.group(5))
            sp.add((base(m.group(1)), m.group(2), m.group(3), rest.strip()))
    F['spawnparams'] = sp
    order = []
    for ln in secs.get('order', []):
        order.append(re.sub(r'\bsite \d+', 'site #', re.sub(r'^  loop \d+', '  loop #', ln)))
    F['order'] = order
    der = collections.defaultdict(list)
    cur = None
    for ln in secs.get('derived', []):
        m = RE_DER.match(ln)
        if m:
            cur = {'fn': base(m.group(1)), 'kind': m.group(2), 'mode': m.group(3), 'flags': m.group(4), 'at': m.group(6) or '-', 'next': base(m.group(7)),
                   'depth': int(m.group(8)), 'guard': m.group(9), 'chain': None}
            der[(cur['fn'], cur['kind'])].append(cur)
        elif cur is not None and ln.startswith('      chain '):
            cur['chain'] = ln[12:]
    F['derived'] = der
    sw = collections.defaultdict(list)
    cur = None
    for ln in secs.get('spawnwrap', []):
        m = RE_SW.match(ln)
        if m:
            cur = {'fn': base(m.group(1)), 'kind': m.group(2), 'param': m.group(3), 'flags': m.group(4), 'at': m.group(6) or '-', 'next': base(m.group(7)),
                   'rest': m.group(9), 'chain': None, 'via': None}
            sw[cur['fn']].append(cur)
        elif cur is not None and ln.startswith('      chain '):
            cur['chain'] = ln[12:]
        elif cur is not None and ln.startswith('      via '):
            cur['via'] = ln[10:]
    F['spawnwrap'] = sw
    return F


def path_of(b, v):
    if v == 'NP' or b == 'c5_642':
        return os.path.join(OUT, b, v, 'facts.txt')
    return os.path.join(EB, b, v, 'facts.txt')


os.makedirs(CD, exist_ok=True)
summary = []
items = []


def say(s=''):
    summary.append(s)
    print(s)


facts = {}
for b in BUILDS:
    for v in ('V1', 'VP', 'NP'):
        p = path_of(b, v)
        if os.path.exists(p):
            facts[(b, v)] = parse(p)

say('# step2a_s1_compare.py: NP (profile-free) against V1 (G0) and VP (P0 seeds), row numbers left out')
for b in BUILDS:
    if (b, 'NP') not in facts or (b, 'V1') not in facts:
        say('%s: missing variants' % b)
        continue
    N, V, P = facts[(b, 'NP')], facts[(b, 'V1')], facts.get((b, 'VP'))
    say('')
    say('## %s' % b)
    # ---- loop classes: every loop V1 classifies by its delay (count-bounded, poll) or gives an exit expression
    cnt = collections.Counter()
    for key, lv in sorted(V['loops'].items()):
        ln = N['loops'].get(key)
        lp = P['loops'].get(key) if P else None
        if lv['kind'] in ('count-bounded', 'poll') or lv['exit'] is not None or lv['delay'] is not None:
            if ln is None:
                st = 'absent'
            else:
                same_kind = ln['kind'] == lv['kind']
                same_exit = ln['exit'] == lv['exit']
                same_delay = (ln['delay'] is None) == (lv['delay'] is None)
                st = 'same' if same_kind and same_exit and same_delay and ln['rule'] == lv['rule'] else (
                    'same-kind-exit' if same_kind and same_exit and same_delay else 'differs')
            cnt[st] += 1
            items.append([b, 'loop', '%s in %s' % key, '%s exit=%s delay=%s rule=%s' % (lv['kind'], lv['exit'], lv['delay'], lv['rule']),
                          '%s exit=%s delay=%s rule=%s' % ((ln or {}).get('kind'), (ln or {}).get('exit'), (ln or {}).get('delay'), (ln or {}).get('rule')),
                          (lp or {}).get('kind'), st])
    # loops NP classifies that V1 does not
    extra = 0
    for key, ln in sorted(N['loops'].items()):
        lv = V['loops'].get(key)
        if ln['kind'] in ('count-bounded', 'poll') and (lv is None or lv['kind'] not in ('count-bounded', 'poll')):
            extra += 1
            items.append([b, 'loop-np-only', '%s in %s' % key, (lv or {}).get('kind'), '%s delay=%s' % (ln['kind'], ln['delay']), '', 'np-only'])
    kinds = lambda F: collections.Counter(l['kind'] for l in F['loops'].values())
    kv, kn, kp = kinds(V), kinds(N), kinds(P) if P else collections.Counter()
    say('loops: V1 count-bounded %d poll %d | VP %d %d | NP %d %d' % (kv['count-bounded'], kv['poll'], kp['count-bounded'], kp['poll'], kn['count-bounded'], kn['poll']))
    say('  V1 loops that wait or carry an exit expression: %s; NP-only waiting loops: %d' % (dict(cnt), extra))
    # ---- task-main order
    ov, on = V['order'], N['order']
    say('order: V1 %d lines, NP %d lines, %s' % (len(ov), len(on), 'identical' if ov == on else 'differ (%d only in V1, %d only in NP)' % (
        sum((collections.Counter(ov) - collections.Counter(on)).values()), sum((collections.Counter(on) - collections.Counter(ov)).values()))))
    items.append([b, 'order', 'the [order] section', '%d lines' % len(ov), '%d lines' % len(on), '%d lines' % len(P['order']) if P else '', 'same' if ov == on else 'differs'])
    # ---- spawn facts: spawn calls and their entries
    sv, sn = V['spawns'], N['spawns']
    c = collections.Counter()
    for k, ent in sorted(sv.items()):
        en = sn.get(k)
        st = 'absent' if en is None else ('same' if en == ent else ('superset' if en > ent else 'differs'))
        c[st] += 1
        items.append([b, 'spawn', '%s in %s' % k, ','.join(sorted(ent)), ','.join(sorted(en)) if en is not None else '-',
                      ','.join(sorted(P['spawns'].get(k, []))) if P else '', st])
    npo = [k for k in sn if k not in sv]
    for k in sorted(npo):
        items.append([b, 'spawn-np-only', '%s in %s' % k, '-', ','.join(sorted(sn[k])), ','.join(sorted(P['spawns'].get(k, []))) if P else '', 'np-only'])
    unres = lambda S: sum(1 for e in S.values() if any(x.startswith('UNRESOLVED') for x in e))
    say('spawn calls: V1 %d (unresolved entry %d), VP %d, NP %d (unresolved entry %d); V1 calls in NP: %s; NP-only %d' % (
        len(sv), unres(sv), len(P['spawns']) if P else -1, len(sn), unres(sn), dict(c), len(npo)))
    spv, spn = V['spawnparams'], N['spawnparams']
    say('spawn parameters: V1 %d, VP %d, NP %d; V1 rows in NP %d, NP-only %d' % (len(spv), len(P['spawnparams']) if P else -1, len(spn), len(spv & spn), len(spn - spv)))
    for r in sorted(spv | spn):
        items.append([b, 'spawnparam', '%s param %s %s' % r[:3], 'yes' if r in spv else '-', 'yes' if r in spn else '-', 'yes' if P and r in P['spawnparams'] else '-',
                      'same' if (r in spv and r in spn) else ('v1-only' if r in spv else 'np-only')])
    # ---- lock flags
    lk = lambda F: {k: f & {'lock-op', 'unlock-op'} for k, f in F['calls'].items() if f & {'lock-op', 'unlock-op'}}
    lv_, ln_, lp_ = lk(V), lk(N), lk(P) if P else {}
    back = sum(1 for k in lv_ if ln_.get(k) == lv_[k])
    say('lock flags: V1 %d calls, VP %d, NP %d; V1 calls flagged alike in NP %d' % (len(lv_), len(lp_), len(ln_), back))
    for k in sorted(set(lv_) | set(ln_)):
        items.append([b, 'lock', '%s %s -> %s' % k, ','.join(sorted(lv_.get(k, []))) or '-', ','.join(sorted(ln_.get(k, []))) or '-', ','.join(sorted(lp_.get(k, []))) or '-',
                      'same' if lv_.get(k) == ln_.get(k) else ('v1-only' if k in lv_ and k not in ln_ else ('np-only' if k not in lv_ else 'differs'))])
    # ---- terminal facts
    tv, tn = V['terms'], N['terms']
    c = collections.Counter()
    for k, r in sorted(tv.items()):
        x = tn.get(k)
        if x is None:
            st = 'absent'
        else:
            st = 'same' if x['flags'] == r['flags'] and x['guard'] == r['guard'] else ('same-guard' if x['guard'] == r['guard'] else 'differs')
        c[st] += 1
        items.append([b, 'terminal', '%s %s' % k, '%s {%s} %s' % (r['fn'], r['flags'], r['guard']), '-' if x is None else '%s {%s} %s' % (x['fn'], x['flags'], x['guard']),
                      'yes' if P and k in P['terms'] else '-', st])
    npo = sorted(k for k in tn if k not in tv)
    for k in npo:
        x = tn[k]
        items.append([b, 'terminal-np-only', '%s %s' % k, '-', '%s {%s} %s' % (x['fn'], x['flags'], x['guard']), 'yes' if P and k in P['terms'] else '-', 'np-only'])
    say('terminal facts: V1 %d, VP %d, NP %d; V1 rows in NP: %s; NP-only %d (by operation: %s)' % (
        len(tv), len(P['terms']) if P else -1, len(tn), dict(c), len(npo), dict(collections.Counter(k[1] for k in npo).most_common(8))))
    # ---- sensitivity of the vocabulary rules (rule choices of the derivation, no numeric bound)
    cond = sum(1 for r in tn.values() if 'conditional' in r['flags'])
    say('terminal rows of conditional (noreturn / may) operations: %d of %d (a vocabulary of must operations only would leave %d)' % (cond, len(tn), len(tn) - cond))
    ext = os.path.join(OUT, b, 'NP', 'extract.txt')
    if os.path.exists(ext):
        with open(ext) as f:
            for ln in f:
                if ln.startswith('  delay operations (F8)') or ln.startswith('  terminal operations (F8)') or ln.startswith('  spawn operations (F7)') \
                        or ln.startswith('  startup-script entries') or ln.startswith('  first pass'):
                    say('derivation:' + ln.rstrip('\n')[1:])
    # ---- call flags of the operations
    for fl in OPFLAGS:
        a = {k for k, f in V['calls'].items() if fl in f}
        n = {k for k, f in N['calls'].items() if fl in f}
        p = {k for k, f in P['calls'].items() if fl in f} if P else set()
        say('call flag %-12s V1 %4d  VP %4d  NP %4d  (V1 calls also in NP %d, NP-only %d)' % (fl, len(a), len(p), len(n), len(a & n), len(n - a)))

# ---- static actor roots (evaluation lookup; the root rule of extract_baseline_count.py): process root main, spawn
# entries, the task-main elements of the [order] section.  Which roots reach the R2 SB functions that pass the SB
# application id to the event call, and the SB task's initialisation (names used to read the output only).
RE_ORD_HDR = re.compile(r'^  loop # .*\(task-main\)$')
RE_ORD_IT = re.compile(r'^    iter \d+ \S+ -> (\S+)')


def roots_and_reach(F):
    defined = set()
    for ln in F['secs'].get('functions', []):
        m = re.match(r'^  fn \d+ (\S+) \S+ params=\d+ \{([^}]*)\}', ln)
        if m and 'defined' in m.group(2):
            defined.add(base(m.group(1)))
    cg = collections.defaultdict(set)
    for (pos, caller, callee), fl in F['calls'].items():
        if callee in defined:
            cg[caller].add(callee)
    for ln in F['secs'].get('targets', []):
        m = RE_TGT.match(ln)
        if m and m.group(4) == 'indirect-call' and not m.group(5).startswith('UNRESOLVED'):
            cg[base(m.group(3))].add(base(m.group(5)))
    roots = {}
    if 'main' in defined:
        roots['main'] = 'process'
    for k, ent in F['spawns'].items():
        for e in ent:
            if e in defined:
                roots.setdefault(e, 'spawn-entry')
    hdr = False
    for ln in F['order']:
        if ln.startswith('  loop '):
            hdr = bool(RE_ORD_HDR.match(ln))
            continue
        m = RE_ORD_IT.match(ln)
        if hdr and m and base(m.group(1)) in defined:
            roots.setdefault(base(m.group(1)), 'table-walk task-main')
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
    return roots, reach


say('')
say('# static actor roots (evaluation lookup): which roots reach the SB functions with the event call and the SB initialisation (r2_core)')
for v in ('V1', 'VP', 'NP'):
    F = facts.get(('r2_core', v))
    if F is None:
        continue
    roots, reach = roots_and_reach(F)
    sbfns = sorted({c for (pos, c, callee) in F['calls'] if callee == 'CFE_EVS_SendEventWithAppID' and '/sb/' in pos})
    mains = {r for r in roots if r.endswith('_TaskMain')}
    api_roots = collections.Counter()
    for fn in sbfns:
        api_roots[tuple(sorted(r for r in reach.get(fn, ()) if r in mains))] += 1
    init = sorted(r for r in reach.get('CFE_SB_AppInit', ()) if r in mains)
    say('%s: roots %d (%s); task-main roots %s; SB functions with the event call %d, by task-main roots reaching them: %s; CFE_SB_AppInit reached by task-main roots %s' % (
        v, len(roots), dict(collections.Counter(roots.values())), sorted(mains), len(sbfns), dict(api_roots), init))

with open(os.path.join(CD, 'items.tsv'), 'w') as f:
    f.write('build\tcategory\tkey\tV1\tNP\tVP\tstatus\n')
    for r in items:
        f.write('\t'.join('' if x is None else str(x) for x in r) + '\n')

# ---- what NP derived for each G0 name, and every derived operation with its chain
rows = []
with open(os.path.join(CD, 'chains.txt'), 'w') as o:
    for b in BUILDS:
        if (b, 'NP') not in facts:
            continue
        N = facts[(b, 'NP')]
        o.write('## %s\n' % b)
        for (fn, kind), rs in sorted(N['derived'].items()):
            r0 = sorted(rs, key=lambda r: r['depth'])[0]
            o.write('%s %s %s {%s} %d rows; shortest: %s\n' % (kind, r0['mode'], fn, r0['flags'], len(rs), r0['chain']))
        for fn, rs in sorted(N['spawnwrap'].items()):
            for r in rs:
                o.write('spawn %s %s param=%s {%s} at %s -> %s%s; %s%s\n' % (r['kind'], fn, r['param'], r['flags'], r['at'], r['next'], r['rest'], r['chain'],
                                                                      ('; via ' + r['via']) if r['via'] else ''))
        for kind, names in G0.items():
            for nm in names:
                if kind in ('delay', 'terminal'):
                    rs = N['derived'].get((nm, kind), [])
                    st = rs[0]['mode'] + (' (vocabulary)' if 'vocabulary' in rs[0]['flags'] else ' (not vocabulary)') if rs else 'not derived'
                    ch = sorted(rs, key=lambda r: r['depth'])[0]['chain'] if rs else '-'
                    if nm == 'abort':
                        st, ch = 'P0 primitive', 'abort'
                elif kind == 'spawn':
                    rs = N['spawnwrap'].get(nm, [])
                    st = rs[0]['kind'] + (' (vocabulary)' if 'vocabulary' in rs[0]['flags'] else '') if rs else 'not derived'
                    ch = rs[0]['chain'] if rs else '-'
                else:
                    st, ch = 'not derived (F5, lock wrappers, is not part of S1)', '-'
                present = nm in {k[2] for k in N['calls']} or any(k[1] == nm for k in N['calls'])
                rows.append([b, kind, nm, 'called' if present else 'not called', st, ch])
with open(os.path.join(CD, 'g0_names.tsv'), 'w') as f:
    f.write('build\tG0 kind\tname\tin NP calls\tNP derivation\tchain\n')
    for r in rows:
        f.write('\t'.join(r) + '\n')
say('')
say('# what NP derived for each G0 name (g0_names.tsv)')
for r in rows:
    say('%-10s %-8s %-24s %-10s %-34s %s' % tuple(r))
with open(os.path.join(CD, 'summary.txt'), 'w') as f:
    f.write('\n'.join(summary) + '\n')

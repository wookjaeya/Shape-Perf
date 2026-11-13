#!/usr/bin/env python3
# step2a_s2_count.py OUT S1 COUNT -- counts and spot checks of stage S2 of redesign step 2a (step2a_s2.sh count).
#
# Reads the text views (`kz-static show`) of the profile-free runs OUT/<build>/NP/ (reads.txt, returns.txt, retconsts.txt,
# loopexits.txt, calls.txt, facts.txt) and writes into COUNT:
#   counts.tsv     per build: rows of the four new sections and their breakdowns (kinds, uses, flags)
#   depth.txt      the expression check of the new sections: nodes cut by the existing depth bound of the expression
#                  builder ("too deep"), and the largest nesting of a printed tree
#   earlier.txt    the earlier sections of the profile-free facts against stage S1 (OUT of step2a_s1.sh): the view without
#                  the four new sections and the lines that name the tool or the basis hash
#   spot_<build>.txt  the spot checks: rows at the source places the completion tests T1-T4 of
#                  plan/extraction_design_20261030.md §6 will use.  The case names below are evaluation queries over the
#                  output; nothing of them is an input of the extraction.
#   summary.txt    all of it in short
import os
import re
import sys
from collections import Counter

OUT, S1, COUNT = sys.argv[1], sys.argv[2], sys.argv[3]
BUILDS = ['c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641', 'c5_642']
os.makedirs(COUNT, exist_ok=True)


def lines(path):
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            return f.read().splitlines()
    except FileNotFoundError:
        return []


def flagset(text):
    return set(x for x in text.split(',') if x)


# ---- parsing the views -------------------------------------------------------------------------------------------
RE_READ = re.compile(r'^  (\S+) reads (field|global) (\S+) at (\S+):(\d+):(\d+) root=([a-z-]+):(\S*?)(?:#(\d+))? use=\{([^}]*)\} \{([^}]*)\} '
                     r'\[(.*?)\](?= index=\[| arg \d+ of site| loop=| ctx=| guard=|$)( index=\[[^\]]*\])?(.*)$')
RE_RET = re.compile(r'^  (\S+) returns at (\S+):(\d+):(\d+) alt (\d+)/(\d+) (\S+) (.*) \[(.*)\] \{([^}]*)\}(.*)$')
RE_RETC = re.compile(r'^  (\S+) returns (-?\d+) \[(.*)\] rows=(\d+) \{([^}]*)\} when=(.*)$')
RE_LOOP = re.compile(r'^  loop (\d+) (\S+):(\d+):(\d+) in (\S+) exits=(\d+) \{([^}]*)\} exit=(.*) mods=(.*)$')
RE_EXIT = re.compile(r'^      exit (\d+) (\S+) at (\S+):(\d+):(\d+) \{([^}]*)\} when=(.*) by=(.*)$')
RE_CALL = re.compile(r'^  site (\d+) (\S+):(\d+):(\d+) (\S+) -> (\S+) (\S+) \{([^}]*)\}')


def parse_reads(b):
    rows = []
    for ln in lines(os.path.join(OUT, b, 'NP', 'reads.txt')):
        m = RE_READ.match(ln)
        if not m:
            continue
        rest = m.group(14) or ''
        site = re.search(r' arg (\d+) of site (\d+) ', rest + ' ')
        rows.append(dict(fn=m.group(1), kind=m.group(2), loc=m.group(3), file=m.group(4), line=int(m.group(5)), col=int(m.group(6)),
                         root_kind=m.group(7), root=m.group(8), use=flagset(m.group(10)), flags=flagset(m.group(11)), text=m.group(12),
                         index=(m.group(13) or '').strip(), arg=int(site.group(1)) if site else None, site=int(site.group(2)) if site else None,
                         loop=' loop=' in rest, raw=ln))
    return rows


def parse_returns(b):
    rows, cur = [], None
    for ln in lines(os.path.join(OUT, b, 'NP', 'returns.txt')):
        m = RE_RET.match(ln)
        if m:
            cur = dict(fn=m.group(1), file=m.group(2), line=int(m.group(3)), alt=int(m.group(5)), n=int(m.group(6)), vkind=m.group(7), value=m.group(8),
                       text=m.group(9), flags=flagset(m.group(10)), raw=[ln])
            rows.append(cur)
        elif cur is not None and ln.startswith('      '):
            cur['raw'].append(ln)
    return rows


def parse_retconsts(b):
    out = []
    for ln in lines(os.path.join(OUT, b, 'NP', 'retconsts.txt')):
        m = RE_RETC.match(ln)
        if m:
            out.append(dict(fn=m.group(1), value=int(m.group(2)), text=m.group(3), rows=int(m.group(4)), flags=flagset(m.group(5)), when=m.group(6), raw=ln))
    return out


def parse_loopexits(b):
    loops, exits = [], []
    for ln in lines(os.path.join(OUT, b, 'NP', 'loopexits.txt')):
        m = RE_LOOP.match(ln)
        if m:
            loops.append(dict(idx=int(m.group(1)), file=m.group(2), line=int(m.group(3)), fn=m.group(5), n=int(m.group(6)), flags=flagset(m.group(7)), exit=m.group(8),
                              raw=ln, exits=[]))
            continue
        m = RE_EXIT.match(ln)
        if m and loops:
            e = dict(k=int(m.group(1)), kind=m.group(2), file=m.group(3), line=int(m.group(4)), flags=flagset(m.group(6)), when=m.group(7), by=m.group(8), raw=ln)
            exits.append(e)
            loops[-1]['exits'].append(e)
    return loops, exits


def parse_calls(b):
    out = {}
    for ln in lines(os.path.join(OUT, b, 'NP', 'calls.txt')):
        m = RE_CALL.match(ln)
        if m:
            out[int(m.group(1))] = dict(file=m.group(2), line=int(m.group(3)), caller=m.group(5), callee=m.group(6), flags=flagset(m.group(8)), raw=ln)
    return out


def basis_counts(b):
    out = {}
    for ln in lines(os.path.join(OUT, b, 'NP', 'facts.txt')):
        m = re.match(r'^  (count\.\S+) = (\S+)', ln)
        if m:
            out[m.group(1)] = m.group(2)
    return out


# ---- counts ------------------------------------------------------------------------------------------------------
USES = ['branch', 'return', 'out-store', 'store', 'call-argument', 'subscript', 'dereference', 'callee', 'other']
RFLAGS = ['through-pointer', 'array-contents', 'whole-record', 'in-macro', 'index-unfolded']
VKINDS = ['const', 'field', 'global', 'call-result', 'param', 'local', 'address', 'string', 'expression']
RETFLAGS = ['from-definition', 'redefined-later', 'undefined-on-a-path', 'unfolded', 'in-loop', 'in-macro', 'no-cfg']
EXKINDS = ['condition', 'break', 'return', 'goto']

table = []  # (metric, {build: value})
data = {}
for b in BUILDS:
    reads = parse_reads(b)
    rets = parse_returns(b)
    rc = parse_retconsts(b)
    loops, exits = parse_loopexits(b)
    calls = parse_calls(b)
    bc = basis_counts(b)
    data[b] = dict(reads=reads, rets=rets, rc=rc, loops=loops, exits=exits, calls=calls, bc=bc)


def put(metric, fn):
    table.append((metric, {b: fn(data[b]) for b in BUILDS}))


put('basis count.loops (FACT_LOOPS rows)', lambda d: d['bc'].get('count.loops', '?'))
put('READS rows', lambda d: len(d['reads']))
put('READS kind field', lambda d: sum(1 for r in d['reads'] if r['kind'] == 'field'))
put('READS kind global', lambda d: sum(1 for r in d['reads'] if r['kind'] == 'global'))
put('READS distinct locations', lambda d: len(set(r['loc'] for r in d['reads'])))
put('READS functions with a read', lambda d: len(set(r['fn'] for r in d['reads'])))
for u in USES:
    put('READS use ' + u, lambda d, u=u: sum(1 for r in d['reads'] if u in r['use']))
for f in RFLAGS:
    put('READS flag ' + f, lambda d, f=f: sum(1 for r in d['reads'] if f in r['flags']))
for rk in ['global', 'param', 'local', 'call-result', 'other']:
    put('READS root ' + rk, lambda d, rk=rk: sum(1 for r in d['reads'] if r['root_kind'] == rk))
put('READS with subscripts', lambda d: sum(1 for r in d['reads'] if r['index']))
put('READS in a loop', lambda d: sum(1 for r in d['reads'] if r['loop']))
put('RETURNS rows (alternatives)', lambda d: len(d['rets']))
put('RETURNS return statements', lambda d: sum(1 for r in d['rets'] if r['alt'] == 0))
put('RETURNS statements with several alternatives', lambda d: sum(1 for r in d['rets'] if r['alt'] == 0 and r['n'] > 1))
put('RETURNS functions', lambda d: len(set(r['fn'] for r in d['rets'])))
for v in VKINDS:
    put('RETURNS value ' + v, lambda d, v=v: sum(1 for r in d['rets'] if r['vkind'] == v))
for f in RETFLAGS:
    put('RETURNS flag ' + f, lambda d, f=f: sum(1 for r in d['rets'] if f in r['flags']))
put('RET_CONSTS rows', lambda d: len(d['rc']))
put('RET_CONSTS functions', lambda d: len(set(r['fn'] for r in d['rc'])))
put('RET_CONSTS functions with only constants', lambda d: len(set(r['fn'] for r in d['rc'] if 'only-constants' in r['flags'])))
put('RET_CONSTS rows returned always', lambda d: sum(1 for r in d['rc'] if r['when'] == 'always'))
put('RET_CONSTS rows with redefined-later', lambda d: sum(1 for r in d['rc'] if 'redefined-later' in r['flags']))
put('RET_CONSTS rows with in-loop', lambda d: sum(1 for r in d['rc'] if 'in-loop' in r['flags']))
put('LOOP_EXITS loops', lambda d: len(d['loops']))
put('LOOP_EXITS loops as-in-loops (FACT_LOOPS has the trees)', lambda d: sum(1 for l in d['loops'] if 'as-in-loops' in l['flags']))
put('LOOP_EXITS loops whose exit trees are new (no delay call, no table walk)', lambda d: sum(1 for l in d['loops'] if 'as-in-loops' not in l['flags']))
put('LOOP_EXITS loops no-exit', lambda d: sum(1 for l in d['loops'] if 'no-exit' in l['flags']))
put('LOOP_EXITS exit rows', lambda d: len(d['exits']))
for k in EXKINDS:
    put('LOOP_EXITS exits ' + k, lambda d, k=k: sum(1 for e in d['exits'] if e['kind'] == k))
put('LOOP_EXITS exits never-taken', lambda d: sum(1 for e in d['exits'] if 'never-taken' in e['flags']))
put('LOOP_EXITS exits unconditional', lambda d: sum(1 for e in d['exits'] if 'unconditional' in e['flags']))

with open(os.path.join(COUNT, 'counts.tsv'), 'w') as f:
    f.write('metric\t' + '\t'.join(BUILDS) + '\n')
    for metric, vals in table:
        f.write(metric + '\t' + '\t'.join(str(vals[b]) for b in BUILDS) + '\n')


# ---- expression depth: the bound of the expression builder (existing, 24 levels: "too deep") ----------------------
def max_nesting(s):
    d = m = 0
    for ch in s:
        if ch == '(':
            d += 1
            m = max(m, d)
        elif ch == ')':
            d -= 1
    return m


with open(os.path.join(COUNT, 'depth.txt'), 'w') as f:
    f.write('# the four new sections of the profile-free runs: nodes cut by the expression builder bound ("too deep"), the largest\n'
            '# nesting of a printed tree (the view prints at most 40 levels and then "...")\n')
    for b in BUILDS:
        cut_all = lines(os.path.join(OUT, b, 'NP', 'too_deep.txt'))
        tot_cut = 0
        tot_ellipsis = 0
        mx = 0
        for s in ['reads', 'returns', 'retconsts', 'loopexits']:
            for ln in lines(os.path.join(OUT, b, 'NP', s + '.txt')):
                tot_cut += ln.count('too deep')
                tot_ellipsis += ln.count('...')
                mx = max(mx, max_nesting(ln))
        f.write('%s\ttoo-deep nodes in the view of the new sections %d\ttoo-deep rows of FACT_EXPRS (kz-dump, every section) %s\tview cut at 40 levels %d\t'
                'largest printed nesting %d\n' % (b, tot_cut, cut_all[0] if cut_all else '?', tot_ellipsis, mx))


# ---- the earlier sections of the profile-free facts against S1 ----------------------------------------------------
def earlier_view(path, drop_new):
    out, skip = [], False
    for ln in lines(path):
        if ln.startswith('['):
            skip = drop_new and any(ln.startswith('[' + s + ']') for s in ['reads', 'returns', 'retconsts', 'loopexits'])
        if skip:
            continue
        if re.search(r'build_identity|extract\.units_from|extract\.extended_facts|count\.(reads|returns|return_constants|loop_exit_rows)', ln):
            continue
        if ln.startswith('[basis] '):
            continue
        out.append(ln)
    return out


with open(os.path.join(COUNT, 'earlier.txt'), 'w') as f:
    f.write('# the view of the profile-free facts without the four new sections, against stage S1 (same builds, same inputs);\n'
            '# lines naming the tool build, the basis hash and the new BASIS items removed\n')
    for b in BUILDS:
        a = earlier_view(os.path.join(S1, b, 'NP', 'facts.txt'), False)
        c = earlier_view(os.path.join(OUT, b, 'NP', 'facts.txt'), True)
        import difflib
        diff = [d for d in difflib.unified_diff(a, c, lineterm='', n=0) if d[:1] in '+-' and not d.startswith(('+++', '---'))]
        f.write('%s\tS1 lines %d\tS2 lines %d\tdiffering lines %d\n' % (b, len(a), len(c), len(diff)))
        for d in diff[:20]:
            f.write('    ' + d[:300] + '\n')


# ---- spot checks (evaluation queries; case names appear only here) ------------------------------------------------
def spot(b, title, rows, out):
    out.append('## ' + title)
    if not rows:
        out.append('   (none)')
    for r in rows:
        if isinstance(r, list):
            out.extend('   ' + x for x in r)
        else:
            out.append('   ' + r)


verdicts = []
for b in BUILDS:
    d = data[b]
    out = ['# spot checks %s (profile-free run %s/%s/NP)' % (b, OUT, b)]
    reads, rets, rc, loops, calls = d['reads'], d['rets'], d['rc'], d['loops'], d['calls']
    if b == 'r2_core':
        # T1: the field read passed as argument 2 of the 39 event calls, and the callee's check
        ev = [r for r in reads if r['loc'] == 'CFE_SB_Global_t.AppId' and r['arg'] == 2 and r['site'] in calls and calls[r['site']]['callee'] == 'CFE_EVS_SendEventWithAppID']
        byfile = Counter(r['file'].split('/')[-1] for r in ev)
        out.append('## T1 reads CFE_SB_Global_t.AppId as argument 2 of CFE_EVS_SendEventWithAppID: %d (%s); uses %s; results of those calls used %d'
                   % (len(ev), ', '.join('%s %d' % kv for kv in sorted(byfile.items())), sorted(Counter(','.join(sorted(r['use'])) for r in ev).items()),
                      sum(1 for r in ev if 'result-used' in calls[r['site']]['flags'])))
        for fn in ['CFE_EVS_SendEventWithAppID', 'EVS_GetAppDataByID', 'CFE_ES_AppID_ToIndex', 'CFE_ResourceId_ToIndex']:
            spot(b, 'RET_CONSTS ' + fn, [r['raw'] for r in rc if r['fn'] == fn], out)
            spot(b, 'RETURNS ' + fn, [r['raw'] for r in rets if r['fn'] == fn], out)
        for fn in ['EVS_AppDataIsMatch', 'EVS_GetAppDataByID', 'CFE_ResourceId_ToIndex']:
            spot(b, 'READS ' + fn, [r['raw'] for r in reads if r['fn'] == fn], out)
        ill = [r for r in rc if r['fn'] == 'CFE_EVS_SendEventWithAppID' and r['text'] == 'CFE_EVS_APP_ILLEGAL_APP_ID']
        ok = bool(ill) and '(eq (call-result EVS_GetAppDataByID' in ill[0]['when']
        null = [r for r in rc if r['fn'] == 'EVS_GetAppDataByID' and r['value'] == 0]
        verdicts.append('%s T1: %d event-call reads of CFE_SB_Global_t.AppId (arg 2); callee returns CFE_EVS_APP_ILLEGAL_APP_ID when EVS_GetAppDataByID(...) == 0: %s; '
                        'EVS_GetAppDataByID returns 0 (NULL) when: %s' % (b, len(ev), 'yes' if ok else 'NO', null[0]['when'] if null else 'NOT FOUND'))
    if b.startswith('c1_'):
        cfline = 993 if b == 'c1_pre_tx' else 1003
        for (fname, lns) in [('bp_flow.c', [126, 132]), ('cf_cfdp.c', [cfline])]:
            sites = set(k for k, c in calls.items() if c['file'].endswith('/' + fname) and c['line'] in lns)
            spot(b, 'calls at %s:%s' % (fname, '/'.join(map(str, lns))), [calls[k]['raw'] for k in sorted(sites)], out)
            spot(b, 'READS at %s:%s or in the arguments of those calls' % (fname, '/'.join(map(str, lns))),
                 [r['raw'] for r in reads if r['file'].endswith('/' + fname) and (r['line'] in lns or r['site'] in sites)], out)
        for fn in ['initialize_throttling', 'CF_CFDP_InitEngine']:
            spot(b, 'LOOP_EXITS ' + fn, [[l['raw']] + [e['raw'] for e in l['exits']] for l in loops if l['fn'].split('@')[0] == fn], out)
        spot(b, 'RETURNS CF_CFDP_InitEngine', [r['raw'] for r in rets if r['fn'] == 'CF_CFDP_InitEngine'], out)
        for fn in ['OS_CountSemGetIdByName', 'OS_CountSemCreate', 'OS_ObjectIdFindByName', 'OS_ObjectIdGetBySearch', 'OS_ObjectIdFindNextMatch', 'OS_ObjectNameMatch']:
            spot(b, 'RET_CONSTS ' + fn, [r['raw'] for r in rc if r['fn'].split('@')[0] == fn], out)
            spot(b, 'RETURNS ' + fn, [r['raw'] for r in rets if r['fn'].split('@')[0] == fn], out)
        spot(b, 'READS OS_ObjectNameMatch', [r['raw'] for r in reads if r['fn'].split('@')[0] == 'OS_ObjectNameMatch'], out)
        bpsites = set(k for k, c in calls.items() if c['file'].endswith('/bp_flow.c') and c['line'] in (126, 132))
        cfsites = set(k for k, c in calls.items() if c['file'].endswith('/cf_cfdp.c') and c['line'] == cfline)
        bp = [r for r in reads if r['site'] in bpsites and r['loc'] == 'BP_Throttle_t.Name']
        cf = [r for r in reads if r['site'] in cfsites and r['loc'] == 'CF_ChannelConfig_t.sem_name']
        defs = [r for r in rets if r['fn'] == 'CF_CFDP_InitEngine' and any('cf_cfdp.c:%d:' % cfline in x for x in r['raw'])]
        retry = [l for l in loops if l['file'].endswith('/cf_cfdp.c') and l['line'] == 1001]
        verdicts.append('%s T2: BP reads BP_Throttle_t.Name at bp_flow.c:126/132 %d (uses %s); CF reads CF_ChannelConfig_t.sem_name at cf_cfdp.c:%d %d (uses %s); '
                        'CF_CFDP_InitEngine return alternatives from the definition at cf_cfdp.c:%d: %d; retry loop cf_cfdp.c:1001: %s'
                        % (b, len(bp), sorted(set(','.join(sorted(r['use'])) for r in bp)), cfline, len(cf), sorted(set(','.join(sorted(r['use'])) for r in cf)), cfline,
                           len(defs), ('exit=' + retry[0]['exit']) if retry else 'none'))
    if b.startswith('c5_'):
        for fn in ['CFE_ES_GetAppID', 'CFE_ES_GetAppIDInternal']:
            spot(b, 'READS ' + fn, [r['raw'] for r in reads if r['fn'] == fn], out)
            spot(b, 'RETURNS ' + fn, [r['raw'] for r in rets if r['fn'] == fn], out)
            spot(b, 'RET_CONSTS ' + fn, [r['raw'] for r in rc if r['fn'] == fn], out)
        # T4: the search of the OSAL task table by the calling thread's id (no in-use test in that search, ED §3.1)
        spot(b, 'READS OS_TaskRegister', [r['raw'] for r in reads if r['fn'] == 'OS_TaskRegister'], out)
        spot(b, 'LOOP_EXITS OS_TaskRegister', [[l['raw']] + [e['raw'] for e in l['exits']] for l in loops if l['fn'] == 'OS_TaskRegister'], out)
        used = [r for r in reads if r['fn'] == 'CFE_ES_GetAppID' and r['loc'].endswith('.RecordUsed')]
        err = [r for r in rc if r['fn'] == 'CFE_ES_GetAppID' and r['text'] == 'CFE_ES_ERR_APPID']
        verdicts.append('%s T3/T4: CFE_ES_GetAppID reads %s (uses %s, index %s); returns CFE_ES_ERR_APPID when %s'
                        % (b, used[0]['loc'] if used else 'NO RecordUsed READ', sorted(used[0]['use']) if used else '-', used[0]['index'] if used else '-',
                           err[0]['when'] if err else 'NOT FOUND'))
    with open(os.path.join(COUNT, 'spot_%s.txt' % b), 'w') as f:
        f.write('\n'.join(out) + '\n')

with open(os.path.join(COUNT, 'summary.txt'), 'w') as f:
    f.write('# step2a_s2_count.py summary\n')
    f.write('## counts (counts.tsv)\n')
    f.write('metric\t' + '\t'.join(BUILDS) + '\n')
    for metric, vals in table:
        f.write(metric + '\t' + '\t'.join(str(vals[b]) for b in BUILDS) + '\n')
    f.write('## expression depth (depth.txt)\n')
    f.write(open(os.path.join(COUNT, 'depth.txt')).read())
    f.write('## earlier sections against S1 (earlier.txt)\n')
    f.write(''.join(l + '\n' for l in lines(os.path.join(COUNT, 'earlier.txt')) if not l.startswith('    ')))
    f.write('## spot checks (spot_<build>.txt)\n')
    for v in verdicts:
        f.write(v + '\n')
print(open(os.path.join(COUNT, 'summary.txt')).read())

#!/usr/bin/env python3
"""step2a_tests_count.py OUT EB [S5 [FIX1 [FIX2]]] -- evidence for the completion tests T1-T7
(plan/extraction_design_20261030.md §6), the re-measure of ED §2.3 under the rule targets, and the candidate burden of
`kz-static relations`; with S5 (the outputs of stage S5, kept when the fix stage re-ran the tests), the rows whose
verdicts the fix stage changed; with FIX1 (the outputs of the first fix stage, kept when the second re-ran the tests),
the rows whose verdicts the second changed; with FIX2 (the outputs of the second fix stage), those the third changed.

Reads OUT/<build>/NP/{relations.txt,facts.txt,extract.txt} written by step2a_tests.sh (profile-free extraction with
--targets-rule R-T1,R-T2; no case input).  The case names below are queries of this evaluation script only: they pick
the rows the tests name out of the tool's output, and never reach the tool.  Every finding names its line in the file
(relations.txt:L, facts.txt:L), so the results document can cite it.  Writes OUT/count/*.txt and *.tsv.
"""
import collections
import os
import re
import sys

OUT, EB = sys.argv[1], sys.argv[2]
S5 = sys.argv[3] if len(sys.argv) > 3 else None
FIX1 = sys.argv[4] if len(sys.argv) > 4 else None
FIX2 = sys.argv[5] if len(sys.argv) > 5 else None
CD = os.path.join(OUT, 'count')
os.makedirs(CD, exist_ok=True)
BUILDS = ['c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641', 'c5_642']
out_lines = []


def say(s=''):
    out_lines.append(s)
    print(s)


def load(build, name, root=None):
    p = os.path.join(root or OUT, build, 'NP', name)
    if not os.path.exists(p):
        return []
    with open(p, encoding='utf-8', errors='replace') as f:
        return f.read().split('\n')


REL = {b: load(b, 'relations.txt') for b in BUILDS}
FACTS = {b: load(b, 'facts.txt') for b in BUILDS}
ROW = re.compile(r'^  ([ABC])(\d+) ')
NS = re.compile(r'^  namespace (\S+) ')


def blocks(lines):
    """rows and namespace blocks: (kind, id, first line index, end index)"""
    out = []
    cur = None
    for i, l in enumerate(lines):
        m = ROW.match(l)
        n = NS.match(l)
        head = m or n or l.startswith('[') or (l and not l.startswith(' '))
        if head and cur is not None:
            out.append((cur[0], cur[1], cur[2], i))
            cur = None
        if m:
            cur = (m.group(1), m.group(1) + m.group(2), i)
        elif n:
            cur = ('NS', n.group(1), i)
    if cur is not None:
        out.append((cur[0], cur[1], cur[2], len(lines)))
    return out


BLK = {b: blocks(REL[b]) for b in BUILDS}


def ref(build, i):
    return 'relations.txt:%d' % (i + 1)


def find_in(lines, s, e, pat):
    rx = re.compile(pat)
    return [(i, lines[i]) for i in range(s, e) if rx.search(lines[i])]


def show(build, hits, what, cut=260):
    if not hits:
        say('    MISSING %s' % what)
        return False
    for i, l in hits:
        say('    %s %s: %s' % (ref(build, i), what, l.strip()[:cut]))
    return True


def rows_where(build, kind, head_pat):
    rx = re.compile(head_pat)
    return [(k, rid, s, e) for (k, rid, s, e) in BLK[build] if k == kind and rx.search(REL[build][s])]


# ---------------------------------------------------------------- summary per build
say('# relations per build (the summary line of relations.txt)')
for b in BUILDS:
    L = REL[b]
    summ = next((l for l in L if l.startswith('summary ')), 'MISSING')
    say('%s: %s' % (b, summ))
    act = next((l for l in L if l.startswith('actors: ')), '')
    say('    %s' % act[:700])
    for pre in ('spawn calls resolved through the static image', 'spawn calls with no resolved entry', 'basis deviations copied', '[C] '):
        h = next((l for l in L if l.startswith(pre)), '')
        if h:
            say('    %s' % h[:900])

# ---------------------------------------------------------------- T1
say('')
say('# T1 R2: path B CFE_SB_Global_t.AppId -> CFE_EVS_SendEventWithAppID#2 (evaluation lookup)')
b = 'r2_core'
L = REL[b]
rows = rows_where(b, 'B', r'CFE_SB_Global_t\.AppId -> CFE_EVS_SendEventWithAppID#2 ')
for (k, rid, s, e) in rows:
    say('  row %s at %s: %s' % (rid, ref(b, s), L[s].strip()))
    sites = find_in(L, s, e, r'^      site ')
    files = collections.Counter(re.search(r'site (\S+?):', l).group(1).split('/')[-1] for _, l in sites)
    say('    sites %d: %s; results %s' % (len(sites), dict(files), dict(collections.Counter(re.search(r'result=(\S+)', l).group(1) for _, l in sites))))
    show(b, find_in(L, s, e, r'^      field: '), 'field')
    show(b, find_in(L, s, e, r'^        (memset|out-pointer|assign) '), 'writer')
    show(b, find_in(L, s, e, r'callee-check: .*ILLEGAL_APP_ID'), 'callee check (illegal id)')
    show(b, find_in(L, s, e, r'callee-check \(called with the parameter\): EVS_GetAppDataByID'), 'callee check one level down')
    show(b, find_in(L, s, e, r'^      handling: '), 'handling')
    show(b, find_in(L, s, e, r'^      actors: '), 'actors', 600)
    show(b, find_in(L, s, e, r'^      feasibility: '), 'feasibility', 900)
    show(b, find_in(L, s, e, r'^      (flip|violation|run-order|layers): '), 'column')
if not rows:
    say('  MISSING the B row')
# the targets: the field and the await
F = FACTS[b]
show_f = lambda pat: [(i, F[i]) for i in range(len(F)) if re.search(pat, F[i])]
for i, l in show_f(r'^  target\.(CFE_SB_Global_t\.AppId|CFE_ES_AppRecord_t\.AppState) ')[:4]:
    say('    facts.txt:%d %s' % (i + 1, l.strip()))
for i, l in show_f(r'in CFE_ES_MainTaskSyncDelay.* relevant=')[:2]:
    say('    facts.txt:%d await row: %s' % (i + 1, l.strip()[:300]))
# the C1 analogue of the T1 relation (not a test condition): its actors and its feasibility text
for b in ('c1_pre_tx',):
    L = REL[b]
    for (k, rid, s, e) in rows_where(b, 'B', r'CFE_SB_Global_t\.AppId -> CFE_EVS_SendEventWithAppID#2 '):
        say('  C1 analogue row %s at %s: %s' % (rid, ref(b, s), L[s].strip()))
        show(b, find_in(L, s, e, r'^      actors: '), 'actors', 900)
        show(b, find_in(L, s, e, r'^      feasibility: '), 'feasibility', 900)
    n_img = sum(1 for (k, rid, s, e) in BLK[b] if k == 'B' and find_in(L, s, e, r'^      feasibility: .*no static order derived: the spawn .* starts writer actor'))
    say('  %s B rows whose feasibility names roots resolved from a static image (no order derived): %d' % (b, n_img))

# ---------------------------------------------------------------- T2
say('')
say('# T2 C1: path A, the COUNTSEM partition of the OSAL name table (evaluation lookup)')
for b in ('c1_pre_tx', 'c1_post_tx'):
    L = REL[b]
    say('## %s' % b)
    ns = [x for x in BLK[b] if x[0] == 'NS' and x[1] == 'OS_common_record_t.name_entry']
    for (k, rid, s, e) in ns:
        say('  namespace block at %s' % ref(b, s))
        show(b, find_in(L, s, e, r'search: '), 'search')
        show(b, find_in(L, s, e, r'active fields: '), 'active fields')
        hits = find_in(L, s, e, r'publication writes of the creator OS_CountSemCreate#1')
        show(b, [(i, l[:120] + ' ... ' + '; '.join(x for x in re.findall(r'[^;]*(?:idmap\.c:(?:639|640|1079)|countsem\.c:100)[^;]*', l))) for i, l in hits], 'publication writes (L639/L640, L100, L1079)', 900)
    for pat in (r'cf_cfdp\.c:(993|1003):\d+ in CF_CFDP_InitEngine -> OS_CountSemGetIdByName', r'bp_flow\.c:126:\d+ in initialize_throttling.* -> OS_CountSemGetIdByName'):
        for (k, rid, s, e) in rows_where(b, 'A', pat):
            say('  row %s at %s: %s' % (rid, ref(b, s), L[s].strip()[:300]))
            show(b, find_in(L, s, e, r'^      (partition|key): '), 'partition/key', 700)
            show(b, find_in(L, s, e, r'^      creator: .*bp_flow\.c:132'), 'creator BP :132 (same partition)', 300)
            show(b, find_in(L, s, e, r'not a pair: probe-create'), 'probe-create', 400)
            show(b, find_in(L, s, e, r'handling: probe-create: '), 'probe-create in the handling column', 400)
            show(b, find_in(L, s, e, r'^      dependency: '), 'dependency', 300)
            show(b, find_in(L, s, e, r'not a pair: key constants differ: .*sch_lab'), 'SCH_LAB not paired (key constants differ)', 300)
            show(b, find_in(L, s, e, r'creator: .*sch_lab'), 'SCH_LAB paired as possible (key unresolved)', 300)
            show(b, find_in(L, s, e, r'^      handling: '), 'handling', 700)
            for i, l in find_in(L, s, e, r'^      handling: terminal '):
                m = re.search(r'\(F8 .*', l)
                say('    %s F8 and its guards: %s' % (ref(b, i), m.group(0)[:1500] if m else '-'))
            show(b, find_in(L, s, e, r'^      (feasibility|flip|violation|layers): '), 'column', 400)

# ---------------------------------------------------------------- T3
say('')
say('# T3 C5: path C, CFE_ES_AppCreate spawn, TaskTable RecordUsed reads per read site (evaluation lookup)')
for b in ('c5_641', 'c5_642'):
    L = REL[b]
    say('## %s' % b)
    rows = rows_where(b, 'C', r'in CFE_ES_AppCreate -> OS_TaskCreate; field CFE_ES_TaskRecord_t\.RecordUsed; read .* in CFE_ES_GetAppID')
    first = True
    for (k, rid, s, e) in rows:
        if first:
            show(b, find_in(L, s, e, r'^      spawn: '), 'spawn (entries, creator identity, locks at the spawn)', 700)
            show(b, find_in(L, s, e, r'^      write after the spawn: '), 'registration write', 400)
            first = False
        feas = find_in(L, s, e, r'^      feasibility: ')
        locks = find_in(L, s, e, r'^      read: ')
        say('  row %s at %s: %s' % (rid, ref(b, s), re.sub(r'^.*; read ', 'read ', L[s].strip())))
        for i, l in locks:
            say('    %s read locks: %s' % (ref(b, i), re.sub(r'^.*? locks ', '', l.strip())[:300]))
        for i, l in feas:
            say('    %s %s' % (ref(b, i), l.strip()[:220]))
        for i, l in find_in(L, s, e, r'^      (flip|read subscript): '):
            say('    %s %s' % (ref(b, i), l.strip()[:400]))
    say('  C rows of this spawn and field read in GetAppID / GetAppIDInternal: %d' % len(rows))
    grades = collections.Counter()
    for (k, rid, s, e) in rows:
        f = find_in(L, s, e, r'^      feasibility: ')
        grades[f[0][1].split(':')[1].strip() if f else '-'] += 1
    say('  their feasibility grades: %s' % dict(grades))
    cct = rows_where(b, 'C', r'in CFE_ES_CreateChildTask -> OS_TaskCreate; field CFE_ES_TaskRecord_t\.RecordUsed; read .* in CFE_ES_GetAppID')
    for (k, rid, s, e) in cct:
        f = find_in(L, s, e, r'^      feasibility: ')
        say('  CreateChildTask row %s at %s: %s -> %s' % (rid, ref(b, s), re.sub(r'^.*; read ', 'read ', L[s].strip())[:160], f[0][1].strip()[:120] if f else '-'))
    allr = rows_where(b, 'C', r'in CFE_ES_AppCreate -> OS_TaskCreate; field CFE_ES_AppRecord_t\.RecordUsed; ')
    for (k, rid, s, e) in allr:
        feas = find_in(L, s, e, r'^      feasibility: ')
        say('  AppRecord row %s at %s: %s -> %s' % (rid, ref(b, s), re.sub(r'^.*; read ', 'read ', L[s].strip()), feas[0][1].strip()[:200] if feas else '-'))
        show(b, find_in(L, s, e, r'^      write after the spawn'), 'write after the spawn', 260)

# ---------------------------------------------------------------- T4
say('')
say('# T4 C5: lifecycle of the OSAL task table (namespace OS_task_record_t.id), apart from the C rows')
for b in ('c5_641', 'c5_642'):
    L = REL[b]
    say('## %s' % b)
    for (k, rid, s, e) in [x for x in BLK[b] if x[0] == 'NS' and x[1] in ('OS_task_record_t.id', 'OS_task_record_t.name')]:
        say('  namespace %s at %s' % (rid, ref(b, s)))
        show(b, find_in(L, s, e, r'search: '), 'search', 300)
        show(b, find_in(L, s, e, r'active fields: '), 'active fields', 300)
        show(b, find_in(L, s, e, r'active test: '), 'active test', 200)
        if rid == 'OS_task_record_t.id':
            show(b, find_in(L, s, e, r'P0 out-pointer: pthread_create'), 'P0 writer of id', 500)
            show(b, find_in(L, s, e, r'lifecycle OS_task_record_t\.id: '), 'writers of id', 400)
            show(b, find_in(L, s, e, r'entry path: '), 'entry path', 1200)
            show(b, find_in(L, s, e, r'namespace layers: '), 'namespace layers', 600)
            show(b, [(i, re.sub(r' guard=\(.*\)', ' guard=...', l)) for i, l in find_in(L, s, e, r'assign at .*osapi\.c:(279|551|572|652|667|679|697|761|793|766|798):')],
                 'write', 300)

# ---------------------------------------------------------------- T5
say('')
say('# T5 input layers of every row; the P0 entries')
t5 = []
for b in BUILDS:
    L = REL[b]
    lay = collections.Counter()
    bad = dev = rests = unv = 0
    unv_entries = collections.Counter()
    for (k, rid, s, e) in BLK[b]:
        if k == 'NS':
            continue
        ls = [l for _, l in find_in(L, s, e, r'^      layers: ')]
        if not ls:
            lay['(no layers line)'] += 1
            continue
        v = ls[0].split('layers: ', 1)[1]
        if ' basis-deviation(' in v:
            dev += 1
        if ' rests-on(' in v:
            rests += 1
        m = re.search(r' P0-unverified\(([^)]*)\)', v)
        if m:
            unv += 1
            for x in m.group(1).split(', '):
                unv_entries[x] += 1
        v = re.sub(r' (basis-deviation|rests-on|P0-unverified)\(.*', '', v)
        key = re.sub(r' \(writers: rule targets\)', '', v).strip()
        lay[key] += 1
        if 'case:' in v or 'hand-entered' in v:
            bad += 1
    # rows whose lines name a lock (a lock set that is not empty) and whose P0-unverified names no lock primitive
    LOCKP = ('pthread_mutex_lock', 'pthread_mutex_unlock', 'pthread_mutex_timedlock', 'pthread_mutex_trylock', 'pthread_cond_wait', 'pthread_cond_timedwait')
    named_lock = no_prim = 0
    for (k, rid, s, e) in BLK[b]:
        if k == 'NS':
            continue
        if not find_in(L, s, e, r'(locks \{#|at the call \{#|locks held \{#)'):
            continue
        named_lock += 1
        ls = [l for _, l in find_in(L, s, e, r'^      layers: ')]
        m = re.search(r' P0-unverified\(([^)]*)\)', ls[0]) if ls else None
        if not m or not any(p in m.group(1).split(', ') for p in LOCKP):
            no_prim += 1
    nsdev = sum(1 for l in L if l.startswith('      namespace layers: ') and 'basis-deviation(' in l)
    say('%s: rows %d, layers %s; rows with a case layer or a hand-entered vocabulary: %d; rows marked basis-deviation %d, rests-on %d, P0-unverified %d; '
        'namespace blocks marked basis-deviation %d' % (b, sum(lay.values()), dict(lay), bad, dev, rests, unv, nsdev))
    say('    P0-unverified entries named (rows): %s' % dict(sorted(unv_entries.items())))
    say('    rows whose lines name a lock: %d; of them without a lock primitive in P0-unverified: %d' % (named_lock, no_prim))
    F = FACTS[b]
    for i, l in enumerate(F):
        if l.startswith('  count.walk_bounds_hit = ') or l.startswith('  param.walk_bounds = '):
            say('    facts.txt:%d %s' % (i + 1, l.strip()[:300]))
    nview = sum(1 for l in F if 'elements not listed here (the text view lists them for at most' in l)
    say('    text-view element print limit applied: %d PTR_FIELDS rows' % nview)
    t5.append((b, sum(lay.values()), bad))
    hdr = [l for l in L[:12] if l.startswith('mode ') or l.startswith('  extract.') or l.startswith('p0-compare')]
    for l in hdr:
        say('    header: %s' % l[:300])
F = FACTS['r2_core']
inp0 = False
n_p0 = n_unv = n_ver = n_tag = 0
for l in F:
    if l.startswith('[p0]'):
        inp0 = True
        continue
    if inp0 and l.startswith('['):
        break
    if inp0 and l.startswith('  ') and not l.startswith('    '):
        n_p0 += 1
    if inp0 and l.startswith('      version '):
        n_tag += 1
        if l.rstrip().endswith('source unverified'):
            n_unv += 1
        else:
            n_ver += 1
say('P0 table rows in r2_core facts: %d (version tag %d; source line %d, source unverified %d)' % (n_p0, n_tag, n_ver, n_unv))
for b in BUILDS:
    E = load(b, 'extract.txt')
    say('%s extract: %s' % (b, ' | '.join(l.strip() for l in E if 'hand-entered inputs' in l or 'rule targets' in l)))

# ---------------------------------------------------------------- T6
say('')
say('# T6 normal-caller controls')
for b in ('c5_641', 'c5_642'):
    L = REL[b]
    for obj in ('task', 'queue', 'bin_sem', 'count_sem', 'mut_sem', 'timer'):
        nsb = [x for x in BLK[b] if x[0] == 'NS' and x[1] == 'OS_%s_record_t.name' % obj]
        if not nsb:
            say('  %s %s: MISSING namespace' % (b, obj))
            continue
        k, rid, s, e = nsb[0]
        kp = find_in(L, s, e, r'key-parameter functions: ')
        say('  %s %s at %s: %s' % (b, rid, ref(b, s), kp[0][1].strip() if kp else 'MISSING key-parameter line'))
for b in ('c1_pre_tx', 'c1_post_tx', 'r2_core'):
    L = REL[b]
    a2 = [(i, l) for i, l in enumerate(L) if 'search: A2 callback search' in l and 'OS_ObjectIdFindNextMatch' in l]
    keys = sorted({re.search(r'key (\S+) ', l).group(1) for _, l in a2})
    say('  %s: A2 callback searches through OS_ObjectIdFindNextMatch: %d, keys %s' % (b, len(a2), keys))
    for i, l in a2:
        say('    %s %s' % (ref(b, i), re.sub(r'^.*: key ', 'key ', l.strip())[:200]))
viol = collections.Counter()
for b in BUILDS:
    for l in REL[b]:
        if l.startswith('      violation: '):
            viol[l.split(':', 2)[1].strip()] += 1
say('  violation column over all rows of all builds: %s' % dict(viol))

# ---------------------------------------------------------------- T7
say('')
say('# T7 determinism')
tw = os.path.join(OUT, 'twice.txt')
if os.path.exists(tw):
    for l in open(tw):
        say('  ' + l.rstrip())

# ---------------------------------------------------------------- the candidate burden
say('')
say('# candidate burden: rows per build and the case relations among them (evaluation lookup)')
case_rows = {
    # the namespace that carries the pairing (OS_common_record_t.name_entry); the same lookups in the callback
    # namespace OS_filesys_internal_record_t.virtual_mountpt are by-products of the field-based may indirect call
    'c1_pre_tx': [('A', r'cf_cfdp\.c:993:\d+ in CF_CFDP_InitEngine -> OS_CountSemGetIdByName#1 namespace OS_common_record_t\.name_entry '),
                  ('A', r'bp_flow\.c:126:\d+ in initialize_throttling.* namespace OS_common_record_t\.name_entry ')],
    'c1_post_tx': [('A', r'cf_cfdp\.c:1003:\d+ in CF_CFDP_InitEngine -> OS_CountSemGetIdByName#1 namespace OS_common_record_t\.name_entry '),
                   ('A', r'bp_flow\.c:126:\d+ in initialize_throttling.* namespace OS_common_record_t\.name_entry ')],
    'r2_core': [('B', r'CFE_SB_Global_t\.AppId -> CFE_EVS_SendEventWithAppID#2 ')],
    'c5_641': [('C', r'in CFE_ES_AppCreate -> OS_TaskCreate; field CFE_ES_TaskRecord_t\.RecordUsed; read .* in CFE_ES_GetAppID')],
    'c5_642': [('C', r'in CFE_ES_AppCreate -> OS_TaskCreate; field CFE_ES_TaskRecord_t\.RecordUsed; read .* in CFE_ES_GetAppID')],
}
byprod = {
    'c1_pre_tx': [('A', r'(cf_cfdp\.c:993:|bp_flow\.c:126:).* namespace OS_filesys_internal_record_t\.virtual_mountpt ')],
    'c1_post_tx': [('A', r'(cf_cfdp\.c:1003:|bp_flow\.c:126:).* namespace OS_filesys_internal_record_t\.virtual_mountpt ')],
}
for b, pats in byprod.items():
    for k, p in pats:
        for (kk, rid, s, e) in rows_where(b, k, p):
            dep = find_in(REL[b], s, e, r'^      dependency: ')
            say('  %s by-product row %s at %s: %s | %s' % (b, rid, ref(b, s), REL[b][s].strip()[:200], dep[0][1].strip() if dep else '-'))
with open(os.path.join(CD, 'burden.tsv'), 'w') as f:
    f.write('build\tA\tB\tC\ttotal\tnamespaces\tcase_rows\tcase_rows_graded\tflip_static_possible\tfeasibility_proven_impossible\tpossible\tundecidable\tnot_graded\n')
    for b in BUILDS:
        L = REL[b]
        n = collections.Counter(k for (k, rid, s, e) in BLK[b])
        crs = [x for k, p in case_rows[b] for x in rows_where(b, k, p)]
        cr = len(crs)
        # the case rows that carry a grade (a row `not graded` is no read of the spawn's threads under this revision)
        crg = sum(1 for (k, rid, s, e) in crs if not find_in(L, s, e, r'^      feasibility: not graded'))
        tot = n['A'] + n['B'] + n['C']
        fp = sum(1 for l in L if l.startswith('      flip: static-possible'))
        pi = sum(1 for l in L if l.startswith('      feasibility: proven-impossible'))
        po = sum(1 for l in L if l.startswith('      feasibility: possible'))
        ud = sum(1 for l in L if l.startswith('      feasibility: undecidable'))
        ng = sum(1 for l in L if l.startswith('      feasibility: not graded'))
        say('%s: A %d, B %d, C %d, total %d (namespace blocks %d); case relation rows %d (%.1f%% of the rows), of them graded %d (%.1f%%); flip static-possible %d; '
            'feasibility proven-impossible %d, possible %d, undecidable %d, not graded %d' % (
                b, n['A'], n['B'], n['C'], tot, n['NS'], cr, 100.0 * cr / tot if tot else 0, crg, 100.0 * crg / tot if tot else 0, fp, pi, po, ud, ng))
        f.write('%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n' % (b, n['A'], n['B'], n['C'], tot, n['NS'], cr, crg, fp, pi, po, ud, ng))

# ---------------------------------------------------------------- ED §2.3 under the rule targets
say('')
say('# ED §2.3 again: the writer sets of the rule targets (R-T1 ∪ R-T2, shared bases)')
CASE_FIELDS = {
    'c1_pre_tx': ['OS_common_record_t.name_entry', 'OS_common_record_t.active_id', 'BP_Throttle_t.Sem', 'BP_Throttle_t.Name', 'CF_Channel_t.sem_id', 'CF_ChannelConfig_t.sem_name'],
    'r2_core': ['CFE_SB_Global_t.AppId', 'EVS_AppData_t.AppID', 'CFE_ES_AppRecord_t.AppId', 'CFE_ES_AppRecord_t.AppState', 'CFE_ES_AppRecord_t.Type',
                'CFE_PSP_IdleTaskState_t.ShutdownReq'],
    'c5_641': ['CFE_ES_TaskRecord_t.RecordUsed', 'CFE_ES_TaskRecord_t.TaskId', 'CFE_ES_TaskRecord_t.AppId', 'OS_task_record_t.id', 'OS_task_record_t.free'],
}
CASE_FIELDS['c1_post_tx'] = CASE_FIELDS['c1_pre_tx']
CASE_FIELDS['c5_642'] = CASE_FIELDS['c5_641']
LOC = re.compile(r'^  (\S+) (writer-set-complete|DISABLED|unchecked) writers=(\d+) .*atype: untyped=(\d+) escape=(\d+) cast=(\d+) union=(\d+) unknown-effect=(\d+)')
FS = re.compile(r'^  (\S+) per-field=(\S+) type-level=(\S+) writers=(\d+)')
TG = re.compile(r'^  target\.(\S+) = rule (R-T1,R-T2|R-T1|R-T2) ')
with open(os.path.join(CD, 'targets_rule.tsv'), 'w') as f:
    f.write('build\ttargets\tR-T1\tR-T2\tboth\tcomplete\tDISABLED\tw0\tw1\tw2_5\tw_gt5\tuntyped\tescape\tcast\tunion\tunknown_effect\tfield_rule_complete\ttype_rule_complete\n')
    for b in BUILDS:
        F = FACTS[b]
        sec = None
        locs, fst, tg = {}, {}, {}
        for i, l in enumerate(F):
            if l.startswith('['):
                sec = l.split(']')[0][1:]
                continue
            if sec == 'locs':
                m = LOC.match(l)
                if m:
                    locs[m.group(1)] = m.groups()[1:] + (i,)
            elif sec == 'fstatus':
                m = FS.match(l)
                if m:
                    fst[m.group(1)] = m.groups()[1:] + (i,)
            elif sec == 'basis':
                m = TG.match(l)
                if m:
                    tg[m.group(1)] = (m.group(2), i)
        st = collections.Counter(v[0] for v in locs.values())
        w = [int(v[1]) for v in locs.values()]
        bucket = (sum(1 for x in w if x == 0), sum(1 for x in w if x == 1), sum(1 for x in w if 2 <= x <= 5), sum(1 for x in w if x > 5))
        dis = [v for v in locs.values() if v[0] == 'DISABLED']
        causes = tuple(sum(1 for v in dis if int(v[k]) > 0) for k in range(2, 7))
        r1 = sum(1 for v in tg.values() if 'R-T1' in v[0])
        r2 = sum(1 for v in tg.values() if 'R-T2' in v[0])
        both = sum(1 for v in tg.values() if v[0] == 'R-T1,R-T2')
        fc = sum(1 for k in locs if k in fst and fst[k][0] == 'writer-set-complete')
        tc = sum(1 for k in locs if k in fst and fst[k][1] == 'writer-set-complete')
        say('%s: targets %d (R-T1 %d, R-T2 %d, both %d); FACT_LOCS complete %d / DISABLED %d; writers 0/1/2-5/>5 %d/%d/%d/%d; DISABLED causes untyped/escape/cast/union/unknown %d/%d/%d/%d/%d; '
            'FIELD_STATUS of the same fields: per-field complete %d, type-level complete %d' % (
                b, len(locs), r1, r2, both, st['writer-set-complete'], st['DISABLED'], *bucket, *causes, fc, tc))
        f.write('%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n' % (b, len(locs), r1, r2, both, st['writer-set-complete'], st['DISABLED'], *bucket, *causes, fc, tc))
        for cf in CASE_FIELDS[b]:
            t = tg.get(cf)
            lo = locs.get(cf)
            fs = fst.get(cf)
            say('    %-40s target %-14s FACT_LOCS %-40s FIELD_STATUS %s' % (
                cf, ('%s (facts.txt:%d)' % (t[0], t[1] + 1)) if t else 'no',
                ('%s writers %s (facts.txt:%d)' % (lo[0], lo[1], lo[7] + 1)) if lo else '-',
                ('per-field %s type-level %s writers %s (facts.txt:%d)' % (fs[0], fs[1], fs[2], fs[3] + 1)) if fs else '-'))
        E = load(b, 'extract.txt')
        say('    ' + ' | '.join(l.strip() for l in E if 'rule targets' in l))

# ---------------------------------------------------------------- the fix stage against S5
def heads(lines):
    out = collections.Counter()
    grade = {}
    for (k, rid, s, e) in blocks(lines):
        if k == 'NS':
            continue
        h = re.sub(r'^  [ABC]\d+ ', k + ' ', lines[s]).strip()
        # the reach phrase of a C context changed wording between revisions (3: "the spawned entries", 4: "the threads this
        # spawn starts"): the head without it, so that a context is compared with itself
        h = re.sub(r'; (the spawned entries|the threads this spawn starts) do not reach this call', '', h)
        f = find_in(lines, s, e, r'^      feasibility: ')
        fl = find_in(lines, s, e, r'^      flip: ')
        g = (f[0][1].split(':')[1].strip() if f else '-', fl[0][1].split(':')[1].strip() if fl else '-')
        out[h] += 1
        grade.setdefault(h, collections.Counter())[g] += 1
    return out, grade


def against(root, label, tsv):
    say('')
    say('# the rows against %s (row heads without their number; the feasibility and flip grades)' % label)
    with open(os.path.join(CD, tsv), 'w') as f:
        f.write('build\tpath\trows_old\trows_new\tonly_old\tonly_new\tgrade_changed\n')
        for b in BUILDS:
            old = load(b, 'relations.txt', root)
            o, og = heads(old)
            n, ng = heads(REL[b])
            for k in 'ABC':
                ok = {h: c for h, c in o.items() if h.startswith(k + ' ')}
                nk = {h: c for h, c in n.items() if h.startswith(k + ' ')}
                only_o = sum(c for h, c in ok.items() if h not in nk)
                only_n = sum(c for h, c in nk.items() if h not in ok)
                changed = [h for h in ok if h in nk and og[h] != ng[h]]
                say('%s %s: rows %s %d, now %d; only in %s %d, only now %d; same head, other grades %d' % (b, k, label, sum(ok.values()), sum(nk.values()), label, only_o, only_n, len(changed)))
                f.write('%s\t%s\t%d\t%d\t%d\t%d\t%d\n' % (b, k, sum(ok.values()), sum(nk.values()), only_o, only_n, len(changed)))
                for h in changed[:60]:
                    say('    changed: %s | %s %s -> now %s' % (h[:230], label, dict(og[h]), dict(ng[h])))
                for h in [h for h in nk if h not in ok][:20]:
                    say('    new: %s' % h[:230])
                for h in [h for h in ok if h not in nk][:20]:
                    say('    gone: %s' % h[:230])
            for g in ('proven-impossible', 'possible', 'undecidable', 'not graded'):
                go = sum(1 for l in old if l.startswith('      feasibility: ' + g))
                gn = sum(1 for l in REL[b] if l.startswith('      feasibility: ' + g))
                say('%s %s: %s %d, now %d' % (b, g, label, go, gn))


if S5:
    against(S5, 'S5', 'against_s5.tsv')
if FIX1:
    against(FIX1, 'fix1', 'against_fix1.tsv')
if FIX2:
    against(FIX2, 'fix2', 'against_fix2.tsv')

with open(os.path.join(CD, 'summary.tsv'), 'w') as f:
    f.write('build\trows\trows_with_case_layer\n')
    for b, n, bad in t5:
        f.write('%s\t%d\t%d\n' % (b, n, bad))

#!/usr/bin/env python3
# step2a_s4_count.py KZ-DUMP KZ-STATIC OUT EB S3 COUNT-DIR -- the counts of redesign step 2a stage S4 (step2a_s4.sh count).
#
# Reads the text views the driver wrote (OUT/<build>/NP/{lockops,lockids,lockset,fstatus,awaits}.txt and facts.txt) and
# writes into COUNT-DIR:
#   counts.tsv     per build: the rows of LOCK_OPS by kind and mode, LOCK_IDS by status, LOCKSET (holding a lock, on a
#                  failure path, not reached, no control-flow graph), FIELD_STATUS under the two rules, AWAIT_L1 under the
#                  two rules, the completeness-pass targets, the extract time
#   c5_t3.txt      the lock sets ED §6 T3 names: 6.4.1 CFE_ES_AppCreate (the spawn, the lock and unlock calls, the
#                  registration writes), 6.4.2 the reads of RecordUsed and the calls of CFE_ES_GetAppIDInternal, and the
#                  6.4.2 CFE_ES_AppCreate rows; the LOCK_IDS row of the lock they hold
#   r2_rules.txt   R2: FIELD_STATUS of CFE_SB_Global_t.AppId and the CFE_ES_AppRecord_t fields, the old FACT_LOCS status
#                  of the same fields in the runs with targets (V2, V3, V3a), the CFE_ES_MainTaskSyncDelay loop under the
#                  old rule (FACT_LOOPS) and the new one (AWAIT_L1) in every run
#   lockflags.tsv  the lock and unlock flags of the calls (FACTS) in the profile-free run against V1 (the G0 names) and VP
#                  (the P0 names only) of stage S1, and the L1 (ii) flag of the loops
# Case and framework names appear only in the queries of this script, never in the tool.
import json
import os
import re
import subprocess
import sys

KZD, KS, OUT, EB, S3, CNT = sys.argv[1:7]
BUILDS = ['c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641', 'c5_642']
S1 = os.path.join(os.path.dirname(S3), 's1')
os.makedirs(CNT, exist_ok=True)


def read(path):
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            return f.read().splitlines()
    except FileNotFoundError:
        return []


def section(lines, name):
    out, on = [], False
    for ln in lines:
        if ln.startswith('['):
            on = ln.startswith('[' + name + ']')
            continue
        if on:
            out.append(ln)
    return out


def basis_item(lines, name):
    for ln in section(lines, 'basis'):
        m = re.match(r'\s+' + re.escape(name) + r' = (.*)', ln)
        if m:
            return m.group(1)
    return ''


def nosite(s):
    return re.sub(r'site [0-9]+ ', '', s)


# ---- counts per build
rows = []
times = {}
for ln in read(os.path.join(OUT, 'run.log')):
    m = re.match(r'(\S+)/(\S+)/NP basis=\d+ extract=(\d+) show=\d+ basis_s=([\d.]+) extract_s=([\d.]+)', ln)
    if m:
        times[os.path.basename(m.group(1)) if '/' not in m.group(2) else m.group(2)] = (m.group(3), m.group(5))
for ln in read(os.path.join(OUT, 'run.log')):
    for b in BUILDS:
        if '/' + b + '/NP ' in ln:
            m = re.search(r'extract=(\d+) .*extract_s=([\d.]+)', ln)
            if m:
                times[b] = (m.group(1), m.group(2))
header = ['build', 'lockops', 'acquire_every', 'acquire_result', 'acquire_try', 'acquire_conditional', 'release_every', 'release_result', 'release_conditional', 'wait',
          'vocabulary_ops', 'lockids', 'ids_static', 'ids_not_static', 'ids_param', 'ids_undecidable', 'lockset', 'lockset_held', 'lockset_failure', 'lockset_not_reached',
          'lockset_no_cfg', 'fstatus', 'field_complete', 'type_complete', 'once', 'awaits', 'await_field', 'await_type', 'l1ii_one_lock', 'l1ii_atomic', 'l1i_field_holds',
          'l1i_type_holds', 'completeness_targets', 'extract_s']
for b in BUILDS:
    d = os.path.join(OUT, b, 'NP')
    ops = [l for l in read(os.path.join(d, 'lockops.txt')) if re.match(r'  \S', l)]
    ids = [l for l in read(os.path.join(d, 'lockids.txt')) if l.startswith('  #')]
    ls = [l for l in read(os.path.join(d, 'lockset.txt')) if re.match(r'  (call|read|write) ', l)]
    fs = [l for l in read(os.path.join(d, 'fstatus.txt')) if re.match(r'  \S', l)]
    aw = [l for l in read(os.path.join(d, 'awaits.txt')) if l.startswith('  loop ')]
    facts = read(os.path.join(d, 'facts.txt'))

    def opc(kind, mode=None):
        return sum(1 for l in ops if l.split()[1] == kind and (mode is None or (len(l.split()) > 2 and l.split()[2] == mode)))
    r = {
        'build': b, 'lockops': len(ops), 'acquire_every': opc('acquire', 'every-return'), 'acquire_result': opc('acquire', 'result-conditional'),
        'acquire_try': opc('acquire', 'try'), 'acquire_conditional': opc('acquire', 'conditional'), 'release_every': opc('release', 'every-return'),
        'release_result': opc('release', 'result-conditional'), 'release_conditional': opc('release', 'conditional'), 'wait': opc('wait'),
        'vocabulary_ops': sum(1 for l in ops if 'vocabulary' in l.split('{')[-1] if '{' in l),
        'lockids': len(ids), 'ids_static': sum(' static-instance ' in l for l in ids), 'ids_not_static': sum(' not-static ' in l for l in ids),
        'ids_param': sum(' parameter-relative parameter-relative ' in l for l in ids), 'ids_undecidable': sum(' undecidable ' in l for l in ids),
        'lockset': len(ls), 'lockset_held': sum(' held=0 ' not in l for l in ls), 'lockset_failure': sum('failure-path' in l for l in ls),
        'lockset_not_reached': sum('not-reached' in l for l in ls), 'lockset_no_cfg': sum('no-cfg' in l for l in ls),
        'fstatus': len(fs), 'field_complete': sum('per-field=writer-set-complete' in l for l in fs), 'type_complete': sum('type-level=writer-set-complete' in l for l in fs),
        'once': sum(' one-instance-once ' in l for l in fs), 'awaits': len(aw),
        'await_field': 0, 'await_type': 0,
        'l1ii_one_lock': sum('L1(ii)=one-lock-region' in l for l in aw), 'l1ii_atomic': sum('L1(ii)=atomic-read' in l for l in aw),
        'l1i_field_holds': sum('L1(i) per-field=holds' in l for l in aw), 'l1i_type_holds': sum('type-level=holds' in l for l in aw),
        'completeness_targets': re.sub(r'.*targets of the completeness pass (\d+).*', r'\1', basis_item(facts, 'count.field_status')),
        'extract_s': times.get(b, ('', ''))[1],
    }
    al = read(os.path.join(d, 'awaits.txt'))
    r['await_field'] = sum(1 for l in al if l.strip().startswith('per-field: await'))
    r['await_type'] = sum(1 for l in al if l.strip().startswith('type-level: await'))
    rows.append(r)
with open(os.path.join(CNT, 'counts.tsv'), 'w') as f:
    f.write('\t'.join(header) + '\n')
    for r in rows:
        f.write('\t'.join(str(r[h]) for h in header) + '\n')

# ---- C5 T3: the lock sets
def lockset_rows(b, file_part, lines_wanted=None, fn=None):
    out = []
    for l in read(os.path.join(OUT, b, 'NP', 'lockset.txt')):
        m = re.search(r'(\S*' + re.escape(file_part) + r'):(\d+):(\d+) in (\S+) held=', l)
        if not m:
            continue
        if lines_wanted is not None and int(m.group(2)) not in lines_wanted:
            continue
        if fn is not None and not m.group(4).startswith(fn):
            continue
        out.append(l)
    return out


def lockid(b, idx):
    for l in read(os.path.join(OUT, b, 'NP', 'lockids.txt')):
        if l.startswith('  #%s ' % idx):
            return l.strip()
    return ''


with open(os.path.join(CNT, 'c5_t3.txt'), 'w') as f:
    f.write('# ED §6 T3: the lock sets of the profile-free run (LOCKSET rows; held relative to the entry of the function)\n')
    f.write('\n## 6.4.1 CFE_ES_AppCreate (cfe_es_apps.c): the lock calls L571, L610, L637, the spawn L614, the registration L645-L667\n')
    for l in lockset_rows('c5_641', 'cfe_es_apps.c', set(range(560, 670)), 'CFE_ES_AppCreate'):
        if re.search(r':(571|610|614|637|645|649|650|651|652|653|662|663|667):', l):
            f.write(l + '\n')
    f.write('\n## 6.4.2 CFE_ES_AppCreate (cfe_es_apps.c): the lock call L571, the spawn L619, the registration L646-L669\n')
    for l in lockset_rows('c5_642', 'cfe_es_apps.c', set(range(560, 680)), 'CFE_ES_AppCreate'):
        if re.search(r':(571|619|633|646|652|654|655|669):', l):
            f.write(l + '\n')
    f.write('\n## 6.4.2 reads of RecordUsed: CFE_ES_GetAppID L800 (lock L790, unlock L806), CFE_ES_GetAppIDInternal L1825\n')
    for l in lockset_rows('c5_642', 'cfe_es_api.c', {790, 800, 806}, 'CFE_ES_GetAppID'):
        f.write(l + '\n')
    for l in lockset_rows('c5_642', 'cfe_es_api.c', {1825}, 'CFE_ES_GetAppIDInternal'):
        f.write(l + '\n')
    f.write('\n## 6.4.2 calls of CFE_ES_GetAppIDInternal (the lock set at each call; L1858 and L1892 are the error branches of the lock wrappers)\n')
    for l in read(os.path.join(OUT, 'c5_642', 'NP', 'lockset.txt')):
        m = re.search(r'cfe_es_api.c:(445|551|667|1020|1284|1858|1892):\d+ in ', l)
        if m and l.startswith('  call '):
            f.write(l + '\n')
    for b in ('c5_641', 'c5_642'):
        f.write('\n## %s: the lock identity those rows hold\n' % b)
        for l in read(os.path.join(OUT, b, 'NP', 'lockids.txt')):
            if 'SharedDataMutex' in l:
                f.write(l + '\n')
        f.write('\n## %s: LOCK_OPS of the ES wrappers\n' % b)
        lines = read(os.path.join(OUT, b, 'NP', 'lockops.txt'))
        for i, l in enumerate(lines):
            if re.match(r'  (CFE_ES_LockSharedData|CFE_ES_UnlockSharedData|OS_MutSemTake|OS_MutSemGive) ', l):
                f.write(l + '\n' + (lines[i + 1] + '\n' if i + 1 < len(lines) else ''))

# ---- R2: the rules
with open(os.path.join(CNT, 'r2_rules.txt'), 'w') as f:
    fields = ['CFE_SB_Global_t.AppId', 'CFE_ES_AppRecord_t.AppState', 'CFE_ES_AppRecord_t.AppId']
    f.write('# R2 core: FIELD_STATUS (new, the same in every run) and FACT_LOCS (old, the run\'s own targets)\n')
    for v in ['NP', 'V2', 'V3', 'V3a']:
        base = os.path.join(OUT, 'r2_core', 'NP') if v == 'NP' else os.path.join(OUT, 'targets', 'r2_core', v)
        fsl = read(os.path.join(base, 'fstatus.txt'))
        f.write('\n## %s FIELD_STATUS\n' % v)
        for l in fsl:
            for fd in fields:
                if l.startswith('  ' + fd + ' '):
                    f.write(l + '\n')
        if v != 'NP':
            f.write('## %s FACT_LOCS (old rule)\n' % v)
            for l in read(os.path.join(base, 'locs.txt')):
                for fd in fields:
                    if ' ' + fd + ' ' in l or l.strip().startswith(fd + ' '):
                        f.write(l + '\n')
    f.write('\n# R2 core: the CFE_ES_MainTaskSyncDelay loop (cfe_es_start.c:871)\n')
    for v in ['NP', 'V2', 'V3', 'V3a']:
        base = os.path.join(OUT, 'r2_core', 'NP') if v == 'NP' else os.path.join(OUT, 'targets', 'r2_core', v)
        al = read(os.path.join(base, 'awaits.txt'))
        f.write('\n## %s AWAIT_L1 (new)\n' % v)
        for i, l in enumerate(al):
            if 'CFE_ES_MainTaskSyncDelay' in l and l.startswith('  loop '):
                f.write('\n'.join(al[i:i + 3]) + '\n')
        loops = read(os.path.join(base, 'loops.txt')) if v != 'NP' else section(read(os.path.join(base, 'facts.txt')), 'loops')
        f.write('## %s FACT_LOOPS (old)\n' % v)
        for i, l in enumerate(loops):
            if 'cfe_es_start.c:871:5 in CFE_ES_MainTaskSyncDelay' in l:
                for x in loops[i + 1:i + 10]:
                    if x.strip().startswith('rule '):
                        f.write(x + '\n')
                        break
    # the old rule in the step-2 runs (no extended facts), for the record
    f.write('\n# step 2 (extract_baseline) FACT_LOOPS rule of the same loop\n')
    for v in ['V1', 'V2', 'V3', 'V3a']:
        loops = section(read(os.path.join(EB, 'r2_core', v, 'facts.txt')), 'loops')
        for i, l in enumerate(loops):
            if 'cfe_es_start.c:871:5 in CFE_ES_MainTaskSyncDelay' in l:
                for x in loops[i + 1:i + 10]:
                    if x.strip().startswith('rule '):
                        f.write('%s %s\n' % (v, x.strip()))
                        break

# ---- the lock flags of the calls against V1 and VP, and the L1 (ii) flag of the loops
def flagged(lines):
    out = {}
    for l in section(lines, 'calls'):
        m = re.match(r'\s+site \d+ (\S+) (\S+) -> (\S+) \S+ \{([^}]*)\}', l)
        if not m:
            continue
        fl = set(m.group(4).split(','))
        k = None
        if 'lock-op' in fl:
            k = 'lock'
        if 'unlock-op' in fl:
            k = 'unlock'
        if k:
            out[(m.group(1), m.group(2).split('@')[0], m.group(3).split('@')[0])] = k
    return out


def l1ii(lines):
    out = set()
    loops = section(lines, 'loops')
    cur = None
    for l in loops:
        m = re.match(r'\s+loop \d+ site \d+ (\S+) in (\S+)', l)
        if m:
            cur = (m.group(1), m.group(2).split('@')[0])
            continue
        if cur and l.strip().startswith('rule ') and 'L1-ii' in l.split(':')[0]:
            out.add(cur)
    return out


with open(os.path.join(CNT, 'lockflags.tsv'), 'w') as f:
    f.write('build\tNP_S4_flagged\tNP_S3_flagged\tV1_flagged\tVP_flagged\tV1_in_NP_S4\tV1_in_NP_S3\tNP_S4_not_V1\tL1ii_V1\tL1ii_NP_S4\tL1ii_NP_S3\tL1ii_V1_in_NP_S4\n')
    for b in BUILDS:
        s4 = read(os.path.join(OUT, b, 'NP', 'facts.txt'))
        s3 = read(os.path.join(S3, b, 'NP', 'facts.txt'))
        v1 = read(os.path.join(S1, b, 'V1', 'facts.txt')) or read(os.path.join(EB, b, 'V1', 'facts.txt'))
        vp = read(os.path.join(S1, b, 'VP', 'facts.txt')) or read(os.path.join(EB, b, 'VP', 'facts.txt'))
        f4, f3, f1, fp = flagged(s4), flagged(s3), flagged(v1), flagged(vp)
        in4 = sum(1 for k, v in f1.items() if f4.get(k) == v)
        in3 = sum(1 for k, v in f1.items() if f3.get(k) == v)
        extra = sum(1 for k in f4 if k not in f1)
        a1, a4, a3 = l1ii(v1), l1ii(s4), l1ii(s3)
        f.write('\t'.join(str(x) for x in [b, len(f4), len(f3), len(f1), len(fp), in4, in3, extra, len(a1), len(a4), len(a3), len(a1 & a4)]) + '\n')
    # the V1 flags that the profile-free run does not have, per build (first few)
    f.write('\n# V1 flags the profile-free run lacks (callee: count)\n')
    for b in BUILDS:
        s4 = flagged(read(os.path.join(OUT, b, 'NP', 'facts.txt')))
        v1 = flagged(read(os.path.join(S1, b, 'V1', 'facts.txt')) or read(os.path.join(EB, b, 'V1', 'facts.txt')))
        miss = {}
        for k, v in v1.items():
            if s4.get(k) != v:
                miss[k[2] + ' (' + v + ')'] = miss.get(k[2] + ' (' + v + ')', 0) + 1
        f.write('%s\t%s\n' % (b, '; '.join('%s: %d' % kv for kv in sorted(miss.items())) or '-'))

# ---- the fields the case replay bound (V2 targets): the old status of the V2 run against the two new rules
with open(os.path.join(CNT, 'case_replay.tsv'), 'w') as f:
    f.write('build\tfield\tV2_FACT_LOCS(old)\tV3_FACT_LOCS(old)\tper_field(new)\ttype_level(new)\twriters_V2\tinstances(new)\n')
    for b in ['c1_pre_tx', 'r2_core', 'c5_641']:
        def locs(path):
            out = {}
            for l in read(path):
                m = re.match(r'\s+location (\S+)\s+writers (\d+).*-> (\S+)', l)
                if m:
                    out[m.group(1)] = (m.group(3), m.group(2))
            return out
        v2 = locs(os.path.join(EB, b, 'V2', 'extract.txt'))
        v3 = locs(os.path.join(EB, b, 'V3', 'extract.txt'))
        new = {}
        for l in read(os.path.join(OUT, b, 'NP', 'fstatus.txt')):
            m = re.match(r'\s+(\S+) per-field=(\S+) type-level=(\S+) .* instances=(\d+)', l)
            if m:
                new[m.group(1)] = (m.group(2), m.group(3), m.group(4))
        for fd in sorted(v2):
            n = new.get(fd, ('no-row', 'no-row', '-'))
            f.write('\t'.join([b, fd, v2[fd][0], v3.get(fd, ('not-a-target', ''))[0], n[0], n[1], v2[fd][1], n[2]]) + '\n')

with open(os.path.join(CNT, 'lockflags.tsv'), 'a') as f:
    f.write('\n# loops with the L1-ii flag in V1, and whether the profile-free run (S4, S3) has it\n')
    for b in BUILDS:
        v1 = l1ii(read(os.path.join(S1, b, 'V1', 'facts.txt')) or read(os.path.join(EB, b, 'V1', 'facts.txt')))
        s4 = l1ii(read(os.path.join(OUT, b, 'NP', 'facts.txt')))
        s3 = l1ii(read(os.path.join(S3, b, 'NP', 'facts.txt')))
        for k in sorted(v1 | s4):
            f.write('%s\t%s %s\tV1=%s S4=%s S3=%s\n' % (b, k[0], k[1], k in v1, k in s4, k in s3))

# ---- summary
with open(os.path.join(CNT, 'summary.txt'), 'w') as f:
    f.write('# step2a_s4_count.py summary\n')
    for r in rows:
        f.write(json.dumps(r) + '\n')
    f.write('\n# targets step\n')
    for l in read(os.path.join(OUT, 'targets', 'targets.txt')):
        f.write(l + '\n')
    f.write('\n# earlier sections (S3 -> S4, profile-free)\n')
    for l in read(os.path.join(CNT, 'earlier.txt')):
        f.write(l + '\n')
    f.write('\n# determinism\n')
    for l in read(os.path.join(OUT, 'twice.txt')):
        f.write(l + '\n')
print('written to', CNT)

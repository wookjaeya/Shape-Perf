#!/usr/bin/env python3
"""step2a_s3_count.py -- counts of the extended facts F4 and F6 (ARGFLOW, PARAM_FLOW, PTR_FIELDS, GLOBAL_IMAGES) of the
profile-free runs of step2a_s3.sh, the relation counts and their sensitivity to the tool parameters of the composition,
and the spot checks.

  step2a_s3_count.py summarize KZ_DUMP FACTS.kz          -> JSON on stdout (no case names: counts of one container)
  step2a_s3_count.py report KZ_DUMP OUT COUNT_DIR         -> COUNT_DIR/{counts.tsv, bounds.tsv, summary.txt, spot_<build>.txt}

The relation counts are the starting sets a relation query over these facts would read (plan/extraction_design_20261030.md
§3.1-§3.2); they are defined on the facts alone:
  P-cmp   parameters whose PARAM_FLOW rows (any depth the bounds allow) include a comparison;
  P-lib   parameters whose PARAM_FLOW rows include an argument of a function without a body whose result is tested;
  shared  ARGFLOW rows whose source is a field, a global or an element read from a shared base (a global object, an element
          of a global array, the pointee of a pointer field);
  B-cand  shared arguments whose callee parameter (a resolved target of an indirect call included) is in P-cmp;
  A-cand  shared or string arguments whose callee parameter is in P-lib.
Case names (C1 BP and CF, R2 SB/EVS, C5 ES/OSAL) appear only in the queries of the spot checks (report).
"""
import json
import os
import re
import subprocess
import sys

BUILDS = ['c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641', 'c5_642']
FIELD = re.compile(r'(\w+)=("(?:[^"\\]|\\.)*"|\{[^}]*\}|\S+)')


def records(kzd, path, wanted):
    """kz-dump records of the wanted sections: {section: [dict]}."""
    out = {s: [] for s in wanted}
    cur = None
    proc = subprocess.Popen([kzd, path], stdout=subprocess.PIPE, text=True, errors='replace')
    for line in proc.stdout:
        if line.startswith('section '):
            name = line.split()[1]
            cur = name if name in wanted else None
            continue
        if cur is None or not line.startswith('  '):
            continue
        body = line.strip().split(' ', 1)[1] if ' ' in line.strip() else ''
        rec = {}
        for k, v in FIELD.findall(body):
            if v.startswith('"'):
                v = bytes(v[1:-1], 'utf-8').decode('unicode_escape', errors='replace') if '\\' in v else v[1:-1]
            rec[k] = v
        out[cur].append(rec)
    proc.wait()
    return out


def flags(v):
    return set(x for x in v.strip('{}').split(',') if x)


def summarize(kzd, path):
    secs = ['BASIS', 'FUNCS', 'FACTS', 'CALL_TARGETS', 'ARGFLOW', 'PARAM_FLOW', 'PTR_FIELDS', 'GLOBAL_IMAGES']
    r = records(kzd, path, set(secs))
    fname = {}
    for f in r['FUNCS']:
        n = f['name']
        if 'static' in flags(f.get('flags', '{}')):
            n += '@%s:%s' % (f['file'], f['line'])
        fname[f['fn']] = n
    callee = {c['site']: (c['callee_fn'], c['kind']) for c in r['FACTS']}
    targets = {}
    for t in r['CALL_TARGETS']:
        if t['kind'] == 'indirect-call' and t['fn'] != 'none':
            targets.setdefault(t['site'], []).append(t['fn'])
    s = {'basis': {}, 'argflow': {}, 'paramflow': {}, 'ptrfields': {}, 'gimages': {}, 'relations': {}, 'nodes': {}}
    for b in r['BASIS']:
        if b['name'].startswith(('param.argflow', 'param.image', 'count.argflow', 'count.param_flow', 'count.ptr_fields', 'count.global_images')):
            s['basis'][b['name']] = b['value']
    # ARGFLOW
    a = s['argflow']
    a['rows'] = len(r['ARGFLOW'])
    for x in r['ARGFLOW']:
        a['src.' + x['src']] = a.get('src.' + x['src'], 0) + 1
        if x['base'] != 'none':
            a['base.' + x['base']] = a.get('base.' + x['base'], 0) + 1
        for fl in flags(x['flags']):
            a['flag.' + fl] = a.get('flag.' + fl, 0) + 1
        if x['hops'] != '0':
            a['hops>0'] = a.get('hops>0', 0) + 1
        if x['read'] != 'none':
            a['read-linked'] = a.get('read-linked', 0) + 1
        if x['reason'] != 'none':
            a['reason.' + x['reason']] = a.get('reason.' + x['reason'], 0) + 1
    # PARAM_FLOW
    p = s['paramflow']
    p['rows'] = len(r['PARAM_FLOW'])
    cmp_depth, lib_depth, nodes = {}, {}, set()
    cmp_by_fn, lib_by_callee = {}, {}  # node -> {function of the comparison / callee of the tested call: shortest depth}
    maxd = 0
    left_out = 0
    for x in r['PARAM_FLOW']:
        node = '%s#%s' % (fname.get(x['fn'], x['fn']), x['param'])
        nodes.add(node)
        d = int(x['depth'])
        k = x['kind']
        fl = flags(x['flags'])
        if k == 'open':
            p['open'] = p.get('open', 0) + 1
            for f in fl:
                p['open.' + f] = p.get('open.' + f, 0) + 1
            left_out += int(x['n'])
            continue
        bucket = '0' if d == 0 else '1' if d == 1 else '2-4' if d <= 4 else '5+'
        p['%s.d%s' % (k, bucket)] = p.get('%s.d%s' % (k, bucket), 0) + 1
        p['depth0' if d == 0 else 'composed'] = p.get('depth0' if d == 0 else 'composed', 0) + 1
        if d > 0 and 'may' in fl:
            p['composed.may'] = p.get('composed.may', 0) + 1
        maxd = max(maxd, d)
        if k == 'comparison':
            cmp_depth[node] = min(cmp_depth.get(node, d), d)
            sf = fname.get(x['sink_fn'], x['sink_fn'])
            byfn = cmp_by_fn.setdefault(node, {})
            byfn[sf] = min(byfn.get(sf, d), d)
        if k == 'call-argument' and 'callee-without-body' in fl and 'result-tested' in fl:
            lib_depth[node] = min(lib_depth.get(node, d), d)
            site = x['sink_site'] if d > 0 else x['site']
            cf = fname.get(callee.get(site, ('none', ''))[0], '?')
            byc = lib_by_callee.setdefault(node, {})
            byc[cf] = min(byc.get(cf, d), d)
    p['nodes'] = len(nodes)
    p['max_depth'] = maxd
    p['left_out'] = left_out
    s['nodes'] = {'cmp': cmp_depth, 'lib': lib_depth, 'cmp_by_fn': cmp_by_fn, 'lib_by_callee': lib_by_callee}
    # relations
    rel = s['relations']
    rel['P-cmp'] = len(cmp_depth)
    rel['P-lib'] = len(lib_depth)
    shared = b_cand = a_cand = 0
    b_pairs, a_pairs = set(), set()
    for x in r['ARGFLOW']:
        is_shared = x['src'] in ('field', 'global', 'element') and x['base'] in ('global', 'global-array-element', 'pointer-field')
        is_string = x['src'] == 'string'
        if not (is_shared or is_string):
            continue
        shared += is_shared
        cf, kind = callee.get(x['site'], ('none', 'none'))
        fns = [cf] if kind == 'direct' and cf != 'none' else targets.get(x['site'], []) if kind == 'indirect' else []
        params = ['%s#%s' % (fname.get(f, f), x['arg_index']) for f in fns]
        loc = '%s.%s' % (x['type'], x['field']) if is_shared else 'string'
        if is_shared and any(q in cmp_depth for q in params):
            b_cand += 1
            for f in fns:
                b_pairs.add((loc, fname.get(f, f)))
        if any(q in lib_depth for q in params):
            a_cand += 1
            for f in fns:
                a_pairs.add((loc, fname.get(f, f)))
    rel.update({'shared': shared, 'B-cand': b_cand, 'B-pairs': len(b_pairs), 'A-cand': a_cand, 'A-pairs': len(a_pairs)})
    # PTR_FIELDS
    t = s['ptrfields']
    t['rows'] = len(r['PTR_FIELDS'])
    for x in r['PTR_FIELDS']:
        key = x['kind'] if x['kind'] != 'read' else 'read.' + x['status']
        t[key] = t.get(key, 0) + 1
        if x['kind'] == 'read':
            for f in flags(x['flags']):
                t['read.flag.' + f] = t.get('read.flag.' + f, 0) + 1
    # GLOBAL_IMAGES
    g = s['gimages']
    g['rows'] = len(r['GLOBAL_IMAGES'])
    objs, trunc = set(), set()
    for x in r['GLOBAL_IMAGES']:
        objs.add(x['object'])
        fl = flags(x['flags'])
        if 'image-truncated' in fl:
            trunc.add(x['object'])
        for f in fl:
            g['flag.' + f] = g.get('flag.' + f, 0) + 1
        g['kind.' + x['kind']] = g.get('kind.' + x['kind'], 0) + 1
    g['objects'] = len(objs)
    g['objects.truncated'] = len(trunc)
    return s


def fmt(v):
    return '-' if v is None else str(v)


def report(kzd, out, cdir):
    os.makedirs(cdir, exist_ok=True)
    summ = {}
    for b in BUILDS:
        with open(os.path.join(out, b, 'NP', 'summary.json')) as f:
            summ[b] = json.load(f)
    # counts.tsv: every metric of the default run
    rows = []
    for part in ('argflow', 'paramflow', 'ptrfields', 'gimages', 'relations'):
        keys = sorted(set(k for b in BUILDS for k in summ[b][part]))
        for k in keys:
            rows.append(['%s.%s' % (part, k)] + [fmt(summ[b][part].get(k)) for b in BUILDS])
    with open(os.path.join(cdir, 'counts.tsv'), 'w') as f:
        f.write('metric\t' + '\t'.join(BUILDS) + '\n')
        for r in rows:
            f.write('\t'.join(r) + '\n')
    # bounds.tsv: the sensitivity of the counts to each tool parameter
    runs = [('default (depth none, size none, image 4096)', None)] + [(o, o) for o in
            ('argflow-depth_1', 'argflow-depth_2', 'argflow-depth_4', 'argflow-depth_8', 'argflow-size_4', 'argflow-size_16', 'argflow-size_64',
             'image-budget_64', 'image-budget_1024')]
    metrics = [('paramflow', 'composed'), ('paramflow', 'open'), ('paramflow', 'left_out'), ('paramflow', 'max_depth'), ('relations', 'P-cmp'),
               ('relations', 'P-lib'), ('relations', 'B-cand'), ('relations', 'B-pairs'), ('relations', 'A-cand'), ('relations', 'A-pairs'),
               ('gimages', 'rows'), ('gimages', 'objects.truncated'), ('ptrfields', 'read.resolved')]
    bsum = {}
    for b in BUILDS:
        for label, tag in runs:
            if tag is None:
                bsum[(b, label)] = summ[b]
            else:
                p = os.path.join(out, b, 'bounds', tag + '.json')
                if os.path.exists(p):
                    with open(p) as f:
                        bsum[(b, label)] = json.load(f)
    with open(os.path.join(cdir, 'bounds.tsv'), 'w') as f:
        f.write('build\trun\t' + '\t'.join('%s.%s' % m for m in metrics) + '\n')
        for b in BUILDS:
            for label, _ in runs:
                s = bsum.get((b, label))
                if s is None:
                    continue
                f.write('%s\t%s\t%s\n' % (b, label, '\t'.join(fmt(s[m[0]].get(m[1], 0)) for m in metrics)))
    # summary.txt
    lines = ['# step2a_s3_count.py report: profile-free runs of step2a_s3.sh (five reference builds)', '']
    lines.append('## sections (default run): ' + ' / '.join(BUILDS))
    for key in ('argflow.rows', 'paramflow.rows', 'paramflow.depth0', 'paramflow.composed', 'paramflow.open', 'paramflow.nodes', 'paramflow.max_depth',
                'ptrfields.rows', 'ptrfields.address-write', 'ptrfields.static-initializer', 'ptrfields.read.resolved', 'ptrfields.read.may',
                'ptrfields.read.needs-field-points-to', 'gimages.rows', 'gimages.objects', 'gimages.objects.truncated', 'relations.P-cmp', 'relations.P-lib',
                'relations.shared', 'relations.B-cand', 'relations.B-pairs', 'relations.A-cand', 'relations.A-pairs'):
        part, k = key.split('.', 1)
        lines.append('  %-40s %s' % (key, ' / '.join(fmt(summ[b][part].get(k, 0)) for b in BUILDS)))
    lines.append('')
    lines.append('## sensitivity (bounds.tsv): B-cand, A-cand, composed rows per run')
    for b in BUILDS:
        parts = []
        for label, _ in runs:
            s = bsum.get((b, label))
            if s is None:
                continue
            parts.append('%s: %s/%s/%s' % (label.split(' ')[0], s['relations'].get('B-cand'), s['relations'].get('A-cand'), s['paramflow'].get('composed', 0)))
        lines.append('  %s  %s' % (b, '; '.join(parts)))
    lines.append('')
    # the depth at which particular parameters reach a comparison / a tested library call, per depth bound (queries only)
    # (build, parameter, 'cmp' or 'lib', the function of the comparison or the callee of the tested call)
    queries = [
        ('c1_pre_tx', 'OS_CountSemGetIdByName#1', 'lib', 'strcmp'),
        ('c1_post_tx', 'OS_CountSemGetIdByName#1', 'lib', 'strcmp'),
        ('c1_pre_tx', 'OS_CountSemCreate#1', 'lib', 'strcmp'),
        ('r2_core', 'CFE_EVS_SendEventWithAppID#2', 'cmp', 'CFE_ResourceId_Equal@cfe/modules/core_api/fsw/inc/cfe_resourceid.h:160'),
        ('r2_core', 'CFE_EVS_SendEventWithAppID#2', 'cmp', '*'),
        ('c5_641', 'OS_TaskRegister#0', 'cmp', '*'),
    ]
    lines.append('## queries: the shortest depth at which a parameter reaches a comparison in a function (cmp) or a tested call of a')
    lines.append('## function without a body (lib), per run ("-": not reached within the run\'s bounds)')
    for b, q, kind, what in queries:
        parts = []
        for label, _ in runs[:8]:
            s = bsum.get((b, label))
            if s is None:
                continue
            if what == '*':
                v = s['nodes']['cmp' if kind == 'cmp' else 'lib'].get(q)
            else:
                v = s['nodes']['cmp_by_fn' if kind == 'cmp' else 'lib_by_callee'].get(q, {}).get(what)
            parts.append('%s %s' % (label.split(' ')[0], fmt(v)))
        lines.append('  %s %s %s %s: %s' % (b, q, kind, what, '; '.join(parts)))
    with open(os.path.join(cdir, 'summary.txt'), 'w') as f:
        f.write('\n'.join(lines) + '\n')
    # spot checks (queries with case names)
    for b in BUILDS:
        spot(out, b, cdir)


def grep(path, pat, limit=40):
    out = []
    rx = re.compile(pat)
    with open(path, errors='replace') as f:
        for line in f:
            if rx.search(line):
                out.append(line.rstrip('\n'))
                if len(out) >= limit:
                    break
    return out


def grep_block(path, pat, follow, limit=40):
    """lines matching pat and the `follow` lines after each"""
    out, n = [], 0
    rx = re.compile(pat)
    with open(path, errors='replace') as f:
        lines = f.readlines()
    for i, line in enumerate(lines):
        if rx.search(line):
            out.extend(l.rstrip('\n') for l in lines[i:i + 1 + follow])
            n += 1
            if n >= limit:
                break
    return out


def spot(out, b, cdir):
    d = os.path.join(out, b, 'NP')
    af, pf, pt, gi = (os.path.join(d, n + '.txt') for n in ('argflow', 'paramflow', 'ptrfields', 'gimages'))
    L = ['# spot checks %s (case names in the queries only)' % b, '']
    if b.startswith('c1'):
        L.append('## the BP key through the pointer field and the static image (ED §2.4, §3.1; T2)')
        L += grep_block(pt, r'read BP_GlobalData_t\.Throttles at \S*bp_flow\.c:(126|132):', 2)
        L += grep(pt, r'address-write BP_GlobalData_t\.Throttles')
        L += grep(gi, r'^  BP_Throttles\[[01]\]\.Name ')
        L += grep(gi, r'^  BP_GlobalData ')
        L.append('')
        L.append('## the arguments of the lookup and the create (ARGFLOW)')
        L += grep(af, r'\[BP_GlobalData\.Throttles\[index\]\.Name\]', 4)
        L += grep(af, r'chan\[i\]\.sem_name\]', 4)
        L.append('')
        L.append('## the CF key: the pointer field written through an address escape (no address write)')
        L += grep(pt, r'read CF_AppData_t\.config_table at \S*cf_cfdp\.c:(99[0-9]|100[0-9]):')
        L.append('')
        L.append('## the name parameter of the lookup API, composed to the comparison callback (PARAM_FLOW)')
        L += grep_block(pf, r'^  OS_CountSemGetIdByName#1 d[0-9]+ (call-argument in OS_ObjectNameMatch|comparison)', 1)
        L += grep_block(pf, r'^  OS_CountSemCreate#1 d[0-9]+ ', 1, 12)
    elif b == 'r2_core':
        n = len(grep(af, r'arg 2 of .*\[CFE_SB_Global\.AppId\] <- field CFE_SB_Global_t\.AppId root=CFE_SB_Global base=global object=CFE_SB_Global', 1000))
        L.append('## T1: CFE_SB_Global_t.AppId as argument 2 of CFE_EVS_SendEventWithAppID (ARGFLOW, base storage)')
        L.append('ARGFLOW rows with source CFE_SB_Global_t.AppId, base global, object CFE_SB_Global, argument 2: %d' % n)
        L += grep(af, r'arg 2 of .*\[CFE_SB_Global\.AppId\]', 3)
        L.append('')
        L.append('## the callee parameter (PARAM_FLOW of CFE_EVS_SendEventWithAppID#2)')
        L += grep_block(pf, r'^  CFE_EVS_SendEventWithAppID#2 ', 1, 20)
        L.append('')
        L.append('## the field\'s object (GLOBAL_IMAGES)')
        L += grep(gi, r'^  CFE_SB_Global ')
    else:
        L.append('## T3/T4: the out-parameter of the spawn wrapper and of the child\'s identity lookup (PARAM_FLOW)')
        L += grep(pf, r'^  OS_TaskCreate#0 d0 ')
        L += grep(pf, r'^  CFE_ES_GetAppID#0 d0 ')
        L += grep(pf, r'^  CFE_ES_GetAppIDInternal#0 d0 ')
        L.append('')
        L.append('## the spawn calls\' entry arguments (ARGFLOW)')
        L += grep(af, r'arg 2 of .*cfe_es_(apps|start)\.c:\d+:\d+ ')
        L.append('')
        L.append('## the object table the core-task spawn reads (GLOBAL_IMAGES)')
        L += grep(gi, r'^  CFE_ES_ObjectTable\[[0-9]+\]\.FuncPtrUnion', 12)
        n = len(grep(gi, r'^  CFE_ES_ObjectTable', 100000))
        L.append('CFE_ES_ObjectTable leaves: %d' % n)
        L.append('')
        L.append('## pointer-field reads (PTR_FIELDS, statuses)')
        L += grep(pt, r' read .* (resolved|may) ', 8)
    with open(os.path.join(cdir, 'spot_%s.txt' % b), 'w') as f:
        f.write('\n'.join(L) + '\n')


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == 'summarize':
        json.dump(summarize(sys.argv[2], sys.argv[3]), sys.stdout, sort_keys=True)
        sys.stdout.write('\n')
        return 0
    if len(sys.argv) >= 5 and sys.argv[1] == 'report':
        report(sys.argv[2], sys.argv[3], sys.argv[4])
        return 0
    sys.stderr.write(__doc__)
    return 64


if __name__ == '__main__':
    sys.exit(main())

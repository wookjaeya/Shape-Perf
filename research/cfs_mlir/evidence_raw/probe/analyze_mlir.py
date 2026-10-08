#!/usr/bin/env python3
"""Feasibility probe: scan MLIR LLVM-dialect text (from mlir-translate --import-llvm
--mlir-print-debuginfo) of cFS app sources and report
  * calls to CFE_SB_/CFE_ES_/CFE_TBL_/CFE_EVS_/CFE_MSG_/CFE_TIME_ (external decl vs. helper defined in module)
  * how MsgId arguments reach SB/MSG calls (direct constant, constant via helper + alloca, global, param, ...)
  * stores/loads on fields of global variables (GEP chains on llvm.mlir.addressof), with DI member names
  * whether ops carry a resolvable source location
This is a textual, intra-procedural, regex-based probe, NOT an analysis tool. It only
inspects the printed IR; it does not model control flow, aliasing through pointers, or
inter-procedural flow beyond the single-hop checks noted in the output.
usage: analyze_mlir.py out.json file1.mlir [file2.mlir ...]
"""
import re, sys, json, collections

FAMILIES = ['CFE_SB_', 'CFE_ES_', 'CFE_TBL_', 'CFE_EVS_', 'CFE_MSG_', 'CFE_TIME_', 'CFE_FS_', 'CFE_PSP_', 'OS_']
# API name -> index of the MsgId argument
MID_ARG = {
    'CFE_SB_Subscribe': [0], 'CFE_SB_SubscribeEx': [0], 'CFE_SB_SubscribeLocal': [0],
    'CFE_SB_Unsubscribe': [0], 'CFE_SB_UnsubscribeLocal': [0],
    'CFE_MSG_Init': [1], 'CFE_MSG_SetMsgId': [1],
    'CFE_SB_MsgId_Equal': [0, 1],
}
MSGPTR_ARG = {'CFE_SB_TransmitMsg': 0, 'CFE_MSG_Init': 0}

R_ALIAS = re.compile(r'^(#[\w.]+) = (.*)$')
R_FUNC = re.compile(r'^  llvm\.func (?:(internal|private|linkonce_odr|weak|available_externally|extern_weak|common|external) )?@"?([\w.$]+)"?\(')
R_RES = re.compile(r'^\s+(%[\w]+)(?::\d+)? = (.*)$')
R_CALL = re.compile(r'llvm\.call @"?([\w.$]+)"?\(([^)]*)\)')
R_CONST = re.compile(r'llvm\.mlir\.constant\((-?\d+) : i\d+\)')
R_ADDR = re.compile(r'llvm\.mlir\.addressof @"?([\w.$]+)"?')
R_GEP = re.compile(r'llvm\.getelementptr [\w|]*\s*(%[\w]+)\[([^\]]*)\].*?-> !llvm\.ptr, !llvm\.struct<"([\w.]+)"')
R_GEP_ANY = re.compile(r'llvm\.getelementptr [\w|]*\s*(%[\w]+)\[([^\]]*)\]')
R_LOAD = re.compile(r'llvm\.load (?:volatile )?(%[\w]+)')
R_STORE = re.compile(r'^\s+llvm\.store (?:volatile )?(%[\w]+), (%[\w]+)')
R_ALLOCA = re.compile(r'llvm\.alloca ')
R_LOCREF = re.compile(r'loc\((#loc\d*|"[^"]*":\d+:\d+|unknown|fused[^\n]*|callsite[^\n]*)\)\s*$')
R_INTR = re.compile(r'"llvm\.intr\.(memset|memcpy|memmove)"\((%[\w]+), (%[\w]+)')


GLOBAL_DI = {}


def parse(path):
    lines = open(path).read().splitlines()
    alias = {}
    for l in lines:
        m = R_ALIAS.match(l)
        if m:
            alias[m.group(1)] = m.group(2)

    def resolve_loc(s, depth=0):
        m = re.search(r'"([^"]+)":(\d+):(\d+)', s)
        if m:
            return f'{m.group(1)}:{m.group(2)}'
        for ref in re.findall(r'#loc\d*\b', s):
            if ref in alias and depth < 20:
                r = resolve_loc(alias[ref], depth + 1)
                if r:
                    return r
        return None

    # DI: struct/typedef name -> member names in declaration order
    di_members = {}
    def members_of(comp_alias):
        body = alias.get(comp_alias, '')
        m = re.search(r'elements = ([^>]*?)(?:, [a-zA-Z]+ =|>$)', body)
        names = []
        if m:
            for a in re.findall(r'#di_derived_type\d*', m.group(1)):
                mm = re.search(r'tag = DW_TAG_member, name = "([^"]+)"', alias.get(a, ''))
                if mm:
                    names.append(mm.group(1))
        return names
    for a, body in alias.items():
        m = re.match(r'#llvm\.di_derived_type<tag = DW_TAG_typedef, name = "([^"]+)", baseType = (#di_composite_type\d*)', body)
        if m:
            di_members.setdefault(m.group(1), members_of(m.group(2)))
        m = re.match(r'#llvm\.di_composite_type<tag = DW_TAG_structure_type, name = "([^"]+)"', body)
        if m:
            di_members.setdefault(m.group(1), members_of(a))

    for k, v in di_members.items():
        if v and not GLOBAL_DI.get(k):
            GLOBAL_DI[k] = v
    funcs = []
    cur = None
    for i, l in enumerate(lines):
        m = R_FUNC.match(l)
        if m:
            is_def = l.rstrip().endswith('{')
            cur = {'name': m.group(2), 'linkage': m.group(1) or 'external', 'is_def': is_def, 'ops': [], 'line': i + 1}
            funcs.append(cur)
            if not is_def:
                cur = None
            continue
        if cur is not None:
            if l == '  }':
                cur = None
                continue
            if l.strip().startswith('^') or not l.strip():
                continue
            cur['ops'].append((i + 1, l))
    return funcs, alias, resolve_loc, di_members


def analyze(path):
    funcs, alias, resolve_loc, di_members = parse(path)
    defined = {f['name'] for f in funcs if f['is_def']}
    declared = {f['name'] for f in funcs if not f['is_def']}
    res = {'file': path, 'n_defined_funcs': len([f for f in funcs if f['is_def']]),
           'defined_funcs': sorted(defined),
           'calls': collections.Counter(), 'calls_by_callee': collections.Counter(),
           'helper_calls_by_callee': collections.Counter(),
           'mid_args': [], 'msgptr_args': [], 'global_stores': [], 'global_loads': [],
           'global_memops': [], 'loc': collections.Counter()}
    for f in funcs:
        if not f['is_def']:
            continue
        defs = {}
        raw_stores = []
        for ln, l in f['ops']:
            m = R_RES.match(l)
            if m:
                defs[m.group(1)] = (ln, m.group(2))
            ms = R_STORE.match(l)
            if ms:
                raw_stores.append((ln, ms.group(1), ms.group(2)))

        def pkey(v, depth=0):
            # canonical key for pointers rooted at an alloca: (alloca, const-indices...)
            if depth > 10 or v not in defs:
                return None
            rhs = defs[v][1]
            if R_ALLOCA.search(rhs) and rhs.startswith('llvm.alloca'):
                return (v,)
            mg = R_GEP_ANY.search(rhs)
            if mg and rhs.startswith('llvm.getelementptr'):
                b = pkey(mg.group(1), depth + 1)
                if b is None:
                    return None
                idx = [x.strip() for x in mg.group(2).split(',')]
                vals = []
                for x in idx:
                    if re.fullmatch(r'-?\d+', x):
                        vals.append(int(x))
                    elif x in defs and R_CONST.search(defs[x][1]) and defs[x][1].startswith('llvm.mlir.constant'):
                        vals.append(int(R_CONST.search(defs[x][1]).group(1)))
                    else:
                        return None
                # drop trailing zeros so that p, p[0], p[0,0] alias (offset-0 coercion)
                key = b + tuple(vals)
                while len(key) > 1 and key[-1] == 0:
                    key = key[:-1]
                return key
            return None

        call_ptr_args = []   # (callee, [args]) for every call
        memcpys = []         # (dst, src)
        for ln, l in f['ops']:
            mc = R_CALL.search(l)
            if mc:
                call_ptr_args.append((mc.group(1), [a.strip() for a in mc.group(2).split(',') if a.strip()]))
            mi = re.search(r'"llvm\.intr\.(?:memcpy|memmove)"\((%[\w]+), (%[\w]+)', l)
            if mi:
                memcpys.append((mi.group(1), mi.group(2)))

        def trace_ptr_src(v):
            gp = gpath(v)
            if gp:
                return {'kind': 'global', 'sym': gp[0], 'path': gp[1]}
            k = pkey(v)
            if k:
                sts = stores_to.get(k, [])
                return {'kind': 'alloca', 'n_stores': len(sts), 'stored': [trace(sv) for _, sv in sts], 'outparam_of': [], 'memcpy_from': []}
            return {'kind': 'unknown_ptr'}

        stores_to = collections.defaultdict(list)
        for ln, sv, sp in raw_stores:
            k = pkey(sp)
            if k:
                stores_to[k].append((ln, sv))

        def gpath(v, depth=0):
            """If pointer v is (GEP chain on) addressof @G, return (G, [field names/indices])."""
            if depth > 10 or v not in defs:
                return None
            _, rhs = defs[v]
            ma = R_ADDR.search(rhs)
            if ma:
                return (ma.group(1), [])
            mg = R_GEP.search(rhs) or R_GEP_ANY.search(rhs)
            if mg:
                base = gpath(mg.group(1), depth + 1)
                if base is None:
                    return None
                idx = [x.strip() for x in mg.group(2).split(',')]
                sname = mg.group(3) if mg.re is R_GEP else None
                path = list(base[1])
                # first index is the pointer offset; following constant indices select fields
                for k, x in enumerate(idx[1:]):
                    if re.fullmatch(r'-?\d+', x):
                        if k == 0 and sname:
                            mem = di_members.get(sname.split('struct.', 1)[-1]) or GLOBAL_DI.get(sname.split('struct.', 1)[-1])
                            if mem and int(x) < len(mem):
                                path.append(mem[int(x)])
                                continue
                        path.append('[' + x + ']')
                    else:
                        path.append('[dyn]')
                return (base[0], path)
            return None

        def gpath_s(v, stored_type=None):
            gp = gpath(v)
            if gp and not gp[1]:
                return (gp[0], ['<base' + (':' + stored_type if stored_type else '') + '>'])
            return gp

        def trace(v, depth=0):
            if v.startswith('%arg'):
                return {'kind': 'param', 'arg': v}
            if depth > 12 or v not in defs:
                return {'kind': 'unknown'}
            ln, rhs = defs[v]
            mc = R_CONST.search(rhs)
            if mc and rhs.startswith('llvm.mlir.constant'):
                return {'kind': 'const', 'value': int(mc.group(1))}
            mcall = R_CALL.search(rhs)
            if mcall:
                callee = mcall.group(1)
                args = [a.strip() for a in mcall.group(2).split(',') if a.strip()]
                if callee == 'CFE_SB_ValueToMsgId' and args:
                    inner = trace(args[0], depth + 1)
                    return {'kind': 'ValueToMsgId', 'of': inner}
                return {'kind': 'call', 'callee': callee}
            ml = R_LOAD.search(rhs)
            if ml and rhs.startswith('llvm.load'):
                p = ml.group(1)
                gp = gpath(p)
                if gp:
                    return {'kind': 'global', 'sym': gp[0], 'path': gp[1]}
                if pkey(p):
                    k0 = pkey(p)
                    sts = stores_to.get(k0, [])
                    vals = [trace(sv, depth + 1) for _, sv in sts]
                    outp = sorted({c for c, ptrs in call_ptr_args if any(pkey(x) and pkey(x)[0] == k0[0] for x in ptrs)})
                    mc_src = [trace_ptr_src(src) for dst, src in memcpys if pkey(dst) and pkey(dst)[0] == k0[0]]
                    return {'kind': 'alloca', 'n_stores': len(sts), 'stored': vals, 'outparam_of': outp, 'memcpy_from': mc_src}
                return {'kind': 'load_other'}
            return {'kind': 'other', 'op': rhs.split(' ')[0]}

        for ln, l in f['ops']:
            lm = R_LOCREF.search(l)
            known = bool(lm and resolve_loc(lm.group(0)))
            opname = None
            m = R_RES.match(l)
            body = m.group(2) if m else l.strip()
            opname = body.split(' ')[0].split('(')[0]
            res['loc'][(opname, known)] += 1
            mcall = R_CALL.search(l)
            if mcall:
                callee = mcall.group(1)
                fam = next((p for p in FAMILIES if callee.startswith(p)), None)
                if fam:
                    kind = 'helper' if callee in defined else 'api'
                    res['calls'][(fam, kind)] += 1
                    (res['helper_calls_by_callee'] if kind == 'helper' else res['calls_by_callee'])[callee] += 1
                args = [a.strip() for a in mcall.group(2).split(',') if a.strip()]
                for k in MID_ARG.get(callee, []):
                    if k < len(args):
                        res['mid_args'].append({'func': f['name'], 'callee': callee, 'arg': k,
                                                'origin': trace(args[k]), 'loc': resolve_loc(lm.group(0)) if lm else None})
                if callee in MSGPTR_ARG and MSGPTR_ARG[callee] < len(args):
                    gp = gpath(args[MSGPTR_ARG[callee]])
                    res['msgptr_args'].append({'func': f['name'], 'callee': callee,
                                               'msg': ({'sym': gp[0], 'path': gp[1]} if gp else trace(args[MSGPTR_ARG[callee]])),
                                               'loc': resolve_loc(lm.group(0)) if lm else None})
            ms = R_STORE.match(l)
            if ms:
                mt = re.search(r': ([\w!.<>]+), !llvm\.ptr', l)
                gp = gpath_s(ms.group(2), mt.group(1) if mt else None)
                if gp:
                    res['global_stores'].append({'func': f['name'], 'sym': gp[0], 'path': gp[1],
                                                 'value': trace(ms.group(1)), 'loc': resolve_loc(lm.group(0)) if lm else None})
            if m and body.startswith('llvm.load'):
                ml = R_LOAD.search(body)
                gp = gpath(ml.group(1)) if ml else None
                if gp:
                    res['global_loads'].append({'func': f['name'], 'sym': gp[0], 'path': gp[1],
                                                'loc': resolve_loc(lm.group(0)) if lm else None})
            mi = R_INTR.search(l)
            if mi:
                gp = gpath(mi.group(2))
                if gp:
                    res['global_memops'].append({'func': f['name'], 'op': mi.group(1), 'sym': gp[0], 'path': gp[1],
                                                 'src': (classify(trace_ptr_src(mi.group(3))) if mi.group(1) != 'memset' else 'memset'),
                                                 'loc': resolve_loc(lm.group(0)) if lm else None})
    return res


def classify(o):
    k = o['kind']
    if k == 'const':
        return 'direct_const'
    if k == 'ValueToMsgId':
        return 'ValueToMsgId(' + classify(o['of']) + ')'
    if k == 'alloca':
        inner = sorted({classify(s) for s in o['stored']} | {'memcpy<-' + classify(s) for s in o.get('memcpy_from', [])})
        outp = o.get('outparam_of') or []
        return f"alloca[{o['n_stores']} store(s)]->" + '|'.join(inner) + (' outparam_of:' + ','.join(outp) if outp else '')
    if k == 'global':
        return 'global'
    return k


def main():
    out = sys.argv[1]
    allres = []
    for p in sys.argv[2:]:
        r = analyze(p)
        allres.append(r)
    summary = {}
    for r in allres:
        locc = collections.Counter()
        for (op, known), n in r['loc'].items():
            grp = op if op in ('llvm.call', 'llvm.store', 'llvm.load', 'llvm.getelementptr', 'llvm.mlir.addressof', 'llvm.mlir.constant') else 'other'
            locc[(grp, known)] += n
        summary[r['file']] = {
            'defined_funcs': r['n_defined_funcs'],
            'api_calls_by_family': {f'{a}{b}': n for (a, b), n in sorted(r['calls'].items())},
            'api_calls_by_callee': dict(sorted(r['calls_by_callee'].items())),
            'helper_calls_by_callee': dict(sorted(r['helper_calls_by_callee'].items())),
            'mid_origins': collections.Counter(f"{m['callee']}#{m['arg']}: {classify(m['origin'])}" for m in r['mid_args']),
            'msgptr': collections.Counter(f"{m['callee']}: " + (m['msg']['sym'] + '.' + '.'.join(m['msg']['path']) if 'sym' in m['msg'] else classify(m['msg'])) for m in r['msgptr_args']),
            'global_store_fields': collections.Counter(f"{s['sym']}.{'.'.join(s['path'])}" for s in r['global_stores']),
            'global_store_value_kinds': collections.Counter(classify(s['value']) for s in r['global_stores']),
            'global_load_fields': len({(s['sym'], tuple(s['path'])) for s in r['global_loads']}),
            'global_loads': len(r['global_loads']),
            'global_memops': [f"{m['op']} {m['sym']}.{'.'.join(m['path'])} src={m.get('src')} @ {m['loc']}" for m in r['global_memops']],
            'loc_known_by_op': {f'{g}:{"known" if k else "unknown"}': n for (g, k), n in sorted(locc.items())},
        }
    def jd(v):
        if isinstance(v, collections.Counter):
            return {(k if isinstance(k, str) else '|'.join(map(str, k))): n for k, n in v.items()}
        return v
    json.dump({'summary': summary, 'detail': [{k: jd(v) for k, v in r.items() if k != 'loc'} for r in allres]},
              open(out, 'w'), indent=1, default=str)
    for f, s in summary.items():
        print('=' * 8, f)
        for k, v in s.items():
            if isinstance(v, (dict, collections.Counter)):
                print(f'  {k}:')
                for kk, vv in (v.items()):
                    print(f'     {vv:4}  {kk}')
            elif isinstance(v, list):
                print(f'  {k}:')
                for x in v:
                    print('       ', x)
            else:
                print(f'  {k}: {v}')


if __name__ == '__main__':
    main()

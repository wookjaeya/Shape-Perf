#!/usr/bin/env python3
"""Feasibility check for section 5 (cfs dialect lifting).

Input : MLIR LLVM-dialect text imported with --mlir-print-debuginfo from an
        app-linked module compiled at -O1 -Xclang -disable-llvm-passes and
        cleaned with --inline --sroa --mem2reg --canonicalize --cse.
Output: for every llvm.load / llvm.store that carries a TBAA tag and whose
        address is rooted at llvm.mlir.addressof @G (optionally via constant
        GEPs), the byte offset taken from the TBAA tag and the DI member path
        of @G at that offset.  Also counts accesses where the address operand
        is the bare addressof (no GEP), i.e. the offset-0 case that the earlier
        probe could not name.
This is a regex-based probe, not an analysis.  It does not model aliasing or
control flow.
"""
import re, sys, collections, json

src = open(sys.argv[1]).read()
lines = src.split('\n')

# ---- attribute alias table -------------------------------------------------
alias = {}
for ln in lines:
    m = re.match(r'^(#[\w.$-]+) = (.*)$', ln)
    if m:
        alias[m.group(1)] = m.group(2)

def field(body, key):
    m = re.search(r'\b' + key + r' = (#[\w.$-]+|"[^"]*"|-?\d+)', body)
    return m.group(1) if m else None

def strip_typedef(t):
    # follow DW_TAG_typedef / const / volatile derived types to a composite
    seen = 0
    while t and seen < 20:
        seen += 1
        body = alias.get(t, '')
        if 'di_composite_type<' in body:
            return t
        if 'di_derived_type<' in body and ('DW_TAG_typedef' in body or 'DW_TAG_const_type' in body or 'DW_TAG_volatile_type' in body):
            t = field(body, 'baseType')
            continue
        return t
    return t

def members(comp):
    body = alias.get(comp, '')
    m = re.search(r'elements = ([^>]*)>$', body)
    if not m:
        return []
    out = []
    for e in re.findall(r'#di_derived_type\d+', m.group(1)):
        eb = alias.get(e, '')
        if 'DW_TAG_member' not in eb:
            continue
        name = field(eb, 'name')
        off = field(eb, 'offsetInBits')
        size = field(eb, 'sizeInBits')
        out.append((name.strip('"') if name else '?', int(off) if off else 0,
                    int(size) if size else 0, field(eb, 'baseType')))
    return out

def path_at(ty, byte_off, depth=0):
    comp = strip_typedef(ty)
    body = alias.get(comp, '')
    if 'DW_TAG_structure_type' not in body or depth > 10:
        return []
    bit = byte_off * 8
    best = None
    for (n, o, s, bt) in members(comp):
        if o <= bit < o + max(s, 1):
            best = (n, o, bt)
    if not best:
        return ['<no member at +%d>' % byte_off]
    n, o, bt = best
    return [n] + path_at(bt, (bit - o) // 8, depth + 1)

# ---- globals with DI -------------------------------------------------------
gdi = {}
for ln in lines:
    m = re.search(r'llvm\.mlir\.global \w+ @([\w.$]+)\(.*dbg_exprs = \[(#[\w]+)\]', ln)
    if m:
        expr = alias.get(m.group(2), '')
        var = field(expr, 'var')
        vb = alias.get(var, '')
        gdi[m.group(1)] = field(vb, 'type')

# ---- SSA def table inside functions -------------------------------------------
defs = {}
for ln in lines:
    m = re.match(r'\s*(%[\w#]+) = (llvm\.mlir\.addressof @([\w.$]+)|llvm\.getelementptr [^%]*(%[\w#]+)\[([^\]]*)\])', ln)
    if m:
        if m.group(3):
            defs[m.group(1)] = ('addr', m.group(3))
        else:
            defs[m.group(1)] = ('gep', m.group(4), m.group(5))

# SSA names are function-local; the probe resolves within the current function.
results = []
cur_fn = None
local = {}
for i, ln in enumerate(lines, 1):
    fm = re.match(r'\s*llvm\.func (?:\w+ )?@([\w.$]+)\(', ln)
    if fm:
        cur_fn = fm.group(1); local = {}
        continue
    m = re.match(r'\s*(%[\w#]+) = llvm\.mlir\.addressof @([\w.$]+)', ln)
    if m:
        local[m.group(1)] = ('addr', m.group(2)); continue
    m = re.match(r'\s*(%[\w#]+) = llvm\.getelementptr [^%]*(%[\w#]+)\[([^\]]*)\]', ln)
    if m:
        local[m.group(1)] = ('gep', m.group(2), m.group(3)); continue
    m = re.match(r'\s*(?:(%[\w#]+) = )?llvm\.(load|store) (%[\w#]+)(?:, (%[\w#]+))? \{[^}]*tbaa = \[(#tbaa_tag\d*)\]', ln)
    if not m:
        continue
    kind = m.group(2)
    addr = m.group(3) if kind == 'load' else m.group(4)
    tag = m.group(5)
    tb = alias.get(tag, '')
    off = field(tb, 'offset')
    scalar = field(tb, 'base_type') == field(tb, 'access_type')
    root, hops, geps = addr, 0, []
    while root in local and local[root][0] == 'gep' and hops < 8:
        geps.insert(0, local[root][2]); root = local[root][1]; hops += 1
    if root in local and local[root][0] == 'addr':
        g = local[root][1]
        ty = gdi.get(g)
        if not scalar:
            p = path_at(ty, int(off)) if (ty and off is not None) else ['<no DI>']
            how = 'tbaa-struct-path'
        else:
            # scalar tag: offset is relative to the scalar; fall back to GEP indices
            p, t = [], ty
            for idxs in geps:
                parts = [x.strip() for x in idxs.split(',')]
                for k in parts[1:]:
                    comp = strip_typedef(t)
                    mem = members(comp)
                    if k.lstrip('-').isdigit() and mem and int(k) < len(mem):
                        p.append(mem[int(k)][0]); t = mem[int(k)][3]
                    else:
                        p.append('[%s]' % k)
            if not p: p = ['<scalar tag, no GEP>']
            how = 'gep-index'
        locm = re.search(r'loc\((#loc\d+)\)\s*$', ln)
        results.append({'fn': cur_fn, 'line': i, 'op': kind, 'global': g,
                        'tbaa_off': int(off) if off else None,
                        'bare_addressof': hops == 0, 'how': how,
                        'path': '.'.join(p)})

cnt = collections.Counter((r['global'], r['path'], r['op'], r['how']) for r in results)
bare = [r for r in results if r['bare_addressof']]
print('accesses rooted at a global with TBAA:', len(results))
print('  of which address = bare addressof (no GEP):', len(bare))
print('  bare-addressof accesses with a DI member name:', sum(1 for r in bare if not r['path'].startswith('<')))
for (g, p, op, how), n in sorted(cnt.items()):
    print('%-20s %-38s %-5s %-16s x%d' % (g, p, op, how, n))
print('\nbare-addressof sites:')
for r in bare:
    print('  %s L%d %s %s -> %s.%s (tbaa off %s)' % (r['fn'], r['line'], r['op'], r['global'], r['global'], r['path'], r['tbaa_off']))
json.dump(results, open(sys.argv[2], 'w'), indent=1)

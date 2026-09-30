"""Tracer-free GOT-slot probe (no gdb, no LD_DEBUG, no shapeperf.* code).

Usage: python probe.py <static.json> <scenario.json>
scenario: {"name":..., "arts": {"A": {"path":..., "tag": "alpha"|null}}, "steps": [["load","A"],["call","A"],...]}
Reads, in-process, the 8-byte GOT slot values of every model-defined JUMP_SLOT / GLOB_DAT of each loaded
model and maps every address to its DSO (via /proc/self/maps, cross-checked against the link_map l_addr)
and to a dynsym name of that DSO.  Also hashes the bytes at the resolved compute address in memory and
compares them to the on-disk compute function of every artifact.
"""
import ctypes, hashlib, json, os, sys

import numpy as np

sys.path.insert(0, '/home/user/work/onnx-mlir/build/Release/lib')
from PyRuntime import OMExecutionSession  # noqa: E402

static = json.load(open(sys.argv[1]))
sc = json.load(open(sys.argv[2]))
x = np.load('/home/user/work/v3/g2/K/L0064/input.npy')
exp = np.load('/home/user/work/v3/g2/K/L0064/expected.npy')
arts = sc['arts']


def disk_func_sha(path, name):
    v, sz = static[path]['funcs'][name]
    # PT_LOAD for .text: file offset == vaddr for these objects (checked with readelf -l by the driver)
    with open(path, 'rb') as f:
        f.seek(v)
        return hashlib.sha256(f.read(sz)).hexdigest()


def maps():
    out = []
    with open('/proc/self/maps') as f:
        for line in f:
            p = line.split()
            lo, hi = (int(v, 16) for v in p[0].split('-'))
            out.append((lo, hi, p[1], int(p[2], 16), p[5] if len(p) > 5 else ''))
    return out


def base_of(path):
    ms = [m for m in maps() if m[4] == path and m[3] == 0]
    return min(m[0] for m in ms) if ms else None


def linkmap(path):
    # glibc: dlopen handle == struct link_map*; l_addr at +0, l_name at +8.  RTLD_NOLOAD: never loads.
    h = ctypes.CDLL(path, mode=os.RTLD_NOLOAD | os.RTLD_LAZY)._handle
    l_addr = ctypes.c_uint64.from_address(h).value
    l_name = ctypes.cast(ctypes.c_void_p.from_address(h + 8).value, ctypes.c_char_p).value.decode()
    return l_addr, l_name


def dlsym_handle(path, name):
    lib = ctypes.CDLL(path, mode=os.RTLD_NOLOAD | os.RTLD_LAZY)
    return ctypes.cast(getattr(lib, name), ctypes.c_void_p).value


sess, loaded, log = {}, {}, []


def who(addr):
    for lo, hi, perm, off, path in maps():
        if lo <= addr < hi:
            for k, (p, b) in loaded.items():
                if p == path:
                    rel = addr - b
                    names = [n for n, v in static[p]['dynsym'].items() if v == rel]
                    pl = static[p]['plt']
                    if not names and pl[0] <= rel < pl[1]:
                        names = ['<own .plt stub+%d: UNRESOLVED>' % ((rel - pl[0]) % 16)]
                    return {'dso': k, 'rel': hex(rel), 'sym': names}
            return {'dso': os.path.basename(path) or '[anon]', 'path': path, 'rel': hex(addr - lo + off), 'sym': []}
    return {'dso': '?', 'rel': hex(addr), 'sym': []}


def slot(k, kind, name):
    p, b = loaded[k]
    tbl = static[p]['jslot' if kind == 'J' else 'gdat']
    if name not in tbl:
        return None
    return ctypes.c_uint64.from_address(b + tbl[name]).value


def snapshot(when):
    snap = {}
    for k, (p, b) in loaded.items():
        d = {}
        for nm in sorted(static[p]['jslot']):
            d['J ' + nm] = who(slot(k, 'J', nm))
        for nm in sorted(static[p]['gdat']):
            d['G ' + nm] = who(slot(k, 'G', nm))
        snap[k] = d
    log.append({'when': when, 'slots': snap})


def chain(k):
    """Follow entry(k) -> ciface slot of k -> main_graph slot of the DSO that holds the resolved ciface."""
    tag = arts[k]['tag'] or 'model'
    p, b = loaded[k]
    ent = who(dlsym_handle(p, 'run_main_graph_' + tag))
    cif_addr = slot(k, 'J', '_mlir_ciface_main_graph_' + tag)
    cif = who(cif_addr)
    res = {'entry(dlsym handle)': ent, 'ciface_slot_in_' + k: cif}
    if cif['dso'] in loaded:
        mg_addr = slot(cif['dso'], 'J', 'main_graph_' + tag)
        mg = who(mg_addr)
        res['main_graph_slot_in_' + cif['dso']] = mg
        if mg['dso'] in loaded and mg['sym'] == ['main_graph_' + tag]:
            pp = loaded[mg['dso']][0]
            sz = static[pp]['funcs']['main_graph_' + tag][1]
            mem = hashlib.sha256(ctypes.string_at(mg_addr, sz)).hexdigest()
            res['compute_mem_sha256'] = mem
            res['compute_mem_matches_disk_of'] = [a for a, v in arts.items()
                                                  if ('main_graph_' + tag) in static[v['path']]['funcs']
                                                  and disk_func_sha(v['path'], 'main_graph_' + tag) == mem]
        res['compute_dso'] = mg['dso']
    return res


for op, k in sc['steps']:
    if op == 'load':
        a = arts[k]
        kw = {'shared_lib_path': a['path']}
        if a['tag']:
            kw['tag'] = a['tag']
        sess[k] = OMExecutionSession(**kw)
        b = base_of(a['path'])
        lb, ln = linkmap(a['path'])
        assert lb == b and ln == a['path'], (hex(lb), hex(b), ln)
        loaded[k] = (a['path'], b)
        log.append({'when': f'load {k}', 'base': hex(b), 'linkmap_l_addr_equal': True})
        snapshot(f'after load {k}')
    else:
        out = sess[k].run([x])
        ok = bool(out[0].shape == exp.shape and np.array_equal(out[0], exp))
        log.append({'when': f'call {k}', 'output_exact': ok, 'chain_after_call': chain(k)})
        snapshot(f'after call {k}')

print(json.dumps({'scenario': sc['name'], 'pid': os.getpid(), 'dlopenflags': sys.getdlopenflags(),
                  'bind_now_env': os.environ.get('LD_BIND_NOW'),
                  'loaded': {k: {'path': p, 'base': hex(b)} for k, (p, b) in loaded.items()}, 'log': log}))

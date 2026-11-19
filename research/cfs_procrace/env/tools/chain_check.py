#!/usr/bin/env python3
"""chain_check.py -- check the chain from a site manifest (plan) through a derived build to a boot record.

Run with `python3 -I`.  Reads only.

    chain_check.py --plan MANIFEST --build-record FILE --run-meta FILE [--build-root DIR] [--dump-text FILE]
                   [--basis-text FILE] [--main PATH] [-o OUT]
                   [--sidecar FILE] [--plan-facts FILE] [--plan-basis FILE] [--inst-list FILE] [--inst-tree DIR]
                   [--dump FILE] [--bindings FILE] [--bprime-basis FILE] [--facts-header FILE] [--kzt-text FILE]

Links (each one is confirmed, refused or unconfirmed, with what was compared):
  manifest      sha256 of --plan = the "manifest sha256" of the build record = the "manifest ... sha256=" of run.meta
                = every manifest_sha256 of the dump text (run and module lines), when given.
  exe <path>    for every executable of the build record ("<path> sha256 <hex>" lines whose path is not a generated
                source table): the file under --build-root has that sha256 (unconfirmed when --build-root is absent
                or the file is missing), and the run.meta "file <p> sha256=<hex>" line whose path is a suffix of the
                build path (or the reverse) has it too (unconfirmed when run.meta names no such file).
  facts_build   the binary.sha256 of --basis-text (the basis the facts were made from) = the run.meta sha256 of
                --main (the executable that basis describes; default: the first executable of the build record).
                Unconfirmed when no basis text is given (for example, no basis of the derived build yet).
  same_build    (kairos/docs/event_semantics.md 13.4, 13.5 (sa)) one same_build_object line per object and a summary
                line: the build-id of each "buildid" line of the dump text (dump 1.5) against readelf -n of the same file
                under --build-root (object=main: --main; object=module:<n>: the one build record path whose end matches
                the line's path); a module of the dump's "module" lines without a buildid line, or a dump without a
                main line, is unconfirmed(no build-id in dump).  A dump before 1.5 has no buildid line.
Optional links (added 2026-11-16 for a generated plan; each is emitted only when its inputs are given):
  sidecar       the "# sidecar sha256=<hex>" header line of --plan = sha256 of --sidecar.
  plan_inputs   the sidecar's "input facts sha256=" and "input basis sha256=" = sha256 of --plan-facts and --plan-basis
                (the facts and basis the plan was generated from).
  exe (again)   with --inst-list, the instance list run.meta names ("instance_files ... list_sha256=<hex>", checked first)
                is a second source of run sha256 for every executable; where run.meta has a "file" line too, both agree.
  instance      every entry of --inst-list = the file at the same relative path under --inst-tree (the tree the
                instance was unpacked from); count of entries equal, differing, missing.
  bprime        the parent basis of the B' facts (--facts-header: the kz-dump text head of facts.kz, "parent[0] ...
                sha256=") = sha256 of --bprime-basis = the parent basis of the run container (--kzt-text).
  dump          run.meta "dump <name> bytes=<n> sha256=<hex>" = sha256 of --dump = the run container's
                BASIS convert.dump.sha256 (with --kzt-text).
  bindings      run.meta "bindings <path> sha256=<hex>" = sha256 of --bindings = the run container's FILE_HASHES entry of
                that path (with --kzt-text).
  The manifest link also compares, with --kzt-text, the run container's NRUN manifest_sha256 and its FILE_HASHES entry
  of the plan path.
The chain verdict is refused when any link is refused, else unconfirmed when any of manifest, exe or facts_build is
unconfirmed, else confirmed.  same_build is reported apart and is no part of the chain verdict (nor of a boot's gate).
Exit status: 0 confirmed, 1 refused, 3 unconfirmed, 2 usage or read error.
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys

GENERATED = re.compile(r'(cfe_module_version_table\.c|cfe_build_env_table\.c)$')


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for blk in iter(lambda: f.read(1 << 20), b''):
            h.update(blk)
    return h.hexdigest()


def read(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read().split('\n')


def build_record(path):
    man, exes = None, []
    for line in read(path):
        m = re.search(r'manifest sha256 ([0-9a-f]{64})', line)
        if m and man is None:
            man = m.group(1)
        m = re.match(r'(\S+) sha256 ([0-9a-f]{64})\b', line)
        if m and not GENERATED.search(m.group(1)) and not line.startswith(('runtime ', 'plugin ')):
            exes.append((m.group(1), m.group(2)))
    return man, exes


def run_meta(path):
    man, files = None, {}
    for line in read(path):
        m = re.match(r'manifest (\S+) sha256=([0-9a-f]{64})', line)
        if m and man is None:
            man = m.group(2)
        m = re.match(r'file (\S+) sha256=([0-9a-f]{64})', line)
        if m:
            files[m.group(1)] = m.group(2)
    return man, files


def match_meta(files, bpath):
    hits = [p for p in files if bpath.endswith('/' + p) or bpath == p or p.endswith('/' + bpath)]
    return hits[0] if len(hits) == 1 else None


def parse_kzt(lines):
    """The kz-dump text of a run container: parent basis, NRUN manifest_sha256s, FILE_HASHES, convert.dump.sha256."""
    out = {'parent': None, 'nrun': [], 'files': {}, 'dump': None}
    for line in lines:
        m = re.match(r'\s*parent\[0\] kind=basis .*sha256=([0-9a-f]{64})', line)
        if m and out['parent'] is None:
            out['parent'] = m.group(1)
        if re.match(r'\s*NRUN\[\d+\] ', line):
            out['nrun'] += re.findall(r'manifest_sha256=([0-9a-f]{64})', line)
        m = re.match(r'\s*FILE_HASHES\[\d+\] path="([^"]*)" sha256=([0-9a-f]{64})', line)
        if m:
            out['files'][m.group(1)] = m.group(2)
        m = re.match(r'\s*BASIS\[\d+\] name="convert\.dump\.sha256" value="([0-9a-f]{64})"', line)
        if m:
            out['dump'] = m.group(1)
    return out


def build_id(path):
    try:
        r = subprocess.run(['readelf', '-n', path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    except OSError:
        return None
    m = re.search(r'Build ID: ([0-9a-f]+)', r.stdout.decode('utf-8', 'replace'))
    return m.group(1) if m else None


def main():
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument('--plan', required=True)
    ap.add_argument('--build-record', required=True)
    ap.add_argument('--run-meta', required=True)
    ap.add_argument('--build-root')
    ap.add_argument('--dump-text')
    ap.add_argument('--basis-text')
    ap.add_argument('--main')
    ap.add_argument('-o', dest='out')
    for opt in ('--sidecar', '--plan-facts', '--plan-basis', '--inst-list', '--inst-tree', '--dump', '--bindings',
                '--bprime-basis', '--facts-header', '--kzt-text'):
        ap.add_argument(opt)
    a = ap.parse_args()
    lines, status = [], {}

    def link(name, st, detail):
        status[name] = st
        lines.append('%s\t%s\t%s' % (name, st, detail))

    try:
        plan = sha(a.plan)
        bman, exes = build_record(a.build_record)
        mman, mfiles = run_meta(a.run_meta)
        dman = []
        dobj = []      # (object, path, build_id or None, note, path_cut) of the dump's buildid lines (dump 1.5)
        dmods = []     # the module numbers of the dump's module lines
        if a.dump_text:
            for line in read(a.dump_text):
                dman += re.findall(r'manifest_sha256=([0-9a-f]{64})', line)
                m = re.match(r'buildid object=(main|module:\d+) path="((?:[^"\\]|\\.)*)" build_id=([0-9a-f]+|-) note=(present|absent|unreadable) ', line)
                if m:
                    path = re.sub(r'\\x([0-9a-f]{2})', lambda x: chr(int(x.group(1), 16)), m.group(2)).replace('\\"', '"').replace('\\\\', '\\')
                    dobj.append((m.group(1), path, None if m.group(3) == '-' else m.group(3), m.group(4), ' path_cut=yes' in line))
                m = re.match(r'module (\d+) actor=', line)
                if m and int(m.group(1)) not in dmods:
                    dmods.append(int(m.group(1)))
        btxt = None
        if a.basis_text:
            for line in read(a.basis_text):
                m = re.match(r'binary\.sha256 = ([0-9a-f]{64})', line)
                if m:
                    btxt = m.group(1)
                    break
        meta = read(a.run_meta)
        plan_lines = read(a.plan)
        kzt = parse_kzt(read(a.kzt_text)) if a.kzt_text else None
        inst = None
        if a.inst_list:
            inst = {}
            for line in read(a.inst_list):
                m = re.match(r'([0-9a-f]{64})  (?:\./)?(.+)$', line)
                if m:
                    inst[m.group(2)] = m.group(1)
            inst_sha = sha(a.inst_list)
    except OSError as e:
        sys.stderr.write('chain_check.py: %s\n' % e)
        return 2

    vals = {'plan': plan, 'build_record': bman, 'run.meta': mman}
    if a.dump_text:
        vals['dump'] = ','.join(sorted(set(dman))) if dman else None
    if kzt is not None:
        vals['kzt.nrun'] = ','.join(sorted(set(kzt['nrun']))) if kzt['nrun'] else None
        vals['kzt.file_hashes'] = kzt['files'].get(os.path.abspath(a.plan))
    present = {k: v for k, v in vals.items() if v}
    detail = ' '.join('%s=%s' % (k, (v or 'missing')[:16]) for k, v in vals.items())
    if any(v != plan for v in present.values()):
        link('manifest', 'refused', detail)
    elif len(present) < len(vals):
        link('manifest', 'unconfirmed', detail)
    else:
        link('manifest', 'confirmed', detail)

    if a.sidecar:
        hdr = [m.group(1) for m in (re.match(r'# sidecar sha256=([0-9a-f]{64})', x) for x in plan_lines) if m]
        try:
            sc = sha(a.sidecar)
        except OSError as e:
            sys.stderr.write('chain_check.py: %s\n' % e)
            return 2
        if not hdr:
            link('sidecar', 'unconfirmed', 'no sidecar line in the plan header (file %s)' % sc[:16])
        elif hdr[0] == sc:
            link('sidecar', 'confirmed', 'plan header=%s = file' % sc[:16])
        else:
            link('sidecar', 'refused', 'plan header=%s file=%s' % (hdr[0][:16], sc[:16]))
        if a.plan_facts or a.plan_basis:
            side = read(a.sidecar)
            parts, st = [], 'confirmed'
            for kind, path in (('facts', a.plan_facts), ('basis', a.plan_basis)):
                rec = [m.group(1) for m in (re.match(r'input %s sha256=([0-9a-f]{64})' % kind, x) for x in side) if m]
                if not path:
                    parts.append('%s=not-given' % kind)
                    st = 'unconfirmed' if st == 'confirmed' else st
                    continue
                fh = sha(path)
                if not rec:
                    parts.append('%s sidecar=missing file=%s' % (kind, fh[:16]))
                    st = 'unconfirmed' if st == 'confirmed' else st
                elif rec[0] != fh:
                    parts.append('%s sidecar=%s file=%s' % (kind, rec[0][:16], fh[:16]))
                    st = 'refused'
                else:
                    parts.append('%s=%s' % (kind, fh[:16]))
            link('plan_inputs', st, ' '.join(parts))

    if inst is not None:
        named = [m.group(1) for m in (re.search(r'^instance_files .* list_sha256=([0-9a-f]{64})', x) for x in meta) if m]
        if not named:
            link('inst_list', 'unconfirmed', 'run.meta names no instance list (file %s)' % inst_sha[:16])
        elif named[0] != inst_sha:
            link('inst_list', 'refused', 'run.meta list_sha256=%s file=%s' % (named[0][:16], inst_sha[:16]))
        else:
            link('inst_list', 'confirmed', 'run.meta list_sha256=%s = file (%d entries)' % (inst_sha[:16], len(inst)))
        if a.inst_tree:
            eq, diff, miss = 0, [], []
            for p_, h_ in sorted(inst.items()):
                f = os.path.join(a.inst_tree, p_)
                if not os.path.isfile(f):
                    miss.append(p_)
                elif sha(f) != h_:
                    diff.append(p_)
                else:
                    eq += 1
            st = 'refused' if diff else ('unconfirmed' if miss else 'confirmed')
            link('instance', st, 'entries=%d equal=%d differing=%d%s missing=%d%s' % (
                len(inst), eq, len(diff), (' [' + ','.join(diff[:5]) + ']') if diff else '', len(miss),
                (' [' + ','.join(miss[:5]) + ']') if miss else ''))

    if not exes:
        link('exe', 'unconfirmed', 'no executable in the build record')
    for bpath, h in exes:
        parts, st = ['record=%s' % h[:16]], 'confirmed'
        if a.build_root:
            f = os.path.join(a.build_root, bpath)
            if os.path.isfile(f):
                fh = sha(f)
                parts.append('tree=%s' % fh[:16])
                if fh != h:
                    st = 'refused'
            else:
                parts.append('tree=missing')
                st = 'unconfirmed' if st == 'confirmed' else st
        else:
            parts.append('tree=not-given')
            st = 'unconfirmed' if st == 'confirmed' else st
        mp = match_meta(mfiles, bpath)
        ip = match_meta(inst, bpath) if inst is not None else None
        if mp:
            parts.append('run.meta[%s]=%s' % (mp, mfiles[mp][:16]))
            if mfiles[mp] != h:
                st = 'refused'
        if ip:
            parts.append('inst_list[%s]=%s' % (ip, inst[ip][:16]))
            if inst[ip] != h:
                st = 'refused'
        if not mp and not ip:
            parts.append('run.meta=no-such-file')
            st = 'unconfirmed' if st == 'confirmed' else st
        link('exe %s' % bpath, st, ' '.join(parts))

    mainp = a.main or (exes[0][0] if exes else None)
    mainh = None
    if mainp:
        mp = match_meta(mfiles, mainp)
        mainh = mfiles.get(mp) if mp else None
        if mainh is None and inst is not None:
            ip = match_meta(inst, mainp)
            mainh = inst.get(ip) if ip else None
    if not a.basis_text:
        link('facts_build', 'unconfirmed', 'no basis of the facts given (main %s run=%s)' % (mainp, (mainh or 'missing')[:16]))
    elif not btxt or not mainh:
        link('facts_build', 'unconfirmed', 'basis binary.sha256=%s run main %s=%s' % ((btxt or 'missing')[:16], mainp, (mainh or 'missing')[:16]))
    elif btxt == mainh:
        link('facts_build', 'confirmed', 'basis binary.sha256=%s = run main %s' % (btxt[:16], mainp))
    else:
        link('facts_build', 'refused', 'basis binary.sha256=%s != run main %s=%s' % (btxt[:16], mainp, mainh[:16]))

    if a.bprime_basis or a.facts_header:
        vals_b, st = {}, 'confirmed'
        if a.bprime_basis:
            vals_b['basis'] = sha(a.bprime_basis)
        if a.facts_header:
            fp = [m.group(1) for m in (re.match(r'\s*parent\[0\] kind=basis .*sha256=([0-9a-f]{64})', x) for x in read(a.facts_header)) if m]
            vals_b['facts.parent'] = fp[0] if fp else None
        if kzt is not None:
            vals_b['kzt.parent'] = kzt['parent']
        pres = [v for v in vals_b.values() if v]
        if len(set(pres)) > 1:
            st = 'refused'
        elif len(pres) < len(vals_b) or len(vals_b) < 3:
            st = 'unconfirmed'
        link('bprime', st, ' '.join('%s=%s' % (k, (v or 'missing')[:16]) for k, v in vals_b.items()))

    if a.dump:
        dm = [m.group(1) for m in (re.match(r'dump \S+ bytes=\d+ sha256=([0-9a-f]{64})', x) for x in meta) if m]
        vals_d = {'file': sha(a.dump), 'run.meta': dm[0] if dm else None}
        if kzt is not None:
            vals_d['kzt'] = kzt['dump']
        pres = [v for v in vals_d.values() if v]
        st = 'refused' if len(set(pres)) > 1 else ('unconfirmed' if len(pres) < len(vals_d) or kzt is None else 'confirmed')
        link('dump', st, ' '.join('%s=%s' % (k, (v or 'missing')[:16]) for k, v in vals_d.items()))

    if a.bindings:
        bm = [m.group(1) for m in (re.match(r'bindings \S+ sha256=([0-9a-f]{64})', x) for x in meta) if m]
        vals_n = {'file': sha(a.bindings), 'run.meta': bm[0] if bm else None}
        if kzt is not None:
            vals_n['kzt'] = kzt['files'].get(os.path.abspath(a.bindings))
        pres = [v for v in vals_n.values() if v]
        st = 'refused' if len(set(pres)) > 1 else ('unconfirmed' if len(pres) < len(vals_n) or kzt is None else 'confirmed')
        link('bindings', st, ' '.join('%s=%s' % (k, (v or 'missing')[:16]) for k, v in vals_n.items()))

    # same_build (event_semantics.md 13.4, 13.5 (sa)): per object, then the summary; never part of the chain verdict
    def tree_id(rel):
        if a.build_root and rel and os.path.isfile(os.path.join(a.build_root, rel)):
            return build_id(os.path.join(a.build_root, rel))
        return None

    def record_path(dpath):
        hits = [p for p, _ in exes if dpath and (p == dpath or p.endswith('/' + dpath) or dpath.endswith('/' + p))]
        return hits[0] if len(hits) == 1 else None

    objs = []  # (verdict, object, path, tree id, dump id)
    seen_main = False
    seen_mods = set()
    for obj, dpath, did, note, cut in dobj:
        if obj == 'main':
            seen_main = True
            rel = mainp
        else:
            seen_mods.add(int(obj.split(':')[1]))
            rel = None if cut else record_path(dpath)
        bid = tree_id(rel) if rel else None
        if did is None:
            st = 'unconfirmed(no build-id note)'
        elif rel is None:
            st = 'unconfirmed(object not in the build record)'
        elif bid is None:
            st = 'unconfirmed(no build-id in the build tree)'
        else:
            st = 'confirmed' if did == bid else 'refused'
        objs.append((st, obj, rel or dpath, bid, did))
    if not seen_main and a.dump_text:
        objs.insert(0, ('unconfirmed(no build-id in dump)', 'main', mainp, tree_id(mainp), None))
    for n in dmods:
        if n not in seen_mods:
            objs.append(('unconfirmed(no build-id in dump)', 'module:%d' % n, '-', None, None))
    for st, obj, path, bid, did in objs:
        lines.append('same_build_object\t%s\t%s path=%s tree=%s dump=%s' % (st, obj, path or '-', bid or 'not read', did or 'none'))
    nconf = sum(1 for o in objs if o[0] == 'confirmed')
    nref = sum(1 for o in objs if o[0] == 'refused')
    nunc = len(objs) - nconf - nref
    if not objs:
        sb = 'unconfirmed(no build-id in dump)'
    elif nref:
        sb = 'refused'
    elif nunc:
        sb = [o[0] for o in objs if o[0].startswith('unconfirmed')][0]
    else:
        sb = 'confirmed'
    lines.append('same_build\t%s\t%d objects: confirmed %d, refused %d, unconfirmed %d' % (sb, len(objs), nconf, nref, nunc))

    core = [v for k, v in status.items()]
    verdict = 'refused' if 'refused' in core else ('unconfirmed' if 'unconfirmed' in core else 'confirmed')
    lines.append('chain\t%s\t%d links' % (verdict, len(core)))
    text = '\n'.join(lines) + '\n'
    if a.out:
        with open(a.out, 'w', encoding='utf-8') as f:
            f.write('# chain_check.py --plan %s --build-record %s --run-meta %s --build-root %s --dump-text %s --basis-text %s --main %s\n'
                    % (a.plan, a.build_record, a.run_meta, a.build_root, a.dump_text, a.basis_text, mainp))
            extra = [(k, getattr(a, k)) for k in ('sidecar', 'plan_facts', 'plan_basis', 'inst_list', 'inst_tree', 'dump',
                                                  'bindings', 'bprime_basis', 'facts_header', 'kzt_text') if getattr(a, k)]
            if extra:
                f.write('# and %s\n' % ' '.join('--%s %s' % (k.replace('_', '-'), v) for k, v in extra))
            f.write(text)
    sys.stdout.write(text)
    return {'confirmed': 0, 'refused': 1, 'unconfirmed': 3}[verdict]


if __name__ == '__main__':
    sys.exit(main())

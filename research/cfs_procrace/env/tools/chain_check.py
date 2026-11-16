#!/usr/bin/env python3
"""chain_check.py -- check the chain from a site manifest (plan) through a derived build to a boot record.

Run with `python3 -I`.  Reads only.

    chain_check.py --plan MANIFEST --build-record FILE --run-meta FILE [--build-root DIR] [--dump-text FILE]
                   [--basis-text FILE] [--main PATH] [-o OUT]

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
  same_build    the GNU build-id of --main under --build-root (readelf -n), recorded beside; the dump of runtime 1.4
                carries no build-id, so this link is always unconfirmed(no build-id in dump) unless the dump text has
                a build_id field, which is then compared.
The chain verdict is refused when any link is refused, else unconfirmed when any of manifest, exe or facts_build is
unconfirmed, else confirmed.  same_build is reported apart (it is unconfirmed for every dump of runtime 1.4).
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
        did = None
        if a.dump_text:
            for line in read(a.dump_text):
                dman += re.findall(r'manifest_sha256=([0-9a-f]{64})', line)
                m = re.search(r'build_id=([0-9a-f]+)', line)
                if m and did is None:
                    did = m.group(1)
        btxt = None
        if a.basis_text:
            for line in read(a.basis_text):
                m = re.match(r'binary\.sha256 = ([0-9a-f]{64})', line)
                if m:
                    btxt = m.group(1)
                    break
    except OSError as e:
        sys.stderr.write('chain_check.py: %s\n' % e)
        return 2

    vals = {'plan': plan, 'build_record': bman, 'run.meta': mman}
    if a.dump_text:
        vals['dump'] = ','.join(sorted(set(dman))) if dman else None
    present = {k: v for k, v in vals.items() if v}
    detail = ' '.join('%s=%s' % (k, (v or 'missing')[:16]) for k, v in vals.items())
    if any(v != plan for v in present.values()):
        link('manifest', 'refused', detail)
    elif len(present) < len(vals):
        link('manifest', 'unconfirmed', detail)
    else:
        link('manifest', 'confirmed', detail)

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
        if mp:
            parts.append('run.meta[%s]=%s' % (mp, mfiles[mp][:16]))
            if mfiles[mp] != h:
                st = 'refused'
        else:
            parts.append('run.meta=no-such-file')
            st = 'unconfirmed' if st == 'confirmed' else st
        link('exe %s' % bpath, st, ' '.join(parts))

    mainp = a.main or (exes[0][0] if exes else None)
    mainh = None
    if mainp:
        mp = match_meta(mfiles, mainp)
        mainh = mfiles.get(mp) if mp else None
    if not a.basis_text:
        link('facts_build', 'unconfirmed', 'no basis of the facts given (main %s run=%s)' % (mainp, (mainh or 'missing')[:16]))
    elif not btxt or not mainh:
        link('facts_build', 'unconfirmed', 'basis binary.sha256=%s run main %s=%s' % ((btxt or 'missing')[:16], mainp, (mainh or 'missing')[:16]))
    elif btxt == mainh:
        link('facts_build', 'confirmed', 'basis binary.sha256=%s = run main %s' % (btxt[:16], mainp))
    else:
        link('facts_build', 'refused', 'basis binary.sha256=%s != run main %s=%s' % (btxt[:16], mainp, mainh[:16]))

    bid = None
    if a.build_root and mainp and os.path.isfile(os.path.join(a.build_root, mainp)):
        bid = build_id(os.path.join(a.build_root, mainp))
    if did is None:
        st = 'unconfirmed(no build-id in dump)'
    elif bid is None:
        st = 'unconfirmed(no build-id in the build tree)'
    else:
        st = 'confirmed' if did == bid else 'refused'
    lines.append('same_build\t%s\tbuild-id of %s in the tree=%s dump=%s' % (st, mainp, bid or 'not read', did or 'none'))

    core = [v for k, v in status.items()]
    verdict = 'refused' if 'refused' in core else ('unconfirmed' if 'unconfirmed' in core else 'confirmed')
    lines.append('chain\t%s\t%d links' % (verdict, len(core)))
    text = '\n'.join(lines) + '\n'
    if a.out:
        with open(a.out, 'w', encoding='utf-8') as f:
            f.write('# chain_check.py --plan %s --build-record %s --run-meta %s --build-root %s --dump-text %s --basis-text %s --main %s\n'
                    % (a.plan, a.build_record, a.run_meta, a.build_root, a.dump_text, a.basis_text, mainp))
            f.write(text)
    sys.stdout.write(text)
    return {'confirmed': 0, 'refused': 1, 'unconfirmed': 3}[verdict]


if __name__ == '__main__':
    sys.exit(main())

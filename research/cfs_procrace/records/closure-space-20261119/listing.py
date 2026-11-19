# listing.py (python3 -I) -- the listing of results 26.0 (k) / 26.2 (1): every directory under the obs1 output root and
# the directories of the work root (named excluded classes are listed without a size), with its size, its ledger rows,
# whether its ledger list names exactly the files on disk, who still reads it, and whether it may be archived by the
# rule written below.  Reads only; writes the one output file.  No row id, key, case name or verdict is an input.
#
#   listing.py --research R --obs1 DIR --work DIR --ledger LEDGER --out OUT
#              --reader TAG=FILE ... --baselines DIR --exclude REL=CLASS ... --expand REL ...
#
# readers: every obs1 directory name that appears as a whole word in FILE is marked "read by TAG".  The files given are
#   the scripts and inputs of the steps still to run (the boot step and the code and freeze steps of results 26).
# baselines: every <name>.sha256 in DIR names the obs1 directory <name> as a comparison baseline of the freeze step
#   (the outputs of the frozen explanation script that the fixed script is compared with, spec 11 item 16).
# reference roots: a ledger row whose local column begins "removed" has as root a kept directory that holds the bytes
#   of a removed one (reclaim convention); such a root is never archived.  --twin NAME adds one (the kept twin a
#   reclaim of this step points to).
# never: --never PATTERN=REASON (fnmatch on the obs1 name) is never archived (the lead's list in the task of this step,
#   and the current round).
# archivable (all must hold): an extract round or an order/relations round; not read by any reader; not a baseline;
#   not a reference root; not never; exactly one ledger row with that root, method sha256c, local "yes"; its list names
#   exactly the regular files on disk; no symbolic link, no hard link, no other file type.
# tier: 1 = extract round (results 26.0 (k): the intermediate extract rounds), 2 = order/relations round of a superseded
#   tool round (the task's "superseded kept rounds"), R = a --reclaim name (recorded earlier as byte-identical to a kept
#   twin; removed by the reclaim convention after a new identity check, not archived), '-' otherwise.  A check output
#   (name ending _check or _keys) is not a round.
import argparse, fnmatch, os, re, stat, sys

ap = argparse.ArgumentParser()
ap.add_argument('--research', required=True)
ap.add_argument('--obs1', required=True)
ap.add_argument('--work', required=True)
ap.add_argument('--ledger', required=True)
ap.add_argument('--out', required=True)
ap.add_argument('--reader', action='append', default=[])
ap.add_argument('--baselines', required=True)
ap.add_argument('--exclude', action='append', default=[])
ap.add_argument('--expand', action='append', default=[])
ap.add_argument('--never', action='append', default=[])
ap.add_argument('--twin', action='append', default=[])
ap.add_argument('--reclaim', action='append', default=[])
a = ap.parse_args()


def walk(d):
    app = blk = nf = nl = nh = no = 0
    files = set()
    for root, dirs, fs in os.walk(d):
        st = os.lstat(root)
        blk += st.st_blocks * 512
        for n in dirs:
            p = os.path.join(root, n)
            if os.path.islink(p):
                nl += 1
        for n in fs:
            p = os.path.join(root, n)
            st = os.lstat(p)
            if stat.S_ISLNK(st.st_mode):
                nl += 1
                continue
            if not stat.S_ISREG(st.st_mode):
                no += 1
                continue
            nf += 1
            app += st.st_size
            blk += st.st_blocks * 512
            if st.st_nlink > 1:
                nh += 1
            files.add('./' + os.path.relpath(p, d))
    return app, blk, nf, nl, nh, no, files


def kind_of(n):
    if 'plans' in n or n == 'plangen':
        return 'plans'
    if re.search(r'(_extract|_onextract)$', n):
        return 'extract'
    if re.search(r'(_order|_relcheck)$', n) or re.fullmatch(r'order_rev1b?', n):
        return 'order'
    if re.search(r'(_check|_keys)$', n):
        return 'check'
    if re.search(r'_(derived|rt|bprime)$', n):
        return 'derived'
    if re.search(r'_(boot|convert)$', n) or 'explain' in n:
        return 'boot'
    if n.startswith('xr4_'):
        return 'explain-dry'
    if 'regress' in n or 'unitcheck' in n or n.endswith('_sens') or n.endswith('_readback') or n.endswith('_bref'):
        return 'regress'
    return 'other'


led = []
for ln in open(a.ledger, encoding='utf-8'):
    f = ln.rstrip('\n').split('\t')
    if not f or f[0].startswith('#') or f[0] == 'id' or len(f) < 10:
        continue
    led.append(f)
rows_by_root, refroots = {}, set()
for f in led:
    r = f[4].rstrip('/')
    rows_by_root.setdefault(r, []).append(f)
    if f[5].startswith('removed'):
        refroots.add(r)
obs1 = a.obs1.rstrip('/')
for t in a.twin:
    refroots.add(obs1 + '/' + t)

names = sorted(n for n in os.listdir(obs1) if os.path.isdir(os.path.join(obs1, n)) and not os.path.islink(os.path.join(obs1, n)))
readers = {}
for spec in a.reader:
    tag, path = spec.split('=', 1)
    txt = open(path, encoding='utf-8', errors='replace').read()
    for n in names:
        if re.search(r'(?<![A-Za-z0-9_])' + re.escape(n) + r'(?![A-Za-z0-9_])', txt):
            readers.setdefault(n, []).append(tag)
baselines = {fn[:-7] for fn in os.listdir(a.baselines) if fn.endswith('.sha256')}


def list_check(f, files):
    if f[2] != 'sha256c':
        return 'method-' + f[2]
    L = f[3] if f[3].startswith('/') else os.path.join(a.research, f[3])
    if not os.path.isfile(L):
        return 'list-missing'
    named = set()
    for ln in open(L, encoding='utf-8'):
        m = re.match(r'^[0-9a-f]{64}  (.*)$', ln.rstrip('\n'))
        if m:
            p = m.group(1)
            named.add(p if p.startswith('./') else './' + p)
    if named == files:
        return 'yes'
    return 'no(missing=%d,extra=%d)' % (len(named - files), len(files - named))


out = ['# kz-space-listing 1 (records/closure-space-20261119/listing.py); sizes from lstat: apparent = sum of file sizes, '
       'blocks = 512 x st_blocks over files and directories (what df counts)',
       '\t'.join(['dir', 'kind', 'apparent', 'blocks', 'files', 'symlinks', 'hardlinks', 'other', 'ledger_rows',
                  'list_names_disk', 'read_by', 'archivable', 'tier', 'reason'])]
for n in names:
    d = obs1 + '/' + n
    app, blk, nf, nl, nh, no, files = walk(d)
    k = kind_of(n)
    rows = rows_by_root.get(d, [])
    live = [f for f in rows if not f[5].startswith('removed')]
    lc = ';'.join(list_check(f, files) for f in live) if live else 'none'
    rb = list(readers.get(n, []))
    if n in baselines:
        rb.append('baseline')
    if d in refroots:
        rb.append('reference-root')
    why = []
    if k not in ('extract', 'order'):
        why.append('not an extract or order round (%s)' % k)
    for spec in a.never:
        p, r = spec.split('=', 1)
        if fnmatch.fnmatch(n, p):
            why.append('never: ' + r)
            break
    if rb:
        why.append('read or referenced: ' + ','.join(rb))
    if len(live) != 1:
        why.append('ledger rows with this root: %d' % len(live))
    elif live[0][5] != 'yes' or live[0][2] != 'sha256c':
        why.append('ledger row not sha256c/local yes')
    elif lc != 'yes':
        why.append('list does not name the files on disk: ' + lc)
    if nl or nh or no:
        why.append('links or other file types')
    ok = not why
    tier = ('R' if n in a.reclaim else '1' if k == 'extract' else '2') if ok else '-'
    out.append('\t'.join(['redesign/obs1/' + n, k, str(app), str(blk), str(nf), str(nl), str(nh), str(no),
                          ','.join(f[0] for f in rows) or '-', lc, ','.join(rb) or '-', 'yes' if ok else 'no', tier,
                          '; '.join(why) or 'intermediate or superseded round, read by nothing still to run']))

work = a.work.rstrip('/')
excl = dict(e.split('=', 1) for e in a.exclude)
expand = set(a.expand)
tops = []
for n in sorted(os.listdir(work)):
    p = os.path.join(work, n)
    if not os.path.isdir(p) or os.path.islink(p):
        continue
    if n in expand:
        for m in sorted(os.listdir(p)):
            q = os.path.join(p, m)
            if os.path.isdir(q) and not os.path.islink(q):
                tops.append(n + '/' + m)
    else:
        tops.append(n)
for rel in tops:
    if rel == 'redesign/obs1':
        continue
    d = work + '/' + rel
    if rel in excl:
        out.append('\t'.join([rel, 'excluded:' + excl[rel]] + ['-'] * 9 + ['no', '-', 'excluded class (' + excl[rel] + ')']))
        continue
    app, blk, nf, nl, nh, no, files = walk(d)
    rows = [f for r, fs in rows_by_root.items() for f in fs if r == d or r.startswith(d + '/')]
    out.append('\t'.join([rel, 'work', str(app), str(blk), str(nf), str(nl), str(nh), str(no),
                          ','.join(f[0] for f in rows) or '-', '-', '-', 'no', '-',
                          'outside the obs1 round sequence: not chosen by the rule (no single ledger row whose list names its files)']))
open(a.out, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
na = sum(1 for l in out[2:] if l.split('\t')[11] == 'yes')
print('listing: %d obs1 directories, %d work directories, %d archivable' % (len(names), len(tops) - 1, na))

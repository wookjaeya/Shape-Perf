# listing.py (python3 -I) --research R --obs O --since-commit C --since-time T --out OUT
# kz-space2-listing 1 (results 26.8; lead ruling 26.7 (나)): the outputs of results 26.3-26.5 under the kept-output
# directory O, and for each whether it may be replaced by an xz tar archive before the C1 boots.  Reads only.
#
# Which directories: every directory of O whose ledger row was added after commit C (the commit that closed results
# 26.2; the row ids of LEDGER.tsv at C against those now), and every directory of O modified at or after time T (UTC, the end of
# results 26.2).  Both sets are printed; a directory in one and not the other is reported.
#
# A directory is archivable when all of these hold (the rule of records/closure-space-20261119/listing.py, narrowed to
# the directories above):
#   1. exactly one ledger row has root O/NAME; its method is sha256c and local is "yes";
#   2. its list names every regular file on disk and nothing else (paths compared in the form ./<path>), and every
#      sha256 of the list equals the file's bytes now;
#   3. no entry other than regular files and directories (no symbolic link, no device, no fifo), no hard link;
#   4. no file of the boot step reads it: boot/run_cycle.sh, boot/commands.txt, boot/declaration3.tsv (the newest
#      boot declaration), boot/test_run_cycle.sh, and the three freeze files (the frozen explanation script's parts and
#      inputs); the test is the text "xr5_" and the exact name in each file (the boot step reads O only through the
#      paths these files write: cz_plans_v4_ns2, c1p2_*, cz_final_extract, cz_final_order, xr4_dry_inputs, c1cyc_*);
#   5. no other ledger row names it in its list or root column (it is no twin of a removed directory).
# The committed scripts that name it (git grep of *.sh and *.py) are listed with the results section that ran them;
# they belong to finished steps (results 26.3 and 26.5) and are not readers still to run.
import argparse
import hashlib
import os
import re
import stat
import subprocess
import sys

ap = argparse.ArgumentParser()
ap.add_argument('--research', required=True)
ap.add_argument('--obs', required=True)
ap.add_argument('--since-commit', required=True)
ap.add_argument('--since-time', required=True)
ap.add_argument('--out', required=True)
a = ap.parse_args()
R = os.path.abspath(a.research)
O = os.path.abspath(a.obs)
REPO = os.path.abspath(os.path.join(R, '..', '..'))
LED = os.path.join(R, 'records', 'LEDGER.tsv')
BOOT = os.path.join(R, 'records', 'closure-c1prep-20261119', 'boot')
READERS = [os.path.join(BOOT, f) for f in ('run_cycle.sh', 'commands.txt', 'declaration3.tsv', 'test_run_cycle.sh')] + [
    os.path.join(R, 'records', d, 'freeze.tsv') for d in
    ('closure-explain-rev4-20261119', 'closure-preboot-20261119', 'closure-prebootfix-20261119')]
SCRIPT_STEP = {
    'research/cfs_procrace/records/closure-explain-fix-20261119/scripts/runs.sh': 'results 26.3 (ran)',
    'research/cfs_procrace/records/closure-prebootfix-20261119/scripts/runs.sh': 'results 26.5 (ran)',
}


def run(cmd, cwd=None):
    p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if p.returncode not in (0, 1):
        sys.stderr.write('listing.py: %s rc %d: %s\n' % (' '.join(cmd), p.returncode, p.stderr))
        sys.exit(2)
    return p.stdout


ledger = []
for ln in open(LED, encoding='utf-8').read().split('\n'):
    f = ln.split('\t')
    if not ln or ln.startswith('#') or f[0] == 'id' or len(f) != 10:
        continue
    ledger.append(f)

# set 1: ledger rows added after C whose root is under O
old_ids = set()
for ln in run(['git', 'show', '%s:%s' % (a.since_commit, os.path.relpath(LED, REPO))], cwd=REPO).split('\n'):
    f = ln.split('\t')
    if ln and not ln.startswith('#'):
        old_ids.add(f[0])
added = [f for f in ledger if f[0] not in old_ids]
set1 = sorted(os.path.basename(f[4]) for f in added if os.path.dirname(f[4]) == O)

# set 2: directories of O modified at or after T
import datetime
t0 = datetime.datetime.strptime(a.since_time, '%Y-%m-%dT%H:%M:%SZ').replace(tzinfo=datetime.timezone.utc).timestamp()
set2 = sorted(n for n in os.listdir(O) if os.path.isdir(os.path.join(O, n)) and not os.path.islink(os.path.join(O, n))
              and os.stat(os.path.join(O, n)).st_mtime >= t0)


def norm(p):
    return p if p.startswith('./') else './' + p


def files_on_disk(d):
    regs, other, hard = [], [], []
    for root, dirs, files in os.walk(d):
        for n in dirs + files:
            p = os.path.join(root, n)
            st = os.lstat(p)
            rel = './' + os.path.relpath(p, d)
            if stat.S_ISDIR(st.st_mode):
                continue
            if stat.S_ISREG(st.st_mode):
                regs.append(rel)
                if st.st_nlink > 1:
                    hard.append(rel)
            else:
                other.append(rel)
    return sorted(regs), other, hard


def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as fh:
        for b in iter(lambda: fh.read(1 << 20), b''):
            h.update(b)
    return h.hexdigest()


reader_text = {p: open(p, encoding='utf-8', errors='replace').read() for p in READERS}
scripts = [s for s in run(['git', 'grep', '-l', 'xr5_', '--', '*.sh', '*.py'], cwd=REPO).split('\n') if s]

rows = []
for name in sorted(set(set1) | set(set2)):
    d = os.path.join(O, name)
    mine = [f for f in ledger if f[4] == d]
    why = []
    if len(mine) != 1:
        why.append('ledger rows with this root: %d' % len(mine))
    row = mine[0] if len(mine) == 1 else None
    if row and (row[2] != 'sha256c' or row[5] != 'yes'):
        why.append('row %s method %s local %s' % (row[0], row[2], row[5]))
    regs, other, hard = files_on_disk(d)
    if other:
        why.append('entries that are not regular files or directories: %d' % len(other))
    if hard:
        why.append('hard links: %d' % len(hard))
    nlist, eq, bad = '-', '-', '-'
    if row:
        L = row[3] if row[3].startswith('/') else os.path.join(R, row[3])
        lst = [ln.split('  ', 1) for ln in open(L, encoding='utf-8').read().split('\n') if ln]
        nlist = len(lst)
        listed = sorted(norm(p) for _, p in lst)
        eq = 'yes' if listed == regs else 'no'
        if eq == 'no':
            why.append('the list does not name exactly the files on disk')
        bad = sum(1 for h, p in lst if not os.path.isfile(os.path.join(d, norm(p))) or sha(os.path.join(d, norm(p))) != h)
        if bad:
            why.append('files whose bytes differ from the list: %d' % bad)
    pat = re.compile(re.escape(name) + r'(?![A-Za-z0-9_])')
    rd = []
    for p, t in reader_text.items():
        k = len(pat.findall(t)) + t.count('xr5_')
        if k:
            rd.append('%s:%d' % (os.path.relpath(p, R), k))
    if rd:
        why.append('named by a file of the boot step or a freeze file')
    others = [f[0] for f in ledger if f is not row and (pat.search(f[3]) or pat.search(f[4]))]
    if others:
        why.append('named by the list or root of other ledger rows: %s' % ','.join(others))
    sc = []
    for s in scripts:
        t = open(os.path.join(REPO, s), encoding='utf-8', errors='replace').read()
        if pat.search(t) or ('xr5_synth_' in t and name.startswith('xr5_synth_')) or ('xr5_dry_' in t and name.startswith('xr5_dry_')):
            sc.append('%s [%s]' % (os.path.relpath(os.path.join(REPO, s), R), SCRIPT_STEP.get(s, 'not known')))
    if any('not known' in s for s in sc):
        why.append('named by a committed script of no finished step')
    ab = int(run(['du', '-sb', d]).split()[0])
    bb = int(run(['du', '-sB1', d]).split()[0])
    rows.append([name, row[0] if row else '-', row[3] if row else '-', str(len(regs)), str(nlist), eq, str(bad),
                 str(len(other)), str(len(hard)), str(ab), str(bb), ';'.join(rd) or '-', ';'.join(others) or '-',
                 ';'.join(sc) or '-', 'no' if why else 'yes', '; '.join(why) or 'every condition holds',
                 'set1' if name in set1 and name not in set2 else 'set2' if name in set2 and name not in set1 else 'both'])

with open(a.out, 'w', encoding='utf-8') as out:
    out.write('# kz-space2-listing 1 (records/closure-space2-20261119/listing.py); %s\n' %
              datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'))
    out.write('# set1 (ledger rows added after %s with root under %s): %d: %s\n' % (a.since_commit, O, len(set1), ' '.join(set1)))
    out.write('# set2 (directories of %s modified at or after %s): %d: %s\n' % (O, a.since_time, len(set2), ' '.join(set2)))
    out.write('# in one set only: %s\n' % (' '.join(sorted(set(set1) ^ set(set2))) or 'none'))
    out.write('# readers tested (condition 4): %s\n' % ' '.join(os.path.relpath(p, R) for p in READERS))
    out.write('\t'.join(['name', 'ledger_row', 'list', 'files_on_disk', 'list_lines', 'list_eq_disk', 'bytes_differ',
                         'other_entries', 'hard_links', 'apparent_bytes', 'block_bytes', 'boot_or_freeze_readers',
                         'other_ledger_rows', 'scripts_naming', 'archivable', 'why', 'found_by']) + '\n')
    for r in rows:
        out.write('\t'.join(r) + '\n')
    tb = sum(int(r[10]) for r in rows if r[14] == 'yes')
    ta = sum(int(r[9]) for r in rows if r[14] == 'yes')
    out.write('# archivable %d of %d; apparent %d B, blocks %d B\n' % (sum(1 for r in rows if r[14] == 'yes'), len(rows), ta, tb))
print('listing: %d directories, %d archivable' % (len(rows), sum(1 for r in rows if r[14] == 'yes')))

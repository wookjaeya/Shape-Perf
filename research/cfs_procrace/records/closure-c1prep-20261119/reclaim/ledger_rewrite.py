# ledger_rewrite.py (python3 -I) LEDGER OUT_LEDGER -- the ledger rows of the four option-off extract rounds replaced by
# xz tar archives (archive.sh): method "parts", root the archive directory (its PARTS.sha256 names the archive's
# sha256), the list column still the directory's member list (the sha256 of every member, checked file by file in
# the round trip before the removal), local and verified_copy saying so, the note extended.  A header line states the
# convention.  Every other line is copied unchanged.  rc 0, or 1 when a row is not found or not in its old form.
import sys

LED, OUT = sys.argv[1], sys.argv[2]
ROWS = {
    'OBS_objacc_off_extract': 'objacc_off_extract',
    'OBS_sem_off_extract': 'sem_off_extract',
    'OBS_rf_off_extract': 'rf_off_extract',
    'OBS_ll_off_extract': 'll_off_extract',
}
A = '/home/user/work/preserve/archives-20261119/'
REC = 'records/closure-c1prep-20261119/reclaim/'
HEAD = ('#   A directory replaced by a compressed archive (first use 2026-11-19, records/closure-c1prep-20261119/reclaim; '
        'results 12.10 item 2 allows it for the option-off extract rounds and the intermediate option-on rounds only): '
        'method parts, root = the archive directory (PARTS.sha256 names the archive\'s sha256, MEMBERS.sha256 is a copy '
        'of the list), list = the directory\'s member list, checked file by file against a stream of the archive before '
        'the directory was removed; local = "replaced <date> by an xz tar archive"; verified_copy names the round trip.')
lines = open(LED, encoding='utf-8').read().split('\n')
out, seen, hdr = [], set(), False
for ln in lines:
    f = ln.split('\t')
    if not hdr and ln.startswith('id\t'):
        out.append(HEAD)
        hdr = True
    if f and f[0] in ROWS:
        n = ROWS[f[0]]
        if len(f) != 10 or f[2] != 'sha256c' or f[4] != '/home/user/work/redesign/obs1/' + n or f[5] != 'yes':
            sys.stderr.write('row %s is not in its old form\n' % f[0])
            sys.exit(1)
        f[2] = 'parts'
        f[4] = A + n
        f[5] = 'replaced 2026-11-19 by an xz tar archive (%s%s/%s.tar.xz)' % (A, n, n)
        f[6] = 'round trip file by file 2026-11-19 (%sroundtrip_%s.txt)' % (REC, n)
        f[9] = f[9] + ('; directory /home/user/work/redesign/obs1/%s replaced 2026-11-19 by the archive (space for the C1 '
                       'cycle preparation, results 25; %sroundtrip_%s.txt)' % (n, REC, n))
        ln = '\t'.join(f)
        seen.add(f[0])
    out.append(ln)
if seen != set(ROWS) or not hdr:
    sys.stderr.write('rows not found: %s\n' % sorted(set(ROWS) - seen))
    sys.exit(1)
open(OUT, 'w', encoding='utf-8').write('\n'.join(out))
print('rewritten rows: %d' % len(seen))

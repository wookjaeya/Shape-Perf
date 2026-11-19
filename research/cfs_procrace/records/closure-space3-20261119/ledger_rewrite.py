# ledger_rewrite.py (python3 -I) LEDGER OUT archive ROW NAME -- rewrite one ledger row of results 26.9 by the
# compressed-archive convention already in the ledger header (first use 2026-11-19, records/closure-c1prep-20261119/
# reclaim; second use records/closure-space-20261119; third use records/closure-space2-20261119).  A copy of
# records/closure-space2-20261119/ledger_rewrite.py with this step's record directory, results section and header line:
#   archive  (the directory /home/user/work/redesign/obs1/NAME was replaced by archive.sh; its round trip passed):
#            method parts; root = /home/user/work/preserve/archives-20261119/NAME (PARTS.sha256 names the archive's
#            sha256, MEMBERS.sha256 is a copy of the list); the list column unchanged (the directory's member list,
#            checked file by file against a stream of the archive before the removal); local "replaced 2026-11-19 by
#            an xz tar archive (<archive>)"; verified_copy names the round-trip file; the note extended.
# The first use adds one header line after the third-use line stating the scope of this step (once).
# The row must be in its old form (10 columns, sha256c, root the obs1 directory NAME, local "yes"), else rc 1 and
# nothing is written.  Every other line is copied unchanged.  Prints one line.
import sys

if len(sys.argv) != 6 or sys.argv[3] != 'archive':
    sys.stderr.write('usage: ledger_rewrite.py LEDGER OUT archive ROW NAME\n')
    sys.exit(64)
LED, OUT, ACT, ROW, NAME = sys.argv[1:6]
O = '/home/user/work/redesign/obs1/'
A = '/home/user/work/preserve/archives-20261119/'
REC = 'records/closure-space3-20261119/'
HEAD = ('#   Fourth use of the compressed archive, 2026-11-19 (records/closure-space3-20261119; results 26.9, review findings '
        'fix-judge-02 and fix-decl-03, the per-boot need restated): lines 9-12 of records/closure-space2-20261119/chosen.tsv '
        '(the static dry runs, the regression and the candidate re-run of results 26.3), with the same columns as above.')
ANCHOR = '#   Third use of the compressed archive, 2026-11-19'
lines = open(LED, encoding='utf-8').read().split('\n')
out, seen, have_head, anchor = [], 0, any(l == HEAD for l in lines), False
for ln in lines:
    f = ln.split('\t')
    if f[0] == ROW:
        if len(f) != 10 or f[2] != 'sha256c' or f[4] != O + NAME or f[5] != 'yes':
            sys.stderr.write('row %s is not in its old form\n' % ROW)
            sys.exit(1)
        f[2] = 'parts'
        f[4] = A + NAME
        f[5] = 'replaced 2026-11-19 by an xz tar archive (%s%s/%s.tar.xz)' % (A, NAME, NAME)
        f[6] = 'round trip file by file 2026-11-19 (%sarchive/roundtrip_%s.txt)' % (REC, NAME)
        f[9] = f[9] + ('; directory %s%s replaced 2026-11-19 by the archive (space for the C1 boots, results 26.9; '
                       '%sarchive/roundtrip_%s.txt)' % (O, NAME, REC, NAME))
        ln = '\t'.join(f)
        seen += 1
    out.append(ln)
    if not have_head and ln.startswith(ANCHOR):
        out.append(HEAD)
        anchor = True
if seen != 1 or not (have_head or anchor):
    sys.stderr.write('ledger_rewrite.py: row %s found %d times; header anchor %s\n' % (ROW, seen, have_head or anchor))
    sys.exit(1)
open(OUT, 'w', encoding='utf-8').write('\n'.join(out))
print('%s %s %s: row rewritten%s' % (ACT, ROW, NAME, '' if have_head else '; header line added'))

# ledger_rewrite.py (python3 -I) LEDGER OUT ACTION ROW NAME [TWIN_ROW TWIN_NAME FILES BYTES] -- rewrite one ledger row
# of results 26.2, by the two conventions already in the ledger header (written there before this step):
#   archive  (the directory /home/user/work/redesign/obs1/NAME was replaced by archive.sh; its round trip passed):
#            method parts; root = /home/user/work/preserve/archives-20261119/NAME (PARTS.sha256 names the archive's
#            sha256, MEMBERS.sha256 is a copy of the list); the list column unchanged (the directory's member list,
#            checked file by file against a stream of the archive before the removal); local "replaced 2026-11-19 by
#            an xz tar archive (<archive>)"; verified_copy names the round-trip file; the note extended.
#   reclaim  (the directory is byte-identical to the kept directory TWIN_NAME, file by file at the same paths, by the
#            identity listing of this step; the convention of records/reclaim-20261118): root = the twin directory, so
#            the check still reads every byte the list names; local "removed 2026-11-19; bytes in TWIN_ROW"; the note
#            names the removed directory, its files and bytes, the twin row and the listing, then the old note.
# The first use adds one header line after the existing compressed-archive line stating the scope of this step (once).
# The row must be in its old form (10 columns, sha256c, root the obs1 directory NAME, local "yes"), else rc 1 and
# nothing is written.  Every other line is copied unchanged.  Prints one line.
import sys

if len(sys.argv) not in (6, 10):
    sys.stderr.write('usage: ledger_rewrite.py LEDGER OUT archive|reclaim ROW NAME [TWIN_ROW TWIN_NAME FILES BYTES]\n')
    sys.exit(64)
LED, OUT, ACT, ROW, NAME = sys.argv[1:6]
O = '/home/user/work/redesign/obs1/'
A = '/home/user/work/preserve/archives-20261119/'
REC = 'records/closure-space-20261119/'
HEAD = ('#   Second use of the compressed archive, 2026-11-19 (records/closure-space-20261119; results 26.0 (k) and 26.2): '
        'the kept extract rounds and the order and relations rounds of the tool rounds before the current one '
        '(cz_*) that no step still to run reads (listing.tsv and chosen.tsv there), with the same columns as above; '
        'the byte-identical directories of that step follow the 2026-11-18 convention above.')
ANCHOR = '#   A directory replaced by a compressed archive (first use 2026-11-19'
if ACT not in ('archive', 'reclaim') or (ACT == 'reclaim') != (len(sys.argv) == 10):
    sys.stderr.write('ledger_rewrite.py: archive takes no twin; reclaim takes TWIN_ROW TWIN_NAME FILES BYTES\n')
    sys.exit(64)
lines = open(LED, encoding='utf-8').read().split('\n')
out, seen, have_head, anchor = [], 0, any(l == HEAD for l in lines), False
for ln in lines:
    f = ln.split('\t')
    if f[0] == ROW:
        if len(f) != 10 or f[2] != 'sha256c' or f[4] != O + NAME or f[5] != 'yes':
            sys.stderr.write('row %s is not in its old form\n' % ROW)
            sys.exit(1)
        if ACT == 'archive':
            f[2] = 'parts'
            f[4] = A + NAME
            f[5] = 'replaced 2026-11-19 by an xz tar archive (%s%s/%s.tar.xz)' % (A, NAME, NAME)
            f[6] = 'round trip file by file 2026-11-19 (%sarchive/roundtrip_%s.txt)' % (REC, NAME)
            f[9] = f[9] + ('; directory %s%s replaced 2026-11-19 by the archive (space for the C1 boots, results 26.2; '
                           '%sarchive/roundtrip_%s.txt)' % (O, NAME, REC, NAME))
        else:
            trow, tname, nfiles, nbytes = sys.argv[6:10]
            f[4] = O + tname
            f[5] = 'removed 2026-11-19; bytes in %s' % trow
            f[9] = ('directory %s%s removed 2026-11-19 (%s files, %s bytes): every file is byte-identical to the file of '
                    '%s at the same path (%sreclaim/listing_before_%s.txt); this row\'s list is checked in that directory. '
                    'Before the removal: %s' % (O, NAME, nfiles, nbytes, trow, REC, NAME, f[9]))
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

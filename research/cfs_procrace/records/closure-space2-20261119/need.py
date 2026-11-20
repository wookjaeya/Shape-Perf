# need.py (python3 -I) --research R --listing listing.tsv --free BYTES --out need.txt --chosen chosen.tsv
# kz-space2-need 1 (results 26.8; lead ruling 26.7 (나)): the need, the order and the expected free space after each line.
#   need: read from the header of the newest boot declaration (records/closure-c1prep-20261119/boot/declaration3.tsv,
#         the line "... B free is needed before boot 1"); its parts are restated as that header gives them.
#   free: the value of df -B1 / given on the command line (measured just before the run).
#   order: ORDER below, fixed here before any archive: the superseded first trial, then the evaluation outputs of an
#         unfrozen source snapshot, then the four synthetic records (test input, never evidence), then the two runtime
#         test dump dry runs, the two static dry runs, the regression on the 2026-11-16 inputs, and last the candidate
#         re-run (byte-identical to a committed directory; the smallest gain).  The ones a re-run after a fix found by
#         the review of results 26.5 (lead ruling 26.7 (라)) would compare against first come last.
#   expected gain [I]: blocks x (1 - ratio), the ratio the largest archive bytes / directory blocks of the 28 archives
#         of results 26.2 (records/closure-space-20261119/progress.tsv and chosen.tsv); the content here is JSON
#         explanation records, not facts and order rounds, so the ratio is a guess and the run measures the real one.
# Only directories the listing marks archivable are taken.  Prints one line.
import argparse
import re
import os
import sys

ORDER = [
    ('xr5_synth_c1_pre_tx_script_trial1', 'the first trial of the synthetic generator, superseded by xr5_synth_c1_pre_tx_script'),
    ('xr5_eval_dry', 'explain_eval.py of the unfrozen source snapshot dc879c6 on the six dry runs; superseded by the re-evaluations of results 26.5 (heads and tails committed)'),
    ('xr5_synth_c1_pre_tx_reverse', 'synthetic use events (test input, never evidence); compared in results 26.4 and 26.5'),
    ('xr5_synth_c1_pre_tx_script', 'synthetic use events (test input, never evidence); compared in results 26.4 and 26.5'),
    ('xr5_synth_c1_post_tx_reverse', 'synthetic use events (test input, never evidence); compared in results 26.4 and 26.5'),
    ('xr5_synth_c1_post_tx_script', 'synthetic use events (test input, never evidence); compared in results 26.4 and 26.5'),
    ('xr5_dry_rt_rt_objects', 'dry run on a runtime step test dump (judge rc 3 build_mismatch by contract); compared in results 26.5'),
    ('xr5_dry_rt_rt_format15', 'dry run on a runtime step test dump (judge rc 3 build_mismatch by contract); compared in results 26.5'),
    ('xr5_dry_static_c1_pre_tx', 'static dry run with the 16 branches, no run; compared in results 26.5'),
    ('xr5_dry_static_c1_post_tx', 'static dry run with the 16 branches, no run; compared in results 26.5'),
    ('xr5_regress_rev4fix', 'the regression on the 2026-11-16 inputs (8 D1 branches); compared in results 26.5'),
    ('xr5_candidates_regen', 'candidates.py re-run: 64 files byte-identical to records/closure-c1prep-20261119/candidates'),
]

ap = argparse.ArgumentParser()
ap.add_argument('--research', required=True)
ap.add_argument('--listing', required=True)
ap.add_argument('--free', required=True, type=int)
ap.add_argument('--out', required=True)
ap.add_argument('--chosen', required=True)
a = ap.parse_args()
R = os.path.abspath(a.research)
FLOOR = 314572800

decl = open(os.path.join(R, 'records/closure-c1prep-20261119/boot/declaration3.tsv'), encoding='utf-8').read()
hdr = ' '.join(l[1:].strip() for l in decl.split('\n') if l.startswith('#'))
m = re.search(r'per boot ([0-9,]+) B,\s+12 boots ([0-9,]+) B and a margin of ([0-9,]+) B above the floor ([0-9,]+) B: '
              r'([0-9,]+) B free is needed\s+before boot 1', hdr)
if not m:
    sys.stderr.write('need.py: the space line of declaration3.tsv not found\n')
    sys.exit(2)
per_boot, boots, margin, floor, need = (int(x.replace(',', '')) for x in m.groups())
if floor != FLOOR or need != floor + boots + margin or boots != 12 * per_boot:
    sys.stderr.write('need.py: the declaration\'s parts do not add up\n')
    sys.exit(2)

old = os.path.join(R, 'records/closure-space-20261119')
blocks = {}
for ln in open(os.path.join(old, 'chosen.tsv'), encoding='utf-8'):
    f = ln.rstrip('\n').split('\t')
    if len(f) == 7 and f[0].isdigit():
        blocks[f[2].split('/')[-1]] = int(f[4])
ratios = []
for ln in open(os.path.join(old, 'progress.tsv'), encoding='utf-8'):
    f = ln.rstrip('\n').split('\t')
    if f[0].isdigit():
        ratios.append((int(f[5]) / blocks[f[1]], f[1], int(f[5]), blocks[f[1]]))
rmax = max(ratios)

lst = {}
for ln in open(a.listing, encoding='utf-8'):
    f = ln.rstrip('\n').split('\t')
    if ln.startswith('#') or f[0] == 'name':
        continue
    lst[f[0]] = f
if sorted(lst) != sorted(n for n, _ in ORDER):
    sys.stderr.write('need.py: the listing and ORDER name different directories\n')
    sys.exit(2)

L = ['# kz-space2-need 1 (records/closure-space2-20261119/need.py); [I] except the inputs, which are [M] in kept files',
     'floor %d' % floor,
     'per_boot %d (records/closure-space-20261119/need.txt, as declaration3.tsv restates it)' % per_boot,
     'boots_total %d (12 boots)' % boots,
     'margin %d' % margin,
     'need_free %d (records/closure-c1prep-20261119/boot/declaration3.tsv header: free before boot 1)' % need,
     'free_now %d' % a.free,
     'short_now %d' % max(0, need - a.free),
     'archive_ratio_used %.6f (the largest of the 28 archives of results 26.2: %s, %d / %d)' % (rmax[0], rmax[1], rmax[2], rmax[3]),
     'stop rule: before each line, if df -B1 / >= need_free, stop (the line and those after it are not touched)']
C = ['# kz-space2-chosen 1: the directories this step may archive, in this order (need.py ORDER); run_seq.sh stops before '
     'the first line at which the measured free space has reached need_free (need.txt); the lines after the stop stay '
     'in this list for a later step that finds the space short at its start (protocol 14.9), by the same method',
     '\t'.join(['seq', 'tier', 'dir', 'ledger_row', 'blocks', 'expected_gain', 'expected_free_after', 'why'])]
free = a.free
met = None
for i, (n, why) in enumerate(ORDER, 1):
    f = lst[n]
    if f[14] != 'yes':
        sys.stderr.write('need.py: %s is not archivable in the listing\n' % n)
        sys.exit(2)
    b = int(f[10])
    gain = int(b * (1 - rmax[0]))
    free += gain
    if met is None and free >= need:
        met = i
    C.append('\t'.join([str(i), 'A', 'redesign/obs1/' + n, f[1], str(b), str(gain), str(free), why]))
L.append('chosen %d directories, blocks %d; expected free after all %d (expected gain %d)' %
         (len(ORDER), sum(int(lst[n][10]) for n, _ in ORDER), free, free - a.free))
L.append('expected: need met after seq %s' % (met if met else 'none (short by %d)' % (need - free)))
open(a.out, 'w', encoding='utf-8').write('\n'.join(L) + '\n')
open(a.chosen, 'w', encoding='utf-8').write('\n'.join(C) + '\n')
print('need %d, free %d, short %d; expected need met after seq %s' % (need, a.free, max(0, need - a.free), met))

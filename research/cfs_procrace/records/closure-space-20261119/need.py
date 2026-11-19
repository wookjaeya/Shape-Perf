# need.py (python3 -I) -- the space need of results 26.0 (k) / 26.2 (2), every term read from a kept file, and the
# ordered list of directories this step may touch (from listing.tsv), with the expected free space after each.
#
#   need.py --research R --listing listing.tsv --free BYTES --out need.txt --chosen chosen.tsv
#
# need above the floor = B x per_boot + code_and_freeze + margin, where
#   per_boot = boot + convert + explain16 + records                                         [I] (one boot of 25.12)
#     boot      the dip of the 2026-11-16 boot of the same C1 build (records/obs1-20261116-boot/df_boot.log)
#     convert   the blocks of that boot's convert and text directory (obs1/c1pre_convert, from listing.tsv)
#     explain16 16 x (dip of the frozen script's run on that boot's record, D1 eight branches) / 8
#               (closure-explain-rev4-20261119/guard/g06_regress_rev4_2.log); the record-bearing measure is used
#               because a boot's records carry use events; the static 16-branch dip is printed beside it
#     records   the blocks of the two record directories of that boot (boot step and explanation step)
#   code_and_freeze = the runs of results 26.0 (b) and spec 11 item 16 as measured on their earlier counterparts:
#     fixtures (scratch, removed at the end: g09), the regression (g06), the dry runs on the two real plans with
#     synthetic use events in two orders (2 x the two static dry runs g07), the runtime-dump dry runs (g08)
#   margin = 100,000,000 B (results 26.0 (k); evaluation protocol 14.9)
# The archive of a directory is expected to take at most the largest ratio (archive bytes / directory blocks) of the
# four archives of f9ce046 (their round-trip files); the measured value replaces it after each archive.
import argparse, os, re, subprocess

ap = argparse.ArgumentParser()
ap.add_argument('--research', required=True)
ap.add_argument('--listing', required=True)
ap.add_argument('--free', type=int, required=True)
ap.add_argument('--out', required=True)
ap.add_argument('--chosen', required=True)
a = ap.parse_args()
R = a.research
FLOOR, BOOTS, MARGIN = 314572800, 12, 100000000


def dip(rel):
    for ln in open(os.path.join(R, rel), encoding='utf-8'):
        if ln.startswith('end '):
            m = re.search(r' dip=(\d+)', ln)
            if m:
                v = int(m.group(1))
    return v


def blocks_of(path):
    t = 0
    for root, dirs, fs in os.walk(path):
        t += os.lstat(root).st_blocks * 512
        for n in fs:
            t += os.lstat(os.path.join(root, n)).st_blocks * 512
    return t


lst = {}
for ln in open(a.listing, encoding='utf-8'):
    f = ln.rstrip('\n').split('\t')
    if ln.startswith('#') or f[0] == 'dir':
        continue
    lst[f[0]] = f

G = 'records/closure-explain-rev4-20261119/guard/'
P = 'records/closure-c1prep-20261119/guard/'
terms = []
boot = dip('records/obs1-20261116-boot/df_boot.log')
convert = int(lst['redesign/obs1/c1pre_convert'][3])
reg8 = dip(G + 'g06_regress_rev4_2.log')
explain16 = reg8 * 16 // 8
static16 = dip(P + 's_explaincheck2_pre.log')
records = blocks_of(os.path.join(R, 'records/obs1-20261116-boot')) + blocks_of(os.path.join(R, 'records/obs1-20261116-reviewfix'))
per_boot = boot + convert + explain16 + records
per_boot_static = boot + convert + static16 + records
fx = dip(G + 'g09_fixtures.log')
rg = dip(G + 'g06_regress_rev4_2.log')
st = 2 * (dip(G + 'g07_static_c1_pre_tx_2.log') + dip(G + 'g07_static_c1_post_tx_2.log'))
rt = dip(G + 'g08_rt_rt_objects_2.log') + dip(G + 'g08_rt_rt_format15_2.log')
code = fx + rg + st + rt
need = BOOTS * per_boot + code + MARGIN
need_static = BOOTS * per_boot_static + code + MARGIN
target = FLOOR + need

ratios = []
for n in ('objacc_off_extract', 'sem_off_extract', 'rf_off_extract', 'll_off_extract'):
    txt = open(os.path.join(R, 'records/closure-c1prep-20261119/reclaim/roundtrip_%s.txt' % n), encoding='utf-8').read()
    b = int(re.search(r' (\d+) bytes \(blocks\)', txt).group(1))
    z = int(re.search(r'\.tar\.xz (\d+) bytes', txt).group(1))
    ratios.append((z / b, n, z, b))
rmax = max(ratios)[0]

L = []
L.append('# kz-space-need 1 (records/closure-space-20261119/need.py); all [I] except the inputs, which are [M] in kept files')
L.append('floor %d' % FLOOR)
L.append('free_now %d' % a.free)
L.append('per_boot.boot %d (dip, records/obs1-20261116-boot/df_boot.log)' % boot)
L.append('per_boot.convert %d (blocks of obs1/c1pre_convert)' % convert)
L.append('per_boot.explain16 %d (16 x %d / 8; %sg06_regress_rev4_2.log)' % (explain16, reg8, G))
L.append('per_boot.explain16_static %d (beside it, not used: %ss_explaincheck2_pre.log)' % (static16, P))
L.append('per_boot.records %d (blocks of records/obs1-20261116-boot and records/obs1-20261116-reviewfix)' % records)
L.append('per_boot %d' % per_boot)
L.append('boots %d' % BOOTS)
L.append('boots_total %d' % (BOOTS * per_boot))
L.append('code.fixtures %d (scratch, removed at the end; g09_fixtures.log)' % fx)
L.append('code.regression %d (g06_regress_rev4_2.log)' % rg)
L.append('code.dry_static %d (2 orders x (g07 c1_pre_tx + g07 c1_post_tx))' % st)
L.append('code.dry_rt %d (g08 rt_objects + g08 rt_format15)' % rt)
L.append('code_total %d' % code)
L.append('margin %d' % MARGIN)
L.append('need_above_floor %d' % need)
L.append('target_free %d' % target)
L.append('to_reclaim_now %d' % max(0, target - a.free))
L.append('beside: with the static 16-branch measure per boot the need would be %d (target %d)' % (need_static, FLOOR + need_static))
for r, n, z, b in sorted(ratios):
    L.append('archive_ratio %s %.6f (%d / %d)' % (n, r, z, b))
L.append('archive_ratio_used %.6f' % rmax)

order = []
for tier in ('R', '1', '2'):
    rows = sorted((f for f in lst.values() if f[11] == 'yes' and f[12] == tier), key=lambda f: (-int(f[3]), f[0]))
    order += rows
C = ['# kz-space-chosen 1: the directories this step may touch, in this order; it stops at the first point where the '
     'measured free space reaches target_free (need.txt); R reclaim by identity (results 26.0 (k)), then tier 1 archive '
     '(extract rounds), then tier 2 archive (order and relations rounds of superseded tool rounds)',
     '\t'.join(['seq', 'tier', 'dir', 'ledger_row', 'blocks', 'expected_gain', 'expected_free_after'])]
free = a.free
met_at = None
for i, f in enumerate(order, 1):
    b = int(f[3])
    gain = b if f[12] == 'R' else int(b * (1 - rmax))
    free += gain
    if met_at is None and free >= target:
        met_at = i
    C.append('\t'.join([str(i), f[12], f[0], f[8], str(b), str(gain), str(free)]))
L.append('chosen %d directories (tier 1 %d, R %d, tier 2 %d); expected free after all %d' % (
    len(order), sum(1 for f in order if f[12] == '1'), sum(1 for f in order if f[12] == 'R'),
    sum(1 for f in order if f[12] == '2'), free))
L.append('expected: need met after seq %s' % (met_at if met_at else 'none (expected shortfall %d)' % (target - free)))
open(a.out, 'w', encoding='utf-8').write('\n'.join(L) + '\n')
open(a.chosen, 'w', encoding='utf-8').write('\n'.join(C) + '\n')
print('\n'.join(L))

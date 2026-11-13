#!/usr/bin/env bash
# w4w5_r2_summary.sh <out root> -- the machine-readable summary of the W4/W5 R2 package (w4w5_r2.sh outputs), pilot/dev:
# <out>/summary.tsv with the columns section, record, reading, item, value, and <out>/sites_matrix.tsv (calls per owner
# application record, site, EventID and AppID passed, one column per record).  Reads only the package's own outputs, the
# run metadata of the four records and their console logs.
#   section run      execution path and record capture per record (run metadata, console lines, replay header lines)
#   section class    occurrences per class and reading: appid0_CFE_ES, appid0_CFE_EVS (AppID 0 passed), sb_self (the SB
#                    task with its own AppID), other_sb / other_tbl (other tasks with the SB / TBL AppID), with the verdict,
#                    summary view, basis kinds and assumptions as kz occ show prints them
#   section relation the replay's summary counts of the execution relation
#   section task     calls of CFE_EVS_SendEventWithAppID per owner application (evs.send.entry), total and by EventID
#   section diff     kz occ diff head lines per pair and reading (exit code 3 = differences or ambiguous keys)
#   section recorded the numbers recorded before (STATUS_20261027.md section 3.1), for the side-by-side table
set -uo pipefail
O=${1:?out root}
python3 -I - "$O" <<'EOF'
import sys, os, re, collections
O = sys.argv[1]
recs = ['p3', 'f0b', 'd4gi', 'gi']
meta = {'p3': '/home/user/work/procrace/r2_runs/v153_pilot3/v153_pilot3_perf_operational_esrestart.log',
        'd4gi': '/home/user/work/instr/gcc/d4/r2/run1/console.log',
        'f0b': '/home/user/work/w5/run/r2/f0b/w5_r2_f0b_perf_operational_esrestart.log',
        'gi': '/home/user/work/w5/run/r2/gi/console.log'}
rows = []
def add(*a): rows.append('\t'.join(str(x) for x in a))
names = {}
for l in open(os.path.join(O, 'p3', 'cur.txt')):
    m = re.search(r"record (\d+) app '([A-Z_]+)'", l)
    if m: names[m.group(1)] = m.group(2)
for r in recs:
    md = dict(l.rstrip('\n').split('=', 1) for l in open(meta[r] + '.meta') if '=' in l and not l.startswith(' '))
    for k in ('command', 'uname', 'core-cpu1 sha256', 'keyfiles_in_cwd_before', 'operational_line_read_ms', 'stop_actions',
              'process_end_ms', 'stop_reason', 'exit_status', 'console.reset_type_at_start', 'console.exit_reset_status'):
        add('run', r, '-', k, md.get(k, '').strip())
    con = open(meta[r], errors='replace').read().splitlines()
    for pat, item in ((r'CFE_PSP: Reset Type:', 'console.reset_type_option'), (r'Starting the cFE with a', 'console.start'),
                      (r'SetupResetVariables:', 'console.es_reset'), (r'entering OPERATIONAL', 'console.operational'),
                      (r'CFE_ES_ResetCFE:', 'console.es_restart'), (r'Exiting cFE with', 'console.exit')):
        hit = [l for l in con if re.search(pat, l)]
        add('run', r, '-', item, hit[0].strip() if hit else 'absent')
    txt = open(os.path.join(O, r, 'cur.txt')).read().splitlines()
    for pre in ('raw events', 'threads', 'app_owner levels', 'core status of owners', 'loss ', 'start at'):
        hit = [l for l in txt if l.startswith(pre)]
        add('run', r, '-', 'replay.' + pre.strip().replace(' ', '_'), hit[0] if hit else 'absent')
    for g in ('cur', 'p1off', 'nocode'):
        c = collections.Counter()
        for l in open(os.path.join(O, 'occ', 'show_%s_%s.txt' % (r, g))):
            if not l.startswith('#'): continue
            f = [x.strip() for x in l.split(' | ')]
            owner = re.match(r'app\[(\d+)\]', f[2]); owner = owner.group(1) if owner else '-'
            res = f[3]
            cls = ('appid0_' + names.get(owner, owner)) if res == '0x0' else 'sb_self' if (owner == '3' and res == '0x110003') \
                else 'other_sb' if res == '0x110003' else 'other_tbl' if res == '0x110004' else 'other_' + res
            m = re.match(r'(.*) basis (\{.*?\}) assume (\{.*?\}) outcome (\S+)', f[6] + ' ' + f[7])
            c[(cls, f[6], f[7])] += 1
        for (cls, verdict, basis), n in sorted(c.items()):
            add('class', r, g, cls, '%d\t%s\t%s' % (n, verdict, basis))
        t = open(os.path.join(O, r, g + '.txt')).read().splitlines()
        blk = False
        for l in t:
            if l.startswith('summary counts'):
                if blk: break
                blk = True; continue
            if blk and l.startswith('  relation ') and not l.startswith('  relation/') and not l.startswith('  relation with'):
                add('relation', r, g, l.split()[1], l.split()[2])
# calls per owner application, site, EventID, AppID
mat = collections.defaultdict(lambda: [0] * 4); keys = set()
for i, r in enumerate(recs):
    for l in open(os.path.join(O, r, 'sbsites.tsv')):
        rec, site, eid, app, n = l.rstrip('\n').split('\t')
        mat[(int(rec) if rec.isdigit() else 999, site, int(eid), app)][i] += int(n)
with open(os.path.join(O, 'sites_matrix.tsv'), 'w') as w:
    w.write('rec\tapp\tsite\tEventID\tAppID\t' + '\t'.join(recs) + '\n')
    for k in sorted(mat):
        w.write('%s\t%s\t%s\t%d\t%s\t%s\n' % (k[0], names.get(str(k[0]), '-'), k[1], k[2], k[3], '\t'.join(map(str, mat[k]))))
for i, r in enumerate(recs):
    tot = collections.Counter(); byeid = collections.defaultdict(collections.Counter)
    for k, v in mat.items():
        if k[1] != 'evs_send_entry': continue
        tot[k[0]] += v[i]; byeid[k[0]]['%d/%s' % (k[2], k[3])] += v[i]
    for rec in sorted(tot):
        add('task', r, '-', names.get(str(rec), rec), '%d\t%s' % (tot[rec], ' '.join('%s:%d' % kv for kv in sorted(byeid[rec].items()) if kv[1])))
ex = dict(l.split() for l in open(os.path.join(O, 'occ', 'exit.txt')))
for fn in sorted(os.listdir(os.path.join(O, 'occ'))):
    if not (fn.startswith('diff_') or fn.startswith('w4_')): continue
    t = open(os.path.join(O, 'occ', fn)).read().splitlines()
    base = fn[:-4]
    for l in t:
        if l.startswith(('occurrences', 'verdict differences', 'basis differences', 'matching:')):
            add('diff', base, '-', l.split(':')[0] if not l.startswith('occurrences') else 'occurrences', l)
    add('diff', base, '-', 'exit', ex.get(base, '?'))
for item, val in (('p3.calls', '269 = AppID 0 24 (ES 20 + EVS 4) + SB self-send 5 + other 240'),
                  ('p3.relations', 'core-application 264, intra-application 5'),
                  ('d4gi.calls', '272 = AppID 0 24 + SB self-send 5 + other 243'),
                  ('d4gi.relations', 'core-application 267, intra-application 5')):
    add('recorded', 'STATUS_20261027.md 3.1', '-', item, val)
open(os.path.join(O, 'summary.tsv'), 'w').write('section\trecord\treading\titem\tvalue\n' + '\n'.join(rows) + '\n')
EOF
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
sha256sum "$O/summary.tsv" "$O/sites_matrix.tsv" $( [ -f "$O/section.md" ] && echo "$O/section.md" ) "$HERE/w4w5_r2.sh" "$HERE/w4w5_r2_summary.sh" \
  /home/user/work/kairos-build/tools/kz/kz /home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static > "$O/sha256.txt"
cat "$O/sha256.txt"

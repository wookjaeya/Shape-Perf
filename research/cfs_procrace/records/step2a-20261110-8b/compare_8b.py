# compare_8b.py KZDUMP OUTFILE NEW REF [REF ...] -- the outputs of the 8b run (NEW) against earlier runs (REF: T/ and
# tests_rv4), build by build.  Containers (NP/basis.kz, NP/facts.kz, identity/<b>/<V>/{basis,facts}.kz) are dumped
# with kz-dump (the tool_sha256, artifact_sha256 and container lines left out, as the driver's dumpn does) and
# compared section by section: the records only in one of them, BASIS paired by name (changed items listed), HEADER
# lines listed.  Text outputs are compared line by line with the lines AR 15.0 declares (the facts or basis hash
# line) left out and listed; the verdict inputs (count/) and the other texts byte for byte.  Prints one line per
# comparison (same / DIFF and what differs) and a summary; exit 0 always (the reader judges against AR 15.0).
import collections, hashlib, os, re, subprocess, sys

KZD, OF, NEW = sys.argv[1:4]
REFS = sys.argv[4:]
BUILDS = ['c1_pre_tx', 'c1_post_tx', 'r2_core', 'c5_641', 'c5_642']
IDCASES = [(b, v) for b in ('c1_pre_tx', 'r2_core', 'c5_641') for v in ('V0', 'V2')]
out = []


def sha(p):
    h = hashlib.sha256()
    with open(p, 'rb') as f:
        for blk in iter(lambda: f.read(1 << 20), b''):
            h.update(blk)
    return h.hexdigest()


def dump(path):
    r = subprocess.run([KZD, path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        raise SystemExit('kz-dump %s: rc %d' % (path, r.returncode))
    secs, cur = {'HEADER': []}, 'HEADER'
    for line in r.stdout.decode('utf-8', 'replace').split('\n'):
        if not line or re.match(r'^(tool_sha256|artifact_sha256)=|^container ', line):
            continue
        m = re.match(r'section (\S+) (.*)', line)
        if m:
            cur = m.group(1)
            secs[cur] = []
            secs['HEADER'].append('section %s %s' % (cur, re.sub(r' (count|crc32)=\S+', '', m.group(2))))
        elif cur == 'HEADER':
            secs['HEADER'].append(line.strip())
        elif line.startswith('  '):
            secs[cur].append(re.sub(r'^  \S+\[\d+\] ', '', line))
    return secs


def cmp_container(label, a_path, b_path):
    a, b = dump(a_path), dump(b_path)
    diffs = []
    for s in sorted(set(a) | set(b)):
        if s == 'STRINGS':
            continue
        ra, rb = a.get(s), b.get(s)
        if ra is None or rb is None:
            diffs.append('%s only in %s' % (s, 'new' if ra is None else 'ref'))
            continue
        if ra == rb:
            continue
        ca, cb = collections.Counter(ra), collections.Counter(rb)
        gone, came = sorted((ca - cb).elements()), sorted((cb - ca).elements())
        if s == 'BASIS':
            name = lambda r: (re.match(r'name="([^"]*)"', r) or re.match(r'(.*)', r)).group(1)
            ng, nc = collections.Counter(name(r) for r in gone), collections.Counter(name(r) for r in came)
            changed = sorted(set(ng) & set(nc))
            diffs.append('BASIS changed %s; removed %s; added %s' % (','.join(changed) or '-', ','.join(sorted(set(ng) - set(nc))) or '-',
                                                                   ','.join(sorted(set(nc) - set(ng))) or '-'))
        elif s == 'HEADER':
            diffs.append('HEADER ref {%s} new {%s}' % (' | '.join(x[:80] for x in gone), ' | '.join(x[:80] for x in came)))
        else:
            diffs.append('%s removed %d added %d (e.g. %s)' % (s, len(gone), len(came), ' | '.join(x[:160] for x in (gone + came)[:2])))
    n = sum(1 for s in a if s not in ('STRINGS', 'HEADER'))
    out.append('%s\t%s\t%d sections\t%s' % ('same' if not diffs else 'DIFF', label, n, '; '.join(diffs) or '-'))


def cmp_text(label, a_path, b_path, skip=()):
    if not os.path.exists(a_path) or not os.path.exists(b_path):
        out.append('MISSING\t%s\t%s' % (label, a_path if not os.path.exists(a_path) else b_path))
        return
    la = open(a_path, encoding='utf-8', errors='replace').read().split('\n')
    lb = open(b_path, encoding='utf-8', errors='replace').read().split('\n')
    if la == lb:
        out.append('same\t%s\tbytes' % label)
        return
    if len(la) != len(lb):
        out.append('DIFF\t%s\tlines %d vs %d' % (label, len(la), len(lb)))
        return
    d = [i + 1 for i, (x, y) in enumerate(zip(la, lb)) if x != y]
    other = [i for i in d if i not in skip]
    if not other:
        out.append('same-but-declared\t%s\tdiffering lines %s (declared)' % (label, ','.join(map(str, d))))
    else:
        out.append('DIFF\t%s\tdiffering lines %s; e.g. line %d: %s -> %s' % (label, ','.join(map(str, d[:20])), other[0], la[other[0] - 1][:120],
                                                                          lb[other[0] - 1][:120]))


def cmp_bytes(label, a_path, b_path):
    if not os.path.exists(a_path) or not os.path.exists(b_path):
        out.append('MISSING\t%s' % label)
        return
    out.append('%s\t%s\tbytes' % ('same' if sha(a_path) == sha(b_path) else 'DIFF', label))


def facts_txt(label, a_path, b_path):
    la = open(a_path, encoding='utf-8', errors='replace').read().split('\n')
    lb = open(b_path, encoding='utf-8', errors='replace').read().split('\n')
    if la == lb:
        out.append('same\t%s\tbytes' % label)
        return
    if len(la) != len(lb):
        out.append('DIFF\t%s\tlines %d vs %d' % (label, len(la), len(lb)))
        return
    d = [(x, y) for x, y in zip(la, lb) if x != y]
    keys = sorted({(re.match(r'\s*(\S+)', y) or re.match(r'(.*)', y)).group(1) for _, y in d})
    out.append('%s\t%s\t%d lines differ: %s' % ('DIFF', label, len(d), ','.join(keys)))


for ref in REFS:
    tag = os.path.basename(ref.rstrip('/'))
    for bld in BUILDS:
        r, n = os.path.join(ref, bld, 'NP'), os.path.join(NEW, bld, 'NP')
        cmp_container('%s %s NP/basis.kz' % (tag, bld), os.path.join(r, 'basis.kz'), os.path.join(n, 'basis.kz'))
        cmp_container('%s %s NP/facts.kz' % (tag, bld), os.path.join(r, 'facts.kz'), os.path.join(n, 'facts.kz'))
        cmp_text('%s %s NP/extract.txt' % (tag, bld), os.path.join(r, 'extract.txt'), os.path.join(n, 'extract.txt'), skip=(1,))
        cmp_text('%s %s NP/relations.txt' % (tag, bld), os.path.join(r, 'relations.txt'), os.path.join(n, 'relations.txt'), skip=(2,))
        cmp_text('%s %s NP/basis.txt' % (tag, bld), os.path.join(r, 'basis.txt'), os.path.join(n, 'basis.txt'), skip=(1,))
        facts_txt('%s %s NP/facts.txt' % (tag, bld), os.path.join(r, 'facts.txt'), os.path.join(n, 'facts.txt'))
        for f in ('relations.log', 'args.txt'):
            cmp_bytes('%s %s NP/%s' % (tag, bld, f), os.path.join(r, f), os.path.join(n, f))
        r2, n2 = os.path.join(ref, bld, 'NP2'), os.path.join(NEW, bld, 'NP2')
        cmp_text('%s %s NP2/relations.txt' % (tag, bld), os.path.join(r2, 'relations.txt'), os.path.join(n2, 'relations.txt'), skip=(2,))
        cmp_text('%s %s NP2/extract.txt' % (tag, bld), os.path.join(r2, 'extract.txt'), os.path.join(n2, 'extract.txt'), skip=(1,))
    for f in ('summary.txt', 'targets_rule.tsv', 'summary.tsv', 'burden.tsv', 'against_s5.tsv', 'against_fix1.tsv', 'against_fix2.tsv', 'against_fix3.tsv'):
        cmp_bytes('%s count/%s' % (tag, f), os.path.join(ref, 'count', f), os.path.join(NEW, 'count', f))
    for bld, v in IDCASES:
        r, n = os.path.join(ref, 'identity', bld, v), os.path.join(NEW, 'identity', bld, v)
        cmp_container('%s identity %s %s basis.kz' % (tag, bld, v), os.path.join(r, 'basis.kz'), os.path.join(n, 'basis.kz'))
        cmp_container('%s identity %s %s facts.kz' % (tag, bld, v), os.path.join(r, 'facts.kz'), os.path.join(n, 'facts.kz'))
        cmp_text('%s identity %s %s extract.txt' % (tag, bld, v), os.path.join(r, 'extract.txt'), os.path.join(n, 'extract.txt'), skip=(1,))
        facts_txt('%s identity %s %s facts.txt' % (tag, bld, v), os.path.join(r, 'facts.txt'), os.path.join(n, 'facts.txt'))
        for f in ('basis.dump.diff', 'facts.dump.diff', 'show.diff', 'args.txt'):
            cmp_bytes('%s identity %s %s %s' % (tag, bld, v, f), os.path.join(r, f), os.path.join(n, f))
    # the T7 lines: the verdict words (the hashes follow the facts)
    tw = lambda p: [re.sub(r'\([0-9a-f]{16}\)', '(h)', x) for x in open(p, encoding='utf-8').read().split('\n')]
    out.append('%s\t%s twice.txt\tverdict words' % ('same' if tw(os.path.join(ref, 'twice.txt')) == tw(os.path.join(NEW, 'twice.txt')) else 'DIFF', tag))

cnt = collections.Counter(x.split('\t', 1)[0] for x in out)
with open(OF, 'w', encoding='utf-8') as fh:
    fh.write('# compare_8b.py: %s against %s; %s\n' % (NEW, ' '.join(REFS), dict(cnt)))
    fh.write('\n'.join(out) + '\n')
print(dict(cnt))

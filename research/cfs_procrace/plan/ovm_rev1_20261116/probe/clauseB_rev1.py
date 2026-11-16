#!/usr/bin/env python3
# Offline probe, revision 1: clause B (FIXED-ORDER) of the method document, section 3.3, applied PER CONSUMING ACTOR
# to one relation row and to existing run records.  It reads files only; it starts nothing.
#
# usage:
#   python3 -I clauseB_rev1.py --relations <relations.txt> --row <ROW> \
#           --record NAME=<replay text>,<occurrence table>,<record dump> [--record ...]
#   python3 -I clauseB_rev1.py --selftest
#
# Inputs
#   relations.txt      the step 2a relation rows.  The row (e.g. B65) gives: the shared field, the callee and the
#                      argument position, the call sites ("site <file>:<line>:<col> in <function> ... result=<use>"),
#                      the writers of the field, the callee's constant returns, the handling counts, the order grade.
#   replay text        "occurrence <kind> actor <a> ordinal <n> key [<k>] call #<i> return #<j>" blocks with a
#                      "result <v>" line; "identity <a>: ..." lines.
#   occurrence table   one line per occurrence, in the same order, with the record event index of the call (nevent).
#   record dump        text dump of the record: NSITES (label, kind, line, symbol, source) and NEVENTS (actor, aseq, site).
#
# How the relation filter is derived (nothing is listed by hand):
#   F1  the sites are the "site" lines of the row;
#   F2  a site of the record belongs to the relation when it is a point site whose source path ends with the row
#       site's file, whose line equals the row site's line and whose symbol equals the row site's function;
#   F3  an occurrence (one call of the row's callee) belongs to the relation when the event just before the call in
#       the same actor's own sequence (aseq - 1) is at a site matched by F2.  That site also gives the enclosing
#       function of the call (the only outer-call context the record holds);
#   F4  the publication w is the first event at a record site matched (as in F2) with a writer line of the row whose
#       value is not the field's initial value.
# What the filter cannot do:
#   - a row site with no point site in the record cannot be recognised.  An occurrence with no site capture has
#     membership "unknown": it is neither counted for a consumer nor against it.  The value of the argument is NOT
#     used to decide membership (the consumed field value differs by definition before and after w);
#   - the call site of the consumer's own code that entered the enclosing function (the outermost API call site) and
#     the string key of the instance are not in the record;
#   - the column of a row site is not in the record (file, line and function are compared);
#   - before/after w is the position in the merged event order of the record relative to the event of F4.
#
# Clause B, per consumer c (an actor of the record other than the actor of w).  Every item prints exactly one of
#   성립 / 불성립 / 자료 없음 / 미검사      (item 1 may also print 미완성: the static proof is not complete)
#   1  the facts prove that u precedes w for c                         <- the row's order grade
#   2  c executed a use site of the row before w at least once, and all those occurrences are fail
#   3  c's OWN occurrences after w at the sites c used before w are not empty, and none returns a status of item 2
#   4  at least one occurrence of item 2 "stands": its instance (actor, site, outer call site, string key) does not
#      reappear with a non-fail grade inside the actor's initialization interval
#   5  the label of the use is R or J: here J = the status is discarded at the site (row) and the absence result
#      read from c's own occurrences before w is of the fail grade
# The occurrences of another consumer never enter c's items.  They are printed as reference only.
# Grade rule (method document 3.1, R-CLASS, cFE status): fail when bits 31-30 are 11; info when they are 01.
import collections
import hashlib
import os
import re
import sys
import tempfile

OK, NO, NODATA, UNCHECKED, INCOMPLETE = '성립', '불성립', '자료 없음', '미검사', '미완성'


def grade(res):
    if res is None:
        return 'no-result'
    v = int(res, 16) if res.lower().startswith('0x') else int(res)
    if v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF:
        return 'fail'
    if 0x40000000 <= v < 0x80000000:
        return 'info'
    return 'ok'


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for b in iter(lambda: f.read(1 << 16), b''):
            h.update(b)
    return h.hexdigest()


# ---------------------------------------------------------------- the relation row
def read_row(path, rid):
    lines, on = [], False
    head = None
    for n, line in enumerate(open(path, errors='replace'), 1):
        m = re.match(r'^  ([ABC]\d+) (.*)$', line)
        if m:
            if on:
                break
            if m.group(1) == rid:
                on, head, headline = True, m.group(2).rstrip(), n
            continue
        if on:
            if not line.startswith('      '):
                break
            lines.append((n, line.strip()))
    if head is None:
        raise SystemExit('row %s not found in %s' % (rid, path))
    row = {'id': rid, 'head': head, 'line': headline, 'sites': [], 'writers': [], 'consts': []}
    m = re.match(r'(\S+) -> (\S+?)#(\d+) sites=(\d+)', head)
    if m:
        row.update(field=m.group(1), callee=m.group(2), argpos=int(m.group(3)), nsites=int(m.group(4)))
    for n, x in lines:
        m = re.match(r'site (\S+):(\d+):(\d+) in (\S+) .*result=(\w+)', x)
        if m:
            row['sites'].append({'file': m.group(1), 'line': int(m.group(2)), 'col': int(m.group(3)),
                                 'fn': m.group(4).split('@')[0], 'use': m.group(5), 'rowline': n})
            continue
        m = re.match(r'field: INIT (\S+)', x)
        if m:
            row['init'] = m.group(1)
            continue
        m = re.match(r'(\S+) at (\S+):(\d+):(\d+) in (\S+).* value=(\S+)', x)
        if m and not x.startswith(('site ', 'callee-check', 'handling', 'feasibility', 'flip', 'dependency')):
            row['writers'].append({'how': m.group(1), 'file': m.group(2), 'line': int(m.group(3)),
                                   'fn': m.group(5).split('@')[0], 'value': m.group(6), 'rowline': n})
            continue
        m = re.match(r'callee-check: (\S+) returns (-?\d+) \[([^\]]*)\]', x)
        if m:
            row['consts'].append((m.group(1), int(m.group(2)), m.group(3)))
            continue
        m = re.match(r'handling: discarded (\d+), used (\d+), tested (\d+)', x)
        if m:
            row['handling'] = tuple(int(g) for g in m.groups())
            continue
        if x.startswith('actors:'):
            row['actors'] = x
            continue
        if x.startswith('feasibility:'):
            row['feas'], row['feasline'] = x[len('feasibility:'):].strip(), n
            continue
        if x.startswith('flip:'):
            row['flip'] = x[len('flip:'):].strip()
    return row


def static_order(row):
    """the row's order sentence: {'fn', 'writer': (actor, iteration), 'before': [(actor, it)], 'after': [...], 'proved'}"""
    feas = row.get('feas', '')
    out = {'proved': False, 'grade': feas.split(':')[0] + ':' + (feas.split(':')[1] if ':' in feas else ''), 'fn': None,
           'writer': None, 'before': [], 'after': []}
    m = re.search(r'creation order at the table walk \S+ in (\S+?): writer actor (\S+) at iteration (\d+); '
                  r'reader actors created before it: ([^;.]*); after it: ([^.]*)\.', feas)
    if m:
        out['fn'] = m.group(1)
        out['writer'] = (m.group(2), int(m.group(3)))
        for key, txt in (('before', m.group(4)), ('after', m.group(5))):
            out[key] = [(a, int(i)) for a, i in re.findall(r'(\S+) \(iteration (\d+)\)', txt)]
    # No grade of these rows states a proved u-before-w order.  A row that says "undecidable" (or anything other
    # than a proof) leaves item 1 incomplete; this probe knows no row text that proves it and therefore never
    # sets 'proved' from the text.
    return out


# ---------------------------------------------------------------- the record
def read_dump(path):
    sites, events, sect = {}, [], None
    for line in open(path, errors='replace'):
        if line.startswith('section '):
            sect = line.split()[1]
            continue
        if sect == 'NSITES':
            m = re.match(r'\s+NSITES\[(\d+)\] id=(\d+) kind=(\S+) .*? line=(\d+) .*?label="([^"]*)" .*?symbol="([^"]*)" source="([^"]*)"', line)
            if m:
                sites[int(m.group(1))] = {'kind': m.group(3), 'line': int(m.group(4)), 'label': m.group(5),
                                          'symbol': m.group(6), 'source': m.group(7)}
        elif sect == 'NEVENTS':
            m = re.match(r'\s+NEVENTS\[(\d+)\] t=\d+ actor=(\d+) aseq=(\d+) site=(\d+)', line)
            if m:
                assert int(m.group(1)) == len(events)
                events.append((int(m.group(2)), int(m.group(3)), int(m.group(4))))
    return sites, events


def read_replay(path):
    occ, cur, ident = [], None, []
    for n, line in enumerate(open(path, errors='replace'), 1):
        m = re.match(r'identity (\S+): creator (\S+), (.*)$', line)
        if m:
            req = re.search(r'requested from (\S+?),', m.group(3) + ',')
            ident.append({'actor': m.group(1), 'creator': m.group(2), 'req': req.group(1) if req else None})
            continue
        m = re.match(r'occurrence (\S+) actor (\S+) ordinal (\d+) key \[([^\]]*)\] call #(\d+)', line)
        if m:
            cur = {'kind': m.group(1), 'actor': m.group(2), 'key': m.group(4), 'result': None, 'textline': n}
            occ.append(cur)
            continue
        if cur is not None:
            m = re.match(r'\s+result (\S+)', line)
            if m and cur['result'] is None:
                cur['result'] = m.group(1)
    return occ, ident


def read_occtable(path):
    rows, hdr = [], None
    for line in open(path, errors='replace'):
        f = line.rstrip('\n').split('\t')
        if hdr is None:
            hdr = f
            continue
        rows.append(dict(zip(hdr, f)))
    return rows


def match_site(rec_site, file, line, fn):
    return rec_site['kind'] == 'point' and rec_site['line'] == line and rec_site['symbol'] == fn and \
        (rec_site['source'] == file or rec_site['source'].endswith('/' + file))


def load_record(row, replay, occtable, dump):
    """returns (occurrences, info).  occurrence: actor, idx (record event index of the call), site (row site index or
    None), member (True or None=unknown), result, key, outer, skey"""
    sites, events = read_dump(dump)
    rocc, ident = read_replay(replay)
    tab = read_occtable(occtable)
    info = {'problems': [], 'ident': ident}
    if len(rocc) != len(tab):
        raise SystemExit('occurrence counts differ: replay text %d, table %d' % (len(rocc), len(tab)))
    # F2
    rel_sites = {}
    for si, s in sites.items():
        hits = [k for k, rs in enumerate(row['sites']) if match_site(s, rs['file'], rs['line'], rs['fn'])]
        if len(hits) == 1:
            rel_sites[si] = hits[0]
        elif len(hits) > 1:
            info['problems'].append('record site %d matches %d row sites (column not in the record)' % (si, len(hits)))
    info['rel_sites'] = rel_sites
    info['sites'] = sites
    # F4
    pub_sites = set()
    for si, s in sites.items():
        for w in row['writers']:
            if w['value'] != row.get('init') and match_site(s, w['file'], w['line'], w['fn']):
                pub_sites.add(si)
    info['pub_sites'] = pub_sites
    w_idx = next((i for i, e in enumerate(events) if e[2] in pub_sites), None)
    info['w'] = w_idx
    info['w_actor_index'] = events[w_idx][0] if w_idx is not None else None
    byseq = {(a, q): i for i, (a, q, s) in enumerate(events)}
    out, name_of = [], {}
    for k, (o, t) in enumerate(zip(rocc, tab)):
        ne = int(t['nevent'])
        a, q, s = events[ne]
        name_of[a] = o['actor']
        if o['actor'] != t['actor'] or int(o['key'], 0) != int(t['app_id'], 0):
            info['problems'].append('occurrence %d: replay text and table disagree' % k)
        if sites[s]['kind'] != 'call' or sites[s]['symbol'] != row['callee']:
            info['problems'].append('occurrence %d: its event is not a call of %s' % (k, row['callee']))
        prev = byseq.get((a, q - 1))
        site = rel_sites.get(events[prev][2]) if prev is not None else None   # F3
        out.append({'actor': o['actor'], 'idx': ne, 'site': site, 'member': True if site is not None else None,
                    'result': o['result'], 'key': o['key'], 'outer': None, 'skey': None, 'textline': o['textline']})
    info['w_actor'] = name_of.get(info['w_actor_index'], 'actor#%s' % info['w_actor_index'])
    info['init_end'] = {}          # per actor: record event index of the end of its initialization interval; not in these records
    return out, info


# ---------------------------------------------------------------- the clause, per consumer
def evaluate(consumer, occs, w, row, item1, init_end=None, pooled=False):
    """occs: ALL occurrences of the record.  Only the consumer's own enter its items (pooled=True is the earlier,
    wrong computation, kept for the regression check only)."""
    mine = [o for o in occs if o['actor'] == consumer]
    bef = [o for o in mine if o['idx'] < w]
    aft_src = occs if pooled else mine
    aft = [o for o in aft_src if o['idx'] > w]
    bef_m = [o for o in bef if o['member']]
    bef_u = [o for o in bef if o['member'] is None]
    aft_m = [o for o in aft if o['member']]
    aft_u = [o for o in aft if o['member'] is None]
    r = {'consumer': consumer, 'n_before': len(bef_m), 'n_before_unknown': len(bef_u), 'n_after': len(aft_m),
         'n_after_unknown': len(aft_u), 'items': {}, 'why': {}}
    # item 1
    r['items'][1], r['why'][1] = item1
    # item 2
    st2 = sorted(set(str(o['result']) for o in bef_m))
    if bef_m:
        nf = [o for o in bef_m if grade(o['result']) != 'fail']
        r['items'][2] = OK if not nf else NO
        r['why'][2] = '게시 전 관계 소속 발생 %d번, fail %d번, fail 아님 %d번, 상태 값 %s' % (len(bef_m), len(bef_m) - len(nf), len(nf), st2)
    elif bef_u:
        r['items'][2] = NODATA
        r['why'][2] = '게시 전 발생 %d번이 모두 지점 미관측(관계 소속 미확인)' % len(bef_u)
    else:
        r['items'][2] = NO
        r['why'][2] = '게시 전에 이 callee를 부른 발생이 0번'
    # item 3: the consumer's own occurrences after w at the sites it used before w
    used = set(o['site'] for o in bef_m)
    corr = [o for o in aft_m if o['site'] in used]
    r['n_after_corr'] = len(corr)
    if not bef_m:
        r['items'][3] = NODATA
        r['why'][3] = '항목 2의 상태 값이 없음(게시 전 관계 소속 발생 0번)'
    elif not corr:
        r['items'][3] = NODATA
        r['why'][3] = ('게시 후 대응 사용 0번: 같은 소비자가 게시 전에 쓴 지점에서의 게시 후 발생이 없음'
                       ' (게시 후 이 소비자의 관계 소속 발생 %d번, 지점 미관측 발생 %d번)' % (len(aft_m), len(aft_u)))
    else:
        bad = [o for o in corr if str(o['result']) in st2]
        r['items'][3] = NO if bad else OK
        r['why'][3] = '게시 후 대응 사용 %d번, 그 가운데 항목 2의 상태 값을 돌려준 것 %d번' % (len(corr), len(bad))
    # item 4
    fails = [o for o in bef_m if grade(o['result']) == 'fail']
    if not fails:
        r['items'][4] = NODATA
        r['why'][4] = '항목 2의 fail 발생이 없음'
    else:
        later = lambda f: [o for o in mine if o['member'] and o['idx'] > f['idx'] and o['site'] == f['site'] and grade(o['result']) != 'fail']
        coarse = sum(1 for f in fails if not later(f))        # no later non-fail occurrence of the actor at the site at all
        have_keys = all(o['outer'] is not None and o['skey'] is not None for o in mine if o['member'])
        end = (init_end or {}).get(consumer)
        if end is None:
            r['items'][4] = UNCHECKED
            r['why'][4] = ('행위자의 초기화 구간의 끝이 기록에 없음' + ('' if have_keys else '; 인스턴스 키의 바깥 호출 지점과 문자열 키도 기록에 없음')
                           + '. 참고(판정 아님): fail 발생 %d번 가운데 같은 행위자·같은 지점의 fail 아닌 재출현이 기록 전체에 없는 것 %d번' % (len(fails), coarse))
        elif not have_keys:
            inside = sum(1 for f in fails if not [o for o in later(f) if o['idx'] <= end])
            if inside:
                r['items'][4] = OK      # no non-fail reappearance at the site at all: true for every finer instance key
                r['why'][4] = '초기화 구간 안에서 같은 행위자·같은 지점의 fail 아닌 재출현이 없는 fail 발생 %d번(지점 단위는 인스턴스 단위를 포함)' % inside
            else:
                r['items'][4] = UNCHECKED
                r['why'][4] = '같은 지점의 fail 아닌 재출현이 있으나 인스턴스 키(바깥 호출 지점, 문자열 키)가 기록에 없어 같은 인스턴스인지 가릴 수 없음'
        else:
            def stands(f):
                return not [o for o in later(f) if o['idx'] <= end and o['outer'] == f['outer'] and o['skey'] == f['skey']]
            n = sum(1 for f in fails if stands(f))
            r['items'][4] = OK if n else NO
            r['why'][4] = 'fail 발생 %d번 가운데 초기화 구간 안에서 같은 인스턴스로 fail 아닌 등급이 다시 나타나지 않은 것 %d번' % (len(fails), n)
    # item 5
    uses = sorted(set(row['sites'][o['site']]['use'] for o in bef_m))
    if not bef_m:
        r['items'][5] = NODATA
        r['why'][5] = '이 소비자의 부재 결과가 기록에 없음(정적 평가: %s)' % row.get('flip', '?').split(':')[0]
    elif uses == ['discarded'] and all(grade(o['result']) == 'fail' for o in bef_m):
        r['items'][5] = OK
        r['why'][5] = '표시 J: 쓴 지점의 결과 처리가 모두 discarded(행), 기록의 부재 결과가 fail 등급'
    elif all(grade(o['result']) == 'fail' for o in bef_m):
        r['items'][5] = UNCHECKED
        r['why'][5] = '결과 처리 %s: 버려지지 않은 지점의 부재 분기 효과는 행에 없음(R인지 T인지 가릴 수 없음)' % uses
    else:
        r['items'][5] = NO
        r['why'][5] = '기록의 부재 결과에 fail 아닌 등급이 있음'
    vals = list(r['items'].values())
    if all(v == OK for v in vals):
        r['verdict'] = '충족'
    elif NO in vals:
        r['verdict'] = '미충족'
    else:
        r['verdict'] = '미확정'
    r['keys_before'] = dict(collections.Counter(o['key'] for o in bef_m))
    r['sites_before'] = dict(collections.Counter(o['site'] for o in bef_m))
    r['sites_after'] = dict(collections.Counter(o['site'] for o in aft_m))
    return r


def item1_for(consumer, order, actor_map):
    st = actor_map.get(consumer)
    if st is None:
        return (NODATA, '행의 생성 순서 문장에 이 소비자에 대응하는 정적 행위자가 없음')
    if order['proved']:
        return (OK, '행이 증명')
    if st in [a for a, _ in order['before']]:
        return (INCOMPLETE, '정적 행위자 %s: 행의 등급은 "%s"(작성자보다 먼저 만들어짐 + 대기, 증명 아님)' % (st, order['grade'].strip()))
    if st in [a for a, _ in order['after']]:
        return (INCOMPLETE, '정적 행위자 %s: 행은 작성자보다 뒤에 만들어진다고 적음(반대 방향의 후보, 증명 아님)' % st)
    return (NODATA, '정적 행위자 %s에 대한 순서 문장이 행에 없음' % st)


def map_actors(order, ident, w_actor):
    """rank correspondence (an assumption, printed as such): the k-th actor of the record whose creation was
    requested from the function of the row's table walk <-> the actor with the k-th smallest iteration number in
    the row's sentence.  Accepted only if the counts agree and the writer's rank falls on the actor of w."""
    if not order['fn'] or not order['writer']:
        return {}, '행에 생성 순서 문장이 없음'
    stat = sorted([order['writer']] + order['before'] + order['after'], key=lambda x: x[1])
    rec = [i['actor'] for i in ident if i['req'] == order['fn']]
    if len(stat) != len(rec):
        return {}, '거부: 행의 행위자 %d개, 기록에서 %s가 만든 행위자 %d개' % (len(stat), order['fn'], len(rec))
    m = {r: s[0] for r, s in zip(rec, stat)}
    if m.get(w_actor) != order['writer'][0]:
        return {}, '거부: 작성자의 순위가 게시 w의 행위자(%s)에 오지 않음' % w_actor
    return m, '채택(가정): %s가 만든 순서의 순위와 행의 반복 번호의 순위를 맞춤. 점검 하나 통과: 작성자 %s <-> w의 행위자 %s' % (order['fn'], order['writer'][0], w_actor)


def report(name, row, occs, info, paths):
    P = print
    P('== 기록 %s' % name)
    for p in paths:
        P('   입력 %s sha256 %s' % (p, sha(p)))
    for x in info['problems']:
        P('   문제: ' + x)
    sites = info['sites']
    P('   [필터 F2] 행의 지점 %d곳 가운데 기록에 지점별 관측이 있는 것 %d곳: %s' % (
        len(row['sites']), len(set(info['rel_sites'].values())),
        ', '.join('%s:%d in %s (기록 지점 "%s")' % (os.path.basename(row['sites'][k]['file']), row['sites'][k]['line'], row['sites'][k]['fn'], sites[si]['label'])
                  for si, k in sorted(info['rel_sites'].items()))))
    w = info['w']
    if w is None:
        P('   [필터 F4] 게시 w에 해당하는 기록 지점이 없음: 판정하지 않음')
        return []
    P('   [필터 F4] 게시 w = 기록 사건 %d, 행위자 %s, 기록 지점 %s' % (w, info['w_actor'], sorted(sites[s]['label'] for s in info['pub_sites'])))
    mem = [o for o in occs if o['member']]
    unk = [o for o in occs if o['member'] is None]
    P('   [미확인] 행을 낸 빌드의 소스와 이 기록의 실행 파일이 같은 판인지는 대조하지 않았다. 확인한 것은 F2의 일치(파일, 줄, 함수)뿐이다.')
    P('   [필터 F3] %s 호출 발생 %d번: 관계 소속 확인 %d번, 소속 미확인(지점 미관측) %d번' % (row['callee'], len(occs), len(mem), len(unk)))
    P('      소속 확인: 게시 전 %d번, 게시 후 %d번; 소속 미확인: 게시 전 %d번, 게시 후 %d번' % (
        sum(1 for o in mem if o['idx'] < w), sum(1 for o in mem if o['idx'] > w),
        sum(1 for o in unk if o['idx'] < w), sum(1 for o in unk if o['idx'] > w)))
    P('      소속 미확인 발생의 인자 값 분포(참고, 필터에 쓰지 않음): %s' % dict(collections.Counter(o['key'] for o in unk)))
    P('      소속 확인 발생의 인자 값 분포(참고): 게시 전 %s, 게시 후 %s' % (
        dict(collections.Counter(o['key'] for o in mem if o['idx'] < w)), dict(collections.Counter(o['key'] for o in mem if o['idx'] > w))))
    order = static_order(row)
    amap, how = map_actors(order, info['ident'], info['w_actor'])
    P('   [정적 행위자 대응] %s' % how)
    if amap:
        P('      ' + ', '.join('%s <-> %s' % kv for kv in amap.items()))
    consumers = [i['actor'] for i in info['ident'] if i['actor'] != info['w_actor']]
    for o in occs:
        if o['actor'] not in consumers and o['actor'] != info['w_actor']:
            consumers.append(o['actor'])
    res = [evaluate(c, occs, w, row, item1_for(c, order, amap), info['init_end']) for c in consumers]
    others_ok = lambda c: sum(1 for o in mem if o['idx'] > w and o['actor'] != c and grade(o['result']) != 'fail')
    P('   소비자별 조항 B (게시자 %s 제외, 소비자 %d개).  열: 소비자 | 게시 전 소속/미확인 | 게시 후 소속/대응/미확인 | 1 | 2 | 3 | 4 | 5 | 판정' % (info['w_actor'], len(res)))
    for r in res:
        P('   %-16s | %3d/%-3d | %3d/%3d/%-3d | %s | %s | %s | %s | %s | %s' % (
            r['consumer'], r['n_before'], r['n_before_unknown'], r['n_after'], r['n_after_corr'], r['n_after_unknown'],
            r['items'][1], r['items'][2], r['items'][3], r['items'][4], r['items'][5], r['verdict']))
    P('   게시 전에 관계 소속 발생이 있는 소비자의 항목별 근거:')
    for r in res:
        if not r['n_before'] and not r['n_before_unknown']:
            continue
        P('   - %s  판정 %s' % (r['consumer'], r['verdict']))
        for k in (1, 2, 3, 4, 5):
            P('       항목 %d: %s -- %s' % (k, r['items'][k], r['why'][k]))
        P('       게시 전 지점별: %s; 인자 값: %s (필드의 초기값 %s)' % (
            {'%s:%d' % (os.path.basename(row['sites'][s]['file']), row['sites'][s]['line']): n for s, n in r['sites_before'].items()},
            r['keys_before'], row.get('init')))
        P('       참고(판정에 쓰지 않음): 다른 행위자의 게시 후 관계 소속 발생 가운데 fail 아닌 것 %d번' % others_ok(r['consumer']))
    pubocc = [o for o in occs if o['actor'] == info['w_actor']]
    P('   게시자 %s 자신의 발생(소비자 아님): 게시 전 %d번, 게시 후 %d번(소속 확인 %d번)' % (
        info['w_actor'], sum(1 for o in pubocc if o['idx'] < w), sum(1 for o in pubocc if o['idx'] > w),
        sum(1 for o in pubocc if o['idx'] > w and o['member'])))
    c = collections.Counter(r['verdict'] for r in res)
    P('   요약: 조항 B 충족 %d, 미확정 %d, 미충족 %d (소비자 수).  충족 0이면 이 기록으로는 FIXED-ORDER 판정이 서지 않는다.' % (c['충족'], c['미확정'], c['미충족']))
    P('   미충족 가운데 "게시 전에 이 callee를 부른 발생이 0번"인 소비자 %d개(조항 B의 대상이 되지 않음)' % sum(1 for r in res if r['why'][2].startswith('게시 전에 이 callee')))
    return res


def describe_row(row, path):
    P = print
    P('행 %s (%s:%d) sha256 %s' % (row['id'], path, row['line'], sha(path)))
    P('   공유 필드 %s, callee %s의 인자 %d, 초기값(INIT) %s' % (row.get('field'), row.get('callee'), row.get('argpos', -1), row.get('init')))
    P('   [필터 F1] 지점 %d곳(머리줄의 sites=%s), 결과 처리 %s, handling 줄 discarded/used/tested = %s' % (
        len(row['sites']), row.get('nsites'), dict(collections.Counter(s['use'] for s in row['sites'])), row.get('handling')))
    P('   지점이 든 함수: %s' % dict(collections.Counter(s['fn'] for s in row['sites'])))
    P('   작성자 줄 %d개: %s' % (len(row['writers']), ['%s:%d in %s value=%s' % (os.path.basename(w['file']), w['line'], w['fn'], w['value']) for w in row['writers']]))
    P('   callee의 fail 등급 상수: %s' % sorted(set('%s=%s' % (n, hex(v)) for _, v, n in row['consts'] if grade(str(v)) == 'fail')))
    P('   정적 뒤집힘 평가(flip): %s' % row.get('flip', '?')[:160])
    o = static_order(row)
    P('   순서 등급(%s:%d): "%s"; 작성자 %s; 먼저 만들어지는 독자 %s; 뒤에 만들어지는 독자 %s' % (
        path.split('/')[-3] if path.count('/') > 2 else path, row.get('feasline', 0), o['grade'].strip(), o['writer'], o['before'], o['after']))
    P('   행의 독자 행위자: %s' % (row.get('actors', '?')[:400]))
    P('   항목 1의 지위: 미완성. 행의 등급이 증명이 아니다(행의 문장: "neither order is proven"%s).' % (
        '' if 'neither order is proven' in row.get('feas', '') else ' -- 이 문장은 이 행에 없음'))


# ---------------------------------------------------------------- regression check on a tiny synthetic record
def selftest():
    row = {'sites': [{'file': 'm/a.c', 'line': 10, 'col': 1, 'fn': 'F', 'use': 'discarded'},
                     {'file': 'm/a.c', 'line': 20, 'col': 1, 'fn': 'G', 'use': 'discarded'}], 'flip': 'static-undetermined: x'}
    FAIL, OKV = '0xc2000003', '0'
    def occ(actor, idx, site, res, member=True, outer=None, skey=None):
        return {'actor': actor, 'idx': idx, 'site': site, 'member': member, 'result': res, 'key': '0', 'outer': outer, 'skey': skey}
    w = 100
    occs = [occ('X', 10, 0, FAIL), occ('X', 11, 0, FAIL),                       # X: fails before w, nothing after
            occ('Y', 110, 0, OKV), occ('Y', 111, 0, OKV), occ('Y', 112, 0, OKV),  # Y: only successes after w, same site
            occ('Z', 20, 0, FAIL, outer='o1', skey='k1'), occ('Z', 120, 0, OKV, outer='o2', skey='k2'),
            occ('Q', 30, 0, FAIL, outer='o1', skey='k1'), occ('Q', 130, 0, OKV, outer='o1', skey='k1'),
            occ('S', 40, 0, FAIL), occ('S', 140, 0, FAIL),                      # S: the item-2 status again after w
            occ('U', 50, None, FAIL, member=None), occ('U', 150, None, OKV, member=None),   # U: site not observed
            occ('V', 60, 0, FAIL), occ('V', 160, 1, OKV)]                       # V: after w only at ANOTHER site
    proved = (OK, 'test: item 1 given as proved')
    ends = {'Z': 500, 'Q': 500}
    checks = []
    def chk(name, cond):
        checks.append((name, cond))
    x = evaluate('X', occs, w, row, proved)
    chk('X: 게시 후 대응 사용이 없는 소비자는 조항을 충족하지 않는다', x['verdict'] != '충족')
    chk('X: 항목 2 성립', x['items'][2] == OK)
    chk('X: 항목 3 자료 없음(다른 소비자 Y의 성공이 채우지 않는다)', x['items'][3] == NODATA)
    xp = evaluate('X', [o for o in occs if o['actor'] in ('X', 'Y')], w, row, proved, pooled=True)
    chk('민감도: 전체 합산 방식(이전 계산)이면 X의 항목 3이 성립으로 나온다 -- 이 점검이 그 결함을 잡는다', xp['items'][3] == OK)
    z = evaluate('Z', occs, w, row, proved, ends)
    chk('Z: 다섯 항목이 모두 성립하면 충족', z['verdict'] == '충족' and all(v == OK for v in z['items'].values()))
    z2 = evaluate('Z', occs, w, row, (INCOMPLETE, 'test'), ends)
    chk('Z: 항목 1이 미완성이면 충족이 아니다', z2['verdict'] == '미확정')
    z3 = evaluate('Z', occs, w, row, proved, None)
    chk('Z: 초기화 구간의 끝이 없으면 항목 4는 미검사', z3['items'][4] == UNCHECKED and z3['verdict'] == '미확정')
    q = evaluate('Q', occs, w, row, proved, ends)
    chk('Q: 같은 인스턴스가 fail 아닌 등급으로 다시 나타나면 항목 4 불성립', q['items'][4] == NO and q['verdict'] == '미충족')
    s = evaluate('S', occs, w, row, proved)
    chk('S: 게시 후에 항목 2의 상태 값이 나오면 항목 3 불성립', s['items'][3] == NO)
    u = evaluate('U', occs, w, row, proved)
    chk('U: 지점 미관측 발생은 항목 2·3을 채우지 않는다', u['items'][2] == NODATA and u['items'][3] == NODATA and u['verdict'] != '충족')
    v = evaluate('V', occs, w, row, proved)
    chk('V: 다른 지점에서의 게시 후 성공은 대응 사용이 아니다', v['items'][3] == NODATA and v['verdict'] != '충족')
    y = evaluate('Y', occs, w, row, proved)
    chk('Y: 게시 전 사용이 없으면 항목 2 불성립', y['items'][2] == NO and y['verdict'] == '미충족')
    # the parsers and the filter on tiny text files
    d = tempfile.mkdtemp(prefix='clauseB_rev1_selftest_')
    rel = os.path.join(d, 'relations.txt')
    open(rel, 'w').write(
        '  B1 T_t.Id -> Callee#2 sites=2 callee-check=yes\n'
        '      site m/a.c:10:3 in F [G.Id] base=global hops=0 result=discarded\n'
        '      site m/a.c:20:3 in G [G.Id] base=global hops=0 result=discarded\n'
        '      field: INIT 0 (G); writers 2 from WRITERS\n'
        '        memset at m/i.c:5:5 in Early value=0 [fill 0]\n'
        '        out-pointer at m/t.c:30:5 in Init via Get value=symbolic [x]\n'
        '      actors: readers {P (e), W (e)}; writers {W (e)} (inter-actor)\n'
        '      callee-check: Callee returns 3254779907 [ILLEGAL] when (x)\n'
        '      handling: discarded 2, used 0, tested 0\n'
        '      feasibility: undecidable: fixed unconfirmed (x): creation order at the table walk m/s.c:1:1 in Mk: writer actor W at iteration 2; reader actors created before it: P (iteration 1); after it: none. so neither order is proven.\n'
        '      flip: static-undetermined: x\n'
        '  B2 other\n')
    dump = os.path.join(d, 'dump')
    ev = [(0, 0, 3), (0, 1, 0), (0, 2, 1),      # P: site point a.c:10, call, ret   (before w)
          (1, 0, 2),                           # W: publication
          (1, 1, 3), (1, 2, 0), (1, 3, 1),      # W: its own use after w
          (2, 0, 0), (2, 1, 1)]                # R: a call with no site capture after w
    open(dump, 'w').write(
        'section NSITES type=101\n'
        '  NSITES[0] id=0 kind=call phase=before decoder=g line=1 bind=0 flags={} label="c.entry" op="" symbol="Callee" source=""\n'
        '  NSITES[1] id=1 kind=return phase=after decoder=g line=1 bind=1 flags={} label="c.ret" op="" symbol="Callee" source=""\n'
        '  NSITES[2] id=2 kind=point phase=point decoder=g line=30 bind=2 flags={} label="pub" op="" symbol="Init" source="/src/m/t.c"\n'
        '  NSITES[3] id=3 kind=point phase=point decoder=g line=10 bind=3 flags={} label="valid@a_10" op="" symbol="F" source="/src/m/a.c"\n'
        'section NEVENTS type=103\n' +
        ''.join('  NEVENTS[%d] t=%d actor=%d aseq=%d site=%d bind=0\n' % (i, i, a, q, s) for i, (a, q, s) in enumerate(ev)))
    rep = os.path.join(d, 'cur.txt')
    open(rep, 'w').write(
        'identity P: creator root, owner x, requested from Mk, core\n'
        'identity W: creator root, owner x, requested from Mk, core\n'
        'identity R: creator root, owner x, requested from Other, app\n'
        'occurrence K actor P ordinal 1 key [0] call #1 return #2\n  result 0xc2000003 outcome agrees\n'
        'occurrence K actor W ordinal 1 key [0x3] call #5 return #6\n  result 0 outcome agrees\n'
        'occurrence K actor R ordinal 1 key [0x3] call #7 return #8\n  result 0 outcome agrees\n')
    tab = os.path.join(d, 'occ.tsv')
    open(tab, 'w').write('occ\tactor\tapp_id\tnevent\n0\tP\t0x0\t1\n1\tW\t0x3\t5\n2\tR\t0x3\t7\n')
    r = read_row(rel, 'B1')
    o, info = load_record(r, rep, tab, dump)
    chk('파서: 행의 지점 2곳, 작성자 2개', len(r['sites']) == 2 and len(r['writers']) == 2)
    chk('필터: 기록 지점 1곳만 행의 지점에 대응', sorted(info['rel_sites'].items()) == [(3, 0)])
    chk('필터: 게시 w는 초기값이 아닌 작성자의 지점(사건 3, 행위자 W)', info['w'] == 3 and info['w_actor'] == 'W')
    chk('필터: 발생 3번 가운데 소속 확인 2번, 미확인 1번', [x['member'] for x in o] == [True, True, None])
    order = static_order(r)
    amap, how = map_actors(order, info['ident'], info['w_actor'])
    chk('정적 행위자 대응: P<->P, W<->W', amap == {'P': 'P', 'W': 'W'})
    p = evaluate('P', o, info['w'], r, item1_for('P', order, amap), info['init_end'])
    chk('P(게시 후 자기 사용 없음): 항목 1 미완성, 2 성립, 3 자료 없음, 4 미검사, 5 성립, 판정 미확정',
        [p['items'][k] for k in (1, 2, 3, 4, 5)] == [INCOMPLETE, OK, NODATA, UNCHECKED, OK] and p['verdict'] == '미확정')
    for f in (rel, dump, rep, tab):
        os.remove(f)
    os.rmdir(d)
    bad = 0
    for name, cond in checks:
        print('%s  %s' % ('PASS' if cond else 'FAIL', name))
        bad += 0 if cond else 1
    print('selftest: %d checks, %d failed' % (len(checks), bad))
    return 1 if bad else 0


def main(argv):
    if '--selftest' in argv:
        return selftest()
    rel = rid = None
    recs = []
    i = 1
    while i < len(argv):
        if argv[i] == '--relations':
            rel = argv[i + 1]; i += 2
        elif argv[i] == '--row':
            rid = argv[i + 1]; i += 2
        elif argv[i] == '--record':
            name, rest = argv[i + 1].split('=', 1)
            recs.append((name, rest.split(','))); i += 2
        else:
            raise SystemExit('unknown argument ' + argv[i])
    if not (rel and rid and recs):
        raise SystemExit(__doc__ or 'usage: see the head of this file')
    print('clause B per consumer (offline probe, revision 1).  읽기만 한다. 이 출력은 기존 기록에 대한 점검이고 탐지 주장이 아니다.')
    row = read_row(rel, rid)
    describe_row(row, rel)
    allres = {}
    for name, (replay, occtable, dump) in recs:
        occs, info = load_record(row, replay, occtable, dump)
        allres[name] = report(name, row, occs, info, (replay, occtable, dump))
    print('== 기록 사이')
    print('   항목 2의 "모든 기준 부팅"은 기록마다 따로 평가했다. 기록 사이에서 같은 소비자를 맞추는 일은 하지 않았다(미검사):')
    print('   행위자 이름이 기록마다 다르고, 소유 앱 ID는 같은 앱의 주 작업과 자식 작업을 가르지 못한다.')
    for name, res in allres.items():
        print('   %s: 게시 전 관계 소속 발생이 있는 소비자 %s' % (name, [(r['consumer'], r['n_before'], r['verdict']) for r in res if r['n_before']]))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

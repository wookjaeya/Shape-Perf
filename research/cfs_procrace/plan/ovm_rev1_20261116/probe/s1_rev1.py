#!/usr/bin/env python3
# Offline probe, revision 1: the S1 labels of the method document (section 3.3) applied to the text of the step 2a
# relation rows, with every row traced to exactly one named disposition.  It reads one relations.txt; it starts nothing.
#
# usage: python3 -I s1_rev1.py <relations.txt> [--tsv <per-row output>] [--show ROW,ROW]
#
# All numbers are counts of ROWS of a text probe.  They are preliminary: not the effect of a finished classifier,
# not counts of plans, not counts of boots.  One row can stand for many call sites, objects and instances.
#
# 1. Label (unchanged from the earlier probe, so that a difference in a count is not a hidden change of the label rule)
#    fail-class constant (R-CLASS): v < 0, or 0xC0000000 <= v <= 0xFFFFFFFF.
#    path A (lookup use): absence constants = constants after "search returns:" on the flip line
#       H  a "probe-create" line, or an enclosing loop line with a delay whose exit compares a local with a fail constant
#       R  else a "handling: terminal" line
#       J  else a fail constant and result=discarded;   T? else a fail constant and result=tested
#       E  else a fail constant (result=used);          0  else constants but none fail;   E  no constant at all
#    path B (shared field as argument): constants of the "callee-check: F returns V" lines
#       J  fail constant and discarded>0;  T? fail constant, discarded=0, tested>0;  E fail constant, only used
#       0  callee check without fail constant;  E  no callee-check line
#    path C (write after spawn, child read): constants "F returns V [N] with the earlier value" on the flip line
#       F  a fail constant with the earlier value (the caller's handling is not in the row: R or J not decidable)
#       0  constants but none fail with the earlier value;  E  no constant
#    label classes:  decided = R, J, H, 0;  effect of the handling undetermined = T?, F;  absence result undetermined = E
# 2. Order class, from the row's feasibility line only
#    ENF    "proven-impossible"                                          (the row proves the opposite order impossible)
#    FIXED  "fixed unconfirmed" and a reader actor named as created BEFORE the writer   (fixed-order candidate)
#    COND   an order that rests on a premise the facts do not establish and that is not FIXED:
#           "fixed unconfirmed" with no reader created before the writer; path C "undecidable" that names a lock
#    RELOC  the row gives no static order and a counterpart exists: path A with a creator in another actor,
#           path B "no static order ...", path C "possible"              (relocation candidate)
#    NOPAIR path A with no creator in another actor; path C "not graded" (the context is no read of the spawned threads)
#    UNK    anything else (e.g. path C: reach of the call by the spawned threads undecided)
# 3. Reach flag: is the callee's failure constant known to be reached from the absence value?  Row text only; the
#    facts file is not consulted for it (the flip column is what the row generator evaluated from the facts).
#    REACH-STATED   the row says so: path A "returns V (no match", path C "returns V [N] with the earlier value"
#    REACH-FLIP     path A or B flip "static-possible": the row says the failure return changes with the order,
#                   without naming the constant
#    CALLEE-ONLY    a fail constant is listed for the callee or the search, and the row does not tie it to absence
#    UNEVALUATED    flip "static-undetermined": the check depends on the value but was not evaluated   -> 미확인
#    INDEPENDENT    flip "none": the row says no constant return depends on the value, or both values give the same
#    NO-CHECK       path B with no callee-check line                                                  -> 미확인
#    NO-FAILCONST   no fail constant in the row
# 4. Disposition: exactly one per row, first match wins
#    D01 순서 강제(행의 증명)                        ENF
#    D02 고정 순서 후보                              FIXED            (observed without intervention; never a directed run)
#    D03 조건부 순서 후보                            COND             (observed; no directed run before the premise is settled)
#    D04 치유                                        label H
#    D05 실패 표시 없음                              label 0
#    D06 표시 확정·상대 없음                         label R or J, path A, no creator in another actor
#    D07 지향 실행 후보(재배치 후보)                 label R or J, counterpart, RELOC, reach not INDEPENDENT (for J)
#    D08 재배치 후보·실패 상수가 값과 무관(행의 평가) label J, counterpart, RELOC, reach INDEPENDENT
#    D09 표시 확정·순서 미정                         label R or J, counterpart, order UNK or NOPAIR
#    D10 부재 결과 있음·처리 효과 미확정(F)          label F
#    D11 처리 효과 미확정(T?)                        label T?
#    D12 부재 결과 미확정(E)                         label E
#    D13 미분류                                      nothing matched (must be 0)
#    observation set (no intervention) = D02 D03 D07 D08 D09 D10 D11 D12
#    directed-run candidates           = D07
import collections
import hashlib
import re
import sys


def cls(v):
    if v < 0 or 0xC0000000 <= v <= 0xFFFFFFFF:
        return 'fail'
    if 0x40000000 <= v < 0x80000000:
        return 'info'
    return 'ok'


def consts(text):
    return [(int(v), n) for v, n in re.findall(r'returns (-?\d+) \[([^\]]*)\]', text)]


DISP = collections.OrderedDict([
    ('D01', '순서 강제(행의 증명)'), ('D02', '고정 순서 후보'), ('D03', '조건부 순서 후보'), ('D04', '치유'),
    ('D05', '실패 표시 없음'), ('D06', '표시 확정·상대 없음'), ('D07', '지향 실행 후보(재배치 후보)'),
    ('D08', '재배치 후보·실패 상수가 값과 무관(행의 평가)'), ('D09', '표시 확정·순서 미정'),
    ('D10', '부재 결과 있음·처리 효과 미확정(F)'), ('D11', '처리 효과 미확정(T?)'), ('D12', '부재 결과 미확정(E)'),
    ('D13', '미분류')])
OBSERVE = ('D02', 'D03', 'D07', 'D08', 'D09', 'D10', 'D11', 'D12')
UNDET = ('T?', 'F', 'E')


def read_rows(path):
    rows, cur = [], None
    for n, line in enumerate(open(path, errors='replace'), 1):
        m = re.match(r'^  ([ABC])(\d+) (.*)$', line)
        if m:
            cur = {'id': m.group(1) + m.group(2), 'p': m.group(1), 'head': m.group(3).rstrip(), 'lines': [], 'line': n}
            rows.append(cur)
            continue
        if re.match(r'^\[[ABC]\]', line) or re.match(r'^  namespace ', line):
            cur = None
            continue
        if cur is not None and line.startswith('      '):
            cur['lines'].append(line.strip())
    return rows


def classify(r):
    L = r['lines']
    get = lambda pre: [x for x in L if x.startswith(pre)]
    feas = (get('feasibility:') or ['feasibility: ?'])[0][13:]
    flip = (get('flip:') or ['flip: ?'])[0][6:]
    flipg = flip.split(':')[0].split()[0]
    r['flip'], r['feas'] = flipg, feas
    why = ''
    if r['p'] == 'A':
        act = (get('actors:') or [''])[0]
        res = (re.search(r'result=(\w+)', act) or [None, '?'])[1]
        ab = consts(flip.split('search returns:')[1]) if 'search returns:' in flip else []
        failc = [c for c in ab if cls(c[0]) == 'fail']
        term = bool(get('handling: terminal'))
        probe = any('probe-create' in x for x in L if x.startswith('handling:') or x.startswith('not a pair:'))

        def _retry(x):
            if ' delay ' not in x:
                return False
            ex = x.split(' exit ', 1)[1].split(' delay ', 1)[0] if ' exit ' in x else ''
            return any(cls(int(v)) == 'fail' for v in re.findall(r'\((?:ne|eq) local:\w+ (-?\d+)\)', ex))
        retry = [x for x in get('handling: in the loop') if _retry(x)]
        if probe or retry:
            lab = 'H'
        elif term:
            lab = 'R'
        elif failc and res == 'discarded':
            lab = 'J'
        elif failc and res == 'tested':
            lab = 'T?'
        elif failc:
            lab = 'E'
        elif ab:
            lab = '0'
        else:
            lab = 'E'
        dep = (get('dependency:') or [''])[0]
        pair = int((re.search(r'(\d+) creator\(s\)', dep) or [0, 0])[1])
        elem = (re.search(r'namespace (\S+)', r['head']) or [0, '?'])[1]
        nsite = 1
        # order
        if pair > 0:
            oc = 'RELOC' if feas.startswith('undecidable: the facts give no static order') else 'UNK'
        else:
            oc = 'NOPAIR'
        # reach
        stated = set(int(v) for v in re.findall(r'returns (-?\d+) \(no match', ' '.join(L)))
        if not failc:
            reach = 'NO-FAILCONST'
        elif any(c[0] in stated for c in failc):
            reach = 'REACH-STATED'
        elif flipg == 'static-possible':
            reach = 'REACH-FLIP'
        else:
            reach = 'CALLEE-ONLY'
        r.update(res=res, pair=pair)
    elif r['p'] == 'B':
        h = (get('handling: discarded') or [''])[0]
        m = re.search(r'discarded (\d+), used (\d+), tested (\d+)', h)
        d, u, t = (int(m.group(1)), int(m.group(2)), int(m.group(3))) if m else (0, 0, 0)
        cc = [x for x in L if x.startswith('callee-check:')]
        ab = [c for x in cc for c in consts(x)]
        failc = [c for c in ab if cls(c[0]) == 'fail']
        if failc and d > 0:
            lab = 'J'
        elif failc and t > 0:
            lab = 'T?'
        elif failc:
            lab = 'E'
        elif cc:
            lab = '0'
        else:
            lab = 'E'
        elem = r['head'].split()[0]
        nsite = int((re.search(r'sites=(\d+)', r['head']) or [0, 1])[1])
        cl = re.findall(r'reader actors created before it: ([^;.]*); after it: ([^.]*)\.', feas)
        before = any(not b.strip().startswith('none') for b, a in cl)
        after = any(not a.strip().startswith('none') for b, a in cl)
        r['mixed'] = before and after
        if feas.startswith('proven-impossible'):
            oc = 'ENF'
        elif before:
            oc = 'FIXED'
        elif cl or feas.startswith('undecidable: fixed unconfirmed'):
            oc = 'COND'
        elif feas.startswith('undecidable: no static order'):
            oc = 'RELOC'
        else:
            oc = 'UNK'
        if not cc:
            reach = 'NO-CHECK'
        elif not failc:
            reach = 'NO-FAILCONST'
        elif flipg == 'none':
            reach = 'INDEPENDENT'
        elif flipg == 'static-possible':
            reach = 'REACH-FLIP'
        elif flipg == 'static-undetermined':
            reach = 'UNEVALUATED'
        else:
            reach = 'CALLEE-ONLY'
        r.update(res='d%d/u%d/t%d' % (d, u, t), pair=1)
    else:
        early = re.findall(r'returns (-?\d+) \[([^\]]*)\] with the earlier value', flip)
        allc = consts(flip)
        failc = [(int(v), n) for v, n in early if cls(int(v)) == 'fail']
        lab = 'F' if failc else ('0' if allc else 'E')
        elem = (re.search(r'field (\S+?);', r['head']) or [0, '?'])[1]
        nsite = 1
        if feas.startswith('proven-impossible'):
            oc = 'ENF'
        elif feas.startswith('possible'):
            oc = 'RELOC'
        elif feas.startswith('not graded'):
            oc = 'NOPAIR'
        elif feas.startswith('undecidable') and re.search(r'\block\b', feas.split(';')[0]):
            oc = 'COND'
        else:
            oc = 'UNK'
        if failc:
            reach = 'REACH-STATED'
        elif any(cls(c[0]) == 'fail' for c in allc):
            reach = 'CALLEE-ONLY'
        elif flipg == 'static-undetermined':
            reach = 'UNEVALUATED'
        elif flipg == 'none':
            reach = 'INDEPENDENT'
        else:
            reach = 'NO-FAILCONST'
        r.update(res='(not in row)', pair=1)
    # the order classes of the earlier probe, kept only to reproduce its numbers
    if r['p'] == 'A':
        old = 'unordered'
    elif r['p'] == 'B':
        cl = re.findall(r'reader actors created before it: ([^;.]*); after it: ([^.]*)\.', feas)
        old = 'fixed?' if any(not b.strip().startswith('none') for b, a in cl) else ('after-only?' if cl and any(not a.strip().startswith('none') for b, a in cl) else ('walk-no-reader' if cl else 'unordered'))
    else:
        old = 'enforced' if feas.startswith('proven-impossible') else ('unordered' if feas.startswith('possible') else 'undecided')
    r.update(lab=lab, elem=elem, nsite=nsite, oc=oc, reach=reach, oldorder=old, fail=bool(failc))
    counterpart = r['pair'] > 0
    if oc == 'ENF':
        d, why = 'D01', 'feasibility: proven-impossible'
    elif oc == 'FIXED':
        d, why = 'D02', '행이 작성자보다 먼저 만들어지는 독자를 적음(fixed unconfirmed); 표시 ' + lab
    elif oc == 'COND':
        d, why = 'D03', '순서가 확인되지 않은 전제에 기댐: ' + feas[:60] + '; 표시 ' + lab
    elif lab == 'H':
        d, why = 'D04', 'probe-create 또는 실패 상태에 대한 지연 재시도 루프'
    elif lab == '0':
        d, why = 'D05', 'fail 등급의 부재 상수가 없음'
    elif lab in ('R', 'J') and not counterpart:
        d, why = 'D06', '표시 %s이나 다른 행위자의 생성자가 행에 없음' % lab
    elif lab in ('R', 'J') and oc == 'RELOC' and lab == 'J' and reach == 'INDEPENDENT':
        d, why = 'D08', '표시 J이나 flip none: 상수 반환이 값에 달리지 않는다고 행이 평가'
    elif lab in ('R', 'J') and oc == 'RELOC':
        d, why = 'D07', '표시 %s, 상대 있음, 정적 순서 없음; 도달 표시 %s' % (lab, reach)
    elif lab in ('R', 'J'):
        d, why = 'D09', '표시 %s, 순서 등급 %s' % (lab, oc)
    elif lab == 'F':
        d, why = 'D10', '부재 상수는 fail(earlier value)이나 호출자의 처리가 행에 없음; 순서 등급 ' + oc
    elif lab == 'T?':
        d, why = 'D11', '상태가 검사되나 부재 분기의 효과가 행에 없음; 순서 등급 ' + oc
    elif lab == 'E':
        d, why = 'D12', '행이 부재 결과를 주지 않음; 순서 등급 ' + oc
    else:
        d, why = 'D13', '규칙에 걸리지 않음'
    r['disp'], r['why'] = d, why
    return r


def main(argv):
    path = argv[1]
    tsv = show = None
    for i, a in enumerate(argv):
        if a == '--tsv':
            tsv = argv[i + 1]
        if a == '--show':
            show = set(argv[i + 1].split(','))
    rows = [classify(r) for r in read_rows(path)]
    P = print
    C = collections.Counter
    n = len(rows)
    P('s1 rev1 (offline text probe; 예비 집계: 행 수이며 계획 수도 부팅 수도 아니다)')
    P('input %s sha256 %s' % (path, hashlib.sha256(open(path, 'rb').read()).hexdigest()))
    P('rows %d %s' % (n, dict(sorted(C(r['p'] for r in rows).items()))))
    # the flow: every row -> one disposition
    dc = C(r['disp'] for r in rows)
    P('[흐름] 입력 행 -> 처리(행마다 정확히 하나). 합계 %d (입력 %d과 같아야 한다: %s)' % (sum(dc.values()), n, '같음' if sum(dc.values()) == n else '다름'))
    for d, name in DISP.items():
        sub = [r for r in rows if r['disp'] == d]
        P('   %s %-34s %4d  경로 %s  표시 %s  관측 집합 %s' % (d, name, len(sub), dict(sorted(C(r['p'] for r in sub).items())),
                                                              dict(sorted(C(r['lab'] for r in sub).items())), '예' if d in OBSERVE else '아니오'))
    assert dc['D13'] == 0 and sum(dc.values()) == n
    P('   처리 x 순서 등급: %s' % {'%s/%s' % k: v for k, v in sorted(C((r['disp'], r['oc']) for r in rows).items())})
    # set (a): label classes
    dec = [r for r in rows if r['lab'] in ('R', 'J', 'H', '0')]
    eff = [r for r in rows if r['lab'] in ('T?', 'F')]
    absn = [r for r in rows if r['lab'] == 'E']
    P('[집합 1] 분류가 성립한 행 %d (R %d, J %d, H %d, 0 %d) / 처리 효과 미확정 %d (T? %d, F %d) / 부재 결과 미확정 %d (E)  합 %d' % (
        len(dec), *[sum(1 for r in rows if r['lab'] == x) for x in ('R', 'J', 'H', '0')], len(eff),
        sum(1 for r in rows if r['lab'] == 'T?'), sum(1 for r in rows if r['lab'] == 'F'), len(absn), len(dec) + len(eff) + len(absn)))
    und = [r for r in rows if r['lab'] in UNDET]
    P('   미확정 표시(T?, F, E) 행 %d의 처리: %s' % (len(und), dict(sorted(C(r['disp'] for r in und).items()))))
    out_und = [r for r in und if r['disp'] not in OBSERVE]
    P('   미확정 표시 가운데 관측 집합 밖 %d (모두 D01 순서 강제여야 한다: %s)' % (len(out_und), dict(C(r['disp'] for r in out_und))))
    # set (b): observation without intervention / static gaps
    obs = [r for r in rows if r['disp'] in OBSERVE]
    P('[집합 2] 비개입 관측 대상 %d행 = %s' % (len(obs), ' + '.join('%s %d' % (d, dc[d]) for d in OBSERVE)))
    P('   단위: 행 %d, 서로 다른 (경로, 공유 요소) %d, 행에 적힌 지점 수의 합 %d (경로 B는 sites=N, A와 C는 행마다 1)' % (
        len(obs), len(set((r['p'], r['elem']) for r in obs)), sum(r['nsite'] for r in obs)))
    P('   그 가운데 다른 행위자의 상대가 행에 있는 것 %d, 없는 것(경로 A, 생성자 없음) %d' % (
        sum(1 for r in obs if r['pair'] > 0), sum(1 for r in obs if r['pair'] == 0)))
    P('   순서 강제(D01)인데 표시가 미확정이라 이전 집계에는 들어 있던 행 %d (포함하면 관측 대상 %d)' % (len(out_und), len(obs) + len(out_und)))
    gap_order = [r for r in rows if r['disp'] in ('D02', 'D03')]
    gap_eff = [r for r in obs if r['lab'] in ('T?', 'F')]
    P('   관측만으로 메워지지 않는 정적 정보 공백(검토 판단에 따른 두 부류):')
    P('     순서 전제 공백 %d행: 순서 문장이 확인되지 않은 전제에 기댄다(D02 %d + D03 %d). 한 실행의 관측은 그 실행의 순서만 준다' % (len(gap_order), dc['D02'], dc['D03']))
    P('     처리 효과 공백 %d행: 부재 분기의 효과가 행에 없다(T? %d, F %d). 관측은 그 분기가 실행된 경우에만 효과를 준다(실행 의존)' % (
        len(gap_eff), sum(1 for r in gap_eff if r['lab'] == 'T?'), sum(1 for r in gap_eff if r['lab'] == 'F')))
    P('     두 부류에 모두 드는 행 %d' % len([r for r in gap_order if r['lab'] in ('T?', 'F')]))
    # set (c): order candidates
    reloc = [r for r in rows if r['oc'] == 'RELOC']
    P('[집합 3] 고정 순서 후보 %d (D02; 표시 %s; 뒤에 만들어지는 독자도 함께 적힌 행 %d) / 조건부 순서 후보 %d (D03; 표시 %s) / 재배치 후보(순서 등급 RELOC) %d (표시 %s)' % (
        dc['D02'], dict(sorted(C(r['lab'] for r in rows if r['disp'] == 'D02').items())), sum(1 for r in rows if r['disp'] == 'D02' and r.get('mixed')),
        dc['D03'], dict(sorted(C(r['lab'] for r in rows if r['disp'] == 'D03').items())), len(reloc), dict(sorted(C(r['lab'] for r in reloc).items()))))
    P('   순서 등급 전체: %s' % dict(sorted(C(r['oc'] for r in rows).items())))
    d7 = [r for r in rows if r['disp'] == 'D07']
    P('   지향 실행 후보(D07) %d행: 경로 %s, 표시 %s, 도달 표시 %s; 서로 다른 (경로, 공유 요소) %d; 행에 적힌 지점 수의 합 %d' % (
        len(d7), dict(sorted(C(r['p'] for r in d7).items())), dict(sorted(C(r['lab'] for r in d7).items())), dict(sorted(C(r['reach'] for r in d7).items())),
        len(set((r['p'], r['elem']) for r in d7)), sum(r['nsite'] for r in d7)))
    P('     행: %s' % [r['id'] for r in d7])
    P('   고정 후보(D02)는 지향 실행 후보에서 제외: D02와 D07에 함께 드는 행 %d' % len([r for r in rows if r['disp'] == 'D02' and r in d7]))
    P('   보류: 재배치 후보이면서 표시 F(D10, 호출자의 처리를 사실에서 이은 뒤에야 후보가 됨) %d; 재배치 후보이면서 값과 무관(D08) %d' % (
        sum(1 for r in rows if r['disp'] == 'D10' and r['oc'] == 'RELOC'), dc['D08']))
    P('   고정 후보 행: %s' % [(r['id'], r['lab'], r['reach']) for r in rows if r['disp'] == 'D02'])
    P('   조건부 후보 가운데 표시 J·F: %s' % [(r['id'], r['lab'], r['reach']) for r in rows if r['disp'] == 'D03' and r['lab'] in ('J', 'F', 'R')])
    # reach flag
    P('[도달 표시] callee의 실패 상수가 부재 값에서 도달하는가(행의 글자만): %s' % dict(sorted(C(r['reach'] for r in rows).items())))
    P('   미확인으로 읽는 것: UNEVALUATED %d, NO-CHECK %d;  callee에만 있음: CALLEE-ONLY %d;  행이 명시: REACH-STATED %d, REACH-FLIP %d' % (
        *[sum(1 for r in rows if r['reach'] == x) for x in ('UNEVALUATED', 'NO-CHECK', 'CALLEE-ONLY', 'REACH-STATED', 'REACH-FLIP')],))
    P('   표시 x 도달 표시: %s' % {'%s/%s' % k: v for k, v in sorted(C((r['lab'], r['reach']) for r in rows).items())})
    # sets that need a run
    P('[집합 4] 생성된 계획 / 유효성 통과 / 실행 / 목표 도달 / 해제 / 비교 가능: 미실행 (이 탐침은 계획을 만들지 않고 실행하지 않는다)')
    P('[집합 5] 예산 미실행 / 계획 거부 / 기록 불충분: 미실행')
    # the earlier numbers, reproduced with the earlier formulas, and the differences
    surv = [r for r in rows if (r['lab'] in ('R', 'J') and r['pair'] > 0) or r['lab'] == 'F']
    oldE = [r for r in rows if r['lab'] == 'E']
    old_obs = surv + oldE
    old_narrow = [r for r in surv if r['oldorder'] != 'enforced' and not (r['p'] == 'B' and r['flip'] == 'none')]
    P('[이전 집계의 재현] 이전 식으로 다시 계산: 관측 집합 %d (살아남은 행 %d + E %d), 좁은 후보 %d, T? %d' % (
        len(old_obs), len(surv), len(oldE), len(old_narrow), sum(1 for r in rows if r['lab'] == 'T?')))
    ids = lambda S: set(r['id'] for r in S)
    add = [r for r in obs if r['id'] not in ids(old_obs)]
    rem = [r for r in old_obs if r['id'] not in ids(obs)]
    a_t = [r for r in add if r['lab'] == 'T?']
    a_fix = [r for r in add if r['lab'] != 'T?' and r['disp'] == 'D02']
    a_cond = [r for r in add if r['lab'] != 'T?' and r['disp'] == 'D03']
    a_other = [r for r in add if r not in a_t and r not in a_fix and r not in a_cond]
    P('[차이: 관측 집합] 이전 %d -> 지금 %d  (더해짐 %d, 빠짐 %d)' % (len(old_obs), len(obs), len(add), len(rem)))
    P('   + %d  결함 수정: T? 행을 관측 집합에 보존 (그 가운데 D02 %d, D03 %d, D11 %d)' % (len(a_t), *[sum(1 for r in a_t if r['disp'] == d) for d in ('D02', 'D03', 'D11')]))
    P('   + %d  규칙 변경(방법 문서 3.1의 정의에 맞춤): 고정 순서 후보는 표시와 무관하게 관측 집합에 든다. 표시 %s' % (len(a_fix), dict(C(r['lab'] for r in a_fix))))
    P('   + %d  규칙 변경: 조건부 순서 후보(새 등급)는 표시와 무관하게 관측한다. 표시 %s' % (len(a_cond), dict(C(r['lab'] for r in a_cond))))
    P('   + %d  그 밖(0이어야 한다)' % len(a_other))
    P('   - %d  규칙 변경: 행이 순서를 증명한 행(D01)은 표시가 미확정이어도 관측 대상에서 뺀다. 표시 %s; 그 밖의 빠짐 %d(0이어야 한다)' % (
        sum(1 for r in rem if r['disp'] == 'D01'), dict(C(r['lab'] for r in rem if r['disp'] == 'D01')), sum(1 for r in rem if r['disp'] != 'D01')))
    P('   새 사실로 인한 차이 0: 입력은 같은 relations.txt다')
    addn = [r for r in d7 if r['id'] not in ids(old_narrow)]
    remn = [r for r in old_narrow if r['id'] not in ids(d7)]
    P('[차이: 좁은 후보 -> 지향 실행 후보] 이전 %d -> 지금 %d  (더해짐 %d, 빠짐 %d)' % (len(old_narrow), len(d7), len(addn), len(remn)))
    P('   - %d  결함 수정: 고정 순서 후보(D02)를 지향 실행 후보에서 제외. 행 %s' % (sum(1 for r in remn if r['disp'] == 'D02'), [r['id'] for r in remn if r['disp'] == 'D02']))
    P('   - %d  규칙 변경(방법 문서 3.1의 S4 정의에 맞춤): 표시 F는 R·J가 아니므로 후보가 아니다(호출자의 처리가 행에 없음). 처리 %s' % (
        sum(1 for r in remn if r['lab'] == 'F'), dict(sorted(C(r['disp'] for r in remn if r['lab'] == 'F').items()))))
    P('   - %d  규칙 변경: 조건부 순서 후보(D03, 표시 R·J)를 전제가 정해질 때까지 제외. 행 %s' % (
        sum(1 for r in remn if r['disp'] == 'D03' and r['lab'] != 'F'), [r['id'] for r in remn if r['disp'] == 'D03' and r['lab'] != 'F']))
    P('   - %d  규칙 변경: 표시 R·J이나 순서 등급이 UNK·NOPAIR인 행(D09). 행 %s' % (sum(1 for r in remn if r['disp'] == 'D09'), [r['id'] for r in remn if r['disp'] == 'D09']))
    rest = [r for r in remn if not (r['disp'] in ('D02', 'D09') or r['lab'] == 'F' or (r['disp'] == 'D03'))]
    P('   그 밖의 빠짐 %d, 더해짐 %d (둘 다 0이어야 한다)' % (len(rest), len(addn)))
    P('   새 사실로 인한 차이 0')
    if show:
        for r in rows:
            if r['id'] in show:
                P('SHOW %s 표시 %s 순서 %s 도달 %s 처리 %s %s -- %s' % (r['id'], r['lab'], r['oc'], r['reach'], r['disp'], DISP[r['disp']], r['why']))
    if tsv:
        with open(tsv, 'w') as f:
            f.write('row\tsource_line\tpath\tlabel\tlabel_class\torder_class\treach\tflip\tresult\tcounterpart\tsites\telement\tdisposition\tdisposition_name\tobserve\treason\n')
            for r in rows:
                lc = 'decided' if r['lab'] in ('R', 'J', 'H', '0') else ('effect-undetermined' if r['lab'] in ('T?', 'F') else 'absence-undetermined')
                f.write('\t'.join([r['id'], str(r['line']), r['p'], r['lab'], lc, r['oc'], r['reach'], r['flip'], r['res'], str(r['pair']), str(r['nsite']),
                                   r['elem'], r['disp'], DISP[r['disp']], 'yes' if r['disp'] in OBSERVE else 'no', r['why']]) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))

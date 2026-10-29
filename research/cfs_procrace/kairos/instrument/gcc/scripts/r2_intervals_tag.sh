#!/usr/bin/env bash
# r2_intervals_tag.sh <run.kzt> <replay.txt> <out dir> -- R2 interval separation (plan/r2_intervals_20261029.md): every R2
# occurrence of a replay is given the interval it belongs to and the basis of that assignment, and the core classes are
# counted per interval.  Pilot/dev tooling: it reads a record and a replay text, it runs nothing and changes no judgement.
#   <run.kzt>     an F0 run container (kz observe; converted here with `kz convert` to the neutral event layer) or a run
#                 container that already has the neutral layer (an instrumented run: `kz convert --kzrt ... --out`)
#   <replay.txt>  the text output of `kz replay <run.kzt> ...` for the same container (any reading).  Its identity lines
#                 give the core status the sync rule needs; an F0 record has it only from a replay with --basis and
#                 --core-request-caller CFE_ES_CreateObjects (as w4w5_r2.sh); without it a core task's init is 'time'
# Intervals (plan section 2): init = before the ES main task's entry into OPERATIONAL (site es.main.operational,
# cfe_es_start.c:233-234); operational = after it and before the commanded restart (site es.restart_cmd.reset_cfe,
# cfe_es_task.c:664); shutdown = after the restart, up to the end of the record (the process exit).
# Basis of each relation to a boundary (plan section 3), strongest first; only the first three are established order:
#   po    program order: the occurrence is in the actor that records the boundary
#   sync  the startup sync: the occurrence precedes, in its actor, that actor's raise to RUNNING
#         (CFE_ES_WaitForSystemState.raise_state in a wait for OPERATIONAL, or for CORE_READY by a core application), the
#         actor was created by the main task before a successful CFE_ES_MainTaskSyncDelay(RUNNING) of the main task, and
#         that sync precedes es.main.operational in the main task (cfe_es_start.c:860-918, 218-234; cfe_es_api.c:514-576)
#   rf    the occurrence follows, in its actor, a return CFE_SUCCESS of CFE_ES_WaitForSystemState(min >= OPERATIONAL):
#         the wait loop read SystemState >= OPERATIONAL, whose only store is cfe_es_start.c:234 (cfe_es_api.c:585)
#   time  by recorded time only (the record's clock): an order of the record, never happens-before
#   none  no recorded time, equal times, or the boundary is not in the record
# Writes <out>/neutral.dump (kz-dump of the neutral layer), <out>/occ.tsv, <out>/boundaries.tsv, <out>/intervals.txt and
# <out>/sha256.txt.  Exit 0, or 2 when an input cannot be read or an occurrence cannot be matched to its record event.
set -uo pipefail
RUN=${1:?usage: r2_intervals_tag.sh <run.kzt> <replay.txt> <out dir>}; REP=${2:?replay text}; O=${3:?out dir}
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
KZ_DUMP=${KZ_DUMP:-/home/user/work/kairos-build/tools/kz-dump/kz-dump}
mkdir -p "$O"
[ -r "$RUN" ] && [ -r "$REP" ] || { echo "cannot read $RUN or $REP" >&2; exit 2; }
"$KZ_DUMP" "$RUN" > "$O/neutral.dump" 2> "$O/neutral.err" || { echo "kz-dump $RUN failed" >&2; exit 2; }
if grep -q '^section NEVENTS ' "$O/neutral.dump"; then
  NEUTRAL=$RUN; echo "neutral layer: in the container" > "$O/neutral.log"
else
  NEUTRAL=$O/neutral.kzt
  "$KZ_BIN" convert "$RUN" --out "$NEUTRAL" > "$O/neutral.log" 2>&1 || { echo "kz convert $RUN failed" >&2; exit 2; }
  "$KZ_DUMP" "$NEUTRAL" > "$O/neutral.dump" 2> "$O/neutral.err" || { echo "kz-dump $NEUTRAL failed" >&2; exit 2; }
fi
python3 -I - "$O/neutral.dump" "$REP" "$O" <<'EOF'
import sys, re, collections
dump, rep, O = sys.argv[1:4]
OPERATIONAL, CORE_READY, RUNNING_APPSTATE = 5, 3, 3   # default_cfe_es_extern_typedefs.h L199, L209, L279
CFE_SUCCESS = 0

def num(s):
    if s is None or s == 'none': return None
    return int(s, 16) if s.startswith('0x') else int(s)

def field(line, key):
    m = re.search(r'(?:^|\s)' + key + r'=("(?:[^"\\]|\\.)*"|\S+)', line)
    if not m: return None
    v = m.group(1)
    return v[1:-1] if v.startswith('"') else v

# ---- the neutral layer ----
sites, slotrole, actors, events, values = {}, {}, {}, {}, collections.defaultdict(dict)
for l in open(dump):
    m = re.match(r'\s*(NSITES|NSLOTS|NACTORS|NEVENTS|NVALUES)\[(\d+)\]', l)
    if not m: continue
    kind, i = m.group(1), int(m.group(2))
    if kind == 'NSITES': sites[i] = field(l, 'label')
    elif kind == 'NSLOTS': slotrole[(int(field(l, 'site')), int(field(l, 'slot')))] = field(l, 'role')
    elif kind == 'NACTORS':
        actors[i] = dict(name=field(l, 'name') or '', binding=field(l, 'binding'), native_key=num(field(l, 'native_key')),
                         backend_id=num(field(l, 'backend_id')), generation=num(field(l, 'generation')) or 0,
                         flags=field(l, 'flags') or '', owner=num(field(l, 'owner_id')), request_func=field(l, 'request_func') or '',
                         creator=num(field(l, 'creator')), creator_event=num(field(l, 'creator_event')))
    elif kind == 'NEVENTS':
        events[i] = dict(t=num(field(l, 't')), actor=int(field(l, 'actor')), aseq=int(field(l, 'aseq')), site=num(field(l, 'site')),
                         link=num(field(l, 'link')), clock='clock' in (field(l, 'flags') or ''))
    else:
        values[int(field(l, 'event'))][int(field(l, 'slot'))] = num(field(l, 'v'))
if not events:
    print('no neutral events in ' + dump, file=sys.stderr); sys.exit(2)

def val(ei, role):
    e = events[ei]
    for (s, k), r in slotrole.items():
        if s == e['site'] and r == role: return values[ei].get(k)
    return None

# actor names as the replay trace names them (lib/trace/neutral.cpp actor_names)
def prefix(b): return {'linux-tid': 'tid', 'kzrt-actor': 'a', 'kzrt-provisional': 'P'}.get(b, '?')
def akey(a):
    if a['binding'] == 'unresolved': return ''
    if a['binding'] == 'linux-tid': return str(a['native_key'])
    return str(a['backend_id'] & ~0x80000000)
def base(a): return a['name'] if a['name'] else prefix(a['binding']) + akey(a)
cnt = collections.Counter(base(a) for a in actors.values() if 'join-record' in a['flags'])
aname = {}
for i, a in actors.items():
    s = base(a)
    if 'join-record' in a['flags']:
        p = prefix(a['binding'])
        if cnt[s] > 1 and not s.startswith(p): s += '.' + p + akey(a)
        if a['generation'] != 0: s += '.g' + str(a['generation'])
    aname[i] = s
byname = {}
for i, s in aname.items(): byname.setdefault(s, i)

label = lambda ei: sites.get(events[ei]['site'])
per_actor = collections.defaultdict(list)
for ei in sorted(events): per_actor[events[ei]['actor']].append(ei)
def evs(lab): return [ei for ei in sorted(events) if label(ei) == lab]

B1, B2, SR = evs('es.main.operational'), evs('es.restart_cmd.reset_cfe'), evs('sb.sub_reporting.set')
b1 = B1[0] if len(B1) == 1 else None
b2 = B2[0] if len(B2) == 1 else None
main = events[b1]['actor'] if b1 is not None else None

# core applications: the replay's own decision where it printed one (its identity lines, from --basis and
# --core-request-caller: "core service" / "not a core service", or "requested from F"), else the requesting function the
# instrumented binding recorded (NACTORS request_func); the core start's requests come from CFE_ES_CreateObjects
# (cfe_es_start.c:810-814; SPEC 3.3)
core_by_name = {}
for l in open(rep):
    m = re.match(r'identity (\S+): (.*)$', l.rstrip('\n'))
    if not m: continue
    if m.group(2).endswith(', core service'): core_by_name[m.group(1)] = True
    elif m.group(2).endswith(', not a core service'): core_by_name[m.group(1)] = False
    else:
        r = re.search(r', requested from (\S+)', m.group(2))
        if r: core_by_name[m.group(1)] = r.group(1) == 'CFE_ES_CreateObjects'
def is_core(a):
    if aname[a] in core_by_name: return core_by_name[aname[a]]
    return actors[a]['request_func'] == 'CFE_ES_CreateObjects'

# waits: entry (min_system_state), return (status) linked to the entry; the raise inside the open wait call
wait_ok_ret, running_raise = {}, {}
for a, lst in per_actor.items():
    open_entry = None
    for ei in lst:
        lb = label(ei)
        if lb == 'es.wait.entry': open_entry = ei
        elif lb == 'CFE_ES_WaitForSystemState.raise_state' and open_entry is not None:
            mn = val(open_entry, 'min_system_state')
            core = is_core(a)
            # the level raised (cfe_es_api.c:536-569): RUNNING or above for min >= OPERATIONAL (any type), RUNNING for a
            # core application's min >= CORE_READY; a core application is one the core start requested (SPEC 3.3)
            if mn is not None and (mn >= OPERATIONAL or (mn >= CORE_READY and core)):
                running_raise.setdefault(a, events[ei]['aseq'])
        elif lb == 'es.wait.ret':
            en = events[ei]['link'] if events[ei]['link'] in events else open_entry
            mn = val(en, 'min_system_state') if en is not None else None
            if val(ei, 'status') == CFE_SUCCESS and mn is not None and mn >= OPERATIONAL:
                wait_ok_ret.setdefault(a, events[ei]['aseq'])
            open_entry = None

# the main task's successful RUNNING syncs before es.main.operational (entry aseq)
syncs = []
if main is not None:
    open_entry = None
    for ei in per_actor[main]:
        if events[ei]['aseq'] > events[b1]['aseq']: break
        lb = label(ei)
        if lb == 'es.sync.entry': open_entry = ei
        elif lb == 'es.sync.ret' and open_entry is not None:
            st = val(open_entry, 'app_state')
            if val(ei, 'status') == CFE_SUCCESS and st is not None and st >= RUNNING_APPSTATE: syncs.append(events[open_entry]['aseq'])
            open_entry = None
def polled(a):
    c, ce = actors[a]['creator'], actors[a]['creator_event']
    if main is None or c != main or ce is None or ce not in events: return False
    return any(events[ce]['aseq'] < s for s in syncs)

def by_time(ei, bi):
    e, b = events[ei], events[bi]
    if not (e['clock'] and b['clock']) or e['t'] == b['t']: return ('?', 'none')
    return ('before' if e['t'] < b['t'] else 'after', 'time')

def rel_b1(ei):
    if b1 is None: return ('?', 'none')
    a, q = events[ei]['actor'], events[ei]['aseq']
    if a == main: return ('before' if q < events[b1]['aseq'] else 'after', 'po')
    if a in wait_ok_ret and q > wait_ok_ret[a]: return ('after', 'rf')
    if a in running_raise and q < running_raise[a] and polled(a): return ('before', 'sync')
    return by_time(ei, b1)

def rel_b2(ei):
    if b2 is None: return ('?', 'none')
    a, q = events[ei]['actor'], events[ei]['aseq']
    if a == events[b2]['actor']: return ('before' if q < events[b2]['aseq'] else 'after', 'po')
    return by_time(ei, b2)

EST = {'po', 'sync', 'rf'}
def interval(r1, r2):
    if r1[0] == 'before' and r2[0] == 'after': return ('inconsistent', r1[1] + '/' + r2[1], 'no')
    if r1[0] == 'before': return ('init', r1[1], 'yes' if r1[1] in EST else 'no')
    if r2[0] == 'after': return ('shutdown', r2[1], 'yes' if r2[1] in EST else 'no')
    if r1[0] == 'after' and r2[0] == 'before':
        return ('operational', r1[1] + '/' + r2[1], 'yes' if r1[1] in EST and r2[1] in EST else 'no')
    if r1[0] == 'after' and b2 is None: return ('after-operational', r1[1] + '/restart-not-recorded', 'no')
    return ('undetermined', r1[1] + '/' + r2[1], 'no')

# ---- the replay text: trace event table and occurrences ----
tr_site, rank, occs, cur = {}, {}, [], None
calls_per_actor = collections.Counter()
section = None
for l in open(rep):
    l = l.rstrip('\n')
    if l == 'event table': section = 'events'; continue
    if section == 'events':
        m = re.match(r'  #(\d+) (\S+) (\S+) (\S+)$', l)
        if m:
            n, a, s, ph = int(m.group(1)), m.group(2), m.group(3), m.group(4)
            if s == 'CFE_EVS_SendEventWithAppID' and ph == 'call':
                calls_per_actor[a] += 1; rank[n] = (a, calls_per_actor[a])
            continue
        if l.startswith('edges '): section = None
    m = re.match(r'occurrence (\S+) actor (\S+) ordinal (\d+) key \[([^\]]*)\] call #(\d+)', l)
    if m:
        cur = dict(rspec=m.group(1), actor=m.group(2), ordinal=int(m.group(3)), key=m.group(4), call=int(m.group(5)),
                   status='?', scope='?', summary='?'); occs.append(cur); continue
    if cur is not None:
        m = re.match(r'  status (\S+)(?:.* scope (default|teardown))?', l)
        if m: cur['status'] = m.group(1); cur['scope'] = m.group(2) or '-'; continue
        m = re.match(r'  summary (\S+)', l)
        if m: cur['summary'] = m.group(1)
if not occs:
    print('no occurrence lines in ' + rep, file=sys.stderr); sys.exit(2)

sends = collections.defaultdict(list)
for a, lst in per_actor.items():
    for ei in lst:
        if label(ei) == 'evs.send.entry': sends[aname[a]].append(ei)
problems = []
for a, n in calls_per_actor.items():
    if len(sends.get(a, [])) != n: problems.append('actor %s: %d calls in the replay trace, %d evs.send.entry events in the record' % (a, n, len(sends.get(a, []))))

rows = []
for i, o in enumerate(occs):
    r = rank.get(o['call'])
    ei = sends[r[0]][r[1] - 1] if r and r[0] == o['actor'] and len(sends.get(r[0], [])) >= r[1] else None
    keyv = num(o['key']) if re.fullmatch(r'0x[0-9a-f]+|\d+', o['key']) else None
    if ei is None or val(ei, 'app_id') != keyv:
        problems.append('occurrence %d (actor %s call #%d key [%s]): no matching record event' % (i, o['actor'], o['call'], o['key'])); continue
    a = events[ei]['actor']; appid = val(ei, 'app_id'); owner = actors[a]['owner']
    cls = 'appid0' if appid == 0 else 'owner-unknown' if owner is None else 'self' if owner == appid else 'other'
    r1, r2 = rel_b1(ei), rel_b2(ei)
    iv, basis, est = interval(r1, r2)
    rows.append([i, o['actor'], '%#x' % owner if owner is not None else 'none', '%#x' % appid, val(ei, 'event_id'), cls, ei,
                 events[ei]['aseq'], events[ei]['t'] if events[ei]['clock'] else '-', r1[0], r1[1], r2[0], r2[1], iv, basis, est,
                 o['status'], o['summary'], o['scope']])
hdr = ['occ', 'actor', 'owner', 'app_id', 'event_id', 'class', 'nevent', 'aseq', 't_ns', 'vs_operational', 'basis1',
       'vs_restart', 'basis2', 'interval', 'assigned_by', 'established', 'status', 'summary', 'replay_scope']
with open(O + '/occ.tsv', 'w') as f:
    f.write('\t'.join(hdr) + '\n')
    for r in rows: f.write('\t'.join(str(x) for x in r) + '\n')

with open(O + '/boundaries.tsv', 'w') as f:
    f.write('boundary\tnevent\tactor\taseq\tt_ns\tvalue\n')
    for nm, lst, role in (('operational', B1, None), ('restart', B2, 'reset_type'), ('sub_reporting', SR, 'state')):
        for ei in lst:
            f.write('%s\t%d\t%s\t%d\t%s\t%s\n' % (nm, ei, aname[events[ei]['actor']], events[ei]['aseq'],
                    events[ei]['t'] if events[ei]['clock'] else '-', val(ei, role) if role else '-'))

out = []
P = out.append
P('r2_intervals_tag: record %s, replay %s' % (dump, rep))
P('boundaries: es.main.operational %d, es.restart_cmd.reset_cfe %d, sb.sub_reporting.set %d (boundaries.tsv)' % (len(B1), len(B2), len(SR)))
if len(B1) != 1: P('  OPERATIONAL boundary: %s; relations to it are not assigned' % ('not in the record (a plan before sites 35-37 / 42-44)' if not B1 else 'more than one event'))
if len(B2) > 1: P('  restart boundary: more than one event; relations to it are not assigned')
if b1 is not None and b2 is not None:
    P('  OPERATIONAL vs restart: %s %s (actors %s, %s)' % (by_time(b1, b2)[0], by_time(b1, b2)[1], aname[main], aname[events[b2]['actor']]))
if b1 is not None:
    P('  main task %s: successful RUNNING syncs before OPERATIONAL %d; actors with an rf return %d, with a RUNNING raise %d (polled %d)'
      % (aname[main], len(syncs), len(wait_ok_ret), len(running_raise), sum(1 for a in running_raise if polled(a))))
P('occurrences %d, matched to record events %d; mapping problems %d' % (len(occs), len(rows), len(problems)))
for p in problems[:20]: P('  ' + p)
C = collections.Counter
P('\nper interval and class (established = every relation the interval needs is po/sync/rf):')
c = C((r[13], r[5], r[15]) for r in rows)
for iv in ('init', 'operational', 'shutdown', 'after-operational', 'undetermined', 'inconsistent'):
    ks = sorted(k for k in c if k[0] == iv)
    if ks: P('  %-17s %s' % (iv, ', '.join('%s %s=%d' % (k[1], 'est' if k[2] == 'yes' else 'time/none', c[k]) for k in ks)))
P('\nper interval, class and basis of assignment:')
c = C((r[13], r[5], r[14]) for r in rows)
for k in sorted(c): P('  %s\t%s\t%s\t%d' % (k + (c[k],)))
P('\nper interval, class and summary view:')
c = C((r[13], r[5], r[17]) for r in rows)
for k in sorted(c): P('  %s\t%s\t%s\t%d' % (k + (c[k],)))
P('\nper interval, class, AppID and EventID:')
c = C((r[13], r[5], r[3], r[4]) for r in rows)
for k in sorted(c, key=lambda k: (k[0], k[1], k[2], k[3] if k[3] is not None else -1)): P('  %s\t%s\t%s\t%s\t%d' % (k + (c[k],)))
P('\nper actor (owner): occurrences per interval [basis of assignment]:')
by_a = collections.defaultdict(C)
for r in rows: by_a[(r[1], r[2])][r[13] + '[' + r[14] + ']'] += 1
for k in sorted(by_a): P('  %s (%s): %s' % (k[0], k[1], ', '.join('%s %d' % (x, by_a[k][x]) for x in sorted(by_a[k]))))
P('\nreplay scope vs interval (the replay decides SCOPE by the merged order of the decision and the shutdown request):')
c = C((r[18], r[13]) for r in rows)
for k in sorted(c): P('  scope %s\t%s\t%d' % (k + (c[k],)))
e22 = [r for r in rows if r[4] == 22]
if SR:
    en = [ei for ei in SR if val(ei, 'state') == 1 and events[ei]['clock']]
    bad = [r for r in e22 if r[8] == '-' or not any(events[ei]['t'] < r[8] for ei in en)]
    P('\nEID 22 (subscription report) %d; with an enable (sb.sub_reporting.set state 1) earlier in recorded time %d, without %d'
      ' (the report reads the flag at cfe_sb_task.c:659; the only store of 1 is the setter)' % (len(e22), len(e22) - len(bad), len(bad)))
else:
    P('\nEID 22 (subscription report) %d; the setter of subscription reporting is not in the record' % len(e22))
open(O + '/intervals.txt', 'w').write('\n'.join(out) + '\n')
print('\n'.join(out[:8]))
sys.exit(2 if problems else 0)
EOF
rc=$?
sha256sum "$RUN" "$REP" "$O/neutral.dump" "$O/occ.tsv" "$O/boundaries.tsv" "$O/intervals.txt" "$KZ_BIN" "$KZ_DUMP" "${BASH_SOURCE[0]}" > "$O/sha256.txt" 2>/dev/null
[ "$NEUTRAL" = "$RUN" ] || sha256sum "$NEUTRAL" >> "$O/sha256.txt"
exit $rc

#!/usr/bin/env bash
# replay_c5.sh -- C5 (cFE #72) 2026-10-29: the two natural boots of run_c5.sh (records /home/user/work/c5run/{641,642},
# bundle records/c5-20261029-boots) read with ONE fixed analysis configuration: the merged tree's kz and kz-static, the
# C5 Phi profile c5-cfe-6.4.x (one instance per release) and a B' of each derived instrumented build.  Pilot/dev boots,
# not main-experiment results.  Nothing is run but kz-static (step bprime), kz, kz-phi, kz-dump and text tools.  No boot.
#
#   replay_c5.sh bprime   B' of the derived build 64<V> (derive_c5.sh instr) with the merged kz-static
#                         -> <out>/bprime/64<V>/{basis.kz,facts.kz,facts_est.kz,...}
#   replay_c5.sh config   the fixed configuration record -> <out>/config.txt
#   replay_c5.sh one <V>  one record: kz convert --kzrt, the readings cur, p1off, nocode, --emit-trace, kz occ show, the
#                         control reading cur_estfacts -> <out>/64<V>/
#   replay_c5.sh tables   kz occ diff of the two releases' cur readings; TSV summaries (python3 -I; reads only the
#                         outputs above and the records' text files) -> <out>/tables/
#   replay_c5.sh all      bprime config one 641 one 642 tables
# <out> = /home/user/work/c5run/replay.
#
# B' (step bprime).  The boots' convert used the E-ST B' (/home/user/work/c5tools/build/64<V>/est_instr/basis.kz, made by
# the C5 tools branch's kz-static 3ec20abf with the header-only profile basis/est-64<V>).  kz replay takes --facts only
# when the facts were extracted with the replay's profile ("the facts were extracted with profile <est hash>, the replay
# uses <Phi hash>", checked 2026-10-29), as docs/static_facts.md section 2 says.  So the B' is made again:
#   - inputs of E-ST unchanged (est_run.sh step 2): the compile database of est_instr (made by est_inputs.sh from the
#     instr build log), the build directory and bundle root of the derived tree, the recipe basis/core-linux.kzb;
#   - the profile is the Phi profile instance (as basis_gi.sh gives the case profile to basis and extract), not est-64<V>;
#   - extract with the four command-line operations basis_gi.sh gives (OS_TaskDelay, CFE_ES_LockSharedData,
#     CFE_ES_UnlockSharedData, CFE_ES_CreateChildTask; the same functions exist in the 6.4.x trees).  basis_gi.sh's
#     --target list names C1/OSAL 5 types and is not used.  E-ST gave no operation: facts_est.kz is that extract (no
#     operation) on the same basis, and the reading cur_estfacts is the control of this choice.
# Readings (w4w5_c1_replay.sh L64-L76 / replay_w6.sh "one"): cur --compare-narrowing; p1off --outcome-narrowing off;
# nocode --outcome-narrowing off --reads-from-outcome off; --emit-trace.  No --const (the profile has no `ref eval`
# constant), no --core-request-caller (no reference names a function whose creation requests start the 6.4.x core
# applications: they are made by CFE_ES_CreateObjects, not by a request), no --process.
# Harness choices (unspecified_by_reference): the output layout, the control reading, the TSV columns.
set -uo pipefail
STEP=${1:?bprime|config|one|tables|all}
R=/home/user/Shape-Perf/research/cfs_procrace
K=$R/kairos
PR=$K/profiles/c5-cfe-6.4.x
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KZ=$KZB/kz/kz; KZS=$KZB/kz-static/kz-static; KZD=$KZB/kz-dump/kz-dump; KZP=$KZB/kz-phi/kz-phi
RUN=/home/user/work/c5run
OUT=$RUN/replay
CT=/home/user/work/c5tools/build
SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
[ -e /home/user/work/.campaign_running ] && { echo "a campaign is running (/home/user/work/.campaign_running)" >&2; exit 3; }
prof() { echo --profile "$PR" --profile "$PR/cfe-6.4.${1#64}"; }   # the profile instance of release 64<V>

case "$STEP" in
bprime)
  for V in 641 642; do
    T=$CT/$V/b/cFE-6.4.${V#64}-OSS-release; E=$CT/$V/est_instr; D=$OUT/bprime/$V; mkdir -p "$D"
    # est_run.sh step 2 with the Phi profile instance
    $KZS basis $(prof $V) --compile-db "$E/compile_commands.json" --build-dir "$T/build/cpu1" --bundle-root "$T" \
        --recipe "$PR/basis/core-linux.kzb" --text -o "$D/basis.kz" > "$D/basis.txt" 2>&1; b=$?
    # basis_gi.sh extract operations
    $KZS extract $(prof $V) --basis "$D/basis.kz" --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData \
        --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask -o "$D/facts.kz" > "$D/extract.txt" 2>&1; x=$?
    # E-ST extract arguments (no operation): the control facts
    $KZS extract $(prof $V) --basis "$D/basis.kz" -o "$D/facts_est.kz" > "$D/extract_est.txt" 2>&1; y=$?
    for c in pthread_create signal; do $KZS show "$D/facts.kz" --section calls --callee $c > "$D/calls_$c.txt" 2>&1; done
    $KZD "$D/basis.kz" > "$D/basis.dump" 2>&1
    { echo "# replay_c5.sh bprime $V $(date -u +%FT%T.%3NZ) repo $(git -C $R rev-parse HEAD)"
      echo "basis rc=$b extract rc=$x extract_est rc=$y"
      echo "units $(sed -n 's/^  translation units *//p' "$D/basis.txt"); extract: $(grep -E '^  (translation units|functions|call sites)' "$D/extract.txt" | tr -s ' ' | tr '\n' ';')"
      echo "extract_est: $(grep -E '^  (translation units|functions|call sites)' "$D/extract_est.txt" | tr -s ' ' | tr '\n' ';')"
      echo "call facts pthread_create $(grep -c '^  site ' "$D/calls_pthread_create.txt"), signal $(grep -c '^  site ' "$D/calls_signal.txt")"
      grep -E '^(target\.elf|front_end\.lowered|startup\.(file|script)|closure\.duplicate_definitions)' "$D/basis.txt" | cut -c1-200
      echo "## E-ST B' of the boots (kz-static 3ec20abf), for comparison: $(sha256sum "$E/basis.kz" | cut -c1-64)"
      echo "## sha256"
      sha256sum "$D/basis.kz" "$D/facts.kz" "$D/facts_est.kz" "$E/compile_commands.json" "$PR/basis/core-linux.kzb" "$KZS"
    } > "$D/bprime.record"
    echo "bprime $V basis $b extract $x extract_est $y"
  done ;;
config)
  mkdir -p "$OUT"
  { echo "# replay_c5.sh config $(date -u +%FT%T.%3NZ)"
    echo "repo $(git -C $R rev-parse HEAD) diff_kairos_lines=$(git -C $R diff HEAD -- kairos | wc -l)"
    for t in "$KZ" "$KZS" "$KZD" "$KZP"; do echo "tool $t sha256=$(sha256sum "$t" | cut -c1-64)"; done
    echo "kz-static identity: $($KZS --identity 2>&1 | tr '\n' ' ')"
    echo "profiles $PR last_commit=$(git -C $R log -1 --format=%H -- kairos/profiles/c5-cfe-6.4.x) tree=$(git -C $R rev-parse HEAD:research/cfs_procrace/kairos/profiles/c5-cfe-6.4.x) uncommitted_lines=$(git -C $R diff HEAD -- kairos/profiles/c5-cfe-6.4.x | wc -l)"
    for V in 641 642; do echo "profile hash $V (kz-phi hash c5-cfe-6.4.x c5-cfe-6.4.x/cfe-6.4.${V#64}): $($KZP hash "$PR" "$PR/cfe-6.4.${V#64}" 2>/dev/null | tail -1)"; done
    echo "expected profile hash (plan/expected_results_C5.md section 1): 641 38a6db1bcdb849c9677628fa61af8d6047a979e0363c85dc827c285c04126c2e, 642 d92989a03cb21effe4f5224adf27ea906b0f70a72894bb8dbbc9ea8ccbb95303"
    echo "frozen expectations plan/expected_results_C5.md last_commit=$(git -C $R log -1 --format=%h -- plan/expected_results_C5.md) sha256=$(sha256sum $R/plan/expected_results_C5.md | cut -c1-64)"
    echo "constants: none (no ref eval constant in the profile); --core-request-caller: none; --process: none"
    echo "## profile files"; (cd $K/profiles && find c5-cfe-6.4.x -name '*.kzp' -type f | sort | xargs sha256sum)
    echo "## inputs per release (manifest of the build, binds of the build, dump of the boot)"
    for V in 641 642; do
      sha256sum "$PR/instr/sites-$V.draft.kzs" "$CT/$V/binds_instr.kzs" "$RUN/$V/dump/cpu1.kzrt"
      echo "manifest of build_instr.record ($V): $(sed -n 's/.*manifest sha256 \([0-9a-f]*\).*/\1/p' "$CT/$V/build_instr.record" | head -1)"
      echo "dump of run.meta ($V): $(sed -n 's/^dump cpu1.kzrt bytes=[0-9]* sha256=//p' "$RUN/$V/run.meta")"
    done
    echo "## B' (bprime): sha256sum -c"; for f in $OUT/bprime/*/bprime.record; do sed -n '/^## sha256/,$p' "$f"; done | grep -v '^#' | sha256sum -c - 2>&1
  } > "$OUT/config.txt"; sed -n '1,9p' "$OUT/config.txt" ;;
one)
  V=${2:?release 641|642}; O=$OUT/$V; D=$OUT/bprime/$V; MAN=$PR/instr/sites-$V.draft.kzs; BINDS=$CT/$V/binds_instr.kzs
  DUMP=$RUN/$V/dump/cpu1.kzrt
  mkdir -p "$O"; : > "$O/exit.txt"
  want=$(sed -n 's/^manifest .* sha256=\([0-9a-f]*\) .*/\1/p' "$RUN/$V/run.meta" | head -1); have=$(sha256sum "$MAN" | cut -c1-64)
  [ "$want" = "$have" ] || { echo "manifest mismatch: run.meta $want, $MAN $have" >> "$O/exit.txt"; exit 65; }
  $KZ convert --kzrt "$DUMP" --manifest "$MAN" --manifest "$BINDS" --basis "$D/basis.kz" --out "$O/run.kzt" > "$O/convert.txt" 2>&1
  echo "convert $?" >> "$O/exit.txt"
  $KZD "$O/run.kzt" > "$O/run.kzt.txt" 2>&1
  # the boot's own convert (E-ST B'): what differs apart from the basis and artifact hash lines
  diff <(grep -vE 'sha256|basis|parent|artifact' "$RUN/$V/convert/run.kzt.txt") <(grep -vE 'sha256|basis|parent|artifact' "$O/run.kzt.txt") > "$O/kzt_vs_boot.diff"
  echo "kzt_vs_boot $(grep -c '^[<>]' "$O/kzt_vs_boot.diff")" >> "$O/exit.txt"
  A=("$O/run.kzt" $(prof $V) --basis "$D/basis.kz" --facts "$D/facts.kz" --static-basis "$D/basis.kz")
  rd() {  # name args...
    local n=$1; shift
    $KZ replay "${A[@]}" "$@" --out "$O/$n.kz" > "$O/$n.txt" 2>&1; echo "$n $?" >> "$O/exit.txt"
    $KZ occ show "$O/$n.kz" > "$O/occ_$n.txt" 2>&1; echo "occ_$n $?" >> "$O/exit.txt"
  }
  rd cur --compare-narrowing
  rd p1off --outcome-narrowing off
  rd nocode --outcome-narrowing off --reads-from-outcome off
  $KZ replay "${A[@]}" --emit-trace > "$O/trace.txt" 2>&1; echo "trace $?" >> "$O/exit.txt"
  # control: the E-ST extract arguments (facts_est.kz)
  $KZ replay "$O/run.kzt" $(prof $V) --basis "$D/basis.kz" --facts "$D/facts_est.kz" --static-basis "$D/basis.kz" \
      --compare-narrowing --out "$O/cur_estfacts.kz" > "$O/cur_estfacts.txt" 2>&1; echo "cur_estfacts $?" >> "$O/exit.txt"
  $KZ occ diff "$O/cur.kz" "$O/cur_estfacts.kz" > "$O/occdiff_cur_estfacts.txt" 2>&1; echo "occdiff_cur_estfacts $?" >> "$O/exit.txt"
  sha256sum "$DUMP" "$MAN" "$BINDS" "$D/basis.kz" "$D/facts.kz" "$D/facts_est.kz" "$O"/*.kz "$O"/run.kzt "$O"/*.txt "$KZ" \
    | grep -v '/sha256.txt$' > "$O/sha256.txt"
  echo "one $V $(tr '\n' ' ' < "$O/exit.txt")" ;;
tables)
  mkdir -p "$OUT/tables"
  # release comparison (expected_results_C5.md E-16): the two cur readings, matched by kz occ diff; several fixes are mixed
  # between the releases (Trac #40-#42), which the results document marks, not this output
  $KZ occ diff "$OUT/641/cur.kz" "$OUT/642/cur.kz" > "$OUT/tables/occdiff_641_642_cur.txt" 2>&1
  echo "occdiff_641_642_cur exit $?" > "$OUT/tables/occdiff_641_642_cur.exit"
  python3 -I - "$RUN" "$OUT" <<'PY'
import os, re, sys
RUN, OUT = sys.argv[1], sys.argv[2]
T = os.path.join(OUT, 'tables')
ERR_APPID = 0xc4000001
EV = re.compile(r'^ev actor=(\d+) aseq=(\d+) site=(\d+)\[([^\]]*)\] .*? t=(\d+) .*?v=\[([^\]]*)\](?: str="([^"]*)")?')
def val(x):
    return None if x == '-' else int(x, 16) if x.startswith('0x') else int(x)
def events(v):
    ev = []
    for line in open(os.path.join(RUN, v, 'cpu1.dump.txt')):
        m = EV.match(line)
        if m:
            a, s, site, lab, t, vs, st = m.groups()
            ev.append(dict(actor=int(a), aseq=int(s), site=int(site), label=lab, t=int(t),
                           v=[val(x) for x in vs.split(',')], s=st))
    ev.sort(key=lambda e: (e['t'], e['actor'], e['aseq']))
    return ev
def tids(v):
    out = {}
    for line in open(os.path.join(RUN, v, 'cpu1.dump.txt')):
        m = re.match(r'^actor (\d+) .*key=tid:(\d+)', line)
        if m: out[int(m.group(2))] = int(m.group(1))
    return out
def sched(v):
    out = {}
    for line in open(os.path.join(RUN, v, 'state_running.txt')):
        m = re.match(r'^task (\d+) policy=(\d+) rt_priority=(\d+) nice=(-?\d+) processor=(\d+) state=(\S+)', line)
        if m: out[int(m.group(1))] = m.groups()[1:]
    return out
def hexs(x): return '-' if x is None else hex(x)
rows_task, rows_look, rows_out, rows_sched = [], [], [], []
for v in ('641', '642'):
    ev = events(v)
    # spawns (sites 13-17 of one creator in sequence) and app creates (sites 1, 9, 8, 33, 2)
    spawns, cur, apps, acur = [], {}, [], {}
    for e in ev:
        a, s = e['actor'], e['site']
        if s == 13: cur[a] = dict(creator=a, t=e['t'], name=None, thread=None, task_id=None, status=None)
        elif s == 14 and a in cur: cur[a]['name'] = e['s']
        elif s == 15 and a in cur: cur[a]['thread'] = e['v'][1]
        elif s == 16 and a in cur: cur[a]['task_id'] = e['v'][0]
        elif s == 17 and a in cur: cur[a]['status'] = e['v'][0]; spawns.append(cur.pop(a))
        if s == 1: acur[a] = dict(t=e['t'], prio=e['v'][1], pub_taskid=None, t_pub=None, appid=None, owner_out=None, status=None)
        elif s == 9 and a in acur: acur[a]['pub_taskid'] = e['v'][0]; acur[a]['t_pub'] = e['t']
        elif s == 8 and a in acur: acur[a]['appid'] = e['v'][0]
        elif s == 33 and a in acur: acur[a]['owner_out'] = e['v'][0]
        elif s == 2 and a in acur: acur[a]['status'] = e['v'][0]; apps.append(acur.pop(a))
    first = {}
    for e in ev: first.setdefault(e['actor'], e)
    ann = {e['actor']: e['v'][0] for e in ev if e['site'] == 20}
    selfth = {e['actor']: e['v'][0] for e in ev if e['site'] == 19}
    main_sample = sorted({e['actor'] for e in ev if e['site'] == 32})
    # the actor's own spawn: the latest spawn whose pthread_t equals the actor's pthread_self and that precedes its first event
    own = {}
    for a in sorted(first):
        c = [sp for sp in spawns if selfth.get(a) is not None and sp['thread'] == selfth[a] and sp['t'] < first[a]['t']]
        if c: own[a] = c[-1]
    pub_t = {ap['pub_taskid']: ap for ap in apps}
    by_task = {}
    for sp in spawns: by_task[sp['task_id']] = sp
    # lookups: GetAppID (23 entry, 24 task, 25 store_ok, 26 ret) and GetAppIDInternal (27 task, 28 store_ok, 29 ret)
    looks, open_ = [], {}
    for e in ev:
        a, s = e['actor'], e['site']
        # GetAppID opens at its entry (site 23); GetAppIDInternal has no entry site and opens at its point (site 27)
        if s in (23, 27): open_[a] = dict(actor=a, api='GetAppID' if s == 23 else 'GetAppIDInternal', t=e['t'], task_id=None, app_id=None, status=None)
        L = open_.get(a)
        if L is None: continue
        if s in (24, 27): L['task_id'] = e['v'][0]
        elif s in (25, 28): L['app_id'] = e['v'][0]
        elif s in (26, 29): L['status'] = e['v'][0]; looks.append(open_.pop(a))
    # rows per actor
    tid_actor = {a: t for t, a in tids(v).items()}
    sc = sched(v)
    for a in sorted(first):
        sp = own.get(a)
        name = sp['name'] if sp else ('initial thread' if a == 1 else '?')
        ap = pub_t.get(sp['task_id']) if sp else None
        L = [l for l in looks if l['actor'] == a]
        Lg = [l for l in L if l['api'] == 'GetAppID']
        f = Lg[0] if Lg else None
        own_pub = ap['t_pub'] if ap else None
        ann_pub = pub_t[ann[a]]['t_pub'] if a in ann and ann[a] in pub_t else None
        rows_task.append([v, 'a%d' % a, str(tid_actor.get(a, '-')), name, hexs(sp['task_id'] if sp else None),
            hexs(ann.get(a)), hexs(ap['appid'] if ap else None), hexs(ap['owner_out'] if ap else None),
            'yes' if a in main_sample else 'no',
            str(len(Lg)), str(len(L) - len(Lg)),
            ','.join(sorted({hexs(l['task_id']) for l in L})) or '-',
            ','.join(sorted({hexs(l['app_id']) for l in L})) or '-',
            ','.join(sorted({hexs(l['status']) for l in L})) or '-',
            hexs(f['task_id']) if f else '-', hexs(f['app_id']) if f else '-', hexs(f['status']) if f else '-',
            ('%+.3f' % ((f['t'] - own_pub) / 1e6)) if f and own_pub else '-',
            ('%+.3f' % ((f['t'] - ann_pub) / 1e6)) if f and ann_pub else '-'])
        s = sc.get(tid_actor.get(a, -1))
        rows_sched.append([v, 'a%d' % a, str(tid_actor.get(a, '-')), name] + (list(s[:2]) if s else ['-', '-']))
    for l in looks:
        rows_look.append([v, 'a%d' % l['actor'], l['api'], str(l['t']), hexs(l['task_id']), hexs(l['app_id']), hexs(l['status'])])
    o1 = sum(1 for l in looks if l['status'] == ERR_APPID)
    other = sum(1 for l in looks if l['status'] not in (0, ERR_APPID))
    s30 = [e for e in ev if e['site'] == 30]
    s31 = [e for e in ev if e['site'] == 31]
    s21 = [e for e in ev if e['site'] == 21]
    app_actors = [a for a in own if pub_t.get(own[a]['task_id'])]
    o2 = [(a, l['app_id'], pub_t[own[a]['task_id']]['owner_out']) for a in app_actors for l in looks
          if l['actor'] == a and l['status'] == 0 and l['app_id'] != pub_t[own[a]['task_id']]['owner_out']]
    rows_out.append([v, str(len(looks)), str(sum(1 for l in looks if l['api'] == 'GetAppID')),
        str(sum(1 for l in looks if l['api'] != 'GetAppID')), str(o1), str(other),
        str(len(o2)), ';'.join(sorted({'a%d:%s!=%s' % (a, hexs(x), hexs(y)) for a, x, y in o2})) or '-',
        str(sum(1 for e in s30 if e['v'][0] != 0)), str(sum(1 for e in s30 if e['v'][1] == 0xffffffff)),
        str(len(s31)), str(sum(1 for e in s31 if e['v'][0] != 0)), str(sum(1 for e in s21 if e['v'][0] != 0)),
        str(sum(1 for a in app_actors for l in looks if l['actor'] == a and l['task_id'] == 0)),
        ','.join('a%d' % a for a in main_sample) or '-'])
def w(name, head, rows):
    with open(os.path.join(T, name), 'w') as f:
        f.write('\t'.join(head) + '\n')
        for r in rows: f.write('\t'.join(r) + '\n')
w('tasks.tsv', ['rel', 'actor', 'tid', 'own_spawn_name', 'own_spawn_task_id', 'announced_id', 'pub_appid_own', 'owner_out_own',
    'SAMPLE_AppMain_entry', 'n_GetAppID', 'n_GetAppIDInternal', 'task_ids_read', 'app_ids_returned', 'statuses',
    'first_task_id', 'first_app_id', 'first_status', 'first_minus_own_pub_ms', 'first_minus_announced_pub_ms'], rows_task)
w('lookups.tsv', ['rel', 'actor', 'api', 't_ns', 'task_id', 'app_id', 'status'], rows_look)
w('outcomes.tsv', ['rel', 'lookups', 'GetAppID', 'GetAppIDInternal', 'O1_ERR_APPID', 'other_status', 'O2_app_tasks',
    'O2_detail', 'site30_status_nonzero', 'site30_appid_ffffffff', 'site31', 'site31_status_nonzero', 'site21_status_nonzero',
    'app_task_task_id_0', 'site32_actor'], rows_out)
w('sched.tsv', ['rel', 'actor', 'tid', 'own_spawn_name', 'policy(1=FIFO)', 'rt_priority'], rows_sched)
# replay readings: the counts each reading prints and the per-occurrence lines of kz occ show
rows_sum, rows_occ = [], []
for v in ('641', '642'):
    for r in ('cur', 'p1off', 'nocode', 'cur_estfacts'):
        txt = open(os.path.join(OUT, v, r + '.txt')).read()
        occ = re.search(r'^occurrences (\d+)', txt, re.M)
        fnd = re.search(r'^findings (\d+)', txt, re.M)
        blk = txt.split('summary counts (outcome narrowing ')[1] if 'summary counts (outcome narrowing ' in txt else ''
        obs = ';'.join('%s %s' % m for m in re.findall(r'^  observation (\S+) (\d+)', blk.split('\nsummary counts')[0], re.M))
        rel = ';'.join('%s %s' % m for m in re.findall(r'^  relation (\S+) (\d+)$', blk.split('\nsummary counts')[0], re.M))
        bas = ';'.join('%s %s' % m for m in re.findall(r'^  basis combination (.+) (\d+)$', blk.split('\nsummary counts')[0], re.M))
        st = {}
        for m in re.finditer(r'^  status (\S+)', txt, re.M): st[m.group(1)] = st.get(m.group(1), 0) + 1
        why = {}
        for m in re.finditer(r'^  summary (\S+)', txt, re.M): why[m.group(1)] = why.get(m.group(1), 0) + 1
        fk = {}
        for m in re.finditer(r'^finding #\d+: (.*?) by actor a\d+$|^finding #\d+: (.*?) of the actor a\d+$', txt, re.M):
            k = (m.group(1) or m.group(2)); fk[k] = fk.get(k, 0) + 1
        rows_sum.append([v, r, occ.group(1) if occ else '-', ';'.join('%s %d' % kv for kv in sorted(st.items())),
                         ';'.join('%s %d' % kv for kv in sorted(why.items())), obs or '-', rel or '-', bas or '-',
                         fnd.group(1) if fnd else '-', ';'.join('%s: %d' % kv for kv in sorted(fk.items())) or '-'])
        if r == 'cur_estfacts': continue
        for line in open(os.path.join(OUT, v, 'occ_' + r + '.txt')):
            if line.startswith('#'):
                rows_occ.append([v, r] + [x.strip() for x in line.rstrip('\n').split('|')])
w('readings.tsv', ['rel', 'reading', 'occurrences', 'status', 'summary', 'observation(selected reading)', 'relation',
    'basis combination', 'findings', 'finding kinds'], rows_sum)
w('occ.tsv', ['rel', 'reading', 'occ', 'op', 'app', 'key', 'ordinal', 'exec', 'status summary', 'basis', 'edges'], rows_occ)
print('tables:', ', '.join(sorted(os.listdir(T))))
PY
  ;;
all) "$SELF" bprime && "$SELF" config && "$SELF" one 641 && "$SELF" one 642 && "$SELF" tables ;;
*) echo "usage: replay_c5.sh bprime|config|one <V>|tables|all" >&2; exit 64 ;;
esac

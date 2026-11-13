#!/usr/bin/env bash
# r2_intervals_selftest.sh <work dir> [<rt dir> <binding list>] -- tool test of r2_intervals_tag.sh on the two record paths
# and of the replay's shutdown scope on the instrumented path (plan/r2_intervals_20261029.md sections 4 and 6).  The
# values here are TOOL-TEST values, not research conditions and not results: no cFS is started.
#   instrumented path  a small C program linked with the derived build's runtime (<rt dir>, default
#                      /home/user/work/instr/gcc/d4/rt-r2) emits, with the bind ids of the derived build's binding list
#                      (<binding list>, default /home/user/work/instr/gcc/d4/r2/binds_instr.kzs), the records of a fixed
#                      synthetic schedule: the core start (ES, SB), two applications (one with the startup sync, one
#                      without), the RUNNING syncs of the main task, OPERATIONAL, the subscription-reporting setter, the
#                      commanded restart followed in the ES task by the profile's shutdown request
#                      (CFE_PSP_Restart.request_shutdown), the main task's teardown point at the end, and sends of the
#                      event function before and after each boundary.  The steps are serialized by a condition variable,
#                      so the recorded times follow the step order.  Two schedules: "base" (the ES task's restart and
#                      request at step 16, then TO_LAB's last send at step 17) and "swap" (the same two steps exchanged:
#                      only the recorded order of the other actor's send and the request differs).  Each dump is
#                      converted (kz convert --kzrt), replayed with profiles/cfs-v7.0.1 and tagged; every occurrence's
#                      interval, basis of assignment, replay scope and relation to the request is compared with the
#                      expected table below.
#   F0 path            the preserved F0 record env/logs/s1r2_p1.kzt (plan sites 0-7 only, 2026-10-15) and its basis:
#                      every occurrence must match its record event, and with no boundary in that plan none is assigned.
set -uo pipefail
W=${1:?usage: r2_intervals_selftest.sh <work dir> [<rt dir> <binding list>]}
RT=${2:-/home/user/work/instr/gcc/d4/rt-r2}; BINDS=${3:-/home/user/work/instr/gcc/d4/r2/binds_instr.kzs}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KZ="$(cd "$HERE/../../.." && pwd)"; R="$(cd "$KZ/.." && pwd)"
P=$KZ/profiles/cfs-v7.0.1; SITES=$P/instr/sites.kzs
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
RTDUMP=$RT/kz-rt-dump
mkdir -p "$W"; : > "$W/selftest.log"
log() { echo "$*" | tee -a "$W/selftest.log"; }
fail=0

# ---- instrumented path ----
defs=()
for s in 1 2 5 6 7 8 10 11 14 15 21 22 23 38 39 42 43 44 45 46; do
  line=$(awk -v s="site=$s" '$1 == "bind" && $3 == s && / status=bound/ {print; exit}' "$BINDS")
  [ -n "$line" ] || { log "no bound binding of site $s in $BINDS"; exit 2; }
  b=$(echo "$line" | awk '{print $2}'); v=$(echo "$line" | sed -n 's/.* vals=\(0x[0-9a-f]*\).*/\1/p'); defs+=(-DB$s=$b -DV$s=${v:-0})
done
SHA=$(sha256sum "$SITES" | cut -c1-64)
grep -q "manifest=.* sha256 $SHA" "$RT/runtime.record" || { log "the runtime $RT was not built for $SITES ($SHA)"; exit 2; }
cat > "$W/harness.c" <<'EOF'
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "kz_rt.h"
/* steps run one at a time, in their number order */
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static int cur = 1;
static void at(int s) { pthread_mutex_lock(&mu); while (cur != s) pthread_cond_wait(&cv, &mu); pthread_mutex_unlock(&mu); }
static void done(void) { pthread_mutex_lock(&mu); cur++; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&mu); }
#define O(site, link, a, b, c, d) kz_rt_obs(site, B##site, link, V##site, (uint64_t)(a), (uint64_t)(b), (uint64_t)(c), (uint64_t)(d))
enum { ES = 0x110000, SB = 0x110003, TO = 0x11000f, CI = 0x110010, ILLEGAL = 0xc2000003u };
static void send(uint32_t eid, uint32_t app, uint32_t st) { uint32_t q = O(10, 0, eid, 2, app, 0); O(11, q, st, 0, 0, 0); }
static uint32_t wait_raise(uint32_t min) { uint32_t q = O(21, 0, min, 1000, 0, 0); O(23, 0, 3, 0x1000, 0, 0); return q; }
static uint32_t sync_ok(uint32_t state) { uint32_t q = O(14, 0, state, 1000, 0, 0); O(15, q, 0, 0, 0, 0); return q; }
struct task { uint32_t id; void *(*fn)(void *); pthread_t th; };
static void spawn(int core, uint32_t owner, struct task *t) {   /* the creation request of the core start or of AppCreate */
  uint32_t q1 = core ? O(1, 0, 0x5000, 0x6000, owner, 0) : O(38, 0, 0x5000, 0x6000, owner, 0);
  uint32_t q5 = O(5, 0, 0x5000, 0x6000, 100, 0);
  O(8, 0, t->id, 0x6000, 0, 0);
  pthread_create(&t->th, NULL, t->fn, t);
  O(6, q5, 0, 0, 0, 0);
  if (core) O(2, q1, 0, 0, 0, 0); else O(39, q1, 0, 0, 0, 0);
}
static void *es_task(void *a) {
  struct task *t = a; uint32_t qw;
  at(2); O(7, 0, t->id, 0, 0, 0); send(5, 0, ILLEGAL); send(10, 0, ILLEGAL); qw = wait_raise(3); done();
  at(6); O(22, qw, 0, 0, 0, 0); done();
  /* the commanded restart, then (CFE_ES_ResetCFE -> CFE_PSP_Restart) the shutdown request; a send after it in this actor */
  at(STEP_ES_RESTART); O(43, 0, 2, 0, 0, 0); O(45, 0, 0, 0, 0, 0); send(14, SB, 0); done();
  return NULL;
}
static void *sb_task(void *a) {
  struct task *t = a; uint32_t qw;
  at(4); O(7, 0, t->id, 0, 0, 0); send(5, SB, 0); qw = wait_raise(3); done();
  at(7); O(22, qw, 0, 0, 0, 0); done();
  at(13); O(44, 0, 1, 0, 0, 0); done();
  return NULL;
}
static void *to_task(void *a) {
  struct task *t = a; uint32_t qw;
  at(9); O(7, 0, t->id, 0, 0, 0); send(5, SB, 0); qw = wait_raise(5); done();
  at(12); O(22, qw, 0, 0, 0, 0); send(10, SB, 0); done();
  at(14); send(22, SB, 0); done();
  at(STEP_TO_LAST); send(25, SB, 0); done();
  return NULL;
}
static void *ci_task(void *a) {
  struct task *t = a;
  at(10); O(7, 0, t->id, 0, 0, 0); send(5, SB, 0); done();
  at(15); send(14, SB, 0); done();
  return NULL;
}
int main(void) {
  uint8_t sha[32]; const char *h = MANIFEST_SHA;
  for (int i = 0; i < 32; i++) sscanf(h + 2 * i, "%2hhx", &sha[i]);
  kz_rt_module(1, sha);
  struct task e = {0x40001, es_task, 0}, s = {0x40002, sb_task, 0}, to = {0x40003, to_task, 0}, ci = {0x40004, ci_task, 0};
  at(1); spawn(1, ES, &e); done();
  at(3); sync_ok(3); spawn(1, SB, &s); done();
  at(5); sync_ok(3); done();
  at(8); spawn(0, TO, &to); spawn(0, CI, &ci); done();
  at(11); sync_ok(3); O(42, 0, 0, 0, 0, 0); done();
  pthread_join(e.th, NULL); pthread_join(s.th, NULL); pthread_join(to.th, NULL); pthread_join(ci.th, NULL);
  O(46, 0, 0, 0, 0, 0);   /* the teardown point: after OS_DeleteAllObjects */
  return 0;
}
EOF
# expected per schedule: actor (by owner) event_id class interval assigned_by replay_scope vs_request basis_req, in time order
cat > "$W/expected_base.tsv" <<'EOF'
es	5	appid0	init	sync	default	before	po
es	10	appid0	init	sync	default	before	po
sb	5	self	init	sync	default	before	time
to	5	other	init	sync	default	before	time
ci	5	other	init	time	default	before	time
to	10	other	operational	rf/time	default	before	time
to	22	other	operational	rf/time	default	before	time
ci	14	other	operational	time/time	default	before	time
es	14	other	shutdown	po	teardown	after	po
to	25	other	shutdown	time	teardown	after	time
EOF
cat > "$W/expected_swap.tsv" <<'EOF'
es	5	appid0	init	sync	default	before	po
es	10	appid0	init	sync	default	before	po
sb	5	self	init	sync	default	before	time
to	5	other	init	sync	default	before	time
ci	5	other	init	time	default	before	time
to	10	other	operational	rf/time	default	before	time
to	22	other	operational	rf/time	default	before	time
ci	14	other	operational	time/time	default	before	time
to	25	other	operational	rf/time	default	before	time
es	14	other	shutdown	po	teardown	after	po
EOF
run_schedule() {   # <name> <step of the ES restart> <step of TO's last send>
  local N=$1 D=$W/$1
  mkdir -p "$D"
  gcc -O0 -g -I "$KZ/runtime/include" "${defs[@]}" -DMANIFEST_SHA="\"$SHA\"" -DSTEP_ES_RESTART=$2 -DSTEP_TO_LAST=$3 "$W/harness.c" \
    -o "$D/harness" -Wl,--whole-archive "$RT/libkzrt-posix.a" "$RT/libkzrt.a" -Wl,--no-whole-archive \
    "$RT/CMakeFiles/kzrt-posix-auto.dir/src/kz_rt_posix_auto.c.o" "$RT/CMakeFiles/kzrt-build-info.dir/src/kz_rt_build_info.c.o" -lpthread \
    > "$D/harness.build.log" 2>&1 || { log "$N: harness build failed"; cat "$D/harness.build.log"; return 2; }
  rm -f "$D/dump.kzrt"
  KZ_RT_MODE=record KZ_RT_DUMP="$D/dump.kzrt" "$D/harness"; log "$N: harness rc=$?"
  "$RTDUMP" --manifest "$SITES" --manifest "$BINDS" "$D/dump.kzrt" > "$D/dump.txt" 2>&1; log "$N: kz-rt-dump check rc=$?"
  "$KZ_BIN" convert --kzrt "$D/dump.kzrt" --manifest "$SITES" --manifest "$BINDS" --out "$D/run.kzt" > "$D/convert.log" 2>&1; log "$N: convert rc=$?"
  "$KZ_BIN" replay "$D/run.kzt" --profile "$P" --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --out "$D/cur.kz" > "$D/replay.txt" 2>&1; log "$N: replay rc=$?"
  "$HERE/r2_intervals_tag.sh" "$D/run.kzt" "$D/replay.txt" "$D/tag" > /dev/null; log "$N: tag rc=$?"
  log "$N: replay $(grep -E '^(shutdown_request|terminated) ' "$D/replay.txt" | tr '\n' ';')"
  python3 -I - "$D/tag/occ.tsv" "$W/expected_$N.tsv" "$N" <<'EOF' | tee -a "$W/selftest.log"
import sys
rows = [l.rstrip('\n').split('\t') for l in open(sys.argv[1])][1:]
exp = [tuple(l.rstrip('\n').split('\t')) for l in open(sys.argv[2])]
own = {'0x110000': 'es', '0x110003': 'sb', '0x11000f': 'to', '0x110010': 'ci'}
got = [g[1:] for g in sorted((int(r[8]), own.get(r[2], r[2]), r[4], r[5], r[13], r[14], r[18], r[19], r[20]) for r in rows)]
bad = [(e, g) for e, g in zip(exp, got) if e != tuple(g)]
print('%s: occurrences %d (expected %d), differing %d' % (sys.argv[3], len(got), len(exp), len(bad) + abs(len(got) - len(exp))))
for e, g in bad: print('  expected %s got %s' % ('\t'.join(e), '\t'.join(g)))
sys.exit(0 if not bad and len(got) == len(exp) else 1)
EOF
  [ "${PIPESTATUS[0]}" = 0 ] || return 1
  grep -q '^EID 22 (subscription report) 1; with an enable (sb.sub_reporting.set state 1) earlier in recorded time 1, without 0' "$D/tag/intervals.txt" \
    && log "$N: EID 22 after the enable: ok" || { log "$N: EID 22 check line differs"; return 1; }
}
run_schedule base 16 17 || fail=1
run_schedule swap 17 16 || fail=1

# ---- F0 path ----
F0=$R/env/logs/s1r2_p1.kzt; F0B=$R/env/logs/s1r2_p1_basis.kz
"$KZ_BIN" replay "$F0" --profile "$P" --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --basis "$F0B" > "$W/f0_replay.txt" 2>&1; log "f0 replay rc=$?"
"$HERE/r2_intervals_tag.sh" "$F0" "$W/f0_replay.txt" "$W/tag_f0" > /dev/null; log "tag (F0) rc=$?"
if grep -q '^occurrences 271, matched to record events 271; mapping problems 0' "$W/tag_f0/intervals.txt" \
   && grep -q '^  OPERATIONAL boundary: not in the record' "$W/tag_f0/intervals.txt" \
   && [ "$(awk -F'\t' 'NR > 1 && $14 != "undetermined"' "$W/tag_f0/occ.tsv" | wc -l)" = 0 ]; then
  log "F0 path: 271 occurrences matched, no boundary in that plan, none assigned: ok"
else log "F0 path: differs (see $W/tag_f0/intervals.txt)"; fail=1; fi
sha256sum "$W/harness.c" "$W"/{base,swap}/{harness,dump.kzrt,run.kzt,replay.txt,cur.kz} "$W"/{base,swap}/tag/occ.tsv "$W/f0_replay.txt" \
  "$W/tag_f0/occ.tsv" "$BINDS" "$SITES" "$RT/runtime.record" "$F0" "$KZ_BIN" > "$W/sha256.txt" 2>/dev/null
log "selftest $([ $fail = 0 ] && echo passed || echo FAILED)"
exit $fail

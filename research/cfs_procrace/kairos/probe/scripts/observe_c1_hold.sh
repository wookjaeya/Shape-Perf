#!/usr/bin/env bash
# observe_c1_hold.sh -- one controlled hold run of a C1 W3 build: `kz hold` (F0, SPEC 4.13) wraps
# `kz observe` (the perf-sample observer), which runs one cFS instance.  Label: controlled.  For the
# post-fix (833fdbb) builds this is stage B (E-06, controlled per P-C1HOLD); for the pre-fix
# (281a941) builds this is stage C (controlled: researcher intervention, outside P-C1HOLD,
# PI_DECISIONS_RESOLVED_20261025 section 2 C).  The gate/release come from the hold plan
# profiles/c1-bp-b051eb0/hold/hold-cf<REV>.kzh; the observation plan is observe/sites-cf<REV>.kzp.
#
#   observe_c1_hold.sh <cfg pre|post> <cand 1df262a|1b0b338> <inst tx|rx> <kz> --out-dir DIR --tracefs DIR [--closure DIR]
#   observe_c1_hold.sh --one <run dir> <inst> <meta>     (internal: start one instance, wait, stop)
#
# One instance per run (PI decision P7: tx and rx are separate records; never merged).  The gate and
# the release are hardware execution breakpoints in the debug registers; the text is never modified.
# No value, application logic or resource name is changed.  The whole kz hold is wrapped in a
# `timeout` (a pilot safeguard, researcher choice): a run it ends is INCONCLUSIVE, never a result.
#
# Must run as root (the supervisor and the observer, D1); the cFS process runs as the existing
# non-root account ubuntu (cFS README L107), as in W3.  Nothing here sets a CPU, affinity, priority,
# sysctl or repetition count.
set -uo pipefail
unset KZ

C1=/home/user/work/procrace/c1_build
PROV_ROOTS=/home/user/work/procrace/c1_runs/prov_roots
U=ubuntu
SAFEGUARD_S=150   # pilot safeguard (researcher choice): wall-clock cap on the whole hold run

port_owner() { local hex inode f; hex=$(printf '%04X' "$1")
  inode=$(awk -v h=":$hex" 'NR>1 && substr($2, length($2)-4)==h {print $10; exit}' /proc/net/udp)
  [ -z "$inode" ] && return 0
  for f in /proc/[0-9]*/fd/*; do [ "$(readlink "$f" 2>/dev/null)" = "socket:[$inode]" ] && { echo "pid=$(echo "$f" | cut -d/ -f3)"; return 0; }; done
}

# ---- internal: start one instance, let it run, stop it (ES restart POWERON), as smoke_one.sh -----
if [ "${1:-}" = "--one" ]; then
  RUN="$2"; INST="$3"; M="$4"
  case "$INST" in tx) PORT=1234 ;; rx) PORT=1235 ;; *) echo "bad inst" >&2; exit 64 ;; esac
  AS=(setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups --)
  echo "instance.start_utc=$(date -u +%FT%T.%NZ)" >> "$M"
  ( cd "$RUN/inst/$INST" && exec "${AS[@]}" ./core-$INST > console_output.log 2>&1 < /dev/null ) & P=$!
  echo "${INST}_pid=$P" >> "$M"
  # Let CF run its start-up lookups/retries to their natural end (CF itself ends the retry loop); the
  # hold releases on CF's exit (stage B) or on BP's second create (stage C).  8 s covers the post-fix
  # retry window (nominal ~2.5 s) and settling; no repetition or wall-clock bound is asserted from it.
  sleep 8
  grep -q 'CFE_ES_Main entering OPERATIONAL state' "$RUN/inst/$INST/console_output.log" && echo "$INST.operational=yes" >> "$M" || echo "$INST.operational=no" >> "$M"
  ps -L -o tid,cls,rtprio,ni,pri,psr,comm -p $P >> "$M" 2>&1 || true
  own=$(port_owner $PORT); echo "udp${PORT}_owner=$own" >> "$M"
  if [ "$own" = "pid=$P" ]; then
    ( cd "$RUN/inst/host" && ./cmdUtil --port=$PORT --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) >> "$M" 2>&1
    echo "stop=ci-es-restart-poweron (CI L196-L197)" >> "$M"
  else echo "stop=skipped-port-owner-mismatch $PORT:$own" >> "$M"; fi
  sleep 5
  if kill -0 $P 2>/dev/null; then echo "pid $P alive after 5 s -> SIGKILL" >> "$M"; kill -KILL $P; fi
  wait $P; E=$?; echo "${INST}_exit=$E" >> "$M"
  echo "instance.end_utc=$(date -u +%FT%T.%NZ)" >> "$M"
  exit 0
fi

CFG="${1:?usage: $0 <pre|post> <1df262a|1b0b338> <tx|rx> <kz> --out-dir DIR --tracefs DIR}"; CAND="${2:?cand}"; INST="${3:?inst}"; KZ_BIN="${4:?kz}"; shift 4
OUT=""; TRACEFS=""; CLOSURE=""
while [ $# -gt 0 ]; do case "$1" in --out-dir) OUT="${2:?}"; shift 2 ;; --tracefs) TRACEFS="${2:?}"; shift 2 ;; --closure) CLOSURE="${2:?}"; shift 2 ;; *) echo "unknown arg: $1" >&2; exit 64 ;; esac; done
case "$CFG" in pre) REV=281a941; STAGE="C (controlled, researcher intervention, outside P-C1HOLD)" ;; post) REV=833fdbb; STAGE="B (E-06, controlled per P-C1HOLD)" ;; *) echo "cfg pre|post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a|1b0b338) ;; *) echo "cand 1df262a|1b0b338" >&2; exit 64 ;; esac
case "$INST" in tx|rx) ;; *) echo "inst tx|rx" >&2; exit 64 ;; esac
[ -n "$OUT" ] || { echo "--out-dir required (runs kept outside the repository)" >&2; exit 64; }
[ -n "$TRACEFS" ] || { echo "--tracefs required (the observer reads memory at some sites)" >&2; exit 64; }
[ "$(id -u)" = 0 ] || { echo "must run as root (the supervisor and the observer are root: D1)" >&2; exit 2; }
case "$INST" in tx) PORT=1234 ;; rx) PORT=1235 ;; esac

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; SELF="$HERE/$(basename "${BASH_SOURCE[0]}")"
KAIROS="$(cd "$HERE/../.." && pwd)"; REPO="$(cd "$KAIROS/.." && pwd)"
PROF="$KAIROS/profiles/c1-bp-b051eb0"
PLAN="$PROF/observe/sites-cf$REV.kzp"
HPLAN="$PROF/hold/hold-cf$REV.kzh"
VIEW="$PROV_ROOTS/${CFG}_$CAND"
B="$C1/$CFG/$CAND"
KZ_BIN="$(readlink -f "$KZ_BIN")"; [ -x "$KZ_BIN" ] || { echo "not executable: $KZ_BIN" >&2; exit 64; }
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
RUN="$OUT/${STAGE:0:1}_${CFG}_${CAND}_${INST}"
[ ! -e "$RUN" ] || { echo "refusing to overwrite $RUN" >&2; exit 2; }
for v in "$VIEW/cf" "$VIEW/bp" "$VIEW/cfe" "$VIEW/osal"; do [ -d "$v" ] || { echo "source view missing: $v" >&2; exit 2; }; done
if [ -n "$(port_owner $PORT)" ]; then echo "UDP $PORT busy; not starting" >&2; exit 3; fi
mkdir -p "$RUN"; D="$RUN/driver.txt"

# refuse a kz older than its sources (as observe_c1_smoke.sh)
STALE="$(find "$KAIROS/include" "$KAIROS/lib/core" "$KAIROS/lib/trace" "$KAIROS/probe/anchor" "$KAIROS/probe/plan" \
             "$KAIROS/probe/loader" "$KAIROS/probe/hold" "$KAIROS/tools/kz" \
             "$KAIROS/cmake" "$KAIROS/CMakeLists.txt" -type f ! -name '.*' -newer "$KZ_BIN" | sort | head -5)"
[ -z "$STALE" ] || { printf 'kz is older than its sources (rebuild it first):\n%s\n' "$STALE" >&2; exit 2; }
git -C "$REPO" diff --no-ext-diff --binary HEAD -- kairos > "$RUN/source.diff"
{
  echo "observe_c1_hold.sh cfg=$CFG cand=$CAND inst=$INST stage=$STAGE label=controlled start_utc=$(date -u +%FT%T.%3NZ)"
  echo "kz=$KZ_BIN sha256=$(sha256sum "$KZ_BIN" | cut -c1-64)"
  echo "kz.source: commit $(git -C "$REPO" rev-parse HEAD); uncommitted kairos/ diff sha256=$(sha256sum "$RUN/source.diff" | cut -c1-64)"
  echo "observe plan=$PLAN sha256=$(sha256sum "$PLAN" | cut -c1-64)"
  echo "hold plan=$HPLAN sha256=$(sha256sum "$HPLAN" | cut -c1-64)"
  echo "source view=$VIEW (cf $(git -C "$VIEW/cf" rev-parse HEAD), bp $(git -C "$VIEW/bp" rev-parse HEAD), cfe $(git -C "$VIEW/cfe" rev-parse HEAD), osal $(git -C "$VIEW/osal" rev-parse HEAD))"
} > "$D"

# lay out the one instance (CI tar/untar as smoke_one.sh), plus host (cmdUtil for the stop)
( cd "$B/exe" && tar -cf "$RUN/cfs.tar" "$INST" host )
mkdir "$RUN/inst" && chown $U:$U "$RUN/inst"
( cd "$RUN/inst" && setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups -- tar -xf ../cfs.tar && \
  setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups -- mkdir -p "./$INST/storage" )
rm -f "$RUN/cfs.tar"
T="$RUN/inst/$INST"
for f in core-$INST cf/cf.so cf/bp.so; do echo "file $INST/$f sha256=$(sha256sum "$T/$f" | cut -c1-64) (build $(sha256sum "$B/exe/$INST/$f" | cut -c1-64))" >> "$D"; done

# one basis per probed file (the executable is core-$INST; the profile's primary is core-tx)
"$KZ_BIN" anchor "$T/core-$INST" "$PLAN" -o "$RUN/basis_core.kz" --source-root "$VIEW" --bin core-tx >> "$D" 2>&1 || { echo "anchor core failed" >&2; exit 2; }
"$KZ_BIN" anchor "$T/cf/cf.so" "$PLAN" -o "$RUN/basis_cf.so.kz" --source-root "$VIEW" --bin cf.so >> "$D" 2>&1 || { echo "anchor cf.so failed" >&2; exit 2; }
"$KZ_BIN" anchor "$T/cf/bp.so" "$PLAN" -o "$RUN/basis_bp.so.kz" --source-root "$VIEW" --bin bp.so >> "$D" 2>&1 || { echo "anchor bp.so failed" >&2; exit 2; }

# conditions declared by the caller
TB=$'\t'
{
  echo "# conditions of C1 hold run $CFG/$CAND/$INST (stage $STAGE); label controlled; one run"
  echo "item${TB}run.purpose${TB}controlled hold run (stage $STAGE): $PLAN + hold plan $HPLAN${TB}unset"
  echo "item${TB}run.label${TB}controlled${TB}from-reference"
  echo "item${TB}run.count${TB}this run alone; repetition counts are the PI's (P8, D14)${TB}unspecified_by_reference"
  echo "item${TB}cfs.build${TB}$B (W3), unmodified files${TB}from-reference"
  echo "item${TB}cfs.user${TB}$U (setpriv; cFS README L107)${TB}from-reference"
  echo "item${TB}observer.user${TB}root (D1); supervisor root (ptrace)${TB}from-reference"
  echo "item${TB}observer.instances${TB}$INST only (P7: tx and rx are separate records)${TB}from-reference"
  echo "item${TB}hold.gate_release${TB}from $HPLAN (debug-register execution breakpoints; text not modified)${TB}from-reference"
  echo "item${TB}hold.safeguard${TB}whole run wrapped in timeout ${SAFEGUARD_S}s (pilot safeguard, researcher choice); a run it ends is INCONCLUSIVE${TB}deviation"
  echo "item${TB}run.stratum${TB}not established (files read by tar/sha256sum/kz anchor before the run)${TB}deviation"
  echo "deviation${TB}control.method${TB}W3 build-smoke and smoke1-3 ran without a hold${TB}kz hold wraps kz observe; a debug-register breakpoint holds one thread at the gate and releases on an event${TB}stage $STAGE needs the controlled order${TB}ptrace stops on the whole tree, one held thread; holder cost and ptrace x uprobe interaction recorded in hold.kzr${TB}yes"
  echo "deviation${TB}observer.instances${TB}the W3 smoke started tx and rx together${TB}this run starts $INST only${TB}P7 separate records; the gate must select one instance's thread${TB}the other instance is not run in this record${TB}yes"
  echo "item${TB}observer.tracefs${TB}$TRACEFS (operator-mounted)${TB}unspecified_by_reference"
} > "$RUN/conditions.tsv"

# the run: kz hold wraps kz observe wraps one instance (wrapped in the pilot-safeguard timeout)
M="$RUN/hold.meta"
{ echo "stage=$STAGE cfg=$CFG cand=$CAND inst=$INST label=controlled"; echo "kernel=$(uname -r)"; echo "nproc=$(nproc)"; } > "$M"
timeout "${SAFEGUARD_S}" "$KZ_BIN" hold "$RUN/basis_core.kz" "$PLAN" "$HPLAN" \
    --basis "$RUN/basis_cf.so.kz" --basis "$RUN/basis_bp.so.kz" \
    --binary core-tx="$T/core-$INST" --binary cf.so="$T/cf/cf.so" --binary bp.so="$T/cf/bp.so" \
    -o "$RUN/hold.kzr" --verbose -- \
    "$KZ_BIN" observe "$RUN/basis_core.kz" "$PLAN" --basis "$RUN/basis_cf.so.kz" --basis "$RUN/basis_bp.so.kz" \
        --binary "$T/core-$INST" -o "$RUN/run.kzt" --record "$RUN/run.kzt.record.txt" --conditions "$RUN/conditions.tsv" \
        --stratum none --run-ordinal 0 --tracefs "$TRACEFS" \
        -- "$SELF" --one "$RUN" "$INST" "$M" >> "$D" 2>&1
RC=$?
echo "timeout+kz hold exit=$RC (124 = pilot safeguard fired -> INCONCLUSIVE) end_utc=$(date -u +%FT%T.%3NZ)" >> "$D"

# counts and the replay (current reading; the Integrate agent re-replays with the new rules)
if [ -e "$RUN/run.kzt" ]; then
  CONSTS="--const CFE_PLATFORM_ES_MAX_APPLICATIONS=32"
  "$KZ_BIN" hits "$RUN/run.kzt" > "$RUN/hits.txt" 2>&1 || true
  CUR=()
  if [ -n "$CLOSURE" ] && [ -f "$CLOSURE/c1prof/closure.facts.kz" ]; then
    CUR=(--basis "$RUN/basis_core.kz" --facts "$CLOSURE/c1prof/closure.facts.kz" --static-basis "$CLOSURE/c1prof/closure.basis.kz")
  fi
  "$KZ_BIN" replay "$RUN/run.kzt" --profile "$PROF" --profile "$PROF/cf-$REV" --profile "$PROF/bundle-$CAND" --profile "$PROF/rspec-$REV" $CONSTS "${CUR[@]}" --compare-narrowing \
      --out "$RUN/replay.kz" > "$RUN/replay.txt" 2>&1; echo "kz replay exit=$?" >> "$D"
fi

# CF retry evidence from the console (attempts and the init result), for the run note
grep -n -iE 'failed to get sem id|CF Initialized|Created semaphore|Found semaphore|terminating|CF:' "$T/console_output.log" > "$RUN/cf_console.txt" 2>/dev/null || true

{ echo "artifacts (sha256):"; for f in "$RUN"/*; do [ -f "$f" ] && [ "$f" != "$D" ] && echo "  $(sha256sum "$f" | cut -c1-64)  ${f##*/}"; done; } >> "$D"
echo "RUN=$RUN"
exit "$RC"

#!/usr/bin/env bash
# observe_c1_smoke.sh -- one build-smoke observation of a C1 W3 build by `kz observe` (label: build-smoke /
# pilot; not W4, not a main-experiment result).  The procedure is the W3 build-smoke of
# plan/C1_build_manifest.md section 7 (c1_build/smoke_one.sh: the start/stop steps of nasa/bp b051eb0
# .github/workflows/build-run-bp-common.yml) with the observer of docs/observer_smoke_run.md added; every
# difference from either is written into the run's conditions file (deviations) and listed in
# docs/c1_smoke_observe.md.
#
#   observe_c1_smoke.sh <cfg pre|post> <cand 1df262a|1b0b338> <kz-executable> --out-dir DIR
#   observe_c1_smoke.sh --instances <instance dir> <meta file>      (internal: the start/stop steps; the
#                                                                    command kz observe runs)
#
# Must run as root (the observer, D1); cFS runs as the existing non-root account `ubuntu` (cFS README L107,
# "as a normal user"), as in W3.  Nothing here sets a CPU, affinity, priority, sysctl or repetition count.
# Only the tx instance is probed (its three files: core-tx, cf.so, bp.so; PI decision P7 is open); rx is
# started and stopped exactly as in W3 but from its own files, which carry no probe.
set -uo pipefail
unset KZ

C1=/home/user/work/procrace/c1_build
PROV_ROOTS=/home/user/work/procrace/c1_runs/prov_roots
U=ubuntu

port_owner() { # UDP port -> pid via /proc/net/udp inode and /proc/*/fd (the host has no `ss`; smoke_one.sh)
  local hex inode f; hex=$(printf '%04X' "$1")
  inode=$(awk -v h=":$hex" 'NR>1 && substr($2, length($2)-4)==h {print $10; exit}' /proc/net/udp)
  [ -z "$inode" ] && return 0
  for f in /proc/[0-9]*/fd/*; do [ "$(readlink "$f" 2>/dev/null)" = "socket:[$inode]" ] && { echo "pid=$(echo "$f" | cut -d/ -f3)"; return 0; }; done
}

# ---- the start/stop steps (CI L168-L200 as in smoke_one.sh), run as the observer's child --------------------
if [ "${1:-}" = "--instances" ]; then
  RUN="$2"; M="$3"
  AS=(setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups --)
  { echo "instances.start_utc=$(date -u +%FT%T.%NZ)"; } >> "$M"
  ( cd "$RUN/tx" && exec "${AS[@]}" ./core-tx > console_output.log 2>&1 < /dev/null ) & TX=$!
  ( cd "$RUN/rx" && exec "${AS[@]}" ./core-rx > console_output.log 2>&1 < /dev/null ) & RX=$!
  echo "tx_pid=$TX rx_pid=$RX" >> "$M"
  sleep 5   # CI L176 (reference value)
  for n in tx rx; do grep -q 'CFE_ES_Main entering OPERATIONAL state' "$RUN/$n/console_output.log" && echo "$n.operational_at_5s=yes" >> "$M" || echo "$n.operational_at_5s=no" >> "$M"; done
  for p in $TX $RX; do
    { echo "## threads of $p ($(readlink /proc/$p/exe))"; ps -L -o tid,cls,rtprio,ni,pri,psr,comm -p $p; } >> "$M" 2>&1
  done
  own1234=$(port_owner 1234); own1235=$(port_owner 1235); echo "udp1234_owner=$own1234 udp1235_owner=$own1235" >> "$M"
  if [ "$own1234" = "pid=$TX" ] && [ "$own1235" = "pid=$RX" ]; then
    ( cd "$RUN/host" && ./cmdUtil --port=1234 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002; ./cmdUtil --port=1235 --endian=LE --pktid=0x1806 --cmdcode=2 --half=0x0002 ) >> "$M" 2>&1
    echo "stop=ci-es-restart-poweron (CI L196-L197)" >> "$M"
  else echo "stop=skipped-port-owner-mismatch 1234:$own1234 1235:$own1235" >> "$M"; fi
  sleep 5   # CI L200 (reference value)
  for p in $TX $RX; do if kill -0 $p 2>/dev/null; then echo "pid $p alive after 5 s -> SIGKILL" >> "$M"; kill -KILL $p; fi; done
  wait $TX; TXE=$?; wait $RX; RXE=$?
  echo "tx_exit=$TXE rx_exit=$RXE" >> "$M"
  echo "instances.end_utc=$(date -u +%FT%T.%NZ)" >> "$M"
  [ "$TXE" = 0 ] && [ "$RXE" = 0 ]
  exit $?
fi

CFG="${1:?usage: $0 <pre|post> <1df262a|1b0b338> <kz> --out-dir DIR}"; CAND="${2:?cand}"; KZ_BIN="${3:?kz}"; shift 3
OUT=""
while [ $# -gt 0 ]; do case "$1" in --out-dir) OUT="${2:?}"; shift 2 ;; *) echo "unknown argument: $1" >&2; exit 64 ;; esac; done
[ -n "$OUT" ] || { echo "--out-dir is required (runs are kept outside the repository)" >&2; exit 64; }
case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "cfg is pre or post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a|1b0b338) ;; *) echo "cand is 1df262a or 1b0b338" >&2; exit 64 ;; esac
[ "$(id -u)" = 0 ] || { echo "must run as root (the observer is root: D1)" >&2; exit 2; }
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SELF="$HERE/$(basename "${BASH_SOURCE[0]}")"
KAIROS="$(cd "$HERE/../.." && pwd)"
REPO="$(cd "$KAIROS/.." && pwd)"
PROF="$KAIROS/profiles/c1-bp-b051eb0"
PLAN="$PROF/observe/sites-cf$REV.kzp"
VIEW="$PROV_ROOTS/${CFG}_$CAND"
KZ_BIN="$(readlink -f "$KZ_BIN")"; [ -x "$KZ_BIN" ] || { echo "not an executable: $KZ_BIN" >&2; exit 64; }
B="$C1/$CFG/$CAND"
TAG="c1smoke_${CFG}_${CAND}"
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
RUN="$OUT/${CFG}_$CAND"
[ ! -e "$RUN" ] || { echo "refusing to overwrite $RUN" >&2; exit 2; }
for v in "$VIEW/cf" "$VIEW/bp" "$VIEW/cfe" "$VIEW/osal"; do [ -d "$v" ] || { echo "source view missing: $v" >&2; exit 2; }; done
if [ -n "$(port_owner 1234)$(port_owner 1235)" ]; then echo "UDP 1234/1235 busy; not starting" >&2; exit 3; fi
mkdir -p "$RUN"
D="$RUN/driver.txt"

# 0. provenance of the tool (as observe_cfs_smoke.sh): refuse a kz older than its sources
STALE="$(find "$KAIROS/include" "$KAIROS/lib/core" "$KAIROS/lib/trace" "$KAIROS/probe/anchor" "$KAIROS/probe/plan" "$KAIROS/probe/loader" "$KAIROS/tools/kz" \
             "$KAIROS/cmake" "$KAIROS/CMakeLists.txt" -type f ! -name '.*' -newer "$KZ_BIN" | sort | head -5)"
[ -z "$STALE" ] || { printf 'kz is older than its sources (rebuild it first):\n%s\n' "$STALE" >&2; exit 2; }
git -C "$REPO" diff --no-ext-diff --binary HEAD -- kairos > "$RUN/source.diff"
{
  echo "observe_c1_smoke.sh cfg=$CFG cand=$CAND label=build-smoke/pilot (not W4, not a main-experiment result) start_utc=$(date -u +%FT%T.%3NZ)"
  echo "kz=$KZ_BIN sha256=$(sha256sum "$KZ_BIN" | cut -c1-64)"
  echo "kz.source: repository commit $(git -C "$REPO" rev-parse HEAD); uncommitted changes of kairos/ in source.diff sha256=$(sha256sum "$RUN/source.diff" | cut -c1-64)"
  echo "plan=$PLAN sha256=$(sha256sum "$PLAN" | cut -c1-64)"
  echo "source view=$VIEW (cf $(git -C "$VIEW/cf" rev-parse HEAD), bp $(git -C "$VIEW/bp" rev-parse HEAD), cfe $(git -C "$VIEW/cfe" rev-parse HEAD), osal $(git -C "$VIEW/osal" rev-parse HEAD))"
} > "$D"

# 1. the instances (CI L133-L135 tar, L154-L155 untar, L165-L166 storage), as smoke_one.sh -- before the
#    observer, because the probes are registered on the files the instances will run
( cd "$B/exe" && tar -cf "$RUN/cfs-bpapp.tar" . )
mkdir "$RUN/inst" && chown $U:$U "$RUN/inst"
( cd "$RUN/inst" && setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups -- tar -xf ../cfs-bpapp.tar && \
  setpriv --reuid="$(id -u $U)" --regid="$(id -g $U)" --init-groups -- mkdir -p ./tx/storage ./rx/storage )
rm -f "$RUN/cfs-bpapp.tar"
T="$RUN/inst/tx"
for f in core-tx cf/cf.so cf/bp.so; do
  echo "file tx/$f sha256=$(sha256sum "$T/$f" | cut -c1-64) (build output $(sha256sum "$B/exe/tx/$f" | cut -c1-64))" >> "$D"
done

# 2. one basis per probed file
"$KZ_BIN" anchor "$T/core-tx" "$PLAN" -o "$RUN/basis_core-tx.kz" --source-root "$VIEW" --bin core-tx >> "$D" 2>&1 || { echo "anchor core-tx failed" >&2; exit 2; }
"$KZ_BIN" anchor "$T/cf/cf.so" "$PLAN" -o "$RUN/basis_cf.so.kz" --source-root "$VIEW" --bin cf.so >> "$D" 2>&1 || { echo "anchor cf.so failed" >&2; exit 2; }
"$KZ_BIN" anchor "$T/cf/bp.so" "$PLAN" -o "$RUN/basis_bp.so.kz" --source-root "$VIEW" --bin bp.so >> "$D" 2>&1 || { echo "anchor bp.so failed" >&2; exit 2; }

# 3. the conditions declared by the caller (tab separated; probe/loader/run_record.hpp)
TB=$'\t'
{
  echo "# conditions of $TAG: build-smoke/pilot observation of a W3 build (one run; not W4, not a main-experiment result)"
  echo "item${TB}run.purpose${TB}build-smoke/pilot: one observation of the W3 build $CFG/$CAND (plan/C1_build_manifest.md 7 procedure with the observer added)${TB}unset"
  echo "item${TB}run.count${TB}this run alone; repetition counts are the PI's (P8, D14)${TB}unspecified_by_reference"
  echo "item${TB}cfs.build${TB}$B (W3, plan/C1_build_manifest.md 1-6), unmodified files${TB}from-reference"
  echo "item${TB}cfs.start_stop${TB}CI L165-L200 as smoke_one.sh: tx and rx in the background, sleep 5, OPERATIONAL check, ES restart POWERON to UDP 1234/1235, sleep 5${TB}from-reference"
  echo "item${TB}cfs.user${TB}$U (setpriv; cFS README L107 as a normal user)${TB}from-reference"
  echo "item${TB}observer.user${TB}root (D1)${TB}from-reference"
  echo "item${TB}observer.instances${TB}tx only: the probes are on tx/core-tx, tx/cf/cf.so, tx/cf/bp.so; rx runs from its own files, unprobed${TB}unspecified_by_reference"
  echo "item${TB}observer.proc_capture${TB}full (kz observe default)${TB}unspecified_by_reference"
  echo "item${TB}run.stratum${TB}not established (the files are read by tar, sha256sum and kz anchor before the run)${TB}deviation"
  echo "deviation${TB}observer.method${TB}W3 build-smoke ran without an observer (plan/C1_build_manifest.md 7)${TB}kz observe, perf-sample uprobes on three files, no stop of the process${TB}the run exercises the C1 plan on the W3 builds${TB}uprobe traps on the probed paths; not measured${TB}no"
  echo "deviation${TB}procedure.order${TB}smoke_one.sh: tar, untar, then start at once${TB}tar, untar, kz anchor (three files), then kz observe starts the start/stop steps as its child${TB}the probes are registered on the inodes the instances run, before they start${TB}a few seconds between untar and start; the instances are grandchildren of kz observe (bash, setpriv)${TB}no"
  echo "deviation${TB}observer.instances${TB}P7 open: tx only or tx and rx${TB}tx only (rx runs as in W3, unprobed)${TB}the plan is per file; rx runs from separate copies (a second plan over the rx files would observe it)${TB}rx results are not observed${TB}yes"
  echo "deviation${TB}anchors.line_table${TB}the W2 anchor rules use the DWARF line table (line-start, after-call at a line)${TB}the W3 builds are Release without -g: entry/return sites and function-scoped call sites (after-call inside a named function)${TB}no line table in the built files; rebuilding with -g would change the build${TB}no line-start sites; a call site is found by its callee inside a function (one call, no branch-joined return)${TB}no"
  echo "deviation${TB}run.stratum${TB}cold = first run after a VM boot${TB}not established, RUNMETA stratum none${TB}the files are read before the run${TB}page cache warm; not measured${TB}yes"
} > "$RUN/conditions.tsv"

# 4. the observation: kz observe runs the start/stop steps as its child
M="$RUN/smoke.meta"
{ echo "label=build-smoke/pilot (not a W4 experiment, not a main-experiment result)"; echo "cfg=$CFG cand=$CAND"; echo "kernel=$(uname -r)"; echo "nproc=$(nproc)"
  echo "run_user=$U ($(id $U))"; echo "mqueue.msg_max=$(cat /proc/sys/fs/mqueue/msg_max)"; } > "$M"
"$KZ_BIN" observe "$RUN/basis_core-tx.kz" "$PLAN" --basis "$RUN/basis_cf.so.kz" --basis "$RUN/basis_bp.so.kz" \
    -o "$RUN/run.kzt" --record "$RUN/run.kzt.record.txt" --conditions "$RUN/conditions.tsv" --stratum none --run-ordinal 0 \
    -- "$SELF" --instances "$RUN/inst" "$M" >> "$D" 2>&1
RC=$?
echo "kz observe exit=$RC end_utc=$(date -u +%FT%T.%3NZ)" >> "$D"

# 5. counts and the replays (the current reading; the P3 alternative; both P1 readings side by side)
CONSTS="--const CFE_PLATFORM_ES_MAX_APPLICATIONS=32"   # bpxfer_defs/default_platform_cfg.h L491, evaluated with the build's compile command
if [ -e "$RUN/run.kzt" ]; then
  "$KZ_BIN" hits "$RUN/run.kzt" > "$RUN/hits.txt" 2>&1 || true
  "$KZ_BIN" replay "$RUN/run.kzt" --profile "$PROF" --profile "$PROF/cf-$REV" --profile "$PROF/bundle-$CAND" --profile "$PROF/rspec-$REV" $CONSTS --compare-narrowing \
      --out "$RUN/replay.kz" > "$RUN/replay.txt" 2>&1; echo "kz replay (current) exit=$?" >> "$D"
  "$KZ_BIN" replay "$RUN/run.kzt" --profile "$PROF" --profile "$PROF/cf-$REV" --profile "$PROF/bundle-$CAND" --profile "$PROF/rspec-$REV-altP3" $CONSTS \
      > "$RUN/replay_altP3.txt" 2>&1; echo "kz replay (P3 alternative, not current) exit=$?" >> "$D"
fi

# 6. the anchors against objdump, after the run (no line table: the disassembly around each probe)
"$KZ_BIN" observe "$RUN/basis_core-tx.kz" "$PLAN" --basis "$RUN/basis_cf.so.kz" --basis "$RUN/basis_bp.so.kz" --check > "$RUN/plan_check.txt" 2>&1
{
  echo "# anchor cross-check: kz observe --check, then objdump on the same files ($(objdump --version | head -1))"
  FILES=("$T/core-tx" "$T/cf/cf.so" "$T/cf/bp.so")
  grep '^  site ' "$RUN/plan_check.txt" | while read -r line; do
    func=$(echo "$line" | awk '{print $6}'); addr=$(echo "$line" | awk '{print $8}')
    fi=$(echo "$line" | sed -n 's/.*\[file \([0-9]*\)\].*/\1/p'); fi=${fi:-0}
    echo "== $line"
    objdump -d -w --disassemble="$func" "${FILES[$fi]}" | awk -v a="$(printf '%x:' "$addr")" '
      { line[NR] = $0; if ($1 == a) hit = NR }
      END { if (!hit) { print "address not found in the disassembly of the function"; exit }
            for (i = hit - 2; i <= hit + 1; i++) if (i in line) print (i == hit ? ">" : " ") line[i] }'
  done
} > "$RUN/anchor_xcheck.txt" 2>&1

{
  echo "artifacts (sha256):"
  for f in "$RUN"/*; do [ -f "$f" ] && [ "$f" != "$D" ] && echo "  $(sha256sum "$f" | cut -c1-64)  ${f##*/}"; done
} >> "$D"
exit "$RC"

#!/usr/bin/env bash
# run_c5.sh -- ONE natural boot of a C5 derived instrumented build (cFE 6.4.1 or 6.4.2, derive_c5.sh instr), group G-I
# (KZ_RT_MODE=record: recording on, no gate, no plan, no hold; PD29 D3, D4).  Pilot/dev boot, not a main-experiment result.
# One boot per configuration (P8, PD29 A7).  Report label: "original-source modern-host derived configuration, root,
# real-time priority" (PD29 D2, N-C5-1); never the original platform's scheduling.
#
#   run_c5.sh boot <V 1|2> <out dir>        as root; refuses if /home/user/work/.campaign_running exists
#   run_c5.sh check <V 1|2> <out dir>       re-reads a finished run dir and writes <out>/checks.txt (no boot)
#
# Start and stop: the build-smoke procedure (plan/c5_execution_basis_20261028.md 3.1; the smoke script smoke_64x.sh was
# lost with the container on 2026-10-29, so the steps are re-made from that record):
#   process check (pgrep core-cpu1|core-linux.bin|core-tx|core-rx), cd exe, ./core-linux.bin with no arguments (readme
#   6.4.1 L162-L163, 6.4.2 L164-L165) as root, the terminal given by a util-linux `script` pty, SIGINT stop (readme
#   control-c; cfe_psp_start.c L270, L415-L421), state before/running/after.
# Added by PD29 and plan sections:
#   D1   the boot runs in its own IPC namespace (unshare --ipc); inside, 64 is written to /proc/sys/fs/mqueue/msg_max
#        (plan/c5_queue_inventory_20261029.md 7: TO_LAB telemetry pipe depth 64, to_lab_app.c L160); msgsize_max and
#        queues_max keep the namespace defaults; the global value (10, D19) is read before and after, never written.
#        The three values and RLIMIT_MSGQUEUE are recorded inside (A3: no RLIMIT change).
#   N-C5-1  root, so OSAL 4.1.1 posix applies SCHED_FIFO (osapi.c L435-L446 main thread, L587-L640 each task) and the PSP
#        raises/lowers the start thread (cfe_psp_start.c L356-L357, L378-L379).  Recorded per run: those source lines,
#        /proc/<pid>/task/*/{sched,stat,status} while running, kernel.sched_rt_runtime_us/period, affinity, CapEff
#        (CAP_SYS_NICE = bit 23).  A run where SCHED_FIFO is not applied is recorded but not counted (checks.txt).
#   3.4  Power-On start: key files of the run directory are recorded and removed before the boot; a fresh IPC namespace
#        has no SysV segment; the first boot without arguments is PO (cfe_psp_start.c L555-L559); reset lines recorded.
#   9    the runtime store file is always on (KZ_RT_STORE, ES 12.1; on tmpfs /dev/shm, ES 12.4).  The C1/R2 G-I drivers
#        (run_gi_c1.sh, run_gi_r2.sh) set KZ_RT_MODE=record KZ_RT_DUMP KZ_RT_RUN_ID KZ_RT_INSTANCE but not KZ_RT_STORE; this
#        driver sets the same four the same way and adds KZ_RT_STORE.  After the run: the 32-bit kz-rt-dump of the build
#        (runtime.record) decodes the process's own dump; if the run ended by a signal or that dump is missing or fails
#        its check, the dump is recovered from the store (--recover, E-RT, ES 12.1) and that one is used.
# Stop rule: run_cfs.sh "operational" mode (stop_reason operational_seen) mapped to the 6.4.x line that ends startup:
#   6.4.1 "ES Startup: CFE Core Startup Complete" (cfe_es_start.c L315), 6.4.2 "ES Startup: CFE_ES_Main entering
#   OPERATIONAL state" (cfe_es_start.c L292).  Times: wait limit 30 s and settle 5 s are the build-smoke tool values
#   (3.1), unspecified_by_reference; grace = CFE_ES_APP_KILL_TIMEOUT x CFE_ES_APP_SCAN_RATE of the build's
#   cfe_platform_cfg.h (run_cfs.sh rule); after SIGINT and the grace, SIGKILL (the PSP blocks every other signal,
#   cfe_psp_start.c L346-L348, so run_cfs.sh's SIGQUIT step has no 6.4.x counterpart).
# While booting, /home/user/work/.campaign_running exists (as w6_20261029.sh); removed on exit (trap).
# Harness values that no reference fixes are written to <out>/conditions.txt as unspecified_by_reference.
set -uo pipefail
SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
R=/home/user/Shape-Perf/research/cfs_procrace
K=$R/kairos
T=/home/user/work/c5tools
KZB=/home/user/work/merge-check/kz-llvm/tools
KZ=$KZB/kz/kz; KZDUMP=$KZB/kz-dump/kz-dump
FLAG=/home/user/work/.campaign_running
LIMIT_MS=30000      # build-smoke tool value (3.1), unspecified_by_reference
SETTLE_S=5          # build-smoke tool value (3.1), unspecified_by_reference
MODE=${1:?usage: run_c5.sh boot|check <V> <out>}; V=${2:?V 1|2}; OUT=${3:?out dir}
case "$V" in 1|2) ;; *) echo "V is 1 or 2" >&2; exit 64 ;; esac
W=$T/build/64$V
TREE=$W/b/cFE-6.4.$V-OSS-release
EXE=$TREE/build/cpu1/exe
SITES=$K/profiles/c5-cfe-6.4.x/instr/sites-64$V.draft.kzs
BINDS=$W/binds_instr.kzs
RTDUMP=$W/rt32/kz-rt-dump
BP=$W/est_instr
PCFG=$TREE/build/cpu1/inc/cfe_platform_cfg.h
OSAPI=$TREE/osal/src/os/posix/osapi.c
PSPSTART=$TREE/psp/fsw/pc-linux/src/cfe_psp_start.c
if [ "$V" = 1 ]; then END_LINE="ES Startup: CFE Core Startup Complete"; END_REF="cfe_es_start.c L315"
else END_LINE="ES Startup: CFE_ES_Main entering OPERATIONAL state"; END_REF="cfe_es_start.c L292"; fi
TB=$'\t'
now_us() { echo "${EPOCHREALTIME/./}"; }

cfgval() { awk -v n="$1" '$1=="#define" && $2==n {print $3; exit}' "$PCFG"; }

# per-task scheduling of a process: raw sched/stat/status, then a table (stat fields 39 processor, 40 rt_priority, 41 policy)
sched_capture() {
  local p=$1 t rest
  echo "## per-task records of pid $p ($(readlink /proc/$p/exe 2>/dev/null)) at $(date -u +%FT%T.%3NZ)"
  for t in /proc/$p/task/*; do
    echo "### task ${t##*/} comm=$(cat $t/comm 2>/dev/null)"
    echo "stat: $(cat $t/stat 2>/dev/null)"
    sed 's/^/sched: /' $t/sched 2>/dev/null
    grep -E '^(State|Cpus_allowed_list|CapEff|voluntary_ctxt_switches|nonvoluntary_ctxt_switches):' $t/status 2>/dev/null | sed 's/^/status: /'
  done
  echo "## table: tid policy(0 OTHER,1 FIFO,2 RR) rt_priority nice processor state"
  for t in /proc/$p/task/*; do
    rest=$(sed 's/^.*) //' $t/stat 2>/dev/null) || continue
    echo "$rest" | awk -v tid="${t##*/}" '{print "task", tid, "policy=" $39, "rt_priority=" $38, "nice=" $17, "processor=" $37, "state=" $1}'
  done
}
mq_list() {   # the POSIX queues of the current IPC namespace, through a private mqueue mount
  local d; d=$(mktemp -d)
  unshare --mount --propagation private -- sh -c "mount -t mqueue none '$d' && ls -la '$d' && for f in '$d'/*; do [ -e \"\$f\" ] && echo \"\$(basename \"\$f\"): \$(cat \"\$f\")\"; done" 2>&1
  rmdir "$d"
}

# ------------------------------------------------------------------------------------------------ inside the namespace
if [ "$MODE" = __inner ]; then
  N=$OUT/ns.txt; M=$OUT/run.meta
  { echo "## inside the IPC namespace (pid $$) $(date -u +%FT%T.%3NZ)"
    echo "ipc_ns=$(readlink /proc/self/ns/ipc) mnt_ns=$(readlink /proc/self/ns/mnt) pid_ns=$(readlink /proc/self/ns/pid)"
    echo "namespace defaults: msg_max=$(cat /proc/sys/fs/mqueue/msg_max) msgsize_max=$(cat /proc/sys/fs/mqueue/msgsize_max) queues_max=$(cat /proc/sys/fs/mqueue/queues_max)"; } > "$N"
  if ! echo 64 > /proc/sys/fs/mqueue/msg_max; then echo "write of 64 to msg_max failed" >> "$N"; exit 10; fi
  { echo "after writing 64: msg_max=$(cat /proc/sys/fs/mqueue/msg_max) msgsize_max=$(cat /proc/sys/fs/mqueue/msgsize_max) queues_max=$(cat /proc/sys/fs/mqueue/queues_max)"
    echo "RLIMIT_MSGQUEUE: $(prlimit --pid $$ --msgqueue --noheadings --output SOFT,HARD,UNITS 2>&1)"
    echo "## SysV IPC of the namespace before the boot"; ipcs -m -p; ipcs -q; ipcs -s
    echo "## POSIX queues of the namespace before the boot"; mq_list; } >> "$N" 2>&1
  [ "$(cat /proc/sys/fs/mqueue/msg_max)" = 64 ] || exit 10
  STORE=$(cat "$OUT/.store_path")
  PRIV=$(mktemp -d); FIFO=$PRIV/fifo; mkfifo "$FIFO"
  CON=$OUT/console.typescript
  cd "$EXE" || exit 11
  T0=$(now_us)
  env KZ_RT_MODE=record KZ_RT_DUMP="$OUT/dump/cpu1.kzrt" KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=1 KZ_RT_STORE="$STORE" \
      script -q -f -e -c ./core-linux.bin "$CON" < /dev/null > /dev/null 2>&1 &
  SPID=$!
  until [ -s "$CON" ] || ! kill -0 $SPID 2>/dev/null; do sleep 0.01; done   # script creates the typescript first
  tail -n +1 --pid=$SPID -f -- "$CON" > "$FIFO" 2>/dev/null & TAILPID=$!
  exec {RD}<"$FIFO"
  CPID=""; FIRST_US=""; END_US=""; stop_reason=""
  DEADLINE=$(( T0 + LIMIT_MS * 1000 ))
  while :; do
    rem=$(( DEADLINE - $(now_us) ))
    [ $rem -gt 0 ] || { stop_reason=limit_reached_without_end_of_startup; break; }
    rc=0; IFS= read -r -t "$(printf '%d.%06d' $((rem/1000000)) $((rem%1000000)))" -u $RD line || rc=$?
    if [ $rc -eq 0 ]; then
      if [ -z "$CPID" ]; then CPID=$(pgrep -P $SPID | head -1); FIRST_US=$(now_us); fi
      if [[ "$line" == *"$END_LINE"* ]]; then END_US=$(now_us); stop_reason=end_of_startup_seen; break; fi
    elif [ $rc -gt 128 ]; then continue
    else stop_reason=exited_by_itself; break; fi
  done
  [ -n "$CPID" ] || CPID=$(pgrep -P $SPID | head -1)
  { echo "script_pid=$SPID core_pid=${CPID:-none} core_comm=$( [ -n "$CPID" ] && cat /proc/$CPID/comm 2>/dev/null)"
    echo "core_cmdline=$( [ -n "$CPID" ] && tr '\0' ' ' < /proc/$CPID/cmdline 2>/dev/null)"
    echo "stop_reason=$stop_reason"
    echo "first_console_line_read_ms=$( [ -n "$FIRST_US" ] && echo $(( (FIRST_US-T0)/1000 )) || echo none)"
    echo "end_of_startup_line_read_ms=$( [ -n "$END_US" ] && echo $(( (END_US-T0)/1000 )) || echo not_seen)"; } >> "$M"
  if [ "$stop_reason" = end_of_startup_seen ]; then sleep $SETTLE_S; fi
  GRACE_MS=$(cat "$OUT/.grace_ms")
  wait_exit() {   # $1 ms; 0 once script has exited (SRC set)
    local s w r=0
    sleep "$(printf '%d.%03d' $(( $1/1000 )) $(( $1%1000 )))" & s=$!
    wait -n -p w $SPID $s || r=$?
    if [ "$w" = "$SPID" ]; then SRC=$r; kill $s 2>/dev/null; wait $s 2>/dev/null; return 0; fi
    return 1
  }
  SRC=""; STOP_LOG=""
  if [ -n "$CPID" ] && kill -0 "$CPID" 2>/dev/null; then
    { echo "## running state after the settle ($(( ($(now_us)-T0)/1000 )) ms after launch)"
      sched_capture "$CPID"
      echo "## process status"; grep -E '^(Uid|Gid|CapInh|CapPrm|CapEff|CapBnd|Cpus_allowed_list|Threads):' /proc/$CPID/status
      echo "taskset: $(taskset -p $CPID 2>&1)"
      echo "## limits"; cat /proc/$CPID/limits
      echo "## cgroup"; cat /proc/$CPID/cgroup
      echo "## environment names of core-linux.bin (values not recorded except KZ_RT_*, TERM, SHELL)"
      tr '\0' '\n' < /proc/$CPID/environ | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort | paste -sd' '
      tr '\0' '\n' < /proc/$CPID/environ | grep -E '^(KZ_RT_[A-Z_]*|TERM|SHELL)='
      echo "## fds"; ls -l /proc/$CPID/fd | awk 'NR>1 {print $9, $10, $11}'
      echo "## POSIX queues (mq_probe: fd name maxmsg msgsize curmsgs flags)"; "$OUT/tools/mq_probe" "$CPID"; echo "mq_probe exit=$?"
      echo "## POSIX queues of the namespace (mqueue mount)"; mq_list
      echo "## SysV IPC of the namespace"; ipcs -m -p; ipcs -m
      echo "## key files"; ls -la --time-style=full-iso "$EXE"/.cdskeyfile "$EXE"/.resetkeyfile "$EXE"/.reservedkeyfile 2>&1; } > "$OUT/state_running.txt" 2>&1
    T_STOP=$(now_us); STOP_LOG="SIGINT@$(( (T_STOP-T0)/1000 ))ms"; kill -INT "$CPID"
    if ! wait_exit "$GRACE_MS"; then
      STOP_LOG="$STOP_LOG,SIGKILL@$(( ($(now_us)-T0)/1000 ))ms"; kill -KILL "$CPID" 2>/dev/null
      wait_exit "$GRACE_MS" || true
    fi
  else
    echo "## running state: not captured (core-linux.bin had ended: $stop_reason)" > "$OUT/state_running.txt"
  fi
  [ -n "$SRC" ] || { r=0; wait $SPID || r=$?; SRC=$r; }
  T_END=$(now_us)
  kill $TAILPID 2>/dev/null; wait $TAILPID 2>/dev/null; exec {RD}<&-; rm -rf "$PRIV"
  { echo "stop_actions=${STOP_LOG:-none}"
    echo "script_exit=$SRC (script -e: the child's exit code, or 128+signal)"
    echo "process_end_ms=$(( (T_END-T0)/1000 ))"
    echo "console.command_exit_code=$(grep -o 'COMMAND_EXIT_CODE="[0-9]*"' "$CON" | tail -1)"; } >> "$M"
  { echo "## SysV IPC of the namespace after the run"; ipcs -m -p; ipcs -m
    echo "## POSIX queues of the namespace after the run"; mq_list
    echo "msg_max at the end (namespace)=$(cat /proc/sys/fs/mqueue/msg_max)"; } >> "$N" 2>&1
  exit 0
fi

# ------------------------------------------------------------------------------------------------ check (no boot)
if [ "$MODE" = check ]; then
  OUT="$(cd "$OUT" && pwd)"; C=$OUT/console.typescript; X=$OUT/checks.txt
  pmin=$(chrt -m | awk '/SCHED_FIFO/ {split($NF,a,"/"); print a[1]}'); pmax=$(chrt -m | awk '/SCHED_FIFO/ {split($NF,a,"/"); print a[2]}')
  remap() { awk -v p=$1 -v lo=$pmin -v hi=$pmax 'BEGIN { MAX=255; pr=hi-lo+1; if (pr<0) pr=-pr; nb=int(MAX/pr); if (MAX%pr > int(pr/2)) nb++;
      o=lo+int((MAX-p)/nb); if (o>hi) o=hi; if (o<lo) o=lo; print o }'; }   # OS_PriorityRemap, osapi.c L3948-L4015 (pmin, pmax >= 0 here)
  {
    echo "# checks of $OUT ($(date -u +%FT%T.%3NZ)); cFE 6.4.$V"
    echo "## console: start and reset"
    grep -a -E 'Default Reset Type|Starting the cFE with|POWER ON RESET|PROCESSOR RESET|Clearing out CFE|Shared Memory segments' "$C"
    echo "## console: core apps created"; grep -a -E 'Core App: .* created' "$C"
    echo "## console: startup script"; grep -a -E 'Opened ES App Startup file|Loading file|Loading shared library|loaded and created|AppCreate Error|Could not|No free' "$C"
    echo "## console: end of startup ($END_REF): $(grep -a -c -F "$END_LINE" "$C") line(s)"
    grep -a -E 'Startup Sync failed|entering .* state|Startup Complete' "$C"
    echo "## console: queue and error lines"; grep -a -i -E 'queue depth|CreatePipeErr|Error|Init Failed|EXCEPTION' "$C" | head -40
    echo "## console: end"; grep -a -E 'Control-C|Exiting cFE|COMMAND_EXIT_CODE' "$C"
    echo "## scheduling while running (expected OSAL mapping: OS_PriorityRemap with SCHED_FIFO min/max $pmin/$pmax)"
    awk '/^task / {n++; split($3,a,"="); split($4,b,"="); pol[a[2]]++; if (a[2]==1) fifo[b[2]]++}
         END {printf "tasks %d", n; for (k in pol) printf "  policy%s=%d", k, pol[k]; printf "\nSCHED_FIFO priorities (rt_priority:count):"; for (k in fifo) printf " %s:%d", k, fifo[k]; print ""}' "$OUT/state_running.txt"
    printf 'expected (source priority -> OS priority):'
    for p in 61 64 68 60 25 70 0 50 60 70 80; do printf ' %s->%s' $p "$(remap $p)"; done
    echo "  (core EVS 61, SB 64, ES 68, TIME 60, TIME tone/1Hz 25, TBL 70: cfe_platform_cfg.h L1115-L1217; startup script SAMPLE_LIB 0, SAMPLE_APP 50, CI_LAB 60, TO_LAB 70, SCH_LAB 80; PSP start thread max $pmax then min $pmin, cfe_psp_start.c L356-L357, L378-L379; OS_API_Init main thread 50 osapi.c L443)"
    grep -E '^(CapEff|Cpus_allowed_list):|^taskset' "$OUT/state_running.txt" | head -3
    capeff=$(awk '/^CapEff:/ {print $2; exit}' "$OUT/state_running.txt")
    [ -n "$capeff" ] && echo "CAP_SYS_NICE (bit 23) in CapEff of core-linux.bin: $(( (0x$capeff >> 23) & 1 ))"
    echo "## run.meta"; grep -E 'stop_reason|end_of_startup|stop_actions|script_exit|command_exit|dump|recover|convert|kz-dump|store' "$OUT/run.meta"
    D=$OUT/cpu1.dump.txt; [ -s "$D" ] || D=$OUT/cpu1.recovered.dump.txt
    echo "## hook records per site ($(basename "$D"))"
    awk '/^ev /{match($0,/site=[0-9]+\[[^]]*\]/); c[substr($0,RSTART+5,RLENGTH-5)]++} END{for(k in c) print c[k], k}' "$D" | sort -t'[' -k1,1n | sort -n -k2,2 -t' ' | paste -sd' '
    echo "## identity values: spawns (site 14 name, 15 pthread_t out, 16 task id) and announcing actors (site 19 pthread_self, 20 announced id, 21 return)"
    awk '/^ev /{ split($2,a,"="); act=a[2]; match($0,/site=[0-9]+/); s=substr($0,RSTART+5,RLENGTH-5); match($0,/v=\[[^]]*\]/); v=substr($0,RSTART+3,RLENGTH-4); split(v,w,",");
      st=""; if (match($0,/str="[^"]*"/)) st=substr($0,RSTART+4,RLENGTH-4);
      if (s==14) printf "spawn creator_actor=%s name=%s", act, st; if (s==15) printf " pthread=%s", w[2]; if (s==16) printf " task_id=%s\n", w[1];
      if (s==19) printf "announce actor=%s pthread_self=%s", act, w[1]; if (s==20) printf " announced_id=%s", w[1]; if (s==21) printf " ret=%s\n", w[1];
      if (s==1) printf "app_create creator_actor=%s priority=%s\n", act, w[2]; if (s==33) printf "  owner_out app_id=%s\n", w[1] }' "$D"
    echo "## kz convert: identity join and owners (run.kzt)"
    grep -o 'identity join.*withheld [0-9]*' "$OUT/convert/convert.log"
    grep -E '^  (NACTORS|NOWNER)\[' "$OUT/convert/run.kzt.txt" | sed -E 's/ (native_key|request_addr|generation|boot_link|binding|native_kind|name|owner_app|request_func|raw|raw_owner)=[^ ]*//g'
    if [ -s "$OUT/convert/run.kzt" ]; then
      "$KZ" replay "$OUT/convert/run.kzt" --profile "$K/profiles/c5-cfe-6.4.x/basis/est-64$V/00_profile.kzp" --emit-trace > "$OUT/convert/trace_noprofile.txt" 2>&1
      echo "## kz replay --emit-trace with the header-only E-ST profile (no Phi profile; exit $?): convert/trace_noprofile.txt"; cat "$OUT/convert/trace_noprofile.txt"
    fi
  } > "$X" 2>&1
  cat "$X"; exit 0
fi

# ------------------------------------------------------------------------------------------------ boot (outside)
[ "$MODE" = boot ] || { echo "mode boot|check" >&2; exit 64; }
[ "$(id -u)" = 0 ] || { echo "must run as root (PD29 N-C5-1)" >&2; exit 2; }
[ ! -e "$FLAG" ] || { echo "refusing: $FLAG exists (another campaign or boot is running)" >&2; exit 3; }
[ ! -e "$OUT" ] || { echo "refusing to overwrite $OUT" >&2; exit 2; }
if pgrep -x 'core-cpu1|core-linux.bin|core-tx|core-rx' > /dev/null; then echo "another cFS process runs; not starting" >&2; pgrep -a -x 'core-cpu1|core-linux.bin|core-tx|core-rx' >&2; exit 3; fi
for v in $(compgen -e | grep -E '^(LD_|MALLOC_)' || true) GLIBC_TUNABLES; do
  printenv "$v" > /dev/null 2>&1 && { echo "environment variable $v is set; refusing (run_cfs.sh rule)" >&2; exit 4; }
done
for f in "$EXE/core-linux.bin" "$SITES" "$BINDS" "$RTDUMP" "$BP/basis.kz" "$PCFG" "$OSAPI" "$PSPSTART" "$KZ" "$KZDUMP"; do
  [ -e "$f" ] || { echo "missing $f" >&2; exit 5; }
done
want=$(sed -n 's/.*manifest sha256 \([0-9a-f]*\).*/\1/p' "$W/build_instr.record" | head -1)
[ "$(sha256sum "$SITES" | cut -c1-64)" = "$want" ] || { echo "manifest $SITES differs from the build's ($want)" >&2; exit 5; }
: > "$FLAG"; trap 'rm -f "$FLAG"' EXIT
mkdir -p "$OUT/dump" "$OUT/tools"; OUT="$(cd "$OUT" && pwd)"
M=$OUT/run.meta
gcc -O0 -g -Wall -Wextra -o "$OUT/tools/mq_probe" "$R/env/tools/mq_probe.c" -lrt > "$OUT/tools/mq_probe.build.log" 2>&1 || { echo "mq_probe build failed" >&2; exit 6; }
KT=$(cfgval CFE_ES_APP_KILL_TIMEOUT); SR=$(cfgval CFE_ES_APP_SCAN_RATE)
GRACE_MS=$(( KT * SR )); echo $GRACE_MS > "$OUT/.grace_ms"
STORE=/dev/shm/kzrt-c5-64$V-$(date -u +%Y%m%dT%H%M%SZ).store; echo "$STORE" > "$OUT/.store_path"
[ ! -e "$STORE" ] || { echo "store $STORE exists" >&2; exit 5; }
{ echo "label=pilot/dev natural boot of the C5 derived instrumented build 6.4.$V (derive_c5.sh instr), group G-I (record); original-source modern-host derived configuration, root, real-time priority (not a main-experiment result)"
  echo "start_utc=$(date -u +%FT%T.%3NZ)"
  echo "driver $SELF sha256=$(sha256sum "$SELF" | cut -c1-64) repo HEAD $(git -C $R rev-parse HEAD)"
  echo "kernel=$(uname -r) uname=$(uname -a)"
  echo "nproc=$(nproc) user=$(id)"
  echo "outer ipc_ns=$(readlink /proc/self/ns/ipc) outer mqueue before: msg_max=$(cat /proc/sys/fs/mqueue/msg_max) msgsize_max=$(cat /proc/sys/fs/mqueue/msgsize_max) queues_max=$(cat /proc/sys/fs/mqueue/queues_max)"
  echo "outer RLIMIT_MSGQUEUE: $(prlimit --pid $$ --msgqueue --noheadings --output SOFT,HARD,UNITS)"
  echo "kernel.sched_rt_runtime_us=$(cat /proc/sys/kernel/sched_rt_runtime_us) kernel.sched_rt_period_us=$(cat /proc/sys/kernel/sched_rt_period_us) kernel.sched_autogroup_enabled=$(cat /proc/sys/kernel/sched_autogroup_enabled 2>/dev/null || echo n/a)"
  echo "harness CapEff=$(awk '/^CapEff/ {print $2}' /proc/self/status) CAP_SYS_NICE=$(( (0x$(awk '/^CapEff/ {print $2}' /proc/self/status) >> 23) & 1 )) affinity=$(taskset -p $$ | awk '{print $NF}') nice=$(ps -o ni= -p $$ | tr -d ' ') sched=$(chrt -p $$ | tr '\n' ' ')"
  echo "harness cgroup=$(tr '\n' ' ' < /proc/self/cgroup)"
  echo "SCHED_FIFO range: $(chrt -m | grep FIFO)"
  echo "end_of_startup_line=\"$END_LINE\" ($END_REF) limit_ms=$LIMIT_MS settle_s=$SETTLE_S grace_ms=$GRACE_MS (CFE_ES_APP_KILL_TIMEOUT=$KT x CFE_ES_APP_SCAN_RATE=$SR, $PCFG)"
  echo "store=$STORE fs=$(stat -f -c %T /dev/shm)"
  echo "tool script: $(script --version) $(command -v script) sha256=$(sha256sum "$(command -v script)" | cut -c1-64); unshare $(command -v unshare) sha256=$(sha256sum "$(command -v unshare)" | cut -c1-64)"
  echo "tool shell for script -c: SHELL=${SHELL:-unset} ($(readlink -f "${SHELL:-/bin/sh}")); TERM=${TERM:-unset}"
  for f in "$EXE/core-linux.bin" "$EXE"/cf/apps/*; do echo "file ${f#$TREE/} sha256=$(sha256sum "$f" | cut -c1-64)"; done
  echo "manifest $SITES sha256=$(sha256sum "$SITES" | cut -c1-64) (build_instr.record: $want)"
  echo "bindings $BINDS sha256=$(sha256sum "$BINDS" | cut -c1-64)"
  echo "kz-rt-dump (32-bit) $RTDUMP sha256=$(sha256sum "$RTDUMP" | cut -c1-64)"
  echo "kz $KZ sha256=$(sha256sum "$KZ" | cut -c1-64); kz-dump $KZDUMP sha256=$(sha256sum "$KZDUMP" | cut -c1-64)"
  echo "B' $BP/basis.kz sha256=$(sha256sum "$BP/basis.kz" | cut -c1-64)"
  echo "mq_probe $OUT/tools/mq_probe sha256=$(sha256sum "$OUT/tools/mq_probe" | cut -c1-64) (from $R/env/tools/mq_probe.c sha256=$(sha256sum "$R/env/tools/mq_probe.c" | cut -c1-64))"
  echo "environment names of this script (values not recorded): $(env | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort | paste -sd' ')"
} > "$M"
{ echo "# N-C5-1 source lines (OSAL 4.1.1 posix and pc-linux PSP of the build tree)"
  echo "## $OSAPI sha256=$(sha256sum "$OSAPI" | cut -c1-64)"
  echo "### L430-L455 (OS_API_Init: root -> main thread SCHED_FIFO 50)"; awk 'NR>=430 && NR<=455 {printf "L%d %s\n", NR, $0}' "$OSAPI"
  echo "### L584-L640 (OS_TaskCreate: root -> EXPLICIT_SCHED, SCHED_FIFO, OS_PriorityRemap)"; awk 'NR>=584 && NR<=640 {printf "L%d %s\n", NR, $0}' "$OSAPI"
  echo "### L3948-L4030 (OS_PriorityRemap)"; awk 'NR>=3948 && NR<=4030 {printf "L%d %s\n", NR, $0}' "$OSAPI"
  echo "## $PSPSTART sha256=$(sha256sum "$PSPSTART" | cut -c1-64)"
  echo "### L340-L382 (signal mask, start thread SCHED_FIFO max around CFE_ES_Main, then min)"; awk 'NR>=340 && NR<=382 {printf "L%d %s\n", NR, $0}' "$PSPSTART"
  echo "### L415-L421 (SIGINT handler)"; awk 'NR>=415 && NR<=421 {printf "L%d %s\n", NR, $0}' "$PSPSTART"
  echo "## $PCFG task priorities"; grep -n -E 'define CFE_.*_PRIORITY' "$PCFG"
  echo "## startup script $EXE/cf/apps/cfe_es_startup.scr"; grep -n '^CFE_' "$EXE/cf/apps/cfe_es_startup.scr"
} > "$OUT/n_c5_1_sources.txt"
{ echo "# conditions of the C5 natural boot 6.4.$V (derived instrumented build; group G-I)"
  echo "item${TB}run.group${TB}G-I natural: KZ_RT_MODE=record, no plan, no hold (PD29 D3, D4)${TB}from-decision"
  echo "item${TB}run.count${TB}this boot alone (P8, PD29 A7)${TB}from-decision"
  echo "item${TB}cfs.build${TB}$TREE (derive_c5.sh instr; D1 D3 D3' D5 in plan 8.4; DV-1, DV-H)${TB}deviation (PD29 D2: original-source modern-host derived configuration)"
  echo "item${TB}cfs.user${TB}root (readme 6.4.1 L176-L177, 6.4.2 L179-L180; PD29 N-C5-1)${TB}from-decision"
  echo "item${TB}cfs.scheduling${TB}what OSAL 4.1.1 posix and the PSP set for root (n_c5_1_sources.txt); the harness sets no policy, priority, nice or affinity (inherited, recorded in run.meta)${TB}from-reference (applied values recorded in state_running.txt)"
  echo "item${TB}cfs.start${TB}cd build/cpu1/exe; ./core-linux.bin, no arguments (readme 6.4.1 L162-L163, 6.4.2 L164-L165); first boot without arguments is PO (cfe_psp_start.c L555-L559)${TB}from-reference"
  echo "item${TB}cfs.terminal${TB}a pty from util-linux script -q -f -e -c ./core-linux.bin <typescript> (stdin /dev/null; the readme's terminal, as the build-smoke 3.1)${TB}unspecified_by_reference (the tool and its options)"
  echo "deviation${TB}ipc.namespace${TB}the host IPC namespace${TB}unshare --ipc for this boot alone${TB}PD29 D1${TB}separate SysV and POSIX IPC; fresh, empty namespace${TB}yes"
  echo "item${TB}mqueue.msg_max (namespace)${TB}64 (plan/c5_queue_inventory_20261029.md 7: TO_LAB_TLM_PIPE depth 64, to_lab_app.c L160)${TB}derived from the configuration (PD29 D1)"
  echo "item${TB}mqueue.msgsize_max, queues_max (namespace)${TB}namespace defaults, not written${TB}from-decision (D1, QI 7)"
  echo "item${TB}mqueue.msg_max (global)${TB}10, not written (read before and after)${TB}from-decision (D19, D1)"
  echo "item${TB}RLIMIT_MSGQUEUE${TB}unchanged (A3)${TB}from-decision"
  echo "item${TB}start_state${TB}Power-On: key files of exe recorded and removed before the boot if present; fresh IPC namespace without SysV segments (plan 3.4)${TB}from-reference"
  echo "item${TB}stop.method${TB}SIGINT to core-linux.bin (readme control-c; cfe_psp_start.c L270, L415-L421)${TB}from-reference"
  echo "item${TB}stop.trigger${TB}the end-of-startup console line \"$END_LINE\" ($END_REF), as run_cfs.sh operational mode stops on README L114${TB}from-reference (the line); the mapping is the harness's"
  echo "item${TB}stop.wait_limit${TB}$LIMIT_MS ms (build-smoke tool value)${TB}unspecified_by_reference"
  echo "item${TB}stop.settle${TB}$SETTLE_S s after the end-of-startup line (build-smoke tool value)${TB}unspecified_by_reference"
  echo "item${TB}stop.grace${TB}$GRACE_MS ms = CFE_ES_APP_KILL_TIMEOUT $KT x CFE_ES_APP_SCAN_RATE $SR (build cfe_platform_cfg.h; the run_cfs.sh rule)${TB}from-reference (by the run_cfs.sh rule)"
  echo "item${TB}stop.escalation${TB}SIGKILL after the grace (the PSP blocks all other signals, cfe_psp_start.c L346-L348)${TB}unspecified_by_reference"
  echo "deviation${TB}cfs.environment${TB}the build-smoke environment${TB}+= KZ_RT_MODE=record KZ_RT_DUMP KZ_RT_RUN_ID=1 KZ_RT_INSTANCE=1 (as run_gi_r2.sh) KZ_RT_STORE=$STORE${TB}the runtime is configured from the environment; the store file for any end (plan 9 item 2, ES 12.1, 12.4)${TB}none expected; not verified${TB}no"
  echo "deviation${TB}observer.method${TB}build-smoke: no observer${TB}compiler-inserted hooks (kzgcc 0.3) and the kzrt runtime (32-bit, 0.4.0) record into a store backed by a tmpfs file${TB}D3 instrumented path${TB}hook code in the instrumented units; not measured${TB}no"
  echo "item${TB}harness.reads${TB}before: files, key files, IPC; while running (after the settle): /proc/<pid>/task/*, mq_probe, a private mqueue mount, ipcs; after: the same${TB}unspecified_by_reference"
} > "$OUT/conditions.txt"
{ echo "## state before $(date -u +%FT%T.%3NZ)"
  echo "## exe directory"; ls -la --time-style=full-iso "$EXE" "$EXE/cf" "$EXE/cf/apps" "$EXE/ram"
  echo "## key files"; ls -la --time-style=full-iso "$EXE"/.cdskeyfile "$EXE"/.resetkeyfile "$EXE"/.reservedkeyfile 2>&1
  echo "## host SysV shm (not touched; the boot runs in a new IPC namespace)"; ipcs -m -p
  echo "## /dev/shm"; ls -la /dev/shm
  echo "## other cFS processes"; pgrep -a -x 'core-cpu1|core-linux.bin|core-tx|core-rx' || echo none; } > "$OUT/state_before.txt" 2>&1
for k in .cdskeyfile .resetkeyfile .reservedkeyfile; do
  [ -e "$EXE/$k" ] && { echo "removed leftover key file $EXE/$k ($(stat -c '%s B %y' "$EXE/$k"))" >> "$OUT/state_before.txt"; rm -f "$EXE/$k"; }
done
STAMP=$OUT/.stamp; : > "$STAMP"
unshare --ipc -- "$SELF" __inner "$V" "$OUT"; IR=$?
echo "inner_exit=$IR" >> "$M"
{ echo "outer mqueue after: msg_max=$(cat /proc/sys/fs/mqueue/msg_max) msgsize_max=$(cat /proc/sys/fs/mqueue/msgsize_max) queues_max=$(cat /proc/sys/fs/mqueue/queues_max)"
  echo "end_utc=$(date -u +%FT%T.%3NZ)"; } >> "$M"
{ echo "## state after $(date -u +%FT%T.%3NZ)"
  echo "## files created or modified under exe during the run"; ( cd "$EXE" && find . -newer "$STAMP" )
  echo "## key files"; ls -la --time-style=full-iso "$EXE"/.cdskeyfile "$EXE"/.resetkeyfile "$EXE"/.reservedkeyfile 2>&1
  echo "## host SysV shm"; ipcs -m -p
  echo "## /dev/shm"; ls -la /dev/shm
  echo "## cFS processes"; pgrep -a -x 'core-cpu1|core-linux.bin|core-tx|core-rx' || echo none; } > "$OUT/state_after.txt" 2>&1
rm -f "$STAMP" "$OUT/.grace_ms"
[ "$IR" = 0 ] || { echo "inner step failed ($IR) before or around the start; see $OUT/ns.txt"; rm -f "$STORE"; exit 7; }
# ---- records of the runtime
SX=$(sed -n 's/^script_exit=\([0-9]*\).*/\1/p' "$M")
DUMP=$OUT/dump/cpu1.kzrt; USE=""
if [ -s "$DUMP" ]; then
  echo "dump cpu1.kzrt bytes=$(stat -c %s "$DUMP") sha256=$(sha256sum "$DUMP" | cut -c1-64)" >> "$M"
  "$RTDUMP" --manifest "$SITES" --manifest "$BINDS" "$DUMP" > "$OUT/cpu1.dump.txt" 2>&1; DR=$?
  echo "kz-rt-dump exit=$DR" >> "$M"; [ $DR = 0 ] && USE=$DUMP
else echo "dump cpu1.kzrt missing" >> "$M"; fi
echo "store $STORE bytes=$(stat -c %s "$STORE" 2>/dev/null || echo absent)" >> "$M"
if [ -z "$USE" ] || { [ -n "$SX" ] && [ "$SX" -gt 128 ]; }; then
  if [ -n "$SX" ] && [ "$SX" -gt 128 ]; then END=(--signal $((SX-128))); else END=(--exit-status "${SX:-0}"); fi
  "$RTDUMP" --recover "$STORE" --out "$OUT/dump/cpu1.recovered.kzrt" "${END[@]}" --manifest "$SITES" --manifest "$BINDS" > "$OUT/cpu1.recovered.dump.txt" 2>&1
  RR=$?; echo "kz-rt-dump --recover ${END[*]} exit=$RR" >> "$M"
  [ $RR = 0 ] && USE=$OUT/dump/cpu1.recovered.kzrt
fi
rm -f "$STORE"; echo "store removed after decoding (ES 12.4)" >> "$M"
if [ -n "$USE" ]; then
  mkdir -p "$OUT/convert"
  "$KZ" convert --kzrt "$USE" --manifest "$SITES" --manifest "$BINDS" --basis "$BP/basis.kz" --out "$OUT/convert/run.kzt" > "$OUT/convert/convert.log" 2>&1
  echo "kz convert --kzrt $(basename "$USE") exit=$?" >> "$M"
  if [ -s "$OUT/convert/run.kzt" ]; then
    "$KZDUMP" "$OUT/convert/run.kzt" > "$OUT/convert/run.kzt.txt" 2>&1; echo "kz-dump run.kzt exit=$?" >> "$M"
  fi
fi
"$SELF" check "$V" "$OUT" > /dev/null 2>&1
grep -E 'stop_reason|end_of_startup|stop_actions|script_exit|exit=|missing' "$M"

#!/bin/bash
# usage: run_probe.sh <name> <caps:yes|no> <cpus:all|0> <seconds>
set -u
W=/tmp/claude-0/-home-user-Shape-Perf/e983f27d-f015-5a5d-b912-399245630e78/scratchpad/cfs_note/sem-es-tbl
NAME=$1; CAPS=$2; CPUS=$3; SECS=$4
R=$W/runs/$NAME
rm -rf $R; mkdir -p $R
cp -r $W/bundle/build/exe/cpu1/core-cpu1 $W/bundle/build/exe/cpu1/cf $R/
cat > $R/cf/cfe_es_startup.scr <<'SCR'
CFE_APP, probe_lo,   Probe_LO_Main,   PROBE_LO,   200, 16384, 0x0, 0;
CFE_APP, probe_hi,   Probe_HI_Main,   PROBE_HI,    20, 16384, 0x0, 0;
CFE_APP, probe_slow, Probe_SLOW_Main, PROBE_SLOW, 100, 16384, 0x0, 0;
CFE_APP, probe_own,  Probe_OWN_Main,  PROBE_OWN,   90, 16384, 0x0, 0;
CFE_APP, probe_shr,  Probe_SHR_Main,  PROBE_SHR,   95, 16384, 0x0, 0;
!
SCR
chmod -R a+rwX $R
cd $R
if [ "$CAPS" = yes ]; then
  PRIV="setpriv --reuid=65534 --regid=65534 --clear-groups --inh-caps=-all,+sys_nice --ambient-caps=+sys_nice --"
else
  PRIV="setpriv --reuid=65534 --regid=65534 --clear-groups --inh-caps=-all --"
fi
if [ "$CPUS" = 0 ]; then TS="taskset -c 0"; else TS=""; fi
echo "cmd: timeout -s INT $SECS $TS $PRIV ./core-cpu1" > $R/cmd.txt
( timeout -s INT $SECS $TS $PRIV ./core-cpu1 > $R/console.log 2>&1 ) &
sleep 4
P=$(pgrep -u 65534 -f 'core-cpu1' | head -1)
echo "pid=$P" > $R/threads.txt
ps -L -o tid,cls,rtprio,pri,psr,comm -p $P >> $R/threads.txt 2>&1
wait
echo "exit done" >> $R/threads.txt

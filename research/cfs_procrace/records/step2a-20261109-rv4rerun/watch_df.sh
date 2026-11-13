#!/usr/bin/env bash
# free-space log every 2 s while the driver runs; stops the driver below 330,000,000 B
L=${SCRATCH:?scratchpad directory}/rerun_rv4
while [ ! -e "$L/all.rc" ]; do
  a=$(df -B1 --output=avail / | tail -1 | tr -d ' ')
  echo "$(date -u +%T) $a" >> "$L/df.log"
  if [ "$a" -lt 330000000 ]; then echo "$(date -u +%T) LOW $a: stopping the driver" >> "$L/df.log"; kill -TERM -- -"$(cat "$L/pgid")"; break; fi
  sleep 2
done

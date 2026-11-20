#!/usr/bin/env bash
# step.sh BUILD N STEP -- one line of records/closure-c1prep-20261119/boot/commands.txt, run as written there from the
# repository root (results 27.1; records/closure-boots-20261119/README.md):
#   bash research/cfs_procrace/records/closure-c1prep-20261119/boot/run_cycle.sh STEP BUILD N
# (STEP summary: give N as "-"; run_cycle.sh is then called without N, as commands.txt writes it).
# Its stdout and stderr go to BUILD/boot-N/STEP.out beside this directory (summary: BUILD/summary/summary.out); an
# existing one is first kept under a new name (UTC nanoseconds), never overwritten.  One line is appended to
# BUILD/steps.tsv: start and end UTC, build, N, step, rc, free before and after (df -B1 /), the output file.
# It adds nothing to the step: no argument is changed, and it refuses to run when a test seam of run_cycle.sh
# (RUN_CYCLE_OBS, RUN_CYCLE_BOOT_CMD, RUN_CYCLE_FREE_CMD) is set.  rc: the step's own rc; 64 on usage.
set -uo pipefail
B=${1:-}; N=${2:-}; STEP=${3:-}
case "$B" in c1_pre_tx|c1_post_tx) ;; *) echo "usage: step.sh c1_pre_tx|c1_post_tx N STEP" >&2; exit 64 ;; esac
[ -n "$N" ] && [ -n "$STEP" ] || { echo "usage: step.sh c1_pre_tx|c1_post_tx N STEP" >&2; exit 64; }
for v in RUN_CYCLE_OBS RUN_CYCLE_BOOT_CMD RUN_CYCLE_FREE_CMD; do
  [ -z "${!v:-}" ] || { echo "step.sh: $v is set (a test seam of run_cycle.sh): not running" >&2; exit 64; }
done
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REC="$(cd "$HERE/.." && pwd)"
REPO="$(cd "$REC/../../../.." && pwd)"
RCS=research/cfs_procrace/records/closure-c1prep-20261119/boot/run_cycle.sh
[ -f "$REPO/$RCS" ] || { echo "step.sh: no $REPO/$RCS" >&2; exit 64; }
if [ "$STEP" = summary ]; then D=$REC/$B/summary; ARGS=(summary "$B"); else D=$REC/$B/boot-$N; ARGS=("$STEP" "$B" "$N"); fi
mkdir -p "$D" || exit 64
OUT=$D/$STEP.out
if [ -e "$OUT" ]; then k=$D/$STEP.$(date -u +%Y%m%dT%H%M%S.%NZ).out; mv "$OUT" "$k" || exit 64; fi
free_now() { df -B1 --output=avail / | tail -1 | tr -d ' '; }
t0=$(date -u +%FT%T.%3NZ); f0=$(free_now)
( cd "$REPO" && bash "$RCS" "${ARGS[@]}" ) > "$OUT" 2>&1; rc=$?
t1=$(date -u +%FT%T.%3NZ); f1=$(free_now)
T=$REC/$B/steps.tsv
[ -f "$T" ] || printf 'start\tend\tbuild\tN\tstep\trc\tfree_before\tfree_after\toutput\n' > "$T"
printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$t0" "$t1" "$B" "$N" "$STEP" "$rc" "$f0" "$f1" "${OUT#$REC/}" >> "$T"
cat "$OUT"
echo "step.sh: $STEP $B $N rc=$rc free $f0 -> $f1"
exit $rc

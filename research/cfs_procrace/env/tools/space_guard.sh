#!/usr/bin/env bash
# space_guard.sh -- run one command under a free-space guard on the root file system.
#
#   space_guard.sh --need BYTES --log FILE [--floor BYTES] [--interval SECONDS] [--kind NAME] -- CMD [ARG...]
#
# Before the start: free (df -B1 /, bytes available) must be >= floor + need, else the command is not started (rc 75).
# While the command runs: free is sampled every --interval seconds (default 2) into FILE, one line per sample
# ("sample <UTC> <free>"); when a sample is below the floor the command's process group gets SIGTERM, then SIGKILL
# after 5 s, and the guard exits 76.  Otherwise the guard exits with the command's status.
# The last lines of FILE: "start", "end" with the lowest sample, the dip (free at start minus the lowest sample),
# the change (end minus start), the kind (--kind, default "unnamed"; the dip of each tool kind is kept apart) and
# the verdict (completed, refused-at-start, stopped-at-floor).
# Floor default 314572800 B (300 MiB, as records/local-20261109/bundle_local.sh:20): a work safety value.
# The free value comes from `df -B1 /`; SPACE_GUARD_FREE_CMD replaces that command (unit tests only; the log says so).
set -uo pipefail
NEED=; LOG=; FLOOR=314572800; IVL=2; KIND=unnamed
while [ $# -gt 0 ]; do
  case "$1" in
    --need) NEED=${2:-}; shift 2 ;;
    --log) LOG=${2:-}; shift 2 ;;
    --floor) FLOOR=${2:-}; shift 2 ;;
    --interval) IVL=${2:-}; shift 2 ;;
    --kind) KIND=${2:-}; shift 2 ;;
    --) shift; break ;;
    *) echo "space_guard.sh: unknown argument $1" >&2; exit 64 ;;
  esac
done
[ $# -gt 0 ] || { echo "usage: space_guard.sh --need BYTES --log FILE [--floor B] [--interval S] [--kind NAME] -- CMD..." >&2; exit 64; }
case "$NEED$FLOOR" in *[!0-9]*|'') echo "space_guard.sh: --need and --floor are byte counts" >&2; exit 64 ;; esac
[ -n "$LOG" ] || { echo "space_guard.sh: --log is required" >&2; exit 64; }
case "$KIND" in *[!A-Za-z0-9._-]*|'') echo "space_guard.sh: --kind is a short name" >&2; exit 64 ;; esac
free_now() {
  if [ -n "${SPACE_GUARD_FREE_CMD:-}" ]; then bash -c "$SPACE_GUARD_FREE_CMD"
  else df -B1 / | awk 'NR==2 {print $4}'; fi
}
now() { date -u +%FT%T.%3NZ; }
F0=$(free_now) || { echo "space_guard.sh: cannot read free space" >&2; exit 70; }
case "$F0" in *[!0-9]*|'') echo "space_guard.sh: free space is not a number: $F0" >&2; exit 70 ;; esac
{
  echo "# space_guard.sh kind=$KIND need=$NEED floor=$FLOOR interval=$IVL cmd=$*"
  [ -z "${SPACE_GUARD_FREE_CMD:-}" ] || echo "# free values from SPACE_GUARD_FREE_CMD (unit test), not from df"
  echo "start $(now) free=$F0 threshold=$((FLOOR + NEED))"
} >> "$LOG" || { echo "space_guard.sh: cannot write $LOG" >&2; exit 70; }
if [ "$F0" -lt $((FLOOR + NEED)) ]; then
  echo "end $(now) kind=$KIND verdict=refused-at-start free=$F0 needed=$((FLOOR + NEED))" >> "$LOG"
  echo "space_guard.sh: refused: free $F0 < floor $FLOOR + need $NEED" >&2
  exit 75
fi
setsid "$@" &
PID=$!
LOW=$F0; STOP=0
while kill -0 "$PID" 2> /dev/null; do
  F=$(free_now) || F=
  case "$F" in
    *[!0-9]*|'') echo "sample $(now) unreadable" >> "$LOG" ;;
    *) echo "sample $(now) $F" >> "$LOG"
       [ "$F" -lt "$LOW" ] && LOW=$F
       if [ "$F" -lt "$FLOOR" ]; then
         STOP=1
         kill -TERM -- "-$PID" 2> /dev/null || kill -TERM "$PID" 2> /dev/null
         for _ in 1 2 3 4 5; do kill -0 "$PID" 2> /dev/null || break; sleep 1; done
         kill -KILL -- "-$PID" 2> /dev/null || true
         break
       fi ;;
  esac
  sleep "$IVL"
done
wait "$PID"; RC=$?
F1=$(free_now) || F1=0
case "$F1" in *[!0-9]*|'') F1=0 ;; esac
[ "$F1" -lt "$LOW" ] && LOW=$F1
if [ $STOP -eq 1 ]; then
  echo "end $(now) kind=$KIND verdict=stopped-at-floor rc=$RC free=$F1 lowest=$LOW dip=$((F0 - LOW)) change=$((F1 - F0))" >> "$LOG"
  echo "space_guard.sh: stopped: free fell below the floor $FLOOR" >&2
  exit 76
fi
echo "end $(now) kind=$KIND verdict=completed rc=$RC free=$F1 lowest=$LOW dip=$((F0 - LOW)) change=$((F1 - F0))" >> "$LOG"
exit "$RC"

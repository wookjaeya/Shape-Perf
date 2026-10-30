#!/usr/bin/env bash
# control_cmp.sh gih <run dir>  |  control_cmp.sh f0 <confirm2 run dir>
# The realized control of one controlled run as canonical lines (the IR 13.3 unit "what the backend actually held and
# resumed"), so that a G-I-H run (kz-rt-dump of its dump, the plan's point names) and its F0 confirm2 counterpart
# (hold.kzr of kz hold) compare line by line:
#   hold <gate> hit=<n>                    a gate held a thread at its n-th qualifying hit
#   fire <release> hit=<n> <how>[,deferred] how = resumed | satisfied | consumed
#   refused <gate> hit=<n> <reason>         (G-I-H only: a gate refused in its context)
#   unreleased <gate>                      a gate still held a thread at the end
#   timeout <gate>                         (G-I-H only)
# Thread ids, times and ctl_seq are left out (the two backends' own keys and clocks).  Pilot/dev tooling.
set -uo pipefail
case "${1:-}" in
  gih)
    R=${2:?run}; D=$(ls "$R"/*.dump.txt | head -1)
    awk '
      /^plan point / { split($0, a, " "); i = a[3]; for (k = 1; k <= NF; k++) if ($k ~ /^name=/) { n = $k; sub(/^name=/, "", n); N[i] = n } }
      /^ctl / { w = ""; p = ""; h = ""; r = ""
        for (k = 1; k <= NF; k++) { if ($k ~ /^what=/) w = substr($k, 6); if ($k ~ /^point=/) p = substr($k, 7); if ($k ~ /^hit=/) h = substr($k, 5); if ($k ~ /^reason=/) r = substr($k, 8) }
        L[++nl] = w SUBSEP p SUBSEP h SUBSEP r }
      /^point [0-9]+ gate / { for (k = 1; k <= NF; k++) if ($k ~ /^held=/ && $k != "held=-") U[$2] = 1 }
      END {
        for (j = 1; j <= nl; j++) { split(L[j], f, SUBSEP)
          if (f[1] == "gate-hold") print "hold " N[f[2]] " hit=" f[3]
          else if (f[1] == "fire") print "fire " N[f[2]] " hit=" f[3] " " f[4]
          else if (f[1] == "gate-refused") print "refused " N[f[2]] " hit=" f[3] " " f[4]
          else if (f[1] == "timeout") print "timeout " N[f[2]] }
        for (i in U) print "unreleased " N[i] }' "$D" | sort ;;
  f0)
    R=${2:?run}
    awk '
      $1 == "hold" { g = ""; h = "1"; rel = "no"; for (k = 2; k <= NF; k++) { if ($k ~ /^gate=/) g = substr($k, 6); if ($k ~ /^hit=/) h = substr($k, 5); if ($k ~ /^released_by=/) rel = substr($k, 13) }
        print "hold " g " hit=" h; if (rel == "no") print "unreleased " g }
      $1 == "fire" { r = ""; h = "1"; how = "consumed"; d = ""
        for (k = 2; k <= NF; k++) { if ($k ~ /^release=/) r = substr($k, 9); if ($k ~ /^hit=/) h = substr($k, 5); if ($k ~ /^resumed=/ && $k != "resumed=none") how = "resumed"
          if ($k == "satisfied=yes") how = "satisfied"; if ($k == "deferred=yes") d = ",deferred" }
        print "fire " r " hit=" h " " how d }' "$R/hold.kzr" | sort ;;
  *) echo "usage: control_cmp.sh gih|f0 <run dir>" >&2; exit 64 ;;
esac

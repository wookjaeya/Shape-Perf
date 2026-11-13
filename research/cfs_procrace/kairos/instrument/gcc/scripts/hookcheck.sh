#!/usr/bin/env bash
# hookcheck.sh <merged binds.kzs> <bincheck output> -- the link-time check of a derived build: for every binding the plugin
# reported, the observation hook calls (kz_rt_obs, kz_rt_obs_str) with its id that the linked files contain (bincheck.sh
# output: objdump, the immediate in %esi before the call), against the plugin's own count after "optimized" (the
# `-> function:n` of the bind's at=); the control hook calls (kz_rt_ctl, same id) are counted apart (ctl=).  Give
# bincheck.sh the files of one instance (e.g. core-tx cf.so bp.so), where each unit is linked once.
#   ok           bound|inlined|duplicated and the link-time count equals the plugin's count
#   copies       bound|inlined|duplicated, at least one call, but another count (RTL duplication after the plugin's check)
#   missing      bound|inlined|duplicated but no call with the id
#   gap          removed|unsupported (no call expected) and none found
#   gap-called   removed|unsupported but a call with the id is found
#   undeclared   a call with an id the binding list does not declare ("?": no immediate found before the call)
# Prints one line per binding, then a summary; exits 1 on missing, gap-called or undeclared.
set -uo pipefail
B=${1:?usage}; C=${2:?usage}
awk '
  FNR == NR {
    if ($1 != "bind") next
    id = $2; st = $0; sub(/.* status=/, "", st); sub(/ .*/, "", st); status[id] = st
    n = 0; at = $0
    if (match(at, /-> [^"]*"/)) { s = substr(at, RSTART + 3, RLENGTH - 4); k = split(s, parts, ","); for (i = 1; i <= k; i++) { sub(/.*:/, "", parts[i]); n += parts[i] } }
    plugin[id] = n; order[++nb] = id; next
  }
  NF >= 5 && $3 == "kz_rt_ctl" { ctl[$4]++; next }
  NF >= 5 { calls[$4]++; where[$4] = where[$4] " " $1 ":" $2 }
  END {
    bad = 0
    for (i = 1; i <= nb; i++) {
      id = order[i]; st = status[id]; c = calls[id] + 0
      if (st == "bound" || st == "inlined" || st == "duplicated") {
        if (c == 0) { r = "missing"; bad = 1 } else if (c != plugin[id]) r = "copies"; else r = "ok"
      } else { if (c > 0) { r = "gap-called"; bad = 1 } else r = "gap" }
      printf "%s %d %s plugin=%d link=%d%s%s\n", r, id, st, plugin[id], c, (id in ctl ? " ctl=" ctl[id] : ""), where[id]; cnt[r]++
      seen[id] = 1
    }
    for (id in ctl) if (!(id in seen)) { printf "undeclared %s ctl=%d\n", id, ctl[id]; cnt["undeclared"]++; bad = 1 }
    for (id in calls) if (!(id in seen)) { printf "undeclared %s link=%d%s\n", id, calls[id], where[id]; cnt["undeclared"]++; bad = 1 }
    printf "summary:"; for (r in cnt) printf " %s=%d", r, cnt[r]; printf "\n"
    exit bad
  }' "$B" "$C"

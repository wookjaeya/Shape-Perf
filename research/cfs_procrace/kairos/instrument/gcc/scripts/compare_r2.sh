#!/usr/bin/env bash
# compare_r2.sh <F0 kz-dump text> <GCC events.tsv> <out dir> -- the IR section 13.2 comparison of one GCC-plugin pilot
# boot of R2 with the F0 record v153_pilot3, on normalized lines:
#   counts.txt     per logical site: F0 events and GCC events
#   identity.*     per OSAL task id: owner application id and creating task (F0 sites 21, 25, 7; GCC 1, 8, 7)
#   sbsend.*       per (task, SB call site, AppID passed): number of sends (F0 27-29 slot app_id; GCC 40-42 slot 2)
#   evssend.*      per (task, AppID == 0 or not): sends at the entry of CFE_EVS_SendEventWithAppID (F0 0; GCC 45)
#   sbpub.*        the SB task's publication of its id (F0 26 status; GCC 43 status + id, 44 the stored value)
# Counts are compared, not required to be equal (IR section 13.2: "ES 20 + EVS 4" is not an acceptance constant).
set -uo pipefail
F0=${1:?usage}; EV=${2:?usage}; O=${3:?usage}
mkdir -p "$O"
awk -v O="$O" '
  function hex2(s,   i, c, v) { v = 0; s = tolower(s); sub(/^0x/, "", s); if (length(s) > 8) s = substr(s, length(s) - 7); for (i = 1; i <= length(s); i++) { c = index("0123456789abcdef", substr(s, i, 1)) - 1; v = v * 16 + c }; return v }
  function s32(h,   v) { v = hex2(h); if (v >= 2147483648) v -= 4294967296; return v }
  NR == FNR && /THREADS\[/ { tid = $3; sub(/tid=/, "", tid); n = $0; sub(/.* name="/, "", n); sub(/".*/, "", n); tname[tid] = n; next }
  NR == FNR { next }
  /RAW_EVENTS\[/ {
    tid = $3; sub(/tid=/, "", tid); site = $5; sub(/site=/, "", site)
    a = $0; sub(/.*a=\[/, "", a); sub(/\].*/, "", a); split(a, v, ",")
    cnt[site]++; ev[++ne] = tid SUBSEP site SUBSEP a
    if (site == 21) owner[tid] = sprintf("0x%x", hex2(v[3]))
    if (site == 22) owner[tid] = ""
    if (site == 25) { tk = sprintf("0x%x", hex2(v[1])); cr_thread[tk] = tid; cr_owner[tk] = (owner[tid] == "" ? "-" : owner[tid]) }
    if (site == 7) { tk = sprintf("0x%x", hex2(v[1])); entry_thread[tk] = tid; task_of[tid] = tk }
  }
  END {
    for (s in cnt) print s, cnt[s] > (O "/f0_counts.raw")
    for (tk in cr_thread) { c = cr_thread[tk]; ct = (c in task_of) ? task_of[c] : "main"; printf "task=%s owner=%s creator=%s entered=%s f0_name=%s\n", tk, cr_owner[tk], ct, (tk in entry_thread) ? "yes" : "no", (tk in entry_thread) ? tname[entry_thread[tk]] : "-" }
    for (k = 1; k <= ne; k++) {
      split(ev[k], e, SUBSEP); tid = e[1]; site = e[2]; split(e[3], v, ",")
      tk = (tid in task_of) ? task_of[tid] : "tid-" tname[tid]
      if (site == 27 || site == 28 || site == 29) sb[tk " " (site == 27 ? "sb_api_275" : site == 28 ? "sb_api_1183" : "sb_priv_786") " app_id=" sprintf("0x%x", hex2(v[3]))]++
      if (site == 0) evs[tk " app_id_zero=" (hex2(v[3]) == 0 ? "yes" : "no")]++
      if (site == 26) printf "sb_publication task=%s status=%d\n", tk, s32(v[1]) > (O "/sbpub.f0")
    }
    for (k in sb) printf "task=%s n=%d name=%s\n", k, sb[k], "" > (O "/sbsend.f0.raw")
    for (k in evs) printf "task=%s n=%d\n", k, evs[k] > (O "/evssend.f0.raw")
  }' "$F0" "$F0" | sort > "$O/identity.f0"
# F0 thread names per task id, to label the lines
awk '{t = $1; sub(/task=/, "", t); n = $NF; sub(/f0_name=/, "", n); print t, n}' "$O/identity.f0" > "$O/names.txt"
awk -F'\t' -v O="$O" '
  function hex2(s,   i, c, v) { v = 0; s = tolower(s); sub(/^0x/, "", s); if (length(s) > 8) s = substr(s, length(s) - 7); for (i = 1; i <= length(s); i++) { c = index("0123456789abcdef", substr(s, i, 1)) - 1; v = v * 16 + c }; return v }
  function s32(h,   v) { v = hex2(h); if (v >= 2147483648) v -= 4294967296; return v }
  $1 == "ev" {
    a = $2; site = $4; cnt[site]++; ev[++ne] = $0
    if (site == 1) owner[a] = sprintf("0x%x", hex2($11))
    if (site == 2) owner[a] = ""
    if (site == 8) { tk = sprintf("0x%x", hex2($9)); cr_actor[tk] = a; cr_owner[tk] = (owner[a] == "" ? "-" : owner[a]) }
    if (site == 7) { tk = sprintf("0x%x", hex2($9)); entry_actor[tk] = a; task_of[a] = tk }
  }
  END {
    for (s in cnt) print s, cnt[s] > (O "/gcc_counts.raw")
    for (tk in cr_actor) { c = cr_actor[tk]; ct = (c in task_of) ? task_of[c] : "main"; printf "task=%s owner=%s creator=%s entered=%s\n", tk, cr_owner[tk], ct, (tk in entry_actor) ? "yes" : "no" }
    for (k = 1; k <= ne; k++) {
      split(ev[k], f, "\t"); a = f[2]; site = f[4]; tk = (a in task_of) ? task_of[a] : "actor-" a
      if (site == 40 || site == 41 || site == 42) sb[tk " " (site == 40 ? "sb_api_275" : site == 41 ? "sb_api_1183" : "sb_priv_786") " app_id=" sprintf("0x%x", hex2(f[11]))]++
      if (site == 45) evs[tk " app_id_zero=" (hex2(f[11]) == 0 ? "yes" : "no")]++
      if (site == 43) { printf "sb_publication task=%s status=%d app_id(out0)=0x%x\n", tk, s32(f[9]), hex2(f[10]) > (O "/sbpub.gcc"); pub[a] = f[3] }
      if (site == 44) { store[a, f[3]] = hex2(f[9]); nstore[tk]++ }
    }
    # the store inside the publishing call: the same actor, the record just before the after-call record
    for (a in pub) if ((a, pub[a] - 1) in store) printf "CFE_ES_GetAppID.store (inside that call) task=%s app_id=0x%x\n", task_of[a], store[a, pub[a] - 1] > (O "/sbpub.gcc")
    for (t in nstore) printf "CFE_ES_GetAppID.store task=%s n=%d (every caller)\n", t, nstore[t] > (O "/getappid_stores.gcc")
    for (k in sb) printf "task=%s n=%d\n", k, sb[k] > (O "/sbsend.gcc.raw")
    for (k in evs) printf "task=%s n=%d\n", k, evs[k] > (O "/evssend.gcc.raw")
  }' "$EV" | sort > "$O/identity.gcc"
label() { awk 'NR == FNR { n[$1] = $2; next } { t = $1; sub(/task=/, "", t); print $0, "(" ((t in n) ? n[t] : "?") ")" }' "$O/names.txt" - ; }
for k in sbsend evssend; do
  sed 's/ name=$//' "$O/$k.f0.raw" | sort | label > "$O/$k.f0"; sort "$O/$k.gcc.raw" | label > "$O/$k.gcc"
  diff "$O/$k.f0" "$O/$k.gcc" > "$O/$k.diff"; echo "$k diff lines: $(grep -c '^[<>]' "$O/$k.diff")"
done
diff <(sed 's/ f0_name=.*//' "$O/identity.f0") "$O/identity.gcc" > "$O/identity.diff"; echo "identity diff lines: $(grep -c '^[<>]' "$O/identity.diff")"
for s in f0 gcc; do echo "AppID 0 at the SB call sites ($s): $(awk '/app_id=0x0 /{split($0, a, "n="); split(a[2], b, " "); n += b[1]; t[$NF] += b[1]} END {printf "%d", n; for (k in t) printf " %s=%d", k, t[k]}' "$O/sbsend.$s")"; done
{
  echo "# logical site | GCC site | GCC events | F0 site (cfs-v7.0.1/observe/sites.kzp) | F0 events"
  while read -r g f label; do
    gc=$(awk -v s="$g" '$1==s{print $2}' "$O/gcc_counts.raw"); fc=$(awk -v s="$f" '$1==s{print $2}' "$O/f0_counts.raw")
    echo "$label | $g | ${gc:-0} | $f | ${fc:-0}"
  done <<'EOF'
1 21 es.start_task.entry
2 22 es.start_task.ret
3 23 es.child_task.entry
4 24 es.child_task.ret
5 5 osal.task_create.entry
6 6 osal.task_create.ret
7 7 osal.task_entry
8 25 osal.task_create.id
40 27 CFE_EVS_SendEventWithAppID.id_valid@sb_api_275
41 28 CFE_EVS_SendEventWithAppID.id_valid@sb_api_1183
42 29 CFE_EVS_SendEventWithAppID.id_valid@sb_priv_786
43 26 sb_publication
44 - CFE_ES_GetAppID.store(no F0 site)
45 0 evs.send.entry
46 1 evs.send.ret
EOF
} > "$O/counts.txt"

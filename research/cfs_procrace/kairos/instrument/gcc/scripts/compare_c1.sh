#!/usr/bin/env bash
# compare_c1.sh <F0 kz-dump text> <GCC events.tsv> <out dir> -- the IR section 13.2 comparison of one GCC-plugin
# pilot boot of C1 with the F0 record of the same configuration (smoke3 post_1df262a), on normalized lines:
#   counts.txt     per logical site: F0 events (callee-side sites of the F0 plan) and GCC events
#   identity.*     per OSAL task id: the owner application id (the ParentAppId of the enclosing start-task
#                  request on the creating thread) and the creating task (F0 sites 26, 33, 32; GCC 1, 8, 7)
#   sem.*          the semaphore lookups and creations of CF and BP in program order per task: name, status,
#                  and the id (only on success: F0 reads it inside OSAL only then)
# Counts are compared, not required to be equal (IR section 13.2).  Shell and awk only.
set -uo pipefail
F0=${1:?usage}; EV=${2:?usage}; O=${3:?usage}
mkdir -p "$O"
# ---- F0: RAW_EVENTS + EVENT_STRINGS of kz-dump ---------------------------------------------------------
awk '
  function hex2(s,   i, c, v) { v = 0; s = tolower(s); sub(/^0x/, "", s); for (i = 1; i <= length(s); i++) { c = index("0123456789abcdef", substr(s, i, 1)) - 1; v = v * 16 + c }; return v }
  function s32(h,   v) { v = hex2(h); if (v >= 2147483648) v -= 4294967296; return v }
  # two passes over the dump: strings and threads come after the events
  NR == FNR && /EVENT_STRINGS\[/ { e = $2; sub(/event=/, "", e); t = $0; sub(/.*text="/, "", t); sub(/"$/, "", t); str[e] = t; next }
  NR == FNR && /THREADS\[/ { tid = $3; sub(/tid=/, "", tid); n = $0; sub(/.* name="/, "", n); sub(/".*/, "", n); tname[tid] = n; next }
  NR == FNR { next }
  /RAW_EVENTS\[/ {
    i = $1; sub(/RAW_EVENTS\[/, "", i); sub(/\]/, "", i)
    tid = $3; sub(/tid=/, "", tid); site = $5; sub(/site=/, "", site)
    a = $0; sub(/.*a=\[/, "", a); sub(/\].*/, "", a); split(a, v, ",")
    cnt[site]++
    if (site == 26) owner[tid] = sprintf("0x%x", hex2(v[3]))
    if (site == 27) owner[tid] = ""
    if (site == 33) { tk = sprintf("0x%x", hex2(v[1])); cr_thread[tk] = tid; cr_owner[tk] = (owner[tid] == "" ? "-" : owner[tid]) }
    if (site == 32) { tk = sprintf("0x%x", hex2(v[1])); entry_thread[tk] = tid; task_of[tid] = tk }
    # semaphore calls (callee-side entry/ret, the out id inside OSAL)
    if (site == 0) { call[tid] = "lookup"; nm[tid] = str[i]; oid[tid] = "-" }
    if (site == 2) { call[tid] = "create"; nm[tid] = str[i]; oid[tid] = "-" }
    if (site == 5) oid[tid] = sprintf("0x%x", hex2(v[1]))   # osal.countsem_lookup.found s0 = sem_id
    if (site == 7) oid[tid] = sprintf("0x%x", hex2(v[2]))   # osal.countsem_create.created s1 = sem_id
    if ((site == 1 || site == 3) && call[tid] != "") { st = s32(v[1]); seq[tid] = seq[tid] sprintf("%s name=%s status=%d id=%s\n", call[tid], nm[tid], st, st == 0 ? oid[tid] : "-"); call[tid] = "" }
  }
  END {
    for (s in cnt) print s, cnt[s] > "'"$O"'/f0_counts.raw"
    for (tk in cr_thread) { c = cr_thread[tk]; ct = (c in task_of) ? task_of[c] : "main"; printf "task=%s owner=%s creator=%s entered=%s f0_name=%s\n", tk, cr_owner[tk], ct, (tk in entry_thread) ? "yes" : "no", (tk in entry_thread) ? tname[entry_thread[tk]] : "-" }
    for (t in seq) { n = split(seq[t], L, "\n"); for (k = 1; k < n; k++) printf "task=%s %s#%d %s\n", task_of[t], tname[t], k, L[k] > "'"$O"'/f0_sem.raw" }
  }' "$F0" "$F0" | sort > "$O/identity.f0"
# ---- GCC: kzgcc-events tsv ---------------------------------------------------------------------------
awk -F'\t' '
  function hex2(s,   i, c, v) { v = 0; s = tolower(s); sub(/^0x/, "", s); for (i = 1; i <= length(s); i++) { c = index("0123456789abcdef", substr(s, i, 1)) - 1; v = v * 16 + c }; return v }
  function s32(h,   v) { sub(/^0x/, "", h); if (length(h) > 8) h = substr(h, length(h) - 7); v = hex2(h); if (v >= 2147483648) v -= 4294967296; return v }
  $1 == "ev" {
    a = $2; site = $4; cnt[site]++
    if (site == 1) owner[a] = sprintf("0x%x", hex2($11))
    if (site == 2) owner[a] = ""
    if (site == 8) { tk = sprintf("0x%x", hex2($9)); cr_actor[tk] = a; cr_owner[tk] = (owner[a] == "" ? "-" : owner[a]) }
    if (site == 7) { tk = sprintf("0x%x", hex2($9)); entry_actor[tk] = a; task_of[a] = tk }
    if (site == 20 || site == 30) { call[a] = "lookup"; nm[a] = $13 }
    if (site == 32) { call[a] = "create"; nm[a] = $13 }
    if ((site == 21 || site == 31 || site == 33) && call[a] != "") { st = s32($9); seq[a] = seq[a] sprintf("%s name=%s status=%d id=%s\n", call[a], nm[a], st, st == 0 ? sprintf("0x%x", hex2($10)) : "-"); call[a] = "" }
  }
  END {
    for (s in cnt) print s, cnt[s] > "'"$O"'/gcc_counts.raw"
    for (tk in cr_actor) { c = cr_actor[tk]; ct = (c in task_of) ? task_of[c] : "main"; printf "task=%s owner=%s creator=%s entered=%s\n", tk, cr_owner[tk], ct, (tk in entry_actor) ? "yes" : "no" }
    for (a in seq) { n = split(seq[a], L, "\n"); for (k = 1; k < n; k++) printf "task=%s #%d %s\n", task_of[a], k, L[k] > "'"$O"'/gcc_sem.raw" }
  }' "$EV" | sort > "$O/identity.gcc"
sort "$O/f0_sem.raw" > "$O/sem.f0"; sort "$O/gcc_sem.raw" > "$O/sem.gcc"
# ---- per-site counts: logical site (GCC id) <- F0 plan site of the same meaning ---------------------------
{
  echo "# logical site | GCC site | GCC events | F0 site (sites-cf833fdbb.kzp) | F0 events"
  while read -r g f label; do
    gc=$(awk -v s="$g" '$1==s{print $2}' "$O/gcc_counts.raw"); fc=$(awk -v s="$f" '$1==s{print $2}' "$O/f0_counts.raw")
    echo "$label | $g | ${gc:-0} | $f | ${fc:-0}"
  done <<'EOF'
1 26 es.start_task.entry
2 27 es.start_task.ret
3 28 es.child_task.entry
4 29 es.child_task.ret
5 30 osal.task_create.entry
6 31 osal.task_create.ret
7 32 osal.task_entry
8 33 osal.task_create.id
9 8 es.exit_app.entry
20 0 cf_lookup_call.before(F0:osal.countsem_lookup.entry,all-callers)
21 13 cf_lookup_call
22 14 cf_retry_delay
30 0 bp_existence_probe_call.before(F0:osal.countsem_lookup.entry,all-callers)
31 17 bp_existence_probe_call
32 2 bp_create_call.before(F0:osal.countsem_create.entry,all-callers)
33 18 bp_create_call
EOF
} > "$O/counts.txt"
# identity: compare without the F0 thread name column
diff <(sed 's/ f0_name=.*//' "$O/identity.f0") "$O/identity.gcc" > "$O/identity.diff"; echo "identity diff lines: $(grep -c '^[<>]' "$O/identity.diff")"
diff <(grep -E ' (CF|BP)#' "$O/sem.f0" | sed 's/ [A-Z_]*#/ #/') "$O/sem.gcc" > "$O/sem.diff"; echo "sem diff lines (CF, BP): $(grep -c '^[<>]' "$O/sem.diff")"

# classify_gih.awk -- the realized class of one C1 controlled run from its real-record trace (`kz replay --emit-trace`), in the
# notation of confirm2/scripts/classify.awk and kz predict: per key, CF's lookup results in order (run-length, "-17x25",
# "0x1" = result 0 once), and CF's end (x = CFE_ES_ExitApp, f = CF_Init returned 0).  The P11 reading is not computed here:
# for a G-I-H run kz replay itself reads the run's realized control (control-order); this script only names the class, so
# that it compares with the F0 confirm2 class line (classify.awk, its [...] reading-A notes left out).
# CF is the actor of the cf_lookup_call points (the CF-side site), BP the actor of the bp_create_call points: the G-I traces
# name actors by their run actor (a<n>), not by application.  Input: the trace.  Pilot/dev tooling.
$0 ~ /^(record|start_evidence|order|adequacy|observed|closure|unobserved|owner|owner_level|owner_id|owner_core|const|creator|not_app|loss|control) / { next }
NF >= 3 {
  EA[NE] = $1; ES[NE] = $2 " " $3; EN[NE] = ""; RES[NE] = ""
  for (i = 4; i <= NF; i++) {
    if ($i ~ /^arg\.sem_name=@/) { EN[NE] = $i; sub(/^arg\.sem_name=@/, "", EN[NE]) }
    if ($i ~ /^result=/) { RES[NE] = $i; sub(/^result=/, "", RES[NE]) }
  }
  if (ES[NE] == "cf_lookup_call pt") CF = $1
  if (ES[NE] == "bp_create_call pt") BP = $1
  NE++
}
END {
  nk = 0; ends = "?"
  for (i = 0; i < NE; i++) {
    if (EA[i] != CF) continue
    if (ES[i] == "CFE_ES_ExitApp call") ends = "x"
    if (ES[i] == "CF_Init ret" && RES[i] == "0") ends = "f"
    if (ES[i] != "OS_CountSemGetIdByName call") continue
    n = EN[i]; for (j = i + 1; j < NE; j++) if (EA[j] == CF && ES[j] == "OS_CountSemGetIdByName ret") break
    r = (RES[j] == "0" ? "0" : "-17"); if (RES[j] != "0" && RES[j] != "0xffffffef" && RES[j] != "-17") r = RES[j]
    if (!(n in KN)) { nk++; KO[nk] = n; KN[n] = 0 }
    KN[n]++; seq[n, KN[n]] = r
  }
  cls = ""
  for (k = 1; k <= nk; k++) {
    n = KO[k]; s = ""; last = ""; cnt = 0
    for (m = 1; m <= KN[n]; m++) { if (seq[n, m] != last) { if (cnt) s = s last "x" cnt " "; last = seq[n, m]; cnt = 0 } cnt++ }
    s = s last "x" cnt
    cls = cls sprintf("%s: %s ; ", n, s)
  }
  printf "class: %sends: CF=%s\nactors: CF=%s BP=%s\n", cls, ends, CF, BP
}

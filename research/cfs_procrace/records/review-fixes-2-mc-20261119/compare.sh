#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of declaration 12.0 (b) (plan/full_stage_results_20261116.md, section 12.11): the outputs of the
# rebuilt kz-static on the five reference builds against the kept outputs of the earlier tool (kz-static 1a47abd0), one
# step at a time.  The fixes change output on the default path too, so nothing is compared for byte identity with the
# earlier tool's output except where the step says so; every differing line must fall into a class of the step's rules
# file (rules/<kind>.rules, read by cdiff.py), and the containers are compared section by section (sections_cmp.py).
# Each check writes a report into CMP (default ./compare) and prints one line.  The step `declared` then compares the
# class and section lines of every report with the reports of the scratch check build committed with the declaration
# (./declared): the same lines, or the difference is not declared.
#   relations     rf2_relcheck/<b> against rf2_pre_relcheck/<b>: relations.txt (relations.rules), keys.txt (keys.rules),
#                 relations.log empty
#   order         rf2_order/<b> against rf2_pre_order/<b>: the two runs the same bytes (order.txt, edges.tsv; again/ is
#                 removed afterwards); order.txt (order.rules), edges.tsv (edges.rules); order.log empty
#   extract_off   rf2_off_extract/<b>/<b>/NP against ll_off_extract: basis.kz and facts.kz by section (sections_cmp.py);
#                 the facts views (kz-static show of both with the rebuilt tool, view.rules); extract.txt
#                 (extract.rules), relations.txt (relations.rules), basis.txt, sha256.txt and args.txt (files.rules);
#                 relations.log the same bytes as the kept one; status.txt says result PASS
#   extract_final rf2_final_extract/<b>/<b>/NP against ll_final_extract: the same checks
#   off_order     rf2_off_order/<b> against ll_off_order/<b>: two runs the same; order.txt, edges.tsv, keys.txt by their
#                 rules; relations.txt the same bytes as the re-extraction's; logs empty
#   final_order   rf2_final_order/<b> against ll_final_order/<b>: as off_order, and cdr.txt (cdr.rules)
#   plans_t8cf    rf2_plans_t8cf/<b> against mv_plans_v3_t8cf/<b>: plan.kzs, plan.side, plan.mod, plan.exe, args.txt
#                 (plan.rules); plan.log the same bytes
#   plans_final   rf2_plans_final/<b> against mv_plans_v3_final/<b>: the same
#   plans_v4      rf2_plans_v4/<b> against mv_plans_v4/<b>: the same and plan.req
#   plans_v4_all  rf2_plans_v4_all/<b> against mv_plans_v4_all/<b>: the same and plan.req
#   plans_v4_kept rf2_plans_v4_keptfacts/<b> against mv_plans_v4/<b>: the same and plan.req (the kept facts of the earlier
#                 tool read by the rebuilt generator)
#   twin          the outputs of the MC build against the outputs of the scratch check build of the same source
#                 (TWIN_SCRATCH, TWIN_SCRATCH_TOOL, TWIN_SCRATCH_TOOL_SHA256; twin.py): the same bytes apart from the declared path, tool-file and
#                 commit strings and the time line of status.txt
#   declared      the class, section, header, count and summary lines of every report in CMP against ./declared
# Environment: OROOT the root of the new outputs (default /home/user/work/redesign/obs1; the kept outputs are always read
# from there), CMP the report directory, KZB the tool directory (default the MC tree's tools), SCRATCH a scratch directory
# for the facts views (extract steps; the views are removed after each build), SAMPLES=1 to write up to three lines of
# each class beside each report (<report>.samples; for the reader, not compared).
# rc 0 when every check of the step holds, 1 otherwise, 64 on a usage error.
set -uo pipefail
W=/home/user/work/redesign/obs1
O=${OROOT:-$W}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
C=${CMP:-$HERE/compare}
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
STEP=${1:-}
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
mkdir -p "$C"
# cd RULES OLD NEW REPORT LABEL [--sections]
cdf() {
  local rules=$HERE/rules/$1.rules old=$2 new=$3 rep=$C/$4.txt label=$5 extra=${6:-}
  local s=(); [ "${SAMPLES:-0}" = 1 ] && s=(--samples "$rep.samples")
  [ -e "$rep" ] && { bad "$label: report $rep exists (a check output is never overwritten)"; return; }
  python3 -I "$HERE/cdiff.py" "$rules" "$old" "$new" $extra "${s[@]}" > "$rep"; local r=$?
  [ $r = 0 ] && ok "$label: $(head -1 "$rep")" || bad "$label: $(head -1 "$rep") ($4.txt)"
}
# sec OLD NEW REPORT LABEL
sec() {
  local rep=$C/$3.txt
  [ -e "$rep" ] && { bad "$4: report $rep exists"; return; }
  python3 -I "$HERE/sections_cmp.py" "$KD" "$1" "$2" > "$rep"; local r=$?
  [ $r = 0 ] && ok "$4: $(tail -1 "$rep")" || bad "$4: sections_cmp rc $r"
}
same() { cmp -s "$1" "$2" && ok "$3" || bad "$3: differs"; }
empty() { [ -e "$1" ] && [ ! -s "$1" ] && ok "$2 empty" || bad "$2 missing or not empty"; }
BUILDS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
case "$STEP" in
  relations)
    for b in $BUILDS; do
      cdf relations "$W/rf2_pre_relcheck/$b/relations.txt" "$O/rf2_relcheck/$b/relations.txt" "relations_relations_$b" "$b relations.txt"
      cdf keys "$W/rf2_pre_relcheck/$b/keys.txt" "$O/rf2_relcheck/$b/keys.txt" "relations_keys_$b" "$b keys.txt"
      empty "$O/rf2_relcheck/$b/relations.log" "$b relations.log"
    done ;;
  order|off_order|final_order)
    case "$STEP" in
      order) P=$W/rf2_pre_order; N=$O/rf2_order; X= ;;
      off_order) P=$W/ll_off_order; N=$O/rf2_off_order; X=$O/rf2_off_extract ;;
      final_order) P=$W/ll_final_order; N=$O/rf2_final_order; X=$O/rf2_final_extract ;;
    esac
    for b in $BUILDS; do
      fs="order.txt edges.tsv"; [ "$STEP" = final_order ] && fs="$fs cdr.txt"
      for f in $fs; do same "$N/$b/$f" "$N/$b/again/$f" "$b $f two runs the same bytes"; done
      cdf order "$P/$b/order.txt" "$N/$b/order.txt" "${STEP}_order_$b" "$b order.txt"
      cdf edges "$P/$b/edges.tsv" "$N/$b/edges.tsv" "${STEP}_edges_$b" "$b edges.tsv"
      [ "$STEP" = final_order ] && cdf cdr "$P/$b/cdr.txt" "$N/$b/cdr.txt" "${STEP}_cdr_$b" "$b cdr.txt"
      empty "$N/$b/order.log" "$b order.log"
      if [ -n "$X" ]; then
        same "$N/$b/relations.txt" "$X/$b/$b/NP/relations.txt" "$b relations.txt = the re-extraction's"
        cdf keys "$P/$b/keys.txt" "$N/$b/keys.txt" "${STEP}_keys_$b" "$b keys.txt"
        empty "$N/$b/relations.log" "$b relations.log"
      fi
      [ $rc = 0 ] && rm -rf "$N/$b/again"
    done ;;
  extract_off|extract_final)
    SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
    if [ "$STEP" = extract_off ]; then P=$W/ll_off_extract; N=$O/rf2_off_extract; else P=$W/ll_final_extract; N=$O/rf2_final_extract; fi
    for b in $BUILDS; do
      p=$P/$b/$b/NP; n=$N/$b/$b/NP
      sec "$p/basis.kz" "$n/basis.kz" "${STEP}_basis_sections_$b" "$b basis.kz"
      sec "$p/facts.kz" "$n/facts.kz" "${STEP}_facts_sections_$b" "$b facts.kz"
      if "$KS" show "$p/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$n/facts.kz" > "$SCRATCH/new_$b.txt"; then
        cdf view "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" "${STEP}_view_$b" "$b facts view" --sections
      else bad "$b show"; fi
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt"
      cdf extract "$p/extract.txt" "$n/extract.txt" "${STEP}_extract_$b" "$b extract.txt"
      cdf relations "$p/relations.txt" "$n/relations.txt" "${STEP}_relations_$b" "$b relations.txt"
      for f in basis.txt sha256.txt args.txt; do cdf files "$p/$f" "$n/$f" "${STEP}_${f%.txt}_$b" "$b $f"; done
      same "$p/relations.log" "$n/relations.log" "$b relations.log the same bytes as the kept one"
      grep -qx 'result PASS' "$n/status.txt" 2> /dev/null && ok "$b status.txt result PASS" || bad "$b status.txt"
    done ;;
  plans_t8cf|plans_final|plans_v4|plans_v4_all|plans_v4_kept)
    case "$STEP" in
      plans_t8cf) P=$W/mv_plans_v3_t8cf; N=$O/rf2_plans_t8cf; fs="plan.kzs plan.side plan.mod plan.exe args.txt" ;;
      plans_final) P=$W/mv_plans_v3_final; N=$O/rf2_plans_final; fs="plan.kzs plan.side plan.mod plan.exe args.txt" ;;
      plans_v4) P=$W/mv_plans_v4; N=$O/rf2_plans_v4; fs="plan.kzs plan.side plan.req plan.mod plan.exe args.txt" ;;
      plans_v4_all) P=$W/mv_plans_v4_all; N=$O/rf2_plans_v4_all; fs="plan.kzs plan.side plan.req plan.mod plan.exe args.txt" ;;
      plans_v4_kept) P=$W/mv_plans_v4; N=$O/rf2_plans_v4_keptfacts; fs="plan.kzs plan.side plan.req plan.mod plan.exe args.txt" ;;
    esac
    for b in $BUILDS; do
      for f in $fs; do cdf plan "$P/$b/$f" "$N/$b/$f" "${STEP}_$(echo "$f" | tr . _)_$b" "$b $f"; done
      same "$P/$b/plan.log" "$N/$b/plan.log" "$b plan.log the same bytes as the kept one"
    done ;;
  twin)
    TS=${TWIN_SCRATCH:?TWIN_SCRATCH: the output root of the scratch check build}
    TK=${TWIN_SCRATCH_TOOL:?TWIN_SCRATCH_TOOL: the path of the kz-static of the scratch check build, as its runs recorded it}
    ts=${TWIN_SCRATCH_TOOL_SHA256:?TWIN_SCRATCH_TOOL_SHA256: the file sha256 of that kz-static (pre_mc.txt)}
    SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the map}
    rep=$C/twin.txt
    [ -e "$rep" ] && { bad "twin: report exists"; exit 1; }
    rs=$(sha256sum < "$KS" | cut -c1-64)
    sr=$(grep -h '^repo ' "$TS"/rf2_off_extract/*/*/NP/status.txt | sort -u); rr=$(grep -h '^repo ' "$O"/rf2_off_extract/*/*/NP/status.txt | sort -u)
    [ "$(echo "$sr" | wc -l)" = 1 ] && [ "$(echo "$rr" | wc -l)" = 1 ] || { bad "twin: the runs do not name one repository commit each"; exit 1; }
    printf '%s\t%s\n%s\t%s\n%s\t%s\n%s\t%s\n' "$TS/" "$O/" "$TK" "$KS" "$ts" "$rs" "$sr" "$rr" > "$SCRATCH/twin_map.tsv"
    python3 -I "$HERE/twin.py" "$TS" "$O" "$SCRATCH/twin_map.tsv" rf2_relcheck rf2_order rf2_off_extract rf2_final_extract rf2_off_order rf2_final_order \
      rf2_plans_t8cf rf2_plans_final rf2_plans_v4 rf2_plans_v4_all rf2_plans_v4_keptfacts > "$rep"; r=$?
    [ $r = 0 ] && ok "twin: $(tail -1 "$rep")" || bad "twin: $(tail -1 "$rep") (twin.txt)" ;;
  declared)
    D=$HERE/declared
    n=0
    for f in "$D"/*.txt; do
      g=$C/$(basename "$f"); n=$((n + 1))
      if [ ! -e "$g" ]; then bad "$(basename "$f"): no report in $(basename "$C")"; continue; fi
      if cmp -s <(grep -E '^(cdiff|class|header|section|count|summary) ' "$f") <(grep -E '^(cdiff|class|header|section|count|summary) ' "$g"); then ok "$(basename "$f") = declared"; else bad "$(basename "$f") differs from the declared report"; fi
    done
    for g in "$C"/*.txt; do [ "$(basename "$g")" = twin.txt ] && continue; [ -e "$D/$(basename "$g")" ] || bad "$(basename "$g"): a report with no declared counterpart"; done
    echo "reports declared: $n" ;;
  *) echo "usage: compare.sh relations|order|extract_off|extract_final|off_order|final_order|plans_t8cf|plans_final|plans_v4|plans_v4_all|plans_v4_kept|twin|declared" >&2; exit 64 ;;
esac
exit $rc

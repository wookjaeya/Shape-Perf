#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration of the loop flag and lock handle build (plan/full_stage_results_20261116.md
# 10.0), one step at a time.  The default path against the result flow step's outputs (the same bytes apart from the tool
# identity), the final option-on round against that step's option-on round (the declared differences only) and against
# the three intermediate option-on rounds (plan 4.3), and the measurements:
#   relations    ll_relcheck/<b> against rf_relcheck: relations.txt and keys.txt the same bytes; relations.log empty
#   order        ll_order/<b>: two runs the same bytes; order.txt and edges.tsv = rf_order's; order.log empty
#   extract      ll_off_extract/<b>/<b>/NP against rf_off_extract: basis.kz record by record (check_head.py --whole),
#                facts.kz record by record (check_whole.py), the facts views only the extract.units_from line,
#                extract.txt apart from line 1, relations.txt apart from line 2, basis.txt and sha256.txt only the hash
#                lines, args.txt and relations.log the same
#   onextract    ll_off_order/<b>: two runs the same; order.txt against rf_off_order apart from line 2, edges.tsv the
#                same; relations.txt = the re-extraction's; keys.txt = rf_off_order's apart from line 2; logs empty
#   plans        compare_plans.py t8cf rf_plans ll_plans; compare_plans.py extract rf_plans_off ll_plans_off
#   final        ll_final_extract/<b>/<b>/NP against rf_on_extract: basis.kz record by record (check_head.py --whole);
#                facts.kz header lines (check_head_rf.py --allow-size --new-sections 2); the sections by record text
#                (section_text_cmp_ll.py: the same but STRINGS and BASIS, FACT_EXPRS the earlier rows first, LOOP_DEFS
#                and LOCK_HANDLES new; compare/final_sections_<b>.txt) and the record-text sha256 of every section
#                (section_text_sha_2.py -> section_text_final.txt); the facts views (compare_ll.py, compare/final_<b>.txt);
#                extract.txt apart from line 1 two new lines `loop defs` and `lock handles`; relations.txt
#                (compare_rel_ll.py); args.txt only the extract-only line (` --loop-defs --lock-handles` at its end);
#                basis.txt and sha256.txt only the hash lines; relations.log empty
#   final_order  ll_final_order/<b>: two runs the same bytes (order.txt, edges.tsv, cdr.txt); order.txt against rf_on_order
#                (compare_ord_ll.py), edges.tsv (compare_ord_ll.py --tsv); relations.txt = the final re-extraction's;
#                keys.txt = rf_on_order's apart from line 2; logs empty
#   plans_final  compare_plans.py extract rf_plans_on ll_plans_final
#   rounds       (plan 4.3) the final round against each intermediate option-on round by record text
#                (section_text_cmp_ll.py with the sections the later options add: objacc_on_extract, sem_on_extract,
#                rf_on_extract; compare/rounds_<round>_<b>.txt); and each intermediate round's kept per-section record-text
#                sha256 (records/semantics-20261116/section_text_old_2.txt and section_text_new_2.txt,
#                records/result-flow-20261116/section_text_on.txt) against section_text_final.txt (rounds_cmp.py ->
#                compare/rounds_sha.txt)
#   measure      the views of LOOP_DEFS (kept, compare/loopdefs_<b>.txt) and LOCK_HANDLES (SCRATCH, removed);
#                measure_ll.py -> compare/measure_<b>.txt
# One line per check; rc 0 when every check holds, 1 otherwise.  Tools: the build tree's (MC).  The facts views are written
# to SCRATCH and removed after each build's comparison.
set -uo pipefail
O=/home/user/work/redesign/obs1
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
except_line() { cmp -s <(sed "$1d" "$2") <(sed "$1d" "$3"); }
C=$HERE/compare
mkdir -p "$C"
[ "${1:-}" = final ] && : > "$HERE/section_text_final.txt.partial"
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  case "${1:-}" in
    relations)
      for f in relations.txt keys.txt; do cmp -s "$O/rf_relcheck/$b/$f" "$O/ll_relcheck/$b/$f" && ok "$b $f = rf_relcheck's" || bad "$b $f differs from rf_relcheck's"; done
      [ -s "$O/ll_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do cmp -s "$O/ll_order/$b/$f" "$O/ll_order/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      for f in order.txt edges.tsv; do cmp -s "$O/rf_order/$b/$f" "$O/ll_order/$b/$f" && ok "$b $f = rf_order's" || bad "$b $f differs from rf_order's"; done
      [ -s "$O/ll_order/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/ll_order/$b/again" ;;
    extract)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/ll_off_extract/$b/$b/NP; P=$O/rf_off_extract/$b/$b/NP
      python3 -I "$HERE/check_head.py" "$KD" "$P/basis.kz" "$N/basis.kz" --whole > "$C/basis_$b.txt"; [ $? = 0 ] && ok "$b basis.kz: $(head -1 "$C/basis_$b.txt")" || bad "$b basis.kz (compare/basis_$b.txt)"
      python3 -I "$HERE/check_whole.py" "$KD" "$P/facts.kz" "$N/facts.kz" > "$C/facts_whole_$b.txt"; [ $? = 0 ] && ok "$b facts.kz: $(head -1 "$C/facts_whole_$b.txt")" || bad "$b facts.kz (compare/facts_whole_$b.txt)"
      "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
      d=$(diff "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" | grep '^[<>]' | sed -E 's/^([<>])   extract\.units_from = basis [0-9a-f]{16}  \(unset\)$/\1 UNITS/' | sort | uniq -c | tr -s ' ' | tr '\n' ';')
      [ "$d" = " 1 < UNITS; 1 > UNITS;" ] && ok "$b facts view: only the extract.units_from line ($(wc -l < "$SCRATCH/new_$b.txt") lines)" || bad "$b facts view differs beyond the extract.units_from line ($d)"
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt"
      except_line 1 "$P/extract.txt" "$N/extract.txt" && ok "$b extract.txt same apart from line 1" || bad "$b extract.txt differs beyond line 1"
      except_line 2 "$P/relations.txt" "$N/relations.txt" && ok "$b relations.txt same apart from line 2 (facts hash)" || bad "$b relations.txt differs beyond line 2"
      cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
      cmp -s "$P/args.txt" "$N/args.txt" && ok "$b args.txt same" || bad "$b args.txt differs"
      cmp -s "$P/relations.log" "$N/relations.log" && ok "$b relations.log same" || bad "$b relations.log differs" ;;
    onextract)
      E=$O/ll_off_order/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      except_line 2 "$O/rf_off_order/$b/order.txt" "$E/order.txt" && ok "$b order.txt = rf_off_order's apart from line 2" || bad "$b order.txt differs from rf_off_order's beyond line 2"
      cmp -s "$O/rf_off_order/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv = rf_off_order's" || bad "$b edges.tsv differs from rf_off_order's"
      cmp -s "$E/relations.txt" "$O/ll_off_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the re-extraction's" || bad "$b relations.txt differs from the re-extraction's"
      except_line 2 "$O/rf_off_order/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = rf_off_order's apart from line 2" || bad "$b keys.txt differs from rf_off_order's beyond line 2"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      rm -rf "$E/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" t8cf "$O/rf_plans/$b" "$O/ll_plans/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" extract "$O/rf_plans_off/$b" "$O/ll_plans_off/$b" | sed "s/^/$b extract /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    final)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/ll_final_extract/$b/$b/NP; P=$O/rf_on_extract/$b/$b/NP
      python3 -I "$HERE/check_head.py" "$KD" "$P/basis.kz" "$N/basis.kz" --whole > "$C/final_basis_$b.txt"; [ $? = 0 ] && ok "$b basis.kz: $(head -1 "$C/final_basis_$b.txt")" || bad "$b basis.kz (compare/final_basis_$b.txt)"
      python3 -I "$HERE/check_head_rf.py" "$KD" "$P/facts.kz" "$N/facts.kz" --allow-size --new-sections 2 > "$C/final_facts_head_$b.txt"; [ $? = 0 ] && ok "$b facts.kz header: $(head -1 "$C/final_facts_head_$b.txt")" || bad "$b facts.kz header (compare/final_facts_head_$b.txt)"
      python3 -I "$HERE/section_text_cmp_ll.py" "$KD" "$P/facts.kz" "$N/facts.kz" > "$C/final_sections_$b.txt"; [ $? = 0 ] && ok "$b sections by record text: $(head -1 "$C/final_sections_$b.txt" | cut -d: -f2-)" || bad "$b sections by record text (compare/final_sections_$b.txt)"
      python3 -I "$HERE/section_text_sha_2.py" "$KD" "$N/facts.kz" >> "$HERE/section_text_final.txt.partial" || bad "$b section record text sha256"
      "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
      python3 -I "$HERE/compare_ll.py" "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" > "$C/final_$b.txt"; [ $? = 0 ] && ok "$b facts views: $(head -1 "$C/final_$b.txt")" || bad "$b facts views (compare/final_$b.txt)"
      d=$(diff <(sed 1d "$P/extract.txt") <(sed 1d "$N/extract.txt") | grep '^[<>]' | sed -E 's/^> loop defs                    [0-9]+ rows .*$/> LD/; s/^> lock handles                 [0-9]+ rows .*$/> LH/' | tr '\n' ';')
      [ "$d" = "> LD;> LH;" ] && ok "$b extract.txt: apart from line 1 two new lines loop defs, lock handles" || bad "$b extract.txt ($d)"
      python3 -I "$HERE/compare_rel_ll.py" "$P/relations.txt" "$N/relations.txt" > "$C/final_relations_$b.txt"; [ $? = 0 ] && ok "$b relations.txt: $(head -1 "$C/final_relations_$b.txt" | cut -c1-120)" || bad "$b relations.txt (compare/final_relations_$b.txt)"
      d=$(diff "$P/args.txt" "$N/args.txt" | grep '^[<>]' | sed -E 's/^< (extract-only args: .*)$/< X/; s/^> (extract-only args: .*) --loop-defs --lock-handles$/> X/' | tr '\n' ';')
      x=$(grep '^extract-only args: ' "$P/args.txt"); y=$(grep '^extract-only args: ' "$N/args.txt")
      [ "$d" = "< X;> X;" ] && [ "$y" = "$x --loop-defs --lock-handles" ] && ok "$b args.txt: only the extract-only line (--loop-defs --lock-handles at its end)" || bad "$b args.txt ($d)"
      cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
      [ -s "$N/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty"
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" ;;
    final_order)
      E=$O/ll_final_order/$b
      for f in order.txt edges.tsv cdr.txt; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      python3 -I "$HERE/compare_ord_ll.py" "$O/rf_on_order/$b/order.txt" "$E/order.txt" > "$C/final_order_$b.txt"; [ $? = 0 ] && ok "$b order.txt: $(head -1 "$C/final_order_$b.txt")" || bad "$b order.txt (compare/final_order_$b.txt)"
      python3 -I "$HERE/compare_ord_ll.py" --tsv "$O/rf_on_order/$b/edges.tsv" "$E/edges.tsv" > "$C/final_edges_$b.txt"; [ $? = 0 ] && ok "$b edges.tsv: $(head -1 "$C/final_edges_$b.txt")" || bad "$b edges.tsv (compare/final_edges_$b.txt)"
      cmp -s "$E/relations.txt" "$O/ll_final_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the final re-extraction's" || bad "$b relations.txt differs from the final re-extraction's"
      except_line 2 "$O/rf_on_order/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = rf_on_order's apart from line 2" || bad "$b keys.txt differs from rf_on_order's beyond line 2"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      rm -rf "$E/again" ;;
    plans_final)
      python3 -I "$HERE/compare_plans.py" extract "$O/rf_plans_on/$b" "$O/ll_plans_final/$b" | sed "s/^/$b extract-final /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    rounds)
      N=$O/ll_final_extract/$b/$b/NP/facts.kz
      python3 -I "$HERE/section_text_cmp_ll.py" "$KD" "$O/objacc_on_extract/$b/$b/NP/facts.kz" "$N" --new LOCK_HANDLES,LOOP_DEFS,RESULT_EFFECT,RESULT_FLOW,SEMANTICS > "$C/rounds_objacc_$b.txt"; [ $? = 0 ] && ok "$b objacc round: $(cut -d: -f2- "$C/rounds_objacc_$b.txt" | cut -c1-200)" || bad "$b objacc round (compare/rounds_objacc_$b.txt)"
      python3 -I "$HERE/section_text_cmp_ll.py" "$KD" "$O/sem_on_extract/$b/$b/NP/facts.kz" "$N" --new LOCK_HANDLES,LOOP_DEFS,RESULT_EFFECT,RESULT_FLOW > "$C/rounds_sem_$b.txt"; [ $? = 0 ] && ok "$b sem round: $(cut -d: -f2- "$C/rounds_sem_$b.txt" | cut -c1-200)" || bad "$b sem round (compare/rounds_sem_$b.txt)"
      python3 -I "$HERE/section_text_cmp_ll.py" "$KD" "$O/rf_on_extract/$b/$b/NP/facts.kz" "$N" > "$C/rounds_rf_$b.txt"; [ $? = 0 ] && ok "$b rf round: $(cut -d: -f2- "$C/rounds_rf_$b.txt" | cut -c1-200)" || bad "$b rf round (compare/rounds_rf_$b.txt)" ;;
    measure)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the LOCK_HANDLES view}
      F=$O/ll_final_extract/$b/$b/NP
      "$KS" show "$F/facts.kz" --section loopdefs > "$C/loopdefs_$b.txt" && "$KS" show "$F/facts.kz" --section lockhandles > "$SCRATCH/lh_$b.txt" || bad "$b show"
      python3 -I "$HERE/measure_ll.py" "$F/extract.txt" "$C/loopdefs_$b.txt" "$SCRATCH/lh_$b.txt" "$O/rf_on_order/$b/order.txt" "$O/ll_final_order/$b/order.txt" \
        "$O/ll_final_order/$b/cdr.txt" "$O/ll_final_order/$b/relations.txt" > "$C/measure_$b.txt" && ok "$b measured (compare/measure_$b.txt)" || bad "$b measure_ll"
      rm -f "$SCRATCH/lh_$b.txt" ;;
    *) echo "usage: compare.sh relations|order|extract|onextract|plans|final|final_order|plans_final|rounds|measure" >&2; exit 64 ;;
  esac
done
if [ "${1:-}" = final ]; then mv "$HERE/section_text_final.txt.partial" "$HERE/section_text_final.txt"; fi
if [ "${1:-}" = rounds ]; then python3 -I "$HERE/rounds_cmp.py" "$HERE/../semantics-20261116/section_text_old_2.txt" "$HERE/../semantics-20261116/section_text_new_2.txt" "$HERE/../result-flow-20261116/section_text_on.txt" "$HERE/section_text_final.txt" > "$C/rounds_sha.txt"; [ $? = 0 ] && ok "rounds sha: $(head -1 "$C/rounds_sha.txt")" || bad "rounds sha (compare/rounds_sha.txt)"; fi
exit $rc

#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration of the result flow build (plan/full_stage_results_20261116.md 9.0),
# one step at a time.  The default path against the semantics step's outputs (the same bytes apart from the tool
# identity), the option-on round against that step's option-on round (the declared differences only), and the measurements:
#   relations    rf_relcheck/<b> against sem_relcheck: relations.txt and keys.txt the same bytes; relations.log empty
#   order        rf_order/<b>: two runs the same bytes; order.txt and edges.tsv = sem_order's; order.log empty
#   extract      rf_off_extract/<b>/<b>/NP against sem_off_extract: basis.kz record by record (check_head.py --whole),
#                facts.kz record by record (check_whole.py), the facts views only the extract.units_from line,
#                extract.txt apart from line 1, relations.txt apart from line 2, basis.txt and sha256.txt only the hash
#                lines, args.txt and relations.log the same
#   onextract    rf_off_order/<b>: two runs the same; order.txt against sem_off_order apart from line 2, edges.tsv the
#                same; relations.txt = the re-extraction's; keys.txt = sem_off_order's apart from line 2; logs empty
#   plans        compare_plans.py t8cf sem_plans rf_plans; compare_plans.py extract sem_plans_off rf_plans_off
#   extract_on   rf_on_extract/<b>/<b>/NP against sem_on_extract: basis.kz record by record (check_head.py --whole);
#                facts.kz header lines (check_head_rf.py --allow-size --new-sections 2); the sections by record text
#                (section_text_cmp.py: the same but STRINGS and BASIS, FACT_EXPRS the earlier rows first, RESULT_FLOW and
#                RESULT_EFFECT new; compare/on_sections_<b>.txt) and the record-text sha256 of every section
#                (section_text_sha_2.py -> section_text_on.txt); the facts views (compare_rf.py, compare/on_<b>.txt);
#                extract.txt apart from line 1 two new lines `result flow` and `result effect`; relations.txt
#                (compare_rel_rf.py); args.txt only the extract-only line (--result-flow at its end); basis.txt and
#                sha256.txt only the hash lines; relations.log empty
#   onextract_on rf_on_order/<b>: two runs the same bytes; order.txt against sem_on_order (compare_ord_rf.py), edges.tsv
#                (compare_ord_rf.py --tsv); relations.txt = the on re-extraction's; keys.txt = sem_on_order's apart from
#                line 2; logs empty
#   plans_on     compare_plans.py extract sem_plans_on rf_plans_on
#   measure      the offline probe (s1_rev1.py on the relations.txt kept beside the tests_8c_fix facts, on rf_relcheck's
#                and on sem_on_order's; s1_rev2.py on rf_on_order's) into compare/s1_*_<b>.out and .tsv; measure_rf.py
#                (compare/measure_<b>.txt; the use-site key of the C1 relation 1 for the first path)
#   extract_on_rel (results 9.0a) the relations text of rf_on_extract against sem_on_extract again with compare_rel_rf_2.py
#                (compare/on_relations_2_<b>.txt): the check of extract_on whose declared allowance was too narrow
# One line per check; rc 0 when every check holds, 1 otherwise.  Tools: the build tree's (MC).  The facts views are written
# to SCRATCH and removed after each build's comparison.
set -uo pipefail
O=/home/user/work/redesign/obs1
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROBE=$HERE/../../plan/ovm_rev1_20261116/probe
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
except_line() { cmp -s <(sed "$1d" "$2") <(sed "$1d" "$3"); }
C=$HERE/compare
mkdir -p "$C"
[ "${1:-}" = extract_on ] && : > "$HERE/section_text_on.txt.partial"
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  case "${1:-}" in
    relations)
      for f in relations.txt keys.txt; do cmp -s "$O/sem_relcheck/$b/$f" "$O/rf_relcheck/$b/$f" && ok "$b $f = sem_relcheck's" || bad "$b $f differs from sem_relcheck's"; done
      [ -s "$O/rf_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do cmp -s "$O/rf_order/$b/$f" "$O/rf_order/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      for f in order.txt edges.tsv; do cmp -s "$O/sem_order/$b/$f" "$O/rf_order/$b/$f" && ok "$b $f = sem_order's" || bad "$b $f differs from sem_order's"; done
      [ -s "$O/rf_order/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/rf_order/$b/again" ;;
    extract)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/rf_off_extract/$b/$b/NP; P=$O/sem_off_extract/$b/$b/NP
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
      E=$O/rf_off_order/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      except_line 2 "$O/sem_off_order/$b/order.txt" "$E/order.txt" && ok "$b order.txt = sem_off_order's apart from line 2" || bad "$b order.txt differs from sem_off_order's beyond line 2"
      cmp -s "$O/sem_off_order/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv = sem_off_order's" || bad "$b edges.tsv differs from sem_off_order's"
      cmp -s "$E/relations.txt" "$O/rf_off_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the re-extraction's" || bad "$b relations.txt differs from the re-extraction's"
      except_line 2 "$O/sem_off_order/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = sem_off_order's apart from line 2" || bad "$b keys.txt differs from sem_off_order's beyond line 2"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      rm -rf "$E/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" t8cf "$O/sem_plans/$b" "$O/rf_plans/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" extract "$O/sem_plans_off/$b" "$O/rf_plans_off/$b" | sed "s/^/$b extract /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    extract_on)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/rf_on_extract/$b/$b/NP; P=$O/sem_on_extract/$b/$b/NP
      python3 -I "$HERE/check_head.py" "$KD" "$P/basis.kz" "$N/basis.kz" --whole > "$C/on_basis_$b.txt"; [ $? = 0 ] && ok "$b basis.kz: $(head -1 "$C/on_basis_$b.txt")" || bad "$b basis.kz (compare/on_basis_$b.txt)"
      python3 -I "$HERE/check_head_rf.py" "$KD" "$P/facts.kz" "$N/facts.kz" --allow-size --new-sections 2 > "$C/on_facts_head_$b.txt"; [ $? = 0 ] && ok "$b facts.kz header: $(head -1 "$C/on_facts_head_$b.txt")" || bad "$b facts.kz header (compare/on_facts_head_$b.txt)"
      python3 -I "$HERE/section_text_cmp.py" "$KD" "$P/facts.kz" "$N/facts.kz" > "$C/on_sections_$b.txt"; [ $? = 0 ] && ok "$b sections by record text: $(head -1 "$C/on_sections_$b.txt" | cut -d: -f2-)" || bad "$b sections by record text (compare/on_sections_$b.txt)"
      python3 -I "$HERE/section_text_sha_2.py" "$KD" "$N/facts.kz" >> "$HERE/section_text_on.txt.partial" || bad "$b section record text sha256"
      "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
      python3 -I "$HERE/compare_rf.py" "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" > "$C/on_$b.txt"; [ $? = 0 ] && ok "$b facts views: $(head -1 "$C/on_$b.txt")" || bad "$b facts views (compare/on_$b.txt)"
      d=$(diff <(sed 1d "$P/extract.txt") <(sed 1d "$N/extract.txt") | grep '^[<>]' | sed -E 's/^> result flow                  [0-9]+ rows .*$/> RF/; s/^> result effect                [0-9]+ rows .*$/> RE/' | tr '\n' ';')
      [ "$d" = "> RF;> RE;" ] && ok "$b extract.txt: apart from line 1 two new lines result flow, result effect" || bad "$b extract.txt ($d)"
      python3 -I "$HERE/compare_rel_rf.py" "$P/relations.txt" "$N/relations.txt" > "$C/on_relations_$b.txt"; [ $? = 0 ] && ok "$b relations.txt: $(head -1 "$C/on_relations_$b.txt")" || bad "$b relations.txt (compare/on_relations_$b.txt)"
      d=$(diff "$P/args.txt" "$N/args.txt" | grep '^[<>]' | sed -E 's/^< (extract-only args: .*)$/< X/; s/^> (extract-only args: .*) --result-flow$/> X/' | tr '\n' ';')
      x=$(grep '^extract-only args: ' "$P/args.txt"); y=$(grep '^extract-only args: ' "$N/args.txt")
      [ "$d" = "< X;> X;" ] && [ "$y" = "$x --result-flow" ] && ok "$b args.txt: only the extract-only line (--result-flow at its end)" || bad "$b args.txt ($d)"
      cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
      [ -s "$N/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty"
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" ;;
    onextract_on)
      E=$O/rf_on_order/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      python3 -I "$HERE/compare_ord_rf.py" "$O/sem_on_order/$b/order.txt" "$E/order.txt" > "$C/on_order_$b.txt"; [ $? = 0 ] && ok "$b order.txt: $(head -1 "$C/on_order_$b.txt")" || bad "$b order.txt (compare/on_order_$b.txt)"
      python3 -I "$HERE/compare_ord_rf.py" --tsv "$O/sem_on_order/$b/edges.tsv" "$E/edges.tsv" > "$C/on_edges_$b.txt"; [ $? = 0 ] && ok "$b edges.tsv: $(head -1 "$C/on_edges_$b.txt")" || bad "$b edges.tsv (compare/on_edges_$b.txt)"
      cmp -s "$E/relations.txt" "$O/rf_on_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the on re-extraction's" || bad "$b relations.txt differs from the on re-extraction's"
      except_line 2 "$O/sem_on_order/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = sem_on_order's apart from line 2" || bad "$b keys.txt differs from sem_on_order's beyond line 2"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      rm -rf "$E/again" ;;
    plans_on)
      python3 -I "$HERE/compare_plans.py" extract "$O/sem_plans_on/$b" "$O/rf_plans_on/$b" | sed "s/^/$b extract-on /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    measure)
      python3 -I "$PROBE/s1_rev1.py" "/home/user/work/redesign/step2a/tests_8c_fix/$b/NP/relations.txt" --tsv "$C/s1_rev1_plan_$b.tsv" > "$C/s1_rev1_plan_$b.out" || bad "$b s1 rev 1 (the relations kept with the tests_8c_fix facts)"
      python3 -I "$PROBE/s1_rev1.py" "$O/rf_relcheck/$b/relations.txt" --tsv "$C/s1_rev1_rules_$b.tsv" > "$C/s1_rev1_rules_$b.out" || bad "$b s1 rev 1 (rf_relcheck relations)"
      python3 -I "$PROBE/s1_rev1.py" "$O/sem_on_order/$b/relations.txt" --tsv "$C/s1_rev1_before_$b.tsv" > "$C/s1_rev1_before_$b.out" || bad "$b s1 rev 1 (sem_on_order relations)"
      python3 -I "$PROBE/s1_rev2.py" "$O/rf_on_order/$b/relations.txt" --tsv "$C/s1_rev2_after_$b.tsv" > "$C/s1_rev2_after_$b.out" || bad "$b s1 rev 2 (rf_on_order relations)"
      python3 -I "$HERE/measure_rf.py" "$O/rf_on_extract/$b/$b/NP/extract.txt" "$O/rf_on_order/$b/relations.txt" "$O/rf_on_order/$b/keys.txt" "$O/sem_on_order/$b/order.txt" "$O/rf_on_order/$b/order.txt" \
        "$C/s1_rev1_plan_$b.out" "$C/s1_rev1_rules_$b.out" "$C/s1_rev1_before_$b.out" "$C/s1_rev2_after_$b.out" --key 17975444d83f6589 > "$C/measure_$b.txt" && ok "$b measured (compare/measure_$b.txt: $(grep '^T? ' "$C/measure_$b.txt"))" || bad "$b measure_rf" ;;
    extract_on_rel)
      N=$O/rf_on_extract/$b/$b/NP; P=$O/sem_on_extract/$b/$b/NP
      python3 -I "$HERE/compare_rel_rf_2.py" "$P/relations.txt" "$N/relations.txt" > "$C/on_relations_2_$b.txt"; [ $? = 0 ] && ok "$b relations.txt: $(head -1 "$C/on_relations_2_$b.txt")" || bad "$b relations.txt (compare/on_relations_2_$b.txt)" ;;
    *) echo "usage: compare.sh relations|order|extract|onextract|plans|extract_on|extract_on_rel|onextract_on|plans_on|measure" >&2; exit 64 ;;
  esac
done
[ "${1:-}" = extract_on ] && mv "$HERE/section_text_on.txt.partial" "$HERE/section_text_on.txt"
exit $rc

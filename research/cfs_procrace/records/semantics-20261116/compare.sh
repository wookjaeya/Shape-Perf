#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration of the semantics build (plan/full_stage_results_20261116.md 8.0),
# one step at a time.  The default path against the types and object access step's outputs (the same bytes apart from the
# tool identity), the option-on round against that step's option-on round (the declared differences only), and the
# measurements of the new section:
#   relations    sem_relcheck/<b> against objacc_relcheck: relations.txt and keys.txt the same bytes; relations.log empty
#   order        sem_order/<b>: two runs the same bytes; order.txt and edges.tsv = objacc_order's; order.log empty
#   extract      sem_off_extract/<b>/<b>/NP against objacc_off_extract: basis.kz record by record (check_head.py --whole),
#                facts.kz record by record (check_whole.py), the facts views only the extract.units_from line,
#                extract.txt apart from line 1, relations.txt apart from line 2, basis.txt and sha256.txt only the hash
#                lines, args.txt and relations.log the same
#   onextract    sem_off_order/<b>: two runs the same; order.txt against objacc_off_order apart from line 2, edges.tsv the
#                same; relations.txt = the re-extraction's; keys.txt = objacc_off_order's apart from line 2; logs empty
#   plans        compare_plans.py t8cf objacc_plans sem_plans; compare_plans.py extract objacc_plans_off sem_plans_off
#   extract_on   sem_on_extract/<b>/<b>/NP against objacc_on_extract: basis.kz record by record (check_head.py --whole);
#                the per-section sha256 (section_sha.py -> section_sha_on.txt; compare_sections.py against the earlier
#                round's records/object-access-20261116/section_sha_on.txt); facts.kz header lines (check_head.py
#                --allow-size); the facts views (compare_sem.py, compare/on_<b>.txt); extract.txt apart from line 1 one new
#                line `semantics ...`; relations.txt (compare_rel_sem.py); args.txt only the extract-only line;
#                basis.txt and sha256.txt only the hash lines; relations.log empty
#   extract_on_text (results 8.0b) the facts.kz header with the line sections= one more (check_head_sem.py
#                --allow-new-section, compare/on_facts_head_sem_<b>.txt) and the sections by their record text
#                (section_text_sha.py of both rounds -> section_text_old.txt, section_text_new.txt; compare_sections_text.py
#                -> compare/sections_text_on.txt): the two checks of extract_on whose declared allowance was too narrow
#   onextract_on sem_on_order/<b>: two runs the same bytes; order.txt against objacc_on_order apart from line 2, edges.tsv
#                the same; relations.txt = the on re-extraction's; keys.txt = objacc_on_order's apart from line 2; logs
#                empty
#   plans_on     compare_plans.py extract objacc_plans_on sem_plans_on
#   measure      measure_sem.py on the views, the relations and keys of sem_on_order and the plans of sem_plans_on
#                (compare/measure_<b>.txt; the use-site key of the C1 relation 1 for the first path)
# One line per check; rc 0 when every check holds, 1 otherwise.  Tools: the build tree's (MC).  The facts views are written
# to SCRATCH and removed after each build's comparison.
set -uo pipefail
O=/home/user/work/redesign/obs1
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREV=$HERE/../object-access-20261116
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
except_line() { cmp -s <(sed "$1d" "$2") <(sed "$1d" "$3"); }
C=$HERE/compare
mkdir -p "$C"
[ "${1:-}" = extract_on ] && : > "$HERE/section_sha_on.txt.partial"
TR=${TEXT_ROUND:-}
case "$TR" in ""|_2) ;; *) echo "TEXT_ROUND: unset or _2" >&2; exit 64 ;; esac
[ "${1:-}" = extract_on_text ] && : > "$HERE/section_text_old${TR}.txt.partial" && : > "$HERE/section_text_new${TR}.txt.partial"
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  case "${1:-}" in
    relations)
      for f in relations.txt keys.txt; do cmp -s "$O/objacc_relcheck/$b/$f" "$O/sem_relcheck/$b/$f" && ok "$b $f = objacc_relcheck's" || bad "$b $f differs from objacc_relcheck's"; done
      [ -s "$O/sem_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do cmp -s "$O/sem_order/$b/$f" "$O/sem_order/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      for f in order.txt edges.tsv; do cmp -s "$O/objacc_order/$b/$f" "$O/sem_order/$b/$f" && ok "$b $f = objacc_order's" || bad "$b $f differs from objacc_order's"; done
      [ -s "$O/sem_order/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/sem_order/$b/again" ;;
    extract)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/sem_off_extract/$b/$b/NP; P=$O/objacc_off_extract/$b/$b/NP
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
      E=$O/sem_off_order/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      except_line 2 "$O/objacc_off_order/$b/order.txt" "$E/order.txt" && ok "$b order.txt = objacc_off_order's apart from line 2" || bad "$b order.txt differs from objacc_off_order's beyond line 2"
      cmp -s "$O/objacc_off_order/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv = objacc_off_order's" || bad "$b edges.tsv differs from objacc_off_order's"
      cmp -s "$E/relations.txt" "$O/sem_off_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the re-extraction's" || bad "$b relations.txt differs from the re-extraction's"
      except_line 2 "$O/objacc_off_order/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = objacc_off_order's apart from line 2" || bad "$b keys.txt differs from objacc_off_order's beyond line 2"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      rm -rf "$E/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" t8cf "$O/objacc_plans/$b" "$O/sem_plans/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" extract "$O/objacc_plans_off/$b" "$O/sem_plans_off/$b" | sed "s/^/$b extract /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    extract_on)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/sem_on_extract/$b/$b/NP; P=$O/objacc_on_extract/$b/$b/NP
      python3 -I "$HERE/check_head.py" "$KD" "$P/basis.kz" "$N/basis.kz" --whole > "$C/on_basis_$b.txt"; [ $? = 0 ] && ok "$b basis.kz: $(head -1 "$C/on_basis_$b.txt")" || bad "$b basis.kz (compare/on_basis_$b.txt)"
      python3 -I "$HERE/check_head.py" "$KD" "$P/facts.kz" "$N/facts.kz" --allow-size > "$C/on_facts_head_$b.txt"; [ $? = 0 ] && ok "$b facts.kz header: $(head -1 "$C/on_facts_head_$b.txt")" || bad "$b facts.kz header (compare/on_facts_head_$b.txt)"
      python3 -I "$HERE/section_sha.py" "$KD" "$N/facts.kz" >> "$HERE/section_sha_on.txt.partial" || bad "$b section sha256"
      "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
      python3 -I "$HERE/compare_sem.py" "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" > "$C/on_$b.txt"; [ $? = 0 ] && ok "$b facts views: $(head -1 "$C/on_$b.txt")" || bad "$b facts views (compare/on_$b.txt)"
      d=$(diff <(sed 1d "$P/extract.txt") <(sed 1d "$N/extract.txt") | grep '^[<>]' | sed -E 's/^> semantics                    rows .*$/> SEM/' | tr '\n' ';')
      [ "$d" = "> SEM;" ] && ok "$b extract.txt: apart from line 1 one new line semantics" || bad "$b extract.txt ($d)"
      python3 -I "$HERE/compare_rel_sem.py" "$P/relations.txt" "$N/relations.txt" > "$C/on_relations_$b.txt"; [ $? = 0 ] && ok "$b relations.txt: $(head -1 "$C/on_relations_$b.txt")" || bad "$b relations.txt (compare/on_relations_$b.txt)"
      d=$(diff "$P/args.txt" "$N/args.txt" | grep '^[<>]' | sed -E 's/^(> extract-only args: --targets-rule R-T1,R-T2 --object-access)( --semantics \S+)+$/\1 SEMFILES/' | tr '\n' ';')
      [ "$d" = "< extract-only args: --targets-rule R-T1,R-T2 --object-access;> extract-only args: --targets-rule R-T1,R-T2 --object-access SEMFILES;" ] && ok "$b args.txt: only the extract-only line (--semantics)" || bad "$b args.txt ($d)"
      cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
      [ -s "$N/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty"
      rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" ;;
    onextract_on)
      E=$O/sem_on_order/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      except_line 2 "$O/objacc_on_order/$b/order.txt" "$E/order.txt" && ok "$b order.txt = objacc_on_order's apart from line 2" || bad "$b order.txt differs from objacc_on_order's beyond line 2"
      cmp -s "$O/objacc_on_order/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv = objacc_on_order's" || bad "$b edges.tsv differs from objacc_on_order's"
      cmp -s "$E/relations.txt" "$O/sem_on_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the on re-extraction's" || bad "$b relations.txt differs from the on re-extraction's"
      except_line 2 "$O/objacc_on_order/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = objacc_on_order's apart from line 2" || bad "$b keys.txt differs from objacc_on_order's beyond line 2"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      rm -rf "$E/again" ;;
    plans_on)
      python3 -I "$HERE/compare_plans.py" extract "$O/objacc_plans_on/$b" "$O/sem_plans_on/$b" | sed "s/^/$b extract-on /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    measure)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      "$KS" show "$O/sem_on_extract/$b/$b/NP/facts.kz" > "$SCRATCH/view_$b.txt" || bad "$b show"
      python3 -I "$HERE/measure_sem.py" "$SCRATCH/view_$b.txt" "$O/sem_on_order/$b/relations.txt" "$O/sem_on_order/$b/keys.txt" "$O/sem_plans_on/$b/plan.kzs" --key 17975444d83f6589 > "$C/measure_$b.txt" && ok "$b measured (compare/measure_$b.txt: $(head -1 "$C/measure_$b.txt"))" || bad "$b measure_sem"
      grep '^  [a-zP0-9-]* agreement ' "$SCRATCH/view_$b.txt" | sed -E 's/ decl [^ ]+$//' > "$C/agreement_$b.txt"
      rm -f "$SCRATCH/view_$b.txt" ;;
    extract_on_text)
      # results 8.0b: the two checks of extract_on that the declaration 8.0 wrote too narrowly, again with the allowance
      # the addendum declares (the header line sections= one more; sections compared by their record text)
      N=$O/sem_on_extract/$b/$b/NP; P=$O/objacc_on_extract/$b/$b/NP
      # TEXT_ROUND (results 8.0c): unset, the first run (section_text_sha.py); _2, the second (section_text_sha_2.py, the
      # record lines only), into names with that suffix
      python3 -I "$HERE/check_head_sem.py" "$KD" "$P/facts.kz" "$N/facts.kz" --allow-size --allow-new-section > "$C/on_facts_head_sem${TR}_$b.txt"; [ $? = 0 ] && ok "$b facts.kz header: $(head -1 "$C/on_facts_head_sem${TR}_$b.txt")" || bad "$b facts.kz header (compare/on_facts_head_sem${TR}_$b.txt)"
      python3 -I "$HERE/section_text_sha${TR}.py" "$KD" "$P/facts.kz" >> "$HERE/section_text_old${TR}.txt.partial" && python3 -I "$HERE/section_text_sha${TR}.py" "$KD" "$N/facts.kz" >> "$HERE/section_text_new${TR}.txt.partial" || bad "$b section record text sha256" ;;
    *) echo "usage: compare.sh relations|order|extract|onextract|plans|extract_on|extract_on_text|onextract_on|plans_on|measure" >&2; exit 64 ;;
  esac
done
if [ "${1:-}" = extract_on ]; then
  mv "$HERE/section_sha_on.txt.partial" "$HERE/section_sha_on.txt"
  python3 -I "$HERE/compare_sections.py" "$PREV/section_sha_on.txt" "$HERE/section_sha_on.txt" > "$C/sections_on.txt"; [ $? = 0 ] && ok "sections: $(grep -c '^OK' "$C/sections_on.txt") builds, every section of the earlier on-round the same but STRINGS and BASIS, SEMANTICS new (compare/sections_on.txt)" || bad "sections (compare/sections_on.txt)"
fi
if [ "${1:-}" = extract_on_text ]; then
  mv "$HERE/section_text_old${TR}.txt.partial" "$HERE/section_text_old${TR}.txt"; mv "$HERE/section_text_new${TR}.txt.partial" "$HERE/section_text_new${TR}.txt"
  python3 -I "$HERE/compare_sections_text.py" "$HERE/section_text_old${TR}.txt" "$HERE/section_text_new${TR}.txt" > "$C/sections_text_on${TR}.txt"; [ $? = 0 ] && ok "sections by record text: $(grep -c '^OK' "$C/sections_text_on${TR}.txt") builds, every section of the earlier on-round the same record text but STRINGS and BASIS, SEMANTICS new (compare/sections_text_on${TR}.txt)" || bad "sections by record text (compare/sections_text_on${TR}.txt)"
fi
exit $rc

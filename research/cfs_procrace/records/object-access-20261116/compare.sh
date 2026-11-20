#!/usr/bin/env bash
# compare.sh STEP -- the comparisons of the declaration of the types and object access build
# (plan/full_stage_results_20261116.md 7.0), one step at a time.  Option off against the review-fix outputs (the same
# bytes apart from the tool identity), option on against option off (classes of compare_on.py, name_stability.py) and
# measured downstream (measure_on.py):
#   relations   objacc_relcheck/<b> against revfix_relcheck: relations.txt and keys.txt the same bytes; relations.log empty
#   order       objacc_order/<b>: two runs the same bytes; order.txt and edges.tsv = revfix_order's; order.log empty
#   extract     objacc_off_extract/<b>/<b>/NP against revfix_extract: basis.kz record by record (check_head.py --whole),
#               facts.kz record by record (check_whole.py: tool hashes, STRINGS crc32, extract.units_from basis hash), the
#               facts views only the extract.units_from line, extract.txt apart from line 1, relations.txt apart from
#               line 2, basis.txt and sha256.txt only the hash lines, args.txt and relations.log the same
#   onextract   objacc_off_order/<b>: two runs the same; order.txt against revfix_on_extract apart from line 2, edges.tsv
#               the same; relations.txt = the re-extraction's; keys.txt = revfix_on_extract's apart from line 2
#   plans       compare_plans.py t8cf revfix_plans objacc_plans; compare_plans.py extract revfix_plans_on_extract
#               objacc_plans_off
#   extract_on  objacc_on_extract/<b>/<b>/NP against objacc_off_extract: basis.kz and basis.txt the same bytes; the facts
#               views with compare_on.py (compare/on_<b>.txt); args.txt only the extract-only line with --object-access;
#               sha256.txt only the facts.kz line; relations.log empty; name_stability.py on the views of c1_pre_tx and
#               c1_post_tx (compare/names_c1.txt); per-section sha256 of both rounds (section_sha_<round>.txt); the
#               extract.txt and relations.txt differences counted (compare/extract_txt_on_<b>.txt)
#   onextract_on objacc_on_order/<b>: two runs the same bytes; relations.txt = the on re-extraction's; order.log and
#               relations.log empty; measure_on.py objacc_off_order objacc_on_order with the plans (compare/measure_<b>.txt)
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
if [ "${1:-}" = extract_on ]; then
  : > "$HERE/section_sha_off.txt.partial"; : > "$HERE/section_sha_on.txt.partial"
fi
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  case "${1:-}" in
    relations)
      for f in relations.txt keys.txt; do cmp -s "$O/revfix_relcheck/$b/$f" "$O/objacc_relcheck/$b/$f" && ok "$b $f = revfix_relcheck's" || bad "$b $f differs from revfix_relcheck's"; done
      [ -s "$O/objacc_relcheck/$b/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty" ;;
    order)
      for f in order.txt edges.tsv; do cmp -s "$O/objacc_order/$b/$f" "$O/objacc_order/$b/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      for f in order.txt edges.tsv; do cmp -s "$O/revfix_order/$b/$f" "$O/objacc_order/$b/$f" && ok "$b $f = revfix_order's" || bad "$b $f differs from revfix_order's"; done
      [ -s "$O/objacc_order/$b/order.log" ] && bad "$b order.log not empty" || ok "$b order.log empty"
      rm -r "$O/objacc_order/$b/again" ;;
    extract)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/objacc_off_extract/$b/$b/NP; P=$O/revfix_extract/$b/$b/NP
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
      E=$O/objacc_off_order/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      except_line 2 "$O/revfix_on_extract/$b/order.txt" "$E/order.txt" && ok "$b order.txt = revfix_on_extract's apart from line 2" || bad "$b order.txt differs from revfix_on_extract's beyond line 2"
      cmp -s "$O/revfix_on_extract/$b/edges.tsv" "$E/edges.tsv" && ok "$b edges.tsv = revfix_on_extract's" || bad "$b edges.tsv differs from revfix_on_extract's"
      cmp -s "$E/relations.txt" "$O/objacc_off_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the re-extraction's" || bad "$b relations.txt differs from the re-extraction's"
      except_line 2 "$O/revfix_on_extract/$b/keys.txt" "$E/keys.txt" && ok "$b keys.txt = revfix_on_extract's apart from line 2" || bad "$b keys.txt differs from revfix_on_extract's beyond line 2"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      rm -rf "$E/again" ;;
    plans)
      python3 -I "$HERE/compare_plans.py" t8cf "$O/revfix_plans/$b" "$O/objacc_plans/$b" | sed "s/^/$b t8cf /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1
      python3 -I "$HERE/compare_plans.py" extract "$O/revfix_plans_on_extract/$b" "$O/objacc_plans_off/$b" | sed "s/^/$b extract /"; [ ${PIPESTATUS[0]} = 0 ] || rc=1 ;;
    extract_on)
      SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
      N=$O/objacc_on_extract/$b/$b/NP; P=$O/objacc_off_extract/$b/$b/NP
      for f in basis.kz basis.txt relations.log; do cmp -s "$P/$f" "$N/$f" && ok "$b $f = the option-off round's" || bad "$b $f differs from the option-off round's"; done
      [ -s "$N/relations.log" ] && bad "$b relations.log not empty" || ok "$b relations.log empty"
      d=$(diff "$P/args.txt" "$N/args.txt" | grep '^[<>]' | tr '\n' ';')
      [ "$d" = "< extract-only args: --targets-rule R-T1,R-T2;> extract-only args: --targets-rule R-T1,R-T2 --object-access;" ] && ok "$b args.txt: only the extract-only line (--object-access)" || bad "$b args.txt ($d)"
      cmp -s <(sed -E 's/^[0-9a-f]{64}  facts\.kz$/H  facts.kz/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  facts\.kz$/H  facts.kz/' "$N/sha256.txt") && ok "$b sha256.txt: only the facts.kz line" || bad "$b sha256.txt"
      "$KS" show "$P/facts.kz" > "$SCRATCH/off_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/on_$b.txt" || bad "$b show"
      python3 -I "$HERE/compare_on.py" "$SCRATCH/off_$b.txt" "$SCRATCH/on_$b.txt" --build "$b" > "$C/on_$b.txt"; [ $? = 0 ] && ok "$b facts views: $(head -1 "$C/on_$b.txt")" || bad "$b facts views (compare/on_$b.txt)"
      { echo "# extract.txt: lines only off / only on"; diff "$P/extract.txt" "$N/extract.txt" | grep -c '^<'; diff "$P/extract.txt" "$N/extract.txt" | grep -c '^>'
        echo "# relations.txt: lines only off / only on"; diff "$P/relations.txt" "$N/relations.txt" | grep -c '^<'; diff "$P/relations.txt" "$N/relations.txt" | grep -c '^>'; } > "$C/extract_txt_on_$b.txt"
      ok "$b extract.txt and relations.txt differences counted (compare/extract_txt_on_$b.txt)"
      python3 -I "$HERE/section_sha.py" "$KD" "$P/facts.kz" >> "$HERE/section_sha_off.txt.partial" && python3 -I "$HERE/section_sha.py" "$KD" "$N/facts.kz" >> "$HERE/section_sha_on.txt.partial" || bad "$b section sha256"
      if [ "$b" = c1_post_tx ]; then
        python3 -I "$HERE/name_stability.py" "$SCRATCH/on_c1_pre_tx.txt" "$SCRATCH/on_c1_post_tx.txt" > "$C/names_c1.txt"; [ $? = 0 ] && ok "c1 names: $(tail -1 "$C/names_c1.txt") ($(head -1 "$C/names_c1.txt"))" || bad "c1 names (compare/names_c1.txt)"
        python3 -I "$HERE/name_stability.py" "$SCRATCH/off_c1_pre_tx.txt" "$SCRATCH/off_c1_post_tx.txt" > "$C/names_c1_off.txt"; ok "c1 names without the option, for the difference (compare/names_c1_off.txt: $(head -1 "$C/names_c1_off.txt"))"
        rm -f "$SCRATCH"/off_c1_pre_tx.txt "$SCRATCH"/on_c1_pre_tx.txt "$SCRATCH"/off_c1_post_tx.txt "$SCRATCH"/on_c1_post_tx.txt
      fi
      [ "$b" = c1_pre_tx ] || rm -f "$SCRATCH/off_$b.txt" "$SCRATCH/on_$b.txt" ;;
    onextract_on)
      E=$O/objacc_on_order/$b
      for f in order.txt edges.tsv; do cmp -s "$E/$f" "$E/again/$f" && ok "$b $f two runs same" || bad "$b $f two runs differ"; done
      cmp -s "$E/relations.txt" "$O/objacc_on_extract/$b/$b/NP/relations.txt" && ok "$b relations.txt = the on re-extraction's" || bad "$b relations.txt differs from the on re-extraction's"
      for f in order.log relations.log; do [ -s "$E/$f" ] && bad "$b $f not empty" || ok "$b $f empty"; done
      python3 -I "$HERE/measure_on.py" "$O/objacc_off_order/$b" "$E" --plans "$O/objacc_plans_off/$b" "$O/objacc_plans_on/$b" > "$C/measure_$b.txt" && ok "$b measured (compare/measure_$b.txt: $(head -1 "$C/measure_$b.txt"))" || bad "$b measure_on"
      rm -rf "$E/again" ;;
    *) echo "usage: compare.sh relations|order|extract|onextract|plans|extract_on|onextract_on" >&2; exit 64 ;;
  esac
done
if [ "${1:-}" = extract_on ]; then mv "$HERE/section_sha_off.txt.partial" "$HERE/section_sha_off.txt"; mv "$HERE/section_sha_on.txt.partial" "$HERE/section_sha_on.txt"; fi
exit $rc

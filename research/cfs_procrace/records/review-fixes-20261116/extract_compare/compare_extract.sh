#!/usr/bin/env bash
# compare_extract.sh -- the re-run of the extractor-fix step's re-extraction comparison (records/extract-fixes-20261116/
# compare.sh extract, kept as it was with its record compare_extract.txt and compare/) after review, into this directory:
# the same checks with compare_classes.py revision 2 (this directory: masks counted under the masks that act on a line;
# an old "<anonymous>" line paired with its successors over the section, unpaired ones undeclared), facts_allow.tsv with
# the declared fields of the pairs, relations_allow.tsv with the [C] head pattern tightened; extract.txt with extract_txt_allow.tsv
# (the step's rows and the pairs of its location lines, addendum 6.0 (가-1)).  The other checks (basis.kz, facts.kz header, basis.txt, sha256.txt, args.txt, relations.log)
# are re-run unchanged with the step's check_head.py.  Facts views by the kz-static that made extfix_extract (c6aa4811,
# checked first), written to SCRATCH and removed after.  No output directory is written.
set -uo pipefail
O=/home/user/work/redesign/obs1
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
KD=/home/user/work/merge-check/kz-llvm/tools/kz-dump/kz-dump
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OLDREC="$(cd "$HERE/../../extract-fixes-20261116" && pwd)"
SCRATCH=${SCRATCH:?SCRATCH: a scratch directory for the facts views}
WANT=c6aa4811612f13bd79b826e53e1f104e3e759d4d97123029c232558a98c73507
rc=0
ok() { echo "OK $*"; }
bad() { echo "BAD $*"; rc=1; }
got=$(sha256sum "$KS" | cut -d' ' -f1)
[ "$got" = "$WANT" ] && ok "kz-static $got (the tool of extfix_extract)" || { echo "BAD kz-static $got is not $WANT"; exit 1; }
cls() { python3 -I "$HERE/compare_classes.py" "$@"; }
C=$HERE/compare
mkdir -p "$C"
for b in c1_pre_tx c1_post_tx r2_core c5_641 c5_642; do
  N=$O/extfix_extract/$b/$b/NP; P=$O/schema12_extract/$b/$b/NP
  python3 -I "$OLDREC/check_head.py" "$KD" "$P/basis.kz" "$N/basis.kz" --whole > "$C/basis_$b.txt"; [ $? = 0 ] && ok "$b basis.kz: $(head -1 "$C/basis_$b.txt")" || bad "$b basis.kz (compare/basis_$b.txt)"
  python3 -I "$OLDREC/check_head.py" "$KD" "$P/facts.kz" "$N/facts.kz" --allow-size > "$C/facts_head_$b.txt"; [ $? = 0 ] && ok "$b facts.kz: $(head -1 "$C/facts_head_$b.txt")" || bad "$b facts.kz header (compare/facts_head_$b.txt)"
  "$KS" show "$P/facts.kz" > "$SCRATCH/old_$b.txt" && "$KS" show "$N/facts.kz" > "$SCRATCH/new_$b.txt" || bad "$b show"
  cls "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt" --allow "$HERE/facts_allow.tsv" --build "$b" --list 40 > "$C/facts_$b.txt"; [ $? = 0 ] && ok "$b facts view: $(tail -1 "$C/facts_$b.txt")" || bad "$b facts view (compare/facts_$b.txt)"
  rm -f "$SCRATCH/old_$b.txt" "$SCRATCH/new_$b.txt"
  cls "$P/extract.txt" "$N/extract.txt" --allow "$HERE/extract_txt_allow.tsv" --build "$b" > "$C/extract_txt_$b.txt"; [ $? = 0 ] && ok "$b extract.txt: $(tail -1 "$C/extract_txt_$b.txt")" || bad "$b extract.txt (compare/extract_txt_$b.txt)"
  cls "$P/relations.txt" "$N/relations.txt" --allow "$HERE/relations_allow.tsv" --build "$b" > "$C/relations_$b.txt"; [ $? = 0 ] && ok "$b relations.txt: $(tail -1 "$C/relations_$b.txt")" || bad "$b relations.txt (compare/relations_$b.txt)"
  cmp -s <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$P/basis.txt") <(sed -E 's/^basis [0-9a-f]{64}$/basis H/' "$N/basis.txt") && ok "$b basis.txt: only the basis hash line" || bad "$b basis.txt"
  cmp -s <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$P/sha256.txt") <(sed -E 's/^[0-9a-f]{64}  (basis\.kz|facts\.kz|\S+kz-static)$/H  \1/' "$N/sha256.txt") && ok "$b sha256.txt: only the basis.kz, facts.kz, kz-static hashes" || bad "$b sha256.txt"
  cmp -s "$P/args.txt" "$N/args.txt" && ok "$b args.txt same" || bad "$b args.txt differs"
  cmp -s "$P/relations.log" "$N/relations.log" && ok "$b relations.log same" || bad "$b relations.log differs"
done
exit $rc

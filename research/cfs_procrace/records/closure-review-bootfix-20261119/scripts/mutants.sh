#!/usr/bin/env bash
# mutants.sh SCRATCH OUT -- the tests of run_cycle.sh against mutants of the committed run_cycle.sh (results 26.9): each
# mutant takes out one fix of protocol 14.14 (or the two halves fix-judge-05 names); the unchanged test_run_cycle.sh must
# fail on each.  Copies the committed run_cycle.sh, test_run_cycle.sh, space_guard.sh and ledger_check.sh into
# SCRATCH/<mutant>/ (the layout the test reads), edits one line with sed, and runs the test there.  One line per mutant.
# (The first run, out/mutants.trial1.txt, had a pin mutant that also refused every later boot and a keep mutant that
# removed the ledger; both were replaced by one-line edits that take out only the check.)
set -uo pipefail
S=${1:?scratch}; OUT=${2:?out}
H="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; RS="$(cd "$H/../../.." && pwd)"
BOOT=$RS/records/closure-c1prep-20261119/boot
[ ! -e "$OUT" ] || { echo "mutants.sh: $OUT exists" >&2; exit 64; }
mut() { # NAME SED-EXPRESSION
  local d=$S/$1 b; b=$d/research/cfs_procrace/records/closure-c1prep-20261119/boot
  mkdir -p "$b" "$d/research/cfs_procrace/env/tools" || return 2
  cp "$BOOT/test_run_cycle.sh" "$b/"; cp "$RS/env/tools/space_guard.sh" "$RS/env/tools/ledger_check.sh" "$d/research/cfs_procrace/env/tools/"
  sed -e "$2" "$BOOT/run_cycle.sh" > "$b/run_cycle.sh"
  if cmp -s "$BOOT/run_cycle.sh" "$b/run_cycle.sh"; then echo "$1: the edit changed nothing (not a mutant)" >> "$OUT"; rm -rf "$d"; return 0; fi
  bash "$b/test_run_cycle.sh" "$d/w" > "$d/test.out" 2>&1; rc=$?
  echo "$1: test rc $rc; $(tail -1 "$d/test.out"); failing: $(grep '^FAIL' "$d/test.out" | sed 's/ (.*//; s/^FAIL //' | tr '\n' '|')" >> "$OUT"
  rm -rf "$d"
}
mut git_check_declaration_only 's#ls-files --error-unmatch "\$DECL" "\$HERE/run_cycle.sh"#ls-files --error-unmatch "$DECL"#; s#status --porcelain -- "\$DECL" "\$HERE/run_cycle.sh"#status --porcelain -- "$DECL"#'
mut count_missing_as_zero 's#else got=missing; fi$#else got=0; fi#'
mut count_regular_files_only 's#find "\$f" ! -type d | wc -l#find "$f" -type f | wc -l#'
mut gate_no_refusal 's#^    \[ ! -e "\$L/gate.txt" \] || refuse_exists "\$L/gate.txt"$#    :#'
mut control_no_refusal 's#^    \[ ! -e "\$L/control_check.txt" \] || refuse_exists "\$L/control_check.txt"$#    :#'
mut no_space_check 's#^    space_check || {#    true || {#'
mut no_pin_check 's#^    if \[ -f "\$PIN" \]; then need_pin$#    if [ -f "$PIN" ]; then true#'
mut not_booted_rule_off 's#if \[ \$rc -ne 0 \] && \[ ! -e "\$RUN" \]; then#if false; then#'
mut eval_into_record_dir 's#--out "\$EV/eval.txt"#--out "$L/eval.txt"#'
mut eval_sheet_accepted 's#^    \[ ! -e "\$L/sheet_boot.tsv" \] || {#    true || {#'
mut explain_no_reverify 's#^    reverify "\$L/verify_at_explain.txt" "\$N" \\$#    true \\#'
mut verify_at_boot_overwritten 's#^  keep_old "\$1" || return 1$#  :#'
mut convert_refusal_keeps_dir 's#^    if \[ \$rc -eq 75 \] && \[ ! -e "\$CV/run.kzt" \]; then#    if false; then#'
mut keep_no_ledger_check 's#bash "\$R/env/tools/ledger_check.sh" "\$PR" > "\$L/ledger_check_keep.txt"; lc=\$?#echo skipped > "$L/ledger_check_keep.txt"; lc=0#'
echo "mutants done: $(grep -c ': test rc 1;' "$OUT") of $(grep -c . "$OUT") fail the test" >> "$OUT"
cat "$OUT"

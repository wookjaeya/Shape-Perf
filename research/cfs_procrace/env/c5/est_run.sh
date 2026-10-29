#!/usr/bin/env bash
# est_run.sh <V> <W> <kz-static> [instr] -- E-ST on the cFE 6.4.<V> reference build that derive_c5.sh build <V> <W> keep
# made (with `instr`: on the derived build of derive_c5.sh build <V> <W> instr, the B' basis, into <W>/est_instr; no controls)
# (plan/c5_execution_basis_20261028.md section 10.5; PI decision D7 of PI_DECISIONS_RESOLVED_20261029.md section 3a).
# Tool check: no research condition, no case, no run of the target.
#   1  the inputs (est_inputs.sh: compile database and link command files from the build log)
#   2  kz-static basis with kairos/profiles/c5-cfe-6.4.x/basis/core-linux.kzb and the header profile est-64<V>; then
#      kz-static extract on that basis
#   3  the facts D7 asks for: the pthread_create call of OS_TaskCreate (osapi.c:645-648, the diagnostic at 647) and the
#      signal call of cfe_psp_start.c:270, as call facts
#   4  controls with the recipe's front_end_diagnostic statements left out (copies of the recipe under <W>/est/ctl):
#      none -> the extraction stops at osapi.c; only the osapi.c statement -> it stops at cfe_psp_start.c
# Writes <W>/est/est.record.
set -uo pipefail
V=${1:?usage: est_run.sh <V> <W> <kz-static> [instr]}; W=${2:?work dir}; K=${3:?kz-static}; VAR=${4:-keep}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
P="$(cd "$HERE/../../kairos/profiles/c5-cfe-6.4.x/basis" && pwd)"
case "$VAR" in
keep) T="$W/keep/cFE-6.4.$V-OSS-release"; E="$W/est" ;;
instr) T="$W/b/cFE-6.4.$V-OSS-release"; E="$W/est_instr" ;;
*) echo "variant is keep|instr" >&2; exit 64 ;;
esac
REC="$E/est.record"
[ -e "$E/compile_commands.json" ] || "$HERE/est_inputs.sh" "$W/build_$VAR.log" "$T" "$E" > /dev/null || exit 1
log() { echo "$*" | tee -a "$REC"; }
: > "$REC"
log "# est_run.sh $V $(date -u +%FT%TZ): tree $T; kz-static sha256 $(sha256sum "$K" | cut -c1-64) ($("$K" --identity 2>&1 | head -1))"
log "recipe core-linux.kzb sha256 $(sha256sum "$P/core-linux.kzb" | cut -c1-64); profile est-64$V/00_profile.kzp sha256 $(sha256sum "$P/est-64$V/00_profile.kzp" | cut -c1-64)"
sed 's/^/inputs: /' "$E/est_inputs.record" | head -3 >> "$REC"
basis() { # <recipe> <out>
  "$K" basis --profile "$P/est-64$V" --compile-db "$E/compile_commands.json" --build-dir "$T/build/cpu1" --bundle-root "$T" --recipe "$1" --text -o "$2"
}
basis "$P/core-linux.kzb" "$E/basis.kz" > "$E/basis.txt" 2>&1
log "basis rc=$?: $(sed -n 's/^  translation units *//p' "$E/basis.txt") units; $(grep -c '^  warning' "$E/basis.txt") warnings"
grep -E '^(link\.(command|members_in_program|relocatable|members_without_command)|units\.count|front_end\.unit_diagnostic|const\.|startup\.(file|script|entries)|target\.elf|binary\.type|closure\.(modules|duplicate_definitions|name_binding)|instrumentation\.)' "$E/basis.txt" | sed 's/^/  /' >> "$REC"
"$K" extract --profile "$P/est-64$V" --basis "$E/basis.kz" -o "$E/facts.kz" > "$E/extract.txt" 2>&1
log "extract rc=$?: $(sed -n 's/^  translation units *//p' "$E/extract.txt") units, functions $(sed -n 's/^  functions *//p' "$E/extract.txt"), call sites $(sed -n 's/^  call sites *//p' "$E/extract.txt")"
for c in pthread_create signal; do
  "$K" show "$E/facts.kz" --section calls --callee "$c" > "$E/calls_$c.txt" 2>&1
  log "call facts --callee $c: $(grep -c '^  site ' "$E/calls_$c.txt")"
  grep -E '^  site |^      arg ' "$E/calls_$c.txt" | sed 's/^/  /' >> "$REC"
done
# the deviation records of the two units (kz-dump of the same build tree as kz-static)
"$(dirname "$K")/../kz-dump/kz-dump" "$E/basis.kz" > "$E/basis.dump" 2>&1
log "deviation records (DEVIATIONS) of a front-end diagnostic: $(grep -c '^  DEVIATIONS\[[0-9]*\] subject="front-end diagnostic ' "$E/basis.dump")"
grep '^  DEVIATIONS\[[0-9]*\] subject="front-end diagnostic ' "$E/basis.dump" | sed "s#$T/##g; s/^/  /" | cut -c1-400 >> "$REC"
[ "$VAR" = keep ] || exit 0
# controls
mkdir -p "$E/ctl"
grep -v '^front_end_diagnostic ' "$P/core-linux.kzb" > "$E/ctl/none.kzb"
grep -v '^front_end_diagnostic "psp/' "$P/core-linux.kzb" > "$E/ctl/osapi_only.kzb"
for c in none osapi_only; do
  basis "$E/ctl/$c.kzb" "$E/ctl/$c.basis.kz" > "$E/ctl/$c.basis.txt" 2>&1; b=$?
  "$K" extract --profile "$P/est-64$V" --basis "$E/ctl/$c.basis.kz" -o "$E/ctl/$c.facts.kz" > "$E/ctl/$c.extract.txt" 2>&1; x=$?
  log "control $c: basis rc=$b, extract rc=$x: $(grep -m1 'cannot parse' "$E/ctl/$c.extract.txt" | sed "s#$T/##")"
  grep -E '^  /.*:[0-9]+:[0-9]+: ' "$E/ctl/$c.extract.txt" | sed "s#$T/##; s/^ */  /" >> "$REC"
done
exit 0

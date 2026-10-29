#!/usr/bin/env bash
# est_probe.sh <compile_commands.json> <bundle root> <clang> <out> -- which units of an E-ST compile database a Clang
# front end rejects, and with which diagnostics (plan/c5_execution_basis_20261028.md section 10.5; PI decision D7 of
# PI_DECISIONS_RESOLVED_20261029.md section 3a: "if another translation unit fails, do not widen the list, report it").
# Every unit is parsed with its recorded command as kz-static gives it to the front end (lib/llvm/ast_run.cpp
# front_end_command and flag_list: without the compiler, -o <file>, -c and the compiler plugin options; the arguments
# every unit gets: -Wno-unknown-warning-option -Wno-error -w), plus -fsyntax-only and -ferror-limit=0, in the command's
# directory.  Nothing of the recipe is applied: the probe shows the errors D7 is about, before any lowering.
# Writes <out>: per unit "unit <path> rc=<n> errors=<n>" and its error lines (paths relative to the bundle root), then
# "# summary ..." lines.  Tool check only: no research condition, no run of the target.
set -uo pipefail
DB=${1:?usage: est_probe.sh <compile_commands.json> <bundle root> <clang> <out>}; T=${2:?bundle root}; CL=${3:?clang}; O=${4:?out}
T=${T%/}
: > "$O"
echo "# est_probe.sh $(date -u +%FT%TZ): $DB; clang $CL: $("$CL" --version | head -1)" >> "$O"
n=0; bad=0; errs=0
while IFS=$'\t' read -r dir file cmd; do
  read -ra a <<< "$cmd"   # the recorded commands of est_inputs.sh have no quoted arguments
  args=(); skip=0
  for ((i = 1; i < ${#a[@]}; i++)); do
    if [ $skip = 1 ]; then skip=0; continue; fi
    case "${a[$i]}" in -o) skip=1; continue ;; -c) continue ;; -fplugin=*|-fplugin-arg-*) continue ;; esac
    args+=("${a[$i]}")
  done
  out=$(cd "$dir" && "$CL" "${args[@]}" -fsyntax-only -Wno-unknown-warning-option -Wno-error -w -ferror-limit=0 2>&1); rc=$?
  e=$(printf '%s\n' "$out" | grep -c ': error: ')
  n=$((n + 1)); errs=$((errs + e)); [ "$rc" != 0 ] && bad=$((bad + 1))
  echo "unit ${file#"$T"/} rc=$rc errors=$e" >> "$O"
  printf '%s\n' "$out" | grep ': error: ' | sed "s#$T/##g; s#^#  #" >> "$O"
done < <(jq -r '.[] | [.directory, .file, .command] | @tsv' "$DB")
echo "# summary: units $n, units rejected $bad, errors $errs" >> "$O"
echo "# summary by diagnostic: $(grep -o '\[-W[a-z0-9-]*\]$' "$O" | sort | uniq -c | sed 's/^ *//' | tr '\n' ';')" >> "$O"

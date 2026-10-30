#!/usr/bin/env bash
# cmp_compile_commands.sh <reference compile_commands.json> <derived compile_commands.json> <sed script> [<flag to drop>...]
# Compares the compile commands of a derived build with those of the reference build as sorted multisets of
# normalized command lines: the compiler path (first word) is dropped, the reference paths are rewritten to the
# derived ones by the sed script (e.g. "s#<ref root>#<derived root>#g;..."), and the given flags (translations, plugin options) are dropped.  Prints the differing lines and
# a count; exit 0 = the same multiset.  Table object directories carry a hash of their path (cFE add_cfe_tables)
# that changes with the tree path (D1); it is replaced by "H" on both sides.  Shell and awk only (CMake writes one "command" line per entry).
set -uo pipefail
REFJ=${1:?usage}; NEWJ=${2:?usage}; SEDS=${3:?usage}; shift 3
DROP=" $* "
norm() { # <json> <sed script or ->
  grep -E '^ *"command": ' "$1" | sed -e 's/^ *"command": "//' -e 's/",\{0,1\}$//' -e 's/\\"/"/g' |
    { if [ "$2" != - ]; then sed "$2"; else cat; fi; } | sed -E 's/\.tbl[0-9a-f]{40}/.tblH/g' |
    awk -v drop="$DROP" '{ out = ""; skip = 0
      for (i = 2; i <= NF; i++) {
        if (skip) { skip = 0; continue }
        if ($i == "-mllvm" && index(drop, " -mllvm ") > 0) { skip = 1; continue }
        if (index(drop, " " $i " ") > 0) continue
        if ($i ~ /^-fpass-plugin=/ && index(drop, " -fpass-plugin ") > 0) continue
        out = out " " $i }
      print substr(out, 2) }' | sort
}
T=$(mktemp)
diff <(norm "$REFJ" "$SEDS") <(norm "$NEWJ" -) > "$T"; rc=$?
cat "$T"; echo "compile commands: reference $(grep -cE '^ *"command": ' "$REFJ"), derived $(grep -cE '^ *"command": ' "$NEWJ"), differing lines $(grep -c '^[<>]' "$T")"
rm -f "$T"
exit $rc

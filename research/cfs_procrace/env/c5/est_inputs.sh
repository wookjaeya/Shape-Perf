#!/usr/bin/env bash
# est_inputs.sh <build log> <tree> <out dir> -- the inputs of kz-static basis for a cFE 6.4.x build made with classic make
# (plan/c5_execution_basis_20261028.md section 10.5 items 5 and 9; input preparation, no engine change).  The build
# records its commands only in its log (`make` echoes them; no compile_commands.json, no CMake link.txt).  From the
# `## make` part of a log of instrument/gcc/scripts/derive_c5.sh (the `make config` part builds the host tool elf2cfetbl,
# which is not part of the target):
#   <out dir>/compile_commands.json   one entry per compile line `gcc ... -c -o <obj> <src>`: directory = the directory
#                                     make was in (its "Entering directory" line), command = the line as the log has it
#                                     (one space between tokens, as make echoes it), file = <src>, output = <obj>
#   <dir>/<output>.link.txt           one file per link command (`ld ... -r` of a subsystem object, the `gcc -o
#                                     core-linux.bin` of the core, the `gcc -m32 -shared -o <app>.so` of each app),
#                                     the line as the log has it, written into the directory where the command ran:
#                                     kz-static takes a link command file's directory as the command's working
#                                     directory (lib/llvm/basis.cpp program(), the case without CMakeFiles)
#   <out dir>/est_inputs.record       the log's sha256, every file written with its source line number and sha256
# Refuses a log in which a command line is not one of these shapes, or a link file that exists.
set -uo pipefail
LOG=${1:?usage: est_inputs.sh <build log> <tree> <out dir>}; T=${2:?tree}; O=${3:?out dir}
T=$(cd "$T" && pwd)
mkdir -p "$O"
REC="$O/est_inputs.record"
TSV="$O/.commands.tsv"
start=$(grep -n '^## make$' "$LOG" | head -1 | cut -d: -f1)
[ -n "$start" ] || { echo "no '## make' line in $LOG" >&2; exit 2; }
# <line number> TAB <kind> TAB <directory> TAB <output> TAB <source, "-" for a link> TAB <command>
awk -v start="$start" -F'\n' '
  NR < start { next }
  /^make(\[[0-9]+\])?: Entering directory / { d = $0; sub(/^[^'"'"']*'"'"'/, "", d); sub(/'"'"'.*$/, "", d); dirs[++n] = d; next }
  /^make(\[[0-9]+\])?: Leaving directory / { n--; next }
  /^(gcc|ld) / {
    line = $0; sub(/ +$/, "", line)
    k = split(line, t, / +/); out = ""; src = ""; kind = ""
    for (i = 1; i <= k; i++) if (t[i] == "-o" && i < k) out = t[i + 1]
    if (line ~ / -c / && t[k] ~ /\.c$/) { kind = "compile"; src = t[k] }
    else if (t[1] == "ld" && line ~ / -r( |$)/) kind = "relocatable"
    else if (t[1] == "gcc" && line ~ / -shared /) kind = "module"
    else if (t[1] == "gcc" && line !~ / -c /) kind = "executable"
    else kind = "UNKNOWN"
    printf "%d\t%s\t%s\t%s\t%s\t%s\n", NR, kind, dirs[n], out, src == "" ? "-" : src, line  # "-": no field is empty (read)
  }' "$LOG" > "$TSV"
if awk -F'\t' '$2 == "UNKNOWN" || $3 == "" || $4 == ""' "$TSV" | grep -q .; then
  echo "unrecognised command lines (or no directory / output):" >&2; awk -F'\t' '$2 == "UNKNOWN" || $3 == "" || $4 == ""' "$TSV" >&2; exit 2
fi
{
  echo "# est_inputs.sh $(date -u +%FT%TZ): log $LOG sha256 $(sha256sum "$LOG" | cut -c1-64), part from line $start (## make)"
  echo "commands: $(awk -F'\t' '{print $2}' "$TSV" | sort | uniq -c | paste -sd';')"
} > "$REC"
# the compile database
awk -F'\t' '$2 == "compile"' "$TSV" | jq -R -s 'split("\n") | map(select(length > 0) | split("\t")) |
  map({directory: .[2], command: .[5], file: .[4], output: .[3]})' > "$O/compile_commands.json"
echo "compile_commands.json: $(jq length "$O/compile_commands.json") entries, sha256 $(sha256sum "$O/compile_commands.json" | cut -c1-64)" >> "$REC"
# the link command files
while IFS=$'\t' read -r ln kind dir out src cmd; do
  [ "$kind" = compile ] && continue
  case "$dir" in "$T"/*) ;; *) echo "link command outside the tree: $dir" >&2; exit 2 ;; esac
  f="$dir/$(basename "$out").link.txt"
  [ ! -e "$f" ] || { echo "refusing: $f exists" >&2; exit 2; }
  printf '%s\n' "$cmd" > "$f"
  echo "$kind ${f#$T/} (log line $ln) sha256 $(sha256sum "$f" | cut -c1-64)" >> "$REC"
done < "$TSV"
rm -f "$TSV"
cat "$REC"

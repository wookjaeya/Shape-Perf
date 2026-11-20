#!/usr/bin/env bash
# cmp_link_commands.sh <reference arch build dir> <derived arch build dir> <sed script> [<flag to drop>...]
# Compares every link command (CMakeFiles/*/link.txt) of a derived arch build with the reference one at the same
# relative path: the compiler path (first word) is dropped, reference paths are rewritten by the sed script, the
# given flags are dropped ("-mllvm" drops the option after it too; "-fpass-plugin" drops -fpass-plugin=...; "RT"
# drops the runtime archives and start object of the instrumented builds).  Prints differing files and a count.
set -uo pipefail
RD=${1:?usage}; ND=${2:?usage}; SEDS=${3:?usage}; shift 3
DROP=" $* "
norm() { sed "$2" "$1" | awk -v drop="$DROP" '{ out = ""; skip = 0
  for (i = 2; i <= NF; i++) {
    if (skip) { skip = 0; continue }
    if ($i == "-mllvm" && index(drop, " -mllvm ") > 0) { skip = 1; continue }
    if (index(drop, " " $i " ") > 0) continue
    if ($i ~ /^-fpass-plugin=/ && index(drop, " -fpass-plugin ") > 0) continue
    if (index(drop, " RT ") > 0 && ($i ~ /\/libkzrt(-posix)?\.a$/ || $i ~ /kz_rt_posix_auto\.c\.o$/)) continue
    if (index(drop, " RT ") > 0 && ($i == "-Wl,--whole-archive" || $i == "-Wl,--no-whole-archive") && ($(i+1) ~ /libkzrt/ || $(i-1) ~ /libkzrt/)) continue
    out = out " " $i }
  print substr(out, 2) }'; }
# table object directories carry a hash of their path (D1): matched and compared with the hash replaced by "H"
hn() { sed -E 's/\.tbl[0-9a-f]{40}/.tblH/g'; }
declare -A DER
while read -r f; do DER["$(echo "${f#"$ND"/}" | hn)"]=$f; done < <(find "$ND" -name link.txt -path '*CMakeFiles*')
n=0; d=0
while read -r f; do
  rel=$(echo "${f#"$RD"/}" | hn); n=$((n + 1)); g=${DER[$rel]:-}
  if [ -z "$g" ]; then echo "missing in derived: $rel"; d=$((d + 1)); continue; fi
  if ! diff <(norm "$f" "$SEDS" | hn) <(norm "$g" "s#^##" | hn) > /dev/null; then echo "differs: $rel"; diff <(norm "$f" "$SEDS" | hn | tr ' ' '\n') <(norm "$g" "s#^##" | hn | tr ' ' '\n') | head -8; d=$((d + 1)); fi
done < <(find "$RD" -name link.txt -path '*CMakeFiles*' | sort)
echo "link commands: $n compared, $d differ"
[ "$d" = 0 ]

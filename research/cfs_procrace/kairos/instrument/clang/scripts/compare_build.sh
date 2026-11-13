#!/usr/bin/env bash
# compare_build.sh -- what differs between a reference cFS build tree and a derived (instrumented) one: the recorded
# compile commands (compile_commands.json, paths normalized), every object file (byte compare; when they differ,
# whether the differing strings are only paths, version strings or the build time), and the link commands.
#   compare_build.sh <reference build dir> <derived build dir> <extra flag text to remove> <from=to>...
# Each from=to rewrites a reference path prefix to the derived one (for object names and command text).
set -uo pipefail
REFB=$1; DERB=$2; EXTRA=$3; shift 3
SUBS=("$@")
sedargs=(); for s in "${SUBS[@]}"; do sedargs+=(-e "s|${s%%=*}|${s#*=}|g"); done
echo "# compare_build.sh $REFB -> $DERB"
echo "## compile commands (paths mapped; the derived commands without: $EXTRA)"
while IFS= read -r db; do
  r=$(grep -o '"command": "[^"]*"' "$REFB/$db" | sed "${sedargs[@]}" | sort)
  d=$(grep -o '"command": "[^"]*"' "$DERB/$db" | sed "s| $EXTRA||" | sort)
  n=$(printf '%s\n' "$r" | grep -c .); x=$(grep -o '"command": "[^"]*"' "$DERB/$db" | grep -cF -- "$EXTRA")
  if [ "$r" = "$d" ]; then echo "$db: $n commands, identical after removing the added flag ($x commands carry it)"; else echo "$db: DIFFERENT"; diff <(printf '%s\n' "$r") <(printf '%s\n' "$d") | head -20; fi
done < <(cd "$REFB" && find . -name compile_commands.json | sort)
echo "## objects"
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
text_same() { objcopy -O binary --only-section=.text "$1" "$TMP/a" 2>/dev/null; objcopy -O binary --only-section=.text "$2" "$TMP/b" 2>/dev/null; cmp -s "$TMP/a" "$TMP/b"; }
same=0; strs=0; other=0; absent=0
while IFS= read -r o; do
  d=$(printf '%s
' "$o" | sed "${sedargs[@]}")
  if [ ! -f "$DERB/$d" ]; then echo "absent in derived: $o"; absent=$((absent+1)); continue; fi
  if cmp -s "$REFB/$o" "$DERB/$d"; then same=$((same+1)); continue; fi
  n=$(diff <(strings -a "$REFB/$o") <(strings -a "$DERB/$d") | grep '^[<>]' | grep -v '/home/user/work/\|git:\|^[<>] 20[0-9]\{10\}$\|^[<>] \.rodata\.str' | wc -l)
  if text_same "$REFB/$o" "$DERB/$d"; then t="same .text"; else t=".text differs"; fi
  if [ "$n" = 0 ]; then strs=$((strs+1)); echo "differs only in path, version or build-time strings ($t): $o"; else other=$((other+1)); echo "differs ($t): $o"; fi
done < <(cd "$REFB" && find . -name '*.o' | sort)
echo "objects: identical $same, path/version/time strings only $strs, other differences $other, absent $absent"
echo "## link commands (paths mapped)"
while IFS= read -r l; do
  dl=$(printf '%s\n' "$l" | sed "${sedargs[@]}")
  out=$(diff <(sed "${sedargs[@]}" "$REFB/$l" | tr ' ' '\n') <(tr ' ' '\n' < "$DERB/$dl") | grep '^[<>]' | sed "s|^|  |")
  [ -n "$out" ] && { echo "$l:"; printf '%s\n' "$out"; }
done < <(cd "$REFB" && find . -name link.txt | sort)
exit 0

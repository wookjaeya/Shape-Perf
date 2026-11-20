#!/usr/bin/env bash
# compare.sh c1|r2 <F0 kz-dump text> <events.tsv of this candidate> <out dir> -- the IR section 13.2 comparison of one
# pilot boot with the F0 record of the same configuration, done by the GCC candidate's scripts (compare_c1.sh,
# compare_r2.sh: the same normalization for every candidate).  Their outputs name the instrumented column "gcc"; here
# it is renamed "llvm" (files *.gcc -> *.llvm, gcc_counts.raw -> llvm_counts.raw, header "GCC" -> "LLVM").
set -uo pipefail
C=${1:?c1|r2}; F0=${2:?usage}; EV=${3:?usage}; O=${4:?usage}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"$HERE/../../gcc/scripts/compare_$C.sh" "$F0" "$EV" "$O" | sed 's/(gcc)/(llvm)/'
for f in "$O"/*.gcc "$O"/*.gcc.raw; do [ -e "$f" ] && mv "$f" "${f/.gcc/.llvm}"; done
[ -e "$O/gcc_counts.raw" ] && mv "$O/gcc_counts.raw" "$O/llvm_counts.raw"
sed -i 's/GCC site | GCC events/LLVM site | LLVM events/' "$O/counts.txt"

#!/usr/bin/env bash
# check.sh -- the counts behind the dated notes of 2026-11-19 (results 26.1): every count those notes give is read here
# from a kept file (records under ../, kept outputs under /home/user/work/redesign/obs1, kairos/semantics).  It only
# reads; it writes its report to stdout.  No row id, key or verdict is an input: the checks print what the files hold.
#   bash check.sh > check.txt
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; REC="$(cd "$HERE/.." && pwd)"; R="$(cd "$REC/.." && pwd)"
O=/home/user/work/redesign/obs1; FLOOR=314572800; SEM=$R/kairos/semantics
B5="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"

echo "# check.sh report"
echo "repo_head $(git -C "$R" rev-parse --short HEAD)"

echo; echo "## A. guard logs per record directory: logs, end lines, verdict=completed, lowest 'lowest=', end values below the floor"
guard() {  # label then files
  local label=$1; shift
  local n e c low below
  n=$#; e=$(grep -h '^end ' "$@" | wc -l); c=$(grep -h '^end ' "$@" | grep -c 'verdict=completed')
  low=$(grep -h '^end ' "$@" | sed -E 's/.* lowest=([0-9]+).*/\1/' | sort -n | head -1)
  below=$(grep -h '^end ' "$@" | sed -E 's/.* free=([0-9]+).*/\1/' | awk -v f=$FLOOR '$1<f{c++} END{print c+0}')
  echo "$label logs=$n end_lines=$e completed=$c lowest=$low end_below_floor=$below"
}
D=$REC/review-fixes-2-mc-20261119
guard "review-fixes-2-mc-20261119/df_*.log" "$D"/df_*.log
guard "review-fixes-2-mc-20261119/scratch_guard/001-016" "$D"/scratch_guard/0{0[1-9],1[0-6]}_*.log
guard "review-fixes-2-mc-20261119/scratch_guard/017-029" "$D"/scratch_guard/0{1[7-9],2[0-9]}_*.log
guard "closure-kzstatic-mc-20261119/guard" "$REC"/closure-kzstatic-mc-20261119/guard/*.log
guard "closure-runtime-20261119/guard" "$REC"/closure-runtime-20261119/guard/*.log
guard "closure-c1prep-20261119/guard" "$REC"/closure-c1prep-20261119/guard/*.log
echo "end lines below the floor:"
grep -H '^end ' "$D"/df_*.log "$D"/scratch_guard/*.log "$REC"/closure-kzstatic-mc-20261119/guard/*.log \
  "$REC"/closure-runtime-20261119/guard/*.log "$REC"/closure-c1prep-20261119/guard/*.log \
  | sed -E 's#^'"$REC"'/##' | awk -v f=$FLOOR '{if (match($0, / free=[0-9]+/)) { v=substr($0, RSTART+6, RLENGTH-6)+0; if (v<f) print "  " $0 }}'
echo "first line of that log: $(head -1 "$D"/scratch_guard/002_scratch-make.log | cut -c1-80)"

echo; echo "## B. guarded actions that overlap in time (start of one before the end of an earlier one), per directory"
overlaps() {  # files; prints name start end for each overlap pair
  for f in "$@"; do
    s=$(grep -m1 '^start ' "$f" | awk '{print $2}'); e=$(grep -m1 '^end ' "$f" | awk '{print $2}')
    [ -n "$s" ] && [ -n "$e" ] && echo "$s $e $(basename "$f" .log)"
  done | sort | awk '{ for (i = 1; i <= n; i++) if ($1 < E[i]) print "  " N[i] " [" S[i] " .. " E[i] "] overlaps " $3 " [" $1 " .. " $2 "]"; n++; S[n]=$1; E[n]=$2; N[n]=$3 }'
}
echo "review-fixes-2-mc-20261119/df_*.log:"; overlaps "$D"/df_*.log
echo "closure-kzstatic-mc-20261119/guard:"; overlaps "$REC"/closure-kzstatic-mc-20261119/guard/*.log
echo "closure-runtime-20261119/guard:"; overlaps "$REC"/closure-runtime-20261119/guard/*.log
echo "closure-c1prep-20261119/guard:"; overlaps "$REC"/closure-c1prep-20261119/guard/*.log

echo; echo "## C. review-fix build: measurement files against the comparisons (mtime UTC; compare lines as written)"
for f in "$D"/measure/*; do echo "  $(TZ=UTC stat -c '%y' "$f" | cut -c1-19) measure/$(basename "$f")"; done
sed 's/^/  compare_all: /' "$D"/compare_all.txt; sed 's/^/  compare_rest: /' "$D"/compare_rest.txt
echo "  run_all last: $(tail -1 "$D"/run_all.txt)"
echo "  build_start: $(head -1 "$D"/build_start.txt)"
for c in 44a45c7 090e280 5096d4a; do echo "  commit $c $(git -C "$R" log -1 --format=%cI "$c")"; done

echo; echo "## D. cycle preparation: ledger and registry checks around the kzgcc 0.5 build (mtime UTC, last line)"
P=$REC/closure-c1prep-20261119
for f in $(cd "$P" && find . -name '*ledger_check*' -o -name '*tools_check*' | sort); do
  echo "  $(TZ=UTC stat -c '%y' "$P/$f" | cut -c1-19) ${f#./}: $(tail -1 "$P/$f")"
done
echo "  plugin build guard: $(grep -h '^start ' "$P"/guard/p_plugin-build.log | cut -c1-31) / $(grep -h '^end ' "$P"/guard/p_plugin-build.log | cut -c1-29)"
echo "  guard logs with ctest in the name: closure-c1prep $(ls "$P"/guard | grep -c ctest), closure-runtime $(ls "$REC"/closure-runtime-20261119/guard | grep -c ctest)"

echo; echo "## E. the test tool_identity_schema_revision in the kept test lists and logs (matching lines)"
for f in "$D"/ctest_nonstatic_prebuild.log "$D"/scratch/ctest_first_scratch.log \
         "$REC"/closure-runtime-20261119/prebuild/mc_tests_not_in_scratch_164.txt \
         "$REC"/closure-runtime-20261119/scratch/checkbuild_tests_failed_10.txt; do
  echo "  ${f#$REC/}: $(grep -c 'tool_identity_schema_revision' "$f")"
done

echo; echo "## F. alternatives lines 'none found' per build (line, row, rests on), with the row's terminal effects and P0-unverified layers"
for b in $B5; do
  f=$O/cz_final_order/$b/relations.txt
  echo "$b: $(grep -c 'alternatives .*: none found' "$f")"
  awk 'function flush() { if (pend != "") print pend " | absence effect " eff " | layers " lay; pend="" }
       /^  [A-Z][0-9]+ /{ flush(); row=$1; eff=""; lay="" }
       /^      absence: /{ if (match($0, /terminal \[[^]]*\]/)) eff=substr($0, RSTART, RLENGTH) }
       /^      layers: /{ lay=""; s=$0; while (match(s, /P0-unverified\([^)]*\)/)) { lay=lay substr(s, RSTART, RLENGTH) " "; s=substr(s, RSTART+RLENGTH) } }
       /^      alternatives .*: none found/{ r=$0; sub(/.*; rests on: /, "", r); sub(/;.*/, "", r); pend="  line " NR " row " row " rests on: " r }
       END { flush() }' "$f"
done

echo; echo "## G. lifetime-end clear-slot lines per build; positions in the two C1 builds"
for b in $B5; do echo "$b: $(grep -c '^ *lifetime end \[.*\]: clear-slot ' "$O/cz_final_order/$b/relations.txt")"; done
for b in c1_pre_tx c1_post_tx; do
  echo "$b positions:"; grep -n '^ *lifetime end \[.*\]: clear-slot ' "$O/cz_final_order/$b/relations.txt" \
    | sed -E 's#^([0-9]+):.*clear-slot ([^ ]+) in ([^:]+):.*#  \1 \2 \3#; s#/home/user/work/procrace/c1_build/[a-z]+/src/#<src>/#'
done

echo; echo "## H. HB-await edges of the last round (edges.tsv): by write, by writer actor, by writer x waiter pair"
for b in c1_pre_tx r2_core; do
  f=$O/cz_final_order/$b/edges.tsv
  echo "$b: $(awk -F'\t' '$2=="HB-await"' "$f" | wc -l)"
  awk -F'\t' '$2=="HB-await"{print $4}' "$f" | sort | uniq -c | sed 's/^/  from /'
  awk -F'\t' '$2=="HB-await"{w=$5; sub(/ \(.*\)$/, "", w); print w}' "$f" | sort | uniq -c | sed 's/^/  writer /'
  echo "  pairs: $(awk -F'\t' '$2=="HB-await"{w=$5; t=$7; sub(/ \(.*\)$/, "", w); sub(/ \(.*\)$/, "", t); print w " -> " t}' "$f" | sort -u | wc -l); edges per pair:"
  awk -F'\t' '$2=="HB-await"{w=$5; t=$7; sub(/ \(.*\)$/, "", w); sub(/ \(.*\)$/, "", t); print w " -> " t}' "$f" | sort | uniq -c | sed 's/^/   /'
done

echo; echo "## I. framework semantics data: end lines of the seven version files, distinct api names"
for f in "$SEM"/framework_semantics_*.txt; do echo "  $(basename "$f"): $(tail -1 "$f" | sed 's/^end //')"; done
awk '$1=="end"{for (i=2;i<=NF;i++){split($i,a,"="); s[a[1]]+=a[2]}} END{print "  sum entries=" s["entries"] " declared=" s["declared"]}' "$SEM"/framework_semantics_*.txt
echo "  distinct api names: $(cat "$SEM"/framework_semantics_*.txt | awk '$1=="api"{print $2}' | sort -u | wc -l)"
A=$REC/semantics-20261116/compare/agreement_c1_pre_tx.txt
echo "  agreement_c1_pre_tx.txt lines $(wc -l < "$A"), by state (fourth word):"; awk '{print $4}' "$A" | sort | uniq -c | sed 's/^/   /'

echo; echo "## J. P0 declaration data: lines naming strlen, and the api line of strcmp"
P0=$SEM/p0_declarations_glibc-2.39-headers.txt
echo "  strlen: $(grep -c 'strlen' "$P0")"
grep -n '^api strcmp ' "$P0" | sed 's/^/  /'
echo "# end"

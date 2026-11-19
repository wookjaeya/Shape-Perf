#!/usr/bin/env bash
# extract.sh BUILD N -- after the keep step of boot N of BUILD (results 27.1; records/closure-boots-20261119/README.md):
# reads, and only reads, the runner's record directory records/closure-c1prep-20261119/boot/BUILD/boot<N>/, the boot's
# work directories /home/user/work/redesign/obs1/c1cyc_<s>_{boot,convert,explain,eval}<N>/, the ledger and
# boot/declaration_pin.txt, and writes into BUILD/boot-N/ beside this directory:
#   index.txt              the lines the steps wrote that results 27 cites, each under the name of its file: the verify
#                          lines, the space lines, the boot command's rc and the run.meta lines of the start and stop,
#                          the instance_files line and the line count of inst_files.sha256.other, the control check's
#                          summary and loss lines, the chain line, the gate lines, the stop_eval.tsv line count, the
#                          explanation's per-branch line of judge.txt, the eval head line and the main branch's lines of
#                          eval_tail.txt, the F3 lines of eval.txt whose note (column 10) is exactly 600 characters (results
#                          26.9, fix-judge-03), the expectation comparison's total lines, keep.txt, and the declaration
#                          sha256 named by every verify*.txt of the record directory and by the pin file (fix-decl-09)
#   explain_judge.txt, explain_inputs.txt, explain_input_checks.txt   copies from the explanation output
#   explain_index_D1-R0.tsv  a copy of the main branch's index.tsv
#   explain_counts.tsv     per branch: file (index, uses), column, value, lines; the index.tsv columns link, use_cond,
#                          order, intervention, effect, violation, odf, headline, pair_kind and the uses.tsv columns
#                          status, outcome, name_state, link_result (columns found by their header names)
#   expect_counts.tsv      per branch, class and result of expect_compare.txt: lines
#   large_outputs.tsv      per work directory and the record directory: files, apparent bytes, blocks, the keep list's
#                          line count and sha256, and whether the ledger has its row
#   guard_ends.txt         the start and end lines of every guard log of the record directory
#   runner_files.sha256    sha256 of every file of the record directory (what is committed from the runner)
# It counts and copies; it reads no case name and decides nothing.  An input that is absent is written as "absent".
# It refuses when one of its outputs exists (rc 2).  rc 0; 1 when the record directory is absent; 64 on usage.
set -uo pipefail
B=${1:-}; N=${2:-}
case "$B" in c1_pre_tx) S=pre ;; c1_post_tx) S=post ;; *) echo "usage: extract.sh c1_pre_tx|c1_post_tx N" >&2; exit 64 ;; esac
case "$N" in 1|1r|2|3|4|5) ;; *) echo "usage: extract.sh c1_pre_tx|c1_post_tx N (1, 1r, 2-5)" >&2; exit 64 ;; esac
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REC="$(cd "$HERE/.." && pwd)"
R="$(cd "$REC/../.." && pwd)"
BD=$R/records/closure-c1prep-20261119/boot
L=$BD/$B/boot$N
O=/home/user/work/redesign/obs1
RUN=$O/c1cyc_${S}_boot$N/tx
XO=$O/c1cyc_${S}_explain$N
EV=$O/c1cyc_${S}_eval$N
LED=$R/records/LEDGER.tsv
X=$REC/$B/boot-$N
[ -d "$L" ] || { echo "extract.sh: no record directory $L" >&2; exit 1; }
mkdir -p "$X" || exit 64
OUTS=(index.txt explain_judge.txt explain_inputs.txt explain_input_checks.txt explain_index_D1-R0.tsv explain_counts.tsv
      expect_counts.tsv large_outputs.tsv guard_ends.txt runner_files.sha256)
for f in "${OUTS[@]}"; do [ ! -e "$X/$f" ] || { echo "extract.sh: $X/$f exists" >&2; exit 2; }; done
export LC_ALL=C
show() { # TITLE FILE [grep -E pattern]: the file's lines (or the matching ones) under its name, or "absent"
  echo "== $1"
  if [ ! -f "$2" ]; then echo "absent: $2"; return 0; fi
  if [ -n "${3:-}" ]; then grep -E -- "$3" "$2" | sed 's/^/  /'; else sed 's/^/  /' "$2"; fi
}
cpy() { if [ -f "$1" ]; then cp "$1" "$2"; else echo "absent: $1" > "$2"; fi; }
{ echo "kz-boot-extract 1 (records/closure-boots-20261119/scripts/extract.sh; results 27.1) $(date -u +%FT%TZ)"
  echo "build $B boot $N; record directory ${L#$R/}; work directories $O/c1cyc_${S}_{boot,convert,explain,eval}$N"
  for v in verify verify_at_boot verify_at_explain; do show "$v.txt (head and last line)" "$L/$v.txt" '^(declaration sha256=|measured |rows |need rows)'; done
  show "space_at_boot.txt" "$L/space_at_boot.txt" '^(measured|declared|per boot|need|verdict)'
  show "boot.txt (last lines)" "$L/boot.txt" '^(rc=|.*operational|stop=|.*exit|dump |kz-rt-dump)'
  show "run.meta (start, stop, exit, dump, instance files)" "$RUN/run.meta" '^(instances\.|tx\.|rx\.|stop=|tx_exit|udp|dump |kz-rt-dump|instance_files|pid )'
  echo "== inst_files.sha256.other"; if [ -f "$RUN/inst_files.sha256.other" ]; then echo "  lines $(wc -l < "$RUN/inst_files.sha256.other")"; else echo "absent: $RUN/inst_files.sha256.other"; fi
  show "control_check.txt (summary and loss lines)" "$L/control_check.txt" '^(dump text|loss fields|loss records|pool overflow|kz-rt-dump check)'
  show "chain.txt (chain line)" "$L/chain.txt" '^chain	'
  echo "== stop_eval.tsv"; if [ -f "$L/stop_eval.tsv" ]; then echo "  lines $(wc -l < "$L/stop_eval.tsv") sha256 $(sha256sum < "$L/stop_eval.tsv" | cut -c1-64)"; else echo "absent: $L/stop_eval.tsv"; fi
  show "gate.txt" "$L/gate.txt"
  show "explain.txt (rc)" "$L/explain.txt" '^rc='
  show "judge.txt of the explanation output" "$XO/judge.txt"
  show "eval_run.txt (rc)" "$L/eval_run.txt" '^rc='
  show "eval_tail.txt (head line and the main branch's lines)" "$L/eval_tail.txt" '^(== .*eval\.txt|branch D1-R0 )'
  echo "== F3 lines of eval.txt whose note (column 10) is exactly 600 characters (results 26.9, fix-judge-03)"
  if [ -f "$EV/eval.txt" ]; then
    echo "  all branches: $(awk -F'\t' '$1=="F3" && length($10)==600' "$EV/eval.txt" | wc -l) of $(awk -F'\t' '$1=="F3"' "$EV/eval.txt" | wc -l) F3 lines"
    awk -F'\t' '$1=="F3" {t[$3]++; if (length($10)==600) c[$3]++} END {for (b in t) printf "  branch %s: %d of %d\n", b, c[b]+0, t[b]}' "$EV/eval.txt" | sort
  else echo "absent: $EV/eval.txt"; fi
  show "expect_compare.txt (totals)" "$L/expect_compare.txt" '^# (by class|match)'
  show "keep.txt" "$L/keep.txt"
  echo "== declaration sha256 named by every verify*.txt of the record directory and by the pin file"
  for f in "$L"/verify*.txt; do [ -f "$f" ] && echo "  ${f##*/} $(sed -n 's/^declaration sha256=\([0-9a-f]*\) .*/\1/p' "$f")"; done
  if [ -f "$BD/declaration_pin.txt" ]; then echo "  declaration_pin.txt $(sed -n 's/^declaration sha256=\([0-9a-f]*\) .*/\1/p' "$BD/declaration_pin.txt")"; else echo "  declaration_pin.txt absent"; fi
  echo "  declaration4.tsv now $(sha256sum < "$BD/declaration4.tsv" | cut -c1-64)"
} > "$X/index.txt"
cpy "$XO/judge.txt" "$X/explain_judge.txt"
cpy "$XO/inputs.txt" "$X/explain_inputs.txt"
cpy "$XO/input_checks.txt" "$X/explain_input_checks.txt"
cpy "$XO/D1-R0/index.tsv" "$X/explain_index_D1-R0.tsv"
{ printf 'branch\tfile\tcolumn\tvalue\tlines\n'
  for d in "$XO"/*/; do
    br=${d%/}; br=${br##*/}
    [ -f "$d/index.tsv" ] || continue
    awk -F'\t' -v br="$br" -v f=index -v want="link,use_cond,order,intervention,effect,violation,odf,headline,pair_kind" '
      NR==1 {n=split(want, w, ","); for (i=1;i<=NF;i++) for (j=1;j<=n;j++) if ($i==w[j]) col[i]=$i; next}
      {for (i in col) c[col[i] "\t" $i]++}
      END {for (k in c) printf "%s\t%s\t%s\t%d\n", br, f, k, c[k]}' "$d/index.tsv" | sort
    [ -f "$d/uses.tsv" ] && awk -F'\t' -v br="$br" -v f=uses -v want="status,outcome,name_state,link_result" '
      NR==1 {n=split(want, w, ","); for (i=1;i<=NF;i++) for (j=1;j<=n;j++) if ($i==w[j]) col[i]=$i; next}
      {for (i in col) c[col[i] "\t" $i]++}
      END {for (k in c) printf "%s\t%s\t%s\t%d\n", br, f, k, c[k]}' "$d/uses.tsv" | sort
  done; } > "$X/explain_counts.tsv"
{ printf 'branch\tclass\tresult\tlines\n'
  if [ -f "$L/expect_compare.txt" ]; then
    awk -F'\t' '!/^#/ && NF>=8 {c[$1 "\t" $7 "\t" $8]++} END {for (k in c) printf "%s\t%d\n", k, c[k]}' "$L/expect_compare.txt" | sort
  else echo "absent: $L/expect_compare.txt"; fi; } > "$X/expect_counts.tsv"
{ printf 'what\tpath\tfiles\tapparent_bytes\tblocks_bytes\tkeep_list_lines\tkeep_list_sha256\tledger_row\n'
  for x in boot convert explain eval; do
    d=$O/c1cyc_${S}_$x$N; lst=$L/c1cyc_${S}_$x$N.sha256; id=OBS_c1cyc_${S}_$x$N
    if [ -d "$d" ]; then fi_=$(find "$d" -type f | wc -l); ap=$(du -sb "$d" | cut -f1); bl=$(du -sB1 "$d" | cut -f1); else fi_=absent; ap=-; bl=-; fi
    if [ -f "$lst" ]; then ll=$(wc -l < "$lst"); ls_=$(sha256sum < "$lst" | cut -c1-64); else ll=absent; ls_=-; fi
    if awk -F'\t' -v i="$id" '$1==i {f=1} END {exit !f}' "$LED"; then lr=$id; else lr=absent; fi
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$x" "$d" "$fi_" "$ap" "$bl" "$ll" "$ls_" "$lr"
  done
  printf 'record\t%s\t%s\t%s\t%s\t-\t-\t-\n' "${L#$R/}" "$(find "$L" -type f | wc -l)" "$(du -sb "$L" | cut -f1)" "$(du -sB1 "$L" | cut -f1)"
} > "$X/large_outputs.tsv"
{ for f in "$L"/guard_*.log; do [ -f "$f" ] || continue; echo "== ${f##*/}"; grep -E '^(start|end) ' "$f"; done; } > "$X/guard_ends.txt"
(cd "$L" && find . -type f -print0 | sort -z | xargs -0 -r sha256sum) > "$X/runner_files.sha256"
echo "extract.sh: $B $N -> ${X#$R/} ($(ls "$X" | wc -l) entries)"

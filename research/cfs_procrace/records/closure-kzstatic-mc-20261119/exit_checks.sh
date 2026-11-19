#!/usr/bin/env bash
# exit_checks.sh STEP -- the exit checks and measurements of the declared kz-static closure build (results 21.0 (사)) on
# the outputs of run.sh (/home/user/work/redesign/obs1/cz_*), each into a new file under exit/ beside this script.  The
# counting scripts are those of records/closure-kzstatic-20261119/scripts, read in place.  Checks, not tools of the method;
# they judge nothing.
#   keys      keys_compare.py: the use-site keys of cz_final_order against rf2_final_order, and the two C1 builds
#   pairs     pair_check.py on the pair files of cz_final_order and cz_off_order, with the plan sidecars of cz_plans_v4 and
#             cz_plans_v4_ns2 (final facts)
#   lines     the new lines of every build's final-option relations text: success path (summary and per callee),
#             absence origins, holder (holder_counts.py), alternatives and the stored-field rule (alt_counts.py),
#             stored-name buffer and lifetime end (buffer_counts.py)
#   ruling_e  the address-passing call sites and the kinds of the `other` open-reason class on the two C1 builds
#             (kz-static show --section resulteffect views made in SCRATCH and removed; open_other.py)
# Environment: SCRATCH a scratch directory (keys, ruling_e).  rc 0 when every script returned 0, 1 otherwise.
set -uo pipefail
W=/home/user/work/redesign/obs1
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCR=/home/user/Shape-Perf/research/cfs_procrace/records/closure-kzstatic-20261119/scripts
KS=/home/user/work/merge-check/kz-llvm/tools/kz-static/kz-static
E=$HERE/exit
STEP=${1:?usage: exit_checks.sh keys|pairs|lines|ruling_e}
BUILDS="c1_pre_tx c1_post_tx r2_core c5_641 c5_642"
mkdir -p "$E"
rc=0
newf() { [ -e "$1" ] && { echo "refusing: $1 exists" >&2; exit 2; }; : > "$1"; }
case "$STEP" in
  keys)
    S=${SCRATCH:?SCRATCH}/keys_new; [ -e "$S" ] && { echo "refusing: $S exists" >&2; exit 2; }; mkdir -p "$S"
    for b in $BUILDS; do ln -s "$W/cz_final_order/$b/keys.txt" "$S/$b.keys.txt"; done
    newf "$E/keys_compare.txt"
    python3 -I "$SCR/keys_compare.py" "$W/rf2_final_order" "$S" c1_pre_tx c1_post_tx r2_core c5_641 c5_642 > "$E/keys_compare.txt" || rc=1
    rm -r "$S" ;;
  pairs)
    for b in $BUILDS; do
      f=$E/pair_check_final_$b.txt; newf "$f"
      python3 -I "$SCR/pair_check.py" "$W/cz_final_order/$b/order.txt" "$W/cz_final_order/$b/pairs.tsv" "$W/cz_final_order/$b/edges.tsv" \
        "$W/cz_plans_v4_ns2/$b/plan.side" "$W/cz_plans_v4/$b/plan.side" > "$f" || rc=1
      f=$E/pair_check_off_$b.txt; newf "$f"
      python3 -I "$SCR/pair_check.py" "$W/cz_off_order/$b/order.txt" "$W/cz_off_order/$b/pairs.tsv" "$W/cz_off_order/$b/edges.tsv" > "$f" || rc=1
    done ;;
  lines)
    for b in $BUILDS; do
      R=$W/cz_final_order/$b/relations.txt
      f=$E/success_path_$b.txt; newf "$f"
      { grep -E '^summary success path' "$R"; echo "# per creator callee (the namespace-block lines), by state:";
        grep -E '^ +callee-check \(success path\): ' "$R" | sed -E 's/^ +callee-check \(success path\): [^:]+: (held|missing)(\([^)]*\))?.*/\1\2/' | sort | uniq -c; } > "$f" || rc=1
      f=$E/absence_origins_$b.txt; newf "$f"
      { echo "lines $(grep -cE '^ +absence origins: ' "$R")"; echo "# by number of origins:";
        grep -E '^ +absence origins: ' "$R" | grep -oE ': [0-9]+ origin\(s\)' | sort | uniq -c;
        echo "# origins by class:"; grep -E '^ +absence origins: ' "$R" | grep -oE '\[(other|search no-match|[a-z -]+)\]' | sort | uniq -c; } > "$f" || rc=1
      f=$E/holder_$b.txt; newf "$f"; python3 -I "$SCR/holder_counts.py" "$R" > "$f" || rc=1
      f=$E/alternatives_$b.txt; newf "$f"; python3 -I "$SCR/alt_counts.py" "$R" > "$f" || rc=1
      f=$E/buffer_$b.txt; newf "$f"; python3 -I "$SCR/buffer_counts.py" "$R" "$E/buffer_lines_$b.txt" > "$f" || rc=1
      f=$E/heads_$b.txt; newf "$f"; grep -E '^(tool kz-static|rules with object access|summary success path|rules with decided call results|summary stored-name)' "$R" > "$f" || rc=1
    done ;;
  ruling_e)
    S=${SCRATCH:?SCRATCH}
    for b in c1_pre_tx c1_post_tx; do
      v=$S/re_$b.txt; [ -e "$v" ] && { echo "refusing: $v exists" >&2; exit 2; }
      if "$KS" show "$W/cz_final_extract/$b/$b/NP/facts.kz" --section resulteffect > "$v"; then
        f=$E/address_passing_$b.txt; newf "$f"
        grep -oE '; to [A-Za-z_0-9]+ at [0-9]+:[0-9]+ argument [0-9]+' "$v" | sort | uniq -c > "$f" || rc=1
        f=$E/open_other_$b.txt; newf "$f"; python3 -I "$SCR/open_other.py" "$v" > "$f" || rc=1
        sha256sum < "$v" | sed "s|-\$|re_$b.txt $(stat -c%s "$v") B|" >> "$E/ruling_e_views.sha256"
      else rc=1; fi
      rm -f "$v"
    done ;;
  *) echo "usage: exit_checks.sh keys|pairs|lines|ruling_e" >&2; exit 64 ;;
esac
exit $rc

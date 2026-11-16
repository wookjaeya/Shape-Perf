#!/usr/bin/env bash
# run_explain.sh STEP -- the explanation records of the natural boot of B' (plan/observation_path_results_20261116.md 7.0)
# and the evidence of the exit test.  Logs and small text records into this directory; outputs into one new directory
# outside the repository, /home/user/work/redesign/obs1/c1pre_explain/.  Every input is read only.
#   facts-view | explain [NAME] | copy NAME [USE-SITE KEY...] | plan-checks | hand | bundle   (NAME: the output directory under c1pre_explain, default explain)
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
GUARD=$R/env/tools/space_guard.sh
O=/home/user/work/redesign/obs1
P=$O/plangen/c1_pre_tx
W=$O/c1pre_derived
BP=$O/c1pre_bprime/c1_pre_tx/NP
RUN=$O/c1pre_boot/tx
REL=$O/plangen_relcheck/c1_pre_tx
EX=$O/c1pre_explain
MC=/home/user/work/merge-check/kz-llvm
T8=/home/user/work/redesign/step2a/tests_8c_fix/c1_pre_tx/NP
BOOTREC=$R/records/obs1-20261116-boot
W6=/home/user/work/w6/run/c1nat/gi/pre_1df262a_tx
case "${1:-}" in
  facts-view)
    # the identity lines of the plan's facts (T8CF c1_pre_tx) through the registry's kz-dump (MC 05623054)
    [ ! -e "$EX" ] || { echo "run_explain.sh: $EX exists" >&2; exit 2; }
    mkdir "$EX" || exit 2
    { echo "facts $T8/facts.kz sha256=$(sha256sum < "$T8/facts.kz" | cut -c1-64)"
      echo "kz-dump $(sha256sum < "$MC/tools/kz-dump/kz-dump" | cut -c1-64)"
      "$MC/tools/kz-dump/kz-dump" "$T8/facts.kz" | head -8
      "$MC/tools/kz-dump/kz-dump" "$T8/facts.kz" | grep -E 'name="(extract\.build_identity|extract\.p0_table|extract\.p0_table_scope|p0\.pthread_create)"'
    } > "$EX/facts_view.txt"; cp "$EX/facts_view.txt" "$HERE/facts_view.txt"; cat "$HERE/facts_view.txt" | cut -c1-200 ;;
  explain)
    N=${2:-explain}
    "$GUARD" --need 30000000 --kind explain --log "$HERE/df_explain_$N.log" -- \
      python3 -I "$R/env/obs1/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$W/binds_instr.kzs" \
        --relations "$REL/relations.txt" --keys "$REL/keys.txt" --vocabulary "$R/kairos/tests/static/data/reason_vocabulary.txt" \
        --facts-view "$EX/facts_view.txt" --bprime-basis "$BP/basis.kz" --build-label c1_pre_tx --out "$EX/$N" \
        --dump-text "$RUN/tx.dump.txt" --run-meta "$RUN/run.meta" --chain "$BOOTREC/chain.txt" --stop-eval "$BOOTREC/stop_eval.tsv" \
        --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" > "$HERE/explain_$N.log" 2>&1; rc=$?
    echo "explain rc=$rc" >> "$HERE/explain_$N.log"; cat "$HERE/explain_$N.log"; exit $rc ;;
  copy)
    # the records are text; the repository keeps the index, the inputs, the refusals, the sha256 of every record and the
    # records of the rows named by their use-site keys (arguments 3...); all records stay in the output directory
    N=${2:-explain}; shift 2 || true
    [ ! -e "$HERE/records" ] || { echo "run_explain.sh: $HERE/records exists" >&2; exit 2; }
    mkdir "$HERE/records" || exit 2
    cp "$EX/$N/index.tsv" "$EX/$N/inputs.txt" "$EX/$N/refused.txt" "$HERE/"
    for k in "$@"; do cp "$EX/$N/records/"*".$k."*.json "$HERE/records/" || exit 2; done
    (cd "$EX/$N" && find . -type f | sort | xargs sha256sum) > "$HERE/explain_$N.sha256"; wc -l < "$HERE/explain_$N.sha256" ;;
  plan-checks)
    # E2 mechanical checks over the generated plan: every site has a rule name and a ref; no expected value or state
    python3 -I - "$P/plan.kzs" "$P/plan.side" > "$HERE/plan_checks.txt" <<'PY'
import re, sys
m = open(sys.argv[1]).read().splitlines(); s = open(sys.argv[2]).read().splitlines()
sites = [l for l in m if l.startswith("site ")]
norule = [l for l in sites if not re.search(r"label=s\d{4}\.[a-z-]+(\+[a-z-]+)*\b", l)]
noref = [l for l in sites if 'ref="' not in l or 'ref=""' in l]
side_sites = [l for l in s if l.startswith("site ")]
side_noref = [l for l in side_sites if 'refs="' not in l or 'refs=""' in l]
words = re.compile(r"\b(expect|expected|expects|should|must return|normal)\b", re.I)
hits = [(f, i + 1) for f, lines in (("manifest", m), ("sidecar", s)) for i, l in enumerate(lines) if words.search(l) and not l.startswith("#")]
rules = {}
for l in sites:
    for r in re.search(r"label=s\d{4}\.(\S+)", l).group(1).split("+"):
        rules[r] = rules.get(r, 0) + 1
vals = [l for l in m if l.startswith("value ")]
print("manifest sites %d, without a rule name %d, without a ref %d" % (len(sites), len(norule), len(noref)))
print("sidecar site lines %d, without refs %d" % (len(side_sites), len(side_noref)))
print("values %d; keys of the value lines: %s" % (len(vals), sorted({k for l in vals for k in re.findall(r"(\w+)=", l)})))
print("lines with words of expectation (expect, should, must return, normal) outside comments: %d %s" % (len(hits), hits[:5]))
print("rules: " + ", ".join("%s %d" % kv for kv in sorted(rules.items())))
PY
    cat "$HERE/plan_checks.txt" ;;
  hand)
    # the hand-manifest record of 2026-10-29 against this record: the events of the relation-1 call chain
    python3 -I "$HERE/hand_compare_record.py" --hand-dump "$W6/tx.dump.txt" --gen-dump "$RUN/tx.dump.txt" \
      --gen-record "$HERE/records/c1_pre_tx.A21.17975444d83f6589.actor-12.json" > "$HERE/hand_compare.txt" 2>&1; rc=$?
    cat "$HERE/hand_compare.txt"; exit $rc ;;
  bundle)
    # the local bundle of the cycle's outputs outside the repository (stream-verified, not pushed)
    mkdir -p /home/user/work/preserve/bundles-20261116 && BUNDLE_BASE=/home/user/work/preserve/bundles-20261116 bash "$R/records/local-20261109/bundle_local.sh" obs1-20261116-cycle dirs "$O" \
      plangen c1pre_rt c1pre_derived c1pre_bprime c1pre_boot c1pre_convert c1pre_explain -- \
      "the first automatic observation path, c1_pre_tx: plans, runtime tree, derived build, B' basis and facts, the boot, the conversion, the explanation records" \
      > "$HERE/bundle.log" 2>&1; rc=$?
    tail -12 "$HERE/bundle.log"; exit $rc ;;
  *) echo "usage: run_explain.sh facts-view|explain|copy|plan-checks|hand|bundle" >&2; exit 64 ;;
esac

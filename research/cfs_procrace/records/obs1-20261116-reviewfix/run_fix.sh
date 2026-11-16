#!/usr/bin/env bash
# run_fix.sh STEP -- the fixes after review of the explanation step (plan/observation_path_results_20261116.md 7.11).
# Records into this directory; the one output directory outside the repository is new:
# /home/user/work/redesign/obs1/c1pre_explain_reviewfix/.  Every other input is read only; nothing is built or booted.
#   explain | copy | summary | rowsites | runtime-refs | cfe-assert | bindings
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
GUARD=$R/env/tools/space_guard.sh
O=/home/user/work/redesign/obs1
P=$O/plangen/c1_pre_tx
W=$O/c1pre_derived
BP=$O/c1pre_bprime/c1_pre_tx/NP
B=$O/plangen_bref/c1_pre_tx/NP
RUN=$O/c1pre_boot/tx
REL=$O/plangen_relcheck/c1_pre_tx
EX0=$O/c1pre_explain
EX=$O/c1pre_explain_reviewfix
REF=/home/user/work/procrace/c1_build/pre/1df262a
MC=/home/user/work/merge-check/kz-llvm
BOOTREC=$R/records/obs1-20261116-boot
TMP=${TMPDIR:-/tmp}
case "${1:-}" in
  explain)
    [ ! -e "$EX" ] || { echo "run_fix.sh: $EX exists" >&2; exit 2; }
    mkdir "$EX" || exit 2
    "$GUARD" --need 30000000 --kind explain --log "$HERE/df_explain.log" -- \
      python3 -I "$R/env/obs1/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$W/binds_instr.kzs" \
        --relations "$REL/relations.txt" --keys "$REL/keys.txt" --vocabulary "$R/kairos/tests/static/data/reason_vocabulary.txt" \
        --facts-view "$EX0/facts_view.txt" --bprime-basis "$BP/basis.kz" --build-label c1_pre_tx --out "$EX/explain_r4" \
        --dump-text "$RUN/tx.dump.txt" --run-meta "$RUN/run.meta" --chain "$BOOTREC/chain.txt" --stop-eval "$BOOTREC/stop_eval.tsv" \
        --startup-script "$RUN/inst/tx/cf/cfe_es_startup.scr" --instance-list "$RUN/inst_files.sha256" \
        --boot-budget "$HERE/boot_budget.tsv" > "$HERE/explain.log" 2>&1; rc=$?
    echo "explain rc=$rc" >> "$HERE/explain.log"; cat "$HERE/explain.log"; exit $rc ;;
  copy)
    [ ! -e "$HERE/records" ] || { echo "run_fix.sh: $HERE/records exists" >&2; exit 2; }
    mkdir "$HERE/records" || exit 2
    cp "$EX/explain_r4/index.tsv" "$EX/explain_r4/inputs.txt" "$EX/explain_r4/refused.txt" "$HERE/"
    cp "$EX/explain_r4/records/"*".17975444d83f6589."*.json "$HERE/records/" || exit 2
    (cd "$EX/explain_r4" && find . -type f | sort | xargs sha256sum) > "$HERE/explain_r4.sha256"; wc -l < "$HERE/explain_r4.sha256"
    # the records before and after, field by field for the relation-1 record (what the fixes changed)
    python3 -I - "$EX0/explain_r3/records" "$EX/explain_r4/records" > "$HERE/r3_r4_diff.txt" <<'PY'
import json, os, sys
a, b = sys.argv[1], sys.argv[2]
fa, fb = sorted(os.listdir(a)), sorted(os.listdir(b))
print("records r3 %d, r4 %d, same file names %s" % (len(fa), len(fb), fa == fb))
def flat(x, p=""):
    if isinstance(x, dict):
        for k in x:
            yield from flat(x[k], p + "." + k if p else k)
    elif isinstance(x, list):
        for i, v in enumerate(x):
            yield from flat(v, "%s[%d]" % (p, i))
    else:
        yield p, x
keys = {}
for f in fa:
    if f not in fb:
        continue
    A, B = dict(flat(json.load(open(os.path.join(a, f))))), dict(flat(json.load(open(os.path.join(b, f)))))
    for k in sorted(set(A) | set(B)):
        if k.startswith("inputs.") or k == "script":
            continue
        if A.get(k, "<absent>") != B.get(k, "<absent>"):
            top = k.split(".")[0].split("[")[0]
            keys[top] = keys.get(top, 0) + 1
            if ".17975444d83f6589." in f:
                print("A21\t%s\t%r -> %r" % (k, A.get(k, "<absent>"), B.get(k, "<absent>")))
print("changed leaf fields over all records, per top field: " + ", ".join("%s %d" % kv for kv in sorted(keys.items())))
PY
    tail -1 "$HERE/r3_r4_diff.txt" ;;
  summary)
    python3 -I "$HERE/summary.py" "$EX/explain_r4" > "$HERE/summary.txt"; cat "$HERE/summary.txt" ;;
  rowsites)
    python3 -I "$HERE/row_sites.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$W/binds_instr.kzs" \
      --dump-text "$RUN/tx.dump.txt" --key 17975444d83f6589 > "$HERE/row_sites_17975444d83f6589.tsv"; tail -1 "$HERE/row_sites_17975444d83f6589.tsv" ;;
  runtime-refs)
    # the +10 of "references across units" (B 195 -> B' 205): the basis summary lines and the undefined runtime symbols
    # of the instrumented executables of the derived tree
    { echo "# basis.txt of B ($B) and B' ($BP), the link and closure summary lines"
      for d in "$B" "$BP"; do echo "## $d/basis.txt sha256 $(sha256sum < "$d/basis.txt" | cut -c1-64)"; grep -nE '^  (link inputs|link members|module symbols bound|references across units) ' "$d/basis.txt"; done
      echo "# nm -D --undefined-only on the plan's executables of the derived tree (<W>/ws/exe/tx), kz_ symbols"
      for f in core-tx cf/bp.so cf/bplib.so cf/cf.so cf/ci_lab.so cf/sch_lab.so cf/to_lab.so; do
        x="$W/ws/exe/tx/$f"; s=$(nm -D --undefined-only "$x" | awk '{print $NF}' | grep '^kz_' | sort | tr '\n' ' ')
        echo "$f	sha256 $(sha256sum < "$x" | cut -c1-16)	undefined kz_ symbols $(printf '%s' "$s" | wc -w): $s"; done
      echo "# defined kz_rt_ symbols exported by core-tx (dynamic symbol table)"
      nm -D --defined-only "$W/ws/exe/tx/core-tx" | awk '{print $NF}' | grep -E '^kz_rt_(module|obs)$' | sort
    } > "$HERE/runtime_refs.txt"; cat "$HERE/runtime_refs.txt" ;;
  cfe-assert)
    # cfe_assert.so of the derived tree against the original reference build, section by section (no build)
    T=$(mktemp -d "$TMP/cfeassert.XXXXXX") || exit 2
    a="$REF/exe/tx/cf/cfe_assert.so"; b="$W/ws/exe/tx/cf/cfe_assert.so"
    { echo "# reference $a sha256 $(sha256sum < "$a" | cut -c1-64)"; echo "# derived   $b sha256 $(sha256sum < "$b" | cut -c1-64)"
      echo "# rx copy: $(cmp -s "$b" "$W/ws/exe/rx/cf/cfe_assert.so" && echo 'same bytes as tx' || echo 'differs from tx')"
      echo "# in the startup script of the instance: $(grep -c cfe_assert "$RUN/inst/tx/cf/cfe_es_startup.scr") lines"
      echo "# objects of cfe_assert in objects_differ.txt (derived instr vs same-path reference build): $(grep -c cfe_assert "$R/records/obs1-20261116-derived/w/objects_differ.txt")"
      echo "section	size_ref	size_derived	same_bytes"
      for s in $(readelf -SW "$a" | awk '/^ *\[ *[0-9]+\]/{sub(/^ *\[ *[0-9]+\] */,""); print $1}' | grep -v '^NULL$'); do
        objcopy -O binary --only-section="$s" "$a" "$T/a.bin" 2>/dev/null; objcopy -O binary --only-section="$s" "$b" "$T/b.bin" 2>/dev/null
        echo "$s	$(stat -c %s "$T/a.bin")	$(stat -c %s "$T/b.bin")	$(cmp -s "$T/a.bin" "$T/b.bin" && echo yes || echo no)"; done
      echo "# strings (strings -a -n 6) in one file only, after the derived prefixes of path_map.tsv are put back to the reference prefixes"
      strings -a -n 6 "$a" | sort -u > "$T/sa"
      strings -a -n 6 "$b" | python3 -I -c 'import sys
m=[l.rstrip("\n").split("\t") for l in open(sys.argv[1]) if l.strip() and not l.startswith("#")]
m.sort(key=lambda x: -len(x[1]))
for l in sys.stdin:
    for r, d in m:
        l = l.replace(d, r).replace(d.rstrip("/"), r.rstrip("/"))
    sys.stdout.write(l)' "$W/path_map.tsv" | sort -u > "$T/sb"
      echo "only in the reference: $(comm -23 "$T/sa" "$T/sb" | wc -l)"; comm -23 "$T/sa" "$T/sb" | head -20 | sed 's/^/  < /'
      echo "only in the derived (after the map): $(comm -13 "$T/sa" "$T/sb" | wc -l)"; comm -13 "$T/sa" "$T/sb" | head -20 | sed 's/^/  > /'
      echo "# section addresses (readelf -SW) of .text, .rodata, .eh_frame_hdr, .eh_frame: reference, then derived"
      readelf -SW "$a" | grep -E '\] \.(text|rodata|eh_frame_hdr|eh_frame) '; readelf -SW "$b" | grep -E '\] \.(text|rodata|eh_frame_hdr|eh_frame) '
      echo "# .text disassembly (objdump -d --no-show-raw-insn) with every hexadecimal number replaced by H"
      norm() { objdump -d --no-show-raw-insn -j .text "$1" | sed -E 's/0x[0-9a-f]+/H/g; s/\b[0-9a-f]{3,}\b/H/g' | grep -vE 'file format|^$'; }
      norm "$a" > "$T/da"; norm "$b" > "$T/db"
      echo "instruction lines reference $(wc -l < "$T/da"), derived $(wc -l < "$T/db"); differing after the normalisation $(diff "$T/da" "$T/db" | grep -c '^<')"
    } > "$HERE/cfe_assert_compare.txt"; rm -rf "$T"; cat "$HERE/cfe_assert_compare.txt" ;;
  bindings)
    # the binding report again with the unit source sha256 from the B' basis FILE_HASHES (same module options)
    MF=$(sed -n 's/^module options: //p' "$R/records/obs1-20261116-derived/module_flags.txt")
    "$MC/tools/kz-dump/kz-dump" "$BP/basis.kz" | grep -E '^(section FILE_HASHES|  FILE_HASHES\[|  BASIS\[[0-9]+\] name="bundle\.root")' > "$EX/bprime_basis_file_hashes.txt"
    python3 -I "$R/env/tools/plan_bindings.py" --manifest "$P/plan.kzs" --modules "$P/plan.mod" --bindings "$W/binds_instr.kzs" \
      --module-flags "$MF" --file-hashes "$EX/bprime_basis_file_hashes.txt" -o "$HERE/bindings_report_src.tsv"; rc=$?
    grep -E '^(unit|sources)' "$HERE/bindings_report_src.tsv" | cut -f1,2,5,6,7 | head -40; exit $rc ;;
  *) echo "usage: run_fix.sh explain|copy|summary|rowsites|runtime-refs|cfe-assert|bindings" >&2; exit 64 ;;
esac

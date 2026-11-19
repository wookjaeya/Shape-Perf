#!/usr/bin/env bash
# dryrun.sh STEP [BUILD] -- the dry runs of the explanation script revision 4 (no boot, no build).
#   inputs BUILD   facts view (identity lines of the plan's facts through the registry's kz-dump) and the binding lines of
#                  the runtime step's bind check (units of the planned rows only) into obs1/xr4_dry_inputs/BUILD (new)
#   static BUILD   revision 4 on the real static outputs of BUILD (relations, keys and pair file of cz_final_order, plan of
#                  cz_plans_v4_ns2, the five name-space files of the plan) with no run, the D1 branches -> obs1/xr4_dry_static_BUILD
#   rtread NAME    the reader part on the text of the runtime step's test dump runtime/tests/NAME.kzrt (kz-rt-dump of the MC)
#                  with the c1_pre_tx plan -> obs1/xr4_dry_rt_NAME/normal.json; then the judge part on it
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
R="$(cd "$HERE/../.." && pwd)"
O=/home/user/work/redesign/obs1
MC=/home/user/work/merge-check/kz-llvm
EX=$R/env/obs1
VOC=$R/kairos/tests/static/data/reason_vocabulary.txt
step=$1; b=${2:-}
case "$step" in
  inputs)
    D=$O/xr4_dry_inputs/$b; [ ! -e "$D" ] || { echo "$D exists" >&2; exit 2; }; mkdir -p "$D" || exit 2
    F=$O/cz_final_extract/$b/$b/NP/facts.kz
    { echo "facts $F sha256=$(sha256sum < "$F" | cut -c1-64)"
      echo "kz-dump $(sha256sum < "$MC/tools/kz-dump/kz-dump" | cut -c1-64)"
      "$MC/tools/kz-dump/kz-dump" "$F" | head -8
      "$MC/tools/kz-dump/kz-dump" "$F" | grep -E 'name="(extract\.build_identity|extract\.p0_table|extract\.p0_table_scope|p0\.pthread_create)"'
    } > "$D/facts_view.txt"
    cat "$O"/rtb_bind2_$b/binds/*.kzs > "$D/binds_bindcheck.kzs"
    ls "$O"/rtb_bind2_$b/binds/*.kzs | wc -l > "$D/binds_units.txt"
    (cd "$D" && sha256sum facts_view.txt binds_bindcheck.kzs binds_units.txt) ;;
  static)
    OUT=${DRY_OUT:-$O/xr4_dry_static_$b${SFX:-}}; D=$O/xr4_dry_inputs/$b; P=$O/cz_plans_v4_ns2/$b; Z=$O/cz_final_order/$b
    NS=(); for f in $(grep '^input namespace-data ' "$P/plan.side" | sed 's/.* path=//'); do NS+=(--namespaces "$f"); done
    python3 -I "$EX/explain.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$D/binds_bindcheck.kzs" \
      --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" "${NS[@]}" --facts-view "$D/facts_view.txt" \
      --bprime-basis "$O/cz_final_extract/$b/$b/NP/basis.kz" --build-label "$b" --branches "$HERE/out/branches_d1.txt" \
      --label-record "dry run on the static outputs with no run" --vocabulary "$VOC" --out "$OUT" ;;
  rtread)
    OUT=$O/xr4_dry_rt_$b${SFX:-}; [ ! -e "$OUT" ] || { echo "$OUT exists" >&2; exit 2; }; mkdir -p "$OUT" || exit 2
    P=$O/cz_plans_v4_ns2/c1_pre_tx; Z=$O/cz_final_order/c1_pre_tx; D=$O/xr4_dry_inputs/c1_pre_tx
    "$MC/runtime/kz-rt-dump" "$MC/runtime/tests/$b.kzrt" > "$OUT/$b.dump.txt"; echo "kz-rt-dump rc=$?"
    printf '# dry-run stand-in for run.meta (no boot): no manifest, file or stop line\n' > "$OUT/run.meta"
    printf '# dry-run stand-in for a startup script (no boot)\n!\n' > "$OUT/startup.scr"
    { echo "# dry-run stand-in build record: the runtime step's test executable and test modules"
      echo "manifest sha256 $(sha256sum < "$P/plan.kzs" | cut -c1-64)"
      for f in test_rt_objects librt_objects_mod_id.so librt_objects_mod_noid.so; do echo "$f sha256 $(sha256sum < "$MC/runtime/tests/$f" | cut -c1-64)"; done
    } > "$OUT/build_record.txt"
    python3 -I "$R/env/tools/chain_check.py" --plan "$P/plan.kzs" --build-record "$OUT/build_record.txt" --run-meta "$OUT/run.meta" \
      --dump-text "$OUT/$b.dump.txt" --build-root "$MC/runtime/tests" --main test_rt_objects -o "$OUT/chain.txt"; echo "chain_check rc=$?"
    python3 -I "$EX/explain_read.py" --sidecar "$P/plan.side" --manifest "$P/plan.kzs" --bindings "$D/binds_bindcheck.kzs" \
      --relations "$Z/relations.txt" --keys "$Z/keys.txt" --pairs "$Z/pairs.tsv" --facts-view "$D/facts_view.txt" \
      --bprime-basis "$O/cz_final_extract/c1_pre_tx/c1_pre_tx/NP/basis.kz" --build-label c1_pre_tx --branches "$HERE/out/branches_d1.txt" \
      --label-record "dry run on a runtime test dump (no plan of that dump)" --dump-text "$OUT/$b.dump.txt" --run-meta "$OUT/run.meta" \
      --chain "$OUT/chain.txt" --startup-script "$OUT/startup.scr" --normal-out "$OUT/normal.json"; echo "reader rc=$?"
    python3 -I "$EX/explain_judge.py" --normal "$OUT/normal.json" --vocabulary "$VOC" --out "$OUT/judge"; echo "judge rc=$?" ;;
  *) echo "dryrun.sh inputs|static|rtread" >&2; exit 2 ;;
esac

#!/usr/bin/env bash
# run_derived.sh STEP -- the steps of the derived C1 build B' from the generated plan of c1_pre_tx
# (plan/observation_path_results_20261116.md 5.0), each under the space guard, logs into this directory.
#   runtime | prepare | ref | instr | check | bindings | rx | bprime | compare | buildids
# Every output directory is new (the scripts refuse an existing one).  Nothing is booted.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
GS=$R/kairos/instrument/gcc/scripts
GUARD=$R/env/tools/space_guard.sh
NEED=100000000
O=/home/user/work/redesign/obs1
P=$O/plangen/c1_pre_tx
W=$O/c1pre_derived
RT=$O/c1pre_rt/rt
BP=$O/c1pre_bprime
PLUGIN=/home/user/work/c5tools/plugin-build/kzgcc.so
KZB=/home/user/work/merge-check/kz-llvm/tools
PLAN=(--manifest "$P/plan.kzs" --modules "$P/plan.mod" --exes "$P/plan.exe" --path-map "$W/path_map.tsv")
g() { local k=$1; shift; "$GUARD" --need "$NEED" --kind "$k" --log "$HERE/df_$k.log" -- "$@"; }
case "${1:-}" in
  runtime) mkdir -p "$O/c1pre_rt" && g runtime bash "$GS/build_runtime.sh" "$RT" "$P/plan.kzs" "$PLUGIN" > "$HERE/runtime.txt" 2>&1 ;;
  prepare) g prepare bash "$GS/derive_c1.sh" prepare pre 1df262a "$W" > "$HERE/prepare.txt" 2>&1 ;;
  ref)     g derive-ref bash "$GS/derive_plan.sh" c1 build pre 1df262a "$W" ref "${PLAN[@]}" > "$HERE/build_ref.txt" 2>&1 ;;
  instr)   g derive-instr bash "$GS/derive_plan.sh" c1 build pre 1df262a "$W" instr "$PLUGIN" "$RT" "${PLAN[@]}" > "$HERE/build_instr.txt" 2>&1 ;;
  check)   g derive-check bash "$GS/derive_plan.sh" c1 check pre 1df262a "$W" "${PLAN[@]}" > "$HERE/check.txt" 2>&1 ;;
  bindings)
    # the module options as derive_c1.sh computed them (the table through the map), found verbatim in the build record
    MF=$(bash -c '. "$1/derive_args.sh"; shift; derive_plan_args "$@"; derive_module_flags /unused' _ "$GS" "${PLAN[@]}") || exit 1
    { echo "module options: $MF"
      grep -F -- "$MF" "$W/build_instr.record" > /dev/null && echo "in build_instr.record D3 line: yes" || echo "in build_instr.record D3 line: NO"; } > "$HERE/module_flags.txt"
    python3 -I "$R/env/tools/plan_bindings.py" --manifest "$P/plan.kzs" --modules "$P/plan.mod" --bindings "$W/binds_instr.kzs" \
      --module-flags "$MF" -o "$HERE/bindings_report.tsv" ;;
  rx)
    # rx is built by the same mission-install from the same arch objects; its hook calls against the plan's group
    cd "$W" || exit 2
    mapfile -t RX < <(find ws/exe/rx -type f | LC_ALL=C sort | while IFS= read -r f; do [ "$(head -c4 "$f" | od -An -c | tr -d ' \n')" = '177ELF' ] && echo "$f"; done)
    printf '%s\n' "${RX[@]}" > "$HERE/rx_files.txt"
    bash "$GS/bincheck.sh" "${RX[@]}" > "$HERE/bincheck_rx.txt"
    { echo "rx ELF files: ${#RX[@]}; hook calls: $(wc -l < "$HERE/bincheck_rx.txt"); plan group (basis): $(wc -l < bincheck_basis.txt)"
      echo "same (function, hook, id) lists as the plan's group: $(cmp -s <(awk '{print $2, $3, $4}' bincheck_basis.txt | sort) <(awk '{print $2, $3, $4}' "$HERE/bincheck_rx.txt" | sort) && echo yes || echo NO)"
      echo "rx files equal to the tx file of the same name (sha256):"
      for f in "${RX[@]}"; do t=ws/exe/tx/${f#ws/exe/rx/}; [ -e "$t" ] || continue; echo "  ${f#ws/} $( [ "$(sha256sum < "$f")" = "$(sha256sum < "$t")" ] && echo same || echo differs) ${t#ws/}"; done
    } > "$HERE/rx_check.txt"; cat "$HERE/rx_check.txt" ;;
  bprime)  KZB=$KZB ONE_BUILD_NEED=$NEED bash "$R/env/redesign/one_build.sh" c1_pre_tx "$BP" --path-map "$W/path_map.tsv" > "$HERE/bprime_run.txt" 2>&1 ;;
  compare) python3 -I "$R/env/redesign/compare_derived.py" "$KZB/kz-dump/kz-dump" "$HERE/compare.tsv" "$O/plangen_bref/c1_pre_tx/NP" "$BP/c1_pre_tx/NP" \
             --path-map "$W/path_map.tsv" --declared "$HERE/declared_diff.tsv" ;;
  buildids)
    cd "$W/ws" || exit 2
    for f in $(find exe -type f | LC_ALL=C sort); do
      [ "$(head -c4 "$f" | od -An -c | tr -d ' \n')" = '177ELF' ] || continue
      printf '%s\t%s\t%s\n' "$f" "$(sha256sum < "$f" | cut -c1-64)" "$(readelf -n "$f" 2> /dev/null | sed -n 's/.*Build ID: //p' | head -1)"
    done > "$HERE/exe_buildids.tsv"; wc -l < "$HERE/exe_buildids.tsv" ;;
  *) echo "usage: run_derived.sh runtime|prepare|ref|instr|check|bindings|rx|bprime|compare|buildids" >&2; exit 64 ;;
esac

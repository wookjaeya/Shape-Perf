#!/usr/bin/env bash
# run.sh STEP BUILD -- the steps of the C1 cycle preparation (results section 25) for BUILD c1_pre_tx | c1_post_tx,
# each under the space guard (env/tools/space_guard.sh, floor 314,572,800 B), logs into ../guard, outputs into new
# directories /home/user/work/redesign/obs1/c1p_<what>_<b> (the scripts refuse an existing one).  Nothing is booted.
#   budgets                the budget lines of the two plans (sidecar lines 15-22) -> ../out/plan_budgets.txt
#   regen      BUILD       plan_regen.sh with the MC kz-static on the plan's args file -> obs1/c1p_regen/<b>
#   runtime    BUILD       build_runtime.sh (kairos/runtime at HEAD, kzrt 0.5.0) with the plan's manifest and kzgcc 0.4
#   prepare    BUILD       derive_c1.sh prepare <cfg> 1df262a <W>
#   ref        BUILD       derive_plan.sh c1 build ... ref (the reference build at the derived path; removed after hashes)
#   instr      BUILD       derive_plan.sh c1 build ... instr <kzgcc 0.4> <rt>
#   check      BUILD       derive_plan.sh c1 check ...
#   rx         BUILD       bincheck of exe/rx against the plan's group (tx)
#   buildids   BUILD       sha256 and GNU build-id of every installed ELF file
#   bprime     BUILD       one_build.sh <b> <B'> --path-map <W>/path_map.tsv with every extraction option of
#                          cz_final_extract (the facts the plan was generated from)
#   bindings   BUILD       plan_bindings.py with --file-hashes (kz-dump text of the B' basis)
#   compare    BUILD [N]   compare_derived.py B = cz_final_extract/<b>/<b>/NP, B' = the bprime output, against
#                          ../derived/declared_diff.tsv (N names an addendum list declared_diff_<N>.tsv); output
#                          ../derived/compare_<b>[_<N>].tsv, never overwritten
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
D="$(cd "$HERE/.." && pwd)"
R="$(cd "$D/../.." && pwd)"
GS=$R/kairos/instrument/gcc/scripts
GUARD=$R/env/tools/space_guard.sh
O=/home/user/work/redesign/obs1
MC=/home/user/work/merge-check/kz-llvm
KS=$MC/tools/kz-static/kz-static
PLUGIN=/home/user/work/instr/kzgcc-0.4/plugin-build/kzgcc.so
SEM=$R/kairos/semantics
STEP=${1:?usage: run.sh STEP [BUILD]}; B=${2:-}
case "$B" in
  c1_pre_tx) CFG=pre; S=pre ;;
  c1_post_tx) CFG=post; S=post ;;
  '') [ "$STEP" = budgets ] || { echo "run.sh: BUILD is c1_pre_tx or c1_post_tx" >&2; exit 64; } ;;
  *) echo "run.sh: BUILD is c1_pre_tx or c1_post_tx" >&2; exit 64 ;;
esac
S=${S:-}; P=$O/cz_plans_v4_ns2/$B
W=$O/c1p_${S}_derived
RTD=$O/c1p_${S}_rt
RT=$RTD/rt
BP=$O/c1p_${S}_bprime
BREF=$O/cz_final_extract/$B/$B/NP
PLAN=(--manifest "$P/plan.kzs" --modules "$P/plan.mod" --exes "$P/plan.exe" --path-map "$W/path_map.tsv")
g() { local k=$1 n=$2; shift 2; "$GUARD" --need "$n" --kind "$k" --log "$D/guard/${k}_$S.log" -- "$@"; }
OUTD=$D/derived/${S:-plans}
mkdir -p "$OUTD"
case "$STEP" in
  budgets)
    for b in c1_pre_tx c1_post_tx; do
      echo "== $b: $O/cz_plans_v4_ns2/$b/plan.side sha256 $(sha256sum < "$O/cz_plans_v4_ns2/$b/plan.side" | cut -c1-64)"
      sed -n '2,3p;15,22p' "$O/cz_plans_v4_ns2/$b/plan.side"
      echo "-- planned rows (sidecar row lines with state planned): tier $(grep -c '^row .* state=planned .*' "$O/cz_plans_v4_ns2/$b/plan.side")"
      grep '^row .* control-state=planned' "$O/cz_plans_v4_ns2/$b/plan.side" | cut -c1-200 | sed 's/^/  control: /'
    done > "$D/out/plan_budgets.txt"; cat "$D/out/plan_budgets.txt" | cut -c1-160 ;;
  regen)
    [ ! -e "$O/c1p_regen/$B" ] || { echo "refusing: $O/c1p_regen/$B exists" >&2; exit 2; }
    mkdir -p "$O/c1p_regen/$B"
    g regen 10000000 bash "$R/env/tools/plan_regen.sh" "$KS" "$P/args.txt" "$O/c1p_regen/$B" > "$OUTD/regen.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$OUTD/regen.txt"; cat "$OUTD/regen.txt"; exit $rc ;;
  runtime)
    [ ! -e "$RTD" ] || { echo "refusing: $RTD exists" >&2; exit 2; }
    mkdir -p "$RTD" && g runtime 100000000 bash "$GS/build_runtime.sh" "$RT" "$P/plan.kzs" "$PLUGIN" > "$OUTD/runtime.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$OUTD/runtime.txt"; cat "$OUTD/runtime.txt"; exit $rc ;;
  prepare) g prepare 100000000 bash "$GS/derive_c1.sh" prepare "$CFG" 1df262a "$W" > "$OUTD/prepare.txt" 2>&1; rc=$?; echo "rc=$rc" >> "$OUTD/prepare.txt"; cat "$OUTD/prepare.txt"; exit $rc ;;
  ref)   g derive-ref 100000000 bash "$GS/derive_plan.sh" c1 build "$CFG" 1df262a "$W" ref "${PLAN[@]}" > "$OUTD/build_ref.txt" 2>&1; rc=$?; echo "rc=$rc" >> "$OUTD/build_ref.txt"; tail -5 "$OUTD/build_ref.txt"; exit $rc ;;
  instr) g derive-instr 100000000 bash "$GS/derive_plan.sh" c1 build "$CFG" 1df262a "$W" instr "$PLUGIN" "$RT" "${PLAN[@]}" > "$OUTD/build_instr.txt" 2>&1; rc=$?; echo "rc=$rc" >> "$OUTD/build_instr.txt"; tail -5 "$OUTD/build_instr.txt"; exit $rc ;;
  check) g derive-check 100000000 bash "$GS/derive_plan.sh" c1 check "$CFG" 1df262a "$W" "${PLAN[@]}" > "$OUTD/check.txt" 2>&1; rc=$?; echo "rc=$rc" >> "$OUTD/check.txt"; cat "$OUTD/check.txt" | head -60; exit $rc ;;
  rx)
    cd "$W" || exit 2
    mapfile -t RX < <(find ws/exe/rx -type f | LC_ALL=C sort | while IFS= read -r f; do [ "$(head -c4 "$f" | od -An -c | tr -d ' \n')" = '177ELF' ] && echo "$f"; done)
    printf '%s\n' "${RX[@]}" > "$OUTD/rx_files.txt"
    bash "$GS/bincheck.sh" "${RX[@]}" > "$OUTD/bincheck_rx.txt"
    { echo "rx ELF files: ${#RX[@]}; hook calls: $(wc -l < "$OUTD/bincheck_rx.txt"); plan group (basis): $(wc -l < bincheck_basis.txt)"
      echo "same (function, hook, id) lists as the plan's group: $(cmp -s <(awk '{print $2, $3, $4}' bincheck_basis.txt | sort) <(awk '{print $2, $3, $4}' "$OUTD/bincheck_rx.txt" | sort) && echo yes || echo NO)"
      echo "rx files equal to the tx file of the same name (sha256):"
      for f in "${RX[@]}"; do t=ws/exe/tx/${f#ws/exe/rx/}; [ -e "$t" ] || continue; echo "  ${f#ws/} $( [ "$(sha256sum < "$f")" = "$(sha256sum < "$t")" ] && echo same || echo differs) ${t#ws/}"; done
    } > "$OUTD/rx_check.txt"; cat "$OUTD/rx_check.txt" ;;
  buildids)
    cd "$W/ws" || exit 2
    for f in $(find exe -type f | LC_ALL=C sort); do
      [ "$(head -c4 "$f" | od -An -c | tr -d ' \n')" = '177ELF' ] || continue
      printf '%s\t%s\t%s\n' "$f" "$(sha256sum < "$f" | cut -c1-64)" "$(readelf -n "$f" 2> /dev/null | sed -n 's/.*Build ID: //p' | head -1)"
    done > "$OUTD/exe_buildids.tsv"; wc -l < "$OUTD/exe_buildids.tsv" ;;
  bprime)
    X=--object-access
    for f in framework_semantics_cfe-6.4.1.txt framework_semantics_cfe-6.4.2.txt framework_semantics_cfe-6cccb3f.txt framework_semantics_cfe-7.0.1.txt \
             framework_semantics_osal-4.1.1.txt framework_semantics_osal-7.0.1.txt framework_semantics_osal-f34759d.txt p0_declarations_glibc-2.39-headers.txt; do X="$X --semantics $SEM/$f"; done
    X="$X --result-flow --loop-defs --lock-handles"
    KZB=$MC/tools ONE_BUILD_NEED=60000000 ONE_BUILD_EXTRACT_EXTRA="$X" bash "$R/env/redesign/one_build.sh" "$B" "$BP" --path-map "$W/path_map.tsv" > "$OUTD/bprime_run.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$OUTD/bprime_run.txt"; cp "$BP/$B/NP/space_guard.log" "$D/guard/bprime_$S.log" 2> /dev/null; tail -8 "$OUTD/bprime_run.txt"; exit $rc ;;
  bindings)
    MF=$(bash -c '. "$1/derive_args.sh"; shift; derive_plan_args "$@"; derive_module_flags /unused' _ "$GS" "${PLAN[@]}") || exit 1
    { echo "module options: $MF"
      grep -F -- "$MF" "$W/build_instr.record" > /dev/null && echo "in build_instr.record D3 line: yes" || echo "in build_instr.record D3 line: NO"; } > "$OUTD/module_flags.txt"
    T=$(mktemp -d "${TMPDIR:-/tmp}/c1p_fh.XXXXXX") || exit 2
    "$MC/tools/kz-dump/kz-dump" "$BP/$B/NP/basis.kz" > "$T/basis_dump.txt" 2>&1 || { rm -rf "$T"; exit 2; }
    python3 -I "$R/env/tools/plan_bindings.py" --manifest "$P/plan.kzs" --modules "$P/plan.mod" --bindings "$W/binds_instr.kzs" \
      --module-flags "$MF" --file-hashes "$T/basis_dump.txt" -o "$OUTD/bindings_report.tsv"; rc=$?
    rm -rf "$T"; echo "plan_bindings rc=$rc"; tail -12 "$OUTD/bindings_report.tsv"; exit $rc ;;
  compare)
    N=${3:-}; L=$D/derived/declared_diff${N:+_$N}.tsv; OF=$D/derived/compare_${B}${N:+_$N}.tsv
    [ -f "$L" ] || { echo "no list $L" >&2; exit 2; }
    [ ! -e "$OF" ] || { echo "refusing: $OF exists" >&2; exit 2; }
    g compare 10000000 python3 -I "$R/env/redesign/compare_derived.py" "$MC/tools/kz-dump/kz-dump" "$OF" "$BREF" "$BP/$B/NP" \
      --path-map "$W/path_map.tsv" --declared "$L" > "$OUTD/compare${N:+_$N}.txt" 2>&1; rc=$?
    echo "rc=$rc" >> "$OUTD/compare${N:+_$N}.txt"; tail -20 "$OUTD/compare${N:+_$N}.txt"; exit $rc ;;
  *) echo "usage: run.sh budgets | regen|runtime|prepare|ref|instr|check|rx|buildids|bprime|bindings|compare BUILD" >&2; exit 64 ;;
esac

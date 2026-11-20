#!/usr/bin/env bash
# extract_baseline.sh STEP [OUT] -- what the merged kz-static yields for the C1, R2 and C5 reference builds without the
# case profiles (redesign step 2 starting point, plan/extraction_design_20261030.md).  Static analysis only: no cFS is
# booted, no engine code is changed, no file under kairos/ is written.
#
#   STEP   inputs   write the bare profile and the recipes into OUT/inputs (and record their hashes)
#          run      basis + extract + show for every (build, variant) below -> OUT/<build>/<variant>/
#          extra    the variants added after the verification of 2026-10-30 (V0w, VP, V3a below), same layout
#          compare  the case variant against the existing closures of the case drivers (show text, header lines
#                   removed), V0 against V0w and V0r
#          count    extract_baseline_count.py over the show texts -> OUT/count/
#          all      the five in this order
#   OUT    default /home/user/work/redesign/extract_baseline
#
# Builds (all reference builds, no derived build):
#   c1_pre_tx c1_pre_rx c1_post_tx c1_post_rx   procrace/c1_build/<pre|post>/1df262a (candidate 1df262a only), recipe
#                                               c1_build/kz/<cfg>_1df262a/closure[_rx].kzb (written by env/c1/kz_run.sh
#                                               recipe(): link and binary of the build, seven module names fixed in
#                                               that script (cfe_assert, which the scripts do not load, included;
#                                               tinycbor, which they load, without a link command), the startup-script
#                                               block); V0w uses a recipe written by rule instead
#   r2_core                                     procrace/cfs_ref/cFS/build-native_std core-cpu1; recipe = profiles/
#                                               cfs-v7.0.1/basis/core-cpu1.kzb without its `generated`, `table` and
#                                               `declared` lines (hand-chosen basis items; see inputs)
#   r2_all                                      the same build, the executable plus every module the startup script
#                                               loads and the build linked (recipe written by rule in `inputs`)
#   c5_641                                      cFE 6.4.1 reference build (c5tools/build/641/keep, E-ST compile database
#                                               c5tools/build/641/est/compile_commands.json), recipe
#                                               profiles/c5-cfe-6.4.x/basis/core-linux.kzb as it is (DRAFT; its module,
#                                               relocatable and front_end_diagnostic lines written by a person); V0w
#                                               uses a recipe written by rule instead
# Variants:
#   V0  bare: a profile of two statements (`phi 1; profile "...";`, the least the loader accepts) and no command-line
#       vocabulary.  Nothing names a field, an operation, a role, a spawn, a lock, a delay or a terminal.
#   V1  V0 + the vocabulary G0 on the command line (spawn OS_TaskCreate, CFE_ES_CreateChildTask; delay OS_TaskDelay;
#       lock/unlock CFE_ES_LockSharedData/CFE_ES_UnlockSharedData; terminal CFE_ES_ExitApp, CFE_PSP_Restart,
#       CFE_PSP_Panic, abort).  G0 is taken from the case drivers and profiles: it contains the four operations of the
#       case drivers (CASEOPS) word for word, and the other five are names the case profiles use.  It is therefore a
#       case-derived layer, not the thread-library table P0 (VP below is that).  g0_check.tsv only says whether each name
#       has a body in the closure; it checks no semantics, and G0 has no version label.
#   V2  the case inputs as the case drivers give them (env/c1/kz_run_c1.sh, env/r2/replay_w6.sh closures,
#       env/c5/replay_c5.sh bprime): case profile directories, case recipe, the four command-line operations and, for
#       C1, the seven --target fields.  Run with the same merged kz-static so that V2 - V0 is the profile's effect.
#   V0p V0 + one spawn seed, the thread-library primitive pthread_create (what the existing spawn analysis derives from
#       that seed alone)
#   VP  V0 + the thread-library and C-library primitives of the P0 table only (P0OPS: spawn pthread_create; lock/unlock
#       pthread_mutex_lock/pthread_mutex_unlock; delay clock_nanosleep, nanosleep; terminal pthread_exit, exit, abort),
#       no cFE/OSAL name: what the existing extractor gives when no case-derived name is on the command line (it does
#       not carry an operation through wrappers, see plan/extraction_design_20261030.md F7/F8)
#   V0w V0 with a recipe written by rule from the build (C1 tx/rx, C5): the executable's link command and binary, one
#       module line per startup-script line whose module has a link command (C1: native/default/apps/<m>/CMakeFiles/
#       <m>.dir/link.txt, the rule of r2_all; C5: the <file>.link.txt the build-log converter wrote for the script's
#       file), C5's relocatable lines from the link inputs of the core whose own link command is `ld ... -r`.  The
#       startup_script block (the loader semantics L0 with its cites) and C5's two front_end_diagnostic lines (PI D7,
#       an analysis-only deviation) are copied from the case recipe; nothing else is.  Does the hand-chosen module
#       list change the facts?
#   V3a r2_core only: V3 + the target CFE_ES_AppRecord_t.AppState (the field the CFE_ES_MainTaskSyncDelay predicate
#       reads, which rule R-T1 does not select): does adding it give the await rule?
#   V3  V1 + rule-derived extraction targets: rule R-T1 = every Type.field that reaches a call argument in the V0 facts
#       of the same build (`arg i:symbolic T.f` or `arg i:addr-of-field ... of T.f`).  The list is written by this
#       driver (targets_rule.txt), not chosen by a person; kz-static still counts each --target as hand-entered,
#       which is a property of its report, not of where the list came from.
#   r2_core also has V0r: V0 with the original recipe core-cpu1.kzb (does the recipe's hand-chosen basis items change
#   the facts?).  The C1 rx builds have V0 and V1 only.
set -uo pipefail
STEP=${1:?inputs|run|extra|compare|count|all}
OUT=${2:-/home/user/work/redesign/extract_baseline}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
R="$(cd "$HERE/../.." && pwd)"
K=$R/kairos
KZB=${KZB:-/home/user/work/merge-check/kz-llvm/tools}
KS=$KZB/kz-static/kz-static
KZD=$KZB/kz-dump/kz-dump
C1=/home/user/work/procrace/c1_build
REF=/home/user/work/procrace/cfs_ref/cFS
C5T=/home/user/work/c5tools/build/641
C5TREE=$C5T/keep/cFE-6.4.1-OSS-release
IN=$OUT/inputs
[ -e /home/user/work/.campaign_running ] && { echo "a campaign is running (/home/user/work/.campaign_running)" >&2; exit 3; }
mkdir -p "$OUT"

G0=(--spawn-op OS_TaskCreate --spawn-op CFE_ES_CreateChildTask --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData
    --unlock-op CFE_ES_UnlockSharedData --terminal-op CFE_ES_ExitApp --terminal-op CFE_PSP_Restart --terminal-op CFE_PSP_Panic
    --terminal-op abort)
CASEOPS=(--delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask)
P0OPS=(--spawn-op pthread_create --lock-op pthread_mutex_lock --unlock-op pthread_mutex_unlock --delay-op clock_nanosleep
       --delay-op nanosleep --terminal-op pthread_exit --terminal-op exit --terminal-op abort)
C1TG=(--target OS_common_record_t.name_entry --target OS_common_record_t.active_id --target OS_count_sem_internal_record_t.obj_name
      --target CF_Channel_t.sem_id --target CF_ChannelConfig_t.sem_name --target BP_Throttle_t.Name --target BP_Throttle_t.Sem)

inputs() {
  mkdir -p "$IN/bare"
  # the least profile the loader accepts: an empty file is refused ("expected 'phi'"), `phi 1;` alone is refused ("the
  # profile has no name"); --profile itself is required by basis and extract (tools/kz-static/cmd_*.cpp)
  printf 'phi 1;\nprofile "extract-baseline-bare";\n' > "$IN/bare/00_profile.kzp"
  # r2_core: the recipe of the case driver without the lines a person chose (generated files, tables, the declared claim)
  grep -vE '^(generated|table|declared) ' "$K/profiles/cfs-v7.0.1/basis/core-cpu1.kzb" > "$IN/r2_core_min.kzb"
  # r2_all: the executable and every module the startup script names that the build linked.  The startup-script block is
  # that of core-cpu1.kzb plus the module rules of this tree (lines cited below, read in this tree).
  local scr=$REF/build-native_std/exe/cpu1/cf/cfe_es_startup.scr
  {
    echo "kzb 1;"
    echo "name \"core-cpu1 of the native build with the modules of its startup script (rule-written, extract_baseline.sh)\";"
    echo "link \"native/default_cpu1/cpu1/CMakeFiles/core-cpu1.dir/link.txt\";"
    echo "binary \"exe/cpu1/core-cpu1\";"
    sed -n 's/^[[:space:]]*CFE_\(LIB\|APP\),[[:space:]]*\([A-Za-z0-9_]*\),.*/\2/p' "$scr" | while read -r m; do
      l=native/default_cpu1/apps/$m/CMakeFiles/$m.dir/link.txt
      [ -f "$REF/build-native_std/$l" ] && echo "module \"$m\" link \"$l\";" || echo "# no link command for $m ($l)"
    done
    cat <<'EOF'
startup_script {
  reset POWERON;
  rule PROCESSOR volatile nonvolatile;
  rule POWERON nonvolatile;
  file volatile const "CFE_PLATFORM_ES_VOLATILE_STARTUP_FILE" in "cfe/modules/es/fsw/src/cfe_es_apps.c";
  file nonvolatile const "CFE_PLATFORM_ES_NONVOL_STARTUP_FILE" in "cfe/modules/es/fsw/src/cfe_es_apps.c";
  mount "/cf" "exe/cpu1/cf";
  entry library "CFE_LIB" global;
  entry app "CFE_APP" local;
  module_file field 1 path const "CFE_PLATFORM_ES_NONVOL_DISK_MOUNT_STRING" extension const "CFE_DEFAULT_MODULE_EXTENSION" in "cfe/cmake/target/src/target_config.c";
  cite "cfe/modules/es/fsw/src/cfe_es_apps.c" 111 200 note "CFE_ES_StartApplications: the file that is opened for each reset type";
  cite "psp/fsw/pc-linux/src/cfe_psp_start.c" 374 383 note "the PSP maps /cf to ./cf";
  cite "psp/fsw/pc-linux/src/cfe_psp_start.c" 449 449 note "the PSP passes the non-volatile startup file name to the cFE entry point";
  cite "cfe/modules/es/fsw/src/cfe_es_apps.c" 465 495 note "apps are loaded with local symbols, libraries with global symbols";
  cite "osal/src/os/portable/os-impl-posix-dl-loader.c" 72 96 note "OS_ModuleLoad_Impl: RTLD_NOW, RTLD_GLOBAL or RTLD_LOCAL";
  cite "cfe/modules/fs/fsw/src/cfe_fs_api.c" 53 110 note "the default path and extension of a dynamic module";
  cite "cfe/cmake/target/src/target_config.c" 50 52 note "the default module extension of the target configuration";
  cite "cfe/cmake/target/src/target_config.c" 223 225 note "the non-volatile mount point and startup file of the target configuration";
}
EOF
  } > "$IN/r2_all.kzb"
  for c in pre post; do
    cp "$C1/kz/${c}_1df262a/closure.kzb" "$IN/c1_${c}_tx.kzb"
    cp "$C1/kz/${c}_1df262a/closure_rx.kzb" "$IN/c1_${c}_rx.kzb"
  done
  cp "$K/profiles/c5-cfe-6.4.x/basis/core-linux.kzb" "$IN/c5_641.kzb"
  cp "$K/profiles/cfs-v7.0.1/basis/core-cpu1.kzb" "$IN/r2_core_case.kzb"
  # V0w: the module (and C5 relocatable) lines written by rule from the build; the startup_script block (L0) and the
  # front_end_diagnostic lines (PI D7) copied from the case recipe
  for c in pre post; do
    for inst in tx rx; do
      local B=$C1/$c/1df262a/build scr=$C1/$c/1df262a/exe/$inst/cf/cfe_es_startup.scr src=$C1/kz/${c}_1df262a/closure.kzb
      [ $inst = rx ] && src=$C1/kz/${c}_1df262a/closure_rx.kzb
      {
        echo "kzb 1;"
        echo "name \"core-$inst of the C1 build $c/1df262a with the modules of its startup script (rule-written, extract_baseline.sh)\";"
        echo "link \"native/default/$inst/CMakeFiles/core-$inst.dir/link.txt\";"
        echo "binary \"../exe/$inst/core-$inst\";"
        sed -n 's/^[[:space:]]*CFE_\(LIB\|APP\),[[:space:]]*\([A-Za-z0-9_]*\),.*/\2/p' "$scr" | while read -r m; do
          l=native/default/apps/$m/CMakeFiles/$m.dir/link.txt
          [ -f "$B/$l" ] && echo "module \"$m\" link \"$l\";" || echo "# no link command for $m ($l)"
        done
        sed -n '/^startup_script {/,/^}/p' "$src"
      } > "$IN/c1_${c}_${inst}_rule.kzb"
    done
  done
  local CB=$C5TREE/build/cpu1
  {
    echo "kzb 1;"
    echo "name \"core-linux.bin of the cFE 6.4.1 reference build with the modules of its startup script (rule-written, extract_baseline.sh)\";"
    echo "link \"cfe/link/core-linux.bin.link.txt\";"
    echo "binary \"exe/core-linux.bin\";"
    # a link input of the core whose own link command (written by the build-log converter) is a relocatable link
    grep -oE '\.\./[A-Za-z0-9_/]+\.o' "$CB/cfe/link/core-linux.bin.link.txt" | sed 's|^\.\./|cfe/|' | while read -r o; do
      [ -f "$CB/$o.link.txt" ] && grep -qE '(^|[[:space:]])-r([[:space:]]|$)' "$CB/$o.link.txt" && echo "relocatable \"$o\" link \"$o.link.txt\";"
    done
    # a startup-script line names the module by its virtual path; its link command file is the one of that file name
    sed -n 's/^[[:space:]]*CFE_\(LIB\|APP\),[[:space:]]*\([^,[:space:]]*\),.*/\2/p' "$CB/exe/cf/apps/cfe_es_startup.scr" | while read -r m; do
      l=$(cd "$CB" && find . -name "$(basename "$m").link.txt" | sed 's|^\./||' | sort | head -1)
      [ -n "$l" ] && echo "module \"$m\" link \"$l\";" || echo "# no link command for $m"
    done
    grep '^front_end_diagnostic ' "$K/profiles/c5-cfe-6.4.x/basis/core-linux.kzb"
    sed -n '/^startup_script {/,/^}/p' "$K/profiles/c5-cfe-6.4.x/basis/core-linux.kzb"
  } > "$IN/c5_641_rule.kzb"
  ( cd "$IN" && sha256sum bare/00_profile.kzp *.kzb "$KS" "$C1/kz/"{pre,post}_1df262a/closure{,_rx}.kzb \
      "$K/profiles/c5-cfe-6.4.x/basis/core-linux.kzb" "$K/profiles/cfs-v7.0.1/basis/core-cpu1.kzb" > sha256.txt )
  {
    echo "# extract_baseline.sh inputs $(date -u +%FT%TZ)"
    echo "repo $(git -C "$R" rev-parse HEAD) kairos_uncommitted_lines=$(git -C "$R" diff -- kairos | wc -l)"
    echo "kz-static $KS"; "$KS" --identity
    echo "## r2_core_min.kzb = core-cpu1.kzb minus:"; grep -nE '^(generated|table|declared) ' "$K/profiles/cfs-v7.0.1/basis/core-cpu1.kzb"
    echo "## r2_all.kzb module lines: $(grep -c '^module ' "$IN/r2_all.kzb"), without a link command: $(grep -c '^# no link' "$IN/r2_all.kzb")"
    for f in "$IN"/c1_*_rule.kzb "$IN/c5_641_rule.kzb"; do
      echo "## $(basename "$f") module lines: $(grep -c '^module ' "$f"), relocatable lines: $(grep -c '^relocatable ' "$f"), without a link command: $(grep -c '^# no link' "$f"); module names: $(sed -n 's/^module "\([^"]*\)".*/\1/p' "$f" | tr '\n' ' ')"
    done
    # what the loader accepts (audit A1-A2): no --profile, an empty file, `phi 1;` alone, and the two-statement profile,
    # each on a fixture of the static tests (one C file, no compile database)
    mkdir -p "$IN/probe/empty" "$IN/probe/phi_only"
    : > "$IN/probe/empty/00_profile.kzp"; printf 'phi 1;\n' > "$IN/probe/phi_only/00_profile.kzp"
    local fx=$K/tests/static/fixtures/spawn.c
    echo "## loader probe on $fx"
    echo "no --profile: $("$KS" extract --source "$fx" -o "$IN/probe/none.kz" -- -std=c99 2>&1 | head -1) (rc=$?)"
    for p in empty phi_only; do
      echo "$p: $("$KS" extract --profile "$IN/probe/$p" --source "$fx" -o "$IN/probe/$p.kz" -- -std=c99 2>&1 | head -1)"
    done
    echo "bare: $("$KS" extract --profile "$IN/bare" --source "$fx" -o "$IN/probe/bare.kz" -- -std=c99 2>&1 | head -1)"
  } > "$IN/inputs.record"
  cat "$IN/inputs.record"
}

# one (build, variant): basis, extract, show.  Arguments: build variant compile_db build_dir bundle_root recipe -- profile
# and extract arguments (the profile arguments are given to both commands, the rest to extract only)
one() {
  local b=$1 v=$2 cdb=$3 bd=$4 br=$5 rc=$6; shift 6
  local prof=() ext=() sep=0
  for a in "$@"; do [ "$a" = "--" ] && { sep=1; continue; }; [ $sep = 0 ] && prof+=("$a") || ext+=("$a"); done
  local D=$OUT/$b/$v; mkdir -p "$D"
  local t0=$(date +%s.%N)
  "$KS" basis "${prof[@]}" --compile-db "$cdb" --build-dir "$bd" --bundle-root "$br" --recipe "$rc" --text -o "$D/basis.kz" > "$D/basis.txt" 2>&1; local x=$?
  local t1=$(date +%s.%N)
  "$KS" extract "${prof[@]}" --basis "$D/basis.kz" "${ext[@]}" -o "$D/facts.kz" > "$D/extract.txt" 2>&1; local y=$?
  local t2=$(date +%s.%N)
  "$KS" show "$D/facts.kz" > "$D/facts.txt" 2>&1; local z=$?
  "$KZD" "$D/basis.kz" > "$D/basis.dump" 2>&1   # the unit table (TUS: source, member module) for the count step
  printf '%s\n' "profile: ${prof[*]}" "extract args: ${ext[*]}" "compile db: $cdb" "build dir: $bd" "bundle root: $br" "recipe: $rc" > "$D/args.txt"
  ( cd "$D" && sha256sum basis.kz facts.kz "$cdb" "$rc" "$KS" > sha256.txt )
  printf '%s %s basis=%d extract=%d show=%d basis_s=%.1f extract_s=%.1f\n' "$b" "$v" $x $y $z "$(echo "$t1 - $t0" | bc)" "$(echo "$t2 - $t1" | bc)" | tee -a "$OUT/run.log"
}

# rule R-T1 over the V0 facts of a build -> the --target arguments (one per line in targets_rule.txt)
rt1() {
  local f=$OUT/$1/V0/facts.txt t=$OUT/$1/targets_rule.txt
  { grep -oE '^      arg [0-9]+:symbolic [A-Za-z_][A-Za-z_0-9]*\.[A-Za-z_][A-Za-z_0-9]*' "$f" | awk '{print $3}'
    grep -oE '^      arg [0-9]+:addr-of-field .* of [A-Za-z_][A-Za-z_0-9]*\.[A-Za-z_][A-Za-z_0-9]*' "$f" | awk '{print $NF}'; } | sort -u > "$t"
  sed 's/^/--target /' "$t" | tr '\n' ' '
}

run() {
  [ -f "$IN/bare/00_profile.kzp" ] || inputs > /dev/null
  : > "$OUT/run.log"
  local BARE=(--profile "$IN/bare")
  local P1=$K/profiles/c1-bp-b051eb0
  for c in pre post; do
    local rev; [ $c = pre ] && rev=281a941 || rev=833fdbb
    local B=$C1/$c/1df262a/build CDB=$C1/$c/1df262a/build/native/default/compile_commands.json BR=$C1/bundle_1df262a
    local CPROF=(--profile "$P1" --profile "$P1/cf-$rev" --profile "$P1/bundle-1df262a" --profile "$P1/rspec-$rev")
    for inst in tx rx; do
      one c1_${c}_$inst V0 "$CDB" "$B" "$BR" "$IN/c1_${c}_$inst.kzb" "${BARE[@]}" --
      one c1_${c}_$inst V1 "$CDB" "$B" "$BR" "$IN/c1_${c}_$inst.kzb" "${BARE[@]}" -- "${G0[@]}"
    done
    one c1_${c}_tx V2 "$CDB" "$B" "$BR" "$IN/c1_${c}_tx.kzb" "${CPROF[@]}" -- "${CASEOPS[@]}" "${C1TG[@]}"
    one c1_${c}_tx V0p "$CDB" "$B" "$BR" "$IN/c1_${c}_tx.kzb" "${BARE[@]}" -- --spawn-op pthread_create
    one c1_${c}_tx V3 "$CDB" "$B" "$BR" "$IN/c1_${c}_tx.kzb" "${BARE[@]}" -- "${G0[@]}" $(rt1 c1_${c}_tx)
  done
  local RB=$REF/build-native_std RCDB=$REF/build-native_std/native/default_cpu1/compile_commands.json
  one r2_core V0 "$RCDB" "$RB" "$REF" "$IN/r2_core_min.kzb" "${BARE[@]}" --
  one r2_core V0r "$RCDB" "$RB" "$REF" "$IN/r2_core_case.kzb" "${BARE[@]}" --
  one r2_core V1 "$RCDB" "$RB" "$REF" "$IN/r2_core_min.kzb" "${BARE[@]}" -- "${G0[@]}"
  one r2_core V2 "$RCDB" "$RB" "$REF" "$IN/r2_core_case.kzb" --profile "$K/profiles/cfs-v7.0.1" -- "${CASEOPS[@]}"
  one r2_core V0p "$RCDB" "$RB" "$REF" "$IN/r2_core_min.kzb" "${BARE[@]}" -- --spawn-op pthread_create
  one r2_core V3 "$RCDB" "$RB" "$REF" "$IN/r2_core_min.kzb" "${BARE[@]}" -- "${G0[@]}" $(rt1 r2_core)
  one r2_all V0 "$RCDB" "$RB" "$REF" "$IN/r2_all.kzb" "${BARE[@]}" --
  one r2_all V1 "$RCDB" "$RB" "$REF" "$IN/r2_all.kzb" "${BARE[@]}" -- "${G0[@]}"
  one r2_all V3 "$RCDB" "$RB" "$REF" "$IN/r2_all.kzb" "${BARE[@]}" -- "${G0[@]}" $(rt1 r2_all)
  local P5=$K/profiles/c5-cfe-6.4.x
  one c5_641 V0 "$C5T/est/compile_commands.json" "$C5TREE/build/cpu1" "$C5TREE" "$IN/c5_641.kzb" "${BARE[@]}" --
  one c5_641 V1 "$C5T/est/compile_commands.json" "$C5TREE/build/cpu1" "$C5TREE" "$IN/c5_641.kzb" "${BARE[@]}" -- "${G0[@]}"
  one c5_641 V2 "$C5T/est/compile_commands.json" "$C5TREE/build/cpu1" "$C5TREE" "$IN/c5_641.kzb" --profile "$P5" --profile "$P5/cfe-6.4.1" -- "${CASEOPS[@]}"
  one c5_641 V0p "$C5T/est/compile_commands.json" "$C5TREE/build/cpu1" "$C5TREE" "$IN/c5_641.kzb" "${BARE[@]}" -- --spawn-op pthread_create
  one c5_641 V3 "$C5T/est/compile_commands.json" "$C5TREE/build/cpu1" "$C5TREE" "$IN/c5_641.kzb" "${BARE[@]}" -- "${G0[@]}" $(rt1 c5_641)
}

# the variants added after the verification of 2026-10-30 (appended to run.log)
extra() {
  [ -f "$IN/c5_641_rule.kzb" ] || inputs > /dev/null
  local BARE=(--profile "$IN/bare")
  for c in pre post; do
    local B=$C1/$c/1df262a/build CDB=$C1/$c/1df262a/build/native/default/compile_commands.json BR=$C1/bundle_1df262a
    for inst in tx rx; do
      one c1_${c}_$inst V0w "$CDB" "$B" "$BR" "$IN/c1_${c}_${inst}_rule.kzb" "${BARE[@]}" --
    done
    one c1_${c}_tx VP "$CDB" "$B" "$BR" "$IN/c1_${c}_tx.kzb" "${BARE[@]}" -- "${P0OPS[@]}"
  done
  local RB=$REF/build-native_std RCDB=$REF/build-native_std/native/default_cpu1/compile_commands.json
  one r2_core VP "$RCDB" "$RB" "$REF" "$IN/r2_core_min.kzb" "${BARE[@]}" -- "${P0OPS[@]}"
  [ -f "$OUT/r2_core/targets_rule.txt" ] || { echo "run first (r2_core V0)" >&2; return 1; }
  one r2_core V3a "$RCDB" "$RB" "$REF" "$IN/r2_core_min.kzb" "${BARE[@]}" -- "${G0[@]}" $(rt1 r2_core) --target CFE_ES_AppRecord_t.AppState
  one r2_all VP "$RCDB" "$RB" "$REF" "$IN/r2_all.kzb" "${BARE[@]}" -- "${P0OPS[@]}"
  one c5_641 V0w "$C5T/est/compile_commands.json" "$C5TREE/build/cpu1" "$C5TREE" "$IN/c5_641_rule.kzb" "${BARE[@]}" --
  one c5_641 VP "$C5T/est/compile_commands.json" "$C5TREE/build/cpu1" "$C5TREE" "$IN/c5_641.kzb" "${BARE[@]}" -- "${P0OPS[@]}"
}

# the case variant (merged tool) against the closures the case drivers wrote earlier (other kz-static builds); the show
# text without the lines that name a hash, a tool, a basis or a path of the run
compare() {
  local D=$OUT/compare; mkdir -p "$D"; : > "$D/compare.txt"
  norm() { "$KS" show "$1" 2>&1 | grep -vE 'build_identity|units_from|^\[files\]|^  [0-9]+ /' ; }
  cmp1() {  # name new old
    norm "$2" > "$D/$1.new.txt"; norm "$3" > "$D/$1.old.txt"
    diff "$D/$1.old.txt" "$D/$1.new.txt" > "$D/$1.diff"
    echo "$1: old $(sha256sum "$3" | cut -c1-16) new $(sha256sum "$2" | cut -c1-16) diff_lines $(grep -c '^[<>]' "$D/$1.diff")" | tee -a "$D/compare.txt"
  }
  cmp1 c1_pre_tx "$OUT/c1_pre_tx/V2/facts.kz" /home/user/work/w6/closures/pre_1df262a/c1prof/closure.facts.kz
  cmp1 c1_post_tx "$OUT/c1_post_tx/V2/facts.kz" /home/user/work/w6/closures/post_1df262a/c1prof/closure.facts.kz
  cmp1 r2_core "$OUT/r2_core/V2/facts.kz" /home/user/work/w6/replay/r2/closure_f0/facts.kz
  # C5: the B' of env/c5/replay_c5.sh bprime was made on the tree of the derived build (641/b) with the E-ST compile
  # database of that build (est_instr); its show text is compared all the same (paths are relative to the bundle root)
  cmp1 c5_641 "$OUT/c5_641/V2/facts.kz" /home/user/work/c5run/replay/bprime/641/facts.kz
  # V0 with the case recipe against V0 with the rule-written recipe (V0w)
  for b in c1_pre_tx c1_pre_rx c1_post_tx c1_post_rx c5_641; do
    [ -f "$OUT/$b/V0w/facts.kz" ] || continue
    diff <(norm "$OUT/$b/V0/facts.kz") <(norm "$OUT/$b/V0w/facts.kz") > "$D/${b}_V0_vs_V0w.diff"
    echo "$b V0 vs V0w (rule-written module lines): diff_lines $(grep -c '^[<>]' "$D/${b}_V0_vs_V0w.diff")" | tee -a "$D/compare.txt"
  done
  # V0 with the case recipe against V0 with the trimmed recipe (facts sections only)
  diff <(norm "$OUT/r2_core/V0/facts.kz") <(norm "$OUT/r2_core/V0r/facts.kz") > "$D/r2_V0_vs_V0r.diff"
  echo "r2_core V0 vs V0r (recipe with generated/table/declared lines): diff_lines $(grep -c '^[<>]' "$D/r2_V0_vs_V0r.diff")" | tee -a "$D/compare.txt"
}

count() {
  mkdir -p "$OUT/count"
  python3 -I "$HERE/extract_baseline_count.py" "$OUT" "$OUT/count"   # prints and writes count/summary.txt and the TSV files
  ( cd "$OUT" && sha256sum inputs/bare/00_profile.kzp inputs/*.kzb */*/basis.kz */*/facts.kz */targets_rule.txt count/*.tsv count/*.txt \
      "$HERE/extract_baseline.sh" "$HERE/extract_baseline_count.py" > sha256.txt )
}

case "$STEP" in
  inputs) inputs ;;
  run) run ;;
  extra) extra ;;
  compare) compare ;;
  count) count ;;
  all) inputs && run && extra && compare && count ;;
  *) echo "unknown step $STEP" >&2; exit 64 ;;
esac

#!/usr/bin/env bash
# kz_run.sh CFG CAND w3|closure|recipes -- the static basis/extract runs of plan/C1_build_manifest.md section 8 on one C1
# P-REC build, reconstructed 2026-10-29 (env/REBUILD_20261029.md).  The original c1_build/kz_run.sh (and its W3 form
# kz_run.sh.w3) was outside the repository and is lost.
#
#   kz_run.sh <pre|post> <1df262a|1b0b338> recipes   write c1_build/kz/<cfg>_<cand>/{closure.kzb,closure_rx.kzb} only
#   kz_run.sh <pre|post> <1df262a|1b0b338> closure   + basis/extract of closure.kzb (section 8.5; profile cfs-v7.0.1)
#   kz_run.sh <pre|post> <1df262a|1b0b338> w3        per link unit (section 8.2): core-tx, cf.so, bp.so, bplib.so
# Outputs c1_build/kz/<cfg>_<cand>/: <unit>.kzb, <unit>.{basis.kz,facts.kz,basis.log,extract.log} (w3);
# closure.{kzb,basis.kz,facts.kz,basis.log,extract.log} (closure).  closure.kzb is also the recipe of kz_run_c1.sh
# (kairos/docs/c1_smoke_observe.md section 9.2: "recipe 는 kz_run.sh 의 closure.kzb 그대로") and of
# kairos/instrument/gcc/scripts/{basis_gi,w4w5_c1_closures}.sh; closure_rx.kzb is closure.kzb with the three tx paths
# (link, binary, /cf mount) replaced by rx (plan/results_pilot_20261025.md section 0).
#
# Fixed by the references:
#   tool and LLVM          kz-static of /home/user/work/kairos-build/k1r3-llvm (LLVM /home/user/work/llvm-project/build)
#   profile                kairos/profiles/cfs-v7.0.1 (section 8.2: "the only profile" at the time)
#   extract operations     --delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData
#                          --spawn-op CFE_ES_CreateChildTask, targets with --target (section 8.2)
#   recipe values          section 8.5 table, with its citations (line numbers of bundle A; the five files are byte-equal
#                          in bundle B, manifest section 8.5); syntax kairos/docs/static_facts.md section 3.1 and
#                          kairos/profiles/cfs-v7.0.1/basis/core-cpu1.kzb
#   modules                bplib ci_lab to_lab sch_lab cf bp cfe_assert (section 8.5 "모듈 link")
# Reconstruction choices (RC-7): the recipe text (name, comments, statement order, the volatile-file and PROCESSOR rule
# lines copied from core-cpu1.kzb); the seven targets given to every unit and to the closure (the set the later C1
# closures use, kairos/instrument/gcc/scripts/w4w5_c1_closures.sh f0); the per-unit W3 recipes (link + binary only);
# file names of the logs.
set -uo pipefail
CFG=${1:?pre|post}; CAND=${2:?1df262a|1b0b338}; MODE=${3:?w3|closure|recipes}
case "$CFG" in pre|post) ;; *) echo "CFG is pre|post" >&2; exit 64 ;; esac
case "$CAND" in 1df262a|1b0b338) ;; *) echo "CAND is 1df262a|1b0b338" >&2; exit 64 ;; esac
C1=${C1_DIR:-/home/user/work/procrace/c1_build}
KZ=${KZ_SRC:-/home/user/Shape-Perf/research/cfs_procrace/kairos}
K=${KZ_STATIC:-/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static}
B=$C1/$CFG/$CAND/build
O=$C1/kz/${CFG}_${CAND}; mkdir -p "$O"
TG=(--target OS_common_record_t.name_entry --target OS_common_record_t.active_id --target OS_count_sem_internal_record_t.obj_name
    --target CF_Channel_t.sem_id --target CF_ChannelConfig_t.sem_name --target BP_Throttle_t.Name --target BP_Throttle_t.Sem)
OPS=(--delay-op OS_TaskDelay --lock-op CFE_ES_LockSharedData --unlock-op CFE_ES_UnlockSharedData --spawn-op CFE_ES_CreateChildTask)

recipe() {  # <inst tx|rx>
  local I=$1
  cat <<EOF
# closure recipe (kz_run.sh, reconstructed 2026-10-29): core-$I of the C1 P-REC build $CFG/$CAND and the modules its
# startup script loads (plan/C1_build_manifest.md section 8.5).  Paths are relative to the build directory.
kzb 1;

name "core-$I of the C1 P-REC build $CFG/$CAND with the modules of its startup script";

link "native/default/$I/CMakeFiles/core-$I.dir/link.txt";
binary "../exe/$I/core-$I";

# the modules the startup script may load; the script decides which are in the closure
module "bplib" link "native/default/apps/bplib/CMakeFiles/bplib.dir/link.txt";
module "ci_lab" link "native/default/apps/ci_lab/CMakeFiles/ci_lab.dir/link.txt";
module "to_lab" link "native/default/apps/to_lab/CMakeFiles/to_lab.dir/link.txt";
module "sch_lab" link "native/default/apps/sch_lab/CMakeFiles/sch_lab.dir/link.txt";
module "cf" link "native/default/apps/cf/CMakeFiles/cf.dir/link.txt";
module "bp" link "native/default/apps/bp/CMakeFiles/bp.dir/link.txt";
module "cfe_assert" link "native/default/apps/cfe_assert/CMakeFiles/cfe_assert.dir/link.txt";

startup_script {
  reset POWERON;
  rule PROCESSOR volatile nonvolatile;
  rule POWERON nonvolatile;
  file volatile const "CFE_PLATFORM_ES_VOLATILE_STARTUP_FILE" in "cfe/modules/es/fsw/src/cfe_es_apps.c";
  file nonvolatile const "CFE_PLATFORM_ES_NONVOL_STARTUP_FILE" in "cfe/cmake/target/src/target_config.c";
  mount "/cf" "../exe/$I/cf";
  entry library "CFE_LIB" global;
  entry app "CFE_APP" local;
  module_file field 1 path const "CFE_PLATFORM_ES_NONVOL_DISK_MOUNT_STRING" extension const "CFE_DEFAULT_MODULE_EXTENSION" in "cfe/cmake/target/src/target_config.c";
  cite "cfe/modules/es/fsw/src/cfe_es_apps.c" 86 150 note "CFE_ES_StartApplications: the file that is opened for each reset type";
  cite "psp/fsw/pc-linux/src/cfe_psp_start.c" 67 67 note "the PSP's non-volatile startup file is the one of the target configuration";
  cite "psp/fsw/pc-linux/src/cfe_psp_start.c" 387 415 note "the reset type: the command line, else the boot record";
  cite "psp/fsw/pc-linux/src/cfe_psp_start.c" 429 429 note "the PSP passes the non-volatile startup file name to the cFE entry point";
  cite "cfe/cmake/target/src/target_config.c" 143 145 note "the non-volatile mount point and startup file of the target configuration";
  cite "psp/fsw/pc-linux/src/cfe_psp_start.c" 355 358 note "the PSP maps /cf to ./cf";
  cite "cfe/modules/es/fsw/src/cfe_es_apps.c" 316 317 note "the file name field of a script line is parsed as a dynamic module";
  cite "cfe/modules/fs/fsw/src/cfe_fs_api.c" 53 110 note "the default path and extension of a dynamic module";
  cite "cfe/modules/fs/fsw/src/cfe_fs_api.c" 582 588 note "CFE_FS_ParseInputFileName adds what the name lacks";
  cite "cfe/cmake/target/src/target_config.c" 171 171 note "the default module extension of the target configuration";
  cite "cfe/modules/es/fsw/src/cfe_es_apps.c" 410 430 note "libraries are loaded with global symbols, apps with local symbols";
  cite "osal/src/os/portable/os-impl-posix-dl-loader.c" 72 96 note "OS_ModuleLoad_Impl: RTLD_NOW, RTLD_GLOBAL or RTLD_LOCAL";
}
EOF
}
recipe tx > "$O/closure.kzb"
recipe rx > "$O/closure_rx.kzb"

run() {  # <recipe> <out prefix>
  "$K" basis --profile "$KZ/profiles/cfs-v7.0.1" --compile-db "$B/native/default/compile_commands.json" --build-dir "$B" \
      --bundle-root "$C1/bundle_$CAND" --recipe "$1" -o "$2.basis.kz" > "$2.basis.log" 2>&1; local a=$?
  "$K" extract --profile "$KZ/profiles/cfs-v7.0.1" --basis "$2.basis.kz" "${OPS[@]}" "${TG[@]}" -o "$2.facts.kz" > "$2.extract.log" 2>&1; local b=$?
  echo "$(basename "$2") basis rc=$a extract rc=$b"
}
case "$MODE" in
recipes) sha256sum "$O/closure.kzb" "$O/closure_rx.kzb" ;;
closure) run "$O/closure.kzb" "$O/closure" ;;
w3)
  for u in core-tx:native/default/tx/CMakeFiles/core-tx.dir/link.txt:../exe/tx/core-tx \
           cf.so:native/default/apps/cf/CMakeFiles/cf.dir/link.txt:../exe/tx/cf/cf.so \
           bp.so:native/default/apps/bp/CMakeFiles/bp.dir/link.txt:../exe/tx/cf/bp.so \
           bplib.so:native/default/apps/bplib/CMakeFiles/bplib.dir/link.txt:../exe/tx/cf/bplib.so; do
    IFS=: read -r name link bin <<< "$u"
    printf 'kzb 1;\nname "%s of the C1 P-REC build %s/%s (one link unit)";\nlink "%s";\nbinary "%s";\n' "$name" "$CFG" "$CAND" "$link" "$bin" > "$O/$name.kzb"
    run "$O/$name.kzb" "$O/$name"
  done ;;
*) echo "MODE is w3|closure|recipes" >&2; exit 64 ;;
esac

#!/usr/bin/env bash
# prep_defs.sh BUNDLE SRC -- the mission definitions of a C1 P-REC build, reconstructed 2026-10-29 from
# plan/C1_build_manifest.md section 3 ("prep_defs.sh BUNDLE SRC"; env/REBUILD_20261029.md).
#
# Run in the work-space root of one build (c1_build/<pre|post>/<cand>, = the CI workspace root).  The seven commands are
# nasa/bp b051eb0 .github/workflows/build-run-bp-common.yml L117-L124 ("Prepare cFS configuration") with only the paths
# replaced: ./cfs-bundle -> BUNDLE, ./src -> SRC (manifest section 3, DV-P3).  Nothing else is copied or edited.
# Reconstruction choice: RC-4 set -e (the CI step runs under the runner's bash -e default) and the echo of the arguments.
set -euo pipefail
BUNDLE=${1:?usage: prep_defs.sh BUNDLE SRC}; SRC=${2:?usage: prep_defs.sh BUNDLE SRC}
echo "# prep_defs.sh BUNDLE=$BUNDLE SRC=$SRC cwd=$PWD"
mkdir ./bpxfer_defs
cp -v $BUNDLE/cfe/cmake/sample_defs/sample_perfids.h ./bpxfer_defs/cfe_perfids.h
cp -v $BUNDLE/cfe/cmake/sample_defs/cpu1_msgids.h ./bpxfer_defs/cfe_msgids.h
cp -v $BUNDLE/cfe/cmake/sample_defs/cpu1_platform_cfg.h ./bpxfer_defs/default_platform_cfg.h
cp -v -t ./bpxfer_defs $BUNDLE/cfe/cmake/sample_defs/*{osconfig,custom,options}.cmake
cp -v -t ./bpxfer_defs $SRC/bp/.github/buildconfig/*.{cmake,scr}
cp -rv -t ./bpxfer_defs $SRC/bp/.github/buildconfig/{tx,rx,tables}
cat $BUNDLE/cfe/cmake/sample_defs/sample_mission_cfg.h $SRC/bp/.github/buildconfig/bp_msgids_extra.h > ./bpxfer_defs/bpxfer_mission_cfg.h

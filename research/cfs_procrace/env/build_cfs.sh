#!/usr/bin/env bash
# build_cfs.sh -- reproduce the documented native Linux build of the cFS bundle at pinned commits.
#
# Every step is taken from the bundle README at the pinned commit; CONDITIONS.md has the full
# condition table (value / reference / status).
#
#   Reference README: https://github.com/nasa/cFS/blob/088b2fa828db9ff7e00733f1908e0eeb59f66ce3/README.md
#     L89      "Ensure the following software are installed: Make, CMake, GCC, and Git."
#     L91-94   git clone https://github.com/nasa/cFS.git / cd cFS / git submodule init / git submodule update
#     L96      "The default Makefile and sample_defs/ directory should already be located at the top-level
#               of this repository with the default configurations for building the open source applications."
#     L102     "To prep, compile, and run on the host (from cFS directory above) as a normal user
#               (best effort message queue depth and task priorities):"
#     L104     make native_std.prep    # Sets up the build tree
#     L105     make native_std.install # Compiles the software and stages it to the exe directory
#     L106     make native_std.runtest # Executes the tests
#     L107     make native_std.lcov    # not performed: lcov is not installed (CONDITIONS.md B4, D-2b)
#
# Usage:
#   build_cfs.sh build   [PARENT_DIR] [LOG_DIR]   README L89-105 from a fresh clone (logs build_00..build_06)
#   build_cfs.sh runtest [PARENT_DIR] [LOG_DIR]   README L106 on an existing build (logs build_08_*)
#   build_cfs.sh record  [PARENT_DIR] [LOG_DIR]   record the effective settings of an existing build and
#                                                 re-run the source checks (log build_07_record_existing.log)
#   build_cfs.sh existing PARENT_DIR [LOG_DIR] [LOG_PREFIX]   (v4) README L104-105 (prep, install) on an existing
#                                                 source tree that is NOT a git clone and has no build yet, e.g. the
#                                                 CORD-instrumented copy (CONDITIONS.md H-9); same user, environment
#                                                 checks, make goals and logging as "build", then the "record" output.
#                                                 No clone and no pinned-state check: the caller proves the source
#                                                 (manifest + patch). PARENT_DIR is required. Logs: <LOG_PREFIX>00_env,
#                                                 00_steps, 04_prep, 05_install, 06_outputs, 07_record (default prefix
#                                                 cord_build_). Refuses a tree that contains .git (e.g. the ENV clone).
#     PARENT_DIR  directory that contains the clone "cFS"   (default /home/user/work/procrace/cfs_ref)
#     LOG_DIR     where logs are written                    (default <this script dir>/logs)
#
# User:
#   README L102 prescribes prep/compile/run "as a normal user". If this script is started as a normal
#   user, everything runs as that user. If it is started as root (the session user of the research
#   container), the clone/submodule fetch runs as root, ownership of the clone is then handed to the
#   existing non-root account named in CFS_NORMAL_USER, and prep/install/runtest run as that account
#   via runuser. Cloning as root is a convenience, not a necessity (CONDITIONS.md D-1). No account is
#   created and no capability, rlimit or scheduler setting is changed. (CONDITIONS.md C11, E1, D-1.)
#
# Branch/commit:
#   The README clone command names no branch; the remote default branch is "dev". This workflow pins the
#   "main" branch HEAD as of 2026-10-08 (== tag v7.0.1). (CONDITIONS.md A2, unspecified_by_reference.)
#
# Script history (the logs in logs/ name the version that produced them; see ENV.md §4):
#   v1  2026-10-08T01:10Z  produced build_00_env.log, build_01*, build_02*, build_03*, build_04*, build_05*.
#   v2  2026-10-08T01:29Z  fixed scr path in the output record (build_06_outputs.log), compiler-specific
#                          warning/error count patterns, label "(CONDITIONS.md A2)" (v1 printed "(C-REV)").
#   v3  2026-10-08 (audit) subcommands; extended environment refusal list and environment record;
#                          nested-submodule check by gitlinks (mode 160000); README L106 runtest step;
#                          "record" step for an existing build.
#   v4  2026-10-08 (CORD R2) "existing" step for a non-git copy (log prefix parameter; the record step skips the
#                          git checks when the tree has no .git). build/runtest/record behave as in v3.

set -euo pipefail

SCRIPT_VERSION="v4"
BUNDLE_URL="https://github.com/nasa/cFS.git"                       # README L91
BUNDLE_COMMIT="088b2fa828db9ff7e00733f1908e0eeb59f66ce3"           # origin/main HEAD on 2026-10-08, tag v7.0.1 (CONDITIONS.md A2)
CONFIG="native_std"                                                 # README L104-106

# Submodule commits recorded by the bundle commit above (gitlinks of BUNDLE_COMMIT).
read -r -d '' PINNED_SUBMODULES <<'EOF' || true
15a871e66cad7ec75e1be78989a93075ff142055 apps/cf
f5d36625336249312ee9d5815bc875e231815bb4 apps/ci_lab
8510c5c0bd3e3b777086ad4b48aee3e79875553a apps/cs
c31e982eca47739dfcf31d4b40cad6068423e6a2 apps/ds
ed51c5733c0f2d2f521d72f80d1b9254d2c8b5d0 apps/fm
766dab1591b95ff89d99baaab9b43d234de4e699 apps/hk
947b903174bca0878210e7fbc53104cff8e7b335 apps/hs
a1c3a47ea1fa5c0d7751d6ff88848dc9bd8a6c7a apps/lc
7e027886b22eb9e6492d59700cdc4549b116c698 apps/md
f3ee4e678df1fe64b9ee267b7e2a888352e178d5 apps/mm
2f93d1a4159a02b18d67ee83342c9e96b90e23e4 apps/sample_app
3b7b37572d63c3f8e85b1f0fa95853c1b78dcf96 apps/sbn
76db5f7b0dde4ea22b41d19962564826967bc123 apps/sc
bc848cd8a6c0407023b959191e8aa39a01928444 apps/sch_lab
38f7312ec4c1109b8f1c0738730b6e5ac5860f05 apps/to_lab
c5fb2b4d540bd55eb6c3707da7dd13eee679d4dd cfe
2d8b7e862673ef90da4355890e18e92822e8acd7 libs/sample_lib
d2d877a69cff47452bcca274b309147d48e6c16f osal
c4b3b0b65b119e106481ad8e20976ae4d7f554e3 psp
07a33a81fe7a88d248d9a5dc59bcf433e34594d7 tools/cFS-GroundSystem
358339d9fbb0516fdf00306cafc6d2cc7f869cd0 tools/cfs-cosmos-plugin
d70c56ec035694c9a64b317897403266166f5d68 tools/commandline-tools
2acc963b34f77692c6396555dcfb10ef43eb1046 tools/eds
118b55fe1d128b48dc45fd36278b87827c6d3faa tools/elf2cfetbl
cc61e89535db9fabe35fe26d1a600889f06b3104 tools/tblCRCTool
EOF

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=lib_record.sh
. "${SCRIPT_DIR}/lib_record.sh"

STEP="${1:-}"
case "${STEP}" in
    build|runtest|record) shift ;;
    existing) shift; [ -n "${1:-}" ] || { echo "ERROR: existing needs an explicit PARENT_DIR" >&2; exit 2; } ;;
    *) sed -n '2,55p' "$0" >&2; echo "ERROR: first argument must be build, runtest, record or existing" >&2; exit 2 ;;
esac
PARENT_DIR="${1:-/home/user/work/procrace/cfs_ref}"
LOG_DIR="${2:-${SCRIPT_DIR}/logs}"
if [ "${STEP}" = existing ]; then LOG_PREFIX="${3:-cord_build_}"; else LOG_PREFIX="build_"; fi
CFS_DIR="${PARENT_DIR}/cFS"
mkdir -p "${LOG_DIR}"

ts() { date -u +%Y-%m-%dT%H:%M:%S.%3NZ; }
say() { echo "[$(ts)] [build_cfs.sh ${SCRIPT_VERSION} ${STEP}] $*" | tee -a "${LOG_DIR}/${LOG_PREFIX}00_steps.log"; }
g() { git -c safe.directory='*' "$@"; }   # read-only git as root on the ubuntu-owned clone (ENV.md §7)

# ---------------------------------------------------------------- user selection (README L102)
AS_USER=()
if [ "$(id -u)" -eq 0 ]; then
    if [ -z "${CFS_NORMAL_USER:-}" ]; then
        echo "ERROR: running as root. README L102 prescribes a normal user for prep/compile/run." >&2
        echo "       Set CFS_NORMAL_USER to an existing non-root account (see CONDITIONS.md C11, E1, D-1)." >&2
        exit 2
    fi
    if ! id -u "${CFS_NORMAL_USER}" >/dev/null 2>&1 || [ "$(id -u "${CFS_NORMAL_USER}")" -eq 0 ]; then
        echo "ERROR: CFS_NORMAL_USER='${CFS_NORMAL_USER}' is not an existing non-root account." >&2
        exit 2
    fi
    AS_USER=(runuser -u "${CFS_NORMAL_USER}" --)
fi
BUILD_USER="${CFS_NORMAL_USER:-$(id -un)}"

# ---------------------------------------------------------------- environment checks (CONDITIONS.md C4)
# The documented build sets none of these. Each is read by the build when present:
#   OMIT_DEPRECATED, ENABLE_ASAN                 sample_defs/global_build_options.cmake L19, L32
#   MISSIONCONFIG, SIMULATION, ENABLE_UNIT_TESTS cfe/cmake/mission_build.cmake L56, L76-77
#   CFS_APP_PATH                                 cfe/cmake/mission_build.cmake L380
#   BUILDDATE, HOSTNAME (USER is the build user) cfe/cmake/generate_build_env.cmake L16, L26, L36
#   O ARCH PREP_OPTS PLATFORM CPUNAME BUILDTYPE DESTDIR PREP_SOURCE_DIR   target-rules.mk L11-18, L49
#   SUBTGT_PREFIX                                Makefile L31, target-rules.mk L38
#   MAKEFLAGS MFLAGS GNUMAKEFLAGS MAKEFILES      GNU make manual
#   CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS VERBOSE, CMAKE_*, CTEST_*   cmake-env-variables(7), CMake 3.28
#   GCC_EXEC_PREFIX COMPILER_PATH LIBRARY_PATH CPATH C_INCLUDE_PATH DEPENDENCIES_OUTPUT
#   SUNPRO_DEPENDENCIES SOURCE_DATE_EPOCH GCC_COMPARE_DEBUG                GCC manual, env variables
#   LD_PRELOAD LD_LIBRARY_PATH GLIBC_TUNABLES   ld.so(8)
REFUSED_VARS="OMIT_DEPRECATED ENABLE_ASAN MISSIONCONFIG SIMULATION ENABLE_UNIT_TESTS CFS_APP_PATH BUILDDATE HOSTNAME
O ARCH PREP_OPTS PLATFORM CPUNAME BUILDTYPE DESTDIR PREP_SOURCE_DIR SUBTGT_PREFIX CFG GOAL STAMPFILE
MAKEFLAGS MFLAGS GNUMAKEFLAGS MAKEFILES CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS VERBOSE
GCC_EXEC_PREFIX COMPILER_PATH LIBRARY_PATH CPATH C_INCLUDE_PATH DEPENDENCIES_OUTPUT SUNPRO_DEPENDENCIES
SOURCE_DATE_EPOCH GCC_COMPARE_DEBUG LD_PRELOAD LD_LIBRARY_PATH GLIBC_TUNABLES"
check_env() {
    local v bad=0
    for v in ${REFUSED_VARS}; do
        # exported variables only (bash itself keeps an unexported shell variable HOSTNAME)
        if printenv "${v}" >/dev/null 2>&1; then
            echo "ERROR: environment variable ${v} is set ('$(printenv "${v}")'); the documented build does not set it." >&2; bad=1
        fi
    done
    for v in $(compgen -e | grep -E '^(CMAKE_|CTEST_)' || true); do
        echo "ERROR: environment variable ${v} is set; CMake/CTest read it (cmake-env-variables(7))." >&2; bad=1
    done
    [ "${bad}" -eq 0 ] || exit 4
}
env_record() {  # $1 = output file
    {
        echo "# build_cfs.sh ${SCRIPT_VERSION} ${STEP}: environment record, $(ts)"
        echo "## invoking user: $(id)"
        echo "## build user: ${BUILD_USER} ($(id "${BUILD_USER}"))"
        echo "## uname: $(uname -a)"
        echo "## os-release:"; sed -n '1,4p' /etc/os-release
        echo "## README L89 prerequisites (Make, CMake, GCC, Git) and B4 tools (jq, lcov):"
        for t in make cmake gcc git jq lcov; do
            if command -v "$t" >/dev/null 2>&1; then echo "$t: $(command -v "$t") :: $("$t" --version 2>&1 | head -1)";
            else echo "$t: MISSING"; fi
        done
        echo "## nproc: $(nproc)"
        echo "## df:"; df -h "${PARENT_DIR%/*}" | tail -1
        echo "## refused variables checked (none may be set): ${REFUSED_VARS//$'\n'/ } CMAKE_* CTEST_*"
        echo "## environment of this invocation (lib_record.sh rules: values only for listed names):"
        env -0 | rec_env_dump | sed 's/^/  /'
        if [ ${#AS_USER[@]} -gt 0 ]; then
            echo "## environment seen by the build user through '${AS_USER[*]}' (same rules):"
            "${AS_USER[@]}" env -0 | rec_env_dump | sed 's/^/  /'
            echo "## credentials of the build user through '${AS_USER[*]}':"
            "${AS_USER[@]}" grep -E '^(Uid|Gid|Groups|Cap(Inh|Prm|Eff|Bnd|Amb)|NoNewPrivs|Seccomp):' /proc/self/status | sed 's/^/  /'
            echo "## rlimits of the build user through '${AS_USER[*]}':"
            "${AS_USER[@]}" cat /proc/self/limits | sed 's/^/  /'
        fi
    } > "$1" 2>&1
}

# ---------------------------------------------------------------- pinned-state check (used by build and record)
nested_gitlinks() {   # prints "<submodule>: <gitlink line>" for every nested submodule gitlink (mode 160000)
    ( cd "${CFS_DIR}" && g submodule foreach --quiet 'git -c safe.directory="*" ls-files -s | awk -v p="$sm_path" "\$1==\"160000\" {print p \": \" \$0}"' )
}
pinned_state_record() {
    ( cd "${CFS_DIR}"
      echo "## bundle: $(g log -1 --format='%H %cI %s')"
      echo "## git describe --tags: $(g describe --tags 2>/dev/null || echo none)"
      echo "## git submodule status"
      g submodule status
      echo "## nested submodules: gitlinks (mode 160000) inside each submodule (empty = none):"
      nested_gitlinks
      echo "## .gitmodules files inside submodules (informational; a .gitmodules entry without a gitlink is not a submodule):"
      g submodule foreach --quiet 'test -f .gitmodules && echo "$sm_path/.gitmodules" || true'
      echo "## git status --porcelain (expected empty; build-*/ is ignored by .gitignore):"
      g status --porcelain
    )
}
pinned_state_verify() {
    local actual expected
    actual="$(cd "${CFS_DIR}" && g submodule status | awk '{sub(/^[-+ U]/,"",$1); print $1, $2}' | sort -k2)"
    expected="$(echo "${PINNED_SUBMODULES}" | sort -k2)"
    if [ "$(cd "${CFS_DIR}" && g rev-parse HEAD)" != "${BUNDLE_COMMIT}" ] || [ "${actual}" != "${expected}" ]; then
        echo "ERROR: checked-out commits differ from the pinned set" >&2
        diff <(echo "${expected}") <(echo "${actual}") >&2 || true
        exit 6
    fi
    if [ -n "$(cd "${CFS_DIR}" && g status --porcelain)" ]; then
        echo "ERROR: working tree not clean" >&2; exit 6
    fi
    if [ -n "$(nested_gitlinks)" ]; then
        echo "ERROR: nested submodule gitlinks found; README L93-94 (non-recursive) would leave them empty" >&2; exit 6
    fi
    # README L96: Makefile and sample_defs/ must already be at top level -- nothing is copied.
    test -f "${CFS_DIR}/Makefile" && test -d "${CFS_DIR}/sample_defs" || { echo "ERROR: README L96 layout not found" >&2; exit 7; }
}

# ================================================================= step: build (README L89-105)
do_build() {
    for t in make cmake gcc git; do
        command -v "$t" >/dev/null 2>&1 || { echo "ERROR: README L89 prerequisite '$t' is missing" >&2; exit 3; }
    done
    check_env
    if [ -e "${CFS_DIR}" ]; then
        echo "ERROR: ${CFS_DIR} already exists; this step builds from a fresh clone only." >&2
        exit 5
    fi
    mkdir -p "${PARENT_DIR}"
    env_record "${LOG_DIR}/build_00_env.log"

    say "clone: git clone ${BUNDLE_URL} (README L91)"
    ( cd "${PARENT_DIR}" && git clone "${BUNDLE_URL}" ) > "${LOG_DIR}/build_01_clone.log" 2>&1
    say "pin bundle commit ${BUNDLE_COMMIT} (CONDITIONS.md A2)"
    ( cd "${CFS_DIR}" && git checkout --detach "${BUNDLE_COMMIT}" ) >> "${LOG_DIR}/build_01_clone.log" 2>&1
    say "git submodule init / git submodule update (README L93-94)"
    local t0 t1
    t0=$(date +%s.%N)
    ( cd "${CFS_DIR}" && git submodule init && git submodule update ) > "${LOG_DIR}/build_02_submodules.log" 2>&1
    t1=$(date +%s.%N)
    say "submodule init+update took $(echo "$t1 - $t0" | bc) s"

    pinned_state_record > "${LOG_DIR}/build_03_pinned_state.log" 2>&1
    pinned_state_verify
    say "pinned state verified (bundle commit + $(echo "${PINNED_SUBMODULES}" | wc -l) submodule commits, no nested gitlinks); Makefile and sample_defs/ present at top level (README L96); nothing copied"

    if [ ${#AS_USER[@]} -gt 0 ]; then
        say "chown -R ${CFS_NORMAL_USER}: ${PARENT_DIR} (CONDITIONS.md D-1: lets the README L102 normal user write the build tree)"
        chown -R "${CFS_NORMAL_USER}:" "${PARENT_DIR}"
    fi

    make_step build_04_prep    "${CONFIG}.prep"       # README L104
    make_step build_05_install "${CONFIG}.install"    # README L105

    ( cd "${CFS_DIR}/build-${CONFIG}/exe"
      echo "## exe tree"; find . -maxdepth 2 | sort
      echo "## cpu1/cf/cfe_es_startup.scr (generated by sample_defs/generate_startup.cmake)"; cat cpu1/cf/cfe_es_startup.scr
      echo "## sizes"; du -sh "${CFS_DIR}" "${CFS_DIR}/build-${CONFIG}" "${CFS_DIR}/build-${CONFIG}/exe"
    ) > "${LOG_DIR}/build_06_outputs.log" 2>&1
    say "done; executable: ${CFS_DIR}/build-${CONFIG}/exe/cpu1/core-cpu1 (README L111-112)"
}

make_step() {  # name, make goal
    local name="$1" goal="$2" t0 t1 rc
    say "${name}: make ${goal} (cwd ${CFS_DIR}, user ${BUILD_USER})"
    t0=$(date +%s.%N)
    set +e
    ( cd "${CFS_DIR}" && "${AS_USER[@]}" make "${goal}" ) > "${LOG_DIR}/${name}.log" 2>&1
    rc=$?
    set -e
    t1=$(date +%s.%N)
    say "${name}: exit ${rc}, wall $(echo "$t1 - $t0" | bc) s, compiler/CMake warnings $(grep -c -E ': warning:|CMake (Deprecation )?Warning' "${LOG_DIR}/${name}.log" || true), compiler/make errors $(grep -c -E ': (fatal )?error:|CMake Error|\*\*\* |Error [0-9]+' "${LOG_DIR}/${name}.log" || true)"
    return ${rc}
}

# ================================================================= step: existing (README L104-105 on a non-git copy)
do_existing() {
    for t in make cmake gcc; do
        command -v "$t" >/dev/null 2>&1 || { echo "ERROR: README L89 prerequisite '$t' is missing" >&2; exit 3; }
    done
    check_env
    [ -d "${CFS_DIR}" ] || { echo "ERROR: ${CFS_DIR} does not exist" >&2; exit 5; }
    [ ! -e "${CFS_DIR}/.git" ] || { echo "ERROR: ${CFS_DIR} is a git clone; use build/record for it" >&2; exit 5; }
    [ ! -e "${CFS_DIR}/build-${CONFIG}" ] || { echo "ERROR: ${CFS_DIR}/build-${CONFIG} exists; this step builds a fresh tree only" >&2; exit 5; }
    test -f "${CFS_DIR}/Makefile" && test -d "${CFS_DIR}/sample_defs" || { echo "ERROR: README L96 layout not found" >&2; exit 7; }
    local notmine
    notmine="$(find "${PARENT_DIR}" ! -user "${BUILD_USER}" | head -5)"
    [ -z "${notmine}" ] || { echo "ERROR: files not owned by the build user ${BUILD_USER} (CONDITIONS.md D-1): ${notmine}" >&2; exit 5; }
    env_record "${LOG_DIR}/${LOG_PREFIX}00_env.log"
    say "existing tree ${CFS_DIR} (not a git clone; source identity from the caller's manifest/patch); README L96 layout present; nothing copied"
    make_step "${LOG_PREFIX}04_prep"    "${CONFIG}.prep"       # README L104
    make_step "${LOG_PREFIX}05_install" "${CONFIG}.install"    # README L105
    ( cd "${CFS_DIR}/build-${CONFIG}/exe"
      echo "## exe tree"; find . -maxdepth 2 | sort
      echo "## cpu1/cf/cfe_es_startup.scr (generated by sample_defs/generate_startup.cmake)"; cat cpu1/cf/cfe_es_startup.scr
      echo "## sizes"; du -sh "${CFS_DIR}" "${CFS_DIR}/build-${CONFIG}" "${CFS_DIR}/build-${CONFIG}/exe"
    ) > "${LOG_DIR}/${LOG_PREFIX}06_outputs.log" 2>&1
    do_record "${LOG_DIR}/${LOG_PREFIX}07_record.log"
    say "done; executable: ${CFS_DIR}/build-${CONFIG}/exe/cpu1/core-cpu1 (README L111-112)"
}

# ================================================================= step: runtest (README L106)
do_runtest() {
    command -v jq >/dev/null 2>&1 || { echo "ERROR: jq is required by target-rules.mk L68-84 (runtest)" >&2; exit 3; }
    check_env
    [ -f "${CFS_DIR}/build-${CONFIG}/stamp.install" ] || { echo "ERROR: no installed build at ${CFS_DIR}/build-${CONFIG}" >&2; exit 3; }
    local conflicts rc=0 snap="${LOG_DIR}/build_08_runtest_host_state.log"
    env_record "${LOG_DIR}/build_08_runtest_env.log"
    rec_host_snapshot "before runtest" "${BUILD_USER}" > "${snap}"
    conflicts="$(rec_osal_conflicts "${CFS_DIR}" "${BUILD_USER}")"
    if [ -n "${conflicts}" ]; then
        { echo "## REFUSED: another cFS/OSAL activity is present (CONDITIONS.md E13):"; echo "${conflicts}"; } | tee -a "${snap}" >&2
        exit 8
    fi
    echo "## isolation check before runtest: no conflicting activity found" >> "${snap}"
    make_step build_08_runtest "${CONFIG}.runtest" || rc=$?
    rec_host_snapshot "after runtest" "${BUILD_USER}" >> "${snap}"
    {
        echo "## test results ($(ts))"
        local list="${CFS_DIR}/build-${CONFIG}/test-list.json" res="${CFS_DIR}/build-${CONFIG}/test-results"
        echo "tests listed in test-list.json: $(jq length "${list}" 2>/dev/null || echo n/a)"
        echo "passed (<name>.log present): $(find "${res}" -maxdepth 1 -name '*.log' 2>/dev/null | wc -l)"
        echo "failed or not completed (<name>.log.tmp left behind, local-test.mk L26-31):"
        find "${res}" -maxdepth 1 -name '*.log.tmp' -printf '  %f\n' 2>/dev/null | sort
        echo "all_tests_complete.stamp: $(ls "${CFS_DIR}/build-${CONFIG}/native/all_tests_complete.stamp" 2>/dev/null || echo absent)"
    } >> "${snap}"
    say "runtest finished with make exit ${rc}; summary in $(basename "${snap}")"
    return ${rc}
}

# ================================================================= step: record (existing build)
do_record() {
    local out="${1:-${LOG_DIR}/build_07_record_existing.log}" b="${CFS_DIR}/build-${CONFIG}"
    [ -d "${b}" ] || { echo "ERROR: no build tree at ${b}" >&2; exit 3; }
    env_record "${out}.env.tmp"
    {
        echo "# build_cfs.sh ${SCRIPT_VERSION} ${STEP}: effective settings of the existing build, $(ts)"
        if [ "${STEP}" = existing ]; then
            echo "# The environment of this build invocation is in ${LOG_PREFIX}00_env.log."
        else
            echo "# The environment of the original build invocation (script v1, 2026-10-08T01:10Z) was not recorded."
        fi
        echo "# The values below are read from the build artifacts, which hold what the build actually used."
        echo
        if [ -e "${CFS_DIR}/.git" ]; then
            echo "## pinned state and source checks (same checks as the build step)"
            pinned_state_record
        else
            echo "## ${CFS_DIR} is not a git clone: no pinned-state check here (the source is identified by the caller's manifest and patch)"
        fi
        echo
        echo "## ${CONFIG} PREP_OPTS actually passed to cmake (target-rules.mk L50 writes them to stamp.prep)"
        cat "${b}/stamp.prep"
        echo
        echo "## mission-level CMakeCache (${b}/CMakeCache.txt), entries the environment can set"
        grep -n -E '^(CFS_APP_PATH|MISSIONCONFIG|SIMULATION|ENABLE_UNIT_TESTS|CFE_EDS_ENABLED|OMIT_DEPRECATED|ENABLE_ASAN|CMAKE_BUILD_TYPE|CMAKE_C_FLAGS|CMAKE_C_FLAGS_DEBUG|CMAKE_C_COMPILER|CMAKE_GENERATOR|CMAKE_INSTALL_PREFIX|CMAKE_EXPORT_COMPILE_COMMANDS|CMAKE_TOOLCHAIN_FILE|MISSION_DEFS)[:=]' "${b}/CMakeCache.txt"
        echo
        echo "## cpu1 arch-level CMakeCache (${b}/native/default_cpu1/CMakeCache.txt)"
        grep -n -E '^(TARGETSYSTEM|CMAKE_BUILD_TYPE|CMAKE_C_FLAGS|CMAKE_C_FLAGS_DEBUG|CMAKE_C_COMPILER|CMAKE_TOOLCHAIN_FILE|OMIT_DEPRECATED|ENABLE_ASAN)[:=]' "${b}/native/default_cpu1/CMakeCache.txt"
        echo
        echo "## build identity embedded in the binary (cfe/cmake/generate_build_env.cmake L16, L26, L36)"
        grep -h -E '"BUILD(DATE|USER|HOST)"' "${b}/native/default_cpu1/cpu1/cfe_build_env_table.c"
        echo
        echo "## effective compile flags (compile_commands.json); flag set = command minus -D/-I/-o/-c and file names"
        python3 -I - "${b}" <<'PY'
import json, sys, collections, os, shlex
b = sys.argv[1]
for label, path in (("host tools (mission scope)", os.path.join(b, "compile_commands.json")),
                    ("cpu1 FSW and unit tests (arch scope)", os.path.join(b, "native/default_cpu1/compile_commands.json"))):
    entries = json.load(open(path))
    sets = collections.Counter()
    for e in entries:
        args = shlex.split(e["command"])[1:]
        keep, skip = [], False
        for a in args:
            if skip: skip = False; continue
            if a in ("-o", "-c", "-I", "-D"): skip = True; continue
            if a.startswith(("-D", "-I")) or not a.startswith("-"): continue
            keep.append(a)
        sets[" ".join(keep)] += 1
    print(f"### {label}: {len(entries)} compile commands")
    for flags, n in sets.most_common():
        print(f"  {n:5d} x  {flags}")
    allcmd = " ".join(e["command"] for e in entries)
    for probe in ("-O0", "-O1", "-O2", "-O3", "-Os", "-Wcast-align=strict", "-fno-common", "-Wcast-align "):
        print(f"  occurrences of '{probe.strip()}': {allcmd.count(probe)}")
PY
    } > "${out}" 2>&1
    { echo; cat "${out}.env.tmp"; } >> "${out}"
    rm -f "${out}.env.tmp"
    say "recorded effective settings of the existing build -> $(basename "${out}")"
}

case "${STEP}" in
    build)   do_build ;;
    runtest) do_runtest ;;
    record)  do_record ;;
    existing) do_existing ;;
esac

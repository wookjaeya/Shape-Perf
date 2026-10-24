#!/usr/bin/env bash
# observe_cfs_smoke.sh -- one observation of the unmodified cFS by `kz observe` (sub-gate S1-R2
# functional check: three runs, one at a time, ordinals 0, 1, 2).
#
#   observe_cfs_smoke.sh <tag> <kz-executable> <run-ordinal> [--out-dir DIR] [--purpose functional-check|pilot]
#                                                                   e.g. s1r2_p2 /path/to/kz 1
#   observe_cfs_smoke.sh --xcheck <kz> <basis.kz> <profile.kzp> <out>   (the anchor cross-check alone)
#
#   --out-dir DIR   where every output goes (default env/logs, as for s1r2_p1).  W2 (2026-10-24): runs
#                   are kept outside the repository and apart from earlier development records.
#   --purpose P     what the run is, written into its conditions file: functional-check (default: the
#                   S1-R2 three-run functional check of docs/observer_smoke_run.md section 9) or pilot (a
#                   pilot/dev run: not a run of the fixed main set, whose repetition count no reference
#                   fixes and the PI decides, D14).  Options are arguments, never environment variables:
#                   the script's environment reaches cFS.
#
# Must be started as root with CFS_NORMAL_USER set (the rule of env/run_cfs.sh).  Only ONE cFS run may
# be active on the host; run_cfs.sh refuses to start if another instance exists.  This script changes
# nothing of the run's conditions: the cFS command is env/run_cfs.sh with the arguments of the reference
# gdb runs (env/run_r2_gdb.sh), minus the gdb wrapper.  The path of kz is an ARGUMENT, not an
# environment variable: whatever this script's environment holds reaches cFS (run_cfs.sh does not fix
# the environment), so nothing is added to it here, and a KZ variable of the caller is removed.
#
# Steps (every output is a new file in env/logs/ with the prefix <tag>_):
#   0. provenance of the tool: the repository commit, the diff of kairos/ against it (<tag>_source.diff),
#      the SHA-256 of kz and a copy of the kz binary outside the repository (runs/<tag>/kz under the
#      build root); refused if a source file of kz is newer than the binary (a stale build)
#   1. persisted-state snapshot (env/tools/persist_snapshot.sh), as env/run_r2_gdb.sh does
#   2. the basis of core-cpu1 for the observation profile (kz anchor; reads the files, changes none)
#   3. the conditions file of the run (what the caller declares; deviations with reason and effect),
#      including the comparison of the environment names with the reference runs
#   4. kz observe -- env/run_cfs.sh operational ci-es-restart-poweron <log>
#   5. persisted-state snapshot again
#   6. counts from the run container (kz hits), including the per-caller grouping
#   7. after the run: the anchors cross-checked with objdump and addr2line (<tag>_anchor_xcheck.txt);
#      done after the run so that it adds no read of the binary to the pre-run window
# Nothing is analysed here: the counts are for the next stage's comparison with the reference.
set -euo pipefail
unset KZ  # the caller may have exported it; it must not reach cFS (it did in s1r2_p1)

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KAIROS="$(cd "${HERE}/../.." && pwd)"
PROFILE="${KAIROS}/profiles/cfs-v7.0.1/observe/sites.kzp"

# The anchors of a basis, checked against objdump and addr2line (independent code).  Output: one
# block per site (the site line of `kz observe --check`, addr2line -f -i -a, and the disassembly
# around the probed address).  Deterministic: no time, no path of a temporary file.
anchor_xcheck() {
    local kz="$1" basis="$2" profile="$3" out="$4"
    command -v objdump >/dev/null && command -v addr2line >/dev/null || { echo "objdump or addr2line missing" >&2; return 77; }
    local plan bin
    plan="$("${kz}" observe "${basis}" "${profile}" --check)"
    bin="$(printf '%s\n' "${plan}" | sed -n 's/^binary \(.*\): hash, size, build-id.*/\1/p')"
    [ -n "${bin}" ] || { echo "kz observe --check printed no binary" >&2; return 1; }
    {
        echo "# anchor cross-check: kz observe --check, then addr2line and objdump on the same file"
        echo "# $(objdump --version | head -1)"
        echo "# $(addr2line --version | head -1)"
        echo "# binary ${bin} sha256 $(sha256sum "${bin}" | cut -c1-64)"
        echo "# basis $(sha256sum "${basis}" | cut -c1-64)"
        printf '%s\n' "${plan}" | grep '^  site ' | while read -r _ id label kind _ func _ addr _ _ off _; do
            echo "== site ${id} ${label} (${kind}) in ${func}: address ${addr}, file offset ${off}"
            echo "-- addr2line -f -i -a"
            addr2line -f -i -a -e "${bin}" "${addr}"
            echo "-- objdump -d -w --disassemble=${func}: the probed instruction and three on each side"
            objdump -d -w --disassemble="${func}" "${bin}" | awk -v a="$(printf '%x:' "${addr}")" '
                { line[NR] = $0; if ($1 == a) hit = NR }
                END { if (!hit) { print "address not found in the disassembly of the function"; exit }
                      for (i = hit - 3; i <= hit + 3; i++) if (i in line) print (i == hit ? ">" : " ") line[i] }'
        done
    } > "${out}"
}

if [ "${1:-}" = "--xcheck" ]; then
    [ $# -eq 5 ] || { echo "usage: $0 --xcheck <kz> <basis.kz> <profile.kzp> <out>" >&2; exit 64; }
    anchor_xcheck "$2" "$3" "$4" "$5"
    exit $?
fi

TAG="${1:?usage: $0 <tag> <kz-executable> <run-ordinal> [--out-dir DIR] [--purpose functional-check|pilot]}"
KZ_BIN="${2:?usage: $0 <tag> <kz-executable> <run-ordinal>}"
ORDINAL="${3:?usage: $0 <tag> <kz-executable> <run-ordinal>}"
case "${ORDINAL}" in ''|*[!0-9]*) echo "run-ordinal must be a number" >&2; exit 64;; esac
shift 3
OUT_DIR=""
PURPOSE="functional-check"
while [ $# -gt 0 ]; do
    case "$1" in
        --out-dir) OUT_DIR="${2:?--out-dir needs a directory}"; shift 2 ;;
        --purpose) PURPOSE="${2:?--purpose needs a value}"; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 64 ;;
    esac
done
case "${PURPOSE}" in functional-check|pilot) ;; *) echo "--purpose is functional-check or pilot" >&2; exit 64 ;; esac
KZ_BIN="$(readlink -f "${KZ_BIN}")"
[ -x "${KZ_BIN}" ] || { echo "not an executable: ${KZ_BIN}" >&2; exit 64; }
ENV_DIR="$(cd "${KAIROS}/../env" && pwd)"
REPO="$(cd "${KAIROS}/.." && pwd)"
LOGS="${ENV_DIR}/logs"
if [ -n "${OUT_DIR}" ]; then
    mkdir -p "${OUT_DIR}"
    LOGS="$(cd "${OUT_DIR}" && pwd)"
fi
CFS_DIR="${CFS_DIR:-/home/user/work/procrace/cfs_ref/cFS}"
EXE_DIR="${CFS_DIR}/build-native_std/exe/cpu1"
BUILD_ROOT="${KZ_BUILD_ROOT:-/home/user/work/kairos-build}"
REF_META="${ENV_DIR}/logs/r2_g1_gdb_operational_esrestart.log.meta"   # the reference runs' environment names (g1 = g2 = g3)
: "${CFS_NORMAL_USER:?set CFS_NORMAL_USER (env/ENV.md E1)}"
[ "$(id -u)" = 0 ] || { echo "must run as root (the observer is root: D1)" >&2; exit 2; }
for f in driver.txt persist_before.txt persist_after.txt basis.kz conditions.tsv kzt kzt.record.txt hits.txt source.diff anchor_xcheck.txt; do
    [ ! -e "${LOGS}/${TAG}_${f}" ] || { echo "refusing to overwrite ${LOGS}/${TAG}_${f}" >&2; exit 2; }
done
DRIVER="${LOGS}/${TAG}_driver.txt"
CFS_LOG="${LOGS}/${TAG}_perf_operational_esrestart.log"
[ ! -e "${CFS_LOG}" ] || { echo "refusing to overwrite ${CFS_LOG}" >&2; exit 2; }
COPY_DIR="${BUILD_ROOT}/runs/${TAG}"
[ ! -e "${COPY_DIR}" ] || { echo "refusing to overwrite ${COPY_DIR}" >&2; exit 2; }

# 0. Provenance of the tool.  A source file of kz newer than the binary means the binary is not the
#    build of this tree: refuse rather than record a tool whose source cannot be named.  The paths are
#    those of kz's source identity (cmake/KzSourceIdentity.cmake: include/, cmake/, CMakeLists.txt and
#    the libraries kz links); a change in any of them changes the tool hash, so a build relinks kz.
STALE="$(find "${KAIROS}/include" "${KAIROS}/lib/core" "${KAIROS}/lib/trace" "${KAIROS}/probe/anchor" "${KAIROS}/probe/plan" \
              "${KAIROS}/probe/loader" "${KAIROS}/tools/kz" "${KAIROS}/cmake" "${KAIROS}/CMakeLists.txt" -type f ! -name '.*' -newer "${KZ_BIN}" | sort | head -5)"
[ -z "${STALE}" ] || { printf 'kz is older than its sources (rebuild it first):\n%s\n' "${STALE}" >&2; exit 2; }
COMMIT="$(git -C "${REPO}" rev-parse HEAD)"
git -C "${REPO}" diff --no-ext-diff --binary HEAD -- kairos > "${LOGS}/${TAG}_source.diff"
UNTRACKED="$(git -C "${REPO}" ls-files --others --exclude-standard -- kairos | sort)"
mkdir -p "${COPY_DIR}"
cp -p "${KZ_BIN}" "${COPY_DIR}/kz"

{
    echo "observe_cfs_smoke.sh tag=${TAG} run_ordinal=${ORDINAL} purpose=${PURPOSE} out_dir=${LOGS} start_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)"
    echo "kz=${KZ_BIN} sha256=$(sha256sum "${KZ_BIN}" | cut -c1-64)"
    echo "kz.copy=${COPY_DIR}/kz sha256=$(sha256sum "${COPY_DIR}/kz" | cut -c1-64) (outside the repository; the tool hash in the container identifies its sources)"
    echo "kz.source: repository commit ${COMMIT}; uncommitted changes of kairos/ in ${TAG}_source.diff sha256=$(sha256sum "${LOGS}/${TAG}_source.diff" | cut -c1-64)"
    echo "kz.source.untracked_files_under_kairos: $(printf '%s' "${UNTRACKED}" | wc -l) (not in the diff:)"
    [ -z "${UNTRACKED}" ] || printf '%s\n' "${UNTRACKED}" | sed 's/^/  /'
    echo "profile=${PROFILE} sha256=$(sha256sum "${PROFILE}" | cut -c1-64)"
    echo "CFS_NORMAL_USER=${CFS_NORMAL_USER}  CFS_DIR=${CFS_DIR}"
    echo "core-cpu1 sha256=$(sha256sum "${EXE_DIR}/core-cpu1" | cut -c1-64)"
} > "${DRIVER}"

# 1. persisted state before
"${ENV_DIR}/tools/persist_snapshot.sh" "before ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_before.txt" 2>&1

# 2. basis
"${KZ_BIN}" anchor "${EXE_DIR}/core-cpu1" "${PROFILE}" -o "${LOGS}/${TAG}_basis.kz" --source-root "${CFS_DIR}" >> "${DRIVER}" 2>&1

# 3. conditions declared by the caller (tab separated; kairos/probe/loader/run_record.hpp)
T=$'\t'
env_names() { tr '\0' '\n' | sed -n 's/^\([A-Za-z_][A-Za-z0-9_]*\)=.*/\1/p' | sort -u; }
# The names cFS will inherit: this script's environment as the observer will pass it on.
USED_NAMES="$(env -u KZ CFS_NORMAL_USER="${CFS_NORMAL_USER}" env -0 | env_names)"
REF_NAMES="$(awk '/^process.environment/{f=1;next} f&&/^  /{sub(/^  /,""); split($0,a,"="); print a[1]; next} f{exit}' "${REF_META}" | sort -u)"
ONLY_REF="$(comm -23 <(printf '%s\n' "${REF_NAMES}") <(printf '%s\n' "${USED_NAMES}") | paste -sd' ' -)"
ONLY_USED="$(comm -13 <(printf '%s\n' "${REF_NAMES}") <(printf '%s\n' "${USED_NAMES}") | paste -sd' ' -)"
N_REF="$(printf '%s\n' "${REF_NAMES}" | grep -c .)"
N_USED="$(printf '%s\n' "${USED_NAMES}" | grep -c .)"
{
    echo "# conditions of ${TAG}: declared by the caller of kz observe (run ordinal ${ORDINAL} of the S1-R2 functional check)"
    echo "item${T}cfs.mode${T}operational (env/run_cfs.sh: stop when the console shows CFE_ES_Main entering OPERATIONAL state)${T}from-reference"
    echo "item${T}cfs.stop${T}ci-es-restart-poweron (the bundle CI's ES restart command, RestartType 2; PI decision D3 on the stop method is open)${T}from-reference"
    echo "item${T}cfs.user${T}${CFS_NORMAL_USER} (run_cfs.sh drops to it with setpriv; README: run as a normal user)${T}from-reference"
    echo "item${T}cfs.binary${T}build-native_std/exe/cpu1/core-cpu1, unmodified${T}from-reference"
    echo "item${T}cfs.working_directory${T}${EXE_DIR} (README L111)${T}from-reference"
    if [ -z "${ONLY_REF}" ] && [ -z "${ONLY_USED}" ]; then
        echo "item${T}cfs.environment${T}the same ${N_USED} variable names as the reference runs (${REF_META##*/}); values are not compared${T}from-reference"
    else
        echo "item${T}cfs.environment${T}${N_USED} variable names; differs from the reference runs by: only in the reference [${ONLY_REF}], only here [${ONLY_USED}]${T}deviation"
        echo "deviation${T}cfs.environment${T}the ${N_REF} variable names of the reference runs (${REF_META##*/}; r2_g2 and r2_g3 have the same set)${T}${N_USED} names inherited from the harness session that started this script (KZ removed); only in the reference: [${ONLY_REF}]; only here: [${ONLY_USED}]; computed before the run from this script's environment, the names cFS received are in the record (traced.pid*.environment.names) and in ${TAG}_perf_operational_esrestart.log.meta${T}run_cfs.sh passes the caller's environment on (README mode) and the variable set of the harness session changed since the reference runs; this script adds nothing to it${T}cFS reads TERM, TMPDIR and SHELL (env/CONDITIONS.md E12), whose values run_cfs.sh records; no effect of the other names is expected, none is verified${T}unknown"
    fi
    echo "item${T}observer.user${T}root (PI decision D1: the observer runs as root, cFS as the normal user)${T}from-reference"
    echo "item${T}observer.method${T}kz observe, perf-sample backend, fidelity F0 (uprobes by file and offset; the file on disk is not modified)${T}deviation"
    echo "item${T}observer.proc_capture${T}full (kz observe default; the same as s1r2_p1): /proc reads of the running target are counted in PERTURBATION proc-capture${T}unspecified_by_reference"
    if [ "${PURPOSE}" = pilot ]; then
        echo "item${T}run.purpose${T}pilot/dev run of the plan in ${PROFILE##*/} (not a run of the fixed main run set; masterplan 2026-10-24 section 3.4, SPEC rev 1.3 ADR-057)${T}unset"
        echo "item${T}run.count${T}this run alone (ordinal ${ORDINAL}); the repetition count of the main run set is not fixed by the references and is the PI's decision (D14)${T}unspecified_by_reference"
    else
        echo "item${T}run.count${T}3 (S1-R2 functional check: s1r2_p1 was ordinal 0; this run is ordinal ${ORDINAL})${T}deviation"
    fi
    echo "item${T}run.stratum${T}not established: RUNMETA stratum none (core-cpu1 is read before the run; see the deviation)${T}deviation"
    echo "item${T}persist.before${T}${TAG}_persist_before.txt sha256 $(sha256sum "${LOGS}/${TAG}_persist_before.txt" | cut -c1-64)${T}unset"
    echo "item${T}host.kernel_vs_reference${T}$(uname -r) (reference runs: 6.18.44-fc-v80, env/logs/r2_g1_gdb_operational_esrestart.log.meta)${T}deviation"
    echo "deviation${T}observer.method${T}gdb -batch breakpoints under RUN_CFS_WRAP, all-stop (env/run_r2_gdb.sh)${T}kz observe perf-sample uprobes, no stop of the process${T}these runs exercise the new observer (sub-gate S1-R2)${T}the two methods perturb the run differently (gdb stops the process at every hit, uprobes do not); no equivalence of the two observations is claimed${T}no"
    [ "${PURPOSE}" = pilot ] || echo "deviation${T}run.count${T}not fixed by the references: PI decision D14 decides N per purpose at S1 and freezes it before the main experiment${T}3 runs, one at a time (ordinals 0, 1, 2; this run ${ORDINAL})${T}the functional check uses the count of the reference gdb runs r2_g1..r2_g3 (3), as the task rule says${T}no frequency, spread or overhead is claimed from three runs; they show whether the observer's counts agree run by run${T}no"
    echo "deviation${T}host.kernel_vs_reference${T}6.18.44-fc-v80 (reference runs, 2026-10-08)${T}$(uname -r)${T}the VM kernel changed since the reference runs; group A is not re-measured yet (PI decision D11)${T}timing and scheduling may differ from the reference runs; not quantified${T}no"
    echo "deviation${T}run.stratum${T}cold = the first run after a VM boot, file cache not warmed (env/BASELINE_MEASURE.md)${T}not established, RUNMETA stratum none: core-cpu1 is read before the run by sha256sum, kz anchor and the observer's own hash, and the cache state is neither measured nor controlled${T}the procedure must read the binary to anchor and verify the probes before attaching${T}the executable's pages are in the cache; the startup order and duration may differ from a cold boot${T}yes"
} > "${LOGS}/${TAG}_conditions.tsv"

# 4. the observation (KZ is not in the environment; nothing else is added to it)
set +e
env -u KZ CFS_NORMAL_USER="${CFS_NORMAL_USER}" "${KZ_BIN}" observe "${LOGS}/${TAG}_basis.kz" "${PROFILE}" \
    -o "${LOGS}/${TAG}.kzt" --record "${LOGS}/${TAG}.kzt.record.txt" --conditions "${LOGS}/${TAG}_conditions.tsv" --stratum none --run-ordinal "${ORDINAL}" \
    -- "${ENV_DIR}/run_cfs.sh" operational ci-es-restart-poweron "${CFS_LOG}" >> "${DRIVER}" 2>&1
RC=$?
set -e
echo "kz observe exit=${RC} end_utc=$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)" >> "${DRIVER}"

# 5. persisted state after
"${ENV_DIR}/tools/persist_snapshot.sh" "after ${TAG}" "${EXE_DIR}" > "${LOGS}/${TAG}_persist_after.txt" 2>&1

# The environment cFS actually received (run_cfs.sh's .meta) against the names declared before the run.
if [ -e "${CFS_LOG}.meta" ]; then
    GOT_NAMES="$(awk '/^process.environment/{f=1;next} f&&/^  /{sub(/^  /,""); split($0,a,"="); print a[1]; next} f{exit}' "${CFS_LOG}.meta" | sort -u)"
    if [ "${GOT_NAMES}" = "${USED_NAMES}" ]; then
        echo "cfs.environment: the names cFS received (.meta) are the names declared in the conditions" >> "${DRIVER}"
    else
        echo "cfs.environment: the names cFS received (.meta) differ from the declared ones: only declared [$(comm -23 <(printf '%s\n' "${USED_NAMES}") <(printf '%s\n' "${GOT_NAMES}") | paste -sd' ' -)], only received [$(comm -13 <(printf '%s\n' "${USED_NAMES}") <(printf '%s\n' "${GOT_NAMES}") | paste -sd' ' -)]" >> "${DRIVER}"
    fi
fi

# 6. counts
if [ -e "${LOGS}/${TAG}.kzt" ]; then
    {
        "${KZ_BIN}" hits "${LOGS}/${TAG}.kzt"
        echo
        "${KZ_BIN}" hits "${LOGS}/${TAG}.kzt" --site evs.send.entry --before sb.appid.store --group-slot app_id --group-return-slot status
        echo
        "${KZ_BIN}" hits "${LOGS}/${TAG}.kzt" --site evs.send.entry --before sb.appid.store --group-slot event_id --basis "${LOGS}/${TAG}_basis.kz" --group-caller
        echo
        "${KZ_BIN}" hits "${LOGS}/${TAG}.kzt" --site evs.send.entry --group-slot event_id --basis "${LOGS}/${TAG}_basis.kz" --group-caller
    } > "${LOGS}/${TAG}_hits.txt" 2>&1 || true
fi

# 7. the anchors against objdump and addr2line, after the run
set +e
anchor_xcheck "${KZ_BIN}" "${LOGS}/${TAG}_basis.kz" "${PROFILE}" "${LOGS}/${TAG}_anchor_xcheck.txt"
echo "anchor_xcheck exit=$?" >> "${DRIVER}"
set -e

{
    echo "artifacts (sha256):"
    for f in "${LOGS}/${TAG}"*; do [ "${f}" = "${DRIVER}" ] || echo "  $(sha256sum "${f}" | cut -c1-64)  ${f##*/}"; done
} >> "${DRIVER}"
exit "${RC}"

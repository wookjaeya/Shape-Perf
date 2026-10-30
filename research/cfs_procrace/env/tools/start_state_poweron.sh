#!/usr/bin/env bash
# start_state_poweron.sh -- prepare and record the R2 common Power-On start state (PI decision 2026-10-28;
# plan/r2_start_poweron_20261028.md).  Used the same way for the F0 (reference build) and the instrumented builds.
#
# The state that can carry over between core-cpu1 processes (persist_snapshot.sh header) is brought to the state of
# the build's install tree, in which the PSP creates it itself:
#   EEPROM.DAT          absent -> open(O_CREAT) + ftruncate to 512 KiB gives the all-zero image
#                       (psp/fsw/modules/eeprom_mmap_file/cfe_psp_eeprom_mmap_file.c L55, L62); the image of the
#                       earlier F0 R2 runs was the same all-zero 524288 B image (sha256 07854d2f..., v153_pilot3)
#   the three key files absent -> created empty (psp/fsw/pc-linux/src/cfe_psp_memory.c L611-615)
#   SysV shm            no segment of the run user (the reserved areas are then created; with --reset PO they are
#                       cleared in any case, cfe_psp_memory.c L646-656)
#   /dev/shm/osal:RAM   absent or an empty directory (the POSIX volatile-disk format is a no-op,
#                       osal/src/os/posix/src/os-impl-filesys.c L223-235, so files would survive a POWERON)
# Files found in the exe directory are MOVED (not deleted) into <record_dir>/prior_state/ with their hashes.
# A SysV segment of the run user or a non-empty /dev/shm/osal:RAM is not removed: the script refuses (exit 9) and
# records the state, because their origin is not known to this script.
#
# Usage: start_state_poweron.sh <exe_dir> <record_dir> <run_user>
set -uo pipefail
exe="$1"; rec="$2"; user="$3"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "${rec}/prior_state"
"${here}/persist_snapshot.sh" "before Power-On start preparation" "${exe}" > "${rec}/start_state_before_prep.txt" 2>&1
{
    echo "## start_state_poweron.sh $(date -u +%Y-%m-%dT%H:%M:%S.%3NZ) exe=${exe} run_user=${user}"
    for f in EEPROM.DAT .cdskeyfile .resetkeyfile .reservedkeyfile; do
        if [ -e "${exe}/${f}" ]; then
            echo "moved ${f} size=$(stat -c %s "${exe}/${f}") sha256=$(sha256sum "${exe}/${f}" | cut -d' ' -f1) -> prior_state/${f}"
            mv -- "${exe}/${f}" "${rec}/prior_state/${f}"
        else
            echo "absent ${f}"
        fi
    done
} > "${rec}/start_state_prep.txt"
refuse=0
segs="$(ipcs -m 2>/dev/null | awk -v u="${user}" 'NR>3 && $3==u {print}')"
if [ -n "${segs}" ]; then
    { echo "REFUSED: SysV shm segments of ${user} exist:"; echo "${segs}" | sed 's/^/  /'; } >> "${rec}/start_state_prep.txt"
    refuse=1
fi
ram=/dev/shm/osal:RAM
if [ -d "${ram}" ] && [ -n "$(ls -A "${ram}" 2>/dev/null)" ]; then
    { echo "REFUSED: ${ram} is not empty:"; ls -la "${ram}" | sed 's/^/  /'; } >> "${rec}/start_state_prep.txt"
    refuse=1
fi
"${here}/persist_snapshot.sh" "after Power-On start preparation" "${exe}" > "${rec}/start_state_after_prep.txt" 2>&1
[ "${refuse}" -eq 0 ] || { echo "start state refused, see ${rec}/start_state_prep.txt" >&2; exit 9; }
echo "start_state=prepared (EEPROM.DAT and key files absent, no SysV shm of ${user}, ${ram} absent or empty)" >> "${rec}/start_state_prep.txt"

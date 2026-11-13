#!/usr/bin/env bash
# persist_snapshot.sh -- record every piece of state that can carry over from one core-cpu1 process to the next
# on this host (BASELINE_MEASURE.md §5; CONDITIONS.md E7, E13).  Read only: nothing is created, changed or removed.
#
#   EEPROM.DAT in the run directory   psp/fsw/modules/eeprom_mmap_file/cfe_psp_eeprom_mmap_file.c L41, L55-69
#                                     (open O_CREAT, ftruncate to 512 KiB, MAP_SHARED: contents survive)
#   .cdskeyfile .resetkeyfile .reservedkeyfile and the SysV segments keyed by ftok(<file>,'R')
#                                     psp/fsw/pc-linux/src/cfe_psp_memory.c L65-67, L149-158, L328-350, L454-463, L611-615
#   /dev/shm/osal:<volume> directories (OSAL posix volatile disks; /ram is /dev/shm/osal:RAM)
#                                     osal/src/os/posix/src/os-impl-filesys.c L134-136, L181-190, L223-235, L254-256
#   POSIX mqueues: none can persist (os-impl-queues.c L142 unlinks every queue right after mq_open), so only the
#                                     kernel-wide count is recorded (ipcs -q shows SysV queues only).
#
# Usage: persist_snapshot.sh <label> <exe_dir>
set -uo pipefail
label="$1"; exe="$2"
echo "## persisted-data snapshot: ${label} ($(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)); boot time $(uptime -s) (uptime -s, local=UTC)"
f="${exe}/EEPROM.DAT"
if [ -e "${f}" ]; then
    echo "EEPROM.DAT: size=$(stat -c %s "${f}") mtime=$(date -u -r "${f}" +%Y-%m-%dT%H:%M:%S.%NZ) ctime=$(stat -c %z "${f}") sha256=$(sha256sum "${f}" | cut -d' ' -f1) nonzero_bytes=$(python3 -I -c 'import sys; d=open(sys.argv[1],"rb").read(); print(sum(1 for b in d if b))' "${f}")"
else
    echo "EEPROM.DAT: absent"
fi
for k in .cdskeyfile .resetkeyfile .reservedkeyfile; do
    p="${exe}/${k}"
    if [ -e "${p}" ]; then
        ino=$(stat -c %i "${p}"); dev=$(stat -c %d "${p}")
        # glibc ftok(): (st_ino & 0xffff) | ((st_dev & 0xff) << 16) | ((proj_id & 0xff) << 24), proj_id 'R' = 0x52
        printf '%s: inode=%s dev=%s mtime=%s size=%s ftok_key=0x%08x\n' "${k}" "${ino}" "${dev}" \
            "$(date -u -r "${p}" +%Y-%m-%dT%H:%M:%S.%NZ)" "$(stat -c %s "${p}")" \
            $(( (ino & 0xffff) | ((dev & 0xff) << 16) | (0x52 << 24) ))
    else
        echo "${k}: absent"
    fi
done
echo "ipcs -m:"; ipcs -m 2>&1 | sed 's/^/  /'
echo "ipcs -m -p (creator/last pids):"; ipcs -m -p 2>&1 | sed 's/^/  /'
echo "mqueue queues in this IPC namespace: /proc/sys/fs/mqueue/queues_max=$(cat /proc/sys/fs/mqueue/queues_max) (no per-namespace count file; /dev/mqueue mounted: $(grep -c ' mqueue ' /proc/mounts))"
echo "/dev/shm:"; ls -la --full-time /dev/shm 2>&1 | sed 's/^/  /'
found=0
for d in /dev/shm/osal:*; do
    [ -e "${d}" ] || continue
    found=1
    echo "${d}: dir mtime=$(date -u -r "${d}" +%Y-%m-%dT%H:%M:%S.%NZ) owner=$(stat -c %U "${d}") mode=$(stat -c %a "${d}")"
    nf=0
    while IFS= read -r -d '' x; do
        nf=$((nf+1))
        echo "  file ${x#${d}/}: size=$(stat -c %s "${x}") mtime=$(date -u -r "${x}" +%Y-%m-%dT%H:%M:%S.%NZ) sha256=$(sha256sum "${x}" | cut -d' ' -f1)"
    done < <(find "${d}" -type f -print0 2>/dev/null | sort -z)
    echo "  files: ${nf}"
done
[ "${found}" = 1 ] || echo "/dev/shm/osal:*: none"
echo "cf/tmp: $(ls -A "${exe}/cf/tmp" 2>/dev/null | wc -l) entries"

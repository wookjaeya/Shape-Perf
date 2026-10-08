# lib_record.sh -- recording and isolation helpers shared by build_cfs.sh and run_cfs.sh.
# Sourced, not executed. See CONDITIONS.md C4, E1, E12, E13 and ENV.md §4-§6.

# ------------------------------------------------------------------ environment record
# The session environment of the research container carries credentials and personal data, so
# every variable is recorded by NAME, and its VALUE only if the name is on this list. The list holds:
#   - every variable the pinned cFS build or runtime reads (CONDITIONS.md C4 and E12),
#   - the documented CMake, CTest, GCC, GNU make and glibc/ld.so variables that change a build or
#     a process (cmake-env-variables(7) for CMake 3.28, GCC manual "Environment Variables Affecting
#     GCC", GNU make manual "Variables from the Environment" and MAKEFILES, ld.so(8)),
#   - identity, locale, proxy and CA-bundle variables (the last two matter for CONDITIONS.md D-1).
rec_env_value_allowed() {
    case "$1" in
        PATH|HOME|USER|LOGNAME|SHELL|TERM|TMPDIR|PWD|HOSTNAME|TZ|LANG|LANGUAGE|LC_*) return 0 ;;
        LD_*|GLIBC_TUNABLES|MALLOC_*) return 0 ;;
        CC|CXX|CFLAGS|CXXFLAGS|CPPFLAGS|LDFLAGS|ASM|ASMFLAGS) return 0 ;;
        MAKEFLAGS|MFLAGS|GNUMAKEFLAGS|MAKEFILES|MAKELEVEL|MAKEOVERRIDES) return 0 ;;
        CMAKE_*|CTEST_*|VERBOSE|DESTDIR) return 0 ;;
        OMIT_DEPRECATED|ENABLE_ASAN|SIMULATION|MISSIONCONFIG|ENABLE_UNIT_TESTS|CFS_APP_PATH|BUILDDATE) return 0 ;;
        PREP_OPTS|PREP_SOURCE_DIR|SUBTGT_PREFIX|O|ARCH|PLATFORM|CPUNAME|BUILDTYPE|CFG|GOAL|STAMPFILE) return 0 ;;
        GCC_*|COMPILER_PATH|LIBRARY_PATH|CPATH|C_INCLUDE_PATH|SOURCE_DATE_EPOCH|DEPENDENCIES_OUTPUT|SUNPRO_DEPENDENCIES) return 0 ;;
        GIT_SSL_CAINFO|SSL_CERT_FILE|CURL_CA_BUNDLE|HTTPS_PROXY|https_proxy|HTTP_PROXY|http_proxy|NO_PROXY|no_proxy) return 0 ;;
        CFS_NORMAL_USER|CFS_DIR) return 0 ;;
    esac
    return 1
}

# Reads a NUL-separated environment (env -0, or /proc/<pid>/environ) on stdin; prints sorted
# NAME=VALUE lines, with "<value not recorded>" for names off the list and URL user-info removed.
rec_env_dump() {
    local kv name val
    while IFS= read -r -d '' kv; do
        name="${kv%%=*}"
        val="${kv#*=}"
        if rec_env_value_allowed "${name}"; then
            val="$(printf '%s' "${val}" | sed -E 's#(://)[^/@[:space:]]*@#\1<userinfo-redacted>@#g')"
            printf '%s=%s\n' "${name}" "${val}"
        else
            printf '%s=<value not recorded>\n' "${name}"
        fi
    done | LC_ALL=C sort
}

# ------------------------------------------------------------------ host-shared state (isolation)
# Host resources shared by every cFS/OSAL process, with the reference that fixes each:
#   /dev/shm/osal:<volume>  OSAL posix volatile disks, fixed location            osal/src/os/posix/src/os-impl-filesys.c L127-140, L181
#                           (mkfs is a no-op, so contents survive POWER ON boots)  same file L223-235
#   SysV shm via ftok(.cdskeyfile/.resetkeyfile/.reservedkeyfile in the cwd)     psp/fsw/pc-linux/src/cfe_psp_memory.c L65-67, L149, L328, L454
#   UDP 1234-1236 (CI_LAB, processor 1-3)                                         apps/ci_lab/fsw/inc/ci_lab_interface_cfg.h L37-51
#   UDP 3234-3236 (SBN UDP peers CPU 1-3)                                         apps/sbn/fsw/tables/sbn_conf_tbl.c L42-58
#   POSIX mqueues named "/<pid>.<name>"                                           osal/src/os/posix/src/os-impl-queues.c L107-110
# The bundle Makefile itself warns that tests "might conflict ... if those programs run on the host
# and bind to the same network ports" (Makefile L40-42).
REC_UDP_PORTS="1234 1235 1236 3234 3235 3236"

# Prints one line per conflicting activity. Empty output means none was found.
#   $1 = bundle directory (processes executing from its build trees are reported)
#   $2 = account whose attached SysV shm segments are reported
rec_osal_conflicts() {
    local cfs_dir="$1" user="$2" p pid comm exe fd tgt port hex f uid
    for p in /proc/[0-9]*; do
        pid="${p#/proc/}"
        [ "${pid}" = "$$" ] && continue
        [ "${pid}" = "${BASHPID:-$$}" ] && continue
        comm="$(cat "${p}/comm" 2>/dev/null)" || continue
        exe="$(readlink "${p}/exe" 2>/dev/null || true)"
        case "${comm}" in
            core-cpu*) echo "process ${pid} (${comm}, exe=${exe}): another cFS instance"; continue ;;
        esac
        case "${exe}" in
            "${cfs_dir}"/build-*) echo "process ${pid} (${comm}, exe=${exe}): executable from this bundle's build tree"; continue ;;
        esac
        for fd in "${p}"/fd/* "${p}/cwd"; do
            tgt="$(readlink "${fd}" 2>/dev/null)" || continue
            case "${tgt}" in
                /dev/shm/osal:*) echo "process ${pid} (${comm}, exe=${exe}): uses OSAL volatile disk ${tgt}"; break ;;
            esac
            if [[ "${tgt}" =~ ^/[0-9]+\. ]]; then
                echo "process ${pid} (${comm}, exe=${exe}): holds an OSAL-style POSIX mqueue ${tgt}"; break
            fi
        done
    done
    for port in ${REC_UDP_PORTS}; do
        hex="$(printf '%04X' "${port}")"
        for f in /proc/net/udp /proc/net/udp6; do
            [ -r "${f}" ] || continue
            if awk -v h=":${hex}" 'NR>1 && substr($2, length($2)-4) == h {found=1} END {exit !found}' "${f}"; then
                echo "UDP port ${port} is bound (${f})"
            fi
        done
    done
    uid="$(id -u "${user}")"
    ipcs -m 2>/dev/null | awk -v u="${user}" -v id="${uid}" '($3==u || $3==id) && $6+0 > 0 {print "SysV shm key " $1 " owned by " $3 " has nattch=" $6}'
}

# Prints a snapshot of the host-shared state.  $1 = label, $2 = account
rec_host_snapshot() {
    # every probe is allowed to fail (e.g. lsof exits 1 when no UDP socket exists); the output says so
    local d
    echo "## host snapshot: $1 ($(date -u +%Y-%m-%dT%H:%M:%S.%3NZ))"
    echo "loadavg: $(cat /proc/loadavg)"
    echo "ipcs:"; { ipcs -m -q -s 2>&1 || true; } | sed 's/^/  /'
    echo "dev_shm:"; { ls -la --full-time /dev/shm 2>&1 || true; } | sed 's/^/  /'
    echo "dev_shm_osal_contents:"
    for d in /dev/shm/osal:*; do
        [ -e "${d}" ] || continue
        { ls -la --full-time "${d}" 2>&1 || true; } | sed "s|^|  ${d}: |"
    done
    echo "udp_sockets (lsof -nP -iUDP; empty = none):"; { lsof -nP -iUDP 2>&1 || true; } | sed 's/^/  /'
    echo "processes_of_$2:"; { ps -u "$2" -o pid,ppid,lstart,comm,args 2>&1 || true; } | sed 's/^/  /'
}

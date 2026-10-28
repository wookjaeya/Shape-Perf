/* kz_rt_posix.h -- the POSIX adapter of the Kairos runtime (Linux host).
 *
 * Identity: the runtime's thread-local cell (kz_rt_tls_cell), so a thread keeps its provisional id
 * across kz_rt_attach (link evidence same-cell); native key = the Linux TID (gettid), generation 0
 * (the runtime never reuses an actor id, so TID reuse yields a new actor).
 * Clock: CLOCK_MONOTONIC (the clock of the F0 records' t_ns).
 * Control: a private pthread mutex is the control lock (not a target object: usable in osal-internal
 * contexts); a gate holds with a per-actor pthread mutex + condition variable that waits on the
 * runtime's released flag (a real wait, with timeout).  All these objects are in static storage
 * (the per-actor ones are initialized once, with pthread_once): no thread, no allocation, nothing
 * from the target's OSAL tables; the dump's RESOURCE records count them as private.
 * It does not depend on OSAL, so it may be attached at any time (also before OS_API_Init).
 *
 * Store file (kzrt 0.4.0, event_semantics.md section 12): kz_rt_posix_store_file puts a file under the runtime's
 * store, so that the records outlive the process and kz-rt-dump --recover can make the dump of the observed prefix
 * after any ending.  The POSIX start-up object calls it from .preinit_array when KZ_RT_STORE names a file.
 */
#ifndef KZ_RT_POSIX_H
#define KZ_RT_POSIX_H

#include "kz_rt.h"

#ifdef __cplusplus
extern "C" {
#endif

KZ_RT_API const kz_rt_adapter *kz_rt_posix_adapter(void);
/* kz_rt_dump into a new file (O_CREAT|O_TRUNC, 0644): 0 = written. */
KZ_RT_API int kz_rt_posix_dump_file(const char *path);
/* kz_rt_dump into an open file descriptor. */
KZ_RT_API int kz_rt_posix_dump_fd(int fd);
/* Puts the file at path under the runtime's store (kz_rt.h, "the record store"): the file is created or truncated
 * (mode 0600) and sized to the store, the store's current bytes are written to it, and the file is mapped shared at
 * the store's own address (MAP_FIXED), so every later record is in the file.  Call it before any other thread runs a
 * hook (records written between the copy and the mapping would stay in the replaced memory).  Only system calls
 * (sysconf, open, ftruncate, pwrite, mmap, close).  The outcome is recorded for the dump (RUNX store_flags); on a
 * refusal the store stays in process memory and the file is truncated to 0 bytes, so it is never taken for a store.
 * KZ_RT_OK = mapped. */
KZ_RT_API int kz_rt_posix_store_file(const char *path);

#ifdef __cplusplus
}
#endif
#endif /* KZ_RT_POSIX_H */

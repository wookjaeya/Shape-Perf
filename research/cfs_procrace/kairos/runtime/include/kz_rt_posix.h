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

#ifdef __cplusplus
}
#endif
#endif /* KZ_RT_POSIX_H */

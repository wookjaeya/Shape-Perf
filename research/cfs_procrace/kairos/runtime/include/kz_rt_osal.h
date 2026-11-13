/* kz_rt_osal.h -- the OSAL adapter skeleton of the Kairos runtime (builds only against an OSAL's
 * public headers and its generated osconfig.h; see runtime/CMakeLists.txt KZ_RT_OSAL_INCLUDE_DIRS).
 *
 * Status: skeleton.  It compiles against the C1 reference OSAL (f34759d) headers; it has not run
 * on a target.  Use it only after OS_API_Init returned OS_SUCCESS (the objects below come from
 * OSAL's tables).  Before that, and inside OSAL internals, the runtime records into its static
 * buffers without this adapter (bootstrap phase) and gates do not hold.
 *
 * What it takes from the target's OSAL object tables (dump RESOURCE records, origin
 * target-table; review section 11.2):
 *   - 2 mutexes (OS_MutSemCreate): the control lock, and the lock of its wait-slot table;
 *   - wait_sems binary semaphores (OS_BinSemCreate, initial 0): one per thread that can be held
 *     at the same time.  The harness passes the number of gates of the plan (each gate holds at
 *     most one thread), so no count is chosen here.
 * Identity: the runtime's thread-local cell (kz_rt_tls_cell; a BSP without compiler TLS must
 * supply another private per-task cell).  It never calls OS_TaskGetId or any cFE lookup from a
 * hook: those read the target's own tables, which may be incomplete exactly when the hook runs.
 * Clock: OS_GetLocalTime (clock id osal-local).  Its control lock is a target object, so the
 * runtime skips control evaluation in osal-internal contexts (KZ_RT_LOSS_CTL_SKIPPED).
 */
#ifndef KZ_RT_OSAL_H
#define KZ_RT_OSAL_H

#include "kz_rt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the OSAL objects; NULL if one could not be created (the ones created are listed by the
 * adapter's resources() and stay allocated). */
KZ_RT_API const kz_rt_adapter *kz_rt_osal_adapter(uint32_t wait_sems);

#ifdef __cplusplus
}
#endif
#endif /* KZ_RT_OSAL_H */

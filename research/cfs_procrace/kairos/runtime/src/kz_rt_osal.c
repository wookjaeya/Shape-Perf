/* kz_rt_osal.c -- see kz_rt_osal.h (skeleton; compiled, not run on a target).
 * OSAL calls used (OSAL f34759d, osal/src/os/inc): OS_MutSemCreate/Take/Give (osapi-mutex.h L60,
 * L96, L78), OS_BinSemCreate/Take/Give/TimedWait (osapi-binsem.h L71, L125, L106, L145),
 * OS_GetLocalTime and OS_TimeGetTotalNanoseconds (osapi-clock.h L87, L234). */
#include "kz_rt_osal.h"

#include "osapi.h"

#ifndef KZ_RT_OSAL_MAX_WAIT
#define KZ_RT_OSAL_MAX_WAIT KZ_RT_MAX_POINTS
#endif

static osal_id_t g_ctl, g_tbl;
static osal_id_t g_sem[KZ_RT_OSAL_MAX_WAIT];
static uint32_t g_owner[KZ_RT_OSAL_MAX_WAIT]; /* actor index + 1 using the semaphore; 0 = free */
static uint32_t g_nsem, g_nmut;

static kz_rt_cell *osal_cell(void *ctx) {
  (void)ctx;
  return kz_rt_tls_cell();
}

static uint64_t osal_now(void *ctx, uint32_t *clock_id) {
  OS_time_t t;
  (void)ctx;
  *clock_id = KZ_RT_CLOCK_OSAL_LOCAL;
  if (OS_GetLocalTime(&t) != OS_SUCCESS) return 0;
  return (uint64_t)OS_TimeGetTotalNanoseconds(t);
}

static void osal_lock(void *ctx) {
  (void)ctx;
  (void)OS_MutSemTake(g_ctl);
}
static void osal_unlock(void *ctx) {
  (void)ctx;
  (void)OS_MutSemGive(g_ctl);
}

static int osal_wait(void *ctx, uint32_t a, volatile const uint32_t *released, uint64_t timeout_ns) {
  uint32_t i, slot = KZ_RT_OSAL_MAX_WAIT;
  int timed_out = 0;
  (void)ctx;
  (void)OS_MutSemTake(g_tbl);
  if (__atomic_load_n(released, __ATOMIC_ACQUIRE) != 0) {
    (void)OS_MutSemGive(g_tbl);
    return 0;
  }
  for (i = 0; i < g_nsem && slot == KZ_RT_OSAL_MAX_WAIT; ++i)
    if (g_owner[i] == 0) slot = i;
  if (slot != KZ_RT_OSAL_MAX_WAIT) g_owner[slot] = a + 1u;
  (void)OS_MutSemGive(g_tbl);
  if (slot == KZ_RT_OSAL_MAX_WAIT) return 1; /* no free semaphore: the runtime records a timeout */
  while (__atomic_load_n(released, __ATOMIC_ACQUIRE) == 0) {
    int32 r = timeout_ns != 0 ? OS_BinSemTimedWait(g_sem[slot], (uint32)(timeout_ns / 1000000u)) : OS_BinSemTake(g_sem[slot]);
    if (r != OS_SUCCESS) {
      timed_out = __atomic_load_n(released, __ATOMIC_ACQUIRE) == 0;
      break;
    }
  }
  (void)OS_MutSemTake(g_tbl);
  g_owner[slot] = 0;
  (void)OS_MutSemGive(g_tbl);
  return timed_out;
}

static void osal_wake(void *ctx, uint32_t a) {
  uint32_t i;
  (void)ctx;
  (void)OS_MutSemTake(g_tbl);
  for (i = 0; i < g_nsem; ++i)
    if (g_owner[i] == a + 1u) (void)OS_BinSemGive(g_sem[i]);
  (void)OS_MutSemGive(g_tbl);
}

static uint32_t osal_resources(void *ctx, kz_rt_resource *out, uint32_t max) {
  (void)ctx;
  if (max < 2) return 0;
  out[0].kind = KZ_RT_RES_MUTEX;
  out[0].origin = KZ_RT_ORIGIN_TARGET_TABLE;
  out[0].count = g_nmut;
  out[0].what = "OSAL mutex (control lock, wait-slot table)";
  out[1].kind = KZ_RT_RES_BINSEM;
  out[1].origin = KZ_RT_ORIGIN_TARGET_TABLE;
  out[1].count = g_nsem;
  out[1].what = "OSAL binary semaphore (gate wait)";
  return 2;
}

static const kz_rt_adapter g_osal = {
    KZ_RT_ABI_VERSION,
    KZ_RT_CAP_CLOCK | KZ_RT_CAP_LOCK | KZ_RT_CAP_WAIT | KZ_RT_CAP_WAIT_TIMEOUT,
    "osal",
    NULL,
    osal_cell,
    NULL,
    osal_now,
    osal_lock,
    osal_unlock,
    osal_wait,
    osal_wake,
    osal_resources,
};

KZ_RT_API const kz_rt_adapter *kz_rt_osal_adapter(uint32_t wait_sems) {
  char name[16] = "kzrt_w00"; /* "kzrt_w" + two decimal digits */
  uint32_t i;
  if (wait_sems > KZ_RT_OSAL_MAX_WAIT || KZ_RT_OSAL_MAX_WAIT > 100u) return NULL;
  if (OS_MutSemCreate(&g_ctl, "kzrt_ctl", 0) != OS_SUCCESS) return NULL;
  ++g_nmut;
  if (OS_MutSemCreate(&g_tbl, "kzrt_tbl", 0) != OS_SUCCESS) return NULL;
  ++g_nmut;
  for (i = 0; i < wait_sems; ++i) {
    name[6] = (char)('0' + i / 10u);
    name[7] = (char)('0' + i % 10u);
    if (OS_BinSemCreate(&g_sem[i], name, 0, 0) != OS_SUCCESS) return NULL;
    ++g_nsem;
  }
  return &g_osal;
}

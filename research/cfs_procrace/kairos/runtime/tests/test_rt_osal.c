/* test_rt_osal -- the OSAL adapter exercised where OSAL is initialized (stage F; kz_rt_osal.h).  Built only when the
 * runtime is configured with KZ_RT_OSAL_INCLUDE_DIRS and KZ_RT_OSAL_LIBS (a target OSAL build: its libosal.a and
 * libosal_bsp.a, whose generic-linux BSP main() calls OS_Application_Startup below).
 *   1. before OS_API_Init: the runtime is set up (control mode) and records into its static bootstrap buffer, with no
 *      adapter: no OSAL object is touched, and a plan loaded then would use the runtime's private spin lock;
 *   2. after OS_API_Init returned OS_SUCCESS: kz_rt_osal_adapter takes its objects from the OSAL tables (2 mutexes, one
 *      binary semaphore per gate) and is attached; the plan's gate holds an OSAL task at its hit until another OSAL task
 *      reaches the release (kzp-hold 1 through the OSAL wait primitive), and the dump lists the objects as taken from the
 *      target's tables (RESOURCE origin target-table) with the realized-control records.
 * Unit-test values (task names, stack size, priorities, delays), not research conditions. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kz_rt.h"
#include "kz_rt_decode.h"
#include "kz_rt_format.h"
#include "kz_rt_osal.h"
#include "osapi.h"

#define CHECK(c)                                                              \
  do {                                                                        \
    if (!(c)) {                                                               \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c);   \
      exit(1);                                                                \
    }                                                                         \
  } while (0)

enum { S_GATE = 10, S_REL = 20 };
static volatile int g_passed, g_result = -1, g_released;
static unsigned char g_dump[1u << 20];
static size_t g_dump_n;

static int mem_write(void *ctx, const void *b, size_t n) {
  (void)ctx;
  if (g_dump_n + n > sizeof g_dump) return 1;
  memcpy(g_dump + g_dump_n, b, n);
  g_dump_n += n;
  return 0;
}

static uint32_t g_res_mutex, g_res_binsem, g_res_origin_ok = 1, g_hold, g_fire, g_resume, g_boot_events;
static int on_item(void *ctx, const kz_rt_dec_item *it) {
  (void)ctx;
  if (it->type == KZ_RT_REC_RESOURCE) {
    if (it->u.resource.origin != KZ_RT_ORIGIN_TARGET_TABLE) g_res_origin_ok = 0;
    if (it->u.resource.kind == KZ_RT_RES_MUTEX) g_res_mutex = (uint32_t)it->u.resource.count;
    if (it->u.resource.kind == KZ_RT_RES_BINSEM) g_res_binsem = (uint32_t)it->u.resource.count;
  }
  if (it->type == KZ_RT_REC_CTL) {
    if (it->u.ctl.what == KZ_RT_CW_GATE_HOLD) ++g_hold;
    if (it->u.ctl.what == KZ_RT_CW_FIRE && it->u.ctl.reason == KZ_RT_CR_RESUMED) ++g_fire;
    if (it->u.ctl.what == KZ_RT_CW_RESUME) ++g_resume;
  }
  if (it->type == KZ_RT_REC_EVENT && (it->u.event.flags & KZ_RT_EF_BOOT) != 0) ++g_boot_events;
  return 0;
}

static void holder(void) {
  const uint32_t ev = kz_rt_obs(S_GATE, 1010, 0, 0, 0, 0, 0, 0);
  g_result = kz_rt_ctl(S_GATE, 1010, ev, KZ_RT_CTX_TASK);
  g_passed = 1;
}

static void releaser(void) {
  kz_rt_point_state ps;
  int i;
  for (i = 0; i < 5000; ++i) {
    CHECK(kz_rt_ctl_state(0, &ps) == KZ_RT_OK);
    if (ps.held_actor != 0) break;
    OS_TaskDelay(1);
  }
  CHECK(ps.held_actor != 0);
  OS_TaskDelay(20);
  CHECK(g_passed == 0); /* still held */
  CHECK(kz_rt_ctl(S_REL, 1020, kz_rt_obs(S_REL, 1020, 0, 0, 0, 0, 0, 0), KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
  g_released = 1;
}

void OS_Application_Startup(void) {
  kz_rt_config cfg;
  kz_rt_point p[2];
  osal_id_t t1, t2;
  const kz_rt_adapter *ad;
  kz_rt_dec_stats st;
  int i;
  memset(&cfg, 0, sizeof cfg);
  cfg.abi = KZ_RT_ABI_VERSION;
  cfg.mode = KZ_RT_MODE_CONTROL;
  cfg.backend = "unit-test";
  CHECK(kz_rt_init(&cfg) == KZ_RT_OK);
  CHECK(kz_rt_obs(1, 101, 0, 0, 0, 0, 0, 0) != 0); /* before OS_API_Init: the bootstrap buffer, no adapter */
  CHECK(OS_API_Init() == OS_SUCCESS);
  ad = kz_rt_osal_adapter(1);
  CHECK(ad != NULL);
  CHECK(kz_rt_attach(ad) == KZ_RT_OK);
  memset(p, 0, sizeof p);
  p[0].kind = KZ_RT_POINT_GATE;
  p[0].site = S_GATE;
  p[0].occurrence = 1;
  p[0].str_slot = p[0].val_slot = -1;
  p[1].kind = KZ_RT_POINT_RELEASE;
  p[1].site = S_REL;
  p[1].releases = 0;
  p[1].occurrence = 1;
  p[1].str_slot = p[1].val_slot = -1;
  CHECK(kz_rt_plan(p, 2) == KZ_RT_OK);
  CHECK(OS_TaskCreate(&t1, "kzt_hold", holder, OSAL_TASK_STACK_ALLOCATE, 65536, OSAL_PRIORITY_C(100), 0) == OS_SUCCESS);
  CHECK(OS_TaskCreate(&t2, "kzt_rel", releaser, OSAL_TASK_STACK_ALLOCATE, 65536, OSAL_PRIORITY_C(100), 0) == OS_SUCCESS);
  for (i = 0; i < 1000 && !(g_passed && g_released); ++i) OS_TaskDelay(10);
  CHECK(g_passed && g_released);
  CHECK(g_result == KZ_RT_CTL_RESUMED);
  kz_rt_stop();
  CHECK(kz_rt_dump(mem_write, NULL) == KZ_RT_OK);
  CHECK(kz_rt_decode(g_dump, g_dump_n, on_item, NULL, &st) == KZ_RT_DEC_OK && st.crc_ok);
  CHECK(g_res_origin_ok && g_res_mutex == 2 && g_res_binsem == 1);
  CHECK(g_hold == 1 && g_fire == 1 && g_resume == 1);
  CHECK(g_boot_events == 1);
  printf("osal: gate held an OSAL task and the release resumed it; objects from the OSAL tables: mutex %u, binsem %u\n", g_res_mutex, g_res_binsem);
  fflush(stdout);
  exit(0);
}

void OS_Application_Run(void) {}

/* kz_rt_posix_auto.c -- optional start-up object for a POSIX target that should not be edited:
 * linked as an object file (CMake target kzrt-posix-auto, an OBJECT library), its constructor runs
 * before main, calls kz_rt_init and attaches the POSIX adapter, and an atexit handler stops the
 * observation window and writes the dump.  Configuration comes from the environment, so a run
 * that uses it adds these names to the target's environment (record them as a difference from
 * the reference run's environment):
 *   KZ_RT_MODE      off | record | control   (default record)
 *   KZ_RT_DUMP      dump file path            (unset = no dump at exit)
 *   KZ_RT_RUN_ID    decimal run id            (unset = 0)
 *   KZ_RT_INSTANCE  decimal instance id       (unset = 0)
 * Events before this constructor (other constructors) are in the bootstrap buffer under the
 * static default mode.  Plans are not read here (kz_rt_plan is called by a harness). */
#define _GNU_SOURCE 1
#include <stdlib.h>
#include <string.h>

#include "kz_rt_posix.h"

static char g_dump_path[4096];

static uint64_t env_u64(const char *name) {
  const char *s = getenv(name);
  return s != NULL ? (uint64_t)strtoull(s, NULL, 10) : 0u;
}

static void kz_rt_auto_exit(void) {
  kz_rt_stop();
  if (g_dump_path[0] != 0) (void)kz_rt_posix_dump_file(g_dump_path);
}

__attribute__((constructor)) static void kz_rt_auto_start(void) {
  kz_rt_config c;
  const char *mode = getenv("KZ_RT_MODE"), *dump = getenv("KZ_RT_DUMP");
  memset(&c, 0, sizeof c);
  c.abi = KZ_RT_ABI_VERSION;
  c.mode = KZ_RT_MODE_RECORD;
  if (mode != NULL && strcmp(mode, "off") == 0) c.mode = KZ_RT_MODE_OFF;
  if (mode != NULL && strcmp(mode, "control") == 0) c.mode = KZ_RT_MODE_CONTROL;
  c.run_id = env_u64("KZ_RT_RUN_ID");
  c.instance_id = (uint32_t)env_u64("KZ_RT_INSTANCE");
  (void)kz_rt_attach(kz_rt_posix_adapter());
  (void)kz_rt_init(&c);
  if (dump != NULL && strlen(dump) < sizeof g_dump_path) {
    memcpy(g_dump_path, dump, strlen(dump) + 1);
    (void)atexit(kz_rt_auto_exit);
  }
}

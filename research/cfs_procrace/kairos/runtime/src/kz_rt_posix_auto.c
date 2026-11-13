/* kz_rt_posix_auto.c -- optional start-up object for a POSIX target that should not be edited:
 * linked as an object file into the executable (CMake target kzrt-posix-auto, an OBJECT library).
 *
 *   1. A .preinit_array entry (ELF executables; glibc runs it before any constructor, also before the
 *      initializers of the shared libraries the executable needs) reads KZ_RT_STORE and KZ_RT_MODE from the
 *      process environment it is given.  With KZ_RT_STORE it first puts that file under the runtime's store
 *      (kz_rt_posix_store_file: system calls only), so the records of the whole run outlive the process
 *      (event_semantics.md section 12).  Then it calls kz_rt_early_mode: the mode of the whole process is fixed
 *      before the first hook can run.  This is the same-binary switch of the groups G-I0 (off),
 *      G-I (record) and G-I-H (control) of the review section 13.1 (event_semantics.md 8.2).  Otherwise it only
 *      reads the environment array and writes the runtime's own memory.
 *   2. A constructor (before main) attaches the POSIX adapter and calls kz_rt_init with the same mode,
 *      and an atexit handler stops the observation window and writes the dump.  The constructor also
 *      records the process id and its own thread as the process's initial thread (kz_rt_process, RUNX
 *      1.2): glibc runs the executable's constructors in the initial thread before main, and the thread
 *      id is compared with the process id (gettid(2) = getpid(2) only for the thread-group leader).
 *
 * Configuration comes from the environment, so a run that uses it adds these names to the target's
 * environment (record them as a difference from the reference run's environment):
 *   KZ_RT_MODE          off | record | control   (unset = record, recorded as defaulted; any other
 *                                                 text = record, recorded as invalid: kz-rt-dump fails)
 *   KZ_RT_DUMP          dump file path            (unset = no dump at exit)
 *   KZ_RT_RUN_ID        decimal run id            (unset = 0)
 *   KZ_RT_INSTANCE      decimal instance id       (unset = 0)
 *   KZ_RT_ACTOR_QUOTA   decimal per-actor quota of committed bytes (unset = 0 = none: only the pool,
 *                       a build parameter, limits); a run parameter of the tool, recorded in RUNX
 *   KZ_RT_STORE         path of a store file (unset or empty = none: the store stays in process memory, as
 *                       before kzrt 0.4.0).  Created or truncated, sized to the store (bytes: kz_rt_store_region)
 *                       and mapped over it before any constructor; put it on a memory file system (e.g. /dev/shm)
 *                       so that the hooks' writes cost what process memory costs.  After the process ends, however
 *                       it ends, kz-rt-dump --recover makes the dump of the file (a recovered prefix unless the exit
 *                       handler's dump was completed).  A file that cannot be mapped is recorded (RUNX store refused,
 *                       a kz-rt-dump check failure) and emptied.  The harness removes the file when it is done with it
 *   KZ_RT_PLAN          path of a serialized plan ("kzrt-plan 1", written by kz rtplan for this build); read
 *                       only in control mode, after attach and init, before main (kz_rt_plan_load: checked
 *                       against the build identity's manifest).  Control mode without it: every gate is
 *                       inactive (PLAN record status none).  Given in another mode: not read (status ignored,
 *                       a kz-rt-dump check failure, so a G-I run is not taken for a G-I-H run)
 * The build identity (backend, compiler, target, manifest sha256) is not configured here: the runtime
 * takes it from kz_rt_build_info when the build links the build-identity object (kzrt-build-info). */
#define _GNU_SOURCE 1
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "kz_rt_posix.h"

static char g_dump_path[4096];
static int g_store_tried; /* KZ_RT_STORE was looked at (by the .preinit_array entry, else by the constructor) */

/* The plan file into a static buffer (one byte more than a plan may have, so that a longer file is seen). */
static void load_plan(const char *path) {
  static unsigned char buf[KZ_RT_PLAN_MAX_BYTES + 1u];
  size_t n = 0;
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) {
    kz_rt_plan_note(KZ_RT_PLANST_READ_ERROR);
    return;
  }
  while (n < sizeof buf) {
    ssize_t r = read(fd, buf + n, sizeof buf - n);
    if (r < 0 && errno == EINTR) continue;
    if (r < 0) {
      (void)close(fd);
      kz_rt_plan_note(KZ_RT_PLANST_READ_ERROR);
      return;
    }
    if (r == 0) break;
    n += (size_t)r;
  }
  (void)close(fd);
  (void)kz_rt_plan_load(buf, n);
}

static uint64_t env_u64(const char *name) {
  const char *s = getenv(name);
  return s != NULL ? (uint64_t)strtoull(s, NULL, 10) : 0u;
}

/* The mode named by s (NULL = unset); *note gets the KZ_RT_EARLY_* bits. */
static uint32_t parse_mode(const char *s, uint32_t *note) {
  *note = KZ_RT_EARLY_GIVEN;
  if (s == NULL) {
    *note = KZ_RT_EARLY_DEFAULTED;
    return KZ_RT_MODE_RECORD;
  }
  if (strcmp(s, "off") == 0) return KZ_RT_MODE_OFF;
  if (strcmp(s, "record") == 0) return KZ_RT_MODE_RECORD;
  if (strcmp(s, "control") == 0) return KZ_RT_MODE_CONTROL;
  *note = KZ_RT_EARLY_INVALID;
  return KZ_RT_MODE_RECORD;
}

#if defined(__ELF__) && !defined(KZ_RT_AUTO_NO_PREINIT)
extern char **environ;

/* The value of the environment entry "<key>..." in e, or NULL (string comparison by hand this early). */
static const char *env_entry(char **e, const char *key) {
  for (; e != NULL && *e != NULL; ++e) {
    size_t i = 0;
    while (key[i] != 0 && (*e)[i] == key[i]) ++i;
    if (key[i] == 0) return *e + i;
  }
  return NULL;
}

/* glibc passes (argc, argv, envp) to .preinit_array functions.  No library call but the store file's system
 * calls (only when KZ_RT_STORE is given). */
static void kz_rt_auto_preinit(int argc, char **argv, char **envp) {
  char **e = envp != NULL ? envp : environ;
  const char *value = env_entry(e, "KZ_RT_MODE="), *store = env_entry(e, "KZ_RT_STORE=");
  uint32_t note, mode;
  (void)argc;
  (void)argv;
  g_store_tried = 1;
  if (store != NULL && store[0] != 0) (void)kz_rt_posix_store_file(store); /* before anything is recorded */
  mode = parse_mode(value, &note);
  (void)kz_rt_early_mode(mode, note);
}
__attribute__((used, section(".preinit_array"))) static void (*const kz_rt_auto_preinit_entry)(int, char **, char **) = kz_rt_auto_preinit;
#endif

static void kz_rt_auto_exit(void) {
  kz_rt_stop();
  if (g_dump_path[0] != 0) (void)kz_rt_posix_dump_file(g_dump_path);
}

__attribute__((constructor)) static void kz_rt_auto_start(void) {
  kz_rt_config c;
  const char *dump = getenv("KZ_RT_DUMP");
  uint32_t note;
  if (!g_store_tried) { /* without .preinit_array (or when it did not run): here, before attach */
    const char *store = getenv("KZ_RT_STORE");
    g_store_tried = 1;
    if (store != NULL && store[0] != 0) (void)kz_rt_posix_store_file(store);
  }
  memset(&c, 0, sizeof c);
  c.abi = KZ_RT_ABI_VERSION;
  c.mode = parse_mode(getenv("KZ_RT_MODE"), &note);
  c.run_id = env_u64("KZ_RT_RUN_ID");
  c.instance_id = (uint32_t)env_u64("KZ_RT_INSTANCE");
  c.actor_buf_bytes = (uint32_t)env_u64("KZ_RT_ACTOR_QUOTA");
  (void)kz_rt_early_mode(c.mode, note); /* without .preinit_array (or when it did not run): here */
  {
    const uint64_t pid = (uint64_t)getpid(), tid = (uint64_t)syscall(SYS_gettid);
    (void)kz_rt_process(pid, tid, KZ_RT_KEY_LINUX_TID, KZ_RT_PROC_CONSTRUCTOR | (tid == pid ? KZ_RT_PROC_TID_IS_PID : 0u));
  }
  (void)kz_rt_attach(kz_rt_posix_adapter());
  (void)kz_rt_init(&c);
  {
    const char *plan = getenv("KZ_RT_PLAN");
    if (c.mode == KZ_RT_MODE_CONTROL) {
      if (plan != NULL && plan[0] != 0) load_plan(plan);
      else kz_rt_plan_note(KZ_RT_PLANST_NONE);
    } else if (plan != NULL && plan[0] != 0) {
      kz_rt_plan_note(KZ_RT_PLANST_IGNORED);
    }
  }
  if (dump != NULL && strlen(dump) < sizeof g_dump_path) {
    memcpy(g_dump_path, dump, strlen(dump) + 1);
    (void)atexit(kz_rt_auto_exit);
  }
}

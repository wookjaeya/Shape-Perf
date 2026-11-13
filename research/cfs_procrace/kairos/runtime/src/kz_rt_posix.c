/* kz_rt_posix.c -- see kz_rt_posix.h. */
#define _GNU_SOURCE 1
#include "kz_rt_posix.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t g_ctl = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_wait_m[KZ_RT_MAX_ACTORS];
static pthread_cond_t g_wait_c[KZ_RT_MAX_ACTORS];
static pthread_once_t g_once = PTHREAD_ONCE_INIT;
static int g_wait_ok;

static void init_waits(void) {
  pthread_condattr_t a;
  unsigned i;
  int ok = pthread_condattr_init(&a) == 0 && pthread_condattr_setclock(&a, CLOCK_MONOTONIC) == 0;
  for (i = 0; ok && i < KZ_RT_MAX_ACTORS; ++i) ok = pthread_mutex_init(&g_wait_m[i], NULL) == 0 && pthread_cond_init(&g_wait_c[i], &a) == 0;
  g_wait_ok = ok;
}

static kz_rt_cell *posix_cell(void *ctx) {
  (void)ctx;
  return kz_rt_tls_cell();
}

static uint64_t posix_key(void *ctx, uint32_t *kind, uint32_t *gen) {
  (void)ctx;
  *kind = KZ_RT_KEY_LINUX_TID;
  *gen = 0;
  return (uint64_t)syscall(SYS_gettid);
}

static uint64_t posix_now(void *ctx, uint32_t *clock_id) {
  struct timespec ts;
  (void)ctx;
  *clock_id = KZ_RT_CLOCK_POSIX_MONOTONIC;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
  return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

static void posix_lock(void *ctx) {
  (void)ctx;
  (void)pthread_mutex_lock(&g_ctl);
}
static void posix_unlock(void *ctx) {
  (void)ctx;
  (void)pthread_mutex_unlock(&g_ctl);
}

static int posix_wait(void *ctx, uint32_t a, volatile const uint32_t *released, uint64_t timeout_ns) {
  struct timespec dl;
  int timed_out = 0;
  (void)ctx;
  if (a >= KZ_RT_MAX_ACTORS) return 1;
  (void)pthread_once(&g_once, init_waits);
  if (!g_wait_ok) return 1;
  if (timeout_ns != 0) {
    uint64_t ns;
    (void)clock_gettime(CLOCK_MONOTONIC, &dl);
    ns = (uint64_t)dl.tv_nsec + timeout_ns % 1000000000u;
    dl.tv_sec += (time_t)(timeout_ns / 1000000000u + ns / 1000000000u);
    dl.tv_nsec = (long)(ns % 1000000000u);
  }
  (void)pthread_mutex_lock(&g_wait_m[a]);
  /* a stale wake must not end the wait: the predicate is the runtime's flag */
  while (__atomic_load_n(released, __ATOMIC_ACQUIRE) == 0) {
    int r = timeout_ns != 0 ? pthread_cond_timedwait(&g_wait_c[a], &g_wait_m[a], &dl) : pthread_cond_wait(&g_wait_c[a], &g_wait_m[a]);
    if (r == ETIMEDOUT) {
      timed_out = __atomic_load_n(released, __ATOMIC_ACQUIRE) == 0;
      break;
    }
  }
  (void)pthread_mutex_unlock(&g_wait_m[a]);
  return timed_out;
}

static void posix_wake(void *ctx, uint32_t a) {
  (void)ctx;
  if (a >= KZ_RT_MAX_ACTORS) return;
  (void)pthread_once(&g_once, init_waits);
  if (!g_wait_ok) return;
  (void)pthread_mutex_lock(&g_wait_m[a]);
  (void)pthread_cond_broadcast(&g_wait_c[a]);
  (void)pthread_mutex_unlock(&g_wait_m[a]);
}

static uint32_t posix_resources(void *ctx, kz_rt_resource *out, uint32_t max) {
  (void)ctx;
  if (max < 2) return 0;
  out[0].kind = KZ_RT_RES_MUTEX;
  out[0].origin = KZ_RT_ORIGIN_PRIVATE;
  out[0].count = 1u + KZ_RT_MAX_ACTORS;
  out[0].what = "pthread mutex (control lock + per-actor wait), static";
  out[1].kind = KZ_RT_RES_CONDVAR;
  out[1].origin = KZ_RT_ORIGIN_PRIVATE;
  out[1].count = KZ_RT_MAX_ACTORS;
  out[1].what = "pthread cond (per-actor wait), static";
  return 2;
}

static const kz_rt_adapter g_posix = {
    KZ_RT_ABI_VERSION,
    KZ_RT_CAP_CLOCK | KZ_RT_CAP_NATIVE_KEY | KZ_RT_CAP_LOCK | KZ_RT_CAP_LOCK_PRIVATE | KZ_RT_CAP_WAIT | KZ_RT_CAP_WAIT_TIMEOUT,
    "posix",
    NULL,
    posix_cell,
    posix_key,
    posix_now,
    posix_lock,
    posix_unlock,
    posix_wait,
    posix_wake,
    posix_resources,
};

KZ_RT_API const kz_rt_adapter *kz_rt_posix_adapter(void) {
  (void)pthread_once(&g_once, init_waits);
  return &g_posix;
}

static int fd_write(void *ctx, const void *bytes, size_t n) {
  const int fd = *(const int *)ctx;
  const unsigned char *p = (const unsigned char *)bytes;
  while (n > 0) {
    ssize_t w = write(fd, p, n);
    if (w < 0 && errno == EINTR) continue;
    if (w <= 0) return 1;
    p += w;
    n -= (size_t)w;
  }
  return 0;
}

KZ_RT_API int kz_rt_posix_dump_fd(int fd) { return kz_rt_dump(fd_write, &fd); }

KZ_RT_API int kz_rt_posix_dump_file(const char *path) {
  int fd, r;
  if (path == NULL) return KZ_RT_EINVAL;
  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) return KZ_RT_EWRITE;
  r = kz_rt_posix_dump_fd(fd);
  if (close(fd) != 0 && r == KZ_RT_OK) r = KZ_RT_EWRITE;
  return r;
}

/* ---- the store file (kzrt 0.4.0) ------------------------------------------------------------------------- */
static int page_is_zero(const unsigned char *p, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i)
    if (p[i] != 0) return 0;
  return 1;
}

static int pwrite_all(int fd, const unsigned char *p, size_t n, off_t off) {
  while (n > 0) {
    ssize_t w = pwrite(fd, p, n, off);
    if (w < 0 && errno == EINTR) continue;
    if (w <= 0) return 1;
    p += w;
    off += w;
    n -= (size_t)w;
  }
  return 0;
}

/* A refused store file: noted, and the file emptied, so that a later recovery does not take it for a store. */
static int store_refused(uint32_t reason, int fd, int rc) {
  kz_rt_store_note(KZ_RT_STOREF_REFUSED | (reason << 8));
  if (fd >= 0) {
    if (ftruncate(fd, 0) != 0) {
      /* nothing more to do: the size check of kz-rt-dump --recover refuses a file that is not a whole store */
    }
    (void)close(fd);
  }
  return rc;
}

KZ_RT_API int kz_rt_posix_store_file(const char *path) {
  size_t n = 0, off;
  unsigned char *base;
  long pg;
  int fd;
  if (path == NULL || path[0] == 0) return KZ_RT_EINVAL;
  base = (unsigned char *)kz_rt_store_region(&n);
  pg = sysconf(_SC_PAGESIZE);
  if (pg <= 0 || (uintptr_t)base % (uintptr_t)pg != 0 || n % (size_t)pg != 0) return store_refused(KZ_RT_STORE_R_ALIGN, -1, KZ_RT_EINVAL);
  fd = open(path, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
  if (fd < 0) return store_refused(KZ_RT_STORE_R_OPEN, -1, KZ_RT_EWRITE);
  if (ftruncate(fd, (off_t)n) != 0) return store_refused(KZ_RT_STORE_R_SIZE, fd, KZ_RT_EWRITE);
  kz_rt_store_note(KZ_RT_STOREF_FILE); /* before the copy, so that the file says so too */
  /* the store's current bytes; pages still zero are not written (the sized file reads them as zero) */
  for (off = 0; off < n; off += (size_t)pg)
    if (!page_is_zero(base + off, (size_t)pg) && pwrite_all(fd, base + off, (size_t)pg, (off_t)off) != 0)
      return store_refused(KZ_RT_STORE_R_WRITE, fd, KZ_RT_EWRITE);
  if (mmap(base, n, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED) return store_refused(KZ_RT_STORE_R_MAP, fd, KZ_RT_EWRITE);
  (void)close(fd); /* the mapping keeps the file */
  return KZ_RT_OK;
}

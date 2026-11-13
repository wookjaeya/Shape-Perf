/* wrappers.c -- the profile-free mode (extract --no-profile): delay, terminal and spawn operations
 * derived from the library primitives of the P0 table through wrappers.  The library functions are
 * declared here in the shape POSIX gives them, so that the fixture needs no header; nothing names
 * an operation on the command line. */
typedef unsigned long fx_thread_t;
struct timespec {
  long tv_sec;
  long tv_nsec;
};
int pthread_create(fx_thread_t *thread, const void *attr, void *(*start)(void *), void *arg);
int nanosleep(const struct timespec *req, struct timespec *rem);
void exit(int status) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
void pthread_exit(void *value) __attribute__((noreturn));
void *dlsym(void *handle, const char *name);
int pthread_mutex_lock(void *mutex);
int pthread_mutex_unlock(void *mutex);

/* ---- delay: a wrapper of a wrapper on every path (must), one on some path (may) ---- */
int fx_interrupted;

int fx_sleep_impl(unsigned ms) {
  struct timespec t;
  int rc;
  t.tv_sec = (long)(ms / 1000u);
  t.tv_nsec = (long)(ms % 1000u) * 1000000L;
  do {
    rc = nanosleep(&t, &t);
  } while (rc == -1 && fx_interrupted != 0);
  return rc;
}

int fx_delay(unsigned ms) { return fx_sleep_impl(ms); }

int fx_backoff(int busy) {
  if (busy) return fx_delay(10u);
  return 0;
}

int fx_try(void);

/* a retry loop bounded by a count, waiting through the wrapper: count-bounded */
int fx_retry(void) {
  int i, rc = -1;
  for (i = 0; i < 5; ++i) {
    rc = fx_try();
    if (rc != -17) break;
    fx_delay(100u);
  }
  return rc;
}

/* a wait loop through the wrapper: poll */
int fx_ready;
void fx_wait_ready(void) {
  while (fx_ready == 0) fx_delay(1u);
}

/* a loop that calls the `may` wrapper: not classified by it */
void fx_spin(void) {
  int n;
  for (n = 0; fx_try() != 0; ++n) fx_backoff(n);
}

/* ---- termination: must (process), must (thread), noreturn, may ---- */
void fx_panic(int code) {
  if (code > 0) exit(code);
  abort();
}

void fx_task_exit(void) { pthread_exit((void *)0); }

/* ends the process on one path and waits for ever on the other: never returns */
void fx_exit_app(int status) {
  if (status == 1) fx_panic(status);
  for (;;) fx_delay(500u);
}

/* ends the thread when the check fails, returns otherwise */
int fx_check(int ok) {
  if (!ok) fx_task_exit();
  return ok;
}

/* an endless loop with a terminal call on one path: not `must` */
void fx_serve(void) {
  for (;;)
    if (fx_try() < 0) fx_panic(2);
}

/* recursion: the component is solved as one */
int fx_even(int n);
int fx_odd(int n) {
  if (n < 0) fx_panic(3);
  return n == 0 ? 0 : fx_even(n - 1);
}
int fx_even(int n) { return n == 0 ? 1 : fx_odd(n - 1); }

/* ---- spawn: an entry argument, a fixed entry, an entry called through a field ---- */
typedef void (*fx_entry_t)(void);
struct fx_task {
  fx_entry_t entry;
  fx_thread_t id;
};
struct fx_task fx_tasks[4];
int fx_lock_word;

int fx_thread_create(fx_thread_t *id, void *(*start)(void *), void *arg) { return pthread_create(id, 0, start, arg); }

static void *fx_trampoline(void *arg) {
  (void)arg;
  fx_tasks[0].entry();
  fx_task_exit();
  return 0;
}

static int fx_task_create_impl(int slot) { return fx_thread_create(&fx_tasks[slot].id, fx_trampoline, (void *)0); }

int fx_task_create(int slot, fx_entry_t entry) {
  int rc;
  pthread_mutex_lock(&fx_lock_word);
  fx_tasks[slot].entry = entry;
  pthread_mutex_unlock(&fx_lock_word);
  rc = fx_task_create_impl(slot);
  return rc;
}

int fx_app_create(int slot, fx_entry_t main_fn) { return fx_task_create(slot, main_fn); }

static void fx_worker(void) { fx_wait_ready(); }
void fx_start(void) { fx_app_create(1, fx_worker); }

/* an entry looked up by name: data converted to a function pointer */
unsigned long fx_lookup(const char *name) {
  void *p = dlsym((void *)0, name);
  return (unsigned long)p;
}

int fx_load_app(int slot, const char *symbol) {
  unsigned long addr = fx_lookup(symbol);
  return fx_task_create(slot, (fx_entry_t)addr);
}

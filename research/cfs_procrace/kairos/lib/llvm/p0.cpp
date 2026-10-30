// p0.cpp -- see p0.hpp.
#include "p0.hpp"

namespace kzs {

namespace {

constexpr const char* kPosix = "POSIX.1-2017 / glibc 2.39";
constexpr const char* kC11 = "ISO C11 <threads.h> / glibc 2.39";
constexpr const char* kClock = "glibc 2.39 (since 2.30); POSIX.1-2024, not in POSIX.1-2017";
constexpr const char* kCleanup =
    "glibc 2.39, the function the POSIX.1-2017 macro pthread_cleanup_push or pthread_cleanup_pop expands to in <pthread.h> when the unit is "
    "compiled without exceptions (with exceptions the macros call no function)";
constexpr const char* kUnverified = "unverified";

P0Entry make(uint8_t kind, const char* family, const char* name, const char* semantics, const char* version = kPosix, const char* source = kUnverified) {
  P0Entry e;
  e.kind = kind;
  e.family = family;
  e.name = name;
  e.semantics = semantics;
  e.version = version;
  e.source = source;
  return e;
}

P0Entry conditional(P0Entry e) {
  e.conditional = true;
  return e;
}

P0Entry mutex_arg(P0Entry e, int k) {
  e.mutex = k;
  return e;
}

P0Entry data(const char* name, int dest, int src, int fill, bool zero, uint8_t kind, const char* semantics, const char* version = kPosix) {
  P0Entry e = make(KZ_P0_KIND_DATA_FN, "data function", name, semantics, version);
  e.dest = dest;
  e.out = dest;
  e.src = src;
  e.fill = fill;
  e.zero_fill = zero;
  e.writer_kind = kind;
  return e;
}

std::vector<P0Entry> build() {
  std::vector<P0Entry> t;
  // thread creation
  {
    P0Entry e = make(KZ_P0_KIND_THREAD_CREATE, "thread creation", "pthread_create",
                     "creates a thread that runs arg 2 with the argument arg 3; on success the id of the new thread is stored through arg 0, and "
                     "it is stored before the new thread is started",
                     kPosix,
                     "glibc 2.39 nptl/pthread_create.c:624 (__pthread_create_2_1), :755 (*newthread = (pthread_t) pd, before create_thread at :804 "
                     "and :836); upstream copy sha256 36fd1e6da5cee9ce");
    e.out = 0;
    e.entry = 2;
    e.arg = 3;
    t.push_back(e);
  }
  {
    P0Entry e = make(KZ_P0_KIND_THREAD_CREATE, "thread creation", "thrd_create",
                     "creates a thread that runs arg 1 with the argument arg 2; on success (thrd_success) the id of the new thread is stored through arg 0",
                     kC11);
    e.out = 0;
    e.entry = 1;
    e.arg = 2;
    t.push_back(e);
  }
  // mutexes
  // the mutex argument (`mutex`, not written) is the one the semantics name
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "pthread_mutex_lock", "acquires the mutex arg 0, blocking until it is free"), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "pthread_mutex_trylock", "acquires the mutex arg 0 if it is free, else returns without it (EBUSY)")), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "pthread_mutex_timedlock",
                                         "acquires the mutex arg 0, blocking at most until the absolute time arg 1; may return without it (ETIMEDOUT)")),
                        0));
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_RELEASE, "mutex", "pthread_mutex_unlock", "releases the mutex arg 0"), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "pthread_mutex_clocklock",
                                         "acquires the mutex arg 0, blocking at most until the absolute time arg 2 on the clock arg 1; may return without it (ETIMEDOUT)",
                                         kClock)),
                        0));
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "mtx_lock", "acquires the mutex arg 0, blocking until it is free; thrd_success (0 in glibc 2.39) when it holds it", kC11), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "mtx_trylock", "acquires the mutex arg 0 if it is free, else returns without it (thrd_busy)", kC11)), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "mtx_timedlock",
                                         "acquires the mutex arg 0, blocking at most until the absolute time arg 1; may return without it (thrd_timedout)", kC11)),
                        0));
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_RELEASE, "mutex", "mtx_unlock", "releases the mutex arg 0", kC11), 0));
  // spin locks: exclusive like a mutex (the kind names the exclusive acquisition)
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_ACQUIRE, "spin lock", "pthread_spin_lock", "acquires the spin lock arg 0, spinning until it is free; returns 0 when it holds it"), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "spin lock", "pthread_spin_trylock", "acquires the spin lock arg 0 if it is free, else returns without it (EBUSY)")), 0));
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_RELEASE, "spin lock", "pthread_spin_unlock", "releases the spin lock arg 0"), 0));
  // semaphores
  t.push_back(make(KZ_P0_KIND_SEM_WAIT, "semaphore", "sem_wait", "decrements the semaphore arg 0, blocking while it is zero"));
  t.push_back(conditional(make(KZ_P0_KIND_SEM_WAIT, "semaphore", "sem_trywait", "decrements the semaphore arg 0 if it is not zero, else returns without (EAGAIN)")));
  t.push_back(conditional(make(KZ_P0_KIND_SEM_WAIT, "semaphore", "sem_timedwait",
                               "decrements the semaphore arg 0, blocking at most until the absolute time arg 1; may return without (ETIMEDOUT)")));
  t.push_back(make(KZ_P0_KIND_SEM_POST, "semaphore", "sem_post", "increments the semaphore arg 0; one thread blocked on it is woken"));
  t.push_back(conditional(make(KZ_P0_KIND_SEM_WAIT, "semaphore", "sem_clockwait",
                               "decrements the semaphore arg 0, blocking at most until the absolute time arg 2 on the clock arg 1; may return without (ETIMEDOUT)", kClock)));
  // condition variables
  t.push_back(mutex_arg(make(KZ_P0_KIND_COND_WAIT, "condition variable", "pthread_cond_wait",
                             "releases the mutex arg 1, waits on the condition arg 0, and holds the mutex again when it returns"),
                        1));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_COND_WAIT, "condition variable", "pthread_cond_timedwait",
                                         "as pthread_cond_wait, waiting at most until the absolute time arg 2 (ETIMEDOUT); holds the mutex arg 1 again when it returns")),
                        1));
  t.push_back(make(KZ_P0_KIND_COND_SIGNAL, "condition variable", "pthread_cond_signal", "unblocks at least one thread waiting on the condition arg 0"));
  t.push_back(make(KZ_P0_KIND_COND_SIGNAL, "condition variable", "pthread_cond_broadcast", "unblocks every thread waiting on the condition arg 0"));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_COND_WAIT, "condition variable", "pthread_cond_clockwait",
                                         "as pthread_cond_wait, waiting at most until the absolute time arg 3 on the clock arg 2 (ETIMEDOUT); holds the mutex arg 1 again when it returns",
                                         kClock)),
                        1));
  t.push_back(mutex_arg(make(KZ_P0_KIND_COND_WAIT, "condition variable", "cnd_wait",
                             "releases the mutex arg 1, waits on the condition arg 0, and holds the mutex again when it returns", kC11),
                        1));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_COND_WAIT, "condition variable", "cnd_timedwait",
                                         "as cnd_wait, waiting at most until the absolute time arg 2 (thrd_timedout); holds the mutex arg 1 again when it returns", kC11)),
                        1));
  t.push_back(make(KZ_P0_KIND_COND_SIGNAL, "condition variable", "cnd_signal", "unblocks one thread waiting on the condition arg 0", kC11));
  t.push_back(make(KZ_P0_KIND_COND_SIGNAL, "condition variable", "cnd_broadcast", "unblocks every thread waiting on the condition arg 0", kC11));
  // sleeping
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "clock_nanosleep",
                   "suspends the calling thread until the time arg 2 has passed on the clock arg 0 (an interval, or an absolute time when arg 1 has "
                   "TIMER_ABSTIME)"));
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "nanosleep", "suspends the calling thread for the interval arg 0"));
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "sleep", "suspends the calling thread for arg 0 seconds"));
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "usleep", "suspends the calling thread for arg 0 microseconds", "glibc 2.39; not in POSIX.1-2017 (removed in Issue 7)"));
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "thrd_sleep", "suspends the calling thread for the interval arg 0", kC11));
  // termination
  t.push_back(make(KZ_P0_KIND_THREAD_EXIT, "termination", "pthread_exit", "ends the calling thread; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "exit", "ends the process after its exit handlers have run; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "_exit", "ends the process without running exit handlers; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "_Exit", "ends the process without running exit handlers; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "abort", "ends the process abnormally (SIGABRT); does not return"));
  t.push_back(make(KZ_P0_KIND_THREAD_EXIT, "termination", "thrd_exit", "ends the calling thread; does not return", kC11));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "quick_exit", "ends the process after its at_quick_exit handlers have run; does not return",
                   "ISO C11 <stdlib.h> / glibc 2.39; POSIX.1-2024, not in POSIX.1-2017"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "__assert_fail", "reports a failed assertion and ends the process (abort); does not return",
                   "glibc 2.39, the function the assert macro of <assert.h> (ISO C, POSIX.1-2017) expands to"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "__assert_perror_fail", "reports a failed assertion with an error number and ends the process (abort); does not return",
                   "glibc 2.39, the function the assert_perror macro of <assert.h> (GNU) expands to"));
  // thread identity and thread-specific data
  t.push_back(make(KZ_P0_KIND_THREAD_SELF, "thread identity", "pthread_self", "returns the id of the calling thread"));
  t.push_back(make(KZ_P0_KIND_THREAD_SELF, "thread identity", "thrd_current", "returns the id of the calling thread", kC11));
  t.push_back(make(KZ_P0_KIND_TSD_GET, "thread-specific data", "pthread_getspecific", "returns the value the calling thread bound to the key arg 0"));
  t.push_back(make(KZ_P0_KIND_TSD_SET, "thread-specific data", "pthread_setspecific", "binds the value arg 1 to the key arg 0 for the calling thread"));
  t.push_back(make(KZ_P0_KIND_TSD_GET, "thread-specific data", "tss_get", "returns the value the calling thread bound to the key arg 0", kC11));
  t.push_back(make(KZ_P0_KIND_TSD_SET, "thread-specific data", "tss_set", "binds the value arg 1 to the key arg 0 for the calling thread", kC11));
  {
    P0Entry e = make(KZ_P0_KIND_TSD_KEY_CREATE, "thread-specific data", "pthread_key_create",
                     "creates a key and stores it through arg 0; arg 1, when not null, is a destructor that every thread which ends with a non-null value bound "
                     "to the key runs with that value (after its cleanup handlers)");
    e.out = 0;
    e.entry = 1;
    t.push_back(e);
  }
  t.push_back(make(KZ_P0_KIND_TSD_KEY_DELETE, "thread-specific data", "pthread_key_delete", "deletes the key arg 0; no destructor runs"));
  {
    P0Entry e = make(KZ_P0_KIND_TSD_KEY_CREATE, "thread-specific data", "tss_create",
                     "creates a key and stores it through arg 0; arg 1, when not null, is a destructor that every thread which ends with a non-null value bound "
                     "to the key runs with that value",
                     kC11);
    e.out = 0;
    e.entry = 1;
    t.push_back(e);
  }
  t.push_back(make(KZ_P0_KIND_TSD_KEY_DELETE, "thread-specific data", "tss_delete", "deletes the key arg 0; no destructor runs", kC11));
  // cleanup handlers: run by the thread when it ends through pthread_exit or a cancellation while they are pushed
  t.push_back(make(KZ_P0_KIND_CLEANUP_PUSH, "cleanup handler", "__pthread_register_cancel",
                   "pushes the cleanup handler the expansion of pthread_cleanup_push stored in the buffer arg 0", kCleanup));
  t.push_back(make(KZ_P0_KIND_CLEANUP_PUSH, "cleanup handler", "__pthread_register_cancel_defer",
                   "pushes the cleanup handler stored in the buffer arg 0 and makes cancellation deferred (pthread_cleanup_push_defer_np)", kCleanup));
  t.push_back(make(KZ_P0_KIND_CLEANUP_POP, "cleanup handler", "__pthread_unregister_cancel", "pops the cleanup handler of the buffer arg 0 (pthread_cleanup_pop)", kCleanup));
  t.push_back(make(KZ_P0_KIND_CLEANUP_POP, "cleanup handler", "__pthread_unregister_cancel_restore",
                   "pops the cleanup handler of the buffer arg 0 and restores the cancellation type (pthread_cleanup_pop_restore_np)", kCleanup));
  // dynamic loading
  t.push_back(make(KZ_P0_KIND_DL_OPEN, "dynamic loading", "dlopen", "loads the object file named by arg 0 and returns a handle of it"));
  t.push_back(make(KZ_P0_KIND_DL_SYM, "dynamic loading", "dlsym",
                   "returns the address of the symbol named by arg 1 in the object arg 0 (or in the global scope)"));
  t.push_back(make(KZ_P0_KIND_DL_OPEN, "dynamic loading", "dlmopen", "loads the object file named by arg 1 into the namespace arg 0 and returns a handle of it",
                   "glibc 2.39 (GNU); not in POSIX.1-2017"));
  t.push_back(make(KZ_P0_KIND_DL_SYM, "dynamic loading", "dlvsym", "returns the address of the version arg 2 of the symbol named by arg 1 in the object arg 0",
                   "glibc 2.39 (GNU); not in POSIX.1-2017"));
  // data functions: the summaries every extraction applies, in their old order
  t.push_back(data("memset", 0, -1, 1, false, KZ_WRITER_KIND_MEMSET, "fills arg 2 bytes at arg 0 with the byte arg 1"));
  t.push_back(data("bzero", 0, -1, -1, true, KZ_WRITER_KIND_MEMSET, "fills arg 1 bytes at arg 0 with zero",
                   "glibc 2.39; not in POSIX.1-2017 (removed in Issue 7)"));
  t.push_back(data("memcpy", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY, "copies arg 2 bytes from arg 1 to arg 0"));
  t.push_back(data("memmove", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY, "copies arg 2 bytes from arg 1 to arg 0 (the areas may overlap)"));
  t.push_back(data("strcpy", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY, "copies the string arg 1 with its terminator to arg 0"));
  t.push_back(data("strncpy", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY, "copies at most arg 2 bytes of the string arg 1 to arg 0, padding with zero"));
  t.push_back(data("strcat", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY, "appends the string arg 1 to the string arg 0"));
  t.push_back(data("strncat", 0, 1, -1, false, KZ_WRITER_KIND_MEMCPY, "appends at most arg 2 bytes of the string arg 1 to the string arg 0"));
  t.push_back(data("sprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY, "writes the formatted string to arg 0"));
  t.push_back(data("snprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY, "writes at most arg 1 bytes of the formatted string to arg 0"));
  t.push_back(data("vsprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY, "writes the formatted string to arg 0"));
  t.push_back(data("vsnprintf", 0, -1, -1, false, KZ_WRITER_KIND_MEMCPY, "writes at most arg 1 bytes of the formatted string to arg 0"));
  return t;
}

}  // namespace

const std::vector<P0Entry>& p0_table() {
  static const std::vector<P0Entry> table = build();
  return table;
}

const char* p0_table_revision() { return "P0 table revision 2 (2026-10-30)"; }

const char* p0_table_scope() {
  return "families listed whole: thread creation, mutex and spin lock, semaphore, condition variable, sleep, termination of the calling thread or "
         "process, thread identity, thread-specific data, cleanup handlers, dynamic loading (open and symbol lookup), data functions; with the "
         "ISO C11 <threads.h> members of those families and the glibc 2.39 *_clock* members.  Not in this revision: read-write locks "
         "(pthread_rwlock_*; their shared acquisitions are not read by the lock-set analysis, so the family is left out whole and its "
         "acquisitions are no lock operations), barriers (pthread_barrier_*), one-time initialization (pthread_once, call_once), thread "
         "cancellation of another thread (pthread_cancel and the cancellation state functions), signals (kill, raise, pthread_kill, "
         "sigwait), dlclose, dlerror and dladdr, and functions a compiler or a fortify wrapper calls without a call in the source "
         "(__stack_chk_fail, __chk_fail and the *_chk functions)";
}

const std::vector<P0CompareEntry>& p0_compare_table() {
  static const std::vector<P0CompareEntry> table = {
      {"memcmp", "byte-string comparison", "compares arg 2 bytes at arg 0 and arg 1; returns zero exactly when they are equal", kPosix, kUnverified, true},
      {"strcmp", "byte-string comparison", "compares the strings arg 0 and arg 1; returns zero exactly when they are equal", kPosix, kUnverified, true},
      {"strcoll", "byte-string comparison", "compares the strings arg 0 and arg 1 in the collation of the locale; returns zero exactly when they collate equal", kPosix,
       kUnverified, true},
      {"strncmp", "byte-string comparison", "compares at most arg 2 bytes of the strings arg 0 and arg 1; returns zero exactly when they are equal", kPosix, kUnverified,
       true},
      {"strcasecmp", "byte-string comparison", "compares the strings arg 0 and arg 1 ignoring case; returns zero exactly when they are equal so", kPosix, kUnverified, true},
      {"strncasecmp", "byte-string comparison", "compares at most arg 2 bytes of the strings arg 0 and arg 1 ignoring case; returns zero exactly when they are equal so",
       kPosix, kUnverified, true},
      {"strcoll_l", "byte-string comparison", "as strcoll in the locale arg 2; returns zero exactly when they collate equal", kPosix, kUnverified, true},
      {"strcasecmp_l", "byte-string comparison", "as strcasecmp in the locale arg 2; returns zero exactly when they are equal so", kPosix, kUnverified, true},
      {"strncasecmp_l", "byte-string comparison", "as strncasecmp in the locale arg 3; returns zero exactly when they are equal so", kPosix, kUnverified, true},
      {"bcmp", "byte-string comparison", "compares arg 2 bytes at arg 0 and arg 1; returns zero exactly when they are equal",
       "glibc 2.39; not in POSIX.1-2017 (removed in Issue 7)", kUnverified, true},
      {"strverscmp", "byte-string comparison", "compares the strings arg 0 and arg 1 as version strings; returns zero exactly when they are equal",
       "glibc 2.39 (GNU); not in POSIX.1-2017", kUnverified, true},
      {"pthread_equal", "thread-id comparison", "compares the thread ids arg 0 and arg 1; returns non-zero exactly when they are equal", kPosix, kUnverified, false},
      {"thrd_equal", "thread-id comparison", "compares the thread ids arg 0 and arg 1; returns non-zero exactly when they are equal", kC11, kUnverified, false}};
  return table;
}

const char* p0_compare_table_revision() { return "P0 comparison table revision 2 (2026-10-30)"; }

const char* p0_compare_table_scope() {
  return "families listed whole: byte-string comparison (<string.h>, <strings.h>, with the locale _l members, bcmp and strverscmp), "
         "thread-id comparison (pthread_equal, thrd_equal).  Not in this revision: wide-character comparison (wcscmp, wcsncmp, wcscoll, "
         "wcscasecmp, wcsncasecmp, wmemcmp and their _l members) and __memcmpeq (called by a compiler without a call in the source)";
}

bool p0_is_delay(uint8_t kind) { return kind == KZ_P0_KIND_SLEEP; }
bool p0_is_terminal(uint8_t kind) { return kind == KZ_P0_KIND_THREAD_EXIT || kind == KZ_P0_KIND_PROCESS_EXIT; }
bool p0_is_process_terminal(uint8_t kind) { return kind == KZ_P0_KIND_PROCESS_EXIT; }

}  // namespace kzs

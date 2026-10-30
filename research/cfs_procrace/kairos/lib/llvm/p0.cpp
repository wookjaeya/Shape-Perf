// p0.cpp -- see p0.hpp.
#include "p0.hpp"

namespace kzs {

namespace {

constexpr const char* kPosix = "POSIX.1-2017 / glibc 2.39";
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
  // mutexes
  // the mutex argument (`mutex`, not written) is the one the semantics name
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "pthread_mutex_lock", "acquires the mutex arg 0, blocking until it is free"), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "pthread_mutex_trylock", "acquires the mutex arg 0 if it is free, else returns without it (EBUSY)")), 0));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_MUTEX_ACQUIRE, "mutex", "pthread_mutex_timedlock",
                                         "acquires the mutex arg 0, blocking at most until the absolute time arg 1; may return without it (ETIMEDOUT)")),
                        0));
  t.push_back(mutex_arg(make(KZ_P0_KIND_MUTEX_RELEASE, "mutex", "pthread_mutex_unlock", "releases the mutex arg 0"), 0));
  // semaphores
  t.push_back(make(KZ_P0_KIND_SEM_WAIT, "semaphore", "sem_wait", "decrements the semaphore arg 0, blocking while it is zero"));
  t.push_back(conditional(make(KZ_P0_KIND_SEM_WAIT, "semaphore", "sem_trywait", "decrements the semaphore arg 0 if it is not zero, else returns without (EAGAIN)")));
  t.push_back(conditional(make(KZ_P0_KIND_SEM_WAIT, "semaphore", "sem_timedwait",
                               "decrements the semaphore arg 0, blocking at most until the absolute time arg 1; may return without (ETIMEDOUT)")));
  t.push_back(make(KZ_P0_KIND_SEM_POST, "semaphore", "sem_post", "increments the semaphore arg 0; one thread blocked on it is woken"));
  // condition variables
  t.push_back(mutex_arg(make(KZ_P0_KIND_COND_WAIT, "condition variable", "pthread_cond_wait",
                             "releases the mutex arg 1, waits on the condition arg 0, and holds the mutex again when it returns"),
                        1));
  t.push_back(mutex_arg(conditional(make(KZ_P0_KIND_COND_WAIT, "condition variable", "pthread_cond_timedwait",
                                         "as pthread_cond_wait, waiting at most until the absolute time arg 2 (ETIMEDOUT); holds the mutex arg 1 again when it returns")),
                        1));
  t.push_back(make(KZ_P0_KIND_COND_SIGNAL, "condition variable", "pthread_cond_signal", "unblocks at least one thread waiting on the condition arg 0"));
  t.push_back(make(KZ_P0_KIND_COND_SIGNAL, "condition variable", "pthread_cond_broadcast", "unblocks every thread waiting on the condition arg 0"));
  // sleeping
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "clock_nanosleep",
                   "suspends the calling thread until the time arg 2 has passed on the clock arg 0 (an interval, or an absolute time when arg 1 has "
                   "TIMER_ABSTIME)"));
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "nanosleep", "suspends the calling thread for the interval arg 0"));
  t.push_back(make(KZ_P0_KIND_SLEEP, "sleep", "sleep", "suspends the calling thread for arg 0 seconds"));
  // termination
  t.push_back(make(KZ_P0_KIND_THREAD_EXIT, "termination", "pthread_exit", "ends the calling thread; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "exit", "ends the process after its exit handlers have run; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "_exit", "ends the process without running exit handlers; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "_Exit", "ends the process without running exit handlers; does not return"));
  t.push_back(make(KZ_P0_KIND_PROCESS_EXIT, "termination", "abort", "ends the process abnormally (SIGABRT); does not return"));
  // thread identity and thread-specific data
  t.push_back(make(KZ_P0_KIND_THREAD_SELF, "thread identity", "pthread_self", "returns the id of the calling thread"));
  t.push_back(make(KZ_P0_KIND_TSD_GET, "thread-specific data", "pthread_getspecific", "returns the value the calling thread bound to the key arg 0"));
  t.push_back(make(KZ_P0_KIND_TSD_SET, "thread-specific data", "pthread_setspecific", "binds the value arg 1 to the key arg 0 for the calling thread"));
  // dynamic loading
  t.push_back(make(KZ_P0_KIND_DL_OPEN, "dynamic loading", "dlopen", "loads the object file named by arg 0 and returns a handle of it"));
  t.push_back(make(KZ_P0_KIND_DL_SYM, "dynamic loading", "dlsym",
                   "returns the address of the symbol named by arg 1 in the object arg 0 (or in the global scope)"));
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

const char* p0_table_revision() { return "P0 table revision 1 (2026-10-30)"; }

bool p0_is_delay(uint8_t kind) { return kind == KZ_P0_KIND_SLEEP; }
bool p0_is_terminal(uint8_t kind) { return kind == KZ_P0_KIND_THREAD_EXIT || kind == KZ_P0_KIND_PROCESS_EXIT; }
bool p0_is_process_terminal(uint8_t kind) { return kind == KZ_P0_KIND_PROCESS_EXIT; }

}  // namespace kzs

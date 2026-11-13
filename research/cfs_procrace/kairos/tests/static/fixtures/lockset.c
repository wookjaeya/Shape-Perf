/* lockset.c -- F5 LOCKSET and the target-independent rules (kz_facts.h revision 8): lock operations
 * derived from the mutex primitives through wrappers (a table of mutexes indexed by a handle, an
 * operation that holds the lock exactly when it returns 0, one that holds it on every return), the
 * locks held at calls, reads and writes, the identity of a lock through a handle field written once
 * (a static instance) or twice (not one), the failure branch of an acquisition, the writer-set
 * completeness of a field whose sibling's address escapes, and a poll loop whose predicate reads a
 * field through a function it calls.  The library functions are declared in the shape POSIX gives
 * them, so that the fixture needs no header. */
typedef struct {
  long opaque[5];
} fx_mutex_t;
struct timespec {
  long tv_sec;
  long tv_nsec;
};
int pthread_mutex_lock(fx_mutex_t *mutex);
int pthread_mutex_trylock(fx_mutex_t *mutex);
int pthread_mutex_unlock(fx_mutex_t *mutex);
int nanosleep(const struct timespec *req, struct timespec *rem);
void fx_log(const char *text);
void fx_keep(int *where);

/* ---- a table of mutexes and the operations on a handle ---- */
struct fx_mrec {
  fx_mutex_t id;
  int free;
};
struct fx_mrec fx_mtab[4];

int fx_take(unsigned h) {
  int st;
  if (h >= 4u || fx_mtab[h].free != 0) return -35;
  st = pthread_mutex_lock(&fx_mtab[h].id);
  if (st == 22) return -6;
  return 0;
}

int fx_give(unsigned h) {
  if (h >= 4u || fx_mtab[h].free != 0) return -35;
  if (pthread_mutex_unlock(&fx_mtab[h].id) != 0) return -6;
  return 0;
}

int fx_try_take(unsigned h) { return pthread_mutex_trylock(&fx_mtab[h].id); }

int fx_create(unsigned *h) {
  *h = 1u;
  return 0;
}

/* ---- shared data: its mutex handle is written once, the other handle twice ---- */
struct fx_app {
  int used;
  int state;
  int id;
};
struct fx_shared {
  unsigned mutex;
  unsigned other;
  struct fx_app apps[4];
  int tasks[8];
  int count;
};
struct fx_shared fx_g;

int fx_get_internal(int slot) { return fx_g.tasks[slot]; }

void fx_lock_shared(void) {
  int s = fx_take(fx_g.mutex);
  if (s != 0) {
    (void)fx_get_internal(0);
    fx_log("take failed");
  }
}

void fx_unlock_shared(void) {
  if (fx_give(fx_g.mutex) != 0) fx_log("give failed");
}

int fx_get_id(int slot) {
  int v;
  fx_lock_shared();
  v = fx_g.tasks[slot];
  fx_unlock_shared();
  return v;
}

int fx_spawn(int slot);

int fx_app_create(int slot) {
  fx_lock_shared();
  fx_g.apps[slot].used = 1;
  fx_unlock_shared();
  if (fx_spawn(slot) != 0) return -1;
  fx_lock_shared();
  fx_g.tasks[slot] = slot;
  fx_g.count++;
  (void)fx_get_internal(slot);
  fx_unlock_shared();
  return 0;
}

void fx_set_other(unsigned h) { fx_g.other = h; }

int fx_other_reader(void) {
  int v;
  (void)fx_take(fx_g.other);
  v = fx_g.count;
  (void)fx_give(fx_g.other);
  return v;
}

int fx_try_reader(void) {
  int v = -1;
  if (fx_try_take(fx_g.mutex) == 0) {
    v = fx_g.count;
    (void)fx_give(fx_g.mutex);
  }
  return v;
}

/* ---- a poll loop: the predicate reads `state` directly and `used` through the function it calls ---- */
int fx_is_used(const struct fx_app *a) { return a->used != 0; }

void fx_delay(unsigned ms) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (long)ms * 1000000L;
  (void)nanosleep(&t, &t);
}

int fx_wait_state(int want) {
  int i, n;
  while (1) {
    n = 0;
    fx_lock_shared();
    for (i = 0; i < 4; i++)
      if (fx_is_used(&fx_g.apps[i]) && fx_g.apps[i].state < want) n++;
    fx_unlock_shared();
    if (n == 0) break;
    fx_delay(50u);
  }
  return 0;
}

/* a poll loop whose body writes what its predicate reads */
int fx_spin_count(void) {
  while (1) {
    fx_lock_shared();
    if (fx_g.count > 3) {
      fx_unlock_shared();
      break;
    }
    fx_g.count++;
    fx_unlock_shared();
    fx_delay(10u);
  }
  return 0;
}

/* ---- the sibling of a field: its address escapes, and an untyped store is in the program ---- */
struct fx_pair {
  int a;
  int b;
};
struct fx_pair fx_p;

void fx_expose(void) { fx_keep(&fx_p.b); }

void fx_bytes(unsigned char *q) { q[0] = 0; }

int fx_spawn(int slot) { return slot > 4 ? -1 : 0; }

int main(void) {
  (void)fx_create(&fx_g.mutex);
  fx_set_other(2u);
  fx_set_other(3u);
  fx_p.a = 1;
  fx_expose();
  fx_bytes((unsigned char *)&fx_g.tasks[7]);
  (void)fx_app_create(0);
  (void)fx_get_id(0);
  (void)fx_other_reader();
  (void)fx_try_reader();
  (void)fx_wait_state(2);
  (void)fx_spin_count();
  return fx_p.a;
}

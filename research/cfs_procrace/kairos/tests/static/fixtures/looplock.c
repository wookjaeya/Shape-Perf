/* looplock.c -- fixture for loop flag predicates and lock identity by handle (extract --loop-defs, --lock-handles;
 * relations rules revision 11, order rules revision 4; docs/static_facts.md 5v).
 *
 * A lock operation by handle: the mutex of a table element chosen by a call on the handle (the lock identity the lock-set
 * analysis gives is lost behind that call's result).  Two handle fields hold such handles:
 *   shared_global.mutex   created once (one writer instance that runs once): a static instance
 *   twice_global.mutex    created at two places (two writer instances): no static instance
 * The cases:
 *   h_*        the creator holds the shared handle's lock across the spawn and the write, and the child's read holds it
 *              (relations path C through the handle; HB-lock when the handle's semantics item is verified)
 *   p_*        the same through functions that take the handle as a parameter (the handle read at each call)
 *   w_*        the same with the twice-created handle (no static instance)
 *   task_*     a wait whose condition reads a flag set under a guard that reads a call result and a field read into a
 *              local (the flag rule and the value rule); the function returns the flag and its caller tests the result
 *   retry_*    a count-bounded retry whose exit tests a local holding a call result (the value rule)
 *   wait_reset a flag defined twice in the body (refused)
 *   wait_addr  a flag whose address is taken (refused)
 * The values are the fixture's; they are no experiment results. */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdint.h>
#include <time.h>

typedef uint32_t obj_id_t;

/* ---- a lock API by handle ------------------------------------------------------------------------------- */
typedef struct {
  pthread_mutex_t m;
  int used;
} MutRec;
static MutRec mut_table[4];

static uint32_t mut_index(obj_id_t id) { return id % 4u; }

int mut_create(obj_id_t *id, uint32_t slot) {
  if (id == 0) return -2;
  mut_table[slot % 4u].used = 1;
  pthread_mutex_init(&mut_table[slot % 4u].m, 0);
  *id = slot % 4u;
  return 0;
}

int mut_take(obj_id_t id) { return pthread_mutex_lock(&mut_table[mut_index(id)].m); }

int mut_give(obj_id_t id) { return pthread_mutex_unlock(&mut_table[mut_index(id)].m); }

/* ---- the handle fields ------------------------------------------------------------------------------------ */
typedef struct {
  obj_id_t mutex;
} Shared;
Shared shared_global;

typedef struct {
  obj_id_t mutex;
} Twice;
Twice twice_global;

void shared_lock(void) { (void)mut_take(shared_global.mutex); }
void shared_unlock(void) { (void)mut_give(shared_global.mutex); }
void twice_lock(void) { (void)mut_take(twice_global.mutex); }
void twice_unlock(void) { (void)mut_give(twice_global.mutex); }
void param_lock(obj_id_t h) { (void)mut_take(h); }
void param_unlock(obj_id_t h) { (void)mut_give(h); }

void init_locks(void) {
  (void)mut_create(&shared_global.mutex, 0);
  (void)mut_create(&twice_global.mutex, 1);
}

void again_twice(void) { (void)mut_create(&twice_global.mutex, 2); }

/* ---- h: path C through the shared handle -------------------------------------------------------------------- */
typedef struct {
  int used;
} HRec;
static HRec h_recs[4];

int h_used(void) {
  int r = 1;
  shared_lock();
  if (h_recs[0].used == 1) r = 0;
  shared_unlock();
  return r;
}

void *h_worker(void *arg) {
  (void)arg;
  (void)h_used();
  return 0;
}

int h_create(void) {
  pthread_t t;
  shared_lock();
  pthread_create(&t, 0, h_worker, 0);
  h_recs[0].used = 1;
  shared_unlock();
  return 0;
}

/* ---- p: the same through the handle passed as a parameter ------------------------------------------------- */
typedef struct {
  int used;
} PRec;
static PRec p_recs[4];

int p_used(void) {
  int r = 1;
  param_lock(shared_global.mutex);
  if (p_recs[0].used == 1) r = 0;
  param_unlock(shared_global.mutex);
  return r;
}

void *p_worker(void *arg) {
  (void)arg;
  (void)p_used();
  return 0;
}

int p_create(void) {
  pthread_t t;
  param_lock(shared_global.mutex);
  pthread_create(&t, 0, p_worker, 0);
  p_recs[0].used = 1;
  param_unlock(shared_global.mutex);
  return 0;
}

/* ---- w: the twice-created handle ----------------------------------------------------------------------------- */
typedef struct {
  int used;
} WRec;
static WRec w_recs[4];

int w_used(void) {
  int r = 1;
  twice_lock();
  if (w_recs[0].used == 1) r = 0;
  twice_unlock();
  return r;
}

void *w_worker(void *arg) {
  (void)arg;
  (void)w_used();
  return 0;
}

int w_create(void) {
  pthread_t t;
  twice_lock();
  pthread_create(&t, 0, w_worker, 0);
  w_recs[0].used = 1;
  twice_unlock();
  return 0;
}

/* ---- task: a flag wait and its caller -------------------------------------------------------------------------- */
typedef struct {
  int app_id;
  void (*entry)(void);
} TaskRec;
TaskRec task_table[4];
static int task_runs;

TaskRec *task_by_context(void) { return &task_table[0]; }

void task_child(void) { task_runs = 1; }

int get_task_function(void (**fp)(void)) {
  int rc = -5;
  int left = 1000;
  void (*e)(void) = 0;
  struct timespec ts = {0, 1000};
  while (1) {
    TaskRec *t;
    nanosleep(&ts, 0);
    shared_lock();
    t = task_by_context();
    if (t != 0) {
      e = t->entry;
      if (t->app_id != 0 && e != 0) {
        rc = 0;
      }
    }
    shared_unlock();
    if (rc == 0 || left <= 0) break;
    left -= 10;
  }
  if (fp != 0) *fp = e;
  return rc;
}

void *task_entry_point(void *arg) {
  void (*f)(void) = 0;
  (void)arg;
  if (get_task_function(&f) == 0 && f != 0) {
    shared_lock();
    task_runs = 2;
    shared_unlock();
    f();
  }
  return 0;
}

int task_start(void) {
  pthread_t t;
  int rc = pthread_create(&t, 0, task_entry_point, 0);
  shared_lock();
  task_table[0].entry = task_child;
  task_table[0].app_id = 1;
  shared_unlock();
  return rc;
}

/* ---- retry: a count-bounded retry on a call result -------------------------------------------------------------- */
typedef struct {
  const char *name;
  int id;
} Named;
Named named_table[2];

int name_lookup(const char *name, int *id) {
  if (named_table[0].name == name) {
    *id = named_table[0].id;
    return 0;
  }
  return -17;
}

int retry_lookup(int *id) {
  int ret = -17;
  int j;
  struct timespec ts = {0, 1000};
  for (j = 0; j < 25; ++j) {
    ret = name_lookup("x", id);
    if (ret != -17) break;
    nanosleep(&ts, 0);
  }
  return ret;
}

/* ---- refused flags -------------------------------------------------------------------------------------------- */
int ready_flag;

int wait_reset(void) {
  int ok = 0;
  int left = 100;
  struct timespec ts = {0, 1000};
  while (left > 0) {
    nanosleep(&ts, 0);
    ok = 0;
    if (ready_flag != 0) ok = 1;
    if (ok == 1) break;
    left--;
  }
  return ok;
}

void poke(int *p) {
  if (ready_flag != 0) *p = 1;
}

int wait_addr(void) {
  int flag = 0;
  int left = 100;
  struct timespec ts = {0, 1000};
  while (left > 0) {
    nanosleep(&ts, 0);
    poke(&flag);
    if (flag == 1) break;
    left--;
  }
  return flag;
}

int main(void) {
  int id = 0;
  init_locks();
  again_twice();
  (void)h_create();
  (void)p_create();
  (void)w_create();
  (void)task_start();
  (void)retry_lookup(&id);
  (void)wait_reset();
  (void)wait_addr();
  return 0;
}

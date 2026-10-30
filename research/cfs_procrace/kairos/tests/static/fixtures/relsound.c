/* relsound.c -- fixture for the soundness rules of `kz-static relations` path C (relations rules revision 3).
 *
 * Each creator spawns one thread and writes a field of its own record type after the spawn; the child reads the
 * field.  Every creator has its own record type, table, lock and child, so that each case gives its own rows:
 *   entry_*      the child's entry reads the field with no lock; the same function is also called under the lock by
 *                code the child does not run (the read run as the entry gets a row of its own; the other call no grade)
 *   via_*        the child reads through a function it calls directly under the lock and through a function pointer
 *                without it (one row per call: the indirect call is read too)
 *   clear_*      after the locked write the creator clears the whole record outside the lock (a record-wide write)
 *   loop_*       lock, write, spawn and unlock inside a loop: the write follows the spawn of the iteration before
 *                through the back edge, after a new acquisition at the same call
 *   ptr_*        spawn, write and read under one lock reached through a pointer with two writers (no static instance)
 *   hook_*       after the unlock the creator calls through a function pointer the facts do not resolve; at run time it
 *                is a function that writes the field
 *   after_*      the creator's caller writes the field after the creator returns
 *   held_*       the control: the lock is held from one acquisition across the spawn and the write, and the read holds it
 * The values are those of the fixture; they are not experiment results. */
#include <pthread.h>
#include <string.h>

/* ---- entry: the read is the spawned entry itself ---------------------------------------------------------- */
typedef struct { int ready; } EntryRec;
static EntryRec entry_recs[4];
static pthread_mutex_t entry_mutex = PTHREAD_MUTEX_INITIALIZER;

void *entry_worker(void *arg) {
  (void)arg;
  if (entry_recs[1].ready == 1) return &entry_recs[1];
  return 0;
}

void entry_locked_call(void) {
  pthread_mutex_lock(&entry_mutex);
  (void)entry_worker(0);
  pthread_mutex_unlock(&entry_mutex);
}

int entry_spawn(void) {
  pthread_t t;
  int rc;
  pthread_mutex_lock(&entry_mutex);
  rc = pthread_create(&t, 0, entry_worker, 0);
  entry_recs[1].ready = 1;
  pthread_mutex_unlock(&entry_mutex);
  return rc;
}

/* ---- via: a direct call under the lock and an indirect call without it ----------------------------------- */
typedef struct { int ready; } ViaRec;
static ViaRec via_recs[4];
static pthread_mutex_t via_mutex = PTHREAD_MUTEX_INITIALIZER;

int via_read(void) {
  if (via_recs[2].ready == 1) return 0;
  return -1;
}

static int (*via_ptr)(void) = via_read;

void *via_worker(void *arg) {
  (void)arg;
  (void)via_ptr();
  pthread_mutex_lock(&via_mutex);
  (void)via_read();
  pthread_mutex_unlock(&via_mutex);
  return 0;
}

int via_spawn(void) {
  pthread_t t;
  int rc;
  pthread_mutex_lock(&via_mutex);
  rc = pthread_create(&t, 0, via_worker, 0);
  via_recs[2].ready = 1;
  pthread_mutex_unlock(&via_mutex);
  return rc;
}

/* ---- clear: a record-wide write outside the lock after the locked write ----------------------------------- */
typedef struct { int ready; int stage; } ClearRec;
static ClearRec clear_recs[4];
static pthread_mutex_t clear_mutex = PTHREAD_MUTEX_INITIALIZER;

int clear_ready(int i) {
  int r = -1;
  pthread_mutex_lock(&clear_mutex);
  if (clear_recs[i].ready == 1) r = 0;
  pthread_mutex_unlock(&clear_mutex);
  return r;
}

void *clear_worker(void *arg) {
  (void)arg;
  (void)clear_ready(1);
  return 0;
}

int clear_spawn(void) {
  pthread_t t;
  int rc;
  pthread_mutex_lock(&clear_mutex);
  rc = pthread_create(&t, 0, clear_worker, 0);
  clear_recs[1].ready = 1;
  pthread_mutex_unlock(&clear_mutex);
  memset(&clear_recs[1], 0, sizeof clear_recs[1]);
  return rc;
}

/* ---- loop: released and taken again around the loop ------------------------------------------------------- */
typedef struct { int ready; } LoopRec;
static LoopRec loop_recs[4];
static pthread_mutex_t loop_mutex = PTHREAD_MUTEX_INITIALIZER;

int loop_ready(int i) {
  int r = -1;
  pthread_mutex_lock(&loop_mutex);
  if (loop_recs[i].ready == 1) r = 0;
  pthread_mutex_unlock(&loop_mutex);
  return r;
}

void *loop_worker(void *arg) {
  (void)arg;
  (void)loop_ready(0);
  return 0;
}

void loop_spawn(void) {
  int i;
  pthread_t t;
  for (i = 0; i < 4; i++) {
    pthread_mutex_lock(&loop_mutex);
    loop_recs[0].ready = i;
    pthread_create(&t, 0, loop_worker, 0);
    pthread_mutex_unlock(&loop_mutex);
  }
}

/* ---- ptr: one lock reached through a pointer with two writers --------------------------------------------- */
typedef struct { int ready; } PtrRec;
static PtrRec ptr_recs[4];
static pthread_mutex_t ptr_m1 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ptr_m2 = PTHREAD_MUTEX_INITIALIZER;
typedef struct { pthread_mutex_t *mp; } PtrCfg;
static PtrCfg ptr_cfg;

int ptr_ready(int i) {
  int r = -1;
  pthread_mutex_lock(ptr_cfg.mp);
  if (ptr_recs[i].ready == 1) r = 0;
  pthread_mutex_unlock(ptr_cfg.mp);
  return r;
}

void *ptr_worker(void *arg) {
  (void)arg;
  (void)ptr_ready(1);
  return 0;
}

int ptr_spawn(void) {
  pthread_t t;
  int rc;
  pthread_mutex_lock(ptr_cfg.mp);
  rc = pthread_create(&t, 0, ptr_worker, 0);
  ptr_recs[1].ready = 1;
  pthread_mutex_unlock(ptr_cfg.mp);
  return rc;
}

void ptr_pick(int k) {
  if (k) ptr_cfg.mp = &ptr_m1;
  else ptr_cfg.mp = &ptr_m2;
}

/* ---- hook: an unresolved call after the unlock ------------------------------------------------------------ */
typedef struct { int ready; } HookRec;
static HookRec hook_recs[4];
static pthread_mutex_t hook_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char hook_bytes[sizeof(void (*)(void))];

int hook_ready(int i) {
  int r = -1;
  pthread_mutex_lock(&hook_mutex);
  if (hook_recs[i].ready == 1) r = 0;
  pthread_mutex_unlock(&hook_mutex);
  return r;
}

void *hook_worker(void *arg) {
  (void)arg;
  (void)hook_ready(1);
  return 0;
}

static void hook_clear(void) { hook_recs[1].ready = 0; }

int hook_spawn(void) {
  void (*cb)(void);
  pthread_t t;
  int rc;
  pthread_mutex_lock(&hook_mutex);
  rc = pthread_create(&t, 0, hook_worker, 0);
  hook_recs[1].ready = 1;
  pthread_mutex_unlock(&hook_mutex);
  memcpy(&cb, hook_bytes, sizeof cb);
  cb();
  return rc;
}

void hook_install(void) {
  void (*f)(void) = hook_clear;
  memcpy(hook_bytes, &f, sizeof f);
}

/* ---- after: the caller writes the field after the creator returns ----------------------------------------- */
typedef struct { int used; } AfterRec;
static AfterRec after_recs[4];
static pthread_mutex_t after_mutex = PTHREAD_MUTEX_INITIALIZER;

int after_used(void) {
  int r;
  pthread_mutex_lock(&after_mutex);
  if (after_recs[0].used == 1) r = 0;
  else r = 5;
  pthread_mutex_unlock(&after_mutex);
  return r;
}

void *after_worker(void *arg) {
  (void)arg;
  (void)after_used();
  return 0;
}

int after_create(void) {
  pthread_t t;
  pthread_mutex_lock(&after_mutex);
  pthread_create(&t, 0, after_worker, 0);
  after_recs[0].used = 1;
  pthread_mutex_unlock(&after_mutex);
  return 0;
}

int after_start(void) {
  int r = after_create();
  after_recs[0].used = 0;
  return r;
}

/* ---- held: the control --------------------------------------------------------------------------------- */
typedef struct { int used; } HeldRec;
static HeldRec held_recs[4];
static pthread_mutex_t held_mutex = PTHREAD_MUTEX_INITIALIZER;

int held_used(void) {
  int r;
  pthread_mutex_lock(&held_mutex);
  if (held_recs[0].used == 1) r = 0;
  else r = 5;
  pthread_mutex_unlock(&held_mutex);
  return r;
}

void *held_worker(void *arg) {
  (void)arg;
  (void)held_used();
  return 0;
}

int held_create(void) {
  pthread_t t;
  pthread_mutex_lock(&held_mutex);
  pthread_create(&t, 0, held_worker, 0);
  held_recs[0].used = 1;
  pthread_mutex_unlock(&held_mutex);
  return 0;
}

int main(void) {
  (void)entry_spawn();
  entry_locked_call();
  (void)via_spawn();
  (void)clear_spawn();
  loop_spawn();
  ptr_pick(1);
  (void)ptr_spawn();
  hook_install();
  (void)hook_spawn();
  (void)after_start();
  (void)held_create();
  return 0;
}

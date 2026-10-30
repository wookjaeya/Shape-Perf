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
 * and, for relations rules revision 4:
 *   wait_*       a callee of the creator waits on the creator's lock (a condition wait releases it), then writes the field
 *   waitp_*      the same with the wait two calls below the creator, on a mutex passed as a parameter
 *   first_*      the creator releases its lock through the address of the object whose first member is the mutex
 *   union_*      the creator releases its lock through another member of a union that holds the mutex
 *   back_*       the child hands the reading function to a function without a body (it may call it back) and calls it
 *                through a pointer the facts do not resolve; it also calls it under the lock
 *   addr_*       the creator is called through a pointer the facts do not resolve; that caller writes the field after
 *                it returns
 *   jump_*       a callee of the creator jumps back (longjmp) to the creator's setjmp point, which releases the lock
 *                and writes the field
 *   elem_*       the field is an array of scalars; the creator sets one element after the unlock
 *   root_*       the child calls a function that is also the entry of another thread; that function reads with no lock
 *   desc_*       the child starts a thread whose code reads the field with no lock
 *   opn_*        the child starts a thread at a spawn call whose entry is not resolved; the reading function is also
 *                called where no known thread of the spawn reaches
 * and the cases of relations rules revision 5 at the end.  The values are the fixture's; they are no experiment results. */
#include <pthread.h>
#include <setjmp.h>
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

/* ---- wait: a condition wait in a callee releases the creator's lock ------------------------------------- */
typedef struct { int used; } WaitRec;
static WaitRec wait_recs[4];
static pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wait_cv = PTHREAD_COND_INITIALIZER;

int wait_used(void) {
  int r;
  pthread_mutex_lock(&wait_mutex);
  if (wait_recs[0].used == 2) r = 0;
  else r = 5;
  pthread_cond_signal(&wait_cv);
  pthread_mutex_unlock(&wait_mutex);
  return r;
}

void *wait_worker(void *arg) {
  (void)arg;
  (void)wait_used();
  return 0;
}

static void wait_finish(void) {
  pthread_cond_wait(&wait_cv, &wait_mutex);
  wait_recs[0].used = 2;
}

int wait_create(void) {
  pthread_t t;
  pthread_mutex_lock(&wait_mutex);
  pthread_create(&t, 0, wait_worker, 0);
  wait_recs[0].used = 1;
  wait_finish();
  pthread_mutex_unlock(&wait_mutex);
  return 0;
}

/* ---- waitp: the wait two calls below, on a mutex passed as a parameter ------------------------------------ */
typedef struct { int used; } WaitpRec;
static WaitpRec waitp_recs[4];
static pthread_mutex_t waitp_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waitp_cv = PTHREAD_COND_INITIALIZER;

int waitp_used(void) {
  int r;
  pthread_mutex_lock(&waitp_mutex);
  if (waitp_recs[0].used == 2) r = 0;
  else r = 5;
  pthread_cond_signal(&waitp_cv);
  pthread_mutex_unlock(&waitp_mutex);
  return r;
}

void *waitp_worker(void *arg) {
  (void)arg;
  (void)waitp_used();
  return 0;
}

static void waitp_block(pthread_mutex_t *m) { pthread_cond_wait(&waitp_cv, m); }

static void waitp_finish(void) {
  waitp_block(&waitp_mutex);
  waitp_recs[0].used = 2;
}

int waitp_create(void) {
  pthread_t t;
  pthread_mutex_lock(&waitp_mutex);
  pthread_create(&t, 0, waitp_worker, 0);
  waitp_recs[0].used = 1;
  waitp_finish();
  pthread_mutex_unlock(&waitp_mutex);
  return 0;
}

/* ---- first: one mutex named as the object and as its first member ---------------------------------------- */
typedef struct { int used; } FirstRec;
static FirstRec first_recs[4];
typedef struct { pthread_mutex_t lock; int count; } FirstObj;
static FirstObj first_obj = { PTHREAD_MUTEX_INITIALIZER, 0 };

int first_used(void) {
  int r;
  pthread_mutex_lock(&first_obj.lock);
  if (first_recs[0].used == 2) r = 0;
  else r = 5;
  pthread_mutex_unlock(&first_obj.lock);
  return r;
}

void *first_worker(void *arg) {
  (void)arg;
  (void)first_used();
  return 0;
}

static void first_release(void) { pthread_mutex_unlock((pthread_mutex_t *)&first_obj); }
static void first_take(void) { pthread_mutex_lock((pthread_mutex_t *)&first_obj); }

int first_create(void) {
  pthread_t t;
  pthread_mutex_lock(&first_obj.lock);
  pthread_create(&t, 0, first_worker, 0);
  first_recs[0].used = 1;
  first_release();
  first_recs[0].used = 2;
  first_take();
  pthread_mutex_unlock(&first_obj.lock);
  return 0;
}

/* ---- union: one mutex named as two members of a union ----------------------------------------------------- */
typedef struct { int used; } UnionRec;
static UnionRec union_recs[4];
typedef union { pthread_mutex_t a; pthread_mutex_t b; } UnionLock;
static UnionLock union_lock = { PTHREAD_MUTEX_INITIALIZER };

int union_used(void) {
  int r;
  pthread_mutex_lock(&union_lock.a);
  if (union_recs[0].used == 2) r = 0;
  else r = 5;
  pthread_mutex_unlock(&union_lock.a);
  return r;
}

void *union_worker(void *arg) {
  (void)arg;
  (void)union_used();
  return 0;
}

int union_create(void) {
  pthread_t t;
  pthread_mutex_lock(&union_lock.a);
  pthread_create(&t, 0, union_worker, 0);
  union_recs[0].used = 1;
  pthread_mutex_unlock(&union_lock.b);
  union_recs[0].used = 2;
  pthread_mutex_lock(&union_lock.b);
  pthread_mutex_unlock(&union_lock.a);
  return 0;
}

/* ---- back: called back by a function without a body, and through an unresolved pointer ------------------- */
typedef struct { int used; } BackRec;
static BackRec back_recs[4];
static pthread_mutex_t back_mutex = PTHREAD_MUTEX_INITIALIZER;
extern void back_apply(int (*fn)(void));
static unsigned char back_bytes[sizeof(int (*)(void))];

int back_read(void) {
  if (back_recs[0].used == 1) return 0;
  return -1;
}

void *back_worker(void *arg) {
  int (*cb)(void);
  (void)arg;
  back_apply(back_read);
  memcpy(&cb, back_bytes, sizeof cb);
  (void)cb();
  pthread_mutex_lock(&back_mutex);
  (void)back_read();
  pthread_mutex_unlock(&back_mutex);
  return 0;
}

int back_create(void) {
  pthread_t t;
  pthread_mutex_lock(&back_mutex);
  pthread_create(&t, 0, back_worker, 0);
  back_recs[0].used = 1;
  pthread_mutex_unlock(&back_mutex);
  return 0;
}

void back_install(void) {
  int (*p)(void) = back_read;
  memcpy(back_bytes, &p, sizeof p);
}

/* ---- addr: the creator called through an unresolved pointer -------------------------------------------------- */
typedef struct { int used; } AddrRec;
static AddrRec addr_recs[4];
static pthread_mutex_t addr_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char addr_bytes[sizeof(int (*)(void))];

int addr_used(void) {
  int r;
  pthread_mutex_lock(&addr_mutex);
  if (addr_recs[0].used == 2) r = 0;
  else r = 5;
  pthread_mutex_unlock(&addr_mutex);
  return r;
}

void *addr_worker(void *arg) {
  (void)arg;
  (void)addr_used();
  return 0;
}

int addr_create(void) {
  pthread_t t;
  pthread_mutex_lock(&addr_mutex);
  pthread_create(&t, 0, addr_worker, 0);
  addr_recs[0].used = 2;
  pthread_mutex_unlock(&addr_mutex);
  return 0;
}

void addr_install(void) {
  int (*f)(void) = addr_create;
  memcpy(addr_bytes, &f, sizeof f);
}

void addr_run(void) {
  int (*f)(void);
  memcpy(&f, addr_bytes, sizeof f);
  (void)f();
  addr_recs[0].used = 1;
}

/* ---- jump: a non-local jump back to the creator ------------------------------------------------------------ */
typedef struct { int used; } JumpRec;
static JumpRec jump_recs[4];
static pthread_mutex_t jump_mutex = PTHREAD_MUTEX_INITIALIZER;
static jmp_buf jump_buf;
static volatile int jump_fail;
static pthread_t jump_thread;

int jump_used(void) {
  int r;
  pthread_mutex_lock(&jump_mutex);
  if (jump_recs[0].used == 1) r = 0;
  else r = 5;
  pthread_mutex_unlock(&jump_mutex);
  return r;
}

void *jump_worker(void *arg) {
  (void)arg;
  (void)jump_used();
  return 0;
}

static void jump_check(void) {
  if (jump_fail) longjmp(jump_buf, 1);
}

int jump_create(void) {
  if (setjmp(jump_buf) != 0) {
    pthread_mutex_unlock(&jump_mutex);
    jump_recs[0].used = 1;
    return -1;
  }
  pthread_mutex_lock(&jump_mutex);
  pthread_create(&jump_thread, 0, jump_worker, 0);
  jump_check();
  jump_recs[0].used = 1;
  pthread_mutex_unlock(&jump_mutex);
  return 0;
}

/* ---- elem: an array of scalars set by element ------------------------------------------------------------ */
typedef struct { int flags[4]; int count; } ElemRec;
static ElemRec elem_rec;
static const int elem_zero[4] = { 0, 0, 0, 0 };
static pthread_mutex_t elem_mutex = PTHREAD_MUTEX_INITIALIZER;

int elem_ready(int k) {
  int r;
  pthread_mutex_lock(&elem_mutex);
  if (elem_rec.flags[k] == 1) r = 0;
  else r = 5;
  pthread_mutex_unlock(&elem_mutex);
  return r;
}

void *elem_worker(void *arg) {
  (void)arg;
  (void)elem_ready(1);
  return 0;
}

int elem_create(int k) {
  pthread_t t;
  pthread_mutex_lock(&elem_mutex);
  pthread_create(&t, 0, elem_worker, 0);
  memcpy(elem_rec.flags, elem_zero, sizeof elem_rec.flags);
  pthread_mutex_unlock(&elem_mutex);
  elem_rec.flags[k] = 1;
  return 0;
}

/* ---- root: the child calls another thread's entry ------------------------------------------------------- */
typedef struct { int used; } RootRec;
static RootRec root_recs[4];
static pthread_mutex_t root_mutex = PTHREAD_MUTEX_INITIALIZER;

int root_read(void) {
  if (root_recs[0].used == 1) return 0;
  return -1;
}

void root_locked(void) {
  pthread_mutex_lock(&root_mutex);
  (void)root_read();
  pthread_mutex_unlock(&root_mutex);
}

void *root_other(void *arg) {
  (void)arg;
  (void)root_read();
  return 0;
}

void *root_worker(void *arg) {
  (void)root_other(arg);
  root_locked();
  return 0;
}

int root_create(void) {
  pthread_t t;
  pthread_mutex_lock(&root_mutex);
  pthread_create(&t, 0, root_worker, 0);
  root_recs[0].used = 1;
  pthread_mutex_unlock(&root_mutex);
  return 0;
}

int root_also(void) {
  pthread_t t;
  return pthread_create(&t, 0, root_other, 0);
}

/* ---- desc: a thread the child starts reads with no lock -------------------------------------------------- */
typedef struct { int used; } DescRec;
static DescRec desc_recs[4];
static pthread_mutex_t desc_mutex = PTHREAD_MUTEX_INITIALIZER;

int desc_read(void) {
  if (desc_recs[0].used == 1) return 0;
  return -1;
}

void *desc_grand(void *arg) {
  (void)arg;
  (void)desc_read();
  return 0;
}

void *desc_worker(void *arg) {
  pthread_t g;
  (void)arg;
  pthread_mutex_lock(&desc_mutex);
  (void)desc_read();
  pthread_mutex_unlock(&desc_mutex);
  return pthread_create(&g, 0, desc_grand, 0) == 0 ? arg : 0;
}

int desc_create(void) {
  pthread_t t;
  pthread_mutex_lock(&desc_mutex);
  pthread_create(&t, 0, desc_worker, 0);
  desc_recs[0].used = 1;
  pthread_mutex_unlock(&desc_mutex);
  return 0;
}

/* ---- opn: the child starts a thread whose entry is not resolved ------------------------------------------ */
typedef struct { int used; } OpnRec;
static OpnRec opn_recs[4];
static pthread_mutex_t opn_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char opn_bytes[sizeof(void *(*)(void *))];

int opn_read(void) {
  if (opn_recs[0].used == 1) return 0;
  return -1;
}

void *opn_worker(void *arg) {
  void *(*e)(void *);
  pthread_t g;
  (void)arg;
  pthread_mutex_lock(&opn_mutex);
  (void)opn_read();
  pthread_mutex_unlock(&opn_mutex);
  memcpy(&e, opn_bytes, sizeof e);
  return pthread_create(&g, 0, e, 0) == 0 ? arg : 0;
}

void opn_elsewhere(void) { (void)opn_read(); }

int opn_create(void) {
  pthread_t t;
  pthread_mutex_lock(&opn_mutex);
  pthread_create(&t, 0, opn_worker, 0);
  opn_recs[0].used = 1;
  pthread_mutex_unlock(&opn_mutex);
  return 0;
}

/* ---- relations rules revision 5 (with lock-set rules revision 2 and the incomplete-source targets) ----------
 *   recur_*      after the spawn the creator calls a helper that calls the creator again (mutual recursion): the nested
 *                call takes the lock again at the same call site and writes the field
 *   memhook_*    after the spawn the creator calls, under the lock, a global function pointer set through memcpy to a
 *                function that releases and takes the lock again (the flow graph resolves only its initializer)
 *   dlhook_*     the same through a member set with the dlsym idiom (a store through a cast lvalue)
 *   cbq_*        after the spawn the creator calls qsort with a comparison function that releases and takes the lock
 *                again (a function without a body calls back the function passed to it)
 *   unres_*      the child calls a function pointer loaded from bytes (not resolved); at run time it is a function
 *                that calls the reading function with no lock
 *   unres2_*     the reading function is address-taken and the child reaches it only through such a call
 *   img_*        the spawn's entry is read from a union member of a static table whose other elements hold a function
 *                of the other member; the reading function is also called where no known thread reaches */
#include <dlfcn.h>
#include <stdlib.h>

/* ---- recur: the creator runs again after the spawn ------------------------------------------------------- */
typedef struct { int used; } RecurRec;
static RecurRec recur_recs[4];
static pthread_mutex_t recur_mutex = PTHREAD_MUTEX_INITIALIZER;

int recur_read(void) {
  int r;
  pthread_mutex_lock(&recur_mutex);
  if (recur_recs[0].used == 1) r = 0;
  else r = -1;
  pthread_mutex_unlock(&recur_mutex);
  return r;
}

void *recur_worker(void *arg) {
  (void)arg;
  (void)recur_read();
  return 0;
}

int recur_create(int depth);
static int recur_again(void) { return recur_create(1); }

int recur_create(int depth) {
  pthread_t t;
  pthread_mutex_lock(&recur_mutex);
  if (depth == 0) pthread_create(&t, 0, recur_worker, 0);
  recur_recs[0].used = depth;
  pthread_mutex_unlock(&recur_mutex);
  if (depth == 0) (void)recur_again();
  return 0;
}

/* ---- memhook: a hook set through memcpy releases the lock ------------------------------------------------ */
typedef struct { int used; } MemhookRec;
static MemhookRec memhook_recs[4];
static pthread_mutex_t memhook_mutex = PTHREAD_MUTEX_INITIALIZER;

static void memhook_noop(void) { }
static void memhook_relock(void) {
  pthread_mutex_unlock(&memhook_mutex);
  pthread_mutex_lock(&memhook_mutex);
}
static void (*memhook_fn)(void) = memhook_noop;

void memhook_install(void) {
  void (*p)(void) = memhook_relock;
  memcpy(&memhook_fn, &p, sizeof p);
}

int memhook_read(void) {
  int r;
  pthread_mutex_lock(&memhook_mutex);
  if (memhook_recs[0].used == 1) r = 0;
  else r = -1;
  pthread_mutex_unlock(&memhook_mutex);
  return r;
}

void *memhook_worker(void *arg) {
  (void)arg;
  (void)memhook_read();
  return 0;
}

int memhook_create(void) {
  pthread_t t;
  pthread_mutex_lock(&memhook_mutex);
  pthread_create(&t, 0, memhook_worker, 0);
  memhook_fn();
  memhook_recs[0].used = 1;
  pthread_mutex_unlock(&memhook_mutex);
  return 0;
}

/* ---- dlhook: a member set with the dlsym idiom ------------------------------------------------------------ */
typedef struct { int used; } DlhookRec;
static DlhookRec dlhook_recs[4];
static pthread_mutex_t dlhook_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct { void (*hook)(void); int n; } DlhookOps;

static void dlhook_noop(void) { }
static DlhookOps dlhook_ops = {dlhook_noop, 0};
static void *dlhook_handle;

void dlhook_relock(void) {
  pthread_mutex_unlock(&dlhook_mutex);
  pthread_mutex_lock(&dlhook_mutex);
}

void dlhook_install(void) { *(void **)(&dlhook_ops.hook) = dlsym(dlhook_handle, "dlhook_relock"); }

int dlhook_read(void) {
  int r;
  pthread_mutex_lock(&dlhook_mutex);
  if (dlhook_recs[0].used == 1) r = 0;
  else r = -1;
  pthread_mutex_unlock(&dlhook_mutex);
  return r;
}

void *dlhook_worker(void *arg) {
  (void)arg;
  (void)dlhook_read();
  return 0;
}

int dlhook_create(void) {
  pthread_t t;
  pthread_mutex_lock(&dlhook_mutex);
  pthread_create(&t, 0, dlhook_worker, 0);
  dlhook_ops.hook();
  dlhook_recs[0].used = 1;
  pthread_mutex_unlock(&dlhook_mutex);
  return 0;
}

/* ---- cbq: a function without a body calls back a function that releases the lock ------------------------- */
typedef struct { int used; } CbqRec;
static CbqRec cbq_recs[4];
static pthread_mutex_t cbq_mutex = PTHREAD_MUTEX_INITIALIZER;

static int cbq_cmp(const void *a, const void *b) {
  pthread_mutex_unlock(&cbq_mutex);
  pthread_mutex_lock(&cbq_mutex);
  return *(const int *)a - *(const int *)b;
}

int cbq_read(void) {
  int r;
  pthread_mutex_lock(&cbq_mutex);
  if (cbq_recs[0].used == 1) r = 0;
  else r = -1;
  pthread_mutex_unlock(&cbq_mutex);
  return r;
}

void *cbq_worker(void *arg) {
  (void)arg;
  (void)cbq_read();
  return 0;
}

int cbq_create(void) {
  pthread_t t;
  int keys[2] = {2, 1};
  pthread_mutex_lock(&cbq_mutex);
  pthread_create(&t, 0, cbq_worker, 0);
  qsort(keys, 2, sizeof keys[0], cbq_cmp);
  cbq_recs[0].used = 1;
  pthread_mutex_unlock(&cbq_mutex);
  return 0;
}

/* ---- unres: the child calls a pointer the facts do not resolve; it may run a function that reads unlocked -- */
typedef struct { int used; } UnresRec;
static UnresRec unres_recs[4];
static pthread_mutex_t unres_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char unres_bytes[sizeof(void (*)(void))];

int unres_read(void) {
  if (unres_recs[0].used == 1) return 0;
  return -1;
}

void unres_ctx(void) { (void)unres_read(); }

void *unres_worker(void *arg) {
  void (*cb)(void);
  (void)arg;
  memcpy(&cb, unres_bytes, sizeof cb);
  cb();
  pthread_mutex_lock(&unres_mutex);
  (void)unres_read();
  pthread_mutex_unlock(&unres_mutex);
  return 0;
}

int unres_create(void) {
  pthread_t t;
  pthread_mutex_lock(&unres_mutex);
  pthread_create(&t, 0, unres_worker, 0);
  unres_recs[0].used = 1;
  pthread_mutex_unlock(&unres_mutex);
  return 0;
}

void unres_install(void) {
  void (*p)(void) = unres_ctx;
  memcpy(unres_bytes, &p, sizeof p);
}

/* ---- unres2: the address-taken reader is reached only through such a call --------------------------------- */
typedef struct { int used; } Unres2Rec;
static Unres2Rec unres2_recs[4];
static pthread_mutex_t unres2_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char unres2_bytes[sizeof(int (*)(void))];

int unres2_read(void) {
  if (unres2_recs[0].used == 1) return 0;
  return -1;
}

void *unres2_worker(void *arg) {
  int (*cb)(void);
  (void)arg;
  memcpy(&cb, unres2_bytes, sizeof cb);
  (void)cb();
  return 0;
}

int unres2_create(void) {
  pthread_t t;
  pthread_mutex_lock(&unres2_mutex);
  pthread_create(&t, 0, unres2_worker, 0);
  unres2_recs[0].used = 1;
  pthread_mutex_unlock(&unres2_mutex);
  return 0;
}

void unres2_install(void) {
  int (*p)(void) = unres2_read;
  memcpy(unres2_bytes, &p, sizeof p);
}

/* ---- img: the entry from a union member of a static table ------------------------------------------------- */
typedef struct { int used; } ImgRec;
static ImgRec img_recs[4];
static pthread_mutex_t img_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef union { void (*init)(void); void *(*main)(void *); } ImgFn;
typedef struct { int kind; ImgFn fn; } ImgEntry;

int img_read(void) {
  if (img_recs[0].used == 1) return 0;
  return -1;
}

void *img_main(void *arg) {
  pthread_mutex_lock(&img_mutex);
  (void)img_read();
  pthread_mutex_unlock(&img_mutex);
  return arg;
}

static void img_init(void) { }
static const ImgEntry img_table[3] = {{0, {img_init}}, {1, {.main = img_main}}, {0, {img_init}}};
static int img_count = 3;

void img_elsewhere(void) { (void)img_read(); }

int img_create(void) {
  pthread_t t;
  int i;
  pthread_mutex_lock(&img_mutex);
  for (i = 0; i < img_count; i++) {
    if (img_table[i].kind == 1) pthread_create(&t, 0, img_table[i].fn.main, 0);
    else img_table[i].fn.init();
  }
  img_recs[0].used = 1;
  pthread_mutex_unlock(&img_mutex);
  return 0;
}

/* ---- varhook (revision 5, second round): a global function pointer set with the dlsym idiom, a store through a
 * cast lvalue of the variable ------------------------------------------------------------------------------- */
typedef struct { int used; } VarhookRec;
static VarhookRec varhook_recs[4];
static pthread_mutex_t varhook_mutex = PTHREAD_MUTEX_INITIALIZER;
static void varhook_noop(void) { }
static void (*varhook_fn)(void) = varhook_noop;
static void *varhook_handle;

void varhook_relock(void) {
  pthread_mutex_unlock(&varhook_mutex);
  pthread_mutex_lock(&varhook_mutex);
}

void varhook_install(void) { *(void **)(&varhook_fn) = dlsym(varhook_handle, "varhook_relock"); }

int varhook_read(void) {
  int r;
  pthread_mutex_lock(&varhook_mutex);
  if (varhook_recs[0].used == 1) r = 0;
  else r = -1;
  pthread_mutex_unlock(&varhook_mutex);
  return r;
}

void *varhook_worker(void *arg) {
  (void)arg;
  (void)varhook_read();
  return 0;
}

int varhook_create(void) {
  pthread_t t;
  pthread_mutex_lock(&varhook_mutex);
  pthread_create(&t, 0, varhook_worker, 0);
  varhook_fn();
  varhook_recs[0].used = 1;
  pthread_mutex_unlock(&varhook_mutex);
  return 0;
}

/* ---- putcopy (revision 5, second round): a helper with a body copies the pointer to the function pointer it
 * receives and stores through the copy ---------------------------------------------------------------------- */
typedef struct { int used; } PutcopyRec;
static PutcopyRec putcopy_recs[4];
static pthread_mutex_t putcopy_mutex = PTHREAD_MUTEX_INITIALIZER;
static void putcopy_noop(void) { }
static void (*putcopy_fn)(void) = putcopy_noop;

static void putcopy_relock(void) {
  pthread_mutex_unlock(&putcopy_mutex);
  pthread_mutex_lock(&putcopy_mutex);
}

static void putcopy_put(void (**out)(void)) {
  void (**q)(void) = out;
  *q = putcopy_relock;
}

void putcopy_install(void) { putcopy_put(&putcopy_fn); }

int putcopy_read(void) {
  int r;
  pthread_mutex_lock(&putcopy_mutex);
  if (putcopy_recs[0].used == 1) r = 0;
  else r = -1;
  pthread_mutex_unlock(&putcopy_mutex);
  return r;
}

void *putcopy_worker(void *arg) {
  (void)arg;
  (void)putcopy_read();
  return 0;
}

int putcopy_create(void) {
  pthread_t t;
  pthread_mutex_lock(&putcopy_mutex);
  pthread_create(&t, 0, putcopy_worker, 0);
  putcopy_fn();
  putcopy_recs[0].used = 1;
  pthread_mutex_unlock(&putcopy_mutex);
  return 0;
}

/* ---- arrhook (revision 5, second round): an array of function pointers decays to a pointer that a helper keeps
 * and stores through ------------------------------------------------------------------------------------------ */
typedef struct { int used; } ArrhookRec;
static ArrhookRec arrhook_recs[4];
static pthread_mutex_t arrhook_mutex = PTHREAD_MUTEX_INITIALIZER;
static void arrhook_noop(void) { }
static void (*arrhook_fns[2])(void);

static void arrhook_relock(void) {
  pthread_mutex_unlock(&arrhook_mutex);
  pthread_mutex_lock(&arrhook_mutex);
}

void arrhook_setup(void) { arrhook_fns[0] = arrhook_noop; }

void arrhook_install(void) {
  void (**p)(void) = arrhook_fns;
  p[0] = arrhook_relock;
}

int arrhook_read(void) {
  int r;
  pthread_mutex_lock(&arrhook_mutex);
  if (arrhook_recs[0].used == 1) r = 0;
  else r = -1;
  pthread_mutex_unlock(&arrhook_mutex);
  return r;
}

void *arrhook_worker(void *arg) {
  (void)arg;
  (void)arrhook_read();
  return 0;
}

int arrhook_create(void) {
  pthread_t t;
  pthread_mutex_lock(&arrhook_mutex);
  pthread_create(&t, 0, arrhook_worker, 0);
  arrhook_fns[0]();
  arrhook_recs[0].used = 1;
  pthread_mutex_unlock(&arrhook_mutex);
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
  (void)wait_create();
  (void)waitp_create();
  (void)first_create();
  (void)union_create();
  back_install();
  (void)back_create();
  addr_install();
  addr_run();
  (void)jump_create();
  (void)elem_create(1);
  (void)root_also();
  (void)root_create();
  (void)desc_create();
  opn_elsewhere();
  (void)opn_create();
  (void)recur_create(0);
  memhook_install();
  (void)memhook_create();
  dlhook_install();
  (void)dlhook_create();
  (void)cbq_create();
  unres_install();
  (void)unres_create();
  unres2_install();
  (void)unres2_create();
  img_elsewhere();
  (void)img_create();
  varhook_install();
  (void)varhook_create();
  putcopy_install();
  (void)putcopy_create();
  arrhook_setup();
  arrhook_install();
  (void)arrhook_create();
  return 0;
}

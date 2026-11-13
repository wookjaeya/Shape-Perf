/* relpaths.c -- a fixture for the order and entry-path facts (LOCK_PATHS) and the relations rules that read
 * them, over the facts of the profile-free mode with the rule targets (extract --no-profile --targets-rule
 * R-T1,R-T2):
 *
 *   order     a spawn inside a loop: a write before the spawn in the loop body follows it through the back
 *             edge; a write in the branch that excludes the spawn's branch does not follow it;
 *   callees   a write in a function the creator calls after the spawn, outside the lock, keeps the other
 *             order possible; a read in a function that releases its caller's lock before the read does not
 *             hold that lock;
 *   roots     a table of entries with a static image (a union member set by a designated initializer) that a
 *             spawn in a loop reads its entry from; a root that calls another root directly stays an actor; a
 *             root that only dispatches through a function pointer is a trampoline;
 *   ends      a thread-specific key with a destructor (the entry-path rule makes no claim); a terminal
 *             operation reached only under a guard on its argument, called with a value that fails the guard,
 *             with an endless loop on its other path; a spawn wrapper that stores through its argument after.
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NREC 4

typedef struct {
  int ready;
  int stage;
  int count;
} Rec;

static Rec recs[NREC];
static pthread_mutex_t rec_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---- the readers ---------------------------------------------------------------------------------------- */
int rec_ready(int i) {
  int r = -1;
  pthread_mutex_lock(&rec_mutex);
  if (recs[i].ready == 1) r = 0;
  pthread_mutex_unlock(&rec_mutex);
  return r;
}

int rec_stage(int i) {
  int r = -1;
  pthread_mutex_lock(&rec_mutex);
  if (recs[i].stage == 1) r = 0;
  pthread_mutex_unlock(&rec_mutex);
  return r;
}

/* releases the caller's lock, then reads */
int ready_unlocked(int i) {
  pthread_mutex_unlock(&rec_mutex);
  if (recs[i].ready == 1) return 0;
  return -1;
}

int check_ready(int i) {
  int r;
  pthread_mutex_lock(&rec_mutex);
  r = ready_unlocked(i);
  return r;
}

int rec_counted(int i) {
  if (recs[i].count == 0) return -1;
  return 0;
}

void *worker(void *arg) {
  (void)arg;
  (void)rec_ready(1);
  (void)rec_stage(2);
  (void)check_ready(1);
  (void)rec_counted(0);
  return 0;
}

/* ---- the creators --------------------------------------------------------------------------------------- */
void spawn_all(void) {
  int i;
  pthread_t t;
  for (i = 0; i < NREC; i++) {
    recs[i].count = 0; /* before the spawn in the body: after the spawn of the iteration before */
    pthread_create(&t, 0, worker, 0);
  }
}

int spawn_checked(int ok) {
  pthread_t t;
  int rc;
  pthread_mutex_lock(&rec_mutex);
  if (ok) {
    rc = pthread_create(&t, 0, worker, 0);
    recs[1].ready = 1;
  } else {
    pthread_mutex_unlock(&rec_mutex);
    recs[1].ready = 0; /* the branch without the spawn */
    return -1;
  }
  pthread_mutex_unlock(&rec_mutex);
  return rc;
}

void set_stage(int i, int v) { recs[i].stage = v; }

int spawn_staged(void) {
  pthread_t t;
  int rc;
  pthread_mutex_lock(&rec_mutex);
  rc = pthread_create(&t, 0, worker, 0);
  recs[2].stage = 1;
  pthread_mutex_unlock(&rec_mutex);
  set_stage(2, 2); /* a helper writes the field after the spawn, outside the lock */
  return rc;
}

/* ---- the roots ------------------------------------------------------------------------------------------- */
typedef union {
  void (*call)(void);
  void *(*main)(void *);
} EntryUnion;

typedef struct {
  int kind;
  const char *name;
  EntryUnion u;
} EntryDef;

void *alpha_main(void *arg) {
  (void)arg;
  (void)rec_ready(1);
  return 0;
}

void *beta_main(void *arg) {
  (void)arg;
  (void)rec_stage(2);
  return 0;
}

static void early_init(void) { recs[3].count = 1; }

static EntryDef entry_defs[3] = {
    {.kind = 0, .name = "early", .u.call = early_init},
    {.kind = 1, .name = "alpha", .u.main = alpha_main},
    {.kind = 1, .name = "beta", .u.main = beta_main},
};

void start_table(void) {
  int i;
  pthread_t t;
  for (i = 0; i < 3; i++) {
    if (entry_defs[i].kind == 0) entry_defs[i].u.call();
    else pthread_create(&t, 0, entry_defs[i].u.main, 0);
  }
}

void *helper_main(void *arg) {
  (void)arg;
  (void)rec_counted(0);
  return 0;
}

/* a root that calls another root directly: it stays an actor of its own */
void *boss_main(void *arg) {
  (void)helper_main(arg);
  return 0;
}

static void *(*real_entry)(void *);

void *(*get_entry(void))(void *) { return real_entry; }

/* a root that only dispatches through a function pointer: a trampoline */
void *tramp_main(void *arg) {
  void *(*f)(void *) = get_entry();
  return f(arg);
}

/* ---- the ends --------------------------------------------------------------------------------------------- */
typedef struct {
  int used;
  char name[8];
  pthread_t id;
} Task;

static Task tasks[NREC];
static pthread_key_t task_key;

typedef struct {
  int run_status;
} App;

static App app = {1};

static void task_key_dtor(void *p) {
  (void)p;
  recs[0].stage = 0;
}

int task_find(const char *name) {
  int i;
  for (i = 0; i < NREC; i++)
    if (tasks[i].used == 1 && strcmp(tasks[i].name, name) == 0) return i;
  return -1;
}

int task_create(const char *name, void *(*fn)(void *)) {
  int i;
  for (i = 0; i < NREC; i++)
    if (tasks[i].used == 0) {
      tasks[i].used = 1;
      strncpy(tasks[i].name, name, 7);
      pthread_create(&tasks[i].id, 0, fn, 0);
      return i;
    }
  return -1;
}

void task_release(int i) { tasks[i].used = 0; }

/* ends the process only for status 9; otherwise waits for good */
void app_exit(int status) {
  if (status == 9) exit(1);
  for (;;) sleep(1);
}

void *app_main(void *arg) {
  int st;
  (void)arg;
  st = task_find("svc");
  if (st < 0) app.run_status = 3;
  app_exit(app.run_status);
  return 0;
}

void *svc_main(void *arg) {
  (void)arg;
  return 0;
}

/* ---- a derived spawn operation that stores through its argument after it created the thread ---------------- */
typedef struct {
  int started;
} Marks;

static Marks marks;

void *watcher_main(void *arg) {
  (void)arg;
  if (marks.started == 1) return &marks;
  return 0;
}

int spawn_marked(int *flag, void *(*fn)(void *)) {
  pthread_t t;
  int rc = pthread_create(&t, 0, fn, 0);
  *flag = 1;
  return rc;
}

int main(void) {
  pthread_t a, b, c;
  pthread_key_create(&task_key, task_key_dtor);
  spawn_all();
  (void)spawn_checked(1);
  (void)spawn_staged();
  start_table();
  pthread_create(&a, 0, helper_main, 0);
  pthread_create(&b, 0, boss_main, 0);
  real_entry = helper_main;
  pthread_create(&c, 0, tramp_main, 0);
  (void)task_create("app", app_main);
  (void)task_create("svc", svc_main);
  task_release(1);
  (void)spawn_marked(&marks.started, watcher_main);
  return 0;
}

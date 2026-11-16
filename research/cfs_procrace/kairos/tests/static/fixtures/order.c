/* order.c -- a fixture for `kz-static order` (order rules revisions 1 and 2) over the facts of the profile-free mode with the
 * rule targets (extract --no-profile --targets-rule R-T1,R-T2).  Synthetic, for the regression of the component only.
 *
 *   name table   a registration table searched by name (strcmp in a loop over a global array) with a lookup and a create
 *                API (relations path A):
 *     early_*    the parent creates a name, then spawns the thread that looks it up (the creation before the spawn: hard);
 *     probe_*    the parent looks a name up, then spawns the thread that creates it (the lookup before the spawn: the use
 *                fixed early);
 *     late_*     the parent spawns the thread that creates a name, then looks it up (no rule orders them: unordered);
 *   task table   a record written after the spawn, read by the child at the slot of its own thread id (relations path C):
 *     held_*     one lock held from one acquisition across the spawn and the write, held by the read (HB-lock: hard);
 *     window_*   the spawn outside the lock, the write under it (the lock window: unordered);
 *     wrapped_*  the spawn through an operation that returns a failure after the thread was created (child without
 *                success: possible), one that returns the primitive's result (not found), and one over the first that
 *                maps every inner failure to its own constant (possible, through the inner call);
 *   config       a field written before the spawn of the thread that passes it to a function checking it (relations path B:
 *                the write before the spawn, hard);
 *   gate         a flag polled with a delay by another thread (a recognised predicate, an atomic read) and then passed to
 *                the check (relations path B): skip_* returns on the budget exit (the use after the wait is skipped: hard),
 *                cont_* leaves the loop on the budget exit and goes on to the use (continue: conditional);
 *   apps         a table of entries started in a loop (the iterations and the observed-only order of the two starts);
 *   peers        (order rules revision 2) two threads that start their own children through one spawn wrapper, which
 *                writes a field before its spawn call: in the call of the thread whose child reads the field the write is
 *                before the read (hard); in the other thread's call it is not, since that call creates another child
 *                (the wrapper's spawn call is caller-dependent);
 *   local        (order rules revision 2) a spawn operation whose failure return is guarded by a local that holds the
 *                primitive's result (two definitions): the witness rests on the source position alone (child without
 *                success: unknown (position-based)).
 * The values are the fixture's; they are no experiment results. */
#include <pthread.h>
#include <signal.h>
#include <string.h>

int usleep(unsigned int usec); /* POSIX.1-2001 (XSI); declared here: <unistd.h> leaves it out under -std=c99 */

#define NSLOTS 4
#define NAMELEN 16
#define ERR_NOT_FOUND (-17)
#define ERR_ILLEGAL (-5)
#define ERR_TIMEOUT (-9)
#define ERR_SETUP (-11)
#define ERR_MAPPED (-12)

/* ---- the name table (path A) ----------------------------------------------------------------------------- */
typedef struct {
  int used;
  char name[NAMELEN];
} Name;

static Name names[NSLOTS];

int name_find(const char *name, int *out) {
  int i;
  for (i = 0; i < NSLOTS; i++) {
    if (names[i].used == 1 && strcmp(names[i].name, name) == 0) {
      *out = i;
      return 0;
    }
  }
  return ERR_NOT_FOUND;
}

int name_create(const char *name, int *out) {
  int i;
  for (i = 0; i < NSLOTS; i++) {
    if (names[i].used == 1 && strcmp(names[i].name, name) == 0) {
      return ERR_ILLEGAL;
    }
  }
  for (i = 0; i < NSLOTS; i++) {
    if (names[i].used == 0) {
      strncpy(names[i].name, name, NAMELEN - 1);
      names[i].used = 1;
      *out = i;
      return 0;
    }
  }
  return ERR_NOT_FOUND;
}

void *early_child(void *arg) {
  int h = 0;
  (void)arg;
  (void)name_find("early", &h);
  return 0;
}

void *probe_child(void *arg) {
  int h = 0;
  (void)arg;
  (void)name_create("probe", &h);
  return 0;
}

void *late_child(void *arg) {
  int h = 0;
  (void)arg;
  (void)name_create("late", &h);
  return 0;
}

int names_start(void) {
  pthread_t t1, t2, t3;
  int h = 0;
  (void)name_create("early", &h);
  pthread_create(&t1, 0, early_child, 0);
  (void)name_find("probe", &h);
  pthread_create(&t2, 0, probe_child, 0);
  pthread_create(&t3, 0, late_child, 0);
  (void)name_find("late", &h);
  return 0;
}

/* ---- the task table (path C) ----------------------------------------------------------------------------- */
typedef struct {
  int registered;
  pthread_t id;
} Task;

static Task tasks[NSLOTS];
static pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t self_key;

int task_self(void) {
  return (int)(long)pthread_getspecific(self_key);
}

int task_check(void) {
  int r = ERR_NOT_FOUND;
  pthread_mutex_lock(&task_mutex);
  if (tasks[task_self()].registered == 1) r = 0;
  pthread_mutex_unlock(&task_mutex);
  return r;
}

void *task_child(void *arg) {
  (void)arg;
  (void)task_check();
  return 0;
}

int held_spawn(int slot) {
  int rc;
  pthread_mutex_lock(&task_mutex);
  rc = pthread_create(&tasks[slot].id, 0, task_child, 0);
  tasks[slot].registered = 1;
  pthread_mutex_unlock(&task_mutex);
  return rc;
}

int window_spawn(int slot) {
  int rc = pthread_create(&tasks[slot].id, 0, task_child, 0);
  pthread_mutex_lock(&task_mutex);
  tasks[slot].registered = 1;
  pthread_mutex_unlock(&task_mutex);
  return rc;
}

/* ---- spawn operations: a failure after the thread was created, and the primitive's result ------------------ */
typedef struct {
  int registered;
  pthread_t id;
} WTask;

static WTask wtasks[NSLOTS];
static pthread_mutex_t wtask_mutex = PTHREAD_MUTEX_INITIALIZER;
static int setup_ready;

int wtask_check(void) {
  int r = ERR_NOT_FOUND;
  pthread_mutex_lock(&wtask_mutex);
  if (wtasks[task_self()].registered == 1) r = 0;
  pthread_mutex_unlock(&wtask_mutex);
  return r;
}

void *wrapped_child(void *arg) {
  (void)arg;
  (void)wtask_check();
  return 0;
}

int start_checked(pthread_t *out, void *(*entry)(void *)) {
  if (pthread_create(out, 0, entry, 0) != 0) return ERR_ILLEGAL;
  if (setup_ready == 0) return ERR_SETUP;
  return 0;
}

int start_plain(pthread_t *out, void *(*entry)(void *)) {
  return pthread_create(out, 0, entry, 0);
}

/* an operation over start_checked that maps every inner failure to its own constant: the inner call may fail after its
 * thread started, so the mapped failure is a child without success too */
int start_mapped(pthread_t *out, void *(*entry)(void *)) {
  int rc = start_checked(out, entry);
  if (rc != 0) return ERR_MAPPED;
  return 0;
}

int wrapped_spawn(int slot) {
  int rc;
  pthread_mutex_lock(&wtask_mutex);
  rc = start_checked(&wtasks[slot].id, wrapped_child);
  if (rc == 0) wtasks[slot].registered = 1;
  pthread_mutex_unlock(&wtask_mutex);
  return rc;
}

int plain_spawn(int slot) {
  int rc;
  pthread_mutex_lock(&wtask_mutex);
  rc = start_plain(&wtasks[slot].id, wrapped_child);
  if (rc == 0) wtasks[slot].registered = 1;
  pthread_mutex_unlock(&wtask_mutex);
  return rc;
}

int mapped_spawn(int slot) {
  int rc;
  pthread_mutex_lock(&wtask_mutex);
  rc = start_mapped(&wtasks[slot].id, wrapped_child);
  if (rc == 0) wtasks[slot].registered = 1;
  pthread_mutex_unlock(&wtask_mutex);
  return rc;
}

/* ---- a configuration field written before the spawn (path B) ----------------------------------------------- */
typedef struct {
  int level;
} Config;

static Config config;

int level_check(int level) {
  if (level == 0) return ERR_ILLEGAL;
  return level;
}

void *config_reader(void *arg) {
  (void)arg;
  (void)level_check(config.level);
  return 0;
}

void *config_writer(void *arg) {
  (void)arg;
  config.level = 0;
  return 0;
}

/* ---- a flag polled with a delay, then passed to the check (path B) --------------------------------------------- */
typedef struct {
  volatile sig_atomic_t ready;
} Gate;

static Gate gate;

void *gate_main(void *arg) {
  (void)arg;
  gate.ready = 1;
  return 0;
}

int skip_wait(void) {
  int budget = 10;
  while (gate.ready == 0) {
    if (budget <= 0) return ERR_TIMEOUT;
    usleep(1000);
    budget--;
  }
  return level_check(gate.ready);
}

int cont_wait(void) {
  int budget = 10;
  while (gate.ready == 0 && budget > 0) {
    usleep(1000);
    budget--;
  }
  return level_check(gate.ready);
}

void *waiter_main(void *arg) {
  (void)arg;
  (void)skip_wait();
  (void)cont_wait();
  return 0;
}

/* ---- a table of entries started in a loop ----------------------------------------------------------------- */
void *app_one(void *arg) {
  (void)arg;
  return 0;
}

void *app_two(void *arg) {
  (void)arg;
  return 0;
}

typedef struct {
  void *(*entry)(void *);
} App;

static const App app_a = {app_one};
static const App app_b = {app_two};
static const App *const apps[] = {&app_a, &app_b, 0};

void apps_start(void) {
  unsigned long i;
  const App *u;
  pthread_t t;
  for (i = 0;; i++) {
    u = apps[i];
    if (u == 0) break;
    pthread_create(&t, 0, u->entry, 0);
  }
}

/* ---- two threads, one spawn wrapper that writes before its spawn call (order rules revision 2) ------------- */
typedef struct {
  int stamp;
} Peer;

static Peer peer;

int spawn_via(pthread_t *out, void *(*entry)(void *)) {
  peer.stamp = 0;
  return pthread_create(out, 0, entry, 0);
}

void *peer_reader(void *arg) {
  (void)arg;
  (void)level_check(peer.stamp);
  return 0;
}

void *peer_quiet(void *arg) {
  (void)arg;
  return 0;
}

void *peer_one(void *arg) {
  pthread_t t;
  (void)arg;
  (void)spawn_via(&t, peer_quiet);
  return 0;
}

void *peer_two(void *arg) {
  pthread_t t;
  (void)arg;
  (void)spawn_via(&t, peer_reader);
  return 0;
}

/* ---- a failure return guarded by a local that holds the primitive's result (order rules revision 2) -------- */
int start_local(pthread_t *out, void *(*entry)(void *)) {
  int rc = ERR_SETUP;
  rc = pthread_create(out, 0, entry, 0);
  if (rc != 0) return ERR_ILLEGAL;
  return 0;
}

void *local_child(void *arg) {
  (void)arg;
  return 0;
}

int main(void) {
  pthread_t a, b, c, d, e, f, g;
  (void)names_start();
  (void)held_spawn(0);
  (void)window_spawn(1);
  (void)wrapped_spawn(2);
  (void)plain_spawn(3);
  (void)mapped_spawn(0);
  config.level = 3;
  pthread_create(&a, 0, config_reader, 0);
  pthread_create(&b, 0, config_writer, 0);
  pthread_create(&c, 0, gate_main, 0);
  pthread_create(&d, 0, waiter_main, 0);
  apps_start();
  pthread_create(&e, 0, peer_one, 0);
  pthread_create(&f, 0, peer_two, 0);
  (void)start_local(&g, local_child);
  return 0;
}

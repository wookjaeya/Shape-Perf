/* planv4.c -- a fixture for `kz-static plan --manifest 4` (plan rules revision 3, docs/plan_manifest.md sections 7-8)
 * over the facts of the profile-free mode with the rule targets and the extraction options (--object-access, --semantics
 * with the fixture data of data/planv4_fixture/, --result-flow, --loop-defs):
 *
 *   the registration table of plangen.c (a lookup API with a non-null test, a create API with a bounded NUL search),
 *   and a delete API that clears the record through a common retire function;
 *   an event send whose argument 1 is the event type (the fixture semantics data names that role);
 *   two "apps" and a third, each a thread started through a spawn operation by a start function that writes the owner
 *   of the task after the spawn under a mutex; app two reads the owner of its own task under the same mutex,
 *   subscripted by the result of a call (a path C read with a call subscript);
 *   app one looks a name up (its failure sends an event, stores the run status and reaches the app exit) and retries
 *   another lookup with a delay until it does not return the absence value (a wait whose predicate is a local that holds
 *   the call result);
 *   app two creates the name and a second name (the second is a create of another key constant);
 *   app three looks a name up and creates it when the lookup fails (a probe-create row).
 */
#define _DEFAULT_SOURCE 1 /* usleep under -std=c99 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NREGS 4
#define NAMELEN 16
#define ERR_NOT_FOUND (-17)
#define ERR_BAD_POINTER (-2)
#define ERR_NAME_TOO_LONG (-13)
#define ERR_NO_FREE (-14)
#define ERR_BAD_ID (-16)
#define RUN 1
#define RUN_ERROR 3
#define EV_INFO 2
#define EV_ERROR 3

typedef struct {
  int used;
  char name[NAMELEN];
} Reg;

static Reg regs[NREGS];

static int reg_search(const char *name, int *out) {
  int i;
  for (i = 0; i < NREGS; i++) {
    if (regs[i].used == 1 && strcmp(regs[i].name, name) == 0) {
      *out = i;
      return 0;
    }
  }
  return ERR_NOT_FOUND;
}

int reg_find(int *out, const char *name) {
  if (out == 0) return ERR_BAD_POINTER;
  if (name == 0) return ERR_BAD_POINTER;
  return reg_search(name, out);
}

int reg_create(int *out, const char *name) {
  int i;
  if (out == 0 || name == 0) return ERR_BAD_POINTER;
  if (memchr(name, 0, NAMELEN) == 0) return ERR_NAME_TOO_LONG;
  if (reg_search(name, &i) == 0) return ERR_NOT_FOUND;
  for (i = 0; i < NREGS; i++) {
    if (regs[i].used == 0) {
      strncpy(regs[i].name, name, NAMELEN - 1);
      regs[i].used = 1;
      *out = i;
      return 0;
    }
  }
  return ERR_NO_FREE;
}

static int reg_retire(int status, int id) {
  if (status == 0) {
    regs[id].used = 0;
    regs[id].name[0] = 0;
  }
  return status;
}

int reg_delete(int id) {
  int status = ERR_BAD_ID;
  if (id >= 0 && id < NREGS && regs[id].used == 1) status = 0;
  status = reg_retire(status, id);
  return status;
}

/* ---- the framework layer of the fixture: the run loop, the app exit, the event send -------------------- */
int run_loop(int *run_status) { return run_status != 0 && *run_status == RUN; }

void app_exit(int status) { exit(status); }

int send_event(int id, int type) { return id + type; }

/* ---- the spawn operation: the owner is written after the creation, under the task mutex ----------------- */
typedef struct {
  pthread_t thread;
  int owner;
} Task;

static Task tasks[4];
static pthread_mutex_t task_lock = PTHREAD_MUTEX_INITIALIZER;

int spawn_task(void *(*entry)(void *), long id) { return pthread_create(&tasks[id].thread, 0, entry, (void *)id); }

int start_app(void *(*entry)(void *), long id, int owner) {
  int st;
  pthread_mutex_lock(&task_lock);
  st = spawn_task(entry, id);
  if (st == 0) tasks[id].owner = owner;
  pthread_mutex_unlock(&task_lock);
  return st;
}

static int task_index(void) {
  int i;
  for (i = 0; i < 4; i++)
    if (pthread_equal(tasks[i].thread, pthread_self())) return i;
  return 0;
}

/* ---- app one: the dependent lookup and the retried lookup -------------------------------------------- */
typedef struct {
  int run_status;
  int handle, h2;
} AppData;

static AppData one;

static int one_init(void) {
  int status = reg_find(&one.handle, "lime");
  if (status != 0) {
    (void)send_event(7, EV_ERROR);
    return status;
  }
  return 0;
}

void *one_main(void *arg) {
  int status, j, ret;
  (void)arg;
  one.run_status = RUN;
  status = one_init();
  if (status != 0) {
    one.run_status = RUN_ERROR;
  }
  ret = ERR_NOT_FOUND;
  for (j = 0; j < 3; ++j) {
    ret = reg_find(&one.h2, "pear");
    if (ret != ERR_NOT_FOUND) {
      break;
    }
    usleep(1000);
  }
  if (ret != 0) (void)send_event(8, EV_INFO);
  while (run_loop(&one.run_status)) {
    one.handle = 0;
  }
  app_exit(one.run_status);
  return 0;
}

/* ---- app two: the creator, and the read of its own task's owner -------------------------------------- */
static int two_handle, two_second;

static int own_owner(void) {
  int r = 0;
  pthread_mutex_lock(&task_lock);
  if (tasks[task_index()].owner == 20) r = 1;
  pthread_mutex_unlock(&task_lock);
  return r;
}

void *two_main(void *arg) {
  int run_status = RUN;
  (void)arg;
  (void)reg_create(&two_handle, "lime");
  (void)reg_create(&two_second, "plum");
  (void)reg_create(&two_second, "pear");
  if (own_owner() == 0) (void)reg_delete(two_second);
  while (run_loop(&run_status)) {
    two_handle = two_handle + 0;
  }
  app_exit(run_status);
  return 0;
}

/* ---- app three: the probe and create of one name -------------------------------------------------------- */
typedef struct {
  int handle;
  char name[NAMELEN];
} ThreeCfg;

static ThreeCfg three = {0, "fig"};

void *three_main(void *arg) {
  int run_status = RUN;
  (void)arg;
  if (reg_find(&three.handle, three.name) != 0) {
    (void)reg_create(&three.handle, three.name);
  }
  while (run_loop(&run_status)) {
    three.handle = three.handle + 0;
  }
  app_exit(run_status);
  return 0;
}

int main(void) {
  (void)start_app(one_main, 1, 10);
  (void)start_app(two_main, 2, 20);
  (void)start_app(three_main, 3, 30);
  return 0;
}

/* resultflow.c -- a fixture for result flow and effects (extract --no-profile --result-flow, static_facts.md 5u), relations
 * rules revision 10 and order rules revision 3, over the facts of the profile-free mode with the rule targets
 * (extract --no-profile --targets-rule R-T1,R-T2).  Synthetic, for the regression of the component only.
 *
 *   name table   a registration table searched by name (strcmp in a loop over a global array) with a lookup and a create
 *                API (relations path A); the create returns the result of a finishing step that gives its status
 *                parameter back unchanged (the success value is shown to reach the create's return only through it);
 *   checker      (two definitions of one local) the local first holds a create's result and then the lookup's, which is
 *                tested only for success: the absence value falls through to a success return (label T);
 *   reporter     the lookup's failure is returned as the function's own failure constant to a caller that leaves the
 *                thread on it through a terminal (label R, through the return);
 *   looper       a loop that leaves on the lookup's failure after a report and returns the lookup's own result (the
 *                failure value reaches the return through the break);
 *   retrier      the lookup again after a delay on its failure (a retry, label H);
 *   runner       the failure stores a status that the exit call reads after a call that may change it (label T?);
 *   spawn        a spawn operation that returns a failure after its thread was created (the detach failing: child
 *                without success possible, read from the creation call's value) and one that returns failure only when
 *                the creation failed (not found);
 *   gate         a flag polled with a delay by another thread; the wait's function returns a failure constant on its
 *                budget exit, and its caller calls the use only under the success value (a caller-side use: skip).
 * The values are the fixture's; they are no experiment results. */
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

int usleep(unsigned int usec); /* POSIX.1-2001 (XSI); declared here: <unistd.h> leaves it out under -std=c99 */

#define NSLOTS 4
#define NAMELEN 16
#define RF_OK 0
#define RF_ERR_NOT_FOUND (-17)
#define RF_ERR_FULL (-14)
#define RF_ERR_IMPL (-30)
#define RF_ERR_APP (-40)
#define RF_ERR_TIMEOUT (-9)
#define RF_ERR_SPAWN (-50)

typedef struct {
  int used;
  int token;
  char name[NAMELEN];
} Obj;

static Obj objs[NSLOTS];
static int handles[8];
static int reports;

int obj_find(int *out, const char *name) {
  int i;
  for (i = 0; i < NSLOTS; i++) {
    if (objs[i].used == 1 && strcmp(objs[i].name, name) == 0) {
      *out = i;
      return RF_OK;
    }
  }
  return RF_ERR_NOT_FOUND;
}

static int obj_impl(int slot) {
  if (slot < 0) return RF_ERR_IMPL;
  objs[slot].token = slot + 1;
  return RF_OK;
}

/* gives its status back unchanged */
static int obj_finish(int status, int slot, int *out) {
  if (status == RF_OK) {
    objs[slot].used = 1;
    *out = slot;
  }
  return status;
}

int obj_create(int *out, const char *name) {
  int i;
  int rc = RF_ERR_FULL;
  int slot = -1;
  if (out == 0) return RF_ERR_APP;
  for (i = 0; i < NSLOTS; i++) {
    if (objs[i].used == 1 && strcmp(objs[i].name, name) == 0) return RF_ERR_APP;
  }
  for (i = 0; i < NSLOTS; i++) {
    if (objs[i].used == 0) {
      strncpy(objs[i].name, name, NAMELEN - 1);
      slot = i;
      rc = RF_OK;
      break;
    }
  }
  if (rc == RF_OK) {
    rc = obj_impl(slot);
    rc = obj_finish(rc, slot, out);
  }
  return rc;
}

static void report(int code) { reports = reports + code; }

/* ---- the lookups --------------------------------------------------------------------------------------------- */
int checker_init(void) {
  int st;
  st = obj_create(&handles[0], "own");
  if (st != RF_OK) return RF_ERR_APP;
  st = obj_find(&handles[1], "peer");
  if (st == RF_OK) {
    report(1);
  }
  return RF_OK;
}

int reporter_init(void) {
  int st;
  st = obj_find(&handles[2], "peer");
  if (st != RF_OK) {
    report(2);
    return RF_ERR_APP;
  }
  return RF_OK;
}

int looper_init(void) {
  int ret = RF_OK;
  int k;
  for (k = 0; k < 2; k++) {
    ret = obj_find(&handles[3 + k], "peer");
    if (ret != RF_OK) {
      report(3);
      break;
    }
  }
  return ret;
}

void retrier_init(void) {
  int v = RF_ERR_NOT_FOUND;
  int k;
  for (k = 0; k < 5; k++) {
    v = obj_find(&handles[5], "peer");
    if (v == RF_OK) break;
    usleep(1000);
  }
}

static unsigned int run_state = 1;

int run_loop(unsigned int *state) {
  if (*state != 1) return 0;
  *state = 1;
  return 1;
}

void *checker_main(void *arg) {
  (void)arg;
  (void)checker_init();
  return 0;
}

void *reporter_main(void *arg) {
  (void)arg;
  if (reporter_init() != RF_OK) abort();
  if (looper_init() != RF_OK) abort();
  retrier_init();
  return 0;
}

void *runner_main(void *arg) {
  int st;
  (void)arg;
  st = obj_find(&handles[6], "peer");
  if (st != RF_OK) run_state = 3;
  while (run_loop(&run_state)) {
    usleep(1000);
  }
  exit((int)run_state);
}

void *maker_main(void *arg) {
  (void)arg;
  if (obj_create(&handles[7], "peer") != RF_OK) report(4);
  return 0;
}

/* ---- the spawn operations --------------------------------------------------------------------------------- */
int task_spawn_detach(void *(*entry)(void *), void *arg) {
  pthread_t t;
  int rc;
  rc = pthread_create(&t, 0, entry, arg);
  if (rc != 0) return RF_ERR_SPAWN;
  rc = pthread_detach(t);
  if (rc != 0) return RF_ERR_SPAWN;
  return RF_OK;
}

int task_spawn_plain(void *(*entry)(void *), void *arg) {
  pthread_t t;
  int rc;
  rc = pthread_create(&t, 0, entry, arg);
  if (rc != 0) return RF_ERR_SPAWN;
  return RF_OK;
}

/* ---- a flag polled with a delay; the caller uses it under the wait's success ------------------------------- */
typedef struct {
  volatile sig_atomic_t ready;
  int level;
} Gate;

static Gate gate;

void *gate_main(void *arg) {
  (void)arg;
  gate.ready = 1;
  return 0;
}

int gate_wait(void) {
  int budget = 10;
  while (gate.ready == 0) {
    if (budget <= 0) return RF_ERR_TIMEOUT;
    usleep(1000);
    budget--;
  }
  return RF_OK;
}

int gate_use(int level) { return level + 1; }

void *waiter_main(void *arg) {
  int st;
  (void)arg;
  st = gate_wait();
  if (st == RF_OK) gate.level = gate_use(gate.ready);
  return 0;
}

int main(void) {
  pthread_t a, b, c, d, e;
  (void)pthread_create(&a, 0, checker_main, 0);
  (void)pthread_create(&b, 0, reporter_main, 0);
  (void)task_spawn_detach(maker_main, 0);
  (void)task_spawn_plain(runner_main, 0);
  (void)pthread_create(&c, 0, gate_main, 0);
  (void)pthread_create(&d, 0, waiter_main, 0);
  (void)c;
  (void)d;
  (void)e;
  return 0;
}

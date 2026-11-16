/* plankey.c -- plangen.c with the key of app one's lookup in a character array of a configuration record that the app
 * tests before the call (`kz-static plan`, plan rules revision 2, over the facts of extract --no-profile --targets-rule
 * R-T1,R-T2, with and without --object-access); otherwise as plangen.c:
 *
 *   two "apps", each a thread started through a spawn operation (spawn_task) that passes a creation argument to the
 *   thread-creation primitive; the start routine is the app's entry, which receives the argument;
 *   a registration table searched by name (strcmp in a loop over a global array) with a lookup API (a non-null test of
 *   the name before the search) and a create API (a non-null test and a bounded NUL search of the name);
 *   app one looks the name up in its init function; the lookup's failure reaches the app-exit call of the app through
 *   a store of the run status; the run status is the argument of the run-loop call;
 *   app two creates the name;
 *   two calls of the lookup on one line, and a lookup in a loop, in app one;
 *   the spawn operation stores its owner parameter after the spawn;
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define NREGS 4
#define NAMELEN 16
#define ERR_NOT_FOUND (-17)
#define ERR_BAD_POINTER (-2)
#define ERR_NAME_TOO_LONG (-13)
#define ERR_NO_FREE (-14)
#define RUN 1
#define RUN_ERROR 3

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

/* ---- the framework layer of the fixture: the run loop and the app exit -------------------------------- */
int run_loop(int *run_status) { return run_status != 0 && *run_status == RUN; }

void app_exit(int status) { exit(status); }

/* ---- the spawn operation ------------------------------------------------------------------------------ */
typedef struct {
  pthread_t thread;
  int owner;
} Task;

static Task tasks[4];

int spawn_task(void *(*entry)(void *), long id, int owner) {
  int st = pthread_create(&tasks[id].thread, 0, entry, (void *)id);
  if (st != 0) return st;
  tasks[id].owner = owner;
  return 0;
}

/* ---- app one: the dependent lookup -------------------------------------------------------------------- */
typedef struct {
  int run_status;
  int handle, h2, h3;
} AppData;

static AppData one;

#define CFGNAME 12
typedef struct {
  char name[CFGNAME];
} OneCfg;

static OneCfg one_cfg = {"lime"};

static int one_init(void) {
  if (one_cfg.name[0]) return reg_find(&one.handle, one_cfg.name);
  return 0;
}

void *one_main(void *arg) {
  int status, i;
  (void)arg;
  one.run_status = RUN;
  status = one_init();
  if (status != 0) {
    one.run_status = RUN_ERROR;
  }
  (void)reg_find(&one.h2, "lime"); (void)reg_find(&one.h3, "lime");
  for (i = 0; i < 3; i++) (void)reg_find(&one.h2, "lime");
  while (run_loop(&one.run_status)) {
    one.handle = 0;
  }
  app_exit(one.run_status);
  return 0;
}

/* ---- app two: the creator ----------------------------------------------------------------------------- */
static int two_handle;

void *two_main(void *arg) {
  int run_status = RUN;
  (void)arg;
  (void)reg_create(&two_handle, "lime");
  while (run_loop(&run_status)) {
    two_handle = two_handle + 0;
  }
  app_exit(run_status);
  return 0;
}


int main(void) {
  (void)spawn_task(one_main, 1, 10);
  (void)spawn_task(two_main, 2, 20);
  return 0;
}

/* planv5.c -- a fixture for plan rules revision 5 (kz-static plan --manifest 4 with namespace data of coverage rule
 * revision 2, docs/plan_manifest.md section 10) over the facts of the profile-free mode with the rule targets and the
 * extraction options (--object-access, --result-flow): read by test_static_plan only, with the hand-written data files of
 * data/planv5_fixture/.  Synthetic, for the regression of the component only; the values are the fixture's.
 *
 *   object layer  a common table of records holding a pointer to the name, the kind and the active id; a lookup with a
 *                 kind parameter (strcmp in a loop), a free-slot search that clears
 *                 the name pointer under a test of its own local (the free-record condition is not shown), a reset that
 *                 clears it under a test of the active id (shown), an allocation with a kind parameter, a finishing step
 *                 that stores the id on success and rolls the record back otherwise, a delete finishing step, the table's
 *                 fill;
 *   two kinds     a semaphore kind (lookup, create with a bounded NUL search of the name in a helper, the name store and
 *                 the finishing step, delete) and a queue kind (lookup with a bounded NUL search of its own, create, no
 *                 delete); the name of each is kept in an array field of its own table;
 *   a table       a configuration table searched by name with strncmp and filled by a copy (the table-copy generation);
 *   threads       a maker creates both kinds and a configuration entry, an app looks them up through an initialisation
 *                 function, and a command thread calls the same initialisation function (a second caller context). */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define NREC 4
#define NAMELEN 12
#define K_SEM 1
#define K_QUE 2
#define V5_OK 0
#define V5_ERR_PTR (-2)
#define V5_ERR_LONG (-13)
#define V5_ERR_FULL (-14)
#define V5_ERR_DUP (-15)
#define V5_ERR_NOT_FOUND (-17)

typedef struct {
  const char *nm;
  int kind;
  int act;
} Com;
typedef struct {
  char name[NAMELEN];
  int count;
} Sem;
typedef struct {
  char name[NAMELEN];
  int depth;
} Que;
typedef struct {
  char key[NAMELEN];
  int val;
} Cfg;

static Com coms[NREC];
static Sem sems[NREC];
static Que ques[NREC];
static Cfg cfgs[NREC];
static int seen;

/* ---- the object layer --------------------------------------------------------------------------------------------- */
int obj_find(int kind, const char *name, int *out) {
  int i;
  for (i = 0; i < NREC; i++) {
    if (coms[i].act != 0 && coms[i].kind == kind && coms[i].nm != 0 && strcmp(coms[i].nm, name) == 0) {
      *out = coms[i].act;
      return V5_OK;
    }
  }
  return V5_ERR_NOT_FOUND;
}

static int obj_free(int *slot) {
  int i, r = V5_ERR_FULL;
  for (i = 0; i < NREC; i++) {
    if (coms[i].act == 0) {
      r = i;
      break;
    }
  }
  if (r >= 0) coms[r].nm = 0;
  if (r >= 0) *slot = r;
  return r < 0 ? r : V5_OK;
}

static void obj_reset(int i) {
  if (coms[i].act == 0) coms[i].nm = 0;
}

int obj_alloc(int kind, const char *name, int *slot) {
  int found;
  if (obj_find(kind, name, &found) == V5_OK) return V5_ERR_DUP;
  if (obj_free(slot) != V5_OK) return V5_ERR_FULL;
  coms[*slot].kind = kind;
  return V5_OK;
}

int obj_finish(int status, int slot, int *out) {
  if (status == V5_OK) {
    coms[slot].act = slot + 1;
    *out = coms[slot].act;
  } else {
    coms[slot].act = 0;
  }
  return status;
}

int obj_finish_delete(int status, int slot) {
  if (status == V5_OK) coms[slot].act = 0;
  return status;
}

void obj_init(void) { memset(coms, 0, sizeof(coms)); }

/* ---- the semaphore kind ------------------------------------------------------------------------------------------- */
static int name_ok(const char *n) { return memchr(n, 0, NAMELEN) != 0; }

int sem_get(int *id, const char *name) {
  if (id == 0 || name == 0) return V5_ERR_PTR;
  return obj_find(K_SEM, name, id);
}

int sem_create(int *id, const char *name) {
  int slot = 0, rc;
  if (id == 0 || name == 0) return V5_ERR_PTR;
  if (name_ok(name) == 0) return V5_ERR_LONG;
  rc = obj_alloc(K_SEM, name, &slot);
  if (rc == V5_OK) {
    strncpy(sems[slot].name, name, NAMELEN - 1);
    coms[slot].nm = sems[slot].name;
    rc = obj_finish(rc, slot, id);
  }
  return rc;
}

int sem_delete(int id) {
  int rc = V5_ERR_NOT_FOUND;
  if (id > 0 && id <= NREC && coms[id - 1].act == id) rc = V5_OK;
  return obj_finish_delete(rc, id - 1);
}

/* ---- the queue kind (no delete) ----------------------------------------------------------------------------------- */
int que_get(int *id, const char *name) {
  if (id == 0 || name == 0) return V5_ERR_PTR;
  if (memchr(name, 0, NAMELEN) == 0) return V5_ERR_LONG;
  return obj_find(K_QUE, name, id);
}

int que_create(int *id, const char *name) {
  int slot = 0, rc;
  if (id == 0 || name == 0) return V5_ERR_PTR;
  rc = obj_alloc(K_QUE, name, &slot);
  if (rc == V5_OK) {
    strncpy(ques[slot].name, name, NAMELEN - 1);
    coms[slot].nm = ques[slot].name;
    rc = obj_finish(rc, slot, id);
  }
  return rc;
}

/* ---- the configuration table (table-copy generation) -------------------------------------------------------------- */
int cfg_find(int *val, const char *key) {
  int i;
  if (val == 0) return V5_ERR_PTR;
  for (i = 0; i < NREC; i++) {
    if (strncmp(cfgs[i].key, key, NAMELEN) == 0) {
      *val = cfgs[i].val;
      return V5_OK;
    }
  }
  return V5_ERR_NOT_FOUND;
}

int cfg_add(const char *key, int val) {
  int i, found;
  if (cfg_find(&found, key) == V5_OK) return V5_ERR_DUP;
  for (i = 0; i < NREC; i++) {
    if (cfgs[i].key[0] == 0) {
      strncpy(cfgs[i].key, key, NAMELEN - 1);
      cfgs[i].val = val;
      return V5_OK;
    }
  }
  return V5_ERR_FULL;
}

/* ---- the threads -------------------------------------------------------------------------------------------------- */
static int app_sem, app_que, app_cfg, mk_sem, mk_que;

static int app_init(void) { return sem_get(&app_sem, "s1"); }

static void *maker_main(void *arg) {
  (void)arg;
  obj_init();
  if (sem_create(&mk_sem, "s1") != V5_OK) seen = seen + 1;
  if (que_create(&mk_que, "q1") != V5_OK) seen = seen + 2;
  if (cfg_add("c1", 7) != V5_OK) seen = seen + 4;
  obj_reset(3);
  if (seen > 9) (void)sem_delete(mk_sem);
  return 0;
}

static void *app_main(void *arg) {
  (void)arg;
  if (app_init() != V5_OK) exit(1);
  if (que_get(&app_que, "q1") != V5_OK) seen = seen + 8;
  if (cfg_find(&app_cfg, "c1") != V5_OK) seen = seen + 16;
  return 0;
}

static void *cmd_main(void *arg) {
  (void)arg;
  if (app_init() != V5_OK) seen = seen + 32;
  return 0;
}

int main(void) {
  pthread_t a, b, c;
  (void)pthread_create(&a, 0, maker_main, 0);
  (void)pthread_create(&b, 0, app_main, 0);
  (void)pthread_create(&c, 0, cmd_main, 0);
  return 0;
}

/* closure.c -- a fixture for the lines of relations rules revision 14 (static_facts.md 5x and its addenda) over the facts
 * of the profile-free mode with the rule targets, result flow and object access (extract --no-profile --targets-rule
 * R-T1,R-T2 --result-flow --object-access).  Synthetic, for the regression of the component only.
 *
 *   name table   a table of records searched by name (strcmp in a loop over a global array) with a lookup API that
 *                stores the found record's id through its out pointer;
 *   creators     five create APIs of the same table, each the creator callee of a use in another thread (each looks the
 *                name up first and returns a failure constant when it is there, so its name parameter reaches the search):
 *                ent_create      the success constant is returned only after the name store and the store of the new id
 *                                through the out pointer (an expression read at the store), both under the conditions
 *                                of that return: the success path line holds;
 *                ent_create_tmp  the same, but the id goes through a local whose definitions these facts do not all give:
 *                                missing (the stored value is not tied to the success path);
 *                ent_create_any  also returns the success constant early, before any store, for an empty name: missing
 *                                (a second origin can carry the value);
 *                ent_create_cnt  the name store is under a condition the success return does not imply (the line order
 *                                puts the store before the return, but the store is on another branch): missing;
 *                ent_create_fin  returns a local that holds a reservation's result and then a finishing step's, which
 *                                gives its status back unchanged and stores the id through the out pointer; the first
 *                                definition cannot carry the success value to the return (its own branch on it), and
 *                                the out-id store is in the finishing step under a condition on its own parameter;
 *   holders      the looker clears a table of channels and looks a name up into a channel's field (a field holder with a
 *                clearing writer), into an element of a global array (no record field) and into a local; a user thread
 *                reads the channel field once after a test of its value and once without one;
 *   alternatives (relations rules revision 14, the alternatives line and the stored-field rule, 5x seventh addendum)
 *                a runner looks a name up through an initialisation function that returns the lookup's result; on the
 *                failure it stores a status, jumps over a second store to a loop on a call that takes the status's address
 *                and returns 0 for that status, and exits with the status: by line the second store is between the store
 *                and the call, by path it is not; a second runner stores a status on the failure of a lookup whose failure
 *                is turned into another constant, and a store that the source puts after the exit is on a path from the
 *                status store to the call; a command thread calls the initialisation function too (a second context);
 *                a waiter looks a name up again until it is found (a retry);
 *   stored names a common table of name pointers into kind tables (see the comment before names_main);
 *   threads      the main thread starts a looker (the lookups), a maker (the creations), a user, the two runners, the
 *                command thread, the waiter, a thread that makes, renames, reads, resets and deletes stored names, and a
 *                thread that looks one up.
 * The values are the fixture's; they are no experiment results. */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define NSLOTS 4
#define NAMELEN 16
#define CL_OK 0
#define CL_ERR_NOT_FOUND (-17)
#define CL_ERR_PTR (-2)
#define CL_ERR_FULL (-14)
#define CL_ERR_DUP (-15)

typedef struct {
  int used;
  int id;
  char name[NAMELEN];
} Ent;

typedef struct {
  int sem;
  int count;
} Chan;

static Ent ents[NSLOTS];
static Chan chans[2];
static int ids[8];
static int seen;

int ent_find(int *out, const char *name) {
  int i;
  if (out == 0) return CL_ERR_PTR;
  for (i = 0; i < NSLOTS; i++) {
    if (ents[i].used == 1 && strcmp(ents[i].name, name) == 0) {
      *out = ents[i].id;
      return CL_OK;
    }
  }
  return CL_ERR_NOT_FOUND;
}

static int ent_slot(void) {
  int i;
  for (i = 0; i < NSLOTS; i++) {
    if (ents[i].used == 0) return i;
  }
  return -1;
}

int ent_create(int *out, const char *name, int slot) {
  int found;
  if (out == 0) return CL_ERR_PTR;
  if (ent_find(&found, name) == CL_OK) return CL_ERR_DUP;
  if (slot < 0) return CL_ERR_FULL;
  strncpy(ents[slot].name, name, NAMELEN - 1);
  ents[slot].used = 1;
  ents[slot].id = slot + 100;
  *out = ents[slot].id;
  return CL_OK;
}

int ent_create_tmp(int *out, const char *name, int slot) {
  int id;
  int found;
  if (out == 0) return CL_ERR_PTR;
  if (ent_find(&found, name) == CL_OK) return CL_ERR_DUP;
  if (slot < 0) return CL_ERR_FULL;
  strncpy(ents[slot].name, name, NAMELEN - 1);
  ents[slot].used = 1;
  id = slot + 200;
  ents[slot].id = id;
  *out = id;
  return CL_OK;
}

int ent_create_any(int *out, const char *name, int slot) {
  int found;
  if (out == 0) return CL_ERR_PTR;
  if (ent_find(&found, name) == CL_OK) return CL_ERR_DUP;
  if (name[0] == 0) return CL_OK;
  if (slot < 0) return CL_ERR_FULL;
  strncpy(ents[slot].name, name, NAMELEN - 1);
  ents[slot].used = 1;
  ents[slot].id = slot + 300;
  *out = ents[slot].id;
  return CL_OK;
}

int ent_create_cnt(int *out, const char *name, int slot, int copy) {
  int found;
  if (out == 0) return CL_ERR_PTR;
  if (ent_find(&found, name) == CL_OK) return CL_ERR_DUP;
  if (slot < 0) return CL_ERR_FULL;
  if (copy != 0) {
    strncpy(ents[slot].name, name, NAMELEN - 1);
  }
  ents[slot].used = 1;
  ents[slot].id = slot + 400;
  *out = ents[slot].id;
  return CL_OK;
}

/* gives its status back unchanged, and the new id through the out pointer on success */
static int ent_finish(int status, int slot, int *out) {
  if (status == CL_OK && out != 0) *out = ents[slot].id;
  return status;
}

static int ent_reserve(int slot) {
  if (slot < 0) return CL_ERR_FULL;
  ents[slot].used = 1;
  return CL_OK;
}

int ent_create_fin(int *out, const char *name, int slot) {
  int rc;
  int found;
  if (out == 0) return CL_ERR_PTR;
  if (ent_find(&found, name) == CL_OK) return CL_ERR_DUP;
  rc = ent_reserve(slot);
  if (rc == CL_OK) {
    strncpy(ents[slot].name, name, NAMELEN - 1);
    ents[slot].id = slot + 500;
    rc = ent_finish(rc, slot, out);
  }
  return rc;
}

static int chan_use(int id) { return id + 1; }

static void *looker_main(void *arg) {
  int mine;
  (void)arg;
  memset(chans, 0, sizeof(chans));
  if (ent_find(&chans[0].sem, "alpha") != CL_OK) seen = seen + 1;
  if (ent_find(&ids[1], "beta") != CL_OK) seen = seen + 2;
  if (ent_find(&mine, "alpha") == CL_OK) seen = seen + mine;
  return 0;
}

static void *user_main(void *arg) {
  (void)arg;
  if (chans[0].sem != 0) seen = chan_use(chans[0].sem);
  seen = seen + chan_use(chans[1].sem);
  return 0;
}

static void *maker_main(void *arg) {
  (void)arg;
  if (ent_create(&ids[2], "alpha", ent_slot()) != CL_OK) seen = seen + 4;
  if (ent_create_tmp(&ids[3], "beta", ent_slot()) != CL_OK) seen = seen + 8;
  if (ent_create_any(&ids[4], "alpha", ent_slot()) != CL_OK) seen = seen + 16;
  if (ent_create_cnt(&ids[5], "beta", ent_slot(), seen) != CL_OK) seen = seen + 32;
  if (ent_create_fin(&ids[6], "alpha", ent_slot()) != CL_OK) seen = seen + 64;
  return 0;
}

typedef struct {
  unsigned int run;
  unsigned int mode;
} RunState;

static RunState runs;
static int rid[4];

int run_loop(unsigned int *state) {
  if (state != 0 && *state != 1) return 0;
  return 1;
}

/* the same table's lookup without the null check: its no-match value is the search's */
int ent_get(int *out, const char *name) {
  int i;
  for (i = 0; i < NSLOTS; i++) {
    if (ents[i].used == 1 && strcmp(ents[i].name, name) == 0) {
      *out = ents[i].id;
      return CL_OK;
    }
  }
  return CL_ERR_NOT_FOUND;
}

static int run_init(void) { return ent_get(&rid[0], "gamma"); }

static int run_init_c(void) {
  if (ent_get(&rid[1], "delta") != CL_OK) return CL_ERR_FULL;
  return CL_OK;
}

static void *runner_main(void *arg) {
  int st;
  (void)arg;
  st = run_init();
  if (st != CL_OK) {
    runs.run = 3;
    goto loop;
  }
  runs.run = 1;
loop:
  while (run_loop(&runs.run)) {
    seen = seen + 1;
  }
  exit((int)runs.run);
}

static void *runner2_main(void *arg) {
  (void)arg;
  if (run_init_c() != CL_OK) runs.mode = 3;
  if (seen > 7) goto fix;
again:
  while (run_loop(&runs.mode)) {
    seen = seen + 2;
  }
  exit((int)runs.mode);
fix:
  runs.mode = 1;
  goto again;
}

static void *cmd_main(void *arg) {
  (void)arg;
  if (run_init() != CL_OK) seen = seen + 5;
  return 0;
}

static void *waiter_main(void *arg) {
  (void)arg;
  while (ent_get(&rid[2], "eps") != CL_OK) {
    seen = seen + 3;
  }
  return 0;
}

/* stored names (relations rules revision 14, the stored-name buffer and lifetime-end lines, 5x eighth addendum): a common
 * table of records holding a pointer to the name and an active id; two kinds keep the name in an array field of their own
 * table and store the array's address into the common record (the name store), and one kind's name is rewritten by a rename
 * (another writer); the readers of the name pointer test it, pass it to a function that only reads it, to one that writes
 * through it and to the comparison of the search, and copy it to a global; the free-slot search clears the name pointer
 * under a test of its own local (the free-record condition is not shown), a reset clears it under a test of the active id
 * (shown); a finishing step stores the active id read through its parameter and is called by a create (a rollback), by a
 * delete and with a null pointer; a drop stores the empty value; the initialisation fills the common table. */
#define NREC 4
typedef struct {
  const char *nm;
  int act;
} Com;
typedef struct {
  char label[NAMELEN];
  int v;
} KindA;
typedef struct {
  char tag[NAMELEN];
  int w;
} KindB;
static Com coms[NREC];
static KindA kas[NREC];
static KindB kbs[NREC];
static const char *last_nm;

static void com_init(void) { memset(coms, 0, sizeof(coms)); }

int com_find(int *out, const char *name) {
  int i;
  for (i = 0; i < NREC; i++) {
    if (coms[i].act != 0 && coms[i].nm != 0 && strcmp(coms[i].nm, name) == 0) {
      *out = coms[i].act;
      return CL_OK;
    }
  }
  return CL_ERR_NOT_FOUND;
}

static int com_free(void) {
  int i, r = CL_ERR_FULL;
  for (i = 0; i < NREC; i++) {
    if (coms[i].act == 0) {
      r = i;
      break;
    }
  }
  if (r >= 0) coms[r].nm = 0;
  return r;
}

static void com_reset(int i) {
  if (coms[i].act == 0) coms[i].nm = 0;
}

static void com_finish(int i, const int *fin) {
  if (fin != 0) coms[i].act = *fin;
}

static void com_drop(int i) { coms[i].act = 0; }

static int nm_len(const char *p) {
  int n = 0;
  while (p[n] != 0) n++;
  return n;
}

static void nm_up(char *p) {
  if (p[0] >= 'a') p[0] = (char)(p[0] - 32);
}

int ka_create(int *out, const char *name) {
  int dummy, i, fin;
  if (com_find(&dummy, name) == CL_OK) return CL_ERR_DUP;
  i = com_free();
  if (i < 0) return CL_ERR_FULL;
  memset(&kas[i], 0, sizeof(kas[i]));
  strncpy(kas[i].label, name, NAMELEN - 1);
  coms[i].nm = kas[i].label;
  fin = i + 1;
  if (nm_len(coms[i].nm) == 0) fin = 0;
  com_finish(i, &fin);
  *out = fin;
  return CL_OK;
}

int kb_create(int *out, const char *name) {
  int dummy, i, fin;
  if (com_find(&dummy, name) == CL_OK) return CL_ERR_DUP;
  i = com_free();
  if (i < 0) return CL_ERR_FULL;
  memset(&kbs[i], 0, sizeof(kbs[i]));
  strncpy(kbs[i].tag, name, NAMELEN - 1);
  coms[i].nm = kbs[i].tag;
  fin = i + 1;
  com_finish(i, &fin);
  *out = fin;
  return CL_OK;
}

void ka_rename(int i, const char *name) { strncpy(kas[i].label, name, NAMELEN - 1); }

int com_delete(int i) {
  int fin = 0;
  com_finish(i, &fin);
  com_finish(i, 0);
  return CL_OK;
}

static void *names_main(void *arg) {
  int id, i;
  (void)arg;
  com_init();
  (void)ka_create(&id, "one");
  (void)kb_create(&id, "two");
  ka_rename(0, "three");
  for (i = 0; i < NREC; i++) {
    if (coms[i].nm != 0) seen = seen + nm_len(coms[i].nm);
  }
  nm_up((char *)coms[1].nm);
  last_nm = coms[2].nm;
  com_reset(3);
  com_drop(2);
  (void)com_delete(0);
  return 0;
}

static void *names_user_main(void *arg) {
  int id;
  (void)arg;
  if (com_find(&id, "one") == CL_OK) seen = seen + id;
  return 0;
}

int main(void) {
  pthread_t a, b, c, d, e, f, g, h, k;
  (void)pthread_create(&a, 0, looker_main, 0);
  (void)pthread_create(&b, 0, maker_main, 0);
  (void)pthread_create(&c, 0, user_main, 0);
  (void)pthread_create(&d, 0, runner_main, 0);
  (void)pthread_create(&e, 0, runner2_main, 0);
  (void)pthread_create(&f, 0, cmd_main, 0);
  (void)pthread_create(&g, 0, waiter_main, 0);
  (void)pthread_create(&h, 0, names_main, 0);
  (void)pthread_create(&k, 0, names_user_main, 0);
  return 0;
}

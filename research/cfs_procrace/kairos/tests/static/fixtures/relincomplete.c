/* relincomplete.c -- a fixture for the incompleteness line of `kz-static relations` (relations rules revision 6) over
 * the facts of the profile-free mode with the rule targets (extract --no-profile --targets-rule R-T1,R-T2), read at the
 * default image budget (relincomplete_np) and at --image-budget 3 (relincomplete_cut_np), where the two static tables
 * below are cut:
 *
 *   A  a registration table searched by name (strcmp in a loop over a global array), a lookup API and a create API of
 *      it; three lookups:
 *        - a key from a static table of four names reached through a pointer field the facts resolve (its image is read
 *          whole at the default budget, cut at --image-budget 3);
 *        - a key read through a pointer field whose target the facts do not resolve (the pointer comes from a function
 *          without a body);
 *        - a string literal that a thread of another actor creates;
 *   C  a spawn whose entries are read from a static table of three thread entries and a hook (cut at --image-budget 3,
 *      so the entries found there are not all), and the creator's write after the spawn that the threads read.
 */
#include <pthread.h>
#include <string.h>

#define NREGS 4
#define NAMELEN 16
#define ERR_NOT_FOUND (-17)
#define ERR_ILLEGAL (-5)

typedef struct {
  int used;
  char name[NAMELEN];
} Reg;

static Reg regs[NREGS];

/* ---- path A: the table and its API ------------------------------------------------------------------ */
int reg_find(const char *name, int *out) {
  int i;
  for (i = 0; i < NREGS; i++) {
    if (regs[i].used == 1 && strcmp(regs[i].name, name) == 0) {
      *out = i;
      return 0;
    }
  }
  return ERR_NOT_FOUND;
}

int reg_create(const char *name, int *out) {
  int i;
  for (i = 0; i < NREGS; i++) {
    if (regs[i].used == 1 && strcmp(regs[i].name, name) == 0) {
      return ERR_ILLEGAL;
    }
  }
  for (i = 0; i < NREGS; i++) {
    if (regs[i].used == 0) {
      strncpy(regs[i].name, name, NAMELEN - 1);
      regs[i].used = 1;
      *out = i;
      return 0;
    }
  }
  return ERR_NOT_FOUND;
}

typedef struct {
  const char *name;
} NameDef;

/* four leaves: the whole image at the default budget, cut at --image-budget 3 */
static const NameDef name_defs[4] = {{"red"}, {"green"}, {"blue"}, {"grey"}};

/* a pointer the facts do not follow: it comes from a function without a body */
extern const NameDef *name_source(void);

typedef struct {
  const NameDef *known;
  const NameDef *opaque;
  int handle;
} Seeker;

static Seeker seeker;

void seek_known(void) {
  int i;
  seeker.known = name_defs;
  for (i = 0; i < 4; i++) (void)reg_find(seeker.known[i].name, &seeker.handle);
}

void seek_opaque(void) {
  seeker.opaque = name_source();
  (void)reg_find(seeker.opaque[0].name, &seeker.handle);
}

/* ---- path C: thread entries from a static table, and the creator's write after the spawn ------------- */
typedef struct {
  int started;
} State;

static State state;

int state_check(void) { return state.started == 1 ? 0 : ERR_ILLEGAL; }

void *red_main(void *arg) {
  int h = 0;
  (void)arg;
  (void)state_check();
  (void)reg_find("lime", &h);
  return 0;
}

void *green_main(void *arg) {
  int h = 0;
  (void)arg;
  (void)state_check();
  (void)reg_create("lime", &h);
  return 0;
}

void *blue_main(void *arg) {
  (void)arg;
  (void)state_check();
  return 0;
}

static void setup_hook(void) { state.started = 0; }

/* the entry is a member of a union whose other member holds a function of another type in one element: the flow graph
 * of the facts does not resolve the function value read through it, so the entries come from the static image of the
 * table (and rest on the guard of the spawn, which the rules do not read) */
typedef union {
  void (*call)(void);
  void *(*main)(void *);
} TaskEntry;

typedef struct {
  TaskEntry u;
} TaskDef;

/* four leaves: the whole image at the default budget, cut at --image-budget 3 after the first two thread entries */
static TaskDef task_defs[4] = {{.u.call = setup_hook}, {.u.main = red_main}, {.u.main = green_main}, {.u.main = blue_main}};

void start_tasks(int n) {
  int i;
  pthread_t t;
  for (i = 1; i < n; i++) pthread_create(&t, 0, task_defs[i].u.main, 0);
  state.started = 1;
}

int main(void) {
  seek_known();
  seek_opaque();
  start_tasks(4);
  return 0;
}

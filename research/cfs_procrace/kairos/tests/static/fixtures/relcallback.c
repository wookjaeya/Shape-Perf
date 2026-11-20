/* relcallback.c -- a fixture for relations rules revision 7 (path A) over the facts of the profile-free mode with the rule
 * targets (extract --no-profile --targets-rule R-T1,R-T2), golden relcallback_np:
 *
 *   - one generic search loop over a table of records, with the comparison passed as a callback parameter; the table is
 *     selected by a kind the search reads as a subscript (stored into a token first), and a lock mode is stored beside it
 *     that the search does not read;
 *   - two comparison functions over two fields of the record: by name (strcmp) and by path prefix (strncmp);
 *   - a lookup API by name (passes the name comparison), a lookup API by path (passes the path comparison), a create API
 *     that writes the name, and a search API that takes the comparison from its caller;
 *   - users in two threads and the process: a lookup by name with a key read from a static table through a pointer field,
 *     a lookup by path, a creation of the same name, and a search whose comparison comes from a function without a body.
 *
 * The rules this shows: a use belongs to the namespace of a comparison only when the comparison its chain of calls passes to the search is that namespace's (the lookup by path is no use of the name namespace, the lookup and the creation by
 * name are none of the path namespace); a comparison that comes from a function without a body leaves the membership
 * unresolved; the partition takes the kind (a table selector) and names the lock mode as no table selector; the rows name
 * the out-pointer arguments, the pointer field of the key, the writer sets with their premises (zero writers is no
 * verdict) and the constants the creator's callee may return (revision 9: its success value comes through a definition redefined
 * later and a finishing call returns its parameter, so the set is not established); a lookup with no creator says why.
 */
#include <pthread.h>
#include <string.h>

#define NKINDS 2
#define NRECS 4
#define KIND_PATH 0
#define KIND_SEM 1
#define LOCK_SHARED 2
#define LOCK_EXCL 3
#define RC_OK 0
#define RC_NOT_FOUND (-17)
#define RC_NAME_TAKEN (-15)
#define RC_NO_FREE (-14)
#define RC_BAD_POINTER (-2)

typedef struct {
  int active;
  char name[16];
  char path[16];
} Rec;

static Rec table[NKINDS][NRECS];

typedef struct {
  int lock_mode;
  int kind;
  int idx;
} Token;

typedef int (*Match)(const void *ref, const Rec *r);

static void token_init(Token *t, int lock_mode, int kind) {
  t->lock_mode = lock_mode;
  t->kind = kind;
  t->idx = -1;
}

static int in_use(int active) { return active != 0; }

int find_next(Match m, const void *ref, Token *t) {
  int rc = RC_NOT_FOUND;
  const Rec *rec;
  while (1) {
    ++t->idx;
    if (t->idx >= NRECS) break;
    rec = &table[t->kind][t->idx];
    if (in_use(rec->active) && m(ref, rec)) {
      rc = RC_OK;
      break;
    }
  }
  return rc;
}

int name_match(const void *ref, const Rec *r) { return strcmp((const char *)ref, r->name) == 0; }

int path_match(const void *ref, const Rec *r) { return strncmp((const char *)ref, r->path, 8) == 0; }

int get_by_search(int lock_mode, int kind, Match m, const void *ref, Token *t) {
  token_init(t, lock_mode, kind);
  return find_next(m, ref, t);
}

int find_by_name(int lock_mode, int kind, const char *name, int *id) {
  Token t;
  int rc = get_by_search(lock_mode, kind, name_match, name, &t);
  if (rc == RC_OK) *id = t.idx;
  return rc;
}

int sem_find(const char *name, int *id) { return find_by_name(LOCK_SHARED, KIND_SEM, name, id); }

int path_find(const char *path, int *id) {
  Token t;
  int rc = get_by_search(LOCK_SHARED, KIND_PATH, path_match, path, &t);
  if (rc == RC_OK) *id = t.idx;
  return rc;
}

static int alloc_check(int kind, const char *name, Token *t) {
  int i;
  token_init(t, LOCK_EXCL, kind);
  if (find_next(name_match, name, t) == RC_OK) return RC_NAME_TAKEN;
  for (i = 0; i < NRECS; i++) {
    if (table[kind][i].active == 0) {
      t->idx = i;
      return RC_OK;
    }
  }
  return RC_NO_FREE;
}
static int rec_finish(int rc, Token *t, int *id) { if (rc == RC_OK) *id = t->idx; return rc; }
int rec_create(int kind, const char *name, int *id) {
  Token t;
  int rc;
  if (id == 0) return RC_BAD_POINTER;
  rc = alloc_check(kind, name, &t);
  if (rc == RC_OK) {
    strncpy(table[kind][t.idx].name, name, 15);
    table[kind][t.idx].active = 1;
    rc = rec_finish(rc, &t, id); }
  return rc;
}

int sem_create(const char *name, int *id) { return rec_create(KIND_SEM, name, id); }

int search_with(Match m, const char *ref) {
  Token t;
  return get_by_search(LOCK_SHARED, KIND_SEM, m, ref, &t);
}

/* a comparison the facts do not follow: it comes from a function without a body */
extern Match pick_match(void);

typedef struct {
  const char *name;
} Def;

static const Def defs[2] = {{"alpha"}, {"beta"}};

typedef struct {
  const Def *defs;
  int ids[2];
  int path_id;
} Holder;

static Holder holder;

void *user_main(void *arg) {
  (void)arg;
  holder.defs = defs;
  (void)sem_find(holder.defs[0].name, &holder.ids[0]);
  (void)path_find("/mnt/a", &holder.path_id);
  return 0;
}

void *maker_main(void *arg) {
  (void)arg;
  (void)sem_create("alpha", &holder.ids[1]);
  return 0;
}

/* a delete by name (2026-11-18): it looks the record up by its name and clears the key with a constant store: a
 * key-clearing call, no creator (relations rules revision 12) */
int sem_delete(const char *name) {
  Token t;
  int rc = get_by_search(LOCK_EXCL, KIND_SEM, name_match, name, &t);
  if (rc == RC_OK) {
    memset(table[KIND_SEM][t.idx].name, 0, sizeof table[KIND_SEM][t.idx].name);
    table[KIND_SEM][t.idx].active = 0;
  }
  return rc;
}

void *cleaner_main(void *arg) {
  (void)arg;
  (void)sem_delete("alpha");
  return 0;
}

int main(void) {
  pthread_t a, b, c;
  pthread_create(&a, 0, user_main, 0);
  pthread_create(&b, 0, maker_main, 0);
  pthread_create(&c, 0, cleaner_main, 0);
  (void)search_with(pick_match(), "gamma");
  return 0;
}

/* flowfacts.c -- the extended facts F1-F3 of kz-static: the field and global reads with their use
 * (READS), the return values resolved through the definitions of the returned local (RETURNS,
 * RET_CONSTS) and the exits of every loop, whatever its kind (LOOP_EXITS).  Read twice: in the
 * profile-free mode (extract --no-profile) and with the fixture profile and --extended-facts.  The
 * library function is declared here in the shape the C standard gives it, so that the fixture needs
 * no header. */
int strcmp(const char *a, const char *b);

#define FX_OK 0
#define FX_ERR_ID (-3)
#define FX_ERR_NOT_FOUND (-17)
#define FX_TABLE_SIZE 8

typedef struct {
  int used;
  int owner;
  char name[16];
} FxRec;

typedef struct {
  FxRec table[FX_TABLE_SIZE];
  int count;
  int (*hook)(int);
  FxRec *cursor;
} FxStore;

FxStore fx_store;
int fx_level;
int fx_levels[4];

int fx_lookup_index(int id, unsigned *idx);

/* An indexed table: the index from a call, a validity branch on the record, an out-store of another
 * field, and a returned local with two definitions (the initial error code, the success code). */
int fx_owner_of(int id, int *out) {
  int result = FX_ERR_ID;
  unsigned idx;
  if (fx_lookup_index(id, &idx) == FX_OK && fx_store.table[idx].used == 1) {
    *out = fx_store.table[idx].owner;
    result = FX_OK;
  }
  return result;
}

/* A search loop without a delay: the key compared through a call, a break, a constant return. */
int fx_find_by_name(const char *key) {
  int i;
  for (i = 0; i < FX_TABLE_SIZE; ++i) {
    if (fx_store.table[i].used != 0 && strcmp(fx_store.table[i].name, key) == 0) break;
  }
  if (i == FX_TABLE_SIZE) return FX_ERR_NOT_FOUND;
  return i;
}

/* A read through a pointer parameter, returned. */
int fx_is_used(const FxRec *r) { return r->used; }

/* Global reads: a branch, a subscript, elements of a global array. */
int fx_current_level(void) {
  if (fx_level > 3) return fx_levels[3];
  return fx_levels[fx_level];
}

/* A function pointer read as a branch and as the callee; a pointer field dereferenced. */
int fx_run_hook(int v) {
  if (fx_store.hook == 0) return FX_ERR_ID;
  return fx_store.hook(v);
}
int fx_cursor_owner(void) { return fx_store.cursor->owner; }

/* Locals with a single definition, unfolded into the return value. */
int fx_count_plus(int k) {
  int c = fx_store.count;
  int d = c + k;
  return d;
}

/* Every return value a constant: the conditions of each, with the later definitions excluded. */
int fx_classify(int id) {
  int status = FX_OK;
  if (id < 0) {
    status = FX_ERR_ID;
  } else if (id >= FX_TABLE_SIZE) {
    status = FX_ERR_NOT_FOUND;
  }
  return status;
}

/* A return inside a do-while loop; a loop without any exit. */
int fx_first_free(void) {
  int i = 0;
  do {
    if (fx_store.table[i].used == 0) return i;
    ++i;
  } while (i < FX_TABLE_SIZE);
  return FX_ERR_NOT_FOUND;
}
void fx_spin(void) {
  while (1) {
    fx_level = fx_level + 1;
  }
}

/* A loop left by a goto; the returned local after the label. */
int fx_scan_until(int stop) {
  int n = 0;
  for (;;) {
    if (fx_levels[n & 3] == stop) goto done;
    n++;
  }
done:
  return n;
}

/* A whole record read; a local whose address is taken is not resolved. */
FxRec fx_snapshot(int i) {
  FxRec r = fx_store.table[i];
  return r;
}
unsigned fx_index_of(int id) {
  unsigned idx = 0;
  (void)fx_lookup_index(id, &idx);
  return idx;
}

/* The subscript comes from a call through a local with a single definition: the index is unfolded. */
int fx_self_index(void);
int fx_my_owner(int *out) {
  int result = FX_ERR_ID;
  int slot;
  slot = fx_self_index();
  if (fx_store.table[slot].used == 1) {
    *out = fx_store.table[slot].owner;
    result = FX_OK;
  }
  return result;
}

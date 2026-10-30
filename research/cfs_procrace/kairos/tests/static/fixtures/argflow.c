/* argflow.c -- the extended facts F4 and F6 of kz-static: where every call argument comes from
 * (ARGFLOW), where each parameter's value goes, composed through the calls (PARAM_FLOW), the reads
 * through a pointer field resolved to the global whose address an earlier store put there
 * (PTR_FIELDS), and the static values of the globals those facts reach (GLOBAL_IMAGES).  Read twice:
 * in the profile-free mode (extract --no-profile) and with the fixture profile and --extended-facts.
 * The library functions are declared here in the shape the C standard gives them, so that the fixture
 * needs no header. */
int strcmp(const char *a, const char *b);
void *memset(void *s, int c, unsigned long n);

/* names with a default the build may override */
#ifndef FX_SLOT_0_NAME
#define FX_SLOT_0_NAME "slot_zero"
#endif
#define FX_SLOT_1_NAME "slot_one"
#define FX_SLOTS 2
#define FX_NAMES 4

typedef struct {
  char name[16];
  int limit;
  int handle;
} FxSlot;

typedef struct {
  FxSlot *slots;  /* set to the address of a table at run time, or by a static initializer */
  FxSlot *cursor; /* its address escapes to code without a body */
  int ready;
} FxGlobal;

typedef struct {
  const char *name;
  int active;
} FxRec;

typedef int (*FxMatch)(const void *key, const FxRec *rec);

FxSlot fx_slots[FX_SLOTS] = {{FX_SLOT_0_NAME, 3, 0}, {FX_SLOT_1_NAME, 5, 0}};
FxSlot fx_spares[1] = {{"spare", 1, 0}};
FxGlobal fx_global;
FxGlobal fx_other = {fx_spares, 0, 1};
FxRec fx_table[FX_NAMES];
int fx_count = 7;

int fx_unknown_store(void **where); /* no body: what it stores is not known */

/* a comparison through a callback: the key reaches strcmp, a function without a body */
int fx_name_match(const void *key, const FxRec *rec) { return rec->active != 0 && strcmp((const char *)key, rec->name) == 0; }

/* a search loop: the key goes to the callback (an indirect call with one resolved target) */
int fx_find(FxMatch match, const void *key) {
  int i;
  for (i = 0; i < FX_NAMES; ++i)
    if (match(key, &fx_table[i])) return i;
  return -1;
}

/* the lookup API: the name is a copy through a local, then passed down; the result goes out */
int fx_lookup(int *out, const char *name) {
  const char *n = name;
  int idx;
  if (!n) return -2;
  idx = fx_find(fx_name_match, (const void *)n);
  if (idx < 0) return -17;
  *out = idx;
  return 0;
}

/* the create API: the name is stored in a record of the table, the index is a subscript */
int fx_create(int *out, const char *name, int pos) {
  if (pos >= FX_NAMES) return -1;
  fx_table[pos].name = name;
  fx_table[pos].active = 1;
  *out = pos;
  return 0;
}

/* recursion: a parameter that goes round a cycle of calls and is compared on the way */
int fx_down(int level);
int fx_up(int level) { return level > 3 ? level : fx_down(level + 1); }
int fx_down(int level) {
  if (level == 0) return 0;
  return fx_up(level);
}

/* reads through the pointer field: the key of every slot, looked up, created when not found */
static void fx_setup_slots(void) {
  int index;
  for (index = 0; index < FX_SLOTS; index++) {
    if (fx_lookup(&fx_slots[index].handle, fx_global.slots[index].name) == 0) continue;
    (void)fx_create(&fx_global.slots[index].handle, fx_global.slots[index].name, index);
  }
}

/* the writer: clears the record, stores the table's address, then calls the reader */
int fx_init(void) {
  memset(&fx_global, 0, sizeof(fx_global));
  fx_global.slots = fx_slots;
  if (fx_count < 0) return -1;
  fx_setup_slots();
  fx_global.ready = 1;
  return 0;
}

/* a read of the field from a function the writer does not call: no order with the write */
int fx_first_limit(void) { return fx_global.slots[0].limit; }

/* the field of another object of the type, set by its static initializer (the field is written elsewhere) */
int fx_spare_limit(void) { return fx_other.slots->limit; }

/* a pointer field whose address escapes to code without a body: needs field points-to */
int fx_cursor_limit(void) {
  if (fx_unknown_store((void **)&fx_global.cursor) != 0) return -1;
  return fx_global.cursor->limit;
}

/* a switch on a parameter, a parameter passed on to a call through a pointer that is not resolved */
int (*fx_hook)(int);
int fx_dispatch(int code) {
  switch (code) {
    case 1: return fx_hook(code);
    default: return 0;
  }
}

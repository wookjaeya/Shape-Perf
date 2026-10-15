/* spawn.c -- spawn sites and their entry functions.  The profile names fx_spawn as the create
 * operation of a spawn rule. */
typedef void (*task_fn)(void);

int fx_spawn(unsigned *tid, const char *name, task_fn entry);

static void worker_a(void) {}
static void worker_b(void) {}
static void worker_c(void) {}

void start_direct(void) {
  unsigned t;
  fx_spawn(&t, "a", worker_a);
}

static void start_with(task_fn f) {
  unsigned t;
  fx_spawn(&t, "via", f);
}

void start_indirect(void) { start_with(worker_b); }

typedef struct {
  const char *name;
  task_fn entry;
} Desc;

static const Desc g_descs[] = {{"c", worker_c}};

void start_from_table(void) {
  unsigned t;
  fx_spawn(&t, g_descs[0].name, g_descs[0].entry);
}

void start_unknown(task_fn f) {
  unsigned t;
  fx_spawn(&t, "unknown", f);
}

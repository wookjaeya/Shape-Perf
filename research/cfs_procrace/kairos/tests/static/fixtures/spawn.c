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

/* A task record and a trampoline: the function a task runs is stored by the creator and called
 * by the entry every task starts at.  The parameter of start_task is the task-main parameter. */
typedef struct {
  task_fn main;
} TaskRec;

static TaskRec g_task_rec;

static void trampoline(void) { g_task_rec.main(); }

static void start_task(task_fn main_fn) {
  unsigned t;
  g_task_rec.main = main_fn;
  fx_spawn(&t, "task", trampoline);
}

/* A null-terminated table of units; the walk starts a task for every unit that has a main. */
typedef struct {
  const char *name;
  task_fn main;
} Unit;

static const Unit unit_x = {"x", worker_a};
static const Unit unit_y = {"y", 0};
static const Unit unit_z = {"z", worker_c};
static const Unit *const g_units[] = {&unit_x, &unit_y, &unit_z, 0};

void start_units(void) {
  unsigned long i;
  const Unit *u;
  for (i = 0;; i++) {
    u = g_units[i];
    if (u == 0) break;
    if (u->main == 0) continue;
    start_task(u->main);
  }
}

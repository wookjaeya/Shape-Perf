/* loops.c -- loops, their exit expressions, and the walk over a null-terminated table of
 * entry functions (the shape of a startup serialization loop).
 * Targets: Gate.state (--target).  Delay operation: fx_delay (--delay-op).  Spawn: fx_spawn
 * (the profile). */
typedef struct {
  int state;
} Gate;

Gate g_gate;
int g_flag;
int g_count;
Gate g_gates[3];

void fx_delay(int ms);
void fx_abort(void);

/* a poll: leaves when the shared flag has the awaited value */
void poll_flag(void) {
  while (g_flag != 1) {
    fx_delay(5);
  }
}

/* a poll with a budget: the exit expression has two parts; the other exit aborts */
void poll_budget(int timeout) {
  int left = timeout;
  while (left > 0) {
    if (g_gate.state == 2) return;
    fx_delay(10);
    left -= 10;
  }
  fx_abort();
}

/* a counted loop: the trip count is known */
void counted(void) {
  int i;
  for (i = 0; i < 4; i++) g_count += i;
}

/* a loop over a global array of gates, waiting for every one */
void wait_all(void) {
  int i;
  for (i = 0; i < 3; i++) {
    while (g_gates[i].state != 2) fx_delay(1);
  }
}

/* A module table: a configuration object holds a pointer to a null-terminated array of
 * pointers to module objects, each with an init function and a task entry.  Two walks read it:
 * the first calls every init function, the second creates a task for every entry. */
typedef struct {
  const char *name;
  int (*init)(void);
  void (*task)(void);
} Mod;

typedef struct {
  const Mod *const *list;
} Cfg;

int fx_spawn(unsigned *tid, const char *name, void (*entry)(void));

static int a_init(void) {
  g_gate.state = 1;
  return 0;
}
static int b_init(void) {
  g_gate.state = 2;
  return 0;
}
static void b_task(void) {}
static void c_task(void) {}

static const Mod mod_a = {"a", a_init, 0};
static const Mod mod_b = {"b", b_init, b_task};
static const Mod mod_c = {"c", 0, c_task};
static const Mod *const mod_list[] = {&mod_a, &mod_b, &mod_c, 0};
Cfg g_cfg = {mod_list};

void init_all(void) {
  unsigned long i;
  const Mod *e;
  for (i = 0;; i++) {
    e = g_cfg.list[i];
    if (e == 0) break;
    if (e->init != 0) (void)e->init();
  }
}

void spawn_all(void) {
  unsigned long i;
  unsigned tid;
  const Mod *e;
  for (i = 0;; i++) {
    e = g_cfg.list[i];
    if (e == 0) break;
    if (e->task == 0) continue;
    fx_spawn(&tid, e->name, e->task);
  }
}

/* a table loop whose end is tested in the condition: not recognised as a table walk */
static const Mod *const g_flat[] = {&mod_a, &mod_b, 0};
void init_flat(void) {
  int i;
  for (i = 0; g_flat[i] != 0; i++) (void)g_flat[i]->init();
}

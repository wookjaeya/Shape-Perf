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

/* ---- poll loops and the predicate forms of SPEC 4.3 (lock: fx_lock / fx_unlock) ---------------- */
#include <signal.h>

typedef struct {
  int used;
  int state;
} Rec;

Rec g_recs[4];
typedef struct {
  int ready;
  volatile sig_atomic_t phase;
} Sys;

Sys g_sys;

void fx_lock(void);
void fx_unlock(void);

static int rec_used(const Rec *r) { return r->used != 0; }

/* The shape of a startup serialization: under the lock, count the used records that have not
 * reached the awaited state; leave when none is left (form (b): a counter-zero test after a
 * constant-trip loop, with a cursor over the table), or when the budget is spent. */
int sync_delay(int want, int timeout) {
  int status = -1;
  int left = timeout;
  int wait;
  int pending;
  int i;
  const Rec *r;
  while (1) {
    pending = 0;
    fx_lock();
    r = g_recs;
    for (i = 0; i < 4; i++) {
      if (rec_used(r) && r->state < want) {
        ++pending;
      }
      ++r;
    }
    fx_unlock();
    if (pending == 0) {
      status = 0;
      break;
    }
    if (left > 10) {
      wait = 10;
    } else if (left > 0) {
      wait = left;
    } else {
      break;
    }
    fx_delay(wait);
    left -= wait;
  }
  return status;
}

/* The counter is tested by the loop condition, before the reset of the first iteration: the
 * rewriting must not be made. */
void counter_in_condition(void) {
  int pending = 1;
  int i;
  while (pending != 0) {
    pending = 0;
    for (i = 0; i < 4; i++) {
      if (g_recs[i].state != 2) ++pending;
    }
    fx_delay(1);
  }
}

/* A direct test under the lock, but the body writes the tested field through a callee:
 * L1 (i) fails. */
static void bump_gate(void) { g_gate.state++; }
void poll_writer(void) {
  while (1) {
    fx_lock();
    if (g_gate.state == 2) {
      fx_unlock();
      break;
    }
    fx_unlock();
    bump_gate();
    fx_delay(1);
  }
}

/* One test that decides both the condition exit and the budget exit: the part that does not
 * test the budget is the condition. */
void poll_mixed(int timeout) {
  int left = timeout;
  while (1) {
    fx_lock();
    if (g_sys.ready != 0 || left <= 0) {
      fx_unlock();
      break;
    }
    fx_unlock();
    fx_delay(1);
    left -= 1;
  }
}

/* The loop condition reads one volatile sig_atomic_t field: one atomic read (L1 (ii)). */
void wait_phase(int want, int timeout) {
  int left = timeout;
  while (g_sys.phase < want && left > 0) {
    fx_delay(5);
    left -= 5;
  }
}

/* The shape of poll_mixed, but the tested field is not an extraction target: it has no A-TYPE
 * listing, so writes through pointers cannot be excluded and L1 (i) stays undecided. */
void poll_untargeted(void) {
  while (1) {
    fx_lock();
    if (g_recs[0].used != 0) {
      fx_unlock();
      break;
    }
    fx_unlock();
    fx_delay(1);
  }
}

/* ---- locals are told apart by their declaration, not by their name -------------------------------- */

/* The counter shape of sync_delay, after an unrelated block-scoped local of the same name: the
 * counter is still recognised (form (b)), whatever the order of the declarations in memory. */
int sync_decoy(int want) {
  int i;
  const Rec *r;
  {
    int pending = 7;
    (void)pending;
  }
  int pending;
  while (1) {
    pending = 0;
    fx_lock();
    r = g_recs;
    for (i = 0; i < 4; i++) {
      if (r->state < want) ++pending;
      ++r;
    }
    fx_unlock();
    if (pending == 0) break;
    fx_delay(1);
  }
  return 0;
}

/* The exit tests the outer `pending`, which the loop never changes; the inner `pending` counts.
 * The outer one is not a counter: no recognised predicate. */
void sync_shadow(int want) {
  int i;
  const Rec *r;
  int pending = 1;
  while (1) {
    fx_lock();
    {
      int pending = 0;
      r = g_recs;
      for (i = 0; i < 4; i++) {
        if (r->state < want) ++pending;
        ++r;
      }
      (void)pending;
    }
    fx_unlock();
    if (pending == 0) break;
    fx_delay(1);
  }
}

/* One atomic read, and a call result that the test also reads: the callee's reads are not that
 * atomic read, so L1 (ii) does not hold. */
void wait_phase_used(int want) {
  while (g_sys.phase < want && !rec_used(&g_recs[0])) fx_delay(5);
}

/* ---- a table walk through a configuration that is not const ------------------------------------ */

/* The configuration is changed at run time: its static image is not the value the walk reads,
 * so the walk is not unrolled.  (The walks over g_cfg above read a configuration that is not
 * const either, but nothing writes it: they are unrolled under a declared assumption.) */
typedef struct {
  const Mod *const *list;
} LiveCfg;
static const Mod *const alt_list[] = {&mod_c, 0};
LiveCfg g_live = {mod_list};

void switch_live(void) { g_live.list = alt_list; }

void init_live(void) {
  unsigned long i;
  const Mod *e;
  for (i = 0;; i++) {
    e = g_live.list[i];
    if (e == 0) break;
    if (e->init != 0) (void)e->init();
  }
}

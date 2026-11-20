/* kzgcc_target.c -- unit-test target of the kzgcc plugin (not cFS; generic names, unit-test values).
 * The shapes the cFS sites have: a by-name lookup with an out parameter inside a retry loop
 * (CF), a static helper called once with a loop over a table of names (BP's initialize_throttling),
 * stores through a pointer parameter on two branches (CFE_ES_GetAppID), a task created with an id
 * passed to its entry function (OSAL), a call whose result is unused.  Two threads, in the order of the gate below. */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef uint32_t obj_id_t;

struct chan { char sem_name[20]; obj_id_t sem_id; };
struct throttle { const char name[16]; obj_id_t sem; };

struct chan chans[2] = {{"T0_sem", 0}, {"T1_sem", 0}};
struct throttle throttles[2] = {{"T0_sem", 0}, {"T1_sem", 0}};
static char registry[4][16];
static obj_id_t registry_id[4];
static int registry_n;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile int sink;

__attribute__((noinline)) int32_t obj_lookup(obj_id_t *id, const char *name) {
  int32_t r = -17;
  pthread_mutex_lock(&lock);
  for (int i = 0; i < registry_n && i < 4; ++i)
    if (strcmp(registry[i], name) == 0) { *id = registry_id[i]; r = 0; }
  pthread_mutex_unlock(&lock);
  return r;
}

__attribute__((noinline)) int32_t obj_create(obj_id_t *id, const char *name, uint32_t initial) {
  pthread_mutex_lock(&lock);
  int n = registry_n < 3 ? registry_n++ : 3;
  strncpy(registry[n], name, sizeof registry[n] - 1);
  registry_id[n] = 0x30000u + (uint32_t)n + 1u + initial;
  *id = registry_id[n];
  pthread_mutex_unlock(&lock);
  return 0;
}

__attribute__((noinline)) void obj_delay(int ms);  /* defined after consumer_init: kzllvm_lines.kzs names line 64 */

__attribute__((noinline)) int32_t get_own_id(obj_id_t *out, int have) {
  if (have) { *out = 0x110002u; return 0; }
  *out = 0;
  return -3;
}

static void init_throttles(void) {
  for (int i = 0; i < 2; ++i) {
    if (obj_lookup(&throttles[i].sem, throttles[i].name) == 0) sink = 1;
    else if (obj_create(&throttles[i].sem, throttles[i].name, 0) == 0) sink = 2;
  }
}

__attribute__((noinline)) int32_t provider_init(void) { init_throttles(); return 0; }

__attribute__((noinline)) int32_t consumer_init(int tries) {
  int32_t r = -17;
  for (int j = 0; j < tries; ++j) {
    r = obj_lookup(&chans[0].sem_id, chans[0].sem_name);
    if (r != -17) break;
    obj_delay(2);
  }
  return r;
}

/* The order the checks rely on, made explicit (the test's own synchronization, not a site): the provider
 * thread starts its work only after the consumer's first lookup has failed (the consumer's first delay opens
 * the gate), and that delay returns only after the provider has created its names.  So the consumer's
 * lookups are -17 first and then 0 with the id of T0_sem, whichever thread the scheduler runs first. */
static pthread_mutex_t gate_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cond = PTHREAD_COND_INITIALIZER;
static int gate_open, provider_done;

static void open_gate(void) {
  pthread_mutex_lock(&gate_lock);
  gate_open = 1;
  pthread_cond_broadcast(&gate_cond);
  pthread_mutex_unlock(&gate_lock);
}

__attribute__((noinline)) void obj_delay(int ms) {
  open_gate();
  pthread_mutex_lock(&gate_lock);
  while (!provider_done) pthread_cond_wait(&gate_cond, &gate_lock);
  pthread_mutex_unlock(&gate_lock);
  struct timespec t = {0, ms * 1000000L};
  nanosleep(&t, NULL);
}

struct task_arg { obj_id_t id; };

__attribute__((noinline)) void task_entry(obj_id_t id) { sink = (int)id; provider_init(); }

static void *thread_main(void *p) {
  pthread_mutex_lock(&gate_lock);
  while (!gate_open) pthread_cond_wait(&gate_cond, &gate_lock);
  pthread_mutex_unlock(&gate_lock);
  task_entry(((struct task_arg *)p)->id);
  pthread_mutex_lock(&gate_lock);
  provider_done = 1;
  pthread_cond_broadcast(&gate_cond);
  pthread_mutex_unlock(&gate_lock);
  return NULL;
}

__attribute__((noinline)) int32_t task_create(pthread_t *t, struct task_arg *a, obj_id_t id) {
  a->id = id;
  return pthread_create(t, NULL, thread_main, a) == 0 ? 0 : -1;
}

int main(void) {
  obj_id_t self;
  pthread_t t;
  struct task_arg a;
  get_own_id(&self, 0);              /* result unused, first store branch */
  task_create(&t, &a, 0x40009u);
  int32_t r = consumer_init(200);
  open_gate();                       /* had the first lookup not failed, the checks fail instead of the join hanging */
  pthread_join(t, NULL);
  int32_t s = get_own_id(&self, 1);
  printf("consumer %d id 0x%x own %d 0x%x\n", (int)r, (unsigned)chans[0].sem_id, (int)s, (unsigned)self);
  return r == 0 ? 0 : 1;
}

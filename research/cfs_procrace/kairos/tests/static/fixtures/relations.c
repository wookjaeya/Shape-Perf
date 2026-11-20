/* relations.c -- a fixture for `kz-static relations` (paths A, B and C) over the facts of the
 * profile-free mode with the rule targets (extract --no-profile --targets-rule R-T1,R-T2).
 *
 *   A  a registration table searched by name (strcmp in a loop over a global array), a lookup API and a
 *      create API of it; two workers look up and create names: one with a literal of its own, one with a
 *      key from a static configuration table reached through a pointer field, and a probe-create (create
 *      the name when the lookup fails) in one function;
 *   B  a shared field passed to a function that checks it and returns an error constant for a bad value;
 *      the field is written by another thread;
 *   C  a spawn whose creator writes the child's record after the thread is created, under a lock taken
 *      after the spawn (the other order is possible), and one that holds the lock across the spawn and the
 *      write (the other order is proven impossible); the child reads its record at the slot of its own
 *      thread id under the same lock.
 */
#include <pthread.h>
#include <string.h>

#define NSLOTS 4
#define NAMELEN 16
#define BAD_ID 0
#define ERR_NOT_FOUND (-17)
#define ERR_ILLEGAL (-5)

typedef struct {
  int used;
  char name[NAMELEN];
} Slot;

static Slot slots[NSLOTS];
static pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---- path A: the table and its API ------------------------------------------------------------------ */
int slot_find(const char *name, int *out) {
  int i;
  for (i = 0; i < NSLOTS; i++) {
    if (slots[i].used == 1 && strcmp(slots[i].name, name) == 0) {
      *out = i;
      return 0;
    }
  }
  return ERR_NOT_FOUND;
}

int slot_create(const char *name, int *out) {
  int i;
  for (i = 0; i < NSLOTS; i++) {
    if (slots[i].used == 1 && strcmp(slots[i].name, name) == 0) {
      return ERR_ILLEGAL;
    }
  }
  for (i = 0; i < NSLOTS; i++) {
    if (slots[i].used == 0) {
      strncpy(slots[i].name, name, NAMELEN - 1);
      slots[i].used = 1;
      *out = i;
      return 0;
    }
  }
  return ERR_NOT_FOUND;
}

void slot_release(int i) {
  slots[i].used = 0;
}

typedef struct {
  const char *name;
} Config;

static const Config configs[2] = {{"alpha"}, {"beta"}};

typedef struct {
  const Config *cfg;
  int handle;
} Worker;

static Worker worker;

/* the key comes from the static configuration table through the pointer field */
void worker_setup(void) {
  int i;
  worker.cfg = configs;
  for (i = 0; i < 2; i++) {
    if (slot_find(worker.cfg[i].name, &worker.handle) != 0) {
      (void)slot_create(worker.cfg[i].name, &worker.handle);
    }
  }
}

/* ---- path B: a shared id passed to a function that checks it ---------------------------------------- */
typedef struct {
  int self_id;
} Service;

static Service service;

int notify(int id, int code) {
  if (id == BAD_ID) return ERR_ILLEGAL;
  return code;
}

void *service_main(void *arg) {
  (void)arg;
  service.self_id = 7;
  return 0;
}

void *client_main(void *arg) {
  int h = 0;
  (void)arg;
  notify(service.self_id, 1);
  notify(service.self_id, 2);
  (void)slot_find("gamma", &h);
  return 0;
}

void *other_main(void *arg) {
  int h = 0;
  (void)arg;
  (void)slot_create("gamma", &h);
  (void)slot_create("delta", &h);
  return 0;
}

/* ---- path C: a record written after the spawn, read by the child at its own slot --------------------- */
typedef struct {
  int registered;
  pthread_t id;
} TaskRec;

static TaskRec tasks[NSLOTS];
static pthread_key_t self_key;

int task_self(void) {
  return (int)(long)pthread_getspecific(self_key);
}

int task_check(void) {
  int r = ERR_NOT_FOUND;
  pthread_mutex_lock(&table_mutex);
  if (tasks[task_self()].registered == 1) r = 0;
  pthread_mutex_unlock(&table_mutex);
  return r;
}

void *child_main(void *arg) {
  (void)arg;
  (void)task_check();
  return 0;
}

int task_spawn(int slot) {
  int rc = pthread_create(&tasks[slot].id, 0, child_main, 0);
  pthread_mutex_lock(&table_mutex);
  tasks[slot].registered = 1;
  pthread_mutex_unlock(&table_mutex);
  return rc;
}

int task_spawn_locked(int slot) {
  int rc;
  pthread_mutex_lock(&table_mutex);
  rc = pthread_create(&tasks[slot].id, 0, child_main, 0);
  tasks[slot].registered = 1;
  pthread_mutex_unlock(&table_mutex);
  return rc;
}

int main(void) {
  pthread_t a, b, c;
  worker_setup();
  pthread_create(&a, 0, service_main, 0);
  pthread_create(&b, 0, client_main, 0);
  pthread_create(&c, 0, other_main, 0);
  (void)task_spawn(0);
  (void)task_spawn_locked(1);
  slot_release(0);
  return 0;
}

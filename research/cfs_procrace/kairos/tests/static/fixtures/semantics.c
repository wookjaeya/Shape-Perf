/* semantics.c -- a fixture for the semantics join (extract --semantics, static_facts.md 5t) over the facts of the
 * profile-free mode with the rule targets (extract --no-profile --targets-rule R-T1,R-T2).  The file is both the
 * "public header" and the "source" of a small framework for the mechanical scan of docs/semantics_coverage.md (its data
 * file tests/static/data/semantics_fixture/ is the output of that scan):
 *
 *   a status type and its constants; event types; run states;
 *   reg_find: documented returns, one of them a value the body cannot return; its result passes through reg_search;
 *   reg_create: no documentation, an entry by the constants its body returns (rule E3);
 *   send_event: an event-type parameter documented with its constants; app_exit: a run-status parameter, and a terminal;
 *   two "apps" (threads through pthread_create): app one looks a name up, app two creates it, so that relations has rows
 *   that name the functions of the data.
 */
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef int sem_status_t;

/* status constants */
#define SEM_OK 0
#define SEM_ERR_POINTER (-2)
#define SEM_ERR_TOO_LONG (-13)
#define SEM_ERR_NO_FREE (-14)
#define SEM_ERR_NOT_FOUND (-17)
#define SEM_ERR_NEVER (-99)

/* event types */
#define SEM_EV_DEBUG 1
#define SEM_EV_ERROR 3

/* run states */
#define SEM_RUN 1
#define SEM_RUN_ERROR 3

#define NREGS 4
#define NAMELEN 16

/**
 * @brief Find a registered name
 * @param[out] out the index of the name
 * @param[in]  name the name to find
 * @retval #SEM_OK found
 * @retval #SEM_ERR_POINTER a null argument
 * @retval #SEM_ERR_NOT_FOUND no record has the name
 * @retval #SEM_ERR_NEVER documented, but the body never returns it
 */
sem_status_t reg_find(int *out, const char *name);

sem_status_t reg_create(int *out, const char *name);

/**
 * @brief Send an event
 * @param[in] id the event id
 * @param[in] type the event type, one of:
 *            \arg #SEM_EV_DEBUG
 *            \arg #SEM_EV_ERROR
 * @retval #SEM_OK sent
 */
sem_status_t send_event(int id, unsigned short type);

/**
 * @brief End the application
 * @param[in] status one of \arg #SEM_RUN \arg #SEM_RUN_ERROR
 */
void app_exit(unsigned int status);

int helper_count(int x);

typedef struct {
  int used;
  char name[NAMELEN];
} Reg;

static Reg regs[NREGS];
static int events;

static int reg_search(const char *name, int *out) {
  int i;
  for (i = 0; i < NREGS; i++) {
    if (regs[i].used == 1 && strcmp(regs[i].name, name) == 0) {
      *out = i;
      return SEM_OK;
    }
  }
  return SEM_ERR_NOT_FOUND;
}

sem_status_t reg_find(int *out, const char *name) {
  if (out == 0 || name == 0) return SEM_ERR_POINTER;
  return reg_search(name, out);
}

sem_status_t reg_create(int *out, const char *name) {
  int i;
  if (out == 0 || name == 0) return SEM_ERR_POINTER;
  if (memchr(name, 0, NAMELEN) == 0) return SEM_ERR_TOO_LONG;
  for (i = 0; i < NREGS; i++) {
    if (regs[i].used == 0) {
      strncpy(regs[i].name, name, NAMELEN - 1);
      regs[i].used = 1;
      *out = i;
      return SEM_OK;
    }
  }
  return SEM_ERR_NO_FREE;
}

sem_status_t send_event(int id, unsigned short type) {
  events = events + id + type;
  return SEM_OK;
}

void app_exit(unsigned int status) { exit((int)status); }

int helper_count(int x) { return x + 1; }

static int one_handle, two_handle;

void *one_main(void *arg) {
  (void)arg;
  if (reg_find(&one_handle, "lime") != SEM_OK) {
    (void)send_event(7, SEM_EV_ERROR);
    app_exit(SEM_RUN_ERROR);
  }
  return 0;
}

void *two_main(void *arg) {
  (void)arg;
  (void)reg_create(&two_handle, "lime");
  two_handle = helper_count(two_handle);
  return 0;
}

int main(void) {
  pthread_t a, b;
  (void)pthread_create(&a, 0, one_main, 0);
  (void)pthread_create(&b, 0, two_main, 0);
  return 0;
}

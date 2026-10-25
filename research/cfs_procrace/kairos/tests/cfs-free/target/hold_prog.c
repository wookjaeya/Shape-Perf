/*
 * hold_prog.c -- a small two-thread program for testing `kz hold` without any framework.
 *
 * It has the shape of the C1 start-up race in miniature:
 *   - a "provider" thread that creates two named objects in order (hp_obj_create("OBJ0", ...),
 *     then hp_obj_create("OBJ1", ...)), like BP's two counting semaphores;
 *   - a "consumer" thread that looks a name up in a bounded retry loop (hp_obj_lookup), like CF's
 *     start-up lookup, and calls hp_consumer_done when the loop ends;
 *   - both threads are created by the main thread AFTER the program starts, so the supervisor must
 *     auto-attach them (PTRACE_O_TRACECLONE) and arm their debug registers at birth.
 *
 * The hold test gates the provider right before hp_obj_create of "OBJ0" (a name-matched gate) and
 * releases it when the consumer reaches hp_consumer_done (an observed event).  While the provider is
 * held the consumer's lookups find nothing; after the release the provider creates the objects.
 *
 * Built -g -O0 -fno-omit-frame-pointer -fcf-protection=full so every function starts
 * endbr64; push %rbp; mov %rsp,%rbp, as the framework's functions do; the first instruction after
 * the entry marker (entry+4) holds the arguments in the argument registers and the return address
 * at the top of the stack, which is what the gate reads.
 *
 * usage: hold_prog [retries] [provider_delay_ms]
 *   retries         consumer lookup attempts (default 20)
 *   provider_delay_ms  how long the provider waits before it starts (default 30)
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct hp_obj { char name[32]; unsigned id; int live; };
static struct hp_obj g_table[8];
static atomic_int g_count;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* Create a named object; returns 0 and sets *id_out on success.  The gate sits at the entry of this
 * function: the name is the second argument (rsi on x86-64). */
__attribute__((noinline)) int hp_obj_create(unsigned* id_out, const char* name, unsigned initial) {
  (void)initial;
  pthread_mutex_lock(&g_lock);
  int n = atomic_load(&g_count);
  unsigned id = 0x30000u + (unsigned)n;
  strncpy(g_table[n].name, name, sizeof g_table[n].name - 1);
  g_table[n].id = id;
  g_table[n].live = 1;
  atomic_store(&g_count, n + 1);
  pthread_mutex_unlock(&g_lock);
  *id_out = id;
  return 0;
}

/* Look a name up; returns 0 and sets *id_out if found, 1 if not found. */
__attribute__((noinline)) int hp_obj_lookup(unsigned* id_out, const char* name) {
  pthread_mutex_lock(&g_lock);
  int rc = 1;
  for (int i = 0; i < atomic_load(&g_count); i++) {
    if (g_table[i].live && strcmp(g_table[i].name, name) == 0) { *id_out = g_table[i].id; rc = 0; break; }
  }
  pthread_mutex_unlock(&g_lock);
  return rc;
}

/* The consumer calls this once its retry loop ends -- the release event. */
__attribute__((noinline)) void hp_consumer_done(int found, int attempts) {
  printf("consumer done: found=%d attempts=%d\n", found, attempts);
  fflush(stdout);
}

static int g_retries = 20;
static int g_provider_delay_ms = 30;

static void* provider_thread(void* a) {
  (void)a;
  usleep((useconds_t)g_provider_delay_ms * 1000);
  unsigned id0 = 0, id1 = 0;
  hp_obj_create(&id0, "OBJ0", 10);
  hp_obj_create(&id1, "OBJ1", 10);
  return NULL;
}

static void* consumer_thread(void* a) {
  (void)a;
  unsigned id = 0;
  int found = 0, attempts = 0;
  for (attempts = 0; attempts < g_retries; attempts++) {
    if (hp_obj_lookup(&id, "OBJ0") == 0) { found = 1; break; }
    usleep(5000);
  }
  hp_consumer_done(found, attempts);
  return NULL;
}

int main(int argc, char** argv) {
  if (argc > 1) g_retries = atoi(argv[1]);
  if (argc > 2) g_provider_delay_ms = atoi(argv[2]);
  pthread_t prov, cons;
  pthread_create(&prov, NULL, provider_thread, NULL);
  pthread_create(&cons, NULL, consumer_thread, NULL);
  pthread_join(cons, NULL);
  pthread_join(prov, NULL);
  printf("objects=%d\n", atomic_load(&g_count));
  return 0;
}

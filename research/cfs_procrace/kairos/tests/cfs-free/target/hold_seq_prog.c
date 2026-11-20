/*
 * hold_seq_prog.c -- a small two-thread program for testing two properties of `kz hold` without any
 * framework: (1) a thread resumed from a gate can still fire later releases (its debug registers stay
 * armed), and (2) `occurrence <n>`: a gate or release that acts on the n-th qualifying hit only.
 *
 * The shape is the C1 start-up race with both keys, in miniature:
 *   - a "provider" thread creates two named objects in order (seq_create("OBJ0"), then
 *     seq_create("OBJ1")), calling seq_provider_created(index) after each, like BP's two semaphores;
 *   - a "consumer" thread looks up "OBJ0" in a bounded retry loop, then "OBJ1" in another, calling
 *     seq_consumer_retry(key, attempt) after each failed lookup (like CF's retry delay) and
 *     seq_consumer_done(...) when both loops end, like CF's start-up lookups;
 *   - both threads are created after the program starts (the supervisor auto-attaches them).
 *
 * Built -g -O0 -fno-omit-frame-pointer -fcf-protection=full so every function starts
 * endbr64; push %rbp; mov %rsp,%rbp, as the framework's functions do.
 *
 * usage: hold_seq_prog [retries] [provider_delay_ms] [consumer_delay_ms] [retry_delay_ms]
 *   retries            lookup attempts per key (default 20)
 *   provider_delay_ms  how long the provider waits before it starts (default 30)
 *   consumer_delay_ms  how long the consumer waits before it starts (default 0)
 *   retry_delay_ms     the consumer's wait after a failed lookup (default 5)
 * Unit-test values, not research conditions.
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct seq_obj { char name[32]; unsigned id; int live; };
static struct seq_obj g_table[8];
static atomic_int g_count;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* Create a named object; the name is the second argument (rsi on x86-64). */
__attribute__((noinline)) int seq_create(unsigned* id_out, const char* name, unsigned initial) {
  (void)initial;
  pthread_mutex_lock(&g_lock);
  int n = atomic_load(&g_count);
  unsigned id = 0x40000u + (unsigned)n;
  strncpy(g_table[n].name, name, sizeof g_table[n].name - 1);
  g_table[n].id = id;
  g_table[n].live = 1;
  atomic_store(&g_count, n + 1);
  pthread_mutex_unlock(&g_lock);
  *id_out = id;
  return 0;
}

/* Look a name up; 0 and *id_out if found, 1 if not. */
__attribute__((noinline)) int seq_lookup(unsigned* id_out, const char* name) {
  pthread_mutex_lock(&g_lock);
  int rc = 1;
  for (int i = 0; i < atomic_load(&g_count); i++) {
    if (g_table[i].live && strcmp(g_table[i].name, name) == 0) { *id_out = g_table[i].id; rc = 0; break; }
  }
  pthread_mutex_unlock(&g_lock);
  return rc;
}

/* The provider's event after each creation. */
__attribute__((noinline)) void seq_provider_created(int index) { __asm__ volatile("" ::"r"(index)); }

/* The consumer's event after each failed lookup (before its wait). */
__attribute__((noinline)) void seq_consumer_retry(int key, int attempt) { __asm__ volatile("" ::"r"(key), "r"(attempt)); }

/* The consumer's event when both loops end. */
__attribute__((noinline)) void seq_consumer_done(int found0, int attempts0, int found1, int attempts1) {
  printf("consumer done: found0=%d attempts0=%d found1=%d attempts1=%d\n", found0, attempts0, found1, attempts1);
  fflush(stdout);
}

static int g_retries = 20, g_provider_delay_ms = 30, g_consumer_delay_ms = 0, g_retry_delay_ms = 5;

static void* provider_thread(void* a) {
  (void)a;
  if (g_provider_delay_ms > 0) usleep((useconds_t)g_provider_delay_ms * 1000);
  unsigned id0 = 0, id1 = 0;
  seq_create(&id0, "OBJ0", 10);
  seq_provider_created(0);
  seq_create(&id1, "OBJ1", 10);
  seq_provider_created(1);
  return NULL;
}

static int lookup_loop(int key, const char* name, int* attempts) {
  unsigned id = 0;
  for (*attempts = 0; *attempts < g_retries; ++*attempts) {
    if (seq_lookup(&id, name) == 0) return 1;
    seq_consumer_retry(key, *attempts);
    if (g_retry_delay_ms > 0) usleep((useconds_t)g_retry_delay_ms * 1000);
  }
  return 0;
}

static void* consumer_thread(void* a) {
  (void)a;
  if (g_consumer_delay_ms > 0) usleep((useconds_t)g_consumer_delay_ms * 1000);
  int a0 = 0, a1 = 0;
  const int f0 = lookup_loop(0, "OBJ0", &a0);
  const int f1 = lookup_loop(1, "OBJ1", &a1);
  seq_consumer_done(f0, a0, f1, a1);
  return NULL;
}

int main(int argc, char** argv) {
  if (argc > 1) g_retries = atoi(argv[1]);
  if (argc > 2) g_provider_delay_ms = atoi(argv[2]);
  if (argc > 3) g_consumer_delay_ms = atoi(argv[3]);
  if (argc > 4) g_retry_delay_ms = atoi(argv[4]);
  pthread_t prov, cons;
  pthread_create(&prov, NULL, provider_thread, NULL);
  pthread_create(&cons, NULL, consumer_thread, NULL);
  pthread_join(cons, NULL);
  pthread_join(prov, NULL);
  printf("objects=%d\n", atomic_load(&g_count));
  return 0;
}

/*
 * target_prog.c -- a small program for testing the observer without any framework.
 *
 * It has the same shape as what the observer meets in the real target, in miniature:
 *   - a variadic function that returns a status and is called from two wrappers
 *     (tp_send, called from tp_wrap_a and tp_wrap_b);
 *   - a function that publishes a value through a pointer and ignores its status, followed by
 *     a source line to probe after the call (tp_publish);
 *   - a task-creation function and a child-side entry function (tp_task_create, tp_task_entry),
 *     and threads that name themselves after they start;
 *   - burst and multi-thread modes that make many calls.
 * It is built by gcc/clang with -g -O0 -fno-omit-frame-pointer -fcf-protection=full so that every
 * function starts endbr64; push %rbp; mov %rsp,%rbp, as the framework's functions do.
 *
 * The line numbers of this file are data of the test profile (target.kzp): the test checks that
 * the line it names contains the call it expects, so an edit here that moves the line fails loudly.
 *
 * usage: target_prog basic
 *        target_prog burst <calls>
 *        target_prog threads <threads> <calls each>
 *        target_prog exit <code>
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned g_identity; /* 0 until tp_publish stores it */

struct tp_task {
  unsigned id;
  const char* name;
  void* (*entry)(void*);
  unsigned priority;
  void* arg;
};

static unsigned g_next_id = 0x100;
static pthread_t g_threads[64];
static unsigned g_nthreads;

__attribute__((noinline)) int tp_get_identity(unsigned* out) {
  *out = 0x110003u;
  return 0;
}

__attribute__((noinline)) int tp_send(unsigned short event_id, unsigned short event_type, unsigned app_id, const char* spec, ...) {
  va_list ap;
  va_start(ap, spec);
  va_end(ap);
  (void)event_id;
  (void)event_type;
  return app_id == 0 ? (int)0xc2000005u : 0;
}

__attribute__((noinline)) int tp_wrap_a(unsigned app_id) { return tp_send(5, 2, app_id, "a"); }
__attribute__((noinline)) int tp_wrap_b(unsigned app_id) { return tp_send(10, 3, app_id, "b"); }

__attribute__((noinline)) void tp_publish(void) {
  tp_get_identity(&g_identity);
  tp_wrap_a(g_identity);
}

__attribute__((noinline)) void tp_task_entry(unsigned id) { (void)id; }

static void* tp_thread_start(void* p) {
  struct tp_task t = *(struct tp_task*)p;
  free(p);
  tp_task_entry(t.id);
  pthread_setname_np(pthread_self(), t.name);
  return t.entry(t.arg);
}

__attribute__((noinline)) int tp_task_create(unsigned* id, const char* name, void* (*entry)(void*), unsigned priority, void* arg) {
  struct tp_task* t = malloc(sizeof *t);
  if (t == NULL || g_nthreads >= 64) return -1;
  t->id = g_next_id++;
  t->name = name;
  t->entry = entry;
  t->priority = priority;
  t->arg = arg;
  *id = t->id;
  return pthread_create(&g_threads[g_nthreads++], NULL, tp_thread_start, t) == 0 ? 0 : -2;
}

static void join_all(void) {
  for (unsigned i = 0; i < g_nthreads; ++i) pthread_join(g_threads[i], NULL);
  g_nthreads = 0;
}

static void* worker1(void* arg) {
  (void)arg;
  tp_wrap_a(0);
  tp_wrap_b(0);
  tp_wrap_a(7);
  return NULL;
}

static void* worker2(void* arg) {
  (void)arg;
  tp_wrap_b(0);
  tp_wrap_a(0);
  return NULL;
}

static void* looper(void* arg) {
  const long n = (long)(intptr_t)arg;
  for (long i = 0; i < n; ++i) tp_wrap_a((unsigned)(i & 1));
  return NULL;
}

int main(int argc, char** argv) {
  const char* mode = argc > 1 ? argv[1] : "basic";
  unsigned id = 0;
  if (strcmp(mode, "basic") == 0) {
    /* one task at a time, so that the expected events are the same on every run */
    tp_task_create(&id, "WORKER_1", worker1, 11, NULL);
    join_all();
    tp_task_create(&id, "WORKER_2", worker2, 22, NULL);
    join_all();
    tp_publish();
    tp_wrap_b(3);
    return 0;
  }
  if (strcmp(mode, "burst") == 0 && argc > 2) {
    looper((void*)(intptr_t)atol(argv[2]));
    return 0;
  }
  if (strcmp(mode, "threads") == 0 && argc > 3) {
    const int n = atoi(argv[2]);
    static char names[16][16];
    for (int i = 0; i < n && i < 16; ++i) {
      snprintf(names[i], sizeof names[i], "T%02d", i);
      tp_task_create(&id, names[i], looper, 33, (void*)(intptr_t)atol(argv[3]));
    }
    join_all();
    return 0;
  }
  if (strcmp(mode, "exit") == 0 && argc > 2) return atoi(argv[2]);
  fprintf(stderr, "usage: target_prog basic | burst N | threads T N | exit CODE\n");
  return 64;
}

/*
 * Anchor cases of W2 (2026-10-24), appended so that the line numbers above stay: a call of a
 * shared-library function through its PLT stub (memset), a store that has no call on its line, and
 * a call whose return site is also the target of the branch that skips it (a join of two paths).
 * They are not called: the anchor test only resolves them.
 */
__attribute__((noinline)) void tp_clear(struct tp_task* t, size_t n) {
  memset(t, 0, n);
  g_identity = 0x110004u;
}

static unsigned g_join_identity;

__attribute__((noinline)) int tp_join(int c) {
  if (c != 0)
    tp_get_identity(&g_join_identity);
  return 0;
}

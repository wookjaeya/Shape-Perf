/* kzgcc_v4.c -- target of the kzgcc kzs-sites 4 test (kzgcc 0.4, docs/event_semantics.md 13.6): two calls of one
 * callee on one line told apart by the column, a nested call, a single call, a field load and a thread creation with a
 * spawn statement.  Generic names, unit-test values; not cFS code. */
#include <pthread.h>
#include <stdio.h>

__attribute__((noinline)) int pick(int x) { return x * 10; }
__attribute__((noinline)) int pair(int a, int b) { return a + b; }
struct rec {
  int used;
};
struct rec g_rec = {5};
static void *worker(void *arg) { return arg; }

int two_on_line(int a, int b) {
  int r = pick(a) + pick(b);
  r += pair(pick(r), 1);
  r += pick(7);
  return r;
}

int reader(void) { return g_rec.used; }

int spawn_one(void) {
  pthread_t t;
  void *res;
  if (pthread_create(&t, NULL, worker, NULL) != 0) return -1;
  (void)pthread_join(t, &res);
  return 0;
}

int main(void) {
  printf("v4 %d %d %d\n", two_on_line(1, 2), reader(), spawn_one());
  return 0;
}

/* kzgcc_outagg.c -- target of the kzgcc out-of-aggregate test (kzgcc 0.5, docs/event_semantics.md 13.7): after-call
 * reads out0 = *arg0 where arg0 points to a character array of a structure (20 bytes), to a small structure (8 bytes)
 * and to a scalar.  Generic names, unit-test values; not cFS code. */
#include <stdio.h>

struct small {
  int a;
  int b;
};
struct packet {
  int id;
  char name[20];
};
struct packet g_pkt;

__attribute__((noinline)) int name_of(char *out, int id, unsigned len) { return snprintf(out, len, "n%d", id) > 0 ? 0 : -1; }
__attribute__((noinline)) int fill_small(struct small *s) { s->a = 3; s->b = 4; return 1; }
__attribute__((noinline)) int fill_int(int *v) { *v = 42; return 2; }

int run(void) {
  struct small sm;
  int v = 0;
  int r = name_of(g_pkt.name, 7, sizeof(g_pkt.name));
  r += fill_small(&sm);
  r += fill_int(&v);
  return r + sm.a + sm.b + v;
}

int main(void) {
  printf("outagg %d %s\n", run(), g_pkt.name);
  return 0;
}

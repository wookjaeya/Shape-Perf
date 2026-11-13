/* tailjump_prog.c -- a purpose-built program for the entry-reached anchor tests (unit-test values only).
 *
 * Built at -O2 without -g by both compilers (tests/cfs-free/CMakeLists.txt), like a release build: the
 * lookup reaches the search by a tail jump, so the point "after the call of the search" never fires and
 * the search's entry is probed instead (enter=).  Other functions reach the same search too: from a second
 * caller, twice from one function (refused as a hop: two transfers), and a libc function through its PLT
 * stub (refused: another file).  The search records the id through a token and a finishing call, the
 * shape the observer reads an out value from (a memory read at the token).
 */
#include <stdio.h>
#include <string.h>

#if defined(__clang__)
#define KZ_NOIPA __attribute__((noinline))
#define KZ_LEAF __attribute__((noinline, optnone)) /* no specialized clone of the leaf */
#else
#define KZ_NOIPA __attribute__((noipa))
#define KZ_LEAF __attribute__((noipa))
#endif

typedef struct {
  unsigned mode, key, type, idx, id;
} token_t;

KZ_LEAF void finish(token_t *tok, const unsigned *final_id) {
  if (final_id != NULL) tok->id = *final_id;
  __asm__ volatile("" ::: "memory");
}

KZ_NOIPA int find_by_name(unsigned type, const char *name, unsigned *id) {
  token_t tok;
  memset(&tok, 0, sizeof tok);
  if (name == NULL) return -2;
  if (strcmp(name, "alpha") != 0) return -17;
  tok.type = type;
  tok.id = 0x30001u + type;
  *id = tok.id;
  finish(&tok, NULL);
  return 0;
}

KZ_NOIPA int lookup_by_name(unsigned *id, const char *name) {
  if (id == NULL || name == NULL) return -2;
  return find_by_name(3, name, id); /* a tail call at -O2 */
}

KZ_NOIPA int other_lookup(unsigned *id, const char *name) {
  if (id == NULL) return -2;
  return find_by_name(4, name, id);
}

KZ_NOIPA int two_calls(unsigned *id) {
  const int a = find_by_name(1, "alpha", id);
  const int b = find_by_name(2, "beta", id);
  return a + b;
}

KZ_NOIPA int via_libc(const char *s) { return puts(s); }

int main(int argc, char **argv) {
  unsigned id = 0;
  const char *name = argc > 1 ? argv[1] : "alpha";
  int r = lookup_by_name(&id, name);
  r += other_lookup(&id, "beta");
  r += two_calls(&id);
  r += lookup_by_name(&id, "gamma");
  via_libc("tailjump");
  return r == -51 ? 0 : 1;
}

/* kzgcc_owner.c -- unit-test target of the owner-publish link (kzgcc 0.3, kzs-sites 3; kzgcc_owner.kzs): a request that
 * picks its slot inside the call and hands it back after its work, through the caller's out parameter (the publish) and
 * a table (the check).  The second call fails before either store.  Unit-test values only. */
#include <stdint.h>
#include <stdio.h>

typedef struct {
  uint32_t app;
} entry_t;

static entry_t table[4];
static volatile int fail_at = 2;

int create_app(uint32_t *out, int k) {
  uint32_t i = (uint32_t)k + 1u;
  if (k == fail_at) return -1;
  table[k].app = i;
  *out = i;
  return 0;
}

int main(void) {
  uint32_t a = 0, b = 0;
  int r1 = create_app(&a, 1);
  int r2 = create_app(&b, 2);
  printf("owner %d %u %d %u\n", r1, a, r2, b);
  return 0;
}

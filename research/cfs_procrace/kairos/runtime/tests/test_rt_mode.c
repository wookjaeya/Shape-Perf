/* test_rt_mode -- one binary, the mode of the run from KZ_RT_MODE (the start-up object's .preinit_array
 * entry; event_semantics.md 8.2): the groups G-I0 (off), G-I (record) and G-I-H (control) of the review
 * section 13.1 differ only in the environment.  The program runs a hook in a constructor of the highest
 * priority, which runs before every default-priority constructor, the start-up object's included, and
 * three in main, then a control hook (no plan: inactive).  The test cases run this same executable with
 * different KZ_RT_MODE values and check the dumps with kz-rt-dump (tests/CMakeLists.txt). */
#include "rt_test.h"

static uint32_t g_ctor_aseq = 0xFFFFFFFFu;

__attribute__((constructor(101))) static void early_instrumented_code(void) {
  /* stands for instrumented code that runs before main and before the start-up object's constructor */
  g_ctor_aseq = kz_rt_obs(1, 101, 0, 1u, 0xC0, 0, 0, 0);
}

int main(void) {
  const char *m = getenv("KZ_RT_MODE");
  const int off = m != NULL && strcmp(m, "off") == 0;
  uint32_t a1, a2, a3;
  CHECK(off ? g_ctor_aseq == 0 : g_ctor_aseq == 1); /* off: not even the first hook recorded */
  a1 = kz_rt_obs(2, 201, 0, 1u, 1, 0, 0, 0);
  a2 = kz_rt_obs(2, 201, a1, 1u, 2, 0, 0, 0);
  a3 = kz_rt_obs(3, 301, 0, 1u, 3, 0, 0, 0);
  CHECK(off ? (a1 | a2 | a3) == 0 : (a1 != 0 && a2 == a1 + 1 && a3 == a2 + 1));
  CHECK(kz_rt_ctl(3, 301, a3, KZ_RT_CTX_TASK) == KZ_RT_CTL_PASS);
  printf("mode %s: constructor hook aseq %u, main hooks %u %u %u\n", m != NULL ? m : "(unset)", g_ctor_aseq, a1, a2, a3);
  return 0;
}

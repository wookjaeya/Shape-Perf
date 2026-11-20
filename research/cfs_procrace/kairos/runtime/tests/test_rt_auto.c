/* test_rt_auto -- with the kzrt-posix-auto object linked, a program that never calls the runtime's
 * setup functions records from main and leaves a dump at exit (KZ_RT_DUMP, set by the test); the
 * dump names the process and its initial thread (the thread that ran the constructor, here main's). */
#include "rt_test.h"

int main(void) {
  kz_rt_caps caps;
  CHECK(kz_rt_obs(1, 101, 0, 1u, 7, 0, 0, 0) == 1);
  CHECK(kz_rt_obs(2, 201, 1, 1u, 8, 0, 0, 0) == 2);
  CHECK(kz_rt_obs(3, 301, 0, 1u, 9, 0, 0, 0) == 3);
  kz_rt_caps_get(&caps);
  CHECK(caps.adapter_caps != 0); /* attached before main */
  /* the constructor recorded the process and its initial thread (RUNX 1.2): a second call is refused */
  CHECK(kz_rt_process(1, 1, KZ_RT_KEY_LINUX_TID, 0) == KZ_RT_ESTATE);
  printf("auto: 3 events, dump at exit\n");
  return 0;
}

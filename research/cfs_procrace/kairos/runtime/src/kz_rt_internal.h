/* kz_rt_internal.h -- what the runtime core's two translation units (kz_rt.c, kz_rt_plan.c) share; not part of the
 * interface.  The plan's bytes and outcome live in the store (kz_rt.h, "the record store"), so that a dump made from
 * a store file names the plan the run had; kz_rt.c owns the store and hands kz_rt_plan.c its part. */
#ifndef KZ_RT_INTERNAL_H
#define KZ_RT_INTERNAL_H

#include <stdint.h>

#include "kz_rt.h"

#define KZ_RT_HIDDEN __attribute__((visibility("hidden")))

typedef struct kz_rt__plan_mem {
  uint32_t once, set, status, flags, kept, given;
  unsigned char bytes[KZ_RT_PLAN_MAX_BYTES];
} kz_rt__plan_mem;

/* kz_rt.c: the plan's part of the store */
KZ_RT_HIDDEN kz_rt__plan_mem *kz_rt__plan_mem_get(void);
/* kz_rt_plan.c: the PLAN record's content (0 = nothing to record) */
KZ_RT_HIDDEN uint32_t kz_rt__plan_record(const unsigned char **bytes, uint32_t *kept, uint32_t *given, uint32_t *flags);

#endif /* KZ_RT_INTERNAL_H */

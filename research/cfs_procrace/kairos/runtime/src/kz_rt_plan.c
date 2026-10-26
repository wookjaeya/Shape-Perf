/* kz_rt_plan.c -- the serialized control plan "kzrt-plan 1" (kz_rt_format.h): kz_rt_plan_load checks and loads
 * it, and keeps its bytes and the outcome for the dump's PLAN record (kz_rt.c).  Part of the runtime core: C99,
 * freestanding, no library calls, static memory only (event_semantics.md section 4, 8.2).
 *
 * The plan is the host's conversion of a .kzh plan for one build (kz rtplan): its points name the build's site
 * and binding ids.  The check that the plan belongs to this build is the manifest sha256: the plan names the
 * logical site manifest its binding list was produced with, and the binary's build identity (kz_rt_build_info)
 * names the manifest its compiler binding read.  A plan for another manifest is refused before any point is set
 * (the run then has no plan, and the dump says why). */
#include <stddef.h>
#include <stdint.h>

#include "kz_rt.h"
#include "kz_rt_format.h"

static unsigned char g_bytes[KZ_RT_PLAN_MAX_BYTES];
static uint32_t g_once, g_set, g_status, g_flags, g_kept, g_given;

static uint32_t g16(const unsigned char *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8); }
static uint32_t g32(const unsigned char *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint64_t g64(const unsigned char *p) { return (uint64_t)g32(p) | ((uint64_t)g32(p + 4) << 32); }

static uint32_t crc32_of(const unsigned char *p, uint32_t n) {
  uint32_t c = 0xFFFFFFFFu, i, b;
  for (i = 0; i < n; ++i) {
    c ^= p[i];
    for (b = 0; b < 8u; ++b) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return c ^ 0xFFFFFFFFu;
}

#if defined(__GNUC__) || defined(__clang__)
extern const kz_rt_build kz_rt_build_info __attribute__((weak));
#endif

static int hexd(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

/* The build identity's manifest sha256: 1 = given and valid, 0 = no build identity or no manifest, -1 = malformed
 * (the same reading as the RUN record's, kz_rt.c). */
static int build_sha(unsigned char out[32]) {
  const char *h = NULL;
  uint32_t i;
#if defined(__GNUC__) || defined(__clang__)
  {
    const kz_rt_build *b = &kz_rt_build_info;
    if (b != NULL && b->abi == KZ_RT_ABI_VERSION) h = b->manifest_sha256_hex;
  }
#endif
  if (h == NULL) return 0;
  for (i = 0; i < 32u; ++i) {
    const int a = hexd(h[2u * i]), c = a < 0 ? -1 : hexd(h[2u * i + 1u]);
    if (a < 0 || c < 0) return -1;
    out[i] = (unsigned char)((a << 4) | c);
  }
  return h[64] == 0 ? 1 : -1;
}

/* Records the outcome (the first call only; the bytes were copied by the caller). */
static void settle(uint32_t status, uint32_t flags) {
  g_status = status;
  g_flags = flags;
  __atomic_store_n(&g_set, 1u, __ATOMIC_RELEASE);
}

KZ_RT_API void kz_rt_plan_note(uint32_t status) {
  if (status != KZ_RT_PLANST_NONE && status != KZ_RT_PLANST_IGNORED && status != KZ_RT_PLANST_READ_ERROR) return;
  if (__atomic_exchange_n(&g_once, 1u, __ATOMIC_ACQ_REL) != 0) return;
  g_kept = g_given = 0;
  settle(status, 0);
}

KZ_RT_API int kz_rt_plan_load(const void *bytes, size_t n) {
  const unsigned char *b = (const unsigned char *)bytes;
  kz_rt_point pts[KZ_RT_MAX_POINTS];
  unsigned char sha[32];
  uint32_t i, np, flags = 0;
  int bs, r;
  if (bytes == NULL && n != 0) return KZ_RT_EINVAL;
  if (__atomic_exchange_n(&g_once, 1u, __ATOMIC_ACQ_REL) != 0) return KZ_RT_ESTATE;
  g_given = n > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)n;
  g_kept = n > KZ_RT_PLAN_MAX_BYTES ? KZ_RT_PLAN_MAX_BYTES : (uint32_t)n;
  for (i = 0; i < g_kept; ++i) g_bytes[i] = b[i];
  if (n > KZ_RT_PLAN_MAX_BYTES) flags |= KZ_RT_PLANF_TRUNCATED;
  if (n < KZ_RT_PLAN_HEADER_BYTES + KZ_RT_PLAN_TRAILER_BYTES || n > KZ_RT_PLAN_MAX_BYTES) {
    settle(KZ_RT_PLANST_SIZE, flags);
    return KZ_RT_EINVAL;
  }
  b = g_bytes;
  for (i = 0; i < 8u; ++i)
    if (b[i] != (unsigned char)KZ_RT_PLAN_MAGIC[i]) break;
  np = g32(b + 12);
  if (i != 8u || g16(b + 8) != KZ_RT_PLAN_MAJOR || g32(b + 16) != (uint32_t)n || np > KZ_RT_MAX_POINTS ||
      (uint32_t)n != KZ_RT_PLAN_HEADER_BYTES + np * KZ_RT_PLAN_POINT_BYTES + KZ_RT_PLAN_TRAILER_BYTES) {
    settle(KZ_RT_PLANST_FORMAT, flags);
    return KZ_RT_EINVAL;
  }
  if (crc32_of(b, (uint32_t)n - KZ_RT_PLAN_TRAILER_BYTES) != g32(b + n - KZ_RT_PLAN_TRAILER_BYTES)) {
    settle(KZ_RT_PLANST_CRC, flags);
    return KZ_RT_EINVAL;
  }
  bs = build_sha(sha);
  if (bs == 1) {
    flags |= KZ_RT_PLANF_CHECKED;
    for (i = 0; i < 32u; ++i)
      if (sha[i] != b[24u + i]) {
        settle(KZ_RT_PLANST_MANIFEST, flags);
        return KZ_RT_EINVAL;
      }
  } else if (bs == 0) {
    flags |= KZ_RT_PLANF_NO_BUILD;
  } else {
    settle(KZ_RT_PLANST_MANIFEST, flags); /* a malformed build identity cannot vouch for the plan */
    return KZ_RT_EINVAL;
  }
  for (i = 0; i < np; ++i) {
    const unsigned char *q = b + KZ_RT_PLAN_HEADER_BYTES + i * KZ_RT_PLAN_POINT_BYTES;
    kz_rt_point *p = &pts[i];
    uint32_t j;
    p->kind = g32(q);
    p->label = g32(q + 4);
    p->site = g32(q + 8);
    p->releases = g32(q + 12);
    p->occurrence = g32(q + 16);
    p->flags = g32(q + 20);
    p->bind_n = g32(q + 24);
    for (j = 0; j < KZ_RT_POINT_BINDS; ++j) p->bind[j] = g32(q + 28 + 4u * j);
    p->str_slot = (int32_t)g32(q + 60);
    p->str_len = g32(q + 64);
    for (j = 0; j < KZ_RT_STR_MAX; ++j) p->str[j] = q[68 + j];
    p->val_slot = (int32_t)g32(q + 132);
    p->rsv = 0;
    p->val = g64(q + 140);
    p->timeout_ns = g64(q + 148);
  }
  r = kz_rt_plan(pts, np);
  if (r != KZ_RT_OK) {
    settle(r == KZ_RT_ESTATE ? KZ_RT_PLANST_STATE : KZ_RT_PLANST_POINTS, flags);
    return r;
  }
  settle(KZ_RT_PLANST_LOADED, flags);
  return KZ_RT_OK;
}

/* For the dump (kz_rt.c): the PLAN record's content; 0 = nothing to record. */
__attribute__((visibility("hidden"))) uint32_t kz_rt__plan_record(const unsigned char **bytes, uint32_t *kept, uint32_t *given, uint32_t *flags);
__attribute__((visibility("hidden"))) uint32_t kz_rt__plan_record(const unsigned char **bytes, uint32_t *kept, uint32_t *given, uint32_t *flags) {
  if (__atomic_load_n(&g_set, __ATOMIC_ACQUIRE) == 0) return 0;
  *bytes = g_bytes;
  *kept = g_kept;
  *given = g_given;
  *flags = g_flags;
  return g_status;
}

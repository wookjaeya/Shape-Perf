/*
 * schema_probe.c -- a deliberately broken or correct record built with KZ_RECORD, used by the
 * compile checks in CMakeLists.txt to show that the layout assertions really fire:
 *   KZ_PROBE_CASE=0  a record without padding: must compile
 *   KZ_PROBE_CASE=1  padding between fields (uint8_t then uint32_t): must be rejected
 *   KZ_PROBE_CASE=2  padding at the end (uint64_t then uint32_t): must be rejected
 * Compiled as C99 with the C compiler and as C++ with the C++ compiler.
 */
#include "kz/schema/kz_schema.h"

#if KZ_PROBE_CASE == 0
#define KZ_PROBE_FIELDS(X, T) X(T, uint32_t, a, ) X(T, uint32_t, b, ) X(T, uint8_t, c, [8])
#elif KZ_PROBE_CASE == 1
#define KZ_PROBE_FIELDS(X, T) X(T, uint8_t, a, ) X(T, uint32_t, b, )
#elif KZ_PROBE_CASE == 2
#define KZ_PROBE_FIELDS(X, T) X(T, uint64_t, a, ) X(T, uint32_t, b, )
#else
#error "KZ_PROBE_CASE must be 0, 1 or 2"
#endif

KZ_RECORD(kz_probe_record, KZ_PROBE_FIELDS)

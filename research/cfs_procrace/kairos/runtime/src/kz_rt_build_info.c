/* kz_rt_build_info.c -- the build identity of an instrumented build (kz_rt.h "build identity",
 * kairos/docs/event_semantics.md 8.5).  A fixed source: the build gives the values as -D names when
 * it compiles this file with the target compiler (CMake: the OBJECT library kzrt-build-info, from the
 * cache variables KZ_RT_BUILD_BACKEND, KZ_RT_BUILD_COMPILER, KZ_RT_BUILD_TARGET, KZ_RT_BUILD_MANIFEST),
 * and links the object into the executable:
 *   KZ_RT_BUILD_BACKEND          string, required: the compiler binding, e.g. "kzgcc 0.1 (gcc-plugin)"
 *   KZ_RT_BUILD_COMPILER         string; unset = "__VERSION__ " + the compiler's __VERSION__ (the compiler
 *                                that compiles this file, which the build makes the target compiler)
 *   KZ_RT_BUILD_TARGET           string; unset = no target recorded ("")
 *   KZ_RT_BUILD_MANIFEST_SHA256  string of 64 hex digits: the sha256 of the bytes of the site manifest
 *                                file the binding read (the digest it passes to kz_rt_module); unset = none
 * The values are recorded as given; the build records how it obtained them.  Not part of libkzrt.a. */
#include "kz_rt.h"

#ifndef KZ_RT_BUILD_BACKEND
#error "KZ_RT_BUILD_BACKEND (the compiler binding's identity) is required"
#endif
#ifndef KZ_RT_BUILD_COMPILER
#define KZ_RT_BUILD_COMPILER "__VERSION__ " __VERSION__
#define KZ_RT_BUILD_COMPILER_ORIGIN "compiler=__VERSION__"
#else
#define KZ_RT_BUILD_COMPILER_ORIGIN "compiler=-D"
#endif
#ifndef KZ_RT_BUILD_TARGET
#define KZ_RT_BUILD_TARGET ""
#endif
#ifdef KZ_RT_BUILD_MANIFEST_SHA256
/* 64 digits and the terminating NUL (the digits themselves are checked by the runtime: RUNX flag) */
typedef char kz_rt_build_sha_length_check[sizeof(KZ_RT_BUILD_MANIFEST_SHA256) == 65u ? 1 : -1];
#define KZ_RT_BUILD_MANIFEST_VALUE KZ_RT_BUILD_MANIFEST_SHA256
#else
#define KZ_RT_BUILD_MANIFEST_VALUE NULL
#endif

KZ_RT_API extern const kz_rt_build kz_rt_build_info;
KZ_RT_API const kz_rt_build kz_rt_build_info = {KZ_RT_ABI_VERSION, KZ_RT_BUILD_BACKEND, KZ_RT_BUILD_COMPILER, KZ_RT_BUILD_TARGET,
                                                KZ_RT_BUILD_MANIFEST_VALUE, "kz_rt_build_info.c (-D values; " KZ_RT_BUILD_COMPILER_ORIGIN ")"};

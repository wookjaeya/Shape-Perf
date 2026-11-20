/* rt_objects_module.c -- a loaded module for test_rt_objects (kzrt 0.5.0, event_semantics.md 13.5): its constructor
 * registers it as a compiler binding does (kzgcc.cpp build_module_ctor: kz_rt_module(module, a 32-byte constant of this
 * unit)), so that the runtime finds this object from the registration's argument address.  RT_OBJECTS_MODULE is the
 * module number.  Unit-test values. */
#include "kz_rt.h"

static const unsigned char kz_test_sha[32] = {0x11, 0x22, 0x33, 0x44};

__attribute__((constructor)) static void rt_objects_module_register(void) { kz_rt_module(RT_OBJECTS_MODULE, kz_test_sha); }

int rt_objects_module_value(void);
int rt_objects_module_value(void) { return RT_OBJECTS_MODULE; }

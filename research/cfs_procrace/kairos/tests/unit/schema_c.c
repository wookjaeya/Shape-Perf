/*
 * schema_c.c -- compiled twice, as C99 (-std=c99 -pedantic -Wall -Wextra -Werror) and as C11
 * (see tests/unit/CMakeLists.txt), to show that kz_schema.h is plain C and that the record
 * layouts seen from C are the layouts seen from C++.  Exports flat layout tables.
 */
#include "kz/schema/kz_schema.h"
#include "schema_c.h"

#define KZ_C_SYM(name) KZ_PASTE(KZ_C_PFX, name)

#define KZ_C_FIELD(T, type, name, arr) \
  {#T, #name, (unsigned)offsetof(T, name), (unsigned)sizeof(type), (unsigned)(sizeof(((T *)0)->name) / sizeof(type))},
#define KZ_C_FIELDS(T, LIST) LIST(KZ_C_FIELD, T)
const struct kz_c_field KZ_C_SYM(fields)[] = {KZ_ALL_RECORDS(KZ_C_FIELDS)};
const unsigned KZ_C_SYM(nfields) = (unsigned)(sizeof(KZ_C_SYM(fields)) / sizeof(KZ_C_SYM(fields)[0]));

#define KZ_C_RECORD(T, LIST) {#T, (unsigned)sizeof(T)},
const struct kz_c_record KZ_C_SYM(records)[] = {KZ_ALL_RECORDS(KZ_C_RECORD)};
const unsigned KZ_C_SYM(nrecords) = (unsigned)(sizeof(KZ_C_SYM(records)) / sizeof(KZ_C_SYM(records)[0]));

const long KZ_C_SYM(language) = (long)__STDC_VERSION__;

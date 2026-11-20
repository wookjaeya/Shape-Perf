/* schema_c.h -- what the C translation units of the schema test export. */
#ifndef KZ_TEST_SCHEMA_C_H
#define KZ_TEST_SCHEMA_C_H

#ifdef __cplusplus
extern "C" {
#endif

struct kz_c_field {
  const char *record;
  const char *field;
  unsigned offset;
  unsigned size;
  unsigned count;
};

struct kz_c_record {
  const char *name;
  unsigned size;
};

#define KZ_C_DECLS(pfx)                          \
  extern const struct kz_c_field pfx##fields[];  \
  extern const unsigned pfx##nfields;            \
  extern const struct kz_c_record pfx##records[]; \
  extern const unsigned pfx##nrecords;           \
  extern const long pfx##language;

KZ_C_DECLS(kz_c99_)
KZ_C_DECLS(kz_c11_)

#ifdef __cplusplus
}
#endif

#endif /* KZ_TEST_SCHEMA_C_H */

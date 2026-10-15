/* atype.c -- accesses that bypass the typed view of a field.  Each scenario has its own type,
 * so each location gets its own verdict.
 * Targets (--target): Plain.id Cast.id Esc.id Mix.id Inner.id Safe.id */
typedef __SIZE_TYPE__ size_t;
void *memset(void *dest, int fill, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

typedef struct { int id; } Plain;
typedef struct { int id; } Cast;
typedef struct { int id; } Esc;
typedef union { int id; unsigned raw; } Mix;
typedef struct { int id; } Inner;
typedef union { Inner in; unsigned raw; } Over;
typedef struct { int id; } Safe;

Plain g_plain;
Cast g_cast;
Esc g_esc;
Mix g_mix;
Over g_over;
Safe g_safe;
char g_buffer[8];

/* a store through an untyped pointer that never sees a Plain: it does not count */
void fill_buffer(void) {
  char *q = g_buffer;
  q[0] = 1;
}

void plain_write(void) { g_plain.id = 1; }

/* a view of Cast as bytes, then a store through it: disabling */
void poke_cast(void) {
  char *p = (char *)&g_cast;
  p[0] = 1;
}

/* the address of the field leaves the call-argument form: disabling */
void leak_address(void) {
  int *q = &g_esc.id;
  *q = 3;
}

/* the other member of the union is written: disabling */
void write_other_member(void) { g_mix.raw = 9u; }

/* a member of a union that holds the type is written through another member: disabling */
void write_raw(void) { g_over.raw = 1u; }

/* memset and memcpy are typed writers, not escapes */
void data_writers(const Safe *src) {
  memset(&g_safe, 0, sizeof g_safe);
  memcpy(&g_safe, src, sizeof g_safe);
}

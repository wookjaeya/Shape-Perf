/* atype.c -- accesses that bypass the typed view of a field.  Each scenario has its own type,
 * so each location gets its own verdict.
 * Targets (--target): Plain.id Cast.id Esc.id Mix.id Inner.id Safe.id, and for the second half
 * Ext.id Ind.id Var.id Asm.id Span.id Sub.id Outp.id Laund.id */
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

/* ---- code whose effect is unknown, extents, record-typed targets (SPEC 4.2 steps 2-3) ---------- */
typedef struct { int pre; int id; } Ext;  /* reached through a pointer field by a function without a body */
typedef struct { Ext *ext; } Holder;
typedef struct { int id; } Ind;           /* passed to an indirect call that has no known target */
typedef struct { int id; } Var;           /* passed in a variadic position */
typedef struct { int id; } Asm;           /* an operand of inline assembly */
typedef struct { int pre; int id; } Span; /* memset of an earlier field, sized for the whole record */
typedef struct { unsigned v; } Wrap;
typedef struct { Wrap id; } Sub;          /* a record-typed target: a store to a member of it */
typedef struct { Wrap id; } Outp;         /* a record-typed target stored through an out-pointer */
typedef struct { int id; } Laund;         /* a const view, then the const is cast away */

Ext g_ext;
Ind g_ind;
Var g_var;
Asm g_asm;
Span g_span;
Sub g_sub;
Outp g_outp;
Laund g_laund;

void ext_fill(Holder *h); /* no body in the closed program */
void (*g_hook)(Ind *p);   /* never assigned: no function flows to it */
void log_values(int n, ...) { (void)n; }

/* the pointer to the holder reaches Ext through the field `ext`: disabling */
void t_ext(void) {
  Holder h;
  h.ext = &g_ext;
  ext_fill(&h);
}

/* an indirect call whose targets are not known: disabling */
void t_ind(void) { g_hook(&g_ind); }

/* the callee has a body, but a variadic argument is read by va_arg, which is not followed: disabling */
void t_var(void) { log_values(1, &g_var); }

/* an operand of inline assembly: disabling */
void t_asm(void) { __asm__ __volatile__("" : : "r"(&g_asm)); }

/* the size covers the whole record, not only the field named: a writer of Span.id */
void t_span(void) { memset(&g_span.pre, 0, sizeof g_span); }

/* a store to a member of a record-typed target is a (partial) writer of it */
void t_sub(void) { g_sub.id.v = 7u; }

/* stores through `(*p).v` and `p->v` are the out-pointer summary of the parameter */
static void set_wrap(Wrap *p) {
  (*p).v = 3u;
  p->v += 1u;
}
void t_out(void) { set_wrap(&g_outp.id); }

/* a const view of the record is not exempt: the const is cast away later, and the store
 * through `char *` no longer mentions the type: disabling */
void t_laund(void) {
  const void *v = &g_laund;
  char *p = (char *)v;
  p[0] = 1;
}

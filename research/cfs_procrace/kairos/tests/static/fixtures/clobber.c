/* clobber.c -- writes that may reach any object: every target is DISABLED.
 * Targets (--target): Any.id Far.id
 * Not compiled by the fixture compile check: `overrun` writes past the end of an object on
 * purpose, which both compilers reject at -Werror; kz-static reads it without -Werror. */
typedef __SIZE_TYPE__ size_t;
void *memset(void *dest, int fill, size_t n);

typedef struct { int id; } Any;
typedef struct { int pre; int id; } Far;

Any g_any;
Far g_far;

void plain(void) { g_any.id = 1; }

/* inline assembly that clobbers memory may store to any object, by name or by address */
void barrier(void) { __asm__ __volatile__("" : : : "memory"); }

/* a constant size that goes past the end of the enclosing object (4 + 8 > 8) */
void overrun(void) { memset(&g_far.id, 0, sizeof g_far); }

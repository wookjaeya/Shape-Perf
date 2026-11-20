/* tcase.c -- the input program of the kz-instr-clang test (a small C99 program, not a cFS file).
 * It has a call site whose arguments have side effects, a call site inside a macro body, a variadic
 * callee, return paths inside macro expansions, a store and a skipped preprocessor region. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int32_t i32;
#define CHECK_PTR(p) \
    if ((p) == 0) return -2
#define API_ERR (-17)
#define LOOKUP_MACRO(o) api_lookup(o, "macro")

static uint32_t g_ids[4];
static const char g_names[3][8] = {"alpha", "beta", "gamma"};
static int g_side;

i32 api_lookup(uint32_t *out, const char *name);
int note_event(int id, unsigned app, const char *fmt, ...);

i32 api_lookup(uint32_t *out, const char *name)
{
    CHECK_PTR(out);
    CHECK_PTR(name);
    if (strcmp(name, "beta") == 0)
    {
        *out = 7u + (uint32_t)g_side;
        return 0;
    }
#if 0
    return api_lookup(out, name);
#endif
    return API_ERR;
}

int note_event(int id, unsigned app, const char *fmt, ...)
{
    va_list ap;
    int     n;
    va_start(ap, fmt);
    n = vprintf(fmt, ap);
    va_end(ap);
    return n + id * 0 + (int)(app * 0u);
}

static int next_id(void)
{
    return ++g_side;
}

int main(void)
{
    int i = 0, j = 1, k = 0;
    i32 r1, r2, r3;
    r1 = api_lookup(&g_ids[i++], g_names[j++]);
    r2 = LOOKUP_MACRO(&g_ids[3]);
    note_event(next_id(), (unsigned)g_ids[0], "line %d\n", k++);
    r3 = api_lookup(0, "x");
    if (i != 1 || j != 2 || k != 1 || g_side != 1)
    {
        printf("side effects differ: i=%d j=%d k=%d side=%d\n", i, j, k, g_side);
        return 1;
    }
    if (r1 != 0 || g_ids[0] != 7u || r2 != API_ERR || r3 != -2)
    {
        printf("results differ: %d %u %d %d\n", (int)r1, (unsigned)g_ids[0], (int)r2, (int)r3);
        return 1;
    }
    printf("end at line %d\n", __LINE__);
    return 0;
}

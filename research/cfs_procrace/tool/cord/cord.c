/*
 * CORD event recorder.
 *
 * - One buffer per thread, allocated on first use and registered in a fixed
 *   table with an atomic index (no lock).
 * - A global atomic counter gives each event a sequence number, so the
 *   analyzer sees the observed total order.
 * - Output: JSON lines in $CORD_OUT (default cord_trace.jsonl), written at
 *   exit, on SIGSEGV/SIGBUS/SIGABRT/SIGTERM/SIGINT, and optionally once after
 *   $CORD_DUMP_AFTER_MS milliseconds by a helper thread.
 * - Witness hook: $CORD_DELAY="<pattern>@<usec>[;...]". <pattern> is a
 *   resource key or "file:line" (file basename). Each matching
 *   cord_maybe_delay() call sleeps for <usec> microseconds.
 *
 * The recorder calls only libc and the kernel, never OSAL or cFE.
 */
#define _GNU_SOURCE
#include "cord.h"

#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define CORD_MAX_THREADS 512
#define CORD_KEYLEN 64
#define CORD_MAX_DELAYS 16

struct cord_ev {
    uint64_t seq;
    uint64_t t_ns;
    int kind;
    int line;
    long a, b;
    const char *file;
    const char *func;
    char key[CORD_KEYLEN];
};

struct cord_buf {
    int tid;
    unsigned long pthread;
    size_t cap;
    _Atomic size_t n;
    _Atomic size_t dropped;
    struct cord_ev *ev;
};

static struct cord_buf *g_bufs[CORD_MAX_THREADS];
static _Atomic int g_nbufs;
static _Atomic int g_overflow_threads;
static _Atomic uint64_t g_seq;
static _Atomic long g_token;
static _Atomic int g_dumped;
static __thread struct cord_buf *t_buf;

struct cord_delay {
    char pat[128];
    long usec;
};
static struct cord_delay g_delays[CORD_MAX_DELAYS];
static int g_ndelays;

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static struct cord_buf *get_buf(void)
{
    if (t_buf)
        return t_buf;
    size_t cap = 1u << 16;
    const char *s = getenv("CORD_BUF");
    if (s && atol(s) > 0)
        cap = (size_t)atol(s);
    struct cord_buf *b = calloc(1, sizeof(*b));
    if (!b)
        return NULL;
    b->ev = calloc(cap, sizeof(struct cord_ev));
    if (!b->ev) {
        free(b);
        return NULL;
    }
    b->cap = cap;
    b->tid = (int)syscall(SYS_gettid);
    b->pthread = (unsigned long)pthread_self();
    int i = atomic_fetch_add(&g_nbufs, 1);
    if (i >= CORD_MAX_THREADS) {
        atomic_fetch_add(&g_overflow_threads, 1);
        free(b->ev);
        free(b);
        return NULL;
    }
    g_bufs[i] = b;
    t_buf = b;
    return b;
}

void cord_event(int kind, const char *key, long a, long b,
                const char *file, int line, const char *func)
{
    struct cord_buf *buf = get_buf();
    if (!buf)
        return;
    size_t i = atomic_load_explicit(&buf->n, memory_order_relaxed);
    if (i >= buf->cap) {
        atomic_fetch_add(&buf->dropped, 1);
        return;
    }
    struct cord_ev *e = &buf->ev[i];
    e->seq = atomic_fetch_add(&g_seq, 1);
    e->t_ns = now_ns();
    e->kind = kind;
    e->a = a;
    e->b = b;
    e->file = file;
    e->line = line;
    e->func = func;
    if (key) {
        strncpy(e->key, key, CORD_KEYLEN - 1);
        e->key[CORD_KEYLEN - 1] = 0;
    } else {
        e->key[0] = 0;
    }
    atomic_store_explicit(&buf->n, i + 1, memory_order_release);
}

long cord_new_token(void)
{
    return atomic_fetch_add(&g_token, 1) + 1;
}

static const char *base(const char *p)
{
    const char *s = p ? strrchr(p, '/') : NULL;
    return s ? s + 1 : (p ? p : "");
}

void cord_maybe_delay(const char *key, const char *file, int line)
{
    if (g_ndelays == 0)
        return;
    char site[160];
    snprintf(site, sizeof site, "%s:%d", base(file), line);
    for (int i = 0; i < g_ndelays; i++) {
        if ((key && strcmp(g_delays[i].pat, key) == 0) || strcmp(g_delays[i].pat, site) == 0) {
            cord_event(CORD_K_NOTE, "cord.delay", g_delays[i].usec, 0, file, line, "cord_maybe_delay");
            usleep((useconds_t)g_delays[i].usec);
            return;
        }
    }
}

static void json_str(FILE *f, const char *s)
{
    fputc('"', f);
    for (; s && *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\')
            fprintf(f, "\\%c", c);
        else if (c < 0x20)
            fprintf(f, "\\u%04x", c);
        else
            fputc(c, f);
    }
    fputc('"', f);
}

static void dump(const char *suffix)
{
    const char *out = getenv("CORD_OUT");
    char path[512];
    snprintf(path, sizeof path, "%s%s", out ? out : "cord_trace.jsonl", suffix);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    int nb = atomic_load(&g_nbufs);
    if (nb > CORD_MAX_THREADS)
        nb = CORD_MAX_THREADS;
    fprintf(f, "{\"meta\":1,\"threads\":%d,\"overflow_threads\":%d,\"seq\":%llu}\n", nb,
            atomic_load(&g_overflow_threads), (unsigned long long)atomic_load(&g_seq));
    for (int i = 0; i < nb; i++) {
        struct cord_buf *b = g_bufs[i];
        if (!b)
            continue;
        size_t n = atomic_load_explicit(&b->n, memory_order_acquire);
        if (atomic_load(&b->dropped))
            fprintf(f, "{\"meta\":2,\"tid\":%d,\"dropped\":%zu}\n", b->tid, atomic_load(&b->dropped));
        for (size_t j = 0; j < n; j++) {
            struct cord_ev *e = &b->ev[j];
            fprintf(f, "{\"seq\":%llu,\"t\":%llu,\"tid\":%d,\"k\":%d,\"key\":",
                    (unsigned long long)e->seq, (unsigned long long)e->t_ns, b->tid, e->kind);
            json_str(f, e->key);
            fprintf(f, ",\"a\":%ld,\"b\":%ld,\"file\":", e->a, e->b);
            json_str(f, base(e->file));
            fprintf(f, ",\"line\":%d,\"func\":", e->line);
            json_str(f, e->func);
            fputs("}\n", f);
        }
    }
    fclose(f);
}

void cord_flush(void)
{
    if (atomic_exchange(&g_dumped, 1))
        return;
    dump("");
}

static void on_signal(int sig)
{
    cord_event(CORD_K_NOTE, "cord.signal", sig, 0, __FILE__, __LINE__, __func__);
    cord_flush();
    signal(sig, SIG_DFL);
    raise(sig);
}

static void *dumper(void *arg)
{
    long ms = (long)arg;
    usleep((useconds_t)(ms * 1000));
    cord_event(CORD_K_NOTE, "cord.timed_dump", ms, 0, __FILE__, __LINE__, __func__);
    cord_flush();
    return NULL;
}

static void parse_delays(void)
{
    const char *s = getenv("CORD_DELAY");
    if (!s || !*s)
        return;
    char tmp[1024];
    strncpy(tmp, s, sizeof tmp - 1);
    tmp[sizeof tmp - 1] = 0;
    char *save = NULL;
    for (char *tok = strtok_r(tmp, ";", &save); tok && g_ndelays < CORD_MAX_DELAYS;
         tok = strtok_r(NULL, ";", &save)) {
        char *at = strrchr(tok, '@');
        if (!at)
            continue;
        *at = 0;
        strncpy(g_delays[g_ndelays].pat, tok, sizeof g_delays[0].pat - 1);
        g_delays[g_ndelays].usec = atol(at + 1);
        g_ndelays++;
    }
}

__attribute__((constructor)) static void cord_init(void)
{
    parse_delays();
    atexit(cord_flush);
    int sigs[] = {SIGSEGV, SIGBUS, SIGABRT, SIGTERM, SIGINT, SIGFPE};
    for (size_t i = 0; i < sizeof sigs / sizeof sigs[0]; i++)
        signal(sigs[i], on_signal);
    const char *d = getenv("CORD_DUMP_AFTER_MS");
    if (d && atol(d) > 0) {
        pthread_t th;
        pthread_attr_t at;
        pthread_attr_init(&at);
        pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
        pthread_create(&th, &at, dumper, (void *)atol(d));
        pthread_attr_destroy(&at);
    }
}

/*
 * CORD event recorder: public interface.
 *
 * Events go to per-thread buffers. The buffers are written as JSON lines to
 * $CORD_OUT at process exit or on a fatal signal. The recorder never calls
 * into OSAL or cFE, so it works before any framework service is ready.
 *
 * Build the instrumented code with -DCORD_ENABLE. Without it every macro
 * expands to nothing.
 */
#ifndef CORD_H
#define CORD_H

#ifdef __cplusplus
extern "C" {
#endif

enum cord_kind {
    CORD_K_PUB = 1,      /* resource becomes usable (recorded after the commit) */
    CORD_K_USE,          /* a task depends on the resource; a = status */
    CORD_K_TCREATE,      /* parent, before the create call; a = token */
    CORD_K_TSTART,       /* child, at its entry; a = token */
    CORD_K_SIG,          /* post/give/put on a sync object */
    CORD_K_WAIT,         /* successful take/get on a sync object */
    CORD_K_SETSTATE,     /* write of a designated state variable; a = value */
    CORD_K_WAITRET,      /* end of a wait on a state variable; a = value read */
    CORD_K_LOCK,
    CORD_K_UNLOCK,
    CORD_K_NOTE,         /* free-form marker (outcome, status) */
    CORD_K_TASKNAME      /* names the current task; key = name */
};

void cord_event(int kind, const char *key, long a, long b,
                const char *file, int line, const char *func);

/* Witness hook: sleeps if $CORD_DELAY names this key or site. */
void cord_maybe_delay(const char *key, const char *file, int line);

/* Unique token for a task creation (TCREATE/TSTART pairing). */
long cord_new_token(void);

void cord_flush(void);

#ifdef __cplusplus
}
#endif

#ifdef CORD_ENABLE
#define CORD_EV(k, key, a, b) cord_event((k), (key), (long)(a), (long)(b), __FILE__, __LINE__, __func__)
#define CORD_PUB_PRE(key)     cord_maybe_delay((key), __FILE__, __LINE__)
#define CORD_PUB(key)         CORD_EV(CORD_K_PUB, (key), 0, 0)
#define CORD_USE(key, status) CORD_EV(CORD_K_USE, (key), (status), 0)
#define CORD_SIG(key)         CORD_EV(CORD_K_SIG, (key), 0, 0)
#define CORD_WAIT(key)        CORD_EV(CORD_K_WAIT, (key), 0, 0)
#define CORD_SETSTATE(key, v) CORD_EV(CORD_K_SETSTATE, (key), (v), 0)
#define CORD_WAITRET(key, v)  CORD_EV(CORD_K_WAITRET, (key), (v), 0)
#define CORD_LOCK(key)        CORD_EV(CORD_K_LOCK, (key), 0, 0)
#define CORD_UNLOCK(key)      CORD_EV(CORD_K_UNLOCK, (key), 0, 0)
#define CORD_NOTE(key, a)     CORD_EV(CORD_K_NOTE, (key), (a), 0)
#define CORD_TASKNAME(name)   CORD_EV(CORD_K_TASKNAME, (name), 0, 0)
#else
#define CORD_EV(k, key, a, b) ((void)0)
#define CORD_PUB_PRE(key)     ((void)0)
#define CORD_PUB(key)         ((void)0)
#define CORD_USE(key, status) ((void)0)
#define CORD_SIG(key)         ((void)0)
#define CORD_WAIT(key)        ((void)0)
#define CORD_SETSTATE(key, v) ((void)0)
#define CORD_WAITRET(key, v)  ((void)0)
#define CORD_LOCK(key)        ((void)0)
#define CORD_UNLOCK(key)      ((void)0)
#define CORD_NOTE(key, a)     ((void)0)
#define CORD_TASKNAME(name)   ((void)0)
#endif

#endif /* CORD_H */

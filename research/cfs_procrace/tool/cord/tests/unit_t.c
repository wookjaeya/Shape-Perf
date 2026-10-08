#include "cord.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
static pthread_mutex_t tbl = PTHREAD_MUTEX_INITIALIZER;
static int registered;           /* the "resource" */
static sem_t go;
static void *provider(void *x) {  /* like BP: creates the object */
    CORD_EV(CORD_K_TSTART, "", (long)x, 0); CORD_TASKNAME("PROVIDER");
    usleep(1000);
    CORD_PUB_PRE("obj:THR");
    pthread_mutex_lock(&tbl); registered = 1; CORD_PUB("obj:THR"); pthread_mutex_unlock(&tbl);
    return 0;
}
static void *user(void *x) {      /* like CF: looks it up */
    CORD_EV(CORD_K_TSTART, "", (long)x, 0); CORD_TASKNAME("USER");
    usleep(5000);
    pthread_mutex_lock(&tbl); int ok = registered; CORD_USE("obj:THR", ok ? 0 : -1); pthread_mutex_unlock(&tbl);
    printf("user lookup %s\n", ok ? "OK" : "NOT_FOUND");
    /* a properly synchronized resource for contrast */
    sem_wait(&go); CORD_WAIT("sem:go");
    CORD_USE("obj:SYNCED", 0);
    return 0;
}
int main(void) {
    CORD_TASKNAME("MAIN"); sem_init(&go, 0, 0);
    pthread_t a, b; long t1 = cord_new_token(), t2 = cord_new_token();
    CORD_EV(CORD_K_TCREATE, "", t1, 0); pthread_create(&a, 0, provider, (void*)t1);
    CORD_EV(CORD_K_TCREATE, "", t2, 0); pthread_create(&b, 0, user, (void*)t2);
    usleep(2000); CORD_PUB("obj:SYNCED"); CORD_SIG("sem:go"); sem_post(&go);
    pthread_join(a, 0); pthread_join(b, 0); return 0;
}

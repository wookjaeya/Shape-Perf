/* Tool microbenchmark only (not a research condition): cost of cord_event and memory of per-thread buffers. */
#define CORD_ENABLE
#include "../cord.c"
#include <stdio.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e9+t.tv_nsec;}
static void vm(const char*l){FILE*f=fopen("/proc/self/status","r");char b[256];while(fgets(b,sizeof b,f))if(!strncmp(b,"VmSize",6)||!strncmp(b,"VmRSS",5))printf("  %s %s",l,b);fclose(f);}
#define N 60000
static void *work(void*a){(void)a;for(int i=0;i<N;i++)CORD_USE("sb.appid",i);return 0;}
static void *few(void*a){(void)a;for(int i=0;i<30;i++)CORD_USE("sb.appid",i);return 0;}
int main(void){
  printf("sizeof(struct cord_ev)=%zu bytes; per-thread buffer=%zu events -> %.1f MB reserved per recording thread\n",
         sizeof(struct cord_ev),(size_t)1<<16,(double)(sizeof(struct cord_ev)<<16)/1048576);
  vm("start");
  pthread_t th[32];
  for(int i=0;i<25;i++)pthread_create(&th[i],0,few,0);
  for(int i=0;i<25;i++)pthread_join(th[i],0);
  vm("after 25 threads x 30 events");
  double t0=now(); work(0); double t1=now();
  printf("1 thread : %.0f ns/event\n",(t1-t0)/N);
  for(int k=2;k<=4;k+=2){t0=now();for(int i=0;i<k;i++)pthread_create(&th[i],0,work,0);for(int i=0;i<k;i++)pthread_join(th[i],0);t1=now();
    printf("%d threads concurrently: %.0f ns/event (wall per event per thread)\n",k,(t1-t0)/N);}
  vm("end");
  return 0;}

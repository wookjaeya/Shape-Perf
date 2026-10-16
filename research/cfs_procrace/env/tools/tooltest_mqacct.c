/* tool unit test only: find the smallest RLIMIT_MSGQUEUE (soft) under which mq_open(maxmsg,msgsize) succeeds,
   i.e. the bytes the kernel charges for one queue.  Lowers the limit only in this test process. */
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
static int try(long lim,long maxmsg,long msgsize){ struct rlimit r; getrlimit(RLIMIT_MSGQUEUE,&r); r.rlim_cur=lim; if(setrlimit(RLIMIT_MSGQUEUE,&r)){perror("setrlimit");exit(1);}
 char name[64]; snprintf(name,sizeof name,"/%d.ACCT",(int)getpid()); struct mq_attr a={0}; a.mq_maxmsg=maxmsg; a.mq_msgsize=msgsize;
 mqd_t q=mq_open(name,O_CREAT|O_EXCL|O_RDWR,0600,&a); int ok=(q!=(mqd_t)-1); int e=errno; if(ok){mq_close(q);} mq_unlink(name); if(!ok && e!=EMFILE){fprintf(stderr,"unexpected errno %d\n",e);exit(1);} return ok; }
int main(int argc,char**argv){ long maxmsg=atol(argv[1]), msgsize=atol(argv[2]); long lo=0, hi=819200; /* lo fails, hi succeeds */
 if(!try(hi,maxmsg,msgsize)){printf("fails even at %ld\n",hi);return 1;}
 while(hi-lo>1){ long mid=(lo+hi)/2; if(try(mid,maxmsg,msgsize)) hi=mid; else lo=mid; }
 printf("maxmsg=%ld msgsize=%ld charged_bytes=%ld per_msg_overhead=%ld\n",maxmsg,msgsize,hi,(hi-maxmsg*msgsize)/maxmsg); return 0; }

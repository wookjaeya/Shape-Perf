/* tool unit test only (not a research condition): create an OSAL-style unlinked queue, put 3 messages, wait for stdin, then count what is left */
#include <mqueue.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
int main(void){ char name[64]; struct mq_attr a={0}; a.mq_maxmsg=10; a.mq_msgsize=8;
 snprintf(name,sizeof name,"/%d.TESTQ",(int)getpid());
 mqd_t q=mq_open(name,O_CREAT|O_RDWR,0666,&a); if(q==(mqd_t)-1){perror("mq_open");return 1;}
 mq_unlink(name); char m[8]={0}; for(int i=0;i<3;i++) mq_send(q,m,8,0);
 printf("READY %d\n",(int)getpid()); fflush(stdout); getchar();
 struct mq_attr b; mq_getattr(q,&b); printf("after probe: curmsgs=%ld\n",(long)b.mq_curmsgs); return 0; }

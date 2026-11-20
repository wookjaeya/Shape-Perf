/*
 * mq_probe.c -- read the kernel attributes of the POSIX message queues held open by a process,
 *               without consuming or sending any message.  (CONDITIONS.md M1 rows; BASELINE_MEASURE.md §2)
 *
 * Why this is needed: OSAL posix creates every queue as "/<pid>.<queue_name>" and mq_unlink()s it right
 * after mq_open() (osal/src/os/posix/src/os-impl-queues.c L111-116, L142), so the queue cannot be opened by
 * name.  The descriptor is still reachable through /proc/<pid>/fd/<n>; open(2) on that magic link yields a new
 * open file description of the same mqueue inode, and mq_getattr(3) (syscall mq_getsetattr with a NULL new
 * attribute) reads mq_maxmsg, mq_msgsize and mq_curmsgs.  Nothing else is done with the descriptor: no
 * mq_receive, no mq_send, no mq_notify, and the new description is closed right away.  The new description is
 * opened O_RDONLY|O_NONBLOCK, so even an accidental read could not block.
 *
 * Only descriptors whose link target looks like an OSAL queue name ("/<digits>.<name>", optionally followed by
 * " (deleted)") are opened, and each is confirmed to be on the mqueue filesystem (fstatfs f_type ==
 * MQUEUE_MAGIC 0x19800202, linux/magic.h) before mq_getattr.  Other descriptors (sockets, the console file,
 * EEPROM.DAT mapping, ...) are not touched.
 *
 * Usage:  mq_probe <pid>
 * Output (one line per queue, tab separated, sorted by fd):
 *   fd  name  mq_maxmsg  mq_msgsize  mq_curmsgs  mq_flags
 * Exit status: 0 on success (even with no queue), 2 on usage error, 3 if /proc/<pid>/fd cannot be read.
 * Needs the right to open /proc/<pid>/fd/<n> (ptrace read access: same uid, or root with CAP_SYS_PTRACE and
 * CAP_DAC_OVERRIDE).
 *
 * Build: gcc -O0 -g -Wall -Wextra -o mq_probe mq_probe.c -lrt
 */
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>
#include <unistd.h>

#ifndef MQUEUE_MAGIC
#define MQUEUE_MAGIC 0x19800202
#endif

static int looks_like_osal_queue(const char *t)
{
    /* "/<digits>.<something>" */
    if (t[0] != '/' || t[1] < '0' || t[1] > '9')
    {
        return 0;
    }
    t++;
    while (*t >= '0' && *t <= '9')
    {
        t++;
    }
    return *t == '.';
}

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    char           dirpath[64];
    DIR           *d;
    struct dirent *e;
    int            fds[4096];
    int            nfd = 0;
    int            i;
    long           pid;
    char          *end;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <pid>\n", argv[0]);
        return 2;
    }
    pid = strtol(argv[1], &end, 10);
    if (*end != 0 || pid <= 0)
    {
        fprintf(stderr, "bad pid '%s'\n", argv[1]);
        return 2;
    }

    snprintf(dirpath, sizeof(dirpath), "/proc/%ld/fd", pid);
    d = opendir(dirpath);
    if (d == NULL)
    {
        fprintf(stderr, "opendir(%s): %s\n", dirpath, strerror(errno));
        return 3;
    }
    while ((e = readdir(d)) != NULL && nfd < (int)(sizeof(fds) / sizeof(fds[0])))
    {
        if (e->d_name[0] >= '0' && e->d_name[0] <= '9')
        {
            fds[nfd++] = atoi(e->d_name);
        }
    }
    closedir(d);
    qsort(fds, nfd, sizeof(fds[0]), cmp_int);

    printf("fd\tname\tmq_maxmsg\tmq_msgsize\tmq_curmsgs\tmq_flags\n");
    for (i = 0; i < nfd; i++)
    {
        char           linkpath[96];
        char           target[PATH_MAX];
        ssize_t        n;
        int            qfd;
        struct statfs  sfs;
        struct mq_attr attr;

        snprintf(linkpath, sizeof(linkpath), "/proc/%ld/fd/%d", pid, fds[i]);
        n = readlink(linkpath, target, sizeof(target) - 1);
        if (n < 0)
        {
            continue; /* descriptor closed meanwhile */
        }
        target[n] = 0;
        if (!looks_like_osal_queue(target))
        {
            continue;
        }

        qfd = open(linkpath, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (qfd < 0)
        {
            printf("%d\t%s\tERR_open:%s\t-\t-\t-\n", fds[i], target, strerror(errno));
            continue;
        }
        if (fstatfs(qfd, &sfs) != 0 || (unsigned long)sfs.f_type != (unsigned long)MQUEUE_MAGIC)
        {
            printf("%d\t%s\tNOT_MQUEUE\t-\t-\t-\n", fds[i], target);
            close(qfd);
            continue;
        }
        if (mq_getattr((mqd_t)qfd, &attr) != 0)
        {
            printf("%d\t%s\tERR_getattr:%s\t-\t-\t-\n", fds[i], target, strerror(errno));
            close(qfd);
            continue;
        }
        close(qfd);
        printf("%d\t%s\t%ld\t%ld\t%ld\t0x%lx\n",
               fds[i],
               target,
               (long)attr.mq_maxmsg,
               (long)attr.mq_msgsize,
               (long)attr.mq_curmsgs,
               (unsigned long)attr.mq_flags);
    }
    return 0;
}

/* test_rt_end -- records that outlive the process (event_semantics.md section 12).  The target is this program run
 * again as a child: linked with the start-up object, KZ_RT_STORE names a store file, three threads record, and the
 * process ends in the way the case names.  The parent ends it as a harness would, makes the dump of the store file
 * with kz-rt-dump --recover and checks the dump: it decodes whole, every actor's records are its whole prefix in
 * order, a record being written when the process ended is flagged partial, and the dump says how the run ended.
 * Counts and waits are unit-test values, not research conditions.
 *
 *   test_rt_end <kill|segv|abort|term|term-kill|normal> <dir> <kz-rt-dump>     the harness (parent)
 *   test_rt_end child <case> <ready-fd>                                        the target
 *
 * Cases: kill (raise SIGKILL), segv (SIGSEGV inside a hook: a string copy from an unreadable page), abort, term (a
 * stall that SIGTERM ends), term-kill (a stall whose SIGTERM the target ignores: SIGTERM, then SIGKILL), normal
 * (main returns: the exit handler's dump; the store is recovered too and must hold the same records). */
#include "rt_test.h" /* first: it defines _GNU_SOURCE */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define PER_THREAD 300u /* records per thread: several chunks each */
#define THREADS 3u      /* main (tag 0) and two workers */

static int g_stall;
static int g_ready_fd = -1;
static pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_c = PTHREAD_COND_INITIALIZER;
static unsigned g_done;

static void record(uint32_t tag) {
  uint32_t i, prev = 0;
  for (i = 0; i < PER_THREAD; ++i) prev = kz_rt_obs(10 + tag, 100 + tag, prev, 3u, tag, i, 0, 0);
}

static void *worker(void *arg) {
  record((uint32_t)(uintptr_t)arg);
  (void)pthread_mutex_lock(&g_m);
  ++g_done;
  (void)pthread_cond_broadcast(&g_c);
  (void)pthread_mutex_unlock(&g_m);
  while (g_stall) pause(); /* the stall: alive, recording nothing */
  return NULL;
}

static int child(const char *c, int fd) {
  pthread_t t[THREADS - 1u];
  struct rlimit nocore = {0, 0};
  uint32_t i;
  (void)setrlimit(RLIMIT_CORE, &nocore); /* no core file from the crashing cases */
  (void)prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
  g_ready_fd = fd;
  g_stall = strcmp(c, "term") == 0 || strcmp(c, "term-kill") == 0;
  if (strcmp(c, "term-kill") == 0) (void)signal(SIGTERM, SIG_IGN); /* a target whose SIGTERM does not end it */
  for (i = 0; i + 1u < THREADS; ++i) CHECK(pthread_create(&t[i], NULL, worker, (void *)(uintptr_t)(i + 1u)) == 0);
  record(0);
  (void)pthread_mutex_lock(&g_m);
  while (g_done < THREADS - 1u) (void)pthread_cond_wait(&g_c, &g_m);
  (void)pthread_mutex_unlock(&g_m);
  if (g_stall) {
    CHECK(write(g_ready_fd, "r", 1) == 1);
    for (;;) pause();
  }
  for (i = 0; i + 1u < THREADS; ++i) CHECK(pthread_join(t[i], NULL) == 0);
  if (strcmp(c, "kill") == 0) raise(SIGKILL);
  if (strcmp(c, "abort") == 0) abort();
  if (strcmp(c, "segv") == 0) {
    /* the hook copies a "validated" string from a page it cannot read: the process dies inside the hook */
    void *p = mmap(NULL, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    CHECK(p != MAP_FAILED);
    (void)kz_rt_obs_str(20, 200, 0, 1u, (uint64_t)(uintptr_t)p, 0, 0, 0, 0, 16);
    fprintf(stderr, "segv: the hook returned\n");
    return 3;
  }
  return 0; /* normal: the start-up object's exit handler writes the dump */
}

/* ---- the harness ---------------------------------------------------------------------------------------------- */
static int run(char *const argv[]) {
  int st;
  pid_t p = fork();
  CHECK(p >= 0);
  if (p == 0) {
    execv(argv[0], argv);
    _exit(127);
  }
  CHECK(waitpid(p, &st, 0) == p);
  return WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
}

static int read_all(const char *path, membuf *m) {
  unsigned char b[65536];
  ssize_t r;
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) return 0;
  m->n = 0;
  while ((r = read(fd, b, sizeof b)) > 0) CHECK(mem_write(m, b, (size_t)r) == 0);
  (void)close(fd);
  return 1;
}

static void decode_file(const char *path, membuf *m, items *it, kz_rt_dec_stats *st) {
  CHECK(read_all(path, m));
  it->n = 0;
  CHECK(kz_rt_decode(m->p, m->n, collect, it, st) == KZ_RT_DEC_OK);
  CHECK(st->end_seen && st->crc_ok && st->partial == 0 && st->unknown == 0);
}

static const kz_rt_dec_runx *find_runx(const items *s) {
  size_t i;
  for (i = 0; i < s->n; ++i)
    if (s->v[i].type == KZ_RT_REC_RUNX) return &s->v[i].u.runx;
  return NULL;
}

/* Every run actor's records are its whole prefix, in order: aseq first_aseq, first_aseq + 1, ... in its buffer, and
 * the values of its thread's loop 0, 1, 2, ...; gives the records and the actor per thread tag (main 0, workers 1, 2). */
static void check_prefixes(const items *s, uint32_t per_tag[THREADS], uint32_t actor_of[THREADS]) {
  size_t i;
  for (i = 0; i < THREADS; ++i) per_tag[i] = actor_of[i] = 0;
  for (i = 0; i < s->n; ++i) {
    const kz_rt_dec_actor *a;
    uint32_t next, tag = 0xFFFFFFFFu, n = 0;
    size_t j;
    if (s->v[i].type != KZ_RT_REC_ACTOR) continue;
    a = &s->v[i].u.actor;
    next = a->first_aseq;
    for (j = 0; j < s->n; ++j) {
      const kz_rt_dec_event *e;
      if (s->v[j].type != KZ_RT_REC_EVENT || s->v[j].u.event.owner != a->actor) continue;
      e = &s->v[j].u.event;
      CHECK(e->actor == a->actor && e->aseq == next);
      if (tag == 0xFFFFFFFFu) tag = (uint32_t)e->v[0];
      CHECK(tag < THREADS && e->v[0] == tag && e->v[1] == n && e->site == 10 + tag);
      ++next;
      ++n;
    }
    CHECK(a->last_aseq + 1u == next || a->last_aseq == next); /* last_aseq may lag the record committed last */
    if (tag != 0xFFFFFFFFu) {
      CHECK(actor_of[tag] == 0);
      per_tag[tag] = n;
      actor_of[tag] = a->actor;
    }
  }
}

static char s_self[] = "/proc/self/exe", s_child[] = "child", s_recover[] = "--recover", s_out[] = "--out", s_signal[] = "--signal",
            s_exit[] = "--exit-status";

static int harness(char *c, const char *dir, char *tool) {
  char store[4096], dump[4096], rec[4096], fdtext[16], sig[16], cut[4096];
  int pfd[2], st, abnormal = strcmp(c, "normal") != 0, stall = strcmp(c, "term") == 0 || strcmp(c, "term-kill") == 0;
  pid_t p;
  membuf m = {0}, m2 = {0};
  items it = {0}, it2 = {0};
  kz_rt_dec_stats ds;
  const kz_rt_dec_runx *x;
  uint32_t per_tag[THREADS], actor_of[THREADS], i, end_flag, end_value;
  struct stat sb;
  snprintf(store, sizeof store, "%s/rt_end_%s.store", dir, c);
  snprintf(dump, sizeof dump, "%s/rt_end_%s.kzrt", dir, c);
  snprintf(rec, sizeof rec, "%s/rt_end_%s.rec.kzrt", dir, c);
  (void)unlink(store);
  (void)unlink(dump);
  (void)unlink(rec);
  CHECK(pipe(pfd) == 0);
  p = fork();
  CHECK(p >= 0);
  if (p == 0) {
    char *const args[] = {s_self, s_child, c, fdtext, NULL};
    (void)close(pfd[0]);
    snprintf(fdtext, sizeof fdtext, "%d", pfd[1]);
    setenv("KZ_RT_STORE", store, 1);
    setenv("KZ_RT_DUMP", dump, 1);
    setenv("KZ_RT_MODE", "record", 1);
    setenv("KZ_RT_RUN_ID", "7", 1);
    execv("/proc/self/exe", args);
    _exit(127);
  }
  (void)close(pfd[1]);
  if (stall) { /* the harness sees the stall (here: the target says it is ready), then SIGTERM, then SIGKILL */
    struct pollfd q;
    char b;
    int k;
    q.fd = pfd[0];
    q.events = POLLIN;
    CHECK(poll(&q, 1, 20000) == 1 && read(pfd[0], &b, 1) == 1);
    CHECK(kill(p, SIGTERM) == 0);
    for (k = 0; k < 50 && waitpid(p, &st, WNOHANG) == 0; ++k) sleep_ms(10);
    if (k == 50) {
      CHECK(kill(p, SIGKILL) == 0);
      CHECK(waitpid(p, &st, 0) == p);
    }
  } else {
    CHECK(waitpid(p, &st, 0) == p);
  }
  (void)close(pfd[0]);
  if (WIFSIGNALED(st)) {
    end_flag = KZ_RT_ENDF_SIGNAL;
    end_value = (uint32_t)WTERMSIG(st);
  } else {
    CHECK(WIFEXITED(st));
    end_flag = KZ_RT_ENDF_EXIT;
    end_value = (uint32_t)WEXITSTATUS(st);
  }
  printf("%s: the target ended with %s %u\n", c, end_flag == KZ_RT_ENDF_SIGNAL ? "signal" : "exit status", end_value);
  if (strcmp(c, "kill") == 0 || strcmp(c, "term-kill") == 0) CHECK(end_flag == KZ_RT_ENDF_SIGNAL && end_value == SIGKILL);
  if (strcmp(c, "segv") == 0) CHECK(end_flag == KZ_RT_ENDF_SIGNAL && end_value == SIGSEGV);
  if (strcmp(c, "abort") == 0) CHECK(end_flag == KZ_RT_ENDF_SIGNAL && end_value == SIGABRT);
  if (strcmp(c, "term") == 0) CHECK(end_flag == KZ_RT_ENDF_SIGNAL && end_value == SIGTERM);
  if (!abnormal) CHECK(end_flag == KZ_RT_ENDF_EXIT && end_value == 0);
  CHECK(stat(dump, &sb) == (abnormal ? -1 : 0)); /* only a normal exit ran the exit handler's dump */

  /* the dump of the store file, as the harness makes it */
  snprintf(sig, sizeof sig, "%u", end_value);
  {
    char *const args[] = {tool, s_recover, store, s_out, rec, end_flag == KZ_RT_ENDF_SIGNAL ? s_signal : s_exit, sig, NULL};
    CHECK(run(args) == 0);
  }
  decode_file(rec, &m, &it, &ds);
  CHECK((ds.dump_flags & KZ_RT_DUMP_RECOVERED) != 0 && ds.minor >= 4u);
  x = find_runx(&it);
  CHECK(x != NULL && x->end_kind == KZ_RT_END_RECOVERED && x->store_flags == KZ_RT_STOREF_FILE);
  CHECK((x->end_flags & (KZ_RT_ENDF_SIGNAL | KZ_RT_ENDF_EXIT)) == end_flag && x->end_value == end_value);
  CHECK(((x->end_flags & KZ_RT_ENDF_DUMPED) != 0) == !abnormal);
  CHECK(loss_count(&it, KZ_RT_LOSS_ABNORMAL_END, 0) == (abnormal ? 1u : 0u));
  check_prefixes(&it, per_tag, actor_of);
  for (i = 0; i < THREADS; ++i) CHECK(per_tag[i] == PER_THREAD && actor_of[i] != 0);
  CHECK(count_type(&it, KZ_RT_REC_EVENT) == THREADS * PER_THREAD && loss_count(&it, KZ_RT_LOSS_PARTIAL, 0) == 0);
  for (i = 0; i < it.n; ++i) /* a record being written when the process ended: only main's, in the crash inside the hook */
    if (it.v[i].type == KZ_RT_REC_ACTOR) {
      const kz_rt_dec_actor *a = &it.v[i].u.actor;
      const int crashed = strcmp(c, "segv") == 0 && a->actor == actor_of[0];
      CHECK(a->partial == (uint32_t)crashed && loss_count(&it, KZ_RT_LOSS_PARTIAL, a->actor) == (uint64_t)crashed);
      CHECK(a->overflow == 0 && loss_count(&it, KZ_RT_LOSS_OVERFLOW, a->actor) == 0);
      if (crashed) printf("segv: actor %u was inside a hook when the process ended: flagged partial\n", a->actor);
    }

  if (!abnormal) {
    /* a normal exit is unchanged: the exit handler's dump is an in-process dump with the same records */
    size_t a = 0, b = 0;
    decode_file(dump, &m2, &it2, &ds);
    CHECK((ds.dump_flags & KZ_RT_DUMP_RECOVERED) == 0 && (ds.dump_flags & KZ_RT_DUMP_STOPPED) != 0);
    x = find_runx(&it2);
    CHECK(x != NULL && x->end_kind == KZ_RT_END_IN_PROCESS && x->end_flags == 0 && x->store_flags == KZ_RT_STOREF_FILE);
    CHECK(loss_count(&it2, KZ_RT_LOSS_ABNORMAL_END, 0) == 0);
    for (;;) {
      while (a < it.n && it.v[a].type != KZ_RT_REC_EVENT) ++a;
      while (b < it2.n && it2.v[b].type != KZ_RT_REC_EVENT) ++b;
      if (a == it.n || b == it2.n) break;
      CHECK(it.v[a].u.event.actor == it2.v[b].u.event.actor && it.v[a].u.event.aseq == it2.v[b].u.event.aseq);
      CHECK(it.v[a].u.event.v[0] == it2.v[b].u.event.v[0] && it.v[a].u.event.v[1] == it2.v[b].u.event.v[1]);
      ++a;
      ++b;
    }
    CHECK(a == it.n && b == it2.n);
    { /* a cut file and a store of another build are refused, and nothing is written */
      FILE *f;
      char *const args[] = {tool, s_recover, cut, s_out, rec, NULL};
      snprintf(cut, sizeof cut, "%s/rt_end_cut.store", dir);
      (void)unlink(rec);
      CHECK(read_all(store, &m2));
      f = fopen(cut, "wb");
      CHECK(f != NULL && fwrite(m2.p, 1, m2.n / 2u, f) == m2.n / 2u);
      fclose(f);
      CHECK(run(args) == 2 && stat(rec, &sb) == -1);
      m2.p[40] ^= 0x01; /* pool_bytes of the header */
      f = fopen(cut, "wb");
      CHECK(f != NULL && fwrite(m2.p, 1, m2.n, f) == m2.n);
      fclose(f);
      CHECK(run(args) == 2 && stat(rec, &sb) == -1);
      (void)unlink(cut);
    }
  }
  (void)unlink(store); /* the store files are as large as the store; the dumps stay */
  printf("%s: recovered dump checked (%u records per thread, end %s)\n", c, PER_THREAD, abnormal ? "abnormal: a prefix" : "normal: dumped");
  free(m.p);
  free(m2.p);
  free(it.v);
  free(it2.v);
  return 0;
}

int main(int argc, char **argv) {
  if (argc == 4 && strcmp(argv[1], "child") == 0) return child(argv[2], atoi(argv[3]));
  if (argc == 4) return harness(argv[1], argv[2], argv[3]);
  fprintf(stderr, "usage: test_rt_end <case> <dir> <kz-rt-dump> | child <case> <fd>\n");
  return 2;
}

// run_target.cpp -- see run_target.hpp.
#include "run_target.hpp"

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cstring>

namespace kz::probe {

namespace {

std::atomic<pid_t> g_child{0};
std::atomic<int> g_forwarded{0};

void forward_signal(int sig) {
  g_forwarded.store(sig);
  const pid_t c = g_child.load();
  if (c > 0) kill(c, sig);
}

}  // namespace

uint64_t monotonic_ns() {
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ull + static_cast<uint64_t>(ts.tv_nsec);
}

Status run_target(const std::vector<std::string>& args, TargetRun* out, const std::function<void(pid_t)>& after_start) {
  *out = TargetRun();
  if (args.empty()) return Status::error(Err::Invalid, "no command to run");
  std::vector<char*> argv;
  for (const std::string& a : args) argv.push_back(const_cast<char*>(a.c_str()));
  argv.push_back(nullptr);

  struct sigaction sa, old_int, old_term, old_hup;
  std::memset(&sa, 0, sizeof sa);
  sa.sa_handler = forward_signal;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, &old_int);
  sigaction(SIGTERM, &sa, &old_term);
  sigaction(SIGHUP, &sa, &old_hup);
  g_forwarded.store(0);

  out->t_start_ns = monotonic_ns();
  const pid_t pid = fork();
  if (pid < 0) {
    const int e = errno;
    sigaction(SIGINT, &old_int, nullptr);
    sigaction(SIGTERM, &old_term, nullptr);
    sigaction(SIGHUP, &old_hup, nullptr);
    return Status::error(Err::Io, std::string("fork failed: ") + std::strerror(e));
  }
  if (pid == 0) {
    // The child gets the signal dispositions of a freshly started program.
    sigaction(SIGINT, &old_int, nullptr);
    sigaction(SIGTERM, &old_term, nullptr);
    sigaction(SIGHUP, &old_hup, nullptr);
    execvp(argv[0], argv.data());
    const int e = errno;
    const char* msg = "kz observe: cannot execute the command: ";
    if (write(2, msg, std::strlen(msg)) < 0) {}
    if (write(2, std::strerror(e), std::strlen(std::strerror(e))) < 0) {}
    if (write(2, "\n", 1) < 0) {}
    _exit(127);
  }
  g_child.store(pid);
  out->pid = pid;
  if (after_start) after_start(pid);
  int status = 0;
  pid_t w;
  do {
    w = wait4(pid, &status, 0, &out->usage);
  } while (w < 0 && errno == EINTR);
  out->t_end_ns = monotonic_ns();
  g_child.store(0);
  sigaction(SIGINT, &old_int, nullptr);
  sigaction(SIGTERM, &old_term, nullptr);
  sigaction(SIGHUP, &old_hup, nullptr);
  out->forwarded_signal = g_forwarded.load();
  if (w < 0) return Status::error(Err::Io, std::string("waiting for the command failed: ") + std::strerror(errno));
  if (WIFEXITED(status)) {
    out->exited = true;
    out->exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    out->term_signal = WTERMSIG(status);
  }
  return Status::success();
}

}  // namespace kz::probe

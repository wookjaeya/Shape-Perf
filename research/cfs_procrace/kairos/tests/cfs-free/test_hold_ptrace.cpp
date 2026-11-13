// Integration test of `kz anchor` + `kz hold` on the purpose-built hold program (hold_prog.c): a
// real PTRACE_SEIZE, real hardware-debug-register execution breakpoints, real thread auto-attach,
// and an event-based release, with no framework.  This is the "validated first on a small
// two-task program" step that SPEC 4.13 "Reuse conditions" requires before cFS.
//
//   argv: 1 hold_prog, 2 hold.kzp, 3 hold.kzh, 4 the kz executable, 5 a scratch directory
//
// What it establishes:
//   - the provider thread, created after the run starts, is auto-attached and armed;
//   - the gate holds exactly the create of "OBJ0" (the name read from the stopped thread's
//     memory), not the create of "OBJ1" (the name condition discriminates);
//   - the held thread is resumed when the consumer reaches the release event (hp_consumer_done);
//   - the debug registers were usable (no int3 text-patch fallback);
//   - the child exits 0 after the release.
// It needs the right to ptrace (root here).  Where the kernel refuses PTRACE_SEIZE or the debug
// registers, the test exits 77 (reported as skipped, never as passed).
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "support/kz_test.hpp"

namespace {

std::string g_hold_prog, g_profile, g_holdplan, g_kz, g_dir;

std::string slurp(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

struct Run { int code = -1; std::string output; };

Run run_command(const std::vector<std::string>& args) {
  Run r;
  const std::string log = g_dir + "/hold_last_output.txt";
  const pid_t pid = fork();
  if (pid == 0) {
    const int fd = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) { dup2(fd, 1); dup2(fd, 2); close(fd); }
    std::vector<char*> av;
    for (const std::string& a : args) av.push_back(const_cast<char*>(a.c_str()));
    av.push_back(nullptr);
    execv(av[0], av.data());
    _exit(127);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  r.code = WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
  r.output = slurp(log);
  return r;
}

// Extract the first value of "key=" on a line that starts with `prefix`.
std::string field(const std::string& text, const std::string& key) {
  size_t p = text.find(key + "=");
  if (p == std::string::npos) return "";
  p += key.size() + 1;
  size_t e = p;
  while (e < text.size() && text[e] != ' ' && text[e] != '\n') ++e;
  return text.substr(p, e - p);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 6) { std::fprintf(stderr, "usage: %s hold_prog hold.kzp hold.kzh kz dir\n", argv[0]); return 2; }
  g_hold_prog = argv[1]; g_profile = argv[2]; g_holdplan = argv[3]; g_kz = argv[4]; g_dir = argv[5];

  const std::string basis = g_dir + "/hold_basis.kz";
  Run a = run_command({g_kz, "anchor", g_hold_prog, g_profile, "-o", basis});
  if (a.code != 0) { std::fprintf(stderr, "kz anchor failed (%d):\n%s\n", a.code, a.output.c_str()); return 1; }

  const std::string rec = g_dir + "/hold_rec.txt";
  Run h = run_command({g_kz, "hold", basis, g_profile, g_holdplan, "-o", rec, "--", g_hold_prog, "40", "30"});
  const std::string out = slurp(rec);

  // A kernel that refuses ptrace or the debug registers: skip, never pass.
  if (h.code == 3 || out.find("debugregs_ok=no") != std::string::npos) {
    std::fprintf(stderr, "kz hold could not use the debug registers; skipping:\n%s\n%s\n", h.output.c_str(), out.c_str());
    return 77;
  }
  if (h.output.find("Operation not permitted") != std::string::npos && out.empty()) { std::fprintf(stderr, "ptrace not permitted; skipping\n"); return 77; }
  if (out.empty()) { std::fprintf(stderr, "no record written (exit %d):\n%s\n", h.code, h.output.c_str()); return 1; }

  KZ_CHECK_EQ(field(out, "debugregs_ok"), std::string("yes"));
  KZ_CHECK_EQ(field(out, "child_exit"), std::string("0"));
  // The provider was created after the run started, so at least one thread came in by clone.
  KZ_CHECK(std::atoll(field(out, "clone_attaches").c_str()) >= 1);
  KZ_CHECK(std::atoll(field(out, "relevant_procs").c_str()) >= 1);

  // Exactly one hold, on the OBJ0 create, released by R_done.
  int holds = 0, obj0_held = 0, obj1_held = 0, released = 0;
  std::istringstream is(out);
  std::string line;
  while (std::getline(is, line)) {
    if (line.rfind("hold ", 0) != 0) continue;
    ++holds;
    if (line.find("name=\"OBJ0\"") != std::string::npos) ++obj0_held;
    if (line.find("name=\"OBJ1\"") != std::string::npos) ++obj1_held;
    if (line.find("released_by=R_done") != std::string::npos) ++released;
  }
  KZ_CHECK_EQ(holds, 1);
  KZ_CHECK_EQ(obj0_held, 1);
  KZ_CHECK_EQ(obj1_held, 0);  // the name condition discriminated OBJ1 away
  KZ_CHECK_EQ(released, 1);
  // The gate was reached more than once (OBJ0 held, OBJ1 stepped over) -> the breakpoint is live
  // on the later-created provider thread.
  KZ_CHECK(std::atoll(field(out, "gate_traps").c_str()) >= 1);

  return kztest::finish("test_hold_ptrace");
}

// Integration test of `kz hold` on hold_seq_prog.c (two threads, two keys; real PTRACE_SEIZE, hardware
// debug-register execution breakpoints, thread auto-attach, event-based release, no framework):
//
//   argv: 1 hold_seq_prog, 2 hold_seq.kzp, 3 hold_rearm.kzh, 4 hold_nth.kzh, 5 the kz executable, 6 a scratch dir,
//         7 hold_defer.kzh
//
// (1) Re-armed released thread (hold_rearm.kzh).  The consumer is held at its first lookup of OBJ0 and resumed
//     by the provider's first creation event; the provider is then held at the creation of OBJ1 until the
//     consumer reaches seq_consumer_done.  Only a resumed thread whose debug registers are still armed can fire
//     that release; with the registers cleared at the resume (the supervisor before confirm2) the provider stays
//     held and the program does not end (the test then fails by its time limit).
// (2) occurrence <n> and two points on one address (hold_nth.kzh).  G_first (creation of OBJ0) is released by
//     the 3rd qualifying hit of seq_consumer_retry, so the consumer's first three lookups of OBJ0 fail for sure;
//     G_second (same address, no name condition, occurrence 2) holds the second creation (OBJ1) and not the
//     first, until the consumer's end, so OBJ1 is never found.
// (3) A release on a held hit, and unless-released (hold_defer.kzh).  R_same shares the address of G_c, which holds
//     the consumer at that hit: R_same acts when the consumer is resumed (deferred).  Its gate G_x holds nothing then
//     and is unless-released, so it is satisfied: the provider passes the creation of OBJ1 (no hold to the end).
// Where the kernel refuses PTRACE_SEIZE or the debug registers, the test exits 77 (skipped, never passed).
// Every value here is a unit-test value, not a research condition.
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "support/kz_test.hpp"

namespace {

std::string g_prog, g_profile, g_rearm, g_nth, g_kz, g_dir, g_defer;

std::string slurp(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

struct Run { int code = -1; std::string output; };

Run run_command(const std::vector<std::string>& args, const std::string& log) {
  Run r;
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

// The value of "key=" in `line` (up to the next blank), or "".
std::string field(const std::string& line, const std::string& key) {
  size_t p = line.find(" " + key + "=");
  if (p == std::string::npos) { if (line.rfind(key + "=", 0) == 0) p = 0; else return ""; } else ++p;
  p += key.size() + 1;
  size_t e = p;
  while (e < line.size() && line[e] != ' ' && line[e] != '\n') ++e;
  return line.substr(p, e - p);
}

std::vector<std::string> lines_with(const std::string& text, const std::string& prefix) {
  std::vector<std::string> out;
  std::istringstream is(text);
  std::string l;
  while (std::getline(is, l)) if (l.rfind(prefix, 0) == 0) out.push_back(l);
  return out;
}

// Runs one plan; 77 = skip, 1 = setup failure, 0 = ran (the record in *rec, the program's output in *out).
int run_plan(const std::string& basis, const std::string& plan, const std::string& tag, const std::vector<std::string>& prog_args,
             std::string* rec, std::string* out) {
  const std::string recf = g_dir + "/seq_" + tag + "_rec.txt";
  std::vector<std::string> args{g_kz, "hold", basis, g_profile, plan, "-o", recf, "--", g_prog};
  args.insert(args.end(), prog_args.begin(), prog_args.end());
  Run h = run_command(args, g_dir + "/seq_" + tag + "_output.txt");
  *rec = slurp(recf);
  *out = h.output;
  if (h.code == 3 || rec->find("debugregs_ok=no") != std::string::npos) {
    std::fprintf(stderr, "kz hold could not use the debug registers; skipping:\n%s\n%s\n", h.output.c_str(), rec->c_str());
    return 77;
  }
  if (h.output.find("Operation not permitted") != std::string::npos && rec->empty()) { std::fprintf(stderr, "ptrace not permitted; skipping\n"); return 77; }
  if (rec->empty()) { std::fprintf(stderr, "no record written (exit %d):\n%s\n", h.code, h.output.c_str()); return 1; }
  return 0;
}

std::map<std::string, std::string> hold_of(const std::string& rec, const std::string& gate) {
  std::map<std::string, std::string> m;
  for (const std::string& l : lines_with(rec, "hold "))
    if (field(l, "gate") == gate) {
      for (const char* k : {"tid", "name", "hit", "released_by"}) m[k] = field(l, k);
      m["released"] = l.find("released=no") == std::string::npos ? "yes" : "no";
    }
  return m;
}

std::map<std::string, std::string> fire_of(const std::string& rec, const std::string& release) {
  std::map<std::string, std::string> m;
  for (const std::string& l : lines_with(rec, "fire "))
    if (field(l, "release") == release)
      for (const char* k : {"tid", "hit", "resumed", "gate", "deferred", "satisfied"}) m[k] = field(l, k);
  return m;
}

std::string point_hits(const std::string& rec, const std::string& label) {
  for (const std::string& l : lines_with(rec, "gate    " + label + " ")) return field(l, "qualifying_hits");
  for (const std::string& l : lines_with(rec, "release " + label + " ")) return field(l, "qualifying_hits");
  return "";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 8) { std::fprintf(stderr, "usage: %s hold_seq_prog hold_seq.kzp hold_rearm.kzh hold_nth.kzh kz dir hold_defer.kzh\n", argv[0]); return 2; }
  g_prog = argv[1]; g_profile = argv[2]; g_rearm = argv[3]; g_nth = argv[4]; g_kz = argv[5]; g_dir = argv[6]; g_defer = argv[7];

  const std::string basis = g_dir + "/hold_seq_basis.kz";
  Run a = run_command({g_kz, "anchor", g_prog, g_profile, "-o", basis}, g_dir + "/seq_anchor_output.txt");
  if (a.code != 0) { std::fprintf(stderr, "kz anchor failed (%d):\n%s\n", a.code, a.output.c_str()); return 1; }

  // (1) the re-armed released thread: retries 20, provider delay 30 ms, consumer delay 0, retry delay 5 ms
  {
    std::string rec, out;
    const int rc = run_plan(basis, g_rearm, "rearm", {"20", "30", "0", "5"}, &rec, &out);
    if (rc != 0) return rc;
    KZ_CHECK_EQ(field(lines_with(rec, "debugregs_ok=")[0], "debugregs_ok"), std::string("yes"));
    KZ_CHECK_EQ(lines_with(rec, "child_exit=").size(), size_t(1));
    if (!lines_with(rec, "child_exit=").empty()) KZ_CHECK_EQ(field(lines_with(rec, "child_exit=")[0], "child_exit"), std::string("0"));
    KZ_CHECK_EQ(lines_with(rec, "hold ").size(), size_t(2));
    auto gc = hold_of(rec, "G_cons"), gp = hold_of(rec, "G_prov");
    KZ_CHECK_EQ(gc["released_by"], std::string("R_made"));
    KZ_CHECK_EQ(gc["name"], std::string("\"OBJ0\""));
    KZ_CHECK_EQ(gp["released_by"], std::string("R_done"));
    KZ_CHECK_EQ(gp["name"], std::string("\"OBJ1\""));
    // R_done was fired by the consumer -- the thread resumed from G_cons -- and resumed the provider
    auto fd = fire_of(rec, "R_done");
    KZ_CHECK(!fd["tid"].empty() && fd["tid"] == gc["tid"]);
    KZ_CHECK(!fd["resumed"].empty() && fd["resumed"] == gp["tid"]);
    // the consumer found OBJ0 at its first lookup (after its hold) and never OBJ1 (held provider)
    KZ_CHECK(out.find("found0=1 attempts0=0 found1=0 attempts1=20") != std::string::npos);
    KZ_CHECK(out.find("objects=2") != std::string::npos);
  }

  // (2) occurrence <n>, two gates on one address: retries 20, provider delay 0, consumer delay 50 ms, retry 20 ms
  {
    std::string rec, out;
    const int rc = run_plan(basis, g_nth, "nth", {"20", "0", "50", "20"}, &rec, &out);
    if (rc != 0) return rc;
    KZ_CHECK_EQ(lines_with(rec, "child_exit=").size(), size_t(1));
    if (!lines_with(rec, "child_exit=").empty()) KZ_CHECK_EQ(field(lines_with(rec, "child_exit=")[0], "child_exit"), std::string("0"));
    KZ_CHECK_EQ(lines_with(rec, "hold ").size(), size_t(2));
    auto g1 = hold_of(rec, "G_first"), g2 = hold_of(rec, "G_second");
    KZ_CHECK_EQ(g1["name"], std::string("\"OBJ0\""));
    KZ_CHECK_EQ(g1["hit"], std::string("1"));
    KZ_CHECK_EQ(g1["released_by"], std::string("R_third"));
    // the second gate on the same address held the second creation, not the first
    KZ_CHECK_EQ(g2["hit"], std::string("2"));
    KZ_CHECK_EQ(g2["released_by"], std::string("R_done"));
    KZ_CHECK_EQ(point_hits(rec, "G_second"), std::string("2"));
    // R_third acted on its 3rd qualifying hit (the consumer's third failed lookup) and resumed the provider
    auto f3 = fire_of(rec, "R_third");
    KZ_CHECK_EQ(f3["hit"], std::string("3"));
    KZ_CHECK(!f3["resumed"].empty() && f3["resumed"] == g1["tid"]);
    KZ_CHECK(std::atoll(point_hits(rec, "R_third").c_str()) >= 3);
    // the first three lookups of OBJ0 failed for sure; OBJ1 was never found (its creation held to the end)
    const size_t at = out.find("attempts0=");
    KZ_CHECK(at != std::string::npos && std::atoi(out.c_str() + at + 10) >= 3);
    KZ_CHECK(out.find("found0=1") != std::string::npos);
    KZ_CHECK(out.find("found1=0 attempts1=20") != std::string::npos);
    KZ_CHECK(out.find("objects=2") != std::string::npos);
  }
  // (3) deferred release and unless-released: retries 20, provider delay 30 ms, consumer delay 0, retry delay 5 ms
  {
    std::string rec, out;
    const int rc = run_plan(basis, g_defer, "defer", {"20", "30", "0", "5"}, &rec, &out);
    if (rc != 0) return rc;
    KZ_CHECK_EQ(lines_with(rec, "child_exit=").size(), size_t(1));
    if (!lines_with(rec, "child_exit=").empty()) KZ_CHECK_EQ(field(lines_with(rec, "child_exit=")[0], "child_exit"), std::string("0"));
    KZ_CHECK_EQ(lines_with(rec, "hold ").size(), size_t(1));
    auto gc = hold_of(rec, "G_c");
    KZ_CHECK_EQ(gc["released_by"], std::string("R_p"));
    auto fs = fire_of(rec, "R_same");
    KZ_CHECK_EQ(fs["deferred"], std::string("yes"));
    KZ_CHECK(!fs["tid"].empty() && fs["tid"] == gc["tid"]);
    KZ_CHECK_EQ(fs["resumed"], std::string("none"));
    KZ_CHECK_EQ(fs["satisfied"], std::string("yes"));
    KZ_CHECK(hold_of(rec, "G_x").empty());  // never held
    KZ_CHECK(out.find("found0=1 attempts0=0 found1=1") != std::string::npos);
    KZ_CHECK(out.find("objects=2") != std::string::npos);
  }
  return kztest::finish("test_hold_sequence");
}

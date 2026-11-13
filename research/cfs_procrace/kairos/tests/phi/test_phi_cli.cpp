// The kz-phi command line tool on the real profile: its output is that of the library, a broken
// profile is rejected with exit status 1 and a position, a usage or I/O error gives 2.
//
//   test_phi_cli <kz-phi> <profile-dir> <data-dir> <trace-dir>

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "support/kz_test.hpp"

namespace fs = std::filesystem;

namespace {

// Runs a command; returns its exit status, captures stdout and stderr separately.
int run(const std::string& command, std::string* out, std::string* err = nullptr) {
  const std::string err_file = "kz_phi_cli_err_" + std::to_string(static_cast<long>(::getpid())) + ".txt";
  out->clear();
  FILE* p = popen((command + " 2>" + err_file).c_str(), "r");
  if (p == nullptr) return -1;
  char buf[4096];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), p)) > 0) out->append(buf, n);
  const int status = pclose(p);
  if (err != nullptr) {
    std::ifstream in(err_file, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    *err = ss.str();
  }
  std::remove(err_file.c_str());
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: test_phi_cli <kz-phi> <profile-dir> <data-dir> <trace-dir>\n");
    return 2;
  }
  const std::string tool = argv[1], dir = argv[2], data = argv[3], traces = argv[4];

  // the profile, loaded by the library
  std::vector<kz::phi::SourceText> files;
  {
    std::vector<std::string> paths;
    for (const fs::directory_entry& e : fs::directory_iterator(dir))
      if (e.is_regular_file() && e.path().extension() == ".kzp") paths.push_back(e.path().string());
    std::sort(paths.begin(), paths.end());
    for (const std::string& p : paths) files.push_back(kz::phi::SourceText{p, read_file(p)});
  }
  kz::phi::Profile prof;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(files, &prof, &diags));
  const std::string canonical = kz::phi::canonical_text(prof);

  std::string out, err;
  KZ_CHECK_EQ(run(tool + " check " + dir, &out, &err), 0);
  KZ_CHECK(out.rfind("ok: ", 0) == 0);
  KZ_CHECK_EQ(run(tool + " dump " + dir, &out), 0);
  KZ_CHECK_EQ(out, canonical);
  KZ_CHECK_EQ(run(tool + " hash " + dir, &out), 0);
  KZ_CHECK_EQ(out, kz::to_hex(kz::phi::profile_hash(prof)) + "\n");
  KZ_CHECK_EQ(run(tool + " stats " + dir, &out), 0);
  KZ_CHECK_EQ(out, kz::phi::stats_text(kz::phi::profile_stats(prof)));

  {  // the canonical text, given back to the tool as a single file, is a fixed point with the same hash
    const std::string again = "kz_phi_cli_canon_" + std::to_string(static_cast<long>(::getpid())) + ".kzp";
    {
      std::ofstream f(again, std::ios::binary);
      f << canonical;
    }
    std::string out2;
    KZ_CHECK_EQ(run(tool + " dump " + again, &out2), 0);
    KZ_CHECK_EQ(out2, canonical);
    KZ_CHECK_EQ(run(tool + " hash " + again, &out2), 0);
    KZ_CHECK_EQ(out2, kz::to_hex(kz::phi::profile_hash(prof)) + "\n");
    std::remove(again.c_str());
  }

  {  // a profile with a syntax error: exit 1, the position on stderr
    const std::string bad = "kz_phi_cli_bad_" + std::to_string(static_cast<long>(::getpid())) + ".kzp";
    {
      std::ofstream f(bad, std::ios::binary);
      f << "phi 1;\nprofile \"x\";\nconst A = ;\n";
    }
    KZ_CHECK_EQ(run(tool + " check " + bad, &out, &err), 1);
    KZ_CHECK(err.find(bad + ":3:11: error: expected an integer, a string, true, false or 'ref eval ...'") != std::string::npos);
    // a semantic error: exit 1, every error is listed
    {
      std::ofstream f(bad, std::ios::binary);
      f << "phi 1;\nprofile \"x\";\nkind k() { states A < A; initial B; }\n";
    }
    KZ_CHECK_EQ(run(tool + " check " + bad, &out, &err), 1);
    KZ_CHECK(err.find("state 'A' is declared twice") != std::string::npos);
    KZ_CHECK(err.find("the initial state 'B' is not one of its states") != std::string::npos);
    std::remove(bad.c_str());
  }

  // usage and I/O errors
  KZ_CHECK_EQ(run(tool, &out, &err), 2);
  KZ_CHECK_EQ(run(tool + " check", &out, &err), 2);
  KZ_CHECK_EQ(run(tool + " frobnicate " + dir, &out, &err), 2);
  KZ_CHECK_EQ(run(tool + " check no_such_file_anywhere.kzp", &out, &err), 2);
  KZ_CHECK_EQ(run(tool + " verify-prov " + dir, &out, &err), 2);  // no --root
  KZ_CHECK_EQ(run(tool + " replay " + dir, &out, &err), 2);       // no --trace
  KZ_CHECK_EQ(run(tool + " replay --trace no_such_trace.trace " + dir, &out, &err), 2);

  // the replay command prints the library's result; the golden file is the same text under a header line
  {
    const std::string predicate = "R19=forall r: es.app where state(r) != FREE { state(es.appstate(r)) >= RUNNING }";
    const std::string cmd = tool + " replay --trace " + traces + "/startup_ok.trace --predicate '" + predicate + "' --version cfe=7.0.1 " + dir;
    KZ_CHECK_EQ(run(cmd, &out, &err), 0);
    const std::string golden = read_file(data + "/replay_startup_ok.golden");
    const size_t nl = golden.find('\n');
    KZ_CHECK(nl != std::string::npos);
    KZ_CHECK_EQ(out, golden.substr(nl + 1));
    // without the predicate and the version the result says why the observation rule gave no edge
    KZ_CHECK_EQ(run(tool + " replay --trace " + traces + "/startup_ok.trace " + dir, &out, &err), 0);
    KZ_CHECK(out.find("rule R19 is not applied: its guard on 'cfe' cannot be evaluated (no version given)") != std::string::npos);
    // a predicate that does not resolve is an error
    KZ_CHECK_EQ(run(tool + " replay --trace " + traces + "/startup_ok.trace --predicate 'R19=nonsense(' " + dir, &out, &err), 2);
    KZ_CHECK_EQ(run(tool + " replay --trace " + traces + "/startup_ok.trace --predicate 'NoSuchRule=true' " + dir, &out, &err), 2);
    KZ_CHECK(err.find("not an observation rule") != std::string::npos);
  }

  return kztest::finish("test_phi_cli");
}

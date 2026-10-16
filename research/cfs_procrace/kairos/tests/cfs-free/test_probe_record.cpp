// Unit tests of the run record (conditions file), the /proc readers, the CPU-list parser and the
// target runner.
#include <sys/resource.h>
#include <unistd.h>

#include <fstream>

#include "perf_session.hpp"
#include "proc_capture.hpp"
#include "run_record.hpp"
#include "run_target.hpp"
#include "support/kz_test.hpp"

using namespace kz::probe;

namespace {

void test_conditions() {
  RunRecord r;
  const std::string text =
      "# a comment\n"
      "\n"
      "item\tcfs.user\tubuntu\tfrom-reference\n"
      "item\trun.count\t1\tdeviation\n"
      "item\trun.stop\tci-es-restart-poweron\tfrom-reference\n"
      "item\tnote\tfree text with spaces\tunset\n"
      "deviation\trun.count\t3 (the reference gdb runs)\t1\tsmoke test\tno frequency can be claimed\tno\n";
  KZ_CHECK_OK(load_conditions(text, &r));
  KZ_CHECK_EQ(r.items.size(), size_t(4));
  KZ_CHECK_EQ(r.deviations.size(), size_t(1));
  KZ_CHECK_EQ(r.items[0].policy, uint8_t(KZ_POLICY_FROM_REFERENCE));
  KZ_CHECK_EQ(r.items[1].policy, uint8_t(KZ_POLICY_DEVIATION));
  KZ_CHECK_EQ(r.items[1].deviation, 0);          // linked to the item of the same name
  KZ_CHECK_EQ(r.items[0].deviation, -1);
  KZ_CHECK_EQ(r.deviations[0].avoidable, uint8_t(KZ_AVOIDABLE_NO));
  KZ_CHECK_EQ(r.deviations[0].reason, std::string("smoke test"));
  KZ_CHECK(r.find("cfs.user") != nullptr);
  KZ_CHECK(r.find("nope") == nullptr);
  const std::string t = r.text();
  KZ_CHECK(t.find("cfs.user = ubuntu   [from-reference]") != std::string::npos);
  KZ_CHECK(t.find("deviation: reference: 3 (the reference gdb runs); used: 1; reason: smoke test; effect: no frequency can be claimed") != std::string::npos);

  // a deviation with no item of its name is still reported
  RunRecord orphan;
  KZ_CHECK_OK(load_conditions("deviation\tx\ta\tb\tr\te\tyes\n", &orphan));
  KZ_CHECK(orphan.text().find("deviation (no item of that name): x") != std::string::npos);

  auto bad = [](const std::string& s) { RunRecord x; return load_conditions(s, &x).code == kz::Err::Invalid; };
  KZ_CHECK(bad("item\tonly two\tfields\n"));
  KZ_CHECK(bad("item\tn\tv\tnot-a-policy\n"));
  KZ_CHECK(bad("item\t\tv\tunset\n"));
  KZ_CHECK(bad("deviation\ta\tb\tc\n"));
  KZ_CHECK(bad("deviation\ta\tb\tc\tr\te\tmaybe\n"));
  KZ_CHECK(bad("deviation\t\tb\tc\tr\te\tyes\n"));      // no subject
  KZ_CHECK(bad("deviation\ta\tb\tc\t\te\tyes\n"));      // no reason
  KZ_CHECK(bad("frobnicate\tx\n"));
  uint8_t p = 0;
  KZ_CHECK(policy_from_name("unspecified_by_reference", &p));
  KZ_CHECK_EQ(p, uint8_t(KZ_POLICY_UNSPECIFIED));
  KZ_CHECK(!policy_from_name("whatever", &p));
}

void test_stat_parsing() {
  // "pid (comm) state ppid ... priority nice ... processor rt_priority policy ..."; field N is token N-3 after the ')'.
  // Build the tail so that field 18 = 31, field 19 = -4, field 39 = 2, field 40 = 9, field 41 = 1.
  std::vector<std::string> f(60, "0");
  f[3 - 3] = "R";
  f[18 - 3] = "31";
  f[19 - 3] = "-4";
  f[39 - 3] = "2";
  f[40 - 3] = "9";
  f[41 - 3] = "1";
  std::string text = "1234 (we ird) name) ";
  for (const std::string& t : f) text += t + " ";
  ThreadCapture tc;
  KZ_CHECK_OK(parse_stat_line(text, &tc));
  KZ_CHECK_EQ(tc.comm, std::string("we ird) name"));    // cut at the LAST ')'
  KZ_CHECK_EQ(tc.state, 'R');
  KZ_CHECK_EQ(tc.kernel_priority, 31);
  KZ_CHECK_EQ(tc.nice, -4);
  KZ_CHECK_EQ(tc.last_cpu, 2);
  KZ_CHECK_EQ(tc.rt_priority, 9);
  KZ_CHECK_EQ(tc.policy, 1);
  KZ_CHECK_EQ(std::string(policy_name(tc.policy)), std::string("SCHED_FIFO"));
  KZ_CHECK_EQ(std::string(policy_name(0)), std::string("SCHED_OTHER"));
  KZ_CHECK_EQ(std::string(policy_name(2)), std::string("SCHED_RR"));
  KZ_CHECK_EQ(std::string(policy_name(99)), std::string("unknown"));
  ThreadCapture bad;
  KZ_CHECK_ERR(parse_stat_line("no parentheses here", &bad), kz::Err::Invalid);
  KZ_CHECK_ERR(parse_stat_line("1 (x) S 1 2 3", &bad), kz::Err::Invalid);  // too few fields
}

void test_proc_of_self() {
  const uint32_t pid = static_cast<uint32_t>(getpid());
  ProcessCapture pc;
  KZ_CHECK_OK(capture_process(pid, &pc));
  KZ_CHECK(pc.ok);
  KZ_CHECK(pc.uid.find(std::to_string(getuid())) == 0);
  KZ_CHECK(!pc.cmdline.empty());
  KZ_CHECK(!pc.exe.empty());
  KZ_CHECK(!pc.cwd.empty());
  KZ_CHECK_EQ(pc.env_sha256.size(), size_t(64));
  KZ_CHECK(std::is_sorted(pc.env_names.begin(), pc.env_names.end()));
  KZ_CHECK(!pc.limits.empty());
  const std::vector<uint32_t> tids = list_threads(pid);
  KZ_CHECK(!tids.empty());
  KZ_CHECK(std::find(tids.begin(), tids.end(), pid) != tids.end());
  ThreadCapture tc;
  KZ_CHECK_OK(capture_thread(pid, pid, &tc));
  KZ_CHECK(tc.ok);
  KZ_CHECK(tc.policy >= 0);
  KZ_CHECK(!tc.cpus_allowed.empty());
  // a thread that does not exist is reported, not invented
  ThreadCapture gone;
  KZ_CHECK_ERR(capture_thread(pid, 0x7ffffff0u, &gone), kz::Err::Io);
  KZ_CHECK(!gone.ok);
  ProcessCapture none;
  KZ_CHECK_ERR(capture_process(0x7ffffff0u, &none), kz::Err::Io);
  KZ_CHECK(list_threads(0x7ffffff0u).empty());

  // Every read of /proc is counted (it is an action of the observer on the target: PERTURBATION
  // proc-capture).  Mode full reads status, cmdline, the exe and cwd links, environ, cgroup and
  // limits; mode status reads the status file only and copies nothing of the target's memory.
  KZ_CHECK_EQ(pc.reads, 7u);
  KZ_CHECK(pc.mode == ProcCaptureMode::Full);
  KZ_CHECK_EQ(tc.reads, 2u);
  KZ_CHECK_EQ(gone.reads, 1u);  // the attempt is counted too
  ProcessCapture brief;
  KZ_CHECK_OK(capture_process(pid, &brief, ProcCaptureMode::Status));
  KZ_CHECK(brief.ok);
  KZ_CHECK_EQ(brief.reads, 1u);
  KZ_CHECK_EQ(brief.uid, pc.uid);
  KZ_CHECK(brief.cmdline.empty());
  KZ_CHECK(brief.env_names.empty());
  KZ_CHECK(brief.limits.empty());
  ProcCaptureMode m = ProcCaptureMode::Full;
  KZ_CHECK(parse_proc_capture_mode("status", &m) && m == ProcCaptureMode::Status);
  KZ_CHECK(parse_proc_capture_mode("full", &m) && m == ProcCaptureMode::Full);
  KZ_CHECK(!parse_proc_capture_mode("environ", &m));
  KZ_CHECK_EQ(std::string(proc_capture_mode_name(ProcCaptureMode::Status)), std::string("status"));
}

void test_cpu_list() {
  std::vector<int> v;
  KZ_CHECK_OK(parse_cpu_list("0-3\n", &v));
  KZ_CHECK(v == std::vector<int>({0, 1, 2, 3}));
  KZ_CHECK_OK(parse_cpu_list("0,2-3,7", &v));
  KZ_CHECK(v == std::vector<int>({0, 2, 3, 7}));
  KZ_CHECK_OK(parse_cpu_list("5", &v));
  KZ_CHECK(v == std::vector<int>({5}));
  KZ_CHECK_ERR(parse_cpu_list("", &v), kz::Err::Invalid);
  KZ_CHECK_ERR(parse_cpu_list("a", &v), kz::Err::Invalid);
  KZ_CHECK_ERR(parse_cpu_list("3-1", &v), kz::Err::Invalid);
  KZ_CHECK_ERR(parse_cpu_list("1-", &v), kz::Err::Invalid);
}

void test_run_target() {
  TargetRun tr;
  KZ_CHECK_OK(run_target({"/bin/sh", "-c", "exit 7"}, &tr));
  KZ_CHECK(tr.exited);
  KZ_CHECK_EQ(tr.exit_code, 7);
  KZ_CHECK(tr.t_end_ns > tr.t_start_ns);
  KZ_CHECK(tr.pid > 0);
  KZ_CHECK_OK(run_target({"/bin/sh", "-c", "kill -TERM $$"}, &tr));
  KZ_CHECK(!tr.exited);
  KZ_CHECK_EQ(tr.term_signal, 15);
  // a command that cannot be executed is reported by the shell-convention status 127, not by a crash of the observer
  KZ_CHECK_OK(run_target({"/nonexistent/command"}, &tr));
  KZ_CHECK(tr.exited);
  KZ_CHECK_EQ(tr.exit_code, 127);
  KZ_CHECK_ERR(run_target({}, &tr), kz::Err::Invalid);
  // the command is started with the observer's environment unchanged and with only descriptors 0-2 open
  setenv("KZ_TEST_MARKER", "visible", 1);
  const std::string out = "kz_run_target_env.txt";
  KZ_CHECK_OK(run_target({"/bin/sh", "-c", "printf '%s|' \"$KZ_TEST_MARKER\" > " + out + "; ls /proc/self/fd | wc -l >> " + out}, &tr));
  std::ifstream f(out);
  std::string line;
  std::getline(f, line);
  KZ_CHECK(line.rfind("visible|", 0) == 0);
  const int fds = std::atoi(line.substr(8).c_str());
  KZ_CHECK(fds <= 5);  // 0, 1, 2, the output redirection and the directory handle of ls
  std::remove(out.c_str());
}

}  // namespace

int main() {
  test_conditions();
  test_stat_parsing();
  test_proc_of_self();
  test_cpu_list();
  test_run_target();
  return kztest::finish("test_probe_record");
}

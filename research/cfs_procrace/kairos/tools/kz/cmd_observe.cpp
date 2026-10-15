// kz observe -- run a command under the perf-sample observer and record what its probes see.
//
//   kz observe <basis.kz> <profile.kzp> -o <run.kzt> [options] -- <command> [arguments]
//
// The observer attaches uprobes by file and offset (through perf_event_open, as root) to the
// binary that the basis names, BEFORE the command starts, then starts the command exactly as
// given and waits for it.  It refuses to attach if the file on disk is not the file the basis
// was built for (SHA-256, size, build-id, and the bytes at every probe target).
//
// This command observes.  It holds, delays and releases nothing (SPEC 5.9, 4.13): the control
// functions (`kz hold`, `kz gate`) are separate.  The command is run as the caller's child, with
// the caller's environment, user and scheduling unchanged; a command that has to run as another
// user (the framework's own run script drops to a normal user) does that itself.
//
// Exit status: 0 observation complete and the command exited 0; 10 observation complete and the
// command did not exit 0 (see the record); 1 usage; 2 refused (basis, profile or binary do not
// match); 3 the probes could not be attached; 4 the container could not be written; 5 the
// observation is damaged (an undecodable stream or a ring error): the container is not written.
#include <sched.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <fcntl.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "basis.hpp"
#include "command.hpp"
#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"
#include "kz/trace/decode_match.hpp"
#include "kz/trace/decode_merge.hpp"
#include "kz/trace/identity_join.hpp"
#include "perf_session.hpp"
#include "proc_capture.hpp"
#include "run_container.hpp"
#include "run_record.hpp"
#include "run_target.hpp"

namespace {

using kz::Status;

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz observe <basis.kz> <profile.kzp> -o <run.kzt> [options] -- <command> [arguments]\n"
               "  -o FILE             the run container to write (required)\n"
               "  --record FILE       the human-readable record (default: <run.kzt>.record.txt)\n"
               "  --conditions FILE   conditions and deviations declared by the caller (tab separated; see run_record.hpp)\n"
               "  --binary PATH       the traced file, if it is not at the path recorded in the basis\n"
               "  --ring-pages N      data pages of each CPU's ring buffer, a power of two (default 512)\n"
               "  --poll-ms N         the longest the drain sleeps with data possibly waiting (default 10)\n"
               "  --drain-cpus LIST   run the drain thread on these CPUs only (default: the observer's own placement)\n"
               "  --run-ordinal N     the ordinal of this run in its series (default 0)\n"
               "  --stratum S         cold | warm | none (default none): declared by the caller, not detected\n"
               "  --check             verify basis, profile and binary, print the plan and stop (attaches nothing)\n"
               "The command follows `--`; it is started as the observer's child after the probes are attached.\n");
}

std::string trim(std::string s) {
  while (!s.empty() && (s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
  size_t i = 0;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
  return s.substr(i);
}

std::string read_text(const std::string& path) {
  std::ifstream f(path);
  if (!f) return std::string();
  std::ostringstream ss;
  ss << f.rdbuf();
  return trim(ss.str());
}

std::string cpu_set_text(const cpu_set_t& set) {
  std::string out;
  int run_start = -1;
  for (int c = 0; c <= CPU_SETSIZE; ++c) {
    const bool in = c < CPU_SETSIZE && CPU_ISSET(c, &set);
    if (in && run_start < 0) run_start = c;
    if (!in && run_start >= 0) {
      if (!out.empty()) out += ",";
      out += run_start == c - 1 ? std::to_string(run_start) : std::to_string(run_start) + "-" + std::to_string(c - 1);
      run_start = -1;
    }
  }
  return out;
}

std::string hex64(uint64_t v) {
  char b[24];
  std::snprintf(b, sizeof b, "0x%llx", static_cast<unsigned long long>(v));
  return b;
}

std::string wall_clock_utc() {
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm t;
  gmtime_r(&ts.tv_sec, &t);
  char buf[64];
  std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, ts.tv_nsec / 1000000);
  return buf;
}

// What the drain thread learns about the traced processes while they run.
struct LiveCapture {
  std::set<uint32_t> pids, tids;
  std::vector<kz::probe::ProcessCapture> processes;
  std::vector<kz::probe::ThreadCapture> threads;
  std::vector<uint64_t> thread_delay_ns;  // capture time - the time of the record that revealed the thread

  void note_thread(uint32_t pid, uint32_t tid, uint64_t record_time) {
    if (!tids.insert(tid).second) return;
    kz::probe::ThreadCapture tc;
    (void)kz::probe::capture_thread(pid, tid, &tc);
    if (!tc.ok) {  // gone before we looked: say so, with what we know
      tc.pid = pid;
      tc.tid = tid;
    }
    threads.push_back(tc);
    const uint64_t now = kz::probe::monotonic_ns();
    thread_delay_ns.push_back(now >= record_time ? now - record_time : 0);
  }
  void scan(uint16_t, const std::vector<kz::trace::PerfRecord>& recs) {
    for (const kz::trace::PerfRecord& r : recs) {
      if (r.kind == kz::trace::RecKind::Sample) {
        if (pids.insert(r.pid).second) {
          kz::probe::ProcessCapture pc;
          (void)kz::probe::capture_process(r.pid, &pc);
          pc.pid = r.pid;
          processes.push_back(pc);
          for (uint32_t t : kz::probe::list_threads(r.pid)) note_thread(r.pid, t, r.time);
        }
        note_thread(r.pid, r.tid, r.time);
      } else if (r.kind == kz::trace::RecKind::Fork && pids.count(r.pid) != 0) {
        note_thread(r.pid, r.tid, r.time);
      }
    }
  }
};

struct Options {
  std::string basis, profile, out, record, conditions, binary;
  uint32_t ring_pages = 512, poll_ms = 10, run_ordinal = 0;
  std::string drain_cpus, stratum = "none";
  bool check = false;
  std::vector<std::string> command;
};

bool parse_options(int argc, char** argv, Options* o) {
  int i = 1;
  std::vector<std::string> positional;
  for (; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--") { ++i; break; }
    auto value = [&](std::string* dst) {
      if (i + 1 >= argc) { std::fprintf(stderr, "kz observe: %s needs a value\n", a.c_str()); return false; }
      *dst = argv[++i];
      return true;
    };
    auto number = [&](uint32_t* dst) {
      std::string v;
      if (!value(&v)) return false;
      char* end = nullptr;
      const unsigned long x = std::strtoul(v.c_str(), &end, 10);
      if (end == v.c_str() || *end != '\0' || x > 0x7fffffffu) { std::fprintf(stderr, "kz observe: %s takes a number\n", a.c_str()); return false; }
      *dst = static_cast<uint32_t>(x);
      return true;
    };
    if (a == "--help" || a == "-h") { usage(stdout); std::exit(0); }
    else if (a == "-o") { if (!value(&o->out)) return false; }
    else if (a == "--record") { if (!value(&o->record)) return false; }
    else if (a == "--conditions") { if (!value(&o->conditions)) return false; }
    else if (a == "--binary") { if (!value(&o->binary)) return false; }
    else if (a == "--ring-pages") { if (!number(&o->ring_pages)) return false; }
    else if (a == "--poll-ms") { if (!number(&o->poll_ms)) return false; }
    else if (a == "--run-ordinal") { if (!number(&o->run_ordinal)) return false; }
    else if (a == "--drain-cpus") { if (!value(&o->drain_cpus)) return false; }
    else if (a == "--stratum") { if (!value(&o->stratum)) return false; }
    else if (a == "--check") o->check = true;
    else if (!a.empty() && a[0] == '-') { std::fprintf(stderr, "kz observe: unknown option %s\n", a.c_str()); return false; }
    else positional.push_back(a);
  }
  for (; i < argc; ++i) o->command.push_back(argv[i]);
  if (positional.size() != 2) { std::fprintf(stderr, "kz observe: a basis and a profile are required\n"); return false; }
  o->basis = positional[0];
  o->profile = positional[1];
  if (o->out.empty() && !o->check) { std::fprintf(stderr, "kz observe: -o <run.kzt> is required\n"); return false; }
  if (o->command.empty() && !o->check) { std::fprintf(stderr, "kz observe: the command to run follows `--`\n"); return false; }
  if (o->record.empty() && !o->out.empty()) o->record = o->out + ".record.txt";
  if (o->stratum != "cold" && o->stratum != "warm" && o->stratum != "none") { std::fprintf(stderr, "kz observe: --stratum is cold, warm or none\n"); return false; }
  return true;
}

void print_plan(const std::vector<kz::trace::SiteSpec>& sites) {
  static const char* kinds[] = {"?", "entry", "point", "ret"};
  for (const kz::trace::SiteSpec& s : sites) {
    std::printf("  site %-2u %-22s %-5s in %s  address %s  file offset %s%s\n", s.site, s.label.c_str(), kinds[s.kind <= 3 ? s.kind : 0],
                s.symbol.c_str(), hex64(s.vaddr).c_str(), hex64(s.file_offset).c_str(), s.insn_class == KZ_INSN_CLASS_EMULATED ? "  [emulated insn]" : "");
    for (const kz::trace::SlotSpec& sl : s.slots) std::printf("      slot %u: %s\n", sl.slot, sl.role.c_str());
  }
}

int run(int argc, char** argv) {
  Options opt;
  if (!parse_options(argc, argv, &opt)) { usage(stderr); return 1; }

  // 1. The inputs: profile, basis, plan.
  kz::Bytes prof;
  Status s = kz::read_whole_file(opt.profile, &prof);
  if (!s.ok()) { std::fprintf(stderr, "kz observe: %s\n", s.describe().c_str()); return 2; }
  kz::plan::Profile profile;
  s = kz::plan::parse_profile(std::string_view(reinterpret_cast<const char*>(prof.data()), prof.size()), &profile);
  if (!s.ok()) { std::fprintf(stderr, "kz observe: %s\n", s.describe().c_str()); return 2; }
  kz::plan::LoadedBasis basis;
  s = kz::plan::load_basis(opt.basis, &basis);
  if (!s.ok()) { std::fprintf(stderr, "kz observe: basis: %s\n", s.describe().c_str()); return 2; }
  std::vector<kz::trace::SiteSpec> sites;
  s = kz::plan::make_plan(profile, basis, &sites);
  if (!s.ok()) { std::fprintf(stderr, "kz observe: %s\n", s.describe().c_str()); return 2; }

  // 2. The binary: one open descriptor, hashed and verified, then probed by that same descriptor.
  const std::string exe_path = opt.binary.empty() ? basis.binary_path : opt.binary;
  const int exe_fd = open(exe_path.c_str(), O_RDONLY | O_CLOEXEC);
  if (exe_fd < 0) { std::fprintf(stderr, "kz observe: cannot open the traced binary %s: %s\n", exe_path.c_str(), std::strerror(errno)); return 2; }
  struct stat st;
  if (fstat(exe_fd, &st) != 0 || !S_ISREG(st.st_mode)) { std::fprintf(stderr, "kz observe: %s is not a regular file\n", exe_path.c_str()); return 2; }
  kz::Bytes binary(static_cast<size_t>(st.st_size));
  {
    size_t got = 0;
    while (got < binary.size()) {
      const ssize_t n = pread(exe_fd, binary.data() + got, binary.size() - got, static_cast<off_t>(got));
      if (n <= 0) { std::fprintf(stderr, "kz observe: cannot read %s\n", exe_path.c_str()); return 2; }
      got += static_cast<size_t>(n);
    }
  }
  s = kz::plan::verify_binary(binary, basis);
  if (!s.ok()) {
    std::fprintf(stderr, "kz observe: REFUSED: %s\n  (%s)\n", s.message.c_str(), exe_path.c_str());
    return 2;
  }
  if (opt.check) {
    std::printf("basis %s\nbinary %s: hash, size, build-id and the bytes at %zu targets match\nplan (%zu sites):\n", kz::to_hex(basis.container.artifact_hash()).c_str(), exe_path.c_str(),
                basis.targets.size(), sites.size());
    print_plan(sites);
    return 0;
  }

  kz::probe::RunRecord record;
  if (!opt.conditions.empty()) {
    kz::Bytes cb;
    s = kz::read_whole_file(opt.conditions, &cb);
    if (s.ok()) s = kz::probe::load_conditions(std::string(cb.begin(), cb.end()), &record);
    if (!s.ok()) { std::fprintf(stderr, "kz observe: conditions: %s\n", s.describe().c_str()); return 1; }
  }

  // 3. The observer's own conditions, before anything is started.
  utsname un;
  uname(&un);
  const std::string kernel = std::string(un.sysname) + " " + un.release + " " + un.version + " " + un.machine;
  cpu_set_t aff;
  CPU_ZERO(&aff);
  sched_getaffinity(0, sizeof aff, &aff);
  const int self_policy = sched_getscheduler(0);
  sched_param self_param = {};
  sched_getparam(0, &self_param);
  errno = 0;
  const int self_nice = getpriority(PRIO_PROCESS, 0);
  const std::string start_utc = wall_clock_utc();
  const uint64_t observer_start_ns = kz::probe::monotonic_ns();

  kz::probe::SessionConfig cfg;
  cfg.exe_path = "/proc/self/fd/" + std::to_string(exe_fd);
  cfg.sites = sites;
  cfg.ring_data_pages = opt.ring_pages;
  cfg.poll_timeout_ms = opt.poll_ms;
  if (!opt.drain_cpus.empty()) {
    s = kz::probe::parse_cpu_list(opt.drain_cpus, &cfg.drain_cpus);
    if (!s.ok()) { std::fprintf(stderr, "kz observe: --drain-cpus: %s\n", s.describe().c_str()); return 1; }
  }

  // 4. Attach, then start the command.
  kz::probe::PerfSession session;
  s = session.open(cfg);
  if (!s.ok()) { std::fprintf(stderr, "kz observe: cannot attach: %s\n", s.message.c_str()); return 3; }
  LiveCapture live;
  s = session.enable();
  if (s.ok()) s = session.start_drain([&live](uint16_t cpu, const std::vector<kz::trace::PerfRecord>& r) { live.scan(cpu, r); });
  if (!s.ok()) { std::fprintf(stderr, "kz observe: cannot start: %s\n", s.message.c_str()); return 3; }

  kz::probe::TargetRun tr;
  Status rs = kz::probe::run_target(opt.command, &tr);
  rusage self_at_exit;
  getrusage(RUSAGE_SELF, &self_at_exit);

  std::vector<kz::trace::CpuStream> streams;
  kz::trace::LayoutTable layouts;
  kz::probe::SessionStats sstats;
  const uint64_t stop_begin_ns = kz::probe::monotonic_ns();
  const Status stop_status = session.stop(&streams, &layouts, &sstats);
  const uint64_t stop_end_ns = kz::probe::monotonic_ns();
  const std::string end_utc = wall_clock_utc();
  if (!rs.ok()) { std::fprintf(stderr, "kz observe: %s\n", rs.message.c_str()); return 3; }

  // 5. Offline: decode, identity join, call matching.
  kz::trace::DecodeInput din;
  din.streams = std::move(streams);
  din.layouts = layouts;
  din.sites = sites;
  din.traced.known = true;
  din.traced.ino = st.st_ino;
  din.traced.dev_major = major(st.st_dev);
  din.traced.dev_minor = minor(st.st_dev);
  kz::trace::DecodedRun decoded;
  const Status decode_status = kz::trace::decode_run(din, &decoded);
  kz::trace::SymbolIndex symbols;
  (void)kz::trace::load_symbols(basis.container, &symbols);
  kz::trace::JoinInput jin;
  jin.events = &decoded.events;
  jin.sites = &sites;
  jin.side = &decoded.side;
  jin.symbols = &symbols;
  jin.bias_known = decoded.bias_known;
  jin.load_bias = decoded.load_bias;
  std::vector<kz::trace::ThreadRecord> threads;
  kz::trace::JoinStats jstats;
  const Status join_status = kz::trace::join_identities(jin, &threads, &jstats);
  kz::trace::MatchStats mstats;
  const std::vector<kz::trace::CallPair> pairs = kz::trace::match_calls(decoded.events, sites, &mstats);
  (void)pairs;

  // 6. The record.
  auto add = [&](const std::string& n, const std::string& v, uint8_t policy = KZ_POLICY_UNSET) { record.add(n, v, policy); };
  auto num = [](uint64_t v) { return std::to_string(v); };
  add("observer.tool", std::string("kz observe ") + KZ_TOOL_VERSION);
  add("observer.backend", "perf-sample (perf_event_open uprobe PMU type " + num(sstats.uprobe_pmu_type) + "), fidelity F0: the binary is not modified");
  add("observer.pid", num(static_cast<uint64_t>(getpid())));
  add("observer.uid", num(getuid()) + " (euid " + num(geteuid()) + ")");
  add("observer.scheduling", std::string(kz::probe::policy_name(self_policy)) + ", rt priority " + num(static_cast<uint64_t>(self_param.sched_priority)) + ", nice " + std::to_string(self_nice) +
                                 " (the command inherits this: nothing was changed before the fork)");
  add("observer.affinity", cpu_set_text(aff));
  add("observer.drain_thread_cpus", opt.drain_cpus.empty() ? "unchanged (the observer's own placement)" : opt.drain_cpus);
  add("observer.cpu_ns.process_at_target_exit", num(static_cast<uint64_t>(self_at_exit.ru_utime.tv_sec + self_at_exit.ru_stime.tv_sec) * 1000000000ull +
                                                     static_cast<uint64_t>(self_at_exit.ru_utime.tv_usec + self_at_exit.ru_stime.tv_usec) * 1000ull));
  add("observer.cpu_ns.drain_thread", num(sstats.drain_cpu_ns));
  add("observer.drain_thread_last_cpu", std::to_string(sstats.drain_last_cpu));
  add("tool.ring.data_pages_per_cpu", num(opt.ring_pages), KZ_POLICY_UNSPECIFIED);
  add("tool.drain.poll_timeout_ms", num(opt.poll_ms), KZ_POLICY_UNSPECIFIED);
  add("tool.clock", sstats.clock + " (use_clockid): one clock for all CPUs; evidence only, never an HB edge");
  add("host.kernel", kernel);
  add("host.online_cpus", read_text("/sys/devices/system/cpu/online"));
  add("host.clocksource", read_text("/sys/devices/system/clocksource/clocksource0/current_clocksource"));
  add("host.perf_event_paranoid", read_text("/proc/sys/kernel/perf_event_paranoid"));
  add("host.perf_event_max_sample_rate", read_text("/proc/sys/kernel/perf_event_max_sample_rate"));
  add("host.perf_cpu_time_max_percent", read_text("/proc/sys/kernel/perf_cpu_time_max_percent"));
  add("host.perf_event_mlock_kb", read_text("/proc/sys/kernel/perf_event_mlock_kb"));
  add("binary.path", exe_path);
  add("binary.sha256", kz::to_hex(kz::sha256(binary.data(), binary.size())));
  add("binary.build_id", basis.build_id);
  add("binary.size", num(binary.size()));
  add("binary.inode", num(st.st_ino) + " on device " + num(major(st.st_dev)) + ":" + num(minor(st.st_dev)));
  add("binary.verified_against_basis", "size, SHA-256, build-id and the bytes at " + num(basis.targets.size()) + " targets");
  add("basis.artifact_sha256", kz::to_hex(basis.container.artifact_hash()));
  add("profile.sha256", kz::to_hex(profile.sha256));
  add("plan.sites", num(sites.size()));
  {
    std::string cmd;
    for (const std::string& a : opt.command) cmd += (cmd.empty() ? "" : " ") + a;
    add("target.command_line", cmd);
  }
  {
    char cwd[4096];
    add("target.start_directory", getcwd(cwd, sizeof cwd) != nullptr ? cwd : "?");
  }
  add("target.exit", tr.exited ? "exited " + std::to_string(tr.exit_code) : "killed by signal " + std::to_string(tr.term_signal));
  add("target.forwarded_signal", std::to_string(tr.forwarded_signal));
  add("target.wall_ns", num(tr.t_end_ns - tr.t_start_ns));
  // After the command has exited: the probes are removed one event at a time, and the kernel makes
  // each removal wait for a grace period (measured at about 0.1 s per uprobe event with strace).
  // This time is spent when the traced command no longer runs.
  add("observer.stop_ns", num(stop_end_ns - stop_begin_ns));
  add("target.child_cpu_ms", num(static_cast<uint64_t>(tr.usage.ru_utime.tv_sec + tr.usage.ru_stime.tv_sec) * 1000 +
                                  static_cast<uint64_t>(tr.usage.ru_utime.tv_usec + tr.usage.ru_stime.tv_usec) / 1000));
  for (const kz::probe::ProcessCapture& p : live.processes) {
    const std::string pre = "traced.pid" + num(p.pid) + ".";
    if (!p.ok) { add(pre + "capture", "the process was gone before the observer could read /proc"); continue; }
    add(pre + "uid", p.uid);
    add(pre + "gid", p.gid);
    add(pre + "groups", p.groups);
    add(pre + "cmdline", p.cmdline);
    add(pre + "exe", p.exe);
    add(pre + "cwd", p.cwd);
    add(pre + "capabilities", "eff " + p.cap_eff + ", prm " + p.cap_prm + ", bnd " + p.cap_bnd + ", no_new_privs " + p.no_new_privs + ", seccomp " + p.seccomp);
    add(pre + "cpus_allowed", p.cpus_allowed);
    add(pre + "cgroup", p.cgroup);
    {
      std::string names;
      for (const std::string& n : p.env_names) names += (names.empty() ? "" : " ") + n;
      add(pre + "environment.names", names);
      add(pre + "environment.sha256_of_block", p.env_sha256 + " (" + num(p.env_bytes) + " bytes; values are not recorded)");
    }
    for (const std::string& l : p.limits) add(pre + "limit", l);
  }
  for (size_t i = 0; i < live.threads.size(); ++i) {
    const kz::probe::ThreadCapture& t = live.threads[i];
    if (!t.ok) {
      add("traced.thread." + num(t.tid), "gone before the observer could read /proc (seen " + num(live.thread_delay_ns[i]) + " ns after its record)");
      continue;
    }
    add("traced.thread." + num(t.tid), "comm=" + t.comm + " policy=" + kz::probe::policy_name(t.policy) + " rt_priority=" + std::to_string(t.rt_priority) +
                                           " nice=" + std::to_string(t.nice) + " kernel_priority=" + std::to_string(t.kernel_priority) + " last_cpu=" + std::to_string(t.last_cpu) +
                                           " cpus_allowed=" + t.cpus_allowed + " read_after_record_ns=" + num(live.thread_delay_ns[i]));
  }
  for (const kz::probe::CpuStats& c : sstats.cpus)
    add("ring.cpu" + std::to_string(c.cpu), "ring " + num(c.ring_bytes) + " bytes, drained " + num(c.bytes_drained) + " bytes, most unread bytes seen at a drain " + num(c.high_water_bytes));
  add("drain.rounds", num(sstats.drain_rounds));
  add("loss.perf_record_lost.reports", num(decoded.stats.lost_reports));
  // The kernel counts every record it could not write -- probe samples and the side-band records of any process -- because they share the ring.
  add("loss.perf_record_lost.dropped_records", num(decoded.stats.dropped_records));
  add("loss.perf_throttle.intervals", num(decoded.stats.throttle_intervals));
  add("loss.stream_errors", num(decoded.stats.stream_errors));
  add("decode.records", num(decoded.stats.parse.records));
  add("decode.samples", num(decoded.stats.parse.samples));
  add("decode.raw_events", num(decoded.events.size()));
  add("decode.records_not_interpreted", num(decoded.stats.parse.other));
  add("decode.samples_without_site", num(decoded.stats.samples_without_site));
  add("decode.slot_failures", num(decoded.stats.slot_failures));
  add("decode.events_with_unreadable_slot", num(decoded.stats.slots_truncated));
  add("decode.side_events_of_other_processes_dropped", num(decoded.stats.side_dropped));
  add("decode.mappings_of_other_files_dropped", num(decoded.stats.maps_dropped));
  add("decode.load_bias", decoded.bias_known ? hex64(decoded.load_bias) + " (mismatches " + num(decoded.stats.bias_mismatches) + ")" : "unknown (no entry or point sample)");
  {
    std::string pids;
    for (uint32_t p : decoded.traced_pids) pids += (pids.empty() ? "" : ",") + num(p);
    add("decode.traced_pids", pids);
  }
  for (size_t i = 0; i < sites.size(); ++i) add("hits.site" + num(i) + "." + sites[i].label, num(decoded.stats.site_hits[i]));
  add("match.call_pairs", num(mstats.pairs));
  add("match.entries_without_return", num(mstats.entries_without_return));
  add("match.returns_without_entry", num(mstats.returns_without_entry));
  add("identity.threads", num(jstats.threads));
  add("identity.spawn_matched", num(jstats.spawn_matched));
  add("identity.spawn_ambiguous", num(jstats.spawn_ambiguous));
  add("identity.creates_without_fork", num(jstats.creates_without_fork));
  add("identity.creates_without_return", num(jstats.creates_without_return));
  add("identity.returns_without_create", num(jstats.returns_without_create));
  add("identity.entries_unmatched", num(jstats.entries_unmatched));
  add("identity.threads_without_name", num(jstats.threads_without_name));
  add("identity.names_after_first_event", num(jstats.names_after_first_event));
  add("identity.events_without_thread", num(jstats.events_without_thread));

  uint64_t boot_ns = 0;
  {
    uint64_t first = 0;
    for (const kz::trace::MapEvent& m : decoded.maps)
      if (first == 0 || m.rec.t_ns < first) first = m.rec.t_ns;
    uint64_t last = 0;
    for (const kz_raw_ev& e : decoded.events) last = std::max<uint64_t>(last, e.t_ns);
    if (first != 0 && last > first) boot_ns = last - first;
  }
  add("trace.first_exec_mapping_to_last_event_ns", num(boot_ns));
  add("trace.window", "observer attached " + num(tr.t_start_ns - observer_start_ns) + " ns before the command started");

  kz::probe::RunInputs ri;
  ri.basis_artifact = basis.container.artifact_hash();
  ri.profile_sha256 = profile.sha256;
  ri.tool_hash = KZ_THIS_TOOL_HASH("kz");
  ri.sites = sites;
  ri.exe_path = exe_path;
  ri.inode = st.st_ino;
  ri.device = st.st_dev;
  kz::probe::build_id_from_hex(basis.build_id, ri.build_id);
  ri.exe_size = binary.size();
  ri.exe_sha256 = kz::sha256(binary.data(), binary.size());
  ri.decoded = decoded;
  ri.threads = threads;
  ri.session = sstats;
  ri.record = record;
  ri.run_ordinal = opt.run_ordinal;
  ri.stratum = opt.stratum == "cold" ? KZ_STRATUM_COLD : opt.stratum == "warm" ? KZ_STRATUM_WARM : KZ_STRATUM_NONE;
  ri.kernel = kernel;
  ri.harness = std::string("kz observe ") + KZ_TOOL_VERSION + " tool " + kz::to_hex(KZ_THIS_TOOL_HASH("kz"));
  ri.placement = "observer affinity " + cpu_set_text(aff) + "; drain thread " + (opt.drain_cpus.empty() ? "unpinned" : "pinned to " + opt.drain_cpus) + ", last on CPU " + std::to_string(sstats.drain_last_cpu);
  ri.observer_cpu_ns = sstats.drain_cpu_ns;
  ri.boot_ns = boot_ns;
  if (!tr.exited || tr.exit_code != 0) ri.run_flags |= KZ_RUN_FLAG_ABNORMAL_EXIT;

  const bool damaged = !stop_status.ok() || !decode_status.ok() || !join_status.ok();
  int code = 0;
  kz::Bytes container;
  if (damaged) {
    std::fprintf(stderr, "kz observe: the observation is damaged:%s%s%s\n", stop_status.ok() ? "" : (" ring: " + stop_status.message + ";").c_str(),
                 decode_status.ok() ? "" : (" decode: " + decode_status.message + ";").c_str(), join_status.ok() ? "" : (" join: " + join_status.message + ";").c_str());
    code = 5;
  } else {
    Status ws = kz::probe::write_run_container(ri, &container);
    if (ws.ok()) ws = kz::write_whole_file(opt.out, container);
    if (!ws.ok()) { std::fprintf(stderr, "kz observe: %s\n", ws.describe().c_str()); code = 4; }
    else code = (tr.exited && tr.exit_code == 0) ? 0 : 10;
  }

  // The human-readable record: the same items, plus wall-clock times, which no container holds.
  {
    std::ostringstream o;
    o << "kz observe record\n";
    o << "observation started (UTC) " << start_utc << ", ended " << end_utc << "\n";
    o << "run container " << opt.out << (container.empty() ? " (not written)" : "") << "\n";
    if (!container.empty()) o << "run container artifact_sha256 " << kz::to_hex(kz::sha256(container.data(), container.size())) << "\n";
    o << "exit status of kz observe " << code << "\n\n";
    o << record.text();
    o << "\nthreads (identity join):\n";
    for (const kz::trace::ThreadRecord& t : threads) {
      o << "  pid " << t.rec.pid << " tid " << t.rec.tid << " gen " << t.rec.generation << " name '" << t.name << "' creator tid "
        << (t.rec.ptid == KZ_NONE32 ? std::string("-") : std::to_string(t.rec.ptid)) << " announced id "
        << (t.rec.announced_id == KZ_NONE32 ? std::string("-") : hex64(t.rec.announced_id)) << " entry " << (t.entry_symbol.empty() ? "-" : t.entry_symbol) << " events " << t.rec.n_events << " flags 0x"
        << std::hex << t.rec.flags << std::dec << "\n";
    }
    if (!opt.record.empty()) {
      std::ofstream f(opt.record);
      f << o.str();
    }
  }
  std::fprintf(stderr, "kz observe: %zu raw events at %zu sites, %zu threads; dropped %llu records, %llu throttle intervals; command %s; record %s\n", decoded.events.size(), sites.size(), threads.size(),
               static_cast<unsigned long long>(decoded.stats.dropped_records), static_cast<unsigned long long>(decoded.stats.throttle_intervals),
               tr.exited ? ("exited " + std::to_string(tr.exit_code)).c_str() : ("killed by signal " + std::to_string(tr.term_signal)).c_str(), opt.record.c_str());
  return code;
}

}  // namespace

KZ_COMMAND(observe, "run a command under the perf-sample observer and record the probe events", run);

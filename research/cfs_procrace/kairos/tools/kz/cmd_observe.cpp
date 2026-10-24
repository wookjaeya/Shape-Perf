// kz observe -- run a command under the perf-sample observer and record what its probes see.
//
//   kz observe <basis.kz> <profile.kzp> -o <run.kzt> [--basis <basis2.kz>]... [options] -- <command> [arguments]
//
// A profile over several files (several `binary` statements: the executable and shared objects the
// process loads) needs one basis per file: the first argument is the primary's, --basis gives the
// others.  Every file is opened once, verified against its basis and probed through that descriptor;
// the uprobes are registered by inode before the command starts, so a shared object the process maps
// later is probed when it is mapped.
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
               "  --basis FILE        the basis of a further file of a plan over several files (repeat per file)\n"
               "  --ring-pages N      data pages of each CPU's ring buffer, a power of two (default 512)\n"
               "  --poll-ms N         the longest the drain sleeps with data possibly waiting (default 10)\n"
               "  --drain-cpus LIST   run the drain thread on these CPUs only (default: the observer's own placement)\n"
               "  --run-ordinal N     the ordinal of this run in its series (default 0)\n"
               "  --stratum S         cold | warm | none (default none): declared by the caller, not detected; give\n"
               "                      cold or warm only when the procedure established it (otherwise none, and say why\n"
               "                      in --conditions)\n"
               "  --proc-capture M    full | status (default full): what the observer reads from /proc while the\n"
               "                      command runs; full also reads cmdline and environ (copies of the target's memory)\n"
               "  --tracefs DIR       a mounted tracefs (with uprobe_events), needed by sites that read memory (fetch\n"
               "                      slots); default /sys/kernel/tracing if it has uprobe_events.  The observer creates\n"
               "                      one probe event per such site there and removes it when the session closes\n"
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

// What the drain thread learns about the traced processes while they run.  These /proc reads are
// actions of the observer on the running target: they are counted and timed (`reads`, `ns`, the
// first and last read) for PERTURBATION kind proc-capture (PI decision D1).
struct LiveCapture {
  kz::probe::ProcCaptureMode mode = kz::probe::ProcCaptureMode::Full;
  std::set<uint32_t> pids, tids;
  std::vector<kz::probe::ProcessCapture> processes;
  std::vector<kz::probe::ThreadCapture> threads;
  std::vector<uint64_t> thread_delay_ns;  // capture time - the time of the record that revealed the thread
  uint64_t reads = 0, ns = 0, first_ns = 0, last_ns = 0;  // CLOCK_MONOTONIC

  void account(uint64_t begin, uint64_t end, uint64_t n) {
    reads += n;
    ns += end >= begin ? end - begin : 0;
    if (first_ns == 0 || begin < first_ns) first_ns = begin;
    last_ns = std::max(last_ns, end);
  }
  void note_thread(uint32_t pid, uint32_t tid, uint64_t record_time) {
    if (!tids.insert(tid).second) return;
    kz::probe::ThreadCapture tc;
    const uint64_t begin = kz::probe::monotonic_ns();
    (void)kz::probe::capture_thread(pid, tid, &tc);
    const uint64_t now = kz::probe::monotonic_ns();
    account(begin, now, tc.reads);
    if (!tc.ok) {  // gone before we looked: say so, with what we know
      tc.pid = pid;
      tc.tid = tid;
    }
    threads.push_back(tc);
    thread_delay_ns.push_back(now >= record_time ? now - record_time : 0);
  }
  void scan(uint16_t, const std::vector<kz::trace::PerfRecord>& recs) {
    for (const kz::trace::PerfRecord& r : recs) {
      if (r.kind == kz::trace::RecKind::Sample) {
        if (pids.insert(r.pid).second) {
          kz::probe::ProcessCapture pc;
          const uint64_t begin = kz::probe::monotonic_ns();
          (void)kz::probe::capture_process(r.pid, &pc, mode);
          const std::vector<uint32_t> tids_now = kz::probe::list_threads(r.pid);
          account(begin, kz::probe::monotonic_ns(), pc.reads + 1);
          pc.pid = r.pid;
          processes.push_back(pc);
          for (uint32_t t : tids_now) note_thread(r.pid, t, r.time);
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
  std::string drain_cpus, stratum = "none", proc_capture = "full", tracefs;
  bool check = false;
  std::vector<std::string> command;
  std::vector<std::string> also_bases;
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
    else if (a == "--basis") { std::string v; if (!value(&v)) return false; o->also_bases.push_back(v); }
    else if (a == "--ring-pages") { if (!number(&o->ring_pages)) return false; }
    else if (a == "--poll-ms") { if (!number(&o->poll_ms)) return false; }
    else if (a == "--run-ordinal") { if (!number(&o->run_ordinal)) return false; }
    else if (a == "--drain-cpus") { if (!value(&o->drain_cpus)) return false; }
    else if (a == "--stratum") { if (!value(&o->stratum)) return false; }
    else if (a == "--proc-capture") { if (!value(&o->proc_capture)) return false; }
    else if (a == "--tracefs") { if (!value(&o->tracefs)) return false; }
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
  kz::probe::ProcCaptureMode m;
  if (!kz::probe::parse_proc_capture_mode(o->proc_capture, &m)) { std::fprintf(stderr, "kz observe: --proc-capture is full or status\n"); return false; }
  return true;
}

void print_plan(const std::vector<kz::trace::SiteSpec>& sites) {
  static const char* kinds[] = {"?", "entry", "point", "ret"};
  for (const kz::trace::SiteSpec& s : sites) {
    std::printf("  site %-2u %-22s %-5s in %s  address %s  file offset %s%s%s\n", s.site, s.label.c_str(), kinds[s.kind <= 3 ? s.kind : 0],
                s.symbol.c_str(), hex64(s.vaddr).c_str(), hex64(s.file_offset).c_str(), s.insn_class == KZ_INSN_CLASS_EMULATED ? "  [emulated insn]" : "",
                s.binary != 0 ? ("  [file " + std::to_string(s.binary) + "]").c_str() : "");
    if (!s.via_symbol.empty())
      std::printf("      reached by %s; kept where %s\n", s.via_chain.c_str(),
                  s.via_ret_dynamic ? ("the stack pointer is the one at the entry of the open call of " + s.via_symbol + " (tail jump)").c_str()
                                    : ("the return address is " + hex64(s.via_expect_ret) + " (link-time), inside an open call of " + s.via_symbol).c_str());
    for (const kz::trace::SlotSpec& sl : s.slots) {
      if (kz::trace::is_fetch_slot(sl))
        std::printf("      slot %u: %s (%s at %+d(%s), probe-event fetch argument)\n", sl.slot, sl.role.c_str(), sl.src == KZ_SLOT_SRC_STR ? "string" : "memory", sl.offset,
                    [&] {
                      static const char* const regs[16] = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
                      return sl.index < 16 ? regs[sl.index] : "?";
                    }());
      else
        std::printf("      slot %u: %s\n", sl.slot, sl.role.c_str());
    }
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
  std::vector<kz::plan::LoadedBasis> also_bases(opt.also_bases.size());
  for (size_t i = 0; i < opt.also_bases.size(); ++i) {
    s = kz::plan::load_basis(opt.also_bases[i], &also_bases[i]);
    if (!s.ok()) { std::fprintf(stderr, "kz observe: basis %s: %s\n", opt.also_bases[i].c_str(), s.describe().c_str()); return 2; }
  }
  std::vector<const kz::plan::LoadedBasis*> all_bases{&basis};
  for (const kz::plan::LoadedBasis& b : also_bases) all_bases.push_back(&b);
  std::vector<kz::trace::SiteSpec> sites;
  s = kz::plan::make_plan(profile, all_bases, &sites);
  if (!s.ok()) { std::fprintf(stderr, "kz observe: %s\n", s.describe().c_str()); return 2; }
  if (profile.binaries.size() > 1 && basis.binary_name != profile.binaries[0]) {
    std::fprintf(stderr, "kz observe: the first basis must be the primary binary's ('%s'); it is for '%s'\n", profile.binaries[0].c_str(), basis.binary_name.c_str());
    return 2;
  }

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
  // The further files: each opened once, verified against its basis, probed through that descriptor.
  struct AlsoFile {
    const kz::plan::LoadedBasis* basis = nullptr;
    std::string path;
    int fd = -1;
    struct stat st = {};
    kz::Digest sha256 = {};
    uint64_t size = 0;
  };
  std::vector<AlsoFile> also_files;
  for (size_t b = 1; b < profile.binaries.size(); ++b) {
    AlsoFile af;
    for (const kz::plan::LoadedBasis& lb : also_bases)
      if (lb.binary_name == profile.binaries[b]) af.basis = &lb;
    af.path = af.basis->binary_path;
    af.fd = open(af.path.c_str(), O_RDONLY | O_CLOEXEC);
    if (af.fd < 0) { std::fprintf(stderr, "kz observe: cannot open the traced file %s: %s\n", af.path.c_str(), std::strerror(errno)); return 2; }
    if (fstat(af.fd, &af.st) != 0 || !S_ISREG(af.st.st_mode)) { std::fprintf(stderr, "kz observe: %s is not a regular file\n", af.path.c_str()); return 2; }
    kz::Bytes bytes(static_cast<size_t>(af.st.st_size));
    size_t got = 0;
    while (got < bytes.size()) {
      const ssize_t n = pread(af.fd, bytes.data() + got, bytes.size() - got, static_cast<off_t>(got));
      if (n <= 0) { std::fprintf(stderr, "kz observe: cannot read %s\n", af.path.c_str()); return 2; }
      got += static_cast<size_t>(n);
    }
    s = kz::plan::verify_binary(bytes, *af.basis);
    if (!s.ok()) {
      std::fprintf(stderr, "kz observe: REFUSED: %s\n  (%s)\n", s.message.c_str(), af.path.c_str());
      return 2;
    }
    af.sha256 = kz::sha256(bytes.data(), bytes.size());
    af.size = bytes.size();
    also_files.push_back(af);
  }
  if (opt.check) {
    std::printf("basis %s\nbinary %s: hash, size, build-id and the bytes at %zu targets match\n", kz::to_hex(basis.container.artifact_hash()).c_str(), exe_path.c_str(), basis.targets.size());
    for (const AlsoFile& af : also_files)
      std::printf("basis %s\nbinary %s: hash, size, build-id and the bytes at %zu targets match\n", kz::to_hex(af.basis->container.artifact_hash()).c_str(), af.path.c_str(),
                  af.basis->targets.size());
    std::printf("plan (%zu sites):\n", sites.size());
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
  for (const AlsoFile& af : also_files) cfg.also_paths.push_back("/proc/self/fd/" + std::to_string(af.fd));
  cfg.sites = sites;
  cfg.ring_data_pages = opt.ring_pages;
  cfg.poll_timeout_ms = opt.poll_ms;
  cfg.tracefs = opt.tracefs;
  bool any_fetch = false;
  for (const kz::trace::SiteSpec& site : sites) any_fetch = any_fetch || kz::trace::has_fetch_slots(site);
  if (any_fetch && cfg.tracefs.empty() && access("/sys/kernel/tracing/uprobe_events", W_OK) == 0) cfg.tracefs = "/sys/kernel/tracing";
  if (any_fetch && cfg.tracefs.empty()) {
    std::fprintf(stderr, "kz observe: the plan reads memory at %s: give a mounted tracefs with --tracefs DIR (/sys/kernel/tracing has no writable uprobe_events)\n",
                 "some site (fetch slots)");
    return 3;
  }
  if (!opt.drain_cpus.empty()) {
    s = kz::probe::parse_cpu_list(opt.drain_cpus, &cfg.drain_cpus);
    if (!s.ok()) { std::fprintf(stderr, "kz observe: --drain-cpus: %s\n", s.describe().c_str()); return 1; }
  }

  // 4. Attach, then start the command.
  kz::probe::PerfSession session;
  s = session.open(cfg);
  if (!s.ok()) { std::fprintf(stderr, "kz observe: cannot attach: %s\n", s.message.c_str()); return 3; }
  LiveCapture live;
  (void)kz::probe::parse_proc_capture_mode(opt.proc_capture, &live.mode);
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
  for (const AlsoFile& af : also_files) {
    kz::trace::TracedFile tf;
    tf.known = true;
    tf.ino = af.st.st_ino;
    tf.dev_major = major(af.st.st_dev);
    tf.dev_minor = minor(af.st.st_dev);
    din.also.push_back(tf);
  }
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
  std::vector<kz::trace::OwnerRecord> owners;
  kz::trace::JoinStats jstats;
  const Status join_status = kz::trace::join_identities(jin, &threads, &jstats, &owners);
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
  if (!sstats.fetch_events.empty()) {
    add("observer.fetch", "tracefs " + cfg.tracefs + ": " + num(sstats.fetch_events.size()) +
                              " probe event(s) of uprobe_events with fetch arguments (memory read by the kernel when the probe fires), opened as tracepoint events "
                              "with PERF_SAMPLE_RAW; " + (sstats.fetch_cleanup.empty() ? std::string("not removed") : sstats.fetch_cleanup));
    for (const kz::probe::FetchEvent& fe : sstats.fetch_events) add("observer.fetch.site" + num(fe.site), fe.name + " id " + num(fe.id) + ": " + fe.definition);
  }
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
  for (size_t i = 0; i < also_files.size(); ++i) {
    const AlsoFile& af = also_files[i];
    const std::string pre = "binary.also" + num(i + 1) + ".";
    add(pre + "name", af.basis->binary_name);
    add(pre + "path", af.path);
    add(pre + "sha256", kz::to_hex(af.sha256));
    add(pre + "build_id", af.basis->build_id);
    add(pre + "size", num(af.size));
    add(pre + "inode", num(af.st.st_ino) + " on device " + num(major(af.st.st_dev)) + ":" + num(minor(af.st.st_dev)));
    add(pre + "verified_against_basis", "size, SHA-256, build-id and the bytes at " + num(af.basis->targets.size()) + " targets");
    add(pre + "basis_artifact_sha256", kz::to_hex(af.basis->container.artifact_hash()));
    add(pre + "load_bias", i + 1 < decoded.file_bias_known.size() && decoded.file_bias_known[i + 1] ? hex64(decoded.file_bias[i + 1])
                                                                                                    : std::string("unknown (no entry or point sample of this file)"));
  }
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
  add("observer.proc_capture.mode", std::string(kz::probe::proc_capture_mode_name(live.mode)) +
                                        (live.mode == kz::probe::ProcCaptureMode::Full ? ": status, cmdline, environ, exe, cwd, cgroup, limits of each traced process"
                                                                                        : ": status of each traced process") +
                                        "; stat and status of each thread; read by the drain thread while the command runs");
  add("observer.proc_capture.reads", num(live.reads));
  add("observer.proc_capture.ns", num(live.ns) + " (wall time in the reads)");
  add("observer.proc_capture.window",
      live.reads == 0 ? std::string("none")
                      : "from " + std::to_string(static_cast<int64_t>(live.first_ns) - static_cast<int64_t>(tr.t_start_ns)) + " ns to " +
                            std::to_string(static_cast<int64_t>(live.last_ns) - static_cast<int64_t>(tr.t_start_ns)) + " ns after the command started");
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
  for (size_t i = 0; i < sites.size(); ++i)
    if (!sites[i].via_symbol.empty())
      add("decode.via.site" + num(i) + "." + sites[i].label, num(decoded.stats.site_hits[i] - decoded.stats.via_unattributed[i]) + " kept (attributed to an open call of " +
                                                                 sites[i].via_symbol + " by the return address), " + num(decoded.stats.via_unattributed[i]) +
                                                                 " not kept (other callers, or no open call)");
  add("decode.strings", num(decoded.strings.size()) + " string value(s) of fetch slots");
  for (size_t i = 0; i < sites.size(); ++i) add("perturbation.site" + num(i) + ".trap", kz::probe::trap_account_text(sites, i));
  add("match.call_pairs", num(mstats.pairs));
  add("match.entries_without_return", num(mstats.entries_without_return));
  add("match.returns_without_entry", num(mstats.returns_without_entry));
  add("match.events_with_unreadable_key", num(mstats.events_with_unreadable_key));
  add("match.site_pairs_without_key", num(mstats.sites_without_key));
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
  add("identity.creates_unreadable", num(jstats.creates_unreadable));
  add("identity.returns_unreadable", num(jstats.returns_unreadable));
  add("identity.entries_unreadable", num(jstats.entries_unreadable));
  add("identity.matched_by_time", num(jstats.matched_by_time) +
                                      " (the creator link of these records is FORK-time containment in the creator's call: one thread's timestamps, under A-CLK-PO)");
  add("identity.ids_unverified", num(jstats.ids_unverified) +
                                     " (announced ids not compared with the creator side: no spawn-id site of the plan read the id the spawn call assigned)");
  add("identity.ids_matched", num(jstats.ids_matched) + " (announced id equal to the id the spawn call assigned: the creator link rests on the id)");
  add("identity.ids_mismatched", num(jstats.ids_mismatched));
  add("identity.spawn_ids_outside_create", num(jstats.spawn_ids_outside_create));
  add("identity.spawn_ids_unreadable", num(jstats.spawn_ids_unreadable) + ", repeated in one call " + num(jstats.spawn_ids_repeated));
  add("identity.requests", num(jstats.requests) + " (creation requests; unreadable " + num(jstats.requests_unreadable) + ", without return " + num(jstats.requests_without_return) +
                               ", returns without request " + num(jstats.request_returns_without_request) + ")");
  add("identity.app_owner", "confirmed " + num(jstats.owners_confirmed) + ", inferred " + num(jstats.owners_inferred) + ", undetermined " + num(jstats.owners_undetermined) +
                                " (from the creation side only: request argument or creator's owner, spawn-assigned id against the announced id; no framework lookup)");

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
  for (const AlsoFile& af : also_files) {
    kz::probe::FileIdentity fi;
    fi.path = af.path;
    fi.inode = af.st.st_ino;
    fi.device = af.st.st_dev;
    kz::probe::build_id_from_hex(af.basis->build_id, fi.build_id);
    fi.size = af.size;
    fi.sha256 = af.sha256;
    ri.also.push_back(fi);
  }
  ri.decoded = decoded;
  ri.threads = threads;
  ri.owners = owners;
  ri.session = sstats;
  ri.record = record;
  ri.run_ordinal = opt.run_ordinal;
  ri.stratum = opt.stratum == "cold" ? KZ_STRATUM_COLD : opt.stratum == "warm" ? KZ_STRATUM_WARM : KZ_STRATUM_NONE;
  ri.kernel = kernel;
  ri.harness = std::string("kz observe ") + KZ_TOOL_VERSION + " tool " + kz::to_hex(KZ_THIS_TOOL_HASH("kz"));
  ri.placement = "observer affinity " + cpu_set_text(aff) + "; drain thread " + (opt.drain_cpus.empty() ? "unpinned" : "pinned to " + opt.drain_cpus) + ", last on CPU " + std::to_string(sstats.drain_last_cpu);
  ri.observer_cpu_ns = sstats.drain_cpu_ns;
  ri.boot_ns = boot_ns;
  ri.proc_reads = live.reads;
  ri.proc_ns = live.ns;
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
    for (size_t ti = 0; ti < threads.size(); ++ti) {
      const kz::trace::ThreadRecord& t = threads[ti];
      o << "  pid " << t.rec.pid << " tid " << t.rec.tid << " gen " << t.rec.generation << " name '" << t.name << "' creator tid "
        << (t.rec.ptid == KZ_NONE32 ? std::string("-") : std::to_string(t.rec.ptid)) << " announced id "
        << (t.rec.announced_id == KZ_NONE32 ? std::string("-") : hex64(t.rec.announced_id)) << " entry " << (t.entry_symbol.empty() ? "-" : t.entry_symbol) << " events " << t.rec.n_events << " flags 0x"
        << std::hex << t.rec.flags << std::dec;
      if (ti < owners.size()) {
        const kz_app_owner& ow = owners[ti].rec;
        static const char* const kLevel[] = {"undetermined", "inferred", "confirmed"};
        o << " app_owner " << (ow.level <= 2 ? kLevel[ow.level] : "?") << " key " << (ow.owner_key == KZ_NONE32 ? std::string("-") : hex64(ow.owner_key)) << " evidence 0x" << std::hex << ow.evidence
          << std::dec << (owners[ti].app_name.empty() ? std::string() : " name '" + owners[ti].app_name + "'");
      }
      o << "\n";
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

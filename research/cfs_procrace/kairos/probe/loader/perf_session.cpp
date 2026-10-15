// perf_session.cpp -- see perf_session.hpp.
#include "perf_session.hpp"

#include <errno.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace kz::probe {

using kz::trace::CpuStream;
using kz::trace::EventLayout;
using kz::trace::LayoutTable;
using kz::trace::PerfNeeds;
using kz::trace::SiteSpec;
namespace pa = kz::trace::perfabi;

namespace {

long perf_event_open(struct perf_event_attr* attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

std::string errno_text(int e) { return std::string(std::strerror(e)) + " (errno " + std::to_string(e) + ")"; }

Status read_small_file(const std::string& path, std::string* out) {
  std::ifstream f(path);
  if (!f) return Status::error(Err::Io, "cannot read " + path);
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  return Status::success();
}

// The retprobe bit of the uprobe PMU's `config`, from its format file ("config:0").
Status uprobe_retprobe_bit(unsigned* bit) {
  std::string text;
  Status s = read_small_file("/sys/bus/event_source/devices/uprobe/format/retprobe", &text);
  if (!s.ok()) return Status::error(Err::Io, "the kernel has no uprobe PMU format 'retprobe' (/sys/bus/event_source/devices/uprobe/format/retprobe)");
  unsigned b = 0;
  if (std::sscanf(text.c_str(), "config:%u", &b) != 1 || b > 63) return Status::error(Err::Invalid, "unexpected uprobe PMU retprobe format: " + text);
  *bit = b;
  return Status::success();
}

}  // namespace

Status parse_cpu_list(const std::string& text, std::vector<int>* out) {
  out->clear();
  size_t i = 0;
  const size_t n = text.size();
  auto number = [&](int* v) {
    if (i >= n || text[i] < '0' || text[i] > '9') return false;
    long x = 0;
    while (i < n && text[i] >= '0' && text[i] <= '9') {
      x = x * 10 + (text[i] - '0');
      if (x > 100000) return false;
      ++i;
    }
    *v = static_cast<int>(x);
    return true;
  };
  while (i < n) {
    if (text[i] == '\n' || text[i] == ' ') { ++i; continue; }
    int a = 0, b = 0;
    if (!number(&a)) return Status::error(Err::Invalid, "cpu list: a number was expected in '" + text + "'");
    b = a;
    if (i < n && text[i] == '-') {
      ++i;
      if (!number(&b) || b < a) return Status::error(Err::Invalid, "cpu list: bad range in '" + text + "'");
    }
    for (int c = a; c <= b; ++c) out->push_back(c);
    if (i < n && text[i] == ',') ++i;
  }
  if (out->empty()) return Status::error(Err::Invalid, "cpu list is empty");
  return Status::success();
}

struct PerfSession::Impl {
  struct PerCpu {
    int cpu = 0;
    int owner_fd = -1;            // the side-band event; owns the ring
    std::vector<int> site_fds;    // one per site
    void* map = nullptr;
    size_t map_len = 0;
    uint8_t* data = nullptr;
    uint64_t data_size = 0;
    std::vector<uint8_t> bytes;   // everything drained so far
    CpuStats stats;
  };
  SessionConfig cfg;
  std::vector<PerCpu> cpus;
  LayoutTable layouts;
  uint32_t pmu_type = 0;
  OnlineScan scan;
  std::thread drain;
  std::atomic<bool> stop_flag{false};
  std::atomic<bool> running{false};
  SessionStats stats;
  Status drain_error = Status::success();
  bool opened = false;

  // Closing a uprobe event removes the probe, and the kernel waits for a grace period before the
  // close returns: about 0.1 s per event (measured with strace).  Closing from several threads
  // does not shorten the total (measured: the kernel serializes the removals), so the close is
  // plain.  It happens after the traced command has exited.
  void close_all() {
    for (PerCpu& c : cpus) {
      if (c.map != nullptr && c.map != MAP_FAILED) munmap(c.map, c.map_len);
      for (int fd : c.site_fds)
        if (fd >= 0) close(fd);
      if (c.owner_fd >= 0) close(c.owner_fd);
    }
    cpus.clear();
    opened = false;
  }

  // Moves what the kernel has written into the CPU's byte stream; returns the bytes moved.
  Status drain_cpu(PerCpu& c, uint64_t* moved) {
    auto* page = static_cast<perf_event_mmap_page*>(c.map);
    const uint64_t head = __atomic_load_n(&page->data_head, __ATOMIC_ACQUIRE);
    const uint64_t tail = page->data_tail;
    *moved = 0;
    if (head == tail) return Status::success();
    const uint64_t unread = head - tail;
    if (unread > c.stats.high_water_bytes) c.stats.high_water_bytes = unread;
    const size_t before = c.bytes.size();
    Status s = kz::trace::ring_copy(c.data, c.data_size, head, tail, &c.bytes);
    if (!s.ok()) return s;
    __atomic_store_n(&page->data_tail, head, __ATOMIC_RELEASE);
    c.stats.bytes_drained += unread;
    *moved = unread;
    if (scan) {
      std::vector<kz::trace::PerfRecord> recs;
      (void)kz::trace::parse_records(c.bytes.data() + before, c.bytes.size() - before, static_cast<uint16_t>(c.cpu), layouts, &recs);
      scan(static_cast<uint16_t>(c.cpu), recs);
    }
    return Status::success();
  }

  void drain_loop() {
    if (!cfg.drain_cpus.empty()) {
      // Only this thread is moved: the thread that forks the target keeps its own placement, so the
      // target inherits nothing from this choice.
      cpu_set_t set;
      CPU_ZERO(&set);
      for (int c : cfg.drain_cpus)
        if (c >= 0 && c < CPU_SETSIZE) CPU_SET(c, &set);
      pthread_setaffinity_np(pthread_self(), sizeof set, &set);
    }
    std::vector<pollfd> pfds;
    for (PerCpu& c : cpus) {
      pollfd p;
      p.fd = c.owner_fd;
      p.events = POLLIN;
      p.revents = 0;
      pfds.push_back(p);
    }
    while (!stop_flag.load(std::memory_order_acquire)) {
      const int r = poll(pfds.data(), pfds.size(), static_cast<int>(cfg.poll_timeout_ms));
      if (r > 0) ++stats.drain_wakeups_by_data;
      ++stats.drain_rounds;
      for (PerCpu& c : cpus) {
        uint64_t moved = 0;
        Status s = drain_cpu(c, &moved);
        if (!s.ok() && drain_error.ok()) drain_error = s;
      }
    }
    timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == 0) stats.drain_cpu_ns = static_cast<uint64_t>(ts.tv_sec) * 1000000000ull + static_cast<uint64_t>(ts.tv_nsec);
    stats.drain_last_cpu = sched_getcpu_wrapper();
  }

  static int sched_getcpu_wrapper() {
#ifdef __linux__
    return ::sched_getcpu();
#else
    return -1;
#endif
  }
};

PerfSession::PerfSession() : impl_(new Impl) {}
PerfSession::~PerfSession() {
  if (impl_->drain.joinable()) {
    impl_->stop_flag.store(true);
    impl_->drain.join();
  }
  impl_->close_all();
}

Status PerfSession::open(const SessionConfig& cfg) {
  Impl& m = *impl_;
  if (m.opened) return Status::error(Err::Invalid, "the session is already open");
  m.cfg = cfg;
  if (cfg.sites.empty()) return Status::error(Err::Invalid, "no site to probe");
  if (cfg.ring_data_pages == 0 || (cfg.ring_data_pages & (cfg.ring_data_pages - 1)) != 0)
    return Status::error(Err::Invalid, "ring_data_pages must be a power of two (the kernel requires it)");

  std::string text;
  Status s = read_small_file("/sys/bus/event_source/devices/uprobe/type", &text);
  if (!s.ok()) return Status::error(Err::Io, "the kernel has no uprobe PMU (/sys/bus/event_source/devices/uprobe/type is missing): CONFIG_UPROBES/PERF_EVENTS");
  m.pmu_type = static_cast<uint32_t>(std::strtoul(text.c_str(), nullptr, 10));
  unsigned ret_bit = 0;
  s = uprobe_retprobe_bit(&ret_bit);
  if (!s.ok()) return s;

  std::vector<int> cpus = cfg.cpus;
  if (cpus.empty()) {
    s = read_small_file("/sys/devices/system/cpu/online", &text);
    if (!s.ok()) return s;
    s = parse_cpu_list(text, &cpus);
    if (!s.ok()) return s;
  }
  const long page = sysconf(_SC_PAGESIZE);
  const size_t map_len = static_cast<size_t>(page) * (1 + cfg.ring_data_pages);

  for (int cpu : cpus) {
    Impl::PerCpu c;
    c.cpu = cpu;
    c.stats.cpu = cpu;
    // The side-band event owns the ring.
    perf_event_attr a;
    std::memset(&a, 0, sizeof a);
    a.size = sizeof a;
    a.type = PERF_TYPE_SOFTWARE;
    a.config = PERF_COUNT_SW_DUMMY;
    a.sample_type = PERF_SAMPLE_IDENTIFIER | PERF_SAMPLE_TID | PERF_SAMPLE_TIME | PERF_SAMPLE_CPU;
    a.sample_id_all = 1;
    a.task = 1;
    a.comm = 1;
    a.comm_exec = 1;
    a.mmap = 1;   // executable mappings ...
    a.mmap2 = 1;  // ... in the form that carries device and inode
    a.disabled = 1;
    a.use_clockid = 1;
    a.clockid = CLOCK_MONOTONIC;
    a.watermark = 1;
    a.wakeup_watermark = static_cast<uint32_t>(map_len / 4);
    c.owner_fd = static_cast<int>(perf_event_open(&a, -1, cpu, -1, PERF_FLAG_FD_CLOEXEC));
    if (c.owner_fd < 0) {
      const int e = errno;
      std::string why = "perf_event_open of the side-band event on CPU " + std::to_string(cpu) + " failed: " + errno_text(e);
      if (e == EACCES || e == EPERM) why += "; the observer needs CAP_PERFMON (run as root) or kernel.perf_event_paranoid <= 0";
      m.close_all();
      return Status::error(Err::Io, why);
    }
    c.map = mmap(nullptr, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, c.owner_fd, 0);
    if (c.map == MAP_FAILED) {
      const int e = errno;
      c.map = nullptr;
      close(c.owner_fd);
      m.close_all();
      return Status::error(Err::Io, "mmap of the ring buffer on CPU " + std::to_string(cpu) + " failed: " + errno_text(e) +
                                        " (" + std::to_string(map_len) + " bytes; see perf_event_mlock_kb)");
    }
    c.map_len = map_len;
    auto* mp = static_cast<perf_event_mmap_page*>(c.map);
    c.data = static_cast<uint8_t*>(c.map) + mp->data_offset;
    c.data_size = mp->data_size;
    c.stats.ring_bytes = c.data_size;
    uint64_t id = 0;
    if (ioctl(c.owner_fd, PERF_EVENT_IOC_ID, &id) < 0) {
      const int e = errno;
      m.cpus.push_back(std::move(c));
      m.close_all();
      return Status::error(Err::Io, "PERF_EVENT_IOC_ID failed: " + errno_text(e));
    }
    EventLayout side;
    side.id = id;
    side.sample_type = PERF_SAMPLE_IDENTIFIER | PERF_SAMPLE_TID | PERF_SAMPLE_TIME | PERF_SAMPLE_CPU;
    side.site = KZ_NONE32;
    s = m.layouts.add(side);
    if (!s.ok()) { m.cpus.push_back(std::move(c)); m.close_all(); return s; }

    // One uprobe event per site.
    for (const SiteSpec& site : cfg.sites) {
      const PerfNeeds need = kz::trace::needs_of(site);
      perf_event_attr u;
      std::memset(&u, 0, sizeof u);
      u.size = sizeof u;
      u.type = m.pmu_type;
      u.config = site.kind == KZ_SITE_KIND_RET_PROBE ? (1ull << ret_bit) : 0;
      u.config1 = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(cfg.exe_path.c_str()));  // uprobe_path
      u.config2 = site.file_offset;                                                         // probe_offset
      u.sample_period = 1;
      u.sample_type = PERF_SAMPLE_IDENTIFIER | PERF_SAMPLE_IP | PERF_SAMPLE_TID | PERF_SAMPLE_TIME | PERF_SAMPLE_CPU;
      if (need.regs_user != 0 || need.stack_bytes != 0) u.sample_type |= PERF_SAMPLE_REGS_USER;
      u.sample_regs_user = need.regs_user;
      if (need.stack_bytes != 0) {
        u.sample_type |= PERF_SAMPLE_STACK_USER;
        u.sample_stack_user = need.stack_bytes;
      }
      u.sample_id_all = 1;
      u.disabled = 1;
      u.use_clockid = 1;
      u.clockid = CLOCK_MONOTONIC;
      const int fd = static_cast<int>(perf_event_open(&u, -1, cpu, -1, PERF_FLAG_FD_CLOEXEC));
      if (fd < 0) {
        const int e = errno;
        std::string why = "perf_event_open of the uprobe for site '" + site.label + "' (offset 0x" + [&] { char b[24]; std::snprintf(b, sizeof b, "%llx", static_cast<unsigned long long>(site.file_offset)); return std::string(b); }() +
                          ") on CPU " + std::to_string(cpu) + " failed: " + errno_text(e);
        if (e == EINVAL) why += "; the offset is not on an instruction boundary of a regular file, or the attribute is not accepted";
        if (e == ENOENT) why += "; the file or the PMU was not found";
        if (e == EACCES || e == EPERM) why += "; the observer needs CAP_PERFMON (run as root)";
        m.cpus.push_back(std::move(c));
        m.close_all();
        return Status::error(Err::Io, why);
      }
      c.site_fds.push_back(fd);
      if (ioctl(fd, PERF_EVENT_IOC_SET_OUTPUT, c.owner_fd) < 0) {
        const int e = errno;
        m.cpus.push_back(std::move(c));
        m.close_all();
        return Status::error(Err::Io, "PERF_EVENT_IOC_SET_OUTPUT for site '" + site.label + "' failed: " + errno_text(e));
      }
      uint64_t eid = 0;
      if (ioctl(fd, PERF_EVENT_IOC_ID, &eid) < 0) {
        const int e = errno;
        m.cpus.push_back(std::move(c));
        m.close_all();
        return Status::error(Err::Io, "PERF_EVENT_IOC_ID failed: " + errno_text(e));
      }
      EventLayout lay;
      lay.id = eid;
      lay.sample_type = u.sample_type;
      lay.regs_user = u.sample_regs_user;
      lay.stack_user = u.sample_stack_user;
      lay.site = site.site;
      s = m.layouts.add(lay);
      if (!s.ok()) { m.cpus.push_back(std::move(c)); m.close_all(); return s; }
    }
    m.cpus.push_back(std::move(c));
  }
  m.stats.uprobe_pmu_type = m.pmu_type;
  m.opened = true;
  return Status::success();
}

Status PerfSession::enable() {
  Impl& m = *impl_;
  if (!m.opened) return Status::error(Err::Invalid, "the session is not open");
  for (Impl::PerCpu& c : m.cpus) {
    if (ioctl(c.owner_fd, PERF_EVENT_IOC_ENABLE, 0) < 0) return Status::error(Err::Io, "enabling the side-band event failed: " + errno_text(errno));
    for (int fd : c.site_fds)
      if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) < 0) return Status::error(Err::Io, "enabling a uprobe event failed: " + errno_text(errno));
  }
  return Status::success();
}

Status PerfSession::start_drain(OnlineScan scan) {
  Impl& m = *impl_;
  if (!m.opened) return Status::error(Err::Invalid, "the session is not open");
  m.scan = std::move(scan);
  m.stop_flag.store(false);
  m.drain = std::thread([this] { impl_->drain_loop(); });
  m.running.store(true);
  return Status::success();
}

Status PerfSession::stop(std::vector<CpuStream>* streams, LayoutTable* layouts, SessionStats* stats) {
  Impl& m = *impl_;
  if (!m.opened) return Status::error(Err::Invalid, "the session is not open");
  // 1. The probes stop first; the side-band events keep running for the flush below.
  for (Impl::PerCpu& c : m.cpus)
    for (int fd : c.site_fds) ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
  // 2. The drain thread ends; then everything the kernel wrote is moved.
  if (m.drain.joinable()) {
    m.stop_flag.store(true, std::memory_order_release);
    m.drain.join();
  }
  Status first_error = m.drain_error;
  auto drain_all = [&]() {
    for (Impl::PerCpu& c : m.cpus) {
      for (;;) {
        uint64_t moved = 0;
        Status s = m.drain_cpu(c, &moved);
        if (!s.ok()) {
          if (first_error.ok()) first_error = s;
          break;
        }
        if (moved == 0) break;
      }
    }
  };
  drain_all();
  // 3. Flush the loss counters.  The kernel reports records it dropped only when the NEXT record is
  // written to the same ring (PERF_RECORD_LOST precedes it); if the last records of a run were
  // dropped and nothing is written afterwards, the count would stay in the kernel and the loss would
  // be invisible.  The probes are off and the rings are empty, so one harmless record per CPU --
  // a thread pinned to the CPU renames itself and exits -- makes the kernel report what it owes.
  for (Impl::PerCpu& c : m.cpus) {
    std::thread flusher([cpu = c.cpu] {
      cpu_set_t set;
      CPU_ZERO(&set);
      if (cpu >= 0 && cpu < CPU_SETSIZE) CPU_SET(cpu, &set);
      pthread_setaffinity_np(pthread_self(), sizeof set, &set);
      prctl(PR_SET_NAME, "kz-flush", 0, 0, 0);
    });
    flusher.join();
  }
  drain_all();
  // 4. Now the side-band events stop as well.
  for (Impl::PerCpu& c : m.cpus) ioctl(c.owner_fd, PERF_EVENT_IOC_DISABLE, 0);
  drain_all();
  streams->clear();
  m.stats.cpus.clear();
  for (Impl::PerCpu& c : m.cpus) {
    CpuStream cs;
    cs.cpu = static_cast<uint16_t>(c.cpu);
    cs.bytes = std::move(c.bytes);
    streams->push_back(std::move(cs));
    m.stats.cpus.push_back(c.stats);
  }
  *layouts = m.layouts;
  *stats = m.stats;
  m.close_all();
  return first_error;
}

}  // namespace kz::probe

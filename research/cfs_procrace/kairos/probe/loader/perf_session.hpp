// perf_session.hpp -- the perf-sample backend: uprobe events through perf_event_open, one set per
// CPU, one ring buffer per CPU, and a drain that only moves bytes.
//
// What is opened (SPEC 5.1 "perf-sample uprobe backend"; PI decision P-LIBBPF: the existing
// environment, no library, no install):
//   per online CPU  one side-band event (a software "dummy" event that records FORK, EXIT, COMM
//                   and the executable mappings of every process on the CPU) that owns the CPU's
//                   ring buffer, and one uprobe event per site, whose output is redirected into
//                   that ring buffer, so that one stream per CPU carries everything in kernel
//                   order;
//   attach          by file (inode) and offset through the uprobe PMU, system-wide per CPU
//                   (pid = -1), before the target starts: every process that maps the file is
//                   observed, including threads created later, and nothing is installed in the
//                   binary or the process (uprobes are applied to the processes' memory by the
//                   kernel);
//   per record      IDENTIFIER (the event, hence the site), IP, TID, TIME on the clock named in the
//                   run record (CLOCK_MONOTONIC), CPU, the user registers and the user-stack bytes
//                   that the site's slots need and no more.
// The drain copies ring bytes into memory and nothing else; decoding is offline
// (kz/trace/decode_*).  Loss comes from the kernel's own reports (LOST, THROTTLE) and is decoded
// into LOSS records; there is no other loss detector in this backend.
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "kz/core/status.hpp"
#include "kz/trace/decode_merge.hpp"

namespace kz::probe {

struct SessionConfig {
  std::string exe_path;            // the file to probe; resolved by the kernel (e.g. /proc/self/fd/N of a verified descriptor)
  std::vector<std::string> also_paths;  // further files of a plan over several files (SiteSpec::binary 1, 2, ...), as exe_path
  std::vector<kz::trace::SiteSpec> sites;
  uint32_t ring_data_pages = 512;  // per CPU; a power of two (the kernel requires it)
  uint32_t poll_timeout_ms = 10;   // the longest the drain sleeps with data possibly waiting
  std::vector<int> cpus;           // empty: every online CPU
  std::vector<int> drain_cpus;     // empty: the drain thread keeps the placement it inherits; else it runs on these CPUs only
};

struct CpuStats {
  int cpu = 0;
  uint64_t bytes_drained = 0;
  uint64_t high_water_bytes = 0;   // the most unread bytes seen at a drain
  uint64_t ring_bytes = 0;         // size of the data area
};

struct SessionStats {
  std::vector<CpuStats> cpus;
  uint64_t drain_cpu_ns = 0;       // CPU time of the drain thread
  uint64_t drain_rounds = 0;
  uint64_t drain_wakeups_by_data = 0;
  int drain_last_cpu = -1;
  uint32_t uprobe_pmu_type = 0;
  std::string clock = "CLOCK_MONOTONIC";
};

// Called by the drain thread with the records of each chunk, parsed online; used only to learn
// which threads exist while they still do.  An error in the online parse is ignored there: the
// offline decode is the authority.
using OnlineScan = std::function<void(uint16_t cpu, const std::vector<kz::trace::PerfRecord>&)>;

class PerfSession {
 public:
  PerfSession();
  ~PerfSession();
  PerfSession(const PerfSession&) = delete;
  PerfSession& operator=(const PerfSession&) = delete;

  // Creates every event (disabled) and maps the rings.  On failure nothing stays open and the
  // Status carries the system error, the CPU and the site that failed.
  Status open(const SessionConfig& cfg);
  // Enables the events (side-band first).
  Status enable();
  // Starts the drain thread.
  Status start_drain(OnlineScan scan = OnlineScan());
  // Disables the events, drains what is left, stops the thread and returns the streams.
  Status stop(std::vector<kz::trace::CpuStream>* streams, kz::trace::LayoutTable* layouts, SessionStats* stats);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Parses the kernel's cpu list text ("0-3,6") of /sys/devices/system/cpu/online.
Status parse_cpu_list(const std::string& text, std::vector<int>* out);

}  // namespace kz::probe

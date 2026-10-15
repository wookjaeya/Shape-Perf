// decode_merge.hpp -- from the per-CPU byte streams of a session to the typed records of a run.
//
// The per-CPU rings are merged into one order, (kernel timestamp, CPU, arrival).  The kernel
// timestamp is the clock named in the run record (CLOCK_MONOTONIC: one clock for all CPUs).
// That order is the program order of each thread under the clock assumption A-CLK-PO (SPEC
// 5.4) and nothing more: it is never used as an ordering between threads (SPEC 5.8).
//
// What is kept:
//   - every sample becomes a raw event of its site, with the per-thread sequence number the
//     merged order gives it;
//   - FORK, EXIT and COMM records of the traced processes become side events;
//   - the executable mappings of the traced file become map events;
//   - LOST and THROTTLE records become loss records (SPEC 5.6: loss is never read as the
//     absence of events).
// A process is traced if it maps the traced file or produced a sample.  The side-band stream is
// system-wide (the kernel reports every fork and rename on the CPU), so everything about other
// processes is dropped and counted.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"
#include "kz/trace/decode_perf.hpp"
#include "kz/trace/decode_sites.hpp"

namespace kz::trace {

struct SideEvent {
  kz_side_ev rec;    // rec.comm is not set: the string is written when the container is built
  std::string comm;
};

struct MapEvent {
  kz_map_ev rec;     // rec.path is not set, as above
  std::string path;
};

struct CpuStream {
  uint16_t cpu = 0;
  std::vector<uint8_t> bytes;  // the records copied out of the ring of this CPU, in order
};

struct TracedFile {
  bool known = false;
  uint64_t ino = 0;
  uint32_t dev_major = 0;
  uint32_t dev_minor = 0;
};

struct DecodeInput {
  std::vector<CpuStream> streams;
  LayoutTable layouts;
  std::vector<SiteSpec> sites;  // the plan; a site's id is its index
  TracedFile traced;
};

struct DecodeStats {
  ParseStats parse;
  uint64_t samples_without_site = 0;   // a sample of an event that feeds no plan site
  uint64_t slot_failures = 0;          // samples whose slots could not be applied
  uint64_t slots_truncated = 0;        // events with a slot the kernel did not supply (flag TRUNC)
  uint64_t side_dropped = 0;           // FORK/EXIT/COMM of other processes
  uint64_t maps_dropped = 0;           // executable mappings that are not the traced file
  uint64_t lost_reports = 0;           // PERF_RECORD_LOST records (the kernel's reports)
  uint64_t dropped_records = 0;        // the sum of their counts: records of ANY kind the kernel dropped from a shared ring
                                       // (probe samples and the side-band records of every process), so an upper bound
                                       // of the probe hits that were lost
  uint64_t throttle_intervals = 0;
  uint64_t stream_errors = 0;          // streams that ended in a malformed record
  uint64_t bias_mismatches = 0;        // entry/point samples whose address disagrees with the first
  std::vector<uint64_t> site_hits;     // per site
};

struct DecodedRun {
  std::vector<kz_raw_ev> events;
  std::vector<SideEvent> side;
  std::vector<MapEvent> maps;
  std::vector<kz_loss> loss;
  std::vector<uint32_t> traced_pids;   // sorted
  bool bias_known = false;
  uint64_t load_bias = 0;              // runtime address - link-time address of the traced file
  DecodeStats stats;
  std::vector<Status> stream_status;   // per input stream: ok, or the parse error
};

// Decodes and merges.  A malformed stream does not stop the others: its records up to the
// error are kept, the error is in stream_status and counted, and the function returns the
// first such error (callers that must not accept a damaged trace test the return value).
Status decode_run(const DecodeInput& in, DecodedRun* out);

}  // namespace kz::trace

// decode_perf.hpp -- the perf-sample backend's wire format, decoded.
//
// The observer copies the kernel's per-CPU ring buffers into linear byte streams while the
// target runs and decodes them afterwards, so that nothing in the hot path can fail on a
// malformed record (SPEC 5.3: the traced threads make no extra system calls; the drain only
// moves bytes).  This file is the offline half: ring arithmetic and record parsing, on
// plain byte buffers, with no system call and no dependence on the kernel headers.
//
// Wire format: a stream of records, each `struct perf_event_header { u32 type; u16 misc;
// u16 size; }` followed by `size - 8` bytes, `size` a multiple of 8 (perf_event_open(2)).
// Every event of a session carries PERF_SAMPLE_IDENTIFIER and sample_id_all, so the id of
// the event that wrote a record is the first word of a sample and the last word of every
// other record; that id selects the EventLayout used to parse the rest.  The decoder
// accepts exactly the sample layouts it understands (IDENTIFIER, IP, TID, TIME, CPU,
// REGS_USER, STACK_USER) and refuses any other bit: a misread layout would mis-assign
// every later field, so it fails closed.
//
// All functions are total on arbitrary bytes: they never read outside the buffer and
// report malformed input as a Status (tests feed them every truncation and corrupted
// copies of valid streams under the sanitizers).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"

namespace kz::trace {

// perf_event_attr.sample_type bits and record types (stable kernel ABI; verified against
// <linux/perf_event.h> by the tests).
namespace perfabi {
constexpr uint64_t kSampleIp = 1ull << 0;
constexpr uint64_t kSampleTid = 1ull << 1;
constexpr uint64_t kSampleTime = 1ull << 2;
constexpr uint64_t kSampleAddr = 1ull << 3;
constexpr uint64_t kSampleRead = 1ull << 4;
constexpr uint64_t kSampleCallchain = 1ull << 5;
constexpr uint64_t kSampleId = 1ull << 6;
constexpr uint64_t kSampleCpu = 1ull << 7;
constexpr uint64_t kSamplePeriod = 1ull << 8;
constexpr uint64_t kSampleStreamId = 1ull << 9;
constexpr uint64_t kSampleRaw = 1ull << 10;
constexpr uint64_t kSampleBranchStack = 1ull << 11;
constexpr uint64_t kSampleRegsUser = 1ull << 12;
constexpr uint64_t kSampleStackUser = 1ull << 13;
constexpr uint64_t kSampleIdentifier = 1ull << 16;

constexpr uint32_t kRecordMmap = 1;
constexpr uint32_t kRecordLost = 2;
constexpr uint32_t kRecordComm = 3;
constexpr uint32_t kRecordExit = 4;
constexpr uint32_t kRecordThrottle = 5;
constexpr uint32_t kRecordUnthrottle = 6;
constexpr uint32_t kRecordFork = 7;
constexpr uint32_t kRecordSample = 9;
constexpr uint32_t kRecordMmap2 = 10;

constexpr uint16_t kMiscCommExec = 1u << 13;
constexpr uint32_t kRegCount = 24;  // x86-64 perf register numbers 0..23
}  // namespace perfabi

// How the records of one event are laid out.
struct EventLayout {
  uint64_t id = 0;            // PERF_EVENT_IOC_ID of the event = the IDENTIFIER of its records
  uint64_t sample_type = 0;   // attr.sample_type
  uint64_t regs_user = 0;     // attr.sample_regs_user: bit n = perf register number n
  uint32_t stack_user = 0;    // attr.sample_stack_user: bytes requested
  uint32_t site = KZ_NONE32;  // plan site this event feeds; KZ_NONE32 for a side-band event
};

// The events of a session, looked up by id.
class LayoutTable {
 public:
  Status add(const EventLayout& layout);  // refuses a duplicate id and a layout it cannot parse
  const EventLayout* find(uint64_t id) const;
  size_t size() const { return layouts_.size(); }

 private:
  std::vector<EventLayout> layouts_;  // sorted by id
};

enum class RecKind : uint8_t { Sample, Lost, Throttle, Unthrottle, Fork, Exit, Comm, Mmap2, Other };

struct PerfRecord {
  RecKind kind = RecKind::Other;
  uint32_t type_code = 0;    // PERF_RECORD_* as read
  uint16_t misc = 0;
  uint16_t buffer_cpu = 0;   // the CPU whose ring the record came from (set by the caller)
  uint32_t order = 0;        // arrival index within its stream
  uint64_t id = 0;           // id of the event that wrote it (0 if the record has none)
  uint32_t pid = 0, tid = 0; // sample_id trailer / TID field
  uint32_t cpu = 0;          // CPU recorded in the sample_id trailer / CPU field
  uint64_t time = 0;
  // Sample.
  uint64_t ip = 0;
  uint64_t regs_valid = 0;   // bit n set: regs[n] was captured
  uint64_t regs[perfabi::kRegCount] = {};
  std::vector<uint8_t> stack;  // the bytes dumped from the user stack, trimmed to dyn_size
  uint64_t stack_dyn = 0;      // bytes the kernel actually copied
  uint32_t stack_requested = 0;
  // Lost.
  uint64_t lost = 0;
  // Fork / Exit: pid/tid above are the thread; ppid/ptid its parent.
  uint32_t ppid = 0, ptid = 0;
  // Comm / Mmap2.
  std::string text;          // comm, or the path of a mapping
  // Mmap2.
  uint64_t addr = 0, len = 0, pgoff = 0, ino = 0;
  uint32_t dev_major = 0, dev_minor = 0, prot = 0, map_flags = 0;
};

struct ParseStats {
  uint64_t records = 0;
  uint64_t samples = 0;
  uint64_t other = 0;        // record types the decoder does not interpret (counted, skipped)
  uint64_t unknown_id = 0;   // records whose event id has no layout (kept as Other)
};

// Parses the records of one linear stream.  `buffer_cpu` is stored in every record.  On a
// malformed record, the records before it are returned and the Status names the byte offset;
// `*consumed` (if not null) is set to the offset of the first byte not parsed.
Status parse_records(const uint8_t* data, size_t len, uint16_t buffer_cpu, const LayoutTable& layouts,
                     std::vector<PerfRecord>* out, ParseStats* stats = nullptr, size_t* consumed = nullptr);

// Ring arithmetic (perf_event_mmap_page): the data area is `data_size` bytes (a power of
// two); the kernel writes at `head`, the reader consumes up to `tail`; both only grow.
// Appends the bytes [tail, head) to `*out` in order, unwrapping the ring.  Fails if the
// reader fell behind by more than the ring (head - tail > data_size: the kernel overwrote
// unread data, which the non-overwrite mode used here never does) or if head < tail.
Status ring_copy(const uint8_t* data_area, uint64_t data_size, uint64_t head, uint64_t tail, std::vector<uint8_t>* out);

// Checks that a layout only uses bits the parser understands.
Status check_layout(const EventLayout& layout);

}  // namespace kz::trace

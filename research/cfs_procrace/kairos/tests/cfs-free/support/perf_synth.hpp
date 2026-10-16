// perf_synth.hpp -- builds perf record streams byte by byte, for the decoder's unit tests.
//
// The writer follows perf_event_open(2) (record layouts) directly and shares no code with the
// decoder.  Its layouts are also checked against bytes that the kernel wrote
// (real_stream.hpp, test_trace_decode_real.cpp), so a misreading of the manual would show up
// there.  Values are unit-test values.
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "kz/trace/decode_perf.hpp"

namespace kzsyn {

using kz::trace::EventLayout;
namespace pa = kz::trace::perfabi;

class Stream {
 public:
  std::vector<uint8_t> bytes;

  void u16(uint16_t v) { raw(&v, 2); }
  void u32(uint32_t v) { raw(&v, 4); }
  void u64(uint64_t v) { raw(&v, 8); }
  void raw(const void* p, size_t n) {
    const uint8_t* b = static_cast<const uint8_t*>(p);
    cur.insert(cur.end(), b, b + n);
  }
  void pad8() {
    while (cur.size() % 8 != 0) cur.push_back(0);
  }

  // Closes the record being built: header (type, misc, size) in front, size a multiple of 8.
  void end(uint32_t type, uint16_t misc = 0) {
    pad8();
    const uint16_t size = static_cast<uint16_t>(8 + cur.size());
    const uint8_t* t = reinterpret_cast<const uint8_t*>(&type);
    const uint8_t* m = reinterpret_cast<const uint8_t*>(&misc);
    const uint8_t* s = reinterpret_cast<const uint8_t*>(&size);
    bytes.insert(bytes.end(), t, t + 4);
    bytes.insert(bytes.end(), m, m + 2);
    bytes.insert(bytes.end(), s, s + 2);
    bytes.insert(bytes.end(), cur.begin(), cur.end());
    cur.clear();
  }

  // sample_id trailer of a non-sample record, per the event's sample_type.
  void trailer(const EventLayout& l, uint32_t pid, uint32_t tid, uint64_t time, uint32_t cpu) {
    if (l.sample_type & pa::kSampleTid) { u32(pid); u32(tid); }
    if (l.sample_type & pa::kSampleTime) u64(time);
    if (l.sample_type & pa::kSampleCpu) { u32(cpu); u32(0); }
    if (l.sample_type & pa::kSampleIdentifier) u64(l.id);
  }

  struct SampleArgs {
    uint64_t ip = 0;
    uint32_t pid = 0, tid = 0, cpu = 0;
    uint64_t time = 0;
    uint64_t abi = 2;                 // 0: no registers follow
    std::vector<uint64_t> regs;       // in ascending register number order, popcount(mask) of them
    std::vector<uint8_t> stack;       // the dumped bytes (size = stack.size() rounded as given)
    uint64_t stack_size = ~0ull;      // 'size' field; default stack.size()
    uint64_t stack_dyn = ~0ull;       // 'dyn_size' field; default stack.size()
  };

  void sample(const EventLayout& l, const SampleArgs& a) {
    u64(l.id);
    if (l.sample_type & pa::kSampleIp) u64(a.ip);
    if (l.sample_type & pa::kSampleTid) { u32(a.pid); u32(a.tid); }
    if (l.sample_type & pa::kSampleTime) u64(a.time);
    if (l.sample_type & pa::kSampleCpu) { u32(a.cpu); u32(0); }
    if (l.sample_type & pa::kSampleRegsUser) {
      u64(a.abi);
      if (a.abi != 0)
        for (uint64_t r : a.regs) u64(r);
    }
    if (l.sample_type & pa::kSampleStackUser) {
      const uint64_t size = a.stack_size == ~0ull ? a.stack.size() : a.stack_size;
      u64(size);
      if (size != 0) {
        raw(a.stack.data(), a.stack.size());
        for (size_t i = a.stack.size(); i < size; ++i) cur.push_back(0);
        u64(a.stack_dyn == ~0ull ? a.stack.size() : a.stack_dyn);
      }
    }
    end(pa::kRecordSample, 2);
  }

  void fork(const EventLayout& side, uint32_t pid, uint32_t ppid, uint32_t tid, uint32_t ptid, uint64_t time, uint32_t cpu = 0) {
    u32(pid); u32(ppid); u32(tid); u32(ptid); u64(time);
    trailer(side, pid, tid, time, cpu);
    end(pa::kRecordFork);
  }
  void exit_(const EventLayout& side, uint32_t pid, uint32_t ppid, uint32_t tid, uint32_t ptid, uint64_t time, uint32_t cpu = 0) {
    u32(pid); u32(ppid); u32(tid); u32(ptid); u64(time);
    trailer(side, pid, tid, time, cpu);
    end(pa::kRecordExit);
  }
  void comm(const EventLayout& side, uint32_t pid, uint32_t tid, const std::string& name, uint64_t time, bool exec = false, uint32_t cpu = 0) {
    u32(pid); u32(tid);
    raw(name.c_str(), name.size() + 1);
    pad8();
    trailer(side, pid, tid, time, cpu);
    end(pa::kRecordComm, exec ? pa::kMiscCommExec : 0);
  }
  void mmap2(const EventLayout& side, uint32_t pid, uint32_t tid, uint64_t addr, uint64_t len, uint64_t pgoff, uint32_t maj,
             uint32_t min, uint64_t ino, const std::string& path, uint64_t time, uint32_t cpu = 0) {
    u32(pid); u32(tid); u64(addr); u64(len); u64(pgoff); u32(maj); u32(min); u64(ino); u64(0); u32(5); u32(2);
    raw(path.c_str(), path.size() + 1);
    pad8();
    trailer(side, pid, tid, time, cpu);
    end(pa::kRecordMmap2, 2);
  }
  void lost(const EventLayout& writer, uint64_t count, uint32_t pid, uint32_t tid, uint64_t time, uint32_t cpu = 0) {
    u64(writer.id); u64(count);
    trailer(writer, pid, tid, time, cpu);
    end(pa::kRecordLost);
  }
  void throttle(const EventLayout& l, bool on, uint64_t time, uint32_t pid = 0, uint32_t tid = 0, uint32_t cpu = 0) {
    u64(time); u64(l.id); u64(l.id);
    trailer(l, pid, tid, time, cpu);
    end(on ? pa::kRecordThrottle : pa::kRecordUnthrottle);
  }
  // A record of a type the decoder does not interpret, with a correct trailer.
  void other(const EventLayout& l, uint32_t type, uint64_t time) {
    u64(0x1122334455667788ull);
    trailer(l, 1, 1, time, 0);
    end(type);
  }

 private:
  std::vector<uint8_t> cur;
};

// Layouts used by the tests.
inline EventLayout side_layout(uint64_t id) {
  EventLayout l;
  l.id = id;
  l.sample_type = pa::kSampleIdentifier | pa::kSampleTid | pa::kSampleTime | pa::kSampleCpu;
  return l;
}
inline EventLayout probe_layout(uint64_t id, uint32_t site, uint64_t regs, uint32_t stack) {
  EventLayout l;
  l.id = id;
  l.sample_type = pa::kSampleIdentifier | pa::kSampleIp | pa::kSampleTid | pa::kSampleTime | pa::kSampleCpu | pa::kSampleRegsUser;
  if (stack != 0) l.sample_type |= pa::kSampleStackUser;
  l.regs_user = regs;
  l.stack_user = stack;
  l.site = site;
  return l;
}

}  // namespace kzsyn

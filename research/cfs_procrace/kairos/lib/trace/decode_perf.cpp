// decode_perf.cpp -- see include/kz/trace/decode_perf.hpp.
#include "kz/trace/decode_perf.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace kz::trace {

namespace {

constexpr size_t kHeaderSize = 8;

uint32_t rd32(const uint8_t* p) {
  uint32_t v;
  std::memcpy(&v, p, sizeof v);
  return v;
}
uint64_t rd64(const uint8_t* p) {
  uint64_t v;
  std::memcpy(&v, p, sizeof v);
  return v;
}
uint16_t rd16(const uint8_t* p) {
  uint16_t v;
  std::memcpy(&v, p, sizeof v);
  return v;
}

int popcount64(uint64_t v) {
  int n = 0;
  while (v != 0) {
    v &= v - 1;
    ++n;
  }
  return n;
}

// The bits of sample_type that this decoder understands.
constexpr uint64_t kUnderstood = perfabi::kSampleIp | perfabi::kSampleTid | perfabi::kSampleTime | perfabi::kSampleCpu |
                                 perfabi::kSampleIdentifier | perfabi::kSampleRegsUser | perfabi::kSampleStackUser;

// The sample_id trailer that follows the body of every record that is not a sample.
size_t trailer_size(uint64_t sample_type) {
  size_t n = 0;
  if ((sample_type & perfabi::kSampleTid) != 0) n += 8;
  if ((sample_type & perfabi::kSampleTime) != 0) n += 8;
  if ((sample_type & perfabi::kSampleCpu) != 0) n += 8;
  if ((sample_type & perfabi::kSampleIdentifier) != 0) n += 8;
  return n;
}

// A cursor that refuses to read past the end of its range.
struct Cursor {
  const uint8_t* p;
  size_t left;
  bool ok = true;
  const uint8_t* take(size_t n) {
    if (!ok || n > left) {
      ok = false;
      return nullptr;
    }
    const uint8_t* r = p;
    p += n;
    left -= n;
    return r;
  }
  uint64_t u64() {
    const uint8_t* b = take(8);
    return b != nullptr ? rd64(b) : 0;
  }
  uint32_t u32() {
    const uint8_t* b = take(4);
    return b != nullptr ? rd32(b) : 0;
  }
};

// The trailer fields in the order the kernel writes them.
void parse_trailer(const uint8_t* t, size_t len, uint64_t sample_type, PerfRecord* r) {
  Cursor c{t, len};
  if ((sample_type & perfabi::kSampleTid) != 0) {
    r->pid = c.u32();
    r->tid = c.u32();
  }
  if ((sample_type & perfabi::kSampleTime) != 0) r->time = c.u64();
  if ((sample_type & perfabi::kSampleCpu) != 0) {
    r->cpu = c.u32();
    (void)c.u32();
  }
  if ((sample_type & perfabi::kSampleIdentifier) != 0) r->id = c.u64();
}

std::string c_string(const uint8_t* p, size_t n) {
  size_t k = 0;
  while (k < n && p[k] != 0) ++k;
  return std::string(reinterpret_cast<const char*>(p), k);
}

Status bad(const char* what, size_t offset) {
  return Status::error(Err::Invalid, std::string("perf stream: ") + what + " at byte " + std::to_string(offset));
}

Status parse_sample(const uint8_t* body, size_t len, const EventLayout& lay, PerfRecord* r) {
  Cursor c{body, len};
  r->id = c.u64();  // IDENTIFIER, already known to be present
  if ((lay.sample_type & perfabi::kSampleIp) != 0) r->ip = c.u64();
  if ((lay.sample_type & perfabi::kSampleTid) != 0) {
    r->pid = c.u32();
    r->tid = c.u32();
  }
  if ((lay.sample_type & perfabi::kSampleTime) != 0) r->time = c.u64();
  if ((lay.sample_type & perfabi::kSampleCpu) != 0) {
    r->cpu = c.u32();
    (void)c.u32();
  }
  if ((lay.sample_type & perfabi::kSampleRegsUser) != 0) {
    const uint64_t abi = c.u64();
    if (abi != 0) {  // 0 = no registers (the task was not in user mode); otherwise a full set follows
      const int n = popcount64(lay.regs_user);
      for (uint32_t reg = 0, k = 0; reg < perfabi::kRegCount && k < static_cast<uint32_t>(n); ++reg) {
        if ((lay.regs_user & (1ull << reg)) == 0) continue;
        r->regs[reg] = c.u64();
        r->regs_valid |= 1ull << reg;
        ++k;
      }
    }
  }
  if ((lay.sample_type & perfabi::kSampleStackUser) != 0) {
    const uint64_t size = c.u64();
    r->stack_requested = lay.stack_user;
    if (size != 0) {
      if (size > c.left) return Status::error(Err::Truncated, "sample: stack dump longer than the record");
      const uint8_t* data = c.take(static_cast<size_t>(size));
      const uint64_t dyn = c.u64();
      if (!c.ok) return Status::error(Err::Truncated, "sample: no dyn_size after the stack dump");
      if (dyn > size) return Status::error(Err::Invalid, "sample: dyn_size exceeds the dump size");
      r->stack_dyn = dyn;
      r->stack.assign(data, data + dyn);
    }
  }
  if (!c.ok) return Status::error(Err::Truncated, "sample: record shorter than its layout");
  return Status::success();
}

}  // namespace

Status check_layout(const EventLayout& l) {
  if ((l.sample_type & perfabi::kSampleIdentifier) == 0)
    return Status::error(Err::Invalid, "event layout: PERF_SAMPLE_IDENTIFIER is required (it names the event of every record)");
  if ((l.sample_type & ~kUnderstood) != 0)
    return Status::error(Err::Invalid, "event layout: sample_type has bits this decoder does not parse: 0x" +
                                           [&] {
                                             char b[24];
                                             std::snprintf(b, sizeof b, "%llx", static_cast<unsigned long long>(l.sample_type & ~kUnderstood));
                                             return std::string(b);
                                           }());
  if ((l.sample_type & perfabi::kSampleRegsUser) == 0 && l.regs_user != 0)
    return Status::error(Err::Invalid, "event layout: sample_regs_user without PERF_SAMPLE_REGS_USER");
  if ((l.regs_user >> perfabi::kRegCount) != 0)
    return Status::error(Err::Invalid, "event layout: sample_regs_user names a register beyond the x86-64 set");
  if ((l.sample_type & perfabi::kSampleStackUser) != 0 && (l.sample_type & perfabi::kSampleRegsUser) == 0)
    return Status::error(Err::Invalid, "event layout: the stack dump needs the user registers (the kernel starts it at sp)");
  if ((l.sample_type & perfabi::kSampleStackUser) == 0 && l.stack_user != 0)
    return Status::error(Err::Invalid, "event layout: sample_stack_user without PERF_SAMPLE_STACK_USER");
  return Status::success();
}

Status LayoutTable::add(const EventLayout& layout) {
  Status s = check_layout(layout);
  if (!s.ok()) return s;
  auto it = std::lower_bound(layouts_.begin(), layouts_.end(), layout.id,
                             [](const EventLayout& a, uint64_t id) { return a.id < id; });
  if (it != layouts_.end() && it->id == layout.id)
    return Status::error(Err::Duplicate, "event id " + std::to_string(layout.id) + " is already in the layout table");
  layouts_.insert(it, layout);
  return Status::success();
}

const EventLayout* LayoutTable::find(uint64_t id) const {
  auto it = std::lower_bound(layouts_.begin(), layouts_.end(), id, [](const EventLayout& a, uint64_t v) { return a.id < v; });
  return (it != layouts_.end() && it->id == id) ? &*it : nullptr;
}

Status parse_records(const uint8_t* data, size_t len, uint16_t buffer_cpu, const LayoutTable& layouts,
                     std::vector<PerfRecord>* out, ParseStats* stats, size_t* consumed) {
  ParseStats local;
  ParseStats& st = stats != nullptr ? *stats : local;
  size_t off = 0;
  uint32_t order = 0;
  Status result = Status::success();
  while (off < len) {
    if (len - off < kHeaderSize) {
      result = Status::error(Err::Truncated, "perf stream: " + std::to_string(len - off) + " trailing bytes, less than a record header, at byte " + std::to_string(off));
      break;
    }
    const uint32_t type = rd32(data + off);
    const uint16_t misc = rd16(data + off + 4);
    const uint16_t size = rd16(data + off + 6);
    if (size < kHeaderSize || (size % 8) != 0) {
      result = bad("record size is not a multiple of 8 or is below the header size", off);
      break;
    }
    if (size > len - off) {
      result = Status::error(Err::Truncated, "perf stream: record of " + std::to_string(size) + " bytes runs past the end of the stream, at byte " + std::to_string(off));
      break;
    }
    const uint8_t* body = data + off + kHeaderSize;
    const size_t blen = size - kHeaderSize;

    PerfRecord r;
    r.type_code = type;
    r.misc = misc;
    r.buffer_cpu = buffer_cpu;
    r.order = order++;
    ++st.records;

    // The id of the event that wrote the record: the first word of a sample, the last
    // word of everything else (sample_id_all with IDENTIFIER).
    uint64_t id = 0;
    bool have_id = false;
    if (type == perfabi::kRecordSample) {
      if (blen >= 8) {
        id = rd64(body);
        have_id = true;
      }
    } else if (blen >= 8) {
      id = rd64(body + blen - 8);
      have_id = true;
    }
    const EventLayout* lay = have_id ? layouts.find(id) : nullptr;

    if (lay == nullptr) {
      // A record we cannot attribute (an event outside this session, or a record type with
      // no trailer): counted and skipped, never guessed at.
      r.kind = RecKind::Other;
      ++st.other;
      if (have_id) {
        r.id = id;
        ++st.unknown_id;
      }
      out->push_back(std::move(r));
      off += size;
      continue;
    }

    const size_t tsize = trailer_size(lay->sample_type);
    switch (type) {
      case perfabi::kRecordSample: {
        r.kind = RecKind::Sample;
        Status s = parse_sample(body, blen, *lay, &r);
        if (!s.ok()) {
          result = Status::error(s.code, s.message + ", at byte " + std::to_string(off));
          goto done;
        }
        ++st.samples;
        break;
      }
      case perfabi::kRecordLost: {
        r.kind = RecKind::Lost;
        if (blen < 16 + tsize) { result = bad("LOST record too short", off); goto done; }
        // body: u64 id, u64 lost, then the trailer
        r.lost = rd64(body + 8);
        parse_trailer(body + 16, blen - 16, lay->sample_type, &r);
        break;
      }
      case perfabi::kRecordThrottle:
      case perfabi::kRecordUnthrottle: {
        r.kind = type == perfabi::kRecordThrottle ? RecKind::Throttle : RecKind::Unthrottle;
        if (blen < 24 + tsize) { result = bad("THROTTLE record too short", off); goto done; }
        parse_trailer(body + 24, blen - 24, lay->sample_type, &r);
        r.time = rd64(body);  // the record's own time is the authoritative one
        break;
      }
      case perfabi::kRecordFork:
      case perfabi::kRecordExit: {
        r.kind = type == perfabi::kRecordFork ? RecKind::Fork : RecKind::Exit;
        if (blen < 24 + tsize) { result = bad("FORK/EXIT record too short", off); goto done; }
        parse_trailer(body + 24, blen - 24, lay->sample_type, &r);
        r.pid = rd32(body);
        r.ppid = rd32(body + 4);
        r.tid = rd32(body + 8);
        r.ptid = rd32(body + 12);
        r.time = rd64(body + 16);
        break;
      }
      case perfabi::kRecordComm: {
        r.kind = RecKind::Comm;
        if (blen < 8 + tsize) { result = bad("COMM record too short", off); goto done; }
        parse_trailer(body + (blen - tsize), tsize, lay->sample_type, &r);
        r.pid = rd32(body);
        r.tid = rd32(body + 4);
        r.text = c_string(body + 8, blen - tsize - 8);
        break;
      }
      case perfabi::kRecordMmap2: {
        r.kind = RecKind::Mmap2;
        if (blen < 56 + 8 + tsize) { result = bad("MMAP2 record too short", off); goto done; }
        parse_trailer(body + (blen - tsize), tsize, lay->sample_type, &r);
        r.pid = rd32(body);
        r.tid = rd32(body + 4);
        r.addr = rd64(body + 8);
        r.len = rd64(body + 16);
        r.pgoff = rd64(body + 24);
        r.dev_major = rd32(body + 32);
        r.dev_minor = rd32(body + 36);
        r.ino = rd64(body + 40);
        // body + 48: inode generation
        r.prot = rd32(body + 56);
        r.map_flags = rd32(body + 60);
        r.text = c_string(body + 64, blen - tsize - 64);
        break;
      }
      default:
        r.kind = RecKind::Other;
        ++st.other;
        r.id = id;
        break;
    }
    out->push_back(std::move(r));
    off += size;
  }
done:
  if (consumed != nullptr) *consumed = off;
  return result;
}

Status ring_copy(const uint8_t* data_area, uint64_t data_size, uint64_t head, uint64_t tail, std::vector<uint8_t>* out) {
  if (data_size == 0 || (data_size & (data_size - 1)) != 0)
    return Status::error(Err::Invalid, "ring: the data area size " + std::to_string(data_size) + " is not a power of two");
  if (head < tail) return Status::error(Err::Invalid, "ring: head " + std::to_string(head) + " is behind tail " + std::to_string(tail));
  const uint64_t n = head - tail;
  if (n > data_size)
    return Status::error(Err::Invalid, "ring: " + std::to_string(n) + " unread bytes exceed the data area of " + std::to_string(data_size) + " (the reader fell behind)");
  if (n == 0) return Status::success();
  const uint64_t start = tail & (data_size - 1);
  const uint64_t first = std::min<uint64_t>(n, data_size - start);
  const size_t old = out->size();
  out->resize(old + static_cast<size_t>(n));
  std::memcpy(out->data() + old, data_area + start, static_cast<size_t>(first));
  if (first < n) std::memcpy(out->data() + old + first, data_area, static_cast<size_t>(n - first));
  return Status::success();
}

}  // namespace kz::trace

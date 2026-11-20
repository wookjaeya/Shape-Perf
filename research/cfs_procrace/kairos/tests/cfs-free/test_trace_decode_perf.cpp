// Unit tests of the perf record decoder on synthetic streams: every record kind, the corners
// of each layout, malformed input, and the ring arithmetic.  The layouts are also checked on
// kernel-written bytes (test_trace_decode_real).
#include <cstdlib>
#include <cstring>
#include <random>

#include "kz/trace/decode_perf.hpp"
#include "support/kz_test.hpp"
#include "support/perf_synth.hpp"

using namespace kz::trace;
using kzsyn::Stream;
namespace pa = kz::trace::perfabi;

namespace {

LayoutTable table(std::initializer_list<EventLayout> ls) {
  LayoutTable t;
  for (const EventLayout& l : ls) KZ_CHECK_OK(t.add(l));
  return t;
}

void test_layout_checks() {
  EventLayout l = kzsyn::side_layout(1);
  KZ_CHECK_OK(check_layout(l));
  EventLayout no_ident = l;
  no_ident.sample_type &= ~perfabi::kSampleIdentifier;
  KZ_CHECK_ERR(check_layout(no_ident), kz::Err::Invalid);
  EventLayout unknown_bit = l;
  unknown_bit.sample_type |= perfabi::kSampleCallchain;
  KZ_CHECK_ERR(check_layout(unknown_bit), kz::Err::Invalid);
  EventLayout stack_no_regs = kzsyn::probe_layout(2, 0, 0, 8);
  stack_no_regs.sample_type &= ~perfabi::kSampleRegsUser;
  KZ_CHECK_ERR(check_layout(stack_no_regs), kz::Err::Invalid);
  EventLayout regs_no_bit = l;
  regs_no_bit.regs_user = 1;
  KZ_CHECK_ERR(check_layout(regs_no_bit), kz::Err::Invalid);
  EventLayout big_reg = kzsyn::probe_layout(3, 0, 1u << 24, 0);
  KZ_CHECK_ERR(check_layout(big_reg), kz::Err::Invalid);
  EventLayout stack_bytes_no_bit = kzsyn::probe_layout(4, 0, 1, 0);
  stack_bytes_no_bit.stack_user = 16;
  KZ_CHECK_ERR(check_layout(stack_bytes_no_bit), kz::Err::Invalid);

  LayoutTable t;
  KZ_CHECK_OK(t.add(kzsyn::side_layout(7)));
  KZ_CHECK_ERR(t.add(kzsyn::side_layout(7)), kz::Err::Duplicate);
  KZ_CHECK_OK(t.add(kzsyn::side_layout(3)));
  KZ_CHECK_OK(t.add(kzsyn::side_layout(9)));
  KZ_CHECK_EQ(t.size(), size_t(3));
  KZ_CHECK(t.find(3) != nullptr && t.find(7) != nullptr && t.find(9) != nullptr);
  KZ_CHECK(t.find(4) == nullptr);
}

void test_sample_forms() {
  const EventLayout side = kzsyn::side_layout(10);
  const EventLayout entry = kzsyn::probe_layout(11, 0, (1u << 5) | (1u << 3) | (1u << 7), 16);
  const EventLayout plain = kzsyn::probe_layout(12, 1, (1u << 0), 0);
  const LayoutTable t = table({side, entry, plain});
  Stream s;
  Stream::SampleArgs a;
  a.ip = 0x1000; a.pid = 5; a.tid = 6; a.cpu = 2; a.time = 777;
  a.regs = {0x11, 0x22, 0x33};  // registers 3, 5, 7
  a.stack = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  s.sample(entry, a);
  // dyn_size shorter than the dump: only dyn bytes are meaningful
  a.stack_dyn = 4;
  s.sample(entry, a);
  // no stack at all (size 0: no dyn_size follows)
  a.stack.clear(); a.stack_size = 0;
  s.sample(entry, a);
  // abi 0: the task was not in user mode, no registers follow
  Stream::SampleArgs b;
  b.ip = 9; b.pid = 1; b.tid = 1; b.time = 5; b.abi = 0;
  s.sample(plain, b);
  Stream::SampleArgs c;
  c.ip = 0xdeadbeef; c.pid = 2; c.tid = 3; c.time = 42; c.cpu = 1; c.regs = {0xc2000005};
  s.sample(plain, c);

  std::vector<PerfRecord> r;
  ParseStats st;
  KZ_CHECK_OK(parse_records(s.bytes.data(), s.bytes.size(), 3, t, &r, &st));
  KZ_CHECK_EQ(r.size(), size_t(5));
  KZ_CHECK_EQ(st.samples, uint64_t(5));
  KZ_CHECK_EQ(r[0].regs_valid, uint64_t((1u << 3) | (1u << 5) | (1u << 7)));
  KZ_CHECK_EQ(r[0].regs[3], uint64_t(0x11));
  KZ_CHECK_EQ(r[0].regs[5], uint64_t(0x22));
  KZ_CHECK_EQ(r[0].regs[7], uint64_t(0x33));
  KZ_CHECK_EQ(r[0].regs[4], uint64_t(0));
  KZ_CHECK_EQ(r[0].stack.size(), size_t(16));
  KZ_CHECK_EQ(r[0].stack[15], uint8_t(16));
  KZ_CHECK_EQ(r[0].ip, uint64_t(0x1000));
  KZ_CHECK_EQ(r[0].pid, uint32_t(5));
  KZ_CHECK_EQ(r[0].tid, uint32_t(6));
  KZ_CHECK_EQ(r[0].cpu, uint32_t(2));
  KZ_CHECK_EQ(r[0].time, uint64_t(777));
  KZ_CHECK_EQ(r[0].buffer_cpu, uint16_t(3));
  KZ_CHECK_EQ(r[0].order, uint32_t(0));
  KZ_CHECK_EQ(r[4].order, uint32_t(4));
  KZ_CHECK_EQ(r[1].stack.size(), size_t(4));
  KZ_CHECK_EQ(r[1].stack_dyn, uint64_t(4));
  KZ_CHECK_EQ(r[2].stack.size(), size_t(0));
  KZ_CHECK_EQ(r[3].regs_valid, uint64_t(0));
  KZ_CHECK_EQ(r[3].ip, uint64_t(9));
  KZ_CHECK_EQ(r[4].regs[0], uint64_t(0xc2000005));
  KZ_CHECK_EQ(r[4].id, uint64_t(12));
}

void test_side_forms() {
  const EventLayout side = kzsyn::side_layout(10);
  const EventLayout probe = kzsyn::probe_layout(11, 0, 1, 0);
  const LayoutTable t = table({side, probe});
  Stream s;
  s.fork(side, 100, 99, 101, 100, 5000, 2);
  s.exit_(side, 100, 99, 101, 100, 6000, 2);
  s.comm(side, 100, 100, "core-cpu1", 4000, true, 1);
  s.comm(side, 100, 101, "A_15_char_name!", 5100, false, 0);  // exactly 15 characters
  s.comm(side, 100, 102, "", 5200);                           // empty name
  s.mmap2(side, 100, 100, 0x558d6017c000, 0x1000, 0x1000, 254, 0, 1966900, "/path/to/tgt", 4001);
  s.lost(probe, 17, 100, 101, 5300, 3);
  s.throttle(probe, true, 5400, 100, 101, 3);
  s.throttle(probe, false, 5900, 100, 101, 3);
  s.other(side, 8 /* PERF_RECORD_READ */, 7000);
  std::vector<PerfRecord> r;
  ParseStats st;
  KZ_CHECK_OK(parse_records(s.bytes.data(), s.bytes.size(), 0, t, &r, &st));
  KZ_CHECK_EQ(r.size(), size_t(10));
  KZ_CHECK_EQ(r[0].kind, RecKind::Fork);
  KZ_CHECK_EQ(r[0].pid, uint32_t(100));
  KZ_CHECK_EQ(r[0].ppid, uint32_t(99));
  KZ_CHECK_EQ(r[0].tid, uint32_t(101));
  KZ_CHECK_EQ(r[0].ptid, uint32_t(100));
  KZ_CHECK_EQ(r[0].time, uint64_t(5000));
  KZ_CHECK_EQ(r[0].cpu, uint32_t(2));
  KZ_CHECK_EQ(r[0].id, uint64_t(10));
  KZ_CHECK_EQ(r[1].kind, RecKind::Exit);
  KZ_CHECK_EQ(r[2].kind, RecKind::Comm);
  KZ_CHECK_EQ(r[2].text, std::string("core-cpu1"));
  KZ_CHECK((r[2].misc & perfabi::kMiscCommExec) != 0);
  KZ_CHECK_EQ(r[2].time, uint64_t(4000));
  KZ_CHECK_EQ(r[3].text, std::string("A_15_char_name!"));
  KZ_CHECK_EQ(r[3].tid, uint32_t(101));
  KZ_CHECK_EQ(r[4].text, std::string(""));
  KZ_CHECK_EQ(r[5].kind, RecKind::Mmap2);
  KZ_CHECK_EQ(r[5].addr, uint64_t(0x558d6017c000));
  KZ_CHECK_EQ(r[5].len, uint64_t(0x1000));
  KZ_CHECK_EQ(r[5].pgoff, uint64_t(0x1000));
  KZ_CHECK_EQ(r[5].dev_major, uint32_t(254));
  KZ_CHECK_EQ(r[5].ino, uint64_t(1966900));
  KZ_CHECK_EQ(r[5].text, std::string("/path/to/tgt"));
  KZ_CHECK_EQ(r[5].prot, uint32_t(5));
  KZ_CHECK_EQ(r[6].kind, RecKind::Lost);
  KZ_CHECK_EQ(r[6].lost, uint64_t(17));
  KZ_CHECK_EQ(r[6].id, uint64_t(11));
  KZ_CHECK_EQ(r[6].tid, uint32_t(101));
  KZ_CHECK_EQ(r[6].time, uint64_t(5300));
  KZ_CHECK_EQ(r[7].kind, RecKind::Throttle);
  KZ_CHECK_EQ(r[7].time, uint64_t(5400));
  KZ_CHECK_EQ(r[8].kind, RecKind::Unthrottle);
  KZ_CHECK_EQ(r[8].time, uint64_t(5900));
  KZ_CHECK_EQ(r[9].kind, RecKind::Other);
  KZ_CHECK_EQ(st.other, uint64_t(1));
}

void test_unknown_and_malformed() {
  const EventLayout side = kzsyn::side_layout(10);
  const LayoutTable t = table({side});
  // A record of an event outside the session: kept as Other, counted, not guessed at.
  {
    Stream s;
    s.fork(kzsyn::side_layout(999), 1, 1, 2, 1, 5);
    std::vector<PerfRecord> r;
    ParseStats st;
    KZ_CHECK_OK(parse_records(s.bytes.data(), s.bytes.size(), 0, t, &r, &st));
    KZ_CHECK_EQ(r.size(), size_t(1));
    KZ_CHECK_EQ(r[0].kind, RecKind::Other);
    KZ_CHECK_EQ(st.unknown_id, uint64_t(1));
  }
  // Header problems.
  auto parse = [&](std::vector<uint8_t> b, size_t* used = nullptr) {
    std::vector<PerfRecord> r;
    return parse_records(b.data(), b.size(), 0, t, &r, nullptr, used);
  };
  {
    std::vector<uint8_t> three(3, 0);
    KZ_CHECK_ERR(parse(three), kz::Err::Truncated);
    // size field below the header size
    std::vector<uint8_t> small = {7, 0, 0, 0, 0, 0, 4, 0};
    KZ_CHECK_ERR(parse(small), kz::Err::Invalid);
    // size not a multiple of 8
    std::vector<uint8_t> odd = {7, 0, 0, 0, 0, 0, 12, 0, 1, 2, 3, 4};
    KZ_CHECK_ERR(parse(odd), kz::Err::Invalid);
    // size beyond the stream
    std::vector<uint8_t> longer = {7, 0, 0, 0, 0, 0, 64, 0, 1, 2, 3, 4, 5, 6, 7, 8};
    KZ_CHECK_ERR(parse(longer), kz::Err::Truncated);
  }
  // A FORK record too short for its fields; sample records shorter than their layout.
  {
    Stream s;
    s.u64(10);  // an id, nothing else
    s.end(pa::kRecordFork);
    size_t used = 99;
    KZ_CHECK_ERR(parse(s.bytes, &used), kz::Err::Invalid);
    KZ_CHECK_EQ(used, size_t(0));
  }
  {
    const EventLayout probe = kzsyn::probe_layout(11, 0, 0x7, 16);
    const LayoutTable tp = table({probe});
    Stream::SampleArgs a;
    a.regs = {1, 2, 3};
    a.stack.assign(16, 0xaa);
    Stream good;
    good.sample(probe, a);
    std::vector<PerfRecord> r;
    KZ_CHECK_OK(parse_records(good.bytes.data(), good.bytes.size(), 0, tp, &r));
    // dyn_size larger than the dump is invalid
    Stream bad_dyn;
    a.stack_dyn = 17;
    bad_dyn.sample(probe, a);
    KZ_CHECK_ERR(parse_records(bad_dyn.bytes.data(), bad_dyn.bytes.size(), 0, tp, &r), kz::Err::Invalid);
    // a stack dump claiming more bytes than the record holds
    Stream big;
    a.stack_dyn = ~0ull;
    a.stack_size = 4096;
    big.sample(probe, a);
    // (the writer pads the claimed size with zeros, so shrink the record header to cut it off)
    std::vector<uint8_t> cut(big.bytes.begin(), big.bytes.begin() + 8 + 8 * 11);
    cut[6] = static_cast<uint8_t>(cut.size());
    cut[7] = static_cast<uint8_t>(cut.size() >> 8);
    KZ_CHECK_ERR(parse_records(cut.data(), cut.size(), 0, tp, &r), kz::Err::Truncated);
  }
}

// Whatever the bytes are, the parser stays inside its buffer and reports a position that does not
// run past the end.  Under ASan/UBSan this is also a memory-safety test.
void test_corruption_is_safe() {
  const EventLayout side = kzsyn::side_layout(10);
  const EventLayout entry = kzsyn::probe_layout(11, 0, (1u << 5) | (1u << 7), 8);
  const LayoutTable t = table({side, entry});
  Stream s;
  Stream::SampleArgs a;
  a.ip = 1; a.pid = 1; a.tid = 2; a.time = 3; a.regs = {5, 7};
  a.stack = {1, 2, 3, 4, 5, 6, 7, 8};
  s.fork(side, 1, 1, 2, 1, 10);
  s.comm(side, 1, 2, "thread-name", 11);
  s.sample(entry, a);
  s.mmap2(side, 1, 2, 0x1000, 0x2000, 0, 8, 1, 77, "/x/y", 12);
  s.lost(entry, 3, 1, 2, 13);
  s.exit_(side, 1, 1, 2, 1, 14);
  const std::vector<uint8_t> good = s.bytes;
  for (size_t i = 0; i < good.size(); ++i) {
    for (uint8_t mask : {uint8_t(0x01), uint8_t(0x80), uint8_t(0xff)}) {
      std::vector<uint8_t> b = good;
      b[i] ^= mask;
      std::vector<PerfRecord> r;
      size_t used = 0;
      (void)parse_records(b.data(), b.size(), 0, t, &r, nullptr, &used);
      KZ_CHECK(used <= b.size());
    }
  }
  std::mt19937_64 rng(12345);
  for (int round = 0; round < 3000; ++round) {
    std::vector<uint8_t> b = good;
    const int flips = 1 + static_cast<int>(rng() % 4);
    for (int k = 0; k < flips; ++k) b[rng() % b.size()] = static_cast<uint8_t>(rng());
    b.resize(rng() % (b.size() + 1));
    std::vector<PerfRecord> r;
    size_t used = 0;
    (void)parse_records(b.data(), b.size(), 0, t, &r, nullptr, &used);
    KZ_CHECK(used <= b.size());
  }
  // Pure noise.
  for (int round = 0; round < 500; ++round) {
    std::vector<uint8_t> b(rng() % 300);
    for (uint8_t& x : b) x = static_cast<uint8_t>(rng());
    std::vector<PerfRecord> r;
    size_t used = 0;
    (void)parse_records(b.data(), b.size(), 0, t, &r, nullptr, &used);
    KZ_CHECK(used <= b.size());
  }
}

void test_ring_copy() {
  // A ring of 16 bytes holding 0..15; head and tail grow without bound and wrap in the data area.
  std::vector<uint8_t> ring(16);
  for (int i = 0; i < 16; ++i) ring[i] = static_cast<uint8_t>(i);
  std::vector<uint8_t> out;
  KZ_CHECK_OK(ring_copy(ring.data(), 16, 8, 0, &out));  // no wrap
  KZ_CHECK_EQ(out.size(), size_t(8));
  for (int i = 0; i < 8; ++i) KZ_CHECK_EQ(out[i], uint8_t(i));
  out.clear();
  KZ_CHECK_OK(ring_copy(ring.data(), 16, 22, 12, &out));  // wraps: bytes 12..15 then 0..5
  const uint8_t want[10] = {12, 13, 14, 15, 0, 1, 2, 3, 4, 5};
  KZ_CHECK_EQ(out.size(), size_t(10));
  KZ_CHECK(std::memcmp(out.data(), want, 10) == 0);
  out.clear();
  KZ_CHECK_OK(ring_copy(ring.data(), 16, 16 + 16, 16, &out));  // a full ring, tail at the wrap point
  KZ_CHECK_EQ(out.size(), size_t(16));
  for (int i = 0; i < 16; ++i) KZ_CHECK_EQ(out[i], uint8_t(i));
  out.clear();
  KZ_CHECK_OK(ring_copy(ring.data(), 16, 5, 5, &out));  // empty
  KZ_CHECK_EQ(out.size(), size_t(0));
  // appends: the output vector keeps what it had
  out = {0xee};
  KZ_CHECK_OK(ring_copy(ring.data(), 16, 2, 0, &out));
  KZ_CHECK_EQ(out.size(), size_t(3));
  KZ_CHECK_EQ(out[0], uint8_t(0xee));
  // errors
  KZ_CHECK_ERR(ring_copy(ring.data(), 16, 17, 0, &out), kz::Err::Invalid);  // reader fell behind: 17 > 16
  KZ_CHECK_ERR(ring_copy(ring.data(), 16, 3, 5, &out), kz::Err::Invalid);   // head behind tail
  KZ_CHECK_ERR(ring_copy(ring.data(), 24, 3, 0, &out), kz::Err::Invalid);   // not a power of two
  KZ_CHECK_ERR(ring_copy(ring.data(), 0, 0, 0, &out), kz::Err::Invalid);
  // Property: random head/tail windows give exactly the bytes a byte-at-a-time reader would.
  std::mt19937_64 rng(777);
  for (int round = 0; round < 2000; ++round) {
    const uint64_t size = 1ull << (3 + rng() % 5);
    std::vector<uint8_t> big(size);
    for (uint8_t& x : big) x = static_cast<uint8_t>(rng());
    const uint64_t tail = rng() % (4 * size);
    const uint64_t n = rng() % (size + 1);
    std::vector<uint8_t> got;
    KZ_CHECK_OK(ring_copy(big.data(), size, tail + n, tail, &got));
    KZ_CHECK_EQ(got.size(), static_cast<size_t>(n));
    for (uint64_t i = 0; i < n; ++i) KZ_CHECK_EQ(got[i], big[(tail + i) % size]);
  }
}

}  // namespace

int main() {
  test_layout_checks();
  test_sample_forms();
  test_side_forms();
  test_unknown_and_malformed();
  test_corruption_is_safe();
  test_ring_copy();
  return kztest::finish("test_trace_decode_perf");
}
